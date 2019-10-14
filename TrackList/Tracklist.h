/******************************************************************************
/ Tracklist.h
/
/ Copyright (c) 2009 Tim Payne (SWS)
/
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

class SWS_TrackListWnd;

class SWS_TrackListView : public SWS_ListView
{
public:
	SWS_TrackListView(HWND hwndList, HWND hwndEdit, SWS_TrackListWnd* pTrackListWnd);

protected:
	void SetItemText(SWS_ListItem* item, int iCol, const char* str);
	void GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax);
	void GetItemList(SWS_ListItemList* pList);
	int  GetItemState(SWS_ListItem* item);
	void OnItemBtnClk(SWS_ListItem* item, int iCol, int iKeyState);
	void OnItemDblClk(SWS_ListItem* item, int iCol);
	void OnItemSelChanged(SWS_ListItem* item, int iState);

private:
	SWS_TrackListWnd* m_pTrackListWnd;
};

class SWS_TrackListWnd : public SWS_DockWnd
{
public:
	SWS_TrackListWnd();
	void Update();
	void ClearFilter();
	void ScheduleUpdate() { m_bUpdate = true; }
	bool HideFiltered() { return m_bHideFiltered; }
	bool Linked() { return m_bLink; }
	SWSProjConfig<FilteredVisState>* GetFilter() { return &m_filter; }
	MediaTrack* m_trLastTouched;
	
protected:
	void OnInitDlg();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	HMENU OnContextMenu(int x, int y, bool* wantDefaultItems);
	void OnDestroy();
	void OnTimer(WPARAM wParam=0);
	int OnKey(MSG* msg, int iKeyState);

private:
	bool m_bUpdate;
	SWSProjConfig<FilteredVisState> m_filter;
	bool m_bHideFiltered;
	bool m_bLink;
	const char* m_cOptionsKey;
};

int TrackListInit();
void TrackListExit();
void ScheduleTracklistUpdate();
