/******************************************************************************
/ BR_TempoDlg.cpp
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
#include "BR_TempoDlg.h"
#include "BR_TempoTools.h"
#include "BR_Util.h"
#include "../SnM/SnM_Dlg.h"
#include "../MarkerList/MarkerListActions.h"
#include "../reaper/localize.h"

#define SHAPE_KEY			"BR - ChangeTempoShape"
#define SHAPE_WND_KEY		"BR - ChangeTempoShape WndPos"
#define RAND_KEY			"BR - RandomizeTempo"
#define RAND_WND_KEY		"BR - RandomizeTempo WndPos"
#define CONV_KEY			"BR - ConvertTempo"
#define CONV_WND_KEY		"BR - ConvertTempo WndPos"
#define SEL_ADJ_KEY			"BR - SelectAdjustTempo"
#define SEL_ADJ_WND_KEY		"BR - SelectAdjustTempo WndPos"
#define DESEL_KEY			"BR - DeselectNthTempo"
#define DESEL_WND_KEY		"BR - DeselectNthTempo WndPos"

//Globals
bool g_convertMarkersToTempoDialog = false;
bool g_selectAdjustTempoDialog = false;
bool g_deselectTempoDialog = false;
bool g_tempoShapeDialog = false;
double g_tempoShapeSplitRatio = -1;
int g_tempoShapeSplitMiddle = -1;
static char* g_oldTempo = NULL;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//							Change Shape Of Tempo Markers - Options
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
WDL_DLGRET TempoShapeOptionsProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, uMsg, wParam, lParam))
		return r;
	
	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			// Dropdown (split ratio)
			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_BR_SHAPE_SPLIT_RATIO));
			int x = (int)SendDlgItemMessage(hwnd, IDC_BR_SHAPE_SPLIT_RATIO, CB_ADDSTRING, 0, (LPARAM)"1/2");
			SendDlgItemMessage(hwnd, IDC_BR_SHAPE_SPLIT_RATIO, CB_SETITEMDATA, x, 0);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_SHAPE_SPLIT_RATIO, CB_ADDSTRING, 0, (LPARAM)"1/3");
			SendDlgItemMessage(hwnd, IDC_BR_SHAPE_SPLIT_RATIO, CB_SETITEMDATA, x, 1);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_SHAPE_SPLIT_RATIO, CB_ADDSTRING, 0, (LPARAM)"1/4");
			SendDlgItemMessage(hwnd, IDC_BR_SHAPE_SPLIT_RATIO, CB_SETITEMDATA, x, 2);
						
			// Load options from .ini file
			int split;
			char splitRatio[128];
			LoadOptionsTempoShape (split, splitRatio, sizeof(splitRatio));
			
			// Set controls and global variables
			SetDlgItemText(hwnd, IDC_BR_SHAPE_SPLIT_RATIO, splitRatio);
			CheckDlgButton(hwnd, IDC_BR_SHAPE_SPLIT, !!split);
			EnableWindow(GetDlgItem(hwnd, IDC_BR_SHAPE_SPLIT_RATIO), !!split);
			SetTempoShapeGlobalVariable (split, splitRatio, sizeof(splitRatio));

			RestoreWindowPos(hwnd, SHAPE_WND_KEY, false);
		}
		break;
        
		case WM_COMMAND:
		{
            switch(LOWORD(wParam))
            {
            	case IDC_BR_SHAPE_SPLIT:
				{
					// Read dialog values
					int split = IsDlgButtonChecked(hwnd, IDC_BR_SHAPE_SPLIT);
					char splitRatio[128]; GetDlgItemText(hwnd, IDC_BR_SHAPE_SPLIT_RATIO, splitRatio, 128);
					
					// Check split ratio
					double convertedRatio; IsThisFraction (splitRatio, sizeof(splitRatio), convertedRatio);
					if (convertedRatio <= 0 || convertedRatio >= 1)
						strcpy(splitRatio, "0");

					// Set global variable and update split ratio edit box
					SetTempoShapeGlobalVariable (split, splitRatio, sizeof(splitRatio));
					SetDlgItemText(hwnd, IDC_BR_SHAPE_SPLIT_RATIO, splitRatio);

					// Enable/disable check boxes
					EnableWindow(GetDlgItem(hwnd, IDC_BR_SHAPE_SPLIT_RATIO), !!split);														
				}
				break;

				case IDC_BR_SHAPE_SPLIT_RATIO:
				{
					// Read dialog values
					int split = IsDlgButtonChecked(hwnd, IDC_BR_SHAPE_SPLIT);
					char splitRatio[128]; GetDlgItemText(hwnd, IDC_BR_SHAPE_SPLIT_RATIO, splitRatio, 128);

					// Check split ratio
					double convertedRatio; IsThisFraction (splitRatio, sizeof(splitRatio), convertedRatio);
					if (convertedRatio <= 0 || convertedRatio >= 1)
						strcpy(splitRatio, "0");

					// Set global variable
					SetTempoShapeGlobalVariable (split, splitRatio, sizeof(splitRatio));
				}
				break;

				case IDCANCEL:
				{
					g_tempoShapeDialog = false;
					ShowWindow(hwnd, SW_HIDE);
				}
				break;
			} 
		}
		break;

		case WM_DESTROY:
		{
			SaveWindowPos(hwnd, SHAPE_WND_KEY);
			SaveOptionsTempoShape (hwnd);
		}
		break; 
	}
	return 0;
};

void SetTempoShapeGlobalVariable (int split, char* splitRatio, int ratioLen)
{
	if (split == 0)
		g_tempoShapeSplitMiddle = 0;
	else
		g_tempoShapeSplitMiddle = 1;
			
	double convertedRatio; IsThisFraction (splitRatio, ratioLen, convertedRatio);
	if (convertedRatio <= 0 || convertedRatio >= 1)
		g_tempoShapeSplitRatio = 0;
	else
		g_tempoShapeSplitRatio = convertedRatio;
};

void LoadOptionsTempoShape (int &split, char* splitRatio, int ratioLen)
{
	char tmp[256];
	GetPrivateProfileString("SWS", SHAPE_KEY, "0 4/8", tmp, 256, get_ini_file());
	sscanf(tmp, "%d %s", &split, splitRatio);
	
	// Restore defaults if .ini has been tampered with
	double convertedRatio;
	IsThisFraction (splitRatio, ratioLen, convertedRatio);
	if (split != 0 && split != 1)
		split = 0;
	if (convertedRatio <= 0 || convertedRatio >= 1)
		strcpy(splitRatio, "0");
};

void SaveOptionsTempoShape (HWND hwnd)
{
	int split = IsDlgButtonChecked(hwnd, IDC_BR_SHAPE_SPLIT);
	char splitRatio[128]; GetDlgItemText(hwnd, IDC_BR_SHAPE_SPLIT_RATIO, splitRatio, 128);

	char tmp[256];
	_snprintf(tmp, sizeof(tmp), "%d %s", split, splitRatio);
	WritePrivateProfileString("SWS", SHAPE_KEY, tmp, get_ini_file());	
};

int IsTempoShapeOptionsVisible (COMMAND_T*)
{
	return g_tempoShapeDialog;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//							Randomize Selected Tempo Markers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
WDL_DLGRET RandomizeTempoProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, uMsg, wParam, lParam))
		return r;
	
	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			// Get current tempo
			GetDeleteCurrentTempo(true);
			if (g_oldTempo == NULL) // unsuccessful? bail out!
				EndDialog(hwnd, 0);

			// Drop lists
			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_BR_RAND_UNIT));
			int x = (int)SendDlgItemMessage(hwnd, IDC_BR_RAND_UNIT, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("BPM","sws_DLG_171"));
			SendDlgItemMessage(hwnd, IDC_BR_RAND_UNIT, CB_SETITEMDATA, x, 0);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_RAND_UNIT, CB_ADDSTRING, 0, (LPARAM)"%");
			SendDlgItemMessage(hwnd, IDC_BR_RAND_UNIT, CB_SETITEMDATA, x, 1);
			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_BR_RAND_LIMIT_UNIT));
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_RAND_LIMIT_UNIT, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("BPM","sws_DLG_171"));
			SendDlgItemMessage(hwnd, IDC_BR_RAND_LIMIT_UNIT, CB_SETITEMDATA, x, 0);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_RAND_LIMIT_UNIT, CB_ADDSTRING, 0, (LPARAM)"%");
			SendDlgItemMessage(hwnd, IDC_BR_RAND_LIMIT_UNIT, CB_SETITEMDATA, x, 1);

			// Load options from .ini
			double min, max, minLimit, maxLimit;
			char eMin[128], eMax[128], eMinLimit[128], eMaxLimit[128];
			int unit, unitLimit, limit;
			LoadOptionsRandomizeTempo (min, max, unit, minLimit, maxLimit, unitLimit, limit);
			sprintf(eMin, "%.19g", min);
			sprintf(eMax, "%.19g", max);
			sprintf(eMinLimit, "%.19g", minLimit);
			sprintf(eMaxLimit, "%.19g", maxLimit);

			// Set controls
			SetDlgItemText(hwnd, IDC_BR_RAND_MIN, eMin);
			SetDlgItemText(hwnd, IDC_BR_RAND_MAX, eMax);
			SetDlgItemText(hwnd, IDC_BR_RAND_LIMIT_MIN, eMinLimit);
			SetDlgItemText(hwnd, IDC_BR_RAND_LIMIT_MAX, eMaxLimit);
			CheckDlgButton(hwnd, IDC_BR_RAND_LIMIT, !!limit);
			EnableWindow(GetDlgItem(hwnd, IDC_BR_RAND_LIMIT_MIN), !!limit);
			EnableWindow(GetDlgItem(hwnd, IDC_BR_RAND_LIMIT_MAX), !!limit);
			SendDlgItemMessage(hwnd, IDC_BR_RAND_UNIT, CB_SETCURSEL, unit, 0);
			SendDlgItemMessage(hwnd, IDC_BR_RAND_LIMIT_UNIT, CB_SETCURSEL, unitLimit, 0);
#ifndef _WIN32
			RECT r;
			int c = 2;
			GetWindowRect(GetDlgItem(hwnd, IDC_BR_RAND_UNIT), &r); ScreenToClient(hwnd, (LPPOINT)&r);
			SetWindowPos(GetDlgItem(hwnd, IDC_BR_RAND_UNIT), HWND_TOP, r.left, r.top+c, 0, 0, SWP_NOSIZE);
			GetWindowRect(GetDlgItem(hwnd, IDC_BR_RAND_LIMIT_UNIT), &r); ScreenToClient(hwnd, (LPPOINT)&r);
			SetWindowPos(GetDlgItem(hwnd, IDC_BR_RAND_LIMIT_UNIT), HWND_TOP, r.left, r.top+c, 0, 0, SWP_NOSIZE);
#endif
			// Set new random tempo and preview it
			SetRandomTempo(hwnd, min, max, unit, minLimit, maxLimit, unitLimit, limit);

			RestoreWindowPos(hwnd, RAND_WND_KEY, false);
		}
		break;
        
		case WM_COMMAND:
		{
            switch(LOWORD(wParam))
            {            	
				case IDC_BR_RAND_LIMIT:
				{
					int limit = IsDlgButtonChecked(hwnd, IDC_BR_RAND_LIMIT);
					EnableWindow(GetDlgItem(hwnd, IDC_BR_RAND_LIMIT_MIN), !!limit);
					EnableWindow(GetDlgItem(hwnd, IDC_BR_RAND_LIMIT_MAX), !!limit);
				}
				break;

				case IDC_BR_RAND_SEED:
				{
					// Get info from the dialog
					char eMin[128], eMax[128], eMinLimit[128], eMaxLimit[128];
					GetDlgItemText(hwnd, IDC_BR_RAND_MIN, eMin, 128);
					GetDlgItemText(hwnd, IDC_BR_RAND_MAX, eMax, 128);
					GetDlgItemText(hwnd, IDC_BR_RAND_LIMIT_MIN, eMinLimit, 128);
					GetDlgItemText(hwnd, IDC_BR_RAND_LIMIT_MAX, eMaxLimit, 128);

					double min = AltAtof(eMin);
					double max = AltAtof(eMax);
					double minLimit = AltAtof(eMinLimit);
					double maxLimit = AltAtof(eMaxLimit);
					int limit = IsDlgButtonChecked(hwnd, IDC_BR_RAND_LIMIT);
					int unit = (int)SendDlgItemMessage(hwnd,IDC_BR_RAND_UNIT,CB_GETCURSEL,0,0);
					int unitLimit = (int)SendDlgItemMessage(hwnd,IDC_BR_RAND_LIMIT_UNIT,CB_GETCURSEL,0,0);

					// Update edit boxes with "atofed" values
					sprintf(eMin, "%.19g", min);
					sprintf(eMax, "%.19g", max);
					sprintf(eMinLimit, "%.19g", minLimit);
					sprintf(eMaxLimit, "%.19g", maxLimit);
					SetDlgItemText(hwnd, IDC_BR_RAND_MIN, eMin);
					SetDlgItemText(hwnd, IDC_BR_RAND_MAX, eMax);
					SetDlgItemText(hwnd, IDC_BR_RAND_LIMIT_MIN, eMinLimit);
					SetDlgItemText(hwnd, IDC_BR_RAND_LIMIT_MAX, eMaxLimit);

					// Set new random tempo and preview it
					SetRandomTempo(hwnd, min, max, unit, minLimit, maxLimit, unitLimit, limit);
				}
				break;

				case IDOK:
				{
					GetDeleteCurrentTempo(false);
					SaveOptionsRandomizeTempo(hwnd);
					SaveWindowPos(hwnd, RAND_WND_KEY);
					EndDialog(hwnd, 0);
					
				}
				break;

				case IDCANCEL:
				{
					SetOldTempo();
					GetDeleteCurrentTempo(false);
					SaveOptionsRandomizeTempo(hwnd);
					SaveWindowPos(hwnd, RAND_WND_KEY);
					EndDialog(hwnd, 0);
				}														
				break;
			} 
		}
		break;		
	}
	return 0;
};

void GetDeleteCurrentTempo(bool get)
{
	if (get)
	{
		char* tempoEnv = GetSetObjectState(SWS_GetTrackEnvelopeByName(CSurf_TrackFromID(0, false), "Tempo map" ), "");
		size_t size = strlen(tempoEnv)+1;
		g_oldTempo = new (nothrow) char[size];
		if (g_oldTempo != NULL)
			memcpy(g_oldTempo, tempoEnv, size);
		FreeHeapPtr(tempoEnv);
	}
	else
	{
		if (g_oldTempo != NULL)
		{
			delete [] g_oldTempo;
			g_oldTempo = NULL;
		}
	}
};

void SetRandomTempo (HWND hwnd, double min, double max, int unit, double minLimit, double maxLimit, int unitLimit, int limit)
{

	// Get old tempo envelope
	size_t size = strlen(g_oldTempo)+1;
	char* oldTempo = new (nothrow) char[size];
	if (oldTempo != NULL)
		memcpy(oldTempo, g_oldTempo, size);
	else
	{
		EndDialog(hwnd, 0);
		return;
	}

	// Get TEMPO MARKERS timebase (not project timebase)
	int timeBase = *(int*)GetConfigVar("tempoenvtimelock");

	// Get time signature
	int pNum, pDen;
	double firstPoint;
	GetTempoTimeSigMarker(NULL, 0, &firstPoint, NULL, NULL, NULL, NULL, NULL, NULL);
	TimeMap_GetTimeSigAtTime(NULL, firstPoint, &pNum, &pDen, NULL);
	
	// Prepare variables
	double pTime = firstPoint;
	double pBpm = 1;
	int pShape = 1;
	double pOldTime = firstPoint;
	double pOldBpm = 1;
	int pOldShape = 1;

	// Loop tempo chunk searching for tempo markers
	WDL_FastString newState;
	LineParser lp(false);
	char* token = strtok(oldTempo, "\n");
	while(token != NULL)
	{
		lp.parse(token);
		if (strcmp(lp.gettoken_str(0),  "PT") == 0)
		{
			double cTime = lp.gettoken_float(1);
			double cBpm = lp.gettoken_float(2);
			int cShape = lp.gettoken_int(3);
			int cSig = lp.gettoken_int(4);
			int cSelected = lp.gettoken_int(5);
			int cPartial = lp.gettoken_int(6);
			
			// Save "soon to be old" values
			double oldTime = cTime;
			double oldBpm = cBpm;
			int oldShape = cShape;
									
			// If point is selected calculate it's new BPM
			if (cSelected == 1)
			{
				double random = (rand() % 101);
				random /= 100;
				
				double newBpm;
				// Calculate new bpm
				if (unit == 0) // Value
					newBpm = (cBpm + min) + ((max-min) * random);				
				else 		   // Percentage
					newBpm = (cBpm * (100 + min + random*(max-min)))/100;
				
				// Check against limits
				if (limit == 1)
				{
					// Value
					if (unitLimit == 0)
					{
						if (newBpm < minLimit)
							newBpm = minLimit;
						else if (newBpm > maxLimit)
							newBpm = maxLimit;
					}
					// Percentage
					else
					{
						if (newBpm < cBpm*(1 + minLimit/100))
							newBpm = cBpm*(1 + minLimit/100);
						else if (newBpm > cBpm*(1 + maxLimit/100))
							newBpm = cBpm*(1 + maxLimit/100);
	
					}
				}
				cBpm = newBpm;
				
				// Check if BPM is legal
				if (cBpm < 0.001)
					cBpm = 0.001;
				else if (cBpm > 960)
					cBpm = 960;
			}
			
			// Get new position but only if timebase is beats
			if (timeBase == 1)
			{
				double measure;
				if (pOldShape == 1)
					measure = (oldTime-pOldTime) * pOldBpm / 240;
				else
					measure = (oldTime-pOldTime) * (oldBpm+pOldBpm) / 480;
				
				if (pShape == 1)
					cTime = pTime + (240*measure) / pBpm;
				else
					cTime = pTime + (480*measure) / (pBpm + cBpm);
			}

			// Update tempo point
			newState.AppendFormatted(256, "PT %.16f %.16f %d %d %d %d\n", cTime, cBpm, cShape, cSig, cSelected, cPartial);

			// Update data on previous point
			pTime = cTime;
			pBpm = cBpm;
			pShape = cShape;
			pOldTime = oldTime;
			pOldBpm = oldBpm;
			pOldShape = oldShape;
		}
		else
		{
			newState.Append(token);
			newState.Append("\n");
		}			
		token = strtok(NULL, "\n");	
	}
	delete [] oldTempo;

	// Update tempo chunk
	GetSetObjectState(SWS_GetTrackEnvelopeByName(CSurf_TrackFromID(0, false), "Tempo map" ), newState.Get());

	// Refresh tempo map
	double t, b; int n, d; bool s;
	GetTempoTimeSigMarker(NULL, 0, &t, NULL, NULL, &b, &n, &d, &s);
	SetTempoTimeSigMarker(NULL, 0, t, -1, -1, b, n, d, s);
	UpdateTimeline();
};

void SetOldTempo ()
{
	// Set back the old tempo
	GetSetObjectState(SWS_GetTrackEnvelopeByName(CSurf_TrackFromID(0, false), "Tempo map" ), g_oldTempo);
	
	// Refresh tempo map
	double t, b; int n, d; bool s;
	GetTempoTimeSigMarker(NULL, 0, &t, NULL, NULL, &b, &n, &d, &s);
	SetTempoTimeSigMarker(NULL, 0, t, -1, -1, b, n, d, s);
	UpdateTimeline();	
};

void LoadOptionsRandomizeTempo (double &min, double &max, int &unit, double &minLimit, double &maxLimit, int &unitLimit, int &limit)
{
	char tmp[256];
	GetPrivateProfileString("SWS", RAND_KEY, "-1 1 0 40 260 0 0", tmp, 256, get_ini_file());
	sscanf(tmp, "%lf %lf %d %lf %lf %d %d", &min, &max, &unit, &minLimit, &maxLimit, &unitLimit, &limit);
	
	// Restore defaults if .ini has been tampered with
	if (unit != 0 && unit != 1)
		unit = 0;
	if (unitLimit != 0 && unitLimit != 1)
		unitLimit = 0;
	if (limit != 0 && limit != 1)
		limit = 0;	
};

void SaveOptionsRandomizeTempo (HWND hwnd)
{
	char eMin[128], eMax[128], eMinLimit[128], eMaxLimit[128];
	GetDlgItemText(hwnd, IDC_BR_RAND_MIN, eMin, 128);
	GetDlgItemText(hwnd, IDC_BR_RAND_MAX, eMax, 128);
	GetDlgItemText(hwnd, IDC_BR_RAND_LIMIT_MIN, eMinLimit, 128);
	GetDlgItemText(hwnd, IDC_BR_RAND_LIMIT_MAX, eMaxLimit, 128);
	
	double min = AltAtof(eMin);
	double max = AltAtof(eMax);
	double minLimit = AltAtof(eMinLimit);
	double maxLimit = AltAtof(eMaxLimit);
	int unit = (int)SendDlgItemMessage(hwnd,IDC_BR_RAND_UNIT,CB_GETCURSEL,0,0);
	int unitLimit = (int)SendDlgItemMessage(hwnd,IDC_BR_RAND_LIMIT_UNIT,CB_GETCURSEL,0,0);
	int limit = IsDlgButtonChecked(hwnd, IDC_BR_RAND_LIMIT);

	char tmp[256];
	_snprintf(tmp, sizeof(tmp), "%lf %lf %d %lf %lf %d %d", min, max, unit, minLimit, maxLimit, unitLimit, limit);
	WritePrivateProfileString("SWS", RAND_KEY, tmp, get_ini_file());	
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//							Convert Project Markers To Tempo Markers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
WDL_DLGRET ConvertMarkersToTempoProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, uMsg, wParam, lParam))
		return r;

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			// Drop down
			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_BR_CON_SPLIT_RATIO));
			int x = (int)SendDlgItemMessage(hwnd, IDC_BR_CON_SPLIT_RATIO, CB_ADDSTRING, 0, (LPARAM)"1/2");
			SendDlgItemMessage(hwnd, IDC_BR_CON_SPLIT_RATIO, CB_SETITEMDATA, x, 0);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_CON_SPLIT_RATIO, CB_ADDSTRING, 0, (LPARAM)"1/3");
			SendDlgItemMessage(hwnd, IDC_BR_CON_SPLIT_RATIO, CB_SETITEMDATA, x, 1);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_CON_SPLIT_RATIO, CB_ADDSTRING, 0, (LPARAM)"1/4");
			SendDlgItemMessage(hwnd, IDC_BR_CON_SPLIT_RATIO, CB_SETITEMDATA, x, 2);
			
			// Load options from .ini
			int markers, num, den, removeMarkers, timeSel, gradual, split;
			char eNum[128], eDen[128] , eMarkers[128], splitRatio[128];
			LoadOptionsConversion (markers, num, den, removeMarkers, timeSel, gradual, split, splitRatio, sizeof(splitRatio));
			sprintf(eNum, "%d", num);
			sprintf(eDen, "%d", den);
			sprintf(eMarkers, "%d", markers);
			
			// Set controls
			SetDlgItemText(hwnd, IDC_BR_CON_MARKERS, eMarkers);
			SetDlgItemText(hwnd, IDC_BR_CON_NUM, eNum);
			SetDlgItemText(hwnd, IDC_BR_CON_DEN, eDen);
			SetDlgItemText(hwnd, IDC_BR_CON_SPLIT_RATIO, splitRatio);
			CheckDlgButton(hwnd, IDC_BR_CON_REMOVE, !!removeMarkers);
			CheckDlgButton(hwnd, IDC_BR_CON_TIMESEL, !!timeSel);
			CheckDlgButton(hwnd, IDC_BR_CON_GRADUAL, !!gradual);
			CheckDlgButton(hwnd, IDC_BR_CON_SPLIT, !!split);
			EnableWindow(GetDlgItem(hwnd, IDC_BR_CON_SPLIT), !!gradual);
			EnableWindow(GetDlgItem(hwnd, IDC_BR_CON_SPLIT_RATIO), !!split);
#ifndef _WIN32
			RECT r;
			int c = 4;
			GetWindowRect(GetDlgItem(hwnd, IDC_BR_CON_MARKERS_STATIC), &r); ScreenToClient(hwnd, (LPPOINT)&r);
			SetWindowPos(GetDlgItem(hwnd, IDC_BR_CON_MARKERS_STATIC), HWND_BOTTOM, r.left, r.top+c, 0, 0, SWP_NOSIZE);
			GetWindowRect(GetDlgItem(hwnd, IDC_BR_CON_SIG_STATIC1), &r); ScreenToClient(hwnd, (LPPOINT)&r);
			SetWindowPos(GetDlgItem(hwnd, IDC_BR_CON_SIG_STATIC1), HWND_BOTTOM, r.left, r.top+c, 0, 0, SWP_NOSIZE);
			GetWindowRect(GetDlgItem(hwnd, IDC_BR_CON_SIG_STATIC2), &r); ScreenToClient(hwnd, (LPPOINT)&r);
			SetWindowPos(GetDlgItem(hwnd, IDC_BR_CON_SIG_STATIC2), HWND_BOTTOM, r.left, r.top+c, 0, 0, SWP_NOSIZE);
			GetWindowRect(GetDlgItem(hwnd, IDC_BR_CON_MARKERS), &r); ScreenToClient(hwnd, (LPPOINT)&r);
			SetWindowPos(GetDlgItem(hwnd, IDC_BR_CON_MARKERS), HWND_TOP, r.left, r.top+c, 0, 0, SWP_NOSIZE);
			GetWindowRect(GetDlgItem(hwnd, IDC_BR_CON_NUM), &r); ScreenToClient(hwnd, (LPPOINT)&r);
			SetWindowPos(GetDlgItem(hwnd, IDC_BR_CON_NUM), HWND_TOP, r.left, r.top+c, 0, 0, SWP_NOSIZE);
			GetWindowRect(GetDlgItem(hwnd, IDC_BR_CON_DEN), &r); ScreenToClient(hwnd, (LPPOINT)&r);
			SetWindowPos(GetDlgItem(hwnd, IDC_BR_CON_DEN), HWND_TOP, r.left, r.top+c, 0, 0, SWP_NOSIZE);
			GetWindowRect(GetDlgItem(hwnd, IDC_BR_CON_GRADUAL), &r); ScreenToClient(hwnd, (LPPOINT)&r);
			SetWindowPos(GetDlgItem(hwnd, IDC_BR_CON_GRADUAL), HWND_TOP, r.left, r.top+c, 0, 0, SWP_NOSIZE);
			c = 3;
			GetWindowRect(GetDlgItem(hwnd, IDC_BR_CON_REMOVE), &r); ScreenToClient(hwnd, (LPPOINT)&r);
			SetWindowPos(GetDlgItem(hwnd, IDC_BR_CON_REMOVE), HWND_TOP, r.left, r.top+c, 0, 0, SWP_NOSIZE);
			GetWindowRect(GetDlgItem(hwnd, IDC_BR_CON_TIMESEL), &r); ScreenToClient(hwnd, (LPPOINT)&r);
			SetWindowPos(GetDlgItem(hwnd, IDC_BR_CON_TIMESEL), HWND_TOP, r.left, r.top+c, 0, 0, SWP_NOSIZE);
			GetWindowRect(GetDlgItem(hwnd, IDC_BR_CON_SPLIT), &r); ScreenToClient(hwnd, (LPPOINT)&r);
			SetWindowPos(GetDlgItem(hwnd, IDC_BR_CON_SPLIT), HWND_TOP, r.left, r.top+c, 0, 0, SWP_NOSIZE);
			GetWindowRect(GetDlgItem(hwnd, IDC_BR_CON_SPLIT_RATIO), &r); ScreenToClient(hwnd, (LPPOINT)&r);
			SetWindowPos(GetDlgItem(hwnd, IDC_BR_CON_SPLIT_RATIO), HWND_TOP, r.left, r.top+c, 0, 0, SWP_NOSIZE);
#endif
			if (gradual == 0) {ShowGradualOptions(false, hwnd);}									
			RestoreWindowPos(hwnd, CONV_WND_KEY, false);
		}
		break;
        
		case WM_COMMAND :
		{
            switch(LOWORD(wParam))
            {
                case IDOK:
				{
					// Edit boxes and dropdown
					char eNum[128], eDen[128], eMarkers[128], splitRatio[128];
					double convertedSplitRatio;
					GetDlgItemText(hwnd, IDC_BR_CON_NUM, eNum, 128);
					GetDlgItemText(hwnd, IDC_BR_CON_DEN, eDen, 128);
					GetDlgItemText(hwnd, IDC_BR_CON_MARKERS, eMarkers, 128);
					GetDlgItemText(hwnd, IDC_BR_CON_SPLIT_RATIO, splitRatio, 128);
					
					int num = atoi(eNum); if (num > 255){num = 255;}
					int den = atoi(eDen); if (den > 255){den = 255;}
					
					int markers = atoi(eMarkers);
					IsThisFraction (splitRatio, sizeof(splitRatio), convertedSplitRatio);
					if (convertedSplitRatio <= 0 || convertedSplitRatio >= 1)
					{
						convertedSplitRatio = 0;
						strcpy(splitRatio, "0");
					}
										
					// Check boxes				
					int removeMarkers = IsDlgButtonChecked(hwnd, IDC_BR_CON_REMOVE);
					int timeSel = IsDlgButtonChecked(hwnd, IDC_BR_CON_TIMESEL);
					int gradual = IsDlgButtonChecked(hwnd, IDC_BR_CON_GRADUAL);
					int split = IsDlgButtonChecked(hwnd, IDC_BR_CON_SPLIT);
															
					// Update edit boxes and dropdown to show "atoied" value
					sprintf(eNum, "%d", num);
					sprintf(eDen, "%d", den);
					sprintf(eMarkers, "%d", markers);
					SetDlgItemText(hwnd, IDC_BR_CON_NUM, eNum);
					SetDlgItemText(hwnd, IDC_BR_CON_DEN, eDen);
					SetDlgItemText(hwnd, IDC_BR_CON_MARKERS, eMarkers);
					SetDlgItemText(hwnd, IDC_BR_CON_SPLIT_RATIO, splitRatio);

					// Check values
					int i = 0;
					if (num <= 0 || den <= 0 || markers <= 0)
					{	
						i = 1;
						while (true)
						{
							if (markers <= 0)
							{
								SetFocus(GetDlgItem(hwnd, IDC_BR_CON_MARKERS));
								SendMessage(GetDlgItem(hwnd, IDC_BR_CON_MARKERS), EM_SETSEL, 0, -1);
								break;
							}
							if (num <= 0)
							{
								SetFocus(GetDlgItem(hwnd, IDC_BR_CON_NUM));
								SendMessage(GetDlgItem(hwnd, IDC_BR_CON_NUM), EM_SETSEL, 0, -1);
								break;
							}
							if (den <= 0)
							{	
								SetFocus(GetDlgItem(hwnd, IDC_BR_CON_DEN));
								SendMessage(GetDlgItem(hwnd, IDC_BR_CON_DEN), EM_SETSEL, 0, -1);
								break;
							}
						}
						MessageBox(g_hwndParent, __LOCALIZE("All values have to be positive integers.","sws_DLG_166"), __LOCALIZE("SWS - Error","sws_mbox"), MB_OK);
						SetFocus(hwnd);
					}
					
					// If all went well, start converting
					if (wParam == IDOK && i == 0)
					{
						if (convertedSplitRatio == 0)
							split = 0;
						if (!IsWindowEnabled(GetDlgItem(hwnd, IDC_BR_CON_TIMESEL)))
							timeSel = 0;
						ConvertMarkersToTempo(markers, num, den, removeMarkers, timeSel, gradual, split, convertedSplitRatio);
					}
				}
				break;
			
				case IDC_BR_CON_GRADUAL:
				{
					int gradual = IsDlgButtonChecked(hwnd, IDC_BR_CON_GRADUAL);
					EnableWindow(GetDlgItem(hwnd, IDC_BR_CON_SPLIT), !!gradual);
					ShowGradualOptions(!!gradual, hwnd);
					
				}
				break;

				case IDC_BR_CON_SPLIT:
				{
					int split = IsDlgButtonChecked(hwnd, IDC_BR_CON_SPLIT);
					EnableWindow(GetDlgItem(hwnd, IDC_BR_CON_SPLIT_RATIO), !!split);	
				}
				break;

				case IDCANCEL:
				{
					g_convertMarkersToTempoDialog = false;
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
				EnableWindow(GetDlgItem(hwnd, IDC_BR_CON_TIMESEL), false);
			else
				EnableWindow(GetDlgItem(hwnd, IDC_BR_CON_TIMESEL), true);			
		}
		break;

		case WM_DESTROY:
		{
			SaveWindowPos(hwnd, CONV_WND_KEY);
			SaveOptionsConversion (hwnd);
		}
		break; 
	}
	return 0;
};

void ConvertMarkersToTempo (int markers, int num, int den, int removeMarkers, int timeSel, int gradualTempo, int split, double splitRatio)
{
	// Get time selection
	double tStart, tEnd;
	if (timeSel == 1)
		GetSet_LoopTimeRange2 (NULL, false, false, &tStart, &tEnd, false);
		
	// Get project markers
	vector<double> markerPositions;
	bool region, prevRegion;
	double mPos, prevMPos;
	int current, previous, markerId, i = 0;
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
				if (timeSel == 0)
					markerPositions.push_back(mPos);
				else
				{
					if (mPos > tEnd)
						break;
					if (mPos >= tStart)
						markerPositions.push_back(mPos);
				}
			}	
		}
		++i;
	}
	
	// Check number of markers
	if (markerPositions.size() <=1)
	{
		if (timeSel == 0)
		{
			MessageBox(g_hwndParent, __LOCALIZE("Not enough project markers in the project to perform conversion.", "sws_DLG_166"), __LOCALIZE("SWS - Error", "sws_mbox"), MB_OK);	
			return;
		}
		if (timeSel == 1)
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
				if (*(int*)GetConfigVar("tempoenvtimelock") == 0) 	// when tempo envelope timebase is time, these tempo markers
					break;											// can't be moved so there is no need to warn user
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
	
	// CREATE SQUARE POINTS
	if (gradualTempo == 0)
	{
		// Set first tempo marker with time signature
		double length = markerPositions[1] - markerPositions[0];
		double measure = num / (den * (double)markers);
		double bpm = 240*measure / length;
		if (bpm > 960)
			++exceed;
		SetTempoTimeSigMarker (NULL, -1, markerPositions[0], -1, -1, bpm, num, den, false);

		// Set the rest of the tempo markers. 
		for (size_t i = 1; i+1 < markerPositions.size(); ++i)
		{ 
			length = markerPositions[i+1] - markerPositions[i];
			bpm = 240*measure / length;
			if (bpm > 960)
				++exceed;
			SetTempoTimeSigMarker (NULL, -1, markerPositions[i], -1, -1, bpm, 0, 0, false);
		}
	}

	// CREATE LINEAR POINTS
	else
	{
		// Get linear points' bpm (these get put where project markers are)
		vector <double> linearPoints;
		double measure = num / (den * (double)markers);
		double bpm_1;
		for (size_t i = 1; i < markerPositions.size(); ++i)
		{ 
			double length = markerPositions[i] - markerPositions[i-1];
			double bpm_2 = (240 * measure) / length;
			if (i == 1 || (timeSel == 1 && i+1 == markerPositions.size()) )	// First marker is the same as a square point marker						
				linearPoints.push_back(bpm_2);								// Last marker is the same as square point marker only if
			else															// if converting within time selection (to escape rounding
				linearPoints.push_back((bpm_2 + bpm_1) / 2);				// issues) and it is positioned on last-1 project marker	
			bpm_1 = bpm_2; 
		}
		if (timeSel == 0) // Get tempo point for the last PROJECT marker only if converting whole project
		{
			size_t i = markerPositions.size() - 1;
			double length = markerPositions[i] - markerPositions[i-1];
			double bpm_2 = (240 * measure) / length;
			linearPoints.push_back((bpm_2 + bpm_1) / 2);
		}
	
		// Set first marker with time signature		
		SetTempoTimeSigMarker (NULL, -1, markerPositions[0], -1, -1, linearPoints[0], num, den, true);
		
		// Set the rest of the points
		for(size_t i = 1; i+1 <= linearPoints.size(); ++i)
		{
			// Get middle point's position and BPM
			measure = num / (den * (double)markers);
			double position, bpm;
			FindMiddlePoint(position, bpm, measure, linearPoints[i-1], linearPoints[i], markerPositions[i-1], markerPositions[i]);
			
			// Set middle point
			if (split == 0)
			{	
				SetTempoTimeSigMarker (NULL, -1, position, -1, -1, bpm, 0, 0, true);
				if (bpm > 960)
					++exceed;
				measure /=2; // Used for checking at the end
			}
				
			// Or split it
			else
			{	double position1, position2, bpm1, bpm2;
				SplitMiddlePoint (position1, position2, bpm1, bpm2, splitRatio, measure, linearPoints[i-1], bpm, linearPoints[i], markerPositions[i-1], position, markerPositions[i]);
				
				SetTempoTimeSigMarker (NULL, -1, position1, -1, -1, bpm1, 0, 0, true);
				SetTempoTimeSigMarker (NULL, -1, position2, -1, -1, bpm2, 0, 0, true);
				if (bpm1 > 960 || bpm2 > 960)
					++exceed;				
				measure = measure*(1-splitRatio)/2; // Used for 
				position = position2;				// checking 
				bpm = bpm2;							// at the end
			}
								
			// Previously calculated BPM for marker position currently in the loop
			// may need to get readjusted. All though first error may seem really
			// small, it can add up and create quite a mess
			bpm = (480 * measure) / (markerPositions[i] - position) - bpm;
			if(bpm != linearPoints[i])
				linearPoints[i] = bpm;

			// Last point created will have square shape only when converting within time selection
			if (timeSel == 1 && i+1 == linearPoints.size())
				SetTempoTimeSigMarker (NULL, -1, markerPositions[i], -1, -1, linearPoints[i], 0, 0, false);
			else
				SetTempoTimeSigMarker (NULL, -1, markerPositions[i], -1, -1, linearPoints[i], 0, 0, true);
			if (linearPoints[i] > 960)
				++exceed;			
		}
	}

	UpdateTimeline();
	Undo_EndBlock2 (NULL, __LOCALIZE("Convert project markers to tempo markers","sws_undo"), UNDO_STATE_ALL);

	// Remove markers
	if (removeMarkers == 1)
	{
		Undo_BeginBlock2(NULL);
		if (timeSel == 0)
		{	
			DeleteAllMarkers();
			Undo_EndBlock2 (NULL, __LOCALIZE("Delete all project markers","sws_undo"), UNDO_STATE_ALL);
		}
		if (timeSel == 1)
		{	 
			Main_OnCommand(40420, 0);
			Undo_EndBlock2 (NULL, __LOCALIZE("Delete project markers within time selection","sws_undo"), UNDO_STATE_ALL);
		}
	}

	// Warn user if there were tempo markers created with a BPM over 960
	if (exceed != 0)
		ShowMessageBox(__LOCALIZE("Some of the created tempo markers have a BPM over 960. If you try to edit them, they will revert back to 960 or lower.\n\nIt is recommended that you undo, edit project markers and try again.", "sws_DLG_166"),__LOCALIZE("SWS - Warning", "sws_mbox"), 0);
};

void ShowGradualOptions (bool show, HWND hwnd)
{
	int c;
	if (!show)
	{
		ShowWindow(GetDlgItem(hwnd, IDC_BR_CON_SPLIT), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_CON_SPLIT_RATIO), SW_HIDE);
		c = -29;
	}

	else
	{
		ShowWindow(GetDlgItem(hwnd, IDC_BR_CON_SPLIT), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_CON_SPLIT_RATIO), SW_SHOW);
		c = 29;			
	}

	RECT r;	
	
	// Move/resize group boxes
	GetWindowRect(GetDlgItem(hwnd, IDC_BR_CON_GROUP2), &r);
	SetWindowPos(GetDlgItem(hwnd, IDC_BR_CON_GROUP2), HWND_BOTTOM, 0, 0, r.right-r.left, r.bottom-r.top+c, SWP_NOMOVE);

	// Move buttons
	GetWindowRect(GetDlgItem(hwnd, IDOK), &r); ScreenToClient(hwnd, (LPPOINT)&r);
	SetWindowPos(GetDlgItem(hwnd, IDOK), NULL, r.left, r.top+c, 0, 0, SWP_NOSIZE);	   
	GetWindowRect(GetDlgItem(hwnd, IDCANCEL), &r); ScreenToClient(hwnd, (LPPOINT)&r);
	SetWindowPos(GetDlgItem(hwnd, IDCANCEL), NULL, r.left, r.top+c, 0, 0, SWP_NOSIZE);
	
	// Resize window
	GetWindowRect(hwnd, &r);
#ifdef _WIN32
	    SetWindowPos(hwnd, NULL, 0, 0, r.right-r.left, r.bottom-r.top+c, SWP_NOMOVE);
#else
	    SetWindowPos(hwnd, NULL, r.left, r.top-c,  r.right-r.left, r.bottom-r.top+c, NULL);
#endif
};

void LoadOptionsConversion (int &markers, int &num, int &den, int &removeMarkers, int &timeSel, int &gradual, int &split, char* splitRatio, int ratioLen)
{
	char tmp[256];
	GetPrivateProfileString("SWS", CONV_KEY, "4 4 4 1 0 0 0 4/8", tmp, 256, get_ini_file());
	sscanf(tmp, "%d %d %d %d %d %d %d %s", &markers, &num, &den, &removeMarkers, &timeSel, &gradual, &split, splitRatio);
	
	// Restore defaults if .ini has been tampered with
	double convertedRatio;
	IsThisFraction (splitRatio, ratioLen, convertedRatio);
	if (markers <= 0)
		markers = 4;
	if (num <= 0 || num > 255)
		num = 4;
	if (den <= 0 || den > 255)
		den = 4;
	if (removeMarkers != 0 && removeMarkers != 1)
		removeMarkers = 1;
	if (timeSel != 0 && timeSel != 1)
		timeSel = 0;
	if (gradual != 0 && gradual != 1)
		gradual = 0;
	if (split != 0 && split != 1)
		split = 0;
	if (convertedRatio <= 0 || convertedRatio >= 1)
		strcpy(splitRatio, "0");
};

void SaveOptionsConversion (HWND hwnd)
{
	char eNum[128], eDen[128], eMarkers[128], splitRatio[128];
	GetDlgItemText(hwnd, IDC_BR_CON_NUM, eNum, 128);
	GetDlgItemText(hwnd, IDC_BR_CON_DEN, eDen, 128);
	GetDlgItemText(hwnd, IDC_BR_CON_MARKERS, eMarkers, 128);
	GetDlgItemText(hwnd, IDC_BR_CON_SPLIT_RATIO, splitRatio, 128);
	int num = atoi(eNum);
	int den = atoi(eDen);
	int markers = atoi(eMarkers);
	int removeMarkers = IsDlgButtonChecked(hwnd, IDC_BR_CON_REMOVE);
	int timeSel = IsDlgButtonChecked(hwnd, IDC_BR_CON_TIMESEL);
	int gradual = IsDlgButtonChecked(hwnd, IDC_BR_CON_GRADUAL);
	int split = IsDlgButtonChecked(hwnd, IDC_BR_CON_SPLIT);

	char tmp[256];
	_snprintf(tmp, sizeof(tmp), "%d %d %d %d %d %d %d %s", markers, num, den, removeMarkers, timeSel, gradual, split, splitRatio);
	WritePrivateProfileString("SWS", CONV_KEY, tmp, get_ini_file());	
};

int IsConvertMarkersToTempoVisible (COMMAND_T*)
{
	return g_convertMarkersToTempoDialog;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//							Select And Adjust Selected Tempo Markers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
WDL_DLGRET SelectAdjustTempoProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, uMsg, wParam, lParam))
		return r;

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			// Drop downs
			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_BR_SEL_TIME_RANGE));
			int x = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_TIME_RANGE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("All","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TIME_RANGE, CB_SETITEMDATA, x, 0);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_TIME_RANGE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Time selection","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TIME_RANGE, CB_SETITEMDATA, x, 1);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_TIME_RANGE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Ignore time selection","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TIME_RANGE, CB_SETITEMDATA, x, 2);
			
			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_BR_SEL_SHAPE));
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_SHAPE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("All","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_SHAPE, CB_SETITEMDATA, x, 0);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_SHAPE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Square","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_SHAPE, CB_SETITEMDATA, x, 1);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_SHAPE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Linear","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_SHAPE, CB_SETITEMDATA, x, 2);
			
			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_BR_SEL_TYPE_DEF));
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_TYPE_DEF, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("All","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TYPE_DEF, CB_SETITEMDATA, x, 0);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_TYPE_DEF, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Tempo markers","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TYPE_DEF, CB_SETITEMDATA, x, 1);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_TYPE_DEF, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Time signature markers","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TYPE_DEF, CB_SETITEMDATA, x, 2);
			
			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_BR_ADJ_SHAPE));
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_ADJ_SHAPE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Preserve","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_ADJ_SHAPE, CB_SETITEMDATA, x, 0);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_ADJ_SHAPE,CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Invert","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_ADJ_SHAPE, CB_SETITEMDATA, x, 1);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_ADJ_SHAPE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Linear","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_ADJ_SHAPE, CB_SETITEMDATA, x, 2);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_ADJ_SHAPE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Square","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_ADJ_SHAPE, CB_SETITEMDATA, x, 3);
			
			// Load options from .ini
			double bpmStart, bpmEnd;
			char eBpmStart[128], eBpmEnd[128], eNum[128], eDen[128];
			int num, den, bpmEnb, sigEnb, timeSel, shape, type, selPref, invertPref, adjustType, adjustShape;
			LoadOptionsSelAdj (bpmStart, bpmEnd, num, den, bpmEnb, sigEnb, timeSel, shape, type, selPref, invertPref, adjustType, adjustShape);
			sprintf(eBpmStart, "%.19g", bpmStart);
			sprintf(eBpmEnd, "%.19g", bpmEnd);
			sprintf(eNum, "%d", num);
			sprintf(eDen, "%d", den);

			// Find tempo at cursor
			char bpmCursor[128]; double effBpmCursor;
			TimeMap_GetTimeSigAtTime(NULL, GetCursorPositionEx(NULL), NULL, NULL, &effBpmCursor);
			sprintf(bpmCursor, "%.16g", effBpmCursor);

			// Set controls		
			SetDlgItemText(hwnd, IDC_BR_SEL_BPM_START, eBpmStart);
			SetDlgItemText(hwnd, IDC_BR_SEL_BPM_END, eBpmEnd);
			SetDlgItemText(hwnd, IDC_BR_SEL_SIG_NUM, eNum);
			SetDlgItemText(hwnd, IDC_BR_SEL_SIG_DEN, eDen);
			SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_1, "0");			
			SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_2, bpmCursor);
			SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_3, "0");
			SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_TAR_1, "0");			
			SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_TAR_2, "0");
			SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_TAR_3, "0");
			SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_VAL, "0");
			SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_PERC, "0");
			CheckDlgButton(hwnd, IDC_BR_SEL_BPM, !!bpmEnb);
			CheckDlgButton(hwnd, IDC_BR_SEL_SIG, !!sigEnb);
			CheckDlgButton(hwnd, IDC_BR_SEL_ADD, !!selPref);
			CheckDlgButton(hwnd, IDC_BR_SEL_INVERT_PREF, !!invertPref);
			CheckDlgButton(hwnd, IDC_BR_ADJ_BPM_VAL_ENB, !!adjustType);
			CheckDlgButton(hwnd, IDC_BR_ADJ_BPM_PERC_ENB, !adjustType);
			EnableWindow(GetDlgItem(hwnd, IDC_BR_SEL_BPM_START), !!bpmEnb);
			EnableWindow(GetDlgItem(hwnd, IDC_BR_SEL_BPM_END), !!bpmEnb);
			EnableWindow(GetDlgItem(hwnd, IDC_BR_SEL_SIG_NUM), !!sigEnb);
			EnableWindow(GetDlgItem(hwnd, IDC_BR_SEL_SIG_DEN), !!sigEnb);
			EnableWindow(GetDlgItem(hwnd, IDC_BR_ADJ_BPM_VAL), !!adjustType);
			EnableWindow(GetDlgItem(hwnd, IDC_BR_ADJ_BPM_PERC), !adjustType);
			SendDlgItemMessage(hwnd, IDC_BR_SEL_SHAPE, CB_SETCURSEL, shape, 0);
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TYPE_DEF, CB_SETCURSEL, type, 0);
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TIME_RANGE, CB_SETCURSEL, timeSel, 0);
			SendDlgItemMessage(hwnd, IDC_BR_ADJ_SHAPE, CB_SETCURSEL, adjustShape, 0);
#ifndef _WIN32
			RECT r;
			GetWindowRect(GetDlgItem(hwnd, IDC_BR_SEL_BPM_STATIC), &r); ScreenToClient(hwnd, (LPPOINT)&r);
			SetWindowPos(GetDlgItem(hwnd, IDC_BR_SEL_BPM_STATIC), HWND_BOTTOM, r.left-3, r.top+2, 0, 0, SWP_NOSIZE);
			GetWindowRect(GetDlgItem(hwnd, IDC_BR_SEL_SIG_STATIC), &r); ScreenToClient(hwnd, (LPPOINT)&r);
			SetWindowPos(GetDlgItem(hwnd, IDC_BR_SEL_SIG_STATIC), HWND_BOTTOM, r.left-3, r.top, 0, 0, SWP_NOSIZE);
#endif 
			RestoreWindowPos(hwnd, SEL_ADJ_WND_KEY, false);		
		}
		break;
        
		case WM_COMMAND:
		{
            switch(LOWORD(wParam))
            {
				////// SELECT MARKERS //////

				// Check boxes
				case IDC_BR_SEL_BPM:
				{
					int x = IsDlgButtonChecked(hwnd, IDC_BR_SEL_BPM);
					EnableWindow(GetDlgItem(hwnd, IDC_BR_SEL_BPM_START), !!x);
					EnableWindow(GetDlgItem(hwnd, IDC_BR_SEL_BPM_END), !!x);
				}
				break;
				
				case IDC_BR_SEL_SIG:
				{
					int x = IsDlgButtonChecked(hwnd, IDC_BR_SEL_SIG);
					EnableWindow(GetDlgItem(hwnd, IDC_BR_SEL_SIG_NUM), !!x);
					EnableWindow(GetDlgItem(hwnd, IDC_BR_SEL_SIG_DEN), !!x);
				}
				break;

				// Control buttons
				case IDC_BR_SEL_SELECT:
				{
					Undo_BeginBlock2(NULL);
					UpdateSelectionFields (hwnd);
					SelectTempoCase (hwnd, 0, 0);
					Undo_EndBlock2 (NULL, __LOCALIZE("Select tempo markers","sws_undo"), UNDO_STATE_ALL);
				}
				break;

				case IDC_BR_SEL_DESELECT:
				{
					Undo_BeginBlock2(NULL);
					UpdateSelectionFields (hwnd);
					SelectTempoCase (hwnd, 1, 0);
					Undo_EndBlock2 (NULL, __LOCALIZE("Deselect tempo markers","sws_undo"), UNDO_STATE_ALL);
				}
				break;

				case IDC_BR_SEL_INVERT:
				{
					Undo_BeginBlock2(NULL);
					UpdateSelectionFields (hwnd);
					if (IsDlgButtonChecked(hwnd, IDC_BR_SEL_INVERT_PREF) == 0)
						SelectTempo (1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
					else
						SelectTempoCase (hwnd, 2, 0);
					Undo_EndBlock2 (NULL, __LOCALIZE("Invert selection of tempo markers","sws_undo"), UNDO_STATE_ALL);
				}
				break;
				
				case IDC_BR_SEL_CLEAR:
				{
					Undo_BeginBlock2(NULL);
					UpdateSelectionFields (hwnd);
					SelectTempo (0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
					Undo_EndBlock2 (NULL, __LOCALIZE("Deselect tempo markers","sws_undo"), UNDO_STATE_ALL);
				}
				break;

				case IDC_BR_SEL_DESELECT_NTH:
				{
					CallDeselTempoDialog(true, hwnd); // show child dialog and pass parent's handle to it
				}
				break;
				
				
				////// ADJUST MARKERS //////
				
				// Radio buttons
				case IDC_BR_ADJ_BPM_VAL_ENB:
				{
					int enb = IsDlgButtonChecked(hwnd, IDC_BR_ADJ_BPM_VAL_ENB);
					EnableWindow(GetDlgItem(hwnd, IDC_BR_ADJ_BPM_VAL), !!enb);
					EnableWindow(GetDlgItem(hwnd, IDC_BR_ADJ_BPM_PERC), !enb);
					UpdateTargetBpm(hwnd, 1, 1, 1);
				}
				break;

				case IDC_BR_ADJ_BPM_PERC_ENB:
				{
					int enb = IsDlgButtonChecked(hwnd, IDC_BR_ADJ_BPM_PERC_ENB);
					EnableWindow(GetDlgItem(hwnd, IDC_BR_ADJ_BPM_PERC), !!enb);
					EnableWindow(GetDlgItem(hwnd, IDC_BR_ADJ_BPM_VAL), !enb);
					UpdateTargetBpm(hwnd, 1, 1, 1);
				}
				break;

				// Edit boxes				
				case IDC_BR_ADJ_BPM_VAL:
				{
					if (HIWORD(wParam) == EN_CHANGE && GetFocus() == GetDlgItem(hwnd, IDC_BR_ADJ_BPM_VAL))
						UpdateTargetBpm(hwnd, 1, 1, 1);
				}
				break;

				case IDC_BR_ADJ_BPM_PERC:
				{
					if (HIWORD(wParam) == EN_CHANGE && GetFocus() == GetDlgItem(hwnd, IDC_BR_ADJ_BPM_PERC))
						UpdateTargetBpm(hwnd, 1, 1, 1);					
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
												
						bpmVal = AltAtof(bpmTar) - atof(bpmCur);
						if (atof(bpmCur) != 0)
							bpmPerc = bpmVal / atof(bpmCur) * 100;
						else
							bpmPerc = 0;
						
						sprintf(bpmTar, "%.6g", bpmVal);
						sprintf(bpmCur, "%.6g", bpmPerc);
						SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_VAL, bpmTar);
						SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_PERC, bpmCur);
						UpdateTargetBpm(hwnd, 0, 1, 1);
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
												
						bpmVal = AltAtof(bpmTar) - atof(bpmCur);
						if (atof(bpmCur) != 0)
							bpmPerc = bpmVal / atof(bpmCur) * 100;
						else
							bpmPerc = 0;
						
						sprintf(bpmTar, "%.6g", bpmVal);
						sprintf(bpmCur, "%.6g", bpmPerc);
						SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_VAL, bpmTar);
						SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_PERC, bpmCur);
						UpdateTargetBpm(hwnd, 1, 0, 1);
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
													
						bpmVal = AltAtof(bpmTar) - atof(bpmCur);
						if (atof(bpmCur) != 0)
							bpmPerc = bpmVal / atof(bpmCur) * 100;
						else
							bpmPerc = 0;
						
						sprintf(bpmTar, "%.6g", bpmVal);
						sprintf(bpmCur, "%.6g", bpmPerc);
						SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_VAL, bpmTar);
						SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_PERC, bpmCur);
						UpdateTargetBpm(hwnd, 1, 1, 0);
					}
				}
				break;

				// Control buttons
				case IDC_BR_ADJ_APPLY:
				{
					AdjustTempoCase (hwnd);										
				}
				break;

				case IDCANCEL:
				{
					KillTimer(hwnd, 1);
					g_selectAdjustTempoDialog = false;
					CallDeselTempoDialog(false, hwnd); // hide child dialog and past parent's handle to it
					ShowWindow(hwnd, SW_HIDE);					
				}				
				break;
			} 
		}
		break;

		case WM_TIMER:
		{ 
			// Get number of selected points and set Windows caption
			vector<int> selectedPoints = GetSelectedTempo();
			char pointCount[512];
			_snprintf(pointCount, sizeof(pointCount), __LOCALIZE_VERFMT("SWS Select and adjust tempo markers (%d of %d points selected)","sws_DLG_167") , selectedPoints.size(), CountTempoTimeSigMarkers(NULL) );
			SetWindowText(hwnd,pointCount);

			// Update current and target edit boxes
			UpdateCurrentBpm(hwnd, selectedPoints);			
		}
		break;

		case WM_DESTROY:
		{
			SaveWindowPos(hwnd, SEL_ADJ_WND_KEY);
			SaveOptionsSelAdj (hwnd);
		}
		break; 
	}
	return 0;
};

void SelectTempo (int mode, int Nth, int timeSel, int bpm, double bpmStart, double bpmEnd, int shape, int sig, int num, int den, int type)
{
	/*
	mode	 ---> Ignore criteria
				--> 0 to clear
				--> 1 to invert selection
				--> 2 to deselect every nth selected point

			 ---> Check by criteria before selecting
		   		--> 3 to set new
		   		--> 4 to add to existing selection
				--> 5 to deselect
				--> 6 to deselect every Nth selected marker
				--> 7 to invert

	criteria ---> Nth	  ---> ordinal number of the selected point
			 ---> timeSel ---> 0 for all, 1 for time selection, 2 to exclude time selection
			 ---> bpm     ---> 0 to ignore BPM, if 1 BpmStart and BpmEnd need to be specified
			 ---> shape   ---> 0 for all, 1 for square, 2 for linear
			 ---> sig     ---> 0 to ignore sig, if 1 num and den need to be specified
			 ---> type    ---> 0 for all, 1 for tempo markers only, 2 for time signature only
	*/

	// Get time selection
	double tStart, tEnd;
	GetSet_LoopTimeRange2 (NULL, false, false, &tStart, &tEnd, false);
	
	// Get tempo chunk
	TrackEnvelope* envelope = SWS_GetTrackEnvelopeByName(CSurf_TrackFromID(0, false), "Tempo map" );
	char* envState = GetSetObjectState(envelope, "");
	char* token = strtok(envState, "\n");

	// Loop tempo chunk searching for tempo markers
	WDL_FastString newState;
	int currentNth = 1;
	LineParser lp(false);
	
	while(token != NULL)
	{
		lp.parse(token);
		if (strcmp(lp.gettoken_str(0),  "PT") == 0)
		{	
			double cTime = lp.gettoken_float(1);
			double cBpm = lp.gettoken_float(2);
			int cShape = lp.gettoken_int(3);
			int cSig = lp.gettoken_int(4);
			int cSelected = lp.gettoken_int(5);
			int cPartial = lp.gettoken_int(6);
			
			
			// Clear selected points
			if (mode == 0)
				cSelected = 0;

			// Invert selected points
			else if (mode == 1)
			{	
				if (cSelected == 1)
					cSelected = 0;
				else
					cSelected = 1;
			}

			// Deselect every Nth selected point
			else if (mode == 2)
			{
				if (cSelected == 1)
				{
					if (currentNth == Nth)
						cSelected = 0;
					++currentNth;
					if(currentNth > Nth)
						currentNth = 1;
				}
			}
			
			// Select/Deselect points by criteria
			else
			{
				bool selectPt = true;
				
				//// Check if point conforms to criteria

				// Check BPM
				if (selectPt)
				{
					if (bpm == 0)
						selectPt = true;
					
					else
					{
						if(cBpm >= bpmStart && cBpm <= bpmEnd )
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
					else
					{
						int effNum, effDen;
						TimeMap_GetTimeSigAtTime(NULL, cTime, &effNum, &effDen, NULL);
						if (num == effNum && den == effDen)
							selectPt = true;
						else
							selectPt = false;
					}
				}

				// Check time
				if (selectPt)
				{
					if (timeSel == 0) 
						selectPt = true;
					
					else if (timeSel == 1)
					{
						if (cTime >= tStart && cTime <= tEnd)
							selectPt = true;
						else
							selectPt = false;
					}
					
					else if (timeSel == 2)
					{
						if (cTime >= tStart && cTime <= tEnd)
							selectPt = false;
						else
							selectPt = true;
					}
				}

				// Check shape
				if (selectPt)
				{
					if (shape == 0)
						selectPt = true;

					else if (shape == 1)
					{
						if (cShape == 1)
							selectPt = true;
						else
							selectPt = false;
					}

					else if (shape == 2)
					{
						if (cShape == 0)
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

					else if (type == 1)
					{
						if (cSig == 0)
							selectPt = true;
						else
							selectPt = false;
					}

					else if (type == 2)
					{
						if (cSig == 0)
							selectPt = false;
						else
							selectPt = true;
					}
				}

				//// Depending on the mode, designate point for selection or deselection

				// Mode "add to selection" - no matter the upper criteria, selected point stays selected
				if (mode == 4)
				{
					if (cSelected == 1)
						selectPt = true;
				}

				// Mode "deselect while obeying criteria" - deselected points stay that way...others get checked by criteria
				else if (mode == 5)
				{
					if (cSelected == 0)
						selectPt = false;
					else
					{
						if (selectPt)
							selectPt = false;
						else
							selectPt = true;
					}
				}

				// Mode "deselect every Nth selected marker while obeying criteria"
				else if (mode == 6)
				{
					if (selectPt)
					{
						if (cSelected == 1)
						{
							if (currentNth == Nth)
								selectPt = false;
							
							++currentNth;
							if(currentNth > Nth)
								currentNth = 1;
						}
						
						else
							selectPt = false;
					}

					else
					{
						if (cSelected == 1)
							selectPt = true;
						else
							selectPt = false;
					}
				}

				// Mode "invert while obeying criteria" - check if point passed criteria and then invert selection
				else if (mode == 7)
				{
					if (selectPt)
					{
						if (cSelected == 1)
							selectPt = false;
						else
							selectPt = true;
					}
					else
					{
						if (cSelected == 1)
							selectPt = true;
						else
							selectPt = false;
					}
				}

				/// Finally select or deselect the point
				if (selectPt)
					cSelected = 1;
				else
					cSelected = 0;				
			}

			// Update tempo point
			newState.AppendFormatted(256, "PT %.16f %.16f %d %d %d %d\n", cTime, cBpm, cShape, cSig, cSelected, cPartial);
		}
		
		else
		{
			newState.Append(token);
			newState.Append("\n");
		}			
		token = strtok(NULL, "\n");	
	}
	
	// Update tempo chunk
	GetSetObjectState(envelope, newState.Get());
	FreeHeapPtr(envState);
};

void AdjustTempo (int mode, double bpm, int shape)
{
	/*	
	mode ---> 0 to add, 1 to calculate percentage
	shape --> 0 to ignore, 1 to invert, 2 for linear, 3 for square
	*/

	// Get tempo chunk
	TrackEnvelope* envelope = SWS_GetTrackEnvelopeByName(CSurf_TrackFromID(0, false), "Tempo map" );
	char* envState = GetSetObjectState(envelope, "");
	char* token = strtok(envState, "\n");

	// Get TEMPO MARKERS timebase (not project timebase)
	int timeBase = *(int*)GetConfigVar("tempoenvtimelock");
	
	// Temporary change of preferences - prevent reselection of points in time selection
	int envClickSegMode = *(int*)GetConfigVar("envclicksegmode");
	*(int*)GetConfigVar("envclicksegmode") = 1;

	// Prepare variables
	double pTime; GetTempoTimeSigMarker(NULL, 0, &pTime, NULL, NULL, NULL, NULL, NULL, NULL);
	double pBpm = 1;
	int pShape = 1;
	double pOldTime = pTime;
	double pOldBpm = 1;
	int pOldShape = 1;
	
	// Loop through tempo chunk and perform BPM calculations
	WDL_FastString newState;
	LineParser lp(false);
	while(token != NULL)
	{
		lp.parse(token);
		if (strcmp(lp.gettoken_str(0),  "PT") == 0)
		{	
			double cTime = lp.gettoken_float(1);
			double cBpm = lp.gettoken_float(2);
			int cShape = lp.gettoken_int(3);
			int cSig = lp.gettoken_int(4);
			int cSelected = lp.gettoken_int(5);
			int cPartial = lp.gettoken_int(6);

			// Save "soon to be old" values
			double oldTime = cTime;
			double oldBpm = cBpm;
			int oldShape = cShape;
									
			// If point is selected calculate it's new BPM and shape.
			if (cSelected == 1)
			{
				// Calculate BPM
				if (mode == 0)
					cBpm += bpm;
				else
					cBpm *= 1 + bpm/100;
				
				// Check if BPM is legal
				if (cBpm < 0.001)
					cBpm = 0.001;
				else if (cBpm > 960)
					cBpm = 960;		

				// Set shape					
				if (shape == 3)
					cShape = 1;
				else if (shape == 2)
					cShape = 0;
				else if (shape == 1)
				{
					if (cShape == 1)
						cShape = 0;
					else
						cShape = 1;	
				}
			}
			
			// Get new position but only if timebase is beats
			if (timeBase == 1)
			{
				double measure;
				if (pOldShape == 1)
					measure = (oldTime-pOldTime) * pOldBpm / 240;
				else
					measure = (oldTime-pOldTime) * (oldBpm+pOldBpm) / 480;
				
				if (pShape == 1)
					cTime = pTime + (240*measure) / pBpm;
				else
					cTime = pTime + (480*measure) / (pBpm + cBpm);
			}

			// Update tempo point
			newState.AppendFormatted(256, "PT %.16f %.16f %d %d %d %d\n", cTime, cBpm, cShape, cSig, cSelected, cPartial);

			// Update data on previous point
			pTime = cTime;
			pBpm = cBpm;
			pShape = cShape;
			pOldTime = oldTime;
			pOldBpm = oldBpm;
			pOldShape = oldShape;
		}
		
		else
		{
			newState.Append(token);
			newState.Append("\n");
		}
			
		token = strtok(NULL, "\n");	
	}
	
	// Update tempo chunk
	GetSetObjectState(envelope, newState.Get());
	FreeHeapPtr(envState);

	// Refresh tempo map
	double t, b; int n, d; bool s;
	GetTempoTimeSigMarker(NULL, 0, &t, NULL, NULL, &b, &n, &d, &s);
	SetTempoTimeSigMarker(NULL, 0, t, -1, -1, b, n, d, s);
	UpdateTimeline();

	// Restore preferences back to the previous state
	*(int*)GetConfigVar("envclicksegmode") = envClickSegMode;
};

void UpdateTargetBpm (HWND hwnd, int doFirst, int doCursor, int doLast)
{
	char eBpmAdj[128], eBpmFirstCur[128], eBpmCursorCur[128], eBpmLastCur[128];
	double bpmFirstTar, bpmCursorTar, bpmLastTar;														
	GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_1, eBpmFirstCur, 128);
	GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_2, eBpmCursorCur, 128);
	GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_3, eBpmLastCur, 128);

	if (atof(eBpmFirstCur) == 0)
	{
		sprintf(eBpmFirstCur, "%d", 0);
		sprintf(eBpmCursorCur, "%d", 0);
		sprintf(eBpmLastCur,"%d", 0);
	}
	else
	{
		// Calculate target
		if (IsDlgButtonChecked(hwnd, IDC_BR_ADJ_BPM_VAL_ENB) == 1)
		{
			GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_VAL, eBpmAdj, 128);
			bpmFirstTar = AltAtof(eBpmAdj) + AltAtof(eBpmFirstCur);
			bpmCursorTar = AltAtof(eBpmAdj) + AltAtof(eBpmCursorCur);
			bpmLastTar = AltAtof(eBpmAdj) + AltAtof(eBpmLastCur);
		}
		else
		{						
			GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_PERC, eBpmAdj, 128);
			bpmFirstTar = (1 + AltAtof(eBpmAdj)/100) * AltAtof(eBpmFirstCur);
			bpmCursorTar = (1 + AltAtof(eBpmAdj)/100) * AltAtof(eBpmCursorCur);
			bpmLastTar = (1 + AltAtof(eBpmAdj)/100) * AltAtof(eBpmLastCur);
		}

		// Check values
		if (bpmFirstTar < 0.001)
			bpmFirstTar = 0.001;
		else if (bpmFirstTar > 960)
			bpmFirstTar = 960;
		if (bpmCursorTar < 0.001)
			bpmCursorTar = 0.001;
		else if (bpmCursorTar > 960)
			bpmCursorTar = 960;
		if (bpmLastTar < 0.001)
			bpmLastTar = 0.001;
		else if (bpmLastTar > 960)
			bpmLastTar = 960;
		        
		sprintf(eBpmFirstCur, "%.6g", bpmFirstTar);
		sprintf(eBpmCursorCur, "%.6g", bpmCursorTar);
		sprintf(eBpmLastCur, "%.6g", bpmLastTar);
	}
	
	// Update target edit boxes
	if (doFirst == 1)
		SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_TAR_1, eBpmFirstCur);
	if (doCursor == 1)
		SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_TAR_2, eBpmCursorCur);
	if (doLast == 1)
		SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_TAR_3, eBpmLastCur);
};

