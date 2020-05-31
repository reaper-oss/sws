/******************************************************************************
/ RecCheck.cpp
/
/ Copyright (c) 2011 Tim Payne (SWS)
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

// Actions that behave differently based on the cursor context

#include "stdafx.h"

#include "../SnM/SnM_Dlg.h"
#include "RecCheck.h"

#include <WDL/localize/localize.h>

static bool g_bEnRecInputCheck = false;
#define RECINPUTCHECK_KEY "Record input check"
#define RECINPUTWNDPOS_KEY "RecInputCheckWndPos"

INT_PTR WINAPI doRecInputDialog(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwndDlg, uMsg, wParam, lParam))
		return r;

	switch(uMsg)
	{
	case WM_INITDIALOG:
		CheckDlgButton(hwndDlg, IDC_DONTSHOW, !g_bEnRecInputCheck ? BST_CHECKED : BST_UNCHECKED);
		RestoreWindowPos(hwndDlg, RECINPUTWNDPOS_KEY, false);
		return 0;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			g_bEnRecInputCheck = IsDlgButtonChecked(hwndDlg, IDC_DONTSHOW) ? false : true;
			WritePrivateProfileString(SWS_INI, RECINPUTCHECK_KEY, g_bEnRecInputCheck ? "1" : "0", get_ini_file());
			SaveWindowPos(hwndDlg, RECINPUTWNDPOS_KEY);
			EndDialog(hwndDlg, (INT_PTR)(LOWORD(wParam)));
			break;
		}
	}
	return 0;
}

// Return true for "continue recording"
bool RecordInputCheck()
{
	if (!g_bEnRecInputCheck)
		return true;

	bool bDupe = false;
	// Check all the armed track's rec inputs for dupes
	WDL_TypedBuf<int> inputs;
	for (int i = 1; !bDupe && i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_RECARM", NULL) && *(int*)GetSetMediaTrackInfo(tr, "I_RECMODE", NULL) != 2)
		{
			int iInput = *(int*)GetSetMediaTrackInfo(tr, "I_RECINPUT", NULL);
			// Ignore < 0 inputs
			if (iInput >= 0)
			{
				for (int j = 0; j < inputs.GetSize(); j++)
				{
					if (inputs.Get()[j] == iInput)
					{
						bDupe = true;
						break;
					}
				}
				if (!bDupe)
				{
					int size = inputs.GetSize();
					inputs.Resize(size+1);
					inputs.Get()[size] = iInput;
				}
			}
		}
	}
	if (bDupe)
	{	// Display the dlg
		INT_PTR iRet = DialogBox(g_hInst, MAKEINTRESOURCE(IDD_RECINPUTCHECK), g_hwndParent, doRecInputDialog);
		if (iRet == IDCANCEL)
			return false;
	}
	return true;
}

static void SetRecordInputCheck(COMMAND_T* ct)
{
	switch(ct->user)
	{
		case -1: g_bEnRecInputCheck = !g_bEnRecInputCheck; break;
		case 0:  g_bEnRecInputCheck = false; break;
		case 1:  g_bEnRecInputCheck = true;  break;
	}
	WritePrivateProfileString(SWS_INI, RECINPUTCHECK_KEY, g_bEnRecInputCheck ? "1" : "0", get_ini_file());
}

int IsRecInputChecked(COMMAND_T*)
{
	return g_bEnRecInputCheck;
}

//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "SWS: Enable checking for duplicate inputs when recording" },		"SWS_ENRECINCHECK",		SetRecordInputCheck, NULL, 1, },
	{ { DEFACCEL, "SWS: Disable checking for duplicate inputs when recording" },	"SWS_DISRECINCHECK",	SetRecordInputCheck, NULL, 0, },
	{ { DEFACCEL, "SWS: Toggle checking for duplicate inputs when recording" },		"SWS_TOGRECINCHECK",	SetRecordInputCheck, NULL, -1, IsRecInputChecked },
	{ {}, LAST_COMMAND, }, // Denote end of table
};
//!WANT_LOCALIZE_1ST_STRING_END

int RecordCheckInit()
{
	SWSRegisterCommands(g_commandTable);
	g_bEnRecInputCheck =  GetPrivateProfileInt(SWS_INI, RECINPUTCHECK_KEY, 1, get_ini_file()) ? true : false;
	return 1;
}
