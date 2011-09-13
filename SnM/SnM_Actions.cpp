/******************************************************************************
/ SnM_Actions.cpp
/
/ Copyright (c) 2009-2011 Tim Payne (SWS), Jeffos
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
#include "SnM_NotesHelpView.h"
#include "SnM_MidiLiveView.h"
#include "../Misc/Adam.h"


///////////////////////////////////////////////////////////////////////////////
// S&M "static" actions (main section)
///////////////////////////////////////////////////////////////////////////////

static COMMAND_T g_SNM_cmdTable[] = 
{
	// Beware! S&M actions expect "SWS/S&M: " in their names (removed from undo messages: too long)

	// Routing & cue buss -----------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Create cue buss track from track selection, pre-fader (post-FX)" }, "S&M_SENDS1", cueTrack, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Create cue buss track from track selection, post-fader (post-pan)" }, "S&M_SENDS2", cueTrack, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Create cue buss track from track selection, pre-FX" }, "S&M_SENDS3", cueTrack, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Open Cue Buss generator" }, "S&M_SENDS4", openCueBussWnd, "S&&M Cue Buss generator", NULL, isCueBussWndDisplayed},
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
#ifdef _SNM_MISC	// fragile sucky solution + win only + limited (toggle actions are not able to *open* wnds, just hide/show once opened) 
					// + only makes sense with the pref "General > Adavanced UI/System > allow env./routing windows to stay open" 
#ifdef _WIN32
	{ { DEFACCEL, "SWS/S&M: Close all routing windows" }, "S&M_WNCLS1", closeAllRoutingWindows, NULL, },
	{ { DEFACCEL, "SWS/S&M: Close all envelope windows" }, "S&M_WNCLS2", closeAllEnvWindows, NULL, },
	{ { DEFACCEL, "SWS/S&M: Toggle show all routing windows" }, "S&M_WNTGL1", toggleAllRoutingWindows, NULL, -666, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle show all envelope windows" }, "S&M_WNTGL2", toggleAllEnvWindows, NULL, -666, fakeIsToggledAction},
#endif
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
#ifdef _SNM_MISC // only diff. with the ones below is that they don't focus the main window on cycle
	{ { DEFACCEL, "SWS/S&M: Focus previous floating FX for selected tracks (cycle)" }, "S&M_WNFOCUS1", cycleFocusFXWndSelTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Focus next floating FX for selected tracks (cycle)" }, "S&M_WNFOCUS2", cycleFocusFXWndSelTracks, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Focus previous floating FX (cycle)" }, "S&M_WNFOCUS3", cycleFocusFXWndAllTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Focus next floating FX (cycle)" }, "S&M_WNFOCUS4", cycleFocusFXWndAllTracks, NULL, 1},
#endif
	// see above comment, used to be "... (+ main window on cycle)" => turned into "... (cycle)"
	{ { DEFACCEL, "SWS/S&M: Focus previous floating FX for selected tracks (cycle)" }, "S&M_WNFOCUS5", cycleFocusFXMainWndSelTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Focus next floating FX for selected tracks (cycle)" }, "S&M_WNFOCUS6", cycleFocusFXMainWndSelTracks, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Focus previous floating FX (cycle)" }, "S&M_WNFOCUS7", cycleFocusFXMainWndAllTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Focus next floating FX (cycle)" }, "S&M_WNFOCUS8", cycleFocusFXMainWndAllTracks, NULL, 1},

	{ { DEFACCEL, "SWS/S&M: Focus main window" }, "S&M_WNMAIN", focusMainWindow, NULL, },
#ifdef _WIN32
	{ { DEFACCEL, "SWS/S&M: Focus main window (close others)" }, "S&M_WNMAIN_HIDE_OTHERS", focusMainWindowCloseOthers, NULL, },
	{ { DEFACCEL, "SWS/S&M: Cycle focused window" }, "S&M_WNFOCUS9", cycleFocusWnd, NULL, },
	{ { DEFACCEL, "SWS/S&M: Focus next window (cycle, hide/unhide others)" }, "S&M_WNFOCUS_NEXT", cycleFocusHideOthersWnd, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Focus previous window (cycle, hide/unhide others)" }, "S&M_WNFOCUS_PREV", cycleFocusHideOthersWnd, NULL, -1},
#endif

	{ { DEFACCEL, "SWS/S&M: Show FX chain for selected tracks (selected FX)" }, "S&M_SHOWFXCHAINSEL", showFXChain, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Hide FX chain windows for selected tracks" }, "S&M_HIDEFXCHAIN", hideFXChain, NULL, },
	{ { DEFACCEL, "SWS/S&M: Toggle show FX chain windows for selected tracks" }, "S&M_TOGLFXCHAIN", toggleFXChain, NULL, -666, isToggleFXChain},

	{ { DEFACCEL, "SWS/S&M: Float selected FX for selected tracks" }, "S&M_FLOATFXEL", floatFX, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Unfloat selected FX for selected tracks" }, "S&M_UNFLOATFXEL", unfloatFX, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Toggle float selected FX for selected tracks" }, "S&M_TOGLFLOATFXEL", toggleFloatFX, NULL, -1, fakeIsToggledAction},

	// Track FX selection & move up/down---------------------------------------
	{ { DEFACCEL, "SWS/S&M: Select last FX for selected tracks" }, "S&M_SEL_LAST_FX", selectTrackFX, NULL, -3},
	{ { DEFACCEL, "SWS/S&M: Select previous FX (cycling) for selected tracks" }, "S&M_SELFXPREV", selectTrackFX, NULL, -2},
	{ { DEFACCEL, "SWS/S&M: Select next FX (cycling) for selected tracks" }, "S&M_SELFXNEXT", selectTrackFX, NULL, -1},

	{ { DEFACCEL, "SWS/S&M: Move selected FX up in chain for selected tracks" }, "S&M_MOVE_FX_UP", moveFX, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Move selected FX down in chain for selected tracks" }, "S&M_MOVE_FX_DOWN", moveFX, NULL, 1},

	// Track FX online/offline & bypass/unbypass ------------------------------
	//JFB TODO: configurable dynamic actions (but ct->user needs to be 0-based first)
	{ { DEFACCEL, "SWS/S&M: Toggle FX 1 online/offline for selected tracks" }, "S&M_FXOFF1", toggleFXOfflineSelectedTracks, NULL, 1, isFXOfflineSelectedTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 2 online/offline for selected tracks" }, "S&M_FXOFF2", toggleFXOfflineSelectedTracks, NULL, 2, isFXOfflineSelectedTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 3 online/offline for selected tracks" }, "S&M_FXOFF3", toggleFXOfflineSelectedTracks, NULL, 3, isFXOfflineSelectedTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 4 online/offline for selected tracks" }, "S&M_FXOFF4", toggleFXOfflineSelectedTracks, NULL, 4, isFXOfflineSelectedTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 5 online/offline for selected tracks" }, "S&M_FXOFF5", toggleFXOfflineSelectedTracks, NULL, 5, isFXOfflineSelectedTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 6 online/offline for selected tracks" }, "S&M_FXOFF6", toggleFXOfflineSelectedTracks, NULL, 6, isFXOfflineSelectedTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 7 online/offline for selected tracks" }, "S&M_FXOFF7", toggleFXOfflineSelectedTracks, NULL, 7, isFXOfflineSelectedTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 8 online/offline for selected tracks" }, "S&M_FXOFF8", toggleFXOfflineSelectedTracks, NULL, 8, isFXOfflineSelectedTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle last FX online/offline for selected tracks" }, "S&M_FXOFFLAST", toggleFXOfflineSelectedTracks, NULL, -1, isFXOfflineSelectedTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle selected FX online/offline for selected tracks" }, "S&M_FXOFFSEL", toggleFXOfflineSelectedTracks, NULL, 0, isFXOfflineSelectedTracks},

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
	// ..equivalent online/offline actions exist natively

	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except selected) online/offline for selected tracks" }, "S&M_FXOFFEXCPTSEL", toggleExceptFXOfflineSelectedTracks, NULL, 0, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except selected) bypass for selected tracks" }, "S&M_FXBYPEXCPTSEL", toggleExceptFXBypassSelectedTracks, NULL, 0, fakeIsToggledAction},
#ifdef _SNM_MISC // very specific..
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

	// Take FX online/offline & bypass/unbypass ------------------------------
	{ { DEFACCEL, "SWS/S&M: Toggle all take FX online/offline for selected items" }, "S&M_TGL_TAKEFX_ONLINE", toggleAllFXsOfflineSelectedItems, NULL, -666, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Set all take FX offline for selected items" }, "S&M_TAKEFX_OFFLINE", setAllFXsOfflineSelectedItems, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Set all take FX online for selected items" }, "S&M_TAKEFX_ONLINE", setAllFXsOfflineSelectedItems, NULL, 0},

	{ { DEFACCEL, "SWS/S&M: Toggle all take FX bypass for selected items" }, "S&M_TGL_TAKEFX_BYP", toggleAllFXsBypassSelectedItems, NULL, -666, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Bypass all take FX for selected items" }, "S&M_TAKEFX_BYPASS", setAllFXsBypassSelectedItems, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Unbypass all take FX for selected items" }, "S&M_TAKEFX_UNBYPASS", setAllFXsBypassSelectedItems, NULL, 0},

	// FX Chains (items & tracks) ---------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Open Resources window (FX chains)" }, "S&M_SHOWFXCHAINSLOTS", OpenResourceView, "S&&M Resources", 0, IsResourceViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Clear FX chain slot..." }, "S&M_CLRFXCHAINSLOT", ResourceViewClearSlotPrompt, NULL, 0},
//	{ { DEFACCEL, "SWS/S&M: Auto-save FX Chains" }, "S&M_SAVE_FXCHAIN_SLOT", ResourceViewAutoSave, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Apply FX chain to selected items, prompt for slot" }, "S&M_TAKEFXCHAINp1", loadSetTakeFXChain, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Apply FX chain to selected items, all takes, prompt for slot" }, "S&M_TAKEFXCHAINp2", loadSetAllTakesFXChain, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Paste FX chain to selected items, prompt for slot" }, "S&M_PASTE_TAKEFXCHAINp1", loadPasteTakeFXChain, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Paste FX chain to selected items, all takes, prompt for slot" }, "S&M_PASTE_TAKEFXCHAINp2", loadPasteAllTakesFXChain, NULL, -1},

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

	{ { DEFACCEL, "SWS/S&M: Copy input FX chain from selected track" }, "S&M_COPY_INFXCHAIN", copyTrackInputFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Cut input FX chain from selected tracks" }, "S&M_CUT_INFXCHAIN", cutTrackInputFXChain, NULL, },
	{ { DEFACCEL, "SWS/S&M: Paste (replace) input FX chain to selected tracks" }, "S&M_PASTE_REPLACE_INFXCHAIN", setTrackInputFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Paste input FX chain to selected tracks" }, "S&M_PASTE_INFXCHAIN", pasteTrackInputFXChain, NULL, }, 

	{ { DEFACCEL, "SWS/S&M: Clear FX chain for selected items" },  "S&M_CLRFXCHAIN1", clearActiveTakeFXChain, NULL, },
	{ { DEFACCEL, "SWS/S&M: Clear FX chain for selected items, all takes" },  "S&M_CLRFXCHAIN2", clearAllTakesFXChain, NULL, },
	{ { DEFACCEL, "SWS/S&M: Clear FX chain for selected tracks" }, "S&M_CLRFXCHAIN3", clearTrackFXChain, NULL, },
	{ { DEFACCEL, "SWS/S&M: Clear input FX chain for selected tracks" }, "S&M_CLR_INFXCHAIN", clearTrackInputFXChain, NULL, },

	{ { DEFACCEL, "SWS/S&M: Copy FX chain (depending on focus)" }, "S&M_SMART_CPY_FXCHAIN", smartCopyFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Paste FX chain (depending on focus)" }, "S&M_SMART_PST_FXCHAIN", smartPasteFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Paste (replace) FX chain (depending on focus)" }, "S&M_SMART_SET_FXCHAIN", smartPasteReplaceFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Cut FX chain (depending on focus)" }, "S&M_SMART_CUT_FXCHAIN", smartCutFXChain, NULL, }, 

	{ { DEFACCEL, "SWS/S&M: Apply FX chain to selected tracks, prompt for slot" }, "S&M_TRACKFXCHAINp1", loadSetTrackFXChain, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Paste FX chain to selected tracks, prompt for slot" }, "S&M_PASTE_TRACKFXCHAINp1", loadPasteTrackFXChain, NULL, -1},

	// FX presets -------------------------------------------------------------
#ifdef _SNM_PRESETS
	{ { DEFACCEL, "SWS/S&M: Trigger next preset for selected FX of selected tracks" }, "S&M_NEXT_SELFX_PRESET", triggerNextPreset, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Trigger previous preset for selected FX of selected tracks" }, "S&M_PREVIOUS_SELFX_PRESET", triggerPreviousPreset, NULL, -1},
#endif
	
	// MIDI learn -------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Reassign MIDI learned channels of all FX for selected tracks (prompt)" }, "S&M_ALL_FX_LEARN_CHp", reassignLearntMIDICh, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Reassign MIDI learned channels of selected FX for selected tracks (prompt)" }, "S&M_SEL_FX_LEARN_CHp", reassignLearntMIDICh, NULL, -2},
	{ { DEFACCEL, "SWS/S&M: Reassign MIDI learned channels of all FX to input channel for selected tracks" }, "S&M_ALLIN_FX_LEARN_CHp", reassignLearntMIDICh, NULL, -3},
	
	// Track templates --------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Open Resources window (track templates)" }, "S&M_SHOW_RESVIEW_TR_TEMPLATES", OpenResourceView, NULL, 1, IsResourceViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Clear track template slot..." }, "S&M_CLR_TRTEMPLATE_SLOT", ResourceViewClearSlotPrompt, NULL, 1},
//	{ { DEFACCEL, "SWS/S&M: Auto-save track template" }, "S&M_SAVE_TRTEMPLATE_SLOT", ResourceViewAutoSave, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Apply track template to selected tracks, prompt for slot" }, "S&M_APPLY_TRTEMPLATEp", loadSetTrackTemplate, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Import tracks from track template, prompt for slot" }, "S&M_ADD_TRTEMPLATEp", loadImportTrackTemplate, NULL, -1},

	// Projects & project templates -------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Open Resources window (project templates)" }, "S&M_SHOW_RESVIEW_PRJ_TEMPLATES", OpenResourceView, NULL, 2, IsResourceViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Clear project template slot..." }, "S&M_CLR_PRJTEMPLATE_SLOT", ResourceViewClearSlotPrompt, NULL, 2},
//	{ { DEFACCEL, "SWS/S&M: Auto-save project template" }, "S&M_SAVE_PRJTEMPLATE_SLOT", ResourceViewAutoSave, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Select/load project template, prompt for slot" }, "S&M_APPLY_PRJTEMPLATEp", loadOrSelectProject, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Select/load project template (new tab), prompt for slot" }, "S&M_NEWTAB_PRJTEMPLATEp", loadOrSelectProjectNewTab, NULL, -1},

	{ { DEFACCEL, "SWS/S&M: Project loader/selecter: configuration" }, "S&M_PRJ_LOADER_CONF", projectLoaderConf, NULL, },
	{ { DEFACCEL, "SWS/S&M: Project loader/selecter: next (cycle)" }, "S&M_PRJ_LOADER_NEXT", loadOrSelectNextPreviousProject, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Project loader/selecter: previous (cycle)" }, "S&M_PRJ_LOADER_PREV", loadOrSelectNextPreviousProject, NULL, -1},

	{ { DEFACCEL, "SWS/S&M: Open project path in explorer/finder" }, "S&M_OPEN_PRJ_PATH", openProjectPathInExplorerFinder, NULL, },
	
	// Items ------------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Scroll to selected item (no undo)" }, "S&M_SCROLL_ITEM", scrollToSelItem, NULL, },

	// Takes ------------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Pan active takes of selected items to 100% right" }, "S&M_PAN_TAKES_100R", setPan, NULL, -100},
	{ { DEFACCEL, "SWS/S&M: Pan active takes of selected items to 100% left" }, "S&M_PAN_TAKES_100L", setPan, NULL, 100},
	{ { DEFACCEL, "SWS/S&M: Pan active takes of selected items to center" }, "S&M_PAN_TAKES_CENTER", setPan, NULL, 0},

	{ { DEFACCEL, "SWS/S&M: Copy active take" }, "S&M_COPY_TAKE", copyCutTake, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Cut active take" }, "S&M_CUT_TAKE", copyCutTake, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Paste take" }, "S&M_PASTE_TAKE", pasteTake, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Paste take (after active take)" }, "S&M_PASTE_TAKE_AFTER", pasteTake, NULL, 1},

	{ { DEFACCEL, "SWS/S&M: Takes - Clear active takes/items" }, "S&M_CLRTAKE1", clearTake, NULL, },
	{ { DEFACCEL, "SWS/S&M: Takes - Build lanes for selected tracks" }, "S&M_LANETAKE1", buildLanes, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Takes - Activate lane from selected item" }, "S&M_LANETAKE2", activateLaneFromSelItem, NULL, },
	{ { DEFACCEL, "SWS/S&M: Takes - Build lanes for selected items" }, "S&M_LANETAKE3", buildLanes, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Takes - Activate lane under mouse cursor" }, "S&M_LANETAKE4", activateLaneUnderMouse, NULL, },
	{ { DEFACCEL, "SWS/S&M: Takes - Remove empty takes/items among selected items" }, "S&M_DELEMPTYTAKE", removeAllEmptyTakes/*removeEmptyTakes*/, NULL, },
	{ { DEFACCEL, "SWS/S&M: Takes - Remove empty MIDI takes/items among selected items" }, "S&M_DELEMPTYTAKE2", removeEmptyMidiTakes, NULL, },
