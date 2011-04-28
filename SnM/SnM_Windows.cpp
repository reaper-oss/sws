/******************************************************************************
/ SnM_Windows.cpp
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
#include "SNM_ChunkParserPatcher.h"


///////////////////////////////////////////////////////////////////////////////
// Misc. window actions/helpers
///////////////////////////////////////////////////////////////////////////////

void AddUniqueHnwd(HWND _hAdd, HWND* _hwnds, int* count)
{
	for (int i=0; i < *count; i++)
		if(_hwnds[i] == _hAdd)
			return;
	_hwnds[(*count)++] = _hAdd;
}

bool IsChildOf(HWND _hChild, const char* _title, int _nComp)
{
	if (_nComp < 0)
		_nComp = strlen(_title);

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

//JFB TODO clean with WDL_PtrList instead
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

HWND GetReaChildWindowByTitle(HWND _parent, const char* _title, int _nComp)
{
#ifdef _WIN32
	char buf[512] = "";
	g_childHwndsCount = 0;
	EnumChildWindows(_parent, EnumReaChildWindows, 0); 
	for (int i=0; i < g_childHwndsCount && i < MAX_ENUM_CHILD_HWNDS; i++)
	{
		HWND w = g_childHwnds[i];
		GetWindowText(w, buf, 512);
		if (!strncmp(buf, _title, _nComp))
			return w;
	}
#endif
	return NULL;
}

HWND GetReaWindowByTitle(const char* _title, int _nComp)
{
#ifdef _WIN32
	if (_nComp < 0)
		_nComp = strlen(_title);

	// docked in main window ?
	HWND w = GetReaChildWindowByTitle(GetMainHwnd(), _title, _nComp);
	if (w)
		return w;

	g_hwndsCount = 0;
	char buf[512] = "";
	EnumWindows(EnumReaWindows, 0); 
	for (int i=0; i < g_hwndsCount && i < MAX_ENUM_HWNDS; i++)
	{
		w = g_hwnds[i];
		GetWindowText(w, buf, 512);

		// floating window ?
		if (!strcmp(_title, buf))
			return w;
		// in a floating docker ?
		else if (!strcmp("Docker", buf))
		{
			w = GetReaChildWindowByTitle(w, _title, _nComp);
			if (w) 
				return w;
		}
	}
#endif
	return NULL;
}

HWND SearchWindow(const char* _title)
{
	HWND searchedWnd = NULL;
#ifdef _WIN32
	searchedWnd = FindWindow(NULL, _title);
#else
/*JFB TODO OSX: tried what follows that in the hope it'll work on OSX but it's KO (http://code.google.com/p/sws-extension/issues/detail?id=175#c83)
	if (GetMainHwnd())
	{
		HWND w = GetWindow(GetMainHwnd(), GW_HWNDFIRST);
		while (w)
		{ 
			if (IsWindowVisible(w) && GetWindow(w, GW_OWNER) == GetMainHwnd())
			{
				char name[BUFFER_SIZE] = "";
				int iLenName = GetWindowText(w, name, BUFFER_SIZE);
				if (!strcmp(name, _title)) {
					searchedWnd = w;
					break;
				}
			}
			w = GetWindow(w, GW_HWNDNEXT);
		}
	}
*/
#endif
	return searchedWnd;
}

HWND GetActionListBox(char* _currentSection, int _sectionMaxSize)
{
	HWND actionsWnd = GetReaWindowByTitle("Actions");
	if (actionsWnd && _currentSection)
	{
		HWND cbSection = GetDlgItem(actionsWnd, 0x525);
		if (cbSection)
			GetWindowText(cbSection, _currentSection, _sectionMaxSize);
	}
	return (actionsWnd ? GetDlgItem(actionsWnd, 0x52B) : NULL);
}


///////////////////////////////////////////////////////////////////////////////
// Routing, env windows - WIN ONLY!
//
// I know... I'm not happy with this "get window by name" solution either.
// Though, floating FX and FX chain window actions are now okay (new dedicated 
// APIs since REAPER v3.41, thanks Cockos!)
///////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32

