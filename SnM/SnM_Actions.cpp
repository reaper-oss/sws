/******************************************************************************
/ SnM_Actions.cpp
/
/ Copyright (c) 2009-2010 Tim Payne (SWS), JF BÈdague
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
#include "SNM_NotesHelpView.h"
#include "SNM_MidiLiveView.h"
#include "../Utility/SectionLock.h"


///////////////////////////////////////////////////////////////////////////////
// S&M actions (main section)
///////////////////////////////////////////////////////////////////////////////

static COMMAND_T g_SNM_cmdTable[] = 
{
	// Be carefull: S&M actions expect "SWS/S&M: " in their titles (removed from undo messages, too long)

	// Sends, receives & cue buss ----------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Create cue buss track from track selection, pre-fader (post-FX)" }, "S&M_SENDS1", cueTrack, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Create cue buss track from track selection, post-fader (post-pan)" }, "S&M_SENDS2", cueTrack, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Create cue buss track from track selection, pre-FX" }, "S&M_SENDS3", cueTrack, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Open Cue Buss window..." }, "S&M_SENDS4", cueTrackPrompt, NULL, },
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
	{ { DEFACCEL, "SWS/S&M: Toggle show all FX chain windows (!)" }, "S&M_WNTGL4", toggleAllFXChainsWindows, NULL, -666, fakeIsToggledAction},	
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
#ifdef _WIN32
	{ { DEFACCEL, "SWS/S&M: Cycle focused window" }, "S&M_WNFOCUS9", cycleFocusWnd, NULL, -1},
#endif
	{ { DEFACCEL, "SWS/S&M: Focus main window" }, "S&M_WNMAIN", setMainWindowActive, NULL, },

	{ { DEFACCEL, "SWS/S&M: Show FX chain for selected tracks (FX 1)" }, "S&M_SHOWFXCHAIN1", showFXChain, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Show FX chain for selected tracks (FX 2)" }, "S&M_SHOWFXCHAIN2", showFXChain, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Show FX chain for selected tracks (FX 3)" }, "S&M_SHOWFXCHAIN3", showFXChain, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Show FX chain for selected tracks (FX 4)" }, "S&M_SHOWFXCHAIN4", showFXChain, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Show FX chain for selected tracks (FX 5)" }, "S&M_SHOWFXCHAIN5", showFXChain, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Show FX chain for selected tracks (FX 6)" }, "S&M_SHOWFXCHAIN6", showFXChain, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Show FX chain for selected tracks (FX 7)" }, "S&M_SHOWFXCHAIN7", showFXChain, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Show FX chain for selected tracks (FX 8)" }, "S&M_SHOWFXCHAIN8", showFXChain, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Show FX chain for selected tracks (selected FX)" }, "S&M_SHOWFXCHAINSEL", showFXChain, NULL, -1},

	{ { DEFACCEL, "SWS/S&M: Hide FX chain windows for selected tracks" }, "S&M_HIDEFXCHAIN", hideFXChain, NULL, },
	{ { DEFACCEL, "SWS/S&M: Toggle show FX chain windows for selected tracks" }, "S&M_TOGLFXCHAIN", toggleFXChain, NULL, -666, isToggleFXChain},

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
	{ { DEFACCEL, "SWS/S&M: Bypass last FX for selected tracks" }, "S&M_FXBYP_SETONLAST", setFXBypassSelectedTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Bypass selected FX for selected tracks" }, "S&M_FXBYP_SETONSEL", setFXBypassSelectedTracks, NULL, 0},
	
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
	{ { DEFACCEL, "SWS/S&M: Open FX Chains window..." }, "S&M_SHOWFXCHAINSLOTS", OpenFXChainView, NULL, NULL, IsFXChainViewEnabled},
	{ { DEFACCEL, "SWS/S&M: Clear FX chain slot..." }, "S&M_CLRFXCHAINSLOT", clearFXChainSlotPrompt, NULL, },

	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected items, slot 1" }, "S&M_TAKEFXCHAIN1", loadSetTakeFXChain, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected items, slot 2" }, "S&M_TAKEFXCHAIN2", loadSetTakeFXChain, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected items, slot 3" }, "S&M_TAKEFXCHAIN3", loadSetTakeFXChain, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected items, slot 4" }, "S&M_TAKEFXCHAIN4", loadSetTakeFXChain, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected items, slot 5" }, "S&M_TAKEFXCHAIN5", loadSetTakeFXChain, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected items, slot 6" }, "S&M_TAKEFXCHAIN6", loadSetTakeFXChain, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected items, slot 7" }, "S&M_TAKEFXCHAIN7", loadSetTakeFXChain, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected items, slot 8" }, "S&M_TAKEFXCHAIN8", loadSetTakeFXChain, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected items, prompt for slot" }, "S&M_TAKEFXCHAINp1", loadSetTakeFXChain, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected items, all takes, prompt for slot" }, "S&M_TAKEFXCHAINp2", loadSetAllTakesFXChain, NULL, -1},

	{ { DEFACCEL, "SWS/S&M: Copy FX chain from selected item" }, "S&M_COPYFXCHAIN1", copyTakeFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Cut FX chain from selected items" }, "S&M_COPYFXCHAIN2", cutTakeFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Paste (replace) FX chain to selected items" }, "S&M_COPYFXCHAIN3", setTakeFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Paste (replace) FX chain to selected items, all takes" }, "S&M_COPYFXCHAIN4", setAllTakesFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Copy FX chain from selected track" }, "S&M_COPYFXCHAIN5", copyTrackFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Cut FX chain from selected tracks" }, "S&M_COPYFXCHAIN6", cutTrackFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Paste (replace) FX chain to selected tracks" }, "S&M_COPYFXCHAIN7", setTrackFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Paste FX chain to selected items" }, "S&M_COPYFXCHAIN8", pasteTakeFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Paste FX chain to selected items, all takes" }, "S&M_COPYFXCHAIN9", pasteAllTakesFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Paste FX chain to selected tracks" }, "S&M_COPYFXCHAIN10", pasteTrackFXChain, NULL, }, 

	{ { DEFACCEL, "SWS/S&M: Clear FX chain for selected items" },  "S&M_CLRFXCHAIN1", clearActiveTakeFXChain, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Clear FX chain for selected items, all takes" },  "S&M_CLRFXCHAIN2", clearAllTakesFXChain, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Clear FX chain for selected tracks" }, "S&M_CLRFXCHAIN3", clearTrackFXChain, NULL, -1},

	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected tracks, slot 1" }, "S&M_TRACKFXCHAIN1", loadSetTrackFXChain, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected tracks, slot 2" }, "S&M_TRACKFXCHAIN2", loadSetTrackFXChain, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected tracks, slot 3" }, "S&M_TRACKFXCHAIN3", loadSetTrackFXChain, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected tracks, slot 4" }, "S&M_TRACKFXCHAIN4", loadSetTrackFXChain, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected tracks, slot 5" }, "S&M_TRACKFXCHAIN5", loadSetTrackFXChain, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected tracks, slot 6" }, "S&M_TRACKFXCHAIN6", loadSetTrackFXChain, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected tracks, slot 7" }, "S&M_TRACKFXCHAIN7", loadSetTrackFXChain, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected tracks, slot 8" }, "S&M_TRACKFXCHAIN8", loadSetTrackFXChain, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Load/apply FX chain to selected tracks, prompt for slot" }, "S&M_TRACKFXCHAINp1", loadSetTrackFXChain, NULL, -1},
	
	
	// Takes ------------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Takes - Clear active takes/items" }, "S&M_CLRTAKE1", clearTake, NULL, },
	{ { DEFACCEL, "SWS/S&M: Takes - Build lanes for selected tracks" }, "S&M_LANETAKE1", buildLanes, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Takes - Activate lane from selected item" }, "S&M_LANETAKE2", activateLaneFromSelItem, NULL, },
	{ { DEFACCEL, "SWS/S&M: Takes - Build lanes for selected items" }, "S&M_LANETAKE3", buildLanes, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Takes - Activate lane under mouse cursor" }, "S&M_LANETAKE4", activateLaneUnderMouse, NULL, },
	{ { DEFACCEL, "SWS/S&M: Takes - Remove empty takes/items among selected items" }, "S&M_DELEMPTYTAKE", removeAllEmptyTakes/*removeEmptyTakes*/, NULL, },
	{ { DEFACCEL, "SWS/S&M: Takes - Remove empty MIDI takes/items among selected items" }, "S&M_DELEMPTYTAKE2", removeEmptyMidiTakes, NULL, },
