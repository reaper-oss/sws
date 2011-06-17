/******************************************************************************
/ SnM_Project.cpp
/
/ Copyright (c) 2011 Jeffos
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

#include "stdafx.h"
#include "SnM_Actions.h"
#include "SnM_FXChainView.h"


///////////////////////////////////////////////////////////////////////////////
// Select project ("MIDI CC absolute only" actions)
///////////////////////////////////////////////////////////////////////////////

class SNM_SelectProjectScheduledJob : public SNM_ScheduledJob
{
public:
	SNM_SelectProjectScheduledJob(int _approxDelayMs, int _val, int _valhw, int _relmode, HWND _hwnd) 
		: SNM_ScheduledJob(SNM_SCHEDJOB_SEL_PRJ, _approxDelayMs),m_val(_val),m_valhw(_valhw),m_relmode(_relmode),m_hwnd(_hwnd) {}
	void Perform() {
		ReaProject* proj = Enum_Projects(m_val, NULL, 0);
		if (proj) SelectProjectInstance(proj);
	}
protected:
	int m_val, m_valhw, m_relmode;
	HWND m_hwnd;
};

void SelectProject(MIDI_COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd) {
	if (!_relmode && _valhw < 0) { // Absolute CC only
		SNM_SelectProjectScheduledJob* job = 
			new SNM_SelectProjectScheduledJob(SNM_SCHEDJOB_DEFAULT_DELAY, _val, _valhw, _relmode, _hwnd);
		AddOrReplaceScheduledJob(job);
	}
}


///////////////////////////////////////////////////////////////////////////////
// Resources view: project template slots
///////////////////////////////////////////////////////////////////////////////

void loadOrSelectProject(const char* _title, int _slot, bool _newTab, bool _errMsg)
{
	// Prompt for slot if needed
	if (_slot == -1) _slot = g_prjTemplateFiles.PromptForSlot(_title); //loops on err
	if (_slot == -1) return; // user has cancelled

	char fn[BUFFER_SIZE]="";
	if (g_prjTemplateFiles.GetOrBrowseSlot(_slot, fn, BUFFER_SIZE, _errMsg)) 
	{
		char fn2[BUFFER_SIZE] = "";
		ReaProject* prj = NULL;
		int i=0;
		bool found = false;
		while (!found && (prj = Enum_Projects(i++, fn2, BUFFER_SIZE)))
			if (!strcmp(fn, fn2))
				found = true;

		if (found)
			SelectProjectInstance(prj);
		else
		{
			if (_newTab)
				Main_OnCommand(40859,0);
			Main_openProject(fn);
		}
	}
}

bool autoSaveProjectSlot(int _slot, bool _saveCurPrj, const char* _dirPath, char* _fn, int _fnMaxSize)
{
	bool slotUpdate = false;
	if (_saveCurPrj)
		Main_OnCommand(40026,0);

	char prjFn[BUFFER_SIZE] = "", name[BUFFER_SIZE] = "";
	EnumProjects(-1, prjFn, BUFFER_SIZE);
	ExtractFileNameEx(prjFn, name, true); //JFB!!! Xen code a revoir

	GenerateFilename(_dirPath, name, g_prjTemplateFiles.GetFileExt(), _fn, _fnMaxSize);
	WDL_String chunk;
	if (LoadChunk(prjFn, &chunk))
		slotUpdate |= (SaveChunk(_fn, &chunk) && g_prjTemplateFiles.AddSlot(_fn));
/*JFB insert slot code commented: can mess the user's slot actions (because all following ids change)
		slotUpdate |= (SaveChunk(_fn, &chunk) && g_prjTemplateFiles.InsertSlot(_slot, _fn));
*/
	return slotUpdate;
}

void loadOrSelectProject(COMMAND_T* _ct) {
	int slot = (int)_ct->user;
	if (slot < 0 || slot < g_prjTemplateFiles.GetSize())
		loadOrSelectProject(SNM_CMD_SHORTNAME(_ct), slot, false, slot < 0 || !g_prjTemplateFiles.Get(slot)->IsDefault());
}

void loadNewTabOrSelectProject(COMMAND_T* _ct) {
	int slot = (int)_ct->user;
	if (slot < 0 || slot < g_prjTemplateFiles.GetSize())
		loadOrSelectProject(SNM_CMD_SHORTNAME(_ct), slot, true, slot < 0 || !g_prjTemplateFiles.Get(slot)->IsDefault());
}
