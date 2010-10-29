/******************************************************************************
/ Adam.cpp
/
/ Copyright (c) 2010 Adam Wathan
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
#include "Adam.h"

void AWFillGapsAdv(COMMAND_T* t)
{
	// Set up dialog info
	const char names[] = "Trigger Pad (ms),Crossfade Length (ms),Maximum Gap (ms),Maximum Stretch (0.5 is double),Preserve Transient (ms),Transient Crossfade Length (ms),Fade Shape (0 = linear),Mark possible artifacts? (0/1)";
	static char defaultValues[100] = "5,5,15,0.5,35,5,0,1";
	char retVals[100];
	char defValsBackup[100];
	int maxReturnLen = 100;
	int nitems = 8;
	bool runScript = 0;
	
	// Copy defaultValues into returnedValues (retains values in case user hits cancel)
	strcpy(retVals, defaultValues);
	strcpy(defValsBackup, defaultValues);
	
	// If 0 is passed, call dialog and get values, if 1 is passed, just use last settings
	if (t->user == 0)
	{
		// Call dialog, use retVals so defaultValues doesn't get miffed up
		runScript = GetUserInputs("Advanced Item Smoothing",nitems,names,retVals,maxReturnLen);
	}
	else if (t->user == 1)
	{
		runScript = 1;
	}
	
	// If user hit okay, get the parameters they entered
	if (runScript)	
	{
		// If user didn't hit cancel, copy the returned values back into the default values before ruining retVals with evil strtok
		strcpy(defaultValues, retVals);
		
		// Divided by 1000 to convert to milliseconds except maxStretch
		double triggerPad = (atof(strtok(retVals, ",")))/1000;
		double fadeLength = (atof(strtok(NULL, ",")))/1000;
		double maxGap = (atof(strtok(NULL, ",")))/1000;
		double maxStretch = atof(strtok(NULL, ","));
		double presTrans = (atof(strtok(NULL, ",")))/1000;
		double transFade = (atof(strtok(NULL, ",")))/1000;
		int fadeShape = atoi(strtok(NULL, ","));
		int markErrors = atoi(strtok(NULL, ","));
	
		if ((triggerPad < 0) || (fadeLength < 0) || (maxGap < 0) || (maxStretch < 0) || (maxStretch > 1) || (presTrans < 0) || (transFade < 0) || (fadeShape < 0) || (fadeShape > 5))
		{
			strcpy(defaultValues, defValsBackup);
			
			//ShowMessageBox("Don't use such stupid values, try again.","Invalid Input",0);
			MessageBox(g_hwndParent, "All values must be non-negative, Maximum Stretch must be a value from 0 to 1 and Fade Shape must be a value from 0 to 5.", "Item Smoothing Input Error", MB_OK);
			return;
		}
		
		
		
		// Run loop for every track in project
		for (int trackIndex = 0; trackIndex < GetNumTracks(); trackIndex++)
		{
			
			// Gets current track in loop
			MediaTrack* track = GetTrack(0, trackIndex);
			
			// Run loop for every item on track
			
			int itemCount = GetTrackNumMediaItems(track);
			
			for (int itemIndex = 0; itemIndex < (itemCount - 1); itemIndex++)
			{
				
				MediaItem* item1 = GetTrackMediaItem(track, itemIndex);
				MediaItem* item2 = GetTrackMediaItem(track, itemIndex + 1);
				// errorFlag = 0;
				
				// BEGIN TIMESTRETCH CODE---------------------------------------------------------------
				
				// If stretching is enabled
				if (maxStretch < 1)
				{
				
					// Only stretch if both items are selected, avoids stretching unselected items or the last selected item
					if (GetMediaItemInfo_Value(item1, "B_UISEL") && GetMediaItemInfo_Value(item2, "B_UISEL"))
					{
						
						// Get various pieces of info for the 2 items
						double item1Start = GetMediaItemInfo_Value(item1, "D_POSITION");
						double item1Length = GetMediaItemInfo_Value(item1, "D_LENGTH");
						double item1End = item1Start + item1Length;
				
						double item2Start = GetMediaItemInfo_Value(item2, "D_POSITION");
				
						// Calculate gap between items
						double gap = item2Start - item1End;
						
						// If gap is bigger than the max allowable gap
						if (gap > maxGap)
						{
							
							// If preserve transient is enabled, split item at preserve point
							if (presTrans > 0)
							{
							
								// Get snap offset for item1
								double item1SnapOffset = GetMediaItemInfo_Value(item1, "D_SNAPOFFSET");
								
								// Calculate position of item1 transient
								double item1TransPos = item1Start + item1SnapOffset;
								
								// Split point is (presTrans) after the transient point
								// Subtract fade length because easier to change item end then item start
								// (don't need take loop to adjust all start offsets)
								double splitPoint = item1TransPos + presTrans - transFade;
								
								// Check for default item fades
								double defItemFades = *(double*)(GetConfigVar("deffadelen"));
								bool fadeFlag = 0;
								
								if (defItemFades > 0)
								{
									fadeFlag = 1;
									Main_OnCommand(41194,0);
								}
								
								// Split item1 at the split point
								MediaItem* item1B = SplitMediaItem(item1, splitPoint);
								
								// Revert item fades
								if (fadeFlag)
								{
									Main_OnCommand(41194,0);
								}
								
								// Get new item1 length after split
								item1Length = GetMediaItemInfo_Value(item1, "D_LENGTH") + transFade;
								
								// Create overlap of 'transFade'
								SetMediaItemInfo_Value(item1, "D_LENGTH", item1Length);
								
								// Crossfade the overlap
								SetMediaItemInfo_Value(item1, "D_FADEOUTLEN_AUTO", transFade);
								SetMediaItemInfo_Value(item1B, "D_FADEINLEN_AUTO", transFade);
								
								// Set Fade Shapes
								SetMediaItemInfo_Value(item1, "C_FADEOUTSHAPE", fadeShape);
								SetMediaItemInfo_Value(item1B, "C_FADEINSHAPE", fadeShape);
								
								// Set the stretched half to be item 1 so loop continues properly
								item1 = item1B;
								
								// Get item1 info since item1 is now item1B
								item1Start = GetMediaItemInfo_Value(item1, "D_POSITION");
								item1Length = GetMediaItemInfo_Value(item1, "D_LENGTH");
								item1End = item1Start + item1Length;
								
								// Increase itemIndex and itemCount since split added item
								itemIndex += 1;
								itemCount += 1;
							}
							
							// Calculate distance between item1Start and the beginning of the maximum allowable gap
							double item1StartToGap = (item2Start - maxGap) - item1Start;
							
							// Calculate amount to stretch
							double stretchPercent = (item1Length/(item1StartToGap));
							
							if (stretchPercent < maxStretch)
							{
								stretchPercent = maxStretch;
								
							}
							
							item1Length *= (1/stretchPercent);
							
							SetMediaItemInfo_Value(item1, "D_LENGTH", item1Length);	
							
							for (int takeIndex = 0; takeIndex < GetMediaItemNumTakes(item1); takeIndex++)
							{
								MediaItem_Take* currentTake = GetMediaItemTake(item1, takeIndex);
								SetMediaItemTakeInfo_Value(currentTake, "D_PLAYRATE", stretchPercent);
							}
						}	
					}	
				}
				
				// END TIME STRETCH CODE ---------------------------------------------------------------
			
				// BEGIN FIX OVERLAP CODE --------------------------------------------------------------
				
				// If first item is selected selected, run this loop
				// if (GetMediaItemInfo_Value(item1, "B_UISEL"))
				if (GetMediaItemInfo_Value(item1, "B_UISEL"))
				{
					// Get various pieces of info for the 2 items
					double item1Start = GetMediaItemInfo_Value(item1, "D_POSITION");
					double item1Length = GetMediaItemInfo_Value(item1, "D_LENGTH");
					double item1End = item1Start + item1Length;
				
					double item2Start = GetMediaItemInfo_Value(item2, "D_POSITION");
				
					// If the first item overlaps the second item, trim the first item
					if (item1End > (item2Start - triggerPad))
					{
						
						// If both items selected, account for trigger pad, if not, just trim to item 2 start
						if (GetMediaItemInfo_Value(item1, "B_UISEL") && GetMediaItemInfo_Value(item2, "B_UISEL"))
						{
							item1Length = item2Start - item1Start - triggerPad;
						}
						else
						{
							item1Length = item2Start - item1Start;
						}
						
						SetMediaItemInfo_Value(item1, "D_LENGTH", item1Length);
					}
					
					if (item1End <= (item2Start - triggerPad))
					{
						// If both items selected, account for trigger pad, if not, do nothing
						if (GetMediaItemInfo_Value(item1, "B_UISEL") && GetMediaItemInfo_Value(item2, "B_UISEL"))
						{
							item1Length -= triggerPad;
							SetMediaItemInfo_Value(item1, "D_LENGTH", item1Length);
						}
					}
				}
				
				// END FIX OVERLAP CODE ----------------------------------------------------------------
				
				// BEGIN FILL GAPS CODE ----------------------------------------------------------------
				
				// If both items selected, run this loop
				if (GetMediaItemInfo_Value(item1, "B_UISEL") && GetMediaItemInfo_Value(item2, "B_UISEL"))
				{ 
				
					double item1Start = GetMediaItemInfo_Value(item1, "D_POSITION");
					double item1Length = GetMediaItemInfo_Value(item1, "D_LENGTH");
					double item1End = item1Start + item1Length;
				
					double item2Start = GetMediaItemInfo_Value(item2, "D_POSITION");
					double item2Length = GetMediaItemInfo_Value(item2, "D_LENGTH");
					double item2SnapOffset = GetMediaItemInfo_Value(item2, "D_SNAPOFFSET");
					
					
			
					// If there is a gap, fill it and also add an overlap to the left that is "fadeLength" long
					if (item2Start >= item1End)
					{
						double item2StartDiff = item2Start - item1End + fadeLength;
						item2Length += item2StartDiff;
						
						// Calculate gap between items
						double gap = item2Start - item1End;
						
						// Check to see if gap is bigger than maxGap, even after time stretching
						// Make sure to account for triggerPad since it is subtracted before this point and shouldn't count towards the gap
						if (gap > (maxGap + triggerPad))
						{
							// If gap is big and mark errors is enabled, add a marker
							if (markErrors == 1)
							{
							AddProjectMarker(NULL, false, item1End, NULL, "Possible Artifact", NULL);
							}
							
						}
							
						
						// Adjust start offset for all takes in item2 to account for fill
						for (int takeIndex = 0; takeIndex < GetMediaItemNumTakes(item2); takeIndex++)
							{
								MediaItem_Take* currentTake = GetMediaItemTake(item2, takeIndex);
								double startOffset = GetMediaItemTakeInfo_Value(currentTake, "D_STARTOFFS");
								startOffset -= item2StartDiff;
								SetMediaItemTakeInfo_Value(currentTake, "D_STARTOFFS", startOffset);
							}
							
						// Finally trim the item to fill the gap and adjust the snap offset
						SetMediaItemInfo_Value(item2, "D_POSITION", (item1End - fadeLength));
						SetMediaItemInfo_Value(item2, "D_LENGTH", item2Length);
						SetMediaItemInfo_Value(item2, "D_SNAPOFFSET", (item2SnapOffset + item2StartDiff));
						
						// Crossfade the overlap between the two items
						SetMediaItemInfo_Value(item1, "D_FADEOUTLEN_AUTO", fadeLength);
						SetMediaItemInfo_Value(item2, "D_FADEINLEN_AUTO", fadeLength);
						
						// Set Fade Shapes
						SetMediaItemInfo_Value(item1, "C_FADEOUTSHAPE", fadeShape);
						SetMediaItemInfo_Value(item2, "C_FADEINSHAPE", fadeShape);
					}
				}
				
				// END FILL GAPS CODE ------------------------------------------------------------------
				
			}
			
		}
	}
	
	
	UpdateTimeline();
	Undo_OnStateChangeEx(SWSAW_CMD_SHORTNAME(t), UNDO_STATE_ITEMS | UNDO_STATE_MISCCFG, -1);
}


void AWFillGapsQuick(COMMAND_T* t)
{
		
	
	// Run loop for every track in project
	for (int trackIndex = 0; trackIndex < GetNumTracks(); trackIndex++)
	{
		
		// Gets current track in loop
		MediaTrack* track = GetTrack(0, trackIndex);
		
		// Run loop for every item on track
		
		int itemCount = GetTrackNumMediaItems(track);
		
		for (int itemIndex = 0; itemIndex < (itemCount - 1); itemIndex++)
		{
			
			MediaItem* item1 = GetTrackMediaItem(track, itemIndex);
			MediaItem* item2 = GetTrackMediaItem(track, itemIndex + 1);
			
		
			// BEGIN FIX OVERLAP CODE --------------------------------------------------------------
			
			// If first item is selected, run this loop
			if (GetMediaItemInfo_Value(item1, "B_UISEL"))
			{
				// Get various pieces of info for the 2 items
				double item1Start = GetMediaItemInfo_Value(item1, "D_POSITION");
				double item1Length = GetMediaItemInfo_Value(item1, "D_LENGTH");
				double item1End = item1Start + item1Length;
			
				double item2Start = GetMediaItemInfo_Value(item2, "D_POSITION");
			
				// If the first item overlaps the second item, trim the first item
				if (item1End > (item2Start))
				{
					item1Length = item2Start - item1Start;
										
					SetMediaItemInfo_Value(item1, "D_LENGTH", item1Length);
				}
				
			}
			
			// END FIX OVERLAP CODE ----------------------------------------------------------------
			
			// BEGIN FILL GAPS CODE ----------------------------------------------------------------
			
			// If both items selected, run this loop
			if (GetMediaItemInfo_Value(item1, "B_UISEL") && GetMediaItemInfo_Value(item2, "B_UISEL"))
			{ 
			
				double item1Start = GetMediaItemInfo_Value(item1, "D_POSITION");
				double item1Length = GetMediaItemInfo_Value(item1, "D_LENGTH");
				double item1End = item1Start + item1Length;
			
				double item2Start = GetMediaItemInfo_Value(item2, "D_POSITION");
				double item2Length = GetMediaItemInfo_Value(item2, "D_LENGTH");
				double item2SnapOffset = GetMediaItemInfo_Value(item2, "D_SNAPOFFSET");
		
				// If there is a gap, fill it and also add an overlap to the left that is "fadeLength" long
				if (item2Start >= item1End)
				{
					double item2StartDiff = item2Start - item1End;
					item2Length += item2StartDiff;
					
					// Adjust start offset for all takes in item2 to account for fill
					for (int takeIndex = 0; takeIndex < GetMediaItemNumTakes(item2); takeIndex++)
						{
							MediaItem_Take* currentTake = GetMediaItemTake(item2, takeIndex);
							double startOffset = GetMediaItemTakeInfo_Value(currentTake, "D_STARTOFFS");
							startOffset -= item2StartDiff;
							SetMediaItemTakeInfo_Value(currentTake, "D_STARTOFFS", startOffset);
						}
						
					// Finally trim the item to fill the gap and adjust the snap offset
					SetMediaItemInfo_Value(item2, "D_POSITION", (item1End));
					SetMediaItemInfo_Value(item2, "D_LENGTH", item2Length);
					SetMediaItemInfo_Value(item2, "D_SNAPOFFSET", (item2SnapOffset + item2StartDiff));
					
				}
			}
			
			// END FILL GAPS CODE ------------------------------------------------------------------
			
		}
		
	}

	
	
	UpdateTimeline();
	Undo_OnStateChangeEx(SWSAW_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
}

void AWFillGapsQuickXFade(COMMAND_T* t)
{
	double fadeLength = fabs(*(double*)GetConfigVar("deffadelen"));
	
	// Run loop for every track in project
	for (int trackIndex = 0; trackIndex < GetNumTracks(); trackIndex++)
	{
		
		// Gets current track in loop
		MediaTrack* track = GetTrack(0, trackIndex);
		
		// Run loop for every item on track
		
		int itemCount = GetTrackNumMediaItems(track);
		
		for (int itemIndex = 0; itemIndex < (itemCount - 1); itemIndex++)
		{
			
			MediaItem* item1 = GetTrackMediaItem(track, itemIndex);
			MediaItem* item2 = GetTrackMediaItem(track, itemIndex + 1);
			
			
			// BEGIN FIX OVERLAP CODE --------------------------------------------------------------
			
			// If first item is selected, run this loop
			if (GetMediaItemInfo_Value(item1, "B_UISEL"))
			{
				// Get various pieces of info for the 2 items
				double item1Start = GetMediaItemInfo_Value(item1, "D_POSITION");
				double item1Length = GetMediaItemInfo_Value(item1, "D_LENGTH");
				double item1End = item1Start + item1Length;
				
				double item2Start = GetMediaItemInfo_Value(item2, "D_POSITION");
				
				// If the first item overlaps the second item, trim the first item
				if (item1End > (item2Start))
				{
					
					item1Length = item2Start - item1Start;
										
					SetMediaItemInfo_Value(item1, "D_LENGTH", item1Length);
				}
			}
			
			// END FIX OVERLAP CODE ----------------------------------------------------------------
			
			// BEGIN FILL GAPS CODE ----------------------------------------------------------------
			
			// If both items selected, run this loop
			if (GetMediaItemInfo_Value(item1, "B_UISEL") && GetMediaItemInfo_Value(item2, "B_UISEL"))
			{ 
				
				double item1Start = GetMediaItemInfo_Value(item1, "D_POSITION");
				double item1Length = GetMediaItemInfo_Value(item1, "D_LENGTH");
				double item1End = item1Start + item1Length;
				
				double item2Start = GetMediaItemInfo_Value(item2, "D_POSITION");
				double item2Length = GetMediaItemInfo_Value(item2, "D_LENGTH");
				double item2SnapOffset = GetMediaItemInfo_Value(item2, "D_SNAPOFFSET");
				
				// If there is a gap, fill it and also add an overlap to the left that is "fadeLength" long
				if (item2Start >= item1End)
				{
					double item2StartDiff = item2Start - item1End + fadeLength;
					item2Length += item2StartDiff;
					
					// Adjust start offset for all takes in item2 to account for fill
					for (int takeIndex = 0; takeIndex < GetMediaItemNumTakes(item2); takeIndex++)
					{
						MediaItem_Take* currentTake = GetMediaItemTake(item2, takeIndex);
						double startOffset = GetMediaItemTakeInfo_Value(currentTake, "D_STARTOFFS");
						startOffset -= item2StartDiff;
						SetMediaItemTakeInfo_Value(currentTake, "D_STARTOFFS", startOffset);
					}
					
					// Finally trim the item to fill the gap and adjust the snap offset
					SetMediaItemInfo_Value(item2, "D_POSITION", (item1End - fadeLength));
					SetMediaItemInfo_Value(item2, "D_LENGTH", item2Length);
					SetMediaItemInfo_Value(item2, "D_SNAPOFFSET", (item2SnapOffset + item2StartDiff));
					
					// Crossfade the overlap between the two items
					SetMediaItemInfo_Value(item1, "D_FADEOUTLEN_AUTO", fadeLength);
					SetMediaItemInfo_Value(item2, "D_FADEINLEN_AUTO", fadeLength);
					
				}
			}
			
			// END FILL GAPS CODE ------------------------------------------------------------------
			
		}
		
	}
	
	
	
	UpdateTimeline();
	Undo_OnStateChangeEx(SWSAW_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
}


void AWFixOverlaps(COMMAND_T* t)
{
	
	// Run loop for every track in project
	for (int trackIndex = 0; trackIndex < GetNumTracks(); trackIndex++)
	{
		
		// Gets current track in loop
		MediaTrack* track = GetTrack(0, trackIndex);
		
		// Run loop for every item on track
		
		int itemCount = GetTrackNumMediaItems(track);
		
		for (int itemIndex = 0; itemIndex < (itemCount - 1); itemIndex++)
		{
			
			MediaItem* item1 = GetTrackMediaItem(track, itemIndex);
			MediaItem* item2 = GetTrackMediaItem(track, itemIndex + 1);
			
			
			// BEGIN FIX OVERLAP CODE --------------------------------------------------------------
			
			// If first item is selected, run this loop
			if (GetMediaItemInfo_Value(item1, "B_UISEL"))
			{
				// Get various pieces of info for the 2 items
				double item1Start = GetMediaItemInfo_Value(item1, "D_POSITION");
				double item1Length = GetMediaItemInfo_Value(item1, "D_LENGTH");
				double item1End = item1Start + item1Length;
				
				double item2Start = GetMediaItemInfo_Value(item2, "D_POSITION");
				
				// If the first item overlaps the second item, trim the first item
				if (item1End > (item2Start))
				{
					
					item1Length = item2Start - item1Start;
										
					SetMediaItemInfo_Value(item1, "D_LENGTH", item1Length);
				}
			
			}
			
			// END FIX OVERLAP CODE ----------------------------------------------------------------
		}
		
	}
	
	
	
	UpdateTimeline();
	Undo_OnStateChangeEx(SWSAW_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
}








void AWRecordConditional(COMMAND_T* t)
{
	double t1, t2;
	GetSet_LoopTimeRange(false, false, &t1, &t2, false);
	
	if (t1 != t2)
	{
		Main_OnCommand(40076, 0); //Set record mode to time selection auto punch
	}
	
	else 
	{
		Main_OnCommand(40252, 0); //Set record mode to time selection auto punch
	}
	
	Main_OnCommand(1013,0); // Transport: Record
}


void AWPlayStopAutoGroup(COMMAND_T* t)
{
	if (GetPlayState() & 4)
	{
		Main_OnCommand(1016, 0); //If recording, Stop
		int numItems = CountSelectedMediaItems(0);
	
		if (numItems > 1)
		{
			Main_OnCommand(40032, 0); //Group selected items
		}
	}
	else
	{
		Main_OnCommand(40044, 0); //If Transport is playing or stopped, Play/Stop
	}
	
	UpdateTimeline();
	Undo_OnStateChangeEx(SWSAW_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
}

void AWSelectToEnd(COMMAND_T* t)
{
	double projEnd = 0;
	
	// Find end position of last item in project
	
	for (int trackIndex = 0; trackIndex < GetNumTracks(); trackIndex++)
	{
		MediaTrack* track = GetTrack(0, trackIndex);
		
		int itemNum = GetTrackNumMediaItems(track) - 1;
		
		MediaItem* item = GetTrackMediaItem(track, itemNum);	
		
		double itemEnd = GetMediaItemInfo_Value(item, "D_POSITION") + GetMediaItemInfo_Value(item, "D_LENGTH");
		
		if (itemEnd > projEnd)
		{
			projEnd = itemEnd;
		}
	}
	
	double selStart = GetCursorPosition();
	
	GetSet_LoopTimeRange2(0, 1, 0, &selStart, &projEnd, 0);
	
	Main_OnCommand(40718, 0); // Select all items in time selection on selected tracks
	
	UpdateTimeline();
	Undo_OnStateChangeEx(SWSAW_CMD_SHORTNAME(t), UNDO_STATE_ITEMS | UNDO_STATE_MISCCFG, -1);
}






/* Sorry this is experimental and sucks right now

void AWRecordQuickPunch(COMMAND_T* t)
{
	if (GetPlayState() & 4)
	{
		double selStart;
		double selEnd;
		double playPos;
		
		GetSet_LoopTimeRange2(0, 0, 0, &selStart, &selEnd, 0);
		
		playPos = GetPlayPosition();
		
		if (selStart > playPos)
		{
			GetSet_LoopTimeRange2(0, 1, 0, &playPos, &selEnd, 0);
		}
		else if (selStart < playPos)
		{
			GetSet_LoopTimeRange2(0, 1, 0, &selStart, &playPos, 0);
		}
			
	}
	else if (GetPlayState() == 0)
	{
		Main_OnCommand(40076, 0); //Set to time selection punch mode
		
		double selStart = 6000;
		double selEnd = 6001;
		
		GetSet_LoopTimeRange2(0, 1, 0, &selStart, &selEnd, 0);
		
		Main_OnCommand(1013, 0);
	}
	
	UpdateTimeline();
	Undo_OnStateChangeEx(SWSAW_CMD_SHORTNAME(t), UNDO_STATE_ITEMS | UNDO_STATE_MISCCFG, -1);
}
*/







