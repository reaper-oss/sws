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
#include "version.h"
#include "SnM_Actions.h"
#include "SNM_FXChainView.h"
#include "SnM_NotesHelpView.h"
#include "SnM_MidiLiveView.h"
#include "../Misc/Adam.h"


void QuickTest(COMMAND_T* _ct) {}


///////////////////////////////////////////////////////////////////////////////
// S&M "static" actions (main section)
///////////////////////////////////////////////////////////////////////////////

static COMMAND_T g_SNM_cmdTable[] = 
{
	// Beware! S&M actions expect "SWS/S&M: " in their names (tag removed from undo messages)

//	{ { DEFACCEL, "SWS/S&M: QuickTest" }, "S&M_QUICKTEST", QuickTest, NULL, },

	// Routing & cue buss -----------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Create cue buss from track selection (use last settings)" }, "S&M_CUEBUS", cueTrack, NULL, -1},
#ifdef _SNM_MISC // the previous action is enough
	{ { DEFACCEL, "SWS/S&M: Create cue buss from track selection (pre-fader/post-FX)" }, "S&M_SENDS1", cueTrack, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Create cue buss from track selection (post-fader)" }, "S&M_SENDS2", cueTrack, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Create cue buss from track selection (pre-FX)" }, "S&M_SENDS3", cueTrack, NULL, 1},
#endif
	{ { DEFACCEL, "SWS/S&M: Open/close Cue Buss generator" }, "S&M_SENDS4", openCueBussWnd, "S&&M Cue Buss generator", NULL, isCueBussWndDisplayed},

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
	{ { DEFACCEL, "SWS/S&M: Close all floating FX windows" }, "S&M_WNCLS3", closeAllFXWindows, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Close all FX chain windows" }, "S&M_WNCLS4", closeAllFXChainsWindows, NULL, },
	{ { DEFACCEL, "SWS/S&M: Close all floating FX windows for selected tracks" }, "S&M_WNCLS5", closeAllFXWindows, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Close all floating FX windows, except focused one" }, "S&M_WNCLS6", closeAllFXWindowsExceptFocused, NULL, },
	{ { DEFACCEL, "SWS/S&M: Show all floating FX windows (!)" }, "S&M_WNTSHW1", showAllFXWindows, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Show all FX chain windows (!)" }, "S&M_WNTSHW2", showAllFXChainsWindows, NULL, },
	{ { DEFACCEL, "SWS/S&M: Show all floating FX windows for selected tracks" }, "S&M_WNTSHW3", showAllFXWindows, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Toggle show all floating FX (!)" }, "S&M_WNTGL3", toggleAllFXWindows, NULL, 0, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle show all FX chain windows (!)" }, "S&M_WNTGL4", toggleAllFXChainsWindows, NULL, -666, FakeIsToggleAction},	
	{ { DEFACCEL, "SWS/S&M: Toggle show all floating FX for selected tracks" }, "S&M_WNTGL5", toggleAllFXWindows, NULL, 1, FakeIsToggleAction},

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
	{ { DEFACCEL, "SWS/S&M: Toggle float selected FX for selected tracks" }, "S&M_TOGLFLOATFXEL", toggleFloatFX, NULL, -1, FakeIsToggleAction},

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

	{ { DEFACCEL, "SWS/S&M: Toggle all FX online/offline for selected tracks" }, "S&M_FXOFFALL", toggleAllFXsOfflineSelectedTracks, NULL, -666, FakeIsToggleAction},

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
	
	{ { DEFACCEL, "SWS/S&M: Toggle all FX bypass for selected tracks" }, "S&M_FXBYPALL", toggleAllFXsBypassSelectedTracks, NULL, -666, FakeIsToggleAction},

	{ { DEFACCEL, "SWS/S&M: Bypass all FX for selected tracks" }, "S&M_FXBYPALL2", setAllFXsBypassSelectedTracks, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Unbypass all FX for selected tracks" }, "S&M_FXBYPALL3", setAllFXsBypassSelectedTracks, NULL, 0},
	// ..equivalent online/offline actions exist natively

	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except selected) online/offline for selected tracks" }, "S&M_FXOFFEXCPTSEL", toggleExceptFXOfflineSelectedTracks, NULL, 0, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except selected) bypass for selected tracks" }, "S&M_FXBYPEXCPTSEL", toggleExceptFXBypassSelectedTracks, NULL, 0, FakeIsToggleAction},
#ifdef _SNM_MISC // very specific..
	//JFB TODO: release as "hidden" dynamic actions (but ct->user needs to be 0-based first)
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 1) online/offline for selected tracks" }, "S&M_FXOFFEXCPT1", toggleExceptFXOfflineSelectedTracks, NULL, 1, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 2) online/offline for selected tracks" }, "S&M_FXOFFEXCPT2", toggleExceptFXOfflineSelectedTracks, NULL, 2, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 3) online/offline for selected tracks" }, "S&M_FXOFFEXCPT3", toggleExceptFXOfflineSelectedTracks, NULL, 3, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 4) online/offline for selected tracks" }, "S&M_FXOFFEXCPT4", toggleExceptFXOfflineSelectedTracks, NULL, 4, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 5) online/offline for selected tracks" }, "S&M_FXOFFEXCPT5", toggleExceptFXOfflineSelectedTracks, NULL, 5, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 6) online/offline for selected tracks" }, "S&M_FXOFFEXCPT6", toggleExceptFXOfflineSelectedTracks, NULL, 6, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 7) online/offline for selected tracks" }, "S&M_FXOFFEXCPT7", toggleExceptFXOfflineSelectedTracks, NULL, 7, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 8) online/offline for selected tracks" }, "S&M_FXOFFEXCPT8", toggleExceptFXOfflineSelectedTracks, NULL, 8, FakeIsToggleAction},

	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 1) bypass for selected tracks" }, "S&M_FXBYPEXCPT1", toggleExceptFXBypassSelectedTracks, NULL, 1, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 2) bypass for selected tracks" }, "S&M_FXBYPEXCPT2", toggleExceptFXBypassSelectedTracks, NULL, 2, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 3) bypass for selected tracks" }, "S&M_FXBYPEXCPT3", toggleExceptFXBypassSelectedTracks, NULL, 3, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 4) bypass for selected tracks" }, "S&M_FXBYPEXCPT4", toggleExceptFXBypassSelectedTracks, NULL, 4, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 5) bypass for selected tracks" }, "S&M_FXBYPEXCPT5", toggleExceptFXBypassSelectedTracks, NULL, 5, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 6) bypass for selected tracks" }, "S&M_FXBYPEXCPT6", toggleExceptFXBypassSelectedTracks, NULL, 6, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 7) bypass for selected tracks" }, "S&M_FXBYPEXCPT7", toggleExceptFXBypassSelectedTracks, NULL, 7, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except 8) bypass for selected tracks" }, "S&M_FXBYPEXCPT8", toggleExceptFXBypassSelectedTracks, NULL, 8, FakeIsToggleAction},