//	{ { DEFACCEL, "SWS/S&M: Takes - Remove all empty takes/items among selected items" }, "S&M_DELEMPTYTAKE3", removeAllEmptyTakes, NULL, },
	{ { DEFACCEL, "SWS/S&M: Takes - Move all up (cycling) in selected items" }, "S&M_MOVETAKE1", moveTakes, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Takes - Move all down (cycling) in selected items" }, "S&M_MOVETAKE2", moveTakes, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Takes - Move active up (cycling) in selected items" }, "S&M_MOVETAKE3", moveActiveTake, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Takes - Move active down (cycling) in selected items" }, "S&M_MOVETAKE4", moveActiveTake, NULL, 1},

	{ { DEFACCEL, "SWS/S&M: Delete selected items' takes and source files (prompt, no undo)" }, "S&M_DELTAKEANDFILE1", deleteTakeAndMedia, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Delete selected items' takes and source files (no undo)" }, "S&M_DELTAKEANDFILE2", deleteTakeAndMedia, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Delete active take and source file in selected items (prompt, no undo)" }, "S&M_DELTAKEANDFILE3", deleteTakeAndMedia, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Delete active take and source file in selected items (no undo)" }, "S&M_DELTAKEANDFILE4", deleteTakeAndMedia, NULL, 4},

	// Notes/help -------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Open Notes/help window..." }, "S&M_SHOWNOTESHELP", OpenNotesHelpView, NULL, NULL, IsNotesHelpViewEnabled},
	{ { DEFACCEL, "SWS/S&M: Notes/help - Switch to project notes (disables auto updates)" }, "S&M_DISABLENOTESHELP", SwitchNotesHelpType, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Notes/help - Switch to item notes" }, "S&M_ITEMNOTES", SwitchNotesHelpType, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Notes/help - Switch to track notes" }, "S&M_TRACKNOTES", SwitchNotesHelpType, NULL, 2},
