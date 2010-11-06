/******************************************************************************
/ SnM_FXChainView.h
/ JFB TODO: now, a better name would be "SnM_ResourceView.h"
/
/ Copyright (c) 2009-2010 Tim Payne (SWS), JF Bédague
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

#ifndef _SNM_ResourceView_H_
#define _SNM_ResourceView_H_


class SNM_ResourceView : public SWS_ListView
{
public:
	SNM_ResourceView(HWND hwndList, HWND hwndEdit);

protected:
	void GetItemText(LPARAM item, int iCol, char* str, int iStrMax);
	void SetItemText(LPARAM item, int iCol, const char* str);
	void OnItemDblClk(LPARAM item, int iCol);
	void GetItemList(WDL_TypedBuf<LPARAM>* pBuf);
	void OnBeginDrag(LPARAM item);
};

class SNM_ResourceWnd : public SWS_DockWnd
{
public:
	SNM_ResourceWnd();
	void SetType(int _type);

	void Update();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	void SelectBySlot(int _slot);

	WDL_String m_filter;

protected:
	void OnInitDlg();
	HMENU OnContextMenu(int x, int y);
	void OnDestroy();
	int OnKey(MSG* msg, int iKeyState);
	void OnDroppedFiles(HDROP h);
	int OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);

	void FillDblClickTypeCombo();
	void AddSlot(bool _update);
	void InsertAtSelectedSlot(bool _update);
	void DeleteSelectedSlots(bool _update);

	int m_previousType;

	// WDL UI
	WDL_VWnd_Painter m_vwnd_painter;
	WDL_VWnd m_parentVwnd; // owns all children windows
	SNM_VirtualComboBox m_cbType;

	// FX chains
	SNM_VirtualComboBox m_cbDblClickType;
	SNM_VirtualComboBox m_cbDblClickTo;
};


class PathSlotItem
{
public:
	PathSlotItem(const char* _resDir, int _slot, const char* _fullPath, const char* _desc)
	{
		m_resDir.Set(_resDir);
		m_slot = _slot;
		m_desc.Set(_desc);
		SetFullPath(_fullPath);
	}
	bool IsDefault(){return (!m_fullPath.GetLength());}
	void Clear() {m_fullPath.Set(""); m_name.Set(""); m_shortPath.Set(""); m_desc.Set("");}
	void SetFullPath(const char* _fullPath) 
	{
		if (_fullPath && *_fullPath)
		{
			m_fullPath.Set(_fullPath);
			char buf[BUFFER_SIZE];	
			ExtractFileNameEx(_fullPath, buf, true); //JFB TODO: Xen code a revoir
			m_name.Set(buf);
			if (GetShortResourcePath(m_resDir.Get(), _fullPath, buf, BUFFER_SIZE)) 
			{
				m_shortPath.Set(buf);
				return; // <--- !
			}
		}
		m_fullPath.Set(""); m_name.Set(""); m_shortPath.Set("");
	}
	int m_slot; // in case, we want discontinuous slots at some point..
	WDL_String m_resDir, m_fullPath, m_desc, m_name, m_shortPath; // The 2 last ones are deduced from m_fullPath, added for perf. reasons
};

class FileSlotList : public WDL_PtrList_DeleteOnDestroy<PathSlotItem>
{
  public:
	FileSlotList(int _type, const char* _resDir, const char* _desc, const char* _ext) 
		: m_type(_type), m_resDir(_resDir),m_desc(_desc),m_ext(_ext),WDL_PtrList_DeleteOnDestroy<PathSlotItem>() {}
	void GetFileFilter(char* _filter, int _maxFilterLength) {
		if (_filter) _snprintf(_filter, _maxFilterLength, "REAPER %s (*.%s)\0*.%s\0", m_desc.Get(), m_ext.Get(), m_ext.Get());
	}
	int PromptForSlot(const char* _title);
	void ClearSlot(int _slot, bool _guiUpdate=true);
	bool CheckAndStoreSlot(int _slot, const char* _filename, bool _errMsg=false, bool _acceptEmpty=false);
	bool BrowseStoreSlot(int _slot);
	const char* GetDesc() {return m_desc.Get();}
	bool LoadOrBrowseSlot(int _slot, bool _errMsg=false);
	void DisplaySlot(int _slot);
	PathSlotItem* NewSlotItem(int _slot, const char* _fullPath, const char* _desc) {return new PathSlotItem(m_resDir.Get(), _slot, _fullPath, _desc);}
	PathSlotItem* AddEmptySlot() {return Add(NewSlotItem(GetSize(), "", ""));}
	PathSlotItem* InsertEmptySlot(int _slot) {
		PathSlotItem* item = NULL;
		if (_slot >=0 && _slot < GetSize()) {
			item = Insert(_slot, NewSlotItem(_slot, "", ""));
			for (int i=_slot+1; i < GetSize(); i++)
				Get(i)->m_slot++;
		} 
		else if (_slot == GetSize()) 
			AddEmptySlot();
		return item;
	}
	void DeleteSlot(int _slot, bool _wantDelete=true) {
		Delete(_slot, _wantDelete);
		for (int i=_slot; i < GetSize(); i++)
			Get(i)->m_slot--;
	}
	int m_type;
	WDL_String m_resDir; // Resource sub-directory name *AND* S&M.ini section
	WDL_String m_desc; // used in user messages
	WDL_String m_ext; // e.g. "rfxchain"
};

//JFB!!!
extern FileSlotList g_fxChainFiles;
extern FileSlotList g_trTemplateFiles;

#endif