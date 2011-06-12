/******************************************************************************
/ SnM_FXChainView.h
/ JFB TODO? now, SnM_Resources.cpp/.h would be better names..
/
/ Copyright (c) 2009-2011 Tim Payne (SWS), Jeffos
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

// used if more than NB_SLOTS_FAST_LISTVIEW slots are loaded
class SNM_FastResourceView : public SNM_ResourceView {
public:
	SNM_FastResourceView(HWND hwndList, HWND hwndEdit) : SNM_ResourceView(hwndList, hwndEdit) {}
	virtual void Update();
};

class SNM_ResourceWnd : public SWS_DockWnd
{
public:
	SNM_ResourceWnd();
	void SetType(int _type);
	void Update();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	void SelectBySlot(int _slot);

protected:
	void OnInitDlg();
	HMENU OnContextMenu(int x, int y);
	void OnDestroy();
	int OnKey(MSG* msg, int iKeyState);
	int GetValidDroppedFilesCount(HDROP _h);
	void OnDroppedFiles(HDROP _h);
	void DrawControls(LICE_IBitmap* _bm, RECT* _r);
	int OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);

	void FillDblClickTypeCombo();
	void AddSlot(bool _update);
	void InsertAtSelectedSlot(bool _update);
	void DeleteSelectedSlots(bool _update, bool _delFiles=false);
	void AutoSaveSlots(int _slotPos);

	int m_previousType;
	bool m_autoSaveTrTmpltWithItemsPref;
	int m_autoSaveFXChainPref;
	int m_lastThemeBrushColor;

	// WDL UI
	WDL_VWnd_Painter m_vwnd_painter;
	WDL_VWnd m_parentVwnd; // owns all children windows
	WDL_VirtualComboBox m_cbType, m_cbDblClickType, m_cbDblClickTo;
	WDL_VirtualIconButton m_btnAutoSave;
	WDL_VirtualStaticText m_txtDblClickType, m_txtDblClickTo;
};


class PathSlotItem {
public:
	PathSlotItem(const char* _shortPath="", const char* _desc="") : m_shortPath(_shortPath), m_desc(_desc) {}
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
		if (_filter) {
			_snprintf(_filter, _maxFilterLength, "REAPER %s (*.%s)X*.%s", m_desc.Get(), m_ext.Get(), m_ext.Get());
			// special code for multiple null terminated strings ('X' -> '\0')
			char* p = strchr(_filter, ')');
			if (p) *(p+1) = '\0';
		}
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
		if (_slot >=0 && _slot < GetSize()) 
			item = Insert(_slot, new PathSlotItem(shortPath, _desc));
		else 
			item = Add(new PathSlotItem(shortPath, _desc));
		return item;
	}
	int FindByResFulltPath(const char* _resFullPath) {
		if (_resFullPath) {
			PathSlotItem* item;
			for (int i=0; i < GetSize(); i++) {
				item = Get(i);
				if (item && item->m_shortPath.GetLength() && stristr(_resFullPath, item->m_shortPath.Get()))
					return i;
			}
		}
		return -1;
	}
	bool GetFullPath(int _slot, char* _fullFn, int _fullFnSz) {
		PathSlotItem* item = Get(_slot);
		if (item) {
			GetFullResourcePath(m_resDir.Get(), item->m_shortPath.Get(), _fullFn, _fullFnSz);
			return true;
		}
		return false;
	};
	bool SetFromFullPath(int _slot, const char* _fullPath)	{
		PathSlotItem* item = Get(_slot);
		if (item) {
			char shortPath[BUFFER_SIZE] = "";
			GetShortResourcePath(m_resDir.Get(), _fullPath, shortPath, BUFFER_SIZE);
			item->m_shortPath.Set(shortPath);
			return true;
		}
		return false;
	};
	int PromptForSlot(const char* _title);
	bool GetOrBrowseSlot(int _slot, char* _fn, int _fnSz, bool _errMsg=false);
	bool BrowseSlot(int _slot, char* _fn=NULL, int _fnSz=0);
	void EditSlot(int _slot);
	void ClearSlot(int _slot, bool _guiUpdate=true);
	void ClearSlotPrompt(COMMAND_T* _ct);
	const char* GetDesc() {return m_desc.Get();}
	const char* GetFileExt() {return m_ext.Get();}
	const char* GetResourceDir() {return m_resDir.Get();}

private:
	int m_type;
	WDL_String m_resDir; // Resource sub-directory name *AND* S&M.ini section
	WDL_String m_desc; // used in user messages
	WDL_String m_ext; // file extension w/o '.' (ex: "rfxchain")
};

//JFB
extern FileSlotList g_fxChainFiles;
extern FileSlotList g_trTemplateFiles;
extern FileSlotList g_prjTemplateFiles;

#endif