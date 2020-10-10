/******************************************************************************
/ XenQueryDlg.cpp
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

char *g_QueryString;
int g_QueryMaxChars;
const char *g_QueryTitle;
int g_DialogResult;

WDL_DLGRET GenQueryDlgProc1(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, Message, wParam, lParam))
		return r;

	if (Message==WM_INITDIALOG)
	{
		if (strlen(g_QueryTitle)>0)
			SetWindowText(hwnd,g_QueryTitle);
		if (strlen(g_QueryString)>0)
			SetDlgItemText(hwnd,IDC_EDIT1,g_QueryString);

		SetFocus(GetDlgItem(hwnd, IDC_EDIT1));
		SendMessage(GetDlgItem(hwnd, IDC_EDIT1), EM_SETSEL, 0, -1);
		return 0;
	}
	if (Message==WM_COMMAND && LOWORD(wParam)==IDOK)
	{
		GetDlgItemText(hwnd,IDC_EDIT1,g_QueryString,g_QueryMaxChars);
		EndDialog(hwnd,0);
		g_DialogResult=0;
		return 0;
	}
	if (Message==WM_COMMAND && LOWORD(wParam)==IDCANCEL)
	{
		g_DialogResult=1;
		EndDialog(hwnd,0);
		return 0;
	}
	return 0;
}

int XenSingleStringQueryDlg(HWND hParent,const char *QueryTitle,char *QueryResult,int maxchars)
{
	g_DialogResult=-1;
	g_QueryMaxChars=maxchars;
	g_QueryTitle=QueryTitle;
	g_QueryString=QueryResult;
	DialogBox(g_hInst,MAKEINTRESOURCE(IDD_GENERICQUERY1),hParent,(DLGPROC)GenQueryDlgProc1);
	return g_DialogResult;
}
