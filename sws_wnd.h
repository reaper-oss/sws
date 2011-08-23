/******************************************************************************
/ sws_wnd.h
/
/ Copyright (c) 2011 Tim Payne (SWS)
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

#ifdef _WIN32
#define SWSDLG_TYPEFACE "MS Shell Dlg"
#else
#define SWSDLG_TYPEFACE "Arial"
#endif

typedef struct SWS_LVColumn
{
	int iWidth;
	int iType; // & 1 = editable, & 2 = clickable button
	const char* cLabel;
	int iPos;
} SWS_LVColumn;

class SWS_ListItem; // abstract.  At some point it might make sense to make this a real class?

class SWS_ListItemList
{
public:
	SWS_ListItemList() {}
	~SWS_ListItemList() {}
	int GetSize() { return m_list.GetSize(); }
	void Add(SWS_ListItem* item) { m_list.InsertSorted((INT_PTR*)item, ILIComp); }
	SWS_ListItem* Get(int iIndex) { return (SWS_ListItem*)m_list.Get(iIndex); }
	int Find(SWS_ListItem* item) { return m_list.FindSorted((INT_PTR*)item, ILIComp); }
	void Delete(int iIndex) { m_list.Delete(iIndex); }
	// Remove returns and also removes the last item.  It's the last because it's more efficient to remove at the end.
	SWS_ListItem* Remove() { if (!m_list.GetSize()) return NULL; int last = m_list.GetSize()-1; SWS_ListItem* item = (SWS_ListItem*)m_list.Get(last); m_list.Delete(last); return item; }
	void Empty() { m_list.Empty(); }
private:
	static int ILIComp(const INT_PTR** a, const INT_PTR** b) { return *a-*b; };
	WDL_PtrList<INT_PTR> m_list;
};

class SWS_ListView
{
public:
	SWS_ListView(HWND hwndList, HWND hwndEdit, int iCols, SWS_LVColumn* pCols, const char* cINIKey, bool bTooltips);
	virtual ~SWS_ListView();
	int GetListItemCount() { return ListView_GetItemCount(m_hwndList); }
	SWS_ListItem* GetListItem(int iIndex, int* iState = NULL);
	bool IsSelected(int index);
	SWS_ListItem* EnumSelected(int* i);
	bool SelectByItem(SWS_ListItem* item);
	int OnNotify(WPARAM wParam, LPARAM lParam);
	void OnDestroy();
	int EditingKeyHandler(MSG *msg);
	int LVKeyHandler(MSG *msg, int iKeyState);
	virtual void Update();
	bool DoColumnMenu(int x, int y);
	SWS_ListItem* GetHitItem(int x, int y, int* iCol);
	void EditListItem(SWS_ListItem* item, int iCol);
	int GetEditingItem() { return m_iEditingItem; }
	bool EditListItemEnd(bool bSave, bool bResort = true);
	int OnEditingTimer();
	
	bool IsActive(bool bWantEdit) { return GetFocus() == m_hwndList || (bWantEdit && m_iEditingItem != -1); }
	void DisableUpdates(bool bDisable) { m_bDisableUpdates = bDisable; }
	bool UpdatesDisabled() { return m_bDisableUpdates; }
	HWND GetHWND() { return m_hwndList; }

protected:
	void EditListItem(int iIndex, int iCol);
	int DisplayToDataCol(int iCol);

	// These methods are used to "pull" data for updating the listview
	virtual void SetItemText(SWS_ListItem* item, int iCol, const char* str) {}
	virtual void GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax) { str[0] = 0; }
	virtual void GetItemTooltip(SWS_ListItem* item, char* str, int iStrMax) {}
	virtual void GetItemList(SWS_ListItemList* pList) { pList->Empty(); }
	virtual int  GetItemState(SWS_ListItem* item) { return -1; } // Selection state: -1 == unchanged, 0 == false, 1 == selected
	// These inform the derived class of user interaction
	virtual bool OnItemSelChanging(SWS_ListItem* item, bool bSel) { return false; } // Returns TRUE to prevent the change, or FALSE to allow the change
	virtual void OnItemSelChanged(SWS_ListItem* item, int iState) { }
	virtual void OnItemClk(SWS_ListItem* item, int iCol, int iKeyState) {}
	virtual void OnItemBtnClk(SWS_ListItem* item, int iCol, int iKeyState) {}
	virtual void OnItemDblClk(SWS_ListItem* item, int iCol) {}
	virtual int  OnItemSort(SWS_ListItem* item1, SWS_ListItem* item2);
	virtual void OnBeginDrag(SWS_ListItem* item) {}
	int DataToDisplayCol(int iCol);
	void SetListviewColumnArrows(int iSortCol);
	static int CALLBACK sListCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lSortParam);

	HWND m_hwndList;
	HWND m_hwndTooltip;
	bool m_bDisableUpdates;
	int m_iSortCol; // 1 based col index, negative for desc sort
	int m_iEditingItem;
	const int m_iCols;
	SWS_LVColumn* m_pCols;

#ifndef _WIN32
	int m_iClickedKeys;
#endif

private:
	void ShowColumns();

#ifndef _WIN32
	int m_iClickedCol;
	SWS_ListItem* m_pClickedItem;
#else
	DWORD m_dwSavedSelTime;
	bool m_bShiftSel;
#endif
	WDL_TypedBuf<int> m_pSavedSel;
	HWND m_hwndEdit;
	int m_iEditingCol;
	SWS_LVColumn* m_pDefaultCols;
	const char* m_cINIKey;
};

#pragma pack(push, 4)
typedef struct SWS_DockWnd_State // Coverted to little endian on store
{
	RECT r;
	int state;
	int whichdock;
} SWS_DockWnd_State;
#pragma pack(pop)

class SWS_DockWnd
{
public:
	SWS_DockWnd(int iResource, const char* cWndTitle, const char* cId, int iDockOrder, int iCmdID);
	virtual ~SWS_DockWnd();

	void Show(bool bToggle, bool bActivate);
	virtual bool IsActive(bool bWantEdit = false);
	bool IsValidWindow() { return IsWindow(m_hwnd) ? true : false; }
	HWND GetHWND() { return m_hwnd; }
	virtual void OnCommand(WPARAM wParam, LPARAM lParam) {}

	static const int DOCK_MSG = 0xFF0000;

protected:
	void Init(); // call from derived constructor!!
	bool IsDocked() { return (m_state.state & 2) == 2; }
	void ToggleDocking();
	virtual void OnInitDlg() {}
	virtual int OnNotify(WPARAM wParam, LPARAM lParam) { return 0; }
	virtual HMENU OnContextMenu(int x, int y) { return NULL; }
	virtual void OnResize() {}
	virtual void OnDestroy() {}
	virtual void OnTimer(WPARAM wParam=0) {}
	virtual void OnDroppedFiles(HDROP h) {}
	virtual int OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam) { return 0; }
	virtual int OnKey(MSG* msg, int iKeyState) { return 0; } // return 1 for "processed key"
	
	// Functions for derived classes to load/save some view information (for startup/screensets)
	virtual int SaveView(char* cViewBuf, int iLen) { return 0; } // return num of chars in state (if cViewBuf == NULL, ret # of bytes needed)
	virtual void LoadView(const char* cViewBuf, int iLen) {}

	HWND m_hwnd;
	bool m_bUserClosed;
	bool m_bLoadingState;
	WDL_WndSizer m_resize;
	WDL_PtrList<SWS_ListView> m_pLists;

private:
	static INT_PTR WINAPI sWndProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	int wndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT screensetCallbackOld(int action, char *id, void *param, int param2);
	static LRESULT screensetCallback(int action, char *id, void *param, void *actionParm, int actionParmSize);
	static int keyHandler(MSG *msg, accelerator_register_t *ctx);
	int SaveState(char* cStateBuf, int iMaxLen);
	void LoadState(const char* cStateBuf, int iLen);
	int m_iCmdID;
	const int m_iResource;
	const char* m_cWndTitle;
	const char* m_cId;
	const int m_iDockOrder; // v4 TODO delete me
	accelerator_register_t m_ar;
	SWS_DockWnd_State m_state;
};
