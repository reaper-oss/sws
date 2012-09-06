/******************************************************************************
/ SnM.cpp
/
/ Copyright (c) 2009-2012 Jeffos
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
#include "SnM.h"
#include "version.h"
#include "../Misc/Adam.h"
#include "../reaper/localize.h"


void QuickTest(COMMAND_T* _ct) {}


///////////////////////////////////////////////////////////////////////////////
// S&M actions (main section)
///////////////////////////////////////////////////////////////////////////////

static COMMAND_T g_SNM_cmdTable[] = 
{
//	{ { DEFACCEL, "SWS/S&M: [Internal] QuickTest" }, "S&M_QUICKTEST", QuickTest, NULL, },

//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions

	// Routing & cue buss -----------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Create cue buss from track selection (use last settings)" }, "S&M_CUEBUS", CueBuss, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Open/close Cue Buss generator" }, "S&M_SENDS4", OpenCueBussDlg, NULL, NULL, IsCueBussDlgDisplayed},

	{ { DEFACCEL, "SWS/S&M: Remove receives from selected tracks" }, "S&M_SENDS5", RemoveReceives, NULL, },
	{ { DEFACCEL, "SWS/S&M: Remove sends from selected tracks" }, "S&M_SENDS6", RemoveSends, NULL, },
	{ { DEFACCEL, "SWS/S&M: Remove routing from selected tracks" }, "S&M_SENDS7", RemoveRouting, NULL, },

	{ { DEFACCEL, "SWS/S&M: Copy selected tracks (with routing)" }, "S&M_COPYSNDRCV1", CopyWithIOs, NULL, },
	{ { DEFACCEL, "SWS/S&M: Paste tracks (with routing) or items" }, "S&M_PASTSNDRCV1", PasteWithIOs, NULL, },
	{ { DEFACCEL, "SWS/S&M: Cut selected tracks (with routing)" }, "S&M_CUTSNDRCV1", CutWithIOs, NULL, },

	{ { DEFACCEL, "SWS/S&M: Copy selected tracks routings" }, "S&M_COPYSNDRCV2", CopyRoutings, NULL, },
	{ { DEFACCEL, "SWS/S&M: Paste routings to selected tracks" }, "S&M_PASTSNDRCV2", PasteRoutings, NULL, },
	{ { DEFACCEL, "SWS/S&M: Cut selected tracks routings" }, "S&M_CUTSNDRCV2", CutRoutings, NULL, },

	{ { DEFACCEL, "SWS/S&M: Copy selected tracks sends" }, "S&M_COPYSNDRCV3", CopySends, NULL, },
	{ { DEFACCEL, "SWS/S&M: Paste sends to selected tracks" }, "S&M_PASTSNDRCV3", PasteSends, NULL, },
	{ { DEFACCEL, "SWS/S&M: Cut selected tracks sends" }, "S&M_CUTSNDRCV3", CutSends, NULL, },

	{ { DEFACCEL, "SWS/S&M: Copy selected tracks receives" }, "S&M_COPYSNDRCV4", CopyReceives, NULL, },
	{ { DEFACCEL, "SWS/S&M: Paste receives to selected tracks" }, "S&M_PASTSNDRCV4", PasteReceives, NULL, },
	{ { DEFACCEL, "SWS/S&M: Cut selected tracks receives" }, "S&M_CUTSNDRCV4", CutReceives, NULL, },

	{ { DEFACCEL, "SWS/S&M: Save default track send preferences" }, "S&M_SAVE_DEFSNDFLAGS", SaveDefaultTrackSendPrefs, NULL, },
	{ { DEFACCEL, "SWS/S&M: Recall default track send preferences" }, "S&M_RECALL_DEFSNDFLAGS", RecallDefaultTrackSendPrefs, NULL, },
	{ { DEFACCEL, "SWS/S&M: Set default track sends to audio and MIDI" }, "S&M_DEFSNDFLAGS_BOTH", SetDefaultTrackSendPrefs, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Set default track sends to audio only" }, "S&M_DEFSNDFLAGS_AUDIO", SetDefaultTrackSendPrefs, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Set default track sends to MIDI only" }, "S&M_DEFSNDFLAGS_MIDI", SetDefaultTrackSendPrefs, NULL, 2},

	// Windows ----------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Close all floating FX windows" }, "S&M_WNCLS3", CloseAllFXWindows, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Close all FX chain windows" }, "S&M_WNCLS4", CloseAllFXChainsWindows, NULL, },
	{ { DEFACCEL, "SWS/S&M: Close all floating FX windows for selected tracks" }, "S&M_WNCLS5", CloseAllFXWindows, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Close all floating FX windows, except focused one" }, "S&M_WNCLS6", CloseAllFXWindowsExceptFocused, NULL, },
	{ { DEFACCEL, "SWS/S&M: Show all floating FX windows (!)" }, "S&M_WNTSHW1", ShowAllFXWindows, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Show all FX chain windows (!)" }, "S&M_WNTSHW2", ShowAllFXChainsWindows, NULL, },
	{ { DEFACCEL, "SWS/S&M: Show all floating FX windows for selected tracks" }, "S&M_WNTSHW3", ShowAllFXWindows, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Toggle show all floating FX (!)" }, "S&M_WNTGL3", ToggleAllFXWindows, NULL, 0, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle show all FX chain windows (!)" }, "S&M_WNTGL4", ToggleAllFXChainsWindows, NULL, -666, FakeIsToggleAction},	
	{ { DEFACCEL, "SWS/S&M: Toggle show all floating FX for selected tracks" }, "S&M_WNTGL5", ToggleAllFXWindows, NULL, 1, FakeIsToggleAction},

	{ { DEFACCEL, "SWS/S&M: Float previous FX (and close others) for selected tracks" }, "S&M_WNONLY1", CycleFloatFXWndSelTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Float next FX (and close others) for selected tracks" }, "S&M_WNONLY2", CycleFloatFXWndSelTracks, NULL, 1},

	// see above comment, used to be `... (+ main window on cycle)` => turned into `... (cycle)`
	{ { DEFACCEL, "SWS/S&M: Focus previous floating FX for selected tracks (cycle)" }, "S&M_WNFOCUS5", CycleFocusFXMainWndSelTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Focus next floating FX for selected tracks (cycle)" }, "S&M_WNFOCUS6", CycleFocusFXMainWndSelTracks, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Focus previous floating FX (cycle)" }, "S&M_WNFOCUS7", CycleFocusFXMainWndAllTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Focus next floating FX (cycle)" }, "S&M_WNFOCUS8", CycleFocusFXMainWndAllTracks, NULL, 1},

	{ { DEFACCEL, "SWS/S&M: Focus main window" }, "S&M_WNMAIN", FocusMainWindow, NULL, },
#ifdef _WIN32
	{ { DEFACCEL, "SWS/S&M: Focus main window (close others)" }, "S&M_WNMAIN_HIDE_OTHERS", FocusMainWindowCloseOthers, NULL, },
	{ { DEFACCEL, "SWS/S&M: Cycle focused window" }, "S&M_WNFOCUS9", CycleFocusWnd, NULL, },
	{ { DEFACCEL, "SWS/S&M: Focus next window (cycle, hide/unhide others)" }, "S&M_WNFOCUS_NEXT", CycleFocusHideOthersWnd, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Focus previous window (cycle, hide/unhide others)" }, "S&M_WNFOCUS_PREV", CycleFocusHideOthersWnd, NULL, -1},
#endif
	{ { DEFACCEL, "SWS/S&M: Show FX chain for selected tracks (selected FX)" }, "S&M_SHOWFXCHAINSEL", ShowFXChain, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Hide FX chain windows for selected tracks" }, "S&M_HIDEFXCHAIN", HideFXChain, NULL, },
	{ { DEFACCEL, "SWS/S&M: Toggle show FX chain windows for selected tracks" }, "S&M_TOGLFXCHAIN", ToggleFXChain, NULL, -666, IsToggleFXChain},

	{ { DEFACCEL, "SWS/S&M: Float selected FX for selected tracks" }, "S&M_FLOATFXEL", FloatFX, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Unfloat selected FX for selected tracks" }, "S&M_UNFLOATFXEL", UnfloatFX, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Toggle float selected FX for selected tracks" }, "S&M_TOGLFLOATFXEL", ToggleFloatFX, NULL, -1, FakeIsToggleAction},

	// Track FX selection & move up/down---------------------------------------
	{ { DEFACCEL, "SWS/S&M: Select last FX for selected tracks" }, "S&M_SEL_LAST_FX", SelectTrackFX, NULL, -3},
	{ { DEFACCEL, "SWS/S&M: Select previous FX (cycling) for selected tracks" }, "S&M_SELFXPREV", SelectTrackFX, NULL, -2},
	{ { DEFACCEL, "SWS/S&M: Select next FX (cycling) for selected tracks" }, "S&M_SELFXNEXT", SelectTrackFX, NULL, -1},

	{ { DEFACCEL, "SWS/S&M: Move selected FX up in chain for selected tracks" }, "S&M_MOVE_FX_UP", MoveFX, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Move selected FX down in chain for selected tracks" }, "S&M_MOVE_FX_DOWN", MoveFX, NULL, 1},

	// Track FX online/offline & bypass/unbypass ------------------------------
	{ { DEFACCEL, "SWS/S&M: Toggle last FX online/offline for selected tracks" }, "S&M_FXOFFLAST", ToggleFXOfflineSelTracks, NULL, -2, IsFXOfflineSelTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle selected FX online/offline for selected tracks" }, "S&M_FXOFFSEL", ToggleFXOfflineSelTracks, NULL, -1, IsFXOfflineSelTracks},
	{ { DEFACCEL, "SWS/S&M: Set last FX online for selected tracks" }, "S&M_FXOFF_SETONLAST", SetFXOnlineSelTracks, NULL, -2},
	{ { DEFACCEL, "SWS/S&M: Set selected FX online for selected tracks" }, "S&M_FXOFF_SETONSEL", SetFXOnlineSelTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Set last FX offline for selected tracks" }, "S&M_FXOFF_SETOFFLAST", SetFXOfflineSelTracks, NULL, -2},
	{ { DEFACCEL, "SWS/S&M: Set selected FX offline for selected tracks" }, "S&M_FXOFF_SETOFFSEL", SetFXOfflineSelTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX online/offline for selected tracks" }, "S&M_FXOFFALL", ToggleAllFXsOfflineSelTracks, NULL, -666, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle last FX bypass for selected tracks" }, "S&M_FXBYPLAST", ToggleFXBypassSelTracks, NULL, -2, IsFXBypassedSelTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle selected FX bypass for selected tracks" }, "S&M_FXBYPSEL", ToggleFXBypassSelTracks, NULL, -1, IsFXBypassedSelTracks},
	{ { DEFACCEL, "SWS/S&M: Bypass last FX for selected tracks" }, "S&M_FXBYP_SETONLAST", SetFXBypassSelTracks, NULL, -2},
	{ { DEFACCEL, "SWS/S&M: Bypass selected FX for selected tracks" }, "S&M_FXBYP_SETONSEL", SetFXBypassSelTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Unbypass last FX for selected tracks" }, "S&M_FXBYP_SETOFFLAST", SetFXUnbypassSelTracks, NULL, -2},
	{ { DEFACCEL, "SWS/S&M: Unbypass selected FX for selected tracks" }, "S&M_FXBYP_SETOFFSEL", SetFXUnbypassSelTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX bypass for selected tracks" }, "S&M_FXBYPALL", ToggleAllFXsBypassSelTracks, NULL, -666, FakeIsToggleAction},
	// note: toggle online/offline actions exist natively ^^
	{ { DEFACCEL, "SWS/S&M: Bypass all FX for selected tracks" }, "S&M_FXBYPALL2", SetAllFXsBypassSelTracks, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Unbypass all FX for selected tracks" }, "S&M_FXBYPALL3", SetAllFXsBypassSelTracks, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except selected) online/offline for selected tracks" }, "S&M_FXOFFEXCPTSEL", ToggleExceptFXOfflineSelTracks, NULL, -1, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except selected) bypass for selected tracks" }, "S&M_FXBYPEXCPTSEL", ToggleExceptFXBypassSelTracks, NULL, -1, FakeIsToggleAction},

	// Take FX online/offline & bypass/unbypass ------------------------------
	{ { DEFACCEL, "SWS/S&M: Toggle all take FX online/offline for selected items" }, "S&M_TGL_TAKEFX_ONLINE", ToggleAllFXsOfflineSelItems, NULL, -666, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Set all take FX offline for selected items" }, "S&M_TAKEFX_OFFLINE", SetAllFXsOfflineSelItems, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Set all take FX online for selected items" }, "S&M_TAKEFX_ONLINE", SetAllFXsOfflineSelItems, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Toggle all take FX bypass for selected items" }, "S&M_TGL_TAKEFX_BYP", ToggleAllFXsBypassSelItems, NULL, -666, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Bypass all take FX for selected items" }, "S&M_TAKEFX_BYPASS", SetAllFXsBypassSelItems, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Unbypass all take FX for selected items" }, "S&M_TAKEFX_UNBYPASS", SetAllFXsBypassSelItems, NULL, 0},

	// Resources view
	{ { DEFACCEL, "SWS/S&M: Open/close Resources window" }, "S&M_SHOW_RESOURCES_VIEW", OpenResourceView, NULL, -1, IsResourceViewDisplayed},

	// FX Chains (items & tracks) ---------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Open/close Resources window (FX chains)" }, "S&M_SHOWFXCHAINSLOTS", OpenResourceView, NULL, SNM_SLOT_FXC, IsResourceViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Clear FX chain slot..." }, "S&M_CLRFXCHAINSLOT", ResViewClearSlotPrompt, NULL, SNM_SLOT_FXC},
	{ { DEFACCEL, "SWS/S&M: Delete all FX chain slots" }, "S&M_DEL_ALL_FXCHAINSLOT", ResViewDeleteAllSlots, NULL, SNM_SLOT_FXC},
	{ { DEFACCEL, "SWS/S&M: Auto-save FX Chain slots for selected tracks" }, "S&M_SAVE_FXCHAIN_SLOT1", ResViewAutoSaveFXChain, NULL, FXC_AUTOSAVE_PREF_TRACK},
	{ { DEFACCEL, "SWS/S&M: Auto-save FX Chain slots for selected items" }, "S&M_SAVE_FXCHAIN_SLOT2", ResViewAutoSaveFXChain, NULL, FXC_AUTOSAVE_PREF_ITEM},
	{ { DEFACCEL, "SWS/S&M: Auto-save input FX Chain slots for selected tracks" }, "S&M_SAVE_FXCHAIN_SLOT3", ResViewAutoSaveFXChain, NULL, FXC_AUTOSAVE_PREF_INPUT_FX},
	{ { DEFACCEL, "SWS/S&M: Paste (replace) FX chain to selected items, prompt for slot" }, "S&M_TAKEFXCHAINp1", LoadSetTakeFXChain, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Paste (replace) FX chain to selected items, all takes, prompt for slot" }, "S&M_TAKEFXCHAINp2", LoadSetAllTakesFXChain, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Paste FX chain to selected items, prompt for slot" }, "S&M_PASTE_TAKEFXCHAINp1", LoadPasteTakeFXChain, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Paste FX chain to selected items, all takes, prompt for slot" }, "S&M_PASTE_TAKEFXCHAINp2", LoadPasteAllTakesFXChain, NULL, -1},

	{ { DEFACCEL, "SWS/S&M: Copy FX chain from selected item" }, "S&M_COPYFXCHAIN1", CopyTakeFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Cut FX chain from selected items" }, "S&M_COPYFXCHAIN2", CutTakeFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Paste (replace) FX chain to selected items" }, "S&M_COPYFXCHAIN3", SetTakeFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Paste (replace) FX chain to selected items, all takes" }, "S&M_COPYFXCHAIN4", SetAllTakesFXChain, NULL, }, 

	{ { DEFACCEL, "SWS/S&M: Copy FX chain from selected track" }, "S&M_COPYFXCHAIN5", CopyTrackFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Cut FX chain from selected tracks" }, "S&M_COPYFXCHAIN6", CutTrackFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Paste (replace) FX chain to selected tracks" }, "S&M_COPYFXCHAIN7", SetTrackFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Paste FX chain to selected items" }, "S&M_COPYFXCHAIN8", PasteTakeFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Paste FX chain to selected items, all takes" }, "S&M_COPYFXCHAIN9", PasteAllTakesFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Paste FX chain to selected tracks" }, "S&M_COPYFXCHAIN10", PasteTrackFXChain, NULL, }, 

	{ { DEFACCEL, "SWS/S&M: Copy input FX chain from selected track" }, "S&M_COPY_INFXCHAIN", CopyTrackInputFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Cut input FX chain from selected tracks" }, "S&M_CUT_INFXCHAIN", CutTrackInputFXChain, NULL, },
	{ { DEFACCEL, "SWS/S&M: Paste (replace) input FX chain to selected tracks" }, "S&M_PASTE_REPLACE_INFXCHAIN", SetTrackInputFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Paste input FX chain to selected tracks" }, "S&M_PASTE_INFXCHAIN", PasteTrackInputFXChain, NULL, }, 

	{ { DEFACCEL, "SWS/S&M: Clear FX chain for selected items" },  "S&M_CLRFXCHAIN1", ClearActiveTakeFXChain, NULL, },
	{ { DEFACCEL, "SWS/S&M: Clear FX chain for selected items, all takes" }, "S&M_CLRFXCHAIN2", ClearAllTakesFXChain, NULL, },
	{ { DEFACCEL, "SWS/S&M: Clear FX chain for selected tracks" }, "S&M_CLRFXCHAIN3", ClearTrackFXChain, NULL, },
	{ { DEFACCEL, "SWS/S&M: Clear input FX chain for selected tracks" }, "S&M_CLR_INFXCHAIN", ClearTrackInputFXChain, NULL, },

	{ { DEFACCEL, "SWS/S&M: Copy FX chain (depending on focus)" }, "S&M_SMART_CPY_FXCHAIN", SmartCopyFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Paste FX chain (depending on focus)" }, "S&M_SMART_PST_FXCHAIN", SmartPasteFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Paste (replace) FX chain (depending on focus)" }, "S&M_SMART_SET_FXCHAIN", SmartPasteReplaceFXChain, NULL, }, 
	{ { DEFACCEL, "SWS/S&M: Cut FX chain (depending on focus)" }, "S&M_SMART_CUT_FXCHAIN", SmartCutFXChain, NULL, }, 

	{ { DEFACCEL, "SWS/S&M: Paste (replace) FX chain to selected tracks, prompt for slot" }, "S&M_TRACKFXCHAINp1", LoadSetTrackFXChain, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Paste FX chain to selected tracks, prompt for slot" }, "S&M_PASTE_TRACKFXCHAINp1", LoadPasteTrackFXChain, NULL, -1},

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
	{ { DEFACCEL, "SWS/S&M: Open/select project template, prompt for slot" }, "S&M_APPLY_PRJTEMPLATEp", LoadOrSelectProjectSlot, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Open/select project template (new tab), prompt for slot" }, "S&M_NEWTAB_PRJTEMPLATEp", LoadOrSelectProjectTabSlot, NULL, -1},

	{ { DEFACCEL, "SWS/S&M: Project loader/selecter: configuration" }, "S&M_PRJ_LOADER_CONF", ProjectLoaderConf, NULL, },
	{ { DEFACCEL, "SWS/S&M: Project loader/selecter: next (cycle)" }, "S&M_PRJ_LOADER_NEXT", LoadOrSelectNextPreviousProject, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Project loader/selecter: previous (cycle)" }, "S&M_PRJ_LOADER_PREV", LoadOrSelectNextPreviousProject, NULL, -1},

	{ { DEFACCEL, "SWS/S&M: Open project path in explorer/finder" }, "S&M_OPEN_PRJ_PATH", OpenProjectPathInExplorerFinder, NULL, },

	{ { DEFACCEL, "SWS/S&M: Insert silence (seconds)" }, "S&M_INSERT_SILENCE_S", InsertSilence, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Insert silence (measures.beats)" }, "S&M_INSERT_SILENCE_MB", InsertSilence, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Insert silence (samples)" }, "S&M_INSERT_SILENCE_SMP", InsertSilence, NULL, 2},
	
	// Media file slots -------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Open/close Resources window (media files)" }, "S&M_SHOW_RESVIEW_MEDIA", OpenResourceView, NULL, SNM_SLOT_MEDIA, IsResourceViewDisplayed},
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
	{ { DEFACCEL, "SWS/S&M: Open/close Resources window (images)" }, "S&M_SHOW_RESVIEW_IMAGE", OpenResourceView, NULL, SNM_SLOT_IMG, IsResourceViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Clear image slot..." }, "S&M_CLR_IMAGE_SLOT", ResViewClearSlotPrompt, NULL, SNM_SLOT_IMG},
	{ { DEFACCEL, "SWS/S&M: Delete all image slots" }, "S&M_DEL_ALL_IMAGE_SLOT", ResViewDeleteAllSlots, NULL, SNM_SLOT_IMG},
	{ { DEFACCEL, "SWS/S&M: Show image, prompt for slot" }, "S&M_SHOW_IMAGEp", ShowImageSlot, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Set track icon for selected tracks, prompt for slot" }, "S&M_SET_TRACK_ICONp", SetSelTrackIconSlot, NULL, -1},

	// Theme slots ------------------------------------------------------------
#ifdef _WIN32
	{ { DEFACCEL, "SWS/S&M: Open/close Resources window (themes)" }, "S&M_SHOW_RESVIEW_THEME", OpenResourceView, NULL, SNM_SLOT_THM, IsResourceViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Clear theme slot..." }, "S&M_CLR_THEME_SLOT", ResViewClearSlotPrompt, NULL, SNM_SLOT_THM},
	{ { DEFACCEL, "SWS/S&M: Delete all theme slots" }, "S&M_DEL_ALL_THEME_SLOT", ResViewDeleteAllSlots, NULL, SNM_SLOT_THM},
	{ { DEFACCEL, "SWS/S&M: Load theme, prompt for slot" }, "S&M_LOAD_THEMEp", LoadThemeSlot, NULL, -1},
#endif

	// FX presets -------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Trigger next preset for selected FX of selected tracks" }, "S&M_NEXT_SELFX_PRESET", NextPresetSelTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Trigger previous preset for selected FX of selected tracks" }, "S&M_PREVIOUS_SELFX_PRESET", PrevPresetSelTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Trigger next preset for last touched FX" }, "S&M_NEXT_FOCFX_PRESET", NextPrevPresetLastTouchedFX, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Trigger previous preset for last touched FX" }, "S&M_PREV_FOCFX_PRESET", NextPrevPresetLastTouchedFX, NULL, -1},
	
	// MIDI learn -------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Reassign MIDI learned channels of all FX for selected tracks (prompt)" }, "S&M_ALL_FX_LEARN_CHp", ReassignLearntMIDICh, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Reassign MIDI learned channels of selected FX for selected tracks (prompt)" }, "S&M_SEL_FX_LEARN_CHp", ReassignLearntMIDICh, NULL, -2},
	{ { DEFACCEL, "SWS/S&M: Reassign MIDI learned channels of all FX to input channel for selected tracks" }, "S&M_ALLIN_FX_LEARN_CHp", ReassignLearntMIDICh, NULL, -3},
	
	// Items ------------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Scroll to selected item (no undo)" }, "S&M_SCROLL_ITEM", ScrollToSelItem, NULL, },

	// Takes ------------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Pan active takes of selected items to 100% right" }, "S&M_PAN_TAKES_100R", SetPan, NULL, -100},
	{ { DEFACCEL, "SWS/S&M: Pan active takes of selected items to 100% left" }, "S&M_PAN_TAKES_100L", SetPan, NULL, 100},
	{ { DEFACCEL, "SWS/S&M: Pan active takes of selected items to center" }, "S&M_PAN_TAKES_CENTER", SetPan, NULL, 0},

	{ { DEFACCEL, "SWS/S&M: Copy active take" }, "S&M_COPY_TAKE", CopyCutTake, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Cut active take" }, "S&M_CUT_TAKE", CopyCutTake, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Paste take" }, "S&M_PASTE_TAKE", PasteTake, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Paste take (after active take)" }, "S&M_PASTE_TAKE_AFTER", PasteTake, NULL, 1},

	{ { DEFACCEL, "SWS/S&M: Takes - Clear active takes/items" }, "S&M_CLRTAKE1", ClearTake, NULL, },
	{ { DEFACCEL, "SWS/S&M: Takes - Activate lanes from selected items" }, "S&M_LANETAKE2", ActivateLaneFromSelItem, NULL, },
	{ { DEFACCEL, "SWS/S&M: Takes - Activate lane under mouse cursor" }, "S&M_LANETAKE4", ActivateLaneUnderMouse, NULL, },
	{ { DEFACCEL, "SWS/S&M: Takes - Remove empty takes/items among selected items" }, "S&M_DELEMPTYTAKE", RemoveAllEmptyTakes /*RemoveEmptyTakes*/, NULL, },
	{ { DEFACCEL, "SWS/S&M: Takes - Remove empty MIDI takes/items among selected items" }, "S&M_DELEMPTYTAKE2", RemoveEmptyMidiTakes, NULL, },
	{ { DEFACCEL, "SWS/S&M: Takes - Move active up (cycling) in selected items" }, "S&M_MOVETAKE3", MoveActiveTake, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Takes - Move active down (cycling) in selected items" }, "S&M_MOVETAKE4", MoveActiveTake, NULL, 1},

	{ { DEFACCEL, "SWS/S&M: Delete selected items' takes and source files (prompt, no undo)" }, "S&M_DELTAKEANDFILE1", DeleteTakeAndMedia, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Delete selected items' takes and source files (no undo)" }, "S&M_DELTAKEANDFILE2", DeleteTakeAndMedia, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Delete active take and source file in selected items (prompt, no undo)" }, "S&M_DELTAKEANDFILE3", DeleteTakeAndMedia, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Delete active take and source file in selected items (no undo)" }, "S&M_DELTAKEANDFILE4", DeleteTakeAndMedia, NULL, 4},

	// Notes/Subs/Help --------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Open/close Notes window" }, "S&M_SHOW_NOTES_VIEW", OpenNotesHelpView, NULL, -1, IsNotesHelpViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Open/close Notes window (project notes)" }, "S&M_SHOWNOTESHELP", OpenNotesHelpView, NULL, SNM_NOTES_PROJECT, IsNotesHelpViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Open/close Notes window (item notes)" }, "S&M_ITEMNOTES", OpenNotesHelpView, NULL, SNM_NOTES_ITEM, IsNotesHelpViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Open/close Notes window (track notes)" }, "S&M_TRACKNOTES", OpenNotesHelpView, NULL, SNM_NOTES_TRACK, IsNotesHelpViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Open/close Notes window (marker/region names)" }, "S&M_MARKERNAMES", OpenNotesHelpView, NULL, SNM_NOTES_REGION_NAME, IsNotesHelpViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Open/close Notes window (marker/region subtitles)" }, "S&M_MARKERSUBTITLES", OpenNotesHelpView, NULL, SNM_NOTES_REGION_SUBTITLES, IsNotesHelpViewDisplayed},
#ifdef _WIN32
	{ { DEFACCEL, "SWS/S&M: Open/close Notes window (action help)" }, "S&M_ACTIONHELP", OpenNotesHelpView, NULL, SNM_NOTES_ACTION_HELP, IsNotesHelpViewDisplayed},
#endif
	{ { DEFACCEL, "SWS/S&M: Notes - Toggle lock" }, "S&M_ACTIONHELPTGLOCK", ToggleNotesHelpLock, NULL, NULL, IsNotesHelpLocked},
	{ { DEFACCEL, "SWS/S&M: Notes - Set action help file..." }, "S&M_ACTIONHELPPATH", SetActionHelpFilename, NULL, },
	{ { DEFACCEL, "SWS/S&M: Notes - Import subtitle file..." }, "S&M_IMPORT_SUBTITLE", ImportSubTitleFile, NULL, },
	{ { DEFACCEL, "SWS/S&M: Notes - Export subtitle file..." }, "S&M_EXPORT_SUBTITLE", ExportSubTitleFile, NULL, },

	// Split ------------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Split selected items at edit cursor (MIDI) or prior zero crossing (audio)" }, "S&M_SPLIT1", SplitMidiAudio, NULL, },
	{ { DEFACCEL, "SWS/S&M: Split selected items at time selection, edit cursor (MIDI) or prior zero crossing (audio)" }, "S&M_SPLIT2", SmartSplitMidiAudio, NULL, },
	{ { DEFACCEL, "SWS/gofer: Split selected items at mouse cursor (obey snapping)" }, "S&M_SPLIT10", GoferSplitSelectedItems, NULL, },
	{ { DEFACCEL, "SWS/S&M: Split and select items in region near cursor" }, "S&M_SPLIT11", SplitSelectAllItemsInRegion, NULL, },

	// ME ---------------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Active MIDI Editor - Hide all CC lanes" }, "S&M_MEHIDECCLANES", MEHideCCLanes, NULL, },
	{ { DEFACCEL, "SWS/S&M: Active MIDI Editor - Create CC lane" }, "S&M_MECREATECCLANE", MECreateCCLane, NULL, },

	// Tracks -----------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Copy selected track grouping" }, "S&M_COPY_TR_GRP", CopyCutTrackGrouping, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Cut selected tracks grouping" }, "S&M_CUT_TR_GRP", CopyCutTrackGrouping, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Paste grouping to selected tracks" }, "S&M_PASTE_TR_GRP", PasteTrackGrouping, NULL, },
	{ { DEFACCEL, "SWS/S&M: Remove track grouping for selected tracks" }, "S&M_REMOVE_TR_GRP", RemoveTrackGrouping, NULL, },
	{ { DEFACCEL, "SWS/S&M: Set selected tracks to first unused group (default flags)" }, "S&M_SET_TRACK_UNUSEDGROUP", SetTrackToFirstUnusedGroup, NULL, },

	{ { DEFACCEL, "SWS/S&M: Save selected tracks folder states" }, "S&M_SAVEFOLDERSTATE1", SaveTracksFolderStates, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Restore selected tracks folder states" }, "S&M_RESTOREFOLDERSTATE1", RestoreTracksFolderStates, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks folder states to on" }, "S&M_FOLDERON", SetTracksFolderState, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks folder states to off" }, "S&M_FOLDEROFF", SetTracksFolderState, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Save selected tracks folder compact states" }, "S&M_SAVEFOLDERSTATE2", SaveTracksFolderStates, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Restore selected tracks folder compact states" }, "S&M_RESTOREFOLDERSTATE2", RestoreTracksFolderStates, NULL, 1},

	// Take envelopes ---------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Show take volume envelopes" }, "S&M_TAKEENV1", ShowHideTakeVolEnvelope, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Show take pan envelopes" }, "S&M_TAKEENV2", ShowHideTakePanEnvelope, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Show take mute envelopes" }, "S&M_TAKEENV3", ShowHideTakeMuteEnvelope, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Hide take volume envelopes" }, "S&M_TAKEENV4", ShowHideTakeVolEnvelope, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Hide take pan envelopes" }, "S&M_TAKEENV5", ShowHideTakePanEnvelope, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Hide take mute envelopes" }, "S&M_TAKEENV6", ShowHideTakeMuteEnvelope, NULL, 0},

	{ { DEFACCEL, "SWS/S&M: Set active take pan envelopes to 100% right" }, "S&M_TAKEENV_100R", PanTakeEnvelope, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Set active take pan envelopes to 100% left" }, "S&M_TAKEENV_100L", PanTakeEnvelope, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Set active take pan envelopes to center" }, "S&M_TAKEENV_CENTER", PanTakeEnvelope, NULL, 0},

	{ { DEFACCEL, "SWS/S&M: Show take pitch envelope" }, "S&M_TAKEENV10", ShowHideTakePitchEnvelope, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Hide take pitch envelope" }, "S&M_TAKEENV11", ShowHideTakePitchEnvelope, NULL, 0},

	// Track envelopes --------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Remove all envelopes for selected tracks" }, "S&M_REMOVE_ALLENVS", RemoveAllEnvsSelTracks, NULL, },
	{ { DEFACCEL, "SWS/S&M: Toggle arming of all active envelopes for selected tracks" }, "S&M_TGLARMALLENVS", ToggleArmTrackEnv, NULL, 0, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Arm all active envelopes for selected tracks" }, "S&M_ARMALLENVS", ToggleArmTrackEnv, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Disarm all active envelopes for selected tracks" }, "S&M_DISARMALLENVS", ToggleArmTrackEnv, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Toggle arming of volume envelope for selected tracks" }, "S&M_TGLARMVOLENV", ToggleArmTrackEnv, NULL, 3, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle arming of pan envelope for selected tracks" }, "S&M_TGLARMPANENV", ToggleArmTrackEnv, NULL, 4, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle arming of mute envelope for selected tracks" }, "S&M_TGLARMMUTEENV", ToggleArmTrackEnv, NULL, 5, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle arming of all receive volume envelopes for selected tracks" }, "S&M_TGLARMAUXVOLENV", ToggleArmTrackEnv, NULL, 6, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle arming of all receive pan envelopes for selected tracks" }, "S&M_TGLARMAUXPANENV", ToggleArmTrackEnv, NULL, 7, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle arming of all receive mute envelopes for selected tracks" }, "S&M_TGLARMAUXMUTEENV", ToggleArmTrackEnv, NULL, 8, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle arming of all plugin envelopes for selected tracks" }, "S&M_TGLARMPLUGENV", ToggleArmTrackEnv, NULL, 9, FakeIsToggleAction},

	{ { DEFACCEL, "SWS/S&M: Select only track with selected envelope" }, "S&M_SELTR_SELENV", SelOnlyTrackWithSelEnv, NULL, },

	// Toolbar ----------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Toggle toolbars auto refresh enable" },	"S&M_TOOLBAR_REFRESH_ENABLE", EnableToolbarsAutoRefesh, "Enable toolbars auto refresh", 0, IsToolbarsAutoRefeshEnabled},
	{ { DEFACCEL, "SWS/S&M: Toolbar track envelopes in touch/latch/write mode toggle" }, "S&M_TOOLBAR_WRITE_ENV", ToggleWriteEnvExists, NULL, 0, WriteEnvExists},
	{ { DEFACCEL, "SWS/S&M: Toolbar left item selection toggle" }, "S&M_TOOLBAR_ITEM_SEL0", ToggleItemSelExists, NULL, SNM_ITEM_SEL_LEFT, ItemSelExists},
	{ { DEFACCEL, "SWS/S&M: Toolbar right item selection toggle" },"S&M_TOOLBAR_ITEM_SEL1", ToggleItemSelExists, NULL, SNM_ITEM_SEL_RIGHT, ItemSelExists},
