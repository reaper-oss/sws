/*****************************************************************************
/ BR_ReaScript.cpp
/
/ Copyright (c) 2014-2015 Dominik Martin Drzic
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
#include "BR_ReaScript.h"
#include "BR_EnvelopeUtil.h"
#include "BR_MidiUtil.h"
#include "BR_MouseUtil.h"
#include "BR_Util.h"
#include "../SnM/SnM_Chunk.h"
#include "../SnM/SnM_Util.h"

/******************************************************************************
* Globals                                                                     *
******************************************************************************/
static BR_MouseInfo g_mouseInfo(BR_MouseInfo::MODE_ALL, false);

/******************************************************************************
* ReaScript export                                                            *
******************************************************************************/
BR_Envelope* BR_EnvAlloc (TrackEnvelope* envelope, bool takeEnvelopesUseProjectTime)
{
	if (envelope)
		return new (nothrow) BR_Envelope(envelope, takeEnvelopesUseProjectTime);
	else
		return NULL;
}

int BR_EnvCountPoints (BR_Envelope* envelope)
{
	if (envelope)
		return envelope->CountPoints();
	else
		return 0;
}

bool BR_EnvDeletePoint (BR_Envelope* envelope, int id)
{
	if (envelope)
		return envelope->DeletePoint(id);
	else
		return false;
}

int BR_EnvFind (BR_Envelope* envelope, double position, double delta)
{
	if (envelope)
	{
		int id = envelope->Find(position, delta);
		if (envelope->ValidateId(id))
			return id;
		else
			return -1;
	}
	else
		return -1;
}

int BR_EnvFindNext (BR_Envelope* envelope, double position)
{
	if (envelope)
	{
		int id = envelope->FindNext(position);
		if (envelope->ValidateId(id))
			return id;
		else
			return -1;
	}
	else
		return -1;
}

int BR_EnvFindPrevious (BR_Envelope* envelope, double position)
{
	if (envelope)
	{
		int id = envelope->FindPrevious(position);
		if (envelope->ValidateId(id))
			return id;
		else
			return -1;
	}
	else
		return -1;
}

bool BR_EnvFree (BR_Envelope* envelope, bool commit)
{
	if (envelope)
	{
		bool commited = (commit) ? envelope->Commit() : false;
		delete envelope;
		return commited;
	}
	else
		return false;
}

MediaItem_Take* BR_EnvGetParentTake (BR_Envelope* envelope)
{
	if (envelope)
		return envelope->GetTake();
	else
		return NULL;
}

MediaTrack* BR_EnvGetParentTrack (BR_Envelope* envelope)
{
	if (envelope)
	{
		if (envelope->IsTakeEnvelope())
			return NULL;
		else
			return envelope->GetParent();
	}
	return NULL;
}

bool BR_EnvGetPoint (BR_Envelope* envelope, int id, double* positionOut, double* valueOut, int* shapeOut, bool* selectedOut, double* bezierOut)
{
	if (envelope)
	{
		WritePtr(selectedOut, envelope->GetSelection(id));
		return envelope->GetPoint(id, positionOut, valueOut, shapeOut, bezierOut);
	}
	else
		return false;
}

