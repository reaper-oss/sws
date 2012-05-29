/******************************************************************************
/ SnM_Resources.cpp
/
/ Copyright (c) 2009-2012 Jeffos
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

//JFB TODO?
// ShellExecute("edit", .. ?
// import/export slots
// combined flags for insert media (dbl click)

#include "stdafx.h"
#include "SnM.h"
#include "../Prompt.h"
#ifdef _WIN32
#include "../DragDrop.h"
#endif
#include "../reaper/localize.h"


// common cmds
#define AUTOFILL_MSG				0xF000
#define AUTOFILL_DIR_MSG			0xF001
#define AUTOFILL_PRJ_MSG			0xF002
#define AUTOFILL_DEFAULT_MSG		0xF003
#define AUTOFILL_SYNC_MSG			0xF004
#define CLEAR_SLOTS_MSG				0xF005
#define DEL_SLOTS_MSG				0xF006
#define DEL_FILES_MSG				0xF007
#define ADD_SLOT_MSG				0xF008
#define INSERT_SLOT_MSG				0xF009
#define EDIT_MSG					0xF00A
#define EXPLORE_MSG					0xF00B
#define LOAD_MSG					0xF00C
#define AUTOSAVE_MSG				0xF00D
#define AUTOSAVE_DIR_MSG			0xF00E
#define AUTOSAVE_DIR_PRJ_MSG		0xF00F
#define AUTOSAVE_DIR_DEFAULT_MSG	0xF010
#define AUTOSAVE_SYNC_MSG			0xF011
#define FILTER_BY_NAME_MSG			0xF012
#define FILTER_BY_PATH_MSG			0xF013
#define FILTER_BY_COMMENT_MSG		0xF014
#define RENAME_MSG					0xF015
#define COPY_BOOKMARK_MSG			0xF016
#define DEL_BOOKMARK_MSG			0xF017
#define REN_BOOKMARK_MSG			0xF018
#define NEW_BOOKMARK_FXC_MSG		0xF019
#define NEW_BOOKMARK_TR_MSG			0xF01A
#define NEW_BOOKMARK_PRJ_MSG		0xF01B
#define NEW_BOOKMARK_MED_MSG		0xF01C
#define NEW_BOOKMARK_IMG_MSG		0xF01D
#define NEW_BOOKMARK_THM_MSG		0xF01E // leave some room here at least as much as default slot types) -->
#define NEW_BOOKMARK_END_MSG		0xF02F// <--
// fx chains cmds
#define FXC_APPLY_TR_MSG			0xF030
#define FXC_APPLY_TAKE_MSG			0xF031
#define FXC_APPLY_ALLTAKES_MSG		0xF032
#define FXC_COPY_MSG				0xF033
#define FXC_PASTE_TR_MSG			0xF034
#define FXC_PASTE_TAKE_MSG			0xF035
#define FXC_PASTE_ALLTAKES_MSG		0xF036
#define FXC_AUTOSAVE_INPUTFX_MSG	0xF037
#define FXC_AUTOSAVE_TR_MSG			0xF038
#define FXC_AUTOSAVE_ITEM_MSG		0xF039
#define FXC_AUTOSAVE_DEFNAME_MSG	0xF03A
#define FXC_AUTOSAVE_FX1NAME_MSG	0xF03B
// track template cmds
#define TRT_APPLY_MSG				0xF040
#define TRT_APPLY_WITH_ENV_ITEM_MSG	0xF041
#define TRT_IMPORT_MSG				0xF042
#define TRT_REPLACE_ITEMS_MSG		0xF043
#define TRT_PASTE_ITEMS_MSG			0xF044
#define TRT_AUTOSAVE_WITEMS_MSG		0xF045
#define TRT_AUTOSAVE_WENVS_MSG		0xF046
// project template cmds
#define PRJ_OPEN_SELECT_MSG			0xF050
#define PRJ_OPEN_SELECT_TAB_MSG		0xF051
#define PRJ_AUTOFILL_RECENTS_MSG	0xF052
#define PRJ_LOADER_CONF_MSG			0xF053
#define PRJ_LOADER_SET_MSG			0xF054
#define PRJ_LOADER_CLEAR_MSG		0xF055
// media file cmds
#define MED_PLAY_MSG				0xF060
#define MED_LOOP_MSG				0xF061
#define MED_ADD_CURTR_MSG			0xF062
#define MED_ADD_NEWTR_MSG			0xF063
#define MED_ADD_TAKES_MSG			0xF064
#define MED_AUTOSAVE_MSG			0xF065
// image file cmds
#define IMG_SHOW_MSG				0xF070 
#define IMG_TRICON_MSG				0xF071
#ifdef _WIN32
// theme file cmds
#define THM_LOAD_MSG				0xF080 
#endif

// labels for undo points and popup menu items
#define FXC_APPLY_TR_STR			__LOCALIZE("Paste (replace) FX chain to selected tracks","sws_DLG_150")
#define FXCIN_APPLY_TR_STR			__LOCALIZE("Paste (replace) input FX chain to selected tracks","sws_DLG_150")
#define FXC_APPLY_TAKE_STR			__LOCALIZE("Paste (replace) FX chain to selected items","sws_DLG_150")
#define FXC_APPLY_ALLTAKES_STR		__LOCALIZE("Paste (replace) FX chain to selected items, all takes","sws_DLG_150")
#define FXC_PASTE_TR_STR			__LOCALIZE("Paste FX chain to selected tracks","sws_DLG_150")
#define FXCIN_PASTE_TR_STR			__LOCALIZE("Paste input FX chain to selected tracks","sws_DLG_150")
#define FXC_PASTE_TAKE_STR			__LOCALIZE("Paste FX chain to selected items","sws_DLG_150")
#define FXC_PASTE_ALLTAKES_STR		__LOCALIZE("Paste FX chain to selected items, all takes","sws_DLG_150")
#define TRT_APPLY_STR				__LOCALIZE("Apply track template to selected tracks","sws_DLG_150")
#define TRT_APPLY_WITH_ENV_ITEM_STR	__LOCALIZE("Apply track template (+items/envelopes) to selected tracks","sws_DLG_150")
#define TRT_IMPORT_STR				__LOCALIZE("Import tracks from track template","sws_DLG_150")
#define TRT_REPLACE_ITEMS_STR		__LOCALIZE("Paste (replace) template items to selected tracks","sws_DLG_150")
#define TRT_PASTE_ITEMS_STR			__LOCALIZE("Paste template items to selected tracks","sws_DLG_150")
#define PRJ_SELECT_LOAD_STR			__LOCALIZE("Open/select project","sws_DLG_150")
#define PRJ_SELECT_LOAD_TAB_STR		__LOCALIZE("Open/select project (new tab)","sws_DLG_150")
#define MED_PLAY_STR				__LOCALIZE("Play media file in selected tracks (toggle)","sws_DLG_150")
#define MED_LOOP_STR				__LOCALIZE("Loop media file in selected tracks (toggle)","sws_DLG_150")
#define MED_PLAYLOOP_STR			__LOCALIZE("Play/loop media file","sws_DLG_150")
#define MED_ADD_STR					__LOCALIZE("Insert media","sws_DLG_150")
#define IMG_SHOW_STR				__LOCALIZE("Show image","sws_DLG_150")
#define IMG_TRICON_STR				__LOCALIZE("Set track icon for selected tracks","sws_DLG_150")
#ifdef _WIN32
#define THM_LOAD_STR				__LOCALIZE("Load theme","sws_DLG_150")
#endif

#define DRAGDROP_EMPTY_SLOT		">Empty<" // no localization: internal stuff
#define FILTER_DEFAULT_STR		__LOCALIZE("Filter","sws_DLG_150")
#define AUTOSAVE_ERR_STR		__LOCALIZE("Probable cause: no selection, nothing to save, cannot write file, invalid filename, etc...","sws_DLG_150")
#define AUTOFILL_ERR_STR		__LOCALIZE("Probable cause: all files are already present, empty directory, directory not found , etc...","sws_DLG_150")
#define NO_SLOT_ERR_STR			__LOCALIZE("No %s slot defined in the Resources view!","sws_DLG_150")

enum {
  BUTTONID_AUTOFILL=2000, //JFB would be great to have _APS_NEXT_CONTROL_VALUE *always* defined
  BUTTONID_AUTOSAVE,
  COMBOID_TYPE,
  CONTAINER_ADD_DEL,
  BUTTONID_ADD_BOOKMARK,
  BUTTONID_DEL_BOOKMARK,
  BUTTONID_TIED_ACTIONS,
  BUTTONID_OFFSET_TR_TEMPLATE,
  TXTID_DBL_TYPE,
  COMBOID_DBLCLICK_TYPE,
  TXTID_DBL_TO,
  COMBOID_DBLCLICK_TO
};

enum {
  COL_SLOT=0,
  COL_NAME,
  COL_PATH,
  COL_COMMENT,
  COL_COUNT
};

/*JFB static*/ SNM_ResourceWnd* g_pResourcesWnd = NULL;

// JFB important notes:
// all global WDL_PtrList vars used to be WDL_PtrList_DeleteOnDestroy ones but
// something weird could occur when REAPER unloads the extension: hang or crash 
// (e.g. issues 292 & 380) on Windows 7 while saving ini files (those lists were 
// already unallocated..)
// slots lists are allocated on the heap for the same reason..
// anyway, no prob here because application exit will destroy the entire runtime 
// context regardless.

WDL_PtrList<PathSlotItem> g_dragPathSlotItems; 
WDL_PtrList<FileSlotList> g_slots;

// prefs
int g_resViewType = -1;
int g_tiedSlotActions[SNM_NUM_DEFAULT_SLOTS]; // slot actions of default type/idx are tied to type/value
int g_dblClickPrefs[SNM_MAX_SLOT_TYPES]; // flags, loword: dbl-click type, hiword: dbl-click to (only for fx chains, atm)
WDL_FastString g_filter(FILTER_DEFAULT_STR);
int g_filterPref = 1; // bitmask: &1 = filter by name, &2 = filter by path, &4 = filter by comment
WDL_PtrList<WDL_FastString> g_autoSaveDirs;
WDL_PtrList<WDL_FastString> g_autoFillDirs;
bool g_syncAutoDirPrefs[SNM_MAX_SLOT_TYPES];
int g_prjLoaderStartPref = -1; // 1-based
int g_prjLoaderEndPref = -1; // 1-based
int g_autoSaveTrTmpltPref = 3; // &1: with items, &2: with envs
int g_autoSaveFXChainPref = FXC_AUTOSAVE_PREF_TRACK;
int g_autoSaveFXChainNamePref = 0;

// helper funcs
FileSlotList* GetSlotList(int _type = -1) {
	if (_type < 0) _type = g_resViewType;
	return g_slots.Get(_type);
}

const char* GetAutoSaveDir(int _type = -1) {
	if (_type < 0) _type = g_resViewType;
	return g_autoSaveDirs.Get(_type)->Get();
}

void SetAutoSaveDir(const char* _path, int _type = -1) {
	if (_type < 0) _type = g_resViewType;
	g_autoSaveDirs.Get(_type)->Set(_path);
	if (g_syncAutoDirPrefs[_type])
		g_autoFillDirs.Get(_type)->Set(_path);
}

const char* GetAutoFillDir(int _type = -1) {
	if (_type < 0) _type = g_resViewType;
	return g_autoFillDirs.Get(_type)->Get();
}

void SetAutoFillDir(const char* _path, int _type = -1) {
	if (_type < 0) _type = g_resViewType;
	g_autoFillDirs.Get(_type)->Set(_path);
	if (g_syncAutoDirPrefs[_type])
		g_autoSaveDirs.Get(_type)->Set(_path);
}

int GetTypeForUser(int _type = -1) {
	if (_type < 0) _type = g_resViewType;
	if (_type >= SNM_NUM_DEFAULT_SLOTS)
		for (int i=0; i < SNM_NUM_DEFAULT_SLOTS; i++)
			if (!_stricmp(g_slots.Get(_type)->GetFileExt(), g_slots.Get(i)->GetFileExt()))
				return i;
	return _type;
}

int GetTypeFromSlotList(FileSlotList* _l) {
	for (int i=0; i < g_slots.GetSize(); i++)
		if (g_slots.Get(i) == _l)
			return i;
	return -1;
}

bool IsMultiType(int _type = -1) {
	if (_type < 0) _type = g_resViewType;
	for (int i=0; i < g_slots.GetSize(); i++)
		if (i != _type && GetTypeForUser(i) == GetTypeForUser(_type))
			return true;
	return false;
}

const char* GetMenuDesc(int _type = -1) {
	if (_type < 0) _type = g_resViewType;
	if (_type >= SNM_NUM_DEFAULT_SLOTS)
		return g_slots.Get(_type)->GetDesc();
	return g_slots.Get(_type)->GetMenuDesc();
}

void GetIniSectionName(int _type, char* _bufOut, size_t _bufOutSz) {
	if (_type >= SNM_NUM_DEFAULT_SLOTS) {
		*_bufOut = '\0';
		_snprintfSafe(_bufOut, _bufOutSz, "CustomSlotType%d", _type-SNM_NUM_DEFAULT_SLOTS+1);
		return;
	}
	// relative path needed, for ex: data\track_icons
	lstrcpyn(_bufOut, GetFileRelativePath(g_slots.Get(_type)->GetResourceDir()), _bufOutSz);
	*_bufOut = toupper(*_bufOut);
}

// note: it is up to the caller to free things.. 
void GetIniSectionNames(WDL_PtrList<WDL_FastString>* _iniSections) {
	char iniSec[64]="";
	for (int i=0; i < g_slots.GetSize(); i++) {
		GetIniSectionName(i, iniSec, sizeof(iniSec));
		_iniSections->Add(new WDL_FastString(iniSec));
	}
}

bool IsFiltered() {
	return (g_filter.GetLength() && strcmp(g_filter.Get(), FILTER_DEFAULT_STR));
}


///////////////////////////////////////////////////////////////////////////////
// FileSlotList
///////////////////////////////////////////////////////////////////////////////

// _path: short resource path or full path
PathSlotItem* FileSlotList::AddSlot(const char* _path, const char* _desc) {
	char shortPath[BUFFER_SIZE] = "";
	GetShortResourcePath(m_resDir.Get(), _path, shortPath, BUFFER_SIZE);
	return Add(new PathSlotItem(shortPath, _desc));
}

// _path: short resource path or full path
PathSlotItem* FileSlotList::InsertSlot(int _slot, const char* _path, const char* _desc) {
	PathSlotItem* item = NULL;
	char shortPath[BUFFER_SIZE] = "";
	GetShortResourcePath(m_resDir.Get(), _path, shortPath, BUFFER_SIZE);
	if (_slot >=0 && _slot < GetSize()) item = Insert(_slot, new PathSlotItem(shortPath, _desc));
	else item = Add(new PathSlotItem(shortPath, _desc));
	return item;
}

int FileSlotList::FindByResFulltPath(const char* _resFullPath) {
	if (_resFullPath)
		for (int i=0; i < GetSize(); i++)
			if (PathSlotItem* item = Get(i))
				if (item->m_shortPath.GetLength() && strstr(_resFullPath, item->m_shortPath.Get())) // no stristr: osx + utf-8
					return i;
	return -1;
}

bool FileSlotList::GetFullPath(int _slot, char* _fullFn, int _fullFnSz) {
	if (PathSlotItem* item = Get(_slot)) {
		GetFullResourcePath(m_resDir.Get(), item->m_shortPath.Get(), _fullFn, _fullFnSz);
		return true;
	}
	return false;
}

bool FileSlotList::SetFromFullPath(int _slot, const char* _fullPath) {
	if (PathSlotItem* item = Get(_slot)) {
		char shortPath[BUFFER_SIZE] = "";
		GetShortResourcePath(m_resDir.Get(), _fullPath, shortPath, BUFFER_SIZE);
		item->m_shortPath.Set(shortPath);
		return true;
	}
	return false;
}

const char* FileSlotList::GetMenuDesc() {
	if (!m_menuDesc.GetLength()) {
		if (m_desc.Get()[m_desc.GetLength()-1] == 's') m_menuDesc.Set(m_desc.Get());
		else m_menuDesc.SetFormatted(m_desc.GetLength()+8, "%ss", m_desc.Get()); // add trailing 's'
		if (m_menuDesc.GetLength()) {
			char* p = (char*)m_menuDesc.Get();
			*p = toupper(*p); // 1st char to upper
		}
	}
	return m_menuDesc.Get();
}

bool FileSlotList::IsValidFileExt(const char* _ext) {
	if (!_ext) return false;
	if (!m_ext.GetLength()) return IsMediaExtension(_ext, false);
	return (_stricmp(_ext, m_ext.Get()) == 0);
}

