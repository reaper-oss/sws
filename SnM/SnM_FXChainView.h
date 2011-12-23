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
  SNM_SLOT_MEDIA,
  SNM_SLOT_IMG,
  // etc..
#ifdef _WIN32
  SNM_SLOT_THM,
#endif
  SNM_NUM_DEFAULT_SLOTS
};

enum {
  FXC_AUTOSAVE_PREF_TRACK=0,
  FXC_AUTOSAVE_PREF_INPUT_FX,
  FXC_AUTOSAVE_PREF_ITEM
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
	FileSlotList(const char* _resDir, const char* _desc, const char* _ext, bool _notepad, bool _autoSave, bool _dblClick) 
		: m_resDir(_resDir),m_desc(_desc),m_ext(_ext),m_notepad(_notepad),m_autoSave(_autoSave),m_dblClick(_dblClick),
		WDL_PtrList<PathSlotItem>() {}
	~FileSlotList() {}

	PathSlotItem* AddSlot(const char* _path="", const char* _desc="");
	PathSlotItem* InsertSlot(int _slot, const char* _path="", const char* _desc="");
	int FindByResFulltPath(const char* _resFullPath);
	bool GetFullPath(int _slot, char* _fullFn, int _fullFnSz);
	bool SetFromFullPath(int _slot, const char* _fullPath);
	bool GetOrBrowseSlot(int _slot, char* _fn, int _fnSz, bool _errMsg=false);
	WDL_FastString* GetOrPromptOrBrowseSlot(const char* _title, int _slot);
	bool BrowseSlot(int _slot, char* _fn=NULL, int _fnSz=0);
	void EditSlot(int _slot);
	void ClearSlot(int _slot, bool _guiUpdate=true);
	void ClearSlotPrompt(COMMAND_T* _ct);
	const char* GetResourceDir() {  return m_resDir.Get(); }
	const char* GetDesc() { return m_desc.Get(); }
	const char* GetMenuDesc();
	const char* GetFileExt() { return m_ext.Get(); }
	bool IsValidFileExt(const char* _ext);
	void GetFileFilter(char* _filter, int _maxFilterLength);
	bool HasNotepad() { return m_notepad; }
	bool HasAutoSave() { return m_autoSave; }
	bool HasDblClick() { return m_dblClick; }
	void SetNotepad(bool _notepad) { m_notepad=_notepad; }
	void SetAutoSave(bool _autoSave) { m_autoSave=_autoSave; }
	void SetDblClick(bool _dblClick) { m_dblClick=_dblClick; }
protected:
	WDL_FastString m_resDir;	// resource sub-directory name and S&M.ini section/key names
	WDL_FastString m_desc;		// used in user messages and in main dropdown box menu items
	WDL_FastString m_ext;		// file extension w/o '.' (ex: "rfxchain"), "" means all supported media files
	WDL_FastString m_menuDesc;	// deduced from m_desc (lazy init)
	bool m_notepad, m_autoSave, m_dblClick;
};


extern WDL_PtrList<FileSlotList> g_slots;
extern int g_tiedSlotActions[SNM_NUM_DEFAULT_SLOTS];
extern int g_prjLoaderStartPref;
extern int g_prjLoaderEndPref;


class SNM_ResourceView : public SWS_ListView
{
public:
	SNM_ResourceView(HWND hwndList, HWND hwndEdit);
	void Perform();
protected:
	void GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax);
	bool IsEditListItemAllowed(SWS_ListItem* item, int iCol);
	void SetItemText(SWS_ListItem* item, int iCol, const char* str);
	void OnItemDblClk(SWS_ListItem* item, int iCol);
	void GetItemList(SWS_ListItemList* pList);
	void OnBeginDrag(SWS_ListItem* item);
};


class SNM_ResourceWnd : public SWS_DockWnd
{
public:
	SNM_ResourceWnd();
	void SetType(int _type);
	void Update();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	void ClearListSelection();
	void SelectBySlot(int _slot1, int _slot2 = -1);
protected:
	void OnInitDlg();
	void OnDestroy();
	INT_PTR WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	HMENU OnContextMenu(int x, int y);
	int OnKey(MSG* msg, int iKeyState);
	int GetValidDroppedFilesCount(HDROP _h);
	void OnDroppedFiles(HDROP _h);
	void DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight = NULL);
	bool GetToolTipString(int _xpos, int _ypos, char* _bufOut, int _bufOutSz);
	HBRUSH OnColorEdit(HWND _hwnd, HDC _hdc);

	void FillTypeCombo();
	void FillDblClickCombos();
	void AddSlot(bool _update);
	void InsertAtSelectedSlot(bool _update);
	void ClearDeleteSlots(int _mode, bool _update);

	int m_previousType;

	WDL_VirtualComboBox m_cbType, m_cbDblClickType, m_cbDblClickTo;
	WDL_VirtualIconButton m_btnAutoSave, m_btnTiedActions;
	WDL_VirtualStaticText m_txtDblClickType, m_txtDblClickTo;
	SNM_MiniAddDelButtons m_btnsAddDel;
};


void FlushCustomTypesIniFile();
void AutoSave(int _type, int _whichFXChain, bool _trTmpltWithItems);
void AutoFill(int _type, const char* _startPath);


class SNM_ImageWnd : public SWS_DockWnd
{
public:
	SNM_ImageWnd();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	void SetImage(LICE_IBitmap* _img) { m_img.SetImage(_img); }
	void SetStretch(bool _stretch) { m_stretch = _stretch; }
	bool IsStretched() { return m_stretch; }
	void RequestRedraw() { m_parentVwnd.RequestRedraw(NULL); }
protected:
	void OnInitDlg();
	HMENU OnContextMenu(int x, int y);
	void DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight = NULL);
	SNM_ImageVWnd m_img; bool m_stretch;
};


#endif
