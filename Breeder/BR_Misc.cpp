/******************************************************************************
/ BR_Misc.cpp
/
/ Copyright (c) 2013-2015 Dominik Martin Drzic
/ http://forum.cockos.com/member.php?u=27094
/ http://github.com/Jeff0S/sws
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
#include "BR_Misc.h"
#include "BR_ContinuousActions.h"
#include "BR_EnvelopeUtil.h"
#include "BR_MidiUtil.h"
#include "BR_MouseUtil.h"
#include "BR_ProjState.h"
#include "BR_Util.h"
#include "../Prompt.h"
#include "../SnM/SnM.h"
#include "../SnM/SnM_Chunk.h"
#include "../SnM/SnM_Dlg.h"
#include "../SnM/SnM_Util.h"
#include "../Xenakios/XenakiosExts.h"
#include "../reaper/localize.h"

/******************************************************************************
* Constants                                                                   *
******************************************************************************/
const char* const ADJUST_PLAYRATE_KEY = "BR - AdjustPlayrate";
const char* const ADJUST_PLAYRATE_WND = "BR - AdjustPlayrateWnd";

/******************************************************************************
* Globals                                                                     *
******************************************************************************/
HWND g_adjustPlayrateWnd = NULL;

/******************************************************************************
* Commands: Misc continuous actions                                           *
******************************************************************************/
static bool g_mousePlaybackRestorePlayState = false;

static void MousePlaybackPlayState (bool play, bool pause, bool rec)
{
	g_mousePlaybackRestorePlayState = false;
}

static bool MousePlaybackInit (COMMAND_T* ct, bool init)
{
	static double s_playPos  = -1;
	static double s_pausePos = -1;
	static vector<pair<GUID,int> >* s_trackSoloMuteState = NULL;
	static vector<pair<GUID,int> >* s_itemMuteState      = NULL;

	if (init)
	{
		if (IsRecording())
			return false;

		BR_MouseInfo mouseInfo(BR_MouseInfo::MODE_ARRANGE                                  |
		                       BR_MouseInfo::MODE_RULER                                    |
							   BR_MouseInfo::MODE_MIDI_EDITOR                              |
							   BR_MouseInfo::MODE_IGNORE_ALL_TRACK_LANE_ELEMENTS_BUT_ITEMS |
							   BR_MouseInfo::MODE_IGNORE_ENVELOPE_LANE_SEGMENT             |
							   (((int)ct->user < -1) ? BR_MouseInfo::MODE_MCP_TCP : 0));

		if ((int)ct->user > 0 && mouseInfo.GetPosition() == -1)
			return false;

		// Get info on which item and track to solo
		MediaItem*  itemToSolo  = NULL;
		MediaTrack* trackToSolo = NULL;

		if (abs((int)ct->user) != 1)
		{
			if (!strcmp(mouseInfo.GetWindow(), "midi_editor") && mouseInfo.GetMidiEditor())
			{
				itemToSolo  = GetMediaItemTake_Item(SWS_MIDIEditor_GetTake(mouseInfo.GetMidiEditor()));
				trackToSolo = GetMediaItem_Track(itemToSolo);
			}
			else
			{
				itemToSolo  = mouseInfo.GetItem();
				trackToSolo = mouseInfo.GetTrack();
			}

			if (abs((int)ct->user) != 3)
				itemToSolo = NULL;

			if (trackToSolo == GetMasterTrack(NULL))
				trackToSolo = NULL;
		}

		PreventUIRefresh(1);

		// Solo track
		if (trackToSolo)
		{
			if (s_trackSoloMuteState)
				delete s_trackSoloMuteState;

			s_trackSoloMuteState = new vector<pair<GUID,int> >;
			if (!s_trackSoloMuteState)
				return false;

			int count = CountTracks(NULL);
			for (int i = 0; i < count; ++i)
			{
				MediaTrack* track = GetTrack(NULL, i);
				int solo = (int)GetMediaTrackInfo_Value(track, "I_SOLO");
				int mute = (int)GetMediaTrackInfo_Value(track, "B_MUTE");
				if (solo != 0 || mute != 0 || track == trackToSolo)
				{
					pair<GUID,int> trackState;
					trackState.first  = *GetTrackGUID(track);
					trackState.second = solo << 8 | mute;
					s_trackSoloMuteState->push_back(trackState);

					SetMediaTrackInfo_Value(track, "I_SOLO", ((track == trackToSolo) ? 1 : 0));
					SetMediaTrackInfo_Value(track, "B_MUTE", 0);
				}
			}
		}

		// Mute all items in soloed track except itemToSolo
		if (itemToSolo && trackToSolo)
		{
			if (s_itemMuteState)
				delete s_itemMuteState;

			s_itemMuteState = new vector<pair<GUID,int> >;
			if (!s_itemMuteState)
				return false;

			int count = CountTrackMediaItems(trackToSolo);
			for (int i = 0; i < count; ++i)
			{
				MediaItem* item = GetTrackMediaItem(trackToSolo, i);
				int mute = (int)GetMediaItemInfo_Value(item, "B_MUTE");
				if (mute == 0 || item == itemToSolo)
				{
					pair<GUID,int> itemState;
					itemState.first  = GetItemGuid(item);
					itemState.second = mute;
					s_itemMuteState->push_back(itemState);

					SetMediaItemInfo_Value(item, "B_MUTE", ((item == itemToSolo) ? 0 : 1));
				}
			}
		}

		PreventUIRefresh(-1);

		s_playPos  = (IsPlaying()) ? GetPlayPositionEx(NULL)   : -1;
		s_pausePos = (IsPaused())  ? GetCursorPositionEx(NULL) : -1;
		g_mousePlaybackRestorePlayState = true;

		StartPlayback(((int)ct->user < 0) ? GetCursorPositionEx(NULL) : mouseInfo.GetPosition());
		RegisterCsurfPlayState(true, MousePlaybackPlayState); // register Csurf after starting playback
		return true;
	}
	else
	{
		PreventUIRefresh(1);

		// Restore tracks' solo and mute state
		if (s_trackSoloMuteState)
		{
			for (size_t i = 0; i < s_trackSoloMuteState->size(); ++i)
			{
				if (MediaTrack* track = GuidToTrack(&s_trackSoloMuteState->at(i).first))
				{
					SetMediaTrackInfo_Value(track, "I_SOLO", s_trackSoloMuteState->at(i).second >> 8);
					SetMediaTrackInfo_Value(track, "B_MUTE", s_trackSoloMuteState->at(i).second &  0xF);
				}
			}
		}

		// Restore items' mute state
		if (s_itemMuteState)
		{
			for (size_t i = 0; i < s_itemMuteState->size(); ++i)
			{
				if (MediaItem* item = GuidToItem(&s_itemMuteState->at(i).first))
					SetMediaItemInfo_Value(item, "B_MUTE", s_itemMuteState->at(i).second);
			}
		}

		RegisterCsurfPlayState(false, MousePlaybackPlayState); // deregister Csurf before setting playstate
		if (g_mousePlaybackRestorePlayState)
		{
			if (s_playPos != -1)
			{
				StartPlayback(s_playPos);
			}
			else if (s_pausePos != -1)
			{
				OnPauseButton();
				SetEditCurPos(s_pausePos, true, false);
			}
			else
			{
				OnStopButton();
			}
		}
		PreventUIRefresh(-1);


		delete s_trackSoloMuteState;
		delete s_itemMuteState;
		s_trackSoloMuteState = NULL;
		s_itemMuteState      = NULL;
		g_mousePlaybackRestorePlayState = true;
		s_playPos  = -1;
		s_pausePos = -1;
		return true;
	}
}