void UpdateCurrentBpm (HWND hwnd, const vector<int>& selectedPoints)
{
	double bpmFirst, bpmCursor, bpmLast;
	char eBpmFirst[128], eBpmCursor[128], eBpmLast[128], eBpmFirstChk[128], eBpmCursorChk[128], eBpmLastChk[128];
	
	// Get BPM info on selected points
	if (selectedPoints.size() != 0)
	{	
		GetTempoTimeSigMarker(NULL, selectedPoints[0], NULL, NULL, NULL, &bpmFirst, NULL, NULL, NULL);
		GetTempoTimeSigMarker(NULL, selectedPoints.back(), NULL, NULL, NULL, &bpmLast, NULL, NULL, NULL);
		sprintf(eBpmFirst, "%.6g", bpmFirst);
		sprintf(eBpmLast, "%.6g", bpmLast);
	}
	
	else
	{
		bpmFirst = bpmLast = 0;
		sprintf(eBpmFirst, "%d", bpmFirst);
		sprintf(eBpmLast, "%d", bpmLast);
	}

	TimeMap_GetTimeSigAtTime(NULL, GetCursorPositionEx(NULL), NULL, NULL, &bpmCursor);
	sprintf(eBpmCursor, "%.6g", bpmCursor);
			
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
		UpdateTargetBpm(hwnd, 1, 1, 1);
	}
};

