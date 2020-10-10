/******************************************************************************
/ BR_TempoDlg.cpp
/
/ Copyright (c) 2013-2015 Dominik Martin Drzic
/ http://forum.cockos.com/member.php?u=27094
/ http://github.com/reaper-oss/sws
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
#include "BR_EnvelopeUtil.h"
#include "BR_Misc.h"
#include "BR_Tempo.h"
#include "BR_Util.h"
#include "../SnM/SnM_Dlg.h"
#include "../SnM/SnM_Util.h"

#include <WDL/localize/localize.h>

/******************************************************************************
* Constants                                                                   *
******************************************************************************/
const char* const SHAPE_KEY   = "BR - ChangeTempoShape";
const char* const SHAPE_WND   = "BR - ChangeTempoShape WndPos";
const char* const RAND_KEY    = "BR - RandomizeTempo";
const char* const RAND_WND    = "BR - RandomizeTempo WndPos";
const char* const CONVERT_KEY = "BR - ConvertTempo";
const char* const CONVERT_WND = "BR - ConvertTempo WndPos";
const char* const SEL_ADJ_KEY = "BR - SelectAdjustTempo";
const char* const SEL_ADJ_WND = "BR - SelectAdjustTempo WndPos";
const char* const UNSEL_KEY   = "BR - DeselectNthTempo";
const char* const UNSEL_WND   = "BR - DeselectNthTempo WndPos";

/******************************************************************************
* Globals                                                                     *
******************************************************************************/
static double g_tempoShapeSplitRatio  = -1;
static int    g_tempoShapeSplitMiddle = -1;

/******************************************************************************
* Convert project markers to tempo markers                                    *
******************************************************************************/
static void ConvertMarkersToTempo (int markers, int num, int den, bool removeMarkers, bool timeSel, bool gradualTempo, bool split, double splitRatio)
{
	vector<double> markerPositions = GetProjectMarkers(timeSel, MIN_GRID_DIST); // delta: sometimes time selection won't catch start/end project marker due to small rounding differences

	if (markerPositions.size() <=1)
	{
		if (timeSel)
			MessageBox(g_hwndParent, __LOCALIZE("Not enough project markers in the time selection to perform conversion.", "sws_DLG_166"), __LOCALIZE("SWS/BR - Error", "sws_mbox"), MB_OK);
		else
			MessageBox(g_hwndParent, __LOCALIZE("Not enough project markers in the project to perform conversion.", "sws_DLG_166"), __LOCALIZE("SWS/BR - Error", "sws_mbox"), MB_OK);
		return;
	}

	if (CountTempoTimeSigMarkers(NULL) > 0)
	{
		bool check = false;
		int count = CountTempoTimeSigMarkers(NULL);
		for (int i = 0; i < count; ++i)
		{
			double position;
			GetTempoTimeSigMarker(NULL, i, &position, NULL, NULL, NULL, NULL, NULL, NULL);

			if (!check && position > markerPositions.front() && position <= markerPositions.back())
			{
				int answer = MessageBox(g_hwndParent, __LOCALIZE("Detected existing tempo markers between project markers.\nAre you sure you want to continue? Strange things may happen.","sws_DLG_166"), __LOCALIZE("SWS/BR - Warning","sws_mbox"), MB_YESNO);
				if (answer == IDNO)  return;
				if (answer == IDYES) check = true;
			}

			if (position > markerPositions.back())
			{
				// When tempo envelope timebase is time, these tempo markers can't be moved so there is no need to warn user
				const int timeBase = ConfigVar<int>("tempoenvtimelock").value_or(0);
				if (timeBase == 0)
					break;
				else
				{
					int answer = MessageBox(g_hwndParent, __LOCALIZE("Detected existing tempo markers after the last project marker.\nThey may get moved during the conversion. Are you sure you want to continue?","sws_DLG_166"), __LOCALIZE("SWS/BR - Warning","sws_mbox"), MB_YESNO);
					if (answer == IDNO)  return;
					if (answer == IDYES) break;
				}
			}
		}
	}

	Undo_BeginBlock2(NULL);
	double measure = num / (den * (double)markers);
	int exceed = 0;

	vector<double> stretchMarkers;
	if (!gradualTempo)
	{
		for (size_t i = 0; i < markerPositions.size() - 1 ; ++i)
		{
			double bpm = (240 * measure) / (markerPositions[i+1] - markerPositions[i]);
			if (bpm > MAX_BPM)
				++exceed;

			if (i == 0) // Set first tempo marker with time signature
				SetTempoTimeSigMarker(NULL, -1, markerPositions[i], -1, -1, bpm, num, den, false);
			else
				SetTempoTimeSigMarker(NULL, -1, markerPositions[i], -1, -1, bpm, 0, 0, false);

			if (IsSetAutoStretchMarkersOn(NULL))
			{
				double position;
				if (GetTempoTimeSigMarker(NULL, FindTempoMarker(markerPositions[i], MIN_TEMPO_DIST / 2), &position, NULL, NULL, NULL, NULL, NULL, NULL))
					stretchMarkers.push_back(position);
			}
		}
	}
	else
	{
		// Get linear points' BPM (these get put where project markers are)
		vector <double> linearPoints;
		double prevBpm;
		for (size_t i = 0; i < markerPositions.size()-1; ++i)
		{
			double bpm = (240 * measure) / (markerPositions[i+1] - markerPositions[i]);

			// First marker is the same as square point
			if (i == 0)
				linearPoints.push_back(bpm);

			// Markers in between are the average of the current and previous transition
			else if (i != markerPositions.size()-2)
				linearPoints.push_back((bpm + prevBpm) / 2);

			// Last two (or one - depending on time selection)
			else
			{
				// for time selection:    last-1 is same as square,  last is ignored
				// for the whole project: last-1 is averaged,        last is same as square
				if (!timeSel)
					linearPoints.push_back((bpm + prevBpm) / 2);
				linearPoints.push_back(bpm);
			}
			prevBpm = bpm;
		}

		double beatsInternval = (split) ? (measure*(1-splitRatio) / 2) : (measure/2);

		// Set points
		for (size_t i = 0; i < linearPoints.size(); ++i)
		{


			// First tempo marker has time signature, last will have square shape if converting within time selection
			if (i == 0)
				SetTempoTimeSigMarker(NULL, -1, markerPositions[i], -1, -1, linearPoints[i], num, den, true);
			else if (i != linearPoints.size()-1)
				SetTempoTimeSigMarker(NULL, -1, markerPositions[i], -1, -1, linearPoints[i], 0, 0, true);
			else
			{
				if (timeSel)
					SetTempoTimeSigMarker(NULL, -1, markerPositions[i], -1, -1, linearPoints[i], 0, 0, false);
				else
					SetTempoTimeSigMarker(NULL, -1, markerPositions[i], -1, -1, linearPoints[i], 0, 0, true);
			}

			if (IsSetAutoStretchMarkersOn(NULL))
			{
				double position;
				if (GetTempoTimeSigMarker(NULL, FindTempoMarker(markerPositions[i], MIN_TEMPO_DIST/2), &position, NULL, NULL, NULL, NULL, NULL, NULL))
					stretchMarkers.push_back(position);
			}

			if (linearPoints[i] > MAX_BPM)
				++exceed;

			// Create middle point(s)
			if (i != linearPoints.size()-1)
			{
				double pos, bpm;
				CalculateMiddlePoint(&pos, &bpm, measure, markerPositions[i], markerPositions[i+1], linearPoints[i], linearPoints[i+1]);

				if (!split)
				{
					SetTempoTimeSigMarker(NULL, -1, pos, -1, -1, bpm, 0, 0, true);
					if (IsSetAutoStretchMarkersOn(NULL))
					{
						double position;
						if (GetTempoTimeSigMarker(NULL, FindTempoMarker(pos, MIN_TEMPO_DIST / 2), &position, NULL, NULL, NULL, NULL, NULL, NULL))
							stretchMarkers.push_back(position);
					}
					if (bpm > MAX_BPM)
						++exceed;
				}
				else
				{
					double pos1, pos2, bpm1, bpm2;
					CalculateSplitMiddlePoints(&pos1, &pos2, &bpm1, &bpm2, splitRatio, measure, markerPositions[i], pos, markerPositions[i+1], linearPoints[i], bpm, linearPoints[i+1]);

					SetTempoTimeSigMarker(NULL, -1, pos1, -1, -1, bpm1, 0, 0, true);
					SetTempoTimeSigMarker(NULL, -1, pos2, -1, -1, bpm2, 0, 0, true);
					if (IsSetAutoStretchMarkersOn(NULL))
					{
						double position;
						if (GetTempoTimeSigMarker(NULL, FindTempoMarker(pos1, MIN_TEMPO_DIST / 2), &position, NULL, NULL, NULL, NULL, NULL, NULL))
							stretchMarkers.push_back(position);
						if (GetTempoTimeSigMarker(NULL, FindTempoMarker(pos2, MIN_TEMPO_DIST / 2), &position, NULL, NULL, NULL, NULL, NULL, NULL))
							stretchMarkers.push_back(position);
					}

					if (bpm1 > MAX_BPM || bpm2 > MAX_BPM)
						++exceed;

					pos = pos2; // used for checking at the end
					bpm = bpm2;
				}

				// Middle point's BPM  is always calculated in a relation to the point behind so it will land on the correct musical position.
				// But it can also make the end point move from it's designated musical position due to rounding errors (even if small at the
				// beginning, they can accumulate). That's why we recalculate next point's BPM (in a relation to last middle point) so it always lands
				// on the correct musical position
				linearPoints[i+1] = (480*beatsInternval) / (markerPositions[i+1]-pos) - bpm;
			}
		}
	}

	if (removeMarkers)
	{
		if (timeSel)
			Main_OnCommand(40420, 0);
		else
		{
			bool region;
			int id, x = 0;
			while (EnumProjectMarkers(x, &region, NULL, NULL, NULL, &id))
			{
				if (!region)
					DeleteProjectMarker(NULL, id, false);
				else
					++x;
			}
		}
	}

	InsertStretchMarkersInAllItems(stretchMarkers);
	UpdateTimeline();
	Undo_EndBlock2(NULL, __LOCALIZE("Convert project markers to tempo markers","sws_undo"), UNDO_STATE_TRACKCFG | UNDO_STATE_MISCCFG);

	if (exceed != 0)
		MessageBox(g_hwndParent, __LOCALIZE("Some of the created tempo markers have a BPM over 960. If you try to edit them, they will revert back to 960 or lower.\n\nIt is recommended that you undo, edit project markers and try again.", "sws_DLG_166"),__LOCALIZE("SWS/BR - Warning", "sws_mbox"), MB_OK);
}