static HCURSOR MousePlaybackCursor (COMMAND_T* ct, int window)
{
	switch (window)
	{
		case BR_ContinuousAction::MAIN_RULER:
		case BR_ContinuousAction::MAIN_ARRANGE:
		case BR_ContinuousAction::MIDI_NOTES_VIEW:
		case BR_ContinuousAction::MIDI_PIANO:
			return GetSwsMouseCursor(CURSOR_MISC_SPEAKER);
	}
	return NULL;
}

static void MousePlayback (COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd)
{
}

void PlaybackAtMouseCursorInit ()
{
	//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
	static COMMAND_T s_commandTable[] =
	{
		{ { DEFACCEL, "SWS/BR: Play from mouse cursor position (perform until shortcut released)" },                                                      "BR_CONT_PLAY_MOUSE",            NULL, NULL, 1, NULL, SECTION_MAIN, MousePlayback},
		{ { DEFACCEL, "SWS/BR: Play from mouse cursor position and solo track under mouse for the duration (perform until shortcut released)" },          "BR_CONT_PLAY_MOUSE_SOLO_TRACK", NULL, NULL, 2, NULL, SECTION_MAIN, MousePlayback},
		{ { DEFACCEL, "SWS/BR: Play from mouse cursor position and solo item and track under mouse for the duration (perform until shortcut released)" }, "BR_CONT_PLAY_MOUSE_SOLO_ITEM",  NULL, NULL, 3, NULL, SECTION_MAIN, MousePlayback},

		{ { DEFACCEL, "SWS/BR: Play from edit cursor position (perform until shortcut released)" },                                                       "BR_CONT_PLAY_EDIT",             NULL, NULL, -1, NULL, SECTION_MAIN, MousePlayback},
		{ { DEFACCEL, "SWS/BR: Play from edit cursor position and solo track under mouse for the duration (perform until shortcut released)" },           "BR_CONT_PLAY_EDIT_SOLO_TRACK",  NULL, NULL, -2, NULL, SECTION_MAIN, MousePlayback},
		{ { DEFACCEL, "SWS/BR: Play from edit cursor position and solo item and track under mouse for the duration (perform until shortcut released)" },  "BR_CONT_PLAY_EDIT_SOLO_ITEM",   NULL, NULL, -3, NULL, SECTION_MAIN, MousePlayback},

		{ { DEFACCEL, "SWS/BR: Play from mouse cursor position (perform until shortcut released)" },                                               "BR_ME_CONT_PLAY_MOUSE",            NULL, NULL, 1, NULL, SECTION_MIDI_EDITOR, MousePlayback},
		{ { DEFACCEL, "SWS/BR: Play from mouse cursor position and solo active item's track for the duration (perform until shortcut released)" }, "BR_ME_CONT_PLAY_MOUSE_SOLO_TRACK", NULL, NULL, 2, NULL, SECTION_MIDI_EDITOR, MousePlayback},
		{ { DEFACCEL, "SWS/BR: Play from mouse cursor position and solo active item for the duration (perform until shortcut released)" },         "BR_ME_CONT_PLAY_MOUSE_SOLO_ITEM",  NULL, NULL, 3, NULL, SECTION_MIDI_EDITOR, MousePlayback},

		{ { DEFACCEL, "SWS/BR: Play from edit cursor position (perform until shortcut released)" },                                                "BR_ME_CONT_PLAY_EDIT",             NULL, NULL, -1, NULL, SECTION_MIDI_EDITOR, MousePlayback},
		{ { DEFACCEL, "SWS/BR: Play from edit cursor position and solo active item's track for the duration (perform until shortcut released)" },  "BR_ME_CONT_PLAY_EDIT_SOLO_TRACK",  NULL, NULL, -2, NULL, SECTION_MIDI_EDITOR, MousePlayback},
		{ { DEFACCEL, "SWS/BR: Play from edit cursor position and solo active item for the duration (perform until shortcut released)" },          "BR_ME_CONT_PLAY_EDIT_SOLO_ITEM",   NULL, NULL, -3, NULL, SECTION_MIDI_EDITOR, MousePlayback},

		{ {}, LAST_COMMAND}
	};
	//!WANT_LOCALIZE_1ST_STRING_END

	int i = -1;
	while (s_commandTable[++i].id != LAST_COMMAND)
		ContinuousActionRegister(new BR_ContinuousAction(&s_commandTable[i], MousePlaybackInit, NULL, MousePlaybackCursor, NULL));
}

/******************************************************************************
* Commands: Misc                                                              *
******************************************************************************/
void SplitItemAtTempo (COMMAND_T* ct)
{
	WDL_TypedBuf<MediaItem*> items;
	SWS_GetSelectedMediaItems(&items);
	if (!items.GetSize() || !CountTempoTimeSigMarkers(NULL) || IsLocked(ITEM_FULL))
		return;

	bool update = false;
	for (int i = 0; i < items.GetSize(); ++i)
	{
		MediaItem* item = items.Get()[i];
		if (!IsItemLocked(item))
		{
			double iStart = GetMediaItemInfo_Value(item, "D_POSITION");
			double iEnd = iStart + GetMediaItemInfo_Value(item, "D_LENGTH");
			double tPos = iStart - 1;

			// Split item currently in the loop
			while (true)
			{
				item = SplitMediaItem(item, tPos);
				if (!item) // split at nonexistent position?
					item = items.Get()[i];
				else
					update = true;

				tPos = TimeMap2_GetNextChangeTime(NULL, tPos);
				if (tPos > iEnd || tPos == -1 )
					break;
			}
		}
	}
	if (update)
	{
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ITEMS, -1);
		UpdateArrange();
	}
}

void SplitItemAtStretchMarkers (COMMAND_T* ct)
{
	WDL_TypedBuf<MediaItem*> items;
	SWS_GetSelectedMediaItems(&items);
	if (!items.GetSize() || IsLocked(ITEM_FULL))
		return;

	bool update = false;
	for (int i = 0; i < items.GetSize(); ++i)
	{
		MediaItem* item = items.Get()[i];
		if (!IsItemLocked(item))
		{
			MediaItem_Take* take = GetActiveTake(item);
			double iStart        = GetMediaItemInfo_Value(item, "D_POSITION");
			double iEnd          = GetMediaItemInfo_Value(item, "D_LENGTH") + iStart;
			double playRate      = GetMediaItemTakeInfo_Value(take, "D_PLAYRATE");

			vector<double> stretchMarkers;
			for (int i = 0; i < GetTakeNumStretchMarkers(take); ++i)
			{
				double position;
				GetTakeStretchMarker(take, i, &position, NULL);
				position = (position / playRate) + iStart;

				if (position > iEnd)
					break;
				else
					stretchMarkers.push_back(position);
			}

			for (size_t i = 0; i < stretchMarkers.size(); ++i)
			{
				item = SplitMediaItem(item, stretchMarkers[i]);
				if (!item) // split at nonexistent position?
					item = items.Get()[i];
				else
					update = true;
			}
		}
	}
	if (update)
	{
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ITEMS, -1);
		UpdateArrange();
	}
}

