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
// TODO: Finish move tempo marker actions
//	   : Finish Select and adjust tempo markers dialog and actions
#include "stdafx.h"
#include "Breeder.h"

#include "../MarkerList/MarkerListActions.h"
#include "../reaper/localize.h"
#include "../SnM/SnM_Dlg.h"

//Globals
bool ConvertProjectMarkersVisible = false;
bool SelectAdjustTempoPointsVisible = false;


//Prototypes
void BRConvertProjectMarkersToTempoMarkers(int num, int den, int measure, int timeSelection, int removeMarkers);
void BRSelectTempoPoints (int time, int BPM, double BPMStart, double BPMEnd, int shape, int sig, int sigNum, int sigDen);
vector<int> BRGetSelectedTempoPoints();
WDL_DLGRET BRConvertProjectMarkersToTempoMarkersProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
WDL_DLGRET BRSelectAdjustTempoPointsProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


//////////////////////////////////////////////////////////////////////////////////////////////////////////
//								  			ACTIONS
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
	return ConvertProjectMarkersVisible;
};
void BRConvertProjectMarkersToTempoMarkersAction(COMMAND_T* = NULL)
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
	static HWND hwnd = CreateDialog (g_hInst, MAKEINTRESOURCE(IDD_BR_MARKERS_TO_TEMPO), g_hwndParent, BRConvertProjectMarkersToTempoMarkersProc);

	if (ConvertProjectMarkersVisible)
	{	ConvertProjectMarkersVisible = false;
		
		ShowWindow(hwnd, SW_HIDE);
	}
	else
	{
		ConvertProjectMarkersVisible = true;
		ShowWindow(hwnd, SW_SHOW);
		SetFocus(hwnd);
	}
};



/*
void BRMoveSelTempoPoint(COMMAND_T* t ) 
{ 
	double timeDiff = (int)t->user;
	timeDiff = timeDiff/1000;
	
		
	// Get selected points' ID into vector
	vector<int> selectedTempoPoints = BRGetSelectedTempoPoints();
	
	// Check number of selected points
	int user;
	if (selectedTempoPoints.size()== 0)
		return;
	if (selectedTempoPoints.size() > 1)
	{
		user = ShowMessageBox("More than one point selected. Continue?", "Multiple points selected", 4);
		if (user == 7)
			return;
	}

	// Declare needed variables
	double pTime, pBeat, pBPM, 
		   cTime, cBeat, cBPM, 
		   nTime, nBeat, nBPM;
	
	int pMeasure, pNum, pDen, pEffNum, pEffDen,
		cMeasure, cNum, cDen, cEffNum, cEffDen,
		nMeasure, nNum, nDen, nEffNum, nEffDen;
		
	bool pShape, 
		 cShape,
		 nShape; 
		 
	double measureLengthPC, measureLengthCN;

	double pNewBPM, cNewBPM;

	// Loop through selected points and perform BPM calculations
	for (size_t i = 0; i < selectedTempoPoints.size(); ++i)
	{
		// Get current time position of all points
		GetTempoTimeSigMarker(NULL, selectedTempoPoints[i] -1 , &pTime, &pMeasure, &pBeat, &pBPM, &pNum, &pDen, &pShape);
		GetTempoTimeSigMarker(NULL, selectedTempoPoints[i], &cTime, &cMeasure, &cBeat, &cBPM, &cNum, &cDen, &cShape);
		GetTempoTimeSigMarker(NULL, selectedTempoPoints[i] + 1 , &nTime, &nMeasure, &nBeat, &nBPM, &nNum, &nDen, &nShape);

		if (pNum == 0)
			TimeMap_GetTimeSigAtTime(NULL, pTime, &pEffNum, &pEffDen, NULL);
		else
			pEffNum = pNum, pEffDen = pDen; 
		if (cNum == 0)
			TimeMap_GetTimeSigAtTime(NULL, cTime, &cEffNum, &cEffDen, NULL);
		else
			cEffNum = cNum, cEffDen = cDen;

		// Calculate needed BPM values

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
				cNewBPM = ((480 * cEffNum * measureLengthCN) / (cEffDen * (nTime - cTime - timeDiff))) - nBPM;
			}
		}

		// If previous point's shape is linear
		if (pShape == 1)
		{
			// If current point's shape is square...
			if (cShape == 0)
			{
			}

			// If current point's shape is linear...
			if (cShape == 1)
			{
			}
		}



		// Set BPM values. IF statement acts as a safety net for illogical calculations and reaper dislikes (it hates BPM over 960) 
		if (pNewBPM >= 0.001 && cNewBPM > 0.001 && pNewBPM <=960 && cNewBPM <= 960 &&(cTime+timeDiff) > pTime && (cTime+timeDiff) < nTime)
				{
					SetTempoTimeSigMarker(NULL, selectedTempoPoints[i] - 1, pTime, -1, -1, pNewBPM, pNum, pDen, pShape);
					SetTempoTimeSigMarker(NULL, selectedTempoPoints[i], (cTime + timeDiff), -1, -1, cNewBPM, cNum, cDen, cShape);
				}
	}
	UpdateTimeline();
};


bool IsSelectAdjustTempoPointsVisible(COMMAND_T* = NULL)
{
	return SelectAdjustTempoPointsVisible;
};
void BRSelectAdjustTempoPointsAction(COMMAND_T* = NULL)
{	
	static HWND hwnd = CreateDialog (g_hInst, MAKEINTRESOURCE(IDD_BR_SELECT_ADJUST_TEMPO), g_hwndParent, BRSelectAdjustTempoPointsProc);

	if (SelectAdjustTempoPointsVisible)
	{	
		SelectAdjustTempoPointsVisible = false;
		
		KillTimer(hwnd, 1);	
		ShowWindow(hwnd, SW_HIDE);
	}

	else
	{
		SelectAdjustTempoPointsVisible = true;
		
		SetTimer(hwnd, 1, 500, NULL);
		ShowWindow(hwnd, SW_SHOW);
		SetFocus(hwnd);
	}
};
*/