static void ShowGradualOptions (bool show, HWND hwnd)
{
	int c;
	if (show)
	{
		ShowWindow(GetDlgItem(hwnd, IDC_BR_CON_SPLIT), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_CON_SPLIT_RATIO), SW_SHOW);
		c = 29;
	}

	else
	{
		ShowWindow(GetDlgItem(hwnd, IDC_BR_CON_SPLIT), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_CON_SPLIT_RATIO), SW_HIDE);
		c = -29;
	}

	RECT r;

	GetWindowRect(GetDlgItem(hwnd, IDC_BR_CON_GROUP2), &r);
	SetWindowPos(GetDlgItem(hwnd, IDC_BR_CON_GROUP2), HWND_BOTTOM, 0, 0, r.right-r.left, r.bottom-r.top+c, SWP_NOMOVE);

	GetWindowRect(GetDlgItem(hwnd, IDOK), &r); ScreenToClient(hwnd, (LPPOINT)&r);
	SetWindowPos(GetDlgItem(hwnd, IDOK), NULL, r.left, r.top+c, 0, 0, SWP_NOSIZE);
	GetWindowRect(GetDlgItem(hwnd, IDCANCEL), &r); ScreenToClient(hwnd, (LPPOINT)&r);
	SetWindowPos(GetDlgItem(hwnd, IDCANCEL), NULL, r.left, r.top+c, 0, 0, SWP_NOSIZE);

	GetWindowRect(hwnd, &r);
	#ifdef _WIN32
		SetWindowPos(hwnd, NULL, 0, 0, r.right-r.left, r.bottom-r.top+c, SWP_NOMOVE);
	#else
		SetWindowPos(hwnd, NULL, r.left, r.top-c,  r.right-r.left, r.bottom-r.top+c, 0);
	#endif
}

static void SaveOptionsConversion (HWND hwnd)
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

	char tmp[512];
	snprintf(tmp, sizeof(tmp), "%d %d %d %d %d %d %d %s", markers, num, den, removeMarkers, timeSel, gradual, split, splitRatio);
	WritePrivateProfileString("SWS", CONVERT_KEY, tmp, get_ini_file());
}

static void LoadOptionsConversion (int& markers, int& num, int& den, int& removeMarkers, int& timeSel, int& gradual, int& split, char* splitRatio, int splitRatioSz)
{
	char tmp[512];
	GetPrivateProfileString("SWS", CONVERT_KEY, "", tmp, sizeof(tmp), get_ini_file());

	LineParser lp(false);
	lp.parse(tmp);
	markers         = (lp.getnumtokens() > 0) ? lp.gettoken_int(0) : 4;
	num             = (lp.getnumtokens() > 1) ? lp.gettoken_int(1) : 4;
	den             = (lp.getnumtokens() > 2) ? lp.gettoken_int(2) : 4;
	removeMarkers   = (lp.getnumtokens() > 3) ? lp.gettoken_int(3) : 1;
	timeSel         = (lp.getnumtokens() > 4) ? lp.gettoken_int(4) : 0;
	gradual         = (lp.getnumtokens() > 5) ? lp.gettoken_int(5) : 0;
	split           = (lp.getnumtokens() > 6) ? lp.gettoken_int(6) : 0;
	strncpy(splitRatio, (lp.getnumtokens() > 7) ? lp.gettoken_str(7) : "1/2", splitRatioSz);

	double convertedRatio;
	IsFraction(splitRatio, convertedRatio);
	if (convertedRatio <= 0 || convertedRatio >= 1)
		strcpy(splitRatio, "0");
	if (markers <= 0)
		markers = 4;
	if (num < MIN_SIG || num > MAX_SIG)
		num = 4;
	if (den < MIN_SIG || den > MAX_SIG)
		den = 4;
	if (removeMarkers != 0 && removeMarkers != 1)
		removeMarkers = 1;
	if (timeSel != 0 && timeSel != 1)
		timeSel = 0;
	if (gradual != 0 && gradual != 1)
		gradual = 0;
	if (split != 0 && split != 1)
		split = 0;
}

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
			SendDlgItemMessage(hwnd, IDC_BR_CON_SPLIT_RATIO, CB_ADDSTRING, 0, (LPARAM)"1/2");
			SendDlgItemMessage(hwnd, IDC_BR_CON_SPLIT_RATIO, CB_ADDSTRING, 0, (LPARAM)"1/3");
			SendDlgItemMessage(hwnd, IDC_BR_CON_SPLIT_RATIO, CB_ADDSTRING, 0, (LPARAM)"1/4");

			// Load options from .ini
			int markers, num, den, removeMarkers, timeSel, gradual, split;
			char eNum[128], eDen[128] , eMarkers[128], splitRatio[128];
			LoadOptionsConversion(markers, num, den, removeMarkers, timeSel, gradual, split, splitRatio, sizeof(splitRatio));
			snprintf(eNum,     sizeof(eNum),     "%d", num);
			snprintf(eDen,     sizeof(eDen),     "%d", den);
			snprintf(eMarkers, sizeof(eMarkers), "%d", markers);

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

			if (!gradual)
				ShowGradualOptions(false, hwnd);

			SetTimer(hwnd, 1, 100, NULL);
			RestoreWindowPos(hwnd, CONVERT_WND, false);
			ShowWindow(hwnd, SW_SHOW);
			SetFocus(hwnd);
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

					int num = atoi(eNum); if (num > MAX_SIG){num = MAX_SIG;}
					int den = atoi(eDen); if (den > MAX_SIG){den = MAX_SIG;}

					int markers = atoi(eMarkers);
					IsFraction(splitRatio, convertedSplitRatio);
					if (convertedSplitRatio <= 0 || convertedSplitRatio >= 1)
					{
						convertedSplitRatio = 0;
						strcpy(splitRatio, "0");
					}

					// Check boxes
					bool removeMarkers = !!IsDlgButtonChecked(hwnd, IDC_BR_CON_REMOVE);
					bool timeSel = !!IsDlgButtonChecked(hwnd, IDC_BR_CON_TIMESEL);
					bool gradual = !!IsDlgButtonChecked(hwnd, IDC_BR_CON_GRADUAL);
					bool split = !!IsDlgButtonChecked(hwnd, IDC_BR_CON_SPLIT);

					// Update edit boxes and dropdown to show "atoied" value
					snprintf(eNum,     sizeof(eNum),     "%d", num);
					snprintf(eDen,     sizeof(eDen),     "%d", den);
					snprintf(eMarkers, sizeof(eMarkers), "%d", markers);
					SetDlgItemText(hwnd, IDC_BR_CON_NUM, eNum);
					SetDlgItemText(hwnd, IDC_BR_CON_DEN, eDen);
					SetDlgItemText(hwnd, IDC_BR_CON_MARKERS, eMarkers);
					SetDlgItemText(hwnd, IDC_BR_CON_SPLIT_RATIO, splitRatio);

					if (markers <= 0 || num <= 0 || den <= 0)
					{
						MessageBox(g_hwndParent, __LOCALIZE("All values have to be positive integers.","sws_DLG_166"), __LOCALIZE("SWS/BR - Error","sws_mbox"), MB_OK);

						if (markers <= 0)
						{
							SetFocus(GetDlgItem(hwnd, IDC_BR_CON_MARKERS));
							SendMessage(GetDlgItem(hwnd, IDC_BR_CON_MARKERS), EM_SETSEL, 0, -1);
						}
						else if (num <= 0)
						{
							SetFocus(GetDlgItem(hwnd, IDC_BR_CON_NUM));
							SendMessage(GetDlgItem(hwnd, IDC_BR_CON_NUM), EM_SETSEL, 0, -1);
						}
						else if (den <= 0)
						{
							SetFocus(GetDlgItem(hwnd, IDC_BR_CON_DEN));
							SendMessage(GetDlgItem(hwnd, IDC_BR_CON_DEN), EM_SETSEL, 0, -1);
						}
					}
					else
					{
						if (convertedSplitRatio == 0)
							split = false;
						if (!IsWindowEnabled(GetDlgItem(hwnd, IDC_BR_CON_TIMESEL)))
							timeSel = false;
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
					ConvertMarkersToTempoDialog(NULL);
				}
				break;
			}
		}
		break;

		case WM_TIMER:
		{
			double tStart, tEnd;
			GetSet_LoopTimeRange2(NULL, false, false, &tStart, &tEnd, false);
			if (tStart == tEnd)
				EnableWindow(GetDlgItem(hwnd, IDC_BR_CON_TIMESEL), false);
			else
				EnableWindow(GetDlgItem(hwnd, IDC_BR_CON_TIMESEL), true);
		}
		break;

		case WM_DESTROY:
		{
			KillTimer(hwnd, 1);
			SaveWindowPos(hwnd, CONVERT_WND);
			SaveOptionsConversion(hwnd);
		}
		break;
	}
	return 0;
}

