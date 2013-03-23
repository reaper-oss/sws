/******************************************************************************
/ BR.cpp
/
/ Copyright (c) 2013 Dominik Martin Drzic
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
#include "BR_Tempo.h"
#include "BR_Misc.h"
#include "BR_Update.h"
#include "../reaper/localize.h"

//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] = 
{
	// Misc
	///////////////////////////////////////////////////////////////////////
	{ { DEFACCEL, "SWS/BR: Move edit cursor to next envelope point" },									"SWS_BRMOVEEDITTONEXTENV",			CursorToEnv, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Move edit cursor to next envelope point and select it" },					"SWS_BRMOVEEDITSELNEXTENV",			CursorToEnv, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Move edit cursor to next envelope point and add to selection" },				"SWS_BRMOVEEDITTONEXTENVADDSELL",	CursorToEnv, NULL, 3},
	
	{ { DEFACCEL, "SWS/BR: Move edit cursor to previous envelope point" },								"SWS_BRMOVEEDITTOPREVENV",			CursorToEnv, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Move edit cursor to previous envelope point and select it" },				"SWS_BRMOVEEDITSELPREVENV",			CursorToEnv, NULL, -2},
	{ { DEFACCEL, "SWS/BR: Move edit cursor to previous envelope point and add to selection" },			"SWS_BRMOVEEDITTOPREVENVADDSELL",	CursorToEnv, NULL, -3},
	
	{ { DEFACCEL, "SWS/BR: Split selected items at tempo markers" },									"SWS_BRSPLITSELECTEDTEMPO",			SplitItemAtTempo},
	{ { DEFACCEL, "SWS/BR: Create project markers from selected tempo markers" },						"BR_TEMPO_TO_MARKERS",				AddMarkerAtTempo},
	{ { DEFACCEL, "SWS/BR: Create project markers from notes in selected MIDI items" },					"BR_MIDI_NOTES_TO_MARKERS",			AddMarkerAtNotes},
	
	// Tempo
	///////////////////////////////////////////////////////////////////////
	{ { DEFACCEL, "SWS/BR: Convert project markers to tempo markers..." },								"SWS_BRCONVERTMARKERSTOTEMPO",		ConvertMarkersToTempoDialog, NULL, 0, IsConvertMarkersToTempoVisible},
	{ { DEFACCEL, "SWS/BR: Select and adjust tempo markers..." },										"SWS_BRADJUSTSELTEMPO",				SelectAdjustTempoDialog, NULL, 0, IsSelectAdjustTempoVisible},
	{ { DEFACCEL, "SWS/BR: Randomize selected tempo markers..." },										"BR_RANDOMIZE_TEMPO",				RandomizeTempoDialog},
	
	{ { DEFACCEL, "SWS/BR: Set tempo marker shape (options)..." },										"BR_TEMPO_SHAPE_OPTIONS",			TempoShapeOptionsDialog, NULL, 0, IsTempoShapeOptionsVisible},
	{ { DEFACCEL, "SWS/BR: Set tempo marker shape to linear (preserve positions)" },					"BR_TEMPO_SHAPE_LINEAR",			TempoShapeLinear},
	{ { DEFACCEL, "SWS/BR: Set tempo marker shape to square (preserve positions)" },					"BR_TEMPO_SHAPE_SQUARE",			TempoShapeSquare},
	{ { DEFACCEL, "SWS/BR: Delete tempo marker (preserve overall tempo and positions if possible)" },	"BR_DELETE_TEMPO",					DeleteTempo},
	
	{ { DEFACCEL, "SWS/BR: Move closest tempo marker to edit cursor" },									"BR_MOVE_CLOSEST_TEMPO",			MoveTempo, NULL, 3},
	{ { DEFACCEL, "SWS/BR: Move tempo marker forward" },												"SWS_BRMOVETEMPOFORWARD",			MoveTempo, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Move tempo marker back" },													"SWS_BRMOVETEMPOBACK",				MoveTempo, NULL, -2},
	{ { DEFACCEL, "SWS/BR: Move tempo marker forward 0.1 ms" },											"SWS_BRMOVETEMPOFORWARD01",			MoveTempo, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Move tempo marker forward 1 ms" },											"SWS_BRMOVETEMPOFORWARD1",			MoveTempo, NULL, 10},
	{ { DEFACCEL, "SWS/BR: Move tempo marker forward 10 ms"},											"SWS_BRMOVETEMPOFORWARD10",			MoveTempo, NULL, 100},
	{ { DEFACCEL, "SWS/BR: Move tempo marker forward 100 ms"},											"SWS_BRMOVETEMPOFORWARD100",		MoveTempo, NULL, 1000},
	{ { DEFACCEL, "SWS/BR: Move tempo marker forward 1000 ms" },										"SWS_BRMOVETEMPOFORWARD1000",		MoveTempo, NULL, 10000},
	{ { DEFACCEL, "SWS/BR: Move tempo marker back 0.1 ms"},												"SWS_BRMOVETEMPOBACK01",			MoveTempo, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Move tempo marker back 1 ms"},												"SWS_BRMOVETEMPOBACK1",				MoveTempo, NULL, -10},
	{ { DEFACCEL, "SWS/BR: Move tempo marker back 10 ms"},												"SWS_BRMOVETEMPOBACK10",			MoveTempo, NULL, -100},
	{ { DEFACCEL, "SWS/BR: Move tempo marker back 100 ms"},												"SWS_BRMOVETEMPOBACK100",			MoveTempo, NULL, -1000},
	{ { DEFACCEL, "SWS/BR: Move tempo marker back 1000 ms"},											"SWS_BRMOVETEMPOBACK1000",			MoveTempo, NULL, -10000},
	
	{ { DEFACCEL, "SWS/BR: Increase tempo marker 0.001 BPM (preserve it's time position)" },			"BR_INC_TEMPO_0.001_BPM",			EditTempo, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Increase tempo marker 0.01 BPM (preserve it's time position)" },				"BR_INC_TEMPO_0.01_BPM",			EditTempo, NULL, 10},
	{ { DEFACCEL, "SWS/BR: Increase tempo marker 0.1 BPM (preserve it's time position)" },				"BR_INC_TEMPO_0.1_BPM",				EditTempo, NULL, 100},
	{ { DEFACCEL, "SWS/BR: Increase tempo marker 01 BPM (preserve it's time position)" },				"BR_INC_TEMPO_1_BPM",				EditTempo, NULL, 1000},
	{ { DEFACCEL, "SWS/BR: Decrease tempo marker 0.001 BPM (preserve it's time position)" },			"BR_DEC_TEMPO_0.001_BPM",			EditTempo, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Decrease tempo marker 0.01 BPM (preserve it's time position)" },				"BR_DEC_TEMPO_0.01_BPM",			EditTempo, NULL, -10},
	{ { DEFACCEL, "SWS/BR: Decrease tempo marker 0.1 BPM (preserve it's time position)" },				"BR_DEC_TEMPO_0.1_BPM",				EditTempo, NULL, -100},
	{ { DEFACCEL, "SWS/BR: Decrease tempo marker 01 BPM (preserve it's time position)" },				"BR_DEC_TEMPO_1_BPM",				EditTempo, NULL, -1000},
	{ { DEFACCEL, "SWS/BR: Increase tempo marker 0.001% (preserve it's time position)" },				"BR_INC_TEMPO_0.001_PERC",			EditTempo, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Increase tempo marker 0.01% (preserve it's time position)" },				"BR_INC_TEMPO_0.01_PERC",			EditTempo, NULL, 20},
	{ { DEFACCEL, "SWS/BR: Increase tempo marker 0.1% (preserve it's time position)" },					"BR_INC_TEMPO_0.1_PERC",			EditTempo, NULL, 200},
	{ { DEFACCEL, "SWS/BR: Increase tempo marker 01% (preserve it's time position)" },					"BR_INC_TEMPO_1_PERC",				EditTempo, NULL, 2000},
	{ { DEFACCEL, "SWS/BR: Decrease tempo marker 0.001% (preserve it's time position)" },				"BR_DEC_TEMPO_0.001_PERC",			EditTempo, NULL, -2},
	{ { DEFACCEL, "SWS/BR: Decrease tempo marker 0.01% (preserve it's time position)" },				"BR_DEC_TEMPO_0.01_PERC",			EditTempo, NULL, -20},
	{ { DEFACCEL, "SWS/BR: Decrease tempo marker 0.1% (preserve it's time position)" },					"BR_DEC_TEMPO_0.1_PERC",			EditTempo, NULL, -200},
	{ { DEFACCEL, "SWS/BR: Decrease tempo marker 01% (preserve it's time position)" },					"BR_DEC_TEMPO_1_PERC",				EditTempo, NULL, -2000},
	
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (increase 0.001 BPM)" },					"BR_INC_GR_TEMPO_0.001_BPM",		EditTempoGradual, NULL, 1},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (increase 0.01 BPM)" },					"BR_INC_GR_TEMPO_0.01_BPM",			EditTempoGradual, NULL, 10},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (increase 0.1 BPM)" },					"BR_INC_GR_TEMPO_0.1_BPM",			EditTempoGradual, NULL, 100},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (increase 01 BPM)" },					"BR_INC_GR_TEMPO_1_BPM",			EditTempoGradual, NULL, 1000},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (decrease 0.001 BPM)" },					"BR_DEC_GR_TEMPO_0.001_BPM",		EditTempoGradual, NULL, -1},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (decrease 0.01 BPM)" },					"BR_DEC_GR_TEMPO_0.01_BPM",			EditTempoGradual, NULL, -10},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (decrease 0.1 BPM)" },					"BR_DEC_GR_TEMPO_0.1_BPM",			EditTempoGradual, NULL, -100},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (decrease 01 BPM)" },					"BR_DEC_GR_TEMPO_1_BPM",			EditTempoGradual, NULL, -1000},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (increase 0.001%)" },					"BR_INC_GR_TEMPO_0.001_PERC",		EditTempoGradual, NULL, 2},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (increase 0.01%)" },						"BR_INC_GR_TEMPO_0.01_PERC",		EditTempoGradual, NULL, 20},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (increase 0.1%)" },						"BR_INC_GR_TEMPO_0.1_PERC",			EditTempoGradual, NULL, 200},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (increase 01%)" },						"BR_INC_GR_TEMPO_1_PERC",			EditTempoGradual, NULL, 2000},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (decrease 0.001%)" },					"BR_DEC_GR_TEMPO_0.001_PERC",		EditTempoGradual, NULL, -2},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (decrease 0.01%)" },						"BR_DEC_GR_TEMPO_0.01_PERC",		EditTempoGradual, NULL, -20},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (decrease 0.1%)" },						"BR_DEC_GR_TEMPO_0.1_PERC",			EditTempoGradual, NULL, -200},
	{ { DEFACCEL, "SWS/BR: Alter slope of gradual tempo marker (decrease 01%)" },						"BR_DEC_GR_TEMPO_1_PERC",			EditTempoGradual, NULL, -2000},
	
	{ {}, LAST_COMMAND, },
};
//!WANT_LOCALIZE_1ST_STRING_END

int BreederInit()
{
	SWSRegisterCommands(g_commandTable);
	VersionCheckInit();
	return 1;
};