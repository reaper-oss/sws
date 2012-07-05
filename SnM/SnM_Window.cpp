/******************************************************************************
/ SnM_Windows.cpp
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
// Misc window actions/helpers
///////////////////////////////////////////////////////////////////////////////

bool SNM_IsActiveWindow(HWND _h) {
	if (!_h || !IsWindow(_h))
		return false;
	return (GetFocus() == _h || IsChild(_h, GetFocus()));
}

void AddUniqueHnwd(HWND _hAdd, HWND* _hwnds, int* count)
{
	for (int i=0; i < *count; i++)
		if(_hwnds[i] == _hAdd)
			return;
	_hwnds[(*count)++] = _hAdd;
}

bool IsChildOf(HWND _hChild, const char* _title)
{
	HWND hCurrent = _hChild;
	char buf[512] = "";
	while (hCurrent) 
	{
		hCurrent = GetParent(hCurrent);
		if (hCurrent)
		{
			GetWindowText(hCurrent, buf, 512);
			if (!strncmp(buf, _title, strlen(_title)))
				return true;
		}
	}
	return false;
}

#ifdef _WIN32

#define MAX_ENUM_CHILD_HWNDS 512
#define MAX_ENUM_HWNDS 256

//JFB TODO: cleanup with WDL_PtrList instead
static int g_hwndsCount = 0;
static HWND g_hwnds[MAX_ENUM_CHILD_HWNDS];
static int g_childHwndsCount = 0;
static HWND g_childHwnds[MAX_ENUM_CHILD_HWNDS];

BOOL CALLBACK EnumReaWindows(HWND _hwnd, LPARAM _lParam)
{
   HWND hCurrent, hNew;
   hCurrent = _hwnd;
   hNew = hCurrent;
   while (hNew != NULL) {
	   hNew = GetParent(hCurrent);
	   if (hNew != NULL)
		   hCurrent = hNew;
   }
   if (hCurrent == GetMainHwnd())
   {
	   	char buf[256];
		GetClassName(_hwnd, buf, sizeof(buf));
		if (!strcmp(buf, "#32770"))
			AddUniqueHnwd(_hwnd, g_hwnds, &g_hwndsCount); 
   }
   return TRUE;
} 

static BOOL CALLBACK EnumReaChildWindows(HWND _hwnd, LPARAM _lParam)
{
	char str[256];
	GetClassName(_hwnd, str, sizeof(str));
	if(strcmp(str, "#32770") == 0)
	{
		if (g_childHwndsCount < MAX_ENUM_CHILD_HWNDS)
			AddUniqueHnwd(_hwnd, g_childHwnds, &g_childHwndsCount); 
		EnumChildWindows(_hwnd, EnumReaChildWindows, _lParam + 1);
	}
	return TRUE;
}
#endif

HWND GetReaChildWindowByTitle(HWND _parent, const char* _title)
{
#ifdef _WIN32
	char buf[512] = "";
	g_childHwndsCount = 0;
	EnumChildWindows(_parent, EnumReaChildWindows, 0); 
	for (int i=0; i < g_childHwndsCount && i < MAX_ENUM_CHILD_HWNDS; i++)
	{
		HWND w = g_childHwnds[i];
		GetWindowText(w, buf, 512);
		if (!strcmp(buf, _title))
			return w;
	}
#endif
	return NULL;
}

#ifndef _WIN32
// note: _title and _dockerName must be localized
HWND GetReaWindowByTitleInFloatingDocker(const char* _title, const char* _dockerName)
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
#endif

// note: _title must be localized
HWND GetReaWindowByTitle(const char* _title)
{
#ifdef _WIN32
	// docked in main window?
	HWND w = GetReaChildWindowByTitle(GetMainHwnd(), _title);
	if (w) return w;

	g_hwndsCount = 0;
	char buf[512] = "";
	EnumWindows(EnumReaWindows, 0);
	for (int i=0; i < g_hwndsCount && i < MAX_ENUM_HWNDS; i++)
	{
		w = g_hwnds[i];
		GetWindowText(w, buf, 512);

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

#else // almost works on win + osx (since wdl 7cf02d) but utf8 issue on win

	// in a floating window?
	HWND w = FindWindowEx(NULL, NULL, NULL, _title);
	if (w) return w;

	// docked in main window?
	HWND hdock = FindWindowEx(GetMainHwnd(), NULL, NULL, "REAPER_dock");
	while(hdock) {
		if (w = FindWindowEx(hdock, NULL, NULL, _title)) return w;
		hdock = FindWindowEx(GetMainHwnd(), hdock, NULL, "REAPER_dock");
	}
	// in a floating docker (w/ other hwnds)?
	if (w = GetReaWindowByTitleInFloatingDocker(_title, __localizeFunc("Docker", "docker", 0)))
		return w;
	// in a floating docker (w/o other hwnds)?
	{
		char dockerName[256]="";
		if (_snprintfStrict(dockerName, sizeof(dockerName), "%s%s", _title, __localizeFunc(" (docked)", "docker", 0)) > 0)
			if (w = GetReaWindowByTitleInFloatingDocker(_title, dockerName))
				return w;
	}
#endif
	return NULL;
}


///////////////////////////////////////////////////////////////////////////////
// WALTER helpers
///////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
static BOOL CALLBACK EnumXCPWindows(HWND _hwnd, LPARAM _lParam)
{
	char str[256] = "";
	GetClassName(_hwnd, str, 256);
	if (!strcmp(str, "REAPERVirtWndDlgHost"))
		AddUniqueHnwd(_hwnd, g_childHwnds, &g_childHwndsCount);
	else 
		EnumChildWindows(_hwnd, EnumXCPWindows, _lParam);
	return TRUE;
}
#endif

void ShowThemeHelper(WDL_FastString* _report, HWND _hwnd, bool _mcp, bool _sel)
{
#ifdef _WIN32
	g_childHwndsCount = 0;
	EnumChildWindows(_hwnd, EnumXCPWindows, 0);
	for (int i=0; i < g_childHwndsCount && i < MAX_ENUM_CHILD_HWNDS; i++)
	{
		HWND w = g_childHwnds[i];
		if (IsWindowVisible(w))
		{
			bool mcpChild = IsChildOf(w, __localizeFunc("Mixer", "DLG_151", 0));
			if ((_mcp && mcpChild) || (!_mcp && !mcpChild))
			{
				MediaTrack* tr = (MediaTrack*)GetWindowLongPtr(w, GWLP_USERDATA);

				// CSurf_TrackToID() would fail here but it is ok atm: win only
				// (IP_TRACKNUMBER is a casting nightmare on OSX)
				int trIdx = (int)GetSetMediaTrackInfo(tr, "IP_TRACKNUMBER", NULL);
				if (trIdx && (!_sel || (_sel && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))))
				{
					RECT r;	GetClientRect(w, &r);
					char* trName = (char*)GetSetMediaTrackInfo(tr, "P_NAME", NULL);
					_report->AppendFormatted(512, "%s Track #%d '%s': W=%d, H=%d\n", _mcp ? "MCP" : "TCP", trIdx==-1 ? 0 : trIdx, trIdx==-1 ? "[MASTER]" : (trName?trName:""), r.right-r.left, r.bottom-r.top);
				}
			}
		}
	}
#endif
} 

void ShowThemeHelper(COMMAND_T* _ct)
{
	WDL_FastString report("");
	ShowThemeHelper(&report, GetMainHwnd(), false, (int)_ct->user == 1);
	if ((int)_ct->user != 1 && report.GetLength())
		report.Append("\n");

	HWND w = GetReaWindowByTitle(__localizeFunc("Mixer Master", "mixer", 0));
	if (w && IsWindowVisible(w)) 
		ShowThemeHelper(&report, w, true, (int)_ct->user == 1);

	w = GetReaWindowByTitle(__localizeFunc("Mixer", "DLG_151", 0));
	if (w && IsWindowVisible(w)) 
		ShowThemeHelper(&report, w, true, (int)_ct->user == 1);

	SNM_ShowMsg(report.Get(), "S&M - Theme Helper");
}


///////////////////////////////////////////////////////////////////////////////
// Action list helpers
///////////////////////////////////////////////////////////////////////////////

HWND GetActionListBox(char* _currentSection, int _sectionSz)
{
	HWND actionsWnd = GetReaWindowByTitle(__localizeFunc("Actions", "DLG_274", 0));
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
#else
#define SNM_ListView_GetSelectedCount ListView_GetSelectedCountCast
#define SNM_ListView_GetItemCount ListView_GetItemCountCast
#define SNM_ListView_GetItem ListView_GetItemCast
#define SNM_ListView_GetItemText ListView_GetItemTextCast
#endif

// returns the list view's selected item, -1 if failed, -2 if the related action's custom id cannot be retrieved (hidden column)
// note: no multi-selection mgmt here..
// API LIMITATION: things like kbd_getTextFromCmd() cannot work for other sections than the main one
int GetSelectedAction(char* _section, int _secSize, int* _cmdId, char* _id, int _idSize, char* _desc, int _descSize)
{
	HWND hList = GetActionListBox(_section, _secSize);
	if (hList && SNM_ListView_GetSelectedCount(hList))
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
					// SWS action? => get the custom id in a reliable way
					if (!_section || (_section && !strcmp(_section,__localizeFunc("Main","accel_sec",0))))
						if (COMMAND_T* ct = SWSGetCommandByID(cmdId))
							return (_snprintfStrict(_id, _idSize, "_%s", ct->id)>0 ? i : -1);

					// best effort to get the custom id (relies on displayed columns..)
					SNM_ListView_GetItemText(hList, i, 4, _id, _idSize);  //JFB displaytodata? (ok: columns not re-orderable yet)
					if (!*_id)
					{
						if (!IsMacro(actionName))
							return (_snprintfStrict(_id, _idSize, "%d", (int)li.lParam)>0 ? i : -1);
						else
							return -2;
					}
				}
				return i;
			}
		}
	}
	return -1;
}

// assumes the actions dlg is already opened with the right section!
bool SuckyLearnAction(int _cmdId)
{
	if (HWND h = GetReaWindowByTitle(__localizeFunc("Actions", "DLG_274", 0)))
	{
		char oldFilter[128] = "";
		GetWindowText(GetDlgItem(h, 0x52C), oldFilter, sizeof(oldFilter));
		SendMessage(h, WM_COMMAND, 0xB, 0); // clr filter

		if (HWND hList = (h ? GetDlgItem(h, 0x52B) : NULL))
		{
			ListView_SetItemState(hList, -1, 0, LVIS_SELECTED); //JFB!!! OSX // clr current sel

			LVITEM li;
			li.mask = LVIF_PARAM;
			li.stateMask = LVIS_SELECTED;
			li.iSubItem = 0;
			bool found = false;
			for (int i=0; i < SNM_ListView_GetItemCount(hList); i++)
			{
				li.iItem = i;
				SNM_ListView_GetItem(hList, &li);
				if (_cmdId == (int)li.lParam) {
					ListView_SetItemState(hList, i, LVIS_SELECTED, LVIS_SELECTED); //JFB!!! OSX
					found = true;
				}
			}

			// trigger the add (learn) button
			if (found)
			{
				SendMessage(h, WM_COMMAND, 0x8, 0);
				SetWindowText(GetDlgItem(h, 0x52C), oldFilter); // restore filter
				SendMessage(h, WM_CLOSE, 0, 0);
			}
		}
	}
	return false;
}


///////////////////////////////////////////////////////////////////////////////
// Track FX chain windows: show/hide
// note: Cockos' TrackFX_GetChainVisible() and GetSelectedTrackFX() are not the
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
			TrackFX_Show(tr, GetSelectedTrackFX(tr), 0);
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
	// fake toggle state update
	if (SNM_CountSelectedTracks(NULL, true) > 1)
		FakeToggle(_ct);
}

// for toggle state
bool IsToggleFXChain(COMMAND_T * _ct) 
{
	int selTrCount = SNM_CountSelectedTracks(NULL, true);
	// single track selection: we can return a toggle state
	if (selTrCount == 1)
		return (TrackFX_GetChainVisible(SNM_GetSelectedTrack(NULL, 0, true)) != -1);
	// several tracks selected: possible mix of different states 
	// => return a fake toggle state (best effort)
	else if (selTrCount)
		return FakeIsToggleAction(_ct);
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
	FakeToggle(_ct);
}


///////////////////////////////////////////////////////////////////////////////
// Floating track FX windows: show/hide
///////////////////////////////////////////////////////////////////////////////

// _fx = -1 for selected FX
void ToggleFloatFX(MediaTrack* _tr, int _fx)
{
	if (_tr &&  _fx < TrackFX_GetCount(_tr))
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
	FakeToggle(_ct);
}

void ShowAllFXWindows(COMMAND_T * _ct) {
	FloatUnfloatFXs(true, 3, -1, ((int)_ct->user == 1));
}
void CloseAllFXWindows(COMMAND_T * _ct) {
	FloatUnfloatFXs(true, 2, -1, ((int)_ct->user == 1));
}
void ToggleAllFXWindows(COMMAND_T * _ct) {
	FloatUnfloatFXs(true, 0, -1, ((int)_ct->user == 1));
	FakeToggle(_ct);
}

void CloseAllFXWindowsExceptFocused(COMMAND_T * _ct)
{
	HWND w = GetForegroundWindow();
	for (int i=0; i <= GetNumTracks(); i++) // incl. master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && IsWindow(w))
		{
			int fxCount = TrackFX_GetCount(tr);
			for (int j = 0; j < fxCount; j++)
			{
				HWND w2 = TrackFX_GetFloatingWindow(tr,j);
				if (!IsWindow(w2) || w != w2)	
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
				if (IsWindow(TrackFX_GetFloatingWindow(_tr, j)))
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
		if (i > GetNumTracks())  {
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
	if (IsWindow(w2)) {
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
						if (IsWindow(w)) { // store ids (to show it back later) and hide it
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
void CycleFocusFXWndAllTracks(COMMAND_T * _ct) {
	CycleFocusFXMainWnd((int)_ct->user, false, false);
}
void CycleFocusFXWndSelTracks(COMMAND_T * _ct) {
	CycleFocusFXMainWnd((int)_ct->user, true, false);
}
#endif

void CycleFocusFXMainWndAllTracks(COMMAND_T * _ct) {
	CycleFocusFXMainWnd((int)_ct->user, false, true);
}

void CycleFocusFXMainWndSelTracks(COMMAND_T * _ct) {
	CycleFocusFXMainWnd((int)_ct->user, true, true);
}

void CycleFloatFXWndSelTracks(COMMAND_T * _ct)
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


///////////////////////////////////////////////////////////////////////////////
// Other focus actions
///////////////////////////////////////////////////////////////////////////////

void CycleFocusWnd(COMMAND_T * _ct) 
{
#ifdef _WIN32
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
#endif
}


int g_lastFocusHideOthers = 0;

void CycleFocusHideOthersWnd(COMMAND_T * _ct) 
{
#ifdef _WIN32
	g_hwndsCount = 0;
	EnumWindows(EnumReaWindows, 0);
	AddUniqueHnwd(GetMainHwnd(), g_hwnds, &g_hwndsCount);

	// sort & filter windows
	WDL_PtrList<HWND> hwndList;
	for (int i =0; i < g_hwndsCount && i < MAX_ENUM_HWNDS; i++)
	{
		char buf[512] = "";
		GetWindowText(g_hwnds[i], buf, 512);
		if (strcmp(__localizeFunc("Item/track info","sws_DLG_131",0), buf) &&
			strcmp(__localizeFunc("Docker", "docker", 0), buf)) // "closed" floating dockers are in fact hidden (and not redrawn => issue)
		{
			int j = 0;
			for (j=0; j < hwndList.GetSize(); j++)
			{
				char buf2[512] = "";
				GetWindowText(*hwndList.Get(j), buf2, 512);
				if (strcmp(buf, buf2) < 0)
					break;
			}
			hwndList.Insert(j, &g_hwnds[i]);
		}
	}

	// find out the window to be displayed
	g_lastFocusHideOthers += (int)_ct->user; // not a % !
	if (g_lastFocusHideOthers < 0)
		g_lastFocusHideOthers = hwndList.GetSize();
	else  if (g_lastFocusHideOthers == (hwndList.GetSize()+1))
		g_lastFocusHideOthers = 0;

	// focus one & hide others
	if (g_lastFocusHideOthers < hwndList.GetSize())
	{
		int lastOk = g_lastFocusHideOthers;
		for (int i=0; i < hwndList.GetSize(); i++)
		{
			HWND h = *hwndList.Get(i);
			if (i != g_lastFocusHideOthers) {
				if (h != GetMainHwnd())
					ShowWindow(h, SW_HIDE);
			}
			else {
				ShowWindow(h, SW_SHOW);
				lastOk = i;
			}
		}
		SetForegroundWindow(*hwndList.Get(lastOk)); 
	}
	// focus main window & unhide others
	else
	{
		for (int i=0; i < hwndList.GetSize(); i++)
			ShowWindow(*hwndList.Get(i), SW_SHOW);
		SetForegroundWindow(GetMainHwnd()); 
	}
#endif
}

void FocusMainWindow(COMMAND_T* _ct) {
	SetForegroundWindow(GetMainHwnd()); 
}

void FocusMainWindowCloseOthers(COMMAND_T* _ct) 
{
#ifdef _WIN32
	g_hwndsCount = 0;
	EnumWindows(EnumReaWindows, 0); 
	for (int i=0; i < g_hwndsCount && i < MAX_ENUM_HWNDS; i++)
		if (g_hwnds[i] != GetMainHwnd() && IsWindowVisible(g_hwnds[i]))
			SendMessage(g_hwnds[i], WM_SYSCOMMAND, SC_CLOSE, 0);
#endif
	SetForegroundWindow(GetMainHwnd()); 
}


///////////////////////////////////////////////////////////////////////////////
// Other
///////////////////////////////////////////////////////////////////////////////

void GetVisibleTCPTracks(WDL_PtrList<void>* _trList)
{
#ifdef _WIN32
	g_childHwndsCount = 0;
	EnumChildWindows(GetMainHwnd(), EnumXCPWindows, 0);
	for (int i=0; i < g_childHwndsCount && i < MAX_ENUM_CHILD_HWNDS; i++)
	{
		HWND w = g_childHwnds[i];
		if (IsWindowVisible(w) && !IsChildOf(w, __localizeFunc("Mixer", "DLG_151", 0)))
			_trList->Add((void*)GetWindowLongPtr(w, GWLP_USERDATA));
	}
#endif
}

