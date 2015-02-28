/******************************************************************************
/ BR_ProjState.h
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
extern SWSProjConfig<WDL_PtrList_DeleteOnDestroy<BR_EnvSel> >             g_envSel;
extern SWSProjConfig<WDL_PtrList_DeleteOnDestroy<BR_CursorPos> >          g_cursorPos;
extern SWSProjConfig<WDL_PtrList_DeleteOnDestroy<BR_MidiNoteSel> >        g_midiNoteSel;
extern SWSProjConfig<WDL_PtrList_DeleteOnDestroy<BR_MidiCCEvents> >       g_midiCCEvents;
extern SWSProjConfig<WDL_PtrList_DeleteOnDestroy<BR_ItemMuteState> >      g_itemMuteState;
extern SWSProjConfig<WDL_PtrList_DeleteOnDestroy<BR_TrackSoloMuteState> > g_trackSoloMuteState;
extern SWSProjConfig<BR_MidiToggleCCLane>                                 g_midiToggleHideCCLanes;

/******************************************************************************
* Call on startup to register state saving functionality                      *
******************************************************************************/
int ProjStateInit ();

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
	BR_CursorPos (int slot);
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
	void SaveState (ProjectStateContext* ctx);
	bool Save (BR_MidiEditor& midiEditor, int lane);
	bool Restore (BR_MidiEditor& midiEditor, int lane, bool allVisible, double startPositionPppq, bool showWarningForInvalidLane = true);
	int  GetSlot ();
	int CountSavedEvents();
	double GetSourcePpqStart ();

private:
	struct Event
	{
		double positionPpq;
		int channel, msg2, msg3;
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
	bool Hide (void* midiEditor, int laneToKeep, int editorHeight = -1, int inlineHeight = -1);
	bool Restore (void* midiEditor);
	bool IsHidden ();

private:
	vector<WDL_FastString> m_ccLanes;
};