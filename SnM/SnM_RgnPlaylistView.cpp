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
#include "../MarkerList/MarkerListClass.h"

#define DELETE_MSG				0xF000
#define PERFORM_MSG				0xF001
#define CROP_PRJ_MSG			0xF002
#define ADD_ALL_REGIONS_MSG		0xF003
#define ADD_START_MSG			0xF100
#define ADD_END_MSG				0xF7FF // => 1791 marker/region *indexes* supported, oh well..
#define INSERT_START_MSG		0xF800
#define INSERT_END_MSG			0xFEFF // => 1791 marker/region indexes supported

#define UNDO_PLAYLIST_STR		__LOCALIZE("Region Playlist edition", "sws_undo")


enum {
  BUTTONID_PLAY=2000, //JFB would be great to have _APS_NEXT_CONTROL_VALUE *always* defined
  BUTTONID_STOP,
  BUTTONID_CROP
};

enum {
  COL_RGN=0,
  COL_RGN_COUNT,
  COL_COUNT
};

static SNM_RegionPlaylistWnd* g_pRgnPlaylistWnd = NULL;
SWSProjConfig<WDL_PtrList<SNM_RgnPlaylistItem> > g_pRegionPlaylists;
bool g_rgnPlaylistEnabled = false;
SNM_RgnPlaylistItem* g_playingItem = NULL; // see PlaylistRun()


///////////////////////////////////////////////////////////////////////////////
// SNM_PlaylistView
///////////////////////////////////////////////////////////////////////////////

// !WANT_LOCALIZE_STRINGS_BEGIN:sws_DLG_165
static SWS_LVColumn g_playlistCols[] = { {200,2,"Region"}, {70,1,"Count"} };
// !WANT_LOCALIZE_STRINGS_END

SNM_PlaylistView::SNM_PlaylistView(HWND hwndList, HWND hwndEdit)
	: SWS_ListView(hwndList, hwndEdit, COL_COUNT, g_playlistCols, "RgnPlaylistViewState", false, false)
{
}

// "compact" the playlist (e.g. 2 consecutive regions "3" are merged into one, its counter is incremented)
void SNM_PlaylistView::UpdateCompact()
{
	for (int i=g_pRegionPlaylists.Get()->GetSize()-1; i>=0 ; i--)
		if (SNM_RgnPlaylistItem* item = g_pRegionPlaylists.Get()->Get(i))
			if ((i-1)>=0 && g_pRegionPlaylists.Get()->Get(i-1) && item->m_rgnId == g_pRegionPlaylists.Get()->Get(i-1)->m_rgnId) {
				g_pRegionPlaylists.Get()->Get(i-1)->m_cnt++;
				g_pRegionPlaylists.Get()->Delete(i, true);
			}
	Update();
}

void SNM_PlaylistView::GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax)
{
	if (str) *str = '\0';
	if (SNM_RgnPlaylistItem* pItem = (SNM_RgnPlaylistItem*)item)
	{
		switch (iCol) {
			case COL_RGN: {
				char buf[128]="";
				if (pItem->m_rgnId>0 && EnumMarkerRegionDesc(GetMarkerRegionIndexFromId(pItem->m_rgnId), buf, 128, 3) >= 0 && *buf)
					if (_snprintf(str, iStrMax, "%s %s", g_playingItem==pItem ? UTF8_BULLET : " ", buf) > 0)
						return; // !!
				int num = pItem->m_rgnId & 0x3FFFFFFF;
				if (_snprintf(str, iStrMax, __LOCALIZE("Unknown region %d","sws_DLG_165"), num) > 0)
					return; // !!
				lstrcpyn(str, __LOCALIZE("Unknown region","sws_DLG_165"), iStrMax);
				break;
			}
			case COL_RGN_COUNT:
				_snprintf(str, iStrMax, "%d", pItem->m_cnt);
				break;
		}
	}
}

void SNM_PlaylistView::GetItemList(SWS_ListItemList* pList) {
	for (int i=0; i < g_pRegionPlaylists.Get()->GetSize(); i++)
		if (SWS_ListItem* item = (SWS_ListItem*)g_pRegionPlaylists.Get()->Get(i))
			pList->Add(item);
}

