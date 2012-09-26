/******************************************************************************
/ SnM_RgnPlaylistView.h
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

//#pragma once

#ifndef _SNM_PLAYLISTVIEW_H_
#define _SNM_PLAYLISTVIEW_H_

#include "../MarkerList/MarkerListClass.h"

class SNM_Playlist_MarkerRegionSubscriber : public SNM_MarkerRegionSubscriber {
public:
	SNM_Playlist_MarkerRegionSubscriber() : SNM_MarkerRegionSubscriber() {}
	void NotifyMarkerRegionUpdate(int _updateFlags);
};

// note: no other attributes (like a comment) because of
// the playlist "auto-compacting" feature..
class SNM_PlaylistItem {
public:
	SNM_PlaylistItem(int _rgnId=-1, int _cnt=1, int _playReq=0) : m_rgnId(_rgnId),m_cnt(_cnt),m_playReq(_playReq) {}
	int m_rgnId, m_cnt, m_playReq;
};

class SNM_Playlist : public WDL_PtrList<SNM_PlaylistItem>
{
public:
	SNM_Playlist(SNM_Playlist* _p = NULL, const char* _name = NULL)
		: m_name(_name), WDL_PtrList<SNM_PlaylistItem>() {
		if (_p) {
			for (int i=0; i < _p->GetSize(); i++)
				Add(new SNM_PlaylistItem(_p->Get(i)->m_rgnId, _p->Get(i)->m_cnt));
			if (!_name)
				m_name.Set(_p->m_name.Get());
		}
	}
	SNM_Playlist(const char* _name) : m_name(_name), WDL_PtrList<SNM_PlaylistItem>() {}
	~SNM_Playlist() {}
	WDL_FastString m_name;
};

class SNM_Playlists : public WDL_PtrList<SNM_Playlist>
{
public:
	SNM_Playlists() : m_cur(0), WDL_PtrList<SNM_Playlist>() {}
	~SNM_Playlists() {}
	int m_cur;
};

class SNM_PlaylistView : public SWS_ListView {
public:
	SNM_PlaylistView(HWND hwndList, HWND hwndEdit);
	void UpdateCompact();
protected:
	void GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax);
	void GetItemList(SWS_ListItemList* pList);
	void SetItemText(SWS_ListItem* item, int iCol, const char* str);
	void OnItemDblClk(SWS_ListItem* item, int iCol);
	int OnItemSort(SWS_ListItem* _item1, SWS_ListItem* _item2);
	void OnBeginDrag(SWS_ListItem* item);
	void OnDrag();
	void OnEndDrag();
	WDL_PtrList<SNM_PlaylistItem> m_draggedItems;
};

class SNM_RegionPlaylistWnd : public SWS_DockWnd
{
public:
	SNM_RegionPlaylistWnd();
	~SNM_RegionPlaylistWnd() {}
	void GetMinSize(int* w, int* h) { *w=202; *h=175; }
	void OnCommand(WPARAM wParam, LPARAM lParam);
	void Update(int _flags = 3);
	void FillPlaylistCombo();
	void CSurfSetTrackListChange();
protected:
	void OnInitDlg();
	void OnDestroy();
	HMENU OnContextMenu(int x, int y, bool* wantDefaultItems);
	void AddPasteContextMenu(HMENU _menu);
	int OnKey(MSG* msg, int iKeyState) ;
	void DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight = NULL);
	bool GetToolTipString(int _xpos, int _ypos, char* _bufOut, int _bufOutSz);

	SNM_Playlist_MarkerRegionSubscriber m_mkrRgnSubscriber;
	WDL_VirtualStaticText m_txtPlaylist, m_txtLength;
	WDL_VirtualComboBox m_cbPlaylist;
	SNM_MiniAddDelButtons m_btnsAddDel;
	SNM_ToolbarButton m_btnCrop;
	WDL_VirtualIconButton m_btnPlay, m_btnStop, m_btnRepeat;
};

class SNM_Playlist_UpdateJob : public SNM_ScheduledJob {
public:
	SNM_Playlist_UpdateJob() : SNM_ScheduledJob(SNM_SCHEDJOB_PLAYLIST_UPDATE, 150) {}
	void Perform();
};

double GetPlayListLength(SNM_Playlist* _playlist);
void PlaylistRun();
void PlaylistPlay(int _playlistId, int _itemId = 0);
void PlaylistPlay(COMMAND_T*);
void PlaylistStopped();
void SetPlaylistRepeat(COMMAND_T*);
bool IsPlaylistRepeat(COMMAND_T*);
void AppendPasteCropPlaylist(SNM_Playlist* _playlist, int _mode);
void AppendPasteCropPlaylist(COMMAND_T*);
int RegionPlaylistInit();
void RegionPlaylistExit();
void OpenRegionPlaylist(COMMAND_T*);
bool IsRegionPlaylistDisplayed(COMMAND_T*);

#endif
