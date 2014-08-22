/******************************************************************************
/ BR_MidiEditor.cpp
/
/ Copyright (c) 2014 Dominik Martin Drzic
/ http://forum.cockos.com/member.php?u=27094
/ https://code.google.com/p/sws-extension
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
#include "BR_MidiEditor.h"
#include "BR_MidiTools.h"
#include "BR_Misc.h"
#include "BR_ProjState.h"
#include "BR_Util.h"
#include "../Fingers/RprMidiCCLane.h"
#include "../SnM/SnM_Track.h"
#include "../reaper/localize.h"

/******************************************************************************
* Globals                                                                     *
******************************************************************************/
static preview_register_t g_ItemPreview = {{}, 0};
static bool g_itemPreviewPlaying = false;
static bool g_itemPreviewPaused = false;

/******************************************************************************
* MIDI take preview - separate from arrange preview so they don't exclude     *
* each other out                                                              *
******************************************************************************/
static void MidiTakePreviewTimer ();
static void MidiTakePreview (int mode, MediaItem_Take* take, MediaTrack* track, double volume, double startOffset, double measureSync, bool pauseDuringPrev)
{
	/* mode: 0 -> stop   *
	*        1 -> start  *
	*        2 -> toggle */

	if (IsRecording()) // Reaper won't preview anything during recording but extension will still think preview is in progress (could disrupt toggle states and send unneeded CC123)
		return;

	if (g_itemPreviewPlaying)
	{
		if (g_ItemPreview.preview_track)
		{
			StopTrackPreview(&g_ItemPreview);
			SendAllNotesOff((MediaTrack*)g_ItemPreview.preview_track);
		}
		else
		{
			StopPreview(&g_ItemPreview);
		}

		g_itemPreviewPlaying = false;
		delete g_ItemPreview.src;

		if (g_itemPreviewPaused && mode != 1) // requesting new preview while old one is still playing shouldn't unpause playback
		{
			if (IsPaused())
				OnPauseButton();
			g_itemPreviewPaused = false;
		}
	}

	if (mode == 0 || mode == 2)
		return;

	if (take)
	{
		MediaItem* item         = GetMediaItemTake_Item(take);
		MediaItem_Take* oldTake = GetActiveTake(item);
		bool itemMuteState      = *(bool*)GetSetMediaItemInfo(item, "B_MUTE", NULL);
		double effectiveTakeLen = EffectiveMidiTakeLength(take, true, true);

		GetSetMediaItemInfo(item, "B_MUTE", &g_bFalse);     // needs to be set before getting the source
		SetActiveTake(take);                                // active item take and editor take may differ
		PCM_source* src = ((PCM_source*)item)->Duplicate(); // must be item source otherwise item/take volume won't get accounted for

		if (src && effectiveTakeLen > 0 && effectiveTakeLen > startOffset)
		{
			GetSetMediaItemInfo((MediaItem*)src, "D_POSITION", &g_d0);
			GetSetMediaItemInfo((MediaItem*)src, "D_LENGTH", &effectiveTakeLen);

			if (!g_ItemPreview.src)
			{
				#ifdef _WIN32
					InitializeCriticalSection(&g_ItemPreview.cs);
				#else
					pthread_mutex_init(&g_ItemPreview.mutex, NULL);
				#endif
				g_ItemPreview.loop = false;
			}

			g_ItemPreview.src           = src;
			g_ItemPreview.m_out_chan    = (track) ? (-1) : (0);
			g_ItemPreview.curpos        = startOffset;
			g_ItemPreview.volume        = volume;
			g_ItemPreview.preview_track = track;


			// Pause before preview otherwise MidiTakePreviewPlayState will stop it
			g_itemPreviewPaused = pauseDuringPrev;
			if (g_itemPreviewPaused && IsPlaying() && !IsPaused())
				OnPauseButton();

			if (g_ItemPreview.preview_track)
				g_itemPreviewPlaying = !!PlayTrackPreview2Ex(NULL, &g_ItemPreview, (measureSync) ? (1) : (0), measureSync);
			else
				g_itemPreviewPlaying = !!PlayPreviewEx(&g_ItemPreview, (measureSync) ? (1) : (0), measureSync);

			if (g_itemPreviewPlaying)
				plugin_register("timer",(void*)MidiTakePreviewTimer);
			else
				delete g_ItemPreview.src;
		}

		SetActiveTake(oldTake);
		GetSetMediaItemInfo(item, "B_MUTE", &itemMuteState);
	}
}

