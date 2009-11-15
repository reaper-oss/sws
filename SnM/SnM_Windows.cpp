/******************************************************************************
/ SnM_Actions.cpp
/
/ Copyright (c) 2009 Tim Payne (SWS), JF Bédague (S&M)
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

// Jeffos' note: 
// I know... I'm not happy with this "get window by name" solution either.

void toggleShowHideWin(const char * _title)
{
	HWND w = FindWindow(NULL, _title);
	if (w != NULL)
		ShowWindow(w, IsWindowVisible(w) ? SW_HIDE : SW_SHOW);
}

void closeWin(const char * _title)
{
		HWND w = FindWindow(NULL, _title);
		if (w != NULL)
			SendMessage(w, WM_SYSCOMMAND, SC_CLOSE, 0);
}

void closeOrToggleWindows(bool _routing, bool _env, bool _toggle)
{
	for (int i=1; i <= GetNumTracks(); i++)
	{
		MediaTrack *tr = CSurf_TrackFromID(i, false);
		if (tr)
		{
			char trName[128];
			sprintf(trName, "%s", GetTrackInfo((int)tr, NULL));

			//  *** Routing ***
			if (_routing)
			{
				char routingName[128];
				sprintf(routingName, "Routing for track %d \"%s\"", i, trName);

				if (_toggle)
					toggleShowHideWin(routingName);
				else
					closeWin(routingName);
			}

			// *** Env ***
			if (_env)
			{
				char envName[128];
				sprintf(envName, "Envelopes for track %d \"%s\"", i, trName);
				if (_toggle)
					toggleShowHideWin(envName);
				else
					closeWin(envName);
			}
		}
	}
}

void closeRoutingWindows(COMMAND_T * _c)
{
	closeOrToggleWindows(true, false, false);
}

void closeEnvWindows(COMMAND_T * _c)
{
	closeOrToggleWindows(false, true, false);
}

void toggleRoutingWindows(COMMAND_T * _c)
{
	closeOrToggleWindows(true, false, true);
}

void toggleEnvWindows(COMMAND_T * _c)
{
	closeOrToggleWindows(false, true, true);
}

