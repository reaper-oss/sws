/******************************************************************************
/ SnM_Windows.cpp
/
/ Copyright (c) 2009-2013 Jeffos
/
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
#include "SnM_Dlg.h"
#include "SnM_FX.h"
#include "SnM_Track.h"
#include "SnM_Util.h"
#include "SnM_Window.h"
#include "../reaper/localize.h"


///////////////////////////////////////////////////////////////////////////////
// Misc window actions/helpers
///////////////////////////////////////////////////////////////////////////////

bool SNM_IsActiveWindow(HWND _h)
{
	if (!_h || !SWS_IsWindow(_h))
		return false;
	return (GetFocus() == _h || IsChild(_h, GetFocus()));
}

bool IsChildOf(HWND _hChild, const char* _title)
{
	HWND hCurrent = _hChild;
	char buf[256] = "";
	while (hCurrent) 
	{
		hCurrent = GetParent(hCurrent);
		if (hCurrent)
		{
			GetWindowText(hCurrent, buf, sizeof(buf));
			if (!strncmp(buf, _title, strlen(_title)))
				return true;
		}
	}
	return false;
}

#ifdef _WIN32

BOOL CALLBACK EnumReaWindows(HWND _hwnd, LPARAM _lParam)
{
	HWND hCurrent, hNew;
	hCurrent = _hwnd;
	hNew = hCurrent;
	while (hNew != NULL)
	{
		hNew = GetParent(hCurrent);
		if (hNew != NULL)
			hCurrent = hNew;
	}
	if (hCurrent == GetMainHwnd())
	{
		char buf[256];
		GetClassName(_hwnd, buf, sizeof(buf));
		if (!strcmp(buf, "#32770") ||
		!strcmp(buf, "REAPERMediaExplorerMainwnd") ||
		!strcmp(buf, "REAPERmidieditorwnd"))
		{
			WDL_PtrList<HWND__>* hwnds = (WDL_PtrList<HWND__>*)_lParam;
			if (hwnds && hwnds->Find(_hwnd) == -1)
				hwnds->Add(_hwnd);
		}
	}
	return TRUE;
}

static BOOL CALLBACK EnumReaChildWindows(HWND _hwnd, LPARAM _lParam)
{
	char buf[256];
	GetClassName(_hwnd, buf, sizeof(buf));
	if (!strcmp(buf, "#32770") ||
		!strcmp(buf, "REAPERMediaExplorerMainwnd") ||
		!strcmp(buf, "REAPERmidieditorwnd"))
	{
		WDL_PtrList<HWND__>* hwnds = (WDL_PtrList<HWND__>*)_lParam;
		if (hwnds && hwnds->Find(_hwnd) == -1)
			hwnds->Add(_hwnd);
		EnumChildWindows(_hwnd, EnumReaChildWindows, _lParam);
	}
	return TRUE;
}

HWND GetReaChildWindowByTitle(HWND _parent, const char* _title)
{
	char buf[256] = "";
	WDL_PtrList<HWND__> hwnds;
	EnumChildWindows(_parent, EnumReaChildWindows, (LPARAM)&hwnds);
	for (int i=0; i<hwnds.GetSize(); i++)
	{
		if (HWND w = hwnds.Get(i))
		{
			GetWindowText(w, buf, sizeof(buf));
			if (!strcmp(buf, _title))
				return w;
		}
	}
	return NULL;
}

// note: _title must be localized
HWND GetReaHwndByTitle(const char* _title)
{
	// docked in main window?
	HWND w = GetReaChildWindowByTitle(GetMainHwnd(), _title);
	if (w) return w;

	char buf[2048] = "";
	WDL_PtrList<HWND__> hwnds;
	EnumWindows(EnumReaWindows, (LPARAM)&hwnds);
	for (int i=0; i<hwnds.GetSize(); i++)
	{
		if (w = hwnds.Get(i))
		{
			GetWindowText(w, buf, sizeof(buf));

			// in floating window?
			if (!strcmp(_title, buf))
				return w;
			// in a floating docker (w/ other hwnds)?
			else if (!strcmp(__localizeFunc("Docker", "docker", 0), buf)) {
				if (w = GetReaChildWindowByTitle(w, _title))
					return w;
			}
			// in a floating docker (w/o other hwnds)?
			else {
				char dockerName[256]="";
				if (_snprintfStrict(dockerName, sizeof(dockerName), "%s%s", _title, __localizeFunc(" (docked)", "docker", 0)) > 0 && !strcmp(dockerName, buf))
					if (w = GetReaChildWindowByTitle(w, _title))
						return w;
			}
		}
	}
	return NULL;
}

#else

// note: _title and _dockerName must be localized
HWND GetReaHwndByTitleInFloatingDocker(const char* _title, const char* _dockerName)
{
	HWND hdock = FindWindowEx(NULL, NULL, NULL, _dockerName);
	while(hdock)
	{
		HWND hdock2 = FindWindowEx(hdock, NULL, NULL, "REAPER_dock");
		while(hdock2) {
			if (HWND w = FindWindowEx(hdock2, NULL, NULL, _title)) return w;
			hdock2 = FindWindowEx(hdock, hdock2, NULL, "REAPER_dock");
		}
		hdock = FindWindowEx(NULL, hdock, NULL, _dockerName);
	}
	return NULL;
}

// note: _title must be localized
// note: almost works on win + osx (since wdl 7cf02d) but utf8 issue on win
HWND GetReaHwndByTitle(const char* _title)
{
	// in a floating window?
	HWND w = FindWindowEx(NULL, NULL, NULL, _title);
	if (w) return w;
    
	// docked in main window?
	HWND hdock = FindWindowEx(GetMainHwnd(), NULL, NULL, "REAPER_dock");
	while(hdock) {
		if ((w = FindWindowEx(hdock, NULL, NULL, _title))) return w;
		hdock = FindWindowEx(GetMainHwnd(), hdock, NULL, "REAPER_dock");
	}
	// in a floating docker (w/ other hwnds)?
	if ((w = GetReaHwndByTitleInFloatingDocker(_title, __localizeFunc("Docker", "docker", 0))))
		return w;
	// in a floating docker (w/o other hwnds)?
	{
		char dockerName[256]="";
		if (_snprintfStrict(dockerName, sizeof(dockerName), "%s%s", _title, __localizeFunc(" (docked)", "docker", 0)) > 0)
			if ((w = GetReaHwndByTitleInFloatingDocker(_title, dockerName)))
				return w;
	}
	return NULL;
}

#endif

static BOOL CALLBACK EnumXCPWindows(HWND _hwnd, LPARAM _lParam)
{
	MediaTrack* tr = (MediaTrack*)GetWindowLongPtr(_hwnd, GWLP_USERDATA);
	if (tr && CSurf_TrackToID(tr, false)>=0)
	{
		WDL_PtrList<HWND__>* hwnds = (WDL_PtrList<HWND__>*)_lParam;
		if (hwnds && hwnds->Find(_hwnd) == -1)
			hwnds->Add(_hwnd);
	}
	else
		EnumChildWindows(_hwnd, EnumXCPWindows, _lParam);
	return TRUE;
}

void GetVisibleTCPTracks(WDL_PtrList<void>* _trList)
{
	WDL_PtrList<HWND__> hwnds;
	EnumChildWindows(GetMainHwnd(), EnumXCPWindows, (LPARAM)&hwnds);
	for (int i=0; i < hwnds.GetSize(); i++)
		if (HWND hwnd = hwnds.Get(i))
			if (IsWindowVisible(hwnd) && !IsChildOf(hwnd, __localizeFunc("Mixer", "DLG_151", 0)))
				_trList->Add((void*)GetWindowLongPtr(hwnd, GWLP_USERDATA));
}


///////////////////////////////////////////////////////////////////////////////
// Global focus actions (mostly Win-only)
///////////////////////////////////////////////////////////////////////////////

void FocusMainWindow(COMMAND_T* _ct) {
	SetForegroundWindow(GetMainHwnd());
}

#ifdef _WIN32

void CycleFocusWnd(COMMAND_T * _ct)
{
	if (GetMainHwnd())
	{
		HWND focusedWnd = GetForegroundWindow();
		HWND w = GetWindow(GetMainHwnd(), GW_HWNDLAST);
		while (w)
		{
			if (IsWindowVisible(w) && GetWindow(w, GW_OWNER) == GetMainHwnd() && focusedWnd != w) {
				SetForegroundWindow(w);
				return;
			}
			w = GetWindow(w, GW_HWNDPREV);
		}
	}
}

int g_lastFocusHideOthers = 0;

void CycleFocusHideOthersWnd(COMMAND_T * _ct)
{
	WDL_PtrList<HWND__> reahwnds;
	EnumWindows(EnumReaWindows, (LPARAM)&reahwnds);
	if (reahwnds.Find(GetMainHwnd()) == -1)
		reahwnds.Add(GetMainHwnd());

	// sort & filter windows
	char buf[256] = "";
	WDL_PtrList<HWND__> hwnds;
	for (int i =0; i<reahwnds.GetSize(); i++)
	{
		if (HWND w = reahwnds.Get(i))
		{
			GetWindowText(w, buf, sizeof(buf));
			if (strcmp(__localizeFunc("Item/track info","sws_DLG_131",0), buf) && // xen stuff to remove
				strcmp(__localizeFunc("Docker", "docker", 0), buf)) // "closed" floating dockers are in fact hidden (and not redrawn => issue)
			{
				int j=0;
				char buf2[256] = "";
				for (j=0; j<hwnds.GetSize(); j++)
				{
					GetWindowText(hwnds.Get(j), buf2, sizeof(buf2));
					if (strcmp(buf, buf2) < 0)
						break;
				}
				hwnds.Insert(j, w);
			}
		}
	}

	// find out the window to be displayed
	g_lastFocusHideOthers += (int)_ct->user; // not a % !
	if (g_lastFocusHideOthers < 0)
		g_lastFocusHideOthers = hwnds.GetSize();
	else if (g_lastFocusHideOthers == (hwnds.GetSize()+1))
		g_lastFocusHideOthers = 0;

	// focus one & hide others
	if (g_lastFocusHideOthers < hwnds.GetSize())
	{
		int lastOk = g_lastFocusHideOthers;
		for (int i=0; i < hwnds.GetSize(); i++)
		{
			if (HWND h = hwnds.Get(i))
			{
				if (i != g_lastFocusHideOthers) {
					if (h != GetMainHwnd())
						ShowWindow(h, SW_HIDE);
				}
				else {
					ShowWindow(h, SW_SHOW);
					lastOk = i;
				}
			}
		}
		SetForegroundWindow(hwnds.Get(lastOk));
	}
	// focus main window & unhide others
	else
	{
		for (int i=0; i<hwnds.GetSize(); i++)
			ShowWindow(hwnds.Get(i), SW_SHOW);
		SetForegroundWindow(GetMainHwnd());
	}
}

void FocusMainWindowCloseOthers(COMMAND_T* _ct)
{
	WDL_PtrList<HWND__> hwnds;
	EnumWindows(EnumReaWindows, (LPARAM)&hwnds);
	for (int i=0; i < hwnds.GetSize(); i++)
		if (HWND w = hwnds.Get(i))
			if (w != GetMainHwnd() && IsWindowVisible(w))
				SendMessage(w, WM_SYSCOMMAND, SC_CLOSE, 0);
	SetForegroundWindow(GetMainHwnd());
}

#endif


///////////////////////////////////////////////////////////////////////////////
// WALTER helpers
// Note: intentionally not localized..
///////////////////////////////////////////////////////////////////////////////

void ShowThemeHelper(WDL_FastString* _report, HWND _hwnd, bool _mcp, bool _sel)
{
	WDL_PtrList<HWND__> hwnds;
	EnumChildWindows(_hwnd, EnumXCPWindows, (LPARAM)&hwnds);
	for (int i=0; i < hwnds.GetSize(); i++)
	{
		if (HWND w = hwnds.Get(i))
		{
//			if (IsWindowVisible(w))
			{
				bool mcpChild = IsChildOf(w, __localizeFunc("Mixer", "DLG_151", 0));
				if ((_mcp && mcpChild) || (!_mcp && !mcpChild))
				{
					MediaTrack* tr = (MediaTrack*)GetWindowLongPtr(w, GWLP_USERDATA);
					int trIdx = CSurf_TrackToID(tr, false);
					if (trIdx>=0 && (!_sel || (_sel && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))))
					{
						RECT r;	GetClientRect(w, &r);
						char* trName = (char*)GetSetMediaTrackInfo(tr, "P_NAME", NULL);
						_report->AppendFormatted(
							1024,
							__LOCALIZE_VERFMT("%s Track #%d '%s': W=%d, H=%d\n","theme_helper"),
							_mcp ? __LOCALIZE("MCP","theme_helper") : __LOCALIZE("TCP","theme_helper"),
							trIdx==-1 ? 0 : trIdx,
							trIdx==-1 ? __LOCALIZE("[MASTER]","theme_helper") : (trName?trName:""),
							(int)(r.right-r.left),
							(int)(r.bottom-r.top));
					}
				}
			}
		}
	}
}

void ShowThemeHelper(COMMAND_T* _ct)
{
	WDL_FastString report("");
	ShowThemeHelper(&report, GetMainHwnd(), false, (int)_ct->user == 1);
	if ((int)_ct->user != 1 && report.GetLength())
		report.Append("\n");

	HWND w = GetReaHwndByTitle(__localizeFunc("Mixer Master", "mixer", 0));
	if (w && IsWindowVisible(w)) 
		ShowThemeHelper(&report, w, true, (int)_ct->user == 1);

	w = GetReaHwndByTitle(__localizeFunc("Mixer", "DLG_151", 0));
	if (w && IsWindowVisible(w)) 
		ShowThemeHelper(&report, w, true, (int)_ct->user == 1);

	SNM_ShowMsg(report.Get(), __LOCALIZE("S&M - Theme Helper","theme_helper"));
}


///////////////////////////////////////////////////////////////////////////////
// Action list hacks
// Everything here could be cleaned up thanks to schwa's suggestion here:
// http://forum.cockos.com/showpost.php?p=1144769&postcount=7
// .. unfortunately, it remained a suggestion (my fault?)
///////////////////////////////////////////////////////////////////////////////

HWND GetActionListBox(char* _currentSection, int _sectionSz)
{
	HWND actionsWnd = GetReaHwndByTitle(__localizeFunc("Actions", "DLG_274", 0));
	if (actionsWnd && _currentSection)
		if (HWND cbSection = GetDlgItem(actionsWnd, 0x525))
			GetWindowText(cbSection, _currentSection, _sectionSz);
	return (actionsWnd ? GetDlgItem(actionsWnd, 0x52B) : NULL);
}


// overrides some wdl's list view funcs to avoid cast issues on osx
// (useful with native list views which are SWELL_ListView but that were not instanciated by the extension..)
#ifdef _WIN32
#define SNM_ListView_GetSelectedCount ListView_GetSelectedCount
#define SNM_ListView_GetItemCount ListView_GetItemCount
#define SNM_ListView_GetItem ListView_GetItem
#define SNM_ListView_GetItemText ListView_GetItemText
#define SNM_ListView_SetItemState ListView_SetItemState
#else
#define SNM_ListView_GetSelectedCount ListView_GetSelectedCountCast
#define SNM_ListView_GetItemCount ListView_GetItemCountCast
#define SNM_ListView_GetItem ListView_GetItemCast
#define SNM_ListView_GetItemText ListView_GetItemTextCast
#define SNM_ListView_SetItemState ListView_SetItemStateCast
#endif


// returns the list view's selected item, or:
// -1 if the action wnd is not opened
// deprecated: -2 if the custom id cannot be retrieved (hidden column)
// -3 if there is no selected action
// note: no multi-selection mgmt here..
int GetSelectedAction(char* _section, int _secSize, int* _cmdId, char* _id, int _idSize, char* _desc, int _descSize)
{
	HWND hList = GetActionListBox(_section, _secSize);
	if (hList)
	{
		if (SNM_ListView_GetSelectedCount(hList))
		{
			LVITEM li;
			li.mask = LVIF_STATE | LVIF_PARAM;
			li.stateMask = LVIS_SELECTED;
			li.iSubItem = 0;
			for (int i=0; i < SNM_ListView_GetItemCount(hList); i++)
			{
				li.iItem = i;
				SNM_ListView_GetItem(hList, &li);
				if (li.state == LVIS_SELECTED)
				{
					int cmdId = (int)li.lParam;
					if (_cmdId) *_cmdId = cmdId;

					char actionName[SNM_MAX_ACTION_NAME_LEN] = "";
					SNM_ListView_GetItemText(hList, i, 1, actionName, SNM_MAX_ACTION_NAME_LEN); //JFB displaytodata? (ok: columns not re-orderable yet)
					if (_desc && _descSize > 0)
						lstrcpyn(_desc, actionName, _descSize);

					if (_id && _idSize > 0)
					{
						const char *custid=ReverseNamedCommandLookup(cmdId);
						if (custid) _snprintfStrict(_id, _idSize, "_%s", custid);
						else _snprintfStrict(_id, _idSize, "%d", cmdId);
					}
					return i;
				}
			}
		}
		else
			return -3;
	}
	return -1;
}

bool GetSelectedAction(char* _idstrOut, int _idStrSz, KbdSectionInfo* _expectedSection)
{
	if (!_expectedSection)
		_expectedSection = SNM_GetActionSection(SNM_SEC_IDX_MAIN);

	char section[SNM_MAX_SECTION_NAME_LEN] = "";
	int actionId, selItem = GetSelectedAction(section, SNM_MAX_SECTION_NAME_LEN, &actionId, _idstrOut, _idStrSz);
	if (selItem!=-1 && strcmp(section, __localizeFunc(_expectedSection->name,"accel_sec",0))) // err -1 has the priority
		selItem = -4;
	switch (selItem)
	{
		case -4: {
			char msg[256]="";
			_snprintfSafe(msg,
				sizeof(msg),
				__LOCALIZE_VERFMT("The section \"%s\" is not selected in the Actions window!\nDo you want to select it?","sws_mbox"),
				__localizeFunc(_expectedSection->name,"accel_sec",0));
			if (IDYES == MessageBox(GetMainHwnd(), msg, __LOCALIZE("S&M - Error","sws_mbox"), MB_YESNO))
				ShowActionList(_expectedSection, NULL);
			return false;
		}
		case -3:
			MessageBox(GetMainHwnd(),
				__LOCALIZE("There is no selected action in the Actions window!","sws_mbox"),
				__LOCALIZE("S&M - Error","sws_mbox"),
				MB_OK);
			return false;
/* deprecated thanks to the new API func ReverseNamedCommandLookup()
		case -2:
			MessageBox(GetMainHwnd(),
				__LOCALIZE("Action IDs are not displayed in the Actions window!\nTip: right-click on the table header > Show action IDs.","sws_mbox"),
				__LOCALIZE("S&M - Error","sws_mbox"),
				MB_OK);
			return false;
*/
		case -1:
			if (IDYES == MessageBox(
				GetMainHwnd(),
				__LOCALIZE("Actions window not opened!\nDo you want to open it?","sws_mbox"),
				__LOCALIZE("S&M - Error","sws_mbox"),
				MB_YESNO))
			{
				ShowActionList(_expectedSection, NULL);
			}
			return false;
	}
	return true;
}

