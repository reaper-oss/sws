/******************************************************************************
/ BR_Misc.cpp
/
/ Copyright (c) 2013-2014 Dominik Martin Drzic
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
#include "BR_Misc.h"
#include "BR_EnvTools.h"
#include "BR_MidiTools.h"
#include "BR_ProjState.h"
#include "BR_Util.h"
#include "../Xenakios/XenakiosExts.h"
#include "../reaper/localize.h"

/******************************************************************************
* Commands                                                                    *
******************************************************************************/
void SplitItemAtTempo (COMMAND_T* ct)
{
	WDL_TypedBuf<MediaItem*> items;
	SWS_GetSelectedMediaItems(&items);
	if (!items.GetSize() || !CountTempoTimeSigMarkers(NULL))
		return;

	Undo_BeginBlock2(NULL);
	for (int i = 0; i < items.GetSize(); ++i)
	{
		MediaItem* item = items.Get()[i];
		double iStart = GetMediaItemInfo_Value(item, "D_POSITION");
		double iEnd = iStart + GetMediaItemInfo_Value(item, "D_LENGTH");
		double tPos = iStart - 1;

		// Split item currently in the loop
		while (true)
		{
			item = SplitMediaItem(item, tPos);
			if (!item) // split at nonexistent position?
				item = items.Get()[i];

			tPos = TimeMap2_GetNextChangeTime(NULL, tPos);
			if (tPos > iEnd || tPos == -1 )
				break;
		}
	}
	Undo_EndBlock2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);
	UpdateArrange();
}

void MarkersAtTempo (COMMAND_T* ct)
{
	BR_Envelope tempoMap(GetTempoEnv());
	if (!tempoMap.CountSelected())
		return;

	PreventUIRefresh(1);
	Undo_BeginBlock2(NULL);
	for (int i = 0; i < tempoMap.CountSelected(); ++i)
	{
		int id = tempoMap.GetSelected(i);
		double position; tempoMap.GetPoint(id, &position, NULL, NULL, NULL);
		AddProjectMarker(NULL, false, position, 0, NULL, -1);
	}
	Undo_EndBlock2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);
	PreventUIRefresh(-1);
	UpdateTimeline();
}

void MidiItemTempo (COMMAND_T* ct)
{
	bool stateChanged = false;
	for (int i = 0; i < CountSelectedMediaItems(NULL); ++i)
	{
		MediaItem* item = GetSelectedMediaItem(NULL, i);

		bool midiFound = false;
		for (int i = 0; i < CountTakes(item); ++i)
		{
			if (IsMidi(GetMediaItemTake(item, i)))
			{
				midiFound = true;
				break;
			}
		}
		if (!midiFound)
			continue;

		WDL_FastString newState;
		char* chunk = GetSetObjectState(item, "");
		double position = *(double*)GetSetMediaItemInfo(item, "D_POSITION", NULL);

		char* token = strtok(chunk, "\n");
		while (token != NULL)
		{
			if (!strncmp(token, "IGNTEMPO ", 9))
			{
				int value, num, den; double bpm;
				if ((int)ct->user == 0)
				{
					value = 1;
					TimeMap_GetTimeSigAtTime(NULL, position, &num, &den, &bpm);
				}
				else
				{
					value = 0;
					sscanf(token, "IGNTEMPO %*d %lf %d %d\n", &bpm, &num, &den);
				}

				newState.AppendFormatted(256, "IGNTEMPO %d %.14lf %d %d\n", value, bpm, num, den);
			}
			else
			{
				AppendLine(newState, token);
			}
			token = strtok(NULL, "\n");
		}
		GetSetObjectState(item, newState.Get());
		FreeHeapPtr(chunk);
		stateChanged = true;
	}
	if (stateChanged)
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
}

void MarkersAtNotes (COMMAND_T* ct)
{
	PreventUIRefresh(1);
	bool success = false;

	for (int i = 0; i < CountSelectedMediaItems(NULL); ++i)
	{
		MediaItem* item = GetSelectedMediaItem(NULL, i);
		MediaItem_Take* take = GetActiveTake(item);
		double itemStart =  GetMediaItemInfo_Value(item, "D_POSITION");
		double itemEnd = GetMediaItemInfo_Value(item, "D_POSITION") + GetMediaItemInfo_Value(item, "D_LENGTH");
		double sourceLen = GetMediaItemTake_Source(take)->GetLength();

		// Due to possible tempo changes, always work with PPQ, never time
		double itemPPQEnd = MIDI_GetPPQPosFromProjTime(take, itemEnd);
		double sourcePPQLen = MIDI_GetPPQPosFromProjTime(take, itemStart + sourceLen) - MIDI_GetPPQPosFromProjTime(take, itemStart);

		int noteCount = 0;
		MIDI_CountEvts(take, &noteCount, NULL, NULL);
		if (noteCount != 0)
		{
			success = true;
			for (int j = 0; j < noteCount; ++j)
			{
				double position;
				MIDI_GetNote(take, j, NULL, NULL, &position, NULL, NULL, NULL, NULL);
				while (position <= itemPPQEnd) // in case source is looped
				{
					AddProjectMarker(NULL, false, MIDI_GetProjTimeFromPPQPos(take, position), 0, NULL, -1);
					position += sourcePPQLen;
				}
			}
		}
	}

	if (success)
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
	PreventUIRefresh(-1);
	UpdateArrange();
}

