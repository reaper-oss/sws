#ifndef _GROOVE_DIALOG_H_
#define _GROOVE_DIALOG_H_

class GrooveDialog : public SWS_DockWnd
{
public:
	GrooveDialog();
	void Refresh() { if (IsValidWindow()) RefreshGrooveList(); }
	void ApplySelectedGroove(); // NF: made public, so we can call from action list
	virtual ~GrooveDialog();
private:
	// SWS_DockWnd overrides
	void OnInitDlg();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	int  OnKey(MSG* msg, int iKeyState);
	HMENU OnContextMenu(int x, int y, bool* wantDefaultItems);

	void OnGrooveList(WORD wParam, LPARAM lParam);
	void OnGrooveFolderButton(WORD wParam, LPARAM lParam);
	void OnStrengthChange(WORD wParam, LPARAM lParam);
	void OnVelStrengthChange(WORD wParam, LPARAM lParam);
	void RefreshGrooveList();

	std::string currentDir;
	bool mHorizontalView;
};

#endif /* _GROOVE_DIALOG_H_ */
