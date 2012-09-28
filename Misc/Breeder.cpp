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

//Globals
static bool g_convertProjectMarkersDialog = false;
static bool g_selectAdjustTempoPointsDialog = false;

//Prototypes
void BRConvertProjMarkersToTempo (int num, int den, int measure, int timeSelection, int removeMarkers);
void BRSelectTempoMarkers (int mode, int timeSel, int bpm, double bpmStart, double bpmEnd, int shape, int sig, int sigNum, int sigDen, int type);
int BRModifySelectedTempoPoints (int mode, double bpmVal, double bpmPerc, int shape);
vector<int> BRGetSelectedTempoPoints ();

WDL_DLGRET BRConvertProjMarkersToTempoProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
WDL_DLGRET BRSelAdjTempoProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void BRUpdateCurrentBpm (HWND hwnd, vector<int> selectedPoints);
void BRUpdateTargetBpm (HWND hwnd, int doFirst, int doCursor, int doLast);
void BRSelectTempoMarkersCase (HWND hwnd);
void BRModifyTempoMarkersCase (HWND hwnd);

																  
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//							ACTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BRSplitSelItemsAtTempo (COMMAND_T* ct)
{
	// Get selected items 	
	WDL_TypedBuf<MediaItem*> selectedItems;
	SWS_GetSelectedMediaItems(&selectedItems);
	if (selectedItems.GetSize() == 0)
		return;

	// Loop through selected items 
	Undo_BeginBlock2(NULL);
	
	for (int i = 0; i < selectedItems.GetSize(); ++i)
	{	
		MediaItem* newItem = selectedItems.Get()[i];
		double iStart = GetMediaItemInfo_Value(newItem, "D_POSITION");
		double iEnd = iStart + GetMediaItemInfo_Value(newItem, "D_LENGTH");
		double tPos = iStart - 1;
		
		// Split item currently in the loop
		while (true)
		{
			if ((newItem = SplitMediaItem(newItem, tPos)) == 0 )
				newItem = selectedItems.Get()[i];
		
			tPos = TimeMap2_GetNextChangeTime(NULL, tPos);
			if (tPos > iEnd || tPos == -1 )
				break;
		}
	}
	UpdateArrange();
	
	Undo_EndBlock2 (NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);
};

bool IsConvertProjMarkerToTempoVisible (COMMAND_T* = NULL)
{
	return g_convertProjectMarkersDialog;
};
void BRConvertProjMarkersToTempoAction (COMMAND_T* = NULL)
{
	// Create dialog on the first run...
	static HWND hwnd = CreateDialog (g_hInst, MAKEINTRESOURCE(IDD_BR_MARKERS_TO_TEMPO), g_hwndParent, BRConvertProjMarkersToTempoProc);

	// Check its state and hide/show it accordingly
	if (g_convertProjectMarkersDialog)
	{	
		g_convertProjectMarkersDialog = false;
		KillTimer(hwnd, 1);	
		ShowWindow(hwnd, SW_HIDE);
	}
	
	else
	{		
		// Detect timebase
		bool cancel = false;
		if (*(int*)GetConfigVar("itemtimelock") != 0)
		{	
			int answer = MessageBox(g_hwndParent, __LOCALIZE("Project timebase is not set to time. Do you want to set it now?","sws_DLG_166"), __LOCALIZE("SWS - Warning","sws_mbox"), MB_YESNOCANCEL);
			if (answer == 6)
				*(int*)GetConfigVar("itemtimelock") = 0;
			if (answer == 2)
				cancel = true;
		}	
		
		if (!cancel)
		{
			g_convertProjectMarkersDialog = true;
			SetTimer(hwnd, 1, 100, NULL);
			ShowWindow(hwnd, SW_SHOW);
			SetFocus(hwnd);
		}
	}
};