#endif

	// Take FX online/offline & bypass/unbypass ------------------------------
	{ { DEFACCEL, "SWS/S&M: Toggle all take FX online/offline for selected items" }, "S&M_TGL_TAKEFX_ONLINE", toggleAllFXsOfflineSelectedItems, NULL, -666, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Set all take FX offline for selected items" }, "S&M_TAKEFX_OFFLINE", setAllFXsOfflineSelectedItems, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Set all take FX online for selected items" }, "S&M_TAKEFX_ONLINE", setAllFXsOfflineSelectedItems, NULL, 0},

	{ { DEFACCEL, "SWS/S&M: Toggle all take FX bypass for selected items" }, "S&M_TGL_TAKEFX_BYP", toggleAllFXsBypassSelectedItems, NULL, -666, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Bypass all take FX for selected items" }, "S&M_TAKEFX_BYPASS", setAllFXsBypassSelectedItems, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Unbypass all take FX for selected items" }, "S&M_TAKEFX_UNBYPASS", setAllFXsBypassSelectedItems, NULL, 0},

	// Resources view
	{ { DEFACCEL, "SWS/S&M: Open/close Resources window" }, "S&M_SHOW_RESOURCES_VIEW", OpenResourceView, "S&&M Resources", -1, IsResourceViewDisplayed},

	// FX Chains (items & tracks) ---------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Open/close Resources window (FX chains)" }, "S&M_SHOWFXCHAINSLOTS", OpenResourceView, "S&&M Resources", SNM_SLOT_FXC, IsResourceViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Clear FX chain slot..." }, "S&M_CLRFXCHAINSLOT", ResViewClearSlotPrompt, NULL, SNM_SLOT_FXC},
	{ { DEFACCEL, "SWS/S&M: Delete all FX chain slots" }, "S&M_DEL_ALL_FXCHAINSLOT", ResViewDeleteAllSlots, NULL, SNM_SLOT_FXC},
	{ { DEFACCEL, "SWS/S&M: Auto-save FX Chain slots for selected tracks" }, "S&M_SAVE_FXCHAIN_SLOT1", ResViewAutoSaveFXChain, NULL, FXC_AUTOSAVE_PREF_TRACK},
	{ { DEFACCEL, "SWS/S&M: Auto-save FX Chain slots for selected items" }, "S&M_SAVE_FXCHAIN_SLOT2", ResViewAutoSaveFXChain, NULL, FXC_AUTOSAVE_PREF_ITEM},
	{ { DEFACCEL, "SWS/S&M: Auto-save input FX Chain slots for selected tracks" }, "S&M_SAVE_FXCHAIN_SLOT3", ResViewAutoSaveFXChain, NULL, FXC_AUTOSAVE_PREF_INPUT_FX},
	{ { DEFACCEL, "SWS/S&M: Paste (replace) FX chain to selected items, prompt for slot" }, "S&M_TAKEFXCHAINp1", loadSetTakeFXChain, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Paste (replace) FX chain to selected items, all takes, prompt for slot" }, "S&M_TAKEFXCHAINp2", loadSetAllTakesFXChain, NULL, -1},
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

	{ { DEFACCEL, "SWS/S&M: Paste (replace) FX chain to selected tracks, prompt for slot" }, "S&M_TRACKFXCHAINp1", loadSetTrackFXChain, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Paste FX chain to selected tracks, prompt for slot" }, "S&M_PASTE_TRACKFXCHAINp1", loadPasteTrackFXChain, NULL, -1},

	// Track templates --------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Open/close Resources window (track templates)" }, "S&M_SHOW_RESVIEW_TR_TEMPLATES", OpenResourceView, NULL, SNM_SLOT_TR, IsResourceViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Clear track template slot..." }, "S&M_CLR_TRTEMPLATE_SLOT", ResViewClearSlotPrompt, NULL, SNM_SLOT_TR},
	{ { DEFACCEL, "SWS/S&M: Delete all track template slots" }, "S&M_DEL_ALL_TRTEMPLATE_SLOT", ResViewDeleteAllSlots, NULL, SNM_SLOT_TR},
	{ { DEFACCEL, "SWS/S&M: Auto-save track template slots" }, "S&M_SAVE_TRTEMPLATE_SLOT1", ResViewAutoSaveTrTemplate, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Auto-save track template slots (with items, envelopes) " }, "S&M_SAVE_TRTEMPLATE_SLOT2", ResViewAutoSaveTrTemplate, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Apply track template to selected tracks, prompt for slot" }, "S&M_APPLY_TRTEMPLATEp", LoadApplyTrackTemplateSlot, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Apply track template (+envelopes/items) to selected tracks, prompt for slot" }, "S&M_APPLY_TRTEMPLATE_ITEMSENVSp", LoadApplyTrackTemplateSlotWithItemsEnvs, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Import tracks from track template, prompt for slot" }, "S&M_ADD_TRTEMPLATEp", LoadImportTrackTemplateSlot, NULL, -1},

	// Projects & project templates -------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Open/close Resources window (project templates)" }, "S&M_SHOW_RESVIEW_PRJ_TEMPLATES", OpenResourceView, NULL, SNM_SLOT_PRJ, IsResourceViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Clear project template slot..." }, "S&M_CLR_PRJTEMPLATE_SLOT", ResViewClearSlotPrompt, NULL, SNM_SLOT_PRJ},
	{ { DEFACCEL, "SWS/S&M: Delete all project template slots" }, "S&M_DEL_ALL_PRJTEMPLATE_SLOT", ResViewDeleteAllSlots, NULL, SNM_SLOT_PRJ},
	{ { DEFACCEL, "SWS/S&M: Auto-save project template slot" }, "S&M_SAVE_PRJTEMPLATE_SLOT", ResViewAutoSave, NULL, SNM_SLOT_PRJ},
	{ { DEFACCEL, "SWS/S&M: Open/select project template, prompt for slot" }, "S&M_APPLY_PRJTEMPLATEp", loadOrSelectProjectSlot, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Open/select project template (new tab), prompt for slot" }, "S&M_NEWTAB_PRJTEMPLATEp", loadOrSelectProjectTabSlot, NULL, -1},

	{ { DEFACCEL, "SWS/S&M: Project loader/selecter: configuration" }, "S&M_PRJ_LOADER_CONF", projectLoaderConf, NULL, },
	{ { DEFACCEL, "SWS/S&M: Project loader/selecter: next (cycle)" }, "S&M_PRJ_LOADER_NEXT", loadOrSelectNextPreviousProject, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Project loader/selecter: previous (cycle)" }, "S&M_PRJ_LOADER_PREV", loadOrSelectNextPreviousProject, NULL, -1},

	{ { DEFACCEL, "SWS/S&M: Open project path in explorer/finder" }, "S&M_OPEN_PRJ_PATH", openProjectPathInExplorerFinder, NULL, },
	
	// Media file slots -------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Open/close Resources window (media files)" }, "S&M_SHOW_RESVIEW_MEDIA", OpenResourceView, "S&&M Resources", SNM_SLOT_MEDIA, IsResourceViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Clear media file slot..." }, "S&M_CLR_MEDIA_SLOT", ResViewClearSlotPrompt, NULL, SNM_SLOT_MEDIA},
	{ { DEFACCEL, "SWS/S&M: Delete all media file slots" }, "S&M_DEL_ALL_MEDIA_SLOT", ResViewDeleteAllSlots, NULL, SNM_SLOT_MEDIA},
	{ { DEFACCEL, "SWS/S&M: Auto-save media file slots for selected items" }, "S&M_SAVE_MEDIA_SLOT", ResViewAutoSave, NULL, SNM_SLOT_MEDIA},
	{ { DEFACCEL, "SWS/S&M: Play media file in selected tracks, prompt for slot" }, "S&M_PLAYMEDIA_SELTRACKp", PlaySelTrackMediaSlot, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Loop media file in selected tracks, prompt for slot" }, "S&M_LOOPMEDIA_SELTRACKp", LoopSelTrackMediaSlot, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Play media file in selected tracks (toggle), prompt for slot" }, "S&M_TGL_PLAYMEDIA_SELTRACKp", TogglePlaySelTrackMediaSlot, NULL, -1, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Loop media file in selected tracks (toggle), prompt for slot" }, "S&M_TGL_LOOPMEDIA_SELTRACKp", ToggleLoopSelTrackMediaSlot, NULL, -1, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Play media file in selected tracks (toggle pause), prompt for slot" }, "S&M_TGLPAUSE_PLAYMEDIA_SELTRACKp", TogglePauseSelTrackMediaSlot, NULL, -1, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Loop media file in selected tracks (toggle pause), prompt for slot - Infinite looping! To be stopped!" }, "S&M_TGLPAUSE_LOOPMEDIA_SELTRACKp", ToggleLoopPauseSelTrackMediaSlot, NULL, -1, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Stop all playing media files" }, "S&M_STOPMEDIA_ALLTRACK", StopTrackPreviews, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Stop all playing media files in selected tracks" }, "S&M_STOPMEDIA_SELTRACK", StopTrackPreviews, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Add media file to current track, prompt for slot" }, "S&M_ADDMEDIA_CURTRACKp", InsertMediaSlotCurTr, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Add media file to new track, prompt for slot" }, "S&M_ADDMEDIA_NEWTRACKp", InsertMediaSlotNewTr, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Add media file to selected items as takes, prompt for slot" }, "S&M_ADDMEDIA_SELITEMp", InsertMediaSlotTakes, NULL, -1},

	// Image slots ------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Open/close Resources window (images)" }, "S&M_SHOW_RESVIEW_IMAGE", OpenResourceView, "S&&M Resources", SNM_SLOT_IMG, IsResourceViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Clear image slot..." }, "S&M_CLR_IMAGE_SLOT", ResViewClearSlotPrompt, NULL, SNM_SLOT_IMG},
	{ { DEFACCEL, "SWS/S&M: Delete all image slots" }, "S&M_DEL_ALL_IMAGE_SLOT", ResViewDeleteAllSlots, NULL, SNM_SLOT_IMG},
	{ { DEFACCEL, "SWS/S&M: Show image, prompt for slot" }, "S&M_SHOW_IMAGEp", ShowImageSlot, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Set track icon for selected tracks, prompt for slot" }, "S&M_SET_TRACK_ICONp", SetSelTrackIconSlot, NULL, -1},

	// Theme slots ------------------------------------------------------------
#ifdef _WIN32
	{ { DEFACCEL, "SWS/S&M: Open/close Resources window (themes)" }, "S&M_SHOW_RESVIEW_THEME", OpenResourceView, "S&&M Resources", SNM_SLOT_THM, IsResourceViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Clear theme slot..." }, "S&M_CLR_THEME_SLOT", ResViewClearSlotPrompt, NULL, SNM_SLOT_THM},
	{ { DEFACCEL, "SWS/S&M: Delete all theme slots" }, "S&M_DEL_ALL_THEME_SLOT", ResViewDeleteAllSlots, NULL, SNM_SLOT_THM},
	{ { DEFACCEL, "SWS/S&M: Load theme, prompt for slot" }, "S&M_LOAD_THEMEp", LoadThemeSlot, NULL, -1},
#endif

	// FX presets -------------------------------------------------------------
#ifdef _SNM_PRESETS
	{ { DEFACCEL, "SWS/S&M: Trigger next preset for selected FX of selected tracks" }, "S&M_NEXT_SELFX_PRESET", triggerNextPreset, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Trigger previous preset for selected FX of selected tracks" }, "S&M_PREVIOUS_SELFX_PRESET", triggerPreviousPreset, NULL, -1},
#endif
	
	// MIDI learn -------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Reassign MIDI learned channels of all FX for selected tracks (prompt)" }, "S&M_ALL_FX_LEARN_CHp", reassignLearntMIDICh, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Reassign MIDI learned channels of selected FX for selected tracks (prompt)" }, "S&M_SEL_FX_LEARN_CHp", reassignLearntMIDICh, NULL, -2},
	{ { DEFACCEL, "SWS/S&M: Reassign MIDI learned channels of all FX to input channel for selected tracks" }, "S&M_ALLIN_FX_LEARN_CHp", reassignLearntMIDICh, NULL, -3},
	
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
	{ { DEFACCEL, "SWS/S&M: Takes - Activate lanes from selected items" }, "S&M_LANETAKE2", activateLaneFromSelItem, NULL, },
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

	// Notes/Subs/Help --------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Open/close Notes window" }, "S&M_SHOW_NOTES_VIEW", OpenNotesHelpView, NULL, -1, IsNotesHelpViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Open/close Notes window (project notes)" }, "S&M_SHOWNOTESHELP", OpenNotesHelpView, NULL, SNM_NOTES_PROJECT, IsNotesHelpViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Open/close Notes window (item notes)" }, "S&M_ITEMNOTES", OpenNotesHelpView, NULL, SNM_NOTES_ITEM, IsNotesHelpViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Open/close Notes window (track notes)" }, "S&M_TRACKNOTES", OpenNotesHelpView, NULL, SNM_NOTES_TRACK, IsNotesHelpViewDisplayed},
#ifdef _SNM_MARKER_REGION_NAME
	{ { DEFACCEL, "SWS/S&M: Open/close Notes window (marker/region names)" }, "S&M_MARKERNAMES", OpenNotesHelpView, NULL, SNM_NOTES_REGION_NAME, IsNotesHelpViewDisplayed},
#endif
	{ { DEFACCEL, "SWS/S&M: Open/close Notes window (marker/region subtitles)" }, "S&M_MARKERSUBTITLES", OpenNotesHelpView, NULL, SNM_NOTES_REGION_SUBTITLES, IsNotesHelpViewDisplayed},
#ifdef _WIN32
	{ { DEFACCEL, "SWS/S&M: Open/close Notes window (action help)" }, "S&M_ACTIONHELP", OpenNotesHelpView, NULL, SNM_NOTES_ACTION_HELP, IsNotesHelpViewDisplayed},
#endif
	{ { DEFACCEL, "SWS/S&M: Notes - Toggle lock" }, "S&M_ACTIONHELPTGLOCK", ToggleNotesHelpLock, NULL, NULL, IsNotesHelpLocked},
	{ { DEFACCEL, "SWS/S&M: Notes - Set action help file..." }, "S&M_ACTIONHELPPATH", SetActionHelpFilename, NULL, },
	{ { DEFACCEL, "SWS/S&M: Notes - Import subtitle file..." }, "S&M_IMPORT_SUBTITLE", ImportSubTitleFile, NULL, },
	{ { DEFACCEL, "SWS/S&M: Notes - Export subtitle file..." }, "S&M_EXPORT_SUBTITLE", ExportSubTitleFile, NULL, },

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
	{ { DEFACCEL, "SWS/S&M: Remove track grouping for selected tracks" }, "S&M_REMOVE_TR_GRP", removeTrackGrouping, NULL, },
	{ { DEFACCEL, "SWS/S&M: Set selected tracks to first unused group (default flags)" }, "S&M_SET_TRACK_UNUSEDGROUP", SetTrackToFirstUnusedGroup, NULL, },

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
	{ { DEFACCEL, "SWS/S&M: Toggle show take volume envelope" }, "S&M_TAKEENV7", showHideTakeVolEnvelope, NULL, -1, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle show take pan envelope" }, "S&M_TAKEENV8", showHideTakePanEnvelope, NULL, -1, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle show take mute envelope" }, "S&M_TAKEENV9", showHideTakeMuteEnvelope, NULL, -1, FakeIsToggleAction},
#endif
	{ { DEFACCEL, "SWS/S&M: Show take pitch envelope" }, "S&M_TAKEENV10", showHideTakePitchEnvelope, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Hide take pitch envelope" }, "S&M_TAKEENV11", showHideTakePitchEnvelope, NULL, 0},

	// Track envelopes --------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Toggle arming of all active envelopes for selected tracks" }, "S&M_TGLARMALLENVS", toggleArmTrackEnv, NULL, 0, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Arm all active envelopes for selected tracks" }, "S&M_ARMALLENVS", toggleArmTrackEnv, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Disarm all active envelopes for selected tracks" }, "S&M_DISARMALLENVS", toggleArmTrackEnv, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Toggle arming of volume envelope for selected tracks" }, "S&M_TGLARMVOLENV", toggleArmTrackEnv, NULL, 3, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle arming of pan envelope for selected tracks" }, "S&M_TGLARMPANENV", toggleArmTrackEnv, NULL, 4, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle arming of mute envelope for selected tracks" }, "S&M_TGLARMMUTEENV", toggleArmTrackEnv, NULL, 5, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle arming of all receive volume envelopes for selected tracks" }, "S&M_TGLARMAUXVOLENV", toggleArmTrackEnv, NULL, 6, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle arming of all receive pan envelopes for selected tracks" }, "S&M_TGLARMAUXPANENV", toggleArmTrackEnv, NULL, 7, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle arming of all receive mute envelopes for selected tracks" }, "S&M_TGLARMAUXMUTEENV", toggleArmTrackEnv, NULL, 8, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle arming of all plugin envelopes for selected tracks" }, "S&M_TGLARMPLUGENV", toggleArmTrackEnv, NULL, 9, FakeIsToggleAction},

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
	{ { DEFACCEL, "SWS/S&M: Open/close Live Configs window" }, "S&M_SHOWMIDILIVE", OpenLiveConfigView, "S&&M Live Configs", NULL, IsLiveConfigViewDisplayed},

	// Cyclactions ---------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Open/close Cycle Action editor" }, "S&M_CYCLEDITOR", OpenCyclactionView, "S&&M Cycle Action editor", 0, IsCyclactionViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Open/close Cycle Action editor (event list)" }, "S&M_CYCLEDITOR_ME_LIST", OpenCyclactionView, "S&&M Cycle Action editor", 1, IsCyclactionViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Open/close Cycle Action editor (piano roll)" }, "S&M_CYCLEDITOR_ME_PIANO", OpenCyclactionView, "S&&M Cycle Action editor", 2, IsCyclactionViewDisplayed},

	// REC inputs -------------------------------------------------------------
	//JFB TODO: configurable dynamic actions *with max* (but ct->user needs to be 0-based first)
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

	// Localization
	{ { DEFACCEL, "SWS/S&M: LangPack file - Load..." }, "S&M_LOAD_LANGPACK", LoadAssignLangPack, NULL, },
	{ { DEFACCEL, "SWS/S&M: LangPack file - Generate..." }, "S&M_GEN_LANGPACK", GenerateLangPack, NULL, },
	{ { DEFACCEL, "SWS/S&M: LangPack file - Upgrade" }, "S&M_UPGRADE_LANGPACK", UpgradeLangPack, NULL, },
	{ { DEFACCEL, "SWS/S&M: LangPack file - Reset to factory settings" }, "S&M_RESET_LANGPACK", ResetLangPack, NULL, },

	// Other, misc ------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Open/close image window" }, "S&M_OPEN_IMAGEVIEW", OpenImageView, NULL, },
	{ { DEFACCEL, "SWS/S&M: Clear image window" }, "S&M_CLR_IMAGEVIEW", ClearImageView, NULL, },
	{ { DEFACCEL, "SWS/S&M: Send all notes off to selected tracks" }, "S&M_CC123_SEL_TRACKS", CC123SelTracks, NULL, },
	{ { DEFACCEL, "SWS/S&M: Show action list (S&M Extension section)" }, "S&M_ACTION_LIST", SNM_ShowActionList, NULL, },
#ifdef _WIN32
	{ { DEFACCEL, "SWS/S&M: Show theme helper (all tracks)" }, "S&M_THEME_HELPER_ALL", ShowThemeHelper, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Show theme helper (selected track)" }, "S&M_THEME_HELPER_SEL", ShowThemeHelper, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Left mouse click at cursor position (use w/o modifier)" }, "S&M_MOUSE_L_CLICK", SimulateMouseClick, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Dump ALR Wiki summary (w/o SWS extension)" }, "S&M_ALRSUMMARY1", DumpWikiActionList, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Dump ALR Wiki summary (SWS extension only)" }, "S&M_ALRSUMMARY2", DumpWikiActionList, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Dump action list (w/o SWS extension)" }, "S&M_DUMP_ACTION_LIST", DumpActionList, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Dump action list (SWS extension only)" }, "S&M_DUMP_SWS_ACTION_LIST", DumpActionList, NULL, 4},
