/******************************************************************************
/ SnM_Misc.h
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

#ifndef _SNM_MISC_H_
#define _SNM_MISC_H_

// reascript export
void GenAPI(COMMAND_T*);
MediaItem_Take* SNM_GetMediaItemTakeByGUID(ReaProject* _project, const char* _guid);
bool SNM_GetSetSourceState(MediaItem* _item, int takeIdx, char* _state, bool _setnewvalue);
bool SNM_AddReceive(MediaTrack* _srcTr, MediaTrack* _destTr, int _type);
int SNM_GetIntConfigVar(const char* _varName, int _errVal);
bool SNM_SetIntConfigVar(const char* _varName, int _newVal);
double SNM_GetDoubleConfigVar(const char* _varName, double _errVal);
bool SNM_SetDoubleConfigVar(const char* _varName, double _newVal);

// exotic resources slots
#ifdef _WIN32
void LoadThemeSlot(int _slotType, const char* _title, int _slot);
void LoadThemeSlot(COMMAND_T*);
#endif
void ShowImageSlot(int _slotType, const char* _title, int _slot);
void ShowImageSlot(COMMAND_T*);
void ShowNextPreviousImageSlot(COMMAND_T*);
void SetSelTrackIconSlot(int _slotType, const char* _title, int _slot);
void SetSelTrackIconSlot(COMMAND_T*);

// misc
bool WaitForTrackMute(DWORD* _muteTime);
void SimulateMouseClick(COMMAND_T*);
void DumpWikiActionList(COMMAND_T*);
void DumpActionList(COMMAND_T*);

#endif
