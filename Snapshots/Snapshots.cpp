/******************************************************************************
/ Snapshots.cpp
/
/ Copyright (c) 2011 Tim Payne (SWS)
/
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
#include "SnM/SnM.h" // dynamic actions

#include <WDL/projectcontext.h>
#include <WDL/localize/localize.h>

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

enum
{
  WNDID_LR = 2000,
  BTNID_L,
  BTNID_R
};

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
SWS_SnapshotsWnd* g_pSSWnd=NULL;
void PasteSnapshot(COMMAND_T*);
void MergeSnapshot(Snapshot* ss);
void DeleteSnapshot(Snapshot* ss);
void DeleteAllSnapshots(COMMAND_T* = NULL);

static int g_iMask = ALL_MASK;
static int g_compatMask = ALL_MASK; // // for disabling features unavailable in the running version of REAPER
static bool g_bSelOnly_OnRecall = false;
static bool g_bSelOnly_OnSave = false;
static int g_iSavedMask;
static int g_iSavedType;
static bool g_bApplyFilterOnRecall = true;
static bool g_bHideNewOnRecall = true;
static bool g_bPromptOnNew = false;
static bool g_bHideOptions = false;
static bool g_bShowSelOnly = false;
static bool g_bPromptOnDeleted = true;

void UpdateSnapshotsDialog(bool bSelChange)
{
	if (!bSelChange || g_bShowSelOnly)
		g_pSSWnd->Update();
}

// Clipboard operations:
void CopySnapshotToClipboard(Snapshot* ss)
{
	WDL_FastString ssStr;
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
			SetClipboardData(CF_TEXT, hglbCopy);
		}
		CloseClipboard();
	}
}

Snapshot* GetSnapshotFromClipboard()
{
	Snapshot* ss = NULL;
	if (OpenClipboard(g_hwndParent))
	{
		HGLOBAL clipBoard = GetClipboardData(CF_TEXT);
		if (clipBoard)
		{
			char* clipData = (char*)GlobalLock(clipBoard);
			if (clipData)
			{
				ss = new Snapshot(clipData);
				GlobalUnlock(clipBoard);
				if (!ss->m_tracks.GetSize())
				{
					MessageBox(g_hwndParent, __LOCALIZE("Clipboard does not contain a valid snapshot.","sws_DLG_101"), __LOCALIZE("SWS Snapshot Paste Error","sws_DLG_101"), MB_OK);
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
	if (BrowseForSaveFile(__LOCALIZE("Export snapshot...","sws_DLG_101"), cPath, filename, "SWSSnap files\0*.SWSSnap\0", filename, 256))
	{
		ProjectStateContext* cfg = ProjectCreateFileWrite(filename);
		if (cfg)
		{
			WDL_FastString chunk;
			ss->GetChunk(&chunk);
			char line[4096];
			int pos = 0;
			while(GetChunkLine(chunk.Get(), line, 4096, &pos, false))
				cfg->AddLine("%s",line);
			delete cfg;
		}
		else
			MessageBox(g_hwndParent, __LOCALIZE("Unable to write to file.","sws_DLG_101"), __LOCALIZE("SWS Snaphot Export Error","sws_DLG_101"), MB_OK);
	}

}

void ImportSnapshot()
{
	char str[4096];
	GetProjectPath(str, 256);
	char* cFile = BrowseForFiles(__LOCALIZE("Import snapshot...","sws_DLG_101"), str, NULL, false, "SWSSnap files\0*.SWSSnap\0");
	if (cFile)
	{
		ProjectStateContext* cfg = ProjectCreateFileRead(cFile);
		if (cfg)
		{
			WDL_TypedBuf<char> chunk;
			int iLen = 0;
			while(!cfg->GetLine(str, 4096))
			{
				int iNewLen = iLen + (int)strlen(str);
				chunk.Resize(iNewLen + 2);
				strcpy(chunk.Get()+iLen, str);
				strcpy(chunk.Get()+iNewLen, "\n");
				iLen = iNewLen + 1;
			}
			delete cfg;

			Snapshot* ss = new Snapshot(chunk.Get());
			if (!ss->m_tracks.GetSize())
			{
				MessageBox(g_hwndParent, __LOCALIZE("File does not contain a valid snapshot.","sws_DLG_101"), __LOCALIZE("SWS Snapshot Import Error","sws_DLG_101"), MB_OK);
				delete ss;
				ss = NULL;
			}
			else
				MergeSnapshot(ss); // Handles delete of ss if necessary
		}
		else
			MessageBox(g_hwndParent, __LOCALIZE("Unable to open file.","sws_DLG_101"), __LOCALIZE("SWS Snaphot Import Error","sws_DLG_101"), MB_OK);

		free(cFile);
	}
}

// !WANT_LOCALIZE_STRINGS_BEGIN:sws_DLG_101
static SWS_LVColumn g_cols[] = { { 20, 0, "#" }, { 60, 1, "Name" }, { 60, 0, "Date" }, { 60, 0, "Time" }, { 100, 1, "Notes" } };
// !WANT_LOCALIZE_STRINGS_END

SWS_SnapshotsView::SWS_SnapshotsView(HWND hwndList, HWND hwndEdit)
:SWS_ListView(hwndList, hwndEdit, 5, g_cols, "Snapshots View State", true, "sws_DLG_101")
{
}

int SWS_SnapshotsView::OnItemSort(SWS_ListItem* lParam1, SWS_ListItem* lParam2)
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
	case 5: // Description
		iRet = strcmp(item1->m_cNotes, item2->m_cNotes);
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

void SWS_SnapshotsView::SetItemText(SWS_ListItem* item, int iCol, const char* str)
{
	Snapshot* ss = (Snapshot*)item;
	switch (iCol)
	{
	case 1:
		ss->SetName(str);
		break;
	case 4:
		ss->SetNotes(str);
		break;
	}
}

void SWS_SnapshotsView::GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax)
{
	Snapshot* ss = (Snapshot*)item;
	switch(iCol)
	{
	case 0:
		snprintf(str, iStrMax, "%d", ss->m_iSlot);
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
	case 4:
		if (ss->m_cNotes)
			lstrcpyn(str, ss->m_cNotes, iStrMax);
		else
			str[0] = 0;
		break;
	}
}

void SWS_SnapshotsView::GetItemTooltip(SWS_ListItem* item, char* str, int iStrMax)
{
	if (item)
	{
		Snapshot* ss = (Snapshot*)item;
		ss->Tooltip(str, iStrMax);
	}
}

void SWS_SnapshotsView::OnItemClk(SWS_ListItem* item, int iCol, int iKeyState)
{
	if (!item)
		return;

	Snapshot* ss = (Snapshot*)item;

	// Recall (std click)
	if (!(iKeyState & LVKF_SHIFT) && !(iKeyState & LVKF_CONTROL) && !(iKeyState & LVKF_ALT))
	{
		g_ss.Get()->m_pCurSnapshot = ss;
		if (ss->UpdateReaper(g_bApplyFilterOnRecall ? g_iMask : ALL_MASK, g_bSelOnly_OnRecall, g_bHideNewOnRecall))
			Update();
	}
	// Save (ctrl click)
	if (!(iKeyState & LVKF_SHIFT) && (iKeyState & LVKF_CONTROL) && !(iKeyState & LVKF_ALT))
	{
		g_ss.Get()->m_pCurSnapshot = g_ss.Get()->m_snapshots.Set(g_ss.Get()->m_snapshots.Find(ss), new Snapshot(ss->m_iSlot, g_iMask, g_bSelOnly_OnSave, ss->m_cName, ss->m_cNotes));
		delete ss;
		Update();
	}
	// Delete (alt click)
	else if (!(iKeyState & LVKF_SHIFT) && !(iKeyState & LVKF_CONTROL) && (iKeyState & LVKF_ALT))
	{
		DeleteSnapshot(ss);
		Update();
	}
}

void SWS_SnapshotsView::GetItemList(SWS_ListItemList* pList)
{
	if (g_bShowSelOnly)
	{	// Show snaps that include selected tracks
		for (int i = 0; i < g_ss.Get()->m_snapshots.GetSize(); i++)
			if (g_ss.Get()->m_snapshots.Get(i)->IncludesSelTracks())
				pList->Add((SWS_ListItem*)g_ss.Get()->m_snapshots.Get(i));
	}
	else
	{
		for (int i = 0; i < g_ss.Get()->m_snapshots.GetSize(); i++)
			pList->Add((SWS_ListItem*)g_ss.Get()->m_snapshots.Get(i));
	}
}

int SWS_SnapshotsView::GetItemState(SWS_ListItem* item)
{
	Snapshot* ss = (Snapshot*)item;
	return ss == g_ss.Get()->m_pCurSnapshot ? 1 : 0;
}

SWS_SnapshotsWnd::SWS_SnapshotsWnd()
:SWS_DockWnd(IDD_SNAPS, __LOCALIZE("Snapshots","sws_DLG_101"), "SWSSnapshots"),m_iSelType(0)
{
	// Restore state
	char str[32];
	GetPrivateProfileString(SWS_INI, SNAP_OPTIONS_KEY, "559 0 0 0 1 0 0 0 0 1", str, 32, get_ini_file());
	LineParser lp(false);
	if (!lp.parse(str))
	{
		g_iMask = lp.gettoken_int(0);
		g_bApplyFilterOnRecall = lp.gettoken_int(1) ? true : false;
		g_bHideOptions = lp.gettoken_int(2) ? true : false;
		g_bPromptOnNew = lp.gettoken_int(3) ? true : false;
		g_bHideNewOnRecall = lp.gettoken_int(4) ? true : false;
		g_bSelOnly_OnSave = lp.gettoken_int(5) ? true : false;
		m_iSelType = lp.gettoken_int(6);
		g_bSelOnly_OnRecall = lp.gettoken_int(7) ? true : false;
		g_bShowSelOnly = lp.gettoken_int(8) ? true : false;
		g_bPromptOnDeleted = lp.gettoken_int(9) ? true : false;
	}
	// Remove deprecated FXATM
	if (g_iMask & FXATM_MASK)
	{
		g_iMask |= FXCHAIN_MASK;
		g_iMask &= ~FXATM_MASK;
	}

	g_iSavedMask = g_iMask;
	g_iSavedType = m_iSelType;

	// Must call SWS_DockWnd::Init() to restore parameters and open the window if necessary
	Init();
}

SWS_SnapshotsWnd::~SWS_SnapshotsWnd()
{
#ifdef _SNAP_TINY_BUTTONS
	m_tinyLRbtns.RemoveAllChildren(false);
	m_tinyLRbtns.SetRealParent(NULL);
#endif
}

void SWS_SnapshotsWnd::Update()
{
	static bool bRecurseCheck = false;
	if (!IsValidWindow() || bRecurseCheck || !m_pLists.GetSize() || m_pLists.Get(0)->UpdatesDisabled())
		return;
	bRecurseCheck = true;

	//Update the check boxes
	CheckDlgButton(m_hwnd, IDC_MIX,					m_iSelType == 0			? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_CURVIS,				m_iSelType == 1			? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_CUSTOM,				m_iSelType == 2			? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_SELECTEDONLY_SAVE,	g_bSelOnly_OnSave		? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_SELECTEDONLY_RECALL,	g_bSelOnly_OnRecall		? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_APPLYRECALL,			g_bApplyFilterOnRecall	? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_NAMEPROMPT,			g_bPromptOnNew			? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_HIDENEW,				g_bHideNewOnRecall		? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_SHOWSELONLY,			g_bShowSelOnly			? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(m_hwnd, IDC_DELTRACKSPROMPT,		g_bPromptOnDeleted		? BST_CHECKED : BST_UNCHECKED);
	for (int i = 0; i < MASK_CTRLS; i++)
	{
		CheckDlgButton(m_hwnd, cSSCtrls[i], g_iMask & cSSMasks[i] & g_compatMask ? BST_CHECKED : BST_UNCHECKED);
		EnableWindow(GetDlgItem(m_hwnd,  cSSCtrls[i]), m_iSelType == 2 && cSSMasks[i] & g_compatMask);
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
	m_pLists.Get(0)->EditListItem((SWS_ListItem*)g_ss.Get()->m_snapshots.Get(i), 1);
}

bool SWS_SnapshotsWnd::GetPromptOnDeletedTracks()
{
	return g_bPromptOnDeleted;
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
	m_resize.init_item(IDC_NEXT, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_PREVIOUS, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_SWAP_UP, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_SWAP_DOWN, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_SELECTEDONLY_SAVE, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_SELECTEDONLY_RECALL, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_SHOWSELONLY, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_HELPTEXT, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_APPLYRECALL, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_NAMEPROMPT, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_HIDENEW, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_DELTRACKSPROMPT, 1.0, 0.0, 1.0, 0.0);
#ifndef _SNAP_TINY_BUTTONS
	m_resize.init_item(IDC_OPTIONS, 1.0, 1.0, 1.0, 1.0);
#endif

#ifndef _WIN32
	SetWindowText(GetDlgItem(m_hwnd, IDC_HELPTEXT), __LOCALIZE("Del: Alt-click\nSave: Cmd-click","sws_DLG_101"));
#endif

	m_pLists.Add(new SWS_SnapshotsView(GetDlgItem(m_hwnd, IDC_LIST), GetDlgItem(m_hwnd, IDC_EDIT)));

#ifdef _SNAP_TINY_BUTTONS
	m_vwnd_painter.SetGSC(WDL_STYLE_GetSysColor);
	m_parentVwnd.SetRealParent(m_hwnd);

	m_btnLeft.SetID(BTNID_L);
	m_tinyLRbtns.AddChild(&m_btnLeft);
	m_btnRight.SetID(BTNID_R);
	m_tinyLRbtns.AddChild(&m_btnRight);
	m_tinyLRbtns.SetID(WNDID_LR);
	m_parentVwnd.AddChild(&m_tinyLRbtns);
#endif

	Update();
}

void SWS_SnapshotsWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
		case LOADSEL_MSG:
		{
			Snapshot* ss = (Snapshot*)m_pLists.Get(0)->EnumSelected(NULL);
			if (ss)
			{
				g_ss.Get()->m_pCurSnapshot = ss;
				if (ss->UpdateReaper(g_bApplyFilterOnRecall ? g_iMask : ALL_MASK, g_bSelOnly_OnRecall, g_bHideNewOnRecall))
					Update();
			}
			break;
		}
		case MERGE_MSG:
		{
			Snapshot* ss = (Snapshot*)m_pLists.Get(0)->EnumSelected(NULL);
			if (MergeSnapshots(ss))
				Update();
			break;
		}
		case IDC_SAVE:
			NewSnapshot();
			break;
		case IDC_NEXT:
		{
			Snapshot* ss = (Snapshot*)m_pLists.Get(0)->EnumSelected(NULL,1);
			if (ss)
			{
				g_ss.Get()->m_pCurSnapshot = ss;
				if (ss->UpdateReaper(g_bApplyFilterOnRecall ? g_iMask : ALL_MASK, g_bSelOnly_OnRecall, g_bHideNewOnRecall))
					Update();
			}
			break;
		}
		case IDC_PREVIOUS:
		{
			Snapshot* ss = (Snapshot*)m_pLists.Get(0)->EnumSelected(NULL,-1);
			if (ss)
			{
				g_ss.Get()->m_pCurSnapshot = ss;
				if (ss->UpdateReaper(g_bApplyFilterOnRecall ? g_iMask : ALL_MASK, g_bSelOnly_OnRecall, g_bHideNewOnRecall))
					Update();
			}
			break;
		}
		case IDC_SWAP_UP:
		{
			Snapshot* ss = (Snapshot*)m_pLists.Get(0)->EnumSelected(NULL);
			if (ss)
			{
				if ((ss->m_iSlot > 0) ) //&& (m_pLists.Get(0)->GetSortColumn() == 1) //can either ignore the button here or ...
				{
					Snapshot* ss_swap = (Snapshot*)m_pLists.Get(0)->EnumSelected(NULL,-1);
					if (ss_swap)
					{
						m_pLists.Get(0)->SetSortColumn(1); //force the sort to slot...doesnt make sense with other sorts
						int temp = ss->m_iSlot;
						ss->m_iSlot = ss_swap->m_iSlot;
						ss_swap->m_iSlot = temp;
						Update();
					}
				}
			}
			break;
		}
		case IDC_SWAP_DOWN:
		{
			Snapshot* ss = (Snapshot*)m_pLists.Get(0)->EnumSelected(NULL);
			if (ss)
			{
				if (ss->m_iSlot < g_ss.Get()->m_snapshots.GetSize())
				{
					Snapshot* ss_swap = (Snapshot*)m_pLists.Get(0)->EnumSelected(NULL,1);
					if (ss_swap)
					{
						m_pLists.Get(0)->SetSortColumn(1); //force the sort to slot...doesnt make sense with other sorts
						int temp = ss->m_iSlot;
						ss->m_iSlot = ss_swap->m_iSlot;
						ss_swap->m_iSlot = temp;
						Update();
					}
				}
			}
			break;
		}

#ifdef _SNAP_TINY_BUTTONS
		case BTNID_R:
			g_bHideOptions=true;
			SendMessage(m_hwnd, WM_SIZE, 0, 0);
			break;
		case BTNID_L:
			g_bHideOptions=false;
			SendMessage(m_hwnd, WM_SIZE, 0, 0);
			break;
#else
		case IDC_OPTIONS:
			g_bHideOptions = !g_bHideOptions;
			SendMessage(m_hwnd, WM_SIZE, 0, 0);
			break;
#endif
		case RENAME_MSG:
			m_pLists.Get(0)->EditListItem(m_pLists.Get(0)->EnumSelected(NULL), 1);
			break;
		case SAVE_MSG:
		{
			Snapshot* ss = (Snapshot*)m_pLists.Get(0)->EnumSelected(NULL);
			if (ss)
			{
				g_ss.Get()->m_pCurSnapshot = g_ss.Get()->m_snapshots.Set(g_ss.Get()->m_snapshots.Find(ss), new Snapshot(ss->m_iSlot, g_iMask, g_bSelOnly_OnSave, ss->m_cName, ss->m_cNotes));
				delete ss;
				Update();
			}
			break;
		}
		case COPY_MSG:
		{
			Snapshot* ss = (Snapshot*)m_pLists.Get(0)->EnumSelected(NULL);
			CopySnapshotToClipboard(ss);
			break;
		}
		case DELETE_MSG:
		{
			if(Snapshot* ss = (Snapshot*)m_pLists.Get(0)->EnumSelected(NULL)) {
				DeleteSnapshot(ss);
				Update();
			}
			break;
		}
		case SEL_MSG:
		{
			Snapshot* ss = (Snapshot*)m_pLists.Get(0)->EnumSelected(NULL);
			if (ss)
				ss->SelectTracks();
			break;
		}
		case ADDSEL_MSG:
		{
			Snapshot* ss = (Snapshot*)m_pLists.Get(0)->EnumSelected(NULL);
			if (ss)
			{
				ss->AddSelTracks();
				Update();
			}
			break;
		}
		case DELSEL_MSG:
		{
			Snapshot* ss = (Snapshot*)m_pLists.Get(0)->EnumSelected(NULL);
			if (ss)
			{
				ss->DelSelTracks();
				Update();
			}
			break;
		}
		case DETAILS_MSG:
		{
			Snapshot* ss = (Snapshot*)m_pLists.Get(0)->EnumSelected(NULL);
			WDL_FastString details;
			ss->GetDetails(&details);
			DisplayInfoBox(m_hwnd, __LOCALIZE("Snapshot Details","sws_DLG_101"), details.Get());
			break;
		}
		case EXPORT_MSG:
		{
			Snapshot* ss = (Snapshot*)m_pLists.Get(0)->EnumSelected(NULL);
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
		case IDC_SELECTEDONLY_SAVE:
		case IDC_SELECTEDONLY_RECALL:
		case IDC_APPLYRECALL:
		case IDC_NAMEPROMPT:
		case IDC_HIDENEW:
		case IDC_SHOWSELONLY:
		case IDC_DELTRACKSPROMPT:
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

HMENU SWS_SnapshotsWnd::OnContextMenu(int x, int y, bool* wantDefaultItems)
{
	HMENU contextMenu = CreatePopupMenu();
	SWS_ListItem* item = m_pLists.Get(0)->GetHitItem(x, y, NULL);

	if (item)
	{
		AddToMenu(contextMenu, __LOCALIZE("Merge into project...","sws_DLG_101"), MERGE_MSG);
		AddToMenu(contextMenu, __LOCALIZE("Rename","sws_DLG_101"), RENAME_MSG);
		AddToMenu(contextMenu, SWS_SEPARATOR, 0);
		AddToMenu(contextMenu, __LOCALIZE("Show snapshot details","sws_DLG_101"), DETAILS_MSG);
		AddToMenu(contextMenu, __LOCALIZE("Select tracks in snapshot","sws_DLG_101"), SEL_MSG);
		AddToMenu(contextMenu, __LOCALIZE("Add selected track(s) to snapshot","sws_DLG_101"), ADDSEL_MSG);
		AddToMenu(contextMenu, __LOCALIZE("Delete selected track(s) from snapshot","sws_DLG_101"), DELSEL_MSG);
		AddToMenu(contextMenu, __LOCALIZE("Overwrite snapshot","sws_DLG_101"), SAVE_MSG);
		AddToMenu(contextMenu, __LOCALIZE("Delete snapshot","sws_DLG_101"), DELETE_MSG);
		AddToMenu(contextMenu, __LOCALIZE("Copy snapshot","sws_DLG_101"), COPY_MSG);
		AddToMenu(contextMenu, __LOCALIZE("Export snapshot...","sws_DLG_101"), EXPORT_MSG);
	}
	else
	{
		for (int i = 0; i < g_ss.Get()->m_snapshots.GetSize(); i++)
		{
			int iCmd = SWSGetCommandID(GetSnapshot, i+1);
			char cName[50];
			snprintf(cName, 50, __LOCALIZE_VERFMT("Recall %s","sws_DLG_101"), g_ss.Get()->m_snapshots.Get(i)->m_cName);
			if (!iCmd)
				iCmd = LOAD_MSG + i;
			AddToMenu(contextMenu, cName, iCmd);
			if (g_ss.Get()->m_snapshots.Get(i) == g_ss.Get()->m_pCurSnapshot)
				CheckMenuItem(contextMenu, iCmd, MF_CHECKED);
		}
	}
	AddToMenu(contextMenu, __LOCALIZE("Import snapshot...","sws_DLG_101"), IMPORT_MSG);
	AddToMenu(contextMenu, __LOCALIZE("New snapshot","sws_DLG_101"), SWSGetCommandID(NewSnapshot));
	AddToMenu(contextMenu, __LOCALIZE("Paste snapshot","sws_DLG_101"), SWSGetCommandID(PasteSnapshot));
	AddToMenu(contextMenu, __LOCALIZE("Delete all snapshots","sws_DLG_101"), SWSGetCommandID(DeleteAllSnapshots));

	return contextMenu;
}

void SWS_SnapshotsWnd::OnResize()
{
#ifndef _SNAP_TINY_BUTTONS
	if (!g_bHideOptions)
	{
		ShowControls(true);
		memcpy(&m_resize.get_item(IDC_LIST)->orig, &m_resize.get_item(IDC_LIST)->real_orig, sizeof(RECT));
		memcpy(&m_resize.get_item(IDC_OPTIONS)->orig, &m_resize.get_item(IDC_OPTIONS)->real_orig, sizeof(RECT));
		m_resize.get_item(IDC_OPTIONS)->scales[0] = 1.0;
		m_resize.get_item(IDC_OPTIONS)->scales[2] = 1.0;
		SetDlgItemText(m_hwnd, IDC_OPTIONS, __LOCALIZE("<- Hide Options","sws_DLG_101"));
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
		SetDlgItemText(m_hwnd, IDC_OPTIONS, __LOCALIZE("Show Options ->","sws_DLG_101"));
	}
#else
	if (!g_bHideOptions)
	{
		ShowControls(true);
		m_resize.get_item(IDC_LIST)->orig = m_resize.get_item(IDC_LIST)->real_orig;
	}
	else
	{
		ShowControls(false);
		RECT* r = &m_resize.get_item(IDC_LIST)->orig;
		r->right = m_resize.get_orig_rect().right - r->left;
	}
#endif
	InvalidateRect(m_hwnd, NULL, 0);
}

void SWS_SnapshotsWnd::OnDestroy()
{
	char str[256];

	// Save window state
	sprintf(str, "%d %d %d %d %d %d %d %d %d %d",
		g_iMask,
		g_bApplyFilterOnRecall ? 1 : 0,
		g_bHideOptions ? 1 : 0,
		g_bPromptOnNew ? 1 : 0,
		g_bHideNewOnRecall ? 1 : 0,
		g_bSelOnly_OnSave ? 1 : 0,
		m_iSelType,
		g_bSelOnly_OnRecall ? 1 : 0,
		g_bShowSelOnly ? 1 : 0,
		g_bPromptOnDeleted ? 1 : 0);
	WritePrivateProfileString(SWS_INI, SNAP_OPTIONS_KEY, str, get_ini_file());

#ifdef _SNAP_TINY_BUTTONS
	m_tinyLRbtns.RemoveAllChildren(false);
	m_tinyLRbtns.SetRealParent(NULL);
#endif
}

int SWS_SnapshotsWnd::OnKey(MSG* msg, int iKeyState)
{
	if (msg->message == WM_KEYDOWN && !iKeyState)
	{
		switch(msg->wParam)
		{
		case VK_RETURN:
			OnCommand(LOADSEL_MSG, 0);
			return 1;
		case VK_DELETE:
			OnCommand(DELETE_MSG, 0);
			return 1;
		case VK_F2:
			OnCommand(RENAME_MSG, 0);
			return 1;
		}
	}
	return 0;
}

void SWS_SnapshotsWnd::GetOptions()
{
	if (!IsValidWindow())
		return;

	if (IsDlgButtonChecked(m_hwnd, IDC_MIX) == BST_CHECKED)
	{
		m_iSelType = 0;
		g_iMask = MIX_MASK & g_compatMask;
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
	g_bSelOnly_OnSave = IsDlgButtonChecked(m_hwnd, IDC_SELECTEDONLY_SAVE) == BST_CHECKED;
	g_bSelOnly_OnRecall = IsDlgButtonChecked(m_hwnd, IDC_SELECTEDONLY_RECALL) == BST_CHECKED;
	g_bHideNewOnRecall = IsDlgButtonChecked(m_hwnd, IDC_HIDENEW) == BST_CHECKED;
	g_bShowSelOnly = IsDlgButtonChecked(m_hwnd, IDC_SHOWSELONLY) == BST_CHECKED;
	g_bApplyFilterOnRecall = IsDlgButtonChecked(m_hwnd, IDC_APPLYRECALL) == BST_CHECKED;
	g_bPromptOnNew = IsDlgButtonChecked(m_hwnd, IDC_NAMEPROMPT) == BST_CHECKED;
	g_bPromptOnDeleted = IsDlgButtonChecked(m_hwnd, IDC_DELTRACKSPROMPT) == BST_CHECKED;
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
	ShowWindow(GetDlgItem(m_hwnd, IDC_PREVIOUS), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_NEXT), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_SWAP_UP), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_SWAP_DOWN), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_SELECTEDONLY_SAVE), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_SELECTEDONLY_RECALL), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_SHOWSELONLY), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_HELPTEXT), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_APPLYRECALL), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_NAMEPROMPT), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_HIDENEW), bShow);
	ShowWindow(GetDlgItem(m_hwnd, IDC_DELTRACKSPROMPT), bShow);
}

#ifdef _SNAP_TINY_BUTTONS
void SWS_SnapshotsWnd::DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight)
{
	RECT r;
	GetWindowRect(GetDlgItem(m_hwnd, IDC_LIST), &r);
	ScreenToClient(m_hwnd, (LPPOINT)&r);
	ScreenToClient(m_hwnd, ((LPPOINT)&r)+1);
	r.top = r.bottom - (9*2+1+1);
	r.bottom = r.top + (9*2+1);
	r.left = r.right + 1;
	r.right = r.left + 5;

	m_btnLeft.SetEnabled(g_bHideOptions);
	m_btnRight.SetEnabled(!g_bHideOptions);
	m_tinyLRbtns.SetPosition(&r);
	m_tinyLRbtns.SetVisible(true);
}
#endif

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
	//clean up the slots when deleting and there won't be gaps to fill in the iSlot numbering...just append new entries.  :)
	Snapshot* pNewSS = g_ss.Get()->m_snapshots.Add(new Snapshot(g_ss.Get()->m_snapshots.GetSize() + 1, iMask, bSelOnly, NULL, NULL));
	g_ss.Get()->m_pCurSnapshot = pNewSS;
	if (g_bPromptOnNew)
	{
		char cName[256];
		strncpy(cName, pNewSS->m_cName, 256);
		if (PromptUserForString(g_hwndParent, __LOCALIZE("Enter Snapshot Name","sws_DLG_101"), cName, 256) && strlen(cName))
			pNewSS->SetName(cName);
	}
	g_pSSWnd->Update();
}

void NewSnapshotEdit(COMMAND_T* = NULL)
{
	NewSnapshot(g_iMask, g_bSelOnly_OnSave);
	g_pSSWnd->RenameCurrent();
}

void NewSnapshot(COMMAND_T* ct)
{
	if (ct && ct->user == 1)
		NewSnapshot(g_iMask, false);
	else if (ct && ct->user == 2)
		NewSnapshot(g_iMask, true);
	else
		NewSnapshot(g_iMask, g_bSelOnly_OnSave);
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
		snprintf(str, sizeof(str), "%s %d", __LOCALIZE("Mix","sws_DLG_101"), slot);
		g_ss.Get()->m_pCurSnapshot = g_ss.Get()->m_snapshots.Insert(i, new Snapshot(slot, g_iMask, g_bSelOnly_OnSave, str, NULL));
	}
	else // Overwriting slot
	{
		Snapshot* oldSnapshot = g_ss.Get()->m_snapshots.Get(i);
		g_ss.Get()->m_pCurSnapshot = g_ss.Get()->m_snapshots.Set(i, new Snapshot(oldSnapshot->m_iSlot, g_iMask, g_bSelOnly_OnSave, oldSnapshot->m_cName, oldSnapshot->m_cNotes));
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

void SaveCurSnapshot(COMMAND_T*)     { if (g_ss.Get()->m_pCurSnapshot) 
                                           SaveSnapshot(g_ss.Get()->m_pCurSnapshot->m_iSlot); }
void SaveSnapshot(COMMAND_T* ct)     { SaveSnapshot((int)ct->user + 1); }
void GetCurSnapshot(COMMAND_T*)      { if (g_ss.Get()->m_pCurSnapshot) 
                                           GetSnapshot(g_ss.Get()->m_pCurSnapshot->m_iSlot, g_bApplyFilterOnRecall ? g_iMask : ALL_MASK, g_bSelOnly_OnRecall); }
void GetPreviousSnapshot(COMMAND_T*) { if ((g_ss.Get()->m_pCurSnapshot) && (g_ss.Get()->m_pCurSnapshot)->m_iSlot >= 0) 
                                           GetSnapshot(((g_ss.Get()->m_pCurSnapshot->m_iSlot) - 1), g_bApplyFilterOnRecall ? g_iMask : ALL_MASK, g_bSelOnly_OnRecall); }
void GetNextSnapshot(COMMAND_T*)     { if (g_ss.Get()->m_pCurSnapshot) 
                                           GetSnapshot(((g_ss.Get()->m_pCurSnapshot->m_iSlot) + 1), g_bApplyFilterOnRecall ? g_iMask : ALL_MASK, g_bSelOnly_OnRecall); }
void GetSnapshot(COMMAND_T* ct)      { GetSnapshot((int)ct->user + 1, g_bApplyFilterOnRecall ? g_iMask : ALL_MASK, g_bSelOnly_OnRecall); }
void SetSnapType(COMMAND_T* ct)
{
	int type=(int)ct->user;
	g_pSSWnd->SetFilterType(type);
	if (type==0) g_iMask = MIX_MASK & g_compatMask;
	else if (type==1) g_iMask = VIS_MASK;
	// use current g_iMask otherwise
	UpdateSnapshotsDialog();
}
void TogSnapParam(COMMAND_T* ct) { g_pSSWnd->SetFilterType(2); g_iMask ^= ct->user; UpdateSnapshotsDialog(); }
void ToggleSelOnlySave(COMMAND_T*)	{ g_bSelOnly_OnSave = !g_bSelOnly_OnSave; UpdateSnapshotsDialog(); }
void ToggleSelOnlyRecall(COMMAND_T*){ g_bSelOnly_OnRecall = !g_bSelOnly_OnRecall; UpdateSnapshotsDialog(); }
void ToggleShowForSelTracks(COMMAND_T*){ g_bShowSelOnly = !g_bShowSelOnly; UpdateSnapshotsDialog(); }
void ToggleAppToRec(COMMAND_T*)	 { g_bApplyFilterOnRecall = !g_bApplyFilterOnRecall; UpdateSnapshotsDialog(); }
void ClearFilter(COMMAND_T*)	 { g_pSSWnd->SetFilterType(2); g_iMask = 0; UpdateSnapshotsDialog(); }
void SaveFilter(COMMAND_T*)		 { g_iSavedMask = g_iMask; g_iSavedType = g_pSSWnd->GetFilterType(); }
void RestoreFilter(COMMAND_T*)	 { g_pSSWnd->SetFilterType(g_iSavedType); g_iMask = g_iSavedMask; UpdateSnapshotsDialog(); }

int IsSnapParamEn(COMMAND_T* ct)
{
	if (ct->doCommand == TogSnapParam)
		return (g_iMask & ct->user) ? true : false;
	else if (ct->doCommand == ToggleSelOnlySave)
		return g_bSelOnly_OnSave;
	else if (ct->doCommand == ToggleSelOnlyRecall)
		return g_bSelOnly_OnRecall;
	else if (ct->doCommand == ToggleShowForSelTracks)
		return g_bShowSelOnly;
	else if (ct->doCommand == ToggleAppToRec)
		return g_bApplyFilterOnRecall;
	return false;
}


int SnapshotsWindowEnabled(COMMAND_T*)
{
	return g_pSSWnd->IsWndVisible();
}

void CopyCurSnapshot(COMMAND_T*)
{
	if (g_ss.Get()->m_pCurSnapshot)
		CopySnapshotToClipboard(g_ss.Get()->m_pCurSnapshot);
}

void CopySelSnapshot(COMMAND_T*)
{
	Snapshot ss(1, g_iMask, true, __LOCALIZE("unnamed","sws_DLG_101"), NULL);
	CopySnapshotToClipboard(&ss);
}

void CopyAllSnapshot(COMMAND_T*)
{
	Snapshot ss(1, g_iMask, false, __LOCALIZE("unnamed","sws_DLG_101"), NULL);
	CopySnapshotToClipboard(&ss);
}

void DeleteCurSnapshot(COMMAND_T*)
{
	if(Snapshot *ss = g_ss.Get()->m_pCurSnapshot) {
		DeleteSnapshot(ss);
		g_pSSWnd->Update();
	}
}

void DeleteAllSnapshots(COMMAND_T *action)
{
	g_ss.Get()->m_pCurSnapshot = NULL;
	g_ss.Get()->m_snapshots.Empty(true);

	if(action) { // was this user-initiated?
		Undo_OnStateChangeEx(action->menuText, UNDO_STATE_MISCCFG, -1);
		g_pSSWnd->Update();
	}
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

void DeleteSnapshot(Snapshot *ss)
{
	if (g_ss.Get()->m_pCurSnapshot == ss)
		g_ss.Get()->m_pCurSnapshot = NULL;

	const int iSlot = ss->m_iSlot;
	g_ss.Get()->m_snapshots.Delete(g_ss.Get()->m_snapshots.Find(ss), true);

	// clean up gaps rather than search for them on insert
	for (int i = 0; i < g_ss.Get()->m_snapshots.GetSize(); i++)
		if (g_ss.Get()->m_snapshots.Get(i)->m_iSlot > iSlot)
			g_ss.Get()->m_snapshots.Get(i)->m_iSlot--;

	char undoStr[128] = "";
	snprintf(undoStr, 128, __LOCALIZE_VERFMT("Delete snapshot %d", "sws_DLG_101"), iSlot);
	Undo_OnStateChangeEx(undoStr, UNDO_STATE_MISCCFG, -1);
}

//!WANT_LOCALIZE_SWS_CMD_TABLE_BEGIN:sws_actions
static COMMAND_T g_commandTable[] =
{
	{ { DEFACCEL, "SWS: Open snapshots window" },							"SWSSNAPSHOT_OPEN",	     OpenSnapshotsDialog,  "Show snapshots list and settings", 0, SnapshotsWindowEnabled },
	{ { DEFACCEL, "SWS: Add selected track(s) to current snapshot" },		"SWSSNAPSHOT_ADD",	     AddSnapshotTracks,	   "Add selected track(s) to current snapshot", },
	{ { DEFACCEL, "SWS: Add selected track(s) to all snapshots" },			"SWSSNAPSHOTS_ADD",	     AddTracks,			   "Add selected track(s) to all snapshots", },
	{ { DEFACCEL, "SWS: Delete selected track(s) from current snapshot" },	"SWSSNAPSHOT_DEL",	 DelSnapshotTracks,	   "Delete selected track(s) from current snapshot", },
	{ { DEFACCEL, "SWS: Delete selected track(s) from all snapshots" },		"SWSSNAPSHOTS_DEL",	     DelTracks,			   "Delete selected track(s) from all snapshots", },
	{ { DEFACCEL, "SWS: Select current snapshot track(s)" },				"SWSSNAPSHOT_SEL",	     SelSnapshotTracks,	   "Select current snapshot's track(s)", },
	{ { DEFACCEL, NULL }, NULL, NULL, SWS_SEPARATOR, },
	{ { DEFACCEL, "SWS: New snapshot (all tracks)" },						"SWSSNAPSHOT_NEWALL",    NewSnapshot,		   "New snapshot (all tracks)", 1 },
	{ { DEFACCEL, "SWS: New snapshot (selected track(s))" },				"SWSSNAPSHOT_NEWSEL",    NewSnapshot,		   "New snapshot (selected track(s))", 2 },
	{ { DEFACCEL, "SWS: Save over current snapshot" },						"SWSSNAPSHOT_SAVE",	     SaveCurSnapshot,	   "Save over current snapshot", },
	{ { DEFACCEL, "SWS: Recall current snapshot" },							"SWSSNAPSHOT_GET",	     GetCurSnapshot,       "Recall current snapshot", },
	{ { DEFACCEL, "SWS: Recall previous snapshot" },						"SWSSNAPSHOT_GET_PREVIOUS",	 GetPreviousSnapshot,  "Recall previous snapshot", },
	{ { DEFACCEL, "SWS: Recall next snapshot" },							"SWSSNAPSHOT_GET_NEXT",	     GetNextSnapshot,      "Recall next snapshot", },
	{ { DEFACCEL, "SWS: Copy current snapshot" },							"SWSSNAPSHOT_COPY",	     CopyCurSnapshot,      "Copy current snapshot", },
	{ { DEFACCEL, "SWS: Copy new snapshot (selected track(s))" },			"SWSSNAPSHOT_COPYSEL",   CopySelSnapshot,      "Copy new snapshot (selected track(s))", },
	{ { DEFACCEL, "SWS: Copy new snapshot (all track(s))" },				"SWSSNAPSHOT_COPYALL",   CopyAllSnapshot,      "Copy new snapshot (all track(s))", },
	{ { DEFACCEL, "SWS: Paste snapshot" },									"SWSSNAPSHOT_PASTE",	 PasteSnapshot,        "Paste snapshot", },

	{ { DEFACCEL, "SWS: New snapshot (with current settings)" },			"SWSSNAPSHOT_NEW",	     NewSnapshot,		   NULL, 0 },
	{ { DEFACCEL, "SWS: New snapshot and edit name" },						"SWSSNAPSHOT_NEWEDIT",   NewSnapshotEdit,	   NULL, 1 },
	{ { DEFACCEL, "SWS: Delete current snapshot" },						"SWSSNAPSHOT_DELCUR",   DeleteCurSnapshot },
	{ { DEFACCEL, "SWS: Delete all snapshots" },							"SWSSNAPSHOT_DELALL",   DeleteAllSnapshots, "Delete all snapshots" },

	{ { DEFACCEL, "SWS: Set snapshots to 'mix' mode" },						"SWSSNAPSHOT_MIXMODE",   SetSnapType,    NULL, 0 },
	{ { DEFACCEL, "SWS: Set snapshots to 'visibility' mode" },				"SWSSNAPSHOT_VISMODE",   SetSnapType,    NULL, 1 },

	{ { DEFACCEL, "SWS: Toggle snapshot mute" },							"SWSSNAPSHOT_MUTE",			TogSnapParam,			NULL, MUTE_MASK,    IsSnapParamEn },
	{ { DEFACCEL, "SWS: Toggle snapshot solo" },							"SWSSNAPSHOT_SOLO",			TogSnapParam,			NULL, SOLO_MASK,    IsSnapParamEn },
	{ { DEFACCEL, "SWS: Toggle snapshot pan" },								"SWSSNAPSHOT_PAN",			TogSnapParam,			NULL, PAN_MASK,     IsSnapParamEn },
	{ { DEFACCEL, "SWS: Toggle snapshot vol" },								"SWSSNAPSHOT_VOL",			TogSnapParam,			NULL, VOL_MASK,     IsSnapParamEn },
	{ { DEFACCEL, "SWS: Toggle snapshot sends" },							"SWSSNAPSHOT_SEND",			TogSnapParam,			NULL, SENDS_MASK,   IsSnapParamEn },
	{ { DEFACCEL, "SWS: Toggle snapshot FX" },								"SWSSNAPSHOT_FX",			TogSnapParam,			NULL, FXCHAIN_MASK, IsSnapParamEn },
	{ { DEFACCEL, "SWS: Toggle snapshot visibility" },						"SWSSNAPSHOT_VIS",			TogSnapParam,			NULL, VIS_MASK,     IsSnapParamEn },
	{ { DEFACCEL, "SWS: Toggle snapshot selection" },						"SWSSNAPSHOT_TOGSEL",		TogSnapParam,			NULL, SEL_MASK,     IsSnapParamEn },
	{ { DEFACCEL, "SWS: Toggle snapshot selected only on save" },			"SWSSNAPSHOT_SELONLY",		ToggleSelOnlySave,		NULL, 0,			IsSnapParamEn },
	{ { DEFACCEL, "SWS: Toggle snapshot selected only on recall" },			"SWSSNAPSHOT_SELONLYRECALL",ToggleSelOnlyRecall,	NULL, 0,			IsSnapParamEn },
	{ { DEFACCEL, "SWS: Toggle snapshot apply filter to recall" },			"SWSSNAPSHOT_APPLYLOAD",	ToggleAppToRec,			NULL, 0,			IsSnapParamEn },
	{ { DEFACCEL, "SWS: Toggle snapshot show only for selected tracks" },	"SWSSNAPSHOT_SHOWONLYSEL",	ToggleShowForSelTracks, NULL, 0,			IsSnapParamEn },

	{ { DEFACCEL, "SWS: Clear all snapshot filter options" },				"SWSSNAPSHOT_CLEARFILT", ClearFilter,    NULL, },
	{ { DEFACCEL, "SWS: Save current snapshot filter options" },			"SWSSNAPSHOT_SAVEFILT",  SaveFilter,     NULL, },
	{ { DEFACCEL, "SWS: Restore snapshot filter options" },					"SWSSNAPSHOT_RESTFILT",  RestoreFilter,  NULL, },
	{ {}, LAST_COMMAND, }, // Denote end of table
};
//!WANT_LOCALIZE_SWS_CMD_TABLE_END

static bool ProcessExtensionLine(const char *line, ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	WDL_TypedBuf<char> buf;
	if (GetChunkFromProjectState("<SWSSNAPSHOT", &buf, line, ctx))
	{
		g_ss.Get()->m_snapshots.Add(new Snapshot(buf.Get()));
		g_pSSWnd->Update();
		return true;
	}
	return false;
}

static void SaveExtensionConfig(ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	WDL_FastString chunk;
	char line[4096];
	for (int i = 0; i < g_ss.Get()->m_snapshots.GetSize(); i++)
	{
		Snapshot* ss = g_ss.Get()->m_snapshots.Get(i);
		ss->GetChunk(&chunk);
		int iPos = 0;
		while(GetChunkLine(chunk.Get(), line, 4096, &iPos, false))
			ctx->AddLine("%s",line);
	}
}

static void BeginLoadProjectState(bool isUndo, struct project_config_extension_t *reg)
{
	DeleteAllSnapshots();
	g_ss.Cleanup();
	UpdateSnapshotsDialog();
}

static project_config_extension_t g_projectconfig = { ProcessExtensionLine, SaveExtensionConfig, BeginLoadProjectState, NULL };

static void menuhook(const char* menustr, HMENU hMenu, int flag)
{
	if (strcmp(menustr, "Track control panel context") == 0 && flag == 0)
	{
		AddToMenu(hMenu, SWS_SEPARATOR, 0);
		AddSubMenu(hMenu, SWSCreateMenuFromCommandTable(g_commandTable), __LOCALIZE("SWS Snapshots","sws_DLG_101"));
	}
}

int SnapshotsInit()
{
	if (!plugin_register("projectconfig",&g_projectconfig))
		return 0;

	SWSRegisterCommands(g_commandTable);

	if (!plugin_register("hookcustommenu", (void*)menuhook))
		return 0;

	// DEPRECATED: DefaultNbSnapsRecall is replaced by [NbOfActions] in S&M.ini
	// This migrates the old setting to the new one (SNM_Init is called later)
	const int nbrecall = GetPrivateProfileInt(SWS_INI, "DefaultNbSnapsRecall", -1, get_ini_file());
	if (nbrecall >= 0)
		FindDynamicAction(GetSnapshot)->count = nbrecall;

	g_pSSWnd = new SWS_SnapshotsWnd;

	// disable features unavailable in the running version of REAPER
	if (atof(GetAppVersion()) < 6)
		g_compatMask &= ~PLAY_OFFSET_MASK;

	return 1;
}

void RegisterSnapshotSlot(const int slot) // slot is a 1-based index
{
	static int lastRegistered = 0;
	static const DYN_COMMAND_T *cmd = FindDynamicAction(GetSnapshot);

	if (slot <= lastRegistered)
		return;

	if (slot > cmd->count) // only register new slots above the configured amount
		cmd->Register(slot - 1);

	lastRegistered = slot;
}

void SnapshotsExit()
{
	plugin_register("-projectconfig",&g_projectconfig);
	plugin_register("-hookcustommenu", (void*)menuhook);

	// deletes the old setting key (see SnapshotsInit)
	WritePrivateProfileString(SWS_INI, "DefaultNbSnapsRecall", nullptr, get_ini_file());

	delete g_pSSWnd;
}
