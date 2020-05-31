/******************************************************************************
/ DiskSpaceCalculator.cpp
/
/ Copyright (c) 2009 Tim Payne (SWS), original code by Xenakios
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

#include "../SnM/SnM_Dlg.h"

#include <WDL/localize/localize.h>

using namespace std;

void UpdateDialogControls(HWND hwnd)
{
	// IDC_STATIC7
	int combosel=0;
	char buf[317];
	combosel=(int)SendMessage(GetDlgItem(hwnd,IDC_COMBO2), CB_GETCURSEL, 0, 0);
	SendMessage(GetDlgItem(hwnd,IDC_COMBO2), CB_GETLBTEXT, combosel, (LPARAM)buf);
	int samplerate=atoi(buf);
	combosel=(int)SendMessage(GetDlgItem(hwnd,IDC_COMBO1), CB_GETCURSEL, 0, 0);
	SendMessage(GetDlgItem(hwnd,IDC_COMBO1), CB_GETLBTEXT, combosel, (LPARAM)buf);
	int bits=atoi(buf);
	int bytemultip=bits/8;
	GetDlgItemText(hwnd,IDC_EDIT2,buf,49);
	double seconds=atof(buf)*60.0;
	GetDlgItemText(hwnd,IDC_EDIT1,buf,49);
	int numtracks=atoi(buf);
	if (numtracks<=0) numtracks=1;
	INT64 bytesrequired=(INT64)(numtracks*samplerate*bytemultip*seconds);
	double megsrequired=bytesrequired/1024.0/1024.0;
	if (megsrequired<1024)
		sprintf(buf,"%.2f MB",megsrequired);
	
	if (megsrequired>=1024)
		sprintf(buf,"%.2f GB",megsrequired/1024.0);
	SetDlgItemText(hwnd,IDC_STATIC7,buf);
	
}

WDL_DLGRET DiskCalcDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, Message, wParam, lParam))
		return r;

	if (Message==WM_INITDIALOG)
	{
		SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)"8");
		SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)"16");
		SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)"24");
		SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)"32");
		SendMessage(GetDlgItem(hwnd, IDC_COMBO2), CB_ADDSTRING, 0, (LPARAM)"22050");
		SendMessage(GetDlgItem(hwnd, IDC_COMBO2), CB_ADDSTRING, 0, (LPARAM)"24000");
		SendMessage(GetDlgItem(hwnd, IDC_COMBO2), CB_ADDSTRING, 0, (LPARAM)"44100");
		SendMessage(GetDlgItem(hwnd, IDC_COMBO2), CB_ADDSTRING, 0, (LPARAM)"48000");
		SendMessage(GetDlgItem(hwnd, IDC_COMBO2), CB_ADDSTRING, 0, (LPARAM)"88200");
		SendMessage(GetDlgItem(hwnd, IDC_COMBO2), CB_ADDSTRING, 0, (LPARAM)"96000");
		SendMessage(GetDlgItem(hwnd, IDC_COMBO2), CB_ADDSTRING, 0, (LPARAM)"176400");
		SendMessage(GetDlgItem(hwnd, IDC_COMBO2), CB_ADDSTRING, 0, (LPARAM)"192000");
		
		SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_SETCURSEL, 2, 0);
		SendMessage(GetDlgItem(hwnd, IDC_COMBO2), CB_SETCURSEL, 2, 0);
		
		SetDlgItemText(hwnd,IDC_EDIT1,"8");
		SetDlgItemText(hwnd,IDC_EDIT2,"4");
		UpdateDialogControls(hwnd);
	}
	if (Message==WM_COMMAND && (LOWORD(wParam)==IDOK || LOWORD(wParam)==IDCANCEL))
	{
		EndDialog(hwnd,0);
		return 0;
	}
	if (Message==WM_COMMAND || Message==WM_NOTIFY)
	{
		if (HIWORD(wParam)==CBN_SELCHANGE || HIWORD(wParam)==EN_CHANGE)
		{
			UpdateDialogControls(hwnd);
			return 0;
		}
	}

	return 0;
}

void DoShowDiskspaceCalc(COMMAND_T*)
{
	DialogBox(g_hInst,MAKEINTRESOURCE(IDD_DISKSPACECALC),g_hwndParent,DiskCalcDlgProc);
}
