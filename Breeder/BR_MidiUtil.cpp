/******************************************************************************
/ BR_MidiUtil.cpp
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
#include "BR_MidiUtil.h"
#include "BR_MouseUtil.h"
#include "BR_Util.h"
#include "../SnM/SnM_Chunk.h"
#include "../reaper/localize.h"

/******************************************************************************
* BR_MidiEditor                                                               *
******************************************************************************/
BR_MidiEditor::BR_MidiEditor () :
m_take          (NULL),
m_midiEditor    (MIDIEditor_GetActive()),
m_startPos      (-1),
m_hZoom         (-1),
m_vPos          (-1),
m_vZoom         (-1),
m_noteshow      (-1),
m_timebase      (-1),
m_pianoroll     (-1),
m_drawChannel   (-1),
m_eventFilterCh (-1),
m_ppq           (-1),
m_lastLane      (-666),
m_valid         (false)
{
	m_valid        = this->Build();
	m_lastLane     = GetLastClickedVelLane(m_midiEditor);
	m_ccLanesCount = (int)m_ccLanes.size();
}

BR_MidiEditor::BR_MidiEditor (void* midiEditor) :
m_take          (NULL),
m_midiEditor    (midiEditor),
m_startPos      (-1),
m_hZoom         (-1),
m_vPos          (-1),
m_vZoom         (-1),
m_noteshow      (-1),
m_timebase      (-1),
m_pianoroll     (-1),
m_drawChannel   (-1),
m_eventFilterCh (-1),
m_ppq           (-1),
m_lastLane      (-666),
m_eventFilter   (false),
m_valid         (false)
{
	m_valid        = this->Build();
	m_lastLane     = GetLastClickedVelLane(m_midiEditor);
	m_ccLanesCount = (int)m_ccLanes.size();
}

BR_MidiEditor::BR_MidiEditor (MediaItem_Take* take) :
m_take           (take),
m_midiEditor     (NULL),
m_startPos       (-1),
m_hZoom          (-1),
m_vPos           (-1),
m_vZoom          (-1),
m_noteshow       (-1),
m_timebase       (-1),
m_pianoroll      (-1),
m_drawChannel    (-1),
m_eventFilterCh  (-1),
m_ppq            (-1),
m_lastLane       (-666),
m_eventFilter    (false),
m_valid          (false)
{
	m_valid        = this->Build();
	m_ccLanesCount = (int)m_ccLanes.size();
	if (m_valid)
	{
		m_timebase = 1;
		m_hZoom = GetHZoomLevel();
	}
}

MediaItem_Take* BR_MidiEditor::GetActiveTake ()
{
	return m_take;
}

double BR_MidiEditor::GetStartPos ()
{
	return m_startPos;
}

double BR_MidiEditor::GetHZoom ()
{
	return m_hZoom;
}

int BR_MidiEditor::GetPPQ ()
{
	return m_ppq;
}

int BR_MidiEditor::GetVPos ()
{
	return m_vPos;
}

int BR_MidiEditor::GetVZoom ()
{
	return m_vZoom;
}

int BR_MidiEditor::GetNoteshow ()
{
	return m_noteshow;
}

int BR_MidiEditor::GetTimebase ()
{
	return m_timebase;
}

int BR_MidiEditor::GetPianoRoll ()
{
	return m_pianoroll;
}

int BR_MidiEditor::GetDrawChannel ()
{
	return m_drawChannel;
}

int BR_MidiEditor::CountCCLanes ()
{
	return m_ccLanesCount;
}

int BR_MidiEditor::GetCCLane (int idx)
{
	if (idx >= 0 && idx < m_ccLanesCount)
		return m_ccLanes[idx];
	else
		return -1;
}

int BR_MidiEditor::GetCCLaneHeight (int idx)
{
	if (idx >= 0 && idx < m_ccLanesCount)
		return m_ccLanesHeight[idx];
	else
		return -1;
}

int BR_MidiEditor::GetLastClickedCCLane ()
{
	return m_lastLane;
}

int BR_MidiEditor::FindCCLane (int lane)
{
	int id = -1;
	for (size_t i = 0; i < m_ccLanes.size(); ++i)
	{
		if (m_ccLanes[i] == lane)
		{
			id = i;
			break;
		}
	}
	return id;
}

