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


#include "stdafx.h"
#include "../../WDL/projectcontext.h"
#include "SnM_Actions.h"
#include "SNM_FXChainView.h"
#ifdef _WIN32
#include "../MediaPool/DragDrop.h" //JFB: move to the trunk?
#endif

//#define _SNM_ITT // WIP: Item/take templates

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
#define AUTO_SAVE_DIR					0x110009
#define FXC_LOAD_APPLY_TRACK_MSG		0x11000A // specific FX chains cmds
#define FXC_LOAD_APPLY_TAKE_MSG			0x11000B
#define FXC_LOAD_APPLY_ALL_TAKES_MSG	0x11000C
#define FXC_COPY_MSG					0x11000D
#define FXC_LOAD_PASTE_TRACK_MSG		0x11000E
#define FXC_LOAD_PASTE_TAKE_MSG			0x11000F
#define FXC_LOAD_PASTE_ALL_TAKES_MSG	0x110010
#define FXC_AUTO_SAVE_INPUT_FX			0x110011
#define FXC_AUTO_SAVE_TRACK				0x110012
#define FXC_AUTO_SAVE_ITEM				0x110013
#define TRT_LOAD_APPLY_MSG				0x110014 // specific track template cmds
#define TRT_LOAD_IMPORT_MSG				0x110015
#define TRT_AUTO_SAVE_WITH_ITEMS		0x110016

// labels shared by actions and popup menu items
#define FXC_LOAD_APPLY_TRACK_STR		"Load/apply FX chain to selected tracks"
#define FXCIN_LOAD_APPLY_TRACK_STR		"Load/apply input FX chain to selected tracks"
#define FXC_LOAD_APPLY_TAKE_STR			"Load/apply FX chain to selected items"
#define FXC_LOAD_APPLY_ALL_TAKES_STR	"Load/apply FX chain to selected items, all takes"
#define FXC_LOAD_PASTE_TRACK_STR		"Load/paste FX chain to selected tracks"
#define FXC_LOAD_PASTE_TAKE_STR			"Load/paste FX chain to selected items"
#define FXC_LOAD_PASTE_ALL_TAKES_STR	"Load/paste FX chain to selected items, all takes"
#define TRT_LOAD_APPLY_STR				"Load/apply track template to selected tracks"
#define TRT_LOAD_IMPORT_STR				"Load/import tracks from track template"
#define TRT_LOAD_APPLY_ITEMS_STR		"Load/replace track template items"
#define TRT_LOAD_PASTE_ITEMS_STR		"Load/paste track template items"

#define DRAGNDROP_EMPTY_SLOT_FILE		">Empty<"

enum {
  SNM_SLOT_TYPE_FX_CHAINS=0,
  SNM_SLOT_TYPE_TR_TEMPLATES,
#ifdef _SNM_ITT
  SNM_SLOT_TYPE_ITEM_TEMPLATES,
#endif
  SNM_SLOT_TYPE_COUNT
};

