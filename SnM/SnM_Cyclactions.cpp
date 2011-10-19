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


#include "stdafx.h"
#include "SnM_Actions.h"
#include "SnM_Cyclactions.h"
#include "../../WDL/projectcontext.h"


// [0] = main section action, [1] = ME event list section action, [2] = ME piano roll section action
WDL_PtrList_DeleteOnDestroy<Cyclaction> g_cyclactions[SNM_MAX_CYCLING_SECTIONS];
char g_cyclactionCustomIds[SNM_MAX_CYCLING_SECTIONS][SNM_MAX_ACTION_CUSTID_LEN] = {"S&M_CYCLACTION_", "S&M_ME_LIST_CYCLACTION", "S&M_ME_PIANO_CYCLACTION"};
char g_cyclactionIniSections[SNM_MAX_CYCLING_SECTIONS][64] = {"MAIN_CYCLACTIONS", "ME_LIST_CYCLACTIONS", "ME_PIANO_CYCLACTIONS"};
char g_cyclactionSections[SNM_MAX_CYCLING_SECTIONS][SNM_MAX_SECTION_NAME_LEN] = {"Main", "MIDI Event List Editor", "MIDI Editor"}; //JFB must == native section names!

HWND g_cyclactionsHwnd = NULL;
bool g_undos;


///////////////////////////////////////////////////////////////////////////////
// The meat!
///////////////////////////////////////////////////////////////////////////////

// for subbtle "recursive cycle action" cases
// (e.g. a cycle action that calls a macro that calls a cycle action..)
static bool g_bReentrancyCheck = false;

class ScheduledActions : public SNM_ScheduledJob
{
public:
	ScheduledActions(int _approxDelayMs, int _section, int _cycleId, const char* _name, WDL_PtrList<WDL_String>* _actions) 
	: SNM_ScheduledJob(SNM_SCHEDJOB_CYCLACTION, _approxDelayMs), m_section(_section), m_cycleId(_cycleId), m_name(_name)
	{
		for (int i= 0; _actions && i < _actions->GetSize(); i++)
			m_actions.Add(new WDL_String(_actions->Get(i)->Get()));
	}	
	// best effort: ingore unknown actions but goes one..
	void Perform()
	{
		g_bReentrancyCheck = true;

		if (g_undos)
			Undo_BeginBlock2(NULL);

		for (int i= 0; i < m_actions.GetSize(); i++)
		{
			int cmd = NamedCommandLookup(m_actions.Get(i)->Get());
			if (cmd) {
				if (!m_section && !KBD_OnMainActionEx(cmd, 0, 0, 0, g_hwndParent, NULL)) // Main section
					break;
				else if (m_section && !MIDIEditor_LastFocused_OnCommand(cmd, m_section == 1)) // Both ME sections
					break;
			}
		}
		// refresh toolbar button
		{
			char custCmdId[SNM_MAX_ACTION_CUSTID_LEN] = "";
			_snprintf(custCmdId, SNM_MAX_ACTION_CUSTID_LEN, "_%s%d", g_cyclactionCustomIds[m_section], m_cycleId);
			RefreshToolbar(NamedCommandLookup(custCmdId));
		}

		if (g_undos)
			Undo_EndBlock2(NULL, m_name.Get(), UNDO_STATE_ALL);

		m_actions.Empty(true);
		g_bReentrancyCheck = false;
	}
	int m_section, m_cycleId;
	WDL_String m_name;
	WDL_PtrList_DeleteOnDestroy<WDL_String> m_actions;
};

