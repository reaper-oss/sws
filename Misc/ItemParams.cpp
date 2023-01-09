/******************************************************************************
/ ItemParams.cpp
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

#include "ItemParams.h"
#include "TrackSel.h"
#include "../Breeder/BR_Util.h"

#include <WDL/localize/localize.h>

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
	Undo_OnStateChangeEx(__LOCALIZE("Delete all items on selected tracks","sws_undo"), UNDO_STATE_ITEMS, -1);
}

// I rarely want to toggle the loop item section, I just want to reset it!
// So here, check it's state, then set/reset it, and also make sure loop item is on.
void LoopItemSection(COMMAND_T*)
{
	WDL_PtrList<void> items;
	WDL_PtrList<void> sections;
	const int cnt=CountSelectedMediaItems(NULL);
	for (int i = 0; i < cnt; i++)
	{
		MediaItem* item = GetSelectedMediaItem(0, i);
		items.Add(item);
		MediaItem_Take* take = GetMediaItemTake(item, -1);
		if (take && strcmp(((PCM_source*)GetSetMediaItemTakeInfo(take, "P_SOURCE", NULL))->GetType(), "SECTION") == 0)
			sections.Add(item);
	}

	PreventUIRefresh(1);
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

	PreventUIRefresh(-1);

	// Turn on loop section
	Main_OnCommand(40547, 0); // takes care of UI refresh
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
	Undo_OnStateChangeEx(__LOCALIZE("Move selected items left edge to edit cursor","sws_undo"), UNDO_STATE_ITEMS, -1);
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
	Undo_OnStateChangeEx(__LOCALIZE("Move selected items right edge to edit cursor","sws_undo"), UNDO_STATE_ITEMS, -1);
}

void InsertFromTrackName(COMMAND_T*)
{
	WDL_PtrList<void> tracks;
	const int trSelCnt = CountSelectedTracks(NULL);
	for (int i = 0; i < trSelCnt; i++)
		tracks.Add(GetSelectedTrack(NULL, i));

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

double QuantizeTime(double dTime, double dMin, double dMax)
{
	double dNewTime = GetClosestGridDiv(dTime);
	while (dNewTime <= dMin)
		dNewTime = GetNextGridDiv(dNewTime);

	while (dNewTime >= dMax)
		dNewTime = GetPrevGridDiv(dNewTime);

	return dNewTime;
}

// 1: Quant start, keep len
// 2: Quant start, keep end
// 3: Quant end, keep len
// 4: Quant end, keep start
// 5: Quant both
void QuantizeItemEdges(COMMAND_T* t)
{
	// Eventually a dialog?  for now quantize to grid
	const int cnt=CountSelectedMediaItems(NULL);
	for (int i = 0; i < cnt; i++)
	{
		MediaItem* item = GetSelectedMediaItem(NULL, i);
		double dStart = *(double*)GetSetMediaItemInfo(item, "D_POSITION", NULL);
		double dOffset = *(double*)GetSetMediaItemInfo(item, "D_SNAPOFFSET", NULL);
		double dLen   = *(double*)GetSetMediaItemInfo(item, "D_LENGTH", NULL);
		double dEnd   = dStart + dLen;

		// Correct for snap offset
		dStart += dOffset;
		dLen -= dOffset;

		switch(t->user)
		{
		case 1:
			dStart = QuantizeTime(dStart, -DBL_MAX, dEnd);
			break;
		case 2:
			dStart = QuantizeTime(dStart, -DBL_MAX, dEnd);
			dLen   = dEnd - dStart;
			break;
		case 3:
			dEnd = QuantizeTime(dEnd, dStart, DBL_MAX);
			dStart = dEnd - dLen;
			break;
		case 4:
			dEnd = QuantizeTime(dEnd, dStart, DBL_MAX);
			dLen = dEnd - dStart;
			break;
		case 5:
			dStart = QuantizeTime(dStart, -DBL_MAX, dEnd);
			dEnd = QuantizeTime(dEnd, dStart, DBL_MAX);
			dLen = dEnd - dStart;
			break;
		}

		// Uncorrect for the snap offset
		dStart -= dOffset;
		dLen += dOffset;

		GetSetMediaItemInfo(item, "D_POSITION", &dStart);
		GetSetMediaItemInfo(item, "D_LENGTH",	&dLen);
	}
	UpdateTimeline();
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
}

void SetChanModeAllTakes(COMMAND_T* t)
{
	int mode = (int)t->user;
	const int cnt=CountSelectedMediaItems(NULL);
	for (int i = 0; i < cnt; i++)
	{
		MediaItem* item = GetSelectedMediaItem(NULL, i);
		for (int j = 0; j < CountTakes(item); j++)
			GetSetMediaItemTakeInfo(GetTake(item, j), "I_CHANMODE", &mode);
	}

	UpdateTimeline();
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
}

void SetPreservePitch(COMMAND_T* t)
{
	bool bPP = t->user ? true : false;
	const int cnt=CountSelectedMediaItems(NULL);
	for (int i = 0; i < cnt; i++)
	{
		MediaItem* item = GetSelectedMediaItem(NULL, i);
		for (int j = 0; j < CountTakes(item); j++)
			GetSetMediaItemTakeInfo(GetTake(item, j), "B_PPITCH", &bPP);
	}

	UpdateTimeline();
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
}

void SetPitch(COMMAND_T* t)
{
	const int cnt=CountSelectedMediaItems(NULL);
	for (int i = 0; i < cnt; i++)
	{
		MediaItem* item = GetSelectedMediaItem(NULL, i);
		for (int j = 0; j < CountTakes(item); j++)
		{
			double dPitch = (double)t->user / 100.0;
			if (dPitch != 0.0)
				dPitch += *(double*)GetSetMediaItemTakeInfo(GetTake(item, j), "D_PITCH", NULL);
			GetSetMediaItemTakeInfo(GetTake(item, j), "D_PITCH", &dPitch);
		}
	}

	UpdateTimeline();
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
}

// Original code by FNG/fingers
// Modified by SWS to change the rate of all takes and to use snap offset
void NudgePlayrate(COMMAND_T *t)
{
	const int cnt=CountSelectedMediaItems(NULL);
	for(int i = 0; i < cnt; i++)
	{
		MediaItem* item = GetSelectedMediaItem(NULL, i);
		double snapOffset = *(double*)GetSetMediaItemInfo(item, "D_SNAPOFFSET", NULL);
		for (int j = 0; j < CountTakes(item); j++)
		{
			MediaItem_Take* take = GetTake(item, j);
			double rate  = *(double*)GetSetMediaItemTakeInfo(take, "D_PLAYRATE", NULL);
			double newRate = 1.0;
			if (t->user != 0)
				newRate = rate * pow(2.0, 1.0/12.0 / (double)t->user);
			GetSetMediaItemTakeInfo(take, "D_PLAYRATE", &newRate);
			GetSetMediaItemTakeInfo(take, "B_PPITCH", &g_bFalse);
			
			// Take into account the snap offset
			if (snapOffset != 0.0)
			{
				// Rate can be clipped by Reaper, re-read it here
				newRate = *(double*)GetSetMediaItemTakeInfo(take, "D_PLAYRATE", NULL);
				double offset = *(double*)GetSetMediaItemTakeInfo(take, "D_STARTOFFS", NULL);
				offset += (1.0 - newRate / rate) * snapOffset * rate;
				if (offset < 0.0)
					offset = 0.0;
				GetSetMediaItemTakeInfo(take, "D_STARTOFFS", &offset);
			}
		}
	}
	UpdateTimeline();
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
}

void SetItemChannels(COMMAND_T* t)
{
	WDL_TypedBuf<MediaItem*> items;
	SWS_GetSelectedMediaItems(&items);
	bool bStereo = abs((int)t->user) == 2;
	for (int i = 0; i < items.GetSize(); i++)
	{
		MediaItem* item = items.Get()[i];
		MediaItem_Take* take = GetActiveTake(item);
		bool bWasStereo = false;
		// Ignore empty takes
		if (take)
		{	
			// Find max # of channels in all takes
			int iMaxNumChannels = 2;
			for (int j = 0; j < GetMediaItemNumTakes(item); j++)
			{
				PCM_source* src = (PCM_source*)GetSetMediaItemTakeInfo(GetMediaItemTake(item, j), "P_SOURCE", NULL);
				if (src && src->GetNumChannels() > iMaxNumChannels)
					iMaxNumChannels = src->GetNumChannels();
			}
			if (bStereo) // Stereo mode requires max channel of the first
				iMaxNumChannels--;

			int iChan = *(int*)GetSetMediaItemTakeInfo(take, "I_CHANMODE", NULL) - 2; // Subtract the normal/rev/down modes
			if (iChan > 64) // Convert to mono
			{
				bWasStereo = true;
				iChan -= 64;
			}

			if (bWasStereo == bStereo)
			{
				if (iChan < 0)
					iChan = 0;

				if (t->user > 0)
					iChan++;
				else
					iChan--;
			}
			else if (iChan < 0 && t->user > 0) // Corner case of next stereo when in a special mode
				iChan = 1;
			else if (t->user > 0 && !bStereo)
				iChan++;
			else if (t->user < 0 && bStereo)
				iChan--;

			// Limit to 1-max
			if (iChan < 1)
				iChan = iMaxNumChannels;
			else if (iChan > iMaxNumChannels)
				iChan = 1;

			iChan += 2; // Convert back into proper form to set
			if (bStereo)
				iChan += 64;


			for (int j = 0; j < GetMediaItemNumTakes(item); j++)
				GetSetMediaItemTakeInfo(GetMediaItemTake(item, j), "I_CHANMODE", &iChan);
		}
	}
	UpdateTimeline();
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
}

void CrossfadeSelItems(COMMAND_T* t)
{
	double dFadeLen = fabs(*ConfigVar<double>("deffadelen")); // Abs because neg value means "not auto"
	double dEdgeAdj = dFadeLen / 2.0;
	bool bChanges = false;

	for (int iTrack = 1; iTrack <= GetNumTracks(); iTrack++)
	{
		MediaTrack* tr = CSurf_TrackFromID(iTrack, false);
		for (int iItem1 = 0; iItem1 < GetTrackNumMediaItems(tr); iItem1++)
		{
			MediaItem* item1 = GetTrackMediaItem(tr, iItem1);
			if (*(bool*)GetSetMediaItemInfo(item1, "B_UISEL", NULL))
			{
				double dStart1 = *(double*)GetSetMediaItemInfo(item1, "D_POSITION", NULL);
				double dEnd1   = *(double*)GetSetMediaItemInfo(item1, "D_LENGTH", NULL) + dStart1;

				// Try to match up the end with the start of the other selected media items
				for (int iItem2 = 0; iItem2 < GetTrackNumMediaItems(tr); iItem2++)
				{
					MediaItem* item2 = GetTrackMediaItem(tr, iItem2);
					if (item1 != item2 && *(bool*)GetSetMediaItemInfo(item2, "B_UISEL", NULL))
					{
						double dStart2 = *(double*)GetSetMediaItemInfo(item2, "D_POSITION", NULL);
						//double dTest = fabs(dEnd1 - dStart2);

						// Need a tolerance for "items are adjacent".
						if (fabs(dEnd1 - dStart2) < SWS_ADJACENT_ITEM_THRESHOLD)
						{	// Found a matching item

							// We're all good, move the edges around and set the crossfades
							double dLen1 = dEnd1 - dStart1 + dEdgeAdj;
							GetSetMediaItemInfo(item1, "D_LENGTH", &dLen1);
							GetSetMediaItemInfo(item1, "D_FADEOUTLEN_AUTO", &dFadeLen);

							double dLen2 = *(double*)GetSetMediaItemInfo(item2, "D_LENGTH", NULL);
							dStart2 -= dEdgeAdj;
							dLen2   += dEdgeAdj;
							GetSetMediaItemInfo(item2, "D_POSITION", &dStart2);
							GetSetMediaItemInfo(item2, "D_LENGTH", &dLen2);
							GetSetMediaItemInfo(item2, "D_FADEINLEN_AUTO", &dFadeLen);
							double dSnapOffset2 = *(double*)GetSetMediaItemInfo(item2, "D_SNAPOFFSET", NULL);
							if (dSnapOffset2)
							{	// Only adjust the snap offset if it's non-zero
								dSnapOffset2 += dEdgeAdj;
								GetSetMediaItemInfo(item2, "D_SNAPOFFSET", &dSnapOffset2);
							}

							AdjustTakesStartOffset(item2, dEdgeAdj);

							bChanges = true;
							break;
						}
					}
				}
			}
		}
	}
	if (bChanges)
	{
		UpdateTimeline();
		Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
	}
}

void SetItemLen(COMMAND_T* t)
{
	WDL_TypedBuf<MediaItem*> items;
	SWS_GetSelectedMediaItems(&items);
	if (items.GetSize() == 0)
		return;

	static double dLen = 1.0;
	char reply[318];
	sprintf(reply, "%f", dLen);
	if (GetUserInputs(__LOCALIZE("Set selected items length","sws_mbox"), 1, __LOCALIZE("New item length (s)","sws_mbox"), reply, sizeof(reply)))
	{
		dLen = atof(reply);
		if (dLen <= 0.0)
			return;

		for (int i = 0; i < items.GetSize(); i++)
			GetSetMediaItemInfo(items.Get()[i], "D_LENGTH", &dLen);

		UpdateTimeline();
		Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(t), UNDO_STATE_ITEMS, -1);
	}
}

//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "SWS: Toggle mute of items on selected track(s)" },			"SWS_TOGITEMMUTE",		TogItemMute,			},
	{ { DEFACCEL, "SWS: Delete all items on selected track(s)" },				"SWS_DELALLITEMS",		DelAllItems,			},
	{ { DEFACCEL, "SWS: Loop section of selected item(s)" },					"SWS_LOOPITEMSECTION",	LoopItemSection,		},
	{ { DEFACCEL, "SWS: Move selected item(s) left edge to edit cursor" },		"SWS_ITEMLEFTTOCUR",	MoveItemLeftToCursor,	},
	{ { DEFACCEL, "SWS: Move selected item(s) right edge to edit cursor" },		"SWS_ITEMRIGHTTOCUR",	MoveItemRightToCursor,	},
	{ { DEFACCEL, "SWS: Insert file matching selected track(s) name" },			"SWS_INSERTFROMTN",		InsertFromTrackName,	},
	{ { DEFACCEL, "SWS: Quantize item's start to grid (keep length)" },			"SWS_QUANTITESTART2",	QuantizeItemEdges, NULL, 1 },
	{ { DEFACCEL, "SWS: Quantize item's start to grid (change length)" },		"SWS_QUANTITESTART1",	QuantizeItemEdges, NULL, 2 },
	{ { DEFACCEL, "SWS: Quantize item's end to grid (keep length)" },			"SWS_QUANTITEEND2",		QuantizeItemEdges, NULL, 3 },
	{ { DEFACCEL, "SWS: Quantize item's end to grid (change length)" },			"SWS_QUANTITEEND1",		QuantizeItemEdges, NULL, 4 },
	{ { DEFACCEL, "SWS: Quantize item's edges to grid (change length)" },		"SWS_QUANTITEEDGES",	QuantizeItemEdges, NULL, 5 },
	{ { DEFACCEL, "SWS: Set all takes channel mode to normal" },				"SWS_TAKESCHANMODE0",	SetChanModeAllTakes, NULL, 0 },
	{ { DEFACCEL, "SWS: Set all takes channel mode to reverse stereo" },		"SWS_TAKESCHANMODE1",	SetChanModeAllTakes, NULL, 1 },
	{ { DEFACCEL, "SWS: Set all takes channel mode to mono (downmix)" },		"SWS_TAKESCHANMODE2",	SetChanModeAllTakes, NULL, 2 },
	{ { DEFACCEL, "SWS: Set all takes channel mode to mono (left)" },			"SWS_TAKESCHANMODE3",	SetChanModeAllTakes, NULL, 3 },
	{ { DEFACCEL, "SWS: Set all takes channel mode to mono (right)" },			"SWS_TAKESCHANMODE4",	SetChanModeAllTakes, NULL, 4 },
	{ { DEFACCEL, "SWS: Clear all takes preserve pitch" },						"SWS_TAKESCPREVPITCH",	SetPreservePitch, NULL, 0 },
	{ { DEFACCEL, "SWS: Set all takes preserve pitch" },						"SWS_TAKESSPREVPITCH",	SetPreservePitch, NULL, 1 },
	{ { DEFACCEL, "SWS: Reset all takes' pitch" },								"SWS_TAKESPITCHRESET",	SetPitch, NULL, 0 },
	{ { DEFACCEL, "SWS: Pitch all takes up one cent" },							"SWS_TAKESPITCH_1C",	SetPitch, NULL, 1 },
	{ { DEFACCEL, "SWS: Pitch all takes up one semitone" },						"SWS_TAKESPITCH_1S",	SetPitch, NULL, 100 },
	{ { DEFACCEL, "SWS: Pitch all takes up one octave" },						"SWS_TAKESPITCH_1O",	SetPitch, NULL, 1200 },
	{ { DEFACCEL, "SWS: Pitch all takes down one cent" },						"SWS_TAKESPITCH-1C",	SetPitch, NULL, -1 },
	{ { DEFACCEL, "SWS: Pitch all takes down one semitone" },					"SWS_TAKESPITCH-1S",	SetPitch, NULL, -100 },
	{ { DEFACCEL, "SWS: Pitch all takes down one octave" },						"SWS_TAKESPITCH-1O",	SetPitch, NULL, -1200 },
	{ { DEFACCEL, "SWS: Crossfade adjacent selected items (move edges of adjacent items)" },					"SWS_CROSSFADE",		CrossfadeSelItems, },

	{ { DEFACCEL, "SWS: Increase item rate by ~0.6% (10 cents) preserving length, clear 'preserve pitch'" },	"FNG_NUDGERATEUP",		NudgePlayrate, NULL, 10 },
	{ { DEFACCEL, "SWS: Decrease item rate by ~0.6% (10 cents) preserving length, clear 'preserve pitch'" },	"FNG_NUDGERATEDOWN",	NudgePlayrate, NULL, -10 },
	{ { DEFACCEL, "SWS: Increase item rate by ~6% (one semitone) preserving length, clear 'preserve pitch'" },	"FNG_INCREASERATE",		NudgePlayrate, NULL, 1 },
	{ { DEFACCEL, "SWS: Decrease item rate by ~6% (one semitone) preserving length, clear 'preserve pitch'"},	"FNG_DECREASERATE",		NudgePlayrate, NULL, -1 },
	{ { DEFACCEL, "SWS: Reset item rate, preserving length, clear 'preserve pitch'"},							"SWS_RESETRATE",		NudgePlayrate, NULL, 0 },

	{ { DEFACCEL, "SWS: Set all takes to next mono channel mode"},				"SWS_ITEMCHANMONONEXT",		SetItemChannels, NULL, 1 },
	{ { DEFACCEL, "SWS: Set all takes to prev mono channel mode"},				"SWS_ITEMCHANMONOPREV",		SetItemChannels, NULL, -1 },
	{ { DEFACCEL, "SWS: Set all takes to next stereo channel mode"},			"SWS_ITEMCHANSTEREONEXT",	SetItemChannels, NULL, 2 },
	{ { DEFACCEL, "SWS: Set all takes to prev stereo channel mode"},			"SWS_ITEMCHANSTEREOPREV",	SetItemChannels, NULL, -2 },

	{ { DEFACCEL, "SWS: Set selected items length..."},							"SWS_SETITEMLEN",			SetItemLen, NULL, 0 },

	{ {}, LAST_COMMAND, }, // Denote end of table
};
//!WANT_LOCALIZE_1ST_STRING_END

int ItemParamsInit()
{
	SWSRegisterCommands(g_commandTable);

	return 1;
}