void SNM_PlaylistView::SetItemText(SWS_ListItem* item, int iCol, const char* str) {
	if (iCol==COL_RGN_COUNT)
		if (SNM_RgnPlaylistItem* pItem = (SNM_RgnPlaylistItem*)item) {
			pItem->m_cnt = atoi(str);
			Undo_OnStateChangeEx(UNDO_PLAYLIST_STR, UNDO_STATE_MISCCFG, -1); 
		}
}

void SNM_PlaylistView::OnItemDblClk(SWS_ListItem* item, int iCol) {
	if (g_pRgnPlaylistWnd)
		g_pRgnPlaylistWnd->OnCommand(PERFORM_MSG, 0);
}

// "disable" sort
int SNM_PlaylistView::OnItemSort(SWS_ListItem* _item1, SWS_ListItem* _item2) 
{
	int i1=-1, i2=-1;
	for (int i=0; (i1<0 && i2<0) || i<g_pRegionPlaylists.Get()->GetSize(); i++)
	{
		SWS_ListItem* item = (SWS_ListItem*)g_pRegionPlaylists.Get()->Get(i);
		if (item == _item1) i1=i;
		if (item == _item2) i2=i;
	}
	if (i1 >= 0 && i2 >= 0) {
		if (i1 > i2) return 1;
		else if (i1 < i2) return -1;
	}
	return 0;
}

void SNM_PlaylistView::OnBeginDrag(SWS_ListItem* item) {
	SetCapture(GetParent(m_hwndList));
}

void SNM_PlaylistView::OnDrag()
{
	POINT p; GetCursorPos(&p);
	if (SNM_RgnPlaylistItem* hitItem = (SNM_RgnPlaylistItem*)GetHitItem(p.x, p.y, NULL))
	{
		int iNewPriority = g_pRegionPlaylists.Get()->Find(hitItem);
		int x=0, iSelPriority;
		while(SNM_RgnPlaylistItem* selItem = (SNM_RgnPlaylistItem*)EnumSelected(&x))
		{
			iSelPriority = g_pRegionPlaylists.Get()->Find(selItem);
			if (iNewPriority == iSelPriority) return;
			m_draggedItems.Add(selItem);
		}

		bool bDir = iNewPriority > iSelPriority;
		for (int i = bDir ? 0 : m_draggedItems.GetSize()-1; bDir ? i < m_draggedItems.GetSize() : i >= 0; bDir ? i++ : i--) {
			g_pRegionPlaylists.Get()->Delete(g_pRegionPlaylists.Get()->Find(m_draggedItems.Get(i)), false);
			g_pRegionPlaylists.Get()->Insert(iNewPriority, m_draggedItems.Get(i));
		}

		ListView_DeleteAllItems(m_hwndList); // because of the special sort criteria ("not sortable" somehow)
		Update(); // no UpdateCompact() here! it would crash, see OnEndDrag()!

		for (int i=0; i < m_draggedItems.GetSize(); i++)
			SelectByItem((SWS_ListItem*)m_draggedItems.Get(i), i==0, i==0);
	}
}

void SNM_PlaylistView::OnEndDrag()
{
	UpdateCompact();
	if (m_draggedItems.GetSize()) {
		Undo_OnStateChangeEx(UNDO_PLAYLIST_STR, UNDO_STATE_MISCCFG, -1);
		m_draggedItems.Empty(false);
	}
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

	m_vwnd_painter.SetGSC(WDL_STYLE_GetSysColor);
    m_parentVwnd.SetRealParent(m_hwnd);
	
	m_btnPlay.SetID(BUTTONID_PLAY);
	m_parentVwnd.AddChild(&m_btnPlay);
	m_btnStop.SetID(BUTTONID_STOP);
	m_parentVwnd.AddChild(&m_btnStop);
	m_btnCrop.SetID(BUTTONID_CROP);
	m_parentVwnd.AddChild(&m_btnCrop);

	Update();
}