void MarkersAtTempo (COMMAND_T* ct)
{
	BR_Envelope tempoMap(GetTempoEnv());
	if (!tempoMap.CountSelected() || IsLocked(MARKERS))
		return;

	PreventUIRefresh(1);
	Undo_BeginBlock2(NULL);
	for (int i = 0; i < tempoMap.CountSelected(); ++i)
	{
		int id = tempoMap.GetSelected(i);
		double position; tempoMap.GetPoint(id, &position, NULL, NULL, NULL);
		AddProjectMarker(NULL, false, position, 0, NULL, -1);
	}
	Undo_EndBlock2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_MISCCFG);
	PreventUIRefresh(-1);
}

void MarkersAtNotes (COMMAND_T* ct)
{
	if (IsLocked(MARKERS))
		return;

	PreventUIRefresh(1);

	bool update = false;
	for (int i = 0; i < CountSelectedMediaItems(NULL); ++i)
	{
		MediaItem* item      = GetSelectedMediaItem(NULL, i);
		MediaItem_Take* take = GetActiveTake(item);
		double itemStart = GetMediaItemInfo_Value(item, "D_POSITION");
		double itemEnd   = GetMediaItemInfo_Value(item, "D_POSITION") + GetMediaItemInfo_Value(item, "D_LENGTH");

		// Due to possible tempo changes, always work with PPQ, never time
		double itemStartPPQ = MIDI_GetPPQPosFromProjTime(take, itemStart);
		double itemEndPPQ = MIDI_GetPPQPosFromProjTime(take, itemEnd);
		double sourceLenPPQ = GetMidiSourceLengthPPQ(take);

		int noteCount = 0;
		MIDI_CountEvts(take, &noteCount, NULL, NULL);
		if (noteCount != 0)
		{
			update = true;
			for (int j = 0; j < noteCount; ++j)
			{
				double position;
				MIDI_GetNote(take, j, NULL, NULL, &position, NULL, NULL, NULL, NULL);
				while (position <= itemEndPPQ) // in case source is looped
				{
					if (CheckBounds(position, itemStartPPQ, itemEndPPQ))
						AddProjectMarker(NULL, false, MIDI_GetProjTimeFromPPQPos(take, position), 0, NULL, -1);
					position += sourceLenPPQ;
				}
			}
		}
	}

	if (update)
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_MISCCFG, -1);
	PreventUIRefresh(-1);
}

void MarkersAtStretchMarkers (COMMAND_T* ct)
{
	if (IsLocked(MARKERS))
		return;

	PreventUIRefresh(1);
	bool update = false;
	for (int i = 0; i < CountSelectedMediaItems(NULL); ++i)
	{
		MediaItem_Take* take = GetActiveTake(GetSelectedMediaItem(NULL, i));
		double iStart     = GetMediaItemInfo_Value(GetSelectedMediaItem(NULL, i), "D_POSITION");
		double iEnd       = GetMediaItemInfo_Value(GetSelectedMediaItem(NULL, i), "D_LENGTH") + iStart;
		double playRate      = GetMediaItemTakeInfo_Value(take, "D_PLAYRATE");

		for (int i = 0; i < GetTakeNumStretchMarkers(take); ++i)
		{
			double position;
			GetTakeStretchMarker(take, i, &position, NULL);
			position = (position / playRate) + iStart;
			if (position <= iEnd && AddProjectMarker(NULL, false, position, 0, NULL, -1) != -1)
				update = true;
		}
	}

	if (update)
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_MISCCFG, -1);
	PreventUIRefresh(-1);
}

void MarkerAtMouse (COMMAND_T* ct)
{
	if (IsLocked(MARKERS))
		return;

	double position = PositionAtMouseCursor(true);
	if (position != -1)
	{
		if ((int)ct->user == 1)
			position = SnapToGrid(NULL, position);

		if (AddProjectMarker(NULL, false, position, 0, NULL, -1) != -1)
			Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_MISCCFG, -1);
		UpdateArrange();
	}
}

void MarkersRegionsAtItems (COMMAND_T* ct)
{
	if (!CountSelectedMediaItems(NULL) || ((int)ct->user == 0 && IsLocked(MARKERS)) || ((int)ct->user == 1 && IsLocked(REGIONS)))
		return;

	Undo_BeginBlock2(NULL);
	PreventUIRefresh(1);

	for (int i = 0; i < CountSelectedMediaItems(NULL); ++i)
	{
		MediaItem* item =  GetSelectedMediaItem(NULL, i);
		double iStart = *(double*)GetSetMediaItemInfo(item, "D_POSITION", NULL);
		double iEnd = iStart + *(double*)GetSetMediaItemInfo(item, "D_LENGTH", NULL);
		char* pNotes = (char*)GetSetMediaItemInfo(item, "P_NOTES", NULL);

		string notes(pNotes, strlen(pNotes)+1);
		ReplaceAll(notes, "\r\n", " ");

		if ((int)ct->user == 0)  // Markers
			AddProjectMarker(NULL, false, iStart, 0, notes.c_str(), -1);
		else                     // Regions
			AddProjectMarker(NULL, true, iStart, iEnd, notes.c_str(), -1);
	}

	PreventUIRefresh(-1);
	Undo_EndBlock2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_MISCCFG);
}

void MoveClosestMarker (COMMAND_T* ct)
{
	if (IsLocked(MARKERS))
		return;

	double position;
	if      (abs((int)ct->user) == 1) position = GetPlayPositionEx(NULL);
	else if (abs((int)ct->user) == 2) position = GetCursorPositionEx(NULL);
	else                              position = PositionAtMouseCursor(true);

	if (position >= 0)
	{
		int id = FindClosestProjMarkerIndex(position);
		if (id >= 0)
		{
			if ((int)ct->user < 0) position = SnapToGrid(NULL, position);
			int markerId;
			EnumProjectMarkers3(NULL, id, NULL, NULL, NULL, NULL, &markerId, NULL);

			SetProjectMarkerByIndex(NULL, id, NULL, position, NULL, markerId, NULL, NULL);
			Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_MISCCFG, -1);
		}
	}
}

void MidiItemTempo (COMMAND_T* ct)
{
	if (IsLocked(ITEM_FULL))
		return;

	PreventUIRefresh(1);
	bool ignoreTempo = ((int)ct->user == 2 || (int)ct->user == 3);
	bool update = false;
	for (int i = 0; i < CountSelectedMediaItems(NULL); ++i)
	{
		MediaItem* item = GetSelectedMediaItem(NULL, i);
		if (!IsItemLocked(item))
		{
			double bpm; int num, den;
			TimeMap_GetTimeSigAtTime(NULL, GetMediaItemInfo_Value(item, "D_POSITION"), &num, &den, &bpm);

			if ((int)ct->user == 0 || (int)ct->user == 2)
			{
				if (SetIgnoreTempo(item, ignoreTempo, bpm, num, den, true))
					update = true;
			}
			else
			{
				BR_MidiItemTimePos timePos(item);
				if (SetIgnoreTempo(item, ignoreTempo, bpm, num, den, true))
				{
					timePos.Restore();
					update = true;
				}
			}
		}
	}

	if (update)
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ITEMS, -1);
	PreventUIRefresh(-1);
}

