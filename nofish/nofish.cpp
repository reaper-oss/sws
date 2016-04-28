/******************************************************************************
/ nofish.h
/
/ Copyright (c) 2016 nofish
/ http://forum.cockos.com/member.php?u=6870
/ http://github.com/Jeff0S/sws
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


void BypassFXexceptVSTiForSelTracks(COMMAND_T* ct);

static COMMAND_T g_commandTable[] =
{
	//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Bypass FX (except VSTi) for selected tracks
	// conversion of spk77's EEL script
	// http://forum.cockos.com/showpost.php?p=1475585&postcount=6
	///////////////////////////////////////////////////////////////////////////////////////////////////
	{ { DEFACCEL, "SWS/nofish: Bypass FX (except VSTi) for selected tracks" }, "NOFISH_BYPASS_FX_EXCEPT_VSTI_FOR_SEL_TRACKS", BypassFXexceptVSTiForSelTracks, NULL },

	//!WANT_LOCALIZE_1ST_STRING_END

	{ {}, LAST_COMMAND, },
};

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
	Undo_EndBlock(SWS_CMD_SHORTNAME(ct), 0);
}

int nofish_Init()
{
	SWSRegisterCommands(g_commandTable);
	return 1;
}