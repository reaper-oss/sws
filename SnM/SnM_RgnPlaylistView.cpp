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
#include "SnM.h"
#include "../Prompt.h"
#include "../reaper/localize.h"

#define MUTEX_PLAYLISTS			SWS_SectionLock lock(&g_plsMutex) // for quick un-mutexing

#define DELETE_MSG				0xF000
#define PERFORM_MSG				0xF001
#define CROP_PRJ_MSG			0xF002
#define NEW_PLAYLIST_MSG		0xF003
#define COPY_PLAYLIST_MSG		0xF004
#define DEL_PLAYLIST_MSG		0xF005
#define REN_PLAYLIST_MSG		0xF006
#define ADD_ALL_REGIONS_MSG		0xF007
#define CROP_PRJTAB_MSG			0xF008
#define APPEND_PRJ_MSG			0xF009
#define PASTE_CURSOR_MSG		0xF00A
#define APPEND_SEL_RGN_MSG		0xF00B
#define PASTE_SEL_RGN_MSG		0xF00C
#define ADD_REGION_START_MSG	0xF100
#define ADD_REGION_END_MSG		0xF7FF // => 1791 marker/region *indexes* supported, oh well..
#define INSERT_REGION_START_MSG	0xF800
#define INSERT_REGION_END_MSG	0xFEFF // => 1791 marker/region indexes supported

#define UNDO_PLAYLIST_STR		__LOCALIZE("Region Playlist edition", "sws_undo")


enum {
  BUTTONID_PLAY=2000, //JFB would be great to have _APS_NEXT_CONTROL_VALUE *always* defined
  BUTTONID_STOP,
  BUTTONID_REPEAT,
  TXTID_PLAYLIST,
  COMBOID_PLAYLIST,
  CONTAINER_ADD_DEL,
  BUTTONID_NEW_PLAYLIST,
  BUTTONID_DEL_PLAYLIST,
  TXTID_LENGTH,
  BUTTONID_PASTE
};

enum {
  COL_RGN=0,
  COL_RGN_COUNT,
  COL_COUNT
};


SNM_RegionPlaylistWnd* g_pRgnPlaylistWnd = NULL;
SWSProjConfig<SNM_Playlists> g_pls;
SWS_Mutex g_plsMutex;

bool g_playingPlaylist = false;
bool g_repeatPlaylist = false;
int g_playId = -1;

SNM_Playlist* GetPlaylist(int _id = -1)
{
	MUTEX_PLAYLISTS;
	if (_id < 0) _id = g_pls.Get()->m_cur;
	return g_pls.Get()->Get(_id);
}


///////////////////////////////////////////////////////////////////////////////
// SNM_PlaylistView
///////////////////////////////////////////////////////////////////////////////

// !WANT_LOCALIZE_STRINGS_BEGIN:sws_DLG_165
static SWS_LVColumn g_playlistCols[] = { {200,2,"Region"}, {70,1,"Loop count"} };
// !WANT_LOCALIZE_STRINGS_END

SNM_PlaylistView::SNM_PlaylistView(HWND hwndList, HWND hwndEdit)
	: SWS_ListView(hwndList, hwndEdit, COL_COUNT, g_playlistCols, "RgnPlaylistViewState", false, "sws_DLG_165", false)
{
}

// "compact" the playlist (e.g. 2 consecutive regions "3" are merged into one, its counter is incremented)
void SNM_PlaylistView::UpdateCompact()
{
	MUTEX_PLAYLISTS;
	if (SNM_Playlist* pl = GetPlaylist())
		for (int i=pl->GetSize()-1; i>=0 ; i--)
			if (SNM_PlaylistItem* item = pl->Get(i))
				if ((i-1)>=0 && pl->Get(i-1) && item->m_rgnId == pl->Get(i-1)->m_rgnId) {
					pl->Get(i-1)->m_cnt++;
					pl->Delete(i, true);
				}
	Update();
}

void SNM_PlaylistView::GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax)
{
	MUTEX_PLAYLISTS;
	if (str) *str = '\0';
	if (SNM_PlaylistItem* pItem = (SNM_PlaylistItem*)item)
	{
		switch (iCol)
		{
			case COL_RGN: {
				char buf[128]="";
				if (pItem->m_rgnId>0 && EnumMarkerRegionDesc(GetMarkerRegionIndexFromId(pItem->m_rgnId), buf, 128, SNM_REGION_MASK, true) && *buf) {
					_snprintfSafe(str, iStrMax, "%s %s", GetPlaylist() && GetPlaylist()->Get(g_playId)==pItem ? UTF8_BULLET : " ", buf);
					break;
				}
				int num = pItem->m_rgnId & 0x3FFFFFFF;
				_snprintfSafe(str, iStrMax, __LOCALIZE("Unknown region %d","sws_DLG_165"), num);
				break;
			}
			case COL_RGN_COUNT:
				_snprintfSafe(str, iStrMax, "%d", pItem->m_cnt);
				break;
		}
	}
}

void SNM_PlaylistView::GetItemList(SWS_ListItemList* pList)
{
	MUTEX_PLAYLISTS;
	if (SNM_Playlist* pl = GetPlaylist())
		for (int i=0; i < pl->GetSize(); i++)
			if (SWS_ListItem* item = (SWS_ListItem*)pl->Get(i))
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
	if (SNM_Playlist* pl = GetPlaylist())
	{
		for (int i=0; (i1<0 && i2<0) || i<pl->GetSize(); i++)
		{
			SWS_ListItem* item = (SWS_ListItem*)pl->Get(i);
			if (item == _item1) i1=i;
			if (item == _item2) i2=i;
		}
		if (i1 >= 0 && i2 >= 0) {
			if (i1 > i2) return 1;
			else if (i1 < i2) return -1;
		}
	}
	return 0;
}

