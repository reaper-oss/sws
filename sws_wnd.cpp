/******************************************************************************
/ sws_wnd.cpp
/
/ Copyright (c) 2009 Tim Payne (SWS)
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


/* From askjf.com:
    * It's often the easiest to build your application out of plain dialog (DialogBoxParam() or CreateDialogParam())
    * Use Get/SetWindowLong(hwnd,GWL_USERDATA) (or GWLP_USERDATA for x64) for "this" pointers, if an object owns the dialog. Set it in WM_INITDIALOG, and get it for other messages.
    * On the same token, when the owner object has a dialog, it's good to do the following:
          o The object's constructor sets m_hwnd to 0.
          o The object's destructor does if (m_hwnd) DestroyWindow(m_hwnd);
          o WM_INITDIALOG sets this->m_hwnd to hwndDlg
          o WM_DESTROY clears this->m_hwnd to 0
*/

#include "stdafx.h"

#define ECOFFSET_HEIGHT		2

SWS_DockWnd::SWS_DockWnd(int iResource, const char* cName, int iDockOrder)
:m_hwnd(NULL), m_bDocked(false), m_iResource(iResource), m_cName(cName), m_iDockOrder(iDockOrder), m_bUserClosed(false), m_pList(NULL)
{
	screenset_register((char*)m_cName, screensetCallback, this);

	m_cWndPosKey = new char[strlen(m_cName)+8];
	sprintf(m_cWndPosKey, "%s WndPos", m_cName);
	m_cWndStateKey = new char[strlen(m_cName)+7];
	sprintf(m_cWndStateKey, "%s State", m_cName);

	// Restore state
	char str[10];
	GetPrivateProfileString(SWS_INI, m_cWndStateKey, "0 0", str, 10, get_ini_file());
	if (strlen(str) == 3)
	{
		m_bDocked  = str[2] == '1';
		m_bShowAfterInit = str[0] == '1';
	}
}

SWS_DockWnd::~SWS_DockWnd()
{
	delete m_cWndPosKey;
	delete m_cWndStateKey;
}

void SWS_DockWnd::Show(bool bToggle, bool bActivate)
{
	if (!IsWindow(m_hwnd))
	{
		CreateDialogParam(g_hInst,MAKEINTRESOURCE(m_iResource),g_hwndParent,SWS_DockWnd::sWndProc,(LPARAM)this);
		if (m_bDocked && bActivate)
			DockWindowActivate(m_hwnd);
	}
	else if (!IsWindowVisible(m_hwnd))
	{
		if (m_bDocked)
			DockWindowActivate(m_hwnd);
		else
			ShowWindow(m_hwnd, SW_SHOW);
	}
	else if (bToggle)// If already visible close the window
		SendMessage(m_hwnd, WM_COMMAND, IDCANCEL, 0);
}

bool SWS_DockWnd::IsActive()
{
	if (m_pList && m_pList->IsActive())
		return true;
	return GetFocus() == m_hwnd;
}

INT_PTR WINAPI SWS_DockWnd::sWndProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SWS_DockWnd* pObj = (SWS_DockWnd*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	if (!pObj && uMsg == WM_INITDIALOG)
	{
		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
		pObj = (SWS_DockWnd*)lParam;
		pObj->m_hwnd = hwndDlg;
	}
	if (pObj)
		return pObj->wndProc(uMsg, wParam, lParam);
	else
		return 0;
}