void MidiItemTrim (COMMAND_T* ct)
{
	if (IsLocked(ITEM_FULL) || IsLocked(ITEM_EDGES))
		return;

	bool update = false;
	for (int i = 0; i < CountSelectedMediaItems(0); ++i)
	{
		MediaItem* item = GetSelectedMediaItem(0, i);
		if (DoesItemHaveMidiEvents(item) && !IsItemLocked(item))
		{
			double start = -1;
			double end   = -1;

			for (int j = 0; j < CountTakes(item); ++j)
			{
				MediaItem_Take* take = GetTake(item, j);
				if (MIDI_CountEvts(take, NULL, NULL, NULL))
				{
					double currentStart = EffectiveMidiTakeStart(take, false, false, false);
					double currentEnd   = EffectiveMidiTakeEnd(take, false, false, false);
					if (start == -1 && end == -1)
					{
						start = currentStart;
						end   = currentEnd;
					}
					else
					{
						start = min(currentStart, start);
						end   = max(currentEnd, end);
					}
				}
			}

			update = TrimItem(item, start, end);
		}
	}

	if (update)
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ITEMS, -1);
}

void SnapFollowsGridVis (COMMAND_T* ct)
{
	int option;
	GetConfig("projshowgrid", option);
	SetConfig("projshowgrid", ToggleBit(option, 15));
	RefreshToolbar(0);
}

void PlaybackFollowsTempoChange (COMMAND_T*)
{
	int option; GetConfig("seekmodes", option);

	option = ToggleBit(option, 5);
	SetConfig("seekmodes", option);
	RefreshToolbar(0);

	char tmp[256];
	_snprintfSafe(tmp, sizeof(tmp), "%d", option);
	WritePrivateProfileString("reaper", "seekmodes", tmp, get_ini_file());
}

void TrimNewVolPanEnvs (COMMAND_T* ct)
{
	SetConfig("envtrimadjmode", (int)ct->user);
	RefreshToolbar(0);

	char tmp[256];
	_snprintfSafe(tmp, sizeof(tmp), "%d", (int)ct->user);
	WritePrivateProfileString("reaper", "envtrimadjmode", tmp, get_ini_file());
}

void ToggleDisplayItemLabels (COMMAND_T* ct)
{
	int option; GetConfig("labelitems2", option);
	option = ToggleBit(option, (int)ct->user);
	SetConfig("labelitems2", option);

	char tmp[256];
	_snprintfSafe(tmp, sizeof(tmp), "%d", option);
	WritePrivateProfileString("reaper", "labelitems2", tmp, get_ini_file());

	UpdateArrange();
}

void CycleRecordModes (COMMAND_T*)
{
	int mode; GetConfig("projrecmode", mode);
	if (++mode > 2) mode = 0;

	if      (mode == 0) Main_OnCommandEx(40253, 0, NULL);
	else if (mode == 1) Main_OnCommandEx(40252, 0, NULL);
	else if (mode == 2) Main_OnCommandEx(40076, 0, NULL);
}

void FocusArrangeTracks (COMMAND_T* ct)
{
	if ((int)ct->user == 0)
	{
		TrackEnvelope* envelope = GetSelectedEnvelope(NULL);
		SetCursorContext(envelope ? 2 : 1, envelope);
	}
	else
		SetCursorContext(0, NULL);
}

void MoveActiveWndToMouse (COMMAND_T* ct)
{
	if (HWND hwnd = GetForegroundWindow())
	{
		#ifndef _WIN32
			if (hwnd == GetArrangeWnd() || hwnd == GetRulerWndAlt() || hwnd == GetTcpWnd())
				hwnd = g_hwndParent;

			if (hwnd != g_hwndParent)
			{
				while (GetParent(hwnd) != g_hwndParent && hwnd)
					hwnd = GetParent(hwnd);
				if (!hwnd)
					return;

				bool floating;
				int result = DockIsChildOfDock(hwnd, &floating);
				if (result != -1 && !floating)
					hwnd = g_hwndParent;
			}
		#endif

		if (GetBit((int)ct->user, 4) && !IsFloatingTrackFXWindow(hwnd))
			return;

		RECT r;  GetWindowRect(hwnd, &r);
		POINT p; GetCursorPos(&p);

		int horz = GetBit((int)ct->user, 2) * ((GetBit((int)ct->user, 3)) ? 1 : -1);
		int vert = GetBit((int)ct->user, 0) * ((GetBit((int)ct->user, 1)) ? 1 : -1);
		CenterOnPoint(&r, p, horz, vert, 0, 0);

		RECT screen;
		GetMonitorRectFromPoint(p, &screen);
		BoundToRect(screen, &r);

		SetWindowPos(hwnd, NULL, r.left, r.top, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
	}
}

void ToggleItemOnline (COMMAND_T* ct)
{
	if (IsLocked(ITEM_FULL))
		return;

	for (int i = 0; i < CountSelectedMediaItems(NULL); ++i)
	{
		MediaItem* item = GetSelectedMediaItem(NULL, i);
		if (!IsItemLocked(item))
		{
			for (int j = 0; j < CountTakes(item); ++j)
			{
				if (PCM_source* source = GetMediaItemTake_Source(GetMediaItemTake(item, j)))
					source->SetAvailable(!source->IsAvailable());
			}
		}
	}
	UpdateArrange();
}

void ItemSourcePathToClipBoard (COMMAND_T* ct)
{
	WDL_FastString sourceList;
	for (int i = 0; i < CountSelectedMediaItems(NULL); ++i)
	{
		MediaItem* item = GetSelectedMediaItem(NULL, i);
		if (PCM_source* source = GetMediaItemTake_Source(GetActiveTake(item)))
		{
			// If section, get the "real" source
			if (!strcmp(source->GetType(), "SECTION"))
				source = source->GetSource();

			if (const char* fileName = source->GetFileName())
				if (strcmp(fileName, "")) // skip in-project files
					sourceList.AppendFormatted(SNM_MAX_PATH, "%s\n", fileName);
		}
	}

	if (OpenClipboard(g_hwndParent))
	{
		EmptyClipboard();
		#ifdef _WIN32
			#if !defined(WDL_NO_SUPPORT_UTF8)
			if (WDL_HasUTF8(sourceList.Get()))
			{
				DWORD size;
				WCHAR* wc = WDL_UTF8ToWC(sourceList.Get(), false, 0, &size);
				if (HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, size*sizeof(WCHAR)))
				{
					if (LPVOID cp = GlobalLock(hglbCopy))
						memcpy(cp, wc, size*sizeof(WCHAR));
					GlobalUnlock(hglbCopy);
					SetClipboardData(CF_UNICODETEXT, hglbCopy);
				}
				free(wc);

			}
			else
			#endif
		#endif
		{
			if (HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, sourceList.GetLength() + 1))
			{
				if (LPVOID cp = GlobalLock(hglbCopy))
					memcpy(cp, sourceList.Get(), sourceList.GetLength() + 1);
				GlobalUnlock(hglbCopy);
				SetClipboardData(CF_TEXT, hglbCopy);
			}
		}
		CloseClipboard();
	}
}