bool BR_MidiEditor::IsCCLaneVisible (int lane)
{
	for (size_t i = 0; i < m_ccLanes.size(); ++i)
	{
		if (m_ccLanes[i] == lane)
			return true;
	}
	return false;
}

bool BR_MidiEditor::IsChannelVisible (int channel)
{
	if (m_eventFilter)
	{
		if (m_eventFilterCh == 0)
			return true;
		else
			return !!GetBit(m_eventFilterCh, channel);
	}
	else
		return true;
}

void* BR_MidiEditor::GetEditor ()
{
	return m_midiEditor;
}

bool BR_MidiEditor::IsValid ()
{
	return m_valid;
}

bool BR_MidiEditor::Build ()
{
	m_take = (m_midiEditor) ? MIDIEditor_GetTake(m_midiEditor) : m_take;

	if (m_take)
	{
		MediaItem* item = GetMediaItemTake_Item(m_take);
		int takeId = GetTakeId(m_take, item);
		if (takeId >= 0)
		{
			SNM_TakeParserPatcher p(item, CountTakes(item));
			WDL_FastString takeChunk;
			if (p.GetTakeChunk(takeId, &takeChunk))
			{
				SNM_ChunkParserPatcher ptk(&takeChunk, false);
				LineParser lp(false);

				int laneId = 0;
				WDL_FastString lineLane;
				while (ptk.Parse(SNM_GET_SUBCHUNK_OR_LINE, 1, "SOURCE", "VELLANE", laneId++, -1, &lineLane))
				{
					lp.parse(lineLane.Get());
					m_ccLanes.push_back(lp.gettoken_int(1));
					m_ccLanesHeight.push_back(lp.gettoken_int((m_midiEditor) ? 2 : 3));
					lineLane.DeleteSub(0, lineLane.GetLength());
				}

				WDL_FastString dataLine;
				if (ptk.Parse(SNM_GET_SUBCHUNK_OR_LINE, 1, "SOURCE", "HASDATA", 0, -1, &dataLine))
				{
					lp.parse(dataLine.Get());
					m_ppq = lp.gettoken_int(2);
				}
				else if (ptk.Parse(SNM_GET_SUBCHUNK_OR_LINE, 1, "SOURCE", "FILE", 0, -1, &dataLine))
				{
					lp.parse(dataLine.Get());
					m_ppq = GetMIDIFilePPQ (lp.gettoken_str(1));
					if (!m_ppq)
						return false;
				}
				else
					return false;

				WDL_FastString lineView;
				if (ptk.Parse(SNM_GET_SUBCHUNK_OR_LINE, 1, "SOURCE", "CFGEDITVIEW", 0, -1, &lineView))
				{
					lp.parse(lineView.Get());
					m_startPos = lp.gettoken_float(1);
					m_hZoom    = lp.gettoken_float(2);
					m_vPos     = (m_midiEditor) ? lp.gettoken_int(3) : lp.gettoken_int(7);
					m_vZoom    = (m_midiEditor) ? lp.gettoken_int(4) : lp.gettoken_int(6);
				}
				else
					return false;

				WDL_FastString lineFilter;
				if (ptk.Parse(SNM_GET_SUBCHUNK_OR_LINE, 1, "SOURCE", "EVTFILTER", 0, -1, &lineFilter))
				{
					lp.parse(lineFilter.Get());
					m_eventFilterCh     = lp.gettoken_int(1);
					m_eventFilter       = !!GetBit(lp.gettoken_int(7), 0);

					if (!!GetBit(lp.gettoken_int(7), 2)) // invert filter if needed
						m_eventFilterCh = ~m_eventFilterCh;
				}
				else
					return false;

				WDL_FastString lineProp;
				if (ptk.Parse(SNM_GET_SUBCHUNK_OR_LINE, 1, "SOURCE", "CFGEDIT", 0, -1, &lineProp))
				{
					lp.parse(lineProp.Get());
					m_pianoroll    = lp.gettoken_int(6);
					m_drawChannel  = lp.gettoken_int(9) - 1;
					m_noteshow     = lp.gettoken_int(18);
					m_timebase     = lp.gettoken_int(19);
				}
				else
					return false;

				return true;
			}
		}
	}
	return false;
}