//////////////////////////////////////////////////////////////////////////////////////////////////////////
//								  		VARIOUS FUNCTIONS
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
			SetTempoTimeSigMarker (NULL, -1, markerPositions[i], -1, -1, bpm, 0, 0, NULL);
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
//								  		DIALOG BOX PROCEDURES
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
					ConvertProjectMarkersVisible = false;
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

			/// SET CONTROLS ///
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
					SelectAdjustTempoPointsVisible = false;
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
			char pointCount [128] = {0};
			sprintf(pointCount, "SWS Select and adjust tempo markers (%d of %d points selected)", selectedPoints.size(), CountTempoTimeSigMarkers(NULL) );
			SetWindowText(hwnd,pointCount);
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
//											EXTENSIONS STUFF
//////////////////////////////////////////////////////////////////////////////////////////////////////////


//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "SWS/BR: Convert project markers to tempo markers" },													"SWS_BRCONVERTMARKERSTOTEMPO",			BRConvertProjectMarkersToTempoMarkersAction, NULL, 0, IsConvertProjectMarkerVisible },
	{ {	DEFACCEL, "SWS/BR: Split selected items at tempo markers" },													"SWS_BRSPLITSELECTEDTEMPO",				BRSplitSelectedItemsAtTempoMarkers, },
	/*
	{ {	DEFACCEL, "SWS/BR: Move selected tempo point forward 1 ms (preserve unselected points' position)" },			"SWS_BRMOVETEMPOFORWARD1",				BRMoveSelTempoPoint, NULL, 1},
	{ {	DEFACCEL, "SWS/BR: Move selected tempo point forward 10 ms (preserve unselected points' position)"},			"SWS_BRMOVETEMPOFORWARD10",				BRMoveSelTempoPoint, NULL, 10},
	{ {	DEFACCEL, "SWS/BR: Move selected tempo point forward 100 ms (preserve unselected points' position)"},			"SWS_BRMOVETEMPOFORWARD100",			BRMoveSelTempoPoint, NULL, 100},
	{ {	DEFACCEL, "SWS/BR: Move selected tempo point forward 1000 ms (preserve unselected points' position)" },			"SWS_BRMOVETEMPOFORWARD1000",			BRMoveSelTempoPoint, NULL, 1000},
	{ {	DEFACCEL, "SWS/BR: Move selected tempo point back 1 ms (preserve unselected points' position)"},				"SWS_BRMOVETEMPOBACK1",					BRMoveSelTempoPoint, NULL, -1},
	{ {	DEFACCEL, "SWS/BR: Move selected tempo point back 10 ms (preserve unselected points' position)"},				"SWS_BRMOVETEMPOBACK10",				BRMoveSelTempoPoint, NULL, -10},
	{ {	DEFACCEL, "SWS/BR: Move selected tempo point back 100 ms (preserve unselected points' position)"},				"SWS_BRMOVETEMPOBACK100",				BRMoveSelTempoPoint, NULL, -100},
	{ {	DEFACCEL, "SWS/BR: Move selected tempo point back 1000 ms (preserve unselected points' position)"},				"SWS_BRMOVETEMPOBACK1000",				BRMoveSelTempoPoint, NULL, -1000},
	
	{ {	DEFACCEL, "SWS/BR: Select and adjust tempo markers" },															"SWS_BRADJUSTSELTEMPO",					BRSelectAdjustTempoPointsAction, NULL, 0, IsSelectAdjustTempoPointsVisible },
	*/

	{ {}, LAST_COMMAND, }, // Denote end of table
};

//!WANT_LOCALIZE_1ST_STRING_END




int BreederInit()
{
	SWSRegisterCommands(g_commandTable);
	return 1;
};