#endif
#ifdef _SWS_MENU
	{ { DEFACCEL, NULL }, NULL, NULL, SWS_SEPARATOR, }, // for main "Extensions" menu
#endif
	{ {}, LAST_COMMAND, }, // Denote end of table
};


///////////////////////////////////////////////////////////////////////////////
// S&M "dynamic" actions (main section)
//
// "dynamic" means that the number of instances of each action can be
// customized in the S&M.ini file (section "NbOfActions"). 
// In the folowing table:
// - items are not real commands but "meta" commands, this table must be 
//   registered with SNMRegisterDynamicCommands()
// - COMMAND_T.user is used to specify the default number of actions to create
// - COMMAND_T.menuText is used to specify a custom max number of actions 
//   (NULL means max = 99)
// - a function doCommand(COMMAND_T*) or getEnabled(COMMAND_T*) will be 
//   trigered with 0-based COMMAND_T.user
// - action names are formated strings, they must contain "%02d" (better sort 
//   in the action list, 2 digits for max = 99)
// - custom command ids aren't formated strings, but final ids will end with 
//   slot numbers (1-based for display reasons)
//
// example: 
// { { DEFACCEL, "Do stuff %02d" }, "DO_STUFF", doStuff, NULL, 2}
// if not overrided in the S&M.ini file (e.g. "DO_STUFF=32"), 2 actions will 
// be created: "Do stuff 01" and "Do stuff 02" both calling doStuff(c) with 
// c->user=0 and c->user=1, respectively. 
// custom ids will be "_DO_STUFF1" and "_DO_STUFF2", repectively.
//
///////////////////////////////////////////////////////////////////////////////

