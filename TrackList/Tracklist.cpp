/******************************************************************************
/ Tracklist.cpp
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


#include "stdafx.h"
#include "../Freeze/Freeze.h"
#include "../Snapshots/SnapshotClass.h"
#include "../Snapshots/Snapshots.h"
#include "TracklistFilter.h"
#include "Tracklist.h"

#define DOCK_MSG		0x10004
#define RENAME_MSG		0x10005
#define SELPREV_MSG		0x1000B
#define SELNEXT_MSG		0x1000C
#define LOADSNAP_MSG	0x10100 // Keep space afterwards

#define MAJORADJUST false
	
// Globals
static COMMAND_T* g_pCommandTable;
static SWS_TrackListWnd* g_pList;

// Prototypes
void OpenTrackList(COMMAND_T* = NULL);
void ShowInMCP(COMMAND_T* = NULL);
void ShowInTCP(COMMAND_T* = NULL);
void HideFromMCP(COMMAND_T* = NULL);
void HideFromTCP(COMMAND_T* = NULL);
void ShowInMCPOnly(COMMAND_T* = NULL);
void ShowInTCPOnly(COMMAND_T* = NULL);
void ShowInMCPandTCP(COMMAND_T* = NULL);
void HideTracks(COMMAND_T* = NULL);
void ShowSelOnly(COMMAND_T* = NULL);
void ShowInTCPEx(COMMAND_T* = NULL);
void ShowInMCPEx(COMMAND_T* = NULL);
void HideUnSel(COMMAND_T* = NULL);
void ShowAll(COMMAND_T* = NULL);
void NewVisSnapshot(COMMAND_T* = NULL);

enum TL_COLS { COL_NUM, COL_NAME, COL_TCP, COL_MCP, COL_ARM, COL_MUTE, COL_SOLO, /*COL_INPUT, */ NUM_COLS };

static SWS_LVColumn g_cols[] = { { 25, 0, "#" }, { 250, 1, "Name" }, { 40, 0, "TCP" }, { 40, 0, "MCP" },
	{ 40, 0, "Arm", -1 },  { 40, 0, "Mute", -1 }, { 40, 0, "Solo", -1 } /*, { 40, 0, "Input", -1 } */ };

SWS_TrackListView::SWS_TrackListView(HWND hwndList, HWND hwndEdit, SWS_TrackListWnd* pTrackListWnd)
:SWS_ListView(hwndList, hwndEdit, NUM_COLS, g_cols, "TrackList View State", false), m_pTrackListWnd(pTrackListWnd)
{
}

void SWS_TrackListView::GetItemText(LPARAM item, int iCol, char* str, int iStrMax)
{
	MediaTrack* tr = (MediaTrack*)item;
	if (tr)
	{
		switch (iCol)
		{
		case COL_NUM: // #
			_snprintf(str, iStrMax, "%d", CSurf_TrackToID(tr, false));
			break;
		case COL_NAME: // Name
			lstrcpyn(str, (char*)GetSetMediaTrackInfo(tr, "P_NAME", NULL), iStrMax);
			break;
		case COL_TCP: // TCP
			lstrcpyn(str, GetTrackVis(tr) & 2 ? "*" : "", iStrMax);
			break;
		case COL_MCP: // MCP
			lstrcpyn(str, GetTrackVis(tr) & 1 ? "*" : "", iStrMax);
			break;
		case COL_ARM:
			lstrcpyn(str, *(int*)GetSetMediaTrackInfo(tr, "I_RECARM", NULL) ? "*" : "", iStrMax);
			break;
		case COL_MUTE:
			lstrcpyn(str, *(bool*)GetSetMediaTrackInfo(tr, "B_MUTE", NULL) ? "*" : "", iStrMax);
			break;
		case COL_SOLO:
			lstrcpyn(str, *(int*)GetSetMediaTrackInfo(tr, "I_SOLO", NULL) ? "*" : "", iStrMax);
			break;
//		case COL_INPUT:
//			_snprintf(str, iStrMax, "%d", *(int*)GetSetMediaTrackInfo(tr, "I_RECINPUT", NULL) + 1);
//			break;
		}
	}
}