INT_PTR SNM_RegionPlaylistWnd::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static int sListOldColors[LISTVIEW_COLORHOOK_STATESIZE];
	if (ListView_HookThemeColorsMessage(m_hwnd, uMsg, lParam, sListOldColors, IDC_LIST, 0, COL_COUNT))
		return 1;
	return SWS_DockWnd::WndProc(uMsg, wParam, lParam);
}

void SNM_RegionPlaylistWnd::Update(int _flags)
{
	if ((_flags&1) && m_pLists.GetSize())
		((SNM_PlaylistView*)m_pLists.Get(0))->UpdateCompact();
	if (_flags&2)
		m_parentVwnd.RequestRedraw(NULL);
}

void SNM_RegionPlaylistWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch(LOWORD(wParam))
	{
		case BUTTONID_PLAY:
			PlaylistPlay(0, true);
			break;
		case BUTTONID_STOP:
			PlaylistStopped();
			OnStopButton();
			break;
		case BUTTONID_CROP:
		case CROP_PRJ_MSG:
			CropProjectToPlaylist();
			break;
		case DELETE_MSG: {
			bool updt = false;
			WDL_PtrList_DeleteOnDestroy<SNM_RgnPlaylistItem> delItems; // delay item deletion (still used in GUI)
			int x=0, slot;
			while(SNM_RgnPlaylistItem* item = (SNM_RgnPlaylistItem*)m_pLists.Get(0)->EnumSelected(&x)) {
				slot = g_pRegionPlaylists.Get()->Find(item);
				if (slot>=0) {
					delItems.Add(item);
					g_pRegionPlaylists.Get()->Delete(slot, false);
					updt=true;
				}
			}
			if (updt) {
				Undo_OnStateChangeEx(UNDO_PLAYLIST_STR, UNDO_STATE_MISCCFG, -1); 
				Update();
			} // + delItems cleanup
			break;
		}
		case PERFORM_MSG: {
			int x=0;
			if (SNM_RgnPlaylistItem* pItem = (SNM_RgnPlaylistItem*)m_pLists.Get(0)->EnumSelected(&x))
				PlaylistPlay(g_pRegionPlaylists.Get()->Find(pItem), true);
			break;
		}
		case ADD_ALL_REGIONS_MSG: {
				int x=0, y, num; bool isRgn, updt=false;
				while (y = EnumProjectMarkers2(NULL, x, &isRgn, NULL, NULL, NULL, &num)) {
					if (isRgn) {
						SNM_RgnPlaylistItem* newItem = new SNM_RgnPlaylistItem(MakeMarkerRegionId(num, isRgn));
						updt |= (g_pRegionPlaylists.Get()->Add(newItem) != NULL);
					}
					x=y;
				}
				if (updt) {
					Update();
					Undo_OnStateChangeEx(UNDO_PLAYLIST_STR, UNDO_STATE_MISCCFG, -1);
				}
				else
					MessageBox(m_hwnd, __LOCALIZE("No region found in project!","sws_DLG_165"), __LOCALIZE("S&M - Error","sws_DLG_165"), MB_OK);
			break;
		}
		default:
			if (LOWORD(wParam) >= INSERT_START_MSG && LOWORD(wParam) <= INSERT_END_MSG)
			{
				SNM_RgnPlaylistItem* newItem = new SNM_RgnPlaylistItem(GetMarkerRegionIdFromIndex(LOWORD(wParam)-INSERT_START_MSG));
				if (g_pRegionPlaylists.Get()->GetSize()) {
					if (SNM_RgnPlaylistItem* item = (SNM_RgnPlaylistItem*)m_pLists.Get(0)->EnumSelected(NULL)) {
						int slot = g_pRegionPlaylists.Get()->Find(item);
						if (slot >= 0 && g_pRegionPlaylists.Get()->Insert(slot, newItem)) {
							Update();
							m_pLists.Get(0)->SelectByItem((SWS_ListItem*)newItem);
							Undo_OnStateChangeEx(UNDO_PLAYLIST_STR, UNDO_STATE_MISCCFG, -1); 
							return; // <-- !!
						}
					}
				}
				// empty list, no selection, etc.. => add
				if (g_pRegionPlaylists.Get()->Add(newItem)) {
					Update();
					m_pLists.Get(0)->SelectByItem((SWS_ListItem*)newItem);
					Undo_OnStateChangeEx(UNDO_PLAYLIST_STR, UNDO_STATE_MISCCFG, -1); 
				}
			}
			else if (LOWORD(wParam) >= ADD_START_MSG && LOWORD(wParam) <= ADD_END_MSG)
			{
				SNM_RgnPlaylistItem* newItem = new SNM_RgnPlaylistItem(GetMarkerRegionIdFromIndex(LOWORD(wParam)-ADD_START_MSG));
				if (g_pRegionPlaylists.Get()->Add(newItem)) {
					Update();
					m_pLists.Get(0)->SelectByItem((SWS_ListItem*)newItem);
					Undo_OnStateChangeEx(UNDO_PLAYLIST_STR, UNDO_STATE_MISCCFG, -1); 
				}
			}
			else
				Main_OnCommand((int)wParam, (int)lParam);
			break;
	}
}