bool toggleShowHideWin(const char * _title)
{
	HWND w = SearchWindow(_title);
	if (w != NULL)
	{
		ShowWindow(w, IsWindowVisible(w) ? SW_HIDE : SW_SHOW);
		return true;
	}
	return false;
}

bool closeWin(const char * _title)
{
	HWND w = SearchWindow(_title);
	if (w != NULL)
	{
		SendMessage(w, WM_SYSCOMMAND, SC_CLOSE, 0);
		return true;
	}
	return false;
}

void closeOrToggleAllWindows(bool _routing, bool _env, bool _toggle)
{
	for (int i=0; i <= GetNumTracks(); i++)
	{
		MediaTrack *tr = CSurf_TrackFromID(i, false);
		if (tr)
		{
			char trName[128];
			strcpy(trName, GetTrackInfo((int)tr, NULL));

			//  *** Routing ***
			if (_routing)
			{
				char routingName[128];
				if (!i) strcpy(routingName, "Outputs for Master Track");
				else sprintf(routingName, "Routing for track %d \"%s\"", i, trName);

				if (_toggle) toggleShowHideWin(routingName);
				else closeWin(routingName);
			}

			// *** Env ***
			if (_env)
			{
				char envName[128];
				if (!i) strcpy(envName, "Envelopes for Master Track");
				else sprintf(envName, "Envelopes for track %d \"%s\"", i, trName);

				if (_toggle) toggleShowHideWin(envName);
				else closeWin(envName);
			}
		}
	}
}

void closeAllRoutingWindows(COMMAND_T * _ct) {
	closeOrToggleAllWindows(true, false, false);
}

void closeAllEnvWindows(COMMAND_T * _ct) {
	closeOrToggleAllWindows(false, true, false);
}

//JFB: not used anymore
void toggleAllRoutingWindows(COMMAND_T * _ct) {
	closeOrToggleAllWindows(true, false, true);
	fakeToggleAction(_ct);
}

//JFB not used anymore
void toggleAllEnvWindows(COMMAND_T * _ct) {
	closeOrToggleAllWindows(false, true, true);
	fakeToggleAction(_ct);
}

#endif


///////////////////////////////////////////////////////////////////////////////
// FX chain windows: show/hide
// note: Cockos' TrackFX_GetChainVisible() and my getSelectedFX() are not the
// exactly the same. Here, TrackFX_GetChainVisible() is used to get a selected 
// FX in a -visible- chain while getSelectedFX() will always return one.
///////////////////////////////////////////////////////////////////////////////

void showFXChain(COMMAND_T* _ct)
{
	int focusedFX = _ct ? (int)_ct->user : -1; // -1: current selected fx
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false); // include master
		// NULL _ct => all tracks, selected tracks otherwise
		if (tr && (!_ct || (_ct && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))))
			TrackFX_Show(tr, (focusedFX == -1 ? getSelectedFX(tr) : focusedFX), 1);
	}
	// no undo
}

void hideFXChain(COMMAND_T* _ct) 
{
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false); // include master
		// NULL _ct => all tracks, selected tracks otherwise
		if (tr && (!_ct || (_ct && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))))
			TrackFX_Show(tr, getSelectedFX(tr), 0);
	}
	// no undo
}

void toggleFXChain(COMMAND_T* _ct) 
{
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false); // include master
		// NULL _ct => all tracks, selected tracks otherwise
		if (tr && (!_ct || (_ct && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))))
		{
			int currentFX = TrackFX_GetChainVisible(tr);
			TrackFX_Show(tr, getSelectedFX(tr), (currentFX == -2 || currentFX >= 0) ? 0 : 1);
		}
	}
	// no undo

	// fake toggle state update
	if (CountSelectedTracksWithMaster(NULL) > 1)
		fakeToggleAction(_ct);
}

// for toggle state
bool isToggleFXChain(COMMAND_T * _ct) 
{
	int selTrCount = CountSelectedTracksWithMaster(NULL);
	// single track selection: we can return a toggle state
	if (selTrCount == 1)
		return (TrackFX_GetChainVisible(GetFirstSelectedTrackWithMaster(NULL)) != -1);
	// several tracks selected: possible mix of different states 
	// => return a fake toggle state (best effort)
	else if (selTrCount)
		return fakeIsToggledAction(_ct);
	return false;
}