void MarkersRegionsAtItems (COMMAND_T* ct)
{
	if (!CountSelectedMediaItems(NULL))
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
	UpdateTimeline();
	Undo_EndBlock2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);
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
	int option;
	GetConfig("seekmodes", option);
	SetConfig("seekmodes", ToggleBit(option, 5));
	RefreshToolbar(0);
}

void TrimNewVolPanEnvs (COMMAND_T* ct)
{
	SetConfig("envtrimadjmode", (int)ct->user);
	RefreshToolbar(0);
}

void CycleRecordModes (COMMAND_T*)
{
	int mode; GetConfig("projrecmode", mode);
	if (++mode > 2) mode = 0;

	if      (mode == 0) Main_OnCommandEx(40253, 0, NULL);
	else if (mode == 1) Main_OnCommandEx(40252, 0, NULL);
	else if (mode == 2) Main_OnCommandEx(40076, 0, NULL);
}

void FocusArrange (COMMAND_T* ct)
{
	SetFocus(GetArrangeWnd());
}

void ToggleItemOnline (COMMAND_T* ct)
{
	for (int i = 0; i < CountSelectedMediaItems(NULL); ++i)
	{
		MediaItem* item = GetSelectedMediaItem(NULL, i);
		for (int j = 0; j < CountTakes(item); ++j)
		{
			if (PCM_source* source = GetMediaItemTake_Source(GetMediaItemTake(item, j)))
				source->SetAvailable(!source->IsAvailable());
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
					sourceList.AppendFormatted(4096, "%s\n", fileName);
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

				HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, size*sizeof(WCHAR));
				memcpy(GlobalLock(hglbCopy), wc, size*sizeof(WCHAR));
				free(wc);
				GlobalUnlock(hglbCopy);
				SetClipboardData(CF_UNICODETEXT, hglbCopy);
			}
			else
			#endif
		#endif
		{
			HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, sourceList.GetLength() + 1);
			memcpy(GlobalLock(hglbCopy), sourceList.Get(), sourceList.GetLength() + 1);
			GlobalUnlock(hglbCopy);
			SetClipboardData(CF_TEXT, hglbCopy);
		}
		CloseClipboard();
	}
}

void PreviewItemAtMouse (COMMAND_T* ct)
{
	double position;
	if (MediaItem* item = ItemAtMouseCursor(&position))
	{
		vector<int> options = GetDigits((int)ct->user);
		int toggle = options[0];
		int output = options[1];
		int type   = options[2];
		int pause  = options[3];

		MediaTrack* track     = NULL;
		double      volume    = 1;
		double      start     = (type  == 2) ? (position - GetMediaItemInfo_Value(item, "D_POSITION")) : 0;
		double      measure   = (type  == 3) ? 1 : 0;
		bool        pausePlay = (pause == 2) ? true : false;

		if (output == 2)
			volume = GetMediaTrackInfo_Value(GetMediaItem_Track(item), "D_VOL");
		else if (output == 3)
			track = GetMediaItem_Track(item);

		ItemPreview(toggle, item, track, volume, start, measure, pausePlay);
	}
}

void PlaybackAtMouseCursor (COMMAND_T* ct)
{
	if (IsRecording())
		return;

	// Do both MIDI and arrange from here to prevent focusing issues (not unexpected in dual monitor situation)
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

void SaveCursorPosSlot (COMMAND_T* ct)
{
	int slot = (int)ct->user;

	for (int i = 0; i < g_cursorPos.Get()->GetSize(); i++)
	{
		if (slot == g_cursorPos.Get()->Get(i)->GetSlot())
			return g_cursorPos.Get()->Get(i)->Save();
	}

	g_cursorPos.Get()->Add(new BR_CursorPos(slot));
}

void RestoreCursorPosSlot (COMMAND_T* ct)
{
	int slot = (int)ct->user;

	for (int i = 0; i < g_cursorPos.Get()->GetSize(); i++)
	{
		if (slot == g_cursorPos.Get()->Get(i)->GetSlot())
		{
			g_cursorPos.Get()->Get(i)->Restore();
			Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
		}
	}
}

/******************************************************************************
* Toggle states                                                               *
******************************************************************************/
int IsSnapFollowsGridVisOn (COMMAND_T* = NULL)
{
	int option; GetConfig("projshowgrid", option);
	return !GetBit(option, 15);
}

int IsPlaybackFollowingTempoChange (COMMAND_T* = NULL)
{
	int option; GetConfig("seekmodes", option);
	return GetBit(option, 5);
}

int IsTrimNewVolPanEnvsOn (COMMAND_T* ct)
{
	int option; GetConfig("envtrimadjmode", option);
	return (option == (int)ct->user);
}