#ifdef _SNM_MISC // seems confusing, the above one is fine..
	{ { DEFACCEL, "SWS/S&M: Takes - Remove all empty takes/items among selected items" }, "S&M_DELEMPTYTAKE3", removeAllEmptyTakes, NULL, },
#endif
	{ { DEFACCEL, "SWS/S&M: Takes - Move active up (cycling) in selected items" }, "S&M_MOVETAKE3", moveActiveTake, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Takes - Move active down (cycling) in selected items" }, "S&M_MOVETAKE4", moveActiveTake, NULL, 1},
#ifdef _SNM_MISC // deprecated: native actions "Rotate take lanes forward/backward" added in REAPER v3.67
	{ { DEFACCEL, "SWS/S&M: Takes - Move all up (cycling) in selected items" }, "S&M_MOVETAKE1", moveTakes, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Takes - Move all down (cycling) in selected items" }, "S&M_MOVETAKE2", moveTakes, NULL, 1},
#endif

	{ { DEFACCEL, "SWS/S&M: Delete selected items' takes and source files (prompt, no undo)" }, "S&M_DELTAKEANDFILE1", deleteTakeAndMedia, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Delete selected items' takes and source files (no undo)" }, "S&M_DELTAKEANDFILE2", deleteTakeAndMedia, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Delete active take and source file in selected items (prompt, no undo)" }, "S&M_DELTAKEANDFILE3", deleteTakeAndMedia, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Delete active take and source file in selected items (no undo)" }, "S&M_DELTAKEANDFILE4", deleteTakeAndMedia, NULL, 4},

#ifdef _SNM_MISC
	{ { DEFACCEL, "SWS/S&M: Save selected item as item/take template..." }, "S&M_SAVEITEMTAKETEMPLATE", saveItemTakeTemplate, NULL, },
#endif

	// Notes/Help -------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Open Notes/Help window (project notes)" }, "S&M_SHOWNOTESHELP", OpenNotesHelpView, "S&&M Notes/Help", 0, IsNotesHelpViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Open Notes/Help window (item notes)" }, "S&M_ITEMNOTES", OpenNotesHelpView, NULL, 1, IsNotesHelpViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Open Notes/Help window (track notes)" }, "S&M_TRACKNOTES", OpenNotesHelpView, NULL, 2, IsNotesHelpViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Open Notes/Help window (marker names)" }, "S&M_MARKERNAMES", OpenNotesHelpView, NULL, 3, IsNotesHelpViewDisplayed},
#ifdef _WIN32
	{ { DEFACCEL, "SWS/S&M: Open Notes/Help window (action help)" }, "S&M_ACTIONHELP", OpenNotesHelpView, NULL, 4, IsNotesHelpViewDisplayed},
#endif
	{ { DEFACCEL, "SWS/S&M: Notes/Help - Toggle lock" }, "S&M_ACTIONHELPTGLOCK", ToggleNotesHelpLock, NULL, NULL, IsNotesHelpLocked},
	{ { DEFACCEL, "SWS/S&M: Notes/Help - Set action help file..." }, "S&M_ACTIONHELPPATH", SetActionHelpFilename, NULL, },

	// Split ------------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Split selected items at edit cursor (MIDI) or prior zero crossing (audio)" }, "S&M_SPLIT1", splitMidiAudio, NULL, },
	{ { DEFACCEL, "SWS/S&M: Split selected items at time selection, edit cursor (MIDI) or prior zero crossing (audio)" }, "S&M_SPLIT2", smartSplitMidiAudio, NULL, },
#ifdef _SNM_MISC
//  Deprecated: contrary to their native versions, the following actions were spliting selected items *and only them*, 
//  see http://forum.cockos.com/showthread.php?t=51547.
//  Due to REAPER v3.67's new native pref "If no items are selected, some split/trim/delete actions affect all items at the edit cursor", 
//  those actions are less useful: they would still split only selected items, even if that native pref is ticked. 
//  Also removed because of the spam in the action list (many split actions).
	{ { DEFACCEL, "SWS/S&M: Split selected items at play cursor" }, "S&M_SPLIT3", splitSelectedItems, NULL, 40196},
	{ { DEFACCEL, "SWS/S&M: Split selected items at time selection" }, "S&M_SPLIT4", splitSelectedItems, NULL, 40061},
	{ { DEFACCEL, "SWS/S&M: Split selected items at edit cursor (no change selection)" }, "S&M_SPLIT5", splitSelectedItems, NULL, 40757},
	{ { DEFACCEL, "SWS/S&M: Split selected items at time selection (select left)" }, "S&M_SPLIT6", splitSelectedItems, NULL, 40758},
	{ { DEFACCEL, "SWS/S&M: Split selected items at time selection (select right)" }, "S&M_SPLIT7", splitSelectedItems, NULL, 40759},
	{ { DEFACCEL, "SWS/S&M: Split selected items at edit or play cursor" }, "S&M_SPLIT8", splitSelectedItems, NULL, 40012},
	{ { DEFACCEL, "SWS/S&M: Split selected items at edit or play cursor (ignoring grouping)" }, "S&M_SPLIT9", splitSelectedItems, NULL, 40186},
#endif
	{ { DEFACCEL, "SWS/gofer: Split selected items at mouse cursor (obey snapping)" }, "S&M_SPLIT10", goferSplitSelectedItems, NULL, },

	// ME ---------------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Active MIDI Editor - Hide all CC lanes" }, "S&M_MEHIDECCLANES", MEHideCCLanes, NULL, },
	{ { DEFACCEL, "SWS/S&M: Active MIDI Editor - Create CC lane" }, "S&M_MECREATECCLANE", MECreateCCLane, NULL, },

	// Tracks -----------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Copy selected track grouping" }, "S&M_COPY_TR_GRP", copyCutTrackGrouping, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Cut selected tracks grouping" }, "S&M_CUT_TR_GRP", copyCutTrackGrouping, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Paste grouping to selected tracks" }, "S&M_PASTE_TR_GRP", pasteTrackGrouping, NULL, },

	{ { DEFACCEL, "SWS/S&M: Save selected tracks folder states" }, "S&M_SAVEFOLDERSTATE1", saveTracksFolderStates, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Restore selected tracks folder states" }, "S&M_RESTOREFOLDERSTATE1", restoreTracksFolderStates, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks folder states to on" }, "S&M_FOLDERON", setTracksFolderState, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks folder states to off" }, "S&M_FOLDEROFF", setTracksFolderState, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Save selected tracks folder compact states" }, "S&M_SAVEFOLDERSTATE2", saveTracksFolderStates, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Restore selected tracks folder compact states" }, "S&M_RESTOREFOLDERSTATE2", restoreTracksFolderStates, NULL, 1},

	// Take envelopes ---------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Show take volume envelopes" }, "S&M_TAKEENV1", showHideTakeVolEnvelope, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Show take pan envelopes" }, "S&M_TAKEENV2", showHideTakePanEnvelope, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Show take mute envelopes" }, "S&M_TAKEENV3", showHideTakeMuteEnvelope, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Hide take volume envelopes" }, "S&M_TAKEENV4", showHideTakeVolEnvelope, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Hide take pan envelopes" }, "S&M_TAKEENV5", showHideTakePanEnvelope, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Hide take mute envelopes" }, "S&M_TAKEENV6", showHideTakeMuteEnvelope, NULL, 0},

	{ { DEFACCEL, "SWS/S&M: Set active take pan envelopes to 100% right" }, "S&M_TAKEENV_100R", panTakeEnvelope, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Set active take pan envelopes to 100% left" }, "S&M_TAKEENV_100L", panTakeEnvelope, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Set active take pan envelopes to center" }, "S&M_TAKEENV_CENTER", panTakeEnvelope, NULL, 0},
#ifdef _SNM_MISC // exist natively..
	{ { DEFACCEL, "SWS/S&M: Toggle show take volume envelope" }, "S&M_TAKEENV7", showHideTakeVolEnvelope, NULL, -1, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle show take pan envelope" }, "S&M_TAKEENV8", showHideTakePanEnvelope, NULL, -1, fakeIsToggledAction},
	{ { DEFACCEL, "SWS/S&M: Toggle show take mute envelope" }, "S&M_TAKEENV9", showHideTakeMuteEnvelope, NULL, -1, fakeIsToggledAction},
#endif
	{ { DEFACCEL, "SWS/S&M: Show take pitch envelope" }, "S&M_TAKEENV10", showHideTakePitchEnvelope, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Hide take pitch envelope" }, "S&M_TAKEENV11", showHideTakePitchEnvelope, NULL, 0},

	// Track envelopes --------------------------------------------------------
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

	// Toolbar ----------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Toggle toolbars auto refresh enable" },	"S&M_TOOLBAR_REFRESH_ENABLE", EnableToolbarsAutoRefesh, "Enable toolbars auto refresh", 0, IsToolbarsAutoRefeshEnabled},
	{ { DEFACCEL, "SWS/S&M: Toolbar track envelopes in touch/latch/write mode toggle" }, "S&M_TOOLBAR_WRITE_ENV", toggleWriteEnvExists, NULL, 0, writeEnvExists},
	{ { DEFACCEL, "SWS/S&M: Toolbar left item selection toggle" }, "S&M_TOOLBAR_ITEM_SEL0", toggleItemSelExists, NULL, SNM_ITEM_SEL_LEFT, itemSelExists},
	{ { DEFACCEL, "SWS/S&M: Toolbar right item selection toggle" },"S&M_TOOLBAR_ITEM_SEL1", toggleItemSelExists, NULL, SNM_ITEM_SEL_RIGHT, itemSelExists},
#ifdef _WIN32
	{ { DEFACCEL, "SWS/S&M: Toolbar top item selection toggle" }, "S&M_TOOLBAR_ITEM_SEL2", toggleItemSelExists, NULL, SNM_ITEM_SEL_UP, itemSelExists},
	{ { DEFACCEL, "SWS/S&M: Toolbar bottom item selection toggle" }, "S&M_TOOLBAR_ITEM_SEL3", toggleItemSelExists, NULL, SNM_ITEM_SEL_DOWN, itemSelExists},
#endif

	// Find -------------------------------------------------------------------
	{ { {FCONTROL | FVIRTKEY, 'F', 0 }, "SWS/S&M: Find" }, "S&M_SHOWFIND", OpenFindView, "S&&M Find", NULL, IsFindViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Find next" }, "S&M_FIND_NEXT", FindNextPrev, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Find previous" }, "S&M_FIND_PREVIOUS", FindNextPrev, NULL, -1},

	// Live Configs -----------------------------------------------------------
	//JFB TODO: "not configurable" dynamic actions
	{ { DEFACCEL, "SWS/S&M: Open Live Configs window" }, "S&M_SHOWMIDILIVE", OpenLiveConfigView, "S&&M Live Configs", NULL, IsLiveConfigViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Toggle enable live config 1" }, "S&M_TOGGLE_LIVE_CFG1", ToggleEnableLiveConfig, NULL, 0, IsLiveConfigEnabled},
	{ { DEFACCEL, "SWS/S&M: Toggle enable live config 2" }, "S&M_TOGGLE_LIVE_CFG2", ToggleEnableLiveConfig, NULL, 1, IsLiveConfigEnabled},
	{ { DEFACCEL, "SWS/S&M: Toggle enable live config 3" }, "S&M_TOGGLE_LIVE_CFG3", ToggleEnableLiveConfig, NULL, 2, IsLiveConfigEnabled},
	{ { DEFACCEL, "SWS/S&M: Toggle enable live config 4" }, "S&M_TOGGLE_LIVE_CFG4", ToggleEnableLiveConfig, NULL, 3, IsLiveConfigEnabled},
	{ { DEFACCEL, "SWS/S&M: Toggle enable live config 5" }, "S&M_TOGGLE_LIVE_CFG5", ToggleEnableLiveConfig, NULL, 4, IsLiveConfigEnabled},
	{ { DEFACCEL, "SWS/S&M: Toggle enable live config 6" }, "S&M_TOGGLE_LIVE_CFG6", ToggleEnableLiveConfig, NULL, 5, IsLiveConfigEnabled},
	{ { DEFACCEL, "SWS/S&M: Toggle enable live config 7" }, "S&M_TOGGLE_LIVE_CFG7", ToggleEnableLiveConfig, NULL, 6, IsLiveConfigEnabled},
	{ { DEFACCEL, "SWS/S&M: Toggle enable live config 8" }, "S&M_TOGGLE_LIVE_CFG8", ToggleEnableLiveConfig, NULL, 7, IsLiveConfigEnabled},

	// Cyclactions ---------------------------------------------------------------
#ifdef _WIN32
	{ { DEFACCEL, "SWS/S&M: Open Cycle Action editor" }, "S&M_CREATE_CYCLACTION", openCyclactionsWnd, "S&&M Cycle Action editor", 0, isCyclationsWndDisplayed},
	{ { DEFACCEL, "SWS/S&M: Open Cycle Action editor (MIDI editor event list)" }, "S&M_CREATE_ME_LIST_CYCLACTION", openCyclactionsWnd, "S&&M Cycle Action editor", 1, isCyclationsWndDisplayed},
	{ { DEFACCEL, "SWS/S&M: Open Cycle Action editor (MIDI editor piano roll)" }, "S&M_CREATE_ME_PIANO_CYCLACTION", openCyclactionsWnd, "S&&M Cycle Action editor", 2, isCyclationsWndDisplayed},
#else
	{ { DEFACCEL, "SWS/S&M: Create cycle action" }, "S&M_CREATE_CYCLACTION", openCyclactionsWnd, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Create cycle action (MIDI editor event list)" }, "S&M_CREATE_ME_LIST_CYCLACTION", openCyclactionsWnd, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Create cycle action (MIDI editor piano roll)" }, "S&M_CREATE_ME_PIANO_CYCLACTION", openCyclactionsWnd, NULL, 2},
#endif

	// REC inputs -------------------------------------------------------------
	//JFB TODO: "not configurable" dynamic actions
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to all channels" }, "S&M_MIDI_INPUT_ALL_CH", setMIDIInputChannel, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 01" }, "S&M_MIDI_INPUT_CH1", setMIDIInputChannel, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 02" }, "S&M_MIDI_INPUT_CH2", setMIDIInputChannel, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 03" }, "S&M_MIDI_INPUT_CH3", setMIDIInputChannel, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 04" }, "S&M_MIDI_INPUT_CH4", setMIDIInputChannel, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 05" }, "S&M_MIDI_INPUT_CH5", setMIDIInputChannel, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 06" }, "S&M_MIDI_INPUT_CH6", setMIDIInputChannel, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 07" }, "S&M_MIDI_INPUT_CH7", setMIDIInputChannel, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 08" }, "S&M_MIDI_INPUT_CH8", setMIDIInputChannel, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 09" }, "S&M_MIDI_INPUT_CH9", setMIDIInputChannel, NULL, 9},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 10" }, "S&M_MIDI_INPUT_CH10", setMIDIInputChannel, NULL, 10},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 11" }, "S&M_MIDI_INPUT_CH11", setMIDIInputChannel, NULL, 11},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 12" }, "S&M_MIDI_INPUT_CH12", setMIDIInputChannel, NULL, 12},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 13" }, "S&M_MIDI_INPUT_CH13", setMIDIInputChannel, NULL, 13},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 14" }, "S&M_MIDI_INPUT_CH14", setMIDIInputChannel, NULL, 14},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 15" }, "S&M_MIDI_INPUT_CH15", setMIDIInputChannel, NULL, 15},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 16" }, "S&M_MIDI_INPUT_CH16", setMIDIInputChannel, NULL, 16},

	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to source channel" }, "S&M_MAP_MIDI_INPUT_CH_SRC", remapMIDIInputChannel, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 01" }, "S&M_MAP_MIDI_INPUT_CH1", remapMIDIInputChannel, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 02" }, "S&M_MAP_MIDI_INPUT_CH2", remapMIDIInputChannel, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 03" }, "S&M_MAP_MIDI_INPUT_CH3", remapMIDIInputChannel, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 04" }, "S&M_MAP_MIDI_INPUT_CH4", remapMIDIInputChannel, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 05" }, "S&M_MAP_MIDI_INPUT_CH5", remapMIDIInputChannel, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 06" }, "S&M_MAP_MIDI_INPUT_CH6", remapMIDIInputChannel, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 07" }, "S&M_MAP_MIDI_INPUT_CH7", remapMIDIInputChannel, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 08" }, "S&M_MAP_MIDI_INPUT_CH8", remapMIDIInputChannel, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 09" }, "S&M_MAP_MIDI_INPUT_CH9", remapMIDIInputChannel, NULL, 9},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 10" }, "S&M_MAP_MIDI_INPUT_CH10", remapMIDIInputChannel, NULL, 10},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 11" }, "S&M_MAP_MIDI_INPUT_CH11", remapMIDIInputChannel, NULL, 11},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 12" }, "S&M_MAP_MIDI_INPUT_CH12", remapMIDIInputChannel, NULL, 12},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 13" }, "S&M_MAP_MIDI_INPUT_CH13", remapMIDIInputChannel, NULL, 13},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 14" }, "S&M_MAP_MIDI_INPUT_CH14", remapMIDIInputChannel, NULL, 14},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 15" }, "S&M_MAP_MIDI_INPUT_CH15", remapMIDIInputChannel, NULL, 15},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 16" }, "S&M_MAP_MIDI_INPUT_CH16", remapMIDIInputChannel, NULL, 16},

	// Other, misc ------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Show action list (S&M Extension section)" }, "S&M_ACTION_LIST", SNM_ShowActionList, NULL, 0},
