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
#include "../SnM/SnM_Dlg.h"
#include "../version.h"
#include "../../WDL/jnetlib/jnetlib.h"
#include "../../WDL/jnetlib/httpget.h"
#include "../reaper/localize.h"

#define STARTUP_VERSION_KEY		"BR - StartupVersionCheck"
#define OFFICIAL_VERSION_URL	"http://sws-extension.googlecode.com/svn/tags/release/version.h"
#define BETA_VERSION_URL		"http://sws-extension.googlecode.com/svn/trunk/version.h"

// General settings
static time_t g_searchTimeOut = 5;			// timeout for web search (separate for beta and official)
static bool g_searchOfficial;				// by default official version search is performed on startup
static bool g_searchBeta;					// while beta search is not (loaded in VersionCheckInit())

// Thread management
static HANDLE g_threadHandle = NULL;		// Always use StartStopThread to prevent from spawning multiple threads
static bool g_killThread = false;

// Used for searching
static int g_status = -2;					// see SetVersionMessage() for codes
static double g_progress = 0;
static bool g_startupDone = true;
static bool g_searching = false;

// Info on remote versions
static BR_Version g_versionO;				// official
static BR_Version g_versionB;				// beta

// Dialog parent (needed for About window)
static HWND g_searchParent;					// Assigned in VersionChecInit() and changed by options functions (when calling from About dialog let it be parent)
											// Why? Search dialog offers "Retry" functionality so this will let new window get created with the right parent

WDL_DLGRET StartupProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Search function
//////////////////////////////////////////////////////////////////////////////////////////
int CompareVersion(BR_Version one, BR_Version two)
{
	// return 0 if equal
	//		  1 if one is greater
	//		  2 if two is greater
	if (one.maj > two.maj)
		return 1;
	else if (one.maj < two.maj)
		return 2;

	if (one.min > two.min)
		return 1;
	else if (one.min < two.min)
		return 2;

	if (one.rev > two.rev)
		return 1;
	else if (one.rev < two.rev)
		return 2;

	if (one.build > two.build)
		return 1;
	else if (one.build < two.build)
		return 2;
	return 0;
};

