/******************************************************************************
/ sws_about.cpp
/
/ Copyright (c) 2013 Tim Payne (SWS), Jeffos
/ https://code.google.com/p/sws-extension
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


static HWND s_hwndAbout = NULL;


int IsOfficialOrBeta()
{
	WDL_FastString v;
	v.SetFormatted(128, "%d.%d.%d #%d; ", SWS_VERSION);
	const char* p=strrchr(v.Get(),'.');
	if (p)
	{
		p++;
		if (*p == '0') return 1; // official
		else return 2; // pre-release
	}
	return 0;
}

void WhatsNew(COMMAND_T*)
{
	WDL_FastString fnIn;
	fnIn.SetFormatted(BUFFER_SIZE, WHATSNEW_TXT, 
#ifdef _WIN32
		GetExePath());
#else
		GetResourcePath());
#endif

#ifndef _WIN32 // reaper_sws_whatsnew.txt not deployed on OSX anymore, replaced with online help
	if (FileOrDirExistsErrMsg(fnIn.Get(), false))
	{
		SNM_DeleteFile(fnIn.Get(), false); // lazy cleanup
	}

	if (IsOfficialOrBeta()==2)
	{
		WDL_FastString url;
		url.SetFormatted(512, SWS_URL_BETA_WHATSNEW, SWS_VERSION);
		ShellExecute(GetMainHwnd(), "open", url.Get(), NULL, NULL, SW_SHOWNORMAL);
	}
	else
	{
		ShellExecute(GetMainHwnd(), "open", SWS_URL_WHATSNEW, NULL, NULL, SW_SHOWNORMAL);
	}
#else
	if (FileOrDirExistsErrMsg(fnIn.Get()))
	{
		WDL_FastString fnOut;
		char tmpDir[BUFFER_SIZE] = "";
		GetTempPath(sizeof(tmpDir), tmpDir);
		fnOut.SetFormatted(BUFFER_SIZE, WHATSNEW_HTM, tmpDir);

		if (!GenHtmlWhatsNew(fnIn.Get(), fnOut.Get(), true, SWS_URL))
		{
			ShellExecute(GetMainHwnd(), "open", fnOut.Get(), NULL, NULL, SW_SHOWNORMAL);
		}
		else
		{
			MessageBox(GetMainHwnd(), __LOCALIZE("The generation of the \"What's new?\" HTML page has failed!\nOpening a raw text file instead...","sws_DLG_109"), __LOCALIZE("SWS - Warning","sws_DLG_109"), MB_OK);
			ShellExecute(GetMainHwnd(), "open", fnIn.Get(), NULL, NULL, SW_SHOWNORMAL);
		}
	}
#endif
}

INT_PTR WINAPI doAbout(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwndDlg, uMsg, wParam, lParam))
		return r;

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			char cVersion[256];
			sprintf(cVersion, __LOCALIZE_VERFMT("Version %d.%d.%d Build #%d, built on %s","sws_DLG_109"), SWS_VERSION, __DATE__);
			SetWindowText(GetDlgItem(hwndDlg, IDC_VERSION), cVersion);
			SetWindowText(GetDlgItem(hwndDlg, IDC_WEBSITE), SWS_URL);	
			bool official, beta; GetStartupSearchOptions(&official, &beta, NULL);
			CheckDlgButton(hwndDlg, IDC_CHECK1, official);
			CheckDlgButton(hwndDlg, IDC_CHECK2, beta);

			s_hwndAbout = hwndDlg;
			ShowWindow(hwndDlg, SW_SHOW);
			break;
		}
		case WM_DESTROY:
			s_hwndAbout = NULL;
			break;
		case WM_DRAWITEM:
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
			break;
		}
		case WM_COMMAND:
		{
			if (wParam == IDC_WEBSITE)
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
				VersionCheckDialog(hwndDlg);
			else if (wParam == IDCANCEL) {
				SetStartupSearchOptions(!!IsDlgButtonChecked(hwndDlg, IDC_CHECK1), !!IsDlgButtonChecked(hwndDlg, IDC_CHECK2), 0);
				DestroyWindow(hwndDlg);
			}
			break;
		}
	}
	return 0;
}

void OpenAboutBox(COMMAND_T*)
{
	if (s_hwndAbout)
		DestroyWindow(s_hwndAbout);
	else 
		CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_ABOUT), g_hwndParent, doAbout);
}

int IsAboutBoxOpen(COMMAND_T*) {
	return (s_hwndAbout != NULL);
}

//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "SWS: About" }, "SWS_ABOUT", OpenAboutBox, "About SWS Extensions", NULL, IsAboutBoxOpen, },
	{ { DEFACCEL, "SWS/S&M: What's new..." }, "S&M_WHATSNEW", WhatsNew, },
	{ { DEFACCEL, "SWS/BR: Check for new SWS version..." }, "BR_VERSION_CHECK", VersionCheckAction, },
	{ {}, LAST_COMMAND, }, // Denote end of table
};
//!WANT_LOCALIZE_1ST_STRING_END

int AboutBoxInit()
{
	SWSRegisterCommands(g_commandTable);
	return 1;
}