#ifdef _WIN32
	{ { DEFACCEL, "SWS/S&M: Show theme helper (all tracks)" }, "S&M_THEME_HELPER_ALL", ShowThemeHelper, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Show theme helper (selected track)" }, "S&M_THEME_HELPER_SEL", ShowThemeHelper, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Left mouse click at cursor position (use w/o modifier)" }, "S&M_MOUSE_L_CLICK", SimulateMouseClick, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Dump ALR Wiki summary (w/o SWS extension)" }, "S&M_ALRSUMMARY1", DumpWikiActionList2, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Dump ALR Wiki summary (SWS extension only)" }, "S&M_ALRSUMMARY2", DumpWikiActionList2, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Dump action list (w/o SWS extension)" }, "S&M_DUMP_ACTION_LIST", DumpActionList, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Dump action list (SWS extension only)" }, "S&M_DUMP_SWS_ACTION_LIST", DumpActionList, NULL, 4},
#endif
#ifdef _SNM_MISC // experimental, deprecated, etc.. 
	{ { DEFACCEL, "SWS/S&M: Let REAPER breathe" }, "S&M_LETBREATHE", LetREAPERBreathe, NULL, },
	{ { DEFACCEL, "SWS/S&M: test -> Padre show take volume envelope" }, "S&M_TMP1", ShowTakeEnvPadreTest, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: test -> Padre show take pan envelope" }, "S&M_TMP2", ShowTakeEnvPadreTest, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: test -> Padre show take mute envelope" }, "S&M_TMP3", ShowTakeEnvPadreTest, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: stuff..." }, "S&M_TMP4", OpenStuff, NULL, },
