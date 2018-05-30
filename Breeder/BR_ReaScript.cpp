/*****************************************************************************
/ BR_ReaScript.cpp
/
/ Copyright (c) 2014-2015 Dominik Martin Drzic
/ http://forum.cockos.com/member.php?u=27094
/ http://github.com/reaper-oss/sws
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

// #781
#include "../Misc/Analysis.h"

// #880
#include "../Breeder/BR_Loudness.h"

// #755
#include "../SnM/SnM_Notes.h"

/******************************************************************************
* Globals                                                                     *
******************************************************************************/
static BR_MouseInfo g_mouseInfo(BR_MouseInfo::MODE_ALL, false);
WDL_PtrList_DOD<BR_Envelope> g_script_brenvs; // just to validate function parameters

/******************************************************************************
* ReaScript export                                                            *
******************************************************************************/
BR_Envelope* BR_EnvAlloc (TrackEnvelope* envelope, bool takeEnvelopesUseProjectTime)
{
	if (envelope)
		return g_script_brenvs.Add(new BR_Envelope(envelope, takeEnvelopesUseProjectTime));
	return NULL;
}

int BR_EnvCountPoints (BR_Envelope* envelope)
{
	if (envelope && g_script_brenvs.Find(envelope)>=0)
		return envelope->CountPoints();
	return 0;
}

bool BR_EnvDeletePoint (BR_Envelope* envelope, int id)
{
	if (envelope && g_script_brenvs.Find(envelope)>=0)
		return envelope->DeletePoint(id);
	return false;
}

int BR_EnvFind (BR_Envelope* envelope, double position, double delta)
{
	if (envelope && g_script_brenvs.Find(envelope)>=0)
	{
		int id = envelope->Find(position, delta);
		if (envelope->ValidateId(id))
			return id;
	}
	return -1;
}

int BR_EnvFindNext (BR_Envelope* envelope, double position)
{
	if (envelope && g_script_brenvs.Find(envelope)>=0)
	{
		int id = envelope->FindNext(position);
		if (envelope->ValidateId(id))
			return id;
	}
	return -1;
}

int BR_EnvFindPrevious (BR_Envelope* envelope, double position)
{
	if (envelope && g_script_brenvs.Find(envelope)>=0)
	{
		int id = envelope->FindPrevious(position);
		if (envelope->ValidateId(id))
			return id;
	}
	return -1;
}

bool BR_EnvFree (BR_Envelope* envelope, bool commit)
{
	int idx=g_script_brenvs.Find(envelope);
	if (envelope && idx>=0)
	{
		bool commited = (commit) ? envelope->Commit() : false;
		g_script_brenvs.Delete(idx, true);
		return commited;
	}
	return false;
}

MediaItem_Take* BR_EnvGetParentTake (BR_Envelope* envelope)
{
	if (envelope && g_script_brenvs.Find(envelope)>=0)
		return envelope->GetTake();
	return NULL;
}

MediaTrack* BR_EnvGetParentTrack (BR_Envelope* envelope)
{
	if (envelope && g_script_brenvs.Find(envelope)>=0)
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
	if (envelope && g_script_brenvs.Find(envelope)>=0)
	{
		WritePtr(selectedOut, envelope->GetSelection(id));
		return envelope->GetPoint(id, positionOut, valueOut, shapeOut, bezierOut);
	}
	return false;
}

void BR_EnvGetProperties (BR_Envelope* envelope, bool* activeOut, bool* visibleOut, bool* armedOut, bool* inLaneOut, int* laneHeightOut, int* defaultShapeOut, double* minValueOut, double* maxValueOut, double* centerValueOut, int* typeOut, bool* faderScalingOut)
{
	if (envelope && g_script_brenvs.Find(envelope)>=0)
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
	if (envelope && g_script_brenvs.Find(envelope)>=0)
		return envelope->SetCreatePoint(id, position, value, shape, bezier, selected);
	return false;
}

void BR_EnvSetProperties (BR_Envelope* envelope, bool active, bool visible, bool armed, bool inLane, int laneHeight, int defaultShape, bool faderScaling)
{
	if (envelope && g_script_brenvs.Find(envelope)>=0)
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
	if (envelope && g_script_brenvs.Find(envelope)>=0)
		envelope->Sort();
}

double BR_EnvValueAtPos (BR_Envelope* envelope, double position)
{
	if (envelope && g_script_brenvs.Find(envelope)>=0)
		return envelope->ValueAtPosition(position);
	return 0;
}

void BR_GetArrangeView (ReaProject* proj, double* startPositionOut, double* endPositionOut)
{
	double start, end;
	GetSetArrangeView(proj, false, &start, &end);

	WritePtr(startPositionOut, start);
	WritePtr(endPositionOut, end);
}