void SWS_TrackListView::OnItemClk(LPARAM item, int iCol)
{
	bool bCtrl  = GetAsyncKeyState(VK_CONTROL) & 0x8000 ? true : false;
	bool bAlt   = GetAsyncKeyState(VK_MENU)    & 0x8000 ? true : false;
	bool bShift = GetAsyncKeyState(VK_SHIFT)   & 0x8000 ? true : false;

	MediaTrack* tr = (MediaTrack*)item;

	if (!tr || iCol == COL_NUM || iCol == COL_NAME || !*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
	{
		bShift = false; // override shift
		// Update the track selections
		m_bDisableUpdates = true;
		for (int i = 0; i < ListView_GetItemCount(m_hwndList); i++)
		{
			int iNewState = ListView_GetItemState(m_hwndList, i, LVIS_SELECTED) ? 1 : 0;
			MediaTrack* trSel =(MediaTrack*)GetListItem(i);
			if (iNewState != *(int*)GetSetMediaTrackInfo(trSel, "I_SELECTED", NULL))
				GetSetMediaTrackInfo(trSel, "I_SELECTED", &iNewState);
		}
		m_bDisableUpdates = false;
	}

	if (tr && iCol == COL_TCP || iCol == COL_MCP)
	{
		bool bClickedStar = ((iCol == COL_TCP && GetTrackVis(tr) & 2) || 
							 (iCol == COL_MCP && GetTrackVis(tr) & 1));
		m_bDisableUpdates = true;

		if (m_pTrackListWnd->Linked() && !bShift)
		{
			if (bCtrl && bAlt)
				ShowSelOnly();
			else if (bClickedStar)
				HideTracks();
			else
				ShowInMCPandTCP();
		}
		else if (iCol == COL_TCP)
		{
			if (bCtrl && bAlt)
				ShowInTCPEx();
			else if (bClickedStar)
				HideFromTCP();
			else
				ShowInTCP();
		}
		else // iCol == COL_MCP
		{
			if (bCtrl && bAlt)
				ShowInMCPEx();
			else if (bClickedStar)
				HideFromMCP();
			else
				ShowInMCP();
		}
		m_bDisableUpdates = false;
		m_pTrackListWnd->Update();
	}
	else if (iCol == COL_MUTE)
		Main_OnCommand(6, 0);
	else if (iCol == COL_SOLO)
		Main_OnCommand(7, 0);
	else if (iCol == COL_ARM)
		Main_OnCommand(9, 0);
}

int SWS_TrackListView::OnItemSort(LPARAM item1, LPARAM item2)
{
	if (abs(m_iSortCol) == 1)
	{
		int iRet;
		MediaTrack* tr1 = (MediaTrack*)item1;
		MediaTrack* tr2 = (MediaTrack*)item2;
		if (CSurf_TrackToID(tr1, false) > CSurf_TrackToID(tr2, false))
			iRet = 1;
		else if (CSurf_TrackToID(tr1, false) < CSurf_TrackToID(tr2, false))
			iRet = -1;
		if (m_iSortCol < 0)
			return -iRet;
		else
			return iRet;
	}
	return SWS_ListView::OnItemSort(item1, item2);
}

void SWS_TrackListView::SetItemText(LPARAM item, int iCol, const char* str)
{
	if (iCol == 1)
	{
		MediaTrack* tr = (MediaTrack*)item;
		GetSetMediaTrackInfo(tr, "P_NAME", (char*)str);
	}
}

int SWS_TrackListView::GetItemCount()
{
	return m_pTrackListWnd->GetFilter()->Get()->GetFilteredTracks()->GetSize();
}

LPARAM SWS_TrackListView::GetItemPointer(int iItem)
{
	return (LPARAM)m_pTrackListWnd->GetFilter()->Get()->GetFilteredTracks()->Get(iItem);
}

bool SWS_TrackListView::GetItemState(LPARAM item)
{
	MediaTrack* tr = (MediaTrack*)item;
	return *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL) ? true : false;
}