void DeleteTakeUnderMouse (COMMAND_T* ct)
{
	BR_MouseInfo mouseInfo(BR_MouseInfo::MODE_ARRANGE | BR_MouseInfo::MODE_IGNORE_ENVELOPE_LANE_SEGMENT);

	// Don't differentiate between things within the item, but ignore any track lane envelopes
	if (!strcmp(mouseInfo.GetWindow(), "arrange") && !strcmp(mouseInfo.GetSegment(), "track") && mouseInfo.GetItem() && (!mouseInfo.GetEnvelope() || mouseInfo.IsTakeEnvelope()))
	{
		if (CountTakes(mouseInfo.GetItem()) > 1 && !IsItemLocked(mouseInfo.GetItem()) && !IsLocked(ITEM_FULL))
		{
			SNM_TakeParserPatcher takePatcher(mouseInfo.GetItem());
			takePatcher.RemoveTake(mouseInfo.GetTakeId());
			if (takePatcher.Commit())
				Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ITEMS, -1);
		}
	}
}

void SelectTrackUnderMouse (COMMAND_T* ct)
{
	BR_MouseInfo mouseInfo(BR_MouseInfo::MODE_MCP_TCP);
	if (!strcmp(mouseInfo.GetWindow(), ((int)ct->user == 0) ? "tcp" : "mcp") && !strcmp(mouseInfo.GetSegment(), "track"))
	{
		if ((int)GetMediaTrackInfo_Value(mouseInfo.GetTrack(), "I_SELECTED") == 0)
		{
			SetMediaTrackInfo_Value(mouseInfo.GetTrack(), "I_SELECTED", 1);
			Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG, -1);
		}
	}
}

void PlaybackAtMouseCursor (COMMAND_T* ct)
{
	if (IsRecording())
		return;

	// Do both MIDI editor and arrange from here to prevent focusing issues (not unexpected in dual monitor situation)
	double mousePos = PositionAtMouseCursor(true);
	if (mousePos == -1)
		mousePos = ME_PositionAtMouseCursor(true, true);

	if (mousePos != -1)
	{
		if ((int)ct->user == 0)
		{
			StartPlayback(mousePos);
		}
		else
		{
			if (!IsPlaying() || IsPaused())
				StartPlayback(mousePos);
			else
			{
				if ((int)ct->user == 1) OnPauseButton();
				if ((int)ct->user == 2) OnStopButton();
			}
		}
	}
}

void SelectItemsByType (COMMAND_T* ct)
{
	if (IsLocked(ITEM_FULL))
		return;

	double tStart, tEnd;
	GetSet_LoopTimeRange(false, false, &tStart, &tEnd, false);

	bool checkTimeSel = ((int)ct->user > 0) ? false : ((tStart == tEnd) ? false : true);
	bool update = false;
	for (int i = 0; i < CountMediaItems(NULL); ++i)
	{
		MediaItem* item = GetMediaItem(NULL, i);
		if (!IsItemLocked(item))
		{
			if (checkTimeSel)
			{
				double itemStart = GetMediaItemInfo_Value(item, "D_POSITION");
				double itemEnd   = itemStart + GetMediaItemInfo_Value(item, "D_LENGTH");
				if (!AreOverlappedEx(itemStart, itemEnd, tStart, tEnd))
					continue;
			}

			if (MediaItem_Take* take = GetActiveTake(item))
			{
				bool select = false;
				int type = GetTakeType(take);

				if      (abs((int)ct->user) == 1) select = (type == 0) ? true : false;
				else if (abs((int)ct->user) == 2) select = (type == 1) ? true : false;
				else if (abs((int)ct->user) == 3) select = (type == 2) ? true : false;
				else if (abs((int)ct->user) == 4) select = (type == 3) ? true : false;
				else if (abs((int)ct->user) == 5) select = (type == 4) ? true : false;
				else if (abs((int)ct->user) == 6) select = (type == 5) ? true : false;
				if (select)
				{
					SetMediaItemInfo_Value(item, "B_UISEL", 1);
					update = true;
				}
			}
			else if (abs((int)ct->user) == 7)
			{
				SetMediaItemInfo_Value(item, "B_UISEL", 1);
				update = true;
			}
		}
	}

	if (update)
	{
		UpdateArrange();
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ITEMS, -1);
	}
}

void SaveCursorPosSlot (COMMAND_T* ct)
{
	int slot = (int)ct->user;

	for (int i = 0; i < g_cursorPos.Get()->GetSize(); ++i)
	{
		if (slot == g_cursorPos.Get()->Get(i)->GetSlot())
			return g_cursorPos.Get()->Get(i)->Save();
	}

	g_cursorPos.Get()->Add(new BR_CursorPos(slot));
}

void RestoreCursorPosSlot (COMMAND_T* ct)
{
	int slot = (int)ct->user;

	for (int i = 0; i < g_cursorPos.Get()->GetSize(); ++i)
	{
		if (slot == g_cursorPos.Get()->Get(i)->GetSlot())
		{
			if (g_cursorPos.Get()->Get(i)->Restore())
			Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_MISCCFG, -1);
			break;
		}
	}
}

void SaveItemMuteStateSlot (COMMAND_T* ct)
{
	int  slot         = abs((int)ct->user) - 1;
	bool selectedOnly = ((int)ct->user > 0);

	for (int i = 0; i < g_itemMuteState.Get()->GetSize(); ++i)
	{
		if (slot == g_itemMuteState.Get()->Get(i)->GetSlot())
			return g_itemMuteState.Get()->Get(i)->Save(selectedOnly);
	}

	g_itemMuteState.Get()->Add(new BR_ItemMuteState(slot, selectedOnly));
}

void RestoreItemMuteStateSlot (COMMAND_T* ct)
{
	int  slot         = abs((int)ct->user) - 1;
	bool selectedOnly = ((int)ct->user > 0);

	for (int i = 0; i < g_itemMuteState.Get()->GetSize(); ++i)
	{
		if (slot == g_itemMuteState.Get()->Get(i)->GetSlot())
		{
			if (g_itemMuteState.Get()->Get(i)->Restore(selectedOnly))
				Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ITEMS, -1);
			break;
		}
	}
}

void SaveTrackSoloMuteStateSlot (COMMAND_T* ct)
{
	int  slot         = abs((int)ct->user) - 1;
	bool selectedOnly = ((int)ct->user > 0);

	for (int i = 0; i < g_trackSoloMuteState.Get()->GetSize(); ++i)
	{
		if (slot == g_trackSoloMuteState.Get()->Get(i)->GetSlot())
			return g_trackSoloMuteState.Get()->Get(i)->Save(selectedOnly);
	}

	g_trackSoloMuteState.Get()->Add(new BR_TrackSoloMuteState(slot, selectedOnly));
}

void RestoreTrackSoloMuteStateSlot (COMMAND_T* ct)
{
	int  slot         = abs((int)ct->user) - 1;
	bool selectedOnly = ((int)ct->user > 0);

	for (int i = 0; i < g_trackSoloMuteState.Get()->GetSize(); ++i)
	{
		if (slot == g_trackSoloMuteState.Get()->Get(i)->GetSlot())
		{
			if (g_trackSoloMuteState.Get()->Get(i)->Restore(selectedOnly))
				Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG, -1);
			break;
		}
	}
}