double BR_GetClosestGridDivision (double position)
{
	return GetClosestGridDiv(position);
}

void BR_GetCurrentTheme (char* themePathOut, int themePathOut_sz, char* themeNameOut, int themeNameOut_sz)
{
	WDL_FastString fullThemePath;
	WDL_FastString themeName = GetCurrentThemeName(&fullThemePath);

	_snprintfSafe(themePathOut, themePathOut_sz, "%s", fullThemePath.Get());
	_snprintfSafe(themeNameOut, themeNameOut_sz, "%s", themeName.Get());
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
		_snprintfSafe(guidStringOut, guidStringOut_sz, "%s", guid);
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
	if (mcpLayoutNameOut && mcpLayoutNameOut_sz > 0) *mcpLayoutNameOut=0;
	if (tcpLayoutNameOut && tcpLayoutNameOut_sz > 0) *tcpLayoutNameOut=0;
	if (track)
	{
		if (mcpLayoutNameOut)
		{
			const char *l = (const char*)GetSetMediaTrackInfo(track, "P_MCP_LAYOUT", NULL);
			if (l) _snprintfSafe(mcpLayoutNameOut, mcpLayoutNameOut_sz, "%s", l);
		}
		if (tcpLayoutNameOut)
		{
			const char *l = (const char*)GetSetMediaTrackInfo(track, "P_TCP_LAYOUT", NULL);
			if (l) _snprintfSafe(tcpLayoutNameOut, tcpLayoutNameOut_sz, "%s", l);
		}
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
		length = GetMidiSourceLengthPPQ(take, false, &isMidi);
		if (!isMidi) length = -1;
	}
	return length;
}

bool BR_GetMidiTakePoolGUID (MediaItem_Take* take, char* guidStringOut, int guidStringOut_sz)
{
	if (take && IsMidi(take, NULL) && guidStringOut)
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
				WDL_FastString pooledEventsLine;
				if (ptk.Parse(SNM_GET_SUBCHUNK_OR_LINE, 1, "SOURCE", "POOLEDEVTS", 0, -1, &pooledEventsLine))
				{
					LineParser lp(false);
					lp.parse(pooledEventsLine.Get());
					_snprintfSafe(guidStringOut, guidStringOut_sz, "%s", lp.gettoken_str(1));
				}
			}
		}
	}

	if (PCM_source* source = GetMediaItemTake_Source(take))
		return !strcmp(source->GetType(), "MIDIPOOL");
	else
		return false;
}

