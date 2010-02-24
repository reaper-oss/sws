/******************************************************************************
/ Autocolor.cpp
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
#include "Color.h"
#include "Autocolor.h"

#define DELETE_MSG		0x100F0
#define SELNEXT_MSG		0x100F1
#define SELPREV_MSG		0x100F2

// INI params
#define AC_OPTIONS_KEY	"Autocolor Options"
#define AC_ITEM_KEY		"Autocolor %d"
#define AC_COUNT_KEY	"Autocolor count"

// Globals
SWS_AutoColorWnd* g_pACWnd = NULL;
WDL_PtrList<SWS_AutoColorItem> g_pACItems;
bool g_bACEnabled = false;
bool g_bACInit = false;

static SWS_LVColumn g_cols[] = { { 100, 1, "Filter" }, { 45, 0, "Color" }, };

SWS_AutoColorView::SWS_AutoColorView(HWND hwndList, HWND hwndEdit)
:SWS_ListView(hwndList, hwndEdit, 2, g_cols, "AutoColorViewState", false)
{
}

void SWS_AutoColorView::GetItemText(LPARAM item, int iCol, char* str, int iStrMax)
{
	SWS_AutoColorItem* pItem = (SWS_AutoColorItem*)item;
	if (pItem && iCol == 0)
		lstrcpyn(str, pItem->m_str.Get(), iStrMax);
}

int SWS_AutoColorView::OnItemSort(LPARAM item1, LPARAM item2)
{
	if (abs(m_iSortCol) == 2)
	{
		int iRet;
		SWS_AutoColorItem* i1 = (SWS_AutoColorItem*)item1;
		SWS_AutoColorItem* i2 = (SWS_AutoColorItem*)item2;
		if (i1->m_col > i2->m_col)
			iRet = 1;
		else if (i1->m_col < i2->m_col)
			iRet = -1;
		if (m_iSortCol < 0)
			return -iRet;
		else
			return iRet;
	}
	return SWS_ListView::OnItemSort(item1, item2);
}

void SWS_AutoColorView::SetItemText(LPARAM item, int iCol, const char* str)
{
	SWS_AutoColorItem* pItem = (SWS_AutoColorItem*)item;
	if (pItem && iCol == 0)
	{
		pItem->m_str.Set(str);
		Update();
	}
}

void SWS_AutoColorView::GetItemList(WDL_TypedBuf<LPARAM>* pBuf)
{
	pBuf->Resize(g_pACItems.GetSize());
	for (int i = 0; i < pBuf->GetSize(); i++)
		pBuf->Get()[i] = (LPARAM)g_pACItems.Get(i);
}

SWS_AutoColorWnd::SWS_AutoColorWnd()
:SWS_DockWnd(IDD_AUTOCOLOR, "Auto Color", 30005)
{
	if (m_bShowAfterInit)
		Show(false, false);
}

void SWS_AutoColorWnd::Update()
{
	if (IsValidWindow())
	{
		// Set the checkboxes
		CheckDlgButton(m_hwnd, IDC_INITIAL, g_bACInit    ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(m_hwnd, IDC_ENABLED, g_bACEnabled ? BST_CHECKED : BST_UNCHECKED);

		if (m_pLists.GetSize())
			m_pLists.Get(0)->Update();
	}
}

void SWS_AutoColorWnd::OnInitDlg()
{
	m_resize.init_item(IDC_LIST,      0.0, 0.0, 1.0, 1.0);
	m_resize.init_item(IDC_ADD,       0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_REMOVE,    0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_INITIAL,   0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_ENABLED,   0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_APPLY,     1.0, 1.0, 1.0, 1.0);
	m_resize.init_item(IDC_COLORMGMT, 1.0, 1.0, 1.0, 1.0);
	m_pLists.Add(new SWS_AutoColorView(GetDlgItem(m_hwnd, IDC_LIST), GetDlgItem(m_hwnd, IDC_EDIT)));
	Update();
}

void SWS_AutoColorWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
		case IDC_APPLY:
			break;
		case IDC_ADD:
			g_pACItems.Add(new SWS_AutoColorItem("(name)", 0));
			Update();
			break;
		case IDC_REMOVE:
			for (int i = 0; i < g_pACItems.GetSize(); i++)
				if (m_pLists.Get(0)->IsSelected(i))
				{
					int idx = g_pACItems.Find((SWS_AutoColorItem*)m_pLists.Get(0)->GetListItem(i));
					if (idx >= 0)
						g_pACItems.Delete(idx);
				}
			Update();						
			break;
		case IDC_INITIAL:
			g_bACInit = !g_bACInit;
			Update();
			break;
		case IDC_ENABLED:
			g_bACEnabled = !g_bACEnabled;
			Update();
			break;
		case IDC_COLORMGMT:
			ShowColorDialog();
			break;
	}
}

HMENU SWS_AutoColorWnd::OnContextMenu(int x, int y)
{
	int iCol;
	LPARAM item = m_pLists.Get(0)->GetHitItem(x, y, &iCol);

	// TODO add tags
	if (item && iCol == 0)
	{
		HMENU hMenu = CreatePopupMenu();


		return hMenu;
	}
	return NULL;
}

void OpenAutoColor(COMMAND_T*)
{
	g_pACWnd->Show(true, true);
}

void EnableAutoColor(COMMAND_T*)
{
	g_bACEnabled = !g_bACEnabled;
	g_pACWnd->Update();
}

static bool IsAutoColorOpen(COMMAND_T*)		{ return g_pACWnd->IsValidWindow(); }
static bool IsAutoColorEnabled(COMMAND_T*)	{ return g_bACEnabled; }

static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "SWS: Open auto color window" },		"SWSAUTOCOLOR_OPEN",	OpenAutoColor,		"SWS Auto Color",			0, IsAutoColorOpen },
	{ { DEFACCEL, "SWS: Enable auto coloring" },		"SWSAUTOCOLOR_ENABLE",	EnableAutoColor,	"Enable SWS auto coloring", 0, IsAutoColorEnabled },
	{ {}, LAST_COMMAND, }, // Denote end of table
};

static int translateAccel(MSG *msg, accelerator_register_t *ctx)
{
	if (g_pACWnd->IsActive())
	{
		if (msg->message == WM_KEYDOWN)
		{
			bool bCtrl  = GetAsyncKeyState(VK_CONTROL) & 0x8000 ? true : false;
			bool bAlt   = GetAsyncKeyState(VK_MENU)    & 0x8000 ? true : false;
			bool bShift = GetAsyncKeyState(VK_SHIFT)   & 0x8000 ? true : false;

			if (msg->wParam == VK_DELETE && !bCtrl && !bAlt && !bShift)
			{
				SendMessage(g_pACWnd->GetHWND(), WM_COMMAND, DELETE_MSG, 0);
				return 1;
			}
			/*else if (msg->wParam == VK_UP && !bCtrl && !bAlt && !bShift)
			{
				SendMessage(g_pACWnd->GetHWND(), WM_COMMAND, SELPREV_MSG, 0);
				return 1;
			}
			else if (msg->wParam == VK_DOWN && !bCtrl && !bAlt && !bShift)
			{
				SendMessage(g_pACWnd->GetHWND(), WM_COMMAND, SELNEXT_MSG, 0);
				return 1;
			}*/
		}
		return -666;
	}
	return 0;
} 

