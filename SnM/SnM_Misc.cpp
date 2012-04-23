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


///////////////////////////////////////////////////////////////////////////////
// theme slots (Resources view)
///////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
void LoadThemeSlot(int _slotType, const char* _title, int _slot)
{
	if (WDL_FastString* fnStr = g_slots.Get(_slotType)->GetOrPromptOrBrowseSlot(_title, _slot))
	{
		char cmd[BUFFER_SIZE]=""; _snprintf(cmd, BUFFER_SIZE, "%s\\reaper.exe", GetExePath());
		WDL_FastString fnStr2; fnStr2.SetFormatted(BUFFER_SIZE, " \"%s\"", fnStr->Get());
		_spawnl(_P_NOWAIT, cmd, fnStr2.Get(), NULL);
		delete fnStr;
	}
}

void LoadThemeSlot(COMMAND_T* _ct) {
	LoadThemeSlot(g_tiedSlotActions[SNM_SLOT_THM], SWS_CMD_SHORTNAME(_ct), (int)_ct->user);
}
#endif


///////////////////////////////////////////////////////////////////////////////
// image slots (Resources view)
///////////////////////////////////////////////////////////////////////////////

void ShowImageSlot(int _slotType, const char* _title, int _slot) {
	if (WDL_FastString* fnStr = g_slots.Get(_slotType)->GetOrPromptOrBrowseSlot(_title, _slot)) {
		OpenImageView(fnStr->Get());
		delete fnStr;
	}
}

void ShowImageSlot(COMMAND_T* _ct) {
	ShowImageSlot(g_tiedSlotActions[SNM_SLOT_IMG], SWS_CMD_SHORTNAME(_ct), (int)_ct->user);
}

void SetSelTrackIconSlot(int _slotType, const char* _title, int _slot) {
	if (WDL_FastString* fnStr = g_slots.Get(_slotType)->GetOrPromptOrBrowseSlot(_title, _slot)) {
		SetSelTrackIcon(fnStr->Get());
		delete fnStr;
	}
}

void SetSelTrackIconSlot(COMMAND_T* _ct) {
	SetSelTrackIconSlot(g_tiedSlotActions[SNM_SLOT_IMG], SWS_CMD_SHORTNAME(_ct), (int)_ct->user);
}


///////////////////////////////////////////////////////////////////////////////
// misc	actions / helpers
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
// _type: 3 & 4 for basic dump (3=native actions, 4=SWS)
bool DumpActionList(int _type, const char* _title, const char* _lineFormat, const char* _heading, const char* _ending)
{
	char currentSection[SNM_MAX_SECTION_NAME_LEN] = "";
	HWND hList = GetActionListBox(currentSection, SNM_MAX_SECTION_NAME_LEN);
	if (hList && currentSection)
	{
		char sectionURL[SNM_MAX_SECTION_NAME_LEN] = ""; 
		if (!GetSectionNameAsURL(_type == 1 || _type == 2, currentSection, sectionURL, SNM_MAX_SECTION_NAME_LEN))
		{
			MessageBox(GetMainHwnd(), __LOCALIZE("Dump failed: unknown section!","sws_mbox"), _title, MB_OK);
			return false;
		}

		char name[SNM_MAX_SECTION_NAME_LEN*2] = "", fn[BUFFER_SIZE] = "";
		_snprintf(name, SNM_MAX_SECTION_NAME_LEN*2, "%s%s.txt", sectionURL, !(_type % 2) ? "_SWS" : "");
		if (!BrowseForSaveFile(_title, GetResourcePath(), name, SNM_TXT_EXT_LIST, fn, BUFFER_SIZE))
			return false;

		//flush
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
			for (int i = 0; i < ListView_GetItemCount(hList); i++)
			{
				li.iItem = i;
				ListView_GetItem(hList, &li);
				int cmdId = (int)li.lParam;

				char customId[SNM_MAX_ACTION_CUSTID_LEN] = "";
				char cmdName[SNM_MAX_ACTION_NAME_LEN] = "";
				ListView_GetItemText(hList, i, 1, cmdName, SNM_MAX_ACTION_NAME_LEN);
				ListView_GetItemText(hList, i, 4, customId, SNM_MAX_ACTION_CUSTID_LEN);

				int isSws = IsSwsAction(cmdName);
				if (!IsMacro(cmdName) &&
					((_type%2 && !isSws) || (!(_type%2) && isSws && IsLocalizableAction(customId))))
				{
					if (!*customId) // for native actions..
						_snprintf(customId, SNM_MAX_ACTION_CUSTID_LEN, "%d", cmdId);
					fprintf(f, _lineFormat, sectionURL, customId, cmdName, customId);
				}
			}
			if (_ending)
				fprintf(f, "%s", _ending); 

			fclose(f);

			char msg[BUFFER_SIZE] = "";
			if (_snprintf(msg, BUFFER_SIZE, __LOCALIZE_VERFMT("Wrote %s","sws_mbox"), fn) > 0)
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