void RunCycleAction(int _section, COMMAND_T* _ct)
{
	int cycleId = (int)_ct->user;
	Cyclaction* action = g_cyclactions[_section].Get(cycleId-1); // cycle action id is 1-based (for user display) !
	if (!action)
		return;

	// process new actions until cycle point
	WDL_String name(action->GetName());
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
	WDL_PtrList_DeleteOnDestroy<WDL_String> actions;
	for (int i=startIdx; i < action->GetCmdSize(); i++)
	{
		const char* cmd = action->GetCmd(i);
		char buf[256] = ""; 
		bool done = false;
		if (i == (action->GetCmdSize()-1))
		{
			action->m_performState = 0;
			if (strcmp(action->GetName(), _ct->accel.desc)) strcpy(buf, action->GetName());
			done = true;
		}
		else if (*cmd == '!') // last ! ignored
		{
			action->m_performState++;
			if (cmd[1])	strcpy(buf, (char *)(cmd+1));
			done = true;
		}

		// add the actions (checks done at perform time, i.e. best effort)
		if (*cmd != '!') {
			actions.Add(new WDL_String(cmd));
			if (!hasCustomIds && !atoi(cmd)) // Custom, extension !?
				hasCustomIds = true;
		}

		// if "!" followed by some text (or cycling back to 1st action, if needed)
		if (*buf)
		{
			// Dynamic action renaming
			if (SWSUnregisterCommand(_ct->accel.accel.cmd) && 
				RegisterCyclation(buf, action->IsToggle(), _section, (int)_ct->user, _ct->accel.accel.cmd))
			{
				SWSFreeCommand(_ct);
			}
		}
		if (done)
			break;
	}

	if (actions.GetSize() && !g_bReentrancyCheck)
	{
		ScheduledActions* job = new ScheduledActions(50, _section, cycleId, name.Get(), &actions);
		// note: I "skip whilst respecting" (!) the SWS re-entrance test - see hookCommandProc() in sws_entension.cpp -
		// thanks to scheduled actions that performed 50ms later (or so). we include macros too (can contain SWS stuff..)
		if (hasCustomIds)
			AddOrReplaceScheduledJob(job);
		// perform immedialtely
		else
		{
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
	Cyclaction* action = g_cyclactions[_type].Get(cycleId-1); // cycle action id is 1-based (for user display) !
	return (action && /*action->IsToggle() &&*/ (action->m_performState % 2) != 0);
}

bool IsMainCyclactionEnabled(COMMAND_T* _ct) {return IsCyclactionEnabled(0, _ct);}
bool IsMEListCyclactionEnabled(COMMAND_T* _ct) {return IsCyclactionEnabled(1, _ct);}
bool IsMEPianoCyclactionEnabled(COMMAND_T* _ct) {return IsCyclactionEnabled(2, _ct);}

bool CheckEditableCyclaction(const char* _actionStr, WDL_String* _errMsg, bool _allowEmpty = true)
{
	if (!(_actionStr && *_actionStr && *_actionStr != ',' && strcmp(_actionStr, "#,")))
	{
		if (_errMsg)
			_errMsg->AppendFormatted(256, "Error: invalid cycle action '%s'\n\n", _actionStr ? _actionStr : "NULL");
		return false;
	}
	if (!_allowEmpty && !strcmp(EMPTY_CYCLACTION, _actionStr))
		return false; // no msg: empty cycle actions are internal stuff
	return true;
}

bool CheckRegisterableCyclaction(int _section, Cyclaction* _a, WDL_String* _errMsg, bool _checkCmdIds)
{
	if (_a)
	{
		int steps=0, noop=0;
		for (int i=0; i < _a->GetCmdSize(); i++)
		{
			const char* cmd = _a->GetCmd(i);
			if (strstr(cmd, "_CYCLACTION")) {
				if (_errMsg) 
					_errMsg->AppendFormatted(256, "Warning: cycle action '%s' (section '%s') was added but not registered\nDetails: recursive cycle action (i.e. uses another cycle action)\n\n", _a->GetName(), g_cyclactionSections[_section]);
				return false;
			}
			else if (*cmd == '!')
				steps++;
			else if (!strcmp(cmd, "65535"))
				noop++;
			else if (!_section) // main section?
			{
				if (atoi(cmd) >= g_iFirstCommand)
				{
					if (_errMsg) 
						_errMsg->AppendFormatted(256, "Warning: cycle action '%s' (section '%s') was added but not registered\nDetails: for extensions' actions, you must use custom ids (e.g. _SWS_ABOUT),\nnot command ids (e.g. 47145)\n\n", _a->GetName(), g_cyclactionSections[_section]);
					return false;
				}
				// API limit: NamedCommandLookup() KO in other sections than the main one
				// => all cyclactions belong to the main section although they can target other sections..
				if(_checkCmdIds && !NamedCommandLookup(cmd))
				{
					if (_errMsg) 
						_errMsg->AppendFormatted(256, "Warning: cycle action '%s' (section '%s') was added but not registered\nDetails: command id (or custom id) '%s' not found\n\n", _a->GetName(), g_cyclactionSections[_section], cmd);
					return false;
				}
			}
		}

		if ((steps + noop) == _a->GetCmdSize())
		{
			if (_errMsg && !_a->IsEmpty()) 
				_errMsg->AppendFormatted(256, "Warning: cycle action '%s' (section '%s') was added but not registered\nDetails: : no valid command id (or custom id) found\n\n", _a->GetName(), g_cyclactionSections[_section]);
			return false;
		}
	}
	else {
		if (_errMsg) 
			_errMsg->AppendFormatted(256, "Error: invalid cycle action (section '%s')\n\n", g_cyclactionSections[_section]);
		return false;
	}
	return true;
}

// return true if cyclaction added (but not necesary registered)
bool CreateCyclaction(int _section, const char* _actionStr, WDL_String* _errMsg, bool _checkCmdIds)
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
		char cID[SNM_MAX_ACTION_CUSTID_LEN];
		_snprintf(cID, SNM_MAX_ACTION_CUSTID_LEN, "%s%d", g_cyclactionCustomIds[_section], _cycleId);
		return SWSRegisterCommandExt3(
			!_section ? RunMainCyclaction : _section == 1 ? RunMEListCyclaction : RunMEPianoCyclaction, 
			!_toggle ? NULL : (!_section ? IsMainCyclactionEnabled : _section == 1 ? IsMEListCyclactionEnabled : IsMEPianoCyclactionEnabled), 
			_cmdId, cID, _name, _cycleId, __FILE__);
	}
	return 0;
}

