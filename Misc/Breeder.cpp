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
// TODO: Finish Select and adjust tempo markers dialog and actions

#include "stdafx.h"
#include "Breeder.h"
#include "../MarkerList/MarkerListActions.h"
#include "../reaper/localize.h"
#include "../SnM/SnM_Dlg.h"

//Globals
static bool g_convertProjectMarkersDialog = false;
static bool g_selectAdjustTempoPointsDialog = false;

//Prototypes
void BRConvertProjectMarkersToTempoMarkers(int num, int den, int measure, int timeSelection, int removeMarkers);
//void BRSelectTempoPoints (int time, int BPM, double BPMStart, double BPMEnd, int shape, int sig, int sigNum, int sigDen);
vector<int> BRGetSelectedTempoPoints();
WDL_DLGRET BRConvertProjectMarkersToTempoMarkersProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//WDL_DLGRET BRSelectAdjustTempoPointsProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

																  
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//								  		ACTIONS
//////////////////////////////////////////////////////////////////////////////////////////////////////////

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
				if (tPos > iEnd || tPos == -1 )
					break;
			}
		}
		UpdateArrange();
		Undo_EndBlock2 (NULL, __LOCALIZE("Split selected items at temo markers","sws_undo"), UNDO_STATE_ALL);
	}
};


bool IsConvertProjectMarkerVisible(COMMAND_T* = NULL)
{
	return g_convertProjectMarkersDialog;
};
void BRConvertProjectMarkersToTempoMarkersAction(COMMAND_T* = NULL)
{	
	// Create dialog on the first run...
	static HWND hwnd = CreateDialog (g_hInst, MAKEINTRESOURCE(IDD_BR_MARKERS_TO_TEMPO), g_hwndParent, BRConvertProjectMarkersToTempoMarkersProc);

	// Check its state and hide/show it accordingly
	if (g_convertProjectMarkersDialog)
	{	
		g_convertProjectMarkersDialog = false;
		ShowWindow(hwnd, SW_HIDE);
	}
	
	else
	{		
		// Detect timebase
		if (*(int*)GetConfigVar("itemtimelock") != 0)
		{	
			int answer = MessageBox(g_hwndParent, __LOCALIZE("Project timebase is not set to time. Do you want to set it now?","sws_DLG_166"), __LOCALIZE("SWS - Warning","sws_mbox"), MB_YESNOCANCEL);
			if (answer == 6)
				*(int*)GetConfigVar("itemtimelock") = 0;
		}	
		g_convertProjectMarkersDialog = true;
		ShowWindow(hwnd, SW_SHOW);
		SetFocus(hwnd);
	}
};


