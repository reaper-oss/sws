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
}

void toggleAllEnvWindows(COMMAND_T * _ct) {
	closeOrToggleAllWindows(false, true, true);
}

#endif


///////////////////////////////////////////////////////////////////////////////
// FX chain windows
// note: Cockos' TrackFX_GetChainVisible() and my getSelectedFX() are not the 
//       exactly the same, TrackFX_GetChainVisible() is used to know if the chain 
//       is not necessary to get the current selected FX visible (it allows to
//       popup it if needed
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
}

void showAllFXChainsWindows(COMMAND_T* _ct) {
	showFXChain(NULL);
}
void closeAllFXChainsWindows(COMMAND_T * _ct) {
	hideFXChain(NULL);
}
void toggleAllFXChainsWindows(COMMAND_T * _ct) {
	toggleFXChain(NULL);
}

///////////////////////////////////////////////////////////////////////////////
// FX windows
///////////////////////////////////////////////////////////////////////////////

// _fx = -1, for current selected FX
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

// _all: =true for all FXs/tracks, false for selected tracks + for given the given _fx
// _fx = -1, for current selected FX. Ignored when _all == true.
// showflag=0 for toggle, =2 for hide floating window (index valid), =3 for show floating window (index valid)
void floatUnfloatTrackFXs(MediaTrack* _tr, bool _all, int _showFlag, int _fx, bool _selTracks) 
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
		floatUnfloatTrackFXs(CSurf_TrackFromID(i, false), _all, _showFlag, _fx, _selTracks) ;
}

void floatFX(COMMAND_T* _ct) {
	floatUnfloatFXs(false, 3, _ct->user, true);
}
void unfloatFX(COMMAND_T* _ct) {
	floatUnfloatFXs(false, 2, _ct->user, true);
}
void toggleFloatFX(COMMAND_T* _ct) {
	floatUnfloatFXs(false, 0, _ct->user, true);
}

void showAllFXWindows(COMMAND_T * _ct) {
	floatUnfloatFXs(true, 3, -1, (_ct->user == 1));
}
void closeAllFXWindows(COMMAND_T * _ct) {
	floatUnfloatFXs(true, 2, -1, (_ct->user == 1));
}
void toggleAllFXWindows(COMMAND_T * _ct) {
	floatUnfloatFXs(true, 0, -1, (_ct->user == 1));
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
					floatUnfloatTrackFXs(tr, false, 2, j, false); // close
			}
		}
	}
}

// returns -1 if none
int getFocusedFX(MediaTrack* _tr, int* _firstFound)
{
	int focused = -1;
	if (_firstFound) *_firstFound = -1;
	HWND w = GetForegroundWindow();
	if (_tr && IsWindow(w))
	{
		int fxCount = TrackFX_GetCount(_tr);
		for (int j = 0; j < fxCount; j++)
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

bool cycleTracksAndFXs(int _trStart, int _fxStart, int _dir, bool _selectedTracks,
     bool (*job)(MediaTrack*,int,bool)) // see 2 "jobs" bellow..
{
	int cpt1 = 0;
	int i = _trStart;
	while (cpt1 <= GetNumTracks())
	{
		if (i > GetNumTracks()) i = 0;
		else if (i < 0) i = GetNumTracks();

		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && TrackFX_GetCount(tr) && 
			(!_selectedTracks || (_selectedTracks && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))))
		{
			int cpt2 = 0;
			int j = ((i == _trStart) ? (_fxStart + _dir) : (_dir < 0 ? (TrackFX_GetCount(tr)-1) : 0));
			while (cpt2 < TrackFX_GetCount(tr))
			{
				// ** check max / min **
				// Single track => fx cycle
				if ((!_selectedTracks && GetNumTracks() == 1) ||
					(_selectedTracks && CountSelectedTracks(0) == 1))
				{
					if (j >= TrackFX_GetCount(tr)) j = 0;
					else if (j < 0) j = TrackFX_GetCount(tr)-1;
				}
				// multiple tracks case (may => track cycle)
				else if (j >= TrackFX_GetCount(tr) || j < 0)
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
	// close others ("close" not "hide")
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		int fxCount = (tr ? TrackFX_GetCount(tr) : 0);
		if (fxCount && (!_selectedTracks || (_selectedTracks && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))))
		{
			for (int j = 0; j < fxCount; j++)
				if (tr != _tr || j != _fx) 
					floatUnfloatTrackFXs(tr, false, 2, j, true);
		}
	}
	// float ("only")
	floatUnfloatTrackFXs(_tr, false, 3, _fx, true);
	return true;
}

bool cycleFocusFXWnd(int _dir, bool _selectedTracks)
{
	if (!_selectedTracks || (_selectedTracks && CountSelectedTracks(0)))
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
				int focusedPrevious = getFocusedFX(tr, (firstFXFound < 0 ? &firstFXFound : NULL));
				if (!firstTrFound && firstFXFound >= 0) firstTrFound = tr;
				if (focusedPrevious >= 0 && cycleTracksAndFXs(i, focusedPrevious, _dir, _selectedTracks, focusJob))
					return true;
			}
			i += _dir; // +1, -1
		}

		// there was no already focused window if we're here..
		// => focus the 1st found one
		if (firstTrFound) 
			focusJob(firstTrFound, firstFXFound, _selectedTracks);
	}
	return false;
}

void cycleFocusFXWndAllTracks(COMMAND_T * _ct) {
	cycleFocusFXWnd(_ct->user, false);
}

void cycleFocusFXWndSelTracks(COMMAND_T * _ct) {
	cycleFocusFXWnd(_ct->user, true);
}

void cycleFloatFXWndSelTracks(COMMAND_T * _ct)
{
	int dir = _ct->user;
	if (CountSelectedTracks(0))
	{
		MediaTrack* firstTrFound = NULL;
		int firstFXFound = -1;

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

				int focusedPrevious = getFocusedFX(tr);
				if (focusedPrevious >= 0 && cycleTracksAndFXs(i, focusedPrevious, dir, true, floatOnlyJob))
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

void setMainWindowActive(COMMAND_T* _ct) {
	SetForegroundWindow(GetMainHwnd()); 
}