void FlushCyclactions(int _section)
{
	for (int i=0; i < g_cyclactions[_section].GetSize(); i++)
	{
		char custCmdId[SNM_MAX_ACTION_CUSTID_LEN] = "";
		_snprintf(custCmdId, SNM_MAX_ACTION_CUSTID_LEN, "_%s%d", g_cyclactionCustomIds[_section], i+1);		
		int cmd = NamedCommandLookup(custCmdId);
		COMMAND_T* ct = NULL;
		if (cmd && (ct = SWSUnregisterCommand(cmd)))
		{
			SWSFreeCommand(ct);
		}
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
	char actionStr[MAX_CYCLATION_LEN] = "";
	WDL_String msg;
	for (int sec=0; sec < SNM_MAX_CYCLING_SECTIONS; sec++)
	{
		if (_section == sec || _section == -1)
		{
			if (!_cyclactions)
				FlushCyclactions(sec);

			GetPrivateProfileString(g_cyclactionIniSections[sec], "NB_ACTIONS", "0", buf, 32, _iniFn ? _iniFn : g_SNMiniFilename.Get()); 
			int nb = atoi(buf);
			for (int j=0; j < nb; j++) 
			{
				_snprintf(buf, 32, "ACTION%d", j+1);
				GetPrivateProfileString(g_cyclactionIniSections[sec], buf, EMPTY_CYCLACTION, actionStr, MAX_CYCLATION_LEN, _iniFn ? _iniFn : g_SNMiniFilename.Get());
				// import into _cyclactions
				if (_cyclactions)
				{
					if (CheckEditableCyclaction(actionStr, _errMsg ? &msg : NULL, false))
						_cyclactions[sec].Add(new Cyclaction(actionStr, true));
				}
				// main model update + action register
				else if (!CreateCyclaction(sec, actionStr, _errMsg ? &msg : NULL, _checkCmdIds))
					CreateCyclaction(sec, EMPTY_CYCLACTION, NULL, false);  // +no-op in order to preserve cycle actions' ids
			}
		}
	}

	if (_errMsg && msg.GetLength())
		SNM_ShowMsg(msg.Get(), "S&M - Cycle Actions - Error(s) & Warning(s)", g_cyclactionsHwnd);
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
			WDL_String iniSection("; Do not tweak by hand! Use the Cycle Action editor instead\n"), escapedStr;

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
						iniSection.AppendFormatted(MAX_CYCLATION_LEN+16, "ACTION%d=%s\n", j+1, escapedStr.Get()); 
						maxId = max(j+1, maxId);
					}
					else 
					{
						a->m_added = false;
						if (freeCycleIds.GetSize())
						{
							makeEscapedConfigString(a->m_desc.Get(), &escapedStr);
							int id = *(freeCycleIds.Get(0));
							iniSection.AppendFormatted(MAX_CYCLATION_LEN+16, "ACTION%d=%s\n", id+1, escapedStr.Get()); 
							freeCycleIds.Delete(0, true);
							maxId = max(id+1, maxId);
						}
						else
						{
							makeEscapedConfigString(a->m_desc.Get(), &escapedStr);
							iniSection.AppendFormatted(MAX_CYCLATION_LEN+16, "ACTION%d=%s\n", ++maxId, escapedStr.Get()); 
						}
					}
				}
			}
			// "NB_ACTIONS" is a bad name now: it's a max (kept for upgrability)
			iniSection.AppendFormatted(32, "NB_ACTIONS=%d\n", maxId);
			SaveIniSection(g_cyclactionIniSections[sec], &iniSection, _iniFn ? _iniFn : g_SNMiniFilename.Get());
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

void Cyclaction::SetCmd(WDL_String* _cmd, const char* _newCmd) {
	int i = m_cmds.Find(_cmd);
	if (_cmd && _newCmd && i >= 0) {
		m_cmds.Get(i)->Set(_newCmd);
		UpdateFromCmd();
	}
}

WDL_String* Cyclaction::AddCmd(const char* _cmd) {
	WDL_String* c = new WDL_String(_cmd); 
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
			m_cmds.Add(new WDL_String(tok));
	}
	m_empty = (strcmp(EMPTY_CYCLACTION, m_desc.Get()) == 0);
}

void Cyclaction::UpdateFromCmd()
{
	WDL_String newDesc(IsToggle() ? "#" : "");
	newDesc.Append(GetName());
	newDesc.Append(",");
	for (int i = 0; i < m_cmds.GetSize(); i++) {
		newDesc.Append(m_cmds.Get(i));
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

static SWS_LVColumn g_cyclactionsCols[] = { { 50, 0, "Id" }, { 260, 1, "Cycle action name" }, { 50, 2, "Toggle" } };
static SWS_LVColumn g_commandsCols[] = { { 180, 1, "Command" }, { 180, 0, "Name (main section only)" } };
static SNM_CyclactionsView* g_lvL = NULL;
static SNM_CommandsView* g_lvR = NULL;

// fake list views' contents
static Cyclaction g_DEFAULT_L("Right click here to add cycle actions");
static WDL_String g_EMPTY_R("<- Select a cycle action");
static WDL_String g_DEFAULT_R("Right click here to add commands");

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
	if (g_cyclactionsHwnd)
		EnableWindow(GetDlgItem(g_cyclactionsHwnd, IDC_APPLY), g_edited);
}

void UpdateListViews() {
	if (g_lvL)
		g_lvL->Update();
	if (g_lvR)
		g_lvR->Update();
}

void UpdateSection(int _newSection) {
	if (_newSection != g_editedSection) {
		g_editedSection = _newSection;
		g_editedAction = NULL;
		UpdateListViews();
	}
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
	g_undos = (IsDlgButtonChecked(g_cyclactionsHwnd, IDC_CHECK1) == BST_CHECKED);
	WritePrivateProfileString("Cyclactions", "Undos", g_undos ? "1" : "0", g_SNMiniFilename.Get());

	// cycle actions
	AllEditListItemEnd(true);
	bool wasEdited = g_edited;
	UpdateEditedStatus(false); // ok, apply: eof edition, note: g_edited=false here!
	SaveCyclactions(g_editedActions);
#ifdef _WIN32
	// force ini file cache refresh: fix for the strange issue 397 (?)
	// see http://support.microsoft.com/kb/68827 & http://code.google.com/p/sws-extension/issues/detail?id=397
	WritePrivateProfileString(NULL, NULL, NULL, g_SNMiniFilename.Get());
#endif
	LoadCyclactions(wasEdited, true); // + flush, unregister, re-register
	EditModelInit();
}

void Cancel(bool _checkSave)
{
	AllEditListItemEnd(false);
	if (_checkSave && g_edited && IDYES == MessageBox(g_cyclactionsHwnd ? g_cyclactionsHwnd : g_hwndParent, "Save cycle actions before quitting ?", "S&M - Cycle Action editor - Warning", MB_YESNO))
	{
		SaveCyclactions(g_editedActions);
		LoadCyclactions(true, true); // + flush, unregister, re-register
	}
	UpdateEditedStatus(false); // cancel: eof edition
	EditModelInit();
}

void ResetSection(int _section)
{
	WDL_PtrList_DeleteOnDestroy<Cyclaction> actionsToDelete;
	// keep pointers (may be used in a listview: delete after listview update)
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
	Cyclaction* a = (Cyclaction*)item;
	if (a)
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
	Cyclaction* a = (Cyclaction*)_item;
	if (a && a != &g_DEFAULT_L && _iCol == 1)
	{
		WDL_String errMsg;
		if (!CheckEditableCyclaction(_str, &errMsg) && strcmp(a->GetName(), _str)) 
		{
			if (errMsg.GetLength())
				MessageBox(g_cyclactionsHwnd ? g_cyclactionsHwnd :  g_hwndParent, errMsg.Get(), "S&M - Error", MB_OK);
		}
		else
		{
			a->SetName(_str);
			UpdateEditedStatus(true);
		}
	}
	// no update on cell editing (disabled)
}

// filter EMPTY_CYCLACTIONs
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
	WDL_String* pItem = (WDL_String*)item;
	if (pItem)
	{
		switch (iCol)
		{
			case 0:
				lstrcpyn(str, pItem->Get(), iStrMax);				
				break;
			case 1:
				if (pItem->GetLength() && pItem->Get()[0] == '!') {
					lstrcpyn(str, "Step -----", iStrMax);
					return;
				}
				//JFB API limitation: only for main section..
				if (g_editedAction && !g_editedSection) {
					if (atoi(pItem->Get()) >= g_iFirstCommand)
						return;
					int cmd = NamedCommandLookup(pItem->Get());
					if (cmd)
						lstrcpyn(str, kbd_getTextFromCmd(cmd, NULL), iStrMax); 
				}
				break;
		}
	}
}

