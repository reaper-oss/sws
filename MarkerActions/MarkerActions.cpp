/******************************************************************************
/ MarkerActions.cpp
/
/ Copyright (c) 2011 Tim Payne (SWS)
/
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

static bool g_bMAEnabled = true;
static bool g_bIgnoreNext = false;
static void RefreshMAToolbar();

void RunActionMarker(const char* cName)
{
	if (cName && cName[0] == '!' && !EnumProjects(0x40000000, NULL, 0)) // disabled while rendering (0x40000000 trick == rendered project, if any)
	{
		if (g_bIgnoreNext)
		{	// Ignore the entire marker action
			g_bIgnoreNext = false;
			return;
		}
		LineParser lp(false);
		lp.parse(&cName[1]);
		for (int i = 0; i < lp.getnumtokens(); i++)
		{
			int iCommand = lp.gettoken_int(i);
			if (!iCommand)
				iCommand = NamedCommandLookup(lp.gettoken_str(i));

			if (iCommand)
			{
				int iZero = 0;
				if (!kbd_RunCommandThroughHooks(NULL, &iCommand, &iZero, &iZero, &iZero, g_hwndParent))
				{
					KBD_OnMainActionEx(iCommand, 0, 0, 0, g_hwndParent, NULL);
				}
			}
		}
	}
}

void MarkerActionTimer()
{
	static double dLastPos = 0.0;
	static double dUsualPosDelta = 0.050;
	if (g_bMAEnabled && GetPlayState() & 1)
	{
		double dPlayPos = GetPlayPosition();
		double dDelta = dPlayPos - dLastPos;
		// Ignore markers when the play state seems to have jumped forward or backwards
		if (dDelta > 0.0 && dDelta < dUsualPosDelta * 5.0)
		{
			// Quick and dirty IIR
			dUsualPosDelta = dUsualPosDelta * 0.99 + dDelta * 0.01;
			int x = 0;
			const char* cName;
			double dMarkerPos;
			// Look for markers with '!' as the first char with the right time
			while ((x = EnumProjectMarkers(x, NULL, &dMarkerPos, NULL, &cName, NULL)))
				if (dMarkerPos >= dLastPos && dMarkerPos < dPlayPos)
					RunActionMarker(cName);
		}
		dLastPos = dPlayPos;
	}
	else
		dLastPos = GetCursorPosition();
}

void MarkerActionsToggle(COMMAND_T* = NULL)
{
	g_bMAEnabled = !g_bMAEnabled;
	if (g_bMAEnabled) plugin_register("timer", (void*)MarkerActionTimer);
	else              plugin_register("-timer",(void*)MarkerActionTimer);
	WritePrivateProfileString("SWS", "MarkerActionsEnabled", g_bMAEnabled ? "1" : "0", get_ini_file());
	RefreshMAToolbar();
}

int MarkerActionsEnabled(COMMAND_T*)
{
	return g_bMAEnabled;
}

void MarkerActionsEnable(COMMAND_T*)
{
	if (!g_bMAEnabled)
		MarkerActionsToggle();
}

void MarkerActionsDisable(COMMAND_T*)
{
	if (g_bMAEnabled)
		MarkerActionsToggle();
}

void MarkerActionRunUnderCursor(COMMAND_T*)
{
	if (!g_bMAEnabled)
		return;

	int x = 0;
	const char* cName;
	double dMarkerPos;
	double dCurPos = GetCursorPosition();
	// Look for markers with '!' as the first char with the right time
	while ((x = EnumProjectMarkers(x, NULL, &dMarkerPos, NULL, &cName, NULL)))
		if (dMarkerPos == dCurPos)
			RunActionMarker(cName);
}

void MarkerActionIgnoreNext(COMMAND_T*)
{
	g_bIgnoreNext = true;
}

void MarkerNudge(bool bRight)
{
	// Find the marker underneath the edit cursor
	int x = 0;
	int iIndex, iColor;
	double dPos, dEnd;
	bool bReg;
	double dCurPos = GetCursorPosition();
	while ((x = EnumProjectMarkers3(NULL, x, &bReg, &dPos, &dEnd, NULL, &iIndex, &iColor)))
	{
		if (dPos == dCurPos)
		{
			dCurPos += (bRight ? 1.0 : -1.0) / GetHZoomLevel();
			if (dCurPos >= 0)
			{
				SetProjectMarkerByIndex(NULL, x-1, bReg, dCurPos, dEnd, iIndex, NULL, iColor);
				SetEditCurPos(dCurPos, true, false);
				return;
			}
		}
	}
}

void MarkerNudgeLeft(COMMAND_T*)  { MarkerNudge(false); }
void MarkerNudgeRight(COMMAND_T*) { MarkerNudge(true); }

void RegionsFromItems(COMMAND_T* ct)
{
	// Ignore the fact that the user may have items selected with the exact same times.  Just blindly create regions!
	WDL_TypedBuf<MediaItem*> items;
	SWS_GetSelectedMediaItems(&items);
	bool bUndo = false;
	for (int i = 0; i < items.GetSize(); i++)
	{
		MediaItem_Take* take = GetActiveTake(items.Get()[i]);
		if (take)
		{
			char* cName = (char*)GetSetMediaItemTakeInfo(take, "P_NAME", NULL);
			double dStart = *(double*)GetSetMediaItemInfo(items.Get()[i], "D_POSITION", NULL);
			double dEnd = *(double*)GetSetMediaItemInfo(items.Get()[i], "D_LENGTH", NULL) + dStart;
			AddProjectMarker(NULL, true, dStart, dEnd, cName, -1);
			bUndo = true;
		}
		else if (!CountTakes(items.Get()[i]))  /* In case of an empty item there is no take so process item instead */
		{
			double dStart = *(double*)GetSetMediaItemInfo(items.Get()[i], "D_POSITION", NULL);
			double dEnd = *(double*)GetSetMediaItemInfo(items.Get()[i], "D_LENGTH", NULL) + dStart;
			AddProjectMarker(NULL, true, dStart, dEnd, NULL, -1);
			bUndo = true;
		}
	}
	if (bUndo)
	{
		UpdateTimeline();
		Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct), UNDO_STATE_MISCCFG, -1);
	}
}