void UpdateSelectionFields (HWND hwnd)
{
	char eBpmStart[128], eBpmEnd[128], eNum[128], eDen[128];	
	GetDlgItemText(hwnd, IDC_BR_SEL_BPM_START, eBpmStart, 128);
	GetDlgItemText(hwnd, IDC_BR_SEL_BPM_END, eBpmEnd, 128);
	GetDlgItemText(hwnd, IDC_BR_SEL_SIG_NUM, eNum, 128);
	GetDlgItemText(hwnd, IDC_BR_SEL_SIG_DEN, eDen, 128);
		
	double bpmStart = AltAtof(eBpmStart);
	double bpmEnd = AltAtof(eBpmEnd);
	int num = atoi(eNum); if (num <= 0){num = 4;} else if (num > 255){num = 255;}
	int den = atoi(eDen); if (den <= 0){den = 4;} else if (den > 255){den = 255;}

	// Update edit boxes with "atoied/atofed" values
	sprintf(eBpmStart, "%.19g", bpmStart);
	sprintf(eBpmEnd, "%.19g", bpmEnd);
	sprintf(eNum, "%d", num);
	sprintf(eDen, "%d", den);
	SetDlgItemText(hwnd, IDC_BR_SEL_BPM_START, eBpmStart);
	SetDlgItemText(hwnd, IDC_BR_SEL_BPM_END, eBpmEnd);
	SetDlgItemText(hwnd, IDC_BR_SEL_SIG_NUM, eNum);
	SetDlgItemText(hwnd, IDC_BR_SEL_SIG_DEN, eDen);
};

