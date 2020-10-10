/******************************************************************************
/ TrackSel.cpp
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
#include "TrackSel.h"

static SWSProjConfig<WDL_PtrList_DOD<GUID> > g_pSelTracks;
void SaveTracks(WDL_TypedBuf<MediaTrack*>* tracks)
{
	g_pSelTracks.Cleanup();
	g_pSelTracks.Get()->Empty(true);
	for (int i = 0; i < tracks->GetSize(); i++)
	{
		GUID* g = new GUID;
		*g = *(GUID*)GetSetMediaTrackInfo(tracks->Get()[i], "GUID", NULL);
		g_pSelTracks.Get()->Add(g);
	}
}

void SaveSelTracks(COMMAND_T*)
{
	WDL_TypedBuf<MediaTrack*> selTracks;
	SWS_GetSelectedTracks(&selTracks, true);
	SaveTracks(&selTracks);
}

void RestoreSelTracks(COMMAND_T*)
{
	ClearSelected();
	for (int i = 0; i < g_pSelTracks.Get()->GetSize(); i++)
		for (int j = 0; j <= GetNumTracks(); j++)
		{
			MediaTrack* tr = CSurf_TrackFromID(j, false);
			if (TrackMatchesGuid(tr, g_pSelTracks.Get()->Get(i)))
				GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i1);
		}
}

void TogSaveSelTracks(COMMAND_T* ct)
{
	WDL_TypedBuf<MediaTrack*> selTracks;
	SWS_GetSelectedTracks(&selTracks, true);
	RestoreSelTracks(ct);
	SaveTracks(&selTracks);
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

void SelChildren(COMMAND_T*)			{ SelectFolderTracks(-1, 1, false); }
void SelChildrenOnly(COMMAND_T* = NULL)	{ SelectFolderTracks(-1, 1, true); }
void SelParents(COMMAND_T* = NULL)		{ SelectFolderTracks(1, -1, false); }
void SelParentsOnly(COMMAND_T* = NULL)	{ SelectFolderTracks(1, -1, true); }
void UnselParents(COMMAND_T* = NULL)	{ SelectFolderTracks(0, -1, false); }
void UnselChildren(COMMAND_T* = NULL)	{ SelectFolderTracks(-1, 0, false); }

void TogTrackSel(COMMAND_T*)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		GetSetMediaTrackInfo(tr, "I_SELECTED", *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL) ? &g_i0 : &g_i1);
	}
}

void SelTracksWItems(COMMAND_T*)
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

void SetLTT(COMMAND_T*)
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

void SelNearestNextFolder(COMMAND_T* = NULL) // #981
{
	MediaTrack* selTr = GetSelectedTrack(0, 0);
	if (selTr)
	{
		int nextTrID = (int)GetMediaTrackInfo_Value(selTr, "IP_TRACKNUMBER") + 1;

		for (; nextTrID <= GetNumTracks(); nextTrID++)
		{
			MediaTrack* tr = CSurf_TrackFromID(nextTrID, false);
			if ((int)GetMediaTrackInfo_Value(tr, "I_FOLDERDEPTH") == 1)
			{
				ClearSelected();
				GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i1);
				return;
			}
		}
	}
}

void SelNearestPrevFolder(COMMAND_T* = NULL) // #981
{
	{
		MediaTrack* selTr = GetSelectedTrack(0, 0);
		if (selTr)
		{
			int prevTrID = (int)GetMediaTrackInfo_Value(selTr, "IP_TRACKNUMBER") - 1;

			for (; prevTrID > 0; prevTrID--)
			{
				MediaTrack* tr = CSurf_TrackFromID(prevTrID, false);
				if ((int)GetMediaTrackInfo_Value(tr, "I_FOLDERDEPTH") == 1)
				{
					ClearSelected();
					GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i1);
					return;
				}
			}
		}
	}
}

// ct->user: 0 == sel unmuted tracks, 1 = sel muted tracks
void SelMutedTracks(COMMAND_T* ct)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		bool bMute = *(bool*)GetSetMediaTrackInfo(tr, "B_MUTE", NULL);
		int iSel = ((ct->user == 1 && bMute) || (ct->user == 0 && !bMute)) ? 1 : 0;
		GetSetMediaTrackInfo(tr, "I_SELECTED", &iSel);
	}
	TrackList_AdjustWindows(false);
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
	WDL_PtrList<void> pSelTracks;
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

void SelectTrack(COMMAND_T* ct)
{
	for (int i = 0; i <= GetNumTracks(); i++)
		GetSetMediaTrackInfo(CSurf_TrackFromID(i, false), "I_SELECTED", (int)ct->user + 1 == i ? &g_i1 : &g_i0);
}

//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "SWS: Save current track selection" },						"SWS_SAVESEL",			SaveSelTracks,		},
	{ { DEFACCEL, "SWS: Restore saved track selection" },						"SWS_RESTORESEL",		RestoreSelTracks,	},
	{ { DEFACCEL, "SWS: Toggle between current and saved track selection" },	"SWS_TOGSAVESEL",		TogSaveSelTracks,	}, 
	{ { DEFACCEL, "SWS: Toggle (invert) track selection" },						"SWS_TOGTRACKSEL",		TogTrackSel,		},
	{ { DEFACCEL, "SWS: Select only track(s) with selected item(s)" },			"SWS_SELTRKWITEM",		SelTracksWItems,	},
	{ { DEFACCEL, "SWS: Set last touched track to match track selection (deprecated)" },		"SWS_SETLTT",			SetLTT,				},
	
	// Folder/parent/child selection
	{ { DEFACCEL, "SWS: Select only children of selected folders" },			"SWS_SELCHILDREN",		SelChildrenOnly,	},
	{ { DEFACCEL, "SWS: Select children of selected folder track(s)" },			"SWS_SELCHILDREN2",		SelChildren,		},
	{ { DEFACCEL, "SWS: Select only parent(s) of selected folder track(s)" },	"SWS_SELPARENTS",		SelParentsOnly,		},
	{ { DEFACCEL, "SWS: Select parent(s) of selected folder track(s)" },		"SWS_SELPARENTS2",		SelParents,			},
	{ { DEFACCEL, "SWS: Unselect parent(s) of selected folder track(s)" },		"SWS_UNSELPARENTS",		UnselParents,		},
	{ { DEFACCEL, "SWS: Unselect children of selected folder track(s)" },		"SWS_UNSELCHILDREN",	UnselChildren,		},
	{ { DEFACCEL, "SWS: Select all folders (parents only)" },					"SWS_SELALLPARENTS",	SelAllParents,		},
	{ { DEFACCEL, "SWS: Select all folder start tracks" },						"SWS_SELFOLDSTARTS",	SelFolderStarts,	},
	{ { DEFACCEL, "SWS: Select all non-folders" },								"SWS_SELNOTFOLDER",		SelNotFolder,		},
	{ { DEFACCEL, "SWS: Select next folder" },									"SWS_SELNEXTFOLDER",	SelNextFolder,		},
	{ { DEFACCEL, "SWS: Select previous folder" },								"SWS_SELPREVFOLDER",	SelPrevFolder,		},
	{ { DEFACCEL, "SWS: Select nearest next folder" },									"SWS_SELNEARESTNEXTFOLDER",	SelNearestNextFolder, },
	{ { DEFACCEL, "SWS: Select nearest previous folder" },									"SWS_SELNEARESTPREVFOLDER",	SelNearestPrevFolder, },
	
	// Sel based on states
	{ { DEFACCEL, "SWS: Select muted tracks" },										"SWS_SELMUTEDTRACKS",	SelMutedTracks, NULL, 1 },
	{ { DEFACCEL, "SWS: Select unmuted tracks" },									"SWS_SELUNMUTEDTRACKS",	SelMutedTracks, NULL, 0	},
	{ { DEFACCEL, "SWS: Select soloed tracks" },									"SWS_SELSOLOEDTRACKS",	SelSoloedTracks,	},
	{ { DEFACCEL, "SWS: Select tracks with flipped phase" },						"SWS_SELPHASETRACKS",	SelPhaseTracks,		},
	{ { DEFACCEL, "SWS: Select armed tracks" },										"SWS_SELARMEDTRACKS",	SelArmedTracks,		},
	{ { DEFACCEL, "SWS: Select tracks with active routing to selected track(s)" },	"SWS_SELROUTED",		SelRouted,			},
	{ { DEFACCEL, "SWS: Select only rec armed track(s)" },							"SWS_SELRECARM",		SelArmed,			},
	{ { DEFACCEL, "SWS: Unselect rec armed track(s)" },								"SWS_UNSELRECARM",		UnselArmed,			},

	// Master track
	{ { DEFACCEL, "SWS: Select master track" },									"SWS_SELMASTER",		SelMaster,			},
	{ { DEFACCEL, "SWS: Unselect master track" },								"SWS_UNSELMASTER",		UnselMaster,		},
	{ { DEFACCEL, "SWS: Toggle master track select" },							"SWS_TOGSELMASTER",		TogSelMaster,		},

	{ {}, LAST_COMMAND, }, // Denote end of table
};
//!WANT_LOCALIZE_1ST_STRING_END

int TrackSelInit()
{
	SWSRegisterCommands(g_commandTable);
	return 1;
}
