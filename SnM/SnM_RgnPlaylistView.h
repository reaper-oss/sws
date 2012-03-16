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

#pragma once

#ifndef _SNM_PLAYLISTVIEW_H_
#define _SNM_PLAYLISTVIEW_H_


// note: no other attributes (like a comment") because of 
// the playlist "auto-compacting" feature..
class SNM_RgnPlaylistItem {
public:
	SNM_RgnPlaylistItem(int _rgnId=-1, int _cnt=1, int _playRequested=0)
		: m_rgnId(_rgnId),m_cnt(_cnt),m_playRequested(_playRequested) {}
	int m_rgnId, m_cnt, m_playRequested;
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
	WDL_PtrList<SNM_RgnPlaylistItem> m_draggedItems;
};

class SNM_RegionPlaylistWnd : public SWS_DockWnd
{
public:
	SNM_RegionPlaylistWnd();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	void Update(int _flags = 3);
protected:
	void OnInitDlg();
	void OnDestroy() {}
	HMENU OnContextMenu(int x, int y, bool* wantDefaultItems);
	INT_PTR WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	int OnKey(MSG* msg, int iKeyState) ;
	void DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight = NULL);
	HBRUSH OnColorEdit(HWND _hwnd, HDC _hdc);
	SNM_ToolbarButton m_btnCrop;
	WDL_VirtualIconButton m_btnPlay, m_btnStop;
};

class SNM_RgnPlaylistCurNextItems {
public:
	SNM_RgnPlaylistCurNextItems(SNM_RgnPlaylistItem* _cur=NULL, SNM_RgnPlaylistItem* _next=NULL)
		: m_cur(_cur),m_next(_next) {}
	SNM_RgnPlaylistItem* m_cur;
	SNM_RgnPlaylistItem* m_next;
};


void PlaylistRun();
void PlaylistPlay(int _playlistId, bool _errMsg);
void PlaylistPlay(COMMAND_T*);
void PlaylistStopped();

void CropProjectToPlaylist();

int RegionPlaylistInit();
void RegionPlaylistExit();
void OpenRegionPlaylist(COMMAND_T*);
bool IsRegionPlaylistDisplayed(COMMAND_T*);


#endif
