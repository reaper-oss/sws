/******************************************************************************
/ sws_about.cpp
/
/ Copyright (c) 2009 Tim Payne (SWS), Jeffos
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
#include "./reaper/localize.h"
#include "./SnM/SnM_Dlg.h"
#include "./SnM/SnM_Util.h"
#include "./Breeder/BR_Update.h"
#include "version.h"
#include "license.h"
#include "Prompt.h"


#define WHATSNEW_HTM			"%ssws_whatsnew.html"
#ifdef _WIN32
#define WHATSNEW_TXT			"%s\\Plugins\\reaper_sws_whatsnew.txt"
#else
#define WHATSNEW_TXT			"%s/UserPlugins/reaper_sws_whatsnew.txt"
#endif


void WhatsNew(COMMAND_T*)
{
	WDL_FastString fnIn;
	fnIn.SetFormatted(BUFFER_SIZE, WHATSNEW_TXT, 
#ifdef _WIN32
		GetExePath());
#else
		GetResourcePath());
#endif

	if (FileExistsErrMsg(fnIn.Get()))
	{
		WDL_FastString fnOut;
		char tmpDir[BUFFER_SIZE] = "";
#ifdef _WIN32
		GetTempPath(sizeof(tmpDir), tmpDir);
#else
		strcpy(tmpDir, "/tmp/");
#endif
		fnOut.SetFormatted(BUFFER_SIZE, WHATSNEW_HTM, tmpDir);

		if (!GenHtmlWhatsNew(fnIn.Get(), fnOut.Get(), true))
		{
/*JFB commented: fails on osx (safari), optional on windows
			fnOut.Insert("file://", 0);
*/
			ShellExecute(GetMainHwnd(), "open", fnOut.Get(), NULL, NULL, SW_SHOWNORMAL);
		}
		else
		{
			MessageBox(GetMainHwnd(), __LOCALIZE("The generation of the \"What's new?\" HTML page has failed!\nOpening a raw text file instead...","sws_DLG_109"), __LOCALIZE("SWS - Warning","sws_DLG_109"), MB_OK);
			ShellExecute(GetMainHwnd(), "open", fnIn.Get(), NULL, NULL, SW_SHOWNORMAL);
		}
	}
}

INT_PTR WINAPI doAbout(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwndDlg, uMsg, wParam, lParam))
		return r;

	if (uMsg == WM_INITDIALOG)
	{
		char cVersion[256];
		sprintf(cVersion, __LOCALIZE_VERFMT("Version %d.%d.%d Build #%d, built on %s","sws_DLG_109"), SWS_VERSION, __DATE__);
		SetWindowText(GetDlgItem(hwndDlg, IDC_VERSION), cVersion);
#ifdef _WIN64
		SetDlgItemText(hwndDlg, IDC_LATESTVER, "http://www.standingwaterstudios.com/reaper/sws_extension_x64.exe");
#endif
#ifndef _WIN32
		SetDlgItemText(hwndDlg, IDC_LATESTVER, "http://www.standingwaterstudios.com/reaper/sws_osx.dmg");
#endif
		int official, beta; GetStartupSearchOptions(hwndDlg, official, beta);
		CheckDlgButton(hwndDlg, IDC_CHECK1, !!official);
		CheckDlgButton(hwndDlg, IDC_CHECK2, !!beta);
	}
	else if (uMsg == WM_DRAWITEM)
	{
		DRAWITEMSTRUCT *di = (DRAWITEMSTRUCT *)lParam;
		if (di->CtlType == ODT_BUTTON) 
		{
			if (SWS_THEMING)
			{
				if (di->itemState & ODS_SELECTED)
					SetTextColor(di->hDC, RGB(0,0,0));
			}
			else
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
		else if (wParam == IDC_INFO)
			WhatsNew(NULL);
		else if (wParam == IDC_LICENSE)
			DisplayInfoBox(hwndDlg, __LOCALIZE("SWS/S&M Extension - License","sws_DLG_109"), LICENSE_TEXT);
		else if (wParam == IDC_UPDATE)
			VersionCheckAction(NULL);
		else if (wParam == IDCANCEL)
		{
			SetStartupSearchOptions(IsDlgButtonChecked(hwndDlg, IDC_CHECK1), IsDlgButtonChecked(hwndDlg, IDC_CHECK2));
			EndDialog(hwndDlg, 0);
		}
	}
	return 0;
}

void OpenAboutBox(COMMAND_T*)
{
	DialogBox(g_hInst,MAKEINTRESOURCE(IDD_ABOUT), g_hwndParent, doAbout);
}

//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "SWS: About" }, "SWS_ABOUT", OpenAboutBox, "About SWS Extensions", },
	{ { DEFACCEL, "SWS/S&M: What's new?" }, "S&M_WHATSNEW", WhatsNew, },
	{ { DEFACCEL, "SWS/BR: Check for new SWS version..." }, "BR_VERSION_CHECK", VersionCheckAction, },
	{ {}, LAST_COMMAND, }, // Denote end of table
};
//!WANT_LOCALIZE_1ST_STRING_END

int AboutBoxInit()
{
	SWSRegisterCommands(g_commandTable);
	return 1;
}