void BRMoveSelTempo (COMMAND_T* t) 
{
	// Get amount of movement to be performed on the selected tempo points
	double timeDiff = (int)t->user;
	
	if (t->user == 5 || t->user == -5)
		timeDiff = 1 / GetHZoomLevel() * timeDiff / 5;
	
	else if (t->user == 2 || t->user == -2)
		timeDiff = 0.0001 * timeDiff / 2;
	
	else
		timeDiff = timeDiff/1000;
	
	// Get selected points' ID, remove first tempo marker in the project from the vector and check vector size
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
			}
		
			// If current point's shape is linear...
			if (cShape == 1)
			{
				measureLengthCN = (cEffDen * (nTime - cTime) * (cBPM + nBPM)) / (480 * cEffNum);
				cNewBPM = (480 * cEffNum * measureLengthCN) / (cEffDen * (nTime - cTime - timeDiff)) - nBPM;
			}

			// Fix for the last tempo point
			if (selectedTempoPoints[i] + 1 == CountTempoTimeSigMarkers(NULL)) 
					cNewBPM = cBPM;
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
			}
				
			// If current point's shape is linear...
			if (cShape == 1)
			{
				measureLengthCN = (cEffDen * (nTime - cTime) * (cBPM + nBPM)) / (480 * cEffNum);
				cNewBPM = (480 * cEffNum * measureLengthCN) / (cEffDen * (nTime - cTime - timeDiff)) - nBPM;				
			}
			
			// Fix for the last tempo point
			if (selectedTempoPoints[i] + 1 == CountTempoTimeSigMarkers(NULL)) 
					cNewBPM = cBPM;

			pNewBPM = (480 * pEffNum * measureLengthPC) / (pEffDen * (cTime - pTime + timeDiff)) - cNewBPM;
		}

		
		///// Check points behind previous point /////
		double ppTime, ppBPM;
		int ppNum, ppDen;
		int pp = selectedTempoPoints[i] - 2, changeDirection = 1;
		bool ppShape, ppDo = true, ppPossible = true; 
		
		// Check that the point behind previous point exists
		if (pp < 0)
			ppDo = false;

		else
		{
			// Get the first point behind previous point - If square, change ppDo var so later we can skip the process of adjusting previous points
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
							
							// Add any point with time signature to vector for later correction of partial points
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
				pp++; //correct ID due to the nature of a while loop
			}
		}

		
		///// Set BPM values /////

		// Fix to the nTime var (so it can pass the next IF statement) in case the last tempo point is selected 
		if (selectedTempoPoints[i] + 1 == CountTempoTimeSigMarkers(NULL))
			nTime = cTime+timeDiff + 1;
		
				
		// IF statement acts as a safety net for illogical calculations and reaper dislikes (it hates BPM over 960)
		// The flow of changes must go from the later points to earlier (one of the reasons is the bug in the API that screws things up when using musical position
		// instead of time position).
		if (pNewBPM >= 0.001 && pNewBPM <=960 && cNewBPM >= 0.001 && cNewBPM <= 960 && (cTime+timeDiff) > pTime && (cTime+timeDiff) < nTime && ppPossible == true)
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
					SetTempoTimeSigMarker(NULL, x, ppTime, -1, -1, (ppBPM - changeDirection * (pNewBPM - pBPM)), ppNum, ppDen, ppShape);
					changeDirection = changeDirection * -1;
				}
			}
			
			// Fix for partial measures		
			if (partialTime.size() != 0)
			{
				for (size_t x = partialTime.size() - 1; x > 0; --x)
				{
					GetTempoTimeSigMarker(NULL, partialID[x], NULL, NULL, NULL, &cBPM, &cNum, &cDen, &cShape);
					SetTempoTimeSigMarker(NULL, partialID[x], partialTime[x], -1, -1, cBPM, cNum, cDen, cShape);
				}
				GetTempoTimeSigMarker(NULL, partialID[0], NULL, NULL, NULL, &cBPM, &cNum, &cDen, &cShape);
				SetTempoTimeSigMarker(NULL, partialID[0], partialTime[0], -1, -1, cBPM, cNum, cDen, cShape);
			}
		}
		
		else
			++skipped;
	
	} // end of the for loop that goes through all of the selected points

	// Restore preferences back to the previous state
	*(int*)GetConfigVar("envclicksegmode") = envClickSegMode;

	// DONE!
	UpdateTimeline();
	Undo_EndBlock2 (NULL, SWS_CMD_SHORTNAME(t), UNDO_STATE_ALL);

	// but not yet...warn user if some points weren't moved
	static bool g_pointsNoMovedNoWarning;
	
	if (selectedTempoPoints.size() > 1 && skipped != 0 && (!g_pointsNoMovedNoWarning))
	{
		char buffer[512] = {0};
		_snprintf(buffer, sizeof(buffer), __LOCALIZE_VERFMT("%d of the selected points can't be moved because some of the required BPM manipulations are impossible. Would you like to be warned if it happens again?", "sws_mbox"), skipped);
			
		int userAnswer = ShowMessageBox(buffer, __LOCALIZE("SWS - Warning", "sws_mbox"), 4);
		if (userAnswer == 7)
			g_pointsNoMovedNoWarning = true;
	
	}
};