#ifdef _WIN32
	{ { DEFACCEL, "SWS/S&M: Notes/help - Switch to action help" }, "S&M_ACTIONHELP", SwitchNotesHelpType, NULL, 3},
#endif
	{ { DEFACCEL, "SWS/S&M: Notes/help - Toggle lock" }, "S&M_ACTIONHELPTGLOCK", ToggleNotesHelpLock, NULL, NULL, IsNotesHelpLocked},
	{ { DEFACCEL, "SWS/S&M: Notes/help - Set action help file..." }, "S&M_ACTIONHELPPATH", SetActionHelpFilename, NULL, },


	// Split ------------------------------------------------------------------
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

	// ME ---------------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Active ME - Hide all CC lanes" }, "S&M_MEHIDECCLANES", MEHideCCLanes, NULL, },
	{ { DEFACCEL, "SWS/S&M: Active ME - Create CC lane" }, "S&M_MECREATECCLANE", MECreateCCLane, NULL, },
	{ { DEFACCEL, "SWS/S&M: Active ME - Set displayed CC lanes, slot 1" }, "S&M_MESETCCLANES1", MESetCCLanes, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Active ME - Set displayed CC lanes, slot 2" }, "S&M_MESETCCLANES2", MESetCCLanes, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Active ME - Set displayed CC lanes, slot 3" }, "S&M_MESETCCLANES3", MESetCCLanes, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Active ME - Set displayed CC lanes, slot 4" }, "S&M_MESETCCLANES4", MESetCCLanes, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Active ME - Save displayed CC lanes, slot 1" }, "S&M_MESAVECCLANES1", MESaveCCLanes, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Active ME - Save displayed CC lanes, slot 2" }, "S&M_MESAVECCLANES2", MESaveCCLanes, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Active ME - Save displayed CC lanes, slot 3" }, "S&M_MESAVECCLANES3", MESaveCCLanes, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Active ME - Save displayed CC lanes, slot 4" }, "S&M_MESAVECCLANES4", MESaveCCLanes, NULL, 3},
	
	// Tracks -----------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Save selected tracks folder states" }, "S&M_SAVEFOLDERSTATE1", saveTracksFolderStates, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Restore selected tracks folder states" }, "S&M_RESTOREFOLDERSTATE1", restoreTracksFolderStates, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks folder states to on" }, "S&M_FOLDERON", setTracksFolderState, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks folder states to off" }, "S&M_FOLDEROFF", setTracksFolderState, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Save selected tracks folder compact states" }, "S&M_SAVEFOLDERSTATE2", saveTracksFolderStates, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Restore selected tracks folder compact states" }, "S&M_RESTOREFOLDERSTATE2", restoreTracksFolderStates, NULL, 1},

	{ { DEFACCEL, "SWS/S&M: Toggle arming of all active envelopes for selected tracks" }, "S&M_TGLARMALLENVS", toggleArmTrackEnv, NULL, 0, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Arm all active envelopes for selected tracks" }, "S&M_ARMALLENVS", toggleArmTrackEnv, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Disarm all active envelopes for selected tracks" }, "S&M_DISARMALLENVS", toggleArmTrackEnv, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Toggle arming of volume envelope for selected tracks" }, "S&M_TGLARMVOLENV", toggleArmTrackEnv, NULL, 3, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle arming of pan envelope for selected tracks" }, "S&M_TGLARMPANENV", toggleArmTrackEnv, NULL, 4, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle arming of mute envelope for selected tracks" }, "S&M_TGLARMMUTEENV", toggleArmTrackEnv, NULL, 5, fakeIsToggledAction},

	{ { DEFACCEL, "SWS/S&M: Toggle arming of all receive volume envelopes for selected tracks" }, "S&M_TGLARMAUXVOLENV", toggleArmTrackEnv, NULL, 6, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle arming of all receive pan envelopes for selected tracks" }, "S&M_TGLARMAUXPANENV", toggleArmTrackEnv, NULL, 7, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle arming of all receive mute envelopes for selected tracks" }, "S&M_TGLARMAUXMUTEENV", toggleArmTrackEnv, NULL, 8, fakeIsToggledAction},

	{ { DEFACCEL, "SWS/S&M: Toggle arming of all plugin envelopes for selected tracks" }, "S&M_TGLARMPLUGENV", toggleArmTrackEnv, NULL, 9, fakeIsToggledAction},

	// Other views ------------------------------------------------------------
	{ { {FCONTROL | FVIRTKEY, 'F', 0 }, "SWS/S&M: Find..." }, "S&M_SHOWFIND", OpenFindView, NULL, NULL, IsFindViewEnabled},
	{ { DEFACCEL, "SWS/S&M: Open Live Configs window..." }, "S&M_SHOWMIDILIVE", OpenMidiLiveView, NULL, NULL, IsMidiLiveViewEnabled},

	// Other ------------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Let REAPER breathe" }, "S&M_LETBREATHE", letREAPERBreathe, NULL, },