#endif

#ifdef _SWS_MENU
	{ { DEFACCEL, NULL }, NULL, NULL, SWS_SEPARATOR, }, // for main "Extensions" menu
#endif

	{ {}, LAST_COMMAND, }, // Denote end of table
};


///////////////////////////////////////////////////////////////////////////////
// S&M "dynamic" actions (main section)
//
// "dynamic" means that the number of instances of each action can be customized in the S&M.ini file (section "NbOfActions"). 
// in this g_SNM_dynamicCmdTable table:
// - items are not real commands but "meta" commands, this table must be registered with SNMRegisterDynamicCommands()
// - COMMAND_T.user is used to specify the default number of actions to create
// - a function doCommand(COMMAND_T*) or getEnabled(COMMAND_T*) will be trigered with 0-based COMMAND_T.user
// - action names are formated strings, they must contain "%02d" and only that. %02d is used for better sort in the action list (max = 99)
// - custom command ids aren't formated strings, but final ids will end with "slot" numbers (1-based for display reasons)
// example: { { DEFACCEL, "Do stuff #%02d" }, "DO_STUFF", doStuff, NULL, 2},
// if not overrided in the S&M.ini file (e.g. "DO_STUFF=99"), 2 actions will be created: "Do stuff #01" and "Do stuff #02" both calling
// doStuff(COMMAND_T* c) with c->user=0 and c->user=1, respectively. custom ids will be "_DO_STUFF1" and "_DO_STUFF2", repectively.
///////////////////////////////////////////////////////////////////////////////