int SNM_RegionPlaylistWnd::OnKey(MSG* _msg, int _iKeyState) 
{
	if (_msg->message == WM_KEYDOWN && !_iKeyState)
	{
		switch(_msg->wParam)
		{
			case VK_DELETE:
				OnCommand(DELETE_MSG, 0);
				return 1;
			case VK_RETURN:
				OnCommand(PERFORM_MSG, 0);
				return 1;
		}
	}
	return 0;
}

void SNM_RegionPlaylistWnd::DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight)
{
	LICE_CachedFont* font = SNM_GetThemeFont();
	IconTheme* it = SNM_GetIconTheme();
	int x0=_r->left+10, h=35;
	if (_tooltipHeight)
		*_tooltipHeight = h;

	SNM_SkinButton(&m_btnPlay, it ? &it->gen_play[g_rgnPlaylistEnabled?1:0] : NULL, __LOCALIZE("Play","sws_DLG_165"));
	if (SNM_AutoVWndPosition(&m_btnPlay, NULL, _r, &x0, _r->top, h, 0)) {
		SNM_SkinButton(&m_btnStop, it ? &(it->gen_stop) : NULL, __LOCALIZE("Stop","sws_DLG_165"));
		if (SNM_AutoVWndPosition(&m_btnStop, NULL, _r, &x0, _r->top, h, 5)) {
			SNM_SkinToolbarButton(&m_btnCrop, __LOCALIZE("Crop project","sws_DLG_165"));
			if (SNM_AutoVWndPosition(&m_btnCrop, NULL, _r, &x0, _r->top, h))
				SNM_AddLogo(_bm, _r, x0, h);
		}
	}
}

HMENU SNM_RegionPlaylistWnd::OnContextMenu(int _x, int _y, bool* _wantDefaultItems)
{
	HMENU hMenu = CreatePopupMenu();
	int x=0; bool hasSel = (m_pLists.Get(0)->EnumSelected(&x) != NULL);
	*_wantDefaultItems = !(m_pLists.Get(0)->GetHitItem(_x, _y, &x) && x >= 0);

	AddToMenu(hMenu, __LOCALIZE("Add all regions","sws_DLG_165"), ADD_ALL_REGIONS_MSG);
	HMENU hAddSubMenu = CreatePopupMenu();
	AddSubMenu(hMenu, hAddSubMenu, __LOCALIZE("Add region","sws_DLG_165"));
	FillMarkerRegionMenu(hAddSubMenu, ADD_START_MSG, 2);

	if (*_wantDefaultItems) {
		AddToMenu(hMenu, SWS_SEPARATOR, 0);
		AddToMenu(hMenu, __LOCALIZE("Crop project to playlist","sws_DLG_165"), CROP_PRJ_MSG, -1, false, g_pRegionPlaylists.Get()->GetSize() ? 0 : MF_GRAYED);
	}
	else {
		HMENU hInsertSubMenu = CreatePopupMenu();
		AddSubMenu(hMenu, hInsertSubMenu, __LOCALIZE("Insert region","sws_DLG_165"), -1, hasSel ? 0 : MF_GRAYED);
		FillMarkerRegionMenu(hInsertSubMenu, INSERT_START_MSG, 2);
		AddToMenu(hMenu, __LOCALIZE("Delete region(s)","sws_DLG_165"), DELETE_MSG, -1, false, hasSel ? 0 : MF_GRAYED);
	}
	return hMenu;
}

