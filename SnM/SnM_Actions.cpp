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

static COMMAND_T g_commandTable[] = 
{
	// be carefull!!!! 
	// S&M functions expect "SWS/S&M: " in their titles (removed from undo messages, too long)

	// Sends, receives & cue bus ----------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Create cue bus track from track selection, Pre-Fader (Post-FX)" }, "S&M_SENDS1", cueTrack, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Create cue bus track from track selection, Post-Fader (Post-Pan)" }, "S&M_SENDS2", cueTrack, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Create cue bus track from track selection, Pre-FX" }, "S&M_SENDS3", cueTrack, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Create cue bus track from track selection (prompt)" }, "S&M_SENDS4", cueTrackPrompt, NULL, },
	{ { DEFACCEL, "SWS/S&M: Remove receives for selected track(s)" }, "S&M_SENDS5", removeReceives, NULL, },

#ifdef _WIN32
	// Windows ----------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Close all I/O window(s)" }, "S&M_WNCLS1", closeRoutingWindows, NULL, },
	{ { DEFACCEL, "SWS/S&M: Close all envelope window(s)" }, "S&M_WNCLS2", closeEnvWindows, NULL, },
	{ { DEFACCEL, "SWS/S&M: Toggle show all I/O window(s)" }, "S&M_WNTGL1", toggleRoutingWindows, NULL, },
	{ { DEFACCEL, "SWS/S&M: Toggle show all envelope window(s)" }, "S&M_WNTGL2", toggleEnvWindows, NULL, },
#endif

	// Track FX selection -----------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Select previous FX (cycling) for selected track(s)" }, "S&M_SELFXPREV", selectFX, NULL, -2},
	{ { DEFACCEL, "SWS/S&M: Select next FX (cycling) for selected track(s)" }, "S&M_SELFXNEXT", selectFX, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Select FX 1 for selected track(s)" }, "S&M_SELFX1", selectFX, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Select FX 2 for selected track(s)" }, "S&M_SELFX2", selectFX, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Select FX 3 for selected track(s)" }, "S&M_SELFX3", selectFX, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Select FX 4 for selected track(s)" }, "S&M_SELFX4", selectFX, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Select FX 5 for selected track(s)" }, "S&M_SELFX5", selectFX, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Select FX 6 for selected track(s)" }, "S&M_SELFX6", selectFX, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Select FX 7 for selected track(s)" }, "S&M_SELFX7", selectFX, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Select FX 8 for selected track(s)" }, "S&M_SELFX8", selectFX, NULL, 7},

	// Track FX online/offline & bypass/unbypass ------------------------------
	{ { DEFACCEL, "SWS/S&M: Toggle FX 1 online/offline for selected track(s)" }, "S&M_FXOFF1", toggleFXOfflineSelectedTracks, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 2 online/offline for selected track(s)" }, "S&M_FXOFF2", toggleFXOfflineSelectedTracks, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 3 online/offline for selected track(s)" }, "S&M_FXOFF3", toggleFXOfflineSelectedTracks, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 4 online/offline for selected track(s)" }, "S&M_FXOFF4", toggleFXOfflineSelectedTracks, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 5 online/offline for selected track(s)" }, "S&M_FXOFF5", toggleFXOfflineSelectedTracks, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 6 online/offline for selected track(s)" }, "S&M_FXOFF6", toggleFXOfflineSelectedTracks, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 7 online/offline for selected track(s)" }, "S&M_FXOFF7", toggleFXOfflineSelectedTracks, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 8 online/offline for selected track(s)" }, "S&M_FXOFF8", toggleFXOfflineSelectedTracks, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Toggle last FX online/offline for selected track(s)" }, "S&M_FXOFFLAST", toggleFXOfflineSelectedTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Toggle selected FX online/offline for selected track(s)" }, "S&M_FXOFFSEL", toggleFXOfflineSelectedTracks, NULL, 0},

	{ { DEFACCEL, "SWS/S&M: Set FX 1 online for selected track(s)" }, "S&M_FXOFF_SETON1", setFXOnlineSelectedTracks, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Set FX 2 online for selected track(s)" }, "S&M_FXOFF_SETON2", setFXOnlineSelectedTracks, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Set FX 3 online for selected track(s)" }, "S&M_FXOFF_SETON3", setFXOnlineSelectedTracks, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Set FX 4 online for selected track(s)" }, "S&M_FXOFF_SETON4", setFXOnlineSelectedTracks, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Set FX 5 online for selected track(s)" }, "S&M_FXOFF_SETON5", setFXOnlineSelectedTracks, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Set FX 6 online for selected track(s)" }, "S&M_FXOFF_SETON6", setFXOnlineSelectedTracks, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Set FX 7 online for selected track(s)" }, "S&M_FXOFF_SETON7", setFXOnlineSelectedTracks, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Set FX 8 online for selected track(s)" }, "S&M_FXOFF_SETON8", setFXOnlineSelectedTracks, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Set last FX online for selected track(s)" }, "S&M_FXOFF_SETONLAST", setFXOnlineSelectedTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Set selected FX online for selected track(s)" }, "S&M_FXOFF_SETONSEL", setFXOnlineSelectedTracks, NULL, 0},

	{ { DEFACCEL, "SWS/S&M: Set FX 1 offline for selected track(s)" }, "S&M_FXOFF_SETOFF1", setFXOfflineSelectedTracks, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Set FX 2 offline for selected track(s)" }, "S&M_FXOFF_SETOFF2", setFXOfflineSelectedTracks, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Set FX 3 offline for selected track(s)" }, "S&M_FXOFF_SETOFF3", setFXOfflineSelectedTracks, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Set FX 4 offline for selected track(s)" }, "S&M_FXOFF_SETOFF4", setFXOfflineSelectedTracks, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Set FX 5 offline for selected track(s)" }, "S&M_FXOFF_SETOFF5", setFXOfflineSelectedTracks, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Set FX 6 offline for selected track(s)" }, "S&M_FXOFF_SETOFF6", setFXOfflineSelectedTracks, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Set FX 7 offline for selected track(s)" }, "S&M_FXOFF_SETOFF7", setFXOfflineSelectedTracks, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Set FX 8 offline for selected track(s)" }, "S&M_FXOFF_SETOFF8", setFXOfflineSelectedTracks, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Set last FX offline for selected track(s)" }, "S&M_FXOFF_SETOFFLAST", setFXOfflineSelectedTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Set selected FX offline for selected track(s)" }, "S&M_FXOFF_SETOFFSEL", setFXOfflineSelectedTracks, NULL, 0},

	{ { DEFACCEL, "SWS/S&M: Toggle all FXs online/offline for selected track(s)" }, "S&M_FXOFFALL", toggleAllFXsOfflineSelectedTracks, NULL, },

	{ { DEFACCEL, "SWS/S&M: Toggle FX 1 bypass for selected track(s)" }, "S&M_FXBYP1", toggleFXBypassSelectedTracks, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 2 bypass for selected track(s)" }, "S&M_FXBYP2", toggleFXBypassSelectedTracks, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 3 bypass for selected track(s)" }, "S&M_FXBYP3", toggleFXBypassSelectedTracks, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 4 bypass for selected track(s)" }, "S&M_FXBYP4", toggleFXBypassSelectedTracks, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 5 bypass for selected track(s)" }, "S&M_FXBYP5", toggleFXBypassSelectedTracks, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 6 bypass for selected track(s)" }, "S&M_FXBYP6", toggleFXBypassSelectedTracks, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 7 bypass for selected track(s)" }, "S&M_FXBYP7", toggleFXBypassSelectedTracks, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Toggle FX 8 bypass for selected track(s)" }, "S&M_FXBYP8", toggleFXBypassSelectedTracks, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Toggle last FX bypass for selected track(s)" }, "S&M_FXBYPLAST", toggleFXBypassSelectedTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Toggle selected FX bypass for selected track(s)" }, "S&M_FXBYPSEL", toggleFXBypassSelectedTracks, NULL, 0},
	
	{ { DEFACCEL, "SWS/S&M: Bypass FX 1 for selected track(s)" }, "S&M_FXBYP_SETON1", setFXBypassSelectedTracks, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Bypass FX 2 for selected track(s)" }, "S&M_FXBYP_SETON2", setFXBypassSelectedTracks, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Bypass FX 3 for selected track(s)" }, "S&M_FXBYP_SETON3", setFXBypassSelectedTracks, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Bypass FX 4 for selected track(s)" }, "S&M_FXBYP_SETON4", setFXBypassSelectedTracks, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Bypass FX 5 for selected track(s)" }, "S&M_FXBYP_SETON5", setFXBypassSelectedTracks, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Bypass FX 6 for selected track(s)" }, "S&M_FXBYP_SETON6", setFXBypassSelectedTracks, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Bypass FX 7 for selected track(s)" }, "S&M_FXBYP_SETON7", setFXBypassSelectedTracks, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Bypass FX 8 for selected track(s)" }, "S&M_FXBYP_SETON8", setFXBypassSelectedTracks, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Bypass last FX bypass for selected track(s)" }, "S&M_FXBYP_SETONLAST", setFXBypassSelectedTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Bypass selected FX bypass for selected track(s)" }, "S&M_FXBYP_SETONSEL", setFXBypassSelectedTracks, NULL, 0},
	
	{ { DEFACCEL, "SWS/S&M: Unbypass FX 1 for selected track(s)" }, "S&M_FXBYP_SETOFF1", setFXUnbypassSelectedTracks, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Unbypass FX 2 for selected track(s)" }, "S&M_FXBYP_SETOFF2", setFXUnbypassSelectedTracks, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Unbypass FX 3 for selected track(s)" }, "S&M_FXBYP_SETOFF3", setFXUnbypassSelectedTracks, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Unbypass FX 4 for selected track(s)" }, "S&M_FXBYP_SETOFF4", setFXUnbypassSelectedTracks, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Unbypass FX 5 for selected track(s)" }, "S&M_FXBYP_SETOFF5", setFXUnbypassSelectedTracks, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Unbypass FX 6 for selected track(s)" }, "S&M_FXBYP_SETOFF6", setFXUnbypassSelectedTracks, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Unbypass FX 7 for selected track(s)" }, "S&M_FXBYP_SETOFF7", setFXUnbypassSelectedTracks, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Unbypass FX 8 for selected track(s)" }, "S&M_FXBYP_SETOFF8", setFXUnbypassSelectedTracks, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Unbypass last FX for selected track(s)" }, "S&M_FXBYP_SETOFFLAST", setFXUnbypassSelectedTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Unbypass selected FX for selected track(s)" }, "S&M_FXBYP_SETOFFSEL", setFXUnbypassSelectedTracks, NULL, 0},
	
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs bypass for selected track(s)" }, "S&M_FXBYPALL", toggleAllFXsBypassSelectedTracks, NULL, },

	{ { DEFACCEL, "SWS/S&M: Bypass all FXs for selected track(s)" }, "S&M_FXBYPALL2", setAllFXsBypassSelectedTracks, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Unbypass all FXs for selected track(s)" }, "S&M_FXBYPALL3", setAllFXsBypassSelectedTracks, NULL, 0},
	// ..related online/offline actions natively implemented

	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 1) online/offline for selected track(s)" }, "S&M_FXOFFEXCPT1", toggleExceptFXOfflineSelectedTracks, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 2) online/offline for selected track(s)" }, "S&M_FXOFFEXCPT2", toggleExceptFXOfflineSelectedTracks, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 3) online/offline for selected track(s)" }, "S&M_FXOFFEXCPT3", toggleExceptFXOfflineSelectedTracks, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 4) online/offline for selected track(s)" }, "S&M_FXOFFEXCPT4", toggleExceptFXOfflineSelectedTracks, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 5) online/offline for selected track(s)" }, "S&M_FXOFFEXCPT5", toggleExceptFXOfflineSelectedTracks, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 6) online/offline for selected track(s)" }, "S&M_FXOFFEXCPT6", toggleExceptFXOfflineSelectedTracks, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 7) online/offline for selected track(s)" }, "S&M_FXOFFEXCPT7", toggleExceptFXOfflineSelectedTracks, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 8) online/offline for selected track(s)" }, "S&M_FXOFFEXCPT8", toggleExceptFXOfflineSelectedTracks, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except selected) online/offline for selected track(s)" }, "S&M_FXOFFEXCPTSEL", toggleExceptFXOfflineSelectedTracks, NULL, 0},

	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 1) bypass for selected track(s)" }, "S&M_FXBYPEXCPT1", toggleExceptFXBypassSelectedTracks, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 2) bypass for selected track(s)" }, "S&M_FXBYPEXCPT2", toggleExceptFXBypassSelectedTracks, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 3) bypass for selected track(s)" }, "S&M_FXBYPEXCPT3", toggleExceptFXBypassSelectedTracks, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 4) bypass for selected track(s)" }, "S&M_FXBYPEXCPT4", toggleExceptFXBypassSelectedTracks, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 5) bypass for selected track(s)" }, "S&M_FXBYPEXCPT5", toggleExceptFXBypassSelectedTracks, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 6) bypass for selected track(s)" }, "S&M_FXBYPEXCPT6", toggleExceptFXBypassSelectedTracks, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 7) bypass for selected track(s)" }, "S&M_FXBYPEXCPT7", toggleExceptFXBypassSelectedTracks, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except 8) bypass for selected track(s)" }, "S&M_FXBYPEXCPT8", toggleExceptFXBypassSelectedTracks, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Toggle all FXs (except selected) bypass for selected track(s)" }, "S&M_FXBYPEXCPTSEL", toggleExceptFXBypassSelectedTracks, NULL, 0},


	// FX Chains (items & tracks) ---------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Show FX chain (FX 1) for selected track(s)" }, "S&M_SHOWFXCHAIN1", showFXChain, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Show FX chain (FX 2) for selected track(s)" }, "S&M_SHOWFXCHAIN2", showFXChain, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Show FX chain (FX 3) for selected track(s)" }, "S&M_SHOWFXCHAIN3", showFXChain, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Show FX chain (FX 4) for selected track(s)" }, "S&M_SHOWFXCHAIN4", showFXChain, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Show FX chain (FX 5) for selected track(s)" }, "S&M_SHOWFXCHAIN5", showFXChain, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Show FX chain (FX 6) for selected track(s)" }, "S&M_SHOWFXCHAIN6", showFXChain, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Show FX chain (FX 7) for selected track(s)" }, "S&M_SHOWFXCHAIN7", showFXChain, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Show FX chain (FX 8) for selected track(s)" }, "S&M_SHOWFXCHAIN8", showFXChain, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Show FX chain (selected FX) for selected track(s)" }, "S&M_SHOWFXCHAINSEL", showFXChain, NULL, -1},

	{ { DEFACCEL, "SWS/S&M: Float FX 1 window for selected track(s)" }, "S&M_FLOATFX1", floatFX, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Float FX 2 window for selected track(s)" }, "S&M_FLOATFX2", floatFX, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Float FX 3 window for selected track(s)" }, "S&M_FLOATFX3", floatFX, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Float FX 4 window for selected track(s)" }, "S&M_FLOATFX4", floatFX, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Float FX 5 window for selected track(s)" }, "S&M_FLOATFX5", floatFX, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Float FX 6 window for selected track(s)" }, "S&M_FLOATFX6", floatFX, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Float FX 7 window for selected track(s)" }, "S&M_FLOATFX7", floatFX, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Float FX 8 window for selected track(s)" }, "S&M_FLOATFX8", floatFX, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Float selected FX window for selected track(s)" }, "S&M_FLOATFXSEL", floatFX, NULL, -1},

