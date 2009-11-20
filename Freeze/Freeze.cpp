/******************************************************************************
/ Freeze.cpp
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
#include "TrackItemState.h"
#include "ItemSelState.h"
#include "MuteState.h"
#include "ActiveTake.h"
#include "TimeState.h"
#include "../ObjectState/TrackEnvelope.h"

//#define TESTCODE

static int g_prevautoxfade = 0;

void SaveXFade(COMMAND_T* = NULL)    { if (GetConfigVar("autoxfade")) g_prevautoxfade = *(int*)GetConfigVar("autoxfade"); }
void RestoreXFade(COMMAND_T* = NULL) { if (GetConfigVar("autoxfade") && g_prevautoxfade != *(int*)GetConfigVar("autoxfade")) Main_OnCommand(40041, 0); }
void XFadeOn(COMMAND_T* = NULL)      { int* p = (int*)GetConfigVar("autoxfade"); if (p && !(*p)) Main_OnCommand(40041, 0); }
void XFadeOff(COMMAND_T* = NULL)     { int* p = (int*)GetConfigVar("autoxfade"); if (p && (*p))  Main_OnCommand(40041, 0); }
void MEPWIXOn(COMMAND_T* = NULL)     { int* p = (int*)GetConfigVar("envattach"); if (p && !(*p)) Main_OnCommand(40070, 0); }
void MEPWIXOff(COMMAND_T* = NULL)    { int* p = (int*)GetConfigVar("envattach"); if (p && (*p))  Main_OnCommand(40070, 0); }

void UnselOnTracks(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
				GetSetMediaItemInfo(GetTrackMediaItem(tr, j), "B_UISEL", &g_bFalse);
	}
	UpdateTimeline();
}

void SelLLItem(COMMAND_T* = NULL)
{
	UnselOnTracks();
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		int iNumItems = GetTrackNumMediaItems(tr);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL) && iNumItems)
		{
			MediaItem* pLL = GetTrackMediaItem(tr, 0);
			double dFirstPos = *(double*)GetSetMediaItemInfo(pLL, "D_POSITION", NULL);
			float fFIPMy = *(float*)GetSetMediaItemInfo(pLL, "F_FREEMODE_Y", NULL);
			for (int j = 1; j < iNumItems; j++)
			{
				if (dFirstPos != *(double*)GetSetMediaItemInfo(GetTrackMediaItem(tr, j), "D_POSITION", NULL))
					break;
				if (fFIPMy < *(float*)GetSetMediaItemInfo(GetTrackMediaItem(tr, j), "F_FREEMODE_Y", NULL))
				{
					pLL = GetTrackMediaItem(tr, j);
					fFIPMy = *(float*)GetSetMediaItemInfo(pLL, "F_FREEMODE_Y", NULL);
				}
			}
			GetSetMediaItemInfo(pLL, "B_UISEL", &g_bTrue);
		}
	}
	UpdateTimeline();
}

void SelULItem(COMMAND_T* = NULL)
{
	UnselOnTracks();
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		int iNumItems = GetTrackNumMediaItems(tr);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL) && iNumItems)
		{
			MediaItem* pUL = GetTrackMediaItem(tr, 0);
			double dFirstPos = *(double*)GetSetMediaItemInfo(pUL, "D_POSITION", NULL);
			float fFIPMy = *(float*)GetSetMediaItemInfo(pUL, "F_FREEMODE_Y", NULL);
			for (int j = 1; j < iNumItems; j++)
			{
				if (dFirstPos != *(double*)GetSetMediaItemInfo(GetTrackMediaItem(tr, j), "D_POSITION", NULL))
					break;
				if (fFIPMy > *(float*)GetSetMediaItemInfo(GetTrackMediaItem(tr, j), "F_FREEMODE_Y", NULL))
				{
					pUL = GetTrackMediaItem(tr, j);
					fFIPMy = *(float*)GetSetMediaItemInfo(pUL, "F_FREEMODE_Y", NULL);
				}
			}
			GetSetMediaItemInfo(pUL, "B_UISEL", &g_bTrue);
		}
	}
	UpdateTimeline();
}

void UnselULItem(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		int iNumItems = GetTrackNumMediaItems(tr);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL) && iNumItems)
		{
			MediaItem* pUL = GetTrackMediaItem(tr, 0);
			double dFirstPos = *(double*)GetSetMediaItemInfo(pUL, "D_POSITION", NULL);
			float fFIPMy = *(float*)GetSetMediaItemInfo(pUL, "F_FREEMODE_Y", NULL);
			for (int j = 1; j < iNumItems; j++)
			{
				if (dFirstPos != *(double*)GetSetMediaItemInfo(GetTrackMediaItem(tr, j), "D_POSITION", NULL))
					break;
				if (fFIPMy > *(float*)GetSetMediaItemInfo(GetTrackMediaItem(tr, j), "F_FREEMODE_Y", NULL))
				{
					pUL = GetTrackMediaItem(tr, j);
					fFIPMy = *(float*)GetSetMediaItemInfo(pUL, "F_FREEMODE_Y", NULL);
				}
			}
			GetSetMediaItemInfo(pUL, "B_UISEL", &g_bFalse);
		}
	}
	UpdateTimeline();
}

void MuteRecvs(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			for (int j = 0; GetSetTrackSendInfo(tr, -1, j, "P_SRCTRACK", NULL); j++)
				GetSetTrackSendInfo(tr, -1, j, "B_MUTE", &g_bTrue);
	}
}

void UnmuteRecvs(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			for (int j = 0; GetSetTrackSendInfo(tr, -1, j, "P_SRCTRACK", NULL); j++)
				GetSetTrackSendInfo(tr, -1, j, "B_MUTE", &g_bFalse);
	}
}

void TogMuteRecvs(COMMAND_T* = NULL)
{
	bool bMute;
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			for (int j = 0; GetSetTrackSendInfo(tr, -1, j, "P_SRCTRACK", NULL); j++)
			{
				bMute = !*(bool*)GetSetTrackSendInfo(tr, -1, j, "B_MUTE", NULL);
				GetSetTrackSendInfo(tr, -1, j, "B_MUTE", &bMute);
			}
	}
}

static WDL_PtrList<GUID> g_pSelTracks;
void SaveSelTracks(COMMAND_T* = NULL)
{
	g_pSelTracks.Empty(false);
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			g_pSelTracks.Add((GUID*)GetSetMediaTrackInfo(tr, "GUID", NULL));
	}
}

void RestoreSelTracks(COMMAND_T* = NULL)
{
	ClearSelected();
	for (int i = 0; i < g_pSelTracks.GetSize(); i++)
		for (int j = 0; j <= GetNumTracks(); j++)
		{
			MediaTrack* tr = CSurf_TrackFromID(j, false);
			if (memcmp(g_pSelTracks.Get(i), GetSetMediaTrackInfo(tr, "GUID", NULL), sizeof(GUID)) == 0)
				GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i1);
		}
}

void GetSelFolderTracks(WDL_PtrList<void>* pParents, WDL_PtrList<void>* pChildren)
{
	MediaTrack* tr;
	WDL_PtrList<void> parentStack;
	int iParentDepth;
	bool bSelected = false;
	MediaTrack* gfd = NULL;
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		tr = CSurf_TrackFromID(i, false);
		int iType;
		int iFolder = GetFolderDepth(tr, &iType, &gfd);

		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			if (pParents && parentStack.GetSize())
			{
				void* p = parentStack.Get(parentStack.GetSize()-1);
				if (pParents->Find(p) == -1)
					pParents->Add(p);
			}
		}

		if (bSelected && pChildren && pChildren->Find(tr) == -1)
			pChildren->Add(tr);

		if (iType == 1)
		{
			if (!bSelected && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			{
				iParentDepth = iFolder;
				bSelected = true;
			}
			parentStack.Add(tr);
		}

		if (iType < 0)
		{
			for (int j = iType; j < 0; j++)
				parentStack.Delete(parentStack.GetSize()-1);

			if (iType + iFolder <= iParentDepth)
				bSelected = false;
		}
	}
}

void SelectFolderTracks(int iSelParent, int iSelChildren, bool bUnsel) // -1 == ignore, 0 == unsel, 1 == sel
{
	WDL_PtrList<void> parents;
	WDL_PtrList<void> children;
	GetSelFolderTracks(&parents, &children);
	if (bUnsel)
		ClearSelected();
	if (iSelParent >= 0)
		for (int i = 0; i < parents.GetSize(); i++)
			GetSetMediaTrackInfo((MediaTrack*)parents.Get(i), "I_SELECTED", iSelParent ? &g_i1 : &g_i0);
	if (iSelChildren >= 0)
		for (int i = 0; i < children.GetSize(); i++)
			GetSetMediaTrackInfo((MediaTrack*)children.Get(i), "I_SELECTED", iSelChildren ? &g_i1 : &g_i0);
}

void SelChildren(COMMAND_T* = NULL)		{ SelectFolderTracks(-1, 1, false); }
void SelChildrenOnly(COMMAND_T* = NULL)	{ SelectFolderTracks(-1, 1, true); }
void SelParents(COMMAND_T* = NULL)		{ SelectFolderTracks(1, -1, false); }
void SelParentsOnly(COMMAND_T* = NULL)	{ SelectFolderTracks(1, -1, true); }
void UnselParents(COMMAND_T* = NULL)	{ SelectFolderTracks(0, -1, false); }
void UnselChildren(COMMAND_T* = NULL)	{ SelectFolderTracks(-1, 0, false); }

void MuteChildren(COMMAND_T* = NULL)
{
	WDL_PtrList<void> children;
	GetSelFolderTracks(NULL, &children);
	for (int i = 0; i < children.GetSize(); i++)
		GetSetMediaTrackInfo((MediaTrack*)children.Get(i), "B_MUTE", &g_bTrue);
}

void UnmuteChildren(COMMAND_T* = NULL)
{
	WDL_PtrList<void> children;
	GetSelFolderTracks(NULL, &children);
	for (int i = 0; i < children.GetSize(); i++)
		GetSetMediaTrackInfo((MediaTrack*)children.Get(i), "B_MUTE", &g_bFalse);
}

void TogMuteChildren(COMMAND_T* = NULL)
{
	bool bMute;
	WDL_PtrList<void> children;
	GetSelFolderTracks(NULL, &children);
	for (int i = 0; i < children.GetSize(); i++)
	{
		bMute = !*(bool*)GetSetMediaTrackInfo((MediaTrack*)children.Get(i), "B_MUTE", NULL);
		GetSetMediaTrackInfo((MediaTrack*)children.Get(i), "B_MUTE", &bMute);
	}
}

void BypassFX(COMMAND_T* = NULL)
{
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			GetSetMediaTrackInfo(tr, "I_FXEN", &g_i0);
	}
}

void UnbypassFX(COMMAND_T* = NULL)
{
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			GetSetMediaTrackInfo(tr, "I_FXEN", &g_i1);
	}
}

void TogItemMute(COMMAND_T* = NULL)
{	// Toggle item's mutes on selected tracks
	bool bMute;
	MediaTrack* tr;
	MediaItem* mi;
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
			{
				mi = GetTrackMediaItem(tr, j);
				bMute = !*(bool*)GetSetMediaItemInfo(mi, "B_MUTE", NULL);
				GetSetMediaItemInfo(mi, "B_MUTE", &bMute);
			}
	}
	UpdateTimeline();
}

void TogItemSel(COMMAND_T* = NULL)
{	// Toggle item's selection states on selected tracks
	bool bSel;
	MediaTrack* tr;
	MediaItem* mi;
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
			{
				mi = GetTrackMediaItem(tr, j);
				bSel = !*(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL);
				GetSetMediaItemInfo(mi, "B_UISEL", &bSel);
			}
	}
	UpdateTimeline();
}

void TogTrackSel(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		GetSetMediaTrackInfo(tr, "I_SELECTED", *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL) ? &g_i0 : &g_i1);
	}
}

void EnableMPSend(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			GetSetMediaTrackInfo(tr, "B_MAINSEND", &g_bTrue);
	}
}
void DisableMPSend(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			GetSetMediaTrackInfo(tr, "B_MAINSEND", &g_bFalse);
	}
}

void TogMPSend(COMMAND_T* = NULL)
{
	bool bMPSend;
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			bMPSend = !*(bool*)GetSetMediaTrackInfo(tr, "B_MAINSEND", NULL);
			GetSetMediaTrackInfo(tr, "B_MAINSEND", &bMPSend);
		}
	}
}

void SelTracksWItems(COMMAND_T* = NULL)
{
	ClearSelected();
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
			if (*(bool*)GetSetMediaItemInfo(GetTrackMediaItem(tr, j), "B_UISEL", NULL))
			{
				GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i1);
				break;
			}
	}
}

void UnselNotStem(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* mi = GetTrackMediaItem(tr, j);
			for (int k = 0; k < GetMediaItemNumTakes(mi); k++)
			{
				PCM_source* src = (PCM_source*)GetSetMediaItemTakeInfo(GetMediaItemTake(mi, k), "P_SOURCE", NULL);
				if (src && !strstr(src->GetFileName(), "stems"))
					GetSetMediaItemInfo(mi, "B_UISEL", &g_bFalse);
			}
		}
	}
	UpdateTimeline();
}

void UnselNotRender(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* mi = GetTrackMediaItem(tr, j);
			if (GetMediaItemNumTakes(mi))
			{
				PCM_source* src = (PCM_source*)GetSetMediaItemTakeInfo(GetMediaItemTake(mi, -1), "P_SOURCE", NULL);
				if (src && !strstr(src->GetFileName(), "render"))
					GetSetMediaItemInfo(mi, "B_UISEL", &g_bFalse);
			}
		}
	}
	UpdateTimeline();
}

void SelAllParents(COMMAND_T* = NULL)
{
	MediaTrack* gfd = NULL;
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		int iType;
		int iFolder = GetFolderDepth(tr, &iType, &gfd);
		if (iFolder == 0 && iType == 1)
			GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i1);
		else
			GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i0);
	}
}

void SelFolderStarts(COMMAND_T* = NULL)
{
	MediaTrack* gfd = NULL;
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		int iType;
		GetFolderDepth(tr, &iType, &gfd);
		if (iType == 1)
			GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i1);
		else
			GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i0);
	}
}

void CollapseFolders(COMMAND_T* = NULL)
{
	MediaTrack* gfd = NULL;
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		int iType;
		int iFolder = GetFolderDepth(tr, &iType, &gfd);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL) && iFolder == 0 && iType == 1)
			GetSetMediaTrackInfo(tr, "I_FOLDERCOMPACT", &g_i2);
	}
	UpdateTimeline();
}

void UncollFolders(COMMAND_T* = NULL)
{
	MediaTrack* gfd = NULL;
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		int iType;
		int iFolder = GetFolderDepth(tr, &iType, &gfd);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL) && iFolder == 0 && iType == 1)
			GetSetMediaTrackInfo(tr, "I_FOLDERCOMPACT", &g_i0);
	}
	UpdateTimeline();
}

void SmallFolders(COMMAND_T* = NULL)
{
	MediaTrack* gfd = NULL;
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		int iType;
		int iFolder = GetFolderDepth(tr, &iType, &gfd);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL) && iFolder == 0 && iType == 1)
			GetSetMediaTrackInfo(tr, "I_FOLDERCOMPACT", &g_i1);
	}
	UpdateTimeline();
}

void SelNotFolder(COMMAND_T* = NULL)
{
	MediaTrack* gfd = NULL;
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		int iType;
		if (GetFolderDepth(tr, &iType, &gfd) == 0 && iType == 0)
			GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i1);
		else
			GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i0);
	}
}

void MuteSends(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			for (int j = 0; GetSetTrackSendInfo(tr, 0, j, "P_DESTTRACK", NULL); j++)
				GetSetTrackSendInfo(tr, 0, j, "B_MUTE", &g_bTrue);
	}
}

void UnmuteSends(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			for (int j = 0; GetSetTrackSendInfo(tr, 0, j, "P_DESTTRACK", NULL); j++)
				GetSetTrackSendInfo(tr, 0, j, "B_MUTE", &g_bFalse);
	}
}

void InsertTrkAbove(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			InsertTrackAtIndex(i-1, false);
			ClearSelected();
			tr = CSurf_TrackFromID(i, false);
			TrackList_AdjustWindows(false);
			GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i1);
			UpdateTimeline();
			Undo_OnStateChangeEx("Insert track above selected track", UNDO_STATE_TRACKCFG, -1);
			return;
		}
	}
}

void SetLTT(COMMAND_T* = NULL)
{
	SaveSelected();
	int iFirst;
	for (iFirst = 1; iFirst <= GetNumTracks(); iFirst++)
		if (*(int*)GetSetMediaTrackInfo(CSurf_TrackFromID(iFirst, false), "I_SELECTED", NULL))
			break;
	if (iFirst > GetNumTracks())
		return;

	// 40505 to sel LTT
	// 40285 go to next
	// 40286 go to prev
	Main_OnCommand(40505, 0);
	int iLTT;
	for (iLTT = 1; iLTT <= GetNumTracks(); iLTT++)
		if (*(int*)GetSetMediaTrackInfo(CSurf_TrackFromID(iLTT, false), "I_SELECTED", NULL))
			break;

	while (iLTT != iFirst)
	{
		if (iLTT > iFirst)
		{
			iLTT--;
			Main_OnCommand(40286, 0);
		}
		else
		{
			iLTT++;
			Main_OnCommand(40285, 0);
		}
	}

	RestoreSelected();
}

void CreateTrack1(COMMAND_T* = NULL)
{
	ClearSelected();
	InsertTrackAtIndex(0, false);
	TrackList_AdjustWindows(false);
	GetSetMediaTrackInfo(CSurf_TrackFromID(1, false), "I_SELECTED", &g_i1);
	SetLTT();
}

void MinimizeTracks(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			GetSetMediaTrackInfo(tr, "I_HEIGHTOVERRIDE", &g_i1);
	}
	TrackList_AdjustWindows(false);
	UpdateTimeline();
}

void SelArmed(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_RECARM", NULL))
			GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i1);
		else
			GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i0);
	}
}

void UnselArmed(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_RECARM", NULL))
			GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i0);
	}
}

void RecToggle(COMMAND_T* = NULL)
{
	// 1016 == stop
	// 1013 == record
	if (!(GetPlayState() & 4))
		Main_OnCommand(1013, 0);
	else
		Main_OnCommand(1016, 0);
}

void RecSrcOut(COMMAND_T* = NULL)
{
	// Set the rec source to output
	// Set to mono or stereo based on first item
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		int iMode = 1; // 5 = mono, 1 = stereo
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			if (GetTrackNumMediaItems(tr))
			{
				MediaItem* mi = GetTrackMediaItem(tr, 0);
				if (GetMediaItemNumTakes(mi))
				{
					MediaItem_Take* mit = GetMediaItemTake(mi, 0);
					PCM_source* p = (PCM_source*)GetSetMediaItemTakeInfo(mit, "P_SOURCE", NULL);
					if (p && p->GetNumChannels() == 1)
						iMode = 5;

				}
			}
			GetSetMediaTrackInfo(tr, "I_RECMODE", &iMode);
		}
	}
}

void SetMonMedia(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			GetSetMediaTrackInfo(tr, "I_RECMONITEMS", &g_i1);
	}
}

void UnsetMonMedia(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			GetSetMediaTrackInfo(tr, "I_RECMONITEMS", &g_i0);
	}
}

static bool g_bRepeat = false;
void SaveRepeat(COMMAND_T* = NULL)	{ g_bRepeat = GetSetRepeat(-1) ? true : false; }
void RestRepeat(COMMAND_T* = NULL)	{ GetSetRepeat(g_bRepeat); }
void SetRepeat(COMMAND_T* = NULL)	{ GetSetRepeat(1); }
void UnsetRepeat(COMMAND_T* = NULL)	{ GetSetRepeat(0); }

void ShowDock(COMMAND_T* = NULL)
{
	int iCurState = GetPrivateProfileInt("REAPER", "dock_vis", 0, get_ini_file());
	if (!iCurState)
		Main_OnCommand(40279, 0);
}

void HideDock(COMMAND_T* = NULL)
{
	int iCurState = GetPrivateProfileInt("REAPER", "dock_vis", 0, get_ini_file());
	if (iCurState)
		Main_OnCommand(40279, 0);
}

void ShowMaster(COMMAND_T* = NULL)
{
	if (GetConfigVar("showmaintrack") && !*(int*)GetConfigVar("showmaintrack"))
		Main_OnCommand(40075, 0);
}

void HideMaster(COMMAND_T* = NULL)
{
	if (GetConfigVar("showmaintrack") && *(int*)GetConfigVar("showmaintrack"))
		Main_OnCommand(40075, 0);
}

#ifdef TESTCODE
#pragma message("TESTCODE Defined")
void TestFunc(COMMAND_T* = NULL)
{
	/*char str[64];
	format_timestr_pos(GetCursorPosition(), str, 64, 2);
	_snprintf(str, 64, "%d.1.0", atoi(str)+1);
	SetEditCurPos(parse_timestr_pos(str, 2), true, false);		*/

	// Try to get cmd ID for something random:
	int id = plugin_register("command_id", "40b7ce356d5cd54a8b0578f71f3354ad");
	int x = 1;


}
#endif

