/******************************************************************************
/ SnM_FXChain.cpp
/
/ Copyright (c) 2009-2010 Tim Payne (SWS), JF Bédague 
/ http://www.standingwaterstudios.com/reaper
/
/ Permission is hereby granted, free of charge, to any person obtaining a copy
/ of this software and associated documentation files (the "Software"), to deal
/ in the Software without restriction, including without limitation the rights to
/ use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
/ of the Software, and to permit persons to whom the Software is furnished to
/ do so, subject to the following conditions:
/ 
/ The above copyright notice and this permission notice shall be included in all
/ copies or substantial portions of the Software.
/ 
/ THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
/ EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
/ OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
/ NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
/ HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
/ WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/ FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
/ OTHER DEALINGS IN THE SOFTWARE.
/
******************************************************************************/

#include "stdafx.h"
#include "SnM_Actions.h"
#include "SNM_Chunk.h"
#include "SNM_FXChainView.h"


WDL_String g_fXChainClipboard;
extern SNM_ResourceWnd* g_pResourcesWnd; // SNM_ResourceView.cpp


///////////////////////////////////////////////////////////////////////////////
// Take FX chain
///////////////////////////////////////////////////////////////////////////////

// _set=false: paste, _set=true: set
void loadSetPasteTakeFXChain(const char* _title, int _slot, bool _activeOnly, bool _set, bool _errMsg)
{
	if (CountSelectedMediaItems(NULL))
	{
		// Prompt for slot if needed
		if (_slot == -1) _slot = g_fxChainFiles.PromptForSlot(_title); //loops on err
		if (_slot == -1) return; // user has cancelled

		// main job
		if (g_fxChainFiles.LoadOrBrowseSlot(_slot, _errMsg))
		{
			if (_set) 
				setTakeFXChain(_title, _slot, _activeOnly, false);
			else 
				pasteTakeFXChain(_title, _slot, _activeOnly);
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// Core functions

void pasteTakeFXChain(const char* _title, int _slot, bool _activeOnly)
{
	bool updated = false;

	// Get the source chain
	WDL_String* chain = NULL;
	WDL_String slotFxChain;
	if (_slot == -2) chain = &g_fXChainClipboard;
	else if (LoadChunk(g_fxChainFiles.Get(_slot)->m_fullPath.Get(), &slotFxChain)) chain = &slotFxChain;
	else return;

	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i+1,false); // doesn't include master
		for (int j = 0; tr && j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* item = GetTrackMediaItem(tr,j);
			if (item && *(bool*)GetSetMediaItemInfo(item,"B_UISEL",NULL))
			{
				SNM_TakeParserPatcher p(item, CountTakes(item));
				int tkIdx = (_activeOnly ? *(int*)GetSetMediaItemInfo(item, "I_CURTAKE", NULL) : 0);
				bool done = false;
				while (!done && tkIdx >= 0)
				{
					WDL_String takeChunk;
					int tkPos, tkOriginalLength;
					if (p.GetTakeChunk(tkIdx, &takeChunk, &tkPos, &tkOriginalLength)) 
					{
						SNM_ChunkParserPatcher ptk(&takeChunk);
						WDL_String* ptkChunk = ptk.GetChunk();

						// already a FX chain for this take?
						int eolTkFx = ptk.Parse(SNM_GET_SUBCHUNK_OR_LINE_EOL, 1, "TAKEFX", "<TAKEFX", 1, 0, 1);

						// paste FX chain (just before the end of the current TAKEFX)
						if (eolTkFx > 0) 
						{
							ptkChunk->Insert(chain->Get(), eolTkFx-2); //-2: before ">\n"
						}
						// set/create FX chain (after SOURCE)
						else 
						{
							int eolSrc = ptk.Parse(SNM_GET_SUBCHUNK_OR_LINE_EOL, 1, "SOURCE", "<SOURCE", -1, 0, 1);
							if (eolSrc > 0)
							{
								// no need of eolTakeFx-- ! eolTakeFx is the previous '\n' position
								WDL_String newTakeFx;
								makeChunkTakeFX(&newTakeFx, chain);
								ptkChunk->Insert(newTakeFx.Get(), eolSrc);
							}
						}
						// Update take
						updated = p.ReplaceTake(tkIdx, tkPos, tkOriginalLength, ptkChunk);
					}
					else done = true;

					if (_activeOnly) done = true;
					else tkIdx++;
				}
			}
		}
	}
	// Undo point
	if (updated)
		Undo_OnStateChangeEx(_title, UNDO_STATE_ALL, -1);
}

void setTakeFXChain(const char* _title, int _slot, bool _activeOnly, bool _clear)
{
	bool updated = false;

	// Get the source chain
	WDL_String* chain = NULL;
	WDL_String slotFxChain;
	switch(_slot)
	{
		case -1: 
			break;
		case -2: 
			chain = &g_fXChainClipboard;
			break;
		default: 
			if (LoadChunk(g_fxChainFiles.Get(_slot)->m_fullPath.Get(), &slotFxChain)) 
				chain = &slotFxChain;
			else
				return;
			break;
	}

	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i+1,false); // doesn't include master
		for (int j = 0; tr && j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* item = GetTrackMediaItem(tr,j);
			if (item && *(bool*)GetSetMediaItemInfo(item,"B_UISEL",NULL))
			{
				SNM_FXChainTakePatcher p(item);
				updated |= p.SetFXChain(_clear ? NULL : chain, _activeOnly);
			}
		}
	}
	// Undo point
	if (updated)
		Undo_OnStateChangeEx(_title, UNDO_STATE_ALL, -1);
}


