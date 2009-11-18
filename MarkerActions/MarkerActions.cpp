/******************************************************************************
/ MarkerActions.cpp
/
/ Copyright (c) 2009 Tim Payne (SWS)
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

static bool g_bMAEnabled = true;

void RunActionMarker(const char* cName)
{
	if (cName && cName[0] == '!')
	{
		LineParser lp(false);
		lp.parse(&cName[1]);
		for (int i = 0; i < lp.getnumtokens(); i++)
			if (lp.gettoken_int(i))
			{
				int iCommand = lp.gettoken_int(i);
				int iZero = 0;
				bool b = kbd_RunCommandThroughHooks(NULL, &iCommand, &iZero, &iZero, &iZero, g_hwndParent);
				if (!b)
				{
					if (KBD_OnMainActionEx)
					{
						KBD_OnMainActionEx(lp.gettoken_int(i), 0, 0, 0, g_hwndParent, NULL);
					}
					else
						Main_OnCommand(lp.gettoken_int(i), 0);
				}
			}
	}
}

void MarkerActionSlice()
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
			char* cName;
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
	WritePrivateProfileString("SWS", "MarkerActionsEnabled", g_bMAEnabled ? "1" : "0", get_ini_file());
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
	char* cName;
	double dMarkerPos;
	double dCurPos = GetCursorPosition();
	// Look for markers with '!' as the first char with the right time
	while ((x = EnumProjectMarkers(x, NULL, &dMarkerPos, NULL, &cName, NULL)))
		if (dMarkerPos == dCurPos)
			RunActionMarker(cName);
}

void MarkerNudge(bool bRight)
{
	// Find the marker underneath the edit cursor
	int x = 0;
	int iIndex;
	double dMarkerPos;
	bool bReg;
	double dCurPos = GetCursorPosition();
	while ((x = EnumProjectMarkers(x, &bReg, &dMarkerPos, NULL, NULL, &iIndex)))
	{
		if (dMarkerPos == dCurPos)
		{
			dCurPos += (bRight ? 1.0 : -1.0) / GetHZoomLevel();
			SetProjectMarker(iIndex, false, dCurPos, 0.0, NULL);
			SetEditCurPos(dCurPos, true, false);
			return;
		}
	}
}

void MarkerNudgeLeft(COMMAND_T*)  { MarkerNudge(false); }
void MarkerNudgeRight(COMMAND_T*) { MarkerNudge(true); }

static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "SWS: Toggle marker actions enable" },    "SWSMA_TOGGLE",		MarkerActionsToggle,		"Enable SWS marker actions", },
	{ { DEFACCEL, "SWS: Enable marker actions" },           "SWSMA_ENABLE",		MarkerActionsEnable,		NULL, },
	{ { DEFACCEL, "SWS: Disable marker actions" },          "SWSMA_DISABLE",	MarkerActionsDisable,		NULL, },
	{ { DEFACCEL, "SWS: Run action marker under cursor" },  "SWSMA_RUNEDIT",	MarkerActionRunUnderCursor,	NULL, },
	{ { DEFACCEL, "SWS: Nudge marker under cursor left" },  "SWS_MNUDGEL",		MarkerNudgeLeft,			NULL, },
	{ { DEFACCEL, "SWS: Nudge marker under cursor right" }, "SWS_MNUDGER",		MarkerNudgeRight,			NULL, },

	{ {}, LAST_COMMAND, }, // Denote end of table
};

static void menuhook(const char* menustr, HMENU hMenu, int flag)
{
	if (flag == 0 && strcmp(menustr, "Main options") == 0)
		AddToMenu(hMenu, g_commandTable[0].menuText, g_commandTable[0].accel.accel.cmd, 40745);
	else if (flag == 1)
		SWSCheckMenuItem(hMenu, g_commandTable[0].accel.accel.cmd, g_bMAEnabled);
}

int MarkerActionsInit()
{
	SWSRegisterCommands(g_commandTable);

	g_bMAEnabled = GetPrivateProfileInt("SWS", "MarkerActionsEnabled", 1, get_ini_file()) ? true : false;

	if (!plugin_register("hookcustommenu", (void*)menuhook))
		return 0;

	return 1;
}
