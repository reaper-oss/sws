/******************************************************************************
/ EditCursor.cpp
/
/ Copyright (c) 2010 Tim Payne (SWS)
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
#include "EditCursor.h"
#include "../Breeder/BR_Util.h"

#define SWS_EDITCURSOR_STACK_SIZE 50

static SWSProjConfig<WDL_TypedBuf<double> > g_editCursorStack;
static SWSProjConfig<int> g_editCursorStackPos;

void UndoEditCursor(COMMAND_T*)
{
	int iPrev = *g_editCursorStackPos.Get() - 1;
	if (iPrev < 0)
		iPrev = SWS_EDITCURSOR_STACK_SIZE - 1;
	if (g_editCursorStack.Get()->Get()[iPrev] != -DBL_MAX)
	{
		*g_editCursorStackPos.Get() = iPrev;
		SetEditCurPos(g_editCursorStack.Get()->Get()[iPrev], true, true);
	}
}

void RedoEditCursor(COMMAND_T*)
{
	int iNext = *g_editCursorStackPos.Get() + 1;
	if (iNext >= SWS_EDITCURSOR_STACK_SIZE)
		iNext = 0;
	if (g_editCursorStack.Get()->Get()[iNext] != -DBL_MAX)
	{
		*g_editCursorStackPos.Get() = iNext;
		SetEditCurPos(g_editCursorStack.Get()->Get()[iNext], true, true);
	}
}

void MoveCursorAndSel(COMMAND_T* ct)
{
	double dStart, dEnd, dPos = GetCursorPosition();
	int iEdge; // -1 for left, 1 for right
	GetSet_LoopTimeRange(false, false, &dStart, &dEnd, false);
	if (dStart == dEnd)
	{
		iEdge = (int)ct->user;
		dStart = dEnd = dPos;
	}
	else if (dPos <= dStart)
		iEdge = -1;
	else if (dPos >= dEnd)
		iEdge = 1;
	else if (dPos - dStart < dEnd - dPos)
		iEdge = -1;
	else
		iEdge = 1;

	// Set edit cursor
	dPos = ((int)ct->user == -1) ? GetPrevGridDiv(dPos) : GetNextGridDiv(dPos);
	SetEditCurPos(dPos, true, false);

	// Extend the time sel
	if (iEdge == -1)
		dStart = dPos;
	else
		dEnd = dPos;
	GetSet_LoopTimeRange(true, false, &dStart, &dEnd, false);

}

// ct->user: -1 left, 1 right
void MoveCursorSample(COMMAND_T* ct)
{
	double dPos = GetCursorPosition();

	const ConfigVar<int> projsrate("projsrate");
	if (!projsrate)
		return;

	const double dSrate = *projsrate;
	int64_t iCurSample = static_cast<int64_t>(dPos * dSrate + 0.5);

	if (ct->user == -1 && (dPos == iCurSample / dSrate))
		iCurSample--;
	else if (ct->user == 1)
		iCurSample++;

	const double dNewPos = iCurSample / dSrate;
	SetEditCurPos(dNewPos, true, false);
}

void MoveCursorMs(COMMAND_T* ct)
{
	//double dPos = GetCursorPosition();
	//dPos += (double)ct->user / 1000.0;
	//SetEditCurPos(dPos, true, true);
	// MoveEditCursor will scrub; leaving the old SetEditCur code above in case there are unintended consequences
	MoveEditCursor((double)ct->user / 1000.0, false);
}

void MoveCursorFade(COMMAND_T* ct)
{
	double dPos = GetCursorPosition();
	dPos += fabs(*ConfigVar<double>("deffadelen")) * (double)ct->user; // Abs because neg value means "not auto"
	SetEditCurPos(dPos, true, false);
}

//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] =
{
	{ { DEFACCEL, "SWS: Undo edit cursor move" },						"SWS_EDITCURUNDO",		UndoEditCursor, },
	{ { DEFACCEL, "SWS: Redo edit cursor move" },						"SWS_EDITCURREDO",		RedoEditCursor, },
	{ { DEFACCEL, "SWS: Move cursor and time selection left to grid" },	"SWS_MOVECURSELLEFT",	MoveCursorAndSel, NULL, -1 },
	{ { DEFACCEL, "SWS: Move cursor and time selection right to grid" },"SWS_MOVECURSELRIGHT",	MoveCursorAndSel, NULL,  1 },
	{ { DEFACCEL, "SWS: Move cursor left 1 sample (on grid)" },			"SWS_MOVECURSAMPLEFT",	MoveCursorSample, NULL, -1 },
	{ { DEFACCEL, "SWS: Move cursor right 1 sample (on grid)" },		"SWS_MOVECURSAMPRIGHT",	MoveCursorSample, NULL,  1 },
	{ { DEFACCEL, "SWS: Move cursor left 1ms" },						"SWS_MOVECUR1MSLEFT",	MoveCursorMs,     NULL, -1 },
	{ { DEFACCEL, "SWS: Move cursor right 1ms" },						"SWS_MOVECUR1MSRIGHT",	MoveCursorMs,     NULL,  1 },
	{ { DEFACCEL, "SWS: Move cursor left 5ms" },						"SWS_MOVECUR5MSLEFT",	MoveCursorMs,     NULL, -5 },
	{ { DEFACCEL, "SWS: Move cursor right 5ms" },						"SWS_MOVECUR5MSRIGHT",	MoveCursorMs,     NULL,  5 },
	{ { DEFACCEL, "SWS: Move cursor left by default fade length" },		"SWS_MOVECURFADELEFT",	MoveCursorFade,   NULL, -1 },
	{ { DEFACCEL, "SWS: Move cursor right by default fade length" },	"SWS_MOVECURFADERIGHT",	MoveCursorFade,   NULL,  1 },

	{ {}, LAST_COMMAND, }, // Denote end of table
};
//!WANT_LOCALIZE_1ST_STRING_END

static void BeginLoadProjectState(bool isUndo, struct project_config_extension_t *reg)
{
	g_editCursorStack.Cleanup();
	g_editCursorStackPos.Cleanup();
}

static project_config_extension_t g_projectconfig = { NULL, NULL, BeginLoadProjectState, NULL };

// Save the edit cursor position
void EditCursorSlice()
{
	// Initialize new proj
	if (g_editCursorStack.Get()->GetSize() == 0)
	{
		g_editCursorStack.Get()->Resize(SWS_EDITCURSOR_STACK_SIZE);
		for (int i = 1; i < SWS_EDITCURSOR_STACK_SIZE; i++)
			g_editCursorStack.Get()->Get()[i] = -DBL_MAX;
		*g_editCursorStackPos.Get() = 0;
		*g_editCursorStack.Get()->Get() = GetCursorPosition();
	}

	double dPos = GetCursorPosition();
	if (dPos != g_editCursorStack.Get()->Get()[*g_editCursorStackPos.Get()])
	{
		(*g_editCursorStackPos.Get())++;
		if (*g_editCursorStackPos.Get() >= SWS_EDITCURSOR_STACK_SIZE)
			*g_editCursorStackPos.Get() = 0;

		int iNext = *g_editCursorStackPos.Get();

		while(g_editCursorStack.Get()->Get()[iNext] != -DBL_MAX)
		{
			g_editCursorStack.Get()->Get()[iNext] = -DBL_MAX;
			iNext++;
			if (iNext >= SWS_EDITCURSOR_STACK_SIZE)
				iNext = 0;
		}
		g_editCursorStack.Get()->Get()[*g_editCursorStackPos.Get()] = dPos;
	}
}

int EditCursorInit()
{
	SWSRegisterCommands(g_commandTable);
	if (!plugin_register("projectconfig",&g_projectconfig))
		return 0;
	return 1;
}

void EditCursorExit()
{
	plugin_register("-projectconfig",&g_projectconfig);
}