int SWS_DockWnd::wndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
	        m_resize.init(m_hwnd);

			// Call derived class initialization
			OnInitDlg();

			if (m_bDocked)
			{
				DockWindowAdd(m_hwnd, (char*)m_cName, m_iDockOrder, true);
			}
			else
			{
				RestoreWindowPos(m_hwnd, m_cWndPosKey);
				ShowWindow(m_hwnd, SW_SHOW);
			}

			break;
		}
		case WM_TIMER:
			OnTimer();
			break;
		case WM_NOTIFY:
		{
			NMHDR* hdr = (NMHDR*)lParam;
			if (m_pList && hdr->hwndFrom == m_pList->GetHWND())
				return m_pList->OnNotify(wParam, lParam);
			else
				return 0;

			/* for future coloring	if (s->hdr.code == LVN_ITEMCHANGING)
			{
				SetWindowLong(hwndDlg, DWL_MSGRESULT, TRUE);
				return TRUE;
			} */
			break;
		}
		case WM_CONTEXTMENU:
		{
			int x = LOWORD(lParam), y = HIWORD(lParam);
			// Are we over the column header?
			if (m_pList && m_pList->DoColumnMenu(x, y))
				break;

			HMENU hMenu = OnContextMenu(x, y);
			if (!hMenu)
				hMenu = CreatePopupMenu();
			else
				AddToMenu(hMenu, SWS_SEPARATOR, 0);

			// Add std menu items
			char str[100];
			sprintf(str, "Dock %s in Docker", m_cName);
			AddToMenu(hMenu, str, DOCK_MSG);
			// Check dock state
			if (m_bDocked)
				CheckMenuItem(hMenu, DOCK_MSG, MF_BYCOMMAND | MF_CHECKED);
			AddToMenu(hMenu, "Close Window", IDCANCEL);

			int xPos = (short)LOWORD(lParam), yPos = (short)HIWORD(lParam);
			if (xPos == -1 || yPos == -1)
			{
				RECT r;
				GetWindowRect(m_hwnd, &r);
				xPos = r.left;
				yPos = r.top;
			}

			kbd_reprocessMenu(hMenu, NULL);
			TrackPopupMenu(hMenu, 0, xPos, yPos, 0, m_hwnd, NULL);
			DestroyMenu(hMenu);

			break;
		}
		case WM_COMMAND:
			OnCommand(wParam, lParam);

			switch (wParam)
			{
			case DOCK_MSG:
				ToggleDocking();
				break;
			case IDCANCEL:
			case IDOK:
				m_bUserClosed = true;
				DestroyWindow(m_hwnd);
				break;
			}
			break;
		case WM_SIZE:
			if (wParam != SIZE_MINIMIZED)
			{
				OnResize();
				m_resize.onResize();
			}
			break;
		case WM_DESTROY:
			OnDestroy();

			if (!m_bDocked)
				SaveWindowPos(m_hwnd, m_cWndPosKey);

			DockWindowRemove(m_hwnd);
			char str[10];
			sprintf(str, "%d %d", m_bUserClosed ? 0 : 1, m_bDocked ? 1 : 0);
			WritePrivateProfileString(SWS_INI, m_cWndStateKey, str, get_ini_file());
			m_bUserClosed = false;
			break;
		case WM_NCDESTROY:
			delete m_pList;
			m_pList = NULL;
			m_hwnd = NULL;
			break;
	}
	return 0;
}

void SWS_DockWnd::ToggleDocking()
{
	if (!m_bDocked)
	{
		m_bDocked = true;
		ShowWindow(m_hwnd, SW_HIDE);
		DockWindowAdd(m_hwnd, (char*)m_cName, m_iDockOrder, false);
		DockWindowActivate(m_hwnd);
	}
	else
	{
		DestroyWindow(m_hwnd);
		m_bDocked = false;
		Show(false, true);
	}
}

LPARAM SWS_DockWnd::screensetCallback(int action, char *id, void *param, int param2)
{
	SWS_DockWnd* pObj = (SWS_DockWnd*)param;
	switch(action)
	{
	case SCREENSET_ACTION_GETHWND:
		return (int)pObj->m_hwnd;
	case SCREENSET_ACTION_IS_DOCKED:
		return (int)pObj->m_bDocked;
	case SCREENSET_ACTION_SHOW:
		if (IsWindow(pObj->m_hwnd))
			DestroyWindow(pObj->m_hwnd);
		pObj->m_bDocked = param2 ? true : false;
		pObj->Show(false, true);
		break;
	case SCREENSET_ACTION_CLOSE:
		if (IsWindow(pObj->m_hwnd))
		{
			pObj->m_bUserClosed = true;
			DestroyWindow(pObj->m_hwnd);
		}
		break;
	case SCREENSET_ACTION_SWITCH_DOCK:
		if (IsWindow(pObj->m_hwnd))
			pObj->ToggleDocking();
		break;
	case SCREENSET_ACTION_NOMOVE:
	case SCREENSET_ACTION_GETHASH:
		return 0;
	}
	return 0;
}

