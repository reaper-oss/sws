#include "stdafx.h"

#include "CommandHandler.h"
#include "RprException.hxx"

CReaperCommandHandler *CReaperCommandHandler::_instance = NULL;

CReaperCommandHandler *CReaperCommandHandler::Instance()
{
	if (_instance == NULL)
	{
		_instance = new CReaperCommandHandler();
	}
	return _instance;
}

CReaperCommand::CReaperCommand() :
m_nCommandId(0), theCommand(NULL), theCommandData(NULL)
{}

bool CReaperCommand::Run(int command, int flag)
{
	if (m_nCommandId && command != m_nCommandId)
		return false;
	if(m_UndoFlags != 0 && m_UndoFlags != UNDO_STATE_ITEMS)
		Undo_BeginBlock();
	PreCommand();
	double startCursorPos = GetCursorPosition();
	try {
		DoCommand(flag);
	} catch(RprLibException &e) {
		if(e.notify()) {
			MessageBox(GetMainHwnd(), e.what(), "Error", 0);
		}
	}
	PostCommand();

	if(!LeaveEditCursorAlone())
		SetEditCurPos(startCursorPos, false, false);
	
	if(m_UndoFlags != 0 && m_UndoFlags != UNDO_STATE_ITEMS) {
		Undo_EndBlock(m_szDescription.c_str(), m_UndoFlags);
	}
	if(m_UndoFlags == UNDO_STATE_ITEMS)
		Undo_OnStateChange(m_szDescription.c_str());
	
	return true;
}

CReaperCommand::~CReaperCommand()
{
	if(theCommandData != NULL)
		free(theCommandData);
}

void CReaperCommand::Setup(void (*command)(int, void *), void *commandData, int commandDataSize)
{
	theCommand = command;
	theCommandData = malloc(commandDataSize);
	::memcpy(theCommandData, commandData, commandDataSize);
	m_nCommandId = 0;
	m_UndoFlags = 0;
}

CReaperCommand::CReaperCommand(void (*command)(int, void *), double commandData)
{
	Setup(command, &commandData, sizeof(double));
}

CReaperCommand::CReaperCommand(void (*command)(int, void *), int commandData)
{
	Setup(command, &commandData, sizeof(int));
}

CReaperCommand::CReaperCommand(void (*command)(int, void *), void *commandData, int commandDataSize)
{
	Setup(command, commandData, commandDataSize);
}

void CReaperCommandHandler::RegisterObserver(CReaperCommandObserver *observer)
{
	CReaperCommandHandler *me = CReaperCommandHandler::Instance();
	me->m_observers.push_back(observer);
}

reaper_plugin_info_t* CReaperCommandHandler::GetPluginInfo()
{
	CReaperCommandHandler *me = CReaperCommandHandler::Instance();
	return me->m_rec;
}

void CReaperCommandHandler::Init(reaper_plugin_info_t *rec, REAPER_PLUGIN_HINSTANCE hInstance)
{
	CReaperCommandHandler *me = CReaperCommandHandler::Instance();
	me->m_rec = rec;
	me->m_hInstance = hInstance;
	rec->Register("hookcommand", (void *)CReaperCommandHandler::hookCommand);
#ifdef _SWS_MENU
	rec->Register("hookcustommenu", (void*)CReaperCommandHandler::menuhook);
#endif
	rec->Register("toggleaction", (void*)CReaperCommandHandler::toggleActionHook);
}

REAPER_PLUGIN_HINSTANCE CReaperCommandHandler::GetModuleInstance()
{
	CReaperCommandHandler *me = CReaperCommandHandler::Instance();
	return me->m_hInstance;
}

void CReaperCommandHandler::AddCommand(CReaperCmdReg &cmdReg)
{
	CReaperCommandHandler *me = CReaperCommandHandler::Instance();

	cmdReg.m_cmd->SetCommandId(me->m_rec->Register("command_id", (void*)cmdReg.m_cmd->GetId()));
	gaccel_register_t gaccel;
	gaccel.accel = cmdReg.m_accel;
	gaccel.desc = cmdReg.m_cmd->GetDescription();
	gaccel.accel.cmd = cmdReg.m_cmd->GetCommandId();
	me->m_rec->Register("gaccel", &gaccel);

	cmdReg.m_cmd->Init();
	me->m_commands.push_back(cmdReg.m_cmd);

	if (cmdReg.m_toggleStateFunc)
	{
		CReaperCmdToggle* tog = new CReaperCmdToggle(gaccel.accel.cmd, cmdReg.m_toggleStateFunc);
		me->m_commandToggles.push_back(tog);
	}
}

void CReaperCommandHandler::AddProjectConfig(project_config_extension_t *cfg)
{
	CReaperCommandHandler *me = CReaperCommandHandler::Instance();
	me->m_rec->Register("projectconfig", cfg);
}

