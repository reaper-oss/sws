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
			if (GetPlayStateEx(NULL)&2)
				OnPauseButton();
			g_itemPreviewPaused = false;
		}

		if (mode == 2)
			return;
	}
	if (mode == 0)
		return;

	if (take)
	{
		MediaItem* item = GetMediaItemTake_Item(take);

		bool itemMuteState      = *(bool*)GetSetMediaItemInfo(item, "B_MUTE", NULL);
		MediaItem_Take* oldTake = GetActiveTake(item);
		GetSetMediaItemInfo(item, "B_MUTE", &g_bFalse); // needs to be set before getting the source
		SetActiveTake(take);                            // active item take and editor take may differ

		// We need to take item source otherwise item/take volume won't get accounted for
		if (PCM_source* src = ((PCM_source*)item)->Duplicate())
		{
			GetSetMediaItemInfo((MediaItem*)src, "D_POSITION", &g_d0);

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
			if (g_itemPreviewPaused && (GetPlayStateEx(NULL)&1) == 1 && (GetPlayStateEx(NULL)&2) != 1)
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
* Commands                                                                    *
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
			muteState = MuteSelectedSaveOldState(take);

		MidiTakePreview(toggle, take, track, volume, start, measure, pausePlay);

		if (muteState.size() > 0)
			SetMutedState(take, muteState);
	}
}

void ME_PlaybackAtMouseCursor (COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd)
{
	PlaybackAtMouseCursor(ct);
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

		for (int i = 0; i < g_midiNoteSel.Get()->GetSize(); i++)
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

		for (int i = 0; i < g_midiNoteSel.Get()->GetSize(); i++)
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