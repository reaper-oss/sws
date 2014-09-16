/******************************************************************************
/ wol_Misc.cpp
/
/ Copyright (c) 2014 wol
/ http://forum.cockos.com/member.php?u=70153
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
#include "wol_Misc.h"

//---------//

void MoveEditCursorToBeatN(COMMAND_T* ct)
{
	int beat = (int)ct->user - 1;
	int meas = 0, cml = 0;
	TimeMap2_timeToBeats(NULL, GetCursorPosition(), &meas, &cml, 0, 0);
	if (beat >= cml)
		beat = cml - 1;
	SetEditCurPos(TimeMap2_beatsToTime(0, (double)beat, &meas), 1, 0);
	//Undo_OnStateChange2(NULL, SWS_CMD_SHORTNAME(ct));
}

void MoveEditCursorTo(COMMAND_T* ct)
{
	if ((int)ct->user == 0)
	{
		int meas = 0;
		SetEditCurPos(TimeMap2_beatsToTime(NULL, (double)(int)(TimeMap2_timeToBeats(NULL, GetCursorPosition(), &meas, NULL, NULL, NULL) + 0.5), &meas), true, false);
	}
	else if ((int)ct->user > 0)
	{
		if ((int)ct->user == 3 && GetToggleCommandState(40370) == 1)
			ApplyNudge(NULL, 2, 6, 18, 1.0f, false, 0);
		else
		{
			int meas = 0, cml = 0;
			int beat = (int)TimeMap2_timeToBeats(NULL, GetCursorPosition(), &meas, &cml, NULL, NULL);
			if (beat == cml - 1)
			{
				if ((int)ct->user > 1)
					++meas;
				SetEditCurPos(TimeMap2_beatsToTime(NULL, 0.0f, &meas), true, false);
			}
			else
				SetEditCurPos(TimeMap2_beatsToTime(NULL, (double)(beat + 1), &meas), true, false);
		}
	}
	else
	{
		if ((int)ct->user == -3 && GetToggleCommandState(40370) == 1)
			ApplyNudge(NULL, 2, 6, 18, 1.0f, true, 0);
		else
		{
			int meas = 0, cml = 0;
			int beat = (int)TimeMap2_timeToBeats(NULL, GetCursorPosition(), &meas, &cml, NULL, NULL);
			if (beat == 0)
			{
				if ((int)ct->user < -1)
					--meas;
				SetEditCurPos(TimeMap2_beatsToTime(NULL, (double)(cml - 1), &meas), true, false);
			}
			else
				SetEditCurPos(TimeMap2_beatsToTime(NULL, (double)(beat - 1), &meas), true, false);
		}
	}
	//Undo_OnStateChange2(NULL, SWS_CMD_SHORTNAME(ct));
}

//---------//

void SelectAllTracksExceptFolderParents(COMMAND_T* ct)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		GetSetMediaTrackInfo(tr, "I_SELECTED", *(int*)GetSetMediaTrackInfo(tr, "I_FOLDERDEPTH", NULL) == 1 ? &g_i0 : &g_i1);
	}
	Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
}