void AWFadeSelection(COMMAND_T* t)
{
	double dFadeLen; 
	double dEdgeAdj;
	double dEdgeAdj1;
	double dEdgeAdj2;
	bool leftFlag=false;
	bool rightFlag=false;
	
	double selStart;
	double selEnd;
	
	GetSet_LoopTimeRange2(0, 0, 0, &selStart, &selEnd, 0);
	
	
	
	for (int iTrack = 1; iTrack <= GetNumTracks(); iTrack++)
	{
		MediaTrack* tr = CSurf_TrackFromID(iTrack, false);
		for (int iItem1 = 0; iItem1 < GetTrackNumMediaItems(tr); iItem1++)
		{
			MediaItem* item1 = GetTrackMediaItem(tr, iItem1);
			if (*(bool*)GetSetMediaItemInfo(item1, "B_UISEL", NULL))
			{
				double dStart1 = *(double*)GetSetMediaItemInfo(item1, "D_POSITION", NULL);
				double dEnd1   = *(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL) + dStart1;
				
				leftFlag = false;
				rightFlag = false;
				
				
				
				// Try to match up the end with the start of the other selected media items
				for (int iItem2 = 0; iItem2 < GetTrackNumMediaItems(tr); iItem2++)
				{

					MediaItem* item2 = GetTrackMediaItem(tr, iItem2);

					if (item1 != item2 && *(bool*)GetSetMediaItemInfo(item2, "B_UISEL", NULL))
					{
					
						double dStart2 = *(double*)GetSetMediaItemInfo(item2, "D_POSITION", NULL);
						double dEnd2   = *(double*)GetSetMediaItemInfo(item2, "D_LENGTH", NULL) + dStart2;
						//double dTest = fabs(dEnd1 - dStart2);
						
						// Need a tolerance for "items are adjacent". Also check to see if split is inside time selection
						// Found one case of items after split having diff edges 1.13e-13 apart, hopefully 2.0e-13 is enough (but not too much)
						
						if ((fabs(dEnd1 - dStart2) < 2.0e-13))
						{	// Found a matching item
								
							// If split is within selection and selection is within total item selection
							if ((selStart > dStart1) && (selEnd < dEnd2) && (selStart < dStart2) && (selEnd > dEnd1))
							{
								dFadeLen = selEnd - selStart;
								dEdgeAdj1 = selEnd - dEnd1;
								
								dEdgeAdj2 = dStart2 - selStart;
								
								if (dEdgeAdj2 > *(double*)GetSetMediaItemTakeInfo(GetActiveTake(item2), "D_STARTOFFS", NULL))
								{	
									
									dFadeLen -= dEdgeAdj2;
									
									dEdgeAdj2 = *(double*)GetSetMediaItemTakeInfo(GetActiveTake(item2), "D_STARTOFFS", NULL);
									
									dFadeLen += dEdgeAdj2;
								}
								
								
								// Move the edges around and set the crossfades
								double dLen1 = dEnd1 - dStart1 + dEdgeAdj1;
								*(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL) = dLen1;
								GetSetMediaItemInfo(item1, "D_FADEOUTLEN_AUTO", &dFadeLen);
								
								double dLen2 = *(double*)GetSetMediaItemInfo(item2, "D_LENGTH", NULL);
								dStart2 -= dEdgeAdj2;
								dLen2   += dEdgeAdj2;
								*(double*)GetSetMediaItemInfo(item2, "D_POSITION", NULL) = dStart2;
								*(double*)GetSetMediaItemInfo(item2, "D_LENGTH", NULL) = dLen2;
								GetSetMediaItemInfo(item2, "D_FADEINLEN_AUTO", &dFadeLen);
								double dSnapOffset2 = *(double*)GetSetMediaItemInfo(item2, "D_SNAPOFFSET", NULL);
								if (dSnapOffset2)
								{	// Only adjust the snap offset if it's non-zero
									dSnapOffset2 += dEdgeAdj2;
									GetSetMediaItemInfo(item2, "D_SNAPOFFSET", &dSnapOffset2);
								}
								
								for (int iTake = 0; iTake < GetMediaItemNumTakes(item2); iTake++)
								{
									MediaItem_Take* take = GetMediaItemTake(item2, iTake);
									if (take)
									{
										double dOffset = *(double*)GetSetMediaItemTakeInfo(take, "D_STARTOFFS", NULL);
										dOffset -= dEdgeAdj2;
										GetSetMediaItemTakeInfo(take, "D_STARTOFFS", &dOffset);
									}
								}
								rightFlag=true;
								break;
								
							}
							// If selection does not surround split or does not exist
							
							
							else
							{
								dFadeLen = fabs(*(double*)GetConfigVar("deffadelen")); // Abs because neg value means "not auto"
								dEdgeAdj1 = dFadeLen / 2.0;
								
								dEdgeAdj2 = dFadeLen / 2.0;
								
								// Need to ensure that there's "room" to move the start of the second item back
								// Check all of the takes' start offset before doing any "work"
								
								if (dEdgeAdj2 > *(double*)GetSetMediaItemTakeInfo(GetActiveTake(item2), "D_STARTOFFS", NULL))
								{	//break;
									
									dEdgeAdj2 = *(double*)GetSetMediaItemTakeInfo(GetActiveTake(item2), "D_STARTOFFS", NULL);
									
									dEdgeAdj1 = dFadeLen - dEdgeAdj2;
								}
							
							
								
								
								// We're all good, move the edges around and set the crossfades
								double dLen1 = dEnd1 - dStart1 + dEdgeAdj1;
								*(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL) = dLen1;
								GetSetMediaItemInfo(item1, "D_FADEOUTLEN_AUTO", &dFadeLen);
								
								double dLen2 = *(double*)GetSetMediaItemInfo(item2, "D_LENGTH", NULL);
								dStart2 -= dEdgeAdj2;
								dLen2   += dEdgeAdj2;
								*(double*)GetSetMediaItemInfo(item2, "D_POSITION", NULL) = dStart2;
								*(double*)GetSetMediaItemInfo(item2, "D_LENGTH", NULL) = dLen2;
								GetSetMediaItemInfo(item2, "D_FADEINLEN_AUTO", &dFadeLen);
								double dSnapOffset2 = *(double*)GetSetMediaItemInfo(item2, "D_SNAPOFFSET", NULL);
								if (dSnapOffset2)
								{	// Only adjust the snap offset if it's non-zero
									dSnapOffset2 += dEdgeAdj2;
									GetSetMediaItemInfo(item2, "D_SNAPOFFSET", &dSnapOffset2);
								}
								
								for (int iTake = 0; iTake < GetMediaItemNumTakes(item2); iTake++)
								{
									MediaItem_Take* take = GetMediaItemTake(item2, iTake);
									if (take)
									{
										double dOffset = *(double*)GetSetMediaItemTakeInfo(take, "D_STARTOFFS", NULL);
										dOffset -= dEdgeAdj2;
										GetSetMediaItemTakeInfo(take, "D_STARTOFFS", &dOffset);
									}
								}
								rightFlag=true;
								break;
							}
							
							
							
						}
						
						
						// If items are adjacent in opposite direction, turn flag on. Actual fades handled when situation is found
						// in reverse, but still need to flag it here
						
						else if ((fabs(dEnd2 - dStart1) < 2.0e-13))
						{
							leftFlag=true;
						}
						
						
						// if items already overlap (item 1 on left and item 2 on right)
						
						else if ((dStart2 < dEnd1) && (dEnd1 < dEnd2) && (dStart1 < dStart2)) 
						{
							
							
							
							// If split is within selection and selection is within total item selection
							if ((selStart > dStart1) && (selEnd < dEnd2) && (selEnd > dStart2))
							{
								dFadeLen = selEnd - selStart;
								dEdgeAdj1 = selEnd - dEnd1;
								
								dEdgeAdj2 = dStart2 - selStart;
								
								
								// Need to ensure that there's "room" to move the start of the second item back
								// Check all of the takes' start offset before doing any "work"								
							
								if (dEdgeAdj2 > *(double*)GetSetMediaItemTakeInfo(GetActiveTake(item2), "D_STARTOFFS", NULL))
								{
									
									dFadeLen -= dEdgeAdj2;
									
									dEdgeAdj2 = *(double*)GetSetMediaItemTakeInfo(GetActiveTake(item2), "D_STARTOFFS", NULL);
									
									dFadeLen += dEdgeAdj2;
									
									
								}
						
							
								// We're all good, move the edges around and set the crossfades
								double dLen1 = dEnd1 - dStart1 + dEdgeAdj1;
								*(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL) = dLen1;
								GetSetMediaItemInfo(item1, "D_FADEOUTLEN_AUTO", &dFadeLen);
								
								double dLen2 = *(double*)GetSetMediaItemInfo(item2, "D_LENGTH", NULL);
								dStart2 -= dEdgeAdj2;
								dLen2   += dEdgeAdj2;
								*(double*)GetSetMediaItemInfo(item2, "D_POSITION", NULL) = dStart2;
								*(double*)GetSetMediaItemInfo(item2, "D_LENGTH", NULL) = dLen2;
								GetSetMediaItemInfo(item2, "D_FADEINLEN_AUTO", &dFadeLen);
								double dSnapOffset2 = *(double*)GetSetMediaItemInfo(item2, "D_SNAPOFFSET", NULL);
								if (dSnapOffset2)
								{	// Only adjust the snap offset if it's non-zero
									dSnapOffset2 += dEdgeAdj2;
									GetSetMediaItemInfo(item2, "D_SNAPOFFSET", &dSnapOffset2);
								}
								
								for (int iTake = 0; iTake < GetMediaItemNumTakes(item2); iTake++)
								{
									MediaItem_Take* take = GetMediaItemTake(item2, iTake);
									if (take)
									{
										double dOffset = *(double*)GetSetMediaItemTakeInfo(take, "D_STARTOFFS", NULL);
										dOffset -= dEdgeAdj2;
										GetSetMediaItemTakeInfo(take, "D_STARTOFFS", &dOffset);
									}
								}
								rightFlag=true;
								break;
								
							}
							
							
							// If selection does not surround split or does not exist
							else
							{
								dFadeLen = dEnd1 - dStart2;
								dEdgeAdj = 0;
								
								// Need to ensure that there's "room" to move the start of the second item back
								// Check all of the takes' start offset before doing any "work"
								int iTake;
								for (iTake = 0; iTake < GetMediaItemNumTakes(item2); iTake++)
									if (dEdgeAdj > *(double*)GetSetMediaItemTakeInfo(GetMediaItemTake(item2, iTake), "D_STARTOFFS", NULL))
										break;
								if (iTake < GetMediaItemNumTakes(item2))
									continue;	// Keep looking
								
								// We're all good, move the edges around and set the crossfades
								double dLen1 = dEnd1 - dStart1 + dEdgeAdj;
								*(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL) = dLen1;
								GetSetMediaItemInfo(item1, "D_FADEOUTLEN_AUTO", &dFadeLen);
								
								double dLen2 = *(double*)GetSetMediaItemInfo(item2, "D_LENGTH", NULL);
								dStart2 -= dEdgeAdj;
								dLen2   += dEdgeAdj;
								*(double*)GetSetMediaItemInfo(item2, "D_POSITION", NULL) = dStart2;
								*(double*)GetSetMediaItemInfo(item2, "D_LENGTH", NULL) = dLen2;
								GetSetMediaItemInfo(item2, "D_FADEINLEN_AUTO", &dFadeLen);
								double dSnapOffset2 = *(double*)GetSetMediaItemInfo(item2, "D_SNAPOFFSET", NULL);
								if (dSnapOffset2)
								{	// Only adjust the snap offset if it's non-zero
									dSnapOffset2 += dEdgeAdj;
									GetSetMediaItemInfo(item2, "D_SNAPOFFSET", &dSnapOffset2);
								}
								
								for (iTake = 0; iTake < GetMediaItemNumTakes(item2); iTake++)
								{
									MediaItem_Take* take = GetMediaItemTake(item2, iTake);
									if (take)
									{
										double dOffset = *(double*)GetSetMediaItemTakeInfo(take, "D_STARTOFFS", NULL);
										dOffset -= dEdgeAdj;
										GetSetMediaItemTakeInfo(take, "D_STARTOFFS", &dOffset);
									}
								}
								rightFlag=true;
								break;
							}
							
							

						}
						
						
						
						// If the items overlap backwards ie item 2 is before item 1, turn on leftFlag
						// Actual fade calculations and stuff will be handled when the overlap gets discovered the opposite way
						// but this prevents other weird behavior
						
						
						else if ((dStart1 < dEnd2) && (dStart2 < dStart1) && (dEnd1 > dEnd2)) 
						{
							leftFlag=true;
						}
						
						
						
						// If there's a gap between two adjacent items, item 1 before item 2
						else if ((dEnd1 < dStart2))
						{
							
							// If selection starts inside item1, crosses gap and ends inside item 2
							if ((selStart > dStart1) && (selStart < dEnd1) && (selEnd > dStart2) && (selEnd < dEnd2))
							{
								dFadeLen = selEnd - selStart;
								dEdgeAdj1 = selEnd - dEnd1;
								dEdgeAdj2 = dStart2 - selStart;
								
								
								// Need to ensure that there's "room" to move the start of the second item back
								// Check all of the takes' start offset before doing any "work"
								int iTake;
								
								
								for (iTake = 0; iTake < GetMediaItemNumTakes(item2); iTake++)
									if (dEdgeAdj2 > *(double*)GetSetMediaItemTakeInfo(GetMediaItemTake(item2, iTake), "D_STARTOFFS", NULL))
									{
										dFadeLen -= dEdgeAdj2;
										
										dEdgeAdj2 = *(double*)GetSetMediaItemTakeInfo(GetMediaItemTake(item2, iTake), "D_STARTOFFS", NULL);
										
										dFadeLen += dEdgeAdj2;
									}
								if (iTake < GetMediaItemNumTakes(item2))
									continue;	// Keep looking
								
								
								
								// We're all good, move the edges around and set the crossfades
								double dLen1 = dEnd1 - dStart1 + dEdgeAdj1;
								*(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL) = dLen1;
								GetSetMediaItemInfo(item1, "D_FADEOUTLEN_AUTO", &dFadeLen);
								
								double dLen2 = *(double*)GetSetMediaItemInfo(item2, "D_LENGTH", NULL);
								dStart2 -= dEdgeAdj2;
								dLen2   += dEdgeAdj2;
								
								*(double*)GetSetMediaItemInfo(item2, "D_POSITION", NULL) = dStart2;
								*(double*)GetSetMediaItemInfo(item2, "D_LENGTH", NULL) = dLen2;
								GetSetMediaItemInfo(item2, "D_FADEINLEN_AUTO", &dFadeLen);
								double dSnapOffset2 = *(double*)GetSetMediaItemInfo(item2, "D_SNAPOFFSET", NULL);
								if (dSnapOffset2)
								{	// Only adjust the snap offset if it's non-zero
									dSnapOffset2 += dEdgeAdj2;
									GetSetMediaItemInfo(item2, "D_SNAPOFFSET", &dSnapOffset2);
								}
								
								for (iTake = 0; iTake < GetMediaItemNumTakes(item2); iTake++)
								{
									MediaItem_Take* take = GetMediaItemTake(item2, iTake);
									if (take)
									{
										double dOffset = *(double*)GetSetMediaItemTakeInfo(take, "D_STARTOFFS", NULL);
										dOffset -= dEdgeAdj2;
										GetSetMediaItemTakeInfo(take, "D_STARTOFFS", &dOffset);
									}
								}
								rightFlag=true;
								break;
								
							}
						}
						
						// Else if there's a gap but the items are in reverse order, set the opposite flag but let the fades
						// get taken care of when it finds them "item 1 (gap) item 2"
						
						else if ((dEnd2 < dStart1))
						{
							if ((selStart > dStart2) && (selStart < dEnd2) && (selEnd > dStart1) && (selEnd < dEnd1))
							{
								leftFlag=true;
								
								//Check for selected items in between item 1 and 2
								
								for (int iItem3 = 0; iItem3 < GetTrackNumMediaItems(tr); iItem3++)
								{
									
									MediaItem* item3 = GetTrackMediaItem(tr, iItem3);
									
									if (item1 != item2 && item2 != item3 && *(bool*)GetSetMediaItemInfo(item3, "B_UISEL", NULL))
									{
										
										double dStart3 = *(double*)GetSetMediaItemInfo(item3, "D_POSITION", NULL);
										double dEnd3   = *(double*)GetSetMediaItemInfo(item3, "D_LENGTH", NULL) + dStart3;
										
										// If end of item 3 is between end of item 2 and start of item 1, reset the flag
										
										if (dEnd3 > dEnd2 && dEnd3 <= dStart1)
										{
											leftFlag = false;
										}
									
									}
								}
							}
						}
					}
				}

				if (!(leftFlag))
				{
					double fadeLength;
					
					if ((selStart <= dStart1) && (selEnd > dStart1) && (selEnd < dEnd1) && (!(rightFlag)))
					{			
						fadeLength = selEnd - dStart1;
						SetMediaItemInfo_Value(item1, "D_FADEINLEN_AUTO", fadeLength);
					}
					
					else if (selStart <= dStart1 && selEnd >= dEnd1)
					{
						double fadeLength = fabs(*(double*)GetConfigVar("deffadelen")); // Abs because neg value means "not auto"
						SetMediaItemInfo_Value(item1, "D_FADEINLEN_AUTO", fadeLength);

					}

					
				}
				
				if (!(rightFlag))
				{
					double fadeLength;
					
					if ((selStart > dStart1) && (selEnd >= dEnd1) && (selStart < dEnd1) && (!(leftFlag)))
					{			
						fadeLength = dEnd1 - selStart;
						SetMediaItemInfo_Value(item1, "D_FADEOUTLEN_AUTO", fadeLength);
					}
					else if (selStart <= dStart1 && selEnd >= dEnd1)
					{
						double fadeLength = fabs(*(double*)GetConfigVar("deffadelen")); // Abs because neg value means "not auto"
						SetMediaItemInfo_Value(item1, "D_FADEOUTLEN_AUTO", fadeLength);
					}
				}		
			}
		}
	}
	
	UpdateTimeline();
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
	
}