#ifdef _WIN32
	{ { DEFACCEL, "SWS/S&M: [Beta] Simulate left mouse click at cursor position" }, "S&M_MOUSE_L_CLICK", simulateMouseClick, NULL, 0},
/*
	{ { DEFACCEL, "SWS/S&M: Simulate left mouse-down at cursor position" }, "S&M_MOUSE_LD_CLICK", simulateMouseClick, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Simulate left mouse-up at cursor position" }, "S&M_MOUSE_LU_CLICK", simulateMouseClick, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Simulate right mouse click at cursor position" }, "S&M_MOUSE_R_CLICK", simulateMouseClick, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Simulate right mouse-down at cursor position" }, "S&M_MOUSE_RD_CLICK", simulateMouseClick, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Simulate right mouse-up at cursor position" }, "S&M_MOUSE_RU_CLICK", simulateMouseClick, NULL, 5},
*/
	{ { DEFACCEL, "SWS/S&M: Save ALR Wiki summary (w/o extensions)" }, "S&M_ALRSUMMARY1", dumpWikiActions2, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Save ALR Wiki summary (SWS/Xenakios/S&M extensions only)" }, "S&M_ALRSUMMARY2", dumpWikiActions2, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Save ALR Wiki summary (FNG extensions only)" }, "S&M_ALRSUMMARY3", dumpWikiActions2, NULL, 3},