void BR_EnvGetProperties (BR_Envelope* envelope, bool* activeOut, bool* visibleOut, bool* armedOut, bool* inLaneOut, int* laneHeightOut, int* defaultShapeOut, double* minValueOut, double* maxValueOut, double* centerValueOut, int* typeOut, bool* faderScalingOut)
{
	if (envelope)
	{
		WritePtr(activeOut,          envelope->IsActive());
		WritePtr(visibleOut,         envelope->IsVisible());
		WritePtr(armedOut,           envelope->IsArmed());
		WritePtr(inLaneOut,          envelope->IsInLane());
		WritePtr(laneHeightOut,      envelope->GetLaneHeight());
		WritePtr(defaultShapeOut,    envelope->GetDefaultShape());
		WritePtr(minValueOut,        envelope->MinValue());
		WritePtr(maxValueOut,        envelope->MaxValue());
		WritePtr(centerValueOut,     envelope->CenterValue());
		WritePtr(faderScalingOut,    envelope->IsScaledToFader());

		if (typeOut)
		{
			int value = -1;
			if      (value == VOLUME)       value = 0;
			else if (value == VOLUME_PREFX) value = 1;
			else if (value == PAN)          value = 2;
			else if (value == PAN_PREFX)    value = 3;
			else if (value == WIDTH)        value = 4;
			else if (value == WIDTH_PREFX)  value = 5;
			else if (value == MUTE)         value = 6;
			else if (value == PITCH)        value = 7;
			else if (value == PLAYRATE)     value = 8;
			else if (value == TEMPO)        value = 9;
			else if (value == PARAMETER)    value = 10;

			*typeOut = value;
		}
	}
	else
	{
		WritePtr(activeOut,          false);
		WritePtr(visibleOut,         false);
		WritePtr(armedOut,           false);
		WritePtr(inLaneOut,          false);
		WritePtr(laneHeightOut,      0);
		WritePtr(defaultShapeOut,    0);
		WritePtr(minValueOut,        0.0);
		WritePtr(maxValueOut,        0.0);
		WritePtr(centerValueOut,     0.0);
		WritePtr(faderScalingOut,    false);
	}
}

bool BR_EnvSetPoint (BR_Envelope* envelope, int id, double position, double value, int shape, bool selected, double bezier)
{
	if (envelope)
		return envelope->SetCreatePoint(id, position, value, shape, bezier, selected);
	else
		return false;
}

void BR_EnvSetProperties (BR_Envelope* envelope, bool active, bool visible, bool armed, bool inLane, int laneHeight, int defaultShape, bool faderScaling)
{
	if (envelope)
	{
		envelope->SetActive(active);
		envelope->SetVisible(visible);
		envelope->SetArmed(armed);
		envelope->SetInLane(inLane);
		envelope->SetLaneHeight(laneHeight);
		envelope->SetScalingToFader(faderScaling);
		if (defaultShape >= 0 && defaultShape <= 5)
			envelope->SetDefaultShape(defaultShape);
	}
}

void BR_EnvSortPoints (BR_Envelope* envelope)
{
	if (envelope)
		envelope->Sort();
}

double BR_EnvValueAtPos (BR_Envelope* envelope, double position)
{
	if (envelope)
		return envelope->ValueAtPosition(position);
	else
		return 0;
}

double BR_GetMidiSourceLenPPQ (MediaItem_Take* take)
{
	double length = -1;
	if (take)
	{
		bool isMidi = false;
		length = GetMidiSourceLengthPPQ(take, &isMidi);
		if (!isMidi) length = -1;
	}
	return length;
}

MediaItem* BR_GetMediaItemByGUID (ReaProject* proj, const char* guidStringIn)
{
	if (guidStringIn)
	{
		GUID guid;
		stringToGuid(guidStringIn, &guid);
		return GuidToItem(&guid, proj);
	}
	else
		return NULL;
}
void BR_GetMediaItemGUID (MediaItem* item, char* guidStringOut, int guidStringOut_sz)
{
	if (guidStringOut)
	{
		char guid[64];
		if (item) guidToString((GUID*)GetSetMediaItemInfo(item, "GUID", NULL), guid);
		else      guidToString(&GUID_NULL, guid);
		_snprintfSafe(guidStringOut, guidStringOut_sz-1, "%s", guid);
	}
}

void BR_GetMediaItemTakeGUID (MediaItem_Take* take, char* guidStringOut, int guidStringOut_sz)
{
	if (guidStringOut)
	{
		char guid[64];
		if (take) guidToString((GUID*)GetSetMediaItemTakeInfo(take, "GUID", NULL), guid);
		else      guidToString(&GUID_NULL, guid);
		_snprintfSafe(guidStringOut,  guidStringOut_sz-1, "%s", guid);
	}
}