void SNM_CommandsView::SetItemText(SWS_ListItem* _item, int _iCol, const char* _str)
{
	WDL_String* cmd = (WDL_String*)_item;
	if (cmd && _str && g_editedAction && cmd != &g_EMPTY_R && cmd != &g_DEFAULT_R && strcmp(cmd->Get(), _str))
	{
		g_editedAction->SetCmd(cmd, _str);

		char buf[128] = "";
		GetItemText(_item, 1, buf, 128);
		ListView_SetItemText(m_hwndList, GetEditingItem(), DisplayToDataCol(1), buf);
		// ^^ direct GUI update 'cause Update() disabled during cell editing

		UpdateEditedStatus(true);
	}
}

void SNM_CommandsView::GetItemList(SWS_ListItemList* pList)
{
	if (g_editedAction)
	{
		int nb = g_editedAction->GetCmdSize();
		if (nb) {
			for (int i = 0; i < nb; i++)
				pList->Add((SWS_ListItem*)g_editedAction->GetCmdString(i));
		}
		else if (CountEditedActions())
			pList->Add((SWS_ListItem*)&g_DEFAULT_R);
	}
	else if (CountEditedActions())
		pList->Add((SWS_ListItem*)&g_EMPTY_R);
}

// special sort criteria
// (so that the order of commands can't be altered when sorting the list)
int SNM_CommandsView::OnItemSort(SWS_ListItem* _item1, SWS_ListItem* _item2) 
{
	if (g_editedAction) {
		int i1 = g_editedAction->FindCmd((WDL_String*)_item1);
		int i2 = g_editedAction->FindCmd((WDL_String*)_item2);
		if (i1 >= 0 && i2 >= 0) {
			if (i1 > i2) return 1;
			else if (i1 < i2) return -1;
		}
	}
	return 0;
}

void SNM_CommandsView::OnBeginDrag(SWS_ListItem* item)
{
	AllEditListItemEnd(true);
	SetCapture(GetParent(m_hwndList));
}

