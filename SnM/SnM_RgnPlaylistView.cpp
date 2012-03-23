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
#include "../Prompt.h"
#include "../reaper/localize.h"
#include "../MarkerList/MarkerListClass.h"

#define MUTEX_PLAYLISTS			SWS_SectionLock lock(&g_playlistsMutex) // allows quick un-mutexing

#define DELETE_MSG				0xF000
#define PERFORM_MSG				0xF001
#define CROP_PRJ_MSG			0xF002
#define NEW_PLAYLIST_MSG		0xF003
#define COPY_PLAYLIST_MSG		0xF004
#define DEL_PLAYLIST_MSG		0xF005
#define REN_PLAYLIST_MSG		0xF006
#define ADD_ALL_REGIONS_MSG		0xF007
#define ADD_REGION_START_MSG	0xF100
#define ADD_REGION_END_MSG		0xF7FF // => 1791 marker/region *indexes* supported, oh well..
#define INSERT_REGION_START_MSG	0xF800
#define INSERT_REGION_END_MSG	0xFEFF // => 1791 marker/region indexes supported

#define UNDO_PLAYLIST_STR		__LOCALIZE("Region Playlist edition", "sws_undo")


enum {
  BUTTONID_PLAY=2000, //JFB would be great to have _APS_NEXT_CONTROL_VALUE *always* defined
  BUTTONID_STOP,
  TXTID_PLAYLIST,
  COMBOID_PLAYLIST,
  CONTAINER_ADD_DEL,
  BUTTONID_NEW_PLAYLIST,
  BUTTONID_DEL_PLAYLIST,
  TXTID_LENGTH,
  BUTTONID_CROP
};

enum {
  COL_RGN=0,
  COL_RGN_COUNT,
  COL_COUNT
};

int g_curPlaylist = 0; // must always be valid, incl. the default value (it is not persisted)
static SNM_RegionPlaylistWnd* g_pRgnPlaylistWnd = NULL;
SWSProjConfig<WDL_PtrList<SNM_Playlist> > g_pPlaylists;
SWS_Mutex g_playlistsMutex;

bool g_rgnPlaylistEnabled = false;
SNM_PlaylistItem* g_lastPlayedItem = NULL; // see PlaylistRun()

// helper funcs
SNM_Playlist* GetPlaylist(int _id = -1)
{
	MUTEX_PLAYLISTS;
	if (_id < 0) _id = g_curPlaylist;
	return g_pPlaylists.Get()->Get(_id);
}


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
	MUTEX_PLAYLISTS;
	if (GetPlaylist())
		for (int i=GetPlaylist()->GetSize()-1; i>=0 ; i--)
			if (SNM_PlaylistItem* item = GetPlaylist()->Get(i))
				if ((i-1)>=0 && GetPlaylist()->Get(i-1) && item->m_rgnId == GetPlaylist()->Get(i-1)->m_rgnId) {
					GetPlaylist()->Get(i-1)->m_cnt++;
					GetPlaylist()->Delete(i, true);
				}
	Update();
}

