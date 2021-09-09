/******************************************************************************
/ MarkerList.cpp
/
/ Copyright (c) 2012 Tim Payne (SWS)
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
#include "MarkerListClass.h"
#include "MarkerList.h"
#include "MarkerListActions.h"

#include <WDL/localize/localize.h>
#include <WDL/projectcontext.h>

#define SAVEWINDOW_POS_KEY "Markerlist Save Window Position"
#define ML_OPTIONS_KEY "MarkerlistOptions"

#define DELETE_MSG		0x100F0
#define COLOR_MSG		0x100F1
#define RENAME_MSG		0x100F2
#define FIRST_LOAD_MSG	0x10100

// Globals
static SWSProjConfig<WDL_PtrList_DOD<MarkerList> > g_savedLists;
MarkerList* g_curList = NULL;
SWS_MarkerListWnd* g_pMarkerList = NULL;

// !WANT_LOCALIZE_STRINGS_BEGIN:sws_DLG_102
static SWS_LVColumn g_cols[] = { { 75, 0, "Time" }, { 45, 0, "Type" }, { 30, 0, "ID" }, { 170, 1, "Description" },  { 70, 0, "Color", -1 }};
// !WANT_LOCALIZE_STRINGS_END

SWS_MarkerListView::SWS_MarkerListView(HWND hwndList, HWND hwndEdit, SWS_MarkerListWnd* pList)
:SWS_ListView(hwndList, hwndEdit, 5, g_cols, "MarkerList View State", false, "sws_DLG_102"), m_pMarkerList(pList)
{
}

void SWS_MarkerListView::GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax)
{
	MarkerItem* mi = (MarkerItem*)item;
	if (mi)
	{
		switch (iCol)
		{
		case 0:
			format_timestr_pos(mi->GetPos(), str, iStrMax, -1);
			break;
		case 1:
			snprintf(str, iStrMax, "%s", mi->IsRegion() ? __LOCALIZE("Region","sws_DLG_102") : __LOCALIZE("Marker","sws_DLG_102"));
			break;
		case 2:
			snprintf(str, iStrMax, "%d", mi->GetNum());
			break;
		case 3:
			lstrcpyn(str, mi->GetName(), iStrMax);
			break;
		case 4:
#ifdef _WIN32
			snprintf(str, iStrMax, "0x%02x%02x%02x", mi->GetColor() & 0xFF, (mi->GetColor() >> 8) & 0xFF, (mi->GetColor() >> 16) & 0xFF);
#else
			snprintf(str, iStrMax, "0x%06x", mi->GetColor());
#endif
			break;
		}
	}
}

void SWS_MarkerListView::OnItemSelChanged(SWS_ListItem* item, int iState)
{
	if (iState & LVIS_FOCUSED)
	{
		MarkerItem* mi = (MarkerItem*)item;
		if (mi->GetPos() != GetCursorPosition())
		{
			SetEditCurPos(mi->GetPos(), m_pMarkerList->m_bScroll, false); // Never play here, do it from the clk handler
			m_pMarkerList->m_dCurPos = mi->GetPos(); // Save pos
		}
	}
}

void SWS_MarkerListView::OnItemClk(SWS_ListItem* item, int iCol, int iKeyState)
{
	MarkerItem* mi = (MarkerItem*)item;
	if (mi) // Doubled-up calls to SetEditCurPos - oh well!
		SetEditCurPos(mi->GetPos(), m_pMarkerList->m_bScroll, m_pMarkerList->m_bPlayOnSel);

	if (iKeyState == LVKF_SHIFT)
	{
		// Find min-max time and sel across them
		double dMaxTime = -DBL_MAX;
		double dMinTime = DBL_MAX;
		for (int i = 0; i < GetListItemCount(); i++)
		{
			int iState;
			mi = (MarkerItem*)GetListItem(i, &iState);
			if (iState)
			{
				if (mi->GetPos() < dMinTime)
					dMinTime = mi->GetPos();
				if (mi->IsRegion() && mi->GetRegEnd() > dMaxTime)
					dMaxTime = mi->GetRegEnd();
				else if (mi->GetPos() > dMaxTime)
					dMaxTime = mi->GetPos();
			}
		}
		if (dMaxTime != -DBL_MAX && dMinTime != DBL_MAX)
			GetSet_LoopTimeRange(true, false, &dMinTime, &dMaxTime, false);
	}
}

void SWS_MarkerListView::OnItemDblClk(SWS_ListItem* item, int iCol)
{
	MarkerItem* mi = (MarkerItem*)item;
	if (iCol != 4)
	{
		if (mi->IsRegion())
		{
			double d1 = mi->GetPos(), d2 = mi->GetRegEnd();
			GetSet_LoopTimeRange(true, true, &d1, &d2, m_pMarkerList->m_bPlayOnSel);
		}
	}
	else if (g_pMarkerList && SWS_IsWindow(g_pMarkerList->GetHWND()))
		SendMessage(g_pMarkerList->GetHWND(), WM_COMMAND, COLOR_MSG, 0);
}

int SWS_MarkerListView::OnItemSort(SWS_ListItem* item1, SWS_ListItem* item2)
{
	int iRet;
	MarkerItem* mi1 = (MarkerItem*)item1;
	MarkerItem* mi2 = (MarkerItem*)item2;

	if (abs(m_iSortCol) == 1)
	{
		if (mi1->GetPos() > mi2->GetPos())
			iRet = 1;
		else if (mi1->GetPos() < mi2->GetPos())
			iRet = -1;
		if (m_iSortCol < 0)
			return -iRet;
		else
			return iRet;
	}
	else if (abs(m_iSortCol) == 3)
	{
		if (mi1->GetNum() > mi2->GetNum())
			iRet = 1;
		else if (mi1->GetNum() < mi2->GetNum())
			iRet = -1;
		if (m_iSortCol < 0)
			return -iRet;
		else
			return iRet;
	}
	return SWS_ListView::OnItemSort(item1, item2);
}

void SWS_MarkerListView::SetItemText(SWS_ListItem* item, int iCol, const char* str)
{
	if (iCol == 3)
	{
		MarkerItem* mi = (MarkerItem*)item;
		mi->SetName(str);
		mi->UpdateProject();
		Update();
	}
}

void SWS_MarkerListView::GetItemList(SWS_ListItemList* pList)
{
	if (m_pMarkerList->m_filter.GetLength())
	{
		LineParser lp(false);
		lp.parse(m_pMarkerList->m_filter.Get());
		for (int i = 0; i < g_curList->m_items.GetSize(); i++)
			for (int j = 0; j < lp.getnumtokens(); j++)
				if (stristr(g_curList->m_items.Get(i)->GetName(), lp.gettoken_str(j)))
				{
					pList->Add((SWS_ListItem*)g_curList->m_items.Get(i));
					break;
				}
	}
	else
	{
		for (int i = 0; i < g_curList->m_items.GetSize(); i++)
			pList->Add((SWS_ListItem*)g_curList->m_items.Get(i));
	}
}

int SWS_MarkerListView::GetItemState(SWS_ListItem* item)
{
	MarkerItem* mi = (MarkerItem*)item;
	return GetCursorPosition() == mi->GetPos() ? 1 : 0;
}

SWS_MarkerListWnd::SWS_MarkerListWnd()
:SWS_DockWnd(IDD_MARKERLIST, __LOCALIZE("Marker List","sws_DLG_102"), "SWSMarkerList"), m_dCurPos(DBL_MAX)
{
	// Must call SWS_DockWnd::Init() to restore parameters and open the window if necessary
	Init();
}

void SWS_MarkerListWnd::Update(bool bForce)
{
	// Change the time string if the project time mode changes
	static int prevTimeMode = -1;
	bool bChanged = bForce;
	if (*ConfigVar<int>("projtimemode") != prevTimeMode)
	{
		prevTimeMode = *ConfigVar<int>("projtimemode");
		bChanged = true;
	}

	double dCurPos = GetCursorPosition();
	if (dCurPos != m_dCurPos)
	{
		m_dCurPos = dCurPos;
		bChanged = true;
	}

	if (!g_curList)
	{
		g_curList = new MarkerList("CurrentList", true);
		bChanged = true;
	}
	else if (g_curList->BuildFromReaper())
		bChanged = true;

	if (m_pLists.GetSize() && bChanged)
	{
		SWS_SectionLock lock(&g_curList->m_mutex);
		m_pLists.Get(0)->Update();
	}
}

void SWS_MarkerListWnd::OnInitDlg()
{
	m_resize.init_item(IDC_LIST, 0.0, 0.0, 1.0, 1.0);
	m_resize.init_item(IDC_STATIC_FILTER, 0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_FILTER, 0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_CLEAR, 0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_PLAY, 0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_SCROLL, 0.0, 1.0, 0.0, 1.0);

	SetWindowLongPtr(GetDlgItem(m_hwnd, IDC_FILTER), GWLP_USERDATA, 0xdeadf00b);

	m_pLists.Add(new SWS_MarkerListView(GetDlgItem(m_hwnd, IDC_LIST), GetDlgItem(m_hwnd, IDC_EDIT), this));
	delete g_curList;
	g_curList = NULL;

	char cOptions[10];
	GetPrivateProfileString(SWS_INI, ML_OPTIONS_KEY, "1 1", cOptions, 10, get_ini_file());
	m_bPlayOnSel = (cOptions[0] == '1');
	m_bScroll = (cOptions[2] == '1');

	CheckDlgButton(m_hwnd, IDC_PLAY, m_bPlayOnSel ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_SCROLL, m_bScroll  ? BST_CHECKED : BST_UNCHECKED);
	
	Update();

	SetTimer(m_hwnd, 1, 500, NULL);
}

void SWS_MarkerListWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
		case IDC_FILTER | (EN_CHANGE << 16):
		{
			char cFilter[100];
			GetWindowText(GetDlgItem(m_hwnd, IDC_FILTER), cFilter, 100);
			m_filter.Set(cFilter);
			Update(true);
			break;
		}
		case IDC_CLEAR:
			SetDlgItemText(m_hwnd, IDC_FILTER, "");
			break;
		case IDC_PLAY:
			m_bPlayOnSel = IsDlgButtonChecked(m_hwnd, IDC_PLAY) == BST_CHECKED;
			break;
		case IDC_SCROLL:
			m_bScroll = IsDlgButtonChecked(m_hwnd, IDC_SCROLL) == BST_CHECKED;
			break;
		case DELETE_MSG:
			if (ListView_GetSelectedCount(m_pLists.Get(0)->GetHWND()))
			{
				Undo_BeginBlock();
				int i = 0;
				MarkerItem* item;
				while ((item = (MarkerItem*)m_pLists.Get(0)->EnumSelected(&i)))
					DeleteProjectMarker(NULL, item->GetNum(), item->IsRegion());
				Undo_EndBlock(__LOCALIZE("Delete marker(s)","sws_undo"), UNDO_STATE_MISCCFG);
				Update();
				break;
			}
		case COLOR_MSG:
		{
			int iColor;
			m_pLists.Get(0)->DisableUpdates(true);
			if (GR_SelectColor(m_hwnd, &iColor))
			{
				int i = 0;
				MarkerItem* item;
				while ((item = (MarkerItem*)m_pLists.Get(0)->EnumSelected(&i)))
				{
					item->SetColor(iColor | 0x1000000);
					item->UpdateProject();
				}
			}
			m_pLists.Get(0)->DisableUpdates(false);
			break;
		}
		case RENAME_MSG:
			m_pLists.Get(0)->EditListItem(m_pLists.Get(0)->EnumSelected(NULL), 3);
			break;

		default:
			if (wParam >= FIRST_LOAD_MSG && wParam - FIRST_LOAD_MSG < (UINT)g_savedLists.Get()->GetSize())
			{	// Load marker list
				g_savedLists.Get()->Get(wParam - FIRST_LOAD_MSG)->UpdateReaper();
				Update();
			}
			else
				Main_OnCommand((int)wParam, (int)lParam);
	}
}

HMENU SWS_MarkerListWnd::OnContextMenu(int x, int y, bool* wantDefaultItems)
{
	HMENU hMenu = CreatePopupMenu();
	AddToMenu(hMenu, __LOCALIZE("Rename","sws_DLG_102"), RENAME_MSG);
	AddToMenu(hMenu, __LOCALIZE("Set color...","sws_DLG_102"), COLOR_MSG);
	AddToMenu(hMenu, __LOCALIZE("Save marker set...","sws_DLG_102"), SWSGetCommandID(SaveMarkerList));
	AddToMenu(hMenu, __LOCALIZE("Delete market set...","sws_DLG_102"), SWSGetCommandID(DeleteMarkerList));

	if (g_savedLists.Get()->GetSize())
		AddToMenu(hMenu, SWS_SEPARATOR, 0);

	char str[256];
	for (int i = 0; i < g_savedLists.Get()->GetSize(); i++)
	{
		snprintf(str, sizeof(str), __LOCALIZE_VERFMT("Load %s","sws_DLG_102"), g_savedLists.Get()->Get(i)->m_name);
		AddToMenu(hMenu, str, FIRST_LOAD_MSG+i);
	}

	AddToMenu(hMenu, SWS_SEPARATOR, 0);
	AddToMenu(hMenu, __LOCALIZE("Copy marker set to clipboard","sws_DLG_102"), SWSGetCommandID(ListToClipboard));
	AddToMenu(hMenu, __LOCALIZE("Paste marker set from clipboard","sws_DLG_102"), SWSGetCommandID(ClipboardToList));
	AddToMenu(hMenu, SWS_SEPARATOR, 0);
	AddToMenu(hMenu, __LOCALIZE("Reorder marker IDs","sws_DLG_102"), SWSGetCommandID(RenumberIds));
	AddToMenu(hMenu, __LOCALIZE("Reorder region IDs","sws_DLG_102"), SWSGetCommandID(RenumberRegions));
	AddToMenu(hMenu, SWS_SEPARATOR, 0);
	AddToMenu(hMenu, __LOCALIZE("Delete selected marker(s)","sws_DLG_102"), DELETE_MSG);
	AddToMenu(hMenu, __LOCALIZE("Delete all markers","sws_DLG_102"), SWSGetCommandID(DeleteAllMarkers));
	AddToMenu(hMenu, __LOCALIZE("Delete all regions","sws_DLG_102"), SWSGetCommandID(DeleteAllRegions));
	AddToMenu(hMenu, SWS_SEPARATOR, 0);
	AddToMenu(hMenu, __LOCALIZE("Export formatted marker list to clipboard","sws_DLG_102"), SWSGetCommandID(ExportToClipboard));
	AddToMenu(hMenu, __LOCALIZE("Export formatted marker list to file","sws_DLG_102"), SWSGetCommandID(ExportToFile));
	AddToMenu(hMenu, __LOCALIZE("Export format...","sws_DLG_102"), SWSGetCommandID(ExportFormat));
	AddToMenu(hMenu, SWS_SEPARATOR, 0);
	AddToMenu(hMenu, __LOCALIZE("Convert markers to regions","sws_DLG_102"), SWSGetCommandID(MarkersToRegions));
	AddToMenu(hMenu, __LOCALIZE("Convert regions to markers","sws_DLG_102"), SWSGetCommandID(RegionsToMarkers));
	
	return hMenu;
}

void SWS_MarkerListWnd::OnDestroy()
{
	KillTimer(m_hwnd, 1);
	char cOptions[4];
	sprintf(cOptions, "%c %c", m_bPlayOnSel ? '1' : '0', m_bScroll ? '1' : '0');
	WritePrivateProfileString(SWS_INI, ML_OPTIONS_KEY, cOptions, get_ini_file());
}

void SWS_MarkerListWnd::OnTimer(WPARAM wParam)
{
	if (ListView_GetSelectedCount(m_pLists.Get(0)->GetHWND()) <= 1 || !IsActive())
		Update();
}

int SWS_MarkerListWnd::OnKey(MSG* msg, int iKeyState)
{
	if (msg->message == WM_KEYDOWN && !iKeyState)
	{
		if (msg->wParam == VK_DELETE)
		{
			// #1119, don't block Del key in Filter textbox
			HWND h = GetDlgItem(m_hwnd, IDC_FILTER);
#ifdef _WIN32
			if (msg->hwnd == h)
#else
			if (GetFocus() == h)
#endif
				return 0;
			else
			{ 
				OnCommand(DELETE_MSG, 0);
				return 1;
			}
		}
		else if (msg->wParam == VK_RETURN)
		{
			if (MarkerItem* mi = (MarkerItem*)m_pLists.Get(0)->EnumSelected(NULL)) {
				SetEditCurPos(mi->GetPos(), m_bScroll, m_bPlayOnSel);
				return 1;
			}
		}
/* JFB commented: generic impl. in sws_wnd.cpp
		else if (msg->wParam == VK_HOME)
		{
			for (int i = 0; i < ListView_GetItemCount(m_pLists.Get(0)->GetHWND()); i++)
				ListView_SetItemState(m_pLists.Get(0)->GetHWND(), i, i == 0 ? LVIS_SELECTED : 0, LVIS_SELECTED);
			return 1;
		}
		else if (msg->wParam == VK_END)
		{
			int count = ListView_GetItemCount(m_pLists.Get(0)->GetHWND());
			for (int i = 0; i < count; i++)
				ListView_SetItemState(m_pLists.Get(0)->GetHWND(), i, i == count-1 ? LVIS_SELECTED : 0, LVIS_SELECTED);
			return 1;
		}
*/
		else if (msg->wParam == VK_F2)
		{
			OnCommand(RENAME_MSG, 0);
			return 1;
		}
	}
	return 0;
}

