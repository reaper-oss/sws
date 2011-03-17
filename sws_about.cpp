/******************************************************************************
/ sws_about.cpp
/
/ Copyright (c) 2009 Tim Payne (SWS)
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

// Display the about box
#include "stdafx.h"
#include "version.h"
#include "license.h"

INT_PTR WINAPI doAbout(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_INITDIALOG)
	{
		char cVersion[256];
		sprintf(cVersion, "Version %d.%d.%d Build #%d, built on %s", SWS_VERSION, __DATE__);
		SetWindowText(GetDlgItem(hwndDlg, IDC_VERSION), cVersion);
#ifdef _WIN64
		SetDlgItemText(hwndDlg, IDC_LATESTVER, "http://www.standingwaterstudios.com/reaper/sws_extension_x64.exe");
#endif
#ifndef _WIN32
		SetDlgItemText(hwndDlg, IDC_LATESTVER, "http://www.standingwaterstudios.com/reaper/sws_osx.dmg");
#endif
		
	}
	else if (uMsg == WM_DRAWITEM)
	{
		DRAWITEMSTRUCT *di = (DRAWITEMSTRUCT *)lParam;
		if (di->CtlType == ODT_BUTTON) 
		{
			SetTextColor(di->hDC, (di->itemState & ODS_SELECTED) ? RGB(0,0,0) : RGB(0,0,220));
			RECT r = di->rcItem;
			char buf[512];
			GetWindowText(di->hwndItem, buf, sizeof(buf));
			DrawText(di->hDC, buf, -1, &r, DT_NOPREFIX | DT_LEFT | DT_VCENTER);
		}
	}
	else if (uMsg == WM_COMMAND)
	{
		if (wParam == IDC_WEBSITE || wParam == IDC_LATESTVER)
		{
			char cLink[512];
			GetDlgItemText(hwndDlg, (int)wParam, cLink, 512);
			ShellExecute(hwndDlg, "open", cLink , NULL, NULL, SW_SHOWNORMAL);
		}
		else if (wParam == IDC_LICENSE)
			MessageBox(hwndDlg, LICENSE_TEXT, "SWS License", MB_OK);
		else if (wParam == IDCANCEL)
			EndDialog(hwndDlg, 0);
	}
	return 0;
}

void OpenAboutBox(COMMAND_T*)
{
	DialogBox(g_hInst,MAKEINTRESOURCE(IDD_ABOUT), g_hwndParent, doAbout);
}

static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "SWS: About" }, "SWS_ABOUT", OpenAboutBox, "About SWS Extensions", },
	{ {}, LAST_COMMAND, }, // Denote end of table
};

static void menuhook(const char* menustr, HMENU hMenu, int flag)
{
	if (strcmp(menustr, "Main extensions") == 0 && flag == 0)
		AddToMenu(hMenu, g_commandTable[0].menuText, g_commandTable[0].accel.accel.cmd);
}

int AboutBoxInit()
{
	SWSRegisterCommands(g_commandTable);

#ifdef _SWS_MENU
	if (!plugin_register("hookcustommenu", (void*)menuhook))
		return 0;
#endif
	return 1;
}