void SelectTempoCase (HWND hwnd, int operationType, int deselectNth)
{
	/*
	operation type --> 0 to select
				   --> 1 to deselect, if deselectNth > 1, deselect every Nth selected marker
				   --> 2 to invert
	*/

	// Read values from the dialog
	int mode, bpm, sig;

	if (IsDlgButtonChecked(hwnd, IDC_BR_SEL_ADD) == 1)
		mode = 4; // Add to existing selection
	else
		mode = 3; // Create new selection
	
	if (IsDlgButtonChecked(hwnd, IDC_BR_SEL_BPM) == 1)
		bpm = 1;
	else
		bpm = 0;
	
	if (IsDlgButtonChecked(hwnd, IDC_BR_SEL_SIG) == 1)
		sig = 1;
	else
		sig = 0;

	int timeSel = (int)SendDlgItemMessage(hwnd,IDC_BR_SEL_TIME_RANGE,CB_GETCURSEL,0,0);
	int shape = (int)SendDlgItemMessage(hwnd,IDC_BR_SEL_SHAPE,CB_GETCURSEL,0,0);
	int type = (int)SendDlgItemMessage(hwnd,IDC_BR_SEL_TYPE_DEF,CB_GETCURSEL,0,0);

	char eBpmStart[128], eBpmEnd[128], eNum[128], eDen[128];	
	GetDlgItemText(hwnd, IDC_BR_SEL_BPM_START, eBpmStart, 128);
	GetDlgItemText(hwnd, IDC_BR_SEL_BPM_END, eBpmEnd, 128);
	GetDlgItemText(hwnd, IDC_BR_SEL_SIG_NUM, eNum, 128);
	GetDlgItemText(hwnd, IDC_BR_SEL_SIG_DEN, eDen, 128);
	int num = atoi(eNum); if (num <= 0){num = 4;} else if (num > 255){num = 255;}
	int den = atoi(eDen); if (den <= 0){den = 4;} else if (den > 255){den = 255;}
	double bpmStart = AltAtof(eBpmStart);
	double bpmEnd = AltAtof(eBpmEnd);

	// Invert BPM values if needed
	if (bpmStart > bpmEnd)
	{
		double temp = bpmStart;
		bpmStart = bpmEnd;
		bpmEnd = temp;
	}

	// Select
	if (operationType == 0)
		SelectTempo (mode, 0, timeSel, bpm, bpmStart, bpmEnd, shape, sig, num, den, type);

	// Deselect
	if (operationType == 1)	
	{
		if (deselectNth == 0)
			SelectTempo (5, 0, timeSel, bpm, bpmStart, bpmEnd, shape, sig, num, den, type);
		else
			SelectTempo (6, deselectNth, timeSel, bpm, bpmStart, bpmEnd, shape, sig, num, den, type);
	}
	
	//Invert
	if (operationType == 2)
		SelectTempo (7, 0, timeSel, bpm, bpmStart, bpmEnd, shape, sig, num, den, type);
};