#endif
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
	{ { DEFACCEL, "SWS/S&M: test -> Padre show take volume envelope" }, "S&M_TMP1", ShowTakeEnvPadreTest, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: test -> Padre show take pan envelope" }, "S&M_TMP2", ShowTakeEnvPadreTest, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: test -> Padre show take mute envelope" }, "S&M_TMP3", ShowTakeEnvPadreTest, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: stuff..." }, "S&M_TMP4", openStuff, NULL, },
#endif	

	{ {}, LAST_COMMAND, }, // Denote end of table
};


// *** Action toggle states ***

bool g_fakeToggleStates[MAX_ACTION_COUNT+1]; // +1 for no-op, 1-based..

bool fakeIsToggledAction(COMMAND_T* _ct) {
	return ((_ct && _ct->accel.accel.cmd && _ct->accel.accel.cmd < MAX_ACTION_COUNT) ? 
		g_fakeToggleStates[_ct->accel.accel.cmd] : false);
}

void fakeToggleAction(COMMAND_T* _ct) {
	if (_ct && _ct->accel.accel.cmd && _ct->accel.accel.cmd < MAX_ACTION_COUNT)
		g_fakeToggleStates[_ct->accel.accel.cmd] = !g_fakeToggleStates[_ct->accel.accel.cmd];
}


///////////////////////////////////////////////////////////////////////////////
// "S&M Extension" action section
///////////////////////////////////////////////////////////////////////////////

#define SNM_SECTION_MAX_ACTION_COUNT	128


// Globals
static WDL_IntKeyedArray<MIDI_COMMAND_T*> g_SNMSection_midiCmds;
static WDL_IntKeyedArray<MIDI_COMMAND_T*> g_SNMSection_toggles;
WDL_PtrList<struct KbdAccel>* g_SNMSection_reaAccels;  
WDL_PtrList<struct CommandAction>* g_SNMSection_reaRecentCmds;
KbdCmd g_SNMSection_kbdCmds[SNM_SECTION_MAX_ACTION_COUNT];
KbdKeyBindingInfo g_SNMSection_defKeys[SNM_SECTION_MAX_ACTION_COUNT];
int g_SNMSection_minCmdId = 0;
int g_SNMSection_maxCmdId = 0;


