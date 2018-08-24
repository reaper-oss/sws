/******************************************************************************
/ js_swell_ReaScript.h
/
/ Copyright (c) 2017 The Human Race
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

bool  Window_GetRect(void* windowHWND, int* leftOut, int* topOut, int* rightOut, int* bottomOut);
bool  Window_GetClientRect(void* windowHWND, int* leftOut, int* topOut, int* rightOut, int* bottomOut);
void  Window_ScreenToClient	(void* windowHWND, int x, int y, int* xOut, int* yOut);
void  Window_ClientToScreen (void* windowHWND, int x, int y, int* xOut, int* yOut);

void* Window_FromPoint	(int x, int y);

void* Window_GetParent(void* windowHWND);
bool  Window_IsChild(void* parentHWND, void* childHWND);
void* Window_GetRelated(void* windowHWND, int relation);

void* Window_Find(const char* title, bool exact);
void* Window_FindChild(void* parentHWND, const char* title, bool exact);
void  Window_ListAllChild(void* parentHWND, const char* section, const char* key);
void  Window_ListAllTop(const char* section, const char* key);
void  Window_ListFind(const char* title, bool exact, const char* section, const char* key);
void  MIDIEditor_ListAll(char* buf, int buf_sz);

void  Window_Move(void* windowHWND, int left, int top);
void  Window_Resize(void* windowHWND, int width, int height);
void  Window_SetPosition(void* windowHWND, int left, int top, int width, int height);
bool  Window_SetZOrder(void* windowHWND, const char* ZOrder, void* insertAfterHWND, int flags);

void  Window_SetFocus(void* windowHWND);
void* Window_GetFocus();
void  Window_SetForeground(void* windowHWND);
void* Window_GetForeground();

void  Window_Enable(void* windowHWND, bool enable);
void  Window_Destroy(void* windowHWND);
void  Window_Show(void* windowHWND, int state);
bool  Window_IsVisible(void* windowHWND);

bool  Window_SetTitle(void* windowHWND, const char* title);
void  Window_GetTitle(void* windowHWND, char* buf, int buf_sz);

void* Window_HandleFromAddress(int addressLow32Bits, int addressHigh32Bits);
bool  Window_IsWindow(void* windowHWND);

int   WindowMessage_Intercept(void* windowHWND, const char* messages);
bool  WindowMessage_ListIntercepts(void* windowHWND, char* buf, int buf_sz);
bool  WindowMessage_Post(void* windowHWND, const char* message, int wParamLow, int wParamHigh, int lParamLow, int lParamHigh);
int   WindowMessage_Send(void* windowHWND, const char* message, int wParamLow, int wParamHigh, int lParamLow, int lParamHigh);
bool  WindowMessage_Peek(void* windowHWND, const char* message, double* timeOut, int* wParamLowOut, int* wParamHighOut, int* lParamLowOut, int* lParamHighOut);
int   WindowMessage_Release(void* windowHWND, const char* messages);
void  WindowMessage_ReleaseWindow(void* windowHWND);
void  WindowMessage_ReleaseAll();

int   Mouse_GetState(int flags);
bool  Mouse_SetPosition(int x, int y);
void* Mouse_LoadCursor(int cursorNumber);
void* Mouse_LoadCursorFromFile(const char* pathAndFileName);
void  Mouse_SetCursor(void* cursorHandle);

void* GDI_GetDC(void* windowHWND);
void* GDI_GetWindowDC(void* windowHWND);
void  GDI_ReleaseDC(void* windowHWND, void* deviceHDC);
void  GDI_RoundRect(void* deviceHDC, int x, int y, int x2, int y2, int xrnd, int yrnd);
void  GDI_DrawFocusRect(void* windowHWND, int left, int top, int right, int bottom);
void  GDI_SetPen(void* deviceHDC, int iStyle, int width, int color);
void  GDI_SetFont(void* deviceHDC, int height, int weight, int angle, bool italic, bool underline, bool strikeOut, const char* fontName);
void  GDI_Rectangle(void* deviceHDC, int left, int top, int right, int bottom);
void  GDI_RoundRect(void* deviceHDC, int left, int top, int right, int bottom, int xrnd, int yrnd);
void  GDI_FillRect(void* deviceHDC, int left, int top, int right, int bottom, int color);
void  GDI_SetBkMode(void* deviceHDC, int mode);
void  GDI_SetBkColor(void* deviceHDC, int color);
void  GDI_SetTextColor(void* deviceHDC, int color);
int   GDI_GetTextColor(void* deviceHDC);
int   GDI_DrawText(void* deviceHDC, const char *text, int len, int left, int top, int right, int bottom, int align);
void  GDI_SetPixel(void* deviceHDC, int x, int y, int color);
void  GDI_MoveTo(void* deviceHDC, int x, int y);
void  GDI_LineTo(void* deviceHDC, int x, int y);
void  GDI_Ellipse(void* deviceHDC, int left, int top, int right, int bottom);

void  GDI_DrawFocusRect(void* windowHWND, int left, int top, int right, int bottom);

bool  Window_GetScrollInfo(void* windowHWND, const char* scrollbar, int* positionOut, int* pageSizeOut, int* minOut, int* maxOut, int* trackPosOut);
bool  Window_SetScrollInfo(void* windowHWND, const char* scrollbar, int position, int min, int max);
bool  Window_Scroll(void* windowHWND, int XAmount, int YAmount);
bool  Window_InvalidateRect(void* windowHWND, int left, int top, int right, int bottom);
void  Window_Update(void* windowHWND);