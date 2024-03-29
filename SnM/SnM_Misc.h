/******************************************************************************
/ SnM_Misc.h
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

#ifndef _SNM_MISC_H_
#define _SNM_MISC_H_

// reascript export
WDL_FastString* SNM_CreateFastString(const char* _str);
void SNM_DeleteFastString(WDL_FastString* _str);
const char* SNM_GetFastString(WDL_FastString* _str);
int SNM_GetFastStringLength(WDL_FastString* _str);
WDL_FastString* SNM_SetFastString(WDL_FastString* _str, const char* _newStr);
MediaItem_Take* SNM_GetMediaItemTakeByGUID(ReaProject* _project, const char* _guid);
bool SNM_GetSourceType(MediaItem_Take* _tk, WDL_FastString* _type);
bool SNM_GetSetSourceState(MediaItem* _item, int takeIdx, WDL_FastString* _state, bool _setnewvalue);
bool SNM_GetSetSourceState2(MediaItem_Take* _tk, WDL_FastString* _state, bool _setnewvalue);
bool SNM_GetSetObjectState(void* _obj, WDL_FastString* _state, bool _setnewvalue, bool _minstate);
bool SNM_SetProjectMarker(ReaProject* _proj, int _num, bool _isrgn, double _pos, double _rgnend, const char* _name, int _color);
bool SNM_GetProjectMarkerName(ReaProject* _proj, int _num, bool _isrgn, WDL_FastString* _name);
int SNM_GetIntConfigVarEx(ReaProject*, const char* varName, int errVal);
inline int SNM_GetIntConfigVar(const char* varName, int errVal) { return SNM_GetIntConfigVarEx(nullptr, varName, errVal); }
bool SNM_SetIntConfigVarEx(ReaProject*, const char* varName, int newVal);
inline bool SNM_SetIntConfigVar(const char* varName, int newVal) { return SNM_SetIntConfigVarEx(nullptr, varName, newVal); }
double SNM_GetDoubleConfigVarEx(ReaProject*, const char* varName, double errVal);
inline double SNM_GetDoubleConfigVar(const char* varName, double errVal) { return SNM_GetDoubleConfigVarEx(nullptr, varName, errVal); }
bool SNM_SetDoubleConfigVarEx(ReaProject*, const char* varName, double newVal);
inline bool SNM_SetDoubleConfigVar(const char* varName, double newVal) { return SNM_SetDoubleConfigVarEx(nullptr, varName, newVal); }
bool SNM_GetLongConfigVarEx(ReaProject*, const char* varName, int* highOut, int* lowOut);
inline bool SNM_GetLongConfigVar(const char* varName, int* highOut, int* lowOut) { return SNM_GetLongConfigVarEx(nullptr, varName, highOut, lowOut); }
bool SNM_SetLongConfigVarEx(ReaProject*, const char* varName, int newHighVal, int newLowVal);
inline bool SNM_SetLongConfigVar(const char* varName, int newHighVal, int newLowVal) { return SNM_SetLongConfigVarEx(nullptr, varName, newHighVal, newLowVal); }
bool SNM_SetStringConfigVar(const char* _varName, const char *_newVal);
const char* ULT_GetMediaItemNote(MediaItem* _item);
void ULT_SetMediaItemNote(MediaItem* _item, const char* _str);
bool SNM_ReadMediaFileTag(const char *fn, const char* tag, char* tagval, int tagval_sz);
bool SNM_TagMediaFile(const char *fn, const char* tag, const char* tagval);

// toolbar auto refresh
void EnableToolbarsAutoRefesh(COMMAND_T*);
int IsToolbarsAutoRefeshEnabled(COMMAND_T*);
void AutoRefreshToolbarRun();

// misc actions
void ChangeMetronomeVolume(COMMAND_T*);
void SimulateMouseClick(COMMAND_T*);
void DumpWikiActionList(COMMAND_T*);
void DumpActionList(COMMAND_T*);

#endif
