/******************************************************************************
/ BR_TempoDlg.h

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
#pragma once

/******************************************************************************
* Convert project markers to tempo markers                                    *
******************************************************************************/
WDL_DLGRET ConvertMarkersToTempoProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/******************************************************************************
* Select and adjust tempo markers                                             *
******************************************************************************/
WDL_DLGRET SelectAdjustTempoProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/******************************************************************************
* Randomize selected tempo markers                                            *
******************************************************************************/
WDL_DLGRET RandomizeTempoProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/******************************************************************************
* Set tempo marker shape (options)                                            *
******************************************************************************/
WDL_DLGRET TempoShapeOptionsProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
bool GetTempoShapeOptions (double* splitRatio);
