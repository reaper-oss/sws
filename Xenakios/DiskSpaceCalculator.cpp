/******************************************************************************
/ DiskSpaceCalculator.cpp
/
/ Copyright (c) 2009 Tim Payne (SWS), original code by Xenakios
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

using namespace std;

void UpdateDialogControls(HWND hwnd)
{
	// IDC_STATIC7
	int combosel=0;
	char buf[50];
	combosel=ComboBox_GetCurSel(GetDlgItem(hwnd,IDC_COMBO2));
	ComboBox_GetLBText(GetDlgItem(hwnd,IDC_COMBO2),combosel,buf);
	//GetDlgItemText(hwnd,IDC_COMBO2,buf,49);
	int samplerate=atoi(buf);
	combosel=ComboBox_GetCurSel(GetDlgItem(hwnd,IDC_COMBO1));
	ComboBox_GetLBText(GetDlgItem(hwnd,IDC_COMBO1),combosel,buf);
	//GetDlgItemText(hwnd,IDC_COMBO1,buf,49);
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
	if (Message==WM_INITDIALOG)
	{
		ComboBox_AddString(GetDlgItem(hwnd,IDC_COMBO1),"8");
		ComboBox_AddString(GetDlgItem(hwnd,IDC_COMBO1),"16");
		ComboBox_AddString(GetDlgItem(hwnd,IDC_COMBO1),"24");
		ComboBox_AddString(GetDlgItem(hwnd,IDC_COMBO1),"32");
		ComboBox_AddString(GetDlgItem(hwnd,IDC_COMBO2),"22050");
		ComboBox_AddString(GetDlgItem(hwnd,IDC_COMBO2),"24000");
		ComboBox_AddString(GetDlgItem(hwnd,IDC_COMBO2),"44100");
		ComboBox_AddString(GetDlgItem(hwnd,IDC_COMBO2),"48000");
		ComboBox_AddString(GetDlgItem(hwnd,IDC_COMBO2),"88200");
		ComboBox_AddString(GetDlgItem(hwnd,IDC_COMBO2),"96000");
		ComboBox_AddString(GetDlgItem(hwnd,IDC_COMBO2),"176400");
		ComboBox_AddString(GetDlgItem(hwnd,IDC_COMBO2),"192000");
		ComboBox_SetCurSel(GetDlgItem(hwnd,IDC_COMBO1),2);
		ComboBox_SetCurSel(GetDlgItem(hwnd,IDC_COMBO2),2);
		//HFONT uusfontti=CreateFont(
		//SetWindow (GetDlgItem(hwnd,IDC_STATIC7),uusfontti,true);
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
	DialogBoxA(g_hInst,MAKEINTRESOURCE(IDD_DISKSPACECALC),g_hwndParent,DiskCalcDlgProc);
}