#ifdef _WIN32
	{ { DEFACCEL, "SWS/S&M: Toolbar top item selection toggle" }, "S&M_TOOLBAR_ITEM_SEL2", ToggleItemSelExists, NULL, SNM_ITEM_SEL_UP, ItemSelExists},
	{ { DEFACCEL, "SWS/S&M: Toolbar bottom item selection toggle" }, "S&M_TOOLBAR_ITEM_SEL3", ToggleItemSelExists, NULL, SNM_ITEM_SEL_DOWN, ItemSelExists},
#endif

	// Find -------------------------------------------------------------------
	{ { {FCONTROL | FVIRTKEY, 'F', 0 }, "SWS/S&M: Find" }, "S&M_SHOWFIND", OpenFindView, NULL, NULL, IsFindViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Find next" }, "S&M_FIND_NEXT", FindNextPrev, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Find previous" }, "S&M_FIND_PREVIOUS", FindNextPrev, NULL, -1},

	// Live Configs -----------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Open/close Live Configs window" }, "S&M_SHOWMIDILIVE", OpenLiveConfigView, NULL, NULL, IsLiveConfigViewDisplayed},

	// Cyclactions ---------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Open/close Cycle Action editor" }, "S&M_CYCLEDITOR", OpenCyclactionView, NULL, 0, IsCyclactionViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Open/close Cycle Action editor (event list)" }, "S&M_CYCLEDITOR_ME_LIST", OpenCyclactionView, NULL, 1, IsCyclactionViewDisplayed},
	{ { DEFACCEL, "SWS/S&M: Open/close Cycle Action editor (piano roll)" }, "S&M_CYCLEDITOR_ME_PIANO", OpenCyclactionView, NULL, 2, IsCyclactionViewDisplayed},

	// REC inputs -------------------------------------------------------------
	//JFB TODO: configurable dynamic actions *with max* (but ct->user needs to be 0-based first)
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to all channels" }, "S&M_MIDI_INPUT_ALL_CH", SetMIDIInputChannel, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 01" }, "S&M_MIDI_INPUT_CH1", SetMIDIInputChannel, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 02" }, "S&M_MIDI_INPUT_CH2", SetMIDIInputChannel, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 03" }, "S&M_MIDI_INPUT_CH3", SetMIDIInputChannel, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 04" }, "S&M_MIDI_INPUT_CH4", SetMIDIInputChannel, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 05" }, "S&M_MIDI_INPUT_CH5", SetMIDIInputChannel, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 06" }, "S&M_MIDI_INPUT_CH6", SetMIDIInputChannel, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 07" }, "S&M_MIDI_INPUT_CH7", SetMIDIInputChannel, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 08" }, "S&M_MIDI_INPUT_CH8", SetMIDIInputChannel, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 09" }, "S&M_MIDI_INPUT_CH9", SetMIDIInputChannel, NULL, 9},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 10" }, "S&M_MIDI_INPUT_CH10", SetMIDIInputChannel, NULL, 10},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 11" }, "S&M_MIDI_INPUT_CH11", SetMIDIInputChannel, NULL, 11},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 12" }, "S&M_MIDI_INPUT_CH12", SetMIDIInputChannel, NULL, 12},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 13" }, "S&M_MIDI_INPUT_CH13", SetMIDIInputChannel, NULL, 13},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 14" }, "S&M_MIDI_INPUT_CH14", SetMIDIInputChannel, NULL, 14},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 15" }, "S&M_MIDI_INPUT_CH15", SetMIDIInputChannel, NULL, 15},
	{ { DEFACCEL, "SWS/S&M: Set selected tracks MIDI input to channel 16" }, "S&M_MIDI_INPUT_CH16", SetMIDIInputChannel, NULL, 16},

	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to source channel" }, "S&M_MAP_MIDI_INPUT_CH_SRC", RemapMIDIInputChannel, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 01" }, "S&M_MAP_MIDI_INPUT_CH1", RemapMIDIInputChannel, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 02" }, "S&M_MAP_MIDI_INPUT_CH2", RemapMIDIInputChannel, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 03" }, "S&M_MAP_MIDI_INPUT_CH3", RemapMIDIInputChannel, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 04" }, "S&M_MAP_MIDI_INPUT_CH4", RemapMIDIInputChannel, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 05" }, "S&M_MAP_MIDI_INPUT_CH5", RemapMIDIInputChannel, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 06" }, "S&M_MAP_MIDI_INPUT_CH6", RemapMIDIInputChannel, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 07" }, "S&M_MAP_MIDI_INPUT_CH7", RemapMIDIInputChannel, NULL, 7},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 08" }, "S&M_MAP_MIDI_INPUT_CH8", RemapMIDIInputChannel, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 09" }, "S&M_MAP_MIDI_INPUT_CH9", RemapMIDIInputChannel, NULL, 9},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 10" }, "S&M_MAP_MIDI_INPUT_CH10", RemapMIDIInputChannel, NULL, 10},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 11" }, "S&M_MAP_MIDI_INPUT_CH11", RemapMIDIInputChannel, NULL, 11},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 12" }, "S&M_MAP_MIDI_INPUT_CH12", RemapMIDIInputChannel, NULL, 12},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 13" }, "S&M_MAP_MIDI_INPUT_CH13", RemapMIDIInputChannel, NULL, 13},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 14" }, "S&M_MAP_MIDI_INPUT_CH14", RemapMIDIInputChannel, NULL, 14},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 15" }, "S&M_MAP_MIDI_INPUT_CH15", RemapMIDIInputChannel, NULL, 15},
	{ { DEFACCEL, "SWS/S&M: Map selected tracks MIDI input to channel 16" }, "S&M_MAP_MIDI_INPUT_CH16", RemapMIDIInputChannel, NULL, 16},

	// Region playlist --------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Open/close Region Playlist window" }, "S&M_SHOW_RGN_PLAYLIST", OpenRegionPlaylist, NULL, NULL, IsRegionPlaylistDisplayed},
	{ { DEFACCEL, "SWS/S&M: Region Playlist - Play" }, "S&M_PLAY_RGN_PLAYLIST", PlaylistPlay, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Region Playlist - Crop project to playlist" }, "S&M_CROP_RGN_PLAYLIST1", AppendPasteCropPlaylist, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Region Playlist - Crop project to playlist (new project tab)" }, "S&M_CROP_RGN_PLAYLIST2", AppendPasteCropPlaylist, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Region Playlist - Append playlist to project" }, "S&M_APPEND_RGN_PLAYLIST", AppendPasteCropPlaylist, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Region Playlist - Paste playlist at edit cursor" }, "S&M_PASTE_RGN_PLAYLIST", AppendPasteCropPlaylist, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Region Playlist - Set repeat off" }, "S&M_PLAYLIST_REPEAT_ON", SetPlaylistRepeat, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Region Playlist - Set repeat on" }, "S&M_PLAYLIST_REPEAT_OFF", SetPlaylistRepeat, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Region Playlist - Toggle repeat" }, "S&M_PLAYLIST_TGL_REPEAT", SetPlaylistRepeat, NULL, -1, IsPlaylistRepeat},

	// Other, misc ------------------------------------------------------------
	{ { DEFACCEL, "SWS/S&M: Open/close image window" }, "S&M_OPEN_IMAGEVIEW", OpenImageView, NULL, },
	{ { DEFACCEL, "SWS/S&M: Clear image window" }, "S&M_CLR_IMAGEVIEW", ClearImageView, NULL, },
	{ { DEFACCEL, "SWS/S&M: Show next image slot" }, "S&M_SHOW_NEXT_IMG", ShowNextPreviousImageSlot, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Show previous image slot" }, "S&M_SHOW_PREV_IMG", ShowNextPreviousImageSlot, NULL, -1},

	{ { DEFACCEL, "SWS/S&M: Send all notes off to selected tracks" }, "S&M_CC123_SEL_TRACKS", CC123SelTracks, NULL, },
#ifdef _WIN32
	{ { DEFACCEL, "SWS/S&M: What's new?" }, "S&M_WHATSNEW", WhatsNew, NULL, },
	{ { DEFACCEL, "SWS/S&M: Show theme helper (all tracks)" }, "S&M_THEME_HELPER_ALL", ShowThemeHelper, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Show theme helper (selected track)" }, "S&M_THEME_HELPER_SEL", ShowThemeHelper, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Left mouse click at cursor position (use w/o modifier)" }, "S&M_MOUSE_L_CLICK", SimulateMouseClick, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Dump ALR Wiki summary (w/o SWS extension)" }, "S&M_ALRSUMMARY1", DumpWikiActionList, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Dump ALR Wiki summary (SWS extension only)" }, "S&M_ALRSUMMARY2", DumpWikiActionList, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Dump action list (w/o SWS extension)" }, "S&M_DUMP_ACTION_LIST", DumpActionList, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Dump action list (SWS extension only)" }, "S&M_DUMP_SWS_ACTION_LIST", DumpActionList, NULL, 4},