static COMMAND_T g_SNM_dynamicCmdTable[] =
{
	{ { DEFACCEL, "SWS/S&M: Apply FX chain to selected items, slot %02d" }, "S&M_TAKEFXCHAIN", loadSetTakeFXChain, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Paste FX chain to selected items, slot %02d" }, "S&M_PASTE_TAKEFXCHAIN", loadPasteTakeFXChain, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Apply FX chain to selected items, all takes, slot %02d" }, "S&M_FXCHAIN_ALLTAKES", loadSetAllTakesFXChain, NULL, 0}, // default: none
	{ { DEFACCEL, "SWS/S&M: Paste FX chain to selected items, all takes, slot %02d" }, "S&M_PASTE_FXCHAIN_ALLTAKES", loadPasteAllTakesFXChain, NULL, 0}, // default: none

	{ { DEFACCEL, "SWS/S&M: Apply FX chain to selected tracks, slot %02d" }, "S&M_TRACKFXCHAIN", loadSetTrackFXChain, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Paste FX chain to selected tracks, slot %02d" }, "S&M_PASTE_TRACKFXCHAIN", loadPasteTrackFXChain, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Apply input FX chain to selected tracks, slot %02d" }, "S&M_INFXCHAIN", loadSetTrackInFXChain, NULL, 0}, // default: none
	{ { DEFACCEL, "SWS/S&M: Paste input FX chain to selected tracks, slot %02d" }, "S&M_PASTE_INFXCHAIN", loadPasteTrackInFXChain, NULL, 0}, // default: none

	{ { DEFACCEL, "SWS/S&M: Apply track template to selected tracks, slot %02d" }, "S&M_APPLY_TRTEMPLATE", loadSetTrackTemplate, NULL, 10},
	{ { DEFACCEL, "SWS/S&M: Import tracks from track template, slot %02d" }, "S&M_ADD_TRTEMPLATE", loadImportTrackTemplate, NULL, 10},

	{ { DEFACCEL, "SWS/S&M: Select/load project template, slot %02d" }, "S&M_APPLY_PRJTEMPLATE", loadOrSelectProject, NULL, 10},
	{ { DEFACCEL, "SWS/S&M: Select/load project template (new tab), slot %02d" }, "S&M_NEWTAB_PRJTEMPLATE", loadOrSelectProjectNewTab, NULL, 10},
