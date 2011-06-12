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


#define NB_SLOTS_FAST_LISTVIEW			500

// Commands
#define AUTO_INSERT_SLOTS				0x110000 // common cmds
#define CLEAR_MSG						0x110001
#define DEL_SLOTS_MSG					0x110002
#define DEL_SLOTFILES_MSG				0x110003
#define ADD_SLOT_MSG					0x110004
#define INSERT_SLOT_MSG					0x110005
#define EDIT_MSG						0x110006
#define EXPLORE_MSG						0x110007
#define LOAD_MSG						0x110008
#define AUTOSAVE_DIR_MSG				0x110009
#define FILTER_BY_PATH_MSG				0x11000A
#define FXC_LOAD_APPLY_TRACK_MSG		0x11000B // specific FX chains cmds
#define FXC_LOAD_APPLY_TAKE_MSG			0x11000C
#define FXC_LOAD_APPLY_ALL_TAKES_MSG	0x11000D
#define FXC_COPY_MSG					0x11000E
#define FXC_LOAD_PASTE_TRACK_MSG		0x11000F
#define FXC_LOAD_PASTE_TAKE_MSG			0x110010
#define FXC_LOAD_PASTE_ALL_TAKES_MSG	0x110011
#define FXC_AUTO_SAVE_INPUT_FX			0x110012
#define FXC_AUTO_SAVE_TRACK				0x110013
#define FXC_AUTO_SAVE_ITEM				0x110014
#define TRT_LOAD_APPLY_MSG				0x110015 // specific track template cmds
#define TRT_LOAD_IMPORT_MSG				0x110016
#define TRT_LOAD_APPLY_ITEMS_MSG		0x110017
#define TRT_LOAD_PASTE_ITEMS_MSG		0x110018
#define TRT_AUTO_SAVE_WITEMS_MSG		0x110019
#define PRJ_SELECT_LOAD_MSG				0x11001A
#define PRJ_SELECT_LOAD_NEWTAB_MSG		0x11001B
#define PRJ_AUTO_SAVE_MSG				0x11001C
#define AUTOSAVE_DIR_PRJ_MSG			0x11001D
#define AUTOSAVE_DIR_DEFAULT_MSG		0x11001E

// labels shared by actions and popup menu items
#define FXC_LOAD_APPLY_TRACK_STR		"Load/apply FX chain to selected tracks"
#define FXCIN_LOAD_APPLY_TRACK_STR		"Load/apply input FX chain to selected tracks"
#define FXC_LOAD_APPLY_TAKE_STR			"Load/apply FX chain to selected items"
#define FXC_LOAD_APPLY_ALL_TAKES_STR	"Load/apply FX chain to selected items, all takes"
#define FXC_LOAD_PASTE_TRACK_STR		"Load/paste FX chain to selected tracks"
#define FXC_LOAD_PASTE_TAKE_STR			"Load/paste FX chain to selected items"
#define FXC_LOAD_PASTE_ALL_TAKES_STR	"Load/paste FX chain to selected items, all takes"
#define TRT_LOAD_APPLY_STR				"Load/apply track template to selected tracks (w/o items)"
#define TRT_LOAD_IMPORT_STR				"Load/import tracks from track template (w/ items)"
#define TRT_LOAD_APPLY_ITEMS_STR		"Load/replace selected tracks' items w/ track template ones"
#define TRT_LOAD_PASTE_ITEMS_STR		"Load/paste track template's items to selected tracks"
#define PRJ_SELECT_LOAD_STR				"Select/load project template"
#define PRJ_SELECT_LOAD_NEWTAB_STR		"Select/load project template (new tab)"

#define DRAGNDROP_EMPTY_SLOT_FILE		">Empty<"
#define FILTER_DEFAULT_STR				"Filter"

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


// Globals
static SNM_ResourceWnd* g_pResourcesWnd = NULL;
static SWS_LVColumn g_fxChainListCols[] = { {65,2,"Slot"}, {100,1,"Name"}, {250,2,"Path"}, {200,1,"Comment"} };

FileSlotList g_fxChainFiles(SNM_SLOT_TYPE_FX_CHAINS, "FXChains", "FX chain", "RfxChain");
FileSlotList g_trTemplateFiles(SNM_SLOT_TYPE_TR_TEMPLATES, "TrackTemplates", "track template", "RTrackTemplate");
FileSlotList g_prjTemplateFiles(SNM_SLOT_TYPE_PRJ_TEMPLATES, "ProjectTemplates", "project template", "RPP");

// shared between the list view & the wnd -------------------------------------
int g_type = -1;

