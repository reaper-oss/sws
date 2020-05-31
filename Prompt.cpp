/******************************************************************************
/ Prompt.cpp
/
/ Copyright (c) 2013 Tim Payne (SWS), Jeffos
/
/
/ Permission is hereby granted, free of charge, to any person obtaining a copy
/ of this software and associated documentation files (the "Software"), to deal
/ in the Software without restriction, including without limitation the rights to
/ use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
/ of the Software, and to permit persons to whom the Software is furnished to
/ do so, subject to the following conditions:
/ 
/ The above copyright notice and this permission notice shall be included in all
/ copies or substantial portions of the Software.
/ 
/ THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
/ EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
/ OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
/ NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
/ HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
/ WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/ FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
/ OTHER DEALINGS IN THE SOFTWARE.
/
******************************************************************************/

#include "stdafx.h"

#include "Prompt.h"
#include "./SnM/SnM_Dlg.h"

#include <WDL/localize/localize.h>

#define PROMPTWND_KEY	"PromptWindowPos"
#define INFOWND_KEY		"InfoWindowPos"

const char* g_cTitle;
char* g_cString;
int g_iMax;
bool g_bOK = false;
bool g_bOption = false;
bool g_bAtMouse = false;
const char* g_cOption;
HWND g_hwndModeless = NULL;

INT_PTR WINAPI doPromptDialog(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwndDlg, uMsg, wParam, lParam))
		return r;

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			SetWindowText(hwndDlg, g_cTitle);
			HWND h = GetDlgItem(hwndDlg, IDC_EDIT);
			SetWindowText(h, g_cString);
			if (g_cOption) {
				h = GetDlgItem(hwndDlg, IDC_CHECK1);
				SetWindowText(h, g_cOption);
			}
			if (g_bAtMouse)
				SetWindowPosAtMouse(hwndDlg);
			else
				RestoreWindowPos(hwndDlg, PROMPTWND_KEY, false);
			return 0;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_CHECK1:
					g_bOption = (IsDlgButtonChecked(hwndDlg, IDC_CHECK1) == 1);
					break;
				case IDOK:
					GetDlgItemText(hwndDlg, IDC_EDIT, g_cString, g_iMax);
					g_bOK = true;
				// Fall through to cancel to save/end
				case IDCANCEL:
					if (!g_bAtMouse)
						SaveWindowPos(hwndDlg, PROMPTWND_KEY);
					EndDialog(hwndDlg, 0);
					break;
			}
			break;
	}
	return 0;
}

INT_PTR WINAPI doInfoDialog(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static WDL_WndSizer resize;

	if (INT_PTR r = SNM_HookThemeColorsMessage(hwndDlg, uMsg, wParam, lParam))
		return r;

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			AttachWindowResizeGrip(hwndDlg);
			resize.init(hwndDlg);
			resize.init_item(IDC_EDIT);
			SetWindowText(hwndDlg, g_cTitle);
			if (g_bAtMouse)
				SetWindowPosAtMouse(hwndDlg);
			else
				RestoreWindowPos(hwndDlg, INFOWND_KEY);
			HWND hEdit = GetDlgItem(hwndDlg, IDC_EDIT);
			SetWindowLongPtr(hEdit, GWLP_USERDATA, 0xdeadf00b);
			SetWindowText(hEdit, g_cString);
			SetFocus(hEdit);
			SendMessage(hEdit, EM_SETSEL, (WPARAM)-1, 0);
			return 0;
		}
		case WM_SIZE:
			if (wParam != SIZE_MINIMIZED)
				resize.onResize();
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
				case IDCANCEL:
					SaveWindowPos(hwndDlg, INFOWND_KEY);
					if (hwndDlg == g_hwndModeless)
					{
						DestroyWindow(g_hwndModeless);
						g_hwndModeless = NULL;
					}
					else
						EndDialog(hwndDlg, 0);
					break;
			}
			break;
	}
	return 0;
}

// return 0 on cancel, 1 if OK (but option not ticked or no option), 2 if OK + option ticked
int PromptUserForString(HWND hParent, const char* cTitle, char* cString, int iMaxChars, bool bAtMouse, const char* cOption)
{
	g_cTitle = cTitle;
	g_cString = cString;
	g_iMax = iMaxChars;
	g_bOK = false;
	g_bAtMouse = bAtMouse;
	g_cOption = cOption;
	g_bOption = false;
	DialogBox(g_hInst, MAKEINTRESOURCE(cOption ? IDD_PROMPT_OPTION : IDD_PROMPT), hParent, doPromptDialog);
	return g_bOK ? (cOption ? (g_bOption?2:1) : 1) : 0;
}

// if modeless, cTitle=="" will close
void DisplayInfoBox(HWND hParent, const char* cTitle, const char* cInfo, bool bAtMouse, bool bModal)
{
	g_cTitle = cTitle;
	g_cString = (char*)cInfo;
	g_bAtMouse = !bModal && g_hwndModeless ? false : bAtMouse; // ignored if a modeless info box is already displayed

	if (g_hwndModeless)
	{
		SaveWindowPos(g_hwndModeless, INFOWND_KEY);
		DestroyWindow(g_hwndModeless);
		g_hwndModeless = NULL;
	}

	if (bModal)
		DialogBox(g_hInst, MAKEINTRESOURCE(IDD_INFO), hParent, doInfoDialog);
	else if (*cTitle) {
		g_hwndModeless = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_INFO), hParent, doInfoDialog);
		ShowWindow(g_hwndModeless, SW_SHOW);
	}
}