bool BR_GetMidiTakeTempoInfo (MediaItem_Take* take, bool* ignoreProjTempoOut, double* bpmOut, int* numOut, int* denOut)
{
	return GetMidiTakeTempoInfo (take, ignoreProjTempoOut, bpmOut, numOut, denOut);
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
	return (void*)g_mouseInfo.GetMidiEditor();
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
			else if (!strcmp(parmname, "I_MIDI_LINK_VOLPAN") || !strcmp(parmname, "I_MIDI_SRCCHAN") || !strcmp(parmname, "I_MIDI_DSTCHAN") || !strcmp(parmname, "I_MIDI_SRCBUS") || !strcmp(parmname, "I_MIDI_DSTBUS"))
			{
				if (void* sendInfo = GetSetTrackSendInfo(track, category, sendidx, "I_MIDIFLAGS", NULL))
				{
					int valueToSet = *(int*)sendInfo;
					if      (newValue == -1)                          valueToSet |= 0x3FC01F;                                            // set all bits for source channel and bus to 1
					else if (!strcmp(parmname, "I_MIDI_LINK_VOLPAN")) valueToSet = SetBit(valueToSet, 10, (newValue != 0));
					else if (!strcmp(parmname, "I_MIDI_SRCCHAN"))     valueToSet = (valueToSet & ~(0x1F))       | ((int)newValue);       // source chan is bits 0-4
					else if (!strcmp(parmname, "I_MIDI_DSTCHAN"))     valueToSet = (valueToSet & ~(0x1F << 5))  | ((int)newValue << 5);  // destination chan is bits 5-9
					else if (!strcmp(parmname, "I_MIDI_SRCBUS"))      valueToSet = (valueToSet & ~(0xFF << 14)) | ((int)newValue << 14); // source MIDI bus is bits 14-21
					else if (!strcmp(parmname, "I_MIDI_DSTBUS"))      valueToSet = (valueToSet & ~(0xFF << 22)) | ((int)newValue << 22); // destination MIDI bus is bits 22-29
					GetSetTrackSendInfo(track, category, sendidx, "I_MIDIFLAGS", (void*)&valueToSet);
					returnValue = 1;
				}
			}
		}
		else
		{
			if (!strcmp(parmname, "I_MIDI_LINK_VOLPAN") || !strcmp(parmname, "I_MIDI_SRCCHAN") || !strcmp(parmname, "I_MIDI_DSTCHAN") || !strcmp(parmname, "I_MIDI_SRCBUS") || !strcmp(parmname, "I_MIDI_DSTBUS"))
			{
				if (void* sendInfo = GetSetTrackSendInfo(track, category, sendidx, "I_MIDIFLAGS", NULL))
				{
					int midiFlags = *(int*)sendInfo;
					if      ((midiFlags & 31) == 31)                  returnValue = -1;
					else if (!strcmp(parmname, "I_MIDI_LINK_VOLPAN")) returnValue = (GetBit(midiFlags, 10) ? 1 : 0);
					else if (!strcmp(parmname, "I_MIDI_SRCCHAN"))     returnValue = midiFlags         & 0x1F; // source chan is bits 0-4
					else if (!strcmp(parmname, "I_MIDI_DSTCHAN"))     returnValue = (midiFlags >> 5)  & 0x1F; // destination chan is bits 5-9
					else if (!strcmp(parmname, "I_MIDI_SRCBUS"))      returnValue = (midiFlags >> 14) & 0xFF; // source MIDI bus is bits 14-21
					else if (!strcmp(parmname, "I_MIDI_DSTBUS"))      returnValue = (midiFlags >> 22) & 0xFF; // destination MIDI bus is bits 22-29
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

bool BR_IsMidiOpenInInlineEditor(MediaItem_Take* take)
{
	return IsOpenInInlineEditor(take);
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
	GetSetArrangeView(proj, true, &startPosition, &endPosition);
}

bool BR_SetItemEdges (MediaItem* item, double startTime, double endTime)
{
	return TrimItem_UseNativeTrimActions(item, startTime, endTime);
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
	bool changedLayouts = false;
	if (track)
	{
		if (mcpLayoutNameIn)
		{
			const char* currentMpc = (const char*)GetSetMediaTrackInfo(track, "P_MCP_LAYOUT", NULL);
			if (currentMpc && strcmp(currentMpc, mcpLayoutNameIn))
			{
				GetSetMediaTrackInfo(track, "P_MCP_LAYOUT", (void*)mcpLayoutNameIn);
				changedLayouts = true;
			}
		}

		if (tcpLayoutNameIn)
		{
			const char* currentMpc = (const char*)GetSetMediaTrackInfo(track, "P_TCP_LAYOUT", NULL);
			if (currentMpc && strcmp(currentMpc, tcpLayoutNameIn))
			{
				GetSetMediaTrackInfo(track, "P_TCP_LAYOUT", (void*)tcpLayoutNameIn);
				changedLayouts = true;
			}
		}
	}
	return changedLayouts;
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

bool BR_TrackFX_GetFXModuleName (MediaTrack* track, int fx, char* nameOut, int nameOutSz)
{
	WDL_FastString module;
	bool found = false;

	if (track)
	{
		SNM_FXSummaryParser trackFxs(track);
		if (SNM_FXSummary* summary =  trackFxs.GetSummaries()->Get(fx))
		{
			module = summary->m_realName;
			found = true;
		}
	}

	_snprintfSafe(nameOut, nameOutSz, "%s", module.Get());
	return found;
}

int BR_Win32_GetPrivateProfileString (const char* sectionName, const char* keyName, const char* defaultString, const char* filePath, char* stringOut, int stringOut_sz)
{
	return (int)GetPrivateProfileString(sectionName, keyName, defaultString, stringOut, stringOut_sz, filePath);
}

int BR_Win32_ShellExecute (const char* operation, const char* file, const char* parameters, const char* directoy, int showFlags)
{
	return (int)ShellExecute(g_hwndParent, operation, file, parameters, directoy, showFlags);
}

bool BR_Win32_WritePrivateProfileString (const char* sectionName, const char* keyName, const char* value, const char* filePath)
{
	return !!WritePrivateProfileString(sectionName, keyName, value, filePath);
}


// *** nofish stuff ***
// #781
double NF_GetMediaItemMaxPeak(MediaItem* item)
{
	double maxPeak = GetMediaItemMaxPeak(item);	
	return maxPeak;
}

double NF_GetMediaItemPeakRMS_Windowed(MediaItem* item)
{
	double peakRMS = GetMediaItemPeakRMS_Windowed(item);
	return peakRMS;
}

double NF_GetMediaItemPeakRMS_NonWindowed(MediaItem* item)
{
	double peakRMSperChannel = GetMediaItemPeakRMS_NonWindowed(item);
	return peakRMSperChannel;
}

double NF_GetMediaItemAverageRMS(MediaItem* item)
{
	double averageRMS = GetMediaItemAverageRMS(item);
	return averageRMS;

}

// #880
bool NF_AnalyzeTakeLoudness_IntegratedOnly(MediaItem_Take * take, double* lufsIntegratedOut)
{
	return NFDoAnalyzeTakeLoudness_IntegratedOnly(take, lufsIntegratedOut);
}

bool NF_AnalyzeTakeLoudness(MediaItem_Take * take, bool analyzeTruePeak, double* lufsIntegratedOut, double* rangeOut, double* truePeakOut, double* truePeakPosOut, double* shorTermMaxOut, double* momentaryMaxOut)
{
	return NFDoAnalyzeTakeLoudness(take, analyzeTruePeak, lufsIntegratedOut, rangeOut, truePeakOut, truePeakPosOut, shorTermMaxOut, momentaryMaxOut);
}

bool NF_AnalyzeTakeLoudness2(MediaItem_Take * take, bool analyzeTruePeak, double* lufsIntegratedOut, double* rangeOut, double* truePeakOut, double* truePeakPosOut, double* shorTermMaxOut, double* momentaryMaxOut, double* shortTermMaxPosOut, double* momentaryMaxPosOut)
{
	return NFDoAnalyzeTakeLoudness2(take, analyzeTruePeak, lufsIntegratedOut, rangeOut, truePeakOut, truePeakPosOut, shorTermMaxOut, momentaryMaxOut, shortTermMaxPosOut, momentaryMaxPosOut);
}

// Track/TakeFX_Get/SetOffline
bool NF_TrackFX_GetOffline(MediaTrack* track, int fx)
{
	char state[2] = "0";
	SNM_ChunkParserPatcher p(track);
	p.SetWantsMinimalState(true);
	if (p.Parse(SNM_GET_CHUNK_CHAR, 2, "FXCHAIN", "BYPASS", fx, 2, state) > 0)
		return !strcmp(state, "1");

	return false;
}

void NF_TrackFX_SetOffline(MediaTrack* track, int fx, bool offline)
{
	SNM_ChunkParserPatcher p(track);
	p.SetWantsMinimalState(true);

	bool updt = false;
	if (offline) {
		updt = (p.ParsePatch(SNM_SET_CHUNK_CHAR, 2, "FXCHAIN", "BYPASS", fx, 2, (void*)"1") > 0);

		// chunk BYPASS 3rd value isn't updated when offlining via chunk. So set it if FX is floating when offlining,
		// to match REAPER behaviour
		// https://forum.cockos.com/showpost.php?p=1901608&postcount=12
		if (updt) {
			HWND hwnd = NULL;
			hwnd = TrackFX_GetFloatingWindow(track, fx);

			if (hwnd) // FX is currently floating, set 3rd value
				p.ParsePatch(SNM_SET_CHUNK_CHAR, 2, "FXCHAIN", "BYPASS", fx, 3, (void*)"1");
			else // not floating
				p.ParsePatch(SNM_SET_CHUNK_CHAR, 2, "FXCHAIN", "BYPASS", fx, 3, (void*)"0");
		}

		// close the GUI for buggy plugins (before chunk update)
		// http://github.com/reaper-oss/sws/issues/317
		int supportBuggyPlug = GetPrivateProfileInt("General", "BuggyPlugsSupport", 0, g_SNM_IniFn.Get());
		if (updt && supportBuggyPlug)
			TrackFX_SetOpen(track, fx, false);
	}
		
	else { // set online
		updt = (p.ParsePatch(SNM_SET_CHUNK_CHAR, 2, "FXCHAIN", "BYPASS", fx, 2, (void*)"0") > 0);

		if (updt) {
			bool committed = false;
			committed = p.Commit(false); // need to commit immediately, otherwise below making FX float wouldn't work 

			if (committed) {
				// check if it should float when onlining
				char state[2] = "0";
				if ((p.Parse(SNM_GET_CHUNK_CHAR, 2, "FXCHAIN", "BYPASS", fx, 3, state) > 0)) {
					if (!strcmp(state, "1"))
						TrackFX_Show(track, fx, 3); // show floating
				}
			}
		}
	}

}


// in chunk take FX Id's are counted consecutively across takes (zero-based)
int FindTakeFXId_inChunk(MediaItem* parentItem, MediaItem_Take* takeIn, int fxIn);


bool NF_TakeFX_GetOffline(MediaItem_Take* takeIn, int fxIn)
{
	// find chunk fx id
	MediaItem* parentItem = GetMediaItemTake_Item(takeIn);
	int takeFXId_inChunk = FindTakeFXId_inChunk(parentItem, takeIn, fxIn);
	
	if (takeFXId_inChunk >= 0) {
		char state[2] = "0";
		SNM_ChunkParserPatcher p(parentItem);
		p.SetWantsMinimalState(true);
		if (p.Parse(SNM_GET_CHUNK_CHAR, 2, "TAKEFX", "BYPASS", takeFXId_inChunk, 2, state) > 0)
			return !strcmp(state, "1");
	}

	return false;
}

void NF_TakeFX_SetOffline(MediaItem_Take* takeIn, int fxIn, bool offline)
{
	// find chunk fx id
	MediaItem* parentItem = GetMediaItemTake_Item(takeIn);
	int takeFXId_inChunk = FindTakeFXId_inChunk(parentItem, takeIn, fxIn);

	if (takeFXId_inChunk >= 0) {
		SNM_ChunkParserPatcher p(parentItem);
		p.SetWantsMinimalState(true);
		bool updt = false;

		// buggy plugins workaround not necessary for take FX, according to
		// https://github.com/reaper-oss/sws/blob/b3ad441575f075d9232872f7873a6af83bb9de61/SnM/SnM_FX.cpp#L299

		if (offline) {
			updt = (p.ParsePatch(SNM_SET_CHUNK_CHAR, 2, "TAKEFX", "BYPASS", takeFXId_inChunk, 2, (void*)"1") >0);
			if (updt) {
				// check for BYPASS 3rd value
				HWND hwnd = NULL;
				hwnd = TakeFX_GetFloatingWindow(takeIn, fxIn);

				if (hwnd) // FX is currently floating, set 3rd value
					p.ParsePatch(SNM_SET_CHUNK_CHAR, 2, "TAKEFX", "BYPASS", takeFXId_inChunk, 3, (void*)"1");
				else // not floating
					p.ParsePatch(SNM_SET_CHUNK_CHAR, 2, "TAKEFX", "BYPASS", takeFXId_inChunk, 3, (void*)"0");
			}
			Main_OnCommand(41204, 0); // fully unload unloaded VSTs
		}

		else { // set online
			updt = (p.ParsePatch(SNM_SET_CHUNK_CHAR, 2, "TAKEFX", "BYPASS", takeFXId_inChunk, 2, (void*)"0") > 0);

			if (updt) {
				bool committed = false;
				committed = p.Commit(false); // need to commit immediately, otherwise below making FX float wouldn't work 

				if (committed) {
					// check if it should float when onlining
					char state[2] = "0";
					if ((p.Parse(SNM_GET_CHUNK_CHAR, 2, "TAKEFX", "BYPASS", takeFXId_inChunk, 3, state) > 0)) {
						if (!strcmp(state, "1"))
							TakeFX_Show(takeIn, fxIn, 3); // show floating
					}
				}
			}

		}

	}
}

int FindTakeFXId_inChunk(MediaItem* parentItem, MediaItem_Take* takeIn, int fxIn)
{
	int takesCount = CountTakes(parentItem);
	int totalTakeFX = 0; int takeFXId_inChunk = -1;

	for (int i = 0; i < takesCount; i++) {
		MediaItem_Take* curTake = GetTake(parentItem, i);

		int FXcountCurTake = TakeFX_GetCount(curTake);

		if (curTake == takeIn) {
			takeFXId_inChunk = totalTakeFX + fxIn;
			break;
		}

		totalTakeFX += FXcountCurTake;
	}
	return takeFXId_inChunk;
}

const char * NF_GetSWSTrackNotes(MediaTrack* track, WDL_FastString* trackNoteOut)
{
	return NFDoGetSWSTrackNotes(track, trackNoteOut);
}

void NF_SetSWSTrackNotes(MediaTrack* track, const char* buf)
{
	NFDoSetSWSTrackNotes(track, buf);
}
// /*** nofish stuff ***


void DO_GetArrangeVertPos(int* MaxHeightOut, int* ViewPosOut) // dopp func 
{
	SCROLLINFO si = { sizeof(SCROLLINFO), };
	si.fMask = SIF_ALL;
	CoolSB_GetScrollInfo(GetArrangeWnd(), SB_VERT, &si);
	int areaHeight = si.nMax + 18;
	int areaPos = si.nPos;
	WritePtr(MaxHeightOut, areaHeight);
	WritePtr(ViewPosOut, areaPos);
}