SWS_TrackListWnd::SWS_TrackListWnd()
:SWS_DockWnd(IDD_TRACKLIST, "Track List", 30003),m_bUpdate(false),
m_trLastTouched(NULL),m_bHideFiltered(false),m_bLink(false),m_cOptionsKey("Track List Options")
{
	// Restore state
	char str[10];
	GetPrivateProfileString(SWS_INI, m_cOptionsKey, "0 0", str, 10, get_ini_file());
	if (strlen(str) == 3)
	{
		m_bHideFiltered = str[0] == '1';
		m_bLink = str[2] == '1';
	}

	if (m_bShowAfterInit)
		Show(false, false);
}

void SWS_TrackListWnd::Update()
{
	static bool bRecurseCheck = false;
	if (!IsValidWindow() || bRecurseCheck || !m_pList || m_pList->UpdatesDisabled())
		return;
	bRecurseCheck = true;

	//Update the check boxes
	CheckDlgButton(m_hwnd, IDC_HIDE, m_bHideFiltered ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_LINK, m_bLink ? BST_CHECKED : BST_UNCHECKED);
	//Update the filter
	char filter[256];
	GetDlgItemText(m_hwnd, IDC_FILTER, filter, 256);
	if (strcmp(filter, m_filter.Get()->GetFilter()))
		SetDlgItemText(m_hwnd, IDC_FILTER, m_filter.Get()->GetFilter());

	m_filter.Get()->GetFilteredTracks();
	m_filter.Get()->UpdateReaper(m_bHideFiltered);

	m_pList->Update();

	bRecurseCheck = false;
}

void SWS_TrackListWnd::ClearFilter()
{
	if (IsValidWindow())
		SendMessage(m_hwnd, WM_COMMAND, IDC_CLEAR, 0);
	else
	{
		m_filter.Get()->SetFilter("");
		m_filter.Get()->UpdateReaper(m_bHideFiltered);
	}
}

void SWS_TrackListWnd::OnInitDlg()
{
	m_resize.init_item(IDC_LIST, 0.0, 0.0, 1.0, 1.0);
	m_resize.init_item(IDC_STATIC_FILTER, 0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_FILTER, 0.0, 1.0, 1.0, 1.0);
	m_resize.init_item(IDC_CLEAR, 0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_HIDE, 0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_LINK, 0.0, 1.0, 0.0, 1.0);

	m_pList = new SWS_TrackListView(GetDlgItem(m_hwnd, IDC_LIST), GetDlgItem(m_hwnd, IDC_EDIT), this);

	Update();

	SetTimer(m_hwnd, 0, 50, NULL);
}

