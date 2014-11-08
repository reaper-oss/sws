/******************************************************************************
/ wol_Misc.cpp
/
/ Copyright (c) 2014 wol
/ http://forum.cockos.com/member.php?u=70153
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
#include "wol_Misc.h"
#include "wol_Util.h"
#include "../SnM/SnM_Dlg.h"
#include "../SnM/SnM_Util.h"
#include "../reaper/localize.h"

static struct wol_Misc_Ini {
	static const char* SectionMidi;

	static const char* RandomizeSelectedMidiNotesVelocitiesMin[8];
	static const char* RandomizeSelectedMidiNotesVelocitiesMax[8];
	static const char* EnableRealtimeRandomizeSelectedMidiNotesVelocities;

	static const char* SelectNotesByVelocitiesInRangeMinAdd[8];
	static const char* SelectNotesByVelocitiesInRangeMaxAdd[8];
	static const char* SelectNotesByVelocitiesInRangeMinClear[8];
	static const char* SelectNotesByVelocitiesInRangeMaxClear[8];
	static const char* EnableRealtimeNoteSelectionByVelocityInRange;
	static const char* SelectNotesByVelocityInRangeType;

	static const char* SelectRandomMidiNotesPercentAdd[8];
	static const char* SelectRandomMidiNotesPercentClear[8];
	static const char* SelectRandomMidiNotesPercentAmong[8];
	static const char* EnableRealtimeRandomNoteSelectionPerc;
	static const char* SelectRandomMidiNotesPercType;

	static const char* CompressSelectedMidiNotesVelocitiesMeanCoeff[8];
	static const char* CompressSelectedMidiNotesVelocitiesMedianCoeff[8];
	static const char* CompressSelectedMidiNotesVelocitiesVelocityCoeff[8];
	static const char* CompressSelectedMidiNotesVelocitiesVelocityVel[8];
	static const char* CompressSelectedMidiNotesVelocitiesTowards;
} wol_Misc_Ini;

const char* wol_Misc_Ini::SectionMidi = "Midi";



///////////////////////////////////////////////////////////////////////////////////////////////////
// Navigation
///////////////////////////////////////////////////////////////////////////////////////////////////
void MoveEditCursorToBeatN(COMMAND_T* ct)
{
	int beat = (int)ct->user - 1;
	int meas = 0, cml = 0;
	TimeMap2_timeToBeats(NULL, GetCursorPosition(), &meas, &cml, 0, 0);
	if (beat >= cml)
		beat = cml - 1;
	SetEditCurPos(TimeMap2_beatsToTime(0, (double)beat, &meas), 1, 0);
}

void MoveEditCursorTo(COMMAND_T* ct)
{
	if ((int)ct->user == 0)
	{
		int meas = 0;
		SetEditCurPos(TimeMap2_beatsToTime(NULL, (double)(int)(TimeMap2_timeToBeats(NULL, GetCursorPosition(), &meas, NULL, NULL, NULL) + 0.5), &meas), true, false);
	}
	else if ((int)ct->user > 0)
	{
		if ((int)ct->user == 3 && GetToggleCommandState(40370) == 1)
			ApplyNudge(NULL, 2, 6, 18, 1.0f, false, 0);
		else
		{
			int meas = 0, cml = 0;
			int beat = (int)TimeMap2_timeToBeats(NULL, GetCursorPosition(), &meas, &cml, NULL, NULL);
			if (beat == cml - 1)
			{
				if ((int)ct->user > 1)
					++meas;
				SetEditCurPos(TimeMap2_beatsToTime(NULL, 0.0f, &meas), true, false);
			}
			else
				SetEditCurPos(TimeMap2_beatsToTime(NULL, (double)(beat + 1), &meas), true, false);
		}
	}
	else
	{
		if ((int)ct->user == -3 && GetToggleCommandState(40370) == 1)
			ApplyNudge(NULL, 2, 6, 18, 1.0f, true, 0);
		else
		{
			int meas = 0, cml = 0;
			int beat = (int)TimeMap2_timeToBeats(NULL, GetCursorPosition(), &meas, &cml, NULL, NULL);
			if (beat == 0)
			{
				if ((int)ct->user < -1)
					--meas;
				SetEditCurPos(TimeMap2_beatsToTime(NULL, (double)(cml - 1), &meas), true, false);
			}
			else
				SetEditCurPos(TimeMap2_beatsToTime(NULL, (double)(beat - 1), &meas), true, false);
		}
	}
}



///////////////////////////////////////////////////////////////////////////////////////////////////
// Track
///////////////////////////////////////////////////////////////////////////////////////////////////
void SelectAllTracksExceptFolderParents(COMMAND_T* ct)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		GetSetMediaTrackInfo(tr, "I_SELECTED", *(int*)GetSetMediaTrackInfo(tr, "I_FOLDERDEPTH", NULL) == 1 ? &g_i0 : &g_i1);
	}
	Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
}



///////////////////////////////////////////////////////////////////////////////////////////////////
// Midi
///////////////////////////////////////////////////////////////////////////////////////////////////
void MoveEditCursorToNote(COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd)
{
	if (MediaItem_Take* take = MIDIEditor_GetTake(MIDIEditor_GetActive()))
	{
		int note_count = 0;
		MIDI_CountEvts(take, &note_count, NULL, NULL);
		int i = 0;
		for (i = 0; i < note_count; ++i)
		{
			double start_pos = 0.0f;
			if (MIDI_GetNote(take, i, NULL, NULL, &start_pos, NULL, NULL, NULL, NULL))
			{
				if (((int)ct->user < 0) ? MIDI_GetProjTimeFromPPQPos(take, start_pos) >= GetCursorPosition() : MIDI_GetProjTimeFromPPQPos(take, start_pos) > GetCursorPosition())
				{
					if ((int)ct->user < 0 && i > 0)
						MIDI_GetNote(take, i - 1, NULL, NULL, &start_pos, NULL, NULL, NULL, NULL);
					SetEditCurPos(MIDI_GetProjTimeFromPPQPos(take, start_pos), true, false);
					break;
				}
			}
		}
		if ((int)ct->user < 0 && i == note_count)
		{
			double start_pos = 0.0f;
			MIDI_GetNote(take, i - 1, NULL, NULL, &start_pos, NULL, NULL, NULL, NULL);
			SetEditCurPos(MIDI_GetProjTimeFromPPQPos(take, start_pos), true, false);
		}
	}
}

//---------//
#define CMD_RT_RANDMIDIVEL UserInputAndSlotsEditorWnd::CMD_OPTION_MIN

const char* wol_Misc_Ini::RandomizeSelectedMidiNotesVelocitiesMin[8] = {
	"RandomizeSelectedMidiNotesVelocitiesMinSlot1",
	"RandomizeSelectedMidiNotesVelocitiesMinSlot2",
	"RandomizeSelectedMidiNotesVelocitiesMinSlot3",
	"RandomizeSelectedMidiNotesVelocitiesMinSlot4",
	"RandomizeSelectedMidiNotesVelocitiesMinSlot5",
	"RandomizeSelectedMidiNotesVelocitiesMinSlot6",
	"RandomizeSelectedMidiNotesVelocitiesMinSlot7",
	"RandomizeSelectedMidiNotesVelocitiesMinSlot8",
};
const char* wol_Misc_Ini::RandomizeSelectedMidiNotesVelocitiesMax[8] = {
	"RandomizeSelectedMidiNotesVelocitiesMaxSlot1",
	"RandomizeSelectedMidiNotesVelocitiesMaxSlot2",
	"RandomizeSelectedMidiNotesVelocitiesMaxSlot3",
	"RandomizeSelectedMidiNotesVelocitiesMaxSlot4",
	"RandomizeSelectedMidiNotesVelocitiesMaxSlot5",
	"RandomizeSelectedMidiNotesVelocitiesMaxSlot6",
	"RandomizeSelectedMidiNotesVelocitiesMaxSlot7",
	"RandomizeSelectedMidiNotesVelocitiesMaxSlot8",
};
const char* wol_Misc_Ini::EnableRealtimeRandomizeSelectedMidiNotesVelocities = "EnableRealtimeRandomizeSelectedMidiNotesVelocities";

struct RandomizerSlots {
	int min;
	int max;
};

static RandomizerSlots g_RandomizerSlots[8];
static bool g_realtimeRandomize = false;

#define RANDMIDIVELWND_ID "WolRandMidiVelWnd"
static SNM_WindowManager<RandomMidiVelWnd> g_RandMidiVelWndMgr(RANDMIDIVELWND_ID);

static bool RandomizeSelectedMidiVelocities(int min, int max)
{
	if (MediaItem_Take* take = MIDIEditor_GetTake(MIDIEditor_GetActive()))
	{
		if (min > 0 && max < 128)
		{
			if (min < max)
			{
				int i = -1;
				while ((i = MIDI_EnumSelNotes(take, i)) != -1)
				{
					int vel = min + (int)(g_MTRand.rand()*(max - min) + 0.5);
					MIDI_SetNote(take, i, NULL, NULL, NULL, NULL, NULL, NULL, &vel);
				}
			}
			else
			{
				int i = -1;
				while ((i = MIDI_EnumSelNotes(take, i)) != -1)
					MIDI_SetNote(take, i, NULL, NULL, NULL, NULL, NULL, NULL, &min);
			}
			return true;
		}
	}
	return false;
}

void OnCommandCallback_RandMidiVelWnd(int cmd, int* kn1, int* kn2)
{
	static string undoDesc = "";
	switch (cmd)
	{
	case (UserInputAndSlotsEditorWnd::CMD_USERANSWER + IDOK):
	{
		if (RandomizeSelectedMidiVelocities(*kn1, *kn2))
			Undo_OnStateChangeEx2(NULL, undoDesc.c_str(), UNDO_STATE_ALL, -1);
		break;
	}
	case UserInputAndSlotsEditorWnd::CMD_CLOSE:
	{
		undoDesc.clear();
		break;
	}
	case CMD_SETUNDOSTR:
	{
		undoDesc = (const char*)kn1;
		break;
	}
	case CMD_RT_RANDMIDIVEL:
	{
		g_realtimeRandomize = !g_realtimeRandomize;
		SaveIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.EnableRealtimeRandomizeSelectedMidiNotesVelocities, g_realtimeRandomize);
		if (RandomMidiVelWnd* wnd = g_RandMidiVelWndMgr.Get())
		{
			wnd->EnableRealtimeNotify(g_realtimeRandomize);
			wnd->SetOptionStateChecked(CMD_RT_RANDMIDIVEL, g_realtimeRandomize);
		}
		break;
	}
	case UserInputAndSlotsEditorWnd::CMD_RT_KNOB1:
	case UserInputAndSlotsEditorWnd::CMD_RT_KNOB2:
	{
		if (RandomizeSelectedMidiVelocities(*kn1, *kn2))
			Undo_OnStateChangeEx2(NULL, undoDesc.c_str(), UNDO_STATE_ALL, -1);
		break;
	}
	default:
	{
		if (cmd < 8)
		{
			*kn1 = g_RandomizerSlots[cmd/* - UserInputAndSlotsEditorWnd::CMD_LOAD*/].min;
			*kn2 = g_RandomizerSlots[cmd].max;
			if (g_realtimeRandomize && RandomizeSelectedMidiVelocities(*kn1, *kn2))
				Undo_OnStateChangeEx2(NULL, undoDesc.c_str(), UNDO_STATE_ALL, -1);
		}
		else if (cmd < 16 && cmd > 7)
		{
			SaveIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.RandomizeSelectedMidiNotesVelocitiesMin[cmd - UserInputAndSlotsEditorWnd::CMD_SAVE], *kn1);
			SaveIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.RandomizeSelectedMidiNotesVelocitiesMax[cmd - UserInputAndSlotsEditorWnd::CMD_SAVE], *kn2);
			g_RandomizerSlots[cmd - UserInputAndSlotsEditorWnd::CMD_SAVE].min = *kn1;
			g_RandomizerSlots[cmd - UserInputAndSlotsEditorWnd::CMD_SAVE].max = *kn2;
		}
		break;
	}
	}
}