void FileSlotList::GetFileFilter(char* _filter, size_t _filterSz) {
	if (!_filter) return;
	if (m_ext.GetLength()) {
		if (_snprintfStrict(_filter, _filterSz, "REAPER %s (*.%s)X*.%s", m_desc.Get(), m_ext.Get(), m_ext.Get()) > 0) {
			if (char* p = strchr(_filter, ')')) // special code for multiple null terminated strings ('X' -> '\0')
				*(p+1) = '\0';
		}
		else
			lstrcpyn(_filter, "\0\0", _filterSz);
	}
	else
		memcpy(_filter, plugin_getFilterList(), _filterSz); // memcpy because of '\0'
}

void FileSlotList::ClearSlot(int _slot, bool _guiUpdate) {
	if (_slot >=0 && _slot < GetSize())	{
		Get(_slot)->Clear();
		if (_guiUpdate && g_pResourcesWnd)
			g_pResourcesWnd->Update();
	}
}

void FileSlotList::ClearSlotPrompt(COMMAND_T* _ct)
{
	if (!GetSize()) {
		WDL_FastString msg; msg.SetFormatted(128, NO_SLOT_ERR_STR, g_slots.Get(GetTypeForUser(GetTypeFromSlotList(this)))->GetDesc());
		MessageBox(GetMainHwnd(), msg.Get(), __LOCALIZE("S&M - Error","sws_DLG_150"), MB_OK);
		return;
	}
	int slot = PromptForInteger(SWS_CMD_SHORTNAME(_ct), "Slot", 1, GetSize()); //loops on err
	if (slot == -1) return; // user has cancelled
	else ClearSlot(slot);
}

