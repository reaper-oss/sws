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
#define TGL_INFINITE_LOOP_MSG	0xF00D
#define ADD_REGION_START_MSG	0xF100
#define ADD_REGION_END_MSG		0xF7FF // => 1791 marker/region *indexes* supported, oh well..
#define INSERT_REGION_START_MSG	0xF800
#define INSERT_REGION_END_MSG	0xFEFF // => 1791 marker/region indexes supported

#define UNDO_PLAYLIST_STR		__LOCALIZE("Region Playlist edition", "sws_undo")


enum {
  BUTTONID_LOCK=2000, //JFB would be great to have _APS_NEXT_CONTROL_VALUE *always* defined
  BUTTONID_PLAY,
  BUTTONID_STOP,
  BUTTONID_REPEAT,
  TXTID_PLAYLIST,
  COMBOID_PLAYLIST,
  CONTAINER_ADD_DEL,
  BUTTONID_NEW_PLAYLIST,
  BUTTONID_DEL_PLAYLIST,
  TXTID_LENGTH,
  BUTTONID_PASTE,
  TXTID_MONITOR_PL,
  TXTID_MONITOR_CUR,
  TXTID_MONITOR_NEXT
};

enum {
  COL_RGN=0,
  COL_RGN_COUNT,
  COL_COUNT
};


SNM_RegionPlaylistWnd* g_pRgnPlaylistWnd = NULL;
SWSProjConfig<SNM_Playlists> g_pls;
SWS_Mutex g_plsMutex;
char g_rgnplBigFontName[64] = SWSDLG_TYPEFACE;

bool g_monitorMode = true;

bool g_repeatPlaylist = false;
int g_playPlaylist = -1; // -1: stopped, playlist id otherwise
int g_playItem = -1; // index of the playlist item being played
int g_playNext = -1; // index of the next playlist item to be played

// needed to make PlaylistRun() as idle as possible..
bool g_isRunLoop = false;
double g_lastRunPos = -1.0;
double g_nextRunPos;
double g_nextRunEnd;

int g_oldSeekPref = -1;
int g_oldStopprojlenPref = -1;
int g_oldRepeatState = -1;


SNM_Playlist* GetPlaylist(int _id = -1) //-1 for the current (i.e. displayed) playlist
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
				if ((i-1)>=0 && pl->Get(i-1) && item->m_rgnId == pl->Get(i-1)->m_rgnId)
				{
					bool infinite = (pl->Get(i-1)->m_cnt<0 || item->m_cnt<0);
					pl->Get(i-1)->m_cnt = abs(pl->Get(i-1)->m_cnt) + abs(item->m_cnt);
					if (infinite)
						pl->Get(i-1)->m_cnt *= (-1);
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
			case COL_RGN:
			{
				char buf[128]="";
				if (pItem->m_rgnId>0 && EnumMarkerRegionDesc(GetMarkerRegionIndexFromId(pItem->m_rgnId), buf, 128, SNM_REGION_MASK, true) && *buf)
				{
					SNM_Playlist* curpl = GetPlaylist();
					_snprintfSafe(str, iStrMax, "%s %s", 
						curpl && g_playPlaylist>=0 && curpl==GetPlaylist(g_playPlaylist) ? // current playlist being played?
						(curpl->Get(g_playItem)==pItem ? UTF8_BULLET : (curpl->Get(g_playNext)==pItem ? UTF8_CIRCLE : " ")) : " ", buf);
					break;
				}
				int num = pItem->m_rgnId & 0x3FFFFFFF;
				_snprintfSafe(str, iStrMax, __LOCALIZE("Unknown region %d","sws_DLG_165"), num);
				break;
			}
			case COL_RGN_COUNT:
				if (pItem->m_cnt < 0)
					_snprintfSafe(str, iStrMax, "%s", UTF8_INFINITY);
				else
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

	m_vwnd_painter.SetGSC(WDL_STYLE_GetSysColor);
	m_parentVwnd.SetRealParent(m_hwnd);
	
	m_btnLock.SetID(BUTTONID_LOCK);
	m_parentVwnd.AddChild(&m_btnLock);

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

	m_monPl.SetID(TXTID_MONITOR_PL);
	m_monPl.SetFontName(SWSDLG_TYPEFACE); // no customization for playlist #
	m_monPl.SetFlags(DT_LEFT);
	m_parentVwnd.AddChild(&m_monPl);

	m_monCur.SetID(TXTID_MONITOR_CUR);
	m_monCur.SetTitle(__LOCALIZE("CURRENT","sws_DLG_165"));
	m_monCur.SetFontName(g_rgnplBigFontName);
	m_monCur.SetBorder(true);
	m_parentVwnd.AddChild(&m_monCur);

	m_monNext.SetID(TXTID_MONITOR_NEXT);
	m_monNext.SetTitle(__LOCALIZE("NEXT","sws_DLG_165"));
	m_monNext.SetFontName(g_rgnplBigFontName);
	m_monNext.SetBorder(true);
	m_parentVwnd.AddChild(&m_monNext);

	Update();

	RegisterToMarkerRegionUpdates(&m_mkrRgnSubscriber);
}

void SNM_RegionPlaylistWnd::OnDestroy() {
	UnregisterToMarkerRegionUpdates(&m_mkrRgnSubscriber);
	m_cbPlaylist.Empty();
}

// ScheduledJob used because of multi-notifs
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

	ShowWindow(GetDlgItem(m_hwnd, IDC_LIST), !g_monitorMode && g_pls.Get()->GetSize() ? SW_SHOW : SW_HIDE);

	if (_flags&1)
		if (m_pLists.GetSize())
			((SNM_PlaylistView*)m_pLists.Get(0))->UpdateCompact();

	if (_flags&2)
	{
		UpdateMonitoring();
		m_parentVwnd.RequestRedraw(NULL);
	}

	// "fast" update (used while playing) - exclusive with the above!
	if (_flags&4)
	{
		// monitoring mode
		if (g_monitorMode)
			UpdateMonitoring();
		// edition mode
		else if (m_pLists.GetSize())
			((SNM_PlaylistView*)m_pLists.Get(0))->Update(); // no playlist compacting
	}

	bRecurseCheck = false;
}