void BRMoveSelTempoPoint(COMMAND_T* t ) 
{ 
	// Get amount of movement to be performed on the selected tempo points
	double timeDiff = (int)t->user;
	if (t->user == 5 || t->user == -5)
			timeDiff = 1 / GetHZoomLevel() * timeDiff / 5;
	else if (t->user == 2 || t->user == -2)
			timeDiff = 0.0001 * timeDiff / 2;
	else
			timeDiff = timeDiff/1000;
	
	// Get selected points' ID, remove first tempo markers in the project from the vector and check vector size
	vector<int> selectedTempoPoints = BRGetSelectedTempoPoints();
	if (selectedTempoPoints.size() != 0)
	{
		if (selectedTempoPoints[0] == 0)
		selectedTempoPoints.erase (selectedTempoPoints.begin());
	}
	if (selectedTempoPoints.size() == 0)
		return;

	// Temporary change of preferences - prevent reselection of points in time selection
	int envClickSegMode = *(int*)GetConfigVar("envclicksegmode");
	*(int*)GetConfigVar("envclicksegmode") = 1;
	
	Undo_BeginBlock2(NULL);
	int skipped	= 0;
	// Loop through selected points and perform BPM calculations
	for (size_t i = 0; i < selectedTempoPoints.size(); ++i)
	{
		// Declare needed variables... :D
		double pTime, pBeat, pBPM, cTime, cBeat, cBPM, nTime, nBPM, measureLengthPC, measureLengthCN, pNewBPM, cNewBPM;
		int pMeasure, pNum, pDen, pEffNum, pEffDen, cMeasure, cNum, cDen, cEffNum, cEffDen;
		bool pShape, cShape; 
		vector <int> partialID;
		vector <double> partialTime;

		// Get tempo points
		GetTempoTimeSigMarker(NULL, selectedTempoPoints[i] -1 , &pTime, &pMeasure, &pBeat, &pBPM, &pNum, &pDen, &pShape);
		GetTempoTimeSigMarker(NULL, selectedTempoPoints[i], &cTime, &cMeasure, &cBeat, &cBPM, &cNum, &cDen, &cShape);
		GetTempoTimeSigMarker(NULL, selectedTempoPoints[i] + 1 , &nTime, NULL, NULL, &nBPM, NULL, NULL, NULL);
		
		// Get effective time signature (if point doesn't have it, it's time sig outputs 0)
		// and save points with time signature for later correction of partial points
		if (cNum == 0)
			TimeMap_GetTimeSigAtTime(NULL, cTime, &cEffNum, &cEffDen, NULL);
		else
		{
			cEffNum = cNum, cEffDen = cDen;
			partialID.push_back (selectedTempoPoints[i]);
			partialTime.push_back (cTime + timeDiff);
		}
		
		if (pNum == 0)
			TimeMap_GetTimeSigAtTime(NULL, pTime, &pEffNum, &pEffDen, NULL);
		else
		{
			pEffNum = pNum, pEffDen = pDen;
			partialID.push_back (selectedTempoPoints[i]-1);
			partialTime.push_back (pTime);
		}
		

		///// Calculate BPM values /////		

		// If previous point's shape is square...
		if (pShape == 0)
		{
			measureLengthPC = (pBPM * pEffDen * (cTime - pTime)) / (240 * pEffNum);
			pNewBPM = (240 * pEffNum * measureLengthPC) / (pEffDen * (cTime - pTime + timeDiff));
			
			// If current point's shape is square...
			if (cShape == 0)
			{
			   	measureLengthCN = (cBPM * cEffDen * (nTime - cTime)) / (240 * cEffNum);
				cNewBPM = (240 * cEffNum * measureLengthCN) / (cEffDen * (nTime - cTime - timeDiff));
				// fix for the last tempo point
				if ( selectedTempoPoints[i] + 1 == CountTempoTimeSigMarkers(NULL))
					cNewBPM = cBPM;
			}
		
			// If current point's shape is linear...
			if (cShape == 1)
			{
				measureLengthCN = (cEffDen * (nTime - cTime) * (cBPM + nBPM)) / (480 * cEffNum);
				cNewBPM = (480 * cEffNum * measureLengthCN) / (cEffDen * (nTime - cTime - timeDiff)) - nBPM;
				// fix for the last tempo point
				if ( selectedTempoPoints[i] + 1 == CountTempoTimeSigMarkers(NULL))
					cNewBPM = cBPM;
			}
		}
		
		// If previous point's shape is linear...
		if (pShape == 1)
		{
			measureLengthPC =  (pEffDen * (cTime - pTime) * (pBPM + cBPM)) / (480 * pEffNum);
			
			// If current point's shape is square...
			if (cShape == 0)
			{
				measureLengthCN = (cBPM * cEffDen * (nTime - cTime)) / (240 * cEffNum);
				
				cNewBPM = (240 * cEffNum * measureLengthCN) / (cEffDen * (nTime - cTime - timeDiff));
				// fix for the last tempo point
				if ( selectedTempoPoints[i] + 1 == CountTempoTimeSigMarkers(NULL))
					cNewBPM = cBPM;
				pNewBPM = (480 * pEffNum * measureLengthPC) / (pEffDen * (cTime - pTime + timeDiff)) - cNewBPM;
			}
				
			// If current point's shape is linear...
			if (cShape == 1)
			{
				measureLengthCN = (cEffDen * (nTime - cTime) * (cBPM + nBPM)) / (480 * cEffNum);
				cNewBPM = (480 * cEffNum * measureLengthCN) / (cEffDen * (nTime - cTime - timeDiff)) - nBPM;
				// fix for the last tempo point
				if ( selectedTempoPoints[i] + 1 == CountTempoTimeSigMarkers(NULL))
					cNewBPM = cBPM;

				pNewBPM = (480 * pEffNum * measureLengthPC) / (pEffDen * (cTime - pTime + timeDiff)) - cNewBPM;
			}
		
		}

		
		///// Check points behind previous point (to see if new BPM values are possible) ///////
		double ppTime, ppBPM;
		int ppNum, ppDen;
		int pp = selectedTempoPoints[i] - 2, changeDirection = 1;
		bool ppShape, ppDo = true, ppPossible = true; 
		
		// Check that the point behind previous point exists
		if (pp >= 0)
		{
			// Get the first point behind previous point - If square change ppDo var so later we can skip the process of adjusting previous points
			GetTempoTimeSigMarker(NULL, pp, &ppTime, NULL, NULL, &ppBPM, &ppNum, &ppDen, &ppShape);
			if (ppShape == 0)
				ppDo = false;
					
			// Otherwise go through points backwards, check their shape and BPM to be applied
			else
			{
				while (pp >= 0) 
				{
					GetTempoTimeSigMarker(NULL, pp, &ppTime, NULL, NULL, &ppBPM, &ppNum, &ppDen, &ppShape);
					
					if (ppShape == 0)  // If current points in the loop is square, break
						break;
					else
					{
						// Check if new BPM is possible...
						if ((ppBPM - changeDirection * (pNewBPM - pBPM)) <= 960 && (ppBPM - changeDirection * (pNewBPM - pBPM)) >= 0.001)
						{		
							changeDirection = changeDirection * -1;
							// If possible, also add any point with time signature for later correction of partial points
							if (ppNum != 0)
							{
								partialID.push_back (pp);
								partialTime.push_back (ppTime);
							}
						}
						else 
						{
							ppPossible = false;
							break;
						}
					}
					pp--;
				}
				pp++; //corret ID due to the nature of while loop
			}
		}
		else
			ppDo = false;

		
		///// Set BPM values /////

		// Fix to the nTime var (so it can pass next IF statement) in case the last tempo point is selected 
		if ( selectedTempoPoints[i] + 1 == CountTempoTimeSigMarkers(NULL))
			nTime = cTime+timeDiff + 1;
		
				
		//IF statement acts as a safety net for illogical calculations and reaper dislikes (it hates BPM over 960)
		// The flow of changes must go from the later points to earliner (one of the reasons is the bug in the API that screws things up when using musical position
		// instead of time postion).
		if (pNewBPM > 0.001 && pNewBPM <=960 && cNewBPM > 0.001 && cNewBPM <= 960 && (cTime+timeDiff) > pTime && (cTime+timeDiff) < nTime && ppPossible == true)
		{	
			// Set current point
			SetTempoTimeSigMarker(NULL, selectedTempoPoints[i], cTime, -1, -1, cNewBPM, cNum, cDen, cShape);
			
			// Set previous point
			SetTempoTimeSigMarker(NULL, selectedTempoPoints[i] - 1, pTime , -1, -1, pNewBPM, pNum, pDen, pShape);
						
			// Readjust points behind previous point
			if (ppDo == true)
			{	
				changeDirection = 1;
				for (int x = selectedTempoPoints[i] - 2 ; x >= pp ; --x)
				{
					GetTempoTimeSigMarker(NULL, x, &ppTime, NULL, NULL, &ppBPM, &ppNum, &ppDen, &ppShape);
					SetTempoTimeSigMarker(NULL, x, ppTime, -1, -1,(ppBPM - changeDirection * (pNewBPM - pBPM)), ppNum, ppDen, ppShape);
					changeDirection = changeDirection * -1;
				}
			}
			
			// Fix for partial measures		
			if (partialTime.size() != 0)
			{
				for (int x = partialTime.size() - 1; x >=0; --x)
				{
					GetTempoTimeSigMarker(NULL, partialID[x], NULL, NULL, NULL, &cBPM, &cNum, &cDen, &cShape);
					SetTempoTimeSigMarker(NULL, partialID[x], partialTime[x], -1, -1, cBPM, cNum, cDen, cShape);
				}
			}
		}
		else
			++skipped;
	} // end of the for loop that goes through all of the selected points

	// Restore preferences back to the previous state
	*(int*)GetConfigVar("envclicksegmode") = envClickSegMode;

	// DONE!
	UpdateTimeline();
	Undo_EndBlock2 (NULL, __LOCALIZE("Move selected tempo points","sws_undo"), UNDO_STATE_ALL);

	// but not yet...warn user if some points weren't moved
	static bool g_pointsNoMovedNoWarning;
	
	if (selectedTempoPoints.size() > 1 && skipped != 0 && (!g_pointsNoMovedNoWarning))
	{
		char buffer[1024] = {0};
		if (skipped == 1)
			_snprintf(buffer, sizeof(buffer), __LOCALIZE("%d point wasn't moved because some of the required BPM manipulations are impossible. Would you like to be warned if it happens again?" , "sws_mbox"), skipped);
		else
			_snprintf(buffer, sizeof(buffer), __LOCALIZE("%d points weren't moved because some of the required BPM manipulations are impossible. Would you like to be warned again if the same thing happens?" , "sws_mbox"), skipped);
		
		int userAnswer = ShowMessageBox(buffer, __LOCALIZE("SWS - Warning","sws_mbox"), 4);
		if (userAnswer == 7)
			g_pointsNoMovedNoWarning = true;
	
	}
};