RandomMidiVelWnd::RandomMidiVelWnd()
	: UserInputAndSlotsEditorWnd(__LOCALIZE("SWS/wol-spk77 - Randomize midi velocities tool", "sws_DLG_182a"), __LOCALIZE("Random midi velocities tool", "sws_DLG_182a"), RANDMIDIVELWND_ID, SWSGetCommandID(RandomizeSelectedMidiVelocitiesTool))
{
	SetupTwoKnobs();
	SetupKnob1(1, 127, 64, 1, 12.0f, __LOCALIZE("Min value:", "sws_DLG_182a"), __LOCALIZE("", "sws_DLG_182a"), __LOCALIZE("1", "sws_DLG_182a"));
	SetupKnob2(1, 127, 64, 127, 12.0f, __LOCALIZE("Max value:", "sws_DLG_182a"), __LOCALIZE("", "sws_DLG_182a"), __LOCALIZE("1", "sws_DLG_182a"));
	SetupOnCommandCallback(OnCommandCallback_RandMidiVelWnd);
	SetupOKText(__LOCALIZE("Randomize", "sws_DLG_182a"));
	
	SetupAddOption(CMD_RT_RANDMIDIVEL, true, g_realtimeRandomize, __LOCALIZE("Enable real time randomizing", "sws_DLG_182a"));

	EnableRealtimeNotify(g_realtimeRandomize);
}

void RandomizeSelectedMidiVelocitiesTool(COMMAND_T* ct)
{
	if ((int)ct->user == 0)
	{
		if (RandomMidiVelWnd* w = g_RandMidiVelWndMgr.Create())
		{
			OnCommandCallback_RandMidiVelWnd(CMD_SETUNDOSTR, (int*)SWS_CMD_SHORTNAME(ct), NULL);
			w->Show(true, true);
		}
	}
	else
	{
		if (RandomizeSelectedMidiVelocities(g_RandomizerSlots[(int)ct->user - 1].min, g_RandomizerSlots[(int)ct->user - 1].max))
			Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
	}
}

int IsRandomizeSelectedMidiVelocitiesOpen(COMMAND_T* ct)
{
	if ((int)ct->user == 0)
		if (RandomMidiVelWnd* w = g_RandMidiVelWndMgr.Get())
			return w->IsValidWindow();
	return 0;
}

