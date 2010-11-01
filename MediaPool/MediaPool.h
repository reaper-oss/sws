/******************************************************************************
/ MediaPool.h
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

class SWS_MediaPoolFile
{
public:
	SWS_MediaPoolFile(const char* cFilename, int id);
	~SWS_MediaPoolFile();

	char* GetString(char* str, int iStrMax);
	void SetAction(bool bAction, const char* cGroup, bool bActive);
	bool GetAction() { return m_bAction; }
	const char* GetFilename() { return m_cFilename; }
	int GetID() { return m_id; }
	void SetID(int id) { m_id = id; }

private:
	void RegisterCommand(const char* cGroup);
	void UnregisterCommand();

	bool m_bAction;
	bool m_bActive;
	char* m_cFilename;
	int m_id;
};

class SWS_MediaPoolGroup
{
public:
	SWS_MediaPoolGroup(const char* cName, bool bGlobal);
	~SWS_MediaPoolGroup();
	void AddFile(const char* cFile, int id, bool bAction);
	void SetName(const char* cName);
	void SetActive(bool bActive);
	void PopulateFromReaper();

	WDL_PtrList<SWS_MediaPoolFile> m_files;
	char* m_cGroupname;
	bool m_bGlobal;

};

class SWS_MediaPoolWnd;

class SWS_MediaPoolGroupView : public SWS_ListView
{
public:
	SWS_MediaPoolGroupView(HWND hwndList, HWND hwndEdit, SWS_MediaPoolWnd* pWnd);
	void DoDelete();

protected:
	void SetItemText(LPARAM item, int iCol, const char* str);
	void GetItemText(LPARAM item, int iCol, char* str, int iStrMax);
	void OnItemBtnClk(LPARAM item, int iCol, int iKeyState);
	void OnItemSelChanged(LPARAM item, int iState);
	void GetItemList(WDL_TypedBuf<LPARAM>* pBuf);
	int  GetItemState(LPARAM item);

private:
	SWS_MediaPoolWnd* m_pWnd;
};

class SWS_MediaPoolFileView : public SWS_ListView
{
public:
	SWS_MediaPoolFileView(HWND hwndList, HWND hwndEdit, SWS_MediaPoolWnd* pWnd);
	void DoDelete();

protected:
	void GetItemText(LPARAM item, int iCol, char* str, int iStrMax);
	void OnItemBtnClk(LPARAM item, int iCol, int iKeyState);
	void OnItemSelChanged(LPARAM item, int iState);
	void OnItemDblClk(LPARAM item, int iCol);
	void GetItemList(WDL_TypedBuf<LPARAM>* pBuf);
	void OnBeginDrag(LPARAM item);

private:
	SWS_MediaPoolWnd* m_pWnd;
};

class SWS_MediaPoolWnd : public SWS_DockWnd
{
public:
	SWS_MediaPoolWnd();
	void Update();

	WDL_PtrList<SWS_MediaPoolGroup> m_globalGroups;
	SWSProjConfig<WDL_PtrList_DeleteOnDestroy<SWS_MediaPoolGroup> > m_projGroups;
	SWS_MediaPoolGroup* m_curGroup;
	SWS_MediaPoolGroup m_projGroup;

protected:
	void OnInitDlg();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	void OnDroppedFiles(HDROP h);
	HMENU OnContextMenu(int x, int y);
	int OnKey(MSG* msg, int iKeyState);
};

void OpenMediaPool(COMMAND_T*);
void MediaPoolUpdate();
int MediaPoolInit();
void MediaPoolExit();
