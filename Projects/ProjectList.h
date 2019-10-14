/******************************************************************************
/ ProjectList.h
/
/ Copyright (c) 2010 Tim Payne (SWS)
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

class SWS_ProjectListView : public SWS_ListView
{
public:
	SWS_ProjectListView(HWND hwndList, HWND hwndEdit);

protected:
	void GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax);
	void OnItemDblClk(SWS_ListItem* item, int iCol);
	void GetItemList(SWS_ListItemList* pList);
};

class SWS_ProjectListWnd : public SWS_DockWnd
{
public:
	SWS_ProjectListWnd();
	void Update();
	
protected:
	void OnInitDlg();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	HMENU OnContextMenu(int x, int y, bool* wantDefaultItems);
	int OnKey(MSG* msg, int iKeyState);
};

void OpenProjectList(COMMAND_T*);
int ProjectListEnabled(COMMAND_T*);
void ProjectListUpdate();
int ProjectListInit();
void ProjectListExit();