///////////////////////////////////////////////////////////////////////////////
// Command functions 

void loadSetTakeFXChain(COMMAND_T* _ct){
	int slot = (int)_ct->user;
	loadSetPasteTakeFXChain(SNM_CMD_SHORTNAME(_ct), slot, true, true, slot < 0 || !g_fxChainFiles.Get(slot)->IsDefault());
}

void loadSetAllTakesFXChain(COMMAND_T* _ct){
	int slot = (int)_ct->user;
	loadSetPasteTakeFXChain(SNM_CMD_SHORTNAME(_ct), slot, false, true, slot < 0 || !g_fxChainFiles.Get(slot)->IsDefault());
}

void copyTakeFXChain(COMMAND_T* _ct)
{
	WDL_String* fxChain = NULL;
	for (int i = 0; !fxChain && i < GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i+1,false); // doesn't include master
		for (int j = 0; tr && j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* item = GetTrackMediaItem(tr,j);
			// stop to the 1st found chain
			if (!fxChain && item && *(bool*)GetSetMediaItemInfo(item,"B_UISEL",NULL))
			{
				SNM_FXChainTakePatcher p(item);
				fxChain = p.GetFXChain();
				if (fxChain) 
				{
					WDL_PtrList<const char> removedKeywords;
					removedKeywords.Add("WNDRECT");
					removedKeywords.Add("SHOW");
					removedKeywords.Add("FLOATPOS");
					removedKeywords.Add("LASTSEL");
					removedKeywords.Add("DOCKED");
					RemoveChunkLines(fxChain, &removedKeywords, true);
					g_fXChainClipboard.Set(fxChain->Get());
				}
			}
		}
	}
}

void cutTakeFXChain(COMMAND_T* _ct) {
	copyTakeFXChain(_ct);
	setTakeFXChain(SNM_CMD_SHORTNAME(_ct), -2, true, true);
}

void pasteTakeFXChain(COMMAND_T* _ct) {
	pasteTakeFXChain(SNM_CMD_SHORTNAME(_ct), -2, true);
}

void setTakeFXChain(COMMAND_T* _ct) {
	setTakeFXChain(SNM_CMD_SHORTNAME(_ct), -2, true, false);
}