HBRUSH SNM_RegionPlaylistWnd::OnColorEdit(HWND _hwnd, HDC _hdc)
{
	if (_hwnd == GetDlgItem(m_hwnd, IDC_EDIT))
	{
		int bg, txt;
		SNM_GetThemeListColors(&bg, &txt); // and not SNM_GetThemeEditColors (lists' IDC_EDIT!)
		SetBkColor(_hdc, bg);
		SetTextColor(_hdc, txt);
		return SNM_GetThemeBrush(bg);
	}
	return 0;
}


///////////////////////////////////////////////////////////////////////////////

double GetPlayListLength(double* _length)
{
	double rgnpos, rgnend, length = 0.0;
	for (int i=0; i<g_pRegionPlaylists.Get()->GetSize(); i++)
		if (SNM_RgnPlaylistItem* item = g_pRegionPlaylists.Get()->Get(i))
			if (item->m_rgnId>0 && EnumProjectMarkers2(NULL, GetMarkerRegionIndexFromId(item->m_rgnId), NULL, &rgnpos, &rgnend, NULL, NULL))
				length += ((rgnend-rgnpos)*item->m_cnt);
	return length;
}

// do not use things like ..->Get(i+1) but this func!
int GetNextValidPlaylistItem(int _playlistIdx, bool _startWith=false)
{
	if (int sz = g_pRegionPlaylists.Get()->GetSize())
	{
		for (int i=_playlistIdx+(_startWith?0:1); i<sz; i++)
			if (SNM_RgnPlaylistItem* item = g_pRegionPlaylists.Get()->Get(i))
				if (item->m_rgnId>0 && item->m_cnt>0)
					return i;
		// no issue if _playlistIdx==0: it will not match either..
		for (int i=0; i<=(_playlistIdx-(_startWith?1:0)); i++)
			if (SNM_RgnPlaylistItem* item = g_pRegionPlaylists.Get()->Get(i))
				if (item->m_rgnId>0 && item->m_cnt>0)
					return i;
	}
	return -1;
}

int GetPlaylistItems(double _pos, int* _playId, SNM_RgnPlaylistCurNextItems* _items)
{
	if (int sz = g_pRegionPlaylists.Get()->GetSize())
	{
		int id, idx=FindMarkerRegion(_pos, &id);
		if (id>0)
		{
			for (int i=*_playId; i<sz; i++)
				if (SNM_RgnPlaylistItem* item = g_pRegionPlaylists.Get()->Get(i))
					// assumption: the case "2 consecutive and identical region ids" is impossible, see SNM_PlaylistView::UpdateCompact()
					if (item->m_rgnId==id && item->m_cnt>0)
					{
						_items->m_cur = item;
						*_playId = i;
						if (_items->m_cur->m_cnt>1 && (_items->m_cur->m_playRequested+1) <= _items->m_cur->m_cnt)
							_items->m_next = _items->m_cur;
						else {
							int next = GetNextValidPlaylistItem(i);
							if (next>=0) // just in case ("next" is "i" in the worst case)
								_items->m_next = g_pRegionPlaylists.Get()->Get(next);
						}
						return _items->m_next ? idx : -1;
					}

			// if we are here, it is a loop!
			int first = GetNextValidPlaylistItem(0, true);
			if (first>=0)
			{
				_items->m_cur = g_pRegionPlaylists.Get()->Get(first);
				*_playId = first;
				if (_items->m_cur->m_cnt>1 && (_items->m_cur->m_playRequested+1) <= _items->m_cur->m_cnt)
					_items->m_next = _items->m_cur;
				else {
					int next = GetNextValidPlaylistItem(first);
					if (next>=0) // just in case ("next" is "first" in the worst case)
						_items->m_next = g_pRegionPlaylists.Get()->Get(next);
				}
				return _items->m_next ? GetMarkerRegionIndexFromId(_items->m_cur->m_rgnId) : -1;
			}
		}
	}
	return -1;
}

