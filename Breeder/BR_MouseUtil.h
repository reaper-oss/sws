/******************************************************************************
/ BR_MouseUtil.h
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
#pragma once

class BR_Envelope;

/******************************************************************************
* Miscellaneous                                                               *
******************************************************************************/
double PositionAtMouseCursor (bool checkRuler, bool checkCursorVisibility = true, int* yOffset = NULL, bool* overRuler = NULL);
MediaItem* ItemAtMouseCursor (double* position);
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
	MediaTrack*     GetTrack ();
	MediaItem*      GetItem ();
	MediaItem_Take* GetTake();
	TrackEnvelope*  GetEnvelope ();
	int GetTakeId ();
	int GetEnvelopePoint ();
	int GetStretchMarker ();
	bool IsTakeEnvelope ();

	// MIDI editor
	void* GetMidiEditor ();
	bool  IsInlineMidi ();
	bool  GetCCLane (int* ccLane, int* ccLaneVal, int* ccLaneId); // returns false if mouse is not over CC lane
	int   GetNoteRow ();                                          // returns -1 if mouse is not over any note row
	int   GetPianoRollMode ();                                    // returns 0->normal, 1->named notes, -1->unknown
	bool  SetDetectedCCLaneAsLastClicked ();                      // hacky! works only if arrange state/MIDI editor didn't change after the last update ( switches focus to MIDI editor/arrange briefly...)

	// Both main window and MIDI editor
	double GetPosition ();  // time position in arrange or MIDI ruler (returns -1 if not applicable)

	// Use these in constructor and Update() to optimize things if possible
	enum Modes
	{
		MODE_ALL                 = 0x1,
		MODE_RULER               = 0x2,
		MODE_TRANSPORT           = 0x4,
		MODE_MCP_TCP             = 0x8,
		MODE_ARRANGE             = 0x10,
		MODE_ENV_LANE_DO_SEGMENT = 0x20, // valid only in tandem with MODE_ARRANGE. Set it to look for envelope points/segments in envelope lane (track lane gets checked always)
		MODE_MIDI_EDITOR         = 0x40,
		MODE_MIDI_INLINE         = 0x80,
		MODE_MIDI_EDITOR_ALL     = MODE_MIDI_EDITOR | MODE_MIDI_INLINE,
		MODE_ARRANGE_ALL         = MODE_ARRANGE | MODE_ENV_LANE_DO_SEGMENT
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
		void* midiEditor;
		bool takeEnvelope, inlineMidi;
		double position;
		int takeId, envPointId, stretchMarkerId, noteRow, ccLaneVal, ccLaneId, ccLane, pianoRollMode;
		MouseInfo ();
	};

	void GetContext (const POINT& p);
	bool GetContextMIDI (POINT p, HWND hwnd, BR_MouseInfo::MouseInfo& mouseInfo);
	bool GetContextMIDIInline (BR_MouseInfo::MouseInfo& mouseInfo, int mouseDisplayX, int mouseY, int takeHeight, int takeOffset);
	int IsMouseOverStretchMarker (MediaItem* item, MediaItem_Take* take, int takeHeight, int takeOffset, int mouseDisplayX, int mouseY, double mousePos, double arrangeStart, double arrangeZoom);
	int IsMouseOverEnvelopeLine (BR_Envelope& envelope, int drawableEnvHeight, int yOffset, int mouseDisplayX, int mouseY, double mousePos, double arrangeStart, double arrangeZoom, int* pointUnderMouse);
	int IsMouseOverEnvelopeLineTrackLane (MediaTrack* track, int trackHeight, int trackOffset, list<TrackEnvelope*>& laneEnvs, int mouseDisplayX, int mouseY, double mousePos, double arrangeStart, double arrangeZoom, TrackEnvelope** trackEnvelope, int* pointUnderMouse);
	int IsMouseOverEnvelopeLineTake (MediaItem_Take* take, int takeHeight, int takeOffset, int mouseDisplayX, int mouseY, double mousePos, double arrangeStart, double arrangeZoom, TrackEnvelope** trackEnvelope, int* pointUnderMouse);
	int GetRulerLaneHeight (int rulerH, int lane);
	int IsHwndMidiEditor (HWND hwnd, void** midiEditor, HWND* subView);
	static bool SortEnvHeightsById (const pair<int,int>& left, const pair<int,int>& right);
	void GetTrackOrEnvelopeFromY (int y, TrackEnvelope** _envelope, MediaTrack** _track, list<TrackEnvelope*>* envelopes, int* height, int* offset);

	BR_MouseInfo::MouseInfo m_mouseInfo;
	POINT m_ccLaneClickPoint;
	HWND  m_ccLaneClickPointHwnd;
	int m_mode;
};