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
		sprintf(cVersion, "Version %d.%d.%d Build #%d, built on %s", PRODUCT_VERSION, __DATE__);
		SetWindowText(GetDlgItem(hwndDlg, IDC_VERSION), cVersion);
	}
#if defined(_WIN32) && (_MSC_VER > 1310)
	else if (uMsg == WM_NOTIFY)
	{
		NMLINK* s = (NMLINK*)lParam;
		if (s->hdr.code == NM_CLICK)// && s->hdr.idFrom == IDC_LATESTVER)
		{
			char cLink[256];
			GetWindowText(GetDlgItem(hwndDlg, (int)s->hdr.idFrom), cLink, 256);
			// Strip out leading/trailing <a> </a>
			*strrchr(cLink,'<') = 0;
			ShellExecute(hwndDlg, "open", strchr(cLink,'>')+1, NULL, NULL, SW_SHOWNORMAL);
		}
	}
#endif
	else if (uMsg == WM_COMMAND)
	{
		if (wParam == IDC_LICENSE)
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
	{ { DEFACCEL, "SWS: About..." }, "SWS_ABOUT", OpenAboutBox, "About SWS Extensions...", },
	{ {}, LAST_COMMAND, }, // Denote end of table
};

static void menuhook(int menuid, HMENU hMenu, int flag)
{
	if (menuid == MAINMENU_EXT && flag == 0)
		AddToMenu(hMenu, g_commandTable[0].menuText, g_commandTable[0].accel.accel.cmd);
}

int AboutBoxInit()
{
	SWSRegisterCommands(g_commandTable);

	if (!plugin_register("hookmenu", (void*)menuhook))
		return 0;

	return 1;
}