void SelNextFolder(COMMAND_T* = NULL)
{
	int iDepth = -1;
	int iType;
	int iFolder;
	MediaTrack* gfd;
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (iDepth != -1)
		{
			if ((iFolder = GetFolderDepth(tr, &iType, &gfd)) == iDepth && iType == 1)
			{
				ClearSelected();
				GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i1);
				return;
			}
			else if (iType + iFolder < iDepth)
				iDepth = -1;
		}		
		else if (iDepth == -1 && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			iFolder = GetFolderDepth(tr, &iType, &gfd);
			if (iType == 1)
				iDepth = iFolder;
		}
	}
}

void SelPrevFolder(COMMAND_T* = NULL)
{
	int iDepth = -1;
	int iType;
	int iFolder;
	MediaTrack* gfd;
	for (int i = GetNumTracks(); i > 0; i--)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (iDepth != -1)
		{
			if ((iFolder = GetFolderDepth(tr, &iType, &gfd)) == iDepth && iType == 1)
			{
				ClearSelected();
				GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i1);
				return;
			}
			else if (iFolder < iDepth)
				iDepth = -1;
		}		
		else if (iDepth == -1 && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			iFolder = GetFolderDepth(tr, &iType, &gfd);
			if (iType == 1)
				iDepth = iFolder;
		}
	}
}

