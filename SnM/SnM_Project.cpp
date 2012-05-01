/******************************************************************************
/ SnM_Project.cpp
/
/ Copyright (c) 2011-2012 Jeffos
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
#include "SnM.h"
#include "../Prompt.h"
#include "../reaper/localize.h"


///////////////////////////////////////////////////////////////////////////////
// General project helpers
///////////////////////////////////////////////////////////////////////////////

// note: works with time sig markers too
double GetProjectLength()
{
	double prjlen = 0.0;
	for (int i=1; i <= GetNumTracks(); i++) // skip master
		if (MediaTrack* tr = CSurf_TrackFromID(i, false))
			for (int j=0; j < GetTrackNumMediaItems(tr); j++)
				if (MediaItem* item = GetTrackMediaItem(tr,j))
				{
					double pos = *(double*)GetSetMediaItemInfo(item, "D_POSITION", NULL);
					double len = *(double*)GetSetMediaItemInfo(item, "D_LENGTH", NULL);
					if ((pos+len)>prjlen)
						prjlen = pos+len;
				}
	return prjlen;
}

// note: callers must surround this func with Undo_BeginBlock/Undo_EndBlock
bool InsertSilence(const char* _undoTitle, double _pos, double _len)
{
	if (_pos >=0.0 && _len > 0.0 && _pos <GetProjectLength())
	{
		if (_undoTitle)
			Undo_BeginBlock2(NULL);

		double timeSel1, timeSel2, d=_pos+_len;
		GetSet_LoopTimeRange2(NULL, false, false, &timeSel1, &timeSel2, false);
		GetSet_LoopTimeRange2(NULL, true, false, &_pos, &d, false);
		Main_OnCommand(40200, 0); // insert space at time sel

		// restore time sel, enlarge it if needed (mimic native behavior with regions)
		if (timeSel1>_pos) timeSel1+=_len;
		if (_pos<timeSel2) timeSel2+=_len;
		GetSet_LoopTimeRange2(NULL, true, false, &timeSel1, &timeSel2, false);

		if (_undoTitle)
			Undo_EndBlock2(NULL, _undoTitle, UNDO_STATE_ALL);
		return true;
	}
	return false;
}


///////////////////////////////////////////////////////////////////////////////

void InsertSilence(COMMAND_T* _ct)
{
	char val[64]=""; int modeoverride;
	switch ((int)_ct->user)
	{
		case 0: // s
			modeoverride = 3;
			lstrcpyn(val, "10.0", 64);
			break;
		case 1: // meas.beat
			modeoverride = 2;
			lstrcpyn(val, "1.0", 64);
			break;
		case 2: // smp
			modeoverride = 4;
			lstrcpyn(val, "512", 64);
			break;
	}
	if (PromptUserForString(GetMainHwnd(), SWS_CMD_SHORTNAME(_ct), val, 64) && *val) {
		double t = parse_timestr_len(val, 0.0, modeoverride);
		if (t>0.0)
			InsertSilence(SWS_CMD_SHORTNAME(_ct), GetCursorPositionEx(NULL), t); // includes undo point
		else
			MessageBox(GetMainHwnd(), __LOCALIZE("Invalid input!","sws_mbox"), __LOCALIZE("S&M - Error","sws_mbox"), MB_OK);
	}
}

void OpenProjectPathInExplorerFinder(COMMAND_T*)
{
	char path[BUFFER_SIZE] = "";
	GetProjectPath(path, BUFFER_SIZE);
	if (*path)
	{
		if (char* p = strrchr(path, PATH_SLASH_CHAR)) {
			*(p+1) = '\0'; // ShellExecute() is KO otherwise..
			ShellExecute(NULL, "open", path, NULL, NULL, SW_SHOWNORMAL);
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// Select project ("MIDI CC absolute only" actions)
///////////////////////////////////////////////////////////////////////////////

class SNM_SelectProjectScheduledJob : public SNM_ScheduledJob
{
public:
	SNM_SelectProjectScheduledJob(int _approxDelayMs, int _val, int _valhw, int _relmode, HWND _hwnd) 
		: SNM_ScheduledJob(SNM_SCHEDJOB_SEL_PRJ, _approxDelayMs),m_val(_val),m_valhw(_valhw),m_relmode(_relmode),m_hwnd(_hwnd) {}
	void Perform() {
		if (ReaProject* proj = Enum_Projects(m_val, NULL, 0))
			SelectProjectInstance(proj);
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
// Project template slots (Resources view)
///////////////////////////////////////////////////////////////////////////////

void loadOrSelectProjectSlot(int _slotType, const char* _title, int _slot, bool _newTab)
{
	if (WDL_FastString* fnStr = g_slots.Get(_slotType)->GetOrPromptOrBrowseSlot(_title, _slot))
	{
		char fn2[BUFFER_SIZE] = "";
		ReaProject* prj = NULL;
		int i=0;
		bool found = false;
		while (!found && (prj = Enum_Projects(i++, fn2, BUFFER_SIZE)))
			if (!strcmp(fnStr->Get(), fn2))
				found = true;

		if (found)
			SelectProjectInstance(prj);
		else
		{
			if (_newTab) Main_OnCommand(40859,0);
			Main_openProject((char*)fnStr->Get());
/* API LIMITATION: would be great to set the project as "not saved" here (like native project templates)
	See http://code.google.com/p/sws-extension/issues/detail?id=321
*/
		}
		delete fnStr;
	}
}