enum {
  COMBOID_TYPE=1000,
  COMBOID_DBLCLICK_TYPE,
  COMBOID_DBLCLICK_TO,
  BUTTONID_AUTO_INSERT
#ifdef _SNM_ITT
  ,BUTTONID_ITEMTK_DETAILS,
  BUTTONID_ITEMTK_START
#endif
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
#ifdef _SNM_ITT
FileSlotList g_itemTemplateFiles(SNM_SLOT_TYPE_ITEM_TEMPLATES, "ItemTakeTemplates", "item/take template", "RItemTakeTemplate");
const char* g_itemProps[] = {"Volume", "Fade in", "Fade out", "Loop source", "No autofade", "Play all takes"};
const char* g_takeProps[] = {"Name", "Volume", "Take pan", "FX", "MIDI properties", "Reverse"};
#endif

int g_type = -1;

//JFB TODO? member attributes
int g_dblClickType[SNM_SLOT_TYPE_COUNT];
int g_dblClickTo = 0; // for fx chains only
WDL_PtrList<PathSlotItem> g_dragPathSlotItems; 
WDL_PtrList<FileSlotList> g_filesLists;

WDL_PtrList_DeleteOnDestroy<WDL_String> g_autoSaveDirs;
bool g_autoSaveTrTmpltWithItemsPref = true;
int g_autoSaveFXChainPref = FXC_AUTOSAVE_PREF_TRACK;


FileSlotList* GetCurList() {
	return g_filesLists.Get(g_type);
}

WDL_String* GetCurAutoSaveDir() {
	return g_autoSaveDirs.Get(g_type);
}


///////////////////////////////////////////////////////////////////////////////
// SNM_ResourceView
///////////////////////////////////////////////////////////////////////////////

SNM_ResourceView::SNM_ResourceView(HWND hwndList, HWND hwndEdit)
:SNM_FastListView(hwndList, hwndEdit, 4, g_fxChainListCols, "Resources View State", false)
{}

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
				ExtractFileNameEx(pItem->m_shortPath.Get(), buf, true); //JFB TODO: Xen code a revoir
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
				GetCurList()->GetFullPath(slot, fn, BUFFER_SIZE);
				if (!pItem->IsDefault() && FileExistsErrMsg(fn))
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
						applyOrImportTrackTemplate(TRT_LOAD_APPLY_STR, false, slot, !wasDefaultSlot);
						break;
					case 1:
						applyOrImportTrackTemplate(TRT_LOAD_IMPORT_STR, true, slot, !wasDefaultSlot);
						break;
					case 2:
						replaceOrPasteItemsFromsTrackTemplate(TRT_LOAD_PASTE_ITEMS_STR, true, slot, !wasDefaultSlot);
						break;
					case 3:
						replaceOrPasteItemsFromsTrackTemplate(TRT_LOAD_APPLY_ITEMS_STR, false, slot, !wasDefaultSlot);
						break;
				}
				break;
#ifdef _SNM_ITT
			case SNM_SLOT_TYPE_ITEM_TEMPLATES:
				break;
#endif
		}

		// in case the slot changed
		if (wasDefaultSlot && !pItem->IsDefault())
			Update();
	}
}

