/******************************************************************************
/ BR_MouseUtil.h
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

class BR_Envelope;

/******************************************************************************
* Miscellaneous                                                               *
******************************************************************************/
double PositionAtMouseCursor (bool checkRuler, bool checkCursorVisibility = true, int* yOffset = NULL, bool* overRuler = NULL);
MediaItem* ItemAtMouseCursor (double* position, MediaItem_Take** takeAtMouse = NULL);
MediaItem_Take* TakeAtMouseCursor (double* position);
MediaTrack* TrackAtMouseCursor (int* context, double* position); // context: 0->TCP, 1->MCP, 2->Arrange

/******************************************************************************
* BR_MouseInfo                                                                *
*                                                                             *
* Create the object and use it's methods to query information about stuff     *
* under mouse cursor. Note that the object doesn't recheck state of things    *
* automatically - you need to call Update() manually when needed. If there's  *
* no need to check for things right at the object creation (i.e. global       *
* variable) pass updateNow as false. While all other methods should be        *
* self-explanatory, for information about GetWindow(), GetSegment() and       *
* GetDetails() see the following table:                                       *
*                                                                             *
*   |********************************************************************|    *
*   | window       | segment       | details                             |    *
*   |______________|_______________|_____________________________________|    *
*   | unknown      | ""            | ""                                  |    *
*   |______________|_______________|_____________________________________|    *
*   | ruler        | region_lane   | ""                                  |    *
*   |              | marker_lane   | ""                                  |    *
*   |              | tempo_lane    | ""                                  |    *
*   |              | timeline      | ""                                  |    *
*   |______________|_______________|_____________________________________|    *
*   | transport    | ""            | ""                                  |    *
*   |______________|_______________|_____________________________________|    *
*   | tcp          | track         | ""                                  |    *
*   |              | envelope      | ""                                  |    *
*   |              | empty         | ""                                  |    *
*   |______________|_______________|_____________________________________|    *
*   | mcp          | track         | ""                                  |    *
*   |              | empty         | ""                                  |    *
*   |______________|_______________|_____________________________________|    *
*   | arrange      | track         | empty,                              |    *
*   |              |               | item, item_stretch_marker,          |    *
*   |              |               | env_point, env_segment              |    *
*   |              | envelope      | empty, env_point, env_segment       |    *
*   |              | empty         | ""                                  |    *
*   |______________|_______________|_____________________________________|    *
*   | midi_editor  | unknown       | ""                                  |    *
*   |              | ruler         | ""                                  |    *
*   |              | piano         | ""                                  |    *
*   |              | notes         | ""                                  |    *
*   |              | cc_lane       | cc_selector, cc_lane                |    *
*   |********************************************************************|    *
*                                                                             *
* Note: due to API limitation, GetNoteRow() won't work when dealing with      *
* inline MIDI                                                                 *
******************************************************************************/
class BR_MouseInfo
{
public:
	BR_MouseInfo (int mode = BR_MouseInfo::MODE_ALL, bool updateNow = true);
	~BR_MouseInfo ();
	void Update (const POINT* p = NULL);
	void SetMode (int mode);

	// See description for BR_MouseInfo
	const char* GetWindow ();
	const char* GetSegment ();
	const char* GetDetails ();

	// Main window
	MediaTrack*     GetTrack (); // if mouse is over envelope, returns envelope parent
	MediaItem*      GetItem ();  // returns item even if mouse is over some other track element
	MediaItem_Take* GetTake();
	TrackEnvelope*  GetEnvelope ();
	int GetTakeId ();        // returns -1 if there is no take under mouse cursor
	int GetEnvelopePoint (); // returns -1 if there is no envelope point under mouse cursor
	int GetStretchMarker (); // returns -1 if there is no stretch marker under mouse cursor
	bool IsTakeEnvelope ();  // returns true if envelope under mouse cursor is take envelope