/*
bool IsSelectAdjustTempoPointsVisible(COMMAND_T* = NULL)
{
	return g_selectAdjustTempoPointsDialog;
};
void BRSelectAdjustTempoPointsAction(COMMAND_T* = NULL)
{	
	static HWND hwnd = CreateDialog (g_hInst, MAKEINTRESOURCE(IDD_BR_SELECT_ADJUST_TEMPO), g_hwndParent, BRSelectAdjustTempoPointsProc);

	if (g_selectAdjustTempoPointsDialog)
	{	
		g_selectAdjustTempoPointsDialog = false;
		KillTimer(hwnd, 1);	
		ShowWindow(hwnd, SW_HIDE);
	}

	else
	{
		g_selectAdjustTempoPointsDialog = true;
		
		SetTimer(hwnd, 1, 500, NULL);
		ShowWindow(hwnd, SW_SHOW);
		SetFocus(hwnd);
	}
};

*/


//////////////////////////////////////////////////////////////////////////////////////////////////////////
//								  	VARIOUS FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////////////////

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
	vector<double> markerPositions;
	bool region, prevRegion;
	double mPos, prevMPos;
	int current, previous, markerId, markerCount = 0, i=0;
	
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
	
	// Check if there are existing tempo markers after the first project marker
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
	
		int exceed = 0;
		
		// Set first tempo marker with time signature
		double length = markerPositions[1] - markerPositions[0];
		double bpm = (240 * num / den) / (measure * length);
		if (bpm > 960)
			++exceed;
		SetTempoTimeSigMarker (NULL, -1, markerPositions[0], -1, -1, bpm, num, den, NULL);

		// Set the rest of the tempo markers. 
		for (int i = 1; i+1 < markerCount; ++i)
		{ 
			length = markerPositions[i+1] - markerPositions[i];
			bpm = (60 * 4/den*num) / (measure * length);
			if (bpm > 960)
				++exceed;
			SetTempoTimeSigMarker (NULL, -1, markerPositions[i], -1, -1, bpm, 0, 0, NULL);
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
	// Warn user if there were tempo markers created with a BPM over 960
	if (exceed !=0)
	{
		char buffer[1024] = {0};
		if (exceed == 1)
			_snprintf(buffer, sizeof(buffer), __LOCALIZE("%d of created tempo markers has a BPM over 960. If you try to edit it, it will revert back to 960 or lower.\n\nIt is recommended that you undo, edit project markers and try again." ,"sws_DLG_166"), exceed);
		else
			_snprintf(buffer, sizeof(buffer), __LOCALIZE("%d of created tempo markers have a BPM over 960. If you try to edit them, they will revert back to 960 or lower.\n\nIt is recommended that you undo, edit project markers and try again." ,"sws_DLG_166"), exceed);
		ShowMessageBox(buffer,__LOCALIZE("SWS - Warning", "sws_mbox"), 0);
	}

};


