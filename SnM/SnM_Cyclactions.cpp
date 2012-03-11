/******************************************************************************
/ SnM_Cyclactions.cpp
/
/ Copyright (c) 2011 Jeffos, Tim Payne (SWS)
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

//JFB TODO?
// - storage: use native macro format?
//   (cycle actions have their own format for "historical" reasons)

#include "stdafx.h"
#include "SnM_Actions.h"
#include "SnM_Cyclactions.h"
#include "../reaper/localize.h"
//#include "../../WDL/projectcontext.h"

//#define _SNM_MFC


///////////////////////////////////////////////////////////////////////////////
// Core stuff
///////////////////////////////////////////////////////////////////////////////

// [0] = main section action, [1] = ME event list section action, [2] = ME piano roll section action
WDL_PtrList_DeleteOnDestroy<Cyclaction> g_cyclactions[SNM_MAX_CYCLING_SECTIONS];
char g_cyclactionCustomIds[SNM_MAX_CYCLING_SECTIONS][SNM_MAX_ACTION_CUSTID_LEN] = {"S&M_CYCLACTION_", "S&M_ME_LIST_CYCLACTION", "S&M_ME_PIANO_CYCLACTION"};
char g_cyclactionIniSections[SNM_MAX_CYCLING_SECTIONS][64] = {"Main_Cyclactions", "ME_List_Cyclactions", "ME_Piano_Cyclactions"};
char g_cyclactionSections[SNM_MAX_CYCLING_SECTIONS][SNM_MAX_SECTION_NAME_LEN]; // init & localization in CyclactionInit()

static SNM_CyclactionWnd* g_pCyclactionWnd = NULL;
bool g_undos = true;

// for subbtle "recursive cycle action" cases
// (e.g. a cycle action that calls a macro that calls a cycle action..)
static bool g_bReentrancyCheck = false;

class ScheduledActions : public SNM_ScheduledJob
{
public:
	ScheduledActions(int _approxDelayMs, int _section, int _cycleId, const char* _name, WDL_PtrList<WDL_FastString>* _actions) 
		: SNM_ScheduledJob(SNM_SCHEDJOB_CYCLACTION, _approxDelayMs), m_section(_section), m_cycleId(_cycleId), m_name(_name)
	{
		for (int i= 0; _actions && i < _actions->GetSize(); i++)
			m_actions.Add(new WDL_FastString(_actions->Get(i)->Get()));
	}	
	// best effort: ingore unknown actions but goes on..
	void Perform()
	{
		g_bReentrancyCheck = true;

		if (g_undos)
			Undo_BeginBlock2(NULL);

		for (int i=0; i < m_actions.GetSize(); i++)
		{
			if (int cmd = NamedCommandLookup(m_actions.Get(i)->Get()))
			{
				if (!m_section && !KBD_OnMainActionEx(cmd, 0, 0, 0, GetMainHwnd(), NULL)) // Main section
					break;
				else if (m_section && !MIDIEditor_LastFocused_OnCommand(cmd, m_section == 1)) // Both ME sections
					break;
			}
		}
		// refresh toolbar button
		{
			char custCmdId[SNM_MAX_ACTION_CUSTID_LEN] = "";
			if (_snprintf(custCmdId, SNM_MAX_ACTION_CUSTID_LEN, "_%s%d", g_cyclactionCustomIds[m_section], m_cycleId) > 0)
				RefreshToolbar(NamedCommandLookup(custCmdId));
		}

		if (g_undos)
			Undo_EndBlock2(NULL, m_name.Get(), UNDO_STATE_ALL);

		m_actions.Empty(true);
		g_bReentrancyCheck = false;
	}
	int m_section, m_cycleId;
	WDL_FastString m_name;
	WDL_PtrList_DeleteOnDestroy<WDL_FastString> m_actions;
};

void RunCycleAction(int _section, COMMAND_T* _ct)
{
	int cycleId = (int)_ct->user;
	Cyclaction* action = g_cyclactions[_section].Get(cycleId-1); // cycle action id is 1-based (for user display) !
	if (!action)
		return;

	// process new actions until cycle point
	WDL_FastString name(action->GetName());
	int state=0, startIdx=0;
	while (startIdx < action->GetCmdSize() && state < action->m_performState) {
		const char* cmd = action->GetCmd(startIdx++);
		if (cmd[0] == '!') {
			state++;
			if (cmd[1] && state == action->m_performState)
				name.Set((const char *)(cmd+1));
		}
	}

	bool hasCustomIds = false;
	WDL_PtrList_DeleteOnDestroy<WDL_FastString> actions;
	for (int i=startIdx; i < action->GetCmdSize(); i++)
	{
		const char* cmd = action->GetCmd(i);
		char buf[256] = ""; 
		bool done = false;
		if (i == (action->GetCmdSize()-1))
		{
			action->m_performState = 0;
			if (strcmp(action->GetName(), _ct->accel.desc)) lstrcpyn(buf, action->GetName(), 256);
			done = true;
		}
		else if (*cmd == '!') // last ! ignored
		{
			action->m_performState++;
			if (cmd[1])	lstrcpyn(buf, (char *)(cmd+1), 256);
			done = true;
		}

		// add actions (checks done at perform time, i.e. best effort)
		if (*cmd != '!') {
			actions.Add(new WDL_FastString(cmd));
			if (!hasCustomIds && !atoi(cmd)) // macro, extension !?
				hasCustomIds = true;
		}

		// dynamic action renaming if "!" followed by some text (or cycling back to 1st action)
		if (*buf)
		{
			int cmdId = _ct->accel.accel.cmd;
			SWSFreeUnregisterDynamicCmd(cmdId);
			RegisterCyclation(buf, action->IsToggle(), _section, cycleId, cmdId);
		}
		if (done)
			break;
	}

	if (actions.GetSize() && !g_bReentrancyCheck)
	{
		// note: I "skip whilst respecting" (!) the SWS reentrance test - see hookCommandProc() in sws_entension.cpp -
		// thanks to scheduled actions that are performed a 50 ms later (or so)
		// we include macros too as they can contain SWS stuff..
		ScheduledActions* job = new ScheduledActions(50, _section, cycleId, name.Get(), &actions);
		if (hasCustomIds)
			AddOrReplaceScheduledJob(job);
		// perform immedialtely
		else {
			job->Perform();
			delete job;
		}
	}
}

void RunMainCyclaction(COMMAND_T* _ct) {RunCycleAction(0, _ct);}
void RunMEListCyclaction(COMMAND_T* _ct) {if (g_bv4) RunCycleAction(1, _ct);}
void RunMEPianoCyclaction(COMMAND_T* _ct) {if (g_bv4) RunCycleAction(2, _ct);}

bool IsCyclactionEnabled(int _type, COMMAND_T* _ct) {
	int cycleId = (int)_ct->user;
	Cyclaction* action = g_cyclactions[_type].Get(cycleId-1); // cycle action id is 1-based (for user display)
	return (action && /*action->IsToggle() &&*/ (action->m_performState % 2) != 0);
}

bool IsMainCyclactionEnabled(COMMAND_T* _ct) {return IsCyclactionEnabled(0, _ct);}
bool IsMEListCyclactionEnabled(COMMAND_T* _ct) {return IsCyclactionEnabled(1, _ct);}
bool IsMEPianoCyclactionEnabled(COMMAND_T* _ct) {return IsCyclactionEnabled(2, _ct);}

bool CheckEditableCyclaction(const char* _actionStr, WDL_FastString* _errMsg, bool _allowEmpty = true)
{
	if (!(_actionStr && *_actionStr && *_actionStr != ',' && _strnicmp(_actionStr, "#,", 2))) {
		if (_errMsg) {
			_errMsg->AppendFormatted(256, __LOCALIZE_VERFMT("Error: invalid cycle action '%s'","sws_DLG_161"), _actionStr ? _actionStr : "NULL");
			_errMsg->Append("\n\n");
		}
		return false;
	}
	if (!_allowEmpty && !strcmp(EMPTY_CYCLACTION, _actionStr))
		return false; // no msg: empty cycle actions are internal stuff
	return true;
}