static COMMAND_T g_SNM_dynamicCmdTable[] =
{
	{ { DEFACCEL, "SWS/S&M: Clear FX chain slot %02d" }, "S&M_CLRFXCHAINSLOT", ResViewClearFXChainSlot, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Paste (replace) FX chain to selected items, slot %02d" }, "S&M_TAKEFXCHAIN", loadSetTakeFXChain, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Paste FX chain to selected items, slot %02d" }, "S&M_PASTE_TAKEFXCHAIN", loadPasteTakeFXChain, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Paste (replace) FX chain to selected items, all takes, slot %02d" }, "S&M_FXCHAIN_ALLTAKES", loadSetAllTakesFXChain, NULL, 0}, // default: none
	{ { DEFACCEL, "SWS/S&M: Paste FX chain to selected items, all takes, slot %02d" }, "S&M_PASTE_FXCHAIN_ALLTAKES", loadPasteAllTakesFXChain, NULL, 0}, // default: none
	{ { DEFACCEL, "SWS/S&M: Paste (replace) FX chain to selected tracks, slot %02d" }, "S&M_TRACKFXCHAIN", loadSetTrackFXChain, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Paste FX chain to selected tracks, slot %02d" }, "S&M_PASTE_TRACKFXCHAIN", loadPasteTrackFXChain, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Paste (replace) input FX chain to selected tracks, slot %02d" }, "S&M_INFXCHAIN", loadSetTrackInFXChain, NULL, 0}, // default: none
	{ { DEFACCEL, "SWS/S&M: Paste input FX chain to selected tracks, slot %02d" }, "S&M_PASTE_INFXCHAIN", loadPasteTrackInFXChain, NULL, 0}, // default: none

	{ { DEFACCEL, "SWS/S&M: Clear track template slot %02d" }, "S&M_CLR_TRTEMPLATE_SLOT", ResViewClearTrTemplateSlot, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Apply track template to selected tracks, slot %02d" }, "S&M_APPLY_TRTEMPLATE", LoadApplyTrackTemplateSlot, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Apply track template (+envelopes/items) to selected tracks, slot %02d" }, "S&M_APPLY_TRTEMPLATE_ITEMSENVS", LoadApplyTrackTemplateSlotWithItemsEnvs, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Import tracks from track template, slot %02d" }, "S&M_ADD_TRTEMPLATE", LoadImportTrackTemplateSlot, NULL, 4},

	{ { DEFACCEL, "SWS/S&M: Clear project template slot %02d" }, "S&M_CLR_PRJTEMPLATE_SLOT", ResViewClearPrjTemplateSlot, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Open/select project template, slot %02d" }, "S&M_APPLY_PRJTEMPLATE", loadOrSelectProjectSlot, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Open/select project template (new tab), slot %02d" }, "S&M_NEWTAB_PRJTEMPLATE", loadOrSelectProjectTabSlot, NULL, 4},

	{ { DEFACCEL, "SWS/S&M: Clear media file slot %02d" }, "S&M_CLR_MEDIA_SLOT", ResViewClearMediaSlot, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Play media file in selected tracks, slot %02d" }, "S&M_PLAYMEDIA_SELTRACK", PlaySelTrackMediaSlot, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Loop media file in selected tracks, slot %02d" }, "S&M_LOOPMEDIA_SELTRACK", LoopSelTrackMediaSlot, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Play media file in selected tracks (sync with next measure), slot %02d" }, "S&M_PLAYMEDIA_SELTRACK_SYNC", SyncPlaySelTrackMediaSlot, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Loop media file in selected tracks (sync with next measure), slot %02d" }, "S&M_LOOPMEDIA_SELTRACK_SYNC", SyncLoopSelTrackMediaSlot, NULL, 4},

	{ { DEFACCEL, "SWS/S&M: Play media file in selected tracks (toggle), slot %02d" }, "S&M_TGL_PLAYMEDIA_SELTRACK", TogglePlaySelTrackMediaSlot, NULL, 4, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Loop media file in selected tracks (toggle), slot %02d" }, "S&M_TGL_LOOPMEDIA_SELTRACK", ToggleLoopSelTrackMediaSlot, NULL, 4, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Play media file in selected tracks (toggle pause), slot %02d" }, "S&M_TGLPAUSE_PLAYMEDIA_SELTR", TogglePauseSelTrackMediaSlot, NULL, 4, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Loop media file in selected tracks (toggle pause), slot %02d - Infinite looping! To be stopped!" }, "S&M_TGLPAUSE_LOOPMEDIA_SELTR", ToggleLoopPauseSelTrackMediaSlot, NULL, 0, FakeIsToggleAction},