// dump actions or the wiki ALR summary for the current section *as displayed* in the action dlg
// see http://forum.cockos.com/showthread.php?t=61929 and http://wiki.cockos.com/wiki/index.php/Action_List_Reference
// _type: 1 & 2 for ALR wiki (1=native actions, 2=SWS)
// _type: 3, 4 & 5 for basic dump (3=native actions, 4=SWS, 5=user macros)
// API LIMITATION: could be much simplified if we could retrieve string ids fom cmdIds, 
//                 e.g. if we had: const char* ReverseNamedCommandLookup(int)
bool DumpActionList(int _type, const char* _title, const char* _lineFormat, const char* _heading, const char* _ending)
{
	// keep the help text on a signle line (for the langpack gen tool..)
	const char* help = __LOCALIZE("Note: this action needs the action window to be opened (with the section you want to dump).\nIt obeys the action list filter, so to get the full list you need a clear filter.\nExample: to dump a MIDI editor action list the easiest way is to assign a keyboard shortcut or\ntoolbar button, open the MIDI editor section of the action list and fire the shortcut/dump action.","sws_mbox");
	char currentSection[SNM_MAX_SECTION_NAME_LEN] = "";
	HWND hList = GetActionListBox(currentSection, SNM_MAX_SECTION_NAME_LEN);
	if (hList && currentSection)
	{
		char sectionURL[SNM_MAX_SECTION_NAME_LEN] = ""; 
		if (!GetSectionURL(_type==1||_type==2, currentSection, sectionURL, SNM_MAX_SECTION_NAME_LEN))
		{
			MessageBox(GetMainHwnd(), __LOCALIZE("Dump failed: unknown section!","sws_mbox"), _title, MB_OK);
			return false;
		}

		char name[SNM_MAX_SECTION_NAME_LEN*2] = "", fn[SNM_MAX_PATH] = "";
		if (_snprintfStrict(name, sizeof(name), "%s_Section%s.txt", sectionURL, (_type==2||_type==4) ? "_SWS" : _type==5 ? "_Custom" : "") <= 0)
			*name = '\0';
		if (!BrowseForSaveFile(_title, GetResourcePath(), name, SNM_TXT_EXT_LIST, fn, sizeof(fn)))
			return false;

		if (FILE* f = fopenUTF8(fn, "w"))
		{
			// flush
			fputs("\n", f);
			fclose(f);

			f = fopenUTF8(fn, "a"); 
			if (!f)
				return false; // just in case..

			if (_heading)
				fprintf(f, "%s", _heading);

			int nbWrote=0;
			LVITEM li;
			li.mask = LVIF_STATE | LVIF_PARAM;
			li.iSubItem = 0;
			for (int i=0; i < SNM_ListView_GetItemCount(hList); i++)
			{
				li.iItem = i;
				SNM_ListView_GetItem(hList, &li);
				int cmdId = (int)li.lParam;

				char custId[SNM_MAX_ACTION_CUSTID_LEN] = "";
				char cmdName[SNM_MAX_ACTION_NAME_LEN] = "";
				SNM_ListView_GetItemText(hList, i, 1, cmdName, SNM_MAX_ACTION_NAME_LEN);
				SNM_ListView_GetItemText(hList, i, 4, custId, SNM_MAX_ACTION_CUSTID_LEN);

				bool isCustom = IsMacroOrScript(cmdName);
				int isSws = IsSwsAction(cmdName);
				bool isSwsCustom = !IsLocalizableAction(custId);
				if (((_type==1||_type==3) && !isSws && !isCustom && !isSwsCustom) || 
					((_type==2||_type==4) && isSws  && !isCustom && !isSwsCustom) ||
					(_type==5 && (isCustom||isSwsCustom)))
				{
					if (!*custId && _snprintfStrict(custId, sizeof(custId), "%d", cmdId) <= 0) // for native actions
						*custId = '\0';
					if (*custId) {
						fprintf(f, _lineFormat, sectionURL, custId, cmdName, custId);
						nbWrote++;
					}
				}
			}
			if (_ending)
				fprintf(f, "%s", _ending); 

			fclose(f);

			WDL_FastString msg;
			msg.SetFormatted(SNM_MAX_PATH, 
				nbWrote ? __LOCALIZE_VERFMT("Wrote %s","sws_mbox") :
				__LOCALIZE_VERFMT("No action wrote in %s!\nProbable cause: filtered action list, no matching actions, etc...","sws_mbox"),
				fn);
			msg.Append("\n\n");
			msg.Append(help);
			MessageBox(GetMainHwnd(), msg.Get(), _title, MB_OK);
			return true;
		}
		else
			MessageBox(GetMainHwnd(), __LOCALIZE("Dump failed: unable to write to file!","sws_mbox"), _title, MB_OK);
	}
	else
	{
		WDL_FastString msg(__LOCALIZE("Dump failed: Actions window not opened!","sws_mbox"));
		msg.Append("\n\n");
		msg.Append(help);
		MessageBox(GetMainHwnd(), msg.Get(), _title, MB_OK);
	}
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

void DumpActionList(COMMAND_T* _ct)
{
	DumpActionList((int)_ct->user,
		__LOCALIZE("S&M - Dump action list","sws_mbox"),
		"%s\t%s\t%s\n",
		"Section\tId\tAction\n",
		NULL);
}


#undef SNM_ListView_GetSelectedCount
#undef SNM_ListView_GetItemCount
#undef SNM_ListView_GetItem
#undef SNM_ListView_GetItemText
#undef SNM_ListView_SetItemState


///////////////////////////////////////////////////////////////////////////////
// Track FX chain windows: show/hide
// note: Cockos' TrackFX_GetChainVisible() and my GetSelectedTrackFX() are not
// exactly the same. Here, TrackFX_GetChainVisible() is used to get a selected 
// FX in a -visible- chain, GetSelectedTrackFX() will always return one.
///////////////////////////////////////////////////////////////////////////////

void ShowFXChain(COMMAND_T* _ct)
{
	int focusedFX = _ct ? (int)_ct->user : -1; // -1: current selected fx
	for (int i=0; i <= GetNumTracks(); i++) // incl. master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		// NULL _ct => all tracks, selected tracks otherwise
		if (tr && (!_ct || (_ct && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))))
			TrackFX_Show(tr, (focusedFX == -1 ? GetSelectedTrackFX(tr) : focusedFX), 1);
	}
}

