/******************************************************************************
/ sws_rpf_wrapper.h
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

// reaper_plugin_functions.h (rpf) needs a little help, so instead of modifying
// the file directly, and needing to remodify with each rebuild of the file,
// fix some things here.

extern void (WINAPI *AttachWindowTopmostButton)(HWND hwnd); // v4 only
extern void (WINAPI *AttachWindowResizeGrip)(HWND hwnd); // v4 only
extern BOOL (WINAPI *CoolSB_GetScrollInfo)(HWND hwnd, int nBar, LPSCROLLINFO lpsi);
extern int (WINAPI *CoolSB_SetScrollInfo)(HWND hwnd, int nBar, LPSCROLLINFO lpsi, BOOL fRedraw);
extern void (WINAPI *MainThread_LockTracks)();
extern void (WINAPI *MainThread_UnlockTracks)();

#define WDL_VirtualWnd_ScaledBlitBG WDL_VirtualWnd_ScaledBlitBG_fptr
#define WDL_VirtualWnd_BGCfg WDL_VirtualWnd_BGCfg_stub

#include "reaper_plugin_functions.h"

#undef WDL_VirtualWnd_BGCfg
#undef WDL_VirtualWnd_ScaledBlitBG
