/******************************************************************************
/ Snapshots.h
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

#define _SNAP_TINY_BUTTONS //JFB if disabled, IDC_OPTIONS must be made visible in the .rc

#ifdef _SNAP_TINY_BUTTONS
#include "../SnM/SnM_VWnd.h"
#endif

class SWS_SnapshotsView : public SWS_ListView
{
public:
	SWS_SnapshotsView(HWND hwndList, HWND hwndEdit);
	int OnNotify(WPARAM wParam, LPARAM lParam);

protected:
	int OnItemSort(SWS_ListItem* lParam1, SWS_ListItem* lParam2);
	void SetItemText(SWS_ListItem* item, int iCol, const char* str);
	void GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax);
	void GetItemTooltip(SWS_ListItem* item, char* str, int iStrMax);
	void GetItemList(SWS_ListItemList* pList);
	int  GetItemState(SWS_ListItem* item);
	void OnItemClk(SWS_ListItem* item, int iCol, int iKeyState);
};

class SWS_SnapshotsWnd : public SWS_DockWnd
{
public:
	SWS_SnapshotsWnd();
	virtual ~SWS_SnapshotsWnd();

	void Update();
	void RenameCurrent();
	void SetFilterType(int iType) { m_iSelType = iType; }
	int  GetFilterType() { return m_iSelType; }
	static bool GetPromptOnDeletedTracks(); // NF: #1073
	
protected:
	void OnInitDlg();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	HMENU OnContextMenu(int x, int y, bool* wantDefaultItems);
	void OnResize();
	void OnDestroy();
	int OnKey(MSG* msg, int iKeyState);
	void GetOptions();
	void ShowControls(bool bShow);
#ifdef _SNAP_TINY_BUTTONS
	void GetMinSize(int* w, int* h) { *w=200; *h=MIN_DOCKWND_HEIGHT; }
	void DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight = NULL);
#endif
private:
	int m_iSelType;
#ifdef _SNAP_TINY_BUTTONS
	SNM_TwoTinyButtons m_tinyLRbtns;
	SNM_TinyLeftButton m_btnLeft;
	SNM_TinyRightButton m_btnRight;
#endif
};

class Snapshot;

int SnapshotsInit();
void RegisterSnapshotSlot(int);
void SnapshotsExit();
void OpenSnapshotsDialog(COMMAND_T* = NULL);
void NewSnapshot(int iMask, bool bSelOnly);
void NewSnapshot(COMMAND_T* = NULL);
Snapshot* GetSnapshotPtr(int i);
void GetSnapshot(COMMAND_T*);
void GetSnapshot(int slot, int iMask, bool bSelOnly);
void SaveSnapshot(COMMAND_T*);
void UpdateSnapshotsDialog(bool bSelChange = false);