// _ct: NULL=all tracks, selected tracks otherwise
void HideFXChain(COMMAND_T* _ct) 
{
	for (int i=0; i <= GetNumTracks(); i++) // incl. master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && (!_ct || (_ct && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))))
			TrackFX_Show(tr, GetSelectedTrackFX(tr), 0); // includes an undo point
	}
}

void ToggleFXChain(COMMAND_T* _ct) 
{
	for (int i=0; i <= GetNumTracks(); i++) // incl. master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		// NULL _ct => all tracks, selected tracks otherwise
		if (tr && (!_ct || (_ct && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL)))) {
			int currentFX = TrackFX_GetChainVisible(tr);
			TrackFX_Show(tr, GetSelectedTrackFX(tr), (currentFX == -2 || currentFX >= 0) ? 0 : 1);
		}
	}
}

int IsToggleFXChain(COMMAND_T * _ct) 
{
	int selTrCount = SNM_CountSelectedTracks(NULL, true);
	// single track selection: we can return a toggle state
	if (selTrCount == 1)
		return (TrackFX_GetChainVisible(SNM_GetSelectedTrack(NULL, 0, true)) != -1);
	// several tracks selected: possible mix of different states 
	// => return a fake toggle state (best effort)
	else if (selTrCount)
		return GetFakeToggleState(_ct);
	return false;
}

