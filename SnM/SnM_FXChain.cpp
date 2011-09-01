/******************************************************************************
/ SnM_FXChain.cpp
/
/ Copyright (c) 2009-2011 Tim Payne (SWS), Jeffos
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
// Take FX chains
///////////////////////////////////////////////////////////////////////////////

void makeChunkTakeFX(WDL_String* _outTakeFX, WDL_String* _inRfxChain)
{
	if (_outTakeFX && _inRfxChain)
	{
		_outTakeFX->Append("<TAKEFX\nWNDRECT 0 0 0 0\nSHOW 0\nLASTSEL 1\nDOCKED 0\n");
		_outTakeFX->Append(_inRfxChain->Get());
		_outTakeFX->Append(">\n");
	}
}

int copyTakeFXChain(WDL_String* _fxChain, int _startSelItem)
{
	for (int i = _startSelItem; _fxChain && i < CountSelectedMediaItems(NULL); i++)
	{
		MediaItem* item = GetSelectedMediaItem(NULL, i);
		if (item)
		{
			SNM_FXChainTakePatcher p(item);
			WDL_String* fxChain = p.GetFXChain();
			if (fxChain) 
			{
				WDL_PtrList<const char> removedKeywords;
				removedKeywords.Add("WNDRECT");
				removedKeywords.Add("SHOW");
				removedKeywords.Add("FLOATPOS");
				removedKeywords.Add("LASTSEL");
				removedKeywords.Add("DOCKED");
				RemoveChunkLines(fxChain, &removedKeywords, true);
				_fxChain->Set(fxChain->Get());
				return i;
			}
		}
	}
	return -1;
}

void pasteTakeFXChain(const char* _title, WDL_String* _chain, bool _activeOnly)
{
	bool updated = false;
	if (_chain && _chain->GetLength())
	{
		for (int i = 1; i <= GetNumTracks(); i++) // skip master
		{
			MediaTrack* tr = CSurf_TrackFromID(i, false);
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
						int tkPos, tklen;
						if (p.GetTakeChunk(tkIdx, &takeChunk, &tkPos, &tklen)) 
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
							updated = p.ReplaceTake(tkPos, tklen, ptkChunk);
						}
						else done = true;

						if (_activeOnly) done = true;
						else tkIdx++;
					}
				}
			}
		}
	}
	if (updated)
		Undo_OnStateChangeEx(_title, UNDO_STATE_ALL, -1);
}

// _chain: NULL clears the FX chain
void setTakeFXChain(const char* _title, WDL_String* _chain, bool _activeOnly)
{
	bool updated = false;
	for (int i = 1; i <= GetNumTracks(); i++) // skip master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
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
	if (updated)
		Undo_OnStateChangeEx(_title, UNDO_STATE_ALL, -1);
}


///////////////////////////////////////////////////////////////////////////////
// Take FX chain slots (Resources view)
///////////////////////////////////////////////////////////////////////////////

// _slot: 0-based, -1 will prompt for slot
// _set=false: paste, _set=true: set
void applyTakesFXChainSlot(const char* _title, int _slot, bool _activeOnly, bool _set, bool _errMsg)
{
	// Prompt for slot if needed
	if (_slot == -1) _slot = g_fxChainFiles.PromptForSlot(_title); //loops on err
	if (_slot == -1) return; // user has cancelled

	char fn[BUFFER_SIZE] = "";
	if (g_fxChainFiles.GetOrBrowseSlot(_slot, fn, BUFFER_SIZE, _errMsg) && CountSelectedMediaItems(NULL))
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
bool autoSaveItemFXChainSlots(int _slot, const char* _dirPath, char* _fn, int _fnSize)
{
	bool slotUpdate = false;
	lstrcpyn(_fn, "<No selected item>", _fnSize); // default for err. msg

	for (int i=0; i < CountSelectedMediaItems(NULL); i++)
	{
		MediaItem* item = GetSelectedMediaItem(NULL, i);
		if (item)
		{
			WDL_String fxChain("");
			if (copyTakeFXChain(&fxChain, i) == i)
			{
				RemoveAllIds(&fxChain);

				char* itemName = GetName(item);
				GenerateFilename(_dirPath, (!itemName || *itemName == '\0') ? "Untitled" : itemName, g_fxChainFiles.GetFileExt(), _fn, _fnSize);
/*JFB insert slot code commented: can mess the user's slot actions (because all following ids change)
				slotUpdate |= (SaveChunk(_fn, &fxChain) && g_fxChainFiles.InsertSlot(_slot, _fn));
*/
				slotUpdate |= (SaveChunk(_fn, &fxChain) && g_fxChainFiles.AddSlot(_fn));
			}
			else if (!fxChain.GetLength())
				lstrcpyn(_fn, "<Empty FX chain>", _fnSize); //JFB // for displayed err. msg
		}
	}
	return slotUpdate;
}