WDL_String g_filter(FILTER_DEFAULT_STR);
int g_autoSaveFXChainPref = FXC_AUTOSAVE_PREF_TRACK;
bool g_filterByPathPref = true; // false: filter by comment

int g_dblClickType[SNM_SLOT_TYPE_COUNT];
int g_dblClickTo = 0; // for fx chains only

WDL_PtrList<PathSlotItem> g_dragPathSlotItems; 
WDL_PtrList<FileSlotList> g_filesLists;
WDL_PtrList_DeleteOnDestroy<WDL_String> g_autoSaveDirs;
// ----------------------------------------------------------------------------

FileSlotList* GetCurList() {
	return g_filesLists.Get(g_type);
}

WDL_String* GetCurAutoSaveDir() {
	return g_autoSaveDirs.Get(g_type);
}

bool IsFiltered() {
	return (g_filter.GetLength() && strcmp(g_filter.Get(), FILTER_DEFAULT_STR));
}

///////////////////////////////////////////////////////////////////////////////
// SNM_ResourceView
///////////////////////////////////////////////////////////////////////////////

SNM_ResourceView::SNM_ResourceView(HWND hwndList, HWND hwndEdit)
:SWS_ListView(hwndList, hwndEdit, 4, g_fxChainListCols, "Resources View State", false)
{
#ifdef _SNM_THEMABLE
	ListView_SetBkColor(hwndList, GSC_mainwnd(COLOR_WINDOW));
	ListView_SetTextBkColor(hwndList, GSC_mainwnd(COLOR_WINDOW));
	ListView_SetTextColor(hwndList, GSC_mainwnd(COLOR_BTNTEXT));
#endif
}