//---------//
#define CMD_RT_SELMIDINOTESBYVELINRANGE UserInputAndSlotsEditorWnd::CMD_OPTION_MIN
#define CMD_ADDNOTESTOSEL CMD_RT_SELMIDINOTESBYVELINRANGE + 1
#define CMD_ADDNOTESTONEWSEL CMD_ADDNOTESTOSEL + 1

const char* wol_Misc_Ini::SelectNotesByVelocitiesInRangeMinAdd[8] = {
	"SelectNotesByVelocitiesInRangeMinAddSlot1",
	"SelectNotesByVelocitiesInRangeMinAddSlot2",
	"SelectNotesByVelocitiesInRangeMinAddSlot3",
	"SelectNotesByVelocitiesInRangeMinAddSlot4",
	"SelectNotesByVelocitiesInRangeMinAddSlot5",
	"SelectNotesByVelocitiesInRangeMinAddSlot6",
	"SelectNotesByVelocitiesInRangeMinAddSlot7",
	"SelectNotesByVelocitiesInRangeMinAddSlot8",
};
const char* wol_Misc_Ini::SelectNotesByVelocitiesInRangeMaxAdd[8] = {
	"SelectNotesByVelocitiesInRangeMaxAddSlot1",
	"SelectNotesByVelocitiesInRangeMaxAddSlot2",
	"SelectNotesByVelocitiesInRangeMaxAddSlot3",
	"SelectNotesByVelocitiesInRangeMaxAddSlot4",
	"SelectNotesByVelocitiesInRangeMaxAddSlot5",
	"SelectNotesByVelocitiesInRangeMaxAddSlot6",
	"SelectNotesByVelocitiesInRangeMaxAddSlot7",
	"SelectNotesByVelocitiesInRangeMaxAddSlot8",
};
const char* wol_Misc_Ini::SelectNotesByVelocitiesInRangeMinClear[8] = {
	"SelectNotesByVelocitiesInRangeMinClearSlot1",
	"SelectNotesByVelocitiesInRangeMinClearSlot2",
	"SelectNotesByVelocitiesInRangeMinClearSlot3",
	"SelectNotesByVelocitiesInRangeMinClearSlot4",
	"SelectNotesByVelocitiesInRangeMinClearSlot5",
	"SelectNotesByVelocitiesInRangeMinClearSlot6",
	"SelectNotesByVelocitiesInRangeMinClearSlot7",
	"SelectNotesByVelocitiesInRangeMinClearSlot8",
};
const char* wol_Misc_Ini::SelectNotesByVelocitiesInRangeMaxClear[8] = {
	"SelectNotesByVelocitiesInRangeMaxClearSlot1",
	"SelectNotesByVelocitiesInRangeMaxClearSlot2",
	"SelectNotesByVelocitiesInRangeMaxClearSlot3",
	"SelectNotesByVelocitiesInRangeMaxClearSlot4",
	"SelectNotesByVelocitiesInRangeMaxClearSlot5",
	"SelectNotesByVelocitiesInRangeMaxClearSlot6",
	"SelectNotesByVelocitiesInRangeMaxClearSlot7",
	"SelectNotesByVelocitiesInRangeMaxClearSlot8",
};
const char* wol_Misc_Ini::EnableRealtimeNoteSelectionByVelocityInRange = "EnableRealtimeNoteSelectionByVelocityInRange";
const char* wol_Misc_Ini::SelectNotesByVelocityInRangeType = "SelectNotesByVelocityInRangeType";

struct MidiVelocitiesSlots {
	int min;
	int max;
};

static MidiVelocitiesSlots g_MidiVelocitiesAddSlots[8];
static MidiVelocitiesSlots g_MidiVelocitiesClearSlots[8];
static bool g_realtimeNoteSelByVelInRange = false;
static bool g_addNotesToSel = false; //true add to selection, false clear selection

#define SELMIDINOTESBYVELINRANGEWND_ID "WolSelMidiNotesByVelInRangeWnd"
static SNM_WindowManager<SelMidiNotesByVelInRangeWnd> g_SelMidiNotesByVelInRangeWndMgr(SELMIDINOTESBYVELINRANGEWND_ID);

static bool SelectMidiNotesByVelocityInRange(int min, int max, bool addToSelection)
{
	if (MediaItem_Take* take = MIDIEditor_GetTake(MIDIEditor_GetActive()))
	{
		if (min > 0 && max < 128)
		{
			int cnt = 0;
			MIDI_CountEvts(take, &cnt, NULL, NULL);
			for (int i = 0; i < cnt; ++i)
			{
				bool sel = false;
				int vel = 0;
				MIDI_GetNote(take, i, &sel, NULL, NULL, NULL, NULL, NULL, &vel);
				if (addToSelection && sel)
					continue;

				sel = false;
				if (min < max)
				{
					if (vel >= min && vel <= max)
						sel = true;
				}
				else
				{
					if (vel == min)
						sel = true;
				}
				MIDI_SetNote(take, i, &sel, NULL, NULL, NULL, NULL, NULL, NULL);
			}
			return true;
		}
	}
	return false;
}