void SNM_PlaylistView::OnBeginDrag(SWS_ListItem* item) {
	SetCapture(GetParent(m_hwndList));
}

void SNM_PlaylistView::OnDrag()
{
	MUTEX_PLAYLISTS;
	SNM_Playlist* pl  = GetPlaylist();
	if (!pl) return;

	POINT p; GetCursorPos(&p);
	if (SNM_PlaylistItem* hitItem = (SNM_PlaylistItem*)GetHitItem(p.x, p.y, NULL))
	{
		int iNewPriority = pl->Find(hitItem);
		int x=0, iSelPriority;
		while(SNM_PlaylistItem* selItem = (SNM_PlaylistItem*)EnumSelected(&x))
		{
			iSelPriority = pl->Find(selItem);
			if (iNewPriority == iSelPriority) return;
			m_draggedItems.Add(selItem);
		}

		bool bDir = iNewPriority > iSelPriority;
		for (int i = bDir ? 0 : m_draggedItems.GetSize()-1; bDir ? i < m_draggedItems.GetSize() : i >= 0; bDir ? i++ : i--) {
			pl->Delete(pl->Find(m_draggedItems.Get(i)), false);
			pl->Insert(iNewPriority, m_draggedItems.Get(i));
		}

		ListView_DeleteAllItems(m_hwndList); // because of the special sort criteria ("not sortable" somehow)
		Update(); // no UpdateCompact() here, it would crash! see OnEndDrag()

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
	m_btnRepeat.SetID(BUTTONID_REPEAT);
	m_parentVwnd.AddChild(&m_btnRepeat);

	m_txtPlaylist.SetID(TXTID_PLAYLIST);
	m_parentVwnd.AddChild(&m_txtPlaylist);

	m_cbPlaylist.SetID(COMBOID_PLAYLIST);
	FillPlaylistCombo();
	m_parentVwnd.AddChild(&m_cbPlaylist);

	m_btnsAddDel.SetIDs(CONTAINER_ADD_DEL, BUTTONID_NEW_PLAYLIST, BUTTONID_DEL_PLAYLIST);
	m_parentVwnd.AddChild(&m_btnsAddDel);

	m_txtLength.SetID(TXTID_LENGTH);
	m_parentVwnd.AddChild(&m_txtLength);

	m_btnCrop.SetID(BUTTONID_PASTE);
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

// ScheduledJob used because of possible multi-notifs
void SNM_RegionPlaylistWnd::CSurfSetTrackListChange() {
	AddOrReplaceScheduledJob(new SNM_Playlist_UpdateJob());
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
		ShowWindow(GetDlgItem(m_hwnd, IDC_LIST), g_pls.Get()->GetSize() ? SW_SHOW : SW_HIDE);
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
	for (int i=0; i < g_pls.Get()->GetSize(); i++)
		m_cbPlaylist.AddItem(g_pls.Get()->Get(i)->m_name.Get());
	m_cbPlaylist.SetCurSel(g_pls.Get()->m_cur);
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
				g_pls.Get()->m_cur = m_cbPlaylist.GetCurSel();
				Update();
//				Undo_OnStateChangeEx(UNDO_PLAYLIST_STR, UNDO_STATE_MISCCFG, -1); 
			}
			break;
		case NEW_PLAYLIST_MSG:
		case BUTTONID_NEW_PLAYLIST:
		case COPY_PLAYLIST_MSG:
		{
			char name[64]="";
			lstrcpyn(name, __LOCALIZE("Untitled","sws_DLG_165"), 64);
			if (PromptUserForString(m_hwnd, __LOCALIZE("S&M - Add playlist","sws_DLG_165"), name, 64) && *name)
			{
				if (g_pls.Get()->Add(new SNM_Playlist(LOWORD(wParam)==COPY_PLAYLIST_MSG ? GetPlaylist() : NULL, name)))
				{
					g_pls.Get()->m_cur = g_pls.Get()->GetSize()-1;
					FillPlaylistCombo();
					Update();
					Undo_OnStateChangeEx(UNDO_PLAYLIST_STR, UNDO_STATE_MISCCFG, -1); 
				}
			}
			break;
		}
		case DEL_PLAYLIST_MSG:
		case BUTTONID_DEL_PLAYLIST:
			if (GetPlaylist() && g_pls.Get()->GetSize() > 0)
			{
				WDL_PtrList_DeleteOnDestroy<SNM_Playlist> delItems;
				int reply = IDOK;
				if (GetPlaylist()->GetSize()) // do not ask if empty
					reply = MessageBox(m_hwnd, __LOCALIZE("Are you sure you want to delete this playlist ?","sws_DLG_165"), __LOCALIZE("S&M - Delete playlist","sws_DLG_165"), MB_OKCANCEL);
				if (reply != IDCANCEL)
				{
					delItems.Add(g_pls.Get()->Get(g_pls.Get()->m_cur));
					g_pls.Get()->Delete(g_pls.Get()->m_cur, false);  // no deletion yet (still used in GUI)
					g_pls.Get()->m_cur = BOUNDED(g_pls.Get()->m_cur-1, 0, g_pls.Get()->GetSize()-1);
					FillPlaylistCombo();
					Update();
					Undo_OnStateChangeEx(UNDO_PLAYLIST_STR, UNDO_STATE_MISCCFG, -1); 
				}
			} // + delItems cleanup
			break;
		case REN_PLAYLIST_MSG:
			if (GetPlaylist() && g_pls.Get()->GetSize() > 0)
			{
				char newName[64]="";
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
			OnStopButton();
			break;
		case BUTTONID_REPEAT:
			g_repeatPlaylist = !g_repeatPlaylist;
			break;
		case CROP_PRJ_MSG:
			AppendPasteCropPlaylist(GetPlaylist(), 0);
			break;
		case CROP_PRJTAB_MSG:
			AppendPasteCropPlaylist(GetPlaylist(), 1);
			break;
		case APPEND_PRJ_MSG:
			AppendPasteCropPlaylist(GetPlaylist(), 2);
			break;
		case BUTTONID_PASTE:
		case PASTE_CURSOR_MSG:
			AppendPasteCropPlaylist(GetPlaylist(), 3);
			break;
		case DELETE_MSG: {
			int x=0, slot; bool updt = false;
			WDL_PtrList_DeleteOnDestroy<SNM_PlaylistItem> delItems;
			while(SNM_PlaylistItem* item = (SNM_PlaylistItem*)m_pLists.Get(0)->EnumSelected(&x))
			{
				slot = GetPlaylist()->Find(item);
				if (slot>=0)
				{
					delItems.Add(item);
					GetPlaylist()->Delete(slot, false);  // no deletion yet (still used in GUI)
					updt=true;
				}
			}
			if (updt) {
				Undo_OnStateChangeEx(UNDO_PLAYLIST_STR, UNDO_STATE_MISCCFG, -1); 
				Update();
			} // + delItems cleanup
			break;
		}
		case APPEND_SEL_RGN_MSG:
		case PASTE_SEL_RGN_MSG:
		{
			SNM_Playlist p("temp");
			int x=0;
			while(SNM_PlaylistItem* item = (SNM_PlaylistItem*)m_pLists.Get(0)->EnumSelected(&x))
				p.Add(new SNM_PlaylistItem(item->m_rgnId, item->m_cnt));
			AppendPasteCropPlaylist(&p, LOWORD(wParam)==PASTE_SEL_RGN_MSG ? 3:2);
			break;
		}
		case PERFORM_MSG: 
			if (GetPlaylist()) {
				int x=0;
				if (SNM_PlaylistItem* pItem = (SNM_PlaylistItem*)m_pLists.Get(0)->EnumSelected(&x))
					PlaylistPlay(GetPlaylist()->Find(pItem), true);
			}
			break;
		case ADD_ALL_REGIONS_MSG: {
				int x=0, y, num; bool isRgn, updt=false;
				while (y = EnumProjectMarkers2(NULL, x, &isRgn, NULL, NULL, NULL, &num))
				{
					if (isRgn) {
						SNM_PlaylistItem* newItem = new SNM_PlaylistItem(MakeMarkerRegionId(num, isRgn));
						updt |= (GetPlaylist() && GetPlaylist()->Add(newItem) != NULL);
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
				SNM_Playlist* pl = GetPlaylist();
				if (!pl) break;
				SNM_PlaylistItem* newItem = new SNM_PlaylistItem(GetMarkerRegionIdFromIndex(LOWORD(wParam)-INSERT_REGION_START_MSG));
				if (pl->GetSize())
				{
					if (SNM_PlaylistItem* item = (SNM_PlaylistItem*)m_pLists.Get(0)->EnumSelected(NULL))
					{
						int slot = pl->Find(item);
						if (slot >= 0 && pl->Insert(slot, newItem))
						{
							Update();
							m_pLists.Get(0)->SelectByItem((SWS_ListItem*)newItem);
							Undo_OnStateChangeEx(UNDO_PLAYLIST_STR, UNDO_STATE_MISCCFG, -1); 
							return;
						}
					}
				}
				// empty list, no selection, etc.. => add
				if (pl->Add(newItem))
				{
					Update();
					m_pLists.Get(0)->SelectByItem((SWS_ListItem*)newItem);
					Undo_OnStateChangeEx(UNDO_PLAYLIST_STR, UNDO_STATE_MISCCFG, -1); 
				}
			}
			else if (LOWORD(wParam) >= ADD_REGION_START_MSG && LOWORD(wParam) <= ADD_REGION_END_MSG)
			{
				SNM_PlaylistItem* newItem = new SNM_PlaylistItem(GetMarkerRegionIdFromIndex(LOWORD(wParam)-ADD_REGION_START_MSG));
				if (GetPlaylist() && GetPlaylist()->Add(newItem))
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
	
	bool hasPlaylists = (g_pls.Get()->GetSize()>0);
	double playlistLen = hasPlaylists ? GetPlayListLength(GetPlaylist()) : 0.0;

	SNM_SkinButton(&m_btnPlay, it ? &it->gen_play[g_playingPlaylist?1:0] : NULL, __LOCALIZE("Play","sws_DLG_165"));
	m_btnPlay.SetGrayed(!hasPlaylists || playlistLen < 0.01);
	if (SNM_AutoVWndPosition(&m_btnPlay, NULL, _r, &x0, _r->top, h, 0))
	{
		SNM_SkinButton(&m_btnStop, it ? &(it->gen_stop) : NULL, __LOCALIZE("Stop","sws_DLG_165"));
		m_btnStop.SetGrayed(!hasPlaylists || playlistLen < 0.01);
		if (SNM_AutoVWndPosition(&m_btnStop, NULL, _r, &x0, _r->top, h, 0))
		{
			SNM_SkinButton(&m_btnRepeat, it ? &it->gen_repeat[g_repeatPlaylist?1:0] : NULL, __LOCALIZE("Repeat","sws_DLG_165"));
			m_btnRepeat.SetGrayed(!hasPlaylists || playlistLen < 0.01);
			if (SNM_AutoVWndPosition(&m_btnRepeat, NULL, _r, &x0, _r->top, h))
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
								_snprintfSafe(len, sizeof(len), __LOCALIZE_VERFMT("Length: %s","sws_DLG_165"), timeStr);
								m_txtLength.SetText(len);
								if (SNM_AutoVWndPosition(&m_txtLength, NULL, _r, &x0, _r->top, h)) {
									SNM_SkinToolbarButton(&m_btnCrop, __LOCALIZE("Paste playlist","sws_DLG_165"));
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
}

HMENU SNM_RegionPlaylistWnd::OnContextMenu(int _x, int _y, bool* _wantDefaultItems)
{
	MUTEX_PLAYLISTS;

	HMENU hMenu = CreatePopupMenu();

	// wnd popup
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
		FillMarkerRegionMenu(hAddSubMenu, ADD_REGION_START_MSG, SNM_REGION_MASK);

		if (*_wantDefaultItems) {
			AddToMenu(hMenu, SWS_SEPARATOR, 0);
			AddToMenu(hMenu, __LOCALIZE("Crop project to playlist","sws_DLG_165"), CROP_PRJ_MSG, -1, false, GetPlaylist() && GetPlaylist()->GetSize() ? 0 : MF_GRAYED);
			AddToMenu(hMenu, __LOCALIZE("Crop project to playlist (new project tab)","sws_DLG_165"), CROP_PRJTAB_MSG, -1, false, GetPlaylist() && GetPlaylist()->GetSize() ? 0 : MF_GRAYED);
			AddToMenu(hMenu, __LOCALIZE("Append playlist to project","sws_DLG_165"), APPEND_PRJ_MSG, -1, false, GetPlaylist() && GetPlaylist()->GetSize() ? 0 : MF_GRAYED);
			AddToMenu(hMenu, __LOCALIZE("Paste playlist at edit cursor","sws_DLG_165"), PASTE_CURSOR_MSG, -1, false, GetPlaylist() && GetPlaylist()->GetSize() ? 0 : MF_GRAYED);
		}
		else
		{
			HMENU hInsertSubMenu = CreatePopupMenu();
			AddSubMenu(hMenu, hInsertSubMenu, __LOCALIZE("Insert region","sws_DLG_165"), -1, hasSel ? 0 : MF_GRAYED);
			FillMarkerRegionMenu(hInsertSubMenu, INSERT_REGION_START_MSG, SNM_REGION_MASK);
			AddToMenu(hMenu, __LOCALIZE("Remove region(s)","sws_DLG_165"), DELETE_MSG, -1, false, hasSel ? 0 : MF_GRAYED);
			AddToMenu(hMenu, SWS_SEPARATOR, 0);
			AddToMenu(hMenu, __LOCALIZE("Append region(s) to project","sws_DLG_165"), APPEND_SEL_RGN_MSG, -1, false, hasSel ? 0 : MF_GRAYED);
			AddToMenu(hMenu, __LOCALIZE("Paste region(s) at edit cursor","sws_DLG_165"), PASTE_SEL_RGN_MSG, -1, false, hasSel ? 0 : MF_GRAYED);
		}
	}
	return hMenu;
}
bool SNM_RegionPlaylistWnd::GetToolTipString(int _xpos, int _ypos, char* _bufOut, int _bufOutSz)
{
	if (WDL_VWnd* v = m_parentVwnd.VirtWndFromPoint(_xpos,_ypos,1))
	{
		switch (v->GetID())
		{
			case BUTTONID_PLAY:
				return (_snprintfStrict(_bufOut, _bufOutSz, "%s", __LOCALIZE("Play preview","sws_DLG_165")) > 0);
			case BUTTONID_STOP:
				return (_snprintfStrict(_bufOut, _bufOutSz, __LOCALIZE_VERFMT("Stop","sws_DLG_165"), "") > 0);
			case BUTTONID_REPEAT:
				return (_snprintfStrict(_bufOut, _bufOutSz, __LOCALIZE_VERFMT("Toggle repeat","sws_DLG_165"), "") > 0);
//			case TXTID_PLAYLIST:
//				return false;
			case COMBOID_PLAYLIST:
				return (_snprintfStrict(_bufOut, _bufOutSz, "%s", __LOCALIZE("Current playlist","sws_DLG_165")) > 0);
			case BUTTONID_NEW_PLAYLIST:
				return (_snprintfStrict(_bufOut, _bufOutSz, "%s", __LOCALIZE("Add playlist","sws_DLG_165")) > 0);
			case BUTTONID_DEL_PLAYLIST:
				return (_snprintfStrict(_bufOut, _bufOutSz, "%s", __LOCALIZE("Delete playlist","sws_DLG_165")) > 0);
			case TXTID_LENGTH:
				return (_snprintfStrict(_bufOut, _bufOutSz, "%s", __LOCALIZE("Total playlist length","sws_DLG_165")) > 0);
			case BUTTONID_PASTE:
				return (_snprintfStrict(_bufOut, _bufOutSz, "%s", __LOCALIZE("Paste playlist at edit cursor\n(right-click for other append/crop commands)","sws_DLG_165")) > 0);
		}
	}
	return false;
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

// return the first found playlist idx
bool IsInPlaylist(SNM_Playlist* _playlist, double _pos)
{
	for (int i=0; _playlist && i<_playlist->GetSize(); i++)
		if (SNM_PlaylistItem* plItem = _playlist->Get(i)) {
			double rgnpos, rgnend;
			if (plItem->m_rgnId>0 && EnumProjectMarkers2(NULL, GetMarkerRegionIndexFromId(plItem->m_rgnId), NULL, &rgnpos, &rgnend, NULL, NULL))
				if (_pos >= rgnpos && _pos <= rgnend)
					return true;
		}
	return false;
}

int IsInPlaylists(double _pos)
{
	MUTEX_PLAYLISTS;
	for (int i=0; i < g_pls.Get()->GetSize(); i++)
		if (IsInPlaylist(g_pls.Get()->Get(i), _pos))
			return i;
	return -1;
}

int HasPlaylistNestedMarkersRegions()
{
	MUTEX_PLAYLISTS;
	SNM_Playlist* pl = GetPlaylist();
	for (int i=0; pl && i<pl->GetSize(); i++)
	{
		if (SNM_PlaylistItem* plItem = pl->Get(i))
		{
			double rgnpos, rgnend;
			int rgnidx = GetMarkerRegionIndexFromId(plItem->m_rgnId);
			if (rgnidx>0 && EnumProjectMarkers2(NULL, rgnidx, NULL, &rgnpos, &rgnend, NULL, NULL))
			{
				int x=0, lastx=0; double dPos, dEnd; bool isRgn;
				while (x = EnumProjectMarkers2(NULL, x, &isRgn, &dPos, &dEnd, NULL, NULL))
				{
					if (rgnidx != lastx)
					{
						if (isRgn) {
							if ((dPos > rgnpos && dPos < rgnend) || (dEnd > rgnpos && dEnd < rgnend))
								return GetMarkerRegionNumFromId(plItem->m_rgnId);
						}
						else {
							if (dPos > rgnpos && dPos < rgnend)
								return GetMarkerRegionNumFromId(plItem->m_rgnId);
						}
					}
					lastx=x;
				}
			}
		}
	}
	return -1;
}

double GetPlayListLength(SNM_Playlist* _playlist)
{
	double length=0.0;
	for (int i=0; _playlist && i<_playlist->GetSize(); i++)
		if (SNM_PlaylistItem* plItem = _playlist->Get(i)) {
			double rgnpos, rgnend;
			if (plItem->m_rgnId>0 && EnumProjectMarkers2(NULL, GetMarkerRegionIndexFromId(plItem->m_rgnId), NULL, &rgnpos, &rgnend, NULL, NULL))
				length += ((rgnend-rgnpos)*plItem->m_cnt);
		}
	return length;
}


///////////////////////////////////////////////////////////////////////////////
// Polling on play: PlaylistRun() and related funcs
///////////////////////////////////////////////////////////////////////////////

// never use things like GetPlaylist()->Get(i+1) but this func
int GetNextValidPlaylistIdx(int _playlistIdx, bool _startWith = false)
{
	if (_playlistIdx>=0)
	{
		MUTEX_PLAYLISTS;
		if (SNM_Playlist* pl = GetPlaylist())
		{
			for (int i=_playlistIdx+(_startWith?0:1); i<pl->GetSize(); i++)
				if (SNM_PlaylistItem* item = pl->Get(i))
					if (item->m_rgnId>0 && (item->m_cnt>=1 && item->m_playReq<item->m_cnt) && GetMarkerRegionIndexFromId(item->m_rgnId)>=0)
						return i;
			// no issue if _playlistIdx==0, it will not match either..
			for (int i=0; i<pl->GetSize() && i<=(_playlistIdx-(_startWith?1:0)); i++)
				if (SNM_PlaylistItem* item = pl->Get(i))
					if (item->m_rgnId>0 && (item->m_cnt>=1 && item->m_playReq<item->m_cnt) && GetMarkerRegionIndexFromId(item->m_rgnId)>=0)
						return i;
		}
	}
	return -1;
}

// needed to make PlaylistRun() as idle as possible..
bool g_isRunLoop = false;
int g_playNextId = -1;
double g_lastRunPos = -1.0;
double g_nextRunPos;
double g_nextRunEnd;

int g_oldSeekPref = -1;
int g_oldStopprojlenPref = -1;
int g_oldRepeatState = -1;

void PlaylistRun()
{
	MUTEX_PLAYLISTS;

	static bool bRecurseCheck = false;
	if (bRecurseCheck || !g_playingPlaylist)
		return;
	bRecurseCheck = true;

	double pos = GetPlayPosition2();
	bool updateUI = false;
	if (SNM_Playlist* pl = GetPlaylist())
	{
		if (g_isRunLoop && pos<g_lastRunPos)
			g_isRunLoop = false;

		if (!g_isRunLoop && pos>=g_nextRunPos && pos<=g_nextRunEnd)
		{
			g_playId = g_playNextId;
			updateUI = true;

			SNM_PlaylistItem* cur = pl->Get(g_playId);
			g_isRunLoop = (cur && cur->m_cnt>1 && cur->m_playReq<cur->m_cnt); // region looping?
			if (!g_isRunLoop)
				for (int i=0; i<pl->GetSize(); i++)
					pl->Get(i)->m_playReq = 0;

			int inext = GetNextValidPlaylistIdx(g_playId, g_isRunLoop);
			if (SNM_PlaylistItem* next = pl->Get(inext))
			{
				if (cur && cur->m_rgnId==next->m_rgnId && g_playId!=inext)
					g_isRunLoop = true;

				g_playNextId = inext;
				next->m_playReq++;

				// trick to stop the playlist in sync: smooth seek to the end of the project
				if (!g_repeatPlaylist && inext<g_playId)
				{
					// temp override of the "stop play at project and" option
					if (int* opt = (int*)GetConfigVar("stopprojlen")) {
						g_oldStopprojlenPref = *opt;
						*opt = 1;
					}
					g_nextRunPos = GetProjectLength()+1.0;
					g_nextRunEnd = g_nextRunPos+0.5;
					SeekPlay(g_nextRunPos);
				}
				else if (EnumProjectMarkers2(NULL, GetMarkerRegionIndexFromId(next->m_rgnId), NULL, &g_nextRunPos, &g_nextRunEnd, NULL, NULL))
					SeekPlay(g_nextRunPos);
			}
		}
	}

	g_lastRunPos = pos;
	if (updateUI && g_pRgnPlaylistWnd)
		g_pRgnPlaylistWnd->Update(4);

	bRecurseCheck = false;
}

void PlaylistPlay(int _playlistId, bool _errMsg)
{
	MUTEX_PLAYLISTS;

	SNM_Playlist* pl = GetPlaylist();
	if (!pl) return;

	int num = HasPlaylistNestedMarkersRegions();
	if (num>0) {
		char msg[128] = "";
		_snprintfSafe(msg, sizeof(msg), __LOCALIZE_VERFMT("Play preview might not work as expected!\nThe playlist constains nested markers/regions (inside region %d at least)","sws_DLG_165"), num);
		MessageBox(g_pRgnPlaylistWnd?g_pRgnPlaylistWnd->GetHWND():GetMainHwnd(), msg, __LOCALIZE("S&M - Warning","sws_DLG_165"),MB_OK);
	}

	OnStopButton();
	PlaylistStopped();
	for (int i=0; i < pl->GetSize(); i++)
		pl->Get(i)->m_playReq = 0;

	int idx = GetNextValidPlaylistIdx(_playlistId, true);
	if (SNM_PlaylistItem* cur = pl->Get(idx))
		if (EnumProjectMarkers2(NULL, GetMarkerRegionIndexFromId(cur->m_rgnId), NULL, &g_nextRunPos, &g_nextRunEnd, NULL, NULL))
		{
			// temp override of the "smooth seek" option
			if (int* opt = (int*)GetConfigVar("smoothseek")) {
				g_oldSeekPref = *opt;
				*opt = 3;
			}
			// temp override of the repeat/loop state option
			if (GetSetRepeat(-1) == 1) {
				g_oldRepeatState = 1;
				GetSetRepeat(0);
			}

			cur->m_playReq = 1;
			g_isRunLoop = false;;
			g_playNextId = idx;
			g_playId = -1; // important for the 1st playlist item switch
			g_lastRunPos = g_nextRunPos;

			// go! (indirect UI update)
			SeekPlay(g_nextRunPos, false);
			g_playingPlaylist=true;
		}

	if (_errMsg && !g_playingPlaylist)
		MessageBox(g_pRgnPlaylistWnd?g_pRgnPlaylistWnd->GetHWND():GetMainHwnd(), 
			__LOCALIZE("Nothing to play!\n(empty playlist, removed regions, etc..)","sws_DLG_165"), 
			__LOCALIZE("S&M - Error","sws_DLG_165"), MB_OK);
}

void PlaylistPlay(COMMAND_T* _ct) {
	PlaylistPlay((int)_ct->user, false);
}

void PlaylistStopped()
{
	if (g_playingPlaylist)
	{
		MUTEX_PLAYLISTS;
		g_playingPlaylist = false;
		g_playId = -1;

		// restore options
		if (g_oldSeekPref >= 0)
			if (int* opt = (int*)GetConfigVar("smoothseek")) {
				*opt = g_oldSeekPref;
				g_oldSeekPref = -1;
			}
		if (g_oldStopprojlenPref >= 0)
			if (int* opt = (int*)GetConfigVar("stopprojlen")) {
				*opt = g_oldStopprojlenPref;
				g_oldStopprojlenPref = -1;
			}
		if (g_oldRepeatState >=0 ) {
			GetSetRepeat(g_oldRepeatState);
			g_oldRepeatState = -1;
		}

		if (g_pRgnPlaylistWnd)
			g_pRgnPlaylistWnd->Update();
	}
}


///////////////////////////////////////////////////////////////////////////////

// _mode: 0=crop current project, 1=crop to new project tab, 2=append to current project, 3=paste at cursor position
// note: moves/copies env points too, makes polled items, etc.. according to user prefs
void AppendPasteCropPlaylist(SNM_Playlist* _playlist, int _mode)
{
	MUTEX_PLAYLISTS;

	if (!_playlist || !_playlist->GetSize())
		return;

	bool updated = false;
	double prjlen=GetProjectLength(), startPos=prjlen, endPos=prjlen;

	// insert empty space?
	if (_mode==3)
	{
		startPos = endPos = GetCursorPositionEx(NULL);
		if (startPos<prjlen)
		{
			if (IsInPlaylist(_playlist, startPos)) {
				MessageBox(g_pRgnPlaylistWnd?g_pRgnPlaylistWnd->GetHWND():GetMainHwnd(), 
					__LOCALIZE("Aborted: pasting inside a region which is used in the playlist!","sws_DLG_165"),
					__LOCALIZE("S&M - Error","sws_DLG_165"), MB_OK);
				return;
			}
			if (IsInPlaylists(startPos)>=0) {
				if (IDNO == MessageBox(g_pRgnPlaylistWnd?g_pRgnPlaylistWnd->GetHWND():GetMainHwnd(), 
					__LOCALIZE("Warning: pasting inside a region which is used in other playlists!\nThis region will be larger and those playlists will change, are you sure you want to continue?","sws_DLG_165"),
					__LOCALIZE("S&M - Warning","sws_DLG_165"), MB_YESNO))
					return;
			}
			updated = true;
			Undo_BeginBlock2(NULL);
			InsertSilence(NULL, startPos, GetPlayListLength(_playlist));
		}
	}

//	OnStopButton();

	WDL_PtrList_DeleteOnDestroy<MarkerRegion> rgns;
	for (int i=0; i<_playlist->GetSize(); i++)
	{
		if (SNM_PlaylistItem* plItem = _playlist->Get(i))
		{
			int rgnIdx = GetMarkerRegionIndexFromId(plItem->m_rgnId);
			if (rgnIdx>=0)
			{
				int rgnnum, rgncol=0; double rgnpos, rgnend; char* rgnname;
				if (EnumProjectMarkers3(NULL, rgnIdx, NULL, &rgnpos, &rgnend, &rgnname, &rgnnum, &rgncol))
				{
					WDL_PtrList<void> itemsToKeep;
					if (GetItemsInInterval(&itemsToKeep, rgnpos, rgnend, false))
					{
						if (!updated) { // to do once (for undo stability)
							updated = true;
							Undo_BeginBlock2(NULL);
						}

						// store regions
						bool found = false;
						for (int k=0; !found && k<rgns.GetSize(); k++)
							found |= (rgns.Get(k)->GetNum() == rgnnum);
						if (!found)
							rgns.Add(new MarkerRegion(true, endPos-startPos, endPos+rgnend-rgnpos-startPos, rgnname, rgnnum, rgncol));

						// store data needed to "unsplit"
						WDL_PtrList_DeleteOnDestroy<SNM_ItemChunk> itemSates;
						for (int j=0; j < itemsToKeep.GetSize(); j++)
							itemSates.Add(new SNM_ItemChunk((MediaItem*)itemsToKeep.Get(j)));
						WDL_PtrList<void> splitItems;
						SplitSelectAllItemsInInterval(NULL, rgnpos, rgnend, &splitItems); // ok, split!

						// REAPER "bug": the last param of ApplyNudge() is ignored although
						// it is used in duplicate mode => use a loop instead
						// note: DupSelItems() is an override of ApplyNudge()
						for (int k=0; k<plItem->m_cnt; k++) {
							DupSelItems(NULL, endPos-rgnpos, &itemsToKeep);
							endPos += (rgnend-rgnpos);
						}

						// "unsplit" items
						for (int j=0; j < itemSates.GetSize(); j++)
							if (SNM_ItemChunk* ic = itemSates.Get(j)) {
									SNM_ChunkParserPatcher p(ic->m_item);
									p.SetChunk(ic->m_chunk.Get());
								}
						for (int j=0; j < splitItems.GetSize(); j++)
							if (MediaItem* item = (MediaItem*)splitItems.Get(j))
								if (itemsToKeep.Find(item) < 0)
									DeleteTrackMediaItem(GetMediaItem_Track(item), item);
					}
				}
			}
		}
	}

	if (!updated)
		return;

	///////////////////////////////////////////////////////////////////////////
	// append/paste to current project
	if (_mode == 2 || _mode == 3)
	{
		Main_OnCommand(40289, 0); // unselect all items
		SetEditCurPos(endPos, true, false);
		Undo_EndBlock2(NULL, _mode==2 ? __LOCALIZE("Append playlist to project","sws_undo") : __LOCALIZE("Paste playlist at edit cursor","sws_undo"), UNDO_STATE_ALL);
		return;
	}

	///////////////////////////////////////////////////////////////////////////
	// crop current project or crop to new project tab

	// remove markers/regions
	int x=0, lastx=0, num; bool isRgn;
	while ((x = EnumProjectMarkers2(NULL, x, &isRgn, NULL, NULL, NULL, &num))) {
		if (DeleteProjectMarker(NULL, num, isRgn)) x = lastx;
		lastx = x;
	}
	// restore updated regions in current project
	for (int i=0; i<rgns.GetSize(); i++)
		rgns.Get(i)->AddToProject();

	// dup the playlist (needed when cropping to new project)
	SNM_Playlist* dupPlaylist = _mode==1 ? new SNM_Playlist(_playlist) : NULL;

	for (int i=g_pls.Get()->GetSize()-1; i>=0; i--)
		for (int j=0; j<g_pls.Get()->Get(i)->GetSize(); j++)
			if (GetMarkerRegionIndexFromId(g_pls.Get()->Get(i)->Get(j)->m_rgnId) < 0) {
				g_pls.Get()->Delete(i, true);
				break;
			}

	int newIdx = g_pls.Get()->Find(_playlist);
	if (newIdx < 0) { // not used yet but would be needed when cropping to selected regions
		g_pls.Get()->Add(new SNM_Playlist(_playlist));
		newIdx = g_pls.Get()->GetSize()-1;
	}
	g_pls.Get()->m_cur = newIdx;

	// crop current project
	GetSet_LoopTimeRange(true, false, &startPos, &endPos, false);
	Main_OnCommand(40049, 0); // crop project to time sel
	Main_OnCommand(40289, 0); // unselect all items

	// restore updated regions (in new project)
	for (int i=0; i<rgns.GetSize(); i++)
		rgns.Get(i)->AddToProject();

	if (!_mode)
	{
		// clear time sel + edit cursor position
		GetSet_LoopTimeRange(true, false, &g_d0, &g_d0, false);
		SetEditCurPos(0.0, true, false);
		Undo_EndBlock2(NULL, __LOCALIZE("Crop project to playlist","sws_undo"), UNDO_STATE_ALL);
		if (g_pRgnPlaylistWnd) {
			g_pRgnPlaylistWnd->FillPlaylistCombo();
			g_pRgnPlaylistWnd->Update();
		}
		return;
	}

	///////////////////////////////////////////////////////////////////////////
	// crop in new project tab
	// macro-ish solution because copying the project the proper way (e.g. 
	// copying all track states) is totally unstable
	// note: the project is "copied" w/o extension data, project bay, etc..

	// store master track
	WDL_FastString mstrStates;
	if (MediaTrack* tr = CSurf_TrackFromID(0, false)) {
		SNM_ChunkParserPatcher p(tr);
		mstrStates.Set(p.GetChunk());
	}

	Main_OnCommand(40296, 0); // select all tracks
	Main_OnCommand(40210, 0); // copy tracks

	// trick!
	Undo_EndBlock2(NULL, __LOCALIZE("Crop project to playlist","sws_undo"), UNDO_STATE_ALL);
	Undo_DoUndo2(NULL);

	// force a dummy undo point so that no undo point is added in the new project (!) ------------>
	Undo_BeginBlock2(NULL);
	Main_OnCommand(40859, 0); // new project tab
	Main_OnCommand(40058, 0); // paste item/tracks
	Main_OnCommand(40297, 0); // unselect all tracks
	if (OpenClipboard(g_hwndParent)) { // clear clipboard
	    EmptyClipboard();
		CloseClipboard();
	}
	if (MediaTrack* tr = CSurf_TrackFromID(0, false)) {
		SNM_ChunkParserPatcher p(tr);
		p.SetChunk(mstrStates.Get());
	}
	// restore regions (in new project, new tab)
	for (int i=0; i<rgns.GetSize(); i++)
		rgns.Get(i)->AddToProject();

	// new project: the playlist is empty at this point
	g_pls.Get()->Add(dupPlaylist);
	g_pls.Get()->m_cur = 0;
	Undo_EndBlock2(NULL, "Fake undo", UNDO_STATE_ALL); // <----------------------------------------

	if (g_pRgnPlaylistWnd) {
		g_pRgnPlaylistWnd->FillPlaylistCombo();
		g_pRgnPlaylistWnd->Update();
	}
}

void AppendPasteCropPlaylist(COMMAND_T* _ct) {
	AppendPasteCropPlaylist(GetPlaylist(), (int)_ct->user);
}


///////////////////////////////////////////////////////////////////////////////

void SNM_Playlist_UpdateJob::Perform()
{
	if (g_pRgnPlaylistWnd) {
		g_pRgnPlaylistWnd->FillPlaylistCombo();
		g_pRgnPlaylistWnd->Update();
	}
}


///////////////////////////////////////////////////////////////////////////////

// ScheduledJob used because of possible multi-notifs during project switch (vs CSurfSetTrackListChange)
void SNM_Playlist_MarkerRegionSubscriber::NotifyMarkerRegionUpdate(int _updateFlags) {
	AddOrReplaceScheduledJob(new SNM_Playlist_UpdateJob());
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
		if (SNM_Playlist* playlist = g_pls.Get()->Add(new SNM_Playlist(lp.gettoken_str(1))))
		{
			int success;
			if (lp.gettoken_int(2, &success) && success)
				g_pls.Get()->m_cur = g_pls.Get()->GetSize()-1;

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
	for (int j=0; j < g_pls.Get()->GetSize(); j++)
	{
		WDL_FastString confStr("<S&M_RGN_PLAYLIST "), escStr;
		makeEscapedConfigString(GetPlaylist(j)->m_name.Get(), &escStr);
		confStr.Append(escStr.Get());
		if (j == g_pls.Get()->m_cur)
			confStr.Append(" 1\n");
		else
			confStr.Append("\n");
		int iHeaderLen = confStr.GetLength();

		for (int i=0; i < GetPlaylist(j)->GetSize(); i++)
			if (SNM_PlaylistItem* item = GetPlaylist(j)->Get(i))
				confStr.AppendFormatted(128,"%d %d\n", item->m_rgnId, item->m_cnt);

		// ignore empty playlists when saving but always take them into account for undo
		if (isUndo || (!isUndo && confStr.GetLength() > iHeaderLen)) {
			confStr.Append(">\n");
			StringToExtensionConfig(&confStr, ctx);
		}
	}
}

static void BeginLoadProjectState(bool isUndo, struct project_config_extension_t *reg)
{
	MUTEX_PLAYLISTS;
	g_pls.Cleanup();
	g_pls.Get()->Empty(true);
	g_pls.Get()->m_cur=0;
}

static project_config_extension_t g_projectconfig = {
	ProcessExtensionLine, SaveExtensionConfig, BeginLoadProjectState, NULL
};


///////////////////////////////////////////////////////////////////////////////

int RegionPlaylistInit()
{
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