/******************************************************************************
* BR_MidiItemTimePos                                                          *
******************************************************************************/
BR_MidiItemTimePos::BR_MidiItemTimePos (MediaItem* item, bool deleteSavedEvents /*= true*/) :
item (item)
{
	position = GetMediaItemInfo_Value(item, "D_POSITION");
	length   = GetMediaItemInfo_Value(item, "D_LENGTH");
	timeBase = GetMediaItemInfo_Value(item, "C_BEATATTACHMODE");

	for (int i = 0; i < CountTakes(item); ++i)
	{
		MediaItem_Take* take = GetTake(item, i);

		int noteCount, ccCount, textCount;
		if (MIDI_CountEvts(take, &noteCount, &ccCount, &textCount))
		{
			savedMidiTakes.push_back(BR_MidiItemTimePos::MidiTake(take, noteCount, ccCount, textCount));
			BR_MidiItemTimePos::MidiTake* midiTake = &savedMidiTakes.back();

			for (int i = 0; i < noteCount; ++i)
			{
				midiTake->noteEvents.push_back(BR_MidiItemTimePos::MidiTake::NoteEvent(take, deleteSavedEvents ? 0 : i));
				if (deleteSavedEvents) MIDI_DeleteNote(take, 0);
			}
			for (int i = 0; i < ccCount; ++i)
			{
				midiTake->ccEvents.push_back(BR_MidiItemTimePos::MidiTake::CCEvent(take, deleteSavedEvents ? 0 : i));
				if (deleteSavedEvents) MIDI_DeleteCC(take, 0);
			}
			for (int i = 0; i < textCount; ++i)
			{
				midiTake->sysEvents.push_back(BR_MidiItemTimePos::MidiTake::SysEvent(take, deleteSavedEvents ? 0 : i));
				if (deleteSavedEvents) MIDI_DeleteTextSysexEvt(take, 0);
			}
		}
	}
}

void BR_MidiItemTimePos::Restore (bool clearCurrentEvents /*=true*/, double offset /*=0*/)
{
	SetMediaItemInfo_Value(item, "C_BEATATTACHMODE", 0);
	for (size_t i = 0; i < savedMidiTakes.size(); ++i)
	{
		BR_MidiItemTimePos::MidiTake* midiTake = &savedMidiTakes[i];
		MediaItem_Take* take = midiTake->take;

		if (clearCurrentEvents)
		{
			int noteCount, ccCount, textCount;
			if (MIDI_CountEvts(take, &noteCount, &ccCount, &textCount))
			{
				for (int i = 0; i < noteCount; ++i) MIDI_DeleteNote(take, 0);
				for (int i = 0; i < ccCount; ++i)   MIDI_DeleteCC(take, 0);
				for (int i = 0; i < textCount; ++i) MIDI_DeleteTextSysexEvt(take, 0);
			}
		}

		for (size_t i = 0; i < midiTake->noteEvents.size(); ++i)
			midiTake->noteEvents[i].InsertEvent(take, offset);

		for (size_t i = 0; i < midiTake->ccEvents.size(); ++i)
			midiTake->ccEvents[i].InsertEvent(take, offset);

		for (size_t i = 0; i < midiTake->sysEvents.size(); ++i)
			midiTake->sysEvents[i].InsertEvent(take, offset);
	}

	SetMediaItemInfo_Value(item, "D_POSITION", position);
	SetMediaItemInfo_Value(item, "D_LENGTH", length);
	SetMediaItemInfo_Value(item, "C_BEATATTACHMODE", timeBase);
}

BR_MidiItemTimePos::MidiTake::NoteEvent::NoteEvent (MediaItem_Take* take, int id)
{
	MIDI_GetNote(take, id, &selected, &muted, &pos, &end, &chan, &pitch, &vel);
	pos = MIDI_GetProjTimeFromPPQPos(take, pos);
	end = MIDI_GetProjTimeFromPPQPos(take, end);
}

void BR_MidiItemTimePos::MidiTake::NoteEvent::InsertEvent (MediaItem_Take* take, double offset)
{
	double posPPQ = MIDI_GetPPQPosFromProjTime(take, pos + offset);
	double endPPQ = MIDI_GetPPQPosFromProjTime(take, end + offset);
	MIDI_InsertNote(take, selected, muted, posPPQ, endPPQ, chan, pitch, vel);
}

BR_MidiItemTimePos::MidiTake::CCEvent::CCEvent (MediaItem_Take* take, int id)
{
	MIDI_GetCC(take, id, &selected, &muted, &pos, &chanMsg, &chan, &msg2, &msg3);
	pos = MIDI_GetProjTimeFromPPQPos(take, pos);
}

