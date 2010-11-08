/******************************************************************************
/ SnM_FXChainView.cpp
/ JFB TODO: now, a better name would be "SnM_ResourceView.cpp"
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


#include "stdafx.h"
#include "SnM_Actions.h"
#include "SNM_FXChainView.h"
#ifdef _WIN32
#include "../MediaPool/DragDrop.h" //JFB: move to the trunk?
#endif

// JFB TODO? now, a better key name would be "Resources view Save Window Position
#define SAVEWINDOW_POS_KEY "S&M - FX Chain List Save Window Position" 

// Commands
#define CLEAR_MSG						0x110001
#define DEL_SLOT_MSG					0x110002
#define ADD_SLOT_MSG					0x110003
#define INSERT_SLOT_MSG					0x110004
#define DISPLAY_MSG						0x110005
#define LOAD_MSG						0x110006
#define FXC_LOAD_APPLY_TRACK_MSG		0x110007
#define FXC_LOAD_APPLY_TAKE_MSG			0x110008
#define FXC_LOAD_APPLY_ALL_TAKES_MSG	0x110009
#define FXC_COPY_MSG					0x11000A
#define FXC_LOAD_PASTE_TRACK_MSG		0x11000B
#define FXC_LOAD_PASTE_TAKE_MSG			0x11000C
#define FXC_LOAD_PASTE_ALL_TAKES_MSG	0x11000D

#define FXC_LOAD_APPLY_TRACK_STR		"Load/apply FX chain to selected tracks"
#define FXC_LOAD_APPLY_TAKE_STR			"Load/apply FX chain to selected items"
#define FXC_LOAD_APPLY_ALL_TAKES_STR	"Load/apply FX chain to selected items, all takes"
#define FXC_LOAD_PASTE_TRACK_STR		"Load/paste FX chain to selected tracks"
#define FXC_LOAD_PASTE_TAKE_STR			"Load/paste FX chain to selected items"
#define FXC_LOAD_PASTE_ALL_TAKES_STR	"Load/paste FX chain to selected items, all takes"

#define DRAGNDROP_EMPTY_SLOT_HACK		">empty<"


//JFB3 TODO: better names
enum
{
  COMBOID_TYPE=1000,
  COMBOID_DBLCLICK_TYPE,
  COMBOID_DBLCLICK_TO
#ifdef _SNM_ITT
  ,BUTTONID_ITEMTK_DETAILS,
  BUTTONID_ITEMTK_START
#endif
};

// Globals
/*JFB static*/ SNM_ResourceWnd* g_pResourcesWnd = NULL;
static SWS_LVColumn g_fxChainListCols[] = { {50,2,"Slot"}, {100,2,"Name"}, {250,2,"Path"}, {200,1,"Description"} };

//JFB TODO: consts for resSubdir, filters...
FileSlotList g_fxChainFiles(SNM_SLOT_TYPE_FX_CHAINS, "FXChains", "FX chain", "RfxChain");
FileSlotList g_trTemplateFiles(SNM_SLOT_TYPE_TR_TEMPLATES, "TrackTemplates", "track template", "RTrackTemplate");
#ifdef _SNM_ITT
FileSlotList g_itemTemplateFiles(SNM_SLOT_TYPE_ITEM_TEMPLATES, "ItemTakeTemplates", "item/take template", "RItemTakeTemplate");
const char* g_itemProps[] = {"Volume", "Fade in", "Fade out", "Loop source", "No autofade", "Play all takes"};
const char* g_takeProps[] = {"Name", "Volume", "Take pan", "FX", "MIDI properties", "Reverse"};
#endif

WDL_PtrList<FileSlotList> g_filesLists;

//JFB TODO -> member attributes?
int g_type = SNM_SLOT_TYPE_FX_CHAINS;
int g_dblClickType[SNM_SLOT_TYPE_COUNT];
int g_dblClickTo = 0; // for fx chains only

FileSlotList* GetCurList() {return g_filesLists.Get(g_type);}
WDL_PtrList<PathSlotItem> g_dragPathSlotItems; 


///////////////////////////////////////////////////////////////////////////////
// SNM_ResourceView
///////////////////////////////////////////////////////////////////////////////