void ShowAllFXChainsWindows(COMMAND_T* _ct) {
	ShowFXChain(NULL);
}
void CloseAllFXChainsWindows(COMMAND_T * _ct) {
	HideFXChain(NULL);
}
void ToggleAllFXChainsWindows(COMMAND_T * _ct) {
	ToggleFXChain(NULL);
}


///////////////////////////////////////////////////////////////////////////////
// Floating track FX windows: show/hide
///////////////////////////////////////////////////////////////////////////////

// _fx = -1 for selected FX
void ToggleFloatFX(MediaTrack* _tr, int _fx)
{
	if (_tr && _fx < TrackFX_GetCount(_tr))
	{
		int currenSel = GetSelectedTrackFX(_tr); // avoids several parsings
		if (TrackFX_GetFloatingWindow(_tr, (_fx == -1 ? currenSel : _fx)))
			TrackFX_Show(_tr, (_fx == -1 ? currenSel : _fx), 2);
		else
			TrackFX_Show(_tr, (_fx == -1 ? currenSel : _fx), 3);
	}
}

// _all=true: all FXs for all tracks, false: selected tracks + given _fx
// _fx=-1: current selected FX. Ignored when _all == true.
// showflag=0 for toggle, =2 for hide floating window (valid index), =3 for show floating window (valid index)
void FloatUnfloatFXs(MediaTrack* _tr, bool _all, int _showFlag, int _fx, bool _selTracks) 
{
	bool matchTrack = (_tr && (!_selTracks || (_selTracks && *(int*)GetSetMediaTrackInfo(_tr, "I_SELECTED", NULL))));
	if (_all && matchTrack)
	{
		int nbFx = TrackFX_GetCount(_tr);
		for (int j=0; j < nbFx; j++) {
			if (!_showFlag) ToggleFloatFX(_tr, j);
			else TrackFX_Show(_tr, j, _showFlag);
		}
	}
	else if (!_all && matchTrack) {
		if (!_showFlag) ToggleFloatFX(_tr, (_fx == -1 ? GetSelectedTrackFX(_tr) : _fx));
		else TrackFX_Show(_tr, (_fx == -1 ? GetSelectedTrackFX(_tr) : _fx), _showFlag); // offline fx is managed
	}
}

