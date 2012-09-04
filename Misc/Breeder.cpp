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
/ NON INFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
/ HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
/ WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/ FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
/ OTHER DEALINGS IN THE SOFTWARE.
/
******************************************************************************/

#include "stdafx.h"
#include "Breeder.h"

#include "../MarkerList/MarkerListActions.h"
#include "../reaper/localize.h"
#include "../SnM/SnM_Dlg.h"



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
	int tCount = CountTempoTimeSigMarkers(NULL) - 1;
	
	while (tCount != -1)
	{
		double tPos;
		GetTempoTimeSigMarker(NULL, tCount, &tPos, NULL, NULL, NULL, NULL, NULL, NULL);
		
		if (tPos < ePos)
		{
			SetEditCurPos (tPos, NULL, NULL);
			break;
		}
		--tCount;
	}
};


void BRSplitSelectedItemsAtTempoMarkers(COMMAND_T*)
{
	// Get selected items
	int itemCount = CountSelectedMediaItems(NULL);
	vector<MediaItem*> selectedItems;
	
	if (itemCount != 0)
	{	
		Undo_BeginBlock2(NULL);

		for (int i = 0; i < itemCount; ++i)
			selectedItems.push_back(GetSelectedMediaItem(NULL, i));
	
		// Loop through selected items 
		MediaItem* split;
		MediaItem* item;
		double iStart, iEnd, tPos;
	
		for (int i = 0; i < itemCount; ++i)
		{	
			item = selectedItems[i];
			iStart = GetMediaItemInfo_Value(item, "D_POSITION");
			iEnd = iStart + GetMediaItemInfo_Value(item, "D_LENGTH");
			tPos = iStart - 1;
			
			// Split item currently in the loop
			while (true)
			{	
				split = SplitMediaItem(item, tPos);
				if (split !=0)
					item = split;
			
				tPos = TimeMap2_GetNextChangeTime(NULL, tPos);
				if (tPos > iEnd)
					break;
			}
		}
		UpdateArrange();
		Undo_EndBlock2 (NULL, __LOCALIZE("Split selected items at temo markers","sws_undo"), UNDO_STATE_ALL);
	}
};


