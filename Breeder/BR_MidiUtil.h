/******************************************************************************
/ BR_MidiUtil.h
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
#pragma once

/******************************************************************************
* MIDI timebase - this is how Reaper stores timebase settings internally      *
******************************************************************************/
enum BR_MidiTimebase
{
	PROJECT_BEATS = 0,
	PROJECT_SYNC  = 1,
	PROJECT_TIME  = 2,
	SOURCE_BEATS  = 4
};

/******************************************************************************
* MIDI noteshow - this is how Reaper stores noteshow settings internally      *
******************************************************************************/
enum BR_MidiNoteshow
{
	SHOW_ALL_NOTES             = 0,
	HIDE_UNUSED_NOTES          = 1,
	HIDE_UNUSED_UNNAMED_NOTES  = 2,
	CUSTOM_NOTES_VIEW          = 3,
};

/******************************************************************************
* MIDI CC lanes - this is how Reaper stores CC lanes internally               *
******************************************************************************/
enum BR_MidiVelLanes
{
	CC_VELOCITY         = -1,
	CC_VELOCITY_OFF     = 167,
	CC_PITCH            = 128,
	CC_PROGRAM          = 129,
	CC_CHANNEL_PRESSURE = 130,
	CC_BANK_SELECT      = 131,
	CC_TEXT_EVENTS      = 132,
	CC_SYSEX            = 133,
	CC_14BIT_START      = 134,
	CC_NOTATION_EVENTS  = 166
};

/******************************************************************************
* MIDI status bytes - purely for readability                                  *
******************************************************************************/
enum BR_MidiStatusBytes
{
	STATUS_NOTE_OFF         = 0x80,
	STATUS_NOTE_ON          = 0x90,
	STATUS_POLY_PRESSURE    = 0xA0,
	STATUS_CC               = 0xB0,
	STATUS_PROGRAM          = 0xC0,
	STATUS_CHANNEL_PRESSURE = 0xD0,
	STATUS_PITCH            = 0xE0,
	STATUS_SYS              = 0xF0
};

/******************************************************************************
* Various constants related to position and dimension of MIDI editor elements *
******************************************************************************/
const int MIDI_RULER_H                 = 64;
const int MIDI_LANE_DIVIDER_H          = 9;
const int MIDI_LANE_TOP_GAP            = 4;
const int MIDI_BLACK_KEYS_W            = 73;
const int MIDI_CC_LANE_CLICK_Y_OFFSET  = 5;
const int MIDI_CC_EVENT_WIDTH_PX       = 8;
const int MIDI_MIN_NOTE_VIEW_H         = 110;

const int INLINE_MIDI_MIN_H                  = 32;
const int INLINE_MIDI_MIN_NOTEVIEW_H         = 24;
const int INLINE_MIDI_LANE_DIVIDER_H         = 6;
const int INLINE_MIDI_KEYBOARD_W             = 12;
const int INLINE_MIDI_TOP_BAR_H              = 17;
const int INLINE_MIDI_CC_LANE_CLICK_Y_OFFSET = 2;

/******************************************************************************
* Class for managing normal or inline MIDI editor (read-only for now)         *
******************************************************************************/
class BR_MidiEditor
{
public:
	BR_MidiEditor ();                              // last active main MIDI editor
	explicit BR_MidiEditor (HWND midiEditor);     // main MIDI editor
	explicit BR_MidiEditor (MediaItem_Take* take); // inline MIDI editor

	/* Various MIDI editor view options */
	MediaItem_Take* GetActiveTake ();
	double GetStartPos ();            // can be ppq or time - depends on timebase
	double GetHZoom ();               // can be ppq or time - depends on timebase
	int GetPPQ ();
	int GetVPos ();                   // not really working for inline midi editor (can be 0 which means it's auto adjusted by take height, or is completely wrong if notes are hidden)
	int GetVZoom ();                  // not really working for inline midi editor (can be 0 which means it's auto adjusted by take height, or is completely wrong if notes are hidden)
	int GetNoteshow ();               // see BR_MidiNoteshow
	int GetTimebase ();               // see BR_MidiTimeBase
	int GetPianoRoll ();
	int GetDrawChannel ();

	/* CC lanes */
	int CountCCLanes ();
	int GetCCLane (int idx);
	int GetCCLaneHeight (int idx);
	int GetLastClickedCCLane ();
	int FindCCLane (int lane);
	int GetCCLanesFullheight (bool keyboardView);
	bool IsCCLaneVisible (int lane);
	bool SetCCLaneLastClicked (int idx); // works only for main MIDI editor

	/* Event filter */
	bool IsNoteVisible (MediaItem_Take* take, int id);
	bool IsCCVisible (MediaItem_Take* take, int id);
	bool IsSysVisible (MediaItem_Take* take, int id);
	bool IsChannelVisible (int channel);

	/* Misc */
	HWND GetEditor ();
	vector<int> GetUsedNamedNotes ();

	/* Check if MIDI editor data is valid - should call right after creating the object  */
	bool IsValid ();

private:
	bool Build ();
	bool CheckVisibility (MediaItem_Take* take, int chanMsg, double position, double end, int channel, int param, int value);