SWS_ListView::SWS_ListView(HWND hwndList, HWND hwndEdit, int iCols, SWS_LVColumn* pCols, const char* cINIKey, bool bTooltips)
:m_hwndList(hwndList), m_hwndEdit(hwndEdit), m_hwndTooltip(NULL), m_iSortCol(1), m_iEditingItem(-1), m_iEditingCol(0),
 m_iCols(iCols), m_pCols(pCols), m_bDisableUpdates(false), m_cINIKey(cINIKey)
{
	SetWindowLongPtr(hwndList, GWLP_USERDATA, (LONG_PTR)this);
	// Setup the edit control procedure
	WNDPROC prevEditProc = (WNDPROC)GetWindowLongPtr(hwndEdit, GWLP_WNDPROC);
	if (prevEditProc != (WNDPROC)sEditProc)
	{
		m_prevEditProc = prevEditProc;
		SetWindowLongPtr(hwndEdit, GWLP_WNDPROC, (LONG_PTR)sEditProc);
	}
	else
		m_prevEditProc = NULL;

	SetWindowLongPtr(m_hwndEdit, GWLP_USERDATA, 0xdeadf00b);

	// Load sort and column data
	char cDefaults[256];
	sprintf(cDefaults, "%d", m_iSortCol);
	int iPos = 0;
	for (int i = 0; i < m_iCols; i++)
		_snprintf(cDefaults + strlen(cDefaults), 64-strlen(cDefaults), " %d %d", m_pCols[i].iWidth, m_pCols[i].iPos != -1 ? iPos++ : -1);
	char str[256];
	GetPrivateProfileString(SWS_INI, m_cINIKey, cDefaults, str, 256, get_ini_file());
	LineParser lp(false);
	if (!lp.parse(str))
	{
		m_iSortCol = lp.gettoken_int(0);
		iPos = 0;
		for (int i = 0; i < m_iCols; i++)
		{
			int iWidth = lp.gettoken_int(i*2+1);
			if (iWidth)
			{
				m_pCols[i].iWidth = lp.gettoken_int(i*2+1);
				m_pCols[i].iPos = lp.gettoken_int(i*2+2);
				iPos = m_pCols[i].iPos;
			}
			else if (m_pCols[i].iPos != -1) // new cols are invisible?
			{
				m_pCols[i].iPos = iPos++;
			}
		}
	}

#ifdef _WIN32
	ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);

	// Create the tooltip window (if it's necessary)
	if (bTooltips)
	{
		m_hwndTooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,                
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hwndList, NULL, g_hInst, NULL );
		SetWindowPos(m_hwndTooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}
#endif
	
	ShowColumns();

	// Set colors
	//ListView_SetBkColor(hwndList, GSC_mainwnd(COLOR_WINDOW));
	//ListView_SetTextBkColor(hwndList, GSC_mainwnd(COLOR_WINDOW));
	//ListView_SetTextColor(hwndList, GSC_mainwnd(COLOR_BTNTEXT));

	// Need to call Update(); when ready elsewhere
}

SWS_ListView::~SWS_ListView()
{
}

LPARAM SWS_ListView::GetListItem(int index)
{
	if (index < 0)
		return NULL;
	LVITEM li;
	li.mask = LVIF_PARAM;
	li.iItem = index;
	li.iSubItem = 0;
	ListView_GetItem(m_hwndList, &li);
	return li.lParam;
}

