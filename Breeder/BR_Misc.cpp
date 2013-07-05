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
#include "BR_EnvTools.h"
#include "BR_Util.h"
#include "../Fingers/RprItem.h"
#include "../Fingers/RprMidiTake.h"
#include "../reaper/localize.h"

void SplitItemAtTempo (COMMAND_T* ct)
{
	WDL_TypedBuf<MediaItem*> items;
	SWS_GetSelectedMediaItems(&items);
	if (!items.GetSize() || !CountTempoTimeSigMarkers(NULL))
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
			item = SplitMediaItem(item, tPos);
			if (!item) // in case item did not get split (splitting at nonexistent position)
				item = items.Get()[i];

			tPos = TimeMap2_GetNextChangeTime(NULL, tPos);
			if (tPos > iEnd || tPos == -1 )
				break;
		}
	}
	Undo_EndBlock2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);
	UpdateArrange();
};

void MarkersAtTempo (COMMAND_T* ct)
{
	if (!CountTempoTimeSigMarkers(NULL))
		return;
	LineParser lp(false);
	TrackEnvelope* envelope = SWS_GetTrackEnvelopeByName(CSurf_TrackFromID(0, false), "Tempo map" );
	char* envState = GetSetObjectState(envelope, "");
	char* token = strtok(envState, "\n");

	PreventUIRefresh(1);
	Undo_BeginBlock2(NULL);
	while (token != NULL)
	{
		lp.parse(token);
		if (!strcmp(lp.gettoken_str(0), "PT"))
			if (lp.gettoken_int(5))
				AddProjectMarker(NULL, false, lp.gettoken_float(1), 0, NULL, -1);
		token = strtok(NULL, "\n");	
	}
	Undo_EndBlock2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);
	PreventUIRefresh(-1);
	
	UpdateTimeline();
	FreeHeapPtr(envState);
};

void MidiProjectTempo (COMMAND_T* ct)
{
	bool stateChanged = false;
	for (int i = 0; i < CountSelectedMediaItems(NULL); ++i)
	{
		MediaItem* item = GetSelectedMediaItem(NULL, i);
	
		// Before parsing the whole item chunk, check if it has MIDI takes
		bool midiFound = false;
		for (int i = 0; i < CountTakes(item); ++i)
		{
			char type[64] = {0};
			GetMediaSourceType(GetMediaItemTake_Source(GetMediaItemTake(item, i)), type, sizeof(type));
			if (!strcmp(type, "MIDI") || !strcmp(type, "MIDIPOOL"))
			{
				midiFound = true;
				break;
			}
		}

		if (!midiFound)
			continue;

		// Get item chunk and it's position
		WDL_FastString newState;
		char* chunk = GetSetObjectState(item, "");
		double position = *(double*)GetSetMediaItemInfo(item, "D_POSITION", NULL);
		
		// Find IGNTEMPO lines
		char* token = strtok(chunk, "\n");
		while (token != NULL)
		{
			if (!strncmp(token, "IGNTEMPO ", 9))
			{
				int value, num, den; double bpm;
				if (ct->user == 0)
				{
					value = 1;
					TimeMap_GetTimeSigAtTime(NULL, position, &num, &den, &bpm);
				}
				else
				{
					value = 0;
					sscanf(token, "IGNTEMPO %*d %lf %d %d\n", &bpm, &num, &den);
				}

				newState.AppendFormatted(256, "IGNTEMPO %d %.8f %d %d\n", value, bpm, num, den);
			}
			else
			{
				newState.Append(token);
				newState.Append("\n");
			}
			token = strtok(NULL, "\n");	
		}
		GetSetObjectState(item, newState.Get());
		FreeHeapPtr(chunk);
		stateChanged = true;
	}
	if (stateChanged)
		Undo_OnStateChange2(NULL, SWS_CMD_SHORTNAME(ct));
};

void MarkersAtNotes (COMMAND_T* ct)
{
	RprItemCtrPtr items = RprItemCollec::getSelected();
	if (!items->size())
		return;
	
	PreventUIRefresh(1);	
	Undo_BeginBlock2(NULL);	

	// Check for .MID files and convert them to in project midi
	for (int i = 0; i < items->size(); i++)
		if (!items->getAt(i).getActiveTake().isFile())
			SetMediaItemInfo_Value(items->getAt(i).toReaper(), "B_UISEL", false);
	Main_OnCommand(40684,0);

	// Loop through items and create markers
	for (int i = 0; i < items->size(); i++)
	{	
		RprTake take = items->getAt(i).getActiveTake();
		if (take.isFile() || !take.isMIDI())
			continue;

		// Get end of item (to avoid creating markers when item is trimmed)
		MediaItem* item = items->getAt(i).toReaper();
		double iEnd = GetMediaItemInfo_Value(item, "D_POSITION") + GetMediaItemInfo_Value(item, "D_LENGTH");

		// Loop through notes in active take
		RprMidiTake midiTake(take, true);
		for (int j = 0; j < midiTake.countNotes(); j++)
		{
			double position = midiTake.getNoteAt(j)->getPosition();
			if (position >= iEnd)
				break;
			AddProjectMarker(NULL, false, position, 0, NULL, -1);
		}
	}

	// Convert in project midi back to .MID files
	Main_OnCommand(40685,0);
	for (int i = 0; i < items->size(); i++)
		SetMediaItemInfo_Value(items->getAt(i).toReaper(), "B_UISEL", true);
	
	Undo_EndBlock2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);
	PreventUIRefresh(-1);	
	UpdateArrange();
};

