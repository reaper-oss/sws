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

// LICE conflicts, so rename them!
#define LICE_LoadPNG				SUPPRESS_LICE_1
#define LICE_LoadPNGFromResource	SUPPRESS_LICE_2
#define LICE_PutPixel				SUPPRESS_LICE_3
#define LICE_GetPixel				SUPPRESS_LICE_4
#define LICE_Blit					SUPPRESS_LICE_5
#define LICE_Blur					SUPPRESS_LICE_6
#define LICE_ScaledBlit				SUPPRESS_LICE_7
#define LICE_RotatedBlit			SUPPRESS_LICE_8
#define LICE_GradRect				SUPPRESS_LICE_9
#define LICE_FillRect				SUPPRESS_LICE_10
#define LICE_Clear					SUPPRESS_LICE_11
#define LICE_ClearRect				SUPPRESS_LICE_12
#define LICE_MultiplyAddRect		SUPPRESS_LICE_13
#define LICE_SimpleFill				SUPPRESS_LICE_14
#define LICE_DrawChar				SUPPRESS_LICE_15
#define LICE_DrawText				SUPPRESS_LICE_16
#define LICE_MeasureText			SUPPRESS_LICE_17
#define LICE_Line					SUPPRESS_LICE_18
#define LICE_FillTriangle			SUPPRESS_LICE_19
#define LICE_ClipLine				SUPPRESS_LICE_20
#define LICE_Arc					SUPPRESS_LICE_21
#define LICE_Circle					SUPPRESS_LICE_22
#define LICE_FillCircle				SUPPRESS_LICE_23
#define LICE_RoundRect				SUPPRESS_LICE_24
#define LICE_DrawGlyph				SUPPRESS_LICE_25
#define LICE_DrawRect				SUPPRESS_LICE_26
#define LICE_BorderedRect			SUPPRESS_LICE_27

#ifdef _WIN32
#include "c:/program files/reaper/reaper_plugin_functions.h" // Point where you need to!
#else
#include "reaper_plugin_functions.h" // Point where you need to!
#endif

#undef LICE_LoadPNG
#undef LICE_LoadPNGFromResource
#undef LICE_PutPixel
#undef LICE_GetPixel
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
#undef LICE_FillTriangle
#undef LICE_ClipLine
#undef LICE_Arc
#undef LICE_Circle
#undef LICE_FillCircle
#undef LICE_RoundRect
#undef LICE_DrawGlyph
#undef LICE_DrawRect
#undef LICE_BorderedRect