// Commands

void loadSetTakeFXChain(COMMAND_T* _ct) {
	int slot = (int)_ct->user;
	if (slot < 0 || slot < g_fxChainFiles.GetSize())
		applyTakesFXChainSlot(SNM_CMD_SHORTNAME(_ct), slot, true, true, slot < 0 || !g_fxChainFiles.Get(slot)->IsDefault());
}

void loadPasteTakeFXChain(COMMAND_T* _ct) {
	int slot = (int)_ct->user;
	if (slot < 0 || slot < g_fxChainFiles.GetSize())
		applyTakesFXChainSlot(SNM_CMD_SHORTNAME(_ct), slot, true, false, slot < 0 || !g_fxChainFiles.Get(slot)->IsDefault());
}

void loadSetAllTakesFXChain(COMMAND_T* _ct) {
	int slot = (int)_ct->user;
	if (slot < 0 || slot < g_fxChainFiles.GetSize())
		applyTakesFXChainSlot(SNM_CMD_SHORTNAME(_ct), slot, false, true, slot < 0 || !g_fxChainFiles.Get(slot)->IsDefault());
}

void loadPasteAllTakesFXChain(COMMAND_T* _ct) {
	int slot = (int)_ct->user;
	if (slot < 0 || slot < g_fxChainFiles.GetSize())
		applyTakesFXChainSlot(SNM_CMD_SHORTNAME(_ct), slot, false, false, slot < 0 || !g_fxChainFiles.Get(slot)->IsDefault());
}

void copyTakeFXChain(COMMAND_T* _ct) {
	copyTakeFXChain(&g_fXChainClipboard);
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
// Track FX chains
///////////////////////////////////////////////////////////////////////////////

// best effort for http://code.google.com/p/sws-extension/issues/detail?id=363
// update the track's nb of channels if that info exists (as a comment) in the provided chunk
// return true if update done
bool SetTrackChannelsForFXChain(MediaTrack* _tr, WDL_String* _chain)
{
	if (_tr && _chain && !strncmp(_chain->Get(), "#NCHAN ", 7))
	{
		int nbChInChain = atoi((const char*)(_chain->Get()+7));
		if (nbChInChain && !(nbChInChain % 2)) // valid and even?
		{
			int nbChInTr = *(int*)GetSetMediaTrackInfo(_tr, "I_NCHAN", NULL);
			if (nbChInChain > nbChInTr)
			{
				GetSetMediaTrackInfo(_tr, "I_NCHAN", &nbChInChain);
				return true;
			}
		}
	}
	return false;
}

void pasteTrackFXChain(const char* _title, WDL_String* _chain, bool _inputFX)
{
	bool updated = false;
	if (_chain && _chain->GetLength())
	{
		for (int i = 0; i <= GetNumTracks(); i++) // include master
		{
			MediaTrack* tr = CSurf_TrackFromID(i, false);
			if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			{
				// try to set track channels
				updated |= SetTrackChannelsForFXChain(tr, _chain);

				// the meat
				bool patch = false;
				SNM_FXChainTrackPatcher p(tr);
				WDL_String currentFXChain;
				int pos = p.GetSubChunk(_inputFX ? "FXCHAIN_REC" : "FXCHAIN", 2, 0, &currentFXChain, "<ITEM");

				// paste (well.. insert at the end of the current FX chain)
				if (pos >= 0) 
				{
					patch = true;
					p.GetChunk()->Insert(_chain->Get(), pos + currentFXChain.GetLength() - 2); // -2: before ">\n"
					p.SetUpdates(1);
				}
				// create fx chain
				else 
					patch = p.SetFXChain(_chain, _inputFX);

				updated |= patch;
			}
		}
	}
	if (updated)
		Undo_OnStateChangeEx(_title, UNDO_STATE_ALL, -1);
}

// _chain: NULL to clear
void setTrackFXChain(const char* _title, WDL_String* _chain, bool _inputFX)
{
	bool updated = false;
	for (int i = 0; i <= GetNumTracks(); i++) // include master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			// try to set track channels
			updated |= SetTrackChannelsForFXChain(tr, _chain);

			// the meat
			SNM_FXChainTrackPatcher p(tr);
			bool patch = p.SetFXChain(_chain, _inputFX);
			updated |= patch;
		}
	}
	if (updated)
		Undo_OnStateChangeEx(_title, UNDO_STATE_ALL, -1);
}

