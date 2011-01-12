/******************************************************************************
/ EditCursor.cpp
/
/ Copyright (c) 2010 Tim Payne (SWS)
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
#include "EditCursor.h"

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

static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "SWS: Undo edit cursor move" },			"SWS_EDITCURUNDO",	UndoEditCursor,		},
	{ { DEFACCEL, "SWS: Redo edit cursor move" },			"SWS_EDITCURREDO",	RedoEditCursor,		},

	{ {}, LAST_COMMAND, }, // Denote end of table
};

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
