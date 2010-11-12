/******************************************************************************
/ SnM_FXChainView.h
/ JFB TODO: now, a better name would be "SnM_ResourceView.h"
/
/ Copyright (c) 2009-2010 Tim Payne (SWS), Jeffos
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

#ifndef _SNM_RESVIEW_H_
#define _SNM_RESVIEW_H_

#ifdef _SNM_ITT
#define SNM_FILESLOT_MAX_ITEMTK_PROPS	12
#endif

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
	int GetValidDroppedFilesCount(HDROP _h);
	void OnDroppedFiles(HDROP _h);
	int OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);

	void FillDblClickTypeCombo();
	void AddSlot(bool _update);
	void InsertAtSelectedSlot(bool _update);
	void DeleteSelectedSlots(bool _update);

	int m_previousType;

	// WDL UI
	WDL_VWnd_Painter m_vwnd_painter;
	WDL_VWnd m_parentVwnd; // owns all children windows
	SNM_VirtualComboBox m_cbType; // common to all 
	SNM_VirtualComboBox m_cbDblClickType; // FX chains & Track templates
	SNM_VirtualComboBox m_cbDblClickTo; // FX chains only
#ifdef _SNM_ITT
	WDL_VirtualIconButton m_btnItemTakeDetails;
	WDL_VirtualIconButton m_btnItemTakeProp[SNM_FILESLOT_MAX_ITEMTK_PROPS];
#endif
};


class PathSlotItem {
public:
	PathSlotItem(const char* _shortPath, const char* _desc) : m_shortPath(_shortPath), m_desc(_desc) {}
	bool IsDefault() {return (!m_shortPath.GetLength());}
	void Clear() {m_shortPath.Set(""); m_desc.Set("");}
	WDL_String m_shortPath, m_desc; 
};


class FileSlotList : public WDL_PtrList_DeleteOnDestroy<PathSlotItem>
{
  public:
	FileSlotList(int _type, const char* _resDir, const char* _desc, const char* _ext) 
		: m_type(_type), m_resDir(_resDir),m_desc(_desc),m_ext(_ext),WDL_PtrList_DeleteOnDestroy<PathSlotItem>() {}
	void GetFileFilter(char* _filter, int _maxFilterLength) {
		if (_filter) _snprintf(_filter, _maxFilterLength, "REAPER %s (*.%s)\0*.%s\0", m_desc.Get(), m_ext.Get(), m_ext.Get());
	}
	// _path: short resource path or full path
	PathSlotItem* AddSlot(const char* _path="", const char* _desc="") {
		char shortPath[BUFFER_SIZE] = "";
		GetShortResourcePath(m_resDir.Get(), _path, shortPath, BUFFER_SIZE);
		return Add(new PathSlotItem(shortPath, _desc));
	}
	// _path: short resource path or full path
	PathSlotItem* InsertSlot(int _slot, const char* _path="", const char* _desc="") {
		PathSlotItem* item = NULL;
		char shortPath[BUFFER_SIZE] = "";
		GetShortResourcePath(m_resDir.Get(), _path, shortPath, BUFFER_SIZE);
		if (_slot >=0 && _slot < GetSize()) {
			item = Insert(_slot, new PathSlotItem(shortPath, _desc));
		} 
		else
			item = AddSlot(shortPath, _desc);
		return item;
	}
	int FindByFullPath(const char* _fullPath) {
		int slot = -1;
		if (_fullPath)
			for (int i=0; slot < 0 && i < GetSize(); i++)
			{
				char* fullPath = GetFullPath(i);
				if (fullPath && !_stricmp(fullPath, _fullPath))
					slot = i;
			}
		return slot;
	}
	// *always* returns a full path ("" if an error occured)
	char* GetFullPath(int _slot) //JFB2000 !!!! bof bof
	{
		m_tmp.Set("");
		PathSlotItem* item = Get(_slot);
		if (item) {
			char fullPath[BUFFER_SIZE] = "";
			GetFullResourcePath(m_resDir.Get(), item->m_shortPath.Get(), fullPath, BUFFER_SIZE);
			m_tmp.Set(fullPath);
		}
		return m_tmp.Get();
	};
	void SetFromFullPath(int _slot, const char* _fullPath)	{
		PathSlotItem* item = Get(_slot);
		if (item) {
			char shortPath[BUFFER_SIZE] = "";
			GetShortResourcePath(m_resDir.Get(), _fullPath, shortPath, BUFFER_SIZE);
			item->m_shortPath.Set(shortPath);
		}
	};
	int PromptForSlot(const char* _title);
	bool LoadOrBrowseSlot(int _slot, bool _errMsg=false);
	bool BrowseStoreSlot(int _slot);
	void DisplaySlot(int _slot);
	void ClearSlot(int _slot, bool _guiUpdate=true);
	const char* GetDesc() {return m_desc.Get();}

	int m_type;
	WDL_String m_resDir; // Resource sub-directory name *AND* S&M.ini section
	WDL_String m_desc; // used in user messages
	WDL_String m_ext; // e.g. "rfxchain"
	WDL_String m_tmp;  //JFB2000 !!!! bof bof
};

//JFB!!!
extern FileSlotList g_fxChainFiles;
extern FileSlotList g_trTemplateFiles;
#ifdef _SNM_ITT
extern FileSlotList g_itemTemplateFiles;
#endif

#endif