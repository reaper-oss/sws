/******************************************************************************
/ MarkerList.cpp
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

#include "stdafx.h"

#include "ProjectList.h"
#include "ProjectMgr.h"

#include <WDL/localize/localize.h>

// Globals
SWS_ProjectListWnd* g_pProjList = NULL;

// !WANT_LOCALIZE_STRINGS_BEGIN:sws_DLG_146
static SWS_LVColumn g_cols[] = { { 30, 0, "#" }, { 100, 0, "Name" }, { 185, 0, "Path", -1 }, };
// !WANT_LOCALIZE_STRINGS_END

SWS_ProjectListView::SWS_ProjectListView(HWND hwndList, HWND hwndEdit)
:SWS_ListView(hwndList, hwndEdit, 3, g_cols, "ProjListViewState", false, "sws_DLG_146")
{
}

void SWS_ProjectListView::GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax)
{
	ReaProject* proj = (ReaProject*)item;
	int i = 0;
	char cFilename[512];
	while (proj != EnumProjects(i, cFilename, 512))
		i++;
	
	switch (iCol)
	{
	case 0: // #
		snprintf(str, iStrMax, "%d", i+1);
		break;
	case 1: // Name
		{
			char* pSlash = strrchr(cFilename, PATH_SLASH_CHAR);
			if (pSlash)
			{
				char* pExt = strrchr(cFilename, '.');
				if (pExt) *pExt = 0;
				lstrcpyn(str, pSlash+1, iStrMax);
			}
			else
				lstrcpyn(str, cFilename, iStrMax);
			break;
		}
	case 2: // Path
		{
			char* pSlash = strrchr(cFilename, PATH_SLASH_CHAR);
			if (pSlash) *pSlash = 0;
			lstrcpyn(str, cFilename, iStrMax);
			break;
		}
		break;
	}
}

void SWS_ProjectListView::OnItemDblClk(SWS_ListItem* item, int iCol)
{
	SelectProjectInstance((ReaProject*)item);
	g_pProjList->Show(false, true);
}

void SWS_ProjectListView::GetItemList(SWS_ListItemList* pList)
{
	ReaProject* proj;
	int i = 0;
	while((proj = EnumProjects(i++, NULL, 0)))
		pList->Add((SWS_ListItem*)proj);
}

SWS_ProjectListWnd::SWS_ProjectListWnd()
:SWS_DockWnd(IDD_PROJLIST, __LOCALIZE("Project List","sws_DLG_157"), "SWSProjectList")
{
	// Must call SWS_DockWnd::Init() to restore parameters and open the window if necessary
	Init();
}

void SWS_ProjectListWnd::Update()
{
	if (m_pLists.Get(0))
		m_pLists.Get(0)->Update();
}

void SWS_ProjectListWnd::OnInitDlg()
{
	m_resize.init_item(IDC_LIST, 0.0, 0.0, 1.0, 1.0);
	m_pLists.Add(new SWS_ProjectListView(GetDlgItem(m_hwnd, IDC_LIST), GetDlgItem(m_hwnd, IDC_EDIT)));
	Update();
}

void SWS_ProjectListWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	Main_OnCommand((int)wParam, (int)lParam);
}

HMENU SWS_ProjectListWnd::OnContextMenu(int x, int y, bool* wantDefaultItems)
{
	HMENU hMenu = CreatePopupMenu();
	AddToMenu(hMenu, __localizeFunc(g_projMgrCmdTable[0].menuText,"sws_menu",0), g_projMgrCmdTable[0].cmdId);
	AddToMenu(hMenu, __localizeFunc(g_projMgrCmdTable[1].menuText,"sws_menu",0), g_projMgrCmdTable[1].cmdId);
	return hMenu;
}

int SWS_ProjectListWnd::OnKey(MSG* msg, int iKeyState)
{
	if (msg->message == WM_KEYDOWN && !iKeyState)
	{
		if (msg->wParam == VK_DELETE)
		{
			Main_OnCommand(40860, 0);
			Show(false, true);
			return 1;
		}
		else if (msg->wParam == VK_RETURN)
		{
			if (m_pLists.Get(0))
			{
				ReaProject* proj = (ReaProject*)m_pLists.Get(0)->EnumSelected(NULL);
				if (proj)
					SelectProjectInstance(proj);
			}
			Show(false, true);
			return 1;
		}
	}
	return 0;
}

void OpenProjectList(COMMAND_T*)
{
	g_pProjList->Show(true, true);
}

int ProjectListEnabled(COMMAND_T*)
{
	return g_pProjList->IsWndVisible();
}

void ProjectListUpdate()
{
	g_pProjList->Update();
	UpdateOpenProjectTabActions();
}

int ProjectListInit()
{
	g_pProjList = new SWS_ProjectListWnd();

	return 1;
}

void ProjectListExit()
{
	DELETE_NULL(g_pProjList);
}
