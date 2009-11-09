/******************************************************************************
/ Snapshots.cpp
/
/ Copyright (c) 2009 Tim Payne (SWS)
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
#include "../ObjectState/TrackFX.h"
#include "SnapshotClass.h"
#include "Snapshots.h"
#include "../Prompt.h"

#define SNAP_OPTIONS_KEY "Snapshot Options"
#define RENAME_MSG	0x10001
#define DELETE_MSG	0x10002
#define SAVE_MSG	0x10003
#define LOADSEL_MSG	0x10005
#define SEL_MSG		0x10006
#define ADDSEL_MSG	0x10007
#define DELSEL_MSG	0x10008
#define LOAD_MSG	0x100F0 // leave space!!

class ProjSnapshot
{
public:
	WDL_PtrList<Snapshot> m_snapshots;
	int m_iCurSnapshot;
	ProjSnapshot() : m_iCurSnapshot(-1) {}
	~ProjSnapshot() { m_snapshots.Empty(true); }
};

// Globals
static SWSProjConfig<ProjSnapshot> g_ss;
static SWS_SnapshotsWnd* g_pSSWnd;

void UpdateSnapshotsDialog()
{
	g_pSSWnd->Update();
}

static int g_iCustomMask = ALL_MASK;
static int g_iMaskOverride = 0;
static int g_iMask = ALL_MASK;
static bool g_bSelOnly = false;
static int g_iSavedMask;
static bool g_bSavedSelOnly;
static bool g_bApplyFilterOnRecall = true;
static bool g_bHideNewOnRecall = true;
static bool g_bPromptOnNew = false;
static bool g_bHideOptions = false;

Snapshot* GetSS(int slot)
{
	for (int i = 0; i < g_ss.Get()->m_snapshots.GetSize(); i++)
		if (g_ss.Get()->m_snapshots.Get(i)->m_iSlot == slot)
			return g_ss.Get()->m_snapshots.Get(i);
	return NULL;
}
 
static SWS_LVColumn g_cols[] = { { 20, 0, "#" }, { 60, 1, "Name" }, { 60, 0, "Date" }, { 60, 0, "Time" } };

SWS_SnapshotsView::SWS_SnapshotsView(HWND hwndList, HWND hwndEdit)
:SWS_ListView(hwndList, hwndEdit, 4, g_cols, "Snapshots View State", true)
{
}

int SWS_SnapshotsView::OnItemSort(LPARAM lParam1, LPARAM lParam2)
{
	Snapshot* item1 = (Snapshot*)lParam1;
	Snapshot* item2 = (Snapshot*)lParam2;
	int iRet = 0;

	switch (abs(m_iSortCol))
	{
	case 1: // #
		if (item1->m_iSlot > item2->m_iSlot)
			iRet = 1;
		else if (item1->m_iSlot < item2->m_iSlot)
			iRet = -1;
		break;
	case 2: // Name
		iRet = strcmp(item1->m_cName, item2->m_cName);
		break;
	case 3: // Time
	case 4:
		if (item1->m_time > item2->m_time)
			iRet = 1;
		else if (item1->m_time < item2->m_time)
			iRet = -1;
		break;
	}
	if (m_iSortCol < 0)
		return -iRet;
	else
		return iRet;
}

int SWS_SnapshotsView::OnNotify(WPARAM wParam, LPARAM lParam)
{
	// Tweak the clicked column
	NMLISTVIEW* s = (NMLISTVIEW*)lParam;
	if (s->hdr.code == LVN_COLUMNCLICK && s->iSubItem >= 3)
		s->iSubItem = 2;
	return SWS_ListView::OnNotify(wParam, lParam);
}

void SWS_SnapshotsView::SetItemText(LPARAM item, int iCol, const char* str)
{
	if (iCol == 1)
	{
		Snapshot* ss = (Snapshot*)item;
		ss->SetName(str);
	}
}

void SWS_SnapshotsView::GetItemText(LPARAM item, int iCol, char* str, int iStrMax)
{
	Snapshot* ss = (Snapshot*)item;
	switch(iCol)
	{
	case 0:
		_snprintf(str, iStrMax, "%d", ss->m_iSlot);
		break;
	case 1:
		if (ss->m_cName)
			lstrcpyn(str, ss->m_cName, iStrMax);
		else
			str[0] = 0;
		break;
#ifdef _WIN32
	case 2:
	case 3:
	{
		SYSTEMTIME st, st2;
		int t = ss->m_time;
		struct tm pt;
		int err = _gmtime32_s(&pt, (__time32_t*)&t);
		if (!err)
		{
			st.wMilliseconds = 0;
			st.wSecond = pt.tm_sec;
			st.wMinute = pt.tm_min;
			st.wHour   = pt.tm_hour;
			st.wDay    = pt.tm_mday;
			st.wMonth  = pt.tm_mon + 1;
			st.wYear   = pt.tm_year + 1900;
			st.wDayOfWeek = pt.tm_wday;
			SystemTimeToTzSpecificLocalTime(NULL, &st, &st2);		
			
			if (iCol == 2)
				GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st2, NULL, str, iStrMax);
			else if (iCol == 3)
				GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st2, NULL, str, iStrMax);
		}
		else
			str[0] = 0;
		break;
	}
#else
	case 2:
		GetDateString(ss->m_time, str, iStrMax);
		break;
	case 3:
		GetTimeString(ss->m_time, str, iStrMax);
		break;
#endif
	}
}

void SWS_SnapshotsView::GetItemTooltip(LPARAM item, char* str, int iStrMax)
{
	if (item)
	{
		Snapshot* ss = (Snapshot*)item;
		ss->Tooltip(str, iStrMax);
	}
}

void SWS_SnapshotsView::OnItemClk(LPARAM item, int iCol)
{
	if (!item)
	{
		g_ss.Get()->m_iCurSnapshot = -1;
		return;
	}

	Snapshot* ss = (Snapshot*)item;

	g_pSSWnd->m_pLastTouched = ss;
	bool bShift = GetAsyncKeyState(VK_SHIFT)   & 0x8000 ? true : false;
	bool bCtrl  = GetAsyncKeyState(VK_CONTROL) & 0x8000 ? true : false;
	bool bAlt   = GetAsyncKeyState(VK_MENU)    & 0x8000 ? true : false;

	// Recall (std click)
	if (!bShift && !bCtrl && !bAlt)
	{
		if (ss->UpdateReaper(g_bApplyFilterOnRecall ? g_iMask : ALL_MASK, g_bSelOnly, g_bHideNewOnRecall))
			Update();
	}
	// Save (ctrl click)
	else if (!bShift && bCtrl && !bAlt)
	{
		g_ss.Get()->m_snapshots.Set(g_ss.Get()->m_snapshots.Find(ss), new Snapshot(ss->m_iSlot, g_iMask, g_bSelOnly, ss->m_cName));
		delete ss;
		Update();
	}
	// Delete (alt click)
	else if (!bShift && !bCtrl && bAlt)
	{
		g_ss.Get()->m_snapshots.Delete(g_ss.Get()->m_snapshots.Find(ss), true);
		Update();
	}
}

int SWS_SnapshotsView::GetItemCount()
{
	return (LPARAM)g_ss.Get()->m_snapshots.GetSize();
}

LPARAM SWS_SnapshotsView::GetItemPointer(int iItem)
{
	return (LPARAM)g_ss.Get()->m_snapshots.Get(iItem);
}

bool SWS_SnapshotsView::GetItemState(LPARAM item)
{
	Snapshot* ss = (Snapshot*)item;
	return ss->m_iSlot == g_ss.Get()->m_iCurSnapshot;
}

bool SWS_SnapshotsView::OnItemSelChange(LPARAM item, bool bSel)
{
	if (bSel)
	{
		Snapshot* ss = (Snapshot*)item;
		g_ss.Get()->m_iCurSnapshot = ss->m_iSlot;
	}
	return false;
}

SWS_SnapshotsWnd::SWS_SnapshotsWnd()
:SWS_DockWnd(IDD_SNAPS, "Snapshots", 30002),m_pLastTouched(NULL),m_iSelType(0)
{
	// Restore state
	char str[32];
	GetPrivateProfileString(SWS_INI, SNAP_OPTIONS_KEY, "63 0 0 0 1 0 0", str, 32, get_ini_file());
	LineParser lp(false);
	if (!lp.parse(str))
	{
		g_iMask = lp.gettoken_int(0);
		g_bApplyFilterOnRecall = lp.gettoken_int(1) ? true : false;
		g_bHideOptions = lp.gettoken_int(2) ? true : false;
		g_bPromptOnNew = lp.gettoken_int(3) ? true : false;
		g_bHideNewOnRecall = lp.gettoken_int(4) ? true : false;
		g_bSelOnly = lp.gettoken_int(5) ? true : false;
		m_iSelType = lp.gettoken_int(6);
	}
	g_iSavedMask = g_iMask;
	g_bSavedSelOnly = g_bSelOnly;

	if (m_bShowAfterInit)
		Show(false, false);
}

void SWS_SnapshotsWnd::Update()
{
	static bool bRecurseCheck = false;
	if (!IsValidWindow() || bRecurseCheck || !m_pList || m_pList->UpdatesDisabled())
		return;
	bRecurseCheck = true;

	//Update the check boxes
	CheckDlgButton(m_hwnd, IDC_MIX,          m_iSelType == 0		? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_CURVIS,       m_iSelType == 1		? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_CUSTOM,       m_iSelType == 2		? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_VOL,          g_iMask & VOL_MASK     ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_PAN,          g_iMask & PAN_MASK     ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_MUTE,         g_iMask & MUTE_MASK    ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_SOLO,         g_iMask & SOLO_MASK    ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_FXCHAIN,      g_iMask & FXCHAIN_MASK ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_SENDS,        g_iMask & SENDS_MASK   ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_SELECTION,    g_iMask & SEL_MASK     ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_VISIBILITY,   g_iMask & VIS_MASK		? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_SELECTEDONLY, g_bSelOnly				? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_APPLYRECALL,  g_bApplyFilterOnRecall	? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_NAMEPROMPT,	 g_bPromptOnNew			? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_HIDENEW,		 g_bHideNewOnRecall		? BST_CHECKED : BST_UNCHECKED);

	EnableWindow(GetDlgItem(m_hwnd, IDC_VOL), m_iSelType == 2);
	EnableWindow(GetDlgItem(m_hwnd, IDC_PAN), m_iSelType == 2);
	EnableWindow(GetDlgItem(m_hwnd, IDC_MUTE), m_iSelType == 2);
	EnableWindow(GetDlgItem(m_hwnd, IDC_SOLO), m_iSelType == 2);
	EnableWindow(GetDlgItem(m_hwnd, IDC_FXCHAIN), m_iSelType == 2);
	EnableWindow(GetDlgItem(m_hwnd, IDC_SENDS), m_iSelType == 2);
	EnableWindow(GetDlgItem(m_hwnd, IDC_SELECTION), m_iSelType == 2);
	EnableWindow(GetDlgItem(m_hwnd, IDC_VISIBILITY), m_iSelType == 2);

	m_pList->Update();

	bRecurseCheck = false;
}

void SWS_SnapshotsWnd::RenameCurrent()
{
	Show(false, true);
	// Find the item in the list
	int i;
	for (i = 0; i < g_ss.Get()->m_snapshots.GetSize(); i++)
		if (g_ss.Get()->m_snapshots.Get(i)->m_iSlot == g_ss.Get()->m_iCurSnapshot)
			break;
	m_pList->EditListItem((LPARAM)g_ss.Get()->m_snapshots.Get(i), 1);
}

void SWS_SnapshotsWnd::OnInitDlg()
{
	m_resize.init_item(IDC_LIST, 0.0, 0.0, 1.0, 1.0);
	m_resize.init_item(IDC_FILTERGROUP, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_MIX, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_CURVIS, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_CUSTOM, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_SAVE, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_PAN, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_MUTE, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_SOLO, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_SELECTEDONLY, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_HELPTEXT, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_VOL, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_FXCHAIN, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_SENDS, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_VISIBILITY, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_SELECTION, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_APPLYRECALL, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_NAMEPROMPT, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_HIDENEW, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_OPTIONS, 1.0, 1.0, 1.0, 1.0);

	m_pList = new SWS_SnapshotsView(GetDlgItem(m_hwnd, IDC_LIST), GetDlgItem(m_hwnd, IDC_EDIT));

	Update();
}

void SWS_SnapshotsWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
		case LOADSEL_MSG:
			if (m_pLastTouched)
			{
				if (m_pLastTouched->UpdateReaper(g_bApplyFilterOnRecall ? g_iMask : ALL_MASK, g_bSelOnly, g_bHideNewOnRecall))
					Update();
			}
			break;
		case IDC_SAVE:
			NewSnapshot();
			break;
		case IDC_OPTIONS:
			g_bHideOptions = !g_bHideOptions;
			SendMessage(m_hwnd, WM_SIZE, 0, 0);
			break;
		case RENAME_MSG:
			if (m_pLastTouched)
				m_pList->EditListItem((LPARAM)m_pLastTouched, 1);
			break;
		case SAVE_MSG:
			if (m_pLastTouched)
			{
				g_ss.Get()->m_snapshots.Set(g_ss.Get()->m_snapshots.Find(m_pLastTouched), new Snapshot(m_pLastTouched->m_iSlot, g_iMask, g_bSelOnly, m_pLastTouched->m_cName));
				delete m_pLastTouched;
				Update();
			}
			break;
		case DELETE_MSG:
			if (m_pLastTouched)
			{
				g_ss.Get()->m_snapshots.Delete(g_ss.Get()->m_snapshots.Find(m_pLastTouched), true);
				g_ss.Get()->m_iCurSnapshot = -1;
				Update();
			}
			break;
		case SEL_MSG:
			if (m_pLastTouched)
				m_pLastTouched->SelectTracks();
			break;
		case ADDSEL_MSG:
			if (m_pLastTouched)
			{
				m_pLastTouched->AddSelTracks();
				Update();
			}
			break;
		case DELSEL_MSG:
			if (m_pLastTouched)
			{
				m_pLastTouched->DelSelTracks();
				Update();						
			}
			break;
		case BN_CLICKED << 16 | IDC_MIX:
		case BN_CLICKED << 16 | IDC_CURVIS:
		case BN_CLICKED << 16 | IDC_CUSTOM:
		case BN_CLICKED << 16 | IDC_VOL:
		case BN_CLICKED << 16 | IDC_PAN:
		case BN_CLICKED << 16 | IDC_MUTE:
		case BN_CLICKED << 16 | IDC_SOLO:
		case BN_CLICKED << 16 | IDC_SENDS:
		case BN_CLICKED << 16 | IDC_FXCHAIN:
		case BN_CLICKED << 16 | IDC_SELECTEDONLY:
		case BN_CLICKED << 16 | IDC_VISIBILITY:
		case BN_CLICKED << 16 | IDC_SELECTION:
		case BN_CLICKED << 16 | IDC_APPLYRECALL:
		case BN_CLICKED << 16 | IDC_NAMEPROMPT:
		case BN_CLICKED << 16 | IDC_HIDENEW:
			GetOptions();
			Update();
			break;
		default:
			if (wParam >= LOAD_MSG && wParam < (WPARAM)(LOAD_MSG + g_ss.Get()->m_snapshots.GetSize()))
				GetSnapshot((int)(wParam-LOAD_MSG), ALL_MASK, false);
			else
				Main_OnCommand((int)wParam, (int)lParam);
	}
}

HMENU SWS_SnapshotsWnd::OnContextMenu(int x, int y)
{
	HMENU contextMenu = CreatePopupMenu();
	LPARAM item = m_pList->GetHitItem(x, y, NULL);

	m_pLastTouched = (Snapshot*)item;

	if (item)
	{
		//g_ss.Get()->m_iCurSnapshot = ss->m_iSlot;
		AddToMenu(contextMenu, "Rename", RENAME_MSG);
		AddToMenu(contextMenu, SWS_SEPARATOR, 0);
		AddToMenu(contextMenu, "Select tracks in snapshot", SEL_MSG);
		AddToMenu(contextMenu, "Add selected track(s) to snapshot", ADDSEL_MSG);
		AddToMenu(contextMenu, "Delete selected track(s) from snapshot", DELSEL_MSG);
		AddToMenu(contextMenu, "Overwrite snapshot", SAVE_MSG);
		AddToMenu(contextMenu, "Delete snapshot", DELETE_MSG);
	}
	else
	{
		for (int i = 0; i < g_ss.Get()->m_snapshots.GetSize(); i++)
		{
			int iCmd = SWSGetCommandID(GetSnapshot, i+1);
			char cName[50];
			_snprintf(cName, 50, "Recall %s", g_ss.Get()->m_snapshots.Get(i)->m_cName);
			if (!iCmd)
				iCmd = LOAD_MSG + i;
			AddToMenu(contextMenu, cName, iCmd);
		}
	}
	AddToMenu(contextMenu, "New snapshot", SWSGetCommandID(NewSnapshot));

	return contextMenu;
}

void SWS_SnapshotsWnd::OnResize()
{
	if (!g_bHideOptions)
	{
		ShowControls(true);
		memcpy(&m_resize.get_item(IDC_LIST)->orig, &m_resize.get_item(IDC_LIST)->real_orig, sizeof(RECT));
		memcpy(&m_resize.get_item(IDC_OPTIONS)->orig, &m_resize.get_item(IDC_OPTIONS)->real_orig, sizeof(RECT));
		m_resize.get_item(IDC_OPTIONS)->scales[0] = 1.0;
		m_resize.get_item(IDC_OPTIONS)->scales[2] = 1.0;
		SetDlgItemText(m_hwnd, IDC_OPTIONS, "<- Hide Options");
	}
	else
	{
		ShowControls(false);
		RECT* r = &m_resize.get_item(IDC_LIST)->orig;
		r->bottom = m_resize.get_item(IDC_OPTIONS)->real_orig.top - r->top;  // use top and left as margins
		r->right  = m_resize.get_orig_rect().right - r->left;
		r = &m_resize.get_item(IDC_OPTIONS)->orig;
		r->left = m_resize.get_item(IDC_LIST)->orig.left;
		r->right = r->left + (m_resize.get_item(IDC_OPTIONS)->real_orig.right - m_resize.get_item(IDC_OPTIONS)->real_orig.left);
		m_resize.get_item(IDC_OPTIONS)->scales[0] = 0.0;
		m_resize.get_item(IDC_OPTIONS)->scales[2] = 0.0;
		SetDlgItemText(m_hwnd, IDC_OPTIONS, "Show Options ->");
	}
}

void SWS_SnapshotsWnd::OnDestroy()
{
	char str[256];

	// Save window state
	sprintf(str, "%d %d %d %d %d %d %d",
		g_iMask,
		g_bApplyFilterOnRecall ? 1 : 0,
		g_bHideOptions ? 1 : 0,
		g_bPromptOnNew ? 1 : 0,
		g_bHideNewOnRecall ? 1 : 0,
		g_bSelOnly ? 1 : 0,
		m_iSelType);
	WritePrivateProfileString(SWS_INI, SNAP_OPTIONS_KEY, str, get_ini_file());

	m_pList->OnDestroy();
}

void SWS_SnapshotsWnd::GetOptions()
{
	if (!IsValidWindow())
		return;

	if (IsDlgButtonChecked(m_hwnd, IDC_MIX) == BST_CHECKED)
	{
		m_iSelType = 0;
		g_iMask = VOL_MASK | PAN_MASK | MUTE_MASK | SOLO_MASK | FXCHAIN_MASK | SENDS_MASK;
	}
	else if (IsDlgButtonChecked(m_hwnd, IDC_CURVIS) == BST_CHECKED)
	{
		m_iSelType = 1;
		g_iMask = VIS_MASK;
	}
	else
	{
		g_iMask = 0;
		m_iSelType = 2;
		if (IsDlgButtonChecked(m_hwnd, IDC_VOL) == BST_CHECKED)
			g_iMask |= VOL_MASK;
		if (IsDlgButtonChecked(m_hwnd, IDC_PAN) == BST_CHECKED)
			g_iMask |= PAN_MASK;
		if (IsDlgButtonChecked(m_hwnd, IDC_MUTE) == BST_CHECKED)
			g_iMask |= MUTE_MASK;
		if (IsDlgButtonChecked(m_hwnd, IDC_SOLO) == BST_CHECKED)
			g_iMask |= SOLO_MASK;
		if (IsDlgButtonChecked(m_hwnd, IDC_FXCHAIN) == BST_CHECKED)
			g_iMask |= FXCHAIN_MASK;
		if (IsDlgButtonChecked(m_hwnd, IDC_SENDS) == BST_CHECKED)
			g_iMask |= SENDS_MASK;
		if (IsDlgButtonChecked(m_hwnd, IDC_VISIBILITY) == BST_CHECKED)
			g_iMask |= VIS_MASK;
		if (IsDlgButtonChecked(m_hwnd, IDC_SELECTION) == BST_CHECKED)
			g_iMask |= SEL_MASK;
	}
	g_bSelOnly = IsDlgButtonChecked(m_hwnd, IDC_SELECTEDONLY) == BST_CHECKED;
	g_bHideNewOnRecall = IsDlgButtonChecked(m_hwnd, IDC_HIDENEW) == BST_CHECKED;
	g_bApplyFilterOnRecall = IsDlgButtonChecked(m_hwnd, IDC_APPLYRECALL) == BST_CHECKED;
	g_bPromptOnNew = IsDlgButtonChecked(m_hwnd, IDC_NAMEPROMPT) == BST_CHECKED;
}

void SWS_SnapshotsWnd::ShowControls(bool bShow)
{
	ShowWindow(GetDlgItem(m_hwnd, IDC_MIX), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_CURVIS), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_CUSTOM), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_FILTERGROUP), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_SAVE), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_PAN), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_MUTE), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_SOLO), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_SELECTEDONLY), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_HELPTEXT), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_VOL), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_FXCHAIN), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_SENDS), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_VISIBILITY), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_SELECTION), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_APPLYRECALL), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_NAMEPROMPT), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_HIDENEW), bShow);
}

Snapshot* GetSnapshotPtr(int i)
{
	return g_ss.Get()->m_snapshots.Get(i);
}

void OpenSnapshotsDialog(COMMAND_T*)
{
	g_pSSWnd->Show(true, true);
}

void NewSnapshot(int iMask, bool bSelOnly)
{
	// Find the first free "slot"
	int i;
	for (i = 0; i < g_ss.Get()->m_snapshots.GetSize(); i++)
		if (g_ss.Get()->m_snapshots.Get(i)->m_iSlot != i+1)
			break;

	g_ss.Get()->m_iCurSnapshot = i+1;
	Snapshot* pNewSS = g_ss.Get()->m_snapshots.Insert(i, new Snapshot(i+1, iMask, bSelOnly, NULL));
	if (g_bPromptOnNew)
	{
		char cName[256];
		strncpy(cName, pNewSS->m_cName, 256);
		if (PromptUserForString("Enter Snapshot Name", cName, 256) && strlen(cName))
			pNewSS->SetName(cName);
	}
	g_pSSWnd->Update();
}

void NewSnapshotEdit(COMMAND_T* = NULL)
{
	NewSnapshot(g_iMask, g_bSelOnly);
	g_pSSWnd->RenameCurrent();
}

void NewSnapshot(COMMAND_T*)
{
	NewSnapshot(g_iMask, g_bSelOnly);
}

void SaveSnapshot(int slot)
{
	if (slot == -1)
		return;

	// Find the slot (if it exists)
	int i;
	for (i = 0; i < g_ss.Get()->m_snapshots.GetSize(); i++)
		if (g_ss.Get()->m_snapshots.Get(i)->m_iSlot == slot)
			break;

	// Slot is "new"
	if (i >= g_ss.Get()->m_snapshots.GetSize())
	{
		// Find where in the list to put it to keep sorted
		for (i = 0; i < g_ss.Get()->m_snapshots.GetSize(); i++)
			if (g_ss.Get()->m_snapshots.Get(i)->m_iSlot > slot)
				break;
		char str[20];
		sprintf(str, "Mix %d", slot);
		g_ss.Get()->m_snapshots.Insert(i, new Snapshot(slot, g_iMask, g_bSelOnly, str));
	}
	else // Overwriting slot
	{
		Snapshot* oldSnapshot = g_ss.Get()->m_snapshots.Get(i);
		g_ss.Get()->m_snapshots.Set(i, new Snapshot(oldSnapshot->m_iSlot, g_iMask, g_bSelOnly, oldSnapshot->m_cName));
		delete oldSnapshot;
	}
	g_pSSWnd->Update();
}

void GetSnapshot(int slot, int iMask, bool bSelOnly)
{
	if (slot == -1)
		return;

	// Find the slot
	for (int i = 0; i < g_ss.Get()->m_snapshots.GetSize(); i++)
		if (g_ss.Get()->m_snapshots.Get(i)->m_iSlot == slot)
		{
			g_ss.Get()->m_iCurSnapshot = g_ss.Get()->m_snapshots.Get(i)->m_iSlot;
			g_ss.Get()->m_snapshots.Get(i)->UpdateReaper(iMask, bSelOnly, g_bHideNewOnRecall);
			g_pSSWnd->Update();
			return;
		}
}

void AddSnapshotTracks(COMMAND_T*)
{
	if (g_ss.Get()->m_iCurSnapshot != -1)
	{
		GetSS(g_ss.Get()->m_iCurSnapshot)->AddSelTracks();
		g_pSSWnd->Update();
	}
}

void AddTracks(COMMAND_T*)
{
	for (int i = 0; i < g_ss.Get()->m_snapshots.GetSize(); i++)
		g_ss.Get()->m_snapshots.Get(i)->AddSelTracks();
	g_pSSWnd->Update();
}

void DelSnapshotTracks(COMMAND_T*)
{
	if (g_ss.Get()->m_iCurSnapshot != -1)
	{
		GetSS(g_ss.Get()->m_iCurSnapshot)->DelSelTracks();
		g_pSSWnd->Update();
	}
}

void DelTracks(COMMAND_T*)
{
	for (int i = 0; i < g_ss.Get()->m_snapshots.GetSize(); i++)
		g_ss.Get()->m_snapshots.Get(i)->DelSelTracks();
	g_pSSWnd->Update();
}

void SelSnapshotTracks(COMMAND_T*)
{
	if (g_ss.Get()->m_iCurSnapshot != -1)
		GetSS(g_ss.Get()->m_iCurSnapshot)->SelectTracks();
}

void SaveCurSnapshot(COMMAND_T*){ SaveSnapshot(g_ss.Get()->m_iCurSnapshot); }
void SaveSnapshot(COMMAND_T* ct){ SaveSnapshot((int)ct->user); }
void GetCurSnapshot(COMMAND_T*)	{ GetSnapshot(g_ss.Get()->m_iCurSnapshot, ALL_MASK, false); }
void GetSnapshot(COMMAND_T* ct)	{ GetSnapshot((int)ct->user, ALL_MASK, false); }
void ToggleMute(COMMAND_T*)		{ g_iMask ^= MUTE_MASK;     UpdateSnapshotsDialog(); }
void ToggleSolo(COMMAND_T*)		{ g_iMask ^= SOLO_MASK;     UpdateSnapshotsDialog(); }
void TogglePan(COMMAND_T*)		{ g_iMask ^= PAN_MASK;      UpdateSnapshotsDialog(); }
void ToggleVol(COMMAND_T*)		{ g_iMask ^= VOL_MASK;      UpdateSnapshotsDialog(); }
void ToggleSend(COMMAND_T*)		{ g_iMask ^= SENDS_MASK;    UpdateSnapshotsDialog(); }
void ToggleFx(COMMAND_T*)		{ g_iMask ^= FXATM_MASK;    UpdateSnapshotsDialog(); }
void ToggleVis(COMMAND_T*)		{ g_iMask ^= VIS_MASK;      UpdateSnapshotsDialog(); }
void ToggleSel(COMMAND_T*)		{ g_iMask ^= SEL_MASK;      UpdateSnapshotsDialog(); }
void ToggleSelOnly(COMMAND_T*)	{ g_bSelOnly = !g_bSelOnly; UpdateSnapshotsDialog(); }
//void ToggleAppToRec(COMMAND_T*)	{ g_bApplyFilterOnRecall = !g_bApplyFilterOnRecall; UpdateSnapshotsDialog(); }
void ClearFilter(COMMAND_T*)	{ g_iMask = 0; g_bSelOnly = false; UpdateSnapshotsDialog(); }
void SaveFilter(COMMAND_T*)		{ g_iSavedMask = g_iMask; g_bSavedSelOnly = g_bSelOnly; }
void RestoreFilter(COMMAND_T*)	{ g_iMask = g_iSavedMask; g_bSelOnly = g_bSavedSelOnly; UpdateSnapshotsDialog(); }

static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "SWS: Open mix snapshots" },							"SWSSNAPSHOT_OPEN",	     OpenSnapshotsDialog,  NULL, },
	{ { DEFACCEL, "SWS: Add selected track(s) to current snapshot" },	"SWSSNAPSHOT_ADD",	     AddSnapshotTracks,	   "Add selected track(s) to current snapshot", },
	{ { DEFACCEL, "SWS: Add selected track(s) to all snapshots" },		"SWSSNAPSHOTS_ADD",	     AddTracks,			   "Add selected track(s) to all snapshots", },
	{ { DEFACCEL, "SWS: Delete selected track(s) from current snapshot" },"SWSSNAPSHOT_DEL",	     DelSnapshotTracks,	   "Delete selected track(s) from current snapshot", },
	{ { DEFACCEL, "SWS: Delete selected track(s) from all snapshots" },	"SWSSNAPSHOTS_DEL",	     DelTracks,			   "Delete selected track(s) from all snapshots", },
	{ { DEFACCEL, "SWS: New snapshot" },									"SWSSNAPSHOT_NEW",	     NewSnapshot,		   "New Snapshot", },
	{ { DEFACCEL, "SWS: Select current snapshot track(s)" },				"SWSSNAPSHOT_SEL",	     SelSnapshotTracks,	   "Select current snapshot's track(s)", },
	{ { DEFACCEL, "SWS: Save over current snapshot" },					"SWSSNAPSHOT_SAVE",	     SaveCurSnapshot,	   "Save over current snapshot", },
	{ { DEFACCEL, "SWS: Recall current snapshot" },						"SWSSNAPSHOT_GET",	     GetCurSnapshot,       "Recall current snapshot", },
	{ { DEFACCEL, "SWS: New snapshot and edit name" },					"SWSSNAPSHOT_NEWEDIT",   NewSnapshotEdit,	   NULL, },
	{ { DEFACCEL, "SWS: Save as snapshot 1" },							"SWSSNAPSHOT_SAVE1",	 SaveSnapshot,         NULL, 1 },
	{ { DEFACCEL, "SWS: Save as snapshot 2" },							"SWSSNAPSHOT_SAVE2",	 SaveSnapshot,         NULL, 2 },
	{ { DEFACCEL, "SWS: Save as snapshot 3" },							"SWSSNAPSHOT_SAVE3",	 SaveSnapshot,         NULL, 3 },
	{ { DEFACCEL, "SWS: Save as snapshot 4" },							"SWSSNAPSHOT_SAVE4",	 SaveSnapshot,         NULL, 4 },
	{ { DEFACCEL, "SWS: Save as snapshot 5" },							"SWSSNAPSHOT_SAVE5",	 SaveSnapshot,         NULL, 5 },
	{ { DEFACCEL, "SWS: Save as snapshot 6" },							"SWSSNAPSHOT_SAVE6",	 SaveSnapshot,         NULL, 6 },
	{ { DEFACCEL, "SWS: Save as snapshot 7" },							"SWSSNAPSHOT_SAVE7",	 SaveSnapshot,         NULL, 7 },
	{ { DEFACCEL, "SWS: Save as snapshot 8" },							"SWSSNAPSHOT_SAVE8",	 SaveSnapshot,         NULL, 8 },
	{ { DEFACCEL, "SWS: Save as snapshot 9" },							"SWSSNAPSHOT_SAVE9",	 SaveSnapshot,         NULL, 9 },
	{ { DEFACCEL, "SWS: Save as snapshot 10" },							"SWSSNAPSHOT_SAVE10",	 SaveSnapshot,         NULL, 10 },
	{ { DEFACCEL, "SWS: Save as snapshot 11" },							"SWSSNAPSHOT_SAVE11",	 SaveSnapshot,         NULL, 11 },
	{ { DEFACCEL, "SWS: Save as snapshot 12" },							"SWSSNAPSHOT_SAVE12",	 SaveSnapshot,         NULL, 12 },
	{ { DEFACCEL, "SWS: Recall snapshot 1" },							"SWSSNAPSHOT_GET1",		 GetSnapshot,          NULL, 1 },
	{ { DEFACCEL, "SWS: Recall snapshot 2" },							"SWSSNAPSHOT_GET2",		 GetSnapshot,          NULL, 2 },
	{ { DEFACCEL, "SWS: Recall snapshot 3" },							"SWSSNAPSHOT_GET3",		 GetSnapshot,          NULL, 3 },
	{ { DEFACCEL, "SWS: Recall snapshot 4" },							"SWSSNAPSHOT_GET4",		 GetSnapshot,          NULL, 4 },
	{ { DEFACCEL, "SWS: Recall snapshot 5" },							"SWSSNAPSHOT_GET5",		 GetSnapshot,          NULL, 5 },
	{ { DEFACCEL, "SWS: Recall snapshot 6" },							"SWSSNAPSHOT_GET6",		 GetSnapshot,          NULL, 6 },
	{ { DEFACCEL, "SWS: Recall snapshot 7" },							"SWSSNAPSHOT_GET7",		 GetSnapshot,          NULL, 7 },
	{ { DEFACCEL, "SWS: Recall snapshot 8" },							"SWSSNAPSHOT_GET8",		 GetSnapshot,          NULL, 8 },
	{ { DEFACCEL, "SWS: Recall snapshot 9" },							"SWSSNAPSHOT_GET9",		 GetSnapshot,          NULL, 9 },
	{ { DEFACCEL, "SWS: Recall snapshot 10" },							"SWSSNAPSHOT_GET10",	 GetSnapshot,          NULL, 10 },
	{ { DEFACCEL, "SWS: Recall snapshot 11" },							"SWSSNAPSHOT_GET11",	 GetSnapshot,          NULL, 11 },
	{ { DEFACCEL, "SWS: Recall snapshot 12" },							"SWSSNAPSHOT_GET12",	 GetSnapshot,          NULL, 12 },
	{ { DEFACCEL, "SWS: Toggle snapshot mute" },							"SWSSNAPSHOT_MUTE",		 ToggleMute,           NULL, },
	{ { DEFACCEL, "SWS: Toggle snapshot solo" },							"SWSSNAPSHOT_SOLO",		 ToggleSolo,           NULL, },
	{ { DEFACCEL, "SWS: Toggle snapshot pan" },							"SWSSNAPSHOT_PAN",		 TogglePan,            NULL, },
	{ { DEFACCEL, "SWS: Toggle snapshot vol" },							"SWSSNAPSHOT_VOL",		 ToggleVol,            NULL, },
	{ { DEFACCEL, "SWS: Toggle snapshot sends" },						"SWSSNAPSHOT_SEND",		 ToggleSend,           NULL, },
	{ { DEFACCEL, "SWS: Toggle snapshot fx" },							"SWSSNAPSHOT_FX",		 ToggleFx,             NULL, },
	{ { DEFACCEL, "SWS: Toggle snapshot visibility" },					"SWSSNAPSHOT_VIS",		 ToggleVis,            NULL, },
	{ { DEFACCEL, "SWS: Toggle snapshot selection" },					"SWSSNAPSHOT_TOGSEL",    ToggleSel,            NULL, },
	{ { DEFACCEL, "SWS: Toggle snapshot selected only" },				"SWSSNAPSHOT_SELONLY",	 ToggleSelOnly,        NULL, },
//	{ { DEFACCEL, "SWS: Toggle snapshot apply filter to recall" },       "SWSSNAPSHOT_APPLYLOAD", ToggleAppToRec,       NULL, },
	{ { DEFACCEL, "SWS: Clear all snapshot filter options" },            "SWSSNAPSHOT_CLEARFILT", ClearFilter,          NULL, },
	{ { DEFACCEL, "SWS: Save current snapshot filter options" },         "SWSSNAPSHOT_SAVEFILT",  SaveFilter,           NULL, },
	{ { DEFACCEL, "SWS: Restore snapshot filter options" },              "SWSSNAPSHOT_RESTFILT",  RestoreFilter,        NULL, },
	{ {}, LAST_COMMAND, }, // Denote end of table
};

static bool ProcessExtensionLine(const char *line, ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	LineParser lp(false);
	if (lp.parse(line) || lp.getnumtokens() < 1)
		return false;
	if (strcmp(lp.gettoken_str(0), "<SWSSNAPSHOT") != 0)
		return false; // only look for <SWSSNAPSHOT lines

	int time = 0;
	if (lp.getnumtokens() == 6) // Old-style time, convert
	{
#ifdef _WIN32
		FILETIME ft;
		SYSTEMTIME st;
		ft.dwHighDateTime = strtoul(lp.gettoken_str(4), NULL, 10);
		ft.dwLowDateTime  = strtoul(lp.gettoken_str(5), NULL, 10);
		FileTimeToSystemTime(&ft, &st);
		struct tm pt;
		pt.tm_sec  = st.wSecond;
		pt.tm_min  = st.wMinute;
		pt.tm_hour = st.wHour;
		pt.tm_mday = st.wDay;
		pt.tm_mon  = st.wMonth - 1;
		pt.tm_year = st.wYear - 1900;
		pt.tm_isdst = -1;
		time = (int)mktime(&pt);
#endif
	}
	else
		time = lp.gettoken_int(4);

	Snapshot* s = g_ss.Get()->m_snapshots.Add(new Snapshot(lp.gettoken_int(2), lp.gettoken_int(3), lp.gettoken_str(1), time));

	char linebuf[4096];
	while(true)
	{
		if (!ctx->GetLine(linebuf,sizeof(linebuf)) && !lp.parse(linebuf))
		{
			if (lp.gettoken_str(0)[0] == '>')
				break;
			if (strcmp("TRACK", lp.gettoken_str(0)) == 0)
				s->m_tracks.Add(new TrackSnapshot(&lp));
			else if (s->m_tracks.GetSize())
			{
				if (strcmp("SEND", lp.gettoken_str(0)) == 0)
					s->m_tracks.Get(s->m_tracks.GetSize()-1)->m_sends.Add(new SendSnapshot(&lp));
				else if (strcmp("FX", lp.gettoken_str(0)) == 0) // "One liner"
					s->m_tracks.Get(s->m_tracks.GetSize()-1)->m_fx.Add(new FXSnapshot(&lp));
				else if (strcmp("<FX", lp.gettoken_str(0)) == 0) // Multiple lines
				{
					FXSnapshot* fx = s->m_tracks.Get(s->m_tracks.GetSize()-1)->m_fx.Add(new FXSnapshot(&lp));
					while(true)
						if (!ctx->GetLine(linebuf,sizeof(linebuf)) && !lp.parse(linebuf))
						{
							if (lp.gettoken_str(0)[0] == '>')
								break;
							fx->RestoreParams(lp.gettoken_str(0));
						}
						else
							break;
				}
				else if (strcmp("<FXCHAIN", lp.gettoken_str(0)) == 0) // Multiple lines
				{
					TrackSnapshot* ts = s->m_tracks.Get(s->m_tracks.GetSize()-1);
					ts->m_sFXChain.Set("");
					ts->m_sFXChain.Append(linebuf);
					int iDepth = 1;
					while(iDepth)
					{
						if (!ctx->GetLine(linebuf,sizeof(linebuf)) && !lp.parse(linebuf))
						{
							if (lp.gettoken_str(0)[0] == '>')
								iDepth--;
							else if (lp.gettoken_str(0)[0] == '<')
								iDepth++;
							ts->m_sFXChain.Append("\n");
							ts->m_sFXChain.Append(linebuf);
						}
						else
							break;
					}
				}
			}
		}
		else
			break;
	}
	g_pSSWnd->Update();
	return true;
}

static void SaveExtensionConfig(ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	char str[4096];

	for (int i = 0; i < g_ss.Get()->m_snapshots.GetSize(); i++)
	{
		Snapshot* ss = g_ss.Get()->m_snapshots.Get(i);
		ctx->AddLine("<SWSSNAPSHOT \"%s\" %d %d %d", ss->m_cName, ss->m_iSlot, ss->m_iMask, ss->m_time); 
		for (int j = 0; j < ss->m_tracks.GetSize(); j++)
		{
			TrackSnapshot* ts = ss->m_tracks.Get(j);
			ctx->AddLine(ts->ItemString(str, 4096));
			for (int k = 0; k < ts->m_sends.GetSize(); k++)
				ctx->AddLine(ts->m_sends.Get(k)->ItemString(str, 4096));
			for (int k = 0; k < ts->m_fx.GetSize(); k++)
			{
				bool bDone;
				FXSnapshot* fx = ts->m_fx.Get(k);
				do
					ctx->AddLine(fx->ItemString(str, 4096, &bDone));
				while (!bDone);
			}
			if (ts->m_sFXChain.GetLength())
			{
				const char* cFXString = ts->m_sFXChain.Get();
				const char* cNewLine = strchr(cFXString, '\n');
				while (cNewLine)
				{
					int iLineLen = cNewLine - cFXString;
					strncpy(str, cFXString, iLineLen);
					str[iLineLen] = 0;
					ctx->AddLine(str);
					cFXString = cNewLine+1;
					cNewLine = strchr(cFXString, '\n');
				}
				ctx->AddLine(cFXString); // Add the remainder (this will always be a >)
			}
		}
		ctx->AddLine(">");
	}
}

static void BeginLoadProjectState(bool isUndo, struct project_config_extension_t *reg)
{
	g_ss.Get()->m_snapshots.Empty(true);
	g_ss.Get()->m_iCurSnapshot = -1;
	g_ss.Cleanup();
	UpdateSnapshotsDialog();
}

static project_config_extension_t g_projectconfig = { ProcessExtensionLine, SaveExtensionConfig, BeginLoadProjectState, NULL };

// Seems damned "hacky" to me, but it works, so be it.
static int translateAccel(MSG *msg, accelerator_register_t *ctx)
{
	if (g_pSSWnd->IsActive())
	{
		HWND hwnd = g_pSSWnd->GetHWND();

		if (msg->message == WM_KEYDOWN && msg->wParam == VK_RETURN)
		{
			SendMessage(hwnd, WM_COMMAND, LOADSEL_MSG, 0);
			return 1;
		}
		else if (msg->message == WM_KEYDOWN && msg->wParam == VK_DELETE)
		{
			SendMessage(hwnd, WM_COMMAND, DELETE_MSG, 0);
			return 1;
		}
		else if (msg->message == WM_KEYDOWN && msg->wParam == VK_F2)
		{
			SendMessage(hwnd, WM_COMMAND, RENAME_MSG, 0);
			return 1;
		}
		else if (msg->message == WM_KEYDOWN && (msg->wParam == VK_TAB || msg->wParam == VK_ESCAPE || msg->wParam == VK_DOWN || msg->wParam == VK_UP))
			return -1;
		else
			return -666;
	}
	return 0;
} 

static accelerator_register_t g_ar = { translateAccel, TRUE, NULL };

static void menuhook(const char* menustr, HMENU hMenu, int flag)
{
	if (strcmp(menustr, "Main view") == 0 && flag == 0)
		AddToMenu(hMenu, "SWS Snapshots", g_commandTable[0].accel.accel.cmd, 40075);
	else if (strcmp(menustr, "Track control panel context") == 0 && flag == 0)
		AddSubMenu(hMenu, SWSCreateMenu(g_commandTable), "SWS Snapshots");
	else if (flag == 1)
		SWSCheckMenuItem(hMenu, g_commandTable[0].accel.accel.cmd, g_pSSWnd->IsValidWindow());
}

static void oldmenuhook(int menuid, HMENU hmenu, int flag)
{
	switch (menuid)
	{
	case MAINMENU_VIEW:
		menuhook("Main view", hmenu, flag);
		break;
	case CTXMENU_TCP:
		menuhook("Track control panel context", hmenu, flag);
		break;
	default:
		menuhook("", hmenu, flag);
		break;
	}
}

int SnapshotsInit()
{
	if (!plugin_register("projectconfig",&g_projectconfig))
		return 0;
	if (!plugin_register("accelerator",&g_ar))
		return 0;

	SWSRegisterCommands(g_commandTable);

	if (!plugin_register("hookcustommenu", (void*)menuhook))
		if (!plugin_register("hookmenu", (void*)oldmenuhook))
			return 0;

	g_pSSWnd = new SWS_SnapshotsWnd;

	return 1;
}

void SnapshotsExit()
{
	delete g_pSSWnd;
}