bool IsSelAdjTempoVisible (COMMAND_T* = NULL)
{
	return g_selectAdjustTempoPointsDialog;
};
void BRSelAdjTempoAction (COMMAND_T* = NULL)
{
	static HWND hwnd = CreateDialog (g_hInst, MAKEINTRESOURCE(IDD_BR_SELECT_ADJUST_TEMPO), g_hwndParent, BRSelAdjTempoProc);

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

void BRMoveEditCurSelEnvPoint (COMMAND_T* t)
{
	// Get active envelope
	TrackEnvelope* envelope = GetSelectedTrackEnvelope(NULL);
	char* envState = GetSetObjectState(envelope, "");
	char* token = strtok(envState, "\n");
	
	LineParser lp(false);	
	double ppTime = 0, pTime = 0, cTime = 0, nTime = 0;
	bool found = false;
	
	// Find next point
	if (t->user == 1 && envelope != 0)
	{
		while(token != NULL)
		{
			lp.parse(token);
			if (strcmp(lp.gettoken_str(0),  "PT") == 0)
			{	
				cTime = lp.gettoken_float(1);
				
				if (cTime > GetCursorPositionEx(NULL))
				{
					found = true;
					token = strtok(NULL, "\n");
					lp.parse(token);
					if (strcmp(lp.gettoken_str(0),  "PT") == 0)
						nTime = lp.gettoken_float(1);
					else
						nTime = cTime + 0.001;
					break;
				}
				pTime = cTime;
			}
			token = strtok(NULL, "\n");	
		}
	}
	
	// Find previous point
	if (t->user == -1 && envelope != 0)
	{
		while(token != NULL)
		{
			lp.parse(token);
			if (strcmp(lp.gettoken_str(0),  "PT") == 0)
			{	
				cTime = lp.gettoken_float(1);
				
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
	
	// Yes, this is a hack...however, I feel it has more advantages over modifying an envelope chunk
	// because this way there is no need to parse and set the whole chunk but only find a target point 
	// and then, via manipulation of the preferences and time selection, make reaper do the rest
	// Using PreventUIRefresh is crucial to keep the user unaware of what is happening in the background
	if (found && envelope != 0)
	{
		Undo_BeginBlock2(NULL);
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
		
		// Restore preferences and set edit cursor's new position
		*(int*)GetConfigVar("envclicksegmode") = envClickSegMode;
		SetEditCurPos(cTime, true, false);

		Undo_EndBlock2 (NULL, SWS_CMD_SHORTNAME(t), UNDO_STATE_ALL);
		PreventUIRefresh (-1);
	}
	
	FreeHeapPtr(envState);	
};




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//							VARIOUS FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void BRConvertProjMarkersToTempo (int num, int den, int measure, int timeSelection, int removeMarkers)
{
	// Get time selection
	double tStart, tEnd;
	if (timeSelection == 1)
		GetSet_LoopTimeRange2 (NULL, false, false, &tStart, &tEnd, false);
		
	// Get project markers
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
			// than once. This should prevent those duplicate values from being written into the vector.
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
			MessageBox(g_hwndParent, __LOCALIZE("Not enough project markers in the project to perform conversion.", "sws_DLG_166"), __LOCALIZE("SWS - Error", "sws_mbox"), MB_OK);	
			return;
		}
		if (timeSelection == 1)
		{	
			MessageBox(g_hwndParent, __LOCALIZE("Not enough project markers in the time selection to perform conversion.", "sws_DLG_166"), __LOCALIZE("SWS - Error", "sws_mbox"), MB_OK);	
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
		ShowMessageBox(__LOCALIZE("Some of the created tempo markers have a BPM over 960. If you try to edit them, they will revert back to 960 or lower.\n\nIt is recommended that you undo, edit project markers and try again.", "sws_DLG_166"),__LOCALIZE("SWS - Warning", "sws_mbox"), 0);
};

vector<int> BRGetSelectedTempoPoints ()
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

void BRSelectTempoMarkers (int mode, int timeSel, int bpm, double bpmStart, double bpmEnd, int shape, int sig, int sigNum, int sigDen, int type)
{
	/*
	mode ---> 0 to clear and 1 to invert selection, 2 to set new and 3 to add to existing selection
	timeSel ---> 0 for all, 1 for time selection, 2 to exclude time selection
	bpm ----> 0 to ignore BPM, if 1 BpmStart and BpmEnd need to be specified
	shape --> 0 for all, 1 for square, 2 for linear
	sig ----> 0 to ignore sig, if 1 sigNum and sigDen need to be specified
	type ---> 0 for all, 1 for tempo markers only, 2 for time signature only
	*/

	stringstream newState;
	double qTime, qBpm, tStart, tEnd;
	int qShape, qSig, qSelected, qPartial;

	// Get time selection
	if (timeSel != 0)
		GetSet_LoopTimeRange2 (NULL, false, false, &tStart, &tEnd, false);
	
	// Get tempo chunk
	TrackEnvelope* envelope = SWS_GetTrackEnvelopeByName(CSurf_TrackFromID(0, false), "Tempo map" );
	char* envState = GetSetObjectState(envelope, "");
	char* token = strtok(envState, "\n");

	// Loop tempo chunk searching for tempo markers
	LineParser lp(false);
	while(token != NULL)
	{
		lp.parse(token);
		if (strcmp(lp.gettoken_str(0),  "PT") == 0)
		{	
			qTime = lp.gettoken_float(1);
			qBpm = lp.gettoken_float(2);
			qShape = lp.gettoken_int(3);
			qSig = lp.gettoken_int(4);
			qSelected = lp.gettoken_int(5);
			qPartial = lp.gettoken_int(6);
			
			
			// Clear selected points
			if (mode == 0)
				qSelected = 0;

			// Invert selected points
			if (mode == 1)
			{	
				if (qSelected == 1)
					qSelected = 0;
				else
					qSelected = 1;
			}
			
			// Select points by criteria
			if (mode > 1)
			{
				bool selectPt = true;
				
				// Check time
				if (selectPt)
				{
					if (timeSel == 0)
						selectPt = true;
					
					if (timeSel == 1)
					{
						if (qTime >= tStart && qTime <= tEnd)
							selectPt = true;
						else
							selectPt = false;
					}
					
					if (timeSel == 2)
					{
						if (qTime >= tStart && qTime <= tEnd)
							selectPt = false;
						else
							selectPt = true;
					}
				}

				// Check BPM
				if (selectPt)
				{
					if (bpm == 0)
						selectPt = true;
					
					if (bpm == 1)
					{
						if(qBpm >= bpmStart && qBpm <= bpmEnd )
							selectPt = true;
						else
							selectPt = false;
					}
				}

				// Check shape
				if (selectPt)
				{
					if (shape == 0)
						selectPt = true;

					if (shape == 1)
					{
						if (qShape == 1)
							selectPt = true;
						else
							selectPt = false;
					}

					if (shape == 2)
					{
						if (qShape == 0)
							selectPt = true;
						else
							selectPt = false;
					}					
				}

				// Check time signature
				if (selectPt)
				{
					if (sig == 0)
						selectPt = true;
					
					if (sig == 1)
					{
						int effNum, effDen;
						TimeMap_GetTimeSigAtTime(NULL, qTime, &effNum, &effDen, NULL);
						if (sigNum == effNum && sigDen == effDen)
							selectPt = true;
						else
							selectPt = false;
					}
				}

				// Check type
				if (selectPt)
				{
					if (type == 0)
						selectPt = true;

					if (type == 1)
					{
						if (qSig == 0)
							selectPt = true;
						else
							selectPt = false;
					}

					if (type == 2)
					{
						if (qSig == 0)
							selectPt = false;
						else
							selectPt = true;
					}
				}

				// If mode is "add to selection", no matter the upper criteria selected point stays selected
				if (mode == 3)
				{
					if (qSelected == 1)
						selectPt = true;
				}

				// Finally select or unselect point based on upper criteria
				if (selectPt)
					qSelected = 1;
				else
					qSelected = 0;
			}

			// Update stringstream
			newState << "PT" << " " << setprecision(16) 
					 << qTime << " " 
					 << qBpm << " " 
					 << qShape << " " 
					 << qSig << " " 
					 << qSelected << " " 
					 << qPartial << endl;			
		}
		
		else
			newState << token << endl;
			
		token = strtok(NULL, "\n");	
	}
	
	// Update tempo chunk
	string newEnvState = newState.str();
	GetSetObjectState(envelope, newEnvState.c_str());
	FreeHeapPtr(envState);
};

int BRModifySelectedTempoPoints (int mode, double bpmVal, double bpmPerc, int shape)
{
	/*
	mode ---> 0 to add bpmVal, 1 to calculate percentage using bpmPerc
	shape --> 0 to ignore, 1 to invert, 2 for linear, 3 for square
	*/
	vector<int> selectedTempoPoints = BRGetSelectedTempoPoints();

	// Temporary change of preferences - prevent reselection of points in time selection
	int envClickSegMode = *(int*)GetConfigVar("envclicksegmode");
	*(int*)GetConfigVar("envclicksegmode") = 1;
	
	// Loop through all of the selected tempo markers and modify them
	int exceed = 0;
	for(size_t i = 0; i < selectedTempoPoints.size(); ++i)
	{
		double cTime, cBPM, nTime, nBPM;;
		int cNum, cDen;
		bool cShape; 
		GetTempoTimeSigMarker(NULL, selectedTempoPoints[i], &cTime, NULL, NULL, &cBPM, &cNum, &cDen, &cShape);
		
		// Calculate BPM
		if (mode == 0)
			nBPM = cBPM + bpmVal;
		if (mode == 1)
			nBPM = (bpmPerc / 100 + 1) * cBPM;
		
		// Check if new BPM is legal
		bool possible = true;
		if (nBPM < 0.001 || nBPM > 960)
			possible = false;
		
		// Get new shape
		if (shape == 2)
			cShape = 1;
		if (shape == 3)
			cShape = 0;
		if (shape == 1)
		{
			bool change = false;
			if (cShape == 1 && !change)
			{
				cShape = 0;
				change = true;
			}
			if (cShape == 0 && !change)
				cShape = 1;
		}
		
		// Calculate new time of a modified markers - used to fix partial measures
		if (cNum != 0 && selectedTempoPoints[i] != 0)
		{	
			double pTime, pBPM;
			bool pShape; 
			GetTempoTimeSigMarker(NULL, selectedTempoPoints[i] -1, &pTime, NULL, NULL, &pBPM, NULL, NULL, &pShape);
			
			if ( pShape == 0)
				nTime = cTime;
			else
			{
				int effNum, effDen;
				double measureLength;
				TimeMap_GetTimeSigAtTime(NULL, pTime, &effNum, &effDen, NULL);

				measureLength =  (effDen * (cTime - pTime) * (pBPM + cBPM)) / (480 * effNum);
				nTime = (480 * effNum * measureLength) / (effDen * (pBPM + nBPM)) + pTime;
			}
		}
		
		if (possible)
		{
			SetTempoTimeSigMarker(NULL, selectedTempoPoints[i], cTime, NULL, NULL, nBPM, cNum, cDen, cShape);
			// Fix partial measures
			if (cNum != 0 && selectedTempoPoints[i] != 0)
				SetTempoTimeSigMarker(NULL, selectedTempoPoints[i], nTime, NULL, NULL, nBPM, cNum, cDen, cShape);
		}
		else
			++exceed;
	}

	UpdateTimeline();

	// Restore preferences back to the previous state
	*(int*)GetConfigVar("envclicksegmode") = envClickSegMode;

	return exceed;
};




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//							DIALOG BOX PROCEDURES
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

WDL_DLGRET BRConvertProjMarkersToTempoProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

			char timeSigNum[128], timeSigDen[128] , markersPerMeasure[128];				
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
					
					// Update edit boxes to show a "atoied" value
					sprintf(timeSigNum, "%d", num);
					sprintf(timeSigDen, "%d", den);
					sprintf(markersPerMeasure, "%d", measure);
					SetDlgItemText(hwnd, IDC_BR_NUM, timeSigNum);
					SetDlgItemText(hwnd, IDC_BR_DEN, timeSigDen);
					SetDlgItemText(hwnd, IDC_BR_MARKERS, markersPerMeasure);
					
					// Check values
					int i = 0;
					if ((num <= 0) || (den <= 0) || (measure <= 0))
					{	
						MessageBox(g_hwndParent, __LOCALIZE("All values have to be positive integers.","sws_DLG_166"), __LOCALIZE("SWS - Error","sws_mbox"), MB_OK);
						i = 1;
						while (true)
						{
							if (measure <= 0)
							{
								SetFocus(GetDlgItem(hwnd, IDC_BR_MARKERS));
								SendMessage(GetDlgItem(hwnd, IDC_BR_MARKERS), EM_SETSEL, 0, -1);
								break;
							}
							if (num <= 0)
							{
								SetFocus(GetDlgItem(hwnd, IDC_BR_NUM));
								SendMessage(GetDlgItem(hwnd, IDC_BR_NUM), EM_SETSEL, 0, -1);
								break;
							}
							if (den <= 0)
							{	
								SetFocus(GetDlgItem(hwnd, IDC_BR_DEN));
								SendMessage(GetDlgItem(hwnd, IDC_BR_DEN), EM_SETSEL, 0, -1);
								break;
							}
						}
					}
					
					if (!IsWindowEnabled(GetDlgItem(hwnd, IDC_BR_TIMESEL)))
						timeSelection = 0;

					// If all went well, start converting
					if(wParam == IDOK && i == 0)
					{
						BRConvertProjMarkersToTempo(num, den, measure, timeSelection, removeMarkers);
						SetFocus(hwnd);	
					}
					return 0;
				}
				break;
			
				case IDCANCEL:
				{
					g_convertProjectMarkersDialog = false;
					KillTimer(hwnd, 1);
					ShowWindow(hwnd, SW_HIDE);
				}	
				break;
			}
		}
		break;

		case WM_TIMER:
		{  
			double tStart, tEnd;
			GetSet_LoopTimeRange2 (NULL, false, false, &tStart, &tEnd, false);
				if (tStart == tEnd)
					EnableWindow(GetDlgItem(hwnd, IDC_BR_TIMESEL), false);
				else
					EnableWindow(GetDlgItem(hwnd, IDC_BR_TIMESEL), true);
			
		}
		break;

		
		case WM_DESTROY:
			SaveWindowPos(hwnd, cWndPosKey);
			break; 
	}
	return 0;
};