vector<int> BRGetSelectedTempoPoints()
{	
	vector<int> selectedTempoPoints;
	LineParser lp(false);
	int ID = -1;
		
	TrackEnvelope* envelope = SWS_GetTrackEnvelopeByName(CSurf_TrackFromID(0, false), "Tempo map" );
	char* envState = GetSetObjectState(envelope, "");
	char* token = strtok(envState, "\n");
	
	while(token != NULL)
	{
		lp.parse(token);
		if (strcmp(lp.gettoken_str(0),  "PT") == 0)
		{	
			++ID;
			if (lp.gettoken_int(5) == 1)
				selectedTempoPoints.push_back(ID);
		}
		token = strtok(NULL, "\n");	
	}
	FreeHeapPtr(envState);
	
	return selectedTempoPoints;
};

/*
void BRSelectTempoPoints (int mode, // 0 to clear and 1 to invert selection, 2 to set new and 3 to add to existing selection
						  int time,									// 0 for all, 1 for time selection, 2 to exclude time selection
						  int BPM, double BpmStart, double BpmEnd,	// 0 to ignore BPM, if 1 BpmStart and BpmEnd need to be specified
						  int shape,								// 0 for all, 1 for square, 2 for linear
						  int sig, int sigNum, int sigDen,			// 0 to ignore sig, if 1 sigNum and sigDen need to be specified
						  int type)									// 0 for all, 1 for tempo markers only, 2 for time signature only
{	
	string newState;
	double qTime, qBPM, tStart, tEnd;
	int qShape, qSig, qSelected, qPartial;

	// Get time selection
	if (time != 0)
		GetSet_LoopTimeRange2 (NULL, false, false, &tStart, &tEnd, false);
	
	// Get tempo chunk
	TrackEnvelope* envelope = SWS_GetTrackEnvelopeByName(CSurf_TrackFromID(0, false), "Tempo map" );
	char* envState = GetSetObjectState(envelope, "");
	char* token = strtok(envState, "\n");
	


	// lp.getnumtokens()

	// Loop tempo chunk searching for tempo markers
	LineParser lp(false);
	while(token != NULL)
	{
		lp.parse(token);

		if (strcmp(lp.gettoken_str(0),  "PT") == 0)
		{	
			



		}
		
		else
		{
			
				newState.append(token);
				newState.append("\n");
			
		}
		token = strtok(NULL, "\n");	
	}
	
	
	
	// Update tempo chunk
	newState[newState.size() - 1] = '\0';
	const char * newEnvState = newState.c_str();
	
	FreeHeapPtr(envState);	
};

*/


