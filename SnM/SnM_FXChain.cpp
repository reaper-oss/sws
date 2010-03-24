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

#define MAX_FXCHAIN_SLOTS 32
#define FXCHAIN_SLOT_PROMPT "Slot (1-32):"
#define FXCHAIN_SLOT_ERR "Invalid FX chain slot!\nPlease enter a value in [1; 32]."

WDL_String g_storedFXChain[MAX_FXCHAIN_SLOTS+1]; //+1 for clipboard


///////////////////////////////////////////////////////////////////////////////
// Take FX chain
///////////////////////////////////////////////////////////////////////////////

void loadPasteTakeFXChain(COMMAND_T* _ct){
	loadPasteTakeFXChain(SNM_CMD_SHORTNAME(_ct), (int)_ct->user, true);
}

void loadPasteAllTakesFXChain(COMMAND_T* _ct){
	loadPasteTakeFXChain(SNM_CMD_SHORTNAME(_ct), (int)_ct->user, false);
}

void loadPasteTakeFXChain(const char* _title, int _slot, bool _activeOnly)
{
	if (CountSelectedMediaItems(0))
	{
		// Prompt for slot if needed
		if (_slot == -1) _slot = promptForSlot(_title); //loops on err
		if (_slot == -1) return; // user has cancelled

		// main job
		loadOrBrowseFXChain(_slot, _title);
		if (g_storedFXChain[_slot].GetLength())
			setTakeFXChain(_title, _slot, _activeOnly, false);
	}
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
			if (item && *(bool*)GetSetMediaItemInfo(item,"B_UISEL",NULL))
			{
				SNM_FXChainTakePatcher p(item);
				fxChain = p.GetFXChain();
				if (fxChain)
				{
					g_storedFXChain[MAX_FXCHAIN_SLOTS].Set(fxChain->Get());
					break;
				}
			}
		}
	}
}

void cutTakeFXChain(COMMAND_T* _ct) {
	copyTakeFXChain(_ct);
	setTakeFXChain(SNM_CMD_SHORTNAME(_ct), MAX_FXCHAIN_SLOTS, true, true);
}

void pasteTakeFXChain(COMMAND_T* _ct) {
	setTakeFXChain(SNM_CMD_SHORTNAME(_ct), MAX_FXCHAIN_SLOTS, true, false);
}

void pasteAllTakesFXChain(COMMAND_T* _ct) {
	setTakeFXChain(SNM_CMD_SHORTNAME(_ct), MAX_FXCHAIN_SLOTS, false, false);
}

void clearActiveTakeFXChain(COMMAND_T* _ct) {
	setTakeFXChain(SNM_CMD_SHORTNAME(_ct), (int)_ct->user, true, true);
}

void clearAllTakesFXChain(COMMAND_T* _ct) {
	setTakeFXChain(SNM_CMD_SHORTNAME(_ct), (int)_ct->user, false, true);
}

// could be <0 (e.g. clear, ..)
void setTakeFXChain(const char* _title, int _slot, bool _activeOnly, bool _clear)
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
				updated |= (p.SetFXChain(_clear ? NULL : &g_storedFXChain[_slot], _activeOnly) > 0);
			}
		}
	}
	// Undo point
	if (updated)
		Undo_OnStateChangeEx(_title, UNDO_STATE_ALL, -1);
}

/*JFB
void setActiveTakeFXChain(COMMAND_T* _ct) {
	setTakeFXChain((int)_ct->user, true, false);
}

void setAllTakesFXChain(COMMAND_T* _ct) {
	setTakeFXChain((int)_ct->user, false, false);
}
*/


///////////////////////////////////////////////////////////////////////////////
// Track FX chain
///////////////////////////////////////////////////////////////////////////////

void loadPasteTrackFXChain(COMMAND_T* _ct)
{
	if (CountSelectedTracks(0))
	{
		int slot = (int)_ct->user;

		// Prompt for slot if needed
		if (slot == -1) slot = promptForSlot(SNM_CMD_SHORTNAME(_ct)); //loops on err
		if (slot == -1) return; // user has cancelled

		// main job
		loadOrBrowseFXChain(slot, SNM_CMD_SHORTNAME(_ct));
		if (g_storedFXChain[slot].GetLength())
			setTrackFXChain(SNM_CMD_SHORTNAME(_ct), slot, false);
	}
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
			if (p.GetSubChunk("FXCHAIN", 2, 0, &g_storedFXChain[MAX_FXCHAIN_SLOTS]))
			{
				// remove last ">\n" (ok 'cause built by GetSubChunk())
				g_storedFXChain[MAX_FXCHAIN_SLOTS].DeleteSub(g_storedFXChain[MAX_FXCHAIN_SLOTS].GetLength()-3,2);
				WDL_PtrList<char> removedKeywords;
				removedKeywords.Add("<FXCHAIN");
				removedKeywords.Add("WNDRECT");
				removedKeywords.Add("SHOW");
				removedKeywords.Add("FLOATPOS");
				AddChunkIds(&removedKeywords);
				RemoveChunkLines(&g_storedFXChain[MAX_FXCHAIN_SLOTS], &removedKeywords); // un-float copied fx chain windows
//dbg				ShowConsoleMsg(g_storedFXChain[MAX_FXCHAIN_SLOTS].Get());
			}
		}
	}
}