WDL_DLGRET BRSelAdjTempoProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, uMsg, wParam, lParam))
		return r;
	
	const char cWndPosKey[] = "SelAndAdjTemp WndPos";
	switch(uMsg)
	{
		case WM_INITDIALOG :
		{
			//// SELECT MARKERS ////

			// Drop down menus
			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_BR_SEL_TIME_RANGE));
			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_BR_SEL_SHAPE));
			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_BR_SEL_TYPE_DEF));
			
			int x = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_TIME_RANGE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Project","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TIME_RANGE, CB_SETITEMDATA, x, 0);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_TIME_RANGE,CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Time selection","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TIME_RANGE, CB_SETITEMDATA, x, 1);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_TIME_RANGE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Ignore time selection","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TIME_RANGE, CB_SETITEMDATA, x, 2);
			
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_SHAPE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("All","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_SHAPE, CB_SETITEMDATA, x, 0);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_SHAPE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Square","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_SHAPE, CB_SETITEMDATA, x, 1);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_SHAPE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Linear","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_SHAPE, CB_SETITEMDATA, x, 2);

			x = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_TYPE_DEF, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("All","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TYPE_DEF, CB_SETITEMDATA, x, 0);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_TYPE_DEF, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Tempo markers","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TYPE_DEF, CB_SETITEMDATA, x, 1);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_TYPE_DEF, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Time signature markers","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TYPE_DEF, CB_SETITEMDATA, x, 2);

			SendDlgItemMessage(hwnd, IDC_BR_SEL_TIME_RANGE, CB_SETCURSEL, 0, 0);
			SendDlgItemMessage(hwnd, IDC_BR_SEL_SHAPE, CB_SETCURSEL, 0, 0);
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TYPE_DEF, CB_SETCURSEL, 0, 0);

			// Edit boxes
			char bpmStart[128], bpmEnd[128], sigNum[128], sigDen[128];	
			sprintf(bpmStart, "%d", 100);
			sprintf(bpmEnd, "%d", 150);
			sprintf(sigNum, "%d", 4);
			sprintf(sigDen, "%d", 4);

			SetDlgItemText(hwnd, IDC_BR_SEL_BPM_START, bpmStart);
			SetDlgItemText(hwnd, IDC_BR_SEL_BPM_END, bpmEnd);
			SetDlgItemText(hwnd, IDC_BR_SEL_SIG_NUM, sigNum);
			SetDlgItemText(hwnd, IDC_BR_SEL_SIG_DEN, sigDen);
			EnableWindow(GetDlgItem(hwnd, IDC_BR_SEL_SIG_NUM), 0);
			EnableWindow(GetDlgItem(hwnd, IDC_BR_SEL_SIG_DEN), 0);
			
			// Check buttons			
			CheckDlgButton(hwnd, IDC_BR_SEL_BPM, !!1);
			CheckDlgButton(hwnd, IDC_BR_SEL_SIG, !!0);
			CheckDlgButton(hwnd, IDC_BR_SEL_ADD, !!0);
			

			//// MODIFY MARKERS ////
			
			// Drop down menus
			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_BR_ADJ_SHAPE));

			x = (int)SendDlgItemMessage(hwnd, IDC_BR_ADJ_SHAPE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Preserve","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_ADJ_SHAPE, CB_SETITEMDATA, x, 0);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_ADJ_SHAPE,CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Invert","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_ADJ_SHAPE, CB_SETITEMDATA, x, 1);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_ADJ_SHAPE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Linear","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_ADJ_SHAPE, CB_SETITEMDATA, x, 2);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_ADJ_SHAPE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Square","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_ADJ_SHAPE, CB_SETITEMDATA, x, 3);

			SendDlgItemMessage(hwnd, IDC_BR_ADJ_SHAPE, CB_SETCURSEL, 0, 0);


			// Edit boxes
			char bpm[128], bpmCursor[128], bpmVal[128], bpmPerc[128];
			double effBpmCursor;

			TimeMap_GetTimeSigAtTime(NULL, GetCursorPositionEx(NULL), NULL, NULL, &effBpmCursor);
			sprintf(bpmCursor, "%g", effBpmCursor);
			sprintf(bpm, "%d", 0);
			sprintf(bpmVal, "%d", 0);
			sprintf(bpmPerc, "%d", 0);

			SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_1, bpm);			
			SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_2, bpmCursor);
			SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_3, bpm);
			SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_TAR_1, bpm);			
			SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_TAR_2, bpm);
			SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_TAR_3, bpm);
			SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_VAL, bpmVal);
			SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_PERC, bpmPerc);
			EnableWindow(GetDlgItem(hwnd, IDC_BR_ADJ_BPM_PERC), false);
			
			// Check buttons
			CheckDlgButton(hwnd, IDC_BR_ADJ_BPM_VAL_ENB, 1);
			
		}
		break;
        
		case WM_COMMAND :
		{
            switch(LOWORD(wParam))
            {
				//// SELECT MARKERS ////
				case IDC_BR_SEL_BPM:
				{				
					int enb = IsDlgButtonChecked(hwnd, IDC_BR_SEL_BPM);
					EnableWindow(GetDlgItem(hwnd, IDC_BR_SEL_BPM_START), enb);
					EnableWindow(GetDlgItem(hwnd, IDC_BR_SEL_BPM_END), enb);
				}
				break;
				
				case IDC_BR_SEL_SIG:
				{				
					int enb = IsDlgButtonChecked(hwnd, IDC_BR_SEL_SIG);
					EnableWindow(GetDlgItem(hwnd, IDC_BR_SEL_SIG_NUM), enb);
					EnableWindow(GetDlgItem(hwnd, IDC_BR_SEL_SIG_DEN), enb);
				}
				break;

				case IDC_BR_SEL_INVERT:
				{
					Undo_BeginBlock2(NULL);
					BRSelectTempoMarkers (1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
					Undo_EndBlock2 (NULL, __LOCALIZE("Invert selection of tempo markers","sws_DLG_167"), UNDO_STATE_ALL);
				}
				break;
				
				case IDC_BR_SEL_CLEAR:
				{
					Undo_BeginBlock2(NULL);
					BRSelectTempoMarkers (0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
					Undo_EndBlock2 (NULL, __LOCALIZE("Unselect all tempo markers","sws_DLG_167"), UNDO_STATE_ALL);
				}
				break;

				case IDC_BR_SEL_SELECT:
				{
					Undo_BeginBlock2(NULL);
					BRSelectTempoMarkersCase (hwnd);
					Undo_EndBlock2 (NULL, __LOCALIZE("Select tempo markers","sws_DLG_167"), UNDO_STATE_ALL);
				}
				break;
				
				//// MODIFY MARKERS ////
				case IDC_BR_ADJ_BPM_VAL_ENB:
				{
					int enb = IsDlgButtonChecked(hwnd, IDC_BR_ADJ_BPM_VAL_ENB);
					EnableWindow(GetDlgItem(hwnd, IDC_BR_ADJ_BPM_VAL), enb);
					EnableWindow(GetDlgItem(hwnd, IDC_BR_ADJ_BPM_PERC), !enb);
					
					BRUpdateTargetBpm(hwnd, 1, 1, 1);
				}
				break;

				case IDC_BR_ADJ_BPM_PERC_ENB:
				{
					int enb = IsDlgButtonChecked(hwnd, IDC_BR_ADJ_BPM_PERC_ENB);
					EnableWindow(GetDlgItem(hwnd, IDC_BR_ADJ_BPM_PERC), enb);
					EnableWindow(GetDlgItem(hwnd, IDC_BR_ADJ_BPM_VAL), !enb);
					
					BRUpdateTargetBpm(hwnd, 1, 1, 1);
				}
				break;

				case IDC_BR_ADJ_BPM_VAL:
				{
					if (HIWORD(wParam) == EN_CHANGE && GetFocus() == GetDlgItem(hwnd, IDC_BR_ADJ_BPM_VAL))
						BRUpdateTargetBpm(hwnd, 1, 1, 1);
				}
				break;

				case IDC_BR_ADJ_BPM_PERC:
				{
					if (HIWORD(wParam) == EN_CHANGE && GetFocus() == GetDlgItem(hwnd, IDC_BR_ADJ_BPM_PERC))
						BRUpdateTargetBpm(hwnd, 1, 1, 1);					
				}
				break;
				
				case IDC_BR_ADJ_BPM_TAR_1:
				{
					if (HIWORD(wParam) == EN_CHANGE && GetFocus() == GetDlgItem(hwnd, IDC_BR_ADJ_BPM_TAR_1))
					{	
						char bpmTar[128], bpmCur[128];
						double bpmVal, bpmPerc;

						GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_TAR_1, bpmTar, 128);
						GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_1, bpmCur, 128);
						replace(bpmTar, bpmTar+strlen(bpmTar), ',', '.' );
						
						bpmVal = atof(bpmTar) - atof(bpmCur);
						if (atof(bpmCur) != 0)
							bpmPerc = bpmVal / atof(bpmCur) * 100;
						else
							bpmPerc = 0;
						
						sprintf(bpmTar, "%g", bpmVal);
						sprintf(bpmCur, "%g", bpmPerc);

						SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_VAL, bpmTar);
						SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_PERC, bpmCur);
						BRUpdateTargetBpm(hwnd, 0, 1, 1);
					}
				}
				break;
				
				case IDC_BR_ADJ_BPM_TAR_2:
				{
					if (HIWORD(wParam) == EN_CHANGE && GetFocus() == GetDlgItem(hwnd, IDC_BR_ADJ_BPM_TAR_2))
					{	
						char bpmTar[128], bpmCur[128];
						double bpmVal, bpmPerc;

						GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_TAR_2, bpmTar, 128);
						GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_2, bpmCur, 128);
						replace(bpmTar, bpmTar+strlen(bpmTar), ',', '.' );
						
						bpmVal = atof(bpmTar) - atof(bpmCur);
						if (atof(bpmCur) != 0)
							bpmPerc = bpmVal / atof(bpmCur) * 100;
						else
							bpmPerc = 0;
						
						sprintf(bpmTar, "%g", bpmVal);
						sprintf(bpmCur, "%g", bpmPerc);

						SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_VAL, bpmTar);
						SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_PERC, bpmCur);
						BRUpdateTargetBpm(hwnd, 1, 0, 1);
					}
				}
				break;

				case IDC_BR_ADJ_BPM_TAR_3:
				{
					if (HIWORD(wParam) == EN_CHANGE && GetFocus() == GetDlgItem(hwnd, IDC_BR_ADJ_BPM_TAR_3))
					{	
						char bpmTar[128], bpmCur[128];
						double bpmVal, bpmPerc;

						GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_TAR_3, bpmTar, 128);
						GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_3, bpmCur, 128);
						replace(bpmTar, bpmTar+strlen(bpmTar), ',', '.' );
							
						bpmVal = atof(bpmTar) - atof(bpmCur);
						if (atof(bpmCur) != 0)
							bpmPerc = bpmVal / atof(bpmCur) * 100;
						else
							bpmPerc = 0;
						
						sprintf(bpmTar, "%g", bpmVal);
						sprintf(bpmCur, "%g", bpmPerc);

						SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_VAL, bpmTar);
						SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_PERC, bpmCur);
						BRUpdateTargetBpm(hwnd, 1, 1, 0);
					}
				}
				break;

				case IDC_BR_ADJ_APPLY:
				{
					Undo_BeginBlock2(NULL);
					KillTimer(hwnd, 1);	
					BRModifyTempoMarkersCase (hwnd);
					SetTimer(hwnd, 1, 500, NULL);
					Undo_EndBlock2 (NULL, __LOCALIZE("Modify selected tempo markers","sws_DLG_167"), UNDO_STATE_ALL);

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
			// Get number of selected points and set Windows caption
			vector<int> selectedPoints = BRGetSelectedTempoPoints();

			char pointCount[1024];
			_snprintf(pointCount, sizeof(pointCount), __LOCALIZE_VERFMT("SWS Select and adjust tempo markers (%d of %d points selected)","sws_DLG_167") , selectedPoints.size(), CountTempoTimeSigMarkers(NULL) );
			SetWindowText(hwnd,pointCount);

			// Update current and target edit boxes
			BRUpdateCurrentBpm(hwnd, selectedPoints);			
		}
		break;

		case WM_DESTROY:
			SaveWindowPos(hwnd, cWndPosKey);
			break; 
	}
	return 0;
};