#ifdef _SNM_PRESETS
	{ { DEFACCEL, "SWS/S&M: Trigger next preset for FX %02d of selected tracks" }, "S&M_NEXT_PRESET_FX", triggerNextPreset, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Trigger previous preset for FX %02d of selected tracks" }, "S&M_PREVIOUS_PRESET_FX", triggerPreviousPreset, NULL, 4},
#endif
	{ { DEFACCEL, "SWS/S&M: Select FX %02d for selected tracks" }, "S&M_SELFX", selectTrackFX, NULL, 8},

	{ { DEFACCEL, "SWS/S&M: Show FX chain for selected tracks, FX %02d" }, "S&M_SHOWFXCHAIN", showFXChain, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Float FX %02d for selected tracks" }, "S&M_FLOATFX", floatFX, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Unfloat FX %02d for selected tracks" }, "S&M_UNFLOATFX", unfloatFX, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Toggle float FX %02d for selected tracks" }, "S&M_TOGLFLOATFX", toggleFloatFX, NULL, 8, fakeIsToggledAction},

	{ { DEFACCEL, "SWS/S&M: Active ME - Restore displayed CC lanes, slot %02d" }, "S&M_MESETCCLANES", MESetCCLanes, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Active ME - Save displayed CC lanes, slot %02d" }, "S&M_MESAVECCLANES", MESaveCCLanes, NULL, 4},
	{ {}, LAST_COMMAND, }, // Denote end of table
};


///////////////////////////////////////////////////////////////////////////////
// S&M actions ("S&M extension" section)
///////////////////////////////////////////////////////////////////////////////

static MIDI_COMMAND_T g_SNMSection_cmdTable[] = 
{
	{ { DEFACCEL, "SWS/S&M: Apply live config 1 (MIDI CC absolute only)" }, "S&M_LIVECONFIG1", ApplyLiveConfig, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Apply live config 2 (MIDI CC absolute only)" }, "S&M_LIVECONFIG2", ApplyLiveConfig, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Apply live config 3 (MIDI CC absolute only)" }, "S&M_LIVECONFIG3", ApplyLiveConfig, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Apply live config 4 (MIDI CC absolute only)" }, "S&M_LIVECONFIG4", ApplyLiveConfig, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Apply live config 5 (MIDI CC absolute only)" }, "S&M_LIVECONFIG5", ApplyLiveConfig, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Apply live config 6 (MIDI CC absolute only)" }, "S&M_LIVECONFIG6", ApplyLiveConfig, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Apply live config 7 (MIDI CC absolute only)" }, "S&M_LIVECONFIG7", ApplyLiveConfig, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Apply live config 8 (MIDI CC absolute only)" }, "S&M_LIVECONFIG8", ApplyLiveConfig, NULL, 7},

	{ { DEFACCEL, "SWS/S&M: Select project (MIDI CC absolute only)" }, "S&M_SELECT_PROJECT", SelectProject, NULL, },

