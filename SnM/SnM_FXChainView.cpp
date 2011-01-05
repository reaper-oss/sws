/******************************************************************************
/ SnM_FXChainView.cpp
/
/ Copyright (c) 2009-2010 Tim Payne (SWS), JF BÈdague 
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
#include "SnM_FXChainView.h"
#ifdef _WIN32
#include "../MediaPool/DragDrop.h" //JFB: move to the trunk?
#endif

#define SAVEWINDOW_POS_KEY "S&M - FX Chain List Save Window Position"

#define CLEAR_MSG					0x110001
#define LOAD_MSG					0x110002
#define LOAD_APPLY_TRACK_MSG		0x110103
#define LOAD_APPLY_TAKE_MSG			0x110104
#define LOAD_APPLY_ALL_TAKES_MSG	0x110105
#define COPY_MSG					0x110106
#define DISPLAY_MSG					0x110107
#define LOAD_PASTE_TRACK_MSG		0x110108
#define LOAD_PASTE_TAKE_MSG			0x110109
#define LOAD_PASTE_ALL_TAKES_MSG	0x11010A
#define ADD_SLOT_MSG				0x11010B
#define INSERT_SLOT_MSG				0x11010C
#define DEL_SLOT_MSG				0x11010D

#define LOAD_APPLY_TRACK_STR		"Load/apply FX chain to selected tracks"
#define LOAD_APPLY_TAKE_STR			"Load/apply FX chain to selected items"
#define LOAD_APPLY_ALL_TAKES_STR	"Load/apply FX chain to selected items, all takes"
#define LOAD_PASTE_TRACK_STR		"Load/paste FX chain to selected tracks"
#define LOAD_PASTE_TAKE_STR			"Load/paste FX chain to selected items"
#define LOAD_PASTE_ALL_TAKES_STR	"Load/paste FX chain to selected items, all takes"

enum
{
  COMBOID_TYPE=1000,
  COMBOID_TO
};

// Globals
/*JFB static*/ SNM_FXChainWnd* g_pFXChainsWnd = NULL;
static SWS_LVColumn g_fxChainListCols[] = { {50,2,"Slot"}, {100,2,"Name"}, {250,2,"Path"}, {200,1,"Description"} };
int g_dblClickType = 0, g_dblClickTo = 0; 


///////////////////////////////////////////////////////////////////////////////
// SNM_FXChainView
///////////////////////////////////////////////////////////////////////////////

SNM_FXChainView::SNM_FXChainView(HWND hwndList, HWND hwndEdit)
:SWS_ListView(hwndList, hwndEdit, 4, g_fxChainListCols, "S&M - FX Chain View State", false) 
{}

