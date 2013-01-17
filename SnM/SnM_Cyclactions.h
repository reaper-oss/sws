/******************************************************************************
/ SnM_Cyclactions.h
/
/ Copyright (c) 2011-2013 Jeffos
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

//#pragma once

#ifndef _SNM_CYCLACTION_H_
#define _SNM_CYCLACTION_H_

#define EMPTY_CYCLACTION		"no-op,65535"
#define MAX_CYCLATION_LEN		4096

class Cyclaction
{
public:
	// constructors suppose params are valid
	Cyclaction(const char* _desc=EMPTY_CYCLACTION, bool _added=false) : m_desc(_desc), m_performState(0), m_added(_added), m_empty(false) {UpdateNameAndCmds();}
	Cyclaction(Cyclaction* _a) : m_desc(_a->m_desc), m_performState(_a->m_performState), m_added(_a->m_added), m_empty(_a->IsEmpty()) {UpdateNameAndCmds();}
	~Cyclaction() {}
	void Update(const char* _desc) {m_desc.Set(_desc); UpdateNameAndCmds();}
	bool IsEmpty() {return m_empty;}
	bool IsToggle() {return *m_desc.Get() == '#';}
	void SetToggle(bool _toggle);
	const char* GetName() {return m_name.Get();}
	void SetName(const char* _name) {m_name.Set(_name); UpdateFromCmd();}
	const char* GetStepName(int _performState = -1);
	int GetCmdSize() {return m_cmds.GetSize();}
	const char* GetCmd(int _i) {return m_cmds.Get(_i)->Get();}
	void SetCmd(WDL_FastString* _cmd, const char* _newCmd);
	WDL_FastString* AddCmd(const char* _cmd, int _pos = -1);
	void InsertCmd(int _pos, WDL_FastString* _cmd) {m_cmds.Insert(_pos, _cmd); UpdateFromCmd();}
	void RemoveCmd(WDL_FastString* _cmd, bool _wantDelete=false){m_cmds.Delete(m_cmds.Find(_cmd), _wantDelete); UpdateFromCmd();}
	WDL_FastString* GetCmdString(int _i) {return m_cmds.Get(_i);}
	int FindCmd(WDL_FastString* _cmd) {return m_cmds.Find(_cmd);}

//JFB TODO? protected?
	WDL_FastString m_desc; 
	int m_performState;
	bool m_added;

private:
	void UpdateNameAndCmds();
	void UpdateFromCmd();
	
	WDL_FastString m_name;
	bool m_empty;
	WDL_PtrList_DeleteOnDestroy<WDL_FastString> m_cmds;
};


class SNM_CyclactionWnd : public SWS_DockWnd
{
public:
	SNM_CyclactionWnd();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	void Update(bool _updateListViews=true);
	void UpdateSection(int _newSection);
	bool IsConsolidatedUndo() { return m_btnUndo.GetCheckState()==1; }
	void SetType(int _section) { m_cbSection.SetCurSel(_section); UpdateSection(_section); }
protected:
	void OnInitDlg();
	void OnDestroy();
	void OnResize();
	HMENU OnContextMenu(int x, int y, bool* wantDefaultItems);
	void AddImportExportMenu(HMENU _menu);
	int OnKey(MSG* msg, int iKeyState);
	void DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight = NULL);

	WDL_VirtualComboBox m_cbSection;
	WDL_VirtualIconButton m_btnUndo;
	WDL_VirtualStaticText m_txtSection;
	SNM_ToolbarButton m_btnApply, m_btnCancel, m_btnImpExp, m_btnActionList;
	SNM_TwoTinyButtons m_tinyLRbtns;
	SNM_TinyLeftButton m_btnLeft;
	SNM_TinyRightButton m_btnRight;
};


class SNM_CyclactionsView : public SWS_ListView
{
public:
	SNM_CyclactionsView(HWND hwndList, HWND hwndEdit);
protected:
	void GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax);
	void SetItemText(SWS_ListItem* item, int iCol, const char* str);
	bool IsEditListItemAllowed(SWS_ListItem* item, int iCol);
	void GetItemList(SWS_ListItemList* pList);
	void OnItemSelChanged(SWS_ListItem* item, int iState);
	void OnItemBtnClk(SWS_ListItem* item, int iCol, int iKeyState);
	void OnItemDblClk(SWS_ListItem* item, int iCol);
};


class SNM_CommandsView : public SWS_ListView
{
public:
	SNM_CommandsView(HWND hwndList, HWND hwndEdit);
	void OnDrag();
protected:
	void GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax);
	void SetItemText(SWS_ListItem* item, int iCol, const char* str);
	bool IsEditListItemAllowed(SWS_ListItem* item, int iCol);
	void GetItemList(SWS_ListItemList* pList);
	int OnItemSort(SWS_ListItem* _item1, SWS_ListItem* _item2); 
	void OnBeginDrag(SWS_ListItem* item);
	void OnItemSelChanged(SWS_ListItem* item, int iState);
};

int RegisterCyclation(const char* _name, bool _toggle, int _type, int _cycleId, int _cmdId);
int CyclactionInit();
void CyclactionExit();
void OpenCyclactionView(COMMAND_T*);
bool IsCyclactionViewDisplayed(COMMAND_T*);

#endif