void SNM_ResourceView::GetItemList(WDL_TypedBuf<LPARAM>* pBuf)
{
	if (g_pResourcesWnd && g_pResourcesWnd->m_filter.GetLength())
	{
		int iCount = 0;
		LineParser lp(false);
		lp.parse(g_pResourcesWnd->m_filter.Get());
		for (int i = 0; i < GetCurList()->GetSize(); i++)
		{
			PathSlotItem* item = GetCurList()->Get(i);
			if (item && !item->IsDefault())
			{
				bool match = true;
				for (int j=0; match && j < lp.getnumtokens(); j++)
					match &= (stristr(item->m_shortPath.Get(), lp.gettoken_str(j)) != NULL);
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
		GetCurList()->GetFullPath(slot, fullPath, BUFFER_SIZE);
		bool empty = (pItem->IsDefault() || *fullPath == '\0');
		iMemNeeded += (int)((empty ? strlen(DRAGNDROP_EMPTY_SLOT_FILE) : strlen(fullPath)) + 1);
		fullPaths.Add(new WDL_String(empty ? DRAGNDROP_EMPTY_SLOT_FILE : fullPath));
		g_dragPathSlotItems.Add(pItem);
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
			m_cbDblClickType.AddItem("Apply to sel. tracks");
			m_cbDblClickType.AddItem("Import tracks");
			m_cbDblClickType.AddItem("Paste items to sel. tracks");
			m_cbDblClickType.AddItem("Replace items of sel. tracks");
			break;
	}
	m_cbDblClickType.SetCurSel(g_dblClickType[g_type]);
}

void SNM_ResourceWnd::OnInitDlg()
{
	m_resize.init_item(IDC_LIST, 0.0, 0.0, 1.0, 1.0);
	SetWindowLongPtr(GetDlgItem(m_hwnd, IDC_FILTER), GWLP_USERDATA, 0xdeadf00b);
#ifndef _WIN32
	// Realign the filter box on OSX
	HWND hFilter = GetDlgItem(m_hwnd, IDC_FILTER);
	RECT rFilter;
	GetWindowRect(hFilter, &rFilter);
	ScreenToClient(m_hwnd,(LPPOINT)&rFilter);
	ScreenToClient(m_hwnd,((LPPOINT)&rFilter)+1);
	SetWindowPos(hFilter, NULL, rFilter.left - 25, rFilter.top - 1, abs(rFilter.right - rFilter.left) - 10, abs(rFilter.bottom - rFilter.top), SWP_NOACTIVATE | SWP_NOZORDER);
#endif

	m_pLists.Add(new SNM_ResourceView(GetDlgItem(m_hwnd, IDC_LIST), GetDlgItem(m_hwnd, IDC_EDIT)));

	// Load prefs 
	//JFB!!! pb qd 1ere init (no .ini)
	// Ok pour l'instant car override par OpenResourceView() via son appel à SetType()
	g_type = GetPrivateProfileInt("RESOURCE_VIEW", "TYPE", SNM_SLOT_TYPE_FX_CHAINS, g_SNMiniFilename.Get());
	g_dblClickType[SNM_SLOT_TYPE_FX_CHAINS] = GetPrivateProfileInt("RESOURCE_VIEW", "DBLCLICK_TYPE", 0, g_SNMiniFilename.Get());
	g_dblClickType[SNM_SLOT_TYPE_TR_TEMPLATES] = GetPrivateProfileInt("RESOURCE_VIEW", "DBLCLICK_TYPE_TR_TEMPLATE", 0, g_SNMiniFilename.Get());
#ifdef _SNM_ITT
	g_dblClickType[SNM_SLOT_TYPE_ITEM_TEMPLATES] = 0;
#endif
	g_dblClickTo = GetPrivateProfileInt("RESOURCE_VIEW", "DBLCLICK_TO", 0, g_SNMiniFilename.Get());
	g_autoSaveFXChainPref = GetPrivateProfileInt("RESOURCE_VIEW", "AutoSaveFXChain", FXC_AUTOSAVE_PREF_TRACK, g_SNMiniFilename.Get());
	g_autoSaveTrTmpltWithItemsPref = (GetPrivateProfileInt("RESOURCE_VIEW", "AutoSaveTrTemplateWithItems", 1, g_SNMiniFilename.Get()) == 1);
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
	}

	// WDL GUI init
	IconTheme* it = (IconTheme*)GetIconThemeStruct(NULL);
	m_parentVwnd.SetRealParent(m_hwnd);

	m_cbType.SetID(COMBOID_TYPE);
	m_cbType.SetRealParent(m_hwnd);
	m_cbType.AddItem("FX chains");
	m_cbType.AddItem("Track templates");
#ifdef _SNM_ITT
	m_cbType.AddItem("Item/take templates");
#endif
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

	m_btnAutoInsert.SetID(BUTTONID_AUTO_INSERT);
	WDL_VirtualIconButton_SkinConfig* img = it ? &(it->toolbar_save) : NULL;
	if (img)
		m_btnAutoInsert.SetIcon(img);
	else {
		m_btnAutoInsert.SetTextLabel("Auto save", 0, SNM_GetThemeFont());
		m_btnAutoInsert.SetForceBorder(true);
	}
	m_btnAutoInsert.SetRealParent(m_hwnd);
	m_parentVwnd.AddChild(&m_btnAutoInsert);


#ifdef _SNM_ITT
	m_btnItemTakeDetails.SetID(BUTTONID_ITEMTK_DETAILS);
	m_btnItemTakeDetails.SetRealParent(m_hwnd);
	m_parentVwnd.AddChild(&m_btnItemTakeDetails);

	for (int i=0; i < SNM_FILESLOT_MAX_ITEMTK_PROPS; i++) {
		m_btnItemTakeProp[i].SetID(BUTTONID_ITEMTK_START+i);
		m_btnItemTakeProp[i].SetRealParent(m_hwnd);
		m_parentVwnd.AddChild(&m_btnItemTakeProp[i]);
	}
#endif

	// This restores the text filter when docking/undocking
	// >>> + indirect call to Update() <<<
	SetDlgItemText(GetHWND(), IDC_FILTER, m_filter.Get());

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
			m_filter.Set(cFilter);
			Update();
			break;
		}

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
			GetCurList()->GetFullPath(slot, fullPath, BUFFER_SIZE);
			char* p = strrchr(fullPath, PATH_SLASH_CHAR);
			if (p) {
				*(p+1) = '\0'; // ShellExecute() is KO otherwie..
				ShellExecute(NULL, "open", fullPath, NULL, NULL, SW_SHOWNORMAL);
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
				char errMsg[BUFFER_SIZE];
				_snprintf(errMsg, 512, "%s is empty or does not exist!", startPath);
				MessageBox(GetMainHwnd(), errMsg, "S&M - Warning", MB_OK);
			}
			break;
		}
		case AUTO_SAVE_DIR:
		{
			char path[BUFFER_SIZE];
			if (BrowseForDirectory("Set auto save directory", GetCurAutoSaveDir()->Get(), path, BUFFER_SIZE))
				GetCurAutoSaveDir()->Set(path);
			break;
		}

		// ***** FX chains *****
		case FXC_COPY_MSG:
			if (item)
				copyFXChainSlotToClipBoard(slot);
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
			g_autoSaveFXChainPref = FXC_AUTOSAVE_PREF_INPUT_FX;
			break;
		case FXC_AUTO_SAVE_TRACK:
			g_autoSaveFXChainPref = FXC_AUTOSAVE_PREF_TRACK;
			break;
		case FXC_AUTO_SAVE_ITEM:
			g_autoSaveFXChainPref = FXC_AUTOSAVE_PREF_ITEM;
			break;

		// ***** Track template *****
		case TRT_LOAD_APPLY_MSG:
		case TRT_LOAD_IMPORT_MSG:
			if (item && slot >= 0) {
				applyOrImportTrackTemplate(wParam == TRT_LOAD_APPLY_MSG ? TRT_LOAD_APPLY_STR : TRT_LOAD_IMPORT_STR, wParam != TRT_LOAD_APPLY_MSG, slot, !wasDefaultSlot);
				if (wasDefaultSlot && !item->IsDefault()) // slot has been filled ?
					Update();
			}
			break;
		case TRT_AUTO_SAVE_WITH_ITEMS:
			g_autoSaveTrTmpltWithItemsPref = !g_autoSaveTrTmpltWithItemsPref;
			break;

		// ***** WDL GUI & others *****
		default:
		{
			if (HIWORD(wParam)==0) 
			{
				switch(LOWORD(wParam))
				{
					case BUTTONID_AUTO_INSERT:
						AutoSaveSlots(slot);
					break;
#ifdef _SNM_ITT
					case BUTTONID_ITEMTK_DETAILS:
					{
						HWND hList = GetDlgItem(m_hwnd, IDC_LIST);
						if (hList)
							ShowWindow(hList, IsWindowVisible(hList) ? SW_HIDE : SW_SHOW);
					}
					break;
#endif
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
					SetFocus(GetDlgItem(m_hwnd, IDC_FILTER));
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
	AddToMenu(hMenu, "Auto fill (from resource path)", AUTO_INSERT_SLOTS);
	AddToMenu(hMenu, SWS_SEPARATOR, 0);
	switch(g_type)
	{
		case SNM_SLOT_TYPE_FX_CHAINS:
			AddToMenu(hMenu, "Set FX chain auto save directory...", AUTO_SAVE_DIR);
			if (g_bv4)
				AddToMenu(hMenu, "Auto save button: input FX chains from track selection", FXC_AUTO_SAVE_INPUT_FX, -1, false, g_autoSaveFXChainPref == FXC_AUTOSAVE_PREF_INPUT_FX ? MFS_CHECKED : MFS_UNCHECKED);
			AddToMenu(hMenu, "Auto save button: FX chains from track selection", FXC_AUTO_SAVE_TRACK, -1, false, g_autoSaveFXChainPref == FXC_AUTOSAVE_PREF_TRACK ? MFS_CHECKED : MFS_UNCHECKED);
			AddToMenu(hMenu, "Auto save button: FX chains from item selection", FXC_AUTO_SAVE_ITEM, -1, false, g_autoSaveFXChainPref == FXC_AUTOSAVE_PREF_ITEM ? MFS_CHECKED : MFS_UNCHECKED);
			break;
		case SNM_SLOT_TYPE_TR_TEMPLATES:
			AddToMenu(hMenu, "Set track template auto save directory...", AUTO_SAVE_DIR);
			AddToMenu(hMenu, "Auto save button: save track templates with items", TRT_AUTO_SAVE_WITH_ITEMS, -1, false, g_autoSaveTrTmpltWithItemsPref ? MFS_CHECKED : MFS_UNCHECKED);
			break;
#ifdef _SNM_ITT
		case SNM_SLOT_TYPE_ITEM_TEMPLATES:
			break;
#endif
	}
	AddToMenu(hMenu, SWS_SEPARATOR, 0);
	AddToMenu(hMenu, "Add slot", ADD_SLOT_MSG, -1, false, !m_filter.GetLength() ? MF_ENABLED : MF_GRAYED);

	int iCol;
	LPARAM item = m_pLists.Get(0)->GetHitItem(x, y, &iCol);
	PathSlotItem* pItem = (PathSlotItem*)item;
	if (pItem && iCol >= 0)
	{
		UINT enabled = !pItem->IsDefault() ? MF_ENABLED : MF_GRAYED;
		AddToMenu(hMenu, "Insert slot", INSERT_SLOT_MSG, -1, false, !m_filter.GetLength() ? MF_ENABLED : MF_GRAYED);
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
				break;

#ifdef _SNM_ITT
			case SNM_SLOT_TYPE_ITEM_TEMPLATES:
				break;
#endif
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
	WritePrivateProfileString("RESOURCE_VIEW", "TYPE", cTmp, g_SNMiniFilename.Get()); 
	sprintf(cTmp, "%d", g_dblClickType[SNM_SLOT_TYPE_FX_CHAINS]);
	WritePrivateProfileString("RESOURCE_VIEW", "DBLCLICK_TYPE", cTmp, g_SNMiniFilename.Get()); 
	sprintf(cTmp, "%d", g_dblClickType[SNM_SLOT_TYPE_TR_TEMPLATES]);
	WritePrivateProfileString("RESOURCE_VIEW", "DBLCLICK_TYPE_TR_TEMPLATE", cTmp, g_SNMiniFilename.Get()); 
	sprintf(cTmp, "%d", g_dblClickTo);
	WritePrivateProfileString("RESOURCE_VIEW", "DBLCLICK_TO", cTmp, g_SNMiniFilename.Get()); 
	sprintf(cTmp, "%d", g_autoSaveFXChainPref);
	WritePrivateProfileString("RESOURCE_VIEW", "AutoSaveFXChain", cTmp, g_SNMiniFilename.Get()); 
	WritePrivateProfileString("RESOURCE_VIEW", "AutoSaveTrTemplateWithItems", g_autoSaveTrTmpltWithItemsPref ? "1" : "0", g_SNMiniFilename.Get()); 
	{
		WDL_String escapedStr;
		makeEscapedConfigString(g_autoSaveDirs.Get(0)->Get(), &escapedStr);
		WritePrivateProfileString("RESOURCE_VIEW", "AutoSaveDirFXChain", escapedStr.Get(), g_SNMiniFilename.Get()); 
		makeEscapedConfigString(g_autoSaveDirs.Get(1)->Get(), &escapedStr);
		WritePrivateProfileString("RESOURCE_VIEW", "AutoSaveDirTrTemplate", escapedStr.Get(), g_SNMiniFilename.Get()); 
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

static void DrawControls(WDL_VWnd_Painter *_painter, RECT _r, WDL_VWnd* _parentVwnd)
{
	if (!g_pResourcesWnd)
		return;

	int xo=0, yo=0;
    LICE_IBitmap *bm = _painter->GetBuffer(&xo,&yo);
	if (bm)
	{
		LICE_CachedFont* font = SNM_GetThemeFont();
		int x0=_r.left+10, y0=_r.top+5, h=25;
		int w=25; //default width

		WDL_VirtualComboBox* cbVwnd = (WDL_VirtualComboBox*)_parentVwnd->GetChildByID(COMBOID_TYPE);
		if (cbVwnd)
		{
			RECT tr2={x0,y0+3,x0+110,y0+h-2};
			x0 = tr2.right+5;
			cbVwnd->SetPosition(&tr2);
			cbVwnd->SetFont(font);
		}

		// "Filter:"
		{
			x0 += 10;
			RECT tr={x0,y0,x0+40,y0+h};
			font->DrawText(bm, "Filter:", -1, &tr, DT_LEFT | DT_VCENTER);
			x0 = tr.right+5;
		}

		x0 += 125; // i.e. width of the filter edit box

		// common "dbl-click" control
		cbVwnd = (WDL_VirtualComboBox*)_parentVwnd->GetChildByID(COMBOID_DBLCLICK_TYPE);
		if (cbVwnd)
		{
			if (g_type == SNM_SLOT_TYPE_FX_CHAINS || g_type == SNM_SLOT_TYPE_TR_TEMPLATES)
			{
				RECT tr={x0,y0,x0+48,y0+h};
				font->DrawText(bm, "Dbl-click:", -1, &tr, DT_LEFT | DT_VCENTER);
				x0 = tr.right+5;

				RECT tr2={x0, y0+3, g_type == SNM_SLOT_TYPE_FX_CHAINS ? x0+60 : x0+165, y0+h-2};
				x0 = tr2.right+5;
				cbVwnd->SetPosition(&tr2);
				cbVwnd->SetFont(font);
			}
			cbVwnd->SetVisible(g_type == SNM_SLOT_TYPE_FX_CHAINS || g_type == SNM_SLOT_TYPE_TR_TEMPLATES);
		}

		// fx chain control
		cbVwnd = (WDL_VirtualComboBox*)_parentVwnd->GetChildByID(COMBOID_DBLCLICK_TO);
		if (cbVwnd)
		{
			if (g_type == SNM_SLOT_TYPE_FX_CHAINS)
			{
				x0+=10;
				RECT tr={x0,y0,x0+60,y0+h};
				font->DrawText(bm, "To selected:", -1, &tr, DT_LEFT | DT_VCENTER);
				x0 = tr.right+5;

				RECT tr2={x0,y0+3,x0+112,y0+h-2};
				x0 = tr2.right+5;
				cbVwnd->SetPosition(&tr2);
				cbVwnd->SetFont(font);
			}
			cbVwnd->SetVisible(g_type == SNM_SLOT_TYPE_FX_CHAINS);
		}

		// common "auto save/insert" control
		WDL_VirtualIconButton* btnVwnd = (WDL_VirtualIconButton*)_parentVwnd->GetChildByID(BUTTONID_AUTO_INSERT);
		if (btnVwnd)
		{
			WDL_VirtualIconButton_SkinConfig* img = btnVwnd->GetIcon();
			if (img) {
				w = img->image->getWidth() / 3;
				h = img->image->getHeight();
			}
			else
				w = 70;
			x0 += 10;
			RECT tr2={x0, y0 - (img ? 2:-3), x0 + w, y0 - (img ? 2:1) + h};
			btnVwnd->SetPosition(&tr2);
			x0 += 5+w;
		}

#ifdef _SNM_ITT
		// Item/take templates
		WDL_VirtualIconButton* btn = (WDL_VirtualIconButton*)_parentVwnd->GetChildByID(BUTTONID_ITEMTK_DETAILS);
		if (btn)
		{
			if (g_type == SNM_SLOT_TYPE_ITEM_TEMPLATES)
			{
				x0 += 5;
				RECT tr={x0,y0+4,x0+16,y0+20};
				x0 = tr.right+5;
				btn->SetPosition(&tr);

				HWND hList = GetDlgItem(g_pResourcesWnd->GetHWND(), IDC_LIST);
				btn->SetCheckState(hList && !IsWindowVisible(hList));

				RECT tr2={x0,y0,x0+100,y0+25};
				tmpfont.DrawText(bm, "Show item/take properties", -1, &tr2, DT_LEFT | DT_VCENTER);
				x0 = tr2.right+5;
			}
			btn->SetVisible(g_type == SNM_SLOT_TYPE_ITEM_TEMPLATES);
		}

		int x1=_r.left+10, y1=_r.top+40;
		for (int i=0; i < SNM_FILESLOT_MAX_ITEMTK_PROPS; i++)
		{
			btn = (WDL_VirtualIconButton*)_parentVwnd->GetChildByID(BUTTONID_ITEMTK_START+i);
			if (btn)
			{
				if ((y1+25) >= _r.bottom) {
					x1+=100, y1=_r.top+40;
				}

				if (g_type == SNM_SLOT_TYPE_ITEM_TEMPLATES)
				{
					x0 += 5;
					RECT tr={x1,y1+4,x1+16,y1+20};
//ui					x0 = tr.right+5;

					btn->SetPosition(&tr);
					btn->SetCheckState(true);

/*button
					x0 += 5;
					RECT tr={x0,y0+4,x0+30,y0+20};
					x0 = tr.right+5;
					btn->SetPosition(&tr);
					btn->SetTextLabel("...", 0, &tmpfont);
					btn->SetForceBorder(true);
*/
//ui					RECT tr2={x0,y0,x0+40,y0+25};
					RECT tr2={tr.right+5,y1,x1+80,y1+25};
					tmpfont.DrawText(bm, i < 6 ? g_itemProps[i] : g_takeProps[i-6], -1, &tr2, DT_LEFT | DT_VCENTER);
//ui					x0 = tr2.right+5;
					y1 = tr.bottom+5;
				}
				btn->SetVisible(g_type == SNM_SLOT_TYPE_ITEM_TEMPLATES);
			}
		}
#endif
		_painter->PaintVirtWnd(_parentVwnd, 0);

		if (g_snmLogo && (_r.right - _r.left) > (x0+g_snmLogo->getWidth()))
			LICE_Blit(bm,g_snmLogo,_r.right-g_snmLogo->getWidth()-8,y0+3,NULL,0.125f,LICE_BLIT_MODE_ADD|LICE_BLIT_USE_ALPHA);
	}
}

int SNM_ResourceWnd::OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_PAINT:
		{
			RECT r;
			GetClientRect(m_hwnd,&r);	
			m_parentVwnd.SetPosition(&r);
			m_vwnd_painter.PaintBegin(m_hwnd, WDL_STYLE_GetSysColor(COLOR_WINDOW));
			DrawControls(&m_vwnd_painter, r, &m_parentVwnd);
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
		_snprintf(msg, BUFFER_SIZE, "Auto save directory not found:\n%s\n\nDo you want to define one ?", GetCurAutoSaveDir()->Get());
		switch(MessageBox(g_hwndParent, msg, "S&M - Error", MB_YESNO)) {
			case IDYES: OnCommand(AUTO_SAVE_DIR, 0); break;
			case IDNO: return;
		}
		if (!FileExists(GetCurAutoSaveDir()->Get())) // re-check: the user may have cancelled..
			return;
	}

	bool updt = false;
	char fn[BUFFER_SIZE] = "";
	switch(g_type) 
	{
		case SNM_SLOT_TYPE_FX_CHAINS:
			switch (g_autoSaveFXChainPref)
			{
				case FXC_AUTOSAVE_PREF_INPUT_FX:
					updt = autoSaveTrackFXChainSlots(_slotPos, g_bv4, GetCurAutoSaveDir()->Get(), fn, BUFFER_SIZE);
					break;
				case FXC_AUTOSAVE_PREF_TRACK:
					updt = autoSaveTrackFXChainSlots(_slotPos, false, GetCurAutoSaveDir()->Get(), fn, BUFFER_SIZE);
					break;
				case FXC_AUTOSAVE_PREF_ITEM:
					updt = autoSaveItemFXChainSlots(_slotPos, GetCurAutoSaveDir()->Get(), fn, BUFFER_SIZE);
					break;
			}
			break;
		case SNM_SLOT_TYPE_TR_TEMPLATES:
			updt = autoSaveTrackTemplateSlots(_slotPos, !g_autoSaveTrTmpltWithItemsPref, GetCurAutoSaveDir()->Get(), fn, BUFFER_SIZE);
			break;
#ifdef _SNM_ITT
		case SNM_SLOT_TYPE_ITEM_TEMPLATES:
			break;
#endif
	}

	if (updt) 
	{
		Update();
/*JFB insert slot code commented: can mess the user's slot actions (because all following ids change)
		SelectBySlot(_slotPos < 0 ? GetCurList()->GetSize() - 1 : _slotPos);
*/
		SelectBySlot(GetCurList()->GetSize()-1);
	}
	else
	{
		char msg[BUFFER_SIZE];
		_snprintf(msg, BUFFER_SIZE, "Auto save failed: %s\n(cannot write file, invalid filename, etc..)", fn);
		MessageBox(g_hwndParent, msg, "S&M - Error", MB_OK);
		return;
	}
}


///////////////////////////////////////////////////////////////////////////////

int ResourceViewInit()
{
	// Init the lists of files (ordered by g_type)
	g_filesLists.Empty();
	g_filesLists.Add(&g_fxChainFiles);
	g_filesLists.Add(&g_trTemplateFiles);
#ifdef _SNM_ITT
	g_filesLists.Add(&g_itemTemplateFiles);
#endif

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
		SetFocus(GetDlgItem(g_pResourcesWnd->GetHWND(), IDC_FILTER));
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

//returns -1 on cancel
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

// Returns false if cancelled
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
			GetFullPath(_slot, _fn, _fnSz);
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
		GetFullPath(_slot, fullPath, BUFFER_SIZE);
		if (FileExists(fullPath))
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