/* Commented for the moment: REAPER doesn't obey..
	{ { DEFACCEL, "SWS/S&M: Un-float FX 1 window for selected track(s)" }, "S&M_UNFLOATFX1", unfloatFX, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Un-float FX 2 window for selected track(s)" }, "S&M_UNFLOATFX2", unfloatFX, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Un-float FX 3 window for selected track(s)" }, "S&M_UNFLOATFX3", unfloatFX, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Un-float FX 4 window for selected track(s)" }, "S&M_UNFLOATFX4", unfloatFX, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Un-float FX 5 window for selected track(s)" }, "S&M_UNFLOATFX5", unfloatFX, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Un-float FX 6 window for selected track(s)" }, "S&M_UNFLOATFX6", unfloatFX, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Un-float FX 7 window for selected track(s)" }, "S&M_UNFLOATFX7", unfloatFX, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Un-float FX 8 window for selected track(s)" }, "S&M_UNFLOATFX8", unfloatFX, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Un-float selected FX window for selected track(s)" }, "S&M_UNFLOATFXSEL", unfloatFX, NULL, -1},
*/
	{ { DEFACCEL, "SWS/S&M: Load/Paste FX chain to selected item(s), slot 1" }, "S&M_TAKEFXCHAIN1", loadPasteTakeFXChain, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Load/Paste FX chain to selected item(s), slot 2" }, "S&M_TAKEFXCHAIN2", loadPasteTakeFXChain, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Load/Paste FX chain to selected item(s), slot 3" }, "S&M_TAKEFXCHAIN3", loadPasteTakeFXChain, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Load/Paste FX chain to selected item(s), slot 4" }, "S&M_TAKEFXCHAIN4", loadPasteTakeFXChain, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Load/Paste FX chain to selected item(s), slot 5" }, "S&M_TAKEFXCHAIN5", loadPasteTakeFXChain, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Load/Paste FX chain to selected item(s), slot 6" }, "S&M_TAKEFXCHAIN6", loadPasteTakeFXChain, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Load/Paste FX chain to selected item(s), slot 7" }, "S&M_TAKEFXCHAIN7", loadPasteTakeFXChain, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Load/Paste FX chain to selected item(s), slot 8" }, "S&M_TAKEFXCHAIN8", loadPasteTakeFXChain, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Load/Paste FX chain to selected item(s), prompt for slot" }, "S&M_TAKEFXCHAINp1", loadPasteTakeFXChain, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Load/Paste FX chain to selected item(s), all takes, prompt for slot" }, "S&M_TAKEFXCHAINp2", loadPasteAllTakesFXChain, NULL, -1},

	{ { DEFACCEL, "SWS/S&M: Copy FX chain from selected item" }, "S&M_COPYFXCHAIN1", copyTakeFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Cut FX chain from selected item" }, "S&M_COPYFXCHAIN2", cutTakeFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Paste FX chain to selected item(s)" }, "S&M_COPYFXCHAIN3", pasteTakeFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Paste FX chain to selected item(s), all takes" }, "S&M_COPYFXCHAIN4", pasteAllTakesFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Copy FX chain from selected track" }, "S&M_COPYFXCHAIN5", copyTrackFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Cut FX chain from selected track" }, "S&M_COPYFXCHAIN6", cutTrackFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Paste FX chain to selected track(s)" }, "S&M_COPYFXCHAIN7", pasteTrackFXChain, NULL, }, 

	{ { DEFACCEL, "SWS/S&M: Clear FX chain for selected item(s)" },  "S&M_CLRFXCHAIN1", clearActiveTakeFXChain, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Clear FX chain for selected item(s), all takes" },  "S&M_CLRFXCHAIN2", clearAllTakesFXChain, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Clear FX chain for selected track(s)" }, "S&M_CLRFXCHAIN3", clearTrackFXChain, NULL, 0},

	{ { DEFACCEL, "SWS/S&M: Load/Paste FX chain to selected track(s), slot 1" }, "S&M_TRACKFXCHAIN1", loadPasteTrackFXChain, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Load/Paste FX chain to selected track(s), slot 2" }, "S&M_TRACKFXCHAIN2", loadPasteTrackFXChain, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Load/Paste FX chain to selected track(s), slot 3" }, "S&M_TRACKFXCHAIN3", loadPasteTrackFXChain, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Load/Paste FX chain to selected track(s), slot 4" }, "S&M_TRACKFXCHAIN4", loadPasteTrackFXChain, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Load/Paste FX chain to selected track(s), slot 5" }, "S&M_TRACKFXCHAIN5", loadPasteTrackFXChain, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Load/Paste FX chain to selected track(s), slot 6" }, "S&M_TRACKFXCHAIN6", loadPasteTrackFXChain, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Load/Paste FX chain to selected track(s), slot 7" }, "S&M_TRACKFXCHAIN7", loadPasteTrackFXChain, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Load/Paste FX chain to selected track(s), slot 8" }, "S&M_TRACKFXCHAIN8", loadPasteTrackFXChain, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Load/Paste FX chain to selected track(s), prompt for slot" }, "S&M_TRACKFXCHAINp1", loadPasteTrackFXChain, NULL, -1},

	{ { DEFACCEL, "SWS/S&M: List FX chain slots..." }, "S&M_SHOWFXCHAINSLOTS", showFXChainSlots, NULL, },
	{ { DEFACCEL, "SWS/S&M: Clear FX chain slot..." }, "S&M_CLRFXCHAINSLOT", clearFXChainSlot, NULL, },
	
	
	// Takes ------------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Comping - Clear active take(s)" }, "S&M_CLRTAKE1", clearTake, NULL, },
	{ { DEFACCEL, "SWS/S&M: Comping - Build lane(s) for selected track(s)" }, "S&M_LANETAKE1", makeTakeLanesSelectedTracks, NULL, },
	{ { DEFACCEL, "SWS/S&M: Comping - Select lane from selected item" }, "S&M_LANETAKE2", selectTakeLane, NULL, },
	{ { DEFACCEL, "SWS/S&M: Comping - Remove empty take(s) in selected item(s)" }, "S&M_DELEMPTYTAKE", removeEmptyTakes, NULL, },
	{ { DEFACCEL, "SWS/S&M: Comping - Move takes up (cycling)" }, "S&M_MOVETAKE1", moveTake, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Comping - Move takes down (cycling)" }, "S&M_MOVETAKE2", moveTake, NULL, 1},

	{ { DEFACCEL, "SWS/S&M: Split MIDI or Audio at prior zero crossing" }, "S&M_SPLIT1", splitMidiAudio, NULL, },

	// Experimental, misc., deprecated, etc.. ---------------------------------
//	{ { DEFACCEL, "SWS/S&M: Move track (1 -> 4)" }, "S&M_TMP1", moveTest, NULL, },
//	{ { DEFACCEL, "SWS/S&M: Select items by name" }, "S&M_ITM1", selectItemsByNamePrompt, NULL, },

	{ {}, LAST_COMMAND, }, // Denote end of table
};

int SnMActionsInit()
{
	return SWSRegisterCommands(g_commandTable);
}

// GUI for lazy guys
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