#ifdef _SNM_PRESETS
	{ { DEFACCEL, "SWS/S&M: Trigger preset for selected FX of selected tracks (MIDI CC absolute only)" }, "S&M_SELFX_PRESET", TriggerFXPreset, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Trigger preset for FX 1 of selected tracks (MIDI CC absolute only)" }, "S&M_PRESET_FX1", TriggerFXPreset, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Trigger preset for FX 2 of selected tracks (MIDI CC absolute only)" }, "S&M_PRESET_FX2", TriggerFXPreset, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Trigger preset for FX 3 of selected tracks (MIDI CC absolute only)" }, "S&M_PRESET_FX3", TriggerFXPreset, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Trigger preset for FX 4 of selected tracks (MIDI CC absolute only)" }, "S&M_PRESET_FX4", TriggerFXPreset, NULL, 3},
#endif
	{ {}, LAST_COMMAND, }, // Denote end of table
};


///////////////////////////////////////////////////////////////////////////////
// Fake action toggle states
///////////////////////////////////////////////////////////////////////////////

// store fake toogle states, indexed per cmd id (faster + lazy init)
static WDL_IntKeyedArray<bool*> g_fakeToggleStates;

bool fakeIsToggledAction(COMMAND_T* _ct) {
	return (_ct && _ct->accel.accel.cmd && *g_fakeToggleStates.Get(_ct->accel.accel.cmd, &g_bFalse));
}

void fakeToggleAction(COMMAND_T* _ct) {
	if (_ct && _ct->accel.accel.cmd) {
		g_fakeToggleStates.Insert(_ct->accel.accel.cmd, *g_fakeToggleStates.Get(_ct->accel.accel.cmd, &g_bFalse) ? &g_bFalse : &g_bTrue);
/*JFB doesn't seem required.. hum..
		RefreshToolbar(_ct->accel.accel.cmd);
*/
	}
}


///////////////////////////////////////////////////////////////////////////////
// Toolbars auto refresh option
///////////////////////////////////////////////////////////////////////////////

bool g_toolbarsAutoRefreshEnabled = false;

void EnableToolbarsAutoRefesh(COMMAND_T* _ct) {
	g_toolbarsAutoRefreshEnabled = !g_toolbarsAutoRefreshEnabled;
}

bool IsToolbarsAutoRefeshEnabled(COMMAND_T* _ct) {
	return g_toolbarsAutoRefreshEnabled;
}

// Also see SnMCSurfRun()
void RefreshToolbars() {
	// item sel. buttons
	RefreshToolbar(NamedCommandLookup("_S&M_TOOLBAR_ITEM_SEL0"));
	RefreshToolbar(NamedCommandLookup("_S&M_TOOLBAR_ITEM_SEL1"));
#ifdef _WIN32
	RefreshToolbar(NamedCommandLookup("_S&M_TOOLBAR_ITEM_SEL2"));
	RefreshToolbar(NamedCommandLookup("_S&M_TOOLBAR_ITEM_SEL3"));
#endif
	// write automation button
	RefreshToolbar(NamedCommandLookup("_S&M_TOOLBAR_WRITE_ENV"));

	// host AW's grid toolbar buttons auto refresh
	UpdateGridToolbar();
}


///////////////////////////////////////////////////////////////////////////////
// "S&M Extension" action section
///////////////////////////////////////////////////////////////////////////////

static WDL_IntKeyedArray<MIDI_COMMAND_T*> g_SNMSection_midiCmds;
static WDL_IntKeyedArray<MIDI_COMMAND_T*> g_SNMSection_toggles;
KbdCmd g_SNMSection_kbdCmds[SNM_MAX_SECTION_ACTIONS];
KbdKeyBindingInfo g_SNMSection_defKeys[SNM_MAX_SECTION_ACTIONS];
int g_SNMSection_minCmdId = 0;
int g_SNMSection_maxCmdId = 0;
/*JFB not used yet
static int g_SNMSection_CmdId_gen = 1000;
*/

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
  onAction
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
/*JFB not used: get cmd ids from the main section instead (safer?)
			ct->accel.accel.cmd = g_SNMSection_CmdId_gen++;
*/
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
	if (nbCmds > SNM_MAX_SECTION_ACTIONS)
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

void SNM_ShowActionList(COMMAND_T* _ct) {
	ShowActionList(&g_SNMSection, GetMainHwnd());
}


///////////////////////////////////////////////////////////////////////////////
// S&M core stuff
///////////////////////////////////////////////////////////////////////////////

#ifdef _SWS_MENU
static void SNM_Menuhook(const char* _menustr, HMENU _hMenu, int _flag)
{
	if (!strcmp(_menustr, "Main extensions") && !_flag) 
	{
		SWSCreateMenuFromCommandTable(g_SNM_cmdTable, _hMenu);
	}
/*
	else if (!strcmp(_menustr, "Media item context") && !_flag) {
	}
	else if (!strcmp(_menustr, "Track control panel context") && !_flag) {
	}
	else if (_flag == 1) {
	}
*/
}
#endif

// see g_SNM_dynamicCmdTable's comments
int SNMRegisterDynamicCommands(COMMAND_T* _cmds, const char* _fn)
{
	char actionName[SNM_MAX_ACTION_NAME_LEN], custId[SNM_MAX_ACTION_CUSTID_LEN];
	int i = 0;
	while(_cmds[i].id != LAST_COMMAND)
	{
		COMMAND_T* ct = &_cmds[i++];
		int nb = GetPrivateProfileInt("NbOfActions", ct->id, (int)ct->user, g_SNMiniFilename.Get());
		nb = BOUNDED(nb, 0, SNM_MAX_DYNAMIC_ACTIONS);
		for (int j=0; j < nb; j++)
		{
			_snprintf(actionName, SNM_MAX_ACTION_NAME_LEN, ct->accel.desc, j+1);
			_snprintf(custId, SNM_MAX_ACTION_CUSTID_LEN, "%s%d", ct->id, j+1);
			if (SWSRegisterCommandExt3(ct->doCommand, ct->getEnabled, 0, custId, actionName, j, _fn))
				ct->user = nb; // patch the real number of instances
			else
				return 0;
		}
	}
	return 1;
}