void BRUpdateCurrentBpm (HWND hwnd, vector<int> selectedPoints)
{
	double bpmFirst, bpmCursor, bpmLast;
	char eBpmFirst[128], eBpmCursor[128], eBpmLast[128], eBpmFirstChk[128], eBpmCursorChk[128], eBpmLastChk[128];
	
	// Get BPM info on selected points
	if (selectedPoints.size() != 0)
	{	
		GetTempoTimeSigMarker(NULL, selectedPoints[0], NULL, NULL, NULL, &bpmFirst, NULL, NULL, NULL);
		GetTempoTimeSigMarker(NULL, selectedPoints.back(), NULL, NULL, NULL, &bpmLast, NULL, NULL, NULL);
		sprintf(eBpmFirst, "%g", bpmFirst);
		sprintf(eBpmLast, "%g", bpmLast);
	}
	
	else
	{
		bpmFirst = bpmLast = 0;
		sprintf(eBpmFirst, "%d", bpmFirst);
		sprintf(eBpmLast, "%d", bpmLast);
	}

	TimeMap_GetTimeSigAtTime(NULL, GetCursorPositionEx(NULL), NULL, NULL, &bpmCursor);
	sprintf(eBpmCursor, "%g", bpmCursor);
			
	// Get values from edit boxes
	GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_1, eBpmFirstChk, 128);
	GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_2, eBpmCursorChk, 128);
	GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_3, eBpmLastChk, 128);
	
	// We only update edit boxes if they've changed
	if (atof(eBpmFirst) != atof(eBpmFirstChk) || atof(eBpmCursor) != atof(eBpmCursorChk) || atof(eBpmLast) != atof(eBpmLastChk))
		{	
			// Update current edit boxes
			SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_1, eBpmFirst);
			SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_2, eBpmCursor);
			SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_3, eBpmLast);
			
			// Update target edit boxes
			BRUpdateTargetBpm(hwnd, 1, 1, 1);
		}
};

