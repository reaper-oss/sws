#ifndef _GROOVE_DIALOG_H_
#define _GROOVE_DIALOG_H_

#include "BaseDialog.hxx"
#include <string>
#include <vector>

class GrooveDialog : public CBaseDialog
{
public:
	GrooveDialog(HINSTANCE hInstance, HWND parent);
	void Setup();
	int OnKeyCommand(MSG *msg);
	virtual ~GrooveDialog();
private:
	void OnCommand(WPARAM wParam, LPARAM lParam);
	bool OnRprCommand(int cmd);
	void OnGrooveList(WORD wParam, LPARAM lParam);
	void OnGrooveFolderButton(WORD wParam, LPARAM lParam);
	void OnStrengthChange(WORD wParam, LPARAM lParam);
	void OnStrengthSpinChange(WORD wParam, LPARAM lParam);
	
	void onHide();
	void OnCreate(HWND hWnd);
	void RefreshGrooveList();
	void ApplySelectedGroove();
	void OnActivate(bool activated);

	std::string currentDir;
	accelerator_register_t *mAccel;
	KbdSectionInfo *mKbdSection;
	std::vector<KbdCmd> mKbdCommands;
	std::vector<KbdKeyBindingInfo> mKbdBindings;
	bool mPassToMain;
	bool mStayOnTop;
	
};

#endif /* _GROOVE_DIALOG_H_ */