void SWS_TrackListWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
		case IDC_CLEAR | (BN_CLICKED << 16):
			SetWindowText(GetDlgItem(m_hwnd, IDC_FILTER), "");
			break;
		case IDC_FILTER | (EN_CHANGE << 16):
		{
			char curFilter[256];
			GetWindowText(GetDlgItem(m_hwnd, IDC_FILTER), curFilter, 256);
			m_filter.Get()->SetFilter(curFilter);
			Update();
			break;
		}
		case IDC_HIDE:
			m_bHideFiltered = IsDlgButtonChecked(m_hwnd, IDC_HIDE) == BST_CHECKED ? true : false;
			Update();
			break;
		case IDC_LINK:
			m_bLink = IsDlgButtonChecked(m_hwnd, IDC_LINK) == BST_CHECKED ? true : false;
			Update();
			break;
		case RENAME_MSG:
			if (m_trLastTouched)
				m_pList->EditListItem((LPARAM)m_trLastTouched, 1);
			break;
		case SELPREV_MSG:
		{
			int i;
			bool bShift = GetAsyncKeyState(VK_SHIFT)   & 0x8000 ? true : false;
			HWND hwndList = GetDlgItem(m_hwnd, IDC_LIST);
			for (i = 0; i < ListView_GetItemCount(hwndList); i++)
				if (ListView_GetItemState(hwndList, i, LVIS_SELECTED))
					break;
			if (i >= ListView_GetItemCount(hwndList))
				i = 1;

			if (i > 0)
			{
				if (!bShift)
					ClearSelected();
				GetSetMediaTrackInfo((MediaTrack*)m_pList->GetListItem(i-1), "I_SELECTED", &g_i1);
			}
			break;
		}
		case SELNEXT_MSG:
		{
			int i;
			bool bShift = GetAsyncKeyState(VK_SHIFT)   & 0x8000 ? true : false;
			HWND hwndList = GetDlgItem(m_hwnd, IDC_LIST);
			for (i = ListView_GetItemCount(hwndList)-1; i >= 0; i--)
				if (ListView_GetItemState(hwndList, i, LVIS_SELECTED))
					break;
			if (i < 0)
				i = ListView_GetItemCount(hwndList) - 2;

			if (i < ListView_GetItemCount(hwndList) - 1)
			{
				if (!bShift)
					ClearSelected();
				GetSetMediaTrackInfo((MediaTrack*)m_pList->GetListItem(i+1), "I_SELECTED", &g_i1);
			}
			break;
		}
		default:
			if (wParam >= LOADSNAP_MSG)
				GetSnapshot((int)(wParam - LOADSNAP_MSG), ALL_MASK, false);
			else
				Main_OnCommand((int)wParam, (int)lParam);
	}
}

HMENU SWS_TrackListWnd::OnContextMenu(int x, int y)
{
	HMENU contextMenu = CreatePopupMenu();

	AddToMenu(contextMenu, "Snapshot current track visibility", SWSGetCommandID(NewVisSnapshot));
	Snapshot* s;
	int i = 0;
	while((s = GetSnapshotPtr(i++)) != NULL)
	{
		if (s->m_iMask == VIS_MASK)
		{
			char cMenu[50];
			int iCmd = SWSGetCommandID(GetSnapshot, s->m_iSlot);
			if (!iCmd)
				iCmd = LOADSNAP_MSG + s->m_iSlot;
			_snprintf(cMenu, 50, "Recall snapshot %s", s->m_cName);
			AddToMenu(contextMenu, cMenu, iCmd);
		}
	}

	AddToMenu(contextMenu, "Show all tracks", SWSGetCommandID(ShowAll));
	AddToMenu(contextMenu, "Show SWS Snapshots", SWSGetCommandID(OpenSnapshotsDialog));

	LPARAM item = m_pList->GetHitItem(x, y, NULL);
	if (item)
	{
		m_trLastTouched = (MediaTrack*)item;
		AddToMenu(contextMenu, SWS_SEPARATOR, 0);
//#ifdef _WIN32				
//		AddSubMenu(contextMenu, GetContextMenu(0), "Track Options");
//#endif
		AddToMenu(contextMenu, "Rename", RENAME_MSG);
		AddToMenu(contextMenu, SWS_SEPARATOR, 0);
		AddToMenu(contextMenu, "Show only in MCP", SWSGetCommandID(ShowInMCPOnly));
		AddToMenu(contextMenu, "Show only in TCP", SWSGetCommandID(ShowInTCPOnly));
		AddToMenu(contextMenu, "Show in both MCP and TCP", SWSGetCommandID(ShowInMCPandTCP));
		AddToMenu(contextMenu, "Hide in both MCP and TCP", SWSGetCommandID(HideTracks));
		AddToMenu(contextMenu, SWS_SEPARATOR, 0);
		AddToMenu(contextMenu, "Invert selection", SWSGetCommandID(TogTrackSel));
		AddToMenu(contextMenu, "Hide unselected", SWSGetCommandID(HideUnSel));

		// Check current state
		switch(GetTrackVis(m_trLastTouched))
		{
			case 0: CheckMenuItem(contextMenu, SWSGetCommandID(HideTracks),      MF_BYCOMMAND | MF_CHECKED); break;
			case 1: CheckMenuItem(contextMenu, SWSGetCommandID(ShowInMCPOnly),   MF_BYCOMMAND | MF_CHECKED); break;
			case 2: CheckMenuItem(contextMenu, SWSGetCommandID(ShowInTCPOnly),   MF_BYCOMMAND | MF_CHECKED); break;
			case 3: CheckMenuItem(contextMenu, SWSGetCommandID(ShowInMCPandTCP), MF_BYCOMMAND | MF_CHECKED); break;
		}
	}

	return contextMenu;
}

