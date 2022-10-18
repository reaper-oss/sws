/******************************************************************************
/ BR_ProjState.h
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

#include "BR_MidiUtil.h"

class BR_EnvSel;
class BR_CursorPos;
class BR_MidiNoteSel;
class BR_MidiCCEvents;
class BR_ItemMuteState;
class BR_TrackSoloMuteState;
class BR_MidiToggleCCLane;

/******************************************************************************
* Globals                                                                     *
******************************************************************************/
extern SWSProjConfig<WDL_PtrList_DOD<BR_EnvSel> >             g_envSel;
extern SWSProjConfig<WDL_PtrList_DOD<BR_CursorPos> >          g_cursorPos;
extern SWSProjConfig<WDL_PtrList_DOD<BR_MidiNoteSel> >        g_midiNoteSel;
extern SWSProjConfig<WDL_PtrList_DOD<BR_MidiCCEvents> >       g_midiCCEvents;
extern SWSProjConfig<WDL_PtrList_DOD<BR_ItemMuteState> >      g_itemMuteState;
extern SWSProjConfig<WDL_PtrList_DOD<BR_TrackSoloMuteState> > g_trackSoloMuteState;
extern SWSProjConfig<BR_MidiToggleCCLane>                     g_midiToggleHideCCLanes;

/******************************************************************************
* Project state init/exit                                                     *
******************************************************************************/
int ProjStateInitExit (bool init);

/******************************************************************************
* Envelope selection state                                                    *
******************************************************************************/
class BR_EnvSel
{
public:
	BR_EnvSel (int slot, TrackEnvelope* envelope);
	BR_EnvSel (int slot, ProjectStateContext* ctx);
	void SaveState (ProjectStateContext* ctx);
	void Save (TrackEnvelope* envelope);
	bool Restore (TrackEnvelope* envelope);
	int  GetSlot ();

private:
	int m_slot;
	vector<int> m_selection;
};

/******************************************************************************
* Cursor position state                                                       *
******************************************************************************/
class BR_CursorPos
{
public:
	explicit BR_CursorPos (int slot);
	BR_CursorPos (int slot, ProjectStateContext* ctx);
	void SaveState (ProjectStateContext* ctx);
	void Save ();
	bool Restore ();
	int  GetSlot ();

private:
	int m_slot;
	double m_position;
};

/******************************************************************************
* MIDI notes selection state                                                  *
******************************************************************************/
class BR_MidiNoteSel
{
public:
	BR_MidiNoteSel (int slot, MediaItem_Take* take);
	BR_MidiNoteSel (int slot, ProjectStateContext* ctx);
	void SaveState (ProjectStateContext* ctx);
	void Save (MediaItem_Take* take);
	bool Restore (MediaItem_Take* take);
	int  GetSlot ();

private:
	int m_slot;
	vector<int> m_selection;
};

/******************************************************************************
* MIDI saved CC events state                                                  *
******************************************************************************/
class BR_MidiCCEvents
{
public:
	BR_MidiCCEvents (int slot, BR_MidiEditor& midiEditor, int lane);
	BR_MidiCCEvents (int slot, ProjectStateContext* ctx);
	BR_MidiCCEvents (int slot, int lane); // just sets the source lane, use AddEvents to add events afterwards (in this case ppq of added events and take ppq to which they are restored needs to be the same)
	void SaveState (ProjectStateContext* ctx);
	bool Save (BR_MidiEditor& midiEditor, int lane);
	bool Restore (BR_MidiEditor& midiEditor, int lane, bool allVisible, double startPositionPppq, bool showWarningForInvalidLane = true, bool moveEditCursor = true);
	int  GetSlot ();
	int CountSavedEvents();
	double GetSourcePpqStart ();
	void AddEvent (double ppqPos, int msg2, int msg3, int channel, int shape = 0, double beztension = 0); // always add events in time order from first to last

private:
	struct Event
	{
		double positionPpq, beztension;
		int channel, msg2, msg3, shape;
		bool mute;
		Event ();
		Event (double positionPpq, int channel, int msg2, int msg3, bool mute);
	};

	int m_slot, m_sourceLane, m_ppq;
	double m_sourcePpqStart;
	vector<BR_MidiCCEvents::Event> m_events;
};

/******************************************************************************
* Item mute state                                                             *
******************************************************************************/
class BR_ItemMuteState
{
public:
	BR_ItemMuteState (int slot, bool selectedOnly);
	BR_ItemMuteState (int slot, ProjectStateContext* ctx);
	void SaveState (ProjectStateContext* ctx);
	void Save (bool selectedOnly);
	bool Restore (bool selectedOnly);
	int  GetSlot ();

private:
	struct MuteState{GUID guid; int mute;};
	int m_slot;
	vector<MuteState> m_items;
};

/******************************************************************************
* Track solo/mute state                                                       *
******************************************************************************/
class BR_TrackSoloMuteState
{
public:
	BR_TrackSoloMuteState (int slot, bool selectedOnly);
	BR_TrackSoloMuteState (int slot, ProjectStateContext* ctx);
	void SaveState (ProjectStateContext* ctx);
	void Save (bool selectedOnly);
	bool Restore (bool selectedOnly);
	int  GetSlot ();

private:
	struct SoloMuteState{GUID guid; int solo, mute;};
	int m_slot;
	vector<BR_TrackSoloMuteState::SoloMuteState> m_tracks;

};

/******************************************************************************
* MIDI toggle hide CC lanes                                                   *
******************************************************************************/
class BR_MidiToggleCCLane
{
public:
	BR_MidiToggleCCLane ();
	void SaveState (ProjectStateContext* ctx);
	void LoadState (ProjectStateContext* ctx);
	bool Hide (HWND midiEditor, int laneToKeep, int editorHeight = -1, int inlineHeight = -1);
	bool Restore (HWND midiEditor);
	bool IsHidden ();

private:
	vector<WDL_FastString> m_ccLanes;
};