void showAllFXChainsWindows(COMMAND_T* _ct) {
	showFXChain(NULL);
}
void closeAllFXChainsWindows(COMMAND_T * _ct) {
	hideFXChain(NULL);
}
void toggleAllFXChainsWindows(COMMAND_T * _ct) {
	toggleFXChain(NULL);
	fakeToggleAction(_ct);
}


///////////////////////////////////////////////////////////////////////////////
// Floating FX windows: show/hide
///////////////////////////////////////////////////////////////////////////////

// _fx = -1 for selected FX
void toggleFloatFX(MediaTrack* _tr, int _fx)
{
	if (_tr &&  _fx < TrackFX_GetCount(_tr))
	{
		int currenSel = getSelectedFX(_tr); // avoids several parsings
		if (TrackFX_GetFloatingWindow(_tr, (_fx == -1 ? currenSel : _fx)))
			TrackFX_Show(_tr, (_fx == -1 ? currenSel : _fx), 2);
		else
			TrackFX_Show(_tr, (_fx == -1 ? currenSel : _fx), 3);
	}
}

// _all=true: all FXs for all tracks, false: selected tracks + given _fx
// _fx=-1: current selected FX. Ignored when _all == true.
// showflag=0 for toggle, =2 for hide floating window (index valid), =3 for show floating window (index valid)
void floatUnfloatFXs(MediaTrack* _tr, bool _all, int _showFlag, int _fx, bool _selTracks) 
{
	bool matchTrack = (_tr && (!_selTracks || (_selTracks && *(int*)GetSetMediaTrackInfo(_tr, "I_SELECTED", NULL))));
	// all tracks, all FXs
	if (_all && matchTrack)
	{
		int nbFx = TrackFX_GetCount(_tr);
		for (int j=0; j < nbFx; j++)
		{
			if (!_showFlag) toggleFloatFX(_tr, j);
			else TrackFX_Show(_tr, j, _showFlag);
		}
	}
	// given fx for selected tracks
	else if (!_all && matchTrack)
	{
		if (!_showFlag) 
			toggleFloatFX(_tr, (_fx == -1 ? getSelectedFX(_tr) : _fx));
		else 
			//JFB TOTEST: offline
			TrackFX_Show(_tr, (_fx == -1 ? getSelectedFX(_tr) : _fx), _showFlag);
	}
}

// _all: =true for all FXs/tracks, false for selected tracks + for given the given _fx
// _fx = -1, for current selected FX. Ignored when _all == true.
// showflag=0 for toggle, =2 for hide floating window (index valid), =3 for show floating window (index valid)
void floatUnfloatFXs(bool _all, int _showFlag, int _fx, bool _selTracks) 
{
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && (_all || !_selTracks || (_selTracks && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))))
			floatUnfloatFXs(tr, _all, _showFlag, _fx, _selTracks);
	}
}

void floatFX(COMMAND_T* _ct) {
	floatUnfloatFXs(false, 3, (int)_ct->user, true);
}
void unfloatFX(COMMAND_T* _ct) {
	floatUnfloatFXs(false, 2, (int)_ct->user, true);
}
void toggleFloatFX(COMMAND_T* _ct) {
	floatUnfloatFXs(false, 0, (int)_ct->user, true);
	fakeToggleAction(_ct);
}

void showAllFXWindows(COMMAND_T * _ct) {
	floatUnfloatFXs(true, 3, -1, ((int)_ct->user == 1));
}
void closeAllFXWindows(COMMAND_T * _ct) {
	floatUnfloatFXs(true, 2, -1, ((int)_ct->user == 1));
}
void toggleAllFXWindows(COMMAND_T * _ct) {
	floatUnfloatFXs(true, 0, -1, ((int)_ct->user == 1));
	fakeToggleAction(_ct);
}

