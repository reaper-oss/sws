/******************************************************************************
/ SnM_Resources.h
/
/ Copyright (c) 2009 and later Jeffos
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

#ifndef _SNM_RESOURCES_H_
#define _SNM_RESOURCES_H_

#include "SnM_VWnd.h"


enum {
  SNM_SLOT_FXC=0,
  SNM_SLOT_TR,
  SNM_SLOT_PRJ,
  SNM_SLOT_MEDIA,
  SNM_SLOT_IMG,
  SNM_SLOT_THM,
  // -> add new resource types here..
  SNM_NUM_DEFAULT_SLOTS
};

#define SNM_MAX_SLOT_TYPES	32+SNM_NUM_DEFAULT_SLOTS // 32 bookmarks max

enum {
  FXC_AUTOSAVE_PREF_TRACK=0,
  FXC_AUTOSAVE_PREF_INPUT_FX,
  FXC_AUTOSAVE_PREF_ITEM
};


class ResourceItem {
public:
	ResourceItem(const char* _shortPath="", const char* _comment="") 
		: m_shortPath(_shortPath), m_comment(_comment) {}
	bool IsDefault() { return (!m_shortPath.GetLength()); }
	void Clear() { m_shortPath.Set(""); m_comment.Set(""); }
	WDL_FastString m_shortPath, m_comment;
};


// masks for ResourceList.m_flags
enum {
  SNM_RES_MASK_DBLCLIK=1,
  SNM_RES_MASK_TEXT=2,
  SNM_RES_MASK_AUTOSAVE=4,
  SNM_RES_MASK_AUTOFILL=8
};

class ResourceList : public WDL_PtrList<ResourceItem>
{
  public:
	ResourceList(const char* _resDir, const char* _desc, const char* _ext, int _flags);
	~ResourceList() { m_exts.Empty(true); }
	int GetNonEmptySize() { int cnt=0; for(int i=0; i<GetSize(); i++) if (!Get(i)->IsDefault()) cnt++; return cnt; }  
	ResourceItem* AddSlot(const char* _path="", const char* _desc="");
	ResourceItem* InsertSlot(int _slot, const char* _path="", const char* _desc="");
	int FindByPath(const char* _fullPath);
	bool GetFullPath(int _slot, char* _fullFn, int _fullFnSz);
	bool SetFromFullPath(int _slot, const char* _fullPath);
	bool ClearSlot(int _slot);
	const char* GetResourceDir() {  return m_resDir.Get(); }
	const char* GetName() { return m_name.Get(); }
	void SetName(const char* _desc) { m_name.Set(_desc); }
	const char* GetFileExtStr() { return m_ext.Get(); }
	bool IsValidFileExt(const char* _ext);
	void GetFileFilter(char* _filter, size_t _filterSz, bool _dblNull = true);
	bool IsText()  { return (m_flags & SNM_RES_MASK_TEXT) == SNM_RES_MASK_TEXT; }
	bool IsAutoSave() { return (m_flags & SNM_RES_MASK_AUTOSAVE) == SNM_RES_MASK_AUTOSAVE; }
	bool IsDblClick() { return (m_flags & SNM_RES_MASK_DBLCLIK) == SNM_RES_MASK_DBLCLIK; }
	bool IsAutoFill() { return (m_flags & SNM_RES_MASK_AUTOFILL) == SNM_RES_MASK_AUTOFILL; }
	int GetFlags() { return m_flags; }
	void SetFlags(int _flags) { m_flags=_flags; }
protected:
	WDL_FastString m_resDir;			// resource sub-directory name + S&M.ini section/key names
	WDL_FastString m_name;				// used in user messages, etc..
	WDL_FastString m_ext;				// file extensions w/o '.' (ex: "rfxchain"), "" means all supported media file extensions
	int m_flags;						// see bitmask definition above
private:
	WDL_PtrList<WDL_FastString> m_exts;	// split file extensions
};


class ResourcesView : public SWS_ListView
{
public:
	ResourcesView(HWND hwndList, HWND hwndEdit);
	void Perform(int _what);
protected:
	void GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax);
	bool IsEditListItemAllowed(SWS_ListItem* item, int iCol);
	void SetItemText(SWS_ListItem* item, int iCol, const char* str);
	void OnItemDblClk(SWS_ListItem* item, int iCol);
	void GetItemList(SWS_ListItemList* pList);
	void OnBeginDrag(SWS_ListItem* item);
};


class ResourcesWnd : public SWS_DockWnd
{
public:
	ResourcesWnd();
	virtual ~ResourcesWnd();

	void SetType(int _type);
	int SetType(const char* _name);
	void Update();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	void ClearListSelection();
	void SelectBySlot(int _slot1, int _slot2 = -1, bool _selectOnly = true);
	void GetSelectedSlots(WDL_PtrList<ResourceItem>* _selSlots, WDL_PtrList<ResourceItem>* _selEmptySlots = NULL);
	void FillTypeCombo();
	void FillDblClickCombo();
protected:
	void OnInitDlg();
	void OnDestroy();
	HMENU AutoSaveContextMenu(HMENU _menu, bool _saveItems);
	HMENU AutoFillContextMenu(HMENU _menu, bool _fillItems);
	HMENU AttachPrjContextMenu(HMENU _menu, bool _openSelPrj);
	HMENU BookmarkContextMenu(HMENU _menu);
	HMENU AddMediaOptionsContextMenu(HMENU _menu);
	HMENU OnContextMenu(int x, int y, bool* wantDefaultItems);
	HWND IDC_FILTER_GetFocus(HWND _hwnd, MSG* _msg);
	int OnKey(MSG* msg, int iKeyState);
	int GetValidDroppedFilesCount(HDROP _h);
	void OnDroppedFiles(HDROP _h);
	void DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight = NULL);
	bool GetToolTipString(int _xpos, int _ypos, char* _bufOut, int _bufOutSz);
	virtual void GetMinSize(int* w, int* h) { *w=190; *h=140; }	

	void AddSlot();
	void InsertAtSelectedSlot();

	SNM_VirtualComboBox m_cbType, m_cbDblClickType;
	WDL_VirtualIconButton m_btnAutoFill, m_btnAutoSave, m_btnOffsetTrTemplate;
	WDL_VirtualStaticText m_txtDblClickType, m_txtTiedPrj;
	SNM_TwoTinyButtons m_btnsAddDel;
	SNM_TinyPlusButton m_btnAdd;
	SNM_TinyMinusButton m_btnDel;
};


void AttachResourceFiles();

class AttachResourceFilesJob : public ScheduledJob {
public:
	AttachResourceFilesJob(int _approxMs) : ScheduledJob(SNM_SCHEDJOB_RES_ATTACH, _approxMs) {}
protected:
	void Perform();
};


extern WDL_PtrList_DOD<ResourceList> g_SNM_ResSlots;
extern int g_tiedSlotActions[SNM_NUM_DEFAULT_SLOTS];


bool AutoSaveChunkSlot(const void* _obj, const char* _fn);
bool AutoSaveSlot(int _slotType, const char* _dirPath,
				const char* _name, const char* _ext,
				WDL_PtrList<ResourceItem>* _owSlots, int* _owIdx,
				bool (*SaveSlot)(const void*, const char*)=NULL, const void* _obj=NULL);
void AutoSave(int _type, bool _ow, int _flags = 0);
void AutoFill(int _type);

bool BrowseSlot(int _type, int _slot, bool _tieUntiePrj, char* _fn = NULL, int _fnSz = 0, bool* _updatedList = NULL);
WDL_FastString* GetOrPromptOrBrowseSlot(int _type, int* _slot);

void ClearDeleteSlotsFiles(int _type, int _mode, int _slot = -1);

int AddCustomBookmark(char* _definition);
void NewBookmark(int _type, bool _copyCurrent);
void DeleteBookmark(int _bookmarkType);
void RenameBookmark(int _bookmarkType);

void ResourcesTrackListChange();
void ResourcesUpdate();

int ResourcesInit();
void ResourcesExit();
void OpenResources(COMMAND_T*);
int IsResourcesDisplayed(COMMAND_T*);


class ImageWnd : public SWS_DockWnd
{
public:
	ImageWnd();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	void SetImage(const char* _fn) { m_img.SetImage(_fn); }
	void SetStretch(bool _stretch) { m_stretch = _stretch; }
	bool IsStretched() { return m_stretch; }
	void RequestRedraw() { m_parentVwnd.RequestRedraw(NULL); }
	const char* GetFilename() { return m_img.GetFilename(); }
protected:
	void OnInitDlg();
	HMENU OnContextMenu(int x, int y, bool* wantDefaultItems);
	void DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight = NULL);
	bool GetToolTipString(int _xpos, int _ypos, char* _bufOut, int _bufOutSz);
	SNM_ImageVWnd m_img;
	bool m_stretch;
};


int ImageInit();
void ImageExit();
void OpenImageWnd(COMMAND_T*);
bool OpenImageWnd(const char* _fn);
void ClearImageWnd(COMMAND_T*);
int IsImageWndDisplayed(COMMAND_T*);

void ResourcesDeleteAllSlots(COMMAND_T*);

void ResourcesClearSlotPrompt(COMMAND_T*);
void ResourcesDeleteLastSlot(COMMAND_T*);
void ResourcesClearFXChainSlot(COMMAND_T*);
void ResourcesClearTrTemplateSlot(COMMAND_T*);
void ResourcesClearPrjTemplateSlot(COMMAND_T*);
void ResourcesClearMediaSlot(COMMAND_T*);
void ResourcesClearImageSlot(COMMAND_T*);
void ResourcesClearThemeSlot(COMMAND_T*);

void ResourcesAutoSaveFXChain(COMMAND_T*);
void ResourcesAutoSaveTrTemplate(COMMAND_T*);
void ResourcesAutoSave(COMMAND_T*);

// fx chain slots
void ApplyTracksFXChainSlot(int _slotType, const char* _title, int _slot, bool _set, bool _inputFX);
bool AutoSaveTrackFXChainSlots(int _slotType, const char* _dirPath, WDL_PtrList<ResourceItem>* _owSlots, bool _nameFromFx, bool _inputFX);
void LoadSetTrackFXChainSlot(COMMAND_T*);
void LoadPasteTrackFXChainSlot(COMMAND_T*);
void LoadSetTrackInFXChainSlot(COMMAND_T*);
void LoadPasteTrackInFXChainSlot(COMMAND_T*);

void ApplyTakesFXChainSlot(int _slotType, const char* _title, int _slot, bool _activeOnly, bool _set);
bool AutoSaveItemFXChainSlots(int _slotType, const char* _dirPath, WDL_PtrList<ResourceItem>* _owSlots, bool _nameFromFx);
void LoadSetTakeFXChainSlot(COMMAND_T*);
void LoadPasteTakeFXChainSlot(COMMAND_T*);
void LoadSetAllTakesFXChainSlot(COMMAND_T*);
void LoadPasteAllTakesFXChainSlot(COMMAND_T*);

// track template slots
void ImportTrackTemplateSlot(int _slotType, const char* _title, int _slot);
void ApplyTrackTemplateSlot(int _slotType, const char* _title, int _slot, bool _itemsFromTmplt, bool _envsFromTmplt);
void ReplacePasteItemsTrackTemplateSlot(int _slotType, const char* _title, int _slot, bool _paste);
void ReplaceItemsTrackTemplateSlot(COMMAND_T*);
void PasteItemsTrackTemplateSlot(COMMAND_T*);
bool AutoSaveTrackSlots(int _slotType, const char* _dirPath, WDL_PtrList<ResourceItem>* _owSlots, bool _delItems, bool _delEnvs);
void LoadApplyTrackTemplateSlot(COMMAND_T*);
void LoadApplyTrackTemplateSlotWithItemsEnvs(COMMAND_T*);
void LoadImportTrackTemplateSlot(COMMAND_T*);

// project template slots
void LoadOrSelectProjectSlot(int _slotType, const char* _title, int _slot, bool _newTab);
bool AutoSaveProjectSlot(int _slotType, const char* _dirPath, WDL_PtrList<ResourceItem>* _owSlots, bool _saveCurPrj);
void LoadOrSelectProjectSlot(COMMAND_T*);
void LoadOrSelectProjectTabSlot(COMMAND_T*);
void LoadOrSelectNextPreviousProjectSlot(COMMAND_T*);
void LoadOrSelectNextPreviousProjectTabSlot(COMMAND_T*);

// media slots
void PlaySelTrackMediaSlot(COMMAND_T*);
void LoopSelTrackMediaSlot(COMMAND_T*);
void SyncPlaySelTrackMediaSlot(COMMAND_T*);
void SyncLoopSelTrackMediaSlot(COMMAND_T*);
bool TogglePlaySelTrackMediaSlot(int _slotType, const char* _title, int _slot, bool _pause, bool _loop, double _msi = -1.0);
void TogglePlaySelTrackMediaSlot(COMMAND_T*);
void ToggleLoopSelTrackMediaSlot(COMMAND_T*);
void TogglePauseSelTrackMediaSlot(COMMAND_T*);
void ToggleLoopPauseSelTrackMediaSlot(COMMAND_T*);
#ifdef _SNM_MISC
void SyncTogglePlaySelTrackMediaSlot(COMMAND_T*);
void SyncToggleLoopSelTrackMediaSlot(COMMAND_T*);
void SyncTogglePauseSelTrackMediaSlot(COMMAND_T*);
void SyncToggleLoopPauseSelTrackMediaSlot(COMMAND_T*);
#endif
void AddMediaOptionName(WDL_FastString* _name);
void SetMediaOption(int _opt);
void SetMediaOption(COMMAND_T*);
int IsMediaOption(int _opt);
int IsMediaOption(COMMAND_T*);
void InsertMediaSlot(int _slotType, const char* _title, int _slot, int _insertMode);
void InsertMediaSlotCurTr(COMMAND_T*);
void InsertMediaSlotNewTr(COMMAND_T*);
void InsertMediaSlotTakes(COMMAND_T*);
bool AutoSaveMediaSlots(int _slotType, const char* _dirPath, WDL_PtrList<ResourceItem>* _owSlots);

// image slots
void ShowImageSlot(int _slotType, const char* _title, int _slot);
void ShowImageSlot(COMMAND_T*);
void ShowNextPreviousImageSlot(COMMAND_T*);
void SetSelTrackIconSlot(int _slotType, const char* _title, int _slot);
void SetSelTrackIconSlot(COMMAND_T*);

// theme slots
void LoadThemeSlot(int _slotType, const char* _title, int _slot);
void LoadThemeSlot(COMMAND_T*);

// reascript export
int SNM_SelectResourceBookmark(const char* _name);
void SNM_TieResourceSlotActions(int _bookmarkId);

#endif