INT_PTR WINAPI doSaveDialog(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwndDlg, uMsg, wParam, lParam))
		return r;

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			HWND list = GetDlgItem(hwndDlg, IDC_COMBO);
			WDL_UTF8_HookComboBox(list);
			for (int i = 0; i < g_savedLists.Get()->GetSize(); i++)
				SendMessage(list, CB_ADDSTRING, 0, (LPARAM)g_savedLists.Get()->Get(i)->m_name);
			RestoreWindowPos(hwndDlg, SAVEWINDOW_POS_KEY, false);
			return 0;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
				{
					char str[256];
					GetDlgItemText(hwndDlg, IDC_COMBO, str, 256);
					if (strlen(str) && EnumProjectMarkers(0, NULL, NULL, NULL, NULL, NULL))
					{	// Don't save a blank string, and don't save an empty list
						// Check to see if there's already a saved spot with that name (and overwrite if necessary)
						int i;
						for (i = 0; i < g_savedLists.Get()->GetSize(); i++)
							if (_stricmp(g_savedLists.Get()->Get(i)->m_name, str) == 0)
							{
								delete g_savedLists.Get()->Get(i);
								g_savedLists.Get()->Set(i, new MarkerList(str, true));
								return 0;
							}
						g_savedLists.Get()->Add(new MarkerList(str, true));
					}
				}
				// Fall through to cancel to save/end
				case IDCANCEL:
					SaveWindowPos(hwndDlg, SAVEWINDOW_POS_KEY);
					EndDialog(hwndDlg, 0);
					break;
			}
			break;
	}
	return 0;
}