// _all: true for all FXs/tracks, false for selected tracks + for given the given _fx
// _fx = -1, for current selected FX. Ignored when _all == true.
// showflag=0 for toggle, =2 for hide floating window (index valid), =3 for show floating window (index valid)
void FloatUnfloatFXs(bool _all, int _showFlag, int _fx, bool _selTracks) 
{
	for (int i=0; i <= GetNumTracks(); i++) // incl. master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && (_all || !_selTracks || (_selTracks && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))))
			FloatUnfloatFXs(tr, _all, _showFlag, _fx, _selTracks);
	}
}

void FloatFX(COMMAND_T* _ct) {
	FloatUnfloatFXs(false, 3, (int)_ct->user, true);
}
void UnfloatFX(COMMAND_T* _ct) {
	FloatUnfloatFXs(false, 2, (int)_ct->user, true);
}
void ToggleFloatFX(COMMAND_T* _ct) {
	FloatUnfloatFXs(false, 0, (int)_ct->user, true);
}

void ShowAllFXWindows(COMMAND_T * _ct) {
	FloatUnfloatFXs(true, 3, -1, ((int)_ct->user == 1));
}
void CloseAllFXWindows(COMMAND_T * _ct) {
	FloatUnfloatFXs(true, 2, -1, ((int)_ct->user == 1));
}
void ToggleAllFXWindows(COMMAND_T * _ct) {
	FloatUnfloatFXs(true, 0, -1, ((int)_ct->user == 1));
}