/******************************************************************************
* Select and adjust tempo markers                                             *
******************************************************************************/
static void SelectTempo (int mode, int Nth, int timeSel, int bpm, double bpmStart, double bpmEnd, int shape, int sig, int num, int den, int type)
{
	/*
	mode     ---> Ignore criteria
	               --> 0 to clear
	               --> 1 to invert selection
	               --> 2 to unselect every nth selected point

	         ---> Check by criteria before selecting
	               --> 3 to set new
	               --> 4 to add to existing selection
	               --> 5 to unselect
	               --> 6 to unselect every Nth selected marker
	               --> 7 to invert

	criteria ---> Nth     ---> ordinal number of the selected point
	         ---> timeSel ---> 0 for all, 1 for time selection, 2 to exclude time selection
	         ---> bpm     ---> 0 to ignore BPM, 1 to compare with bpmStart and bpmEnd
	         ---> shape   ---> 0 for all, 1 for square, 2 for linear
	         ---> sig     ---> 0 to ignore sig, if 1 num and den need to be specified
	         ---> type    ---> 0 for all, 1 for tempo markers only, 2 for time signature only, 3 for partial time signature only
	*/

	int currentNth = 1;
	double tStart, tEnd;
	GetSet_LoopTimeRange2 (NULL, false, false, &tStart, &tEnd, false);

	BR_Envelope tempoMap (GetTempoEnv());
	for (int i = 0; i < tempoMap.CountPoints(); ++i)
	{
		// Clear selected points
		if (mode == 0)
			tempoMap.SetSelection(i, false);

		// Invert selected points
		else if (mode == 1)
			tempoMap.SetSelection(i, !tempoMap.GetSelection(i));

		// Unselect every Nth selected point
		else if (mode == 2)
		{
			if (tempoMap.GetSelection(i))
			{
				if (currentNth == Nth)
					tempoMap.SetSelection(i, false);
				++currentNth;
				if (currentNth > Nth)
					currentNth = 1;
			}
		}

		// Select/Unselect points by criteria
		else
		{
			/* Check point by criteria */
			bool selectPt = true;

			double value, position;
			int pointShape;
			bool hasTimeSig, isPartialSigEnabled;
			tempoMap.GetPoint(i, &position, &value, &pointShape, NULL);
			tempoMap.GetTimeSig(i, &hasTimeSig, &isPartialSigEnabled, NULL, NULL);

			if (selectPt && bpm)
			{
				selectPt = (value >= bpmStart && value <= bpmEnd);
			}
			if (selectPt && sig)
			{
				int effNum, effDen;
				TimeMap_GetTimeSigAtTime(NULL, position, &effNum, &effDen, NULL);
				selectPt = (num == effNum && den == effDen);
			}
			if (selectPt && timeSel)
			{
				selectPt = (position >= tStart && position <= tEnd);
				if (timeSel == 2)
					selectPt = !selectPt;
			}
			if (selectPt && shape)
			{
				if (shape == 1)
					selectPt = (pointShape == SQUARE);
				else if (shape == 2)
					selectPt = (pointShape == LINEAR);
			}
			if (selectPt && type)
			{
				if (type == 1)
				{
					selectPt = !hasTimeSig;
				}
				else if (type == 2)
				{
					selectPt = hasTimeSig;
				}
				else if (type == 3)
				{
					selectPt = false;
					if (hasTimeSig && isPartialSigEnabled)
					{
						double prevPosition;
						if (tempoMap.GetPoint(i - 1, &prevPosition, NULL , NULL, NULL))
						{
							double absQN = TimeMap_timeToQN_abs(NULL, position) - TimeMap_timeToQN_abs(NULL, prevPosition);
							double QN    = TimeMap_timeToQN(position)           - TimeMap_timeToQN(prevPosition);
							selectPt = abs(absQN - QN) > MIN_TIME_SIG_PARTIAL_DIFF;
						}
					}
				}
			}

			/* Depending on the mode, designate point for selection/unselection */
			bool selected = tempoMap.GetSelection(i);

			if (mode == 4)      // add to selection
			{
				selectPt = (selected) ? (true) : (selectPt);
			}
			else if (mode == 5) // unselect while obeying criteria
			{
				selectPt = (selected) ? (!selectPt) : (false);
			}
			else if (mode == 6) // unselect every Nth selected marker while obeying criteria
			{
				if (selectPt)
				{
					if (selected)
					{
						if (currentNth == Nth)
							selectPt = false;

						// Update current Nth
						++currentNth;
						if (currentNth > Nth)
							currentNth = 1;
					}
					else
						selectPt = false;
				}
				else
					selectPt = !!selected;
			}
			else if (mode == 7) // invert while obeying criteria
			{
				selectPt = (selectPt) ? (!selected) : (selected);
			}
			tempoMap.SetSelection(i, selectPt);
		}
	}
	tempoMap.Commit();
}

