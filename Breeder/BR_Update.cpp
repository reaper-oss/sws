/******************************************************************************
/ BR_Update.cpp
/
/ Copyright (c) 2013 Dominik Martin Drzic
/ http://forum.cockos.com/member.php?u=27094
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
#include "BR_Update.h"
#include "BR_Util.h"
#include "../version.h"
#include "../../WDL/queue.h"
#include "../../WDL/jnetlib/jnetlib.h"
#include "../../WDL/jnetlib/httpget.h"
#include "../reaper/localize.h"

#define VERSION_CHECK_KEY	"BR - StartupVersionCheck"
#define VERSION_TIME_KEY	"BR - StartupVersionLastCheck"

// Globals
static time_t g_searchTimeOut = 5;			// timeout for web search (default for startup, changing to 10 on user request)
static bool g_startupSearch = true;			// startup search is turned on by the default
static bool g_startupSearchFinished = true;	// notifies if startup search was performed

static bool g_searching = false;			// notifies if search is running or not
static double g_searchProgress = 0;			// progress of ongoing search 
static int g_versionStatus = -2;			// status of local version (see VersionCheck for codes)

// Search function and GUI
//////////////////////////////////////////////////////////////////////////////////////////
void VersionCheck ()
{
	// g_versionStatus codes:  -2 -> No results yet
	//						   -1 -> Error (no internet, code.google down?)
	//							0 -> SWS is up to date 
	//							1 -> Update is available

	// Connect to google code
	char* buf = NULL;
	JNL_HTTPGet web;
	web.addheader("User-Agent:SWS (Mozilla)");
	web.addheader("Accept:*/*");
	web.connect("http://sws-extension.googlecode.com/svn/trunk/version.h");

	// Get remote version.h
	time_t startTime = time(NULL);
	while (time(NULL) - startTime <= g_searchTimeOut && g_searching)
	{
		// Set progress bar used in dialog
		g_searchProgress = ((double)(time(NULL) - startTime)) / (double) g_searchTimeOut;
	
		// Try to get file
		int run=web.run();
		if (web.get_status() < 0 || web.getreplycode() >= 400)
			break;
		if (run >= 0)
		{
			if (run == 1 && web.getreplycode() == 200) // when using only reply code, data was incomplete
			{										   // so get data only after the connection has closed
				int size = web.bytes_available();
				buf = new (nothrow) char[size];
				web.get_bytes(buf, size);
				g_searchProgress = 1; // when version.h is received make progress 100%
				break;
			}
		}
		else
			break;
	}

	// Couldn't get anything. Set status and notify that search is finished
	if (buf == NULL)
	{
		g_versionStatus = -1;
		g_searching = false;
		return;
	}

	// Get remote version number
	char remote[256];
	char* token = strtok(buf, "\n");
	while (token != NULL)
	{
		if (sscanf(token, "#define SWS_VERSION %s", remote)>0)
			break;
		token = strtok(NULL, "\n");
	}

	// Get local version number
	char local[256];
	_snprintf(local, sizeof(local), "%d,%d,%d,%d", SWS_VERSION);

	// Compare versions	and set status
	if(strcmp(local, remote) == 0)
		g_versionStatus = 0; // up to date
	else		  
		g_versionStatus = 1; // new version available

	// Notify that search has finished.
	g_searching = false;
	if (buf != NULL)
		delete buf;
};

DWORD WINAPI VersionCheckThread(void*)
{
	VersionCheck();
	return 0;
}