// returns the first copied track idx (1-based, 0 = master)
int copyTrackFXChain(WDL_String* _fxChain, bool _inputFX, int _startTr)
{
	for (int i = _startTr; i >= 0 && i <= GetNumTracks(); i++) // include master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			SNM_ChunkParserPatcher p(tr);
			if (p.GetSubChunk(_inputFX ? "FXCHAIN_REC" : "FXCHAIN", 2, 0, _fxChain, "<ITEM") >= 0)
			{
				WDL_PtrList<const char> removedKeywords;
				removedKeywords.Add(_inputFX ? "<FXCHAIN_REC" : "<FXCHAIN");
				removedKeywords.Add("WNDRECT");
				removedKeywords.Add("SHOW");
				removedKeywords.Add("FLOATPOS");
				removedKeywords.Add("LASTSEL");
				removedKeywords.Add("DOCKED");
				RemoveChunkLines(_fxChain, &removedKeywords, true);

				// remove last ">\n" 
				_fxChain->DeleteSub(_fxChain->GetLength()-2, 2);
				return i;
			}
		}
	}
	return -1;
}

///////////////////////////////////////////////////////////////////////////////
// Track FX chain slots (Resources view)
///////////////////////////////////////////////////////////////////////////////

// _set=false => paste
void applyTracksFXChainSlot(const char* _title, int _slot, bool _set, bool _inputFX, bool _errMsg)
{
	// Prompt for slot if needed
	if (_slot == -1) _slot = g_fxChainFiles.PromptForSlot(_title); //loops on err
	if (_slot == -1) return; // user has cancelled

	char fn[BUFFER_SIZE]="";
	if (g_fxChainFiles.GetOrBrowseSlot(_slot, fn, BUFFER_SIZE, _errMsg) && CountSelectedTracksWithMaster(NULL))
	{
		WDL_String chain;
		if (LoadChunk(fn, &chain))
		{
			if (_set) 
				setTrackFXChain(_title, &chain, _inputFX);
			else 
				pasteTrackFXChain(_title, &chain, _inputFX);
		}
	}
}