#endif

//!WANT_LOCALIZE_1ST_STRING_END

	// Deprecated, unreleased, etc... -----------------------------------------
#ifdef _SNM_MISC
	// those ones do not focus the main window on cycle
	{ { DEFACCEL, "SWS/S&M: Focus previous floating FX for selected tracks (cycle)" }, "S&M_WNFOCUS1", CycleFocusFXWndSelTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Focus next floating FX for selected tracks (cycle)" }, "S&M_WNFOCUS2", CycleFocusFXWndSelTracks, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Focus previous floating FX (cycle)" }, "S&M_WNFOCUS3", CycleFocusFXWndAllTracks, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Focus next floating FX (cycle)" }, "S&M_WNFOCUS4", CycleFocusFXWndAllTracks, NULL, 1},

	// seems confusing, "SWS/S&M: Takes - Remove empty MIDI takes/items among selected items" is fine..
	{ { DEFACCEL, "SWS/S&M: Takes - Remove all empty takes/items among selected items" }, "S&M_DELEMPTYTAKE3", RemoveAllEmptyTakes, NULL, },

//  Deprecated: contrary to their native versions, the following actions were spliting selected items *and only them*, 
//  see http://forum.cockos.com/showthread.php?t=51547.
//  Due to REAPER v3.67's new native pref `If no items are selected, some split/trim/delete actions affect all items at the edit cursor`, 
//  those actions are less useful: they would still split only selected items, even if that native pref is ticked. 
//  Also removed because of the spam in the action list (many split actions).
	{ { DEFACCEL, "SWS/S&M: Split selected items at play cursor" }, "S&M_SPLIT3", SplitSelectedItems, NULL, 40196},
	{ { DEFACCEL, "SWS/S&M: Split selected items at time selection" }, "S&M_SPLIT4", SplitSelectedItems, NULL, 40061},
	{ { DEFACCEL, "SWS/S&M: Split selected items at edit cursor (no change selection)" }, "S&M_SPLIT5", SplitSelectedItems, NULL, 40757},
	{ { DEFACCEL, "SWS/S&M: Split selected items at time selection (select left)" }, "S&M_SPLIT6", SplitSelectedItems, NULL, 40758},
	{ { DEFACCEL, "SWS/S&M: Split selected items at time selection (select right)" }, "S&M_SPLIT7", SplitSelectedItems, NULL, 40759},
	{ { DEFACCEL, "SWS/S&M: Split selected items at edit or play cursor" }, "S&M_SPLIT8", SplitSelectedItems, NULL, 40012},
	{ { DEFACCEL, "SWS/S&M: Split selected items at edit or play cursor (ignoring grouping)" }, "S&M_SPLIT9", SplitSelectedItems, NULL, 40186},

	// exist natively..
	{ { DEFACCEL, "SWS/S&M: Toggle show take volume envelope" }, "S&M_TAKEENV7", ShowHideTakeVolEnvelope, NULL, -1, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle show take pan envelope" }, "S&M_TAKEENV8", ShowHideTakePanEnvelope, NULL, -1, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Toggle show take mute envelope" }, "S&M_TAKEENV9", ShowHideTakeMuteEnvelope, NULL, -1, FakeIsToggleAction},
