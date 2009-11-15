/******************************************************************************
/ SnM_Actions.h
/
/ Copyright (c) 2009 Tim Payne (SWS), JF Bédague (S&M)
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


#pragma once

// +9 to skip "SWS/S&M: "
#define SNMSWS_ZAP(_ct) (_ct->accel.desc + 9)

int SnMActionsInit();

// SnM_fx.cpp
void toggleFXOfflineSelectedTracks(COMMAND_T* _ct);
void toggleFXBypassSelectedTracks(COMMAND_T* _ct);
int getSetFXState(int _mode, MediaTrack * _tr, int _fx, int * _value);
void toggleFXStateSelectedTracks(int _mode, int _fx, const char * _undoMsg);
void toggleExceptFXOfflineSelectedTracks(COMMAND_T* _ct);
void toggleExceptFXBypassSelectedTracks(COMMAND_T* _ct);
void toggleAllFXsOfflineSelectedTracks(COMMAND_T* _ct);
void toggleAllFXsBypassSelectedTracks(COMMAND_T* _ct);

// SnM_Windows.cpp
void toggleShowHideWin(const char * _title);
void closeWin(const char * _title);
void closeOrToggleWindows(bool _routing, bool _env, bool _toggle);
void closeRoutingWindows(COMMAND_T * _c);
void closeEnvWindows(COMMAND_T * _c);
void toggleRoutingWindows(COMMAND_T * _c);
void toggleEnvWindows(COMMAND_T * _c);

// SnM_Sends.cpp
bool addSend(MediaTrack * _srcTr, MediaTrack * _destTr, int _type);
void cueTrack(char * _busName, int _type, const char * _undoMsg);
void cueTrackPrompt(COMMAND_T* _ct);
void cueTrack(COMMAND_T* _ct);