INT_PTR WINAPI doLoadDialog(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwndDlg, uMsg, wParam, lParam))
		return r;

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			HWND list = GetDlgItem(hwndDlg, IDC_COMBO);
			WDL_UTF8_HookComboBox(list);
			for (int i = 0; i < g_savedLists.Get()->GetSize(); i++)
				SendMessage(list, CB_ADDSTRING, 0, (LPARAM)g_savedLists.Get()->Get(i)->m_name);
            SendMessage(list, CB_SETCURSEL, 0, 0);
			SetWindowText(hwndDlg, __LOCALIZE("Load Marker Set","sws_DLG_102"));
			RestoreWindowPos(hwndDlg, SAVEWINDOW_POS_KEY, false);
			return 0;
		}
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
				{
					HWND list = GetDlgItem(hwndDlg, IDC_COMBO);
					int iList = (int)SendMessage(list, CB_GETCURSEL, 0, 0);
					if (iList >= 0 && iList < g_savedLists.Get()->GetSize())
						g_savedLists.Get()->Get(iList)->UpdateReaper();
				}
				// Fall through to cancel to save/end
				case IDCANCEL:
					SaveWindowPos(hwndDlg, SAVEWINDOW_POS_KEY);
					EndDialog(hwndDlg, 0);
					break;
			}
			break;
	}
	return 0;
}