//////////////////////////////////////////////////////////////////////////////////////////////////////////
//								  	DIALOG BOX PROCEDURES
//////////////////////////////////////////////////////////////////////////////////////////////////////////

// Lots of code shamelessly stolen from a lot 
// of places. :) My excuse is one of the educational nature :) If 
// somebody finds anything that sucks here - please do correct.
WDL_DLGRET BRConvertProjectMarkersToTempoMarkersProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
				{
					g_convertProjectMarkersDialog = false;
					ShowWindow(hwnd, SW_HIDE);
				}	
				break;
			}
		}
		break;
		
		case WM_DESTROY:
			SaveWindowPos(hwnd, cWndPosKey);
			break; 
	}
	return 0;
};

/*
WDL_DLGRET BRSelectAdjustTempoPointsProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, uMsg, wParam, lParam))
		return r;

	const char cWndPosKey[] = "AdjSelTemp WndPos";
	switch(uMsg)
	{
		case WM_INITDIALOG :
		{
			// SELECT MARKERS
			int eTimeRange = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_TIME_RANGE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Project","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TIME_RANGE, CB_SETITEMDATA, eTimeRange, 0);
			eTimeRange = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_TIME_RANGE,CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Time selection","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TIME_RANGE, CB_SETITEMDATA, eTimeRange, 1);
			eTimeRange = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_TIME_RANGE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Ignore time selection","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TIME_RANGE, CB_SETITEMDATA, eTimeRange, 2);
			
			int eShape = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_SHAPE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("All","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_SHAPE, CB_SETITEMDATA, eShape, 0);
			eShape = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_SHAPE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Square","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_SHAPE, CB_SETITEMDATA, eShape, 1);
			eShape = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_SHAPE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Linear","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_SHAPE, CB_SETITEMDATA, eShape, 2);

			int eType = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_TYPE_DEF, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("All","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TYPE_DEF, CB_SETITEMDATA, eType, 0);
			eType = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_TYPE_DEF, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Tempo markers","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TYPE_DEF, CB_SETITEMDATA, eType, 1);
			eType = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_TYPE_DEF, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Time signature markers","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TYPE_DEF, CB_SETITEMDATA, eType, 2);

			int eBpm = 1, eSig = 0, eAdd = 0;
			double BpmStart = 100, BpmEnd = 150;
			int sigNum = 4, sigDen = 4;

			char eBpmStart[128] = {0}, eBpmEnd[128] = {0}, eSigNum[128] = {0}, eSigDen[128] = {0} ;				
			sprintf(eBpmStart, "%.2lf", BpmStart);
			sprintf(eBpmEnd, "%.2lf", BpmEnd);
			sprintf(eSigNum, "%d", sigNum);
			sprintf(eSigDen, "%d", sigDen);

			eTimeRange = eShape = eType = 0;
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TIME_RANGE, CB_SETCURSEL, eTimeRange, 0);
			SendDlgItemMessage(hwnd, IDC_BR_SEL_SHAPE, CB_SETCURSEL, eShape, 0);
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TYPE_DEF, CB_SETCURSEL, eType, 0);
			
			CheckDlgButton(hwnd, IDC_BR_SEL_BPM, !!eBpm);
			CheckDlgButton(hwnd, IDC_BR_SEL_SIG, !!eSig);
			CheckDlgButton(hwnd, IDC_BR_SEL_ADD, !!eAdd);

			SetDlgItemText(hwnd, IDC_BR_SEL_BPM_START, eBpmStart);
			SetDlgItemText(hwnd, IDC_BR_SEL_BPM_END, eBpmEnd);
			SetDlgItemText(hwnd, IDC_BR_SEL_SIG_NUM, eSigNum);
			SetDlgItemText(hwnd, IDC_BR_SEL_SIG_DEN, eSigDen);
				EnableWindow(GetDlgItem(hwnd, IDC_BR_SEL_SIG_NUM), eSig);
				EnableWindow(GetDlgItem(hwnd, IDC_BR_SEL_SIG_DEN), eSig);

			
				
			// MODIFY MARKERS...
			char eBpmFirst[128] = {0}, eBpmLast[128] = {0}, eBpmCursor[128] = {0};
			double bpmFirst = 0, bpmCursor = 0, bpmLast = 0;
			sprintf(eBpmFirst, "%d", bpmFirst);
			sprintf(eBpmCursor, "%d", bpmCursor);
			sprintf(eBpmLast, "%d", bpmLast);
			
			SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_1, eBpmFirst);
			SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_2, eBpmCursor);
			SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_3, eBpmLast);

			CheckDlgButton(hwnd, IDC_BR_ADJ_BPM_VAL_ENB, 1);
			

			
			
			
		}
		break;
        
		case WM_COMMAND :
		{
            switch(LOWORD(wParam))
            {
				case IDOK:
				{	
					BRSelectTempoPoints (0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
				}
				break;

				case IDC_BR_SEL_BPM:
				{				
					int eBPM = IsDlgButtonChecked(hwnd, IDC_BR_SEL_BPM);
					EnableWindow(GetDlgItem(hwnd, IDC_BR_SEL_BPM_START), eBPM);
					EnableWindow(GetDlgItem(hwnd, IDC_BR_SEL_BPM_END), eBPM);
				}
				break;
				
				case IDC_BR_SEL_SIG:
				{				
					int eSig = IsDlgButtonChecked(hwnd, IDC_BR_SEL_SIG);
					EnableWindow(GetDlgItem(hwnd, IDC_BR_SEL_SIG_NUM), eSig);
					EnableWindow(GetDlgItem(hwnd, IDC_BR_SEL_SIG_DEN), eSig);
				}
				break;

				case IDCANCEL:
				{
					KillTimer(hwnd, 1);
					g_selectAdjustTempoPointsDialog = false;
					ShowWindow(hwnd, SW_HIDE);
				}				
					break;
			} 
		}
		break;

		case WM_TIMER:
		{  
			// Get number of selected points
			vector<int> selectedPoints = BRGetSelectedTempoPoints();
			char pointCount[1024] = {0};
			_snprintf(pointCount, sizeof(pointCount), __LOCALIZE("SWS Select and adjust tempo markers (%d of %d points selected)","sws_DLG_167") , selectedPoints.size(), CountTempoTimeSigMarkers(NULL) );
			SetWindowText(hwnd,pointCount);

			// Set current BPM info
			double bpmFirst, bpmCursor, bpmLast;
			char eBpmFirst[128] = {0}, eBpmCursor[128] = {0}, eBpmLast[128] = {0}, eBpmFirstChk[128] = {0}, eBpmCursorChk[128] = {0}, eBpmLastChk[128] = {0};
			
			if (selectedPoints.size() != 0)
			{	
				GetTempoTimeSigMarker(NULL, selectedPoints[0], NULL, NULL, NULL, &bpmFirst, NULL, NULL, NULL);
				GetTempoTimeSigMarker(NULL, selectedPoints.back(), NULL, NULL, NULL, &bpmLast, NULL, NULL, NULL);
				sprintf(eBpmFirst, "%.2lf", bpmFirst);
				sprintf(eBpmLast, "%.2lf", bpmLast);
			}
			else
			{
				bpmFirst = bpmLast = 0;
				sprintf(eBpmFirst, "%d", bpmFirst);
				sprintf(eBpmLast, "%d", bpmLast);
			}
			bpmCursor = TimeMap2_GetDividedBpmAtTime(NULL, GetCursorPosition());
			sprintf(eBpmCursor, "%.2lf", bpmCursor);

			GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_1, eBpmFirstChk, 128);
			GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_2, eBpmCursorChk, 128);
			GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_3, eBpmLastChk, 128);
			
			if (atof(eBpmFirst) != atof(eBpmFirstChk) || atof(eBpmCursor) != atof(eBpmCursorChk) || atof(eBpmLast) != atof(eBpmLastChk))
			{
				SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_1, eBpmFirst);
				SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_2, eBpmCursor);
				SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_3, eBpmLast);
			}
		}
		break;

		case WM_DESTROY:
			SaveWindowPos(hwnd, cWndPosKey);
			break; 
	}
	return 0;
};

*/


