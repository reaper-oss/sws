#include "CommandHandler.h"
#include <list>

namespace MiscCommands {
	void Init();
};


class CmdMacroRecorder : public CReaperCommand, public CReaperCommandObserver
{
public:
	CmdMacroRecorder(bool bRecord) : m_bRecord(bRecord)
	{
		m_Run = true;
		if(bRecord)
		{
			CReaperCommandHandler *cHandler = CReaperCommandHandler::Instance();
			cHandler->RegisterObserver(this);
		}
	}
	virtual void Init() {if (!m_bRecord) m_MyCommandIds.push_back(GetCommandId()); m_nCommandCount = 0; }
private:
	bool m_bRecord;
	static int m_nCommandCount;
	static int m_nCurrentCommandCount;
	static bool m_Run;
	static int m_RunCommandId;
	static std::list<int> m_Commands;
	static std::vector<int> m_MyCommandIds;
	virtual void DoCommand(int flag);
	virtual void GetCommand(int command, int flag);
	virtual bool LeaveEditCursorAlone() { return true; }
	static void RunCommand(int command);
};