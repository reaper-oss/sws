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
	const char names[] = "Trigger Pad (ms),Crossfade Length (ms),Maximum Gap (ms),Maximum Stretch (0.5 is double),Preserve Transient (ms),Transient Crossfade Length (ms)";
	char defaultValues[100] = "5,5,25,0.85,35,5";
	int maxReturnLen = 100;
	int nitems = 6;
	
	// Call dialog
	bool userInput = GetUserInputs("AutoPocket - Smoothing",nitems,names,defaultValues,maxReturnLen);
	
	// If user hit okay, get the parameters they entered
	if (userInput)	
	{
		
		// Divided by 1000 to convert to milliseconds except maxStretch
		double triggerPad = (atof(strtok(defaultValues, ",")))/1000;
		double fadeLength = (atof(strtok(NULL, ",")))/1000;
		double maxGap = (atof(strtok(NULL, ",")))/1000;
		double maxStretch = atof(strtok(NULL, ","));
		double presTrans = (atof(strtok(NULL, ",")))/1000;
		double transFade = (atof(strtok(NULL, ",")))/1000;
		
	
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
								
								// Split item1 at the split point
								MediaItem* item1B = SplitMediaItem(item1, splitPoint);
								
								// Get new item1 length after split
								item1Length = GetMediaItemInfo_Value(item1, "D_LENGTH") + transFade;
								
								// Create overlap of 'transFade'
								SetMediaItemInfo_Value(item1, "D_LENGTH", item1Length);
								
								// Crossfade the overlap
								SetMediaItemInfo_Value(item1, "D_FADEOUTLEN_AUTO", transFade);
								SetMediaItemInfo_Value(item1B, "D_FADEINLEN_AUTO", transFade);
								
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
				
				// If either item are selected, run this loop
				if ((GetMediaItemInfo_Value(item1, "B_UISEL")) || (GetMediaItemInfo_Value(item2, "B_UISEL")))
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
							item1Length = item2Start - item1Start - triggerPad;
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
	}
	
	
	UpdateTimeline();
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
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
			
			// If either item are selected, run this loop
			if ((GetMediaItemInfo_Value(item1, "B_UISEL")) || (GetMediaItemInfo_Value(item2, "B_UISEL")))
			{
				// Get various pieces of info for the 2 items
				double item1Start = GetMediaItemInfo_Value(item1, "D_POSITION");
				double item1Length = GetMediaItemInfo_Value(item1, "D_LENGTH");
				double item1End = item1Start + item1Length;
			
				double item2Start = GetMediaItemInfo_Value(item2, "D_POSITION");
			
				// If the first item overlaps the second item, trim the first item
				if (item1End > (item2Start))
				{
					
					// If both items selected, account for trigger pad, if not, just trim to item 2 start
					if (GetMediaItemInfo_Value(item1, "B_UISEL") && GetMediaItemInfo_Value(item2, "B_UISEL"))
					{
						item1Length = item2Start - item1Start;
					}
					else
					{
						item1Length = item2Start - item1Start;
					}
					
					SetMediaItemInfo_Value(item1, "D_LENGTH", item1Length);
				}
				
				if (item1End <= (item2Start))
				{
					// If both items selected, account for trigger pad, if not, do nothing
					if (GetMediaItemInfo_Value(item1, "B_UISEL") && GetMediaItemInfo_Value(item2, "B_UISEL"))
					{
						item1Length = item2Start - item1Start;
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
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
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
			
			// If either item are selected, run this loop
			if ((GetMediaItemInfo_Value(item1, "B_UISEL")) || (GetMediaItemInfo_Value(item2, "B_UISEL")))
			{
				// Get various pieces of info for the 2 items
				double item1Start = GetMediaItemInfo_Value(item1, "D_POSITION");
				double item1Length = GetMediaItemInfo_Value(item1, "D_LENGTH");
				double item1End = item1Start + item1Length;
				
				double item2Start = GetMediaItemInfo_Value(item2, "D_POSITION");
				
				// If the first item overlaps the second item, trim the first item
				if (item1End > (item2Start))
				{
					
					// If both items selected, account for trigger pad, if not, just trim to item 2 start
					if (GetMediaItemInfo_Value(item1, "B_UISEL") && GetMediaItemInfo_Value(item2, "B_UISEL"))
					{
						item1Length = item2Start - item1Start;
					}
					else
					{
						item1Length = item2Start - item1Start;
					}
					
					SetMediaItemInfo_Value(item1, "D_LENGTH", item1Length);
				}
				
				if (item1End <= (item2Start))
				{
					// If both items selected, account for trigger pad, if not, do nothing
					if (GetMediaItemInfo_Value(item1, "B_UISEL") && GetMediaItemInfo_Value(item2, "B_UISEL"))
					{
						item1Length = item2Start - item1Start;
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
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
}

static COMMAND_T g_commandTable[] = 
{
	// Add commands here (copy paste an example from ItemParams.cpp or similar
	{ { DEFACCEL, "SWS/AdamWathan: Fill gaps between selected items (advanced)" },					"SWS_AWFILLGAPSADV",		AWFillGapsAdv, },
	{ { DEFACCEL, "SWS/AdamWathan: Fill gaps between selected items (quick, no crossfade)" },					"SWS_AWFILLGAPSQUICK",		AWFillGapsQuick, },
	{ { DEFACCEL, "SWS/AdamWathan: Fill gaps between selected items (quick, crossfade using default fade length)" },					"SWS_AWFILLGAPSQUICKXFADE",		AWFillGapsQuickXFade, },
	
	{ {}, LAST_COMMAND, }, // Denote end of table
};

int AdamInit()
{
	SWSRegisterCommands(g_commandTable);

	return 1;
}