void SWS_TrackListWnd::OnDestroy()
{
	m_bUpdate = false;

	KillTimer(m_hwnd, 0);
	m_pList->OnDestroy();

	char str[10];
	_snprintf(str, 10, "%d %d", m_bHideFiltered ? 1 : 0, m_bLink ? 1 : 0);
	WritePrivateProfileString(SWS_INI, m_cOptionsKey, str, get_ini_file());
}

void SWS_TrackListWnd::OnTimer()
{
	if (m_bUpdate)
	{
		Update();
		m_bUpdate = false;
	}
}

void ScheduleTracklistUpdate()
{
	if (g_pList)
		g_pList->ScheduleUpdate();
}

void OpenTrackList(COMMAND_T*)
{
	g_pList->Show(true, true);
}

void OpenTrackListFilt(COMMAND_T* = NULL)
{
	g_pList->Show(false, true);
	HWND hwnd = g_pList->GetHWND();
	SetFocus(GetDlgItem(hwnd, IDC_FILTER));
	SendMessage(GetDlgItem(hwnd, IDC_FILTER), EM_SETSEL, 0, -1);
}

void ShowInMCPOnly(COMMAND_T*)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			SetTrackVis(tr, 1);
	}
	TrackList_AdjustWindows(MAJORADJUST);
	UpdateTimeline();
	Undo_OnStateChangeEx("Show selected track(s) in MCP only", UNDO_STATE_TRACKCFG, -1);
}

void ShowInTCPOnly(COMMAND_T*)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			SetTrackVis(tr, 2);
	}
	TrackList_AdjustWindows(MAJORADJUST);
	UpdateTimeline();
	Undo_OnStateChangeEx("Show selected track(s) in TCP only", UNDO_STATE_TRACKCFG, -1);
}

void ShowInMCPandTCP(COMMAND_T*)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			SetTrackVis(tr, 3);
	}
	TrackList_AdjustWindows(MAJORADJUST);
	UpdateTimeline();
	Undo_OnStateChangeEx("Show selected track(s) in TCP and MCP", UNDO_STATE_TRACKCFG, -1);
}

void HideTracks(COMMAND_T*)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			SetTrackVis(tr, 0);
	}
	TrackList_AdjustWindows(MAJORADJUST);
	UpdateTimeline();
	Undo_OnStateChangeEx("Hide selected track(s)", UNDO_STATE_TRACKCFG, -1);
}

void ShowInMCP(COMMAND_T*)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			SetTrackVis(tr, GetTrackVis(tr) | 1);
	}
	TrackList_AdjustWindows(MAJORADJUST);
	UpdateTimeline();
	Undo_OnStateChangeEx("Show selected track(s) in MCP", UNDO_STATE_TRACKCFG, -1);
}

void ShowInTCP(COMMAND_T*)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			SetTrackVis(tr, GetTrackVis(tr) | 2);
	}
	TrackList_AdjustWindows(MAJORADJUST);
	UpdateTimeline();
	Undo_OnStateChangeEx("Show selected track(s) in TCP", UNDO_STATE_TRACKCFG, -1);
}

