#include "stdafx.h"

#include "CommandHandler.h"
#include "RprException.hxx"

class RprToggleCommand {
public:
	RprToggleCommand(int cmdID, bool (*toggleStateFunc)()) : m_toggleStateFunc(toggleStateFunc) {}
	int getToggleState() { if (m_toggleStateFunc) return m_toggleStateFunc() ? 1 : 0; return -1; }
private:
	bool (*m_toggleStateFunc)();
};


RprCommandManager *RprCommandManager::_instance = NULL;

RprCommandManager *RprCommandManager::Instance()
{
	if (_instance == NULL)
	{
		_instance = new RprCommandManager();
	}
	return _instance;
}

void RprCommand::registerCommand(const char *description, const char *id, void (*command)(int, void *), int undoFlag)
{
	RprCommand *newCmd = new RprCommand(command);
	RprCommand::registerCommand(description, id, newCmd, undoFlag);
}

void RprCommand::registerCommand(const char *description, const char *id, void (*command)(int, void *), int commandData, int undoFlag)
{
	RprCommand *newCmd = new RprCommand(command, &commandData, sizeof(int));
	RprCommand::registerCommand(description, id, newCmd, undoFlag);
}

void RprCommand::registerCommand(const char *description, const char *id, RprCommand *command, int undoFlag)
{
	command->setUndoFlags(undoFlag);
	command->setDescription(description);
	RprCommandManager::addCommand(description, id, command);
}

void RprCommand::setDescription(const char *description)
{
	mDescription = description;
}

void RprCommand::registerAsToggleCommand(const char *id, bool (*toggleStateFunc)())
{
	RprCommandManager::addToggleCommand(id, toggleStateFunc);
}

void RprCommand::registerAsMenuCommand(const char *id, const char *menuText)
{
	RprCommandManager::addMenuCommand(id, menuText);
}

RprCommand::RprCommand() :
mCommand(NULL), mData(NULL), mUndoFlags(NO_UNDO)
{}

void RprCommand::run(int flag)
{
	if(mUndoFlags != 0 && mUndoFlags != UNDO_STATE_ITEMS)
		Undo_BeginBlock();
	
	try {
		doCommand(flag);
	} catch(RprLibException &e) {
		if(e.notify()) {
			MessageBox(GetMainHwnd(), e.what(), "Error", 0);
		}
	}

	if(mUndoFlags != 0 && mUndoFlags != UNDO_STATE_ITEMS) {
		Undo_EndBlock(mDescription.c_str(), mUndoFlags);
	}
	if(mUndoFlags == UNDO_STATE_ITEMS)
		Undo_OnStateChange(mDescription.c_str());
}

RprCommand::~RprCommand()
{
	if(mData != NULL)
		free(mData);
}

void RprCommand::setup(void (*command)(int, void *), void *commandData, int commandDataSize)
{
	mCommand = command;
	mData = malloc(commandDataSize);
	::memcpy(mData, commandData, commandDataSize);
}

RprCommand::RprCommand(void (*command)(int, void *), void *commandData, int commandDataSize)
{
	setup(command, commandData, commandDataSize);
}

void RprCommandManager::Init(reaper_plugin_info_t *rec, REAPER_PLUGIN_HINSTANCE hInstance)
{
	RprCommandManager *me = RprCommandManager::Instance();
	me->m_rec = rec;
	me->m_hInstance = hInstance;
	rec->Register("hookcommand", (void *)RprCommandManager::commandHook);
#ifdef _SWS_MENU
	rec->Register("hookcustommenu", (void*)RprCommandManager::menuHook);
#endif
	rec->Register("toggleaction", (void*)RprCommandManager::toggleCommandHook);

}

void RprCommandManager::addCommand(const char *description, const char *id, RprCommand *command)
{
	RprCommandManager *me = RprCommandManager::Instance();

	int commandId = me->m_rec->Register("command_id", (void *)id);

	me->mIdMap[id] = commandId;
	me->mCommandMap[commandId] = command;
	
	gaccel_register_t gaccel;
	std::memset(&gaccel, 0, sizeof(gaccel_register_t));
	gaccel.desc = description;
	gaccel.accel.cmd = commandId;
	me->m_rec->Register("gaccel", &gaccel);
}

void RprCommandManager::addToggleCommand(const char *id, bool (*toggleStateFunc)())
{
	int commandId = RprCommandManager::getCommandId(id);
	if(commandId != 0) {
		RprToggleCommand *toggleCommand = new RprToggleCommand(commandId, toggleStateFunc);
		RprCommandManager *me = RprCommandManager::Instance();
		me->mToggleCommandMap[commandId] = toggleCommand;
	}
}

void RprCommandManager::addMenuCommand(const char *id, const char *menuText)
{
	RprCommandManager *me = RprCommandManager::Instance();
	MenuListItem item;
	item.first = RprCommandManager::getCommandId(id);
	item.second = menuText;
	me->mMenuList.push_back( item);
}

bool RprCommandManager::commandHook(int command, int flag)
{
	RprCommandManager *me = RprCommandManager::Instance();
	
	CommandMap::iterator i = me->mCommandMap.find(command);
	if(i == me->mCommandMap.end())
		return false;
	RprCommand *cmd = i->second;
	cmd->run(flag);
	return true;
}

int RprCommandManager::getCommandId(const char *id)
{
	RprCommandManager *me = RprCommandManager::Instance();
	IdMap::const_iterator i = me->mIdMap.find(id);
	if( i == me->mIdMap.end())
		return 0;
	return i->second;
}

void RprCommandManager::menuHook(const char* menustr, HMENU hMenu, int flag)
{
	if (strcmp(menustr, "Main extensions") == 0 && flag == 0)
	{
		RprCommandManager *me = RprCommandManager::Instance();
		for(MenuList::const_iterator i = me->mMenuList.begin(); i != me->mMenuList.end(); ++i) {
			AddToMenu(hMenu, i->second.c_str(), i->first);
		}
		AddToMenu(hMenu, SWS_SEPARATOR, 0);
	}
}

// returns:
// -1 = action does not belong to this extension, or does not toggle
//  0 = action belongs to this extension and is currently set to "off"
//  1 = action belongs to this extension and is currently set to "on"
int RprCommandManager::toggleCommandHook(int command)
{
	RprCommandManager *me = RprCommandManager::Instance();
	ToggleCommandMap::const_iterator i = me->mToggleCommandMap.find(command);
	if( i == me->mToggleCommandMap.end()) {
		return -1;
	}
	return i->second->getToggleState();
}

template< typename T, typename U>
static void deleteMap(std::map<T, U*> &theMap)
{
	for(std::map<T, U *>::iterator i = theMap.begin(); i != theMap.end(); ++i) {
		delete i->second;
	}
	theMap.clear();
}

RprCommandManager::~RprCommandManager()
{ 
	deleteMap(mCommandMap);
	deleteMap(mToggleCommandMap);
}