void DelAllItems(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			while(GetTrackNumMediaItems(tr))
				DeleteTrackMediaItem(tr, GetTrackMediaItem(tr, 0));
	}
	UpdateTimeline();
	Undo_OnStateChangeEx("Delete all items on selected track(s)", UNDO_STATE_ITEMS, -1);
}

// Set selected track(s) folder depth to the same folder depth as the previous track(s)
void FolderLikePrev(COMMAND_T* = NULL)
{
	MediaTrack* prevTr = CSurf_TrackFromID(1, false);
	bool bUndo = false;
	MediaTrack* gfd = NULL;
	for (int i = 2; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		int iDelta;
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL) && GetFolderDepth(prevTr, NULL, &gfd) != GetFolderDepth(tr, &iDelta, &gfd))
		{
			GetSetMediaTrackInfo(tr, "I_FOLDERDEPTH", GetSetMediaTrackInfo(prevTr, "I_FOLDERDEPTH", NULL));
			GetSetMediaTrackInfo(prevTr, "I_FOLDERDEPTH", &g_i0);
			bUndo = true;
		}
		prevTr = tr;
	}
	if (bUndo)
		Undo_OnStateChangeEx("Set selected track(s) to same folder as previous track", UNDO_STATE_TRACKCFG, -1);
}

void MakeFolder(COMMAND_T* = NULL)
{
	int i = 0;
	bool bUndo = false;
	MediaTrack* tr = CSurf_TrackFromID(1, false);
	while (++i < GetNumTracks())
	{
		MediaTrack* nextTr = CSurf_TrackFromID(i+1, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL) && *(int*)GetSetMediaTrackInfo(nextTr, "I_SELECTED", NULL))
		{
			bUndo = true;
			int iDepth = *(int*)GetSetMediaTrackInfo(tr, "I_FOLDERDEPTH", NULL) + 1;
			GetSetMediaTrackInfo(tr, "I_FOLDERDEPTH", &iDepth);

			while (++i <= GetNumTracks())
			{
				tr = nextTr;
				nextTr = CSurf_TrackFromID(i+1, false);
				if (!nextTr || !(*(int*)GetSetMediaTrackInfo(nextTr, "I_SELECTED", NULL)))
					break;
			}

			iDepth = *(int*)GetSetMediaTrackInfo(tr, "I_FOLDERDEPTH", NULL) - 1;
			GetSetMediaTrackInfo(tr, "I_FOLDERDEPTH", &iDepth);
		}
		tr = nextTr;
	}
	if (bUndo)
		Undo_OnStateChangeEx("Make folder from selected tracks", UNDO_STATE_TRACKCFG, -1);
}

void IndentTracks(COMMAND_T* = NULL)
{
	bool bUndo = false;
	MediaTrack* prevTr = CSurf_TrackFromID(1, false);

	for (int i = 2; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			int iDepth = *(int*)GetSetMediaTrackInfo(prevTr, "I_FOLDERDEPTH", NULL) + 1;
			if (iDepth < 2)
			{
				bUndo = true;
				GetSetMediaTrackInfo(prevTr, "I_FOLDERDEPTH", &iDepth);
				iDepth = *(int*)GetSetMediaTrackInfo(tr, "I_FOLDERDEPTH", NULL) - 1;
				GetSetMediaTrackInfo(tr, "I_FOLDERDEPTH", &iDepth);
			}
		}
		prevTr = tr;
	}

	if (bUndo)
		Undo_OnStateChangeEx("Indent selected tracks", UNDO_STATE_TRACKCFG, -1);
}

void UnindentTracks(COMMAND_T* = NULL)
{
	bool bUndo = false;
	MediaTrack* prevTr = CSurf_TrackFromID(1, false);
	MediaTrack* gfd = NULL;
	for (int i = 2; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			int iDelta;
			int iDepth = GetFolderDepth(tr, &iDelta, &gfd);
			if (iDepth > 0 && iDelta < 1)
			{
				bUndo = true;
				iDepth = *(int*)GetSetMediaTrackInfo(prevTr, "I_FOLDERDEPTH", NULL) - 1;
				GetSetMediaTrackInfo(prevTr, "I_FOLDERDEPTH", &iDepth);
				iDelta++;
				GetSetMediaTrackInfo(tr, "I_FOLDERDEPTH", &iDelta);
			}
		}
		prevTr = tr;
	}

	if (bUndo)
		Undo_OnStateChangeEx("Unindent selected tracks", UNDO_STATE_TRACKCFG, -1);
}