void OnCommandCallback_SelMidiNotesByVelInRangeWnd(int cmd, int* kn1, int* kn2)
{
	static string undoDesc = "";
	switch (cmd)
	{
	case (UserInputAndSlotsEditorWnd::CMD_USERANSWER + IDOK):
	{
		if (SelectMidiNotesByVelocityInRange(*kn1, *kn2, g_addNotesToSel))
			Undo_OnStateChangeEx2(NULL, undoDesc.c_str(), UNDO_STATE_ALL, -1);
		break;
	}
	case UserInputAndSlotsEditorWnd::CMD_CLOSE:
	{
		undoDesc.clear();
		break;
	}
	case CMD_SETUNDOSTR:
	{
		undoDesc = (const char*)kn1;
		break;
	}
	case CMD_RT_SELMIDINOTESBYVELINRANGE:
	{
		g_realtimeNoteSelByVelInRange = !g_realtimeNoteSelByVelInRange;
		SaveIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.EnableRealtimeNoteSelectionByVelocityInRange, g_realtimeNoteSelByVelInRange);
		if (SelMidiNotesByVelInRangeWnd* wnd = g_SelMidiNotesByVelInRangeWndMgr.Get())
		{
			wnd->EnableRealtimeNotify(g_realtimeNoteSelByVelInRange);
			wnd->SetOptionStateChecked(CMD_RT_SELMIDINOTESBYVELINRANGE, g_realtimeNoteSelByVelInRange);
		}
		break;
	}
	case CMD_ADDNOTESTOSEL:
	case CMD_ADDNOTESTONEWSEL:
	{
		g_addNotesToSel = (cmd == CMD_ADDNOTESTOSEL);
		SaveIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.SelectNotesByVelocityInRangeType, g_addNotesToSel);
		if (SelMidiNotesByVelInRangeWnd* wnd = g_SelMidiNotesByVelInRangeWndMgr.Get())
		{
			wnd->SetOptionStateChecked(CMD_ADDNOTESTOSEL, g_addNotesToSel);
			wnd->SetOptionStateChecked(CMD_ADDNOTESTONEWSEL, !g_addNotesToSel);
		}
		break;
	}
	case UserInputAndSlotsEditorWnd::CMD_RT_KNOB1:
	case UserInputAndSlotsEditorWnd::CMD_RT_KNOB2:
	{
		if (SelectMidiNotesByVelocityInRange(*kn1, *kn2, g_addNotesToSel))
			Undo_OnStateChangeEx2(NULL, undoDesc.c_str(), UNDO_STATE_ALL, -1);
		break;
	}
	default:
	{
		if (cmd < 8)
		{
			if (g_addNotesToSel)
			{
				*kn1 = g_MidiVelocitiesAddSlots[cmd/* - UserInputAndSlotsEditorWnd::CMD_LOAD*/].min;
				*kn2 = g_MidiVelocitiesAddSlots[cmd].max;
			}
			else
			{
				*kn1 = g_MidiVelocitiesClearSlots[cmd/* - UserInputAndSlotsEditorWnd::CMD_LOAD*/].min;
				*kn2 = g_MidiVelocitiesClearSlots[cmd].max;
			}
			if (g_realtimeNoteSelByVelInRange && SelectMidiNotesByVelocityInRange(*kn1, *kn2, g_addNotesToSel))
				Undo_OnStateChangeEx2(NULL, undoDesc.c_str(), UNDO_STATE_ALL, -1);
		}
		else if (cmd < 16 && cmd > 7)
		{
			if (g_addNotesToSel)
			{
				SaveIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.SelectNotesByVelocitiesInRangeMinAdd[cmd - UserInputAndSlotsEditorWnd::CMD_SAVE], *kn1);
				SaveIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.SelectNotesByVelocitiesInRangeMaxAdd[cmd - UserInputAndSlotsEditorWnd::CMD_SAVE], *kn2);
				g_MidiVelocitiesAddSlots[cmd - UserInputAndSlotsEditorWnd::CMD_SAVE].min = *kn1;
				g_MidiVelocitiesAddSlots[cmd - UserInputAndSlotsEditorWnd::CMD_SAVE].max = *kn2;
			}
			else
			{
				SaveIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.SelectNotesByVelocitiesInRangeMinClear[cmd - UserInputAndSlotsEditorWnd::CMD_SAVE], *kn1);
				SaveIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.SelectNotesByVelocitiesInRangeMaxClear[cmd - UserInputAndSlotsEditorWnd::CMD_SAVE], *kn2);
				g_MidiVelocitiesClearSlots[cmd - UserInputAndSlotsEditorWnd::CMD_SAVE].min = *kn1;
				g_MidiVelocitiesClearSlots[cmd - UserInputAndSlotsEditorWnd::CMD_SAVE].max = *kn2;
			}
		}
		break;
	}
	}
}

SelMidiNotesByVelInRangeWnd::SelMidiNotesByVelInRangeWnd()
	: UserInputAndSlotsEditorWnd(__LOCALIZE("SWS/wol-spk77 - Select midi notes by velocities in range tool", "sws_DLG_182b"), __LOCALIZE("Select midi notes by velocities in range tool", "sws_DLG_182b"), SELMIDINOTESBYVELINRANGEWND_ID, SWSGetCommandID(SelectMidiNotesByVelocitiesInRangeTool))
{
	SetupTwoKnobs();
	SetupKnob1(1, 127, 64, 1, 12.0f, __LOCALIZE("Min velocity:", "sws_DLG_182b"), __LOCALIZE("", "sws_DLG_182b"), __LOCALIZE("1", "sws_DLG_182b"));
	SetupKnob2(1, 127, 64, 127, 12.0f, __LOCALIZE("Max velocity:", "sws_DLG_182b"), __LOCALIZE("", "sws_DLG_182b"), __LOCALIZE("1", "sws_DLG_182b"));
	SetupOnCommandCallback(OnCommandCallback_SelMidiNotesByVelInRangeWnd);
	SetupOKText(__LOCALIZE("Select", "sws_DLG_182b"));
	
	SetupAddOption(CMD_RT_SELMIDINOTESBYVELINRANGE, true, g_realtimeNoteSelByVelInRange, __LOCALIZE("Enable real time note selection", "sws_DLG_182b"));
	SetupAddOption(UserInputAndSlotsEditorWnd::CMD_OPTION_MAX, true, false, SWS_SEPARATOR);
	SetupAddOption(CMD_ADDNOTESTOSEL, true, g_addNotesToSel, __LOCALIZE("Add notes to selection", "sws_DLG_182b"));
	SetupAddOption(CMD_ADDNOTESTONEWSEL, true, !g_addNotesToSel, __LOCALIZE("Clear selection", "sws_DLG_182b"));

	EnableRealtimeNotify(g_realtimeNoteSelByVelInRange);
}

void SelectMidiNotesByVelocitiesInRangeTool(COMMAND_T* ct)
{
	if ((int)ct->user == 0)
	{
		if (SelMidiNotesByVelInRangeWnd* w = g_SelMidiNotesByVelInRangeWndMgr.Create())
		{
			OnCommandCallback_SelMidiNotesByVelInRangeWnd(CMD_SETUNDOSTR, (int*)SWS_CMD_SHORTNAME(ct), NULL);
			w->Show(true, true);
		}
	}
	else
	{
		int min = 0, max = 0;
		int id = (int)ct->user - ((int)ct->user < 9 ? 1 : 9);
		bool type = ((int)ct->user < 9 ? false : true);
		if (type)
		{
			min = g_MidiVelocitiesAddSlots[id].min;
			max = g_MidiVelocitiesAddSlots[id].max;
		}
		else
		{
			min = g_MidiVelocitiesClearSlots[id].min;
			max = g_MidiVelocitiesClearSlots[id].max;
		}
		if (SelectMidiNotesByVelocityInRange(min, max, type))
			Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
	}
}

int IsSelectMidiNotesByVelocitiesInRangeOpen(COMMAND_T* ct)
{
	if ((int)ct->user == 0)
		if (SelMidiNotesByVelInRangeWnd* w = g_SelMidiNotesByVelInRangeWndMgr.Get())
			return w->IsValidWindow();
	return 0;
}

//---------//
#define CMD_RT_SELRANDMIDINOTESPERC UserInputAndSlotsEditorWnd::CMD_OPTION_MIN
#define CMD_ADDTOSEL CMD_RT_SELRANDMIDINOTESPERC + 1
#define CMD_CLEARSEL CMD_RT_SELRANDMIDINOTESPERC + 2
#define CMD_AMONGSELNOTES CMD_RT_SELRANDMIDINOTESPERC + 3