void BRConvertProjectMarkersToTempoMarkers(int num, int den, int measure, int timeSelection, int removeMarkers)
{
	// Get time selection
	double tStart, tEnd;
	
	if (timeSelection == 1)
	{
		GetSet_LoopTimeRange2 (NULL, false, false, &tStart, &tEnd, false);
		if (tStart == tEnd)
			{	
				MessageBox(g_hwndParent, __LOCALIZE("To convert only within time selection please create one.","sws_DLG_166"), __LOCALIZE("SWS - Error","sws_mbox"), MB_OK);	
				return;
			}
	}
		

	// Get markers
	bool region, prevRegion;
	double mPos, prevMPos;
	int current, previous, markerId, markerCount = 0, i=0;
	
	vector<double> markerPositions;
	
	while (true)
	{
		current = EnumProjectMarkers2(NULL, i, &region, &mPos, NULL, NULL, &markerId);
		previous = EnumProjectMarkers2(NULL, i-1, &prevRegion, &prevMPos, NULL, NULL, NULL);
		
		if (current == 0)
			break;

		if (region == 0)
		{
			// In certain corner cases involving regions, reaper will output the same marker more
			// than once. This should prevent those duplicate values from being written into vector.
			if ((mPos != prevMPos)||(mPos == prevMPos && prevRegion == 1) || (previous == 0))
			{
				if (timeSelection == 0)
				{	
					markerPositions.push_back(mPos);
					++markerCount;
				}	
				if (timeSelection == 1)
				{
					if (mPos > tEnd)
						break;
					
					if (mPos >= tStart)
					{	
						markerPositions.push_back(mPos);
						++markerCount;
					}
				}
			}	
		}
		++i;
	}
	

	// Check number of markers
	if ( markerPositions.size() <=1)
	{	
		if (timeSelection == 0)
		{
			MessageBox(g_hwndParent, __LOCALIZE("Not enough project markers in the project to perform conversion.","sws_DLG_166"), __LOCALIZE("SWS - Error","sws_mbox"), MB_OK);	
			return;
		}
		
		if (timeSelection == 1)
		{	
			MessageBox(g_hwndParent, __LOCALIZE("Not enough project markers in the time selection to perform conversion.","sws_DLG_166"), __LOCALIZE("SWS - Error","sws_mbox"), MB_OK);	
			return;
		}
	}
	

	// Check if there are existing tempo markers after the first tempo marker
	int tCount = CountTempoTimeSigMarkers(NULL);
	
	if (tCount > 0)
	{		
		double tPos;
		int answer, check = 0;
		for (i = 0; i < tCount; ++i)
		{
			GetTempoTimeSigMarker(NULL, i, &tPos, NULL, NULL, NULL, NULL, NULL, NULL);
			
			if (check == 0 && tPos > markerPositions.front() && tPos <= markerPositions.back())
			{
				answer = MessageBox(g_hwndParent, __LOCALIZE("Detected existing tempo markers between project markers.\nAre you sure you want to continue? Strange things may happen.","sws_DLG_166"), __LOCALIZE("SWS - Warning","sws_mbox"), MB_YESNO);
				if (answer == 7)
					return;
				if (answer == 6)
					check = 1;
			}

			if (tPos > markerPositions.back())
			{
				answer = MessageBox(g_hwndParent, __LOCALIZE("Detected existing tempo markers after the last project marker.\nThey may get moved during the conversion. Are you sure you want to continue?","sws_DLG_166"), __LOCALIZE("SWS - Warning","sws_mbox"), MB_YESNO);
				if (answer == 7)
					return;
				if (answer == 6)
					break;
			}
		}
	}

	// If all went well start converting
	Undo_BeginBlock2(NULL);
	
		// Set first tempo marker with time signature
		double length = markerPositions[1] - markerPositions[0];
		double bpm = (60 * 4/den*num) / (measure * length);
		SetTempoTimeSigMarker (NULL, -1, markerPositions[0], -1, -1, bpm, num, den, NULL);

		// Set the rest of the tempo markers. I tried to use iterators here but 
		// it was MUCH slower than this method. 8000 markers are converted in a 
		// matter of 6 seconds here, while iterators method took forever
		i = 1;
		while (i+1 < markerCount)
		{ 
			length = markerPositions[i+1] - markerPositions[i];
			bpm = (60 * 4/den*num) / (measure * length);
			SetTempoTimeSigMarker (NULL, -1, markerPositions[i], -1, -1, 0, 0, den, NULL);
			++i;
		}
		UpdateTimeline();

	Undo_EndBlock2 (NULL, __LOCALIZE("Convert project markers to tempo markers","sws_undo"), UNDO_STATE_ALL);

	// Remove markers
	
	
	if (removeMarkers == 1)
	{
		Undo_BeginBlock2(NULL);
		if (timeSelection == 0)
		{	
			DeleteAllMarkers();
			Undo_EndBlock2 (NULL, __LOCALIZE("Delete all project markers","sws_undo"), UNDO_STATE_ALL);
		}
		if (timeSelection == 1)
		{	
			Main_OnCommand(40420, 0);
			Undo_EndBlock2 (NULL, __LOCALIZE("Delete project markers within time selection","sws_undo"), UNDO_STATE_ALL);
		}
	}
	
			
};