int SWS_ListView::OnNotify(WPARAM wParam, LPARAM lParam)
{
	NMLISTVIEW* s = (NMLISTVIEW*)lParam;

	if (s->hdr.code == LVN_ITEMCHANGING && s->iItem >= 0 && (s->uNewState ^ s->uOldState) & LVIS_SELECTED)
	{
		// NOTE this is called *before* click, so if you need to decide
		// what to do based on where you've clicked (e.g tracklist)
		// then OnItemSelChange should always return false.
		int iRet = OnItemSelChange(GetListItem(s->iItem), s->uNewState & LVIS_SELECTED ? true : false);
		SetWindowLongPtr(GetParent(m_hwndList), DWLP_MSGRESULT, iRet);
		return iRet;
	}
	else if (s->hdr.code == NM_CLICK)
	{
		ShowWindow(m_hwndEdit, SW_HIDE);
		OnItemClk(GetListItem(s->iItem), DisplayToDataCol(s->iSubItem));
	}
	else if (s->hdr.code == NM_DBLCLK && s->iItem >= 0)
	{
		int iDataCol = DisplayToDataCol(s->iSubItem);
		if (iDataCol >= 0 && iDataCol < m_iCols && m_pCols[iDataCol].bEditable)
			EditListItem(s->iItem, iDataCol);
		else
			OnItemDblClk(GetListItem(s->iItem), iDataCol);
	}
	else if (s->hdr.code == LVN_COLUMNCLICK)
	{
		int iDataCol = DisplayToDataCol(s->iSubItem);
		if (iDataCol+1 == abs(m_iSortCol))
			m_iSortCol = -m_iSortCol;
		else
			m_iSortCol = iDataCol + 1;
		Update();
	}
/*	else if (s->hdr.code == NM_CUSTOMDRAW) // TODO for coloring of the listview
	{
		LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;

		switch(lplvcd->nmcd.dwDrawStage) 
		{
			case CDDS_PREPAINT : //Before the paint cycle begins
				SetWindowLong(hwndDlg, DWL_MSGRESULT, CDRF_NOTIFYITEMDRAW);
				return 1;
			case CDDS_ITEMPREPAINT: //Before an item is drawn
				lplvcd->clrText = GSC_mainwnd(COLOR_BTNTEXT);
				lplvcd->clrTextBk = GSC_mainwnd(COLOR_WINDOW);
				lplvcd->clrFace = RGB(0,0,0);
				SetWindowLong(hwndDlg, DWL_MSGRESULT, CDRF_NEWFONT);
				return 1;
			default:
				SetWindowLong(hwndDlg, DWL_MSGRESULT, CDRF_DODEFAULT);
				return 1;
		}
	}*/
	return 0;
}

void SWS_ListView::OnDestroy()
{
	// Save cols
	char str[256];
	int cols[20];
	HWND header = ListView_GetHeader(m_hwndList);
	int iCols = 0;
	for (int i = 0; i < m_iCols; i++)
		if (m_pCols[i].iPos != -1)
			iCols++;
	Header_GetOrderArray(header, iCols, &cols);
	iCols = 0;

	for (int i = 0; i < m_iCols; i++)
		if (m_pCols[i].iPos != -1)
			m_pCols[i].iPos = cols[iCols++];
		
	sprintf(str, "%d", m_iSortCol);

	iCols = 0;
	for (int i = 0; i < m_iCols; i++)
		if (m_pCols[i].iPos >= 0)
			_snprintf(str + strlen(str), 256-strlen(str), " %d %d", ListView_GetColumnWidth(m_hwndList, iCols++), m_pCols[i].iPos);
		else
			_snprintf(str + strlen(str), 256-strlen(str), " %d %d", m_pCols[i].iWidth, m_pCols[i].iPos);

	WritePrivateProfileString(SWS_INI, m_cINIKey, str, get_ini_file());

	if (m_hwndTooltip)
	{
		DestroyWindow(m_hwndTooltip);
		m_hwndTooltip = NULL;
	}
}

