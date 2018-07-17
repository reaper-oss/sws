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
void  Window_GetClientRect	(void* windowHWND, int* leftOut, int* topOut, int* rightOut, int* bottomOut);
void  Window_ScreenToClient	(void* windowHWND, int x, int y, int* xOut, int* yOut);
void  Window_ClientToScreen (void* windowHWND, int x, int y, int* xOut, int* yOut);
bool  Window_GetScrollInfo(void* windowHWND, const char* bar, int* positionOut, int* pageOut, int* minOut, int* maxOut, int* trackPosOut);

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
bool  Window_SetZOrder(void* windowHWND, const char* ZOrder, void* insertAfterHWND, int flags);

void  Window_SetFocus(void* windowHWND);
void* Window_GetFocus();
void  Window_SetForeground(void* windowHWND);
void* Window_GetForeground();

void  Window_Enable(void* windowHWND, bool enable);
void  Window_Destroy(void* windowHWND);
void  Window_Show(void* windowHWND, int state);

bool  Window_SetTitle(void* windowHWND, const char* title);
void  Window_GetTitle(void* windowHWND, char* buf, int buf_sz);

void* Window_HandleFromAddress(int addressLow32Bits, int addressHigh32Bits);
bool  Window_IsWindow(void* windowHWND);

bool  Window_PostMessage(void* windowHWND, int message, int wParam, int lParamLow, int lParamHigh);
bool  Window_Intercept(void* windowHWND, const char* messages);
bool  Window_InterceptRelease(void* windowHWND);
bool  Window_InterceptPoll(void* windowHWND, const char* message, double* timeOut, int* xPosOut, int* yPosOut, int* keysOut, int* valueOut);

int   Mouse_GetState(int flags);
bool  Mouse_SetPosition(int x, int y);
void* Mouse_LoadCursor(int cursorNumber);
void* Mouse_LoadCursorFromFile(const char* pathAndFileName);
void  Mouse_SetCursor(void* cursorHandle);
bool  Mouse_Intercept(void* windowHWND, const char* messages, bool passThrough, char* buf, int buf_sz);
bool  Mouse_InterceptRelease(void* windowHWND);
