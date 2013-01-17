/******************************************************************************
/ SnM_Project.cpp
/
/ Copyright (c) 2011-2013 Jeffos
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

double GetProjectLength(bool _items, bool _inclRgnsMkrs)
{
	double prjlen = 0.0, pos, end;
	if (_inclRgnsMkrs)
	{
		int x=0; bool isRgn;
		while (x = EnumProjectMarkers2(NULL, x, &isRgn, &pos, &end, NULL, NULL)) {
			if (isRgn) { if (end > prjlen) prjlen = end; }
			else { if (pos > prjlen) prjlen = pos; }
		}
	}
	if (_items)
	{
		double len;
		for (int i=1; i <= GetNumTracks(); i++) // skip master
			if (MediaTrack* tr = CSurf_TrackFromID(i, false))
				for (int j=0; j < GetTrackNumMediaItems(tr); j++)
					if (MediaItem* item = GetTrackMediaItem(tr,j))
					{
						pos = *(double*)GetSetMediaItemInfo(item, "D_POSITION", NULL);
						len = *(double*)GetSetMediaItemInfo(item, "D_LENGTH", NULL);
						if ((pos+len)>prjlen)
							prjlen = pos+len;
					}
	}
	return prjlen;
}

bool InsertSilence(const char* _undoTitle, double _pos, double _len)
{
	if (_pos >=0.0 && _len > 0.0 && _pos <GetProjectLength())
	{
		if (_undoTitle)
			Undo_BeginBlock2(NULL);

		if (PreventUIRefresh)
			PreventUIRefresh(1);

		double timeSel1, timeSel2, d=_pos+_len;
		GetSet_LoopTimeRange2(NULL, false, false, &timeSel1, &timeSel2, false);
		GetSet_LoopTimeRange2(NULL, true, false, &_pos, &d, false);
		Main_OnCommand(40200, 0); // insert space at time sel

		// restore time sel, enlarge it if needed (mimic native behavior with regions)
		if (timeSel1>_pos) timeSel1+=_len;
		if (_pos<timeSel2) timeSel2+=_len;
		GetSet_LoopTimeRange2(NULL, true, false, &timeSel1, &timeSel2, false);

		if (PreventUIRefresh)
			PreventUIRefresh(-1);

		UpdateTimeline(); // ruler + arrange

		if (_undoTitle)
			Undo_EndBlock2(NULL, _undoTitle, UNDO_STATE_ALL);
		return true;
	}
	return false;
}


///////////////////////////////////////////////////////////////////////////////

char g_lastSilenceVal[3][64] = {"10.0", "1.0", "512"};

void InsertSilence(COMMAND_T* _ct)
{
	char val[64]="";
	lstrcpyn(val, g_lastSilenceVal[(int)_ct->user], sizeof(val));

	if (PromptUserForString(GetMainHwnd(), SWS_CMD_SHORTNAME(_ct), val, sizeof(val)) && *val)
	{
		double pos = GetCursorPositionEx(NULL), len = 0.0;
		switch ((int)_ct->user)
		{
			case 0: // s
				len = parse_timestr_len(val, pos, 3);
				break;
			case 1: // meas.beat
/*no! would not take get tempo markers into account since 'val' is not inserted yet
				len = parse_timestr_len(val, pos, 2);
				break;
*/
				if (char* p = strchr(val, '.'))
				{
					*p = '\0';
					int in_meas = atoi(val);
					double in_beats = p[1] ? atof(p+1) : 0.0;

					double bpm; int num, den;
					TimeMap_GetTimeSigAtTime(NULL, pos, &num, &den, &bpm);
					len = in_beats*(60.0/bpm) + in_meas*((240.0*num/den)/bpm);

					*p = '.'; // restore val (used below)
				}
				break;
			case 2: // smp
				len = parse_timestr_len(val, pos, 4);
				break;
		}

		if (len>0.0) {
			lstrcpyn(g_lastSilenceVal[(int)_ct->user], val, sizeof(val));
			InsertSilence(SWS_CMD_SHORTNAME(_ct), pos, len); // includes undo point
		}
		else
			MessageBox(GetMainHwnd(), __LOCALIZE("Invalid input!","sws_mbox"), __LOCALIZE("S&M - Error","sws_mbox"), MB_OK);
	}
}