void VersionCheck ()
{
	// Get local version
	BR_Version versionL;				
	sscanf(SWS_VERSION_STR, "%d,%d,%d,%d", &versionL.maj, &versionL.min, &versionL.rev, &versionL.build);
	
	// When searching by user request lookup both official and beta
	if (g_startupDone)
	{
		g_searchOfficial = true;
		g_searchBeta = true;
	}

	// Get official version and compare
	int statusO = -1;
	if (g_searchOfficial && !g_killThread)
	{
		JNL_HTTPGet web;
		web.addheader("User-Agent:SWS (Mozilla)");
		web.addheader("User-Agent:SWS (Mozilla)");
		web.connect(OFFICIAL_VERSION_URL);
		char* buf;

		time_t startTime = time(NULL);
		while (time(NULL) - startTime <= g_searchTimeOut && !g_killThread)
		{
			// Set progress bar used in dialog
			g_progress = ((double)(time(NULL) - startTime)) / (((double) g_searchTimeOut)*2);

			// Try to get version.h
			int run=web.run();
			if (web.get_status() < 0 || web.getreplycode() >= 400)
				break;
			if (run >= 0)
			{
				if (run == 1 && web.getreplycode() == 200) //  get data only after the connection has closed
				{
					// Read version.h into the buffer
					int size = web.bytes_available();
					buf = new (nothrow) char[size];
					web.get_bytes(buf, size);
					
					// Get official version number
					g_versionO = BR_Version(); // reset to 0
					char* token = strtok(buf, "\n");
					while (token != NULL)
					{
						if (sscanf(token, "#define SWS_VERSION %d,%d,%d,%d", &g_versionO.maj, &g_versionO.min, &g_versionO.rev, &g_versionO.build)>0)
							break;
						token = strtok(NULL, "\n");
					}
					delete buf;

					// Compare with local version
					int compare = CompareVersion(g_versionO, versionL);
					if (compare == 0 || compare == 2)
						statusO = 0;
					else
						statusO = 1;

					// Get out!
					break;
				}
			}
			else
				break;
		}
	}

	// Get beta version and compare
	int statusB = -1;
	if (g_searchBeta && !g_killThread)
	{
		JNL_HTTPGet web;
		web.addheader("User-Agent:SWS (Mozilla)");
		web.addheader("User-Agent:SWS (Mozilla)");
		web.connect(BETA_VERSION_URL);
		char* buf;
		
		time_t startTime = time(NULL);
		while (time(NULL) - startTime <= g_searchTimeOut && !g_killThread)
		{
			// Set progress bar used in dialog
			g_progress = 0.5 + ((double)(time(NULL) - startTime)) / (((double) g_searchTimeOut)*2);

			// Try to get version.h
			int run=web.run();
			if (web.get_status() < 0 || web.getreplycode() >= 400)
				break;
			if (run >= 0)
			{
				if (run == 1 && web.getreplycode() == 200) //  get data only after the connection has closed
				{
					// Read version.h into the buffer
					int size = web.bytes_available();
					buf = new (nothrow) char[size];
					web.get_bytes(buf, size);
					
					// Get official version number
					g_versionB = BR_Version(); // reset to 0
					char* token = strtok(buf, "\n");
					while (token != NULL)
					{
						if (sscanf(token, "#define SWS_VERSION %d,%d,%d,%d", &g_versionB.maj, &g_versionB.min, &g_versionB.rev, &g_versionB.build)>0)
							break;
						token = strtok(NULL, "\n");
					}
					delete buf;

					// Compare with local version
					int compare = CompareVersion(g_versionB, versionL);
					if (compare == 0 || compare == 2)
						statusB = 0;
					else
						statusB = 1;

					// Get out!
					break;
				}
			}
			else
				break;
		}
	}

	// Make progress bar 100% when done searching
	g_progress = 1;
	
	// Set status according to comparison results
	if (!g_killThread)
	{
		if (statusO == -1 && statusB == -1) 
			g_status = -1;
		else if (statusO == 1 && statusB == 1)
		{
			if (CompareVersion(g_versionB, g_versionO) == 0)
				g_status = 1;
			else
				g_status = 3;
		}
		else if (statusO == 1)
			g_status = 1;
		else if (statusB == 1)
			g_status = 2;		
		else
			g_status = 0;
	}

	// Create modal (to prevent thread from finishing and ending dialog) dialog but only when doing startup search and update is found
	// It has no parent so it's visible in task bar and does not stop user in interacting with reaper in anyway
	if (!g_killThread && !g_startupDone && g_status >= 1)
	{
		DialogBox (g_hInst, MAKEINTRESOURCE(IDD_BR_VERSION), NULL, StartupProc);	

	}
	g_searching = false;
	g_startupDone = true;
};

DWORD WINAPI VersionCheckThread(void*)
{
	VersionCheck();
	return 0;
}

void StartStopThread (bool start)
{
	// Make running thread finish
	if (g_threadHandle != NULL)
	{
		g_killThread = true;
		WaitForSingleObject(g_threadHandle,INFINITE);
		CloseHandle(g_threadHandle);
		g_threadHandle = NULL;
	}

	// Reset variables
	g_progress = 0;
	g_status = -2;
	g_searching = false;

	// Start new search thread if requested
	if (start)
	{
		g_killThread = false;
		g_searching = true;
		g_threadHandle = CreateThread(NULL, 0, VersionCheckThread, 0, 0, NULL);
	}
}