WDL_DLGRET VersionCheckProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			// Search requested at startup. WM_SHOWWINDOW will not be executed
			// because dialog is not shown until search is finished so we start
			// search here
			if (!g_startupSearchFinished)
			{
				CenterWindowInReaper(hwnd, HWND_TOPMOST, true);
				g_searching = true;				
				SetTimer(hwnd, 1, 100, NULL);
				CreateThread(NULL, 0, VersionCheckThread, 0, 0, NULL);
			}
		}
		break;

		case WM_SHOWWINDOW:
		{
			// Search requested by user (startup search will not send show/hide message unless finished and update is found - next if should take care of that)
			if (LOWORD(wParam))
			{
				if (g_startupSearchFinished)
				{
					// Prepare dialog
					KillTimer (hwnd, 1);
					CenterWindowInReaper(hwnd, HWND_TOPMOST, false);
					SendMessage(GetDlgItem(hwnd, IDC_BR_VER_PROGRESS), PBM_SETPOS, 0, 0); 
					ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_PROGRESS), SW_SHOW);
					SetWindowText(GetDlgItem(hwnd, IDOK),__LOCALIZE("Proceed to download page","sws_DLG_172"));
					EnableWindow(GetDlgItem(hwnd, IDOK), false);

					// Set up variables and start search
					g_searchTimeOut = 10;
					g_versionStatus = -2;
					g_searching = true;
					SetTimer(hwnd, 1, 100, NULL);
					CreateThread(NULL, 0, VersionCheckThread, 0, 0, NULL);
				}
			}

			// Make sure everything is cleaned up when hiding window
			else
			{	
				KillTimer (hwnd, 1);
				g_searching = false;
				g_startupSearchFinished = true; 
				g_versionStatus = -2;
			}
		}
		break;
		
		case WM_TIMER:
		{
			// Search requested at startup
			if(!g_startupSearchFinished)
			{
				if (!g_searching)
				{	
					// Notify user only if new version has been found
					if (g_versionStatus == 1)
					{
						ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_PROGRESS), SW_HIDE);
						SetWindowText(GetDlgItem(hwnd, IDC_BR_VER_MESSAGE),__LOCALIZE("New version of SWS extensions is available.","sws_DLG_172"));
						EnableWindow(GetDlgItem(hwnd, IDOK), true);
						ShowWindow(hwnd, SW_SHOW);
					}
					KillTimer (hwnd, 1);
					g_versionStatus = -2;
					g_startupSearchFinished = true;
				}
			}

			// Search requested by user
			else
			{
				if (g_searching)
					SendMessage(GetDlgItem(hwnd, IDC_BR_VER_PROGRESS), PBM_SETPOS, (int)(g_searchProgress * 100.0), 0);
				else
				{	
					// Inform user about version status
					if (g_versionStatus <= -1)
					{						
						SetWindowText(GetDlgItem(hwnd, IDC_BR_VER_MESSAGE),__LOCALIZE("Connection could not be established.","sws_DLG_172"));
						SetWindowText(GetDlgItem(hwnd, IDOK),__LOCALIZE("Retry","sws_DLG_172"));
						EnableWindow(GetDlgItem(hwnd, IDOK), true);
					}
					else if (g_versionStatus == 0)
						SetWindowText(GetDlgItem(hwnd, IDC_BR_VER_MESSAGE),__LOCALIZE("SWS extension is up to date.","sws_DLG_172"));
					else if (g_versionStatus == 1)
					{
						SetWindowText(GetDlgItem(hwnd, IDC_BR_VER_MESSAGE),__LOCALIZE("New version of SWS extensions is available.","sws_DLG_172"));
						EnableWindow(GetDlgItem(hwnd, IDOK), true);
					}					
					ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_PROGRESS), SW_HIDE);
					KillTimer (hwnd, 1);   			
				}
			}
		}
		break;

		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case IDOK:
				{
					// Retry
					if (g_versionStatus == -1)
					{
						ShowWindow(hwnd, SW_HIDE);
						ShowWindow(hwnd, SW_SHOW);
					}
					// Proceed to download page
					else
					{
						ShellExecute(NULL, "open", "http://code.google.com/p/sws-extension/downloads/list" , NULL, NULL, SW_SHOWNORMAL);
						ShowWindow(hwnd, SW_HIDE);
					}
				}	
				break;
				
				case IDCANCEL:
					ShowWindow(hwnd, SW_HIDE);
				break;
			} 
		}
		break;
	}
	return 0;
};

void VersionCheckDlg(bool show)
{
	static HWND hwnd = CreateDialog (g_hInst, MAKEINTRESOURCE(IDD_BR_VERSION), g_hwndParent, VersionCheckProc);
	if (show)
		ShowWindow(hwnd, SW_SHOW);
}

// Commands
//////////////////////////////////////////////////////////////////////////////////////////
bool IsVersionCheckEnabled(COMMAND_T* = NULL)
{
	return g_startupSearch;
}

void VersionCheckEnable(COMMAND_T* ct)
{
	g_startupSearch =  !g_startupSearch;
	WritePrivateProfileString("SWS", VERSION_CHECK_KEY, g_startupSearch ? "1" : "0", get_ini_file());
}

void VersionCheckAction(COMMAND_T* ct)
{
	if (!g_startupSearchFinished) // stop ongoing startup search if happening (lol...what a corner case)
	{
		g_searching = false;
		g_startupSearchFinished = true;
		g_versionStatus = -2;
	}
	VersionCheckDlg(true);		
}

// Startup function
//////////////////////////////////////////////////////////////////////////////////////////
void VersionCheckInit()
{
	g_startupSearch = GetPrivateProfileInt("SWS", VERSION_CHECK_KEY, g_startupSearch, get_ini_file()) ? true : false;
	if (g_startupSearch)
	{
		unsigned int lastTime, currentTime = (unsigned int) time(NULL);
		char tmp[256];
		GetPrivateProfileString("SWS", VERSION_TIME_KEY, "0", tmp, 256, get_ini_file());
		sscanf(tmp, "%u", &lastTime);
		
		// Make sure at least 24 hours have passed since last search
		if (currentTime - lastTime >= 86400 || currentTime - lastTime < 0)
		{
			// Write current time
			char tmp[256];
			_snprintf(tmp, sizeof(tmp), "%u", currentTime);
			WritePrivateProfileString("SWS", VERSION_TIME_KEY, tmp, get_ini_file());

			// Start search
			g_startupSearchFinished = false;
			VersionCheckDlg(false);
		}
	}
};