static void AdjustTempo (int mode, double bpm, int shape)
{
	/*
	mode ---> 0 for value, 1 for percentage
	shape --> 0 to ignore, 1 to invert, 2 for linear, 3 for square
	*/

	BR_Envelope tempoMap(GetTempoEnv());
	const int timeBase = ConfigVar<int>("tempoenvtimelock").value_or(0);

	double prevOldTime  = 0;
	double prevOldBpm   = 0;
	int    prevOldShape = 0;

	// Loop through tempo chunk and perform BPM calculations
	for (int i = 0; i < tempoMap.CountPoints(); ++i)
	{
		double oldTime;
		double oldBpm;
		int    oldShape;
		tempoMap.GetPoint(i, &oldTime, &oldBpm, &oldShape, NULL);

		double newTime  = oldTime;
		double newBpm   = oldBpm;
		int    newShape = oldShape;

		if (tempoMap.GetSelection(i))
		{
			if (mode == 0) newBpm += bpm;
			else           newBpm *= 1 + bpm/100;
			newBpm = SetToBounds(newBpm, MIN_BPM, MAX_BPM);

			if      (shape == 3) newShape = SQUARE;
			else if (shape == 2) newShape = LINEAR;
			else if (shape == 1) newShape = (oldShape == LINEAR) ? SQUARE: LINEAR;
		}

		if (timeBase == 1 && i != 0)
		{
			double pTime, pBpm; int pShape;
			tempoMap.GetPoint(i - 1, &pTime, &pBpm, &pShape, NULL);

			double measure;
			if (prevOldShape == SQUARE) measure = (oldTime-prevOldTime) * prevOldBpm          / 240;
			else                        measure = (oldTime-prevOldTime) * (oldBpm+prevOldBpm) / 480;

			if (pShape == SQUARE) newTime = pTime + (240*measure) / pBpm;
			else                  newTime = pTime + (480*measure) / (pBpm + newBpm);
		}

		tempoMap.GetPoint(i, &prevOldTime, &prevOldBpm, &prevOldShape, NULL);
		tempoMap.SetPoint(i, &newTime,     &newBpm,     &newShape,     NULL);
	}
	tempoMap.Commit();
}

static void UpdateTargetBpm (HWND hwnd, int doFirst, int doCursor, int doLast)
{
	char bpm1Cur[128], bpm2Cur[128], bpm3Cur[128];

	GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_1, bpm1Cur, 128);
	GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_2, bpm2Cur, 128);
	GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_3, bpm3Cur, 128);

	if (AltAtof(bpm1Cur) == 0)
	{
		snprintf(bpm1Cur, sizeof(bpm1Cur), "%d", 0);
		snprintf(bpm2Cur, sizeof(bpm2Cur), "%d", 0);
		snprintf(bpm3Cur, sizeof(bpm3Cur), "%d", 0);
	}
	else
	{
		bool doPercent = !!IsDlgButtonChecked(hwnd, IDC_BR_ADJ_BPM_PERC_ENB);
		char bpmAdj[128]; GetDlgItemText(hwnd, (doPercent ? IDC_BR_ADJ_BPM_PERC : IDC_BR_ADJ_BPM_VAL), bpmAdj, 128);
		double bpm1Tar, bpm2Tar, bpm3Tar;

		if (doPercent)
		{
			bpm1Tar = (1 + AltAtof(bpmAdj)/100) * AltAtof(bpm1Cur);
			bpm2Tar = (1 + AltAtof(bpmAdj)/100) * AltAtof(bpm2Cur);
			bpm3Tar = (1 + AltAtof(bpmAdj)/100) * AltAtof(bpm3Cur);

		}
		else
		{
			bpm1Tar = AltAtof(bpmAdj) + AltAtof(bpm1Cur);
			bpm2Tar = AltAtof(bpmAdj) + AltAtof(bpm2Cur);
			bpm3Tar = AltAtof(bpmAdj) + AltAtof(bpm3Cur);
		}

		snprintf(bpm1Cur, sizeof(bpm1Cur), "%.6g", SetToBounds(bpm1Tar, (double)MIN_BPM, (double)MAX_BPM));
		snprintf(bpm2Cur, sizeof(bpm2Cur), "%.6g", SetToBounds(bpm2Tar, (double)MIN_BPM, (double)MAX_BPM));
		snprintf(bpm3Cur, sizeof(bpm3Cur), "%.6g", SetToBounds(bpm3Tar, (double)MIN_BPM, (double)MAX_BPM));
	}

	if (doFirst)  SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_TAR_1, bpm1Cur);
	if (doCursor) SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_TAR_2, bpm2Cur);
	if (doLast)   SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_TAR_3, bpm3Cur);
}

static void UpdateCurrentBpm (HWND hwnd, const vector<int>& selectedPoints)
{
	double bpmFirst = 0, bpmCursor = 0, bpmLast = 0;
	char eBpmFirst[128], eBpmCursor[128], eBpmLast[128], eBpmFirstChk[128], eBpmCursorChk[128], eBpmLastChk[128];

	// Get BPM info on selected points
	if (selectedPoints.size() != 0)
	{
		GetTempoTimeSigMarker(NULL, selectedPoints.front(), NULL, NULL, NULL, &bpmFirst, NULL, NULL, NULL);
		GetTempoTimeSigMarker(NULL, selectedPoints.back(), NULL, NULL, NULL, &bpmLast, NULL, NULL, NULL);
	}
	bpmCursor = TempoAtPosition(GetCursorPositionEx(NULL));
	snprintf(eBpmFirst,  sizeof(eBpmFirst),  "%.6g", bpmFirst);
	snprintf(eBpmCursor, sizeof(eBpmCursor), "%.6g", bpmCursor);
	snprintf(eBpmLast,   sizeof(eBpmLast),   "%.6g", bpmLast);

	GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_1, eBpmFirstChk, 128);
	GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_2, eBpmCursorChk, 128);
	GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_3, eBpmLastChk, 128);

	// Update edit boxes only if they've changed
	if (atof(eBpmFirst) != atof(eBpmFirstChk) || atof(eBpmCursor) != atof(eBpmCursorChk) || atof(eBpmLast) != atof(eBpmLastChk))
	{
		// Update current edit boxes
		SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_1, eBpmFirst);
		SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_2, eBpmCursor);
		SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_CUR_3, eBpmLast);

		// Update target edit boxes
		UpdateTargetBpm(hwnd, 1, 1, 1);
	}
}

static void UpdateSelectionFields (HWND hwnd)
{
	char eBpmStart[128], eBpmEnd[128], eNum[128], eDen[128];
	GetDlgItemText(hwnd, IDC_BR_SEL_BPM_START, eBpmStart, 128);
	GetDlgItemText(hwnd, IDC_BR_SEL_BPM_END, eBpmEnd, 128);
	GetDlgItemText(hwnd, IDC_BR_SEL_SIG_NUM, eNum, 128);
	GetDlgItemText(hwnd, IDC_BR_SEL_SIG_DEN, eDen, 128);

	double bpmStart = AltAtof(eBpmStart);
	double bpmEnd = AltAtof(eBpmEnd);
	int num = atoi(eNum); if (num < MIN_SIG){num = MIN_SIG;} else if (num > MAX_SIG){num = MAX_SIG;}
	int den = atoi(eDen); if (den < MIN_SIG){den = MIN_SIG;} else if (den > MAX_SIG){den = MAX_SIG;}

	// Update edit boxes with "atoied/atofed" values
	snprintf(eBpmStart, sizeof(eBpmStart), "%.19g", bpmStart);
	snprintf(eBpmEnd,   sizeof(eBpmEnd),   "%.19g", bpmEnd);
	snprintf(eNum,      sizeof(eNum),      "%d", num);
	snprintf(eDen,      sizeof(eDen),      "%d", den);
	SetDlgItemText(hwnd, IDC_BR_SEL_BPM_START, eBpmStart);
	SetDlgItemText(hwnd, IDC_BR_SEL_BPM_END, eBpmEnd);
	SetDlgItemText(hwnd, IDC_BR_SEL_SIG_NUM, eNum);
	SetDlgItemText(hwnd, IDC_BR_SEL_SIG_DEN, eDen);
}

