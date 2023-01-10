/******************************************************************************
/ SnM_Cyclactions.h
/
/ Copyright (c) 2011 and later Jeffos
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

//#pragma once

#ifndef _SNM_CYCLACTION_H_
#define _SNM_CYCLACTION_H_

#include "SnM_VWnd.h"


#define CA_VERSION		3
#define CA_MAX_LEN		SNM_MAX_CHUNK_LINE_LENGTH
#define CA_SEP_V1		',' // deprecated: prevented console cmd support
#define CA_SEP_V2		'µ' // deprecated: UTF8 on OSX
#define CA_SEP			'|'
#define CA_EMPTY		"no-op|65535" // use the above CA_SEP separator!
#define CA_TGL1			'#' // CA reports a fake toggle state
#define CA_TGL2			'$' // CA reports a real toggle state

static const char s_CA_SEP_STR[] =  { CA_SEP,  '\0' };
static const char s_CA_TGL1_STR[] = { CA_TGL1, '\0' };
static const char s_CA_TGL2_STR[] = { CA_TGL2, '\0' };


class Cyclaction
{
public:
	// constructors assume their params are valid
	Cyclaction(const char* _def=CA_EMPTY, bool _added=false) : m_def(_def), m_performState(0), m_fakeToggle(false), m_cmdId(0), m_added(_added) { UpdateNameAndCmds(); }
	Cyclaction(Cyclaction* _a) : m_def(_a->m_def), m_performState(_a->m_performState), m_fakeToggle(_a->m_fakeToggle), m_cmdId(_a->m_cmdId), m_added(_a->m_added) { UpdateNameAndCmds(); }
	~Cyclaction() {}
	const char* GetDefinition() { return m_def.Get(); }
	void Update(const char* _def) { m_def.Set(_def); UpdateNameAndCmds(); }
	int IsToggle() { return *m_def.Get()==CA_TGL1 ? 1 : *m_def.Get()==CA_TGL2 ? 2 : 0; }
	void SetToggle(int _toggle);
	bool IsEmpty() { return !strcmp(m_def.Get(), CA_EMPTY); }
	const char* GetName() { return m_name.Get(); }
	void SetName(const char* _name) { m_name.Set(_name); UpdateFromCmd(); }
	int GetStepCount();
	int GetStepIdx(int _performState = -1);
	const char* GetStepName(int _performState = -1);
	int GetCmdSize() { return m_cmds.GetSize(); }
	const char* GetCmd(int _i) { return m_cmds.Get(_i) ? m_cmds.Get(_i)->Get() : ""; }
	void SetCmd(WDL_FastString* _cmd, const char* _newCmd);
	WDL_FastString* AddCmd(const char* _cmd, int _pos = -1);
	void InsertCmd(int _pos, WDL_FastString* _cmd) { m_cmds.Insert(_pos, _cmd); UpdateFromCmd(); }
	void RemoveCmd(WDL_FastString* _cmd, bool _wantDelete=false);
	void ReplaceCmd(WDL_FastString* _cmd, bool _wantDelete=false, WDL_PtrList<WDL_FastString>* _newCmds = NULL);
	WDL_FastString* GetCmdString(int _i) { return m_cmds.Get(_i); }
	int FindCmd(WDL_FastString* _cmd) { return m_cmds.Find(_cmd); }
	int GetIndent(WDL_FastString* _cmd);

	int m_performState;
	bool m_added; // CA added by the user, not yet registered
	int m_cmdId;  // valid once the CA is registered (m_added can be false though, e.g. invalid CA)
	bool m_fakeToggle;

private:
	void UpdateNameAndCmds();
	void UpdateFromCmd();

	WDL_FastString m_def;
	WDL_FastString m_name;
	WDL_PtrList_DeleteOnDestroy<WDL_FastString> m_cmds;
};


class CyclactionWnd : public SWS_DockWnd
{
public:
	CyclactionWnd();
	virtual ~CyclactionWnd();

	void OnCommand(WPARAM wParam, LPARAM lParam);
	void Update(bool _updateListViews=true);
	void UpdateSection(int _newSection);
	void SetType(int _section) { m_cbSection.SetCurSel(_section); UpdateSection(_section); }
protected:
	void OnInitDlg();
	void OnDestroy();
	void OnResize();
	HMENU OnContextMenu(int x, int y, bool* wantDefaultItems);
	void AddImportExportMenu(HMENU _menu, bool _wantReset);
	void AddResetMenu(HMENU _menu);
	int OnKey(MSG* msg, int iKeyState);
	void DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight = NULL);
	virtual void GetMinSize(int* _w, int* _h) { *_w=180; *_h=140; }

	WDL_VirtualComboBox m_cbSection;
	WDL_VirtualIconButton m_btnUndo;
	WDL_VirtualIconButton m_btnPreventUIRefresh;
	WDL_VirtualStaticText m_txtSection;
	SNM_ToolbarButton m_btnApply, m_btnCancel, m_btnImpExp, m_btnActionList;
	SNM_TwoTinyButtons m_tinyLRbtns;
	SNM_TinyLeftButton m_btnLeft;
	SNM_TinyRightButton m_btnRight;
};


class CyclactionsView : public SWS_ListView
{
public:
	CyclactionsView(HWND hwndList, HWND hwndEdit);
protected:
	void GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax);
	void SetItemText(SWS_ListItem* item, int iCol, const char* str);
	bool IsEditListItemAllowed(SWS_ListItem* item, int iCol);
	void GetItemList(SWS_ListItemList* pList);
	void OnItemSelChanged(SWS_ListItem* item, int iState);
	void OnItemBtnClk(SWS_ListItem* item, int iCol, int iKeyState);
	void OnItemDblClk(SWS_ListItem* item, int iCol);
};


class CommandsView : public SWS_ListView
{
public:
	CommandsView(HWND hwndList, HWND hwndEdit);
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


Cyclaction* GetCyclactionFromCustomId(int _section, const char* _cmdStr);
int ExplodeCmd(int _section, const char* _cmdStr, WDL_PtrList<WDL_FastString>* _cmds, WDL_PtrList<WDL_FastString>* _macros, WDL_PtrList<WDL_FastString>* _consoles, int _flags);
int ExplodeMacro(int _section, const char* _cmdStr, WDL_PtrList<WDL_FastString>* _cmds, WDL_PtrList<WDL_FastString>* _macros, WDL_PtrList<WDL_FastString>* _consoles, int _flags);
int ExplodeCyclaction(int _section, const char* _cmdStr, WDL_PtrList<WDL_FastString>* _cmds, WDL_PtrList<WDL_FastString>* _macros, WDL_PtrList<WDL_FastString>* _consoles, int _flags, Cyclaction* _action = NULL);
int ExplodeConsoleAction(int _section, const char* _cmdStr, WDL_PtrList<WDL_FastString>* _cmds, WDL_PtrList<WDL_FastString>* _macros, WDL_PtrList<WDL_FastString>* _consoles, int _flags);

int RegisterCyclation(const char* _name, int _type, int _cycleId, int _cmdId);

void CAsInit();
int CyclactionInit();
void CyclactionExit();
void OpenCyclaction(COMMAND_T*);
int IsCyclactionDisplayed(COMMAND_T*);

#endif
