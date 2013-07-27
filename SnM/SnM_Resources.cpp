/******************************************************************************
/ SnM_Resources.cpp
/
/ Copyright (c) 2009-2013 Jeffos
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

//JFB
// TODO: turn the option "Attach bookmark to this project" into "In-project bookmark"
//       requires some API updates first though : http://askjf.com/index.php?q=2465s

#include "stdafx.h"
#include "SnM.h"
#include "SnM_Dlg.h"
#include "SnM_FXChain.h"
#include "SnM_Item.h"
#include "SnM_Misc.h"
#include "SnM_Project.h"
#include "SnM_Resources.h"
#include "SnM_Track.h"
#include "SnM_Util.h"
#include "../Prompt.h"
#ifdef _WIN32
#include "../DragDrop.h"
#endif
#include "../reaper/localize.h"


#define RES_WND_ID					"SnMResources"
#define IMG_WND_ID					"SnMImage"
#define RES_INI_SEC					"Resources"

// common cmds
#define AUTOFILL_MSG				0xF000
#define AUTOFILL_DIR_MSG			0xF001
#define AUTOFILL_PRJ_MSG			0xF002
#define AUTOFILL_DEFAULT_MSG		0xF003
#define AUTOFILL_SYNC_MSG			0xF004
#define CLR_SLOTS_MSG				0xF005
#define DEL_SLOTS_MSG				0xF006
#define DEL_FILES_MSG				0xF007
#define ADD_SLOT_MSG				0xF008
#define INSERT_SLOT_MSG				0xF009
#define EDIT_MSG					0xF00A
#define EXPLORE_MSG					0xF00B
#define EXPLORE_SAVEDIR_MSG			0xF00C
#define EXPLORE_FILLDIR_MSG			0xF00D
#define LOAD_MSG					0xF00E
#define AUTOSAVE_MSG				0xF00F
#define AUTOSAVE_DIR_MSG			0xF010
#define AUTOSAVE_DIR_PRJ_MSG		0xF011
#define AUTOSAVE_DIR_PRJ2_MSG		0xF012
#define AUTOSAVE_DIR_DEFAULT_MSG	0xF013
#define AUTOSAVE_SYNC_MSG			0xF014
#define FILTER_BY_NAME_MSG			0xF015
#define FILTER_BY_PATH_MSG			0xF016
#define FILTER_BY_COMMENT_MSG		0xF017
#define RENAME_MSG					0xF018
#define TIE_ACTIONS_MSG				0xF019
#define TIE_PROJECT_MSG				0xF01A
#define UNTIE_PROJECT_MSG			0xF01B
#ifdef _WIN32
#define DIFF_MSG					0xF01C
#endif
// fx chains cmds
#define FXC_APPLY_TR_MSG			0xF030
#define FXC_APPLY_TAKE_MSG			0xF031
#define FXC_APPLY_ALLTAKES_MSG		0xF032
#define FXC_PASTE_TR_MSG			0xF033
#define FXC_PASTE_TAKE_MSG			0xF034
#define FXC_PASTE_ALLTAKES_MSG		0xF035
#define FXC_AUTOSAVE_INPUTFX_MSG	0xF036
#define FXC_AUTOSAVE_TR_MSG			0xF037
#define FXC_AUTOSAVE_ITEM_MSG		0xF038
#define FXC_AUTOSAVE_DEFNAME_MSG	0xF039
#define FXC_AUTOSAVE_FX1NAME_MSG	0xF03A
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
#define PRJ_LOADER_SET_MSG			0xF053
#define PRJ_LOADER_CLEAR_MSG		0xF054
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
// theme file cmds
#define THM_LOAD_MSG				0xF080 
// (newer) common cmds
#define LOAD_TIED_PRJ_MSG			0xF090
#define LOAD_TIED_PRJ_TAB_MSG		0xF091
#define COPY_BOOKMARK_MSG			0xF092
#define DEL_BOOKMARK_MSG			0xF093
#define REN_BOOKMARK_MSG			0xF094
#define NEW_BOOKMARK_START_MSG		0xF095 // leave some room here -->
#define NEW_BOOKMARK_END_MSG		0xF0BF // <--


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
#define TIED_PRJ_SELECT_LOAD_STR	__LOCALIZE_VERFMT("Open/select attached project %s","sws_DLG_150")
#define TIED_PRJ_SELECT_LOADTAB_STR	__LOCALIZE_VERFMT("Open/select attached project %s (new tab)","sws_DLG_150")
#define MED_PLAY_STR				__LOCALIZE("Play media file in selected tracks (toggle)","sws_DLG_150")
#define MED_LOOP_STR				__LOCALIZE("Loop media file in selected tracks (toggle)","sws_DLG_150")
#define MED_PLAYLOOP_STR			__LOCALIZE("Play/loop media file","sws_DLG_150")
#define MED_ADD_STR					__LOCALIZE("Insert media","sws_DLG_150")
#define IMG_SHOW_STR				__LOCALIZE("Show image","sws_DLG_150")
#define IMG_TRICON_STR				__LOCALIZE("Set track icon for selected tracks","sws_DLG_150")
#define THM_LOAD_STR				__LOCALIZE("Load theme","sws_DLG_150")

// no default filter text on OSX (cannot catch EN_SETFOCUS/EN_KILLFOCUS)
#ifndef _SNM_SWELL_ISSUES
#define FILTER_DEFAULT_STR		__LOCALIZE("Filter","sws_DLG_150")
#else
#define FILTER_DEFAULT_STR		""
#endif
#define DRAGDROP_EMPTY_SLOT		">Empty<" // no localization: internal stuff
#define AUTOSAVE_ERR_STR		__LOCALIZE("Probable cause: no selection, nothing to save, cannot write file, file in use, invalid filename, etc...","sws_DLG_150")
#define AUTOFILL_ERR_STR		__LOCALIZE("Probable cause: all files are already present, empty directory, directory not found, etc...","sws_DLG_150")
#define NO_SLOT_ERR_STR			__LOCALIZE("No slot found in the bookmark \"%s\" of Resources window!","sws_DLG_150")

enum {
  BTNID_AUTOFILL=2000, //JFB would be great to have _APS_NEXT_CONTROL_VALUE *always* defined
  BTNID_AUTOSAVE,
  CMBID_TYPE,
  TXTID_TIED_PRJ,
  WNDID_ADD_DEL,
  BTNID_ADD_BOOKMARK,
  BTNID_DEL_BOOKMARK,
  BTNID_OFFSET_TR_TEMPLATE,
  TXTID_DBL_TYPE,
  CMBID_DBLCLICK_TYPE,
  TXTID_DBL_TO,
  CMBID_DBLCLICK_TO
};

enum {
  COL_SLOT=0,
  COL_NAME,
  COL_PATH,
  COL_COMMENT,
  COL_COUNT
};


SNM_WindowManager<ResourcesWnd> g_resWndMgr(RES_WND_ID);

// JFB important notes:
// all WDL_PtrList global vars used to be WDL_PtrList_DeleteOnDestroy ones but
// something weird could occur when REAPER unloads the extension: hang or crash 
// (e.g. issues 292 & 380) on Windows 7 while saving ini files (those lists were 
// already unallocated..)
// anyway, no prob here because application exit will destroy the entire runtime 
// context regardless.

WDL_PtrList<ResourceItem> g_dragResourceItems; 
WDL_PtrList<ResourceList> g_SNM_ResSlots;

// prefs
int g_resType = -1; // current type (user selection)
int g_SNM_TiedSlotActions[SNM_NUM_DEFAULT_SLOTS]; // slot actions of default type/idx are tied to type/value
int g_dblClickPrefs[SNM_MAX_SLOT_TYPES]; // flags, loword: dbl-click type, hiword: dbl-click to (only for fx chains, atm)
WDL_FastString g_filter; // see init + localization in ResourcesInit()
int g_filterPref = 1; // bitmask: &1 = filter by name, &2 = filter by path, &4 = filter by comment
WDL_PtrList<WDL_FastString> g_autoSaveDirs;
WDL_PtrList<WDL_FastString> g_autoFillDirs;
WDL_PtrList<WDL_FastString> g_tiedProjects;
bool g_syncAutoDirPrefs[SNM_MAX_SLOT_TYPES];
int g_SNM_PrjLoaderStartPref = -1; // 1-based
int g_SNM_PrjLoaderEndPref = -1; // 1-based

// auto-save prefs
int g_asTrTmpltPref = 3; // &1: with items, &2: with envs
int g_asFXChainPref = FXC_AUTOSAVE_PREF_TRACK;
int g_asFXChainNamePref = 0;

// vars used to track project switches
// note: ReaPoject* is not enough as this pointer can be re-used when loading RPP files
char g_curProjectFn[SNM_MAX_PATH]="";
ReaProject* g_curProject = NULL;


///////////////////////////////////////////////////////////////////////////////
// Helpers
///////////////////////////////////////////////////////////////////////////////

const char* GetAutoSaveDir(int _type = -1) {
	return g_autoSaveDirs.Get(_type<0 ? g_resType : _type)->Get();
}

void SetAutoSaveDir(const char* _path, int _type = -1) {
	if (_type < 0) _type = g_resType;
	g_autoSaveDirs.Get(_type)->Set(_path);
	if (g_syncAutoDirPrefs[_type])
		g_autoFillDirs.Get(_type)->Set(_path);
}

const char* GetAutoFillDir(int _type = -1) {
	return g_autoFillDirs.Get(_type<0 ? g_resType : _type)->Get();
}

void SetAutoFillDir(const char* _path, int _type = -1) {
	if (_type < 0) _type = g_resType;
	g_autoFillDirs.Get(_type)->Set(_path);
	if (g_syncAutoDirPrefs[_type])
		g_autoSaveDirs.Get(_type)->Set(_path);
}

int GetTypeForUser(int _type = -1)
{
	if (_type < 0) _type = g_resType;
	if (_type >= SNM_NUM_DEFAULT_SLOTS)
		for (int i=0; i < SNM_NUM_DEFAULT_SLOTS; i++)
			if (!_stricmp(g_SNM_ResSlots.Get(_type)->GetFileExtStr(), g_SNM_ResSlots.Get(i)->GetFileExtStr()))
				return i;
	return _type;
}

bool IsMultiType(int _type = -1)
{
	if (_type < 0) _type = g_resType;
	int typeForUser = GetTypeForUser(_type);
	for (int i=0; i < g_SNM_ResSlots.GetSize(); i++)
		if (i != _type && GetTypeForUser(i) == typeForUser)
			return true;
	return false;
}

void GetIniSectionName(int _type, char* _bufOut, size_t _bufOutSz)
{
	if (_type >= SNM_NUM_DEFAULT_SLOTS)
	{
		*_bufOut = '\0';
		_snprintfSafe(_bufOut, _bufOutSz, "CustomSlotType%d", _type-SNM_NUM_DEFAULT_SLOTS+1);
		return;
	}
	// relative path needed, for ex: data\track_icons
	lstrcpyn(_bufOut, GetFileRelativePath(g_SNM_ResSlots.Get(_type)->GetResourceDir()), _bufOutSz);
	*_bufOut = toupper(*_bufOut);
}

// it is up to the caller to unalloc.. 
void GetIniSectionNames(WDL_PtrList<WDL_FastString>* _iniSections)
{
	char iniSec[64]="";
	for (int i=0; i < g_SNM_ResSlots.GetSize(); i++) {
		GetIniSectionName(i, iniSec, sizeof(iniSec));
		_iniSections->Add(new WDL_FastString(iniSec));
	}
}

bool IsFiltered() {
	return (g_filter.GetLength() && strcmp(g_filter.Get(), FILTER_DEFAULT_STR));
}

// attach _fn to _bookmarkType's project if the project is open and 
// and if _fn is not already present (optional, not verified by default: safer with some corner cases, 
// e.g. unexisting file referenced in duplicated slots, etc..)
// note: better to re-attach files (no-op) than missing them!
void TieResFileToProject(const char* _fn, int _bookmarkType, bool _checkDup = false, bool _tie = true)
{
	if (!_fn || !*_fn)
		return;

	if (_bookmarkType>=SNM_NUM_DEFAULT_SLOTS && g_tiedProjects.Get(_bookmarkType)->GetLength())
		if (ResourceList* fl = g_SNM_ResSlots.Get(_bookmarkType))
			if (!_checkDup || (_checkDup && fl->FindByPath(_fn)<0))
			{
				int i=0;
				char path[SNM_MAX_PATH] = "";
				while (ReaProject* prj = EnumProjects(i++, path, sizeof(path)))
				{
					if (!_stricmp(g_tiedProjects.Get(_bookmarkType)->Get(), path))
					{
						TieFileToProject(_fn, prj, _tie);
						return;
					}
				}
			}
}

// detach _fn from _bookmarkType's project if the project is open and 
// if _fn is indeed not present anymore (optional, to avoid to datach duplicated slots)
void UntieResFileFromProject(const char* _fn, int _bookmarkType, bool _checkDup = true)
{
	if (!_fn || !*_fn)
		return;

	if (_bookmarkType>=SNM_NUM_DEFAULT_SLOTS && g_tiedProjects.Get(_bookmarkType)->GetLength())
		if (ResourceList* fl = g_SNM_ResSlots.Get(_bookmarkType))
			if (!_checkDup || (_checkDup && fl->FindByPath(_fn)<0))
				TieResFileToProject(_fn, _bookmarkType, false, false);
}

// attach all files to _prj
// _type<0: attach all files of all bookmaks, given bookmark otherwise
void TieAllResFileToProject(ReaProject* _prj, const char* _prjPath, int _type = -1, bool _tie = true)
{
	if (_prj && _prjPath && *_prjPath)
	{
		for (int i = (_type<0 ? SNM_NUM_DEFAULT_SLOTS : _type); 
			i <= (_type<0 ? g_SNM_ResSlots.GetSize()-1 : _type); 
			i++)
		{
			if (g_tiedProjects.Get(i)->GetLength())
			{
				if (!_stricmp(g_tiedProjects.Get(i)->Get(), _prjPath))
				{
					char fullpath[SNM_MAX_PATH]="";
					for (int j=0; j<g_SNM_ResSlots.Get(i)->GetSize(); j++)
						if (g_SNM_ResSlots.Get(i)->GetFullPath(j, fullpath, sizeof(fullpath)))
							TieFileToProject(fullpath, _prj, _tie);
				}
			}
		}
	}
}

// detach all files from _prj
// _type<0: detach all files of all bookmaks, given bookmark otherwise
void UntieAllResFileFromProject(ReaProject* _prj, const char* _prjPath, int _type = -1) {
	TieAllResFileToProject(_prj, _prjPath, _type, false);
}


///////////////////////////////////////////////////////////////////////////////
// ResourceList
///////////////////////////////////////////////////////////////////////////////

ResourceList::ResourceList(const char* _resDir, const char* _desc, const char* _ext, int _flags)
	: m_desc(_desc), m_ext(_ext), m_flags(_flags), WDL_PtrList<ResourceItem>()
{
	char tmp[512]="";

	// resource directory: set specific OS path separator, sanitize path
	int i;
	lstrcpyn(tmp, _resDir, sizeof(tmp));
#ifdef _WIN32
	i=-1;
	while (tmp[++i])
		if (tmp[i]=='/')
			tmp[i]=PATH_SLASH_CHAR;
#endif
	i = strlen(tmp);
	if (tmp[i-1] == PATH_SLASH_CHAR)
		tmp[i-1] = '\0';
	m_resDir.Set(tmp);

	// split extensions
	char ext[16];
	bool first = true;
	lstrcpyn(tmp, _ext, sizeof(tmp));
	char* tok = strtok(tmp, ",");
	while (tok)
	{
		lstrcpyn(ext, tok, sizeof(ext));
		tok = strtok(NULL, ",");

		// upgrade if needed
		if (first && !tok && !_stricmp("RPP", ext))
			m_exts.Add(new WDL_FastString("RPP*"));
		// upgrade + PNG*, see GetFileFilter()
		else if (first && !tok && (!_stricmp("PNG", ext) || !_stricmp("PNG*", ext)))
			SplitStrings(SNM_REAPER_IMG_EXTS, &m_exts);
		else
			m_exts.Add(new WDL_FastString(ext));
		first = false;
	}

	// upgrade if needed
	// in versions <= v2.3.0 #16, the extension "" was meaning "all media files"
	if (!m_exts.GetSize())
		m_exts.Add(new WDL_FastString("WAV*"));
}

// _path: short resource path or full path
ResourceItem* ResourceList::AddSlot(const char* _path, const char* _desc)
{
	char shortPath[SNM_MAX_PATH] = "";
	GetShortResourcePath(m_resDir.Get(), _path, shortPath, sizeof(shortPath));
	return Add(new ResourceItem(shortPath, _desc));
}

// _path: short resource path or full path
ResourceItem* ResourceList::InsertSlot(int _slot, const char* _path, const char* _desc)
{
	ResourceItem* item = NULL;
	char shortPath[SNM_MAX_PATH] = "";
	GetShortResourcePath(m_resDir.Get(), _path, shortPath, sizeof(shortPath));
	if (_slot >=0 && _slot < GetSize())
		item = Insert(_slot, new ResourceItem(shortPath, _desc));
	else
		item = Add(new ResourceItem(shortPath, _desc));
	return item;
}

int ResourceList::FindByPath(const char* _fullPath)
{
	char fullpath[SNM_MAX_PATH];
	if (_fullPath)
		for (int i=0; i<GetSize(); i++)
			if (GetFullPath(i, fullpath, sizeof(fullpath)))
				if (!_stricmp(_fullPath, fullpath))
					return i;
	return -1;
}

bool ResourceList::GetFullPath(int _slot, char* _fullFn, int _fullFnSz)
{
	if (ResourceItem* item = Get(_slot)) {
		GetFullResourcePath(m_resDir.Get(), item->m_shortPath.Get(), _fullFn, _fullFnSz);
		return true;
	}
	return false;
}

bool ResourceList::SetFromFullPath(int _slot, const char* _fullPath)
{
	if (ResourceItem* item = Get(_slot))
	{
		char shortPath[SNM_MAX_PATH] = "";
		GetShortResourcePath(m_resDir.Get(), _fullPath, shortPath, sizeof(shortPath));
		item->m_shortPath.Set(shortPath);
		return true;
	}
	return false;
}

bool ResourceList::IsValidFileExt(const char* _ext)
{
	if (_ext && *_ext && m_exts.GetSize())
	{
		// wildcard?
		if (!strcmp("*", m_exts.Get(0)->Get()))
			return true;

/* JFB replaced with code below => single place to support new file extensions == GetFileFilter()
		for (int i=0; i<m_exts.GetSize(); i++)
			if (!strcmp("*", m_exts.Get(i)->Get()) || !_stricmp(_ext, m_exts.Get(i)->Get()))
				return true;
*/
		WDL_FastString ext;
		ext.SetFormatted(16, "*.%s", _ext);
		char fileFilter[2048]=""; // room needed for file filters!
		GetFileFilter(fileFilter, sizeof(fileFilter), false);
		return (stristr(fileFilter, ext.Get()) != NULL);
	}
	return false;
}

