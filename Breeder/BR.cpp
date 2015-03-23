/*****************************************************************************
/ BR.cpp
/
/ Copyright (c) 2012-2015 Dominik Martin Drzic
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
#include "BR.h"
#include "BR_ContextualToolbars.h"
#include "BR_ContinuousActions.h"
#include "BR_Envelope.h"
#include "BR_Loudness.h"
#include "BR_MidiEditor.h"
#include "BR_Misc.h"
#include "BR_ProjState.h"
#include "BR_Tempo.h"
#include "BR_Update.h"
#include "BR_Util.h"
#include "../SnM/SnM.h"
#include "../reaper/localize.h"

//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] =
{
	/******************************************************************************
	* Contextual toolbars                                                         *
	******************************************************************************/
	{ { DEFACCEL, "SWS/BR: Contextual toolbars..." }, "BR_CONTEXTUAL_TOOLBARS_PREF", ContextualToolbarsOptions, NULL, 0, IsContextualToolbarsOptionsVisible},

	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 1" }, "BR_CONTEXTUAL_TOOLBAR_01",              ToggleContextualToolbar, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 1" }, "BR_ME_CONTEXTUAL_TOOLBAR_01",           NULL, NULL, 1, NULL, SECTION_MIDI_EDITOR,     ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 1" }, "BR_ML_CONTEXTUAL_TOOLBAR_01",           NULL, NULL, 1, NULL, SECTION_MIDI_LIST,       ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 1" }, "BR_MI_CONTEXTUAL_TOOLBAR_01",           NULL, NULL, 1, NULL, SECTION_MIDI_INLINE,     ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 1" }, "BR_MX_CONTEXTUAL_TOOLBAR_01",           NULL, NULL, 1, NULL, SECTION_MEDIA_EXPLORER,  ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 2" }, "BR_CONTEXTUAL_TOOLBAR_02",              ToggleContextualToolbar, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 2" }, "BR_ME_CONTEXTUAL_TOOLBAR_02",           NULL, NULL, 2, NULL, SECTION_MIDI_EDITOR,     ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 2" }, "BR_ML_CONTEXTUAL_TOOLBAR_02",           NULL, NULL, 2, NULL, SECTION_MIDI_LIST,       ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 2" }, "BR_MI_CONTEXTUAL_TOOLBAR_02",           NULL, NULL, 2, NULL, SECTION_MIDI_INLINE,     ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 2" }, "BR_MX_CONTEXTUAL_TOOLBAR_02",           NULL, NULL, 2, NULL, SECTION_MEDIA_EXPLORER,  ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 3" }, "BR_CONTEXTUAL_TOOLBAR_03",              ToggleContextualToolbar, NULL, 3},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 3" }, "BR_ME_CONTEXTUAL_TOOLBAR_03",           NULL, NULL, 3, NULL, SECTION_MIDI_EDITOR,     ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 3" }, "BR_ML_CONTEXTUAL_TOOLBAR_03",           NULL, NULL, 3, NULL, SECTION_MIDI_LIST,       ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 3" }, "BR_MI_CONTEXTUAL_TOOLBAR_03",           NULL, NULL, 3, NULL, SECTION_MIDI_INLINE,     ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 3" }, "BR_MX_CONTEXTUAL_TOOLBAR_03",           NULL, NULL, 3, NULL, SECTION_MEDIA_EXPLORER,  ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 4" }, "BR_CONTEXTUAL_TOOLBAR_04",              ToggleContextualToolbar, NULL, 4},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 4" }, "BR_ME_CONTEXTUAL_TOOLBAR_04",           NULL, NULL, 4, NULL, SECTION_MIDI_EDITOR,     ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 4" }, "BR_ML_CONTEXTUAL_TOOLBAR_04",           NULL, NULL, 4, NULL, SECTION_MIDI_LIST,       ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 4" }, "BR_MI_CONTEXTUAL_TOOLBAR_04",           NULL, NULL, 4, NULL, SECTION_MIDI_INLINE,     ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 4" }, "BR_MX_CONTEXTUAL_TOOLBAR_04",           NULL, NULL, 4, NULL, SECTION_MEDIA_EXPLORER,  ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 5" }, "BR_CONTEXTUAL_TOOLBAR_05",              ToggleContextualToolbar, NULL, 5},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 5" }, "BR_ME_CONTEXTUAL_TOOLBAR_05",           NULL, NULL, 5, NULL, SECTION_MIDI_EDITOR,     ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 5" }, "BR_ML_CONTEXTUAL_TOOLBAR_05",           NULL, NULL, 5, NULL, SECTION_MIDI_LIST,       ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 5" }, "BR_MI_CONTEXTUAL_TOOLBAR_05",           NULL, NULL, 5, NULL, SECTION_MIDI_INLINE,     ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 5" }, "BR_MX_CONTEXTUAL_TOOLBAR_05",           NULL, NULL, 5, NULL, SECTION_MEDIA_EXPLORER,  ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 6" }, "BR_CONTEXTUAL_TOOLBAR_06",              ToggleContextualToolbar, NULL, 6},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 6" }, "BR_ME_CONTEXTUAL_TOOLBAR_06",           NULL, NULL, 6, NULL, SECTION_MIDI_EDITOR,     ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 6" }, "BR_ML_CONTEXTUAL_TOOLBAR_06",           NULL, NULL, 6, NULL, SECTION_MIDI_LIST,       ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 6" }, "BR_MI_CONTEXTUAL_TOOLBAR_06",           NULL, NULL, 6, NULL, SECTION_MIDI_INLINE,     ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 6" }, "BR_MX_CONTEXTUAL_TOOLBAR_06",           NULL, NULL, 6, NULL, SECTION_MEDIA_EXPLORER,  ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 7" }, "BR_CONTEXTUAL_TOOLBAR_07",              ToggleContextualToolbar, NULL, 7},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 7" }, "BR_ME_CONTEXTUAL_TOOLBAR_07",           NULL, NULL, 7, NULL, SECTION_MIDI_EDITOR,     ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 7" }, "BR_ML_CONTEXTUAL_TOOLBAR_07",           NULL, NULL, 7, NULL, SECTION_MIDI_LIST,       ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 7" }, "BR_MI_CONTEXTUAL_TOOLBAR_07",           NULL, NULL, 7, NULL, SECTION_MIDI_INLINE,     ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 7" }, "BR_MX_CONTEXTUAL_TOOLBAR_07",           NULL, NULL, 7, NULL, SECTION_MEDIA_EXPLORER,  ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 8" }, "BR_CONTEXTUAL_TOOLBAR_08",              ToggleContextualToolbar, NULL, 8},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 8" }, "BR_ME_CONTEXTUAL_TOOLBAR_08",           NULL, NULL, 8, NULL, SECTION_MIDI_EDITOR,     ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 8" }, "BR_ML_CONTEXTUAL_TOOLBAR_08",           NULL, NULL, 8, NULL, SECTION_MIDI_LIST,       ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 8" }, "BR_MI_CONTEXTUAL_TOOLBAR_08",           NULL, NULL, 8, NULL, SECTION_MIDI_INLINE,     ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Open/close contextual toolbar under mouse cursor, preset 8" }, "BR_MX_CONTEXTUAL_TOOLBAR_08",           NULL, NULL, 8, NULL, SECTION_MEDIA_EXPLORER,  ToggleContextualToolbar},

	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 1" }, "BR_EXC_CONTEXTUAL_TOOLBAR_01",    ToggleContextualToolbar, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 1" }, "BR_ME_EXC_CONTEXTUAL_TOOLBAR_01", NULL, NULL, -1, NULL, SECTION_MIDI_EDITOR,    ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 1" }, "BR_ML_EXC_CONTEXTUAL_TOOLBAR_01", NULL, NULL, -1, NULL, SECTION_MIDI_LIST,      ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 1" }, "BR_MI_EXC_CONTEXTUAL_TOOLBAR_01", NULL, NULL, -1, NULL, SECTION_MIDI_INLINE,    ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 1" }, "BR_MX_EXC_CONTEXTUAL_TOOLBAR_01", NULL, NULL, -1, NULL, SECTION_MEDIA_EXPLORER, ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 2" }, "BR_EXC_CONTEXTUAL_TOOLBAR_02",    ToggleContextualToolbar, NULL, -2},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 2" }, "BR_ME_EXC_CONTEXTUAL_TOOLBAR_02", NULL, NULL, -2, NULL, SECTION_MIDI_EDITOR,    ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 2" }, "BR_ML_EXC_CONTEXTUAL_TOOLBAR_02", NULL, NULL, -2, NULL, SECTION_MIDI_LIST,      ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 2" }, "BR_MI_EXC_CONTEXTUAL_TOOLBAR_02", NULL, NULL, -2, NULL, SECTION_MIDI_INLINE,    ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 2" }, "BR_MX_EXC_CONTEXTUAL_TOOLBAR_02", NULL, NULL, -2, NULL, SECTION_MEDIA_EXPLORER, ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 3" }, "BR_EXC_CONTEXTUAL_TOOLBAR_03",    ToggleContextualToolbar, NULL, -3},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 3" }, "BR_ME_EXC_CONTEXTUAL_TOOLBAR_03", NULL, NULL, -3, NULL, SECTION_MIDI_EDITOR,    ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 3" }, "BR_ML_EXC_CONTEXTUAL_TOOLBAR_03", NULL, NULL, -3, NULL, SECTION_MIDI_LIST,      ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 3" }, "BR_MI_EXC_CONTEXTUAL_TOOLBAR_03", NULL, NULL, -3, NULL, SECTION_MIDI_INLINE,    ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 3" }, "BR_MX_EXC_CONTEXTUAL_TOOLBAR_03", NULL, NULL, -3, NULL, SECTION_MEDIA_EXPLORER, ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 4" }, "BR_EXC_CONTEXTUAL_TOOLBAR_04",    ToggleContextualToolbar, NULL, -4},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 4" }, "BR_ME_EXC_CONTEXTUAL_TOOLBAR_04", NULL, NULL, -4, NULL, SECTION_MIDI_EDITOR,    ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 4" }, "BR_ML_EXC_CONTEXTUAL_TOOLBAR_04", NULL, NULL, -4, NULL, SECTION_MIDI_LIST,      ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 4" }, "BR_MI_EXC_CONTEXTUAL_TOOLBAR_04", NULL, NULL, -4, NULL, SECTION_MIDI_INLINE,    ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 4" }, "BR_MX_EXC_CONTEXTUAL_TOOLBAR_04", NULL, NULL, -4, NULL, SECTION_MEDIA_EXPLORER, ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 5" }, "BR_EXC_CONTEXTUAL_TOOLBAR_05",    ToggleContextualToolbar, NULL, -5},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 5" }, "BR_ME_EXC_CONTEXTUAL_TOOLBAR_05", NULL, NULL, -5, NULL, SECTION_MIDI_EDITOR,    ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 5" }, "BR_ML_EXC_CONTEXTUAL_TOOLBAR_05", NULL, NULL, -5, NULL, SECTION_MIDI_LIST,      ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 5" }, "BR_MI_EXC_CONTEXTUAL_TOOLBAR_05", NULL, NULL, -5, NULL, SECTION_MIDI_INLINE,    ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 5" }, "BR_MX_EXC_CONTEXTUAL_TOOLBAR_05", NULL, NULL, -5, NULL, SECTION_MEDIA_EXPLORER, ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 6" }, "BR_EXC_CONTEXTUAL_TOOLBAR_06",    ToggleContextualToolbar, NULL, -6},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 6" }, "BR_ME_EXC_CONTEXTUAL_TOOLBAR_06", NULL, NULL, -6, NULL, SECTION_MIDI_EDITOR,    ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 6" }, "BR_ML_EXC_CONTEXTUAL_TOOLBAR_06", NULL, NULL, -6, NULL, SECTION_MIDI_LIST,      ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 6" }, "BR_MI_EXC_CONTEXTUAL_TOOLBAR_06", NULL, NULL, -6, NULL, SECTION_MIDI_INLINE,    ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 6" }, "BR_MX_EXC_CONTEXTUAL_TOOLBAR_06", NULL, NULL, -6, NULL, SECTION_MEDIA_EXPLORER, ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 7" }, "BR_EXC_CONTEXTUAL_TOOLBAR_07",    ToggleContextualToolbar, NULL, -7},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 7" }, "BR_ME_EXC_CONTEXTUAL_TOOLBAR_07", NULL, NULL, -7, NULL, SECTION_MIDI_EDITOR,    ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 7" }, "BR_ML_EXC_CONTEXTUAL_TOOLBAR_07", NULL, NULL, -7, NULL, SECTION_MIDI_LIST,      ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 7" }, "BR_MI_EXC_CONTEXTUAL_TOOLBAR_07", NULL, NULL, -7, NULL, SECTION_MIDI_INLINE,    ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 7" }, "BR_MX_EXC_CONTEXTUAL_TOOLBAR_07", NULL, NULL, -7, NULL, SECTION_MEDIA_EXPLORER, ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 8" }, "BR_EXC_CONTEXTUAL_TOOLBAR_08",    ToggleContextualToolbar, NULL, -8},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 8" }, "BR_ME_EXC_CONTEXTUAL_TOOLBAR_08", NULL, NULL, -8, NULL, SECTION_MIDI_EDITOR,    ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 8" }, "BR_ML_EXC_CONTEXTUAL_TOOLBAR_08", NULL, NULL, -8, NULL, SECTION_MIDI_LIST,      ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 8" }, "BR_MI_EXC_CONTEXTUAL_TOOLBAR_08", NULL, NULL, -8, NULL, SECTION_MIDI_INLINE,    ToggleContextualToolbar},
	{ { DEFACCEL, "SWS/BR: Exclusive toggle contextual toolbar under mouse cursor, preset 8" }, "BR_MX_EXC_CONTEXTUAL_TOOLBAR_08", NULL, NULL, -8, NULL, SECTION_MEDIA_EXPLORER, ToggleContextualToolbar},

	/******************************************************************************
	* Envelopes - Misc                                                            *
	******************************************************************************/
	{ { DEFACCEL, "SWS/BR: Move edit cursor to next envelope point" },                                                                                               "SWS_BRMOVEEDITTONEXTENV",            CursorToEnv1, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Move edit cursor to next envelope point and select it" },                                                                                 "SWS_BRMOVEEDITSELNEXTENV",           CursorToEnv1, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Move edit cursor to next envelope point and add to selection" },                                                                          "SWS_BRMOVEEDITTONEXTENVADDSELL",     CursorToEnv2, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Move edit cursor to previous envelope point" },                                                                                           "SWS_BRMOVEEDITTOPREVENV",            CursorToEnv1, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Move edit cursor to previous envelope point and select it" },                                                                             "SWS_BRMOVEEDITSELPREVENV",           CursorToEnv1, NULL, -2},
	{ { DEFACCEL, "SWS/BR: Move edit cursor to previous envelope point and add to selection" },                                                                      "SWS_BRMOVEEDITTOPREVENVADDSELL",     CursorToEnv2, NULL, -1},

	{ { DEFACCEL, "SWS/BR: Select next envelope point" },                                                                                                            "BR_ENV_SEL_NEXT_POINT",              SelNextPrevEnvPoint, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Select previous envelope point" },                                                                                                        "BR_ENV_SEL_PREV_POINT",              SelNextPrevEnvPoint, NULL, -1},

	{ { DEFACCEL, "SWS/BR: Expand envelope point selection to the right" },                                                                                          "BR_ENV_SEL_EXPAND_RIGHT",            ExpandEnvSel,    NULL, 1},
	{ { DEFACCEL, "SWS/BR: Expand envelope point selection to the left" },                                                                                           "BR_ENV_SEL_EXPAND_LEFT",             ExpandEnvSel,    NULL, -1},
	{ { DEFACCEL, "SWS/BR: Expand envelope point selection to the right (end point only)" },                                                                         "BR_ENV_SEL_EXPAND_RIGHT_END",        ExpandEnvSelEnd, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Expand envelope point selection to the left (end point only)" },                                                                          "BR_ENV_SEL_EXPAND_L_END",            ExpandEnvSelEnd, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Shrink envelope point selection from the right" },                                                                                        "BR_ENV_SEL_SHRINK_RIGHT",            ShrinkEnvSel,    NULL, 1},
	{ { DEFACCEL, "SWS/BR: Shrink envelope point selection from the left" },                                                                                         "BR_ENV_SEL_SHRINK_LEFT",             ShrinkEnvSel,    NULL, -1},
	{ { DEFACCEL, "SWS/BR: Shrink envelope point selection from the right (end point only)" },                                                                       "BR_ENV_SEL_SHRINK_RIGHT_END",        ShrinkEnvSelEnd, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Shrink envelope point selection from the left (end point only)" },                                                                        "BR_ENV_SEL_SHRINK_LEFT_END",         ShrinkEnvSelEnd, NULL, -1},

	{ { DEFACCEL, "SWS/BR: Select envelope points on grid" },                                                                                                        "BR_ENV_SEL_PT_GRID",                 EnvPointsGrid, NULL, BINARY(1100)},
	{ { DEFACCEL, "SWS/BR: Select envelope points on grid (obey time selection, if any)" },                                                                          "BR_ENV_SEL_PT_GRID_TS",              EnvPointsGrid, NULL, BINARY(1101)},
	{ { DEFACCEL, "SWS/BR: Select envelope points between grid" },                                                                                                   "BR_ENV_SEL_PT_NO_GRID",              EnvPointsGrid, NULL, BINARY(0100)},
	{ { DEFACCEL, "SWS/BR: Select envelope points between grid (obey time selection, if any)" },                                                                     "BR_ENV_SEL_PT_NO_GRID_TS",           EnvPointsGrid, NULL, BINARY(0101)},
	{ { DEFACCEL, "SWS/BR: Add envelope points located on grid to existing selection" },                                                                             "BR_ENV_ADD_SEL_PT_GRID",             EnvPointsGrid, NULL, BINARY(1000)},
	{ { DEFACCEL, "SWS/BR: Add envelope points located on grid to existing selection (obey time selection, if any)" },                                               "BR_ENV_ADD_SEL_PT_GRID_TS",          EnvPointsGrid, NULL, BINARY(1001)},
	{ { DEFACCEL, "SWS/BR: Add envelope points located between grid to existing selection" },                                                                        "BR_ENV_ADD_SEL_PT_NO_GRID",          EnvPointsGrid, NULL, BINARY(0000)},
	{ { DEFACCEL, "SWS/BR: Add envelope points located between grid to existing selection (obey time selection, if any)" },                                          "BR_ENV_ADD_SEL_PT_NO_GRID_TS",       EnvPointsGrid, NULL, BINARY(0001)},
	{ { DEFACCEL, "SWS/BR: Delete envelope points on grid" },                                                                                                        "BR_ENV_DEL_PT_GRID",                 EnvPointsGrid, NULL, BINARY(1010)},
	{ { DEFACCEL, "SWS/BR: Delete envelope points on grid (obey time selection, if any)" },                                                                          "BR_ENV_DEL_PT_GRID_TS",              EnvPointsGrid, NULL, BINARY(1011)},
	{ { DEFACCEL, "SWS/BR: Delete envelope points between grid" },                                                                                                   "BR_ENV_DEL_PT_NO_GRID",              EnvPointsGrid, NULL, BINARY(0010)},
	{ { DEFACCEL, "SWS/BR: Delete envelope points between grid (obey time selection, if any)" },                                                                     "BR_ENV_DEL_PT_NO_GRID_TS",           EnvPointsGrid, NULL, BINARY(0011)},
	{ { DEFACCEL, "SWS/BR: Insert envelope points on grid using shape of the previous point" },                                                                      "BR_ENV_INS_PT_GRID",                 CreateEnvPointsGrid, NULL, 0},
	{ { DEFACCEL, "SWS/BR: Insert envelope points on grid using shape of the previous point (obey time selection, if any)" },                                        "BR_ENV_INS_PT_GRID_TS",              CreateEnvPointsGrid, NULL, 1},

	{ { DEFACCEL, "SWS/BR: Shift envelope point selection left" },                                                                                                   "BR_ENV_SHIFT_SEL_LEFT",              ShiftEnvSelection, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Shift envelope point selection right" },                                                                                                  "BR_ENV_SHIFT_SEL_RIGHT",             ShiftEnvSelection, NULL, 1},

	{ { DEFACCEL, "SWS/BR: Select peaks in envelope (add to selection)" },                                                                                           "BR_ENV_SEL_PEAKS_ADD",               PeaksDipsEnv, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Select peaks in envelope" },                                                                                                              "BR_ENV_SEL_PEAKS",                   PeaksDipsEnv, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Select dips in envelope (add to selection)" },                                                                                            "BR_ENV_SEL_DIPS_ADD",                PeaksDipsEnv, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Select dips in envelope" },                                                                                                               "BR_ENV_SEL_DIPS",                    PeaksDipsEnv, NULL, -2},

	{ { DEFACCEL, "SWS/BR: Unselect envelope points outside time selection" },                                                                                       "BR_ENV_UNSEL_OUT_TIME_SEL",          SelEnvTimeSel, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Unselect envelope points in time selection" },                                                                                            "BR_ENV_UNSEL_IN_TIME_SEL",           SelEnvTimeSel, NULL, 1},

	{ { DEFACCEL, "SWS/BR: Set selected envelope points to next point's value" },                                                                                    "BR_SET_ENV_TO_NEXT_VAL",             SetEnvValToNextPrev, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Set selected envelope points to previous point's value" },                                                                                "BR_SET_ENV_TO_PREV_VAL",             SetEnvValToNextPrev, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Set selected envelope points to last selected point's value" },                                                                           "BR_SET_ENV_TO_LAST_SEL_VAL",         SetEnvValToNextPrev, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Set selected envelope points to first selected point's value" },                                                                          "BR_SET_ENV_TO_FIRST_SEL_VAL",        SetEnvValToNextPrev, NULL, -2},

	{ { DEFACCEL, "SWS/BR: Move closest envelope point to edit cursor" },                                                                                            "BR_MOVE_CLOSEST_ENV_ECURSOR",        MoveEnvPointToEditCursor, NULL, 0},
	{ { DEFACCEL, "SWS/BR: Move closest selected envelope point to edit cursor" },                                                                                   "BR_MOVE_CLOSEST_SEL_ENV_ECURSOR",    MoveEnvPointToEditCursor, NULL, 1},

	{ { DEFACCEL, "SWS/BR: Insert 2 envelope points at time selection" },                                                                                            "BR_INSERT_2_ENV_POINT_TIME_SEL",     Insert2EnvPointsTimeSelection, NULL, 0},
	{ { DEFACCEL, "SWS/BR: Insert 2 envelope points at time selection to all visible track envelopes" },                                                             "BR_INS_2_ENV_POINT_TIME_SEL_ALL",    Insert2EnvPointsTimeSelection, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Insert 2 envelope points at time selection to all visible track envelopes in selected tracks" },                                          "BR_INS_2_ENV_POINT_TIME_SEL_TRACKS", Insert2EnvPointsTimeSelection, NULL, 2},

	{ { DEFACCEL, "SWS/BR: Copy selected points in selected envelope to all visible envelopes in selected tracks" },                                                 "BR_COPY_ENV_PT_SEL_TR_ENVS_VIS",     CopyEnvPoints, NULL, BINARY(000)},
	{ { DEFACCEL, "SWS/BR: Copy selected points in selected envelope to all visible record-armed envelopes in selected tracks" },                                    "BR_COPY_ENV_PT_SEL_TR_ENVS_REC",     CopyEnvPoints, NULL, BINARY(001)},
	{ { DEFACCEL, "SWS/BR: Copy points in time selection in selected envelope to all visible envelopes in selected tracks" },                                        "BR_COPY_ENV_TS_SEL_TR_ENVS_Vvisible IS",     CopyEnvPoints, NULL, BINARY(010)},
	{ { DEFACCEL, "SWS/BR: Copy points in time selection in selected envelope to all visible record-armed in envelopes of selected tracks" },                        "BR_COPY_ENV_TS_SEL_TR_ENVS_Rvisible EC",     CopyEnvPoints, NULL, BINARY(010)},

	{ { DEFACCEL, "SWS/BR: Copy selected points in selected envelope to all visible envelopes in selected tracks (paste at edit cursor)" },                          "BR_COPY_ENV_PT_SEL_TR_ENVS_VIS_CUR", CopyEnvPoints, NULL, BINARY(100)},
	{ { DEFACCEL, "SWS/BR: Copy selected points in selected envelope to all visible record-armed envelopes in selected tracks (paste at edit cursor)" },             "BR_COPY_ENV_PT_SEL_TR_ENVS_REC_CUR", CopyEnvPoints, NULL, BINARY(101)},
	{ { DEFACCEL, "SWS/BR: Copy points in time selection in selected envelope to all visible envelopes in selected tracks (paste at edit cursor)" },                 "BR_COPY_ENV_TS_SEL_TR_ENVS_VIS_CUR", CopyEnvPoints, NULL, BINARY(110)},
	{ { DEFACCEL, "SWS/BR: Copy points in time selection in selected envelope to all visible record-armed in envelopes of selected tracks (paste at edit cursor)" }, "BR_COPY_ENV_TS_SEL_TR_ENVS_REC_CUR", CopyEnvPoints, NULL, BINARY(111)},

	{ { DEFACCEL, "SWS/BR: Fit selected envelope points to time selection" },                                                                                        "BR_FIT_ENV_POINTS_TO_TIMESEL",       FitEnvPointsToTimeSel, NULL},
	{ { DEFACCEL, "SWS/BR: Insert new envelope point at mouse cursor using value at current position (obey snapping)" },                                             "BR_ENV_POINT_MOUSE_CURSOR",          CreateEnvPointMouse, NULL},

	{ { DEFACCEL, "SWS/BR: Increase selected envelope points by 0.1 db (volume envelope only)" },                                                                    "BR_INC_VOL_ENV_PT_01.db",            IncreaseDecreaseVolEnvPoints, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Increase selected envelope points by 0.5 db (volume envelope only)" },                                                                    "BR_INC_VOL_ENV_PT_05.db",            IncreaseDecreaseVolEnvPoints, NULL, 5},
	{ { DEFACCEL, "SWS/BR: Increase selected envelope points by 1 db (volume envelope only)" },                                                                      "BR_INC_VOL_ENV_PT_1db",              IncreaseDecreaseVolEnvPoints, NULL, 10},
	{ { DEFACCEL, "SWS/BR: Increase selected envelope points by 5 db (volume envelope only)" },                                                                      "BR_INC_VOL_ENV_PT_5db",              IncreaseDecreaseVolEnvPoints, NULL, 50},
	{ { DEFACCEL, "SWS/BR: Increase selected envelope points by 10 db (volume envelope only)" },                                                                     "BR_INC_VOL_ENV_PT_10db",             IncreaseDecreaseVolEnvPoints, NULL, 100},
	{ { DEFACCEL, "SWS/BR: Decrease selected envelope points by 0.1 db (volume envelope only)" },                                                                    "BR_DEC_VOL_ENV_PT_01.db",            IncreaseDecreaseVolEnvPoints, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Decrease selected envelope points by 0.5 db (volume envelope only)" },                                                                    "BR_DEC_VOL_ENV_PT_05.db",            IncreaseDecreaseVolEnvPoints, NULL, -5},
	{ { DEFACCEL, "SWS/BR: Decrease selected envelope points by 1 db (volume envelope only)" },                                                                      "BR_DEC_VOL_ENV_PT_1db",              IncreaseDecreaseVolEnvPoints, NULL, -10},
	{ { DEFACCEL, "SWS/BR: Decrease selected envelope points by 5 db (volume envelope only)" },                                                                      "BR_DEC_VOL_ENV_PT_5db",              IncreaseDecreaseVolEnvPoints, NULL, -50},
	{ { DEFACCEL, "SWS/BR: Decrease selected envelope points by 10 db (volume envelope only)" },                                                                     "BR_DEC_VOL_ENV_PT_10db",             IncreaseDecreaseVolEnvPoints, NULL, -100},

	{ { DEFACCEL, "SWS/BR: Select envelope under mouse cursor" },                                                                                                    "BR_SEL_ENV_MOUSE",                   SelectEnvelopeUnderMouse, NULL, 0},
	{ { DEFACCEL, "SWS/BR: Select envelope point under mouse cursor (selected envelope only)" },                                                                     "BR_SEL_ENV_PT_MOUSE_ACT_ENV_ONLY",   SelectDeleteEnvPointUnderMouse, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Select envelope point under mouse cursor" },                                                                                              "BR_SEL_ENV_PT_MOUSE",                SelectDeleteEnvPointUnderMouse, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Delete envelope point under mouse cursor (selected envelope only)" },                                                                     "BR_DEL_ENV_PT_MOUSE_ACT_ENV_ONLY",   SelectDeleteEnvPointUnderMouse, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Delete envelope point under mouse cursor" },                                                                                              "BR_DEL_ENV_PT_MOUSE",                SelectDeleteEnvPointUnderMouse, NULL, -2},

	{ { DEFACCEL, "SWS/BR: Unselect envelope" },                                                                                                                     "BR_UNSEL_ENV",                       UnselectEnvelope, NULL, 0},

	{ { DEFACCEL, "SWS/BR: Apply next action to all visible envelopes in selected tracks" },                                                                         "BR_NEXT_CMD_SEL_TK_VIS_ENVS",        ApplyNextCmdToMultiEnvelopes, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Apply next action to all visible record-armed envelopes in selected tracks" },                                                            "BR_NEXT_CMD_SEL_TK_REC_ENVS",        ApplyNextCmdToMultiEnvelopes, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Apply next action to all visible envelopes in selected tracks if there is no track envelope selected" },                                  "BR_NEXT_CMD_SEL_TK_VIS_ENVS_NOSEL",  ApplyNextCmdToMultiEnvelopes, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Apply next action to all visible record-armed envelopes in selected tracks if there is no track envelope selected" },                     "BR_NEXT_CMD_SEL_TK_REC_ENVS_NOSEL",  ApplyNextCmdToMultiEnvelopes, NULL, -2},

	{ { DEFACCEL, "SWS/BR: Save envelope point selection, slot 1" },                                                                                                 "BR_SAVE_ENV_SEL_SLOT_1",             SaveEnvSelSlot, NULL, 0},
	{ { DEFACCEL, "SWS/BR: Save envelope point selection, slot 2" },                                                                                                 "BR_SAVE_ENV_SEL_SLOT_2",             SaveEnvSelSlot, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Save envelope point selection, slot 3" },                                                                                                 "BR_SAVE_ENV_SEL_SLOT_3",             SaveEnvSelSlot, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Save envelope point selection, slot 4" },                                                                                                 "BR_SAVE_ENV_SEL_SLOT_4",             SaveEnvSelSlot, NULL, 3},
	{ { DEFACCEL, "SWS/BR: Save envelope point selection, slot 5" },                                                                                                 "BR_SAVE_ENV_SEL_SLOT_5",             SaveEnvSelSlot, NULL, 4},
	{ { DEFACCEL, "SWS/BR: Save envelope point selection, slot 6" },                                                                                                 "BR_SAVE_ENV_SEL_SLOT_6",             SaveEnvSelSlot, NULL, 5},
	{ { DEFACCEL, "SWS/BR: Save envelope point selection, slot 7" },                                                                                                 "BR_SAVE_ENV_SEL_SLOT_7",             SaveEnvSelSlot, NULL, 6},
	{ { DEFACCEL, "SWS/BR: Save envelope point selection, slot 8" },                                                                                                 "BR_SAVE_ENV_SEL_SLOT_8",             SaveEnvSelSlot, NULL, 7},
	{ { DEFACCEL, "SWS/BR: Restore envelope point selection, slot 1" },                                                                                              "BR_RESTORE_ENV_SEL_SLOT_1",          RestoreEnvSelSlot, NULL, 0},
	{ { DEFACCEL, "SWS/BR: Restore envelope point selection, slot 2" },                                                                                              "BR_RESTORE_ENV_SEL_SLOT_2",          RestoreEnvSelSlot, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Restore envelope point selection, slot 3" },                                                                                              "BR_RESTORE_ENV_SEL_SLOT_3",          RestoreEnvSelSlot, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Restore envelope point selection, slot 4" },                                                                                              "BR_RESTORE_ENV_SEL_SLOT_4",          RestoreEnvSelSlot, NULL, 3},
	{ { DEFACCEL, "SWS/BR: Restore envelope point selection, slot 5" },                                                                                              "BR_RESTORE_ENV_SEL_SLOT_5",          RestoreEnvSelSlot, NULL, 4},
	{ { DEFACCEL, "SWS/BR: Restore envelope point selection, slot 6" },                                                                                              "BR_RESTORE_ENV_SEL_SLOT_6",          RestoreEnvSelSlot, NULL, 5},
	{ { DEFACCEL, "SWS/BR: Restore envelope point selection, slot 7" },                                                                                              "BR_RESTORE_ENV_SEL_SLOT_7",          RestoreEnvSelSlot, NULL, 6},
	{ { DEFACCEL, "SWS/BR: Restore envelope point selection, slot 8" },                                                                                              "BR_RESTORE_ENV_SEL_SLOT_8",          RestoreEnvSelSlot, NULL, 7},

	/******************************************************************************
	* Envelopes - Visibility                                                      *
	******************************************************************************/
	{ { DEFACCEL, "SWS/BR: Hide all but selected track envelope for all tracks" },               "BR_ENV_HIDE_ALL_BUT_ACTIVE",            ShowActiveTrackEnvOnly, NULL, 0},
	{ { DEFACCEL, "SWS/BR: Hide all but selected track envelope for selected tracks" },          "BR_ENV_HIDE_ALL_BUT_ACTIVE_SEL",        ShowActiveTrackEnvOnly, NULL, 1},

	{ { DEFACCEL, "SWS/BR: Show/hide track envelope for last adjusted send (volume/pan only)" }, "BR_LAST_ADJ_SEND_ENVELOPE",             ShowLastAdjustedSendEnv, NULL, 0},
	{ { DEFACCEL, "SWS/BR: Show/hide volume track envelope for last adjusted send" },            "BR_LAST_ADJ_SEND_ENVELOPE_VOL",         ShowLastAdjustedSendEnv, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Show/hide pan track envelope for last adjusted send" },               "BR_LAST_ADJ_SEND_ENVELOPE_PAN",         ShowLastAdjustedSendEnv, NULL, 2},

	{ { DEFACCEL, "SWS/BR: Toggle show all active FX envelopes for selected tracks" },           "BR_T_SHOW_ACT_FX_ENV_SEL_TRACK",        ShowHideFxEnv, NULL, BINARY(110)},
	{ { DEFACCEL, "SWS/BR: Toggle show all FX envelopes for selected tracks" },                  "BR_T_SHOW_FX_ENV_SEL_TRACK",            ShowHideFxEnv, NULL, BINARY(100)},
	{ { DEFACCEL, "SWS/BR: Show all active FX envelopes for selected tracks" },                  "BR_SHOW_ACT_FX_ENV_SEL_TRACK",          ShowHideFxEnv, NULL, BINARY(010)},
	{ { DEFACCEL, "SWS/BR: Show all FX envelopes for selected tracks" },                         "BR_SHOW_FX_ENV_SEL_TRACK",              ShowHideFxEnv, NULL, BINARY(000)},
	{ { DEFACCEL, "SWS/BR: Hide all FX envelopes for selected tracks" },                         "BR_HIDE_FX_ENV_SEL_TRACK",              ShowHideFxEnv, NULL, BINARY(001)},

	{ { DEFACCEL, "SWS/BR: Toggle show all active send envelopes for selected tracks" },         "BR_T_SHOW_ACT_SEND_ENV_ALL_SEL_TRACK",  ShowHideSendEnv, NULL, BINARY(111110)},
	{ { DEFACCEL, "SWS/BR: Toggle show active volume send envelopes for selected tracks" },      "BR_T_SHOW_ACT_SEND_ENV_VOL_SEL_TRACK",  ShowHideSendEnv, NULL, BINARY(001110)},
	{ { DEFACCEL, "SWS/BR: Toggle show active pan send envelopes for selected tracks" },         "BR_T_SHOW_ACT_SEND_ENV_PAN_SEL_TRACK",  ShowHideSendEnv, NULL, BINARY(010110)},
	{ { DEFACCEL, "SWS/BR: Toggle show active mute send envelopes for selected tracks" },        "BR_T_SHOW_ACT_SEND_ENV_MUTE_SEL_TRACK", ShowHideSendEnv, NULL, BINARY(100110)},
	{ { DEFACCEL, "SWS/BR: Show all active send envelopes for selected tracks" },                "BR_SHOW_ACT_SEND_ENV_ALL_SEL_TRACK",    ShowHideSendEnv, NULL, BINARY(111010)},
	{ { DEFACCEL, "SWS/BR: Show active volume send envelopes for selected tracks" },             "BR_SHOW_ACT_SEND_ENV_VOL_SEL_TRACK",    ShowHideSendEnv, NULL, BINARY(001010)},
	{ { DEFACCEL, "SWS/BR: Show active pan send envelopes for selected tracks" },                "BR_SHOW_ACT_SEND_ENV_PAN_SEL_TRACK",    ShowHideSendEnv, NULL, BINARY(010010)},
	{ { DEFACCEL, "SWS/BR: Show active mute send envelopes for selected tracks" },               "BR_SHOW_ACT_SEND_ENV_MUTE_SEL_TRACK",   ShowHideSendEnv, NULL, BINARY(100010)},

	{ { DEFACCEL, "SWS/BR: Toggle show all send envelopes for selected tracks" },                "BR_T_SHOW_SEND_ENV_ALL_SEL_TRACK",      ShowHideSendEnv, NULL, BINARY(111100)},
	{ { DEFACCEL, "SWS/BR: Toggle show volume send envelopes for selected tracks" },             "BR_T_SHOW_SEND_ENV_VOL_SEL_TRACK",      ShowHideSendEnv, NULL, BINARY(001100)},
	{ { DEFACCEL, "SWS/BR: Toggle show pan send envelopes for selected tracks" },                "BR_T_SHOW_SEND_ENV_PAN_SEL_TRACK",      ShowHideSendEnv, NULL, BINARY(010100)},
	{ { DEFACCEL, "SWS/BR: Toggle show mute send envelopes for selected tracks" },               "BR_T_SHOW_SEND_ENV_MUTE_SEL_TRACK",     ShowHideSendEnv, NULL, BINARY(100100)},
	{ { DEFACCEL, "SWS/BR: Show all send envelopes for selected tracks" },                       "BR_SHOW_SEND_ENV_ALL_SEL_TRACK",        ShowHideSendEnv, NULL, BINARY(111000)},
	{ { DEFACCEL, "SWS/BR: Show volume send envelopes for selected tracks" },                    "BR_SHOW_SEND_ENV_VOL_SEL_TRACK",        ShowHideSendEnv, NULL, BINARY(001000)},
	{ { DEFACCEL, "SWS/BR: Show pan send envelopes for selected tracks" },                       "BR_SHOW_SEND_ENV_PAN_SEL_TRACK",        ShowHideSendEnv, NULL, BINARY(010000)},
	{ { DEFACCEL, "SWS/BR: Show mute send envelopes for selected tracks" },                      "BR_SHOW_SEND_ENV_MUTE_SEL_TRACK",       ShowHideSendEnv, NULL, BINARY(100000)},

	{ { DEFACCEL, "SWS/BR: Hide all send envelopes for selected tracks" },                       "BR_HIDE_SEND_ENV_SEL_ALL_TRACK",        ShowHideSendEnv, NULL, BINARY(111001)},
	{ { DEFACCEL, "SWS/BR: Hide volume send envelopes for selected tracks" },                    "BR_HIDE_SEND_ENV_SEL_VOL_TRACK",        ShowHideSendEnv, NULL, BINARY(001001)},
	{ { DEFACCEL, "SWS/BR: Hide pan send envelopes for selected tracks" },                       "BR_HIDE_SEND_ENV_SEL_PAN_TRACK",        ShowHideSendEnv, NULL, BINARY(010001)},
	{ { DEFACCEL, "SWS/BR: Hide mute send envelopes for selected tracks" },                      "BR_HIDE_SEND_ENV_SEL_NUTE_TRACK",       ShowHideSendEnv, NULL, BINARY(100001)},

	/******************************************************************************
	* Loudness                                                                    *
	******************************************************************************/
	{ { DEFACCEL, "SWS/BR: Global loudness preferences..." },                    "BR_LOUDNESS_PREF",                ToggleLoudnessPref, NULL, 0, IsLoudnessPrefVisible},
	{ { DEFACCEL, "SWS/BR: Analyze loudness..." },                               "BR_ANALAYZE_LOUDNESS_DLG",        AnalyzeLoudness,    NULL, 0, IsAnalyzeLoudnessVisible},

	{ { DEFACCEL, "SWS/BR: Normalize loudness of selected items/tracks..." },    "BR_NORMALIZE_LOUDNESS_ITEMS",     NormalizeLoudness,  NULL, 0, IsNormalizeLoudnessVisible},
	{ { DEFACCEL, "SWS/BR: Normalize loudness of selected items to -23 LUFS" },  "BR_NORMALIZE_LOUDNESS_ITEMS23",   NormalizeLoudness,  NULL, 1, },
	{ { DEFACCEL, "SWS/BR: Normalize loudness of selected items to 0 LU" },      "BR_NORMALIZE_LOUDNESS_ITEMS_LU",  NormalizeLoudness,  NULL, -1, },
	{ { DEFACCEL, "SWS/BR: Normalize loudness of selected tracks to -23 LUFS" }, "BR_NORMALIZE_LOUDNESS_TRACKS23",  NormalizeLoudness,  NULL, 2, },
	{ { DEFACCEL, "SWS/BR: Normalize loudness of selected tracks to 0 LU" },     "BR_NORMALIZE_LOUDNESS_TRACKS_LU", NormalizeLoudness,  NULL, -2, },

	/******************************************************************************
	* MIDI editor - Item preview                                                  *
	******************************************************************************/
	{ { DEFACCEL, "SWS/BR: Stop media item preview" },                                                                                                   "BR_ME_STOP_PREV_ACT_ITEM",             NULL, NULL, 0,    NULL, SECTION_MIDI_EDITOR, ME_StopMidiTakePreview},

	{ { DEFACCEL, "SWS/BR: Preview active media item through track" },                                                                                   "BR_ME_PREV_ACT_ITEM",                  NULL, NULL, 1111, NULL, SECTION_MIDI_EDITOR, ME_PreviewActiveTake},
	{ { DEFACCEL, "SWS/BR: Preview active media item through track (start from mouse position)" },                                                       "BR_ME_PREV_ACT_ITEM_POS",              NULL, NULL, 1211, NULL, SECTION_MIDI_EDITOR, ME_PreviewActiveTake},
	{ { DEFACCEL, "SWS/BR: Preview active media item through track (sync with next measure)" },                                                          "BR_ME_PREV_ACT_ITEM_SYNC",             NULL, NULL, 1311, NULL, SECTION_MIDI_EDITOR, ME_PreviewActiveTake},
	{ { DEFACCEL, "SWS/BR: Preview active media item through track and pause during preview" },                                                          "BR_ME_PREV_ACT_ITEM_PAUSE",            NULL, NULL, 1112, NULL, SECTION_MIDI_EDITOR, ME_PreviewActiveTake},
	{ { DEFACCEL, "SWS/BR: Preview active media item through track and pause during preview (start from mouse position)" },                              "BR_ME_PREV_ACT_ITEM_PAUSE_POS",        NULL, NULL, 1212, NULL, SECTION_MIDI_EDITOR, ME_PreviewActiveTake},
	{ { DEFACCEL, "SWS/BR: Preview active media item (selected notes only) through track" },                                                             "BR_ME_PREV_ACT_ITEM_NOTES",            NULL, NULL, 1121, NULL, SECTION_MIDI_EDITOR, ME_PreviewActiveTake},
	{ { DEFACCEL, "SWS/BR: Preview active media item (selected notes only) through track (start from mouse position)" },                                 "BR_ME_PREV_ACT_ITEM_NOTES_POS",        NULL, NULL, 1221, NULL, SECTION_MIDI_EDITOR, ME_PreviewActiveTake},
	{ { DEFACCEL, "SWS/BR: Preview active media item (selected notes only) through track (sync with next measure)" },                                    "BR_ME_PREV_ACT_ITEM_NOTES_SYNC",       NULL, NULL, 1321, NULL, SECTION_MIDI_EDITOR, ME_PreviewActiveTake},
	{ { DEFACCEL, "SWS/BR: Preview active media item (selected notes only) through track and pause during preview" },                                    "BR_ME_PREV_ACT_ITEM_NOTES_PAUSE",      NULL, NULL, 1122, NULL, SECTION_MIDI_EDITOR, ME_PreviewActiveTake},
	{ { DEFACCEL, "SWS/BR: Preview active media item (selected notes only) through track and pause during preview (start from mouse position)" },        "BR_ME_PREV_ACT_ITEM_NOTES_PAUSE_POS",  NULL, NULL, 1222, NULL, SECTION_MIDI_EDITOR, ME_PreviewActiveTake},

	{ { DEFACCEL, "SWS/BR: Toggle preview active media item through track" },                                                                            "BR_ME_TPREV_ACT_ITEM",                 NULL, NULL, 2111, NULL, SECTION_MIDI_EDITOR, ME_PreviewActiveTake},
	{ { DEFACCEL, "SWS/BR: Toggle preview active media item through track (start from mouse position)" },                                                "BR_ME_TPREV_ACT_ITEM_POS",             NULL, NULL, 2211, NULL, SECTION_MIDI_EDITOR, ME_PreviewActiveTake},
	{ { DEFACCEL, "SWS/BR: Toggle preview active media item through track (sync with next measure)" },                                                   "BR_ME_TPREV_ACT_ITEM_SYNC",            NULL, NULL, 2311, NULL, SECTION_MIDI_EDITOR, ME_PreviewActiveTake},
	{ { DEFACCEL, "SWS/BR: Toggle preview active media item through track and pause during preview" },                                                   "BR_ME_TPREV_ACT_ITEM_PAUSE",           NULL, NULL, 2112, NULL, SECTION_MIDI_EDITOR, ME_PreviewActiveTake},
	{ { DEFACCEL, "SWS/BR: Toggle preview active media item through track and pause during preview (start from mouse position)" },                       "BR_ME_TPREV_ACT_ITEM_PAUSE_POS",       NULL, NULL, 2212, NULL, SECTION_MIDI_EDITOR, ME_PreviewActiveTake},
	{ { DEFACCEL, "SWS/BR: Toggle preview active media item (selected notes only) through track" },                                                      "BR_ME_TPREV_ACT_ITEM_NOTES",           NULL, NULL, 2121, NULL, SECTION_MIDI_EDITOR, ME_PreviewActiveTake},
	{ { DEFACCEL, "SWS/BR: Toggle preview active media item (selected notes only) through track (start from mouse position)" },                          "BR_ME_TPREV_ACT_ITEM_NOTES_POS",       NULL, NULL, 2221, NULL, SECTION_MIDI_EDITOR, ME_PreviewActiveTake},
	{ { DEFACCEL, "SWS/BR: Toggle preview active media item (selected notes only) through track (sync with next measure)" },                             "BR_ME_TPREV_ACT_ITEM_NOTES_SYNC",      NULL, NULL, 2321, NULL, SECTION_MIDI_EDITOR, ME_PreviewActiveTake},
	{ { DEFACCEL, "SWS/BR: Toggle preview active media item (selected notes only) through track and pause during preview" },                             "BR_ME_TPREV_ACT_ITEM_NOTES_PAUSE",     NULL, NULL, 2122, NULL, SECTION_MIDI_EDITOR, ME_PreviewActiveTake},
	{ { DEFACCEL, "SWS/BR: Toggle preview active media item (selected notes only) through track and pause during preview (start from mouse position)" }, "BR_ME_TPREV_ACT_ITEM_NOTES_PAUSE_POS", NULL, NULL, 2222, NULL, SECTION_MIDI_EDITOR, ME_PreviewActiveTake},

	/******************************************************************************
	* MIDI editor - Misc                                                          *
	******************************************************************************/
	{ { DEFACCEL, "SWS/BR: Play from mouse cursor position" },                                                                                           "BR_ME_PLAY_MOUSECURSOR",                       NULL, NULL, 0, NULL, SECTION_MIDI_EDITOR, ME_PlaybackAtMouseCursor},
	{ { DEFACCEL, "SWS/BR: Play/pause from mouse cursor position" },                                                                                     "BR_ME_PLAY_PAUSE_MOUSECURSOR",                 NULL, NULL, 1, NULL, SECTION_MIDI_EDITOR, ME_PlaybackAtMouseCursor},
	{ { DEFACCEL, "SWS/BR: Play/stop from mouse cursor position" },                                                                                      "BR_ME_PLAY_STOP_MOUSECURSOR",                  NULL, NULL, 2, NULL, SECTION_MIDI_EDITOR, ME_PlaybackAtMouseCursor},

	{ { DEFACCEL, "SWS/BR: Insert CC event at edit cursor in lane under mouse cursor" },                                                                 "BR_ME_INSERT_CC_EDIT_CURSOR_MOUSE_LANE",       NULL, NULL, 0, NULL, SECTION_MIDI_EDITOR, ME_CCEventAtEditCursor},
	{ { DEFACCEL, "SWS/BR: Show only used CC lanes (detect 14-bit)" },                                                                                   "BR_ME_SHOW_USED_CC_14_BIT",                    NULL, NULL, 0, NULL, SECTION_MIDI_EDITOR, ME_ShowUsedCCLanesDetect14Bit},
	{ { DEFACCEL, "SWS/BR: Create CC lane and make it last clicked" },                                                                                   "BR_ME_CREATE_CC_LAST_CLICKED",                 NULL, NULL, 0, NULL, SECTION_MIDI_EDITOR, ME_CreateCCLaneLastClicked},

	{ { DEFACCEL, "SWS/BR: Hide last clicked CC lane" },                                                                                                 "BR_ME_HIDE_LAST_CLICKED_LANE",                 NULL, NULL, 1,   NULL,                     SECTION_MIDI_EDITOR, ME_HideCCLanes},
	{ { DEFACCEL, "SWS/BR: Hide all CC lanes except last clicked lane" },                                                                                "BR_ME_HIDE_ALL_NO_LAST_CLICKED",               NULL, NULL, 2,   NULL,                     SECTION_MIDI_EDITOR, ME_HideCCLanes},
	{ { DEFACCEL, "SWS/BR: Toggle hide all CC lanes except last clicked lane" },                                                                         "BR_ME_TOGGLE_HIDE_ALL_NO_LAST_CLICKED",        NULL, NULL, 1,   ME_IsToggleHideCCLanesOn, SECTION_MIDI_EDITOR, ME_ToggleHideCCLanes},
	{ { DEFACCEL, "SWS/BR: Toggle hide all CC lanes except last clicked lane (set lane height to 50 pixel)" },                                           "BR_ME_TOGGLE_HIDE_ALL_NO_LAST_CLICKED_50_PX",  NULL, NULL, 50,  ME_IsToggleHideCCLanesOn, SECTION_MIDI_EDITOR, ME_ToggleHideCCLanes},
	{ { DEFACCEL, "SWS/BR: Toggle hide all CC lanes except last clicked lane (set lane height to 100 pixel)" },                                          "BR_ME_TOGGLE_HIDE_ALL_NO_LAST_CLICKED_100_PX", NULL, NULL, 100, ME_IsToggleHideCCLanesOn, SECTION_MIDI_EDITOR, ME_ToggleHideCCLanes},
	{ { DEFACCEL, "SWS/BR: Toggle hide all CC lanes except last clicked lane (set lane height to 150 pixel)" },                                          "BR_ME_TOGGLE_HIDE_ALL_NO_LAST_CLICKED_150_PX", NULL, NULL, 150, ME_IsToggleHideCCLanesOn, SECTION_MIDI_EDITOR, ME_ToggleHideCCLanes},
	{ { DEFACCEL, "SWS/BR: Toggle hide all CC lanes except last clicked lane (set lane height to 200 pixel)" },                                          "BR_ME_TOGGLE_HIDE_ALL_NO_LAST_CLICKED_200_PX", NULL, NULL, 200, ME_IsToggleHideCCLanesOn, SECTION_MIDI_EDITOR, ME_ToggleHideCCLanes},
	{ { DEFACCEL, "SWS/BR: Toggle hide all CC lanes except last clicked lane (set lane height to 250 pixel)" },                                          "BR_ME_TOGGLE_HIDE_ALL_NO_LAST_CLICKED_250_PX", NULL, NULL, 250, ME_IsToggleHideCCLanesOn, SECTION_MIDI_EDITOR, ME_ToggleHideCCLanes},
	{ { DEFACCEL, "SWS/BR: Toggle hide all CC lanes except last clicked lane (set lane height to 300 pixel)" },                                          "BR_ME_TOGGLE_HIDE_ALL_NO_LAST_CLICKED_300_PX", NULL, NULL, 300, ME_IsToggleHideCCLanesOn, SECTION_MIDI_EDITOR, ME_ToggleHideCCLanes},
	{ { DEFACCEL, "SWS/BR: Toggle hide all CC lanes except last clicked lane (set lane height to 350 pixel)" },                                          "BR_ME_TOGGLE_HIDE_ALL_NO_LAST_CLICKED_350_PX", NULL, NULL, 350, ME_IsToggleHideCCLanesOn, SECTION_MIDI_EDITOR, ME_ToggleHideCCLanes},
	{ { DEFACCEL, "SWS/BR: Toggle hide all CC lanes except last clicked lane (set lane height to 400 pixel)" },                                          "BR_ME_TOGGLE_HIDE_ALL_NO_LAST_CLICKED_400_PX", NULL, NULL, 400, ME_IsToggleHideCCLanesOn, SECTION_MIDI_EDITOR, ME_ToggleHideCCLanes},
	{ { DEFACCEL, "SWS/BR: Toggle hide all CC lanes except last clicked lane (set lane height to 450 pixel)" },                                          "BR_ME_TOGGLE_HIDE_ALL_NO_LAST_CLICKED_450_PX", NULL, NULL, 450, ME_IsToggleHideCCLanesOn, SECTION_MIDI_EDITOR, ME_ToggleHideCCLanes},
	{ { DEFACCEL, "SWS/BR: Toggle hide all CC lanes except last clicked lane (set lane height to 500 pixel)" },                                          "BR_ME_TOGGLE_HIDE_ALL_NO_LAST_CLICKED_500_PX", NULL, NULL, 500, ME_IsToggleHideCCLanesOn, SECTION_MIDI_EDITOR, ME_ToggleHideCCLanes},

	{ { DEFACCEL, "SWS/BR: Hide CC lane under mouse cursor" },                                                                                           "BR_ME_HIDE_MOUSE_LANE",                        NULL, NULL, -1,   NULL,                     SECTION_MIDI_EDITOR, ME_HideCCLanes},
	{ { DEFACCEL, "SWS/BR: Hide all CC lanes except lane under mouse cursor" },                                                                          "BR_ME_HIDE_ALL_NO_MOUSE_LANE",                 NULL, NULL, -2,   NULL,                     SECTION_MIDI_EDITOR, ME_HideCCLanes},
	{ { DEFACCEL, "SWS/BR: Toggle hide all CC lanes except lane under mouse cursor" },                                                                   "BR_ME_TOGGLE_HIDE_ALL_NO_MOUSE_LANE",          NULL, NULL, -1,   ME_IsToggleHideCCLanesOn, SECTION_MIDI_EDITOR, ME_ToggleHideCCLanes},
	{ { DEFACCEL, "SWS/BR: Toggle hide all CC lanes except lane under mouse cursor (set lane height to 50 pixel)" },                                     "BR_ME_TOGGLE_HIDE_ALL_NO_MOUSE_LANE_50_PX",    NULL, NULL, -50,  ME_IsToggleHideCCLanesOn, SECTION_MIDI_EDITOR, ME_ToggleHideCCLanes},
	{ { DEFACCEL, "SWS/BR: Toggle hide all CC lanes except lane under mouse cursor (set lane height to 100 pixel)" },                                    "BR_ME_TOGGLE_HIDE_ALL_NO_MOUSE_LANE_100_PX",   NULL, NULL, -100, ME_IsToggleHideCCLanesOn, SECTION_MIDI_EDITOR, ME_ToggleHideCCLanes},
	{ { DEFACCEL, "SWS/BR: Toggle hide all CC lanes except lane under mouse cursor (set lane height to 150 pixel)" },                                    "BR_ME_TOGGLE_HIDE_ALL_NO_MOUSE_LANE_150_PX",   NULL, NULL, -150, ME_IsToggleHideCCLanesOn, SECTION_MIDI_EDITOR, ME_ToggleHideCCLanes},
	{ { DEFACCEL, "SWS/BR: Toggle hide all CC lanes except lane under mouse cursor (set lane height to 200 pixel)" },                                    "BR_ME_TOGGLE_HIDE_ALL_NO_MOUSE_LANE_200_PX",   NULL, NULL, -200, ME_IsToggleHideCCLanesOn, SECTION_MIDI_EDITOR, ME_ToggleHideCCLanes},
	{ { DEFACCEL, "SWS/BR: Toggle hide all CC lanes except lane under mouse cursor (set lane height to 250 pixel)" },                                    "BR_ME_TOGGLE_HIDE_ALL_NO_MOUSE_LANE_250_PX",   NULL, NULL, -250, ME_IsToggleHideCCLanesOn, SECTION_MIDI_EDITOR, ME_ToggleHideCCLanes},
	{ { DEFACCEL, "SWS/BR: Toggle hide all CC lanes except lane under mouse cursor (set lane height to 300 pixel)" },                                    "BR_ME_TOGGLE_HIDE_ALL_NO_MOUSE_LANE_300_PX",   NULL, NULL, -300, ME_IsToggleHideCCLanesOn, SECTION_MIDI_EDITOR, ME_ToggleHideCCLanes},
	{ { DEFACCEL, "SWS/BR: Toggle hide all CC lanes except lane under mouse cursor (set lane height to 350 pixel)" },                                    "BR_ME_TOGGLE_HIDE_ALL_NO_MOUSE_LANE_350_PX",   NULL, NULL, -350, ME_IsToggleHideCCLanesOn, SECTION_MIDI_EDITOR, ME_ToggleHideCCLanes},
	{ { DEFACCEL, "SWS/BR: Toggle hide all CC lanes except lane under mouse cursor (set lane height to 400 pixel)" },                                    "BR_ME_TOGGLE_HIDE_ALL_NO_MOUSE_LANE_400_PX",   NULL, NULL, -400, ME_IsToggleHideCCLanesOn, SECTION_MIDI_EDITOR, ME_ToggleHideCCLanes},
	{ { DEFACCEL, "SWS/BR: Toggle hide all CC lanes except lane under mouse cursor (set lane height to 450 pixel)" },                                    "BR_ME_TOGGLE_HIDE_ALL_NO_MOUSE_LANE_450_PX",   NULL, NULL, -450, ME_IsToggleHideCCLanesOn, SECTION_MIDI_EDITOR, ME_ToggleHideCCLanes},
	{ { DEFACCEL, "SWS/BR: Toggle hide all CC lanes except lane under mouse cursor (set lane height to 500 pixel)" },                                    "BR_ME_TOGGLE_HIDE_ALL_NO_MOUSE_LANE_500_PX",   NULL, NULL, -500, ME_IsToggleHideCCLanesOn, SECTION_MIDI_EDITOR, ME_ToggleHideCCLanes},

	{ { DEFACCEL, "SWS/BR: Convert selected CC events in active item to square envelope points in selected envelope" },                                  "BR_ME_CC_TO_ENV_SQUARE",                       NULL, NULL, 1,  NULL, SECTION_MIDI_EDITOR, ME_CCToEnvPoints},
	{ { DEFACCEL, "SWS/BR: Convert selected CC events in active item to linear envelope points in selected envelope" },                                  "BR_ME_CC_TO_ENV_LINEAR",                       NULL, NULL, 2,  NULL, SECTION_MIDI_EDITOR, ME_CCToEnvPoints},
	{ { DEFACCEL, "SWS/BR: Convert selected CC events in active item to square envelope points in selected envelope (clear existing envelope points)" }, "BR_ME_CC_TO_ENV_SQUARE_CLEAR",                 NULL, NULL, -1, NULL, SECTION_MIDI_EDITOR, ME_CCToEnvPoints},
	{ { DEFACCEL, "SWS/BR: Convert selected CC events in active item to linear envelope points in selected envelope (clear existing envelope points)" }, "BR_ME_CC_TO_ENV_LINEAR_CLEAR",                 NULL, NULL, -2, NULL, SECTION_MIDI_EDITOR, ME_CCToEnvPoints},

	{ { DEFACCEL, "SWS/BR: Copy selected CC events in active item to last clicked lane" },                                                               "BR_ME_SEL_CC_TO_LAST_CLICKED_LANE",            NULL, NULL, 1,  NULL, SECTION_MIDI_EDITOR, ME_CopySelCCToLane},
	{ { DEFACCEL, "SWS/BR: Copy selected CC events in active item to lane under mouse cursor" },                                                         "BR_ME_SEL_CC_TO_LANE_UNDER_MOUSE",             NULL, NULL, -1, NULL, SECTION_MIDI_EDITOR, ME_CopySelCCToLane},

	{ { DEFACCEL, "SWS/BR: Save edit cursor position, slot 1" },                                                                                         "BR_ME_SAVE_CURSOR_POS_SLOT_1",                 NULL, NULL, 0, NULL, SECTION_MIDI_EDITOR, ME_SaveCursorPosSlot},
	{ { DEFACCEL, "SWS/BR: Save edit cursor position, slot 2" },                                                                                         "BR_ME_SAVE_CURSOR_POS_SLOT_2",                 NULL, NULL, 1, NULL, SECTION_MIDI_EDITOR, ME_SaveCursorPosSlot},
	{ { DEFACCEL, "SWS/BR: Save edit cursor position, slot 3" },                                                                                         "BR_ME_SAVE_CURSOR_POS_SLOT_3",                 NULL, NULL, 2, NULL, SECTION_MIDI_EDITOR, ME_SaveCursorPosSlot},
	{ { DEFACCEL, "SWS/BR: Save edit cursor position, slot 4" },                                                                                         "BR_ME_SAVE_CURSOR_POS_SLOT_4",                 NULL, NULL, 3, NULL, SECTION_MIDI_EDITOR, ME_SaveCursorPosSlot},
	{ { DEFACCEL, "SWS/BR: Save edit cursor position, slot 5" },                                                                                         "BR_ME_SAVE_CURSOR_POS_SLOT_5",                 NULL, NULL, 4, NULL, SECTION_MIDI_EDITOR, ME_SaveCursorPosSlot},
	{ { DEFACCEL, "SWS/BR: Save edit cursor position, slot 6" },                                                                                         "BR_ME_SAVE_CURSOR_POS_SLOT_6",                 NULL, NULL, 5, NULL, SECTION_MIDI_EDITOR, ME_SaveCursorPosSlot},
	{ { DEFACCEL, "SWS/BR: Save edit cursor position, slot 7" },                                                                                         "BR_ME_SAVE_CURSOR_POS_SLOT_7",                 NULL, NULL, 6, NULL, SECTION_MIDI_EDITOR, ME_SaveCursorPosSlot},
	{ { DEFACCEL, "SWS/BR: Save edit cursor position, slot 8" },                                                                                         "BR_ME_SAVE_CURSOR_POS_SLOT_8",                 NULL, NULL, 7, NULL, SECTION_MIDI_EDITOR, ME_SaveCursorPosSlot},
	{ { DEFACCEL, "SWS/BR: Restore edit cursor position, slot 1" },                                                                                      "BR_ME_RESTORE_CURSOR_POS_SLOT_1",              NULL, NULL, 0, NULL, SECTION_MIDI_EDITOR, ME_RestoreCursorPosSlot},
	{ { DEFACCEL, "SWS/BR: Restore edit cursor position, slot 2" },                                                                                      "BR_ME_RESTORE_CURSOR_POS_SLOT_2",              NULL, NULL, 1, NULL, SECTION_MIDI_EDITOR, ME_RestoreCursorPosSlot},
	{ { DEFACCEL, "SWS/BR: Restore edit cursor position, slot 3" },                                                                                      "BR_ME_RESTORE_CURSOR_POS_SLOT_3",              NULL, NULL, 2, NULL, SECTION_MIDI_EDITOR, ME_RestoreCursorPosSlot},
	{ { DEFACCEL, "SWS/BR: Restore edit cursor position, slot 4" },                                                                                      "BR_ME_RESTORE_CURSOR_POS_SLOT_4",              NULL, NULL, 3, NULL, SECTION_MIDI_EDITOR, ME_RestoreCursorPosSlot},
	{ { DEFACCEL, "SWS/BR: Restore edit cursor position, slot 5" },                                                                                      "BR_ME_RESTORE_CURSOR_POS_SLOT_5",              NULL, NULL, 4, NULL, SECTION_MIDI_EDITOR, ME_RestoreCursorPosSlot},
	{ { DEFACCEL, "SWS/BR: Restore edit cursor position, slot 6" },                                                                                      "BR_ME_RESTORE_CURSOR_POS_SLOT_6",              NULL, NULL, 5, NULL, SECTION_MIDI_EDITOR, ME_RestoreCursorPosSlot},
	{ { DEFACCEL, "SWS/BR: Restore edit cursor position, slot 7" },                                                                                      "BR_ME_RESTORE_CURSOR_POS_SLOT_7",              NULL, NULL, 6, NULL, SECTION_MIDI_EDITOR, ME_RestoreCursorPosSlot},
	{ { DEFACCEL, "SWS/BR: Restore edit cursor position, slot 8" },                                                                                      "BR_ME_RESTORE_CURSOR_POS_SLOT_8",              NULL, NULL, 7, NULL, SECTION_MIDI_EDITOR, ME_RestoreCursorPosSlot},

	{ { DEFACCEL, "SWS/BR: Save note selection from active item, slot 1" },                                                                              "BR_ME_SAVE_NOTE_SEL_SLOT_1",                   NULL, NULL, 0, NULL, SECTION_MIDI_EDITOR, ME_SaveNoteSelSlot},
	{ { DEFACCEL, "SWS/BR: Save note selection from active item, slot 2" },                                                                              "BR_ME_SAVE_NOTE_SEL_SLOT_2",                   NULL, NULL, 1, NULL, SECTION_MIDI_EDITOR, ME_SaveNoteSelSlot},
	{ { DEFACCEL, "SWS/BR: Save note selection from active item, slot 3" },                                                                              "BR_ME_SAVE_NOTE_SEL_SLOT_3",                   NULL, NULL, 2, NULL, SECTION_MIDI_EDITOR, ME_SaveNoteSelSlot},
	{ { DEFACCEL, "SWS/BR: Save note selection from active item, slot 4" },                                                                              "BR_ME_SAVE_NOTE_SEL_SLOT_4",                   NULL, NULL, 3, NULL, SECTION_MIDI_EDITOR, ME_SaveNoteSelSlot},
	{ { DEFACCEL, "SWS/BR: Save note selection from active item, slot 5" },                                                                              "BR_ME_SAVE_NOTE_SEL_SLOT_5",                   NULL, NULL, 4, NULL, SECTION_MIDI_EDITOR, ME_SaveNoteSelSlot},
	{ { DEFACCEL, "SWS/BR: Save note selection from active item, slot 6" },                                                                              "BR_ME_SAVE_NOTE_SEL_SLOT_6",                   NULL, NULL, 5, NULL, SECTION_MIDI_EDITOR, ME_SaveNoteSelSlot},
	{ { DEFACCEL, "SWS/BR: Save note selection from active item, slot 7" },                                                                              "BR_ME_SAVE_NOTE_SEL_SLOT_7",                   NULL, NULL, 6, NULL, SECTION_MIDI_EDITOR, ME_SaveNoteSelSlot},
	{ { DEFACCEL, "SWS/BR: Save note selection from active item, slot 8" },                                                                              "BR_ME_SAVE_NOTE_SEL_SLOT_8",                   NULL, NULL, 7, NULL, SECTION_MIDI_EDITOR, ME_SaveNoteSelSlot},
	{ { DEFACCEL, "SWS/BR: Restore note selection to active item, slot 1" },                                                                             "BR_ME_RESTORE_NOTE_SEL_SLOT_1",                NULL, NULL, 0, NULL, SECTION_MIDI_EDITOR, ME_RestoreNoteSelSlot},
	{ { DEFACCEL, "SWS/BR: Restore note selection to active item, slot 2" },                                                                             "BR_ME_RESTORE_NOTE_SEL_SLOT_2",                NULL, NULL, 1, NULL, SECTION_MIDI_EDITOR, ME_RestoreNoteSelSlot},
	{ { DEFACCEL, "SWS/BR: Restore note selection to active item, slot 3" },                                                                             "BR_ME_RESTORE_NOTE_SEL_SLOT_3",                NULL, NULL, 2, NULL, SECTION_MIDI_EDITOR, ME_RestoreNoteSelSlot},
	{ { DEFACCEL, "SWS/BR: Restore note selection to active item, slot 4" },                                                                             "BR_ME_RESTORE_NOTE_SEL_SLOT_4",                NULL, NULL, 3, NULL, SECTION_MIDI_EDITOR, ME_RestoreNoteSelSlot},
	{ { DEFACCEL, "SWS/BR: Restore note selection to active item, slot 5" },                                                                             "BR_ME_RESTORE_NOTE_SEL_SLOT_5",                NULL, NULL, 4, NULL, SECTION_MIDI_EDITOR, ME_RestoreNoteSelSlot},
	{ { DEFACCEL, "SWS/BR: Restore note selection to active item, slot 6" },                                                                             "BR_ME_RESTORE_NOTE_SEL_SLOT_6",                NULL, NULL, 5, NULL, SECTION_MIDI_EDITOR, ME_RestoreNoteSelSlot},
	{ { DEFACCEL, "SWS/BR: Restore note selection to active item, slot 7" },                                                                             "BR_ME_RESTORE_NOTE_SEL_SLOT_7",                NULL, NULL, 6, NULL, SECTION_MIDI_EDITOR, ME_RestoreNoteSelSlot},
	{ { DEFACCEL, "SWS/BR: Restore note selection to active item, slot 8" },                                                                             "BR_ME_RESTORE_NOTE_SEL_SLOT_8",                NULL, NULL, 7, NULL, SECTION_MIDI_EDITOR, ME_RestoreNoteSelSlot},

	{ { DEFACCEL, "SWS/BR: Save selected events in last clicked CC lane, slot 1" },                                                                      "BR_ME_SAVE_CC_SLOT_1",                         NULL, NULL, 1,  NULL, SECTION_MIDI_EDITOR, ME_SaveCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Save selected events in last clicked CC lane, slot 2" },                                                                      "BR_ME_SAVE_CC_SLOT_2",                         NULL, NULL, 2,  NULL, SECTION_MIDI_EDITOR, ME_SaveCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Save selected events in last clicked CC lane, slot 3" },                                                                      "BR_ME_SAVE_CC_SLOT_3",                         NULL, NULL, 3,  NULL, SECTION_MIDI_EDITOR, ME_SaveCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Save selected events in last clicked CC lane, slot 4" },                                                                      "BR_ME_SAVE_CC_SLOT_4",                         NULL, NULL, 4,  NULL, SECTION_MIDI_EDITOR, ME_SaveCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Save selected events in last clicked CC lane, slot 5" },                                                                      "BR_ME_SAVE_CC_SLOT_5",                         NULL, NULL, 5,  NULL, SECTION_MIDI_EDITOR, ME_SaveCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Save selected events in last clicked CC lane, slot 6" },                                                                      "BR_ME_SAVE_CC_SLOT_6",                         NULL, NULL, 6,  NULL, SECTION_MIDI_EDITOR, ME_SaveCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Save selected events in last clicked CC lane, slot 7" },                                                                      "BR_ME_SAVE_CC_SLOT_7",                         NULL, NULL, 7,  NULL, SECTION_MIDI_EDITOR, ME_SaveCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Save selected events in last clicked CC lane, slot 8" },                                                                      "BR_ME_SAVE_CC_SLOT_8",                         NULL, NULL, 8,  NULL, SECTION_MIDI_EDITOR, ME_SaveCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Save selected events in CC lane under mouse cursor, slot 1" },                                                                "BR_ME_SAVE_MOUSE_CC_SLOT_1",                   NULL, NULL, -1, NULL, SECTION_MIDI_EDITOR, ME_SaveCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Save selected events in CC lane under mouse cursor, slot 2" },                                                                "BR_ME_SAVE_MOUSE_CC_SLOT_2",                   NULL, NULL, -2, NULL, SECTION_MIDI_EDITOR, ME_SaveCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Save selected events in CC lane under mouse cursor, slot 3" },                                                                "BR_ME_SAVE_MOUSE_CC_SLOT_3",                   NULL, NULL, -3, NULL, SECTION_MIDI_EDITOR, ME_SaveCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Save selected events in CC lane under mouse cursor, slot 4" },                                                                "BR_ME_SAVE_MOUSE_CC_SLOT_4",                   NULL, NULL, -4, NULL, SECTION_MIDI_EDITOR, ME_SaveCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Save selected events in CC lane under mouse cursor, slot 5" },                                                                "BR_ME_SAVE_MOUSE_CC_SLOT_5",                   NULL, NULL, -5, NULL, SECTION_MIDI_EDITOR, ME_SaveCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Save selected events in CC lane under mouse cursor, slot 6" },                                                                "BR_ME_SAVE_MOUSE_CC_SLOT_6",                   NULL, NULL, -6, NULL, SECTION_MIDI_EDITOR, ME_SaveCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Save selected events in CC lane under mouse cursor, slot 7" },                                                                "BR_ME_SAVE_MOUSE_CC_SLOT_7",                   NULL, NULL, -7, NULL, SECTION_MIDI_EDITOR, ME_SaveCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Save selected events in CC lane under mouse cursor, slot 8" },                                                                "BR_ME_SAVE_MOUSE_CC_SLOT_8",                   NULL, NULL, -8, NULL, SECTION_MIDI_EDITOR, ME_SaveCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Restore events to last clicked CC lane, slot 1" },                                                                            "BR_ME_RESTORE_CC_LAST_CL_SLOT_1",              NULL, NULL, 1,  NULL, SECTION_MIDI_EDITOR, ME_RestoreCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Restore events to last clicked CC lane, slot 2" },                                                                            "BR_ME_RESTORE_CC_LAST_CL_SLOT_2",              NULL, NULL, 2,  NULL, SECTION_MIDI_EDITOR, ME_RestoreCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Restore events to last clicked CC lane, slot 3" },                                                                            "BR_ME_RESTORE_CC_LAST_CL_SLOT_3",              NULL, NULL, 3,  NULL, SECTION_MIDI_EDITOR, ME_RestoreCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Restore events to last clicked CC lane, slot 4" },                                                                            "BR_ME_RESTORE_CC_LAST_CL_SLOT_4",              NULL, NULL, 4,  NULL, SECTION_MIDI_EDITOR, ME_RestoreCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Restore events to last clicked CC lane, slot 5" },                                                                            "BR_ME_RESTORE_CC_LAST_CL_SLOT_5",              NULL, NULL, 5,  NULL, SECTION_MIDI_EDITOR, ME_RestoreCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Restore events to last clicked CC lane, slot 6" },                                                                            "BR_ME_RESTORE_CC_LAST_CL_SLOT_6",              NULL, NULL, 6,  NULL, SECTION_MIDI_EDITOR, ME_RestoreCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Restore events to last clicked CC lane, slot 7" },                                                                            "BR_ME_RESTORE_CC_LAST_CL_SLOT_7",              NULL, NULL, 7,  NULL, SECTION_MIDI_EDITOR, ME_RestoreCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Restore events to last clicked CC lane, slot 8" },                                                                            "BR_ME_RESTORE_CC_LAST_CL_SLOT_8",              NULL, NULL, 8,  NULL, SECTION_MIDI_EDITOR, ME_RestoreCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Restore events to CC lane under mouse cursor, slot 1" },                                                                      "BR_ME_RESTORE_CC_MOUSE_SLOT_1",                NULL, NULL, -1, NULL, SECTION_MIDI_EDITOR, ME_RestoreCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Restore events to CC lane under mouse cursor, slot 2" },                                                                      "BR_ME_RESTORE_CC_MOUSE_SLOT_2",                NULL, NULL, -2, NULL, SECTION_MIDI_EDITOR, ME_RestoreCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Restore events to CC lane under mouse cursor, slot 3" },                                                                      "BR_ME_RESTORE_CC_MOUSE_SLOT_3",                NULL, NULL, -3, NULL, SECTION_MIDI_EDITOR, ME_RestoreCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Restore events to CC lane under mouse cursor, slot 4" },                                                                      "BR_ME_RESTORE_CC_MOUSE_SLOT_4",                NULL, NULL, -4, NULL, SECTION_MIDI_EDITOR, ME_RestoreCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Restore events to CC lane under mouse cursor, slot 5" },                                                                      "BR_ME_RESTORE_CC_MOUSE_SLOT_5",                NULL, NULL, -5, NULL, SECTION_MIDI_EDITOR, ME_RestoreCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Restore events to CC lane under mouse cursor, slot 6" },                                                                      "BR_ME_RESTORE_CC_MOUSE_SLOT_6",                NULL, NULL, -6, NULL, SECTION_MIDI_EDITOR, ME_RestoreCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Restore events to CC lane under mouse cursor, slot 7" },                                                                      "BR_ME_RESTORE_CC_MOUSE_SLOT_7",                NULL, NULL, -7, NULL, SECTION_MIDI_EDITOR, ME_RestoreCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Restore events to CC lane under mouse cursor, slot 8" },                                                                      "BR_ME_RESTORE_CC_MOUSE_SLOT_8",                NULL, NULL, -8, NULL, SECTION_MIDI_EDITOR, ME_RestoreCCEventsSlot},
	{ { DEFACCEL, "SWS/BR: Restore events to all visible CC lanes, slot 1" },                                                                            "BR_ME_RESTORE_CC_ALL_VIS_SLOT_1",              NULL, NULL,  0, NULL, SECTION_MIDI_EDITOR, ME_RestoreCCEvents2Slot},
	{ { DEFACCEL, "SWS/BR: Restore events to all visible CC lanes, slot 2" },                                                                            "BR_ME_RESTORE_CC_ALL_VIS_SLOT_2",              NULL, NULL,  1, NULL, SECTION_MIDI_EDITOR, ME_RestoreCCEvents2Slot},
	{ { DEFACCEL, "SWS/BR: Restore events to all visible CC lanes, slot 3" },                                                                            "BR_ME_RESTORE_CC_ALL_VIS_SLOT_3",              NULL, NULL,  2, NULL, SECTION_MIDI_EDITOR, ME_RestoreCCEvents2Slot},
	{ { DEFACCEL, "SWS/BR: Restore events to all visible CC lanes, slot 4" },                                                                            "BR_ME_RESTORE_CC_ALL_VIS_SLOT_4",              NULL, NULL,  3, NULL, SECTION_MIDI_EDITOR, ME_RestoreCCEvents2Slot},
	{ { DEFACCEL, "SWS/BR: Restore events to all visible CC lanes, slot 5" },                                                                            "BR_ME_RESTORE_CC_ALL_VIS_SLOT_5",              NULL, NULL,  4, NULL, SECTION_MIDI_EDITOR, ME_RestoreCCEvents2Slot},
	{ { DEFACCEL, "SWS/BR: Restore events to all visible CC lanes, slot 6" },                                                                            "BR_ME_RESTORE_CC_ALL_VIS_SLOT_6",              NULL, NULL,  5, NULL, SECTION_MIDI_EDITOR, ME_RestoreCCEvents2Slot},
	{ { DEFACCEL, "SWS/BR: Restore events to all visible CC lanes, slot 7" },                                                                            "BR_ME_RESTORE_CC_ALL_VIS_SLOT_7",              NULL, NULL,  6, NULL, SECTION_MIDI_EDITOR, ME_RestoreCCEvents2Slot},
	{ { DEFACCEL, "SWS/BR: Restore events to all visible CC lanes, slot 8" },                                                                            "BR_ME_RESTORE_CC_ALL_VIS_SLOT_8",              NULL, NULL,  7, NULL, SECTION_MIDI_EDITOR, ME_RestoreCCEvents2Slot},

	/******************************************************************************
	* Misc                                                                        *
	******************************************************************************/
	{ { DEFACCEL, "SWS/BR: Split selected items at tempo markers" },                                                                                       "SWS_BRSPLITSELECTEDTEMPO",           SplitItemAtTempo},
	{ { DEFACCEL, "SWS/BR: Split selected items at stretch markers" },                                                                                     "BR_SPLIT_SEL_ITEM_STRETCH_MARKERS",  SplitItemAtStretchMarkers},
	{ { DEFACCEL, "SWS/BR: Create project markers from selected tempo markers" },                                                                          "BR_TEMPO_TO_MARKERS",                MarkersAtTempo},
	{ { DEFACCEL, "SWS/BR: Disable \"Ignore project tempo\" for selected MIDI items" },                                                                    "BR_MIDI_PROJ_TEMPO_DIS",             MidiItemTempo, NULL, 0},
	{ { DEFACCEL, "SWS/BR: Enable \"Ignore project tempo\" for selected MIDI items (use tempo at item's start)" },                                         "BR_MIDI_PROJ_TEMPO_ENB",             MidiItemTempo, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Enable \"Ignore project tempo\" for selected MIDI items preserving time position of MIDI events (use tempo at item's start)" }, "BR_MIDI_PROJ_TEMPO_ENB_TIME",        MidiItemTempo, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Trim MIDI item to active content" },                                                                                            "BR_TRIM_MIDI_ITEM_ACT_CONTENT",      MidiItemTrim, NULL},

	{ { DEFACCEL, "SWS/BR: Create project markers from notes in selected MIDI items" },                                                                    "BR_MIDI_NOTES_TO_MARKERS",           MarkersAtNotes},
	{ { DEFACCEL, "SWS/BR: Create project markers from stretch markers in selected items" },                                                               "BR_STRETCH_MARKERS_TO_MARKERS",      MarkersAtStretchMarkers},
	{ { DEFACCEL, "SWS/BR: Create project marker at mouse cursor" },                                                                                       "BR_MARKER_AT_MOUSE",                 MarkerAtMouse, NULL, 0},
	{ { DEFACCEL, "SWS/BR: Create project marker at mouse cursor (obey snapping)" },                                                                       "BR_MARKER_AT_MOUSE_SNAP",            MarkerAtMouse, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Create project markers from selected items (name by item's notes)" },                                                           "BR_ITEMS_TO_MARKERS_NOTES",          MarkersRegionsAtItems, NULL, 0},
	{ { DEFACCEL, "SWS/BR: Create regions from selected items (name by item's notes)" },                                                                   "BR_ITEMS_TO_REGIONS_NOTES",          MarkersRegionsAtItems, NULL, 1},

	{ { DEFACCEL, "SWS/BR: Move closest project marker to play cursor" },                                                                                  "BR_CLOSEST_PROJ_MARKER_PLAY",        MoveClosestMarker, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Move closest project marker to edit cursor" },                                                                                  "BR_CLOSEST_PROJ_MARKER_EDIT",        MoveClosestMarker, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Move closest project marker to mouse cursor" },                                                                                 "BR_CLOSEST_PROJ_MARKER_MOUSE",       MoveClosestMarker, NULL, 3},
	{ { DEFACCEL, "SWS/BR: Move closest project marker to play cursor (obey snapping)" },                                                                  "BR_CLOSEST_PROJ_MARKER_PLAY_SNAP",   MoveClosestMarker, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Move closest project marker to edit cursor (obey snapping)" },                                                                  "BR_CLOSEST_PROJ_MARKER_EDIT_SNAP",   MoveClosestMarker, NULL, -2},
	{ { DEFACCEL, "SWS/BR: Move closest project marker to mouse cursor (obey snapping)" },                                                                 "BR_CLOSEST_PROJ_MARKER_MOUSE_SNAP",  MoveClosestMarker, NULL, -3},

	{ { DEFACCEL, "SWS/BR: Toggle \"Grid snap settings follow grid visibility\"" },                                                                        "BR_OPTIONS_SNAP_FOLLOW_GRID_VIS",    SnapFollowsGridVis, NULL, 0, IsSnapFollowsGridVisOn},
	{ { DEFACCEL, "SWS/BR: Toggle \"Playback position follows project timebase when changing tempo\"" },                                                   "BR_OPTIONS_PLAYBACK_TEMPO_CHANGE",   PlaybackFollowsTempoChange, NULL, 0, IsPlaybackFollowingTempoChange},
	{ { DEFACCEL, "SWS/BR: Set \"Apply trim when adding volume/pan envelopes\" to \"Always\"" },                                                           "BR_OPTIONS_ENV_TRIM_ALWAYS",         TrimNewVolPanEnvs, NULL, 0, IsTrimNewVolPanEnvsOn},
	{ { DEFACCEL, "SWS/BR: Set \"Apply trim when adding volume/pan envelopes\" to \"In read/write\"" },                                                    "BR_OPTIONS_ENV_TRIM_READWRITE",      TrimNewVolPanEnvs, NULL, 1, IsTrimNewVolPanEnvsOn},
	{ { DEFACCEL, "SWS/BR: Set \"Apply trim when adding volume/pan envelopes\" to \"Never\"" },                                                            "BR_OPTIONS_ENV_TRIM_NEVER",          TrimNewVolPanEnvs, NULL, 2, IsTrimNewVolPanEnvsOn},

	{ { DEFACCEL, "SWS/BR: Cycle through record modes" },                                                                                                  "BR_CYCLE_RECORD_MODES",              CycleRecordModes},

	{ { DEFACCEL, "SWS/BR: Focus arrange" },                                                                                                               "BR_FOCUS_ARRANGE_WND",               FocusArrangeTracks, NULL, 0},
	{ { DEFACCEL, "SWS/BR: Focus tracks" },                                                                                                                "BR_FOCUS_TRACKS",                    FocusArrangeTracks, NULL, 1},

	{ { DEFACCEL, "SWS/BR: Move active floating window to mouse cursor (horizontal: left, vertical: bottom)" },                                            "BR_MOVE_WINDOW_TO_MOUSE_H_L_V_B",    MoveActiveWndToMouse, NULL, BINARY(0101)},
	{ { DEFACCEL, "SWS/BR: Move active floating window to mouse cursor (horizontal: left, vertical: middle)" },                                            "BR_MOVE_WINDOW_TO_MOUSE_H_L_V_M",    MoveActiveWndToMouse, NULL, BINARY(0100)},
	{ { DEFACCEL, "SWS/BR: Move active floating window to mouse cursor (horizontal: left, vertical: top)" },                                               "BR_MOVE_WINDOW_TO_MOUSE_H_L_V_T",    MoveActiveWndToMouse, NULL, BINARY(0111)},
	{ { DEFACCEL, "SWS/BR: Move active floating window to mouse cursor (horizontal: middle, vertical: bottom)" },                                          "BR_MOVE_WINDOW_TO_MOUSE_H_M_V_B",    MoveActiveWndToMouse, NULL, BINARY(0001)},
	{ { DEFACCEL, "SWS/BR: Move active floating window to mouse cursor (horizontal: middle, vertical: middle)" },                                          "BR_MOVE_WINDOW_TO_MOUSE_H_M_V_M",    MoveActiveWndToMouse, NULL, BINARY(0000)},
	{ { DEFACCEL, "SWS/BR: Move active floating window to mouse cursor (horizontal: middle, vertical: top)" },                                             "BR_MOVE_WINDOW_TO_MOUSE_H_M_V_T",    MoveActiveWndToMouse, NULL, BINARY(0011)},
	{ { DEFACCEL, "SWS/BR: Move active floating window to mouse cursor (horizontal: right, vertical: bottom)" },                                           "BR_MOVE_WINDOW_TO_MOUSE_H_R_V_B",    MoveActiveWndToMouse, NULL, BINARY(1101)},
	{ { DEFACCEL, "SWS/BR: Move active floating window to mouse cursor (horizontal: right, vertical: middle)" },                                           "BR_MOVE_WINDOW_TO_MOUSE_H_R_V_M",    MoveActiveWndToMouse, NULL, BINARY(1100)},
	{ { DEFACCEL, "SWS/BR: Move active floating window to mouse cursor (horizontal: right, vertical: top)" },                                              "BR_MOVE_WINDOW_TO_MOUSE_H_R_V_T",    MoveActiveWndToMouse, NULL, BINARY(1111)},

	{ { DEFACCEL, "SWS/BR: Toggle media item online/offline" },                                                                                            "BR_TOGGLE_ITEM_ONLINE",              ToggleItemOnline},
	{ { DEFACCEL, "SWS/BR: Copy take media source file path of selected items to clipboard" },                                                             "BR_TSOURCE_PATH_TO_CLIPBOARD",       ItemSourcePathToClipBoard},

	{ { DEFACCEL, "SWS/BR: Delete take under mouse cursor" },                                                                                              "BR_DELETE_TAKE_MOUSE",               DeleteTakeUnderMouse, NULL, 0},
	{ { DEFACCEL, "SWS/BR: Select TCP track under mouse cursor" },                                                                                         "BR_SEL_TCP_TRACK_MOUSE",             SelectTrackUnderMouse, NULL, 0},
	{ { DEFACCEL, "SWS/BR: Select MCP track under mouse cursor" },                                                                                         "BR_SEL_MCP_TRACK_MOUSE",             SelectTrackUnderMouse, NULL, 1},

	{ { DEFACCEL, "SWS/BR: Play from mouse cursor position" },                                                                                             "BR_PLAY_MOUSECURSOR",                PlaybackAtMouseCursor, NULL, 0},
	{ { DEFACCEL, "SWS/BR: Play/pause from mouse cursor position" },                                                                                       "BR_PLAY_PAUSE_MOUSECURSOR",          PlaybackAtMouseCursor, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Play/stop from mouse cursor position" },                                                                                        "BR_PLAY_STOP_MOUSECURSOR",           PlaybackAtMouseCursor, NULL, 2},

	{ { DEFACCEL, "SWS/BR: Select all audio items" },                                                                                                      "BR_SEL_ALL_ITEMS_AUDIO",             SelectItemsByType, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Select all MIDI items" },                                                                                                       "BR_SEL_ALL_ITEMS_MIDI",              SelectItemsByType, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Select all video items" },                                                                                                      "BR_SEL_ALL_ITEMS_VIDEO",             SelectItemsByType, NULL, 3},
	{ { DEFACCEL, "SWS/BR: Select all click source items" },                                                                                               "BR_SEL_ALL_ITEMS_CLICK",             SelectItemsByType, NULL, 4},
	{ { DEFACCEL, "SWS/BR: Select all timecode generator items" },                                                                                         "BR_SEL_ALL_ITEMS_TIMECODE",          SelectItemsByType, NULL, 5},
	{ { DEFACCEL, "SWS/BR: Select all subproject (PiP) items" },                                                                                           "BR_SEL_ALL_ITEMS_PIP",               SelectItemsByType, NULL, 6},
	{ { DEFACCEL, "SWS/BR: Select all empty items" },                                                                                                      "BR_SEL_ALL_ITEMS_EMPTY",             SelectItemsByType, NULL, 7},
	{ { DEFACCEL, "SWS/BR: Select all audio items (obey time selection, if any)" },                                                                        "BR_SEL_ALL_ITEMS_TIME_SEL_AUDIO",    SelectItemsByType, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Select all MIDI items (obey time selection, if any)" },                                                                         "BR_SEL_ALL_ITEMS_TIME_SEL_MIDI",     SelectItemsByType, NULL, -2},
	{ { DEFACCEL, "SWS/BR: Select all video items (obey time selection, if any)" },                                                                        "BR_SEL_ALL_ITEMS_TIME_SEL_VIDEO",    SelectItemsByType, NULL, -3},
	{ { DEFACCEL, "SWS/BR: Select all click source items (obey time selection, if any)" },                                                                 "BR_SEL_ALL_ITEMS_TIME_SEL_CLICK",    SelectItemsByType, NULL, -4},
	{ { DEFACCEL, "SWS/BR: Select all timecode items (obey time selection, if any)" },                                                                     "BR_SEL_ALL_ITEMS_TIME_SEL_TIMECODE", SelectItemsByType, NULL, -5},
	{ { DEFACCEL, "SWS/BR: Select all subproject (PiP) items (obey time selection, if any)" },                                                             "BR_SEL_ALL_ITEMS_TIME_SEL_PIP",      SelectItemsByType, NULL, -6},
	{ { DEFACCEL, "SWS/BR: Select all empty items (obey time selection, if any)" },                                                                        "BR_SEL_ALL_ITEMS_TIME_SEL_EMPTY",    SelectItemsByType, NULL, -7},

	{ { DEFACCEL, "SWS/BR: Save edit cursor position, slot 1" },                                                                                           "BR_SAVE_CURSOR_POS_SLOT_1",          SaveCursorPosSlot, NULL, 0},
	{ { DEFACCEL, "SWS/BR: Save edit cursor position, slot 2" },                                                                                           "BR_SAVE_CURSOR_POS_SLOT_2",          SaveCursorPosSlot, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Save edit cursor position, slot 3" },                                                                                           "BR_SAVE_CURSOR_POS_SLOT_3",          SaveCursorPosSlot, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Save edit cursor position, slot 4" },                                                                                           "BR_SAVE_CURSOR_POS_SLOT_4",          SaveCursorPosSlot, NULL, 3},
	{ { DEFACCEL, "SWS/BR: Save edit cursor position, slot 5" },                                                                                           "BR_SAVE_CURSOR_POS_SLOT_5",          SaveCursorPosSlot, NULL, 4},
	{ { DEFACCEL, "SWS/BR: Save edit cursor position, slot 6" },                                                                                           "BR_SAVE_CURSOR_POS_SLOT_6",          SaveCursorPosSlot, NULL, 5},
	{ { DEFACCEL, "SWS/BR: Save edit cursor position, slot 7" },                                                                                           "BR_SAVE_CURSOR_POS_SLOT_7",          SaveCursorPosSlot, NULL, 6},
	{ { DEFACCEL, "SWS/BR: Save edit cursor position, slot 8" },                                                                                           "BR_SAVE_CURSOR_POS_SLOT_8",          SaveCursorPosSlot, NULL, 7},
	{ { DEFACCEL, "SWS/BR: Restore edit cursor position, slot 1" },                                                                                        "BR_RESTORE_CURSOR_POS_SLOT_1",       RestoreCursorPosSlot, NULL, 0},
	{ { DEFACCEL, "SWS/BR: Restore edit cursor position, slot 2" },                                                                                        "BR_RESTORE_CURSOR_POS_SLOT_2",       RestoreCursorPosSlot, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Restore edit cursor position, slot 3" },                                                                                        "BR_RESTORE_CURSOR_POS_SLOT_3",       RestoreCursorPosSlot, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Restore edit cursor position, slot 4" },                                                                                        "BR_RESTORE_CURSOR_POS_SLOT_4",       RestoreCursorPosSlot, NULL, 3},
	{ { DEFACCEL, "SWS/BR: Restore edit cursor position, slot 5" },                                                                                        "BR_RESTORE_CURSOR_POS_SLOT_5",       RestoreCursorPosSlot, NULL, 4},
	{ { DEFACCEL, "SWS/BR: Restore edit cursor position, slot 6" },                                                                                        "BR_RESTORE_CURSOR_POS_SLOT_6",       RestoreCursorPosSlot, NULL, 5},
	{ { DEFACCEL, "SWS/BR: Restore edit cursor position, slot 7" },                                                                                        "BR_RESTORE_CURSOR_POS_SLOT_7",       RestoreCursorPosSlot, NULL, 6},
	{ { DEFACCEL, "SWS/BR: Restore edit cursor position, slot 8" },                                                                                        "BR_RESTORE_CURSOR_POS_SLOT_8",       RestoreCursorPosSlot, NULL, 7},

	{ { DEFACCEL, "SWS/BR: Save selected items' mute state, slot 1" },                                                                                     "BR_SAVE_SOLO_MUTE_SEL_ITEMS_SLOT_1",    SaveItemMuteStateSlot, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Save selected items' mute state, slot 2" },                                                                                     "BR_SAVE_SOLO_MUTE_SEL_ITEMS_SLOT_2",    SaveItemMuteStateSlot, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Save selected items' mute state, slot 3" },                                                                                     "BR_SAVE_SOLO_MUTE_SEL_ITEMS_SLOT_3",    SaveItemMuteStateSlot, NULL, 3},
	{ { DEFACCEL, "SWS/BR: Save selected items' mute state, slot 4" },                                                                                     "BR_SAVE_SOLO_MUTE_SEL_ITEMS_SLOT_4",    SaveItemMuteStateSlot, NULL, 4},
	{ { DEFACCEL, "SWS/BR: Save selected items' mute state, slot 5" },                                                                                     "BR_SAVE_SOLO_MUTE_SEL_ITEMS_SLOT_5",    SaveItemMuteStateSlot, NULL, 5},
	{ { DEFACCEL, "SWS/BR: Save selected items' mute state, slot 6" },                                                                                     "BR_SAVE_SOLO_MUTE_SEL_ITEMS_SLOT_6",    SaveItemMuteStateSlot, NULL, 6},
	{ { DEFACCEL, "SWS/BR: Save selected items' mute state, slot 7" },                                                                                     "BR_SAVE_SOLO_MUTE_SEL_ITEMS_SLOT_7",    SaveItemMuteStateSlot, NULL, 7},
	{ { DEFACCEL, "SWS/BR: Save selected items' mute state, slot 8" },                                                                                     "BR_SAVE_SOLO_MUTE_SEL_ITEMS_SLOT_8",    SaveItemMuteStateSlot, NULL, 8},
	{ { DEFACCEL, "SWS/BR: Save all items' mute state, slot 1" },                                                                                          "BR_SAVE_SOLO_MUTE_ALL_ITEMS_SLOT_1",    SaveItemMuteStateSlot, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Save all items' mute state, slot 2" },                                                                                          "BR_SAVE_SOLO_MUTE_ALL_ITEMS_SLOT_2",    SaveItemMuteStateSlot, NULL, -2},
	{ { DEFACCEL, "SWS/BR: Save all items' mute state, slot 3" },                                                                                          "BR_SAVE_SOLO_MUTE_ALL_ITEMS_SLOT_3",    SaveItemMuteStateSlot, NULL, -3},
	{ { DEFACCEL, "SWS/BR: Save all items' mute state, slot 4" },                                                                                          "BR_SAVE_SOLO_MUTE_ALL_ITEMS_SLOT_4",    SaveItemMuteStateSlot, NULL, -4},
	{ { DEFACCEL, "SWS/BR: Save all items' mute state, slot 5" },                                                                                          "BR_SAVE_SOLO_MUTE_ALL_ITEMS_SLOT_5",    SaveItemMuteStateSlot, NULL, -5},
	{ { DEFACCEL, "SWS/BR: Save all items' mute state, slot 6" },                                                                                          "BR_SAVE_SOLO_MUTE_ALL_ITEMS_SLOT_6",    SaveItemMuteStateSlot, NULL, -6},
	{ { DEFACCEL, "SWS/BR: Save all items' mute state, slot 7" },                                                                                          "BR_SAVE_SOLO_MUTE_ALL_ITEMS_SLOT_7",    SaveItemMuteStateSlot, NULL, -7},
	{ { DEFACCEL, "SWS/BR: Save all items' mute state, slot 8" },                                                                                          "BR_SAVE_SOLO_MUTE_ALL_ITEMS_SLOT_8",    SaveItemMuteStateSlot, NULL, -8},
	{ { DEFACCEL, "SWS/BR: Restore items' mute state to selected items, slot 1" },                                                                         "BR_RESTORE_SOLO_MUTE_SEL_ITEMS_SLOT_1", RestoreItemMuteStateSlot, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Restore items' mute state to selected items, slot 2" },                                                                         "BR_RESTORE_SOLO_MUTE_SEL_ITEMS_SLOT_2", RestoreItemMuteStateSlot, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Restore items' mute state to selected items, slot 3" },                                                                         "BR_RESTORE_SOLO_MUTE_SEL_ITEMS_SLOT_3", RestoreItemMuteStateSlot, NULL, 3},
	{ { DEFACCEL, "SWS/BR: Restore items' mute state to selected items, slot 4" },                                                                         "BR_RESTORE_SOLO_MUTE_SEL_ITEMS_SLOT_4", RestoreItemMuteStateSlot, NULL, 4},
	{ { DEFACCEL, "SWS/BR: Restore items' mute state to selected items, slot 5" },                                                                         "BR_RESTORE_SOLO_MUTE_SEL_ITEMS_SLOT_5", RestoreItemMuteStateSlot, NULL, 5},
	{ { DEFACCEL, "SWS/BR: Restore items' mute state to selected items, slot 6" },                                                                         "BR_RESTORE_SOLO_MUTE_SEL_ITEMS_SLOT_6", RestoreItemMuteStateSlot, NULL, 6},
	{ { DEFACCEL, "SWS/BR: Restore items' mute state to selected items, slot 7" },                                                                         "BR_RESTORE_SOLO_MUTE_SEL_ITEMS_SLOT_7", RestoreItemMuteStateSlot, NULL, 7},
	{ { DEFACCEL, "SWS/BR: Restore items' mute state to selected items, slot 8" },                                                                         "BR_RESTORE_SOLO_MUTE_SEL_ITEMS_SLOT_8", RestoreItemMuteStateSlot, NULL, 8},
	{ { DEFACCEL, "SWS/BR: Restore items' mute state to all items, slot 1" },                                                                              "BR_RESTORE_SOLO_MUTE_ALL_ITEMS_SLOT_1", RestoreItemMuteStateSlot, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Restore items' mute state to all items, slot 2" },                                                                              "BR_RESTORE_SOLO_MUTE_ALL_ITEMS_SLOT_2", RestoreItemMuteStateSlot, NULL, -2},
	{ { DEFACCEL, "SWS/BR: Restore items' mute state to all items, slot 3" },                                                                              "BR_RESTORE_SOLO_MUTE_ALL_ITEMS_SLOT_3", RestoreItemMuteStateSlot, NULL, -3},
	{ { DEFACCEL, "SWS/BR: Restore items' mute state to all items, slot 4" },                                                                              "BR_RESTORE_SOLO_MUTE_ALL_ITEMS_SLOT_4", RestoreItemMuteStateSlot, NULL, -4},
	{ { DEFACCEL, "SWS/BR: Restore items' mute state to all items, slot 5" },                                                                              "BR_RESTORE_SOLO_MUTE_ALL_ITEMS_SLOT_5", RestoreItemMuteStateSlot, NULL, -5},
	{ { DEFACCEL, "SWS/BR: Restore items' mute state to all items, slot 6" },                                                                              "BR_RESTORE_SOLO_MUTE_ALL_ITEMS_SLOT_6", RestoreItemMuteStateSlot, NULL, -6},
	{ { DEFACCEL, "SWS/BR: Restore items' mute state to all items, slot 7" },                                                                              "BR_RESTORE_SOLO_MUTE_ALL_ITEMS_SLOT_7", RestoreItemMuteStateSlot, NULL, -7},
	{ { DEFACCEL, "SWS/BR: Restore items' mute state to all items, slot 8" },                                                                              "BR_RESTORE_SOLO_MUTE_ALL_ITEMS_SLOT_8", RestoreItemMuteStateSlot, NULL, -8},

	{ { DEFACCEL, "SWS/BR: Save selected tracks' solo and mute state, slot 1" },                                                                           "BR_SAVE_SOLO_MUTE_SEL_TRACKS_SLOT_1",    SaveTrackSoloMuteStateSlot, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Save selected tracks' solo and mute state, slot 2" },                                                                           "BR_SAVE_SOLO_MUTE_SEL_TRACKS_SLOT_2",    SaveTrackSoloMuteStateSlot, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Save selected tracks' solo and mute state, slot 3" },                                                                           "BR_SAVE_SOLO_MUTE_SEL_TRACKS_SLOT_3",    SaveTrackSoloMuteStateSlot, NULL, 3},
	{ { DEFACCEL, "SWS/BR: Save selected tracks' solo and mute state, slot 4" },                                                                           "BR_SAVE_SOLO_MUTE_SEL_TRACKS_SLOT_4",    SaveTrackSoloMuteStateSlot, NULL, 4},
	{ { DEFACCEL, "SWS/BR: Save selected tracks' solo and mute state, slot 5" },                                                                           "BR_SAVE_SOLO_MUTE_SEL_TRACKS_SLOT_5",    SaveTrackSoloMuteStateSlot, NULL, 5},
	{ { DEFACCEL, "SWS/BR: Save selected tracks' solo and mute state, slot 6" },                                                                           "BR_SAVE_SOLO_MUTE_SEL_TRACKS_SLOT_6",    SaveTrackSoloMuteStateSlot, NULL, 6},
	{ { DEFACCEL, "SWS/BR: Save selected tracks' solo and mute state, slot 7" },                                                                           "BR_SAVE_SOLO_MUTE_SEL_TRACKS_SLOT_7",    SaveTrackSoloMuteStateSlot, NULL, 7},
	{ { DEFACCEL, "SWS/BR: Save selected tracks' solo and mute state, slot 8" },                                                                           "BR_SAVE_SOLO_MUTE_SEL_TRACKS_SLOT_8",    SaveTrackSoloMuteStateSlot, NULL, 8},
	{ { DEFACCEL, "SWS/BR: Save all tracks' solo and mute state, slot 1" },                                                                                "BR_SAVE_SOLO_MUTE_ALL_TRACKS_SLOT_1",    SaveTrackSoloMuteStateSlot, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Save all tracks' solo and mute state, slot 2" },                                                                                "BR_SAVE_SOLO_MUTE_ALL_TRACKS_SLOT_2",    SaveTrackSoloMuteStateSlot, NULL, -2},
	{ { DEFACCEL, "SWS/BR: Save all tracks' solo and mute state, slot 3" },                                                                                "BR_SAVE_SOLO_MUTE_ALL_TRACKS_SLOT_3",    SaveTrackSoloMuteStateSlot, NULL, -3},
	{ { DEFACCEL, "SWS/BR: Save all tracks' solo and mute state, slot 4" },                                                                                "BR_SAVE_SOLO_MUTE_ALL_TRACKS_SLOT_4",    SaveTrackSoloMuteStateSlot, NULL, -4},
	{ { DEFACCEL, "SWS/BR: Save all tracks' solo and mute state, slot 5" },                                                                                "BR_SAVE_SOLO_MUTE_ALL_TRACKS_SLOT_5",    SaveTrackSoloMuteStateSlot, NULL, -5},
	{ { DEFACCEL, "SWS/BR: Save all tracks' solo and mute state, slot 6" },                                                                                "BR_SAVE_SOLO_MUTE_ALL_TRACKS_SLOT_6",    SaveTrackSoloMuteStateSlot, NULL, -6},
	{ { DEFACCEL, "SWS/BR: Save all tracks' solo and mute state, slot 7" },                                                                                "BR_SAVE_SOLO_MUTE_ALL_TRACKS_SLOT_7",    SaveTrackSoloMuteStateSlot, NULL, -7},
	{ { DEFACCEL, "SWS/BR: Save all tracks' solo and mute state, slot 8" },                                                                                "BR_SAVE_SOLO_MUTE_ALL_TRACKS_SLOT_8",    SaveTrackSoloMuteStateSlot, NULL, -8},
	{ { DEFACCEL, "SWS/BR: Restore tracks' solo and mute state to selected tracks, slot 1" },                                                              "BR_RESTORE_SOLO_MUTE_SEL_TRACKS_SLOT_1", RestoreTrackSoloMuteStateSlot, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Restore tracks' solo and mute state to selected tracks, slot 2" },                                                              "BR_RESTORE_SOLO_MUTE_SEL_TRACKS_SLOT_2", RestoreTrackSoloMuteStateSlot, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Restore tracks' solo and mute state to selected tracks, slot 3" },                                                              "BR_RESTORE_SOLO_MUTE_SEL_TRACKS_SLOT_3", RestoreTrackSoloMuteStateSlot, NULL, 3},
	{ { DEFACCEL, "SWS/BR: Restore tracks' solo and mute state to selected tracks, slot 4" },                                                              "BR_RESTORE_SOLO_MUTE_SEL_TRACKS_SLOT_4", RestoreTrackSoloMuteStateSlot, NULL, 4},
	{ { DEFACCEL, "SWS/BR: Restore tracks' solo and mute state to selected tracks, slot 5" },                                                              "BR_RESTORE_SOLO_MUTE_SEL_TRACKS_SLOT_5", RestoreTrackSoloMuteStateSlot, NULL, 5},
	{ { DEFACCEL, "SWS/BR: Restore tracks' solo and mute state to selected tracks, slot 6" },                                                              "BR_RESTORE_SOLO_MUTE_SEL_TRACKS_SLOT_6", RestoreTrackSoloMuteStateSlot, NULL, 6},
	{ { DEFACCEL, "SWS/BR: Restore tracks' solo and mute state to selected tracks, slot 7" },                                                              "BR_RESTORE_SOLO_MUTE_SEL_TRACKS_SLOT_7", RestoreTrackSoloMuteStateSlot, NULL, 7},
	{ { DEFACCEL, "SWS/BR: Restore tracks' solo and mute state to selected tracks, slot 8" },                                                              "BR_RESTORE_SOLO_MUTE_SEL_TRACKS_SLOT_8", RestoreTrackSoloMuteStateSlot, NULL, 8},
	{ { DEFACCEL, "SWS/BR: Restore tracks' solo and mute state to all tracks, slot 1" },                                                                   "BR_RESTORE_SOLO_MUTE_ALL_TRACKS_SLOT_1", RestoreTrackSoloMuteStateSlot, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Restore tracks' solo and mute state to all tracks, slot 2" },                                                                   "BR_RESTORE_SOLO_MUTE_ALL_TRACKS_SLOT_2", RestoreTrackSoloMuteStateSlot, NULL, -2},
	{ { DEFACCEL, "SWS/BR: Restore tracks' solo and mute state to all tracks, slot 3" },                                                                   "BR_RESTORE_SOLO_MUTE_ALL_TRACKS_SLOT_3", RestoreTrackSoloMuteStateSlot, NULL, -3},
	{ { DEFACCEL, "SWS/BR: Restore tracks' solo and mute state to all tracks, slot 4" },                                                                   "BR_RESTORE_SOLO_MUTE_ALL_TRACKS_SLOT_4", RestoreTrackSoloMuteStateSlot, NULL, -4},
	{ { DEFACCEL, "SWS/BR: Restore tracks' solo and mute state to all tracks, slot 5" },                                                                   "BR_RESTORE_SOLO_MUTE_ALL_TRACKS_SLOT_5", RestoreTrackSoloMuteStateSlot, NULL, -5},
	{ { DEFACCEL, "SWS/BR: Restore tracks' solo and mute state to all tracks, slot 6" },                                                                   "BR_RESTORE_SOLO_MUTE_ALL_TRACKS_SLOT_6", RestoreTrackSoloMuteStateSlot, NULL, -6},
	{ { DEFACCEL, "SWS/BR: Restore tracks' solo and mute state to all tracks, slot 7" },                                                                   "BR_RESTORE_SOLO_MUTE_ALL_TRACKS_SLOT_7", RestoreTrackSoloMuteStateSlot, NULL, -7},
	{ { DEFACCEL, "SWS/BR: Restore tracks' solo and mute state to all tracks, slot 8" },                                                                   "BR_RESTORE_SOLO_MUTE_ALL_TRACKS_SLOT_8", RestoreTrackSoloMuteStateSlot, NULL, -8},

	/******************************************************************************
	* Misc - Media item preview                                                   *
	******************************************************************************/
	{ { DEFACCEL, "SWS/BR: Preview media item under mouse" },                                                                                          "BR_PREV_ITEM_CURSOR",                 PreviewItemAtMouse, NULL, 1111},
	{ { DEFACCEL, "SWS/BR: Preview media item under mouse (start from mouse cursor position)" },                                                       "BR_PREV_ITEM_CURSOR_POS",             PreviewItemAtMouse, NULL, 1121},
	{ { DEFACCEL, "SWS/BR: Preview media item under mouse (sync with next measure)" },                                                                 "BR_PREV_ITEM_CURSOR_SYNC",            PreviewItemAtMouse, NULL, 1131},
	{ { DEFACCEL, "SWS/BR: Preview media item under mouse and pause during preview" },                                                                 "BR_PREV_ITEM_PAUSE_CURSOR",           PreviewItemAtMouse, NULL, 1112},
	{ { DEFACCEL, "SWS/BR: Preview media item under mouse and pause during preview (start from mouse cursor position)" },                              "BR_PREV_ITEM_PAUSE_CURSOR_POS",       PreviewItemAtMouse, NULL, 1122},
	{ { DEFACCEL, "SWS/BR: Preview media item under mouse at track fader volume" },                                                                    "BR_PREV_ITEM_CURSOR_FADER",           PreviewItemAtMouse, NULL, 1211},
	{ { DEFACCEL, "SWS/BR: Preview media item under mouse at track fader volume (start from mouse position)" },                                        "BR_PREV_ITEM_CURSOR_FADER_POS",       PreviewItemAtMouse, NULL, 1221},
	{ { DEFACCEL, "SWS/BR: Preview media item under mouse at track fader volume (sync with next measure)" },                                           "BR_PREV_ITEM_CURSOR_FADER_SYNC",      PreviewItemAtMouse, NULL, 1231},
	{ { DEFACCEL, "SWS/BR: Preview media item under mouse at track fader volume and pause during preview" },                                           "BR_PREV_ITEM_PAUSE_CURSOR_FADER",     PreviewItemAtMouse, NULL, 1212},
	{ { DEFACCEL, "SWS/BR: Preview media item under mouse at track fader volume and pause during preview (start from mouse cursor position)" },        "BR_PREV_ITEM_PAUSE_CURSOR_FADER_POS", PreviewItemAtMouse, NULL, 1222},
	{ { DEFACCEL, "SWS/BR: Preview media item under mouse through track" },                                                                            "BR_PREV_ITEM_CURSOR_TRACK",           PreviewItemAtMouse, NULL, 1311},
	{ { DEFACCEL, "SWS/BR: Preview media item under mouse through track (start from mouse position)" },                                                "BR_PREV_ITEM_CURSOR_TRACK_POS",       PreviewItemAtMouse, NULL, 1321},
	{ { DEFACCEL, "SWS/BR: Preview media item under mouse through track (sync with next measure)" },                                                   "BR_PREV_ITEM_CURSOR_TRACK_SYNC",      PreviewItemAtMouse, NULL, 1331},
	{ { DEFACCEL, "SWS/BR: Preview media item under mouse through track and pause during preview" },                                                   "BR_PREV_ITEM_PAUSE_CURSOR_TRACK",     PreviewItemAtMouse, NULL, 1312},
	{ { DEFACCEL, "SWS/BR: Preview media item under mouse through track and pause during preview (start from mouse cursor position)" },                "BR_PREV_ITEM_PAUSE_CURSOR_TRACK_POS", PreviewItemAtMouse, NULL, 1322},

	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse" },                                                                                   "BR_TPREV_ITEM_CURSOR",                 PreviewItemAtMouse, NULL, 2111},
	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse (start from mouse position)" },                                                       "BR_TPREV_ITEM_CURSOR_POS",             PreviewItemAtMouse, NULL, 2121},
	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse (sync with next measure)" },                                                          "BR_TPREV_ITEM_CURSOR_SYNC",            PreviewItemAtMouse, NULL, 2131},
	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse and pause during preview" },                                                          "BR_TPREV_ITEM_PAUSE_CURSOR",           PreviewItemAtMouse, NULL, 2112},
	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse and pause during preview (start from mouse cursor position)" },                       "BR_TPREV_ITEM_PAUSE_CURSOR_POS",       PreviewItemAtMouse, NULL, 2122},
	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse at track fader volume" },                                                             "BR_TPREV_ITEM_CURSOR_FADER",           PreviewItemAtMouse, NULL, 2211},
	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse at track fader volume (start from mouse position)" },                                 "BR_TPREV_ITEM_CURSOR_FADER_POS",       PreviewItemAtMouse, NULL, 2221},
	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse at track fader volume (sync with next measure)" },                                    "BR_TPREV_ITEM_CURSOR_FADER_SYNC",      PreviewItemAtMouse, NULL, 2231},
	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse at track fader volume and pause during preview" },                                    "BR_TPREV_ITEM_PAUSE_CURSOR_FADER",     PreviewItemAtMouse, NULL, 2212},
	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse at track fader volume and pause during preview (start from mouse cursor position)" }, "BR_TPREV_ITEM_PAUSE_CURSOR_FADER_POS", PreviewItemAtMouse, NULL, 2222},
	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse through track" },                                                                     "BR_TPREV_ITEM_CURSOR_TRACK",           PreviewItemAtMouse, NULL, 2311},
	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse through track (start from mouse position)" },                                         "BR_TPREV_ITEM_CURSOR_TRACK_POS",       PreviewItemAtMouse, NULL, 2321},
	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse through track (sync with next measure)" },                                            "BR_TPREV_ITEM_CURSOR_TRACK_SYNC",      PreviewItemAtMouse, NULL, 2331},
	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse through track and pause during preview" },                                            "BR_TPREV_ITEM_PAUSE_CURSOR_TRACK",     PreviewItemAtMouse, NULL, 2312},
	{ { DEFACCEL, "SWS/BR: Toggle preview media item under mouse through track and pause during preview (start from mouse cursor position)" },         "BR_TPREV_ITEM_PAUSE_CURSOR_TRACK_POS", PreviewItemAtMouse, NULL, 2322},

	/******************************************************************************
	* Misc - Adjust playrate                                                      *
	******************************************************************************/
	{ { DEFACCEL, "SWS/BR: Adjust playrate (MIDI CC only)" }, "BR_ADJUST_PLAYRATE_MIDI",         NULL, NULL, 0, NULL,                           0, AdjustPlayrate},
	{ { DEFACCEL, "SWS/BR: Adjust playrate options..." },     "BR_ADJUST_PLAYRATE_MIDI_OPTIONS", NULL, NULL, 1, IsAdjustPlayrateOptionsVisible, 0, AdjustPlayrate},

	/******************************************************************************
	* Tempo                                                                       *
	******************************************************************************/
	{ { DEFACCEL, "SWS/BR: Move closest grid line to edit cursor" },            "BR_MOVE_GRID_TO_EDIT_CUR",   MoveGridToEditPlayCursor, NULL, 0},
	{ { DEFACCEL, "SWS/BR: Move closest grid line to play cursor" },            "BR_MOVE_GRID_TO_PLAY_CUR",   MoveGridToEditPlayCursor, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Move closest measure grid line to edit cursor" },    "BR_MOVE_M_GRID_TO_EDIT_CUR", MoveGridToEditPlayCursor, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Move closest measure grid line to play cursor" },    "BR_MOVE_M_GRID_TO_PLAY_CUR", MoveGridToEditPlayCursor, NULL, 3},
	{ { DEFACCEL, "SWS/BR: Move closest left side grid line to edit cursor" },  "BR_MOVE_L_GRID_TO_EDIT_CUR", MoveGridToEditPlayCursor, NULL, 4},
	{ { DEFACCEL, "SWS/BR: Move closest right side grid line to edit cursor" }, "BR_MOVE_R_GRID_TO_EDIT_CUR", MoveGridToEditPlayCursor, NULL, 5},

	{ { DEFACCEL, "SWS/BR: Move tempo marker forward 0.1 ms" },                                                               "SWS_BRMOVETEMPOFORWARD01",    MoveTempo, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Move tempo marker forward 1 ms" },                                                                 "SWS_BRMOVETEMPOFORWARD1",     MoveTempo, NULL, 10},
	{ { DEFACCEL, "SWS/BR: Move tempo marker forward 10 ms" },                                                                "SWS_BRMOVETEMPOFORWARD10",    MoveTempo, NULL, 100},
	{ { DEFACCEL, "SWS/BR: Move tempo marker forward 100 ms" },                                                               "SWS_BRMOVETEMPOFORWARD100",   MoveTempo, NULL, 1000},
	{ { DEFACCEL, "SWS/BR: Move tempo marker forward 1000 ms" },                                                              "SWS_BRMOVETEMPOFORWARD1000",  MoveTempo, NULL, 10000},
	{ { DEFACCEL, "SWS/BR: Move tempo marker back 0.1 ms" },                                                                  "SWS_BRMOVETEMPOBACK01",       MoveTempo, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Move tempo marker back 1 ms" },                                                                    "SWS_BRMOVETEMPOBACK1",        MoveTempo, NULL, -10},
	{ { DEFACCEL, "SWS/BR: Move tempo marker back 10 ms" },                                                                   "SWS_BRMOVETEMPOBACK10",       MoveTempo, NULL, -100},
	{ { DEFACCEL, "SWS/BR: Move tempo marker back 100 ms" },                                                                  "SWS_BRMOVETEMPOBACK100",      MoveTempo, NULL, -1000},
	{ { DEFACCEL, "SWS/BR: Move tempo marker back 1000 ms" },                                                                 "SWS_BRMOVETEMPOBACK1000",     MoveTempo, NULL, -10000},
	{ { DEFACCEL, "SWS/BR: Move tempo marker forward" },                                                                      "SWS_BRMOVETEMPOFORWARD",      MoveTempo, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Move tempo marker back" },                                                                         "SWS_BRMOVETEMPOBACK",         MoveTempo, NULL, -2},
	{ { DEFACCEL, "SWS/BR: Move closest tempo marker to edit cursor" },                                                       "BR_MOVE_CLOSEST_TEMPO",       MoveTempo, NULL, 3},

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

	{ { DEFACCEL, "SWS/BR: Select all partial time signature markers" },                                                      "BR_PART_TIME_SIG_SEL",        SelectMovePartialTimeSig, NULL, 0},
	{ { DEFACCEL, "SWS/BR: Reset position of selected partial time signature markers" },                                      "BR_PART_TIME_SIG_RESET",      SelectMovePartialTimeSig, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Snap position of selected partial time signature markers to closest grid line" },                  "BR_PART_TIME_SIG_SNAP_GRID",  SelectMovePartialTimeSig, NULL, 2},

	{ { DEFACCEL, "SWS/BR: Tempo help..." },                                                                                  "BR_TEMPO_HELP_WIKI",          OpenTempoWiki},

	{ { DEFACCEL, "SWS/BR: Convert project markers to tempo markers..." },                                                    "SWS_BRCONVERTMARKERSTOTEMPO", ConvertMarkersToTempoDialog, NULL, 0, IsConvertMarkersToTempoVisible},
	{ { DEFACCEL, "SWS/BR: Select and adjust tempo markers..." },                                                             "SWS_BRADJUSTSELTEMPO",        SelectAdjustTempoDialog, NULL, 0, IsSelectAdjustTempoVisible},
	{ { DEFACCEL, "SWS/BR: Randomize tempo markers..." },                                                                     "BR_RANDOMIZE_TEMPO",          RandomizeTempoDialog},

	{ { DEFACCEL, "SWS/BR: Set tempo marker shape (options)..." },                                                            "BR_TEMPO_SHAPE_OPTIONS",      TempoShapeOptionsDialog, NULL, 0, IsTempoShapeOptionsVisible},
	{ { DEFACCEL, "SWS/BR: Set tempo marker shape to linear (preserve positions)" },                                          "BR_TEMPO_SHAPE_LINEAR",       TempoShapeLinear},
	{ { DEFACCEL, "SWS/BR: Set tempo marker shape to square (preserve positions)" },                                          "BR_TEMPO_SHAPE_SQUARE",       TempoShapeSquare},

	{ {}, LAST_COMMAND}
};
//!WANT_LOCALIZE_1ST_STRING_END

int BR_Init ()
{
	SWSRegisterCommands(g_commandTable);

	ContextualToolbarsInit();
	ContinuousActionsInit();
	LoudnessInit();
	ProjStateInit();
	VersionCheckInit();

	return 1;
}

void BR_Exit ()
{
	LoudnessExit();
}

void BR_RegisterContinuousActions ()
{
	SetEnvPointMouseValueInit();
	PlaybackAtMouseCursorInit ();
	MoveGridToMouseInit();
}

int BR_GetSetActionToApply (bool set, int cmd)
{
	static int s_cmd = 0;
	if (set)
		s_cmd = cmd;
	return s_cmd;
}

bool BR_GlobalActionHook (int cmd, int val, int valhw, int relmode, HWND hwnd)
{
	static COMMAND_T* s_actionToRun = NULL;
	static int        s_lowCmd  = 0;
	static int        s_highCmd = 0;
	static set<int>   s_actions;

	static bool s_init = false;
	if (!s_init)
	{
		s_actions.insert(NamedCommandLookup("_BR_NEXT_CMD_SEL_TK_VIS_ENVS"));
		s_actions.insert(NamedCommandLookup("_BR_NEXT_CMD_SEL_TK_REC_ENVS"));
		s_actions.insert(NamedCommandLookup("_BR_NEXT_CMD_SEL_TK_VIS_ENVS_NOSEL"));
		s_actions.insert(NamedCommandLookup("_BR_NEXT_CMD_SEL_TK_REC_ENVS_NOSEL"));
		s_lowCmd  = (s_actions.size() > 0) ? *s_actions.begin()  : 0;
		s_highCmd = (s_actions.size() > 0) ? *s_actions.rbegin() : 0;
		s_init = true;
	}

	bool swallow = false;
	if (cmd >= s_lowCmd && cmd <= s_highCmd && s_actions.find(cmd) != s_actions.end())
	{
		s_actionToRun = SWSGetCommandByID(cmd);
		swallow = true;
	}
	else if (s_actionToRun && BR_GetSetActionToApply(false, 0) != 0)
	{
		COMMAND_T* actionToRun = s_actionToRun;
		s_actionToRun = NULL; // due to reentrancy, mark this as NULL before running the command

		BR_GetSetActionToApply(true, cmd);
		if      (actionToRun->doCommand) actionToRun->doCommand(actionToRun);
		else if (actionToRun->onAction)  actionToRun->onAction(actionToRun, val, valhw, relmode, hwnd);
		BR_GetSetActionToApply(true, 0);

		swallow = true;
	}
	return swallow;
}

bool BR_SwsActionHook (COMMAND_T* ct, int flagOrRelmode, HWND hwnd)
{
	return ContinuousActionHook(ct, flagOrRelmode, hwnd);
}

void BR_CSurfSetPlayState (bool play, bool pause, bool rec)
{
	static const vector<void(*)(bool,bool,bool)>* s_functions = NULL;
	if (!s_functions) RegisterCsurfPlayState(false, NULL, &s_functions);

	if (s_functions->size() > 0)
	{
		for (size_t i = 0; i < s_functions->size(); ++i)
		{
			if (void(*func)(bool,bool,bool) = s_functions->at(i))
				func(play, pause, rec);
		}
		RegisterCsurfPlayState(false, NULL, NULL, true);
	}
}

int BR_CSurfExtended(int call, void* parm1, void* parm2, void* parm3)
{
	if (call == CSURF_EXT_RESET)
	{
		LoudnessUpdate();
	}
	else if (call == CSURF_EXT_SETSENDVOLUME || call == CSURF_EXT_SETSENDPAN)
	{
		if (parm1 && parm2)
		{
			int type = (call == CSURF_EXT_SETSENDPAN) ? PAN : VOLUME;
			GetSetLastAdjustedSend(true, (MediaTrack**)&parm1, (int*)parm2, &type);
		}
	}
	return 0;
}

const char* BR_GetIniFile ()
{
	static WDL_FastString s_iniPath;
	if (s_iniPath.GetLength() == 0)
		s_iniPath.SetFormatted(SNM_MAX_PATH, "%s/BR.ini", GetResourcePath());

	return s_iniPath.Get();
}