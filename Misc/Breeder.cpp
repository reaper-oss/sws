/******************************************************************************
/ Breeder.cpp
/
/ Copyright (c) 2010 Dominik Martin Drzic
/ http://forum.cockos.com/member.php?u=27094
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
#include "../reaper/localize.h"
#include "Breeder.h"


void BRMoveEditNextTempo (COMMAND_T*)
{
	double ePos = GetCursorPosition();
	double tPos = TimeMap2_GetNextChangeTime(NULL, ePos);
	
	if (tPos!=-1)
	SetEditCurPos (tPos, NULL, NULL);
};

void BRMoveEditPrevTempo (COMMAND_T*)
{
	double ePos = GetCursorPosition();
	int tCount = CountTempoTimeSigMarkers(0) - 1;
	
	while (tCount != -1)
	{
		double tPos;
		GetTempoTimeSigMarker(NULL, tCount, &tPos, 0, 0, 0, 0, 0, 0);
		
		if (tPos < ePos)
		{
			SetEditCurPos (tPos, NULL, NULL);
			break;
		}
		tCount--;
	}
};




//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] = 
{
	// Tempo markers
	{ { DEFACCEL, "SWS/BR: Move edit cursor to next tempo marker" },													"SWS_BRMOVEEDITNEXTTEMPO",				BRMoveEditNextTempo, },
	{ { DEFACCEL, "SWS/BR: Move edit cursor to previous tempo marker" },												"SWS_BRMOVEEDITPREVTEMPO",				BRMoveEditPrevTempo, },
	{ {}, LAST_COMMAND, }, // Denote end of table
};

//!WANT_LOCALIZE_1ST_STRING_END



int BreederInit()
{
	SWSRegisterCommands(g_commandTable);
	return 1;
};
