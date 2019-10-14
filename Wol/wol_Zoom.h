/******************************************************************************
/ wol_Zoom.h
/
/ Copyright (c) 2014 and later wol
/ http://forum.cockos.com/member.php?u=70153
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



void ToggleEnableEnvelopesExtendedZoom(COMMAND_T* = NULL);
int IsEnvelopesExtendedZoomEnabled(COMMAND_T* = NULL);

void AdjustSelectedEnvelopeOrTrackHeight(COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd);
void AdjustEnvelopeOrTrackHeightUnderMouse(COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd);

void SetVerticalZoomSelectedEnvelope(COMMAND_T* ct);

void ToggleEnableEnvelopeOverlap(COMMAND_T* = NULL);
int IsEnvelopeOverlapEnabled(COMMAND_T* = NULL);

void ForceEnvelopeOverlap(COMMAND_T* ct);

void ZoomSelectedEnvelopeTimeSelection(COMMAND_T* ct);
void VerticalZoomSelectedEnvelopeLoUpHalf(COMMAND_T* ct);

void SetVerticalZoomCenter(COMMAND_T* ct);
void SetHorizontalZoomCenter(COMMAND_T* ct);

void SaveApplyHeightSelectedEnvelopeSlot(COMMAND_T* ct);



void wol_ZoomInit();