void closeAllFXWindowsExceptFocused(COMMAND_T * _ct)
{
	HWND w = GetForegroundWindow();
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && IsWindow(w))
		{
			int fxCount = TrackFX_GetCount(tr);
			for (int j = 0; j < fxCount; j++)
			{
				HWND w2 = TrackFX_GetFloatingWindow(tr,j);
				if (!IsWindow(w2) || w != w2)	
					floatUnfloatFXs(tr, false, 2, j, false); // close
			}
		}
	}
}

// returns -1 if none
int getFocusedFX(MediaTrack* _tr, int _dir, int* _firstFound)
{
	int focused = -1;
	if (_firstFound) *_firstFound = -1;
	HWND w = GetForegroundWindow();
	if (_tr && IsWindow(w))
	{
		int fxCount = TrackFX_GetCount(_tr);
		for (int j = (_dir > 0 ? 0 : (fxCount-1)); (j < fxCount) && (j>=0); j+=_dir)
		{
			HWND w2 = TrackFX_GetFloatingWindow(_tr,j);
			if (IsWindow(w2))
			{
				if (_firstFound && *_firstFound < 0) *_firstFound = j;
				if (w == w2)	
				{
					focused = j;
					break;
				}
			}
		}
	}
	return focused;
}

// returns -1 if none
int getFirstFloatingFX(MediaTrack* _tr, int _dir)
{
	if (_tr)
	{
		int fxCount = TrackFX_GetCount(_tr);
		for (int j = (_dir > 0 ? 0 : (fxCount-1)); (j < fxCount) && (j>=0); j+=_dir)
			if (IsWindow(TrackFX_GetFloatingWindow(_tr, j)))
				return j;
	}
	return -1;
}

///////////////////////////////////////////////////////////////////////////////
// Floating FX windows: cycle focus
///////////////////////////////////////////////////////////////////////////////

bool cycleTracksAndFXs(int _trStart, int _fxStart, int _dir, bool _selectedTracks,
     bool (*job)(MediaTrack*,int,bool), bool* _cycled) // see 2 "jobs" below..
{
	int cpt1 = 0;
	int i = _trStart;
	while (cpt1 <= GetNumTracks())
	{
		if (i > GetNumTracks()) 
		{
			i = 0;
			*_cycled = (cpt1 > 0); // ie not the first loop
		}
		else if (i < 0) 
		{
			i = GetNumTracks();
			*_cycled = (cpt1 > 0); // ie not the first loop
		}

		MediaTrack* tr = CSurf_TrackFromID(i, false);
		int fxCount = tr ? TrackFX_GetCount(tr) : 0;
		if (tr && fxCount && 
			(!_selectedTracks || (_selectedTracks && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))))
		{
			int cpt2 = 0;
			int j = ((i == _trStart) ? (_fxStart + _dir) : (_dir < 0 ? (TrackFX_GetCount(tr)-1) : 0));
			while (cpt2 < fxCount)
			{
				// ** check max / min **
				// Single track => fx cycle
				if ((!_selectedTracks && GetNumTracks() == 1) ||
					(_selectedTracks && CountSelectedTracksWithMaster(NULL) == 1))
				{
					*_cycled = (cpt2 > 0); // ie not the first loop
					if (j >= fxCount) j = 0;
					else if (j < 0) j = fxCount-1;
				}
				// multiple tracks case (may => track cycle)
				else if (j >= fxCount || j < 0)
					break; // implies track cycle

				// *** Perform custom stuff ***
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

bool focusJob(MediaTrack* _tr, int _fx, bool _selectedTracks)
{
    HWND w2 = TrackFX_GetFloatingWindow(_tr,_fx);
	if (IsWindow(w2)) {
		SetForegroundWindow(w2);
		return true;
	}
	return false;
}

bool floatOnlyJob(MediaTrack* _tr, int _fx, bool _selectedTracks)
{
	// hide others
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		int fxCount = (tr ? TrackFX_GetCount(tr) : 0);
		if (fxCount && (!_selectedTracks || (_selectedTracks && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))))
		{
			for (int j = 0; j < fxCount; j++)
				if (tr != _tr || j != _fx) 
					floatUnfloatFXs(tr, false, 2, j, true);
		}
	}
	// float ("only")
	floatUnfloatFXs(_tr, false, 3, _fx, true);
	return true;
}

bool cycleFocusFXWnd(int _dir, bool _selectedTracks, bool* _cycled)
{
	if (!_selectedTracks || (_selectedTracks && CountSelectedTracksWithMaster(NULL)))
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
				int focusedPrevious = getFocusedFX(tr, _dir, (firstFXFound < 0 ? &firstFXFound : NULL));
				if (!firstTrFound && firstFXFound >= 0) firstTrFound = tr;
				if (focusedPrevious >= 0 && cycleTracksAndFXs(i, focusedPrevious, _dir, _selectedTracks, focusJob, _cycled))
					return true;
			}
			i += _dir; // +1, -1
		}

		// there was no already focused window if we're here..
		// => focus the 1st found one
		if (firstTrFound) 
			return focusJob(firstTrFound, firstFXFound, _selectedTracks);
	}
	return false;
}


