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
#include "../Utility/SectionLock.h"
#include "MarkerListClass.h"
#include "MarkerList.h"
#include "MarkerListActions.h"

#define SAVEWINDOW_POS_KEY "Markerlist Save Window Position"
#define ML_OPTIONS_KEY "MarkerlistOptions"

#define DELETE_MSG		0x100F0
#define FIRST_LOAD_MSG	0x10100

// Globals
static SWSProjConfig<WDL_PtrList<MarkerList> > g_savedLists;
MarkerList* g_curList = NULL;
SWS_MarkerListWnd* g_pMarkerList = NULL;

static SWS_LVColumn g_cols[] = { { 75, 0, "Time" }, { 45, 0, "Type" }, { 30, 0, "ID" }, { 170, 1, "Description" } };

SWS_MarkerListView::SWS_MarkerListView(HWND hwndList, HWND hwndEdit, SWS_MarkerListWnd* pList)
:SWS_ListView(hwndList, hwndEdit, 4, g_cols, "MarkerList View State", false), m_pMarkerList(pList)
{
}

void SWS_MarkerListView::GetItemText(LPARAM item, int iCol, char* str, int iStrMax)
{
	MarkerItem* mi = (MarkerItem*)item;
	if (mi)
	{
		switch (iCol)
		{
		case 0:
			format_timestr_pos(mi->m_dPos, str, iStrMax, -1);
			break;
		case 1:
			_snprintf(str, iStrMax, "%s", mi->m_bReg ? "Region" : "Marker");
			break;
		case 2:
			_snprintf(str, iStrMax, "%d", mi->m_id);
			break;
		case 3:
			lstrcpyn(str, mi->GetName(), iStrMax);
			break;
		}
	}
}

void SWS_MarkerListView::OnItemSelChanged(LPARAM item, int iState)
{
	if (iState & LVIS_FOCUSED)
	{
		MarkerItem* mi = (MarkerItem*)item;
		if (mi->m_dPos != GetCursorPosition())
		{
			SetEditCurPos(mi->m_dPos, m_pMarkerList->m_bScroll, false); // Never play here, do it from the clk handler
			m_pMarkerList->m_dCurPos = mi->m_dPos; // Save pos
		}
	}
}

void SWS_MarkerListView::OnItemClk(LPARAM item, int iCol, int iKeyState)
{
	MarkerItem* mi = (MarkerItem*)item;
	if (mi) // Doubled-up calls to SetEditCurPos - oh well!
		SetEditCurPos(mi->m_dPos, m_pMarkerList->m_bScroll, m_pMarkerList->m_bPlayOnSel);
}

void SWS_MarkerListView::OnItemDblClk(LPARAM item, int iCol)
{
	MarkerItem* mi = (MarkerItem*)item;
	if (mi->m_bReg)
		GetSet_LoopTimeRange(true, true, &mi->m_dPos, &mi->m_dRegEnd, m_pMarkerList->m_bPlayOnSel);
}

int SWS_MarkerListView::OnItemSort(LPARAM item1, LPARAM item2)
{
	int iRet;
	MarkerItem* mi1 = (MarkerItem*)item1;
	MarkerItem* mi2 = (MarkerItem*)item2;

	if (abs(m_iSortCol) == 1)
	{
		if (mi1->m_dPos > mi2->m_dPos)
			iRet = 1;
		else if (mi1->m_dPos < mi2->m_dPos)
			iRet = -1;
		if (m_iSortCol < 0)
			return -iRet;
		else
			return iRet;
	}
	else if (abs(m_iSortCol) == 3)
	{
		if (mi1->m_id > mi2->m_id)
			iRet = 1;
		else if (mi1->m_id < mi2->m_id)
			iRet = -1;
		if (m_iSortCol < 0)
			return -iRet;
		else
			return iRet;
	}
	return SWS_ListView::OnItemSort(item1, item2);
}

void SWS_MarkerListView::SetItemText(LPARAM item, int iCol, const char* str)
{
	if (iCol == 3)
	{
		MarkerItem* mi = (MarkerItem*)item;
		mi->SetName(str);
		SetProjectMarker(mi->m_id, mi->m_bReg, mi->m_dPos, mi->m_dRegEnd, str);
		Update();
	}
}