static INT_PTR WINAPI doDeleteDialog(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwndDlg, uMsg, wParam, lParam))
		return r;

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			HWND list = GetDlgItem(hwndDlg, IDC_COMBO);
			WDL_UTF8_HookComboBox(list);
			for (int i = 0; i < g_savedLists.Get()->GetSize(); i++)
				SendMessage(list, CB_ADDSTRING, 0, (LPARAM)g_savedLists.Get()->Get(i)->m_name);
            SendMessage(list, CB_SETCURSEL, 0, 0);
			SetWindowText(hwndDlg, __LOCALIZE("Delete Marker Set","sws_DLG_102"));
			RestoreWindowPos(hwndDlg, SAVEWINDOW_POS_KEY, false);
			return 0;
		}
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
				{
					HWND list = GetDlgItem(hwndDlg, IDC_COMBO);
					int iList = (int)SendMessage(list, CB_GETCURSEL, 0, 0);
					if (iList >= 0 && iList < g_savedLists.Get()->GetSize())
						g_savedLists.Get()->Delete(iList, true);
				}
				// Fall through to cancel to save/end
				case IDCANCEL:
					SaveWindowPos(hwndDlg, SAVEWINDOW_POS_KEY);
					EndDialog(hwndDlg, 0);
					break;
			}
			break;
	}
	return 0;
}