void CloseAllFXWindowsExceptFocused(COMMAND_T * _ct)
{
	HWND w = GetForegroundWindow();
	for (int i=0; i <= GetNumTracks(); i++) // incl. master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && SWS_IsWindow(w))
		{
			int fxCount = TrackFX_GetCount(tr);
			for (int j = 0; j < fxCount; j++)
			{
				HWND w2 = TrackFX_GetFloatingWindow(tr,j);
				if (!SWS_IsWindow(w2) || w != w2)	
					FloatUnfloatFXs(tr, false, 2, j, false); // close
			}
		}
	}
}

// returns -1 if none
int GetFocusedTrackFXWnd(MediaTrack* _tr)
{
	int trIdx, itemIdx, fxIdx;
	if (GetFocusedFX(&trIdx, &itemIdx, &fxIdx) && trIdx>=0)	// new native API since 4.20
		if (_tr == CSurf_TrackFromID(trIdx, false))
			return fxIdx;
	return -1;
}

// returns -1 if none
int GetFirstTrackFXWnd(MediaTrack* _tr, int _dir)
{
	if (_tr)
		if (int fxCount = TrackFX_GetCount(_tr))
			for (int j = (_dir > 0 ? 0 : (fxCount-1)); (j < fxCount) && (j>=0); j+=_dir)
				if (SWS_IsWindow(TrackFX_GetFloatingWindow(_tr, j)))
					return j;
	return -1;
}