typedef struct {MediaTrack* tr; int fx;} t_TrackFXIds; //JFB TODO: class
WDL_PtrList_DeleteOnDestroy<t_TrackFXIds> g_hiddenFloatingWindows(free);
int g_lastCycleFocusFXDirection = 0; //used for direction change..

void cycleFocusFXMainWnd(int _dir, bool _selectedTracks, bool _showmain) 
{
	bool cycled = false;

	// show back floating FX if needed
	if (_showmain && g_hiddenFloatingWindows.GetSize())
	{
		bool dirChanged = (g_lastCycleFocusFXDirection != _dir);
		for (int i = (dirChanged ? 0 : (g_hiddenFloatingWindows.GetSize()-1)); 
			(i < g_hiddenFloatingWindows.GetSize()) && (i >=0); 
			i += dirChanged ? 1 : -1)
		{
			t_TrackFXIds* hiddenIds = g_hiddenFloatingWindows.Get(i);
			floatUnfloatFXs(hiddenIds->tr, false, 3, hiddenIds->fx, _selectedTracks);
		}
		// .. the focus indirectly restored with last floatUnfloatFXs() call

		g_hiddenFloatingWindows.Empty(true, free);
		return;
	}

	if (cycleFocusFXWnd(_dir, _selectedTracks, &cycled))
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
						if (IsWindow(w))
						{
							// store ids (to show it back later)
							t_TrackFXIds* hiddenIds = (t_TrackFXIds*)malloc(sizeof(t_TrackFXIds));
							hiddenIds->tr = tr;
							hiddenIds->fx = j;
							g_hiddenFloatingWindows.Add(hiddenIds);
							
							// hide it
							floatUnfloatFXs(tr, false, 2, j, _selectedTracks);
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
void cycleFocusFXWndAllTracks(COMMAND_T * _ct) {
	cycleFocusFXMainWnd((int)_ct->user, false, false);
}
void cycleFocusFXWndSelTracks(COMMAND_T * _ct) {
	cycleFocusFXMainWnd((int)_ct->user, true, false);
}
#endif

void cycleFocusFXMainWndAllTracks(COMMAND_T * _ct) {
	cycleFocusFXMainWnd((int)_ct->user, false, true);
}

void cycleFocusFXMainWndSelTracks(COMMAND_T * _ct) {
	cycleFocusFXMainWnd((int)_ct->user, true, true);
}

void cycleFloatFXWndSelTracks(COMMAND_T * _ct)
{
	int dir = (int)_ct->user;
	if (CountSelectedTracksWithMaster(NULL))
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

				int focusedPrevious = getFocusedFX(tr, dir);

				// specific case: make it work even no FX window is focused
				// (e.g. classic pitfall: when the action list is focused, http://forum.cockos.com/showpost.php?p=708536&postcount=305)
				if (focusedPrevious < 0)
					focusedPrevious = getFirstFloatingFX(tr, dir);

				if (focusedPrevious >= 0 && cycleTracksAndFXs(i, focusedPrevious, dir, true, floatOnlyJob, &cycled))
					return;
			}
			i += dir; // +1, -1
		}

		// there was no already focused window if we're here..
		// => float only the 1st found one
		if (firstTrFound) 
			floatOnlyJob(firstTrFound, firstFXFound, true);
	}
}