const char* wol_Misc_Ini::SelectRandomMidiNotesPercentAdd[8] = {
	"SelectRandomMidiNotesPercentAddSlot1",
	"SelectRandomMidiNotesPercentAddSlot2",
	"SelectRandomMidiNotesPercentAddSlot3",
	"SelectRandomMidiNotesPercentAddSlot4",
	"SelectRandomMidiNotesPercentAddSlot5",
	"SelectRandomMidiNotesPercentAddSlot6",
	"SelectRandomMidiNotesPercentAddSlot7",
	"SelectRandomMidiNotesPercentAddSlot8",
};
const char* wol_Misc_Ini::SelectRandomMidiNotesPercentClear[8] = {
	"SelectRandomMidiNotesPercentClearSlot1",
	"SelectRandomMidiNotesPercentClearSlot2",
	"SelectRandomMidiNotesPercentClearSlot3",
	"SelectRandomMidiNotesPercentClearSlot4",
	"SelectRandomMidiNotesPercentClearSlot5",
	"SelectRandomMidiNotesPercentClearSlot6",
	"SelectRandomMidiNotesPercentClearSlot7",
	"SelectRandomMidiNotesPercentClearSlot8",
};
const char* wol_Misc_Ini::SelectRandomMidiNotesPercentAmong[8] = {
	"SelectRandomMidiNotesPercentAmongSlot1",
	"SelectRandomMidiNotesPercentAmongSlot2",
	"SelectRandomMidiNotesPercentAmongSlot3",
	"SelectRandomMidiNotesPercentAmongSlot4",
	"SelectRandomMidiNotesPercentAmongSlot5",
	"SelectRandomMidiNotesPercentAmongSlot6",
	"SelectRandomMidiNotesPercentAmongSlot7",
	"SelectRandomMidiNotesPercentAmongSlot8",
};
const char* wol_Misc_Ini::EnableRealtimeRandomNoteSelectionPerc = "EnableRealtimeRandomNoteSelectionPerc";
const char* wol_Misc_Ini::SelectRandomMidiNotesPercType = "SelectRandomMidiNotesPercType";

static UINT g_SelRandMidiNotesPercentAdd[8];
static UINT g_SelRandMidiNotesPercentClear[8];
static UINT g_SelRandMidiNotesPercentAmong[8];
static bool g_realtimeRandomNoteSelPerc = false;
static int g_selType = 0; //0 add to selection, 1 clear selection, 2 among selected notes

#define SELRANDMIDINOTESPERCWND_ID "WolSelRandMidiNotesPercWnd"
static SNM_WindowManager<SelRandMidiNotesPercWnd> g_SelRandMidiNotesPercWndMgr(SELRANDMIDINOTESPERCWND_ID);

static bool SelectRandomMidiNotesPercent(UINT perc, bool addToSelection, bool amongSelNotes)
{
	if (MediaItem_Take* take = MIDIEditor_GetTake(MIDIEditor_GetActive()))
	{
		if (perc >= 0 && perc < 101)
		{
			int cnt = 0;
			MIDI_CountEvts(take, &cnt, NULL, NULL);
			for (int i = 0; i < cnt; ++i)
			{
				bool sel = false;
				MIDI_GetNote(take, i, &sel, NULL, NULL, NULL, NULL, NULL, NULL);
				if ((addToSelection && sel && !amongSelNotes) || (amongSelNotes && !sel))
					continue;

				sel = (g_MTRand.randInt() % 100) < perc;
				MIDI_SetNote(take, i, &sel, NULL, NULL, NULL, NULL, NULL, NULL);
			}
			return true;
		}
	}
	return false;
}

void OnCommandCallback_SelRandMidiNotesPercWnd(int cmd, int* kn1, int* kn2)
{
	static string undoDesc;
	switch (cmd)
	{
	case (UserInputAndSlotsEditorWnd::CMD_USERANSWER + IDOK) :
	{
		if (SelectRandomMidiNotesPercent((UINT)*kn1, (g_selType == 0 ? true : false), (g_selType == 2 ? true : false)))
			Undo_OnStateChangeEx2(NULL, undoDesc.c_str(), UNDO_STATE_ALL, -1);
		break;
	}
	case UserInputAndSlotsEditorWnd::CMD_CLOSE:
	{
		undoDesc.clear();
		break;
	}
	case CMD_SETUNDOSTR:
	{
		undoDesc = (const char*)kn1;
		break;
	}
	case CMD_RT_SELRANDMIDINOTESPERC:
	{
		g_realtimeRandomNoteSelPerc = !g_realtimeRandomNoteSelPerc;
		SaveIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.EnableRealtimeRandomNoteSelectionPerc, g_realtimeRandomNoteSelPerc);
		if (SelRandMidiNotesPercWnd* wnd = g_SelRandMidiNotesPercWndMgr.Get())
		{
			wnd->EnableRealtimeNotify(g_realtimeRandomNoteSelPerc);
			wnd->SetOptionStateChecked(CMD_RT_SELRANDMIDINOTESPERC, g_realtimeRandomNoteSelPerc);
		}
		break;
	}
	case CMD_ADDTOSEL:
	case CMD_CLEARSEL:
	case CMD_AMONGSELNOTES:
	{
		int _cmd = CMD_ADDTOSEL;
		int res = cmd - _cmd;
		g_selType = res/*cmd - CMD_ADDTOSEL*/;
		SaveIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.SelectRandomMidiNotesPercType, g_selType);
		if (SelRandMidiNotesPercWnd* wnd = g_SelRandMidiNotesPercWndMgr.Get())
		{
			wnd->SetOptionStateChecked(CMD_ADDTOSEL, g_selType == 0);
			wnd->SetOptionStateChecked(CMD_CLEARSEL, g_selType == 1);
			wnd->SetOptionStateChecked(CMD_AMONGSELNOTES, g_selType == 2);
		}
		break;
	}
	case UserInputAndSlotsEditorWnd::CMD_RT_KNOB1:
	case UserInputAndSlotsEditorWnd::CMD_RT_KNOB2:
	{
		if (SelectRandomMidiNotesPercent((UINT)*kn1, (g_selType == 0 ? true : false), (g_selType == 2 ? true : false)))
			Undo_OnStateChangeEx2(NULL, undoDesc.c_str(), UNDO_STATE_ALL, -1);
		break;
	}
	default:
	{
		if (cmd < 8)
		{
			if (g_selType == 0)
				*kn1 = g_SelRandMidiNotesPercentAdd[cmd/* - UserInputAndSlotsEditorWnd::CMD_LOAD*/];
			else if (g_selType == 1)
				*kn1 = g_SelRandMidiNotesPercentClear[cmd];
			else
				*kn1 = g_SelRandMidiNotesPercentAmong[cmd];
			
			if (g_realtimeRandomNoteSelPerc && SelectRandomMidiNotesPercent((UINT)*kn1, (g_selType == 0 ? true : false), (g_selType == 2 ? true : false)))
				Undo_OnStateChangeEx2(NULL, undoDesc.c_str(), UNDO_STATE_ALL, -1);
		}
		else if (cmd < 16 && cmd > 7)
		{
			if (g_selType == 0)
			{
				SaveIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.SelectRandomMidiNotesPercentAdd[cmd - UserInputAndSlotsEditorWnd::CMD_SAVE], *kn1);
				g_SelRandMidiNotesPercentAdd[cmd - UserInputAndSlotsEditorWnd::CMD_SAVE] = (UINT)*kn1;
			}
			else if (g_selType == 1)
			{
				SaveIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.SelectRandomMidiNotesPercentClear[cmd - UserInputAndSlotsEditorWnd::CMD_SAVE], *kn1);
				g_SelRandMidiNotesPercentClear[cmd - UserInputAndSlotsEditorWnd::CMD_SAVE] = (UINT)*kn1;
			}
			else
			{
				SaveIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.SelectRandomMidiNotesPercentAmong[cmd - UserInputAndSlotsEditorWnd::CMD_SAVE], *kn1);
				g_SelRandMidiNotesPercentAmong[cmd - UserInputAndSlotsEditorWnd::CMD_SAVE] = (UINT)*kn1;
			}
		}
		break;
	}
	}
}

