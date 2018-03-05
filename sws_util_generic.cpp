/******************************************************************************
/ sws_util_generic.cpp
/
/ Copyright (c) 2010 ans later Tim Payne (SWS), Jeffos, Dominik Martin Drzic

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


void SWS_GetDateString(int time, char* buf, int bufsize)
{
  time_t a = time;
  struct tm *tm = a >= 0 && a < WDL_INT64_CONST(0x793406fff) ? localtime(&a) : NULL;
  if (!tm) lstrcpyn_safe(buf,"?",bufsize);
  else strftime(buf,bufsize,"%x",tm);
}

void SWS_GetTimeString(int time, char* buf, int bufsize)
{
  time_t a = time;
  struct tm *tm = a >= 0 && a < WDL_INT64_CONST(0x793406fff) ? localtime(&a) : NULL;
  if (!tm) lstrcpyn_safe(buf,"?",bufsize);
  else strftime(buf,bufsize,"%X",tm);
}

// Modified LoadCursor from Cockos WDL
// Supports these cursors:
// IDC_GRID_WARP
// IDC_ENV_PEN_GRID, IDC_ENV_PT_ADJ_VERT
// IDC_MISC_SPEAKER
// IDC_ZOOM_DRAG, IDC_ZOOM_IN, IDC_ZOOM_OUT, IDC_ZOOM_UNDO
HCURSOR SWS_LoadCursor(int id)
{
  return NULL;
}

void mouse_event(DWORD dwFlags, DWORD dx, DWORD dy, DWORD dwData, ULONG_PTR dwExtraInfo)
{
}

void SWS_ShowTextScrollbar(HWND hwnd, bool show)
{
    if (!hwnd) return;

}