///////////////////////////////////////////////////////////////////////////////
// Floating track FX windows: cycle focus
///////////////////////////////////////////////////////////////////////////////

bool CycleTracksAndFXs(int _trStart, int _fxStart, int _dir, bool _selectedTracks,
		bool (*job)(MediaTrack*,int,bool), bool* _cycled) // see 2 "jobs" below..
{
	int cpt1 = 0;
	int i = _trStart;
	while (cpt1 <= GetNumTracks())
	{
		if (i > GetNumTracks()) {
			i = 0;
			*_cycled = (cpt1 > 0); // ie not the first loop
		}
		else if (i < 0)  {
			i = GetNumTracks();
			*_cycled = (cpt1 > 0); // ie not the first loop
		}

		MediaTrack* tr = CSurf_TrackFromID(i, false);
		int fxCount = tr ? TrackFX_GetCount(tr) : 0;
		if (tr && fxCount &&  (!_selectedTracks || (_selectedTracks && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))))
		{
			int cpt2 = 0;
			int j = (i==_trStart ? _fxStart+_dir : (_dir<0 ? fxCount-1 : 0));
			while (cpt2 < fxCount)
			{
				if (j >= fxCount || j < 0)
					break; // implies track cycle

				// perform custom stuff
				if (job(tr, j, _selectedTracks))
					return true;

				cpt2++;
				j += _dir;
			}
		}
		cpt1++;
		i += _dir;
	}
	return false;
}

bool FocusJob(MediaTrack* _tr, int _fx, bool _selectedTracks)
{
	HWND w2 = TrackFX_GetFloatingWindow(_tr,_fx);
	if (SWS_IsWindow(w2)) {
		SetForegroundWindow(w2);
		return true;
	}
	return false;
}

bool FloatOnlyJob(MediaTrack* _tr, int _fx, bool _selectedTracks)
{
	// hide others
	for (int i=0; i <= GetNumTracks(); i++) // incl. master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		int fxCount = (tr ? TrackFX_GetCount(tr) : 0);
		if (fxCount && (!_selectedTracks || (_selectedTracks && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))))
			for (int j = 0; j < fxCount; j++)
				if (tr != _tr || j != _fx) 
					FloatUnfloatFXs(tr, false, 2, j, true);
	}
	// float "only"
	FloatUnfloatFXs(_tr, false, 3, _fx, true);
	return true;
}

bool CycleFocusFXWnd(int _dir, bool _selectedTracks, bool* _cycled)
{
	if (!_selectedTracks || (_selectedTracks && SNM_CountSelectedTracks(NULL, true)))
	{
		MediaTrack* firstTrFound = NULL;
		int firstFXFound = -1;

		int i = (_dir < 0 ? GetNumTracks() : 0);
		while ((_dir < 0 ? i >= 0 : i <= GetNumTracks()))
		{
			MediaTrack* tr = CSurf_TrackFromID(i, false);
			if (tr && TrackFX_GetCount(tr) && 
				(!_selectedTracks || (_selectedTracks && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))))
			{
				int prevFocused = GetFocusedTrackFXWnd(tr);
				if (firstFXFound < 0)
					firstFXFound = GetFirstTrackFXWnd(tr, _dir);
				if (!firstTrFound && firstFXFound >= 0)
					firstTrFound = tr;
				if (prevFocused >= 0 && CycleTracksAndFXs(i, prevFocused, _dir, _selectedTracks, FocusJob, _cycled))
					return true;
			}
			i += _dir; // +1, -1
		}

		// there was no already focused window if we're here..
		// => focus the 1st found one
		if (firstTrFound) 
			return FocusJob(firstTrFound, firstFXFound, _selectedTracks);
	}
	return false;
}