bool CheckRegisterableCyclaction(int _section, Cyclaction* _a, WDL_FastString* _errMsg, bool _checkCmdIds)
{
	if (_a)
	{
		int steps=0, noop=0;
		for (int i=0; i < _a->GetCmdSize(); i++)
		{
			const char* cmd = _a->GetCmd(i);
			if (strstr(cmd, "_CYCLACTION")) {
				if (_errMsg) {
					_errMsg->AppendFormatted(256, __LOCALIZE_VERFMT("Warning: cycle action '%s' (section '%s') was added but not registered","sws_DLG_161"), _a->GetName(), g_cyclactionSections[_section]);
					_errMsg->Append("\n");
					_errMsg->Append(__LOCALIZE("Details: recursive cycle action (i.e. uses another cycle action)","sws_DLG_161"));
					_errMsg->Append("\n\n");
				}
				return false;
			}
			else if (*cmd == '!')
				steps++;
			else if (!strcmp(cmd, "65535"))
				noop++;
			else if (!_section) // main section?
			{
				if (atoi(cmd) >= g_iFirstCommand) {
					if (_errMsg) {
						_errMsg->AppendFormatted(256, __LOCALIZE_VERFMT("Warning: cycle action '%s' (section '%s') was added but not registered","sws_DLG_161"), _a->GetName(), g_cyclactionSections[_section]);
						_errMsg->Append("\n");
						_errMsg->Append(__LOCALIZE("Details: for extensions' actions, you must use custom ids (e.g. _SWS_ABOUT), not command ids (e.g. 47145)","sws_DLG_161"));
						_errMsg->Append("\n\n");
					}
					return false;
				}
				// API LIMITATION: NamedCommandLookup() KO in other sections than the main one
				// => all cyclactions belong to the main section although they can target other sections..
				if(_checkCmdIds && !NamedCommandLookup(cmd)) {
					if (_errMsg) {
						_errMsg->AppendFormatted(256, __LOCALIZE_VERFMT("Warning: cycle action '%s' (section '%s') was added but not registered","sws_DLG_161"), _a->GetName(), g_cyclactionSections[_section]);
						_errMsg->Append("\n");
						_errMsg->AppendFormatted(256, __LOCALIZE_VERFMT("Details: command id (or custom id) '%s' not found","sws_DLG_161"), cmd);
						_errMsg->Append("\n\n");
					}
					return false;
				}
			}
		}
		if ((steps + noop) == _a->GetCmdSize()) {
			if (_errMsg && !_a->IsEmpty()) {
				_errMsg->AppendFormatted(256, __LOCALIZE_VERFMT("Warning: cycle action '%s' (section '%s') was added but not registered","sws_DLG_161"), _a->GetName(), g_cyclactionSections[_section]);
				_errMsg->Append("\n");
				_errMsg->Append(__LOCALIZE("Details: no valid command id (or custom id) found","sws_DLG_161"));
				_errMsg->Append("\n\n");
			}
			return false;
		}
	}
	else {
		if (_errMsg) {
			_errMsg->AppendFormatted(256, __LOCALIZE_VERFMT("Error: invalid cycle action in section '%s'","sws_DLG_161"), g_cyclactionSections[_section]);
			_errMsg->Append("\n\n");
		}
		return false;
	}
	return true;
}

// return true if cyclaction added (but not necesary registered)
bool CreateCyclaction(int _section, const char* _actionStr, WDL_FastString* _errMsg, bool _checkCmdIds)
{
	if (CheckEditableCyclaction(_actionStr, _errMsg))
	{
		Cyclaction* a = new Cyclaction(_actionStr);
		g_cyclactions[_section].Add(a);
		int cycleId = g_cyclactions[_section].GetSize();
		if (CheckRegisterableCyclaction(_section, a, _errMsg, _checkCmdIds))
			RegisterCyclation(a->GetName(), a->IsToggle(), _section, cycleId, 0);
		return true;
	}
	return false;
}

// _cmdId: id to re-use or 0 to ask for a new cmd id
// returns the cmd id, or 0 if failed
// note: we don't use Cyclaction* as prm 'cause of dynamic action renameing
int RegisterCyclation(const char* _name, bool _toggle, int _section, int _cycleId, int _cmdId)
{
	if (!SWSGetCommandID(!_section ? RunMainCyclaction : _section == 1 ? RunMEListCyclaction : RunMEPianoCyclaction, _cycleId))
	{
		char cID[SNM_MAX_ACTION_CUSTID_LEN]="";
		if (_snprintf(cID, SNM_MAX_ACTION_CUSTID_LEN, "%s%d", g_cyclactionCustomIds[_section], _cycleId) > 0)
			return SWSCreateRegisterDynamicCmd(_cmdId,
				!_section ? RunMainCyclaction : _section == 1 ? RunMEListCyclaction : RunMEPianoCyclaction, 
				!_toggle ? NULL : (!_section ? IsMainCyclactionEnabled : _section == 1 ? IsMEListCyclactionEnabled : IsMEPianoCyclactionEnabled),
				cID, _name, _cycleId, __FILE__, false); // no localization for cycle actions (name defined by the user) !
	}
	return 0;
}

void FlushCyclactions(int _section)
{
	for (int i=0; i < g_cyclactions[_section].GetSize(); i++)
	{
		char custCmdId[SNM_MAX_ACTION_CUSTID_LEN] = "";
		_snprintf(custCmdId, SNM_MAX_ACTION_CUSTID_LEN, "_%s%d", g_cyclactionCustomIds[_section], i+1);		
		SWSFreeUnregisterDynamicCmd(NamedCommandLookup(custCmdId));
	}
	g_cyclactions[_section].EmptySafe(true);
}

// _cyclactions: NULL adds/register to main model, otherwise just imports into _cyclactions
// _section or -1 for all sections
// _iniFn: NULL => S&M.ini
// _checkCmdIds: false to skip some checks - usefull when loading cycle actions (but not when creating 
//               them with the editor) because all other referenced actions may not have been registered yet..
// remark: undo pref ignored, only loads cycle actions so that the user keeps it's own undo pref
void LoadCyclactions(bool _errMsg, bool _checkCmdIds, WDL_PtrList_DeleteOnDestroy<Cyclaction>* _cyclactions = NULL, int _section = -1, const char* _iniFn = NULL)
{
	char buf[32] = "";
	char actionBuf[MAX_CYCLATION_LEN] = "";
	WDL_FastString msg;
	for (int sec=0; sec < SNM_MAX_CYCLING_SECTIONS; sec++)
	{
		if (_section == sec || _section == -1)
		{
			if (!_cyclactions)
				FlushCyclactions(sec);

			GetPrivateProfileString(g_cyclactionIniSections[sec], "Nb_Actions", "0", buf, 32, _iniFn ? _iniFn : g_SNMCyclactionIniFn.Get()); 
			int nb = atoi(buf);
			for (int j=0; j < nb; j++) 
			{
				_snprintf(buf, 32, "Action%d", j+1);
				GetPrivateProfileString(g_cyclactionIniSections[sec], buf, EMPTY_CYCLACTION, actionBuf, MAX_CYCLATION_LEN, _iniFn ? _iniFn : g_SNMCyclactionIniFn.Get());

				// import into _cyclactions
				if (_cyclactions)
				{
					if (CheckEditableCyclaction(actionBuf, _errMsg ? &msg : NULL, false))
						_cyclactions[sec].Add(new Cyclaction(actionBuf, true));
				}
				// main model update + action register
				else if (!CreateCyclaction(sec, actionBuf, _errMsg ? &msg : NULL, _checkCmdIds))
					CreateCyclaction(sec, EMPTY_CYCLACTION, NULL, false);  // +no-op in order to preserve cycle actions' ids
			}
		}
	}

	if (_errMsg && msg.GetLength())
		SNM_ShowMsg(msg.Get(), __LOCALIZE("S&M - Warning","sws_DLG_161"), g_pCyclactionWnd?g_pCyclactionWnd->GetHWND():GetMainHwnd());
}