///////////////////////////////////////////////////////////////////////////////
// Other focus actions
///////////////////////////////////////////////////////////////////////////////

void cycleFocusWnd(COMMAND_T * _ct) 
{
#ifdef _WIN32
	if (GetMainHwnd())
	{
		HWND focusedWnd = GetForegroundWindow();
		HWND w = GetWindow(GetMainHwnd(), GW_HWNDLAST);
		while (w) { 
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

void cycleFocusHideOthersWnd(COMMAND_T * _ct) 
{
#ifdef _WIN32
	g_hwndsCount = 0;
	EnumWindows(EnumReaWindows, 0);
	AddUniqueHnwd(GetMainHwnd(), g_hwnds, &g_hwndsCount);

	// Sort & filter windows
	WDL_PtrList<HWND> hwndList;
	for (int i =0; i < g_hwndsCount && i < MAX_ENUM_HWNDS; i++)
	{
		char buf[512] = "";
		GetWindowText(g_hwnds[i], buf, 512);
		if (strcmp("Item/track info", buf) && // wtf !?
			strcmp("Docker", buf)) // "closed" floating dockers are in fact hidden (and not redrawn => issue)
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

	// Compute window to be displayed
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

void focusMainWindow(COMMAND_T* _ct) {
	SetForegroundWindow(GetMainHwnd()); 
}

void focusMainWindowCloseOthers(COMMAND_T* _ct) 
{
#ifdef _WIN32
	g_hwndsCount = 0;
	EnumWindows(EnumReaWindows, 0); 
	for (int i=0; i < g_hwndsCount && i < MAX_ENUM_HWNDS; i++)
		if (g_hwnds[i] != GetMainHwnd())
			SendMessage(g_hwnds[i], WM_SYSCOMMAND, SC_CLOSE, 0);
#endif
	SetForegroundWindow(GetMainHwnd()); 
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

void ShowThemeHelper(WDL_String* _report, HWND _hwnd, bool _mcp, bool _sel)
{
#ifdef _WIN32
	g_childHwndsCount = 0;
	EnumChildWindows(_hwnd, EnumXCPWindows, 0);
	for (int i=0; i < g_childHwndsCount && i < MAX_ENUM_CHILD_HWNDS; i++)
	{
		HWND w = g_childHwnds[i];
		if (IsWindowVisible(w))
		{
			bool mcpChild = IsChildOf(w, "Mixer", -1);
			if ((_mcp && mcpChild) || (!_mcp && !mcpChild))
			{
				MediaTrack* tr = (MediaTrack*)GetWindowLongPtr(w, GWLP_USERDATA);
				int trIdx = (int)GetSetMediaTrackInfo(tr, "IP_TRACKNUMBER", NULL);
				if (trIdx && (!_sel || (_sel && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))))
				{
					RECT r;	GetClientRect(w, &r);
					_report->AppendFormatted(512, "%s Track #%d '%s': W=%d, H=%d\n", _mcp ? "MCP" : "TCP", trIdx==-1 ? 0 : trIdx, trIdx==-1 ? "[MASTER]" : (char*)GetSetMediaTrackInfo(tr, "P_NAME", NULL), r.right-r.left, r.bottom-r.top);
				}
			}
		}
	}
#endif
} 

void ShowThemeHelper(COMMAND_T* _ct)
{
	WDL_String report("");
	ShowThemeHelper(&report, GetMainHwnd(), false, (int)_ct->user == 1);
	if ((int)_ct->user != 1 && report.GetLength())
		report.Append("\n");

	HWND w = GetReaWindowByTitle("Mixer Master");
	if (w && IsWindowVisible(w)) 
		ShowThemeHelper(&report, w, true, (int)_ct->user == 1);
	
	w = GetReaWindowByTitle("Mixer");
	if (w && IsWindowVisible(w)) 
		ShowThemeHelper(&report, w, true, (int)_ct->user == 1);

	SNM_ShowConsoleMsg(report.Get(), "S&M - Theme Helper");
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
		if (IsWindowVisible(w) && !IsChildOf(w, "Mixer", -1))
			_trList->Add((void*)GetWindowLongPtr(w, GWLP_USERDATA));
	}
#endif
} 