SelRandMidiNotesPercWnd::SelRandMidiNotesPercWnd()
	: UserInputAndSlotsEditorWnd(__LOCALIZE("SWS/wol-spk77 - Select random midi notes tool", "sws_DLG_182c"), __LOCALIZE("Select random midi notes tool", "sws_DLG_182c"), SELRANDMIDINOTESPERCWND_ID, SWSGetCommandID(SelectMidiNotesByVelocitiesInRangeTool))
{
	SetupKnob1(1, 100, 50, 50, 12.0f, __LOCALIZE("Percent:", "sws_DLG_182c"), __LOCALIZE("%", "sws_DLG_182c"), __LOCALIZE("", "sws_DLG_182c"));
	SetupOnCommandCallback(OnCommandCallback_SelRandMidiNotesPercWnd);
	SetupOKText(__LOCALIZE("Select", "sws_DLG_182c"));

	SetupAddOption(CMD_RT_SELRANDMIDINOTESPERC, true, g_realtimeRandomNoteSelPerc, __LOCALIZE("Enable real time selection", "sws_DLG_182c"));
	SetupAddOption(UserInputAndSlotsEditorWnd::CMD_OPTION_MAX, true, false, SWS_SEPARATOR);
	SetupAddOption(CMD_ADDTOSEL, true, g_selType == 0, __LOCALIZE("Add notes to selection", "sws_DLG_182c"));
	SetupAddOption(CMD_CLEARSEL, true, g_selType == 1, __LOCALIZE("Clear selection", "sws_DLG_182c"));
	SetupAddOption(CMD_AMONGSELNOTES, true, g_selType == 2, __LOCALIZE("Select notes among selected notes", "sws_DLG_182c"));

	EnableRealtimeNotify(g_realtimeRandomNoteSelPerc);
}

void SelectRandomMidiNotesTool(COMMAND_T* ct)
{
	if ((int)ct->user == 0)
	{
		if (SelRandMidiNotesPercWnd* w = g_SelRandMidiNotesPercWndMgr.Create())
		{
			OnCommandCallback_SelRandMidiNotesPercWnd(CMD_SETUNDOSTR, (int*)SWS_CMD_SHORTNAME(ct), NULL);
			w->Show(true, true);
		}
	}
	else
	{
		int perc = 0;
		int id = (int)ct->user - ((int)ct->user < 9 ? 1 : ((int)ct->user < 17 ? 9 : 17));
		int type = ((int)ct->user < 9 ? 1 : ((int)ct->user < 17 ? 0 : 2));
		if (type == 0)
			perc = g_SelRandMidiNotesPercentAdd[id];
		else if (type == 1)
			perc = g_SelRandMidiNotesPercentClear[id];
		else
			perc = g_SelRandMidiNotesPercentAmong[id];
		if (SelectRandomMidiNotesPercent(perc, type == 0, type == 2))
			Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
	}
}

int IsSelectRandomMidiNotesOpen(COMMAND_T* ct)
{
	if ((int)ct->user == 0)
		if (SelRandMidiNotesPercWnd* w = g_SelRandMidiNotesPercWndMgr.Get())
			return w->IsValidWindow();
	return 0;
}

//---------//
#define CMD_MEAN UserInputAndSlotsEditorWnd::CMD_OPTION_MIN
#define CMD_MEDIAN CMD_MEAN + 1
#define CMD_VELOCITY CMD_MEAN + 2

const char* wol_Misc_Ini::CompressSelectedMidiNotesVelocitiesMeanCoeff[8] = {
	"CompressSelectedMidiNotesVelocitiesMeanCoeffSlot1",
	"CompressSelectedMidiNotesVelocitiesMeanCoeffSlot2",
	"CompressSelectedMidiNotesVelocitiesMeanCoeffSlot3",
	"CompressSelectedMidiNotesVelocitiesMeanCoeffSlot4",
	"CompressSelectedMidiNotesVelocitiesMeanCoeffSlot5",
	"CompressSelectedMidiNotesVelocitiesMeanCoeffSlot6",
	"CompressSelectedMidiNotesVelocitiesMeanCoeffSlot7",
	"CompressSelectedMidiNotesVelocitiesMeanCoeffSlot8",
};
const char* wol_Misc_Ini::CompressSelectedMidiNotesVelocitiesMedianCoeff[8] = {
	"CompressSelectedMidiNotesVelocitiesMedianCoeffSlot1",
	"CompressSelectedMidiNotesVelocitiesMedianCoeffSlot2",
	"CompressSelectedMidiNotesVelocitiesMedianCoeffSlot3",
	"CompressSelectedMidiNotesVelocitiesMedianCoeffSlot4",
	"CompressSelectedMidiNotesVelocitiesMedianCoeffSlot5",
	"CompressSelectedMidiNotesVelocitiesMedianCoeffSlot6",
	"CompressSelectedMidiNotesVelocitiesMedianCoeffSlot7",
	"CompressSelectedMidiNotesVelocitiesMedianCoeffSlot8",
};
const char* wol_Misc_Ini::CompressSelectedMidiNotesVelocitiesVelocityCoeff[8] = {
	"CompressSelectedMidiNotesVelocitiesVelocityCoeffSlot1",
	"CompressSelectedMidiNotesVelocitiesVelocityCoeffSlot2",
	"CompressSelectedMidiNotesVelocitiesVelocityCoeffSlot3",
	"CompressSelectedMidiNotesVelocitiesVelocityCoeffSlot4",
	"CompressSelectedMidiNotesVelocitiesVelocityCoeffSlot5",
	"CompressSelectedMidiNotesVelocitiesVelocityCoeffSlot6",
	"CompressSelectedMidiNotesVelocitiesVelocityCoeffSlot7",
	"CompressSelectedMidiNotesVelocitiesVelocityCoeffSlot8",
};
const char* wol_Misc_Ini::CompressSelectedMidiNotesVelocitiesVelocityVel[8] = {
	"CompressSelectedMidiNotesVelocitiesVelocityVelSlot1",
	"CompressSelectedMidiNotesVelocitiesVelocityVelSlot2",
	"CompressSelectedMidiNotesVelocitiesVelocityVelSlot3",
	"CompressSelectedMidiNotesVelocitiesVelocityVelSlot4",
	"CompressSelectedMidiNotesVelocitiesVelocityVelSlot5",
	"CompressSelectedMidiNotesVelocitiesVelocityVelSlot6",
	"CompressSelectedMidiNotesVelocitiesVelocityVelSlot7",
	"CompressSelectedMidiNotesVelocitiesVelocityVelSlot8",
};
const char* wol_Misc_Ini::CompressSelectedMidiNotesVelocitiesTowards = "CompressSelectedMidiNotesVelocitiesTowards";

struct MidiVelocitiesCompressionSlots {
	int coeff;
	int velocity;
};

