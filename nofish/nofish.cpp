/******************************************************************************
/ nofish.cpp
/
/ Copyright (c) 2016 nofish
/ http://forum.cockos.com/member.php?u=6870
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


#include "stdafx.h"
#include "nofish.h"

// Eraser Tool (continous action)
#include "../Breeder/BR_ContinuousActions.h"
#include "../Breeder/BR_Util.h" // get custom mouse cursor
#include "../Breeder/BR_ReaScript.h" // BR_GetMouseCursorContext(), BR_ItemAtMouseCursor()


//////////////////////////////////////////////////////////////////
//                                                              //
// Bypass FX (except VSTi) for selected tracks                  //
//                                                              //
//////////////////////////////////////////////////////////////////

// SWS: Fixed O(n) actions, no worries nofish
void BypassFXexceptVSTiForSelTracks(COMMAND_T* ct)
{
	Undo_BeginBlock();
	WDL_TypedBuf<MediaTrack*> selTracks;
	SWS_GetSelectedTracks(&selTracks, true); // incl. master
	for (int i = 0; i < selTracks.GetSize(); i++) {
		MediaTrack* track = selTracks.Get()[i];
		int vsti_index = TrackFX_GetInstrument(track); // Get position of first VSTi on current track
		for (int j = 0; j < TrackFX_GetCount(track); j++) { // loop through FX on current track
			if (j != vsti_index) { // if current FX is not a VSTi...
				TrackFX_SetEnabled(track, j, 0); // ...bypass current FX
			}
		}
	}
	Undo_EndBlock(SWS_CMD_SHORTNAME(ct), 0);
}


//////////////////////////////////////////////////////////////////
//                                                              //
// #514 Toggle triplet and dotted grid in MIDI editor           //
//                                                              //
//////////////////////////////////////////////////////////////////

const int ME_SECTION = 32060;

int Main_IsMIDIGridTriplet(COMMAND_T* = NULL)
{
	int toggleState = GetToggleCommandStateEx(ME_SECTION, 41004); // Grid: Set grid type to triplet
	return toggleState;
}


int Main_IsMIDIGridDotted(COMMAND_T* = NULL)
{
	int toggleState = GetToggleCommandStateEx(ME_SECTION, 41005); // Grid: Set grid type to dotted
	return toggleState;
}

void Main_NFToggleDottedMIDI(COMMAND_T* = NULL);

void Main_NFToggleTripletMIDI(COMMAND_T* = NULL)
{

	if (Main_IsMIDIGridDotted())
		Main_NFToggleDottedMIDI();

	if (Main_IsMIDIGridTriplet()) {
		// set to straight
		HWND midiEditor;
		midiEditor = MIDIEditor_GetActive();
		MIDIEditor_OnCommand(midiEditor, 41003); // null ptr. check is handled by REAPER
	}
		
	else {
		// set to triplet
		HWND midiEditor;
		midiEditor = MIDIEditor_GetActive();
		MIDIEditor_OnCommand(midiEditor, 41004);
	}
		
	UpdateMIDIGridToolbar();
}
	
void Main_NFToggleDottedMIDI(COMMAND_T*)
{
	if (Main_IsMIDIGridTriplet())
		Main_NFToggleTripletMIDI();

	if (Main_IsMIDIGridDotted()) {
		// set to straight
		HWND midiEditor;
		midiEditor = MIDIEditor_GetActive();
		MIDIEditor_OnCommand(midiEditor, 41003);
	}
	else {
		// set to dotted
		HWND midiEditor;
		midiEditor = MIDIEditor_GetActive();
		MIDIEditor_OnCommand(midiEditor, 41005);
	}
	
	UpdateMIDIGridToolbar();
}

void UpdateMIDIGridToolbar()
{
	static int cmds[2] =
	{
		NamedCommandLookup("_NF_ME_TOGGLETRIPLET_"),
		NamedCommandLookup("_NF_ME_TOGGLEDOTTED_"),
	};
	for (int i = 0; i < 2; ++i) {
		RefreshToolbar2(ME_SECTION, cmds[i]); // when registered in ME
	}
}

// pass through from ME to Main
void ME_NFToggleTripletMIDI(COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd) {
	Main_NFToggleTripletMIDI(_ct);
}

void ME_NFToggleDottedMIDI(COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd) {
	Main_NFToggleDottedMIDI(_ct);
}
// /#514


//////////////////////////////////////////////////////////////////
//                                                              //
// Eraser tool                                                  //
//                                                              //
//////////////////////////////////////////////////////////////////

char g_curMouseMod[32];
int g_curRelEdgesMode; 

// called on start with init = true and on shortcut release with init = false. Return false to abort init.
static bool EraserToolInit(COMMAND_T* ct, bool init)
{
	bool initSuccessful = true;

	if (init) {
		// get the currently assigned mm, store it and restore after this continious action is performed
		GetMouseModifier("MM_CTX_ITEM", 0, g_curMouseMod, sizeof(g_curMouseMod)); // Media item left drag, Default action
		SetMouseModifier("MM_CTX_ITEM", 0, "28"); // Marquee sel. items and time = 28

		// temp. disable Prefs->"Editing Behaviour -> If no items are selected..."
		// doesn't work well with Eraser tool
		GetConfig("relativeedges", g_curRelEdgesMode);
		SetConfig("relativeedges", SetBit(g_curRelEdgesMode, 8)); 
		
		// select item under mouse
		// BR_GetMouseCursorContext(windowOut, sizeof(windowOut), segmentOut, sizeof(segmentOut), detailsOut, sizeof(detailsOut));
		// MediaItem* item = BR_GetMouseCursorContext_Item();
		MediaItem* item = BR_ItemAtMouseCursor(NULL);
		if (item != NULL) {
			Main_OnCommand(40289, 0); // unsel. all items
			SetMediaItemSelected(item, true);
			UpdateArrange();
		}

		Undo_BeginBlock();
		Main_OnCommand(40635, 0); // remove time sel.
	}

	if (!init) {
		// Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct), UNDO_STATE_ITEMS, -1);

		Main_OnCommand(40289, 0); // unsel. all items
		Main_OnCommand(40635, 0); // remove time sel.

		Undo_EndBlock(SWS_CMD_SHORTNAME(ct), 0);

		// set mm back to original when shortcut is released
		SetMouseModifier("MM_CTX_ITEM", 0, g_curMouseMod);

		// set Prefs -> Editing Behaviour -> If no items are selected... back to original value
		SetConfig("relativeedges", g_curRelEdgesMode);
	}
		
	return initSuccessful;
}

static void DoEraserTool(COMMAND_T* ct)
{
	// BR_GetMouseCursorContext(windowOut, sizeof(windowOut), segmentOut, sizeof(segmentOut), detailsOut, sizeof(detailsOut));
	// MediaItem* item = BR_GetMouseCursorContext_Item();

	MediaItem* item = BR_ItemAtMouseCursor(NULL);
	if (item != NULL) {
		SetMediaItemSelected(item, true);
		UpdateArrange();
	}
	
	Main_OnCommand(40307, 0); // cut sel area of sel items
}

// this gets a custom mouse cursor for the action
static HCURSOR EraserToolCursor(COMMAND_T* ct, int window)
{
	// if MAIN_RULER isn't added the custom mouse cursor doesn't appear immediately when pressing the assigned shortcut
	// not sure why
	if (window == BR_ContinuousAction::MAIN_ARRANGE || window == BR_ContinuousAction::MAIN_RULER)
		return GetSwsMouseCursor(CURSOR_ERASER);
	else
		return NULL;
}

// called when shortcut is released, return undo flag to create undo point (or 0 for no undo point). If NULL, no undo point will get created
static int EraserToolUndo(COMMAND_T* ct)
{
	return 0; // undo point is created in EraserToolInit() already
}


//////////////////////////////////////////////////////////////////
//                                                              //
// Register commands                                            //
//                                                              //
//////////////////////////////////////////////////////////////////

// COMMAND SIGNATURES
/*
void BypassFXexceptVSTiForSelTracks(COMMAND_T* ct);

#514
void Main_NFToggleTripletMIDI(COMMAND_T* = NULL)
void Main_NFToggleDottedMIDI(COMMAND_T*)
void ME_NFToggleTripletMIDI(COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd)
ME_NFToggleDottedMIDI(COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd)
*/