void BRUpdateTargetBpm (HWND hwnd, int doFirst, int doCursor, int doLast)
{
	char bpmAdj[128], bpmFirstTar[128], bpmCursorTar[128], bpmLastTar[128];
																	
						
	GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_1, bpmFirstTar, 128);
	GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_2, bpmCursorTar, 128);
	GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_3, bpmLastTar, 128);

	if (atof(bpmFirstTar) == 0)
	{
		sprintf(bpmFirstTar, "%d", 0);
		sprintf(bpmCursorTar, "%d", 0);
		sprintf(bpmLastTar, "%d", 0);
	}
	
	else
	{
		if (IsDlgButtonChecked(hwnd, IDC_BR_ADJ_BPM_VAL_ENB) == 1)
		{
			GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_VAL, bpmAdj, 128);
			replace(bpmAdj, bpmAdj+strlen(bpmAdj), ',', '.' );
												
			sprintf(bpmFirstTar, "%g", atof(bpmAdj) + atof(bpmFirstTar));
			sprintf(bpmCursorTar, "%g", atof(bpmAdj) + atof(bpmCursorTar));
			sprintf(bpmLastTar, "%g", atof(bpmAdj) + atof(bpmLastTar));
		}
		
		else
		{						
			GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_PERC, bpmAdj, 128);
			replace(bpmAdj, bpmAdj+strlen(bpmAdj), ',', '.' );
		
			sprintf(bpmFirstTar, "%g", (atof(bpmAdj) / 100 + 1) * atof(bpmFirstTar));
			sprintf(bpmCursorTar, "%g", (atof(bpmAdj) / 100 + 1) * atof(bpmCursorTar));
			sprintf(bpmLastTar, "%g", (atof(bpmAdj) / 100 + 1) * atof(bpmLastTar));
		}
	}

	if (doFirst == 1)
		SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_TAR_1, bpmFirstTar);
	if (doCursor == 1)
		SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_TAR_2, bpmCursorTar);
	if (doLast == 1)
		SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_TAR_3, bpmLastTar);
};