void SWS_ListView::Update()
{
	// Fill in the data by pulling it from the derived class
	if (m_iEditingItem == -1 && !m_bDisableUpdates)
	{
		char str[256];

		// Check for deletions - items in the lstwnd are quite likely out of order so gotta do a full O(n^2) search
		for (int i = 0; i < ListView_GetItemCount(m_hwndList); i++)
		{
			LPARAM item = GetListItem(i);
			bool bFound = false;
			for (int j = 0; j < GetItemCount(); j++)
				if (GetItemPointer(j) == item)
				{
					bFound = true;
					break;
				}

			if (!bFound)
				ListView_DeleteItem(m_hwndList, i--);
		}

		// Check for additions
		for (int i = 0; i < GetItemCount(); i++)
		{
			bool bFound = false;
			int j;
			for (j = 0; j < ListView_GetItemCount(m_hwndList); j++)
			{
				if (GetItemPointer(i) == GetListItem(j))
				{
					bFound = true;
					break;
				}
			}

			LVITEM item;
			item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
			item.iItem = j;
			item.lParam = GetItemPointer(i);
			item.pszText = str;
			item.state = GetItemState(item.lParam) ? LVIS_SELECTED | LVIS_FOCUSED : 0;
			item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
			
			int iCol = 0;
			for (int k = 0; k < m_iCols; k++)
				if (m_pCols[k].iPos != -1)
				{
					item.iSubItem = iCol;
					GetItemText(item.lParam, k, str, 256);
					if (!iCol && !bFound)
						ListView_InsertItem(m_hwndList, &item);
					else
						ListView_SetItem(m_hwndList, &item);
					item.mask = LVIF_TEXT;
					iCol++;
				}
		}
		ListView_SortItems(m_hwndList, sListCompare, this);
		int iCol = abs(m_iSortCol) - 1;
		iCol = DataToDisplayCol(iCol) + 1;
		if (m_iSortCol < 0)
			iCol = -iCol;
		SetListviewColumnArrows(iCol);

#ifdef _WIN32
		if (m_hwndTooltip)
		{
			TOOLINFO ti = { sizeof(TOOLINFO), };
			ti.lpszText = str;
			ti.hwnd = m_hwndList;
			ti.uFlags = TTF_SUBCLASS;
			ti.hinst  = g_hInst;

			// Delete all existing tools
			while (SendMessage(m_hwndTooltip, TTM_ENUMTOOLS, 0, (LPARAM)&ti))
				SendMessage(m_hwndTooltip, TTM_DELTOOL, 0, (LPARAM)&ti);

			RECT r;
			// Add tooltips after sort
			for (int i = 0; i < ListView_GetItemCount(m_hwndList); i++)
			{
				// Get the rect of the line
				ListView_GetItemRect(m_hwndList, i, &r, LVIR_BOUNDS);
				memcpy(&ti.rect, &r, sizeof(RECT));
				ti.uId = i;
				GetItemTooltip(GetListItem(i), str, 100);
				SendMessage(m_hwndTooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);
			}
		}
#endif
	}
}