void SWS_MarkerListView::GetItemList(WDL_TypedBuf<LPARAM>* pBuf)
{
	if (m_pMarkerList->m_filter.GetLength())
	{
		int iCount = 0;
		LineParser lp(false);
		lp.parse(m_pMarkerList->m_filter.Get());
		for (int i = 0; i < g_curList->m_items.GetSize(); i++)
			for (int j = 0; j < lp.getnumtokens(); j++)
				if (stristr(g_curList->m_items.Get(i)->GetName(), lp.gettoken_str(j)))
				{
					pBuf->Resize(++iCount);
					pBuf->Get()[iCount-1] = (LPARAM)g_curList->m_items.Get(i);
					break;
				}
	}
	else
	{
		pBuf->Resize(g_curList->m_items.GetSize(), false);
		for (int i = 0; i < pBuf->GetSize(); i++)
			pBuf->Get()[i] = (LPARAM)g_curList->m_items.Get(i);
	}
}

int SWS_MarkerListView::GetItemState(LPARAM item)
{
	MarkerItem* mi = (MarkerItem*)item;
	return GetCursorPosition() == mi->m_dPos ? 1 : 0;
}

SWS_MarkerListWnd::SWS_MarkerListWnd()
:SWS_DockWnd(IDD_MARKERLIST, "Marker List", 30001, SWSGetCommandID(OpenMarkerList)), m_dCurPos(DBL_MAX)
{
	if (m_bShowAfterInit)
		Show(false, false);
}

void SWS_MarkerListWnd::Update(bool bForce)
{
	// Change the time string if the project time mode changes
	static int prevTimeMode = -1;
	bool bChanged = bForce;
	if (*(int*)GetConfigVar("projtimemode") != prevTimeMode)
	{
		prevTimeMode = *(int*)GetConfigVar("projtimemode");
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
		SectionLock lock(g_curList->m_hLock);
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
	bool m_bPlayOnDblClk = cOptions[0] == '1';
	bool m_bScroll = cOptions[2] == '1';

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
				LVITEM li;
				li.mask = LVIF_STATE | LVIF_PARAM;
				li.stateMask = LVIS_SELECTED;
				li.iSubItem = 0;
				for (int i = 0; i < ListView_GetItemCount(m_pLists.Get(0)->GetHWND()); i++)
				{
					li.iItem = i;
					ListView_GetItem(m_pLists.Get(0)->GetHWND(), &li);
					if (li.state == LVIS_SELECTED)
					{
						MarkerItem* item = (MarkerItem*)li.lParam;
						DeleteProjectMarker(NULL, item->m_id, item->m_bReg);
					}
				}
				Undo_EndBlock("Delete marker(s)", UNDO_STATE_MISCCFG);
				Update();
				break;
			}
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

HMENU SWS_MarkerListWnd::OnContextMenu(int x, int y)
{
	HMENU hMenu = CreatePopupMenu();
	AddToMenu(hMenu, "Save marker set...", SWSGetCommandID(SaveMarkerList));
	AddToMenu(hMenu, "Delete market set...", SWSGetCommandID(DeleteMarkerList));

	if (g_savedLists.Get()->GetSize())
		AddToMenu(hMenu, SWS_SEPARATOR, 0);

	char str[256];
	for (int i = 0; i < g_savedLists.Get()->GetSize(); i++)
	{
		sprintf(str, "Load %s", g_savedLists.Get()->Get(i)->m_name);
		AddToMenu(hMenu, str, FIRST_LOAD_MSG+i);
	}

	AddToMenu(hMenu, SWS_SEPARATOR, 0);
	AddToMenu(hMenu, "Copy marker set to clipboard", SWSGetCommandID(ListToClipboard));
	AddToMenu(hMenu, "Paste marker set from clipboard", SWSGetCommandID(ClipboardToList));
	AddToMenu(hMenu, SWS_SEPARATOR, 0);
	AddToMenu(hMenu, "Reorder marker IDs", SWSGetCommandID(RenumberIds));
	AddToMenu(hMenu, "Reorder region IDs", SWSGetCommandID(RenumberRegions));
	AddToMenu(hMenu, SWS_SEPARATOR, 0);
	AddToMenu(hMenu, "Delete selected marker(s)", DELETE_MSG);
	AddToMenu(hMenu, "Delete all markers", SWSGetCommandID(DeleteAllMarkers));
	AddToMenu(hMenu, "Delete all regions", SWSGetCommandID(DeleteAllRegions));
	AddToMenu(hMenu, SWS_SEPARATOR, 0);
	AddToMenu(hMenu, "Export formatted marker list to clipboard", SWSGetCommandID(ExportToClipboard));
	AddToMenu(hMenu, "Export format...", SWSGetCommandID(ExportFormat));
	
	return hMenu;
}

void SWS_MarkerListWnd::OnDestroy()
{
	KillTimer(m_hwnd, 1);
	char cOptions[4];
	sprintf(cOptions, "%c %c", m_bPlayOnSel ? '1' : '0', m_bScroll ? '1' : '0');
	WritePrivateProfileString(SWS_INI, ML_OPTIONS_KEY, cOptions, get_ini_file());
}

void SWS_MarkerListWnd::OnTimer()
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
			OnCommand(DELETE_MSG, 0);
			return 1;
		}
		else if (msg->wParam == VK_RETURN)
		{
			MarkerItem* mi = (MarkerItem*)m_pLists.Get(0)->EnumSelected(NULL);
			if (mi)
				SetEditCurPos(mi->m_dPos, m_bScroll, m_bPlayOnSel);
			return 1;
		}
	}
	return 0;
}