void BR_MidiItemTimePos::MidiTake::CCEvent::InsertEvent (MediaItem_Take* take, double offset)
{
	double posPPQ = MIDI_GetPPQPosFromProjTime(take, pos + offset);
	MIDI_InsertCC(take, selected, muted, posPPQ, chanMsg, chan, msg2, msg3);
}

BR_MidiItemTimePos::MidiTake::SysEvent::SysEvent (MediaItem_Take* take, int id)
{
	msg.Resize(32768);
	msg_sz = msg.GetSize();
	MIDI_GetTextSysexEvt(take, id, &selected, &muted, &pos, &type, msg.Get(), &msg_sz);
	pos = MIDI_GetProjTimeFromPPQPos(take, pos);
}

void BR_MidiItemTimePos::MidiTake::SysEvent::InsertEvent (MediaItem_Take* take, double offset)
{
	double posPPQ = MIDI_GetPPQPosFromProjTime(take, pos + offset);
	MIDI_InsertTextSysexEvt(take, selected, muted, posPPQ, type, msg.Get(), msg_sz);
}

BR_MidiItemTimePos::MidiTake::MidiTake (MediaItem_Take* take, int noteCount /*=0*/, int ccCount /*=0*/, int sysCount /*=0*/) :
take (take)
{
	noteEvents.reserve(noteCount);
	ccEvents.reserve(ccCount),
	sysEvents.reserve(sysCount);
}

/******************************************************************************
* Mouse cursor                                                                *
******************************************************************************/
double ME_PositionAtMouseCursor (bool checkRuler, bool checkCCLanes)
{
	BR_MouseInfo mouseInfo(BR_MouseInfo::MODE_MIDI_EDITOR_ALL);
	if (mouseInfo.GetMidiEditor())
	{
		if (checkRuler && checkCCLanes)                                 return mouseInfo.GetPosition(); // no need for strcmp, if position is invalid it's already -1
		if (!strcmp(mouseInfo.GetSegment(),"notes_view"))               return mouseInfo.GetPosition();
		if (checkRuler   && !strcmp(mouseInfo.GetSegment(), "ruler"))	return mouseInfo.GetPosition();
		if (checkCCLanes && !strcmp(mouseInfo.GetSegment(), "cc_lane")) return mouseInfo.GetPosition();
	}
	return -1;
}

/******************************************************************************
* Miscellaneous                                                               *
******************************************************************************/
vector<int> GetUsedNamedNotes (void* midiEditor, MediaItem_Take* take, bool used, bool named, int channelForNames)
{
	/* Not really reliable, user could have changed default draw channel  *
	*  but without resetting note view settings, view won't get updated   */

	vector<int> allNotesStatus(127, 0);
	MediaItem_Take* midiTake = (midiEditor) ? MIDIEditor_GetTake(midiEditor) : take;

	if (named)
	{
		MediaTrack* track = GetMediaItemTake_Track(midiTake);
		for (int i = 0; i < 128; ++i)
			if (GetTrackMIDINoteNameEx(NULL, track, i, channelForNames))
				allNotesStatus[i] = 1;
	}

	if (used)
	{
		int noteCount;
		if (MIDI_CountEvts(midiTake, &noteCount, NULL, NULL))
		{
			for (int i = 0; i < noteCount; ++i)
			{
				int pitch;
				MIDI_GetNote(midiTake, i, NULL, NULL, NULL, NULL, NULL, &pitch, NULL);
				allNotesStatus[pitch] = 1;
			}
		}
	}

	vector<int> notes;
	notes.reserve(128);
	for (int i = 0; i < 128; ++i)
	{
		if (allNotesStatus[i] == 1)
			notes.push_back(i);
	}

	return notes;
}

vector<int> GetSelectedNotes (MediaItem_Take* take)
{
	vector<int> selectedNotes;

	int noteCount;
	MIDI_CountEvts(take, &noteCount, NULL, NULL);
	for (int i = 0; i < noteCount; ++i)
	{
		bool selected = false;
		MIDI_GetNote(take, i, &selected, NULL, NULL, NULL, NULL, NULL, NULL);
		if (selected)
			selectedNotes.push_back(i);
	}
	return selectedNotes;
}