void OpenProjectPathInExplorerFinder(COMMAND_T*)
{
	char path[SNM_MAX_PATH] = "";
	GetProjectPath(path, sizeof(path));
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

int g_curPrjMidiVal = -1;

class SelectProjectJob : public SNM_MidiActionJob
{
public:
	SelectProjectJob(int _approxDelayMs, int _curval, int _val, int _valhw, int _relmode, HWND _hwnd) 
		: SNM_MidiActionJob(SNM_SCHEDJOB_SEL_PRJ,_approxDelayMs,_curval,_val,_valhw,_relmode,_hwnd) {}
	void Perform() {
		if (ReaProject* proj = Enum_Projects(m_absval, NULL, 0)) // project number is 0-based
			SelectProjectInstance(proj);
	}
};

void SelectProject(MIDI_COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd) {
	if (SelectProjectJob* job = new SelectProjectJob(SNM_SCHEDJOB_DEFAULT_DELAY, g_curPrjMidiVal, _val, _valhw, _relmode, _hwnd))
	{
		g_curPrjMidiVal = job->GetAbsoluteValue();
		AddOrReplaceScheduledJob(job);
	}
}


///////////////////////////////////////////////////////////////////////////////
// Project template slots (Resources view)
///////////////////////////////////////////////////////////////////////////////

void LoadOrSelectProjectSlot(int _slotType, const char* _title, int _slot, bool _newTab)
{
	if (WDL_FastString* fnStr = g_slots.Get(_slotType)->GetOrPromptOrBrowseSlot(_title, &_slot))
	{
		char fn[SNM_MAX_PATH] = "";
		ReaProject* prj = NULL;
		int i=0;
		bool found = false;
		while (!found && (prj = Enum_Projects(i++, fn, sizeof(fn))))
			if (!strcmp(fnStr->Get(), fn))
				found = true;

		if (found)
			SelectProjectInstance(prj);
		else
		{
			if (_newTab)
				Main_OnCommand(40859,0);
			Main_openProject((char*)fnStr->Get());
/* API LIMITATION: would be great to set the project as "not saved" here (like native project templates)
	See http://code.google.com/p/sws-extension/issues/detail?id=321
*/
		}
		delete fnStr;
	}
}

bool AutoSaveProjectSlot(int _slotType, const char* _dirPath, WDL_PtrList<PathSlotItem>* _owSlots, bool _saveCurPrj)
{
	int owIdx = 0;
	if (_saveCurPrj)
		Main_OnCommand(40026,0);
	char prjFn[SNM_MAX_PATH] = "";
	EnumProjects(-1, prjFn, sizeof(prjFn));
	return AutoSaveSlot(_slotType, _dirPath, prjFn, "RPP", _owSlots, &owIdx);
}

void LoadOrSelectProjectSlot(COMMAND_T* _ct) {
	LoadOrSelectProjectSlot(g_tiedSlotActions[SNM_SLOT_PRJ], SWS_CMD_SHORTNAME(_ct), (int)_ct->user, false);
}

void LoadOrSelectProjectTabSlot(COMMAND_T* _ct) {
	LoadOrSelectProjectSlot(g_tiedSlotActions[SNM_SLOT_PRJ], SWS_CMD_SHORTNAME(_ct), (int)_ct->user, true);
}


///////////////////////////////////////////////////////////////////////////////
// Project loader/selecter actions
// note: g_tiedSlotActions[] is ignored here, the opener/selecter only deals 
//       with g_slots.Get(SNM_SLOT_PRJ): that feature is not available for 
//       custom project slots
///////////////////////////////////////////////////////////////////////////////

extern SNM_ResourceWnd* g_pResourcesWnd; // SNM_ResourceView.cpp
int g_prjCurSlot = -1; // 0-based

bool IsProjectLoaderConfValid() {
	return (g_prjLoaderStartPref > 0 && 
		g_prjLoaderEndPref > g_prjLoaderStartPref && 
		g_prjLoaderEndPref <= g_slots.Get(SNM_SLOT_PRJ)->GetSize());
}

void ProjectLoaderConf(COMMAND_T* _ct)
{
	bool ok = false;
	int start, end;
	WDL_FastString question(__LOCALIZE("Start slot (in Resources view):","sws_mbox"));
	question.Append(",");
	question.Append(__LOCALIZE("End slot:","sws_mbox"));

	char reply[64]="";
	_snprintfSafe(reply, sizeof(reply), "%d,%d", g_prjLoaderStartPref, g_prjLoaderEndPref);

	if (GetUserInputs(__LOCALIZE("S&M - Project loader/selecter","sws_mbox"), 2, question.Get(), reply, sizeof(reply)))
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

void LoadOrSelectNextPreviousProject(COMMAND_T* _ct)
{
	// check prefs validity (user configurable..)
	// reminder: 1-based prefs!
	if (IsProjectLoaderConfValid())
	{
		int dir = (int)_ct->user; // -1 (previous) or +1 (next)
		int cpt=0, slotCount = g_prjLoaderEndPref-g_prjLoaderStartPref+1;

		// (try to) find the current project in the slot range defined in the prefs
		if (g_prjCurSlot < 0)
		{
			char pCurPrj[SNM_MAX_PATH] = "";
			EnumProjects(-1, pCurPrj, sizeof(pCurPrj));
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
			LoadOrSelectProjectSlot(SNM_SLOT_PRJ, "", g_prjCurSlot, false);
			if (g_pResourcesWnd) g_pResourcesWnd->SelectBySlot(g_prjCurSlot);
		}
		else g_prjCurSlot = -1;
	}
}


///////////////////////////////////////////////////////////////////////////////
// Project startup action
///////////////////////////////////////////////////////////////////////////////

SWSProjConfig<WDL_FastString> g_prjActions;

void SetProjectStartupAction(COMMAND_T* _ct)
{
	char prjFn[SNM_MAX_PATH] = "";
	EnumProjects(-1, prjFn, sizeof(prjFn));

	WDL_FastString msg;
	msg.SetFormatted(SNM_MAX_PATH, __LOCALIZE("To define a startup action (launched on project load) ","sws_mbox"), prjFn);
	if (*prjFn) msg.AppendFormatted(SNM_MAX_PATH, __LOCALIZE_VERFMT("for %s","sws_mbox"), prjFn);
	msg.Append("\n");
	msg.Append(__LOCALIZE("please select an action, macro, or script in the Actions window and click OK.","sws_mbox"));
	if (g_prjActions.Get()->GetLength())
		if (int cmdId = NamedCommandLookup(g_prjActions.Get()->Get())) {
			msg.Append("\n"); // for translators..
			msg.AppendFormatted(256, __LOCALIZE_VERFMT("Note: this will override the current startup action '%s'","sws_mbox"), kbd_getTextFromCmd(cmdId, NULL));
		}

	ShowActionList(NULL, NULL);
	if (IDOK == MessageBox(GetMainHwnd(), msg.Get(), SWS_CMD_SHORTNAME(_ct), MB_OKCANCEL))
	{
		char idstr[SNM_MAX_ACTION_CUSTID_LEN] = "";
		if (GetSelectedAction(idstr, SNM_MAX_ACTION_CUSTID_LEN, __localizeFunc("Main","accel_sec",0)))
		{
			g_prjActions.Get()->Set(idstr);
			Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_MISCCFG, -1);

			msg.SetFormatted(SNM_MAX_PATH, __LOCALIZE_VERFMT("'%s' is defined as startup action ","sws_mbox"), kbd_getTextFromCmd(NamedCommandLookup(idstr), NULL));
			if (*prjFn) msg.AppendFormatted(SNM_MAX_PATH, __LOCALIZE_VERFMT("for %s","sws_mbox"), prjFn);
			MessageBox(GetMainHwnd(), msg.Get(), SWS_CMD_SHORTNAME(_ct), MB_OK);
		}
	}
}

void ClearProjectStartupAction(COMMAND_T* _ct)
{
	if (g_prjActions.Get()->GetLength())
		if (int cmdId = NamedCommandLookup(g_prjActions.Get()->Get()))
		{
			WDL_FastString msg;
			msg.AppendFormatted(256, __LOCALIZE_VERFMT("Are you sure you want to clear the current startup action '%s'?","sws_mbox"), kbd_getTextFromCmd(cmdId, NULL));
			if (IDOK == MessageBox(GetMainHwnd(), msg.Get(), SWS_CMD_SHORTNAME(_ct), MB_OKCANCEL)) {
				g_prjActions.Get()->Set("");
				Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_MISCCFG, -1);
			}
		}
}

