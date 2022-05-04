/******************************************************************************
/ FolderActions.cpp
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

#include "FolderActions.h"
#include "TrackSel.h"

#include <WDL/localize/localize.h>

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
		Undo_OnStateChangeEx(__LOCALIZE("Set selected track(s) to same folder as previous track","sws_undo"), UNDO_STATE_TRACKCFG | UNDO_STATE_MISCCFG, -1);
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
		Undo_OnStateChangeEx(__LOCALIZE("Make folder from selected tracks","sws_undo"), UNDO_STATE_TRACKCFG | UNDO_STATE_MISCCFG, -1);
}

void IndentTracks(const int increment)
{
	bool undo = false;
	int i = 1, depthChange = increment;
	double depth = 0, delta;
	MediaTrack *tracks[2];
	MediaTrack **prevTrack = &tracks[0], **track = &tracks[1];

	if (increment < 0) {
		// the previous track's depth must be set last to avoid issue #1275
		std::swap(prevTrack, track);
		depthChange = increment * -1;
	}

	for (
		*prevTrack = GetTrack(nullptr, 0);
		(*track = GetTrack(nullptr, i++));
		*prevTrack = *track
	) {
		depth += GetMediaTrackInfo_Value(*prevTrack, "I_FOLDERDEPTH");

		if (!GetMediaTrackInfo_Value(*track, "I_SELECTED"))
			continue;
		else if (increment < 0 && depth < 1) // the track is not indented, cannot unindent more
			continue;

		delta = GetMediaTrackInfo_Value(tracks[0], "I_FOLDERDEPTH") + depthChange;
		if (increment > 0 && delta > 1) // the track is already indented, it must not be indented more
			continue;
		SetMediaTrackInfo_Value(tracks[0], "I_FOLDERDEPTH", delta);

		delta = GetMediaTrackInfo_Value(tracks[1], "I_FOLDERDEPTH") - depthChange;
		SetMediaTrackInfo_Value(tracks[1], "I_FOLDERDEPTH", delta);

		depth += increment;
		undo = true;
	}

	if (undo)
		Undo_OnStateChangeEx(__LOCALIZE("Unindent selected tracks","sws_undo"), UNDO_STATE_TRACKCFG | UNDO_STATE_MISCCFG, -1);
}

void IndentTracks(COMMAND_T *cmd)
{
	IndentTracks(static_cast<int>(cmd->user));
}

void CollapseFolder(COMMAND_T* ct)
{
	MediaTrack* gfd = NULL;
	int iCompact = (int)ct->user;
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		int iType;
		GetFolderDepth(tr, &iType, &gfd);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL) && iType == 1)
			GetSetMediaTrackInfo(tr, "I_FOLDERCOMPACT", &iCompact);
	}
	UpdateTimeline();
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG | UNDO_STATE_MISCCFG, -1);
}

//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "SWS: Mute children of selected folder(s)" },						"SWS_MUTECHILDREN",		MuteChildren,     },
	{ { DEFACCEL, "SWS: Unmute children of selected folder(s)" },					"SWS_UNMUTECHILDREN",	UnmuteChildren,   },
	{ { DEFACCEL, "SWS: Toggle mute of children of selected folder(s)" },			"SWS_TOGMUTECHILDRN",	TogMuteChildren,  },
	{ { DEFACCEL, "SWS: Set selected track(s) to same folder as previous track" },	"SWS_FOLDERPREV",		FolderLikePrev,   },
	{ { DEFACCEL, "SWS: Make folder from selected tracks" },						"SWS_MAKEFOLDER",		MakeFolder,       },
	{ { DEFACCEL, "SWS: Indent selected track(s)" },								"SWS_INDENT",			IndentTracks,     "SWS Indent track(s)", 1 },
	{ { DEFACCEL, "SWS: Unindent selected track(s)" },								"SWS_UNINDENT",			IndentTracks,   "SWS Unindent track(s)", -1 },
	{ { DEFACCEL, "SWS: Set selected folder(s) collapsed" },						"SWS_COLLAPSE",			CollapseFolder, NULL, 2, },
	{ { DEFACCEL, "SWS: Set selected folder(s) uncollapsed" },						"SWS_UNCOLLAPSE",		CollapseFolder, NULL, 0, },
	{ { DEFACCEL, "SWS: Set selected folder(s) small" },							"SWS_FOLDSMALL",		CollapseFolder, NULL, 1, },

	{ {}, LAST_COMMAND, }, // Denote end of table
};
//!WANT_LOCALIZE_1ST_STRING_END

int FolderActionsInit()
{
	SWSRegisterCommands(g_commandTable);

	/* Add indent and unindent to track context menu?
	int i = -1;
	while (g_commandTable[++i].id != LAST_COMMAND)
		if (g_commandTable[i].doCommand == IndentTracks || g_commandTable[i].doCommand == UnindentTracks)
			AddToMenu(hTrackMenu, g_commandTable[i].menuText, g_commandTable[i].cmdId);*/

	return 1;
}