vector<int> MuteSelectedNotes (MediaItem_Take* take)
{
	vector<int> muteStatus;

	int noteCount;
	if (MIDI_CountEvts(take, &noteCount, NULL, NULL))
	{
		for (int i = 0; i < noteCount; ++i)
		{
			bool selected, muted;
			MIDI_GetNote(take, i, &selected, &muted, NULL, NULL, NULL, NULL, NULL);
			muteStatus.push_back((int)muted);
			if (!selected)
			{
				muted = true;
				MIDI_SetNote(take, i, NULL, &muted, NULL, NULL, NULL, NULL, NULL);
			}
		}
	}
	return muteStatus;
}

set<int> GetUsedCCLanes (void* midiEditor, int detect14bit)
{
	MediaItem_Take* take = MIDIEditor_GetTake(midiEditor);
	set<int> usedCC;

	int noteCount, ccCount, sysCount;
	if (take && MIDI_CountEvts(take, &noteCount, &ccCount, &sysCount))
	{
		BR_MidiEditor editor(midiEditor);

		set<int> unpairedMSB;
		for (int id = 0; id < ccCount; ++id)
		{
			int chanMsg, chan, msg2;
			MIDI_GetCC(take, id, NULL, NULL, NULL, &chanMsg, &chan, &msg2, NULL);

			if (!editor.IsChannelVisible(chan))
				continue;

			if      (chanMsg == 0xC0) usedCC.insert(CC_PROGRAM);
			else if (chanMsg == 0xD0) usedCC.insert(CC_CHANNEL_PRESSURE);
			else if (chanMsg == 0xE0) usedCC.insert(CC_PITCH);
			else if (chanMsg == 0xB0)
			{
				if (msg2 > 63 || detect14bit == 0)
					usedCC.insert(msg2);
				else
				{
					if (detect14bit == 1)
						usedCC.insert(msg2);

					// If MSB also check for LSB that makes up 14 bit event
					if (msg2 <= 31)
					{
						int tmpId = id;
						if (MIDI_GetCC(take, tmpId + 1, NULL, NULL, NULL, NULL, NULL, NULL, NULL))
						{
							double pos;
							MIDI_GetCC(take, id, NULL, NULL, &pos, NULL, NULL, NULL, NULL);

							double nextPos;
							int nextChanMsg, nextChan, nextMsg2;
							while (MIDI_GetCC(take, ++tmpId, NULL, NULL, &nextPos, &nextChanMsg, &nextChan, &nextMsg2, NULL))
							{
								if (nextPos > pos)
								{
									if (detect14bit == 2)
									{
										usedCC.insert(msg2);
										unpairedMSB.insert(msg2);
									}
									break;
								}

								if (nextChanMsg == 0xB0 && msg2 == nextMsg2 - 32 && chan == nextChan)
								{
									usedCC.insert(msg2 + CC_14BIT_START);
									break;
								}
							}
						}
						else
						{
							if (detect14bit == 2)
							{
								usedCC.insert(msg2);
								unpairedMSB.insert(msg2);
							}
						}
					}
					// If LSB, just make sure it was paired
					else if (detect14bit == 2)
					{
						int tmpId = id;
						if (MIDI_GetCC(take, tmpId - 1, NULL, NULL, NULL, NULL, NULL, NULL, NULL))
						{
							double pos;
							MIDI_GetCC(take, id, NULL, NULL, &pos, NULL, NULL, NULL, NULL);

							double prevPos;
							int prevChanMsg, prevChan, prevMsg2;
							while (MIDI_GetCC(take, --tmpId, NULL, NULL, &prevPos, &prevChanMsg, &prevChan, &prevMsg2, NULL))
							{
								if (prevPos < pos)
								{
									if (detect14bit == 2)
										usedCC.insert(msg2);
									break;
								}
								if (prevChanMsg == 0xB0 && msg2 == prevMsg2 + 32 && chan == prevChan)
									break;
							}
						}
						else
							usedCC.insert(msg2);
					}
				}
			}
		}
		if (detect14bit == 2)
		{
			for (set<int>::iterator it = unpairedMSB.begin(); it != unpairedMSB.end(); ++it)
			{
				if (usedCC.find(*it + CC_14BIT_START) != usedCC.end())
				{
					usedCC.erase(*it + CC_14BIT_START);
					usedCC.insert(*it + 32);             // MSB is already there, LSB doesn't have to be so add it
				}
			}
		}

		for (int i = 0; i < noteCount; ++i)
		{
			int chan;
			MIDI_GetNote(take, i, NULL, NULL, NULL, NULL, &chan, NULL, NULL);
			if (editor.IsChannelVisible(chan))
			{
				usedCC.insert(-1);
				break;
			}
		}

		bool foundText = false, foundSys = false;
		for (int i = 0; i < sysCount; ++i)
		{
			int type = 0;
			MIDI_GetTextSysexEvt(take, i, NULL, NULL, NULL, &type, NULL, NULL);

			if (type == -1)
			{
				if (!foundSys)
				{
					usedCC.insert(CC_SYSEX);
					foundSys = true;
					if (foundText)
						break;
				}
			}
			else
			{
				if (!foundText)
				{
					usedCC.insert(CC_TEXT_EVENTS);
					foundText = true;
					if (foundSys)
						break;
				}
			}
		}
	}

	return usedCC;
}