void pasteAllTakesFXChain(COMMAND_T* _ct) {
	pasteTakeFXChain(SNM_CMD_SHORTNAME(_ct), -2, false);
}

void setAllTakesFXChain(COMMAND_T* _ct) {
	setTakeFXChain(SNM_CMD_SHORTNAME(_ct), -2, false, false);
}

void clearActiveTakeFXChain(COMMAND_T* _ct) {
	setTakeFXChain(SNM_CMD_SHORTNAME(_ct), (int)_ct->user, true, true);
}

void clearAllTakesFXChain(COMMAND_T* _ct) {
	setTakeFXChain(SNM_CMD_SHORTNAME(_ct), (int)_ct->user, false, true);
}


///////////////////////////////////////////////////////////////////////////////
// Track FX chain
///////////////////////////////////////////////////////////////////////////////

// _set=false: paste, _set=true: set
void loadSetPasteTrackFXChain(const char* _title, int _slot, bool _set, bool _errMsg)
{
	if (CountSelectedTracksWithMaster(NULL))
	{
		// Prompt for slot if needed
		if (_slot == -1) _slot = g_fxChainFiles.PromptForSlot(_title); //loops on err
		if (_slot == -1) return; // user has cancelled

		// main job
		if (g_fxChainFiles.LoadOrBrowseSlot(_slot, _errMsg))
		{
			if (_set) 
				setTrackFXChain(_title, _slot, false);
			else 
				pasteTrackFXChain(_title, _slot);
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// Core functions 

void pasteTrackFXChain(const char* _title, int _slot)
{
	bool updated = false;

	// Get the source chain
	WDL_String* chain = NULL;
	WDL_String slotFxChain;
	if (_slot == -2) chain = &g_fXChainClipboard;
	else if (LoadChunk(g_fxChainFiles.Get(_slot)->m_fullPath.Get(), &slotFxChain)) chain = &slotFxChain;
	else return;

	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i,false); // include master
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			SNM_FXChainTrackPatcher p(tr);
			int fxCount = TrackFX_GetCount(tr);
			bool patch = false;
			
			// paste
			if (fxCount > 0) 
				patch = p.InsertAfterBefore(1, chain->Get(), "FXCHAIN", "WAK", 2, fxCount-1);
			// no fx => set
			else 
				patch = p.SetFXChain(chain);

			updated |= patch;
		}
	}
	// Undo point
	if (updated)
		Undo_OnStateChangeEx(_title, UNDO_STATE_ALL, -1);
}

//JFB3 chain plutot en param ??
void setTrackFXChain(const char* _title, int _slot, bool _clear)
{
	bool updated = false;

	// Get the source chain
	WDL_String* chain = NULL;
	WDL_String slotFxChain;
	switch(_slot)
	{
		case -1: 
			break;
		case -2: 
			chain = &g_fXChainClipboard;
			break;
		default: 
			if (LoadChunk(g_fxChainFiles.Get(_slot)->m_fullPath.Get(), &slotFxChain)) 
				chain = &slotFxChain;
			else
				return;
			break;
	}

	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i,false); // include master
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			SNM_FXChainTrackPatcher p(tr);
			bool patch = p.SetFXChain(_clear ? NULL : chain);
			updated |= patch;
		}
	}
	// Undo point
	if (updated)
		Undo_OnStateChangeEx(_title, UNDO_STATE_ALL, -1);
}


///////////////////////////////////////////////////////////////////////////////
// Command functions 

void loadSetTrackFXChain(COMMAND_T* _ct) {
	int slot = (int)_ct->user;
	loadSetPasteTrackFXChain(SNM_CMD_SHORTNAME(_ct), slot, true, slot < 0 || !g_fxChainFiles.Get(slot)->IsDefault());
}

