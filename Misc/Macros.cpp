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

// WaitUntil is not implemented for swell-generic (sws_util_generic.cpp)
#if defined(_WIN32) || defined(__APPLE__)
#  define HAS_WAITACTION

void WaitAction(COMMAND_T* t)
{
	const int iPlayState = GetPlayState();
	if (!iPlayState || (iPlayState & 2)) // playing/recording, not paused
		return;

	struct WaitData { double dStart, dStop; };
	WaitData wd;
	wd.dStart = GetPlayPosition();
	int iMeasure;
	double dBeat = TimeMap2_timeToBeats(NULL, wd.dStart, &iMeasure, NULL, NULL, NULL);
	switch (t->user)
	{
	case 0: // Bar
		iMeasure++;
		dBeat = 0.0;
		wd.dStop = TimeMap2_beatsToTime(NULL, dBeat, &iMeasure);
		break;
	case 1:
		dBeat = (double)((int)dBeat + 1);
		wd.dStop = TimeMap2_beatsToTime(NULL, dBeat, &iMeasure);
		break;
	case 2:
		double t1, t2;
		GetSet_LoopTimeRange(false, true, &t1, &t2, false);
		if (t1 == t2)
			return;
		wd.dStop = t2;
		break;
	}

	// Check for cursor going past stop, user stopping, and looping around
	WaitUntil([](void *data) {
		WaitData *wd = static_cast<WaitData *>(data);
		const double pos = GetPlayPosition();
		return !GetPlayState() || pos >= wd->dStop || pos < wd->dStart;
	}, &wd);
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
#endif

int MacrosInit()
{
#ifdef HAS_WAITACTION
	SWSRegisterCommands(g_commandTable);
#endif

	return 1;
}
