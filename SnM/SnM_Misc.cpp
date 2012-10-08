/******************************************************************************
/ SnM_Misc.cpp
/
/ Copyright (c) 2009-2012 Jeffos
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
#include "SnM.h"
#include "../reaper/localize.h"
#include "../Prompt.h"
#include "../../WDL/projectcontext.h"


///////////////////////////////////////////////////////////////////////////////
// Reascript export
///////////////////////////////////////////////////////////////////////////////

// write C++ API functions header 
// (same as the native action but w/o export of sws funcs..)
void GenAPI(COMMAND_T*)
{
	UnregisterExportedAPI(g_rec);
	Main_OnCommand(41064, 0);
	RegisterExportedAPI(g_rec);
}

WDL_FastString* SNM_CreateFastString(const char* _str) {
	return new WDL_FastString(_str);
}

void SNM_DeleteFastString(WDL_FastString* _str) { 
	DELETE_NULL(_str);
}

const char* SNM_GetFastString(WDL_FastString* _str) {
	return _str?_str->Get():"";
}

int SNM_GetFastStringLength(WDL_FastString* _str) {
	return _str?_str->GetLength():0;
}

WDL_FastString* SNM_SetFastString(WDL_FastString* _str, const char* _newStr) {
	if (_str) _str->Set(_newStr?_newStr:"");
	return _str;
}

MediaItem_Take* SNM_GetMediaItemTakeByGUID(ReaProject* _project, const char* _guid)
{
	if (_guid && *_guid)
	{
		GUID g;
		stringToGuid(_guid, &g);
		return GetMediaItemTakeByGUID(_project, &g);
	}
	return NULL;
}

bool SNM_GetSourceType(MediaItem_Take* _tk, WDL_FastString* _type)
{
	if (_tk && _type)
		if (PCM_source* source = GetMediaItemTake_Source(_tk)) {
			_type->Set(source->GetType());
			return true;
		}
	return false;
}

// note: PCM_source.Load/SaveState() won't always work
//       e.g. getting an empty take src, turning wav src into midi, etc..
bool SNM_GetSetSourceState(MediaItem* _item, int _takeIdx, WDL_FastString* _state, bool _setnewvalue)
{
	bool ok = false;
	if (_item && _state)
	{
		if (_takeIdx<0)
			_takeIdx = *(int*)GetSetMediaItemInfo(_item, "I_CURTAKE", NULL);

		int tkPos, tklen;
		WDL_FastString takeChunk;
		SNM_TakeParserPatcher p(_item, CountTakes(_item));
		if (p.GetTakeChunk(_takeIdx, &takeChunk, &tkPos, &tklen))
		{
			SNM_ChunkParserPatcher ptk(&takeChunk, false);

			// set
			if (_setnewvalue)
			{
				// standard case: a source is there
				if (ptk.ReplaceSubChunk("SOURCE", 1, 0, _state->Get())) // no break keyword here: we're already at the end of the item..
					ok = p.ReplaceTake(tkPos, tklen, ptk.GetChunk());
				// replacing an empty take
				else
				{
					WDL_FastString newTkChunk("TAKE\n");
					newTkChunk.Append(_state);
					ok = p.ReplaceTake(tkPos, tklen, &newTkChunk);
				}
			}
			// get
			else
			{
				if (ptk.GetSubChunk("SOURCE", 1, 0, _state)<0)
					_state->Set(""); // empty take
				ok = true;
			}
		}
	}
	return ok;
}

bool SNM_GetSetSourceState2(MediaItem_Take* _tk, WDL_FastString* _state, bool _setnewvalue)
{
	if (_tk)
		if (MediaItem* item = GetMediaItemTake_Item(_tk)) {
			int tkIdx = GetTakeIndex(item, _tk);
			if (tkIdx >= 0)
				return SNM_GetSetSourceState(item, tkIdx, _state, _setnewvalue);
		}
	return false;
}

bool SNM_GetSetObjectState(void* _obj, WDL_FastString* _state, bool _setnewvalue, bool _minstate)
{
	bool ok = false;
	if (_obj && _state)
	{
		int fxstate = SNM_PreObjectState(_setnewvalue ? _state : NULL, _minstate);
		char* p = GetSetObjectState(_obj, _setnewvalue ? _state->Get() : NULL);
		if (_setnewvalue)
		{
			ok = (p==NULL);
		}
		else if (p)
		{
			_state->Set(p);
			FreeHeapPtr((void*)p);
			ok = true;
		}
		SNM_PostObjectState(fxstate);
	}
	return ok;
}

bool SNM_AddReceive(MediaTrack* _srcTr, MediaTrack* _destTr, int _type)
{
	if (_srcTr && _destTr && _srcTr!=_destTr && _type>=0 && _type<=3)
	{
		SNM_SendPatcher p = SNM_SendPatcher(_destTr);
		char vol[32] = "1.00000000000000";
		char pan[32] = "0.00000000000000";
		_snprintfSafe(vol, sizeof(vol), "%.14f", *(double*)GetConfigVar("defsendvol"));
		return (p.AddReceive(_srcTr, _type, vol, pan) > 0);
	}
	return false;
}

bool SNM_RemoveReceive(MediaTrack* _tr, int _rcvIdx)
{
	if (_tr && _rcvIdx>=0) 	{
		SNM_ChunkParserPatcher p(_tr);
		return p.RemoveLine("TRACK", "AUXRECV", 1, _rcvIdx, "MIDIOUT");
	}
	return false;
}

int SNM_GetIntConfigVar(const char* _varName, int _errVal) {
	if (int* pVar = (int*)(GetConfigVar(_varName)))
		return *pVar;
	return _errVal;
}

bool SNM_SetIntConfigVar(const char* _varName, int _newVal) {
	if (int* pVar = (int*)(GetConfigVar(_varName))) {
		*pVar = _newVal;
		return true;
	}
	return false;
}

double SNM_GetDoubleConfigVar(const char* _varName, double _errVal) {
	if (double* pVar = (double*)(GetConfigVar(_varName)))
		return *pVar;
	return _errVal;
}

bool SNM_SetDoubleConfigVar(const char* _varName, double _newVal) {
	if (double* pVar = (double*)(GetConfigVar(_varName))) {
		*pVar = _newVal;
		return true;
	}
	return false;
}


///////////////////////////////////////////////////////////////////////////////
// Theme slots (Resources view)
///////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
void LoadThemeSlot(int _slotType, const char* _title, int _slot)
{
	if (WDL_FastString* fnStr = g_slots.Get(_slotType)->GetOrPromptOrBrowseSlot(_title, &_slot))
	{
		char cmd[BUFFER_SIZE]=""; 
		if (_snprintfStrict(cmd, sizeof(cmd), "%s\\reaper.exe", GetExePath()) > 0)
		{
			WDL_FastString prmStr;
			prmStr.SetFormatted(BUFFER_SIZE, " \"%s\"", fnStr->Get());
			_spawnl(_P_NOWAIT, cmd, prmStr.Get(), NULL);
			delete fnStr;
		}
	}
}

void LoadThemeSlot(COMMAND_T* _ct) {
	LoadThemeSlot(g_tiedSlotActions[SNM_SLOT_THM], SWS_CMD_SHORTNAME(_ct), (int)_ct->user);
}
#endif


///////////////////////////////////////////////////////////////////////////////
// Image slots (Resources view)
///////////////////////////////////////////////////////////////////////////////

int g_lastShowImgSlot = -1;

void ShowImageSlot(int _slotType, const char* _title, int _slot) {
	if (WDL_FastString* fnStr = g_slots.Get(_slotType)->GetOrPromptOrBrowseSlot(_title, &_slot)) {
		if (OpenImageView(fnStr->Get()))
			g_lastShowImgSlot = _slot;
		delete fnStr;
	}
}

void ShowImageSlot(COMMAND_T* _ct) {
	ShowImageSlot(g_tiedSlotActions[SNM_SLOT_IMG], SWS_CMD_SHORTNAME(_ct), (int)_ct->user);
}

void ShowNextPreviousImageSlot(COMMAND_T* _ct)
{
	int sz = g_slots.Get(g_tiedSlotActions[SNM_SLOT_IMG])->GetSize();
	if (sz) {
		g_lastShowImgSlot += (int)_ct->user;
		if (g_lastShowImgSlot<0) g_lastShowImgSlot = sz-1;
		else if (g_lastShowImgSlot>=sz) g_lastShowImgSlot = 0;
	}
	ShowImageSlot(g_tiedSlotActions[SNM_SLOT_IMG], SWS_CMD_SHORTNAME(_ct), sz ? g_lastShowImgSlot : -1); // -1: err msg (empty list)
}

void SetSelTrackIconSlot(int _slotType, const char* _title, int _slot)
{
	bool updated = false;
	if (WDL_FastString* fnStr = g_slots.Get(_slotType)->GetOrPromptOrBrowseSlot(_title, &_slot))
	{
		for (int j=0; j <= GetNumTracks(); j++)
			if (MediaTrack* tr = CSurf_TrackFromID(j, false))
				if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
					updated |= SetTrackIcon(tr, fnStr->Get());
		delete fnStr;
	}
	if (updated && _title)
		Undo_OnStateChangeEx(_title, UNDO_STATE_ALL, -1);
}

void SetSelTrackIconSlot(COMMAND_T* _ct) {
	SetSelTrackIconSlot(g_tiedSlotActions[SNM_SLOT_IMG], SWS_CMD_SHORTNAME(_ct), (int)_ct->user);
}


///////////////////////////////////////////////////////////////////////////////
// Misc actions / helpers
///////////////////////////////////////////////////////////////////////////////

bool WaitForTrackMute(DWORD* _muteTime)
{
	if (_muteTime && *_muteTime)
	{
		unsigned int fade = int(*(int*)GetConfigVar("mutefadems10") / 10 + 0.5);
		while ((GetTickCount() - *_muteTime) < fade)
		{
#ifdef _WIN32
			// keep the UI updating
			MSG msg;
			while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
				DispatchMessage(&msg);
#endif
			Sleep(1);
		}
		*_muteTime = 0;
		return true;
	}
	return false;
}

void WinWaitForEvent(DWORD _event, DWORD _timeOut=500, DWORD _minReTrigger=500)
{
#ifdef _WIN32
	static DWORD waitTime = 0;
//	if ((GetTickCount() - waitTime) > _minReTrigger)
	{
		waitTime = GetTickCount();
		while((GetTickCount() - waitTime) < _timeOut) // for safety
		{
			MSG msg;
			if(PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
			{
				// new message to be processed
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				if(msg.message == _event)
					break;
			}
		}
	}
#endif
}

// http://forum.cockos.com/showthread.php?p=612065
void SimulateMouseClick(COMMAND_T* _ct)
{
	POINT p;
	GetCursorPos(&p);
	mouse_event(MOUSEEVENTF_LEFTDOWN, p.x, p.y, 0, 0);
	mouse_event(MOUSEEVENTF_LEFTUP, p.x, p.y, 0, 0);
	WinWaitForEvent(WM_LBUTTONUP);
}

// dump actions or the wiki ALR summary for the current section *as displayed* in the action dlg 
// API LIMITATION: the action dlg is hacked because only the main section could be dumped othewise..
// See http://forum.cockos.com/showthread.php?t=61929 and http://wiki.cockos.com/wiki/index.php/Action_List_Reference
// _type: 1 & 2 for ALR wiki (1=native actions, 2=SWS)
// _type: 3, 4 & 5 for basic dump (3=native actions, 4=SWS, 5=user macros)
bool DumpActionList(int _type, const char* _title, const char* _lineFormat, const char* _heading, const char* _ending)
{
	char currentSection[SNM_MAX_SECTION_NAME_LEN] = "";
	HWND hList = GetActionListBox(currentSection, SNM_MAX_SECTION_NAME_LEN);
	if (hList && currentSection)
	{
		char sectionURL[SNM_MAX_SECTION_NAME_LEN] = ""; 
		if (!GetSectionNameAsURL(_type==1||_type==2, currentSection, sectionURL, SNM_MAX_SECTION_NAME_LEN))
		{
			MessageBox(GetMainHwnd(), __LOCALIZE("Dump failed: unknown section!","sws_mbox"), _title, MB_OK);
			return false;
		}

		char name[SNM_MAX_SECTION_NAME_LEN*2] = "", fn[BUFFER_SIZE] = "";
		if (_snprintfStrict(name, sizeof(name), "%s_Section%s.txt", sectionURL, (_type==2||_type==4) ? "_SWS" : _type==5 ? "_Macros" : "") <= 0)
			*name = '\0';
		if (!BrowseForSaveFile(_title, GetResourcePath(), name, SNM_TXT_EXT_LIST, fn, BUFFER_SIZE))
			return false;

		// flush
		FILE* f = fopenUTF8(fn, "w"); 
		if (f)
		{
			fputs("\n", f);
			fclose(f);

			f = fopenUTF8(fn, "a"); 
			if (!f) return false; //just in case..

			if (_heading)
				fprintf(f, "%s", _heading); 

			LVITEM li;
			li.mask = LVIF_STATE | LVIF_PARAM;
			li.iSubItem = 0;
			for (int i=0; i < ListView_GetItemCount(hList); i++)
			{
				li.iItem = i;
				ListView_GetItem(hList, &li);
				int cmdId = (int)li.lParam;

				char custId[SNM_MAX_ACTION_CUSTID_LEN] = "";
				char cmdName[SNM_MAX_ACTION_NAME_LEN] = "";
				ListView_GetItemText(hList, i, 1, cmdName, SNM_MAX_ACTION_NAME_LEN);
				ListView_GetItemText(hList, i, 4, custId, SNM_MAX_ACTION_CUSTID_LEN);

				bool isMacro = IsMacro(cmdName);
				int isSws = IsSwsAction(cmdName);
				bool isSwsMacro = !IsLocalizableAction(custId);
				if (((_type==1||_type==3) && !isSws && !isMacro && !isSwsMacro) || 
					((_type==2||_type==4) && isSws  && !isMacro && !isSwsMacro) ||
					(_type==5 && (isMacro||isSwsMacro)))
				{
					if (!*custId) // for native actions
						if (_snprintfStrict(custId, sizeof(custId), "%d", cmdId) <= 0)
							*custId = '\0';
					if (*custId)
						fprintf(f, _lineFormat, sectionURL, custId, cmdName, custId);
				}
			}
			if (_ending)
				fprintf(f, "%s", _ending); 

			fclose(f);

			char msg[BUFFER_SIZE] = "";
			_snprintfSafe(msg, sizeof(msg), __LOCALIZE_VERFMT("Wrote %s","sws_mbox"), fn);
			MessageBox(GetMainHwnd(), msg, _title, MB_OK);
			return true;
		}
		else
			MessageBox(GetMainHwnd(), __LOCALIZE("Dump failed: unable to write to file!","sws_mbox"), _title, MB_OK);
	}
	else
		MessageBox(GetMainHwnd(), __LOCALIZE("Dump failed: action window not opened!","sws_mbox"), _title, MB_OK);
	return false;
}

void DumpWikiActionList(COMMAND_T* _ct)
{
	DumpActionList(
		(int)_ct->user, 
		__LOCALIZE("S&M - Save ALR Wiki summary","sws_mbox"),
		"|-\n| [[%s_%s|%s]] || %s\n",
		"{| class=\"wikitable\"\n|-\n! Action name !! Cmd ID\n",
		"|}\n");
}

void DumpActionList(COMMAND_T* _ct) {
	DumpActionList((int)_ct->user, __LOCALIZE("S&M - Dump action list","sws_mbox"), "%s\t%s\t%s\n", "Section\tId\tAction\n", NULL);
}