	MediaItem_Take* m_take;
	HWND m_midiEditor;
	double m_startPos, m_hZoom;
	int m_vPos, m_vZoom, m_noteshow, m_timebase, m_pianoroll, m_drawChannel, m_ccLanesCount, m_ppq, m_lastLane;
	int m_filterChannel, m_filterEventType, m_filterEventParamLo, m_filterEventParamHi, m_filterEventValLo, m_filterEventValHi;
	double m_filterEventPosRepeat, m_filterEventPosLo, m_filterEventPosHi, m_filterEventLenLo, m_filterEventLenHi;
	bool m_filterEnabled, m_filterInverted, m_filterEventParam, m_filterEventVal, m_filterEventPos, m_filterEventLen;
	bool m_valid;
	vector<int> m_ccLanes, m_ccLanesHeight, m_notesOrder;
};

/******************************************************************************
* Class for saving and restoring TIME positioning info of an item and         *
* all of it's MIDI events                                                     *
******************************************************************************/
class BR_MidiItemTimePos
{
public:
	explicit BR_MidiItemTimePos (MediaItem* item); // saves all MIDI events in the item
	void Restore (double timeOffset = 0); // deletes any MIDI events in the item and then restores saved events

private:
	struct MidiTake
	{
		struct NoteEvent
		{
			NoteEvent (MediaItem_Take* take, int id);
			void InsertEvent (MediaItem_Take* take, double offset);
			bool selected, muted;
			double pos, end;
			int chan, pitch, vel;
		};
		struct CCEvent
		{
			CCEvent (MediaItem_Take* take, int id);
			void InsertEvent (MediaItem_Take* take, double offset);
			bool selected, muted;
			double pos, beztension;
			int chanMsg, chan, msg2, msg3, shape;
		};
		struct SysEvent
		{
			SysEvent (MediaItem_Take* take, int id);
			void InsertEvent (MediaItem_Take* take, double offset);
			bool selected, muted;
			double pos;
			int type, msg_sz;
			WDL_TypedBuf<char> msg;
		};

		MidiTake (MediaItem_Take* take, int noteCount = 0, int ccCount = 0, int sysCount = 0);
		vector<NoteEvent> noteEvents;
		vector<CCEvent> ccEvents;
		vector<SysEvent> sysEvents;
		MediaItem_Take* take;
	};
	MediaItem* item;
	double position, length, timeBase;
	bool looped;
	double loopStart, loopEnd, loopedOffset;
	vector<BR_MidiItemTimePos::MidiTake> savedMidiTakes;
};

/******************************************************************************
* Mouse cursor                                                                *
******************************************************************************/
double ME_PositionAtMouseCursor (bool checkRuler, bool checkCCLanes);

/******************************************************************************
* Miscellaneous                                                               *
******************************************************************************/
vector<int> GetSelectedNotes (MediaItem_Take* take);
vector<int> MuteUnselectedNotes (MediaItem_Take* take); // returns previous mute state of all notes
set<int> GetUsedCCLanes (HWND midiEditor, int detect14bit, bool selectedEventsOnly); // detect14bit: 0-> don't detect 14-bit, 1->detect partial 14-bit (count both 14 bit lanes and their counterparts) 2->detect full 14-bit (detect only if all CCs that make it have exactly same time positions)
double EffectiveMidiTakeStart (MediaItem_Take* take, bool ignoreMutedEvents, bool ignoreTextEvents, bool ignoreEventsOutsideItemBoundaries);
double EffectiveMidiTakeEnd (MediaItem_Take* take, bool ignoreMutedEvents, bool ignoreTextEvents, bool ignoreEventsOutsideItemBoundaries);
double GetMidiSourceLengthPPQ (MediaItem_Take* take, bool accountPlayrateIfIgnoringProjTempo, bool* isMidiSource = NULL);
double GetOriginalPpqPos (MediaItem_Take* take, double ppqPos, bool* loopedItem, double* posVisInsertStartPpq, double* posVisInsertEndPpq); // insert start/end are used to check if event can be inserted at returned pos and be visible at the loop iteration where ppqPos is located
void SetMutedNotes (MediaItem_Take* take, const vector<int>& muteStatus);
void SetSelectedNotes (MediaItem_Take* take, const vector<int>& selectedNotes, bool unselectOthers);
void UnselectAllEvents (MediaItem_Take* take, int lane);
bool AreAllNotesUnselected (MediaItem_Take* take);
bool IsMidi (MediaItem_Take* take, bool* inProject = NULL);
bool IsOpenInInlineEditor (MediaItem_Take* take);
bool IsMidiNoteBlack (int note);
bool IsVelLaneValid (int lane);
bool DeleteEventsInLane (MediaItem_Take* take, int lane, bool selectedOnly, double startRangePpq, double endRangePpq, bool doRange);
int GetMidiTakeVisibleLoops (MediaItem_Take* take);
int FindFirstSelectedNote (MediaItem_Take* take, BR_MidiEditor* midiEditorFilterSettings); // Pass midiEditorFilterSettings
int FindFirstSelectedCC   (MediaItem_Take* take, BR_MidiEditor* midiEditorFilterSettings); // to check event which only pass
int FindFirstNote (MediaItem_Take* take, BR_MidiEditor* midiEditorFilterSettings);         // through MIDI filter
int GetMIDIFilePPQ (const char* fp);
int GetLastClickedVelLane (HWND midiEditor); // returns -2 if no last clicked lane
int GetMaxCCLanesFullHeight (HWND midiEditor);
int ConvertLaneToStatusMsg (int lane);
int MapVelLaneToReaScriptCC (int lane); // CC format follows ReaScript scheme: 0-127=CC, 0x100|(0-31)=14-bit CC, 0x200=velocity, 0x201=pitch,
int MapReaScriptCCToVelLane (int cc);   // 0x202=program, 0x203=channel pressure, 0x204=bank/program select, 0x205=text, 0x206=sysex, 0x207=off velocity