//JFB!!! really need of all them?
double g_triggerPos = -1.0;
double g_lastPlayPos = -1.0;
bool g_isRegionLooping = false;
int g_playId = 0;

// made as idle as possible..
void PlaylistRun()
{
	if (!g_rgnPlaylistEnabled)
		return;

	bool updateUI = false;
	double pos = GetPlayPosition();

	if (g_triggerPos >= 0.0)
	{
		 if (pos >= g_triggerPos)
		 {
			SNM_RgnPlaylistCurNextItems items;
			int idx = GetPlaylistItems(pos, &g_playId, &items);
			if (idx>=0)
			{
				items.m_next->m_playRequested++;
				g_isRegionLooping=false;
				g_triggerPos = -1.0;

				idx = GetMarkerRegionIndexFromId(items.m_next->m_rgnId);
				if (idx >= 0)
				{
					double rgnpos, cursorpos = GetCursorPositionEx(NULL);
					EnumProjectMarkers2(NULL, idx, NULL, &rgnpos, NULL, NULL, NULL);
//					rgnpos = SnapToGrid ? rgnpos : SnapToGrid(NULL, rgnpos);
					SetEditCurPos(rgnpos, false, true); // seek play
					SetEditCurPos(cursorpos, false, false); // restore cursor pos
				}
			}
		}
	}
	else
	{
		SNM_RgnPlaylistCurNextItems items;
		int idx = GetPlaylistItems(pos, &g_playId, &items);
		if (idx>=0)
		{
			// detect playlist item switching
			if (g_playingItem != items.m_cur)
			{
				g_playingItem = items.m_cur;
				g_lastPlayPos = -1.0;
				g_isRegionLooping = false;
				updateUI = true;

				for (int i=0; i<g_pRegionPlaylists.Get()->GetSize(); i++)
					if (SNM_RgnPlaylistItem* item = g_pRegionPlaylists.Get()->Get(i))
							item->m_playRequested = 0;
			}
			// detect region looping
			else if (!g_isRegionLooping && pos < g_lastPlayPos)
				g_isRegionLooping = true;

			// compute next trigger position, if needed
			if (!items.m_next->m_playRequested || g_isRegionLooping)
			{
				double rgnend;
				int measEnd;
				EnumProjectMarkers2(NULL, idx, NULL, NULL, &rgnend, NULL, NULL);
//				rgnend = SnapToGrid ? rgnend : SnapToGrid(NULL, rgnend);
				TimeMap2_timeToBeats(NULL, rgnend, &measEnd, NULL, NULL, NULL);
				measEnd--;
				g_triggerPos = TimeMap2_beatsToTime(NULL, 0.0, &measEnd);
			}
		}
	}

	g_lastPlayPos = pos;

	//JFB!!! TODO optimiz
	if (updateUI && g_pRgnPlaylistWnd)
		g_pRgnPlaylistWnd->Update();
}

int g_oldSeekOpt = -1;