bool BR_GetMediaSourceProperties (MediaItem_Take* take, bool* sectionOut, double* startOut, double* lengthOut, double* fadeOut, bool* reverseOut)
{
	return GetMediaSourceProperties(take, sectionOut, startOut, lengthOut, fadeOut, reverseOut);
}

MediaTrack* BR_GetMediaTrackByGUID (ReaProject* proj, const char* guidStringIn)
{
	if (guidStringIn)
	{
		GUID guid;
		stringToGuid(guidStringIn, &guid);
		for (int i = 0; i < CountTracks(proj); ++i)
		{
			MediaTrack* track = GetTrack(proj, i);
			if (GuidsEqual(&guid, TrackToGuid(track)))
				return track;
		}
	}
	return NULL;
}

void BR_GetMediaTrackGUID (MediaTrack* track, char* guidStringOut, int guidStringOut_sz)
{
	if (guidStringOut)
	{
		char guid[64];
		if (track) guidToString(TrackToGuid(track), guid);
		else       guidToString(&GUID_NULL, guid);
		_snprintfSafe(guidStringOut, guidStringOut_sz-1, "%s", guid);
	}
}

void BR_GetMouseCursorContext (char* windowOut, int windowOut_sz, char* segmentOut, int segmentOut_sz, char* detailsOut, int detailsOut_sz)
{
	g_mouseInfo.Update();

	if (windowOut)  _snprintfSafe(windowOut,  windowOut_sz -1, "%s", g_mouseInfo.GetWindow());
	if (segmentOut) _snprintfSafe(segmentOut, segmentOut_sz-1, "%s", g_mouseInfo.GetSegment());
	if (detailsOut) _snprintfSafe(detailsOut, detailsOut_sz-1, "%s", g_mouseInfo.GetDetails());
}

TrackEnvelope* BR_GetMouseCursorContext_Envelope (bool* takeEnvelopeOut)
{
	WritePtr(takeEnvelopeOut, g_mouseInfo.IsTakeEnvelope());
	return g_mouseInfo.GetEnvelope();
}

MediaItem* BR_GetMouseCursorContext_Item ()
{
	return g_mouseInfo.GetItem();
}

void* BR_GetMouseCursorContext_MIDI (bool* inlineEditorOut, int* noteRowOut, int* ccLaneOut, int* ccLaneValOut, int* ccLaneIdOut)
{
	if (g_mouseInfo.GetNoteRow() == -1)
		WritePtr(noteRowOut, -1);
	else
		WritePtr(noteRowOut, g_mouseInfo.GetNoteRow());

	if (g_mouseInfo.GetCCLane(NULL, NULL, NULL))
	{
		g_mouseInfo.GetCCLane(ccLaneOut, ccLaneValOut, ccLaneIdOut);
		WritePtr(ccLaneOut, MapVelLaneToReaScriptCC(*ccLaneOut));
	}
	else
	{
		WritePtr(ccLaneOut,    -1);
		WritePtr(ccLaneValOut, -1);
		WritePtr(ccLaneIdOut,  -1);
	}

	WritePtr(inlineEditorOut, g_mouseInfo.IsInlineMidi());
	return g_mouseInfo.GetMidiEditor();
}

double BR_GetMouseCursorContext_Position ()
{
	double position = g_mouseInfo.GetPosition();
	if (position == -1)
		return -1;
	else
		return position;
}

int BR_GetMouseCursorContext_StretchMarker ()
{
	int id = g_mouseInfo.GetStretchMarker();
	if (id == -1)
		return -1;
	else
		return id;
}

MediaItem_Take* BR_GetMouseCursorContext_Take ()
{
	return g_mouseInfo.GetTake();
}