static void SelectTempoCase (HWND hwnd, int operationType, int unselectNth)
{
	/*
	operation type --> 0 to select
	               --> 1 to unselect, if unselectNth > 1 unselect every Nth selected marker
	               --> 2 to invert
	*/

	int mode;
	if (IsDlgButtonChecked(hwnd, IDC_BR_SEL_ADD))
		mode = 4; // Add to existing selection
	else
		mode = 3; // Create new selection

	int bpm = IsDlgButtonChecked(hwnd, IDC_BR_SEL_BPM);
	int sig = IsDlgButtonChecked(hwnd, IDC_BR_SEL_SIG);
	int timeSel = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_TIME_RANGE, CB_GETCURSEL, 0, 0);
	int shape = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_SHAPE, CB_GETCURSEL, 0, 0);
	int type = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_TYPE, CB_GETCURSEL, 0, 0);

	char eBpmStart[128], eBpmEnd[128], eNum[128], eDen[128];
	GetDlgItemText(hwnd, IDC_BR_SEL_BPM_START, eBpmStart, 128);
	GetDlgItemText(hwnd, IDC_BR_SEL_BPM_END, eBpmEnd, 128);
	GetDlgItemText(hwnd, IDC_BR_SEL_SIG_NUM, eNum, 128);
	GetDlgItemText(hwnd, IDC_BR_SEL_SIG_DEN, eDen, 128);
	double bpmStart = AltAtof(eBpmStart);
	double bpmEnd   = AltAtof(eBpmEnd);
	int num = atoi(eNum); if (num < MIN_SIG){num = MIN_SIG;} else if (num > MAX_SIG){num = MAX_SIG;}
	int den = atoi(eDen); if (den < MIN_SIG){den = MIN_SIG;} else if (den > MAX_SIG){den = MAX_SIG;}

	if (bpmStart > bpmEnd)
	{
		double temp = bpmStart;
		bpmStart = bpmEnd;
		bpmEnd = temp;
	}

	// Select
	if (operationType == 0)
		SelectTempo(mode, 0, timeSel, bpm, bpmStart, bpmEnd, shape, sig, num, den, type);

	// Unselect
	if (operationType == 1)
	{
		if (unselectNth == 0)
			SelectTempo(5, 0, timeSel, bpm, bpmStart, bpmEnd, shape, sig, num, den, type);
		else
			SelectTempo(6, unselectNth, timeSel, bpm, bpmStart, bpmEnd, shape, sig, num, den, type);
	}

	// Invert
	if (operationType == 2)
		SelectTempo (7, 0, timeSel, bpm, bpmStart, bpmEnd, shape, sig, num, den, type);
}

static void AdjustTempoCase (HWND hwnd)
{
	char eBpmVal[128], eBpmPerc[128];
	GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_VAL, eBpmVal, 128);
	GetDlgItemText(hwnd, IDC_BR_ADJ_BPM_PERC, eBpmPerc, 128);
	double bpmVal = AltAtof(eBpmVal);
	double bpmPerc = AltAtof(eBpmPerc);

	// Update edit boxes
	UpdateTargetBpm(hwnd, 1, 1, 1);
	snprintf(eBpmVal,  sizeof(eBpmVal),  "%.6g", bpmVal);
	snprintf(eBpmPerc, sizeof(eBpmPerc), "%.6g", bpmPerc);
	SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_VAL, eBpmVal);
	SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_PERC, eBpmPerc);

	int mode; double bpm;
	if (IsDlgButtonChecked(hwnd, IDC_BR_ADJ_BPM_VAL_ENB))
	{
		mode = 0;       // Adjust by value
		bpm = bpmVal;
	}
	else
	{
		mode = 1;       // Adjust by percentage
		bpm = bpmPerc;
	}
	int shape = (int)SendDlgItemMessage(hwnd, IDC_BR_ADJ_SHAPE, CB_GETCURSEL, 0, 0);

	// Adjust markers
	if (bpm != 0 || shape != 0)
	{
		Undo_BeginBlock2(NULL);
		AdjustTempo(mode, bpm, shape);
		Undo_EndBlock2(NULL, __LOCALIZE("Adjust selected tempo markers","sws_undo"), UNDO_STATE_TRACKCFG);

	}
}

static void SaveOptionsSelAdj (HWND hwnd)
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
	int timeSel = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_TIME_RANGE, CB_GETCURSEL, 0, 0);
	int shape = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_SHAPE, CB_GETCURSEL, 0, 0);
	int type = (int)SendDlgItemMessage(hwnd, IDC_BR_SEL_TYPE, CB_GETCURSEL, 0, 0);
	int selPref = IsDlgButtonChecked(hwnd, IDC_BR_SEL_ADD);
	int invertPref = IsDlgButtonChecked(hwnd, IDC_BR_SEL_INVERT_PREF);
	int adjustType = IsDlgButtonChecked(hwnd, IDC_BR_ADJ_BPM_VAL_ENB);
	int adjustShape = (int)SendDlgItemMessage(hwnd, IDC_BR_ADJ_SHAPE, CB_GETCURSEL, 0, 0);

	char tmp[768];
	snprintf(tmp, sizeof(tmp), "%lf %lf %d %d %d %d %d %d %d %d %d %d %d", bpmStart, bpmEnd, num, den, bpmEnb, sigEnb, timeSel, shape, type, selPref, invertPref, adjustType, adjustShape);
	WritePrivateProfileString("SWS", SEL_ADJ_KEY, tmp, get_ini_file());
}

static void LoadOptionsSelAdj (double& bpmStart, double& bpmEnd, int& num, int& den, int& bpmEnb, int& sigEnb, int& timeSel, int& shape, int& type, int& selPref, int& invertPref, int& adjustType, int& adjustShape)
{
	char tmp[512];
	GetPrivateProfileString("SWS", SEL_ADJ_KEY, "", tmp, sizeof(tmp), get_ini_file());

	LineParser lp(false);
	lp.parse(tmp);
	bpmStart    = (lp.getnumtokens() > 0) ? lp.gettoken_float(0) : 120;
	bpmEnd      = (lp.getnumtokens() > 1) ? lp.gettoken_float(1) : 150;
	num         = (lp.getnumtokens() > 2) ? lp.gettoken_int(2)   : 4;
	den         = (lp.getnumtokens() > 3) ? lp.gettoken_int(3)   : 4;
	bpmEnb      = (lp.getnumtokens() > 4) ? lp.gettoken_int(4)   : 1;
	sigEnb      = (lp.getnumtokens() > 5) ? lp.gettoken_int(5)   : 0;
	timeSel     = (lp.getnumtokens() > 6) ? lp.gettoken_int(6)   : 0;
	shape       = (lp.getnumtokens() > 7) ? lp.gettoken_int(7)   : 0;
	type        = (lp.getnumtokens() > 8) ? lp.gettoken_int(8)   : 0;
	selPref     = (lp.getnumtokens() > 9) ? lp.gettoken_int(9)   : 0;
	invertPref  = (lp.getnumtokens() > 10) ? lp.gettoken_int(10) : 0;
	adjustType  = (lp.getnumtokens() > 11) ? lp.gettoken_int(11) : 0;
	adjustShape = (lp.getnumtokens() > 12) ? lp.gettoken_int(12) : 0;



	// Restore defaults if needed
	if (num < MIN_SIG || num > MAX_SIG)
		num = MIN_SIG;
	if (den < MIN_SIG || den > MAX_SIG)
		den = MIN_SIG;
	if (bpmEnb != 0 && bpmEnb != 1)
		bpmEnb = 1;
	if (sigEnb != 0 && sigEnb != 1)
		sigEnb = 0;
	if (timeSel < 0 || timeSel > 2)
		timeSel = 0;
	if (shape < 0 || shape > 2)
		shape = 0;
	if (type < 0 || type > 3)
		type = 0;
	if (selPref != 0 && selPref != 1)
		selPref = 1;
	if (invertPref != 0 && invertPref != 1)
		invertPref = 0;
	if (adjustType != 0 && adjustType != 1)
		adjustType = 1;
	if (adjustShape < 0 || adjustShape > 3)
		adjustShape = 0;
}

WDL_DLGRET UnselectNthProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static void UnselectNthDialog (bool show, bool toggle, HWND parentHandle)
{
	static HWND s_hwnd = NULL;

	if (toggle)
		show = !s_hwnd;

	if (show)
	{
		if (!s_hwnd)
		{
			s_hwnd = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_BR_UNSELECT_TEMPO), parentHandle, UnselectNthProc);
			#ifndef _WIN32
				SWELL_SetWindowLevel(s_hwnd, 3); // NSFloatingWindowLevel
			#endif
		}
		else
		{
			ShowWindow(s_hwnd, SW_SHOW);
			SetFocus(s_hwnd);
		}
	}
	else
	{
		if (s_hwnd)
		{
			DestroyWindow(s_hwnd);
			s_hwnd = NULL;
		}
	}
}

static void SaveOptionsUnselectNth (HWND hwnd)
{
	int Nth = (int)SendDlgItemMessage(hwnd, IDC_BR_UNSEL_NTH_TEMPO, CB_GETCURSEL, 0, 0);
	int criteria = IsDlgButtonChecked(hwnd, IDC_BR_UNSEL_CRITERIA);

	char tmp[512];
	snprintf(tmp, sizeof(tmp), "%d %d", Nth, criteria);
	WritePrivateProfileString("SWS", UNSEL_KEY, tmp, get_ini_file());
}