#ifdef _SNM_MISC // not sure it is interesting..
	{ { DEFACCEL, "SWS/S&M: Play media file in selected tracks (toggle, sync with next measure), slot %02d" }, "S&M_TGL_PLAYMEDIA_SELTRACK_SYNC", SyncTogglePlaySelTrackMediaSlot, NULL, 4, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Loop media file in selected tracks (toggle, sync with next measure), slot %02d" }, "S&M_TGL_LOOPMEDIA_SELTRACK_SYNC", SyncToggleLoopSelTrackMediaSlot, NULL, 4, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Play media file in selected tracks (toggle pause, sync with next measure), slot %02d" }, "S&M_TGLPAUSE_PLAYMEDIA_SELTR_SYNC", SyncTogglePauseSelTrackMediaSlot, NULL, 4, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Loop media file in selected tracks (toggle pause, sync with next measure), slot %02d - Infinite looping! To be stopped!" }, "S&M_TGLPAUSE_LOOPMEDIA_SELTR_SYNC", SyncToggleLoopPauseSelTrackMediaSlot, NULL, 0, FakeIsToggleAction},
#endif
	{ { DEFACCEL, "SWS/S&M: Add media file to current track, slot %02d" }, "S&M_ADDMEDIA_CURTRACK", InsertMediaSlotCurTr, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Add media file to new track, slot %02d" }, "S&M_ADDMEDIA_NEWTRACK", InsertMediaSlotNewTr, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Add media file to selected items as takes, slot %02d" }, "S&M_ADDMEDIA_SELITEM", InsertMediaSlotTakes, NULL, 4},

	{ { DEFACCEL, "SWS/S&M: Clear image slot %02d" }, "S&M_CLR_IMAGE_SLOT", ResViewClearImageSlot, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Show image, slot %02d" }, "S&M_SHOW_IMG", ShowImageSlot, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Set track icon for selected tracks, slot %02d" }, "S&M_SET_TRACK_ICON", SetSelTrackIconSlot, NULL, 4},