#endif

	{ {}, LAST_COMMAND, } // denote end of table
};


///////////////////////////////////////////////////////////////////////////////
// S&M dynamic actions: "dynamic" means that the number of instances of these actions can be customized in the S&M.ini file (section [NbOfActions]).
// The following table must be registered with SNMRegisterDynamicCommands(), in this table:
// - items are not real commands but "meta" commands
// - COMMAND_T.user is used to specify the default number of actions to create
// - COMMAND_T.menuText is used to specify a custom max number of actions (NULL means max = 99, atm)
// - a function doCommand(COMMAND_T*) or getEnabled(COMMAND_T*) will be triggered with 0-based COMMAND_T.user
// - action names are formatted strings, they *must* contain "%02d" (for better sort in the action list, 2 digits because max=99 atm)
// - custom command ids aren't formated strings, but final ids will end with slot numbers (1-based for display reasons)
// Example: 
// { { DEFACCEL, "Do stuff %02d" }, "DO_STUFF", doStuff, NULL, 2}
// if not overrided in the S&M.ini file (e.g. "DO_STUFF=32"), 2 actions will be created: "Do stuff 01" and "Do stuff 02" 
// both calling doStuff(c) with c->user=0 and c->user=1, respectively. 
// custom ids will be "_DO_STUFF1" and "_DO_STUFF2", repectively.
///////////////////////////////////////////////////////////////////////////////