static void LoadOptionsUnselectNth (int& Nth, int& criteria)
{
	char tmp[512];
	GetPrivateProfileString("SWS", UNSEL_KEY, "", tmp, sizeof(tmp), get_ini_file());

	LineParser lp(false);
	lp.parse(tmp);
	Nth      = (lp.getnumtokens() > 0) ? lp.gettoken_int(0) : 0;
	criteria = (lp.getnumtokens() > 1) ? lp.gettoken_int(1) : 0;

	// Restore defaults if needed
	if (Nth < 0 || Nth > 14)
		Nth = 0;
	if (criteria != 0 && criteria != 1)
		criteria = 1;
}

WDL_DLGRET UnselectNthProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, uMsg, wParam, lParam))
		return r;

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			// Drop down menu
			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_BR_UNSEL_NTH_TEMPO));
			SendDlgItemMessage(hwnd, IDC_BR_UNSEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("2nd","sws_DLG_168"));
			SendDlgItemMessage(hwnd, IDC_BR_UNSEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("3rd","sws_DLG_168"));
			SendDlgItemMessage(hwnd, IDC_BR_UNSEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("4th","sws_DLG_168"));
			SendDlgItemMessage(hwnd, IDC_BR_UNSEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("5th","sws_DLG_168"));
			SendDlgItemMessage(hwnd, IDC_BR_UNSEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("6th","sws_DLG_168"));
			SendDlgItemMessage(hwnd, IDC_BR_UNSEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("7th","sws_DLG_168"));
			SendDlgItemMessage(hwnd, IDC_BR_UNSEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("8th","sws_DLG_168"));
			SendDlgItemMessage(hwnd, IDC_BR_UNSEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("9th","sws_DLG_168"));
			SendDlgItemMessage(hwnd, IDC_BR_UNSEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("10th","sws_DLG_168"));
			SendDlgItemMessage(hwnd, IDC_BR_UNSEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("11th","sws_DLG_168"));
			SendDlgItemMessage(hwnd, IDC_BR_UNSEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("12th","sws_DLG_168"));
			SendDlgItemMessage(hwnd, IDC_BR_UNSEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("13th","sws_DLG_168"));
			SendDlgItemMessage(hwnd, IDC_BR_UNSEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("14th","sws_DLG_168"));
			SendDlgItemMessage(hwnd, IDC_BR_UNSEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("15th","sws_DLG_168"));
			SendDlgItemMessage(hwnd, IDC_BR_UNSEL_NTH_TEMPO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("16th","sws_DLG_168"));

			// Load options from .ini
			int Nth, criteria;
			LoadOptionsUnselectNth(Nth, criteria);

			// Set controls
			SendDlgItemMessage(hwnd, IDC_BR_UNSEL_NTH_TEMPO, CB_SETCURSEL, Nth, 0);
			CheckDlgButton(hwnd, IDC_BR_UNSEL_CRITERIA, !!criteria);

			RestoreWindowPos(hwnd, UNSEL_WND, false);
			ShowWindow(hwnd, SW_SHOW);
			SetFocus(hwnd);
		}
		break;

		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case IDOK:
				{
					int Nth = (int)SendDlgItemMessage(hwnd, IDC_BR_UNSEL_NTH_TEMPO, CB_GETCURSEL, 0, 0) + 2;
					Undo_BeginBlock2(NULL);
					if (IsDlgButtonChecked(hwnd, IDC_BR_UNSEL_CRITERIA))
						SelectTempoCase(GetParent(hwnd), 1, Nth);
					else
						SelectTempo(2, Nth, 0, 0, 0, 0, 0, 0, 0, 0, 0);
					Undo_EndBlock2(NULL, __LOCALIZE("Unselect tempo markers","sws_undo"), UNDO_STATE_TRACKCFG);
				}
				break;

				case IDCANCEL:
				{
					UnselectNthDialog(false, false, NULL);
				}
				break;
			}
		}
		break;

		case WM_DESTROY:
		{
			SaveWindowPos(hwnd, UNSEL_WND);
			SaveOptionsUnselectNth(hwnd);
		}
		break;
	}
	return 0;
}

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
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TIME_RANGE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("All","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TIME_RANGE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Time selection","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TIME_RANGE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Ignore time selection","sws_DLG_167"));

			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_BR_SEL_SHAPE));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_SHAPE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("All","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_SHAPE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Square","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_SHAPE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Linear","sws_DLG_167"));

			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_BR_SEL_TYPE));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TYPE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("All","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TYPE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Tempo marker","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TYPE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Time signature","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TYPE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Partial time signature","sws_DLG_167"));

			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_BR_ADJ_SHAPE));
			SendDlgItemMessage(hwnd, IDC_BR_ADJ_SHAPE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Preserve","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_ADJ_SHAPE,CB_ADDSTRING,  0, (LPARAM)__LOCALIZE("Invert","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_ADJ_SHAPE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Linear","sws_DLG_167"));
			SendDlgItemMessage(hwnd, IDC_BR_ADJ_SHAPE, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Square","sws_DLG_167"));

			// Load options from .ini
			double bpmStart, bpmEnd;
			char eBpmStart[128], eBpmEnd[128], eNum[128], eDen[128];
			int num, den, bpmEnb, sigEnb, timeSel, shape, type, selPref, invertPref, adjustType, adjustShape;
			LoadOptionsSelAdj(bpmStart, bpmEnd, num, den, bpmEnb, sigEnb, timeSel, shape, type, selPref, invertPref, adjustType, adjustShape);
			snprintf(eBpmStart, sizeof(eBpmStart), "%.6g", bpmStart);
			snprintf(eBpmEnd,   sizeof(eBpmEnd),   "%.6g", bpmEnd);
			snprintf(eNum,      sizeof(eNum),      "%d", num);
			snprintf(eDen,      sizeof(eDen),      "%d", den);

			double effBpmCursor = TempoAtPosition(GetCursorPositionEx(NULL));
			char bpmCursor[128];
			snprintf(bpmCursor, sizeof(bpmCursor), "%.6g", effBpmCursor);

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
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TYPE, CB_SETCURSEL, type, 0);
			SendDlgItemMessage(hwnd, IDC_BR_SEL_TIME_RANGE, CB_SETCURSEL, timeSel, 0);
			SendDlgItemMessage(hwnd, IDC_BR_ADJ_SHAPE, CB_SETCURSEL, adjustShape, 0);
			#ifndef _WIN32
				RECT r;
				GetWindowRect(GetDlgItem(hwnd, IDC_BR_SEL_BPM_STATIC), &r); ScreenToClient(hwnd, (LPPOINT)&r);
				SetWindowPos(GetDlgItem(hwnd, IDC_BR_SEL_BPM_STATIC), HWND_BOTTOM, r.left-3, r.top+2, 0, 0, SWP_NOSIZE);
				GetWindowRect(GetDlgItem(hwnd, IDC_BR_SEL_SIG_STATIC), &r); ScreenToClient(hwnd, (LPPOINT)&r);
				SetWindowPos(GetDlgItem(hwnd, IDC_BR_SEL_SIG_STATIC), HWND_BOTTOM, r.left-3, r.top, 0, 0, SWP_NOSIZE);
			#endif

			RestoreWindowPos(hwnd, SEL_ADJ_WND, false);
			SetTimer(hwnd, 1, 500, NULL);
			ShowWindow(hwnd, SW_SHOW);
			SetFocus(hwnd);
		}
		break;

		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				/* SELECT MARKERS */

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
					UpdateSelectionFields(hwnd);
					SelectTempoCase(hwnd, 0, 0);
					Undo_EndBlock2(NULL, __LOCALIZE("Select tempo markers","sws_undo"), UNDO_STATE_TRACKCFG);
				}
				break;

				case IDC_BR_SEL_UNSELECT:
				{
					Undo_BeginBlock2(NULL);
					UpdateSelectionFields(hwnd);
					SelectTempoCase(hwnd, 1, 0);
					Undo_EndBlock2(NULL, __LOCALIZE("Unselect tempo markers","sws_undo"), UNDO_STATE_TRACKCFG);
				}
				break;

				case IDC_BR_SEL_INVERT:
				{
					Undo_BeginBlock2(NULL);
					UpdateSelectionFields(hwnd);
					if (IsDlgButtonChecked(hwnd, IDC_BR_SEL_INVERT_PREF) == 0)
						SelectTempo(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
					else
						SelectTempoCase(hwnd, 2, 0);
					Undo_EndBlock2(NULL, __LOCALIZE("Invert selection of tempo markers","sws_undo"), UNDO_STATE_TRACKCFG);
				}
				break;

				case IDC_BR_SEL_CLEAR:
				{
					Undo_BeginBlock2(NULL);
					UpdateSelectionFields(hwnd);
					SelectTempo(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
					Undo_EndBlock2(NULL, __LOCALIZE("Unselect tempo markers","sws_undo"), UNDO_STATE_TRACKCFG);
				}
				break;

				case IDC_BR_SEL_UNSELECT_NTH:
				{
					UnselectNthDialog(true, true, hwnd);
				}
				break;


				/* ADJUST MARKERS */

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

						snprintf(bpmTar, sizeof(bpmTar), "%.6g", bpmVal);
						snprintf(bpmCur, sizeof(bpmCur), "%.6g", bpmPerc);
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

						snprintf(bpmTar, sizeof(bpmTar), "%.6g", bpmVal);
						snprintf(bpmCur, sizeof(bpmCur), "%.6g", bpmPerc);
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

						snprintf(bpmTar, sizeof(bpmTar), "%.6g", bpmVal);
						snprintf(bpmCur, sizeof(bpmCur), "%.6g", bpmPerc);
						SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_VAL, bpmTar);
						SetDlgItemText(hwnd, IDC_BR_ADJ_BPM_PERC, bpmCur);
						UpdateTargetBpm(hwnd, 1, 1, 0);
					}
				}
				break;

				// Control buttons
				case IDC_BR_ADJ_APPLY:
				{
					AdjustTempoCase(hwnd);
				}
				break;

				case IDCANCEL:
				{
					SelectAdjustTempoDialog(NULL);
				}
				break;
			}
		}
		break;

		case WM_TIMER:
		{
			static WDL_FastString* s_title = NULL;
			if (!s_title)
			{
				if ((s_title = new (nothrow) WDL_FastString()))
				{
					s_title->AppendFormatted(62, "%s", "SWS/BR - ");
					s_title->AppendFormatted(512, "%s", __LOCALIZE_VERFMT("Select and adjust tempo markers (%d of %d points selected)", "sws_DLG_167"));
				}
			}

			if (s_title)
			{
				vector<int> selectedPoints = GetSelPoints(GetTempoEnv());
				char pointCount[512];
				snprintf(pointCount, sizeof(pointCount), s_title->Get(), selectedPoints.size(), CountTempoTimeSigMarkers(NULL) );
				SetWindowText(hwnd, pointCount);
				UpdateCurrentBpm(hwnd, selectedPoints);
			}
		}
		break;

		case WM_DESTROY:
		{
			KillTimer(hwnd, 1);
			UnselectNthDialog(false, false, NULL);
			SaveWindowPos(hwnd, SEL_ADJ_WND);
			SaveOptionsSelAdj(hwnd);
		}
		break;
	}
	return 0;
}