#ifdef _WIN32
	{ { DEFACCEL, "SWS/S&M: Clear theme slot %02d" }, "S&M_CLR_THEME_SLOT", ResViewClearThemeSlot, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Load theme, slot %02d" }, "S&M_LOAD_THEME", LoadThemeSlot, NULL, 4},
#endif

#ifdef _SNM_PRESETS
	{ { DEFACCEL, "SWS/S&M: Trigger next preset for FX %02d of selected tracks" }, "S&M_NEXT_PRESET_FX", triggerNextPreset, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Trigger previous preset for FX %02d of selected tracks" }, "S&M_PREVIOUS_PRESET_FX", triggerPreviousPreset, NULL, 4},
#endif
	{ { DEFACCEL, "SWS/S&M: Select FX %02d for selected tracks" }, "S&M_SELFX", selectTrackFX, NULL, 8},

	{ { DEFACCEL, "SWS/S&M: Show FX chain for selected tracks, FX %02d" }, "S&M_SHOWFXCHAIN", showFXChain, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Float FX %02d for selected tracks" }, "S&M_FLOATFX", floatFX, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Unfloat FX %02d for selected tracks" }, "S&M_UNFLOATFX", unfloatFX, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Toggle float FX %02d for selected tracks" }, "S&M_TOGLFLOATFX", toggleFloatFX, NULL, 8, FakeIsToggleAction},

	{ { DEFACCEL, "SWS/S&M: Active MIDI Editor - Restore displayed CC lanes, slot %02d" }, "S&M_MESETCCLANES", MESetCCLanes, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Active MIDI Editor - Save displayed CC lanes, slot %02d" }, "S&M_MESAVECCLANES", MESaveCCLanes, NULL, 4},

	{ { DEFACCEL, "SWS/S&M: Set selected tracks to group %02d (default flags)" }, "S&M_SET_TRACK_GROUP", SetTrackGroup, SNM_MAX_TRACK_GROUPS_STR, 8},

	{ { DEFACCEL, "SWS/S&M: Toggle Live Config %02d enable" }, "S&M_TOGGLE_LIVE_CFG", ToggleEnableLiveConfig, SNM_LIVECFG_NB_CONFIGS_STR, 8, IsLiveConfigEnabled},
	{ { DEFACCEL, "SWS/S&M: Live Config %02d - Next" }, "S&M_NEXT_LIVE_CFG", NextLiveConfig, SNM_LIVECFG_NB_CONFIGS_STR, 8},
	{ { DEFACCEL, "SWS/S&M: Live Config %02d - Previous" }, "S&M_PREVIOUS_LIVE_CFG", PreviousLiveConfig, SNM_LIVECFG_NB_CONFIGS_STR, 8},

	{ {}, LAST_COMMAND, }, // denote end of table
};


///////////////////////////////////////////////////////////////////////////////
// "S&M extension" section
///////////////////////////////////////////////////////////////////////////////

/*static*/ MIDI_COMMAND_T g_SNMSection_cmdTable[] = 
{
	{ { DEFACCEL, "SWS/S&M: Apply Live Config 1 (MIDI CC absolute only)" }, "S&M_LIVECONFIG1", ApplyLiveConfig, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Apply Live Config 2 (MIDI CC absolute only)" }, "S&M_LIVECONFIG2", ApplyLiveConfig, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Apply Live Config 3 (MIDI CC absolute only)" }, "S&M_LIVECONFIG3", ApplyLiveConfig, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Apply Live Config 4 (MIDI CC absolute only)" }, "S&M_LIVECONFIG4", ApplyLiveConfig, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Apply Live Config 5 (MIDI CC absolute only)" }, "S&M_LIVECONFIG5", ApplyLiveConfig, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Apply Live Config 6 (MIDI CC absolute only)" }, "S&M_LIVECONFIG6", ApplyLiveConfig, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Apply Live Config 7 (MIDI CC absolute only)" }, "S&M_LIVECONFIG7", ApplyLiveConfig, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Apply Live Config 8 (MIDI CC absolute only)" }, "S&M_LIVECONFIG8", ApplyLiveConfig, NULL, 7},

	{ { DEFACCEL, "SWS/S&M: Select project (MIDI CC absolute only)" }, "S&M_SELECT_PROJECT", SelectProject, NULL, 0},

#ifdef _SNM_PRESETS
	{ { DEFACCEL, "SWS/S&M: Trigger preset for selected FX of selected tracks (MIDI CC absolute only)" }, "S&M_SELFX_PRESET", TriggerFXPreset, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Trigger preset for FX 1 of selected tracks (MIDI CC absolute only)" }, "S&M_PRESET_FX1", TriggerFXPreset, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Trigger preset for FX 2 of selected tracks (MIDI CC absolute only)" }, "S&M_PRESET_FX2", TriggerFXPreset, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Trigger preset for FX 3 of selected tracks (MIDI CC absolute only)" }, "S&M_PRESET_FX3", TriggerFXPreset, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Trigger preset for FX 4 of selected tracks (MIDI CC absolute only)" }, "S&M_PRESET_FX4", TriggerFXPreset, NULL, 3},
#endif
	{ {}, LAST_COMMAND, }, // denote end of table
};


///////////////////////////////////////////////////////////////////////////////
// Fake toggle states: used to report toggle states in "best effort mode"
// (example: when an action deals with several selected tracks, the overall
// toggle state might be a mix of different other toggle states)
// note: actions using a fake toggle state must explicitely call FakeToggle()
///////////////////////////////////////////////////////////////////////////////

// store fake toogle states, indexed per cmd id (faster + lazy init)
static WDL_IntKeyedArray<bool*> g_fakeToggleStates;

void FakeToggle(COMMAND_T* _ct) {
	if (_ct && _ct->accel.accel.cmd)
		g_fakeToggleStates.Insert(_ct->accel.accel.cmd, *g_fakeToggleStates.Get(_ct->accel.accel.cmd, &g_bFalse) ? &g_bFalse : &g_bTrue);
}

bool FakeIsToggleAction(COMMAND_T* _ct) {
	return (_ct && _ct->accel.accel.cmd && *g_fakeToggleStates.Get(_ct->accel.accel.cmd, &g_bFalse));
}

#ifdef _SNM_MISC
// not used yet, see comments in the "S&M Extension" action section init..
bool FakeIsToggleAction(MIDI_COMMAND_T* _ct) {
	return (_ct && _ct->accel.accel.cmd && ((_ct->excecCount%2) == 1));
}
#endif


///////////////////////////////////////////////////////////////////////////////
// Toolbars auto refresh option
///////////////////////////////////////////////////////////////////////////////

bool g_toolbarsAutoRefreshEnabled = false;
int g_toolbarsAutoRefreshFreq = SNM_DEF_TOOLBAR_RFRSH_FREQ;

void EnableToolbarsAutoRefesh(COMMAND_T* _ct) {
	g_toolbarsAutoRefreshEnabled = !g_toolbarsAutoRefreshEnabled;
}

bool IsToolbarsAutoRefeshEnabled(COMMAND_T* _ct) {
	return g_toolbarsAutoRefreshEnabled;
}

// see SnMCSurfRun()
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

	// host AW's grid toolbar buttons auto refresh and track timebase auto refresh
	UpdateGridToolbar();
	UpdateTrackTimebaseToolbar();
}


///////////////////////////////////////////////////////////////////////////////
// "S&M Extension" action section
// API LIMITATION: the API does not let us register "toggleaction" for other 
//                 sections than the main one..
///////////////////////////////////////////////////////////////////////////////

static WDL_IntKeyedArray<MIDI_COMMAND_T*> g_SNMSection_midiCmds;
KbdCmd g_SNMSection_kbdCmds[SNM_MAX_SECTION_ACTIONS];
KbdKeyBindingInfo g_SNMSection_defKeys[SNM_MAX_SECTION_ACTIONS];
int g_SNMSection_minCmdId = 0;
int g_SNMSection_maxCmdId = 0;
static int g_SNMSection_CmdId_gen = 40000;