// just update monitoring VWnds
void SNM_RegionPlaylistWnd::UpdateMonitoring()
{
	bool loop = false;
	char bufPl[128]="";
	char bufCur[128]="";
	char bufNext[128]="";
	if (g_playPlaylist>=0)
	{
		if (SNM_Playlist* curpl = GetPlaylist(g_playPlaylist))
		{
			_snprintfSafe(bufPl, sizeof(bufPl), "#%d - %s", g_playPlaylist+1, curpl->m_name.Get());

			if (SNM_PlaylistItem* pItem = curpl->Get(g_playItem)) {
				EnumMarkerRegionDesc(GetMarkerRegionIndexFromId(pItem->m_rgnId), bufCur, sizeof(bufCur), SNM_REGION_MASK, true, false);
			}
			// current item not foud (e.g. g_playItem = -1) => best effort: find region by play pos
			else
			{
				int id, idx = FindMarkerRegion(GetPlayPosition(), SNM_REGION_MASK, &id);
				if (id > 0)
					EnumMarkerRegionDesc(idx, bufCur, sizeof(bufCur), SNM_REGION_MASK, true, false);
				else
					strcpy(bufCur, "?");
			}

			if (g_playItem>=0 && g_playItem==g_playNext) // tiny optimization
				loop = true;
			else if (SNM_PlaylistItem* pItem = curpl->Get(g_playNext))
				EnumMarkerRegionDesc(GetMarkerRegionIndexFromId(pItem->m_rgnId), bufNext, sizeof(bufNext), SNM_REGION_MASK, true, false);
		}
	}
	m_monPl.SetText(bufPl);
	m_monCur.SetText(bufCur);
	m_monNext.SetText(loop ? __LOCALIZE("(loop)","sws_DLG_165") : bufNext, 153);
}

void SNM_RegionPlaylistWnd::FillPlaylistCombo()
{
	MUTEX_PLAYLISTS;
	m_cbPlaylist.Empty();
	for (int i=0; i < g_pls.Get()->GetSize(); i++) {
		char name[128]="";
		_snprintfSafe(name, sizeof(name), "%d - %s", i+1, g_pls.Get()->Get(i)->m_name.Get());
		m_cbPlaylist.AddItem(name);
	}
	m_cbPlaylist.SetCurSel(g_pls.Get()->m_cur);
}