/******************************************************************************
* Randomize selected tempo markers                                            *
******************************************************************************/
static void SetRandomTempo (HWND hwnd, BR_Envelope* oldTempo, double min, double max, int unit, double minLimit, double maxLimit, int unitLimit, int limit)
{
	BR_Envelope tempoMap = *oldTempo;
	const int timeBase = ConfigVar<int>("tempoenvtimelock").value_or(0); // TEMPO MARKERS timebase (not project timebase)

	// Hold value for previous point
	double t0, b0;
	int s0;
	tempoMap.GetPoint(0, &t0, &b0, &s0, NULL);
	double t0_old = t0;
	double b0_old = b0;

	// Go through all tempo points and randomize tempo
	for (int i = 0; i < tempoMap.CountPoints(); ++i)
	{
		double t1, b1;
		int s1;
		tempoMap.GetPoint(i, &t1, &b1, &s1, NULL);

		double newBpm = b1;
		if (tempoMap.GetSelection(i))
		{
			double random = g_MTRand.rand();

			if (unit == 0) newBpm = (b1 + min) + ((max-min) * random);
			else           newBpm = (b1 * (100 + min + random*(max-min)))/100;

			if (limit)
			{
				if   (unitLimit == 0) newBpm = SetToBounds(newBpm, minLimit, maxLimit);
				else                  newBpm = SetToBounds(newBpm, b1 * (1 + minLimit/100), b1 * (1 + maxLimit/100));
			}
			newBpm = SetToBounds(newBpm, MIN_BPM, MAX_BPM);
		}

		double newTime = t1;
		if (timeBase == 1)
		{
			if (s0 == SQUARE)
				newTime = t0 + ((t1 - t0_old) * b0_old) / b0;
			else
				newTime = t0 + ((t1 - t0_old) * (b0_old + b1)) / (b0 + newBpm);
		}

		t0 = newTime;
		b0 = newBpm;
		s0 = s1;
		t0_old = t1;
		b0_old = b1;
		tempoMap.SetPoint(i, &newTime, &newBpm, NULL, NULL);
	}

	if (!tempoMap.IsLocked())
		tempoMap.Commit(true);
}

static void SaveOptionsRandomizeTempo (HWND hwnd)
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
	int unit = (int)SendDlgItemMessage(hwnd, IDC_BR_RAND_UNIT, CB_GETCURSEL, 0, 0);
	int unitLimit = (int)SendDlgItemMessage(hwnd, IDC_BR_RAND_LIMIT_UNIT, CB_GETCURSEL, 0, 0);
	int limit = IsDlgButtonChecked(hwnd, IDC_BR_RAND_LIMIT);

	char tmp[1308];
	snprintf(tmp, sizeof(tmp), "%lf %lf %d %lf %lf %d %d", min, max, unit, minLimit, maxLimit, unitLimit, limit);
	WritePrivateProfileString("SWS", RAND_KEY, tmp, get_ini_file());
}

static void LoadOptionsRandomizeTempo (double& min, double& max, int& unit, double& minLimit, double& maxLimit, int& unitLimit, int& limit)
{
	char tmp[512];
	GetPrivateProfileString("SWS", RAND_KEY, "", tmp, sizeof(tmp), get_ini_file());

	LineParser lp(false);
	lp.parse(tmp);
	min         = (lp.getnumtokens() > 0) ? lp.gettoken_float(0) : -1;
	max         = (lp.getnumtokens() > 1) ? lp.gettoken_float(1) : 1;
	unit        = (lp.getnumtokens() > 2) ? lp.gettoken_int(2)   : 0;
	minLimit    = (lp.getnumtokens() > 3) ? lp.gettoken_float(3) : 40;
	maxLimit    = (lp.getnumtokens() > 4) ? lp.gettoken_float(4) : 260;
	unitLimit   = (lp.getnumtokens() > 5) ? lp.gettoken_int(5)   : 0;
	limit       = (lp.getnumtokens() > 6) ? lp.gettoken_int(6)   : 0;



	// Restore defaults if needed
	if (unit != 0 && unit != 1)
		unit = 0;
	if (unitLimit != 0 && unitLimit != 1)
		unitLimit = 0;
	if (limit != 0 && limit != 1)
		limit = 0;
}

