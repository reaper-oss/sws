/*****************************************************************************
/ BR.cpp
/
/ Copyright (c) 2012-2014 Dominik Martin Drzic
/ http://forum.cockos.com/member.php?u=27094
/ http://www.standingwaterstudios.com/reaper
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
#include "BR.h"
#include "BR_Envelope.h"
#include "BR_Misc.h"
#include "BR_ProjState.h"
#include "BR_Tempo.h"
#include "BR_Update.h"
#include "../reaper/localize.h"

//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] =
{
	/******************************************************************************
	* Misc                                                                        *
	******************************************************************************/
	{ { DEFACCEL, "SWS/BR: Split selected items at tempo markers" },                                               "SWS_BRSPLITSELECTEDTEMPO",        SplitItemAtTempo},
	{ { DEFACCEL, "SWS/BR: Create project markers from selected tempo markers" },                                  "BR_TEMPO_TO_MARKERS",             MarkersAtTempo},
	{ { DEFACCEL, "SWS/BR: Enable \"Ignore project tempo\" for selected MIDI items (use tempo at item's start)" }, "BR_MIDI_PROJ_TEMPO_ENB",          MidiItemTempo, NULL, 0},
	{ { DEFACCEL, "SWS/BR: Disable \"Ignore project tempo\" for selected MIDI items" },                            "BR_MIDI_PROJ_TEMPO_DIS",          MidiItemTempo, NULL, 1},

	{ { DEFACCEL, "SWS/BR: Create project markers from notes in selected MIDI items" },                            "BR_MIDI_NOTES_TO_MARKERS",        MarkersAtNotes},
	{ { DEFACCEL, "SWS/BR: Create project markers from selected items (name by item's notes)" },                   "BR_ITEMS_TO_MARKERS_NOTES",       MarkersRegionsAtItems, NULL, 0},
	{ { DEFACCEL, "SWS/BR: Create regions from selected items (name by item's notes)" },                           "BR_ITEMS_TO_REGIONS_NOTES",       MarkersRegionsAtItems, NULL, 1},

	{ { DEFACCEL, "SWS/BR: Toggle \"Grid snap settings follow grid visibility\"" },                                "BR_OPTIONS_SNAP_FOLLOW_GRID_VIS", SnapFollowsGridVis, NULL, 0, IsSnapFollowsGridVisOn},
	{ { DEFACCEL, "SWS/BR: Set \"Apply trim when adding volume/pan envelopes\" to \"Always\"" },                   "BR_OPTIONS_ENV_TRIM_ALWAYS",      TrimNewVolPanEnvs, NULL, 0, IsTrimNewVolPanEnvsOn},
	{ { DEFACCEL, "SWS/BR: Set \"Apply trim when adding volume/pan envelopes\" to \"In read/write\"" },            "BR_OPTIONS_ENV_TRIM_READWRITE",   TrimNewVolPanEnvs, NULL, 1, IsTrimNewVolPanEnvsOn},
	{ { DEFACCEL, "SWS/BR: Set \"Apply trim when adding volume/pan envelopes\" to \"Never\"" },                    "BR_OPTIONS_ENV_TRIM_NEVER",       TrimNewVolPanEnvs, NULL, 2, IsTrimNewVolPanEnvsOn},

	{ { DEFACCEL, "SWS/BR: Cycle through record modes" },                                                          "BR_CYCLE_RECORD_MODES",           CycleRecordModes},
	{ { DEFACCEL, "SWS/BR: Focus arrange window" },                                                                "BR_FOCUS_ARRANGE_WND",            FocusArrange},

	{ { DEFACCEL, "SWS/BR: Save edit cursor position, slot 1" },                                                   "BR_SAVE_CURSOR_POS_SLOT_1",       SaveCursorPosSlot, NULL, 0},
	{ { DEFACCEL, "SWS/BR: Save edit cursor position, slot 2" },                                                   "BR_SAVE_CURSOR_POS_SLOT_2",       SaveCursorPosSlot, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Save edit cursor position, slot 3" },                                                   "BR_SAVE_CURSOR_POS_SLOT_3",       SaveCursorPosSlot, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Save edit cursor position, slot 4" },                                                   "BR_SAVE_CURSOR_POS_SLOT_4",       SaveCursorPosSlot, NULL, 3},
	{ { DEFACCEL, "SWS/BR: Save edit cursor position, slot 5" },                                                   "BR_SAVE_CURSOR_POS_SLOT_5",       SaveCursorPosSlot, NULL, 4},
	{ { DEFACCEL, "SWS/BR: Restore edit cursor position, slot 1" },                                                "BR_RESTORE_CURSOR_POS_SLOT_1",    RestoreCursorPosSlot, NULL, 0},
	{ { DEFACCEL, "SWS/BR: Restore edit cursor position, slot 2" },                                                "BR_RESTORE_CURSOR_POS_SLOT_2",    RestoreCursorPosSlot, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Restore edit cursor position, slot 3" },                                                "BR_RESTORE_CURSOR_POS_SLOT_3",    RestoreCursorPosSlot, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Restore edit cursor position, slot 4" },                                                "BR_RESTORE_CURSOR_POS_SLOT_4",    RestoreCursorPosSlot, NULL, 3},
	{ { DEFACCEL, "SWS/BR: Restore edit cursor position, slot 5" },                                                "BR_RESTORE_CURSOR_POS_SLOT_5",    RestoreCursorPosSlot, NULL, 4},

	/******************************************************************************
	* Media item preview                                                          *
	******************************************************************************/
	{ { DEFACCEL, "SWS/BR: Preview media item under mouse" },                                                                                    "BR_PREV_ITEM_CURSOR",             PreviewItemAtMouse, NULL, 1111},
	{ { DEFACCEL, "SWS/BR: Preview media item under mouse (start from cursor position)" },                                                       "BR_PREV_ITEM_CURSOR_POS",         PreviewItemAtMouse, NULL, 1121},
	{ { DEFACCEL, "SWS/BR: Preview media item under mouse (sync with next measure)" },                                                           "BR_PREV_ITEM_CURSOR_SYNC",        PreviewItemAtMouse, NULL, 1131},
	{ { DEFACCEL, "SWS/BR: Preview media item under mouse at track fader volume" },                                                              "BR_PREV_ITEM_CURSOR_FADER",       PreviewItemAtMouse, NULL, 1211},
	{ { DEFACCEL, "SWS/BR: Preview media item under mouse at track fader volume (start from cursor position)" },                                 "BR_PREV_ITEM_CURSOR_FADER_POS",   PreviewItemAtMouse, NULL, 1221},
	{ { DEFACCEL, "SWS/BR: Preview media item under mouse at track fader volume (sync with next measure)" },                                     "BR_PREV_ITEM_CURSOR_FADER_SYNC",  PreviewItemAtMouse, NULL, 1231},
	{ { DEFACCEL, "SWS/BR: Preview media item under mouse through track" },                                                                      "BR_PREV_ITEM_CURSOR_TRACK",       PreviewItemAtMouse, NULL, 1311},
	{ { DEFACCEL, "SWS/BR: Preview media item under mouse through track (start from cursor position)" },                                         "BR_PREV_ITEM_CURSOR_TRACK_POS",   PreviewItemAtMouse, NULL, 1321},
	{ { DEFACCEL, "SWS/BR: Preview media item under mouse through track (sync with next measure)" },                                             "BR_PREV_ITEM_CURSOR_TRACK_SYNC",  PreviewItemAtMouse, NULL, 1331},
	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse" },                                                                             "BR_TPREV_ITEM_CURSOR",            PreviewItemAtMouse, NULL, 2111},
	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse (start from cursor position)" },                                                "BR_TPREV_ITEM_CURSOR_POS",        PreviewItemAtMouse, NULL, 2121},
	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse (sync with next measure)" },                                                    "BR_TPREV_ITEM_CURSOR_SYNC",       PreviewItemAtMouse, NULL, 2131},
	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse at track fader volume" },                                                       "BR_TPREV_ITEM_CURSOR_FADER",      PreviewItemAtMouse, NULL, 2211},
	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse at track fader volume (start from cursor position)" },                          "BR_TPREV_ITEM_CURSOR_FADER_POS",  PreviewItemAtMouse, NULL, 2221},
	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse at track fader volume (sync with next measure)" },                              "BR_TPREV_ITEM_CURSOR_FADER_SYNC", PreviewItemAtMouse, NULL, 2231},
	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse through track" },                                                               "BR_TPREV_ITEM_CURSOR_TRACK",      PreviewItemAtMouse, NULL, 2311},
	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse through track (start from cursor position)" },                                  "BR_TPREV_ITEM_CURSOR_TRACK_POS",  PreviewItemAtMouse, NULL, 2321},
	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse through track (sync with next measure)" },                                      "BR_TPREV_ITEM_CURSOR_TRACK_SYNC", PreviewItemAtMouse, NULL, 2331},

	{ { DEFACCEL, "SWS/BR: Preview media item under mouse and pause during preview" },                                                           "BR_PREV_ITEM_PAUSE_CURSOR",             PreviewItemAtMouse, NULL, 1112},
	{ { DEFACCEL, "SWS/BR: Preview media item under mouse and pause during preview (start from cursor position)" },                              "BR_PREV_ITEM_PAUSE_CURSOR_POS",         PreviewItemAtMouse, NULL, 1122},
	{ { DEFACCEL, "SWS/BR: Preview media item under mouse at track fader volume and pause during preview" },                                     "BR_PREV_ITEM_PAUSE_CURSOR_FADER",       PreviewItemAtMouse, NULL, 1212},
	{ { DEFACCEL, "SWS/BR: Preview media item under mouse at track fader volume and pause during preview (start from cursor position)" },        "BR_PREV_ITEM_PAUSE_CURSOR_FADER_POS",   PreviewItemAtMouse, NULL, 1222},
	{ { DEFACCEL, "SWS/BR: Preview media item under mouse through track and pause during preview" },                                             "BR_PREV_ITEM_PAUSE_CURSOR_TRACK",       PreviewItemAtMouse, NULL, 1312},
	{ { DEFACCEL, "SWS/BR: Preview media item under mouse through track and pause during preview (start from cursor position)" },                "BR_PREV_ITEM_PAUSE_CURSOR_TRACK_POS",   PreviewItemAtMouse, NULL, 1322},
	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse and pause during preview" },                                                    "BR_TPREV_ITEM_PAUSE_CURSOR",            PreviewItemAtMouse, NULL, 2112},
	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse and pause during preview (start from cursor position)" },                       "BR_TPREV_ITEM_PAUSE_CURSOR_POS",        PreviewItemAtMouse, NULL, 2122},
	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse at track fader volume and pause during preview" },                              "BR_TPREV_ITEM_PAUSE_CURSOR_FADER",      PreviewItemAtMouse, NULL, 2212},
	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse at track fader volume and pause during preview (start from cursor position)" }, "BR_TPREV_ITEM_PAUSE_CURSOR_FADER_POS",  PreviewItemAtMouse, NULL, 2222},
	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse through track and pause during preview" },                                      "BR_TPREV_ITEM_PAUSE_CURSOR_TRACK",      PreviewItemAtMouse, NULL, 2312},
	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse through track and pause during preview (start from cursor position)" },         "BR_TPREV_ITEM_PAUSE_CURSOR_TRACK_POS",  PreviewItemAtMouse, NULL, 2322},

	/******************************************************************************
	* Envelopes                                                                   *
	******************************************************************************/
	{ { DEFACCEL, "SWS/BR: Move edit cursor to next envelope point" },                          "SWS_BRMOVEEDITTONEXTENV",        CursorToEnv1, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Move edit cursor to next envelope point and select it" },            "SWS_BRMOVEEDITSELNEXTENV",       CursorToEnv1, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Move edit cursor to next envelope point and add to selection" },     "SWS_BRMOVEEDITTONEXTENVADDSELL", CursorToEnv2, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Move edit cursor to previous envelope point" },                      "SWS_BRMOVEEDITTOPREVENV",        CursorToEnv1, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Move edit cursor to previous envelope point and select it" },        "SWS_BRMOVEEDITSELPREVENV",       CursorToEnv1, NULL, -2},
	{ { DEFACCEL, "SWS/BR: Move edit cursor to previous envelope point and add to selection" }, "SWS_BRMOVEEDITTOPREVENVADDSELL", CursorToEnv2, NULL, -1},

	{ { DEFACCEL, "SWS/BR: Select next envelope point" },                                       "BR_ENV_SEL_NEXT_POINT",          SelNextPrevEnvPoint, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Select previous envelope point" },                                   "BR_ENV_SEL_PREV_POINT",          SelNextPrevEnvPoint, NULL, -1},

	{ { DEFACCEL, "SWS/BR: Expand envelope point selection to the right" },                     "BR_ENV_SEL_EXPAND_RIGHT",        ExpandEnvSel,    NULL, 1},
	{ { DEFACCEL, "SWS/BR: Expand envelope point selection to the left" },                      "BR_ENV_SEL_EXPAND_LEFT",         ExpandEnvSel,    NULL, -1},
	{ { DEFACCEL, "SWS/BR: Expand envelope point selection to the right (end point only)" },    "BR_ENV_SEL_EXPAND_RIGHT_END",    ExpandEnvSelEnd, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Expand envelope point selection to the left (end point only)" },     "BR_ENV_SEL_EXPAND_L_END",        ExpandEnvSelEnd, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Shrink envelope point selection from the right" },                   "BR_ENV_SEL_SHRINK_RIGHT",        ShrinkEnvSel,    NULL, 1},
	{ { DEFACCEL, "SWS/BR: Shrink envelope point selection from the left" },                    "BR_ENV_SEL_SHRINK_LEFT",         ShrinkEnvSel,    NULL, -1},
	{ { DEFACCEL, "SWS/BR: Shrink envelope point selection from the right (end point only)" },  "BR_ENV_SEL_SHRINK_RIGHT_END",    ShrinkEnvSelEnd, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Shrink envelope point selection from the left (end point only)" },   "BR_ENV_SEL_SHRINK_LEFT_END",     ShrinkEnvSelEnd, NULL, -1},

	{ { DEFACCEL, "SWS/BR: Shift envelope point selection left" },                              "BR_ENV_SHIFT_SEL_LEFT",          ShiftEnvSelection, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Shift envelope point selection right" },                             "BR_ENV_SHIFT_SEL_RIGHT",         ShiftEnvSelection, NULL, 1},

	{ { DEFACCEL, "SWS/BR: Select peaks in envelope (add to selection)" },                      "BR_ENV_SEL_PEAKS_ADD",           PeaksDipsEnv, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Select peaks in envelope" },                                         "BR_ENV_SEL_PEAKS",               PeaksDipsEnv, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Select dips in envelope (add to selection)" },                       "BR_ENV_SEL_DIPS_ADD",            PeaksDipsEnv, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Select dips in envelope" },                                          "BR_ENV_SEL_DIPS",                PeaksDipsEnv, NULL, -2},

	{ { DEFACCEL, "SWS/BR: Unselect envelope points outside time selection" },                  "BR_ENV_UNSEL_OUT_TIME_SEL",      SelEnvTimeSel, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Unselect envelope points in time selection" },                       "BR_ENV_UNSEL_IN_TIME_SEL",       SelEnvTimeSel, NULL, 1},

	{ { DEFACCEL, "SWS/BR: Set selected envelope points to next point's value" },               "BR_SET_ENV_TO_NEXT_VAL",         SetEnvValToNextPrev, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Set selected envelope points to previous point's value" },           "BR_SET_ENV_TO_PREV_VAL",         SetEnvValToNextPrev, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Set selected envelope points to last selected point's value" },      "BR_SET_ENV_TO_LAST_SEL_VAL",     SetEnvValToNextPrev, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Set selected envelope points to first selected point's value" },     "BR_SET_ENV_TO_FIRST_SEL_VAL",    SetEnvValToNextPrev, NULL, -2},

	{ { DEFACCEL, "SWS/BR: Hide all but active envelope for all tracks" },                      "BR_ENV_HIDE_ALL_BUT_ACTIVE",     ShowActiveEnvOnly, NULL, 0},
	{ { DEFACCEL, "SWS/BR: Hide all but active envelope for selected tracks" },                 "BR_ENV_HIDE_ALL_BUT_ACTIVE_SEL", ShowActiveEnvOnly, NULL, 1},

	{ { DEFACCEL, "SWS/BR: Insert new envelope point at mouse cursor" },                        "BR_ENV_POINT_MOUSE_CURSOR",      CreateEnvPointMouse, NULL},

	{ { DEFACCEL, "SWS/BR: Save envelope point selection, slot 1" },                            "BR_SAVE_ENV_SEL_SLOT_1",         SaveEnvSelSlot, NULL, 0},
	{ { DEFACCEL, "SWS/BR: Save envelope point selection, slot 2" },                            "BR_SAVE_ENV_SEL_SLOT_2",         SaveEnvSelSlot, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Save envelope point selection, slot 3" },                            "BR_SAVE_ENV_SEL_SLOT_3",         SaveEnvSelSlot, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Save envelope point selection, slot 4" },                            "BR_SAVE_ENV_SEL_SLOT_4",         SaveEnvSelSlot, NULL, 3},
	{ { DEFACCEL, "SWS/BR: Save envelope point selection, slot 5" },                            "BR_SAVE_ENV_SEL_SLOT_5",         SaveEnvSelSlot, NULL, 4},
	{ { DEFACCEL, "SWS/BR: Restore envelope point selection, slot 1" },                         "BR_RESTORE_ENV_SEL_SLOT_1",      RestoreEnvSelSlot, NULL, 0},
	{ { DEFACCEL, "SWS/BR: Restore envelope point selection, slot 2" },                         "BR_RESTORE_ENV_SEL_SLOT_2",      RestoreEnvSelSlot, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Restore envelope point selection, slot 3" },                         "BR_RESTORE_ENV_SEL_SLOT_3",      RestoreEnvSelSlot, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Restore envelope point selection, slot 4" },                         "BR_RESTORE_ENV_SEL_SLOT_4",      RestoreEnvSelSlot, NULL, 3},
	{ { DEFACCEL, "SWS/BR: Restore envelope point selection, slot 5" },                         "BR_RESTORE_ENV_SEL_SLOT_5",      RestoreEnvSelSlot, NULL, 4},

	/******************************************************************************
	* Tempo                                                                       *
	******************************************************************************/
	{ { DEFACCEL, "SWS/BR: Move tempo marker forward 0.1 ms" },                                                               "SWS_BRMOVETEMPOFORWARD01",    MoveTempo, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Move tempo marker forward 1 ms" },                                                                 "SWS_BRMOVETEMPOFORWARD1",     MoveTempo, NULL, 10},
	{ { DEFACCEL, "SWS/BR: Move tempo marker forward 10 ms"},                                                                 "SWS_BRMOVETEMPOFORWARD10",    MoveTempo, NULL, 100},
	{ { DEFACCEL, "SWS/BR: Move tempo marker forward 100 ms"},                                                                "SWS_BRMOVETEMPOFORWARD100",   MoveTempo, NULL, 1000},
	{ { DEFACCEL, "SWS/BR: Move tempo marker forward 1000 ms" },                                                              "SWS_BRMOVETEMPOFORWARD1000",  MoveTempo, NULL, 10000},
	{ { DEFACCEL, "SWS/BR: Move tempo marker back 0.1 ms"},                                                                   "SWS_BRMOVETEMPOBACK01",       MoveTempo, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Move tempo marker back 1 ms"},                                                                     "SWS_BRMOVETEMPOBACK1",        MoveTempo, NULL, -10},
	{ { DEFACCEL, "SWS/BR: Move tempo marker back 10 ms"},                                                                    "SWS_BRMOVETEMPOBACK10",       MoveTempo, NULL, -100},
	{ { DEFACCEL, "SWS/BR: Move tempo marker back 100 ms"},                                                                   "SWS_BRMOVETEMPOBACK100",      MoveTempo, NULL, -1000},
	{ { DEFACCEL, "SWS/BR: Move tempo marker back 1000 ms"},                                                                  "SWS_BRMOVETEMPOBACK1000",     MoveTempo, NULL, -10000},
	{ { DEFACCEL, "SWS/BR: Move tempo marker forward" },                                                                      "SWS_BRMOVETEMPOFORWARD",      MoveTempo, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Move tempo marker back" },                                                                         "SWS_BRMOVETEMPOBACK",         MoveTempo, NULL, -2},

	{ { DEFACCEL, "SWS/BR: Move closest tempo marker to edit cursor" },                                                       "BR_MOVE_CLOSEST_TEMPO",       MoveTempo, NULL, 3},
	{ { DEFACCEL, "SWS/BR: Move closest tempo marker to mouse cursor" },                                                      "BR_MOVE_CLOSEST_TEMPO_MOUSE", MoveTempo, NULL, 4},
	{ { DEFACCEL, "SWS/BR: Move closest grid line to mouse cursor (may create tempo marker)" },                               "BR_MOVE_GRID_TO_MOUSE",       MoveTempo, NULL, 5},
	{ { DEFACCEL, "SWS/BR: Move closest measure grid line to mouse cursor (may create tempo marker)" },                       "BR_MOVE_M_GRID_TO_MOUSE",     MoveTempo, NULL, 6},

	{ { DEFACCEL, "SWS/BR: Increase tempo marker 0.001 BPM (preserve overall tempo)" },                                       "BR_INC_TEMPO_0.001_BPM",      EditTempo, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Increase tempo marker 0.01 BPM (preserve overall tempo)" },                                        "BR_INC_TEMPO_0.01_BPM",       EditTempo, NULL, 10},
	{ { DEFACCEL, "SWS/BR: Increase tempo marker 0.1 BPM (preserve overall tempo)" },                                         "BR_INC_TEMPO_0.1_BPM",        EditTempo, NULL, 100},
	{ { DEFACCEL, "SWS/BR: Increase tempo marker 01 BPM (preserve overall tempo)" },                                          "BR_INC_TEMPO_1_BPM",          EditTempo, NULL, 1000},
	{ { DEFACCEL, "SWS/BR: Decrease tempo marker 0.001 BPM (preserve overall tempo)" },                                       "BR_DEC_TEMPO_0.001_BPM",      EditTempo, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Decrease tempo marker 0.01 BPM (preserve overall tempo)" },                                        "BR_DEC_TEMPO_0.01_BPM",       EditTempo, NULL, -10},
	{ { DEFACCEL, "SWS/BR: Decrease tempo marker 0.1 BPM (preserve overall tempo)" },                                         "BR_DEC_TEMPO_0.1_BPM",        EditTempo, NULL, -100},
	{ { DEFACCEL, "SWS/BR: Decrease tempo marker 01 BPM (preserve overall tempo)" },                                          "BR_DEC_TEMPO_1_BPM",          EditTempo, NULL, -1000},
	{ { DEFACCEL, "SWS/BR: Increase tempo marker 0.001% (preserve overall tempo)" },                                          "BR_INC_TEMPO_0.001_PERC",     EditTempo, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Increase tempo marker 0.01% (preserve overall tempo)" },                                           "BR_INC_TEMPO_0.01_PERC",      EditTempo, NULL, 20},
	{ { DEFACCEL, "SWS/BR: Increase tempo marker 0.1% (preserve overall tempo)" },                                            "BR_INC_TEMPO_0.1_PERC",       EditTempo, NULL, 200},
	{ { DEFACCEL, "SWS/BR: Increase tempo marker 01% (preserve overall tempo)" },                                             "BR_INC_TEMPO_1_PERC",         EditTempo, NULL, 2000},
	{ { DEFACCEL, "SWS/BR: Decrease tempo marker 0.001% (preserve overall tempo)" },                                          "BR_DEC_TEMPO_0.001_PERC",     EditTempo, NULL, -2},
	{ { DEFACCEL, "SWS/BR: Decrease tempo marker 0.01% (preserve overall tempo)" },                                           "BR_DEC_TEMPO_0.01_PERC",      EditTempo, NULL, -20},
	{ { DEFACCEL, "SWS/BR: Decrease tempo marker 0.1% (preserve overall tempo)" },                                            "BR_DEC_TEMPO_0.1_PERC",       EditTempo, NULL, -200},
	{ { DEFACCEL, "SWS/BR: Decrease tempo marker 01% (preserve overall tempo)" },                                             "BR_DEC_TEMPO_1_PERC",         EditTempo, NULL, -2000},

	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (increase 0.001 BPM)" },                                       "BR_INC_GR_TEMPO_0.001_BPM",   EditTempoGradual, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (increase 0.01 BPM)" },                                        "BR_INC_GR_TEMPO_0.01_BPM",    EditTempoGradual, NULL, 10},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (increase 0.1 BPM)" },                                         "BR_INC_GR_TEMPO_0.1_BPM",     EditTempoGradual, NULL, 100},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (increase 01 BPM)" },                                          "BR_INC_GR_TEMPO_1_BPM",       EditTempoGradual, NULL, 1000},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (decrease 0.001 BPM)" },                                       "BR_DEC_GR_TEMPO_0.001_BPM",   EditTempoGradual, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (decrease 0.01 BPM)" },                                        "BR_DEC_GR_TEMPO_0.01_BPM",    EditTempoGradual, NULL, -10},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (decrease 0.1 BPM)" },                                         "BR_DEC_GR_TEMPO_0.1_BPM",     EditTempoGradual, NULL, -100},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (decrease 01 BPM)" },                                          "BR_DEC_GR_TEMPO_1_BPM",       EditTempoGradual, NULL, -1000},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (increase 0.001%)" },                                          "BR_INC_GR_TEMPO_0.001_PERC",  EditTempoGradual, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (increase 0.01%)" },                                           "BR_INC_GR_TEMPO_0.01_PERC",   EditTempoGradual, NULL, 20},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (increase 0.1%)" },                                            "BR_INC_GR_TEMPO_0.1_PERC",    EditTempoGradual, NULL, 200},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (increase 01%)" },                                             "BR_INC_GR_TEMPO_1_PERC",      EditTempoGradual, NULL, 2000},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (decrease 0.001%)" },                                          "BR_DEC_GR_TEMPO_0.001_PERC",  EditTempoGradual, NULL, -2},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (decrease 0.01%)" },                                           "BR_DEC_GR_TEMPO_0.01_PERC",   EditTempoGradual, NULL, -20},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (decrease 0.1%)" },                                            "BR_DEC_GR_TEMPO_0.1_PERC",    EditTempoGradual, NULL, -200},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (decrease 01%)" },                                             "BR_DEC_GR_TEMPO_1_PERC",      EditTempoGradual, NULL, -2000},

	{ { DEFACCEL, "SWS/BR: Delete tempo marker (preserve overall tempo and positions if possible)" },                         "BR_DELETE_TEMPO",             DeleteTempo},
	{ { DEFACCEL, "SWS/BR: Delete tempo marker and preserve position and length of items (including MIDI events)" },          "BR_DELETE_TEMPO_ITEMS",       DeleteTempoPreserveItems, NULL, 0},
	{ { DEFACCEL, "SWS/BR: Delete tempo marker and preserve position and length of selected items (including MIDI events)" }, "BR_DELETE_TEMPO_ITEMS_SEL",   DeleteTempoPreserveItems, NULL, 1},

	{ { DEFACCEL, "SWS/BR: Create tempo markers at grid after every selected tempo marker" },                                 "BR_TEMPO_GRID",               TempoAtGrid},

	{ { DEFACCEL, "SWS/BR: Convert project markers to tempo markers..." },                                                    "SWS_BRCONVERTMARKERSTOTEMPO", ConvertMarkersToTempoDialog, NULL, 0, IsConvertMarkersToTempoVisible},
	{ { DEFACCEL, "SWS/BR: Select and adjust tempo markers..." },                                                             "SWS_BRADJUSTSELTEMPO",        SelectAdjustTempoDialog, NULL, 0, IsSelectAdjustTempoVisible},
	{ { DEFACCEL, "SWS/BR: Randomize tempo markers..." },                                                                     "BR_RANDOMIZE_TEMPO",          RandomizeTempoDialog},

	{ { DEFACCEL, "SWS/BR: Set tempo marker shape (options)..." },                                                            "BR_TEMPO_SHAPE_OPTIONS",      TempoShapeOptionsDialog, NULL, 0, IsTempoShapeOptionsVisible},
	{ { DEFACCEL, "SWS/BR: Set tempo marker shape to linear (preserve positions)" },                                          "BR_TEMPO_SHAPE_LINEAR",       TempoShapeLinear},
	{ { DEFACCEL, "SWS/BR: Set tempo marker shape to square (preserve positions)" },                                          "BR_TEMPO_SHAPE_SQUARE",       TempoShapeSquare},

	{ {}, LAST_COMMAND, },
};
//!WANT_LOCALIZE_1ST_STRING_END

int BR_Init()
{
	SWSRegisterCommands(g_commandTable);
	ProjStateInit();
	VersionCheckInit();
	return 1;
}