void CReaperCommandHandler::AddCommands(CReaperCmdReg *cmds, int size)
{
	CReaperCommandHandler *me = CReaperCommandHandler::Instance();
	int i = 0;
	while(i < size)
		me->AddCommand(cmds[i++]); 
}

bool CReaperCommandHandler::hookCommand(int command, int flag)
{
	CReaperCommandHandler *me = CReaperCommandHandler::Instance();
	std::for_each(me->m_observers.begin(), me->m_observers.end(), CReaperCommandHandler::NotifyObserver(command, flag));

	std::vector<CReaperCommand *>::iterator it;
	for( it = me->m_commands.begin(); it != me->m_commands.end(); it++)
	{
		CReaperCommand *cmd = (*it);
		if(cmd->Run(command,flag))
			return true;
	}
	return false;
}

int CReaperCommandHandler::GetID(void (*command)(int, void *))
{
	CReaperCommandHandler *me = CReaperCommandHandler::Instance();
	std::vector<CReaperCommand *>::iterator it;
	for( it = me->m_commands.begin(); it != me->m_commands.end(); it++)
	{
		CReaperCommand *cmd = (*it);
		if (cmd->IsCommand(command))
			return cmd->GetCommandId();
	}
	return 0;
}

void CReaperCommandHandler::registerAccelerator(accelerator_register_t *accelerator)
{
	CReaperCommandHandler *me = CReaperCommandHandler::Instance();
	me->m_rec->Register("accelerator", (void *)accelerator);
}

bool CReaperCommandHandler::onAction(int cmd, int val, int valhw, int relmode, HWND hwnd)
{
	CReaperCommandHandler *me = CReaperCommandHandler::Instance();
	for(std::vector<SWS_DockWnd*>::iterator i = me->mKbdSectionDialogs.begin(); i != me->mKbdSectionDialogs.end(); i++) {
		SWS_DockWnd *dialog = *i;
		if(dialog->GetHWND() == hwnd)
		{
			dialog->OnCommand((WPARAM)cmd, 0);
			return true;
		}
	}
	return false;
}

void CReaperCommandHandler::menuhook(const char* menustr, HMENU hMenu, int flag)
{
	if (strcmp(menustr, "Main extensions") == 0 && flag == 0)
	{
		CReaperCommandHandler *me = CReaperCommandHandler::Instance();
		for(std::vector<CReaperCommand*>::iterator i = me->m_commands.begin(); i != me->m_commands.end(); i++)
		{
			CReaperCommand* cmd = *i;
			if (cmd->GetMenuText())
				AddToMenu(hMenu, cmd->GetMenuText(), cmd->GetCommandId());
		}

		AddToMenu(hMenu, SWS_SEPARATOR, 0);
	}
}

// returns:
// -1 = action does not belong to this extension, or does not toggle
//  0 = action belongs to this extension and is currently set to "off"
//  1 = action belongs to this extension and is currently set to "on"
int CReaperCommandHandler::toggleActionHook(int iCmd)
{
	CReaperCommandHandler *me = CReaperCommandHandler::Instance();
	for(std::vector<CReaperCmdToggle*>::iterator i = me->m_commandToggles.begin(); i != me->m_commandToggles.end(); i++)
	{
		CReaperCmdToggle* tog = *i;
		if (tog->GetCommandId() == iCmd)
			return tog->GetToggleState();
	}
	return -1;
}

void CReaperCommandHandler::registerKbdSection(KbdSectionInfo *kbdSection, SWS_DockWnd *dialog)
{
	CReaperCommandHandler *me = CReaperCommandHandler::Instance();
	kbdSection->onAction = CReaperCommandHandler::onAction;
	me->m_rec->Register("accel_section", kbdSection);
	me->mKbdSectionDialogs.push_back(dialog);
}

CReaperCommandHandler::~CReaperCommandHandler()
{ 
	std::vector<CReaperCommand *>::iterator it;
	for( it = m_commands.begin(); it != m_commands.end(); it++)
	{
		CReaperCommand *cmd = (*it);
		delete cmd;
	}
	std::vector<CReaperCmdToggle *>::iterator it2;
	for( it2 = m_commandToggles.begin(); it2 != m_commandToggles.end(); it2++)
	{
		CReaperCmdToggle *tog = (*it2);
		delete tog;
	}
}

CReaperCmdReg::CReaperCmdReg(const char *szDescription, const char *szID, CReaperCommand *cmd, int undoFlags, ACCEL *accel, bool (*toggleStateFunc)(), const char* cMenuText) :
	m_cmd(cmd)
{
	if(accel)
		m_accel = *accel;
	else
	{
		m_accel.cmd  = 0;
		m_accel.fVirt = 0;
		m_accel.key = 0;
	}
	if(m_cmd)
	{
		m_cmd->SetDescription(szDescription);
		m_cmd->SetId(szID);
		m_cmd->SetUndoFlags(undoFlags);
		m_cmd->SetMenuText(cMenuText);
	}
	m_toggleStateFunc = toggleStateFunc;
}