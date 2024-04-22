/******************************************************************************
/ Color.h
/
/ Copyright (c) 2010 Tim Payne (SWS)
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
#pragma once

extern COLORREF g_custColors[16]; // in native format (RGB or BGR), shared with REAPER
extern COLORREF g_crGradStart, g_crGradEnd; // in portable format (RGB)

void UpdateCustomColors();
void PersistColors();
bool AllBlack();
COLORREF CalcGradient(COLORREF crStart, COLORREF crEnd, double dPos);
int ColorInit();
void ColorExit();
void ShowColorDialog(COMMAND_T* = NULL);

int SWS_ColorToNative(int xrgb);
// int SWS_ColorFromNative(int bgr_or_rgb);
#define SWS_ColorFromNative SWS_ColorToNative

// convert to/from RGB with optional enable bit, or negative value
int ImportColor(int stored);
int ExportColor(int rgb_or_neg);
