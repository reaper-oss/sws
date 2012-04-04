/******************************************************************************
/ SnM_Window.h
/
/ Copyright (c) 2012 Jeffos
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

//#pragma once

#ifndef _SNM_WINDOW_H_
#define _SNM_WINDOW_H_

bool SNM_IsActiveWindow(HWND _h);
bool IsChildOf(HWND _hChild, const char* _title, int _nComp = -1);
HWND GetReaWindowByTitle(const char* _title, int _nComp = -1);
HWND GetActionListBox(char* _currentSection, int _sectionSz);
int GetSelectedAction(char* _section, int _secSize, int* _cmdId, char* _id, int _idSize, char* _desc = NULL, int _descSize = -1);
void showFXChain(COMMAND_T*);
void hideFXChain(COMMAND_T*);
void toggleFXChain(COMMAND_T*);
bool isToggleFXChain(COMMAND_T*);
void showAllFXChainsWindows(COMMAND_T*);
void closeAllFXChainsWindows(COMMAND_T*);
void toggleAllFXChainsWindows(COMMAND_T*);
void floatUnfloatFXs(MediaTrack* _tr, bool _all, int _showFlag, int _fx, bool _selTracks);
void floatFX(COMMAND_T*);
void unfloatFX(COMMAND_T*);
void toggleFloatFX(COMMAND_T*);
void showAllFXWindows(COMMAND_T*);
void closeAllFXWindows(COMMAND_T*);
void closeAllFXWindowsExceptFocused(COMMAND_T*);
void toggleAllFXWindows(COMMAND_T*);
int getFocusedFX(MediaTrack* _tr, int _dir, int* _firstFound = NULL);
#ifdef _SNM_MISC
void cycleFocusFXWndSelTracks(COMMAND_T*);
void cycleFocusFXWndAllTracks(COMMAND_T*);
#endif
void cycleFloatFXWndSelTracks(COMMAND_T*);
void cycleFocusFXMainWndAllTracks(COMMAND_T*);
void cycleFocusFXMainWndSelTracks(COMMAND_T*);
void cycleFocusWnd(COMMAND_T*);
void cycleFocusHideOthersWnd(COMMAND_T*);
void focusMainWindow(COMMAND_T*);
void focusMainWindowCloseOthers(COMMAND_T*);
void ShowThemeHelper(COMMAND_T*);
void GetVisibleTCPTracks(WDL_PtrList<void>* _trList);

#endif