void PlaylistPlay(int _playlistId, bool _errMsg)
{
	g_rgnPlaylistEnabled = false;
	int first = GetNextValidPlaylistItem(_playlistId, true);
	if (first>=0)
	{
		if (SNM_RgnPlaylistItem* item = g_pRegionPlaylists.Get()->Get(first))
		{
			int idx = GetMarkerRegionIndexFromId(item->m_rgnId);
			if (idx>=0)
			{
				double startpos;
				if (EnumProjectMarkers2(NULL, idx, NULL, &startpos, NULL, NULL, NULL))
				{
					// temp override of the "smooth seek" option
					if (int* opt = (int*)GetConfigVar("smoothseek")) {
						g_oldSeekOpt = *opt;
						*opt = 1;
					}
					g_playingItem = item;
					g_triggerPos = -1.0;
					g_lastPlayPos = startpos;
					g_isRegionLooping = false;
					g_playId = first;

					double cursorpos = GetCursorPositionEx(NULL);
					SetEditCurPos(startpos, false, false);
					OnPlayButton();
					SetEditCurPos(cursorpos, false, false);

					// go!
					g_rgnPlaylistEnabled=true;

					if (g_pRgnPlaylistWnd)
						g_pRgnPlaylistWnd->Update();
				}
			}
		}
	}
	if (_errMsg && !g_rgnPlaylistEnabled)
		MessageBox(GetMainHwnd(), __LOCALIZE("Nothing to play!\n(empty playlist, removed regions, etc..)","sws_DLG_165"),  __LOCALIZE("S&M -Error","sws_DLG_165"), MB_OK);
}

void PlaylistPlay(COMMAND_T* _ct) {
	PlaylistPlay((int)_ct->user, false);
}

void PlaylistStopped()
{
	g_rgnPlaylistEnabled = false;
	g_playingItem = NULL;
	g_playId = 0;
	g_triggerPos = -1.0;
	g_lastPlayPos = -1.0;
	g_isRegionLooping = false;
	g_playId = 0;

	// restore the "smooth seek" option
	if (int* opt = (int*)GetConfigVar("smoothseek"))
		if (g_oldSeekOpt >= 0) {
			*opt = g_oldSeekOpt;
			g_oldSeekOpt = -1;
		}

	if (g_pRgnPlaylistWnd)
		g_pRgnPlaylistWnd->Update();
}


///////////////////////////////////////////////////////////////////////////////

void CropProjectToPlaylist()
{
	bool updated = false;
	double startPos, endPos;
	WDL_PtrList_DeleteOnDestroy<MarkerItem> rgns;
	for (int i=0; i<g_pRegionPlaylists.Get()->GetSize(); i++)
	{
		if (SNM_RgnPlaylistItem* item = g_pRegionPlaylists.Get()->Get(i))
		{
			int rgnIdx = GetMarkerRegionIndexFromId(item->m_rgnId);
			if (rgnIdx>=0)
			{
				int rgnnum, rgncol=0; double rgnpos, rgnend; char* rgnname;
				if (EnumProjectMarkers3(NULL, rgnIdx, NULL, &rgnpos, &rgnend, &rgnname, &rgnnum, &rgncol))
				{
					if (ItemsInInterval(rgnpos, rgnend))
					{
						if (!updated) { // do it once
							updated = true;
							Undo_BeginBlock2(NULL);
							Main_OnCommand(40043, 0); // go to end of project
							startPos = endPos = GetCursorPositionEx(NULL);
						}
						
						SplitSelectAllItemsInRegion(NULL, rgnIdx);

						// make sure some envelope options are enabled:
						// move with items + add edge points
						int oldOpt[2] = {-1,-1};
						int* options[2] = {NULL,NULL};
						options[0] = (int*)GetConfigVar("envattach");
						if (options[0]) {
							oldOpt[0] = *options[0];
							*options[0] = 1;
						}
						options[1] = (int*)GetConfigVar("env_reduce");
						if (options[1]) {
							oldOpt[1] = *options[1];
							*options[1] = 2;
						}

						// REAPER bug: the last param of ApplyNudge() is ignored although
						// it is used in duplicate mode.. => use a loop instead
						bool first = true;
						for (int j=0; j<item->m_cnt; j++)
						{
							// store marker
							bool found = false;
							for (int k=0; !found && k<rgns.GetSize(); k++)
								if (rgns.Get(k)->GetID() == rgnnum) // poorely named.. this is not an "ID"
									found = true;
							if (!found)
								rgns.Add(new MarkerItem(true, endPos-startPos, endPos+rgnend-rgnpos-startPos, rgnname, rgnnum, rgncol));

							// append at end of project
							ApplyNudge(NULL, 1, 5, 1, endPos, false, 1); 
							endPos += (rgnend-rgnpos);
						}

						// restore options
						for (int j=0; j < 2; j++)
							if (options[j])
								*options[j] = oldOpt[j];
					}
				}
			}
		}
	}

	if (updated)
	{
		// crop project + remove markers/regions
		GetSet_LoopTimeRange(true, false, &startPos, &endPos, false);
		Main_OnCommand(40049, 0);// crop project to time sel
		int x=0, num; bool isRgn;
		while ((x = EnumProjectMarkers(x, &isRgn, NULL, NULL, NULL, &num)))
			DeleteProjectMarker(NULL, num, isRgn);

		// restore markers
		for (int i=0; i<rgns.GetSize(); i++)
			rgns.Get(i)->AddToProject();

		// clear time sel + edit cursor position
		GetSet_LoopTimeRange(true, false, &g_d0, &g_d0, false);
		SetEditCurPos(0.0, true, false);		
		Main_OnCommand(40289, 0); // unselect all items

		Undo_EndBlock2(NULL, __LOCALIZE("Crop project to playlist","sws_undo"), UNDO_STATE_ALL);

		// regions have most probably moved..
		if (g_pRgnPlaylistWnd)
			g_pRgnPlaylistWnd->Update(1);
		}
}