//////////////////////////////////////////////////////////////////////////////////////////////////////////
//										EXTENSIONS STUFF
//////////////////////////////////////////////////////////////////////////////////////////////////////////


//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "SWS/BR: Convert project markers to tempo markers" },			"SWS_BRCONVERTMARKERSTOTEMPO",			BRConvertProjectMarkersToTempoMarkersAction, NULL, 0, IsConvertProjectMarkerVisible },
	{ {	DEFACCEL, "SWS/BR: Split selected items at tempo markers" },				"SWS_BRSPLITSELECTEDTEMPO",				BRSplitSelectedItemsAtTempoMarkers, },
	
	{ {	DEFACCEL, "SWS/BR: Move selected tempo points forward" },					"SWS_BRMOVETEMPOFORWARD",				BRMoveSelTempoPoint, NULL, 5},
	{ {	DEFACCEL, "SWS/BR: Move selected tempo points back" },						"SWS_BRMOVETEMPOBACK",					BRMoveSelTempoPoint, NULL, -5},
	
	{ {	DEFACCEL, "SWS/BR: Move selected tempo points forward 0.1 ms" },			"SWS_BRMOVETEMPOFORWARD01",				BRMoveSelTempoPoint, NULL, 2},
	{ {	DEFACCEL, "SWS/BR: Move selected tempo points forward 1 ms" },				"SWS_BRMOVETEMPOFORWARD1",				BRMoveSelTempoPoint, NULL, 1},
	{ {	DEFACCEL, "SWS/BR: Move selected tempo points forward 10 ms"},				"SWS_BRMOVETEMPOFORWARD10",				BRMoveSelTempoPoint, NULL, 10},
	{ {	DEFACCEL, "SWS/BR: Move selected tempo points forward 100 ms"},				"SWS_BRMOVETEMPOFORWARD100",			BRMoveSelTempoPoint, NULL, 100},
	{ {	DEFACCEL, "SWS/BR: Move selected tempo points forward 1000 ms" },			"SWS_BRMOVETEMPOFORWARD1000",			BRMoveSelTempoPoint, NULL, 1000},
	{ {	DEFACCEL, "SWS/BR: Move selected tempo points back 0.1 ms"},				"SWS_BRMOVETEMPOBACK01",				BRMoveSelTempoPoint, NULL, -2},
	{ {	DEFACCEL, "SWS/BR: Move selected tempo points back 1 ms"},					"SWS_BRMOVETEMPOBACK1",					BRMoveSelTempoPoint, NULL, -1},
	{ {	DEFACCEL, "SWS/BR: Move selected tempo points back 10 ms"},					"SWS_BRMOVETEMPOBACK10",				BRMoveSelTempoPoint, NULL, -10},
	{ {	DEFACCEL, "SWS/BR: Move selected tempo points back 100 ms"},				"SWS_BRMOVETEMPOBACK100",				BRMoveSelTempoPoint, NULL, -100},
	{ {	DEFACCEL, "SWS/BR: Move selected tempo points back 1000 ms"},				"SWS_BRMOVETEMPOBACK1000",				BRMoveSelTempoPoint, NULL, -1000},
	
//	{ {	DEFACCEL, "SWS/BR: Select and adjust tempo markers" },						"SWS_BRADJUSTSELTEMPO",					BRSelectAdjustTempoPointsAction, NULL, 0, IsSelectAdjustTempoPointsVisible },
	

	{ {}, LAST_COMMAND, }, // Denote end of table
};

//!WANT_LOCALIZE_1ST_STRING_END


int BreederInit()
{
	SWSRegisterCommands(g_commandTable);
	return 1;
};