static void MidiTakePreviewTimer ()
{
	if (g_itemPreviewPlaying)
	{
		#ifdef _WIN32
			EnterCriticalSection(&g_ItemPreview.cs);
		#else
			pthread_mutex_lock(&g_ItemPreview.mutex);
		#endif

		// Have we reached the end?
		if (g_ItemPreview.curpos >= g_ItemPreview.src->GetLength())
		{
			#ifdef _WIN32
				LeaveCriticalSection(&g_ItemPreview.cs);
			#else
				pthread_mutex_unlock(&g_ItemPreview.mutex);
			#endif
			MidiTakePreview(0, NULL, NULL, 0, 0, 0, false);
			plugin_register("-timer",(void*)MidiTakePreviewTimer);
		}
		else
		{
			#ifdef _WIN32
				LeaveCriticalSection(&g_ItemPreview.cs);
			#else
				pthread_mutex_unlock(&g_ItemPreview.mutex);
			#endif
		}
	}
	else
	{
		plugin_register("-timer",(void*)MidiTakePreviewTimer);
	}
}

void MidiTakePreviewPlayState (bool play, bool rec)
{
	if (g_itemPreviewPlaying && (!play || rec || (g_itemPreviewPaused && play)))
		MidiTakePreview(0, NULL, NULL, 0, 0, 0, false);
}

/******************************************************************************
* Commands: MIDI editor - Item preview                                        *
******************************************************************************/
void ME_StopMidiTakePreview (COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd)
{
	MidiTakePreview(0, NULL, NULL, 0, 0, 0, false);
}

void ME_PreviewActiveTake (COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd)
{
	if (MediaItem_Take* take = MIDIEditor_GetTake(MIDIEditor_GetActive()))
	{
		MediaItem* item = GetMediaItemTake_Item(take);

		vector<int> options = GetDigits((int)ct->user);
		int toggle   = options[0];
		int type     = options[1];
		int selNotes = options[2];
		int pause    = options[3];

		MediaTrack* track     = GetMediaItem_Track(item);
		double      volume    = GetMediaItemInfo_Value(item, "D_VOL");
		double      start     = 0;
		double      measure   = (type  == 3) ? 1 : 0;
		bool        pausePlay = (pause == 2) ? true : false;

		if (type == 2)
		{
			double mousePosition = ME_PositionAtMouseCursor(true, true);
			if (mousePosition != -1)
				start = mousePosition - GetMediaItemInfo_Value(item, "D_POSITION");
			else
				return;
		}

		vector<int> muteState;
		if (selNotes == 2 && !AreAllNotesUnselected(take))
			muteState = MuteSelectedNotes(take);

		MidiTakePreview(toggle, take, track, volume, start, measure, pausePlay);

		if (muteState.size() > 0)
			SetMutedNotes(take, muteState);
	}
}

/******************************************************************************
* Commands: MIDI editor - Misc                                                *
******************************************************************************/
void ME_PlaybackAtMouseCursor (COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd)
{
	PlaybackAtMouseCursor(ct);
}

void ME_CCEventAtEditCursor (COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd)
{
	BR_MouseContextInfo mouseContext;
	GetMouseCursorContext(NULL, NULL, NULL, &mouseContext);
	if (mouseContext.midiEditor)
	{
		if (MediaItem_Take* take = MIDIEditor_GetTake(mouseContext.midiEditor))
		{
			double positionPPQ = MIDI_GetPPQPosFromProjTime(take, GetCursorPositionEx(NULL));
			int value          = mouseContext.ccLaneVal;
			int lane           = mouseContext.ccLane;

			if (value != -1 && lane != -2)
			{
				if (lane == CC_TEXT_EVENTS || lane == CC_SYSEX || lane == CC_BANK_SELECT || lane == CC_VELOCITY)
					MessageBox((HWND)mouseContext.midiEditor, __LOCALIZE("Can't insert in velocity, text, sysex and bank select lanes","sws_mbox"), __LOCALIZE("SWS/BR - Warning","sws_mbox"), MB_OK);
				else
				{
					bool do14bit    = (lane >= CC_14BIT_START) ? true : false;
					int type        = (lane == CC_PROGRAM) ? (0xC0) : (lane == CC_CHANNEL_PRESSURE ? 0xD0 : (lane == CC_PITCH ? 0xE0 : 0xB0));
					int channel     = MIDIEditor_GetSetting_int(mouseContext.midiEditor, "default_note_chan");
					int msg2        = CheckBounds(lane, 0, 127) ? ((value >> 7) | 0) : (value & 0x7F);
					int msg3        = CheckBounds(lane, 0, 127) ? (value & 0x7F)     : ((value >> 7) | 0);

					int targetLane  = (do14bit) ? lane - CC_14BIT_START : lane;
					int targetLane2 = (do14bit) ? targetLane + 32       : lane;

					MIDI_InsertCC(take, true, false, positionPPQ, type,	channel, (CheckBounds(targetLane, 0, 127) ? targetLane : msg2), msg3);
					if (do14bit)
						MIDI_InsertCC(take, true, false, positionPPQ, type, channel, targetLane2, msg2);

					Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
				}
			}
		}
	}
}

