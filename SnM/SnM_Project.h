/******************************************************************************
/ SnM_Project.h
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

#pragma once

#ifndef _SNM_PROJECT_H_
#define _SNM_PROJECT_H_

void SelectProject(MIDI_COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd);
void loadOrSelectProjectSlot(int _slotType, const char* _title, int _slot, bool _newTab);
bool autoSaveProjectSlot(int _slotType, const char* _dirPath, char* _fn, int _fnSize, bool _saveCurPrj);
void loadOrSelectProjectSlot(COMMAND_T*);
void loadOrSelectProjectTabSlot(COMMAND_T*);
bool isProjectLoaderConfValid();
void projectLoaderConf(COMMAND_T*);
void loadOrSelectNextPreviousProject(COMMAND_T*);
void openProjectPathInExplorerFinder(COMMAND_T*);

#endif