INT_PTR WINAPI doFormatDialog(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwndDlg, uMsg, wParam, lParam))
		return r;

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			HWND format = GetDlgItem(hwndDlg, IDC_EDIT);
			HWND desc = GetDlgItem(hwndDlg, IDC_DESC);
			char str[256];
			GetPrivateProfileString(SWS_INI, EXPORT_FORMAT_KEY, EXPORT_FORMAT_DEFAULT, str, 256, get_ini_file());
			SetWindowText(format, str);

			WDL_FastString helpStr;
			helpStr.Append(__LOCALIZE("Export marker list format string:\r\n","sws_DLG_102"));
			helpStr.Append(__LOCALIZE("First char one of a/r/m\r\n","sws_DLG_102"));
			helpStr.Append("  ");
			helpStr.Append(__LOCALIZE("(all / only regions / only markers)\r\n","sws_DLG_102"));
			helpStr.Append(__LOCALIZE("Then, in any order, n, i, l, d, t, s, p\r\n","sws_DLG_102"));
			helpStr.Append(__LOCALIZE("n = number (count), starts at 1\r\n","sws_DLG_102"));
			helpStr.Append(__LOCALIZE("i = ID\r\n","sws_DLG_102"));
			helpStr.Append(__LOCALIZE("l = Length in H:M:S\r\n","sws_DLG_102"));
			helpStr.Append(__LOCALIZE("d = Description\r\n","sws_DLG_102"));
			helpStr.Append(__LOCALIZE("t = Absolute time in H:M:S\r\n","sws_DLG_102"));
			helpStr.Append(__LOCALIZE("T = Absolute time in H:M:S.F\r\n","sws_DLG_102"));
			helpStr.Append(__LOCALIZE("s = Absolute time in project samples\r\n","sws_DLG_102"));
			helpStr.Append(__LOCALIZE("p = Absolute time in proj ruler format\r\n","sws_DLG_102"));
			helpStr.Append("\r\n");
			helpStr.Append(__LOCALIZE("You can include normal text in the format.\r\n","sws_DLG_102"));
			helpStr.Append(__LOCALIZE("If you want to use one of the above\r\n","sws_DLG_102"));
			helpStr.Append(__LOCALIZE("characters in normal text, preface it with \\","sws_DLG_102"));
			SetWindowText(desc, helpStr.Get());
			return 0;
		}
		case WM_COMMAND:
			if (LOWORD(wParam) == IDCANCEL)
				EndDialog(hwndDlg, 0);
			else if (LOWORD(wParam) == IDOK)
			{
				HWND format = GetDlgItem(hwndDlg, IDC_EDIT);
				char str[256];
				GetWindowText(format, str, 256);
				if (str[0] == 'a' || str[0] == 'r' || str[0] == 'm')
				{
					// Fix quotes in user string
					WDL_String fixedStr;
					fixedStr.SetFormatted(256, "\"%s\"", str);
					WritePrivateProfileString(SWS_INI, EXPORT_FORMAT_KEY, fixedStr.Get(), get_ini_file());
				}
				EndDialog(hwndDlg, 0);
			}
			break;
	}
	return 0;
}

