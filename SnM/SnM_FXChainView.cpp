/******************************************************************************
/ SnM_FXChainView.cpp
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

//JFB TODO?
// F2: rename

#include "stdafx.h"
#include "../../WDL/projectcontext.h"
#include "SnM_Actions.h"
#include "SNM_FXChainView.h"
#ifdef _WIN32
#include "../MediaPool/DragDrop.h" //JFB: move to the trunk?
#endif


// Commands
#define AUTO_FILL_DIR_MSG				0x110001 // common cmds
#define AUTO_FILL_PRJ_MSG				0x110002
#define AUTO_FILL_DEFAULT_MSG			0x110003
#define CLEAR_MSG						0x110004
#define DEL_SLOTS_MSG					0x110005
#define DEL_SLOTFILES_MSG				0x110006
#define ADD_SLOT_MSG					0x110007
#define INSERT_SLOT_MSG					0x110008
#define EDIT_MSG						0x110009
#define EXPLORE_MSG						0x11000A
#define LOAD_MSG						0x11000B
#define AUTOSAVE_MSG					0x11000C
#define AUTOSAVE_DIR_MSG				0x11000D
#define AUTOSAVE_DIR_PRJ_MSG			0x11000E
#define AUTOSAVE_DIR_DEFAULT_MSG		0x11000F
#define FILTER_BY_NAME_MSG				0x110010
#define FILTER_BY_PATH_MSG				0x110011
#define FILTER_BY_DESC_MSG				0x110012
#define FXC_LOAD_APPLY_TRACK_MSG		0x110020 // specific FX chains cmds
#define FXC_LOAD_APPLY_TAKE_MSG			0x110021
#define FXC_LOAD_APPLY_ALL_TAKES_MSG	0x110022
#define FXC_COPY_MSG					0x110023
#define FXC_LOAD_PASTE_TRACK_MSG		0x110024
#define FXC_LOAD_PASTE_TAKE_MSG			0x110025
#define FXC_LOAD_PASTE_ALL_TAKES_MSG	0x110026
#define FXC_AUTO_SAVE_INPUT_FX			0x110027
#define FXC_AUTO_SAVE_TRACK				0x110028
#define FXC_AUTO_SAVE_ITEM				0x110029
#define TRT_LOAD_APPLY_MSG				0x110040 // specific track template cmds
#define TRT_LOAD_IMPORT_MSG				0x110041
#define TRT_LOAD_APPLY_ITEMS_MSG		0x110042
#define TRT_LOAD_PASTE_ITEMS_MSG		0x110043
#define TRT_AUTO_SAVE_WITEMS_MSG		0x110044
#define PRJ_SELECT_LOAD_MSG				0x110050 // specific project template cmds
#define PRJ_SELECT_LOAD_NEWTAB_MSG		0x110051
#define PRJ_AUTO_FILL_RECENTS_MSG		0x110052
#define PRJ_LOADER_CONF					0x110053
#define PRJ_LOADER_SET					0x110054
#define PRJ_LOADER_CLEAR				0x110055

// labels shared by actions and popup menu items
#define FXC_LOAD_APPLY_TRACK_STR		"Apply FX chain to selected tracks"
#define FXCIN_LOAD_APPLY_TRACK_STR		"Apply input FX chain to selected tracks"
#define FXC_LOAD_APPLY_TAKE_STR			"Apply FX chain to selected items"
#define FXC_LOAD_APPLY_ALL_TAKES_STR	"Apply FX chain to selected items, all takes"
#define FXC_LOAD_PASTE_TRACK_STR		"Paste FX chain to selected tracks"
#define FXC_LOAD_PASTE_TAKE_STR			"Paste FX chain to selected items"
#define FXC_LOAD_PASTE_ALL_TAKES_STR	"Paste FX chain to selected items, all takes"
#define TRT_LOAD_APPLY_STR				"Apply track template to selected tracks (w/o items)"
#define TRT_LOAD_IMPORT_STR				"Import tracks from track template (w/ items)"
#define TRT_LOAD_APPLY_ITEMS_STR		"Replace selected tracks' items w/ track template ones"
#define TRT_LOAD_PASTE_ITEMS_STR		"Paste track template's items to selected tracks"
#define PRJ_SELECT_LOAD_STR				"Select/load project template"
#define PRJ_SELECT_LOAD_NEWTAB_STR		"Select/load project template (new tab)"

#define DRAGNDROP_EMPTY_SLOT_FILE		">Empty<"
#define FILTER_DEFAULT_STR				"Filter"
#define AUTO_SAVE_ERR_STR				"Probable cause: no selection, cannot write file, invalid filename, empty chunk, etc..."

enum {
  SNM_SLOT_TYPE_FX_CHAINS=0,
  SNM_SLOT_TYPE_TR_TEMPLATES,
  SNM_SLOT_TYPE_PRJ_TEMPLATES,
  SNM_SLOT_TYPE_COUNT
};

enum {
  COMBOID_TYPE=1000,
  TXTID_DBL_TYPE,
  COMBOID_DBLCLICK_TYPE,
  TXTID_DBL_TO,
  COMBOID_DBLCLICK_TO,
  BUTTONID_AUTO_SAVE
};

enum {
  FXC_AUTOSAVE_PREF_TRACK=0,
  FXC_AUTOSAVE_PREF_INPUT_FX,
  FXC_AUTOSAVE_PREF_ITEM
};


// JFB important note:
// All global WDL_PtrList vars (includes FileSlotList) *used to be* WDL_PtrList_DeleteOnDestroy ones but
// something weird could occur when REAPER unloads the extension: hang or crash (e.g. issues 292 & 380) 
// on Windows 7 while saving INI files (those lists were already unallocated..)
// Anyway, no prob here because application exit will destroy the entire runtime context regardless.

/*JFB static*/ SNM_ResourceWnd* g_pResourcesWnd = NULL;
static SWS_LVColumn g_fxChainListCols[] = { {65,2,"Slot"}, {100,1,"Name"}, {250,2,"Path"}, {200,1,"Comment"} };

FileSlotList g_fxChainFiles(SNM_SLOT_TYPE_FX_CHAINS, "FXChains", "FX chain", "RfxChain");
FileSlotList g_trTemplateFiles(SNM_SLOT_TYPE_TR_TEMPLATES, "TrackTemplates", "track template", "RTrackTemplate");
FileSlotList g_prjTemplateFiles(SNM_SLOT_TYPE_PRJ_TEMPLATES, "ProjectTemplates", "project template", "RPP");

// shared between the list view & the wnd -------------------------------------
int g_type = -1;

WDL_FastString g_filter(FILTER_DEFAULT_STR);

// other prefs are member variables of SNM_ResourceWnd
int g_filterPref = 1; // 0: name, 1: path, 2: comment
int g_prjLoaderStartPref = -1; // 1-based
int g_prjLoaderEndPref = -1; // 1-based

int g_dblClickType[SNM_SLOT_TYPE_COUNT];
int g_dblClickTo = 0; // for fx chains only

WDL_PtrList<PathSlotItem> g_dragPathSlotItems; 
WDL_PtrList<FileSlotList> g_filesLists;
WDL_PtrList<WDL_FastString> g_autoSaveDirs;
WDL_PtrList<WDL_FastString> g_autoFillDirs;
// ----------------------------------------------------------------------------

FileSlotList* GetCurList() {
	return g_filesLists.Get(g_type);
}

WDL_FastString* GetCurAutoSaveDir() {
	return g_autoSaveDirs.Get(g_type);
}

WDL_FastString* GetCurAutoFillDir() {
	return g_autoFillDirs.Get(g_type);
}

bool IsFiltered() {
	return (g_filter.GetLength() && strcmp(g_filter.Get(), FILTER_DEFAULT_STR));
}


///////////////////////////////////////////////////////////////////////////////
// FileSlotList
///////////////////////////////////////////////////////////////////////////////

// returns -1 on cancel
int FileSlotList::PromptForSlot(const char* _title)
{
	int slot = -1;
	while (slot == -1)
	{
		char promptMsg[64]; 
		_snprintf(promptMsg, 64, "Slot (1-%d):", GetSize());

		char reply[8]= ""; // empty default slot
		if (GetUserInputs(_title, 1, promptMsg, reply, 8))
		{
			slot = atoi(reply); //0 on error
			if (slot > 0 && slot <= GetSize()) {
				return (slot-1);
			}
			else 
			{
				slot = -1;
				char errMsg[128];
				_snprintf(errMsg, 128, "Invalid %s slot!\nPlease enter a value in [1; %d].", m_desc.Get(), GetSize());
				MessageBox(GetMainHwnd(), errMsg, "S&M - Error", /*MB_ICONERROR | */MB_OK);
			}
		}
		else return -1; // user has cancelled
	}
	return -1; //in case the slot comes from mars
}

void FileSlotList::ClearSlot(int _slot, bool _guiUpdate) {
	if (_slot >=0 && _slot < GetSize())	{
		Get(_slot)->Clear();
		if (_guiUpdate && g_pResourcesWnd)
			g_pResourcesWnd->Update();
	}
}

void FileSlotList::ClearSlotPrompt(COMMAND_T* _ct) {
	int slot = PromptForSlot(SNM_CMD_SHORTNAME(_ct)); //loops on err
	if (slot == -1) return; // user has cancelled
	else ClearSlot(slot);
}