// S&M actions (in "S&M Extension" section) 
static MIDI_COMMAND_T g_SNMSection_cmdTable[] = 
{
	{ { DEFACCEL, "SWS/S&M: Apply live config 1 (MIDI CC only)" }, "S&M_LIVECONFIG1", ApplyLiveConfig, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Apply live config 2 (MIDI CC only)" }, "S&M_LIVECONFIG2", ApplyLiveConfig, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Apply live config 3 (MIDI CC only)" }, "S&M_LIVECONFIG3", ApplyLiveConfig, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Apply live config 4 (MIDI CC only)" }, "S&M_LIVECONFIG4", ApplyLiveConfig, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Apply live config 5 (MIDI CC only)" }, "S&M_LIVECONFIG5", ApplyLiveConfig, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Apply live config 6 (MIDI CC only)" }, "S&M_LIVECONFIG6", ApplyLiveConfig, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Apply live config 7 (MIDI CC only)" }, "S&M_LIVECONFIG7", ApplyLiveConfig, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Apply live config 8 (MIDI CC only)" }, "S&M_LIVECONFIG8", ApplyLiveConfig, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Select project (MIDI CC only)" }, "S&M_SELECT_PROJECT", SelectProject, NULL, 7},
	{ {}, LAST_COMMAND, }, // Denote end of table
};


bool onAction(int _cmd, int _val, int _valhw, int _relmode, HWND _hwnd)
{
	static bool bReentrancyCheck = false;
	if (bReentrancyCheck)
		return false;

	// Ignore commands that don't have anything to do with us from this point forward
	if (_cmd < g_SNMSection_minCmdId || _cmd > g_SNMSection_maxCmdId)
		return false;

	MIDI_COMMAND_T* ct = g_SNMSection_midiCmds.Get(_cmd, NULL);
	if (ct)
	{
		bReentrancyCheck = true;
		ct->doCommand(ct, _val, _valhw, _relmode, _hwnd);
		bReentrancyCheck = false;
		return true;
	}
	return false;
}

static KbdSectionInfo g_SNMSection = {
  0x10000101, "S&M Extension",
  g_SNMSection_kbdCmds, 0,
  g_SNMSection_defKeys, 0,
  onAction,
  g_SNMSection_reaAccels,
  g_SNMSection_reaRecentCmds,
  NULL
};

int SNMSectionRegisterCommands(reaper_plugin_info_t* _rec)
{
	if (!_rec)
		return 0;

	// Register commands in specific section
	int i = 0;
	while(g_SNMSection_cmdTable[i].id != LAST_COMMAND)
	{
		MIDI_COMMAND_T* ct = &g_SNMSection_cmdTable[i]; 
		if (ct->doCommand)
		{
			if (!(ct->accel.accel.cmd = plugin_register("command_id", (void*)ct->id)))
				return 0;

			if (ct->getEnabled)
				g_SNMSection_toggles.Insert(ct->accel.accel.cmd, ct);

			if (!g_SNMSection_minCmdId && g_SNMSection_minCmdId > ct->accel.accel.cmd)
				g_SNMSection_minCmdId = ct->accel.accel.cmd;
			if (ct->accel.accel.cmd > g_SNMSection_maxCmdId)
				g_SNMSection_maxCmdId = ct->accel.accel.cmd;

			g_SNMSection_midiCmds.Insert(ct->accel.accel.cmd, ct);
		}
		i++;
	}

	int nbCmds = i;
	if (nbCmds > SNM_SECTION_MAX_ACTION_COUNT)
		return 0;

	// Map MIDI_COMMAND_T[] to the section's KbdCmd[] & KbdKeyBindingInfo[]
	for (i=0; i < nbCmds; i++)
	{
		MIDI_COMMAND_T* ct = &g_SNMSection_cmdTable[i]; 
		g_SNMSection.action_list[i].text = ct->accel.desc;
		g_SNMSection.action_list[i].cmd = g_SNMSection.def_keys[i].cmd = ct->accel.accel.cmd;
		g_SNMSection.def_keys[i].key = ct->accel.accel.key;
		g_SNMSection.def_keys[i].flags = ct->accel.accel.fVirt;
	}
	g_SNMSection.action_list_cnt = g_SNMSection.def_keys_cnt = nbCmds;

	// Register S&M section
	if (!(_rec->Register("accel_section",&g_SNMSection)))
		return 0;

	return 1;
}


///////////////////////////////////////////////////////////////////////////////
// Entry points
///////////////////////////////////////////////////////////////////////////////