void BRSelectTempoMarkersCase (HWND hwnd)
{
	int mode;
	if (IsDlgButtonChecked(hwnd, IDC_BR_SEL_ADD) == 1)
		mode = 3;
	else
		mode = 2;

	int bpm;
	if (IsDlgButtonChecked(hwnd, IDC_BR_SEL_BPM) == 1)
		bpm = 1;
	else
		bpm = 0;

	int sig;
	if (IsDlgButtonChecked(hwnd, IDC_BR_SEL_SIG) == 1)
		sig = 1;
	else
		sig = 0;

	int timeSel = (int)SendDlgItemMessage(hwnd,IDC_BR_SEL_TIME_RANGE,CB_GETCURSEL,0,0);
	int shape = (int)SendDlgItemMessage(hwnd,IDC_BR_SEL_SHAPE,CB_GETCURSEL,0,0);
	int type = (int)SendDlgItemMessage(hwnd,IDC_BR_SEL_TYPE_DEF,CB_GETCURSEL,0,0);

	char eBpmStart[128], eBpmEnd[128], eSigNum[128], eSigDen[128];	
	GetDlgItemText(hwnd, IDC_BR_SEL_BPM_START, eBpmStart, 128);
		replace(eBpmStart, eBpmStart+strlen(eBpmStart), ',', '.' );
	GetDlgItemText(hwnd, IDC_BR_SEL_BPM_END, eBpmEnd, 128);
		replace(eBpmEnd, eBpmEnd+strlen(eBpmEnd), ',', '.' );
	GetDlgItemText(hwnd, IDC_BR_SEL_SIG_NUM, eSigNum, 128);
	GetDlgItemText(hwnd, IDC_BR_SEL_SIG_DEN, eSigDen, 128);
	int sigNum = atoi(eSigNum);
	int sigDen = atoi(eSigDen);
	double bpmStart = atof(eBpmStart);
	double bpmEnd = atof(eBpmEnd);

	// Update edit boxes to show "atoied/atofed" value
	sprintf(eBpmStart, "%.13g", bpmStart);
	sprintf(eBpmEnd, "%.13g", bpmEnd);
	sprintf(eSigNum, "%d", sigNum);
	sprintf(eSigDen, "%d", sigDen);
	SetDlgItemText(hwnd, IDC_BR_SEL_BPM_START, eBpmStart);
	SetDlgItemText(hwnd, IDC_BR_SEL_BPM_END, eBpmEnd);
	SetDlgItemText(hwnd, IDC_BR_SEL_SIG_NUM, eSigNum);
	SetDlgItemText(hwnd, IDC_BR_SEL_SIG_DEN, eSigDen);

	bool check = true;
	if (sigNum <= 0 || sigDen <= 0)
	{
		check = false;
		MessageBox(g_hwndParent, __LOCALIZE("Time signature values can only be positive integers.","sws_DLG_167"), __LOCALIZE("SWS - Error","sws_mbox"), MB_OK);

		while (true)
		{
			if (sigNum <= 0)
			{
				SetFocus(GetDlgItem(hwnd, IDC_BR_SEL_SIG_NUM));
				SendMessage(GetDlgItem(hwnd, IDC_BR_SEL_SIG_NUM), EM_SETSEL, 0, -1);
				break;
			}
			if (sigDen <= 0)
			{	
				SetFocus(GetDlgItem(hwnd, IDC_BR_SEL_SIG_DEN));
				SendMessage(GetDlgItem(hwnd, IDC_BR_SEL_SIG_DEN), EM_SETSEL, 0, -1);
				break;
			}
		}
	}

	if (check)
	{
		if (bpmStart > bpmEnd)
		{
			double temp = bpmStart;
			bpmStart = bpmEnd;
			bpmEnd = temp;
		}
		BRSelectTempoMarkers (mode, timeSel, bpm, bpmStart, bpmEnd, shape, sig, sigNum, sigDen, type);
	}
};

