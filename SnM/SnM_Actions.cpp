/******************************************************************************
/ SnM_Actions.cpp
/
/ Copyright (c) 2009 Tim Payne (SWS), JF Bédague (S&M)
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
#include "SnM_Actions.h"

static COMMAND_T g_commandTable[] = 
{
	// be carefull! called functions may expect "SWS/S&M: " (removed from undo messages, too long)
	{ { DEFACCEL, "SWS/S&M: Cuetrack from track selection, Pre-Fader (Post-FX)" }, "S&M_SENDS1", cueTrack, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Cuetrack from track selection, Post-Fader (Post-Pan)" }, "S&M_SENDS2", cueTrack, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Cuetrack from track selection, Pre-FX" }, "S&M_SENDS3", cueTrack, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Cuetrack from track selection (prompt)" }, "S&M_SENDS4", cueTrackPrompt, NULL, },
#ifdef _WIN32
	{ { DEFACCEL, "SWS/S&M: Close all I/O window(s)" }, "S&M_WNCLS1", closeRoutingWindows, NULL, },
	{ { DEFACCEL, "SWS/S&M: Close all envelope window(s)" }, "S&M_WNCLS2", closeEnvWindows, NULL, },
	{ { DEFACCEL, "SWS/S&M: Toggle show all I/O window(s)" }, "S&M_WNTGL1", toggleRoutingWindows, NULL, },
	{ { DEFACCEL, "SWS/S&M: Toggle show all envelope window(s)" }, "S&M_WNTGL2", toggleEnvWindows, NULL, },
#endif
	{ { DEFACCEL, "SWS/S&M: Toggle FX 1 online/offline for selected track(s)" }, "S&M_FXOFF1", toggleFXOfflineSelectedTracks, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 2 online/offline for selected track(s)" }, "S&M_FXOFF2", toggleFXOfflineSelectedTracks, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 3 online/offline for selected track(s)" }, "S&M_FXOFF3", toggleFXOfflineSelectedTracks, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 4 online/offline for selected track(s)" }, "S&M_FXOFF4", toggleFXOfflineSelectedTracks, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 5 online/offline for selected track(s)" }, "S&M_FXOFF5", toggleFXOfflineSelectedTracks, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 6 online/offline for selected track(s)" }, "S&M_FXOFF6", toggleFXOfflineSelectedTracks, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 7 online/offline for selected track(s)" }, "S&M_FXOFF7", toggleFXOfflineSelectedTracks, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 8 online/offline for selected track(s)" }, "S&M_FXOFF8", toggleFXOfflineSelectedTracks, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Toggle last FX online/offline for selected track(s)" }, "S&M_FXOFFLAST", toggleFXOfflineSelectedTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 1 bypass for selected track(s)" }, "S&M_FXBYP1", toggleFXBypassSelectedTracks, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 2 bypass for selected track(s)" }, "S&M_FXBYP2", toggleFXBypassSelectedTracks, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 3 bypass for selected track(s)" }, "S&M_FXBYP3", toggleFXBypassSelectedTracks, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 4 bypass for selected track(s)" }, "S&M_FXBYP4", toggleFXBypassSelectedTracks, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 5 bypass for selected track(s)" }, "S&M_FXBYP5", toggleFXBypassSelectedTracks, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 6 bypass for selected track(s)" }, "S&M_FXBYP6", toggleFXBypassSelectedTracks, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 7 bypass for selected track(s)" }, "S&M_FXBYP7", toggleFXBypassSelectedTracks, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 8 bypass for selected track(s)" }, "S&M_FXBYP8", toggleFXBypassSelectedTracks, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Toggle last FX bypass for selected track(s)" }, "S&M_FXBYPLAST", toggleFXBypassSelectedTracks, NULL, -1},
	
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs bypass for selected track(s)" }, "S&M_FXBYPALL", toggleAllFXsBypassSelectedTracks, NULL, },
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs online/offline for selected track(s)" }, "S&M_FXOFFALL", toggleAllFXsOfflineSelectedTracks, NULL, },

/* commented for the moment: usefull in very specific cases + too many place in the action list!
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 1) online/offline for selected track(s)" }, "S&M_FXOFFEXCPT1", toggleExceptFXOfflineSelectedTracks, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 2) online/offline for selected track(s)" }, "S&M_FXOFFEXCPT2", toggleExceptFXOfflineSelectedTracks, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 3) online/offline for selected track(s)" }, "S&M_FXOFFEXCPT3", toggleExceptFXOfflineSelectedTracks, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 4) online/offline for selected track(s)" }, "S&M_FXOFFEXCPT4", toggleExceptFXOfflineSelectedTracks, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 5) online/offline for selected track(s)" }, "S&M_FXOFFEXCPT5", toggleExceptFXOfflineSelectedTracks, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 6) online/offline for selected track(s)" }, "S&M_FXOFFEXCPT6", toggleExceptFXOfflineSelectedTracks, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 7) online/offline for selected track(s)" }, "S&M_FXOFFEXCPT7", toggleExceptFXOfflineSelectedTracks, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 8) online/offline for selected track(s)" }, "S&M_FXOFFEXCPT8", toggleExceptFXOfflineSelectedTracks, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 1) bypass for selected track(s)" }, "S&M_FXBYPEXCPT1", toggleExceptFXBypassSelectedTracks, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 2) bypass for selected track(s)" }, "S&M_FXBYPEXCPT2", toggleExceptFXBypassSelectedTracks, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 3) bypass for selected track(s)" }, "S&M_FXBYPEXCPT3", toggleExceptFXBypassSelectedTracks, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 4) bypass for selected track(s)" }, "S&M_FXBYPEXCPT4", toggleExceptFXBypassSelectedTracks, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 5) bypass for selected track(s)" }, "S&M_FXBYPEXCPT5", toggleExceptFXBypassSelectedTracks, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 6) bypass for selected track(s)" }, "S&M_FXBYPEXCPT6", toggleExceptFXBypassSelectedTracks, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 7) bypass for selected track(s)" }, "S&M_FXBYPEXCPT7", toggleExceptFXBypassSelectedTracks, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 8) bypass for selected track(s)" }, "S&M_FXBYPEXCPT8", toggleExceptFXBypassSelectedTracks, NULL, 8},
*/
	{ {}, LAST_COMMAND, }, // Denote end of table
};

int SnMActionsInit()
{
	return SWSRegisterCommands(g_commandTable);
}

