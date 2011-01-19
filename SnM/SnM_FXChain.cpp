/******************************************************************************
/ SnM_FXChain.cpp
/
/ Copyright (c) 2009-2010 Tim Payne (SWS), Jeffos
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

// _slot: 0-based, -1 will prompt for slot
// _set=false: paste, _set=true: set
void applyTakesFXChainSlot(const char* _title, int _slot, bool _activeOnly, bool _set, bool _errMsg)
{
	// Prompt for slot if needed
	if (_slot == -1) _slot = g_fxChainFiles.PromptForSlot(_title); //loops on err
	if (_slot == -1) return; // user has cancelled

	char fn[BUFFER_SIZE]="";
	if (g_fxChainFiles.GetOrBrowseSlot(_slot, fn, BUFFER_SIZE, _errMsg) && 
		CountSelectedMediaItems(NULL))
	{
		WDL_String chain;
		if (LoadChunk(fn, &chain))
		{
			if (_set) 
				setTakeFXChain(_title, &chain, _activeOnly);
			else 
				pasteTakeFXChain(_title, &chain, _activeOnly);
		}
	}
}

void pasteTakeFXChain(const char* _title, WDL_String* _chain, bool _activeOnly)
{
	bool updated = false;
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
							ptkChunk->Insert(_chain->Get(), eolTkFx-2); //-2: before ">\n"
						}
						// set/create FX chain (after SOURCE)
						else 
						{
							int eolSrc = ptk.Parse(SNM_GET_SUBCHUNK_OR_LINE_EOL, 1, "SOURCE", "<SOURCE", -1, 0, 1);
							if (eolSrc > 0)
							{
								// no need of eolTakeFx-- ! eolTakeFx is the previous '\n' position
								WDL_String newTakeFx;
								makeChunkTakeFX(&newTakeFx, _chain);
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

// _chain: NULL clears the FX chain
void setTakeFXChain(const char* _title, WDL_String* _chain, bool _activeOnly)
{
	bool updated = false;
	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i+1,false); // doesn't include master
		for (int j = 0; tr && j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* item = GetTrackMediaItem(tr,j);
			if (item && *(bool*)GetSetMediaItemInfo(item,"B_UISEL",NULL))
			{
				SNM_FXChainTakePatcher p(item);
				updated |= p.SetFXChain(_chain, _activeOnly);
			}
		}
	}
	// Undo point
	if (updated)
		Undo_OnStateChangeEx(_title, UNDO_STATE_ALL, -1);
}

void makeChunkTakeFX(WDL_String* _outTakeFX, WDL_String* _inRfxChain)
{
	if (_outTakeFX && _inRfxChain)
	{
		_outTakeFX->Append("<TAKEFX\nWNDRECT 0 0 0 0\nSHOW 0\nLASTSEL 1\nDOCKED 0\n");
		_outTakeFX->Append(_inRfxChain->Get());
		_outTakeFX->Append(">\n");
	}
}


///////////////////////////////////////////////////////////////////////////////
// Command functions 

void loadSetTakeFXChain(COMMAND_T* _ct){
	int slot = (int)_ct->user;
	applyTakesFXChainSlot(SNM_CMD_SHORTNAME(_ct), slot, true, true, slot < 0 || !g_fxChainFiles.Get(slot)->IsDefault());
}

void loadPasteTakeFXChain(COMMAND_T* _ct){
	int slot = (int)_ct->user;
	applyTakesFXChainSlot(SNM_CMD_SHORTNAME(_ct), slot, true, false, slot < 0 || !g_fxChainFiles.Get(slot)->IsDefault());
}

void loadSetAllTakesFXChain(COMMAND_T* _ct){
	int slot = (int)_ct->user;
	applyTakesFXChainSlot(SNM_CMD_SHORTNAME(_ct), slot, false, true, slot < 0 || !g_fxChainFiles.Get(slot)->IsDefault());
}

void loadPasteAllTakesFXChain(COMMAND_T* _ct){
	int slot = (int)_ct->user;
	applyTakesFXChainSlot(SNM_CMD_SHORTNAME(_ct), slot, false, false, slot < 0 || !g_fxChainFiles.Get(slot)->IsDefault());
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
	setTakeFXChain(SNM_CMD_SHORTNAME(_ct), NULL, true);
}

void pasteTakeFXChain(COMMAND_T* _ct) {
	pasteTakeFXChain(SNM_CMD_SHORTNAME(_ct), &g_fXChainClipboard, true);
}

void setTakeFXChain(COMMAND_T* _ct) {
	setTakeFXChain(SNM_CMD_SHORTNAME(_ct), &g_fXChainClipboard, true);
}

void pasteAllTakesFXChain(COMMAND_T* _ct) {
	pasteTakeFXChain(SNM_CMD_SHORTNAME(_ct), &g_fXChainClipboard, false);
}

void setAllTakesFXChain(COMMAND_T* _ct) {
	setTakeFXChain(SNM_CMD_SHORTNAME(_ct), &g_fXChainClipboard, false);
}

void clearActiveTakeFXChain(COMMAND_T* _ct) {
	setTakeFXChain(SNM_CMD_SHORTNAME(_ct), NULL, true);
}

void clearAllTakesFXChain(COMMAND_T* _ct) {
	setTakeFXChain(SNM_CMD_SHORTNAME(_ct), NULL, false);
}


///////////////////////////////////////////////////////////////////////////////
// Track FX chain
///////////////////////////////////////////////////////////////////////////////

// _set=false: paste, _set=true: set
void applyTracksFXChainSlot(const char* _title, int _slot, bool _set, bool _errMsg)
{
	// Prompt for slot if needed
	if (_slot == -1) _slot = g_fxChainFiles.PromptForSlot(_title); //loops on err
	if (_slot == -1) return; // user has cancelled

	char fn[BUFFER_SIZE]="";
	if (g_fxChainFiles.GetOrBrowseSlot(_slot, fn, BUFFER_SIZE, _errMsg) &&
		CountSelectedTracksWithMaster(NULL))
	{
		WDL_String chain;
		if (LoadChunk(fn, &chain))
		{
			if (_set) 
				setTrackFXChain(_title, &chain);
			else 
				pasteTrackFXChain(_title, &chain);
		}
	}
}

void pasteTrackFXChain(const char* _title, WDL_String* _chain)
{
	bool updated = false;
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
				patch = p.InsertAfterBefore(1, _chain->Get(), "FXCHAIN", "WAK", 2, fxCount-1);
			// no fx => set
			else 
				patch = p.SetFXChain(_chain);

			updated |= patch;
		}
	}
	// Undo point
	if (updated)
		Undo_OnStateChangeEx(_title, UNDO_STATE_ALL, -1);
}

// _chain: NULL to clear
void setTrackFXChain(const char* _title, WDL_String* _chain)
{
	bool updated = false;
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i,false); // include master
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			SNM_FXChainTrackPatcher p(tr);
			bool patch = p.SetFXChain(_chain);
			updated |= patch;
		}
	}
	// Undo point
	if (updated)
		Undo_OnStateChangeEx(_title, UNDO_STATE_ALL, -1);
}

// returns the first copied track idx (1-based, 0 = master)
int copyTrackFXChain(WDL_String* _fxChain, int _startTr)
{
	for (int i = _startTr; i >= 0 && i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i,false); // include master
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			SNM_ChunkParserPatcher p(tr);
			if (p.GetSubChunk("FXCHAIN", 2, 0, _fxChain))
			{
				WDL_PtrList<const char> removedKeywords;
				removedKeywords.Add("<FXCHAIN");
				removedKeywords.Add("WNDRECT");
				removedKeywords.Add("SHOW");
				removedKeywords.Add("FLOATPOS");
				removedKeywords.Add("LASTSEL");
				removedKeywords.Add("DOCKED");
				RemoveChunkLines(_fxChain, &removedKeywords, true);

				// remove last ">\n" 
				_fxChain->DeleteSub(_fxChain->GetLength()-2,2);
				return i;
			}
		}
	}
	return -1;
}

// assumes _fn has a length of BUFFER_SIZE
bool autoSaveTrackFXChainSlots(int _slot, const char* _dirPath, char* _fn)
{
	bool slotUpdate = false;
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i,false); 
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			WDL_String fxChain("");
			if (copyTrackFXChain(&fxChain, i) == i)
			{
				RemoveAllIds(&fxChain);
				char* trName = (char*)GetSetMediaTrackInfo(tr, "P_NAME", NULL);
				GenerateFilename(_dirPath, (!trName || *trName == '\0') ? "Untitled" : trName, g_fxChainFiles.GetFileExt(), _fn, BUFFER_SIZE);
				slotUpdate |= (SaveChunk(_fn, &fxChain) && g_fxChainFiles.InsertSlot(_slot, _fn));
			}
			else if (!fxChain.GetLength())
			{
				// for displayed err. msg
				strncpy(_fn, "<Empty FX chain>", BUFFER_SIZE);
			}
		}
	}
	return slotUpdate;
}


///////////////////////////////////////////////////////////////////////////////
// Command functions 

void loadSetTrackFXChain(COMMAND_T* _ct) {
	int slot = (int)_ct->user;
	applyTracksFXChainSlot(SNM_CMD_SHORTNAME(_ct), slot, true, slot < 0 || !g_fxChainFiles.Get(slot)->IsDefault());
}

void loadPasteTrackFXChain(COMMAND_T* _ct) {
	int slot = (int)_ct->user;
	applyTracksFXChainSlot(SNM_CMD_SHORTNAME(_ct), slot, false, slot < 0 || !g_fxChainFiles.Get(slot)->IsDefault());
}

void clearTrackFXChain(COMMAND_T* _ct) {
	setTrackFXChain(SNM_CMD_SHORTNAME(_ct), NULL);
}

void copyTrackFXChain(COMMAND_T* _ct) {
	copyTrackFXChain(&g_fXChainClipboard);
}

void cutTrackFXChain(COMMAND_T* _ct) {
	copyTrackFXChain(_ct);
	setTrackFXChain(SNM_CMD_SHORTNAME(_ct), NULL);
}

void pasteTrackFXChain(COMMAND_T* _ct) {
	pasteTrackFXChain(SNM_CMD_SHORTNAME(_ct), &g_fXChainClipboard);
}

void setTrackFXChain(COMMAND_T* _ct) {
	setTrackFXChain(SNM_CMD_SHORTNAME(_ct), &g_fXChainClipboard);
}


///////////////////////////////////////////////////////////////////////////////
// Common
///////////////////////////////////////////////////////////////////////////////

void copyFXChainSlotToClipBoard(int _slot)
{
	if (_slot >= 0 && _slot < g_fxChainFiles.GetSize()) 
	{
		char fullPath[BUFFER_SIZE] = "";
		g_fxChainFiles.GetFullPath(_slot, fullPath, BUFFER_SIZE);
		LoadChunk(fullPath, &g_fXChainClipboard);
	}
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