void BRModifyTempoMarkersCase (HWND hwnd)
{
	int mode;
	if (IsDlgButtonChecked(hwnd, IDC_BR_ADJ_BPM_VAL_ENB) == 1)
		mode = 0;
	else
		mode = 1;

	int shape = (int)SendDlgItemMessage(hwnd,IDC_BR_ADJ_SHAPE,CB_GETCURSEL,0,0);

	char eBpmVal[128], eBpmPerc[128];
	GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_VAL, eBpmVal, 128);
		replace(eBpmVal, eBpmVal+strlen(eBpmVal), ',', '.' );
	GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_PERC, eBpmPerc, 128);
		replace(eBpmPerc, eBpmPerc+strlen(eBpmPerc), ',', '.' );
	double bpmVal = atof(eBpmVal);
	double bpmPerc = atof(eBpmPerc);

	// Update edit boxes
	BRUpdateTargetBpm(hwnd, 1, 1, 1);
	sprintf(eBpmVal, "%.13g", bpmVal);
	sprintf(eBpmPerc, "%.13g", bpmPerc);
	SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_VAL, eBpmVal);
	SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_PERC, eBpmPerc);

	int exceed = BRModifySelectedTempoPoints (mode, bpmVal, bpmPerc, shape);
	if (exceed !=0)
		MessageBox(g_hwndParent, __LOCALIZE("Some tempo points couldn't be modified because their new BPM value would be illegal.","sws_DLG_167"), __LOCALIZE("SWS - Error","sws_mbox"), MB_OK);
};



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//							EXTENSIONS STUFF
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "SWS/BR: Convert project markers to tempo markers..." },					"SWS_BRCONVERTMARKERSTOTEMPO",		BRConvertProjMarkersToTempoAction, NULL, 0, IsConvertProjMarkerToTempoVisible },
	{ { DEFACCEL, "SWS/BR: Select and adjust tempo markers..." },							"SWS_BRADJUSTSELTEMPO",				BRSelAdjTempoAction, NULL, 0, IsSelAdjTempoVisible },
	{ { DEFACCEL, "SWS/BR: Split selected items at tempo markers" },						"SWS_BRSPLITSELECTEDTEMPO",			BRSplitSelItemsAtTempo, },

	{ { DEFACCEL, "SWS/BR: Move selected tempo markers forward" },							"SWS_BRMOVETEMPOFORWARD",			BRMoveSelTempo, NULL, 5},
	{ { DEFACCEL, "SWS/BR: Move selected tempo markers back" },								"SWS_BRMOVETEMPOBACK",				BRMoveSelTempo, NULL, -5},
		{ { DEFACCEL, "SWS/BR: Move selected tempo markers forward 0.1 ms" },				"SWS_BRMOVETEMPOFORWARD01",			BRMoveSelTempo, NULL, 2},
		{ { DEFACCEL, "SWS/BR: Move selected tempo markers forward 1 ms" },					"SWS_BRMOVETEMPOFORWARD1",			BRMoveSelTempo, NULL, 1},
		{ { DEFACCEL, "SWS/BR: Move selected tempo markers forward 10 ms"},					"SWS_BRMOVETEMPOFORWARD10",			BRMoveSelTempo, NULL, 10},
		{ { DEFACCEL, "SWS/BR: Move selected tempo markers forward 100 ms"},				"SWS_BRMOVETEMPOFORWARD100",		BRMoveSelTempo, NULL, 100},
		{ { DEFACCEL, "SWS/BR: Move selected tempo markers forward 1000 ms" },				"SWS_BRMOVETEMPOFORWARD1000",		BRMoveSelTempo, NULL, 1000},
		{ { DEFACCEL, "SWS/BR: Move selected tempo markers back 0.1 ms"},					"SWS_BRMOVETEMPOBACK01",			BRMoveSelTempo, NULL, -2},
		{ { DEFACCEL, "SWS/BR: Move selected tempo markers back 1 ms"},						"SWS_BRMOVETEMPOBACK1",				BRMoveSelTempo, NULL, -1},
		{ { DEFACCEL, "SWS/BR: Move selected tempo markers back 10 ms"},					"SWS_BRMOVETEMPOBACK10",			BRMoveSelTempo, NULL, -10},
		{ { DEFACCEL, "SWS/BR: Move selected tempo markers back 100 ms"},					"SWS_BRMOVETEMPOBACK100",			BRMoveSelTempo, NULL, -100},
		{ { DEFACCEL, "SWS/BR: Move selected tempo markers back 1000 ms"},					"SWS_BRMOVETEMPOBACK1000",			BRMoveSelTempo, NULL, -1000},

	{ { DEFACCEL, "SWS/BR: Move edit cursor to next envelope point and select it" },		"SWS_BRMOVEEDITSELNEXTENV",			BRMoveEditCurSelEnvPoint, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Move edit cursor to previous envelope point and select it" },	"SWS_BRMOVEEDITSELPREVENV",			BRMoveEditCurSelEnvPoint, NULL, -1},


	{ {}, LAST_COMMAND, }, // Denote end of table
};
//!WANT_LOCALIZE_1ST_STRING_END


int BreederInit()
{
	SWSRegisterCommands(g_commandTable);
	return 1;
};