void OpenMarkerList(COMMAND_T*)
{
	g_pMarkerList->Show(true, true);
}

void LoadMarkerList(COMMAND_T*)
{
	if (!g_savedLists.Get()->GetSize())
		MessageBox(g_hwndParent, __LOCALIZE("No marker sets available to load.","sws_DLG_102"), __LOCALIZE("SWS - Error","sws_mbox"), MB_OK);
	else
		DialogBox(g_hInst,MAKEINTRESOURCE(IDD_LOAD),g_hwndParent,doLoadDialog);
}

void SaveMarkerList(COMMAND_T*)
{
	DialogBox(g_hInst,MAKEINTRESOURCE(IDD_SAVE),g_hwndParent,doSaveDialog);
}

void DeleteMarkerList(COMMAND_T*)
{
	if (!g_savedLists.Get()->GetSize())
		MessageBox(g_hwndParent, __LOCALIZE("No marker sets available to delete.","sws_DLG_102"), __LOCALIZE("SWS - Error","sws_mbox"), MB_OK);
	else
		DialogBox(g_hInst,MAKEINTRESOURCE(IDD_LOAD),g_hwndParent,doDeleteDialog);
}

void ExportFormat(COMMAND_T*)
{
	DialogBox(g_hInst,MAKEINTRESOURCE(IDD_FORMAT),g_hwndParent,doFormatDialog);
}

