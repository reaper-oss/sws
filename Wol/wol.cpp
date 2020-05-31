/******************************************************************************
/ wol.cpp
/
/ Copyright (c) 2014 and later wol
/ http://forum.cockos.com/member.php?u=70153
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

#include "wol.h"
#include "wol_Util.h"
#include "wol_Zoom.h"

#include <WDL/localize/localize.h>

void SelectAllTracksExceptFolderParents(COMMAND_T* ct);

static COMMAND_T g_commandTable[] =
{
//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Zoom
	///////////////////////////////////////////////////////////////////////////////////////////////////
		{ { DEFACCEL, "SWS/wol: Options - Set \"Vertical zoom center\" to \"Track at center of view\"" }, "WOL_SETVZOOMC_TRACKCVIEW", SetVerticalZoomCenter, NULL, 0},
		{ { DEFACCEL, "SWS/wol: Options - Set \"Vertical zoom center\" to \"Top visible track\"" }, "WOL_SETVZOOMC_TOPVISTRACK", SetVerticalZoomCenter, NULL, 1},
		{ { DEFACCEL, "SWS/wol: Options - Set \"Vertical zoom center\" to \"Last selected track\"" }, "WOL_SETVZOOMC_LASTSELTRACK", SetVerticalZoomCenter, NULL, 2},
		{ { DEFACCEL, "SWS/wol: Options - Set \"Vertical zoom center\" to \"Track under mouse cursor\"" }, "WOL_SETVZOOMC_TRACKMOUSECUR", SetVerticalZoomCenter, NULL, 3},

		{ { DEFACCEL, "SWS/wol: Options - Set \"Horizontal zoom center\" to \"Edit cursor or play cursor (default)\"" }, "WOL_SETHZOOMC_EDITPLAYCUR", SetHorizontalZoomCenter, NULL, 0},
		{ { DEFACCEL, "SWS/wol: Options - Set \"Horizontal zoom center\" to \"Edit cursor\"" }, "WOL_SETHZOOMC_EDITCUR", SetHorizontalZoomCenter, NULL, 1},
		{ { DEFACCEL, "SWS/wol: Options - Set \"Horizontal zoom center\" to \"Center of view\"" }, "WOL_SETHZOOMC_CENTERVIEW", SetHorizontalZoomCenter, NULL, 2},
		{ { DEFACCEL, "SWS/wol: Options - Set \"Horizontal zoom center\" to \"Mouse cursor\"" }, "WOL_SETHZOOMC_MOUSECUR", SetHorizontalZoomCenter, NULL, 3},

		{ { DEFACCEL, "SWS/wol: Set selected envelope height to default" }, "WOL_SETSELENVHDEF", SetVerticalZoomSelectedEnvelope, NULL, 0 },
		{ { DEFACCEL, "SWS/wol: Set selected envelope height to minimum" }, "WOL_SETSELENVHMIN", SetVerticalZoomSelectedEnvelope, NULL, 1 },
		{ { DEFACCEL, "SWS/wol: Set selected envelope height to maximum" }, "WOL_SETSELENVHMAX", SetVerticalZoomSelectedEnvelope, NULL, 2 },
		{ { DEFACCEL, "SWS/wol: Adjust selected envelope height (MIDI CC relative/mousewheel)" }, "WOL_ADJSELENVH", NULL, NULL, 0, NULL, 0, AdjustSelectedEnvelopeOrTrackHeight, },
		{ { DEFACCEL, "SWS/wol: Adjust selected envelope height, zoom center to middle arrange (MIDI CC relative/mousewheel)" }, "WOL_ADJSELENVHZCMA", NULL, NULL, 1, NULL, 0, AdjustSelectedEnvelopeOrTrackHeight, },
		{ { DEFACCEL, "SWS/wol: Adjust selected envelope height, zoom center to mouse cursor (MIDI CC relative/mousewheel)" }, "WOL_ADJSELENVHZCMC", NULL, NULL, 2, NULL, 0, AdjustSelectedEnvelopeOrTrackHeight, },
		{ { DEFACCEL, "SWS/wol: Adjust selected envelope or last touched track height (MIDI CC relative/mousewheel)" }, "WOL_ADJSELENVTRH", NULL, NULL, 3, NULL, 0, AdjustSelectedEnvelopeOrTrackHeight, },
		{ { DEFACCEL, "SWS/wol: Adjust selected envelope or last touched track height, zoom center to middle arrange (MIDI CC relative/mousewheel)" }, "WOL_ADJSELENVTRHZCMA", NULL, NULL, 4, NULL, 0, AdjustSelectedEnvelopeOrTrackHeight, },
		{ { DEFACCEL, "SWS/wol: Adjust selected envelope or last touched track height, zoom center to mouse cursor (MIDI CC relative/mousewheel)" }, "WOL_ADJSELENVTRHZCMC", NULL, NULL, 5, NULL, 0, AdjustSelectedEnvelopeOrTrackHeight, },
		{ { DEFACCEL, "SWS/wol: Adjust envelope or track height under mouse cursor (MIDI CC relative/mousewheel)" }, "WOL_ADJENVTRHMOUSE", NULL, NULL, 0, NULL, 0, AdjustEnvelopeOrTrackHeightUnderMouse, },
		{ { DEFACCEL, "SWS/wol: Adjust envelope or track height under mouse cursor, zoom center to mouse cursor (MIDI CC relative/mousewheel)" }, "WOL_ADJENVTRHMOUSEZCMC", NULL, NULL, 2, NULL, 0, AdjustEnvelopeOrTrackHeightUnderMouse, },
		{ { DEFACCEL, "SWS/wol: Toggle enable extended zoom for envelopes in track lane" }, "WOL_TENEXTZENVTRL", ToggleEnableEnvelopesExtendedZoom, NULL, 0, IsEnvelopesExtendedZoomEnabled },
		{ { DEFACCEL, "SWS/wol: Toggle enable envelope overlap for envelopes in track lane" }, "WOL_TENENVOLENVTRL", ToggleEnableEnvelopeOverlap, NULL, 0, IsEnvelopeOverlapEnabled },
		{ { DEFACCEL, "SWS/wol: Force overlap for selected envelope in track lane in its track height" }, "WOL_FENVOL", ForceEnvelopeOverlap, NULL, 0 },
		{ { DEFACCEL, "SWS/wol: Restore previous envelope overlap settings" }, "WOL_RESENVOLSET", ForceEnvelopeOverlap, NULL, 1 },
		{ { DEFACCEL, "SWS/wol: Horizontal zoom selected envelope in time selection" }, "WOL_HZOOMSELENVTIMESEL", ZoomSelectedEnvelopeTimeSelection, NULL, 0 },
		{ { DEFACCEL, "SWS/wol: Full zoom selected envelope in time selection" }, "WOL_FZOOMSELENVTIMESEL", ZoomSelectedEnvelopeTimeSelection, NULL, 1 },
		{ { DEFACCEL, "SWS/wol: Full zoom selected envelope (in media lane only) to upper half in time selection" }, "WOL_FZOOMSELENVUPHTIMESEL", ZoomSelectedEnvelopeTimeSelection, NULL, 2 },
		{ { DEFACCEL, "SWS/wol: Full zoom selected envelope (in media lane only) to lower half in time selection" }, "WOL_FZOOMSELENVLOHTIMESEL", ZoomSelectedEnvelopeTimeSelection, NULL, 3 },
		{ { DEFACCEL, "SWS/wol: Vertical zoom selected envelope (in media lane only) to upper half" }, "WOL_VZOOMSELENVUPH", VerticalZoomSelectedEnvelopeLoUpHalf, NULL, 0 },
		{ { DEFACCEL, "SWS/wol: Vertical zoom selected envelope (in media lane only) to lower half" }, "WOL_VZOOMSELENVLOH", VerticalZoomSelectedEnvelopeLoUpHalf, NULL, 1 },

		{ { DEFACCEL, "SWS/wol: Save height of selected envelope, slot 1" }, "WOL_SAVEHSELENVSLOT1", SaveApplyHeightSelectedEnvelopeSlot, NULL, 0 },
		{ { DEFACCEL, "SWS/wol: Save height of selected envelope, slot 2" }, "WOL_SAVEHSELENVSLOT2", SaveApplyHeightSelectedEnvelopeSlot, NULL, 1 },
		{ { DEFACCEL, "SWS/wol: Save height of selected envelope, slot 3" }, "WOL_SAVEHSELENVSLOT3", SaveApplyHeightSelectedEnvelopeSlot, NULL, 2 },
		{ { DEFACCEL, "SWS/wol: Save height of selected envelope, slot 4" }, "WOL_SAVEHSELENVSLOT4", SaveApplyHeightSelectedEnvelopeSlot, NULL, 3 },
		{ { DEFACCEL, "SWS/wol: Save height of selected envelope, slot 5" }, "WOL_SAVEHSELENVSLOT5", SaveApplyHeightSelectedEnvelopeSlot, NULL, 4 },
		{ { DEFACCEL, "SWS/wol: Save height of selected envelope, slot 6" }, "WOL_SAVEHSELENVSLOT6", SaveApplyHeightSelectedEnvelopeSlot, NULL, 5 },
		{ { DEFACCEL, "SWS/wol: Save height of selected envelope, slot 7" }, "WOL_SAVEHSELENVSLOT7", SaveApplyHeightSelectedEnvelopeSlot, NULL, 6 },
		{ { DEFACCEL, "SWS/wol: Save height of selected envelope, slot 8" }, "WOL_SAVEHSELENVSLOT8", SaveApplyHeightSelectedEnvelopeSlot, NULL, 7 },
		{ { DEFACCEL, "SWS/wol: Apply height to selected envelope, slot 1" }, "WOL_APPHSELENVSLOT1", SaveApplyHeightSelectedEnvelopeSlot, NULL, 8 },
		{ { DEFACCEL, "SWS/wol: Apply height to selected envelope, slot 2" }, "WOL_APPHSELENVSLOT2", SaveApplyHeightSelectedEnvelopeSlot, NULL, 9 },
		{ { DEFACCEL, "SWS/wol: Apply height to selected envelope, slot 3" }, "WOL_APPHSELENVSLOT3", SaveApplyHeightSelectedEnvelopeSlot, NULL, 10 },
		{ { DEFACCEL, "SWS/wol: Apply height to selected envelope, slot 4" }, "WOL_APPHSELENVSLOT4", SaveApplyHeightSelectedEnvelopeSlot, NULL, 11 },
		{ { DEFACCEL, "SWS/wol: Apply height to selected envelope, slot 5" }, "WOL_APPHSELENVSLOT5", SaveApplyHeightSelectedEnvelopeSlot, NULL, 12 },
		{ { DEFACCEL, "SWS/wol: Apply height to selected envelope, slot 6" }, "WOL_APPHSELENVSLOT6", SaveApplyHeightSelectedEnvelopeSlot, NULL, 13 },
		{ { DEFACCEL, "SWS/wol: Apply height to selected envelope, slot 7" }, "WOL_APPHSELENVSLOT7", SaveApplyHeightSelectedEnvelopeSlot, NULL, 14 },
		{ { DEFACCEL, "SWS/wol: Apply height to selected envelope, slot 8" }, "WOL_APPHSELENVSLOT8", SaveApplyHeightSelectedEnvelopeSlot, NULL, 15 },


	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Track
	///////////////////////////////////////////////////////////////////////////////////////////////////
		{ { DEFACCEL, "SWS/wol: Select all tracks except folder parents" }, "WOL_SELTREXCFOLDPAR", SelectAllTracksExceptFolderParents, NULL },

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// Envelope
	///////////////////////////////////////////////////////////////////////////////////////////////////
		{ { DEFACCEL, "SWS/wol: Put selected envelope in media lane" }, "WOL_PUTSELENVINMEDIALANE", PutSelectedEnvelopeInLane, NULL, 0 },
		{ { DEFACCEL, "SWS/wol: Put selected envelope in envelope lane" }, "WOL_PUTSELENVINENVLANE", PutSelectedEnvelopeInLane, NULL, 1 },


//!WANT_LOCALIZE_1ST_STRING_END

		{ {}, LAST_COMMAND, },
};



void SelectAllTracksExceptFolderParents(COMMAND_T* ct)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		GetSetMediaTrackInfo(tr, "I_SELECTED", *(int*)GetSetMediaTrackInfo(tr, "I_FOLDERDEPTH", NULL) == 1 ? &g_i0 : &g_i1);
	}
	Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG, -1);
}



int WOL_Init()
{
	SWSRegisterCommands(g_commandTable);
	wol_ZoomInit();
	return 1;
}