double EffectiveMidiTakeLength (MediaItem_Take* take, bool ignoreMutedEvents, bool ignoreTextEvents)
{
	int noteCount, ccCount, sysCount;
	if (take && MIDI_CountEvts(take, &noteCount, &ccCount, &sysCount))
	{
		double takeStartTime = GetMediaItemInfo_Value(GetMediaItemTake_Item(take), "D_POSITION");
		double takeStartPPQ  = MIDI_GetPPQPosFromProjTime(take, takeStartTime);
		double takeEndPPQ    = MIDI_GetPPQPosFromProjTime(take, takeStartTime + GetMediaItemInfo_Value(GetMediaItemTake_Item(take), "D_LENGTH"));
		double takeLenPPQ    = takeEndPPQ - takeStartPPQ;
		double sourceLenPPQ  = GetSourceLengthPPQ(take);

		int loopCount = (int)(takeLenPPQ/sourceLenPPQ) + (!fmod(takeLenPPQ, sourceLenPPQ) ? (-1) : (0));
		double effectiveTakeEndPPQ = takeStartPPQ;
		for (int i = 0; i < noteCount; ++i)
		{
			bool muted; double noteEnd;
			MIDI_GetNote(take, i, NULL, &muted, NULL, &noteEnd, NULL, NULL, NULL);
			if (((ignoreMutedEvents && !muted) || !ignoreMutedEvents))
			{
				noteEnd += loopCount*sourceLenPPQ;
				if (noteEnd > takeEndPPQ)
					noteEnd -= sourceLenPPQ;

				if (noteEnd > effectiveTakeEndPPQ)
					effectiveTakeEndPPQ = noteEnd;
			}
		}

		for (int i = ccCount-1 ; i >= 0; --i)
		{
			bool muted; double pos;
			MIDI_GetCC(take, i, NULL, &muted, &pos, NULL, NULL, NULL, NULL);
			if ((ignoreMutedEvents && !muted) || !ignoreMutedEvents)
			{
				pos += loopCount*sourceLenPPQ;
				if (pos > takeEndPPQ) pos -= sourceLenPPQ;

				if (pos > effectiveTakeEndPPQ)
					effectiveTakeEndPPQ = pos + 1; // add 1 ppq so length definitely includes that last event
				break;
			}
		}

		for (int i = 0; i < sysCount; ++i)
		{
			bool muted; double pos; int type;
			MIDI_GetTextSysexEvt(take, i, NULL, &muted, &pos, &type, NULL, NULL);
			if (((ignoreMutedEvents && !muted) || !ignoreMutedEvents) && ((ignoreTextEvents && type == -1) || !ignoreTextEvents))
			{
				pos += loopCount*sourceLenPPQ;
				if (pos > takeEndPPQ) pos -= sourceLenPPQ;

				if (pos > effectiveTakeEndPPQ)
					effectiveTakeEndPPQ = pos + 1; // add 1 ppq so length definitely includes that last event
			}
		}

		return MIDI_GetProjTimeFromPPQPos(take, effectiveTakeEndPPQ) - takeStartTime;
	}
	return 0;
}