static int g_CompSelMidiNotesCoeffMean[8];
static int g_CompSelMidiNotesCoeffMedian[8];
static MidiVelocitiesCompressionSlots g_CompSelMidiNotesCoeffVelocity[8];
static int g_targetVel = 0; //0 mean, 1 median, 2 velocity

#define COMPSELMIDINOTESVELWND_ID "WolCompSelMidiNotesVelWnd"
static SNM_WindowManager<CompressSelectedMidiNotesVelocitiesWnd> g_CompSelMidiNotesVelWndMgr(COMPSELMIDINOTESVELWND_ID);

static bool CompressSelectedMidiNotesVelocities(int coeff, int targetVel)
{
	if (MediaItem_Take* take = MIDIEditor_GetTake(MIDIEditor_GetActive()))
	{
		if (coeff > 0 && coeff < 11)
		{
			int i = -1;
			double _coeff = (double)coeff / 10;
			while ((i = MIDI_EnumSelNotes(take, i)) != -1)
			{
				int vel = 0, diff = 0, newvel = 0;
				MIDI_GetNote(take, i, NULL, NULL, NULL, NULL, NULL, NULL, &vel);
				if (vel > targetVel)
				{
					diff = vel - targetVel;
					newvel = vel - (int)round(diff * _coeff);
					if (round(diff * _coeff) < 1)
						newvel = vel - 1;
					if (newvel < targetVel)
						newvel = targetVel;
				}
				else
				{
					diff = targetVel - vel;
					newvel = vel + (int)round(diff * _coeff);
					if (round(diff * _coeff) < 1)
						newvel = vel + 1;
					if (newvel > targetVel)
						newvel = targetVel;
				}
				MIDI_SetNote(take, i, NULL, NULL, NULL, NULL, NULL, NULL, &newvel);
			}
			return true;
		}
	}
	return false;
}

void OnCommandCallback_CompSelMidiNotesVelWnd(int cmd, int* kn1, int* kn2)
{
	static string undoDesc = "";
	switch (cmd)
	{
	case (UserInputAndSlotsEditorWnd::CMD_USERANSWER + IDOK):
	{
		int vel = 0;
		if (g_targetVel == 2)
			vel = *kn2;
		else
		{
			if (MediaItem_Take* take = MIDIEditor_GetTake(MIDIEditor_GetActive()))
			{
				vector<int> vels;
				vels.clear();
				int i = -1;
				while ((i = MIDI_EnumSelNotes(take, i)) != -1)
				{
					int vel = 0;
					MIDI_GetNote(take, i, NULL, NULL, NULL, NULL, NULL, NULL, &vel);
					vels.push_back(vel);
				}
				if (!vels.size())
					return;

				if (g_targetVel == 0)
					vel = GetMean(vels);
				else
					vel = GetMedian(vels);
			}
			else
				return;
		}

		if (CompressSelectedMidiNotesVelocities(*kn1, vel))
			Undo_OnStateChangeEx2(NULL, undoDesc.c_str(), UNDO_STATE_ALL, -1);
		break;
	}
	case UserInputAndSlotsEditorWnd::CMD_CLOSE:
	{
		undoDesc.clear();
		break;
	}
	case CMD_SETUNDOSTR:
	{
		undoDesc = (const char*)kn1;
		break;
	}
	case CMD_MEAN:
	case CMD_MEDIAN:
	case CMD_VELOCITY:
	{
		g_targetVel = cmd - CMD_MEAN;
		SaveIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.CompressSelectedMidiNotesVelocitiesTowards, g_targetVel);
		if (CompressSelectedMidiNotesVelocitiesWnd* wnd = g_CompSelMidiNotesVelWndMgr.Get())
		{
			wnd->SetOptionStateChecked(CMD_MEAN, g_targetVel == 0);
			wnd->SetOptionStateChecked(CMD_MEDIAN, g_targetVel == 1);
			wnd->SetOptionStateChecked(CMD_VELOCITY, g_targetVel == 2);

			if (g_targetVel == 2)
				wnd->SetupTwoKnobs();
			else
				wnd->SetupTwoKnobs(false);
			wnd->Update();
		}
		break;
	}
	default:
	{
		if (cmd < 8)
		{
			if (g_targetVel == 0)
				*kn1 = g_CompSelMidiNotesCoeffMean[cmd/* - UserInputAndSlotsEditorWnd::CMD_LOAD*/];
			else if (g_targetVel == 1)
				*kn1 = g_CompSelMidiNotesCoeffMedian[cmd/* - UserInputAndSlotsEditorWnd::CMD_LOAD*/];
			else
			{
				*kn1 = g_CompSelMidiNotesCoeffVelocity[cmd/* - UserInputAndSlotsEditorWnd::CMD_LOAD*/].coeff;
				*kn2 = g_CompSelMidiNotesCoeffVelocity[cmd].velocity;
			}
		}
		else if (cmd < 16 && cmd > 7)
		{
			if (g_targetVel == 0)
			{
				SaveIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.CompressSelectedMidiNotesVelocitiesMeanCoeff[cmd - UserInputAndSlotsEditorWnd::CMD_SAVE], *kn1);
				g_CompSelMidiNotesCoeffMean[cmd - UserInputAndSlotsEditorWnd::CMD_SAVE] = *kn1;
			}
			else if (g_targetVel == 1)
			{
				SaveIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.CompressSelectedMidiNotesVelocitiesMedianCoeff[cmd - UserInputAndSlotsEditorWnd::CMD_SAVE], *kn1);
				g_CompSelMidiNotesCoeffMedian[cmd - UserInputAndSlotsEditorWnd::CMD_SAVE] = *kn1;
			}
			else
			{
				SaveIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.CompressSelectedMidiNotesVelocitiesVelocityCoeff[cmd - UserInputAndSlotsEditorWnd::CMD_SAVE], *kn1);
				SaveIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.CompressSelectedMidiNotesVelocitiesVelocityVel[cmd - UserInputAndSlotsEditorWnd::CMD_SAVE], *kn2);
				g_CompSelMidiNotesCoeffVelocity[cmd - UserInputAndSlotsEditorWnd::CMD_SAVE].coeff = *kn1;
				g_CompSelMidiNotesCoeffVelocity[cmd - UserInputAndSlotsEditorWnd::CMD_SAVE].velocity = *kn2;
			}
		}
		break;
	}
	}
}

CompressSelectedMidiNotesVelocitiesWnd::CompressSelectedMidiNotesVelocitiesWnd()
	: UserInputAndSlotsEditorWnd(__LOCALIZE("SWS/wol-spk77 - Compress selected midi notes velocities tool", "sws_DLG_182d"), __LOCALIZE("Compress selected midi notes velocities tool", "sws_DLG_182d"), COMPSELMIDINOTESVELWND_ID, SWSGetCommandID(CompressSelectedMidiNotesVelocitiesTool))
{
	if (g_targetVel == 2)
		SetupTwoKnobs();
	SetupLinkKnobs(false);
	SetupKnob1(1, 10, 5, 5, 20.0f, __LOCALIZE("Coefficient:", "sws_DLG_182d"), __LOCALIZE("", "sws_DLG_182d"), __LOCALIZE("1", "sws_DLG_182d"));
	SetupKnob2(1, 127, 64, 64, 12.0f, __LOCALIZE("Velocity:", "sws_DLG_182d"), __LOCALIZE("", "sws_DLG_182d"), __LOCALIZE("1", "sws_DLG_182d"));
	SetupOnCommandCallback(OnCommandCallback_CompSelMidiNotesVelWnd);
	SetupOKText(__LOCALIZE("Compress", "sws_DLG_182d"));

	SetupAddOption(CMD_MEAN, true, g_targetVel == 0, __LOCALIZE("Compress towards mean velocity", "sws_DLG_182d"));
	SetupAddOption(CMD_MEDIAN, true, g_targetVel == 1, __LOCALIZE("Compress towards median velocity", "sws_DLG_182d"));
	SetupAddOption(CMD_VELOCITY, true, g_targetVel == 2, __LOCALIZE("Compress towards velocity", "sws_DLG_182d"));
}

