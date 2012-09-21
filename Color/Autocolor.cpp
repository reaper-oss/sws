/******************************************************************************
/ Autocolor.cpp
/
/ Copyright (c) 2010-2012 Tim Payne (SWS), Jeffos
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
#include "Color.h"
#include "Autocolor.h"
#include "../SnM/SnM_Util.h"
#include "../SnM/SnM_ChunkParserPatcher.h"
#include "../reaper/localize.h"
#include "../../WDL/projectcontext.h"

#define PRI_UP_MSG		0x10000
#define PRI_DOWN_MSG	0x10001
#define TYPETYPE_MSG    0x100F0
#define FILTERTYPE_MSG	0x10100
#define RGNFILTERTYPE_MSG	0x100E0
#define COLORTYPE_MSG	0x10110
#define LOAD_ICON_MSG	0x110000
#define CLEAR_ICON_MSG	0x110001


// INI params
#define AC_ENABLE_KEY	"AutoColorEnable"
#define AI_ENABLE_KEY	"AutoIconEnable"
#define AC_COUNT_KEY	"AutoColorCount"
#define AC_ITEM_KEY		"AutoColor %d"


enum { AC_UNNAMED, AC_FOLDER, AC_CHILDREN, AC_RECEIVE, AC_ANY, AC_MASTER, NUM_FILTERTYPES };
enum { AC_RGNUNNAMED, AC_RGNANY, NUM_RGNFILTERTYPES };
enum { AC_CUSTOM, AC_GRADIENT, AC_RANDOM, AC_NONE, AC_PARENT, AC_IGNORE, NUM_COLORTYPES };
enum { AC_TRACK, AC_REGION, AC_MARKER, NUM_TYPETYPES };

// !WANT_LOCALIZE_STRINGS_BEGIN:sws_DLG_115
static const char cTypes[][7] = {"Track", "Region", "Marker" };
static const char cFilterTypes[][11] = { "(unnamed)", "(folder)", "(children)", "(receive)", "(any)", "(master)" };
static const char cRgnFilterTypes[][10] = { "(unnamed)", "(any)" };
static const char cColorTypes[][9] = { "Custom", "Gradient", "Random", "None", "Parent", "Ignore" };
// !WANT_LOCALIZE_STRINGS_END


// Globals
static SWS_AutoColorWnd* g_pACWnd = NULL;
static WDL_PtrList<SWS_RuleItem> g_pACItems;
static SWSProjConfig<WDL_PtrList_DeleteOnDestroy<SWS_RuleTrack> > g_pACTracks;
static bool g_bACEnabled = false;
static bool g_bAIEnabled = false;
static WDL_String g_ACIni;
// !WANT_LOCALIZE_STRINGS_BEGIN:sws_DLG_115
static SWS_LVColumn g_cols[] = { {25, 0, "#" }, {25, 1, "Type"}, { 185, 1, "Filter" }, { 70, 1, "Color" }, { 200, 2, "Icon" }};
// !WANT_LOCALIZE_STRINGS_END

// Prototypes
void AutoColorSaveState();

SWS_AutoColorView::SWS_AutoColorView(HWND hwndList, HWND hwndEdit)
:SWS_ListView(hwndList, hwndEdit, 5, g_cols, "AutoColorViewState", false, "sws_DLG_115")
{
}

void SWS_AutoColorView::GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax)
{
	SWS_RuleItem* pItem = (SWS_RuleItem*)item;
	if (!pItem)
		return;

	switch (iCol)
	{
	case 0: // #
		_snprintf(str, iStrMax, "%d", g_pACItems.Find(pItem) + 1);
		break;
    case 1: // Type
        lstrcpyn(str, pItem->m_str_type.Get(), iStrMax);
        break;
	case 2: // Filter
		lstrcpyn(str, pItem->m_str_filter.Get(), iStrMax);
		break;
	case 3: // Color
		if (pItem->m_color < 0)
			lstrcpyn(str, cColorTypes[-pItem->m_color - 1], iStrMax);
		else
#ifdef _WIN32
			_snprintf(str, iStrMax, "0x%02x%02x%02x", pItem->m_color & 0xFF, (pItem->m_color >> 8) & 0xFF, (pItem->m_color >> 16) & 0xFF); // I think this is in reverse. Ini file dec to hex conversion (correct value for color) shows opposite of what this outputs to screen. e.g. 0xFFFF80 instead of 0x80FFFF
#else
			_snprintf(str, iStrMax, "0x%06x", pItem->m_color);
#endif
		break;
	case 4: // icon
		lstrcpyn(str, pItem->m_icon.Get(), iStrMax);
		break;
	}
}

void SWS_AutoColorView::SetItemText(SWS_ListItem* item, int iCol, const char* str)
{
	SWS_RuleItem* pItem = (SWS_RuleItem*)item;
	if (!pItem)
		return;

	switch (iCol)
	{
    case 1: // Type
        {
            for (int i = 0; i < g_pACItems.GetSize(); i++)
                if ((strcmp(str, "Region") != 0) && (strcmp(str, "Track") != 0) && (strcmp(str, "Marker") != 0))
			    {
				    MessageBox(GetParent(m_hwndList), __LOCALIZE("Type must equal either Track, Region or Marker (CASE SENSITIVE). Please select an option from right click context menu.","sws_DLG_115"), __LOCALIZE("SWS - Error","sws_DLG_115"), MB_OK);
				    return;
			    }
            if (strlen(str))
            pItem->m_str_type.Set(str);
        }
        break;
	case 2: // Filter
        {
		//for (int i = 0; i < g_pACItems.GetSize(); i++)
		//	if (strcmp(g_pACItems.Get(i)->m_str_filter.Get(), str) == 0)
		//	{
  //              MessageBox(GetParent(m_hwndList), __LOCALIZE("Autocolor entry with that name already exists. #set","sws_DLG_115"), __LOCALIZE("SWS - Error","sws_DLG_115"), MB_OK);
		//		return;
		//	}
        if (strlen(str))
			pItem->m_str_filter.Set(str);
        }
		break;
	case 3: // Color
		{
			int iNewCol = strtol(str, NULL, 0);
			pItem->m_color = RGB((iNewCol >> 16) & 0xFF, (iNewCol >> 8) & 0xFF, iNewCol & 0xFF); //This order needs to be checked and maybe changed. See previous comment
			
		}
        break;
    default:
        break;
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
	if (pItem && iCol == 4)
		g_pACWnd->OnCommand(LOAD_ICON_MSG, (LPARAM)pItem);
}

void SWS_AutoColorView::OnItemSelChanged(SWS_ListItem* item, int iState)
{
	g_pACWnd->Update();
}

void SWS_AutoColorView::OnBeginDrag(SWS_ListItem* item)
{
	if (abs(m_iSortCol) == 2)
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

		// Remove the dragged items and then read them
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


SWS_AutoColorWnd::SWS_AutoColorWnd()
:SWS_DockWnd(IDD_AUTOCOLOR, __LOCALIZE("Auto Color/Icon","sws_DLG_115"), "SWSAutoColor", 30005, SWSGetCommandID(OpenAutoColor))
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
		// Set the checkbox
		CheckDlgButton(m_hwnd, IDC_AUTOCOLOR, g_bACEnabled ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(m_hwnd, IDC_AUTOICON, g_bAIEnabled ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemText(m_hwnd, IDC_APPLY, (g_bACEnabled || g_bAIEnabled) ? __LOCALIZE("Force","sws_DLG_115") : __LOCALIZE("Apply","sws_DLG_115"));

		// Redraw the owner drawn button
#ifdef _WIN32
		RedrawWindow(GetDlgItem(m_hwnd, IDC_COLOR), NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
#else
		InvalidateRect(GetDlgItem(m_hwnd, IDC_COLOR), NULL, 0);
#endif							

		if (m_pLists.GetSize())
			m_pLists.Get(0)->Update();

		AutoColorSaveState();
		AutoColorRun(false);
	}
}

void SWS_AutoColorWnd::OnInitDlg()
{
	m_resize.init_item(IDC_LIST,      0.0, 0.0, 1.0, 1.0);
	m_resize.init_item(IDC_ADD,       0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_REMOVE,    0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_COLOR,     0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_AUTOCOLOR, 0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_AUTOICON,  0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_APPLY,     1.0, 1.0, 1.0, 1.0);
	m_pView = new SWS_AutoColorView(GetDlgItem(m_hwnd, IDC_LIST), GetDlgItem(m_hwnd, IDC_EDIT));
	m_pLists.Add(m_pView);
	Update();
}

void SWS_AutoColorWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
        case IDC_APPLY:
            {
		        AutoColorRun(true);
            }
		    break;
		case IDC_ADD:
            // default options when we add a new row
			g_pACItems.Add(new SWS_RuleItem(__LOCALIZE("Track","sws_DLG_115"), __LOCALIZE("(name)","sws_DLG_115"), -AC_NONE-1, ""));
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
		case IDC_AUTOCOLOR:
			g_bACEnabled = !g_bACEnabled;
			Update();
			break;
		case IDC_AUTOICON:
			g_bAIEnabled = !g_bAIEnabled;
			Update();
			break;
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
						item->m_color = cc.rgbResult;
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

		default:
            if (wParam >= TYPETYPE_MSG && wParam < TYPETYPE_MSG + NUM_TYPETYPES)
            {
                SWS_RuleItem* item = (SWS_RuleItem*)m_pLists.Get(0)->EnumSelected(NULL);
                if (item)
                {
                    item->m_str_type.Set(cTypes[wParam-TYPETYPE_MSG]);
                }
                Update();
            } else if (wParam >= FILTERTYPE_MSG && wParam < FILTERTYPE_MSG + NUM_FILTERTYPES)
			{
				SWS_RuleItem* item = (SWS_RuleItem*)m_pLists.Get(0)->EnumSelected(NULL);
				if (item)
				{
					int iType = (int)wParam - FILTERTYPE_MSG;
					for (int i = 0; i < g_pACItems.GetSize(); i++)
						if (strcmp(g_pACItems.Get(i)->m_str_filter.Get(), cFilterTypes[iType]) == 0)
						{
                            MessageBox(m_hwnd, __LOCALIZE("Autocolor entry of that type already exists. #oncommand","sws_DLG_115"), __LOCALIZE("SWS - Error","sws_DLG_115"), MB_OK); //FIXME : Brad
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

#ifndef _WIN32
void SWS_AutoColorWnd::OnTimer(WPARAM wParam)
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

void SWS_AutoColorWnd::OnDestroy()
{
	if (m_bSettingColor)
	{
		HideColorChooser();
		KillTimer(m_hwnd, 1);
	}
}
#endif

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

HMENU SWS_AutoColorWnd::OnContextMenu(int x, int y, bool* wantDefaultItems)
{
	int iCol;
	SWS_ListItem* item = m_pLists.Get(0)->GetHitItem(x, y, &iCol);
	HMENU hMenu = CreatePopupMenu();

    if (item && iCol == 1)
    {
        for (int i = 0; i < NUM_TYPETYPES; i++)
            AddToMenu(hMenu, cTypes[i], TYPETYPE_MSG + i);
        AddToMenu(hMenu, SWS_SEPARATOR, 0);
    }
	else if (item && iCol == 2)
	{
        // Display a different set of options for Regions and Markers
        int x = 0;
		SWS_RuleItem* ICitem;
		while ((ICitem = (SWS_RuleItem*)m_pLists.Get(0)->EnumSelected(&x)))
        {
            if(strcmp(ICitem->m_str_type.Get(),"Track") == 0)
            {
		        for (int i = 0; i < NUM_FILTERTYPES; i++)
			        AddToMenu(hMenu, cFilterTypes[i], FILTERTYPE_MSG + i);
		            AddToMenu(hMenu, SWS_SEPARATOR, 0);
            }
            //else
            //    for (int i = 0; i < NUM_RGNFILTERTYPES; i++)
		          //  AddToMenu(hMenu, cRgnFilterTypes[i], RGNFILTERTYPE_MSG + i);
            //        AddToMenu(hMenu, SWS_SEPARATOR, 0);
        }
	}
	else if (item && iCol == 3)
	{
		AddToMenu(hMenu, __LOCALIZE("Set color...","sws_DLG_115"), IDC_COLOR);
		AddToMenu(hMenu, SWS_SEPARATOR, 0);

        // Dont display special color options for Regions and Markers
        int x = 0;
		SWS_RuleItem* ICitem;
		while ((ICitem = (SWS_RuleItem*)m_pLists.Get(0)->EnumSelected(&x)))
        {
            if(strcmp(ICitem->m_str_type.Get(),"Track") == 0)
            {
		        for (int i = 0; i < NUM_COLORTYPES; i++)
			        AddToMenu(hMenu, cColorTypes[i], COLORTYPE_MSG + i);
		            AddToMenu(hMenu, SWS_SEPARATOR, 0);
            }
        }
	}
	else if (item && iCol == 4)
	{
        // Do not display icon options if Type column is Region or Marker
        int x = 0;
		SWS_RuleItem* ICitem;
		while ((ICitem = (SWS_RuleItem*)m_pLists.Get(0)->EnumSelected(&x)))
        {
			if(strcmp(ICitem->m_str_type.Get(),"Track") == 0)
            {
		        AddToMenu(hMenu, __LOCALIZE("Load icon...","sws_DLG_115"), LOAD_ICON_MSG);
		        AddToMenu(hMenu, __LOCALIZE("Clear icon","sws_DLG_115"), CLEAR_ICON_MSG);
		        AddToMenu(hMenu, SWS_SEPARATOR, 0);
            }
        }
	}

	if (item)
	{
		AddToMenu(hMenu, __LOCALIZE("Up in priority","sws_DLG_115"), PRI_UP_MSG);
		AddToMenu(hMenu, __LOCALIZE("Down in priority","sws_DLG_115"), PRI_DOWN_MSG);
	}

	AddToMenu(hMenu, __LOCALIZE("Show color management window","sws_DLG_115"), SWSGetCommandID(ShowColorDialog));

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

void OpenAutoColor(COMMAND_T*)
{
	g_pACWnd->Show(true, true);
}

void ApplyColorRuleToRegion(SWS_RuleItem* rule)
{
    double pos, end;
    int num, color;
    bool isRegn;
    char* name;
    char* itemtype = "Region";
    int x = 0;

    if(strcmp(itemtype, rule->m_str_type.Get()) == 0)
    {
        // Go through all markers and find the information for the one we want to change
        while (EnumProjectMarkers3(NULL, x, NULL, NULL, NULL, NULL, NULL, NULL))
        {
                if(EnumProjectMarkers3(NULL, x, &isRegn, &pos, &end, &name, &num, &color))
                {
                    // Compare the names to check if we need to recolor
                    if ((name && stristr(name, rule->m_str_filter.Get())) && isRegn)
                    {
                        if(SNM_SetProjectMarker(NULL, num, isRegn, pos, end, name, rule->m_color | 0x1000000))
                        {
                            UpdateTimeline();
                        }
                    }
                }
        x++;
        }
    }
    else
        return;
}

void ApplyColorRuleToMarker(SWS_RuleItem* rule)
{
    double pos, end;
    int num, color;
    bool isRegn;
    char* name;
    char* itemtype = "Marker";
    int x = 0;

    if(strcmp(itemtype, rule->m_str_type.Get()) == 0)
    {
        // Go through all markers and find the information for the one we want to change
        while (EnumProjectMarkers3(NULL, x, NULL, NULL, NULL, NULL, NULL, NULL))
        {
            if(EnumProjectMarkers3(NULL, x, &isRegn, &pos, &end, &name, &num, &color))
            {
                // Compare the names to check if we need to recolor
                if ((name && stristr(name, rule->m_str_filter.Get())) && !isRegn)
                {
                    if(SNM_SetProjectMarker(NULL, num, isRegn, pos, end, name, rule->m_color | 0x1000000))
                    {
                        UpdateTimeline();
                    }
                }
            }
        x++;
        }
    }
    else
        return;
}

void ApplyColorRuleToTrack(SWS_RuleItem* rule, bool bDoColors, bool bDoIcons, bool bForce)
{
    char* itemtype = "Track";

    if(strcmp(itemtype, rule->m_str_type.Get()) == 0)
    {
	    if (!bDoColors && !bDoIcons)
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
            if (bColor || bIcon)
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

	            if (bMatch)
	            {
		            // Set the color
		            if (bColor)
		            {
			            int iCurColor = *(int*)GetSetMediaTrackInfo(tr, "I_CUSTOMCOLOR", NULL);
			            if (!(iCurColor & 0x1000000))
				            iCurColor = 0;
			            int newCol = iCurColor;

			            if (rule->m_color == -AC_RANDOM-1)
			            {
				            // Only randomize once
				            if (!(iCurColor & 0x1000000))
					            newCol = RGB(rand() % 256, rand() % 256, rand() % 256) | 0x1000000;
			            }
			            else if (rule->m_color == -AC_CUSTOM-1)
			            {
				            if (!AllBlack())
					            while(!(newCol = g_custColors[iCount++ % 16]));
				            newCol |= 0x1000000;							
			            }
			            else if (rule->m_color == -AC_GRADIENT-1)
				            gradientTracks.Add(tr);
			            else if (rule->m_color == -AC_NONE-1)
				            newCol = 0;
			            else if (rule->m_color == -AC_PARENT-1)
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
				            GetSetMediaTrackInfo(tr, "I_CUSTOMCOLOR", &newCol);

			            pACTrack->m_col = newCol;
			            pACTrack->m_bColored = true;
		            }

		            if (bIcon)
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
		            }
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
    else
        return;
}

// Here's the meat and potatoes, apply the colors!
void AutoColorRun(bool bForce)
{
	static bool bRecurse = false;
	if (bRecurse || (!g_bACEnabled && !g_bAIEnabled && !bForce))
		return;
	bRecurse = true;

	// If forcing, start over with the saved track list
	if (bForce)
		g_pACTracks.Get()->Empty(true);
	else
		// Remove non-existant tracks from the autocolortracklist
		for (int i = 0; i < g_pACTracks.Get()->GetSize(); i++)
			if (CSurf_TrackToID(g_pACTracks.Get()->Get(i)->m_pTr, false) < 0)
			{
				g_pACTracks.Get()->Delete(i, true);
				i--;
			}

	// Clear the "colored" bit and "iconed" bit
	for (int i = 0; i < g_pACTracks.Get()->GetSize(); i++)
	{
		g_pACTracks.Get()->Get(i)->m_bColored = false;
		g_pACTracks.Get()->Get(i)->m_bIconed = false;
	}

	// Apply the rules
	SWS_CacheObjectState(true);
	bool bDoColors = g_bACEnabled || bForce;
	bool bDoIcons  = g_bAIEnabled || bForce;

	for (int i = 0; i < g_pACItems.GetSize(); i++)
    {
		ApplyColorRuleToTrack(g_pACItems.Get(i), bDoColors, bDoIcons, bForce);
        ApplyColorRuleToRegion(g_pACItems.Get(i));
        ApplyColorRuleToMarker(g_pACItems.Get(i));
    }

	// Remove colors/icons if necessary
	for (int i = 0; i < g_pACTracks.Get()->GetSize(); i++)
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

	if (bForce)
		Undo_OnStateChangeEx(__LOCALIZE("Apply auto color/icon","sws_undo"), UNDO_STATE_TRACKCFG | UNDO_STATE_MISCCFG, -1);
	bRecurse = false;
}


void EnableAutoColor(COMMAND_T*)
{
	g_bACEnabled = !g_bACEnabled;
	g_pACWnd->Update();
}

void EnableAutoIcon(COMMAND_T*)
{
	g_bAIEnabled = !g_bAIEnabled;
	g_pACWnd->Update();
}

void ApplyAutoColor(COMMAND_T*)
{
	AutoColorRun(true);
}

static bool IsAutoColorOpen(COMMAND_T*)		{ return g_pACWnd->IsValidWindow(); }
static bool IsAutoColorEnabled(COMMAND_T*)	{ return g_bACEnabled; }
static bool IsAutoIconEnabled(COMMAND_T*)	{ return g_bAIEnabled; }

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
					rt->m_icon.Set(lp.gettoken_str(2));
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
		ctx->AddLine("<SWSAUTOCOLOR");
		char str[128];
		for (int i = 0; i < g_pACTracks.Get()->GetSize(); i++)
		{
			SWS_RuleTrack* rt = g_pACTracks.Get()->Get(i);
			GUID g = GUID_NULL;
			if (CSurf_TrackToID(rt->m_pTr, false))
				g = *(GUID*)GetSetMediaTrackInfo(rt->m_pTr, "GUID", NULL);
			guidToString(&g, str);
			sprintf(str+strlen(str), " %d \"%s\"", rt->m_col, rt->m_icon.Get());
			ctx->AddLine("%s",str);
		}
		ctx->AddLine(">");
	}
}

static void BeginLoadProjectState(bool isUndo, struct project_config_extension_t *reg)
{
	g_pACTracks.Cleanup();
	g_pACTracks.Get()->Empty(true);
}

static project_config_extension_t g_projectconfig = { ProcessExtensionLine, SaveExtensionConfig, BeginLoadProjectState, NULL };

//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "SWS: Open auto color/icon window" },	"SWSAUTOCOLOR_OPEN",	OpenAutoColor,		"SWS Auto color/icon",		0, IsAutoColorOpen },
	{ { DEFACCEL, "SWS: Toggle auto coloring enable" },	"SWSAUTOCOLOR_ENABLE",	EnableAutoColor,	"Enable SWS auto coloring", 0, IsAutoColorEnabled },
	{ { DEFACCEL, "SWS/S&M: Toggle auto icon enable" },	"S&MAUTOICON_ENABLE",	EnableAutoIcon,		"Enable SWS auto icon",		0, IsAutoIconEnabled },
	{ { DEFACCEL, "SWS: Apply auto coloring" },			"SWSAUTOCOLOR_APPLY",	ApplyAutoColor,	},
	{ {}, LAST_COMMAND, }, // Denote end of table
};
//!WANT_LOCALIZE_1ST_STRING_END

int AutoColorInit()
{
	if (!plugin_register("projectconfig",&g_projectconfig))
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
	g_bAIEnabled = GetPrivateProfileInt(SWS_INI, AI_ENABLE_KEY, 0, ini.Get()) ? true : false;

	for (int i = 0; i < iCount; i++)
	{
		char key[32];
		_snprintf(key, 32, AC_ITEM_KEY, i+1);
		GetPrivateProfileString(SWS_INI, key, "", str, BUFFER_SIZE, ini.Get()); // Read in AutoColor line (stored in str)
		if (bUpgrade) // Remove old lines
			WritePrivateProfileString(SWS_INI, key, NULL, get_ini_file());
		LineParser lp(false);
		if (!lp.parse(str) && lp.getnumtokens() == 4)
			g_pACItems.Add(new SWS_RuleItem(lp.gettoken_str(0), lp.gettoken_str(1), lp.gettoken_int(2), lp.gettoken_str(3)));
        else if(!lp.parse(str) && lp.getnumtokens() == 3) //Reformat old format Autocolor line to new format
            g_pACItems.Add(new SWS_RuleItem("Track", lp.gettoken_str(0), lp.gettoken_int(1), lp.gettoken_str(2)));
	}	

	if (bUpgrade)
	{	// Remove old stuff
		WritePrivateProfileString(SWS_INI, AC_ENABLE_KEY, NULL, get_ini_file());
		WritePrivateProfileString(SWS_INI, AI_ENABLE_KEY, NULL, get_ini_file());
		WritePrivateProfileString(SWS_INI, AC_COUNT_KEY, NULL, get_ini_file());
		AutoColorSaveState();
	}

	g_pACWnd = new SWS_AutoColorWnd();

	return 1;
}

void AutoColorSaveState()
{
	// Save state
	char str[BUFFER_SIZE];
	sprintf(str, "%d", g_bACEnabled ? 1 : 0);
	WritePrivateProfileString(SWS_INI, AC_ENABLE_KEY, str, g_ACIni.Get());
	sprintf(str, "%d", g_bAIEnabled ? 1 : 0);
	WritePrivateProfileString(SWS_INI, AI_ENABLE_KEY, str, g_ACIni.Get());
	sprintf(str, "%d", g_pACItems.GetSize());
	WritePrivateProfileString(SWS_INI, AC_COUNT_KEY, str, g_ACIni.Get());

	char key[32];
	for (int i = 0; i < g_pACItems.GetSize(); i++)
	{
		_snprintf(key, 32, AC_ITEM_KEY, i+1);
		WDL_String str1, str2, str3;
        makeEscapedConfigString(g_pACItems.Get(i)->m_str_type.Get(), &str1);
		makeEscapedConfigString(g_pACItems.Get(i)->m_str_filter.Get(), &str2);
		makeEscapedConfigString(g_pACItems.Get(i)->m_icon.Get(), &str3);
		_snprintf(str, BUFFER_SIZE, "\"%s %s %d %s\"", str1.Get(), str2.Get(), g_pACItems.Get(i)->m_color, str3.Get());
		WritePrivateProfileString(SWS_INI, key, str, g_ACIni.Get());
	}
	// Erase the n+1 entry to avoid confusing files
	_snprintf(key, 32, AC_ITEM_KEY, g_pACItems.GetSize() + 1);
	WritePrivateProfileString(SWS_INI, key, NULL, g_ACIni.Get());
}

void AutoColorExit()
{
	AutoColorSaveState();
	delete g_pACWnd;
}
