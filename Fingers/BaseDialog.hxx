#ifndef _BASE_DIALOG_H_
#define _BASE_DIALOG_H_

class CBaseDialog {
public:
	CBaseDialog(HINSTANCE hInstance, HWND parent, int dialogTemplate);
	CBaseDialog(CBaseDialog&); 
	CBaseDialog& operator=(CBaseDialog&);

	HWND getHwnd() {return theHwnd;}
	virtual bool OnRprCommand(int cmd) {return true;}

	void Show();
	void Hide();
	void Toggle();

protected:
	virtual void OnCommand(WPARAM wParam, LPARAM lParam) {}
	virtual void Setup() {}
	virtual void onHide() {}
	virtual void OnActivate(bool activated) {}
	virtual void OnCreate(HWND hWnd) {}
	
	HWND getParentHwnd() {return theParent;}
	HINSTANCE getInstanceHandle() {return theHandle;}

private:
	void Quit();
	static INT_PTR CALLBACK DialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	INT_PTR handleMsg( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void Copy(CBaseDialog&);
	HWND theHwnd;
	HWND theParent;
	int theDialogTemplate;
	HINSTANCE theHandle;
	bool beenShown;
	bool isHiding;
};

struct ControlPos {
	float left;
	float right;
	float top;
	float bottom;
};

#endif /* _BASE_DIALOG_H_ */