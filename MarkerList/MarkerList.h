/******************************************************************************
/ MarkerList.h
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

class SWS_MarkerListWnd;

class SWS_MarkerListView : public SWS_ListView
{
public:
	SWS_MarkerListView(HWND hwndList, HWND hwndEdit, SWS_MarkerListWnd* pList);

protected:
	void SetItemText(SWS_ListItem* item, int iCol, const char* str);
	void GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax);
	void OnItemSelChanged(SWS_ListItem* item, int iState);
	void OnItemClk(SWS_ListItem* item, int iCol, int iKeyState);
	void OnItemDblClk(SWS_ListItem* item, int iCol);
	int  OnItemSort(SWS_ListItem* item1, SWS_ListItem* item2);
	void GetItemList(SWS_ListItemList* pList);
	int  GetItemState(SWS_ListItem* item);

private:
	SWS_MarkerListWnd* m_pMarkerList;
};

class SWS_MarkerListWnd : public SWS_DockWnd
{
public:
	SWS_MarkerListWnd();
	void Update(bool bForce = false);
	double m_dCurPos;

	WDL_String m_filter;
	bool m_bPlayOnSel;
	bool m_bScroll;
	
protected:
	void OnInitDlg();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	HMENU OnContextMenu(int x, int y, bool* wantDefaultItems);
	void OnDestroy();
	void OnTimer(WPARAM wParam=0);
	int OnKey(MSG* msg, int iKeyState);
};

#define EXPORT_FORMAT_KEY "MarkerExport Format"
#define EXPORT_FORMAT_DEFAULT "an - d (l)"

int MarkerListInit();
void MarkerListExit();
extern MarkerList* g_curList;
extern SWS_MarkerListWnd* g_pMarkerList;

// Functions to show dialog boxes
void SaveMarkerList(COMMAND_T* = NULL);
void DeleteMarkerList(COMMAND_T* = NULL);
void ExportFormat(COMMAND_T* = NULL);
void OpenMarkerList(COMMAND_T* = NULL);