///////////////////////////////////////////////////////////////////////////////

static bool ProcessExtensionLine(const char *line, ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	LineParser lp(false);
	if (lp.parse(line) || lp.getnumtokens() < 1)
		return false;

	if (!strcmp(lp.gettoken_str(0), "<S&M_RGN_PLAYLIST"))
	{
		char linebuf[SNM_MAX_CHUNK_LINE_LENGTH]="";
		while(true)
		{
			if (!ctx->GetLine(linebuf,sizeof(linebuf)) && !lp.parse(linebuf))
			{
				if (lp.getnumtokens() && lp.gettoken_str(0)[0] == '>')
					break;
				else if (lp.getnumtokens() == 2)
					g_pRegionPlaylists.Get()->Add(new SNM_RgnPlaylistItem(lp.gettoken_int(0), lp.gettoken_int(1)));
			}
			else
				break;
		}
		if (g_pRgnPlaylistWnd)
			g_pRgnPlaylistWnd->Update();
		return true;
	}
	return false;
}

static void SaveExtensionConfig(ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	// cleanup non-existant regions
	for (int i=0; i < g_pRegionPlaylists.Get()->GetSize(); i++)
		if (SNM_RgnPlaylistItem* item = g_pRegionPlaylists.Get()->Get(i))
			if (GetMarkerRegionIndexFromId(item->m_rgnId) < 0)
				item->m_cnt = 0;

	WDL_FastString confStr("<S&M_RGN_PLAYLIST ");
	confStr.Append("\"Untitled\"");
	confStr.Append("\n");

	int iHeaderLen = confStr.GetLength();
	for (int i=0; i < g_pRegionPlaylists.Get()->GetSize(); i++)
		if (SNM_RgnPlaylistItem* item = g_pRegionPlaylists.Get()->Get(i))
			confStr.AppendFormatted(128,"%d %d\n", item->m_rgnId, item->m_cnt);
	if (confStr.GetLength() > iHeaderLen) {
		confStr.Append(">\n");
		StringToExtensionConfig(&confStr, ctx);
	}
}

static void BeginLoadProjectState(bool isUndo, struct project_config_extension_t *reg) {
	g_pRegionPlaylists.Cleanup();
	g_pRegionPlaylists.Get()->Empty(true);
}

static project_config_extension_t g_projectconfig = {
	ProcessExtensionLine, SaveExtensionConfig, BeginLoadProjectState, NULL
};


///////////////////////////////////////////////////////////////////////////////

int RegionPlaylistInit() {
	g_pRgnPlaylistWnd = new SNM_RegionPlaylistWnd();
	if (!g_pRgnPlaylistWnd || !plugin_register("projectconfig", &g_projectconfig))
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