// NULL _cyclactions => update main model
//_section or -1 for all sections
// NULL _iniFn => S&M.ini
// remark: undo pref ignored, only saves cycle actions
void SaveCyclactions(WDL_PtrList_DeleteOnDestroy<Cyclaction>* _cyclactions = NULL, int _section = -1, const char* _iniFn = NULL)
{
	if (!_cyclactions)
		_cyclactions = g_cyclactions;

	for (int sec=0; sec < SNM_MAX_CYCLING_SECTIONS; sec++)
	{
		if (_section == sec || _section == -1)
		{
			WDL_PtrList_DeleteOnDestroy<int> freeCycleIds;
			WDL_FastString iniSection("; Do not tweak by hand! Use the Cycle Action editor instead\n"), escapedStr; // no localization for ini files

			// prepare "compression" (i.e. will re-assign ids of new actions for the next load)
			for (int j=0; j < _cyclactions[sec].GetSize(); j++)
				if (_cyclactions[sec].Get(j)->IsEmpty())
					freeCycleIds.Add(new int(j));

			int maxId = 0;
			for (int j=0; j < _cyclactions[sec].GetSize(); j++)
			{
				Cyclaction* a = _cyclactions[sec].Get(j);
				if (!_cyclactions[sec].Get(j)->IsEmpty()) // skip empty cyclactions
				{
					if (!a->m_added)
					{
						makeEscapedConfigString(a->m_desc.Get(), &escapedStr);
						iniSection.AppendFormatted(MAX_CYCLATION_LEN+16, "Action%d=%s\n", j+1, escapedStr.Get()); 
						maxId = max(j+1, maxId);
					}
					else 
					{
						a->m_added = false;
						if (freeCycleIds.GetSize())
						{
							makeEscapedConfigString(a->m_desc.Get(), &escapedStr);
							int id = *(freeCycleIds.Get(0));
							iniSection.AppendFormatted(MAX_CYCLATION_LEN+16, "Action%d=%s\n", id+1, escapedStr.Get()); 
							freeCycleIds.Delete(0, true);
							maxId = max(id+1, maxId);
						}
						else
						{
							makeEscapedConfigString(a->m_desc.Get(), &escapedStr);
							iniSection.AppendFormatted(MAX_CYCLATION_LEN+16, "Action%d=%s\n", ++maxId, escapedStr.Get()); 
						}
					}
				}
			}
			// "Nb_Actions" is a bad name now: it is a max id (kept for ascendant comp.)
			iniSection.AppendFormatted(32, "Nb_Actions=%d\n", maxId);
			SaveIniSection(g_cyclactionIniSections[sec], &iniSection, _iniFn ? _iniFn : g_SNMCyclactionIniFn.Get());
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// Cyclaction
///////////////////////////////////////////////////////////////////////////////

const char* Cyclaction::GetStepName(int _performState) {
	int performState = (_performState < 0 ? m_performState : _performState);
	if (performState) {
		int state=1; // because 0 is name
		for (int i=0; i < GetCmdSize(); i++) {
			const char* cmd = GetCmd(i);
			if (*cmd == '!') {
				if (performState == state) {
					if (cmd[1]) return (const char*)(cmd+1);
					else break;
				}
				state++;
			}
		}
	}
	return GetName();
}

void Cyclaction::SetToggle(bool _toggle) {
	if (!IsToggle()) {
		char* s = _strdup(m_desc.Get());
		m_desc.SetFormatted(MAX_CYCLATION_LEN, "#%s", s); 
		free(s);
	}
	else
		m_desc.DeleteSub(0,1);
	// no need for deeper updates (cmds, etc.. )
}

void Cyclaction::SetCmd(WDL_FastString* _cmd, const char* _newCmd) {
	int i = m_cmds.Find(_cmd);
	if (_cmd && _newCmd && i >= 0) {
		m_cmds.Get(i)->Set(_newCmd);
		UpdateFromCmd();
	}
}

WDL_FastString* Cyclaction::AddCmd(const char* _cmd) {
	WDL_FastString* c = new WDL_FastString(_cmd); 
	m_cmds.Add(c); 
	UpdateFromCmd(); 
	return c;
}

void Cyclaction::UpdateNameAndCmds()
{
	m_cmds.EmptySafe(false); // no delete (pointers may still be used in a ListView: to be deleted by callers)

	char actionStr[MAX_CYCLATION_LEN] = "";
	lstrcpyn(actionStr, m_desc.Get(), MAX_CYCLATION_LEN);
	char* tok = strtok(actionStr, ",");
	if (tok) {
		// "#name" = toggle action, "name" = normal action
		m_name.Set(*tok == '#' ? (const char*)tok+1 : tok);
		while (tok = strtok(NULL, ","))
			m_cmds.Add(new WDL_FastString(tok));
	}
	m_empty = (strcmp(EMPTY_CYCLACTION, m_desc.Get()) == 0);
}

void Cyclaction::UpdateFromCmd()
{
	WDL_FastString newDesc(IsToggle() ? "#" : "");
	newDesc.Append(GetName());
	newDesc.Append(",");
	for (int i = 0; i < m_cmds.GetSize(); i++) {
		newDesc.Append(m_cmds.Get(i)->Get());
		newDesc.Append(",");
	}
	m_desc.Set(&newDesc);
	m_empty = (strcmp(EMPTY_CYCLACTION, m_desc.Get()) == 0);
}


///////////////////////////////////////////////////////////////////////////////
// GUI
///////////////////////////////////////////////////////////////////////////////

#define CYCLACTION_STATE_KEY	"CyclactionsState"
#define CYCLACTIONWND_POS_KEY	"CyclactionsWndPos"

// !WANT_LOCALIZE_STRINGS_BEGIN:sws_DLG_161
static SWS_LVColumn g_cyclactionsCols[] = { { 50, 0, "Id" }, { 260, 1, "Cycle action name" }, { 50, 2, "Toggle" } };
static SWS_LVColumn g_commandsCols[] = { { 180, 1, "Command" }, { 180, 0, "Name (main section only)" } };
// !WANT_LOCALIZE_STRINGS_END

// we need these because of cross-calls between both list views
static SNM_CyclactionsView* g_lvL = NULL;
static SNM_CommandsView* g_lvR = NULL;

// fake list views' contents
// !WANT_LOCALIZE_STRINGS_BEGIN:sws_DLG_161
static Cyclaction g_DEFAULT_L("Right click here to add cycle actions");
static WDL_FastString g_EMPTY_R("<- Select a cycle action");
static WDL_FastString g_DEFAULT_R("Right click here to add commands");
// !WANT_LOCALIZE_STRINGS_END

WDL_PtrList_DeleteOnDestroy<Cyclaction> g_editedActions[SNM_MAX_CYCLING_SECTIONS];
Cyclaction* g_editedAction = NULL;
int g_editedSection = 0; // main section action, 1 = ME event list section action, 2 = ME piano roll section action
bool g_edited = false;
char g_lastExportFn[BUFFER_SIZE] = "";
char g_lastImportFn[BUFFER_SIZE] = "";


int CountEditedActions() {
	int count = 0;
	for (int i=0; i < g_editedActions[g_editedSection].GetSize(); i++)
		if(!g_editedActions[g_editedSection].Get(i)->IsEmpty())
			count++;
	return count;
}

void UpdateEditedStatus(bool _edited) {
	g_edited = _edited;
#ifdef _SNM_MFC
	if (g_pCyclactionWnd)
		EnableWindow(GetDlgItem(g_pCyclactionWnd->GetHWND(), IDC_APPLY), g_edited);
#else
	if (g_pCyclactionWnd)
		g_pCyclactionWnd->Update(false);
#endif
}

void UpdateListViews() {
	if (g_lvL) g_lvL->Update();
	if (g_lvR) g_lvR->Update();
}

void AllEditListItemEnd(bool _save) {
	bool edited = g_edited;
	if (g_lvL && g_lvL->EditListItemEnd(_save))
		edited = true;
	if (g_lvR && g_lvR->EditListItemEnd(_save))
		edited = true;
	UpdateEditedStatus(edited);// just to make sure..
}

void EditModelInit()
{
	// keep pointers (may be used in a listview: delete after listview update)
	WDL_PtrList_DeleteOnDestroy<Cyclaction> actionsToDelete;
	for (int i=0; i < g_editedActions[g_editedSection].GetSize(); i++)
		actionsToDelete.Add(g_editedActions[g_editedSection].Get(i));

	// model init (we display/edit copies for apply/cancel)
	for (int sec=0; sec < SNM_MAX_CYCLING_SECTIONS; sec++) {
		g_editedActions[sec].EmptySafe(g_editedSection != sec); // keep current (displayed!) pointers
		for (int i=0; i < g_cyclactions[sec].GetSize(); i++)
			g_editedActions[sec].Add(new Cyclaction(g_cyclactions[sec].Get(i)));
	}
	g_editedAction = NULL;
	UpdateListViews();
} // + actionsToDelete auto clean-up!

void Apply()
{
	// consolidated undo points
	g_undos = (g_pCyclactionWnd && g_pCyclactionWnd->IsConsolidatedUndo());
	WritePrivateProfileString("Cyclactions", "Undos", g_undos ? "1" : "0", g_SNMIniFn.Get()); // in main S&M.ini file (local property)

	// cycle actions
	AllEditListItemEnd(true);
	bool wasEdited = g_edited;
	UpdateEditedStatus(false); // ok, apply: eof edition, g_edited=false here!
	SaveCyclactions(g_editedActions);
#ifdef _WIN32
	// force ini file cache refresh: fix for the strange issue 397 (?)
	// see http://support.microsoft.com/kb/68827 & http://code.google.com/p/sws-extension/issues/detail?id=397
	WritePrivateProfileString(NULL, NULL, NULL, g_SNMCyclactionIniFn.Get());
#endif
	LoadCyclactions(wasEdited, true); // + flush, unregister, re-register
	EditModelInit();
}

void Cancel(bool _checkSave)
{
	AllEditListItemEnd(false);
	if (_checkSave && g_edited && 
			IDYES == MessageBox(g_pCyclactionWnd?g_pCyclactionWnd->GetHWND():GetMainHwnd(),
				__LOCALIZE("Save cycle actions before quitting editor ?","sws_DLG_161"),
				__LOCALIZE("S&M - Warning","sws_DLG_161"), MB_YESNO))
	{
		SaveCyclactions(g_editedActions);
		LoadCyclactions(true, true); // + flush, unregister, re-register
	}
	UpdateEditedStatus(false); // cancel: eof edition
	EditModelInit();
}

void ResetSection(int _section)
{
	// keep pointers (may be used in a listview): deleted after listview update
	WDL_PtrList_DeleteOnDestroy<Cyclaction> actionsToDelete;
	if (_section == g_editedSection)
		for (int i=0; i < g_editedActions[_section].GetSize(); i++)
			actionsToDelete.Add(g_editedActions[_section].Get(i));

	g_editedActions[_section].EmptySafe(_section != g_editedSection);

	if (_section == g_editedSection)
	{
		g_editedAction = NULL;
		UpdateListViews();
	}
	UpdateEditedStatus(true);
} // + actionsToDelete auto clean-up!


////////////////////
// Left list view
////////////////////

SNM_CyclactionsView::SNM_CyclactionsView(HWND hwndList, HWND hwndEdit)
:SWS_ListView(hwndList, hwndEdit, 3, g_cyclactionsCols, "LCyclactionViewState", false) {}

void SNM_CyclactionsView::GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax)
{
	if (str) *str = '\0';
	if (Cyclaction* a = (Cyclaction*)item)
	{
		switch (iCol)
		{
			case 0: 
				if (!a->m_added)
				{
					int id = g_editedActions[g_editedSection].Find(a);
					if (id >= 0)
						_snprintf(str, iStrMax, "%5.d", id+1);
				}
				else
					lstrcpyn(str, "*", iStrMax);
				break;
			case 1:
				lstrcpyn(str, a->GetName(), iStrMax);
				break;
			case 2:
				if (a->IsToggle())
					lstrcpyn(str, UTF8_BULLET, iStrMax);
				break;
		}
	}
}

//JFB cancel cell editing on error would be great.. SWS_ListView mod?
void SNM_CyclactionsView::SetItemText(SWS_ListItem* _item, int _iCol, const char* _str)
{
	if (strchr(_str, ',') || strchr(_str, '\"') || strchr(_str, '\'') || strchr(_str, '#')) {
		WDL_FastString msg(__LOCALIZE("Cycle action names cannot contain any of the following characters:","sws_DLG_161"));
		msg.Append(" # , \" '");
		MessageBox(GetMainHwnd(), msg.Get(), __LOCALIZE("S&M - Error","sws_DLG_161"), MB_OK);
		return;
	}

	Cyclaction* a = (Cyclaction*)_item;
	if (a && a != &g_DEFAULT_L && _iCol == 1)
	{
		WDL_FastString errMsg;
		if (!CheckEditableCyclaction(_str, &errMsg) && strcmp(a->GetName(), _str)) {
			if (errMsg.GetLength())
				MessageBox(g_pCyclactionWnd?g_pCyclactionWnd->GetHWND():GetMainHwnd(), errMsg.Get(), __LOCALIZE("S&M - Error","sws_DLG_161"), MB_OK);
		}
		else {
			a->SetName(_str);
			UpdateEditedStatus(true);
		}
	}
	// no update on cell editing (disabled)
}

bool SNM_CyclactionsView::IsEditListItemAllowed(SWS_ListItem* _item, int _iCol) {
	Cyclaction* a = (Cyclaction*)_item;
	return (a != &g_DEFAULT_L);
}

void SNM_CyclactionsView::GetItemList(SWS_ListItemList* pList)
{
	if (CountEditedActions())
	{
		for (int i = 0; i < g_editedActions[g_editedSection].GetSize(); i++)
		{
			Cyclaction* a = (Cyclaction*)g_editedActions[g_editedSection].Get(i);
			if (a && !a->IsEmpty())
				pList->Add((SWS_ListItem*)a);
		}
	}
	else
		pList->Add((SWS_ListItem*)&g_DEFAULT_L);
}

void SNM_CyclactionsView::OnItemSelChanged(SWS_ListItem* item, int iState)
{
	AllEditListItemEnd(true);
	g_editedAction = (item && iState ? (Cyclaction*)item : NULL);
	g_lvR->Update();
}

void SNM_CyclactionsView::OnItemBtnClk(SWS_ListItem* item, int iCol, int iKeyState) {
	if (item && iCol == 2) {
		Cyclaction* pItem = (Cyclaction*)item;
		pItem->SetToggle(!pItem->IsToggle());
		Update();
		UpdateEditedStatus(true);
	}
}


////////////////////
// Right list view
////////////////////

SNM_CommandsView::SNM_CommandsView(HWND hwndList, HWND hwndEdit)
:SWS_ListView(hwndList, hwndEdit, 2, g_commandsCols, "RCyclactionViewState", false)
{
//	SetWindowLongPtr(hwndList, GWL_STYLE, GetWindowLongPtr(hwndList, GWL_STYLE) | LVS_SINGLESEL);
}

void SNM_CommandsView::GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax)
{
	if (str) *str = '\0';
	if (WDL_FastString* pItem = (WDL_FastString*)item)
	{
		switch (iCol)
		{
			case 0:
				lstrcpyn(str, pItem->Get(), iStrMax);				
				break;
			case 1:
				if (pItem->GetLength() && pItem->Get()[0] == '!') {
					lstrcpyn(str, __LOCALIZE("Step -----","sws_DLG_161"), iStrMax);
					return;
				}
				// API LIMITATION: only for the main section..
				if (g_editedAction && !g_editedSection)
				{
					int cmd = atoi(pItem->Get());
					if (cmd >= g_iFirstCommand && cmd <= g_iLastCommand)
						return; // user has entered a cmd id instead of a custom id for an SWS action..
					if (cmd = NamedCommandLookup(pItem->Get()))
						lstrcpyn(str, kbd_getTextFromCmd(cmd, NULL), iStrMax); 
				}
				break;
		}
	}
}

void SNM_CommandsView::SetItemText(SWS_ListItem* _item, int _iCol, const char* _str)
{
	WDL_FastString* cmd = (WDL_FastString*)_item;
	if (cmd && cmd != &g_EMPTY_R && cmd != &g_DEFAULT_R && 
		_str && g_editedAction && strcmp(cmd->Get(), _str))
	{
		g_editedAction->SetCmd(cmd, _str);

		char buf[128] = "";
		GetItemText(_item, 1, buf, 128);
		ListView_SetItemText(m_hwndList, GetEditingItem(), DisplayToDataCol(1), buf);
		// ^^ direct GUI update 'cause Update() disabled during cell editing

		UpdateEditedStatus(true);
	}
}

bool SNM_CommandsView::IsEditListItemAllowed(SWS_ListItem* _item, int _iCol) {
	WDL_FastString* cmd = (WDL_FastString*)_item;
	return (cmd != &g_EMPTY_R && cmd != &g_DEFAULT_R);
}

void SNM_CommandsView::GetItemList(SWS_ListItemList* pList)
{
	if (g_editedAction)
	{
		if (int nb = g_editedAction->GetCmdSize())
			for (int i = 0; i < nb; i++)
				pList->Add((SWS_ListItem*)g_editedAction->GetCmdString(i));
		else if (CountEditedActions())
			pList->Add((SWS_ListItem*)&g_DEFAULT_R);
	}
	else if (CountEditedActions())
		pList->Add((SWS_ListItem*)&g_EMPTY_R);
}

// special sort criteria
// (so that the order of commands can not be altered when sorting the list)
int SNM_CommandsView::OnItemSort(SWS_ListItem* _item1, SWS_ListItem* _item2) 
{
	if (g_editedAction) {
		int i1 = g_editedAction->FindCmd((WDL_FastString*)_item1);
		int i2 = g_editedAction->FindCmd((WDL_FastString*)_item2);
		if (i1 >= 0 && i2 >= 0) {
			if (i1 > i2) return 1;
			else if (i1 < i2) return -1;
		}
	}
	return 0;
}

void SNM_CommandsView::OnBeginDrag(SWS_ListItem* item) {
	AllEditListItemEnd(true);
	SetCapture(GetParent(m_hwndList));
}

void SNM_CommandsView::OnDrag()
{
	if (g_editedAction)
	{
		POINT p;
		GetCursorPos(&p);
		WDL_FastString* hitItem = (WDL_FastString*)GetHitItem(p.x, p.y, NULL);
		if (hitItem)
		{
			WDL_PtrList<SWS_ListItem> draggedItems;
			int iNewPriority = g_editedAction->FindCmd(hitItem);
			int x=0, iSelPriority;
			SWS_ListItem* selItem = EnumSelected(&x);
/*JFB!!! SWS_ListView issue (r539): limit to one item ^^
			while(SWS_ListItem* selItem = EnumSelected(&x))
*/
			{
				iSelPriority = g_editedAction->FindCmd((WDL_FastString*)selItem);
				if (iNewPriority == iSelPriority)
					return;
				draggedItems.Add(selItem);
			}

			// remove the dragged items and then re-add them
			// switch order of add based on direction of drag
			bool bDir = iNewPriority > iSelPriority;
			for (int i = bDir ? 0 : draggedItems.GetSize()-1; bDir ? i < draggedItems.GetSize() : i >= 0; bDir ? i++ : i--) {
				g_editedAction->RemoveCmd((WDL_FastString*)draggedItems.Get(i));
				g_editedAction->InsertCmd(iNewPriority, (WDL_FastString*)draggedItems.Get(i));
			}

//JFB!!! SWS_ListView issue (r539): simple Update() is KO here
//       because of the list view's "special" sort criteria
//       => force GUI refresh 
			ListView_DeleteAllItems(m_hwndList);
			Update();
			for (int i = 0; i < draggedItems.GetSize(); i++)
				SelectByItem(draggedItems.Get(i));
			UpdateEditedStatus(draggedItems.GetSize() > 0);
		}
	}
}

void SNM_CommandsView::OnItemSelChanged(SWS_ListItem* item, int iState) {
	AllEditListItemEnd(true);
}


///////////////////////////////////////////////////////////////////////////////
// SNM_CyclactionWnd
///////////////////////////////////////////////////////////////////////////////

// commands
#define ADD_CYCLACTION_MSG				0xF001
#define DEL_CYCLACTION_MSG				0xF002
#define RUN_CYCLACTION_MSG				0xF003
#define ADD_CMD_MSG						0xF010
#define LEARN_CMD_MSG					0xF011
#define DEL_CMD_MSG						0xF012
#define IMPORT_CUR_SECTION_MSG			0xF020
#define IMPORT_ALL_SECTIONS_MSG			0xF021
#define EXPORT_SEL_MSG					0xF022
#define EXPORT_CUR_SECTION_MSG			0xF023
#define EXPORT_ALL_SECTIONS_MSG			0xF024
#define RESET_CUR_SECTION_MSG			0xF030
#define RESET_ALL_SECTIONS_MSG			0xF031

enum
{
  COMBOID_SECTION=2000, //JFB would be great to have _APS_NEXT_CONTROL_VALUE *always* defined
  TXTID_SECTION,
  BUTTONID_UNDO,
  BUTTONID_APPLY,
  BUTTONID_CANCEL,
  BUTTONID_ACTIONLIST
};

SNM_CyclactionWnd::SNM_CyclactionWnd() : SWS_DockWnd(IDD_SNM_CYCLACTION,
		__LOCALIZE("S&M - Cycle Action editor","sws_DLG_161"),
		"SnMCyclaction", 30011, SWSGetCommandID(OpenCyclactionView))
{
	// Must call SWS_DockWnd::Init() to restore parameters and open the window if necessary
	Init();
}

INT_PTR SNM_CyclactionWnd::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static int sListOldColors[LISTVIEW_COLORHOOK_STATESIZE][2];
	if (ListView_HookThemeColorsMessage(m_hwnd, uMsg, lParam, sListOldColors[0], IDC_LIST1, 0, 3) ||
		ListView_HookThemeColorsMessage(m_hwnd, uMsg, lParam, sListOldColors[1], IDC_LIST2, 0, 2))
		return 1;
	return SWS_DockWnd::WndProc(uMsg, wParam, lParam);
}

