/******************************************************************************
/ SnM_Project.h
/
/ Copyright (c) 2012-2013 Jeffos
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

#ifndef _SNM_PROJECT_H_
#define _SNM_PROJECT_H_


bool IsActiveProjectInLoadSave(char* _projfn = NULL, int _projfnSz = 0, bool _ensureRPP = false);
void TieFileToProject(const char* _fn, ReaProject* _prj = NULL, bool _tie = true);
void UntieFileFromProject(const char* _fn, ReaProject* _prj = NULL);
double GetProjectLength(bool _items = true, bool _inclRgnsMkrs = false);
bool InsertSilence(const char* _undoTitle, double _pos, double _len);

void SelectProject(MIDI_COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd);
void LoadOrSelectProject(const char* _fn, bool _newTab);

void LoadOrSelectProjectSlot(int _slotType, const char* _title, int _slot, bool _newTab);
bool AutoSaveProjectSlot(int _slotType, const char* _dirPath, WDL_PtrList<void>* _owSlots, bool _saveCurPrj);
void LoadOrSelectProjectSlot(COMMAND_T*);
void LoadOrSelectProjectTabSlot(COMMAND_T*);

bool IsProjectLoaderConfValid();
void ProjectLoaderConf(COMMAND_T*);
void LoadOrSelectNextPreviousProject(COMMAND_T*);

class ProjectActionJob : public SNM_ScheduledJob {
public:
	ProjectActionJob(int _cmdId) : m_cmdId(_cmdId), SNM_ScheduledJob(SNM_SCHEDJOB_PRJ_ACTION, 1000) {}
	void Perform() { if (m_cmdId) Main_OnCommand(m_cmdId, 0); }
	int m_cmdId;
};

void SetProjectStartupAction(COMMAND_T*);
void ClearProjectStartupAction(COMMAND_T*);
int ReaProjectInit();

void InsertSilence(COMMAND_T*);
void OpenProjectPathInExplorerFinder(COMMAND_T* _ct = NULL);

#endif

