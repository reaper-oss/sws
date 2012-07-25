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
extern void (WINAPI *RemoveXPStyle)(HWND hwnd, int rm);
extern BOOL (WINAPI *CoolSB_GetScrollInfo)(HWND hwnd, int nBar, LPSCROLLINFO lpsi);
extern int (WINAPI *CoolSB_SetScrollInfo)(HWND hwnd, int nBar, LPSCROLLINFO lpsi, BOOL fRedraw);
extern void (WINAPI *MainThread_LockTracks)();
extern void (WINAPI *MainThread_UnlockTracks)();

// Avoid VWnd collisions!
#define WDL_VirtualWnd_ScaledBlitBG WDL_VirtualWnd_ScaledBlitBG_fptr
#define WDL_VirtualWnd_BGCfg WDL_VirtualWnd_BGCfg_stub

// Avoid LICE collisions!
#define LICE_LoadPNG REAPER_LICE_LoadPNG
#define LICE_LoadPNGFromResource REAPER_LICE_LoadPNGFromResource
#define LICE_PutPixel REAPER_LICE_PutPixel
#define LICE_GetPixel REAPER_LICE_GetPixel
#define LICE_Copy REAPER_LICE_Copy
#define LICE_Blit REAPER_LICE_Blit
#define LICE_Blur REAPER_LICE_Blur
#define LICE_ScaledBlit REAPER_LICE_ScaledBlit
#define LICE_RotatedBlit REAPER_LICE_RotatedBlit
#define LICE_GradRect REAPER_LICE_GradRect
#define LICE_FillRect REAPER_LICE_FillRect
#define LICE_Clear REAPER_LICE_Clear
#define LICE_ClearRect REAPER_LICE_ClearRect
#define LICE_MultiplyAddRect REAPER_LICE_MultiplyAddRect
#define LICE_SimpleFill REAPER_LICE_SimpleFill
#define LICE_DrawChar REAPER_LICE_DrawChar
#define LICE_DrawText REAPER_LICE_DrawText
#define LICE_MeasureText REAPER_LICE_MeasureText
#define LICE_Line REAPER_LICE_Line
#define LICE_FillTrapezoid REAPER_LICE_FillTrapezoid
#define LICE_FillConvexPolygon REAPER_LICE_FillConvexPolygon
#define LICE_FillTriangle REAPER_LICE_FillTriangle
#define LICE_ClipLine REAPER_LICE_ClipLine
#define LICE_Arc REAPER_LICE_Arc
#define LICE_Circle REAPER_LICE_Circle
#define LICE_FillCircle REAPER_LICE_FillCircle
#define LICE_RoundRect REAPER_LICE_RoundRect
#define LICE_DrawGlyph REAPER_LICE_DrawGlyph
#define LICE_DrawRect REAPER_LICE_DrawRect
#define LICE_BorderedRect REAPER_LICE_BorderedRect

#include "reaper_plugin_functions.h"

#undef LICE_LoadPNG
#undef LICE_LoadPNGFromResource
#undef LICE_PutPixel
#undef LICE_GetPixel
#undef LICE_Copy
#undef LICE_Blit
#undef LICE_Blur
#undef LICE_ScaledBlit
#undef LICE_RotatedBlit
#undef LICE_GradRect
#undef LICE_FillRect
#undef LICE_Clear
#undef LICE_ClearRect
#undef LICE_MultiplyAddRect
#undef LICE_SimpleFill
#undef LICE_DrawChar
#undef LICE_DrawText
#undef LICE_MeasureText
#undef LICE_Line
#undef LICE_FillTrapezoid
#undef LICE_FillConvexPolygon
#undef LICE_FillTriangle
#undef LICE_ClipLine
#undef LICE_Arc
#undef LICE_Circle
#undef LICE_FillCircle
#undef LICE_RoundRect
#undef LICE_DrawGlyph
#undef LICE_DrawRect
#undef LICE_BorderedRect

#undef WDL_VirtualWnd_BGCfg
#undef WDL_VirtualWnd_ScaledBlitBG