/******************************************************************************
* Commands: Misc - Media item preview                                         *
******************************************************************************/
void PreviewItemAtMouse (COMMAND_T* ct)
{
	double position;
	MediaItem_Take* takeAtMouse = NULL;
	if (MediaItem* item = ItemAtMouseCursor(&position, &takeAtMouse))
	{
		vector<int> options = GetDigits((int)ct->user);
		int toggle    = options[0];
		int output    = options[1];
		int type      = options[2];
		int pause     = options[3];
		bool playTake = (options[4] == 2);

		if (playTake && !takeAtMouse)
			return;

		MediaTrack* track     = NULL;
		double      volume    = 1;
		double      start     = (type  == 2) ? (position - GetMediaItemInfo_Value(item, "D_POSITION")) : 0;
		double      measure   = (type  == 3) ? 1 : 0;
		bool        pausePlay = (pause == 2) ? true : false;

		if      (output == 2) volume = GetMediaTrackInfo_Value(GetMediaItem_Track(item), "D_VOL");
		else if (output == 3) track = GetMediaItem_Track(item);

		MediaItem_Take* oldTake = NULL;
		if (playTake)
		{
			oldTake = GetActiveTake(item);
			if (oldTake != NULL) SetActiveTake(takeAtMouse);
		}

		ItemPreview(toggle, item, track, volume, start, measure, pausePlay);
		if (oldTake) SetActiveTake(oldTake);
	}
}

/******************************************************************************
* Commands: Misc - Adjust playrate                                            *
******************************************************************************/
WDL_DLGRET AdjustPlayrateOptionsProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, uMsg, wParam, lParam))
		return r;

	static pair<double,pair<double,double> >* rangeValues;
	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			rangeValues = (pair<double,pair<double,double> >*)lParam;

			char tmp[128];
			_snprintfSafe(tmp, sizeof(tmp), "%.6g", rangeValues->second.first);  SetDlgItemText(hwnd, IDC_EDIT1, tmp);
			_snprintfSafe(tmp, sizeof(tmp), "%.6g", rangeValues->second.second); SetDlgItemText(hwnd, IDC_EDIT2, tmp);
			_snprintfSafe(tmp, sizeof(tmp), "%.6g", rangeValues->first);         SetDlgItemText(hwnd, IDC_EDIT3, tmp);

			RestoreWindowPos(hwnd, ADJUST_PLAYRATE_WND, false);
			ShowWindow(hwnd, SW_SHOW);
			SetFocus(hwnd);
		}
		break;

		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case IDOK:
				{
					char tmp [256];
					GetDlgItemText(hwnd, IDC_EDIT1, tmp, 128); rangeValues->second.first  = SetToBounds(AltAtof(tmp), 0.25, 4.0);
					GetDlgItemText(hwnd, IDC_EDIT2, tmp, 128); rangeValues->second.second = SetToBounds(AltAtof(tmp), 0.25, 4.0);
					GetDlgItemText(hwnd, IDC_EDIT3, tmp, 128); rangeValues->first         = SetToBounds(AltAtof(tmp), 0.00001, 4.0);

					_snprintfSafe(tmp, sizeof(tmp), "%lf %lf %lf", rangeValues->first, rangeValues->second.first, rangeValues->second.second);
					WritePrivateProfileString("SWS", ADJUST_PLAYRATE_KEY, tmp, get_ini_file());

					DestroyWindow(hwnd);
				}
				break;

				case IDCANCEL:
				{
					DestroyWindow(hwnd);
				}
				break;
			}
		}
		break;

		case WM_DESTROY:
		{
			SaveWindowPos(hwnd, ADJUST_PLAYRATE_WND);
			g_adjustPlayrateWnd = NULL;
		}
		break;
	}
	return 0;
}

void AdjustPlayrate (COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd)
{
	static pair<double,pair<double,double> > s_rangeValues;
	static bool                              s_initRangeValues = true;

	if (s_initRangeValues)
	{
		char tmp[256];
		GetPrivateProfileString("SWS", ADJUST_PLAYRATE_KEY, "", tmp, sizeof(tmp), get_ini_file());

		LineParser lp(false);
		lp.parse(tmp);
		s_rangeValues.first         = (lp.getnumtokens() > 0) ? SetToBounds(lp.gettoken_float(0), 0.01, 4.0) : 0.01;
		s_rangeValues.second.first  = (lp.getnumtokens() > 1) ? SetToBounds(lp.gettoken_float(1), 0.25, 4.0) : 0.25;
		s_rangeValues.second.second = (lp.getnumtokens() > 2) ? SetToBounds(lp.gettoken_float(2), 0.25,    4.0) : 4;

		s_initRangeValues = false;
	}

	if ((int)ct->user == 0)
	{
		double playrate = Master_GetPlayRateAtTime(GetPlayPosition2Ex(NULL), NULL);
		playrate = GetMidiOscVal(s_rangeValues.second.first, s_rangeValues.second.second, s_rangeValues.first, playrate, val, valhw, relmode);
		CSurf_OnPlayRateChange(playrate);
	}
	else
	{
		if (!g_adjustPlayrateWnd)
			g_adjustPlayrateWnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_BR_ADJUST_PLAYRATE), hwnd, AdjustPlayrateOptionsProc, (LPARAM)&s_rangeValues);
		else
		{
			DestroyWindow(g_adjustPlayrateWnd);
			g_adjustPlayrateWnd = NULL;
		}

		RefreshToolbar(NamedCommandLookup("_BR_ADJUST_PLAYRATE_MIDI"));
	}
}

/******************************************************************************
* Commands: Misc - Title bar display options                                  *
******************************************************************************/
static int            g_titleBarDisplayUpdateCount = 0;
static int            g_titleBarDisplayOptions     = 0;
static bool           g_titleBarDisplayProjConf    = false;
static WNDPROC        g_titleBarOldWndProc         = NULL;
static WDL_FastString g_titleBarDisplayOld;

static void SaveExtensionConfig (ProjectStateContext *ctx, bool isUndo, project_config_extension_t *reg)
{
	if (isUndo) return;
	TitleBarDisplayOptionsInit(true, 2, false, false); // 2 so it gets updated on save and once more when modified
}

static void BeginLoadProjectState (bool isUndo, project_config_extension_t *reg)
{
	if (isUndo) return;
	TitleBarDisplayOptionsInit(true, 1, false, false);
}

static project_config_extension_t s_projectconfig = {NULL, SaveExtensionConfig, BeginLoadProjectState, NULL};