static COMMAND_T g_commandTable[] = 
{
	// Add commands here (copy paste an example from ItemParams.cpp or similar	
	{ { DEFACCEL, "SWS/AdamWathan: Fill gaps between selected items (advanced)" },												"SWS_AWFILLGAPSADV",				AWFillGapsAdv, NULL, 0 },
	{ { DEFACCEL, "SWS/AdamWathan: Fill gaps between selected items (advanced, use last settings)" },							"SWS_AWFILLGAPSADVLASTSETTINGS",	AWFillGapsAdv, NULL, 1 },
	{ { DEFACCEL, "SWS/AdamWathan: Fill gaps between selected items (quick, no crossfade)" },									"SWS_AWFILLGAPSQUICK",				AWFillGapsQuick, },
	{ { DEFACCEL, "SWS/AdamWathan: Fill gaps between selected items (quick, crossfade using default fade length)" },			"SWS_AWFILLGAPSQUICKXFADE",			AWFillGapsQuickXFade, },
	{ { DEFACCEL, "SWS/AdamWathan: Remove overlaps in selected items preserving item starts" },									"SWS_AWFIXOVERLAPS",				AWFixOverlaps, },
	{ { DEFACCEL, "SWS/AdamWathan: Record (conditional, normal record mode unless time sel exists, then autopunch)" },			"SWS_AWRECORDCOND",					AWRecordConditional, },
	{ { DEFACCEL, "SWS/AdamWathan: Play/Stop (automatically group simultaneously recorded items)" },			"SWS_AWPLAYSTOPGRP",					AWPlayStopAutoGroup, },
	{ { DEFACCEL, "SWS/AdamWathan: Select from cursor to end of project (items and time selection)" },			"SWS_AWSELTOEND",					AWSelectToEnd, },
	//{ { DEFACCEL, "SWS/AdamWathan: Quick Punch Record" },			"SWS_AWQUICKPUNCH",					AWRecordQuickPunch, },
	{ { DEFACCEL, "SWS/AdamWathan: Fade in/out/crossfade selected area of selected items" },			"SWS_AWFADESEL",					AWFadeSelection, },
	
	
	{ {}, LAST_COMMAND, }, // Denote end of table
};

int AdamInit()
{
	SWSRegisterCommands(g_commandTable);

	return 1;
}