void SelNextItem(COMMAND_T* = NULL)
{
	// Find the last selected
	MediaItem* nextMi = NULL;
	for (int i = GetNumTracks(); i > 0; i--)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (GetTrackVis(tr) & 2)
		{
			for (int j = GetTrackNumMediaItems(tr)-1; j >= 0; j--)
			{
				MediaItem* mi = GetTrackMediaItem(tr, j);
				if (*(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL))
				{
					if (nextMi)
					{
						Main_OnCommand(40289, 0); // Unselect all items
						GetSetMediaItemInfo(nextMi, "B_UISEL", &g_bTrue);
						UpdateTimeline();
						return;
					}
				}
				nextMi = mi;
			}
		}
	}
}

void SelPrevItem(COMMAND_T* = NULL)
{
	// Find the last selected
	MediaItem* prevMi = NULL;
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (GetTrackVis(tr) & 2)
		{
			for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
			{
				MediaItem* mi = GetTrackMediaItem(tr, j);
				if (*(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL))
				{
					if (prevMi)
					{
						Main_OnCommand(40289, 0); // Unselect all items
						GetSetMediaItemInfo(prevMi, "B_UISEL", &g_bTrue);
						UpdateTimeline();
						return;
					}
				}
				prevMi = mi;
			}
		}
	}
}

void SelMutedTracks(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		int iSel = *(bool*)GetSetMediaTrackInfo(tr, "B_MUTE", NULL) ? 1 : 0;
		GetSetMediaTrackInfo(tr, "I_SELECTED", &iSel);
	}
	TrackList_AdjustWindows(false);
}

void SelMutedItems(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* mi = GetTrackMediaItem(tr, j);
			GetSetMediaItemInfo(mi, "B_UISEL", (bool*)GetSetMediaItemInfo(mi, "B_MUTE", NULL));
		}
	}
	UpdateTimeline();
}

void SelMutedItemsSel(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
			{
				MediaItem* mi = GetTrackMediaItem(tr, j);
				GetSetMediaItemInfo(mi, "B_UISEL", (bool*)GetSetMediaItemInfo(mi, "B_MUTE", NULL));
			}
	}
	UpdateTimeline();
}

void SelSoloedTracks(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		int iSel = *(int*)GetSetMediaTrackInfo(tr, "I_SOLO", NULL) ? 1 : 0;
		GetSetMediaTrackInfo(tr, "I_SELECTED", &iSel);
	}
	TrackList_AdjustWindows(false);
}

void SelPhaseTracks(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		int iSel = *(bool*)GetSetMediaTrackInfo(tr, "B_PHASE", NULL) ? 1 : 0;
		GetSetMediaTrackInfo(tr, "I_SELECTED", &iSel);
	}
	TrackList_AdjustWindows(false);
}

void SelArmedTracks(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		int iSel = *(int*)GetSetMediaTrackInfo(tr, "I_RECARM", NULL) ? 1 : 0;
		GetSetMediaTrackInfo(tr, "I_SELECTED", &iSel);
	}
	TrackList_AdjustWindows(false);
}

void SelLockedItems(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* mi = GetTrackMediaItem(tr, j);
			if (*(char*)GetSetMediaItemInfo(mi, "C_LOCK", NULL))
				GetSetMediaItemInfo(mi, "B_UISEL", &g_i1);
			else
				GetSetMediaItemInfo(mi, "B_UISEL", &g_i0);
		}
	}
	UpdateTimeline();
}

void SelLockedItemsSel(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
			{
				MediaItem* mi = GetTrackMediaItem(tr, j);
				if (*(char*)GetSetMediaItemInfo(mi, "C_LOCK", NULL))
					GetSetMediaItemInfo(mi, "B_UISEL", &g_i1);
				else
					GetSetMediaItemInfo(mi, "B_UISEL", &g_i0);
			}
	}
	UpdateTimeline();
}

void AddRightItem(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		WDL_PtrList<void> selItems;
		// First find selected item(s)
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* selItem = GetTrackMediaItem(tr, j);
			if (*(bool*)GetSetMediaItemInfo(selItem, "B_UISEL", NULL))
				selItems.Add(selItem);
		}

		for (int j = 0; j < selItems.GetSize(); j++)
		{
			double dLeft     = *(double*)GetSetMediaItemInfo((MediaItem*)selItems.Get(j), "D_POSITION", NULL);
			double dRight    = *(double*)GetSetMediaItemInfo((MediaItem*)selItems.Get(j), "D_LENGTH", NULL) + dLeft;
			double dMinRight = DBL_MAX;
			MediaItem* rightItem = NULL;
			for (int k = 0; k < GetTrackNumMediaItems(tr); k++)
			{
				MediaItem* mi;
				mi = GetTrackMediaItem(tr, k);
				double dLeft2  = *(double*)GetSetMediaItemInfo(mi, "D_POSITION", NULL);
				double dRight2 = *(double*)GetSetMediaItemInfo(mi, "D_LENGTH", NULL) + dLeft2;
				if (dLeft2 > dLeft && dRight2 > dRight && dRight < dMinRight)
				{
					dMinRight = dRight;
					rightItem = mi;
				}
			}
			GetSetMediaItemInfo(rightItem, "B_UISEL", &g_i1);
		}
	}
	UpdateTimeline();
}

static int g_iMasterFXEn = 1;
void SaveMasterFXEn(COMMAND_T*)  { g_iMasterFXEn = *(int*)GetSetMediaTrackInfo(CSurf_TrackFromID(0, false), "I_FXEN", NULL); }
void RestMasterFXEn(COMMAND_T*)  { GetSetMediaTrackInfo(CSurf_TrackFromID(0, false), "I_FXEN", &g_iMasterFXEn); }
void EnableMasterFX(COMMAND_T*)  { GetSetMediaTrackInfo(CSurf_TrackFromID(0, false), "I_FXEN", &g_i1); }
void DisableMasterFX(COMMAND_T*) { GetSetMediaTrackInfo(CSurf_TrackFromID(0, false), "I_FXEN", &g_i0); }
void SelMaster(COMMAND_T*)       { GetSetMediaTrackInfo(CSurf_TrackFromID(0, false), "I_SELECTED", &g_i1); }
void UnselMaster(COMMAND_T*)     { GetSetMediaTrackInfo(CSurf_TrackFromID(0, false), "I_SELECTED", &g_i0); }

void TogSelMaster(COMMAND_T*)
{
	MediaTrack* tr = CSurf_TrackFromID(0, false);
	int iSel = *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL) ? 0 : 1;
	GetSetMediaTrackInfo(tr, "I_SELECTED", &iSel);
}

int RoutedTracks(MediaTrack* tr, WDL_PtrList<void>* pTracks)
{
	int iStartSize = pTracks->GetSize();

	// First, check receives
	for (int i = 0; GetSetTrackSendInfo(tr, -1, i, "P_SRCTRACK", NULL); i++)
		if (!*(bool*)GetSetTrackSendInfo(tr, -1, i, "B_MUTE", NULL))
		{
			MediaTrack* pRec = (MediaTrack*)GetSetTrackSendInfo(tr, -1, i, "P_SRCTRACK", NULL);
			if (!*(bool*)GetSetMediaTrackInfo(pRec, "B_MUTE", NULL))
				pTracks->Add(pRec);
		}

	// Then check folders
	int iType;
	MediaTrack* pNext = NULL;
	int iDepth = GetFolderDepth(tr, &iType, &pNext);
	if (iType == 1)
	{
		int iChild = CSurf_TrackToID(tr, false)+1;
		while(true)
		{
			MediaTrack* pChild = CSurf_TrackFromID(iChild, false);
			if (!pChild)
				break;
			int iChildDepth = GetFolderDepth(pChild, NULL, &pNext);
			if (iChildDepth == iDepth + 1 &&
				*(bool*)GetSetMediaTrackInfo(pChild, "B_MAINSEND", NULL) &&
				!*(bool*)GetSetMediaTrackInfo(pChild, "B_MUTE", NULL))
				pTracks->Add(pChild);
			if (iChildDepth <= iDepth)
				break;
			iChild++;
		}
	}

	int iEndSize = pTracks->GetSize();
	for (int i = iStartSize; i < iEndSize; i++)
		RoutedTracks((MediaTrack*)pTracks->Get(i), pTracks);

	return iEndSize - iStartSize;
}

void SelRouted(COMMAND_T*)
{
	// First build a list of the initially selected tracks
	WDL_PtrList<void> pSelTracks = NULL;
	for (int i = 0; i <= GetNumTracks(); i++)
		if (*(int*)GetSetMediaTrackInfo(CSurf_TrackFromID(i, false), "I_SELECTED", NULL))
			pSelTracks.Add(CSurf_TrackFromID(i, false));

	for (int i = 0; i < pSelTracks.GetSize(); i++)
	{
		WDL_PtrList<void> pRoutedTracks;
		RoutedTracks((MediaTrack*)pSelTracks.Get(i), &pRoutedTracks);
		for (int j = 0; j < pRoutedTracks.GetSize(); j++)
			GetSetMediaTrackInfo((MediaTrack*)pRoutedTracks.Get(j), "I_SELECTED", &g_i1);
	}
}

