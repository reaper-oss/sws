/******************************************************************************
/ Macros.cpp
/
/ Copyright (c) 2010 Tim Payne (SWS)
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

// Actions to help with macro writing

#include "stdafx.h"
#include "Macros.h"

#ifdef _WIN32 // Sorry OSX users, win32 only.

void WaitAction(COMMAND_T* t)
{
	int iPlayState = GetPlayState();
	if (iPlayState && !(iPlayState & 2)) // playing/recording, not paused
	{
		int iMeasure;
		double dStart = GetPlayPosition();
		double dBeat = TimeMap2_timeToBeats(NULL, dStart, &iMeasure, NULL, NULL, NULL);
		double dStop;
		switch (t->user)
		{
		case 0: // Bar
			iMeasure++;
			dBeat = 0.0;
			dStop = TimeMap2_beatsToTime(NULL, dBeat, &iMeasure);
			break;
		case 1:
			dBeat = (double)((int)dBeat + 1);
			dStop = TimeMap2_beatsToTime(NULL, dBeat, &iMeasure);
			break;
		case 2:
			double t1, t2;
			GetSet_LoopTimeRange(false, true, &t1, &t2, false);
			if (t1 == t2)
				return;
			dStop = t2;
			break;
		}

		// Check for cursor going past stop, user stopping, and looping around
		while(GetPlayPosition() < dStop && GetPlayState() && GetPlayPosition() >= dStart)
		{
 			// Keep the UI updating
			MSG msg;
			while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
				DispatchMessage(&msg);
			Sleep(1);
		}
	}
}

//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "SWS: Wait for next bar (if playing)" },			"SWS_BARWAIT",		WaitAction, NULL, 0 },
	{ { DEFACCEL, "SWS: Wait for next beat (if playing)" },			"SWS_BEATWAIT",		WaitAction, NULL, 1 },
	{ { DEFACCEL, "SWS: Wait until end of loop (if playing)" },		"SWS_LOOPWAIT",		WaitAction, NULL, 2 },
	
	{ {}, LAST_COMMAND, }, // Denote end of table
};
//!WANT_LOCALIZE_1ST_STRING_END

int MacrosInit()
{
	SWSRegisterCommands(g_commandTable);

	return 1;
}

#endif // #ifdef _WIN32