bool autoSaveTrackFXChainSlots(int _slot, bool _inputFX, const char* _dirPath, char* _fn, int _fnSize)
{
	bool slotUpdate = false;
	lstrcpyn(_fn, "<No selected track>", _fnSize); //default for err. msg

	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i,false); 
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			WDL_String fxChain("");
			if (copyTrackFXChain(&fxChain, _inputFX, i) == i)
			{
				RemoveAllIds(&fxChain);

				// add track channels (as a comment so that it doesn't bother REAPER)
				// i.e. best effort for http://code.google.com/p/sws-extension/issues/detail?id=363
				WDL_String nbChStr;
				nbChStr.SetFormatted(32, "#NCHAN %d\n", *(int*)GetSetMediaTrackInfo(tr, "I_NCHAN", NULL));
				fxChain.Insert(nbChStr.Get(), 0);

				char* trName = (char*)GetSetMediaTrackInfo(tr, "P_NAME", NULL);
				char autoSlotName[256] = "";
				_snprintf(autoSlotName, 256, "%s%s", (!trName || *trName == '\0') ? "Untitled" : trName, _inputFX ? "_inputFX" : "");
				GenerateFilename(_dirPath, autoSlotName, g_fxChainFiles.GetFileExt(), _fn, _fnSize);
/*JFB insert slot code commented: can mess the user's slot actions (because all following ids change)
				slotUpdate |= (SaveChunk(_fn, &fxChain) && g_fxChainFiles.InsertSlot(_slot, _fn));
*/
				slotUpdate |= (SaveChunk(_fn, &fxChain) && g_fxChainFiles.AddSlot(_fn));
			}
			else if (!fxChain.GetLength())
				lstrcpyn(_fn, "<Empty FX chain>", _fnSize); //JFB bof.. for displayed err. msg
		}
	}
	return slotUpdate;
}


// Commands

void loadSetTrackFXChain(COMMAND_T* _ct) {
	int slot = (int)_ct->user;
	if (slot < 0 || slot < g_fxChainFiles.GetSize())
		applyTracksFXChainSlot(SNM_CMD_SHORTNAME(_ct), slot, true, false, slot < 0 || !g_fxChainFiles.Get(slot)->IsDefault());
}

void loadPasteTrackFXChain(COMMAND_T* _ct) {
	int slot = (int)_ct->user;
	if (slot < 0 || slot < g_fxChainFiles.GetSize())
		applyTracksFXChainSlot(SNM_CMD_SHORTNAME(_ct), slot, false, false, slot < 0 || !g_fxChainFiles.Get(slot)->IsDefault());
}

void loadSetTrackInFXChain(COMMAND_T* _ct) {
	int slot = (int)_ct->user;
	if (slot < 0 || slot < g_fxChainFiles.GetSize())
		applyTracksFXChainSlot(SNM_CMD_SHORTNAME(_ct), slot, true, g_bv4, slot < 0 || !g_fxChainFiles.Get(slot)->IsDefault());
}

void loadPasteTrackInFXChain(COMMAND_T* _ct) {
	int slot = (int)_ct->user;
	if (slot < 0 || slot < g_fxChainFiles.GetSize())
		applyTracksFXChainSlot(SNM_CMD_SHORTNAME(_ct), slot, false, g_bv4, slot < 0 || !g_fxChainFiles.Get(slot)->IsDefault());
}

void clearTrackFXChain(COMMAND_T* _ct) {
	setTrackFXChain(SNM_CMD_SHORTNAME(_ct), NULL, false);
}

void copyTrackFXChain(COMMAND_T* _ct) {
	copyTrackFXChain(&g_fXChainClipboard, false);
}

void cutTrackFXChain(COMMAND_T* _ct) {
	copyTrackFXChain(&g_fXChainClipboard, false);
	setTrackFXChain(SNM_CMD_SHORTNAME(_ct), NULL, false);
}

void pasteTrackFXChain(COMMAND_T* _ct) {
	pasteTrackFXChain(SNM_CMD_SHORTNAME(_ct), &g_fXChainClipboard, false);
}

// i.e. paste (replace)
void setTrackFXChain(COMMAND_T* _ct) {
	setTrackFXChain(SNM_CMD_SHORTNAME(_ct), &g_fXChainClipboard, false);
}

// *** Input FX ***

void clearTrackInputFXChain(COMMAND_T* _ct) {
	if (g_bv4)
		setTrackFXChain(SNM_CMD_SHORTNAME(_ct), NULL, true);
}

void copyTrackInputFXChain(COMMAND_T* _ct) {
	if (g_bv4)
		copyTrackFXChain(&g_fXChainClipboard, true);
}