void SNM_PlaylistView::GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax)
{
	MUTEX_PLAYLISTS;
	if (str) *str = '\0';
	if (SNM_PlaylistItem* pItem = (SNM_PlaylistItem*)item)
	{
		switch (iCol) {
			case COL_RGN: {
				char buf[128]="";
				if (pItem->m_rgnId>0 && EnumMarkerRegionDesc(GetMarkerRegionIndexFromId(pItem->m_rgnId), buf, 128, 2, true) >= 0 && *buf)
					if (_snprintf(str, iStrMax, "%s %s", g_lastPlayedItem==pItem ? UTF8_BULLET : " ", buf) > 0)
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

void SNM_PlaylistView::GetItemList(SWS_ListItemList* pList)
{
	MUTEX_PLAYLISTS;
	if (GetPlaylist())
		for (int i=0; i < GetPlaylist()->GetSize(); i++)
			if (SWS_ListItem* item = (SWS_ListItem*)GetPlaylist()->Get(i))
				pList->Add(item);
}

void SNM_PlaylistView::SetItemText(SWS_ListItem* item, int iCol, const char* str)
{
	MUTEX_PLAYLISTS;
	if (iCol==COL_RGN_COUNT)
		if (SNM_PlaylistItem* pItem = (SNM_PlaylistItem*)item)
		{
			pItem->m_cnt = atoi(str);
			if (g_pRgnPlaylistWnd)
				g_pRgnPlaylistWnd->Update(2); // playlist length probably changed
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
	MUTEX_PLAYLISTS;
	int i1=-1, i2=-1;
	for (int i=0; (i1<0 && i2<0) || (GetPlaylist() && i < GetPlaylist()->GetSize()); i++)
	{
		SWS_ListItem* item = (SWS_ListItem*)GetPlaylist()->Get(i);
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
	MUTEX_PLAYLISTS;
	POINT p; GetCursorPos(&p);
	if (SNM_PlaylistItem* hitItem = (SNM_PlaylistItem*)GetHitItem(p.x, p.y, NULL))
	{
		int iNewPriority = GetPlaylist()->Find(hitItem);
		int x=0, iSelPriority;
		while(SNM_PlaylistItem* selItem = (SNM_PlaylistItem*)EnumSelected(&x))
		{
			iSelPriority = GetPlaylist()->Find(selItem);
			if (iNewPriority == iSelPriority) return;
			m_draggedItems.Add(selItem);
		}

		bool bDir = iNewPriority > iSelPriority;
		for (int i = bDir ? 0 : m_draggedItems.GetSize()-1; bDir ? i < m_draggedItems.GetSize() : i >= 0; bDir ? i++ : i--) {
			GetPlaylist()->Delete(GetPlaylist()->Find(m_draggedItems.Get(i)), false);
			GetPlaylist()->Insert(iNewPriority, m_draggedItems.Get(i));
		}

		ListView_DeleteAllItems(m_hwndList); // because of the special sort criteria ("not sortable" somehow)
		Update(); // no UpdateCompact() here! it would crash, see OnEndDrag()!

		for (int i=0; i < m_draggedItems.GetSize(); i++)
			SelectByItem((SWS_ListItem*)m_draggedItems.Get(i), i==0, i==0);
	}
}

void SNM_PlaylistView::OnEndDrag()
{
	MUTEX_PLAYLISTS;
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
	// must call SWS_DockWnd::Init() to restore parameters and open the window if necessary
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

	m_txtPlaylist.SetID(TXTID_PLAYLIST);
	m_parentVwnd.AddChild(&m_txtPlaylist);

	m_cbPlaylist.SetID(COMBOID_PLAYLIST);
	FillPlaylistCombo();
	m_parentVwnd.AddChild(&m_cbPlaylist);

	m_btnsAddDel.SetIDs(CONTAINER_ADD_DEL, BUTTONID_NEW_PLAYLIST, BUTTONID_DEL_PLAYLIST);
	m_parentVwnd.AddChild(&m_btnsAddDel);

	m_txtLength.SetID(TXTID_LENGTH);
	m_parentVwnd.AddChild(&m_txtLength);

	m_btnCrop.SetID(BUTTONID_CROP);
	m_parentVwnd.AddChild(&m_btnCrop);

	Update();

	RegisterToMarkerRegionUpdates(&m_mkrRgnSubscriber);
}

void SNM_RegionPlaylistWnd::OnDestroy() {
	UnregisterToMarkerRegionUpdates(&m_mkrRgnSubscriber);
	m_cbPlaylist.Empty();
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
	MUTEX_PLAYLISTS;

	static bool bRecurseCheck = false;
	if (bRecurseCheck)
		return;

	bRecurseCheck = true;

	if ((_flags&1) && m_pLists.GetSize()) {
		((SNM_PlaylistView*)m_pLists.Get(0))->UpdateCompact();
		ShowWindow(GetDlgItem(m_hwnd, IDC_LIST), g_pPlaylists.Get()->GetSize() ? SW_SHOW : SW_HIDE);
	}
	if (_flags&2)
		m_parentVwnd.RequestRedraw(NULL);

	// "fast" update without playlist compacting (used while playing)
	if ((_flags&4) && m_pLists.GetSize())
		((SNM_PlaylistView*)m_pLists.Get(0))->Update();

	bRecurseCheck = false;
}

void SNM_RegionPlaylistWnd::FillPlaylistCombo()
{
	MUTEX_PLAYLISTS;
	m_cbPlaylist.Empty();
	for (int i=0; i < g_pPlaylists.Get()->GetSize(); i++)
		m_cbPlaylist.AddItem(GetPlaylist(i)->m_name.Get());
	m_cbPlaylist.SetCurSel(g_curPlaylist); // g_curPlaylist is always valid..
}

void SNM_RegionPlaylistWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	MUTEX_PLAYLISTS;
	switch(LOWORD(wParam))
	{
		case COMBOID_PLAYLIST:
			if (HIWORD(wParam)==CBN_SELCHANGE)
			{
				// stop cell editing (changing the list content would be ignored otherwise,
				// leading to unsynchronized dropdown box vs list view)
				m_pLists.Get(0)->EditListItemEnd(false);
				g_curPlaylist = m_cbPlaylist.GetCurSel();
				Update();
			}
			break;
		case NEW_PLAYLIST_MSG:
		case BUTTONID_NEW_PLAYLIST:
		case COPY_PLAYLIST_MSG:
		{
			char name[64];
			lstrcpyn(name, __LOCALIZE("Untitled","sws_DLG_165"), 64);
			if (PromptUserForString(m_hwnd, __LOCALIZE("S&M - Add playlist","sws_DLG_165"), name, 64) && *name)
			{
				if (g_pPlaylists.Get()->Add(new SNM_Playlist(name, LOWORD(wParam)==COPY_PLAYLIST_MSG ? GetPlaylist() : NULL)))
				{
					g_curPlaylist = g_pPlaylists.Get()->GetSize()-1;
					FillPlaylistCombo();
					Update();
					Undo_OnStateChangeEx(UNDO_PLAYLIST_STR, UNDO_STATE_MISCCFG, -1); 
				}
			}
			break;
		}
		case DEL_PLAYLIST_MSG:
		case BUTTONID_DEL_PLAYLIST:
			if (g_pPlaylists.Get()->GetSize() > 0)
			{
				int reply = MessageBox(m_hwnd, __LOCALIZE("Are you sure you want to delete this playlist ?","sws_DLG_165"), __LOCALIZE("S&M - Delete playlist","sws_DLG_165"), MB_OKCANCEL);
				if (reply != IDCANCEL)
				{
					g_pPlaylists.Get()->Delete(g_curPlaylist, true);//JFB!!! items still displayed..
					g_curPlaylist = BOUNDED(g_curPlaylist-1, 0, g_pPlaylists.Get()->GetSize()-1);
					FillPlaylistCombo();
					Update();
					Undo_OnStateChangeEx(UNDO_PLAYLIST_STR, UNDO_STATE_MISCCFG, -1); 
				}
			}
			break;
		case REN_PLAYLIST_MSG:
			if (g_pPlaylists.Get()->GetSize() > 0)
			{
				char newName[64] = "";
				lstrcpyn(newName, GetPlaylist()->m_name.Get(), 64);
				if (PromptUserForString(m_hwnd, __LOCALIZE("S&M - Rename playlist","sws_DLG_165"), newName, 64) && *newName)
				{
					GetPlaylist()->m_name.Set(newName);
					FillPlaylistCombo();
					Update();
					Undo_OnStateChangeEx(UNDO_PLAYLIST_STR, UNDO_STATE_MISCCFG, -1); 
				}
			}
			break;
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
			WDL_PtrList_DeleteOnDestroy<SNM_PlaylistItem> delItems; // delay item deletion (still used in GUI)
			int x=0, slot;
			while(SNM_PlaylistItem* item = (SNM_PlaylistItem*)m_pLists.Get(0)->EnumSelected(&x))
			{
				slot = GetPlaylist()->Find(item);
				if (slot>=0)
				{
					delItems.Add(item);
					GetPlaylist()->Delete(slot, false);
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
			if (SNM_PlaylistItem* pItem = (SNM_PlaylistItem*)m_pLists.Get(0)->EnumSelected(&x))
				PlaylistPlay(GetPlaylist()->Find(pItem), true);
			break;
		}
		case ADD_ALL_REGIONS_MSG: {
				int x=0, y, num; bool isRgn, updt=false;
				while (y = EnumProjectMarkers2(NULL, x, &isRgn, NULL, NULL, NULL, &num))
				{
					if (isRgn) {
						SNM_PlaylistItem* newItem = new SNM_PlaylistItem(MakeMarkerRegionId(num, isRgn));
						updt |= (GetPlaylist()->Add(newItem) != NULL);
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
			if (LOWORD(wParam) >= INSERT_REGION_START_MSG && LOWORD(wParam) <= INSERT_REGION_END_MSG)
			{
				SNM_PlaylistItem* newItem = new SNM_PlaylistItem(GetMarkerRegionIdFromIndex(LOWORD(wParam)-INSERT_REGION_START_MSG));
				if (GetPlaylist()->GetSize())
				{
					if (SNM_PlaylistItem* item = (SNM_PlaylistItem*)m_pLists.Get(0)->EnumSelected(NULL))
					{
						int slot = GetPlaylist()->Find(item);
						if (slot >= 0 && GetPlaylist()->Insert(slot, newItem))
						{
							Update();
							m_pLists.Get(0)->SelectByItem((SWS_ListItem*)newItem);
							Undo_OnStateChangeEx(UNDO_PLAYLIST_STR, UNDO_STATE_MISCCFG, -1); 
							return; // <-- !!
						}
					}
				}
				// empty list, no selection, etc.. => add
				if (GetPlaylist()->Add(newItem))
				{
					Update();
					m_pLists.Get(0)->SelectByItem((SWS_ListItem*)newItem);
					Undo_OnStateChangeEx(UNDO_PLAYLIST_STR, UNDO_STATE_MISCCFG, -1); 
				}
			}
			else if (LOWORD(wParam) >= ADD_REGION_START_MSG && LOWORD(wParam) <= ADD_REGION_END_MSG)
			{
				SNM_PlaylistItem* newItem = new SNM_PlaylistItem(GetMarkerRegionIdFromIndex(LOWORD(wParam)-ADD_REGION_START_MSG));
				if (GetPlaylist()->Add(newItem))
				{
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
	MUTEX_PLAYLISTS;

	LICE_CachedFont* font = SNM_GetThemeFont();
	IconTheme* it = SNM_GetIconTheme();
	int x0=_r->left+10, h=35;
	if (_tooltipHeight)
		*_tooltipHeight = h;
	
	bool hasPlaylists = (g_pPlaylists.Get()->GetSize()>0);
	double playlistLen = hasPlaylists ? GetPlayListLength() : 0.0;
	SNM_SkinButton(&m_btnPlay, it ? &it->gen_play[g_rgnPlaylistEnabled?1:0] : NULL, __LOCALIZE("Play","sws_DLG_165"));
	m_btnPlay.SetGrayed(!hasPlaylists || playlistLen < 0.01);
	if (SNM_AutoVWndPosition(&m_btnPlay, NULL, _r, &x0, _r->top, h, 0))
	{
		SNM_SkinButton(&m_btnStop, it ? &(it->gen_stop) : NULL, __LOCALIZE("Stop","sws_DLG_165"));
		m_btnStop.SetGrayed(!hasPlaylists || playlistLen < 0.01);
		if (SNM_AutoVWndPosition(&m_btnStop, NULL, _r, &x0, _r->top, h))
		{
			m_txtPlaylist.SetFont(font);
			m_txtPlaylist.SetText(hasPlaylists ? __LOCALIZE("Playlist:","sws_DLG_165") : __LOCALIZE("Playlist: None","sws_DLG_165"));
			if (SNM_AutoVWndPosition(&m_txtPlaylist, NULL, _r, &x0, _r->top, h, hasPlaylists?4:SNM_DEF_VWND_X_STEP))
			{
				m_cbPlaylist.SetFont(font);
				if (!hasPlaylists || (hasPlaylists && SNM_AutoVWndPosition(&m_cbPlaylist, NULL, _r, &x0, _r->top, h, 4)))
				{
					((SNM_AddDelButton*)m_parentVwnd.GetChildByID(BUTTONID_DEL_PLAYLIST))->SetEnabled(hasPlaylists);
					if (SNM_AutoVWndPosition(&m_btnsAddDel, NULL, _r, &x0, _r->top, h))
						if (playlistLen > 0.01)
						{
							char len[64]="", timeStr[32];
							format_timestr_pos(playlistLen, timeStr, 32, -1);
							m_txtLength.SetFont(font);
							_snprintf(len, 64, __LOCALIZE_VERFMT("Length: %s","sws_DLG_165"), timeStr);
							m_txtLength.SetText(len);
							if (SNM_AutoVWndPosition(&m_txtLength, NULL, _r, &x0, _r->top, h)) {
								SNM_SkinToolbarButton(&m_btnCrop, __LOCALIZE("Crop project","sws_DLG_165"));
								if (SNM_AutoVWndPosition(&m_btnCrop, NULL, _r, &x0, _r->top, h))
									SNM_AddLogo(_bm, _r, x0, h);
							}
						}
						else
							SNM_AddLogo(_bm, _r, x0, h);
				}
			}
		}
	}
}

HMENU SNM_RegionPlaylistWnd::OnContextMenu(int _x, int _y, bool* _wantDefaultItems)
{
	MUTEX_PLAYLISTS;

	HMENU hMenu = CreatePopupMenu();
	int x=0; bool hasSel = (m_pLists.Get(0)->EnumSelected(&x) != NULL);
	*_wantDefaultItems = !(m_pLists.Get(0)->GetHitItem(_x, _y, &x) && x >= 0);

	if (*_wantDefaultItems)
	{
		HMENU hPlaylistSubMenu = CreatePopupMenu();
		AddSubMenu(hMenu, hPlaylistSubMenu, __LOCALIZE("Playlists","sws_DLG_165"));
		AddToMenu(hPlaylistSubMenu, __LOCALIZE("New playlist...","sws_DLG_165"), NEW_PLAYLIST_MSG);
		AddToMenu(hPlaylistSubMenu, __LOCALIZE("Copy playlist...","sws_DLG_165"), COPY_PLAYLIST_MSG, -1, false, GetPlaylist() ? MF_ENABLED : MF_GRAYED);
		AddToMenu(hPlaylistSubMenu, __LOCALIZE("Rename...","sws_DLG_165"), REN_PLAYLIST_MSG, -1, false, GetPlaylist() ? MF_ENABLED : MF_GRAYED);
		AddToMenu(hPlaylistSubMenu, __LOCALIZE("Delete","sws_DLG_165"), DEL_PLAYLIST_MSG, -1, false, GetPlaylist() ? MF_ENABLED : MF_GRAYED);
	}

	if (GetPlaylist())
	{
		if (GetMenuItemCount(hMenu))
			AddToMenu(hMenu, SWS_SEPARATOR, 0);

		AddToMenu(hMenu, __LOCALIZE("Add all regions","sws_DLG_165"), ADD_ALL_REGIONS_MSG);
		HMENU hAddSubMenu = CreatePopupMenu();
		AddSubMenu(hMenu, hAddSubMenu, __LOCALIZE("Add region","sws_DLG_165"));
		FillMarkerRegionMenu(hAddSubMenu, ADD_REGION_START_MSG, 2);

		if (*_wantDefaultItems) {
			AddToMenu(hMenu, SWS_SEPARATOR, 0);
			AddToMenu(hMenu, __LOCALIZE("Crop project to playlist","sws_DLG_165"), CROP_PRJ_MSG, -1, false, GetPlaylist() && GetPlaylist()->GetSize() ? 0 : MF_GRAYED);
		}
		else
		{
			HMENU hInsertSubMenu = CreatePopupMenu();
			AddSubMenu(hMenu, hInsertSubMenu, __LOCALIZE("Insert region","sws_DLG_165"), -1, hasSel ? 0 : MF_GRAYED);
			FillMarkerRegionMenu(hInsertSubMenu, INSERT_REGION_START_MSG, 2);
			AddToMenu(hMenu, __LOCALIZE("Delete region(s)","sws_DLG_165"), DELETE_MSG, -1, false, hasSel ? 0 : MF_GRAYED);
		}
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

double GetPlayListLength()
{
	MUTEX_PLAYLISTS;

	double length=0.0;
	if (SNM_Playlist* p = GetPlaylist())
		if (int sz = p->GetSize())
			for (int i=0; i<sz; i++)
				if (SNM_PlaylistItem* item = p->Get(i)) {
					double rgnpos, rgnend;
					if (item->m_rgnId>0 && EnumProjectMarkers2(NULL, GetMarkerRegionIndexFromId(item->m_rgnId), NULL, &rgnpos, &rgnend, NULL, NULL))
						length += ((rgnend-rgnpos)*item->m_cnt);
				}
	return length;
}

// do not use things like ..->Get(i+1) but this func!
int GetNextValidPlaylistIdx(int _playlistIdx, int* _nextRgnIdx, bool _startWith = false)
{
	MUTEX_PLAYLISTS;
	if (SNM_Playlist* p = GetPlaylist())
	{
		if (int sz = p->GetSize())
		{
			for (int i=_playlistIdx+(_startWith?0:1); i<sz; i++)
				if (SNM_PlaylistItem* item = p->Get(i))
					if (item->m_rgnId>0 && item->m_cnt>0) {
						*_nextRgnIdx = GetMarkerRegionIndexFromId(item->m_rgnId);
						if (*_nextRgnIdx >= 0)
							return i;
					}
			// no issue if _playlistIdx==0, it will not match either..
			for (int i=0; i<=(_playlistIdx-(_startWith?1:0)); i++)
				if (SNM_PlaylistItem* item = p->Get(i))
					if (item->m_rgnId>0 && item->m_cnt>0) {
						*_nextRgnIdx = GetMarkerRegionIndexFromId(item->m_rgnId);
						if (*_nextRgnIdx >= 0)
							return i;
					}
		}
	}
	*_nextRgnIdx = -1;
	return -1;
}

// needed to make PlaylistRun() as idle as possible..
SNM_PlaylistCurNextItems g_playingItems;
int g_playId = 0;

// just to make PlaylistRun() more readable..
bool GetPlaylistRunItems(double _pos)
{
	MUTEX_PLAYLISTS;
	if (SNM_Playlist* p = GetPlaylist())
	{
		if (int sz = p->GetSize())
		{
			int id, rgnIdx = FindMarkerRegion(_pos, &id);
			if (id>0)
			{
				for (int i=g_playId; i<sz; i++)
				{
					if (SNM_PlaylistItem* item = p->Get(i))
					{
						// assumption: the case "2 consecutive and identical region ids" is impossible, see SNM_PlaylistView::UpdateCompact()
						// +no need to test the region consistency since it is provided by FindMarkerRegion()
						if (item->m_rgnId==id && item->m_cnt>0)
						{
							g_playingItems.m_cur = item;
							g_playingItems.m_curRgnIdx = rgnIdx;
							g_playId = i;
							if (g_playingItems.m_cur->m_cnt>1 && (g_playingItems.m_cur->m_playRequested+1) <= g_playingItems.m_cur->m_cnt)
							{
								g_playingItems.m_next = g_playingItems.m_cur;
								g_playingItems.m_nextRgnIdx = g_playingItems.m_curRgnIdx;
							}
							else
							{
								// "next" is "i" in the worst case
								int nextRgnIdx, next = GetNextValidPlaylistIdx(i, &nextRgnIdx);
								if (next>=0) {
									g_playingItems.m_next = p->Get(next);
									g_playingItems.m_nextRgnIdx = nextRgnIdx;
								}
							}
							return (g_playingItems.m_next != NULL);
						}
					}
				}

				// if we are here, it is a loop!
				int firstRgnIdx, first = GetNextValidPlaylistIdx(0, &firstRgnIdx, true);
				if (first>=0)
				{
					g_playingItems.m_cur = p->Get(first);
					g_playingItems.m_curRgnIdx = firstRgnIdx;
					g_playId = first;
					if (g_playingItems.m_cur->m_cnt>1 && (g_playingItems.m_cur->m_playRequested+1) <= g_playingItems.m_cur->m_cnt) {
						g_playingItems.m_next = g_playingItems.m_cur;
						g_playingItems.m_nextRgnIdx = g_playingItems.m_curRgnIdx;
					}
					else
					{
						// "next" is "first" in the worst case
						int nextRgnIdx, next = GetNextValidPlaylistIdx(first, &nextRgnIdx);
						if (next>=0) {
							g_playingItems.m_next = p->Get(next);
							g_playingItems.m_nextRgnIdx = nextRgnIdx;
						}
					}
					return (g_playingItems.m_next != NULL);
				}
			}
		}
	}
	return false;
}

// needed to make PlaylistRun() as idle as possible..
double g_triggerPos = -1.0;
double g_lastPlayPos = -1.0;
bool g_isRegionLooping = false;

void PlaylistRun()
{
	MUTEX_PLAYLISTS;

	static bool bRecurseCheck = false;
	if (bRecurseCheck || !g_rgnPlaylistEnabled)
		return;

	bRecurseCheck = true;

	bool updateUI = false;
	double pos = GetPlayPosition();

	if (g_triggerPos >= 0.0)
	{
		 if (pos >= g_triggerPos)
		 {
			g_triggerPos = -1.0;
			g_isRegionLooping = false;

			if (SNM_Playlist* p = GetPlaylist())
				if (p->Find(g_playingItems.m_next) >= 0)// still make sense?
				{
					g_playingItems.m_next->m_playRequested++;

					// request play for the next item of the playlist
					double rgnpos;
					if (EnumProjectMarkers2(NULL, g_playingItems.m_nextRgnIdx, NULL, &rgnpos, NULL, NULL, NULL))
					{
						double cursorpos = GetCursorPositionEx(NULL);
						SetEditCurPos(rgnpos, false, true); // seek play
						SetEditCurPos(cursorpos, false, false); // restore cursor pos
					}
				}
		}
	}
	else if (GetPlaylistRunItems(pos))
	{
		// detect playlist item switching
		if (g_lastPlayedItem != g_playingItems.m_cur)
		{
			g_lastPlayedItem = g_playingItems.m_cur;
			g_lastPlayPos = -1.0;
			g_isRegionLooping = false;
			updateUI = true;

			if (SNM_Playlist* p = GetPlaylist())
				if (int sz = p->GetSize())
					for (int i=0; i<sz; i++)
						if (SNM_PlaylistItem* item = p->Get(i))
								item->m_playRequested = 0;
		}
		// detect region looping
		else if (!g_isRegionLooping && pos < g_lastPlayPos)
			g_isRegionLooping = true;

		// compute next trigger position
		if (!g_playingItems.m_next->m_playRequested || g_isRegionLooping)
		{
			double rgnend;
			if (EnumProjectMarkers2(NULL, g_playingItems.m_curRgnIdx, NULL, NULL, &rgnend, NULL, NULL))
			{
				int measEnd; TimeMap2_timeToBeats(NULL, rgnend, &measEnd, NULL, NULL, NULL);
				measEnd--;
				g_triggerPos = TimeMap2_beatsToTime(NULL, 0.0, &measEnd);
			}
		}
	}

	g_lastPlayPos = pos;

//JFB!!! TODO optimiz
	if (updateUI && g_pRgnPlaylistWnd)
		g_pRgnPlaylistWnd->Update(4);

	bRecurseCheck = false;
}

int g_oldSeekOpt = -1;

void PlaylistPlay(int _playlistId, bool _errMsg)
{
	MUTEX_PLAYLISTS;

	g_rgnPlaylistEnabled = false;
	int firstRgnIdx, first = GetNextValidPlaylistIdx(_playlistId, &firstRgnIdx, true);
	if (first>=0 && firstRgnIdx>=0)
	{
		double startpos;
		if (EnumProjectMarkers2(NULL, firstRgnIdx, NULL, &startpos, NULL, NULL, NULL))
		{
			// temp override of the "smooth seek" option
			if (int* opt = (int*)GetConfigVar("smoothseek")) {
				g_oldSeekOpt = *opt;
				*opt = 1;
			}
			g_playId = first;
			g_lastPlayedItem = NULL;
			g_triggerPos = -1.0;

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
	if (_errMsg && !g_rgnPlaylistEnabled)
		MessageBox(g_pRgnPlaylistWnd?g_pRgnPlaylistWnd->GetHWND():GetMainHwnd(), __LOCALIZE("Nothing to play!\n(empty playlist, removed regions, etc..)","sws_DLG_165"),  __LOCALIZE("S&M -Error","sws_DLG_165"), MB_OK);
}

void PlaylistPlay(COMMAND_T* _ct) {
	PlaylistPlay((int)_ct->user, false);
}

void PlaylistStopped()
{
	MUTEX_PLAYLISTS;
	g_rgnPlaylistEnabled = false;
	g_lastPlayedItem = NULL;

	// restore the "smooth seek" option
	if (int* opt = (int*)GetConfigVar("smoothseek"))
		if (g_oldSeekOpt >= 0) {
			*opt = g_oldSeekOpt;
			g_oldSeekOpt = -1;
		}

	if (g_pRgnPlaylistWnd)
		g_pRgnPlaylistWnd->Update(4);
}


///////////////////////////////////////////////////////////////////////////////

void CropProjectToPlaylist()
{
	MUTEX_PLAYLISTS;

	bool updated = false;
	double startPos, endPos;
	WDL_PtrList_DeleteOnDestroy<MarkerItem> rgns;
	for (int i=0; GetPlaylist() && i < GetPlaylist()->GetSize(); i++)
	{
		if (SNM_PlaylistItem* item = GetPlaylist()->Get(i))
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

						// store regions
						bool found = false;
						for (int k=0; !found && k<rgns.GetSize(); k++)
							if (rgns.Get(k)->GetID() == rgnnum) // poorely named, it is not an "ID"
								found = true;
						if (!found)
							rgns.Add(new MarkerItem(true, endPos-startPos, endPos+rgnend-rgnpos-startPos, rgnname, rgnnum, rgncol));

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
						for (int j=0; j<item->m_cnt; j++) {
							ApplyNudge(NULL, 1, 5, 1, endPos, false, 1); // append at end of project
							endPos += (rgnend-rgnpos);
						}

						// restore options
						for (int j=0; j<2; j++)
							if (options[j])
								*options[j] = oldOpt[j];
					}
				}
			}
		}
	}

	if (updated)
	{
		// remove markers/regions
		int x=0, lastx=0, num; bool isRgn;
		while ((x = EnumProjectMarkers2(NULL, x, &isRgn, NULL, NULL, NULL, &num))) {
			if (DeleteProjectMarker(NULL, num, isRgn))
				x = lastx;
			lastx = x;
		}

		// crop project
		GetSet_LoopTimeRange(true, false, &startPos, &endPos, false);
		Main_OnCommand(40049, 0);// crop project to time sel

		// restore regions used in playlist
		for (int i=0; i<rgns.GetSize(); i++)
			rgns.Get(i)->AddToProject();

		// clear time sel + edit cursor position
		GetSet_LoopTimeRange(true, false, &g_d0, &g_d0, false);
		SetEditCurPos(0.0, true, false);		
		Main_OnCommand(40289, 0); //JFB!!! unselect all items

		// remove playlists that use removed regions
		for (int i=g_pPlaylists.Get()->GetSize()-1; i>=0; i--)
			if (i != g_curPlaylist)
				for (int j=0; j<GetPlaylist(i)->GetSize(); j++)
					if (GetMarkerRegionIndexFromId(GetPlaylist(i)->Get(j)->m_rgnId) < 0) {
						g_pPlaylists.Get()->Delete(i, true);
						g_curPlaylist = BOUNDED(g_curPlaylist>i ? g_curPlaylist : g_curPlaylist-1, 0, g_pPlaylists.Get()->GetSize()-1);
						break;
					}

		if (g_pRgnPlaylistWnd) {
			g_pRgnPlaylistWnd->FillPlaylistCombo();
			g_pRgnPlaylistWnd->Update();
		}

		Undo_EndBlock2(NULL, __LOCALIZE("Crop project to playlist","sws_undo"), UNDO_STATE_ALL);
	}
}


///////////////////////////////////////////////////////////////////////////////

void Playlist_MarkerRegionSubscriber::NotifyMarkerRegionUpdate(int _updateFlags) {
	if (g_pRgnPlaylistWnd)
		g_pRgnPlaylistWnd->Update();
}


///////////////////////////////////////////////////////////////////////////////

static bool ProcessExtensionLine(const char *line, ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	LineParser lp(false);
	if (lp.parse(line) || lp.getnumtokens() < 1)
		return false;

	if (!strcmp(lp.gettoken_str(0), "<S&M_RGN_PLAYLIST"))
	{
		MUTEX_PLAYLISTS;
		if (SNM_Playlist* playlist = g_pPlaylists.Get()->Add(new SNM_Playlist(lp.gettoken_str(1))))
		{
			char linebuf[SNM_MAX_CHUNK_LINE_LENGTH]="";
			while(true)
			{
				if (!ctx->GetLine(linebuf,sizeof(linebuf)) && !lp.parse(linebuf))
				{
					if (lp.getnumtokens() && lp.gettoken_str(0)[0] == '>')
						break;
					else if (lp.getnumtokens() == 2)
						playlist->Add(new SNM_PlaylistItem(lp.gettoken_int(0), lp.gettoken_int(1)));
				}
				else
					break;
			}
			if (g_pRgnPlaylistWnd) {
				g_curPlaylist=0;
				g_pRgnPlaylistWnd->FillPlaylistCombo();
				g_pRgnPlaylistWnd->Update();
			}
			return true;
		}
	}
	return false;
}

static void SaveExtensionConfig(ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	MUTEX_PLAYLISTS;
	for (int j=0; j < g_pPlaylists.Get()->GetSize(); j++)
	{
		WDL_FastString confStr("<S&M_RGN_PLAYLIST "), escStr;
		makeEscapedConfigString(GetPlaylist(j)->m_name.Get(), &escStr);
		confStr.Append(escStr.Get());
		confStr.Append("\n");
		int iHeaderLen = confStr.GetLength();

		for (int i=0; i < GetPlaylist(j)->GetSize(); i++)
			if (SNM_PlaylistItem* item = GetPlaylist(j)->Get(i))
				confStr.AppendFormatted(128,"  %d %d\n", item->m_rgnId, item->m_cnt);

		if (confStr.GetLength() > iHeaderLen) {
			confStr.Append(">\n");
			StringToExtensionConfig(&confStr, ctx);
		}
	}
}

static void BeginLoadProjectState(bool isUndo, struct project_config_extension_t *reg)
{
	MUTEX_PLAYLISTS;
	g_pPlaylists.Cleanup();
	g_pPlaylists.Get()->Empty(true);

	if (g_pRgnPlaylistWnd) {
		g_curPlaylist=0;
		g_pRgnPlaylistWnd->FillPlaylistCombo();
		g_pRgnPlaylistWnd->Update();
	}
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
	if (g_pRgnPlaylistWnd)
		g_pRgnPlaylistWnd->Show(true, true);
}

bool IsRegionPlaylistDisplayed(COMMAND_T*){
	return (g_pRgnPlaylistWnd && g_pRgnPlaylistWnd->IsValidWindow());
}