static COMMAND_T g_SNM_dynamicCmdTable[] =
{
//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions

	{ { DEFACCEL, "SWS/S&M: Create cue buss from track selection, settings %02d" }, "S&M_CUEBUS", CueBuss, STR(SNM_MAX_CUE_BUSS_CONFS), SNM_MAX_CUE_BUSS_CONFS},

	{ { DEFACCEL, "SWS/S&M: Set FX %02d online for selected tracks" }, "S&M_FXOFF_SETON", SetFXOnlineSelTracks, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Set FX %02d offline for selected tracks" }, "S&M_FXOFF_SETOFF", SetFXOfflineSelTracks, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Toggle FX %02d online/offline for selected tracks" }, "S&M_FXOFF", ToggleFXOfflineSelTracks, NULL, 8, IsFXOfflineSelTracks},
	{ { DEFACCEL, "SWS/S&M: Bypass FX %02d for selected tracks" }, "S&M_FXBYP_SETON", SetFXBypassSelTracks, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Unbypass FX %02d for selected tracks" }, "S&M_FXBYP_SETOFF", SetFXUnbypassSelTracks, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Toggle FX %02d bypass for selected tracks" }, "S&M_FXBYP", ToggleFXBypassSelTracks, NULL, 8, IsFXBypassedSelTracks},
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except %02d) online/offline for selected tracks" }, "S&M_FXOFFEXCPT", ToggleExceptFXOfflineSelTracks, NULL, 0, FakeIsToggleAction}, // default: none
	{ { DEFACCEL, "SWS/S&M: Toggle all FX (except %02d) bypass for selected tracks" }, "S&M_FXBYPEXCPT", ToggleExceptFXBypassSelTracks, NULL, 0, FakeIsToggleAction}, // default: none

	{ { DEFACCEL, "SWS/S&M: Clear FX chain slot %02d" }, "S&M_CLRFXCHAINSLOT", ResViewClearFXChainSlot, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Paste (replace) FX chain to selected items, slot %02d" }, "S&M_TAKEFXCHAIN", LoadSetTakeFXChain, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Paste FX chain to selected items, slot %02d" }, "S&M_PASTE_TAKEFXCHAIN", LoadPasteTakeFXChain, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Paste (replace) FX chain to selected items, all takes, slot %02d" }, "S&M_FXCHAIN_ALLTAKES", LoadSetAllTakesFXChain, NULL, 0}, // default: none
	{ { DEFACCEL, "SWS/S&M: Paste FX chain to selected items, all takes, slot %02d" }, "S&M_PASTE_FXCHAIN_ALLTAKES", LoadPasteAllTakesFXChain, NULL, 0}, // default: none
	{ { DEFACCEL, "SWS/S&M: Paste (replace) FX chain to selected tracks, slot %02d" }, "S&M_TRACKFXCHAIN", LoadSetTrackFXChain, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Paste FX chain to selected tracks, slot %02d" }, "S&M_PASTE_TRACKFXCHAIN", LoadPasteTrackFXChain, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Paste (replace) input FX chain to selected tracks, slot %02d" }, "S&M_INFXCHAIN", LoadSetTrackInFXChain, NULL, 0}, // default: none
	{ { DEFACCEL, "SWS/S&M: Paste input FX chain to selected tracks, slot %02d" }, "S&M_PASTE_INFXCHAIN", LoadPasteTrackInFXChain, NULL, 0}, // default: none

	{ { DEFACCEL, "SWS/S&M: Clear track template slot %02d" }, "S&M_CLR_TRTEMPLATE_SLOT", ResViewClearTrTemplateSlot, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Apply track template to selected tracks, slot %02d" }, "S&M_APPLY_TRTEMPLATE", LoadApplyTrackTemplateSlot, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Apply track template (+envelopes/items) to selected tracks, slot %02d" }, "S&M_APPLY_TRTEMPLATE_ITEMSENVS", LoadApplyTrackTemplateSlotWithItemsEnvs, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Import tracks from track template, slot %02d" }, "S&M_ADD_TRTEMPLATE", LoadImportTrackTemplateSlot, NULL, 4},

	{ { DEFACCEL, "SWS/S&M: Clear project template slot %02d" }, "S&M_CLR_PRJTEMPLATE_SLOT", ResViewClearPrjTemplateSlot, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Open/select project template, slot %02d" }, "S&M_APPLY_PRJTEMPLATE", LoadOrSelectProjectSlot, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Open/select project template (new tab), slot %02d" }, "S&M_NEWTAB_PRJTEMPLATE", LoadOrSelectProjectTabSlot, NULL, 4},

	{ { DEFACCEL, "SWS/S&M: Clear media file slot %02d" }, "S&M_CLR_MEDIA_SLOT", ResViewClearMediaSlot, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Play media file in selected tracks, slot %02d" }, "S&M_PLAYMEDIA_SELTRACK", PlaySelTrackMediaSlot, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Loop media file in selected tracks, slot %02d" }, "S&M_LOOPMEDIA_SELTRACK", LoopSelTrackMediaSlot, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Play media file in selected tracks (sync with next measure), slot %02d" }, "S&M_PLAYMEDIA_SELTRACK_SYNC", SyncPlaySelTrackMediaSlot, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Loop media file in selected tracks (sync with next measure), slot %02d" }, "S&M_LOOPMEDIA_SELTRACK_SYNC", SyncLoopSelTrackMediaSlot, NULL, 4},

	{ { DEFACCEL, "SWS/S&M: Play media file in selected tracks (toggle), slot %02d" }, "S&M_TGL_PLAYMEDIA_SELTRACK", TogglePlaySelTrackMediaSlot, NULL, 4, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Loop media file in selected tracks (toggle), slot %02d" }, "S&M_TGL_LOOPMEDIA_SELTRACK", ToggleLoopSelTrackMediaSlot, NULL, 4, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Play media file in selected tracks (toggle pause), slot %02d" }, "S&M_TGLPAUSE_PLAYMEDIA_SELTR", TogglePauseSelTrackMediaSlot, NULL, 4, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Loop media file in selected tracks (toggle pause), slot %02d - Infinite looping! To be stopped!" }, "S&M_TGLPAUSE_LOOPMEDIA_SELTR", ToggleLoopPauseSelTrackMediaSlot, NULL, 0, FakeIsToggleAction},

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

	{ { DEFACCEL, "SWS/S&M: Trigger next preset for FX %02d of selected tracks" }, "S&M_NEXT_PRESET_FX", NextPresetSelTracks, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Trigger previous preset for FX %02d of selected tracks" }, "S&M_PREVIOUS_PRESET_FX", PrevPresetSelTracks, NULL, 4},

	{ { DEFACCEL, "SWS/S&M: Select FX %02d for selected tracks" }, "S&M_SELFX", SelectTrackFX, NULL, 8},

	{ { DEFACCEL, "SWS/S&M: Show FX chain for selected tracks, FX %02d" }, "S&M_SHOWFXCHAIN", ShowFXChain, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Float FX %02d for selected tracks" }, "S&M_FLOATFX", FloatFX, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Unfloat FX %02d for selected tracks" }, "S&M_UNFLOATFX", UnfloatFX, NULL, 8},
	{ { DEFACCEL, "SWS/S&M: Toggle float FX %02d for selected tracks" }, "S&M_TOGLFLOATFX", ToggleFloatFX, NULL, 8, FakeIsToggleAction},

	{ { DEFACCEL, "SWS/S&M: Active MIDI Editor - Restore displayed CC lanes, slot %02d" }, "S&M_MESETCCLANES", MESetCCLanes, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Active MIDI Editor - Save displayed CC lanes, slot %02d" }, "S&M_MESAVECCLANES", MESaveCCLanes, NULL, 4},

	{ { DEFACCEL, "SWS/S&M: Set selected tracks to group %02d (default flags)" }, "S&M_SET_TRACK_GROUP", SetTrackGroup, STR(SNM_MAX_TRACK_GROUPS), 8}, // 8 is hard-coded: not all the 32 groups!

	{ { DEFACCEL, "SWS/S&M: Toggle Live Config %02d enable" }, "S&M_TOGGLE_LIVE_CFG", ToggleEnableLiveConfig, STR(SNM_LIVECFG_NB_CONFIGS), 2, IsLiveConfigEnabled},
	{ { DEFACCEL, "SWS/S&M: Live Config %02d - Next" }, "S&M_NEXT_LIVE_CFG", NextLiveConfig, STR(SNM_LIVECFG_NB_CONFIGS), 2},
	{ { DEFACCEL, "SWS/S&M: Live Config %02d - Previous" }, "S&M_PREVIOUS_LIVE_CFG", PreviousLiveConfig, STR(SNM_LIVECFG_NB_CONFIGS), 2},

	{ { DEFACCEL, "SWS/S&M: Region Playlist %02d - Play" }, "S&M_PLAY_RGN_PLAYLIST", PlaylistPlay, NULL, 4},

//!WANT_LOCALIZE_1ST_STRING_END

#ifdef _SNM_MISC
	{ { DEFACCEL, "SWS/S&M: Play media file in selected tracks (toggle, sync with next measure), slot %02d" }, "S&M_TGL_PLAYMEDIA_SELTRACK_SYNC", SyncTogglePlaySelTrackMediaSlot, NULL, 4, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Loop media file in selected tracks (toggle, sync with next measure), slot %02d" }, "S&M_TGL_LOOPMEDIA_SELTRACK_SYNC", SyncToggleLoopSelTrackMediaSlot, NULL, 4, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Play media file in selected tracks (toggle pause, sync with next measure), slot %02d" }, "S&M_TGLPAUSE_PLAYMEDIA_SELTR_SYNC", SyncTogglePauseSelTrackMediaSlot, NULL, 4, FakeIsToggleAction},
	{ { DEFACCEL, "SWS/S&M: Loop media file in selected tracks (toggle pause, sync with next measure), slot %02d - Infinite looping! To be stopped!" }, "S&M_TGLPAUSE_LOOPMEDIA_SELTR_SYNC", SyncToggleLoopPauseSelTrackMediaSlot, NULL, 0, FakeIsToggleAction},