int MarkerListEnabled(COMMAND_T*)
{
	return g_pMarkerList->IsWndVisible();
}

//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "SWS: Open marker list" },                                "SWSMARKERLIST1",      OpenMarkerList,    "SWS MarkerList", 0, MarkerListEnabled },
	{ { DEFACCEL, NULL }, NULL, NULL, SWS_SEPARATOR, },
	{ { DEFACCEL, "SWS: Load marker set" },                                 "SWSMARKERLIST2",      LoadMarkerList,    "Load marker set...",   },
	{ { DEFACCEL, "SWS: Save marker set" },                                 "SWSMARKERLIST3",      SaveMarkerList,    "Save marker set...",   },
	{ { DEFACCEL, "SWS: Delete marker set" },                               "SWSMARKERLIST4",      DeleteMarkerList,  "Delete marker set...", },
	{ { DEFACCEL, NULL }, NULL, NULL, SWS_SEPARATOR, },
	{ { DEFACCEL, "SWS: Copy marker set to clipboard" },                    "SWSMARKERLIST5",      ListToClipboard,   "Copy marker set to clipboard", },
	{ { DEFACCEL, "SWS: Copy markers in time selection to clipboard (relative to selection start)" },
	                                                                        "SWSML_TOCLIPTIMESEL", ListToClipboardTimeSel, },
	{ { DEFACCEL, "SWS: Paste marker set from clipboard" },                 "SWSMARKERLIST6",      ClipboardToList,   "Paste marker set from clipboard", },
	{ { DEFACCEL, NULL }, NULL, NULL, SWS_SEPARATOR, },
	{ { DEFACCEL, "SWS: Renumber marker IDs" },                             "SWSMARKERLIST7",      RenumberIds,       "Reorder marker IDs", },
	{ { DEFACCEL, "SWS: Renumber region IDs" },                             "SWSMARKERLIST8",      RenumberRegions,   "Reorder region IDs", },
	{ { DEFACCEL, NULL }, NULL, NULL, SWS_SEPARATOR, },
	{ { DEFACCEL, "SWS: Time-select next region" },                         "SWS_SELNEXTREG",      SelNextRegion,     "Select next region", },
	{ { DEFACCEL, "SWS: Time-select previous region" },                     "SWS_SELPREVREG",      SelPrevRegion,     "Select prev region", },
	{ { DEFACCEL, "SWS: Delete all markers" },                              "SWSMARKERLIST9",      DeleteAllMarkers,  "Delete all markers", },
	{ { DEFACCEL, "SWS: Delete all regions" },                              "SWSMARKERLIST10",     DeleteAllRegions,  "Delete all regions", },
	{ { DEFACCEL, NULL }, NULL, NULL, SWS_SEPARATOR, },
	{ { DEFACCEL, "SWS: Export formatted marker list to clipboard" },       "SWSMARKERLIST11",     ExportToClipboard, "Export formatted marker list to clipboard", },
	{ { DEFACCEL, "SWS: Export formatted marker list to file" },            "SWSML_EXPORTFILE",    ExportToFile, },
	{ { DEFACCEL, "SWS: Exported marker list format..." },                  "SWSMARKERLIST12",     ExportFormat,      "Export format...", },
	{ { DEFACCEL, NULL }, NULL, NULL, SWS_SEPARATOR, },
	{ { DEFACCEL, "SWS: Convert markers to regions" },                      "SWSMARKERLIST13",     MarkersToRegions,  "Convert markers to regions", },
	{ { DEFACCEL, "SWS: Convert regions to markers" },                      "SWSMARKERLIST14",     RegionsToMarkers,  "Convert regions to markers", },

	// no menu
	{ { DEFACCEL, "SWS: Go to end of project, including markers/regions" }, "SWS_PROJEND",         GotoEndInclMarkers, },
	{ { DEFACCEL, "SWS: Go to/time-select next marker/region" },            "SWS_SELNEXTMORR",     SelNextMarkerOrRegion, },
	{ { DEFACCEL, "SWS: Go to/time-select previous marker/region" },        "SWS_SELPREVMORR",     SelPrevMarkerOrRegion, },

	{ {}, LAST_COMMAND, }, // Denote end of table
};
//!WANT_LOCALIZE_1ST_STRING_END