void SNM_CyclactionWnd::Update(bool _updateListViews)
{
	if (_updateListViews)
		for (int i=0; i < m_pLists.GetSize(); i++)
			m_pLists.Get(i)->Update();
	m_parentVwnd.RequestRedraw(NULL);
}

void SNM_CyclactionWnd::OnInitDlg()
{
	g_lvL = new SNM_CyclactionsView(GetDlgItem(m_hwnd, IDC_LIST1), GetDlgItem(m_hwnd, IDC_EDIT));
	m_pLists.Add(g_lvL);
	SNM_ThemeListView(g_lvL);

	g_lvR = new SNM_CommandsView(GetDlgItem(m_hwnd, IDC_LIST2), GetDlgItem(m_hwnd, IDC_EDIT));
	m_pLists.Add(g_lvR);
	SNM_ThemeListView(g_lvR);

	m_resize.init_item(IDC_LIST1, 0.0, 0.0, 0.5, 1.0);
	m_resize.init_item(IDC_LIST2, 0.5, 0.0, 1.0, 1.0);
#ifdef _SNM_MFC
	m_resize.init_item(IDC_APPLY, 0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_ABORT, 0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_COMMAND, 1.0, 1.0, 1.0, 1.0);
#endif
	// WDL GUI init
	m_vwnd_painter.SetGSC(WDL_STYLE_GetSysColor);
    m_parentVwnd.SetRealParent(m_hwnd);

	m_txtSection.SetID(TXTID_SECTION);
	m_txtSection.SetText("Section:");
	m_parentVwnd.AddChild(&m_txtSection);

	m_cbSection.SetID(COMBOID_SECTION);
	for (int i=0; i < SNM_MAX_CYCLING_SECTIONS; i++)
		m_cbSection.AddItem(g_cyclactionSections[i]);
	m_cbSection.SetCurSel(g_editedSection);
	m_parentVwnd.AddChild(&m_cbSection);

	m_btnUndo.SetID(BUTTONID_UNDO);
	m_btnUndo.SetCheckState(g_undos);
	m_parentVwnd.AddChild(&m_btnUndo);

	m_btnApply.SetID(BUTTONID_APPLY);
	m_parentVwnd.AddChild(&m_btnApply);

	m_btnCancel.SetID(BUTTONID_CANCEL);
	m_parentVwnd.AddChild(&m_btnCancel);

	m_btnActionList.SetID(BUTTONID_ACTIONLIST);
	m_parentVwnd.AddChild(&m_btnActionList);
#ifdef _SNM_MFC
	EnableWindow(GetDlgItem(m_hwnd, IDC_APPLY), g_edited);
#endif
	Update();
}