void SNM_RegionPlaylistWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	MUTEX_PLAYLISTS;
	switch(LOWORD(wParam))
	{
		case BUTTONID_LOCK:
			if (!HIWORD(wParam))
				ToggleLock();
			break;
		case COMBOID_PLAYLIST:
			if (HIWORD(wParam)==CBN_SELCHANGE)
			{
				// stop cell editing (changing the list content would be ignored otherwise,
				// leading to unsynchronized dropdown box vs list view)
				m_pLists.Get(0)->EditListItemEnd(false);
				g_pls.Get()->m_cur = m_cbPlaylist.GetCurSel();
				Update();
				Undo_OnStateChangeEx(UNDO_PLAYLIST_STR, UNDO_STATE_MISCCFG, -1);
			}
			break;
		case NEW_PLAYLIST_MSG:
		case BUTTONID_NEW_PLAYLIST:
		case COPY_PLAYLIST_MSG:
		{
			char name[64]="";
			lstrcpyn(name, __LOCALIZE("Untitled","sws_DLG_165"), 64);
			if (PromptUserForString(m_hwnd, __LOCALIZE("S&M - Add playlist","sws_DLG_165"), name, 64, true) && *name)
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
				int reply = IDYES;
				if (GetPlaylist()->GetSize()) // do not ask if empty
				{
					char msg[128] = "";
					_snprintfSafe(msg, sizeof(msg), __LOCALIZE_VERFMT("Are you sure you want to delete the playlist #%d \"%s\"?","sws_DLG_165"), g_pls.Get()->m_cur+1, GetPlaylist()->m_name.Get());
					reply = MessageBox(m_hwnd, msg, __LOCALIZE("S&M - Delete playlist","sws_DLG_165"), MB_YESNO);
				}
				if (reply != IDNO)
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
				if (PromptUserForString(m_hwnd, __LOCALIZE("S&M - Rename playlist","sws_DLG_165"), newName, 64, true) && *newName)
				{
					GetPlaylist()->m_name.Set(newName);
					FillPlaylistCombo();
					Update();
					Undo_OnStateChangeEx(UNDO_PLAYLIST_STR, UNDO_STATE_MISCCFG, -1); 
				}
			}
			break;
		case BUTTONID_PLAY:
			PlaylistPlay(g_pls.Get()->m_cur, GetNextValidItem(g_pls.Get()->m_cur, 0, true, true));
			break;
		case BUTTONID_STOP:
			OnStopButton();
			break;
		case BUTTONID_REPEAT:
			SetPlaylistRepeat(NULL);
			break;
		case BUTTONID_PASTE:
			RECT r; m_btnCrop.GetPositionInTopVWnd(&r);
			ClientToScreen(m_hwnd, (LPPOINT)&r);
			ClientToScreen(m_hwnd, ((LPPOINT)&r)+1);
			SendMessage(m_hwnd, WM_CONTEXTMENU, 0, MAKELPARAM((UINT)(r.left), (UINT)(r.bottom+SNM_1PIXEL_Y)));
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
		case PASTE_CURSOR_MSG:
			AppendPasteCropPlaylist(GetPlaylist(), 3);
			break;
		case DELETE_MSG:
		{
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
		case TGL_INFINITE_LOOP_MSG:
		{
			int x=0; bool updt = false;
			while(SNM_PlaylistItem* item = (SNM_PlaylistItem*)m_pLists.Get(0)->EnumSelected(&x)) {
				item->m_cnt *= (-1);
				updt = true;
			}
			if (updt) {
				Undo_OnStateChangeEx(UNDO_PLAYLIST_STR, UNDO_STATE_MISCCFG, -1); 
				Update();
			}
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
			if (GetPlaylist())
			{
				int x=0;
				if (SNM_PlaylistItem* pItem = (SNM_PlaylistItem*)m_pLists.Get(0)->EnumSelected(&x))
					PlaylistPlay(g_pls.Get()->m_cur, GetPlaylist()->Find(pItem));
			}
			break;
		case ADD_ALL_REGIONS_MSG:
		{
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

//#define RGNPL_RIGHT_TRANSPORT //JFB!!! cleanup

void SNM_RegionPlaylistWnd::DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight)
{
	MUTEX_PLAYLISTS;

	LICE_CachedFont* font = SNM_GetThemeFont();
	IconTheme* it = SNM_GetIconTheme();
	int x0=_r->left+8, h=SNM_TOP_GUI_HEIGHT;
	if (_tooltipHeight)
		*_tooltipHeight = h;
	
	bool hasPlaylists = (g_pls.Get()->GetSize()>0);

	SNM_SkinButton(&m_btnLock, it ? &it->toolbar_lock[!g_monitorMode] : NULL, g_monitorMode ? __LOCALIZE("Edition mode","sws_DLG_165") : __LOCALIZE("Monitoring mode","sws_DLG_165"));
	if (SNM_AutoVWndPosition(DT_LEFT, &m_btnLock, NULL, _r, &x0, _r->top, h))
	{
#ifndef RGNPL_RIGHT_TRANSPORT
		SNM_SkinButton(&m_btnPlay, it ? &it->gen_play[g_playPlaylist>=0?1:0] : NULL, __LOCALIZE("Play","sws_DLG_165"));
		if (SNM_AutoVWndPosition(DT_LEFT, &m_btnPlay, NULL, _r, &x0, _r->top, h, 0))
		{
			SNM_SkinButton(&m_btnStop, it ? &(it->gen_stop) : NULL, __LOCALIZE("Stop","sws_DLG_165"));
			if (SNM_AutoVWndPosition(DT_LEFT, &m_btnStop, NULL, _r, &x0, _r->top, h, 0))
			{
				SNM_SkinButton(&m_btnRepeat, it ? &it->gen_repeat[g_repeatPlaylist?1:0] : NULL, __LOCALIZE("Repeat","sws_DLG_165"));
				if (SNM_AutoVWndPosition(DT_LEFT, &m_btnRepeat, NULL, _r, &x0, _r->top, h))
#endif
				{
					// *** monitoring ***
					if (g_monitorMode)
					{
						RECT r = *_r;

						// playlist name
						r.left = x0;
						r.bottom = h;
#ifndef RGNPL_RIGHT_TRANSPORT
						LICE_IBitmap* logo = SNM_GetThemeLogo();
//						r.right -= logo ? logo->getWidth()+5 : 5; // 5: margin
#else
						r.right -= // TODO: play+stop+repeat widths (can be different depending on themes..)
#endif
						m_monPl.SetPosition(&r);
						m_monPl.SetVisible(true);

						r = *_r;

						// margins
						r.top += h;
						r.left += 8;
						r.right -= 10;
						r.bottom -= 8;

						int monHeight = r.bottom - r.top;
						int monWidth = r.right - r.left;

						// vertical display
						if (2*monHeight > monWidth)
						{
							r.bottom = r.top + int(0.5*monHeight + 0.5)-4; // 4 magin between cur/next cells
							m_monCur.SetPosition(&r);
							r.top = r.bottom+2*4;
							r.bottom = r.top + int(0.5*monHeight + 0.5)-4;
							m_monNext.SetPosition(&r);
						}
						// horizontal display
						else
						{
							r.right = r.left + int(0.5*monWidth + 0.5)-4; // 4 magin between cur/next cells
							m_monCur.SetPosition(&r);
							r.left = r.right+2*4;
							r.right = r.left + int(0.5*monWidth + 0.5)-4;
							m_monNext.SetPosition(&r);
						}
						m_monCur.SetVisible(true);
						m_monNext.SetVisible(true);
#ifndef RGNPL_RIGHT_TRANSPORT
						if (_r->right - _r->left > 400) // lazy solution here..
							SNM_AddLogo(_bm, _r, _r->right-10-logo->getWidth(), h);
#endif
					}

					//*** edition ***
					else
					{
						m_txtPlaylist.SetFont(font);
						m_txtPlaylist.SetText(hasPlaylists ? __LOCALIZE("Playlist #","sws_DLG_165") : __LOCALIZE("Playlist: None","sws_DLG_165"));
						if (SNM_AutoVWndPosition(DT_LEFT, &m_txtPlaylist, NULL, _r, &x0, _r->top, h, hasPlaylists?4:SNM_DEF_VWND_X_STEP))
						{
							m_cbPlaylist.SetFont(font);
							if (!hasPlaylists || (hasPlaylists && SNM_AutoVWndPosition(DT_LEFT, &m_cbPlaylist, &m_txtPlaylist, _r, &x0, _r->top, h, 4)))
							{
								((SNM_AddDelButton*)m_parentVwnd.GetChildByID(BUTTONID_DEL_PLAYLIST))->SetEnabled(hasPlaylists);
								if (SNM_AutoVWndPosition(DT_LEFT, &m_btnsAddDel, NULL, _r, &x0, _r->top, h))
								{
									double plLen = hasPlaylists ? GetPlayListLength(GetPlaylist()) : 0.0;
									if (abs(plLen)>0.0) // <0.0 means infinite
									{
										SNM_SkinToolbarButton(&m_btnCrop, __LOCALIZE("Crop/paste","sws_DLG_165"));
										if (SNM_AutoVWndPosition(DT_LEFT, &m_btnCrop, NULL, _r, &x0, _r->top, h))
										{
											char lenStr[64]="", timeStr[32];
											if (plLen >= 0.0)
												format_timestr_pos(plLen, timeStr, 32, -1);
											else
												lstrcpyn(timeStr, __LOCALIZE("infinite","sws_DLG_165"), sizeof(timeStr));
											m_txtLength.SetFont(font);
											_snprintfSafe(lenStr, sizeof(lenStr), __LOCALIZE_VERFMT("Length: %s","sws_DLG_165"), timeStr);
											m_txtLength.SetText(lenStr);

#ifndef RGNPL_RIGHT_TRANSPORT
											if (SNM_AutoVWndPosition(DT_LEFT, &m_txtLength, NULL, _r, &x0, _r->top, h))
												SNM_AddLogo(_bm, _r, x0, h);
#else
											SNM_AutoVWndPosition(DT_LEFT, &m_txtLength, NULL, _r, &x0, _r->top, h);
#endif

										}
									}
#ifndef RGNPL_RIGHT_TRANSPORT
									else
										SNM_AddLogo(_bm, _r, x0, h);
#endif
								}
							}
						}
					}
				}
#ifndef RGNPL_RIGHT_TRANSPORT
			}
		}
#endif
	}

#ifdef RGNPL_RIGHT_TRANSPORT
	x0 = _r->right-10;
	SNM_SkinButton(&m_btnRepeat, it ? &it->gen_repeat[g_repeatPlaylist?1:0] : NULL, __LOCALIZE("Repeat","sws_DLG_165"));
	if (SNM_AutoVWndPosition(DT_RIGHT, &m_btnRepeat, NULL, _r, &x0, _r->top, h, 0))
	{
		SNM_SkinButton(&m_btnStop, it ? &(it->gen_stop) : NULL, __LOCALIZE("Stop","sws_DLG_165"));
		if (SNM_AutoVWndPosition(DT_RIGHT, &m_btnStop, NULL, _r, &x0, _r->top, h, 0))
		{
			SNM_SkinButton(&m_btnPlay, it ? &it->gen_play[g_playPlaylist>=0?1:0] : NULL, __LOCALIZE("Play","sws_DLG_165"));
			SNM_AutoVWndPosition(DT_RIGHT, &m_btnPlay, NULL, _r, &x0, _r->top, h, 0);
		}
	}
#endif
}

void SNM_RegionPlaylistWnd::AddPasteContextMenu(HMENU _menu)
{
	if (GetMenuItemCount(_menu))
		AddToMenu(_menu, SWS_SEPARATOR, 0);
	AddToMenu(_menu, __LOCALIZE("Crop project to playlist","sws_DLG_165"), CROP_PRJ_MSG, -1, false, GetPlaylist() && GetPlaylist()->GetSize() ? 0 : MF_GRAYED);
	AddToMenu(_menu, __LOCALIZE("Crop project to playlist (new project tab)","sws_DLG_165"), CROP_PRJTAB_MSG, -1, false, GetPlaylist() && GetPlaylist()->GetSize() ? 0 : MF_GRAYED);
	AddToMenu(_menu, __LOCALIZE("Append playlist to project","sws_DLG_165"), APPEND_PRJ_MSG, -1, false, GetPlaylist() && GetPlaylist()->GetSize() ? 0 : MF_GRAYED);
	AddToMenu(_menu, __LOCALIZE("Paste playlist at edit cursor","sws_DLG_165"), PASTE_CURSOR_MSG, -1, false, GetPlaylist() && GetPlaylist()->GetSize() ? 0 : MF_GRAYED);

	int x=0; bool hasSel = m_pLists.Get(0)->EnumSelected(&x) != NULL;
	AddToMenu(_menu, SWS_SEPARATOR, 0);
	AddToMenu(_menu, __LOCALIZE("Append selected regions to project","sws_DLG_165"), APPEND_SEL_RGN_MSG, -1, false, hasSel ? 0 : MF_GRAYED);
	AddToMenu(_menu, __LOCALIZE("Paste selected regions at edit cursor","sws_DLG_165"), PASTE_SEL_RGN_MSG, -1, false, hasSel ? 0 : MF_GRAYED);
}

HMENU SNM_RegionPlaylistWnd::OnContextMenu(int _x, int _y, bool* _wantDefaultItems)
{
	if (g_monitorMode)
		return NULL;

	MUTEX_PLAYLISTS;

	HMENU hMenu = CreatePopupMenu();

	// specific context menu for the paste button
	POINT p; GetCursorPos(&p);
	ScreenToClient(m_hwnd,&p);
	if (WDL_VWnd* v = m_parentVwnd.VirtWndFromPoint(p.x,p.y,1))
	{
		switch (v->GetID())
		{
			case BUTTONID_PASTE:
				*_wantDefaultItems = false;
				AddPasteContextMenu(hMenu);
				return hMenu;
		}
	}

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

		AddToMenu(hMenu, SWS_SEPARATOR, 0);

		HMENU hCropPasteSubMenu = CreatePopupMenu();
		AddSubMenu(hMenu, hCropPasteSubMenu, __LOCALIZE("Crop/paste","sws_DLG_165"));
		AddPasteContextMenu(hCropPasteSubMenu);
	}

	if (GetPlaylist())
	{
		if (GetMenuItemCount(hMenu))
			AddToMenu(hMenu, SWS_SEPARATOR, 0);

		AddToMenu(hMenu, __LOCALIZE("Add all regions","sws_DLG_165"), ADD_ALL_REGIONS_MSG);
		HMENU hAddSubMenu = CreatePopupMenu();
		AddSubMenu(hMenu, hAddSubMenu, __LOCALIZE("Add region","sws_DLG_165"));
		FillMarkerRegionMenu(hAddSubMenu, ADD_REGION_START_MSG, SNM_REGION_MASK);

		if (!*_wantDefaultItems) 
		{
			HMENU hInsertSubMenu = CreatePopupMenu();
			AddSubMenu(hMenu, hInsertSubMenu, __LOCALIZE("Insert region","sws_DLG_165"), -1, hasSel ? 0 : MF_GRAYED);
			FillMarkerRegionMenu(hInsertSubMenu, INSERT_REGION_START_MSG, SNM_REGION_MASK);
			AddToMenu(hMenu, __LOCALIZE("Remove regions","sws_DLG_165"), DELETE_MSG, -1, false, hasSel ? 0 : MF_GRAYED);
			AddToMenu(hMenu, SWS_SEPARATOR, 0);
			AddToMenu(hMenu, __LOCALIZE("Toggle infinite loop","sws_DLG_165"), TGL_INFINITE_LOOP_MSG, -1, false, hasSel ? 0 : MF_GRAYED);
			AddToMenu(hMenu, SWS_SEPARATOR, 0);
			AddToMenu(hMenu, __LOCALIZE("Append selected regions to project","sws_DLG_165"), APPEND_SEL_RGN_MSG, -1, false, hasSel ? 0 : MF_GRAYED);
			AddToMenu(hMenu, __LOCALIZE("Paste selected regions at edit cursor","sws_DLG_165"), PASTE_SEL_RGN_MSG, -1, false, hasSel ? 0 : MF_GRAYED);
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
			
			case BUTTONID_LOCK:
				lstrcpyn(_bufOut, __LOCALIZE("Toggle monitoring/edition mode","sws_DLG_165"), _bufOutSz);
				return true;
			case BUTTONID_PLAY:
				if (g_playPlaylist>=0)
					_snprintfSafe(_bufOut, _bufOutSz, __LOCALIZE_VERFMT("Playing playlist #%d","sws_DLG_165"), g_playPlaylist+1);
				else
					lstrcpyn(_bufOut, __LOCALIZE("Play preview","sws_DLG_165"), _bufOutSz);
				return true;
			case BUTTONID_STOP:
				lstrcpyn(_bufOut, __LOCALIZE("Stop","sws_DLG_165"), _bufOutSz);
				return true;
			case BUTTONID_REPEAT:
				lstrcpyn(_bufOut, __LOCALIZE("Toggle repeat","sws_DLG_165"), _bufOutSz);
				return true;
			case COMBOID_PLAYLIST:
				lstrcpyn(_bufOut, __LOCALIZE("Current playlist","sws_DLG_165"), _bufOutSz);
				return true;
			case BUTTONID_NEW_PLAYLIST:
				lstrcpyn(_bufOut, __LOCALIZE("Add playlist","sws_DLG_165"), _bufOutSz);
				return true;
			case BUTTONID_DEL_PLAYLIST:
				lstrcpyn(_bufOut, __LOCALIZE("Delete playlist","sws_DLG_165"), _bufOutSz);
				return true;
			case TXTID_LENGTH:
				lstrcpyn(_bufOut, __LOCALIZE("Total playlist length","sws_DLG_165"), _bufOutSz);
				return true;
			case BUTTONID_PASTE:
				lstrcpyn(_bufOut, __LOCALIZE("Crop/paste","sws_DLG_165"), _bufOutSz);
				return true;
		}
	}
	return false;
}

void SNM_RegionPlaylistWnd::ToggleLock()
{
	g_monitorMode = !g_monitorMode;
	RefreshToolbar(NamedCommandLookup("_S&M_TGL_RGN_PLAYLIST_MODE"));
	Update();
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

int HasNestedMarkersRegions(int _playlistId)
{
	MUTEX_PLAYLISTS;
	SNM_Playlist* pl = GetPlaylist(_playlistId);
	for (int i=0; pl && i<pl->GetSize(); i++)
	{
		if (SNM_PlaylistItem* plItem = pl->Get(i))
		{
			double rgnpos, rgnend;
			int num, rgnidx = GetMarkerRegionIndexFromId(plItem->m_rgnId);
			if (rgnidx>0 && EnumProjectMarkers2(NULL, rgnidx, NULL, &rgnpos, &rgnend, NULL, &num))
			{
				int x=0, lastx=0; double dPos, dEnd; bool isRgn;
				while (x = EnumProjectMarkers2(NULL, x, &isRgn, &dPos, &dEnd, NULL, NULL))
				{
					if (rgnidx != lastx)
					{
						if (isRgn) {
							if ((dPos > rgnpos && dPos < rgnend) || (dEnd > rgnpos && dEnd < rgnend))
								return num;
						}
						else {
							if (dPos > rgnpos && dPos < rgnend)
								return num;
						}
					}
					lastx=x;
				}
			}
		}
	}
	return -1;
}

int IsInfinite(SNM_Playlist* _playlist)
{
	MUTEX_PLAYLISTS;
	int num = -1;
	for (int i=0; _playlist && i<_playlist->GetSize(); i++)
		if (SNM_PlaylistItem* plItem = _playlist->Get(i))
			if (plItem->m_rgnId>0 && EnumProjectMarkers2(NULL, GetMarkerRegionIndexFromId(plItem->m_rgnId), NULL, NULL, NULL, NULL, &num))
				if (plItem->m_cnt<0)
					return num;
	return -1;
}

double GetPlayListLength(SNM_Playlist* _playlist)
{
	bool infinite = false;
	double length=0.0;
	for (int i=0; _playlist && i<_playlist->GetSize(); i++)
	{
		if (SNM_PlaylistItem* plItem = _playlist->Get(i))
		{
			double rgnpos, rgnend;
			if (plItem->m_rgnId>0 && EnumProjectMarkers2(NULL, GetMarkerRegionIndexFromId(plItem->m_rgnId), NULL, &rgnpos, &rgnend, NULL, NULL))
			{
				infinite |= plItem->m_cnt<0;
				length += ((rgnend-rgnpos) * abs(plItem->m_cnt));
			}
		}
	}
	return infinite ? length*(-1) : length;
}


///////////////////////////////////////////////////////////////////////////////
// Polling on play: PlaylistRun() and related funcs
///////////////////////////////////////////////////////////////////////////////

// never use things like GetPlaylist()->Get(i+1) but this func!
int GetNextValidItem(int _playlistId, int _itemId, bool _ignoreLoops, bool _startWith)
{
	if (_playlistId>=0 && _itemId>=0)
	{
		MUTEX_PLAYLISTS;
		if (SNM_Playlist* pl = GetPlaylist(_playlistId))
		{
			for (int i=_itemId+(_startWith?0:1); i<pl->GetSize(); i++)
				if (SNM_PlaylistItem* item = pl->Get(i))
					if (item->m_rgnId>0 && (_ignoreLoops || (!_ignoreLoops && (item->m_cnt<0 || (item->m_cnt>=1 && item->m_playReq<item->m_cnt)))) && GetMarkerRegionIndexFromId(item->m_rgnId)>=0)
						return i;
			// no issue if _playlistIdx==0, it will not match either..
			for (int i=0; i<pl->GetSize() && i<(_itemId+(_startWith?1:0)); i++)
				if (SNM_PlaylistItem* item = pl->Get(i))
					if (item->m_rgnId>0 && (_ignoreLoops || (!_ignoreLoops && (item->m_cnt<0 || (item->m_cnt>=1 && item->m_playReq<item->m_cnt)))) && GetMarkerRegionIndexFromId(item->m_rgnId)>=0)
						return i;
		}
	}
	return -1;
}

// never use things like GetPlaylist()->Get(i-1) but this func!
int GetPrevValidItem(int _playlistId, int _itemId, bool _ignoreLoops, bool _startWith)
{
	if (_playlistId>=0 && _itemId>=0)
	{
		MUTEX_PLAYLISTS;
		if (SNM_Playlist* pl = GetPlaylist(_playlistId))
		{
			for (int i=_itemId-(_startWith?0:1); i>=0; i--)
				if (SNM_PlaylistItem* item = pl->Get(i))
					if (item->m_rgnId>0 && (_ignoreLoops || (!_ignoreLoops && (item->m_cnt<0 || (item->m_cnt>=1 && item->m_playReq<item->m_cnt)))) && GetMarkerRegionIndexFromId(item->m_rgnId)>=0)
						return i;
			// no issue if _playlistIdx==0, it will not match either..
			for (int i=pl->GetSize()-1; i>=0 && i>(_itemId-(_startWith?1:0)); i--)
				if (SNM_PlaylistItem* item = pl->Get(i))
					if (item->m_rgnId>0 && (_ignoreLoops || (!_ignoreLoops && (item->m_cnt<0 || (item->m_cnt>=1 && item->m_playReq<item->m_cnt)))) && GetMarkerRegionIndexFromId(item->m_rgnId)>=0)
						return i;
		}
	}
	return -1;
}

void PlaylistRun()
{
	MUTEX_PLAYLISTS;

	static bool bRecurseCheck = false;
	if (bRecurseCheck || g_playPlaylist<0)
		return;
	bRecurseCheck = true;

	double pos = GetPlayPosition2();
	bool updateUI = false;
	if (SNM_Playlist* pl = GetPlaylist(g_playPlaylist))
	{
		if (g_isRunLoop && pos<g_lastRunPos)
			g_isRunLoop = false;

		if (!g_isRunLoop && pos>=g_nextRunPos && pos<=g_nextRunEnd)
		{
			g_playItem = g_playNext;
			updateUI = (g_monitorMode || pl==GetPlaylist()); // update only if it is the displayed playlist

			SNM_PlaylistItem* cur = pl->Get(g_playItem);
			g_isRunLoop = (cur && (cur->m_cnt<0 || (cur->m_cnt>1 && cur->m_playReq<cur->m_cnt))); // region loop?
			if (!g_isRunLoop)
				for (int i=0; i<pl->GetSize(); i++)
					pl->Get(i)->m_playReq = 0;

			int inext = GetNextValidItem(g_playPlaylist, g_playItem, false, g_isRunLoop);
			if (SNM_PlaylistItem* next = pl->Get(inext))
			{
				if (cur && cur->m_rgnId==next->m_rgnId && g_playItem!=inext)
					g_isRunLoop = true;

				g_playNext = inext;
				next->m_playReq++;

				// trick to stop the playlist in sync: smooth seek to the end of the project
				if (!g_repeatPlaylist && inext<g_playItem)
				{
					// temp override of the "stop play at project end" option
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

// _itemId: no hard coded value! use GetNextValidItem() or GetPrevValidItem() instead!
void PlaylistPlay(int _playlistId, int _itemId, bool _smoothSeek)
{
	MUTEX_PLAYLISTS;

	if (!g_pls.Get()->GetSize()) {
		PlaylistStop();
		MessageBox(g_pRgnPlaylistWnd?g_pRgnPlaylistWnd->GetHWND():GetMainHwnd(), __LOCALIZE("No playlist defined!\nUse the tiny button \"+\" to add one.","sws_DLG_165"), __LOCALIZE("S&M - Error","sws_DLG_165"),MB_OK);
		return;
	}

	SNM_Playlist* pl = GetPlaylist(_playlistId);
	if (!pl) // e.g. when called via actions
	{
		PlaylistStop();
		char msg[128] = "";
		_snprintfSafe(msg, sizeof(msg), __LOCALIZE_VERFMT("Unknown playlist #%d!","sws_DLG_165"), _playlistId+1);
		MessageBox(g_pRgnPlaylistWnd?g_pRgnPlaylistWnd->GetHWND():GetMainHwnd(), msg, __LOCALIZE("S&M - Error","sws_DLG_165"),MB_OK);
		return;
	}

	int num = HasNestedMarkersRegions(_playlistId);
	if (num>0)
	{
		PlaylistStop();
		char msg[256] = "";
		_snprintfSafe(msg, sizeof(msg), __LOCALIZE_VERFMT("Play preview might not work as expected!\nThe playlist #%d constains nested markers/regions (inside region %d at least)","sws_DLG_165"), _playlistId+1, num);
		MessageBox(g_pRgnPlaylistWnd?g_pRgnPlaylistWnd->GetHWND():GetMainHwnd(), msg, __LOCALIZE("S&M - Warning","sws_DLG_165"),MB_OK);
	}

	if (SNM_PlaylistItem* next = pl->Get(_itemId))
	{
		if (!_smoothSeek)
			PlaylistStop();

		if (EnumProjectMarkers2(NULL, GetMarkerRegionIndexFromId(next->m_rgnId), NULL, &g_nextRunPos, &g_nextRunEnd, NULL, NULL))
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

			g_playNext = _itemId;
			g_playItem = _playlistId==g_playPlaylist ? g_playItem : -1; // important for the 1st playlist item switch
			g_isRunLoop = g_playNext==g_playItem;
			for (int i=0; i < pl->GetSize(); i++)
				pl->Get(i)->m_playReq = i==_itemId?1:0;

			SeekPlay(g_nextRunPos, _smoothSeek);
			g_lastRunPos = g_nextRunPos;

			g_playPlaylist = _playlistId;
			if (g_pRgnPlaylistWnd)
				g_pRgnPlaylistWnd->Update(4); // update UI right now (for bullets, etc..)
		}
	}

	if (g_playPlaylist<0)
	{
		char msg[128] = "";
		_snprintfSafe(msg, sizeof(msg), __LOCALIZE_VERFMT("Playlist #%d: nothing to play!\n(empty playlist, removed regions, etc..)","sws_DLG_165"), _playlistId+1);
		MessageBox(g_pRgnPlaylistWnd?g_pRgnPlaylistWnd->GetHWND():GetMainHwnd(), msg, __LOCALIZE("S&M - Error","sws_DLG_165"),MB_OK);
	}
}

void PlaylistPlay(COMMAND_T* _ct) {
	int id = _ct ? (int)_ct->user : -1;
	int playlistId = id>=0 ? id : g_pls.Get()->m_cur;
	PlaylistPlay(playlistId, GetNextValidItem(playlistId, 0, true, true));
}

void PlaylistSeekPrevNext(COMMAND_T* _ct)
{
	// this func only makes sense when a playlist is already running..
	if (g_playPlaylist<0)
		PlaylistPlay(NULL);
	else
		PlaylistPlay(g_playPlaylist, (int)_ct->user>0 ? 
			GetNextValidItem(g_playPlaylist, g_playNext, true, false) : 
			GetPrevValidItem(g_playPlaylist, g_playNext, true, false));
}

void PlaylistStop()
{
	if (g_playPlaylist>=0 || (GetPlayState()&1) == 1) {
		OnStopButton();
		PlaylistStopped();
	}
}

void PlaylistStopped()
{
	if (g_playPlaylist>=0)
	{
		MUTEX_PLAYLISTS;
		g_playPlaylist = -1;
		g_playItem = -1;

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
		if (g_oldRepeatState >=0) {
			GetSetRepeat(g_oldRepeatState);
			g_oldRepeatState = -1;
		}

		if (g_pRgnPlaylistWnd)
			g_pRgnPlaylistWnd->Update();
	}
}

void SetPlaylistRepeat(COMMAND_T* _ct)
{
	int mode = _ct ? (int)_ct->user : -1; // toggle if no COMMAND_T is specified
	bool oldRepeat = g_repeatPlaylist;
	switch(mode) {
		case -1: g_repeatPlaylist=!g_repeatPlaylist; break;
		case 0: g_repeatPlaylist=false; break;
		case 1: g_repeatPlaylist=true; break;
	}
	if (oldRepeat!=g_repeatPlaylist) {
		RefreshToolbar(NamedCommandLookup("_S&M_PLAYLIST_TGL_REPEAT"));
		if (g_pRgnPlaylistWnd)
			g_pRgnPlaylistWnd->Update(2);
	}
}

bool IsPlaylistRepeat(COMMAND_T*) {
	return g_repeatPlaylist;
}


///////////////////////////////////////////////////////////////////////////////

// _mode: 0=crop current project, 1=crop to new project tab, 2=append to current project, 3=paste at cursor position
// note: moves/copies env points too, makes polled items, etc.. according to user prefs
//JFB TODO? crop => markers removed
void AppendPasteCropPlaylist(SNM_Playlist* _playlist, int _mode)
{
	MUTEX_PLAYLISTS;

	if (!_playlist || !_playlist->GetSize())
		return;

	int rgnNum = IsInfinite(_playlist);
	if (rgnNum>=0)
	{
		char msg[256] = "";
		_snprintfSafe(msg, sizeof(msg), __LOCALIZE_VERFMT("Paste/crop aborted: infinite loop (for region %d at least)!","sws_DLG_165"), rgnNum);
		MessageBox(g_pRgnPlaylistWnd?g_pRgnPlaylistWnd->GetHWND():GetMainHwnd(), msg, __LOCALIZE("S&M - Error","sws_DLG_165"),MB_OK);
		return;
	}

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
			if (PreventUIRefresh)
				PreventUIRefresh(1);

			InsertSilence(NULL, startPos, GetPlayListLength(_playlist));
		}
	}

//	OnStopButton();

	// make sure some envelope options are enabled: move with items + add edge points
	int oldOpt[2] = {-1,-1};
	int* options[2] = {NULL,NULL};
	if (options[0] = (int*)GetConfigVar("envattach")) {
		oldOpt[0] = *options[0];
		*options[0] = 1;
	}
	if (options[1] = (int*)GetConfigVar("env_reduce")) {
		oldOpt[1] = *options[1];
		*options[1] = 2;
	}

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
							if (PreventUIRefresh)
								PreventUIRefresh(1);
						}

						// store regions
						bool found = false;
						for (int k=0; !found && k<rgns.GetSize(); k++)
							found |= (rgns.Get(k)->GetNum() == rgnnum);
						if (!found)
							rgns.Add(new MarkerRegion(true, endPos-startPos, endPos+rgnend-rgnpos-startPos, rgnname, rgnnum, rgncol));

						// store data needed to "unsplit"
						// note: not needed when croping as those items will get removed
						WDL_PtrList_DeleteOnDestroy<SNM_ItemChunk> itemSates;
						if (_mode==2 || _mode==3)
							for (int j=0; j < itemsToKeep.GetSize(); j++)
								itemSates.Add(new SNM_ItemChunk((MediaItem*)itemsToKeep.Get(j)));

						WDL_PtrList<void> splitItems;
						SplitSelectItemsInInterval(NULL, rgnpos, rgnend, false, _mode==2 || _mode==3 ? &splitItems : NULL);

						// REAPER "bug": the last param of ApplyNudge() is ignored although
						// it is used in duplicate mode => use a loop instead
						// note: DupSelItems() is an override of the native ApplyNudge()
						if (plItem->m_cnt>0)
							for (int k=0; k < plItem->m_cnt; k++) {
								DupSelItems(NULL, endPos-rgnpos, &itemsToKeep);
								endPos += (rgnend-rgnpos);
							}

						// "unsplit" items (itemSates & splitItems are empty when croping, see above)
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

	// restore options
	if (options[0]) *options[0] = oldOpt[0];
	if (options[1]) *options[1] = oldOpt[1];

	// nothing done..
	if (!updated)
	{
		if (PreventUIRefresh)
			PreventUIRefresh(-1);
		return;
	}

	///////////////////////////////////////////////////////////////////////////
	// append/paste to current project
	if (_mode == 2 || _mode == 3)
	{
//		Main_OnCommand(40289, 0); // unselect all items
		SetEditCurPos(endPos, true, false);

		if (PreventUIRefresh)
			PreventUIRefresh(-1);

		UpdateTimeline(); // ruler+arrange

		Undo_EndBlock2(NULL, _mode==2 ? __LOCALIZE("Append playlist to project","sws_undo") : __LOCALIZE("Paste playlist at edit cursor","sws_undo"), UNDO_STATE_ALL);
		return;
	}

	///////////////////////////////////////////////////////////////////////////
	// crop current project

	// dup the playlist (needed when cropping to new project tab)
	SNM_Playlist* dupPlaylist = _mode==1 ? new SNM_Playlist(_playlist) : NULL;

	// crop current project
	GetSet_LoopTimeRange(true, false, &startPos, &endPos, false);
	Main_OnCommand(40049, 0); // crop project to time sel
//	Main_OnCommand(40289, 0); // unselect all items

	// restore (updated) regions
	for (int i=0; i<rgns.GetSize(); i++)
		rgns.Get(i)->AddToProject();

	// cleanup playlists (some other regions may have been removed)
	for (int i=g_pls.Get()->GetSize()-1; i>=0; i--)
		if (g_pls.Get()->Get(i) != _playlist)
			for (int j=0; j<g_pls.Get()->Get(i)->GetSize(); j++)
				if (GetMarkerRegionIndexFromId(g_pls.Get()->Get(i)->Get(j)->m_rgnId) < 0) {
					g_pls.Get()->Delete(i, true);
					break;
				}
	g_pls.Get()->m_cur = g_pls.Get()->Find(_playlist);
	if (g_pls.Get()->m_cur < 0)
		g_pls.Get()->m_cur = 0; // just in case..

	// crop current proj?
	if (!_mode)
	{
		// clear time sel + edit cursor position
		GetSet_LoopTimeRange(true, false, &g_d0, &g_d0, false);
		SetEditCurPos(0.0, true, false);

		if (PreventUIRefresh)
			PreventUIRefresh(-1);
		UpdateTimeline();

		Undo_EndBlock2(NULL, __LOCALIZE("Crop project to playlist","sws_undo"), UNDO_STATE_ALL);

		if (g_pRgnPlaylistWnd) {
			g_pRgnPlaylistWnd->FillPlaylistCombo();
			g_pRgnPlaylistWnd->Update();
		}
		return;
	}

	///////////////////////////////////////////////////////////////////////////
	// crop to new project tab
	// macro-ish solution because copying the project the proper way (e.g. 
	// copying all track states) is totally unstable
	// note: so the project is "copied" w/o extension data, project bay, etc..

	// store master track
	WDL_FastString mstrStates;
	if (MediaTrack* tr = CSurf_TrackFromID(0, false)) {
		SNM_ChunkParserPatcher p(tr);
		mstrStates.Set(p.GetChunk());
	}

	Main_OnCommand(40296, 0); // select all tracks
	Main_OnCommand(40210, 0); // copy tracks

	if (PreventUIRefresh)
		PreventUIRefresh(-1);

	// trick!
	Undo_EndBlock2(NULL, __LOCALIZE("Crop project to playlist","sws_undo"), UNDO_STATE_ALL);
	if (!Undo_DoUndo2(NULL))
		return;
/*JFB no-op, unfortunately
	CSurf_FlushUndo(true);
*/

	// *** NEW PROJECT TAB***
	Main_OnCommand(40859, 0);

	Undo_BeginBlock2(NULL);

	if (PreventUIRefresh)
		PreventUIRefresh(1);

	Main_OnCommand(40058, 0); // paste item/tracks
	Main_OnCommand(40297, 0); // unselect all tracks
	if (OpenClipboard(GetMainHwnd())) { // clear clipboard
	    EmptyClipboard();
		CloseClipboard();
	}
	if (MediaTrack* tr = CSurf_TrackFromID(0, false)) {
		SNM_ChunkParserPatcher p(tr);
		p.SetChunk(mstrStates.Get());
	}
	// restore regions (in the new project this time)
	for (int i=0; i<rgns.GetSize(); i++)
		rgns.Get(i)->AddToProject();

	// new project: the playlist is empty at this point
	g_pls.Get()->Add(dupPlaylist);
	g_pls.Get()->m_cur = 0;

	if (PreventUIRefresh)
		PreventUIRefresh(-1);
	SNM_UIRefresh(NULL);

	Undo_EndBlock2(NULL, __LOCALIZE("Crop project to playlist","sws_undo"), UNDO_STATE_ALL);

	if (g_pRgnPlaylistWnd) {
		g_pRgnPlaylistWnd->FillPlaylistCombo();
		g_pRgnPlaylistWnd->Update();
	}
}

void AppendPasteCropPlaylist(COMMAND_T* _ct) {
	AppendPasteCropPlaylist(GetPlaylist(), (int)_ct->user);
}


///////////////////////////////////////////////////////////////////////////////

void SNM_Playlist_UpdateJob::Perform() {
	if (g_pRgnPlaylistWnd) {
		g_pRgnPlaylistWnd->FillPlaylistCombo();
		g_pRgnPlaylistWnd->Update();
	}
}


///////////////////////////////////////////////////////////////////////////////

// ScheduledJob used because of multi-notifs
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
	// load prefs
	g_monitorMode = (GetPrivateProfileInt("RegionPlaylist", "MonitorMode", 0, g_SNMIniFn.Get()) == 1);
	g_repeatPlaylist = (GetPrivateProfileInt("RegionPlaylist", "Repeat", 0, g_SNMIniFn.Get()) == 1);
	GetPrivateProfileString("RegionPlaylist", "BigFontName", SWSDLG_TYPEFACE, g_rgnplBigFontName, sizeof(g_rgnplBigFontName), g_SNMIniFn.Get());

	g_pRgnPlaylistWnd = new SNM_RegionPlaylistWnd();
	if (!g_pRgnPlaylistWnd || !plugin_register("projectconfig", &g_projectconfig))
		return 0;
	return 1;
}

void RegionPlaylistExit()
{
	// save prefs
	WritePrivateProfileString("RegionPlaylist", "MonitorMode", g_monitorMode?"1":"0", g_SNMIniFn.Get()); 
	WritePrivateProfileString("RegionPlaylist", "Repeat", g_repeatPlaylist?"1":"0", g_SNMIniFn.Get()); 
	WritePrivateProfileString("RegionPlaylist", "BigFontName", g_rgnplBigFontName, g_SNMIniFn.Get());

	DELETE_NULL(g_pRgnPlaylistWnd);
}

void OpenRegionPlaylist(COMMAND_T*) {
	if (g_pRgnPlaylistWnd)
		g_pRgnPlaylistWnd->Show(true, true);
}

bool IsRegionPlaylistDisplayed(COMMAND_T*){
	return (g_pRgnPlaylistWnd && g_pRgnPlaylistWnd->IsValidWindow());
}

void ToggleRegionPlaylistMode(COMMAND_T*) {
	if (g_pRgnPlaylistWnd)
		g_pRgnPlaylistWnd->ToggleLock();
}

bool IsRegionPlaylistMonitoring(COMMAND_T*) {
	return g_monitorMode;
}
