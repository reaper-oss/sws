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


//////////////////////////////////////////////////////////////////
// Bypass FX (except VSTi) for selected tracks					//
// conversion of spk77's EEL script								//
// http://forum.cockos.com/showpost.php?p=1475585&postcount=6	//
//////////////////////////////////////////////////////////////////

/*
remark for SWS: I've just seen now the comment to not use these O(n) actions in loops, sorry
won't do again
*/
void BypassFXexceptVSTiForSelTracks(COMMAND_T* ct)
{
	Undo_BeginBlock();
	for (int i = 0; i < CountSelectedTracks(0); i++) { // loop through sel. tracks
		MediaTrack* track = GetSelectedTrack(0, i); // get track PTR
			if (track) { // check for NULLPTR
			int vsti_index = TrackFX_GetInstrument(track); // Get position of first VSTi on current track
			for (int j = 0; j < TrackFX_GetCount(track); j++) { // loop through FX on current track
				if (j != vsti_index) { // if current FX is not a VSTi...
					TrackFX_SetEnabled(track, j, 0); // ...bypass current FX
				}
			}
		}
	}
	// a1cab2f6: also check master track
	MediaTrack* mtrack = GetMasterTrack(0);
	if (mtrack) 
	{
		if (GetMediaTrackInfo_Value(mtrack, "I_SELECTED") != 0) {
			int vsti_index = TrackFX_GetInstrument(mtrack); // Get position of first VSTi on current track
			for (int j = 0; j < TrackFX_GetCount(mtrack); j++) { // loop through FX on current track
				if (j != vsti_index) { // if current FX is not a VSTi...
					TrackFX_SetEnabled(mtrack, j, 0); // ...bypass current FX
				}
			}
		}
	}
	

	Undo_EndBlock(SWS_CMD_SHORTNAME(ct), 0);
}


//////////////////////////////////////////////////////////////////
// 																//
// 	#514 Toggle triplet and dotted grid in MIDI editor			//
// 																//
//////////////////////////////////////////////////////////////////

const int ME_SECTION = 32060;

int Main_IsMIDIGridTriplet(COMMAND_T* = NULL)
{
	int toggleState = GetToggleCommandStateEx(ME_SECTION, 41004); // action 41004 = Grid: Set grid type to triplet;
	return toggleState;
}


int Main_IsMIDIGridDotted(COMMAND_T* = NULL)
{
	int toggleState = GetToggleCommandStateEx(ME_SECTION, 41005); // action 41005 = Grid: Set grid type to dotted;
	return toggleState;
}


// fwd. decl.
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
		// RefreshToolbar(cmds[i]); 
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
// /514


//////////////////////////////////////////////////////////////////
// 																//
// Register commands											//
// 																//
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