void SNM_CyclactionWnd::OnDestroy() 
{
/*no! would be triggered on dock/undock..
	Cancel(true);
*/
	m_cbSection.Empty();
}

void SNM_CyclactionWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	int x=0;
	Cyclaction* action = (Cyclaction*)g_lvL->EnumSelected(&x);
	x=0; WDL_FastString* cmd = (WDL_FastString*)g_lvR->EnumSelected(&x);

	switch (LOWORD(wParam))
	{
		case ADD_CYCLACTION_MSG: 
		{
			Cyclaction* a = new Cyclaction(__LOCALIZE("Untitled","sws_DLG_161"));
			a->m_added = true;
			g_editedActions[g_editedSection].Add(a);
			UpdateListViews();
			UpdateEditedStatus(true);
			g_lvL->SelectByItem((SWS_ListItem*)a);
			g_lvL->EditListItem((SWS_ListItem*)a, 1);
		}
		break;
		case DEL_CYCLACTION_MSG: // remove cyclaction (for the user.. in fact it clears)
		{
			// keep pointers (may be used in a listview: delete after listview update)
			int x=0; WDL_PtrList_DeleteOnDestroy<WDL_FastString> cmdsToDelete;
			while(Cyclaction* a = (Cyclaction*)g_lvL->EnumSelected(&x)) {
				for (int i=0; i < a->GetCmdSize(); i++)
					cmdsToDelete.Add(a->GetCmdString(i));
				a->Update(EMPTY_CYCLACTION);
			}
//no!			if (cmdsToDelete.GetSize()) 
			{
				g_editedAction = NULL;
				UpdateListViews();
				UpdateEditedStatus(true);
			}
		} // + cmdsToDelete auto clean-up
		break;
		case RUN_CYCLACTION_MSG:
			if (action)
			{
				int cycleId = g_editedActions[g_editedSection].Find(action);
				if (cycleId >= 0)
				{
					char custCmdId[SNM_MAX_ACTION_CUSTID_LEN] = "";
					_snprintf(custCmdId, SNM_MAX_ACTION_CUSTID_LEN, "_%s%d", g_cyclactionCustomIds[g_editedSection], cycleId+1);
					if (int id = NamedCommandLookup(custCmdId)) {
						Main_OnCommand(id, 0);
						break;
					}
					MessageBox(m_hwnd,
						__LOCALIZE("This action is not registered yet!","sws_DLG_161"),
						__LOCALIZE("S&M - Error","sws_DLG_161"), MB_OK);
				}
			}
			break;
		case ADD_CMD_MSG:
			if (g_editedAction)
			{
				WDL_FastString* newCmd = g_editedAction->AddCmd("!");
				g_lvR->Update();
				UpdateEditedStatus(true);
				g_lvR->EditListItem((SWS_ListItem*)newCmd, 0);
			}
			break;
		case LEARN_CMD_MSG:
		{
			char idstr[SNM_MAX_ACTION_CUSTID_LEN] = "";
			if (LearnAction(idstr, SNM_MAX_ACTION_CUSTID_LEN, g_cyclactionSections[g_editedSection]))
			{
				WDL_FastString* newCmd = g_editedAction->AddCmd(idstr);
				g_lvR->Update();
				UpdateEditedStatus(true);
				g_lvR->SelectByItem((SWS_ListItem*)newCmd);
			}
			break;
		}
		case DEL_CMD_MSG:
			if (g_editedAction)
			{
				// keep pointers (may be used in a listview: delete after listview update)
				int x=0; WDL_PtrList_DeleteOnDestroy<WDL_FastString> cmdsToDelete;
				while(WDL_FastString* delcmd = (WDL_FastString*)g_lvR->EnumSelected(&x)) {
					cmdsToDelete.Add(delcmd);
					g_editedAction->RemoveCmd(delcmd, false);
				}
				if (cmdsToDelete.GetSize()) {
					g_lvR->Update();
					UpdateEditedStatus(true);
				}
			} // + cmdsToDelete auto clean-up
			break;
		case IMPORT_CUR_SECTION_MSG:
			if (char* fn = BrowseForFiles(__LOCALIZE("S&M - Import cycle actions","sws_DLG_161"), g_lastImportFn, NULL, false, SNM_INI_EXT_LIST))
			{
				LoadCyclactions(true, false, g_editedActions, g_editedSection, fn);
				lstrcpyn(g_lastImportFn, fn, BUFFER_SIZE);
				free(fn);
				g_editedAction = NULL;
				UpdateListViews();
				UpdateEditedStatus(true);
			}
			break;
		case IMPORT_ALL_SECTIONS_MSG:
			if (char* fn = BrowseForFiles(__LOCALIZE("S&M - Import cycle actions","sws_DLG_161"), g_lastImportFn, NULL, false, SNM_INI_EXT_LIST))
			{
				LoadCyclactions(true, false, g_editedActions, -1, fn);
				lstrcpyn(g_lastImportFn, fn, BUFFER_SIZE);
				free(fn);
				g_editedAction = NULL;
				UpdateListViews();
				UpdateEditedStatus(true);
			}
			break;
		case EXPORT_SEL_MSG:
		{
			int x=0; WDL_PtrList_DeleteOnDestroy<Cyclaction> actions[SNM_MAX_CYCLING_SECTIONS];
			while(Cyclaction* a = (Cyclaction*)g_lvL->EnumSelected(&x))
				actions[g_editedSection].Add(new Cyclaction(a));
			if (actions[g_editedSection].GetSize()) {
				char fn[BUFFER_SIZE] = "";
				if (BrowseForSaveFile(__LOCALIZE("S&M - Export cycle actions","sws_DLG_161"), g_lastExportFn, strrchr(g_lastExportFn, '.') ? g_lastExportFn : NULL, SNM_INI_EXT_LIST, fn, BUFFER_SIZE)) {
					SaveCyclactions(actions, g_editedSection, fn);
					lstrcpyn(g_lastExportFn, fn, BUFFER_SIZE);
				}
			}
			break;
		}
		case EXPORT_CUR_SECTION_MSG:
		{
			WDL_PtrList_DeleteOnDestroy<Cyclaction> actions[SNM_MAX_CYCLING_SECTIONS];
			for (int i=0; i < g_editedActions[g_editedSection].GetSize(); i++)
				actions[g_editedSection].Add(new Cyclaction(g_editedActions[g_editedSection].Get(i)));
			if (actions[g_editedSection].GetSize()) {
				char fn[BUFFER_SIZE] = "";
				if (BrowseForSaveFile(__LOCALIZE("S&M - Export cycle actions","sws_DLG_161"), g_lastExportFn, strrchr(g_lastExportFn, '.') ? g_lastExportFn : NULL, SNM_INI_EXT_LIST, fn, BUFFER_SIZE)) {
					SaveCyclactions(actions, g_editedSection, fn);
					lstrcpyn(g_lastExportFn, fn, BUFFER_SIZE);
				}
			}
			break;
		}
		case EXPORT_ALL_SECTIONS_MSG:
			if (g_editedActions[0].GetSize() || g_editedActions[1].GetSize() || g_editedActions[2].GetSize()) // yeah, i know..
			{
				char fn[BUFFER_SIZE] = "";
				if (BrowseForSaveFile(__LOCALIZE("S&M - Export cycle actions","sws_DLG_161"), g_lastExportFn, g_lastExportFn, SNM_INI_EXT_LIST, fn, BUFFER_SIZE)) {
					SaveCyclactions(g_editedActions, -1, fn);
					lstrcpyn(g_lastExportFn, fn, BUFFER_SIZE);
				}
			}
			break;
		case RESET_CUR_SECTION_MSG:
			ResetSection(g_editedSection);
			break;
		case RESET_ALL_SECTIONS_MSG:
			for (int sec=0; sec < SNM_MAX_CYCLING_SECTIONS; sec++)
				ResetSection(sec);
			break;
/*JFB
		case IDC_HELPTEXT:
			ShellExecute(m_hwnd, "open", "http://wiki.cockos.com/wiki/index.php/ALR_Main_S%26M_CREATE_CYCLACTION" , NULL, NULL, SW_SHOWNORMAL);
			break;
*/
		case COMBOID_SECTION:
			if (HIWORD(wParam)==CBN_SELCHANGE) {
				AllEditListItemEnd(false);
				UpdateSection(m_cbSection.GetCurSel());
			}
			break;
		case BUTTONID_UNDO:
			if (!HIWORD(wParam) || HIWORD(wParam)==600) {
				m_btnUndo.SetCheckState(!m_btnUndo.GetCheckState()?1:0);
				UpdateEditedStatus(true);
			}
			break;
#ifdef _SNM_MFC
		case IDC_APPLY:
#else
		case BUTTONID_APPLY:
#endif
			Apply();
			break;
#ifdef _SNM_MFC
		case IDC_ABORT: // and not IDCANCEL which is automatically used when closing (see sws_wnd.cpp) !
#else
		case BUTTONID_CANCEL:
#endif
			Cancel(false);
			break;
#ifdef _SNM_MFC
		case IDC_COMMAND:
#else
		case BUTTONID_ACTIONLIST:
#endif
			AllEditListItemEnd(false);
			Main_OnCommand(40605, 0);
			break;
		default:
			Main_OnCommand((int)wParam, (int)lParam);
			break;
	}
}

