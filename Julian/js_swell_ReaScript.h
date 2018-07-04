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

void  Window_SetFocus(void* windowHWND);
void* Window_GetFocus();
void  Window_SetForeground(void* windowHWND);
void* Window_GetForeground();

void  Window_Enable(void* windowHWND, bool enable);
void  Window_Destroy(void* windowHWND);
void  Window_Show(void* windowHWND, int state);

void* Window_SetCapture(void* windowHWND);
void* Window_GetCapture();
void  Window_ReleaseCapture();

bool  Window_SetTitle(void* windowHWND, const char* title);
void  Window_GetTitle(void* windowHWND, char* buf, int buf_sz);

void* Window_HandleFromAddress(int address);
bool  Window_IsWindow(void* windowHWND);

int   Mouse_GetState(int key);
bool  Mouse_SetPosition(int x, int y);
