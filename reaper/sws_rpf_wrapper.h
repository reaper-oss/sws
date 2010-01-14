/******************************************************************************
/ sws_rpf_wrapper.h
/
/ Copyright (c) 2009 Tim Payne (SWS)
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

extern BOOL (WINAPI *CoolSB_GetScrollInfo)(HWND hwnd, int nBar, LPSCROLLINFO lpsi);
extern int (WINAPI *CoolSB_SetScrollInfo)(HWND hwnd, int nBar, LPSCROLLINFO lpsi, BOOL fRedraw);

#define WDL_VirtualWnd_ScaledBlitBG WDL_VirtualWnd_ScaledBlitBG_fptr
#define WDL_VirtualWnd_BGCfg WDL_VirtualWnd_BGCfg_stub

#ifdef _WIN32
#include "c:/program files/reaper/reaper_plugin_functions.h" // Point where you need to!
#else
#include "reaper_plugin_functions.h" // Point where you need to!
#endif

#undef WDL_VirtualWnd_BGCfg
#undef WDL_VirtualWnd_ScaledBlitBG
