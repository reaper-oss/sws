/******************************************************************************
/ wol.cpp
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
#include "wol.h"
#include "wol_Envelope.h"
#include "wol_Misc.h"
#include "wol_Util.h"
#include "wol_Zoom.h"
#include "../reaper/localize.h"

//---------//

static COMMAND_T g_commandTable[] =
{
//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions

		///////////////////////////////////////////////////////////////////////////////////////////////////
		// Zoom
		///////////////////////////////////////////////////////////////////////////////////////////////////
		{ { DEFACCEL, "SWS/wol: Set \"Vertical zoom center\" to \"Track at center of view\"" }, "WOL_SETVZOOMC_TRACKCVIEW", SetVerticalZoomCenter, NULL, 0},
		{ { DEFACCEL, "SWS/wol: Set \"Vertical zoom center\" to \"Top visible track\"" }, "WOL_SETVZOOMC_TOPVISTRACK", SetVerticalZoomCenter, NULL, 1},
		{ { DEFACCEL, "SWS/wol: Set \"Vertical zoom center\" to \"Last selected track\"" }, "WOL_SETVZOOMC_LASTSELTRACK", SetVerticalZoomCenter, NULL, 2},
		{ { DEFACCEL, "SWS/wol: Set \"Vertical zoom center\" to \"Track under mouse cursor\"" }, "WOL_SETVZOOMC_TRACKMOUSECUR", SetVerticalZoomCenter, NULL, 3},

		{ { DEFACCEL, "SWS/wol: Set \"Horizontal zoom center\" to \"Edit cursor or play cursor (default)\"" }, "WOL_SETHZOOMC_EDITPLAYCUR", SetHorizontalZoomCenter, NULL, 0},
		{ { DEFACCEL, "SWS/wol: Set \"Horizontal zoom center\" to \"Edit cursor\"" }, "WOL_SETHZOOMC_EDITCUR", SetHorizontalZoomCenter, NULL, 1},
		{ { DEFACCEL, "SWS/wol: Set \"Horizontal zoom center\" to \"Center of view\"" }, "WOL_SETHZOOMC_CENTERVIEW", SetHorizontalZoomCenter, NULL, 2},
		{ { DEFACCEL, "SWS/wol: Set \"Horizontal zoom center\" to \"Mouse cursor\"" }, "WOL_SETHZOOMC_MOUSECUR", SetHorizontalZoomCenter, NULL, 3},

		{ { DEFACCEL, "SWS/wol: Set selected envelope height to default" }, "WOL_SETSELENVHDEF", SetVerticalZoomSelectedEnvelope, NULL, 0 },
		{ { DEFACCEL, "SWS/wol: Set selected envelope height to minimum" }, "WOL_SETSELENVHMIN", SetVerticalZoomSelectedEnvelope, NULL, 1 },
		{ { DEFACCEL, "SWS/wol: Set selected envelope height to maximum" }, "WOL_SETSELENVHMAX", SetVerticalZoomSelectedEnvelope, NULL, 2 },
		{ { DEFACCEL, "SWS/wol: Adjust selected envelope height (MIDI CC relative/mousewheel)" }, "WOL_ADJSELENVH", NULL, NULL, 0, NULL, 0, AdjustSelectedEnvelopeOrTrackHeight, },
		{ { DEFACCEL, "SWS/wol: Adjust selected envelope or last touched track height (MIDI CC relative/mousewheel)" }, "WOL_ADJSELENVTRH", NULL, NULL, 1, NULL, 0, AdjustSelectedEnvelopeOrTrackHeight, },
		{ { DEFACCEL, "SWS/wol: Toggle enable extended zoom for envelopes in track lane" }, "WOL_TENEXTZENVTRL", ToggleEnableEnvelopesExtendedZoom, NULL, 0, IsEnvelopesExtendedZoomEnabled },
		{ { DEFACCEL, "SWS/wol: Toggle enable envelope overlap for envelopes in track lane" }, "WOL_TENENVOLENVTRL", ToggleEnableEnvelopeOverlap, NULL, 0, IsEnvelopeOverlapEnabled },
		{ { DEFACCEL, "SWS/wol: Force overlap for selected envelope in track lane in its track height" }, "WOL_FENVOL", ForceEnvelopeOverlap, NULL, 0 },
		{ { DEFACCEL, "SWS/wol: Restore previous envelope overlap settings" }, "WOL_RESENVOLSET", ForceEnvelopeOverlap, NULL, 1 },
		{ { DEFACCEL, "SWS/wol: Horizontal zoom to selected envelope in time selection" }, "WOL_HZOOMSELENVTIMESEL", ZoomSelectedEnvelopeTimeSelection, NULL, 0 },
		{ { DEFACCEL, "SWS/wol: Full zoom to selected envelope in time selection" }, "WOL_FZOOMSELENVTIMESEL", ZoomSelectedEnvelopeTimeSelection, NULL, 1 },

		{ { DEFACCEL, "SWS/wol: Save selected envelope height, slot 1" }, "WOL_SAVESELENVHSLOT1", SaveRestoreSelectedEnvelopeHeightSlot, NULL, 0 },
		{ { DEFACCEL, "SWS/wol: Save selected envelope height, slot 2" }, "WOL_SAVESELENVHSLOT2", SaveRestoreSelectedEnvelopeHeightSlot, NULL, 1 },
		{ { DEFACCEL, "SWS/wol: Save selected envelope height, slot 3" }, "WOL_SAVESELENVHSLOT3", SaveRestoreSelectedEnvelopeHeightSlot, NULL, 2 },
		{ { DEFACCEL, "SWS/wol: Save selected envelope height, slot 4" }, "WOL_SAVESELENVHSLOT4", SaveRestoreSelectedEnvelopeHeightSlot, NULL, 3 },
		{ { DEFACCEL, "SWS/wol: Save selected envelope height, slot 5" }, "WOL_SAVESELENVHSLOT5", SaveRestoreSelectedEnvelopeHeightSlot, NULL, 4 },
		{ { DEFACCEL, "SWS/wol: Save selected envelope height, slot 6" }, "WOL_SAVESELENVHSLOT6", SaveRestoreSelectedEnvelopeHeightSlot, NULL, 5 },
		{ { DEFACCEL, "SWS/wol: Save selected envelope height, slot 7" }, "WOL_SAVESELENVHSLOT7", SaveRestoreSelectedEnvelopeHeightSlot, NULL, 6 },
		{ { DEFACCEL, "SWS/wol: Save selected envelope height, slot 8" }, "WOL_SAVESELENVHSLOT8", SaveRestoreSelectedEnvelopeHeightSlot, NULL, 7 },
		{ { DEFACCEL, "SWS/wol: Restore selected envelope height, slot 1" }, "WOL_RESSELENVHSLOT1", SaveRestoreSelectedEnvelopeHeightSlot, NULL, 8 },
		{ { DEFACCEL, "SWS/wol: Restore selected envelope height, slot 2" }, "WOL_RESSELENVHSLOT2", SaveRestoreSelectedEnvelopeHeightSlot, NULL, 9 },
		{ { DEFACCEL, "SWS/wol: Restore selected envelope height, slot 3" }, "WOL_RESSELENVHSLOT3", SaveRestoreSelectedEnvelopeHeightSlot, NULL, 10 },
		{ { DEFACCEL, "SWS/wol: Restore selected envelope height, slot 4" }, "WOL_RESSELENVHSLOT4", SaveRestoreSelectedEnvelopeHeightSlot, NULL, 11 },
		{ { DEFACCEL, "SWS/wol: Restore selected envelope height, slot 5" }, "WOL_RESSELENVHSLOT5", SaveRestoreSelectedEnvelopeHeightSlot, NULL, 12 },
		{ { DEFACCEL, "SWS/wol: Restore selected envelope height, slot 6" }, "WOL_RESSELENVHSLOT6", SaveRestoreSelectedEnvelopeHeightSlot, NULL, 13 },
		{ { DEFACCEL, "SWS/wol: Restore selected envelope height, slot 7" }, "WOL_RESSELENVHSLOT7", SaveRestoreSelectedEnvelopeHeightSlot, NULL, 14 },
		{ { DEFACCEL, "SWS/wol: Restore selected envelope height, slot 8" }, "WOL_RESSELENVHSLOT8", SaveRestoreSelectedEnvelopeHeightSlot, NULL, 15 },

		{ { DEFACCEL, "SWS/wol: Save selected envelope and its height in envelope heights list" }, "WOL_SAVESELENVHLIST", EnvelopeHeightList, NULL, 0 },
		{ { DEFACCEL, "SWS/wol: Restore selected envelope height from envelope heights list" }, "WOL_RESTSELENVHLIST", EnvelopeHeightList, NULL, 1 },
		{ { DEFACCEL, "SWS/wol: Remove selected envelope from envelope heights list" }, "WOL_REMSELENVHLIST", EnvelopeHeightList, NULL, 2 },

		///////////////////////////////////////////////////////////////////////////////////////////////////
		// Track
		///////////////////////////////////////////////////////////////////////////////////////////////////
		{ { DEFACCEL, "SWS/wol: Select all tracks except folder parents" }, "WOL_SELTREXCFOLDPAR", SelectAllTracksExceptFolderParents, NULL },

		///////////////////////////////////////////////////////////////////////////////////////////////////
		// Envelope
		///////////////////////////////////////////////////////////////////////////////////////////////////
		{ { DEFACCEL, "SWS/wol: Select the closest envelope point to the left side of mouse cursor" }, "WOL_SELCLOSEENVPTLEFTMOUSE", SelectClosestEnvelopePointMouseCursor, NULL, -1 },
		{ { DEFACCEL, "SWS/wol: Select the closest envelope point to the left side of mouse cursor and move edit cursor" }, "WOL_SELCLOSEENVPTLEFTMOUSEMOVEEDITC", SelectClosestEnvelopePointMouseCursor, NULL, -2 },
		{ { DEFACCEL, "SWS/wol: Select the closest envelope point to the right side of mouse cursor" }, "WOL_SELCLOSEENVPTRIGHTMOUSE", SelectClosestEnvelopePointMouseCursor, NULL, 1 },
		{ { DEFACCEL, "SWS/wol: Select the closest envelope point to the right side of mouse cursor and move edit cursor" }, "WOL_SELCLOSEENVPTRIGHTMOUSEMOVEEDITC", SelectClosestEnvelopePointMouseCursor, NULL, 2 },

		///////////////////////////////////////////////////////////////////////////////////////////////////
		// Original ReaScript/EEL scripts written by user spk77.
		// All credits to spk77 http://forum.cockos.com/member.php?u=49553
		// Thanks so much!
		///////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////
		// Navigation
		///////////////////////////////////////////////////////////////////////////////////////////////////
		{ { DEFACCEL, "SWS/wol-spk77: Move edit cursor to beat 1" }, "WOL_MOVEEDCURTOBEAT1", MoveEditCursorToBeatN, NULL, 1 },
		{ { DEFACCEL, "SWS/wol-spk77: Move edit cursor to beat 2" }, "WOL_MOVEEDCURTOBEAT2", MoveEditCursorToBeatN, NULL, 2 },
		{ { DEFACCEL, "SWS/wol-spk77: Move edit cursor to beat 3" }, "WOL_MOVEEDCURTOBEAT3", MoveEditCursorToBeatN, NULL, 3 },
		{ { DEFACCEL, "SWS/wol-spk77: Move edit cursor to beat 4" }, "WOL_MOVEEDCURTOBEAT4", MoveEditCursorToBeatN, NULL, 4 },
		{ { DEFACCEL, "SWS/wol-spk77: Move edit cursor to beat 5" }, "WOL_MOVEEDCURTOBEAT5", MoveEditCursorToBeatN, NULL, 5 },
		{ { DEFACCEL, "SWS/wol-spk77: Move edit cursor to beat 6" }, "WOL_MOVEEDCURTOBEAT6", MoveEditCursorToBeatN, NULL, 6 },
		{ { DEFACCEL, "SWS/wol-spk77: Move edit cursor to beat 7" }, "WOL_MOVEEDCURTOBEAT7", MoveEditCursorToBeatN, NULL, 7 },
		{ { DEFACCEL, "SWS/wol-spk77: Move edit cursor to beat 8" }, "WOL_MOVEEDCURTOBEAT8", MoveEditCursorToBeatN, NULL, 8 },
		{ { DEFACCEL, "SWS/wol-spk77: Move edit cursor to beat 9" }, "WOL_MOVEEDCURTOBEAT9", MoveEditCursorToBeatN, NULL, 9 },
		{ { DEFACCEL, "SWS/wol-spk77: Move edit cursor to beat 10" }, "WOL_MOVEEDCURTOBEAT10", MoveEditCursorToBeatN, NULL, 10 },
		{ { DEFACCEL, "SWS/wol-spk77: Move edit cursor to beat 11" }, "WOL_MOVEEDCURTOBEAT11", MoveEditCursorToBeatN, NULL, 11 },
		{ { DEFACCEL, "SWS/wol-spk77: Move edit cursor to beat 12" }, "WOL_MOVEEDCURTOBEAT12", MoveEditCursorToBeatN, NULL, 12 },
		{ { DEFACCEL, "SWS/wol-spk77: Move edit cursor to beat 13" }, "WOL_MOVEEDCURTOBEAT13", MoveEditCursorToBeatN, NULL, 13 },
		{ { DEFACCEL, "SWS/wol-spk77: Move edit cursor to beat 14" }, "WOL_MOVEEDCURTOBEAT14", MoveEditCursorToBeatN, NULL, 14 },
		{ { DEFACCEL, "SWS/wol-spk77: Move edit cursor to beat 15" }, "WOL_MOVEEDCURTOBEAT15", MoveEditCursorToBeatN, NULL, 15 },

		{ { DEFACCEL, "SWS/wol-spk77: Move edit cursor to closest beat" }, "WOL_MOVEEDCURTOCLOSESTBEAT", MoveEditCursorTo, NULL, 0 },
		{ { DEFACCEL, "SWS/wol-spk77: Move edit cursor to next beat in current measure (cycle)" }, "WOL_MOVEEDCURTONEXTBEATCYCLE", MoveEditCursorTo, NULL, 1 },
		{ { DEFACCEL, "SWS/wol-spk77: Move edit cursor to next beat" }, "WOL_MOVEEDCURTONEXTBEAT", MoveEditCursorTo, NULL, 2 },
		{ { DEFACCEL, "SWS/wol-spk77: Move edit cursor to next frame/beat (depends on ruler settings)" }, "WOL_MOVEEDCURNEXTFRAMEBEAT", MoveEditCursorTo, NULL, 3 },
		{ { DEFACCEL, "SWS/wol-spk77: Move edit cursor to previous beat in current measure (cycle)" }, "WOL_MOVEEDCURTOPREVBEATCYCLE", MoveEditCursorTo, NULL, -1 },
		{ { DEFACCEL, "SWS/wol-spk77: Move edit cursor to previous beat" }, "WOL_MOVEEDCURTOPREVBEAT", MoveEditCursorTo, NULL, -2 },
		{ { DEFACCEL, "SWS/wol-spk77: Move edit cursor to previous frame/beat (depends on ruler settings)" }, "WOL_MOVEEDCURPREVFRAMEBEAT", MoveEditCursorTo, NULL, -3 },

		///////////////////////////////////////////////////////////////////////////////////////////////////
		// Midi
		///////////////////////////////////////////////////////////////////////////////////////////////////
		{ { DEFACCEL, "SWS/wol-spk77: Move edit cursor to start of previous note" }, "WOL_MOVEEDCURTOPREVNOTE", NULL, NULL, -1, NULL, 32060, MoveEditCursorToNote },
		{ { DEFACCEL, "SWS/wol-spk77: Move edit cursor to start of next note" }, "WOL_MOVEEDCURTONEXTNOTE", NULL, NULL, 1, NULL, 32060, MoveEditCursorToNote },

		{ { DEFACCEL, "SWS/wol-spk77: Randomize selected midi velocities tool" }, "WOL_RANDSELMIDIVEL", RandomizeSelectedMidiVelocitiesTool, NULL, 0, IsRandomizeSelectedMidiVelocitiesOpen },
		{ { DEFACCEL, "SWS/wol-spk77: Randomize selected midi velocities, slot 1" }, "WOL_RANDSELMIDIVELS1", RandomizeSelectedMidiVelocitiesTool, NULL, 1 },
		{ { DEFACCEL, "SWS/wol-spk77: Randomize selected midi velocities, slot 2" }, "WOL_RANDSELMIDIVELS2", RandomizeSelectedMidiVelocitiesTool, NULL, 2 },
		{ { DEFACCEL, "SWS/wol-spk77: Randomize selected midi velocities, slot 3" }, "WOL_RANDSELMIDIVELS3", RandomizeSelectedMidiVelocitiesTool, NULL, 3 },
		{ { DEFACCEL, "SWS/wol-spk77: Randomize selected midi velocities, slot 4" }, "WOL_RANDSELMIDIVELS4", RandomizeSelectedMidiVelocitiesTool, NULL, 4 },
		{ { DEFACCEL, "SWS/wol-spk77: Randomize selected midi velocities, slot 5" }, "WOL_RANDSELMIDIVELS5", RandomizeSelectedMidiVelocitiesTool, NULL, 5 },
		{ { DEFACCEL, "SWS/wol-spk77: Randomize selected midi velocities, slot 6" }, "WOL_RANDSELMIDIVELS6", RandomizeSelectedMidiVelocitiesTool, NULL, 6 },
		{ { DEFACCEL, "SWS/wol-spk77: Randomize selected midi velocities, slot 7" }, "WOL_RANDSELMIDIVELS7", RandomizeSelectedMidiVelocitiesTool, NULL, 7 },
		{ { DEFACCEL, "SWS/wol-spk77: Randomize selected midi velocities, slot 8" }, "WOL_RANDSELMIDIVELS8", RandomizeSelectedMidiVelocitiesTool, NULL, 8 },

		{ { DEFACCEL, "SWS/wol-spk77: Select midi notes by velocities in range tool" }, "WOL_SELMIDIBYVELRANGE", SelectMidiNotesByVelocitiesInRangeTool, NULL, 0, IsSelectMidiNotesByVelocitiesInRangeOpen },
		{ { DEFACCEL, "SWS/wol-spk77: Select midi notes by velocities in range, slot 1" }, "WOL_SELMIDIBYVELRANGES1", SelectMidiNotesByVelocitiesInRangeTool, NULL, 1 },
		{ { DEFACCEL, "SWS/wol-spk77: Select midi notes by velocities in range, slot 2" }, "WOL_SELMIDIBYVELRANGES2", SelectMidiNotesByVelocitiesInRangeTool, NULL, 2 },
		{ { DEFACCEL, "SWS/wol-spk77: Select midi notes by velocities in range, slot 3" }, "WOL_SELMIDIBYVELRANGES3", SelectMidiNotesByVelocitiesInRangeTool, NULL, 3 },
		{ { DEFACCEL, "SWS/wol-spk77: Select midi notes by velocities in range, slot 4" }, "WOL_SELMIDIBYVELRANGES4", SelectMidiNotesByVelocitiesInRangeTool, NULL, 4 },
		{ { DEFACCEL, "SWS/wol-spk77: Select midi notes by velocities in range, slot 5" }, "WOL_SELMIDIBYVELRANGES5", SelectMidiNotesByVelocitiesInRangeTool, NULL, 5 },
		{ { DEFACCEL, "SWS/wol-spk77: Select midi notes by velocities in range, slot 6" }, "WOL_SELMIDIBYVELRANGES6", SelectMidiNotesByVelocitiesInRangeTool, NULL, 6 },
		{ { DEFACCEL, "SWS/wol-spk77: Select midi notes by velocities in range, slot 7" }, "WOL_SELMIDIBYVELRANGES7", SelectMidiNotesByVelocitiesInRangeTool, NULL, 7 },
		{ { DEFACCEL, "SWS/wol-spk77: Select midi notes by velocities in range, slot 8" }, "WOL_SELMIDIBYVELRANGES8", SelectMidiNotesByVelocitiesInRangeTool, NULL, 8 },
		{ { DEFACCEL, "SWS/wol-spk77: Select midi notes by velocities in range (add to selection), slot 1" }, "WOL_SELMIDIBYVELRANGEADDS1", SelectMidiNotesByVelocitiesInRangeTool, NULL, 9 },
		{ { DEFACCEL, "SWS/wol-spk77: Select midi notes by velocities in range (add to selection), slot 2" }, "WOL_SELMIDIBYVELRANGEADDS2", SelectMidiNotesByVelocitiesInRangeTool, NULL, 10 },
		{ { DEFACCEL, "SWS/wol-spk77: Select midi notes by velocities in range (add to selection), slot 3" }, "WOL_SELMIDIBYVELRANGEADDS3", SelectMidiNotesByVelocitiesInRangeTool, NULL, 11 },
		{ { DEFACCEL, "SWS/wol-spk77: Select midi notes by velocities in range (add to selection), slot 4" }, "WOL_SELMIDIBYVELRANGEADDS4", SelectMidiNotesByVelocitiesInRangeTool, NULL, 12 },
		{ { DEFACCEL, "SWS/wol-spk77: Select midi notes by velocities in range (add to selection), slot 5" }, "WOL_SELMIDIBYVELRANGEADDS5", SelectMidiNotesByVelocitiesInRangeTool, NULL, 13 },
		{ { DEFACCEL, "SWS/wol-spk77: Select midi notes by velocities in range (add to selection), slot 6" }, "WOL_SELMIDIBYVELRANGEADDS6", SelectMidiNotesByVelocitiesInRangeTool, NULL, 14 },
		{ { DEFACCEL, "SWS/wol-spk77: Select midi notes by velocities in range (add to selection), slot 7" }, "WOL_SELMIDIBYVELRANGEADDS7", SelectMidiNotesByVelocitiesInRangeTool, NULL, 15 },
		{ { DEFACCEL, "SWS/wol-spk77: Select midi notes by velocities in range (add to selection), slot 8" }, "WOL_SELMIDIBYVELRANGEADDS8", SelectMidiNotesByVelocitiesInRangeTool, NULL, 16 },

		{ { DEFACCEL, "SWS/wol-spk77: Select random midi notes tool" }, "WOL_SELRANDMIDINOTES", SelectRandomMidiNotesTool, NULL, 0, IsSelectRandomMidiNotesOpen },
		{ { DEFACCEL, "SWS/wol-spk77: Select random midi notes, slot 1" }, "WOL_SELRANDMIDINOTESS1", SelectRandomMidiNotesTool, NULL, 1 },
		{ { DEFACCEL, "SWS/wol-spk77: Select random midi notes, slot 2" }, "WOL_SELRANDMIDINOTESS2", SelectRandomMidiNotesTool, NULL, 2 },
		{ { DEFACCEL, "SWS/wol-spk77: Select random midi notes, slot 3" }, "WOL_SELRANDMIDINOTESS3", SelectRandomMidiNotesTool, NULL, 3 },
		{ { DEFACCEL, "SWS/wol-spk77: Select random midi notes, slot 4" }, "WOL_SELRANDMIDINOTESS4", SelectRandomMidiNotesTool, NULL, 4 },
		{ { DEFACCEL, "SWS/wol-spk77: Select random midi notes, slot 5" }, "WOL_SELRANDMIDINOTESS5", SelectRandomMidiNotesTool, NULL, 5 },
		{ { DEFACCEL, "SWS/wol-spk77: Select random midi notes, slot 6" }, "WOL_SELRANDMIDINOTESS6", SelectRandomMidiNotesTool, NULL, 6 },
		{ { DEFACCEL, "SWS/wol-spk77: Select random midi notes, slot 7" }, "WOL_SELRANDMIDINOTESS7", SelectRandomMidiNotesTool, NULL, 7 },
		{ { DEFACCEL, "SWS/wol-spk77: Select random midi notes, slot 8" }, "WOL_SELRANDMIDINOTESS8", SelectRandomMidiNotesTool, NULL, 8 },
		{ { DEFACCEL, "SWS/wol-spk77: Select random midi notes (add to selection), slot 1" }, "WOL_SELRANDMIDINOTESADDS1", SelectRandomMidiNotesTool, NULL, 9 },
		{ { DEFACCEL, "SWS/wol-spk77: Select random midi notes (add to selection), slot 2" }, "WOL_SELRANDMIDINOTESADDS2", SelectRandomMidiNotesTool, NULL, 10 },
		{ { DEFACCEL, "SWS/wol-spk77: Select random midi notes (add to selection), slot 3" }, "WOL_SELRANDMIDINOTESADDS3", SelectRandomMidiNotesTool, NULL, 11 },
		{ { DEFACCEL, "SWS/wol-spk77: Select random midi notes (add to selection), slot 4" }, "WOL_SELRANDMIDINOTESADDS4", SelectRandomMidiNotesTool, NULL, 12 },
		{ { DEFACCEL, "SWS/wol-spk77: Select random midi notes (add to selection), slot 5" }, "WOL_SELRANDMIDINOTESADDS5", SelectRandomMidiNotesTool, NULL, 13 },
		{ { DEFACCEL, "SWS/wol-spk77: Select random midi notes (add to selection), slot 6" }, "WOL_SELRANDMIDINOTESADDS6", SelectRandomMidiNotesTool, NULL, 14 },
		{ { DEFACCEL, "SWS/wol-spk77: Select random midi notes (add to selection), slot 7" }, "WOL_SELRANDMIDINOTESADDS7", SelectRandomMidiNotesTool, NULL, 15 },
		{ { DEFACCEL, "SWS/wol-spk77: Select random midi notes (add to selection), slot 8" }, "WOL_SELRANDMIDINOTESADDS8", SelectRandomMidiNotesTool, NULL, 16 },
		{ { DEFACCEL, "SWS/wol-spk77: Select random midi notes among selected notes, slot 1" }, "WOL_SELRANDMIDINOTESAMGSELS1", SelectRandomMidiNotesTool, NULL, 17 },
		{ { DEFACCEL, "SWS/wol-spk77: Select random midi notes among selected notes, slot 2" }, "WOL_SELRANDMIDINOTESAMGSELS2", SelectRandomMidiNotesTool, NULL, 18 },
		{ { DEFACCEL, "SWS/wol-spk77: Select random midi notes among selected notes, slot 3" }, "WOL_SELRANDMIDINOTESAMGSELS3", SelectRandomMidiNotesTool, NULL, 19 },
		{ { DEFACCEL, "SWS/wol-spk77: Select random midi notes among selected notes, slot 4" }, "WOL_SELRANDMIDINOTESAMGSELS4", SelectRandomMidiNotesTool, NULL, 20 },
		{ { DEFACCEL, "SWS/wol-spk77: Select random midi notes among selected notes, slot 5" }, "WOL_SELRANDMIDINOTESAMGSELS5", SelectRandomMidiNotesTool, NULL, 21 },
		{ { DEFACCEL, "SWS/wol-spk77: Select random midi notes among selected notes, slot 6" }, "WOL_SELRANDMIDINOTESAMGSELS6", SelectRandomMidiNotesTool, NULL, 22 },
		{ { DEFACCEL, "SWS/wol-spk77: Select random midi notes among selected notes, slot 7" }, "WOL_SELRANDMIDINOTESAMGSELS7", SelectRandomMidiNotesTool, NULL, 23 },
		{ { DEFACCEL, "SWS/wol-spk77: Select random midi notes among selected notes, slot 8" }, "WOL_SELRANDMIDINOTESAMGSELS8", SelectRandomMidiNotesTool, NULL, 24 },

		{ { DEFACCEL, "SWS/wol-spk77: Compress selected midi notes velocities tool" }, "WOL_COMPSELMIDINOTES", CompressSelectedMidiNotesVelocitiesTool, NULL, 0, IsCompressSelectedMidiNotesVelocitiesOpen },
		{ { DEFACCEL, "SWS/wol-spk77: Compress selected midi notes velocities towards mean, slot 1" }, "WOL_COMPSELMIDINOTESMEANS1", CompressSelectedMidiNotesVelocitiesTool, NULL, 1 },
		{ { DEFACCEL, "SWS/wol-spk77: Compress selected midi notes velocities towards mean, slot 2" }, "WOL_COMPSELMIDINOTESMEANS2", CompressSelectedMidiNotesVelocitiesTool, NULL, 2 },
		{ { DEFACCEL, "SWS/wol-spk77: Compress selected midi notes velocities towards mean, slot 3" }, "WOL_COMPSELMIDINOTESMEANS3", CompressSelectedMidiNotesVelocitiesTool, NULL, 3 },
		{ { DEFACCEL, "SWS/wol-spk77: Compress selected midi notes velocities towards mean, slot 4" }, "WOL_COMPSELMIDINOTESMEANS4", CompressSelectedMidiNotesVelocitiesTool, NULL, 4 },
		{ { DEFACCEL, "SWS/wol-spk77: Compress selected midi notes velocities towards mean, slot 5" }, "WOL_COMPSELMIDINOTESMEANS5", CompressSelectedMidiNotesVelocitiesTool, NULL, 5 },
		{ { DEFACCEL, "SWS/wol-spk77: Compress selected midi notes velocities towards mean, slot 6" }, "WOL_COMPSELMIDINOTESMEANS6", CompressSelectedMidiNotesVelocitiesTool, NULL, 6 },
		{ { DEFACCEL, "SWS/wol-spk77: Compress selected midi notes velocities towards mean, slot 7" }, "WOL_COMPSELMIDINOTESMEANS7", CompressSelectedMidiNotesVelocitiesTool, NULL, 7 },
		{ { DEFACCEL, "SWS/wol-spk77: Compress selected midi notes velocities towards mean, slot 8" }, "WOL_COMPSELMIDINOTESMEANS8", CompressSelectedMidiNotesVelocitiesTool, NULL, 8 },
		{ { DEFACCEL, "SWS/wol-spk77: Compress selected midi notes velocities towards median, slot 1" }, "WOL_COMPSELMIDINOTESMEDIANS1", CompressSelectedMidiNotesVelocitiesTool, NULL, 9 },
		{ { DEFACCEL, "SWS/wol-spk77: Compress selected midi notes velocities towards median, slot 2" }, "WOL_COMPSELMIDINOTESMEDIANS2", CompressSelectedMidiNotesVelocitiesTool, NULL, 10 },
		{ { DEFACCEL, "SWS/wol-spk77: Compress selected midi notes velocities towards median, slot 3" }, "WOL_COMPSELMIDINOTESMEDIANS3", CompressSelectedMidiNotesVelocitiesTool, NULL, 11 },
		{ { DEFACCEL, "SWS/wol-spk77: Compress selected midi notes velocities towards median, slot 4" }, "WOL_COMPSELMIDINOTESMEDIANS4", CompressSelectedMidiNotesVelocitiesTool, NULL, 12 },
		{ { DEFACCEL, "SWS/wol-spk77: Compress selected midi notes velocities towards median, slot 5" }, "WOL_COMPSELMIDINOTESMEDIANS5", CompressSelectedMidiNotesVelocitiesTool, NULL, 13 },
		{ { DEFACCEL, "SWS/wol-spk77: Compress selected midi notes velocities towards median, slot 6" }, "WOL_COMPSELMIDINOTESMEDIANS6", CompressSelectedMidiNotesVelocitiesTool, NULL, 14 },
		{ { DEFACCEL, "SWS/wol-spk77: Compress selected midi notes velocities towards median, slot 7" }, "WOL_COMPSELMIDINOTESMEDIANS7", CompressSelectedMidiNotesVelocitiesTool, NULL, 15 },
		{ { DEFACCEL, "SWS/wol-spk77: Compress selected midi notes velocities towards median, slot 8" }, "WOL_COMPSELMIDINOTESMEDIANS8", CompressSelectedMidiNotesVelocitiesTool, NULL, 16 },
		{ { DEFACCEL, "SWS/wol-spk77: Compress selected midi notes velocities towards velocity, slot 1" }, "WOL_COMPSELMIDINOTESVELS1", CompressSelectedMidiNotesVelocitiesTool, NULL, 17 },
		{ { DEFACCEL, "SWS/wol-spk77: Compress selected midi notes velocities towards velocity, slot 2" }, "WOL_COMPSELMIDINOTESVELS2", CompressSelectedMidiNotesVelocitiesTool, NULL, 18 },
		{ { DEFACCEL, "SWS/wol-spk77: Compress selected midi notes velocities towards velocity, slot 3" }, "WOL_COMPSELMIDINOTESVELS3", CompressSelectedMidiNotesVelocitiesTool, NULL, 19 },
		{ { DEFACCEL, "SWS/wol-spk77: Compress selected midi notes velocities towards velocity, slot 4" }, "WOL_COMPSELMIDINOTESVELS4", CompressSelectedMidiNotesVelocitiesTool, NULL, 20 },
		{ { DEFACCEL, "SWS/wol-spk77: Compress selected midi notes velocities towards velocity, slot 5" }, "WOL_COMPSELMIDINOTESVELS5", CompressSelectedMidiNotesVelocitiesTool, NULL, 21 },
		{ { DEFACCEL, "SWS/wol-spk77: Compress selected midi notes velocities towards velocity, slot 6" }, "WOL_COMPSELMIDINOTESVELS6", CompressSelectedMidiNotesVelocitiesTool, NULL, 22 },
		{ { DEFACCEL, "SWS/wol-spk77: Compress selected midi notes velocities towards velocity, slot 7" }, "WOL_COMPSELMIDINOTESVELS7", CompressSelectedMidiNotesVelocitiesTool, NULL, 23 },
		{ { DEFACCEL, "SWS/wol-spk77: Compress selected midi notes velocities towards velocity, slot 8" }, "WOL_COMPSELMIDINOTESVELS8", CompressSelectedMidiNotesVelocitiesTool, NULL, 24 },

		///////////////////////////////////////////////////////////////////////////////////////////////////
		// Mixer
		///////////////////////////////////////////////////////////////////////////////////////////////////
		{ { DEFACCEL, "SWS/wol-spk77: Scroll mixer view left by 1 track" }, "WOL_SCROLLMIXERL1TR", ScrollMixer, NULL, 0 },
		{ { DEFACCEL, "SWS/wol-spk77: Scroll mixer view right by 1 track" }, "WOL_SCROLLMIXERR1TR", ScrollMixer, NULL, 1 },

//!WANT_LOCALIZE_1ST_STRING_END
		{ {}, LAST_COMMAND, },
};

//---------//

int WOL_Init()
{
	SWSRegisterCommands(g_commandTable);

	HWND wnd = Splash_GetWnd();
	if (!wnd)
		wnd = GetMainHwnd();

	wol_UtilInit();
	wol_MiscInit();
	if (!wol_ZoomInit())
		ShowErrorMessageBox(__LOCALIZE("Error registering zoom project config.\n Envelope heights list saving in RPP project is not available.", "sws_mbox"), MB_OK, wnd);

	return 1;
}

void WOL_Exit()
{
	wol_MiscExit();
}