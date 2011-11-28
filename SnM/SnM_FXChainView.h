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


enum {
  SNM_SLOT_FXC=0,
  SNM_SLOT_TR,
  SNM_SLOT_PRJ,
  SNM_SLOT_MEDIA
  // etc..
#ifdef _WIN32
  // keep this one as the last one (win only)
  ,SNM_SLOT_THM
#endif
};


class PathSlotItem {
public:
	PathSlotItem(const char* _shortPath="", const char* _comment="") : m_shortPath(_shortPath), m_comment(_comment) {}
	bool IsDefault() {return (!m_shortPath.GetLength());}
	void Clear() {m_shortPath.Set(""); m_comment.Set("");}
	WDL_FastString m_shortPath, m_comment;
};


class FileSlotList : public WDL_PtrList<PathSlotItem>
{
  public:
	FileSlotList(int _type, const char* _resDir, const char* _desc, const char* _ext, bool _notepad, bool _autoSave, bool _dlClick) 
		: m_type(_type), m_resDir(_resDir),m_desc(_desc),m_ext(_ext),m_notepad(_notepad),m_autoSave(_autoSave),m_dlClick(_dlClick),
		WDL_PtrList<PathSlotItem>() {}
	int GetType() {return m_type;}
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
	bool GetOrBrowseSlot(int _slot, char* _fn, int _fnSz, bool _errMsg=false);
	WDL_FastString* GetOrPromptOrBrowseSlot(const char* _title, int _slot);
	bool BrowseSlot(int _slot, char* _fn=NULL, int _fnSz=0);
	void EditSlot(int _slot);
	void ClearSlot(int _slot, bool _guiUpdate=true);
	void ClearSlotPrompt(COMMAND_T* _ct);
	const char* GetResourceDir(bool _dir=true) { 
		const char* p = m_resDir.Get();
		if (!_dir) {
			const char* p2 = strrchr(p, PATH_SLASH_CHAR);
			if (p2 && *(p2+1)) p = p2+1;
		}
		return p;
	}
	const char* GetDesc() {return m_desc.Get();}
	const char* GetMenuDesc() {
		if (!m_menuDesc.GetLength()) {
			m_menuDesc.SetFormatted(m_desc.GetLength()+8, "%ss", m_desc.Get()); // add trailing 's'
			char* p = (char*)m_menuDesc.Get();
			*p = toupper(*p); // 1st char to upper
		}
		return m_menuDesc.Get();
	}
	const char* GetFileExt() {return m_ext.Get();}
	bool IsValidFileExt(const char* _ext) {
		if (!_ext) return false;
		if (!m_ext.GetLength())
			return IsMediaExtension(_ext, false);
		return (_stricmp(_ext, m_ext.Get()) == 0);
	}
	void GetFileFilter(char* _filter, int _maxFilterLength) {
		if (!_filter) return;
		if (m_ext.GetLength()) {
			_snprintf(_filter, _maxFilterLength, "REAPER %s (*.%s)X*.%s", m_desc.Get(), m_ext.Get(), m_ext.Get());
			// special code for multiple null terminated strings ('X' -> '\0')
			if (char* p = strchr(_filter, ')')) *(p+1) = '\0';
		}
		else memcpy(_filter, plugin_getFilterList(), _maxFilterLength); // memcpy because of '\0'
	}
	bool HasNotepad() {return m_notepad;}
	bool HasAutoSave() {return m_autoSave;}
	bool HasDblClick() {return m_dlClick;}
protected:
	int m_type;
	WDL_FastString m_resDir;	// resource sub-directory name and S&M.ini section/key names
	WDL_FastString m_desc;		// used in user messages and in main dropdown box menu items
	WDL_FastString m_ext;		// file extension w/o '.' (ex: "rfxchain"), "" means all supported media files
	WDL_FastString m_menuDesc;	// deduced from m_desc (lazy init)
	bool m_notepad, m_autoSave, m_dlClick;
};


//JFB
extern WDL_PtrList<FileSlotList> g_slots;
extern int g_prjLoaderStartPref;
extern int g_prjLoaderEndPref;


class SNM_ResourceView : public SWS_ListView
{
public:
	SNM_ResourceView(HWND hwndList, HWND hwndEdit);
protected:
	void GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax);
	bool IsEditListItemAllowed(SWS_ListItem* item, int iCol);
	void SetItemText(SWS_ListItem* item, int iCol, const char* str);
	void OnItemDblClk(SWS_ListItem* item, int iCol);
	void GetItemList(SWS_ListItemList* pList);
	void OnBeginDrag(SWS_ListItem* item);
};


class SNM_ResourceWnd : public SNM_DockWnd
{
public:
	SNM_ResourceWnd();
	void SetType(int _type);
	void Update();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	void ClearListSelection();
	void SelectBySlot(int _slot1, int _slot2 = -1);
protected:
	INT_PTR WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void OnInitDlg();
	HMENU OnContextMenu(int x, int y);
	void OnDestroy();
	int OnKey(MSG* msg, int iKeyState);
	int GetValidDroppedFilesCount(HDROP _h);
	void OnDroppedFiles(HDROP _h);
	void DrawControls(LICE_IBitmap* _bm, RECT* _r);
	HBRUSH ColorEdit(HWND _hwnd, HDC _hdc);

	void FillDblClickTypeCombo();
	void AddSlot(bool _update);
	void InsertAtSelectedSlot(bool _update);
	void ClearDeleteSelectedSlots(int _mode, bool _update);
	void AutoSave();
	void AutoFill(const char* _startPath);

	bool m_autoSaveTrTmpltWithItemsPref;
	int m_previousType, m_autoSaveFXChainPref;

	WDL_VirtualComboBox m_cbType, m_cbDblClickType, m_cbDblClickTo;
	WDL_VirtualIconButton m_btnAutoSave;
	WDL_VirtualStaticText m_txtDblClickType, m_txtDblClickTo;
};


#endif