void SNM_CyclactionWnd::DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight)
{
	LICE_CachedFont* font = SNM_GetThemeFont();

	// 1st row of controls
	int x0=_r->left+10, h=35;
	if (_tooltipHeight)
		*_tooltipHeight = h;

	m_txtSection.SetFont(font);
	if (SNM_AutoVWndPosition(&m_txtSection, NULL, _r, &x0, _r->top, h, 5)) {
		m_cbSection.SetFont(font);
		if (SNM_AutoVWndPosition(&m_cbSection, &m_txtSection, _r, &x0, _r->top, h))
		{
/*no! remains a GUI only info only until applied..
			m_btnUndo.SetCheckState(g_undos);
*/
			m_btnUndo.SetTextLabel(__LOCALIZE("Consolidated undo points","sws_DLG_161"), -1, font);
			if (SNM_AutoVWndPosition(&m_btnUndo, NULL, _r, &x0, _r->top, h))
				SNM_AddLogo(_bm, _r, x0, h);
		}
	}

	// 2nd row of controls
	x0 = _r->left+8; h=39;
	int y0 = _r->bottom-h;

	SNM_SkinToolbarButton(&m_btnApply, __LOCALIZE("Apply","sws_DLG_161"));
	m_btnApply.SetGrayed(!g_edited);
	if (SNM_AutoVWndPosition(&m_btnApply, NULL, _r, &x0, y0, h, 1))
	{
		SNM_SkinToolbarButton(&m_btnCancel, __LOCALIZE("Cancel","sws_DLG_161"));
		if (SNM_AutoVWndPosition(&m_btnCancel, NULL, _r, &x0, y0, h, 1))
		{
			SNM_SkinToolbarButton(&m_btnActionList, __LOCALIZE("Action list...","sws_DLG_161"));
			SNM_AutoVWndPosition(&m_btnActionList, NULL, _r, &x0, y0, h, 0);

			// right re-align
			RECT r; m_btnActionList.GetPosition(&r);
			int w = r.right-r.left;
			r.left = _r->right-w-9;
			r.right = r.left+w;
			m_btnActionList.SetPosition(&r);
		}
	}
}

