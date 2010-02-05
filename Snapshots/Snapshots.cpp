/******************************************************************************
/ Snapshots.cpp
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
#include "SnapshotClass.h"
#include "Snapshots.h"
#include "SnapshotMerge.h"
#include "../Prompt.h"

#define SNAP_OPTIONS_KEY "Snapshot Options"
#define RENAME_MSG	0x10001
#define DELETE_MSG	0x10002
#define SAVE_MSG	0x10003
#define COPY_MSG	0x10004
#define LOADSEL_MSG	0x10005
#define SEL_MSG		0x10006
#define ADDSEL_MSG	0x10007
#define DELSEL_MSG	0x10008
#define DETAILS_MSG 0x10009
#define MERGE_MSG	0x1000A
#define EXPORT_MSG	0x1000B
#define IMPORT_MSG	0x1000C
#define LOAD_MSG	0x100F0 // leave space!!

class ProjSnapshot
{
public:
	WDL_PtrList<Snapshot> m_snapshots;
	Snapshot* m_pCurSnapshot;
	ProjSnapshot() : m_pCurSnapshot(NULL) {}
	~ProjSnapshot() { m_snapshots.Empty(true); }
};

// Globals
static SWSProjConfig<ProjSnapshot> g_ss;
static SWS_SnapshotsWnd* g_pSSWnd;
void PasteSnapshot(COMMAND_T*);
void MergeSnapshot(Snapshot* ss);

void UpdateSnapshotsDialog()
{
	g_pSSWnd->Update();
}

static int g_iMask = ALL_MASK;
static bool g_bSelOnly = false;
static int g_iSavedMask;
static int g_iSavedType;
static bool g_bSavedSelOnly;
static bool g_bApplyFilterOnRecall = true;
static bool g_bHideNewOnRecall = true;
static bool g_bPromptOnNew = false;
static bool g_bHideOptions = false;

#ifndef _WIN32
static int g_iClipFormat;
#endif

// Clipboard operations:
void CopySnapshotToClipboard(Snapshot* ss)
{
	WDL_String ssStr;
	ss->GetChunk(&ssStr);
	if (OpenClipboard(g_hwndParent))
	{
		EmptyClipboard();
		HGLOBAL hglbCopy; 
		hglbCopy = GlobalAlloc(GMEM_MOVEABLE, ssStr.GetLength() + 1); 
		if (hglbCopy)
		{
			memcpy(GlobalLock(hglbCopy), ssStr.Get(), ssStr.GetLength() + 1);	
			GlobalUnlock(hglbCopy);
#ifndef _WIN32
			g_iClipFormat = RegisterClipboardFormat("Text");
			SetClipboardData(g_iClipFormat, hglbCopy); 
#else
			SetClipboardData(CF_TEXT, hglbCopy); 
#endif
		}
		CloseClipboard();
	}
}

Snapshot* GetSnapshotFromClipboard()
{
	Snapshot* ss = NULL;
	if (OpenClipboard(g_hwndParent))
	{
#ifndef _WIN32
		HGLOBAL clipBoard = GetClipboardData(g_iClipFormat);
#else
		HGLOBAL clipBoard = GetClipboardData(CF_TEXT);
#endif
		if (clipBoard)
		{
			char* clipData = (char*)GlobalLock(clipBoard);
			if (clipData)
			{
				ss = new Snapshot(clipData);
				GlobalUnlock(clipBoard);
				if (!ss->m_tracks.GetSize())
				{
					MessageBox(g_hwndParent, "Clipboard does not contain a valid snapshot.", "SWS Snapshot Paste Error", MB_OK);
					delete ss;
					ss = NULL;
				}
			}
		}
		CloseClipboard();
	}
	return ss;
}

// File operations
void ExportSnapshot(Snapshot* ss)
{
	char filename[256];
	char cPath[256];
	GetProjectPath(cPath, 256);
	lstrcpyn(filename, ss->m_cName, 256);
	if (BrowseForSaveFile("Export snapshot...", cPath, filename, "SWSSnap files\0*.SWSSnap\0", filename, 256))
	{
		FILE* f = fopen(filename, "w");
		if (f)
		{
			WDL_String chunk;
			ss->GetChunk(&chunk);
			fwrite(chunk.Get(), chunk.GetLength()+1, 1, f);
			fclose(f);
		}
		else
			MessageBox(g_hwndParent, "Unable to write to file.", "SWS Snaphot Export Error", MB_OK);
	}

}

void ImportSnapshot()
{
	char str[4096];
	GetProjectPath(str, 256);
	char* cFile = BrowseForFiles("Import snapshot...", str, NULL, false, "SWSSnap files\0*.SWSSnap\0");
	if (cFile)
	{
		FILE* f = fopen(cFile, "r");
		WDL_String chunk;
		if (f)
		{
			while(fgets(str, 4096, f))
				chunk.Append(str);
			fclose(f);

			Snapshot* ss = new Snapshot(chunk.Get());
			if (!ss->m_tracks.GetSize())
			{
				MessageBox(g_hwndParent, "File does not contain a valid snapshot.", "SWS Snapshot Import Error", MB_OK);
				delete ss;
				ss = NULL;
			}
			else
				MergeSnapshot(ss); // Handles delete of ss if necessary			
		}
		else
			MessageBox(g_hwndParent, "Unable to open file.", "SWS Snaphot Import Error", MB_OK);
		
		free(cFile);
	}
}

static SWS_LVColumn g_cols[] = { { 20, 2, "#" }, { 60, 3, "Name" }, { 60, 2, "Date" }, { 60, 2, "Time" } };

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
	case 2:
		ss->GetTimeString(str, iStrMax, true);
		break;
	case 3:
		ss->GetTimeString(str, iStrMax, false);
		break;
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

void SWS_SnapshotsView::OnItemClk(LPARAM item, int iCol, int iKeyState)
{
	if (!item)
		return;

	Snapshot* ss = (Snapshot*)item;

	// Recall (std click)
	if (!(iKeyState & LVKF_SHIFT) && !(iKeyState & LVKF_CONTROL) && !(iKeyState & LVKF_ALT))
	{
		g_ss.Get()->m_pCurSnapshot = ss;
		if (ss->UpdateReaper(g_bApplyFilterOnRecall ? g_iMask : ALL_MASK, g_bSelOnly, g_bHideNewOnRecall))
			Update();
	}
	// Save (ctrl click)
	if (!(iKeyState & LVKF_SHIFT) && (iKeyState & LVKF_CONTROL) && !(iKeyState & LVKF_ALT))
	{
		g_ss.Get()->m_pCurSnapshot = g_ss.Get()->m_snapshots.Set(g_ss.Get()->m_snapshots.Find(ss), new Snapshot(ss->m_iSlot, g_iMask, g_bSelOnly, ss->m_cName));
		delete ss;
		Update();
	}
	// Delete (alt click)
	else if (!(iKeyState & LVKF_SHIFT) && !(iKeyState & LVKF_CONTROL) && (iKeyState & LVKF_ALT))
	{
		if (g_ss.Get()->m_pCurSnapshot == ss)
			g_ss.Get()->m_pCurSnapshot = NULL;

		int iSlot = ss->m_iSlot;
		g_ss.Get()->m_snapshots.Delete(g_ss.Get()->m_snapshots.Find(ss), true);
		char undoStr[128];
		sprintf(undoStr, "Delete snapshot %d", iSlot);
		Undo_OnStateChangeEx(undoStr, UNDO_STATE_MISCCFG, -1);
		Update();
	}
}

void SWS_SnapshotsView::GetItemList(WDL_TypedBuf<LPARAM>* pBuf)
{
	pBuf->Resize(g_ss.Get()->m_snapshots.GetSize());
	for (int i = 0; i < pBuf->GetSize(); i++)
		pBuf->Get()[i] = (LPARAM)g_ss.Get()->m_snapshots.Get(i);
}

int SWS_SnapshotsView::GetItemState(LPARAM item)
{
	Snapshot* ss = (Snapshot*)item;
	return ss == g_ss.Get()->m_pCurSnapshot ? LVIS_SELECTED | LVIS_FOCUSED : 0;
}

SWS_SnapshotsWnd::SWS_SnapshotsWnd()
:SWS_DockWnd(IDD_SNAPS, "Snapshots", 30002),m_iSelType(0)
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
	g_iSavedType = m_iSelType;
	g_bSavedSelOnly = g_bSelOnly;

	if (m_bShowAfterInit)
		Show(false, false);
}

void SWS_SnapshotsWnd::Update()
{
	static bool bRecurseCheck = false;
	if (!IsValidWindow() || bRecurseCheck || !m_pLists.GetSize() || m_pLists.Get(0)->UpdatesDisabled())
		return;
	bRecurseCheck = true;

	//Update the check boxes
	CheckDlgButton(m_hwnd, IDC_MIX,          m_iSelType == 0		? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_CURVIS,       m_iSelType == 1		? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_CUSTOM,       m_iSelType == 2		? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_SELECTEDONLY, g_bSelOnly				? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_APPLYRECALL,  g_bApplyFilterOnRecall	? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_NAMEPROMPT,	 g_bPromptOnNew			? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_HIDENEW,		 g_bHideNewOnRecall		? BST_CHECKED : BST_UNCHECKED);
	for (int i = 0; i < MASK_CTRLS; i++)
	{
		CheckDlgButton(m_hwnd, cSSCtrls[i], g_iMask & cSSMasks[i] ? BST_CHECKED : BST_UNCHECKED);
		EnableWindow(GetDlgItem(m_hwnd,  cSSCtrls[i]), m_iSelType == 2);
	}

	m_pLists.Get(0)->Update();

	bRecurseCheck = false;
}

void SWS_SnapshotsWnd::RenameCurrent()
{
	Show(false, true);
	// Find the item in the list
	int i;
	for (i = 0; i < g_ss.Get()->m_snapshots.GetSize(); i++)
		if (g_ss.Get()->m_snapshots.Get(i) == g_ss.Get()->m_pCurSnapshot)
			break;
	m_pLists.Get(0)->EditListItem((LPARAM)g_ss.Get()->m_snapshots.Get(i), 1);
}

void SWS_SnapshotsWnd::OnInitDlg()
{
	m_resize.init_item(IDC_LIST, 0.0, 0.0, 1.0, 1.0);
	m_resize.init_item(IDC_FILTERGROUP, 1.0, 0.0, 1.0, 0.0);
	for (int i = 0; i < MASK_CTRLS; i++)
		m_resize.init_item(cSSCtrls[i], 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_MIX, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_CURVIS, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_CUSTOM, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_SAVE, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_SELECTEDONLY, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_HELPTEXT, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_APPLYRECALL, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_NAMEPROMPT, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_HIDENEW, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_OPTIONS, 1.0, 1.0, 1.0, 1.0);
#ifndef _WIN32
	SetWindowText(GetDlgItem(m_hwnd, IDC_HELPTEXT), "Del: Alt-click\nSave: Cmd-click");
#endif

	m_pLists.Add(new SWS_SnapshotsView(GetDlgItem(m_hwnd, IDC_LIST), GetDlgItem(m_hwnd, IDC_EDIT)));

	Update();
}

void SWS_SnapshotsWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
		case LOADSEL_MSG:
		{
			Snapshot* ss = (Snapshot*)m_pLists.Get(0)->GetFirstSelected();
			if (ss)
			{
				g_ss.Get()->m_pCurSnapshot = ss;
				if (ss->UpdateReaper(g_bApplyFilterOnRecall ? g_iMask : ALL_MASK, g_bSelOnly, g_bHideNewOnRecall))
					Update();
			}
			break;
		}
		case MERGE_MSG:
		{
			Snapshot* ss = (Snapshot*)m_pLists.Get(0)->GetFirstSelected();
			if (MergeSnapshots(ss))
				Update();
			break;
		}
		case IDC_SAVE:
			NewSnapshot();
			break;
		case IDC_OPTIONS:
			g_bHideOptions = !g_bHideOptions;
			SendMessage(m_hwnd, WM_SIZE, 0, 0);
			break;
		case RENAME_MSG:
			m_pLists.Get(0)->EditListItem(m_pLists.Get(0)->GetFirstSelected(), 1);
			break;
		case SAVE_MSG:
		{
			Snapshot* ss = (Snapshot*)m_pLists.Get(0)->GetFirstSelected();
			if (ss)
			{
				g_ss.Get()->m_pCurSnapshot = g_ss.Get()->m_snapshots.Set(g_ss.Get()->m_snapshots.Find(ss), new Snapshot(ss->m_iSlot, g_iMask, g_bSelOnly, ss->m_cName));
				delete ss;
				Update();
			}
			break;
		}
		case COPY_MSG:
		{
			Snapshot* ss = (Snapshot*)m_pLists.Get(0)->GetFirstSelected();
			CopySnapshotToClipboard(ss);
			break;
		}
		case DELETE_MSG:
		{
			Snapshot* ss = (Snapshot*)m_pLists.Get(0)->GetFirstSelected();
			if (ss)
			{
				char undoStr[128];
				sprintf(undoStr, "Delete snapshot %d", ss->m_iSlot);
				g_ss.Get()->m_snapshots.Delete(g_ss.Get()->m_snapshots.Find(ss), true);
				g_ss.Get()->m_pCurSnapshot = NULL;
				Undo_OnStateChangeEx(undoStr, UNDO_STATE_MISCCFG, -1);
				Update();
			}
			break;
		}
		case SEL_MSG:
		{
			Snapshot* ss = (Snapshot*)m_pLists.Get(0)->GetFirstSelected();
			if (ss)
				ss->SelectTracks();
			break;
		}
		case ADDSEL_MSG:
		{
			Snapshot* ss = (Snapshot*)m_pLists.Get(0)->GetFirstSelected();
			if (ss)
			{
				ss->AddSelTracks();
				Update();
			}
			break;
		}
		case DELSEL_MSG:
		{
			Snapshot* ss = (Snapshot*)m_pLists.Get(0)->GetFirstSelected();
			if (ss)
			{
				ss->DelSelTracks();
				Update();
			}
			break;
		}
		case DETAILS_MSG:
		{
			Snapshot* ss = (Snapshot*)m_pLists.Get(0)->GetFirstSelected();
			WDL_String details;
			ss->GetDetails(&details);
			DisplayInfoBox(m_hwnd, "Snapshot Details", details.Get());
			break;
		}
		case EXPORT_MSG:
		{
			Snapshot* ss = (Snapshot*)m_pLists.Get(0)->GetFirstSelected();
			ExportSnapshot(ss);
			break;
		}
		case IMPORT_MSG:
		{
			ImportSnapshot();
			break;
		}
		case IDC_MIX:
		case IDC_CURVIS:
		case IDC_CUSTOM:
		case IDC_SELECTEDONLY:
		case IDC_APPLYRECALL:
		case IDC_NAMEPROMPT:
		case IDC_HIDENEW:
			// Filter controls are handled in the default case
			GetOptions();
			Update();
			break;
		default:
			for (int i = 0; i < MASK_CTRLS; i++)
				if (wParam == cSSCtrls[i])
				{
					GetOptions();
					Update();
					return;
				}

			if (wParam >= LOAD_MSG && wParam < (WPARAM)(LOAD_MSG + g_ss.Get()->m_snapshots.GetSize()))
				GetSnapshot((int)(wParam-LOAD_MSG), ALL_MASK, false);
			else
				Main_OnCommand((int)wParam, (int)lParam);
	}
}

HMENU SWS_SnapshotsWnd::OnContextMenu(int x, int y)
{
	HMENU contextMenu = CreatePopupMenu();
	LPARAM item = m_pLists.Get(0)->GetHitItem(x, y, NULL);

	if (item)
	{
		AddToMenu(contextMenu, "Merge into project...", MERGE_MSG);
		AddToMenu(contextMenu, "Rename", RENAME_MSG);
		AddToMenu(contextMenu, SWS_SEPARATOR, 0);
		AddToMenu(contextMenu, "Show snapshot details", DETAILS_MSG);
		AddToMenu(contextMenu, "Select tracks in snapshot", SEL_MSG);
		AddToMenu(contextMenu, "Add selected track(s) to snapshot", ADDSEL_MSG);
		AddToMenu(contextMenu, "Delete selected track(s) from snapshot", DELSEL_MSG);
		AddToMenu(contextMenu, "Overwrite snapshot", SAVE_MSG);
		AddToMenu(contextMenu, "Delete snapshot", DELETE_MSG);
		AddToMenu(contextMenu, "Copy snapshot", COPY_MSG);
		AddToMenu(contextMenu, "Export snapshot...", EXPORT_MSG);
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
			if (g_ss.Get()->m_snapshots.Get(i) == g_ss.Get()->m_pCurSnapshot)
				CheckMenuItem(contextMenu, iCmd, MF_CHECKED);
		}
	}
	AddToMenu(contextMenu, "Import snapshot...", IMPORT_MSG);
	AddToMenu(contextMenu, "New snapshot", SWSGetCommandID(NewSnapshot));
	AddToMenu(contextMenu, "Paste snapshot", SWSGetCommandID(PasteSnapshot));

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
	InvalidateRect(m_hwnd, NULL, 0);
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
}

void SWS_SnapshotsWnd::GetOptions()
{
	if (!IsValidWindow())
		return;

	if (IsDlgButtonChecked(m_hwnd, IDC_MIX) == BST_CHECKED)
	{
		m_iSelType = 0;
		g_iMask = MIX_MASK;
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
		for (int i = 0; i < MASK_CTRLS; i++)
			if (IsDlgButtonChecked(m_hwnd, cSSCtrls[i]) == BST_CHECKED)
				g_iMask |= cSSMasks[i];
	}
	g_bSelOnly = IsDlgButtonChecked(m_hwnd, IDC_SELECTEDONLY) == BST_CHECKED;
	g_bHideNewOnRecall = IsDlgButtonChecked(m_hwnd, IDC_HIDENEW) == BST_CHECKED;
	g_bApplyFilterOnRecall = IsDlgButtonChecked(m_hwnd, IDC_APPLYRECALL) == BST_CHECKED;
	g_bPromptOnNew = IsDlgButtonChecked(m_hwnd, IDC_NAMEPROMPT) == BST_CHECKED;
}

void SWS_SnapshotsWnd::ShowControls(bool bShow)
{
	for (int i = 0; i < MASK_CTRLS; i++)
		ShowWindow(GetDlgItem(m_hwnd, cSSCtrls[i]), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_MIX), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_CURVIS), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_CUSTOM), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_FILTERGROUP), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_SAVE), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_SELECTEDONLY), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_HELPTEXT), bShow);
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

	Snapshot* pNewSS = g_ss.Get()->m_snapshots.Insert(i, new Snapshot(i+1, iMask, bSelOnly, NULL));
	g_ss.Get()->m_pCurSnapshot = pNewSS;
	if (g_bPromptOnNew)
	{
		char cName[256];
		strncpy(cName, pNewSS->m_cName, 256);
		if (PromptUserForString(g_hwndParent, "Enter Snapshot Name", cName, 256) && strlen(cName))
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
			g_ss.Get()->m_pCurSnapshot = g_ss.Get()->m_snapshots.Get(i);
			if (g_ss.Get()->m_snapshots.Get(i)->UpdateReaper(iMask, bSelOnly, g_bHideNewOnRecall))
				g_pSSWnd->Update();
			return;
		}
}

void AddSnapshotTracks(COMMAND_T*)
{
	if (g_ss.Get()->m_pCurSnapshot)
	{
		g_ss.Get()->m_pCurSnapshot->AddSelTracks();
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
	if (g_ss.Get()->m_pCurSnapshot)
	{
		g_ss.Get()->m_pCurSnapshot->DelSelTracks();
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
	if (g_ss.Get()->m_pCurSnapshot)
		g_ss.Get()->m_pCurSnapshot->SelectTracks();
}

void SaveCurSnapshot(COMMAND_T*) { SaveSnapshot(g_ss.Get()->m_pCurSnapshot->m_iSlot); }
void SaveSnapshot(COMMAND_T* ct) { SaveSnapshot((int)ct->user); }
void GetCurSnapshot(COMMAND_T*)	 { GetSnapshot(g_ss.Get()->m_pCurSnapshot->m_iSlot, ALL_MASK, false); }
void GetSnapshot(COMMAND_T* ct)	 { GetSnapshot((int)ct->user, ALL_MASK, false); }
void SetSnapType(COMMAND_T* ct)  { g_pSSWnd->SetFilterType(ct->user); UpdateSnapshotsDialog(); }
void TogSnapParam(COMMAND_T* ct) { g_pSSWnd->SetFilterType(2); g_iMask ^= ct->user; UpdateSnapshotsDialog(); }
bool IsSnapParamEn(COMMAND_T* ct){ return (g_iMask & ct->user) ? true : false; }

void ToggleSelOnly(COMMAND_T*)	 { g_bSelOnly = !g_bSelOnly; UpdateSnapshotsDialog(); }
void ToggleAppToRec(COMMAND_T*)	 { g_bApplyFilterOnRecall = !g_bApplyFilterOnRecall; UpdateSnapshotsDialog(); }
void ClearFilter(COMMAND_T*)	 { g_pSSWnd->SetFilterType(2); g_iMask = 0; g_bSelOnly = false; UpdateSnapshotsDialog(); }
void SaveFilter(COMMAND_T*)		 { g_iSavedMask = g_iMask; g_bSavedSelOnly = g_bSelOnly; g_iSavedType = g_pSSWnd->GetFilterType(); }
void RestoreFilter(COMMAND_T*)	 { g_pSSWnd->SetFilterType(g_iSavedType); g_iMask = g_iSavedMask; g_bSelOnly = g_bSavedSelOnly; UpdateSnapshotsDialog(); }

static bool SnapshotsWindowEnabled(COMMAND_T*)
{
	return g_pSSWnd->IsValidWindow();
}

void CopyCurSnapshot(COMMAND_T*)
{
	if (g_ss.Get()->m_pCurSnapshot)
		CopySnapshotToClipboard(g_ss.Get()->m_pCurSnapshot);
}

void CopySelSnapshot(COMMAND_T*)
{
	Snapshot ss(1, g_iMask, true, "unnamed");
	CopySnapshotToClipboard(&ss);
}

void CopyAllSnapshot(COMMAND_T*)
{
	Snapshot ss(1, g_iMask, false, "unnamed");
	CopySnapshotToClipboard(&ss);
}

// This function adds the snapshot or deletes it from memory
void MergeSnapshot(Snapshot* ss)
{
	if (!ss)
		return;

	if (MergeSnapshots(ss))
	{
		// Save the pasted snapshot
		// Find the "slot" -- use the saved, or the first avail
		for (int i = 0; i < g_ss.Get()->m_snapshots.GetSize(); i++)
		{
			if (g_ss.Get()->m_snapshots.Get(i)->m_iSlot == ss->m_iSlot)
			{	// Slot collision, pick the first avail
				int j;
				for (j = 0; j < g_ss.Get()->m_snapshots.GetSize(); j++)
					if (g_ss.Get()->m_snapshots.Get(j)->m_iSlot != j+1)
						break;
				ss->m_iSlot = j + 1;
			}
		}

		// Add the snapshot to the list
		g_ss.Get()->m_snapshots.Add(ss);
		g_pSSWnd->Update();
	}
	else
		delete ss;
}

void PasteSnapshot(COMMAND_T*)
{
	Snapshot* ss = GetSnapshotFromClipboard();
	MergeSnapshot(ss);
}

static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "SWS: Open snapshots window" },						"SWSSNAPSHOT_OPEN",	     OpenSnapshotsDialog,  "Show snapshots list and settings", 0, SnapshotsWindowEnabled },
	{ { DEFACCEL, "SWS: Add selected track(s) to current snapshot" },	"SWSSNAPSHOT_ADD",	     AddSnapshotTracks,	   "Add selected track(s) to current snapshot", },
	{ { DEFACCEL, "SWS: Add selected track(s) to all snapshots" },		"SWSSNAPSHOTS_ADD",	     AddTracks,			   "Add selected track(s) to all snapshots", },
	{ { DEFACCEL, "SWS: Delete selected track(s) from current snapshot" },"SWSSNAPSHOT_DEL",	 DelSnapshotTracks,	   "Delete selected track(s) from current snapshot", },
	{ { DEFACCEL, "SWS: Delete selected track(s) from all snapshots" },	"SWSSNAPSHOTS_DEL",	     DelTracks,			   "Delete selected track(s) from all snapshots", },
	{ { DEFACCEL, "SWS: Select current snapshot track(s)" },			"SWSSNAPSHOT_SEL",	     SelSnapshotTracks,	   "Select current snapshot's track(s)", },
	{ { DEFACCEL, NULL }, NULL, NULL, SWS_SEPARATOR, },
	{ { DEFACCEL, "SWS: New snapshot (all tracks)" },					"SWSSNAPSHOT_NEWALL",    NewSnapshot,		   "New snapshot (all tracks)", 2 },
	{ { DEFACCEL, "SWS: New snapshot (selected track(s))" },			"SWSSNAPSHOT_NEWSEL",    NewSnapshot,		   "New snapshot (selected track(s))", 3 },
	{ { DEFACCEL, "SWS: Save over current snapshot" },					"SWSSNAPSHOT_SAVE",	     SaveCurSnapshot,	   "Save over current snapshot", },
	{ { DEFACCEL, "SWS: Recall current snapshot" },						"SWSSNAPSHOT_GET",	     GetCurSnapshot,       "Recall current snapshot", },
	{ { DEFACCEL, "SWS: Copy current snapshot" },						"SWSSNAPSHOT_COPY",	     CopyCurSnapshot,      "Copy current snapshot", },
	{ { DEFACCEL, "SWS: Copy new snapshot (selected track(s))" },		"SWSSNAPSHOT_COPYSEL",   CopySelSnapshot,      "Copy new snapshot (selected track(s))", },
	{ { DEFACCEL, "SWS: Copy new snapshot (all track(s))" },			"SWSSNAPSHOT_COPYALL",   CopyAllSnapshot,      "Copy new snapshot (all track(s))", },
	{ { DEFACCEL, "SWS: Paste snapshot" },								"SWSSNAPSHOT_PASTE",	 PasteSnapshot,        "Paste snapshot", },

	{ { DEFACCEL, "SWS: New snapshot (with current settings)" },		"SWSSNAPSHOT_NEW",	     NewSnapshot,		   NULL, 0 },
	{ { DEFACCEL, "SWS: New snapshot and edit name" },					"SWSSNAPSHOT_NEWEDIT",   NewSnapshotEdit,	   NULL, 1 },
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

	{ { DEFACCEL, "SWS: Set snapshots to 'mix' mode" },					"SWSSNAPSHOT_MIXMODE",   SetSnapType,    NULL, 0 },
	{ { DEFACCEL, "SWS: Set snapshots to 'visibility' mode" },			"SWSSNAPSHOT_VISMODE",   SetSnapType,    NULL, 1 },
	{ { DEFACCEL, "SWS: Toggle snapshot mute" },						"SWSSNAPSHOT_MUTE",		 TogSnapParam,   NULL, MUTE_MASK,    IsSnapParamEn },
	{ { DEFACCEL, "SWS: Toggle snapshot solo" },						"SWSSNAPSHOT_SOLO",		 TogSnapParam,   NULL, SOLO_MASK,    IsSnapParamEn },
	{ { DEFACCEL, "SWS: Toggle snapshot pan" },							"SWSSNAPSHOT_PAN",		 TogSnapParam,   NULL, PAN_MASK,     IsSnapParamEn },
	{ { DEFACCEL, "SWS: Toggle snapshot vol" },							"SWSSNAPSHOT_VOL",		 TogSnapParam,   NULL, VOL_MASK,     IsSnapParamEn },
	{ { DEFACCEL, "SWS: Toggle snapshot sends" },						"SWSSNAPSHOT_SEND",		 TogSnapParam,   NULL, SENDS_MASK,   IsSnapParamEn },
	{ { DEFACCEL, "SWS: Toggle snapshot fx" },							"SWSSNAPSHOT_FX",		 TogSnapParam,   NULL, FXCHAIN_MASK, IsSnapParamEn },
	{ { DEFACCEL, "SWS: Toggle snapshot visibility" },					"SWSSNAPSHOT_VIS",		 TogSnapParam,   NULL, VIS_MASK,     IsSnapParamEn },
	{ { DEFACCEL, "SWS: Toggle snapshot selection" },					"SWSSNAPSHOT_TOGSEL",    TogSnapParam,   NULL, SEL_MASK,     IsSnapParamEn },
	{ { DEFACCEL, "SWS: Toggle snapshot selected only" },				"SWSSNAPSHOT_SELONLY",	 ToggleSelOnly,  NULL, },
	{ { DEFACCEL, "SWS: Toggle snapshot apply filter to recall" },      "SWSSNAPSHOT_APPLYLOAD", ToggleAppToRec, NULL, },
	{ { DEFACCEL, "SWS: Clear all snapshot filter options" },           "SWSSNAPSHOT_CLEARFILT", ClearFilter,    NULL, },
	{ { DEFACCEL, "SWS: Save current snapshot filter options" },        "SWSSNAPSHOT_SAVEFILT",  SaveFilter,     NULL, },
	{ { DEFACCEL, "SWS: Restore snapshot filter options" },             "SWSSNAPSHOT_RESTFILT",  RestoreFilter,  NULL, },
	{ {}, LAST_COMMAND, }, // Denote end of table
};

static bool ProcessExtensionLine(const char *line, ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	WDL_String chunk;
	if (GetChunkFromProjectState("<SWSSNAPSHOT", &chunk, line, ctx))
	{
		Snapshot* newSS = g_ss.Get()->m_snapshots.Add(new Snapshot(chunk.Get()));
		for (int i = 0; i < newSS->m_tracks.GetSize(); i++)
		{
			TrackSnapshot* ts = newSS->m_tracks.Get(i);
			int adf = 0;
		}
		g_pSSWnd->Update();
		return true;
	}
	return false;
}

static void SaveExtensionConfig(ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	WDL_String chunk, line;
	for (int i = 0; i < g_ss.Get()->m_snapshots.GetSize(); i++)
	{
		Snapshot* ss = g_ss.Get()->m_snapshots.Get(i);
		ss->GetChunk(&chunk);
		int iPos = 0;
		while(GetChunkLine(chunk.Get(), &line, &iPos, false))
			ctx->AddLine(line.Get());
	}
}

static void BeginLoadProjectState(bool isUndo, struct project_config_extension_t *reg)
{
	g_ss.Get()->m_snapshots.Empty(true);
	g_ss.Get()->m_pCurSnapshot = NULL;
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
}

int SnapshotsInit()
{
	if (!plugin_register("projectconfig",&g_projectconfig))
		return 0;
	if (!plugin_register("accelerator",&g_ar))
		return 0;

	SWSRegisterCommands(g_commandTable);
	// Add 12 gets by default, more are added dynamically as needed.
	for (int i = 0; i < 12; i++)
		Snapshot::RegisterGetCommand(i+1);

	if (!plugin_register("hookcustommenu", (void*)menuhook))
		return 0;

	g_pSSWnd = new SWS_SnapshotsWnd;

	return 1;
}

void SnapshotsExit()
{
	delete g_pSSWnd;
}
