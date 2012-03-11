/******************************************************************************
/ SnM_RgnPlaylistView.cpp
/
/ Copyright (c) 2012 Jeffos
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
#include "SnM_Actions.h"
#include "SnM_RgnPlaylistView.h"
#include "../reaper/localize.h"


enum {
  BUTTONID_PLAY=2000, //JFB would be great to have _APS_NEXT_CONTROL_VALUE *always* defined
  BUTTONID_STOP
};

static SNM_RegionPlaylistWnd* g_pRgnPlaylistWnd = NULL;


///////////////////////////////////////////////////////////////////////////////
// SNM_PlaylistView
///////////////////////////////////////////////////////////////////////////////

// !WANT_LOCALIZE_STRINGS_BEGIN:sws_DLG_165
static SWS_LVColumn g_playlistCols[] = { {30,2,"#"}, {95,2,"Info"}, {222,1, "Count"}};
// !WANT_LOCALIZE_STRINGS_END

SNM_PlaylistView::SNM_PlaylistView(HWND hwndList, HWND hwndEdit)
:SWS_ListView(hwndList, hwndEdit, 3, g_playlistCols, "RgnPlaylist View State", false)
{
}

void SNM_PlaylistView::GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax)
{
	if (str) *str = '\0';
/*JFB!!!
	if (FindResultItem* pItem = (FindResultItem*)item) {
		int n = g_resultItems.Find(pItem);
		if (n>=0) {
			switch (iCol) {
				case 0:
					_snprintf(str, iStrMax, "%5.d", n+1);
					break;
				case 1:
					lstrcpyn(str, g_strTypes[pItem->m_type], iStrMax);
					break;
				case 2:
					lstrcpyn(str, pItem->m_result.Get(), iStrMax);
					break;
			}
		}
	}
*/
}

void SNM_PlaylistView::GetItemList(SWS_ListItemList* pList)
{
/*JFB!!!
	for (int i = 0; i < g_resultItems.GetSize(); i++)
		if (FindResultItem* r = g_resultItems.Get(i))
			pList->Add((SWS_ListItem*)r);
*/
}

bool SNM_PlaylistView::IsEditListItemAllowed(SWS_ListItem* item, int iCol)
{
//JFB TODO: replace
	return false;
}

void SNM_PlaylistView::SetItemText(SWS_ListItem* item, int iCol, const char* str)
{
/*JFB!!!
	if (FindResultItem* pResult = (FindResultItem*)item)
		pResult->Replace(str);
*/
}

void SNM_PlaylistView::OnItemDblClk(SWS_ListItem* item, int iCol) {
/*JFB!!!
	if (FindResultItem* pResult = (FindResultItem*)item)
		pResult->Focus();
*/
}


///////////////////////////////////////////////////////////////////////////////
// SNM_RegionPlaylistWnd
///////////////////////////////////////////////////////////////////////////////

SNM_RegionPlaylistWnd::SNM_RegionPlaylistWnd()
	: SWS_DockWnd(IDD_SNM_RGNPLAYLIST, __LOCALIZE("Region Playlist","sws_DLG_165"), "SnMRgnPlaylist", 30013, SWSGetCommandID(OpenRegionPlaylist))
{
	// Must call SWS_DockWnd::Init() to restore parameters and open the window if necessary
	Init();
}

void SNM_RegionPlaylistWnd::OnInitDlg()
{
	m_resize.init_item(IDC_LIST, 0.0, 0.0, 1.0, 1.0);
	m_pLists.Add(new SNM_PlaylistView(GetDlgItem(m_hwnd, IDC_LIST), GetDlgItem(m_hwnd, IDC_EDIT)));
	SNM_ThemeListView(m_pLists.Get(0));

	m_resize.init_item(IDC_TEXT, 0.0, 0.0, 1.0, 0.0);
	SetWindowLongPtr(GetDlgItem(m_hwnd, IDC_TEXT), GWLP_USERDATA, 0xdeadf00b);

	m_vwnd_painter.SetGSC(WDL_STYLE_GetSysColor);
    m_parentVwnd.SetRealParent(m_hwnd);
	
	m_btnPlay.SetID(BUTTONID_PLAY);
	m_parentVwnd.AddChild(&m_btnPlay);

	m_btnStop.SetID(BUTTONID_STOP);
	m_parentVwnd.AddChild(&m_btnStop);

	m_parentVwnd.RequestRedraw(NULL);
}

void SNM_RegionPlaylistWnd::OnDestroy()
{
}