void AdjustTempoCase (HWND hwnd)
{
	char eBpmVal[128], eBpmPerc[128];
	GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_VAL, eBpmVal, 128);
	GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_PERC, eBpmPerc, 128);
	double bpmVal = AltAtof(eBpmVal);
	double bpmPerc = AltAtof(eBpmPerc);

	// Update edit boxes
	UpdateTargetBpm(hwnd, 1, 1, 1);
	sprintf(eBpmVal, "%.19g", bpmVal);
	sprintf(eBpmPerc, "%.19g", bpmPerc);
	SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_VAL, eBpmVal);
	SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_PERC, eBpmPerc);
	
	int mode; double bpm;
	if (IsDlgButtonChecked(hwnd, IDC_BR_ADJ_BPM_VAL_ENB) == 1)
	{
		mode = 0; // Adjust by value
		bpm = bpmVal;
	}
	else
	{
		mode = 1; // Adjust by percentage
		bpm = bpmPerc;
	}
	int shape = (int)SendDlgItemMessage(hwnd,IDC_BR_ADJ_SHAPE,CB_GETCURSEL,0,0);

	// Adjust markers
	if (bpm != 0 || shape != 0)
	{
		Undo_BeginBlock2(NULL);
		AdjustTempo (mode, bpm, shape);
		Undo_EndBlock2 (NULL, __LOCALIZE("Adjust selected tempo markers","sws_undo"), UNDO_STATE_ALL);

	}
};