void HideFromMCP(COMMAND_T*)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			SetTrackVis(tr, GetTrackVis(tr) & ~1);
	}
	TrackList_AdjustWindows(MAJORADJUST);
	UpdateTimeline();
	Undo_OnStateChangeEx("Hide selected track(s) from MCP", UNDO_STATE_TRACKCFG, -1);
}

void HideFromTCP(COMMAND_T*)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			SetTrackVis(tr, GetTrackVis(tr) & ~2);
	}
	TrackList_AdjustWindows(MAJORADJUST);
	UpdateTimeline();
	Undo_OnStateChangeEx("Hide selected track(s) from TCP", UNDO_STATE_TRACKCFG, -1);
}

void TogInMCP(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			SetTrackVis(tr, GetTrackVis(tr) ^ 1);
	}
	TrackList_AdjustWindows(MAJORADJUST);
	UpdateTimeline();
	Undo_OnStateChangeEx("Toggle selected track(s) visible in MCP", UNDO_STATE_TRACKCFG, -1);
}

void TogInTCP(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			SetTrackVis(tr, GetTrackVis(tr) ^ 2);
	}
	TrackList_AdjustWindows(MAJORADJUST);
	UpdateTimeline();
	Undo_OnStateChangeEx("Toggle selected track(s) visible in TCP", UNDO_STATE_TRACKCFG, -1);
}

void ToggleHide(COMMAND_T*)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			SetTrackVis(tr, GetTrackVis(tr) ? 0 : 3);
	}
	TrackList_AdjustWindows(MAJORADJUST);
	UpdateTimeline();
	Undo_OnStateChangeEx("Toggle selected track(s) fully visible/hidden", UNDO_STATE_TRACKCFG, -1);
}

void ShowAll(COMMAND_T*)
{
	for (int i = 1; i <= GetNumTracks(); i++)
		SetTrackVis(CSurf_TrackFromID(i, false), 3);
	TrackList_AdjustWindows(MAJORADJUST);
	UpdateTimeline();
	Undo_OnStateChangeEx("Show all tracks", UNDO_STATE_TRACKCFG, -1);
}

void HideAll(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
		SetTrackVis(CSurf_TrackFromID(i, false), 0);
	TrackList_AdjustWindows(MAJORADJUST);
	UpdateTimeline();
	Undo_OnStateChangeEx("Hide all tracks", UNDO_STATE_TRACKCFG, -1);
}

void ShowInMCPEx(COMMAND_T*)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		int iVis = GetTrackVis(tr);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			SetTrackVis(tr, iVis | 1);
		else
			SetTrackVis(tr, iVis & 2);

	}
	TrackList_AdjustWindows(MAJORADJUST);
	UpdateTimeline();
	Undo_OnStateChangeEx("Show selected track(s) in MCP, hide others", UNDO_STATE_TRACKCFG, -1);
}

void ShowInTCPEx(COMMAND_T*)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		int iVis = GetTrackVis(tr);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			SetTrackVis(tr, iVis | 2);
		else
			SetTrackVis(tr, iVis & 1);

	}
	TrackList_AdjustWindows(MAJORADJUST);
	UpdateTimeline();
	Undo_OnStateChangeEx("Show selected track(s) in TCP, hide others", UNDO_STATE_TRACKCFG, -1);
}

void ShowSelOnly(COMMAND_T*)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			SetTrackVis(tr, 3);
		else
			SetTrackVis(tr, 0);

	}
	TrackList_AdjustWindows(MAJORADJUST);
	UpdateTimeline();
	Undo_OnStateChangeEx("Show selected track(s), hide others", UNDO_STATE_TRACKCFG, -1);
}

void HideUnSel(COMMAND_T*)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (!*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			SetTrackVis(tr, 0);
	}
	TrackList_AdjustWindows(MAJORADJUST);
	UpdateTimeline();
	Undo_OnStateChangeEx("Hide unselected track(s)", UNDO_STATE_TRACKCFG, -1);
}

static void ClearFilter(COMMAND_T*)
{
	g_pList->ClearFilter();
}