void cutTrackFXChain(COMMAND_T* _ct) {
	copyTrackFXChain(_ct);
	setTrackFXChain(SNM_CMD_SHORTNAME(_ct), MAX_FXCHAIN_SLOTS, true);
}

void pasteTrackFXChain(COMMAND_T* _ct) {
	setTrackFXChain(SNM_CMD_SHORTNAME(_ct), MAX_FXCHAIN_SLOTS, false);
}

void setTrackFXChain(const char* _title, int _slot, bool _clear)
{
	bool updated = false;
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i,false); // include master
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			SNM_FXChainTrackPatcher p(tr);
			updated |= (p.SetFXChain(_clear ? NULL : &g_storedFXChain[_slot]) > 0);
		}
	}
	// Undo point
	if (updated)
		Undo_OnStateChangeEx(_title, UNDO_STATE_ALL, -1);
}


///////////////////////////////////////////////////////////////////////////////
// Common
///////////////////////////////////////////////////////////////////////////////

//returns -1 on cancel
int promptForSlot(const char* _title)
{
	int slot = -1;
	while (slot == -1)
	{
		char reply[8]= ""; // empty default slot
		if (GetUserInputs(_title, 1, FXCHAIN_SLOT_PROMPT, reply, 8))
		{
			slot = atoi(reply); //0 on error
			if (slot > 0 && slot <= MAX_FXCHAIN_SLOTS) {
				return (slot-1);
			}
			else {
				slot=-1;
				MessageBox(GetMainHwnd(), FXCHAIN_SLOT_ERR, _title, /*MB_ICONERROR | */MB_OK);
			}
		}
		else return -1; // user has cancelled
	}
	return -1; //in case the slot comes from mars
}

void clearFXChainSlot(COMMAND_T* _ct)
{
	int slot = promptForSlot(SNM_CMD_SHORTNAME(_ct)); //loops on err
	if (slot == -1) return; // user has cancelled

	char cPath[256];
	readIniFile(slot, cPath, 256);
	if (strlen(cPath))
	{
		char toBeCleared[256] = "";
		sprintf(toBeCleared, "Are you sure you want to clear the FX chain slot %d?\n(%s)", slot+1, cPath); 
		if (MessageBox(GetMainHwnd(), toBeCleared, SNM_CMD_SHORTNAME(_ct), /*MB_ICONQUESTION | */MB_OKCANCEL) == 1)
			saveIniFile(slot, "");
	}
}

void showFXChainSlots(COMMAND_T* _ct)
{
	WDL_String slots;
	for (int i=0; i < MAX_FXCHAIN_SLOTS; i++)
	{
		char cPath[256];
		readIniFile(i, cPath, 256);
		slots.AppendFormatted((int)strlen(cPath)+12, "Slot %d:\t%s\n", i+1, cPath);
	}

	if (slots.GetLength())
		SNM_ShowConsoleMsg("S&M - FX chain slots", slots.Get());
}

void loadStoreFXChain(int _slot, const char* _filename)
{
	if (_filename && *_filename)
	{
		FILE* f = fopen(_filename, "r");
		if (f)
		{
			g_storedFXChain[_slot].Set("");

			char str[4096];
			while(fgets(str, 4096, f))
				g_storedFXChain[_slot].Append(str);
			fclose(f);

			// persist slot's path
			saveIniFile(_slot, _filename);
		}
	}
}

void browseStoreFXChain(int _slot, const char* _title)
{
	char cPath[256];
	GetProjectPath(cPath, 256);
	char* filename = 
		BrowseForFiles(_title, cPath, NULL, false, "REAPER FX Chain (*.RfxChain)\0*.RfxChain\0");
	if (filename)
	{
		loadStoreFXChain(_slot, filename);		
		free(filename);
	}
}

void loadOrBrowseFXChain(int _slot, const char* _title)
{
	if (!g_storedFXChain[_slot].GetLength())
	{
		// try to read the path from the ini file, or browse if path not found
		char cPath[256];
		readIniFile(_slot, cPath, 256);
		if (*cPath)
			loadStoreFXChain(_slot, cPath);
		else
			browseStoreFXChain(_slot, _title);
	}
}

void readIniFile(int _slot, char* _buf, int _bufSize)
{
	if (_slot >= 0 && _slot < MAX_FXCHAIN_SLOTS)
	{
		char iniFilePath[256];
		sprintf(iniFilePath,SNM_FORMATED_INI_FILE,GetExePath());
		char slot[16] = "";
		sprintf(slot,"SLOT%d",_slot+1);
		GetPrivateProfileString("FXCHAIN",slot,"",_buf,_bufSize,iniFilePath);
	}
}

void saveIniFile(int _slot, const char* _path)
{
	if (_path && _slot >= 0 && _slot < MAX_FXCHAIN_SLOTS)
	{
		char iniFilePath[256];
		sprintf(iniFilePath,SNM_FORMATED_INI_FILE,GetExePath());
		char cSlot[16] = "";
		sprintf(cSlot,"SLOT%d",_slot+1);
		WritePrivateProfileString("FXCHAIN",cSlot,_path,iniFilePath);
	}
}