double EffectiveMidiTakeStart (MediaItem_Take* take, bool ignoreMutedEvents, bool ignoreTextEvents)
{
	int noteCount, ccCount, sysCount;
	if (take && MIDI_CountEvts(take, &noteCount, &ccCount, &sysCount))
	{
		bool validNote = false, validCC = false, validSys = false;
		double noteStart, ccStart, sysStart;

		for (int i = 0; i < noteCount; ++i)
		{
			bool muted; double start;
			MIDI_GetNote(take, i, NULL, &muted, &start, NULL, NULL, NULL, NULL);
			if ((ignoreMutedEvents && !muted) || !ignoreMutedEvents)
			{
				noteStart = start;
				validNote = true;
				break;
			}
		}

		for (int i = 0; i < ccCount; ++i)
		{
			bool muted; double pos;
			MIDI_GetCC(take, i, NULL, &muted, &pos, NULL, NULL, NULL, NULL);
			if ((ignoreMutedEvents && !muted) || !ignoreMutedEvents)
			{
				ccStart = pos;
				validCC = true;
				break;
			}
		}

		for (int i = 0; i < sysCount; ++i)
		{
			bool muted; double pos; int type;
			MIDI_GetTextSysexEvt(take, i, NULL, &muted, &pos, &type, NULL, NULL);
			if (((ignoreMutedEvents && !muted) || !ignoreMutedEvents) && ((ignoreTextEvents && type == -1) || !ignoreTextEvents))
			{
				sysStart = pos;
				validSys = true;
				break;
			}
		}

		if (validNote || validCC || validSys)
		{
			if (!validNote) noteStart = (validSys) ? sysStart : ccStart;
			if (!validCC)   ccStart   = (validSys) ? sysStart : noteStart;
			if (!validSys)  sysStart  = (ccStart)  ? ccStart : noteStart;

			return MIDI_GetProjTimeFromPPQPos(take, min(min(noteStart, ccStart), sysStart));
		}
	}
	return GetMediaItemInfo_Value(GetMediaItemTake_Item(take), "D_POSITION");
}

void SetMutedNotes (MediaItem_Take* take, const vector<int>& muteStatus)
{
	int noteCount = muteStatus.size();
	for (int i = 0; i < noteCount; ++i)
	{
		bool muted = !!muteStatus[i];
		MIDI_SetNote(take, i, NULL, &muted, NULL, NULL, NULL, NULL, NULL);
	}
}

void SetSelectedNotes (MediaItem_Take* take, const vector<int>& selectedNotes, bool unselectOthers)
{
	int selectedCount = selectedNotes.size();
	int selectedId = (selectedCount > 0) ? (0) : (1);

	int noteCount;
	MIDI_CountEvts(take, &noteCount, NULL, NULL);
	for (int i = 0; i < noteCount; ++i)
	{
		bool selected = false;
		if (selectedId < selectedCount && i == selectedNotes[selectedId])
		{
			selected = true;
			++selectedId;
		}

		MIDI_SetNote(take, i, ((unselectOthers) ? (&selected) : ((&selected) ? &selected : NULL)), NULL, NULL, NULL, NULL, NULL, NULL);
	}
}

void UnselectAllEvents (MediaItem_Take* take, int lane)
{
	if (take)
	{
		if ((lane >= 0 && lane <= 127))
		{
			int id = -1;
			while ((id = MIDI_EnumSelCC(take, id)) != -1)
			{
				int cc, chanMsg;
				if (MIDI_GetCC(take, id, NULL, NULL, NULL, &chanMsg, NULL, &cc, NULL) && chanMsg == 0xB0 && cc == lane)
					MIDI_SetCC(take, id, &g_bFalse, NULL, NULL, NULL, NULL, NULL, NULL);
			}
		}
		else if (lane == CC_PROGRAM || lane == CC_CHANNEL_PRESSURE || lane == CC_PITCH || lane == CC_BANK_SELECT)
		{
			if (lane == CC_BANK_SELECT) lane = CC_PROGRAM;

			int type = (lane == CC_PROGRAM) ? (0xC0) : (lane == CC_CHANNEL_PRESSURE ? 0xD0 : 0xE0);
			int id = -1;
			while ((id = MIDI_EnumSelCC(take, id)) != -1)
			{
				int chanMsg;
				if (MIDI_GetCC(take, id, NULL, NULL, NULL, &chanMsg, NULL, NULL, NULL) && chanMsg == type)
					MIDI_SetCC(take, id, &g_bFalse, NULL, NULL, NULL, NULL, NULL, NULL);
			}
		}
		else if (lane == CC_VELOCITY)
		{
			int id = -1;
			while ((id = MIDI_EnumSelNotes(take, id)) != -1)
				MIDI_SetNote(take, id, &g_bFalse, NULL, NULL, NULL, NULL, NULL, NULL);
		}
		else if (lane == CC_TEXT_EVENTS || lane == CC_SYSEX)
		{
			int id = -1;
			while ((id = MIDI_EnumSelTextSysexEvts(take, id)) != -1)
			{
				int type = 0;
				if (MIDI_GetTextSysexEvt(take, id, NULL, NULL, NULL, &type, NULL, NULL) && ((lane == CC_SYSEX && type == -1) || (lane == CC_TEXT_EVENTS && type != -1)))
					MIDI_SetTextSysexEvt(take, id, &g_bFalse, NULL, NULL, NULL, NULL, NULL);
			}
		}
		else if (lane >= CC_14BIT_START)
		{
			int id = -1;

			int cc1 = lane - CC_14BIT_START;
			int cc2 = cc1 + 32;
			while ((id = MIDI_EnumSelCC(take, id)) != -1)
			{
				int cc, chanMsg;
				if (MIDI_GetCC(take, id, NULL, NULL, NULL, &chanMsg, NULL, &cc, NULL) && chanMsg == 0xB0 && (cc == cc1 || cc == cc2))
					MIDI_SetCC(take, id, &g_bFalse, NULL, NULL, NULL, NULL, NULL, NULL);
			}
		}
	}
}