HBRUSH SNM_CyclactionWnd::OnColorEdit(HWND _hwnd, HDC _hdc)
{
	if (_hwnd == GetDlgItem(m_hwnd, IDC_EDIT))
	{
		int bg, txt;
		SNM_GetThemeListColors(&bg, &txt); // not SNM_GetThemeEditColors (lists' IDC_EDIT)
		SetBkColor(_hdc, bg);
		SetTextColor(_hdc, txt);
		return SNM_GetThemeBrush(bg);
	}
	return 0;
}

HMENU SNM_CyclactionWnd::OnContextMenu(int x, int y, bool* wantDefaultItems)
{
	HMENU hMenu = CreatePopupMenu();

	AllEditListItemEnd(true);
		
	bool left=false, right=false; // which list view?
	{
		POINT pt = {x, y};
		RECT r;	GetWindowRect(g_lvL->GetHWND(), &r);
		left = PtInRect(&r, pt) ? true : false;
		GetWindowRect(g_lvR->GetHWND(), &r);
		right = PtInRect(&r,pt) ? true : false;
	}

	if (left || right)
	{
		*wantDefaultItems = false;
		Cyclaction* action = (Cyclaction*)g_lvL->GetHitItem(x, y, NULL);
		WDL_FastString* cmd = (WDL_FastString*)g_lvR->GetHitItem(x, y, NULL);
		if (left)
		{
			AddToMenu(hMenu, __LOCALIZE("Add cycle action","sws_DLG_161"), ADD_CYCLACTION_MSG);
			if (action && action != &g_DEFAULT_L)
			{
				AddToMenu(hMenu, __LOCALIZE("Remove cycle action(s)","sws_DLG_161"), DEL_CYCLACTION_MSG); 
				AddToMenu(hMenu, SWS_SEPARATOR, 0);
				AddToMenu(hMenu, __LOCALIZE("Run","sws_DLG_161"), RUN_CYCLACTION_MSG, -1, false, action->m_added ? MF_GRAYED : MF_ENABLED); 
			}
		}
		else if (g_editedAction && g_editedAction != &g_DEFAULT_L)
		{
			AddToMenu(hMenu, __LOCALIZE("Add command","sws_DLG_161"), ADD_CMD_MSG);
#ifdef _WIN32
			AddToMenu(hMenu, __LOCALIZE("Add/learn from Actions window","sws_DLG_161"), LEARN_CMD_MSG);
#endif
			if (cmd && cmd != &g_EMPTY_R && cmd != &g_DEFAULT_R)
				AddToMenu(hMenu, __LOCALIZE("Remove command(s)","sws_DLG_161"), DEL_CMD_MSG);
		}
	}
	else
	{
		char buf[128] = "";
		HMENU hImpExpSubMenu = CreatePopupMenu();
		AddSubMenu(hMenu, hImpExpSubMenu, __LOCALIZE("Import/export...","sws_DLG_161"));
		if (_snprintf(buf, 128, __LOCALIZE_VERFMT("Import in section '%s'...","sws_DLG_161"), g_cyclactionSections[g_editedSection]) > 0)
			AddToMenu(hImpExpSubMenu, buf, IMPORT_CUR_SECTION_MSG);
		AddToMenu(hImpExpSubMenu, __LOCALIZE("Import all sections...","sws_DLG_161"), IMPORT_ALL_SECTIONS_MSG);
		AddToMenu(hImpExpSubMenu, SWS_SEPARATOR, 0);
		AddToMenu(hImpExpSubMenu, __LOCALIZE("Export selected cycle actions...","sws_DLG_161"), EXPORT_SEL_MSG);
		if (_snprintf(buf, 128, __LOCALIZE_VERFMT("Export section '%s'...","sws_DLG_161"), g_cyclactionSections[g_editedSection]) > 0)
			AddToMenu(hImpExpSubMenu, buf, EXPORT_CUR_SECTION_MSG);
		AddToMenu(hImpExpSubMenu, __LOCALIZE("Export all sections...","sws_DLG_161"), EXPORT_ALL_SECTIONS_MSG);

		HMENU hResetSubMenu = CreatePopupMenu();
		AddSubMenu(hMenu, hResetSubMenu, __LOCALIZE("Reset","sws_DLG_161"));
		if (_snprintf(buf, 128, __LOCALIZE_VERFMT("Reset section '%s'...","sws_DLG_161"), g_cyclactionSections[g_editedSection]) > 0)
			AddToMenu(hResetSubMenu, buf, RESET_CUR_SECTION_MSG);
		AddToMenu(hResetSubMenu, __LOCALIZE("Reset all sections","sws_DLG_161"), RESET_ALL_SECTIONS_MSG);
	}
	return hMenu;
}

