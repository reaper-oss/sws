/*****************************************************************************
/ BR_ReaScript.cpp
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
#include "BR_ReaScript.h"
#include "BR_EnvTools.h"
#include "BR_MidiTools.h"
#include "BR_Util.h"
#include "../SnM/SnM_Chunk.h"
#include "../SnM/SnM_Item.h"
#include "../SnM/SnM_Util.h"

/******************************************************************************
* Globals                                                                     *
******************************************************************************/
static BR_MouseContextInfo g_mouseInfo;

/******************************************************************************
* ReaScript export                                                            *
******************************************************************************/
void* BR_EnvAlloc (TrackEnvelope* envelope)
{
	if (envelope)
		return (void*)(new (nothrow) BR_Envelope(envelope));
	else
		return NULL;
}

int BR_EnvCountPoints (void* envelope)
{
	if (envelope)
		return ((BR_Envelope*)envelope)->Count();
	else
		return 0;
}

bool BR_EnvDeletePoint (void* envelope, int id)
{
	if (envelope)
		return ((BR_Envelope*)envelope)->DeletePoint(id);
	else
		return false;
}

int BR_EnvFind (void* envelope, double position, double delta)
{
	if (envelope)
	{
		BR_Envelope* env = (BR_Envelope*)envelope;
		int id = env->Find(position, delta);

		if (env->ValidateId(id))
			return id;
		else
			return -1;
	}
	else
		return -1;
}

int BR_EnvFindNext (void* envelope, double position)
{
	if (envelope)
	{
		BR_Envelope* env = (BR_Envelope*)envelope;
		int id = env->FindNext(position);

		if (env->ValidateId(id))
			return id;
		else
			return -1;
	}
	else
		return -1;
}

int BR_EnvFindPrevious (void* envelope, double position)
{
	if (envelope)
	{
		BR_Envelope* env = (BR_Envelope*)envelope;
		int id = env->FindPrevious(position);

		if (env->ValidateId(id))
			return id;
		else
			return -1;
	}
	else
		return -1;
}

bool BR_EnvFree (void* envelope, bool commit)
{
	if (envelope)
	{
		bool commited = (commit) ? ((BR_Envelope*)envelope)->Commit() : false;

		delete envelope;
		return commited;
	}
	else
		return false;
}

bool BR_EnvGetPoint (void* envelope, int id, double* position, double* value, int* shape, bool* selected, double* bezier)
{
	if (envelope)
	{
		BR_Envelope* env = (BR_Envelope*)envelope;

		WritePtr(selected, env->GetSelection(id));
		return env->GetPoint(id, position, value, shape, bezier);
	}
	else
		return false;
}

void BR_EnvGetProperties (void* envelope, bool* active, bool* visible, bool* armed, bool* inLane, int* laneHeight, int* defaultShape, double* minValue, double* maxValue, double* centerValue, int* type)
{
	if (envelope)
	{
		BR_Envelope* env = (BR_Envelope*)envelope;

		WritePtr(active,       env->IsActive());
		WritePtr(visible,      env->IsVisible());
		WritePtr(armed,        env->IsArmed());
		WritePtr(inLane,       env->IsInLane());
		WritePtr(laneHeight,   env->LaneHeight());
		WritePtr(defaultShape, env->DefaultShape());
		WritePtr(minValue,     env->MinValue());
		WritePtr(maxValue,     env->MaxValue());
		WritePtr(centerValue,  env->CenterValue());

		if (type)
		{
			int value = env->Type();
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

			*type = value;
		}
	}
	else
	{
		WritePtr(active,       false);
		WritePtr(visible,      false);
		WritePtr(armed,        false);
		WritePtr(inLane,       false);
		WritePtr(laneHeight,   0);
		WritePtr(defaultShape, 0);
		WritePtr(minValue,     0.0);
		WritePtr(maxValue,     0.0);
		WritePtr(centerValue,  0.0);
	}
}

bool BR_EnvSetPoint (void* envelope, int id, double position, double value, int shape, bool selected, double bezier)
{
	if (envelope)
		return ((BR_Envelope*)envelope)->SetCreateSortedPoint(id, position, value, shape, bezier, selected);
	else
		return false;
}