INT_PTR WINAPI doSaveDialog(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
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
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			HWND list = GetDlgItem(hwndDlg, IDC_COMBO);
			WDL_UTF8_HookComboBox(list);
			for (int i = 0; i < g_savedLists.Get()->GetSize(); i++)
				SendMessage(list, CB_ADDSTRING, 0, (LPARAM)g_savedLists.Get()->Get(i)->m_name);
            SendMessage(list, CB_SETCURSEL, 0, 0);
			SetWindowText(hwndDlg, "Load Marker Set");
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
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			HWND list = GetDlgItem(hwndDlg, IDC_COMBO);
			WDL_UTF8_HookComboBox(list);
			for (int i = 0; i < g_savedLists.Get()->GetSize(); i++)
				SendMessage(list, CB_ADDSTRING, 0, (LPARAM)g_savedLists.Get()->Get(i)->m_name);
            SendMessage(list, CB_SETCURSEL, 0, 0);
			SetWindowText(hwndDlg, "Delete Marker Set");
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
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			HWND format = GetDlgItem(hwndDlg, IDC_EDIT);
			HWND desc = GetDlgItem(hwndDlg, IDC_DESC);
			char str[256];
			GetPrivateProfileString(SWS_INI, EXPORT_FORMAT_KEY, EXPORT_FORMAT_DEFAULT, str, 256, get_ini_file());
			SetWindowText(format, str);
			SetWindowText(desc,
				"Export marker list format string:\r\n"
				"First char one of a/r/m\r\n"
				"  (all / only regions / only markers)\r\n"
				"Then, in any order, n, i, l, d, t, s\r\n"
				"n = number (count), starts at 1\r\n"
				"i = ID\r\n"
				"l = Length in H:M:S\r\n"
				"d = Description\r\n"
				"t = Absolute time in H:M:S\r\n"
				"s = Absolute time in project samples\r\n"
				"\r\n"
				"You can include normal text in the format.\r\n"
				"If you want to use one of the above\r\n"
				"characters in normal text, preface it with \\"
				);
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
					WritePrivateProfileString(SWS_INI, EXPORT_FORMAT_KEY, str, get_ini_file());
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
		MessageBox(g_hwndParent, "No marker sets available to load.", "Error", MB_OK);
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
		MessageBox(g_hwndParent, "No marker sets available to delete.", "Error", MB_OK);
	else
		DialogBox(g_hInst,MAKEINTRESOURCE(IDD_LOAD),g_hwndParent,doDeleteDialog);
}

void ExportFormat(COMMAND_T*)
{
	DialogBox(g_hInst,MAKEINTRESOURCE(IDD_FORMAT),g_hwndParent,doFormatDialog);
}

static bool MarkerListEnabled(COMMAND_T*)
{
	return g_pMarkerList->IsValidWindow();
}