void SNM_CommandsView::OnDrag()
{
	if (g_editedAction)
	{
		POINT p;
		GetCursorPos(&p);
		WDL_String* hitItem = (WDL_String*)GetHitItem(p.x, p.y, NULL);
		if (hitItem)
		{
			int iNewPriority = g_editedAction->FindCmd(hitItem);
			int iSelPriority;

			int x=0;
			WDL_PtrList<SWS_ListItem> draggedItems;
			SWS_ListItem* selItem = EnumSelected(&x);
/*JFB!!! SWS_ListView issue (r539): limit to one item ^^
			while(SWS_ListItem* selItem = EnumSelected(&x))
*/
			{
				iSelPriority = g_editedAction->FindCmd((WDL_String*)selItem);
				if (iNewPriority == iSelPriority)
					return;
				draggedItems.Add(selItem);
			}

			// remove the dragged items and then re-add them
			// switch order of add based on direction of drag
			bool bDir = iNewPriority > iSelPriority;
			for (int i = bDir ? 0 : draggedItems.GetSize()-1; bDir ? i < draggedItems.GetSize() : i >= 0; bDir ? i++ : i--)
			{
				g_editedAction->RemoveCmd((WDL_String*)draggedItems.Get(i));
				g_editedAction->InsertCmd(iNewPriority, (WDL_String*)draggedItems.Get(i));
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


////////////////////
// Wnd
////////////////////

INT_PTR WINAPI CyclactionsWndProc(HWND _hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			SetWindowLongPtr(GetDlgItem(_hwnd, IDC_EDIT), GWLP_USERDATA, 0xdeadf00b); //JFB needed !?
			RestoreWindowPos(_hwnd, CYCLACTIONWND_POS_KEY);

			CheckDlgButton(_hwnd, IDC_CHECK1, g_undos ? BST_CHECKED : BST_UNCHECKED);

			int x, x0; 
			for(int i=0; i < SNM_MAX_CYCLING_SECTIONS; i++) {
				x = (int)SendDlgItemMessage(_hwnd,IDC_COMBO,CB_ADDSTRING,0,(LPARAM)g_cyclactionSections[i]);
				if (!i) x0 = x;
			}
			g_editedSection = 0;
			SendDlgItemMessage(_hwnd,IDC_COMBO,CB_SETCURSEL,x0,g_editedSection);

			HWND hListL = GetDlgItem(_hwnd, IDC_LIST1);
			HWND hListR = GetDlgItem(_hwnd, IDC_LIST2);
			g_lvL = new SNM_CyclactionsView(hListL, GetDlgItem(_hwnd, IDC_EDIT));
			g_lvR = new SNM_CommandsView(hListR, GetDlgItem(_hwnd, IDC_EDIT));
			g_edited = false;
			EnableWindow(GetDlgItem(_hwnd, IDC_APPLY), g_edited);
			EditModelInit();
		}
		return 0;
		case WM_CONTEXTMENU:
		{
			AllEditListItemEnd(true);

			int x = LOWORD(lParam), y = HIWORD(lParam);
			if (g_lvL->DoColumnMenu(x, y) || g_lvR->DoColumnMenu(x, y))
				return 0;

			// which list view?
			bool left=false, right=false;
			{
				POINT pt;
				pt.x = x; pt.y = y;
				HWND h = GetDlgItem(_hwnd, IDC_LIST1);
				RECT r;	GetWindowRect(h, &r);
				left = PtInRect(&r, pt) ? true : false;
				h = GetDlgItem(_hwnd, IDC_LIST2);
				GetWindowRect(h, &r);
				right = PtInRect(&r,pt) ? true : false;
			}

			if (left || right)
			{
				SWS_ListView* lv = (left ? (SWS_ListView*)g_lvL : (SWS_ListView*)g_lvR);
				HMENU menu = CreatePopupMenu();
				Cyclaction* action = (Cyclaction*)g_lvL->GetHitItem(x, y, NULL);
				WDL_String* cmd = (WDL_String*)g_lvR->GetHitItem(x, y, NULL);
				if (left) // reminder: no multi sel in this one
				{
					AddToMenu(menu, "Add cycle action", 1000);
					if (action && action != &g_DEFAULT_L)
					{
						AddToMenu(menu, "Remove selected cycle action(s)", 1001); 
						AddToMenu(menu, SWS_SEPARATOR, 0);
						AddToMenu(menu, "Run", 1002, -1, false, action->m_added ? MF_GRAYED : MF_ENABLED); 
					}
				}
				else if (g_editedAction && g_editedAction != &g_DEFAULT_L)
				{
					AddToMenu(menu, "Add command", 1010);
#ifdef _WIN32
					AddToMenu(menu, "Add/learn from 'Actions' window", 1011);
#endif
					if (cmd && cmd != &g_EMPTY_R && cmd != &g_DEFAULT_R)
						AddToMenu(menu, "Remove selected command(s)", 1012);
				}

				int iCmd = TrackPopupMenu(menu,TPM_LEFTALIGN|TPM_TOPALIGN|TPM_RETURNCMD|TPM_NONOTIFY,x,y,0,_hwnd,NULL);
				if (iCmd > 0)
				{
					switch (iCmd)
					{
						// add cyclaction
						case 1000: {
							Cyclaction* a = new Cyclaction("Untitled");
							a->m_added = true;
							g_editedActions[g_editedSection].Add(a);
							UpdateListViews();
							UpdateEditedStatus(true);
							g_lvL->SelectByItem((SWS_ListItem*)a);
							g_lvL->EditListItem((SWS_ListItem*)a, 1);
						}
						break;
						// remove cyclaction (for the user.. in fact in clears)
						case 1001:
						{
							// keep pointers (may be used in a listview: delete after listview update)
							int x=0; WDL_PtrList_DeleteOnDestroy<WDL_String> cmdsToDelete;
							while(Cyclaction* a = (Cyclaction*)g_lvL->EnumSelected(&x)) {
								for (int i=0; i < a->GetCmdSize(); i++)
									cmdsToDelete.Add(a->GetCmdString(i));
								a->Update(EMPTY_CYCLACTION);
							}
//no!							if (cmdsToDelete.GetSize()) 
							{
								g_editedAction = NULL;
								UpdateListViews();
								UpdateEditedStatus(true);
							}
						} // + cmdsToDelete auto clean-up
						break;

						// run
						case 1002:
							if (action) {
								int cycleId = g_editedActions[g_editedSection].Find(action);
								if (cycleId >= 0)
								{
									char custCmdId[SNM_MAX_ACTION_CUSTID_LEN] = "";
									_snprintf(custCmdId, SNM_MAX_ACTION_CUSTID_LEN, "_%s%d", g_cyclactionCustomIds[g_editedSection], cycleId+1);
									int id = SNM_NamedCommandLookup(custCmdId);
									if (id) {
										Main_OnCommand(id, 0);
										break;
									}
									MessageBox(_hwnd, "This action is not registered !", "S&M - Error", MB_OK);
								}
							}
							break;
						// Add cmd
						case 1010:
							if (g_editedAction) {
								WDL_String* newCmd = g_editedAction->AddCmd("!");
								g_lvR->Update();
								UpdateEditedStatus(true);
								g_lvR->EditListItem((SWS_ListItem*)newCmd, 0);
							}
							break;
						// learn cmd
						case 1011: {
							int actionId; char section[SNM_MAX_SECTION_NAME_LEN] = "", idstr[64] = "";
							if (GetSelectedActionId(section, SNM_MAX_SECTION_NAME_LEN, &actionId, idstr, 64) >= 0 && !strcmp(section, g_cyclactionSections[g_editedSection])) {
								WDL_String* newCmd = g_editedAction->AddCmd(idstr);
								g_lvR->Update();
								UpdateEditedStatus(true);
								g_lvR->SelectByItem((SWS_ListItem*)newCmd);
							}
							else {
								char bufMsg[256] = "";
								_snprintf(bufMsg, 256, "Actions window not opened or section '%s' not selected or no selected action !", g_cyclactionSections[g_editedSection]);
								MessageBox(_hwnd, bufMsg, "S&M - Error", MB_OK);
							}
						}
						break;
						// remove sel cmds
						case 1012:
							if (g_lvR && g_editedAction)
							{
								// keep pointers (may be used in a listview: delete after listview update)
								int x=0; WDL_PtrList_DeleteOnDestroy<WDL_String> cmdsToDelete;
								while(WDL_String* delcmd = (WDL_String*)g_lvR->EnumSelected(&x)) {
									cmdsToDelete.Add(delcmd);
									g_editedAction->RemoveCmd(delcmd, false);
								}
								if (cmdsToDelete.GetSize()) {
									g_lvR->Update();
									UpdateEditedStatus(true);
								}
							} // + cmdsToDelete auto clean-up
							break;
					}
				}
				DestroyMenu(menu);
			}
		}
		break;
		case WM_NOTIFY:
		{
			NMHDR* hdr = (NMHDR*)lParam;
			if (hdr && g_lvL && hdr->hwndFrom == g_lvL->GetHWND())
				return g_lvL->OnNotify(wParam, lParam);
			if (hdr && g_lvR && hdr->hwndFrom == g_lvR->GetHWND())
				return g_lvR->OnNotify(wParam, lParam);
		}
		break;
		case WM_CLOSE:
			Cancel(false); //JFB false: painful check for mods (?)
			SaveWindowPos(_hwnd, CYCLACTIONWND_POS_KEY);
			g_cyclactionsHwnd = NULL; // for proper toggle state report, see openCyclactionsWnd()
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_COMBO: 
					if(HIWORD(wParam) == CBN_SELCHANGE) {
						AllEditListItemEnd(false);
						UpdateSection((int)SendDlgItemMessage(_hwnd,IDC_COMBO,CB_GETCURSEL,0,0));
					}
					break;
				case IDC_LIST:
					AllEditListItemEnd(false);
					//JFB KO!! ShowActionList(NULL, GetMainHwnd());
					Main_OnCommand(40605, 0);
					break;
				case IDOK:
				case IDC_APPLY:
					Apply();
					if (LOWORD(wParam) == IDC_APPLY)
						break;
					g_cyclactionsHwnd = NULL; // for proper toggle state report, see openCyclactionsWnd()
					SaveWindowPos(_hwnd, CYCLACTIONWND_POS_KEY);
					ShowWindow(_hwnd, SW_HIDE);
//JFB r525			EndDialog(_hwnd,0);
					break;
				case IDCANCEL:
					Cancel(false);
					g_cyclactionsHwnd = NULL; // for proper toggle state report, see openCyclactionsWnd()
					SaveWindowPos(_hwnd, CYCLACTIONWND_POS_KEY);
					ShowWindow(_hwnd, SW_HIDE);
//JFB r525			EndDialog(_hwnd,0);
					break;
				case IDC_BROWSE:
				{
					AllEditListItemEnd(true);
					HMENU menu=CreatePopupMenu();
					AddToMenu(menu, "Import in current section...", 1020);
					AddToMenu(menu, "Import all sections...", 1021);
					AddToMenu(menu, SWS_SEPARATOR, 0);
					AddToMenu(menu, "Export selected cycle actions...", 1022);
					AddToMenu(menu, "Export current section...", 1023);
					AddToMenu(menu, "Export all sections...", 1024);

					POINT p={0,}; RECT r;
					GetWindowRect(GetDlgItem(_hwnd, IDC_BROWSE), &r);
					ScreenToClient(_hwnd,(LPPOINT)&r); ScreenToClient(_hwnd,((LPPOINT)&r)+1);
					p.x=r.left;	p.y=r.bottom;
				    ClientToScreen(_hwnd,&p);				    
					int iCmd = TrackPopupMenu(menu,TPM_LEFTALIGN|TPM_TOPALIGN|TPM_RETURNCMD|TPM_NONOTIFY,p.x,p.y,0,_hwnd,NULL);
					if (iCmd > 0)
					{
						switch (iCmd)
						{
							// import in current section
							case 1020:
								if (char* fn = BrowseForFiles("S&M - Import cycle actions", g_lastImportFn, NULL, false, SNM_INI_EXT_LIST)) {
									LoadCyclactions(true, false, g_editedActions, g_editedSection, fn);
									lstrcpyn(g_lastImportFn, fn, BUFFER_SIZE);
									free(fn);
									g_editedAction = NULL;
									UpdateListViews();
									UpdateEditedStatus(true);
								}
								break;
							// import all sections
							case 1021:
								if (char* fn = BrowseForFiles("S&M - Import cycle actions", g_lastImportFn, NULL, false, SNM_INI_EXT_LIST)) {
									LoadCyclactions(true, false, g_editedActions, -1, fn);
									lstrcpyn(g_lastImportFn, fn, BUFFER_SIZE);
									free(fn);
									g_editedAction = NULL;
									UpdateListViews();
									UpdateEditedStatus(true);
								}
								break;

							// export selected cycle actions
							case 1022:
							{
								int x=0; WDL_PtrList_DeleteOnDestroy<Cyclaction> actions[SNM_MAX_CYCLING_SECTIONS];
								while(Cyclaction* a = (Cyclaction*)g_lvL->EnumSelected(&x))
									actions[g_editedSection].Add(new Cyclaction(a));
								if (actions[g_editedSection].GetSize()) {
									char fn[BUFFER_SIZE] = "";
									if (BrowseForSaveFile("S&M - Export cycle actions", g_lastExportFn, g_lastExportFn, SNM_INI_EXT_LIST, fn, BUFFER_SIZE)) {
										SaveCyclactions(actions, g_editedSection, fn);
										strcpy(g_lastExportFn, fn);
									}
								}
							}
							break;
							// export current section
							case 1023:
							{
								WDL_PtrList_DeleteOnDestroy<Cyclaction> actions[SNM_MAX_CYCLING_SECTIONS];
								for (int i=0; i < g_editedActions[g_editedSection].GetSize(); i++)
									actions[g_editedSection].Add(new Cyclaction(g_editedActions[g_editedSection].Get(i)));
								if (actions[g_editedSection].GetSize()) {
									char fn[BUFFER_SIZE] = "";
									if (BrowseForSaveFile("S&M - Export cycle actions", g_lastExportFn, g_lastExportFn, SNM_INI_EXT_LIST, fn, BUFFER_SIZE)) {
										SaveCyclactions(actions, g_editedSection, fn);
										strcpy(g_lastExportFn, fn);
									}
								}
							}
								break;
							// export all sections
							case 1024:
								if (g_editedActions[0].GetSize() || g_editedActions[1].GetSize() || g_editedActions[2].GetSize()) { // yeah.., i know..
									char fn[BUFFER_SIZE] = "";
									if (BrowseForSaveFile("S&M - Export cycle actions", g_lastExportFn, g_lastExportFn, SNM_INI_EXT_LIST, fn, BUFFER_SIZE)) {
										SaveCyclactions(g_editedActions, -1, fn);
										strcpy(g_lastExportFn, fn);
									}
								}
								break;
						}
					}
					DestroyMenu(menu);
				}
				break;
				case IDC_REMOVE:
				{
					AllEditListItemEnd(true);
				    HMENU menu=CreatePopupMenu();
					AddToMenu(menu, "Reset current section", 1030);
					AddToMenu(menu, "Reset all sections", 1031);

					POINT p={0,}; RECT r;
					GetWindowRect(GetDlgItem(_hwnd, IDC_REMOVE), &r);
					ScreenToClient(_hwnd,(LPPOINT)&r); ScreenToClient(_hwnd,((LPPOINT)&r)+1);
					p.x=r.left;	p.y=r.bottom;
				    ClientToScreen(_hwnd,&p);				    
					int iCmd = TrackPopupMenu(menu,TPM_LEFTALIGN|TPM_TOPALIGN|TPM_RETURNCMD|TPM_NONOTIFY,p.x,p.y,0,_hwnd,NULL);
					if (iCmd > 0) {
						switch (iCmd) {
							case 1030:
								ResetSection(g_editedSection);
								break;
							case 1031:
								for (int sec=0; sec < SNM_MAX_CYCLING_SECTIONS; sec++) ResetSection(sec);
								break;
						}
					}
					DestroyMenu(menu);
				}
				break;
				case IDC_HELPTEXT:
					ShellExecute(_hwnd, "open", "http://wiki.cockos.com/wiki/index.php/ALR_Main_S%26M_CREATE_CYCLACTION" , NULL, NULL, SW_SHOWNORMAL);
					break;
				case IDC_CHECK1:
					UpdateEditedStatus(true);
					break;
			}
			return 0;
			case WM_MOUSEMOVE:
				if (GetCapture() == _hwnd) {
					if (g_lvR)
						g_lvR->OnDrag();
					return 1;
				}
				break;
			case WM_LBUTTONUP:
				if (GetCapture() == _hwnd) {
					ReleaseCapture();
					return 1;
				}
				break;
			case WM_DRAWITEM:
				{
					DRAWITEMSTRUCT *di = (DRAWITEMSTRUCT *)lParam;
					if (di->CtlType == ODT_BUTTON) 
					{
						SetTextColor(di->hDC, (di->itemState & ODS_SELECTED) ? RGB(0,0,0) : RGB(0,0,220));
						RECT r = di->rcItem;
						char buf[512];
						GetWindowText(di->hwndItem, buf, sizeof(buf));
						DrawText(di->hDC, buf, -1, &r, DT_NOPREFIX | DT_LEFT | DT_VCENTER);
					}
				}
				break;
		case WM_DESTROY:
			Cancel(false); // JFB false: removed check on close (painful)
			g_lvL->OnDestroy();
			delete g_lvL;
			g_lvL = NULL;
			g_lvR->OnDestroy();
			delete g_lvR;
			g_lvR = NULL;
			break;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////

static int translateAccel(MSG *msg, accelerator_register_t *ctx)
{
	if (msg->message == WM_KEYDOWN)
	{
		SWS_ListView* lv = NULL;
		if (g_lvL && g_lvL->IsActive(true))
			lv = g_lvL;
		else if (g_lvR && g_lvR->IsActive(true))
			lv = g_lvR;

		if (SNM_IsActiveWindow(g_cyclactionsHwnd) || lv)
		{
			if (lv)
			{
				int iRet = lv->EditingKeyHandler(msg);
				if (iRet)
					return iRet;
				iRet = lv->LVKeyHandler(msg, SWS_GetModifiers());
				if (iRet)
					return iRet;
			}
			return -1;
		}
	}
	return 0;
}

static accelerator_register_t g_ar = { translateAccel, TRUE, NULL };

///////////////////////////////////////////////////////////////////////////////

static bool ProcessExtensionLine(const char *line, ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	if (!isUndo || !g_undos) // undo only (no save)
		return false;

	LineParser lp(false);
	if (lp.parse(line) || lp.getnumtokens() < 1)
		return false;

	// Load cycle actions' states
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
						// Dynamic action renaming
						char custCmdId[SNM_MAX_ACTION_CUSTID_LEN] = "";
						_snprintf(custCmdId, SNM_MAX_ACTION_CUSTID_LEN, "_%s%d", g_cyclactionCustomIds[sec], cycleId+1);
						int cmdId = NamedCommandLookup(custCmdId);
						if (cmdId)
						{
							COMMAND_T* c = SWSGetCommandByID(cmdId);
							if (c && SWSUnregisterCommand(cmdId) && 
								RegisterCyclation(g_cyclactions[sec].Get(cycleId)->GetStepName(state), g_cyclactions[sec].Get(cycleId)->IsToggle(), sec, cycleId+1, cmdId))
							{
								SWSFreeCommand(c);
							}
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

	WDL_String confStr("<S&M_CYCLACTIONS\n");
	int iHeaderLen = confStr.GetLength();
	for (int i=0; i < SNM_MAX_CYCLING_SECTIONS; i++)
		for (int j=0; j < g_cyclactions[i].GetSize(); j++)
			if (!g_cyclactions[i].Get(j)->IsEmpty())
				confStr.AppendFormatted(128,"%d %d %d\n", i, j, g_cyclactions[i].Get(j)->m_performState);
	if (confStr.GetLength() > iHeaderLen)
	{	// SWS only write out line if there's cycle actions present
		confStr.Append(">\n");
		StringToExtensionConfig(&confStr, ctx);
	}
}

static project_config_extension_t g_projectconfig = {
	ProcessExtensionLine, SaveExtensionConfig, NULL, NULL
};


///////////////////////////////////////////////////////////////////////////////

int CyclactionsInit()
{
	_snprintf(g_lastExportFn, BUFFER_SIZE, SNM_CYCACTION_INI_FILE, GetResourcePath());
	_snprintf(g_lastImportFn, BUFFER_SIZE, SNM_CYCACTION_INI_FILE, GetResourcePath());

	// load undo pref (default == enabled for ascendant compatibility)
	g_undos = (GetPrivateProfileInt("Cyclactions", "Undos", 1, g_SNMiniFilename.Get()) == 1 ? true : false);

	// load cycle actions
	LoadCyclactions(false, false); // do not check cmd ids (may not have been registered yet)

	if (!plugin_register("accelerator",&g_ar) || !plugin_register("projectconfig",&g_projectconfig))
		return 0;
	return 1;
}

void openCyclactionsWnd(COMMAND_T* _ct)
{
#ifdef _WIN32
	static HWND hwnd = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_SNM_CYCLACTION), g_hwndParent, CyclactionsWndProc);

	// Toggle
	if (g_cyclactionsHwnd && (g_editedSection == (int)_ct->user))
	{
		Cancel(true);
		g_cyclactionsHwnd = NULL;
		ShowWindow(hwnd, SW_HIDE);
	}
	else
	{
		g_cyclactionsHwnd = hwnd;
		ShowWindow(hwnd, SW_SHOW);
		SetFocus(hwnd);
		AllEditListItemEnd(false);
		SendDlgItemMessage(hwnd,IDC_COMBO,CB_SETCURSEL,(int)_ct->user,0); // ok: won't lead to a WM_COMMAND
		UpdateSection((int)_ct->user);
	}
#else
	char reply[4096]= "";
	char question[BUFFER_SIZE]= "Name (#name: toggle action):,Command:";
	for (int i=2; i < SNM_MAX_CYCLING_ACTIONS; i++)
		strcat(question, ",Command (or ! or !new name):");

	char title[128]= "S&M - ";
	strcat(title, SNM_CMD_SHORTNAME(_ct));
	if (GetUserInputs(title, SNM_MAX_CYCLING_ACTIONS, question, reply, 4096))
	{
		WDL_String msg;
		if (CreateCyclaction((int)_ct->user, reply, &msg, true))
			SaveCyclactions();
		else if (msg.GetLength())
			SNM_ShowMsg(msg.Get(), "S&M - Cycle Actions - Error(s) & Warning(s)", g_hwndParent);
	}
#endif
}

bool isCyclationsWndDisplayed(COMMAND_T* _ct) {
	return (g_cyclactionsHwnd && IsWindow(g_cyclactionsHwnd) && IsWindowVisible(g_cyclactionsHwnd) ? true : false);
}
