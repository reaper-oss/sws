/******************************************************************************
/ Reaper.cpp
/
/ Copyright (c) 2011 Tim Payne (SWS)
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


// Sole purpose is to just include reaper_plugin_functions.h with REAPERAPI_DECL defined
#define REAPERAPI_DECL
#include "reaper_plugin.h"
#include "sws_rpf_wrapper.h"
//#include "reaper_plugin_functions.h"

// Not included in reaper_plugin_functions.h, so include here:
void (WINAPI *AttachWindowTopmostButton)(HWND hwnd);
void (WINAPI *AttachWindowResizeGrip)(HWND hwnd);
void (WINAPI *RemoveXPStyle)(HWND hwnd, int rm);
BOOL (WINAPI *CoolSB_GetScrollInfo)(HWND hwnd, int nBar, LPSCROLLINFO lpsi);
int (WINAPI *CoolSB_SetScrollInfo)(HWND hwnd, int nBar, LPSCROLLINFO lpsi, BOOL fRedraw);
void (WINAPI *MainThread_LockTracks)();
void (WINAPI *MainThread_UnlockTracks)();
