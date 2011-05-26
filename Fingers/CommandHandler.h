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

    /* Register command as a toggle command. Command must be registered already using registerCommand */
    static void registerAsToggleCommand(const char *id, bool (*toggleStateFunc)());
    /* Register command on menu. Command must be registered already using registerCommand */
    static void registerAsMenuCommand(const char *id, const char *menuText);

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

class RprCommandManager
{
public:
    static RprCommandManager *Instance();
    static void addCommand(const char *description, const char *id, RprCommand *command);
    static void addToggleCommand(const char *id,  bool (*toggleStateFunc)());
    static void addMenuCommand(const char *id, const char *menuText);

    static int getCommandId(const char *id);

    static void Init(reaper_plugin_info_t *, REAPER_PLUGIN_HINSTANCE);
    ~RprCommandManager();

private:
    static int toggleCommandHook(int command);
    static void menuHook(const char* menustr, HMENU hMenu, int flag);
    static bool commandHook(int command, int flag);

    RprCommandManager() : m_rec(NULL), m_hInstance(NULL) {}

    void registerTempToggleCommands();

    static RprCommandManager *_instance;

    typedef std::map<std::string, int> IdMap;
    typedef std::map<int, RprCommand *> CommandMap;
    typedef std::map<int, RprToggleCommand *> ToggleCommandMap;
    typedef std::pair<int, std::string> MenuListItem;
    typedef std::list< MenuListItem > MenuList;


    CommandMap mCommandMap;
    ToggleCommandMap mToggleCommandMap;
    MenuList mMenuList;
    IdMap mIdMap;

    reaper_plugin_info_t *m_rec;
    REAPER_PLUGIN_HINSTANCE m_hInstance;

};


#endif /* _REAPER_COMMAND_HANDLER_H_ */