void BR_EnvSetProperties (void* envelope, bool active, bool visible, bool armed, bool inLane, int laneHeight, int defaultShape)
{
	if (envelope)
	{
		BR_Envelope* env = (BR_Envelope*)envelope;

		env->SetActive(active);
		env->SetVisible(visible);
		env->SetArmed(armed);
		env->SetInLane(inLane);
		env->SetLaneHeight(laneHeight);
		if (defaultShape >= 0 && defaultShape <= 5)
			env->SetDefaultShape(defaultShape);
	}
}

double BR_EnvValueAtPos (void* envelope, double position)
{
	if (envelope)
		return ((BR_Envelope*)envelope)->ValueAtPosition(position);
	else
		return 0;
}

bool BR_GetMediaSourceProperties (MediaItem_Take* take, bool* section, double* start, double* length, double* fade, bool* reverse)
{
	return GetMediaSourceProperties(take, section, start, length, fade, reverse);
}

void BR_GetMouseCursorContext (char* window, char* segment, char* details, int char_sz)
{
	const char* _window;
	const char* _segment;
	const char* _details;
	GetMouseCursorContext(&_window, &_segment, &_details, &g_mouseInfo);

	if (window)  _snprintfSafe(window,  char_sz-1, "%s", _window);
	if (segment) _snprintfSafe(segment, char_sz-1, "%s", _segment);
	if (details) _snprintfSafe(details, char_sz-1, "%s", _details);
}

TrackEnvelope* BR_GetMouseCursorContext_Envelope (bool* takeEnvelope)
{
	WritePtr(takeEnvelope, g_mouseInfo.takeEnvelope);
	return g_mouseInfo.envelope;
}

MediaItem* BR_GetMouseCursorContext_Item ()
{
	return g_mouseInfo.item;
}

void* BR_GetMouseCursorContext_MIDI (bool* inlineEditor, int* noteRow, int* ccLane, int* ccLaneVal, int* ccLaneId)
{
	WritePtr(inlineEditor, g_mouseInfo.midiInlineEditor);
	WritePtr(noteRow,      g_mouseInfo.noteRow);
	WritePtr(ccLane,       g_mouseInfo.ccLane);
	WritePtr(ccLaneVal,    g_mouseInfo.ccLaneVal);
	WritePtr(ccLaneId,     g_mouseInfo.ccLaneId);

	return g_mouseInfo.midiEditor;
}

double BR_GetMouseCursorContext_Position ()
{
	return g_mouseInfo.position;
}

MediaItem_Take* BR_GetMouseCursorContext_Take ()
{
	return g_mouseInfo.take;
}

MediaTrack* BR_GetMouseCursorContext_Track ()
{
	return g_mouseInfo.track;
}

MediaItem* BR_ItemAtMouseCursor (double* position)
{
	return ItemAtMouseCursor(position);
}

bool BR_MIDI_CCLaneReplace (void* midiEditor, int laneId, int newCC)
{
	MediaItem_Take* take = MIDIEditor_GetTake(midiEditor);
	int newLane = MapCCToVelLane(newCC);

	if (take && IsVelLaneValid(newLane))
	{
		MediaItem* item = GetMediaItemTake_Item(take);
		int takeId = GetTakeIndex(item, take);
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

bool BR_MIDI_CCLaneRemove (void* midiEditor, int laneId)
{
	MediaItem_Take* take = MIDIEditor_GetTake(midiEditor);

	if (take)
	{
		MediaItem* item = GetMediaItemTake_Item(take);
		int takeId = GetTakeIndex(item, take);
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
	return PositionAtMouseCursor(checkRuler);
}

bool BR_SetMediaSourceProperties (MediaItem_Take* take, bool section, double start, double length, double fade, bool reverse)
{
	return SetMediaSourceProperties(take, section, start, length, fade, reverse);
}

bool BR_SetTakeSourceFromFile (MediaItem_Take* take, const char* filename, bool inProjectData)
{
	return SetTakeSourceFromFile(take, filename, inProjectData, false);
}

bool BR_SetTakeSourceFromFile2 (MediaItem_Take* take, const char* filename, bool inProjectData, bool keepSourceProperties)
{
	return SetTakeSourceFromFile(take, filename, inProjectData, keepSourceProperties);
}

MediaItem_Take* BR_TakeAtMouseCursor (double* position)
{
	return TakeAtMouseCursor(position);
}

MediaTrack* BR_TrackAtMouseCursor (int* context, double* position)
{
	return TrackAtMouseCursor(context, position);
}