static LRESULT CALLBACK TitleBarDisplayWndCallback (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_SETTEXT)
	{
		static bool s_reentry = false;
		if (g_titleBarDisplayOptions != 0 && !s_reentry)
		{
			static const char* s_separator = __localizeFunc(" - ", "caption", 0);
			const char* versionStr = GetAppVersion();

			// Find position in window text where project name ends
			WDL_FastString searchString;
			searchString.AppendFormatted(512, "%s%s%s", s_separator, "REAPER v", versionStr);
			const char* projNameEnd = strstr_last((const char*)lParam, searchString.Get());
			if (!projNameEnd)
			{
				WDL_FastString searchString;
				searchString.AppendFormatted(512, "%s%s", "REAPER v", versionStr);
				projNameEnd = strstr_last((const char*)lParam, searchString.Get());
			}

			// Project name end position found, modify it
			if (projNameEnd)
			{
				WDL_FastString projectName;  projectName.SetRaw((const char*)lParam, (int)(projNameEnd - (const char*)lParam));
				WDL_FastString majorVersion; majorVersion.SetRaw(versionStr, 1);

				WDL_FastString titleBar;
				if      (projectName.GetLength() == 0)        titleBar.AppendFormatted(4096, "%s %s",     "REAPER", majorVersion.Get());
				else if (GetBit(g_titleBarDisplayOptions, 0)) titleBar.AppendFormatted(4096, "%s%s%s %s", projectName.Get(), s_separator, "REAPER", majorVersion.Get());
				else if (GetBit(g_titleBarDisplayOptions, 1)) titleBar.AppendFormatted(4096, "%s %s%s%s", "REAPER", majorVersion.Get(), s_separator, projectName.Get());
				else                                          titleBar.AppendFormatted(4096, "%s",        projectName.Get());

				s_reentry = true;
				SetWindowText(g_hwndParent, titleBar.Get());
				g_titleBarDisplayOld.SetRaw((const char*)lParam, strlen((const char*)lParam));
				s_reentry = false;

				// Unhook from window procedure not to waste resource unnecessarily (we rehook again on project load, project save or active tab change)
				if (g_titleBarDisplayUpdateCount > 0)
					--g_titleBarDisplayUpdateCount;
				if (g_titleBarDisplayUpdateCount == 0 && g_titleBarOldWndProc && TitleBarDisplayWndCallback == (WNDPROC)GetWindowLongPtr(g_hwndParent, GWLP_WNDPROC)) // restore old proc only if no new wndproc wasn't set in the meantime)
				{
					SetWindowLongPtr(g_hwndParent, GWLP_WNDPROC, (LONG_PTR)g_titleBarOldWndProc);
					g_titleBarOldWndProc = NULL;
					g_titleBarDisplayUpdateCount = 0;
				}
				return 0;
			}
		}
	}
	return g_titleBarOldWndProc(hwnd, uMsg, wParam, lParam);
}

void TitleBarDisplayOptionsInit (bool hookWnd, int updateCount, bool updateNow, bool tabChange)
{
	if (!hookWnd)
	{
		char tmp[64];
		GetPrivateProfileString("common", "titleBarDisplayOptions", "0", tmp, sizeof(tmp), GetIniFileBR());
		g_titleBarDisplayOptions = atoi(tmp);
	}
	else
	{
		if (g_titleBarDisplayOptions != 0 && !g_titleBarOldWndProc)
		{
			if (!g_titleBarDisplayProjConf)
			{
				plugin_register("projectconfig", &s_projectconfig);
				g_titleBarDisplayProjConf = true;
			}
			g_titleBarOldWndProc = (WNDPROC)SetWindowLongPtr(g_hwndParent, GWLP_WNDPROC, (LONG_PTR)TitleBarDisplayWndCallback);
		}

		if (g_titleBarDisplayOptions != 0) g_titleBarDisplayUpdateCount = (updateCount > g_titleBarDisplayUpdateCount) ? updateCount : g_titleBarDisplayUpdateCount;
		else                               g_titleBarDisplayOptions = 0;

		if (g_titleBarDisplayOptions != 0 && updateNow)
		{
			if (tabChange && CountProjectTabs() > 1)
				g_titleBarDisplayOld.DeleteSub(0, g_titleBarDisplayOld.GetLength());

			if (g_titleBarDisplayOld.GetLength() == 0)
			{
				char titleBarStr[4096] = "";
				GetWindowText(g_hwndParent, (LPSTR)titleBarStr, sizeof(titleBarStr));
				g_titleBarDisplayOld.SetRaw(titleBarStr, strlen(titleBarStr));
			}
			SetWindowText(g_hwndParent, (LPCTSTR)g_titleBarDisplayOld.Get());
		}
	}
}

void TitleBarDisplayOptionsExit ()
{
	char tmp[512];
	_snprintfSafe(tmp, sizeof(tmp), "%d", g_titleBarDisplayOptions);
	WritePrivateProfileString("common", "titleBarDisplayOptions", tmp, GetIniFileBR());

	if (g_titleBarOldWndProc)
		SetWindowLongPtr(g_hwndParent, GWLP_WNDPROC, (LONG_PTR)g_titleBarOldWndProc);
	g_titleBarOldWndProc = NULL;
	g_titleBarDisplayUpdateCount = 0;

	plugin_register("-projectconfig", &s_projectconfig);
}

void SetTitleBarDisplayOptions (COMMAND_T* ct)
{
	g_titleBarDisplayOptions = (!GetBit(g_titleBarDisplayOptions, (int)ct->user)) ? SetBit(0, (int)ct->user) : 0;

	if (g_titleBarDisplayOptions == 0)
	{
		if (g_titleBarOldWndProc && TitleBarDisplayWndCallback == (WNDPROC)GetWindowLongPtr(g_hwndParent, GWLP_WNDPROC)) // restore old proc only if no new wndproc wasn't set in the meantime
		{
			SetWindowLongPtr(g_hwndParent, GWLP_WNDPROC, (LONG_PTR)g_titleBarOldWndProc);
			g_titleBarOldWndProc = NULL;
			g_titleBarDisplayUpdateCount = 0;
		}

		plugin_register("-projectconfig", &s_projectconfig);
		g_titleBarDisplayProjConf = false;

		if (g_titleBarDisplayOld.GetLength() != 0)
		{
			SetWindowText(g_hwndParent, (LPCTSTR)g_titleBarDisplayOld.Get());
			g_titleBarDisplayOld.DeleteSub(0, g_titleBarDisplayOld.GetLength());
		}
	}
	else
	{
		TitleBarDisplayOptionsInit(true, 2, true, false); // count should depend if project is modified or not (2 for non-modified, 1 for modified), but since GetProjectStateChangeCount() is broken we have to update it twice just in case
	}
}

/******************************************************************************
* Commands: Misc - Project track selection action                             *
******************************************************************************/
static SWSProjConfig<WDL_FastString> g_trackSelActions;

static bool ProcessExtensionLineTrackSel (const char *line, ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	LineParser lp(false);
	if (lp.parse(line) || lp.getnumtokens() < 1)
		return false;

	if (!strcmp(lp.gettoken_str(0), "BR_PROJ_TRACK_SEL_ACTION"))
	{
		g_trackSelActions.Get()->Set(lp.gettoken_str(1));
		return true;
	}
	return false;
}

static void SaveExtensionConfigTrackSel (ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	if (ctx && g_trackSelActions.Get()->GetLength())
	{
		char line[SNM_MAX_CHUNK_LINE_LENGTH] = "";
		if (_snprintfStrict(line, sizeof(line), "BR_PROJ_TRACK_SEL_ACTION %s", g_trackSelActions.Get()->Get()) > 0)
			ctx->AddLine("%s", line);
	}
}

static void BeginLoadProjectStateTrackSel (bool isUndo, struct project_config_extension_t *reg)
{
	g_trackSelActions.Cleanup();
	g_trackSelActions.Get()->Set("");
}

int ProjectTrackSelInitExit (bool init)
{
	static project_config_extension_t s_projectconfig = {ProcessExtensionLineTrackSel, SaveExtensionConfigTrackSel, BeginLoadProjectStateTrackSel, NULL};

	if (init)
	{
		return plugin_register("projectconfig", &s_projectconfig);
	}
	else
	{
		plugin_register("-projectconfig", &s_projectconfig);
		return 1;
	}
}