void CompressSelectedMidiNotesVelocitiesTool(COMMAND_T* ct)
{
	if ((int)ct->user == 0)
	{
		if (CompressSelectedMidiNotesVelocitiesWnd* w = g_CompSelMidiNotesVelWndMgr.Create())
		{
			OnCommandCallback_CompSelMidiNotesVelWnd(CMD_SETUNDOSTR, (int*)SWS_CMD_SHORTNAME(ct), NULL);
			w->Show(true, true);
		}
	}
	else
	{
		int id = (int)ct->user - ((int)ct->user < 9 ? 1 : ((int)ct->user < 17 ? 9 : 17));
		int target = ((int)ct->user < 9 ? 0 : ((int)ct->user < 17 ? 1 : 2));
		int coeff = 0, vel = 0;
		if (target == 2)
		{
			coeff = g_CompSelMidiNotesCoeffVelocity[id].coeff;
			vel = g_CompSelMidiNotesCoeffVelocity[id].velocity;
		}
		else
		{
			if (MediaItem_Take* take = MIDIEditor_GetTake(MIDIEditor_GetActive()))
			{
				vector<int> vels;
				vels.clear();
				int i = -1;
				while ((i = MIDI_EnumSelNotes(take, i)) != -1)
				{
					int vel = 0;
					MIDI_GetNote(take, i, NULL, NULL, NULL, NULL, NULL, NULL, &vel);
					vels.push_back(vel);
				}
				if (!vels.size())
					return;

				if (target == 0)
				{
					coeff = g_CompSelMidiNotesCoeffMean[id];
					vel = GetMean(vels);
				}
				else
				{
					coeff = g_CompSelMidiNotesCoeffMedian[id];
					vel = GetMedian(vels);
				}
			}
			else
				return;
		}

		if (CompressSelectedMidiNotesVelocities(coeff, vel))
			Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
	}
}

int IsCompressSelectedMidiNotesVelocitiesOpen(COMMAND_T* ct)
{
	if ((int)ct->user == 0)
		if (CompressSelectedMidiNotesVelocitiesWnd* w = g_CompSelMidiNotesVelWndMgr.Get())
			return w->IsValidWindow();
	return 0;
}



///////////////////////////////////////////////////////////////////////////////////////////////////
// Mixer
///////////////////////////////////////////////////////////////////////////////////////////////////
void ScrollMixer(COMMAND_T* ct)
{
	MediaTrack* tr = GetMixerScroll();
	if (tr)
	{
		int count = CountTracks(NULL);
		int i = (int)GetMediaTrackInfo_Value(tr, "IP_TRACKNUMBER") - 1;
		MediaTrack* nTr = NULL;
		if ((int)ct->user == 0)
		{
			for (int k = 1; k <= i; ++k)
			{
				if (IsTrackVisible(GetTrack(NULL, i - k), true))
				{
					nTr = GetTrack(NULL, i - k);
					break;
				}
			}
		}
		else
		{
			for (int k = 1; k < count - i; ++k)
			{
				if (IsTrackVisible(GetTrack(NULL, i + k), true))
				{
					nTr = GetTrack(NULL, i + k);
					break;
				}
			}
		}
		if (nTr && nTr != tr)
			SetMixerScroll(nTr);
	}
}



///////////////////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void wol_MiscInit()
{
	for (int i = 0; i < 8; ++i)
	{
		g_RandomizerSlots[i].min = GetIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.RandomizeSelectedMidiNotesVelocitiesMin[i], 1);
		g_RandomizerSlots[i].max = GetIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.RandomizeSelectedMidiNotesVelocitiesMax[i], 127);
		g_MidiVelocitiesAddSlots[i].min = GetIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.SelectNotesByVelocitiesInRangeMinAdd[i], 1);
		g_MidiVelocitiesAddSlots[i].max = GetIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.SelectNotesByVelocitiesInRangeMaxAdd[i], 127);
		g_MidiVelocitiesClearSlots[i].min = GetIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.SelectNotesByVelocitiesInRangeMinClear[i], 1);
		g_MidiVelocitiesClearSlots[i].max = GetIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.SelectNotesByVelocitiesInRangeMaxClear[i], 127);
		g_SelRandMidiNotesPercentAdd[i] = GetIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.SelectRandomMidiNotesPercentAdd[i], 50);
		g_SelRandMidiNotesPercentClear[i] = GetIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.SelectRandomMidiNotesPercentClear[i], 50);
		g_SelRandMidiNotesPercentAmong[i] = GetIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.SelectRandomMidiNotesPercentAmong[i], 50);
		g_CompSelMidiNotesCoeffMean[i] = GetIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.CompressSelectedMidiNotesVelocitiesMeanCoeff[i], 5);
		g_CompSelMidiNotesCoeffMedian[i] = GetIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.CompressSelectedMidiNotesVelocitiesMedianCoeff[i], 5);
		g_CompSelMidiNotesCoeffVelocity[i].coeff = GetIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.CompressSelectedMidiNotesVelocitiesVelocityCoeff[i], 5);
		g_CompSelMidiNotesCoeffVelocity[i].velocity = GetIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.CompressSelectedMidiNotesVelocitiesVelocityVel[i], 64);
	}

	g_realtimeRandomize = GetIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.EnableRealtimeRandomizeSelectedMidiNotesVelocities, false);
	g_realtimeNoteSelByVelInRange = GetIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.EnableRealtimeNoteSelectionByVelocityInRange, false);
	g_addNotesToSel = GetIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.SelectNotesByVelocityInRangeType, true);
	g_realtimeRandomNoteSelPerc = GetIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.EnableRealtimeRandomNoteSelectionPerc, false);
	g_selType = GetIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.SelectRandomMidiNotesPercType, 0);
	g_targetVel = GetIniSettings(wol_Misc_Ini.SectionMidi, wol_Misc_Ini.CompressSelectedMidiNotesVelocitiesTowards, 0);

	g_RandMidiVelWndMgr.Init();
	g_SelMidiNotesByVelInRangeWndMgr.Init();
	g_SelRandMidiNotesPercWndMgr.Init();
	g_CompSelMidiNotesVelWndMgr.Init();
}

void wol_MiscExit()
{
	g_RandMidiVelWndMgr.Delete();
	g_SelMidiNotesByVelInRangeWndMgr.Delete();
	g_SelRandMidiNotesPercWndMgr.Delete();
	g_CompSelMidiNotesVelWndMgr.Delete();
}