static bool ProcessExtensionLine(const char *line, ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	LineParser lp(false);
	if (lp.parse(line) || lp.getnumtokens() < 1)
		return false;
	if (strcmp(lp.gettoken_str(0), "<MARKERLIST") != 0)
		return false; // only look for <MARKERLIST lines

	MarkerList* ml = g_savedLists.Get()->Add(new MarkerList(lp.gettoken_str(1), false));
  
	char linebuf[4096];
	while(true)
	{
		if (!ctx->GetLine(linebuf,sizeof(linebuf)) && !lp.parse(linebuf))
		{
			if (lp.getnumtokens() > 0 && lp.gettoken_str(0)[0] == '>')
				break;
			ml->m_items.Add(new MarkerItem(&lp));
		}
	}
	return true;
}

static void SaveExtensionConfig(ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	char str[512];
	for (int i = 0; i < g_savedLists.Get()->GetSize(); i++)
	{
		ctx->AddLine("<MARKERLIST \"%s\"", g_savedLists.Get()->Get(i)->m_name);
		for (int j = 0; j < g_savedLists.Get()->Get(i)->m_items.GetSize(); j++)
			ctx->AddLine("%s",g_savedLists.Get()->Get(i)->m_items.Get(j)->ItemString(str, 512));
		ctx->AddLine(">");
	}
}

static void BeginLoadProjectState(bool isUndo, struct project_config_extension_t *reg)
{
	g_savedLists.Get()->Empty(true);
	g_savedLists.Cleanup();
	g_pMarkerList->Update();
}

static project_config_extension_t g_projectconfig = { ProcessExtensionLine, SaveExtensionConfig, BeginLoadProjectState, NULL };

int MarkerListInit()
{
	if (!plugin_register("projectconfig",&g_projectconfig))
		return 0;

	SWSRegisterCommands(g_commandTable);
	g_pMarkerList = new SWS_MarkerListWnd();

	return 1;
}

void MarkerListExit()
{
	plugin_register("-projectconfig",&g_projectconfig);
	DELETE_NULL(g_pMarkerList);
}
