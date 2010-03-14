/******************************************************************************
/ Autocolor.cpp
/
/ Copyright (c) 2010 Tim Payne (SWS)
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

#define PRI_UP_MSG		0x10000
#define PRI_DOWN_MSG	0x10001
#define SELNEXT_MSG		0x100F1
#define SELPREV_MSG		0x100F2
#define TRACKTYPE_MSG	0x10100
#define COLORTYPE_MSG	0x10110

// INI params
#define AC_ENABLE_KEY	"AutoColorEnable"
#define AC_COUNT_KEY	"AutoColorCount"
#define AC_ITEM_KEY		"AutoColor %d"

enum { AC_UNNAMED, AC_FOLDER, AC_CHILDREN, AC_RECEIVE, AC_ANY, NUM_TRACKTYPES };
static const char cTrackTypes[][11] = { "(unnamed)", "(folder)", "(children)", "(receive)", "(any)" };

enum { AC_CUSTOM, AC_GRADIENT, AC_RANDOM, AC_NONE, NUM_COLORTYPES };
static const char cColorTypes[][9] = { "Custom", "Gradient", "Random", "None" };

// Globals
static SWS_AutoColorWnd* g_pACWnd = NULL;
static WDL_PtrList<SWS_AutoColorItem> g_pACItems;
static SWSProjConfig<WDL_PtrList<SWS_AutoColorTrack> > g_pACTracks;
static bool g_bACEnabled = false;

static SWS_LVColumn g_cols[] = { {25, 0, "#" }, { 185, 1, "Filter" }, { 70, 1, "Color" }, };

SWS_AutoColorView::SWS_AutoColorView(HWND hwndList, HWND hwndEdit)
:SWS_ListView(hwndList, hwndEdit, 3, g_cols, "AutoColorViewState", false)
{
}

void SWS_AutoColorView::GetItemText(LPARAM item, int iCol, char* str, int iStrMax)
{
	SWS_AutoColorItem* pItem = (SWS_AutoColorItem*)item;
	if (!pItem)
		return;

	switch (iCol)
	{
	case 0: // #
		_snprintf(str, iStrMax, "%d", g_pACItems.Find(pItem) + 1);
		break;
	case 1: // Filter
		lstrcpyn(str, pItem->m_str.Get(), iStrMax);
		break;
	case 2: // Color
		if (pItem->m_col < 0)
			lstrcpyn(str, cColorTypes[-pItem->m_col - 1], iStrMax);
		else
			_snprintf(str, iStrMax, "0x%02x%02x%02x", pItem->m_col & 0xFF, (pItem->m_col >> 8) & 0xFF, (pItem->m_col >> 16) & 0xFF);
		break;
	}
}

void SWS_AutoColorView::SetItemText(LPARAM item, int iCol, const char* str)
{
	SWS_AutoColorItem* pItem = (SWS_AutoColorItem*)item;
	if (!pItem)
		return;

	switch (iCol)
	{
	case 1: // Filter
		for (int i = 0; i < g_pACItems.GetSize(); i++)
			if (strcmp(g_pACItems.Get(i)->m_str.Get(), str) == 0)
			{
				MessageBox(GetParent(m_hwndList), "Autocolor entry with that name already exists.", "Autocolor Error", MB_OK);
				return;
			}
		if (strlen(str))
			pItem->m_str.Set(str);
		break;
	case 2: // Color
		pItem->m_col = strtol(str, NULL, 0);
		break;
	}

	Update();
}

void SWS_AutoColorView::GetItemList(WDL_TypedBuf<LPARAM>* pBuf)
{
	pBuf->Resize(g_pACItems.GetSize());
	for (int i = 0; i < pBuf->GetSize(); i++)
		pBuf->Get()[i] = (LPARAM)g_pACItems.Get(i);
}

void SWS_AutoColorView::OnItemSelChanged(LPARAM item, bool bSel)
{
	g_pACWnd->Update();
}

int SWS_AutoColorView::OnItemSort(LPARAM item1, LPARAM item2)
{
	if (abs(m_iSortCol) == 1)
	{
		int iRet;
		int i1 = g_pACItems.Find((SWS_AutoColorItem*)item1);
		int i2 = g_pACItems.Find((SWS_AutoColorItem*)item2);
		if (i1 > i2)
			iRet = 1;
		else if (i1 < i2)
			iRet = -1;
		if (m_iSortCol < 0)
			return -iRet;
		else
			return iRet;
	}
	return SWS_ListView::OnItemSort(item1, item2);
}

SWS_AutoColorWnd::SWS_AutoColorWnd()
:SWS_DockWnd(IDD_AUTOCOLOR, "AutoColor", 30005)
#ifndef _WIN32
	,m_bSettingColor(false)
#endif
{
	if (m_bShowAfterInit)
		Show(false, false);
}

void SWS_AutoColorWnd::Update()
{
	if (IsValidWindow())
	{
		// Set the checkbox
		CheckDlgButton(m_hwnd, IDC_ENABLED, g_bACEnabled ? BST_CHECKED : BST_UNCHECKED);

		// Redraw the owner drawn button
#ifdef _WIN32
		RedrawWindow(GetDlgItem(m_hwnd, IDC_COLOR), NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
#else
		InvalidateRect(GetDlgItem(m_hwnd, IDC_COLOR), NULL, 0);
#endif							

		if (m_pLists.GetSize())
			m_pLists.Get(0)->Update();
	}
}

void SWS_AutoColorWnd::OnInitDlg()
{
	m_resize.init_item(IDC_LIST,      0.0, 0.0, 1.0, 1.0);
	m_resize.init_item(IDC_ADD,       0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_REMOVE,    0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_COLOR,     0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_ENABLED,   0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_APPLY,     1.0, 1.0, 1.0, 1.0);
	m_pLists.Add(new SWS_AutoColorView(GetDlgItem(m_hwnd, IDC_LIST), GetDlgItem(m_hwnd, IDC_EDIT)));
	Update();
}

void SWS_AutoColorWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
		case IDC_APPLY:
			AutoColorRun(true);
			break;
		case IDC_ADD:
			g_pACItems.Add(new SWS_AutoColorItem("(name)", 0));
			Update();
			break;
		case IDC_REMOVE:
		{
			int x = 0;
			SWS_AutoColorItem* item;
			while((item = (SWS_AutoColorItem*)m_pLists.Get(0)->EnumSelected(&x)))
			{
				int idx = g_pACItems.Find(item);
				if (idx >= 0)
					g_pACItems.Delete(idx);
			}
			Update();
			break;
		}
		case IDC_ENABLED:
			g_bACEnabled = !g_bACEnabled;
			Update();
			break;
		case IDC_COLOR:
		{
			SWS_AutoColorItem* item = (SWS_AutoColorItem*)m_pLists.Get(0)->EnumSelected(NULL);
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
				cc.rgbResult = item->m_col;
				if (ChooseColor(&cc))
				{
					int x = 0;
					while ((item = (SWS_AutoColorItem*)m_pLists.Get(0)->EnumSelected(&x)))
						item->m_col = cc.rgbResult;
				}
				Update();
#else
				m_bSettingColor = true;
				ShowColorChooser(item->m_col);
				SetTimer(m_hwnd, 0, 50, NULL);
#endif
			}
			break;
		}
		case PRI_UP_MSG:
		{
			int x = 0;
			SWS_AutoColorItem* item;
			while ((item = (SWS_AutoColorItem*)m_pLists.Get(0)->EnumSelected(&x)))
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
					SWS_AutoColorItem* item = (SWS_AutoColorItem*)m_pLists.Get(0)->GetListItem(i);
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
		default:
			if (wParam >= TRACKTYPE_MSG && wParam < TRACKTYPE_MSG + NUM_TRACKTYPES)
			{
				SWS_AutoColorItem* item = (SWS_AutoColorItem*)m_pLists.Get(0)->EnumSelected(NULL);
				if (item)
				{
					int iType = wParam - TRACKTYPE_MSG;
					for (int i = 0; i < g_pACItems.GetSize(); i++)
						if (strcmp(g_pACItems.Get(i)->m_str.Get(), cTrackTypes[iType]) == 0)
						{
							MessageBox(m_hwnd, "Autocolor entry of that type already exists.", "Autocolor Error", MB_OK);
							return;
						}

					item->m_str.Set(cTrackTypes[wParam-TRACKTYPE_MSG]);
				}
				Update();
			}
			else if (wParam >= COLORTYPE_MSG && wParam < COLORTYPE_MSG + NUM_COLORTYPES)
			{
				int x = 0;
				SWS_AutoColorItem* item;
				while ((item = (SWS_AutoColorItem*)m_pLists.Get(0)->EnumSelected(&x)))
					item->m_col = -1 - (wParam - COLORTYPE_MSG);
				Update();
			}
			else
				Main_OnCommand((int)wParam, (int)lParam);
	}
}

#ifndef _WIN32
void SWS_AutoColorWnd::OnTimer()
{
	COLORREF cr;
	if (m_bSettingColor && GetChosenColor(&cr))
	{
		int x = 0;
		SWS_AutoColorItem* item;
		while ((item = (SWS_AutoColorItem*)m_pLists.Get(0)->EnumSelected(&x)))
			item->m_col = cr;

		KillTimer(m_hwnd, 0);
		Update();
	}
}

void SWS_AutoColorWnd::OnDestroy()
{
	if (m_bSettingColor)
	{
		HideColorChooser();
		KillTimer(m_hwnd, 0);
	}
}
#endif

int SWS_AutoColorWnd::OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
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
			SWS_AutoColorItem* item;
			while ((item = (SWS_AutoColorItem*)m_pLists.Get(0)->EnumSelected(&x)))
			{
				if (col < 0)
					col = item->m_col;
				else if (col != item->m_col)
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

HMENU SWS_AutoColorWnd::OnContextMenu(int x, int y)
{
	int iCol;
	LPARAM item = m_pLists.Get(0)->GetHitItem(x, y, &iCol);
	HMENU hMenu = CreatePopupMenu();

	if (item && iCol == 1)
	{
		for (int i = 0; i < NUM_TRACKTYPES; i++)
			AddToMenu(hMenu, cTrackTypes[i], TRACKTYPE_MSG + i);
		AddToMenu(hMenu, SWS_SEPARATOR, 0);
	}
	else if (item && iCol == 2)
	{
		for (int i = 0; i < NUM_COLORTYPES; i++)
			AddToMenu(hMenu, cColorTypes[i], COLORTYPE_MSG + i);
		AddToMenu(hMenu, SWS_SEPARATOR, 0);
	}

	if (item)
	{
		AddToMenu(hMenu, "Up in priority", PRI_UP_MSG);
		AddToMenu(hMenu, "Down in priority", PRI_DOWN_MSG);
	}

	AddToMenu(hMenu, "Color management...", SWSGetCommandID(ShowColorDialog));

	return hMenu;
}

void OpenAutoColor(COMMAND_T*)
{
	g_pACWnd->Show(true, true);
}

void ApplyColorRule(SWS_AutoColorItem* rule)
{
	int iCount = 0;
	WDL_PtrList<void> gradientTracks;

	if (rule->m_col == -AC_CUSTOM-1)
		UpdateCustomColors();

	// Check all tracks for matching strings/properties
	MediaTrack* temp;
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		int iCurColor = *(int*)GetSetMediaTrackInfo(tr, "I_CUSTOMCOLOR", NULL);

		bool bAutocolored = false;
		bool bFound = false;
		for (int k = 0; k < g_pACTracks.Get()->GetSize(); k++)
			if (g_pACTracks.Get()->Get(k)->m_pTr == tr)
			{
				bFound = true;
				if (!g_pACTracks.Get()->Get(k)->m_bColored && iCurColor == g_pACTracks.Get()->Get(k)->m_col)
				{
					bAutocolored = true;
					break;
				}
			}
		
		// New tracks are by default autocolored
		if (!bFound)
			bAutocolored = true;

		if (bAutocolored)
		{
			bool bColor = false;

			// Check "special" rules first
			if (strcmp(rule->m_str.Get(), cTrackTypes[AC_FOLDER]) == 0)
			{
				int iType;
				GetFolderDepth(tr, &iType, &temp);
				if (iType == 1)
					bColor = true;
			}
			else if (strcmp(rule->m_str.Get(), cTrackTypes[AC_CHILDREN]) == 0)
			{
				if (GetFolderDepth(tr, NULL, &temp) >= 1)
					bColor = true;
			}
			else if (strcmp(rule->m_str.Get(), cTrackTypes[AC_RECEIVE]) == 0)
			{
				if (GetSetTrackSendInfo(tr, -1, 0, "P_SRCTRACK", NULL))
					bColor = true;
			}
			else if (strcmp(rule->m_str.Get(), cTrackTypes[AC_UNNAMED]) == 0)
			{
				char* cName = (char*)GetSetMediaTrackInfo(tr, "P_NAME", NULL);
				if (!cName || !cName[0])
					bColor = true;
			}
			else if (strcmp(rule->m_str.Get(), cTrackTypes[AC_ANY]) == 0)
				bColor = true;
			else
			{
				char* cName = (char*)GetSetMediaTrackInfo(tr, "P_NAME", NULL);
				if (cName && stristr(cName, rule->m_str.Get()))
					bColor = true;
			}

			if (bColor)
			{
				int newCol = iCurColor;

				if (rule->m_col == -AC_RANDOM-1)
					newCol = RGB(rand() % 256, rand() % 256, rand() % 256) | 0x1000000;
				else if (rule->m_col == -AC_CUSTOM-1)
					newCol = g_custColors[iCount++ % 16] | 0x1000000;
				else if (rule->m_col == -AC_GRADIENT-1)
					gradientTracks.Add(tr);
				else if (rule->m_col == -AC_NONE-1)
					newCol = 0;
				else
					newCol = rule->m_col | 0x1000000;

				if (newCol != iCurColor)
					GetSetMediaTrackInfo(tr, "I_CUSTOMCOLOR", &newCol);

				bool bFound = false;
				for (int k = 0; k < g_pACTracks.Get()->GetSize(); k++)
					if (g_pACTracks.Get()->Get(k)->m_pTr == tr)
					{
						g_pACTracks.Get()->Get(k)->m_col = newCol;
						g_pACTracks.Get()->Get(k)->m_bColored = true;
						bFound = true;
						break;
					}
				if (!bFound)
					g_pACTracks.Get()->Add(new SWS_AutoColorTrack(tr, newCol));
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

// Here's the meat and potatoes, apply the colors!
void AutoColorRun(bool bForce)
{
	if (!g_bACEnabled && !bForce)
		return;

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

	// Clear the "colored" bit
	for (int i = 0; i < g_pACTracks.Get()->GetSize(); i++)
		g_pACTracks.Get()->Get(i)->m_bColored = false;

	// Apply the rules
	for (int i = 0; i < g_pACItems.GetSize(); i++)
		ApplyColorRule(g_pACItems.Get(i));

	if (bForce)
		Undo_OnStateChangeEx("Apply SWS autocolor", UNDO_STATE_TRACKCFG, -1);
}


void EnableAutoColor(COMMAND_T*)
{
	g_bACEnabled = !g_bACEnabled;
	g_pACWnd->Update();
}

void ApplyAutoColor(COMMAND_T*)
{
	AutoColorRun(true);
}

static bool IsAutoColorOpen(COMMAND_T*)		{ return g_pACWnd->IsValidWindow(); }
static bool IsAutoColorEnabled(COMMAND_T*)	{ return g_bACEnabled; }

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
					g_pACTracks.Get()->Add(new SWS_AutoColorTrack(tr, lp.gettoken_int(1)));
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
			GUID* g = (GUID*)GetSetMediaTrackInfo(g_pACTracks.Get()->Get(i)->m_pTr, "GUID", NULL);
			if (g)
			{
				guidToString(g, str);
				sprintf(str+strlen(str), " %d", g_pACTracks.Get()->Get(i)->m_col);
				ctx->AddLine(str);
			}
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

static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "SWS: Open auto color window" },		"SWSAUTOCOLOR_OPEN",	OpenAutoColor,		"SWS Auto Color",			0, IsAutoColorOpen },
	{ { DEFACCEL, "SWS: Enable auto coloring" },		"SWSAUTOCOLOR_ENABLE",	EnableAutoColor,	"Enable SWS auto coloring", 0, IsAutoColorEnabled },
	{ { DEFACCEL, "SWS: Apply auto coloring" },			"SWSAUTOCOLOR_APPLY",	ApplyAutoColor,	},
	{ {}, LAST_COMMAND, }, // Denote end of table
};

static int translateAccel(MSG *msg, accelerator_register_t *ctx)
{
	if (g_pACWnd->IsActive())
	{
		if (msg->message == WM_KEYDOWN)
		{
			bool bCtrl  = GetAsyncKeyState(VK_CONTROL) & 0x8000 ? true : false;
			bool bAlt   = GetAsyncKeyState(VK_MENU)    & 0x8000 ? true : false;
			bool bShift = GetAsyncKeyState(VK_SHIFT)   & 0x8000 ? true : false;

			if (msg->wParam == VK_DELETE && !bCtrl && !bAlt && !bShift)
			{
				SendMessage(g_pACWnd->GetHWND(), WM_COMMAND, IDC_REMOVE, 0);
				return 1;
			}
			/*else if (msg->wParam == VK_UP && !bCtrl && !bAlt && !bShift)
			{
				SendMessage(g_pACWnd->GetHWND(), WM_COMMAND, SELPREV_MSG, 0);
				return 1;
			}
			else if (msg->wParam == VK_DOWN && !bCtrl && !bAlt && !bShift)
			{
				SendMessage(g_pACWnd->GetHWND(), WM_COMMAND, SELNEXT_MSG, 0);
				return 1;
			}*/
		}
		return -666;
	}
	return 0;
} 

