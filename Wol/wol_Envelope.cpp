/******************************************************************************
/ wol_Envelope.cpp
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
#include "wol_Envelope.h"
#include "../Breeder/BR_EnvelopeUtil.h"
#include "../Breeder/BR_MouseUtil.h"
#include "../Breeder/BR_Util.h"



// Modified version of CursorToEnv1() in BR_Envelope.cpp
void SelectClosestEnvelopePointMouseCursor(COMMAND_T* ct)
{
	TrackEnvelope* envelope = GetSelectedEnvelope(NULL);
	if (!envelope)
		return;
	char* envState = GetSetObjectState(envelope, "");
	char* token = strtok(envState, "\n");

	bool found = false;
	double pTime = 0, cTime = 0, nTime = 0;
	double cursor = PositionAtMouseCursor(true, false, NULL, NULL);
	double takeEnvStartPos = (envelope == GetSelectedTrackEnvelope(NULL)) ? 0 : GetMediaItemInfo_Value(GetMediaItemTake_Item(GetTakeEnvParent(envelope, NULL)), "D_POSITION");
	cursor -= takeEnvStartPos;

	if ((int)ct->user > 0)
	{
		while (token != NULL)
		{
			if (sscanf(token, "PT %lf", &cTime))
			{
				if (cTime > cursor)
				{
					// fake next point if it doesn't exist
					token = strtok(NULL, "\n");
					if (!sscanf(token, "PT %lf", &nTime))
						nTime = cTime + 0.001;

					found = true;
					break;
				}
				pTime = cTime;
			}
			token = strtok(NULL, "\n");
		}
	}
	else
	{
		double ppTime = -1;
		while (token != NULL)
		{
			if (sscanf(token, "PT %lf", &cTime))
			{
				if (cTime >= cursor)
				{
					if (ppTime != -1)
					{
						found = true;
						nTime = cTime;
						cTime = pTime;
						pTime = ppTime;
						if (nTime == 0)
							nTime = cTime + 0.001;
					}
					break;
				}
				ppTime = pTime;
				pTime = cTime;
			}
			token = strtok(NULL, "\n");
		}

		// Fix for a case when cursor is after the last point
		if (!found && cursor > cTime)
		{
			found = true;
			nTime = cTime;
			cTime = pTime;
			pTime = ppTime;
			if (nTime == 0)
				nTime = cTime + 0.001;
		}
	}

	// Yes, this is a hack...however, it's faster than modifying an envelope
	// chunk. With this method chunk is parsed until target point is found, and
	// then via manipulation of the preferences and time selection, reaper does
	// the rest. Another good thing is that we escape envelope redrawing
	// issues: http://forum.cockos.com/project.php?issueid=4416
	if (found)
	{
		Undo_BeginBlock2(NULL);

		PreventUIRefresh(1);

		// Get current time selection
		double tStart, tEnd, nStart, nEnd;
		GetSet_LoopTimeRange2(NULL, false, false, &tStart, &tEnd, false);

		// Enable selection of points in time selection
		int envClickSegMode; GetConfig("envclicksegmode", envClickSegMode);
		SetConfig("envclicksegmode", SetBit(envClickSegMode, 6));

		// Set time selection that in turn sets new envelope selection
		nStart = (cTime + pTime) / 2 + takeEnvStartPos;
		nEnd = (nTime + cTime) / 2 + takeEnvStartPos;
		GetSet_LoopTimeRange2(NULL, true, false, &nStart, &nEnd, false);

		// Preserve point when previous time selection gets restored
		SetConfig("envclicksegmode", ClearBit(envClickSegMode, 6));
		GetSet_LoopTimeRange2(NULL, true, false, &tStart, &tEnd, false);

		// Restore preferences
		SetConfig("envclicksegmode", envClickSegMode);

		PreventUIRefresh(-1);

		if ((int)ct->user == -2 || (int)ct->user == 2)
			SetEditCurPos(cTime + takeEnvStartPos, true, false);
		Undo_EndBlock2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);
	}
	FreeHeapPtr(envState);
}