#endif

	{ {}, LAST_COMMAND, }, // denote end of table
};


///////////////////////////////////////////////////////////////////////////////
// "S&M extension" section
///////////////////////////////////////////////////////////////////////////////

//!WANT_LOCALIZE_1ST_STRING_BEGIN:s&m_section_actions
/*JFB static*/ MIDI_COMMAND_T g_SNMSection_cmdTable[] = 
{
	// keep these as first actions (the live configs' learn feature is tied to these cmd ids, staring with SNM_SNM_SECTION_1ST_CMD_ID)
	{ { DEFACCEL, "SWS/S&M: Apply Live Config 1 (MIDI CC absolute only)" }, "S&M_LIVECONFIG1", ApplyLiveConfig, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Apply Live Config 2 (MIDI CC absolute only)" }, "S&M_LIVECONFIG2", ApplyLiveConfig, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Apply Live Config 3 (MIDI CC absolute only)" }, "S&M_LIVECONFIG3", ApplyLiveConfig, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Apply Live Config 4 (MIDI CC absolute only)" }, "S&M_LIVECONFIG4", ApplyLiveConfig, NULL, 3},
	{ { DEFACCEL, "SWS/S&M: Apply Live Config 5 (MIDI CC absolute only)" }, "S&M_LIVECONFIG5", ApplyLiveConfig, NULL, 4},
	{ { DEFACCEL, "SWS/S&M: Apply Live Config 6 (MIDI CC absolute only)" }, "S&M_LIVECONFIG6", ApplyLiveConfig, NULL, 5},
	{ { DEFACCEL, "SWS/S&M: Apply Live Config 7 (MIDI CC absolute only)" }, "S&M_LIVECONFIG7", ApplyLiveConfig, NULL, 6},
	{ { DEFACCEL, "SWS/S&M: Apply Live Config 8 (MIDI CC absolute only)" }, "S&M_LIVECONFIG8", ApplyLiveConfig, NULL, 7},

	{ { DEFACCEL, "SWS/S&M: Select project (MIDI CC absolute only)" }, "S&M_SELECT_PROJECT", SelectProject, NULL, 0},

	{ { DEFACCEL, "SWS/S&M: Trigger preset for selected FX of selected tracks (MIDI CC absolute only)" }, "S&M_SELFX_PRESET", TriggerFXPreset, NULL, -1},
	{ { DEFACCEL, "SWS/S&M: Trigger preset for FX 1 of selected tracks (MIDI CC absolute only)" }, "S&M_PRESET_FX1", TriggerFXPreset, NULL, 0},
	{ { DEFACCEL, "SWS/S&M: Trigger preset for FX 2 of selected tracks (MIDI CC absolute only)" }, "S&M_PRESET_FX2", TriggerFXPreset, NULL, 1},
	{ { DEFACCEL, "SWS/S&M: Trigger preset for FX 3 of selected tracks (MIDI CC absolute only)" }, "S&M_PRESET_FX3", TriggerFXPreset, NULL, 2},
	{ { DEFACCEL, "SWS/S&M: Trigger preset for FX 4 of selected tracks (MIDI CC absolute only)" }, "S&M_PRESET_FX4", TriggerFXPreset, NULL, 3},

	{ {}, LAST_COMMAND, }, // denote end of table
};
//!WANT_LOCALIZE_1ST_STRING_END