WDL_String g_SNMiniFilename;

int SnMInit(reaper_plugin_info_t* _rec)
{
	// for probable future persistance of fake toggle states..
	for (int i=0; i <= MAX_ACTION_COUNT; i++)
		g_fakeToggleStates[i] = false;

	// Actions should be registered before views
	if (!SWSRegisterCommands(g_SNM_cmdTable))
		return 0;
	if (!SNMSectionRegisterCommands(_rec))
		return 0;

	// Init S&M.ini file (+ "upgrade" old ones to the new REAPER's resource path, if needed)
	char oldIniFilename[BUFFER_SIZE], iniFilename[BUFFER_SIZE];
	_snprintf(oldIniFilename, BUFFER_SIZE, SNM_OLD_FORMATED_INI_FILE, GetExePath()); // old location
	_snprintf(iniFilename, BUFFER_SIZE, SNM_FORMATED_INI_FILE, GetResourcePath());
	if (FileExists(oldIniFilename))
		MoveFile(oldIniFilename, iniFilename);
	g_SNMiniFilename.Set(iniFilename);

	// Init S&M views
	MidiLiveViewInit();
	NotesHelpViewInit();
	FXChainViewInit();
	FindViewInit();
	return 1;
}

void SnMExit()
{
	MidiLiveViewExit();
	FXChainViewExit();
	NotesHelpViewExit();
	FindViewExit();
}


///////////////////////////////////////////////////////////////////////////////
// SNM_ScheduledJob (performed in SnMCSurfRun())
//
// List of reserved ids:
// - 0 to 7: Apply live config n (MIDI CC only), with n in [1;8]
// - 8:		SNM_TrackListChangeScheduledJob
//
///////////////////////////////////////////////////////////////////////////////

WDL_PtrList_DeleteOnDestroy<SNM_ScheduledJob> g_jobs;
HANDLE g_jobsLock;

void AddOrReplaceScheduledJob(SNM_ScheduledJob* _job) 
{
	if (!_job)
		return;

	SectionLock lock(g_jobsLock);
	bool found = false;
	for (int i=0; i<g_jobs.GetSize(); i++)
	{
		SNM_ScheduledJob* job = g_jobs.Get(i);
		if (job->m_id == _job->m_id)
		{
			found = true;
			g_jobs.Set(i, _job);
			delete job;
			break;
		}
	}

	if (!found)
		g_jobs.Add(_job);
}

void DeleteScheduledJob(int _id) 
{
	SectionLock lock(g_jobsLock);
	for (int i=0; i<g_jobs.GetSize(); i++)
	{
		SNM_ScheduledJob* job = g_jobs.Get(i);
		if (job->m_id == _id) { 
			g_jobs.Delete(i, true);
			break;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// IReaperControlSurface callbacks
///////////////////////////////////////////////////////////////////////////////

extern SNM_NotesHelpWnd* g_pNotesHelpWnd;
extern SnM_MidiLiveWnd* g_pMidiLiveWnd;

void SnMCSurfRun()
{
	SectionLock lock(g_jobsLock);
	if (g_jobs.GetSize())
	{
		for (int i=g_jobs.GetSize()-1; i >=0; i--)
		{
			SNM_ScheduledJob* job = g_jobs.Get(i);
			job->m_tick--;
			if (job->m_tick <= 0)
			{
				job->Perform();
				g_jobs.Delete(i, true);
			}
		}
	}
}

void SnMCSurfSetTrackTitle() {
	if (g_pNotesHelpWnd)
		g_pNotesHelpWnd->CSurfSetTrackTitle();
	if (g_pMidiLiveWnd)
		g_pMidiLiveWnd->CSurfSetTrackTitle();
}

void SnMCSurfSetTrackListChange() {
	if (g_pNotesHelpWnd)
		g_pNotesHelpWnd->CSurfSetTrackListChange();
	if (g_pMidiLiveWnd)
		g_pMidiLiveWnd->CSurfSetTrackListChange();
}
