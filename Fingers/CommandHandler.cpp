#include "stdafx.h"

#include "CommandHandler.h"
#include "RprException.h"

#include <WDL/localize/localize.h>

static COMMAND_T createSWSCommand(const char* description, const char* id, RprCommand* command);
static void onSWSCommand(COMMAND_T* cmd);
static int onToggleCommand(COMMAND_T* cmd);
static COMMAND_T createSWSToggleCommand(const char* description, const char* id, RprToggleCommand* command);

class SWSCommandFinder
{
public:
    SWSCommandFinder(const char* id);
    bool operator()(const COMMAND_T& cmd);
private:
    const char* mId;
};


RprCommand::RprCommand()
: mCommand(NULL), mData(NULL), mUndoFlags(NO_UNDO)
{}

RprCommand::RprCommand(void (*command)(int, void *), void *commandData, int commandDataSize)
{
    setup(command, commandData, commandDataSize);
}

void RprCommand::setup(void (*command)(int, void *), void *commandData, int commandDataSize)
{
    mCommand = command;
    mData = malloc(commandDataSize);
    ::memcpy(mData, commandData, commandDataSize);
}

RprCommand::~RprCommand()
{
    if(mData != NULL)
        free(mData);
}

void RprCommand::registerCommand(const char *description, const char *id,
                                 void (*command)(int, void *), int undoFlag)
{
    RprCommand *newCmd = new RprCommand(command);
    RprCommand::registerCommand(description, id, newCmd, undoFlag);
}

void RprCommand::registerCommand(const char *description, const char *id,
                                 void (*command)(int, void *), int commandData, int undoFlag)
{
    RprCommand *newCmd = new RprCommand(command, &commandData, sizeof(int));
    RprCommand::registerCommand(description, id, newCmd, undoFlag);
}

void RprCommand::registerToggleCommand(const char *description, const char *id,
                                       void (*command)(int, void *), int (*toggleCommand)(void), int undoFlag)
{
    RprToggleCommand *newCmd = new RprToggleCommand(command, toggleCommand);
    newCmd->setUndoFlags(undoFlag);
    newCmd->setDescription(description);
    RprCommandManager::addToggleCommand(description, id, newCmd);
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

void RprCommand::run(int flag)
{
    if(mUndoFlags != 0 && mUndoFlags != UNDO_STATE_ITEMS)
        Undo_BeginBlock();

    try
    {
        doCommand(flag);
    }
    catch(RprLibException &e)
    {
        if(e.notify())
        {
            MessageBox(GetMainHwnd(), e.what(), __LOCALIZE("FNG - Error","sws_mbox"), 0);
        }
    }

    if(mUndoFlags != 0 && mUndoFlags != UNDO_STATE_ITEMS)
    {
        Undo_EndBlock(mDescription.c_str(), mUndoFlags);
    }
    if(mUndoFlags == UNDO_STATE_ITEMS)
        Undo_OnStateChange(mDescription.c_str());
}


RprCommandManager *RprCommandManager::_instance = NULL;

RprCommandManager *RprCommandManager::Instance()
{
    if (_instance == NULL)
    {
        _instance = new RprCommandManager();
    }
    return _instance;
}

RprCommandManager::~RprCommandManager()
{}

RprToggleCommand::RprToggleCommand(void (*command)(int, void *),
                                   int (*toggleCommand)(void), void *commandData , int commandDataSize)
                                   : RprCommand(command, commandData, commandDataSize)
{
    mToggleCommand = toggleCommand;
}

int RprToggleCommand::runToggleAction()
{
    return mToggleCommand();
}

void RprCommandManager::addCommand(const char *description, const char *id, RprCommand *command)
{
    RprCommandManager *me = RprCommandManager::Instance();
    me->mSWSCommands.push_back(createSWSCommand(description, id, command));
    SWSRegisterCmd(&me->mSWSCommands.back(), __FILE__);
}

void RprCommandManager::addToggleCommand(const char *description, const char *id, RprToggleCommand *command)
{
    RprCommandManager *me = RprCommandManager::Instance();
    me->mSWSCommands.push_back(createSWSToggleCommand(description, id, command));
    SWSRegisterCmd(&me->mSWSCommands.back(), __FILE__);
}

static COMMAND_T
createSWSCommand(const char* description, const char* id, RprCommand* command)
{
    COMMAND_T SWSCommand = { { DEFACCEL, description }, id, onSWSCommand, NULL,
        (INT_PTR)command, NULL};
    return SWSCommand;
}

static COMMAND_T
createSWSToggleCommand(const char* description, const char* id, RprToggleCommand* command)
{
    COMMAND_T SWSCommand = { { DEFACCEL, description }, id, onSWSCommand, NULL,
        (INT_PTR)command, onToggleCommand};
    return SWSCommand;
}

static void
onSWSCommand(COMMAND_T* cmd)
{
    RprCommand* fingersCommand = (RprCommand*)cmd->user;
    fingersCommand->run(0);
}

static int
onToggleCommand(COMMAND_T* cmd)
{
    RprToggleCommand* fingersCommand = (RprToggleCommand*)cmd->user;
    return fingersCommand->runToggleAction();
}

int RprCommandManager::getCommandId(const char *id)
{
    RprCommandManager *me = RprCommandManager::Instance();

    std::list<COMMAND_T>::iterator command = std::find_if(me->mSWSCommands.begin(),
        me->mSWSCommands.end(), SWSCommandFinder(id));

    if (command != me->mSWSCommands.end())
        return command->cmdId;
    return -1;
}

SWSCommandFinder::SWSCommandFinder(const char* id)
: mId(id) {}

bool SWSCommandFinder::operator()(const COMMAND_T& cmd)
{
    return strcmp(cmd.id, mId) == 0;
}
