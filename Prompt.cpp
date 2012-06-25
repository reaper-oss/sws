/******************************************************************************
/ Prompt.cpp
/
/ Copyright (c) 2010 Tim Payne (SWS)
/ http://www.standingwaterstudios.com/reaper
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

#define PROMPTWND_KEY "PromptWindowPos"
#define INFOWND_KEY "InfoWindowPos"

static const char* g_cTitle;
static char* g_cString;
static int g_iMax;
static bool g_bOK = false;
static bool g_bAtMouse = false;

INT_PTR WINAPI doPromptDialog(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwndDlg, uMsg, wParam, lParam))
		return r;

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			SetWindowText(hwndDlg, g_cTitle);
			HWND hEdit = GetDlgItem(hwndDlg, IDC_EDIT);
			SetWindowText(hEdit, g_cString);
			if (g_bAtMouse)
				SetWindowPosAtMouse(hwndDlg);
			else
				RestoreWindowPos(hwndDlg, PROMPTWND_KEY, false);
			return 0;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
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
			resize.init(hwndDlg);
			resize.init_item(IDC_EDIT);
			SetWindowText(hwndDlg, g_cTitle);
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
					EndDialog(hwndDlg, 0);
					break;
			}
			break;
	}
	return 0;
}


bool PromptUserForString(HWND hParent, const char* cTitle, char* cString, int iMaxChars, bool bAtMouse)
{
	g_cTitle = cTitle;
	g_cString = cString;
	g_iMax = iMaxChars;
	g_bOK = false;
	g_bAtMouse = bAtMouse;
	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_PROMPT), hParent, doPromptDialog);
	return g_bOK;
}

void DisplayInfoBox(HWND hParent, const char* cTitle, const char* cInfo)
{
	g_cTitle = cTitle;
	g_cString = (char*)cInfo;
	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_INFO), hParent, doInfoDialog);
}