int SNM_CyclactionWnd::OnKey(MSG* _msg, int _iKeyState) 
{
	if (_msg->message == WM_KEYDOWN && !_iKeyState)
	{
		if (HWND h = GetFocus())
		{
			int focusedList = -1;
			if (h == m_pLists.Get(0)->GetHWND()) focusedList = 0;
			else if (h == m_pLists.Get(1)->GetHWND()) focusedList = 1;
			else return 0;

			switch(_msg->wParam)
			{
				case VK_F2:
				{
					int x=0;
					if (SWS_ListItem* item = m_pLists.Get(focusedList)->EnumSelected(&x)) {
						m_pLists.Get(focusedList)->EditListItem(item, !focusedList ? 1 : 0);
						return 1;
					}
				}
				case VK_DELETE:
					if (focusedList == 0) {
						OnCommand(DEL_CYCLACTION_MSG, 0);
						return 1;
					}
					else if (focusedList == 1) {
						OnCommand(DEL_CMD_MSG, 0);
						return 1;
					}
			}
		}
	}
	return 0;
}

void SNM_CyclactionWnd::UpdateSection(int _newSection)
{
	if (_newSection != g_editedSection)
	{
		g_editedSection = _newSection;
		g_editedAction = NULL;
		UpdateListViews();
	}
}


///////////////////////////////////////////////////////////////////////////////

static bool ProcessExtensionLine(const char *line, ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	if (!isUndo || !g_undos) // undo only (no save)
		return false;

	LineParser lp(false);
	if (lp.parse(line) || lp.getnumtokens() < 1)
		return false;

	// load cycle actions' states
	if (!strcmp(lp.gettoken_str(0), "<S&M_CYCLACTIONS"))
	{
		char linebuf[128];
		while(true)
		{
			if (!ctx->GetLine(linebuf,sizeof(linebuf)) && !lp.parse(linebuf))
			{
				if (lp.getnumtokens() && lp.gettoken_str(0)[0] == '>')
					break;
				else if (lp.getnumtokens() == 3)
				{
					int success, sec, cycleId, state;
					sec = lp.gettoken_int(0, &success);
					if (success) cycleId = lp.gettoken_int(1, &success);
					if (success) state = lp.gettoken_int(2, &success);
					if (success && g_cyclactions[sec].Get(cycleId) && g_cyclactions[sec].Get(cycleId)->m_performState != state)
					{
						char custCmdId[SNM_MAX_ACTION_CUSTID_LEN] = "";
						_snprintf(custCmdId, SNM_MAX_ACTION_CUSTID_LEN, "_%s%d", g_cyclactionCustomIds[sec], cycleId+1);
						if (int cmdId = NamedCommandLookup(custCmdId))
						{
							// dynamic action renaming
							SWSFreeUnregisterDynamicCmd(cmdId);
							RegisterCyclation(g_cyclactions[sec].Get(cycleId)->GetStepName(state), g_cyclactions[sec].Get(cycleId)->IsToggle(), sec, cycleId+1, cmdId);

							g_cyclactions[sec].Get(cycleId)->m_performState = state; // before refreshing toolbars!
							RefreshToolbar(cmdId);
						}
						else
							g_cyclactions[sec].Get(cycleId)->m_performState = state;
					}
				}
			}
			else
				break;
		}
		return true;
	}
	return false;
}

static void SaveExtensionConfig(ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	if (!isUndo || !g_undos) // undo only (no save)
		return;

	WDL_FastString confStr("<S&M_CYCLACTIONS\n");
	int iHeaderLen = confStr.GetLength();
	for (int i=0; i < SNM_MAX_CYCLING_SECTIONS; i++)
		for (int j=0; j < g_cyclactions[i].GetSize(); j++)
			if (!g_cyclactions[i].Get(j)->IsEmpty())
				confStr.AppendFormatted(128,"%d %d %d\n", i, j, g_cyclactions[i].Get(j)->m_performState);
	if (confStr.GetLength() > iHeaderLen)
	{
		confStr.Append(">\n");
		StringToExtensionConfig(&confStr, ctx);
	}
}

static project_config_extension_t g_projectconfig = {
	ProcessExtensionLine, SaveExtensionConfig, NULL, NULL
};


///////////////////////////////////////////////////////////////////////////////

int CyclactionInit()
{
	// init section names
	lstrcpyn(g_cyclactionSections[0], __localizeFunc("Main","accel_sec",0), SNM_MAX_SECTION_NAME_LEN);
	lstrcpyn(g_cyclactionSections[1], __localizeFunc("MIDI Event List Editor","accel_sec",0), SNM_MAX_SECTION_NAME_LEN);
	lstrcpyn(g_cyclactionSections[2], __localizeFunc("MIDI Editor","accel_sec",0), SNM_MAX_SECTION_NAME_LEN);

	_snprintf(g_lastExportFn, BUFFER_SIZE, SNM_CYCLACTION_EXPORT_FILE, GetResourcePath());
	_snprintf(g_lastImportFn, BUFFER_SIZE, SNM_CYCLACTION_EXPORT_FILE, GetResourcePath());

	// load undo pref (default == enabled for ascendant compatibility)
	g_undos = (GetPrivateProfileInt("Cyclactions", "Undos", 1, g_SNMIniFn.Get()) == 1 ? true : false); // in main S&M.ini file (local property)

	// load cycle actions
	LoadCyclactions(false, false); // do not check cmd ids (may not have been registered yet)

	if (!plugin_register("projectconfig",&g_projectconfig))
		return 0;

	g_pCyclactionWnd = new SNM_CyclactionWnd();
	if (!g_pCyclactionWnd)
		return 0;

	g_editedSection = 0;
	g_edited = false;
	EditModelInit();

	return 1;
}

void CyclactionExit() {
	DELETE_NULL(g_pCyclactionWnd);
}

void OpenCyclactionView(COMMAND_T* _ct)
{
	if (g_pCyclactionWnd) {
		g_pCyclactionWnd->Show((g_editedSection == (int)_ct->user) /* i.e toggle */, true);
		g_pCyclactionWnd->SetType((int)_ct->user);
	}
}

bool IsCyclactionViewDisplayed(COMMAND_T*){
	return (g_pCyclactionWnd && g_pCyclactionWnd->IsValidWindow());
}

