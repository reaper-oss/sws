/******************************************************************************
/ BR_Misc.cpp
/
/ Copyright (c) 2013 Dominik Martin Drzic
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
#include "BR_Misc.h"
#include "../Fingers/RprItem.h"
#include "../Fingers/RprMidiTake.h"
#include "../reaper/localize.h"

void SplitItemAtTempo (COMMAND_T* ct)
{
	WDL_TypedBuf<MediaItem*> items;
	SWS_GetSelectedMediaItems(&items);
	if (items.GetSize() == 0 || CountTempoTimeSigMarkers(NULL) == 0)
		return;

	Undo_BeginBlock2(NULL);
	for (int i = 0; i < items.GetSize(); ++i)
	{	
		MediaItem* item = items.Get()[i];
		double iStart = GetMediaItemInfo_Value(item, "D_POSITION");
		double iEnd = iStart + GetMediaItemInfo_Value(item, "D_LENGTH");
		double tPos = iStart - 1;
		
		// Split item currently in the loop
		while (true)
		{
			if ((item = SplitMediaItem(item, tPos)) == 0 )
				item = items.Get()[i];		
			tPos = TimeMap2_GetNextChangeTime(NULL, tPos);
			if (tPos > iEnd || tPos == -1 )
				break;
		}
	}
	Undo_EndBlock2 (NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);
	UpdateArrange();
};

void MarkersAtTempo (COMMAND_T* ct)
{
	if (CountTempoTimeSigMarkers(NULL) == 0)
		return;
	LineParser lp(false);
	TrackEnvelope* envelope = SWS_GetTrackEnvelopeByName(CSurf_TrackFromID(0, false), "Tempo map" );
	char* envState = GetSetObjectState(envelope, "");
	char* token = strtok(envState, "\n");

	if (PreventUIRefresh)
		PreventUIRefresh (1);
	Undo_BeginBlock2(NULL);
	
	while(token != NULL)
	{
		lp.parse(token);
		if (strcmp(lp.gettoken_str(0),  "PT") == 0)
			if (lp.gettoken_int(5) == 1)
				AddProjectMarker(NULL, false, lp.gettoken_float(1), 0, NULL, -1);
		token = strtok(NULL, "\n");	
	}
	
	Undo_EndBlock2 (NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);
	
	if (PreventUIRefresh)
		PreventUIRefresh (-1);
	UpdateTimeline();
	FreeHeapPtr(envState);
};

void MarkersAtNotes (COMMAND_T* ct)
{
	RprItemCtrPtr items = RprItemCollec::getSelected();
	if (items->size() == 0)
		return;
	
	if (PreventUIRefresh)
		PreventUIRefresh (1);	
	Undo_BeginBlock2(NULL);	

	// Check for .MID files and convert them to in project midi
	for(int i = 0; i < items->size(); i++)
		if(!items->getAt(i).getActiveTake().isFile())
			SetMediaItemInfo_Value(items->getAt(i).toReaper(), "B_UISEL", false);
	Main_OnCommand(40684,0);

	// Loop through items and create markers
	for(int i = 0; i < items->size(); i++)
	{	
		RprTake take = items->getAt(i).getActiveTake();
		if(take.isFile() || !take.isMIDI() )
			continue;

		// Get end of item  (to avoid creating markers when item is trimmed)
		MediaItem* item = items->getAt(i).toReaper();
		double iEnd = GetMediaItemInfo_Value(item, "D_POSITION") + GetMediaItemInfo_Value(item, "D_LENGTH");

		// Loop through notes in active take
		RprMidiTake midiTake(take, true);
		for(int j = 0; j < midiTake.countNotes(); j++)
		{
			RprMidiNote *note = midiTake.getNoteAt(j);
			double position = note->getPosition();
			if (position >= iEnd)
				break;
			AddProjectMarker(NULL, false, position, 0, NULL, -1);
		}
	}

	// Convert in project midi back to .MID files
	Main_OnCommand(40685,0);
	for(int i = 0; i < items->size(); i++)
		SetMediaItemInfo_Value(items->getAt(i).toReaper(), "B_UISEL", true);
	
	Undo_EndBlock2 (NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);
	if (PreventUIRefresh)
		PreventUIRefresh (-1);	
	UpdateArrange();
};

void MarkersRegionsAtItemsNameByNotes (COMMAND_T* ct)
{
	if (CountSelectedMediaItems(NULL) < 1)
		return;

	Undo_BeginBlock2(NULL);
	if (PreventUIRefresh)
		PreventUIRefresh (1);	

	for (int i = 0; i < CountSelectedMediaItems(NULL); ++i)
	{
		MediaItem* item =  GetSelectedMediaItem(NULL, i);
		double iStart = *(double*)GetSetMediaItemInfo(item, "D_POSITION", NULL);
		double iEnd = iStart + *(double*)GetSetMediaItemInfo(item, "D_LENGTH", NULL);
		char* pNotes = (char*)GetSetMediaItemInfo(item, "P_NOTES", NULL);

		// Replace all newlines with spaces so it looks nicer
		string notes (pNotes, strlen(pNotes)+1);
		replace(notes.begin(), notes.end(), '\n', ' ');	

		if (ct->user == 0)	// Markers
			AddProjectMarker(NULL, false, iStart, 0, notes.c_str(), -1);
		else				// Regions
			AddProjectMarker(NULL, true, iStart, iEnd, notes.c_str(), -1);
	}

	if (PreventUIRefresh)
		PreventUIRefresh (-1);	
	Undo_EndBlock2 (NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);
}

void CursorToEnv (COMMAND_T* ct)
{
	// Get active envelope
	TrackEnvelope* envelope = GetSelectedTrackEnvelope(NULL);
	char* envState = GetSetObjectState(envelope, "");
	char* token = strtok(envState, "\n");
	bool found = false;

	// MOVE OR MOVE AND SELECT
	if (ct->user > -3 && ct->user < 3)
	{
		double ppTime = 0, pTime = 0, cTime = 0, nTime = 0;

		// Find next point
		if (ct->user > 0 && envelope != 0)
		{
			while(token != NULL)
			{
				if (sscanf(token, "PT %lf", &cTime) == 1)
				{	
					if (cTime > GetCursorPositionEx(NULL))
					{
						found = true;
						token = strtok(NULL, "\n");
						if (sscanf(token, "PT %lf", &nTime) == 0)
							nTime = cTime + 0.001;
						break;
					}
					pTime = cTime;
				}
				token = strtok(NULL, "\n");	
			}
		}

		// Find previous point
		if (ct->user < 0 && envelope != 0)
		{
			while(token != NULL)
			{
				if (sscanf(token, "PT %lf", &cTime) == 1)
				{					
					if (cTime >= GetCursorPositionEx(NULL))
					{
						if (ppTime != cTime)
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
			if (pTime == cTime && nTime == 0 && pTime != 0)
			{
				found = true;
				nTime = cTime;
				cTime = pTime;
				pTime = ppTime;
				if (nTime == 0)
					nTime = cTime + 0.001;
			}
		}
				
		// Yes, this is a hack...however, testing showed it has more advantages (in time of execution when 
		// dealing with a lot of points) over modifying an envelope chunk. With this method there is no need
		// to parse and set the whole chunk but only find a target point (after which parsing stops) and then,
		// via manipulation of the preferences and time selection, make reaper do the rest.
		// Another good thing is that we escape envelope redrawing issues: http://forum.cockos.com/project.php?issueid=4416
		if (found)
		{
			Undo_BeginBlock2(NULL);
			
			// Select point	
			if (ct->user == -2 || ct->user == 2 )
			{
				if (PreventUIRefresh)
					PreventUIRefresh (1);	
					
				// Get current time selection
				double tStart, tEnd, nStart, nEnd;
				GetSet_LoopTimeRange2(NULL, false, false, &tStart, &tEnd, false);
				
				// Enable selection of points in time selection
				int envClickSegMode = *(int*)GetConfigVar("envclicksegmode");
				*(int*)GetConfigVar("envclicksegmode") = 65;
							
				// Set time selection that in turn sets new envelope selection
				nStart = (cTime - pTime) / 2 + pTime;
				nEnd = (nTime - cTime) / 2 + cTime;
				GetSet_LoopTimeRange2(NULL, true, false, &nStart, &nEnd, false);
							
				// Change preferences again so point selection doesn't get lost when a previous time selection gets restored
				*(int*)GetConfigVar("envclicksegmode") = 1;
				GetSet_LoopTimeRange2(NULL, true, false, &tStart, &tEnd, false);
							
				// Restore preferences
				*(int*)GetConfigVar("envclicksegmode") = envClickSegMode;	
				
				if (PreventUIRefresh)
					PreventUIRefresh (-1);	
			}

			// Set edit cursor position
			SetEditCurPos(cTime, true, false);
			Undo_EndBlock2 (NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);		
		}
	}

	// MOVE AND ADD TO SELECTION
	else
	{
		WDL_FastString newState;
		double cTime;
				
		// Find next point
		if (ct->user > 0 && envelope != 0)
		{
			LineParser lp(false);
			while(token != NULL)
			{
				lp.parse(token);
				if (!found && strcmp(lp.gettoken_str(0),  "PT") == 0)
				{	
					if (lp.gettoken_float(1) > GetCursorPositionEx(NULL))
					{
						found = true;
						cTime = lp.gettoken_float(1);
						newState.AppendFormatted(
													256, "PT %.16f %.16f %d %d %d %d %.13f\n",
													lp.gettoken_float(1),
													lp.gettoken_float(2),
													lp.gettoken_int(3),
													lp.gettoken_int(4),
													1,
													lp.gettoken_int(6),
													lp.gettoken_float(7)
												);				
					}
					else
					{
						newState.Append(token);
						newState.Append("\n");
					}
				}
				else
				{
					newState.Append(token);
					newState.Append("\n");
				}				
				token = strtok(NULL, "\n");	
			}
		}
		
		// Find previous point
		if (ct->user < 0 && envelope != 0)
		{
			LineParser lp(false);
			while(token != NULL)
			{	
				lp.parse(token);
				if (!found && strcmp(lp.gettoken_str(0),  "PT") == 0)
				{
					double pTime = lp.gettoken_float(1);
					char *pToken = token;
					token = strtok(NULL, "\n");
					lp.parse(token);
					
					if ((lp.gettoken_float(1) >= GetCursorPositionEx(NULL) && pTime < GetCursorPositionEx(NULL)) || (strcmp(lp.gettoken_str(0),  "PT") == 1 && pTime <= GetCursorPositionEx(NULL)))
					{
						found = true;
						cTime = pTime;
						lp.parse(pToken);
						newState.AppendFormatted(
													256
													, "PT %.16f %.16f %d %d %d %d %.13f\n"
													, lp.gettoken_float(1)
													, lp.gettoken_float(2)
													, lp.gettoken_int(3)
													, lp.gettoken_int(4)
													, 1
													, lp.gettoken_int(6)
													, lp.gettoken_float(7)
												);					
					}
					else
					{
						newState.Append(pToken);
						newState.Append("\n");
					}
				}	
				else
				{
					newState.Append(token);
					newState.Append("\n");
					token = strtok(NULL, "\n");
				}
				
			}
		}

		// Update envelope chunk
		if (found)
		{
			Undo_BeginBlock2(NULL);
			GetSetObjectState(envelope, newState.Get());
			SetEditCurPos(cTime, true, false);
			Undo_EndBlock2 (NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);
		}
	}
	FreeHeapPtr(envState);	
};