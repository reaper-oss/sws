/******************************************************************************
/ Tracklist.h
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


#pragma once

class SWS_TrackListWnd;

class SWS_TrackListView : public SWS_ListView
{
public:
	SWS_TrackListView(HWND hwndList, HWND hwndEdit, SWS_TrackListWnd* pTrackListWnd);

protected:
	static int CALLBACK ListComparo(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	void SetItemText(LPARAM item, int iCol, const char* str);
	void GetItemText(LPARAM item, int iCol, char* str, int iStrMax);
	int GetItemCount();
	LPARAM GetItemPointer(int iItem);
	bool GetItemState(LPARAM item);
	void OnItemClk(LPARAM item, int iCol);

private:
	SWS_TrackListWnd* m_pTrackListWnd;
};

class SWS_TrackListWnd : public SWS_DockWnd
{
public:
	SWS_TrackListWnd();
	void Update();
	void ClearFilter();
	void ScheduleUpdate() { m_bUpdate = true; } //	if (!g_bDisableUpdates TODO??)
	bool HideFiltered() { return m_bHideFiltered; }
	bool Linked() { return m_bLink; }
	SWSProjConfig<FilteredVisState>* GetFilter() { return &m_filter; }
	
protected:
	void OnInitDlg();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	HMENU OnContextMenu(int x, int y);
	void OnDestroy();
	void OnTimer();

private:
	bool m_bUpdate;
	MediaTrack* m_trLastTouched;
	SWSProjConfig<FilteredVisState> m_filter;
	bool m_bHideFiltered;
	bool m_bLink;
	const char* m_cOptionsKey;
};

int TrackListInit();
void ScheduleTracklistUpdate();