void ExecuteTrackSelAction ()
{
	if (g_trackSelActions.Get()->GetLength())
	{
		if (int cmd = SNM_NamedCommandLookup(g_trackSelActions.Get()->Get()))
			Main_OnCommand(cmd, 0);
	}
}

void SetProjectTrackSelAction (COMMAND_T* ct)
{
	/* Note: this is pretty much c/p from Jeffos' code from SnM_Project.cpp (thanks for the code and the basic idea!) */

	bool process = true;
	if (int cmd = SNM_NamedCommandLookup(g_trackSelActions.Get()->Get()))
	{
		WDL_FastString msg;
		msg.AppendFormatted(256, __LOCALIZE_VERFMT("Are you sure you want to replace current project track selection action: '%s'?","sws_mbox"), kbd_getTextFromCmd(cmd, NULL));
		if (MessageBox(GetMainHwnd(), msg.Get(), __LOCALIZE("SWS/BR - Confirmation","sws_mbox"), MB_YESNO) == IDNO)
			process = false;
	}

	if (process)
	{
		// localization note: some messages are shared with the CA editor

		char actionString[SNM_MAX_ACTION_CUSTID_LEN];
		_snprintfSafe(actionString, sizeof(actionString), "%s", __LOCALIZE("Paste command ID or identifier string here","sws_mbox"));

		if (PromptUserForString(GetMainHwnd(), __LOCALIZE("Set project track selection action","sws_mbox"), actionString, sizeof(actionString), true))
		{
			WDL_FastString msg;
			if (int cmd = SNM_NamedCommandLookup(actionString))
			{
				// more checks: http://forum.cockos.com/showpost.php?p=1252206&postcount=1618
				if (int tstNum = CheckSwsMacroScriptNumCustomId(actionString))
				{
					msg.SetFormatted(256, __LOCALIZE_VERFMT("%s failed: unreliable command ID '%s'!","sws_DLG_161"), __LOCALIZE("Set project track selection action","sws_mbox"), actionString);
					msg.Append("\n");

					if (tstNum==-1)
						msg.Append(__LOCALIZE("For SWS actions, you must use identifier strings (e.g. _SWS_ABOUT), not command IDs (e.g. 47145).\nTip: to copy such identifiers, right-click the action in the Actions window > Copy selected action command ID.","sws_mbox"));
					else if (tstNum==-2)
						msg.Append(__LOCALIZE("For macros/scripts, you must use identifier strings (e.g. _f506bc780a0ab34b8fdedb67ed5d3649), not command IDs (e.g. 47145).\nTip: to copy such identifiers, right-click the macro/script in the Actions window > Copy selected action command ID.","sws_mbox"));
					MessageBox(GetMainHwnd(), msg.Get(), __LOCALIZE("SWS/BR - Error","sws_mbox"), MB_OK);
				}
				else
				{
					g_trackSelActions.Get()->Set(actionString);
					Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_MISCCFG, -1);

					msg.SetFormatted(256, __LOCALIZE_VERFMT("'%s' is defined as project track selection action","sws_mbox"), kbd_getTextFromCmd(cmd, NULL));
					char projectPath[SNM_MAX_PATH] = "";
					EnumProjects(-1, projectPath, sizeof(projectPath));
					if (*projectPath)
					{
						msg.Append("\n");
						msg.AppendFormatted(SNM_MAX_PATH, __LOCALIZE_VERFMT("for %s","sws_mbox"), projectPath);
					}
					msg.Append(".");
					MessageBox(GetMainHwnd(), msg.Get(), __LOCALIZE("Set project track selection action","sws_mbox"), MB_OK);
				}
			}
			else
			{
				msg.SetFormatted(256, __LOCALIZE_VERFMT("%s failed: command ID or identifier string '%s' not found in the 'Main' section of the action list!","sws_DLG_161"), __LOCALIZE("Set project track selection action","sws_mbox"), actionString);
				MessageBox(GetMainHwnd(), msg.Get(), __LOCALIZE("SWS/BR - Error","sws_mbox"), MB_OK);
			}
		}
	}
}

void ShowProjectTrackSelAction (COMMAND_T* ct)
{
	WDL_FastString msg(__LOCALIZE("No project track selection action is defined","sws_mbox"));
	if (int cmd = SNM_NamedCommandLookup(g_trackSelActions.Get()->Get()))
		msg.SetFormatted(256, __LOCALIZE_VERFMT("'%s' is defined as project track selection action", "sws_mbox"), kbd_getTextFromCmd(cmd, NULL));

	char projectPath[SNM_MAX_PATH] = "";
	EnumProjects(-1, projectPath, sizeof(projectPath));
	if (*projectPath) {
		msg.Append("\n");
		msg.AppendFormatted(SNM_MAX_PATH, __LOCALIZE_VERFMT("for %s", "sws_mbox"), projectPath);
	}
	msg.Append(".");
	MessageBox(GetMainHwnd(), msg.Get(), __LOCALIZE("Project track selection action","sws_mbox"), MB_OK);
}

void ClearProjectTrackSelAction (COMMAND_T* ct)
{
	if (int cmd = SNM_NamedCommandLookup(g_trackSelActions.Get()->Get()))
	{
		WDL_FastString msg;
		msg.AppendFormatted(256, __LOCALIZE_VERFMT("Are you sure you want to clear current project track selection action: '%s'?","sws_mbox"), kbd_getTextFromCmd(cmd, NULL));
		if (MessageBox(GetMainHwnd(), msg.Get(), __LOCALIZE("SWS/BR - Confirmation","sws_mbox"), MB_YESNO) == IDYES)
		{
			g_trackSelActions.Get()->Set("");
			Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_MISCCFG, -1);
		}
	}
	else
	{
		WDL_FastString msg(__LOCALIZE("No project track selection action is defined.","sws_mbox"));
		MessageBox(GetMainHwnd(), msg.Get(), __LOCALIZE("Project track selection action","sws_mbox"), MB_OK);
		return;
	}
}



/******************************************************************************
* Toggle states: Misc                                                         *
******************************************************************************/
int IsSnapFollowsGridVisOn (COMMAND_T* ct)
{
	int option; GetConfig("projshowgrid", option);
	return !GetBit(option, 15);
}

int IsPlaybackFollowingTempoChange (COMMAND_T* ct)
{
	int option; GetConfig("seekmodes", option);
	return GetBit(option, 5);
}

int IsTrimNewVolPanEnvsOn (COMMAND_T* ct)
{
	int option; GetConfig("envtrimadjmode", option);
	return (option == (int)ct->user);
}

int IsDisplayDisplayItemLabelsOn (COMMAND_T* ct)
{
	int option; GetConfig("labelitems2", option);
	return ((int)ct->user == 4) ? !GetBit(option, (int)ct->user) : GetBit(option, (int)ct->user);
}

int IsAdjustPlayrateOptionsVisible (COMMAND_T* ct)
{
	return !!g_adjustPlayrateWnd;
}

int IsTitleBarDisplayOptionOn (COMMAND_T* ct)
{
	return !!GetBit(g_titleBarDisplayOptions, (int)ct->user);
}