void ResourceList::GetFileFilter(char* _filter, size_t _filterSz, bool _dblNull)
{
	if (_filter)
	{
		memset(_filter, '\0', _filterSz);

		bool transcode = false;
		if (m_exts.GetSize())
		{
/*JFB commented: linking against LICE/PNG only atm..
			// better filter from LICE for images
			if (!_strnicmp("PNG*", m_exts.Get(0)->Get(), 4)) {
				memcpy(_filter, LICE_GetImageExtensionList(), _filterSz);
				transcode = !_dblNull;
			}
			else 
*/ 
			if (!_strnicmp("RPP*", m_exts.Get(0)->Get(), 4)) {
				memcpy(_filter, plugin_getImportableProjectFilterList(), _filterSz);
				transcode = !_dblNull;
			}
			else if (!_strnicmp("WAV*", m_exts.Get(0)->Get(), 4)) {
				memcpy(_filter, plugin_getFilterList(), _filterSz);
				transcode = !_dblNull;
			}
			// generic filter
			else
			{
				WDL_FastString exts;
				if (strcmp("*", m_exts.Get(0)->Get())) // not wildcard?
				{
					if (m_exts.GetSize()>1)
					{
						exts.Append(__LOCALIZE("All supported files","sws_DLG_150"));
						exts.Append("|");
						for (int i=0; i<m_exts.GetSize(); i++)
							exts.AppendFormatted(128, "*.%s;", m_exts.Get(i)->Get());
						exts.Append("|");
					}

					for (int i=0; i<m_exts.GetSize(); i++)
						exts.AppendFormatted(128, "*.%s|*.%s|", m_exts.Get(i)->Get(), m_exts.Get(i)->Get());
				}
				else if (!_dblNull)
					exts.Set("*");

				if (_dblNull) {
					exts.Append(__LOCALIZE("All files","sws_DLG_150"));
					exts.Append(" (*.*)|*.*|");
				}

				lstrcpyn(_filter, exts.Get(), _filterSz);
				transcode = _dblNull;
			}
		}

		if (transcode)
		{
			int i = -1;
			if (_dblNull) // trick: '|' -> '\0'
			{
				while ((++i) < (int)(_filterSz-2)) // -2: safety
					if (_filter[i] == '|')
						_filter[i] = '\0';
			}
			else // trick: '\0' -> ' '
			{
				while ((++i) < (int)(_filterSz-1)) // -1: safety
					if (_filter[i] == '\0' && _filter[i+1] != '\0')
						_filter[i] = ' ';
			}
		}
	}
}

bool ResourceList::ClearSlot(int _slot)
{
	if (_slot>=0 && _slot<GetSize()) {
		Get(_slot)->Clear();
		return true;
	}
	return false;
}


///////////////////////////////////////////////////////////////////////////////
// ResourcesView
///////////////////////////////////////////////////////////////////////////////

// !WANT_LOCALIZE_STRINGS_BEGIN:sws_DLG_150
static SWS_LVColumn s_resListCols[] = { {65,2,"Slot"}, {100,1,"Name"}, {250,2,"Path"}, {200,1,"Comment"} };
// !WANT_LOCALIZE_STRINGS_END

ResourcesView::ResourcesView(HWND hwndList, HWND hwndEdit)
	: SWS_ListView(hwndList, hwndEdit, COL_COUNT, s_resListCols, "ResourcesViewState", false, "sws_DLG_150")
{
}

