#ifndef _REAPER_COMMAND_HANDLER_H_
#define _REAPER_COMMAND_HANDLER_H_

#ifndef NO_UNDO
#define NO_UNDO 0
#endif

class CReaperCommandHandler;
class CBaseDialog;
class CReaperCommand
{
public:
	CReaperCommand();
	CReaperCommand(void (*command)(int, void *), void *commandData = NULL, int commandDataSize = 0);
	CReaperCommand(void (*command)(int, void *), double commandData);
	CReaperCommand(void (*command)(int, void *), int commandData);

	virtual ~CReaperCommand();
	
	bool Run(int command, int flag);

	void SetCommandId(int CommandId){ m_nCommandId = CommandId; }
	void SetId(const char *id){ m_szId = id; }
	void SetDescription(const char *description) { m_szDescription = description; }
	void SetUndoFlags(int flags) { m_UndoFlags = flags; }

	int GetCommandId() { return m_nCommandId; }
	const char *GetDescription() { return m_szDescription.c_str(); }

	const char *GetId() { return m_szId.c_str(); }
	virtual void Init() {}
	virtual bool LeaveEditCursorAlone() {return false;}

private:
	void Setup(void (*command)(int, void *), void *commandData, int commandDataSize);
	virtual void PreCommand() {}
	virtual void DoCommand(int flag) {if (theCommand != NULL) theCommand(flag, theCommandData);}
	virtual void PostCommand() {}
	void (*theCommand)(int, void *);
	void *theCommandData;
	int m_nCommandId;
	std::string m_szDescription;
	std::string m_szId;
	int m_UndoFlags;
};

class CReaperCmdReg {

public:
	CReaperCmdReg(char *szDescription, char *szID, CReaperCommand *cmd, int undoFlags = UNDO_STATE_ALL, ACCEL *accel = NULL);
	void SetDescription(const char *szDescription) {if (m_cmd) m_cmd->SetDescription(szDescription);}
	const char *GetDescription() {if(m_cmd) return m_cmd->GetDescription(); return 0;}
	const char *GetId() {if(m_cmd) return m_cmd->GetId(); return 0;}
private:
	friend class CReaperCommandHandler;
	ACCEL m_accel;
	CReaperCommand *m_cmd;
};

class CReaperCommandObserver
{
public:
	virtual void GetCommand(int command, int flag) = 0;
};

class CReaperCommandHandler
{
public:
	static CReaperCommandHandler *Instance();
	static void AddCommand(CReaperCmdReg &);
	static void AddCommands(CReaperCmdReg *, int size);
	static void AddProjectConfig(project_config_extension_t *cfg);
	static bool hookCommand(int command, int flag);
	static bool onAction(int cmd, int val, int valhw, int relmode, HWND hwnd);
	static void Init(reaper_plugin_info_t *, REAPER_PLUGIN_HINSTANCE);
	static REAPER_PLUGIN_HINSTANCE GetModuleInstance();
	static reaper_plugin_info_t* GetPluginInfo();
	static void registerAccelerator(accelerator_register_t *accelerator);
	static void registerKbdSection(KbdSectionInfo *kbdSection, CBaseDialog *dialog);
	static void RegisterObserver(CReaperCommandObserver *observer);
	~CReaperCommandHandler();
private:
	
	class NotifyObserver
	{
	public:
		NotifyObserver(int command, int flag) : m_command(command), m_flag(flag) {}
		void operator () (CReaperCommandObserver *observer) {observer->GetCommand(m_command, m_flag);}
	private:
		int m_command;
		int m_flag;
	};

	CReaperCommandHandler() : m_rec(NULL) {}
	static CReaperCommandHandler *_instance;
	std::vector<CReaperCommand *> m_commands;
	std::vector<CReaperCommandObserver *> m_observers;
	std::vector<CBaseDialog *> mKbdSectionDialogs;
	reaper_plugin_info_t *m_rec;
	REAPER_PLUGIN_HINSTANCE m_hInstance;
	
};


#endif /* _REAPER_COMMAND_HANDLER_H_ */