void NF_RegisterContinuousActions()
{
	EraserToolInit();
}

void EraserToolInit()
{
	//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
	static COMMAND_T s_commandTable[] =
	{
		{ { DEFACCEL, "SWS/NF: Eraser tool (perform until shortcut released)" }, "NF_ERASER_TOOL", DoEraserTool, NULL, 0 },
		{ {}, LAST_COMMAND }
	};
	//!WANT_LOCALIZE_1ST_STRING_END

	int i = -1;
	while (s_commandTable[++i].id != LAST_COMMAND)
		ContinuousActionRegister(new BR_ContinuousAction(&s_commandTable[i], EraserToolInit, EraserToolUndo, EraserToolCursor));
}

static COMMAND_T g_commandTable[] =
{
	//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions

	// Bypass FX(except VSTi) for selected tracks
	{ { DEFACCEL, "SWS/NF: Bypass FX (except VSTi) for selected tracks" }, "NF_BYPASS_FX_EXCEPT_VSTI_FOR_SEL_TRACKS", BypassFXexceptVSTiForSelTracks, NULL },

	// #514
	// don't register in Main
	// { { DEFACCEL, "SWS/NF: Toggle triplet grid" },	"NF_MAIN_TOGGLETRIPLET_MIDI",   Main_NFToggleTripletMIDI, NULL, 0, Main_IsMIDIGridTriplet },
	// { { DEFACCEL, "SWS/NF: Toggle dotted grid" },   "NF_MAIN_TOGGLEDOTTED_MIDI",    Main_NFToggleDottedMIDI, NULL, 0, Main_IsMIDIGridDotted },

	{ { DEFACCEL, "SWS/NF: Toggle triplet grid" }, "NF_ME_TOGGLETRIPLET" , NULL, NULL, 0, Main_IsMIDIGridTriplet, ME_SECTION, ME_NFToggleTripletMIDI },
	{ { DEFACCEL, "SWS/NF: Toggle dotted grid" }, "NF_ME_TOGGLEDOTTED" , NULL, NULL, 0, Main_IsMIDIGridDotted, ME_SECTION, ME_NFToggleDottedMIDI  },
	
	//!WANT_LOCALIZE_1ST_STRING_END

	{ {}, LAST_COMMAND, },
};


int nofish_Init()
{
	SWSRegisterCommands(g_commandTable);
	return 1;
}