// Return TRUE if a the column header was clicked
bool SWS_ListView::DoColumnMenu(int x, int y)
{
	int iCol;
	LPARAM item = GetHitItem(x, y, &iCol);
	if (!item && iCol != -1)
	{
		HMENU hMenu = CreatePopupMenu();
		AddToMenu(hMenu, "Visible columns", 0);
		EnableMenuItem(hMenu, 0, MF_BYPOSITION | MF_GRAYED);

		for (int i = 0; i < m_iCols; i++)
		{
			AddToMenu(hMenu, m_pCols[i].cLabel, i + 1);
			if (m_pCols[i].iPos != -1)
				CheckMenuItem(hMenu, i+1, MF_BYPOSITION | MF_CHECKED);
		}
		AddToMenu(hMenu, SWS_SEPARATOR, 0);
		AddToMenu(hMenu, "Reset", m_iCols + 1);

		int iCol = TrackPopupMenu(hMenu, TPM_RETURNCMD, x, y, 0, m_hwndList, NULL);
		DestroyMenu(hMenu);

		if (iCol)
		{
			iCol--;
			if (iCol == m_iCols)
			{
				for (int i = 0; i < m_iCols; i++)
					m_pCols[i].iPos = i;
			}
			else
			{
				// Save all visible column widths
				for (int i = 0; i < m_iCols; i++)
					if (m_pCols[i].iPos != -1)
						m_pCols[i].iWidth = ListView_GetColumnWidth(m_hwndList, DataToDisplayCol(i));

				if (m_pCols[iCol].iPos == -1)
				{	// Adding column
					for (int i = 0; i < m_iCols; i++)
						if (m_pCols[i].iPos >= iCol)
							m_pCols[i].iPos++;
					m_pCols[iCol].iPos = iCol;
				}
				else
				{	// Deleting column
					int iDelPos = m_pCols[iCol].iPos;
					m_pCols[iCol].iPos = -1;
					for (int i = 0; i < m_iCols; i++)
						if (m_pCols[i].iPos > iDelPos)
							m_pCols[i].iPos--;

					// Fix the sort column
					if (abs(m_iSortCol) - 1 == iCol)
						for (int i = 0; i < m_iCols; i++)
							if (m_pCols[i].iPos != -1)
							{
								m_iSortCol = i+1;
								break;
								// Possible to break out leaving the sort column pointing to
								// an invisible col if there's no columns shown!
							}
				}
			}

			ListView_DeleteAllItems(m_hwndList);
			while(ListView_DeleteColumn(m_hwndList, 0));
			ShowColumns();
			Update();
		}
		return true;
	}
	return false;
}

LPARAM SWS_ListView::GetHitItem(int x, int y, int* iCol)
{
	LVHITTESTINFO ht;
	ht.pt.x = x;
	ht.pt.y = y;
	ht.flags = LVHT_ONITEM;
	ScreenToClient(m_hwndList, &ht.pt);
	int iItem = ListView_SubItemHitTest(m_hwndList, &ht);
	HWND header = ListView_GetHeader(m_hwndList);
	RECT r;
	GetWindowRect(header, &r);
	if (x >= r.left && x < r.right && y >= r.top && y < r.bottom)
	{
		if (iCol)
			*iCol = ht.iSubItem != -1 ? ht.iSubItem : 0; // iCol != -1 means "header", set 0 for "unknown column"
		return NULL;
	}
	else if (iItem >= 0)
	{
		if (iCol)
			*iCol = ht.iSubItem;
		return GetListItem(iItem);
	}
	if (iCol)
		*iCol = -1; // -1 + NULL ret means "no mans land"
	return NULL;
}

void SWS_ListView::EditListItem(LPARAM item, int iCol)
{
	// Convert to index and call edit
	LVFINDINFO fi;
	fi.flags = LVFI_PARAM;
	fi.lParam = item;

	int iItem = ListView_FindItem(m_hwndList, -1, &fi);
	if (iItem >= 0)
		EditListItem(iItem, iCol);
}