void ResourcesView::GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax)
{
	if (str) *str = '\0';
	if (ResourceItem* pItem = (ResourceItem*)item)
	{
		switch (iCol)
		{
			case COL_SLOT:
			{
				ResourceList* fl = g_SNM_ResSlots.Get(g_resType);
				int slot = fl ? fl->Find(pItem) : -1;
				if (slot >= 0)
				{
					slot++;
					if (g_resType == SNM_SLOT_PRJ && IsProjectLoaderConfValid()) // no GetTypeForUser() here: only one loader/selecter config atm..
						_snprintfSafe(str, iStrMax, "%5.d %s", slot, 
							slot<g_SNM_PrjLoaderStartPref || slot>g_SNM_PrjLoaderEndPref ? "  " : 
							g_SNM_PrjLoaderStartPref==slot ? "->" : g_SNM_PrjLoaderEndPref==slot ? "<-" :  "--");
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

bool ResourcesView::IsEditListItemAllowed(SWS_ListItem* item, int iCol)
{
	if (ResourceItem* pItem = (ResourceItem*)item)
	{
		if (!pItem->IsDefault())
		{
			ResourceList* fl = g_SNM_ResSlots.Get(g_resType);
			int slot = fl ? fl->Find(pItem) : -1;
			if (slot>=0)
			{
				switch (iCol) {
					case COL_NAME: { // file renaming
						char fn[SNM_MAX_PATH] = "";
						return (fl->GetFullPath(slot, fn, sizeof(fn)) && FileOrDirExists(fn));
					}
					case COL_COMMENT:
						return true;
				}
			}
		}
	}
	return false;
}

void ResourcesView::SetItemText(SWS_ListItem* item, int iCol, const char* str)
{
	ResourceItem* pItem = (ResourceItem*)item;
	ResourceList* fl = g_SNM_ResSlots.Get(g_resType);
	int slot = fl ? fl->Find(pItem) : -1;
	if (pItem && slot>=0)
	{
		switch (iCol)
		{
			case COL_NAME: // file renaming
			{
				if (!IsValidFilenameErrMsg(str, true))
					return;

				char fn[SNM_MAX_PATH] = "";
				if (fl->GetFullPath(slot, fn, sizeof(fn)) && !pItem->IsDefault() && FileOrDirExistsErrMsg(fn))
				{
					const char* ext = GetFileExtension(fn);
					char newFn[SNM_MAX_PATH]="", path[SNM_MAX_PATH]="";
					lstrcpyn(path, fn, sizeof(path));
					if (char* p = strrchr(path, PATH_SLASH_CHAR))
						*p = '\0';
					else
						break; // safety

					if (_snprintfStrict(newFn, sizeof(newFn), "%s%c%s.%s", path, PATH_SLASH_CHAR, str, ext) > 0)
					{
						if (FileOrDirExists(newFn)) 
						{
							char msg[SNM_MAX_PATH]="";
							_snprintfSafe(msg, sizeof(msg), __LOCALIZE_VERFMT("File already exists. Overwrite ?\n%s","sws_mbox"), newFn);
							int res = MessageBox(g_resWndMgr.GetMsgHWND(), msg, __LOCALIZE("S&M - Warning","sws_DLG_150"), MB_YESNO);
							if (res == IDYES)
							{
								if (SNM_DeleteFile(newFn, false))
									UntieResFileFromProject(newFn, g_resType, false);
								else
									break;
							}
							else
								break;
						}

						if (MoveFile(fn, newFn) && fl->SetFromFullPath(slot, newFn))
						{
							UntieResFileFromProject(fn, g_resType);
							TieResFileToProject(newFn, g_resType);

							ListView_SetItemText(m_hwndList, GetEditingItem(), DisplayToDataCol(2), (LPSTR)pItem->m_shortPath.Get());
							// ^^ direct GUI update because Update() is no-op when editing
						}
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

void ResourcesView::OnItemDblClk(SWS_ListItem* item, int iCol) {
	Perform();
}

void ResourcesView::GetItemList(SWS_ListItemList* pList)
{
	ResourceList* fl = g_SNM_ResSlots.Get(g_resType);
	if (!fl)
		return;

	if (IsFiltered())
	{
		char buf[SNM_MAX_PATH] = "";
		LineParser lp(false);
		if (!lp.parse(g_filter.Get()))
		{
			for (int i=0; i < fl->GetSize(); i++)
			{
				if (ResourceItem* item = fl->Get(i))
				{
					bool match = false;
					for (int j=0; !match && j < lp.getnumtokens(); j++)
					{
						if (g_filterPref&1) // name
						{
							GetFilenameNoExt(item->m_shortPath.Get(), buf, sizeof(buf));
							match |= (stristr(buf, lp.gettoken_str(j)) != NULL);
						}
						if (!match && (g_filterPref&2)) // path
						{
							if (fl->GetFullPath(i, buf, sizeof(buf)))
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
		for (int i=0; i < fl->GetSize(); i++)
			pList->Add((SWS_ListItem*)fl->Get(i));
	}
}

void ResourcesView::OnBeginDrag(SWS_ListItem* _item)
{
#ifdef _WIN32
	ResourceList* fl = g_SNM_ResSlots.Get(g_resType);
	if (!fl)
		return;

	g_dragResourceItems.Empty(false);

	// store dragged items (for internal drag-drop) + full paths + get the amount of memory needed
	int iMemNeeded = 0, x=0;
	WDL_PtrList_DeleteOnDestroy<WDL_FastString> fullPaths;
	while (ResourceItem* pItem = (ResourceItem*)EnumSelected(&x))
	{
		int slot = fl->Find(pItem);
		char fullPath[SNM_MAX_PATH] = "";
		if (fl->GetFullPath(slot, fullPath, sizeof(fullPath)))
		{
			bool empty = (pItem->IsDefault() || *fullPath == '\0');
			iMemNeeded += (int)((empty ? strlen(DRAGDROP_EMPTY_SLOT) : strlen(fullPath)) + 1);
			fullPaths.Add(new WDL_FastString(empty ? DRAGDROP_EMPTY_SLOT : fullPath));
			g_dragResourceItems.Add(pItem);
		}
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

void ResourcesView::Perform()
{
	int x=0;
	ResourceItem* pItem = (ResourceItem*)EnumSelected(&x);
	int slot = g_SNM_ResSlots.Get(g_resType)->Find(pItem);
	if (pItem && slot >= 0) 
	{
		int dblClickType = LOWORD(g_dblClickPrefs[g_resType]);
		int dblClickTo = HIWORD(g_dblClickPrefs[g_resType]);
		switch(GetTypeForUser())
		{
			case SNM_SLOT_FXC:
				switch(dblClickTo)
				{
					case 0:
						ApplyTracksFXChainSlot(g_resType, !dblClickType?FXC_APPLY_TR_STR:FXC_PASTE_TR_STR, slot, !dblClickType, false);
						break;
					case 1:
						ApplyTracksFXChainSlot(g_resType, !dblClickType?FXCIN_APPLY_TR_STR:FXCIN_PASTE_TR_STR, slot, !dblClickType, true);
						break;
					case 2:
						ApplyTakesFXChainSlot(g_resType, !dblClickType?FXC_APPLY_TAKE_STR:FXC_PASTE_TAKE_STR, slot, true, !dblClickType);
						break;
					case 3:
						ApplyTakesFXChainSlot(g_resType, !dblClickType?FXC_APPLY_ALLTAKES_STR:FXC_PASTE_ALLTAKES_STR, slot, false, !dblClickType);
						break;
				}
				break;
			case SNM_SLOT_TR:
				switch(dblClickType)
				{
					case 0:
						ImportTrackTemplateSlot(g_resType, TRT_IMPORT_STR, slot);
						break;
					case 1:
						ApplyTrackTemplateSlot(g_resType, TRT_APPLY_STR, slot, false, false);
						break;
					case 2:
						ApplyTrackTemplateSlot(g_resType, TRT_APPLY_WITH_ENV_ITEM_STR, slot, true, true);
						break;
					case 3:
						ReplacePasteItemsTrackTemplateSlot(g_resType, TRT_PASTE_ITEMS_STR, slot, true);
						break;
					case 4:
						ReplacePasteItemsTrackTemplateSlot(g_resType, TRT_REPLACE_ITEMS_STR, slot, false);
						break;
				}
				break;
			case SNM_SLOT_PRJ:
				switch(dblClickType)
				{
					case 0:
						LoadOrSelectProjectSlot(g_resType, PRJ_SELECT_LOAD_STR, slot, false);
						break;
					case 1:
						LoadOrSelectProjectSlot(g_resType, PRJ_SELECT_LOAD_TAB_STR, slot, true);
						break;
				}
				break;
			case SNM_SLOT_MEDIA:
			{
				int insertMode = -1;
				switch(dblClickType) 
				{
					case 0: TogglePlaySelTrackMediaSlot(g_resType, MED_PLAY_STR, slot, false, false); break;
					case 1: TogglePlaySelTrackMediaSlot(g_resType, MED_LOOP_STR, slot, false, true); break;
					case 2: insertMode = 0; break;
					case 3: insertMode = 1; break;
					case 4: insertMode = 3; break;
				}
				if (insertMode >= 0)
					InsertMediaSlot(g_resType, MED_ADD_STR, slot, insertMode);
				break;
			}
			case SNM_SLOT_IMG:
				switch(dblClickType) {
					case 0: ShowImageSlot(g_resType, IMG_SHOW_STR, slot); break;
					case 1: SetSelTrackIconSlot(g_resType, IMG_TRICON_STR, slot); break;
					case 2: InsertMediaSlot(g_resType, MED_ADD_STR, slot, 0); break;
				}
				break;
	 		case SNM_SLOT_THM:
				LoadThemeSlot(g_resType, THM_LOAD_STR, slot);
				break;
			default:
				// works on OSX too
				if (WDL_FastString* fnStr = GetOrPromptOrBrowseSlot(g_resType, &slot)) 
				{
					fnStr->Insert("\"", 0);
					fnStr->Append("\"");
					ShellExecute(GetMainHwnd(), "open", fnStr->Get(), NULL, NULL, SW_SHOWNORMAL);
					delete fnStr;
				}
				break;
		}

		// update (in case the slot has changed, e.g. empty -> filled)
		Update();
	}
}


///////////////////////////////////////////////////////////////////////////////
// ResourcesWnd
///////////////////////////////////////////////////////////////////////////////

// S&M windows lazy init: below's "" prevents registering the SWS' screenset callback
// (use the S&M one instead - already registered via SNM_WindowManager::Init())
ResourcesWnd::ResourcesWnd()
	: SWS_DockWnd(IDD_SNM_RESOURCES, __LOCALIZE("Resources","sws_DLG_150"), "", SWSGetCommandID(OpenResources))
{
	m_id.Set(RES_WND_ID);
	// Must call SWS_DockWnd::Init() to restore parameters and open the window if necessary
	Init();
}

void ResourcesWnd::OnInitDlg()
{
	m_resize.init_item(IDC_LIST, 0.0, 0.0, 1.0, 1.0);
	m_resize.init_item(IDC_FILTER, 1.0, 1.0, 1.0, 1.0);

	SetWindowLongPtr(GetDlgItem(m_hwnd, IDC_FILTER), GWLP_USERDATA, 0xdeadf00b);
	SetWindowLongPtr(GetDlgItem(m_hwnd, IDC_EDIT), GWLP_USERDATA, 0xdeadf00b);

	m_pLists.Add(new ResourcesView(GetDlgItem(m_hwnd, IDC_LIST), GetDlgItem(m_hwnd, IDC_EDIT)));


	LICE_CachedFont* font = SNM_GetThemeFont();

	m_vwnd_painter.SetGSC(WDL_STYLE_GetSysColor);
	m_parentVwnd.SetRealParent(m_hwnd);

	m_cbType.SetID(CMBID_TYPE);
	m_cbType.SetFont(font);
	FillTypeCombo();
	m_parentVwnd.AddChild(&m_cbType);

	m_txtTiedPrj.SetID(TXTID_TIED_PRJ);
	m_txtTiedPrj.SetFont(SNM_GetThemeFont());
	m_parentVwnd.AddChild(&m_txtTiedPrj);

	m_cbDblClickType.SetID(CMBID_DBLCLICK_TYPE);
	m_cbDblClickType.SetFont(font);
	m_parentVwnd.AddChild(&m_cbDblClickType);

	m_cbDblClickTo.SetID(CMBID_DBLCLICK_TO);
	m_cbDblClickTo.SetFont(font);
	m_parentVwnd.AddChild(&m_cbDblClickTo);
	FillDblClickCombos();

	m_btnAutoFill.SetID(BTNID_AUTOFILL);
	m_parentVwnd.AddChild(&m_btnAutoFill);

	m_btnAutoSave.SetID(BTNID_AUTOSAVE);
	m_parentVwnd.AddChild(&m_btnAutoSave);

	m_txtDblClickType.SetID(TXTID_DBL_TYPE);
	m_txtDblClickType.SetFont(font);
	m_txtDblClickType.SetText(__LOCALIZE("Dbl-click to:","sws_DLG_150"));
	m_parentVwnd.AddChild(&m_txtDblClickType);

	m_btnAdd.SetID(BTNID_ADD_BOOKMARK);
	m_btnsAddDel.AddChild(&m_btnAdd);
	m_btnDel.SetID(BTNID_DEL_BOOKMARK);
	m_btnsAddDel.AddChild(&m_btnDel);
	m_btnsAddDel.SetID(WNDID_ADD_DEL);
	m_parentVwnd.AddChild(&m_btnsAddDel);

	m_txtDblClickTo.SetID(TXTID_DBL_TO);
	m_txtDblClickTo.SetFont(font);
	m_txtDblClickTo.SetText(__LOCALIZE("To sel.:","sws_DLG_150"));
	m_parentVwnd.AddChild(&m_txtDblClickTo);

	m_btnOffsetTrTemplate.SetID(BTNID_OFFSET_TR_TEMPLATE);
	m_btnOffsetTrTemplate.SetFont(font);
	m_parentVwnd.AddChild(&m_btnOffsetTrTemplate);

	// restores the text filter when docking/undocking + indirect call to Update()
	SetDlgItemText(m_hwnd, IDC_FILTER, g_filter.Get());

/* see above comment
	Update();
*/
}

void ResourcesWnd::OnDestroy() 
{
	m_cbType.Empty();
	m_cbDblClickType.Empty();
	m_cbDblClickTo.Empty();
	m_btnsAddDel.RemoveAllChildren(false);
}

void ResourcesWnd::SetType(int _type)
{
	int prevType = g_resType;
	g_resType = _type;
	m_cbType.SetCurSel(_type);
	if (prevType != g_resType) {
		FillDblClickCombos();
		Update();
	}
}

int ResourcesWnd::SetType(const char* _name)
{
	if (_name)
		for (int i=0; i<m_cbType.GetCount(); i++)
			if (!_stricmp(_name, m_cbType.GetItem(i))) {
				SetType(i);
				return i;
			}
	return -1;
}

void ResourcesWnd::Update()
{
	if (m_pLists.GetSize())
		GetListView()->Update();
	m_parentVwnd.RequestRedraw(NULL);
}

void ResourcesWnd::FillTypeCombo()
{
	m_cbType.Empty();
	for (int i=0; i < g_SNM_ResSlots.GetSize(); i++)
		if (char* p = _strdup(g_SNM_ResSlots.Get(i)->GetDesc())) {
			*p = toupper(*p); // 1st char to upper
			m_cbType.AddItem(p);
			free(p);
		}
	m_cbType.SetCurSel((g_resType>0 && g_resType<g_SNM_ResSlots.GetSize()) ? g_resType : SNM_SLOT_FXC);
}

void ResourcesWnd::FillDblClickCombos()
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
		case SNM_SLOT_THM:
			m_cbDblClickType.AddItem(__LOCALIZE("Load theme","sws_DLG_150"));
			break;
		default:
			if (typeForUser >= SNM_NUM_DEFAULT_SLOTS) // custom bookmark?
				m_cbDblClickType.AddItem(__LOCALIZE("Open file in default application","sws_DLG_150"));
			break;
	}
	m_cbDblClickType.SetCurSel(LOWORD(g_dblClickPrefs[g_resType]));
	m_cbDblClickTo.SetCurSel(HIWORD(g_dblClickPrefs[g_resType]));
}

void ResourcesWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	ResourceList* fl = g_SNM_ResSlots.Get(g_resType);
	if (!fl || !m_pLists.GetSize())
		return;

	int x=0;
	ResourceItem* item = (ResourceItem*)GetListView()->EnumSelected(&x);
	int slot = fl->Find(item);

	switch(LOWORD(wParam))
	{
		case IDC_FILTER:
			if (HIWORD(wParam)==EN_CHANGE)
			{
				char filter[128]="";
				GetWindowText(GetDlgItem(m_hwnd, IDC_FILTER), filter, sizeof(filter));
				g_filter.Set(filter);
				Update();
			}
#ifndef _SNM_SWELL_ISSUES // EN_SETFOCUS, EN_KILLFOCUS not supported yet
			else if (HIWORD(wParam)==EN_SETFOCUS)
			{
				HWND hFilt = GetDlgItem(m_hwnd, IDC_FILTER);
				char filter[128]="";
				GetWindowText(hFilt, filter, sizeof(filter));
				if (!strcmp(filter, FILTER_DEFAULT_STR))
					SetWindowText(hFilt, "");
			}
			else if (HIWORD(wParam)==EN_KILLFOCUS)
			{
				HWND hFilt = GetDlgItem(m_hwnd, IDC_FILTER);
				char filter[128]="";
				GetWindowText(hFilt, filter, sizeof(filter));
				if (*filter == '\0') 
					SetWindowText(hFilt, FILTER_DEFAULT_STR);
			}
#endif
			break;

		// ***** Common *****
		case ADD_SLOT_MSG:
			AddSlot();
			break;
		case INSERT_SLOT_MSG:
			InsertAtSelectedSlot();
			break;
		case DEL_SLOTS_MSG:
			ClearDeleteSlotsFiles(g_resType, 1);
			break;
		case CLR_SLOTS_MSG:
			ClearDeleteSlotsFiles(g_resType, 2);
			break;
		case DEL_FILES_MSG:
			ClearDeleteSlotsFiles(g_resType, 2|4);
			break;
		case LOAD_MSG:
			if (item && BrowseSlot(g_resType, slot, true))
				Update();
			break;
		case EDIT_MSG:
			if (item && slot>=0 && slot<fl->GetSize())
			{
				char fullPath[SNM_MAX_PATH] = "";
				if (fl->GetFullPath(slot, fullPath, sizeof(fullPath)) && FileOrDirExistsErrMsg(fullPath))
				{
					if (fl->IsText())
					{
						WDL_FastString syscmd;
#ifdef _WIN32
						// do not use ShellExecute's "edit" mode here as some file would be opened in
						// REAPER instead of the default text editor (e.g. .RTrackTemplate files)
						syscmd.SetFormatted(SNM_MAX_PATH, "\"%s\"", fullPath);
						ShellExecute(GetMainHwnd(), "open", "notepad", syscmd.Get(), NULL, SW_SHOWNORMAL);
#else
						syscmd.SetFormatted(SNM_MAX_PATH, "open -t \"%s\"", fullPath);
						system(syscmd.Get());
#endif
					}
				}
			}
			break;
#ifdef _WIN32
		case DIFF_MSG:
		{
			ResourceItem* item2 = (ResourceItem*)GetListView()->EnumSelected(&x);
			if (item && item2 && g_SNM_DiffToolFn.GetLength())
			{
				char fn1[SNM_MAX_PATH]="", fn2[SNM_MAX_PATH]="";
				if (slot>=0 && 
					fl->GetFullPath(slot, fn1, sizeof(fn1)) &&
					fl->GetFullPath(fl->Find(item2), fn2, sizeof(fn2)))
				{
					WDL_FastString prmStr;
					prmStr.SetFormatted(sizeof(fn1)*3, " \"%s\" \"%s\"", fn1, fn2);
					ShellExecute(GetMainHwnd(), "open", g_SNM_DiffToolFn.Get(), prmStr.Get(), NULL, SW_SHOWNORMAL);
				}
			}
			break;
		}
#endif
		case EXPLORE_MSG:
		{
			char fullPath[SNM_MAX_PATH] = "";
			if (fl->GetFullPath(slot, fullPath, sizeof(fullPath)))
				OpenSelectInExplorerFinder(fullPath);
			break;
		}
		case EXPLORE_FILLDIR_MSG:
			ShellExecute(NULL, "open", GetAutoFillDir(), NULL, NULL, SW_SHOWNORMAL);
			break;
		case EXPLORE_SAVEDIR_MSG:
			ShellExecute(NULL, "open", GetAutoSaveDir(), NULL, NULL, SW_SHOWNORMAL);
			break;
		// auto-fill
		case BTNID_AUTOFILL:
		case AUTOFILL_MSG:
			AutoFill(g_resType);
			break;
		case AUTOFILL_DIR_MSG:
		{
			char path[SNM_MAX_PATH] = "";
			if (BrowseForDirectory(__LOCALIZE("Set auto-fill directory","sws_DLG_150"), GetAutoFillDir(), path, sizeof(path)))
				SetAutoFillDir(path);
			break;
		}
		case AUTOFILL_PRJ_MSG:
		{
			char path[SNM_MAX_PATH] = "";
			GetProjectPath(path, sizeof(path));
			SetAutoFillDir(path);
			break;
		}
		case AUTOFILL_DEFAULT_MSG:
		{
			char path[SNM_MAX_PATH] = "";
			if (_snprintfStrict(path, sizeof(path), "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, fl->GetResourceDir()) > 0)
			{
				if (!FileOrDirExists(path))
					CreateDirectory(path, NULL);
				SetAutoFillDir(path);
			}
			break;
		}
		case AUTOFILL_SYNC_MSG: // i.e. sync w/ auto-save
			g_syncAutoDirPrefs[g_resType] = !g_syncAutoDirPrefs[g_resType];
			if (g_syncAutoDirPrefs[g_resType])
				g_autoFillDirs.Get(g_resType)->Set(GetAutoSaveDir());  // do not use SetAutoFillDir() here!
			break;
		// auto-save
		case BTNID_AUTOSAVE:
		case AUTOSAVE_MSG:
			AutoSave(g_resType, true, GetTypeForUser()==SNM_SLOT_TR ? g_asTrTmpltPref : GetTypeForUser()==SNM_SLOT_FXC ? g_asFXChainPref : 0);
			break;
		case AUTOSAVE_DIR_MSG:
		{
			char path[SNM_MAX_PATH] = "";
			if (BrowseForDirectory(__LOCALIZE("Set auto-save directory","sws_DLG_150"), GetAutoSaveDir(), path, sizeof(path)))
				SetAutoSaveDir(path);
			break;
		}
		case AUTOSAVE_DIR_PRJ_MSG:
		{
			char prjPath[SNM_MAX_PATH] = "", path[SNM_MAX_PATH] = "";
			GetProjectPath(prjPath, sizeof(prjPath));

			// see GetFileRelativePath..
			if (_snprintfStrict(path, sizeof(path), "%s%c%s", prjPath, PATH_SLASH_CHAR, GetFileRelativePath(fl->GetResourceDir())) > 0)
			{
				if (!FileOrDirExists(path))
					CreateDirectory(path, NULL);
				SetAutoSaveDir(path);
			}
			break;
		}
		case AUTOSAVE_DIR_PRJ2_MSG:
		{
			char prjPath[SNM_MAX_PATH] = "";
			GetProjectPath(prjPath, sizeof(prjPath));
			SetAutoSaveDir(prjPath);
			break;
		}
		case AUTOSAVE_DIR_DEFAULT_MSG:
		{
			char path[SNM_MAX_PATH] = "";
			if (_snprintfStrict(path, sizeof(path), "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, fl->GetResourceDir()) > 0) {
				if (!FileOrDirExists(path))
					CreateDirectory(path, NULL);
				SetAutoSaveDir(path);
			}
			break;
		}
		case AUTOSAVE_SYNC_MSG: // i.e. sync w/ auto-fill
			g_syncAutoDirPrefs[g_resType] = !g_syncAutoDirPrefs[g_resType];
			if (g_syncAutoDirPrefs[g_resType])
				g_autoSaveDirs.Get(g_resType)->Set(GetAutoFillDir()); // do not use SetAutoSaveDir() here!
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
			if (item)
			{
				char fullPath[SNM_MAX_PATH] = "";
				if (fl->GetFullPath(slot, fullPath, sizeof(fullPath)) && FileOrDirExistsErrMsg(fullPath))
					GetListView()->EditListItem((SWS_ListItem*)item, COL_NAME);
			}
			break;
		case REN_BOOKMARK_MSG:
			RenameBookmark(g_resType);
			break;
		case BTNID_ADD_BOOKMARK:
			NewBookmark(g_resType, false);
			break;
		case COPY_BOOKMARK_MSG:
			NewBookmark(g_resType, true);
			break;
		case BTNID_DEL_BOOKMARK:
		case DEL_BOOKMARK_MSG:
			DeleteBookmark(g_resType);
			break;

		// new bookmark cmds are in an interval of cmds: see "default" case below..

		case LOAD_TIED_PRJ_MSG:
		case LOAD_TIED_PRJ_TAB_MSG:
			if (g_tiedProjects.Get(g_resType)->GetLength())
				LoadOrSelectProject(g_tiedProjects.Get(g_resType)->Get(), LOWORD(wParam)==LOAD_TIED_PRJ_TAB_MSG);
			break;
		case TIE_ACTIONS_MSG:
			g_SNM_TiedSlotActions[GetTypeForUser()] = g_resType;
			break;
		case UNTIE_PROJECT_MSG:
		{
			char path[SNM_MAX_PATH]="";
			ReaProject* prj = EnumProjects(-1, path, sizeof(path));
			UntieAllResFileFromProject(prj, path, g_resType);
			g_tiedProjects.Get(g_resType)->Set("");
			m_parentVwnd.RequestRedraw(NULL);
			break;
		}
		case TIE_PROJECT_MSG:
		{
			char path[SNM_MAX_PATH]="";
			ReaProject* prj = EnumProjects(-1, path, sizeof(path));
			UntieAllResFileFromProject(prj, path, g_resType);
			g_tiedProjects.Get(g_resType)->Set(path);
			TieAllResFileToProject(prj, path, g_resType);
			m_parentVwnd.RequestRedraw(NULL);
			break;
		}

		// ***** FX chain *****
		case FXC_APPLY_TR_MSG:
		case FXC_PASTE_TR_MSG:
			if (item && slot >= 0) {
				ApplyTracksFXChainSlot(g_resType, LOWORD(wParam)==FXC_APPLY_TR_MSG?FXC_APPLY_TR_STR:FXC_PASTE_TR_STR, slot, LOWORD(wParam)==FXC_APPLY_TR_MSG, false);
				Update();
			}
			break;
		case FXC_APPLY_TAKE_MSG:
		case FXC_PASTE_TAKE_MSG:
			if (item && slot >= 0) {
				ApplyTakesFXChainSlot(g_resType, LOWORD(wParam)==FXC_APPLY_TAKE_MSG?FXC_APPLY_TAKE_STR:FXC_PASTE_TAKE_STR, slot, true, LOWORD(wParam)==FXC_APPLY_TAKE_MSG);
				Update();
			}
			break;
		case FXC_APPLY_ALLTAKES_MSG:
		case FXC_PASTE_ALLTAKES_MSG:
			if (item && slot >= 0) {
				ApplyTakesFXChainSlot(g_resType, LOWORD(wParam)==FXC_APPLY_ALLTAKES_MSG?FXC_APPLY_ALLTAKES_STR:FXC_PASTE_ALLTAKES_STR, slot, false, LOWORD(wParam)==FXC_APPLY_ALLTAKES_MSG);
				Update();
			}
			break;
		case FXC_AUTOSAVE_INPUTFX_MSG:
			g_asFXChainPref = FXC_AUTOSAVE_PREF_INPUT_FX;
			break;
		case FXC_AUTOSAVE_TR_MSG:
			g_asFXChainPref = FXC_AUTOSAVE_PREF_TRACK;
			break;
		case FXC_AUTOSAVE_ITEM_MSG:
			g_asFXChainPref = FXC_AUTOSAVE_PREF_ITEM;
			break;
		case FXC_AUTOSAVE_DEFNAME_MSG:
		case FXC_AUTOSAVE_FX1NAME_MSG:
			g_asFXChainNamePref = g_asFXChainNamePref ? 0:1;
			break;

		// ***** Track template *****
		case TRT_APPLY_MSG:
			if (item && slot >= 0) {
				ApplyTrackTemplateSlot(g_resType, TRT_APPLY_STR, slot, false, false);
				Update();
			}
			break;
		case TRT_APPLY_WITH_ENV_ITEM_MSG:
			if (item && slot >= 0) {
				ApplyTrackTemplateSlot(g_resType, TRT_APPLY_WITH_ENV_ITEM_STR, slot, true, true);
				Update();
			}
			break;
		case TRT_IMPORT_MSG:
			if (item && slot >= 0) {
				ImportTrackTemplateSlot(g_resType, TRT_IMPORT_STR, slot);
				Update();
			}
			break;
		case TRT_REPLACE_ITEMS_MSG:
		case TRT_PASTE_ITEMS_MSG:
			if (item && slot >= 0) {
				ReplacePasteItemsTrackTemplateSlot(g_resType, LOWORD(wParam)==TRT_REPLACE_ITEMS_MSG?TRT_REPLACE_ITEMS_STR:TRT_PASTE_ITEMS_STR, slot, LOWORD(wParam)==TRT_PASTE_ITEMS_MSG);
				Update();
			}
			break;
		case BTNID_OFFSET_TR_TEMPLATE:
			if (int* offsPref = (int*)GetConfigVar("templateditcursor")) { // >= REAPER v4.15
				if (*offsPref) *offsPref = 0;
				else *offsPref = 1;
			}
			break;
		case TRT_AUTOSAVE_WITEMS_MSG:
			if (g_asTrTmpltPref & 1) g_asTrTmpltPref &= 0xE;
			else g_asTrTmpltPref |= 1;
			break;
		case TRT_AUTOSAVE_WENVS_MSG:
			if (g_asTrTmpltPref & 2) g_asTrTmpltPref &= 0xD;
			else g_asTrTmpltPref |= 2;
			break;

		// ***** Project template *****
		case PRJ_OPEN_SELECT_MSG:
			if (item && slot >= 0) {
				LoadOrSelectProjectSlot(g_resType, PRJ_SELECT_LOAD_STR, slot, false);
				Update();
			}
			break;
		case PRJ_OPEN_SELECT_TAB_MSG:
			while(item)
			{
				slot = fl->Find(item);
				if (slot>=0)
					LoadOrSelectProjectSlot(g_resType, PRJ_SELECT_LOAD_TAB_STR, slot, true);
				item = (ResourceItem*)GetListView()->EnumSelected(&x);
			}
			Update();
			break;
		case PRJ_AUTOFILL_RECENTS_MSG:
		{
			int startSlot = fl->GetSize();
			int nbRecents = GetPrivateProfileInt("REAPER", "numrecent", 0, get_ini_file());
			nbRecents = min(98, nbRecents); // just in case: 2 digits max..
			if (nbRecents)
			{
				char key[16], path[SNM_MAX_PATH];
				WDL_PtrList_DeleteOnDestroy<WDL_FastString> prjs;
				for (int i=0; i < nbRecents; i++) {
					if (_snprintfStrict(key, sizeof(key), "recent%02d", i+1) > 0) {
						GetPrivateProfileString("Recent", key, "", path, sizeof(path), get_ini_file());
						if (*path)
							prjs.Add(new WDL_FastString(path));
					}
				}
				for (int i=0; i < prjs.GetSize(); i++)
					if (fl->FindByPath(prjs.Get(i)->Get()) < 0) { // skip if already present
						fl->AddSlot(prjs.Get(i)->Get());
						TieResFileToProject(prjs.Get(i)->Get(), g_resType);
					}
			}
			if (startSlot != fl->GetSize()) {
				Update();
				SelectBySlot(startSlot, fl->GetSize());
			}
			else 
			{
				char msg[SNM_MAX_PATH] = "";
				_snprintfSafe(msg, sizeof(msg), __LOCALIZE_VERFMT("No new recent projects found!\n%s","sws_DLG_150"), AUTOFILL_ERR_STR);
				MessageBox(m_hwnd, msg, __LOCALIZE("S&M - Warning","sws_DLG_150"), MB_OK);
			}
			break;
		}
		case PRJ_LOADER_SET_MSG:
		{
			int min=fl->GetSize(), max=0;
			while(item)
			{
				slot = fl->Find(item)+1;
				if (slot>max) max = slot;
				if (slot<min) min = slot;
				item = (ResourceItem*)GetListView()->EnumSelected(&x);
			}
			if (max>min)
			{
				g_SNM_PrjLoaderStartPref = min;
				g_SNM_PrjLoaderEndPref = max;
				Update();
			}
			break;
		}
		case PRJ_LOADER_CLEAR_MSG:
			g_SNM_PrjLoaderStartPref = g_SNM_PrjLoaderEndPref = -1;
			Update();
			break;

		// ***** media files *****
		case MED_PLAY_MSG:
		case MED_LOOP_MSG:
			while(item)
			{
				slot = fl->Find(item);
				if (slot>=0)
					TogglePlaySelTrackMediaSlot(g_resType, MED_PLAYLOOP_STR, slot, false, LOWORD(wParam)==MED_LOOP_MSG);
				item = (ResourceItem*)GetListView()->EnumSelected(&x);
			}
			Update();
			break;
		case MED_ADD_CURTR_MSG:
		case MED_ADD_NEWTR_MSG:
		case MED_ADD_TAKES_MSG:
			while(item)
			{
				slot = fl->Find(item);
				if (slot>=0)
					InsertMediaSlot(g_resType, MED_ADD_STR, slot, LOWORD(wParam)==MED_ADD_CURTR_MSG ? 0 : LOWORD(wParam)==MED_ADD_NEWTR_MSG ? 1 : 3);
				item = (ResourceItem*)GetListView()->EnumSelected(&x);
			}
			Update();
			break;

		// ***** theme *****
		case THM_LOAD_MSG:
			LoadThemeSlot(g_resType, THM_LOAD_STR, slot);
			Update();
			break;

		// ***** image *****
		case IMG_SHOW_MSG:
			ShowImageSlot(g_resType, IMG_SHOW_STR, slot);
			Update();
			break;
		case IMG_TRICON_MSG:
			SetSelTrackIconSlot(g_resType, IMG_TRICON_STR, slot);
			Update();
			break;

			// ***** WDL GUI & others *****
		case CMBID_TYPE:
			if (HIWORD(wParam)==CBN_SELCHANGE)
			{
				// stop cell editing (changing the list content would be ignored otherwise,
				// leading to unsynchronized dropdown box vs list view)
				GetListView()->EditListItemEnd(false);
				SetType(m_cbType.GetCurSel());
			}
			break;
		case CMBID_DBLCLICK_TYPE:
			if (HIWORD(wParam)==CBN_SELCHANGE) {
				int hi = HIWORD(g_dblClickPrefs[g_resType]);
				g_dblClickPrefs[g_resType] = m_cbDblClickType.GetCurSel();
				g_dblClickPrefs[g_resType] |= hi<<16;
				m_parentVwnd.RequestRedraw(NULL); // some options can be hidden for new sel
			}
			break;
		case CMBID_DBLCLICK_TO:
			if (HIWORD(wParam)==CBN_SELCHANGE) {
				g_dblClickPrefs[g_resType] = LOWORD(g_dblClickPrefs[g_resType]);
				g_dblClickPrefs[g_resType] |= m_cbDblClickTo.GetCurSel()<<16;
			}
			break;
		default:
			if (LOWORD(wParam) >= NEW_BOOKMARK_START_MSG && LOWORD(wParam) < (NEW_BOOKMARK_START_MSG+SNM_NUM_DEFAULT_SLOTS))
				NewBookmark(LOWORD(wParam)-NEW_BOOKMARK_START_MSG, false);
			// custom bookmark
			else if (LOWORD(wParam) == (NEW_BOOKMARK_START_MSG+SNM_NUM_DEFAULT_SLOTS))
				NewBookmark(-1, false);
			else
				Main_OnCommand((int)wParam, (int)lParam);
			break;
	}
}

HMENU ResourcesWnd::AutoSaveContextMenu(HMENU _menu, bool _saveItem)
{
	int typeForUser = GetTypeForUser();
	char autoPath[SNM_MAX_PATH] = "";
	_snprintfSafe(autoPath, sizeof(autoPath), __LOCALIZE_VERFMT("[Current auto-save path: %s]","sws_DLG_150"), *GetAutoSaveDir() ? GetAutoSaveDir() : __LOCALIZE("undefined","sws_DLG_150"));
	AddToMenu(_menu, autoPath, 0, -1, false, MF_GRAYED);
	AddToMenu(_menu, __LOCALIZE("Show auto-save path in explorer/finder...","sws_DLG_150"), EXPLORE_SAVEDIR_MSG, -1, false, *GetAutoSaveDir() ? MF_ENABLED : MF_GRAYED);

	AddToMenu(_menu, __LOCALIZE("Sync auto-save and auto-fill paths","sws_DLG_150"), AUTOFILL_SYNC_MSG, -1, false, g_syncAutoDirPrefs[g_resType] ? MFS_CHECKED : MFS_UNCHECKED);

	if (_saveItem) {
		AddToMenu(_menu, SWS_SEPARATOR, 0);
		AddToMenu(_menu, __LOCALIZE("Auto-save","sws_DLG_150"), AUTOSAVE_MSG, -1, false, !IsFiltered() ? MF_ENABLED : MF_GRAYED);
	}

	AddToMenu(_menu, SWS_SEPARATOR, 0);
	AddToMenu(_menu, __LOCALIZE("Set auto-save directory...","sws_DLG_150"), AUTOSAVE_DIR_MSG);
	AddToMenu(_menu, __LOCALIZE("Set auto-save directory to default resource path","sws_DLG_150"), AUTOSAVE_DIR_DEFAULT_MSG);
	AddToMenu(_menu, __LOCALIZE("Set auto-save directory to project path","sws_DLG_150"), AUTOSAVE_DIR_PRJ2_MSG);
	switch(typeForUser)
	{
		case SNM_SLOT_FXC:
			AddToMenu(_menu, __LOCALIZE("Set auto-save directory to project path (/FXChains)","sws_DLG_150"), AUTOSAVE_DIR_PRJ_MSG);
			AddToMenu(_menu, SWS_SEPARATOR, 0);
			AddToMenu(_menu, __LOCALIZE("Auto-save FX chains from track selection","sws_DLG_150"), FXC_AUTOSAVE_TR_MSG, -1, false, g_asFXChainPref == FXC_AUTOSAVE_PREF_TRACK ? MFS_CHECKED : MFS_UNCHECKED);
			AddToMenu(_menu, __LOCALIZE("Auto-save FX chains from item selection","sws_DLG_150"), FXC_AUTOSAVE_ITEM_MSG, -1, false, g_asFXChainPref == FXC_AUTOSAVE_PREF_ITEM ? MFS_CHECKED : MFS_UNCHECKED);
			AddToMenu(_menu, __LOCALIZE("Auto-save input FX chains from track selection","sws_DLG_150"), FXC_AUTOSAVE_INPUTFX_MSG, -1, false, g_asFXChainPref == FXC_AUTOSAVE_PREF_INPUT_FX ? MFS_CHECKED : MFS_UNCHECKED);
			AddToMenu(_menu, SWS_SEPARATOR, 0);
			AddToMenu(_menu, __LOCALIZE("Generate filename from track/item name","sws_DLG_150"), FXC_AUTOSAVE_DEFNAME_MSG, -1, false, !g_asFXChainNamePref ? MFS_CHECKED : MFS_UNCHECKED);
			AddToMenu(_menu, __LOCALIZE("Generate filename from first FX name","sws_DLG_150"), FXC_AUTOSAVE_FX1NAME_MSG, -1, false, g_asFXChainNamePref ? MFS_CHECKED : MFS_UNCHECKED);
			break;
		case SNM_SLOT_TR:
			AddToMenu(_menu, __LOCALIZE("Set auto-save directory to project path (/TrackTemplates)","sws_DLG_150"), AUTOSAVE_DIR_PRJ_MSG);
			AddToMenu(_menu, SWS_SEPARATOR, 0);
			AddToMenu(_menu, __LOCALIZE("Include track items in templates","sws_DLG_150"), TRT_AUTOSAVE_WITEMS_MSG, -1, false, (g_asTrTmpltPref&1) ? MFS_CHECKED : MFS_UNCHECKED);
			AddToMenu(_menu, __LOCALIZE("Include envelopes in templates","sws_DLG_150"), TRT_AUTOSAVE_WENVS_MSG, -1, false, (g_asTrTmpltPref&2) ? MFS_CHECKED : MFS_UNCHECKED);
			break;
		case SNM_SLOT_PRJ:
			AddToMenu(_menu, __LOCALIZE("Set auto-save directory to project path (/ProjectTemplates)","sws_DLG_150"), AUTOSAVE_DIR_PRJ_MSG);
			break;
	}
	return _menu;
}

HMENU ResourcesWnd::AutoFillContextMenu(HMENU _menu, bool _fillItem)
{
	int typeForUser = GetTypeForUser();
	char autoPath[SNM_MAX_PATH] = "";
	_snprintfSafe(autoPath, sizeof(autoPath), __LOCALIZE_VERFMT("[Current auto-fill path: %s]","sws_DLG_150"), *GetAutoFillDir() ? GetAutoFillDir() : __LOCALIZE("undefined","sws_DLG_150"));
	AddToMenu(_menu, autoPath, 0, -1, false, MF_GRAYED);
	AddToMenu(_menu, __LOCALIZE("Show auto-fill path in explorer/finder...","sws_DLG_150"), EXPLORE_FILLDIR_MSG, -1, false, *GetAutoFillDir() ? MF_ENABLED : MF_GRAYED);

	if (g_SNM_ResSlots.Get(g_resType)->IsAutoSave())
		AddToMenu(_menu, __LOCALIZE("Sync auto-save and auto-fill paths","sws_DLG_150"), AUTOSAVE_SYNC_MSG, -1, false, g_syncAutoDirPrefs[g_resType] ? MFS_CHECKED : MFS_UNCHECKED);

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
	return _menu;
}

HMENU ResourcesWnd::AttachPrjContextMenu(HMENU _menu, bool _openSelPrj)
{
	// bookmark, custom type?
	if (g_resType>=SNM_NUM_DEFAULT_SLOTS) //no! && g_tiedProjects.Get(g_resType)->GetLength())
	{
		char buf[128] = "";
		bool curPrjTied = (g_tiedProjects.Get(g_resType)->GetLength() && !_stricmp(g_tiedProjects.Get(g_resType)->Get(), g_curProjectFn));

		// tied prj but not open?
		if (_openSelPrj && g_tiedProjects.Get(g_resType)->GetLength() && !curPrjTied)
		{
			if (GetMenuItemCount(_menu))
				AddToMenu(_menu, SWS_SEPARATOR, 0);
			_snprintfSafe(buf, sizeof(buf), TIED_PRJ_SELECT_LOAD_STR, *g_curProjectFn ? GetFileRelativePath(g_curProjectFn) : __LOCALIZE("(unsaved project?)","sws_DLG_150"));
			AddToMenu(_menu, buf, LOAD_TIED_PRJ_MSG);
			_snprintfSafe(buf, sizeof(buf), TIED_PRJ_SELECT_LOADTAB_STR, *g_curProjectFn ? GetFileRelativePath(g_curProjectFn) : __LOCALIZE("(unsaved project?)","sws_DLG_150"));
			AddToMenu(_menu, buf, LOAD_TIED_PRJ_TAB_MSG);
		}

		if (GetMenuItemCount(_menu))
			AddToMenu(_menu, SWS_SEPARATOR, 0);

		// ensure g_curProjectFn is up to date, job performed immediately
		ResourcesAttachJob job;
		job.Perform();

		_snprintfSafe(buf, sizeof(buf), __LOCALIZE_VERFMT("Attach bookmark to %s","sws_DLG_150"), *g_curProjectFn ? GetFileRelativePath(g_curProjectFn) : __LOCALIZE("(unsaved project?)","sws_DLG_150"));
		AddToMenu(_menu, buf, TIE_PROJECT_MSG, -1, false, *g_curProjectFn && !curPrjTied ? MF_ENABLED : MF_GRAYED);

		_snprintfSafe(buf, sizeof(buf), __LOCALIZE_VERFMT("Detach bookmark from %s","sws_DLG_150"), GetFileRelativePath(g_tiedProjects.Get(g_resType)->Get()));
		AddToMenu(_menu, g_tiedProjects.Get(g_resType)->GetLength() ? buf : __LOCALIZE("Detach bookmark files","sws_DLG_150"), UNTIE_PROJECT_MSG, -1, false, g_tiedProjects.Get(g_resType)->GetLength() ? MF_ENABLED : MF_GRAYED);
	}
	return _menu;
}

HMENU ResourcesWnd::BookmarkContextMenu(HMENU _menu)
{
	int typeForUser = GetTypeForUser();

	HMENU hNewBookmarkSubMenu = CreatePopupMenu();
	AddSubMenu(_menu, hNewBookmarkSubMenu, __LOCALIZE("New bookmark","sws_DLG_150"));
	for (int i=0; i < SNM_NUM_DEFAULT_SLOTS; i++)
		if (char* p = _strdup(g_SNM_ResSlots.Get(i)->GetDesc())) {
			*p = toupper(*p); // 1st char to upper
			AddToMenu(hNewBookmarkSubMenu, p, NEW_BOOKMARK_START_MSG+i);
			free(p);
		}
	AddToMenu(hNewBookmarkSubMenu, __LOCALIZE("Custom...","sws_DLG_150"), NEW_BOOKMARK_START_MSG+SNM_NUM_DEFAULT_SLOTS);

	AddToMenu(_menu, __LOCALIZE("Copy bookmark...","sws_DLG_150"), COPY_BOOKMARK_MSG);
	AddToMenu(_menu, __LOCALIZE("Rename bookmark...","sws_DLG_150"), REN_BOOKMARK_MSG, -1, false, g_resType >= SNM_NUM_DEFAULT_SLOTS ? MF_ENABLED : MF_GRAYED);
	AddToMenu(_menu, __LOCALIZE("Delete bookmark","sws_DLG_150"), DEL_BOOKMARK_MSG, -1, false, g_resType >= SNM_NUM_DEFAULT_SLOTS ? MF_ENABLED : MF_GRAYED);

	AttachPrjContextMenu(_menu, false);

	if (IsMultiType())
	{
		char buf[128] = "";
		AddToMenu(_menu, SWS_SEPARATOR, 0);
		_snprintfSafe(buf, sizeof(buf), __LOCALIZE_VERFMT("Attach %s slot actions to this bookmark","sws_DLG_150"), g_SNM_ResSlots.Get(typeForUser)->GetDesc());
		AddToMenu(_menu, buf, TIE_ACTIONS_MSG, -1, false, g_SNM_TiedSlotActions[typeForUser] == g_resType ? MFS_CHECKED : MFS_UNCHECKED);
	}
	return _menu;
}


HMENU ResourcesWnd::OnContextMenu(int _x, int _y, bool* _wantDefaultItems)
{
	ResourceList* fl = g_SNM_ResSlots.Get(g_resType);
	if (!fl)
		return NULL;

	HMENU hMenu = CreatePopupMenu();

	// specific context menu for auto-fill/auto-save buttons
	POINT p; GetCursorPos(&p);
	ScreenToClient(m_hwnd,&p);
	if (WDL_VWnd* v = m_parentVwnd.VirtWndFromPoint(p.x,p.y,1))
	{
		switch (v->GetID())
		{
			case BTNID_AUTOFILL:
				*_wantDefaultItems = false;
				return AutoFillContextMenu(hMenu, false);
			case BTNID_AUTOSAVE:
				if (fl->IsAutoSave()) {
					*_wantDefaultItems = false;
					return AutoSaveContextMenu(hMenu, false);
				}
				break;
			case CMBID_TYPE:
				*_wantDefaultItems = false;
				return BookmarkContextMenu(hMenu);
			case TXTID_TIED_PRJ:
				if (g_tiedProjects.Get(g_resType)->GetLength())
				{
					*_wantDefaultItems = false;
					AttachPrjContextMenu(hMenu, true);
					return hMenu;
				}
				break;
		}
	}

	// general context menu
	int iCol;
	ResourceItem* pItem = (ResourceItem*)GetListView()->GetHitItem(_x, _y, &iCol);
	UINT enabled = (pItem && !pItem->IsDefault()) ? MF_ENABLED : MF_GRAYED;
	int typeForUser = GetTypeForUser();
	if (pItem && iCol >= 0)
	{
		*_wantDefaultItems = false;
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
				if (g_resType == typeForUser)  // no GetTypeForUser() here: only one loader/selecter config atm..
				{
					AddToMenu(hMenu, SWS_SEPARATOR, 0);
					int x=0, nbsel=0; while(GetListView()->EnumSelected(&x)) nbsel++;
					AddToMenu(hMenu, __LOCALIZE("Project loader/selecter configuration...","sws_DLG_150"), SWSGetCommandID(ProjectLoaderConf));
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
			case SNM_SLOT_THM:
				AddToMenu(hMenu, THM_LOAD_STR, THM_LOAD_MSG, -1, false, enabled);
				break;
		}
	}

	if (GetMenuItemCount(hMenu))
		AddToMenu(hMenu, SWS_SEPARATOR, 0);

	// always displayed, even if the list is empty
	AddToMenu(hMenu, __LOCALIZE("Add slot","sws_DLG_150"), ADD_SLOT_MSG, -1, false, !IsFiltered() ? MF_ENABLED : MF_GRAYED);

	if (pItem && iCol >= 0)
	{
		AddToMenu(hMenu, __LOCALIZE("Insert slot","sws_DLG_150"), INSERT_SLOT_MSG, -1, false, !IsFiltered() ? MF_ENABLED : MF_GRAYED);
		AddToMenu(hMenu, __LOCALIZE("Clear slots","sws_DLG_150"), CLR_SLOTS_MSG, -1, false, enabled);
		AddToMenu(hMenu, __LOCALIZE("Delete slots","sws_DLG_150"), DEL_SLOTS_MSG);

		AddToMenu(hMenu, SWS_SEPARATOR, 0);
		AddToMenu(hMenu, __LOCALIZE("Load slot/file...","sws_DLG_150"), LOAD_MSG);
		AddToMenu(hMenu, __LOCALIZE("Delete files","sws_DLG_150"), DEL_FILES_MSG, -1, false, enabled);
		AddToMenu(hMenu, __LOCALIZE("Rename file","sws_DLG_150"), RENAME_MSG, -1, false, enabled);
		if (fl->IsText())
		{
#ifdef _WIN32
			int x=0, nbsel=0; while(GetListView()->EnumSelected(&x)) nbsel++;
			AddToMenu(hMenu, __LOCALIZE("Diff files...","sws_DLG_150"), DIFF_MSG, -1, false, nbsel==2 && enabled==MF_ENABLED && g_SNM_DiffToolFn.GetLength() ? MF_ENABLED:MF_GRAYED);
#endif
			AddToMenu(hMenu, __LOCALIZE("Edit file...","sws_DLG_150"), EDIT_MSG, -1, false, enabled);
		}	
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
		if (fl->IsAutoSave())
		{
			HMENU hAutoSaveSubMenu = CreatePopupMenu();
			AddSubMenu(hMenu, hAutoSaveSubMenu, __LOCALIZE("Auto-save","sws_DLG_150"));
			AutoSaveContextMenu(hAutoSaveSubMenu, true);
		}

		// bookmarks
		AddToMenu(hMenu, SWS_SEPARATOR, 0);
		HMENU hBookmarkSubMenu = CreatePopupMenu();
		AddSubMenu(hMenu, hBookmarkSubMenu, __LOCALIZE("Bookmark","sws_DLG_150"));
		BookmarkContextMenu(hBookmarkSubMenu);

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

int ResourcesWnd::OnKey(MSG* _msg, int _iKeyState) 
{
	if (_msg->message == WM_KEYDOWN)
	{
		if (!_iKeyState)
		{
			switch(_msg->wParam)
			{
				case VK_F2:
					OnCommand(RENAME_MSG, 0);
					return 1;
				case VK_DELETE:
					ClearDeleteSlotsFiles(g_resType, 1);
					return 1;
				case VK_INSERT:
					InsertAtSelectedSlot();
					return 1;
				case VK_RETURN:
					((ResourcesView*)GetListView())->Perform();
					return 1;
			}
		}
		// ctrl+A => select all
		else if (_iKeyState == LVKF_CONTROL && _msg->wParam == 'A')
		{
			HWND h = GetDlgItem(m_hwnd, IDC_FILTER);
#ifdef _WIN32
			if (_msg->hwnd == h)
#else
			if (GetFocus() == h)
#endif
			{
				SetFocus(h);
				SendMessage(h, EM_SETSEL, 0, -1);
				return 1; // eat
			}
		}
	}
	return 0;
}

int ResourcesWnd::GetValidDroppedFilesCount(HDROP _h)
{
	int validCnt=0;
	int iFiles = DragQueryFile(_h, 0xFFFFFFFF, NULL, 0);
	char fn[SNM_MAX_PATH] = "";
	for (int i=0; i < iFiles; i++) 
	{
		DragQueryFile(_h, i, fn, sizeof(fn));
		if (!strcmp(fn, DRAGDROP_EMPTY_SLOT) || g_SNM_ResSlots.Get(GetTypeForUser())->IsValidFileExt(GetFileExtension(fn)))
			validCnt++;
	}
	return validCnt;
}

void ResourcesWnd::OnDroppedFiles(HDROP _h)
{
	ResourceList* fl = g_SNM_ResSlots.Get(g_resType);
	if (!fl)
		return;

	int dropped = 0; //nb of successfully dropped files
	int iFiles = DragQueryFile(_h, 0xFFFFFFFF, NULL, 0);
	int iValidFiles = GetValidDroppedFilesCount(_h);

	POINT pt;
	DragQueryPoint(_h, &pt);

	RECT r; // ClientToScreen doesn't work right, wtf?
	GetWindowRect(m_hwnd, &r);
	pt.x += r.left;
	pt.y += r.top;

	ResourceItem* pItem = (ResourceItem*)GetListView()->GetHitItem(pt.x, pt.y, NULL);
	int dropSlot = fl->Find(pItem);

	// internal drag-drop?
	if (g_dragResourceItems.GetSize())
	{
		int srcSlot = fl->Find(g_dragResourceItems.Get(0));
		// drag-drop slot to the bottom
		if (srcSlot >= 0 && srcSlot < dropSlot)
			dropSlot++; // drag-drop will be more "natural"
	}

	// drop but not on a slot => create slots
	if (!pItem || dropSlot < 0 || dropSlot >= fl->GetSize()) 
	{
		dropSlot = fl->GetSize();
		for (int i=0; i < iValidFiles; i++)
			fl->Add(new ResourceItem());
	}
	// drop on a slot => insert slots at drop point
	else 
	{
		for (int i=0; i < iValidFiles; i++)
			fl->InsertSlot(dropSlot);
	}

	// re-sync pItem 
	pItem = fl->Get(dropSlot); 

	// patch slots from drop data
	char fn[SNM_MAX_PATH] = "";
	int slot;
	for (int i=0; pItem && i < iFiles; i++)
	{
		slot = fl->Find(pItem);
		DragQueryFile(_h, i, fn, sizeof(fn));

		// internal drag-drop?
		if (g_dragResourceItems.GetSize())
		{
			if (ResourceItem* item = fl->Get(slot))
			{
				item->m_shortPath.Set(g_dragResourceItems.Get(i)->m_shortPath.Get());
				item->m_comment.Set(g_dragResourceItems.Get(i)->m_comment.Get());
				dropped++;
				pItem = fl->Get(slot+1); 
			}
		}
		// .rfxchain? .rTrackTemplate? etc..
		else if (g_SNM_ResSlots.Get(GetTypeForUser())->IsValidFileExt(GetFileExtension(fn)))
		{
			if (fl->SetFromFullPath(slot, fn))
			{
				dropped++;
				pItem = fl->Get(slot+1);
				TieResFileToProject(fn, g_resType);
			}
		}
	}

	if (dropped)
	{
		// internal drag-drop: move (=> delete previous slots)
		for (int i=0; i < g_dragResourceItems.GetSize(); i++)
			for (int j=fl->GetSize()-1; j >= 0; j--)
				if (fl->Get(j) == g_dragResourceItems.Get(i))
				{
					if (j < dropSlot)
						dropSlot--;
					fl->Delete(j, false);
				}

		Update();

		// Select item at drop point
		if (dropSlot >= 0)
			SelectBySlot(dropSlot);
	}

	g_dragResourceItems.Empty(false);
	DragFinish(_h);
}

void ResourcesWnd::DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight)
{
	ResourceList* fl = g_SNM_ResSlots.Get(g_resType);
	if (!fl)
		return;

	// 1st row of controls (top)

	int x0=_r->left+SNM_GUI_X_MARGIN, h=SNM_GUI_TOP_H;
	if (_tooltipHeight)
		*_tooltipHeight = h;

	IconTheme* it = SNM_GetIconTheme();
	int typeForUser = GetTypeForUser();

	// "auto-fill" button
	SNM_SkinButton(&m_btnAutoFill, it ? &(it->toolbar_open) : NULL, __LOCALIZE("Auto-fill","sws_DLG_150"));
	if (SNM_AutoVWndPosition(DT_LEFT, &m_btnAutoFill, NULL, _r, &x0, _r->top, h, 0))
	{
		// "auto-save" button
		m_btnAutoSave.SetGrayed(!fl->IsAutoSave());
		SNM_SkinButton(&m_btnAutoSave, it ? &(it->toolbar_save) : NULL, __LOCALIZE("Auto-save","sws_DLG_150"));
		if (SNM_AutoVWndPosition(DT_LEFT, &m_btnAutoSave, NULL, _r, &x0, _r->top, h))
		{
			// type dropdown
			if (SNM_AutoVWndPosition(DT_LEFT, &m_cbType, NULL, _r, &x0, _r->top, h, 4))
			{
				// add/del bookmark buttons
				m_btnDel.SetEnabled(g_resType >= SNM_NUM_DEFAULT_SLOTS);
				if (SNM_AutoVWndPosition(DT_LEFT, &m_btnsAddDel, NULL, _r, &x0, _r->top, h))
				{
					if (g_resType>=SNM_NUM_DEFAULT_SLOTS && g_tiedProjects.Get(g_resType)->GetLength())
					{
						char buf[128] = "";
						_snprintfSafe(buf, sizeof(buf), __LOCALIZE_VERFMT("Files attached to %s","sws_DLG_150"), GetFileRelativePath(g_tiedProjects.Get(g_resType)->Get()));
						m_txtTiedPrj.SetText(buf);

						// plain text if current project == tied project, alpha otherwise
						if (ColorTheme* ct = SNM_GetColorTheme())
							m_txtTiedPrj.SetColors(LICE_RGBA_FROMNATIVE(ct->main_text, *g_curProjectFn && !_stricmp(g_tiedProjects.Get(g_resType)->Get(), g_curProjectFn) ? 255 : 127));
					}
					else
						m_txtTiedPrj.SetText("");

					if (SNM_AutoVWndPosition(DT_LEFT, &m_txtTiedPrj, NULL, _r, &x0, _r->top, h, 5))
						SNM_AddLogo(_bm, _r, x0, h);
				}
			}
		}
	}

	// 2nd row of controls (bottom)

	x0 = _r->left+SNM_GUI_X_MARGIN; h=SNM_GUI_BOT_H;
	int y0 = _r->bottom-h;

	// defines a new rect r that takes the filter edit box into account (contrary to _r)
	RECT r;
	GetWindowRect(GetDlgItem(m_hwnd, IDC_FILTER), &r);
	ScreenToClient(m_hwnd, (LPPOINT)&r);
	ScreenToClient(m_hwnd, ((LPPOINT)&r)+1);
	r.top = _r->top;
	r.bottom = _r->bottom;
	r.right = r.left;
	r.left = _r->left;

	// "dbl-click to"
	if (fl->IsDblClick())
	{
		if (!SNM_AutoVWndPosition(DT_LEFT, &m_txtDblClickType, NULL, &r, &x0, y0, h, 5))
			return;
		if (!SNM_AutoVWndPosition(DT_LEFT, &m_cbDblClickType, &m_txtDblClickType, &r, &x0, y0, h))
			return;

		switch (typeForUser)
		{
			// "to selected" (fx chain only)
			case SNM_SLOT_FXC:
				if (!SNM_AutoVWndPosition(DT_LEFT, &m_txtDblClickTo, NULL, &r, &x0, y0, h, 5))
					return;
				if (!SNM_AutoVWndPosition(DT_LEFT, &m_cbDblClickTo, &m_txtDblClickTo, &r, &x0, y0, h))
					return;
				break;
			// offset items & envs (tr template only)
			case SNM_SLOT_TR:
				if (int* offsPref = (int*)GetConfigVar("templateditcursor")) // >= REAPER v4.15
				{
					int dblClickPrefs = LOWORD(g_dblClickPrefs[g_resType]);
					bool showOffsOption = true;
					switch(dblClickPrefs)
					{
						case 1: // apply to sel tracks
							showOffsOption = false; // no offset option
							break;
						case 3: // paste template items
						case 4: // paste (replace) template items
							m_btnOffsetTrTemplate.SetTextLabel(__LOCALIZE("Offset template items by edit cursor","sws_DLG_150"));
							break;
						default: // other dbl-click prefs
							m_btnOffsetTrTemplate.SetTextLabel(__LOCALIZE("Offset template items/envs by edit cursor","sws_DLG_150"));
							break;
					}
					if (showOffsOption)
					{
						m_btnOffsetTrTemplate.SetCheckState(*offsPref);
						if (!SNM_AutoVWndPosition(DT_LEFT, &m_btnOffsetTrTemplate, NULL, &r, &x0, y0, h, 5))
							return;
					}
				}
				break;
		}
	}
}

bool ResourcesWnd::GetToolTipString(int _xpos, int _ypos, char* _bufOut, int _bufOutSz)
{
	if (WDL_VWnd* v = m_parentVwnd.VirtWndFromPoint(_xpos,_ypos,1))
	{
		int typeForUser = GetTypeForUser();
		switch (v->GetID())
		{
			case BTNID_AUTOFILL:
				return (_snprintfStrict(_bufOut, _bufOutSz, __LOCALIZE_VERFMT("Auto-fill %s slots (right-click for options)\nfrom %s","sws_DLG_150"), 
					g_SNM_ResSlots.Get(typeForUser)->GetDesc(), 
					*GetAutoFillDir() ? GetAutoFillDir() : __LOCALIZE("undefined","sws_DLG_150")) > 0);
			case BTNID_AUTOSAVE:
				if (g_SNM_ResSlots.Get(g_resType)->IsAutoSave())
				{
					switch (typeForUser)
					{
						case SNM_SLOT_FXC:
							return (_snprintfStrict(_bufOut, _bufOutSz, __LOCALIZE_VERFMT("%s (right-click for options)\nto %s","sws_DLG_150"),
								g_asFXChainPref == FXC_AUTOSAVE_PREF_TRACK ? __LOCALIZE("Auto-save FX chains for selected tracks","sws_DLG_150") :
								g_asFXChainPref == FXC_AUTOSAVE_PREF_ITEM ? __LOCALIZE("Auto-save FX chains for selected items","sws_DLG_150") :
								g_asFXChainPref == FXC_AUTOSAVE_PREF_INPUT_FX ? __LOCALIZE("Auto-save input FX chains for selected tracks","sws_DLG_150")
									: __LOCALIZE("Auto-save FX chain slots","sws_DLG_150"),
								*GetAutoSaveDir() ? GetAutoSaveDir() : __LOCALIZE("undefined","sws_DLG_150")) > 0);
						case SNM_SLOT_TR:
							return (_snprintfStrict(_bufOut, _bufOutSz, __LOCALIZE_VERFMT("Auto-save track templates%s%s for selected tracks (right-click for options)\nto %s","sws_DLG_150"),
								(g_asTrTmpltPref&1) ? __LOCALIZE(" w/ items","sws_DLG_150") : "",
								(g_asTrTmpltPref&2) ? __LOCALIZE(" w/ envs","sws_DLG_150") : "",
								*GetAutoSaveDir() ? GetAutoSaveDir() : __LOCALIZE("undefined","sws_DLG_150")) > 0);
						default:
							return (_snprintfStrict(_bufOut, _bufOutSz, __LOCALIZE_VERFMT("Auto-save %s slots (right-click for options)\nto %s","sws_DLG_150"), 
								g_SNM_ResSlots.Get(typeForUser)->GetDesc(), 
								*GetAutoSaveDir() ? GetAutoSaveDir() : __LOCALIZE("undefined","sws_DLG_150")) > 0);
					}
				}
				break;
			case CMBID_TYPE:
				return (lstrcpyn(_bufOut, __LOCALIZE("Bookmarks (right-click for options)","sws_DLG_150"), _bufOutSz) != NULL);
			case BTNID_ADD_BOOKMARK:
				return (_snprintfStrict(_bufOut, _bufOutSz, __LOCALIZE_VERFMT("New %s bookmark","sws_DLG_150"), g_SNM_ResSlots.Get(typeForUser)->GetDesc()) > 0);
			case BTNID_DEL_BOOKMARK:
				return (lstrcpyn(_bufOut, __LOCALIZE("Delete bookmark","sws_DLG_150"), _bufOutSz) != NULL);
			case TXTID_TIED_PRJ:
				return (g_tiedProjects.Get(g_resType)->GetLength() && 
					_snprintfStrict(_bufOut, _bufOutSz, __LOCALIZE_VERFMT("Bookmark files attached to:\n%s","sws_DLG_150"), g_tiedProjects.Get(g_resType)->Get()) > 0);
		}
	}
	return false;
}

void ResourcesWnd::ClearListSelection()
{
	SWS_ListView* lv = GetListView();
	HWND hList = lv ? lv->GetHWND() : NULL;
	if (hList) // can be called when the view is closed!
		ListView_SetItemState(hList, -1, 0, LVIS_SELECTED);
}

// select a range of slots (contiguous, when the list view is not sorted) 
// or a single slot (when _slot2 < 0)
void ResourcesWnd::SelectBySlot(int _slot1, int _slot2, bool _selectOnly)
{
	SWS_ListView* lv = GetListView();
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

		if (_selectOnly)
			ListView_SetItemState(hList, -1, 0, LVIS_SELECTED);

		int firstSel = -1;
		for (int i=0; i < lv->GetListItemCount(); i++)
		{
			ResourceItem* item = (ResourceItem*)lv->GetListItem(i);
			int slot = g_SNM_ResSlots.Get(g_resType)->Find(item);
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

// gets selected slots and returns the number of non empty slots among them
void ResourcesWnd::GetSelectedSlots(WDL_PtrList<ResourceItem>* _selSlots, WDL_PtrList<ResourceItem>* _selEmptySlots)
{
	if (m_pLists.GetSize())
	{
		int x=0;
		while (ResourceItem* pItem = (ResourceItem*)GetListView()->EnumSelected(&x))
		{
			if (_selEmptySlots && pItem->IsDefault())
				_selEmptySlots->Add(pItem);
			else
				_selSlots->Add(pItem);
		}
	}
}

void ResourcesWnd::AddSlot()
{
	if (ResourceList* fl = g_SNM_ResSlots.Get(g_resType))
	{
		int idx = fl->GetSize();
		if (fl->Add(new ResourceItem())) {
			Update();
			SelectBySlot(idx);
		}
	}
}

void ResourcesWnd::InsertAtSelectedSlot()
{
	ResourceList* fl = g_SNM_ResSlots.Get(g_resType);
	if (fl && fl->GetSize() && m_pLists.GetSize())
	{
		if (ResourceItem* item = (ResourceItem*)GetListView()->EnumSelected(NULL))
		{
			int slot = fl->Find(item);
			if (slot>=0 && fl->InsertSlot(slot) != NULL)
			{
				Update();
				SelectBySlot(slot);
				return; // <-- !!
			}
		}
	}
	AddSlot(); // empty list, no selection, etc.. => add
}


///////////////////////////////////////////////////////////////////////////////
// Auto-save, auto-fill
///////////////////////////////////////////////////////////////////////////////

bool CheckSetAutoDirectory(const char* _title, int _type, bool _autoSave)
{
	WDL_FastString* dir = _autoSave ? g_autoSaveDirs.Get(_type) : g_autoFillDirs.Get(_type);
	if (!FileOrDirExists(dir->Get()))
	{
		char buf[SNM_MAX_PATH] = "";
		_snprintfSafe(buf, sizeof(buf), __LOCALIZE_VERFMT("%s directory not found!\n%s%sDo you want to define one ?","sws_DLG_150"), _title, dir->Get(), dir->GetLength()?"\n":"");
		if (IDYES == MessageBox(g_resWndMgr.GetMsgHWND(), buf, __LOCALIZE("S&M - Warning","sws_DLG_150"), MB_YESNO)) {
			if (BrowseForDirectory(_autoSave ? __LOCALIZE("Set auto-save directory","sws_DLG_150") : __LOCALIZE("Set auto-fill directory","sws_DLG_150"), GetResourcePath(), buf, sizeof(buf))) {
				if (_autoSave) SetAutoSaveDir(buf, _type);
				else SetAutoFillDir(buf, _type);
			}
		}
		else
			return false;
		return FileOrDirExists(dir->Get()); // re-check (browse cancelled, etc..)
	}
	return true;
}

bool AutoSaveChunkSlot(const void* _obj, const char* _fn) {
	return SaveChunk(_fn, (WDL_FastString*)_obj, true);
}

// overwitre a slot -or- genenerate a new filename + add a slot
// _owSlots: slots to overwrite, if any
// note: just a helper func for AutoSaveTrackSlots(), AutoSaveMediaSlot(), etc..
bool AutoSaveSlot(int _type, const char* _dirPath, 
				  const char* _name, const char* _ext,
				  WDL_PtrList<ResourceItem>* _owSlots, int* _owIdx, 
				  bool (*SaveSlot)(const void*, const char*), const void* _obj) // see AutoSaveChunkSlot() example
{
	bool saved = false;
	char fn[SNM_MAX_PATH] = "";
	int todo = 1; // 0: nothing, 1: gen filename + add slot, 2: gen filename + replace slot

	if (*_owIdx < _owSlots->GetSize() && _owSlots->Get(*_owIdx))
	{
		// gets the filename to overwrite
		GetFullResourcePath(g_SNM_ResSlots.Get(_type)->GetResourceDir(), _owSlots->Get(*_owIdx)->m_shortPath.Get(), fn, sizeof(fn));
		if (!*fn)
		{
			todo=2;
		}
		else
		{
			todo=0;
			if (_stricmp(_ext, GetFileExtension(fn))) // corner case, ex: try to write .mid over .wav
			{
				if (char* p = strrchr(fn, '.'))
				{
					strcpy(p+1, _ext);
					char shortPath[SNM_MAX_PATH] = "";
					GetShortResourcePath(g_SNM_ResSlots.Get(_type)->GetResourceDir(), fn, shortPath, sizeof(shortPath));
					_owSlots->Get(*_owIdx)->m_shortPath.Set(shortPath);
				}
			}
			saved = (SaveSlot ? SaveSlot(_obj, fn) : SNM_CopyFile(fn, _name));
		}
		*_owIdx = *_owIdx + 1;
	}

	if (todo)
	{
		char fnNoExt[SNM_MAX_PATH] = "";
		GetFilenameNoExt(_name, fnNoExt, sizeof(fnNoExt));
		Filenamize(fnNoExt);
		if (GenerateFilename(_dirPath, fnNoExt, _ext, fn, sizeof(fn)) && (SaveSlot ? SaveSlot(_obj, fn) : SNM_CopyFile(fn, _name)))
		{
			saved = true;
			if (todo==1)
			{
				g_SNM_ResSlots.Get(_type)->AddSlot(fn);
			}
			else 
			{
				char shortPath[SNM_MAX_PATH] = "";
				GetShortResourcePath(g_SNM_ResSlots.Get(_type)->GetResourceDir(), fn, shortPath, sizeof(shortPath));
				_owSlots->Get(*_owIdx-1)->m_shortPath.Set(shortPath);
			}
		}
	}
	return saved;
}

// _promptOverwrite: false for auto-save *actions*, true for auto-save *button*
// _flags:
//    - for track templates: _flags&1 save template with items, _flags&2 save template with envs
//    - for fx chains: enum FXC_AUTOSAVE_PREF_INPUT_FX, FXC_AUTOSAVE_PREF_TRACK and FXC_AUTOSAVE_PREF_ITEM
//    - n/a otherwise
void AutoSave(int _type, bool _promptOverwrite, int _flags)
{
	ResourcesWnd* resWnd = g_resWndMgr.Get();
	WDL_PtrList<ResourceItem> owSlots; // slots to overwrite
	WDL_PtrList<ResourceItem> selFilledSlots;

	// overwrite non empty slots?
	int ow = IDNO;
	if (resWnd)
		resWnd->GetSelectedSlots(&selFilledSlots, &owSlots); // &owSlots: empty slots are always overwritten

	if (selFilledSlots.GetSize() && _promptOverwrite)
	{
		ow = MessageBox(g_resWndMgr.GetMsgHWND(),
				__LOCALIZE("Some selected slots are already filled, do you want to overwrite them?\nIf you select No, new slots/files will be added.","sws_DLG_150"),
				__LOCALIZE("S&M - Confirmation","sws_DLG_150"),
				MB_YESNOCANCEL);
	}

	if (ow == IDYES) {
		for (int i=0; i<selFilledSlots.GetSize(); i++)
			owSlots.Add(selFilledSlots.Get(i));
	}
	else if (ow == IDCANCEL)
		return;

	if (!CheckSetAutoDirectory(__LOCALIZE("Auto-save","sws_DLG_150"), _type, true))
		return;


	// the meat -------------------------------------------------------------->
	// untie all files, save, tie all: brutal but safe
	char prjPath[SNM_MAX_PATH]="";
	ReaProject* prj = EnumProjects(-1, prjPath, sizeof(prjPath));

	UntieAllResFileFromProject(prj, prjPath, _type);

	bool saved = false;
	int oldSz = g_SNM_ResSlots.Get(_type)->GetSize();
	switch(GetTypeForUser(_type))
	{
		case SNM_SLOT_FXC:
			switch (_flags)
			{
				case FXC_AUTOSAVE_PREF_INPUT_FX:
				case FXC_AUTOSAVE_PREF_TRACK:
					saved = AutoSaveTrackFXChainSlots(_type, GetAutoSaveDir(_type), (WDL_PtrList<void>*)&owSlots, g_asFXChainNamePref==1, _flags==FXC_AUTOSAVE_PREF_INPUT_FX);
					break;
				case FXC_AUTOSAVE_PREF_ITEM:
					saved = AutoSaveItemFXChainSlots(_type, GetAutoSaveDir(_type), (WDL_PtrList<void>*)&owSlots, g_asFXChainNamePref==1);
					break;
			}
			break;
		case SNM_SLOT_TR:
			saved = AutoSaveTrackSlots(_type, GetAutoSaveDir(_type), (WDL_PtrList<void>*)&owSlots, (_flags&1)?false:true, (_flags&2)?false:true);
			break;
		case SNM_SLOT_PRJ:
			saved = AutoSaveProjectSlot(_type, GetAutoSaveDir(_type), (WDL_PtrList<void>*)&owSlots, true);
			break;
		case SNM_SLOT_MEDIA:
			saved = AutoSaveMediaSlots(_type, GetAutoSaveDir(_type), (WDL_PtrList<void>*)&owSlots);
			break;
	}

	TieAllResFileToProject(prj, prjPath, _type);
	// the meat <--------------------------------------------------------------


	if (saved)
	{
		if (resWnd && g_resType==_type)
		{
			resWnd->Update();

			// sel new slots
			bool first = true;
			if (oldSz != g_SNM_ResSlots.Get(_type)->GetSize()) {
				resWnd->SelectBySlot(oldSz, g_SNM_ResSlots.Get(_type)->GetSize(), first);
				first = false;
			}
			// sel overwriten slots
			for (int i=0; i<owSlots.GetSize(); i++)
			{
				int slot = g_SNM_ResSlots.Get(_type)->Find(owSlots.Get(i));
				if (slot >= 0) {
					resWnd->SelectBySlot(slot, slot, first);
					first = false;
				}
			}
		}
	}
	else
	{
		char msg[SNM_MAX_PATH];
		_snprintfSafe(msg, sizeof(msg), __LOCALIZE_VERFMT("Auto-save failed!\n%s","sws_DLG_150"), AUTOSAVE_ERR_STR);
		MessageBox(g_resWndMgr.GetMsgHWND(), msg, __LOCALIZE("S&M - Error","sws_DLG_150"), MB_OK);
	}
}

// recursive from auto-fill path
void AutoFill(int _type)
{
	ResourceList* fl = g_SNM_ResSlots.Get(_type);
	if (!fl)
		return;

	if (!CheckSetAutoDirectory(__LOCALIZE("Auto-fill","sws_DLG_150"), _type, false))
		return;

	int startSlot = g_SNM_ResSlots.Get(_type)->GetSize();

	char fileFilter[2048] = ""; // filters need some room!
	fl->GetFileFilter(fileFilter, sizeof(fileFilter), false);

	WDL_PtrList_DeleteOnDestroy<WDL_String> files; 
	ScanFiles(&files, GetAutoFillDir(_type), fileFilter, true);
	if (int sz = files.GetSize())
		for (int i=0; i<sz; i++)
			if (fl->FindByPath(files.Get(i)->Get()) < 0) { // skip if already present
				TieResFileToProject(files.Get(i)->Get(), _type);
				fl->AddSlot(files.Get(i)->Get());
			}

	if (startSlot != fl->GetSize())
	{
		if (g_resType==_type)
			if (ResourcesWnd* w = g_resWndMgr.Get()) {
				w->Update();
				w->SelectBySlot(startSlot, g_SNM_ResSlots.Get(_type)->GetSize());
			}
	}
	else
	{
		const char* path = GetAutoFillDir(_type);
		char msg[SNM_MAX_PATH]="";
		if (path && *path) _snprintfSafe(msg, sizeof(msg), __LOCALIZE_VERFMT("No slot added from: %s\n%s","sws_DLG_150"), path, AUTOFILL_ERR_STR);
		else _snprintfSafe(msg, sizeof(msg), __LOCALIZE_VERFMT("No slot added!\n%s","sws_DLG_150"), AUTOFILL_ERR_STR);
		MessageBox(g_resWndMgr.GetMsgHWND(), msg, __LOCALIZE("S&M - Warning","sws_DLG_150"), MB_OK);
	}
}


///////////////////////////////////////////////////////////////////////////////
// Get, load, clear, delete slots/files
///////////////////////////////////////////////////////////////////////////////

WDL_FastString g_lastBrowsedFn;

// returns false on cancel/err
bool BrowseSlot(int _type, int _slot, bool _tieUntiePrj, char* _fn, int _fnSz, bool* _updatedList)
{
	bool ok = false;
	if (ResourceList* fl = g_SNM_ResSlots.Get(_type))
	{
		if (_slot>=0 && _slot<fl->GetSize())
		{
			char untiePath[SNM_MAX_PATH] = "";
			if (_tieUntiePrj && _type>=SNM_NUM_DEFAULT_SLOTS && g_tiedProjects.Get(_type)->GetLength()) // avoid useless job
				fl->GetFullPath(_slot, untiePath, sizeof(untiePath));

			char title[128]="", fileFilter[2048]=""; // room needed for file filters!
			_snprintfSafe(title, sizeof(title), __LOCALIZE_VERFMT("S&M - Load resource file in bookmark \"%s\"","sws_DLG_150"), fl->GetDesc());
			fl->GetFileFilter(fileFilter, sizeof(fileFilter));

			if (char* fn = BrowseForFiles(title, g_lastBrowsedFn.GetLength()?g_lastBrowsedFn.Get():GetAutoFillDir(_type), NULL, false, fileFilter)) // single file
			{
				// have to test extension (because of the "All files (*.*)" filter..)
				if (fl->IsValidFileExt(GetFileExtension(fn)))
				{
					if (_fn)
						lstrcpyn(_fn, fn, _fnSz);
					if (fl->SetFromFullPath(_slot, fn))
					{
						ok = true;
						if (_updatedList)
							*_updatedList = true;
						if (_tieUntiePrj) {
							UntieResFileFromProject(untiePath, _type);
							TieResFileToProject(fn, _type);
						}
					}
				}
				else
				{
					WDL_FastString msg;
					msg.SetFormatted(128, __LOCALIZE_VERFMT("The file extension \".%s\" is not supported in the bookmark \"%s\"","sws_DLG_150"), GetFileExtension(fn), fl->GetDesc());
					MessageBox(GetMainHwnd(), msg.Get(), __LOCALIZE("S&M - Error","sws_DLG_150"), MB_OK);
				}
				g_lastBrowsedFn.Set(fn);
				free(fn);
			}
		}
	}
	return ok;
}

// returns NULL if failed, otherwise it's up to the caller to free the returned string
// _slot: in/out param, prompt for slot if -1
WDL_FastString* GetOrPromptOrBrowseSlot(int _type, int* _slot)
{
	ResourceList* fl = g_SNM_ResSlots.Get(_type);
	if (!fl)
		return NULL;

	WDL_FastString* fnStr = NULL;

	// prompt for slot if needed
	if (*_slot == -1)
	{
		if (fl->GetSize())
		{
			char title[128]="";
			_snprintfSafe(title, sizeof(title), __LOCALIZE_VERFMT("S&M - Load resource file in bookmark \"%s\"","sws_DLG_150"), fl->GetDesc());
			*_slot = PromptForInteger(title, __LOCALIZE("Slot","sws_DLG_150"), 1, fl->GetSize()); // loops on err
		}
		else
		{
			WDL_FastString msg;
			msg.SetFormatted(128, NO_SLOT_ERR_STR, fl->GetDesc());
			MessageBox(GetMainHwnd(), msg.Get(), __LOCALIZE("S&M - Error","sws_DLG_150"), MB_OK);
		}
	}

	// user has cancelled or empty
	if (*_slot == -1)
		return NULL; 

	// adds the needed number of slots (macro friendly)
	bool uiUpdate = (*_slot >= fl->GetSize());
	while (*_slot >= fl->GetSize())
		fl->Add(new ResourceItem());

	// get filename or browse if the slot is empty (macro friendly)
	char fn[SNM_MAX_PATH]="";
	bool fnOk=false, listUpdate=false;
	if (fl->Get(*_slot)->IsDefault())
		fnOk = BrowseSlot(_type, *_slot, false, fn, sizeof(fn), &listUpdate);
	else if (fl->GetFullPath(*_slot, fn, sizeof(fn)))
		fnOk = FileOrDirExistsErrMsg(fn, *_slot < 0 || !fl->Get(*_slot)->IsDefault());

	if (fnOk)
	{
		fnStr = new WDL_FastString(fn);

		// in case a slot has been filled..
		if (listUpdate) {
			TieResFileToProject(fn, _type);
			uiUpdate = true;
		}
	}

	if (uiUpdate && g_resType==_type)
		if (ResourcesWnd* w = g_resWndMgr.Get())
			w->Update();

	if (!fnStr)
		*_slot = -1;
	return fnStr;
}

void ClearSlot(int _type, int _slot)
{
	if (ResourceList* fl = g_SNM_ResSlots.Get(_type))
	{
		char untiePath[SNM_MAX_PATH] = "";
		if (_type>=SNM_NUM_DEFAULT_SLOTS && g_tiedProjects.Get(_type)->GetLength()) // avoid useless job
			fl->GetFullPath(_slot, untiePath, sizeof(untiePath));

		if (fl->ClearSlot(_slot))
		{
			UntieResFileFromProject(untiePath, _type);
			if (g_resType==_type)
				if (ResourcesWnd* w = g_resWndMgr.Get())
					w->Update();
		}
	}
}

// _mode&1 = delete sel. slots
// _mode&2 = clear sel. slots
// _mode&4 = delete files
// _mode&8 = delete all slots (exclusive with _mode&1 and _mode&2)
void ClearDeleteSlotsFiles(int _type, int _mode)
{
	ResourceList* fl = g_SNM_ResSlots.Get(_type);
	if (!fl)
		return;

	ResourcesWnd* resWnd = g_resWndMgr.Get();
	int oldSz = fl->GetSize();

	WDL_PtrList<int> slots;
	while (slots.GetSize() != fl->GetSize())
		slots.Add(_mode&8 ? &g_i1 : &g_i0);

	if (resWnd && (_mode&1 || _mode&2))
	{
		int x=0;
		if (SWS_ListView* lv = resWnd->GetListView())
		{
			while(ResourceItem* item = (ResourceItem*)lv->EnumSelected(&x))
			{
				int slot = fl->Find(item);
				slots.Delete(slot);
				slots.Insert(slot, &g_i1);
			}
		}
	}

	char fullPath[SNM_MAX_PATH] = "";
	WDL_PtrList_DeleteOnDestroy<ResourceItem> delItems; // DeleteOnDestroy: keep pointers until updated
	for (int slot=slots.GetSize()-1; slot >=0 ; slot--)
	{
		if (*slots.Get(slot)) // avoids new Find(item) calls
		{
			if (ResourceItem* item = fl->Get(slot))
			{
				if (_mode&4 || (_type>=SNM_NUM_DEFAULT_SLOTS && g_tiedProjects.Get(_type)->GetLength())) // avoid useless job
					fl->GetFullPath(slot, fullPath, sizeof(fullPath));

				if (_mode&4)
					SNM_DeleteFile(fullPath, true);

				if (_mode&1 || _mode&8) 
				{
					slots.Delete(slot, false); // keep the sel list "in sync"
					fl->Delete(slot, false); // remove slot, pointer not deleted yet
					delItems.Add(item); // for later pointer deletion..
				}
				else
					fl->ClearSlot(slot);

				UntieResFileFromProject(fullPath, _type, (_mode&4)!=4);
			}
		}
	}

	if (resWnd && g_resType==_type)
		resWnd->Update();

} // + delItems auto clean-up !


///////////////////////////////////////////////////////////////////////////////
// Ini file helpers
///////////////////////////////////////////////////////////////////////////////

void FlushCustomTypesIniFile()
{
	char iniSec[64]="";
	for (int i=SNM_NUM_DEFAULT_SLOTS; i<g_SNM_ResSlots.GetSize(); i++)
	{
		if (g_SNM_ResSlots.Get(i))
		{
			GetIniSectionName(i, iniSec, sizeof(iniSec));
			WritePrivateProfileStruct(iniSec, NULL, NULL, 0, g_SNM_IniFn.Get()); // flush section
			WritePrivateProfileString(RES_INI_SEC, iniSec, NULL, g_SNM_IniFn.Get());
			WDL_FastString str;
			str.SetFormatted(64, "AutoFillDir%s", iniSec);
			WritePrivateProfileString(RES_INI_SEC, str.Get(), NULL, g_SNM_IniFn.Get());
			str.SetFormatted(64, "AutoSaveDir%s", iniSec);
			WritePrivateProfileString(RES_INI_SEC, str.Get(), NULL, g_SNM_IniFn.Get());
			str.SetFormatted(64, "SyncAutoDirs%s", iniSec);
			WritePrivateProfileString(RES_INI_SEC, str.Get(), NULL, g_SNM_IniFn.Get());
			str.SetFormatted(64, "TiedActions%s", iniSec);
			WritePrivateProfileString(RES_INI_SEC, str.Get(), NULL, g_SNM_IniFn.Get());
			str.SetFormatted(64, "TiedProject%s", iniSec);
			WritePrivateProfileString(RES_INI_SEC, str.Get(), NULL, g_SNM_IniFn.Get());
			str.SetFormatted(64, "DblClick%s", iniSec);
			WritePrivateProfileString(RES_INI_SEC, str.Get(), NULL, g_SNM_IniFn.Get());
		}
	}
}

void ReadSlotIniFile(const char* _key, int _slot, char* _path, int _pathSize, char* _desc, int _descSize)
{
	char buf[32];
	if (_snprintfStrict(buf, sizeof(buf), "Slot%d", _slot+1) > 0)
		GetPrivateProfileString(_key, buf, "", _path, _pathSize, g_SNM_IniFn.Get());
	if (_snprintfStrict(buf, sizeof(buf), "Desc%d", _slot+1) > 0)
		GetPrivateProfileString(_key, buf, "", _desc, _descSize, g_SNM_IniFn.Get());
}

// adds bookmarks and custom slot types from the S&M.ini file
void AddCustomTypesFromIniFile()
{
	int i=0;
	char buf[SNM_MAX_PATH]=""; // can be used with paths..
	WDL_String iniKeyStr("CustomSlotType1");
	GetPrivateProfileString(RES_INI_SEC, iniKeyStr.Get(), "", buf, sizeof(buf), g_SNM_IniFn.Get());
	while (*buf && i<SNM_MAX_SLOT_TYPES)
	{
		AddCustomBookmark(buf);
		iniKeyStr.SetFormatted(32, "CustomSlotType%d", (++i)+1);
		GetPrivateProfileString(RES_INI_SEC, iniKeyStr.Get(), "", buf, sizeof(buf), g_SNM_IniFn.Get());
	}
}


///////////////////////////////////////////////////////////////////////////////
// Bookmarks
///////////////////////////////////////////////////////////////////////////////

// add a custom bookmark, 
// _definition: resource_directory_name,description,file_extensions
// examples:
// - "Configurations,ReaConfig,ReaperConfigZip"
// - "Docs,Document,txt,rtf,pdf"
// - "Misc,Any file,*
int AddCustomBookmark(char* _definition)
{
	if (_definition && *_definition && g_SNM_ResSlots.GetSize()<SNM_MAX_SLOT_TYPES)
	{
		// basic check (_definition can be a user-defined string)
		int i=-1, commas=0;
		while (_definition[++i])
			if (_definition[i]==',')
				commas++;
		if (commas>=2)
		{
			WDL_String tokenStrs[2];
			if (char* tok = strtok(_definition, ","))
			{
				tokenStrs[0].Set(tok);
				if (tok = strtok(NULL, ","))
				{
					tokenStrs[1].Set(tok);

					ResourceList* fl = new ResourceList(
						tokenStrs[0].Get(), 
						tokenStrs[1].Get(), 
						(const char*)(tok+strlen(tok)+1), // add extensions as defined
						0);
					g_SNM_ResSlots.Add(fl);

					// known slot type?
					int newType = g_SNM_ResSlots.GetSize()-1;
					int typeForUser = GetTypeForUser(newType);

					// (very) default inits
					char path[SNM_MAX_PATH]="";
					if (_snprintfStrict(path, sizeof(path), "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, tokenStrs[0].Get()) <= 0)
						*path = '\0';
					g_autoSaveDirs.Add(new WDL_FastString(path));
					g_autoFillDirs.Add(new WDL_FastString(path));
					g_tiedProjects.Add(new WDL_FastString);
					g_syncAutoDirPrefs[newType] = true;
					g_dblClickPrefs[newType] = 0;
					if (newType != typeForUser) // this is a known type => enable some features
						fl->SetFlags(g_SNM_ResSlots.Get(typeForUser)->GetFlags());
					else // enable dbl-click for custom bookmarks
						fl->SetFlags(SNM_RES_MASK_DBLCLIK);
					return newType;
				}
			}
		}
	}
	return -1;
}

// new typed bookmark, new custom bookmark or copy bookmark
// _type: bookmark type to create, -1=custom bookmark
void NewBookmark(int _type, bool _copyCurrent)
{
	if (g_SNM_ResSlots.GetSize() < SNM_MAX_SLOT_TYPES)
	{
		char input[128] = "";
		if (_type>=0)
			_snprintfSafe(input, sizeof(input), __LOCALIZE_VERFMT("My %s slots","sws_DLG_150"), g_SNM_ResSlots.Get(GetTypeForUser(_type))->GetDesc());

		if (int saveOption = PromptUserForString(
				g_resWndMgr.GetMsgHWND(),
				_copyCurrent ? __LOCALIZE("S&M - Copy bookmark","sws_DLG_150") : _type<0 ? __LOCALIZE("S&M - Add custom bookmark","sws_DLG_150") : __LOCALIZE("S&M - Add bookmark","sws_DLG_150"),
				input, sizeof(input),
				true,
				_copyCurrent ? NULL : __LOCALIZE("Attach bookmark to this project","sws_DLG_150")))
		{
			// 1) add a new slot list + new items to auto-fill, auto-save and attached project lists
			int newType = g_SNM_ResSlots.GetSize();

			// custom bookmark?
			if (_type<0)
			{
				if (AddCustomBookmark(input) < 0)
				{
					WDL_FastString msg(__LOCALIZE("Invalid bookmark definition!","sws_DLG_150"));
					msg.Append("\n");
					msg.Append(__LOCALIZE("Expected format: resource_directory_name,description,file_extensions","sws_DLG_150"));
					msg.Append("\n");
					msg.Append(__LOCALIZE("Example: Docs,Document,txt,rtf,pdf","sws_DLG_150"));
					MessageBox(g_resWndMgr.GetMsgHWND(), msg.Get(), __LOCALIZE("S&M - Error","sws_DLG_150"), MB_OK);
					return;
				}
			}
			// standard bookmark?
			else 
			{
				if (!*input || strchr(input, ',')) // check for comma as it is used as a separator..
				{
					WDL_FastString msg(__LOCALIZE("Invalid bookmark name!","sws_DLG_150"));
					msg.Append("\n");
					msg.Append(__LOCALIZE("Note: bookmark names cannot contain the character ,","sws_DLG_150"));
					MessageBox(g_resWndMgr.GetMsgHWND(), msg.Get(), __LOCALIZE("S&M - Error","sws_DLG_150"), MB_OK);
					return;
				}

				g_SNM_ResSlots.Add(new ResourceList(
					g_SNM_ResSlots.Get(_type)->GetResourceDir(), 
					input, 
					g_SNM_ResSlots.Get(_type)->GetFileExtStr(), 
					g_SNM_ResSlots.Get(_type)->GetFlags()));

				g_autoSaveDirs.Add(new WDL_FastString);
				g_autoFillDirs.Add(new WDL_FastString);
				g_tiedProjects.Add(new WDL_FastString);
			}

			// 2) update those new list items according to user inputs
			WDL_FastString *saveDir, *fillDir, *attachedPrj;
			saveDir = g_autoSaveDirs.Get(newType);
			fillDir = g_autoFillDirs.Get(newType);
			attachedPrj = g_tiedProjects.Get(newType);

			// attach files & auto-fill/save paths to the current project
			if (saveOption==2)
			{
				char curPrjFn[SNM_MAX_PATH]="";
				EnumProjects(-1, curPrjFn, sizeof(curPrjFn));

				WDL_FastString curPrjPath;
				if (const char* p = strrchr(curPrjFn, PATH_SLASH_CHAR))
					curPrjPath.Set(curPrjFn, (int)(p-curPrjFn));

				saveDir->Set(curPrjPath.Get());
				fillDir->Set(curPrjPath.Get());
				g_syncAutoDirPrefs[newType] = true;
				attachedPrj->Set(curPrjFn);
				g_dblClickPrefs[newType] = 0;
			}
			// copy
			else if (_copyCurrent && _type>=0) // exclusive param values
			{
				saveDir->Set(GetAutoSaveDir(_type));
				fillDir->Set(GetAutoFillDir(_type));
				g_syncAutoDirPrefs[newType] = g_syncAutoDirPrefs[_type];
				attachedPrj->Set(g_tiedProjects.Get(_type)->Get());
				g_dblClickPrefs[newType] = g_dblClickPrefs[_type];
			}
			// default init
			else if (_type>=0)
			{
				char path[SNM_MAX_PATH]="";
				if (_snprintfStrict(path, sizeof(path), "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, g_SNM_ResSlots.Get(_type)->GetResourceDir()) > 0) {
					if (!FileOrDirExists(path))
						CreateDirectory(path, NULL);
				}
				else
					*path = '\0';

				saveDir->Set(path);
				fillDir->Set(path);
				g_syncAutoDirPrefs[newType] = true;
				attachedPrj->Set("");
				g_dblClickPrefs[newType] = 0;
			}
			// custom bookmark (_type<0), not attached to a project
			else
			{
				// use the default init made in AddCustomBookmark()
			}

			// 3) set other common properties
			int typeForUser = GetTypeForUser(newType);
			if (newType != typeForUser) // this is a known type => enable related features
				g_SNM_ResSlots.Get(newType)->SetFlags(g_SNM_ResSlots.Get(typeForUser)->GetFlags());
			else // enable dbl-click for custom bookmarks
				g_SNM_ResSlots.Get(newType)->SetFlags(SNM_RES_MASK_DBLCLIK);

			// 4) copy slots if needed
			if (_copyCurrent)
			{
				if (ResourceList* flCur = g_SNM_ResSlots.Get(g_resType))
					for (int i=0; i<flCur->GetSize(); i++)
						if (ResourceItem* slotItem = flCur->Get(i))
							g_SNM_ResSlots.Get(newType)->AddSlot(slotItem->m_shortPath.Get(), slotItem->m_comment.Get());
			}

			if (ResourcesWnd* w = g_resWndMgr.Get()) {
				w->FillTypeCombo();
				w->SetType(newType); // incl. UI update
			}
		}
	}
	else
		MessageBox(g_resWndMgr.GetMsgHWND(), __LOCALIZE("Too many resource types!","sws_DLG_150"), __LOCALIZE("S&M - Error","sws_DLG_150"), MB_OK);
}

void DeleteBookmark(int _bookmarkType)
{
	if (_bookmarkType >= SNM_NUM_DEFAULT_SLOTS)
	{
		int reply = IDNO;
		if (g_SNM_ResSlots.Get(_bookmarkType)->GetNonEmptySize()) // do not ask if empty
		{
			char title[128] = "";
			_snprintfSafe(title, sizeof(title), __LOCALIZE_VERFMT("S&M - Delete bookmark \"%s\"","sws_DLG_150"), g_SNM_ResSlots.Get(_bookmarkType)->GetDesc());
			reply = MessageBox(g_resWndMgr.GetMsgHWND(), __LOCALIZE("Delete all resource files too?","sws_DLG_150"), title, MB_YESNOCANCEL);
		}
		if (reply != IDCANCEL)
		{
			// cleanup ini file (types may not be contiguous anymore..)
			FlushCustomTypesIniFile();
			ClearDeleteSlotsFiles(_bookmarkType, 8|(reply==IDYES?4:0));

			// remove current slot type
			int oldType = _bookmarkType;
			int oldTypeForUser = GetTypeForUser(oldType);
			g_dblClickPrefs[oldType] = 0;
			if (g_SNM_TiedSlotActions[oldTypeForUser] == oldType)
				g_SNM_TiedSlotActions[oldTypeForUser] = oldTypeForUser;
			g_autoSaveDirs.Delete(oldType, true);
			g_autoFillDirs.Delete(oldType, true);
			g_tiedProjects.Delete(oldType, true);
			g_syncAutoDirPrefs[oldType] = true;
			g_SNM_ResSlots.Delete(oldType, true);

			if (ResourcesWnd* w = g_resWndMgr.Get()) {
				w->FillTypeCombo();
				w->SetType(oldType-1); // + GUI update
			}
		}
	}
}

void RenameBookmark(int _bookmarkType)
{
	if (_bookmarkType >= SNM_NUM_DEFAULT_SLOTS)
	{
		char newName[64] = "";
		lstrcpyn(newName, g_SNM_ResSlots.Get(_bookmarkType)->GetDesc(), sizeof(newName));
		if (PromptUserForString(g_resWndMgr.GetMsgHWND(), __LOCALIZE("S&M - Rename bookmark","sws_DLG_150"), newName, sizeof(newName), true) && *newName)
		{
			g_SNM_ResSlots.Get(_bookmarkType)->SetDesc(newName);
			if (ResourcesWnd* w = g_resWndMgr.Get()) {
				w->FillTypeCombo();
				w->Update();
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////////////

// just to be notified of "save as"
// note: project load/switch are notified via ResourcesTrackListChange()
static void SaveExtensionConfig(ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	char path[SNM_MAX_PATH]="";
	if (!isUndo && // saving?
		IsActiveProjectInLoadSave(path, sizeof(path)) && // saving current project?
		_stricmp(path, g_curProjectFn)) // saving current project as new_name.rpp?
	{
//JFB! TODO? auto-export if tied? (+ msg?)
#ifdef _SNM_NO_ASYNC_UPDT
		ResourcesAttachJob job;
		job.Perform();
#else
		SNM_AddOrReplaceScheduledJob(new ResourcesAttachJob());
#endif
	}
}

static project_config_extension_t s_projectconfig = {
	NULL, SaveExtensionConfig, NULL, NULL
};


///////////////////////////////////////////////////////////////////////////////

// detect project switches (incl. save as new_name.RPP) and detach/re-attach everything
void ResourcesAttachJob::Perform()
{
	char newProjectFn[SNM_MAX_PATH]="";
	if (ReaProject* newProject = EnumProjects(-1, newProjectFn, sizeof(newProjectFn)))
	{
		// do the job only if needed (track project switches)
		if (_stricmp(g_curProjectFn, newProjectFn))
		{
			// detach all files of all bookmarks from previous active project, if any
			UntieAllResFileFromProject(g_curProject, g_curProjectFn);

			// attach files to new active project
			TieAllResFileToProject(newProject, newProjectFn);

			// set new current project
			g_curProject = newProject;
			lstrcpyn(g_curProjectFn, newProjectFn, sizeof(g_curProjectFn));

			// UI update (tied project display + alpha/plain text)
			if (g_resType>=SNM_NUM_DEFAULT_SLOTS)
				if (ResourcesWnd* w = g_resWndMgr.Get())
					w->GetParentVWnd()->RequestRedraw(NULL);
		}
	}
}


///////////////////////////////////////////////////////////////////////////////

// this is our only notification of active project tab change, so attach/detach everything
// ScheduledJob used because of multi-notifs
void ResourcesTrackListChange()
{
/*no! replaced with code below fixes a potential hang when closing project tabs
	SNM_AddOrReplaceScheduledJob(new ResourcesAttachJob());
*/
	ResourcesAttachJob job;
	job.Perform();
}

void ResourcesUpdate() {
	if (ResourcesWnd* w = g_resWndMgr.Get())
		w->Update();
}
void ResourcesSelectBySlot(int _slot1, int _slot2, bool _selectOnly) {
	if (ResourcesWnd* w = g_resWndMgr.Get())
		w->SelectBySlot(_slot1, _slot2, _selectOnly);
}


///////////////////////////////////////////////////////////////////////////////

int ResourcesInit()
{
	// localization
	g_filter.Set(FILTER_DEFAULT_STR);

	g_SNM_ResSlots.Empty(true);

	// cross-platform resource definition (path separators are replaced if needed)
	g_SNM_ResSlots.Add(new ResourceList("FXChains",			__LOCALIZE("FX chain","sws_DLG_150"),		"RfxChain",							SNM_RES_MASK_AUTOFILL|SNM_RES_MASK_AUTOSAVE|SNM_RES_MASK_DBLCLIK|SNM_RES_MASK_TEXT));
	g_SNM_ResSlots.Add(new ResourceList("TrackTemplates",	__LOCALIZE("track template","sws_DLG_150"),	"RTrackTemplate",					SNM_RES_MASK_AUTOFILL|SNM_RES_MASK_AUTOSAVE|SNM_RES_MASK_DBLCLIK|SNM_RES_MASK_TEXT));
	// RPP* is a keyword (get supported project extensions at runtime)
	g_SNM_ResSlots.Add(new ResourceList("ProjectTemplates",	__LOCALIZE("project","sws_DLG_150"),		"RPP*",								SNM_RES_MASK_AUTOFILL|SNM_RES_MASK_AUTOSAVE|SNM_RES_MASK_DBLCLIK|SNM_RES_MASK_TEXT));
	// WAV* is a keyword (get supported media extensions at runtime)
	g_SNM_ResSlots.Add(new ResourceList("MediaFiles",		__LOCALIZE("media file","sws_DLG_150"),		"WAV*",								SNM_RES_MASK_AUTOFILL|SNM_RES_MASK_AUTOSAVE|SNM_RES_MASK_DBLCLIK));
	// PNG* is a keyword (but we can't get image extensions at runtime, keyword replaced with SNM_REAPER_IMG_EXTS) 
	g_SNM_ResSlots.Add(new ResourceList("Data/track_icons",	__LOCALIZE("image","sws_DLG_150"),			"PNG*",								SNM_RES_MASK_AUTOFILL|SNM_RES_MASK_DBLCLIK));
	g_SNM_ResSlots.Add(new ResourceList("ColorThemes",		__LOCALIZE("theme","sws_DLG_150"),			"ReaperthemeZip,ReaperTheme",		SNM_RES_MASK_AUTOFILL|SNM_RES_MASK_DBLCLIK));

	// -> add new resource types here + related enum on top of the .h

	// add saved bookmarks
	AddCustomTypesFromIniFile();

	// load general prefs
	g_resType = BOUNDED((int)GetPrivateProfileInt(
		RES_INI_SEC, "Type", SNM_SLOT_FXC, g_SNM_IniFn.Get()), SNM_SLOT_FXC, g_SNM_ResSlots.GetSize()-1); // bounded for safety (some custom slot types may have been removed..)

	g_filterPref = GetPrivateProfileInt(RES_INI_SEC, "Filter", 1, g_SNM_IniFn.Get());
	g_asFXChainPref = GetPrivateProfileInt(RES_INI_SEC, "AutoSaveFXChain", FXC_AUTOSAVE_PREF_TRACK, g_SNM_IniFn.Get());
	g_asFXChainNamePref = GetPrivateProfileInt(RES_INI_SEC, "AutoSaveFXChainName", 0, g_SNM_IniFn.Get());
	g_asTrTmpltPref = GetPrivateProfileInt(RES_INI_SEC, "AutoSaveTrTemplate", 3, g_SNM_IniFn.Get());
	g_SNM_PrjLoaderStartPref = GetPrivateProfileInt(RES_INI_SEC, "ProjectLoaderStartSlot", -1, g_SNM_IniFn.Get());
	g_SNM_PrjLoaderEndPref = GetPrivateProfileInt(RES_INI_SEC, "ProjectLoaderEndSlot", -1, g_SNM_IniFn.Get());

	// auto-save, auto-fill directories, etc..
	g_autoSaveDirs.Empty(true);
	g_autoFillDirs.Empty(true);
	g_tiedProjects.Empty(true);
	memset(g_dblClickPrefs, 0, SNM_MAX_SLOT_TYPES*sizeof(int));
/*
	for (int i=0; i < SNM_NUM_DEFAULT_SLOTS; i++)
		g_SNM_TiedSlotActions[i]=i;
*/

	char defaultPath[SNM_MAX_PATH]="", path[SNM_MAX_PATH]="", iniSec[64]="", iniKey[64]="";
	for (int i=0; i < g_SNM_ResSlots.GetSize(); i++)
	{
		GetIniSectionName(i, iniSec, sizeof(iniSec));
		if (_snprintfStrict(defaultPath, sizeof(defaultPath), "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, g_SNM_ResSlots.Get(i)->GetResourceDir()) < 0)
			*defaultPath = '\0';

		// g_autoFillDirs, g_autoSaveDirs and g_tiedProjects must always be filled
		// (even if auto-save is grayed for the user, etc..)
		WDL_FastString *fillPath, *savePath, *tiedPrj;

		savePath = new WDL_FastString(defaultPath);
		if (_snprintfStrict(iniKey, sizeof(iniKey), "AutoSaveDir%s", iniSec) > 0) {
			GetPrivateProfileString(RES_INI_SEC, iniKey, defaultPath, path, sizeof(path), g_SNM_IniFn.Get());
			savePath->Set(path);
		}
		g_autoSaveDirs.Add(savePath);

		fillPath = new WDL_FastString(defaultPath);
		if (_snprintfStrict(iniKey, sizeof(iniKey), "AutoFillDir%s", iniSec) > 0) {
			GetPrivateProfileString(RES_INI_SEC, iniKey, defaultPath, path, sizeof(path), g_SNM_IniFn.Get());
			fillPath->Set(path);
		}
		g_autoFillDirs.Add(fillPath);

		g_syncAutoDirPrefs[i] = true;
		if (_snprintfStrict(iniKey, sizeof(iniKey), "SyncAutoDirs%s", iniSec) > 0)
			g_syncAutoDirPrefs[i] = (GetPrivateProfileInt(RES_INI_SEC, iniKey, 1, g_SNM_IniFn.Get()) == 1);
		if (g_syncAutoDirPrefs[i]) // consistency check (e.g. after sws upgrade)
			g_syncAutoDirPrefs[i] = (strcmp(savePath->Get(), fillPath->Get()) == 0);

		g_dblClickPrefs[i] = 0;
		if (g_SNM_ResSlots.Get(i)->IsDblClick() && _snprintfStrict(iniKey, sizeof(iniKey), "DblClick%s", iniSec) > 0)
			g_dblClickPrefs[i] = GetPrivateProfileInt(RES_INI_SEC, iniKey, 0, g_SNM_IniFn.Get());

		tiedPrj = new WDL_FastString;
		g_tiedProjects.Add(tiedPrj);

		// default type? (fx chains, track templates, etc...)
		if (i < SNM_NUM_DEFAULT_SLOTS)
		{
			// load tied actions
			g_SNM_TiedSlotActions[i] = i;
			if (_snprintfStrict(iniKey, sizeof(iniKey), "TiedActions%s", iniSec) > 0)
				g_SNM_TiedSlotActions[i] = GetPrivateProfileInt(RES_INI_SEC, iniKey, i, g_SNM_IniFn.Get());
		}
		// bookmark, custom type?
		else
		{
			// load tied project
			if (_snprintfStrict(iniKey, sizeof(iniKey), "TiedProject%s", iniSec) > 0) {
				GetPrivateProfileString(RES_INI_SEC, iniKey, "", path, sizeof(path), g_SNM_IniFn.Get());
				tiedPrj->Set(path);
			}
		}
	}

	// read slots from ini file
	char desc[128]="", maxSlotCount[16]="";
	for (int i=0; i < g_SNM_ResSlots.GetSize(); i++)
	{
		if (ResourceList* list = g_SNM_ResSlots.Get(i))
		{
			GetIniSectionName(i, iniSec, sizeof(iniSec));
			
			//JFB TODO? would be faster to read section in one go..
			GetPrivateProfileString(iniSec, "Max_slot", "0", maxSlotCount, sizeof(maxSlotCount), g_SNM_IniFn.Get()); 
			list->EmptySafe(true);
			int cnt = atoi(maxSlotCount);
			for (int j=0; j<cnt; j++) {
				ReadSlotIniFile(iniSec, j, path, sizeof(path), desc, sizeof(desc));
				list->Add(new ResourceItem(path, desc));
			}
		}
	}

	// instanciate the window if needed, can be NULL
	g_resWndMgr.Init();

	if (!plugin_register("projectconfig", &s_projectconfig))
		return 0;

	return 1;
}

void ResourcesExit()
{
	WDL_FastString iniStr, escapedStr;
	WDL_PtrList_DeleteOnDestroy<WDL_FastString> iniSections;
	GetIniSectionNames(&iniSections);


	// *** save the main ini section

	iniStr.Set("");

	// save custom slot type definitions
	for (int i=0; i < g_SNM_ResSlots.GetSize(); i++)
	{
		if (i >= SNM_NUM_DEFAULT_SLOTS)
		{
			WDL_FastString str;
			str.SetFormatted(
				SNM_MAX_PATH,
				"%s,%s,%s",
				g_SNM_ResSlots.Get(i)->GetResourceDir(),
				g_SNM_ResSlots.Get(i)->GetDesc(),
				g_SNM_ResSlots.Get(i)->GetFileExtStr());
			makeEscapedConfigString(str.Get(), &escapedStr);
			iniStr.AppendFormatted(SNM_MAX_PATH, "%s=%s\n", iniSections.Get(i)->Get(), escapedStr.Get());
		}
	}
	iniStr.AppendFormatted(256, "Type=%d\n", g_resType);
	iniStr.AppendFormatted(256, "Filter=%d\n", g_filterPref);

	// save slot type prefs
	for (int i=0; i < g_SNM_ResSlots.GetSize(); i++)
	{
		// save auto-fill & auto-save paths
		if (g_autoFillDirs.Get(i)->GetLength()) {
			makeEscapedConfigString(g_autoFillDirs.Get(i)->Get(), &escapedStr);
			iniStr.AppendFormatted(SNM_MAX_PATH, "AutoFillDir%s=%s\n", iniSections.Get(i)->Get(), escapedStr.Get());
		}
		if (g_autoSaveDirs.Get(i)->GetLength() && g_SNM_ResSlots.Get(i)->IsAutoSave()) {
			makeEscapedConfigString(g_autoSaveDirs.Get(i)->Get(), &escapedStr);
			iniStr.AppendFormatted(SNM_MAX_PATH, "AutoSaveDir%s=%s\n", iniSections.Get(i)->Get(), escapedStr.Get());
		}
		iniStr.AppendFormatted(256, "SyncAutoDirs%s=%d\n", iniSections.Get(i)->Get(), g_syncAutoDirPrefs[i]);

		// default type? (fx chains, track templates, etc...)
		if (i < SNM_NUM_DEFAULT_SLOTS)
		{
			// save tied slot actions
			iniStr.AppendFormatted(256, "TiedActions%s=%d\n", iniSections.Get(i)->Get(), g_SNM_TiedSlotActions[i]);
		}
		// bookmark, custom type?
		else
		{
			// save tied project
			if (g_tiedProjects.Get(i)->GetLength()) {
				makeEscapedConfigString(g_tiedProjects.Get(i)->Get(), &escapedStr);
				iniStr.AppendFormatted(SNM_MAX_PATH, "TiedProject%s=%s\n", iniSections.Get(i)->Get(), escapedStr.Get());
			}
		}

		// dbl-click pref
		if (g_SNM_ResSlots.Get(i)->IsDblClick())
			iniStr.AppendFormatted(256, "DblClick%s=%d\n", iniSections.Get(i)->Get(), g_dblClickPrefs[i]);

		// specific options (saved here for the ini file ordering..)
		switch (i) {
			case SNM_SLOT_FXC:
				iniStr.AppendFormatted(256, "AutoSaveFXChain=%d\n", g_asFXChainPref);
				iniStr.AppendFormatted(256, "AutoSaveFXChainName=%d\n", g_asFXChainNamePref);
				break;
			case SNM_SLOT_TR:
				iniStr.AppendFormatted(256, "AutoSaveTrTemplate=%d\n", g_asTrTmpltPref);
				break;
			case SNM_SLOT_PRJ:
				iniStr.AppendFormatted(256, "ProjectLoaderStartSlot=%d\n", g_SNM_PrjLoaderStartPref);
				iniStr.AppendFormatted(256, "ProjectLoaderEndSlot=%d\n", g_SNM_PrjLoaderEndPref);
				break;
		}
	}
	SaveIniSection(RES_INI_SEC, &iniStr, g_SNM_IniFn.Get());


	// *** save slots ini sections

	for (int i=0; i < g_SNM_ResSlots.GetSize(); i++)
	{
		iniStr.SetFormatted(256, "Max_slot=%d\n", g_SNM_ResSlots.Get(i)->GetSize());
		for (int j=0; j < g_SNM_ResSlots.Get(i)->GetSize(); j++) {
			if (ResourceItem* item = g_SNM_ResSlots.Get(i)->Get(j)) {
				if (item->m_shortPath.GetLength()) {
					makeEscapedConfigString(item->m_shortPath.Get(), &escapedStr);
					iniStr.AppendFormatted(SNM_MAX_PATH, "Slot%d=%s\n", j+1, escapedStr.Get());
				}
				if (item->m_comment.GetLength()) {
					makeEscapedConfigString(item->m_comment.Get(), &escapedStr);
					iniStr.AppendFormatted(256, "Desc%d=%s\n", j+1, escapedStr.Get());
				}
			}
		}
		// write things in one go (avoid to slow down REAPER shutdown)
		SaveIniSection(iniSections.Get(i)->Get(), &iniStr, g_SNM_IniFn.Get());
	}

	g_resWndMgr.Delete();
}


///////////////////////////////////////////////////////////////////////////////
// Actions
///////////////////////////////////////////////////////////////////////////////

void OpenResources(COMMAND_T* _ct)
{
	if (ResourcesWnd* w = g_resWndMgr.Create()) 
	{
		int newType = (int)_ct->user; // -1 means toggle current type
		if (newType == -1)
			newType = g_resType;
		w->Show(g_resType == newType /* i.e toggle */, true);
		w->SetType(newType);
//		SetFocus(GetDlgItem(w->GetHWND(), IDC_FILTER));
	}
}

int IsResourcesDisplayed(COMMAND_T* _ct) {
	if (ResourcesWnd* w = g_resWndMgr.Get())
		return w->IsValidWindow();
	return 0;
}

void ResourcesDeleteAllSlots(COMMAND_T* _ct) {
	ClearDeleteSlotsFiles(g_SNM_TiedSlotActions[(int)_ct->user], 8);
}

void ResourcesClearSlotPrompt(COMMAND_T* _ct)
{
	int type = (int)_ct->user;
	if (type>=0 && type<SNM_NUM_DEFAULT_SLOTS)
		if (ResourceList* fl = g_SNM_ResSlots.Get(g_SNM_TiedSlotActions[type]))
		{
			if (!fl->GetSize())
			{
				WDL_FastString msg;
				msg.SetFormatted(128, NO_SLOT_ERR_STR, fl->GetDesc());
				MessageBox(GetMainHwnd(), msg.Get(), __LOCALIZE("S&M - Error","sws_DLG_150"), MB_OK);
				return;
			}
			int slot = PromptForInteger(SWS_CMD_SHORTNAME(_ct), __LOCALIZE("Slot","sws_DLG_150"), 1, fl->GetSize()); // loops on err
			if (slot != -1) // user has not cancelled
				ClearSlot(g_SNM_TiedSlotActions[type], slot); // incl. UI update + detach file from project
		}
}

void ResourcesClearFXChainSlot(COMMAND_T* _ct) {
	ClearSlot(g_SNM_TiedSlotActions[SNM_SLOT_FXC], (int)_ct->user);
}

void ResourcesClearTrTemplateSlot(COMMAND_T* _ct) {
	ClearSlot(g_SNM_TiedSlotActions[SNM_SLOT_TR], (int)_ct->user);
}

void ResourcesClearPrjTemplateSlot(COMMAND_T* _ct) {
	ClearSlot(g_SNM_TiedSlotActions[SNM_SLOT_PRJ], (int)_ct->user);
}

void ResourcesClearMediaSlot(COMMAND_T* _ct) {
	ClearSlot(g_SNM_TiedSlotActions[SNM_SLOT_MEDIA], (int)_ct->user);
	//JFB TODO? stop sound if needed?
}

void ResourcesClearImageSlot(COMMAND_T* _ct) {
	ClearSlot(g_SNM_TiedSlotActions[SNM_SLOT_IMG], (int)_ct->user);
}

void ResourcesClearThemeSlot(COMMAND_T* _ct) {
	ClearSlot(g_SNM_TiedSlotActions[SNM_SLOT_THM], (int)_ct->user);
}

// specific auto-save for fx chains
void ResourcesAutoSaveFXChain(COMMAND_T* _ct) {
	AutoSave(g_SNM_TiedSlotActions[SNM_SLOT_FXC], false, (int)_ct->user);
}

// specific auto-save for track templates
void ResourcesAutoSaveTrTemplate(COMMAND_T* _ct) {
	AutoSave(g_SNM_TiedSlotActions[SNM_SLOT_TR], false, (int)_ct->user);
}

// auto-save for all other types..
void ResourcesAutoSave(COMMAND_T* _ct)
{
	int type = g_SNM_TiedSlotActions[(int)_ct->user];
	if (type>=0 && type<g_SNM_ResSlots.GetSize())
		if (g_SNM_ResSlots.Get(type)->IsAutoSave())
			AutoSave(type, false, 0);
}


///////////////////////////////////////////////////////////////////////////////
// ReaScript export
///////////////////////////////////////////////////////////////////////////////

int SNM_SelectResourceBookmark(const char* _name)
{
	if (ResourcesWnd* w = g_resWndMgr.Get())
		return w->SetType(_name);
	return -1;
}

void SNM_TieResourceSlotActions(int _bookmarkId) {
	int typeForUser = _bookmarkId>=0 ? GetTypeForUser(_bookmarkId) : -1;
	if (typeForUser>=0 && typeForUser<SNM_NUM_DEFAULT_SLOTS)
		g_SNM_TiedSlotActions[typeForUser] = _bookmarkId;
}


///////////////////////////////////////////////////////////////////////////////
// ImageWnd
///////////////////////////////////////////////////////////////////////////////

#define IMGID	2000 //JFB would be great to have _APS_NEXT_CONTROL_VALUE *always* defined

SNM_WindowManager<ImageWnd> g_imgWndMgr(IMG_WND_ID);
bool g_stretchPref = false;
char g_lastImgFnPref[SNM_MAX_PATH] =  "";


// S&M windows lazy init: below's "" prevents registering the SWS' screenset callback
// (use the S&M one instead - already registered via SNM_WindowManager::Init())
ImageWnd::ImageWnd()
	: SWS_DockWnd(IDD_SNM_IMAGE, __LOCALIZE("Image","sws_DLG_162"), "", SWSGetCommandID(OpenImageWnd))
{
	m_id.Set(IMG_WND_ID);
	m_stretch = g_stretchPref;
	m_img.SetImage(g_lastImgFnPref);

	// Must call SWS_DockWnd::Init() to restore parameters and open the window if necessary
	Init();
}

void ImageWnd::OnInitDlg()
{
	m_vwnd_painter.SetGSC(WDL_STYLE_GetSysColor);
	m_parentVwnd.SetRealParent(m_hwnd);
	
	m_img.SetID(IMGID);
	m_parentVwnd.AddChild(&m_img);
	RequestRedraw();
}

void ImageWnd::OnCommand(WPARAM wParam, LPARAM lParam)
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

HMENU ImageWnd::OnContextMenu(int x, int y, bool* wantDefaultItems)
{
	HMENU hMenu = CreatePopupMenu();
	AddToMenu(hMenu, __LOCALIZE("Stretch to fit","sws_DLG_162"), 0xF001, -1, false, m_stretch?MFS_CHECKED:MFS_UNCHECKED);
	return hMenu;
}

void ImageWnd::DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight)
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

bool ImageWnd::GetToolTipString(int _xpos, int _ypos, char* _bufOut, int _bufOutSz)
{
	if (WDL_VWnd* v = m_parentVwnd.VirtWndFromPoint(_xpos,_ypos,1))
		if (v->GetID()==IMGID && g_SNM_LastImgSlot>=0 && *GetFilename())
			return (_snprintfStrict(_bufOut, _bufOutSz, __LOCALIZE_VERFMT("Image slot %d: %s","sws_DLG_162"), g_SNM_LastImgSlot+1, GetFilename()) > 0);
	return false;
}

int ImageInit()
{
	// load prefs
	g_stretchPref = (GetPrivateProfileInt("ImageView", "Stretch", 0, g_SNM_IniFn.Get()) == 1);
	GetPrivateProfileString("ImageView", "LastImage", "", g_lastImgFnPref, sizeof(g_lastImgFnPref), g_SNM_IniFn.Get());

	// instanciate the window if needed, can be NULL
	g_imgWndMgr.Init();

	return 1;
}

void ImageExit()
{
	// save prefs
	if (ImageWnd* w = g_imgWndMgr.Get())
	{
		WritePrivateProfileString("ImageView", "Stretch", w->IsStretched()?"1":"0", g_SNM_IniFn.Get()); 

		const char* fn = w->GetFilename();
		if (*fn)
		{
			WDL_FastString str;
			makeEscapedConfigString(fn, &str);
			WritePrivateProfileString("ImageView", "LastImage", str.Get(), g_SNM_IniFn.Get());
		}
		else
			WritePrivateProfileString("ImageView", "LastImage", NULL, g_SNM_IniFn.Get());
	}

	g_imgWndMgr.Delete();
}

void OpenImageWnd(COMMAND_T* _ct)
{
	bool isNew;
	if (ImageWnd* w = g_imgWndMgr.Create(&isNew))
	{
		if (isNew) {
			w->SetStretch(g_stretchPref);
			w->SetImage(g_lastImgFnPref);
		}
		w->Show(true, true);
		w->RequestRedraw();
	}
}

bool OpenImageWnd(const char* _fn)
{
	bool isNew, ok = false;
	if (ImageWnd* w = g_imgWndMgr.Create(&isNew))
	{
		if (isNew) {
			w->SetStretch(g_stretchPref);
			w->SetImage(g_lastImgFnPref);
		}
		ok = true;
		WDL_FastString prevFn(w->GetFilename());
		w->SetImage(_fn && *_fn ? _fn : NULL); // NULL clears the current image
		w->Show(!strcmp(prevFn.Get(), _fn) /* i.e toggle */, true);
		w->RequestRedraw();
	}
	return ok;
}

void ClearImageWnd(COMMAND_T*) {
	if (ImageWnd* w = g_imgWndMgr.Get()) {
		w->SetImage(NULL);
		w->RequestRedraw();
	}
}

int IsImageWndDisplayed(COMMAND_T*)
{
	if (ImageWnd* w = g_imgWndMgr.Get())
		return w->IsValidWindow();
	return 0;
}