static accelerator_register_t g_ar = { translateAccel, TRUE, NULL };

static void menuhook(const char* menustr, HMENU hMenu, int flag)
{
	if (strcmp(menustr, "Main view") == 0 && flag == 0)
		AddToMenu(hMenu, g_commandTable[0].menuText, g_commandTable[0].accel.accel.cmd, 40075);
	else if (flag == 0 && strcmp(menustr, "Main options") == 0)
		AddToMenu(hMenu, g_commandTable[1].menuText, g_commandTable[1].accel.accel.cmd, 40745);
}

int AutoColorInit()
{
	if (!plugin_register("accelerator",&g_ar))
		return 0;

	SWSRegisterCommands(g_commandTable);

	if (!plugin_register("hookcustommenu", (void*)menuhook))
		return 0;

	// Restore state
	char str[128];
	GetPrivateProfileString(SWS_INI, AC_OPTIONS_KEY, "0 0", str, 128, get_ini_file());
	g_bACEnabled = str[0] == '1';
	g_bACInit    = str[0] == '0';

	int iCount = GetPrivateProfileInt(SWS_INI, AC_COUNT_KEY, 0, get_ini_file());
	for (int i = 0; i < iCount; i++)
	{
		char key[32];
		_snprintf(key, 32, AC_ITEM_KEY, i+1);
		GetPrivateProfileString(SWS_INI, key, "\"\",0", str, 128, get_ini_file());
		LineParser lp(false);
		if (!lp.parse(str))
			g_pACItems.Add(new SWS_AutoColorItem(lp.gettoken_str(0), lp.gettoken_int(1)));
	}	

	g_pACWnd = new SWS_AutoColorWnd();

	return 1;
}

void AutoColorExit()
{
	// Save state
	char str[128];
	sprintf(str, "%d %d", g_bACEnabled ? 1 : 0, g_bACInit ? 1 : 0);
	WritePrivateProfileString(SWS_INI, AC_OPTIONS_KEY, str, get_ini_file());
	sprintf(str, "%d", g_pACItems.GetSize());
	WritePrivateProfileString(SWS_INI, AC_COUNT_KEY, str, get_ini_file());
	for (int i = 0; i < g_pACItems.GetSize(); i++)
	{
		char key[32];
		_snprintf(key, 32, AC_ITEM_KEY, i+1);
		_snprintf(str, 128, "\"%s\",%d", g_pACItems.Get(i)->m_str.Get(), g_pACItems.Get(i)->m_col);
		WritePrivateProfileString(SWS_INI, key, str, get_ini_file());
	}	

	delete g_pACWnd;
}
