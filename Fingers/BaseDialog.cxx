#include "stdafx.h"

#include "BaseDialog.hxx"


INT_PTR CALLBACK CBaseDialog::DialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CBaseDialog *baseDialog = (CBaseDialog *)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	switch(uMsg)
	{
	case WM_INITDIALOG:
		/* Set the dialog parameter to our dialog object */
		if (baseDialog == NULL) {
			baseDialog = (CBaseDialog *)lParam;
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
		}
		/* allow to fall through */
	default:
		if(baseDialog)
			return baseDialog->handleMsg(hwndDlg, uMsg, wParam, lParam);
	}
	return TRUE;
}

INT_PTR CBaseDialog::handleMsg( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
     {
     case WM_COMMAND:
		 OnCommand(wParam, lParam);
		 break;
	 case WM_ACTIVATEAPP:
		 OnActivate(wParam == TRUE);
		 break;
	 case WM_CLOSE:
	 case WM_DESTROY:
		Quit();
		break;
     }
     return 0;
}

CBaseDialog::CBaseDialog(HINSTANCE hInstance, HWND parent, int dialogTemplate)
{
	beenShown = false;
	isHiding = true;
	theHandle = hInstance;
	theParent = parent;
	theDialogTemplate = dialogTemplate;
}

void CBaseDialog::Copy(CBaseDialog &rhs)
{
	theHwnd = rhs.theHwnd;
}

void CBaseDialog::Quit()
{
	if(GetParent(theHwnd) == NULL) {
		// sws : not applicable to reaper ext
		//PostQuitMessage (0);
		//CloseWindow(theHwnd);
	}
	Hide();
	
}

CBaseDialog::CBaseDialog(CBaseDialog &rhs)
{
	Copy(rhs);
}

CBaseDialog& CBaseDialog::operator=(CBaseDialog &rhs)
{
	if(this != &rhs)
		Copy(rhs);
	return *this;
}

void CBaseDialog::Toggle()
{
	if(isHiding)
		Show();
	else 
		Hide();
}

void CBaseDialog::Show()
{
	isHiding = false;
	if(!beenShown) {
		theHwnd = CreateDialogParam(theHandle, MAKEINTRESOURCE(theDialogTemplate), theParent, DialogProc, (LPARAM)this);
		beenShown = true;
		Setup();
	}
	
	ShowWindow(theHwnd, SW_SHOW);
	OnCreate(theHwnd);
	UpdateWindow(theHwnd);
}

void CBaseDialog::Hide()
{
	isHiding = true;
	onHide();
	ShowWindow(theHwnd, SW_HIDE);
}