void SNM_FXChainView::GetItemText(LPARAM item, int iCol, char* str, int iStrMax)
{
	FXChainSlotItem* pItem = (FXChainSlotItem*)item;
	if (pItem)
	{
		switch (iCol)
		{
			case 0:
				_snprintf(str, iStrMax, "%5.d", pItem->m_slot + 1);
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

void SNM_FXChainView::SetItemText(LPARAM item, int iCol, const char* str)
{
	FXChainSlotItem* pItem = (FXChainSlotItem*)item;
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


void SNM_FXChainView::OnItemDblClk(LPARAM item, int iCol)
{
	FXChainSlotItem* pItem = (FXChainSlotItem*)item;
	if (pItem)
	{
		if (pItem->IsDefault())
		{
			if (g_pFXChainsWnd)
				g_pFXChainsWnd->OnCommand(LOAD_MSG, item);
		}
		else
		{
			switch(g_dblClickTo)
			{
				case 0:
					loadSetPasteTrackFXChain(LOAD_APPLY_TRACK_STR, pItem->m_slot, !g_dblClickType, true);
					break;
				case 1:
					loadSetPasteTakeFXChain(LOAD_APPLY_TAKE_STR, pItem->m_slot, true, !g_dblClickType, true);
					break;
				case 2:
					loadSetPasteTakeFXChain(LOAD_APPLY_ALL_TAKES_STR, pItem->m_slot, false, !g_dblClickType, true);
					break;
				default:
					break;
			}
		}
	}
}

void SNM_FXChainView::GetItemList(WDL_TypedBuf<LPARAM>* pBuf)
{
	if (g_pFXChainsWnd && g_pFXChainsWnd->m_filter.GetLength())
	{
		int iCount = 0;
		LineParser lp(false);
		lp.parse(g_pFXChainsWnd->m_filter.Get());
		for (int i = 0; i < g_fxChainFiles.GetSize(); i++)
		{
			if (!g_fxChainFiles.Get(i)->IsDefault())
			{
				bool match = true;
				for (int j = 0; match && j < lp.getnumtokens(); j++)
					match &= (stristr(g_fxChainFiles.Get(i)->m_shortPath.Get(), lp.gettoken_str(j)) != NULL);
				if (match) {
					pBuf->Resize(++iCount);
					pBuf->Get()[iCount-1] = (LPARAM)g_fxChainFiles.Get(i);
				}
			}
		}
	}
	else
	{
		pBuf->Resize(g_fxChainFiles.GetSize());
		for (int i = 0; i < pBuf->GetSize(); i++)
			pBuf->Get()[i] = (LPARAM)g_fxChainFiles.Get(i);
	}
}

//JFB more than "shared" with Tim's MediaPool => factorize ?

WDL_PtrList<FXChainSlotItem> g_dragItems;// TEMP prevent internal drag'n'drop (fixed in beta brach..)
void SNM_FXChainView::OnBeginDrag(LPARAM item)
{
#ifdef _WIN32
	g_dragItems.Empty();

	LVITEM li;
	li.mask = LVIF_STATE | LVIF_PARAM;
	li.stateMask = LVIS_SELECTED;
	li.iSubItem = 0;
	
	// Get the amount of memory needed for the file list
	int iMemNeeded = 0;
	for (int i = 0; i < ListView_GetItemCount(m_hwndList); i++)
	{
		li.iItem = i;
		ListView_GetItem(m_hwndList, &li);
		if (li.state & LVIS_SELECTED)
		{
			FXChainSlotItem* pItem = (FXChainSlotItem*)li.lParam;
			iMemNeeded += (int)(pItem->m_fullPath.GetLength() + 1);
			g_dragItems.Add(pItem);// TEMP prevent internal drag'n'drop (fixed in beta brach..)
		}
	}
	if (!iMemNeeded)
		return;

	iMemNeeded += sizeof(DROPFILES) + 1;

	HGLOBAL hgDrop = GlobalAlloc (GHND | GMEM_SHARE, iMemNeeded);
	DROPFILES* pDrop = (DROPFILES*)GlobalLock(hgDrop); // 'spose should do some error checking...
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
			FXChainSlotItem* pItem = (FXChainSlotItem*)li.lParam;
			strcpy(pBuf, pItem->m_fullPath.Get());
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
// SNM_FXChainWnd
///////////////////////////////////////////////////////////////////////////////

SNM_FXChainWnd::SNM_FXChainWnd()
:SWS_DockWnd(IDD_SNM_FXCHAINLIST, "FX Chains", "SnMFxChains", 30006, SWSGetCommandID(OpenFXChainView))
{
	if (m_bShowAfterInit)
		Show(false, false);
}

void SNM_FXChainWnd::SelectBySlot(int _slot)
{
	SWS_ListView* lv = m_pLists.Get(0);
	HWND hList = lv ? lv->GetHWND() : NULL;
	if (lv && hList) // this can be called when the view is closed!
	{
		for (int i = 0; i < lv->GetListItemCount(); i++)
		{
			FXChainSlotItem* item = (FXChainSlotItem*)lv->GetListItem(i);
			if (item && item->m_slot == _slot) 
			{
				ListView_SetItemState(hList, -1, 0, LVIS_SELECTED);
				ListView_SetItemState(hList, i, LVIS_SELECTED, LVIS_SELECTED); 
				ListView_EnsureVisible(hList, i, true);
				break;
			}
		}
	}
}

void SNM_FXChainWnd::Update()
{
	if (m_pLists.GetSize())
		m_pLists.Get(0)->Update();
}

void SNM_FXChainWnd::OnInitDlg()
{
	m_resize.init_item(IDC_LIST, 0.0, 0.0, 1.0, 1.0);
	SetWindowLongPtr(GetDlgItem(m_hwnd, IDC_FILTER), GWLP_USERDATA, 0xdeadf00b);

	m_pLists.Add(new SNM_FXChainView(GetDlgItem(m_hwnd, IDC_LIST), GetDlgItem(m_hwnd, IDC_EDIT)));

	// Load prefs 
	g_dblClickType = GetPrivateProfileInt("FXCHAIN_VIEW", "DBLCLICK_TYPE", 0, g_SNMiniFilename.Get());
	g_dblClickTo = GetPrivateProfileInt("FXCHAIN_VIEW", "DBLCLICK_TO", 0, g_SNMiniFilename.Get());

	// WDL GUI init
	m_parentVwnd.SetRealParent(m_hwnd);
	m_cbDblClickType.SetID(COMBOID_TYPE);
	m_cbDblClickType.SetRealParent(m_hwnd);
	m_cbDblClickType.AddItem("Apply");
	m_cbDblClickType.AddItem("Paste");
	m_cbDblClickType.SetCurSel(g_dblClickType);
	m_parentVwnd.AddChild(&m_cbDblClickType);

	m_cbDblClickTo.SetID(COMBOID_TO);
	m_cbDblClickTo.SetRealParent(m_hwnd);
	m_cbDblClickTo.AddItem("Tracks");
	m_cbDblClickTo.AddItem("Items");
	m_cbDblClickTo.AddItem("Items, all takes");
	m_cbDblClickTo.SetCurSel(g_dblClickTo);
	m_parentVwnd.AddChild(&m_cbDblClickTo);

	// This restores the text filter when docking/undocking
	SetDlgItemText(GetHWND(), IDC_FILTER, m_filter.Get());

	Update();
}

void SNM_FXChainWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	int x=0;
	FXChainSlotItem* item = (FXChainSlotItem*)m_pLists.Get(0)->EnumSelected(&x);
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
			DeleteSelectedSlot(true);
			break;

		case LOAD_MSG:
			if (item && browseStoreFXChain(item->m_slot))
				Update();
			break;
		case CLEAR_MSG: 
		{
			bool updt = false;
			while(item) {
				clearFXChainSlot(item->m_slot, false);
				updt = true;
				item = (FXChainSlotItem*)m_pLists.Get(0)->EnumSelected(&x);
			}
			if (updt) Update();
			break;
		}
		case COPY_MSG:
			if (item)
				copySlotToClipBoard(item->m_slot);
			break;
		case DISPLAY_MSG:
			if (item)
				displayFXChain(item->m_slot);
			break;

		// Apply
		case LOAD_APPLY_TRACK_MSG:
			if (item) {
				loadSetPasteTrackFXChain(LOAD_APPLY_TRACK_STR, item->m_slot, true, !item->IsDefault());
				Update();
			}
			break;
		case LOAD_APPLY_TAKE_MSG:
			if (item) {
				loadSetPasteTakeFXChain(LOAD_APPLY_TAKE_STR, item->m_slot, true, true, !item->IsDefault());
				Update();
			}
			break;
		case LOAD_APPLY_ALL_TAKES_MSG:
			if (item) {
				loadSetPasteTakeFXChain(LOAD_APPLY_ALL_TAKES_STR, item->m_slot, false, true, !item->IsDefault());
				Update();
			}
			break;

		// Paste
		case LOAD_PASTE_TRACK_MSG:
			if (item) {
				loadSetPasteTrackFXChain(LOAD_PASTE_TRACK_STR, item->m_slot, false, !item->IsDefault());
				Update();
			}
			break;
		case LOAD_PASTE_TAKE_MSG:
			if (item) {
				loadSetPasteTakeFXChain(LOAD_PASTE_TAKE_STR, item->m_slot, true, false, !item->IsDefault());
				Update();
			}
			break;
		case LOAD_PASTE_ALL_TAKES_MSG:
			if (item) {
				loadSetPasteTakeFXChain(LOAD_PASTE_ALL_TAKES_STR, item->m_slot, false, false, !item->IsDefault());
				Update();
			}
			break;
		
		default:
		{
			// WDL GUI
			if (HIWORD(wParam)==CBN_SELCHANGE && LOWORD(wParam)==COMBOID_TYPE)
				g_dblClickType = m_cbDblClickType.GetCurSel();

			else if (HIWORD(wParam)==CBN_SELCHANGE && LOWORD(wParam)==COMBOID_TO)
				g_dblClickTo = m_cbDblClickTo.GetCurSel();

			// default
			else 
				Main_OnCommand((int)wParam, (int)lParam);

			break;
		}
	}
}

HMENU SNM_FXChainWnd::OnContextMenu(int x, int y)
{
	HMENU hMenu = NULL;
	int iCol;
	LPARAM item = m_pLists.Get(0)->GetHitItem(x, y, &iCol);
	FXChainSlotItem* pItem = (FXChainSlotItem*)item;
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
		AddToMenu(hMenu, SWS_SEPARATOR, 0);
		AddToMenu(hMenu, "Copy", COPY_MSG, -1, false, enabled);
		AddToMenu(hMenu, SWS_SEPARATOR, 0);
		AddToMenu(hMenu, LOAD_APPLY_TRACK_STR, LOAD_APPLY_TRACK_MSG);
		AddToMenu(hMenu, LOAD_PASTE_TRACK_STR, LOAD_PASTE_TRACK_MSG);
		AddToMenu(hMenu, SWS_SEPARATOR, 0);
		AddToMenu(hMenu, LOAD_APPLY_TAKE_STR,		LOAD_APPLY_TAKE_MSG);
		AddToMenu(hMenu, LOAD_APPLY_ALL_TAKES_STR,	LOAD_APPLY_ALL_TAKES_MSG);
		AddToMenu(hMenu, LOAD_PASTE_TAKE_STR,		LOAD_PASTE_TAKE_MSG);
		AddToMenu(hMenu, LOAD_PASTE_ALL_TAKES_STR,	LOAD_PASTE_ALL_TAKES_MSG);
		AddToMenu(hMenu, SWS_SEPARATOR, 0);
		AddToMenu(hMenu, "Display FX chain...", DISPLAY_MSG, -1, false, enabled);
	}
	else 
	{
		hMenu = CreatePopupMenu();
		AddToMenu(hMenu, "Add slot", ADD_SLOT_MSG, -1, false, !m_filter.GetLength() ? MF_ENABLED : MF_DISABLED);
	}
	return hMenu;
}

void SNM_FXChainWnd::OnDestroy() 
{
	// save prefs
	char cType[2], cTo[2];
	sprintf(cType, "%d", m_cbDblClickType.GetCurSel());
	sprintf(cTo, "%d", m_cbDblClickTo.GetCurSel());
	WritePrivateProfileString("FXCHAIN_VIEW", "DBLCLICK_TYPE", cType, g_SNMiniFilename.Get()); 
	WritePrivateProfileString("FXCHAIN_VIEW", "DBLCLICK_TO", cTo, g_SNMiniFilename.Get()); 

	m_cbDblClickType.Empty();
	m_cbDblClickTo.Empty();
	m_parentVwnd.RemoveAllChildren(false);
	m_parentVwnd.SetRealParent(NULL);
}

int SNM_FXChainWnd::OnKey(MSG* msg, int iKeyState) 
{
	if (msg->message == WM_KEYDOWN && msg->wParam == VK_DELETE && !iKeyState)
	{
		DeleteSelectedSlot(true);
		return 1;
	}
	return 0;
}

//JFB more than "shared" with Tim's MediaPool => factorize ?
void SNM_FXChainWnd::OnDroppedFiles(HDROP h)
{
	// TEMP prevent internal drag'n'drop (fixed in beta brach..)
	if (g_dragItems.GetSize())
	{
		g_dragItems.Empty();
		DragFinish(h);
		return;
	}

	// Check to see if we dropped on a group
	POINT pt;
	DragQueryPoint(h, &pt);

	RECT r; // ClientToScreen doesn't work right, wtf?
	GetWindowRect(m_hwnd, &r);
	pt.x += r.left;
	pt.y += r.top;

	FXChainSlotItem* pItem = (FXChainSlotItem*)m_pLists.Get(0)->GetHitItem(pt.x, pt.y, NULL);
	if (pItem)
	{
		int slot = pItem->m_slot;
		SelectBySlot(slot);

		char cFile[BUFFER_SIZE];
		int iFiles = DragQueryFile(h, 0xFFFFFFFF, NULL, 0);
		for (int i = 0; i < iFiles; i++)
			InsertAtSelectedSlot(false);

		// re-sync pItem (has been moved down)
		pItem = g_fxChainFiles.Get(slot); 
		for (int i = 0; pItem && i < iFiles; i++)
		{
			slot = pItem->m_slot;
			DragQueryFile(h, i, cFile, BUFFER_SIZE);
			char* pExt = strrchr(cFile, '.');
			if (pExt && !_stricmp(pExt+1, "rfxchain"))
			{
				checkAndStoreFXChain(slot, cFile);
				pItem = g_fxChainFiles.Get(slot+1); 
			}
		}
		Update();
	}
	DragFinish(h);
}

static void DrawControls(WDL_VWnd_Painter *_painter, RECT _r, WDL_VWnd* _parentVwnd)
{
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

		// "Filter:"
		{
			RECT tr={x0,y0,x0+40,y0+h};
			tmpfont.DrawText(bm, "Filter:", -1, &tr, DT_LEFT | DT_VCENTER);
			x0 = tr.right+5;
		}

#ifdef _WIN32
		x0=_r.left+180, y0=_r.top+5, h=25;
#else
		x0=_r.left+200, y0=_r.top+5, h=25;
#endif
		// Dropdowns
		WDL_VirtualComboBox* cbVwnd = (WDL_VirtualComboBox*)_parentVwnd->GetChildByID(COMBOID_TYPE);
		if (cbVwnd)
		{
			RECT tr={x0,y0,x0+48,y0+h};
			tmpfont.DrawText(bm, "Dbl-click:", -1, &tr, DT_LEFT | DT_VCENTER);
			x0 = tr.right+5;

			RECT tr2={x0,y0+3,x0+60,y0+h-2};
			x0 = tr2.right+5;
			cbVwnd->SetPosition(&tr2);
			cbVwnd->SetFont(&tmpfont);
		}

		cbVwnd = (WDL_VirtualComboBox*)_parentVwnd->GetChildByID(COMBOID_TO);
		if (cbVwnd)
		{
			x0+=5;
			RECT tr={x0,y0,x0+60,y0+h};
			tmpfont.DrawText(bm, "To selected:", -1, &tr, DT_LEFT | DT_VCENTER);
			x0 = tr.right+5;

			RECT tr2={x0,y0+3,x0+105,y0+h-2};
			x0 = tr2.right+5;
			cbVwnd->SetPosition(&tr2);
			cbVwnd->SetFont(&tmpfont);
		}

		_painter->PaintVirtWnd(_parentVwnd, 0);

		if (logo && (_r.right - _r.left) > (x0+logo->getWidth()))
			LICE_Blit(bm,logo,_r.right-logo->getWidth()-8,y0+3,NULL,0.125f,LICE_BLIT_MODE_ADD|LICE_BLIT_USE_ALPHA);
	}
}

int SNM_FXChainWnd::OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
		SetFocus(g_pFXChainsWnd->GetHWND());
		WDL_VWnd *w = m_parentVwnd.VirtWndFromPoint(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
		if (w) w->OnMouseDown(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
		break;
	}
	return 0;
}


void SNM_FXChainWnd::AddSlot(bool _update)
{
	int idx = g_fxChainFiles.GetSize();
	g_fxChainFiles.Add(new FXChainSlotItem(idx, "", ""));

	if (_update) {
		Update();
		SelectBySlot(idx);
	}
}

void SNM_FXChainWnd::InsertAtSelectedSlot(bool _update)
{
	if (g_fxChainFiles.GetSize())
	{
		bool updt = false;
		FXChainSlotItem* item = (FXChainSlotItem*)m_pLists.Get(0)->EnumSelected(NULL);
		if (item)
		{
			int idx = g_fxChainFiles.Find(item);
			if (idx >= 0) {
				updt = true;
				g_fxChainFiles.Insert(idx, new FXChainSlotItem(idx, "", ""));
				for (int i=idx+1; i < g_fxChainFiles.GetSize(); i++)
					g_fxChainFiles.Get(i)->m_slot++;
			}
			if (_update && updt) {
				Update();
				SelectBySlot(idx);
			}
		}
		else
			AddSlot(_update); // empty list => add
	}
	else
		AddSlot(_update); // empty list => add

}

void SNM_FXChainWnd::DeleteSelectedSlot(bool _update)
{
	bool updt = false;
	int x=0;
	FXChainSlotItem* item;
	while((item = (FXChainSlotItem*)m_pLists.Get(0)->EnumSelected(&x)))
	{
		int idx = g_fxChainFiles.Find(item);
		if (idx >= 0)
		{
			updt = true;
			g_fxChainFiles.Delete(idx, true);
			for (int i=idx; i < g_fxChainFiles.GetSize(); i++)
				g_fxChainFiles.Get(i)->m_slot--;
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
			AddToMenu(hMenu, "S&&M FX Chains", cmd);
	}
}

int FXChainViewInit()
{
	char shortPath[BUFFER_SIZE], fullPath[BUFFER_SIZE], desc[128], maxSlotCount[16];

	// Default: 128 slots for "seamless upgrade" (nb of slots was fixed previously)
	GetPrivateProfileString("FXCHAIN", "MAX_SLOT", "128", maxSlotCount, 16, g_SNMiniFilename.Get()); 

	g_fxChainFiles.Empty(true);
	int fxChainSlotCount = atoi(maxSlotCount); 
	for (int i=0; i < fxChainSlotCount; i++) {
		readFXChainSlotIniFile(i, shortPath, BUFFER_SIZE, desc, 128);
		GetFullResourcePath("FXChains", shortPath, fullPath, BUFFER_SIZE);
		g_fxChainFiles.Add(new FXChainSlotItem(i, fullPath, desc));
	}

	g_pFXChainsWnd = new SNM_FXChainWnd();

	if (!g_pFXChainsWnd || !plugin_register("hookcustommenu", (void*)menuhook))
		return 0;

	return 1;
}

void FXChainViewExit() 
{
	char slotCount[16];
	_snprintf(slotCount, 16, "%d", g_fxChainFiles.GetSize());
	WritePrivateProfileString("FXCHAIN", "MAX_SLOT", slotCount, g_SNMiniFilename.Get()); 
	for (int i=0; i < g_fxChainFiles.GetSize(); i++)
		saveFXChainSlotIniFile(i, g_fxChainFiles.Get(i)->m_shortPath.Get(), g_fxChainFiles.Get(i)->m_desc.Get());

	delete g_pFXChainsWnd;
}

void OpenFXChainView(COMMAND_T*) {
	g_pFXChainsWnd->Show(true, true);
}

bool IsFXChainViewEnabled(COMMAND_T*){
	return g_pFXChainsWnd->IsValidWindow();
}
