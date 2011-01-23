#ifndef _GROOVE_DIALOG_H_
#define _GROOVE_DIALOG_H_

class GrooveDialog : public SWS_DockWnd
{
public:
	GrooveDialog();
	bool IsActive(bool bWantEdit);
	virtual ~GrooveDialog();
private:
	// SWS_DockWnd overrides
	void OnInitDlg();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	int OnKey(MSG *msg, int iKeyState);
	

	void OnGrooveList(WORD wParam, LPARAM lParam);
	void OnGrooveFolderButton(WORD wParam, LPARAM lParam);
	void OnStrengthChange(WORD wParam, LPARAM lParam);
	void OnStrengthSpinChange(WORD wParam, LPARAM lParam);
	void RefreshGrooveList();
	void ApplySelectedGroove();

	std::string currentDir;
	accelerator_register_t *mAccel;
	KbdSectionInfo *mKbdSection;
	std::vector<KbdCmd> mKbdCommands;
	std::vector<KbdKeyBindingInfo> mKbdBindings;
	bool mPassToMain;
	bool mIgnoreVelocity;
};

#endif /* _GROOVE_DIALOG_H_ */