void MoveItemLeftToCursor(COMMAND_T* = NULL)
{
	double dEditCur = GetCursorPosition();
	// 
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* mi = GetTrackMediaItem(tr, j);
			if (*(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL))
			{
				GetSetMediaItemInfo(mi, "D_POSITION", &dEditCur);
			}
		}
	}
	UpdateTimeline();
	Undo_OnStateChangeEx("Move selected item(s) left edge to edit cursor", UNDO_STATE_ITEMS, -1);
}

void MoveItemRightToCursor(COMMAND_T* = NULL)
{
	double dEditCur = GetCursorPosition();
	// 
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* mi = GetTrackMediaItem(tr, j);
			if (*(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL))
			{
				double dNewPos = dEditCur - *(double*)GetSetMediaItemInfo(mi, "D_LENGTH", NULL);
				if (dNewPos >= 0.0)
					GetSetMediaItemInfo(mi, "D_POSITION", &dNewPos);
			}
		}
	}
	UpdateTimeline();
	Undo_OnStateChangeEx("Move selected item(s) right edge to edit cursor", UNDO_STATE_ITEMS, -1);
}

void DelTracksChild(COMMAND_T* = NULL)
{
	int iRet = MessageBox(g_hwndParent, "Delete track(s) children too?", "Delete track(s)", MB_YESNOCANCEL);
	if (iRet == IDCANCEL)
		return;
	if (iRet == IDYES)
		SelChildren();
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			DeleteTrack(tr);
			i--;
		}
	}
	Undo_OnStateChangeEx("Delete track(s)", UNDO_STATE_TRACKCFG, -1);
}

void NameTrackLikeItem(COMMAND_T*)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
			{
				MediaItem* mi = GetTrackMediaItem(tr, j);
				if (*(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL) && GetMediaItemNumTakes(mi))
				{
					MediaItem_Take* mit = GetMediaItemTake(mi, -1);
					PCM_source* src = (PCM_source*)GetSetMediaItemTakeInfo(mit, "P_SOURCE", NULL);
					if (src && src->GetFileName())
					{
						const char* cFilename = src->GetFileName();
						const char* pC = strrchr(cFilename, PATH_SLASH_CHAR);
						if (pC)
							cFilename = pC + 1;
						GetSetMediaTrackInfo(tr, "P_NAME", (void*)cFilename);
					}
					break;
				}
			}
		}
	}

	// TODO undo?
}

void SelectTrack(COMMAND_T* ct)
{
	ClearSelected();
	MediaTrack* tr = CSurf_TrackFromID((int)ct->user, false);
	if (tr)
		GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i1);
}

static bool ProcessExtensionLine(const char *line, ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	LineParser lp(false);
	if (lp.parse(line) || lp.getnumtokens() < 1)
		return false;
	if (strcmp(lp.gettoken_str(0), "<TRACKSTATE") == 0)
	{
		TrackState* ts = g_tracks.Get()->Add(new TrackState(&lp));

		char linebuf[4096];
		while(!ctx->GetLine(linebuf,sizeof(linebuf)) && !lp.parse(linebuf))
		{
			if (lp.gettoken_str(0)[0] == '>')
				break;
			else if (strcmp("ITEMSTATE", lp.gettoken_str(0)) == 0)
				ts->m_items.Add(new ItemState(&lp));
		}
		return true;
	}
	else if (strcmp(lp.gettoken_str(0), "<MUTESTATE") == 0)
	{
		MuteState* ms = g_muteStates.Get()->Add(new MuteState(&lp));

		char linebuf[4096];
		while(!ctx->GetLine(linebuf,sizeof(linebuf)) && !lp.parse(linebuf))
		{
			if (lp.gettoken_str(0)[0] == '>')
				break;
			else if (strcmp("CHILD", lp.gettoken_str(0)) == 0)
				ms->m_children.Add(new MuteItem(&lp));
			else if (strcmp("RECEIVE", lp.gettoken_str(0)) == 0)
				ms->m_receives.Add(new MuteItem(&lp));
		}
		return true;
	}
	else if (strcmp(lp.gettoken_str(0), "<SELTRACKITEMSELSTATE") == 0)
	{
		SelItemsTrack* sit = g_selItemsTrack.Get()->Add(new SelItemsTrack(&lp));
		char linebuf[4096];
		while(!ctx->GetLine(linebuf,sizeof(linebuf)) && !lp.parse(linebuf))
		{
			if (lp.gettoken_str(0)[0] == '>')
				break;
			else if (strcmp(lp.gettoken_str(0), "<SLOT") == 0)
			{
				int iSlot = lp.gettoken_int(1) - 1;
				while(!ctx->GetLine(linebuf,sizeof(linebuf)) && !lp.parse(linebuf))
				{
					sit->Add(&lp, iSlot); // Add even a single '>' to ensure "empty" slots are restored properly
					if (lp.gettoken_str(0)[0] == '>')
						break;
				}
			}
		}
		return true;
	}
	else if (strcmp(lp.gettoken_str(0), "<ITEMSELSTATE") == 0)
	{
		char linebuf[4096];
		// Ignore old-style take-as-item-GUID
		if (lp.getnumtokens() == 2 && strlen(lp.gettoken_str(1)) != 38)
		{
			g_selItems.Get()->Empty();
			while(!ctx->GetLine(linebuf,sizeof(linebuf)) && !lp.parse(linebuf))
			{
				if (lp.gettoken_str(0)[0] == '>')
					break;
				g_selItems.Get()->Add(&lp);
			}
		}
		else
		{
			int iDepth = 0;
			while(!ctx->GetLine(linebuf,sizeof(linebuf)) && !lp.parse(linebuf))
			{
				if (lp.gettoken_str(0)[0] == '<')
					++iDepth;
				else if (lp.gettoken_str(0)[0] == '>')
					--iDepth;
				if (iDepth < 0)
					break;
			}
		}
		return true;
	}
	else if (strcmp(lp.gettoken_str(0), "<ACTIVETAKESTRACK") == 0)
	{
		ActiveTakeTrack* att = g_activeTakeTracks.Get()->Add(new ActiveTakeTrack(&lp));
		char linebuf[4096];
		while(!ctx->GetLine(linebuf,sizeof(linebuf)) && !lp.parse(linebuf))
		{
			if (lp.gettoken_str(0)[0] == '>')
				break;
			else if (strcmp(lp.gettoken_str(0), "ITEM") == 0)
				att->m_items.Add(new ActiveTake(&lp));
		}
		return true;
	}
	else if (strcmp(lp.gettoken_str(0), "TIMESEL") == 0)
	{
		g_timeSel.Get()->Add(new TimeSelection(&lp));
		return true;
	}
	return false;
}

static void SaveExtensionConfig(ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	char str[4096];

	for (int i = 0; i < g_tracks.Get()->GetSize(); i++)
	{
		ctx->AddLine(g_tracks.Get()->Get(i)->ItemString(str, 4096)); 
		for (int j = 0; j < g_tracks.Get()->Get(i)->m_items.GetSize(); j++)
			ctx->AddLine(g_tracks.Get()->Get(i)->m_items.Get(j)->ItemString(str, 4096));
		ctx->AddLine(">");
	}
	bool bDone;
	for (int i = 0; i < g_muteStates.Get()->GetSize(); i++)
	{
		do
			ctx->AddLine(g_muteStates.Get()->Get(i)->ItemString(str, 4096, &bDone));
		while (!bDone);
	}
	for (int i = 0; i < g_selItemsTrack.Get()->GetSize(); i++)
	{
		do
			ctx->AddLine(g_selItemsTrack.Get()->Get(i)->ItemString(str, 4096, &bDone));
		while (!bDone);
	}
	if (g_selItems.Get()->NumItems())
	{
		ctx->AddLine("<ITEMSELSTATE 1");
		do
			ctx->AddLine(g_selItems.Get()->ItemString(str, 4096, &bDone));
		while (!bDone);
	}
	for (int i = 0; i < g_activeTakeTracks.Get()->GetSize(); i++)
	{
		ctx->AddLine(g_activeTakeTracks.Get()->Get(i)->ItemString(str, 4096)); 
		for (int j = 0; j < g_activeTakeTracks.Get()->Get(i)->m_items.GetSize(); j++)
			ctx->AddLine(g_activeTakeTracks.Get()->Get(i)->m_items.Get(j)->ItemString(str, 4096));
		ctx->AddLine(">");
	}
	for (int i = 0; i < g_timeSel.Get()->GetSize(); i++)
		ctx->AddLine(g_timeSel.Get()->Get(i)->ItemString(str, 4096));
}

static void BeginLoadProjectState(bool isUndo, struct project_config_extension_t *reg)
{
	g_tracks.Get()->Empty(true);
	g_tracks.Cleanup();
	g_muteStates.Get()->Empty(true);
	g_muteStates.Cleanup();
	g_selItemsTrack.Get()->Empty(true);
	g_selItemsTrack.Cleanup();
	g_selItems.Get()->Empty();
	g_selItems.Cleanup();
	g_activeTakeTracks.Get()->Empty(true);
	g_activeTakeTracks.Cleanup();
	g_timeSel.Get()->Empty(true);
	g_timeSel.Cleanup();
}

