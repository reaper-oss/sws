#ifndef _REAPER_COMMAND_HANDLER_H_
#define _REAPER_COMMAND_HANDLER_H_

#ifndef NO_UNDO
#define NO_UNDO 0
#endif

class RprCommandManager;
class RprToggleCommand;

class RprCommand
{
public:

    /* Register function as command with no command data */
    static void registerCommand(const char *description, const char *id, void (*command)(int, void *), int undoFlag);
    /* Register function as command with command data */
    static void registerCommand(const char *description, const char *id, void (*command)(int, void *), int commandData, int undoFlag);
    /* Register command with command data */
    static void registerCommand(const char *description, const char *id, RprCommand *command, int undoFlag);

    static void registerToggleCommand(const char *description, const char *id, void (*command)(int, void *), int (*toggleCommand)(void), int undoFlag);

    virtual ~RprCommand();

    void run(int flag);

protected:
    RprCommand();
    RprCommand(void (*command)(int, void *), void *commandData = NULL, int commandDataSize = 0);

private:

    void setUndoFlags(int flags) { mUndoFlags = flags; }
    void setDescription(const char *description);

    void setup(void (*command)(int, void *), void *commandData, int commandDataSize);

    virtual void doCommand(int flag) {if (mCommand != NULL) mCommand(flag, mData);}

    void (*mCommand)(int, void *);
    void *mData;

    std::string mDescription;
    int mUndoFlags;
};

class RprToggleCommand: public RprCommand {
public:
    RprToggleCommand(void (*command)(int, void *), int (*toggleCommand)(void), void *commandData = NULL, int commandDataSize = 0);
    int runToggleAction();
private:
    int (*mToggleCommand)(void);
};

class RprCommandManager
{
public:
    static RprCommandManager *Instance();
    static void addCommand(const char *description, const char *id, RprCommand *command);
    static void addToggleCommand(const char *description, const char *id, RprToggleCommand *command);
    static int getCommandId(const char *id);
    ~RprCommandManager();

private:
    RprCommandManager() {}
    static RprCommandManager *_instance;
    std::list<COMMAND_T> mSWSCommands;
};

#endif /* _REAPER_COMMAND_HANDLER_H_ */