INT_PTR SNM_RegionPlaylistWnd::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static int sListOldColors[LISTVIEW_COLORHOOK_STATESIZE];
	if (ListView_HookThemeColorsMessage(m_hwnd, uMsg, lParam, sListOldColors, IDC_LIST, 0, 0))
		return 1;
	return SWS_DockWnd::WndProc(uMsg, wParam, lParam);
}

void SNM_RegionPlaylistWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
/*JFB!!!
	switch(LOWORD(wParam))
	{
		case IDC_TEXT:
			if (HIWORD(wParam)==EN_CHANGE) {
				g_curResult = -1;
				if (g_notFound) {
					g_notFound = false;
					m_parentVwnd.RequestRedraw(NULL); // update not found msg
				}
			}
			break;
		case BUTTONID_ZOOM_SCROLL_EN:
			if (!HIWORD(wParam) ||  HIWORD(wParam)==600)
				m_zoomSrollItems = !m_zoomSrollItems;
			break;
		case BUTTONID_FIND:
			Find(0);
			break;
		case BUTTONID_PREV:
			Find(-1);
			break;
		case BUTTONID_NEXT:
			Find(1);
			break;
		case COMBOID_TYPE:
			if (HIWORD(wParam)==CBN_SELCHANGE) {
				m_type = m_cbType.GetCurSel();
				m_parentVwnd.RequestRedraw(NULL);
				SetFocus(GetDlgItem(m_hwnd, IDC_TEXT));
			}
			break;
		default:
			Main_OnCommand((int)wParam, (int)lParam);
			break;
	}
*/
}

int SNM_RegionPlaylistWnd::OnKey(MSG* _msg, int _iKeyState) 
{
/*JFB!!!
	if (_msg->message == WM_KEYDOWN)
	{
		switch(_msg->wParam)
		{
			case VK_F3:
				// F3: find next
				if (!_iKeyState) {
					Find(1); 
					return 1;
				}
				// Shift-F3: find previous
				else if (_iKeyState == LVKF_SHIFT) {
					Find(-1);
					return 1;
				}
				break;
			case VK_RETURN:
				if (!_iKeyState) {
					Find(0);
					return 1;
				}
				break;
		}
	}
*/
	return 0;
}

void SNM_RegionPlaylistWnd::DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight)
{
	LICE_CachedFont* font = SNM_GetThemeFont();
	IconTheme* it = SNM_GetIconTheme();

	// 1st row of controls
	int x0=_r->left+10, h=35;
	if (_tooltipHeight)
		*_tooltipHeight = h;

	// play button
	SNM_SkinButton(&m_btnPlay, it ? &(it->gen_play[0]) : NULL, "Play");
	if (SNM_AutoVWndPosition(&m_btnPlay, NULL, _r, &x0, _r->top, h, 0))
	{
		SNM_SkinButton(&m_btnStop, it ? &(it->gen_stop) : NULL, "Stop");
		if (SNM_AutoVWndPosition(&m_btnStop, NULL, _r, &x0, _r->top, h))
			SNM_AddLogo(_bm, _r, x0, h);
	}

}

HBRUSH SNM_RegionPlaylistWnd::OnColorEdit(HWND _hwnd, HDC _hdc)
{
	int bg, txt; bool match=false;
	if (_hwnd == GetDlgItem(m_hwnd, IDC_TEXT)) {
		match = true;
		SNM_GetThemeListColors(&bg, &txt);
	}
	if (match) {
		SetBkColor(_hdc, bg);
		SetTextColor(_hdc, txt);
		return SNM_GetThemeBrush(bg);
	}
	return 0;
}


///////////////////////////////////////////////////////////////////////////////

int RegionPlaylistInit()
{
	g_pRgnPlaylistWnd = new SNM_RegionPlaylistWnd();
	if (!g_pRgnPlaylistWnd)
		return 0;
	return 1;
}

void RegionPlaylistExit() {
	DELETE_NULL(g_pRgnPlaylistWnd);
}

void OpenRegionPlaylist(COMMAND_T*) {
	if (g_pRgnPlaylistWnd) {
		g_pRgnPlaylistWnd->Show(true, true);
		SetFocus(GetDlgItem(g_pRgnPlaylistWnd->GetHWND(), IDC_TEXT));
	}
}

bool IsRegionPlaylistDisplayed(COMMAND_T*){
	return (g_pRgnPlaylistWnd && g_pRgnPlaylistWnd->IsValidWindow());
}