static COMMAND_T g_commandTable[] = 
{
	{ { { FSHIFT | FCONTROL | FVIRTKEY, 'M', 0 }, "SWS: Open marker list" },				"SWSMARKERLIST1",  OpenMarkerList,    "SWS MarkerList", 0, MarkerListEnabled },
	{ { DEFACCEL, NULL }, NULL, NULL, SWS_SEPARATOR, },
	{ { DEFACCEL,                                "SWS: Load marker set" },					"SWSMARKERLIST2",  LoadMarkerList,    "Load marker set...",   },
	{ { DEFACCEL,                                "SWS: Save marker set" },					"SWSMARKERLIST3",  SaveMarkerList,    "Save marker set...",   },
	{ { DEFACCEL,                                "SWS: Delete marker set" },				"SWSMARKERLIST4",  DeleteMarkerList,  "Delete marker set...", },
	{ { DEFACCEL, NULL }, NULL, NULL, SWS_SEPARATOR, },
	{ { DEFACCEL,                                "SWS: Copy marker set to clipboard" },		"SWSMARKERLIST5",  ListToClipboard,   "Copy marker set to clipboard", },
	{ { DEFACCEL,                                "SWS: Copy markers in time selection to clipboard (relative to selection start)" },	"SWSML_TOCLIPTIMESEL",  ListToClipboardTimeSel, NULL /*  "Copy markers in time selection to clipboard (relative to selection start)"*/, },
	{ { DEFACCEL,                                "SWS: Paste marker set from clipboard" },	"SWSMARKERLIST6",  ClipboardToList,   "Paste marker set from clipboard", },
	{ { DEFACCEL, NULL }, NULL, NULL, SWS_SEPARATOR, },
	{ { DEFACCEL,								"SWS: Renumber marker IDs" },				"SWSMARKERLIST7",  RenumberIds,		  "Reorder marker IDs", },
	{ { DEFACCEL,								"SWS: Renumber region IDs" },				"SWSMARKERLIST8",  RenumberRegions,   "Reorder region IDs", },
	{ { DEFACCEL, NULL }, NULL, NULL, SWS_SEPARATOR, },
	{ { DEFACCEL,								"SWS: Select next region" },				"SWS_SELNEXTREG",  SelNextRegion,     "Select next region", },
	{ { DEFACCEL,								"SWS: Select previous region" },			"SWS_SELPREVREG",  SelPrevRegion,     "Select prev region", },
	{ { DEFACCEL,								"SWS: Delete all markers" },				"SWSMARKERLIST9",  DeleteAllMarkers,  "Delete all markers", },
	{ { DEFACCEL,								"SWS: Delete all regions" },				"SWSMARKERLIST10", DeleteAllRegions,  "Delete all regions", },
	{ { DEFACCEL, NULL }, NULL, NULL, SWS_SEPARATOR, },
	{ { DEFACCEL,								"SWS: Export formatted marker list to clipboard" },	"SWSMARKERLIST11", ExportToClipboard, "Export formatted marker list to clipboard", },
	{ { DEFACCEL,								"SWS: Exported marker list format..." },		"SWSMARKERLIST12",	ExportFormat,      "Export format...", },

	// no menu
	{ { DEFACCEL, "SWS: Go to end of project, including markers/regions" },					"SWS_PROJEND",		GotoEndInclMarkers, },

	{ {}, LAST_COMMAND, }, // Denote end of table
};

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
			else if (lp.getnumtokens() == 5)
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
			ctx->AddLine(g_savedLists.Get()->Get(i)->m_items.Get(j)->ItemString(str, 512));
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

static void menuhook(const char* menustr, HMENU hMenu, int flag)
{
	if (strcmp(menustr, "Main view") == 0 && flag == 0)
		AddToMenu(hMenu, g_commandTable[0].menuText, g_commandTable[0].accel.accel.cmd, 40075);
	else if (strcmp(menustr, "Main edit") == 0 && flag == 0)
		AddSubMenu(hMenu, SWSCreateMenu(g_commandTable), "SWS Marker utilites");
}

int MarkerListInit()
{
	if (!plugin_register("projectconfig",&g_projectconfig))
		return 0;

	SWSRegisterCommands(g_commandTable);

	if (!plugin_register("hookcustommenu", (void*)menuhook))
		return 0;

	g_pMarkerList = new SWS_MarkerListWnd();

	return 1;
}

void MarkerListExit()
{
	delete g_pMarkerList;
}
