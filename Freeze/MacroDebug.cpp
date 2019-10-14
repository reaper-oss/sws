/***********************************
/ MacroDebug.cpp
/ REAPER extension
/ Copyright (C) 2008 Tim Payne, Standing Water Studios
/
/ This program is free software: you can redistribute it and/or modify
/ it under the terms of the GNU General Public License as published by
/ the Free Software Foundation, either version 3 of the License, or
/ (at your option) any later version.
/
/ This program is distributed in the hope that it will be useful,
/ but WITHOUT ANY WARRANTY; without even the implied warranty of
/ MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/ GNU General Public License for more details.
/
***********************************/

#include "stdafx.h"
#include <reaper_plugin_functions.h>
#include "../sws_util.h"
#include "../resource.h"

// Well, this isn't ready for primetime without the proper API to get
// access to macros, so here it sits unlinked.

class Macro
{
public:
	int* m_pActions;
	int m_iNumActions;
	char m_cName[256];
	
	Macro(LineParser* lp)
	{
		m_pActions = NULL;
		m_cName[0] = 0;
		m_iNumActions = lp->getnumtokens()-5;
		if (m_iNumActions > 0)
		{
			strncpy(m_cName, lp->gettoken_str(4), 256);
			m_cName[255] = 0;
			m_pActions = new int[m_iNumActions];
			for (int i = 0; i < m_iNumActions; i++)
			{
				m_pActions[i] = lp->gettoken_int(i+5);
				if (!m_pActions[i])
				{
					// DAMN!  todo figure out how to convert string into command ID
					m_iNumActions--;
					i--;
					lp->eattoken();
				}
			}
		}
		else
			m_iNumActions = 0;
	}
	~Macro()
	{
		if (m_pActions)
			delete[] m_pActions;
	}
};

static WDL_PtrList<Macro> g_macros;
static Macro* g_selectedMacro = NULL;
#define DEBUG_WINDOWPOS_KEY "CustomActionDebugWindowPos"

static int CALLBACK MacroSort(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	Macro* m1 = (Macro*)lParam1;
	Macro* m2 = (Macro*)lParam2;
	return strcmp(m1->m_cName, m2->m_cName);
}

INT_PTR WINAPI doDebug(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			RestoreWindowPos(hwndDlg, DEBUG_WINDOWPOS_KEY);

			HWND listView = GetDlgItem(hwndDlg, IDC_LIST);
			LineParser lp(false);
			char cFile[256];
			strcpy(cFile, get_ini_file());
			strcpy(strrchr(cFile, '.'), "-kb.ini");
			FILE* f = fopen(cFile, "r");
			char cLine[65536];
			int i = 0;
			LVITEM item;
			item.mask = LVIF_TEXT | LVIF_PARAM;
			item.iSubItem = 0;

			while (f && fgets(cLine, 65536, f))
			{
				lp.parse(cLine);
				if (lp.getnumtokens() - 5 > 0 && strcmp(lp.gettoken_str(0), "ACT") == 0)
				{
					Macro* m = g_macros.Add(new Macro(&lp));
					item.lParam = (LPARAM)m;
					item.iItem = i++;
					item.pszText = m->m_cName;
					ListView_InsertItem(listView, &item);
				}
			}
			if (f)
				fclose(f);
			ListView_SortItems(listView, MacroSort, 0);
			return 0;
		}
		case WM_NOTIFY:
		{
			NMLISTVIEW* s = (NMLISTVIEW*)lParam;
			if (s->hdr.code == LVN_ITEMCHANGED && s->uNewState & LVIS_SELECTED)
				g_selectedMacro = (Macro*)s->lParam;
			break;
		}
		

		case WM_COMMAND:
			//if you need to debug commands, you probably want to ignore the focus stuff.  Break a few lines down.
			if (wParam == 33555433 || wParam == 16778217)
				return 0;
			if (LOWORD(wParam)==IDCANCEL)
			{
				g_selectedMacro	= NULL;
				SaveWindowPos(hwndDlg, DEBUG_WINDOWPOS_KEY);
				EndDialog(hwndDlg,0);
			}
			if (LOWORD(wParam)==IDOK)
			{
				SaveWindowPos(hwndDlg, DEBUG_WINDOWPOS_KEY);
				EndDialog(hwndDlg,0);
			}
			return 0;
		case WM_DESTROY:
			// We're done
			return 0;
	}
	return 0;
}

void MacroDebug()
{
	DialogBox(g_hInst,MAKEINTRESOURCE(IDD_MACRODEBUG),g_hwndParent,doDebug);

	if (g_selectedMacro)
	{
		char cCommandText[256];
		for (int i = 0; i < g_selectedMacro->m_iNumActions; i++)
		{
			snprintf(cCommandText, sizeof(cCommandText), "Run %s?", kbd_getTextFromCmd(g_selectedMacro->m_pActions[i], NULL));
			int iRet = MessageBox(g_hwndParent, cCommandText, g_selectedMacro->m_cName, MB_YESNOCANCEL);
			if (iRet == IDYES)
				Main_OnCommand(g_selectedMacro->m_pActions[i], 0);
			else if (iRet == IDCANCEL)
				break;
		}
		g_selectedMacro = NULL;
	}
	g_macros.Empty(true);

}

static COMMAND_STRUCT g_commandTable[] = 
{
	{ { { 0, 0, 0 }, "SWS: Debug custom action(s)" },                                "SWS_ACTIONDEBUG",    MacroDebug,       NULL, false },
	{ {}, LAST_COMMAND, }, // Denote end of table
};
