/******************************************************************************
/ Autocolor.cpp
/
/ Copyright (c) 2010-2012 Tim Payne (SWS), Jeffos, wol
/ https://code.google.com/p/sws-extension
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
#include "Color.h"
#include "Autocolor.h"
#include "../Breeder/BR_Util.h"
#include "../SnM/SnM_ChunkParserPatcher.h"
#include "../SnM/SnM_Dlg.h"
#include "../SnM/SnM_Marker.h"
#include "../SnM/SnM_Util.h"
#include "../SnM/SnM_Window.h"
#include "../Wol/wol_Util.h"
#include "../reaper/localize.h"
#include "../../WDL/projectcontext.h"

#define PRI_UP_MSG		0x10000
#define PRI_DOWN_MSG	0x10001
#define INSACTION_MSG	0x10002
#define CLEARACTION_MSG 0x10003
#define TYPETYPE_MSG	0x100F0
#define FILTERTYPE_MSG	0x10100
#define COLORTYPE_MSG	0x10110
#define LOAD_ICON_MSG	0x110000
#define CLEAR_ICON_MSG	0x110001


// INI params
#define AC_ENABLE_KEY	"AutoColorEnable"
#define ACM_ENABLE_KEY	"AutoColorMarkerEnable"
#define ACR_ENABLE_KEY	"AutoColorRegionEnable"
#define AI_ENABLE_KEY	"AutoIconEnable"
#define AAT_ENABLE_KEY	"AutoActionTrackEnable"
#define TRAUTOSEL_ENABLE_KEY "WOLTrackAutoSelectionEn"
#define AUTOCOL_DISABLE_KEY "WOLAutoColorDisable"
#define AUTOICON_DISABLE_KEY "WOLAutoIconDisable"
#define AUTOACTION_DISABLE_KEY "WOLAutoActionDisable"
#define AC_COUNT_KEY	"AutoColorCount"
#define AC_ITEM_KEY		"AutoColor %d"



enum { AC_ANY=0, AC_UNNAMED, AC_FOLDER, AC_CHILDREN, AC_RECEIVE, AC_MASTER, NUM_FILTERTYPES };
enum { AC_RGNANY=0, AC_RGNUNNAMED, NUM_RGNFILTERTYPES };
enum { AC_CUSTOM, AC_GRADIENT, AC_RANDOM, AC_NONE, AC_PARENT, AC_IGNORE, NUM_COLORTYPES };
enum { COL_ID=0, COL_TYPE, COL_FILTER, COL_COLOR, COL_ICON, COL_ACTION, COL_COUNT };
enum { AC_TRACK=0, AC_MARKER, AC_REGION, NUM_TYPETYPES }; // keep this order and 2^ values
														  // (values used as masks => adding a 4th type would require another solution)

enum { WNDID_LR = 2000, BTNID_L, BTNID_R };
enum { COLTRL_ID = 0, COLTRL_COLOR, COLTRL_NAME, COLTRL_AC, COLTRL_AI, COLTRL_AA, COLTRL_LASTACTION, COLTRL_COUNT };

// Larger allocs for localized strings..
// !WANT_LOCALIZE_STRINGS_BEGIN:sws_DLG_115
static SWS_LVColumn g_cols[] = { { 25, 0, "#" }, { 25, 0, "Type" }, { 185, 1, "Filter" }, { 70, 1, "Color" }, { 200, 2, "Icon" }, { 50, 2, "Action" } };
static SWS_LVColumn g_TrListViewCols[] = { { 25, 0, "#" }, { 70, 0, "Color" }, { 70, 0, "Name" }, { 70, 2, "Auto color" }, { 70, 2, "Auto icon", }, { 70, 2, "Auto action" }, { 150, 0, "Last executed action" } };
static const char cTypes[][32] = {"Track", "Marker", "Region" }; // keep this order, see above
static const char cFilterTypes[][32] = { "(any)", "(unnamed)", "(folder)", "(children)", "(receive)", "(master)" };
static const char cColorTypes[][32] = { "Custom", "Gradient", "Random", "None", "Parent", "Ignore" };
// !WANT_LOCALIZE_STRINGS_END


// Globals
static SWS_AutoColorWnd* g_pACWnd = NULL;
static WDL_PtrList<SWS_RuleItem> g_pACItems;
static SWSProjConfig<WDL_PtrList<SWS_RuleTrack> > g_pACTracks;
static bool g_bACEnabled = false;
static bool g_bACREnabled = false;
static bool g_bACMEnabled = false;
static bool g_bAIEnabled = false;
static bool g_bAATEnabled = false;
static bool g_bTrackAutoSelectionEn = true;
static bool g_bAutoColorDisable = false;
static bool g_bAutoIconDisable = false;
static bool g_bAutoActionDisable = false;
static int g_listsLastState = 0, g_listsState = 0; // 0=display both list views, 1=left only, -1=right only
static RECT g_origRectL = { 0, 0, 0, 0 };
static RECT g_origRectR = { 0, 0, 0, 0 };
static WDL_String g_ACIni;

// Register to marker/region updates
class AC_MarkerRegionListener : public SNM_MarkerRegionListener {
public:
	AC_MarkerRegionListener() : SNM_MarkerRegionListener() {}
	void NotifyMarkerRegionUpdate(int _updateFlags) { AutoColorMarkerRegion(false, _updateFlags); }
};

AC_MarkerRegionListener g_mkrRgnListener;

// Optimized/custom version of RegisterToMarkerRegionUpdates()
// (to avoid useless polling behind the scene)
bool ACRegisterUnregisterToMarkerRegionUpdates()
{
	if (g_bACMEnabled || g_bACREnabled)
		for (int i = 0; i < g_pACItems.GetSize(); i++)
			if (SWS_RuleItem* rule = (SWS_RuleItem*)g_pACItems.Get(i))
				if ((g_bACMEnabled && rule->m_type == AC_MARKER) || (g_bACREnabled && rule->m_type == AC_REGION)) {
					RegisterToMarkerRegionUpdates(&g_mkrRgnListener);
					return true; // do not use the above returned value
				}
	UnregisterToMarkerRegionUpdates(&g_mkrRgnListener);
	return false; // do not use the above returned value
}

// Prototypes
void AutoColorSaveState();

////////////////////////////////////////////////////////////////////////////////////////////////
// SWS_AutoColorView
////////////////////////////////////////////////////////////////////////////////////////////////
SWS_AutoColorView::SWS_AutoColorView(HWND hwndList, HWND hwndEdit)
:SWS_ListView(hwndList, hwndEdit, COL_COUNT, g_cols, "AutoColorViewState", false, "sws_DLG_115")
{
}

void SWS_AutoColorView::GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax)
{
	if (str)
		*str = '\0';

	SWS_RuleItem* pItem = (SWS_RuleItem*)item;
	if (!pItem)
		return;

	switch (iCol)
	{
	case COL_ID:
		_snprintf(str, iStrMax, "%d", g_pACItems.Find(pItem) + 1);
		break;
	case COL_TYPE:
		lstrcpyn(str, __localizeFunc(cTypes[pItem->m_type],"sws_DLG_115",LOCALIZE_FLAG_NOCACHE), iStrMax);
		break;
	case COL_FILTER:
		{
			// internal str? => localize for display
			for (int i=0; i<NUM_FILTERTYPES; i++)
				if (!strcmp(pItem->m_str_filter.Get(), cFilterTypes[i])) {
					lstrcpyn(str, __localizeFunc(pItem->m_str_filter.Get(),"sws_DLG_115",LOCALIZE_FLAG_NOCACHE), iStrMax);
					return;
				}
			// user defined?
			lstrcpyn(str, pItem->m_str_filter.Get(), iStrMax);
		}
		break;
	case COL_COLOR:
		if (pItem->m_color<0 && (-pItem->m_color-1) < __ARRAY_SIZE(cColorTypes))
			lstrcpyn(str, __localizeFunc(cColorTypes[-pItem->m_color-1],"sws_DLG_115",LOCALIZE_FLAG_NOCACHE), iStrMax);
		else
#ifdef _WIN32
			_snprintf(str, iStrMax, "0x%02x%02x%02x", pItem->m_color & 0xFF, (pItem->m_color >> 8) & 0xFF, (pItem->m_color >> 16) & 0xFF); //Brado: I think this is in reverse. Ini file dec to hex conversion (correct value for color) shows opposite of what this outputs to screen. e.g. 0xFFFF80 instead of 0x80FFFF
#else
			_snprintf(str, iStrMax, "0x%06x", pItem->m_color);
#endif
		break;
	case COL_ICON:
		// hide icon depending on the current rule type
		// (this type can switch from track (w/ icon) to marker, for ex.)
		if (pItem->m_type == AC_TRACK)
			lstrcpyn(str, pItem->m_icon.Get(), iStrMax);
		break;
	case COL_ACTION:
		lstrcpyn(str, pItem->m_actionName.Get(), iStrMax);
		break;
	}
}

//JFB: COL_TYPE must be edited via context menu, not by hand (mismatch between internal/localized strings)
void SWS_AutoColorView::SetItemText(SWS_ListItem* item, int iCol, const char* str)
{
	SWS_RuleItem* pItem = (SWS_RuleItem*)item;
	if (!pItem)
		return;

	switch(iCol)
	{
		case COL_FILTER:
		{
			for (int i = 0; i < g_pACItems.GetSize(); i++)
				if (g_pACItems.Get(i) != pItem)
					if (g_pACItems.Get(i)->m_type == pItem->m_type && !strcmp(g_pACItems.Get(i)->m_str_filter.Get(), str))
					{
						MessageBox(GetParent(m_hwndList), __LOCALIZE("Autocolor entry with that name already exists.", "sws_DLG_115"), __LOCALIZE("SWS - Error", "sws_DLG_115"), MB_OK);
						return;
					}

			if (strlen(str))
				pItem->m_str_filter.Set(str);
			break;
		}
		case COL_COLOR:
		{
			int iNewCol = strtol(str, NULL, 0);
			pItem->m_color = RGB((iNewCol >> 16) & 0xFF, (iNewCol >> 8) & 0xFF, iNewCol & 0xFF);
			break;
		}
	}
	g_pACWnd->Update();
}

void SWS_AutoColorView::GetItemList(SWS_ListItemList* pList)
{
	for (int i = 0; i < g_pACItems.GetSize(); i++)
		pList->Add((SWS_ListItem*)g_pACItems.Get(i));
}

void SWS_AutoColorView::OnItemDblClk(SWS_ListItem* item, int iCol)
{
	SWS_RuleItem* pItem = (SWS_RuleItem*)item;
	if (pItem && iCol == COL_ICON)
		g_pACWnd->OnCommand(LOAD_ICON_MSG, (LPARAM)pItem);
}

void SWS_AutoColorView::OnItemSelChanged(SWS_ListItem* item, int iState)
{
	g_pACWnd->Update();
}

void SWS_AutoColorView::OnBeginDrag(SWS_ListItem* item)
{
	if (abs(m_iSortCol) == (COL_ID+1)) //1-based
		SetCapture(GetParent(m_hwndList));
}

void SWS_AutoColorView::OnDrag()
{
	POINT p;
	GetCursorPos(&p);
	SWS_RuleItem* hitItem = (SWS_RuleItem*)GetHitItem(p.x, p.y, NULL);
	if (hitItem)
	{
		int iNewPriority = g_pACItems.Find(hitItem);
		int iSelPriority;

		WDL_PtrList<SWS_RuleItem> draggedItems;
		int x = 0;
		SWS_RuleItem* selItem;
		while((selItem = (SWS_RuleItem*)EnumSelected(&x)))
		{
			iSelPriority = g_pACItems.Find(selItem);
			if (iNewPriority == iSelPriority)
				return;
			draggedItems.Add(selItem);
		}

		// Remove the dragged items and then readd them
		// Switch order of add based on direction of drag & sort order
		bool bDir = iNewPriority > iSelPriority;
		if (m_iSortCol < 0)
			bDir = !bDir;
		for (int i = bDir ? 0 : draggedItems.GetSize()-1; bDir ? i < draggedItems.GetSize() : i >= 0; bDir ? i++ : i--)
		{
			int index = g_pACItems.Find(draggedItems.Get(i));
			g_pACItems.Delete(index);
			g_pACItems.Insert(iNewPriority, draggedItems.Get(i));
		}

		g_pACWnd->Update();

		for (int i=0; i < draggedItems.GetSize(); i++)
			SelectByItem((SWS_ListItem*)draggedItems.Get(i), i==0, i==0);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////
// SWS_AutoColorTrackListView
////////////////////////////////////////////////////////////////////////////////////////////////
SWS_AutoColorTrackListView::SWS_AutoColorTrackListView(HWND hwndList, HWND hwndEdit)
	:SWS_ListView(hwndList, hwndEdit, COLTRL_COUNT, g_TrListViewCols, "AutoColorTrackListViewState", false, "sws_DLG_115")
{
}

void SWS_AutoColorTrackListView::GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax)
{
	if (str)
		*str = '\0';

	SWS_RuleTrack* pTrack = (SWS_RuleTrack*)item;
	if (!pTrack)
		return;

	switch (iCol)
	{
	case COLTRL_ID:
		_snprintf(str, iStrMax, "%d", pTrack->m_id);
		break;
	case COLTRL_COLOR:
	{
		int _col = *(int*)GetSetMediaTrackInfo(pTrack->m_pTr, "I_CUSTOMCOLOR", NULL);
#ifdef _WIN32
		_snprintf(str, iStrMax, "0x%02x%02x%02x", _col & 0xFF, (_col >> 8) & 0xFF, (_col >> 16) & 0xFF);
#else
		_snprintf(str, iStrMax, "0x%06x", _col);
#endif
		break;
	}
	case COLTRL_NAME:
		lstrcpyn(str, (char*)GetSetMediaTrackInfo(pTrack->m_pTr, "P_NAME", NULL), iStrMax);
		break;
	case COLTRL_AC:
		_snprintf(str, iStrMax, "%s", pTrack->m_bACEnabled ? UTF8_BULLET : "");
		break;
	case COLTRL_AI:
		_snprintf(str, iStrMax, "%s", pTrack->m_bAIEnabled ? UTF8_BULLET : "");
		break;
	case COLTRL_AA:
		_snprintf(str, iStrMax, "%s", pTrack->m_bAAEnabled ? UTF8_BULLET : "");
		break;
	case COLTRL_LASTACTION:
		lstrcpyn(str, pTrack->m_lastExecAction.Get(), iStrMax);
		break;
	}
}

void SWS_AutoColorTrackListView::OnItemBtnClk(SWS_ListItem* item, int iCol, int iKeyState)
{
	SWS_RuleTrack* pTrack = (SWS_RuleTrack*)item;
	if (!item)
		return;

	switch (iCol)
	{
	case COLTRL_AC:
		pTrack->m_bACEnabled = !pTrack->m_bACEnabled;
		break;
	case COLTRL_AI:
		pTrack->m_bAIEnabled = !pTrack->m_bAIEnabled;
		break;
	case COLTRL_AA:
		pTrack->m_bAAEnabled = !pTrack->m_bAAEnabled;
		break;
	}
	AutoColorTrackListUpdate();
}

void SWS_AutoColorTrackListView::GetItemList(SWS_ListItemList* pList)
{
	if (g_pACTracks.Get()->GetSize())
		for (int i = 0; i < g_pACTracks.Get()->GetSize(); i++)
			if (g_pACTracks.Get()->Get(i)->m_id)   //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!workaround: master gets always added
				pList->Add((SWS_ListItem*)g_pACTracks.Get()->Get(i));
}

void SWS_AutoColorTrackListView::OnBeginDrag(SWS_ListItem* item)
{
	POINT p;
	GetCursorPos(&p);
	int iCol;
	GetHitItem(p.x, p.y, &iCol);
	if (iCol == COLTRL_AC || iCol == COLTRL_AI || iCol == COLTRL_AA)
		m_iBeginDragCol = iCol;
	else
		m_iBeginDragCol = -2;
	m_pOldHitItem = NULL;
	SetCapture(GetParent(m_hwndList));
}

void SWS_AutoColorTrackListView::OnDrag()
{
	POINT p;
	GetCursorPos(&p);
	int iCol = -2;
	SWS_RuleItem* hitItem = (SWS_RuleItem*)GetHitItem(p.x, p.y, &iCol);
	if (hitItem && hitItem != m_pOldHitItem && m_iBeginDragCol > 0 && iCol == m_iBeginDragCol)
	{
		SWS_RuleTrack* pTrack = (SWS_RuleTrack*)hitItem;
		switch (m_iBeginDragCol)
		{
		case COLTRL_AC:
			pTrack->m_bACEnabled = !pTrack->m_bACEnabled;
			break;
		case COLTRL_AI:
			pTrack->m_bAIEnabled = !pTrack->m_bAIEnabled;
			break;
		case COLTRL_AA:
			pTrack->m_bAAEnabled = !pTrack->m_bAAEnabled;
			break;
		}
		AutoColorTrackListUpdate();
		m_pOldHitItem = hitItem;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////
// SWS_AutoColorWnd
////////////////////////////////////////////////////////////////////////////////////////////////
SWS_AutoColorWnd::SWS_AutoColorWnd()
:SWS_DockWnd(IDD_AUTOCOLOR, __LOCALIZE("Auto Color/Icon/Action","sws_DLG_115"), "SWSAutoColor", SWSGetCommandID(OpenAutoColor))
#ifndef _WIN32
	,m_bSettingColor(false)
#endif
{
	// Must call SWS_DockWnd::Init() to restore parameters and open the window if necessary
	Init();
}

void SWS_AutoColorWnd::Update()
{
	if (IsValidWindow())
	{
		SetDlgItemText(m_hwnd, IDC_APPLY, (g_bACEnabled || g_bACMEnabled || g_bACREnabled || g_bAIEnabled || g_bAATEnabled) ? __LOCALIZE("Force", "sws_DLG_115") : __LOCALIZE("Apply", "sws_DLG_115"));

		// Redraw the owner drawn button
#ifdef _WIN32
		RedrawWindow(GetDlgItem(m_hwnd, IDC_COLOR), NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
#else
		InvalidateRect(GetDlgItem(m_hwnd, IDC_COLOR), NULL, 0);
#endif

		if (m_pLists.GetSize())   // Track list updated only when scheduled update is called and then triggered by timer
			m_pLists.Get(0)->Update();

		AutoColorSaveState();
		AutoColorTrack(false);
		if (ACRegisterUnregisterToMarkerRegionUpdates())
			AutoColorMarkerRegion(false);
	}
}

void SWS_AutoColorWnd::OnInitDlg()
{
	m_resize.init_item(IDC_LIST,      0.0, 0.0, 0.5, 1.0);
	m_resize.init_item(IDC_LISTTR,	  0.5, 0.0, 1.0, 1.0);
	m_resize.init_item(IDC_ADD,       0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_REMOVE,    0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_COLOR,     0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_OPTIONS,   1.0, 1.0, 1.0, 1.0);
	m_resize.init_item(IDC_APPLY,     1.0, 1.0, 1.0, 1.0);

	m_pView = new SWS_AutoColorView(GetDlgItem(m_hwnd, IDC_LIST), GetDlgItem(m_hwnd, IDC_EDIT));
	m_pTrListView = new SWS_AutoColorTrackListView(GetDlgItem(m_hwnd, IDC_LISTTR), GetDlgItem(m_hwnd, IDC_EDIT));
	m_pLists.Add(m_pView);
	m_pLists.Add(m_pTrListView);
	g_origRectL = m_resize.get_item(IDC_LIST)->real_orig;
	g_origRectR = m_resize.get_item(IDC_LISTTR)->real_orig;
	g_listsState = g_listsLastState = 0;

	m_vwnd_painter.SetGSC(WDL_STYLE_GetSysColor);
	m_parentVwnd.SetRealParent(m_hwnd);

	m_btnLeft.SetID(BTNID_L);
	m_tinyLRbtns.AddChild(&m_btnLeft);
	m_btnRight.SetID(BTNID_R);
	m_tinyLRbtns.AddChild(&m_btnRight);
	m_tinyLRbtns.SetID(WNDID_LR);
	m_parentVwnd.AddChild(&m_tinyLRbtns);

	SetTimer(m_hwnd, 2, 150, NULL);   // Timer 2 used to update track list view

	Update();
}

//JFB TODO? handle multi-selection
void SWS_AutoColorWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
		case IDC_APPLY:
			AutoColorTrack(true);
			AutoColorMarkerRegion(true);
			break;
		case IDC_OPTIONS:
		{
			HWND hwnd = GetDlgItem(m_hwnd, IDC_OPTIONS);
			RECT r;  GetClientRect(hwnd, &r);
			ClientToScreen(hwnd, (LPPOINT)&r);
			ClientToScreen(hwnd, ((LPPOINT)&r)+1);
			SendMessage(m_hwnd, WM_CONTEXTMENU, 0, MAKELPARAM((UINT)(r.left), (UINT)(r.bottom+SNM_1PIXEL_Y)));
			break;
		}
		case IDC_ADD:
			// default options when we add a new row
			g_pACItems.Add(new SWS_RuleItem(AC_TRACK, "(name)", -AC_NONE-1, "-1", ""));
			Update();
			break;
		case IDC_REMOVE:
		{
			int x = 0;
			SWS_RuleItem* item;
			while((item = (SWS_RuleItem*)m_pLists.Get(0)->EnumSelected(&x)))
			{
				int idx = g_pACItems.Find(item);
				if (idx >= 0)
					g_pACItems.Delete(idx);
			}
			Update();
			break;
		}
		case IDC_COLOR:
		{
			SWS_RuleItem* item = (SWS_RuleItem*)m_pLists.Get(0)->EnumSelected(NULL);
			if (item)
			{
				// Display the color picker
#ifdef _WIN32
				CHOOSECOLOR cc;
				memset(&cc, 0, sizeof(CHOOSECOLOR));
				cc.lStructSize = sizeof(CHOOSECOLOR);
				cc.hwndOwner = m_hwnd;
				UpdateCustomColors();
				cc.lpCustColors = g_custColors;
				cc.Flags = CC_FULLOPEN | CC_RGBINIT;
				cc.rgbResult = item->m_color;
				if (ChooseColor(&cc))
				{
					int x = 0;
					while ((item = (SWS_RuleItem*)m_pLists.Get(0)->EnumSelected(&x)))
						item->m_color = cc.rgbResult & 0xFFFFFF; // fix for issue 600
				}
				Update();
#else
				m_bSettingColor = true;
				ShowColorChooser(item->m_color);
				SetTimer(m_hwnd, 1, 50, NULL);
#endif
			}
			break;
		}
		case PRI_UP_MSG:
		{
			int x = 0;
			SWS_RuleItem* item;
			while ((item = (SWS_RuleItem*)m_pLists.Get(0)->EnumSelected(&x)))
			{
				int iPos = g_pACItems.Find(item);
				if (iPos <= 0)
					break;
				g_pACItems.Delete(iPos, false);
				g_pACItems.Insert(iPos-1, item);
			}
			Update();
			break;
		}
		case PRI_DOWN_MSG:
		{
			// Go in reverse order
			for (int i = m_pLists.Get(0)->GetListItemCount()-1; i >= 0; i--)
			{
				if (m_pLists.Get(0)->IsSelected(i))
				{
					SWS_RuleItem* item = (SWS_RuleItem*)m_pLists.Get(0)->GetListItem(i);
					int iPos = g_pACItems.Find(item);
					if (iPos < 0)
						break;
					g_pACItems.Delete(iPos, false);
					g_pACItems.Insert(iPos+1, item);
				}
			}
			Update();
			break;
		}
		case INSACTION_MSG:
		{
			SWS_RuleItem* item = (SWS_RuleItem*)m_pLists.Get(0)->EnumSelected(NULL);
			if (item)
			{
				if (item->m_type == AC_TRACK)
				{
					char idstr[SNM_MAX_ACTION_CUSTID_LEN] = "";
					if (GetSelectedAction(idstr, SNM_MAX_ACTION_CUSTID_LEN))
					{
						if (idstr[0] == '_')
						{
							item->m_action = NamedCommandLookup(idstr);
							item->m_actionString.Set(idstr);
							item->m_actionName.Set(kbd_getTextFromCmd(item->m_action, SectionFromUniqueID(0)));
						}
						else
						{
							item->m_action = strtol(idstr, NULL, 0);
							if (item->m_action < 1)
							{
								item->m_action = -1;
								item->m_actionString.Set("-1");
								item->m_actionName.Set("");
							}
							else
							{
								item->m_actionString.Set(idstr);
								item->m_actionName.Set(kbd_getTextFromCmd(item->m_action, SectionFromUniqueID(0)));
							}
						}
					}
				}
				else
				{
					item->m_action = -1;
					item->m_actionString.Set("-1");
					item->m_actionName.Set(UTF8_BULLET);
				}
			}
			Update();
			break;
		}
		case CLEARACTION_MSG:
		{
			SWS_RuleItem* item = (SWS_RuleItem*)m_pLists.Get(0)->EnumSelected(NULL);
			if (item)
			{
				item->m_action = -1;
				item->m_actionString.Set("-1");
				if (item->m_type == AC_TRACK)
					item->m_actionName.Set("");
				else
					item->m_actionName.Set(UTF8_BULLET);
			}
			Update();
			break;
		}
		case LOAD_ICON_MSG:
		{
			SWS_RuleItem* item = (SWS_RuleItem*)m_pLists.Get(0)->EnumSelected(NULL);
			if (item)
			{
				char filename[BUFFER_SIZE], dir[32];
				sprintf(dir,"Data%ctrack_icons",PATH_SLASH_CHAR);
				if (BrowseResourcePath(__LOCALIZE("Load icon","sws_DLG_115"), dir, "PNG files (*.PNG)\0*.PNG\0ICO files (*.ICO)\0*.ICO\0JPEG files (*.JPG)\0*.JPG\0BMP files (*.BMP)\0*.BMP\0PCX files (*.PCX)\0*.PCX\0",
					filename, BUFFER_SIZE))
					item->m_icon.Set(filename);
			}
			Update();
			break;
		}
		case CLEAR_ICON_MSG:
		{
			SWS_RuleItem* item = (SWS_RuleItem*)m_pLists.Get(0)->EnumSelected(NULL);
			if (item)
				item->m_icon.Set("");
			Update();
			break;
		}
		case BTNID_R:
		{
			if (g_listsState < 1)
			{
				g_listsState++;
				SendMessage(m_hwnd, WM_SIZE, 0, 0);
			}
			break;
		}
		case BTNID_L:
		{
			if (g_listsState > -1)
			{
				g_listsState--;
				SendMessage(m_hwnd, WM_SIZE, 0, 0);
			}
			break;
		}
		default:
			if (wParam >= TYPETYPE_MSG && wParam < TYPETYPE_MSG + NUM_TYPETYPES)
			{
				SWS_RuleItem* item = (SWS_RuleItem*)m_pLists.Get(0)->EnumSelected(NULL);
				if (item)
				{
					item->m_type = (int)wParam - TYPETYPE_MSG;

					item->m_action = -1;
					if (item->m_type == AC_TRACK)
					{
						item->m_actionString.Set("-1");
						item->m_actionName.Set("");
					}
					else
					{
						item->m_actionString.Set("-1");
						item->m_actionName.Set(UTF8_BULLET);
					}
				}
				Update();
			}
			else if (wParam >= FILTERTYPE_MSG && wParam < FILTERTYPE_MSG + NUM_FILTERTYPES)
			{
				SWS_RuleItem* item = (SWS_RuleItem*)m_pLists.Get(0)->EnumSelected(NULL);
				if (item)
				{
					int iType = (int)wParam - FILTERTYPE_MSG;
					for (int i = 0; i < g_pACItems.GetSize(); i++)
						if (g_pACItems.Get(i) != item)
							if (g_pACItems.Get(i)->m_type == item->m_type && !strcmp(g_pACItems.Get(i)->m_str_filter.Get(), cFilterTypes[iType])) {
								MessageBox(m_hwnd, __LOCALIZE("Autocolor entry of that type already exists.","sws_DLG_115"), __LOCALIZE("SWS - Error","sws_DLG_115"), MB_OK);
								return;
							}
					item->m_str_filter.Set(cFilterTypes[wParam-FILTERTYPE_MSG]);
				}
				Update();
			}
			else if (wParam >= COLORTYPE_MSG && wParam < COLORTYPE_MSG + NUM_COLORTYPES)
			{
				int x = 0;
				SWS_RuleItem* item;
				while ((item = (SWS_RuleItem*)m_pLists.Get(0)->EnumSelected(&x)))
					item->m_color = -1 - ((int)wParam - COLORTYPE_MSG);
				Update();
			}
			else
				Main_OnCommand((int)wParam, (int)lParam);
	}
}


void SWS_AutoColorWnd::OnTimer(WPARAM wParam)
{
#ifndef _WIN32
	if (wParam == 1)
	{
		COLORREF cr;
		if (m_bSettingColor && GetChosenColor(&cr))
		{
			int x = 0;
			SWS_RuleItem* item;
			while ((item = (SWS_RuleItem*)m_pLists.Get(0)->EnumSelected(&x)))
				item->m_color = cr;

			KillTimer(m_hwnd, 1);
			Update();
		}
	}
#endif
	if (wParam == 2)
		m_pLists.Get(1)->Update();
}


void SWS_AutoColorWnd::OnDestroy()
{
#ifndef _WIN32
	if (m_bSettingColor)
	{
		HideColorChooser();
		KillTimer(m_hwnd, 1);
	}
#endif

	KillTimer(m_hwnd, 2);
	m_tinyLRbtns.RemoveAllChildren(false);
}

void SWS_AutoColorWnd::OnResize()
{
	if (g_listsState != g_listsLastState)
	{
		g_listsLastState = g_listsState;
		switch (g_listsState)
		{
		case -1: // right list view only
			m_resize.remove_item(IDC_LIST);
			m_resize.remove_item(IDC_LISTTR);
			m_resize.init_item(IDC_LIST, 0.0, 0.0, 0.0, 0.0);
			m_resize.init_item(IDC_LISTTR, 0.0, 0.0, 1.0, 1.0);
			m_resize.get_item(IDC_LISTTR)->orig = g_origRectR;
			m_resize.get_item(IDC_LISTTR)->orig.left = g_origRectL.left;
			ShowWindow(GetDlgItem(m_hwnd, IDC_LIST), SW_HIDE);
			break;
		case 0: // default: both list views
			m_resize.remove_item(IDC_LIST);
			m_resize.remove_item(IDC_LISTTR);
			m_resize.init_item(IDC_LIST, 0.0, 0.0, 0.5, 1.0);
			m_resize.init_item(IDC_LISTTR, 0.5, 0.0, 1.0, 1.0);
			m_resize.get_item(IDC_LIST)->orig = g_origRectL;
			m_resize.get_item(IDC_LISTTR)->orig = g_origRectR;
			ShowWindow(GetDlgItem(m_hwnd, IDC_LIST), SW_SHOW);
			ShowWindow(GetDlgItem(m_hwnd, IDC_LISTTR), SW_SHOW);
			break;
		case 1: // left list view only
			m_resize.remove_item(IDC_LIST);
			m_resize.remove_item(IDC_LISTTR);
			m_resize.init_item(IDC_LIST, 0.0, 0.0, 1.0, 1.0);
			m_resize.init_item(IDC_LISTTR, 0.0, 0.0, 0.0, 0.0);
			m_resize.get_item(IDC_LIST)->orig = g_origRectL;
			m_resize.get_item(IDC_LIST)->orig.right = g_origRectR.right;
			ShowWindow(GetDlgItem(m_hwnd, IDC_LISTTR), SW_HIDE);
			break;
		}
		InvalidateRect(m_hwnd, NULL, 0);
	}
}

void SWS_AutoColorWnd::DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight)
{
	RECT r;
	int h = 7;
	// tiny left/right buttons
	if (IsWindowVisible(GetDlgItem(m_hwnd, IDC_LIST))) // rules view is displayed: add tiny buttons on its right
	{
		GetWindowRect(GetDlgItem(m_hwnd, IDC_LIST), &r);
		ScreenToClient(m_hwnd, (LPPOINT)&r);
		ScreenToClient(m_hwnd, ((LPPOINT)&r) + 1);
		r.top = _r->top + h;
		r.bottom = _r->top + h + (9 * 2 + 1);
		r.left = r.right + 1;
		r.right = r.left + 5;
	}
	else // add tiny buttons on the left
	{
		r.top = _r->top + h;
		r.bottom = _r->top + h + (9 * 2 + 1);
		r.left = 1;
		r.right = r.left + 5;
	}

	m_btnLeft.SetEnabled(g_listsState>-1);
	m_btnRight.SetEnabled(g_listsState<1);
	m_tinyLRbtns.SetPosition(&r);
	m_tinyLRbtns.SetVisible(true);
}

INT_PTR SWS_AutoColorWnd::OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_DRAWITEM)
	{
		LPDRAWITEMSTRUCT pDI = (LPDRAWITEMSTRUCT)lParam;
		if (pDI->CtlID == IDC_COLOR)
		{
			// Dialog-box background grey for no selection, or multiple colors in selection
			// Doesn't account for the "special colors" because they're less than zero and get trapped
			// out below anyway.
			int col = -1;
			int x = 0;
			SWS_RuleItem* item;
			while ((item = (SWS_RuleItem*)m_pLists.Get(0)->EnumSelected(&x)))
			{
				if (col < 0)
					col = item->m_color;
				else if (col != item->m_color)
				{
					col = -1;
					break;
				}
			}

			if (col < 0)
				col = GetSysColor(COLOR_3DFACE);

			HBRUSH hb = CreateSolidBrush(col);
			FillRect(pDI->hDC, &pDI->rcItem, hb);
			DeleteObject(hb);
			return 1;
		}
	}
	return 0;
}

// must remain in sync with main menu > extensions > sws options
void SWS_AutoColorWnd::AddOptionsMenu(HMENU _menu)
{
	if (GetMenuItemCount(_menu))
		AddToMenu(_menu, SWS_SEPARATOR, 0);
	AddToMenu(_menu, __LOCALIZE("Enable auto track coloring", "sws_ext_menu"), NamedCommandLookup("_SWSAUTOCOLOR_ENABLE"), -1, false, g_bACEnabled ?  MF_CHECKED : MF_UNCHECKED);
	AddToMenu(_menu, __LOCALIZE("Enable auto marker coloring", "sws_ext_menu"), NamedCommandLookup("_S&MAUTOCOLOR_MKR_ENABLE"), -1, false, g_bACMEnabled ?  MF_CHECKED : MF_UNCHECKED);
	AddToMenu(_menu, __LOCALIZE("Enable auto region coloring", "sws_ext_menu"), NamedCommandLookup("_S&MAUTOCOLOR_RGN_ENABLE"), -1, false, g_bACREnabled ?  MF_CHECKED : MF_UNCHECKED);
	AddToMenu(_menu, __LOCALIZE("Enable auto track icon", "sws_ext_menu"), NamedCommandLookup("_S&MAUTOICON_ENABLE"), -1, false, g_bAIEnabled ?  MF_CHECKED : MF_UNCHECKED);
	AddToMenu(_menu, __LOCALIZE("Enable auto action", "sws_ext_menu"), NamedCommandLookup("_WOLAUTOACTION_ENABLE"), -1, false, g_bAATEnabled ? MF_CHECKED : MF_UNCHECKED);
	AddToMenu(_menu, SWS_SEPARATOR, 0);
	AddToMenu(_menu, __LOCALIZE("Enable track auto selection", "sws_ext_menu"), NamedCommandLookup("_WOL_TENTRAUTOSELAUTO"), -1, false, g_bTrackAutoSelectionEn ? MF_CHECKED : MF_UNCHECKED);

	AddToMenu(_menu, __LOCALIZE("Automatically disable auto color for processed tracks", "sws_ext_menu"), NamedCommandLookup("_WOL_TENAUTOCOLORDIS"), -1, false, g_bAutoColorDisable ? MF_CHECKED : MF_UNCHECKED);
	AddToMenu(_menu, __LOCALIZE("Automatically disable auto icon for processed tracks", "sws_ext_menu"), NamedCommandLookup("_WOL_TENAUTOICONDIS"), -1, false, g_bAutoIconDisable ? MF_CHECKED : MF_UNCHECKED);
	AddToMenu(_menu, __LOCALIZE("Automatically disable auto action for processed tracks ", "sws_ext_menu"), NamedCommandLookup("_WOL_TENAUTOACTIONDIS"), -1, false, g_bAutoActionDisable ? MF_CHECKED : MF_UNCHECKED);
}

HMENU SWS_AutoColorWnd::OnContextMenu(int x, int y, bool* wantDefaultItems)
{
	HMENU hMenu = CreatePopupMenu();

	// specific context menu for the button "options"
	{
		POINT pt = {x, y + 3*(SNM_1PIXEL_Y*(-1))}; // +/- 3: tiny trick, see OnCommand()
		RECT r;	GetWindowRect(GetDlgItem(m_hwnd, IDC_OPTIONS), &r);
		if (PtInRect(&r, pt))
		{
			*wantDefaultItems = false;
			AddOptionsMenu(hMenu);
			return hMenu;
		}
	}

	// "standard" context menu
	POINT pt = { x, y };
	RECT r;
	GetWindowRect(GetDlgItem(m_hwnd, IDC_LIST), &r);
	if (PtInRect(&r, pt))
	{
		int iCol;
		if (SWS_RuleItem* item = (SWS_RuleItem*)m_pLists.Get(0)->GetHitItem(x, y, &iCol))
		{
			*wantDefaultItems = false;

			switch (iCol)
			{
				case COL_TYPE:
				{
					for (int i = 0; i < NUM_TYPETYPES; i++)
						AddToMenu(hMenu, __localizeFunc(cTypes[i],"sws_DLG_115",LOCALIZE_FLAG_NOCACHE), TYPETYPE_MSG + i);
					AddToMenu(hMenu, SWS_SEPARATOR, 0);
					break;
				}
				case COL_FILTER:
				{
					int numFilters = 0;
					if (item->m_type == AC_TRACK)
						numFilters= NUM_FILTERTYPES;
					else if (item->m_type == AC_MARKER || item->m_type == AC_REGION)
						numFilters = NUM_RGNFILTERTYPES;
					if (numFilters)
					{
						for (int i=0; i<numFilters; i++)
							AddToMenu(hMenu, __localizeFunc(cFilterTypes[i],"sws_DLG_115",LOCALIZE_FLAG_NOCACHE), FILTERTYPE_MSG + i);
						AddToMenu(hMenu, SWS_SEPARATOR, 0);
					}
					break;
				}
				case COL_COLOR:
				{
					AddToMenu(hMenu, __LOCALIZE("Set color...","sws_DLG_115"), IDC_COLOR);
					AddToMenu(hMenu, SWS_SEPARATOR, 0);

					if (item->m_type == AC_TRACK)
					{
						for (int i = 0; i < NUM_COLORTYPES; i++)
							AddToMenu(hMenu, __localizeFunc(cColorTypes[i],"sws_DLG_115",LOCALIZE_FLAG_NOCACHE), COLORTYPE_MSG + i);
					}
					else if (item->m_type == AC_MARKER || item->m_type == AC_REGION)
						AddToMenu(hMenu, __localizeFunc(cColorTypes[AC_NONE],"sws_DLG_115",LOCALIZE_FLAG_NOCACHE), COLORTYPE_MSG + AC_NONE);
					AddToMenu(hMenu, SWS_SEPARATOR, 0);
					break;
				}
				case COL_ICON:
				{
					AddToMenu(hMenu, __LOCALIZE("Load icon...","sws_DLG_115"), LOAD_ICON_MSG, -1, false, item->m_type == AC_TRACK ? MF_ENABLED : MF_GRAYED);
					AddToMenu(hMenu, __LOCALIZE("Clear icon","sws_DLG_115"), CLEAR_ICON_MSG, -1, false, item->m_type == AC_TRACK ? MF_ENABLED : MF_GRAYED);
					AddToMenu(hMenu, SWS_SEPARATOR, 0);
					break;
				}
				case COL_ACTION:
				{
					AddToMenu(hMenu, __LOCALIZE("Insert selected action (in the Actions window)", "sws_DLG_115"), INSACTION_MSG, -1, false, item->m_type == AC_TRACK ? MF_ENABLED : MF_GRAYED);
					AddToMenu(hMenu, __LOCALIZE("Clear action", "sws_DLG_115"), CLEARACTION_MSG, -1, false, item->m_type == AC_TRACK ? MF_ENABLED : MF_GRAYED);
					AddToMenu(hMenu, SWS_SEPARATOR, 0);
					break;
				}
			}
			AddToMenu(hMenu, __LOCALIZE("Up in priority","sws_DLG_115"), PRI_UP_MSG);
			AddToMenu(hMenu, __LOCALIZE("Down in priority","sws_DLG_115"), PRI_DOWN_MSG);
		}
	}
	else
	{
		AddToMenu(hMenu, __LOCALIZE("Show color management window","sws_DLG_115"), SWSGetCommandID(ShowColorDialog));
		AddToMenu(hMenu, SWS_SEPARATOR, 0);

		HMENU hOptionsSubMenu = CreatePopupMenu();
		AddSubMenu(hMenu, hOptionsSubMenu, __LOCALIZE("Options", "sws_menu"));
		AddOptionsMenu(hOptionsSubMenu);
	}
	return hMenu;
}

int SWS_AutoColorWnd::OnKey(MSG* msg, int iKeyState)
{
	if (msg->message == WM_KEYDOWN && msg->wParam == VK_DELETE && !iKeyState)
	{
		OnCommand(IDC_REMOVE, 0);
		return 1;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////////
void OpenAutoColor(COMMAND_T*)
{
	g_pACWnd->Show(true, true);
}

void AutoColorTrackListUpdate()
{
	g_pACWnd->ScheduleTrackListUpdate();
}

void ApplyColorRuleToTrack(SWS_RuleItem* rule, bool bDoColors, bool bDoIcons, bool bDoAction, bool bForce)
{
	if(rule->m_type == AC_TRACK)
	{
		if (!bDoColors && !bDoIcons && !bDoAction)
			return;

		int iCount = 0;
		WDL_PtrList<void> gradientTracks;

		if (rule->m_color == -AC_CUSTOM-1)
			UpdateCustomColors();

		// Check all tracks for matching strings/properties
		MediaTrack* temp = NULL;
		for (int i = 0; i <= GetNumTracks(); i++)
		{
			MediaTrack* tr = CSurf_TrackFromID(i, false);
			bool bColor = bDoColors;
			bool bIcon  = bDoIcons;
			bool bAction = bDoAction;

			bool bFound = false;
			SWS_RuleTrack* pACTrack = NULL;
			for (int j = 0; j < g_pACTracks.Get()->GetSize(); j++)
			{
				pACTrack = g_pACTracks.Get()->Get(j);
				if (pACTrack->m_pTr == tr)
				{
					bFound = true;
					break;
				}
			}

			if (bFound)
			{
				// If already colored by a different rule, or ignoring the color ignore this track
				if (pACTrack->m_bColored || rule->m_color == -AC_IGNORE-1)
					bColor = false;
				// If already iconed by a different rule, or the icon field is blank (ignore), ignore
				if (pACTrack->m_bIconed || !rule->m_icon.Get()[0])
					bIcon = false;
			}
			else
				pACTrack = g_pACTracks.Get()->Add(new SWS_RuleTrack(tr));

			// Do the track rule matching
			if (bColor || bIcon || bAction)
			{
				bool bMatch = false;

				if (i) // ignore master for most things
				{
					// Check "special" rules first:
					if (strcmp(rule->m_str_filter.Get(), cFilterTypes[AC_FOLDER]) == 0)
					{
						int iType;
						GetFolderDepth(tr, &iType, &temp);
						if (iType == 1)
							bMatch = true;
					}
					else if (strcmp(rule->m_str_filter.Get(), cFilterTypes[AC_CHILDREN]) == 0)
					{
						temp = CSurf_TrackFromID(0, false); // JFB fix: 'temp' could be out of sync
						if (GetFolderDepth(tr, NULL, &temp) >= 1)
							bMatch = true;
					}
					else if (strcmp(rule->m_str_filter.Get(), cFilterTypes[AC_RECEIVE]) == 0)
					{
						if (GetSetTrackSendInfo(tr, -1, 0, "P_SRCTRACK", NULL))
							bMatch = true;
					}
					else if (strcmp(rule->m_str_filter.Get(), cFilterTypes[AC_UNNAMED]) == 0)
					{
						char* cName = (char*)GetSetMediaTrackInfo(tr, "P_NAME", NULL);
						if (!cName || !cName[0])
							bMatch = true;
					}
					else if (strcmp(rule->m_str_filter.Get(), cFilterTypes[AC_ANY]) == 0)
					{
						bMatch = true;
					}
					else // Check for name match
					{
						char* cName = (char*)GetSetMediaTrackInfo(tr, "P_NAME", NULL);
						if (cName && stristr(cName, rule->m_str_filter.Get()))
							bMatch = true;
					}
				}
				else if (strcmp(rule->m_str_filter.Get(), cFilterTypes[AC_MASTER]) == 0)
				{	// Check master rule
					bMatch = true;
				}

				int ruleId = g_pACItems.Find(rule);
				if (bMatch && pACTrack->m_lastMatchedRule != ruleId)
				{
					// Set the color
					if (bColor && pACTrack->m_bACEnabled)
					{
						int iCurColor = *(int*)GetSetMediaTrackInfo(tr, "I_CUSTOMCOLOR", NULL);
						if (!(iCurColor & 0x1000000))
							iCurColor = 0;
						int newCol = iCurColor;

						if (rule->m_color == -AC_RANDOM - 1)
						{
							// Only randomize once
							if (!(iCurColor & 0x1000000))
								newCol = RGB(rand() % 256, rand() % 256, rand() % 256) | 0x1000000;
						}
						else if (rule->m_color == -AC_CUSTOM - 1)
						{
							if (!AllBlack())
								while (!(newCol = g_custColors[iCount++ % 16]));
							newCol |= 0x1000000;
						}
						else if (rule->m_color == -AC_GRADIENT - 1)
						{
							gradientTracks.Add(tr);
							if (g_bAutoColorDisable)
								pACTrack->m_bACEnabled = false;
						}
						else if (rule->m_color == -AC_NONE - 1)
							newCol = 0;
						else if (rule->m_color == -AC_PARENT - 1)
						{
							MediaTrack* parent = (MediaTrack*)GetSetMediaTrackInfo(tr, "P_PARTRACK", NULL);
							if (parent)
							{
								int pcol = *(int*)GetSetMediaTrackInfo(parent, "I_CUSTOMCOLOR", NULL);
								if (pcol & 0x1000000) // Only color like parent if the parent has color (maybe not?)
									newCol = pcol;
							}
						}
						else
							newCol = rule->m_color | 0x1000000;

						// Only set the color if the user hasn't changed the color manually (but record it as being changed)
						if ((bForce || iCurColor == pACTrack->m_col) && newCol != iCurColor)
						{
							GetSetMediaTrackInfo(tr, "I_CUSTOMCOLOR", &newCol);
							if (g_bAutoColorDisable)
								pACTrack->m_bACEnabled = false;
						}

						pACTrack->m_col = newCol;
						pACTrack->m_bColored = true;
					}
					else if (!pACTrack->m_bACEnabled)
						pACTrack->m_bColored = true;

					if (bAction && pACTrack->m_bAAEnabled && (rule->m_action != -1))
					{
						if (g_bTrackAutoSelectionEn)
						{
							Main_OnCommand(40297, 0);
							GetSetMediaTrackInfo(pACTrack->m_pTr, "I_SELECTED", &g_i1);
						}
						Main_OnCommand(rule->m_action, 0);
						pACTrack->m_lastExecAction.Set(rule->m_actionName.Get());

						if (g_bAutoActionDisable)
							pACTrack->m_bAAEnabled = false;
					}

					if (bIcon && pACTrack->m_bAIEnabled)
					{
						if (strcmp(rule->m_icon.Get(), pACTrack->m_icon.Get()))
						{
							SNM_ChunkParserPatcher p(tr); // nothing done yet
							char pIconLine[BUFFER_SIZE] = "";
							int iconChunkPos = p.Parse(SNM_GET_CHUNK_CHAR, 1, "TRACK", "TRACKIMGFN", 0, 1, pIconLine, NULL, "TRACKID");
							if (strcmp(pIconLine, rule->m_icon.Get()))
							{
								// Only overwrite the icon if there's no icon, or we're forcing, or we set it ourselves earlier
								if (bForce || iconChunkPos == 0 || strcmp(pIconLine, pACTrack->m_icon.Get()) == 0)
								{
									if (rule->m_icon.GetLength())
										sprintf(pIconLine, "TRACKIMGFN \"%s\"\n", rule->m_icon.Get());
									else // The code as written will never hit this case, as empty m_icon means "ignore"
										*pIconLine = 0;

									if (iconChunkPos > 0)
										p.ReplaceLine(--iconChunkPos, pIconLine);
									else
										p.InsertAfterBefore(0, pIconLine, "TRACK", "FX", 1, 0, "TRACKID");
								}
							}
							pACTrack->m_icon.Set(rule->m_icon.Get());
						}
						pACTrack->m_bIconed = true;

						if (g_bAutoIconDisable)
							pACTrack->m_bAIEnabled = false;
					}
					else if (!pACTrack->m_bAIEnabled)
						pACTrack->m_bIconed = true;

					AutoColorTrackListUpdate();
					pACTrack->m_lastMatchedRule = ruleId;
				}
				else if (pACTrack->m_lastMatchedRule == ruleId)
				{
					pACTrack->m_bColored = true;
					pACTrack->m_bIconed = true;
				}
			}
		}

		// Handle gradients
		for (int i = 0; i < gradientTracks.GetSize(); i++)
		{
			int newCol = g_crGradStart | 0x1000000;
			if (i && gradientTracks.GetSize() > 1)
				newCol = CalcGradient(g_crGradStart, g_crGradEnd, (double)i / (gradientTracks.GetSize()-1)) | 0x1000000;
			for (int j = 0; j < g_pACTracks.Get()->GetSize(); j++)
				if (g_pACTracks.Get()->Get(j)->m_pTr == (MediaTrack*)gradientTracks.Get(i))
				{
					g_pACTracks.Get()->Get(j)->m_col = newCol;
					break;
				}
			GetSetMediaTrackInfo((MediaTrack*)gradientTracks.Get(i), "I_CUSTOMCOLOR", &newCol);
		}
	}
}

// Here's the meat and potatoes, apply the colors/icons!
void AutoColorTrack(bool bForce)
{
	static bool bRecurse = false;
	if (bRecurse || (!g_bACEnabled && !g_bAIEnabled && !g_bAATEnabled && !bForce))
		return;
	bRecurse = true;

	// Remove non-existant tracks from the autocolortracklist
	for (int i = 0; i < g_pACTracks.Get()->GetSize(); ++i)
		if (CSurf_TrackToID(g_pACTracks.Get()->Get(i)->m_pTr, false) < 0)
		{
			g_pACTracks.Get()->Delete(i, true);
			i--;
		}

	// All tracks in the list for track list view
	for (int i = 1; i <= GetNumTracks(); ++i)
	{
		bool bFound = false;
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; j < g_pACTracks.Get()->GetSize(); ++j)
		{
			if (g_pACTracks.Get()->Get(j)->m_pTr == tr)
			{
				bFound = true;
				break;
			}
		}
		if (!bFound)
			g_pACTracks.Get()->Add(new SWS_RuleTrack(tr));
	}

	// Clear the "colored" bit and "iconed" bit and, if forcing, the last matched rule
	for (int i = 0; i < g_pACTracks.Get()->GetSize(); ++i)
	{
		g_pACTracks.Get()->Get(i)->m_bColored = false;
		g_pACTracks.Get()->Get(i)->m_bIconed = false;
		if (bForce)
			g_pACTracks.Get()->Get(i)->m_lastMatchedRule = -1;
	}

	PreventUIRefresh(1);
	vector<MediaTrack*> selectedTracks;
	for (int i = 0; i < CountSelectedTracks(NULL); ++i)
		selectedTracks.push_back(GetSelectedTrack(NULL, i));

	// Apply the rules
	SWS_CacheObjectState(true);
	bool bDoColors = g_bACEnabled || bForce;
	bool bDoIcons  = g_bAIEnabled || bForce;
	bool bDoActions = g_bAATEnabled || bForce;

	for (int i = 0; i < g_pACItems.GetSize(); ++i)
		ApplyColorRuleToTrack(g_pACItems.Get(i), bDoColors, bDoIcons, bDoActions, bForce);

	// Remove colors/icons if necessary
	for (int i = 0; i < g_pACTracks.Get()->GetSize(); ++i)
	{
		SWS_RuleTrack* pACTrack = g_pACTracks.Get()->Get(i);
		if (bDoColors && !pACTrack->m_bColored && pACTrack->m_col)
		{
			int iCurColor = *(int*)GetSetMediaTrackInfo(pACTrack->m_pTr, "I_CUSTOMCOLOR", NULL);

			if (!(iCurColor & 0x1000000))
				iCurColor = 0;

			// Only remove color on tracks that we colored ourselves
			if (pACTrack->m_col == iCurColor)
				GetSetMediaTrackInfo(pACTrack->m_pTr, "I_CUSTOMCOLOR", &g_i0);
			pACTrack->m_col = 0;
		}

		if (bDoIcons && !pACTrack->m_bIconed && *pACTrack->m_icon.Get())
		{	// There's an icon set, but there shouldn't be!
			SNM_ChunkParserPatcher p(pACTrack->m_pTr); // Yay for the patcher

			// Only remove the icon on the track if we set it ourselves
			char pIconLine[BUFFER_SIZE] = "";
			int iconChunkPos = p.Parse(SNM_GET_CHUNK_CHAR, 1, "TRACK", "TRACKIMGFN", 0, 1, pIconLine, NULL, "TRACKID");
			if (iconChunkPos && strcmp(pACTrack->m_icon.Get(), pIconLine) == 0)
				p.ReplaceLine(--iconChunkPos, "");

			pACTrack->m_icon.Set("");
		}
	}
	SWS_CacheObjectState(false);

	for (int i = 0; i < CountTracks(NULL); ++i)
		SetMediaTrackInfo_Value(GetTrack(NULL, i), "I_SELECTED", 0);
	for (size_t i = 0; i < selectedTracks.size(); ++i)
		SetMediaTrackInfo_Value(selectedTracks[i], "I_SELECTED", 1);
	PreventUIRefresh(-1);

	if (bForce)
		Undo_OnStateChangeEx(__LOCALIZE("Apply auto color/icon/action","sws_undo"), UNDO_STATE_TRACKCFG | UNDO_STATE_MISCCFG, -1);
	bRecurse = false;
}

void ApplyColorRuleToMarkerRegion(SWS_RuleItem* _rule, int _flags)
{
	ColorTheme* ct = SNM_GetColorTheme();
	if (!_rule || !_flags || !ct)
		return;

	double pos, end;
	int x=0, num, color;
	bool isRgn, update=false;
	const char* name;

	PreventUIRefresh(1);
	if(_rule->m_type & _flags)
	{
		while (x = EnumProjectMarkers3(NULL, x, &isRgn, &pos, &end, &name, &num, &color))
		{
			if ((!strcmp(cFilterTypes[AC_RGNANY], _rule->m_str_filter.Get()) ||
				(!strcmp(cFilterTypes[AC_RGNUNNAMED], _rule->m_str_filter.Get()) && (!name || !*name)) ||
				(name && stristr(name, _rule->m_str_filter.Get())))
				&&
				((_flags&AC_REGION && isRgn && _rule->m_type==AC_REGION) ||
				(_flags&AC_MARKER && !isRgn && _rule->m_type==AC_MARKER)))
			{
				update |= SetProjectMarkerByIndex(NULL, x-1, isRgn, pos, end, num, NULL, _rule->m_color==-AC_NONE-1 ? (isRgn?ct->marker:ct->region) : _rule->m_color | 0x1000000);
			}
		}
	}
	PreventUIRefresh(-1);

	if (update)
		UpdateTimeline();
}

void AutoColorMarkerRegion(bool _force, int _flags)
{
	static bool bRecurse = false;
	if (bRecurse || (!g_bACREnabled && !g_bACMEnabled && !_force))
		return;
	bRecurse = true;

	int newFlags = 0;
	if (_flags&AC_MARKER && (g_bACMEnabled || _force))
		newFlags |= AC_MARKER;
	if (_flags&AC_REGION && (g_bACREnabled || _force))
		newFlags |= AC_REGION;

	if (newFlags)
	{
		if (PreventUIRefresh)
			PreventUIRefresh(1);

		for (int i=g_pACItems.GetSize()-1; i>=0; i--) // reverse to obey priority
			ApplyColorRuleToMarkerRegion(g_pACItems.Get(i), newFlags);

		if (PreventUIRefresh)
			PreventUIRefresh(-1);

		UpdateTimeline();
	}

	bRecurse = false;
}

void TEDSelectedTrackAuto(COMMAND_T* ct)
{
	for (int i = 0; i < CountSelectedTracks(NULL); ++i)
	{
		SWS_RuleTrack* pTrack = NULL;
		MediaTrack* tr = GetSelectedTrack(NULL, i);
		for (int j = 0; j < g_pACTracks.Get()->GetSize(); ++j)
		{
			pTrack = g_pACTracks.Get()->Get(j);
			if (pTrack->m_pTr == tr)
			{
				switch ((int)ct->user)
				{
				case 0:
					pTrack->m_bACEnabled = !pTrack->m_bACEnabled;
					break;
				case 1:
					pTrack->m_bAIEnabled = !pTrack->m_bAIEnabled;
					break;
				case 2:
					pTrack->m_bAAEnabled = !pTrack->m_bAAEnabled;
					break;
				case 3:
					pTrack->m_bACEnabled = true;
					break;
				case 4:
					pTrack->m_bAIEnabled = true;
					break;
				case 5:
					pTrack->m_bAAEnabled = true;
					break;
				case 6:
					pTrack->m_bACEnabled = false;
					break;
				case 7:
					pTrack->m_bAIEnabled = false;
					break;
				case 8:
					pTrack->m_bAAEnabled = false;
					break;
				}
				MarkProjectDirty(NULL);
			}
		}
	}
}

void EnableAutoColor(COMMAND_T* ct)
{
	switch((int)ct->user)
	{
		case 0: g_bACEnabled = !g_bACEnabled; break;
		case 1: g_bACMEnabled = !g_bACMEnabled; break;
		case 2: g_bACREnabled = !g_bACREnabled; break;
	}
	g_pACWnd->Update();
}

void EnableAutoIcon(COMMAND_T*)
{
	g_bAIEnabled = !g_bAIEnabled;
	g_pACWnd->Update();
}

void EnableAutoAction(COMMAND_T*)
{
	g_bAATEnabled = !g_bAATEnabled;
	g_pACWnd->Update();
}

void ApplyAutoColor(COMMAND_T*)
{
	AutoColorTrack(true);
	AutoColorMarkerRegion(true);
}

int IsAutoColorOpen(COMMAND_T*)		{ return g_pACWnd->IsValidWindow(); }
int IsAutoIconEnabled(COMMAND_T*)	{ return g_bAIEnabled; }

int IsAutoColorEnabled(COMMAND_T* ct)
{
	switch((int)ct->user)
	{
		case 0: return g_bACEnabled;
		case 1: return g_bACMEnabled;
		case 2: return g_bACREnabled;
	}
	return false;
 }

int IsAutoActionEnabled(COMMAND_T* ct)
{
	return g_bAATEnabled;
}

void ToggleEnableTrackAutoSelection(COMMAND_T*)
{
	g_bTrackAutoSelectionEn = !g_bTrackAutoSelectionEn;
	char str[32];
	sprintf(str, "%d", g_bTrackAutoSelectionEn);
	WritePrivateProfileString(SWS_INI, TRAUTOSEL_ENABLE_KEY, str, g_ACIni.Get());
}

int IsTrackAutoSelectionEnabled(COMMAND_T*)
{
	return g_bTrackAutoSelectionEn;
}

void ToggleEnableAutoDisable(COMMAND_T* ct)
{
	switch ((int)ct->user)
	{
		case 0:
		{
			g_bAutoColorDisable = !g_bAutoColorDisable;
			char str[32];
			sprintf(str, "%d", g_bAutoColorDisable);
			WritePrivateProfileString(SWS_INI, AUTOCOL_DISABLE_KEY, str, g_ACIni.Get());
			break;
		}
		case 1:
		{
			g_bAutoIconDisable = !g_bAutoIconDisable;
			char str[32];
			sprintf(str, "%d", g_bAutoIconDisable);
			WritePrivateProfileString(SWS_INI, AUTOICON_DISABLE_KEY, str, g_ACIni.Get());
			break;
		}
		case 2:
		{
			g_bAutoActionDisable = !g_bAutoActionDisable;
			char str[32];
			sprintf(str, "%d", g_bAutoActionDisable);
			WritePrivateProfileString(SWS_INI, AUTOACTION_DISABLE_KEY, str, g_ACIni.Get());
			break;
		}
	}
}

int IsAutoDisableEnabled(COMMAND_T* ct)
{
	switch ((int)ct->user)
	{
		case 0: return g_bAutoColorDisable;
		case 1: return g_bAutoIconDisable;
		case 2: return g_bAutoActionDisable;
	}
	return false;
}

static bool ProcessExtensionLine(const char *line, ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	LineParser lp(false);
	if (lp.parse(line) || lp.getnumtokens() < 1)
		return false;
	if (strcmp(lp.gettoken_str(0), "<SWSAUTOCOLOR") == 0)
	{
		char linebuf[4096];
		while(true)
		{
			if (!ctx->GetLine(linebuf,sizeof(linebuf)) && !lp.parse(linebuf))
			{
				if (lp.gettoken_str(0)[0] == '>')
					break;

				GUID g;
				stringToGuid(lp.gettoken_str(0), &g);
				MediaTrack* tr = GuidToTrack(&g);
				if (tr)
				{
					SWS_RuleTrack* rt = g_pACTracks.Get()->Add(new SWS_RuleTrack(tr));
					rt->m_col = lp.gettoken_int(1);
					if (lp.getnumtokens() <= 3)
						rt->m_icon.Set(lp.gettoken_str(2));
					else
					{
						rt->m_bACEnabled = (lp.gettoken_int(2) != 0);
						rt->m_bAIEnabled = (lp.gettoken_int(3) != 0);
						rt->m_bAAEnabled = (lp.gettoken_int(4) != 0);
						rt->m_lastMatchedRule = lp.gettoken_int(5);
						rt->m_icon.Set(lp.gettoken_str(6));
						rt->m_lastExecAction.Set(lp.gettoken_str(7));
					}
				}
			}
			else
				break;
		}
		return true;
	}
	return false;
}

static void SaveExtensionConfig(ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	// first delete unused tracks
	for (int i = 0; i < g_pACTracks.Get()->GetSize(); i++)
		if (CSurf_TrackToID(g_pACTracks.Get()->Get(i)->m_pTr, false) < 0)
		{
			g_pACTracks.Get()->Delete(i, true);
			i--;
		}

	if (g_pACTracks.Get()->GetSize())
	{
		char str[SNM_MAX_CHUNK_LINE_LENGTH];
		vector<string> tracks;
		for (int i = 0; i < g_pACTracks.Get()->GetSize(); i++)
		{
			if (SWS_RuleTrack* rt = g_pACTracks.Get()->Get(i))
			{
				if (rt->IsNotDefault())
				{
					GUID g = GUID_NULL;
					if (CSurf_TrackToID(rt->m_pTr, false))
						g = *(GUID*)GetSetMediaTrackInfo(rt->m_pTr, "GUID", NULL);
					guidToString(&g, str);
					// guid color ACen AIen AAen icon lastAction
					sprintf(str + strlen(str), " %d %d %d %d %d \"%s\" \"%s\"", rt->m_col, rt->m_bACEnabled ? 1 : 0, rt->m_bAIEnabled ? 1 : 0, rt->m_bAAEnabled ? 1 : 0, rt->m_lastMatchedRule, rt->m_icon.Get(), rt->m_lastExecAction.Get());
					tracks.push_back(string(str));
				}
			}
		}
		if (tracks.size())
		{
			ctx->AddLine("<SWSAUTOCOLOR");
			for (unsigned i = 0; i < tracks.size(); ++i)
				ctx->AddLine(tracks.at(i).c_str());
			ctx->AddLine(">");
		}
	}
}

static void BeginLoadProjectState(bool isUndo, struct project_config_extension_t *reg)
{
	g_pACTracks.Cleanup();
	g_pACTracks.Get()->Empty(true);
	g_pACWnd->Update();
}

static project_config_extension_t g_projectconfig = { ProcessExtensionLine, SaveExtensionConfig, BeginLoadProjectState, NULL };

//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] =
{
	{ { DEFACCEL, "SWS: Open auto color/icon/action window" },	   "SWSAUTOCOLOR_OPEN",		  OpenAutoColor,	"SWS Auto color/icon/action",  0, IsAutoColorOpen },
	{ { DEFACCEL, "SWS: Toggle auto track coloring enable" },	   "SWSAUTOCOLOR_ENABLE",	  EnableAutoColor,	"Enable auto track coloring",  0, IsAutoColorEnabled },
	{ { DEFACCEL, "SWS/S&M: Toggle auto marker coloring enable" }, "S&MAUTOCOLOR_MKR_ENABLE", EnableAutoColor,	"Enable auto marker coloring", 1, IsAutoColorEnabled },
	{ { DEFACCEL, "SWS/S&M: Toggle auto region coloring enable" }, "S&MAUTOCOLOR_RGN_ENABLE", EnableAutoColor,	"Enable auto region coloring", 2, IsAutoColorEnabled },
	{ { DEFACCEL, "SWS/S&M: Toggle auto track icon enable"},	   "S&MAUTOICON_ENABLE",	  EnableAutoIcon,	"Enable auto track icon",	   0, IsAutoIconEnabled },
	{ { DEFACCEL, "SWS/wol: Toggle auto action enable" },	       "WOLAUTOACTION_ENABLE",	  EnableAutoAction,	"Enable auto action",		   0, IsAutoActionEnabled },

	{ { DEFACCEL, "SWS/wol: Toggle enable auto color for selected track(s)" },  "WOLSELTRAUTOCOLOR_TOGGLE",	  TEDSelectedTrackAuto, "Toggle selected track auto color",	  0, },
	{ { DEFACCEL, "SWS/wol: Toggle enable auto icon for selected track(s)" },   "WOLSELTRAUTOICON_TOGGLE",    TEDSelectedTrackAuto, "Toggle selected track auto icon",	  1, },
	{ { DEFACCEL, "SWS/wol: Toggle enable auto action for selected track(s)" }, "WOLSELTRAUTOACTION_TOGGLE",  TEDSelectedTrackAuto, "Toggle selected track auto action",  2, },
	{ { DEFACCEL, "SWS/wol: Enable auto color for selected track(s)" },		    "WOLSELTRAUTOCOLOR_ENABLE",   TEDSelectedTrackAuto, "Enable selected track auto color",	  3, },
	{ { DEFACCEL, "SWS/wol: Enable auto icon for selected track(s)" },			"WOLSELTRAUTOICON_ENABLE",    TEDSelectedTrackAuto, "Enable selected track auto icon",	  4, },
	{ { DEFACCEL, "SWS/wol: Enable auto action for selected track(s)" },		"WOLSELTRAUTOACTION_ENABLE",  TEDSelectedTrackAuto, "Enable selected track auto action",  5, },
	{ { DEFACCEL, "SWS/wol: Disable auto color for selected track(s)" },		"WOLSELTRAUTOCOLOR_DISABLE",  TEDSelectedTrackAuto, "Disable selected track auto color",  6, },
	{ { DEFACCEL, "SWS/wol: Disable auto icon for selected track(s)" },			"WOLSELTRAUTOICON_DISABLE",	  TEDSelectedTrackAuto, "Disable selected track auto icon",	  7, },
	{ { DEFACCEL, "SWS/wol: Disable auto action for selected track(s)" },		"WOLSELTRAUTOACTION_DISABLE", TEDSelectedTrackAuto, "Disable selected track auto action", 8, },

	{ { DEFACCEL, "SWS/wol: Toggle enable track auto selection when an action is performed with auto action" }, "WOL_TENTRAUTOSELAUTO", ToggleEnableTrackAutoSelection, NULL, 0, IsTrackAutoSelectionEnabled },

	{ { DEFACCEL, "SWS/wol: Toggle automatic auto color disable for processed tracks" },   "WOL_TENAUTOCOLORDIS",  ToggleEnableAutoDisable, NULL, 0, IsAutoDisableEnabled },
	{ { DEFACCEL, "SWS/wol: Toggle automatic auto icon disable for processed tracks" },    "WOL_TENAUTOICONDIS",   ToggleEnableAutoDisable, NULL, 1, IsAutoDisableEnabled },
	{ { DEFACCEL, "SWS/wol: Toggle automatic auto action disable for processed tracks " }, "WOL_TENAUTOACTIONDIS", ToggleEnableAutoDisable, NULL, 2, IsAutoDisableEnabled },

	{ { DEFACCEL, "SWS: Apply auto coloring" },	"SWSAUTOCOLOR_APPLY",		ApplyAutoColor,	},

	{ {}, LAST_COMMAND, }, // Denote end of table
};
//!WANT_LOCALIZE_1ST_STRING_END

static void AutoColorMenuHook(const char* menustr, HMENU hMenu, int flag)
{
	if (!strcmp(menustr, "Track control panel context"))
	{
		if (flag == 0)
		{
			HMENU hSubMenu = CreatePopupMenu();
			AddToMenu(hSubMenu, "Enable auto track coloring for selected track(s)", NamedCommandLookup("_WOLSELTRAUTOCOLOR_ENABLE"));
			AddToMenu(hSubMenu, "Enable auto track icon for selected track(s)", NamedCommandLookup("_WOLSELTRAUTOICON_ENABLE"));
			AddToMenu(hSubMenu, "Enable auto action for selected track(s)", NamedCommandLookup("_WOLSELTRAUTOACTION_ENABLE"));
			AddToMenu(hSubMenu, "Disable auto track coloring for selected track(s)", NamedCommandLookup("_WOLSELTRAUTOCOLOR_DISABLE"));
			AddToMenu(hSubMenu, "Disable auto track icon for selected track(s)", NamedCommandLookup("_WOLSELTRAUTOICON_DISABLE"));
			AddToMenu(hSubMenu, "Disable auto action for selected track(s)", NamedCommandLookup("_WOLSELTRAUTOACTION_DISABLE"));

			AddSubMenu(hMenu, hSubMenu, "SWS Auto Color/Icon/Action", 40906);
		}
	}
}

void AutoColorPostInit()
{
	for (int i = 0; i < g_pACItems.GetSize(); i++)
	{
		if (g_pACItems.Get(i)->m_type == AC_TRACK)
		{
			if (g_pACItems.Get(i)->m_actionString.Get()[0] == '_')
			{
				g_pACItems.Get(i)->m_action = NamedCommandLookup(g_pACItems.Get(i)->m_actionString.Get());
				g_pACItems.Get(i)->m_actionName.Set(kbd_getTextFromCmd(g_pACItems.Get(i)->m_action, SectionFromUniqueID(0)));
			}
			else
			{
				g_pACItems.Get(i)->m_action = strtol(g_pACItems.Get(i)->m_actionString.Get(), NULL, 0);
				if (g_pACItems.Get(i)->m_action == -1)
					g_pACItems.Get(i)->m_actionName.Set("");
				else
					g_pACItems.Get(i)->m_actionName.Set(kbd_getTextFromCmd(g_pACItems.Get(i)->m_action, SectionFromUniqueID(0)));
			}
		}
		else
		{
			g_pACItems.Get(i)->m_action = -1;
			g_pACItems.Get(i)->m_actionString.Set("-1");
			g_pACItems.Get(i)->m_actionName.Set(UTF8_BULLET);
		}
	}
	plugin_register("-timer", (void*)AutoColorPostInit);
	g_pACWnd->Update();
}

int AutoColorInitTimer()
{
	plugin_register("timer", (void*)AutoColorPostInit);
	return 1;
}

int AutoColorInit()
{
	if (!plugin_register("projectconfig", &g_projectconfig))
		return 0;

	SWSRegisterCommands(g_commandTable);

	g_ACIni.SetFormatted(MAX_PATH, "%s%csws-autocoloricon.ini", GetResourcePath(), PATH_SLASH_CHAR);

	// "Upgrade" to the new ini file for just the auto color stuff if necessary
	WDL_String ini(g_ACIni);
	bool bUpgrade = false;
	int iCount = GetPrivateProfileInt(SWS_INI, AC_COUNT_KEY, 0, get_ini_file());
	if (iCount)
	{
		bUpgrade = true;
		ini.Set(get_ini_file());
	}
	else
		iCount = GetPrivateProfileInt(SWS_INI, AC_COUNT_KEY, 0, ini.Get());

	// Restore state
	char str[BUFFER_SIZE];
	g_bACEnabled = GetPrivateProfileInt(SWS_INI, AC_ENABLE_KEY, 0, ini.Get()) ? true : false;
	g_bACMEnabled = GetPrivateProfileInt(SWS_INI, ACM_ENABLE_KEY, 0, ini.Get()) ? true : false;
	g_bACREnabled = GetPrivateProfileInt(SWS_INI, ACR_ENABLE_KEY, 0, ini.Get()) ? true : false;
	g_bAIEnabled = GetPrivateProfileInt(SWS_INI, AI_ENABLE_KEY, 0, ini.Get()) ? true : false;
	g_bAATEnabled = GetPrivateProfileInt(SWS_INI, AAT_ENABLE_KEY, 0, g_ACIni.Get()) ? true : false;
	g_bTrackAutoSelectionEn = GetPrivateProfileInt(SWS_INI, TRAUTOSEL_ENABLE_KEY, 0, g_ACIni.Get()) ? true : false;
	g_bAutoColorDisable = GetPrivateProfileInt(SWS_INI, AUTOCOL_DISABLE_KEY, 0, g_ACIni.Get()) ? true : false;
	g_bAutoIconDisable = GetPrivateProfileInt(SWS_INI, AUTOICON_DISABLE_KEY, 0, g_ACIni.Get()) ? true : false;
	g_bAutoActionDisable = GetPrivateProfileInt(SWS_INI, AUTOACTION_DISABLE_KEY, 0, g_ACIni.Get()) ? true : false;

	for (int i = 0; i < iCount; i++)
	{
		char key[32];
		_snprintf(key, 32, AC_ITEM_KEY, i + 1);
		GetPrivateProfileString(SWS_INI, key, "", str, BUFFER_SIZE, ini.Get()); // Read in AutoColor line (stored in str)
		if (bUpgrade) // Remove old lines
			WritePrivateProfileString(SWS_INI, key, NULL, get_ini_file());
		LineParser lp(false);
		if (!lp.parse(str) && lp.getnumtokens() == 5)   //New format: type filter color action icon
			g_pACItems.Add(new SWS_RuleItem(lp.gettoken_int(0), lp.gettoken_str(1), lp.gettoken_int(2), lp.gettoken_str(3), lp.gettoken_str(4)));
		else if (!lp.parse(str) && lp.getnumtokens() == 4)
			g_pACItems.Add(new SWS_RuleItem(lp.gettoken_int(0), lp.gettoken_str(1), lp.gettoken_int(2), "-1", lp.gettoken_str(3)));
		else if (!lp.parse(str) && lp.getnumtokens() == 3) //Reformat old format Autocolor line to new format (i.e. region + marker)
			g_pACItems.Add(new SWS_RuleItem(AC_TRACK, lp.gettoken_str(0), lp.gettoken_int(1), "-1", lp.gettoken_str(2)));
	}

	if (bUpgrade)
	{	// Remove old stuff
		WritePrivateProfileString(SWS_INI, AC_ENABLE_KEY, NULL, get_ini_file());
		WritePrivateProfileString(SWS_INI, AI_ENABLE_KEY, NULL, get_ini_file());
		WritePrivateProfileString(SWS_INI, AC_COUNT_KEY, NULL, get_ini_file());
		AutoColorSaveState();
	}

	ACRegisterUnregisterToMarkerRegionUpdates();

	g_pACWnd = new SWS_AutoColorWnd();

	return 1;
}

void AutoColorSaveState()
{
	// Save state
	char str[BUFFER_SIZE];
	sprintf(str, "%d", g_bACEnabled ? 1 : 0);
	WritePrivateProfileString(SWS_INI, AC_ENABLE_KEY, str, g_ACIni.Get());
	sprintf(str, "%d", g_bACMEnabled ? 1 : 0);
	WritePrivateProfileString(SWS_INI, ACM_ENABLE_KEY, str, g_ACIni.Get());
	sprintf(str, "%d", g_bACREnabled ? 1 : 0);
	WritePrivateProfileString(SWS_INI, ACR_ENABLE_KEY, str, g_ACIni.Get());
	sprintf(str, "%d", g_bAIEnabled ? 1 : 0);
	WritePrivateProfileString(SWS_INI, AI_ENABLE_KEY, str, g_ACIni.Get());
	sprintf(str, "%d", g_bAATEnabled ? 1 : 0);
	WritePrivateProfileString(SWS_INI, AAT_ENABLE_KEY, str, g_ACIni.Get());
	sprintf(str, "%d", g_pACItems.GetSize());
	WritePrivateProfileString(SWS_INI, AC_COUNT_KEY, str, g_ACIni.Get());

	char key[32];
	for (int i = 0; i < g_pACItems.GetSize(); i++)
	{
		_snprintf(key, 32, AC_ITEM_KEY, i+1);
		WDL_String str1, str2;
		makeEscapedConfigString(g_pACItems.Get(i)->m_str_filter.Get(), &str1);
		makeEscapedConfigString(g_pACItems.Get(i)->m_icon.Get(), &str2);
		_snprintf(str, BUFFER_SIZE, "\"%d %s %d %s %s\"", g_pACItems.Get(i)->m_type, str1.Get(), g_pACItems.Get(i)->m_color, g_pACItems.Get(i)->m_actionString.Get(), str2.Get());
		WritePrivateProfileString(SWS_INI, key, str, g_ACIni.Get());
	}
	// Erase the n+1 entry to avoid confusing files
	_snprintf(key, 32, AC_ITEM_KEY, g_pACItems.GetSize() + 1);
	WritePrivateProfileString(SWS_INI, key, NULL, g_ACIni.Get());
}

void AutoColorExit()
{
	UnregisterToMarkerRegionUpdates(&g_mkrRgnListener);
	AutoColorSaveState();
	delete g_pACWnd;
}
