/******************************************************************************
/ SnapshotMerge.cpp
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
#include "SnapshotMerge.h"
#include "../Prompt.h"

#define MERGE_STATE_KEY		"MergeState"
#define MERGEWND_POS_KEY	"MergeWndPos"

#define CREATETRACK ((MediaTrack*)1)

static Snapshot* g_ss = NULL;
static WDL_PtrList<SWS_SSMergeItem> g_mergeItems;
static int g_iMask;

static SWS_LVColumn g_cols[] = { { 120, 0, "Source track" }, { 120, 0, "Destination" }, };

SWS_SnapshotMergeView::SWS_SnapshotMergeView(HWND hwndList, HWND hwndEdit)
:SWS_ListView(hwndList, hwndEdit, 2, g_cols, "MergeViewState", false)
{
}

int SWS_SnapshotMergeView::OnItemSort(LPARAM lParam1, LPARAM lParam2)
{
	SWS_SSMergeItem* item1 = (SWS_SSMergeItem*)lParam1;
	SWS_SSMergeItem* item2 = (SWS_SSMergeItem*)lParam2;
	int iRet = 0;

	switch (abs(m_iSortCol))
	{
	case 1: // Source
		if (item1->m_iIndex > item2->m_iIndex)
			iRet = 1;
		else if (item1->m_iIndex < item2->m_iIndex)
			iRet = -1;
		break;
	case 2: // Dest
		if (CSurf_TrackToID(item1->m_destTr, false) > CSurf_TrackToID(item2->m_destTr, false))
			iRet = 1;
		else if (CSurf_TrackToID(item1->m_destTr, false) < CSurf_TrackToID(item2->m_destTr, false))
			iRet = -1;
		break;
	}

	if (m_iSortCol < 0)
		return -iRet;
	else
		return iRet;
}

void SWS_SnapshotMergeView::GetItemText(LPARAM item, int iCol, char* str, int iStrMax)
{
	SWS_SSMergeItem* mi = (SWS_SSMergeItem*)item;
	str[0] = 0;

	if (!mi)
		return;

	switch(iCol)
	{
	case 0:
		if (memcmp(&mi->m_ts->m_guid, &GUID_NULL, sizeof(GUID)) == 0)
			lstrcpyn(str, "(master)", iStrMax);
		else if (mi->m_ts->m_sName.GetLength())
			_snprintf(str, iStrMax, "%d: %s", mi->m_iIndex, mi->m_ts->m_sName.Get());
		else
			_snprintf(str, iStrMax, "%d", mi->m_iIndex);
		break;
	case 1:
		{
			int iTrack = CSurf_TrackToID(mi->m_destTr, false);
			if (iTrack == 0)
				lstrcpyn(str, "(master)", iStrMax);
			else if (iTrack > 0)
			{
				char* cName = (char*)GetSetMediaTrackInfo(mi->m_destTr, "P_NAME", NULL);
				if (cName && cName[0])
					_snprintf(str, iStrMax, "%d: %s", iTrack, cName);
				else
					_snprintf(str, iStrMax, "%d", iTrack);
			}
			else if (mi->m_destTr == NULL)
				lstrcpyn(str, "(none)", iStrMax);
			else if (mi->m_destTr == CREATETRACK)
				lstrcpyn(str, "(create new)", iStrMax);
			break;
		}
	}
}

void SWS_SnapshotMergeView::GetItemList(WDL_TypedBuf<LPARAM>* pBuf)
{
	pBuf->Resize(g_mergeItems.GetSize());
	for (int i = 0; i < g_mergeItems.GetSize(); i++)
		pBuf->Get()[i] = (LPARAM)g_mergeItems.Get(i);
}

INT_PTR WINAPI mergeWndProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static WDL_WndSizer resize;
	static SWS_SnapshotMergeView* mv = NULL;
	switch (uMsg)
	{
		case WM_INITDIALOG:
			resize.init(hwndDlg);
			resize.init_item(IDC_LIST);
			resize.init_item(IDC_ADD, 0.0, 1.0, 0.0, 1.0);
			resize.init_item(IDC_REMOVE, 0.0, 1.0, 0.0, 1.0);
			resize.init_item(IDCANCEL, 1.0, 1.0, 1.0, 1.0);
			resize.init_item(IDOK, 1.0, 1.0, 1.0, 1.0);
			resize.init_item(IDC_FILTERGROUP, 1.0, 0.0, 1.0, 0.0);
			for (int i = 0; i < MASK_CTRLS; i++)
			{
				resize.init_item(cSSCtrls[i], 1.0, 0.0, 1.0, 0.0);
				if (!(g_iMask & cSSMasks[i]))
					EnableWindow(GetDlgItem(hwndDlg, cSSCtrls[i]), false);
				else
					CheckDlgButton(hwndDlg, cSSCtrls[i], BST_CHECKED);
			}
			RestoreWindowPos(hwndDlg, MERGEWND_POS_KEY);

			mv = new SWS_SnapshotMergeView(GetDlgItem(hwndDlg, IDC_LIST), NULL);
			mv->Update();
			
			return 0;
		case WM_CONTEXTMENU:
		{
			// 
			int x = LOWORD(lParam), y = HIWORD(lParam);
			if (mv->DoColumnMenu(x, y))
				return 0;

			int iCol;
			SWS_SSMergeItem* mi = (SWS_SSMergeItem*)mv->GetHitItem(x, y, &iCol);
			if (mi && (iCol == 0 || iCol == 1))
			{
				HMENU contextMenu = CreatePopupMenu();

				AddToMenu(contextMenu, "Examine track snapshot", 1000);
				AddToMenu(contextMenu, SWS_SEPARATOR, 0);

				if (iCol == 0)
				{
					AddToMenu(contextMenu, "Select source track", 0);
					EnableMenuItem(contextMenu, 2, MF_BYPOSITION | MF_GRAYED);

					char menuText[80];
					for (int i = 0; i < g_ss->m_tracks.GetSize(); i++)
					{
						TrackSnapshot* ts = g_ss->m_tracks.Get(i);
						if (memcmp(&ts->m_guid, &GUID_NULL, sizeof(GUID)) == 0)
							strcpy(menuText, "(master)");
						else if (ts->m_sName.GetLength())
							_snprintf(menuText, 80, "%d: %s", i, ts->m_sName.Get());
						else
							sprintf(menuText, "%d", i);

						AddToMenu(contextMenu, menuText, i+1);
					}
				}
				else // iCol == 1
				{
					AddToMenu(contextMenu, "Select destination track", 0);
					EnableMenuItem(contextMenu, 2, MF_BYPOSITION | MF_GRAYED);

					if (memcmp(&mi->m_ts->m_guid, &GUID_NULL, sizeof(GUID)))
						AddToMenu(contextMenu, "(create new)", 1);
					AddToMenu(contextMenu, "(none)", 2);
					AddToMenu(contextMenu, "(master)", 3);
					char menuText[80];
					for (int i = 1; i <= GetNumTracks(); i++)
					{
						char* cName = (char*)GetSetMediaTrackInfo(CSurf_TrackFromID(i, false), "P_NAME", NULL);
						if (cName && cName[0])
							_snprintf(menuText, 80, "%d: %s", i, cName);
						else
							sprintf(menuText, "%d", i);
						AddToMenu(contextMenu, menuText, i+3);
					}
				}

				int iCmd = TrackPopupMenu(contextMenu, TPM_RETURNCMD, x, y, 0, hwndDlg, NULL);
				if (iCmd > 0)
				{
					if (iCmd == 1000)
					{
						WDL_String details;
						mi->m_ts->GetDetails(&details, g_ss->m_iMask);
						DisplayInfoBox(hwndDlg, "Track Snapshot Details", details.Get());
					}
					else if (iCol == 0)
					{
						mi->m_ts = g_ss->m_tracks.Get(iCmd-1);
						mi->m_iIndex = iCmd-1;
					}
					else // iCol == 1
					{
						if (iCmd == 1)
							mi->m_destTr = CREATETRACK;
						else if (iCmd == 2)
							mi->m_destTr = NULL;
						else
							mi->m_destTr = CSurf_TrackFromID(iCmd-3, false);
					}
				}

				DestroyMenu(contextMenu);
				mv->Update();
			}
			break;
		}
		case WM_NOTIFY:
		{
			NMHDR* hdr = (NMHDR*)lParam;
			if (hdr->hwndFrom == mv->GetHWND())
				return mv->OnNotify(wParam, lParam);
			break;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDC_ADD:
				{
					// Add a line, using same as 1) first sel, 2) matching index in snapshot 3) last in list
					SWS_SSMergeItem* item = (SWS_SSMergeItem*)mv->GetFirstSelected();
					if (!item && g_mergeItems.GetSize() < g_ss->m_tracks.GetSize())
					{
						item = g_mergeItems.Add(new SWS_SSMergeItem(g_ss->m_tracks.Get(g_mergeItems.GetSize()), g_mergeItems.GetSize(), NULL));
						item->m_destTr = GuidToTrack(&item->m_ts->m_guid);
					}
					else
					{
						if (!item)
							item = g_mergeItems.Get(g_mergeItems.GetSize()-1);
						g_mergeItems.Add(new SWS_SSMergeItem(item->m_ts, item->m_iIndex, item->m_destTr));
					}
					mv->Update();
					break;
				}
			case IDC_REMOVE:
				for (int i = 0; i < ListView_GetItemCount(mv->GetHWND()); i++)
					if (mv->IsSelected(i))
						g_mergeItems.Delete(g_mergeItems.Find((SWS_SSMergeItem*)mv->GetListItem(i)), true);
				mv->Update();
				break;
			case IDOK:
				{
					// See if there's duplicate destination tracks
					for (int i = 0; i < g_mergeItems.GetSize()-1; i++)
						if (g_mergeItems.Get(i)->m_destTr && g_mergeItems.Get(i)->m_destTr != CREATETRACK)
							for (int j = i+1; j < g_mergeItems.GetSize(); j++)
								if (g_mergeItems.Get(i)->m_destTr == g_mergeItems.Get(j)->m_destTr)
								{
									MessageBox(hwndDlg, "Cannot have multiple sources with the same destination!", "Snapshot Recall Error", MB_OK);
									break;
								}
					
					// Update the snapshot and "recall it"
					// 1) Save the existing snapshot's tracks
					WDL_PtrList<TrackSnapshot> oldTrackSS;
					for (int i = 0; i < g_ss->m_tracks.GetSize(); i++)
						oldTrackSS.Add(g_ss->m_tracks.Get(i));
					g_ss->m_tracks.Empty(false);

					// 2) Create new TrackSnapshot tracks from the user's selections
					for (int i = 0; i < g_mergeItems.GetSize(); i++)
					{
						if (g_mergeItems.Get(i)->m_destTr)
						{
							if (g_mergeItems.Get(i)->m_destTr == CREATETRACK)
							{
								int iNewTrack = GetNumTracks()+1;
								InsertTrackAtIndex(iNewTrack, false);
								g_mergeItems.Get(i)->m_destTr = CSurf_TrackFromID(iNewTrack, false);
							}

							TrackSnapshot* ts = g_ss->m_tracks.Add(new TrackSnapshot(*g_mergeItems.Get(i)->m_ts));
							if (CSurf_TrackToID(g_mergeItems.Get(i)->m_destTr, false) == 0)
								ts->m_guid = GUID_NULL;
							else
								ts->m_guid = *(GUID*)GetSetMediaTrackInfo(g_mergeItems.Get(i)->m_destTr, "GUID", NULL);
						}
					}

					oldTrackSS.Empty(true);
					g_ss->UpdateReaper(g_iMask, false, false);				
					// fall through!
				}
			case IDCANCEL:
				SaveWindowPos(hwndDlg, MERGEWND_POS_KEY);
				EndDialog(hwndDlg,0);
				break;
			default:
				for (int i = 0; i < MASK_CTRLS; i++)
					if (wParam == cSSCtrls[i])
					{
						g_iMask = 0;
						for (int i = 0; i < MASK_CTRLS; i++)
							if (IsDlgButtonChecked(hwndDlg, cSSCtrls[i]) == BST_CHECKED)
								g_iMask |= cSSMasks[i];
						mv->Update();
					}
			}
			return 0;
		case WM_SIZE:
			if (wParam != SIZE_MINIMIZED)
				resize.onResize();
			break;
		case WM_DESTROY:
			mv->OnDestroy();
			delete mv;
			mv = NULL;
			g_mergeItems.Empty(true);
			break;
	}
	return 0;
}

void MergeSnapshots(Snapshot* ss)
{
	g_ss = ss;

	if (!ss->m_tracks.GetSize())
		return;

	g_iMask = ss->m_iMask;
	
	// For matching of names
	WDL_PtrList<void> projTracks;
	for (int i = 1; i <= GetNumTracks(); i++)
		projTracks.Add(CSurf_TrackFromID(i, false));

	for (int i = 0; i < ss->m_tracks.GetSize(); i++)
	{
		SWS_SSMergeItem* mi = g_mergeItems.Add(new SWS_SSMergeItem(ss->m_tracks.Get(i), i, NULL));
		mi->m_destTr = GuidToTrack(&mi->m_ts->m_guid);
		// First "match" is with the GUID, try the name next
		if (!mi->m_destTr && mi->m_ts->m_sName.GetLength())
		{
			// Try name matching
			for (int j = 0; j < projTracks.GetSize(); j++)
				if (strcmp((char*)GetSetMediaTrackInfo((MediaTrack*)projTracks.Get(j), "P_NAME", NULL), mi->m_ts->m_sName.Get()) == 0)
				{
					mi->m_destTr = (MediaTrack*)projTracks.Get(j);
					projTracks.Delete(j);
					break;
				}
		}
	}

	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_SSMERGE), g_hwndParent, mergeWndProc);
}
