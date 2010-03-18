/******************************************************************************
/ sws_wnd.cpp
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

SWS_DockWnd::SWS_DockWnd(int iResource, const char* cName, int iDockOrder)
:m_hwnd(NULL), m_bDocked(false), m_iResource(iResource), m_cName(cName), m_iDockOrder(iDockOrder), m_bUserClosed(false)
{
	screenset_register((char*)m_cName, screensetCallback, this);

	m_ar.translateAccel = keyHandler;
	m_ar.isLocal = true;
	m_ar.user = this;
	plugin_register("accelerator", &m_ar);

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
	plugin_register("-accelerator", &m_ar);
}

void SWS_DockWnd::Show(bool bToggle, bool bActivate)
{
	if (!IsWindow(m_hwnd))
	{
		CreateDialogParam(g_hInst,MAKEINTRESOURCE(m_iResource),g_hwndParent,SWS_DockWnd::sWndProc,(LPARAM)this);
		if (m_bDocked && bActivate)
			DockWindowActivate(m_hwnd);
	}
	else if (!IsWindowVisible(m_hwnd) || (bActivate && !bToggle))
	{
		if (m_bDocked)
			DockWindowActivate(m_hwnd);
		else
			ShowWindow(m_hwnd, SW_SHOW);
		SetFocus(m_hwnd);
	}
	else if (bToggle)// If already visible close the window
		SendMessage(m_hwnd, WM_COMMAND, IDCANCEL, 0);
}

bool SWS_DockWnd::IsActive(bool bWantEdit)
{
	for (int i = 0; i < m_pLists.GetSize(); i++)
		if (m_pLists.Get(i)->IsActive(bWantEdit))
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
			for (int i = 0; i < m_pLists.GetSize(); i++)
				if (hdr->hwndFrom == m_pLists.Get(i)->GetHWND())
					return m_pLists.Get(i)->OnNotify(wParam, lParam);

			return OnNotify(wParam, lParam);

			/* for future coloring	if (s->hdr.code == LVN_ITEMCHANGING)
			{
				SetWindowLong(hwndDlg, DWL_MSGRESULT, TRUE);
				return TRUE;
			} 
			break;*/
		}
		case WM_CONTEXTMENU:
		{
			int x = LOWORD(lParam), y = HIWORD(lParam);
			// Are we over the column header?
			for (int i = 0; i < m_pLists.GetSize(); i++)
				if (m_pLists.Get(i)->DoColumnMenu(x, y))
					return 0;

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
		case WM_DROPFILES:
			OnDroppedFiles((HDROP)wParam);
			break;
		case WM_DESTROY:
			OnDestroy();
			for (int i = 0; i < m_pLists.GetSize(); i++)
				m_pLists.Get(i)->OnDestroy();

			if (!m_bDocked)
				SaveWindowPos(m_hwnd, m_cWndPosKey);

			DockWindowRemove(m_hwnd);
			char str[10];
			sprintf(str, "%d %d", m_bUserClosed ? 0 : 1, m_bDocked ? 1 : 0);
			WritePrivateProfileString(SWS_INI, m_cWndStateKey, str, get_ini_file());
			m_bUserClosed = false;
#ifdef _WIN32
			break;
		case WM_NCDESTROY:
#endif
			m_pLists.Empty(true);
			m_hwnd = NULL;
			break;
		default:
			return OnUnhandledMsg(uMsg, wParam, lParam);
	}
	return 0;
}

