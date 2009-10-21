/******************************************************************************
/ MarkerList.h
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

class SWS_MarkerListView : public SWS_ListView
{
public:
	SWS_MarkerListView(HWND hwndList, HWND hwndEdit);

protected:
	void SetItemText(LPARAM item, int iCol, const char* str);
	void GetItemText(LPARAM item, int iCol, char* str, int iStrMax);
	bool OnItemSelChange(LPARAM item, bool bSel);
	void OnItemDblClk(LPARAM item, int iCol);
	int OnItemSort(LPARAM item1, LPARAM item2);

	int GetItemCount();
	LPARAM GetItemPointer(int iItem);
	bool GetItemState(LPARAM item);
};

class SWS_MarkerListWnd : public SWS_DockWnd
{
public:
	SWS_MarkerListWnd();
	void Update();
	
protected:
	void OnInitDlg();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	HMENU OnContextMenu(int x, int y);
	void OnDestroy();
	void OnTimer();
};

#define TRACKLIST_FORMAT_KEY "Tracklist Format"
#define TRACKLIST_FORMAT_DEFAULT "an - d (l)"

int MarkerListInit();
extern MarkerList* g_curList;
extern SWS_MarkerListWnd* pMarkerList;

// Functions to show dialog boxes
void SaveMarkerList(COMMAND_T* = NULL);
void DeleteMarkerList(COMMAND_T* = NULL);
void ExportFormat(COMMAND_T* = NULL);
void OpenMarkerList(COMMAND_T* = NULL);