bool onAction(int _cmd, int _val, int _valhw, int _relmode, HWND _hwnd)
{
	static bool bReentrancyCheck = false;
	if (bReentrancyCheck)
		return false;

	// Ignore commands that don't have anything to do with us from this point forward
	if (_cmd < g_SNMSection_minCmdId || _cmd > g_SNMSection_maxCmdId)
		return false;
	
	if (MIDI_COMMAND_T* ct = g_SNMSection_midiCmds.Get(_cmd, NULL))
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

int SNMSectionRegisterCommands(reaper_plugin_info_t* _rec, bool _localize)
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
/*JFB no more used (cmd ids are now generated rather than registered in the main section)
			if (!(ct->accel.accel.cmd = plugin_register("command_id", (void*)ct->id)))
				return 0;
*/
			ct->accel.accel.cmd = g_SNMSection_CmdId_gen++;

			if (!g_SNMSection_minCmdId || g_SNMSection_minCmdId > ct->accel.accel.cmd)
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

	// map MIDI_COMMAND_T[] to the section's KbdCmd[] & KbdKeyBindingInfo[]
	for (i=0; i < nbCmds; i++)
	{
		MIDI_COMMAND_T* ct = &g_SNMSection_cmdTable[i];
		g_SNMSection.action_list[i].text = GetLocalizedActionName(ct->id, ct->accel.desc, SNM_I8N_SNM_ACTION_SEC);
		g_SNMSection.action_list[i].cmd = g_SNMSection.def_keys[i].cmd = ct->accel.accel.cmd;
		g_SNMSection.def_keys[i].key = ct->accel.accel.key;
		g_SNMSection.def_keys[i].flags = ct->accel.accel.fVirt;
	}
	g_SNMSection.action_list_cnt = g_SNMSection.def_keys_cnt = nbCmds;

	// register the S&M section
	if (!(_rec->Register("accel_section",&g_SNMSection)))
		return 0;

	return 1;
}

void SNM_ShowActionList(COMMAND_T* _ct) {
	ShowActionList(&g_SNMSection, GetMainHwnd());
}


///////////////////////////////////////////////////////////////////////////////
// "Dynamic" actions
// see g_SNM_dynamicCmdTable's comments
///////////////////////////////////////////////////////////////////////////////

int SNMRegisterDynamicCommands(COMMAND_T* _cmds, const char* _inifn)
{
	char actionName[SNM_MAX_ACTION_NAME_LEN]="", custId[SNM_MAX_ACTION_CUSTID_LEN]="";
	int i = 0;
	while(_cmds[i].id != LAST_COMMAND)
	{
		COMMAND_T* ct = &_cmds[i++];
		int nb = GetPrivateProfileInt("NbOfActions", ct->id, (int)ct->user, _inifn);
		nb = BOUNDED(nb, 0, ct->menuText == NULL ? SNM_MAX_DYNAMIC_ACTIONS : atoi(ct->menuText));
		for (int j=0; j < nb; j++)
		{
			_snprintf(actionName, SNM_MAX_ACTION_NAME_LEN, ct->accel.desc, j+1);
			_snprintf(custId, SNM_MAX_ACTION_CUSTID_LEN, "%s%d", ct->id, j+1);
			if (SWSCreateRegisterDynamicCmd(0, ct->doCommand, ct->getEnabled, custId, actionName, j, __FILE__, true))
				ct->user = nb; // patch the real number of instances
			else
				return 0;
		}
	}
	return 1;
}

void SNMSaveDynamicCommands(COMMAND_T* _cmds, const char* _inifn)
{
	WDL_FastString iniSection, str;
	iniSection.SetFormatted(128, "; Set the number of slot actions you want below (none/hidden: 0, max: %d, exit REAPER first!)\n", SNM_MAX_DYNAMIC_ACTIONS);
	if (IsLangPackUsed("SWS")) {
		iniSection.AppendFormatted(BUFFER_SIZE, 
			"; Note: the name of each action generated here can be updated/localized in the section [%s] of the current LangPack file (%s)\n", 
			SNM_I8N_SWS_ACTION_SEC, GetCurLangPackFn("SWS")->Get());
	}

	WDL_String nameStr; // no fast string here: the buffer gets mangeled..
	int i=0;
	while(_cmds[i].id != LAST_COMMAND)
	{
		COMMAND_T* ct = &_cmds[i++];
/*no! would look for a localized action name!
		nameStr.Set(SNM_CMD_SHORTNAME(ct));
*/
		nameStr.Set((const char*)ct->accel.desc+9); // +9 to skip "SWS/S&M: "
		ReplaceStringFormat(nameStr.Get(), 'n');
		if (ct->menuText != NULL) // is a custom max value specified ?
		{
			nameStr.Append(" (n <= ");
			nameStr.Append(ct->menuText);
			nameStr.Append("!)");
		}

		// indent things (note: a \t solution would suck here!
		str.SetFormatted(BUFFER_SIZE, "%s=%d", ct->id, (int)ct->user);
		while (str.GetLength() < 32) str.Append(" ");
		str.Append(" ; ");
		iniSection.Append(str.Get());
		iniSection.Append(nameStr.Get());
		iniSection.Append("\n");
	}
	SaveIniSection("NbOfActions", &iniSection, _inifn);
}


///////////////////////////////////////////////////////////////////////////////
// S&M.ini file
///////////////////////////////////////////////////////////////////////////////

WDL_FastString g_SNMIniFn;
WDL_FastString g_SNMCyclactionIniFn;
int g_SNMIniFileVersion = 0;
int g_SNMbeta = 0;
int g_SNMMediaFlags = 0;

void IniFileInit()
{
	g_SNMIniFn.SetFormatted(BUFFER_SIZE, SNM_FORMATED_INI_FILE, GetResourcePath());
	g_SNMCyclactionIniFn.SetFormatted(BUFFER_SIZE, SNM_CYCLACTION_INI_FILE, GetResourcePath());

	// move from old location if needed/possible
	WDL_FastString fn;
	fn.SetFormatted(BUFFER_SIZE, SNM_OLD_FORMATED_INI_FILE, GetExePath());
	if (FileExists(fn.Get()))
		MoveFile(fn.Get(), g_SNMIniFn.Get()); // no check: use the new file whatever happens

	// ini files upgrade, if needed
	g_SNMIniFileVersion = GetPrivateProfileInt("General", "IniFileUpgrade", 0, g_SNMIniFn.Get());
	SNM_UpgradeIniFiles();
	fn.SetFormatted(64, "%p", SWSGetCommandByID); WritePrivateProfileString("General", "Magic", fn.Get() , g_SNMIniFn.Get());
	g_SNMIniFileVersion = SNM_INI_FILE_VERSION;

	// load general prefs
	g_SNMMediaFlags |= (GetPrivateProfileInt("General", "MediaFileLockAudio", 0, g_SNMIniFn.Get()) ? 1:0);
	g_toolbarsAutoRefreshEnabled = (GetPrivateProfileInt("General", "ToolbarsAutoRefresh", 1, g_SNMIniFn.Get()) == 1);
	g_toolbarsAutoRefreshFreq = BOUNDED(GetPrivateProfileInt("General", "ToolbarsAutoRefreshFreq", SNM_DEF_TOOLBAR_RFRSH_FREQ, g_SNMIniFn.Get()), 100, 5000);
	g_buggyPlugSupport = GetPrivateProfileInt("General", "BuggyPlugsSupport", 0, g_SNMIniFn.Get());
//	g_SNMbeta = GetPrivateProfileInt("General", "Beta", 0, g_SNMIniFn.Get());
}