WDL_DLGRET RandomizeTempoProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, uMsg, wParam, lParam))
		return r;

	static BR_Envelope* s_oldTempo = NULL;
	static int s_undoMask;
	#ifndef _WIN32
		static bool s_positionSet = false;
	#endif
	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			s_oldTempo = new (nothrow) BR_Envelope(GetTempoEnv());

			// Save and set preferences
			if(ConfigVar<int> undomask = "undomask") {
				s_undoMask = *undomask;
				*undomask = ClearBit(s_undoMask, 3); // turn off undo for edit cursor
			}

			// Drop lists
			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_BR_RAND_UNIT));
			SendDlgItemMessage(hwnd, IDC_BR_RAND_UNIT, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("BPM","sws_DLG_171"));
			SendDlgItemMessage(hwnd, IDC_BR_RAND_UNIT, CB_ADDSTRING, 0, (LPARAM)"%");

			WDL_UTF8_HookComboBox(GetDlgItem(hwnd, IDC_BR_RAND_LIMIT_UNIT));
			SendDlgItemMessage(hwnd, IDC_BR_RAND_LIMIT_UNIT, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("BPM","sws_DLG_171"));
			SendDlgItemMessage(hwnd, IDC_BR_RAND_LIMIT_UNIT, CB_ADDSTRING, 0, (LPARAM)"%");

			// Load options from .ini
			double min, max, minLimit, maxLimit;
			char eMin[128], eMax[128], eMinLimit[128], eMaxLimit[128];
			int unit, unitLimit, limit;
			LoadOptionsRandomizeTempo (min, max, unit, minLimit, maxLimit, unitLimit, limit);
			snprintf(eMin,      sizeof(eMin),      "%.19g", min);
			snprintf(eMax,      sizeof(eMax),      "%.19g", max);
			snprintf(eMinLimit, sizeof(eMinLimit), "%.19g", minLimit);
			snprintf(eMaxLimit, sizeof(eMaxLimit), "%.19g", maxLimit);

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
			SetRandomTempo(hwnd, s_oldTempo, min, max, unit, minLimit, maxLimit, unitLimit, limit);

			#ifdef _WIN32
				RestoreWindowPos(hwnd, RAND_WND, false);
			#else
				s_positionSet = false;
			#endif
		}
		break;

		#ifndef _WIN32
			case WM_ACTIVATE:
			{
				// SetWindowPos doesn't seem to work in WM_INITDIALOG on OSX
				// when creating a dialog with DialogBox so call here
				if (!s_positionSet)
					RestoreWindowPos(hwnd, RAND_WND, false);
				s_positionSet = true;
			}
			break;
		#endif

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
					int unit = (int)SendDlgItemMessage(hwnd, IDC_BR_RAND_UNIT, CB_GETCURSEL, 0, 0);
					int unitLimit = (int)SendDlgItemMessage(hwnd, IDC_BR_RAND_LIMIT_UNIT, CB_GETCURSEL, 0, 0);

					// Update edit boxes with "atofed" values
					snprintf(eMin,      sizeof(eMin),      "%.19g", min);
					snprintf(eMax,      sizeof(eMax),      "%.19g", max);
					snprintf(eMinLimit, sizeof(eMinLimit), "%.19g", minLimit);
					snprintf(eMaxLimit, sizeof(eMaxLimit), "%.19g", maxLimit);
					SetDlgItemText(hwnd, IDC_BR_RAND_MIN, eMin);
					SetDlgItemText(hwnd, IDC_BR_RAND_MAX, eMax);
					SetDlgItemText(hwnd, IDC_BR_RAND_LIMIT_MIN, eMinLimit);
					SetDlgItemText(hwnd, IDC_BR_RAND_LIMIT_MAX, eMaxLimit);

					// Set new random tempo and preview it
					SetRandomTempo(hwnd, s_oldTempo, min, max, unit, minLimit, maxLimit, unitLimit, limit);
				}
				break;

				case IDOK:
				{
					ConfigVar<int>("undomask").try_set(s_undoMask); // treat undo behavior of edit cursor per user preference
					Undo_OnStateChangeEx2(NULL, __LOCALIZE("Randomize selected tempo markers", "sws_undo"), UNDO_STATE_TRACKCFG | UNDO_STATE_MISCCFG, -1);
					EndDialog(hwnd, 0);
				}
				break;

				case IDCANCEL:
				{
					s_oldTempo->Commit(true);
					EndDialog(hwnd, 0);
				}
				break;
			}
		}
		break;

		case WM_DESTROY:
		{
			delete s_oldTempo;
			s_oldTempo = NULL;
			SaveOptionsRandomizeTempo(hwnd);
			SaveWindowPos(hwnd, RAND_WND);

			// Restore preferences
			ConfigVar<int>("undomask").try_set(s_undoMask);
		}
		break;
	}
	return 0;
}

/******************************************************************************
* Set tempo marker shape (options)                                            *
******************************************************************************/
static void SaveOptionsTempoShape (HWND hwnd)
{
	int split = IsDlgButtonChecked(hwnd, IDC_BR_SHAPE_SPLIT);
	char splitRatio[128]; GetDlgItemText(hwnd, IDC_BR_SHAPE_SPLIT_RATIO, splitRatio, 128);

	char tmp[512];
	snprintf(tmp, sizeof(tmp), "%d %s", split, splitRatio);
	WritePrivateProfileString("SWS", SHAPE_KEY, tmp, get_ini_file());
}

static void LoadOptionsTempoShape (int& split, char* splitRatio, int splitRatioSz)
{
	char tmp[512];
	GetPrivateProfileString("SWS", SHAPE_KEY, "", tmp, sizeof(tmp), get_ini_file());

	LineParser lp(false);
	lp.parse(tmp);
	split = (lp.getnumtokens() > 0) ? lp.gettoken_int(0) : 0;
	strncpy(splitRatio, (lp.getnumtokens() > 1) ? lp.gettoken_str(1) : "4/8", splitRatioSz);

	// Restore defaults if needed
	double convertedRatio;
	IsFraction(splitRatio, convertedRatio);
	if (split != 0 && split != 1)
		split = 0;
	if (convertedRatio <= 0 || convertedRatio >= 1)
		strcpy(splitRatio, "0");
}

static void SetTempoShapeOptions (int& split, char* splitRatio)
{
	if (split == 0)
		g_tempoShapeSplitMiddle = 0;
	else
		g_tempoShapeSplitMiddle = 1;

	double convertedRatio; IsFraction(splitRatio, convertedRatio);
	if (convertedRatio <= 0 || convertedRatio >= 1)
		g_tempoShapeSplitRatio = 0;
	else
		g_tempoShapeSplitRatio = convertedRatio;
};

bool GetTempoShapeOptions (double* splitRatio)
{
	if (g_tempoShapeSplitMiddle == -1)
	{
		int split; char splitRatio[128];
		LoadOptionsTempoShape(split, splitRatio, sizeof(splitRatio));
		SetTempoShapeOptions (split, splitRatio);
	}

	WritePtr(splitRatio, g_tempoShapeSplitRatio);
	if (g_tempoShapeSplitMiddle == 1 && g_tempoShapeSplitRatio != 0)
		return true;
	else
		return false;
}

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
			SendDlgItemMessage(hwnd, IDC_BR_SHAPE_SPLIT_RATIO, CB_ADDSTRING, 0, (LPARAM)"1/2");
			SendDlgItemMessage(hwnd, IDC_BR_SHAPE_SPLIT_RATIO, CB_ADDSTRING, 0, (LPARAM)"1/3");
			SendDlgItemMessage(hwnd, IDC_BR_SHAPE_SPLIT_RATIO, CB_ADDSTRING, 0, (LPARAM)"1/4");

			// Load options from .ini file
			int split;
			char splitRatio[128];
			LoadOptionsTempoShape(split, splitRatio, sizeof(splitRatio));

			// Set controls and global variables
			SetDlgItemText(hwnd, IDC_BR_SHAPE_SPLIT_RATIO, splitRatio);
			CheckDlgButton(hwnd, IDC_BR_SHAPE_SPLIT, !!split);
			EnableWindow(GetDlgItem(hwnd, IDC_BR_SHAPE_SPLIT_RATIO), !!split);
			SetTempoShapeOptions(split, splitRatio);

			RestoreWindowPos(hwnd, SHAPE_WND, false);
			ShowWindow(hwnd, SW_SHOW);
			SetFocus(hwnd);
		}
		break;

		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case IDC_BR_SHAPE_SPLIT:
				{
					int split = IsDlgButtonChecked(hwnd, IDC_BR_SHAPE_SPLIT);
					char splitRatio[128]; GetDlgItemText(hwnd, IDC_BR_SHAPE_SPLIT_RATIO, splitRatio, 128);

					double convertedRatio; IsFraction(splitRatio, convertedRatio);
					if (convertedRatio <= 0 || convertedRatio >= 1)
						strcpy(splitRatio, "0");

					SetTempoShapeOptions (split, splitRatio);
					SetDlgItemText(hwnd, IDC_BR_SHAPE_SPLIT_RATIO, splitRatio);

					EnableWindow(GetDlgItem(hwnd, IDC_BR_SHAPE_SPLIT_RATIO), !!split);
				}
				break;

				case IDC_BR_SHAPE_SPLIT_RATIO:
				{
					int split = IsDlgButtonChecked(hwnd, IDC_BR_SHAPE_SPLIT);
					char splitRatio[128]; GetDlgItemText(hwnd, IDC_BR_SHAPE_SPLIT_RATIO, splitRatio, 128);

					double convertedRatio; IsFraction(splitRatio, convertedRatio);
					if (convertedRatio <= 0 || convertedRatio >= 1)
						strcpy(splitRatio, "0");

					SetTempoShapeOptions(split, splitRatio);
				}
				break;

				case IDCANCEL:
				{
					TempoShapeOptionsDialog(NULL);
				}
				break;
			}
		}
		break;

		case WM_DESTROY:
		{
			SaveWindowPos(hwnd, SHAPE_WND);
			SaveOptionsTempoShape(hwnd);
		}
		break;
	}
	return 0;
}