WDL_PtrList_DeleteOnDestroy<SNM_TrackInt> g_hiddenFloatingWindows;
int g_lastCycleFocusFXDirection = 0; //used for direction change..

void CycleFocusFXMainWnd(int _dir, bool _selectedTracks, bool _showmain) 
{
	bool cycled = false;

	// show back floating FX if needed
	if (_showmain && g_hiddenFloatingWindows.GetSize())
	{
		SNM_TrackInt* hiddenIds = NULL;
		bool dirChanged = (g_lastCycleFocusFXDirection != _dir);
		for (int i = (dirChanged ? 0 : (g_hiddenFloatingWindows.GetSize()-1)); 
			(i < g_hiddenFloatingWindows.GetSize()) && (i >=0); 
			i += dirChanged ? 1 : -1)
		{
			hiddenIds = g_hiddenFloatingWindows.Get(i);
			FloatUnfloatFXs(hiddenIds->m_tr, false, 3, hiddenIds->m_int, _selectedTracks);
		}
		if (hiddenIds)
			FocusJob(hiddenIds->m_tr, hiddenIds->m_int, _selectedTracks);
		g_hiddenFloatingWindows.Empty(true);
		return;
	}

	if (CycleFocusFXWnd(_dir, _selectedTracks, &cycled))
	{
		if (_showmain && cycled)
		{
			g_lastCycleFocusFXDirection = _dir;

			// toggle show/hide all floating FX for all tracks
			int trCpt = _dir > 0 ? 0 : GetNumTracks();
			while (trCpt <= GetNumTracks() && trCpt >= 0)
			{
				MediaTrack* tr = CSurf_TrackFromID(trCpt, false);
				int fxCount = (tr ? TrackFX_GetCount(tr) : 0);
				if (fxCount && (!_selectedTracks || (_selectedTracks && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))))
				{
					for (int j = (_dir > 0 ? 0 : (fxCount-1)); (j < fxCount) && (j>=0); j+=_dir)
					{
						HWND w = TrackFX_GetFloatingWindow(tr, j);
						if (SWS_IsWindow(w)) { // store ids (to show it back later) and hide it
							g_hiddenFloatingWindows.Add(new SNM_TrackInt(tr, j));
							FloatUnfloatFXs(tr, false, 2, j, _selectedTracks);
						}
					}
				}
				trCpt += _dir; // +1 or -1
			}
			SetForegroundWindow(GetMainHwnd()); 
		}
	}
}

#ifdef _SNM_MISC
void CycleFocusFXWndAllTracks(COMMAND_T* _ct) {
	CycleFocusFXMainWnd((int)_ct->user, false, false);
}
void CycleFocusFXWndSelTracks(COMMAND_T* _ct) {
	CycleFocusFXMainWnd((int)_ct->user, true, false);
}
#endif

void CycleFocusFXMainWndAllTracks(COMMAND_T* _ct) {
	CycleFocusFXMainWnd((int)_ct->user, false, true);
}

void CycleFocusFXMainWndSelTracks(COMMAND_T* _ct) {
	CycleFocusFXMainWnd((int)_ct->user, true, true);
}

void CycleFloatFXWndSelTracks(COMMAND_T* _ct)
{
	int dir = (int)_ct->user;
	if (SNM_CountSelectedTracks(NULL, true))
	{
		MediaTrack* firstTrFound = NULL;
		int firstFXFound = -1;
		bool cycled = false; // not used yet..

		int i = (dir < 0 ? GetNumTracks() : 0);
		while ((dir < 0 ? i >= 0 : i <= GetNumTracks()))
		{
			MediaTrack* tr = CSurf_TrackFromID(i, false);
			int fxCount = (tr ? TrackFX_GetCount(tr) : 0);
			if (fxCount && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			{
				if (!firstTrFound) {
					firstTrFound = tr;
					firstFXFound = (dir < 0 ? (fxCount-1) : 0);
				}

				// specific case: make it work even no FX window is focused
				// (classic pitfall when the action list is focused, see
				// http://forum.cockos.com/showpost.php?p=708536&postcount=305)
				int prevFocused = GetFocusedTrackFXWnd(tr);
				if (prevFocused < 0)
					prevFocused = GetFirstTrackFXWnd(tr, dir);

				if (prevFocused >= 0 && CycleTracksAndFXs(i, prevFocused, dir, true, FloatOnlyJob, &cycled))
					return;
			}
			i += dir; // +1, -1
		}

		// there was no focused window if we're here..
		// => float only the 1st found one
		if (firstTrFound) 
			FloatOnlyJob(firstTrFound, firstFXFound, true);
	}
}