void IniFileExit()
{
	// save general prefs & info
	WDL_FastString iniSection;
	iniSection.SetFormatted(128, "; SWS/S&M Extension v%d.%d.%d Build %d\n; ", SWS_VERSION); 
	iniSection.Append(g_SNMIniFn.Get()); 
	iniSection.AppendFormatted(128, "\nIniFileUpgrade=%d\n", g_SNMIniFileVersion); 
	iniSection.AppendFormatted(128, "MediaFileLockAudio=%d\n", g_SNMMediaFlags&1 ? 1:0); 
	iniSection.AppendFormatted(128, "ToolbarsAutoRefresh=%d\n", g_toolbarsAutoRefreshEnabled ? 1 : 0); 
	iniSection.AppendFormatted(128, "ToolbarsAutoRefreshFreq=%d ; in ms (min: 100, max: 5000)\n", g_toolbarsAutoRefreshFreq);
	iniSection.AppendFormatted(128, "BuggyPlugsSupport=%d\n", g_buggyPlugSupport ? 1 : 0); 
//	iniSection.AppendFormatted(128, "Beta=%d\n", g_SNMbeta); 
	SaveIniSection("General", &iniSection, g_SNMIniFn.Get());

	// save dynamic actions
	SNMSaveDynamicCommands(g_SNM_dynamicCmdTable, g_SNMIniFn.Get());

#ifdef _WIN32
	// force ini file's cache flush, see http://support.microsoft.com/kb/68827
	WritePrivateProfileString(NULL, NULL, NULL, g_SNMIniFn.Get());
#endif
}


///////////////////////////////////////////////////////////////////////////////
// S&M core stuff
///////////////////////////////////////////////////////////////////////////////

#ifdef _SWS_MENU
static void SNM_Menuhook(const char* _menustr, HMENU _hMenu, int _flag)
{
	if (!strcmp(_menustr, "Main extensions") && !_flag) SWSCreateMenuFromCommandTable(g_SNM_cmdTable, _hMenu);
/*
	else if (!strcmp(_menustr, "Media item context") && !_flag) {}
	else if (!strcmp(_menustr, "Track control panel context") && !_flag) {}
	else if (_flag == 1) {}
*/
}
#endif

int SnMInit(reaper_plugin_info_t* _rec)
{
	if (!_rec)
		return 0;

	IniFileInit();

#ifdef _SWS_MENU
	if (!plugin_register("hookcustommenu", (void*)SNM_Menuhook))
		return 0;
#endif
	// actions should be registered before views
	if (!SWSRegisterCommands(g_SNM_cmdTable) || 
		!SNMRegisterDynamicCommands(g_SNM_dynamicCmdTable, g_SNMIniFn.Get()) ||
		!SNMSectionRegisterCommands(_rec, true))
		return 0;

	SNM_UIInit();
	LiveConfigViewInit();
	ResourceViewInit();
	NotesHelpViewInit();
	FindViewInit();
	ImageViewInit();
	CyclactionInit(); // keep it as the last one!
	return 1;
}

void SnMExit()
{
	LiveConfigViewExit();
	ResourceViewExit();
	NotesHelpViewExit();
	FindViewExit();
	ImageViewExit();
	CyclactionExit();
	SNM_UIExit();
	IniFileExit();
}


///////////////////////////////////////////////////////////////////////////////
// SNM_ScheduledJob (managed in SnMCSurfRun())
///////////////////////////////////////////////////////////////////////////////

WDL_PtrList_DeleteOnDestroy<SNM_ScheduledJob> g_jobs;
SWS_Mutex g_jobsMutex;

void AddOrReplaceScheduledJob(SNM_ScheduledJob* _job) 
{
	if (!_job) return;
	SWS_SectionLock lock(&g_jobsMutex);
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
	SWS_SectionLock lock(&g_jobsMutex);
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
// SNM_MarkerRegionSubscriber (managed in SnMCSurfRun())
///////////////////////////////////////////////////////////////////////////////

WDL_PtrList<SNM_MarkerRegionSubscriber> g_mkrRgnSubscribers;
SWS_Mutex g_mkrRgnSubscribersMutex;
int g_markerCount=0, g_regionCount=0;

void RegisterToMarkerRegionUpdates(SNM_MarkerRegionSubscriber* _sub) 
{
	if (!_sub) return;
	SWS_SectionLock lock(&g_mkrRgnSubscribersMutex);
	if (g_mkrRgnSubscribers.Find(_sub) < 0)
		g_mkrRgnSubscribers.Add(_sub);
}

void UnregisterToMarkerRegionUpdates(SNM_MarkerRegionSubscriber* _sub) 
{
	if (!_sub) return;
	SWS_SectionLock lock(&g_mkrRgnSubscribersMutex);
	int idx = g_mkrRgnSubscribers.Find(_sub);
	if (idx > 0)
		g_mkrRgnSubscribers.Delete(idx, false);
}

// returns 0 if nothing changed, 1: marker update, 2: region update, 3: both region & marker updates
// note: just diffs nb of markers/regions (that's enough atm)
int MarkerRegionChanged()
{
	int x=0, markerCount=0, regionCount=0; bool isRgn;
	while (x = EnumProjectMarkers2(NULL, x, &isRgn, NULL, NULL, NULL, NULL))
		if (isRgn) regionCount++;
		else markerCount++;

	int updateFlags = (markerCount!=g_markerCount ? 1 : 0);
	updateFlags |= (regionCount!=g_regionCount ? 2 : 0);

	g_regionCount = regionCount;
	g_markerCount = markerCount;

	return updateFlags;
}


///////////////////////////////////////////////////////////////////////////////
// IReaperControlSurface callbacks
///////////////////////////////////////////////////////////////////////////////

double g_toolbarMsCounter = 0.0;
double g_itemSelToolbarMsCounter = 0.0;
double g_markerRegionNotifyMsCounter = 0.0;

void SnMCSurfRun()
{
	// perform scheduled jobs
	{
		SWS_SectionLock lock(&g_jobsMutex);
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

	// marker/region updates notifications
	g_markerRegionNotifyMsCounter += SNM_CSURF_RUN_TICK_MS;
	if (g_markerRegionNotifyMsCounter > 500)
	{
		g_markerRegionNotifyMsCounter = 0.0;

		SWS_SectionLock lock(&g_mkrRgnSubscribersMutex);
		if (int updateFlags = MarkerRegionChanged())
			for (int i=g_mkrRgnSubscribers.GetSize()-1; i >=0; i--)
				g_mkrRgnSubscribers.Get(i)->NotifyMarkerRegionUpdate(updateFlags);
	}

	// stop playing track previews if needed
	StopTrackPreviewsRun();

	// toolbars auto-refresh options
	g_toolbarMsCounter += SNM_CSURF_RUN_TICK_MS;
	g_itemSelToolbarMsCounter += SNM_CSURF_RUN_TICK_MS;

	if (g_itemSelToolbarMsCounter > 1000) { // might be hungry => gentle hard-coded freq
		g_itemSelToolbarMsCounter = 0.0;
		if (g_toolbarsAutoRefreshEnabled) 
			itemSelToolbarPoll();
	}
	if (g_toolbarMsCounter > g_toolbarsAutoRefreshFreq) {
		g_toolbarMsCounter = 0.0;
		if (g_toolbarsAutoRefreshEnabled) 
			RefreshToolbars();
	}
}

extern SNM_NotesHelpWnd* g_pNotesHelpWnd;
extern SNM_LiveConfigsWnd* g_pLiveConfigsWnd;

void SnMCSurfSetTrackTitle() {
	if (g_pNotesHelpWnd) g_pNotesHelpWnd->CSurfSetTrackTitle();
	if (g_pLiveConfigsWnd) g_pLiveConfigsWnd->CSurfSetTrackTitle();
}

void SnMCSurfSetTrackListChange() {
	if (g_pNotesHelpWnd) g_pNotesHelpWnd->CSurfSetTrackListChange();
	if (g_pLiveConfigsWnd) g_pLiveConfigsWnd->CSurfSetTrackListChange();
}