///////////////////////////////////////////////////////////////////////////////
// Fake toggle states: used to report toggle states in "best effort mode"
// (example: when an action deals with several selected tracks, the overall
// toggle state might be a mix of different other toggle states)
// note1: actions using a fake toggle state must explicitely call FakeToggle()
// note2: no fake toggle for MIDI_COMMAND_T (only possible in main section atm)
///////////////////////////////////////////////////////////////////////////////

// store fake toggle states, indexed per cmd id (faster + lazy init)
static WDL_IntKeyedArray<bool*> g_fakeToggleStates;

void FakeToggle(COMMAND_T* _ct) {
	if (_ct && _ct->accel.accel.cmd)
		g_fakeToggleStates.Insert(_ct->accel.accel.cmd, *g_fakeToggleStates.Get(_ct->accel.accel.cmd, &g_bFalse) ? &g_bFalse : &g_bTrue);
}

bool FakeIsToggleAction(COMMAND_T* _ct) {
	return (_ct && _ct->accel.accel.cmd && *g_fakeToggleStates.Get(_ct->accel.accel.cmd, &g_bFalse));
}


///////////////////////////////////////////////////////////////////////////////
// Toolbars auto refresh option, see SNM_CSurfRun()
///////////////////////////////////////////////////////////////////////////////

bool g_toolbarsAutoRefreshEnabled = false;
int g_toolbarsAutoRefreshFreq = SNM_DEF_TOOLBAR_RFRSH_FREQ;

void EnableToolbarsAutoRefesh(COMMAND_T* _ct) {
	g_toolbarsAutoRefreshEnabled = !g_toolbarsAutoRefreshEnabled;
}

bool IsToolbarsAutoRefeshEnabled(COMMAND_T* _ct) {
	return g_toolbarsAutoRefreshEnabled;
}

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
static int g_SNMSection_CmdId_gen = SNM_SNM_SECTION_1ST_CMD_ID;

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

/*JFB static*/ KbdSectionInfo g_SNMSection = {
  0x10000101, "S&M Extension",
  g_SNMSection_kbdCmds, 0,
  g_SNMSection_defKeys, 0,
  onAction
};