// Dialog box procedure - lots of code shamelessly stolen from a lot 
// of places. :) My excuse is one of the educational nature :) If 
// somebody finds anything that sucks here - please do correct.
WDL_DLGRET BRProjectMarkersToTempoMarkersProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, uMsg, wParam, lParam))
		return r;

	const char cWndPosKey[] = "ConvertMarkers WndPos";
	switch(uMsg)
	{
		case WM_INITDIALOG :
		{
			/// CREATE DEFAULT VALUES ///
			
			// Remove marker checkbox
			int removeMarkers = 1;

			// Time selection checkbox
			double tStart, tEnd;
			int timeSelection;
			GetSet_LoopTimeRange2 (NULL, false, false, &tStart, &tEnd, false);
			if (tStart == tEnd)
			{
				timeSelection = 0;
				tStart = 0;
			}
			else
				timeSelection = 1;

			// Time signature and markers edit box
			bool region, check = true;
			double mPos;
			int currentMarker, i = 0;
			while (check == true)
			{			
				currentMarker = EnumProjectMarkers2(NULL, i, &region, &mPos, NULL, NULL, NULL);
				if (timeSelection == 1)
				{
					if (mPos >= tStart && mPos <=tEnd)
						check = region;
					if (currentMarker == 0)
					{		
						check = region;
						mPos = tStart;
					}
				}
				if (timeSelection == 0)
					check = region;
				++i;
			}
			
			int num, den;
			if (timeSelection == 1 && mPos > tEnd)
				TimeMap_GetTimeSigAtTime(NULL, tStart, &num, &den, NULL);
			else
				TimeMap_GetTimeSigAtTime(NULL, mPos, &num, &den, NULL);

			char timeSigNum[128] = {0}, timeSigDen[128] = {0}, markersPerMeasure[128] = {0} ;				
			sprintf(timeSigNum, "%d", num);
			sprintf(timeSigDen, "%d", den);
			sprintf(markersPerMeasure, "%d", num);

			/// SET CONTROLS ///
			SetDlgItemText(hwnd, IDC_BR_NUM, timeSigNum);
			SetDlgItemText(hwnd, IDC_BR_DEN, timeSigDen);
			SetDlgItemText(hwnd, IDC_BR_MARKERS, markersPerMeasure);
			CheckDlgButton(hwnd, IDC_BR_TIMESEL, !!timeSelection);
			CheckDlgButton(hwnd, IDC_BR_REMOVE, !!removeMarkers);
			
			RestoreWindowPos(hwnd, cWndPosKey, false);
			return 0;
		}
		break;
        
		case WM_COMMAND :
		{
            switch(LOWORD(wParam))
            {
                case IDOK:
				{
					// Read values
					char timeSigNum[128], timeSigDen[128], markersPerMeasure[128];
					int timeSelection, removeMarkers;
					
					GetDlgItemText(hwnd, IDC_BR_NUM, timeSigNum, 128);
					GetDlgItemText(hwnd, IDC_BR_DEN, timeSigDen, 128);
					GetDlgItemText(hwnd, IDC_BR_MARKERS, markersPerMeasure, 128);
					timeSelection = IsDlgButtonChecked(hwnd, IDC_BR_TIMESEL);
					removeMarkers = IsDlgButtonChecked(hwnd, IDC_BR_REMOVE);
					
					int num = atoi(timeSigNum);
					int den = atoi(timeSigDen);
					int measure = atoi(markersPerMeasure);
					
					// Check values
					int i = 0;
					if ((num <= 0) || (den <= 0) || (measure <= 0))
					{	
						MessageBox(g_hwndParent, __LOCALIZE("You can't input zero as a value.","sws_DLG_166"), __LOCALIZE("SWS - Error","sws_mbox"), MB_OK);
						i = 1;
						while (true)
						{
							if (measure <= 0)
							{
								SetFocus(GetDlgItem(hwnd, IDC_BR_MARKERS));
								break;
							}
							if (num <= 0)
							{
								SetFocus(GetDlgItem(hwnd, IDC_BR_NUM));
								break;
							}
							if (den <= 0)
							{	
								SetFocus(GetDlgItem(hwnd, IDC_BR_DEN));
								break;
							}
						}
					}
	
					// If all went well, start converting
					if(wParam == IDOK && i == 0)
					{
						BRConvertProjectMarkersToTempoMarkers(num, den, measure, timeSelection, removeMarkers);
						SetFocus(hwnd);	
					}
					return 0;
				}
				break;
			
				case IDCANCEL:
					ShowWindow(hwnd, SW_HIDE);
					break;
			}
		}
		
		case WM_DESTROY:
			SaveWindowPos(hwnd, cWndPosKey);
			break; 
	}
	return 0;
};

void BRProjectMarkersToTempoMarkers(COMMAND_T* t)
{	
	// Detect timebase
	if (*(int*)GetConfigVar("itemtimelock") != 0)
	{	
		int answer = MessageBox(g_hwndParent, __LOCALIZE("Project timebase is not set to time. Do you want to set it now?","sws_DLG_166"), __LOCALIZE("SWS - Warning","sws_mbox"), MB_YESNOCANCEL);
		if (answer == 2)
			return;
		if (answer == 6)
			*(int*)GetConfigVar("itemtimelock") = 0;
	}	

	// Call dialog
	static HWND hwnd = CreateDialog (g_hInst, MAKEINTRESOURCE(IDD_BR_MARKERS_TO_TEMPO), g_hwndParent, BRProjectMarkersToTempoMarkersProc);
	ShowWindow(hwnd, SW_SHOW);
	SetFocus(hwnd);
};




//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "SWS/BR: Move edit cursor to next tempo marker" },													"SWS_BRMOVEEDITNEXTTEMPO",				BRMoveEditNextTempo, },
	{ { DEFACCEL, "SWS/BR: Move edit cursor to previous tempo marker" },												"SWS_BRMOVEEDITPREVTEMPO",				BRMoveEditPrevTempo, },
	{ { DEFACCEL, "SWS/BR: Convert project markers to tempo markers" },													"SWS_BRCONVERTMARKERSTOTEMPO",			BRProjectMarkersToTempoMarkers, },
	{ {	DEFACCEL, "SWS/BR: Split selected items at tempo markers" },													"SWS_BRSPLITSELECTEDTEMPO",				BRSplitSelectedItemsAtTempoMarkers, },

	{ {}, LAST_COMMAND, }, // Denote end of table
};

//!WANT_LOCALIZE_1ST_STRING_END




int BreederInit()
{
	SWSRegisterCommands(g_commandTable);
	return 1;
};
