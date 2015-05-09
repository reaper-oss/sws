/*****************************************************************************
/ BR_ReaScript.cpp
/
/ Copyright (c) 2014-2015 Dominik Martin Drzic
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
#include "BR_ReaScript.h"
#include "BR_EnvelopeUtil.h"
#include "BR_MidiUtil.h"
#include "BR_MouseUtil.h"
#include "BR_Util.h"
#include "../SnM/SnM.h"
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
		WritePtr(minValueOut,        envelope->LaneMinValue());
		WritePtr(maxValueOut,        envelope->LaneMaxValue());
		WritePtr(centerValueOut,     envelope->LaneCenterValue());
		WritePtr(faderScalingOut,    envelope->IsScaledToFader());

		if (typeOut)
		{
			int value = envelope->Type();
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
			else                            value = -1;

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
		WritePtr(typeOut,            -1);
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

void BR_GetArrangeView (ReaProject* proj, double* startPositionOut, double* endPositionOut)
{
	double start, end;
	RECT r; GetWindowRect(GetArrangeWnd(), &r);
	GetSet_ArrangeView2(NULL, false, r.left, r.right-SCROLLBAR_W, &start, &end);

	WritePtr(startPositionOut, start);
	WritePtr(endPositionOut, end);
}

double BR_GetClosestGridDivision (double position)
{
	return GetClosestGridDiv(position);
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
	if (item && guidStringOut && guidStringOut_sz > 0)
	{
		char guid[64];
		if (item) guidToString((GUID*)GetSetMediaItemInfo(item, "GUID", NULL), guid);
		else      guidToString(&GUID_NULL, guid);
		_snprintfSafe(guidStringOut, guidStringOut_sz, "%s", guid);
	}
}

bool BR_GetMediaItemImageResource (MediaItem* item, char* imageOut, int imageOut_sz, int* imageFlagsOut)
{
	bool resourceFound = false;
	if (item)
	{
		SNM_ChunkParserPatcher p(item);
		p.SetWantsMinimalState(true);

		char image[SNM_MAX_PATH]      = "";
		char imageFlags[SNM_MAX_PATH] = "0";

		resourceFound =  !!p.Parse(SNM_GET_CHUNK_CHAR, 1, "ITEM", "RESOURCEFN",       0, 1, image,      NULL, "VOLPAN");
		if (resourceFound) p.Parse(SNM_GET_CHUNK_CHAR, 1, "ITEM", "IMGRESOURCEFLAGS", 0, 1, imageFlags, NULL, "VOLPAN");

		if (imageOut && imageOut_sz > 0) _snprintfSafe(imageOut, imageOut_sz, "%s", image);
		WritePtr(imageFlagsOut, atoi(imageFlags));
	}
	return resourceFound;
}

void BR_GetMediaItemTakeGUID (MediaItem_Take* take, char* guidStringOut, int guidStringOut_sz)
{
	if (take && guidStringOut && guidStringOut_sz > 0)
	{
		char guid[64];
		if (take) guidToString((GUID*)GetSetMediaItemTakeInfo(take, "GUID", NULL), guid);
		else      guidToString(&GUID_NULL, guid);
		_snprintfSafe(guidStringOut,  guidStringOut_sz, "%s", guid);
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

int BR_GetMediaTrackFreezeCount (MediaTrack* track)
{
	return GetTrackFreezeCount(track);
}

void BR_GetMediaTrackGUID (MediaTrack* track, char* guidStringOut, int guidStringOut_sz)
{
	if (track && guidStringOut && guidStringOut_sz > 0)
	{
		char guid[64];
		if (track) guidToString(TrackToGuid(track), guid);
		else       guidToString(&GUID_NULL, guid);
		_snprintfSafe(guidStringOut, guidStringOut_sz, "%s", guid);
	}
}

void BR_GetMediaTrackLayouts (MediaTrack* track, char* mcpLayoutNameOut, int mcpLayoutNameOut_sz, char* tcpLayoutNameOut, int tcpLayoutNameOut_sz)
{
	bool nothingFound = true;

	if (track && (mcpLayoutNameOut || tcpLayoutNameOut))
	{
		SNM_ChunkParserPatcher p(track);
		p.SetWantsMinimalState(true);

		WDL_FastString layoutsLine;
		if (p.Parse(SNM_GET_SUBCHUNK_OR_LINE, 1, "TRACK", "LAYOUTS", 0, 1, &layoutsLine))
		{
			LineParser lp(false);
			lp.parse(layoutsLine.Get());
			if (mcpLayoutNameOut && mcpLayoutNameOut_sz > 0) _snprintfSafe(mcpLayoutNameOut, mcpLayoutNameOut_sz, "%s", lp.gettoken_str(2));
			if (tcpLayoutNameOut && tcpLayoutNameOut_sz > 0) _snprintfSafe(tcpLayoutNameOut, tcpLayoutNameOut_sz, "%s", lp.gettoken_str(1));

			nothingFound = false;
		}
	}

	if (nothingFound)
	{
		if (mcpLayoutNameOut && mcpLayoutNameOut_sz > 0) _snprintfSafe(mcpLayoutNameOut, mcpLayoutNameOut_sz, "%s", "");
		if (tcpLayoutNameOut && tcpLayoutNameOut_sz > 0) _snprintfSafe(tcpLayoutNameOut, tcpLayoutNameOut_sz, "%s", "");
	}
}

TrackEnvelope* BR_GetMediaTrackSendInfo_Envelope (MediaTrack* track, int category, int sendidx, int envelopeType)
{
	const char* sendType = (envelopeType == 0) ? ("<VOLENV") : (envelopeType == 1) ? ("<PANENV") : (envelopeType == 2) ? ("<MUTEENV") : NULL;

	if (sendType)
		return (TrackEnvelope*)GetSetTrackSendInfo(track, category, sendidx, "P_ENV", (void*)sendType);
	else
		return NULL;
}

MediaTrack* BR_GetMediaTrackSendInfo_Track (MediaTrack* track, int category, int sendidx, int trackType)
{
	if (trackType == 0 || trackType == 1)
		return (MediaTrack*)GetSetTrackSendInfo(track, category, sendidx, ((trackType == 0) ? "P_SRCTRACK" : "P_DESTTRACK"), NULL);
	else
		return NULL;
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

bool BR_GetMidiTakeTempoInfo (MediaItem_Take* take, bool* ignoreProjTempoOut, double* bpmOut, int* numOut, int* denOut)
{
	bool   ignoreTempo = false;
	double bpm         = 0;
	int    num         = 0;
	int    den         = 0;

	bool succes = false;
	if (take && IsMidi(take, NULL))
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
				WDL_FastString tempoLine;
				if (ptk.Parse(SNM_GET_SUBCHUNK_OR_LINE, 1, "SOURCE", "IGNTEMPO", 0, -1, &tempoLine))
				{
					LineParser lp(false);
					lp.parse(tempoLine.Get());

					ignoreTempo = !!lp.gettoken_int(1);
					bpm         = lp.gettoken_float(2);
					num         = lp.gettoken_int(3);
					den         = lp.gettoken_int(4);
					succes = true;
				}
			}
		}
	}

	WritePtr(ignoreProjTempoOut, ignoreTempo);
	WritePtr(bpmOut,             bpm);
	WritePtr(numOut,             num);
	WritePtr(denOut,             den);
	return succes;
}

void BR_GetMouseCursorContext (char* windowOut, int windowOut_sz, char* segmentOut, int segmentOut_sz, char* detailsOut, int detailsOut_sz)
{
	g_mouseInfo.Update();

	if (windowOut  && windowOut_sz  > 0) _snprintfSafe(windowOut,  windowOut_sz,  "%s", g_mouseInfo.GetWindow());
	if (segmentOut && segmentOut_sz > 0) _snprintfSafe(segmentOut, segmentOut_sz, "%s", g_mouseInfo.GetSegment());
	if (detailsOut && detailsOut_sz > 0) _snprintfSafe(detailsOut, detailsOut_sz, "%s", g_mouseInfo.GetDetails());
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

double BR_GetNextGridDivision (double position)
{
	if (position >= 0)
		return GetNextGridDiv(position);
	else
		return 0;
}

double BR_GetPrevGridDivision (double position)
{
	if (position > 0)
		return GetPrevGridDiv(position);
	else
		return 0;
}

double BR_GetSetTrackSendInfo (MediaTrack* track, int category, int sendidx, const char* parmname, bool setNewValue, double newValue)
{
	double returnValue = 0;
	if (track)
	{
		if (setNewValue)
		{
			if (!strcmp(parmname, "B_MUTE") || !strcmp(parmname, "B_PHASE") || !strcmp(parmname, "B_MONO"))
			{
				bool valueToSet = !!newValue;
				GetSetTrackSendInfo(track, category, sendidx, parmname, (void*)&valueToSet);
				returnValue = 1;
			}
			else if (!strcmp(parmname, "D_VOL") || !strcmp(parmname, "D_PAN") || !strcmp(parmname, "D_PANLAW"))
			{
				double valueToSet = (double)newValue;
				GetSetTrackSendInfo(track, category, sendidx, parmname, (void*)&valueToSet);
				returnValue = 1;
			}
			else if (!strcmp(parmname, "I_SENDMODE") || !strcmp(parmname, "I_SRCCHAN") || !strcmp(parmname, "I_DSTCHAN") || !strcmp(parmname, "I_MIDIFLAGS"))
			{
				int valueToSet = (int)newValue;
				GetSetTrackSendInfo(track, category, sendidx, parmname, (void*)&valueToSet);
				returnValue = 1;
			}
			else if (!strcmp(parmname, "I_MIDI_SRCCHAN") || !strcmp(parmname, "I_MIDI_DSTCHAN") || !strcmp(parmname, "I_MIDI_SRCBUS") || !strcmp(parmname, "I_MIDI_DSTBUS"))
			{
				if (void* sendInfo = GetSetTrackSendInfo(track, category, sendidx, "I_MIDIFLAGS", NULL))
				{
					int valueToSet = *(int*)sendInfo;
					if      (newValue == -1)                      valueToSet |= 0x3FC01F;                                            // set all bits for source channel and bus to 1
					else if (!strcmp(parmname, "I_MIDI_SRCCHAN")) valueToSet = (valueToSet & ~(0x1F))       | ((int)newValue);       // source chan is bits 0-4
					else if (!strcmp(parmname, "I_MIDI_DSTCHAN")) valueToSet = (valueToSet & ~(0x1F << 5))  | ((int)newValue << 5);  // destination chan is bits 5-9
					else if (!strcmp(parmname, "I_MIDI_SRCBUS"))  valueToSet = (valueToSet & ~(0xFF << 14)) | ((int)newValue << 14); // source MIDI bus is bits 14-21
					else if (!strcmp(parmname, "I_MIDI_DSTBUS"))  valueToSet = (valueToSet & ~(0xFF << 22)) | ((int)newValue << 22); // destination MIDI bus is bits 22-29
					GetSetTrackSendInfo(track, category, sendidx, "I_MIDIFLAGS", (void*)&valueToSet);
				}
			}
		}
		else
		{
			if (!strcmp(parmname, "I_MIDI_SRCCHAN") || !strcmp(parmname, "I_MIDI_DSTCHAN") || !strcmp(parmname, "I_MIDI_SRCBUS") || !strcmp(parmname, "I_MIDI_DSTBUS"))
			{
				if (void* sendInfo = GetSetTrackSendInfo(track, category, sendidx, "I_MIDIFLAGS", NULL))
				{
					int midiFlags = *(int*)sendInfo;
					if      ((midiFlags & 31) == 31)              returnValue = -1;
					else if (!strcmp(parmname, "I_MIDI_SRCCHAN")) returnValue = midiFlags         & 0x1F; // source chan is bits 0-4
					else if (!strcmp(parmname, "I_MIDI_DSTCHAN")) returnValue = (midiFlags >> 5)  & 0x1F; // destination chan is bits 5-9
					else if (!strcmp(parmname, "I_MIDI_SRCBUS"))  returnValue = (midiFlags >> 14) & 0xFF; // source MIDI bus is bits 14-21
					else if (!strcmp(parmname, "I_MIDI_DSTBUS"))  returnValue = (midiFlags >> 22) & 0xFF; // destination MIDI bus is bits 22-29
				}
			}

			else if (void* sendInfo = GetSetTrackSendInfo(track, category, sendidx, parmname, NULL))
			{
				if (!strcmp(parmname, "B_MUTE") || !strcmp(parmname, "B_PHASE") || !strcmp(parmname, "B_MONO"))
				{
					returnValue = (double)(*(bool*)sendInfo);
				}
				else if (!strcmp(parmname, "D_VOL") || !strcmp(parmname, "D_PAN") || !strcmp(parmname, "D_PANLAW"))
				{
					returnValue = *(double*)sendInfo;
				}
				else if (!strcmp(parmname, "I_SENDMODE") || !strcmp(parmname, "I_SRCCHAN") || !strcmp(parmname, "I_DSTCHAN"))
				{
					returnValue = (double)(*(int*)sendInfo);
				}

			}
		}
	}

	return returnValue;
}

int BR_GetTakeFXCount (MediaItem_Take* take)
{
	return GetTakeFXCount(take);
}

bool BR_IsTakeMidi (MediaItem_Take* take, bool* inProjectMidiOut)
{
	return IsMidi(take, inProjectMidiOut);
}

MediaItem* BR_ItemAtMouseCursor (double* positionOut)
{
	return ItemAtMouseCursor(positionOut);
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

double BR_PositionAtMouseCursor (bool checkRuler)
{
	return PositionAtMouseCursor(checkRuler, true);
}

void BR_SetArrangeView (ReaProject* proj, double startPosition, double endPosition)
{
	RECT r; GetWindowRect(GetArrangeWnd(), &r);
	GetSet_ArrangeView2(proj, true, r.left, r.right - SCROLLBAR_W, &startPosition, &endPosition);
}

bool BR_SetItemEdges (MediaItem* item, double startTime, double endTime)
{
	return TrimItem(item, startTime, endTime);
}

void BR_SetMediaItemImageResource (MediaItem* item, const char* imageIn, int imageFlags)
{
	if (item)
	{
		char* itemState = GetSetObjectState(item, "");
		char* token = strtok(itemState, "\n");

		WDL_FastString newState;
		LineParser lp(false);

		bool didImage      = !imageIn;
		bool didImageFlags = false;
		bool doImageFlags  = (!imageIn || (imageIn && strcmp(imageIn, "")));
		bool commitTwice   = false; // in case image path is the same as one already set in the item, reaper will remove it in case we supply chunk with the same path (and sometimes we just want to change the flag)
		bool skipCommit    = false;

		int blockCount = 0;
		while (token != NULL)
		{
			lp.parse(token);
			if      (lp.gettoken_str(0)[0] == '<')  ++blockCount;
			else if (lp.gettoken_str(0)[0] == '>')  --blockCount;

			if (blockCount == 1)
			{
				if (!strcmp(lp.gettoken_str(0), "RESOURCEFN") && !didImage)
				{
					if (strcmp(imageIn, ""))
					{
						for (int i = 0; i < lp.getnumtokens(); ++i)
						{
							if (i == 1) newState.AppendFormatted(SNM_MAX_PATH + 2, "\"%s\"", imageIn);
							else        newState.Append(lp.gettoken_str(i));
							newState.Append(" ");
						}
						newState.Append("\n");

						if (!strcmp(imageIn, lp.gettoken_str(1)))
							commitTwice = true;
					}
					else
					{
						doImageFlags = false;
					}


					didImage = true;
				}
				else if (!strcmp(lp.gettoken_str(0), "IMGRESOURCEFLAGS") && !didImageFlags)
				{
					if (doImageFlags)
					{
						for (int i = 0; i < lp.getnumtokens(); ++i)
						{
							if (i == 1) newState.AppendFormatted(128, "%d", imageFlags);
							else        newState.Append(lp.gettoken_str(i));
							newState.Append(" ");
						}
						newState.Append("\n");

						if (commitTwice && lp.gettoken_int(1) == imageFlags)
							skipCommit = true;
					}


					didImageFlags = true;
				}
				else if (lp.gettoken_str(0)[0] == '>')
				{
					AppendLine(newState, token);

					if (!didImage && strcmp(imageIn, ""))
					{
						newState.Append("RESOURCEFN \"");
						newState.Append(imageIn);
						newState.Append("\"");
						newState.Append("\n");
						didImage = true;
					}

					if (didImage && !didImageFlags && doImageFlags)
					{
						newState.Append("IMGRESOURCEFLAGS ");
						newState.AppendFormatted(128, "%d", imageFlags);
						newState.Append("\n");
						didImageFlags = true;
					}
				}
				else
				{
					AppendLine(newState, token);
				}
			}
			else
				AppendLine(newState, token);

			token = strtok(NULL, "\n");
		}

		if (!skipCommit)
		{
			GetSetObjectState(item, newState.Get());
			if (commitTwice)
				GetSetObjectState(item, newState.Get());
		}
		FreeHeapPtr(itemState);
	}
}

bool BR_SetMediaSourceProperties (MediaItem_Take* take, bool section, double start, double length, double fade, bool reverse)
{
	return SetMediaSourceProperties(take, section, start, length, fade, reverse);
}

bool BR_SetMediaTrackLayouts (MediaTrack* track, const char* mcpLayoutNameIn, const char* tcpLayoutNameIn)
{
	bool updated = false;
	if (track && (mcpLayoutNameIn || tcpLayoutNameIn))
	{
		char* trackState = GetSetObjectState(track, "");
		char* token = strtok(trackState, "\n");

		WDL_FastString newState;
		LineParser lp(false);

		int blockCount = 0;
		bool didLayouts = false;
		while (token != NULL)
		{
			lp.parse(token);
			if      (lp.gettoken_str(0)[0] == '<')  ++blockCount;
			else if (lp.gettoken_str(0)[0] == '>')  --blockCount;

			if (blockCount == 1)
			{
				if (!strcmp(lp.gettoken_str(0), "LAYOUTS") && !didLayouts)
				{
					for (int i = 0; i < lp.getnumtokens(); ++i)
					{
						if (i == 1)
						{
							newState.AppendFormatted(512, "\"%s\"", (tcpLayoutNameIn ? tcpLayoutNameIn : lp.gettoken_str(i)));
							if (tcpLayoutNameIn && strcmp(tcpLayoutNameIn, lp.gettoken_str(i)))
								updated = true;
						}
						else if (i == 2)
						{
							newState.AppendFormatted(512, "\"%s\"", (mcpLayoutNameIn ? mcpLayoutNameIn : lp.gettoken_str(i)));
							if (mcpLayoutNameIn && strcmp(mcpLayoutNameIn, lp.gettoken_str(i)))
								updated = true;
						}
						else
						{
							newState.Append(lp.gettoken_str(i));
						}
						newState.Append(" ");
					}

					newState.Append("\n");
					didLayouts = true;
				}
				else
				{
					AppendLine(newState, token);
				}
			}
			else if (blockCount == 0)
			{
				if (lp.gettoken_str(0)[0] == '>')
				{
					if (!didLayouts)
					{
						if ((tcpLayoutNameIn && strcmp(tcpLayoutNameIn, "")) || (mcpLayoutNameIn && strcmp(mcpLayoutNameIn, "")))
							updated = true;

						if (updated)
						{
							newState.Append("LAYOUTS ");

							newState.Append("\"");
							newState.Append((tcpLayoutNameIn ? tcpLayoutNameIn : ""));
							newState.Append("\"");

							newState.Append("\"");
							newState.Append((mcpLayoutNameIn ? mcpLayoutNameIn : ""));
							newState.Append("\"");

							newState.Append("\n");
						}
					}
					AppendLine(newState, token);
				}
				else
				{
					AppendLine(newState, token);
				}
			}
			else
			{
				AppendLine(newState, token);
			}

			token = strtok(NULL, "\n");
		}

		if (updated)
			GetSetObjectState(track, newState.Get());
		FreeHeapPtr(trackState);
	}
	return updated;
}

bool BR_SetMidiTakeTempoInfo (MediaItem_Take* take, bool ignoreProjTempo, double bpm, int num, int den)
{
	bool succes = false;
	if (take && IsMidi(take, NULL))
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

				WDL_FastString tempoLine;
				if (int position = ptk.Parse(SNM_GET_SUBCHUNK_OR_LINE, 1, "SOURCE", "IGNTEMPO", 0, -1, &tempoLine))
				{
					LineParser lp(false);
					lp.parse(tempoLine.Get());

					if (ignoreProjTempo != !!lp.gettoken_int(1) ||  bpm != lp.gettoken_float(2) || num != lp.gettoken_int(3) || den != lp.gettoken_int(4))
					{
						WDL_FastString newLine;
						for (int i = 0; i < lp.getnumtokens(); ++i)
						{
							if      (i == 1) newLine.AppendFormatted(256, "%d",  ignoreProjTempo ? 1 : 0);
							else if (i == 2) newLine.AppendFormatted(256, "%lf", bpm);
							else if (i == 3) newLine.AppendFormatted(256, "%d",  num);
							else if (i == 4) newLine.AppendFormatted(256, "%d",  den);
							else             newLine.AppendFormatted(256, "%s", lp.gettoken_str(i));
							newLine.Append(" ");
						}
						newLine.Append("\n");
						ptk.ReplaceLine(position - 1, newLine.Get());
						succes = p.ReplaceTake(tkPos, tklen, ptk.GetChunk());
					}

				}
			}
		}
	}

	return succes;
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