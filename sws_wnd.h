/******************************************************************************
/ sws_wnd.h
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

typedef struct SWS_LVColumn
{
	int iWidth;
	bool bEditable;
	const char* cLabel;
	int iPos;
} SWS_LVColumn;

class SWS_ListView
{
public:
	SWS_ListView(HWND hwndList, HWND hwndEdit, int iCols, SWS_LVColumn* pCols, const char* cINIKey, bool bTooltips);
	virtual ~SWS_ListView();
	LPARAM GetListItem(int iIndex);
	int OnNotify(WPARAM wParam, LPARAM lParam);
	void OnDestroy();
	int KeyHandler(MSG *msg);
	void Update();
	bool DoColumnMenu(int x, int y);
	LPARAM GetHitItem(int x, int y, int* iCol);
	void EditListItem(LPARAM item, int iCol);
	
	bool IsActive() { return GetFocus() == m_hwndList; }
	void DisableUpdates(bool bDisable) { m_bDisableUpdates = bDisable; }
	bool UpdatesDisabled() { return m_bDisableUpdates; }
	HWND GetHWND() { return m_hwndList; }

protected:
	void EditListItem(int iIndex, int iCol);

	// These methods are used to "pull" data for updating the listview
	virtual void SetItemText(LPARAM item, int iCol, const char* str) {}
	virtual void GetItemText(LPARAM item, int iCol, char* str, int iStrMax) { str[0] = 0; }
	virtual void GetItemTooltip(LPARAM item, char* str, int iStrMax) {}
	virtual int GetItemCount() { return 0; }
	virtual LPARAM GetItemPointer(int iItem) { return NULL; }
	virtual bool GetItemState(LPARAM item) { return false; } // Selection state
	// These inform the derived class of user interaction
	virtual bool OnItemSelChange(LPARAM item, bool bSel) { return false; } // Returns TRUE to prevent the change, or FALSE to allow the change
	virtual void OnItemClk(LPARAM item, int iCol) {}
	virtual void OnItemDblClk(LPARAM item, int iCol) {}
	virtual int OnItemSort(LPARAM item1, LPARAM item2);
	virtual void OnBeginDrag() {}

	HWND m_hwndList;
	bool m_bDisableUpdates;
	int m_iSortCol; // 1 based col index, negative for desc sort

private:
	void EditListItemEnd(bool bSave, bool bResort = true);
	void ShowColumns();
	void SetListviewColumnArrows(int iSortCol);
	int DisplayToDataCol(int iCol);
	int DataToDisplayCol(int iCol);
	static int CALLBACK sListCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lSortParam);

	HWND m_hwndEdit;
	HWND m_hwndTooltip;
	int m_iEditingItem;
	int m_iEditingCol;
	const int m_iCols;
	SWS_LVColumn* m_pCols;
	const char* m_cINIKey;
};

class SWS_DockWnd
{
public:
	SWS_DockWnd(int iResource, const char* cName, int iDockOrder);
	void Show(bool bToggle, bool bActivate);
	bool IsActive();
	bool IsValidWindow() { return IsWindow(m_hwnd) ? true : false; }
	HWND GetHWND() { return m_hwnd; }
	virtual ~SWS_DockWnd();

	static const int DOCK_MSG = 0xFF0000;

protected:
	void ToggleDocking();
	virtual void OnInitDlg() {}
	virtual void OnCommand(WPARAM wParam, LPARAM lParam) {}
	virtual int OnNotify(WPARAM wParam, LPARAM lParam) { return 0; }
	virtual HMENU OnContextMenu(int x, int y) { return NULL; }
	virtual void OnResize() {}
	virtual void OnDestroy() {}
	virtual void OnTimer() {}
	virtual void OnDroppedFiles(HDROP h) {}

	const int m_iResource;
	const char* m_cName;
	char* m_cWndPosKey;
	char* m_cWndStateKey;
	const int m_iDockOrder;
	HWND m_hwnd;
	bool m_bUserClosed;
	bool m_bDocked;
	bool m_bShowAfterInit;
	WDL_WndSizer m_resize;
	WDL_PtrList<SWS_ListView> m_pLists;

private:
	static INT_PTR WINAPI sWndProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	int wndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LPARAM screensetCallback(int action, char *id, void *param, int param2);
	static int keyHandler(MSG *msg, accelerator_register_t *ctx);
	accelerator_register_t m_ar;
};
