/******************************************************************************
/ ItemSel.cpp
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

//JFB most of actions in there do not set undo points: normal?

#include "stdafx.h"
#include "ItemSel.h"


void UnselOnTracks(COMMAND_T* = NULL)
{
	PreventUIRefresh(1);
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
				GetSetMediaItemInfo(GetTrackMediaItem(tr, j), "B_UISEL", &g_bFalse);
	}
	PreventUIRefresh(-1);
	UpdateArrange();
}

void SelLLItem(COMMAND_T* = NULL)
{
	PreventUIRefresh(1);
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
	PreventUIRefresh(-1);
	UpdateArrange();
}

void SelULItem(COMMAND_T* = NULL)
{
	PreventUIRefresh(1);
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
	PreventUIRefresh(-1);
	UpdateArrange();
}

void UnselULItem(COMMAND_T* = NULL)
{
	PreventUIRefresh(1);
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
	PreventUIRefresh(-1);
	UpdateArrange();
}

void TogItemSel(COMMAND_T* = NULL)
{	// Toggle item's selection states on selected tracks
	PreventUIRefresh(1);
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
	PreventUIRefresh(-1);
	UpdateArrange();
}

void SelNextItem(COMMAND_T* ctx)
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
						if (ctx->user == 0)
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

void SelPrevItem(COMMAND_T* ctx)
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
						if (ctx->user == 0)
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

void SelMutedItems(COMMAND_T* ct)
{
	PreventUIRefresh(1);
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* mi = GetTrackMediaItem(tr, j);
			bool sel = *(bool*)GetSetMediaItemInfo(mi, "B_MUTE", NULL);
			if ((int)ct->user == 0) sel = !sel;
			GetSetMediaItemInfo(mi, "B_UISEL", &sel);
		}
	}
	PreventUIRefresh(-1);
	UpdateArrange();
}

void SelMutedItemsSel(COMMAND_T* ct)
{
	PreventUIRefresh(1);
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
			{
				MediaItem* mi = GetTrackMediaItem(tr, j);
				bool sel = *(bool*)GetSetMediaItemInfo(mi, "B_MUTE", NULL);
				if ((int)ct->user == 0) sel = !sel;
				GetSetMediaItemInfo(mi, "B_UISEL", &sel);
			}
	}
	PreventUIRefresh(-1);
	UpdateArrange();
}

void SelLockedItems(COMMAND_T* = NULL)
{
	PreventUIRefresh(1);
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
	PreventUIRefresh(-1);
	UpdateArrange();
}

void SelLockedItemsSel(COMMAND_T* = NULL)
{
	PreventUIRefresh(1);
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
	PreventUIRefresh(-1);
	UpdateArrange();
}

void AddRightItem(COMMAND_T* = NULL)
{
	PreventUIRefresh(1);
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
				if (dLeft2 > dLeft && dRight2 > dRight && dRight2 < dMinRight)
				{
					dMinRight = dRight2;
					rightItem = mi;
				}
			}
			if (rightItem)
				GetSetMediaItemInfo(rightItem, "B_UISEL", &g_i1);
		}
	}
	PreventUIRefresh(-1);
	UpdateArrange();
}

void AddLeftItem(COMMAND_T* = NULL)
{
	PreventUIRefresh(1);
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
			double dLeft    = *(double*)GetSetMediaItemInfo((MediaItem*)selItems.Get(j), "D_POSITION", NULL);
			double dRight   = *(double*)GetSetMediaItemInfo((MediaItem*)selItems.Get(j), "D_LENGTH", NULL) + dLeft;
			double dMaxLeft = -DBL_MAX;
			MediaItem* leftItem = NULL;
			for (int k = 0; k < GetTrackNumMediaItems(tr); k++)
			{
				MediaItem* mi;
				mi = GetTrackMediaItem(tr, k);
				double dLeft2  = *(double*)GetSetMediaItemInfo(mi, "D_POSITION", NULL);
				double dRight2 = *(double*)GetSetMediaItemInfo(mi, "D_LENGTH", NULL) + dLeft2;
				if (dLeft2 < dLeft && dRight2 < dRight && dLeft2 > dMaxLeft)
				{
					dMaxLeft = dLeft2;
					leftItem = mi;
				}
			}
			if (leftItem)
				GetSetMediaItemInfo(leftItem, "B_UISEL", &g_i1);
		}
	}
	PreventUIRefresh(-1);
	UpdateArrange();
}

void UnselNotStem(COMMAND_T* = NULL)
{
	PreventUIRefresh(1);
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* mi = GetTrackMediaItem(tr, j);
			for (int k = 0; k < GetMediaItemNumTakes(mi); k++)
			{
				PCM_source* src = (PCM_source*)GetSetMediaItemTakeInfo(GetMediaItemTake(mi, k), "P_SOURCE", NULL);
				if (src && src->GetFileName() && !strstr(src->GetFileName(), "stems"))
					GetSetMediaItemInfo(mi, "B_UISEL", &g_bFalse);
			}
		}
	}
	PreventUIRefresh(-1);
	UpdateArrange();
}

void UnselNotRender(COMMAND_T* = NULL)
{
	PreventUIRefresh(1);
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* mi = GetTrackMediaItem(tr, j);
			if (GetMediaItemNumTakes(mi))
			{
				PCM_source* src = (PCM_source*)GetSetMediaItemTakeInfo(GetMediaItemTake(mi, -1), "P_SOURCE", NULL);
				if (src && src->GetFileName() && !strstr(src->GetFileName(), "render"))
					GetSetMediaItemInfo(mi, "B_UISEL", &g_bFalse);
			}
		}
	}
	PreventUIRefresh(-1);
	UpdateArrange();
}

//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "SWS: Unselect all items on selected track(s)" },							"SWS_UNSELONTRACKS",	UnselOnTracks,		},
	{ { DEFACCEL, "SWS: Select lower-leftmost item on selected track(s)" },					"SWS_SELLLI",			SelLLItem,			},
	{ { DEFACCEL, "SWS: Select upper-leftmost item on selected track(s)" },					"SWS_SELULI",			SelULItem,			},
	{ { DEFACCEL, "SWS: Unselect upper-leftmost item on selected track(s)" },				"SWS_UNSELULI",			UnselULItem,		},
	{ { DEFACCEL, "SWS: Toggle selection of items on selected track(s)" },					"SWS_TOGITEMSEL",		TogItemSel,			},
	{ { DEFACCEL, "SWS: Select muted items" },												"SWS_SELMUTEDITEMS",	SelMutedItems,		NULL, 1, },
	{ { DEFACCEL, "SWS: Select unmuted items" },											"SWS_SELUNMUTEDITEMS",	SelMutedItems,		NULL, 0, },
	{ { DEFACCEL, "SWS: Select muted items on selected track(s)" },							"SWS_SELMUTEDITEMS2",	SelMutedItemsSel,	NULL, 1, },
	{ { DEFACCEL, "SWS: Select unmuted items on selected track(s)" },						"SWS_SELUNMUTEDITEMS2",	SelMutedItemsSel,	NULL, 0, },
	{ { DEFACCEL, "SWS: Select next item (across tracks)" },								"SWS_SELNEXTITEM",		SelNextItem,		NULL, 0, },
	{ { DEFACCEL, "SWS: Select previous item (across tracks)" },							"SWS_SELPREVITEM",		SelPrevItem,		NULL, 0, },
	{ { DEFACCEL, "SWS: Select next item, keeping current selection (across tracks)" },		"SWS_SELNEXTITEM2",		SelNextItem,		NULL, 1 },
	{ { DEFACCEL, "SWS: Select previous item, keeping current selection (across tracks)" },	"SWS_SELPREVITEM2",		SelPrevItem,		NULL, 1 },
	{ { DEFACCEL, "SWS: Select locked items" },												"SWS_SELLOCKITEMS",		SelLockedItems,		},
	{ { DEFACCEL, "SWS: Select locked items on selected track(s)" },						"SWS_SELLOCKITEMS2",	SelLockedItemsSel,	},
	{ { DEFACCEL, "SWS: Add item(s) to right of selected item(s) to selection" },			"SWS_ADDRIGHTITEM",		AddRightItem,		},
	{ { DEFACCEL, "SWS: Add item(s) to left of selected item(s) to selection" },			"SWS_ADDLEFTITEM",		AddLeftItem,		},
	{ { DEFACCEL, "SWS: Unselect items without 'stems' in source filename" },				"SWS_UNSELNOTSTEM",		UnselNotStem,		},
	{ { DEFACCEL, "SWS: Unselect items without 'render' in source filename" },				"SWS_UNSELNOTRENDER",	UnselNotRender,		},

	{ {}, LAST_COMMAND, }, // Denote end of table
};
//!WANT_LOCALIZE_1ST_STRING_END

int ItemSelInit()
{
	SWSRegisterCommands(g_commandTable);

	return 1;
}