// returns false if cancelled
bool FileSlotList::BrowseSlot(int _slot, char* _fn, int _fnSz)
{
	bool ok = false;
	if (_slot >= 0 && _slot < GetSize())
	{
		char title[128]="", fileFilter[512]="", defaultPath[BUFFER_SIZE]="";
		_snprintfSafe(title, sizeof(title), __LOCALIZE_VERFMT("S&M - Load %s (slot %d)","sws_DLG_150"), m_desc.Get(), _slot+1);
		if (_snprintfStrict(defaultPath, sizeof(defaultPath), "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, m_resDir.Get()) <= 0)
			*defaultPath = '\0';
		GetFileFilter(fileFilter, sizeof(fileFilter));

		int type = GetTypeFromSlotList(this);
		const char* path = type>0 ? GetAutoFillDir(type) : "";
		if (char* fn = BrowseForFiles(title, m_lastBrowsedFn.GetLength()?m_lastBrowsedFn.Get():(*path?path:defaultPath), NULL, false, fileFilter))
		{
			if (_fn)
				lstrcpyn(_fn, fn, _fnSz);
			if (SetFromFullPath(_slot, fn)) {
				if (g_pResourcesWnd)
					g_pResourcesWnd->Update();
				ok = true;
			}
			m_lastBrowsedFn.Set(fn);
			free(fn);
		}
	}
	return ok;
}

bool FileSlotList::GetOrBrowseSlot(int _slot, char* _fn, int _fnSz, bool _errMsg)
{
	bool ok = false;
	if (_slot >= 0 && _slot < GetSize())
	{
		if (Get(_slot)->IsDefault())
			ok = BrowseSlot(_slot, _fn, _fnSz);
		else if (GetFullPath(_slot, _fn, _fnSz))
			ok = FileExistsErrMsg(_fn, _errMsg);
	}
	return ok;
}

// returns NULL if failed, otherwise it's up to the caller to free the returned string
WDL_FastString* FileSlotList::GetOrPromptOrBrowseSlot(const char* _title, int _slot)
{
	// prompt for slot if needed
	if (_slot == -1)
	{
		if (GetSize())
			_slot = PromptForInteger(_title, "Slot", 1, GetSize()); // loops on err
		else {
			WDL_FastString msg; msg.SetFormatted(128, NO_SLOT_ERR_STR, g_slots.Get(GetTypeForUser(GetTypeFromSlotList(this)))->GetDesc());
			MessageBox(GetMainHwnd(), msg.Get(), __LOCALIZE("S&M - Error","sws_DLG_150"), MB_OK);
		}
	}
	if (_slot == -1) // user has cancelled or empty
		return NULL; 

	// adds the needed number of slots (more macro friendly)
	if (_slot >= GetSize())
	{
		for (int i=GetSize(); i <= _slot; i++)
			Add(new PathSlotItem());
		if (g_pResourcesWnd) {
			g_pResourcesWnd->Update();
			g_pResourcesWnd->SelectBySlot(_slot);
		}
	}

	if (_slot < GetSize())
	{
		char fn[BUFFER_SIZE]="";
		if (GetOrBrowseSlot(_slot, fn, BUFFER_SIZE, _slot < 0 || !Get(_slot)->IsDefault()))
			return new WDL_FastString(fn);
	}
	return NULL;
}

void FileSlotList::EditSlot(int _slot)
{
	if (_slot >= 0 && _slot < GetSize())
	{
		char fullPath[BUFFER_SIZE] = "";
		if (GetFullPath(_slot, fullPath, BUFFER_SIZE) && FileExistsErrMsg(fullPath, true))
		{
#ifdef _WIN32
			WinSpawnNotepad(fullPath);
#else
			WDL_FastString chain;
			if (LoadChunk(fullPath, &chain, false))
			{
				char title[64] = "";
				_snprintfSafe(title, sizeof(title), __LOCALIZE_VERFMT("S&M - %s (slot %d)","sws_DLG_150"), m_desc.Get(), _slot+1);
				SNM_ShowMsg(chain.Get(), title);
			}
#endif
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// SNM_ResourceView
///////////////////////////////////////////////////////////////////////////////

// !WANT_LOCALIZE_STRINGS_BEGIN:sws_DLG_150
static SWS_LVColumn g_resListCols[] = { {65,2,"Slot"}, {100,1,"Name"}, {250,2,"Path"}, {200,1,"Comment"} };
// !WANT_LOCALIZE_STRINGS_END

SNM_ResourceView::SNM_ResourceView(HWND hwndList, HWND hwndEdit)
	: SWS_ListView(hwndList, hwndEdit, COL_COUNT, g_resListCols, "ResourcesViewState", false, "sws_DLG_150")
{
}

void SNM_ResourceView::GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax)
{
	if (str) *str = '\0';
	if (PathSlotItem* pItem = (PathSlotItem*)item)
	{
		switch (iCol)
		{
			case COL_SLOT:
			{
				int slot = GetSlotList()->Find(pItem);
				if (slot >= 0)
				{
					slot++;
					if (g_resViewType == SNM_SLOT_PRJ && IsProjectLoaderConfValid()) // no GetTypeForUser() here: only one loader/selecter config atm..
						_snprintfSafe(str, iStrMax, "%5.d %s", slot, 
							slot<g_prjLoaderStartPref || slot>g_prjLoaderEndPref ? "  " : 
							g_prjLoaderStartPref==slot ? "->" : g_prjLoaderEndPref==slot ? "<-" :  "--");
					else
						_snprintfSafe(str, iStrMax, "%5.d", slot);
				}
				break;
			}
			case COL_NAME:
				GetFilenameNoExt(pItem->m_shortPath.Get(), str, iStrMax);
				break;
			case COL_PATH:
				lstrcpyn(str, pItem->m_shortPath.Get(), iStrMax);
				break;
			case COL_COMMENT:
				lstrcpyn(str, pItem->m_comment.Get(), iStrMax);
				break;
		}
	}
}

bool SNM_ResourceView::IsEditListItemAllowed(SWS_ListItem* item, int iCol)
{
	if (PathSlotItem* pItem = (PathSlotItem*)item)
	{
		if (!pItem->IsDefault())
		{
			int slot = GetSlotList()->Find(pItem);
			if (slot >= 0)
			{
				switch (iCol) {
					case COL_NAME: { // file renaming
						char fn[BUFFER_SIZE] = "";
						return (GetSlotList()->GetFullPath(slot, fn, BUFFER_SIZE) && FileExistsErrMsg(fn, false));
					}					
					case COL_COMMENT:
						return true;
				}
			}
		}
	}
	return false;
}

void SNM_ResourceView::SetItemText(SWS_ListItem* item, int iCol, const char* str)
{
	PathSlotItem* pItem = (PathSlotItem*)item;
	int slot = GetSlotList()->Find(pItem);
	if (pItem && slot >=0)
	{
		switch (iCol)
		{
			case COL_NAME: // file renaming
			{
				if (!IsValidFilenameErrMsg(str, true))
					return;

				char fn[BUFFER_SIZE] = "";
				if (GetSlotList()->GetFullPath(slot, fn, BUFFER_SIZE) && !pItem->IsDefault() && FileExistsErrMsg(fn, true))
				{
					const char* ext = GetFileExtension(fn);
					char newFn[BUFFER_SIZE]="", path[BUFFER_SIZE]="";
					lstrcpyn(path, fn, BUFFER_SIZE);
					if (char* p = strrchr(path, PATH_SLASH_CHAR))
						*p = '\0';
					else
						break; // safety

					if (_snprintfStrict(newFn, sizeof(newFn), "%s%c%s.%s", path, PATH_SLASH_CHAR, str, ext) > 0)
					{
						if (FileExists(newFn)) 
						{
							char msg[BUFFER_SIZE]="";
							_snprintfSafe(msg, sizeof(msg), __LOCALIZE_VERFMT("File already exists. Overwrite ?\n%s","sws_mbox"), newFn);
							int res = MessageBox(g_pResourcesWnd?g_pResourcesWnd->GetHWND():GetMainHwnd(), msg, __LOCALIZE("S&M - Warning","sws_DLG_150"), MB_YESNO);
							if (res == IDYES) {
								if (!SNM_DeleteFile(newFn, false))
									break;
							}
							else 
								break;
						}

						if (MoveFile(fn, newFn) && GetSlotList()->SetFromFullPath(slot, newFn))
							ListView_SetItemText(m_hwndList, GetEditingItem(), DisplayToDataCol(2), (LPSTR)pItem->m_shortPath.Get());
							// ^^ direct GUI update 'cause Update() is no-op when editing
					}
				}
				break;
			}
			case COL_COMMENT:
				pItem->m_comment.Set(str);
				pItem->m_comment.Ellipsize(128,128);
				Update();
				break;
		}
	}
}

void SNM_ResourceView::OnItemDblClk(SWS_ListItem* item, int iCol) {
	Perform();
}

void SNM_ResourceView::GetItemList(SWS_ListItemList* pList)
{
	if (IsFiltered())
	{
		char buf[BUFFER_SIZE] = "";
		LineParser lp(false);
		if (!lp.parse(g_filter.Get()))
		{
			for (int i=0; i < GetSlotList()->GetSize(); i++)
			{
				if (PathSlotItem* item = GetSlotList()->Get(i))
				{
					bool match = false;
					for (int j=0; !match && j < lp.getnumtokens(); j++)
					{
						if (g_filterPref&1) // name
						{
							GetFilenameNoExt(item->m_shortPath.Get(), buf, BUFFER_SIZE);
							match |= (stristr(buf, lp.gettoken_str(j)) != NULL);
						}
						if (!match && (g_filterPref&2)) // path
						{
							if (GetSlotList()->GetFullPath(i, buf, BUFFER_SIZE))
								if (char* p = strrchr(buf, PATH_SLASH_CHAR)) {
									*p = '\0';
									match |= (stristr(buf, lp.gettoken_str(j)) != NULL);
								}
						}
						if (!match && (g_filterPref&4)) // comment
							match |= (stristr(item->m_comment.Get(), lp.gettoken_str(j)) != NULL);
					}
					if (match)
						pList->Add((SWS_ListItem*)item);
				}
			}
		}
	}
	else
	{
		for (int i=0; i < GetSlotList()->GetSize(); i++)
			pList->Add((SWS_ListItem*)GetSlotList()->Get(i));
	}
}

void SNM_ResourceView::OnBeginDrag(SWS_ListItem* _item)
{
#ifdef _WIN32
	g_dragPathSlotItems.Empty(false);

	// store dragged items (for internal d'n'd) + full paths + get the amount of memory needed
	int iMemNeeded = 0, x=0;
	WDL_PtrList_DeleteOnDestroy<WDL_FastString> fullPaths;
	PathSlotItem* pItem = (PathSlotItem*)EnumSelected(&x);
	while(pItem)
	{
		int slot = GetSlotList()->Find(pItem);
		char fullPath[BUFFER_SIZE] = "";
		if (GetSlotList()->GetFullPath(slot, fullPath, BUFFER_SIZE))
		{
			bool empty = (pItem->IsDefault() || *fullPath == '\0');
			iMemNeeded += (int)((empty ? strlen(DRAGDROP_EMPTY_SLOT) : strlen(fullPath)) + 1);
			fullPaths.Add(new WDL_FastString(empty ? DRAGDROP_EMPTY_SLOT : fullPath));
			g_dragPathSlotItems.Add(pItem);
		}
		pItem = (PathSlotItem*)EnumSelected(&x);
	}
	if (!iMemNeeded)
		return;

	iMemNeeded += sizeof(DROPFILES) + 1;

	HGLOBAL hgDrop = GlobalAlloc (GHND | GMEM_SHARE, iMemNeeded);
	DROPFILES* pDrop = NULL;
	if (hgDrop)
		pDrop = (DROPFILES*)GlobalLock(hgDrop); // 'spose should do some error checking...
	
	// for safety..
	if (!hgDrop || !pDrop)
		return;

	pDrop->pFiles = sizeof(DROPFILES);
	pDrop->fWide = false;
	char* pBuf = (char*)pDrop + pDrop->pFiles;

	// add files to the DROPFILES struct, double-NULL terminated
	for (int i=0; i < fullPaths.GetSize(); i++) {
		strcpy(pBuf, fullPaths.Get(i)->Get());
		pBuf += strlen(pBuf) + 1;
	}
	*pBuf = 0;

	GlobalUnlock(hgDrop);
	FORMATETC etc = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	STGMEDIUM stgmed = { TYMED_HGLOBAL, { 0 }, 0 };
	stgmed.hGlobal = hgDrop;

	SWS_IDataObject* dataObj = new SWS_IDataObject(&etc, &stgmed);
	SWS_IDropSource* dropSrc = new SWS_IDropSource;
	DWORD effect;

	DoDragDrop(dataObj, dropSrc, DROPEFFECT_COPY, &effect);
#endif
}

void SNM_ResourceView::Perform()
{
	int x=0;
	PathSlotItem* pItem = (PathSlotItem*)EnumSelected(&x);
	int slot = GetSlotList()->Find(pItem);

	if (pItem && slot >= 0) 
	{
		bool wasDefaultSlot = pItem->IsDefault();
		int dblClickType = LOWORD(g_dblClickPrefs[g_resViewType]);
		int dblClickTo = HIWORD(g_dblClickPrefs[g_resViewType]);
		switch(GetTypeForUser())
		{
			case SNM_SLOT_FXC:
				switch(dblClickTo)
				{
					case 0:
						ApplyTracksFXChainSlot(g_resViewType, !dblClickType?FXC_APPLY_TR_STR:FXC_PASTE_TR_STR, slot, !dblClickType, false);
						break;
					case 1:
						ApplyTracksFXChainSlot(g_resViewType, !dblClickType?FXCIN_APPLY_TR_STR:FXCIN_PASTE_TR_STR, slot, !dblClickType, true);
						break;
					case 2:
						ApplyTakesFXChainSlot(g_resViewType, !dblClickType?FXC_APPLY_TAKE_STR:FXC_PASTE_TAKE_STR, slot, true, !dblClickType);
						break;
					case 3:
						ApplyTakesFXChainSlot(g_resViewType, !dblClickType?FXC_APPLY_ALLTAKES_STR:FXC_PASTE_ALLTAKES_STR, slot, false, !dblClickType);
						break;
				}
				break;
			case SNM_SLOT_TR:
				switch(dblClickType)
				{
					case 0:
						ImportTrackTemplateSlot(g_resViewType, TRT_IMPORT_STR, slot);
						break;
					case 1:
						ApplyTrackTemplateSlot(g_resViewType, TRT_APPLY_STR, slot, false, false);
						break;
					case 2:
						ApplyTrackTemplateSlot(g_resViewType, TRT_APPLY_WITH_ENV_ITEM_STR, slot, true, true);
						break;
					case 3:
						ReplacePasteItemsTrackTemplateSlot(g_resViewType, TRT_PASTE_ITEMS_STR, slot, true);
						break;
					case 4:
						ReplacePasteItemsTrackTemplateSlot(g_resViewType, TRT_REPLACE_ITEMS_STR, slot, false);
						break;
				}
				break;
			case SNM_SLOT_PRJ:
				switch(dblClickType)
				{
					case 0:
						LoadOrSelectProjectSlot(g_resViewType, PRJ_SELECT_LOAD_STR, slot, false);
						break;
					case 1:
						LoadOrSelectProjectSlot(g_resViewType, PRJ_SELECT_LOAD_TAB_STR, slot, true);
						break;
				}
				break;
			case SNM_SLOT_MEDIA:
			{
				int insertMode = -1;
				switch(dblClickType) 
				{
					case 0: TogglePlaySelTrackMediaSlot(g_resViewType, MED_PLAY_STR, slot, false, false); break;
					case 1: TogglePlaySelTrackMediaSlot(g_resViewType, MED_LOOP_STR, slot, false, true); break;
					case 2: insertMode = 0; break;
					case 3: insertMode = 1; break;
					case 4: insertMode = 3; break;
				}
				if (insertMode >= 0)
					InsertMediaSlot(g_resViewType, MED_ADD_STR, slot, insertMode);
				break;
			}
			case SNM_SLOT_IMG:
				switch(dblClickType) {
					case 0: ShowImageSlot(g_resViewType, IMG_SHOW_STR, slot); break;
					case 1: SetSelTrackIconSlot(g_resViewType, IMG_TRICON_STR, slot); break;
					case 2: InsertMediaSlot(g_resViewType, MED_ADD_STR, slot, 0); break;
				}
				break;
#ifdef _WIN32
	 		case SNM_SLOT_THM:
				LoadThemeSlot(g_resViewType, THM_LOAD_STR, slot);
				break;
#endif
		}

		// update if the slot changed
		if (wasDefaultSlot && !pItem->IsDefault())
			Update();
	}
}


///////////////////////////////////////////////////////////////////////////////
// SNM_ResourceWnd
///////////////////////////////////////////////////////////////////////////////

SNM_ResourceWnd::SNM_ResourceWnd()
	: SWS_DockWnd(IDD_SNM_RESOURCES, __LOCALIZE("Resources","sws_DLG_150"), "SnMResources", 30006, SWSGetCommandID(OpenResourceView))
{
	// Must call SWS_DockWnd::Init() to restore parameters and open the window if necessary
	Init();
}

void SNM_ResourceWnd::OnInitDlg()
{
	m_resize.init_item(IDC_LIST, 0.0, 0.0, 1.0, 1.0);
	m_resize.init_item(IDC_FILTER, 1.0, 1.0, 1.0, 1.0);

	SetWindowLongPtr(GetDlgItem(m_hwnd, IDC_FILTER), GWLP_USERDATA, 0xdeadf00b);

	m_pLists.Add(new SNM_ResourceView(GetDlgItem(m_hwnd, IDC_LIST), GetDlgItem(m_hwnd, IDC_EDIT)));
	SNM_ThemeListView(m_pLists.Get(0));

	// WDL GUI init
	m_vwnd_painter.SetGSC(WDL_STYLE_GetSysColor);
	m_parentVwnd.SetRealParent(m_hwnd);

	m_cbType.SetID(COMBOID_TYPE);
	FillTypeCombo();
	m_parentVwnd.AddChild(&m_cbType);

	m_cbDblClickType.SetID(COMBOID_DBLCLICK_TYPE);
	m_parentVwnd.AddChild(&m_cbDblClickType);
	m_cbDblClickTo.SetID(COMBOID_DBLCLICK_TO);
	m_parentVwnd.AddChild(&m_cbDblClickTo);
	FillDblClickCombos();

	m_btnAutoFill.SetID(BUTTONID_AUTOFILL);
	m_parentVwnd.AddChild(&m_btnAutoFill);

	m_btnAutoSave.SetID(BUTTONID_AUTOSAVE);
	m_parentVwnd.AddChild(&m_btnAutoSave);

	m_txtDblClickType.SetID(TXTID_DBL_TYPE);
	m_txtDblClickType.SetText(__LOCALIZE("Dbl-click to:","sws_DLG_150"));
	m_parentVwnd.AddChild(&m_txtDblClickType);

	m_btnsAddDel.SetIDs(CONTAINER_ADD_DEL, BUTTONID_ADD_BOOKMARK, BUTTONID_DEL_BOOKMARK);
	m_parentVwnd.AddChild(&m_btnsAddDel);

	m_btnTiedActions.SetID(BUTTONID_TIED_ACTIONS);
	m_parentVwnd.AddChild(&m_btnTiedActions);

	m_txtDblClickTo.SetID(TXTID_DBL_TO);
	m_txtDblClickTo.SetText(__LOCALIZE("To sel.:","sws_DLG_150"));
	m_parentVwnd.AddChild(&m_txtDblClickTo);

	m_btnOffsetTrTemplate.SetID(BUTTONID_OFFSET_TR_TEMPLATE);
	m_parentVwnd.AddChild(&m_btnOffsetTrTemplate);

	// restores the text filter when docking/undocking + indirect call to Update() !
	SetDlgItemText(GetHWND(), IDC_FILTER, g_filter.Get());

/* Perfs: see above comment
	Update();
*/
}

void SNM_ResourceWnd::OnDestroy() 
{
	m_cbType.Empty();
	m_cbDblClickType.Empty();
	m_cbDblClickTo.Empty();
}

INT_PTR SNM_ResourceWnd::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static int sListOldColors[LISTVIEW_COLORHOOK_STATESIZE];
	if (ListView_HookThemeColorsMessage(m_hwnd, uMsg, lParam, sListOldColors, IDC_LIST, 0, COL_COUNT))
		return 1;
	return SWS_DockWnd::WndProc(uMsg, wParam, lParam);
}

void SNM_ResourceWnd::SetType(int _type)
{
	int prevType = g_resViewType;
	g_resViewType = _type;
	m_cbType.SetCurSel(_type);
	if (prevType != g_resViewType) {
		FillDblClickCombos();
		Update();
	}
}

void SNM_ResourceWnd::Update()
{
	if (m_pLists.GetSize())
		m_pLists.Get(0)->Update();
	m_parentVwnd.RequestRedraw(NULL);
}

void SNM_ResourceWnd::FillTypeCombo()
{
	m_cbType.Empty();
	for (int i=0; i < g_slots.GetSize(); i++)
		m_cbType.AddItem(GetMenuDesc(i));
	m_cbType.SetCurSel((g_resViewType>0 && g_resViewType<g_slots.GetSize()) ? g_resViewType : SNM_SLOT_FXC);
}

void SNM_ResourceWnd::FillDblClickCombos()
{
	int typeForUser = GetTypeForUser();
	m_cbDblClickType.Empty();
	m_cbDblClickTo.Empty();
	switch(typeForUser)
	{
		case SNM_SLOT_FXC:
			m_cbDblClickType.AddItem(__LOCALIZE("Paste (replace)","sws_DLG_150"));
			m_cbDblClickType.AddItem(__LOCALIZE("Paste","sws_DLG_150"));

			m_cbDblClickTo.AddItem(__LOCALIZE("Tracks","sws_DLG_150"));
			m_cbDblClickTo.AddItem(__LOCALIZE("Tracks (input FX)","sws_DLG_150"));
			m_cbDblClickTo.AddItem(__LOCALIZE("Items","sws_DLG_150"));
			m_cbDblClickTo.AddItem(__LOCALIZE("Items (all takes)","sws_DLG_150"));
			break;
		case SNM_SLOT_TR:
			m_cbDblClickType.AddItem(__LOCALIZE("Import tracks from track template","sws_DLG_150"));
			m_cbDblClickType.AddItem(__LOCALIZE("Apply template to sel tracks","sws_DLG_150"));
			m_cbDblClickType.AddItem(__LOCALIZE("Apply template to sel tracks (+items/envs)","sws_DLG_150"));
			m_cbDblClickType.AddItem(__LOCALIZE("Paste template items to sel tracks","sws_DLG_150"));
			m_cbDblClickType.AddItem(__LOCALIZE("Paste (replace) template items to sel tracks","sws_DLG_150"));
			break;
		case SNM_SLOT_PRJ:
			m_cbDblClickType.AddItem(__LOCALIZE("Open/select project","sws_DLG_150"));
			m_cbDblClickType.AddItem(__LOCALIZE("Open/select project tab","sws_DLG_150"));
			break;
		case SNM_SLOT_MEDIA:
			m_cbDblClickType.AddItem(__LOCALIZE("Play in sel tracks (toggle)","sws_DLG_150"));
			m_cbDblClickType.AddItem(__LOCALIZE("Loop in sel tracks (toggle)","sws_DLG_150"));
			m_cbDblClickType.AddItem(__LOCALIZE("Add to current track","sws_DLG_150"));
			m_cbDblClickType.AddItem(__LOCALIZE("Add to new track","sws_DLG_150"));
			m_cbDblClickType.AddItem(__LOCALIZE("Add to sel items as takes","sws_DLG_150"));
			break;
		case SNM_SLOT_IMG:
			m_cbDblClickType.AddItem(IMG_SHOW_STR);
			m_cbDblClickType.AddItem(IMG_TRICON_STR);
			m_cbDblClickType.AddItem(__LOCALIZE("Add to current track as item","sws_DLG_150"));
			break;
#ifdef _WIN32
		case SNM_SLOT_THM:
			m_cbDblClickType.AddItem(__LOCALIZE("Load theme","sws_DLG_150"));
			break;
#endif
	}
	m_cbDblClickType.SetCurSel(LOWORD(g_dblClickPrefs[g_resViewType]));
	m_cbDblClickTo.SetCurSel(HIWORD(g_dblClickPrefs[g_resViewType]));
}

void SNM_ResourceWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	int x=0;
	PathSlotItem* item = (PathSlotItem*)m_pLists.Get(0)->EnumSelected(&x);
	int slot = GetSlotList()->Find(item);
	bool wasDefaultSlot = item ? item->IsDefault() : false;

	switch(LOWORD(wParam))
	{
		case IDC_FILTER:
			if (HIWORD(wParam)==EN_CHANGE)
			{
				char cFilter[128];
				GetWindowText(GetDlgItem(m_hwnd, IDC_FILTER), cFilter, 128);
				g_filter.Set(cFilter);
				Update();
			}
#ifdef _WIN32
			else if (HIWORD(wParam)==EN_SETFOCUS)
			{
				HWND hFilt = GetDlgItem(m_hwnd, IDC_FILTER);
				char cFilter[128];
				GetWindowText(hFilt, cFilter, 128);
				if (!strcmp(cFilter, FILTER_DEFAULT_STR))
					SetWindowText(hFilt, "");
			}
			else if (HIWORD(wParam)==EN_KILLFOCUS)
			{
				HWND hFilt = GetDlgItem(m_hwnd, IDC_FILTER);
				char cFilter[128];
				GetWindowText(hFilt, cFilter, 128);
				if (*cFilter == '\0') 
					SetWindowText(hFilt, FILTER_DEFAULT_STR);
			}
#endif                
			break;

		// ***** Common *****
		case ADD_SLOT_MSG:
			AddSlot(true);
			break;
		case INSERT_SLOT_MSG:
			InsertAtSelectedSlot(true);
			break;
		case CLEAR_SLOTS_MSG:
			ClearDeleteSlots(0, true);
			break;
		case DEL_SLOTS_MSG:
			ClearDeleteSlots(1, true);
			break;
		case DEL_FILES_MSG:
			ClearDeleteSlots(4|2, true);
			break;
		case LOAD_MSG:
			if (item) GetSlotList()->BrowseSlot(slot);
			break;
		case EDIT_MSG:
			if (item) GetSlotList()->EditSlot(slot);
			break;
		case EXPLORE_MSG:
		{
			char fullPath[BUFFER_SIZE] = "";
			if (GetSlotList()->GetFullPath(slot, fullPath, BUFFER_SIZE))
				if (char* p = strrchr(fullPath, PATH_SLASH_CHAR)) {
					*(p+1) = '\0'; // ShellExecute() is KO otherwie..
					ShellExecute(NULL, "open", fullPath, NULL, NULL, SW_SHOWNORMAL);
				}
			break;
		}
		// auto-fill
		case BUTTONID_AUTOFILL:
		case AUTOFILL_MSG:
			AutoFill(g_resViewType);
			break;
		case AUTOFILL_DIR_MSG:
		{
			char path[BUFFER_SIZE] = "";
			if (BrowseForDirectory(__LOCALIZE("S&&M - Set auto-fill directory","sws_DLG_150"), GetAutoFillDir(), path, BUFFER_SIZE))
				SetAutoFillDir(path);
			break;
		}
		case AUTOFILL_PRJ_MSG:
		{
			char path[BUFFER_SIZE] = "";
			GetProjectPath(path, BUFFER_SIZE);
			SetAutoFillDir(path);
			break;
		}
		case AUTOFILL_DEFAULT_MSG:
		{
			char path[BUFFER_SIZE] = "";
			if (_snprintfStrict(path, sizeof(path), "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, GetSlotList()->GetResourceDir()) > 0) {
				if (!FileExists(path))
					CreateDirectory(path, NULL);
				SetAutoFillDir(path);
			}
			break;
		}
		case AUTOFILL_SYNC_MSG: // i.e. sync w/ auto-save
			g_syncAutoDirPrefs[g_resViewType] = !g_syncAutoDirPrefs[g_resViewType];
			if (g_syncAutoDirPrefs[g_resViewType])
				g_autoFillDirs.Get(g_resViewType)->Set(GetAutoSaveDir());  // do not use SetAutoFillDir() here!
			break;
		// auto-save
		case BUTTONID_AUTOSAVE:
		case AUTOSAVE_MSG:
			AutoSave(g_resViewType, GetTypeForUser()==SNM_SLOT_TR ? g_autoSaveTrTmpltPref : GetTypeForUser()==SNM_SLOT_FXC ? g_autoSaveFXChainPref : 0);
			break;
		case AUTOSAVE_DIR_MSG:
		{
			char path[BUFFER_SIZE] = "";
			if (BrowseForDirectory(__LOCALIZE("S&&M - Set auto-save directory","sws_DLG_150"), GetAutoSaveDir(), path, BUFFER_SIZE))
				SetAutoSaveDir(path);
			break;
		}
		case AUTOSAVE_DIR_PRJ_MSG:
		{
			char prjPath[BUFFER_SIZE] = "", path[BUFFER_SIZE] = "";
			GetProjectPath(prjPath, BUFFER_SIZE);
			// see GetFileRelativePath..
			if (_snprintfStrict(path, sizeof(path), "%s%c%s", prjPath, PATH_SLASH_CHAR, GetFileRelativePath(GetSlotList()->GetResourceDir())) > 0) {
				if (!FileExists(path))
					CreateDirectory(path, NULL);
				SetAutoSaveDir(path);
			}
			break;
		}
		case AUTOSAVE_DIR_DEFAULT_MSG:
		{
			char path[BUFFER_SIZE] = "";
			if (_snprintfStrict(path, sizeof(path), "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, GetSlotList()->GetResourceDir()) > 0) {
				if (!FileExists(path))
					CreateDirectory(path, NULL);
				SetAutoSaveDir(path);
			}
			break;
		}
		case AUTOSAVE_SYNC_MSG: // i.e. sync w/ auto-fill
			g_syncAutoDirPrefs[g_resViewType] = !g_syncAutoDirPrefs[g_resViewType];
			if (g_syncAutoDirPrefs[g_resViewType])
				g_autoSaveDirs.Get(g_resViewType)->Set(GetAutoFillDir()); // do not use SetAutoSaveDir() here!
			break;
		// text filter mode
		case FILTER_BY_NAME_MSG:
			if (g_filterPref&1) g_filterPref &= 6; // 110
			else g_filterPref |= 1;
			Update();
//			SetFocus(GetDlgItem(m_hwnd, IDC_FILTER));
			break;
		case FILTER_BY_PATH_MSG:
			if (g_filterPref&2) g_filterPref &= 5; // 101
			else g_filterPref |= 2;
			Update();
//			SetFocus(GetDlgItem(m_hwnd, IDC_FILTER));
			break;
		case FILTER_BY_COMMENT_MSG:
			if (g_filterPref&4) g_filterPref &= 3; // 011
			else g_filterPref |= 4;
			Update();
//			SetFocus(GetDlgItem(m_hwnd, IDC_FILTER));
			break;
		case RENAME_MSG:
			if (item) {
				char fullPath[BUFFER_SIZE] = "";
				if (GetSlotList()->GetFullPath(slot, fullPath, BUFFER_SIZE) && FileExistsErrMsg(fullPath, true))
					m_pLists.Get(0)->EditListItem((SWS_ListItem*)item, COL_NAME);
			}
			break;
		case REN_BOOKMARK_MSG:
			RenameBookmark(g_resViewType);
			break;
		case BUTTONID_ADD_BOOKMARK:
			NewBookmark(g_resViewType, false);
			break;
		case COPY_BOOKMARK_MSG:
			NewBookmark(g_resViewType, true);
			break;
		case BUTTONID_DEL_BOOKMARK:
		case DEL_BOOKMARK_MSG:
			DeleteBookmark(g_resViewType);
			break;

		// new bookmark cmds are in an interval of cmds: see "default" case below..

		case BUTTONID_TIED_ACTIONS:
			if (!HIWORD(wParam) || HIWORD(wParam)==600)
				g_tiedSlotActions[GetTypeForUser()] = g_resViewType;
			break;

		// ***** FX chain *****
		case FXC_COPY_MSG:
			if (item) CopyFXChainSlotToClipBoard(slot);
			break;
		case FXC_APPLY_TR_MSG:
		case FXC_PASTE_TR_MSG:
			if (item && slot >= 0) {
				ApplyTracksFXChainSlot(g_resViewType, LOWORD(wParam)==FXC_APPLY_TR_MSG?FXC_APPLY_TR_STR:FXC_PASTE_TR_STR, slot, LOWORD(wParam)==FXC_APPLY_TR_MSG, false);
				if (wasDefaultSlot && !item->IsDefault()) // slot has been filled ?
					Update();
			}
			break;
		case FXC_APPLY_TAKE_MSG:
		case FXC_PASTE_TAKE_MSG:
			if (item && slot >= 0) {
				ApplyTakesFXChainSlot(g_resViewType, LOWORD(wParam)==FXC_APPLY_TAKE_MSG?FXC_APPLY_TAKE_STR:FXC_PASTE_TAKE_STR, slot, true, LOWORD(wParam)==FXC_APPLY_TAKE_MSG);
				if (wasDefaultSlot && !item->IsDefault()) // slot has been filled ?
					Update();
			}
			break;
		case FXC_APPLY_ALLTAKES_MSG:
		case FXC_PASTE_ALLTAKES_MSG:
			if (item && slot >= 0) {
				ApplyTakesFXChainSlot(g_resViewType, LOWORD(wParam)==FXC_APPLY_ALLTAKES_MSG?FXC_APPLY_ALLTAKES_STR:FXC_PASTE_ALLTAKES_STR, slot, false, LOWORD(wParam)==FXC_APPLY_ALLTAKES_MSG);
				if (wasDefaultSlot && !item->IsDefault()) // slot has been filled ?
					Update();
			}
			break;
		case FXC_AUTOSAVE_INPUTFX_MSG:
			g_autoSaveFXChainPref = FXC_AUTOSAVE_PREF_INPUT_FX;
			break;
		case FXC_AUTOSAVE_TR_MSG:
			g_autoSaveFXChainPref = FXC_AUTOSAVE_PREF_TRACK;
			break;
		case FXC_AUTOSAVE_ITEM_MSG:
			g_autoSaveFXChainPref = FXC_AUTOSAVE_PREF_ITEM;
			break;
		case FXC_AUTOSAVE_DEFNAME_MSG:
		case FXC_AUTOSAVE_FX1NAME_MSG:
			g_autoSaveFXChainNamePref = g_autoSaveFXChainNamePref ? 0:1;
			break;

		// ***** Track template *****
		case TRT_APPLY_MSG:
			if (item && slot >= 0) {
				ApplyTrackTemplateSlot(g_resViewType, TRT_APPLY_STR, slot, false, false);
				if (wasDefaultSlot && !item->IsDefault()) // slot has been filled ?
					Update();
			}
			break;
		case TRT_APPLY_WITH_ENV_ITEM_MSG:
			if (item && slot >= 0) {
				ApplyTrackTemplateSlot(g_resViewType, TRT_APPLY_WITH_ENV_ITEM_STR, slot, true, true);
				if (wasDefaultSlot && !item->IsDefault()) // slot has been filled ?
					Update();
			}
			break;
		case TRT_IMPORT_MSG:
			if (item && slot >= 0) {
				ImportTrackTemplateSlot(g_resViewType, TRT_IMPORT_STR, slot);
				if (wasDefaultSlot && !item->IsDefault()) // slot has been filled ?
					Update();
			}
			break;
		case TRT_REPLACE_ITEMS_MSG:
		case TRT_PASTE_ITEMS_MSG:
			if (item && slot >= 0) {
				ReplacePasteItemsTrackTemplateSlot(g_resViewType, LOWORD(wParam)==TRT_REPLACE_ITEMS_MSG?TRT_REPLACE_ITEMS_STR:TRT_PASTE_ITEMS_STR, slot, LOWORD(wParam)==TRT_PASTE_ITEMS_MSG);
				if (wasDefaultSlot && !item->IsDefault()) // slot has been filled ?
					Update();
			}
			break;
		case BUTTONID_OFFSET_TR_TEMPLATE:
			if (int* offsPref = (int*)GetConfigVar("templateditcursor")) { // >= REAPER v4.15
				if (*offsPref) *offsPref = 0;
				else *offsPref = 1;
			}
			break;
		case TRT_AUTOSAVE_WITEMS_MSG:
			if (g_autoSaveTrTmpltPref & 1) g_autoSaveTrTmpltPref &= 0xE;
			else g_autoSaveTrTmpltPref |= 1;
			break;
		case TRT_AUTOSAVE_WENVS_MSG:
			if (g_autoSaveTrTmpltPref & 2) g_autoSaveTrTmpltPref &= 0xD;
			else g_autoSaveTrTmpltPref |= 2;
			break;

		// ***** Project template *****
		case PRJ_OPEN_SELECT_MSG:
			if (item && slot >= 0) {
				LoadOrSelectProjectSlot(g_resViewType, PRJ_SELECT_LOAD_STR, slot, false);
				if (wasDefaultSlot && !item->IsDefault()) // slot has been filled ?
					Update();
			}
			break;
		case PRJ_OPEN_SELECT_TAB_MSG:
		{
			bool updt = false;
			while(item) {
				slot = GetSlotList()->Find(item);
				wasDefaultSlot = item->IsDefault();
				LoadOrSelectProjectSlot(g_resViewType, PRJ_SELECT_LOAD_TAB_STR, slot, true);
				updt |= (wasDefaultSlot && !item->IsDefault()); // slot has been filled ?
				item = (PathSlotItem*)m_pLists.Get(0)->EnumSelected(&x);
			}
			if (updt) Update();
			break;
		}
		case PRJ_AUTOFILL_RECENTS_MSG:
		{
			int startSlot = GetSlotList()->GetSize();
			int nbRecents = GetPrivateProfileInt("REAPER", "numrecent", 0, get_ini_file());
			nbRecents = min(98, nbRecents); // just in case: 2 digits max..
			if (nbRecents)
			{
				char key[16], path[BUFFER_SIZE];
				WDL_PtrList_DeleteOnDestroy<WDL_FastString> prjs;
				for (int i=0; i < nbRecents; i++) {
					if (_snprintfStrict(key, sizeof(key), "recent%02d", i+1) > 0) {
						GetPrivateProfileString("Recent", key, "", path, BUFFER_SIZE, get_ini_file());
						if (*path)
							prjs.Add(new WDL_FastString(path));
					}
				}
				for (int i=0; i < prjs.GetSize(); i++)
					if (GetSlotList()->FindByResFulltPath(prjs.Get(i)->Get()) < 0) // skip if already present
						GetSlotList()->AddSlot(prjs.Get(i)->Get());
			}
			if (startSlot != GetSlotList()->GetSize()) {
				Update();
				SelectBySlot(startSlot, GetSlotList()->GetSize());
			}
			else {
				char msg[BUFFER_SIZE] = "";
				_snprintfSafe(msg, sizeof(msg), __LOCALIZE_VERFMT("No new recent projects found!\n%s","sws_DLG_150"), AUTOFILL_ERR_STR);
				MessageBox(m_hwnd, msg, __LOCALIZE("S&M - Warning","sws_DLG_150"), MB_OK);
			}
			break;
		}
		case PRJ_LOADER_CONF_MSG:
			ProjectLoaderConf(NULL);
			break;
		case PRJ_LOADER_SET_MSG:
		{
			int min=GetSlotList()->GetSize(), max=0;
			while(item) {
				slot = GetSlotList()->Find(item)+1;
				if (slot>max) max = slot;
				if (slot<min) min = slot;
				item = (PathSlotItem*)m_pLists.Get(0)->EnumSelected(&x);
			}
			if (max>min) {
				g_prjLoaderStartPref = min;
				g_prjLoaderEndPref = max;
				Update();
			}
			break;
		}
		case PRJ_LOADER_CLEAR_MSG:
			g_prjLoaderStartPref = g_prjLoaderEndPref = -1;
			Update();
			break;

		// ***** media files *****
		case MED_PLAY_MSG:
		case MED_LOOP_MSG:
			while(item) {
				slot = GetSlotList()->Find(item);
				wasDefaultSlot = item->IsDefault();
				TogglePlaySelTrackMediaSlot(g_resViewType, MED_PLAYLOOP_STR, slot, false, LOWORD(wParam)==MED_LOOP_MSG);
				item = (PathSlotItem*)m_pLists.Get(0)->EnumSelected(&x);
			}
			break;
		case MED_ADD_CURTR_MSG:
		case MED_ADD_NEWTR_MSG:
		case MED_ADD_TAKES_MSG:
			while(item) {
				slot = GetSlotList()->Find(item);
				wasDefaultSlot = item->IsDefault();
				InsertMediaSlot(g_resViewType, MED_ADD_STR, slot, LOWORD(wParam)==MED_ADD_CURTR_MSG ? 0 : LOWORD(wParam)==MED_ADD_NEWTR_MSG ? 1 : 3);
				item = (PathSlotItem*)m_pLists.Get(0)->EnumSelected(&x);
			}
			break;

		// ***** theme *****
#ifdef _WIN32
		case THM_LOAD_MSG:
			LoadThemeSlot(g_resViewType, THM_LOAD_STR, slot);
			break;
#endif

		// ***** image *****
		case IMG_SHOW_MSG:
			ShowImageSlot(g_resViewType, IMG_SHOW_STR, slot);
			break;
		case IMG_TRICON_MSG:
			SetSelTrackIconSlot(g_resViewType, IMG_TRICON_STR, slot);
			break;

			// ***** WDL GUI & others *****
		case COMBOID_TYPE:
			if (HIWORD(wParam)==CBN_SELCHANGE)
			{
				// stop cell editing (changing the list content would be ignored otherwise,
				// leading to unsynchronized dropdown box vs list view)
				m_pLists.Get(0)->EditListItemEnd(false);
				SetType(m_cbType.GetCurSel());
			}
			break;
		case COMBOID_DBLCLICK_TYPE:
			if (HIWORD(wParam)==CBN_SELCHANGE) {
				int hi = HIWORD(g_dblClickPrefs[g_resViewType]);
				g_dblClickPrefs[g_resViewType] = m_cbDblClickType.GetCurSel();
				g_dblClickPrefs[g_resViewType] |= hi<<16;
				m_parentVwnd.RequestRedraw(NULL); // some options can be hidden for new sel
			}
			break;
		case COMBOID_DBLCLICK_TO:
			if (HIWORD(wParam)==CBN_SELCHANGE) {
				g_dblClickPrefs[g_resViewType] = LOWORD(g_dblClickPrefs[g_resViewType]);
				g_dblClickPrefs[g_resViewType] |= m_cbDblClickTo.GetCurSel()<<16;
			}
			break;
		default:
			if (LOWORD(wParam) >= NEW_BOOKMARK_FXC_MSG && LOWORD(wParam) < (NEW_BOOKMARK_FXC_MSG+SNM_NUM_DEFAULT_SLOTS))
				NewBookmark(LOWORD(wParam)-NEW_BOOKMARK_FXC_MSG, false);
			else
				Main_OnCommand((int)wParam, (int)lParam);
			break;
	}
}

void SNM_ResourceWnd::AutoSaveContextMenu(HMENU _menu, bool _saveItem)
{
	int typeForUser = GetTypeForUser();
	char autoPath[BUFFER_SIZE] = "";
	_snprintfSafe(autoPath, sizeof(autoPath), __LOCALIZE_VERFMT("[Current auto-save path: %s]","sws_DLG_150"), *GetAutoSaveDir() ? GetAutoSaveDir() : __LOCALIZE("undefined","sws_DLG_150"));
	AddToMenu(_menu, autoPath, 0, -1, false, MF_DISABLED); // different from MFS_DISABLED! more readable (and more REAPER-ish)
	AddToMenu(_menu, __LOCALIZE("Sync auto-save and auto-fill paths","sws_DLG_150"), AUTOFILL_SYNC_MSG, -1, false, g_syncAutoDirPrefs[g_resViewType] ? MFS_CHECKED : MFS_UNCHECKED);

	if (_saveItem) {
		AddToMenu(_menu, SWS_SEPARATOR, 0);
		AddToMenu(_menu, __LOCALIZE("Auto-save","sws_DLG_150"), AUTOSAVE_MSG, -1, false, !IsFiltered() ? MF_ENABLED : MF_GRAYED);
	}

	AddToMenu(_menu, SWS_SEPARATOR, 0);
	AddToMenu(_menu, __LOCALIZE("Set auto-save directory...","sws_DLG_150"), AUTOSAVE_DIR_MSG);
	AddToMenu(_menu, __LOCALIZE("Set auto-save directory to default resource path","sws_DLG_150"), AUTOSAVE_DIR_DEFAULT_MSG);
	switch(typeForUser)
	{
		case SNM_SLOT_FXC:
			AddToMenu(_menu, __LOCALIZE("Set auto-save directory to project path (/FXChains)","sws_DLG_150"), AUTOSAVE_DIR_PRJ_MSG);
			AddToMenu(_menu, SWS_SEPARATOR, 0);
			AddToMenu(_menu, __LOCALIZE("Auto-save FX chains from track selection","sws_DLG_150"), FXC_AUTOSAVE_TR_MSG, -1, false, g_autoSaveFXChainPref == FXC_AUTOSAVE_PREF_TRACK ? MFS_CHECKED : MFS_UNCHECKED);
			AddToMenu(_menu, __LOCALIZE("Auto-save FX chains from item selection","sws_DLG_150"), FXC_AUTOSAVE_ITEM_MSG, -1, false, g_autoSaveFXChainPref == FXC_AUTOSAVE_PREF_ITEM ? MFS_CHECKED : MFS_UNCHECKED);
			AddToMenu(_menu, __LOCALIZE("Auto-save input FX chains from track selection","sws_DLG_150"), FXC_AUTOSAVE_INPUTFX_MSG, -1, false, g_autoSaveFXChainPref == FXC_AUTOSAVE_PREF_INPUT_FX ? MFS_CHECKED : MFS_UNCHECKED);
			AddToMenu(_menu, SWS_SEPARATOR, 0);
			AddToMenu(_menu, __LOCALIZE("Create filename from track/item name","sws_DLG_150"), FXC_AUTOSAVE_DEFNAME_MSG, -1, false, !g_autoSaveFXChainNamePref ? MFS_CHECKED : MFS_UNCHECKED);
			AddToMenu(_menu, __LOCALIZE("Create filename from 1st FX name","sws_DLG_150"), FXC_AUTOSAVE_FX1NAME_MSG, -1, false, g_autoSaveFXChainNamePref ? MFS_CHECKED : MFS_UNCHECKED);
			break;
		case SNM_SLOT_TR:
			AddToMenu(_menu, __LOCALIZE("Set auto-save directory to project path (/TrackTemplates)","sws_DLG_150"), AUTOSAVE_DIR_PRJ_MSG);
			AddToMenu(_menu, SWS_SEPARATOR, 0);
			AddToMenu(_menu, __LOCALIZE("Include track items in templates","sws_DLG_150"), TRT_AUTOSAVE_WITEMS_MSG, -1, false, (g_autoSaveTrTmpltPref&1) ? MFS_CHECKED : MFS_UNCHECKED);
			AddToMenu(_menu, __LOCALIZE("Include envelopes in templates","sws_DLG_150"), TRT_AUTOSAVE_WENVS_MSG, -1, false, (g_autoSaveTrTmpltPref&2) ? MFS_CHECKED : MFS_UNCHECKED);
			break;
		case SNM_SLOT_PRJ:
			AddToMenu(_menu, __LOCALIZE("Set auto-save directory to project path (/ProjectTemplates)","sws_DLG_150"), AUTOSAVE_DIR_PRJ_MSG);
			break;
	}
}

void SNM_ResourceWnd::AutoFillContextMenu(HMENU _menu, bool _fillItem)
{
	int typeForUser = GetTypeForUser();
	char autoPath[BUFFER_SIZE] = "";
	_snprintfSafe(autoPath, sizeof(autoPath), __LOCALIZE_VERFMT("[Current auto-fill path: %s]","sws_DLG_150"), *GetAutoFillDir() ? GetAutoFillDir() : __LOCALIZE("undefined","sws_DLG_150"));
	AddToMenu(_menu, autoPath, 0, -1, false, MF_DISABLED); // different from MFS_DISABLED! more readable (and more REAPER-ish)
	if (GetSlotList()->HasAutoSave())
		AddToMenu(_menu, __LOCALIZE("Sync auto-save and auto-fill paths","sws_DLG_150"), AUTOSAVE_SYNC_MSG, -1, false, g_syncAutoDirPrefs[g_resViewType] ? MFS_CHECKED : MFS_UNCHECKED);

	if (_fillItem) {
		AddToMenu(_menu, SWS_SEPARATOR, 0);
		AddToMenu(_menu, __LOCALIZE("Auto-fill","sws_DLG_150"), AUTOFILL_MSG, -1, false, !IsFiltered() ? MF_ENABLED : MF_GRAYED);
	}
	if (typeForUser == SNM_SLOT_PRJ) {
		if (!_fillItem) AddToMenu(_menu, SWS_SEPARATOR, 0);
		AddToMenu(_menu, __LOCALIZE("Auto-fill with recent projects","sws_DLG_150"), PRJ_AUTOFILL_RECENTS_MSG, -1, false, !IsFiltered() ? MF_ENABLED : MF_GRAYED);
	}
	AddToMenu(_menu, SWS_SEPARATOR, 0);
	AddToMenu(_menu, __LOCALIZE("Set auto-fill directory...","sws_DLG_150"), AUTOFILL_DIR_MSG, -1, false, !IsFiltered() ? MF_ENABLED : MF_GRAYED);
	AddToMenu(_menu, __LOCALIZE("Set auto-fill directory to default resource path","sws_DLG_150"), AUTOFILL_DEFAULT_MSG, -1, false, !IsFiltered() ? MF_ENABLED : MF_GRAYED);
	AddToMenu(_menu, __LOCALIZE("Set auto-fill directory to project path","sws_DLG_150"), AUTOFILL_PRJ_MSG, -1, false, !IsFiltered() ? MF_ENABLED : MF_GRAYED);
}

HMENU SNM_ResourceWnd::OnContextMenu(int x, int y, bool* wantDefaultItems)
{
	HMENU hMenu = CreatePopupMenu();

	// specific context menu for auto-fill/auto-save buttons
	POINT p; GetCursorPos(&p);
	ScreenToClient(m_hwnd,&p);
	if (WDL_VWnd* v = m_parentVwnd.VirtWndFromPoint(p.x,p.y,1))
	{
		switch (v->GetID())
		{
			case BUTTONID_AUTOFILL:
				*wantDefaultItems = false;
				AutoFillContextMenu(hMenu, false);
				return hMenu;
			case BUTTONID_AUTOSAVE:
				if (GetSlotList()->HasAutoSave()) {
					*wantDefaultItems = false;
					AutoSaveContextMenu(hMenu, false);
					return hMenu;
				}
				break;
		}
	}

	// general context menu
	int iCol;
	PathSlotItem* pItem = (PathSlotItem*)m_pLists.Get(0)->GetHitItem(x, y, &iCol);
	UINT enabled = (pItem && !pItem->IsDefault()) ? MF_ENABLED : MF_GRAYED;
	int typeForUser = GetTypeForUser();
	if (pItem && iCol >= 0)
	{
		*wantDefaultItems = false;
		switch(typeForUser)
		{
			case SNM_SLOT_FXC:
				AddToMenu(hMenu, FXC_PASTE_TR_STR, FXC_PASTE_TR_MSG, -1, false, enabled);
				AddToMenu(hMenu, FXC_APPLY_TR_STR, FXC_APPLY_TR_MSG, -1, false, enabled);
				AddToMenu(hMenu, SWS_SEPARATOR, 0);
				AddToMenu(hMenu, FXC_PASTE_TAKE_STR, FXC_PASTE_TAKE_MSG, -1, false, enabled);
				AddToMenu(hMenu, FXC_PASTE_ALLTAKES_STR, FXC_PASTE_ALLTAKES_MSG, -1, false, enabled);
				AddToMenu(hMenu, FXC_APPLY_TAKE_STR, FXC_APPLY_TAKE_MSG, -1, false, enabled);
				AddToMenu(hMenu, FXC_APPLY_ALLTAKES_STR, FXC_APPLY_ALLTAKES_MSG, -1, false, enabled);
/*JFB seems confusing, commented: it is not a "copy slot" thingy!
				AddToMenu(hMenu, SWS_SEPARATOR, 0);
				AddToMenu(hMenu, "Copy", FXC_COPY_MSG, -1, false, enabled);
*/
				break;
			case SNM_SLOT_TR:
				AddToMenu(hMenu, TRT_IMPORT_STR, TRT_IMPORT_MSG, -1, false, enabled);
				AddToMenu(hMenu, TRT_APPLY_STR, TRT_APPLY_MSG, -1, false, enabled);
				AddToMenu(hMenu, TRT_APPLY_WITH_ENV_ITEM_STR, TRT_APPLY_WITH_ENV_ITEM_MSG, -1, false, enabled);
				AddToMenu(hMenu, SWS_SEPARATOR, 0);
				AddToMenu(hMenu, TRT_PASTE_ITEMS_STR, TRT_PASTE_ITEMS_MSG, -1, false, enabled);
				AddToMenu(hMenu, TRT_REPLACE_ITEMS_STR, TRT_REPLACE_ITEMS_MSG, -1, false, enabled);
				break;
			case SNM_SLOT_PRJ:
			{
				AddToMenu(hMenu, PRJ_SELECT_LOAD_STR, PRJ_OPEN_SELECT_MSG, -1, false, enabled);
				AddToMenu(hMenu, PRJ_SELECT_LOAD_TAB_STR, PRJ_OPEN_SELECT_TAB_MSG, -1, false, enabled);
				if (g_resViewType == typeForUser)  // no GetTypeForUser() here: only one loader/selecter config atm..
				{
					AddToMenu(hMenu, SWS_SEPARATOR, 0);
					int x=0, nbsel=0; while(m_pLists.Get(0)->EnumSelected(&x))nbsel++;
					AddToMenu(hMenu, __LOCALIZE("Project loader/selecter configuration...","sws_DLG_150"), PRJ_LOADER_CONF_MSG);
					AddToMenu(hMenu, __LOCALIZE("Set project loader/selecter from selection","sws_DLG_150"), PRJ_LOADER_SET_MSG, -1, false, nbsel>1 ? MF_ENABLED : MF_GRAYED);
					AddToMenu(hMenu, __LOCALIZE("Clear project loader/selecter configuration","sws_DLG_150"), PRJ_LOADER_CLEAR_MSG, -1, false, IsProjectLoaderConfValid() ? MF_ENABLED : MF_GRAYED);
				}
				break;
			}
			case SNM_SLOT_MEDIA:
				AddToMenu(hMenu, MED_PLAY_STR, MED_PLAY_MSG, -1, false, enabled);
				AddToMenu(hMenu, MED_LOOP_STR, MED_LOOP_MSG, -1, false, enabled);
				AddToMenu(hMenu, SWS_SEPARATOR, 0);
				AddToMenu(hMenu, __LOCALIZE("Add to current track","sws_DLG_150"), MED_ADD_CURTR_MSG, -1, false, enabled);
				AddToMenu(hMenu, __LOCALIZE("Add to new tracks","sws_DLG_150"), MED_ADD_NEWTR_MSG, -1, false, enabled);
				AddToMenu(hMenu, __LOCALIZE("Add to selected items as takes","sws_DLG_150"), MED_ADD_TAKES_MSG, -1, false, enabled);
				break;
			case SNM_SLOT_IMG:
				AddToMenu(hMenu, IMG_SHOW_STR, IMG_SHOW_MSG, -1, false, enabled);
				AddToMenu(hMenu, IMG_TRICON_STR, IMG_TRICON_MSG, -1, false, enabled);
				AddToMenu(hMenu, __LOCALIZE("Add to current track as item","sws_DLG_150"), MED_ADD_CURTR_MSG, -1, false, enabled);
				break;
#ifdef _WIN32
			case SNM_SLOT_THM:
				AddToMenu(hMenu, THM_LOAD_STR, THM_LOAD_MSG, -1, false, enabled);
				break;
#endif
		}
	}

	if (GetMenuItemCount(hMenu))
		AddToMenu(hMenu, SWS_SEPARATOR, 0);

	// always displayed, even if the list is empty
	AddToMenu(hMenu, __LOCALIZE("Add slot","sws_DLG_150"), ADD_SLOT_MSG, -1, false, !IsFiltered() ? MF_ENABLED : MF_GRAYED);

	if (pItem && iCol >= 0)
	{
		AddToMenu(hMenu, __LOCALIZE("Insert slot","sws_DLG_150"), INSERT_SLOT_MSG, -1, false, !IsFiltered() ? MF_ENABLED : MF_GRAYED);
		AddToMenu(hMenu, __LOCALIZE("Clear slots","sws_DLG_150"), CLEAR_SLOTS_MSG, -1, false, enabled);
		AddToMenu(hMenu, __LOCALIZE("Delete slots","sws_DLG_150"), DEL_SLOTS_MSG);

		AddToMenu(hMenu, SWS_SEPARATOR, 0);
		AddToMenu(hMenu, __LOCALIZE("Load slot/file...","sws_DLG_150"), LOAD_MSG);
		AddToMenu(hMenu, __LOCALIZE("Delete files","sws_DLG_150"), DEL_FILES_MSG, -1, false, enabled);
		AddToMenu(hMenu, __LOCALIZE("Rename file","sws_DLG_150"), RENAME_MSG, -1, false, enabled);
		if (GetSlotList()->HasNotepad())
#ifdef _WIN32
			AddToMenu(hMenu, __LOCALIZE("Edit file...","sws_DLG_150"), EDIT_MSG, -1, false, enabled);
#else
			AddToMenu(hMenu, __LOCALIZE("Display file...","sws_DLG_150"), EDIT_MSG, -1, false, enabled);
#endif
		AddToMenu(hMenu, __LOCALIZE("Show path in explorer/finder...","sws_DLG_150"), EXPLORE_MSG, -1, false, enabled);
	}
	else
	{
		// auto-fill
		AddToMenu(hMenu, SWS_SEPARATOR, 0);
		HMENU hAutoFillSubMenu = CreatePopupMenu();
		AddSubMenu(hMenu, hAutoFillSubMenu, __LOCALIZE("Auto-fill","sws_DLG_150"));
		AutoFillContextMenu(hAutoFillSubMenu, true);

		// auto-save
		if (GetSlotList()->HasAutoSave())
		{
			HMENU hAutoSaveSubMenu = CreatePopupMenu();
			AddSubMenu(hMenu, hAutoSaveSubMenu, __LOCALIZE("Auto-save","sws_DLG_150"));
			AutoSaveContextMenu(hAutoSaveSubMenu, true);
		}

		// bookmarks
//		AddToMenu(hMenu, SWS_SEPARATOR, 0);
		HMENU hBookmarkSubMenu = CreatePopupMenu();
		AddSubMenu(hMenu, hBookmarkSubMenu, __LOCALIZE("Bookmarks","sws_DLG_150"));
		HMENU hNewBookmarkSubMenu = CreatePopupMenu();
		AddSubMenu(hBookmarkSubMenu, hNewBookmarkSubMenu, __LOCALIZE("New bookmark","sws_DLG_150"));
		for (int i=0; i < SNM_NUM_DEFAULT_SLOTS; i++)
			AddToMenu(hNewBookmarkSubMenu, GetMenuDesc(i), NEW_BOOKMARK_FXC_MSG+i);
		AddToMenu(hBookmarkSubMenu, __LOCALIZE("Copy bookmark...","sws_DLG_150"), COPY_BOOKMARK_MSG);
		AddToMenu(hBookmarkSubMenu, __LOCALIZE("Rename...","sws_DLG_150"), REN_BOOKMARK_MSG, -1, false, g_resViewType >= SNM_NUM_DEFAULT_SLOTS ? MF_ENABLED : MF_GRAYED);
		AddToMenu(hBookmarkSubMenu, __LOCALIZE("Delete","sws_DLG_150"), DEL_BOOKMARK_MSG, -1, false, g_resViewType >= SNM_NUM_DEFAULT_SLOTS ? MF_ENABLED : MF_GRAYED);

		// filter prefs
		AddToMenu(hMenu, SWS_SEPARATOR, 0);
		HMENU hFilterSubMenu = CreatePopupMenu();
		AddSubMenu(hMenu, hFilterSubMenu, __LOCALIZE("Filter on","sws_DLG_150"));
		AddToMenu(hFilterSubMenu, __LOCALIZE("Name","sws_DLG_150"), FILTER_BY_NAME_MSG, -1, false, (g_filterPref&1) ? MFS_CHECKED : MFS_UNCHECKED);
		AddToMenu(hFilterSubMenu, __LOCALIZE("Path","sws_DLG_150"), FILTER_BY_PATH_MSG, -1, false, (g_filterPref&2) ? MFS_CHECKED : MFS_UNCHECKED);
		AddToMenu(hFilterSubMenu, __LOCALIZE("Comment","sws_DLG_150"), FILTER_BY_COMMENT_MSG, -1, false, (g_filterPref&4) ? MFS_CHECKED : MFS_UNCHECKED);
	}
	return hMenu;
}

int SNM_ResourceWnd::OnKey(MSG* _msg, int _iKeyState) 
{
	if (_msg->message == WM_KEYDOWN && !_iKeyState)
	{
		switch(_msg->wParam)
		{
			case VK_F2:
				OnCommand(RENAME_MSG, 0);
				return 1;
			case VK_DELETE:
				ClearDeleteSlots(1, true);
				return 1;
			case VK_INSERT:
				InsertAtSelectedSlot(true);
				return 1;
			case VK_RETURN:
				((SNM_ResourceView*)m_pLists.Get(0))->Perform();
				return 1;
		}
	}
	return 0;
}

int SNM_ResourceWnd::GetValidDroppedFilesCount(HDROP _h)
{
	int validCnt=0;
	int iFiles = DragQueryFile(_h, 0xFFFFFFFF, NULL, 0);
	char cFile[BUFFER_SIZE];
	for (int i=0; i < iFiles; i++) 
	{
		DragQueryFile(_h, i, cFile, BUFFER_SIZE);
		if (!strcmp(cFile, DRAGDROP_EMPTY_SLOT) || g_slots.Get(GetTypeForUser())->IsValidFileExt(GetFileExtension(cFile)))
			validCnt++;
	}
	return validCnt;
}

void SNM_ResourceWnd::OnDroppedFiles(HDROP _h)
{
	int dropped = 0; //nb of successfully dropped files
	int iFiles = DragQueryFile(_h, 0xFFFFFFFF, NULL, 0);
	int iValidFiles = GetValidDroppedFilesCount(_h);

	// Check to see if we dropped on a group
	POINT pt;
	DragQueryPoint(_h, &pt);

	RECT r; // ClientToScreen doesn't work right, wtf?
	GetWindowRect(m_hwnd, &r);
	pt.x += r.left;
	pt.y += r.top;

	PathSlotItem* pItem = (PathSlotItem*)m_pLists.Get(0)->GetHitItem(pt.x, pt.y, NULL);
	int dropSlot = GetSlotList()->Find(pItem);

	// internal d'n'd ?
	if (g_dragPathSlotItems.GetSize())
	{
		int srcSlot = GetSlotList()->Find(g_dragPathSlotItems.Get(0));
		// drag'n'drop slot to the bottom
		if (srcSlot >= 0 && srcSlot < dropSlot)
			dropSlot++; // d'n'd will be more "natural"
	}

	// drop but not on a slot => create slots
	if (!pItem || dropSlot < 0 || dropSlot >= GetSlotList()->GetSize()) 
	{
		dropSlot = GetSlotList()->GetSize();
		for (int i=0; i < iValidFiles; i++)
			GetSlotList()->Add(new PathSlotItem());
	}
	// drop on a slot => insert slots at drop point
	else 
	{
		for (int i=0; i < iValidFiles; i++)
			GetSlotList()->InsertSlot(dropSlot);
	}

	// re-sync pItem 
	pItem = GetSlotList()->Get(dropSlot); 

	// Patch added/inserted slots from dropped data
	char cFile[BUFFER_SIZE];
	int slot;
	for (int i=0; pItem && i < iFiles; i++)
	{
		slot = GetSlotList()->Find(pItem);
		DragQueryFile(_h, i, cFile, BUFFER_SIZE);

		// internal d'n'd ?
		if (g_dragPathSlotItems.GetSize())
		{
			if (PathSlotItem* item = GetSlotList()->Get(slot))
			{
				item->m_shortPath.Set(g_dragPathSlotItems.Get(i)->m_shortPath.Get());
				item->m_comment.Set(g_dragPathSlotItems.Get(i)->m_comment.Get());
				dropped++;
				pItem = GetSlotList()->Get(slot+1); 
			}
		}
		// .rfxchain? .rTrackTemplate? etc..
		else if (g_slots.Get(GetTypeForUser())->IsValidFileExt(GetFileExtension(cFile))) {
			if (GetSlotList()->SetFromFullPath(slot, cFile)) {
				dropped++;
				pItem = GetSlotList()->Get(slot+1); 
			}
		}
	}

	if (dropped)
	{
		// internal drag'n'drop: move (=> delete previous slots)
		for (int i=0; i < g_dragPathSlotItems.GetSize(); i++)
			for (int j=GetSlotList()->GetSize()-1; j >= 0; j--)
				if (GetSlotList()->Get(j) == g_dragPathSlotItems.Get(i)) {
					if (j < dropSlot) dropSlot--;
					GetSlotList()->Delete(j, false);
				}

		Update();

		// Select item at drop point
		if (dropSlot >= 0)
			SelectBySlot(dropSlot);
	}

	g_dragPathSlotItems.Empty(false);
	DragFinish(_h);
}

void SNM_ResourceWnd::DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight)
{
	// 1st row of controls
	int x0=_r->left+10, h=35;
	if (_tooltipHeight)
		*_tooltipHeight = h;

	LICE_CachedFont* font = SNM_GetThemeFont();
	IconTheme* it = SNM_GetIconTheme();
	int typeForUser = GetTypeForUser();

	// "auto-fill" button
	SNM_SkinButton(&m_btnAutoFill, it ? &(it->toolbar_open) : NULL, __LOCALIZE("Auto-fill","sws_DLG_150"));
	if (SNM_AutoVWndPosition(&m_btnAutoFill, NULL, _r, &x0, _r->top, h, 0))
	{
		// "auto-save" button
		m_btnAutoSave.SetGrayed(!GetSlotList()->HasAutoSave());
		SNM_SkinButton(&m_btnAutoSave, it ? &(it->toolbar_save) : NULL, __LOCALIZE("Auto-save","sws_DLG_150"));
		if (SNM_AutoVWndPosition(&m_btnAutoSave, NULL, _r, &x0, _r->top, h))
		{
			// type dropdown
			m_cbType.SetFont(font);
			if (SNM_AutoVWndPosition(&m_cbType, NULL, _r, &x0, _r->top, h, 4))
			{
				// add & del bookmark buttons
				((SNM_AddDelButton*)m_parentVwnd.GetChildByID(BUTTONID_DEL_BOOKMARK))->SetEnabled(g_resViewType >= SNM_NUM_DEFAULT_SLOTS);
				if (SNM_AutoVWndPosition(&m_btnsAddDel, NULL, _r, &x0, _r->top, h))
				{
					if (IsMultiType())
					{
						m_btnTiedActions.SetCheckState(g_tiedSlotActions[typeForUser] == g_resViewType);
						char buf[64] = "";
						_snprintfSafe(buf, sizeof(buf), __LOCALIZE_VERFMT("Tie %s slot actions","sws_DLG_150"), g_slots.Get(typeForUser)->GetDesc());
						m_btnTiedActions.SetTextLabel(buf, -1, font);
						if (SNM_AutoVWndPosition(&m_btnTiedActions, NULL, _r, &x0, _r->top, h, 5))
							SNM_AddLogo(_bm, _r, x0, h);
					}
					else
						SNM_AddLogo(_bm, _r, x0, h);
				}
			}
		}
	}

	// 2nd row of controls
	x0 = _r->left+8; h=39;
	int y0 = _r->bottom-h;

	// defines a new rect r that takes the filter edit box into account (contrary to _r)
	RECT r;
	GetWindowRect(GetDlgItem(m_hwnd, IDC_FILTER), &r);
	ScreenToClient(m_hwnd, (LPPOINT)&r);
	ScreenToClient(m_hwnd, ((LPPOINT)&r)+1);
	r.top = _r->top; r.bottom = _r->bottom;
	r.right = r.left; r.left = _r->left;

	// "dbl-click to"
	if (GetSlotList()->HasDblClick())
	{
		m_txtDblClickType.SetFont(font);
		if (!SNM_AutoVWndPosition(&m_txtDblClickType, NULL, &r, &x0, y0, h, 5))
			return;
		m_cbDblClickType.SetFont(font);
		if (!SNM_AutoVWndPosition(&m_cbDblClickType, &m_txtDblClickType, &r, &x0, y0, h))
			return;

		switch (typeForUser)
		{
			// "to selected" (fx chain only)
			case SNM_SLOT_FXC:
				m_txtDblClickTo.SetFont(font);
				if (!SNM_AutoVWndPosition(&m_txtDblClickTo, NULL, &r, &x0, y0, h, 5))
					return;
				m_cbDblClickTo.SetFont(font);
				if (!SNM_AutoVWndPosition(&m_cbDblClickTo, &m_txtDblClickTo, &r, &x0, y0, h))
					return;
				break;
			// offset items & envs (tr template only)
			case SNM_SLOT_TR:
				if (int* offsPref = (int*)GetConfigVar("templateditcursor")) // >= REAPER v4.15
				{
					int dblClickPrefs = LOWORD(g_dblClickPrefs[g_resViewType]);
					bool showOffsOption = true;
					switch(dblClickPrefs) {
						case 1: // apply to sel tracks
							showOffsOption = false; // no offset option
							break;
						case 3: // paste template items
						case 4: // paste (replace) template items
							m_btnOffsetTrTemplate.SetTextLabel(__LOCALIZE("Offset template items by edit cursor","sws_DLG_150"), -1, font);
							break;
						default: // other dbl-click prefs
							m_btnOffsetTrTemplate.SetTextLabel(__LOCALIZE("Offset template items/envs by edit cursor","sws_DLG_150"), -1, font);
							break;
					}
					if (showOffsOption) {
						m_btnOffsetTrTemplate.SetCheckState(*offsPref);
						if (!SNM_AutoVWndPosition(&m_btnOffsetTrTemplate, NULL, &r, &x0, y0, h, 5))
							return;
					}
				}
				break;
		}
	}
}

//JFB hard coded labels.. ideally it should be the same than related action names
bool SNM_ResourceWnd::GetToolTipString(int _xpos, int _ypos, char* _bufOut, int _bufOutSz)
{
	if (WDL_VWnd* v = m_parentVwnd.VirtWndFromPoint(_xpos,_ypos,1))
	{
		int typeForUser = GetTypeForUser();
		switch (v->GetID())
		{
			case BUTTONID_AUTOFILL:
				return (_snprintfStrict(_bufOut, _bufOutSz, __LOCALIZE_VERFMT("Auto-fill %s slots (right-click for options)\nfrom %s","sws_DLG_150"), 
					g_slots.Get(typeForUser)->GetDesc(), 
					*GetAutoFillDir() ? GetAutoFillDir() : __LOCALIZE("undefined","sws_DLG_150")) > 0);
			case BUTTONID_AUTOSAVE:
				if (g_slots.Get(g_resViewType)->HasAutoSave())
				{
					switch (typeForUser)
					{
						case SNM_SLOT_FXC:
							return (_snprintfStrict(_bufOut, _bufOutSz, __LOCALIZE_VERFMT("%s (right-click for options)\nto %s","sws_DLG_150"),
								g_autoSaveFXChainPref == FXC_AUTOSAVE_PREF_TRACK ? __LOCALIZE("Auto-save FX chains for selected tracks","sws_DLG_150") :
								g_autoSaveFXChainPref == FXC_AUTOSAVE_PREF_ITEM ? __LOCALIZE("Auto-save FX chains for selected items","sws_DLG_150") :
								g_autoSaveFXChainPref == FXC_AUTOSAVE_PREF_INPUT_FX ? __LOCALIZE("Auto-save input FX chains for selected tracks","sws_DLG_150")
									: __LOCALIZE("Auto-save FX chain slots","sws_DLG_150"),
								*GetAutoSaveDir() ? GetAutoSaveDir() : __LOCALIZE("undefined","sws_DLG_150")) > 0);
						case SNM_SLOT_TR:
							return (_snprintfStrict(_bufOut, _bufOutSz, __LOCALIZE_VERFMT("Auto-save track templates%s%s for selected tracks (right-click for options)\nto %s","sws_DLG_150"),
								(g_autoSaveTrTmpltPref&1) ? __LOCALIZE(" w/ items","sws_DLG_150") : "",
								(g_autoSaveTrTmpltPref&2) ? __LOCALIZE(" w/ envs","sws_DLG_150") : "",
								*GetAutoSaveDir() ? GetAutoSaveDir() : __LOCALIZE("undefined","sws_DLG_150")) > 0);
						default:
							return (_snprintfStrict(_bufOut, _bufOutSz, __LOCALIZE_VERFMT("Auto-save %s slots (right-click for options)\nto %s","sws_DLG_150"), 
								g_slots.Get(typeForUser)->GetDesc(), 
								*GetAutoSaveDir() ? GetAutoSaveDir() : __LOCALIZE("undefined","sws_DLG_150")) > 0);
					}
				}
				break;
			case COMBOID_TYPE:
				return (lstrcpyn(_bufOut, __LOCALIZE("Resource/slot type","sws_DLG_150"), _bufOutSz) != NULL);
			case BUTTONID_ADD_BOOKMARK:
				return (lstrcpyn(_bufOut, __LOCALIZE("New bookmark","sws_DLG_150"), _bufOutSz) != NULL);
			case BUTTONID_DEL_BOOKMARK:
				return (lstrcpyn(_bufOut, __LOCALIZE("Delete bookmark (and files, if confirmed)","sws_DLG_150"), _bufOutSz) != NULL);
		}
	}
	return false;
}

HBRUSH SNM_ResourceWnd::OnColorEdit(HWND _hwnd, HDC _hdc)
{
	int bg, txt; bool match=false;
	if (_hwnd == GetDlgItem(m_hwnd, IDC_EDIT)) {
		match = true;
		SNM_GetThemeListColors(&bg, &txt);
	}
	else if (_hwnd == GetDlgItem(m_hwnd, IDC_FILTER)) {
		match = true;
		SNM_GetThemeEditColors(&bg, &txt);
	}

	if (match) {
		SetBkColor(_hdc, bg);
		SetTextColor(_hdc, txt);
		return SNM_GetThemeBrush(bg);
	}
	return 0;
}

void SNM_ResourceWnd::ClearListSelection()
{
	SWS_ListView* lv = m_pLists.Get(0);
	HWND hList = lv ? lv->GetHWND() : NULL;
	if (hList) // can be called when the view is closed!
		ListView_SetItemState(hList, -1, 0, LVIS_SELECTED);
}

// select a range of slots or a single slot (when _slot2 < 0)
void SNM_ResourceWnd::SelectBySlot(int _slot1, int _slot2)
{
	SWS_ListView* lv = m_pLists.Get(0);
	HWND hList = lv ? lv->GetHWND() : NULL;
	if (lv && hList) // can be called when the view is closed!
	{
		int slot1=_slot1, slot2=_slot2;

		// check params
		if (_slot1 < 0)
			return;
		if (_slot2 < 0)
			slot2 = slot1;
		else if (_slot2 < _slot1) {
			slot1 = _slot2;
			slot2 = _slot1;
		}

		ListView_SetItemState(hList, -1, 0, LVIS_SELECTED);

		int firstSel = -1;
		for (int i=0; i < lv->GetListItemCount(); i++)
		{
			PathSlotItem* item = (PathSlotItem*)lv->GetListItem(i);
			int slot = GetSlotList()->Find(item);
			if (item && slot >= _slot1 && slot <= slot2) 
			{
				if (firstSel < 0)
					firstSel = i;
				ListView_SetItemState(hList, i, LVIS_SELECTED, LVIS_SELECTED);
				if (_slot2 < 0) // one slot to be selected?
					break;
			}
		}
		if (firstSel >= 0)
			ListView_EnsureVisible(hList, firstSel, true);
	}
}

void SNM_ResourceWnd::AddSlot(bool _update)
{
	int idx = GetSlotList()->GetSize();
	if (GetSlotList()->Add(new PathSlotItem()) && _update) {
		Update();
		SelectBySlot(idx);
	}
}

void SNM_ResourceWnd::InsertAtSelectedSlot(bool _update)
{
	if (GetSlotList()->GetSize())
	{
		bool updt = false;
		PathSlotItem* item = (PathSlotItem*)m_pLists.Get(0)->EnumSelected(NULL);
		if (item)
		{
			int slot = GetSlotList()->Find(item);
			if (slot >= 0)
				updt = (GetSlotList()->InsertSlot(slot) != NULL);
			if (_update && updt) 
			{
				Update();
				SelectBySlot(slot);
				return; // <-- !!
			}
		}
	}
	AddSlot(_update); // empty list, no selection, etc.. => add
}

// _mode&1 = delete sel. slots, clear sel. slots otherwise
// _mode&2 = clear sel. slots or delete if empty & last sel. slots
// _mode&4 = delete files
// _mode&8 = delete all slots (exclusive with _mode&1 and _mode&2)
void SNM_ResourceWnd::ClearDeleteSlots(int _mode, bool _update)
{
	bool updt = false;
	int oldSz = GetSlotList()->GetSize();

	WDL_PtrList<int> slots;
	while (slots.GetSize() != GetSlotList()->GetSize())
		slots.Add(_mode&8 ? &g_i1 : &g_i0);

	int x=0;
	if ((_mode&8) != 8)
	{
		while(PathSlotItem* item = (PathSlotItem*)m_pLists.Get(0)->EnumSelected(&x)) {
			int slot = GetSlotList()->Find(item);
			slots.Delete(slot);
			slots.Insert(slot, &g_i1);
		}
	}

	int endSelSlot=-1;
	x=GetSlotList()->GetSize();
	while(x >= 0 && slots.Get(--x) == &g_i1) endSelSlot=x;

	char fullPath[BUFFER_SIZE] = "";
	WDL_PtrList_DeleteOnDestroy<PathSlotItem> delItems; // DeleteOnDestroy: keep (displayed!) pointers until the list view is updated
	for (int slot=slots.GetSize()-1; slot >=0 ; slot--)
	{
		if (*slots.Get(slot)) // avoids new Find(item) calls
		{
			if (PathSlotItem* item = GetSlotList()->Get(slot))
			{
				if (_mode&4) {
					GetFullResourcePath(GetSlotList()->GetResourceDir(), item->m_shortPath.Get(), fullPath, BUFFER_SIZE);
					SNM_DeleteFile(fullPath, true);
				}

				if (_mode&1 || _mode&8 || (_mode&2 && endSelSlot>=0 && slot>=endSelSlot)) 
				{
					slots.Delete(slot, false); // keep the sel list "in sync"
					GetSlotList()->Delete(slot, false); // remove slot - but no deleted pointer yet
					delItems.Add(item); // for later pointer deletion..
				}
				else
					GetSlotList()->ClearSlot(slot, false);
				updt = true;
			}
		}
	}

	if (_update && updt)
	{
		Update();
		if (oldSz != GetSlotList()->GetSize()) // deletion?
			ClearListSelection();
	}

} // + delItems auto clean-up !


///////////////////////////////////////////////////////////////////////////////

bool CheckSetAutoDirectory(const char* _title, WDL_FastString* _dir)
{
	if (!FileExists(_dir->Get()))
	{
		char buf[BUFFER_SIZE] = "";
		_snprintfSafe(buf, sizeof(buf), __LOCALIZE_VERFMT("%s directory not found!\n%s%sDo you want to define one ?","sws_DLG_150"), _title, _dir->GetLength()?_dir->Get():"", _dir->GetLength()?"\n":"");
		switch(MessageBox(g_pResourcesWnd?g_pResourcesWnd->GetHWND():GetMainHwnd(), buf, __LOCALIZE("S&M - Warning","sws_DLG_150"), MB_YESNO))
		{
			case IDYES: 
				if (BrowseForDirectory("S&&M - Set directory", GetResourcePath(), buf, BUFFER_SIZE))
					_dir->Set(buf);
				break;
			case IDNO:
				return false;
		}
		if (!FileExists(_dir->Get())) // re-check (cancel, etc..)
			return false;
	}
	return true;
}

// _flags:
//   for track templates: _flags&1 save template with items, _flags&2 save template with envs
//   for fx chains: enum FXC_AUTOSAVE_PREF_INPUT_FX, FXC_AUTOSAVE_PREF_TRACK and FXC_AUTOSAVE_PREF_ITEM
void AutoSave(int _type, int _flags)
{
	if (!CheckSetAutoDirectory(__LOCALIZE("Auto-save","sws_DLG_150"), g_autoSaveDirs.Get(_type)))
		return;

	int slotStart = g_slots.Get(_type)->GetSize();
	char fn[BUFFER_SIZE] = "";
	switch(GetTypeForUser(_type))
	{
		case SNM_SLOT_FXC:
			switch (_flags)
			{
				case FXC_AUTOSAVE_PREF_INPUT_FX:
				case FXC_AUTOSAVE_PREF_TRACK:
					AutoSaveTrackFXChainSlots(_type, g_autoSaveDirs.Get(_type)->Get(), fn, BUFFER_SIZE, g_autoSaveFXChainNamePref==1, _flags==FXC_AUTOSAVE_PREF_INPUT_FX);
					break;
				case FXC_AUTOSAVE_PREF_ITEM:
					AutoSaveItemFXChainSlots(_type, g_autoSaveDirs.Get(_type)->Get(), fn, BUFFER_SIZE, g_autoSaveFXChainNamePref==1);
					break;
			}
			break;
		case SNM_SLOT_TR:
			AutoSaveTrackSlots(_type, g_autoSaveDirs.Get(_type)->Get(), fn, BUFFER_SIZE, (_flags&1)?false:true, (_flags&2)?false:true);
			break;
		case SNM_SLOT_PRJ:
			AutoSaveProjectSlot(_type, g_autoSaveDirs.Get(_type)->Get(), fn, BUFFER_SIZE, true);
			break;
		case SNM_SLOT_MEDIA:
			AutoSaveMediaSlot(_type, g_autoSaveDirs.Get(_type)->Get(), fn, BUFFER_SIZE);
			break;
	}

	if (slotStart != g_slots.Get(_type)->GetSize())
	{
		if (g_pResourcesWnd && g_resViewType==_type) {
			g_pResourcesWnd->Update();
			g_pResourcesWnd->SelectBySlot(slotStart, g_slots.Get(_type)->GetSize());
		}
	}
	else
	{
		char msg[BUFFER_SIZE];
		if (fn && *fn) _snprintfSafe(msg, sizeof(msg), __LOCALIZE_VERFMT("Auto-save failed: %s\n%s","sws_DLG_150"), fn, AUTOSAVE_ERR_STR);
		else _snprintfSafe(msg, sizeof(msg), __LOCALIZE_VERFMT("Auto-save failed!\n%s","sws_DLG_150"), AUTOSAVE_ERR_STR);
		MessageBox(g_pResourcesWnd?g_pResourcesWnd->GetHWND():GetMainHwnd(), msg, __LOCALIZE("S&M - Error","sws_DLG_150"), MB_OK);
	}
}

// recursive from _path
void AutoFill(int _type)
{
	if (!CheckSetAutoDirectory(__LOCALIZE("Auto-fill","sws_DLG_150"), g_autoFillDirs.Get(_type)))
		return;

	int startSlot = g_slots.Get(_type)->GetSize();
	WDL_PtrList_DeleteOnDestroy<WDL_String> files; 
	ScanFiles(&files, g_autoFillDirs.Get(_type)->Get(), g_slots.Get(_type)->GetFileExt(), true);
	if (int sz = files.GetSize())
		for (int i=0; i < sz; i++)
			if (g_slots.Get(_type)->FindByResFulltPath(files.Get(i)->Get()) < 0) // skip if already present
				g_slots.Get(_type)->AddSlot(files.Get(i)->Get());

	if (startSlot != g_slots.Get(_type)->GetSize())
	{
		if (g_pResourcesWnd && g_resViewType==_type) {
			g_pResourcesWnd->Update();
			g_pResourcesWnd->SelectBySlot(startSlot, g_slots.Get(_type)->GetSize());
		}
	}
	else
	{
		const char* path = g_autoFillDirs.Get(_type)->Get();
		char msg[BUFFER_SIZE]="";
		if (path && *path) _snprintfSafe(msg, sizeof(msg), __LOCALIZE_VERFMT("No slot added from: %s\n%s","sws_DLG_150"), path, AUTOFILL_ERR_STR);
		else _snprintfSafe(msg, sizeof(msg), __LOCALIZE_VERFMT("No slot added!\n%s","sws_DLG_150"), AUTOFILL_ERR_STR);
		MessageBox(g_pResourcesWnd?g_pResourcesWnd->GetHWND():GetMainHwnd(), msg, __LOCALIZE("S&M - Warning","sws_DLG_150"), MB_OK);
	}
}


///////////////////////////////////////////////////////////////////////////////

void NewBookmark(int _type, bool _copyCurrent)
{
	if (g_slots.GetSize() < SNM_MAX_SLOT_TYPES)
	{
		// consistency update (just in case..)
		_type = (_copyCurrent ? g_resViewType : GetTypeForUser(_type));

		char name[64] = "";
		_snprintfSafe(name, sizeof(name), __LOCALIZE_VERFMT("My %s","sws_DLG_150"), g_slots.Get(GetTypeForUser(_type))->GetMenuDesc());
		if (PromptUserForString(g_pResourcesWnd?g_pResourcesWnd->GetHWND():GetMainHwnd(), __LOCALIZE("S&M - Add resource bookmark","sws_DLG_150"), name, 64) && *name)
		{
			int newType = g_slots.GetSize();

			g_slots.Add(new FileSlotList(
				g_slots.Get(_type)->GetResourceDir(), name, g_slots.Get(_type)->GetFileExt(), 
				g_slots.Get(_type)->HasNotepad(), g_slots.Get(_type)->HasAutoSave(), g_slots.Get(_type)->HasDblClick()));

			if (_copyCurrent)
			{
				g_autoSaveDirs.Add(new WDL_FastString(GetAutoSaveDir(_type)));
				g_autoFillDirs.Add(new WDL_FastString(GetAutoFillDir(_type)));
			}
			else
			{
				char path[BUFFER_SIZE] = "";
				if (_snprintfStrict(path, sizeof(path), "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, g_slots.Get(_type)->GetResourceDir()) > 0) {
					if (!FileExists(path))
						CreateDirectory(path, NULL);
				}
				else *path = '\0';
				g_autoSaveDirs.Add(new WDL_FastString(path));
				g_autoFillDirs.Add(new WDL_FastString(path));
			}
			g_syncAutoDirPrefs[newType] = _copyCurrent ? g_syncAutoDirPrefs[_type] : true;
			g_dblClickPrefs[newType] = _copyCurrent ? g_dblClickPrefs[_type] : 0;

			if (_copyCurrent)
				for (int i=0; i < GetSlotList()->GetSize(); i++)
					if (PathSlotItem* slotItem = GetSlotList()->Get(i))
						g_slots.Get(newType)->AddSlot(slotItem->m_shortPath.Get(), slotItem->m_comment.Get());

			if (g_pResourcesWnd) {
				g_pResourcesWnd->FillTypeCombo();
				g_pResourcesWnd->SetType(newType); // + GUI update
			}

			// ... custom slot type definitions are saved when exiting
		}
	}
	else
		MessageBox(g_pResourcesWnd?g_pResourcesWnd->GetHWND():GetMainHwnd(), __LOCALIZE("Too many resource types!","sws_DLG_150"), __LOCALIZE("S&M - Error","sws_DLG_150"), MB_OK);
}

void DeleteBookmark(int _bookmarkType)
{
	if (_bookmarkType >= SNM_NUM_DEFAULT_SLOTS)
	{
		int reply = IDNO;
		if (g_slots.Get(_bookmarkType)->GetSize()) // do not ask if empty
			reply = MessageBox(g_pResourcesWnd?g_pResourcesWnd->GetHWND():GetMainHwnd(), __LOCALIZE("Delete files too ?","sws_DLG_150"), __LOCALIZE("S&M - Delete resource bookmark","sws_DLG_150"), MB_YESNOCANCEL);
		if (reply != IDCANCEL)
		{
			// cleanup ini file (types may not be contiguous anymore..)
			FlushCustomTypesIniFile();
			if (g_pResourcesWnd)
				g_pResourcesWnd->ClearDeleteSlots(8 | (reply==IDYES?4:0), true); // true so that deleted items are not displayed anymore

			// remove current slot type
			int oldType = _bookmarkType;
			int oldTypeForUser = GetTypeForUser(oldType);
			g_dblClickPrefs[oldType] = 0;
			if (g_tiedSlotActions[oldTypeForUser] == oldType)
				g_tiedSlotActions[oldTypeForUser] = oldTypeForUser;
			g_autoSaveDirs.Delete(oldType, true);
			g_autoFillDirs.Delete(oldType, true);
			g_syncAutoDirPrefs[oldType] = true;
			g_slots.Delete(oldType, true);

			if (g_pResourcesWnd) {
				g_pResourcesWnd->FillTypeCombo();
				g_pResourcesWnd->SetType(oldType-1); // + GUI update
			}
		}
	}
}

void RenameBookmark(int _bookmarkType)
{
	if (_bookmarkType >= SNM_NUM_DEFAULT_SLOTS)
	{
		char newName[64] = "";
		lstrcpyn(newName, g_slots.Get(_bookmarkType)->GetDesc(), 64);
		if (PromptUserForString(g_pResourcesWnd?g_pResourcesWnd->GetHWND():GetMainHwnd(), __LOCALIZE("S&M - Rename bookmark","sws_DLG_150"), newName, 64) && *newName)
		{
			g_slots.Get(_bookmarkType)->SetDesc(newName);
			if (g_pResourcesWnd) {
				g_pResourcesWnd->FillTypeCombo();
				g_pResourcesWnd->Update();
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////////////

void FlushCustomTypesIniFile()
{
	char iniSec[64]="";
	for (int i=SNM_NUM_DEFAULT_SLOTS; i < g_slots.GetSize(); i++)
	{
		if (g_slots.Get(i))
		{
			GetIniSectionName(i, iniSec, sizeof(iniSec));
			WritePrivateProfileStruct(iniSec, NULL, NULL, 0, g_SNMIniFn.Get()); // flush section
			WritePrivateProfileString("RESOURCE_VIEW", iniSec, NULL, g_SNMIniFn.Get());
			WDL_FastString str;
			str.SetFormatted(BUFFER_SIZE, "AutoFillDir%s", iniSec);
			WritePrivateProfileString("RESOURCE_VIEW", str.Get(), NULL, g_SNMIniFn.Get());
			str.SetFormatted(BUFFER_SIZE, "AutoSaveDir%s", iniSec);
			WritePrivateProfileString("RESOURCE_VIEW", str.Get(), NULL, g_SNMIniFn.Get());
			str.SetFormatted(BUFFER_SIZE, "SyncAutoDirs%s", iniSec);
			WritePrivateProfileString("RESOURCE_VIEW", str.Get(), NULL, g_SNMIniFn.Get());
			str.SetFormatted(BUFFER_SIZE, "TiedActions%s", iniSec);
			WritePrivateProfileString("RESOURCE_VIEW", str.Get(), NULL, g_SNMIniFn.Get());
			str.SetFormatted(BUFFER_SIZE, "DblClick%s", iniSec);
			WritePrivateProfileString("RESOURCE_VIEW", str.Get(), NULL, g_SNMIniFn.Get());
		}
	}
}

void ReadSlotIniFile(const char* _key, int _slot, char* _path, int _pathSize, char* _desc, int _descSize)
{
	char buf[32];
	if (_snprintfStrict(buf, sizeof(buf), "Slot%d", _slot+1) > 0)
		GetPrivateProfileString(_key, buf, "", _path, _pathSize, g_SNMIniFn.Get());
	if (_snprintfStrict(buf, sizeof(buf), "Desc%d", _slot+1) > 0)
		GetPrivateProfileString(_key, buf, "", _desc, _descSize, g_SNMIniFn.Get());
}

// adds bookmarks and custom slot types from the S&M.ini file, example: 
// CustomSlotType1="Data\track_icons,track icons,png"
// CustomSlotType2="TextFiles,text file,txt"
void AddCustomTypesFromIniFile()
{
	int i=0; char buf[BUFFER_SIZE]=""; 
	WDL_String iniKeyStr("CustomSlotType1"), tokenStrs[2];
	GetPrivateProfileString("RESOURCE_VIEW", iniKeyStr.Get(), "", buf, BUFFER_SIZE, g_SNMIniFn.Get());
	while (*buf && i < SNM_MAX_SLOT_TYPES)
	{
		if (char* tok = strtok(buf, ","))
		{
			if (tok[strlen(tok)-1] == PATH_SLASH_CHAR) tok[strlen(tok)-1] = '\0';
			tokenStrs[0].Set(tok);
			if (tok = strtok(NULL, ","))
			{
				tokenStrs[1].Set(tok);
				tok = strtok(NULL, ","); // no NULL test here (special case for media files..)
				g_slots.Add(new FileSlotList(tokenStrs[0].Get(), tokenStrs[1].Get(), tok?tok:"", false, false, false));

				// known slot type?
				int newType = g_slots.GetSize()-1;
				int typeForUser = GetTypeForUser(newType);
				if (newType != typeForUser) // this is a known type => enable some stuff
				{
					g_slots.Get(newType)->SetNotepad(g_slots.Get(typeForUser)->HasNotepad());
					g_slots.Get(newType)->SetAutoSave(g_slots.Get(typeForUser)->HasAutoSave());
					g_slots.Get(newType)->SetDblClick(g_slots.Get(typeForUser)->HasDblClick());
				}
			}
		}
		iniKeyStr.SetFormatted(32, "CustomSlotType%d", (++i)+1);
		GetPrivateProfileString("RESOURCE_VIEW", iniKeyStr.Get(), "", buf, BUFFER_SIZE, g_SNMIniFn.Get());
	}
}

int ResourceViewInit()
{
// JFB add new resource types here ------------------------------------------->
	g_slots.Empty(true);
	g_slots.Add(new FileSlotList("FXChains", __LOCALIZE("FX chain","sws_DLG_150"), "RfxChain", true, true, true));
	g_slots.Add(new FileSlotList("TrackTemplates", __LOCALIZE("track template","sws_DLG_150"), "RTrackTemplate", true, true, true));
	g_slots.Add(new FileSlotList("ProjectTemplates", __LOCALIZE("project","sws_DLG_150"), "RPP", true, true, true));
	g_slots.Add(new FileSlotList("MediaFiles", __LOCALIZE("media file","sws_DLG_150"), "", false, true, true)); // "" means "all supported media files"
#ifdef _WIN32
	g_slots.Add(new FileSlotList("Data\\track_icons", __LOCALIZE("PNG image","sws_DLG_150"), "png", false, false, true));
#else
	g_slots.Add(new FileSlotList("Data/track_icons", __LOCALIZE("PNG image","sws_DLG_150"), "png", false, false, true));
#endif
	// etc..
// <---------------------------------------------------------------------------
#ifdef _WIN32
	g_slots.Add(new FileSlotList("ColorThemes", __LOCALIZE("theme","sws_DLG_150"), "ReaperthemeZip", false, false, true)); 
#endif
	AddCustomTypesFromIniFile();

	// load general prefs
	g_resViewType = BOUNDED((int)GetPrivateProfileInt(
		"RESOURCE_VIEW", "Type", SNM_SLOT_FXC, g_SNMIniFn.Get()), SNM_SLOT_FXC, g_slots.GetSize()-1); // bounded for safety (some custom slot types may have been removed..)

	g_filterPref = GetPrivateProfileInt("RESOURCE_VIEW", "Filter", 1, g_SNMIniFn.Get());
	g_autoSaveFXChainPref = GetPrivateProfileInt("RESOURCE_VIEW", "AutoSaveFXChain", FXC_AUTOSAVE_PREF_TRACK, g_SNMIniFn.Get());
	g_autoSaveFXChainNamePref = GetPrivateProfileInt("RESOURCE_VIEW", "AutoSaveFXChainName", 0, g_SNMIniFn.Get());
	g_autoSaveTrTmpltPref = GetPrivateProfileInt("RESOURCE_VIEW", "AutoSaveTrTemplate", 3, g_SNMIniFn.Get());
	g_prjLoaderStartPref = GetPrivateProfileInt("RESOURCE_VIEW", "ProjectLoaderStartSlot", -1, g_SNMIniFn.Get());
	g_prjLoaderEndPref = GetPrivateProfileInt("RESOURCE_VIEW", "ProjectLoaderEndSlot", -1, g_SNMIniFn.Get());

	// auto-save, auto-fill directories, etc..
	g_autoSaveDirs.Empty(true);
	g_autoFillDirs.Empty(true);
	memset(g_dblClickPrefs, 0, SNM_MAX_SLOT_TYPES*sizeof(int));
//	for (int i=0; i < SNM_NUM_DEFAULT_SLOTS; i++) g_tiedSlotActions[i]=i;

	char defaultPath[BUFFER_SIZE]="", path[BUFFER_SIZE]="", iniSec[64]="", iniKey[64]="";
	for (int i=0; i < g_slots.GetSize(); i++)
	{
		GetIniSectionName(i, iniSec, sizeof(iniSec));
		if (_snprintfStrict(defaultPath, sizeof(defaultPath), "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, g_slots.Get(i)->GetResourceDir()) < 0)
			*defaultPath = '\0';

		// g_autoFillDirs & g_autoSaveDirs must always be filled (even if auto-save is grayed for the user)
		WDL_FastString *fillPath, *savePath;
		if (_snprintfStrict(iniKey, sizeof(iniKey), "AutoSaveDir%s", iniSec) > 0) {
			GetPrivateProfileString("RESOURCE_VIEW", iniKey, defaultPath, path, BUFFER_SIZE, g_SNMIniFn.Get());
			savePath = new WDL_FastString(path);
		}
		else
			savePath = new WDL_FastString(defaultPath);
		g_autoSaveDirs.Add(savePath);

		if (_snprintfStrict(iniKey, sizeof(iniKey), "AutoFillDir%s", iniSec) > 0) {
			GetPrivateProfileString("RESOURCE_VIEW", iniKey, defaultPath, path, BUFFER_SIZE, g_SNMIniFn.Get());
			fillPath = new WDL_FastString(path);
		}
		else
			fillPath = new WDL_FastString(defaultPath);
		g_autoFillDirs.Add(fillPath);

		if (_snprintfStrict(iniKey, sizeof(iniKey), "SyncAutoDirs%s", iniSec) > 0)
			g_syncAutoDirPrefs[i] = (GetPrivateProfileInt("RESOURCE_VIEW", iniKey, 0, g_SNMIniFn.Get()) == 1);
		else
			g_syncAutoDirPrefs[i] = false;
		if (g_syncAutoDirPrefs[i]) // consistency check (e.g. after sws upgrade)
			g_syncAutoDirPrefs[i] = (strcmp(savePath->Get(), fillPath->Get()) == 0);

		if (g_slots.Get(i)->HasDblClick()) {
			if (_snprintfStrict(iniKey, sizeof(iniKey), "DblClick%s", iniSec) > 0)
				g_dblClickPrefs[i] = GetPrivateProfileInt("RESOURCE_VIEW", iniKey, 0, g_SNMIniFn.Get());
			else
				g_dblClickPrefs[i] = 0;
		}
		// load tied actions for default types (fx chains, track templates, etc...)
		if (i < SNM_NUM_DEFAULT_SLOTS) {
			if (_snprintfStrict(iniKey, sizeof(iniKey), "TiedActions%s", iniSec) > 0)
				g_tiedSlotActions[i] = GetPrivateProfileInt("RESOURCE_VIEW", iniKey, i, g_SNMIniFn.Get());
			else
				g_tiedSlotActions[i] = i;
		}
	}

	// read slots from ini file
	char desc[128]="", maxSlotCount[16]="";
	for (int i=0; i < g_slots.GetSize(); i++)
	{
		if (FileSlotList* list = g_slots.Get(i))
		{
			GetIniSectionName(i, iniSec, sizeof(iniSec));
			GetPrivateProfileString(iniSec, "Max_slot", "0", maxSlotCount, 16, g_SNMIniFn.Get()); 
			list->EmptySafe(true);
			int slotCount = atoi(maxSlotCount);
			for (int j=0; j < slotCount; j++) {
				ReadSlotIniFile(iniSec, j, path, BUFFER_SIZE, desc, 128);
				list->Add(new PathSlotItem(path, desc));
			}
		}
	}

	// instanciate the view
	g_pResourcesWnd = new SNM_ResourceWnd();
	if (!g_pResourcesWnd)
		return 0;
	return 1;
}

void ResourceViewExit()
{
	WDL_FastString iniStr, escapedStr;
	WDL_PtrList_DeleteOnDestroy<WDL_FastString> iniSections;

	GetIniSectionNames(&iniSections);


	// *** save the "RESOURCE_VIEW" ini section

	iniStr.Set("");

	// save custom slot type definitions
	for (int i=0; i < g_slots.GetSize(); i++)
	{
		if (i >= SNM_NUM_DEFAULT_SLOTS) {
			WDL_FastString str;
			str.SetFormatted(BUFFER_SIZE, "%s,%s,%s", g_slots.Get(i)->GetResourceDir(), g_slots.Get(i)->GetDesc(), g_slots.Get(i)->GetFileExt());
			makeEscapedConfigString(str.Get(), &escapedStr);
			iniStr.AppendFormatted(BUFFER_SIZE, "%s=%s\n", iniSections.Get(i)->Get(), escapedStr.Get());
		}
	}
	iniStr.AppendFormatted(BUFFER_SIZE, "Type=%d\n", g_resViewType);
	iniStr.AppendFormatted(BUFFER_SIZE, "Filter=%d\n", g_filterPref);

	// save slot type prefs
	for (int i=0; i < g_slots.GetSize(); i++)
	{
		// save auto-fill & auto-save paths
		if (g_autoFillDirs.Get(i)->GetLength()) {
			makeEscapedConfigString(g_autoFillDirs.Get(i)->Get(), &escapedStr);
			iniStr.AppendFormatted(BUFFER_SIZE, "AutoFillDir%s=%s\n", iniSections.Get(i)->Get(), escapedStr.Get());
		}
		if (g_autoSaveDirs.Get(i)->GetLength() && g_slots.Get(i)->HasAutoSave()) {
			makeEscapedConfigString(g_autoSaveDirs.Get(i)->Get(), &escapedStr);
			iniStr.AppendFormatted(BUFFER_SIZE, "AutoSaveDir%s=%s\n", iniSections.Get(i)->Get(), escapedStr.Get());
		}
		iniStr.AppendFormatted(BUFFER_SIZE, "SyncAutoDirs%s=%d\n", iniSections.Get(i)->Get(), g_syncAutoDirPrefs[i]);

		// save tied actions for default types (fx chains, track templates, etc...)
		if (i < SNM_NUM_DEFAULT_SLOTS)
			iniStr.AppendFormatted(BUFFER_SIZE, "TiedActions%s=%d\n", iniSections.Get(i)->Get(), g_tiedSlotActions[i]);

		// dbl-click pref
		if (g_slots.Get(i)->HasDblClick())
			iniStr.AppendFormatted(BUFFER_SIZE, "DblClick%s=%d\n", iniSections.Get(i)->Get(), g_dblClickPrefs[i]);

		// specific options (saved here for the ini file ordering..)
		switch (i) {
			case SNM_SLOT_FXC:
				iniStr.AppendFormatted(BUFFER_SIZE, "AutoSaveFXChain=%d\n", g_autoSaveFXChainPref);
				iniStr.AppendFormatted(BUFFER_SIZE, "AutoSaveFXChainName=%d\n", g_autoSaveFXChainNamePref);
				break;
			case SNM_SLOT_TR:
				iniStr.AppendFormatted(BUFFER_SIZE, "AutoSaveTrTemplate=%d\n", g_autoSaveTrTmpltPref);
				break;
			case SNM_SLOT_PRJ:
				iniStr.AppendFormatted(BUFFER_SIZE, "ProjectLoaderStartSlot=%d\n", g_prjLoaderStartPref);
				iniStr.AppendFormatted(BUFFER_SIZE, "ProjectLoaderEndSlot=%d\n", g_prjLoaderEndPref);
				break;
		}
	}
	SaveIniSection("RESOURCE_VIEW", &iniStr, g_SNMIniFn.Get());


	// *** save slots ini sections

	for (int i=0; i < g_slots.GetSize(); i++)
	{
		iniStr.SetFormatted(BUFFER_SIZE, "Max_slot=%d\n", g_slots.Get(i)->GetSize());
		for (int j=0; j < g_slots.Get(i)->GetSize(); j++) {
			if (PathSlotItem* item = g_slots.Get(i)->Get(j)) {
				if (item->m_shortPath.GetLength()) {
					makeEscapedConfigString(item->m_shortPath.Get(), &escapedStr);
					iniStr.AppendFormatted(BUFFER_SIZE, "Slot%d=%s\n", j+1, escapedStr.Get());
				}
				if (item->m_comment.GetLength()) {
					makeEscapedConfigString(item->m_comment.Get(), &escapedStr);
					iniStr.AppendFormatted(BUFFER_SIZE, "Desc%d=%s\n", j+1, escapedStr.Get());
				}
			}
		}
		// write things in one go (avoid to slow down REAPER shutdown)
		SaveIniSection(iniSections.Get(i)->Get(), &iniStr, g_SNMIniFn.Get());
	}

	DELETE_NULL(g_pResourcesWnd);
}

void OpenResourceView(COMMAND_T* _ct) 
{
	if (g_pResourcesWnd) 
	{
		int newType = (int)_ct->user; // -1 means toggle current type
		if (newType == -1)
			newType = g_resViewType;
		g_pResourcesWnd->Show((g_resViewType == newType) /* i.e toggle */, true);
		g_pResourcesWnd->SetType(newType);
//		SetFocus(GetDlgItem(g_pResourcesWnd->GetHWND(), IDC_FILTER));
	}
}

bool IsResourceViewDisplayed(COMMAND_T* _ct) {
	return (g_pResourcesWnd && g_pResourcesWnd->IsValidWindow());
}

void ResViewDeleteAllSlots(COMMAND_T* _ct)
{
	FileSlotList* fl = g_slots.Get(g_tiedSlotActions[(int)_ct->user]);
	WDL_PtrList_DeleteOnDestroy<PathSlotItem> delItems; // keep (displayed!) pointers until the list view is updated
	for (int i=fl->GetSize()-1; i >= 0; i--) {
		delItems.Add(fl->Get(i));
		fl->Delete(i, false);
	}
	if (g_pResourcesWnd && delItems.GetSize())
		g_pResourcesWnd->Update();
} // + delItems auto clean-up !

void ResViewClearSlotPrompt(COMMAND_T* _ct) {
	g_slots.Get(g_tiedSlotActions[(int)_ct->user])->ClearSlotPrompt(_ct);
}

void ResViewClearFXChainSlot(COMMAND_T* _ct) {
	g_slots.Get(g_tiedSlotActions[SNM_SLOT_FXC])->ClearSlot((int)_ct->user);
}

void ResViewClearTrTemplateSlot(COMMAND_T* _ct) {
	g_slots.Get(g_tiedSlotActions[SNM_SLOT_TR])->ClearSlot((int)_ct->user);
}

void ResViewClearPrjTemplateSlot(COMMAND_T* _ct) {
	g_slots.Get(g_tiedSlotActions[SNM_SLOT_PRJ])->ClearSlot((int)_ct->user);
}

void ResViewClearMediaSlot(COMMAND_T* _ct) {
	//JFB TODO? stop sound?
	g_slots.Get(g_tiedSlotActions[SNM_SLOT_MEDIA])->ClearSlot((int)_ct->user);
}

void ResViewClearImageSlot(COMMAND_T* _ct) {
	g_slots.Get(g_tiedSlotActions[SNM_SLOT_IMG])->ClearSlot((int)_ct->user);
}

#ifdef _WIN32
void ResViewClearThemeSlot(COMMAND_T* _ct) {
	g_slots.Get(g_tiedSlotActions[SNM_SLOT_THM])->ClearSlot((int)_ct->user);
}
#endif

// specific auto-save for fx chains
void ResViewAutoSaveFXChain(COMMAND_T* _ct) {
	AutoSave(g_tiedSlotActions[SNM_SLOT_FXC], (int)_ct->user);
}

// specific auto-save for track templates
void ResViewAutoSaveTrTemplate(COMMAND_T* _ct) {
	AutoSave(g_tiedSlotActions[SNM_SLOT_TR], (int)_ct->user);
}

// auto-save for all other types..
void ResViewAutoSave(COMMAND_T* _ct) {
	int type = (int)_ct->user;
	if (g_slots.Get(type)->HasAutoSave()) {
		AutoSave(g_tiedSlotActions[type], 0);
	}
}


///////////////////////////////////////////////////////////////////////////////
// SNM_ImageWnd
///////////////////////////////////////////////////////////////////////////////

SNM_ImageWnd* g_pImageWnd = NULL;

SNM_ImageWnd::SNM_ImageWnd()
	: SWS_DockWnd(IDD_SNM_IMAGE, __LOCALIZE("Image","sws_DLG_162"), "SnMImage", 30012, SWSGetCommandID(OpenImageView))
{
	m_stretch = false;

	// Must call SWS_DockWnd::Init() to restore parameters and open the window if necessary
	Init();
}

void SNM_ImageWnd::OnInitDlg()
{
	SetWindowLongPtr(GetDlgItem(m_hwnd, IDC_EDIT), GWLP_USERDATA, 0xdeadf00b);

	m_vwnd_painter.SetGSC(WDL_STYLE_GetSysColor);
    m_parentVwnd.SetRealParent(m_hwnd);
	
	m_img.SetID(2000); //JFB would be great to have _APS_NEXT_CONTROL_VALUE *always* defined
	m_parentVwnd.AddChild(&m_img);
	RequestRedraw();
}

void SNM_ImageWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch(LOWORD(wParam))
	{
		case 0xF001:
			m_stretch = !m_stretch;
			RequestRedraw();
			break;
		default:
			Main_OnCommand((int)wParam, (int)lParam);
			break;
	}
}

HMENU SNM_ImageWnd::OnContextMenu(int x, int y, bool* wantDefaultItems)
{
	HMENU hMenu = CreatePopupMenu();
	AddToMenu(hMenu, __LOCALIZE("Stretch to fit","sws_DLG_162"), 0xF001, -1, false, m_stretch?MFS_CHECKED:MFS_UNCHECKED);
	return hMenu;
}

void SNM_ImageWnd::DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight)
{
	int x0=_r->left+10, h=35;
	if (_tooltipHeight)
		*_tooltipHeight = h;

	m_img.SetVisible(true);
	if (!m_stretch)
	{
		int x = _r->left + int((_r->right-_r->left)/2 - m_img.GetWidth()/2 + 0.5);
		int y = _r->top + int((_r->bottom-_r->top)/2 - m_img.GetHeight()/2 + 0.5);
		RECT tr = {x, y, x+m_img.GetWidth(), y+m_img.GetHeight()};
		m_img.SetPosition(&tr);
		SNM_AddLogo(_bm, _r, x0, h);
	}
	else
		m_img.SetPosition(_r);
}

int ImageViewInit()
{
	g_pImageWnd = new SNM_ImageWnd();
	if (!g_pImageWnd)
		return 0;

	// load prefs
	g_pImageWnd->SetStretch(!!GetPrivateProfileInt("ImageView", "Stretch", 0, g_SNMIniFn.Get()));

	return 1;
}

void ImageViewExit()
{
	// save prefs
	if (g_pImageWnd)
		WritePrivateProfileString("ImageView", "Stretch", g_pImageWnd->IsStretched()?"1":"0", g_SNMIniFn.Get()); 
	DELETE_NULL(g_pImageWnd);
}

void OpenImageView(COMMAND_T* _ct) {
	if (g_pImageWnd) {
		g_pImageWnd->Show(true, true);
		g_pImageWnd->RequestRedraw();
	}
}

WDL_FastString g_lastImageFn;

void OpenImageView(const char* _fn)
{
	if (g_pImageWnd)
	{
		WDL_FastString g_prevFn(g_lastImageFn.Get());
		g_lastImageFn.Set("");
		if (_fn && *_fn) {
			if (LICE_IBitmap* img = LICE_LoadPNG(_fn, NULL)) {
				g_pImageWnd->SetImage(img);
				g_lastImageFn.Set(_fn);
			}
		}
		else
			g_pImageWnd->SetImage(NULL);

		g_pImageWnd->Show(!strcmp(g_prevFn.Get(), _fn) /* i.e toggle */, true);
		g_pImageWnd->RequestRedraw();
	}
}

void ClearImageView(COMMAND_T*) {
	if (g_pImageWnd) {
		g_pImageWnd->SetImage(NULL);
		g_pImageWnd->RequestRedraw();
	}
}

bool IsImageViewDisplayed(COMMAND_T*){
	return (g_pImageWnd && g_pImageWnd->IsValidWindow());
}
