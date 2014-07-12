/******************************************************************************
/ Autocolor.h
/
/ Copyright (c) 2010-2012 Tim Payne (SWS), Jeffos, wol
/ https://code.google.com/p/sws-extension
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

#include "../SnM/SnM.h"
#include "../SnM/SnM_VWnd.h"

#define WOL_AA_MARKER_MASK 1
#define WOL_AA_REGION_MASK 2

class SWS_RuleItem
{
public:
	SWS_RuleItem(int type, const char* filter, int color, const char* actionid, const char* icon)
		: m_type(type), m_str_filter(filter), m_color(color), m_icon(icon) {
		m_action = -1;
		m_actionString.Set(actionid);

		if (type)
			m_actionName.Set(UTF8_BULLET);
		else
			m_actionName.Set("");
	}

	int m_type;
	WDL_String m_str_filter;
	int m_color;
	int m_action;
	WDL_String m_actionString;
	WDL_String m_actionName;
	WDL_String m_icon;
};

class SWS_RuleTrack
{
public:
	SWS_RuleTrack(MediaTrack* tr, int id = 0)
		:m_pTr(tr), m_col(0), m_bColored(false), m_bIconed(false), m_bACEnabled(true), m_bAIEnabled(true), m_bAAEnabled(true), m_lastMatchedRule(-1) {
		m_lastExecAction.Set("");
		if (id)
			m_id = id;
		else
			m_id = CSurf_TrackToID(tr, false);
	}

	bool IsNotDefault()
	{
		if (!m_bACEnabled || !m_bAIEnabled || !m_bAAEnabled || m_lastMatchedRule != -1 || strcmp(m_lastExecAction.Get(), ""))
			return true;
		return false;
	}

	MediaTrack* m_pTr;
	int m_col;
	bool m_bColored;
	WDL_String m_icon;
	bool m_bIconed;

	bool m_bACEnabled;
	bool m_bAIEnabled;
	bool m_bAAEnabled;

	int m_id; // 1-based
	WDL_String m_lastExecAction;

	int m_lastMatchedRule;
};

class SWS_AutoColorView : public SWS_ListView
{
public:
	SWS_AutoColorView(HWND hwndList, HWND hwndEdit);
	void OnDrag();

protected:
	void SetItemText(SWS_ListItem* item, int iCol, const char* str);
	void GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax);
	void GetItemList(SWS_ListItemList* pList);
	void OnItemDblClk(SWS_ListItem* item, int iCol);
	void OnItemSelChanged(SWS_ListItem* item, int iState);
	void OnBeginDrag(SWS_ListItem* item);
};

class SWS_AutoColorTrackListView : public SWS_ListView
{
public:
	SWS_AutoColorTrackListView(HWND hwndList, HWND hwndEdit);
	void OnDrag();

protected:
	void GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax);
	void GetItemList(SWS_ListItemList* pList);
	void OnItemBtnClk(SWS_ListItem* item, int iCol, int iKeyState);
	void OnBeginDrag(SWS_ListItem* item);

private:
	int m_iBeginDragCol;
	SWS_RuleItem* m_pOldHitItem;
};

class SWS_AutoColorWnd : public SWS_DockWnd
{
public:
	SWS_AutoColorWnd();
	void Update();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	void ScheduleTrackListUpdate() { m_bTrackListUpdate = true; }
	
protected:
	void OnInitDlg();
#ifndef _WIN32
	bool m_bSettingColor;
#endif
	void OnTimer(WPARAM wParam = 0);

	void OnDestroy();
	INT_PTR OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);
	HMENU OnContextMenu(int x, int y, bool* wantDefaultItems);
	void AddOptionsMenu(HMENU _menu);
	int OnKey(MSG* msg, int iKeyState);
	void DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight = NULL);
	void OnResize();

	SWS_AutoColorView* m_pView;
	SWS_AutoColorTrackListView* m_pTrListView;

private:
	bool m_bTrackListUpdate;

	SNM_TwoTinyButtons m_tinyLRbtns;
	SNM_TinyLeftButton m_btnLeft;
	SNM_TinyRightButton m_btnRight;
};

int AutoColorInit();
int AutoColorInitTimer();
void AutoColorExit();
void OpenAutoColor(COMMAND_T* = NULL);
void AutoColorMarkerRegion(bool bForce, int flags = SNM_MARKER_MASK|SNM_REGION_MASK);
void AutoColorTrack(bool bForce);
void AutoColorTrackListUpdate();