// GUI
//////////////////////////////////////////////////////////////////////////////////////////
void SetVersionMessage(HWND hwnd, int status)
{
	char tmp[256] = {0};
	
	// Search initialized 
	if (status == -2)
	{
		EnableWindow(GetDlgItem(hwnd, IDC_BR_VER_DOWNLOAD), false);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_PROGRESS), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_DOWNLOAD), SW_SHOW);
	}

	// No connection
	else if (status == -1) 
	{
		_snprintf(tmp, sizeof(tmp), __LOCALIZE("Connection could not be established","sws_DLG_172"));
		SetWindowText(GetDlgItem(hwnd, IDC_BR_VER_DOWNLOAD), __LOCALIZE("Retry","sws_DLG_172"));
		EnableWindow(GetDlgItem(hwnd, IDC_BR_VER_DOWNLOAD), true);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_DOWNLOAD), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_OFF), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_BETA), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_PROGRESS), SW_HIDE);
	}

	// Up to date
	else if (status == 0)
	{
		_snprintf(tmp, sizeof(tmp), __LOCALIZE("SWS extension is up to date.","sws_DLG_172"));
		EnableWindow(GetDlgItem(hwnd, IDC_BR_VER_DOWNLOAD), false);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_DOWNLOAD), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_OFF), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_BETA), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_PROGRESS), SW_HIDE);
	}

	// Official update available
	else if (status == 1)
	{
		_snprintf(tmp, sizeof(tmp), __LOCALIZE("Official update is available: Version %d.%d.%d Build #%d","sws_DLG_172"), g_versionO.maj, g_versionO.min, g_versionO.rev, g_versionO.build);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_DOWNLOAD), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_OFF), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_BETA), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_PROGRESS), SW_HIDE);
	}
	
	// Beta update available
	else if (status == 2)
	{
		_snprintf(tmp, sizeof(tmp), __LOCALIZE("Beta update is available: Version %d.%d.%d Build #%d","sws_DLG_172"), g_versionB.maj, g_versionB.min, g_versionB.rev, g_versionB.build);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_DOWNLOAD), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_OFF), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_BETA), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_PROGRESS), SW_HIDE);
	}

	// Both official and beta updates available
	else if (status == 3)
	{
		_snprintf(tmp, sizeof(tmp), __LOCALIZE("Official update is available: Version %d.%d.%d Build #%d\nBeta update is available: Version %d.%d.%d Build #%d","sws_DLG_172"), g_versionO.maj, g_versionO.min, g_versionO.rev, g_versionO.build, g_versionB.maj, g_versionB.min, g_versionB.rev, g_versionB.build);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_OFF), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_BETA), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_DOWNLOAD), SW_HIDE);
		ShowWindow(GetDlgItem(hwnd, IDC_BR_VER_PROGRESS), SW_HIDE);
	}

	SetWindowText(GetDlgItem(hwnd, IDC_BR_VER_MESSAGE), tmp);
};

WDL_DLGRET CommandProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, uMsg, wParam, lParam))
		return r;
	
	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			CenterWindowInReaper (hwnd, HWND_TOPMOST, false);
			StartStopThread (true);
			SetVersionMessage (hwnd, g_status);
			SetTimer(hwnd, 1, 100, NULL);

		}
		break;

		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case IDC_BR_VER_DOWNLOAD:
				{
					if (g_status <= -1)
					{
						EndDialog(hwnd, 0);
						VersionCheckAction(NULL);
					}
					if (g_status == 1)
					{
						ShellExecute(NULL, "open", "http://www.standingwaterstudios.com/new.php" , NULL, NULL, SW_SHOWNORMAL);
						EndDialog(hwnd, 0);
					}
					else if (g_status == 2)
					{
						ShellExecute(NULL, "open", "http://code.google.com/p/sws-extension/downloads/list" , NULL, NULL, SW_SHOWNORMAL);
						EndDialog(hwnd, 0);
					}

				}	
				break;

				case IDC_BR_VER_OFF:
				{
					ShellExecute(NULL, "open", "http://www.standingwaterstudios.com/new.php" , NULL, NULL, SW_SHOWNORMAL);
					EndDialog(hwnd, 0);
				}
				break;
				
				case IDC_BR_VER_BETA:
				{
					ShellExecute(NULL, "open", "http://code.google.com/p/sws-extension/downloads/list" , NULL, NULL, SW_SHOWNORMAL);
					EndDialog(hwnd, 0);
				}
				break;

				case IDCANCEL:
				{
					KillTimer(hwnd, 1);
					EndDialog(hwnd, 0);										
				}
				break;
			} 
		}
		break;

		case WM_TIMER:
		{
			SendMessage(GetDlgItem(hwnd, IDC_BR_VER_PROGRESS), PBM_SETPOS, (int)(g_progress * 100.0), 0);
			if (!g_searching)
			{
				SetVersionMessage(hwnd, g_status);
				KillTimer(hwnd, 1);
			}


		}
		break;
	}
	return 0;
};