MediaTrack* BR_GetMouseCursorContext_Track ()
{
	return g_mouseInfo.GetTrack();
}

MediaItem* BR_ItemAtMouseCursor (double* positionOut)
{
	return ItemAtMouseCursor(positionOut);
}

bool BR_IsTakeMidi (MediaItem_Take* take, bool* inProjectMidiOut)
{
	return IsMidi(take, inProjectMidiOut);
}

bool BR_MIDI_CCLaneReplace (HWND midiEditor, int laneId, int newCC)
{
	MediaItem_Take* take = MIDIEditor_GetTake(midiEditor);
	int newLane = MapReaScriptCCToVelLane(newCC);

	if (take && IsVelLaneValid(newLane))
	{
		MediaItem* item = GetMediaItemTake_Item(take);
		int takeId = GetTakeId(take, item);
		if (takeId >= 0)
		{
			SNM_TakeParserPatcher p(item, CountTakes(item));
			WDL_FastString takeChunk;
			int tkPos, tklen;
			if (p.GetTakeChunk(takeId, &takeChunk, &tkPos, &tklen))
			{
				SNM_ChunkParserPatcher ptk(&takeChunk, false);

				if (ptk.Parse(SNM_GET_SUBCHUNK_OR_LINE, 1, "SOURCE", "VELLANE", laneId, -1))
				{
					WDL_FastString newLaneStr;
					newLaneStr.AppendFormatted(256, "%d", newLane);
					ptk.ParsePatch(SNM_SET_CHUNK_CHAR, 1, "SOURCE", "VELLANE", laneId, 1, (void*)newLaneStr.Get());
					return p.ReplaceTake(tkPos, tklen, ptk.GetChunk());
				}
			}
		}
	}
	return false;
}

bool BR_MIDI_CCLaneRemove (HWND midiEditor, int laneId)
{
	MediaItem_Take* take = MIDIEditor_GetTake(midiEditor);

	if (take)
	{
		MediaItem* item = GetMediaItemTake_Item(take);
		int takeId = GetTakeId(take, item);
		if (takeId >= 0)
		{
			SNM_TakeParserPatcher p(item, CountTakes(item));
			WDL_FastString takeChunk;
			int tkPos, tklen;
			if (p.GetTakeChunk(takeId, &takeChunk, &tkPos, &tklen))
			{
				SNM_ChunkParserPatcher ptk(&takeChunk, false);

				if (ptk.Parse(SNM_GET_SUBCHUNK_OR_LINE, 1, "SOURCE", "VELLANE", laneId, -1))
				{
					ptk.RemoveLine("SOURCE", "VELLANE", 1, laneId);
					return p.ReplaceTake(tkPos, tklen, ptk.GetChunk());
				}
			}
		}
	}
	return false;
}

double BR_PositionAtMouseCursor (bool checkRuler)
{
	return PositionAtMouseCursor(checkRuler, true);
}

bool BR_SetMediaSourceProperties (MediaItem_Take* take, bool section, double start, double length, double fade, bool reverse)
{
	return SetMediaSourceProperties(take, section, start, length, fade, reverse);
}

bool BR_SetTakeSourceFromFile (MediaItem_Take* take, const char* filenameIn, bool inProjectData)
{
	return SetTakeSourceFromFile(take, filenameIn, inProjectData, false);
}

bool BR_SetTakeSourceFromFile2 (MediaItem_Take* take, const char* filenameIn, bool inProjectData, bool keepSourceProperties)
{
	return SetTakeSourceFromFile(take, filenameIn, inProjectData, keepSourceProperties);
}

MediaItem_Take* BR_TakeAtMouseCursor (double* positionOut)
{
	return TakeAtMouseCursor(positionOut);
}

MediaTrack* BR_TrackAtMouseCursor (int* contextOut, double* positionOut)
{
	return TrackAtMouseCursor(contextOut, positionOut);
}