/******************************************************************************
/ SnM_Window.h
/
/ Copyright (c) 2012 and later Jeffos
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

//#pragma once

#ifndef _SNM_WINDOW_H_
#define _SNM_WINDOW_H_

bool SNM_IsActiveWindow(HWND _h);
bool IsChildOf(HWND _hChild, const char* _title);
HWND GetReaHwndByTitle(const char* _title);
void GetVisibleTCPTracks(WDL_PtrList<MediaTrack>* _trList);

void FocusMainWindow(COMMAND_T*);
#ifdef _WIN32
void CycleFocusWnd(COMMAND_T*);
void CycleFocusHideOthersWnd(COMMAND_T*);
void FocusMainWindowCloseOthers(COMMAND_T*);
#endif

void ShowThemeHelper(COMMAND_T*);

HWND GetActionListBox(char* _currentSection, int _sectionSz);
int GetSelectedAction(char* _section, int _secSize, int* _cmdId, char* _id, int _idSize, char* _desc = NULL, int _descSize = -1);
bool GetSelectedAction(char* _idstrOut, int _idStrSz, KbdSectionInfo* _expectedSection = NULL);

void ShowFXChain(COMMAND_T*);
void HideFXChain(COMMAND_T*);
void ToggleFXChain(COMMAND_T*);
int IsToggleFXChain(COMMAND_T*);
void ShowAllFXChainsWindows(COMMAND_T*);
void CloseAllFXChainsWindows(COMMAND_T*); // NF: includes take FX
void ToggleAllFXChainsWindows(COMMAND_T*);

void FloatUnfloatFXs(MediaTrack* _tr, bool _all, int _showFlag, int _fx, bool _selTracks);
void FloatUnfloatTakeFXs(MediaTrack* _tr, bool _all, int _showFlag, int _fx, bool _selTracks);
void FloatFX(COMMAND_T*);
void UnfloatFX(COMMAND_T*);
void ToggleFloatFX(COMMAND_T*);
void ShowAllFXWindows(COMMAND_T*);
void CloseAllFXWindows(COMMAND_T*); // NF: includes take FX
void CloseAllFXWindowsExceptFocused(COMMAND_T*); // NF: includes take FX
void ToggleAllFXWindows(COMMAND_T*);
int GetFocusedTrackFXWnd(MediaTrack* _tr);
int GetFirstTrackFXWnd(MediaTrack* _tr, int _dir);
#ifdef _SNM_MISC
void CycleFocusFXWndSelTracks(COMMAND_T*);
void CycleFocusFXWndAllTracks(COMMAND_T*);
#endif
void CycleFloatFXWndSelTracks(COMMAND_T*);
void CycleFocusFXMainWndAllTracks(COMMAND_T*);
void CycleFocusFXMainWndSelTracks(COMMAND_T*);

#endif