//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] =
{
	{ { DEFACCEL, "SWS: Toggle marker actions enable" },    "SWSMA_TOGGLE",		MarkerActionsToggle,		"Enable SWS marker actions", 0, MarkerActionsEnabled },
	{ { DEFACCEL, "SWS: Enable marker actions" },           "SWSMA_ENABLE",		MarkerActionsEnable,		NULL, },
	{ { DEFACCEL, "SWS: Disable marker actions" },          "SWSMA_DISABLE",	MarkerActionsDisable,		NULL, },
	{ { DEFACCEL, "SWS: Run action marker under cursor" },  "SWSMA_RUNEDIT",	MarkerActionRunUnderCursor,	NULL, },
	{ { DEFACCEL, "SWS: Ignore next marker action" },		"SWSMA_IGNORE",		MarkerActionIgnoreNext,		NULL, },

	// Not sure if these should be in MarkerActions.cpp or MarkerListActions.cpp.  Eh, doesn't matter.
	{ { DEFACCEL, "SWS: Nudge marker under cursor left" },  "SWS_MNUDGEL",		MarkerNudgeLeft,			NULL, },
	{ { DEFACCEL, "SWS: Nudge marker under cursor right" }, "SWS_MNUDGER",		MarkerNudgeRight,			NULL, },
	{ { DEFACCEL, "SWS: Create regions from selected items (name by active take)" }, "SWS_REGIONSFROMITEMS",	RegionsFromItems, NULL, },

	{ {}, LAST_COMMAND, }, // Denote end of table
};
//!WANT_LOCALIZE_1ST_STRING_END

static void RefreshMAToolbar()
{
	RefreshToolbar(g_commandTable[0].cmdId);
}

int MarkerActionsInit()
{
	SWSRegisterCommands(g_commandTable);

	g_bMAEnabled = GetPrivateProfileInt("SWS", "MarkerActionsEnabled", 1, get_ini_file()) ? true : false;
	if (g_bMAEnabled) plugin_register("timer", (void*)MarkerActionTimer);
	return 1;
}

void MarkerActionsExit()
{
	plugin_register("-timer",(void*)MarkerActionTimer);
}