void clearTrackFXChain(COMMAND_T* _ct) {
	setTrackFXChain(SNM_CMD_SHORTNAME(_ct), (int)_ct->user, true);
}

void copyTrackFXChain(COMMAND_T* _ct)
{
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i,false); // include master
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			SNM_ChunkParserPatcher p(tr);
			if (p.GetSubChunk("FXCHAIN", 2, 0, &g_fXChainClipboard))
			{
				// remove last ">\n" 
				g_fXChainClipboard.DeleteSub(g_fXChainClipboard.GetLength()-3,2);

				WDL_PtrList<const char> removedKeywords;
				removedKeywords.Add("<FXCHAIN");
				removedKeywords.Add("WNDRECT");
				removedKeywords.Add("SHOW");
				removedKeywords.Add("FLOATPOS");
				removedKeywords.Add("LASTSEL");
				removedKeywords.Add("DOCKED");
				RemoveChunkLines(&g_fXChainClipboard, &removedKeywords, true);
//JFB TODO: stop at first ?
			}
		}
	}
}

void cutTrackFXChain(COMMAND_T* _ct) {
	copyTrackFXChain(_ct);
	setTrackFXChain(SNM_CMD_SHORTNAME(_ct), -2, true);
}

void pasteTrackFXChain(COMMAND_T* _ct) {
	pasteTrackFXChain(SNM_CMD_SHORTNAME(_ct), -2);
}

void setTrackFXChain(COMMAND_T* _ct) {
	setTrackFXChain(SNM_CMD_SHORTNAME(_ct), -2, false);
}


///////////////////////////////////////////////////////////////////////////////
// Common
///////////////////////////////////////////////////////////////////////////////

void makeChunkTakeFX(WDL_String* _outTakeFX, WDL_String* _inRfxChain)
{
	if (_outTakeFX && _inRfxChain)
	{
		// JFB TODO: one single append..
		_outTakeFX->Append("<TAKEFX\n");
		_outTakeFX->Append("WNDRECT 0 0 0 0\n"); 
		_outTakeFX->Append("SHOW 0\n"); // un-float fx chain window
		_outTakeFX->Append("LASTSEL 1\n");
		_outTakeFX->Append("DOCKED 0\n");
		_outTakeFX->Append(_inRfxChain->Get());
		_outTakeFX->Append(">\n");
	}
}

void clearFXChainSlotPrompt(COMMAND_T* _ct)
{
	int slot = g_fxChainFiles.PromptForSlot(SNM_CMD_SHORTNAME(_ct)); //loops on err
	if (slot == -1) return; // user has cancelled
	else g_fxChainFiles.ClearSlot(slot);
}

void copySlotToClipBoard(int _slot)
{
	if (_slot >= 0 && _slot < g_fxChainFiles.GetSize())
		LoadChunk(g_fxChainFiles.Get(_slot)->m_fullPath.Get(), &g_fXChainClipboard);
}

void readSlotIniFile(const char* _key, int _slot, char* _path, int _pathSize, char* _desc, int _descSize)
{
	char buf[32];
	_snprintf(buf, 32, "SLOT%d", _slot+1);
	GetPrivateProfileString(_key, buf, "", _path, _pathSize, g_SNMiniFilename.Get());
	_snprintf(buf, 32, "DESC%d", _slot+1);
	GetPrivateProfileString(_key, buf, "", _desc, _descSize, g_SNMiniFilename.Get());
}

void saveSlotIniFile(const char* _key, int _slot, const char* _path, const char* _desc)
{
	char buf[32] = "";
	_snprintf(buf, 32, "SLOT%d", _slot+1);
	WritePrivateProfileString(_key, buf, (_path && *_path) ? _path : NULL, g_SNMiniFilename.Get());
	_snprintf(buf, 32, "DESC%d", _slot+1);
	WritePrivateProfileString(_key, buf, (_desc && *_desc) ? _desc : NULL, g_SNMiniFilename.Get());	
}