bool autoSaveProjectSlot(int _slotType, const char* _dirPath, char* _fn, int _fnSize, bool _saveCurPrj)
{
	if (_saveCurPrj) Main_OnCommand(40026,0);
	char prjFn[BUFFER_SIZE] = "", name[256] = "";
	EnumProjects(-1, prjFn, BUFFER_SIZE);
	GetFilenameNoExt(prjFn, name, 256);
	GenerateFilename(_dirPath, name, g_slots.Get(_slotType)->GetFileExt(), _fn, _fnSize);
	return (SNM_CopyFile(_fn, prjFn) && g_slots.Get(_slotType)->AddSlot(_fn));
}

void loadOrSelectProjectSlot(COMMAND_T* _ct) {
	loadOrSelectProjectSlot(g_tiedSlotActions[SNM_SLOT_PRJ], SWS_CMD_SHORTNAME(_ct), (int)_ct->user, false);
}

void loadOrSelectProjectTabSlot(COMMAND_T* _ct) {
	loadOrSelectProjectSlot(g_tiedSlotActions[SNM_SLOT_PRJ], SWS_CMD_SHORTNAME(_ct), (int)_ct->user, true);
}


///////////////////////////////////////////////////////////////////////////////
// Project loader/selecter actions
// note: g_tiedSlotActions[] is ignored here, the opener/selecter only deals 
//       with g_slots.Get(SNM_SLOT_PRJ): that feature is not available for 
//       custom project slots
///////////////////////////////////////////////////////////////////////////////

extern SNM_ResourceWnd* g_pResourcesWnd; // SNM_ResourceView.cpp
int g_prjCurSlot = -1; // 0-based

bool isProjectLoaderConfValid() {
	return (g_prjLoaderStartPref > 0 && 
		g_prjLoaderEndPref > g_prjLoaderStartPref && 
		g_prjLoaderEndPref <= g_slots.Get(SNM_SLOT_PRJ)->GetSize());
}

void projectLoaderConf(COMMAND_T* _ct)
{
	bool ok = false;
	int start, end;
	WDL_FastString question(__LOCALIZE("Start slot (in Resources view):","sws_mbox"));
	question.Append(",");
	question.Append(__LOCALIZE("End slot:","sws_mbox"));

	char reply[BUFFER_SIZE]="";
	_snprintf(reply, BUFFER_SIZE, "%d,%d", g_prjLoaderStartPref, g_prjLoaderEndPref);

	if (GetUserInputs(__LOCALIZE("S&M - Project loader/selecter","sws_mbox"), 2, question.Get(), reply, 4096))
	{
		if (*reply && *reply != ',' && strlen(reply) > 2)
		{
			if (char* p = strchr(reply, ','))
			{
				start = atoi(reply);
				end = atoi((char*)(p+1));
				if (start > 0 && end > start && end <= g_slots.Get(SNM_SLOT_PRJ)->GetSize())
				{
					g_prjLoaderStartPref = start;
					g_prjLoaderEndPref = end;
					ok = true;
				}
			}
		}

		if (ok)
		{
			g_prjCurSlot = -1; // reset current prj
			if (g_pResourcesWnd)
				g_pResourcesWnd->Update();
		}
		else
			MessageBox(GetMainHwnd(), __LOCALIZE("Invalid start and/or end slot(s) !\nProbable cause: out of bounds, the Resources view is empty, etc...","sws_mbox"), __LOCALIZE("S&M - Error","sws_mbox"), MB_OK);
	}
}

void loadOrSelectNextPreviousProject(COMMAND_T* _ct)
{
	// check prefs validity (user configurable..)
	// reminder: 1-based prefs!
	if (isProjectLoaderConfValid())
	{
		int dir = (int)_ct->user; // -1 (previous) or +1 (next)
		int cpt=0, slotCount = g_prjLoaderEndPref-g_prjLoaderStartPref+1;

		// (try to) find the current project in the slot range defined in the prefs
		if (g_prjCurSlot < 0)
		{
			char pCurPrj[BUFFER_SIZE] = "";
			EnumProjects(-1, pCurPrj, BUFFER_SIZE);
			if (pCurPrj && *pCurPrj)
				for (int i=g_prjLoaderStartPref-1; g_prjCurSlot < 0 && i < g_prjLoaderEndPref-1; i++)
					if (!g_slots.Get(SNM_SLOT_PRJ)->Get(i)->IsDefault() && strstr(pCurPrj, g_slots.Get(SNM_SLOT_PRJ)->Get(i)->m_shortPath.Get()))
						g_prjCurSlot = i;
		}

		if (g_prjCurSlot < 0) // not found => default init
			g_prjCurSlot = (dir > 0 ? g_prjLoaderStartPref-2 : g_prjLoaderEndPref);

		// the meat
		do
		{
			if ((dir > 0 && (g_prjCurSlot+dir) > (g_prjLoaderEndPref-1)) ||	
				(dir < 0 && (g_prjCurSlot+dir) < (g_prjLoaderStartPref-1)))
			{
				g_prjCurSlot = (dir > 0 ? g_prjLoaderStartPref-1 : g_prjLoaderEndPref-1);
			}
			else g_prjCurSlot += dir;
		}
		while (++cpt <= slotCount && g_slots.Get(SNM_SLOT_PRJ)->Get(g_prjCurSlot)->IsDefault());

		// found one?
		if (cpt <= slotCount) {
			loadOrSelectProjectSlot(SNM_SLOT_PRJ, "", g_prjCurSlot, false);
			if (g_pResourcesWnd) g_pResourcesWnd->SelectBySlot(g_prjCurSlot);
		}
		else g_prjCurSlot = -1;
	}
}