SNM_ResourceView::SNM_ResourceView(HWND hwndList, HWND hwndEdit)
:SWS_ListView(hwndList, hwndEdit, 4, g_fxChainListCols, "S&M - FX Chain View State", false) 
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
				if (slot >= 0)
					_snprintf(str, iStrMax, "%5.d", slot+1);
				else
					lstrcpyn(str, "?", iStrMax);
			}
			break;
			case 1:
				lstrcpyn(str, pItem->m_name.Get(), iStrMax);
				break;
			case 2:
				lstrcpyn(str, pItem->m_shortPath.Get(), iStrMax);				
				break;
			case 3:
				lstrcpyn(str, pItem->m_desc.Get(), iStrMax);				
				break;
			default:
				break;
		}
	}
}

void SNM_ResourceView::SetItemText(LPARAM item, int iCol, const char* str)
{
	PathSlotItem* pItem = (PathSlotItem*)item;
	if (pItem)
	{
		if (iCol==3)
		{
			// Limit the desc. size (for RPP files)
			pItem->m_desc.Set(str);
			pItem->m_desc.Ellipsize(128,128);
			Update();
		}
	}
}


void SNM_ResourceView::OnItemDblClk(LPARAM item, int iCol)
{
	PathSlotItem* pItem = (PathSlotItem*)item;
	int slot = GetCurList()->Find(pItem);
	if (pItem && slot >= 0) 
	{
		if (pItem->IsDefault())
		{
			if (g_pResourcesWnd)
				g_pResourcesWnd->OnCommand(LOAD_MSG, item);
		}
		else
		{
			switch(g_type)
			{
				case SNM_SLOT_TYPE_FX_CHAINS:
					switch(g_dblClickTo)
					{
						case 0:
							loadSetPasteTrackFXChain(FXC_LOAD_APPLY_TRACK_STR, slot, g_dblClickType[g_type]==0, true);
							break;
						case 1:
							loadSetPasteTakeFXChain(FXC_LOAD_APPLY_TAKE_STR, slot, true, g_dblClickType[g_type]==0, true);
							break;
						case 2:
							loadSetPasteTakeFXChain(FXC_LOAD_APPLY_ALL_TAKES_STR, slot, false, g_dblClickType[g_type]==0, true);
							break;
					}
					break;

				case SNM_SLOT_TYPE_TR_TEMPLATES:
					loadSetOrAddTrackTemplate("Apply track template", (g_dblClickType[g_type]==1), slot, true);
					break;
#ifdef _SNM_ITT
				case SNM_SLOT_TYPE_ITEM_TEMPLATES:
					//JFB3 TODO
					break;
#endif
			}
		}
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
				for (int j = 0; match && j < lp.getnumtokens(); j++)
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

//JFB more than "shared" with Tim's MediaPool => factorize ?
void SNM_ResourceView::OnBeginDrag(LPARAM _item)
{
#ifdef _WIN32
	LVITEM li;
	li.mask = LVIF_STATE | LVIF_PARAM;
	li.stateMask = LVIS_SELECTED;
	li.iSubItem = 0;
	
	// Sore dragged items (for internal d'n'd) 
	// +g et the amount of memory needed for the file list
	g_dragPathSlotItems.Empty(false);
	int iMemNeeded = 0;
	for (int i = 0; i < ListView_GetItemCount(m_hwndList); i++)
	{
		li.iItem = i;
		ListView_GetItem(m_hwndList, &li);
		if (li.state & LVIS_SELECTED)
		{
			PathSlotItem* pItem = (PathSlotItem*)li.lParam;
			if (pItem)
			{
			    // little hack to allow d'n'drop of empty slots
				iMemNeeded += (int)((pItem->IsDefault() ? strlen(DRAGNDROP_EMPTY_SLOT_HACK) : pItem->m_fullPath.GetLength()) + 1);
				g_dragPathSlotItems.Add(pItem);
			}
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

	// Add the files to the DROPFILES struct, double-NULL terminated
	for (int i = 0; i < ListView_GetItemCount(m_hwndList); i++)
	{
		li.iItem = i;
		ListView_GetItem(m_hwndList, &li);
		if (li.state & LVIS_SELECTED)
		{
			PathSlotItem* pItem = (PathSlotItem*)li.lParam;
			strcpy(pBuf, pItem->IsDefault() ? DRAGNDROP_EMPTY_SLOT_HACK : pItem->m_fullPath.Get());
			pBuf += strlen(pBuf) + 1;
		}
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
:SWS_DockWnd(IDD_SNM_FXCHAINLIST, "Resources", 30006, SWSGetCommandID(OpenResourceView))
{
	m_previousType = g_type;
	if (m_bShowAfterInit)
		Show(false, false);
}

void SNM_ResourceWnd::SetType(int _type)
{
	m_previousType = g_type;
	g_type = _type;
	m_cbType.SetCurSel(_type);
	if (m_previousType != g_type)
		Update();
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

void SNM_ResourceWnd::Update()
{
	if (m_pLists.GetSize())
		m_pLists.Get(0)->Update();
	m_parentVwnd.RequestRedraw(NULL); //JFB3!!! should be like this in all views!!!!
}

void SNM_ResourceWnd::FillDblClickTypeCombo()
{
	m_cbDblClickType.Empty();
	if (g_type == SNM_SLOT_TYPE_FX_CHAINS)
	{
		m_cbDblClickType.AddItem("Apply");
		m_cbDblClickType.AddItem("Paste");
	}
	// include tr templates..
	else
	{
		m_cbDblClickType.AddItem("Apply to selected tracks");
		m_cbDblClickType.AddItem("Add as new track");
	}
}

void SNM_ResourceWnd::OnInitDlg()
{
	m_resize.init_item(IDC_LIST, 0.0, 0.0, 1.0, 1.0);
	SetWindowLongPtr(GetDlgItem(m_hwnd, IDC_FILTER), GWLP_USERDATA, 0xdeadf00b);

	m_pLists.Add(new SNM_ResourceView(GetDlgItem(m_hwnd, IDC_LIST), GetDlgItem(m_hwnd, IDC_EDIT)));

	// Load prefs 
	g_type = GetPrivateProfileInt("RESOURCE_VIEW", "TYPE", 0, g_SNMiniFilename.Get());
	g_dblClickType[SNM_SLOT_TYPE_FX_CHAINS] = GetPrivateProfileInt("RESOURCE_VIEW", "DBLCLICK_TYPE", 0, g_SNMiniFilename.Get());
	g_dblClickType[SNM_SLOT_TYPE_TR_TEMPLATES] = GetPrivateProfileInt("RESOURCE_VIEW", "DBLCLICK_TYPE_TR_TEMPLATE", 0, g_SNMiniFilename.Get());
#ifdef _SNM_ITT
	g_dblClickType[SNM_SLOT_TYPE_ITEM_TEMPLATES] = 0;
#endif
	g_dblClickTo = GetPrivateProfileInt("RESOURCE_VIEW", "DBLCLICK_TO", 0, g_SNMiniFilename.Get());

	// WDL GUI init
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
	FillDblClickTypeCombo();

	m_cbDblClickType.SetCurSel(g_dblClickType[g_type]);
	m_parentVwnd.AddChild(&m_cbDblClickType);

	m_cbDblClickTo.SetID(COMBOID_DBLCLICK_TO);
	m_cbDblClickTo.SetRealParent(m_hwnd);
	m_cbDblClickTo.AddItem("Tracks");
	m_cbDblClickTo.AddItem("Items");
	m_cbDblClickTo.AddItem("Items, all takes");
	m_cbDblClickTo.SetCurSel(g_dblClickTo);
	m_parentVwnd.AddChild(&m_cbDblClickTo);

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
	SetDlgItemText(GetHWND(), IDC_FILTER, m_filter.Get());

	Update();
}

void SNM_ResourceWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	int x=0;
	PathSlotItem* item = (PathSlotItem*)m_pLists.Get(0)->EnumSelected(&x);
	int slot = GetCurList()->Find(item);
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

		case ADD_SLOT_MSG:
			AddSlot(true);
			break;
		case INSERT_SLOT_MSG:
			InsertAtSelectedSlot(true);
			break;
		case DEL_SLOT_MSG:
			DeleteSelectedSlots(true);
			break;
		case LOAD_MSG:
			if (item && slot >= 0 && GetCurList()->BrowseStoreSlot(slot))
				Update();
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
		case DISPLAY_MSG:
			if (item)
				GetCurList()->DisplaySlot(slot);
			break;

		case FXC_COPY_MSG:
			if (item)
				copyFXChainSlotToClipBoard(slot);
			break;

		// Apply
		case FXC_LOAD_APPLY_TRACK_MSG:
			if (item && slot >= 0) {
				loadSetPasteTrackFXChain(FXC_LOAD_APPLY_TRACK_STR, slot, true, !item->IsDefault());
				Update();
			}
			break;
		case FXC_LOAD_APPLY_TAKE_MSG:
			if (item && slot >= 0) {
				loadSetPasteTakeFXChain(FXC_LOAD_APPLY_TAKE_STR, slot, true, true, !item->IsDefault());
				Update();
			}
			break;
		case FXC_LOAD_APPLY_ALL_TAKES_MSG:
			if (item && slot >= 0) {
				loadSetPasteTakeFXChain(FXC_LOAD_APPLY_ALL_TAKES_STR, slot, false, true, !item->IsDefault());
				Update();
			}
			break;

		// Paste
		case FXC_LOAD_PASTE_TRACK_MSG:
			if (item && slot >= 0) {
				loadSetPasteTrackFXChain(FXC_LOAD_PASTE_TRACK_STR, slot, false, !item->IsDefault());
				Update();
			}
			break;
		case FXC_LOAD_PASTE_TAKE_MSG:
			if (item && slot >= 0) {
				loadSetPasteTakeFXChain(FXC_LOAD_PASTE_TAKE_STR, slot, true, false, !item->IsDefault());
				Update();
			}
			break;
		case FXC_LOAD_PASTE_ALL_TAKES_MSG:
			if (item && slot >= 0) {
				loadSetPasteTakeFXChain(FXC_LOAD_PASTE_ALL_TAKES_STR, slot, false, false, !item->IsDefault());
				Update();
			}
			break;
		
		default:
		{
			// WDL GUI
			if (HIWORD(wParam)==CBN_SELCHANGE && LOWORD(wParam)==COMBOID_TYPE)
			{
				int previousType = g_type;
				g_type = m_cbType.GetCurSel();
				if (g_type != previousType)
				{
					FillDblClickTypeCombo();
					m_cbDblClickType.SetCurSel(g_dblClickType[g_type]); // ok: no recursive call..
					Update();
					SetFocus(GetDlgItem(m_hwnd, IDC_FILTER));
				}
			}
			else if (HIWORD(wParam)==CBN_SELCHANGE && LOWORD(wParam)==COMBOID_DBLCLICK_TYPE)
				g_dblClickType[g_type] = m_cbDblClickType.GetCurSel();
			else if (HIWORD(wParam)==CBN_SELCHANGE && LOWORD(wParam)==COMBOID_DBLCLICK_TO)
				g_dblClickTo = m_cbDblClickTo.GetCurSel();

			// default
			else 
				Main_OnCommand((int)wParam, (int)lParam);

			break;
		}
	}
}

HMENU SNM_ResourceWnd::OnContextMenu(int x, int y)
{
	HMENU hMenu = NULL;
	int iCol;
	LPARAM item = m_pLists.Get(0)->GetHitItem(x, y, &iCol);
	PathSlotItem* pItem = (PathSlotItem*)item;
	if (pItem && iCol >= 0)
	{
		UINT enabled = pItem->m_fullPath.GetLength() ? MF_ENABLED : MF_DISABLED;

		hMenu = CreatePopupMenu();

		AddToMenu(hMenu, "Add slot", ADD_SLOT_MSG, -1, false, !m_filter.GetLength() ? MF_ENABLED : MF_DISABLED);
		AddToMenu(hMenu, "Insert slot", INSERT_SLOT_MSG, -1, false, !m_filter.GetLength() ? MF_ENABLED : MF_DISABLED);
		AddToMenu(hMenu, "Delete slots", DEL_SLOT_MSG);
		AddToMenu(hMenu, SWS_SEPARATOR, 0);
		AddToMenu(hMenu, "Load slot...", LOAD_MSG);
		AddToMenu(hMenu, "Clear slots", CLEAR_MSG, -1, false, enabled);
		switch(g_type)
		{
			case SNM_SLOT_TYPE_FX_CHAINS:
				AddToMenu(hMenu, SWS_SEPARATOR, 0);
				AddToMenu(hMenu, "Copy", FXC_COPY_MSG, -1, false, enabled);
				AddToMenu(hMenu, SWS_SEPARATOR, 0);
				AddToMenu(hMenu, FXC_LOAD_APPLY_TRACK_STR, FXC_LOAD_APPLY_TRACK_MSG);
				AddToMenu(hMenu, FXC_LOAD_PASTE_TRACK_STR, FXC_LOAD_PASTE_TRACK_MSG);
				AddToMenu(hMenu, SWS_SEPARATOR, 0);
				AddToMenu(hMenu, FXC_LOAD_APPLY_TAKE_STR, FXC_LOAD_APPLY_TAKE_MSG);
				AddToMenu(hMenu, FXC_LOAD_APPLY_ALL_TAKES_STR, FXC_LOAD_APPLY_ALL_TAKES_MSG);
				AddToMenu(hMenu, FXC_LOAD_PASTE_TAKE_STR, FXC_LOAD_PASTE_TAKE_MSG);
				AddToMenu(hMenu, FXC_LOAD_PASTE_ALL_TAKES_STR, FXC_LOAD_PASTE_ALL_TAKES_MSG);
				AddToMenu(hMenu, SWS_SEPARATOR, 0);
				AddToMenu(hMenu, "Display FX chain...", DISPLAY_MSG, -1, false, enabled);
				break;
			//JFB3 TODO
			case SNM_SLOT_TYPE_TR_TEMPLATES:
				AddToMenu(hMenu, SWS_SEPARATOR, 0);
				AddToMenu(hMenu, "Display track template...", DISPLAY_MSG, -1, false, enabled);
				break;
#ifdef _SNM_ITT
			//JFB3 TODO
			case SNM_SLOT_TYPE_ITEM_TEMPLATES:
				AddToMenu(hMenu, SWS_SEPARATOR, 0);
				AddToMenu(hMenu, "Display item/take template...", DISPLAY_MSG, -1, false, enabled);
				break;
#endif
		}
	}
	else 
	{
		hMenu = CreatePopupMenu();
		AddToMenu(hMenu, "Add slot", ADD_SLOT_MSG, -1, false, !m_filter.GetLength() ? MF_ENABLED : MF_DISABLED);
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

	m_cbType.Empty();
	m_cbDblClickType.Empty();
	m_cbDblClickTo.Empty();
	m_parentVwnd.RemoveAllChildren(false);
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

		// empty slot ?
		if (!strcmp(cFile, DRAGNDROP_EMPTY_SLOT_HACK))
			validCnt++;
		else {
			// .rfxchain? .rTrackTemplate? etc..
			char* pExt = strrchr(cFile, '.');
			if (pExt && !_stricmp(pExt+1, GetCurList()->m_ext.Get())) 
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

	// internal d'n'd ? if so, get the first dragged source slot 
	if (g_dragPathSlotItems.GetSize())
	{
		int srcSlot = GetCurList()->Find(g_dragPathSlotItems.Get(0));
		// drag'n'drop slot to the bottom
		if (srcSlot >= 0 && srcSlot < dropSlot)
			dropSlot++; // more 'natural' 
	}

	// drop but not on a slot => create slots
	if (!pItem || dropSlot < 0 || dropSlot >= GetCurList()->GetSize()) 
	{
		dropSlot = GetCurList()->GetSize();
		for (int i = 0; i < iValidFiles; i++)
			GetCurList()->AddSlot();
	}
	// drop on a slot => insert need slots at drop point
	else 
	{
		for (int i = 0; i < iValidFiles; i++)
			GetCurList()->InsertEmptySlot(dropSlot);
	}

	// re-sync pItem 
	pItem = GetCurList()->Get(dropSlot); 

	char cFile[BUFFER_SIZE];
	int slot;
	for (int i = 0; pItem && i < iFiles; i++)
	{
		slot = GetCurList()->Find(pItem);
		DragQueryFile(_h, i, cFile, BUFFER_SIZE);

		// empty slots
		if (!strcmp(cFile, DRAGNDROP_EMPTY_SLOT_HACK)) {
			dropped++;
			pItem = GetCurList()->Get(slot+1); 
		}
		// patch added/inserted slots from dropped data
		else {
			// .rfxchain? .rTrackTemplate? etc..
			char* pExt = strrchr(cFile, '.');
			if (pExt && !_stricmp(pExt+1, GetCurList()->m_ext.Get())) { 		
				if (GetCurList()->CheckAndStoreSlot(slot, cFile)) {
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

	int xo=0, yo=0, sz;
    LICE_IBitmap *bm = _painter->GetBuffer(&xo,&yo);
	if (bm)
	{
		ColorTheme* ct = (ColorTheme*)GetColorThemeStruct(&sz);

		static LICE_IBitmap *logo=  NULL;
		if (!logo)
		{
#ifdef _WIN32
			logo = LICE_LoadPNGFromResource(g_hInst,IDB_SNM,NULL);
#else
			// SWS doesn't work, sorry. :( logo =  LICE_LoadPNGFromNamedResource("SnM.png",NULL);
			logo = NULL;
#endif
		}

		static LICE_CachedFont tmpfont;
		if (!tmpfont.GetHFont())
		{
			LOGFONT lf = {
			  14,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,
			  #ifdef _WIN32
			  "MS Shell Dlg"
			  #else
			  "Arial"
			  #endif
			};
			if (ct) 
				lf = ct->mediaitem_font;
			tmpfont.SetFromHFont(CreateFontIndirect(&lf),LICE_FONT_FLAG_OWNS_HFONT);                 
		}
		tmpfont.SetBkMode(TRANSPARENT);
		if (ct)	tmpfont.SetTextColor(LICE_RGBA_FROMNATIVE(ct->main_text,255));
		else tmpfont.SetTextColor(LICE_RGBA(255,255,255,255));

		int x0=_r.left+10, y0=_r.top+5, h=25;
		WDL_VirtualComboBox* cbVwnd = (WDL_VirtualComboBox*)_parentVwnd->GetChildByID(COMBOID_TYPE);
		if (cbVwnd)
		{
			RECT tr2={x0,y0+3,x0+128,y0+h-2};
			x0 = tr2.right+5;
			cbVwnd->SetPosition(&tr2);
			cbVwnd->SetFont(&tmpfont);
		}

		// "Filter:"
		{
			x0 += 10;
			RECT tr={x0,y0,x0+40,y0+h};
			tmpfont.DrawText(bm, "Filter:", -1, &tr, DT_LEFT | DT_VCENTER);
			x0 = tr.right+5;
		}

#ifdef _WIN32
		x0 += 125; // i.e. width of the filter edit box
#else
		x0 += 145;
#endif

		// FX chains additionnal controls
		cbVwnd = (WDL_VirtualComboBox*)_parentVwnd->GetChildByID(COMBOID_DBLCLICK_TYPE);
		if (cbVwnd)
		{
			if (g_type == SNM_SLOT_TYPE_FX_CHAINS || g_type == SNM_SLOT_TYPE_TR_TEMPLATES)
			{
				RECT tr={x0,y0,x0+48,y0+h};
				tmpfont.DrawText(bm, "Dbl-click:", -1, &tr, DT_LEFT | DT_VCENTER);
				x0 = tr.right+5;

				RECT tr2={x0, y0+3, g_type == SNM_SLOT_TYPE_FX_CHAINS ? x0+60 : x0+148, y0+h-2};
				x0 = tr2.right+5;
				cbVwnd->SetPosition(&tr2);
				cbVwnd->SetFont(&tmpfont);
			}
			cbVwnd->SetVisible(g_type == SNM_SLOT_TYPE_FX_CHAINS || g_type == SNM_SLOT_TYPE_TR_TEMPLATES);
		}

		cbVwnd = (WDL_VirtualComboBox*)_parentVwnd->GetChildByID(COMBOID_DBLCLICK_TO);
		if (cbVwnd)
		{
			if (g_type == SNM_SLOT_TYPE_FX_CHAINS)
			{
				x0+=10;
				RECT tr={x0,y0,x0+60,y0+h};
				tmpfont.DrawText(bm, "To selected:", -1, &tr, DT_LEFT | DT_VCENTER);
				x0 = tr.right+5;

				RECT tr2={x0,y0+3,x0+105,y0+h-2};
				x0 = tr2.right+5;
				cbVwnd->SetPosition(&tr2);
				cbVwnd->SetFont(&tmpfont);
			}
			cbVwnd->SetVisible(g_type == SNM_SLOT_TYPE_FX_CHAINS);
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
					btn->SetCheckState(true);//JFB3

/*JFB3 buttons OK!!
					x0 += 5;
					RECT tr={x0,y0+4,x0+30,y0+20};
					x0 = tr.right+5;
					btn->SetPosition(&tr);
					btn->SetTextLabel("...", 0, &tmpfont);
					btn->SetForceBorder(true);//JFB??
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

		if (logo && (_r.right - _r.left) > (x0+logo->getWidth()))
			LICE_Blit(bm,logo,_r.right-logo->getWidth()-8,y0+3,NULL,0.125f,LICE_BLIT_MODE_ADD|LICE_BLIT_USE_ALPHA);
	}
}

int SNM_ResourceWnd::OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_PAINT:
		{
			RECT r; int sz;
			GetClientRect(m_hwnd,&r);	
			m_parentVwnd.SetPosition(&r);

			ColorTheme* ct = (ColorTheme*)GetColorThemeStruct(&sz);
			if (ct)	m_vwnd_painter.PaintBegin(m_hwnd, ct->tracklistbg_color);
			else m_vwnd_painter.PaintBegin(m_hwnd, LICE_RGBA(0,0,0,255));
			DrawControls(&m_vwnd_painter,r, &m_parentVwnd);
			m_vwnd_painter.PaintEnd();
		}
		break;

		case WM_LBUTTONDOWN:
		{
			SetFocus(g_pResourcesWnd->GetHWND());
			WDL_VWnd *w = m_parentVwnd.VirtWndFromPoint(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
			if (w) 
			{
				w->OnMouseDown(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
#ifdef _SNM_ITT
				//JFB3!!! tests...
				HWND hList = GetDlgItem(m_hwnd, IDC_LIST);
				if (w->GetID() == BUTTONID_ITEMTK_DETAILS && hList)
					ShowWindow(hList, IsWindowVisible(hList) ? SW_HIDE : SW_SHOW);
#endif
			}
		}
		break;

		case WM_LBUTTONUP:
		{
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);
			WDL_VWnd *w = m_parentVwnd.VirtWndFromPoint(x,y);
			if (w) w->OnMouseUp(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
		}
		break;

		case WM_MOUSEMOVE:
		{
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);
			WDL_VWnd *w = m_parentVwnd.VirtWndFromPoint(x,y);
			if (w) w->OnMouseMove(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
		}
		break;
	}
	return 0;
}


void SNM_ResourceWnd::AddSlot(bool _update)
{
	int idx = GetCurList()->GetSize();
	if (GetCurList()->AddSlot() && _update) {
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
				updt = (GetCurList()->InsertEmptySlot(slot) != NULL);
			if (_update && updt) {
				Update();
				SelectBySlot(slot);
			}
		}
		else
			AddSlot(_update); // empty list => add
	}
	else
		AddSlot(_update); // empty list => add

}

void SNM_ResourceWnd::DeleteSelectedSlots(bool _update)
{
	bool updt = false;
	int x=0;
	PathSlotItem* item;
	while((item = (PathSlotItem*)m_pLists.Get(0)->EnumSelected(&x)))
	{
		int slot = GetCurList()->Find(item);
		if (slot >= 0) {
			updt = true;
			GetCurList()->Delete(slot, true);
		}
	}
	if (_update && updt)
		Update();
}


///////////////////////////////////////////////////////////////////////////////

static void menuhook(const char* menustr, HMENU hMenu, int flag)
{
	if (!strcmp(menustr, "Main view") && !flag)
	{
		int cmd = NamedCommandLookup("_S&M_SHOWFXCHAINSLOTS");
		if (cmd > 0)
		{
			int afterCmd = NamedCommandLookup("_SWSCONSOLE");
			AddToMenu(hMenu, "S&&M Resources", cmd, afterCmd > 0 ? afterCmd : 40075);
		}
	}
}

int ResourceViewInit()
{
	// Init the lists of files (ordered by g_type)
	g_filesLists.Empty();
	g_filesLists.Add(&g_fxChainFiles);
	g_filesLists.Add(&g_trTemplateFiles);
#ifdef _SNM_ITT
	g_filesLists.Add(&g_itemTemplateFiles);
#endif

	char shortPath[BUFFER_SIZE], fullPath[BUFFER_SIZE], desc[128], maxSlotCount[16];
	for (int i=0; i < g_filesLists.GetSize(); i++)
	{
		FileSlotList* list = g_filesLists.Get(i);
		if (list)
		{
			// FX chains: 128 slots for "seamless upgrade" (nb of slots was fixed previously)
			GetPrivateProfileString(list->m_resDir.Get(), "MAX_SLOT", "0", maxSlotCount, 16, g_SNMiniFilename.Get()); 
			list->Empty(true);
			int slotCount = atoi(maxSlotCount);
			for (int j=0; j < slotCount; j++) 
			{
				readSlotIniFile(list->m_resDir.Get(), j, shortPath, BUFFER_SIZE, desc, 128);
				GetFullResourcePath(list->m_resDir.Get(), shortPath, fullPath, BUFFER_SIZE);
				list->AddSlot(fullPath, desc);
			}
		}
	}

	g_pResourcesWnd = new SNM_ResourceWnd();

	if (!g_pResourcesWnd || !plugin_register("hookcustommenu", (void*)menuhook))
		return 0;

	return 1;
}

void ResourceViewExit()
{
	char slotCount[16];
	for (int i=0; i < g_filesLists.GetSize(); i++)
	{
		FileSlotList* list = g_filesLists.Get(i);
		if (list)
		{
			_snprintf(slotCount, 16, "%d", list->GetSize());
			WritePrivateProfileString(list->m_resDir.Get(), "MAX_SLOT", slotCount, g_SNMiniFilename.Get()); 
			for (int j=0; j < list->GetSize(); j++)
				saveSlotIniFile(list->m_resDir.Get(), j, list->Get(j)->m_shortPath.Get(), list->Get(j)->m_desc.Get());
		}
	}
	delete g_pResourcesWnd;
	g_pResourcesWnd = NULL;
}

void OpenResourceView(COMMAND_T* _ct) 
{
	if (g_pResourcesWnd) {
		g_pResourcesWnd->Show((g_type == (int)_ct->user) /* i.e toggle */, true);
		g_pResourcesWnd->SetType((int)_ct->user);
		SetFocus(GetDlgItem(g_pResourcesWnd->GetHWND(), IDC_FILTER));
	}
}

bool IsResourceViewDisplayed(COMMAND_T* _ct)
{
	if (g_pResourcesWnd)
		return ((g_type == (int)_ct->user) && g_pResourcesWnd->IsValidWindow());
	return false;
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

void FileSlotList::ClearSlot(int _slot, bool _guiUpdate)
{
	if (_slot >=0 && _slot < GetSize())
	{
//JFB commented: otherwise it leads to multiple confirmation msg with multiple selection in the FX chain view..
//		char cPath[BUFFER_SIZE];
//		readFXChainSlotIniFile(_slot, cPath, BUFFER_SIZE);
//		if (strlen(cPath))
		{
//			char toBeCleared[256] = "";
//			sprintf(toBeCleared, "Are you sure you want to clear the FX chain slot %d?\n(%s)", _slot+1, cPath); 
//			if (MessageBox(GetMainHwnd(), toBeCleared, "S&M - Clear FX Chain slot", /*MB_ICONQUESTION | */MB_OKCANCEL) == 1)
			{
				Get(_slot)->Clear();
				if (_guiUpdate && g_pResourcesWnd)
					g_pResourcesWnd->Update();
			}
		}
	}
}

bool FileSlotList::CheckAndStoreSlot(int _slot, const char* _filename, bool _errMsg)
{
	if (_filename && _slot >= 0 && _slot < GetCurList()->GetSize())
	{
		if (FileExists(_filename)) {
			/*JFB!!! GetCurList()->*/Get(_slot)->SetFullPath(_filename);
			return true;
		}
		else if (_errMsg) {
			char buf[BUFFER_SIZE];
			_snprintf(buf, BUFFER_SIZE, "File not found:\n%s", _filename);
			MessageBox(g_hwndParent, buf, "S&M - Error", MB_OK);
		}
	}
	return false;
}

// Returns false if cancelled
bool FileSlotList::BrowseStoreSlot(int _slot)
{
	bool ok = false;
	if (GetSize())
	{
		char title[128]="", filename[BUFFER_SIZE]="", fileFilter[256]="";
		_snprintf(title, 128, "S&M - Load %s (slot %d)", m_desc.Get(), _slot+1);
		GetFileFilter(fileFilter, 256);
		if (BrowseResourcePath(title, m_resDir.Get(), fileFilter, filename, BUFFER_SIZE, true))
			ok = CheckAndStoreSlot(_slot, filename);
	}
	return ok;
}

bool FileSlotList::LoadOrBrowseSlot(int _slot, bool _errMsg)
{
	// browse if file not found
	if (!Get(_slot)->IsDefault())
		return CheckAndStoreSlot(_slot, Get(_slot)->m_fullPath.Get(), _errMsg);
	else 
		return BrowseStoreSlot(_slot);
}

void FileSlotList::DisplaySlot(int _slot)
{
	if (_slot >= 0 && _slot < GetSize())
	{
#ifdef _WIN32
		WinSpawnNotepad(Get(_slot)->m_fullPath.Get());
#else
		WDL_String chain;
		if (LoadChunk(Get(_slot)->m_fullPath.Get(), &chain))
		{
			char title[64] = "";
			_snprintf(title, 64, "S&M - %s (slot %d)", m_desc.Get(), _slot+1);
			SNM_ShowConsoleMsg(chain.Get(), title);
		}
#endif
	}
}