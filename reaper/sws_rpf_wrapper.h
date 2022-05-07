/******************************************************************************
/ sws_rpf_wrapper.h
/
/ Copyright (c) 2011 and later Tim Payne (SWS), Jeffos
/
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

#ifndef _SWS_RPF_WRAPPER_H_
#define _SWS_RPF_WRAPPER_H_

// reaper_plugin_functions.h (rpf) needs a little help, so instead of modifying
// the file directly, and needing to remodify with each rebuild of the file,
// fix some things here.
#ifdef REAPERAPI_IMPLEMENT
#define REAPER_EXTRA_API_DECL
#else
#define REAPER_EXTRA_API_DECL extern
#endif

// Not included in reaper_plugin_functions.h, so include here:
REAPER_EXTRA_API_DECL void (*AttachWindowTopmostButton)(HWND hwnd);
REAPER_EXTRA_API_DECL void (*AttachWindowResizeGrip)(HWND hwnd);
REAPER_EXTRA_API_DECL BOOL (*RemoveXPStyle)(HWND hwnd, int rm);
REAPER_EXTRA_API_DECL BOOL (WINAPI *CoolSB_GetScrollInfo)(HWND hwnd, int nBar, LPSCROLLINFO lpsi);
REAPER_EXTRA_API_DECL int (WINAPI *CoolSB_SetScrollInfo)(HWND hwnd, int nBar, LPSCROLLINFO lpsi, BOOL fRedraw);
REAPER_EXTRA_API_DECL void (*MainThread_LockTracks)();
REAPER_EXTRA_API_DECL void (*MainThread_UnlockTracks)();
REAPER_EXTRA_API_DECL bool (*OnColorThemeOpenFile)(const char*);

// https://forum.cockos.com/showthread.php?t=227910
REAPER_EXTRA_API_DECL bool (*ListView_HeaderHitTest)(HWND, POINT);

// jcsteh/osara#359
REAPER_EXTRA_API_DECL bool (*osara_isShortcutHelpEnabled)();

// Avoid VWnd collisions
#define WDL_VirtualWnd_ScaledBlitBG WDL_VirtualWnd_ScaledBlitBG_fptr
#define WDL_VirtualWnd_BGCfg WDL_VirtualWnd_BGCfg_stub

// Avoid LICE collisions
#define REAPERAPI_NO_LICE
#include "WDL/lice/lice.h"

#ifndef _WIN32
// using RegisterClipboardFormat instead of constant CF_TEXT for compatibility
// with REAPER v5 (prior to WDL commit 0f77b72adf1cdbe98fd56feb41eb097a8fac5681)
#undef  CF_TEXT
#define CF_TEXT RegisterClipboardFormat("SWELL__CF_TEXT")
#endif

#include "reaper_plugin_functions.h"

#undef WDL_VirtualWnd_BGCfg
#undef WDL_VirtualWnd_ScaledBlitBG

#endif