WDL_String g_SNMiniFilename;

void IniFileInit()
{
	// Init S&M.ini file(+ "upgrade": move old ones to the new REAPER's resource path)
	char buf[SNM_MAX_INI_SECTION], iniFn[BUFFER_SIZE];
	_snprintf(buf, BUFFER_SIZE, SNM_OLD_FORMATED_INI_FILE, GetExePath()); // old location
	_snprintf(iniFn, BUFFER_SIZE, SNM_FORMATED_INI_FILE, GetResourcePath());
	if (FileExists(buf))
		MoveFile(buf, iniFn);
	g_SNMiniFilename.Set(iniFn);
/*JFB not needed (issue 292 fixed in r504)
#ifdef _WIN32
	// issue 292: force the S&M.ini cache refresh (accessed below) by accessing a 3rd one
	GetPrivateProfileString("dummy","dummy","",buf,SNM_MAX_INI_SECTION,get_ini_file());
#endif
*/
	// S&M.ini cleanup & "auto upgrade"
	// [FXCHAIN] -> [FXChains]
	*buf = '\0';
	int sectionSz = GetPrivateProfileSection("FXCHAIN", buf, SNM_MAX_INI_SECTION, iniFn);
	WritePrivateProfileStruct("FXCHAIN", NULL, NULL, 0, iniFn); //flush section
	if (sectionSz)
		WritePrivateProfileSection("FXChains", buf, iniFn);

	// [FXCHAIN_VIEW] -> [RESOURCE_VIEW]
	*buf = '\0';
	sectionSz = GetPrivateProfileSection("FXCHAIN_VIEW", buf, SNM_MAX_INI_SECTION, iniFn);
	WritePrivateProfileStruct("FXCHAIN_VIEW", NULL, NULL, 0, iniFn); //flush section
	if (sectionSz)
		WritePrivateProfileSection("RESOURCE_VIEW", buf, iniFn);

	// Load general prefs 
	g_toolbarsAutoRefreshEnabled = (GetPrivateProfileInt("General", "ToolbarsAutoRefresh", 1, iniFn) == 1);
	g_buggyPlugSupport = GetPrivateProfileInt("General", "BuggyPlugsSupport", 0, iniFn);
}

void IniFileExit()
{
	WDL_String iniSection;

	// save general prefs & info
	iniSection.AppendFormatted(128, "; S&M.ini - SWS/S&M Extension v%d.%d.%d Build #%d\n", SWS_VERSION); 
	iniSection.AppendFormatted(BUFFER_SIZE, "; %s\n", g_SNMiniFilename.Get()); 
	iniSection.AppendFormatted(64, "ToolbarsAutoRefresh=%d\n", g_toolbarsAutoRefreshEnabled ? 1 : 0); 
	iniSection.AppendFormatted(64, "BuggyPlugsSupport=%d\n", g_buggyPlugSupport ? 1 : 0); 
	SaveIniSection("General", &iniSection, g_SNMiniFilename.Get());

	// save dynamic actions
	iniSection.SetFormatted(128, "; Set the number of slots/actions you want below (none: 0, max: %d, quit REAPER first!)\n", SNM_MAX_DYNAMIC_ACTIONS);
	int i = 0; char name[SNM_MAX_ACTION_NAME_LEN];
	while(g_SNM_dynamicCmdTable[i].id != LAST_COMMAND) {
		COMMAND_T* ct = &g_SNM_dynamicCmdTable[i++];
		strncpy(name, SNM_CMD_SHORTNAME(ct), SNM_MAX_ACTION_NAME_LEN); // strncpy: lstrcpyn() KO here
		ReplaceStringFormat(name, 'n');
		iniSection.AppendFormatted(SNM_MAX_ACTION_CUSTID_LEN+SNM_MAX_ACTION_NAME_LEN+8, "%s=%d ; %s\n", ct->id, (int)ct->user, name);
	}
	SaveIniSection("NbOfActions", &iniSection, g_SNMiniFilename.Get());
/*JFB not needed (issue 292 fixed in r504)
#ifdef _WIN32
		// issue 292: force writing of the ini file
		// http://support.microsoft.com/kb/68827
		WritePrivateProfileString(NULL, NULL, NULL, g_SNMiniFilename.Get());
#endif
*/
}

int SnMInit(reaper_plugin_info_t* _rec)
{
	if (!_rec)
		return 0;

	IniFileInit();

#ifdef _SWS_MENU
	if (!plugin_register("hookcustommenu", (void*)SNM_Menuhook))
		return 0;
#endif
	// Actions should be registered before views
	if (!SWSRegisterCommands(g_SNM_cmdTable) || 
		!SNMRegisterDynamicCommands(g_SNM_dynamicCmdTable, __FILE__) ||
		!SNMSectionRegisterCommands(_rec))
		return 0;

	SNM_UIInit();
	LiveConfigViewInit();
	ResourceViewInit();
	NotesHelpViewInit();
	FindViewInit();
	CyclactionsInit();
	return 1;
}

void SnMExit()
{
	LiveConfigViewExit();
	ResourceViewExit();
	NotesHelpViewExit();
	FindViewExit();
	SNM_UIExit(); 
	IniFileExit();
}


///////////////////////////////////////////////////////////////////////////////
// SNM_ScheduledJob (performed in SnMCSurfRun())
///////////////////////////////////////////////////////////////////////////////

WDL_PtrList_DeleteOnDestroy<SNM_ScheduledJob> g_jobs;
SWS_Mutex g_jobsLock;

void AddOrReplaceScheduledJob(SNM_ScheduledJob* _job) 
{
	if (!_job)
		return;

	SWS_SectionLock lock(&g_jobsLock);
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
	SWS_SectionLock lock(&g_jobsLock);
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

double g_approxPollMsCounter = 0.0;

void SnMCSurfRun()
{
	// Polling (SNM_CSURF_RUN_POLL_MS = every second or so)
	g_approxPollMsCounter += SNM_CSURF_RUN_TICK_MS;
	if (g_approxPollMsCounter >= SNM_CSURF_RUN_POLL_MS)
	{
		g_approxPollMsCounter = 0.0;
		if (g_toolbarsAutoRefreshEnabled) 
		{
			itemSelToolbarPoll();
			RefreshToolbars();
		}
	}

	// Perform scheduled jobs
	SWS_SectionLock lock(&g_jobsLock);
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

extern SNM_NotesHelpWnd* g_pNotesHelpWnd;
extern SNM_LiveConfigsWnd* g_pLiveConfigsWnd;

void SnMCSurfSetTrackTitle() {
	if (g_pNotesHelpWnd)
		g_pNotesHelpWnd->CSurfSetTrackTitle();
	if (g_pLiveConfigsWnd)
		g_pLiveConfigsWnd->CSurfSetTrackTitle();
}

void SnMCSurfSetTrackListChange() {
	if (g_pNotesHelpWnd)
		g_pNotesHelpWnd->CSurfSetTrackListChange();
	if (g_pLiveConfigsWnd)
		g_pLiveConfigsWnd->CSurfSetTrackListChange();
}