void MarkersRegionsAtItems (COMMAND_T* ct)
{
	if (!CountSelectedMediaItems(NULL))
		return;

	Undo_BeginBlock2(NULL);
	PreventUIRefresh(1);	

	for (int i = 0; i < CountSelectedMediaItems(NULL); ++i)
	{
		MediaItem* item =  GetSelectedMediaItem(NULL, i);
		double iStart = *(double*)GetSetMediaItemInfo(item, "D_POSITION", NULL);
		double iEnd = iStart + *(double*)GetSetMediaItemInfo(item, "D_LENGTH", NULL);
		char* pNotes = (char*)GetSetMediaItemInfo(item, "P_NOTES", NULL);
		
		// Replace newlines with spaces
		string notes(pNotes, strlen(pNotes)+1);
		ReplaceAll(notes, "\r\n", " ");
	
		if (ct->user == 0)	// Markers
			AddProjectMarker(NULL, false, iStart, 0, notes.c_str(), -1);
		else				// Regions
			AddProjectMarker(NULL, true, iStart, iEnd, notes.c_str(), -1);
	}

	PreventUIRefresh(-1);
	UpdateTimeline();
	Undo_EndBlock2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);
};

void CursorToEnv1 (COMMAND_T* ct)
{
	// Get active envelope
	TrackEnvelope* envelope = GetSelectedTrackEnvelope(NULL);
	if (!envelope)
		return;
	char* envState = GetSetObjectState(envelope, "");
	char* token = strtok(envState, "\n");
	
	bool found = false;
	double pTime = 0, cTime = 0, nTime = 0;
	double cursor = GetCursorPositionEx(NULL);
	
	// Find next point
	if (ct->user > 0)
	{
		while (token != NULL)
		{
			if (sscanf(token, "PT %lf", &cTime))
			{	
				if (cTime > cursor)
				{
					found = true;
					token = strtok(NULL, "\n");
					if (!sscanf(token, "PT %lf", &nTime))
						nTime = cTime + 0.001;
					break;
				}
				pTime = cTime;
			}
			token = strtok(NULL, "\n");	
		}
	}

	// Find previous point
	else if (ct->user < 0)
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
		if (!found && cursor > cTime )
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
			PreventUIRefresh(1);	
				
			// Get current time selection
			double tStart, tEnd, nStart, nEnd;
			GetSet_LoopTimeRange2(NULL, false, false, &tStart, &tEnd, false);
			
			// Enable selection of points in time selection
			int envClickSegMode = *(int*)GetConfigVar("envclicksegmode");
			*(int*)GetConfigVar("envclicksegmode") = SetBit(envClickSegMode, 6);
						
			// Set time selection that in turn sets new envelope selection
			nStart = (cTime - pTime) / 2 + pTime;
			nEnd = (nTime - cTime) / 2 + cTime;
			GetSet_LoopTimeRange2(NULL, true, false, &nStart, &nEnd, false);
						
			// Change preferences again so point selection doesn't get lost when a previous time selection gets restored
			*(int*)GetConfigVar("envclicksegmode") = ClearBit(envClickSegMode, 6);
			GetSet_LoopTimeRange2(NULL, true, false, &tStart, &tEnd, false);
						
			// Restore preferences
			*(int*)GetConfigVar("envclicksegmode") = envClickSegMode;	
			
			PreventUIRefresh(-1);	
		}

		// Set edit cursor position
		SetEditCurPos(cTime, true, false);
		Undo_EndBlock2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);		
	}
	FreeHeapPtr(envState);	
};

void CursorToEnv2 (COMMAND_T* ct)
{
	// Get active envelope
	TrackEnvelope* env = GetSelectedTrackEnvelope(NULL);
	if (!env)
		return;
	BR_EnvChunk envelope(env);

	// Find next/previous point
	int id;
	if (ct->user > 0)
		id = envelope.FindNext(GetCursorPositionEx(NULL));		
	else
		id = envelope.FindPrevious(GetCursorPositionEx(NULL));

	// Select and set edit cursor
	double targetPos;
	if (envelope.GetPoint(id, &targetPos, NULL, NULL, NULL))
	{
		// Select all points at the same time position
		double position;
		while (envelope.GetPoint(id, &position, NULL, NULL, NULL))
		{
			if (targetPos == position)
			{
				envelope.SetSelection(id, true);
				if (ct->user > 0) {++id;} else {--id;}
			}
			else
				break;
		}

		Undo_BeginBlock2(NULL);
		SetEditCurPos(targetPos, true, false);
		envelope.Commit();
		Undo_EndBlock2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);
	}
};

void ShowActiveEnvOnly (COMMAND_T* ct)
{
	// Get active envelope (with VIS and ACT already set)
	TrackEnvelope* env = GetSelectedTrackEnvelope(NULL);
	if (!env)
		return;	
	BR_EnvChunk envelope(env, true);
	
	// If dealing with selected tracks only, check if there is selection
	if (ct->user == 1)
		if (!CountSelectedTracks(NULL))
			return;
	
	Undo_BeginBlock2(NULL);

	// If envelope has only one point Reaper will not show it after hiding all envs and committing so create another artifical point
	bool flag = false;
	if (envelope.Count() <= 1)
	{
		flag = envelope.CreatePoint(0, 100, 100, 0, 0, false);
		envelope.Commit(); // Give back edited envelope to reaper before hiding all envelopes
	}

	// Hide all envelopes
	if (ct->user == 0)
		Main_OnCommand(41150, 0); // all
	else
		Main_OnCommand(40889 ,0); // for selected tracks

	// Set back active envelope
	if (flag)
		envelope.DeletePoint(envelope.Count()-1);
	envelope.Commit(true);

	Undo_EndBlock2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);
};