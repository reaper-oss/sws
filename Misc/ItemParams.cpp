/******************************************************************************
/ ItemParams.cpp
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
#include "ItemParams.h"
#include "TrackSel.h"

void TogItemMute(COMMAND_T* = NULL)
{	// Toggle item's mutes on selected tracks
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
			{
				MediaItem* mi = GetTrackMediaItem(tr, j);
				bool bMute = !*(bool*)GetSetMediaItemInfo(mi, "B_MUTE", NULL);
				GetSetMediaItemInfo(mi, "B_MUTE", &bMute);
			}
	}
	UpdateTimeline();
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

// I rarely want to toggle the loop item section, I just want to reset it!
// So here, check it's state, then set/reset it, and also make sure loop item is on.
void LoopItemSection(COMMAND_T*)
{
	WDL_PtrList<void> items;
	WDL_PtrList<void> sections;
	for (int i = 0; i < CountSelectedMediaItems(0); i++)
	{
		MediaItem* item = GetSelectedMediaItem(0, i);
		items.Add(item);
		MediaItem_Take* take = GetMediaItemTake(item, -1);
		if (take && strcmp(((PCM_source*)GetSetMediaItemTakeInfo(take, "P_SOURCE", NULL))->GetType(), "SECTION") == 0)
			sections.Add(item);
	}

	if (sections.GetSize())
	{	// Some items already are in "section mode", turn it off
		Main_OnCommand(40289, 0); // Unselect all items
		for (int i = 0; i < sections.GetSize(); i++)
		{
			MediaItem* item = (MediaItem*)sections.Get(i);
			GetSetMediaItemInfo(item, "B_UISEL", &g_bTrue);
			Main_OnCommand(40547, 0); // Toggle loop section of item source
			GetSetMediaItemInfo(item, "B_UISEL", &g_bFalse);
		}
		// Restore item selection
		for (int i = 0; i < items.GetSize(); i++)
			GetSetMediaItemInfo((MediaItem*)items.Get(i), "B_UISEL", &g_bTrue);
	}

	// Turn on loop item source
	for (int i = 0; i < items.GetSize(); i++)
		GetSetMediaItemInfo((MediaItem*)items.Get(i), "B_LOOPSRC", &g_bTrue);

	// Turn on loop section
	Main_OnCommand(40547, 0);
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

void InsertFromTrackName(COMMAND_T*)
{
	WDL_PtrList<void> tracks;
	for (int i = 0; i < CountSelectedTracks(0); i++)
		tracks.Add(GetSelectedTrack(0, i));

	if (!tracks.GetSize())
		return;

	char cPath[256];
	EnumProjects(-1, cPath, 256);
	char* pSlash = strrchr(cPath, PATH_SLASH_CHAR);
	if (pSlash)
		pSlash[1] = 0;
	
	double dCurPos = GetCursorPosition();
	for (int i = 0; i < tracks.GetSize(); i++)
	{
		ClearSelected();
		MediaTrack* tr = (MediaTrack*)tracks.Get(i);
		GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i1);
		SetLTT();
		WDL_String cFilename;
		cFilename.Set((char*)GetSetMediaTrackInfo(tr, "P_NAME", NULL));
		if (cFilename.GetLength())
		{
			if (FileExists(cFilename.Get()))
				InsertMedia(cFilename.Get(), 0);
			else
			{
				cFilename.Insert(cPath, 0);
				if (FileExists(cFilename.Get()))
					InsertMedia(cFilename.Get(), 0);
			}
			SetEditCurPos(dCurPos, false, false);
		}
	}
	// Restore selected
	for (int i = 0; i < tracks.GetSize(); i++)
		GetSetMediaTrackInfo((MediaTrack*)tracks.Get(i), "I_SELECTED", &g_i1);
}

static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "SWS: Toggle mute of items on selected track(s)" },			"SWS_TOGITEMMUTE",		TogItemMute,			},
	{ { DEFACCEL, "SWS: Delete all items on selected track(s)" },				"SWS_DELALLITEMS",		DelAllItems,			},
	{ { DEFACCEL, "SWS: Loop section of selected item(s)" },					"SWS_LOOPITEMSECTION",	LoopItemSection,		},
	{ { DEFACCEL, "SWS: Move selected item(s) left edge to edit cursor" },		"SWS_ITEMLEFTTOCUR",	MoveItemLeftToCursor,	},
	{ { DEFACCEL, "SWS: Move selected item(s) right edge to edit cursor" },		"SWS_ITEMRIGHTTOCUR",	MoveItemRightToCursor,	},
	{ { DEFACCEL, "SWS: Insert file matching selected track(s) name" },			"SWS_INSERTFROMTN",		InsertFromTrackName,	},

	{ {}, LAST_COMMAND, }, // Denote end of table
};

int ItemParamsInit()
{
	SWSRegisterCommands(g_commandTable);

	return 1;
}