void NewVisSnapshot(COMMAND_T*)
{
	NewSnapshot(VIS_MASK, false);
}

static bool ProcessExtensionLine(const char *line, ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	LineParser lp(false);
	if (lp.parse(line) || lp.getnumtokens() < 1)
		return false;
	if (strcmp(lp.gettoken_str(0), "<SWSTRACKFILTER") == 0)
	{
		g_pList->GetFilter()->Get()->SetFilter(lp.gettoken_str(1));
		char linebuf[4096];
		while(true)
		{
			if (!ctx->GetLine(linebuf,sizeof(linebuf)) && !lp.parse(linebuf))
			{
				if (lp.gettoken_str(0)[0] == '>')
					break;
				g_pList->GetFilter()->Get()->Init(&lp);
			}
			else
				break;
		}
		return true;
	}
	return false;
}

static void SaveExtensionConfig(ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	char str[4096];
	bool bDone;
	while (g_pList->GetFilter()->Get()->ItemString(str, 4096, &bDone))
	{
		ctx->AddLine(str);
		if (bDone)
			break;
	}
}

static void BeginLoadProjectState(bool isUndo, struct project_config_extension_t *reg)
{
	g_pList->GetFilter()->Get()->SetFilter(NULL);
	g_pList->GetFilter()->Cleanup();
}

static project_config_extension_t g_projectconfig = { ProcessExtensionLine, SaveExtensionConfig, BeginLoadProjectState, NULL };

static COMMAND_T g_commandTable[] =
{
	{ { DEFACCEL, "SWS: Show Tracklist" },								"SWSTL_OPEN",		OpenTrackList,		"Show SWS Tracklist", },
	{ { DEFACCEL, "SWS: Show Tracklist with filter focused" },			"SWSTL_OPENFILT",	OpenTrackListFilt,	NULL, },

	// Set all bits
	{ { DEFACCEL, "SWS: Show selected track(s) in MCP only" },			"SWSTL_MCPONLY",	ShowInMCPOnly,		NULL, },
	{ { DEFACCEL, "SWS: Show selected track(s) in TCP only" },			"SWSTL_TCPONLY",	ShowInTCPOnly,		NULL, },
	{ { DEFACCEL, "SWS: Show selected track(s) in TCP and MCP" },		"SWSTL_BOTH",		ShowInMCPandTCP,	NULL, },
	{ { DEFACCEL, "SWS: Hide selected track(s)" },						"SWSTL_HIDE",		HideTracks,			NULL, },

	// Set bits individually
	{ { DEFACCEL, "SWS: Show selected track(s) in MCP" },				"SWSTL_SHOWMCP",	ShowInMCP,			NULL, },
	{ { DEFACCEL, "SWS: Show selected track(s) in TCP" },				"SWSTL_SHOWTCP",	ShowInTCP,			NULL, },
	{ { DEFACCEL, "SWS: Hide selected track(s) from MCP" },				"SWSTL_HIDEMCP",	HideFromMCP,		NULL, },
	{ { DEFACCEL, "SWS: Hide selected track(s) from TCP" },				"SWSTL_HIDETCP",	HideFromTCP,		NULL, },

	// Toggle
	{ { DEFACCEL, "SWS: Toggle selected track(s) visible in MCP" },		"SWSTL_TOGMCP",		TogInMCP,			NULL, },
	{ { DEFACCEL, "SWS: Toggle selected track(s) visible in TCP" },		"SWSTL_TOGTCP",		TogInTCP,			NULL, },
	{ { DEFACCEL, "SWS: Toggle selected track(s) fully visible/hidden" }, "SWSTL_TOGGLE",	ToggleHide,			NULL, },

	// Affect all tracks
	{ { DEFACCEL, "SWS: Show all tracks" },								"SWSTL_SHOWALL",	ShowAll,			NULL, },
	{ { DEFACCEL, "SWS: Hide all tracks" },								"SWSTL_HIDEALL",	HideAll,			NULL, },
	{ { DEFACCEL, "SWS: Show selected track(s) in MCP, hide others" },	"SWSTL_SHOWMCPEX",	ShowInMCPEx,		NULL, },
	{ { DEFACCEL, "SWS: Show selected track(s) in TCP, hide others" },	"SWSTL_SHOWTCPEX",	ShowInTCPEx,		NULL, },
	{ { DEFACCEL, "SWS: Show selected track(s), hide others" },			"SWSTL_SHOWEX",		ShowSelOnly,		NULL, },
	{ { DEFACCEL, "SWS: Hide unselected track(s)" },						"SWSTL_HIDEUNSEL",	HideUnSel,			NULL, },

	{ { DEFACCEL, "SWS: Clear tracklist filter" },                       "SWSTL_CLEARFLT",   ClearFilter,		NULL, },
	{ { DEFACCEL, "SWS: Snapshot current track visibility" },            "SWSTL_SNAPSHOT",   NewVisSnapshot,		NULL, },
	{ {}, LAST_COMMAND, }, // Denote end of table
};