bool AreAllNotesUnselected (MediaItem_Take* take)
{
	bool firstNote = false;
	MIDI_GetNote(take, 0, &firstNote, NULL, NULL, NULL, NULL, NULL, NULL);

	if (!firstNote && MIDI_EnumSelNotes(take, 0) == -1)
		return true;
	else
		return false;
}

bool IsMidi (MediaItem_Take* take, bool* inProject /*= NULL*/)
{
	if (PCM_source* source = GetMediaItemTake_Source(take))
	{
		const char* type = source->GetType();
		if (!strcmp(type, "MIDI") || !strcmp(type, "MIDIPOOL"))
		{
			if (inProject)
			{
				const char* fileName = source->GetFileName();
				if (fileName && !strcmp(fileName, ""))
					*inProject = true;
				else
					*inProject = false;
			}
			return true;
		}
	}
	WritePtr(inProject, false);
	return false;
}

bool IsOpenInInlineEditor (MediaItem_Take* take)
{
	bool inProject = false;
	if (GetActiveTake(GetMediaItemTake_Item(take)) == take && IsMidi(take, &inProject) && inProject)
	{
		if (PCM_source* source = GetMediaItemTake_Source(take))
		{
			if (source->Extended(PCM_SOURCE_EXT_INLINEEDITOR, 0, 0, 0) > 0)
				return true;
		}
	}
	return false;
}

bool IsMidiNoteBlack (int note)
{
	note = 1 << (note % 12);     // note position in an octave (first bit is C, second bit is C# etc...)
	return (note & 0x054A) != 0; // 54A = all black notes bits set to 1
}

bool IsVelLaneValid (int lane)
{
	if (lane >= -1 && lane <= 165)
		return true;
	else
		return false;
}

int GetMIDIFilePPQ (const char* fp)
{
	WDL_FileRead file(fp);

	char line[14] = "";
	if (file.Read(line, sizeof(line)) && !strncmp(line, "MThd\0\0\0\x6", 8))
	{
		int byte1 = (int)((unsigned char)line[12]);
		int byte2 = (int)((unsigned char)line[13]);
		return byte1 << 8 | byte2;
	}
	else
		return 0;
}

int GetLastClickedVelLane (void* midiEditor)
{
	int cc = MIDIEditor_GetSetting_int(midiEditor, "last_clicked_cc_lane");
	if (cc == -1)
		return -666;
	else
		return MapReaScriptCCToVelLane(cc);
}

int MapVelLaneToReaScriptCC (int lane)
{
	if (lane == -1)	                return 0x200;
	if (lane >= 0   && lane <= 127) return lane;
	if (lane >= 128 && lane <= 133)	return 0x200 | (lane+1 & 0x7F);
	if (lane >= 134 && lane <= 165) return 0x100 | (lane - 134);
	else                            return -1;
}

int MapReaScriptCCToVelLane (int cc)
{
	if (cc == 0x200)                return -1;
	if (cc >= 0     && cc <= 127)   return cc;
	if (cc >= 0x201 && cc <= 0x206) return cc - 385;
	if (cc >= 0x100 && cc <= 0x11F) return cc - 122;
	else                            return -2;
}