void SWS_DockWnd::ToggleDocking()
{
	if (!m_bDocked)
	{
		m_bDocked = true;
		SaveWindowPos(m_hwnd, m_cWndPosKey);
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

int SWS_DockWnd::keyHandler(MSG* msg, accelerator_register_t* ctx)
{
	SWS_DockWnd* p = (SWS_DockWnd*)ctx->user;
	if (p && p->IsActive(true))
	{
		// Check the listviews first to catch editing
		for (int i = 0; i < p->m_pLists.GetSize(); i++)
			if (p->m_pLists.Get(i)->GetHWND() == msg->hwnd || p->m_pLists.Get(i)->IsActive(true))
			{
				int iRet = p->m_pLists.Get(i)->KeyHandler(msg);
				if (iRet)
					return iRet;
				else
					break;
			}

		// Key wasn't handled by the listview(s), call the DockWnd
		int iKeys = GetAsyncKeyState(VK_CONTROL) & 0x8000 ? LVKF_CONTROL : 0;
		iKeys    |= GetAsyncKeyState(VK_MENU)    & 0x8000 ? LVKF_ALT     : 0;
		iKeys    |= GetAsyncKeyState(VK_SHIFT)   & 0x8000 ? LVKF_SHIFT   : 0;

		return p->OnKey(msg, iKeys);
	}
	return 0;
}

SWS_ListView::SWS_ListView(HWND hwndList, HWND hwndEdit, int iCols, SWS_LVColumn* pCols, const char* cINIKey, bool bTooltips)
:m_hwndList(hwndList), m_hwndEdit(hwndEdit), m_hwndTooltip(NULL), m_iSortCol(1), m_iEditingItem(-1), m_iEditingCol(0),
  m_iCols(iCols), m_pCols(NULL), m_pDefaultCols(pCols), m_bDisableUpdates(false), m_cINIKey(cINIKey),
#ifndef _WIN32
  m_pClickedItem(NULL)
#else
  m_dwSavedSelTime(0),m_bShiftSel(false)
#endif
{
	SetWindowLongPtr(hwndList, GWLP_USERDATA, (LONG_PTR)this);
	if (m_hwndEdit)
		SetWindowLongPtr(m_hwndEdit, GWLP_USERDATA, 0xdeadf00b);

	// Load sort and column data
	m_pCols = new SWS_LVColumn[m_iCols];
	memcpy(m_pCols, m_pDefaultCols, sizeof(SWS_LVColumn) * m_iCols);

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

	// Setup UTF8
	WDL_UTF8_HookListView(hwndList);

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
	delete [] m_pCols;
}

LPARAM SWS_ListView::GetListItem(int index, int* iState)
{
	if (index < 0)
		return NULL;
	LVITEM li;
	li.mask = LVIF_PARAM | (iState ? LVIF_STATE : 0);
	li.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
	li.iItem = index;
	li.iSubItem = 0;
	ListView_GetItem(m_hwndList, &li);
	if (iState)
		*iState = li.state;
	return li.lParam;
}

bool SWS_ListView::IsSelected(int index)
{
	if (index < 0)
		return false;
	return ListView_GetItemState(m_hwndList, index, LVIS_SELECTED) ? true : false;
}

LPARAM SWS_ListView::EnumSelected(int* i)
{
	int temp = 0;
	if (!i)
		i = &temp;
	LVITEM li;
	li.mask = LVIF_PARAM | LVIF_STATE;
	li.stateMask = LVIS_SELECTED;
	li.iSubItem = 0;

	while (*i < ListView_GetItemCount(m_hwndList))
	{
		li.iItem = (*i)++;
		ListView_GetItem(m_hwndList, &li);
		if (li.state)
			return li.lParam;
	}
	return NULL;
}

int SWS_ListView::OnNotify(WPARAM wParam, LPARAM lParam)
{
	NMLISTVIEW* s = (NMLISTVIEW*)lParam;

#ifdef _WIN32
	if (!m_bDisableUpdates && s->hdr.code == LVN_ITEMCHANGING && s->iItem >= 0 && (s->uNewState ^ s->uOldState) & LVIS_SELECTED)
	{
		// These calls are made in big groups, save the cur state on the first call
		// so we can restore it later if necessary
		if (GetTickCount() - m_dwSavedSelTime > 20)
		{
			m_pSavedSel.Resize(ListView_GetItemCount(m_hwndList), false);
			for (int i = 0; i < m_pSavedSel.GetSize(); i++)
				m_pSavedSel.Get()[i] = IsSelected(i);
			m_dwSavedSelTime = GetTickCount();
			m_bShiftSel = GetAsyncKeyState(VK_SHIFT) & 0x8000 ? true : false;
		}
		
		int iRet = OnItemSelChange(GetListItem(s->iItem), s->uNewState & LVIS_SELECTED ? true : false);
		SetWindowLongPtr(GetParent(m_hwndList), DWLP_MSGRESULT, iRet);
		return iRet;
	}
#endif
	if (!m_bDisableUpdates && s->hdr.code == LVN_ITEMCHANGED && s->iItem >= 0)
	{
		OnItemSelChanged(GetListItem(s->iItem), s->uNewState & LVIS_SELECTED ? true : false);
#ifndef _WIN32
		// See OSX comments in NM_CLICK below

		// Send the full compliment of OnItemSelChange messges, either from the saved array or the curent state
		if (m_pSavedSel.GetSize() && m_pSavedSel.GetSize() == ListView_GetItemCount(m_hwndList))
		{	// Restore the "correct" selection
			for (int i = 0; i < m_pSavedSel.GetSize(); i++)
				ListView_SetItemState(m_hwndList, i, m_pSavedSel.Get()[i] ? LVIS_SELECTED : 0, LVIS_SELECTED);
			m_pSavedSel.Resize(0, false);
		}
		else
		{
			// Send OnItemSelChange messages for everything on the list
			for (int i = 0; i < ListView_GetItemCount(m_hwndList); i++)
			{
				int iState;
				LPARAM item = GetListItem(i, &iState);
				OnItemSelChange(item, iState & LVIS_SELECTED);
			}
		}
	
		if (m_pClickedItem)
		{
			OnItemClk(m_pClickedItem, m_iClickedCol, m_iClickedKeys);
			m_pClickedItem = NULL;
		}
#endif
		return 0;
	}
	else if (s->hdr.code == NM_CLICK)
	{
		EditListItemEnd(true);
		int iDataCol = DisplayToDataCol(s->iSubItem);

		if (s->iItem >= 0 && m_pCols[iDataCol].iType & 2)
		{	// Clicked on a "clickable" column!
#ifdef _WIN32
			int iKeys = ((NMITEMACTIVATE*)lParam)->uKeyFlags;
			if ((GetTickCount() - m_dwSavedSelTime < 20 || (iKeys & LVKF_SHIFT)) && m_pSavedSel.GetSize() == ListView_GetItemCount(m_hwndList) && m_pSavedSel.Get()[s->iItem])
			{
				bool bSaveDisableUpdates = m_bDisableUpdates;
				m_bDisableUpdates = true;

				// If there's a valid saved selection, and the user clicked on a selected track, the restore that selection
				for (int i = 0; i < m_pSavedSel.GetSize(); i++)
				{
					OnItemSelChange(GetListItem(i), m_pSavedSel.Get()[i]);
					ListView_SetItemState(m_hwndList, i, m_pSavedSel.Get()[i] ? LVIS_SELECTED : 0, LVIS_SELECTED);
					// TODO store and set the FOCUSED bit as well??
				}

				m_bDisableUpdates = bSaveDisableUpdates;
			}
			else if (m_bShiftSel)
			{
				// Ignore shift if the selection changed (because of a shift key hit)
				iKeys &= ~LVKF_SHIFT;
				m_bShiftSel = false;
			}
			OnItemClk(GetListItem(s->iItem), iDataCol, iKeys);
#else
			// In OSX NM_CLICK comes *before* the changed notification.
			// Cases:
			// 1 - the user clicked on a non-selected item
			//     Call OnClk later, in the LVN_CHANGE handler!
			// 2 - one item is selected, and the user clicked on that.
			//     Call OnClk now, because no change will be sent later
			// 3 - more than one item is selected, user clicked a selected one
			//     Call OnClk now.  LVN_CHANGE is called later, and change the selection
			//     back to where it should be in that handler
			
			int iState;
			LPARAM item = GetListItem(s->iItem, &iState);
			m_iClickedKeys  = GetAsyncKeyState(VK_CONTROL) & 0x8000 ? LVKF_CONTROL : 0;
			m_iClickedKeys |= GetAsyncKeyState(VK_MENU)    & 0x8000 ? LVKF_ALT     : 0;
			m_iClickedKeys |= GetAsyncKeyState(VK_SHIFT)   & 0x8000 ? LVKF_SHIFT   : 0;
			
			// Case 1:
			if (!(iState & LVIS_SELECTED))
			{
				m_pClickedItem = item;
				m_iClickedCol = iDataCol;
				m_iClickedKeys &= (LVKF_ALT | LVKF_CONTROL); // Ignore shift later
				return 0;
			}
		
			if (ListView_GetSelectedCount(m_hwndList) == 1)
			{	// Case 2:
				OnItemClk(item, iDataCol, m_iClickedKeys);
				m_pSavedSel.Resize(0, false); // Saved sel size of zero means to not reset
			}
			else
			{	// Case 3:
				// Save the selections to be restored later
				m_pSavedSel.Resize(ListView_GetItemCount(m_hwndList), false);
				for (int i = 0; i < m_pSavedSel.GetSize(); i++)
					m_pSavedSel.Get()[i] = IsSelected(i);
				OnItemClk(item, iDataCol, m_iClickedKeys);
			}
#endif
		}
	}
	else if (s->hdr.code == NM_DBLCLK && s->iItem >= 0)
	{
		int iDataCol = DisplayToDataCol(s->iSubItem);
		if (iDataCol >= 0 && iDataCol < m_iCols && m_pCols[iDataCol].iType & 1)
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
	else if (s->hdr.code == LVN_BEGINDRAG)
	{
		OnBeginDrag();
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
	int iCols = 0;
	for (int i = 0; i < m_iCols; i++)
		if (m_pCols[i].iPos != -1)
			iCols++;
#ifdef _WIN32
	HWND header = ListView_GetHeader(m_hwndList);
	Header_GetOrderArray(header, iCols, &cols);
#else
	for (int i = 0; i < iCols; i++)
		cols[i] = i;
#endif
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

int SWS_ListView::KeyHandler(MSG *msg)
{
	if (msg->message == WM_KEYDOWN)
	{
		if (m_iEditingItem != -1)
		{
			bool bShift = GetAsyncKeyState(VK_SHIFT)   & 0x8000 ? true : false;

			if (msg->wParam == VK_ESCAPE)
			{
				EditListItemEnd(false);
				return 1;
			}
			else if (msg->wParam == VK_TAB)
			{
				int iItem = m_iEditingItem;
				DisableUpdates(true);
				EditListItemEnd(true, false);
				if (!bShift)
				{
					if (++iItem >= ListView_GetItemCount(m_hwndList))
						iItem = 0;
				}
				else
				{
					if (--iItem < 0)
						iItem = ListView_GetItemCount(m_hwndList) - 1;
				}
				EditListItem(GetListItem(iItem), m_iEditingCol);
				DisableUpdates(false);
				return 1;
			}
			else if (msg->wParam == VK_RETURN)
			{
				EditListItemEnd(true);
				return 1;
			}
		}

		// can override these in accelerators that are instantiated earlier, if you like!
		// Catch a few keys that are handled natively by the list view
		else if (msg->wParam == VK_UP || msg->wParam == VK_DOWN || msg->wParam == VK_TAB)
		{
			return -1;
		}
	}
		
	return 0;
}

void SWS_ListView::Update()
{
	// Fill in the data by pulling it from the derived class
	if (m_iEditingItem == -1 && !m_bDisableUpdates)
	{
		m_bDisableUpdates = true;
		char str[256];

		WDL_TypedBuf<LPARAM> items;
		GetItemList(&items);

		// Check for deletions - items in the lstwnd are quite likely out of order so gotta do a full O(n^2) search
		int lvItemCount = ListView_GetItemCount(m_hwndList);
		for (int i = 0; i < lvItemCount; i++)
		{
			LPARAM item = GetListItem(i);
			bool bFound = false;
			for (int j = 0; j < items.GetSize(); j++)
				if (items.Get()[j] == item)
				{
					bFound = true;
					break;
				}

			if (!bFound)
			{
				ListView_DeleteItem(m_hwndList, i--);
				lvItemCount--;
			}
		}

		// Check for additions
		lvItemCount = ListView_GetItemCount(m_hwndList);
		for (int i = 0; i < items.GetSize(); i++)
		{
			bool bFound = false;
			int j;
			for (j = 0; j < lvItemCount; j++)
			{
				if (items.Get()[i] == GetListItem(j))
				{
					bFound = true;
					break;
				}
			}

			// Update the list, no matter what, because text may have changed
			LVITEM item;
			item.mask = LVIF_TEXT | LVIF_PARAM;
			int iNewState = GetItemState(items.Get()[i]);
			if (iNewState >= 0)
			{
				int iCurState = ListView_GetItemState(m_hwndList, j, LVIS_SELECTED | LVIS_FOCUSED);
				if (iNewState && !(iCurState & LVIS_SELECTED))
				{
					item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
					item.state = LVIS_SELECTED;
					item.stateMask = LVIS_SELECTED;
				}
				else if (!iNewState && (iCurState & LVIS_SELECTED))
				{
					item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
					item.state = 0;
					item.stateMask = LVIS_SELECTED | ((iCurState & LVIS_FOCUSED) ? LVIS_FOCUSED : 0);
				}
			}

			item.iItem = j;
			item.lParam = items.Get()[i];
			item.pszText = str;
			
			int iCol = 0;
			for (int k = 0; k < m_iCols; k++)
				if (m_pCols[k].iPos != -1)
				{
					item.iSubItem = iCol;
					GetItemText(item.lParam, k, str, 256);
					if (!iCol && !bFound)
					{
						ListView_InsertItem(m_hwndList, &item);
						lvItemCount++;
					}
					else
						ListView_SetItem(m_hwndList, &item);
					item.mask = LVIF_TEXT;
					iCol++;
				}
			}

		ListView_SortItems(m_hwndList, sListCompare, (LPARAM)this);
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
		m_bDisableUpdates = false;
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
				memcpy(m_pCols, m_pDefaultCols, sizeof(SWS_LVColumn) * m_iCols);
				int iPos = 0;
				for (int i = 0; i < m_iCols; i++)
					if (m_pCols[i].iPos != -1)
						m_pCols[i].iPos = iPos++;
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
	POINT pt = { x, y };
	ht.pt = pt;
	ht.flags = LVHT_ONITEM;
	ScreenToClient(m_hwndList, &ht.pt);
	int iItem = ListView_SubItemHitTest(m_hwndList, &ht);
#ifdef _WIN32
	RECT r;
	HWND header = ListView_GetHeader(m_hwndList);
	GetWindowRect(header, &r);
	if (PtInRect(&r, pt))
#else
	if (ht.pt.y < 0)
#endif
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
#ifdef _WIN32
	LVFINDINFO fi;
	fi.flags = LVFI_PARAM;
	fi.lParam = item;
	int iItem = ListView_FindItem(m_hwndList, -1, &fi);
#else
	int iItem = -1;
	LVITEM li;
	li.mask = LVIF_PARAM;
	for (int i = 0; i < ListView_GetItemCount(m_hwndList); i++)
	{
		li.iItem = i;
		ListView_GetItem(m_hwndList, &li);
		if (li.lParam == item)
		{
			iItem = i;
			break;
		}
	}
#endif
	if (iItem >= 0)
		EditListItem(iItem, iCol);
}

void SWS_ListView::EditListItem(int iIndex, int iCol)
{
	RECT r;
	m_iEditingItem = iIndex;
	m_iEditingCol = iCol;
	int iDispCol = DataToDisplayCol(iCol);
	ListView_GetSubItemRect(m_hwndList, iIndex, iDispCol, LVIR_LABEL, &r);

#ifdef _WIN32
	RECT sr = r;
	ClientToScreen(m_hwndList, (LPPOINT)&sr);
	ClientToScreen(m_hwndList, ((LPPOINT)&sr)+1);

	HWND hDlg = GetParent(m_hwndEdit);
	ScreenToClient(hDlg, (LPPOINT)&sr);
	ScreenToClient(hDlg, ((LPPOINT)&sr)+1);

	// Create a new edit control to go over that rect
	int lOffset = 0;
	if (iDispCol)
		lOffset += GetSystemMetrics(SM_CXEDGE) * 2;

	SetWindowPos(m_hwndEdit, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	SetWindowPos(m_hwndEdit, HWND_TOP, sr.left+lOffset, sr.top, sr.right-sr.left, sr.bottom-sr.top, SWP_SHOWWINDOW | SWP_NOZORDER);
#else
	SetWindowPos(m_hwndEdit, HWND_TOP, r.left, r.top-1, r.right-r.left, r.bottom-r.top+2, 0);
	ShowWindow(m_hwndEdit, SW_SHOW);
#endif

	LPARAM item = GetListItem(iIndex);
	char str[100];
	GetItemText(item, iCol, str, 100);
	SetWindowText(m_hwndEdit, str);
	SetFocus(m_hwndEdit);
	SendMessage(m_hwndEdit, EM_SETSEL, 0, -1);
}

void SWS_ListView::EditListItemEnd(bool bSave, bool bResort)
{
	if (m_iEditingItem != -1 && bSave && IsWindow(m_hwndList) && IsWindow(m_hwndEdit))
	{
		char newStr[100];
		char curStr[100];
		GetWindowText(m_hwndEdit, newStr, 100);
		LPARAM item = GetListItem(m_iEditingItem);
		GetItemText(item, m_iEditingCol, curStr, 100);
		if (strcmp(curStr, newStr))
		{
			SetItemText(item, m_iEditingCol, newStr);
			GetItemText(item, m_iEditingCol, newStr, 100);
			ListView_SetItemText(m_hwndList, m_iEditingItem, DataToDisplayCol(m_iEditingCol), newStr);
		}
		if (bResort)
			ListView_SortItems(m_hwndList, sListCompare, (LPARAM)this);
		// TODO resort? Just call update?
		// Update is likely called when SetItemText is called too...
		m_iEditingItem = -1;
		ShowWindow(m_hwndEdit, SW_HIDE);
	}
}

int SWS_ListView::OnItemSort(LPARAM item1, LPARAM item2)
{
	// Just sort by string
	char str1[64];
	char str2[64];
	GetItemText(item1, abs(m_iSortCol)-1, str1, 64);
	GetItemText(item2, abs(m_iSortCol)-1, str2, 64);

	// If strings are purely numbers, sort numerically
	char* pEnd1, *pEnd2;
	int i1 = strtol(str1, &pEnd1, 0);
	int i2 = strtol(str2, &pEnd2, 0);
	int iRet = 0;
	if ((i1 || i2) && !*pEnd1 && !*pEnd2)
	{
		if (i1 > i2)
			iRet = 1;
		else if (i1 < i2)
			iRet = -1;
	}
	else
		iRet = strcmp(str1, str2);
	
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
	int iCols = 0;
	for (int i = 0; i < m_iCols; i++)
		if (m_pCols[i].iPos != -1)
			cols[iCols++] = m_pCols[i].iPos;
#ifdef _WIN32
	HWND header = ListView_GetHeader(m_hwndList);
	Header_SetOrderArray(header, iCols, &cols);
#endif
}

void SWS_ListView::SetListviewColumnArrows(int iSortCol)
{
#ifdef _WIN32
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
#else
	SetColumnArrows(m_hwndList, iSortCol);
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