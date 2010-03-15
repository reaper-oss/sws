/******************************************************************************
/ MarkerList.cpp
/
/ Copyright (c) 2010 Tim Payne (SWS)
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
#include "ProjectList.h"
#include "ProjectMgr.h"

// Globals
SWS_ProjectListWnd* g_pProjList = NULL;

static SWS_LVColumn g_cols[] = { { 30, 0, "#" }, { 285, 0, "Name" }, };

SWS_ProjectListView::SWS_ProjectListView(HWND hwndList, HWND hwndEdit)
:SWS_ListView(hwndList, hwndEdit, 2, g_cols, "ProjListViewState", false)
{
}

void SWS_ProjectListView::GetItemText(LPARAM item, int iCol, char* str, int iStrMax)
{
	ReaProject* proj = (ReaProject*)item;
	int i = 0;
	char cFilename[512];
	while (proj != EnumProjects(i, cFilename, 512))
		i++;

	if (iCol == 0)
		_snprintf(str, iStrMax, "%d", i+1);
	else if (iCol == 1)
		lstrcpyn(str, cFilename, iStrMax);
}

bool SWS_ProjectListView::OnItemSelChange(LPARAM item, bool bSel)
{
	if (bSel)
		SelectProjectInstance((ReaProject*)item);
	return false;
}

void SWS_ProjectListView::GetItemList(WDL_TypedBuf<LPARAM>* pBuf)
{
	ReaProject* proj;
	int i = 0;
	pBuf->Resize(0, false);
	while((proj = Enum_Projects(i++, NULL, 0)))
	{
		pBuf->Resize(i);
		pBuf->Get()[i-1] = (LPARAM)proj;
	}
}

int SWS_ProjectListView::GetItemState(LPARAM item)
{
	if ((ReaProject*)item == Enum_Projects(-1, NULL, 0))
		return LVIS_SELECTED | LVIS_FOCUSED;
	return 0;
}

SWS_ProjectListWnd::SWS_ProjectListWnd()
:SWS_DockWnd(IDD_PROJLIST, "Project List", 30006)
{
	if (m_bShowAfterInit)
		Show(false, false);
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

HMENU SWS_ProjectListWnd::OnContextMenu(int x, int y)
{
	HMENU hMenu = CreatePopupMenu();
	AddToMenu(hMenu, g_projMgrCmdTable[0].menuText, g_projMgrCmdTable[0].accel.accel.cmd);
	AddToMenu(hMenu, g_projMgrCmdTable[1].menuText, g_projMgrCmdTable[1].accel.accel.cmd);
	return hMenu;
}

void OpenProjectList(COMMAND_T*)
{
	g_pProjList->Show(true, true);
}

bool ProjectListEnabled(COMMAND_T*)
{
	return g_pProjList->IsValidWindow();
}

void ProjectListUpdate()
{
	g_pProjList->Update();
}

static int translateAccel(MSG *msg, accelerator_register_t *ctx)
{
	if (g_pProjList->IsActive())
	{
		if (msg->message == WM_KEYDOWN)
		{
			bool bCtrl  = GetAsyncKeyState(VK_CONTROL) & 0x8000 ? true : false;
			bool bAlt   = GetAsyncKeyState(VK_MENU)    & 0x8000 ? true : false;
			bool bShift = GetAsyncKeyState(VK_SHIFT)   & 0x8000 ? true : false;

			if (msg->wParam == VK_DELETE && !bCtrl && !bAlt && !bShift)
			{
				Main_OnCommand(40860, 0);
				SetFocus(g_pProjList->GetHWND());
				return 1;
			}
			else if (msg->wParam == VK_UP && !bCtrl && !bAlt && !bShift)
			{
				Main_OnCommand(40862, 0);
				SetFocus(g_pProjList->GetHWND());
				return 1;
			}
			else if (msg->wParam == VK_DOWN && !bCtrl && !bAlt && !bShift)
			{
				Main_OnCommand(40861, 0);
				SetFocus(g_pProjList->GetHWND());
				return 1;
			}
		}
		return -666;
	}
	return 0;
} 

static accelerator_register_t g_ar = { translateAccel, TRUE, NULL };

int ProjectListInit()
{
	if (!plugin_register("accelerator",&g_ar))
		return 0;

	g_pProjList = new SWS_ProjectListWnd();

	return 1;
}

void ProjectListExit()
{
	delete g_pProjList;
}
