/******************************************************************************
/ SnM_Cyclactions.h
/
/ Copyright (c) 2011 Jeffos, Tim Payne (SWS)
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

#define EMPTY_CYCLACTION		"no-op,65535"
#define MAX_CYCLATION_LEN		4096

class Cyclaction
{
public:
	// constructors suppose params are valid
	Cyclaction(const char* _desc, bool _added=false) : m_desc(_desc), m_performState(0), m_added(_added), m_empty(false) {UpdateNameAndCmds();}
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
	void SetCmd(WDL_String* _cmd, const char* _newCmd);
	WDL_String* AddCmd(const char* _cmd);
	void InsertCmd(int _pos, WDL_String* _cmd) {m_cmds.Insert(_pos, _cmd); UpdateFromCmd();}
	void RemoveCmd(WDL_String* _cmd, bool _wantDelete=false){m_cmds.Delete(m_cmds.Find(_cmd), _wantDelete); UpdateFromCmd();}
	WDL_String* GetCmdString(int _i) {return m_cmds.Get(_i);}
	int FindCmd(WDL_String* _cmd) {return m_cmds.Find(_cmd);}
	
	WDL_String m_desc; 
	int m_performState;
	bool m_added;

private:
	void UpdateNameAndCmds();
	void UpdateFromCmd();
	
	WDL_String m_name;
	bool m_empty;
	WDL_PtrList_DeleteOnDestroy<WDL_String> m_cmds;
};

class SNM_CyclactionsView : public SWS_ListView
{
public:
	SNM_CyclactionsView(HWND hwndList, HWND hwndEdit);
protected:
	void GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax);
	void SetItemText(SWS_ListItem* item, int iCol, const char* str);
	void GetItemList(SWS_ListItemList* pList);
	void OnItemSelChanged(SWS_ListItem* item, int iState);
	void OnItemBtnClk(SWS_ListItem* item, int iCol, int iKeyState);
};

class SNM_CommandsView : public SWS_ListView
{
public:
	SNM_CommandsView(HWND hwndList, HWND hwndEdit);
	void OnDrag();
protected:
	void GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax);
	void SetItemText(SWS_ListItem* item, int iCol, const char* str);
	void GetItemList(SWS_ListItemList* pList);
	int OnItemSort(SWS_ListItem* _item1, SWS_ListItem* _item2); 
	void OnBeginDrag(SWS_ListItem* item);
	void OnItemSelChanged(SWS_ListItem* item, int iState);
};