	// MIDI editor
	HWND  GetMidiEditor ();
	bool  IsInlineMidi ();
	bool  GetCCLane (int* ccLane, int* ccLaneVal, int* ccLaneId); // returns false if mouse is not over CC lane
	int   GetNoteRow ();                                          // returns -1 if mouse is not over any note row (sketchy when it comes to inline MIDI editor, see BR_MidiEditor)
	int   GetPianoRollMode ();                                    // returns 0->normal, 1->named notes, -1->unknown
	bool  SetDetectedCCLaneAsLastClicked ();                      // hacky! works only if MIDI editor/arrange state didn't change after last update (it also briefly switches focus to MIDI editor/arrange...)

	// Both main window and MIDI editor
	double GetPosition ();  // time position in arrange or MIDI ruler (returns -1 if not applicable)

	// Use these in constructor and SetMode() to optimize things if possible
	enum Modes
	{
		MODE_ALL             = 0x1,
		MODE_RULER           = 0x2,
		MODE_TRANSPORT       = 0x4,
		MODE_MCP_TCP         = 0x8,
		MODE_ARRANGE         = 0x10,
		MODE_MIDI_EDITOR     = 0x20,
		MODE_MIDI_INLINE     = 0x40,
		MODE_MIDI_EDITOR_ALL = MODE_MIDI_EDITOR | MODE_MIDI_INLINE,

		// More optimization for MODE_ARRANGE or MODE_ALL
		MODE_IGNORE_ALL_TRACK_LANE_ELEMENTS_BUT_ITEMS = 0x80,
		MODE_IGNORE_ENVELOPE_LANE_SEGMENT             = 0x100
	};

private:
	struct MouseInfo
	{
		const char* window;
		const char* segment;
		const char* details;
		MediaTrack* track;
		MediaItem* item;
		MediaItem_Take* take;
		TrackEnvelope* envelope;
		HWND midiEditor;
		bool takeEnvelope, inlineMidi;
		double position;
		int takeId, envPointId, stretchMarkerId, noteRow, ccLaneVal, ccLaneId, ccLane, pianoRollMode;
		MouseInfo ();
	};

	void GetContext (const POINT& p);
	bool GetContextMIDI (POINT p, HWND hwnd, BR_MouseInfo::MouseInfo& mouseInfo);
	bool GetContextMIDIInline (BR_MouseInfo::MouseInfo& mouseInfo, int mouseDisplayX, int mouseY, int takeHeight, int takeOffset);
	bool IsStretchMarkerVisible (MediaItem_Take* take, int id, double takePlayrate, double arrangeZoom);
	int IsMouseOverStretchMarker (MediaItem* item, MediaItem_Take* take, int takeHeight, int takeOffset, int mouseDisplayX, int mouseY, double mousePos, double arrangeStart, double arrangeZoom);
	int IsMouseOverEnvelopeLine (BR_Envelope& envelope, int drawableEnvHeight, int yOffset, int mouseDisplayX, int mouseY, double mousePos, double arrangeStart, double arrangeZoom, int* pointUnderMouse);
	int IsMouseOverEnvelopeLineTrackLane (MediaTrack* track, int trackHeight, int trackOffset, list<TrackEnvelope*>& laneEnvs, int mouseDisplayX, int mouseY, double mousePos, double arrangeStart, double arrangeZoom, TrackEnvelope** trackEnvelope, int* pointUnderMouse);
	int IsMouseOverEnvelopeLineTake (MediaItem_Take* take, int takeHeight, int takeOffset, int mouseDisplayX, int mouseY, double mousePos, double arrangeStart, double arrangeZoom, TrackEnvelope** trackEnvelope, int* pointUnderMouse);
	int GetRulerLaneHeight (int rulerH, int lane);
	int IsHwndMidiEditor (HWND hwnd, HWND* midiEditor, HWND* subView);
	static bool SortEnvHeightsById (const pair<int,int>& left, const pair<int,int>& right);
	void GetTrackOrEnvelopeFromY (int y, TrackEnvelope** _envelope, MediaTrack** _track, list<TrackEnvelope*>* envelopes, int* height, int* offset);

	BR_MouseInfo::MouseInfo m_mouseInfo;
	POINT m_ccLaneClickPoint;
	HWND  m_midiEditorPianoWnd;
	int m_mode;
};