static accelerator_register_t g_ar = { translateAccel, TRUE, NULL };

static void menuhook(const char* menustr, HMENU hMenu, int flag)
{
	if (strcmp(menustr, "Main view") == 0 && flag == 0)
		AddToMenu(hMenu, g_commandTable[0].menuText, g_commandTable[0].accel.accel.cmd, 40075);
	else if (flag == 0 && strcmp(menustr, "Main options") == 0)
		AddToMenu(hMenu, g_commandTable[1].menuText, g_commandTable[1].accel.accel.cmd, 40745);
}

int AutoColorInit()
{
	if (!plugin_register("accelerator",&g_ar))
		return 0;
	if (!plugin_register("projectconfig",&g_projectconfig))
		return 0;

	SWSRegisterCommands(g_commandTable);

	if (!plugin_register("hookcustommenu", (void*)menuhook))
		return 0;

	// Restore state
	char str[128];
	g_bACEnabled = GetPrivateProfileInt(SWS_INI, AC_ENABLE_KEY, 0, get_ini_file()) ? true : false;

	int iCount = GetPrivateProfileInt(SWS_INI, AC_COUNT_KEY, 0, get_ini_file());
	for (int i = 0; i < iCount; i++)
	{
		char key[32];
		_snprintf(key, 32, AC_ITEM_KEY, i+1);
		GetPrivateProfileString(SWS_INI, key, "\"\" 0", str, 128, get_ini_file());
		LineParser lp(false);
		if (!lp.parse(str))
			g_pACItems.Add(new SWS_AutoColorItem(lp.gettoken_str(0), lp.gettoken_int(1)));
	}	

	g_pACWnd = new SWS_AutoColorWnd();

	return 1;
}

void AutoColorExit()
{
	// Save state
	char str[128];
	sprintf(str, "%d", g_bACEnabled ? 1 : 0);
	WritePrivateProfileString(SWS_INI, AC_ENABLE_KEY, str, get_ini_file());
	sprintf(str, "%d", g_pACItems.GetSize());
	WritePrivateProfileString(SWS_INI, AC_COUNT_KEY, str, get_ini_file());
	for (int i = 0; i < g_pACItems.GetSize(); i++)
	{
		char key[32];
		_snprintf(key, 32, AC_ITEM_KEY, i+1);
		_snprintf(str, 128, "\"%s\" %d", g_pACItems.Get(i)->m_str.Get(), g_pACItems.Get(i)->m_col);
		WritePrivateProfileString(SWS_INI, key, str, get_ini_file());
	}	

	delete g_pACWnd;
}
