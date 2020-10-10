/******************************************************************************
/ sws_waitdlg.cpp
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

// Display a progress bar with dProgress from 0.0 - 1.0.
// The box closes and the constructor returns when dProgress >= 1.0.
// ESC closes the box as well, but it blocks until dProgress >= 1.0.
// You'll want to start a thread to do the work that updates dProgress.

// Note, on Win7 the progress bar update is filtered (why??) such that
// it appears that the progress is "behind" where it actually is for
// items that are of "normal" length

#include "stdafx.h"

#include "sws_waitdlg.h"

#include <WDL/localize/localize.h>

const char SWS_WAITDLG_WNDPOS_KEY[] = "Wait Dialog Position";

SWS_WaitDlg::SWS_WaitDlg(const char* cTitle, double* dProgress, HWND hParent)
{
	m_hwnd = NULL;
	m_dProgress = dProgress;
	m_cTitle = cTitle;
	double dPrevProgress = *dProgress;
	Sleep(0);
	if (*dProgress - dPrevProgress < 0.10)
		// 10% done in one time slice?  Skip the dlg display.
		DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_SNM_WAIT), hParent ? hParent : g_hwndParent, sWaitDlgWndProc, (LPARAM)this);
	// Block until process done if user-closed dlg
	while (*dProgress < 1.0)
		Sleep(1);
}

INT_PTR WINAPI SWS_WaitDlg::sWaitDlgWndProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) // static
{
	SWS_WaitDlg* pObj = (SWS_WaitDlg*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	if (!pObj && uMsg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
		pObj = (SWS_WaitDlg*)lParam;
		pObj->m_hwnd = hwndDlg;
	}
	if (pObj)
		return pObj->waitDlgWndProc(uMsg, wParam, lParam);
	else
		return 0;
}

int SWS_WaitDlg::waitDlgWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HWND hProgress = NULL;
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			SetWindowText(m_hwnd, m_cTitle);
			RestoreWindowPos(m_hwnd, SWS_WAITDLG_WNDPOS_KEY, false);
			hProgress = GetDlgItem(m_hwnd, IDC_PROGRESS);
			SendMessage(hProgress, PBM_SETPOS, (int)(*m_dProgress * 100.0), 0);
			SetTimer(m_hwnd, 1, 50, NULL);
			break;
		}
		case WM_TIMER:
			SendMessage(hProgress, PBM_SETPOS, (int)(*m_dProgress * 100.0)+1, 0); // Silly workaround for Win7 progress bar
			SendMessage(hProgress, PBM_SETPOS, (int)(*m_dProgress * 100.0), 0);
			if (*m_dProgress >= 1.0)
				SendMessage(m_hwnd, WM_COMMAND, IDCANCEL, 0);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
				case IDCANCEL:
					SaveWindowPos(m_hwnd, SWS_WAITDLG_WNDPOS_KEY);
					KillTimer(m_hwnd, 1);
					EndDialog(m_hwnd, 0);
					break;
			}
			break;
	}
	return 0;
}
