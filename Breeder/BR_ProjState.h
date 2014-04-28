/******************************************************************************
/ BR_ProjState.h
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

class BR_EnvSel;
class BR_CursorPos;
class BR_MidiNoteSel;

/******************************************************************************
* Globals                                                                     *
******************************************************************************/
extern SWSProjConfig<WDL_PtrList_DeleteOnDestroy<BR_EnvSel> >      g_envSel;
extern SWSProjConfig<WDL_PtrList_DeleteOnDestroy<BR_CursorPos> >   g_cursorPos;
extern SWSProjConfig<WDL_PtrList_DeleteOnDestroy<BR_MidiNoteSel> > g_midiNoteSel;

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
	void Restore (TrackEnvelope* envelope);
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
	void Restore ();
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
	void Restore (MediaItem_Take* take);
	int  GetSlot ();
private:
	int m_slot;
	vector<int> m_selection;
};