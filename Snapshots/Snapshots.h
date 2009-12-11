/******************************************************************************
/ Snapshots.h
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

class SWS_SnapshotsView : public SWS_ListView
{
public:
	SWS_SnapshotsView(HWND hwndList, HWND hwndEdit);
	int OnNotify(WPARAM wParam, LPARAM lParam);

protected:
	int OnItemSort(LPARAM lParam1, LPARAM lParam2);
	void SetItemText(LPARAM item, int iCol, const char* str);
	void GetItemText(LPARAM item, int iCol, char* str, int iStrMax);
	void GetItemTooltip(LPARAM item, char* str, int iStrMax);
	void GetItemList(WDL_TypedBuf<LPARAM>* pBuf);
	int  GetItemState(LPARAM item);
	bool OnItemSelChange(LPARAM item, bool bSel);
	void OnItemClk(LPARAM item, int iCol, int iKeyState);
};

class SWS_SnapshotsWnd : public SWS_DockWnd
{
public:
	SWS_SnapshotsWnd();
	void Update();
	void RenameCurrent();

	Snapshot* m_pLastTouched;
	
protected:
	void OnInitDlg();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	HMENU OnContextMenu(int x, int y);
	void OnResize();
	void OnDestroy();
	void GetOptions();
	void ShowControls(bool bShow);

private:
	int m_iSelType;
};


int SnapshotsInit();
void SnapshotsExit();
void OpenSnapshotsDialog(COMMAND_T* = NULL);
void NewSnapshot(int iMask, bool bSelOnly);
void NewSnapshot(COMMAND_T* = NULL);
Snapshot* GetSnapshotPtr(int i);
void GetSnapshot(COMMAND_T* ct);
void GetSnapshot(int slot, int iMask, bool bSelOnly);
void UpdateSnapshotsDialog();