// returns false if cancelled
bool FileSlotList::BrowseSlot(int _slot, char* _fn, int _fnSz)
{
	bool ok = false;
	if (_slot >= 0 && _slot < GetSize())
	{
		char title[128]="", filename[BUFFER_SIZE]="", fileFilter[256]="";
		_snprintf(title, 128, "S&M - Load %s (slot %d)", m_desc.Get(), _slot+1);
		GetFileFilter(fileFilter, 256);
		if (BrowseResourcePath(title, m_resDir.Get(), fileFilter, filename, BUFFER_SIZE, true))
		{
			if (_fn)
				lstrcpyn(_fn, filename, _fnSz);

			if (SetFromFullPath(_slot, filename))
			{
				if (g_pResourcesWnd)
					g_pResourcesWnd->Update();
				ok = true;
			}
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
		{
			ok = BrowseSlot(_slot, _fn, _fnSz);
		}
		else
		{
			if (GetFullPath(_slot, _fn, _fnSz))
				ok = FileExistsErrMsg(_fn, _errMsg);
		}
	}
	return ok;
}

void FileSlotList::EditSlot(int _slot)
{
	if (_slot >= 0 && _slot < GetSize())
	{
		char fullPath[BUFFER_SIZE] = "";
		if (GetFullPath(_slot, fullPath, BUFFER_SIZE) && FileExists(fullPath))
		{
#ifdef _WIN32
			WinSpawnNotepad(fullPath);
#else
			WDL_FastString chain;
			if (LoadChunk(fullPath, &chain, false))
			{
				char title[64] = "";
				_snprintf(title, 64, "S&M - %s (slot %d)", m_desc.Get(), _slot+1);
				SNM_ShowMsg(chain.Get(), title);
			}
#endif
		}
		else if (*fullPath != '\0')
		{
			char buf[BUFFER_SIZE];
			_snprintf(buf, BUFFER_SIZE, "File not found:\n%s", fullPath);
			MessageBox(g_hwndParent, buf, "S&M - Error", MB_OK);
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// SNM_ResourceView
///////////////////////////////////////////////////////////////////////////////

SNM_ResourceView::SNM_ResourceView(HWND hwndList, HWND hwndEdit)
:SWS_ListView(hwndList, hwndEdit, 4, g_fxChainListCols, "Resources View State", false)
{
}

void SNM_ResourceView::GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax)
{
	if (str) *str = '\0';
	if (PathSlotItem* pItem = (PathSlotItem*)item)
	{
		switch (iCol)
		{
			case 0:
			{
				int slot = GetCurList()->Find(pItem);
				if (slot >= 0) {
					slot++;
					if (g_type == SNM_SLOT_TYPE_PRJ_TEMPLATES && isProjectLoaderConfValid())
						_snprintf(str, iStrMax, "%5.d %s", slot, slot<g_prjLoaderStartPref || slot>g_prjLoaderEndPref ? "  " : 
							g_prjLoaderStartPref==slot ? "->" : g_prjLoaderEndPref==slot ? "<-" :  "--");
					else
						_snprintf(str, iStrMax, "%5.d", slot);
				}
			}
			break;
			case 1:
			{
				char buf[BUFFER_SIZE] = "";
				ExtractFileNameEx(pItem->m_shortPath.Get(), buf, true);
				lstrcpyn(str, buf, iStrMax);
			}
			break;
			case 2:
				lstrcpyn(str, pItem->m_shortPath.Get(), iStrMax);				
				break;
			case 3:
				lstrcpyn(str, pItem->m_desc.Get(), iStrMax);				
				break;
		}
	}
}

bool SNM_ResourceView::IsEditListItemAllowed(SWS_ListItem* item, int iCol)
{
	if (PathSlotItem* pItem = (PathSlotItem*)item)
		return !pItem->IsDefault();
	return false;
}

void SNM_ResourceView::SetItemText(SWS_ListItem* item, int iCol, const char* str)
{
	PathSlotItem* pItem = (PathSlotItem*)item;
	int slot = GetCurList()->Find(pItem);
	if (pItem && slot >=0)
	{
		switch (iCol)
		{
			// file renaming
			case 1:
			{
				char fn[BUFFER_SIZE] = "";
				if (GetCurList()->GetFullPath(slot, fn, BUFFER_SIZE) && !pItem->IsDefault() && FileExistsErrMsg(fn))
				{
					char newFn[BUFFER_SIZE];
					lstrcpyn(newFn, fn, BUFFER_SIZE);
					if (char* p = strrchr(newFn, PATH_SLASH_CHAR)) *p = '\0';
					else break; // safety
					_snprintf(newFn, BUFFER_SIZE, "%s%c%s.%s", newFn, PATH_SLASH_CHAR, str, GetCurList()->GetFileExt());
					if (FileExists(newFn)) 
					{
						char buf[BUFFER_SIZE];
						_snprintf(buf, BUFFER_SIZE, "File already exists. Overwrite ?\n%s", newFn);
						int res = MessageBox(g_hwndParent, buf, "S&M - Warning", MB_YESNO);
						if (res == IDYES) {
							if (!SNM_DeleteFile(newFn))
								break;
						}
						else 
							break;
					}
					if (MoveFile(fn, newFn) && GetCurList()->SetFromFullPath(slot, newFn))
						ListView_SetItemText(m_hwndList, GetEditingItem(), DisplayToDataCol(2), (LPSTR)pItem->m_shortPath.Get());
						// ^^ direct GUI update 'cause Update() is no-op when editing
				}
			}
			break;

			// description
			case 3:
				// Limit the desc. size (for RPP files)
				pItem->m_desc.Set(str);
				pItem->m_desc.Ellipsize(128,128);
				Update();
				break;
		}
	}
}

void SNM_ResourceView::OnItemDblClk(SWS_ListItem* item, int iCol)
{
	PathSlotItem* pItem = (PathSlotItem*)item;
	int slot = GetCurList()->Find(pItem);
	if (pItem && slot >= 0) 
	{
		bool wasDefaultSlot = pItem->IsDefault();
		switch(g_type)
		{
			case SNM_SLOT_TYPE_FX_CHAINS:
				switch(g_dblClickTo)
				{
					case 0:
						applyTracksFXChainSlot(FXC_LOAD_APPLY_TRACK_STR, slot, !g_dblClickType[g_type], false, !wasDefaultSlot);
						break;
					case 1:
						applyTracksFXChainSlot(FXCIN_LOAD_APPLY_TRACK_STR, slot, !g_dblClickType[g_type], g_bv4, !wasDefaultSlot);
						break;
					case 2:
						applyTakesFXChainSlot(FXC_LOAD_APPLY_TAKE_STR, slot, true, !g_dblClickType[g_type], !wasDefaultSlot);
						break;
					case 3:
						applyTakesFXChainSlot(FXC_LOAD_APPLY_ALL_TAKES_STR, slot, false, !g_dblClickType[g_type], !wasDefaultSlot);
						break;
				}
				break;
			case SNM_SLOT_TYPE_TR_TEMPLATES:
				switch(g_dblClickType[g_type])
				{
					case 0:
						applyOrImportTrackSlot(TRT_LOAD_APPLY_STR, false, slot, false, false, !wasDefaultSlot);
						break;
					case 1:
						applyOrImportTrackSlot(TRT_LOAD_IMPORT_STR, true, slot, false, false, !wasDefaultSlot);
						break;
					case 2:
						replaceOrPasteItemsFromTrackSlot(TRT_LOAD_PASTE_ITEMS_STR, true, slot, !wasDefaultSlot);
						break;
					case 3:
						replaceOrPasteItemsFromTrackSlot(TRT_LOAD_APPLY_ITEMS_STR, false, slot, !wasDefaultSlot);
						break;
				}
				break;
			case SNM_SLOT_TYPE_PRJ_TEMPLATES:
				switch(g_dblClickType[g_type])
				{
					case 0:
						loadOrSelectProject(PRJ_SELECT_LOAD_STR, slot, false, !wasDefaultSlot);
						break;
					case 1:
						loadOrSelectProject(PRJ_SELECT_LOAD_NEWTAB_STR, slot, true, !wasDefaultSlot);
						break;
				}
				break;
		}

		// in case the slot changed
		if (wasDefaultSlot && !pItem->IsDefault())
			Update();
	}
}

void SNM_ResourceView::GetItemList(SWS_ListItemList* pList)
{
	if (IsFiltered())
	{
		int iCount = 0;
		LineParser lp(false);
		lp.parse(g_filter.Get());
		for (int i = 0; i < GetCurList()->GetSize(); i++)
		{
			PathSlotItem* item = GetCurList()->Get(i);
			if (item && (g_filterPref == 2 || (g_filterPref != 2 && !item->IsDefault()))) // 2: filter by comment
			{
				bool match = true;
				for (int j=0; match && j < lp.getnumtokens(); j++)
				{
					switch(g_filterPref)
					{
						case 0: // name
						{
							char buf[BUFFER_SIZE] = "";
							ExtractFileNameEx(item->m_shortPath.Get(), buf, true);
							match &= (stristr(buf, lp.gettoken_str(j)) != NULL);
							break;
						}
						case 1: // path
						{
							char buf[BUFFER_SIZE] = "";
							if (GetCurList()->GetFullPath(i, buf, BUFFER_SIZE)) {
								if (char* p = strrchr(buf, PATH_SLASH_CHAR)) {
									*p = '\0';
									match &= (stristr(buf, lp.gettoken_str(j)) != NULL);
								}
							}
							break;
						}
						case 2: // comment
							match &= (stristr(item->m_desc.Get(), lp.gettoken_str(j)) != NULL);
							break;
					}
				}
				if (match)
					pList->Add((SWS_ListItem*)item);
			}
		}
	}
	else
	{
		for (int i = 0; i < GetCurList()->GetSize(); i++)
			pList->Add((SWS_ListItem*)GetCurList()->Get(i));
	}
}

void SNM_ResourceView::OnBeginDrag(SWS_ListItem* _item)
{
#ifdef _WIN32
	g_dragPathSlotItems.Empty(false);

	// Store dragged items (for internal d'n'd) + full paths + get the amount of memory needed
	int iMemNeeded = 0, x=0;
	WDL_PtrList_DeleteOnDestroy<WDL_FastString> fullPaths;
	PathSlotItem* pItem = (PathSlotItem*)EnumSelected(&x);
	while(pItem) {
		int slot = GetCurList()->Find(pItem);
		char fullPath[BUFFER_SIZE] = "";
		if (GetCurList()->GetFullPath(slot, fullPath, BUFFER_SIZE))
		{
			bool empty = (pItem->IsDefault() || *fullPath == '\0');
			iMemNeeded += (int)((empty ? strlen(DRAGNDROP_EMPTY_SLOT_FILE) : strlen(fullPath)) + 1);
			fullPaths.Add(new WDL_FastString(empty ? DRAGNDROP_EMPTY_SLOT_FILE : fullPath));
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

	// Add the files to the DROPFILES struct, double-NULL terminated
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


///////////////////////////////////////////////////////////////////////////////
// SNM_ResourceWnd
///////////////////////////////////////////////////////////////////////////////

SNM_ResourceWnd::SNM_ResourceWnd()
:SWS_DockWnd(IDD_SNM_FXCHAINLIST, "Resources", "SnMResources", 30006, SWSGetCommandID(OpenResourceView))
{
	m_previousType = g_type;
	m_autoSaveTrTmpltWithItemsPref = true;

	// Must call SWS_DockWnd::Init() to restore parameters and open the window if necessary
	Init();
}

void SNM_ResourceWnd::SetType(int _type)
{
	m_previousType = g_type;
	g_type = _type;
	m_cbType.SetCurSel(_type);
	if (m_previousType != g_type)
	{
		FillDblClickTypeCombo();
		Update();
	}
}

void SNM_ResourceWnd::Update()
{
	if (m_pLists.GetSize())
		m_pLists.Get(0)->Update();
	m_parentVwnd.RequestRedraw(NULL);
}

void SNM_ResourceWnd::FillDblClickTypeCombo()
{
	m_cbDblClickType.Empty();
	switch(g_type)
	{
		case SNM_SLOT_TYPE_FX_CHAINS:
			m_cbDblClickType.AddItem("Apply");
			m_cbDblClickType.AddItem("Paste");
			break;
		case SNM_SLOT_TYPE_TR_TEMPLATES:
			m_cbDblClickType.AddItem("Apply to sel. tracks (w/o items)");
			m_cbDblClickType.AddItem("Import tracks (w/ items)");
			m_cbDblClickType.AddItem("Paste items to sel. tracks");
			m_cbDblClickType.AddItem("Replace items of sel. tracks");
			break;
		case SNM_SLOT_TYPE_PRJ_TEMPLATES:
			m_cbDblClickType.AddItem("Load/select project");
			m_cbDblClickType.AddItem("Load/select project tab");
			break;
	}
	m_cbDblClickType.SetCurSel(g_dblClickType[g_type]);
}

void SNM_ResourceWnd::OnInitDlg()
{
	m_resize.init_item(IDC_LIST, 0.0, 0.0, 1.0, 1.0);
	m_resize.init_item(IDC_FILTER, 1.0, 0.0, 1.0, 0.0);
	SetWindowLongPtr(GetDlgItem(m_hwnd, IDC_FILTER), GWLP_USERDATA, 0xdeadf00b);
/*JFB commented: useless (?) since r488 theming updates
#ifndef _WIN32
	// Realign the filter box on OSX
	HWND hFilter = GetDlgItem(m_hwnd, IDC_FILTER);
	RECT rFilter;
	GetWindowRect(hFilter, &rFilter);
	ScreenToClient(m_hwnd,(LPPOINT)&rFilter);
	ScreenToClient(m_hwnd,((LPPOINT)&rFilter)+1);
	SetWindowPos(hFilter, NULL, rFilter.left - 25, rFilter.top - 1, abs(rFilter.right - rFilter.left) - 10, abs(rFilter.bottom - rFilter.top), SWP_NOACTIVATE | SWP_NOZORDER);
#endif
*/
	m_pLists.Add(new SNM_ResourceView(GetDlgItem(m_hwnd, IDC_LIST), GetDlgItem(m_hwnd, IDC_EDIT)));
	SNM_ThemeListView(m_pLists.Get(0), true);

	// Load prefs 
	//JFB!!! pb qd 1ere init (no .ini)
	// Ok pour l'instant car override par OpenResourceView() via son appel à SetType()
	g_type = GetPrivateProfileInt("RESOURCE_VIEW", "Type", SNM_SLOT_TYPE_FX_CHAINS, g_SNMiniFilename.Get());
	g_dblClickType[SNM_SLOT_TYPE_FX_CHAINS] = GetPrivateProfileInt("RESOURCE_VIEW", "DblClick_Type", 0, g_SNMiniFilename.Get());
	g_dblClickType[SNM_SLOT_TYPE_TR_TEMPLATES] = GetPrivateProfileInt("RESOURCE_VIEW", "DblClick_Type_Tr_Template", 0, g_SNMiniFilename.Get());
	g_dblClickType[SNM_SLOT_TYPE_PRJ_TEMPLATES] = GetPrivateProfileInt("RESOURCE_VIEW", "DblClick_Type_Prj_Template", 0, g_SNMiniFilename.Get());
	g_dblClickTo = GetPrivateProfileInt("RESOURCE_VIEW", "DblClick_To", 0, g_SNMiniFilename.Get());
	m_autoSaveFXChainPref = GetPrivateProfileInt("RESOURCE_VIEW", "AutoSaveFXChain", FXC_AUTOSAVE_PREF_TRACK, g_SNMiniFilename.Get());
	m_autoSaveTrTmpltWithItemsPref = (GetPrivateProfileInt("RESOURCE_VIEW", "AutoSaveTrTemplateWithItems", 1, g_SNMiniFilename.Get()) == 1);
	g_filterPref = GetPrivateProfileInt("RESOURCE_VIEW", "FilterByPath", 1, g_SNMiniFilename.Get());

	g_prjLoaderStartPref = GetPrivateProfileInt("RESOURCE_VIEW", "ProjectLoaderStartSlot", 1, g_SNMiniFilename.Get());
	g_prjLoaderEndPref = GetPrivateProfileInt("RESOURCE_VIEW", "ProjectLoaderEndSlot", g_filesLists.Get(SNM_SLOT_TYPE_PRJ_TEMPLATES)->GetSize(), g_SNMiniFilename.Get());

	// auto-save & auto-fill directories
	{
		char defaultPath[BUFFER_SIZE] = "", path[BUFFER_SIZE] = "";
		g_autoSaveDirs.Empty(true);
		_snprintf(defaultPath, BUFFER_SIZE, "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, g_filesLists.Get(0)->GetResourceDir());
		GetPrivateProfileString("RESOURCE_VIEW", "AutoSaveDirFXChain", defaultPath, path, BUFFER_SIZE, g_SNMiniFilename.Get());
		g_autoSaveDirs.Add(new WDL_FastString(path));
		_snprintf(defaultPath, BUFFER_SIZE, "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, g_filesLists.Get(1)->GetResourceDir());
		GetPrivateProfileString("RESOURCE_VIEW", "AutoSaveDirTrTemplate", defaultPath, path, BUFFER_SIZE, g_SNMiniFilename.Get());
		g_autoSaveDirs.Add(new WDL_FastString(path));
		_snprintf(defaultPath, BUFFER_SIZE, "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, g_filesLists.Get(2)->GetResourceDir());
		GetPrivateProfileString("RESOURCE_VIEW", "AutoSaveDirPrjTemplate", defaultPath, path, BUFFER_SIZE, g_SNMiniFilename.Get());
		g_autoSaveDirs.Add(new WDL_FastString(path));

		g_autoFillDirs.Empty(true);
		GetPrivateProfileString("RESOURCE_VIEW", "AutoFillDirFXChain", defaultPath, path, BUFFER_SIZE, g_SNMiniFilename.Get());
		g_autoFillDirs.Add(new WDL_FastString(path));
		_snprintf(defaultPath, BUFFER_SIZE, "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, g_filesLists.Get(1)->GetResourceDir());
		GetPrivateProfileString("RESOURCE_VIEW", "AutoFillDirTrTemplate", defaultPath, path, BUFFER_SIZE, g_SNMiniFilename.Get());
		g_autoFillDirs.Add(new WDL_FastString(path));
		_snprintf(defaultPath, BUFFER_SIZE, "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, g_filesLists.Get(2)->GetResourceDir());
		GetPrivateProfileString("RESOURCE_VIEW", "AutoFillDirPrjTemplate", defaultPath, path, BUFFER_SIZE, g_SNMiniFilename.Get());
		g_autoFillDirs.Add(new WDL_FastString(path));
	}

	// WDL GUI init
	m_vwnd_painter.SetGSC(WDL_STYLE_GetSysColor);
	m_parentVwnd.SetRealParent(m_hwnd);

	m_cbType.SetID(COMBOID_TYPE);
	m_cbType.AddItem("FX chains");
	m_cbType.AddItem("Track templates");
	m_cbType.AddItem("Project templates");
	m_cbType.SetCurSel(g_type);
	m_parentVwnd.AddChild(&m_cbType);

	m_cbDblClickType.SetID(COMBOID_DBLCLICK_TYPE);
	FillDblClickTypeCombo(); //JFB single call to SetType() instead !?
	m_parentVwnd.AddChild(&m_cbDblClickType);

	m_cbDblClickTo.SetID(COMBOID_DBLCLICK_TO);
	m_cbDblClickTo.AddItem("Tracks");
	m_cbDblClickTo.AddItem("Tracks (input FX)");
	m_cbDblClickTo.AddItem("Items");
	m_cbDblClickTo.AddItem("Items (all takes)");
	m_cbDblClickTo.SetCurSel(g_dblClickTo);
	m_parentVwnd.AddChild(&m_cbDblClickTo);

	m_btnAutoSave.SetID(BUTTONID_AUTO_SAVE);
	m_parentVwnd.AddChild(&m_btnAutoSave);

	m_txtDblClickType.SetID(TXTID_DBL_TYPE);
	m_txtDblClickType.SetText("Dbl-click to:");
	m_parentVwnd.AddChild(&m_txtDblClickType);

	m_txtDblClickTo.SetID(TXTID_DBL_TO);
	m_txtDblClickTo.SetText("To selected:");
	m_parentVwnd.AddChild(&m_txtDblClickTo);

	// This restores the text filter when docking/undocking
	// >>> + indirect call to Update() <<<
	SetDlgItemText(GetHWND(), IDC_FILTER, g_filter.Get());

/* Perfs: see above comment
	Update();
*/
}

void SNM_ResourceWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	int x=0;
	PathSlotItem* item = (PathSlotItem*)m_pLists.Get(0)->EnumSelected(&x);
	int slot = GetCurList()->Find(item);
	bool wasDefaultSlot = item ? item->IsDefault() : false;
	switch(wParam)
	{
		case (IDC_FILTER | (EN_CHANGE << 16)):
		{
			char cFilter[128];
			GetWindowText(GetDlgItem(m_hwnd, IDC_FILTER), cFilter, 128);
			g_filter.Set(cFilter);
			Update();
			break;
		}
#ifdef _WIN32
		case (IDC_FILTER | (EN_SETFOCUS << 16)):
		{
			HWND hFilt = GetDlgItem(m_hwnd, IDC_FILTER);
			char cFilter[128];
			GetWindowText(hFilt, cFilter, 128);
			if (!strcmp(cFilter, FILTER_DEFAULT_STR))
				SetWindowText(hFilt, "");
			break;
		}
		case (IDC_FILTER | (EN_KILLFOCUS << 16)):
		{
			HWND hFilt = GetDlgItem(m_hwnd, IDC_FILTER);
			char cFilter[128];
			GetWindowText(hFilt, cFilter, 128);
			if (*cFilter == '\0') 
				SetWindowText(hFilt, FILTER_DEFAULT_STR);
			break;
		}
#endif

		// ***** Common *****
		case ADD_SLOT_MSG:
			AddSlot(true);
			break;
		case INSERT_SLOT_MSG:
			InsertAtSelectedSlot(true);
			break;
		case DEL_SLOTFILES_MSG:
		case DEL_SLOTS_MSG:
			DeleteSelectedSlots(true, wParam == DEL_SLOTFILES_MSG);
			break;
		case LOAD_MSG:
			if (item)
				GetCurList()->BrowseSlot(slot);
			break;
		case CLEAR_MSG: 
		{
			bool updt = false;
			while(item) {
				slot = GetCurList()->Find(item);
				GetCurList()->ClearSlot(slot, false);
				updt = true;
				item = (PathSlotItem*)m_pLists.Get(0)->EnumSelected(&x);
			}
			if (updt) Update();
			break;
		}
		case EDIT_MSG:
			if (item)
				GetCurList()->EditSlot(slot);
			break;
		case EXPLORE_MSG:
		{
			char fullPath[BUFFER_SIZE] = "";
			if (GetCurList()->GetFullPath(slot, fullPath, BUFFER_SIZE))
				if (char* p = strrchr(fullPath, PATH_SLASH_CHAR)) {
					*(p+1) = '\0'; // ShellExecute() is KO otherwie..
					ShellExecute(NULL, "open", fullPath, NULL, NULL, SW_SHOWNORMAL);
				}
			break;
		}
		case AUTO_FILL_DIR_MSG:
		{
			char startPath[BUFFER_SIZE] = "";
			if (BrowseForDirectory("Auto-fill directory", GetCurAutoFillDir()->Get(), startPath, BUFFER_SIZE)) {
				GetCurAutoFillDir()->Set(startPath);
				AutoFill(startPath);
			}
			break;
		}
		case AUTO_FILL_PRJ_MSG:
		{
			char startPath[BUFFER_SIZE] = "";
			GetProjectPath(startPath, BUFFER_SIZE);
			AutoFill(startPath); // will look for sub-directories
			break;
		}
		case AUTO_FILL_DEFAULT_MSG:
		{
			char startPath[BUFFER_SIZE] = "";
			_snprintf(startPath, BUFFER_SIZE, "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, GetCurList()->GetResourceDir());
			AutoFill(startPath);
			break;
		}
		case AUTOSAVE_MSG:
			AutoSave();
			break;
		case AUTOSAVE_DIR_MSG:
		{
			char path[BUFFER_SIZE] = "";
			if (BrowseForDirectory("Set auto-save directory", GetCurAutoSaveDir()->Get(), path, BUFFER_SIZE))
				GetCurAutoSaveDir()->Set(path);
			break;
		}
		case AUTOSAVE_DIR_PRJ_MSG:
		{
			char prjPath[BUFFER_SIZE] = "", path[BUFFER_SIZE] = "";
			GetProjectPath(prjPath, BUFFER_SIZE);
			if (!strstr(prjPath, GetResourcePath()))
				_snprintf(path, BUFFER_SIZE, "%s%c%s", prjPath, PATH_SLASH_CHAR, GetCurList()->GetResourceDir());
			else
				_snprintf(path, BUFFER_SIZE, "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, GetCurList()->GetResourceDir());
			if (!FileExists(path))
				CreateDirectory(path, NULL);
			GetCurAutoSaveDir()->Set(path);
			break;
		}
		case AUTOSAVE_DIR_DEFAULT_MSG:
		{
			char path[BUFFER_SIZE] = "";
			_snprintf(path, BUFFER_SIZE, "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, GetCurList()->GetResourceDir());
			if (!FileExists(path))
				CreateDirectory(path, NULL);
			GetCurAutoSaveDir()->Set(path);
			break;
		}
		case FILTER_BY_NAME_MSG:
			g_filterPref = 0;
			Update();
//			SetFocus(GetDlgItem(m_hwnd, IDC_FILTER));
			break;
		case FILTER_BY_PATH_MSG:
			g_filterPref = 1;
			Update();
//			SetFocus(GetDlgItem(m_hwnd, IDC_FILTER));
			break;
		case FILTER_BY_DESC_MSG:
			g_filterPref = 2;
			Update();
//			SetFocus(GetDlgItem(m_hwnd, IDC_FILTER));
			break;

		// ***** FX chains *****
		case FXC_COPY_MSG:
			if (item) copyFXChainSlotToClipBoard(slot);
			break;
		case FXC_LOAD_APPLY_TRACK_MSG:
		case FXC_LOAD_PASTE_TRACK_MSG:
			if (item && slot >= 0) {
				applyTracksFXChainSlot(wParam == FXC_LOAD_APPLY_TRACK_MSG ? FXC_LOAD_APPLY_TRACK_STR : FXC_LOAD_PASTE_TRACK_STR, slot, wParam == FXC_LOAD_APPLY_TRACK_MSG, false, !wasDefaultSlot);
				if (wasDefaultSlot && !item->IsDefault()) // slot has been filled ?
					Update();
			}
			break;
		case FXC_LOAD_APPLY_TAKE_MSG:
		case FXC_LOAD_PASTE_TAKE_MSG:
			if (item && slot >= 0) {
				applyTakesFXChainSlot(wParam == FXC_LOAD_APPLY_TAKE_MSG ? FXC_LOAD_APPLY_TAKE_STR : FXC_LOAD_PASTE_TAKE_STR, slot, true, wParam == FXC_LOAD_APPLY_TAKE_MSG, !wasDefaultSlot);
				if (wasDefaultSlot && !item->IsDefault()) // slot has been filled ?
					Update();
			}
			break;
		case FXC_LOAD_APPLY_ALL_TAKES_MSG:
		case FXC_LOAD_PASTE_ALL_TAKES_MSG:
			if (item && slot >= 0) {
				applyTakesFXChainSlot(wParam == FXC_LOAD_APPLY_ALL_TAKES_MSG ? FXC_LOAD_APPLY_ALL_TAKES_STR : FXC_LOAD_PASTE_ALL_TAKES_STR, slot, false, wParam == FXC_LOAD_APPLY_ALL_TAKES_MSG, !wasDefaultSlot);
				if (wasDefaultSlot && !item->IsDefault()) // slot has been filled ?
					Update();
			}
			break;
		case FXC_AUTO_SAVE_INPUT_FX:
			m_autoSaveFXChainPref = FXC_AUTOSAVE_PREF_INPUT_FX;
			break;
		case FXC_AUTO_SAVE_TRACK:
			m_autoSaveFXChainPref = FXC_AUTOSAVE_PREF_TRACK;
			break;
		case FXC_AUTO_SAVE_ITEM:
			m_autoSaveFXChainPref = FXC_AUTOSAVE_PREF_ITEM;
			break;

		// ***** Track template *****
		case TRT_LOAD_APPLY_MSG:
		case TRT_LOAD_IMPORT_MSG:
			if (item && slot >= 0) {
				applyOrImportTrackSlot(wParam == TRT_LOAD_APPLY_MSG ? TRT_LOAD_APPLY_STR : TRT_LOAD_IMPORT_STR, wParam == TRT_LOAD_IMPORT_MSG, slot, false, false, !wasDefaultSlot);
				if (wasDefaultSlot && !item->IsDefault()) // slot has been filled ?
					Update();
			}
			break;
		case TRT_LOAD_APPLY_ITEMS_MSG:
		case TRT_LOAD_PASTE_ITEMS_MSG:
			if (item && slot >= 0) {
				replaceOrPasteItemsFromTrackSlot(wParam == TRT_LOAD_APPLY_ITEMS_MSG ? TRT_LOAD_APPLY_ITEMS_STR : TRT_LOAD_PASTE_ITEMS_STR, wParam == TRT_LOAD_PASTE_ITEMS_MSG, slot, !wasDefaultSlot);
				if (wasDefaultSlot && !item->IsDefault()) // slot has been filled ?
					Update();
			}
			break;
		case TRT_AUTO_SAVE_WITEMS_MSG:
			m_autoSaveTrTmpltWithItemsPref = !m_autoSaveTrTmpltWithItemsPref;
			break;

		// ***** Project template *****
		case PRJ_SELECT_LOAD_MSG:
		case PRJ_SELECT_LOAD_NEWTAB_MSG:
			if (item && slot >= 0) {
				loadOrSelectProject(wParam == PRJ_SELECT_LOAD_MSG ? PRJ_SELECT_LOAD_STR : PRJ_SELECT_LOAD_NEWTAB_STR, slot, wParam == PRJ_SELECT_LOAD_NEWTAB_MSG, !wasDefaultSlot);
				if (wasDefaultSlot && !item->IsDefault()) // slot has been filled ?
					Update();
			}
			break;
		case PRJ_AUTO_FILL_RECENTS_MSG:
		{
			int startSlot = GetCurList()->GetSize();
			int nbRecents = GetPrivateProfileInt("REAPER", "numrecent", 0, get_ini_file());
			nbRecents = min(98, nbRecents); // just in case: 2 digits max..
			if (nbRecents)
			{
				char key[16], path[BUFFER_SIZE];
				WDL_PtrList_DeleteOnDestroy<WDL_FastString> prjs;
				for (int i=0; i < nbRecents; i++) {
					_snprintf(key, 9, "recent%02d", i+1);
					GetPrivateProfileString("Recent", key, "", path, BUFFER_SIZE, get_ini_file());
					if (*path)
						prjs.Add(new WDL_FastString(path));
				}
				for (int i=0; i < prjs.GetSize(); i++)
					if (GetCurList()->FindByResFulltPath(prjs.Get(i)->Get()) < 0) // skip if already present
						GetCurList()->AddSlot(prjs.Get(i)->Get());
			}
			if (startSlot != GetCurList()->GetSize()) {
				Update();
				SelectBySlot(startSlot, GetCurList()->GetSize());
			}
			else
				MessageBox(GetMainHwnd(), "No valid recent projects found!", "S&M - Warning", MB_OK);
			break;
		}
		case PRJ_LOADER_CONF:
			projectLoaderConf(NULL);
			break;

		case PRJ_LOADER_SET:
		{
			int min=GetCurList()->GetSize(), max=0;
			while(item) {
				slot = GetCurList()->Find(item)+1;
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
		case PRJ_LOADER_CLEAR:
			g_prjLoaderStartPref = g_prjLoaderEndPref = -1;
			Update();
			break;

		// ***** WDL GUI & others *****
		default:
		{
			if (HIWORD(wParam)==0) 
			{
				switch(LOWORD(wParam))
				{
					case BUTTONID_AUTO_SAVE:
						AutoSave();
					break;
				}
			}
			else if (HIWORD(wParam)==CBN_SELCHANGE && LOWORD(wParam)==COMBOID_TYPE)
			{
				// stop cell editing (changing the list content would be ignored otherwise
				// => dropdown box & list box unsynchronized)
				m_pLists.Get(0)->EditListItemEnd(false);

				//JFB SetType() instead !?
				int previousType = g_type;
				g_type = m_cbType.GetCurSel();
				if (g_type != previousType)
				{
					FillDblClickTypeCombo();
					Update();
//					SetFocus(GetDlgItem(m_hwnd, IDC_FILTER));
				}
			}
			else if (HIWORD(wParam)==CBN_SELCHANGE && LOWORD(wParam)==COMBOID_DBLCLICK_TYPE)
				g_dblClickType[g_type] = m_cbDblClickType.GetCurSel();

			else if (HIWORD(wParam)==CBN_SELCHANGE && LOWORD(wParam)==COMBOID_DBLCLICK_TO)
				g_dblClickTo = m_cbDblClickTo.GetCurSel();
			else 
				Main_OnCommand((int)wParam, (int)lParam);
			break;
		}
	}
}

HMENU SNM_ResourceWnd::OnContextMenu(int x, int y)
{
	HMENU hMenu = CreatePopupMenu();

	HMENU hAutoFillSubMenu = CreatePopupMenu();
	AddSubMenu(hMenu, hAutoFillSubMenu, "Auto-fill");
	AddToMenu(hAutoFillSubMenu, "Auto-fill...", AUTO_FILL_DIR_MSG, -1, false, !IsFiltered() ? MF_ENABLED : MF_GRAYED);
	AddToMenu(hAutoFillSubMenu, "Auto-fill from default resource path", AUTO_FILL_DEFAULT_MSG, -1, false, !IsFiltered() ? MF_ENABLED : MF_GRAYED);
	AddToMenu(hAutoFillSubMenu, "Auto-fill from project path", AUTO_FILL_PRJ_MSG, -1, false, !IsFiltered() ? MF_ENABLED : MF_GRAYED);
	if (g_type == SNM_SLOT_TYPE_PRJ_TEMPLATES)
		AddToMenu(hAutoFillSubMenu, "Auto-fill with recent projects", PRJ_AUTO_FILL_RECENTS_MSG, -1, false, !IsFiltered() ? MF_ENABLED : MF_GRAYED);

	AddToMenu(hMenu, SWS_SEPARATOR, 0);

	HMENU hAutoSaveSubMenu = CreatePopupMenu();
	AddToMenu(hMenu, "Auto-save", AUTOSAVE_MSG, -1, false, !IsFiltered() ? MF_ENABLED : MF_GRAYED);
	AddSubMenu(hMenu, hAutoSaveSubMenu, "Auto-save configuration");
	char autoSavePath[BUFFER_SIZE] = "";
	_snprintf(autoSavePath, BUFFER_SIZE, "(Current auto-save path: %s)", GetCurAutoSaveDir()->Get());
	AddToMenu(hAutoSaveSubMenu, autoSavePath, 0, -1, false, MF_DISABLED); // different from MFS_DISABLED! more readable (REAPER like)
	AddToMenu(hAutoSaveSubMenu, "Set auto-save directory...", AUTOSAVE_DIR_MSG);
	AddToMenu(hAutoSaveSubMenu, "Set auto-save directory to default resource path", AUTOSAVE_DIR_DEFAULT_MSG);
	switch(g_type)
	{
		case SNM_SLOT_TYPE_FX_CHAINS:
			AddToMenu(hAutoSaveSubMenu, "Set auto-save directory to project path (/FXChains)", AUTOSAVE_DIR_PRJ_MSG);
			AddToMenu(hAutoSaveSubMenu, SWS_SEPARATOR, 0);
			AddToMenu(hAutoSaveSubMenu, "Auto-save FX chains from track selection", FXC_AUTO_SAVE_TRACK, -1, false, m_autoSaveFXChainPref == FXC_AUTOSAVE_PREF_TRACK ? MFS_CHECKED : MFS_UNCHECKED);
			AddToMenu(hAutoSaveSubMenu, "Auto-save FX chains from item selection", FXC_AUTO_SAVE_ITEM, -1, false, m_autoSaveFXChainPref == FXC_AUTOSAVE_PREF_ITEM ? MFS_CHECKED : MFS_UNCHECKED);
			if (g_bv4)
				AddToMenu(hAutoSaveSubMenu, "Auto-save input FX chains from track selection", FXC_AUTO_SAVE_INPUT_FX, -1, false, m_autoSaveFXChainPref == FXC_AUTOSAVE_PREF_INPUT_FX ? MFS_CHECKED : MFS_UNCHECKED);
			break;
		case SNM_SLOT_TYPE_TR_TEMPLATES:
			AddToMenu(hAutoSaveSubMenu, "Set auto-save directory to project path (/TrackTemplates)", AUTOSAVE_DIR_PRJ_MSG);
			AddToMenu(hAutoSaveSubMenu, SWS_SEPARATOR, 0);
			AddToMenu(hAutoSaveSubMenu, "Auto-save track templates with items", TRT_AUTO_SAVE_WITEMS_MSG, -1, false, m_autoSaveTrTmpltWithItemsPref ? MFS_CHECKED : MFS_UNCHECKED);
			break;
		case SNM_SLOT_TYPE_PRJ_TEMPLATES:
			AddToMenu(hAutoSaveSubMenu, "Set auto-save directory to project path (/ProjectTemplates)", AUTOSAVE_DIR_PRJ_MSG);
			break;
	}
	AddToMenu(hMenu, SWS_SEPARATOR, 0);
	AddToMenu(hMenu, "Add slot", ADD_SLOT_MSG, -1, false, !IsFiltered() ? MF_ENABLED : MF_GRAYED);

	int iCol;
	SWS_ListItem* item = m_pLists.Get(0)->GetHitItem(x, y, &iCol);
	PathSlotItem* pItem = (PathSlotItem*)item;
	if (pItem && iCol >= 0)
	{
		UINT enabled = !pItem->IsDefault() ? MF_ENABLED : MF_GRAYED;
		AddToMenu(hMenu, "Insert slot", INSERT_SLOT_MSG, -1, false, !IsFiltered() ? MF_ENABLED : MF_GRAYED);
		AddToMenu(hMenu, "Clear slots", CLEAR_MSG, -1, false, enabled);
		AddToMenu(hMenu, "Delete slots", DEL_SLOTS_MSG);
		AddToMenu(hMenu, "Delete slots and files", DEL_SLOTFILES_MSG);
		AddToMenu(hMenu, SWS_SEPARATOR, 0);
		AddToMenu(hMenu, "Load slot...", LOAD_MSG);
		AddToMenu(hMenu, SWS_SEPARATOR, 0);

		switch(g_type)
		{
			case SNM_SLOT_TYPE_FX_CHAINS:
				AddToMenu(hMenu, "Copy", FXC_COPY_MSG, -1, false, enabled);
				AddToMenu(hMenu, SWS_SEPARATOR, 0);
				AddToMenu(hMenu, FXC_LOAD_APPLY_TRACK_STR, FXC_LOAD_APPLY_TRACK_MSG);
				AddToMenu(hMenu, FXC_LOAD_PASTE_TRACK_STR, FXC_LOAD_PASTE_TRACK_MSG);
				AddToMenu(hMenu, SWS_SEPARATOR, 0);
				AddToMenu(hMenu, FXC_LOAD_APPLY_TAKE_STR, FXC_LOAD_APPLY_TAKE_MSG);
				AddToMenu(hMenu, FXC_LOAD_APPLY_ALL_TAKES_STR, FXC_LOAD_APPLY_ALL_TAKES_MSG);
				AddToMenu(hMenu, FXC_LOAD_PASTE_TAKE_STR, FXC_LOAD_PASTE_TAKE_MSG);
				AddToMenu(hMenu, FXC_LOAD_PASTE_ALL_TAKES_STR, FXC_LOAD_PASTE_ALL_TAKES_MSG);
				break;
			case SNM_SLOT_TYPE_TR_TEMPLATES:
				AddToMenu(hMenu, TRT_LOAD_APPLY_STR, TRT_LOAD_APPLY_MSG);
				AddToMenu(hMenu, TRT_LOAD_IMPORT_STR, TRT_LOAD_IMPORT_MSG);
				AddToMenu(hMenu, SWS_SEPARATOR, 0);
				AddToMenu(hMenu, TRT_LOAD_APPLY_ITEMS_STR, TRT_LOAD_APPLY_ITEMS_MSG);
				AddToMenu(hMenu, TRT_LOAD_PASTE_ITEMS_STR, TRT_LOAD_PASTE_ITEMS_MSG);
				break;
			case SNM_SLOT_TYPE_PRJ_TEMPLATES:
				AddToMenu(hMenu, PRJ_SELECT_LOAD_STR, PRJ_SELECT_LOAD_MSG);
				AddToMenu(hMenu, PRJ_SELECT_LOAD_NEWTAB_STR, PRJ_SELECT_LOAD_NEWTAB_MSG);
				break;
		}
		AddToMenu(hMenu, SWS_SEPARATOR, 0);
#ifdef _WIN32
		AddToMenu(hMenu, "Edit/display file...", EDIT_MSG, -1, false, enabled);
#else
		AddToMenu(hMenu, "Display file...", EDIT_MSG, -1, false, enabled);
#endif
		AddToMenu(hMenu, "Show path in explorer/finder...", EXPLORE_MSG, -1, false, enabled);
	}

	if (g_type == SNM_SLOT_TYPE_PRJ_TEMPLATES) {
		int x=0, nbsel=0; while(m_pLists.Get(0)->EnumSelected(&x))nbsel++;
		AddToMenu(hMenu, SWS_SEPARATOR, 0);
		AddToMenu(hMenu, "Project loader/selecter configuration...", PRJ_LOADER_CONF);
		AddToMenu(hMenu, "Set project loader/selecter from selection", PRJ_LOADER_SET, -1, false, nbsel>1 ? MF_ENABLED : MF_GRAYED);
		AddToMenu(hMenu, "Clear project loader/selecter configuration", PRJ_LOADER_CLEAR, -1, false, isProjectLoaderConfValid() ? MF_ENABLED : MF_GRAYED);
	}

	AddToMenu(hMenu, SWS_SEPARATOR, 0);

	HMENU hFilterSubMenu = CreatePopupMenu();
	AddSubMenu(hMenu, hFilterSubMenu, "Filter on");
	AddToMenu(hFilterSubMenu, "Name", FILTER_BY_NAME_MSG, -1, false, g_filterPref == 0 ? MFS_CHECKED : MFS_UNCHECKED);
	AddToMenu(hFilterSubMenu, "Path (directory only)", FILTER_BY_PATH_MSG, -1, false, g_filterPref == 1 ? MFS_CHECKED : MFS_UNCHECKED);
	AddToMenu(hFilterSubMenu, "Comment", FILTER_BY_DESC_MSG, -1, false, g_filterPref == 2 ? MFS_CHECKED : MFS_UNCHECKED);

	return hMenu;
}

void SNM_ResourceWnd::OnDestroy() 
{
	// save prefs
	char cTmp[6];
	sprintf(cTmp, "%d", m_cbType.GetCurSel());
	WritePrivateProfileString("RESOURCE_VIEW", "Type", cTmp, g_SNMiniFilename.Get());

	sprintf(cTmp, "%d", g_dblClickType[SNM_SLOT_TYPE_FX_CHAINS]);
	WritePrivateProfileString("RESOURCE_VIEW", "DblClick_Type", cTmp, g_SNMiniFilename.Get());

	sprintf(cTmp, "%d", g_dblClickType[SNM_SLOT_TYPE_TR_TEMPLATES]);
	WritePrivateProfileString("RESOURCE_VIEW", "DblClick_Type_Tr_Template", cTmp, g_SNMiniFilename.Get());

	sprintf(cTmp, "%d", g_dblClickType[SNM_SLOT_TYPE_PRJ_TEMPLATES]);
	WritePrivateProfileString("RESOURCE_VIEW", "DblClick_Type_Prj_Template", cTmp, g_SNMiniFilename.Get());

	sprintf(cTmp, "%d", g_dblClickTo);
	WritePrivateProfileString("RESOURCE_VIEW", "DblClick_To", cTmp, g_SNMiniFilename.Get()); 

	sprintf(cTmp, "%d", m_autoSaveFXChainPref);
	WritePrivateProfileString("RESOURCE_VIEW", "AutoSaveFXChain", cTmp, g_SNMiniFilename.Get()); 

	WritePrivateProfileString("RESOURCE_VIEW", "AutoSaveTrTemplateWithItems", m_autoSaveTrTmpltWithItemsPref ? "1" : "0", g_SNMiniFilename.Get()); 

	sprintf(cTmp, "%d", g_filterPref);
	WritePrivateProfileString("RESOURCE_VIEW", "FilterByPath", cTmp, g_SNMiniFilename.Get()); 

	sprintf(cTmp, "%d", g_prjLoaderStartPref);
	WritePrivateProfileString("RESOURCE_VIEW", "ProjectLoaderStartSlot", cTmp, g_SNMiniFilename.Get()); 
	sprintf(cTmp, "%d", g_prjLoaderEndPref);
	WritePrivateProfileString("RESOURCE_VIEW", "ProjectLoaderEndSlot", cTmp, g_SNMiniFilename.Get()); 

	// auto-save & auto-fill directories
	{
		WDL_FastString escapedStr;
		makeEscapedConfigString(g_autoSaveDirs.Get(0)->Get(), &escapedStr);
		WritePrivateProfileString("RESOURCE_VIEW", "AutoSaveDirFXChain", escapedStr.Get(), g_SNMiniFilename.Get()); 
		makeEscapedConfigString(g_autoSaveDirs.Get(1)->Get(), &escapedStr);
		WritePrivateProfileString("RESOURCE_VIEW", "AutoSaveDirTrTemplate", escapedStr.Get(), g_SNMiniFilename.Get()); 
		makeEscapedConfigString(g_autoSaveDirs.Get(2)->Get(), &escapedStr);
		WritePrivateProfileString("RESOURCE_VIEW", "AutoSaveDirPrjTemplate", escapedStr.Get(), g_SNMiniFilename.Get()); 

		makeEscapedConfigString(g_autoFillDirs.Get(0)->Get(), &escapedStr);
		WritePrivateProfileString("RESOURCE_VIEW", "AutoFillDirFXChain", escapedStr.Get(), g_SNMiniFilename.Get());
		makeEscapedConfigString(g_autoFillDirs.Get(1)->Get(), &escapedStr);
		WritePrivateProfileString("RESOURCE_VIEW", "AutoFillDirTrTemplate", escapedStr.Get(), g_SNMiniFilename.Get()); 
		makeEscapedConfigString(g_autoFillDirs.Get(2)->Get(), &escapedStr);
		WritePrivateProfileString("RESOURCE_VIEW", "AutoFillDirPrjTemplate", escapedStr.Get(), g_SNMiniFilename.Get());
	}

	m_cbType.Empty();
	m_cbDblClickType.Empty();
	m_cbDblClickTo.Empty();
	m_parentVwnd.RemoveAllChildren(false);
	m_parentVwnd.SetRealParent(NULL);
}

int SNM_ResourceWnd::OnKey(MSG* msg, int iKeyState) 
{
	if (msg->message == WM_KEYDOWN && msg->wParam == VK_DELETE && !iKeyState)
	{
		DeleteSelectedSlots(true);
		return 1;
	}
	return 0;
}

int SNM_ResourceWnd::GetValidDroppedFilesCount(HDROP _h)
{
	int validCnt=0;
	int iFiles = DragQueryFile(_h, 0xFFFFFFFF, NULL, 0);
	char cFile[BUFFER_SIZE];
	for (int i = 0; i < iFiles; i++) 
	{
		DragQueryFile(_h, i, cFile, BUFFER_SIZE);

		// empty slot d'n'd ?
		if (!strcmp(cFile, DRAGNDROP_EMPTY_SLOT_FILE))
			validCnt++;
		// .rfxchain? .rTrackTemplate? etc..
		else 
		{
			char* pExt = strrchr(cFile, '.');
			if (pExt && !_stricmp(pExt+1, GetCurList()->GetFileExt())) 
				validCnt++;
		}
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
	int dropSlot = GetCurList()->Find(pItem);

	// internal d'n'd ?
	if (g_dragPathSlotItems.GetSize())
	{
		int srcSlot = GetCurList()->Find(g_dragPathSlotItems.Get(0));
		// drag'n'drop slot to the bottom
		if (srcSlot >= 0 && srcSlot < dropSlot)
			dropSlot++; // d'n'd will be more 'natural'
	}

	// drop but not on a slot => create slots
	if (!pItem || dropSlot < 0 || dropSlot >= GetCurList()->GetSize()) 
	{
		dropSlot = GetCurList()->GetSize();
		for (int i = 0; i < iValidFiles; i++)
			GetCurList()->Add(new PathSlotItem());
	}
	// drop on a slot => insert need slots at drop point
	else 
	{
		for (int i = 0; i < iValidFiles; i++)
			GetCurList()->InsertSlot(dropSlot);
	}

	// re-sync pItem 
	pItem = GetCurList()->Get(dropSlot); 

	// Patch added/inserted slots from dropped data
	char cFile[BUFFER_SIZE];
	int slot;
	for (int i = 0; pItem && i < iFiles; i++)
	{
		slot = GetCurList()->Find(pItem);
		DragQueryFile(_h, i, cFile, BUFFER_SIZE);

		// internal d'n'd ?
		if (g_dragPathSlotItems.GetSize())
		{
			PathSlotItem* item = GetCurList()->Get(slot);
			if (item)
			{
				item->m_shortPath.Set(g_dragPathSlotItems.Get(i)->m_shortPath.Get());
				item->m_desc.Set(g_dragPathSlotItems.Get(i)->m_desc.Get());
				dropped++;
				pItem = GetCurList()->Get(slot+1); 
			}
		}
		// .rfxchain? .rTrackTemplate? etc..
		else 
		{
			char* pExt = strrchr(cFile, '.');
			if (pExt && !_stricmp(pExt+1, GetCurList()->GetFileExt())) 
			{ 		
				if (GetCurList()->SetFromFullPath(slot, cFile))
				{
					dropped++;
					pItem = GetCurList()->Get(slot+1); 
				}
			}
		}
	}

	if (dropped)
	{
		// internal drag'n'drop: move (=> delete previous slots)
		for (int i=0; i < g_dragPathSlotItems.GetSize(); i++)
			for (int j=GetCurList()->GetSize()-1; j >= 0; j--)
				if (GetCurList()->Get(j) == g_dragPathSlotItems.Get(i))
				{
					if (j < dropSlot) dropSlot--;
					GetCurList()->Delete(j, false);
				}

		Update();

		// Select item at drop point
		if (dropSlot >= 0)
			SelectBySlot(dropSlot);
	}

	g_dragPathSlotItems.Empty(false);
	DragFinish(_h);
}

void SNM_ResourceWnd::DrawControls(LICE_IBitmap* _bm, RECT* _r)
{
	if (!_bm)
		return;

	LICE_CachedFont* font = SNM_GetThemeFont();
	IconTheme* it = (IconTheme*)GetIconThemeStruct(NULL);// returns the whole icon theme (icontheme.h) and the size
	int x0=_r->left+10, h=35;

	// defines a new rect 'r' that takes the filter edit box into account (contrary to '_r')
	RECT r;
	GetWindowRect(GetDlgItem(m_hwnd, IDC_FILTER), &r);
	ScreenToClient(m_hwnd, (LPPOINT)&r);
	ScreenToClient(m_hwnd, ((LPPOINT)&r)+1);
	r.top = _r->top; r.bottom = _r->bottom;
	r.right = r.left; r.left = _r->left;

	//"auto-save" button (common)
	WDL_VirtualIconButton_SkinConfig* img = it ? &(it->toolbar_save) : NULL;
	if (img)
		m_btnAutoSave.SetIcon(img);
	else {
		m_btnAutoSave.SetTextLabel("Auto-save", 0, font);
		m_btnAutoSave.SetForceBorder(true);
	}
	if (!SetVWndAutoPosition(&m_btnAutoSave, NULL, &r, &x0, _r->top, h))
		return;

	// type dropdown
	m_cbType.SetFont(font);
	if (!SetVWndAutoPosition(&m_cbType, NULL, &r, &x0, _r->top, h))
		return;

	// "dbl-click to:" (common)
	m_txtDblClickType.SetFont(font);
	if (!SetVWndAutoPosition(&m_txtDblClickType, NULL, &r, &x0, _r->top, h, 5))
		return;

	m_cbDblClickType.SetFont(font);
	if (!SetVWndAutoPosition(&m_cbDblClickType, &m_txtDblClickType, &r, &x0, _r->top, h))
		return;

	// "to selected:" (fx chain only)
	m_txtDblClickTo.SetVisible(g_type == SNM_SLOT_TYPE_FX_CHAINS);
	if (g_type == SNM_SLOT_TYPE_FX_CHAINS)
	{
		m_txtDblClickTo.SetFont(font);
		if (!SetVWndAutoPosition(&m_txtDblClickTo, NULL, &r, &x0, _r->top, h, 5))
			return;
	}

	m_cbDblClickTo.SetVisible(g_type == SNM_SLOT_TYPE_FX_CHAINS);
	if (g_type == SNM_SLOT_TYPE_FX_CHAINS)
	{
		m_cbDblClickTo.SetFont(font);
		if (!SetVWndAutoPosition(&m_cbDblClickTo, &m_txtDblClickTo, &r, &x0, _r->top, h))
			return;
	}

/*JFB MFC filter edit box instead
	AddSnMLogo(_bm, _r, x0, h);
*/
}

int SNM_ResourceWnd::OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_PAINT:
		{
			SNM_ThemeListView(m_pLists.Get(0), false);

			int xo, yo; RECT r;
			GetClientRect(m_hwnd, &r);	
			m_parentVwnd.SetPosition(&r);
			m_vwnd_painter.PaintBegin(m_hwnd, WDL_STYLE_GetSysColor(COLOR_WINDOW));
			DrawControls(m_vwnd_painter.GetBuffer(&xo, &yo), &r);
			m_vwnd_painter.PaintVirtWnd(&m_parentVwnd);
			m_vwnd_painter.PaintEnd();
		}
		break;
		case WM_LBUTTONDOWN:
			SetFocus(m_hwnd);
			if (m_parentVwnd.OnMouseDown(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)) > 0)
				SetCapture(m_hwnd);
			break;
		case WM_LBUTTONUP:
			if (GetCapture() == m_hwnd) {
				m_parentVwnd.OnMouseUp(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
				ReleaseCapture();
			}
			break;
		case WM_MOUSEMOVE:
			m_parentVwnd.OnMouseMove(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
			break;
#ifdef _SNM_THEMABLE
		case WM_CTLCOLOREDIT:
			if ((HWND)lParam == GetDlgItem(m_hwnd, IDC_EDIT) || (HWND)lParam == GetDlgItem(m_hwnd, IDC_FILTER)) {
				int bg, txt; SNM_GetThemeWinColors(&bg, &txt);
				SetBkColor((HDC)wParam, bg);
				SetTextColor((HDC)wParam, txt);
				return (INT_PTR)SNM_GetThemeBrush();
			}
			break;
#endif
	}
	return 0;
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
		for (int i = 0; i < lv->GetListItemCount(); i++)
		{
			PathSlotItem* item = (PathSlotItem*)lv->GetListItem(i);
			int slot = GetCurList()->Find(item);
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
	int idx = GetCurList()->GetSize();
	if (GetCurList()->Add(new PathSlotItem()) && _update) {
		Update();
		SelectBySlot(idx);
	}
}

void SNM_ResourceWnd::InsertAtSelectedSlot(bool _update)
{
	if (GetCurList()->GetSize())
	{
		bool updt = false;
		PathSlotItem* item = (PathSlotItem*)m_pLists.Get(0)->EnumSelected(NULL);
		if (item)
		{
			int slot = GetCurList()->Find(item);
			if (slot >= 0)
				updt = (GetCurList()->InsertSlot(slot) != NULL);
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

void SNM_ResourceWnd::DeleteSelectedSlots(bool _update, bool _delFiles)
{
	// DeleteOnDestroy list: keep (displayed!) pointers until the list view is updated
	FileSlotList deItems(GetCurList()->GetType(), GetCurList()->GetResourceDir(), GetCurList()->GetDesc(), GetCurList()->GetFileExt());
	int x=0;
	PathSlotItem* item;
	while((item = (PathSlotItem*)m_pLists.Get(0)->EnumSelected(&x)))
	{
		int slot = GetCurList()->Find(item);
		if (slot >= 0) 
		{
			if (_delFiles)
			{
				char fullPath[BUFFER_SIZE] = "";
				GetFullResourcePath(GetCurList()->GetResourceDir(), item->m_shortPath.Get(), fullPath, BUFFER_SIZE);
				SNM_DeleteFile(fullPath);
			}
			deItems.Add(item);
			GetCurList()->Delete(slot, false); // no delete yet
		}
	}
	if (_update && deItems.GetSize())
		Update();
} // + deItems auto-clean-up !

void SNM_ResourceWnd::AutoSave()
{
	if (!FileExists(GetCurAutoSaveDir()->Get()))
	{
		char msg[BUFFER_SIZE] = "";
		_snprintf(msg, BUFFER_SIZE, "Auto-save directory not found:\n%s\n\nDo you want to define one ?", GetCurAutoSaveDir()->Get());
		switch(MessageBox(g_hwndParent, msg, "S&M - Error", MB_YESNO)) {
			case IDYES: OnCommand(AUTOSAVE_DIR_MSG, 0); break;
			case IDNO: return;
		}
		if (!FileExists(GetCurAutoSaveDir()->Get())) // re-check: the user may have cancelled..
			return;
	}

	bool slotUpdate = false;
	int slotStart = GetCurList()->GetSize();
	char fn[BUFFER_SIZE] = "";
	switch(g_type) 
	{
		case SNM_SLOT_TYPE_FX_CHAINS:
			switch (m_autoSaveFXChainPref)
			{
				case FXC_AUTOSAVE_PREF_INPUT_FX:
					slotUpdate = autoSaveTrackFXChainSlots(g_bv4, GetCurAutoSaveDir()->Get(), fn, BUFFER_SIZE);
					break;
				case FXC_AUTOSAVE_PREF_TRACK:
					slotUpdate = autoSaveTrackFXChainSlots(false, GetCurAutoSaveDir()->Get(), fn, BUFFER_SIZE);
					break;
				case FXC_AUTOSAVE_PREF_ITEM:
					slotUpdate = autoSaveItemFXChainSlots(GetCurAutoSaveDir()->Get(), fn, BUFFER_SIZE);
					break;
			}
			break;
		case SNM_SLOT_TYPE_TR_TEMPLATES:
			slotUpdate = autoSaveTrackSlots(!m_autoSaveTrTmpltWithItemsPref, true /*JFB!!!*/, GetCurAutoSaveDir()->Get(), fn, BUFFER_SIZE);
			break;
		case SNM_SLOT_TYPE_PRJ_TEMPLATES:
			slotUpdate = autoSaveProjectSlot(true, GetCurAutoSaveDir()->Get(), fn, BUFFER_SIZE);
		break;
	}

	if (slotUpdate) 
	{
		Update();
		SelectBySlot(slotStart, GetCurList()->GetSize());
	}
	else
	{
		char msg[BUFFER_SIZE];
		if (*fn) _snprintf(msg, BUFFER_SIZE, "Auto-save failed: %s\n%s", fn, AUTO_SAVE_ERR_STR);
		else _snprintf(msg, BUFFER_SIZE, "Auto-save failed!\n%s", AUTO_SAVE_ERR_STR);
		MessageBox(g_hwndParent, msg, "S&M - Error", MB_OK);
	}
}

void SNM_ResourceWnd::AutoFill(const char* _startPath)
{
	if (_startPath && *_startPath)
	{
		//JFB TODO get rid of that => WDL instead
		vector<string> files; 
		SearchDirectory(files, _startPath, GetCurList()->GetFileExt(), true);

		int sz = (int)files.size();
		if (sz) {
			int startSlot = GetCurList()->GetSize();
			for (int i=0; i < sz; i++)				
				if (GetCurList()->FindByResFulltPath(files[i].c_str()) < 0) // skip if already present
					GetCurList()->AddSlot(files[i].c_str());
			if (startSlot != GetCurList()->GetSize()) {
				Update();
				SelectBySlot(startSlot, GetCurList()->GetSize());
			}
		}
		else {
			char errMsg[BUFFER_SIZE] = "";
			_snprintf(errMsg, 512, "%s is empty or does not exist!", _startPath);
			MessageBox(GetMainHwnd(), errMsg, "S&M - Warning", MB_OK);
		}
	}
}


///////////////////////////////////////////////////////////////////////////////

int ResourceViewInit()
{
	// Init the lists of files (ordered by g_type)
	g_filesLists.Empty();
	g_filesLists.Add(&g_fxChainFiles);
	g_filesLists.Add(&g_trTemplateFiles);
	g_filesLists.Add(&g_prjTemplateFiles);

	// read slots from ini file
	char path[BUFFER_SIZE] = "";
	char desc[128], maxSlotCount[16];
	for (int i=0; i < g_filesLists.GetSize(); i++)
	{
		FileSlotList* list = g_filesLists.Get(i);
		if (list)
		{
			GetPrivateProfileString(list->GetResourceDir(), "MAX_SLOT", "0", maxSlotCount, 16, g_SNMiniFilename.Get()); 
			list->EmptySafe(true);
			int slotCount = atoi(maxSlotCount);
			for (int j=0; j < slotCount; j++) 
			{
				readSlotIniFile(list->GetResourceDir(), j, path, BUFFER_SIZE, desc, 128);
				list->Add(new PathSlotItem(path, desc));
			}
		}
	}

	g_pResourcesWnd = new SNM_ResourceWnd();
	if (!g_pResourcesWnd)
		return 0;
	return 1;
}

void ResourceViewExit()
{
	// save slots to ini file
	for (int i=0; i < g_filesLists.GetSize(); i++)
	{
		FileSlotList* list = g_filesLists.Get(i);
		if (list)
		{
			PathSlotItem* item;
			const char* resDir = list->GetResourceDir();
			WDL_FastString iniSection;
			iniSection.SetFormatted(32, "MAX_SLOT=%d\n", list->GetSize());
			for (int j=0; j < list->GetSize(); j++)
			{
				item = list->Get(j);
				if (item)
				{
					WDL_FastString escapedStr;
					if (item->m_shortPath.GetLength())
					{
						makeEscapedConfigString(item->m_shortPath.Get(), &escapedStr);
						iniSection.AppendFormatted(BUFFER_SIZE, "SLOT%d=%s\n", j+1, escapedStr.Get()); 
					}
					if (item->m_desc.GetLength())
					{
						makeEscapedConfigString(item->m_desc.Get(), &escapedStr);
						iniSection.AppendFormatted(BUFFER_SIZE, "DESC%d=%s\n", j+1, escapedStr.Get());
					}
				}
			}
			// Write things in one go (avoid to slow down REAPER shutdown)
			SaveIniSection(resDir, &iniSection, g_SNMiniFilename.Get());
		}
	}

	delete g_pResourcesWnd;
	g_pResourcesWnd = NULL;
}

void OpenResourceView(COMMAND_T* _ct) 
{
	if (g_pResourcesWnd) 
	{
		int prevType = g_type;
		if (g_type < 0)
			g_type = (int)_ct->user;
		g_pResourcesWnd->Show((prevType == (int)_ct->user) /* i.e toggle */, true);
		g_pResourcesWnd->SetType((int)_ct->user);
//		SetFocus(GetDlgItem(g_pResourcesWnd->GetHWND(), IDC_FILTER));
	}
}

bool IsResourceViewDisplayed(COMMAND_T* _ct) {
	return (g_pResourcesWnd && g_pResourcesWnd->IsValidWindow());
}

void ResourceViewClearSlotPrompt(COMMAND_T* _ct) {
	g_filesLists.Get((int)_ct->user)->ClearSlotPrompt(_ct);
}

//JFB TO DO (requires slight refactoring..)
void ResourceViewAutoSave(COMMAND_T* _ct) {
}