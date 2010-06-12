/******************************************************************************
/ SnM_Actions.cpp
/
/ Copyright (c) 2009-2010 Tim Payne (SWS), JF Bédague
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

bool g_fakeToggleStates[MAX_ACTION_COUNT+1]; // +1 for no-op, 1-based..

static COMMAND_T g_commandTable[] = 
{
	// be carefull!!!! 
	// S&M functions expect "SWS/S&M: " in their titles (removed from undo messages, too long)

	// Sends, receives & cue buss ----------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Create cue buss track from track selection, pre-fader (post-FX)" }, "S&M_SENDS1", cueTrack, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Create cue buss track from track selection, post-fader (post-pan)" }, "S&M_SENDS2", cueTrack, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Create cue buss track from track selection, pre-FX" }, "S&M_SENDS3", cueTrack, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Open cue buss window..." }, "S&M_SENDS4", cueTrackPrompt, NULL, },
	{ { DEFACCEL, "SWS/S&M: Create cue buss track from track selection" }, "S&M_CUEBUS", cueTrack, NULL, -1},

	{ { DEFACCEL, "SWS/S&M: Remove receives from selected tracks" }, "S&M_SENDS5", removeReceives, NULL, },
	{ { DEFACCEL, "SWS/S&M: Remove sends from selected tracks" }, "S&M_SENDS6", removeSends, NULL, },
	{ { DEFACCEL, "SWS/S&M: Remove routing from selected tracks" }, "S&M_SENDS7", removeRouting, NULL, },

	{ { DEFACCEL, "SWS/S&M: Copy tracks (with routing)" }, "S&M_COPYSNDRCV1", copyWithIOs, NULL, },
	{ { DEFACCEL, "SWS/S&M: Paste items or tracks with routing" }, "S&M_PASTSNDRCV1", pasteWithIOs, NULL, },
	{ { DEFACCEL, "SWS/S&M: Cut tracks (with routing)" }, "S&M_CUTSNDRCV1", cutWithIOs, NULL, },

	{ { DEFACCEL, "SWS/S&M: Copy selected tracks routings" }, "S&M_COPYSNDRCV2", copyRoutings, NULL, },
	{ { DEFACCEL, "SWS/S&M: Paste routings to selected tracks" }, "S&M_PASTSNDRCV2", pasteRoutings, NULL, },
	{ { DEFACCEL, "SWS/S&M: Cut selected tracks routings" }, "S&M_CUTSNDRCV2", cutRoutings, NULL, },

	{ { DEFACCEL, "SWS/S&M: Copy selected tracks sends" }, "S&M_COPYSNDRCV3", copySends, NULL, },
	{ { DEFACCEL, "SWS/S&M: Paste sends to selected tracks" }, "S&M_PASTSNDRCV3", pasteSends, NULL, },
	{ { DEFACCEL, "SWS/S&M: Cut selected tracks sends" }, "S&M_CUTSNDRCV3", cutSends, NULL, },

	{ { DEFACCEL, "SWS/S&M: Copy selected tracks receives" }, "S&M_COPYSNDRCV4", copyReceives, NULL, },
	{ { DEFACCEL, "SWS/S&M: Paste receives to selected tracks" }, "S&M_PASTSNDRCV4", pasteReceives, NULL, },
	{ { DEFACCEL, "SWS/S&M: Cut selected tracks receives" }, "S&M_CUTSNDRCV4", cutReceives, NULL, },


	// Windows ----------------------------------------------------------------
#ifdef _WIN32
	{ { DEFACCEL, "SWS/S&M: Close all routing windows" }, "S&M_WNCLS1", closeAllRoutingWindows, NULL, },
	{ { DEFACCEL, "SWS/S&M: Close all envelope windows" }, "S&M_WNCLS2", closeAllEnvWindows, NULL, },
	{ { DEFACCEL, "SWS/S&M: Toggle show all routing windows" }, "S&M_WNTGL1", toggleAllRoutingWindows, NULL, -666, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle show all envelope windows" }, "S&M_WNTGL2", toggleAllEnvWindows, NULL, -666, fakeIsToggledAction},
#endif
	{ { DEFACCEL, "SWS/S&M: Close all floating FX windows" }, "S&M_WNCLS3", closeAllFXWindows, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Close all FX chain windows" }, "S&M_WNCLS4", closeAllFXChainsWindows, NULL, },
	{ { DEFACCEL, "SWS/S&M: Close all floating FX windows for selected tracks" }, "S&M_WNCLS5", closeAllFXWindows, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Close all floating FX windows, except focused one" }, "S&M_WNCLS6", closeAllFXWindowsExceptFocused, NULL, },
	{ { DEFACCEL, "SWS/S&M: Show all floating FX windows (!)" }, "S&M_WNTSHW1", showAllFXWindows, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Show all FX chain windows (!)" }, "S&M_WNTSHW2", showAllFXChainsWindows, NULL, },
	{ { DEFACCEL, "SWS/S&M: Show all floating FX windows for selected tracks" }, "S&M_WNTSHW3", showAllFXWindows, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Toggle show all floating FX (!)" }, "S&M_WNTGL3", toggleAllFXWindows, NULL, 0, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle show all FX chain (!)" }, "S&M_WNTGL4", toggleAllFXChainsWindows, NULL, -666, fakeIsToggledAction},	
	{ { DEFACCEL, "SWS/S&M: Toggle show all floating FX for selected tracks" }, "S&M_WNTGL5", toggleAllFXWindows, NULL, 1, fakeIsToggledAction},

	{ { DEFACCEL, "SWS/S&M: Float previous FX (and close others) for selected tracks" }, "S&M_WNONLY1", cycleFloatFXWndSelTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Float next FX (and close others) for selected tracks" }, "S&M_WNONLY2", cycleFloatFXWndSelTracks, NULL, 1},

	{ { DEFACCEL, "SWS/S&M: Focus previous floating FX for selected tracks (cycle)" }, "S&M_WNFOCUS1", cycleFocusFXWndSelTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Focus next floating FX for selected tracks  (cycle)" }, "S&M_WNFOCUS2", cycleFocusFXWndSelTracks, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Focus previous floating FX (cycle)" }, "S&M_WNFOCUS3", cycleFocusFXWndAllTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Focus next floating FX (cycle)" }, "S&M_WNFOCUS4", cycleFocusFXWndAllTracks, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Focus previous floating FX for selected tracks (+ main window on cycle)" }, "S&M_WNFOCUS5", cycleFocusFXMainWndSelTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Focus next floating FX for selected tracks (+ main window on cycle)" }, "S&M_WNFOCUS6", cycleFocusFXMainWndSelTracks, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Focus previous floating FX (+ main window on cycle)" }, "S&M_WNFOCUS7", cycleFocusFXAndMainWndAllTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Focus next floating FX (+ main window on cycle)" }, "S&M_WNFOCUS8", cycleFocusFXAndMainWndAllTracks, NULL, 1},

	{ { DEFACCEL, "SWS/S&M: Focus main window" }, "S&M_WNMAIN", setMainWindowActive, NULL, },

	{ { DEFACCEL, "SWS/S&M: Show FX chain (FX 1) for selected tracks" }, "S&M_SHOWFXCHAIN1", showFXChain, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Show FX chain (FX 2) for selected tracks" }, "S&M_SHOWFXCHAIN2", showFXChain, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Show FX chain (FX 3) for selected tracks" }, "S&M_SHOWFXCHAIN3", showFXChain, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Show FX chain (FX 4) for selected tracks" }, "S&M_SHOWFXCHAIN4", showFXChain, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Show FX chain (FX 5) for selected tracks" }, "S&M_SHOWFXCHAIN5", showFXChain, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Show FX chain (FX 6) for selected tracks" }, "S&M_SHOWFXCHAIN6", showFXChain, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Show FX chain (FX 7) for selected tracks" }, "S&M_SHOWFXCHAIN7", showFXChain, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Show FX chain (FX 8) for selected tracks" }, "S&M_SHOWFXCHAIN8", showFXChain, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Show FX chain (selected FX) for selected tracks" }, "S&M_SHOWFXCHAINSEL", showFXChain, NULL, -1},

	{ { DEFACCEL, "SWS/S&M: Hide FX chain for selected tracks" }, "S&M_HIDEFXCHAIN", hideFXChain, NULL, },
	{ { DEFACCEL, "SWS/S&M: Toggle show FX chain for selected tracks" }, "S&M_TOGLFXCHAIN", toggleFXChain, NULL, -666, isToggleFXChain},

	{ { DEFACCEL, "SWS/S&M: Float FX 1 for selected tracks" }, "S&M_FLOATFX1", floatFX, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Float FX 2 for selected tracks" }, "S&M_FLOATFX2", floatFX, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Float FX 3 for selected tracks" }, "S&M_FLOATFX3", floatFX, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Float FX 4 for selected tracks" }, "S&M_FLOATFX4", floatFX, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Float FX 5 for selected tracks" }, "S&M_FLOATFX5", floatFX, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Float FX 6 for selected tracks" }, "S&M_FLOATFX6", floatFX, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Float FX 7 for selected tracks" }, "S&M_FLOATFX7", floatFX, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Float FX 8 for selected tracks" }, "S&M_FLOATFX8", floatFX, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Float selected FX for selected tracks" }, "S&M_FLOATFXEL", floatFX, NULL, -1},

	{ { DEFACCEL, "SWS/S&M: Unfloat FX 1 for selected tracks" }, "S&M_UNFLOATFX1", unfloatFX, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Unfloat FX 2 for selected tracks" }, "S&M_UNFLOATFX2", unfloatFX, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Unfloat FX 3 for selected tracks" }, "S&M_UNFLOATFX3", unfloatFX, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Unfloat FX 4 for selected tracks" }, "S&M_UNFLOATFX4", unfloatFX, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Unfloat FX 5 for selected tracks" }, "S&M_UNFLOATFX5", unfloatFX, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Unfloat FX 6 for selected tracks" }, "S&M_UNFLOATFX6", unfloatFX, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Unfloat FX 7 for selected tracks" }, "S&M_UNFLOATFX7", unfloatFX, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Unfloat FX 8 for selected tracks" }, "S&M_UNFLOATFX8", unfloatFX, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Unfloat selected FX for selected tracks" }, "S&M_UNFLOATFXEL", unfloatFX, NULL, -1},

	{ { DEFACCEL, "SWS/S&M: Toggle float FX 1 for selected tracks" }, "S&M_TOGLFLOATFX1", toggleFloatFX, NULL, 0, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle float FX 2 for selected tracks" }, "S&M_TOGLFLOATFX2", toggleFloatFX, NULL, 1, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle float FX 3 for selected tracks" }, "S&M_TOGLFLOATFX3", toggleFloatFX, NULL, 2, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle float FX 4 for selected tracks" }, "S&M_TOGLFLOATFX4", toggleFloatFX, NULL, 3, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle float FX 5 for selected tracks" }, "S&M_TOGLFLOATFX5", toggleFloatFX, NULL, 4, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle float FX 6 for selected tracks" }, "S&M_TOGLFLOATFX6", toggleFloatFX, NULL, 5, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle float FX 7 for selected tracks" }, "S&M_TOGLFLOATFX7", toggleFloatFX, NULL, 6, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle float FX 8 for selected tracks" }, "S&M_TOGLFLOATFX8", toggleFloatFX, NULL, 7, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle float selected FX for selected tracks" }, "S&M_TOGLFLOATFXEL", toggleFloatFX, NULL, -1, fakeIsToggledAction},


	// Track FX selection -----------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Select previous FX (cycling) for selected tracks" }, "S&M_SELFXPREV", selectFX, NULL, -2},
	{ { DEFACCEL, "SWS/S&M: Select next FX (cycling) for selected tracks" }, "S&M_SELFXNEXT", selectFX, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Select FX 1 for selected tracks" }, "S&M_SELFX1", selectFX, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Select FX 2 for selected tracks" }, "S&M_SELFX2", selectFX, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Select FX 3 for selected tracks" }, "S&M_SELFX3", selectFX, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Select FX 4 for selected tracks" }, "S&M_SELFX4", selectFX, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Select FX 5 for selected tracks" }, "S&M_SELFX5", selectFX, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Select FX 6 for selected tracks" }, "S&M_SELFX6", selectFX, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Select FX 7 for selected tracks" }, "S&M_SELFX7", selectFX, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Select FX 8 for selected tracks" }, "S&M_SELFX8", selectFX, NULL, 7},


	// Track FX online/offline & bypass/unbypass ------------------------------
	{ { DEFACCEL, "SWS/S&M: Toggle FX 1 offline for selected tracks" }, "S&M_FXOFF1", toggleFXOfflineSelectedTracks, NULL, 1, isFXOfflineSelectedTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 2 offline for selected tracks" }, "S&M_FXOFF2", toggleFXOfflineSelectedTracks, NULL, 2, isFXOfflineSelectedTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 3 offline for selected tracks" }, "S&M_FXOFF3", toggleFXOfflineSelectedTracks, NULL, 3, isFXOfflineSelectedTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 4 offline for selected tracks" }, "S&M_FXOFF4", toggleFXOfflineSelectedTracks, NULL, 4, isFXOfflineSelectedTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 5 offline for selected tracks" }, "S&M_FXOFF5", toggleFXOfflineSelectedTracks, NULL, 5, isFXOfflineSelectedTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 6 offline for selected tracks" }, "S&M_FXOFF6", toggleFXOfflineSelectedTracks, NULL, 6, isFXOfflineSelectedTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 7 offline for selected tracks" }, "S&M_FXOFF7", toggleFXOfflineSelectedTracks, NULL, 7, isFXOfflineSelectedTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 8 offline for selected tracks" }, "S&M_FXOFF8", toggleFXOfflineSelectedTracks, NULL, 8, isFXOfflineSelectedTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle last FX offline for selected tracks" }, "S&M_FXOFFLAST", toggleFXOfflineSelectedTracks, NULL, -1, isFXOfflineSelectedTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle selected FX offline for selected tracks" }, "S&M_FXOFFSEL", toggleFXOfflineSelectedTracks, NULL, 0, isFXOfflineSelectedTracks},

	{ { DEFACCEL, "SWS/S&M: Set FX 1 online for selected tracks" }, "S&M_FXOFF_SETON1", setFXOnlineSelectedTracks, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Set FX 2 online for selected tracks" }, "S&M_FXOFF_SETON2", setFXOnlineSelectedTracks, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Set FX 3 online for selected tracks" }, "S&M_FXOFF_SETON3", setFXOnlineSelectedTracks, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Set FX 4 online for selected tracks" }, "S&M_FXOFF_SETON4", setFXOnlineSelectedTracks, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Set FX 5 online for selected tracks" }, "S&M_FXOFF_SETON5", setFXOnlineSelectedTracks, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Set FX 6 online for selected tracks" }, "S&M_FXOFF_SETON6", setFXOnlineSelectedTracks, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Set FX 7 online for selected tracks" }, "S&M_FXOFF_SETON7", setFXOnlineSelectedTracks, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Set FX 8 online for selected tracks" }, "S&M_FXOFF_SETON8", setFXOnlineSelectedTracks, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Set last FX online for selected tracks" }, "S&M_FXOFF_SETONLAST", setFXOnlineSelectedTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Set selected FX online for selected tracks" }, "S&M_FXOFF_SETONSEL", setFXOnlineSelectedTracks, NULL, 0},

	{ { DEFACCEL, "SWS/S&M: Set FX 1 offline for selected tracks" }, "S&M_FXOFF_SETOFF1", setFXOfflineSelectedTracks, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Set FX 2 offline for selected tracks" }, "S&M_FXOFF_SETOFF2", setFXOfflineSelectedTracks, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Set FX 3 offline for selected tracks" }, "S&M_FXOFF_SETOFF3", setFXOfflineSelectedTracks, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Set FX 4 offline for selected tracks" }, "S&M_FXOFF_SETOFF4", setFXOfflineSelectedTracks, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Set FX 5 offline for selected tracks" }, "S&M_FXOFF_SETOFF5", setFXOfflineSelectedTracks, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Set FX 6 offline for selected tracks" }, "S&M_FXOFF_SETOFF6", setFXOfflineSelectedTracks, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Set FX 7 offline for selected tracks" }, "S&M_FXOFF_SETOFF7", setFXOfflineSelectedTracks, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Set FX 8 offline for selected tracks" }, "S&M_FXOFF_SETOFF8", setFXOfflineSelectedTracks, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Set last FX offline for selected tracks" }, "S&M_FXOFF_SETOFFLAST", setFXOfflineSelectedTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Set selected FX offline for selected tracks" }, "S&M_FXOFF_SETOFFSEL", setFXOfflineSelectedTracks, NULL, 0},

	{ { DEFACCEL, "SWS/S&M: Toggle all FX online/offline for selected tracks" }, "S&M_FXOFFALL", toggleAllFXsOfflineSelectedTracks, NULL, -666, fakeIsToggledAction},

	{ { DEFACCEL, "SWS/S&M: Toggle FX 1 bypass for selected tracks" }, "S&M_FXBYP1", toggleFXBypassSelectedTracks, NULL, 1, isFXBypassedSelectedTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 2 bypass for selected tracks" }, "S&M_FXBYP2", toggleFXBypassSelectedTracks, NULL, 2, isFXBypassedSelectedTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 3 bypass for selected tracks" }, "S&M_FXBYP3", toggleFXBypassSelectedTracks, NULL, 3, isFXBypassedSelectedTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 4 bypass for selected tracks" }, "S&M_FXBYP4", toggleFXBypassSelectedTracks, NULL, 4, isFXBypassedSelectedTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 5 bypass for selected tracks" }, "S&M_FXBYP5", toggleFXBypassSelectedTracks, NULL, 5, isFXBypassedSelectedTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 6 bypass for selected tracks" }, "S&M_FXBYP6", toggleFXBypassSelectedTracks, NULL, 6, isFXBypassedSelectedTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 7 bypass for selected tracks" }, "S&M_FXBYP7", toggleFXBypassSelectedTracks, NULL, 7, isFXBypassedSelectedTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 8 bypass for selected tracks" }, "S&M_FXBYP8", toggleFXBypassSelectedTracks, NULL, 8, isFXBypassedSelectedTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle last FX bypass for selected tracks" }, "S&M_FXBYPLAST", toggleFXBypassSelectedTracks, NULL, -1, isFXBypassedSelectedTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle selected FX bypass for selected tracks" }, "S&M_FXBYPSEL", toggleFXBypassSelectedTracks, NULL, 0, isFXBypassedSelectedTracks},
	
	{ { DEFACCEL, "SWS/S&M: Bypass FX 1 for selected tracks" }, "S&M_FXBYP_SETON1", setFXBypassSelectedTracks, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Bypass FX 2 for selected tracks" }, "S&M_FXBYP_SETON2", setFXBypassSelectedTracks, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Bypass FX 3 for selected tracks" }, "S&M_FXBYP_SETON3", setFXBypassSelectedTracks, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Bypass FX 4 for selected tracks" }, "S&M_FXBYP_SETON4", setFXBypassSelectedTracks, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Bypass FX 5 for selected tracks" }, "S&M_FXBYP_SETON5", setFXBypassSelectedTracks, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Bypass FX 6 for selected tracks" }, "S&M_FXBYP_SETON6", setFXBypassSelectedTracks, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Bypass FX 7 for selected tracks" }, "S&M_FXBYP_SETON7", setFXBypassSelectedTracks, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Bypass FX 8 for selected tracks" }, "S&M_FXBYP_SETON8", setFXBypassSelectedTracks, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Bypass last FX bypass for selected tracks" }, "S&M_FXBYP_SETONLAST", setFXBypassSelectedTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Bypass selected FX bypass for selected tracks" }, "S&M_FXBYP_SETONSEL", setFXBypassSelectedTracks, NULL, 0},
	
	{ { DEFACCEL, "SWS/S&M: Unbypass FX 1 for selected tracks" }, "S&M_FXBYP_SETOFF1", setFXUnbypassSelectedTracks, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Unbypass FX 2 for selected tracks" }, "S&M_FXBYP_SETOFF2", setFXUnbypassSelectedTracks, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Unbypass FX 3 for selected tracks" }, "S&M_FXBYP_SETOFF3", setFXUnbypassSelectedTracks, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Unbypass FX 4 for selected tracks" }, "S&M_FXBYP_SETOFF4", setFXUnbypassSelectedTracks, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Unbypass FX 5 for selected tracks" }, "S&M_FXBYP_SETOFF5", setFXUnbypassSelectedTracks, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Unbypass FX 6 for selected tracks" }, "S&M_FXBYP_SETOFF6", setFXUnbypassSelectedTracks, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Unbypass FX 7 for selected tracks" }, "S&M_FXBYP_SETOFF7", setFXUnbypassSelectedTracks, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Unbypass FX 8 for selected tracks" }, "S&M_FXBYP_SETOFF8", setFXUnbypassSelectedTracks, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Unbypass last FX for selected tracks" }, "S&M_FXBYP_SETOFFLAST", setFXUnbypassSelectedTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Unbypass selected FX for selected tracks" }, "S&M_FXBYP_SETOFFSEL", setFXUnbypassSelectedTracks, NULL, 0},
	
	{ { DEFACCEL, "SWS/S&M: Toggle all FX bypass for selected tracks" }, "S&M_FXBYPALL", toggleAllFXsBypassSelectedTracks, NULL, -666, fakeIsToggledAction},

	{ { DEFACCEL, "SWS/S&M: Bypass all FX for selected tracks" }, "S&M_FXBYPALL2", setAllFXsBypassSelectedTracks, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Unbypass all FX for selected tracks" }, "S&M_FXBYPALL3", setAllFXsBypassSelectedTracks, NULL, 0},
	// ..related online/offline actions natively implemented

	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except selected) offline for selected tracks" }, "S&M_FXOFFEXCPTSEL", toggleExceptFXOfflineSelectedTracks, NULL, 0, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except selected) bypass for selected tracks" }, "S&M_FXBYPEXCPTSEL", toggleExceptFXBypassSelectedTracks, NULL, 0, fakeIsToggledAction},
#ifdef _SNM_MISC
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 1) online/offline for selected tracks" }, "S&M_FXOFFEXCPT1", toggleExceptFXOfflineSelectedTracks, NULL, 1, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 2) online/offline for selected tracks" }, "S&M_FXOFFEXCPT2", toggleExceptFXOfflineSelectedTracks, NULL, 2, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 3) online/offline for selected tracks" }, "S&M_FXOFFEXCPT3", toggleExceptFXOfflineSelectedTracks, NULL, 3, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 4) online/offline for selected tracks" }, "S&M_FXOFFEXCPT4", toggleExceptFXOfflineSelectedTracks, NULL, 4, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 5) online/offline for selected tracks" }, "S&M_FXOFFEXCPT5", toggleExceptFXOfflineSelectedTracks, NULL, 5, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 6) online/offline for selected tracks" }, "S&M_FXOFFEXCPT6", toggleExceptFXOfflineSelectedTracks, NULL, 6, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 7) online/offline for selected tracks" }, "S&M_FXOFFEXCPT7", toggleExceptFXOfflineSelectedTracks, NULL, 7, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 8) online/offline for selected tracks" }, "S&M_FXOFFEXCPT8", toggleExceptFXOfflineSelectedTracks, NULL, 8, fakeIsToggledAction},

	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 1) bypass for selected tracks" }, "S&M_FXBYPEXCPT1", toggleExceptFXBypassSelectedTracks, NULL, 1, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 2) bypass for selected tracks" }, "S&M_FXBYPEXCPT2", toggleExceptFXBypassSelectedTracks, NULL, 2, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 3) bypass for selected tracks" }, "S&M_FXBYPEXCPT3", toggleExceptFXBypassSelectedTracks, NULL, 3, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 4) bypass for selected tracks" }, "S&M_FXBYPEXCPT4", toggleExceptFXBypassSelectedTracks, NULL, 4, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 5) bypass for selected tracks" }, "S&M_FXBYPEXCPT5", toggleExceptFXBypassSelectedTracks, NULL, 5, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 6) bypass for selected tracks" }, "S&M_FXBYPEXCPT6", toggleExceptFXBypassSelectedTracks, NULL, 6, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 7) bypass for selected tracks" }, "S&M_FXBYPEXCPT7", toggleExceptFXBypassSelectedTracks, NULL, 7, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 8) bypass for selected tracks" }, "S&M_FXBYPEXCPT8", toggleExceptFXBypassSelectedTracks, NULL, 8, fakeIsToggledAction},
#endif


	// FX Chains (items & tracks) ---------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Open FX chains window..." }, "S&M_SHOWFXCHAINSLOTS", OpenFXChainList, NULL, },
	{ { DEFACCEL, "SWS/S&M: Clear FX chain slot..." }, "S&M_CLRFXCHAINSLOT", clearFXChainSlotPrompt, NULL, },
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected items, slot 1" }, "S&M_TAKEFXCHAIN1", loadPasteTakeFXChain, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected items, slot 2" }, "S&M_TAKEFXCHAIN2", loadPasteTakeFXChain, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected items, slot 3" }, "S&M_TAKEFXCHAIN3", loadPasteTakeFXChain, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected items, slot 4" }, "S&M_TAKEFXCHAIN4", loadPasteTakeFXChain, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected items, slot 5" }, "S&M_TAKEFXCHAIN5", loadPasteTakeFXChain, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected items, slot 6" }, "S&M_TAKEFXCHAIN6", loadPasteTakeFXChain, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected items, slot 7" }, "S&M_TAKEFXCHAIN7", loadPasteTakeFXChain, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected items, slot 8" }, "S&M_TAKEFXCHAIN8", loadPasteTakeFXChain, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected items, prompt for slot" }, "S&M_TAKEFXCHAINp1", loadPasteTakeFXChain, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected items, all takes, prompt for slot" }, "S&M_TAKEFXCHAINp2", loadPasteAllTakesFXChain, NULL, -1},

	{ { DEFACCEL, "SWS/S&M: Copy FX chain from selected item" }, "S&M_COPYFXCHAIN1", copyTakeFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Cut FX chain from selected item" }, "S&M_COPYFXCHAIN2", cutTakeFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Paste FX chain to selected items" }, "S&M_COPYFXCHAIN3", pasteTakeFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Paste FX chain to selected items, all takes" }, "S&M_COPYFXCHAIN4", pasteAllTakesFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Copy FX chain from selected track" }, "S&M_COPYFXCHAIN5", copyTrackFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Cut FX chain from selected track" }, "S&M_COPYFXCHAIN6", cutTrackFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Paste FX chain to selected tracks" }, "S&M_COPYFXCHAIN7", pasteTrackFXChain, NULL, }, 

	{ { DEFACCEL, "SWS/S&M: Clear FX chain for selected items" },  "S&M_CLRFXCHAIN1", clearActiveTakeFXChain, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Clear FX chain for selected items, all takes" },  "S&M_CLRFXCHAIN2", clearAllTakesFXChain, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Clear FX chain for selected tracks" }, "S&M_CLRFXCHAIN3", clearTrackFXChain, NULL, 0},

	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected tracks, slot 1" }, "S&M_TRACKFXCHAIN1", loadPasteTrackFXChain, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected tracks, slot 2" }, "S&M_TRACKFXCHAIN2", loadPasteTrackFXChain, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected tracks, slot 3" }, "S&M_TRACKFXCHAIN3", loadPasteTrackFXChain, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected tracks, slot 4" }, "S&M_TRACKFXCHAIN4", loadPasteTrackFXChain, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected tracks, slot 5" }, "S&M_TRACKFXCHAIN5", loadPasteTrackFXChain, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected tracks, slot 6" }, "S&M_TRACKFXCHAIN6", loadPasteTrackFXChain, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected tracks, slot 7" }, "S&M_TRACKFXCHAIN7", loadPasteTrackFXChain, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected tracks, slot 8" }, "S&M_TRACKFXCHAIN8", loadPasteTrackFXChain, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected tracks, prompt for slot" }, "S&M_TRACKFXCHAINp1", loadPasteTrackFXChain, NULL, -1},
	
	
	// Takes ------------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Takes - Clear active takes/items" }, "S&M_CLRTAKE1", clearTake, NULL, },
	{ { DEFACCEL, "SWS/S&M: Takes - Build lanes for selected tracks" }, "S&M_LANETAKE1", buildLanes, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Takes - Select lane from selected item" }, "S&M_LANETAKE2", selectTakeLane, NULL, },
	{ { DEFACCEL, "SWS/S&M: Takes - Build lanes for selected itemss" }, "S&M_LANETAKE3", buildLanes, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Takes - Remove empty source take/items among selected items" }, "S&M_DELEMPTYTAKE", removeEmptyTakes, NULL, },
	{ { DEFACCEL, "SWS/S&M: Takes - Remove empty MIDI take/items among selected items" }, "S&M_DELEMPTYTAKE2", removeEmptyMidiTakes, NULL, },
	{ { DEFACCEL, "SWS/S&M: Takes - Remove all empty takes/items among selected items" }, "S&M_DELEMPTYTAKE3", removeAllEmptyTakes, NULL, },
	{ { DEFACCEL, "SWS/S&M: Takes - Move all up (cycling) in selected items" }, "S&M_MOVETAKE1", moveTakes, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Takes - Move all down (cycling) in selected items" }, "S&M_MOVETAKE2", moveTakes, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Takes - Move active up (cycling) in selected items" }, "S&M_MOVETAKE3", moveActiveTake, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Takes - Move active down (cycling) in selected items" }, "S&M_MOVETAKE4", moveActiveTake, NULL, 1},

	{ { DEFACCEL, "SWS/S&M: Split selected items at cursor (MIDI) or at prior zero crossing (audio)" }, "S&M_SPLIT1", splitMidiAudio, NULL, },
	{ { DEFACCEL, "SWS/S&M: Split selected items at time selection (if exists), else at cursor (MIDI) or at prior zero crossing (audio)" }, "S&M_SPLIT2", smartSplitMidiAudio, NULL, },
	{ { DEFACCEL, "SWS/S&M: Split selected items at play cursor" }, "S&M_SPLIT3", splitSelectedItems, NULL, 40196},
	{ { DEFACCEL, "SWS/S&M: Split selected items at time selection" }, "S&M_SPLIT4", splitSelectedItems, NULL, 40061},
	{ { DEFACCEL, "SWS/S&M: Split selected items at edit cursor (no change selection)" }, "S&M_SPLIT5", splitSelectedItems, NULL, 40757},
	{ { DEFACCEL, "SWS/S&M: Split selected items at time selection (select left)" }, "S&M_SPLIT6", splitSelectedItems, NULL, 40758},
	{ { DEFACCEL, "SWS/S&M: Split selected items at time selection (select right)" }, "S&M_SPLIT7", splitSelectedItems, NULL, 40759},
	{ { DEFACCEL, "SWS/S&M: Split selected items at edit or play cursor" }, "S&M_SPLIT8", splitSelectedItems, NULL, 40012},
	{ { DEFACCEL, "SWS/S&M: Split selected items at edit or play cursor (ignoring grouping)" }, "S&M_SPLIT9", splitSelectedItems, NULL, 40186},
	{ { DEFACCEL, "SWS/gofer: Split selected items at mouse cursor (obey snapping)" }, "S&M_SPLIT10", goferSplitSelectedItems, NULL, },

	{ { DEFACCEL, "SWS/S&M: Show take volume envelope" }, "S&M_TAKEENV1", showHideTakeVolEnvelope, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Show take pan envelope" }, "S&M_TAKEENV2", showHideTakePanEnvelope, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Show take mute envelope" }, "S&M_TAKEENV3", showHideTakeMuteEnvelope, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Hide take volume envelope" }, "S&M_TAKEENV4", showHideTakeVolEnvelope, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Hide take pan envelope" }, "S&M_TAKEENV5", showHideTakePanEnvelope, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Hide take mute envelope" }, "S&M_TAKEENV6", showHideTakeMuteEnvelope, NULL, 0},
/*exist natively
	{ { DEFACCEL, "SWS/S&M: Toggle show take volume envelope" }, "S&M_TAKEENV7", showHideTakeVolEnvelope, NULL, -1, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle show take pan envelope" }, "S&M_TAKEENV8", showHideTakePanEnvelope, NULL, -1, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle show take mute envelope" }, "S&M_TAKEENV9", showHideTakeMuteEnvelope, NULL, -1, fakeIsToggledAction},
*/

