/******************************************************************************
/ SnM_Misc.cpp
/
/ Copyright (c) 2009-2013 Jeffos
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
// => funcs must be made dumb-proof!
///////////////////////////////////////////////////////////////////////////////

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
				// standard case: a source is defined
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
		char cmd[SNM_MAX_PATH]=""; 
		if (_snprintfStrict(cmd, sizeof(cmd), "%s\\reaper.exe", GetExePath()) > 0)
		{
			WDL_FastString prmStr;
			prmStr.SetFormatted(SNM_MAX_PATH, " \"%s\"", fnStr->Get());
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

int g_SNM_LastImgSlot = -1;

void ShowImageSlot(int _slotType, const char* _title, int _slot) {
	if (WDL_FastString* fnStr = g_slots.Get(_slotType)->GetOrPromptOrBrowseSlot(_title, &_slot)) {
		if (OpenImageWnd(fnStr->Get()))
			g_SNM_LastImgSlot = _slot;
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
		g_SNM_LastImgSlot += (int)_ct->user;
		if (g_SNM_LastImgSlot<0) g_SNM_LastImgSlot = sz-1;
		else if (g_SNM_LastImgSlot>=sz) g_SNM_LastImgSlot = 0;
	}
	ShowImageSlot(g_tiedSlotActions[SNM_SLOT_IMG], SWS_CMD_SHORTNAME(_ct), sz ? g_SNM_LastImgSlot : -1); // -1: err msg (empty list)
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
		Undo_OnStateChangeEx2(NULL, _title, UNDO_STATE_ALL, -1);
}

void SetSelTrackIconSlot(COMMAND_T* _ct) {
	SetSelTrackIconSlot(g_tiedSlotActions[SNM_SLOT_IMG], SWS_CMD_SHORTNAME(_ct), (int)_ct->user);
}


///////////////////////////////////////////////////////////////////////////////
// Misc actions / helpers
///////////////////////////////////////////////////////////////////////////////

//increase/decrease the metronome volume
void ChangeMetronomeVolume(COMMAND_T* _ct) {
	KBD_OnMainActionEx(999, 0x40+(int)_ct->user, -1, 2, GetMainHwnd(), NULL);
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