void CallDeselTempoDialog(bool show, HWND parentHandle)
{
	static HWND hwnd = CreateDialog (g_hInst, MAKEINTRESOURCE(IDD_BR_DESELECT_TEMPO), parentHandle, DeselectNthProc);
	
	if (show)
	{
		if (!g_deselectTempoDialog)
		{
			ShowWindow(hwnd, SW_SHOW);
			SetFocus(hwnd);
			g_deselectTempoDialog = true;
		}

		else
		{
			ShowWindow(hwnd, SW_HIDE);
			g_deselectTempoDialog = false;
		}
	}
	else
	{
		ShowWindow(hwnd, SW_HIDE);
		g_deselectTempoDialog = false;
	}
};

void LoadOptionsSelAdj (double &bpmStart, double &bpmEnd, int &num, int &den, int &bpmEnb, int &sigEnb, int &timeSel, int &shape, int &type, int &selPref, int &invertPref, int &adjustType, int &adjustShape)
{
	char tmp[256];
	GetPrivateProfileString("SWS", SEL_ADJ_KEY, "120 150 4 4 1 0 0 0 0 0 0 0 0", tmp, 256, get_ini_file());
	sscanf(tmp, "%lf %lf %d %d %d %d %d %d %d %d %d %d %d", &bpmStart, &bpmEnd, &num, &den, &bpmEnb, &sigEnb, &timeSel, &shape, &type, &selPref, &invertPref, &adjustType, &adjustShape);

	// Restore defaults if .ini has been tampered with
	if (num <= 0 || num > 255)
		num = 4;
	if (den <= 0 || den > 255)
		den = 4;
	if (bpmEnb != 0 && bpmEnb != 1)
		bpmEnb = 1;
	if (sigEnb != 0 && sigEnb != 1)
		sigEnb = 0;
	if (timeSel < 0 || timeSel > 2)
		timeSel = 0;
	if (shape < 0 || shape > 2)
		shape = 0;
	if (type < 0 || type > 2)
		type = 0;
	if (selPref != 0 && selPref != 1)
		selPref = 1;
	if (invertPref != 0 && invertPref != 1)
		invertPref = 0;
	if (adjustType != 0 && adjustType != 1)
		adjustType = 1;
	if (adjustShape < 0 || adjustShape > 3)
		adjustShape = 0;
};