static project_config_extension_t g_projectconfig = { ProcessExtensionLine, SaveExtensionConfig, BeginLoadProjectState, NULL };

static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "SWS: Save selected track(s) items' states" },                  "SWS_SAVETRACK",      SaveTrack,        NULL, },
	{ { DEFACCEL, "SWS: Restore selected track(s) items' states" },               "SWS_RESTORETRACK",   RestoreTrack,     NULL, },
	{ { DEFACCEL, "SWS: Save selected track(s) selected items' states" },         "SWS_SAVESELONTRACK", SaveSelOnTrack,   NULL, },
	{ { DEFACCEL, "SWS: Restore selected track(s) selected items' states" },      "SWS_RESTSELONTRACK", RestoreSelOnTrack,NULL, },
	{ { DEFACCEL, "SWS: Select item(s) with saved state on selected track(s)" },  "SWS_SELWITHSTATE",   SelItemsWithState,NULL, },
	{ { DEFACCEL, "SWS: Save auto crossfade state" },                             "SWS_SAVEXFD",        SaveXFade,        NULL, },
	{ { DEFACCEL, "SWS: Restore auto crossfade state" },                          "SWS_RESTOREXFD",     RestoreXFade,     NULL, },
	{ { DEFACCEL, "SWS: Set auto crossfade on" },                                 "SWS_XFDON",          XFadeOn,          NULL, },
	{ { DEFACCEL, "SWS: Set auto crossfade off" },                                "SWS_XFDOFF",         XFadeOff,         NULL, },
	{ { DEFACCEL, "SWS: Set move envelope points with items on" },                "SWS_MVPWIDON",       MEPWIXOn,         NULL, },
	{ { DEFACCEL, "SWS: Set move envelope points with items off" },               "SWS_MVPWIDOFF",      MEPWIXOff,        NULL, },
	{ { DEFACCEL, "SWS: Select lower-leftmost item on selected track(s)" },       "SWS_SELLLI",         SelLLItem,        NULL, },
	{ { DEFACCEL, "SWS: Select upper-leftmost item on selected track(s)" },       "SWS_SELULI",         SelULItem,        NULL, },
	{ { DEFACCEL, "SWS: Unselect upper-leftmost item on selected track(s)" },     "SWS_UNSELULI",       UnselULItem,      NULL, },
	{ { DEFACCEL, "SWS: Mute all receives for selected track(s)" },               "SWS_MUTERECVS",      MuteRecvs,        NULL, },
	{ { DEFACCEL, "SWS: Unmute all receives for selected track(s)" },             "SWS_UNMUTERECVS",    UnmuteRecvs,      NULL, },
	{ { DEFACCEL, "SWS: Toggle mute on receives for selected track(s)" },         "SWS_TOGMUTERECVS",   TogMuteRecvs,     NULL, },
	{ { DEFACCEL, "SWS: Unselect all items on selected track(s)" },               "SWS_UNSELONTRACKS",  UnselOnTracks,    NULL, },
	{ { DEFACCEL, "SWS: Save current track selection" },                          "SWS_SAVESEL",        SaveSelTracks,    NULL, },
	{ { DEFACCEL, "SWS: Restore saved track selection" },                         "SWS_RESTORESEL",     RestoreSelTracks, NULL, },
	{ { DEFACCEL, "SWS: Select only children of selected folders" },              "SWS_SELCHILDREN",    SelChildrenOnly,  NULL, },
	{ { DEFACCEL, "SWS: Select children of selected folder track(s)" },           "SWS_SELCHILDREN2",   SelChildren,      NULL, },
	{ { DEFACCEL, "SWS: Select only parent(s) of selected folder track(s)" },     "SWS_SELPARENTS",     SelParentsOnly,   NULL, },
	{ { DEFACCEL, "SWS: Select parent(s) of selected folder track(s)" },          "SWS_SELPARENTS2",    SelParents,       NULL, },
	{ { DEFACCEL, "SWS: Unselect parent(s) of selected folder track(s)" },        "SWS_UNSELPARENTS",   UnselParents,     NULL, },
	{ { DEFACCEL, "SWS: Unselect children of selected folder track(s)" },         "SWS_UNSELCHILDREN",  UnselChildren,    NULL, },
	{ { DEFACCEL, "SWS: Mute children of selected folder(s)" },                   "SWS_MUTECHILDREN",   MuteChildren,     NULL, },
	{ { DEFACCEL, "SWS: Unmute children of selected folder(s)" },                 "SWS_UNMUTECHILDREN", UnmuteChildren,   NULL, },
	{ { DEFACCEL, "SWS: Toggle mute of children of selected folder(s)" },         "SWS_TOGMUTECHILDRN", TogMuteChildren,  NULL, },
	{ { DEFACCEL, "SWS: Bypass FX on selected track(s)" },                        "SWS_BYPASSFX",       BypassFX,         NULL, },
	{ { DEFACCEL, "SWS: Unbypass FX on selected track(s)" },                      "SWS_UNBYPASSFX",     UnbypassFX,       NULL, },
	{ { DEFACCEL, "SWS: Toggle mute of items on selected track(s)" },			 "SWS_TOGITEMMUTE",    TogItemMute,      NULL, },
	{ { DEFACCEL, "SWS: Toggle selection of items on selected track(s)" },		 "SWS_TOGITEMSEL",     TogItemSel,       NULL, },
	{ { DEFACCEL, "SWS: Toggle (invert) track selection" },						 "SWS_TOGTRACKSEL",    TogTrackSel,      NULL, },
	{ { DEFACCEL, "SWS: Enable master/parent send on selected track(s)" },		 "SWS_ENMPSEND",       EnableMPSend,     NULL, },
	{ { DEFACCEL, "SWS: Disable master/parent send on selected track(s)" },		 "SWS_DISMPSEND",      DisableMPSend,    NULL, },
	{ { DEFACCEL, "SWS: Toggle master/parent send on selected track(s)" },		 "SWS_TOGMPSEND",      TogMPSend,        NULL, },
	{ { DEFACCEL, "SWS: Select only track(s) with selected item(s)" },            "SWS_SELTRKWITEM",    SelTracksWItems,  NULL, },
	{ { DEFACCEL, "SWS: Save selected track(s) mutes (+receives, children)" },    "SWS_SAVEMUTES",      SaveMutes,        NULL, },
	{ { DEFACCEL, "SWS: Restore selected track(s) mutes (+receives, children)" }, "SWS_RESTRMUTES",     RestoreMutes,     NULL, },
	{ { DEFACCEL, "SWS: Unselect items without 'stems' in source filename" },     "SWS_UNSELNOTSTEM",   UnselNotStem,     NULL, },
	{ { DEFACCEL, "SWS: Unselect items without 'render' in source filename" },    "SWS_UNSELNOTRENDER", UnselNotRender,   NULL, },
	{ { DEFACCEL, "SWS: Select all folders (parents only)" },                     "SWS_SELALLPARENTS",  SelAllParents,    NULL, },
	{ { DEFACCEL, "SWS: Select all folder start tracks" },                        "SWS_SELFOLDSTARTS",  SelFolderStarts,  NULL, },
	{ { DEFACCEL, "SWS: Set selected folder(s) collapsed" },                      "SWS_COLLAPSE",       CollapseFolders,  NULL, },
	{ { DEFACCEL, "SWS: Set selected folder(s) uncollapsed" },                    "SWS_UNCOLLAPSE",     UncollFolders,    NULL, },
	{ { DEFACCEL, "SWS: Set selected folder(s) small" },                          "SWS_FOLDSMALL",      SmallFolders,     NULL, },
	{ { DEFACCEL, "SWS: Select all non-folders" },                                "SWS_SELNOTFOLDER",   SelNotFolder,     NULL, },
	{ { DEFACCEL, "SWS: Mute all sends from selected track(s)" },                 "SWS_MUTESENDS",      MuteSends,        NULL, },
	{ { DEFACCEL, "SWS: Unmute all sends from selected track(s)" },               "SWS_UNMUTESENDS",    UnmuteSends,      NULL, },
	{ { DEFACCEL, "SWS: Insert track above selected tracks" },                    "SWS_INSRTTRKABOVE",  InsertTrkAbove,   NULL, },
	{ { DEFACCEL, "SWS: Set last touched track to match track selection" },       "SWS_SETLTT",         SetLTT,           NULL, },
	{ { DEFACCEL, "SWS: Create and select first track" },                         "SWS_CREATETRK1",     CreateTrack1,     NULL, },
	{ { DEFACCEL, "SWS: Minimize selected track(s)" },                            "SWS_MINTRACKS",      MinimizeTracks,   NULL, },
	{ { DEFACCEL, "SWS: Select only rec armed track(s)" },                        "SWS_SELRECARM",      SelArmed,         NULL, },
	{ { DEFACCEL, "SWS: Unselect rec armed track(s)" },                           "SWS_UNSELRECARM",    UnselArmed,       NULL, },
	{ { DEFACCEL, "SWS: Transport: Record/stop" },                                "SWS_RECTOGGLE",      RecToggle,        NULL, },
	{ { DEFACCEL, "SWS: Set selected track(s) record output mode based on items" }, "SWS_SETRECSRCOUT", RecSrcOut,        NULL, },
	{ { DEFACCEL, "SWS: Set selected track(s) monitor track media while recording" }, "SWS_SETMONMEDIA", SetMonMedia,     NULL, },
	{ { DEFACCEL, "SWS: Unset selected track(s) monitor track media while recording" }, "SWS_UNSETMONMEDIA", UnsetMonMedia, NULL, },
	{ { DEFACCEL, "SWS: Save transport repeat state" },                           "SWS_SAVEREPEAT",     SaveRepeat,       NULL, },
	{ { DEFACCEL, "SWS: Restore transport repeat state" },                        "SWS_RESTREPEAT",     RestRepeat,       NULL, },
	{ { DEFACCEL, "SWS: Set transport repeat state" },                            "SWS_SETREPEAT",      SetRepeat,        NULL, },
	{ { DEFACCEL, "SWS: Unset transport repeat state" },                          "SWS_UNSETREPEAT",    UnsetRepeat,      NULL, },
	{ { DEFACCEL, "SWS: Select next folder" },                                    "SWS_SELNEXTFOLDER",  SelNextFolder,    NULL, },
	{ { DEFACCEL, "SWS: Select previous folder" },                                "SWS_SELPREVFOLDER",  SelPrevFolder,    NULL, },
	{ { DEFACCEL, "SWS: Delete all items on selected track(s)" },                 "SWS_DELALLITEMS",    DelAllItems,      NULL, },
	{ { DEFACCEL, "SWS: Set selected track(s) to same folder as previous track" },"SWS_FOLDERPREV",     FolderLikePrev,   NULL, },
	{ { DEFACCEL, "SWS: Make folder from selected tracks" },                      "SWS_MAKEFOLDER",     MakeFolder,       NULL, },
	{ { DEFACCEL, "SWS: Indent selected track(s)" },								 "SWS_INDENT",         IndentTracks,     "SWS Indent track(s)", },
	{ { DEFACCEL, "SWS: Unindent selected track(s)" },							 "SWS_UNINDENT",       UnindentTracks,   "SWS Unindent track(s)", },
	{ { DEFACCEL, "SWS: Select next item (across tracks)" },                      "SWS_SELNEXTITEM",    SelNextItem,      NULL, },
	{ { DEFACCEL, "SWS: Select previous item (across tracks)" },                  "SWS_SELPREVITEM",    SelPrevItem,      NULL, },
	{ { DEFACCEL, "SWS: Select muted tracks" },									 "SWS_SELMUTEDTRACKS", SelMutedTracks,   NULL, },
	{ { DEFACCEL, "SWS: Select muted items" },									 "SWS_SELMUTEDITEMS",  SelMutedItems,    NULL, },
	{ { DEFACCEL, "SWS: Select muted items on selected track(s)" },				 "SWS_SELMUTEDITEMS2", SelMutedItemsSel, NULL, },
	{ { DEFACCEL, "SWS: Select soloed tracks" },								 "SWS_SELSOLOEDTRACKS", SelSoloedTracks,   NULL, },
	{ { DEFACCEL, "SWS: Select tracks with flipped phase" },					 "SWS_SELPHASETRACKS", SelPhaseTracks,   NULL, },
	{ { DEFACCEL, "SWS: Select armed tracks" },								     "SWS_SELARMEDTRACKS", SelArmedTracks,   NULL, },
	{ { DEFACCEL, "SWS: Select tracks with active routing to selected track(s)" },"SWS_SELROUTED",      SelRouted,        NULL, },
	{ { DEFACCEL, "SWS: Select locked items" },									 "SWS_SELLOCKITEMS",   SelLockedItems,   NULL, },
	{ { DEFACCEL, "SWS: Select locked items on selected track(s)" },				 "SWS_SELLOCKITEMS2",  SelLockedItemsSel,NULL, },
	{ { DEFACCEL, "SWS: Add item(s) to right of selected item(s) to selection" }, "SWS_ADDRIGHTITEM",   AddRightItem,     NULL, },

	{ { DEFACCEL, "SWS: Save master FX enabled state" },							 "SWS_SAVEMSTFXEN",    SaveMasterFXEn,   NULL, },
	{ { DEFACCEL, "SWS: Restore master FX enabled state" },					     "SWS_RESTMSTFXEN",    RestMasterFXEn,   NULL, },
	{ { DEFACCEL, "SWS: Show master track in track control panel" },              "SWS_SHOWMASTER",     ShowMaster,       NULL, },
	{ { DEFACCEL, "SWS: Hide master track in track control panel" },              "SWS_HIDEMASTER",     HideMaster,       NULL, },
	{ { DEFACCEL, "SWS: Enable master FX" },									  "SWS_ENMASTERFX",     EnableMasterFX,   NULL, },
	{ { DEFACCEL, "SWS: Disable master FX" },									  "SWS_DISMASTERFX",    DisableMasterFX,  NULL, },
	{ { DEFACCEL, "SWS: Select master track" },									  "SWS_SELMASTER",      SelMaster,        NULL, },
	{ { DEFACCEL, "SWS: Unselect master track" },								  "SWS_UNSELMASTER",    UnselMaster,      NULL, },
	{ { DEFACCEL, "SWS: Toggle master track select" },							  "SWS_TOGSELMASTER",   TogSelMaster,     NULL, },

	{ { DEFACCEL, "SWS: Save selected track(s) selected item(s), slot 1" },       "SWS_SAVESELITEMS1",  SaveSelTrackSelItems,	NULL, 0 },
	{ { DEFACCEL, "SWS: Save selected track(s) selected item(s), slot 2" },       "SWS_SAVESELITEMS2",  SaveSelTrackSelItems,	NULL, 1 },
	{ { DEFACCEL, "SWS: Save selected track(s) selected item(s), slot 3" },       "SWS_SAVESELITEMS3",  SaveSelTrackSelItems,	NULL, 2 },
	{ { DEFACCEL, "SWS: Save selected track(s) selected item(s), slot 4" },       "SWS_SAVESELITEMS4",  SaveSelTrackSelItems,	NULL, 3 },
	{ { DEFACCEL, "SWS: Save selected track(s) selected item(s), slot 5" },       "SWS_SAVESELITEMS5",  SaveSelTrackSelItems,	NULL, 4 },
	{ { DEFACCEL, "SWS: Restore selected track(s) selected item(s), slot 1" },    "SWS_RESTSELITEMS1",  RestoreSelTrackSelItems,	NULL, 0 },
	{ { DEFACCEL, "SWS: Restore selected track(s) selected item(s), slot 2" },    "SWS_RESTSELITEMS2",  RestoreSelTrackSelItems,	NULL, 1 },
	{ { DEFACCEL, "SWS: Restore selected track(s) selected item(s), slot 3" },    "SWS_RESTSELITEMS3",  RestoreSelTrackSelItems,	NULL, 2 },
	{ { DEFACCEL, "SWS: Restore selected track(s) selected item(s), slot 4" },    "SWS_RESTSELITEMS4",  RestoreSelTrackSelItems,	NULL, 3 },
	{ { DEFACCEL, "SWS: Restore selected track(s) selected item(s), slot 5" },    "SWS_RESTSELITEMS5",  RestoreSelTrackSelItems,	NULL, 4 },
	{ { DEFACCEL, "SWS: Restore last item selection on selected track(s)" },      "SWS_RESTLASTSEL",    RestoreLastSelItemTrack,	NULL, },
	{ { DEFACCEL, "SWS: Save selected item(s)" },								  "SWS_SAVEALLSELITEMS1", SaveSelItems,          NULL, 0 }, // Slots aren't supported here (yet?)
	{ { DEFACCEL, "SWS: Restore saved selected item(s)" },						  "SWS_RESTALLSELITEMS1", RestoreSelItems,       NULL, 0 },

	{ { DEFACCEL, "SWS: Save time selection, slot 1" },                           "SWS_SAVETIME1",      SaveTimeSel,      NULL, 1 },
	{ { DEFACCEL, "SWS: Save time selection, slot 2" },                           "SWS_SAVETIME2",      SaveTimeSel,      NULL, 2 },
	{ { DEFACCEL, "SWS: Save time selection, slot 3" },                           "SWS_SAVETIME3",      SaveTimeSel,      NULL, 3 },
	{ { DEFACCEL, "SWS: Save time selection, slot 4" },                           "SWS_SAVETIME4",      SaveTimeSel,      NULL, 4 },
	{ { DEFACCEL, "SWS: Save time selection, slot 5" },                           "SWS_SAVETIME5",      SaveTimeSel,      NULL, 5 },
	{ { DEFACCEL, "SWS: Save loop selection, slot 1" },                           "SWS_SAVELOOP1",      SaveLoopSel,      NULL, 1 },
	{ { DEFACCEL, "SWS: Save loop selection, slot 2" },                           "SWS_SAVELOOP2",      SaveLoopSel,      NULL, 2 },
	{ { DEFACCEL, "SWS: Save loop selection, slot 3" },                           "SWS_SAVELOOP3",      SaveLoopSel,      NULL, 3 },
	{ { DEFACCEL, "SWS: Save loop selection, slot 4" },                           "SWS_SAVELOOP4",      SaveLoopSel,      NULL, 4 },
	{ { DEFACCEL, "SWS: Save loop selection, slot 5" },                           "SWS_SAVELOOP5",      SaveLoopSel,      NULL, 5 },
	{ { DEFACCEL, "SWS: Restore time selection, slot 1" },                        "SWS_RESTTIME1",      RestoreTimeSel,   NULL, 1 },
	{ { DEFACCEL, "SWS: Restore time selection, slot 2" },                        "SWS_RESTTIME2",      RestoreTimeSel,   NULL, 2 },
	{ { DEFACCEL, "SWS: Restore time selection, slot 3" },                        "SWS_RESTTIME3",      RestoreTimeSel,   NULL, 3 },
	{ { DEFACCEL, "SWS: Restore time selection, slot 4" },                        "SWS_RESTTIME4",      RestoreTimeSel,   NULL, 4 },
	{ { DEFACCEL, "SWS: Restore time selection, slot 5" },                        "SWS_RESTTIME5",      RestoreTimeSel,   NULL, 5 },
	{ { DEFACCEL, "SWS: Restore loop selection, slot 1" },                        "SWS_RESTLOOP1",      RestoreLoopSel,   NULL, 1 },
	{ { DEFACCEL, "SWS: Restore loop selection, slot 2" },                        "SWS_RESTLOOP2",      RestoreLoopSel,   NULL, 2 },
	{ { DEFACCEL, "SWS: Restore loop selection, slot 3" },                        "SWS_RESTLOOP3",      RestoreLoopSel,   NULL, 3 },
	{ { DEFACCEL, "SWS: Restore loop selection, slot 4" },                        "SWS_RESTLOOP4",      RestoreLoopSel,   NULL, 4 },
	{ { DEFACCEL, "SWS: Restore loop selection, slot 5" },                        "SWS_RESTLOOP5",      RestoreLoopSel,   NULL, 5 },
	{ { DEFACCEL, "SWS: Restore time selection, next slot" },                     "SWS_RESTTIMENEXT",   RestoreTimeNext,  NULL, },
	{ { DEFACCEL, "SWS: Restore loop selection, next slot" },                     "SWS_RESTLOOPNEXT",   RestoreLoopNext,  NULL, },

	{ { DEFACCEL, "SWS: Save active takes on selected track(s)" },                "SWS_SAVEACTTAKES",   SaveActiveTakes,    NULL, },
	{ { DEFACCEL, "SWS: Restore active takes on selected track(s)" },             "SWS_RESTACTTAKES",   RestoreActiveTakes, NULL, },

	{ { DEFACCEL, "SWS: Show docker" },                                           "SWS_SHOWDOCK",       ShowDock,         NULL, },
	{ { DEFACCEL, "SWS: Hide docker" },                                           "SWS_HIDEDOCK",       HideDock,         NULL, },
	{ { DEFACCEL, "SWS: Move selected item(s) left edge to edit cursor" },        "SWS_ITEMLEFTTOCUR",  MoveItemLeftToCursor, NULL, },
	{ { DEFACCEL, "SWS: Move selected item(s) right edge to edit cursor" },       "SWS_ITEMRIGHTTOCUR", MoveItemRightToCursor, NULL, },
	{ { DEFACCEL, "SWS: Delete track(s) with children (prompt)" },		 		 "SWS_DELTRACKCHLD",   DelTracksChild,   NULL, },
	{ { DEFACCEL, "SWS: Name selected track(s) like first sel item" },		     "SWS_NAMETKLIKEITEM", NameTrackLikeItem, NULL, },

	{ { DEFACCEL, "SWS: Select only track 1" },							 		 "SWS_SEL1",		   SelectTrack,		 NULL, 1 },
	{ { DEFACCEL, "SWS: Select only track 2" },							 		 "SWS_SEL2",		   SelectTrack,		 NULL, 2 },
	{ { DEFACCEL, "SWS: Select only track 3" },							 		 "SWS_SEL3",		   SelectTrack,		 NULL, 3 },
	{ { DEFACCEL, "SWS: Select only track 4" },							 		 "SWS_SEL4",		   SelectTrack,		 NULL, 4 },
	{ { DEFACCEL, "SWS: Select only track 5" },							 		 "SWS_SEL5",		   SelectTrack,		 NULL, 5 },
	{ { DEFACCEL, "SWS: Select only track 6" },							 		 "SWS_SEL6",		   SelectTrack,		 NULL, 6 },
	{ { DEFACCEL, "SWS: Select only track 7" },							 		 "SWS_SEL7",		   SelectTrack,		 NULL, 7 },
	{ { DEFACCEL, "SWS: Select only track 8" },							 		 "SWS_SEL8",		   SelectTrack,		 NULL, 8 },
	{ { DEFACCEL, "SWS: Select only track 9" },							 		 "SWS_SEL9",		   SelectTrack,		 NULL, 9 },
	{ { DEFACCEL, "SWS: Select only track 10" },							 		 "SWS_SEL10",		   SelectTrack,		 NULL, 10 },
	{ { DEFACCEL, "SWS: Select only track 11" },							 		 "SWS_SEL11",		   SelectTrack,		 NULL, 11 },
	{ { DEFACCEL, "SWS: Select only track 12" },							 		 "SWS_SEL12",		   SelectTrack,		 NULL, 12 },
	{ { DEFACCEL, "SWS: Select only track 13" },							 		 "SWS_SEL13",		   SelectTrack,		 NULL, 13 },
	{ { DEFACCEL, "SWS: Select only track 14" },							 		 "SWS_SEL14",		   SelectTrack,		 NULL, 14 },
	{ { DEFACCEL, "SWS: Select only track 15" },							 		 "SWS_SEL15",		   SelectTrack,		 NULL, 15 },
	{ { DEFACCEL, "SWS: Select only track 16" },							 		 "SWS_SEL16",		   SelectTrack,		 NULL, 16 },
	{ { DEFACCEL, "SWS: Select only track 17" },							 		 "SWS_SEL17",		   SelectTrack,		 NULL, 17 },
	{ { DEFACCEL, "SWS: Select only track 18" },							 		 "SWS_SEL18",		   SelectTrack,		 NULL, 18 },
	{ { DEFACCEL, "SWS: Select only track 19" },							 		 "SWS_SEL19",		   SelectTrack,		 NULL, 19 },
	{ { DEFACCEL, "SWS: Select only track 20" },							 		 "SWS_SEL20",		   SelectTrack,		 NULL, 20 },
	{ { DEFACCEL, "SWS: Select only track 21" },							 		 "SWS_SEL21",		   SelectTrack,		 NULL, 21 },
	{ { DEFACCEL, "SWS: Select only track 22" },							 		 "SWS_SEL22",		   SelectTrack,		 NULL, 22 },
	{ { DEFACCEL, "SWS: Select only track 23" },							 		 "SWS_SEL23",		   SelectTrack,		 NULL, 23 },
	{ { DEFACCEL, "SWS: Select only track 24" },							 		 "SWS_SEL24",		   SelectTrack,		 NULL, 24 },
	{ { DEFACCEL, "SWS: Select only track 25" },							 		 "SWS_SEL25",		   SelectTrack,		 NULL, 25 },
	{ { DEFACCEL, "SWS: Select only track 26" },							 		 "SWS_SEL26",		   SelectTrack,		 NULL, 26 },
	{ { DEFACCEL, "SWS: Select only track 27" },							 		 "SWS_SEL27",		   SelectTrack,		 NULL, 27 },
	{ { DEFACCEL, "SWS: Select only track 28" },							 		 "SWS_SEL28",		   SelectTrack,		 NULL, 28 },
	{ { DEFACCEL, "SWS: Select only track 29" },							 		 "SWS_SEL29",		   SelectTrack,		 NULL, 29 },
	{ { DEFACCEL, "SWS: Select only track 30" },							 		 "SWS_SEL30",		   SelectTrack,		 NULL, 30 },
	{ { DEFACCEL, "SWS: Select only track 31" },							 		 "SWS_SEL31",		   SelectTrack,		 NULL, 31 },
	{ { DEFACCEL, "SWS: Select only track 32" },							 		 "SWS_SEL32",		   SelectTrack,		 NULL, 32 },

#ifdef TESTCODE
	{ { DEFACCEL, "SWS: Test" }, "SWS_TEST",  TestFunc, NULL, },
#endif

	{ {}, LAST_COMMAND, }, // Denote end of table
};

int FreezeInit()
{
	SWSRegisterCommands(g_commandTable);
	if (!plugin_register("projectconfig",&g_projectconfig))
		return 0;

	/* Add indent and unindent to track context menu?
	int i = -1;
	while (g_commandTable[++i].id != LAST_COMMAND)
		if (g_commandTable[i].doCommand == IndentTracks || g_commandTable[i].doCommand == UnindentTracks)
			AddToMenu(hTrackMenu, g_commandTable[i].menuText, g_commandTable[i].accel.accel.cmd);*/

	g_prevautoxfade = *(int*)GetConfigVar("autoxfade");

	return 1;
}