WDL_DLGRET StartupProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, uMsg, wParam, lParam))
		return r;
	
	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			CenterWindowInReaper (hwnd, HWND_TOPMOST, false);
			SetVersionMessage (hwnd, g_status);
			SetTimer(hwnd, 1, 100, NULL);

		}
		break;

		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case IDC_BR_VER_DOWNLOAD:
				{
					if (g_status == 1)
					{
						ShellExecute(NULL, "open", "http://www.standingwaterstudios.com/new.php" , NULL, NULL, SW_SHOWNORMAL);
						EndDialog(hwnd, 0);
					}
					else if (g_status == 2)
					{
						ShellExecute(NULL, "open", "http://code.google.com/p/sws-extension/downloads/list" , NULL, NULL, SW_SHOWNORMAL);
						EndDialog(hwnd, 0);
					}

				}	
				break;

				case IDC_BR_VER_OFF:
				{
					ShellExecute(NULL, "open", "http://www.standingwaterstudios.com/new.php" , NULL, NULL, SW_SHOWNORMAL);
					EndDialog(hwnd, 0);
				}
				break;
				
				case IDC_BR_VER_BETA:
				{
					ShellExecute(NULL, "open", "http://code.google.com/p/sws-extension/downloads/list" , NULL, NULL, SW_SHOWNORMAL);
					EndDialog(hwnd, 0);
				}
				break;

				case IDCANCEL:
				{
					EndDialog(hwnd, 0);
				}
				break;
			} 
		}
		break;

		case WM_TIMER:
		{
			if (g_killThread) // make sure dialog gets closed and exits current thread if StartStopThread is called
				EndDialog (hwnd, 0);
		}
		break;
	}
	return 0;
};

// Command and functions for ABOUT dialog (they change dialog parent)
//////////////////////////////////////////////////////////////////////////////////////////
void VersionCheckAction(COMMAND_T* ct)
{
	StartStopThread (false);
	DialogBox (g_hInst, MAKEINTRESOURCE(IDD_BR_VERSION), g_searchParent, CommandProc);
}

void GetStartupSearchOptions(HWND hwnd, int &official, int &beta)
{
	char tmp[256];
	GetPrivateProfileString("SWS", STARTUP_VERSION_KEY, "1 0 0", tmp, 256, get_ini_file());
	sscanf(tmp, "%d %d", &official, &beta);

	g_searchParent = hwnd;
}

void SetStartupSearchOptions(int official, int beta)
{
	char tmp[256]; unsigned int lastTime;
	GetPrivateProfileString("SWS", STARTUP_VERSION_KEY, "1 0 0", tmp, 256, get_ini_file());
	sscanf(tmp, "%*d %*d %d", &lastTime);
	_snprintf(tmp, sizeof(tmp), "%d %d %u", official, beta, lastTime);
	WritePrivateProfileString("SWS", STARTUP_VERSION_KEY, tmp, get_ini_file());

	g_searchParent = g_hwndParent;
}

// Startup function
//////////////////////////////////////////////////////////////////////////////////////////
void VersionCheckInit()
{	
	// See note at the start of file
	g_searchParent = g_hwndParent;

	// Get options
	char tmp[256]; int official, beta; unsigned int lastTime;
	GetPrivateProfileString("SWS", STARTUP_VERSION_KEY, "1 0 0", tmp, 256, get_ini_file());
	sscanf(tmp, "%d %d %u", &official, &beta, &lastTime);
	
	g_searchOfficial = official ? true : false;
	g_searchBeta = beta ? true : false;

	if (g_searchOfficial || g_searchBeta)
	{
		// Make sure at least 24 hours have passed since last search
		unsigned int currentTime = (unsigned int)time(NULL);
		if (currentTime - lastTime >= 86400 || currentTime - lastTime < 0)
		{
			// Write current time
			char tmp[256];
			_snprintf(tmp, sizeof(tmp), "%d %d %u", !!g_searchOfficial, !!g_searchBeta, currentTime);
			WritePrivateProfileString("SWS", STARTUP_VERSION_KEY, tmp, get_ini_file());

			// Start search
			g_startupDone = false;
			StartStopThread (true);
		}
	}
};