#ifdef _SNM_MISC
	// Experimental, misc., deprecated, etc.. ---------------------------------
	{ { DEFACCEL, "SWS/S&M: Select items by name" }, "S&M_ITM1", selectItemsByNamePrompt, NULL, },
	{ { DEFACCEL, "SWS/S&M: test -> Padre show take volume envelope" }, "S&M_TMP1", ShowTakeEnvPadreTest, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: test -> Padre show take pan envelope" }, "S&M_TMP2", ShowTakeEnvPadreTest, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: test -> Padre show take mute envelope" }, "S&M_TMP3", ShowTakeEnvPadreTest, NULL, 2},
#endif	

	{ {}, LAST_COMMAND, }, // Denote end of table
};

int SnMActionsInit()
{
	FXChainListInit();

	// for possible future persistance of fake toggle states..
	for (int i=0; i <= MAX_ACTION_COUNT; i++)
		g_fakeToggleStates[i] = false;
	return SWSRegisterCommands(g_commandTable);
}

void SnMExit()
{
	FXChainListExit();
	flushAllRoutingClipboards();
	flushHiddenFXWindows();
}


// *** Action toggle states ***
bool fakeIsToggledAction(COMMAND_T* _ct) {
	return ((_ct && _ct->accel.accel.cmd) ? g_fakeToggleStates[_ct->accel.accel.cmd] : false);
}

void fakeToggleAction(COMMAND_T* _ct) {
	if (_ct && _ct->accel.accel.cmd)
		g_fakeToggleStates[_ct->accel.accel.cmd] = !g_fakeToggleStates[_ct->accel.accel.cmd];
}


///////////////////////////////////////////////////////////////////////////////
// GUIs
///////////////////////////////////////////////////////////////////////////////

// one for lazy guys
void SNM_ShowConsoleMsg(const char* _title, const char* _msg) 
{
		ShowConsoleMsg(""); //clear
		ShowConsoleMsg(_msg);
#ifdef _WIN32
		// a little hack..
		HWND w = FindWindow(NULL, "ReaScript console output");
		if (w != NULL)
			SetWindowTextA(w, _title);
#endif
}