int SNM_SectionRegisterCommands(reaper_plugin_info_t* _rec, bool _localize)
{
	if (!_rec)
		return 0;

	g_SNMSection.name = __LOCALIZE("S&M Extension","sws_accel_sec");

	// Register commands in specific section
	int i = 0;
	while(g_SNMSection_cmdTable[i].id != LAST_COMMAND)
	{
		MIDI_COMMAND_T* ct = &g_SNMSection_cmdTable[i]; 
		if (ct->doCommand)
		{
/*JFB no more used
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
		g_SNMSection.action_list[i].text = GetLocalizedActionName(ct->accel.desc, 0, "s&m_section_actions");
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


///////////////////////////////////////////////////////////////////////////////
// "Dynamic" actions, see g_SNM_dynamicCmdTable's comments
///////////////////////////////////////////////////////////////////////////////

int SNM_RegisterDynamicCommands(COMMAND_T* _cmds, const char* _inifn)
{
	char actionName[SNM_MAX_ACTION_NAME_LEN]="", custId[SNM_MAX_ACTION_CUSTID_LEN]="";
	int i=0;
	while(_cmds[i].id != LAST_COMMAND)
	{
		COMMAND_T* ct = &_cmds[i++];
		int nb = GetPrivateProfileInt("NbOfActions", ct->id, (int)ct->user, _inifn);
		nb = BOUNDED(nb, 0, ct->menuText == NULL ? SNM_MAX_DYNAMIC_ACTIONS : atoi(ct->menuText));
		for (int j=0; j < nb; j++)
		{
			if (_snprintfStrict(actionName, sizeof(actionName), GetLocalizedActionName(ct->accel.desc, LOCALIZE_FLAG_VERIFY_FMTS), j+1) > 0 &&
				_snprintfStrict(custId, sizeof(custId), "%s%d", ct->id, j+1) > 0)
			{
				if (SWSCreateRegisterDynamicCmd(0, ct->doCommand, ct->getEnabled, custId, actionName, j, __FILE__, false)) // already localized 
					ct->user = nb; // patch the real number of instances
				else
					return 0;
			}
			else
				return 0;
		}
	}
	return 1;
}

void SNM_SaveDynamicCommands(COMMAND_T* _cmds, const char* _inifn)
{
	WDL_FastString iniSection, str;
	iniSection.Set("; Set the number of actions you want below. Quit REAPER first!\n");
	iniSection.AppendFormatted(256, 
		"; Unless specified (e.g. \"[n <= 32!]\"), the maximum number of actions is %d. To hide/remove actions: 0.\n", 
		SNM_MAX_DYNAMIC_ACTIONS);

	WDL_String nameStr; // no fast string here: the buffer gets mangeled..
	int i=0;
	while(_cmds[i].id != LAST_COMMAND)
	{
		COMMAND_T* ct = &_cmds[i++];
		nameStr.Set(SWS_CMD_SHORTNAME(ct));
		Replace02d(nameStr.Get(), 'n');
		if (ct->menuText != NULL) // is a specific max value defined?
		{
			nameStr.Append(" [n <= ");
			nameStr.Append(ct->menuText);
			nameStr.Append("!]");
		}

		// indent things (a \t solution would suck here!)
		str.SetFormatted(BUFFER_SIZE, "%s=%d", ct->id, (int)ct->user);
		while (str.GetLength() < 40) str.Append(" ");
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
WDL_FastString g_SNMDiffToolFn;
int g_SNMIniFileVersion = 0;
int g_SNMbeta = 0;

void IniFileInit()
{
	g_SNMIniFn.SetFormatted(BUFFER_SIZE, SNM_FORMATED_INI_FILE, GetResourcePath());
	g_SNMCyclactionIniFn.SetFormatted(BUFFER_SIZE, SNM_CYCLACTION_INI_FILE, GetResourcePath());

	// move from old location if needed/possible
	WDL_String fn; // no fast string here: the buffer gets mangeled..
	fn.SetFormatted(BUFFER_SIZE, SNM_OLD_FORMATED_INI_FILE, GetExePath());
	if (FileExists(fn.Get()))
		MoveFile(fn.Get(), g_SNMIniFn.Get()); // no check: use the new file whatever happens

	// ini files upgrade, if needed
	SNM_UpgradeIniFiles();

	// load general prefs
	g_SNMMediaFlags |= (GetPrivateProfileInt("General", "MediaFileLockAudio", 0, g_SNMIniFn.Get()) ? 1:0);
	g_toolbarsAutoRefreshEnabled = (GetPrivateProfileInt("General", "ToolbarsAutoRefresh", 1, g_SNMIniFn.Get()) == 1);
	g_toolbarsAutoRefreshFreq = BOUNDED(GetPrivateProfileInt("General", "ToolbarsAutoRefreshFreq", SNM_DEF_TOOLBAR_RFRSH_FREQ, g_SNMIniFn.Get()), 100, 5000);
	g_buggyPlugSupport = GetPrivateProfileInt("General", "BuggyPlugsSupport", 0, g_SNMIniFn.Get());
#ifdef _WIN32
	g_SNMClearType = (GetPrivateProfileInt("General", "ClearTypeFont", 0, g_SNMIniFn.Get()) == 1);
	fn.SetLen(BUFFER_SIZE);
	GetPrivateProfileString("General", "DiffTool", "", fn.Get(), BUFFER_SIZE, g_SNMIniFn.Get());
	g_SNMDiffToolFn.Set(fn.Get());
#endif
//	g_SNMbeta = GetPrivateProfileInt("General", "Beta", 0, g_SNMIniFn.Get());
}

void IniFileExit()
{
	// save general prefs & info
	WDL_FastString iniSection;
	iniSection.AppendFormatted(128, "; REAPER v%s\n", GetAppVersion()); 
	iniSection.AppendFormatted(128, "; SWS/S&M Extension v%d.%d.%d Build %d\n; ", SWS_VERSION); 
	iniSection.Append(g_SNMIniFn.Get()); 
	iniSection.AppendFormatted(128, "\nIniFileUpgrade=%d\n", g_SNMIniFileVersion); 
	iniSection.AppendFormatted(128, "MediaFileLockAudio=%d\n", g_SNMMediaFlags&1 ? 1:0); 
	iniSection.AppendFormatted(128, "ToolbarsAutoRefresh=%d\n", g_toolbarsAutoRefreshEnabled ? 1:0); 
	iniSection.AppendFormatted(128, "ToolbarsAutoRefreshFreq=%d ; in ms (min: 100, max: 5000)\n", g_toolbarsAutoRefreshFreq);
	iniSection.AppendFormatted(128, "BuggyPlugsSupport=%d\n", g_buggyPlugSupport ? 1:0);
#ifdef _WIN32
	iniSection.AppendFormatted(128, "ClearTypeFont=%d\n", g_SNMClearType ? 1:0);
	iniSection.AppendFormatted(BUFFER_SIZE, "DiffTool=\"%s\"\n", g_SNMDiffToolFn.Get());
#endif
//	iniSection.AppendFormatted(128, "Beta=%d\n", g_SNMbeta); 
	SaveIniSection("General", &iniSection, g_SNMIniFn.Get());

	// save dynamic actions
	SNM_SaveDynamicCommands(g_SNM_dynamicCmdTable, g_SNMIniFn.Get());

#ifdef _WIN32
	// force ini file's cache flush, see http://support.microsoft.com/kb/68827
	WritePrivateProfileString(NULL, NULL, NULL, g_SNMIniFn.Get());
#endif
}


///////////////////////////////////////////////////////////////////////////////
// S&M core stuff
///////////////////////////////////////////////////////////////////////////////

bool SNM_HasExtension() {
	WDL_FastString fn;
	fn.SetFormatted(BUFFER_SIZE, SNM_EXTENSION_FILE,
#ifdef _WIN32
		GetExePath());
#else
		GetResourcePath());
#endif
	return FileExists(fn.Get());
}

int SNM_Init(reaper_plugin_info_t* _rec)
{
	if (!_rec)
		return 0;

	IniFileInit();

	// actions must be registered before views
	if (!SWSRegisterCommands(g_SNM_cmdTable) || 
		!SNM_RegisterDynamicCommands(g_SNM_dynamicCmdTable, g_SNMIniFn.Get()) ||
		!SNM_SectionRegisterCommands(_rec, true))
		return 0;

	SNM_UIInit();
	LiveConfigViewInit();
	ResourceViewInit();
	NotesHelpViewInit();
	FindViewInit();
	ImageViewInit();
	RegionPlaylistInit();
	CyclactionInit(); // keep it as the last one!

	return 1;
}

void SNM_Exit()
{
	LiveConfigViewExit();
	ResourceViewExit();
	NotesHelpViewExit();
	FindViewExit();
	ImageViewExit();
	RegionPlaylistExit();
	CyclactionExit();
	SNM_UIExit();
	IniFileExit();
}


///////////////////////////////////////////////////////////////////////////////
// SNM_ScheduledJob, see SNM_CSurfRun()
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
// SNM_MarkerRegionSubscriber, see SNM_CSurfRun()
///////////////////////////////////////////////////////////////////////////////

WDL_PtrList<SNM_MarkerRegionSubscriber> g_mkrRgnSubscribers;
SWS_Mutex g_mkrRgnSubscribersMutex;
WDL_PtrList<MarkerRegion> g_mkrRgnCache;

bool RegisterToMarkerRegionUpdates(SNM_MarkerRegionSubscriber* _sub) {
	if (_sub) {
		SWS_SectionLock lock(&g_mkrRgnSubscribersMutex);
		if (g_mkrRgnSubscribers.Find(_sub) < 0)
			return (g_mkrRgnSubscribers.Add(_sub) != NULL);
	}
	return false;
}

bool UnregisterToMarkerRegionUpdates(SNM_MarkerRegionSubscriber* _sub) {
	if (_sub) {
		SWS_SectionLock lock(&g_mkrRgnSubscribersMutex);
		int idx = g_mkrRgnSubscribers.Find(_sub);
		if (idx >= 0) {
			g_mkrRgnSubscribers.Delete(idx, false);
			return true;
		}
	}
	return false;
}

// returns an update mask: 0 if nothing changed, &SNM_MARKER_MASK: marker change, &SNM_REGION_MASK: region change
int UpdateMarkerRegionCache()
{
	int updateFlags=0;
	int i=0, x=0, num, col; double pos, rgnend; char* name; bool isRgn;

	// added/updated markers/regions?
	while (x = EnumProjectMarkers3(NULL, x, &isRgn, &pos, &rgnend, &name, &num, &col))
	{
		MarkerRegion* m = g_mkrRgnCache.Get(i);
		if (!m || (m && !m->Compare(isRgn, pos, rgnend, name, num, col))) {
			if (m) g_mkrRgnCache.Delete(i);
			g_mkrRgnCache.Insert(i, new MarkerRegion(isRgn, pos, rgnend, name, num, col));
			updateFlags |= (isRgn ? SNM_REGION_MASK : SNM_MARKER_MASK);
		}
		i++;
	}
	// removed markers/regions?
	for (int j=g_mkrRgnCache.GetSize()-1; j>=i; j--) {
		if (MarkerRegion* m = g_mkrRgnCache.Get(j))
			updateFlags |= (m->IsRegion() ? SNM_REGION_MASK : SNM_MARKER_MASK);
		g_mkrRgnCache.Delete(j, true);
	}
	// project time mode update?
	static int prevTimemode = *(int*)GetConfigVar("projtimemode");
	if (updateFlags != (SNM_MARKER_MASK|SNM_REGION_MASK))
		if (int* timemode = (int*)GetConfigVar("projtimemode"))
			if (*timemode != prevTimemode) {
				prevTimemode = *timemode;
				return SNM_MARKER_MASK|SNM_REGION_MASK;
			}
	return updateFlags;
}


///////////////////////////////////////////////////////////////////////////////
// IReaperControlSurface callbacks
///////////////////////////////////////////////////////////////////////////////

double g_toolbarMsCounter = 0.0;
double g_itemSelToolbarMsCounter = 0.0;
double g_markerRegionNotifyMsCounter = 0.0;

// processing order is important here!
void SNM_CSurfRun()
{
	// region playlist
	PlaylistRun();

	// stop playing track previews if needed
	StopTrackPreviewsRun();

	// perform scheduled jobs
	{
		SWS_SectionLock lock(&g_jobsMutex);
		for (int i=g_jobs.GetSize()-1; i >=0; i--)
		{
			SNM_ScheduledJob* job = g_jobs.Get(i);
			job->m_tick--;
			if (job->m_tick <= 0) {
				job->Perform();
				g_jobs.Delete(i, true);
			}
		}
	}

	// marker/region updates notifications
	g_markerRegionNotifyMsCounter += SNM_CSURF_RUN_TICK_MS;
	if (g_markerRegionNotifyMsCounter > 500.0)
	{
		g_markerRegionNotifyMsCounter = 0.0;

		SWS_SectionLock lock(&g_mkrRgnSubscribersMutex);
		if (int sz=g_mkrRgnSubscribers.GetSize())
			if (int updateFlags = UpdateMarkerRegionCache())
				for (int i=sz-1; i>=0; i--)
					g_mkrRgnSubscribers.Get(i)->NotifyMarkerRegionUpdate(updateFlags);
	}

	// toolbars auto-refresh options
	g_toolbarMsCounter += SNM_CSURF_RUN_TICK_MS;
	g_itemSelToolbarMsCounter += SNM_CSURF_RUN_TICK_MS;

	if (g_itemSelToolbarMsCounter > 1000) { // might be hungry => gentle hard-coded freq
		g_itemSelToolbarMsCounter = 0.0;
		if (g_toolbarsAutoRefreshEnabled) 
			ItemSelToolbarPoll();
	}
	if (g_toolbarMsCounter > g_toolbarsAutoRefreshFreq) {
		g_toolbarMsCounter = 0.0;
		if (g_toolbarsAutoRefreshEnabled) 
			RefreshToolbars();
	}
}

extern SNM_NotesHelpWnd* g_pNotesHelpWnd;
extern SNM_LiveConfigsWnd* g_pLiveConfigsWnd;
extern SNM_RegionPlaylistWnd* g_pRgnPlaylistWnd;

void SNM_CSurfSetTrackTitle() {
	if (g_pNotesHelpWnd) g_pNotesHelpWnd->CSurfSetTrackTitle();
	if (g_pLiveConfigsWnd) g_pLiveConfigsWnd->CSurfSetTrackTitle();
}

void SNM_CSurfSetTrackListChange() {
	if (g_pNotesHelpWnd) g_pNotesHelpWnd->CSurfSetTrackListChange();
	if (g_pLiveConfigsWnd) g_pLiveConfigsWnd->CSurfSetTrackListChange();
	if (g_pRgnPlaylistWnd) g_pRgnPlaylistWnd->CSurfSetTrackListChange();
}

bool g_lastPlayState=false, g_lastPauseState=false, g_lastRecState=false;

void SNM_CSurfSetPlayState(bool _play, bool _pause, bool _rec)
{
	if (g_lastPlayState != _play) {
		if (g_lastPlayState && !_play) PlaylistStopped();
		g_lastPlayState = _play;
	}
	if (g_lastPauseState != _pause) {
		g_lastPauseState = _pause;
	}
	if (g_lastRecState != _rec) {
		g_lastRecState = _rec;
	}
}

int SNM_CSurfExtended(int _call, void* _parm1, void* _parm2, void* _parm3) {
	return 0; // return 0 if unsupported
}