// Seems damned "hacky" to me, but it works, so be it.
static int translateAccel(MSG *msg, accelerator_register_t *ctx)
{
	if (g_pList->IsActive())
	{
		if (msg->message == WM_KEYDOWN)
		{
			bool bCtrl  = GetAsyncKeyState(VK_CONTROL) & 0x8000 ? true : false;
			bool bAlt   = GetAsyncKeyState(VK_MENU)    & 0x8000 ? true : false;
			bool bShift = GetAsyncKeyState(VK_SHIFT)   & 0x8000 ? true : false;
			HWND hwnd = g_pList->GetHWND();

#ifdef _WIN32
			if (msg->wParam == VK_TAB && !bCtrl && !bAlt && !bShift)
			{
				SendMessage(hwnd, WM_NEXTDLGCTL, 0, 0);
				return 1;
			}
			else if (msg->wParam == VK_TAB && !bCtrl && !bAlt && bShift)
			{
				SendMessage(hwnd, WM_NEXTDLGCTL, 1, 0);
				return 1;
			}
			else
#endif				
			if (msg->wParam == VK_LEFT && !bCtrl && !bAlt && !bShift)
			{
				TogInTCP();
			}
			else if (msg->wParam == VK_RIGHT && !bCtrl && !bAlt && !bShift)
			{
				TogInMCP();
			}
			else if (msg->wParam == VK_DELETE && !bCtrl && !bAlt && !bShift)
			{
				Main_OnCommand(40005, 0); // remove selected tracks
				return 1;
			}
			else if (msg->wParam == VK_UP && !bCtrl && !bAlt)
			{
				SendMessage(hwnd, WM_COMMAND, SELPREV_MSG, 0);
				return 1;
			}
			else if (msg->wParam == VK_DOWN && !bCtrl && !bAlt)
			{
				SendMessage(hwnd, WM_COMMAND, SELNEXT_MSG, 0);
				return 1;
			}
		}
		return -666;
	}
	return 0;
}

static accelerator_register_t g_ar = { translateAccel, TRUE, NULL };

static void menuhook(int menuid, HMENU hMenu, int flag)
{
	if (menuid == MAINMENU_VIEW && flag == 0)
		AddToMenu(hMenu, g_commandTable[0].menuText, g_commandTable[0].accel.accel.cmd, 40075);
	else if (flag == 1)
		SWSCheckMenuItem(hMenu, g_commandTable[0].accel.accel.cmd, g_pList->IsValidWindow());
}


int TrackListInit()
{
	if (!plugin_register("accelerator",&g_ar))
		return 0;
	if (!plugin_register("projectconfig",&g_projectconfig))
		return 0;

	SWSRegisterCommands(g_commandTable);
	g_pCommandTable = g_commandTable;

	if (!plugin_register("hookmenu", (void*)menuhook))
		return 0;

	g_pList = new SWS_TrackListWnd;

	return 1;
}