void SWS_ListView::EditListItem(int iIndex, int iCol)
{
#ifdef _WIN32
	RECT r;
	m_iEditingItem = iIndex;
	m_iEditingCol = iCol;
	ListView_GetSubItemRect(m_hwndList, iIndex, DataToDisplayCol(iCol), LVIR_LABEL, &r);

	// Create a new edit control to go over that rect
	// Ridiculous code just to get the proper offsets
	RECT rList, rDlg, rDlg2;
	TITLEBARINFO tb = { sizeof(TITLEBARINFO), };
	GetWindowRect(m_hwndList, &rList);
	GetWindowRect(GetParent(m_hwndList), &rDlg);
	GetClientRect(GetParent(m_hwndList), &rDlg2);
	GetTitleBarInfo(GetParent(m_hwndList), &tb);
	int lOffset, tOffset;
	if (tb.rgstate[0] & STATE_SYSTEM_INVISIBLE)
	{
		lOffset = rList.left - rDlg.left + 5;
		tOffset = rList.top  - rDlg.top + 2;
	}
	else
	{
		lOffset = rList.left - rDlg.left + 1;
		tOffset = rList.top  - rDlg.top - (tb.rcTitleBar.bottom - tb.rcTitleBar.top) - 2;
	}
	if (iCol == 1 && m_pCols[0].iPos == -1)
		lOffset -= 4; // WTF, when the first column is hidden (for whatever reason), editing is offset.  Huh.
	SetWindowPos(m_hwndEdit, HWND_TOP, r.left+lOffset, r.top+tOffset, r.right-r.left, r.bottom-r.top+ECOFFSET_HEIGHT, 0);
	ShowWindow(m_hwndEdit, SW_SHOW);
	SetWindowLongPtr(m_hwndEdit, GWLP_USERDATA, 0xdeadf00b);

	LPARAM item = GetListItem(iIndex);
	char str[100];
	GetItemText(item, iCol, str, 100);
	SetWindowText(m_hwndEdit, str);
	SendMessage(m_hwndEdit, EM_SETSEL, 0, -1);
	SetFocus(m_hwndEdit);
#endif
}

LRESULT SWS_ListView::sEditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// The EDIT control needs to have special USERDATA value for Reaper, so get the pointer to the class
	// from the list control.  This is a total hack because what if the list isn't IDC_LIST, or there's
	// more than one list?  Hmm...
	HWND hDlg = GetParent(hwnd);
	HWND hList = GetDlgItem(hDlg, IDC_LIST);
	SWS_ListView* pObj = (SWS_ListView*)GetWindowLongPtr(hList, GWLP_USERDATA);
	if (pObj)
		return pObj->editProc(uMsg, wParam, lParam);
	else
		return 0;
}

LRESULT SWS_ListView::editProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
#ifdef _WIN32
	if (uMsg == WM_GETDLGCODE)
		return DLGC_WANTALLKEYS;
#endif
	if (uMsg == WM_KEYDOWN && wParam == VK_TAB)
	{		
		int iNext = m_iEditingItem + 1;
		int count = ListView_GetItemCount(m_hwndList);
		if (iNext >= count)
			iNext = 0;

		m_bDisableUpdates = true;
		ShowWindow(m_hwndEdit, SW_HIDE);  // Forces KILLFOCUS message which saves edit
		EditListItem(iNext, m_iEditingCol);
		m_bDisableUpdates = false;
	}
	else if (uMsg == WM_KEYDOWN && wParam == VK_ESCAPE)
	{
		m_iEditingItem = -1;
		ShowWindow(m_hwndEdit, SW_HIDE);
	}
	else if (m_iEditingItem >= 0 && (uMsg == WM_KILLFOCUS || (uMsg == WM_KEYDOWN && wParam == VK_RETURN)))
	{
		if (IsWindow(m_hwndList) && IsWindow(m_hwndEdit))
		{
			LPARAM item = GetListItem(m_iEditingItem); // Get the item before hiding the window and setting g_iEI to -1
			int i = m_iEditingItem;
			m_iEditingItem = -1;

			char newStr[100];
			char curStr[100];
			GetWindowText(m_hwndEdit, newStr, 100);
			GetItemText(item, m_iEditingCol, curStr, 100);
			if (strcmp(curStr, newStr))
			{
				ListView_SetItemText(m_hwndList, i, DataToDisplayCol(m_iEditingCol), newStr);
				SetItemText(item, m_iEditingCol, newStr);
			}
			ShowWindow(m_hwndEdit, SW_HIDE);
			Update();
		}
		else
			m_iEditingItem = -1;
	}

	if (m_prevEditProc)
		return CallWindowProc(m_prevEditProc, m_hwndEdit, uMsg, wParam, lParam);
	else
		return 0;
}

