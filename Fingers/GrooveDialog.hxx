#ifndef _GROOVE_DIALOG_H_
#define _GROOVE_DIALOG_H_

class GrooveDialog : public SWS_DockWnd
{
public:
	GrooveDialog();
	bool IsActive(bool bWantEdit);
	void Refresh() { if (IsValidWindow()) RefreshGrooveList(); }
	virtual ~GrooveDialog();
private:
	// SWS_DockWnd overrides
	void OnInitDlg();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	HMENU OnContextMenu(int x, int y);

	void OnGrooveList(WORD wParam, LPARAM lParam);
	void OnGrooveFolderButton(WORD wParam, LPARAM lParam);
	void OnStrengthChange(WORD wParam, LPARAM lParam);
	void OnVelStrengthChange(WORD wParam, LPARAM lParam);
	void ApplySelectedGroove();
	void RefreshGrooveList();

	std::string currentDir;
	bool mHorizontalView;
};

#endif /* _GROOVE_DIALOG_H_ */