void SaveOptionsSelAdj (HWND hwnd)
{
	char eBpmStart[128], eBpmEnd[128], eNum[128], eDen[128];	
	GetDlgItemText(hwnd, IDC_BR_SEL_BPM_START, eBpmStart, 128);
	GetDlgItemText(hwnd, IDC_BR_SEL_BPM_END, eBpmEnd, 128);
	GetDlgItemText(hwnd, IDC_BR_SEL_SIG_NUM, eNum, 128);
	GetDlgItemText(hwnd, IDC_BR_SEL_SIG_DEN, eDen, 128);	
	double bpmStart = AltAtof(eBpmStart);
	double bpmEnd = AltAtof(eBpmEnd);
	int num = atoi(eNum);
	int den = atoi(eDen);
	int bpmEnb = IsDlgButtonChecked(hwnd, IDC_BR_SEL_BPM);
	int sigEnb = IsDlgButtonChecked(hwnd, IDC_BR_SEL_SIG);
	int timeSel = (int)SendDlgItemMessage(hwnd,IDC_BR_SEL_TIME_RANGE,CB_GETCURSEL,0,0);
	int shape = (int)SendDlgItemMessage(hwnd,IDC_BR_SEL_SHAPE,CB_GETCURSEL,0,0);
	int type = (int)SendDlgItemMessage(hwnd,IDC_BR_SEL_TYPE_DEF,CB_GETCURSEL,0,0);
	int selPref = IsDlgButtonChecked(hwnd, IDC_BR_SEL_ADD);
	int invertPref = IsDlgButtonChecked(hwnd, IDC_BR_SEL_INVERT_PREF);
	int adjustType = IsDlgButtonChecked(hwnd, IDC_BR_ADJ_BPM_VAL_ENB);
	int adjustShape = (int)SendDlgItemMessage(hwnd,IDC_BR_ADJ_SHAPE,CB_GETCURSEL,0,0);

	char tmp[256];
	_snprintf(tmp, sizeof(tmp), "%lf %lf %d %d %d %d %d %d %d %d %d %d %d", bpmStart, bpmEnd, num, den, bpmEnb, sigEnb, timeSel, shape, type, selPref, invertPref, adjustType, adjustShape);
	WritePrivateProfileString("SWS", SEL_ADJ_KEY, tmp, get_ini_file());
};

