/******************************************************************************
/ SnapshotMerge.cpp
/
/ Copyright (c) 2010 Tim Payne (SWS)
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

#include "../SnM/SnM_Dlg.h"
#include "SnapshotClass.h"
#include "SnapshotMerge.h"
#include "../Prompt.h"

#include <WDL/localize/localize.h>

#define MERGE_STATE_KEY		"MergeState"
#define MERGEWND_POS_KEY	"MergeWndPos"

#define CREATETRACK ((MediaTrack*)1)

static Snapshot* g_ss = NULL;
static WDL_PtrList<SWS_SSMergeItem> g_mergeItems;
static int g_iMask;
static bool g_bSave = false;

// !WANT_LOCALIZE_STRINGS_BEGIN:sws_DLG_112
static SWS_LVColumn g_cols[] = { { 120, 0, "Source track" }, { 120, 0, "Destination" }, };
// !WANT_LOCALIZE_STRINGS_END

SWS_SnapshotMergeView::SWS_SnapshotMergeView(HWND hwndList, HWND hwndEdit)
:SWS_ListView(hwndList, hwndEdit, 2, g_cols, "MergeViewState", false, "sws_DLG_112")
{
}

int SWS_SnapshotMergeView::OnItemSort(SWS_ListItem* lParam1, SWS_ListItem* lParam2)
{
	SWS_SSMergeItem* item1 = (SWS_SSMergeItem*)lParam1;
	SWS_SSMergeItem* item2 = (SWS_SSMergeItem*)lParam2;
	int iRet = 0;

	switch (abs(m_iSortCol))
	{
	case 1: // Source
		if (item1->m_ts->m_iTrackNum > item2->m_ts->m_iTrackNum)
			iRet = 1;
		else if (item1->m_ts->m_iTrackNum < item2->m_ts->m_iTrackNum)
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

void SWS_SnapshotMergeView::GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax)
{
	SWS_SSMergeItem* mi = (SWS_SSMergeItem*)item;
	str[0] = 0;

	if (!mi)
		return;

	switch(iCol)
	{
	case 0:
		if (GuidsEqual(&mi->m_ts->m_guid, &GUID_NULL))
			lstrcpyn(str, __LOCALIZE("(master)","sws_DLG_112"), iStrMax);
		else if (mi->m_ts->m_sName.GetLength())
			snprintf(str, iStrMax, "%d: %s", mi->m_ts->m_iTrackNum, mi->m_ts->m_sName.Get());
		else
			snprintf(str, iStrMax, "%d", mi->m_ts->m_iTrackNum);
		break;
	case 1:
		{
			int iTrack = CSurf_TrackToID(mi->m_destTr, false);
			if (iTrack == 0)
				lstrcpyn(str, __LOCALIZE("(master)","sws_DLG_112"), iStrMax);
			else if (iTrack > 0)
			{
				char* cName = (char*)GetSetMediaTrackInfo(mi->m_destTr, "P_NAME", NULL);
				if (cName && cName[0])
					snprintf(str, iStrMax, "%d: %s", iTrack, cName);
				else
					snprintf(str, iStrMax, "%d", iTrack);
			}
			else if (mi->m_destTr == NULL)
				lstrcpyn(str, __LOCALIZE("(none)","sws_DLG_112"), iStrMax);
			else if (mi->m_destTr == CREATETRACK)
				lstrcpyn(str, __LOCALIZE("(create new)","sws_DLG_112"), iStrMax);
			break;
		}
	}
}

void SWS_SnapshotMergeView::GetItemList(SWS_ListItemList* pList)
{
	for (int i = 0; i < g_mergeItems.GetSize(); i++)
		pList->Add((SWS_ListItem*)g_mergeItems.Get(i));
}

INT_PTR WINAPI mergeWndProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static WDL_WndSizer resize;
	static SWS_SnapshotMergeView* mv = NULL;

	if (INT_PTR r = SNM_HookThemeColorsMessage(hwndDlg, uMsg, wParam, lParam))
		return r;

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
			resize.init_item(IDC_SAVE, 1.0, 0.0, 1.0, 0.0);
			resize.init_item(IDC_SNAME, 1.0, 0.0, 1.0, 0.0);
			resize.init_item(IDC_NAME, 1.0, 0.0, 1.0, 0.0);
			resize.init_item(IDC_UPDATE, 1.0, 0.0, 1.0, 0.0);

			EnableWindow(GetDlgItem(hwndDlg, IDC_NAME), false);
			SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_NAME), GWLP_USERDATA, 0xdeadf00b);
			SetDlgItemText(hwndDlg, IDC_NAME, g_ss->m_cName);
			EnableWindow(GetDlgItem(hwndDlg, IDC_UPDATE), false);
			CheckDlgButton(hwndDlg, IDC_UPDATE, BST_CHECKED);
			g_bSave = false;

			RestoreWindowPos(hwndDlg, MERGEWND_POS_KEY);

			mv = new SWS_SnapshotMergeView(GetDlgItem(hwndDlg, IDC_LIST), NULL);
			mv->Update();
			
			return 0;
		case WM_CONTEXTMENU:
		{
			int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
			if (mv->DoColumnMenu(x, y))
				return 0;

			int iCol;
			SWS_SSMergeItem* mi = (SWS_SSMergeItem*)mv->GetHitItem(x, y, &iCol);
			if (mi && (iCol == 0 || iCol == 1))
			{
				HMENU contextMenu = CreatePopupMenu();

				AddToMenu(contextMenu, __LOCALIZE("Examine track snapshot","sws_DLG_112"), 1000);
				AddToMenu(contextMenu, SWS_SEPARATOR, 0);

				if (iCol == 0)
				{
					AddToMenu(contextMenu, __LOCALIZE("Select source track","sws_DLG_112"), 0, -1, false, MF_GRAYED);

					char menuText[80];
					for (int i = 0; i < g_ss->m_tracks.GetSize(); i++)
					{
						TrackSnapshot* ts = g_ss->m_tracks.Get(i);
						if (GuidsEqual(&ts->m_guid, &GUID_NULL))
							strcpy(menuText, __LOCALIZE("(master)","sws_DLG_112"));
						else if (ts->m_sName.GetLength())
							snprintf(menuText, 80, "%d: %s", i, ts->m_sName.Get());
						else
							sprintf(menuText, "%d", i);

						AddToMenu(contextMenu, menuText, i+1);
					}
				}
				else // iCol == 1
				{
					AddToMenu(contextMenu, __LOCALIZE("Select destination track","sws_DLG_112"), 0, -1, false, MF_GRAYED);
					AddToMenu(contextMenu, __LOCALIZE("(create new)","sws_DLG_112"), 1);
					AddToMenu(contextMenu, __LOCALIZE("(none)","sws_DLG_112"), 2);
					AddToMenu(contextMenu, __LOCALIZE("(master)","sws_DLG_112"), 3);
					char menuText[80];
					for (int i = 1; i <= GetNumTracks(); i++)
					{
						char* cName = (char*)GetSetMediaTrackInfo(CSurf_TrackFromID(i, false), "P_NAME", NULL);
						if (cName && cName[0])
							snprintf(menuText, 80, "%d: %s", i, cName);
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
						WDL_FastString details;
						mi->m_ts->GetDetails(&details, g_ss->m_iMask);
						DisplayInfoBox(hwndDlg, __LOCALIZE("Track Snapshot Details","sws_DLG_112"), details.Get());
					}
					else if (iCol == 0)
					{
						for (int i = 0; i < ListView_GetItemCount(mv->GetHWND()); i++)
							if (mv->IsSelected(i))
							{
								mi = (SWS_SSMergeItem*)mv->GetListItem(i);
								mi->m_ts = g_ss->m_tracks.Get(iCmd-1);
							}
					}
					else // iCol == 1
					{
						for (int i = 0; i < ListView_GetItemCount(mv->GetHWND()); i++)
							if (mv->IsSelected(i))
							{
								mi = (SWS_SSMergeItem*)mv->GetListItem(i);
								if (iCmd == 1)
									mi->m_destTr = CREATETRACK;
								else if (iCmd == 2)
									mi->m_destTr = NULL;
								else
									mi->m_destTr = CSurf_TrackFromID(iCmd-3, false);
							}
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
			if (mv && hdr->hwndFrom == mv->GetHWND())
				return mv->OnNotify(wParam, lParam);
			break;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDC_ADD:
				{
					// Add a line, using same as 1) first sel, 2) matching index in snapshot 3) last in list
					SWS_SSMergeItem* item = (SWS_SSMergeItem*)mv->EnumSelected(NULL);
					if (!item && g_mergeItems.GetSize() < g_ss->m_tracks.GetSize())
					{
						item = g_mergeItems.Add(new SWS_SSMergeItem(g_ss->m_tracks.Get(g_mergeItems.GetSize()), NULL));
						item->m_destTr = GuidToTrack(&item->m_ts->m_guid);
					}
					else
					{
						if (!item)
							item = g_mergeItems.Get(g_mergeItems.GetSize()-1);
						g_mergeItems.Add(new SWS_SSMergeItem(item->m_ts, item->m_destTr));
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
			case IDC_SAVE:
			{
				bool bEnable = IsDlgButtonChecked(hwndDlg, IDC_SAVE) ? true : false;
				EnableWindow(GetDlgItem(hwndDlg, IDC_NAME), bEnable);
				EnableWindow(GetDlgItem(hwndDlg, IDC_UPDATE), bEnable);
				if (bEnable)
				{
					SetFocus(GetDlgItem(hwndDlg, IDC_NAME));
					SendMessage(GetDlgItem(hwndDlg, IDC_NAME), EM_SETSEL, 0, -1);
				}
				else
				{
					CheckDlgButton(hwndDlg, IDC_UPDATE, BST_CHECKED);
				}
				break;
			}
			case IDOK:
				{
					for (int i = 0; i < g_mergeItems.GetSize()-1; i++)
					{
						// See if there's duplicate destination tracks
						if (g_mergeItems.Get(i)->m_destTr && g_mergeItems.Get(i)->m_destTr != CREATETRACK)
							for (int j = i+1; j < g_mergeItems.GetSize(); j++)
								if (g_mergeItems.Get(i)->m_destTr == g_mergeItems.Get(j)->m_destTr)
								{
									MessageBox(hwndDlg, __LOCALIZE("Cannot have multiple sources with the same destination!","sws_DLG_112"), __LOCALIZE("Snapshot Recall Error","sws_DLG_112"), MB_OK);
									return 0;
								}
					}
					
					// Create a new snapshot from the selection
					// Update the snapshot and "recall it"
					// 1) Save the existing snapshot's tracks
					WDL_PtrList<TrackSnapshot> oldTrackSS;
					for (int i = 0; i < g_ss->m_tracks.GetSize(); i++)
						oldTrackSS.Add(g_ss->m_tracks.Get(i));
					g_ss->m_tracks.Empty();

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
								GetSetMediaTrackInfo(g_mergeItems.Get(i)->m_destTr, "P_NAME", (void*)g_mergeItems.Get(i)->m_ts->m_sName.Get());
							}

							TrackSnapshot* ts = g_ss->m_tracks.Add(new TrackSnapshot(*g_mergeItems.Get(i)->m_ts));
							if (CSurf_TrackToID(g_mergeItems.Get(i)->m_destTr, false) == 0)
								ts->m_guid = GUID_NULL;
							else
								ts->m_guid = *(GUID*)GetSetMediaTrackInfo(g_mergeItems.Get(i)->m_destTr, "GUID", NULL);
						}
					}

					// Now go through and try to auto-resolve any send-track issues 
					for (int i = 0; i < g_ss->m_tracks.GetSize(); i++)
					{
						WDL_PtrList<TrackSend>* pSends = &g_ss->m_tracks.Get(i)->m_sends.m_sends;
						for (int j = 0; j < pSends->GetSize(); j++)
							if (!GuidToTrack(pSends->Get(j)->GetGuid()))
							{	// Perhaps the recv was in the snapshot and the user matched it with a different track?
								int iMatches = 0;
								MediaTrack* pDest = NULL;
								for (int k = 0; k < g_mergeItems.GetSize(); k++)
									if (GuidsEqual(&g_mergeItems.Get(k)->m_ts->m_guid, pSends->Get(j)->GetGuid()))
									{
										pDest = g_mergeItems.Get(k)->m_destTr;
										iMatches++;
									}
								if (iMatches == 1 && pDest) // Must be an exact 1:1 match
									pSends->Get(j)->SetGuid((GUID*)GetSetMediaTrackInfo(pDest, "GUID", NULL));
							}
					}

					// Check options
					// Set name before update so undo str is correct
					if(IsDlgButtonChecked(hwndDlg, IDC_SAVE) == BST_CHECKED)
					{
						char cName[80];
						GetDlgItemText(hwndDlg, IDC_NAME, cName, 80);
						g_ss->SetName(cName);
						g_bSave = true;
					}
					// Update reaper if necessary
					if (IsDlgButtonChecked(hwndDlg, IDC_UPDATE) == BST_CHECKED)
					{
						g_ss->UpdateReaper(g_iMask, false, false);
						// In case tracks were added, update everything
						TrackList_AdjustWindows(false);
						UpdateTimeline();
					}

					// Restore snapshot's tracks if we're not saving, else delete the old
					if (IsDlgButtonChecked(hwndDlg, IDC_SAVE) != BST_CHECKED)
					{
						g_ss->m_tracks.Empty(true);
						for (int i = 0; i < oldTrackSS.GetSize(); i++)
							g_ss->m_tracks.Add(oldTrackSS.Get(i));
					}
					else
						oldTrackSS.Empty(true);

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
			resize.init(NULL);
			break;
	}
	return 0;
}

bool MergeSnapshots(Snapshot* ss)
{
	g_ss = ss;

	if (!ss || !ss->m_tracks.GetSize())
		return false;

	g_iMask = ss->m_iMask;

	// If the paste is occuring with matching selected tracks, use those
	if (ss->m_tracks.GetSize() == CountSelectedTracks(NULL))
	{
		for (int i = 0; i < ss->m_tracks.GetSize(); i++)
			g_mergeItems.Add(new SWS_SSMergeItem(ss->m_tracks.Get(i), GetSelectedTrack(NULL, i)));
	}
	// If there's no tracks, by default set to "create"
	else if (GetNumTracks() == 0)
	{
		for (int i = 0; i < ss->m_tracks.GetSize(); i++)
			g_mergeItems.Add(new SWS_SSMergeItem(ss->m_tracks.Get(i), CREATETRACK));
	}
	else
	{	// Next try matching with GUIDS
		WDL_PtrList<void> projTracks;
		for (int i = 1; i <= GetNumTracks(); i++)
			projTracks.Add(CSurf_TrackFromID(i, false));

		// First try to "match" with the GUID  (and fill g_mergeItems)
		for (int i = 0; i < ss->m_tracks.GetSize(); i++)
		{
			SWS_SSMergeItem* mi = g_mergeItems.Add(new SWS_SSMergeItem(ss->m_tracks.Get(i), NULL));
			// check for master first
			if (GuidsEqual(&mi->m_ts->m_guid, &GUID_NULL))
				mi->m_destTr = CSurf_TrackFromID(0, false);
			else for (int j = 0; j < projTracks.GetSize(); j++)
			{
				if (TrackMatchesGuid((MediaTrack*)projTracks.Get(j), &mi->m_ts->m_guid))
				{
					mi->m_destTr = (MediaTrack*)projTracks.Get(j);
					projTracks.Delete(j);
					break;
				}
			}
		}

		// Next do name matching
		for (int i = 0; i < g_mergeItems.GetSize(); i++)
		{
			SWS_SSMergeItem* mi = g_mergeItems.Get(i);
			if (!mi->m_destTr)
			{
				for (int j = 0; j < projTracks.GetSize(); j++)
					if (!strncmp((char*)GetSetMediaTrackInfo((MediaTrack*)projTracks.Get(j), "P_NAME", NULL), mi->m_ts->m_sName.Get(), mi->m_ts->m_sName.GetLength()))
					{
						mi->m_destTr = (MediaTrack*)projTracks.Get(j);
						projTracks.Delete(j);
						break;
					}
			}
		}

		// Per Issue 577, prefer (none) over just picking random tracks. (OK, not random, in order)
		//  I agree, not sure what I was thinking when I wrote the below. TRP 7/9/13
		// Fill in blanks with whatever tracks exist
		//for (int i = 0; projTracks.GetSize() && i < g_mergeItems.GetSize(); i++)
		//	if (!g_mergeItems.Get(i)->m_destTr)
		//	{
		//		g_mergeItems.Get(i)->m_destTr = (MediaTrack*)projTracks.Get(0);
		//		projTracks.Delete(0);
		//	}
	}

	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_SSMERGE), g_hwndParent, mergeWndProc);

	return g_bSave;
}
