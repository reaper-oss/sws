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

int SnMActionsInit();

// SnM_fx.cpp
void toggleFXOfflineSelectedTracks(COMMAND_T* _ct);
void toggleFXBypassSelectedTracks(COMMAND_T* _ct);
int getSetFXOnline(int _type, MediaTrack * _tr, int _fx, int * _value);

// SnM_Windows.cpp
void closeWin(const char * _title);
void closeWindows(bool _routing, bool _env);
void closeRoutingWindows(COMMAND_T * _c);
void closeEnvWindows(COMMAND_T * _c);

// SnM_Sends.cpp
bool addSend(MediaTrack * _srcTr, MediaTrack * _destTr, int _type);
void cueTrack(COMMAND_T* _ct);