void cutTrackInputFXChain(COMMAND_T* _ct) {
	if (g_bv4) {
		copyTrackFXChain(&g_fXChainClipboard, true);
		setTrackFXChain(SNM_CMD_SHORTNAME(_ct), NULL, true);
	}
}

void pasteTrackInputFXChain(COMMAND_T* _ct) {
	if (g_bv4)
		pasteTrackFXChain(SNM_CMD_SHORTNAME(_ct), &g_fXChainClipboard, true);
}

// i.e. paste (replace)
void setTrackInputFXChain(COMMAND_T* _ct) {
	if (g_bv4)
		setTrackFXChain(SNM_CMD_SHORTNAME(_ct), &g_fXChainClipboard, true);
}


///////////////////////////////////////////////////////////////////////////////
// Common take & track FX chain code
///////////////////////////////////////////////////////////////////////////////

void copyFXChainSlotToClipBoard(int _slot)
{
	if (_slot >= 0 && _slot < g_fxChainFiles.GetSize()) 
	{
		char fullPath[BUFFER_SIZE] = "";
		if (g_fxChainFiles.GetFullPath(_slot, fullPath, BUFFER_SIZE))
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

void smartCopyFXChain(COMMAND_T* _ct) {
	if (GetCursorContext() == 1 && CountSelectedMediaItems(NULL)) copyTakeFXChain(&g_fXChainClipboard);
	else copyTrackFXChain(&g_fXChainClipboard, false);
}

void smartPasteFXChain(COMMAND_T* _ct) {
	if (GetCursorContext() == 1 && CountSelectedMediaItems(NULL)) pasteTakeFXChain(SNM_CMD_SHORTNAME(_ct), &g_fXChainClipboard, true);
	else pasteTrackFXChain(SNM_CMD_SHORTNAME(_ct), &g_fXChainClipboard, false);
}

void smartPasteReplaceFXChain(COMMAND_T* _ct) {
	if (GetCursorContext() == 1 && CountSelectedMediaItems(NULL)) setTakeFXChain(SNM_CMD_SHORTNAME(_ct), &g_fXChainClipboard, true);
	else setTrackFXChain(SNM_CMD_SHORTNAME(_ct), &g_fXChainClipboard, false);
}

void smartCutFXChain(COMMAND_T* _ct) {
	if (GetCursorContext() == 1 && CountSelectedMediaItems(NULL)) {
		copyTakeFXChain(_ct);
		setTakeFXChain(SNM_CMD_SHORTNAME(_ct), NULL, true);
	}
	else {
		copyTrackFXChain(&g_fXChainClipboard, false);
		setTrackFXChain(SNM_CMD_SHORTNAME(_ct), NULL, false);
	}
}


///////////////////////////////////////////////////////////////////////////////
// Other
///////////////////////////////////////////////////////////////////////////////

void reassignLearntMIDICh(COMMAND_T* _ct)
{
	bool updated = false;
	int ch = (int)_ct->user;
	int prm = ch; // -1: all fx, -2: sel fx, -3: all fx to input channel

	// Prompt for channel if needed
	switch(prm)
	{
		case -1:
		case -2:
			ch = PromptForMIDIChannel(SNM_CMD_SHORTNAME(_ct)); //loops on err
			if (ch == -1) return; // user has cancelled
			break;
	}

	for (int i = 0; i <= GetNumTracks(); i++) // include master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			SNM_LearnMIDIChPatcher p(tr);
			switch(prm)
			{
				case -2: {
					int fx = getSelectedTrackFX(tr);
					if (fx > 0)
						updated |= p.SetChannel(ch, fx);
					break;
				}
				case -3: {
					int in = *(int*)GetSetMediaTrackInfo(tr, "I_RECINPUT", NULL);
					if ((in & 0x1000) == 0x1000) {
						in &= 0x1F;
						if (in > 0)
							updated |= p.SetChannel(in-1, -1);
					}
					break;
				}
				default:
					updated |= p.SetChannel(ch, -1);
					break;
			}
		}
	}
	if (updated)
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}
