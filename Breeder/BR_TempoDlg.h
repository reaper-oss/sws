/******************************************************************************
/ BR_TempoDlg.h

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
#pragma once

WDL_DLGRET ConvertMarkersToTempoProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void ConvertMarkersToTempo (int markers, int num, int den, bool removeMarkers, bool timeSel, bool gradualTempo, bool split, double splitRatio);
void ShowGradualOptions (bool show, HWND hwnd);
void LoadOptionsConversion (int &markers, int &num, int &den, int &removeMarkers, int &timeSel, int &gradual, int &split, char* splitRatio);
void SaveOptionsConversion (HWND hwnd);

WDL_DLGRET SelectAdjustTempoProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void SelectTempo (int mode, int Nth, int timeSel, int bpm, double bpmStart, double bpmEnd, int shape, int sig, int num, int den, int type);
void AdjustTempo (int mode, double bpm, int shape);
void UpdateTargetBpm (HWND hwnd, int doFirst, int doCursor, int doLast);
void UpdateCurrentBpm (HWND hwnd, const vector<int> &selectedPoints);
void UpdateSelectionFields (HWND hwnd);
void SelectTempoCase (HWND hwnd, int operationType, int unselectNth);
void AdjustTempoCase (HWND hwnd);
void UnselectNthDialog(bool show, HWND parentHandle);
void LoadOptionsSelAdj (double &bpmStart, double &bpmEnd, int &num, int &den, int &bpmEnb, int &sigEnb, int &timeSel, int &shape, int &type, int &selPref, int &invertPref, int &adjustType, int &adjustShape);
void SaveOptionsSelAdj (HWND hwnd);
WDL_DLGRET UnselectNthProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void LoadOptionsUnselectNth (int &Nth, int &criteria);
void SaveOptionsUnselectNth (HWND hwnd);

WDL_DLGRET TempoShapeOptionsProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void SetTempoShapeGlobalVariable (int split, char* splitRatio);
void LoadOptionsTempoShape (int &split, char* splitRatio);
void SaveOptionsTempoShape (HWND hwnd);

WDL_DLGRET RandomizeTempoProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void SetRandomTempo (HWND hwnd, char* oldTempo, double min, double max, int unit, double minLimit, double maxLimit, int unitLimit, int limit);
void SetPreRandTempo (char* oldTempo);
void LoadOptionsRandomizeTempo (double &min, double &max, int &unit, double &minLimit, double &maxLimit, int &unitLimit, int &limit);
void SaveOptionsRandomizeTempo (HWND hwnd);