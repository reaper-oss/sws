/******************************************************************************
/ SnM_Windows.cpp
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
#include "SNM_ChunkParserPatcher.h"

typedef struct
{
	MediaTrack* tr;
	int fx;
} t_TrackFXIds;

WDL_PtrList<t_TrackFXIds> g_hiddenFloatingWindows;

#ifdef _WIN32

///////////////////////////////////////////////////////////////////////////////
// Routing, env windows - WIN ONLY!
//
// Jeffos' note: 
// I know... I'm not happy with this "get window by name" solution either.
// Though, floating FX and FX chain windows action are now "clean" (new 
// dedicated APIs since REAPER v3.41, thanks Cockos!)
///////////////////////////////////////////////////////////////////////////////

bool toggleShowHideWin(const char * _title) {
	HWND w = FindWindow(NULL, _title);
	if (w != NULL)
	{
		ShowWindow(w, IsWindowVisible(w) ? SW_HIDE : SW_SHOW);
		return true;
	}
	return false;
}

bool closeWin(const char * _title) {
	HWND w = FindWindow(NULL, _title);
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

void toggleAllRoutingWindows(COMMAND_T * _ct) {
	closeOrToggleAllWindows(true, false, true);
	fakeToggleAction(_ct);
}

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
	if (NumSelTracks() > 1)
		fakeToggleAction(_ct);
}

// for toggle state
bool isToggleFXChain(COMMAND_T * _ct) 
{
	int selTrCount = NumSelTracks();
	// single track selection: we can return a toggle state
	if (selTrCount == 1)
		return (TrackFX_GetChainVisible(GetFirstSelectedTrack()) != -1);
	// several tracks selected: possible mix of different state 
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
			//TODO: offline
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
	floatUnfloatFXs(true, 3, -1, (int)(_ct->user == 1));
}
void closeAllFXWindows(COMMAND_T * _ct) {
	floatUnfloatFXs(true, 2, -1, (int)(_ct->user == 1));
}
void toggleAllFXWindows(COMMAND_T * _ct) {
	floatUnfloatFXs(true, 0, -1, (int)(_ct->user == 1));
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


///////////////////////////////////////////////////////////////////////////////
// Floating FX windows: cycle focus
///////////////////////////////////////////////////////////////////////////////

void flushHiddenFXWindows() {
	g_hiddenFloatingWindows.Empty(true, free);
}

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
					(_selectedTracks && NumSelTracks() == 1))
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
	if (IsWindow(w2))  {
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
	if (!_selectedTracks || (_selectedTracks && NumSelTracks()))
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

int g_lastCycleFocusFXDirection = 0; //used for direction change..
void cycleFocusFXAndMainWnd(int _dir, bool _selectedTracks, bool _showmain) 
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

		flushHiddenFXWindows();
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

void cycleFocusFXWndAllTracks(COMMAND_T * _ct) {
	cycleFocusFXAndMainWnd((int)_ct->user, false, false);
}

void cycleFocusFXWndSelTracks(COMMAND_T * _ct) {
	cycleFocusFXAndMainWnd((int)_ct->user, true, false);
}

void cycleFocusFXAndMainWndAllTracks(COMMAND_T * _ct) {
	cycleFocusFXAndMainWnd((int)_ct->user, false, true);
}

void cycleFocusFXMainWndSelTracks(COMMAND_T * _ct) {
	cycleFocusFXAndMainWnd((int)_ct->user, true, true);
}

void cycleFloatFXWndSelTracks(COMMAND_T * _ct)
{
	int dir = (int)_ct->user;
	if (NumSelTracks())
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
// Misc. window actions
///////////////////////////////////////////////////////////////////////////////

void cycleFocusWnd(COMMAND_T * _ct) 
{
	if (GetMainHwnd())
	{
		HWND focusedWnd = GetForegroundWindow();
		HWND w = GetWindow(GetMainHwnd(), GW_HWNDLAST);
		while (w)
		{ 
			if (IsWindowVisible(w) && 
				GetWindow(w, GW_OWNER) == GetMainHwnd() &&
				focusedWnd != w)
			{
				SetForegroundWindow(w);
				return;
			}
			w = GetWindow(w, GW_HWNDPREV);
		}
	}
}

void setMainWindowActive(COMMAND_T* _ct) {
	SetForegroundWindow(GetMainHwnd()); 
}