static bool ProcessExtensionLine(const char *line, ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	LineParser lp(false);
	if (lp.parse(line) || lp.getnumtokens() < 1)
		return false;

	if (!strcmp(lp.gettoken_str(0), "S&M_PROJACTION"))
	{
		g_prjActions.Get()->Set(lp.gettoken_str(1));
		if (!isUndo)
			if (int cmdId = NamedCommandLookup(lp.gettoken_str(1)))
			{
//				AddOrReplaceScheduledJob(new ProjectActionJob(cmdId)); // ~1s delay
				Main_OnCommand(cmdId, 0);
			}
		return true;
	}
	return false;
}

static void SaveExtensionConfig(ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	char line[SNM_MAX_CHUNK_LINE_LENGTH] = "";
	if (ctx && g_prjActions.Get()->GetLength())
		if (_snprintfStrict(line, sizeof(line), "S&M_PROJACTION %s", g_prjActions.Get()->Get()) > 0)
			ctx->AddLine("%s", line);
}

static void BeginLoadProjectState(bool isUndo, struct project_config_extension_t *reg) {
	g_prjActions.Cleanup();
	g_prjActions.Get()->Set("");
}

static project_config_extension_t g_projectconfig = {
	ProcessExtensionLine, SaveExtensionConfig, BeginLoadProjectState, NULL
};

int ReaProjectInit() {
	return plugin_register("projectconfig", &g_projectconfig);
}