int SWS_ListView::OnItemSort(LPARAM item1, LPARAM item2)
{
	// Just sort by string
	char str1[64];
	char str2[64];
	GetItemText(item1, abs(m_iSortCol)-1, str1, 64);
	GetItemText(item2, abs(m_iSortCol)-1, str2, 64);

	int iRet = strcmp(str1, str2);
	if (m_iSortCol < 0)
		return -iRet;
	else
		return iRet;
}

void SWS_ListView::ShowColumns()
{
	LVCOLUMN col;
	col.mask = LVCF_TEXT | LVCF_WIDTH;

	int iCol = 0;
	for (int i = 0; i < m_iCols; i++)
	{
		if (m_pCols[i].iPos >= 0)
		{
			col.pszText = (char*)m_pCols[i].cLabel;
			col.cx = m_pCols[i].iWidth;
			ListView_InsertColumn(m_hwndList, iCol, &col);
			iCol++;
		}
	}

	int cols[20];
	HWND header = ListView_GetHeader(m_hwndList);
	int iCols = 0;
	for (int i = 0; i < m_iCols; i++)
		if (m_pCols[i].iPos != -1)
			cols[iCols++] = m_pCols[i].iPos;
	Header_SetOrderArray(header, iCols, &cols);
}

void SWS_ListView::SetListviewColumnArrows(int iSortCol)
{
#if defined(_WIN32) && (_MSC_VER > 1310)
	// Set the column arrows
	HWND header = ListView_GetHeader(m_hwndList);
	for (int i = 0; i < Header_GetItemCount(header); i++)
	{
		HDITEM hi;
		hi.mask = HDI_FORMAT | HDI_BITMAP;
		Header_GetItem(header, i, &hi);
		if (hi.hbm)
			DeleteObject(hi.hbm);
		hi.fmt &= ~(HDI_BITMAP|HDF_BITMAP_ON_RIGHT|HDF_SORTDOWN|HDF_SORTUP);

		if (IsCommCtrlVersion6())
		{
			if (iSortCol == i+1)
                hi.fmt |= HDF_SORTUP;
			else if (-iSortCol == i+1)
                hi.fmt |= HDF_SORTDOWN;
		}
		else
		{
			if (iSortCol == i+1)
			{
				hi.fmt |= HDF_BITMAP|HDF_BITMAP_ON_RIGHT;
				hi.hbm = (HBITMAP)LoadImage(g_hInst, MAKEINTRESOURCE(IDB_UP), IMAGE_BITMAP, 0,0, LR_LOADMAP3DCOLORS);
			}
			else if (-iSortCol == i+1)
			{
				hi.fmt |= HDF_BITMAP|HDF_BITMAP_ON_RIGHT;
				hi.hbm = (HBITMAP)LoadImage(g_hInst, MAKEINTRESOURCE(IDB_DOWN), IMAGE_BITMAP, 0,0, LR_LOADMAP3DCOLORS);
			}
		}
		Header_SetItem(header, i, &hi);
	}
#endif
}

int SWS_ListView::DisplayToDataCol(int iCol)
{	// The display column # can be potentially less than the data column if there are hidden columns
	if (iCol < 0)
		return iCol;
	int iVis = 0;
	int iDataCol = iCol;
	for (int i = 0; i < m_iCols && iVis <= iCol; i++)
		if (m_pCols[i].iPos == -1)
			iDataCol++;
		else
			iVis++;
	return iDataCol;
}

int SWS_ListView::DataToDisplayCol(int iCol)
{	// The data col # can be more than the display col if there are hidden columns
	for (int i = 0; i < iCol; i++)
		if (m_pCols[i].iPos == -1)
			iCol--;
	return iCol;
}

int SWS_ListView::sListCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lSortParam)
{
	SWS_ListView* pLV = (SWS_ListView*)lSortParam;
	return pLV->OnItemSort(lParam1, lParam2);
}