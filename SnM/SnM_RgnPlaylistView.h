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

#ifndef _SNM_PlaylistView_H_
#define _SNM_PlaylistView_H_


class SNM_PlaylistView : public SWS_ListView {
public:
	SNM_PlaylistView(HWND hwndList, HWND hwndEdit);
protected:
	void GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax);
	void GetItemList(SWS_ListItemList* pList);
	bool IsEditListItemAllowed(SWS_ListItem* item, int iCol);
	void SetItemText(SWS_ListItem* item, int iCol, const char* str);
	void OnItemDblClk(SWS_ListItem* item, int iCol);
};


class SNM_RegionPlaylistWnd : public SWS_DockWnd
{
public:
	SNM_RegionPlaylistWnd();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	bool Find(int _mode);
	bool FindMediaItem(int _type, bool _allTakes, bool (*jobTake)(MediaItem_Take*,const char*,char*), bool (*jobItem)(MediaItem*,const char*,char*) = NULL);
	bool FindTrack(int _type, bool (*job)(MediaTrack*,const char*,char*));
	bool FindMarkerRegion(int _type);
	bool FindFilename(int _type, bool _withPath, bool (*job)(const char*,const char*,char*));
protected:
	void OnInitDlg();
	void OnDestroy();
	INT_PTR WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	int OnKey(MSG* msg, int iKeyState) ;
	void DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight = NULL);
	HBRUSH OnColorEdit(HWND _hwnd, HDC _hdc);

	WDL_VirtualIconButton m_btnPlay, m_btnStop;
};

int RegionPlaylistInit();
void RegionPlaylistExit();
void OpenRegionPlaylist(COMMAND_T*);
bool IsRegionPlaylistDisplayed(COMMAND_T*);

#endif
