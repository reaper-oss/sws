/******************************************************************************
/ wol.cpp
/
/ Copyright (c) 2014 wol
/ http://forum.cockos.com/member.php?u=70153
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
#include "wol_Zoom.h"
#include "../reaper/localize.h"

void SelectAllTracksExceptFolderParents(COMMAND_T* ct);

static COMMAND_T g_commandTable[] =
{
//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
		{ { DEFACCEL, "SWS/wol: Set \"Vertical zoom center\" to \"Track at center of view\"" }, "WOL_SETVZOOMC_TRACKCVIEW", SetVerticalZoomCenter, NULL, 0},
		{ { DEFACCEL, "SWS/wol: Set \"Vertical zoom center\" to \"Top visible track\"" }, "WOL_SETVZOOMC_TOPVISTRACK", SetVerticalZoomCenter, NULL, 1},
		{ { DEFACCEL, "SWS/wol: Set \"Vertical zoom center\" to \"Last selected track\"" }, "WOL_SETVZOOMC_LASTSELTRACK", SetVerticalZoomCenter, NULL, 2},
		{ { DEFACCEL, "SWS/wol: Set \"Vertical zoom center\" to \"Track under mouse cursor\"" }, "WOL_SETVZOOMC_TRACKMOUSECUR", SetVerticalZoomCenter, NULL, 3},

		{ { DEFACCEL, "SWS/wol: Set \"Horizontal zoom center\" to \"Edit cursor or play cursor (default)\"" }, "WOL_SETHZOOMC_EDITPLAYCUR", SetHorizontalZoomCenter, NULL, 0},
		{ { DEFACCEL, "SWS/wol: Set \"Horizontal zoom center\" to \"Edit cursor\"" }, "WOL_SETHZOOMC_EDITCUR", SetHorizontalZoomCenter, NULL, 1},
		{ { DEFACCEL, "SWS/wol: Set \"Horizontal zoom center\" to \"Center of view\"" }, "WOL_SETHZOOMC_CENTERVIEW", SetHorizontalZoomCenter, NULL, 2},
		{ { DEFACCEL, "SWS/wol: Set \"Horizontal zoom center\" to \"Mouse cursor\"" }, "WOL_SETHZOOMC_MOUSECUR", SetHorizontalZoomCenter, NULL, 3},

		{ { DEFACCEL, "SWS/wol: Set selected envelope height to default" }, "WOL_SETSELENVHDEF", SetVerticalZoomSelectedEnvelope, NULL, 0 },
		{ { DEFACCEL, "SWS/wol: Set selected envelope height to minimum" }, "WOL_SETSELENVHMIN", SetVerticalZoomSelectedEnvelope, NULL, 1 },
		{ { DEFACCEL, "SWS/wol: Set selected envelope height to maximum" }, "WOL_SETSELENVHMAX", SetVerticalZoomSelectedEnvelope, NULL, 2 },
		{ { DEFACCEL, "SWS/wol: Adjust selected envelope height (MIDI CC relative/mousewheel)" }, "WOL_ADJSELENVH", NULL, NULL, 0, NULL, 0, AdjustSelectedEnvelopeOrTrackHeight, },
		{ { DEFACCEL, "SWS/wol: Adjust selected envelope or last touched track height (MIDI CC relative/mousewheel)" }, "WOL_ADJSELENVTRH", NULL, NULL, 1, NULL, 0, AdjustSelectedEnvelopeOrTrackHeight, },
		{ { DEFACCEL, "SWS/wol: Toggle enable extended zoom for envelopes in track lane" }, "WOL_TENEXTZENVTRL", ToggleEnableEnvelopesExtendedZoom, NULL, 0, IsEnvelopesExtendedZoomEnabled },
		{ { DEFACCEL, "SWS/wol: Toggle enable envelope overlap for envelopes in track lane" }, "WOL_TENENVOLENVTRL", ToggleEnableEnvelopeOverlap, NULL, 0, IsEnvelopeOverlapEnabled },
		{ { DEFACCEL, "SWS/wol: Force overlap for selected envelope in track lane in its track height" }, "WOL_FENVOL", ForceEnvelopeOverlap, NULL, 0 },
		{ { DEFACCEL, "SWS/wol: Restore previous envelope overlap settings" }, "WOL_RESENVOLSET", ForceEnvelopeOverlap, NULL, 1 },
		{ { DEFACCEL, "SWS/wol: Horizontal zoom to selected envelope in time selection" }, "WOL_HZOOMSELENVTIMESEL", ZoomSelectedEnvelopeTimeSelection, NULL, 0 },
		{ { DEFACCEL, "SWS/wol: Full zoom to selected envelope in time selection" }, "WOL_FZOOMSELENVTIMESEL", ZoomSelectedEnvelopeTimeSelection, NULL, 1 },

		{ { DEFACCEL, "SWS/wol: Select all tracks except folder parents" }, "WOL_SELTREXCFOLDPAR", SelectAllTracksExceptFolderParents, NULL },
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