int IsSelectAdjustTempoVisible (COMMAND_T* = NULL)
{
	return g_selectAdjustTempoDialog;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//							Deselect Nth Selected Tempo Marker
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
WDL_DLGRET DeselectNthProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, uMsg, wParam, lParam))
		return r;
	
	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			// Drop down menu
			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_BR_DESEL_NTH_TEMPO));
			
			int x = (int)SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("2nd","sws_DLG_168"));
			SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_SETITEMDATA, x, 0);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("3rd","sws_DLG_168"));
			SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_SETITEMDATA, x, 1);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("4th","sws_DLG_168"));
			SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_SETITEMDATA, x, 2);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("5th","sws_DLG_168"));
			SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_SETITEMDATA, x, 3);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("6th","sws_DLG_168"));
			SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_SETITEMDATA, x, 4);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("7th","sws_DLG_168"));
			SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_SETITEMDATA, x, 5);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("8th","sws_DLG_168"));
			SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_SETITEMDATA, x, 6);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("9th","sws_DLG_168"));
			SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_SETITEMDATA, x, 7);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("10th","sws_DLG_168"));
			SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_SETITEMDATA, x, 8);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("11th","sws_DLG_168"));
			SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_SETITEMDATA, x, 9);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("12th","sws_DLG_168"));
			SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_SETITEMDATA, x, 10);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("13th","sws_DLG_168"));
			SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_SETITEMDATA, x, 11);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("14th","sws_DLG_168"));
			SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_SETITEMDATA, x, 12);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("15th","sws_DLG_168"));
			SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_SETITEMDATA, x, 13);
			x = (int)SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("16th","sws_DLG_168"));
			SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_SETITEMDATA, x, 14);

			// Load options from .ini
			int Nth, criteria;
			LoadOptionsDeselectNth (Nth, criteria);			

			// Set controls
			SendDlgItemMessage(hwnd, IDC_BR_DESEL_NTH_TEMPO, CB_SETCURSEL, Nth, 0);
			CheckDlgButton(hwnd, IDC_BR_DESEL_CRITERIA, !!criteria);

			RestoreWindowPos(hwnd, DESEL_WND_KEY, false);
		}
		break;
        
		case WM_COMMAND:
		{
            switch(LOWORD(wParam))
            {
				case IDOK:
				{	
					int Nth = (int)SendDlgItemMessage(hwnd,IDC_BR_DESEL_NTH_TEMPO,CB_GETCURSEL,0,0) + 2;
					Undo_BeginBlock2(NULL);
					if (IsDlgButtonChecked(hwnd, IDC_BR_DESEL_CRITERIA) == 1)
						SelectTempoCase (GetParent(hwnd), 1, Nth);
					else
						SelectTempo (2, Nth, 0, 0, 0, 0, 0, 0, 0, 0, 0);
					Undo_EndBlock2 (NULL, __LOCALIZE("Deselect tempo markers","sws_undo"), UNDO_STATE_ALL);
				}
				break;
				
				case IDCANCEL:
				{
					ShowWindow(hwnd, SW_HIDE);
					g_deselectTempoDialog = false;
				}
				break;
			} 
		}
		break;

		case WM_DESTROY:
		{
			SaveWindowPos(hwnd, DESEL_WND_KEY);
			SaveOptionsDeselectNth (hwnd);
		}
		break; 
	}
	return 0;
};

void LoadOptionsDeselectNth (int &Nth, int &criteria)
{
	char tmp[256];
	GetPrivateProfileString("SWS", DESEL_KEY, "0 0", tmp, 256, get_ini_file());
	sscanf(tmp, "%d %d ", &Nth, &criteria);

	// Restore defaults if .ini has been tampered with
	if (Nth < 0 && Nth > 14)
		Nth = 0;
	if (criteria != 0 && criteria != 1)
		criteria = 1;	
};

void SaveOptionsDeselectNth (HWND hwnd)
{
	int Nth = (int)SendDlgItemMessage(hwnd,IDC_BR_DESEL_NTH_TEMPO,CB_GETCURSEL,0,0);
	int criteria = IsDlgButtonChecked(hwnd, IDC_BR_DESEL_CRITERIA);

	char tmp[256];
	_snprintf(tmp, sizeof(tmp), "%d %d", Nth, criteria);
	WritePrivateProfileString("SWS", DESEL_KEY, tmp, get_ini_file());
};