void SNM_ResourceView::GetItemText(LPARAM item, int iCol, char* str, int iStrMax)
{
	PathSlotItem* pItem = (PathSlotItem*)item;
	if (pItem)
	{
		switch (iCol)
		{
			case 0:
			{
				int slot = GetCurList()->Find(pItem);
				if (slot >= 0) _snprintf(str, iStrMax, "%5.d", slot+1);
				else strcpy(str, "?");
			}
			break;
			case 1:
			{
				char buf[BUFFER_SIZE] = "";
				ExtractFileNameEx(pItem->m_shortPath.Get(), buf, true); //JFB!!! Xen code a revoir
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

void SNM_ResourceView::SetItemText(LPARAM item, int iCol, const char* str)
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
					strncpy(newFn, fn, BUFFER_SIZE);
					char* p = strrchr(newFn, PATH_SLASH_CHAR);
					if (p) *p = '\0';
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
						ListView_SetItemText(m_hwndList, GetEditingItem(), DisplayToDataCol(2), pItem->m_shortPath.Get());
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


void SNM_ResourceView::OnItemDblClk(LPARAM item, int iCol)
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
						applyOrImportTrackSlot(TRT_LOAD_APPLY_STR, false, slot, false, !wasDefaultSlot);
						break;
					case 1:
						applyOrImportTrackSlot(TRT_LOAD_IMPORT_STR, true, slot, false, !wasDefaultSlot);
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

void SNM_ResourceView::GetItemList(WDL_TypedBuf<LPARAM>* pBuf)
{
	if (IsFiltered())
	{
		int iCount = 0;
		LineParser lp(false);
		lp.parse(g_filter.Get());
		for (int i = 0; i < GetCurList()->GetSize(); i++)
		{
			PathSlotItem* item = GetCurList()->Get(i);
			if (item && (!g_filterByPathPref || (g_filterByPathPref && !item->IsDefault())))
			{
				bool match = true;
				for (int j=0; match && j < lp.getnumtokens(); j++)
					match &= (stristr(g_filterByPathPref ? item->m_shortPath.Get() : item->m_desc.Get(), lp.gettoken_str(j)) != NULL);
				if (match) {
					pBuf->Resize(++iCount);
					pBuf->Get()[iCount-1] = (LPARAM)item;
				}
			}
		}
	}
	else
	{
		pBuf->Resize(GetCurList()->GetSize());
		for (int i = 0; i < pBuf->GetSize(); i++)
			pBuf->Get()[i] = (LPARAM)GetCurList()->Get(i);
	}
}

void SNM_ResourceView::OnBeginDrag(LPARAM _item)
{
#ifdef _WIN32
	g_dragPathSlotItems.Empty(false);

	// Store dragged items (for internal d'n'd) + full paths + get the amount of memory needed
	int iMemNeeded = 0, x=0;
	WDL_PtrList_DeleteOnDestroy<WDL_String> fullPaths;
	PathSlotItem* pItem = (PathSlotItem*)EnumSelected(&x);
	while(pItem) {
		int slot = GetCurList()->Find(pItem);
		char fullPath[BUFFER_SIZE] = "";
		if (GetCurList()->GetFullPath(slot, fullPath, BUFFER_SIZE))
		{
			bool empty = (pItem->IsDefault() || *fullPath == '\0');
			iMemNeeded += (int)((empty ? strlen(DRAGNDROP_EMPTY_SLOT_FILE) : strlen(fullPath)) + 1);
			fullPaths.Add(new WDL_String(empty ? DRAGNDROP_EMPTY_SLOT_FILE : fullPath));
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
// SNM_FastResourceView
///////////////////////////////////////////////////////////////////////////////

// "Brutal force" update to make it run much faster, but subbtle thing 
// like selection restorations aren't managed anymore..

//JFB!!! to be kept in sync with SWS_ListView::Update(), last check in SWS v2.0#26
void SNM_FastResourceView::Update()
{
	// Fill in the data by pulling it from the derived class
	if (m_iEditingItem == -1 && !m_bDisableUpdates)
	{
		m_bDisableUpdates = true;
		char str[256];

		WDL_TypedBuf<LPARAM> items;
		GetItemList(&items);

/*JFB original code
		// Check for deletions - items in the lstwnd are quite likely out of order so gotta do a full O(n^2) search
		int lvItemCount = ListView_GetItemCount(m_hwndList);
		for (int i = 0; i < lvItemCount; i++)
		{
			LPARAM item = GetListItem(i);
			bool bFound = false;
			for (int j = 0; j < items.GetSize(); j++)
				if (items.Get()[j] == item)
				{
					bFound = true;
					break;
				}

			if (!bFound)
			{
				ListView_DeleteItem(m_hwndList, i--);
				lvItemCount--;
			}
		}

		// Check for additions
		lvItemCount = ListView_GetItemCount(m_hwndList);
		for (int i = 0; i < items.GetSize(); i++)
		{
			bool bFound = false;
			int j;
			for (j = 0; j < lvItemCount; j++)
			{
				if (items.Get()[i] == GetListItem(j))
				{
					bFound = true;
					break;
				}
			}

			// Update the list, no matter what, because text may have changed
			LVITEM item;
			item.mask = 0;
			int iNewState = GetItemState(items.Get()[i]);
			if (iNewState >= 0)
			{
				int iCurState = bFound ? ListView_GetItemState(m_hwndList, j, LVIS_SELECTED | LVIS_FOCUSED) : 0;
				if (iNewState && !(iCurState & LVIS_SELECTED))
				{
					item.mask |= LVIF_STATE;
					item.state = LVIS_SELECTED;
					item.stateMask = LVIS_SELECTED;
				}
				else if (!iNewState && (iCurState & LVIS_SELECTED))
				{
					item.mask |= LVIF_STATE;
					item.state = 0;
					item.stateMask = LVIS_SELECTED | ((iCurState & LVIS_FOCUSED) ? LVIS_FOCUSED : 0);
				}
			}

			item.iItem = j;
			item.pszText = str;

			int iCol = 0;
			for (int k = 0; k < m_iCols; k++)
				if (m_pCols[k].iPos != -1)
				{
					item.iSubItem = iCol;
					GetItemText(items.Get()[i], k, str, 256);
					if (!iCol && !bFound)
					{
						item.mask |= LVIF_PARAM | LVIF_TEXT;
						item.lParam = items.Get()[i];
						ListView_InsertItem(m_hwndList, &item);
						lvItemCount++;
					}
					else
					{
						char curStr[256];
						ListView_GetItemText(m_hwndList, j, iCol, curStr, 256);
						if (strcmp(str, curStr))
							item.mask |= LVIF_TEXT;
						if (item.mask)
						{
							// Only set if there's changes
							// May be less efficient here, but less messages get sent for sure!
							ListView_SetItem(m_hwndList, &item);
						}
					}
					item.mask = 0;
					iCol++;
				}
		}
*/

//JFB mod -------------------------------------------------------------------->
		ListView_DeleteAllItems(m_hwndList);
		for (int i = 0; i < items.GetSize(); i++)
		{
			LVITEM item;
			item.mask = 0;
			item.iItem = i;
			item.pszText = str;

			int iCol = 0;
			for (int k = 0; k < m_iCols; k++)
				if (m_pCols[k].iPos != -1)
				{
					item.iSubItem = iCol;
					GetItemText(items.Get()[i], k, str, 256);
					if (!iCol)
					{
						item.mask |= LVIF_PARAM | LVIF_TEXT;
						item.lParam = items.Get()[i];
						ListView_InsertItem(m_hwndList, &item);
					}
					else
					{
						item.mask |= LVIF_TEXT;
						ListView_SetItem(m_hwndList, &item);
					}
					item.mask = 0;
					iCol++;
				}
		}
//JFB mod <--------------------------------------------------------------------

		ListView_SortItems(m_hwndList, sListCompare, (LPARAM)this);
		int iCol = abs(m_iSortCol) - 1;
		iCol = DataToDisplayCol(iCol) + 1;
		if (m_iSortCol < 0)
			iCol = -iCol;
		SetListviewColumnArrows(iCol);

#ifdef _WIN32
		if (m_hwndTooltip)
		{
			TOOLINFO ti = { sizeof(TOOLINFO), };
			ti.lpszText = str;
			ti.hwnd = m_hwndList;
			ti.uFlags = TTF_SUBCLASS;
			ti.hinst  = g_hInst;

			// Delete all existing tools
			while (SendMessage(m_hwndTooltip, TTM_ENUMTOOLS, 0, (LPARAM)&ti))
				SendMessage(m_hwndTooltip, TTM_DELTOOL, 0, (LPARAM)&ti);

			RECT r;
			// Add tooltips after sort
			for (int i = 0; i < ListView_GetItemCount(m_hwndList); i++)
			{
				// Get the rect of the line
				ListView_GetItemRect(m_hwndList, i, &r, LVIR_BOUNDS);
				memcpy(&ti.rect, &r, sizeof(RECT));
				ti.uId = i;
				GetItemTooltip(GetListItem(i), str, 100);
				SendMessage(m_hwndTooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);
			}
		}
#endif
		m_bDisableUpdates = false;
	}
}


///////////////////////////////////////////////////////////////////////////////
// SNM_ResourceWnd
///////////////////////////////////////////////////////////////////////////////

SNM_ResourceWnd::SNM_ResourceWnd()
:SWS_DockWnd(IDD_SNM_FXCHAINLIST, "Resources", "SnMResources", 30006, SWSGetCommandID(OpenResourceView))
{
	m_previousType = g_type;
	m_autoSaveTrTmpltWithItemsPref = true;
	m_lastThemeBrushColor = -1;

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
			m_cbDblClickType.AddItem("Load/select project"); //JFB!!!
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
/*JFB!!! Commented: useless since r488 theming updates !???
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
	// "fast listview" if there are more than NB_SLOTS_FAST_LISTVIEW slots, normal otherwise
	// (a fast list view has few drawbacks, e.g. selection not restored after updates)
	bool fastViewNeeded = false;
	for (int i=0; !fastViewNeeded && i < g_filesLists.GetSize(); i++)
		fastViewNeeded = (g_filesLists.Get(i)->GetSize() > NB_SLOTS_FAST_LISTVIEW);
	m_pLists.Add(fastViewNeeded ? new SNM_FastResourceView(GetDlgItem(m_hwnd, IDC_LIST), GetDlgItem(m_hwnd, IDC_EDIT)) : new SNM_ResourceView(GetDlgItem(m_hwnd, IDC_LIST), GetDlgItem(m_hwnd, IDC_EDIT)));

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
	g_filterByPathPref = (GetPrivateProfileInt("RESOURCE_VIEW", "FilterByPath", 1, g_SNMiniFilename.Get()) == 1);
	// auto save directories
	{
		char defaultPath[BUFFER_SIZE] = "", path[BUFFER_SIZE] = "";
		g_autoSaveDirs.Empty(true);
		_snprintf(defaultPath, BUFFER_SIZE, "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, g_filesLists.Get(0)->GetResourceDir());
		GetPrivateProfileString("RESOURCE_VIEW", "AutoSaveDirFXChain", defaultPath, path, BUFFER_SIZE, g_SNMiniFilename.Get());
		g_autoSaveDirs.Add(new WDL_String(path));
		_snprintf(defaultPath, BUFFER_SIZE, "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, g_filesLists.Get(1)->GetResourceDir());
		GetPrivateProfileString("RESOURCE_VIEW", "AutoSaveDirTrTemplate", defaultPath, path, BUFFER_SIZE, g_SNMiniFilename.Get());
		g_autoSaveDirs.Add(new WDL_String(path));
		_snprintf(defaultPath, BUFFER_SIZE, "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, g_filesLists.Get(2)->GetResourceDir());
		GetPrivateProfileString("RESOURCE_VIEW", "AutoSaveDirPrjTemplate", defaultPath, path, BUFFER_SIZE, g_SNMiniFilename.Get());
		g_autoSaveDirs.Add(new WDL_String(path));
	}

	// WDL GUI init
	m_parentVwnd.SetRealParent(m_hwnd);

	m_cbType.SetID(COMBOID_TYPE);
	m_cbType.SetRealParent(m_hwnd);
	m_cbType.AddItem("FX chains");
	m_cbType.AddItem("Track templates");
	m_cbType.AddItem("Project templates");
	m_cbType.SetCurSel(g_type);
	m_parentVwnd.AddChild(&m_cbType);

	m_cbDblClickType.SetID(COMBOID_DBLCLICK_TYPE);
	m_cbDblClickType.SetRealParent(m_hwnd);
	FillDblClickTypeCombo(); //JFB single call to SetType() instead !?
	m_parentVwnd.AddChild(&m_cbDblClickType);

	m_cbDblClickTo.SetID(COMBOID_DBLCLICK_TO);
	m_cbDblClickTo.SetRealParent(m_hwnd);
	m_cbDblClickTo.AddItem("Tracks");
	m_cbDblClickTo.AddItem("Tracks (input FX)");
	m_cbDblClickTo.AddItem("Items");
	m_cbDblClickTo.AddItem("Items (all takes)");
	m_cbDblClickTo.SetCurSel(g_dblClickTo);
	m_parentVwnd.AddChild(&m_cbDblClickTo);

	m_btnAutoSave.SetID(BUTTONID_AUTO_SAVE);
	m_btnAutoSave.SetRealParent(m_hwnd);
	m_parentVwnd.AddChild(&m_btnAutoSave);

	m_txtDblClickType.SetID(TXTID_DBL_TYPE);
	m_txtDblClickType.SetRealParent(m_hwnd);
	m_parentVwnd.AddChild(&m_txtDblClickType);

	m_txtDblClickTo.SetID(TXTID_DBL_TO);
	m_txtDblClickTo.SetRealParent(m_hwnd);
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
			if (GetCurList()->GetFullPath(slot, fullPath, BUFFER_SIZE))	{
				char* p = strrchr(fullPath, PATH_SLASH_CHAR);
				if (p) {
					*(p+1) = '\0'; // ShellExecute() is KO otherwie..
					ShellExecute(NULL, "open", fullPath, NULL, NULL, SW_SHOWNORMAL);
				}
			}
			break;
		}
		case AUTO_INSERT_SLOTS:
		{
			char startPath[BUFFER_SIZE];
			_snprintf(startPath, BUFFER_SIZE, "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, GetCurList()->GetResourceDir());
			vector<string> files; //JFB TODO get rid of that vector 
			SearchDirectory(files, startPath, GetCurList()->GetFileExt(), true);
			if ((int)files.size()) 
			{
				int insertedCount = 0;
				bool noslot = false;
				for (int i=((int)files.size() - 1); i >=0 ; i--)
				{
					// skip if already present (would also be very easy to overflow the view otherwise)
					if (GetCurList()->FindByResFulltPath(files[i].c_str()) < 0)
					{
						if (slot < 0) { // trick: avoid reversed alphabetical order..
							noslot = true;
							GetCurList()->Add(new PathSlotItem());
							slot = GetCurList()->GetSize()-1;
						}
						if (GetCurList()->InsertSlot(slot, files[i].c_str()))
							insertedCount++;
					}
				}

				if (noslot) // .. end of trick
					GetCurList()->Delete(GetCurList()->GetSize()-1); 

				if (insertedCount)
				{
					Update();
					SelectBySlot(slot + insertedCount);
				}
			}
			else 
			{
				char errMsg[BUFFER_SIZE] = "";
				_snprintf(errMsg, 512, "%s is empty or does not exist!", startPath);
				MessageBox(GetMainHwnd(), errMsg, "S&M - Warning", MB_OK);
			}
			break;
		}
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
		case FILTER_BY_PATH_MSG:
			g_filterByPathPref = !g_filterByPathPref;
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
				applyOrImportTrackSlot(wParam == TRT_LOAD_APPLY_MSG ? TRT_LOAD_APPLY_STR : TRT_LOAD_IMPORT_STR, wParam == TRT_LOAD_IMPORT_MSG, slot, false, !wasDefaultSlot);
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

		// ***** Track template *****
		case PRJ_SELECT_LOAD_MSG:
		case PRJ_SELECT_LOAD_NEWTAB_MSG:
			if (item && slot >= 0) {
				loadOrSelectProject(wParam == PRJ_SELECT_LOAD_MSG ? PRJ_SELECT_LOAD_STR : PRJ_SELECT_LOAD_NEWTAB_STR, slot, wParam == PRJ_SELECT_LOAD_NEWTAB_MSG, !wasDefaultSlot);
				if (wasDefaultSlot && !item->IsDefault()) // slot has been filled ?
					Update();
			}
			break;

		// ***** WDL GUI & others *****
		default:
		{
			if (HIWORD(wParam)==0) 
			{
				switch(LOWORD(wParam))
				{
					case BUTTONID_AUTO_SAVE:
						AutoSaveSlots(slot);
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
	AddToMenu(hMenu, "Auto-fill (from resource path)", AUTO_INSERT_SLOTS);
	AddToMenu(hMenu, SWS_SEPARATOR, 0);
	AddToMenu(hMenu, "Filter by path", FILTER_BY_PATH_MSG, -1, false, g_filterByPathPref ? MFS_CHECKED : MFS_UNCHECKED);
	AddToMenu(hMenu, "Filter by comment", FILTER_BY_PATH_MSG, -1, false, g_filterByPathPref ? MFS_UNCHECKED : MFS_CHECKED);
	AddToMenu(hMenu, SWS_SEPARATOR, 0);
	char autoSavePath[BUFFER_SIZE] = "";
	_snprintf(autoSavePath, BUFFER_SIZE, "[Current %ss auto-save path: %s]", GetCurList()->GetDesc(), GetCurAutoSaveDir()->Get());
	AddToMenu(hMenu, autoSavePath, -1, -1, false, MF_GRAYED);
	AddToMenu(hMenu, "Set auto-save directory...", AUTOSAVE_DIR_MSG);
	AddToMenu(hMenu, "Set auto-save directory to default resource path", AUTOSAVE_DIR_DEFAULT_MSG);
	switch(g_type)
	{
		case SNM_SLOT_TYPE_FX_CHAINS:
			AddToMenu(hMenu, "Set auto-save directory to project path (/FXChains)", AUTOSAVE_DIR_PRJ_MSG);
			AddToMenu(hMenu, "Auto-save button: FX chains from track selection", FXC_AUTO_SAVE_TRACK, -1, false, m_autoSaveFXChainPref == FXC_AUTOSAVE_PREF_TRACK ? MFS_CHECKED : MFS_UNCHECKED);
			AddToMenu(hMenu, "Auto-save button: FX chains from item selection", FXC_AUTO_SAVE_ITEM, -1, false, m_autoSaveFXChainPref == FXC_AUTOSAVE_PREF_ITEM ? MFS_CHECKED : MFS_UNCHECKED);
			if (g_bv4)
				AddToMenu(hMenu, "Auto-save button: input FX chains from track selection", FXC_AUTO_SAVE_INPUT_FX, -1, false, m_autoSaveFXChainPref == FXC_AUTOSAVE_PREF_INPUT_FX ? MFS_CHECKED : MFS_UNCHECKED);
			break;
		case SNM_SLOT_TYPE_TR_TEMPLATES:
			AddToMenu(hMenu, "Set auto-save directory to project path (/TrackTemplates)", AUTOSAVE_DIR_PRJ_MSG);
			AddToMenu(hMenu, "Auto-save button: save track templates with items", TRT_AUTO_SAVE_WITEMS_MSG, -1, false, m_autoSaveTrTmpltWithItemsPref ? MFS_CHECKED : MFS_UNCHECKED);
			break;
		case SNM_SLOT_TYPE_PRJ_TEMPLATES:
			AddToMenu(hMenu, "Set auto-save directory to project path (/ProjectTemplates)", AUTOSAVE_DIR_PRJ_MSG);
			break;
	}
	AddToMenu(hMenu, SWS_SEPARATOR, 0);
	AddToMenu(hMenu, "Add slot", ADD_SLOT_MSG, -1, false, !IsFiltered() ? MF_ENABLED : MF_GRAYED);

	int iCol;
	LPARAM item = m_pLists.Get(0)->GetHitItem(x, y, &iCol);
	PathSlotItem* pItem = (PathSlotItem*)item;
	if (pItem && iCol >= 0)
	{
		UINT enabled = !pItem->IsDefault() ? MF_ENABLED : MF_GRAYED;
		AddToMenu(hMenu, "Insert slot", INSERT_SLOT_MSG, -1, false, !IsFiltered() ? MF_ENABLED : MF_GRAYED);
		AddToMenu(hMenu, "Load slot...", LOAD_MSG);
		AddToMenu(hMenu, "Clear slots", CLEAR_MSG, -1, false, enabled);
		AddToMenu(hMenu, "Delete slots", DEL_SLOTS_MSG);
		AddToMenu(hMenu, "Delete slots and files", DEL_SLOTFILES_MSG);
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
		AddToMenu(hMenu, "Edit...", EDIT_MSG, -1, false, enabled);
#else
		AddToMenu(hMenu, "Display...", EDIT_MSG, -1, false, enabled);
#endif
		AddToMenu(hMenu, "Show path in Explorer/Finder", EXPLORE_MSG, -1, false, enabled);
	}
	return hMenu;
}

void SNM_ResourceWnd::OnDestroy() 
{
	// save prefs
	char cTmp[2];
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
	WritePrivateProfileString("RESOURCE_VIEW", "FilterByPath", g_filterByPathPref ? "1" : "0", g_SNMiniFilename.Get()); 
	{
		WDL_String escapedStr;
		makeEscapedConfigString(g_autoSaveDirs.Get(0)->Get(), &escapedStr);
		WritePrivateProfileString("RESOURCE_VIEW", "AutoSaveDirFXChain", escapedStr.Get(), g_SNMiniFilename.Get()); 
		makeEscapedConfigString(g_autoSaveDirs.Get(1)->Get(), &escapedStr);
		WritePrivateProfileString("RESOURCE_VIEW", "AutoSaveDirTrTemplate", escapedStr.Get(), g_SNMiniFilename.Get()); 
		makeEscapedConfigString(g_autoSaveDirs.Get(2)->Get(), &escapedStr);
		WritePrivateProfileString("RESOURCE_VIEW", "AutoSaveDirPrjTemplate", escapedStr.Get(), g_SNMiniFilename.Get()); 
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
			dropSlot++; //JFB!!! d'n'd will be more 'natural'
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
	//JFB!!! OK on OSX !??
	RECT r;
	GetWindowRect(GetDlgItem(m_hwnd, IDC_FILTER), &r);
	ScreenToClient(m_hwnd, (LPPOINT)&r);
	ScreenToClient(m_hwnd, ((LPPOINT)&r)+1);
	r.top = _r->top; r.bottom = _r->bottom;
	r.right = r.left; r.left = _r->left;

	// type dropdown
	m_cbType.SetFont(font);
	if (!SetVWndAutoPosition(&m_cbType, NULL, &r, &x0, _r->top, h))
		return;

	// "dbl-click to:" (common)
	m_txtDblClickType.SetFont(font);
	m_txtDblClickType.SetText("Dbl-click to:");
	if (!SetVWndAutoPosition(&m_txtDblClickType, NULL, &r, &x0, _r->top, h, 5))
		return;

	m_cbDblClickType.SetFont(font);
	if (!SetVWndAutoPosition(&m_cbDblClickType, &m_txtDblClickType, &r, &x0, _r->top, h))
		return;

	// "To selected:" (fx chain only)
	m_txtDblClickTo.SetVisible(g_type == SNM_SLOT_TYPE_FX_CHAINS);
	if (g_type == SNM_SLOT_TYPE_FX_CHAINS)
	{
		m_txtDblClickTo.SetFont(font);
		m_txtDblClickTo.SetText("To selected:");
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

	// common "auto-save/insert" control
	WDL_VirtualIconButton_SkinConfig* img = it ? &(it->toolbar_save) : NULL;
	if (img)
		m_btnAutoSave.SetIcon(img);
	else {
		m_btnAutoSave.SetTextLabel("Auto-save", 0, font);
		m_btnAutoSave.SetForceBorder(true);
	}
	if (!SetVWndAutoPosition(&m_btnAutoSave, NULL, &r, &x0, _r->top, h))
		return;

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
#ifdef _SNM_THEMABLE
			int winCol = GSC_mainwnd(COLOR_WINDOW);
			if (m_lastThemeBrushColor != winCol) 
			{
				m_lastThemeBrushColor = winCol;
				HWND hl = GetDlgItem(m_hwnd, IDC_LIST);
				ListView_SetBkColor(hl, winCol);
				ListView_SetTextBkColor(hl, winCol);
				ListView_SetTextColor(hl, GSC_mainwnd(COLOR_BTNTEXT));
			}
#endif
			int xo, yo;
			RECT r;
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
		{
			if ((HWND)lParam == GetDlgItem(m_hwnd, IDC_EDIT) || (HWND)lParam == GetDlgItem(m_hwnd, IDC_FILTER))
			{
				SetBkColor((HDC)wParam, GSC_mainwnd(COLOR_WINDOW));
				SetTextColor((HDC)wParam, GSC_mainwnd(COLOR_BTNTEXT));
				return (INT_PTR)SNM_GetThemeBrush();
			}
		}
#endif
	}
	return 0;
}

void SNM_ResourceWnd::SelectBySlot(int _slot)
{
	SWS_ListView* lv = m_pLists.Get(0);
	HWND hList = lv ? lv->GetHWND() : NULL;
	if (lv && hList) // this can be called when the view is closed!
	{
		for (int i = 0; i < lv->GetListItemCount(); i++)
		{
			PathSlotItem* item = (PathSlotItem*)lv->GetListItem(i);
			int slot = GetCurList()->Find(item);
			if (item && slot == _slot) 
			{
				ListView_SetItemState(hList, -1, 0, LVIS_SELECTED);
				ListView_SetItemState(hList, i, LVIS_SELECTED, LVIS_SELECTED); 
				ListView_EnsureVisible(hList, i, true);
				break;
			}
		}
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
	bool updt = false;
	int x=0;
	PathSlotItem* item;
	while((item = (PathSlotItem*)m_pLists.Get(0)->EnumSelected(&x)))
	{
		int slot = GetCurList()->Find(item);
		if (slot >= 0) 
		{
			updt = true;
			if (_delFiles)
			{
				char fullPath[BUFFER_SIZE] = "";
				GetFullResourcePath(GetCurList()->GetResourceDir(), item->m_shortPath.Get(), fullPath, BUFFER_SIZE);
				SNM_DeleteFile(fullPath);
			}
			GetCurList()->Delete(slot, true);
		}
	}
	if (_update && updt)
		Update();
}

// _slotPos: insert at this slot, or add if < 0
void SNM_ResourceWnd::AutoSaveSlots(int _slotPos)
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
	char fn[BUFFER_SIZE] = "";
	switch(g_type) 
	{
		case SNM_SLOT_TYPE_FX_CHAINS:
			switch (m_autoSaveFXChainPref)
			{
				case FXC_AUTOSAVE_PREF_INPUT_FX:
					slotUpdate = autoSaveTrackFXChainSlots(_slotPos, g_bv4, GetCurAutoSaveDir()->Get(), fn, BUFFER_SIZE);
					break;
				case FXC_AUTOSAVE_PREF_TRACK:
					slotUpdate = autoSaveTrackFXChainSlots(_slotPos, false, GetCurAutoSaveDir()->Get(), fn, BUFFER_SIZE);
					break;
				case FXC_AUTOSAVE_PREF_ITEM:
					slotUpdate = autoSaveItemFXChainSlots(_slotPos, GetCurAutoSaveDir()->Get(), fn, BUFFER_SIZE);
					break;
			}
			break;
		case SNM_SLOT_TYPE_TR_TEMPLATES:
			slotUpdate = autoSaveTrackSlots(_slotPos, !m_autoSaveTrTmpltWithItemsPref, GetCurAutoSaveDir()->Get(), fn, BUFFER_SIZE);
			break;
		case SNM_SLOT_TYPE_PRJ_TEMPLATES:
			slotUpdate = autoSaveProjectSlot(_slotPos, true, GetCurAutoSaveDir()->Get(), fn, BUFFER_SIZE);
		break;
	}

	if (slotUpdate) 
	{
		Update();
/*JFB insert slot code commented: can mess the user's slot actions (because all following ids change)
		SelectBySlot(_slotPos < 0 ? GetCurList()->GetSize() - 1 : _slotPos);
*/
		SelectBySlot(GetCurList()->GetSize()-1);
	}
	else if (fn)
	{
		char msg[BUFFER_SIZE];
		_snprintf(msg, BUFFER_SIZE, "Auto-save failed: %s\n%", fn, *fn == '<' ? "" : "(cannot write file, invalid filename, etc..)");
		MessageBox(g_hwndParent, msg, "S&M - Error", MB_OK);
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
			list->Empty(true);
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
			WDL_String iniSection;
			iniSection.SetFormatted(32, "MAX_SLOT=%d\n", list->GetSize());
			for (int j=0; j < list->GetSize(); j++)
			{
				item = list->Get(j);
				if (item)
				{
					WDL_String escapedStr;
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
			SaveIniSection(resDir, &iniSection);
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

void ClearSlotPrompt(COMMAND_T* _ct) {
	g_filesLists.Get((int)_ct->user)->ClearSlotPrompt(_ct);
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
				strncpy(_fn, filename, _fnSz);

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
			WDL_String chain;
			if (LoadChunk(fullPath, &chain))
			{
				char title[64] = "";
				_snprintf(title, 64, "S&M - %s (slot %d)", m_desc.Get(), _slot+1);
				SNM_ShowConsoleMsg(chain.Get(), title);
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