void ME_ShowUsedCCLanesDetect14Bit (COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd)
{
	if (MediaItem_Take* take = MIDIEditor_GetTake(MIDIEditor_GetActive()))
	{
		RprTake rprTake(take);
		if (RprMidiCCLane* laneView = new (nothrow) RprMidiCCLane(rprTake))
		{
			int defaultHeight = 67; // same height FNG versions use (to keep behavior identical)
			set<int> usedCC = GetUsedCCLanes(MIDIEditor_GetActive(), 2);

			for (int i = 0; i < laneView->countShown(); ++i)
			{
				if (usedCC.find(laneView->getIdAt(i)) == usedCC.end())
					laneView->remove(i--);
			}

			// Special case: Bank select and CC0 (from FNG version to keep behavior identical)
			if (usedCC.find(0) != usedCC.end() && usedCC.find(CC_BANK_SELECT) != usedCC.end() && !laneView->isShown(131))
				laneView->append(131, defaultHeight);

			for (set<int>::iterator it = usedCC.begin(); it != usedCC.end(); ++it)
			{
				if (!laneView->isShown(*it))
					laneView->append(*it, defaultHeight);
				else
				{
					for (int i = 0; i < laneView->countShown(); ++i)
					{
						if (laneView->getIdAt(i) == *it && laneView->getHeight(i) == 0)
							laneView->setHeightAt(i, defaultHeight);
					}
				}
			}

			if (laneView->countShown() == 0)
				laneView->append(-1, 0);

			delete laneView;
			Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
		}
	}
}

void ME_SaveCursorPosSlot (COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd)
{
	SaveCursorPosSlot(ct);
}

void ME_RestoreCursorPosSlot (COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd)
{
	RestoreCursorPosSlot(ct);
}

void ME_SaveNoteSelSlot (COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd)
{
	if (MediaItem_Take* take = MIDIEditor_GetTake(MIDIEditor_GetActive()))
	{
		int slot = (int)ct->user;

		for (int i = 0; i < g_midiNoteSel.Get()->GetSize(); ++i)
		{
			if (slot == g_midiNoteSel.Get()->Get(i)->GetSlot())
				return g_midiNoteSel.Get()->Get(i)->Save(take);
		}
		g_midiNoteSel.Get()->Add(new BR_MidiNoteSel(slot, take));
	}
}

void ME_RestoreNoteSelSlot (COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd)
{
	if (MediaItem_Take* take = MIDIEditor_GetTake(MIDIEditor_GetActive()))
	{
		int slot = (int)ct->user;

		for (int i = 0; i < g_midiNoteSel.Get()->GetSize(); ++i)
		{
			if (slot == g_midiNoteSel.Get()->Get(i)->GetSlot())
			{
				g_midiNoteSel.Get()->Get(i)->Restore(take);
				Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
				return;
			}
		}
	}
}

void ME_SaveCCEventsSlot (COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd)
{
	BR_MidiEditor midiEditor;
	if (midiEditor.IsValid())
	{
		int lane = midiEditor.GetLastClickedCCLane();
		if (lane != CC_TEXT_EVENTS && lane != CC_SYSEX && lane != CC_BANK_SELECT)
		{
			int slot = (int)ct->user;
			for (int i = 0; i < g_midiCCEvents.Get()->GetSize(); ++i)
			{
				if (slot == g_midiCCEvents.Get()->Get(i)->GetSlot())
					return g_midiCCEvents.Get()->Get(i)->Save(midiEditor);
			}
			g_midiCCEvents.Get()->Add(new BR_MidiCCEvents(slot, midiEditor));
		}
		else
			MessageBox((HWND)midiEditor.GetEditor(), __LOCALIZE("Can't save events from text, sysex and bank select lanes","sws_mbox"), __LOCALIZE("SWS/BR - Warning","sws_mbox"), MB_OK);
	}
}

void ME_RestoreCCEventsSlot (COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd)
{
	BR_MidiEditor midiEditor;
	if (midiEditor.IsValid())
	{
		int slot = abs((int)ct->user) - 1;
		bool allVisible = ((int)ct->user < 0) ? true : false;

		for (int i = 0; i < g_midiCCEvents.Get()->GetSize(); ++i)
		{
			if (slot == g_midiCCEvents.Get()->Get(i)->GetSlot())
			{
				bool process = true;
				int lane = midiEditor.GetLastClickedCCLane();
				if (!allVisible && (lane == CC_TEXT_EVENTS || lane == CC_SYSEX || lane == CC_BANK_SELECT || lane == CC_VELOCITY))
					process = false;

				if (process)
				{
					if (g_midiCCEvents.Get()->Get(i)->Restore(midiEditor, allVisible))
						Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
				}
				else
					MessageBox((HWND)midiEditor.GetEditor(), __LOCALIZE("Can't restore to velocity, text, sysex and bank select lanes","sws_mbox"), __LOCALIZE("SWS/BR - Warning","sws_mbox"), MB_OK);
				return;
			}
		}
	}
}