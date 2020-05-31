/******************************************************************************
/ CreateTrax.cpp
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

struct t_newtrackparams
{
	int numtracks;
	int inputtype;
	int inputidx;
	string basename;
	double panlaw;
};

t_newtrackparams g_newtrackparams;

WDL_DLGRET CreateTxDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	char buf[200];

	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, Message, wParam, lParam))
		return r;

	if (Message==WM_INITDIALOG)
	{
		
		sprintf(buf,"%d",g_newtrackparams.numtracks);
		SetDlgItemText(hwnd,IDC_EDIT1,buf);
		SetDlgItemText(hwnd,IDC_EDIT2,g_newtrackparams.basename.c_str());
		HWND hCombo = GetDlgItem(hwnd,IDC_COMBO1);
		WDL_UTF8_HookComboBox(hCombo);
		int idx=0;
		while (GetInputChannelName(idx))
		{
			SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)GetInputChannelName(idx));
			idx++;
		}
		SendMessage(hCombo, CB_SETCURSEL, 0, 0);
		SetFocus(GetDlgItem(hwnd, IDC_EDIT1));
		SendMessage(GetDlgItem(hwnd, IDC_EDIT1), EM_SETSEL, 0, -1);
		return 0;
	}
	if (Message==WM_COMMAND && LOWORD(wParam)==IDOK)
	{
		
		GetDlgItemText(hwnd,IDC_EDIT1,buf,199);
		g_newtrackparams.numtracks=atoi(buf);
		if (g_newtrackparams.numtracks<1) g_newtrackparams.numtracks=1;
		if (g_newtrackparams.numtracks>256) g_newtrackparams.numtracks=256;
		GetDlgItemText(hwnd,IDC_EDIT2,buf,199);
		g_newtrackparams.basename.assign(buf);
		Undo_BeginBlock();
		int i;
		int idxfirstcreated=0;
		vector<MediaTrack*> vectk;
		for (i=0;i<g_newtrackparams.numtracks;i++)
		{
			Main_OnCommand(40001,0);
			XenGetProjectTracks(vectk,true);
			if (vectk.size()==1)
			{
				if (i==0)
				{
					idxfirstcreated=CSurf_TrackToID(vectk[0],false);
				}
				snprintf(buf,sizeof(buf),"%s %.2d",g_newtrackparams.basename.c_str(),i+1);
				GetSetMediaTrackInfo(vectk[0],"P_NAME",&buf);
			}
		}
		if (idxfirstcreated>0)
		{
			for (i=0;i<g_newtrackparams.numtracks;i++)
			{
				MediaTrack* curt=CSurf_TrackFromID(i+idxfirstcreated,false);
				int issel=1;
				GetSetMediaTrackInfo(curt,"I_SELECTED",&issel);
			}
		}
		Undo_EndBlock(__LOCALIZE("Create tracks","sws_undo"),0);
		sprintf(buf,"%d",g_newtrackparams.numtracks);
		WritePrivateProfileString("XENAKIOSCOMMANDS","NTDLG_NUMNEWTRACKS",buf,g_XenIniFilename.Get());
		WritePrivateProfileString("XENAKIOSCOMMANDS","NTDLG_BASENAME",g_newtrackparams.basename.c_str(),g_XenIniFilename.Get());
		EndDialog(hwnd,0);
		return 0;
	}
	if (Message==WM_COMMAND && LOWORD(wParam)==IDCANCEL)
	{
		EndDialog(hwnd,0);
		return 0;
	}
	return 0;
}

void DoCreateTraxDlg(COMMAND_T*)
{
	char buf[512];
	GetPrivateProfileString("XENAKIOSCOMMANDS","NTDLG_NUMNEWTRACKS","1",buf,512,g_XenIniFilename.Get());
	g_newtrackparams.numtracks=atoi(buf);
	if (g_newtrackparams.numtracks<1) g_newtrackparams.numtracks=1;
	if (g_newtrackparams.numtracks>256) g_newtrackparams.numtracks=256;
	GetPrivateProfileString("XENAKIOSCOMMANDS","NTDLG_BASENAME","New Track",buf,512,g_XenIniFilename.Get());
	g_newtrackparams.basename.assign(buf);
	DialogBox(g_hInst,MAKEINTRESOURCE(IDD_CRTNEWTX), g_hwndParent,(DLGPROC)CreateTxDlgProc);
}
