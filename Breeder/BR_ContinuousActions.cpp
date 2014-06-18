/******************************************************************************
/ BR_ContinuousActions.cpp
/
/ Copyright (c) 2014 Dominik Martin Drzic
/ http://forum.cockos.com/member.php?u=27094
/ https://code.google.com/p/sws-extension
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
#include "BR_ContinuousActions.h"
#include "BR_Util.h"

/******************************************************************************
* Constants                                                                   *
******************************************************************************/
const int ACTION_FLAG  = -666;

/******************************************************************************
* Globals                                                                     *
******************************************************************************/
static WDL_PtrList<BR_ContinuousAction> g_actions;
static BR_ContinuousAction*             g_actionInProgress = NULL;
static WNDPROC                          g_arrangeWndProc   = NULL;
static WNDPROC                          g_rulerWndProc     = NULL;

/******************************************************************************
* Continuous actions                                                          *
******************************************************************************/
static LRESULT CALLBACK ArrangeWndProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_SETCURSOR && g_actionInProgress && g_actionInProgress->GetCursor)
	{
		if (HCURSOR cursor = g_actionInProgress->GetCursor(BR_ContinuousAction::ARRANGE))
		{
			SetCursor(cursor);
			return 0;
		}
	}
	return g_arrangeWndProc(hwnd, uMsg, wParam, lParam);
}

static LRESULT CALLBACK RulerWndProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_SETCURSOR && g_actionInProgress && g_actionInProgress->GetCursor)
	{
		if (HCURSOR cursor = g_actionInProgress->GetCursor(BR_ContinuousAction::RULER))
		{
			SetCursor(cursor);
			return 0;
		}
	}
	return g_rulerWndProc(hwnd, uMsg, wParam, lParam);
}

static void ContinuousActionTimer ()
{
	if (g_actionInProgress && g_actionInProgress->cmd)
	{
		SetFocus(GetArrangeWnd()); // Don't let other windows steal our keyboard accelerator
		Main_OnCommand(g_actionInProgress->cmd, ACTION_FLAG);
	}
}

static int ContinuousActionTranslateAccel (MSG* msg, accelerator_register_t* ctx)
{
	if (msg->message == WM_KEYUP || msg->message == WM_SYSKEYUP)
		ContinuousActionStopAll();
	return 0;
}

static bool ContinuousActionInit (bool init, int cmd, BR_ContinuousAction* action)
{
	static accelerator_register_t s_accelerator = { ContinuousActionTranslateAccel, TRUE, NULL };
	static int s_tooltip = -1;

	bool initSuccessful = true;
	if (init)
	{
		GetConfig("tooltips", s_tooltip);
		g_actionInProgress = action;
		if (g_actionInProgress)
		{
			if (g_actionInProgress->Init)
				initSuccessful = g_actionInProgress->Init(true);
		}
		else
			initSuccessful = false;

		if (initSuccessful)
		{
			#ifdef _WIN32
				if (g_actionInProgress->GetCursor)
				{
					if (!g_arrangeWndProc && GetArrangeWnd())
					{
						// Kill any envelope tooltips (otherwise they will get stuck when setting our mouse cursor)
						SetConfig("tooltips", SetBit(SetBit(s_tooltip, 0), 2));
						SendMessage(GetArrangeWnd(), WM_SETCURSOR, (WPARAM)GetArrangeWnd(), 0);

						g_arrangeWndProc = (WNDPROC)SetWindowLongPtr(GetArrangeWnd(), GWLP_WNDPROC, (LONG_PTR)ArrangeWndProc);
						if (g_actionInProgress->GetCursor(BR_ContinuousAction::ARRANGE))
							SendMessage(GetArrangeWnd(), WM_SETCURSOR, (WPARAM)GetArrangeWnd(), 0);
					}

					if (!g_rulerWndProc && GetRulerWndAlt())
					{
						g_rulerWndProc = (WNDPROC)SetWindowLongPtr(GetRulerWndAlt(), GWLP_WNDPROC, (LONG_PTR)RulerWndProc);
						if (g_actionInProgress->GetCursor(BR_ContinuousAction::RULER))
							SendMessage(GetRulerWndAlt(), WM_SETCURSOR, (WPARAM)GetRulerWndAlt(), 0);
					}
				}
			#endif

			plugin_register("accelerator", &s_accelerator);
			plugin_register("timer", (void*)ContinuousActionTimer);
		}
		else
		{
			g_actionInProgress = NULL;
		}
	}
	else
	{
		#ifdef _WIN32
			if (g_arrangeWndProc && GetArrangeWnd())
			{
				SetWindowLongPtr(GetArrangeWnd(), GWLP_WNDPROC, (LONG_PTR)g_arrangeWndProc);
				if (g_actionInProgress->GetCursor(BR_ContinuousAction::ARRANGE))
					SendMessage(GetArrangeWnd(), WM_SETCURSOR, (WPARAM)GetArrangeWnd(), 0);
				g_arrangeWndProc = NULL;
			}

			if (g_rulerWndProc && GetRulerWndAlt())
			{
				SetWindowLongPtr(GetRulerWndAlt(), GWLP_WNDPROC, (LONG_PTR)g_rulerWndProc);
				if (g_actionInProgress->GetCursor(BR_ContinuousAction::RULER))
					SendMessage(GetRulerWndAlt(), WM_SETCURSOR, (WPARAM)GetRulerWndAlt(), 0);
				g_rulerWndProc = NULL;
			}
		#endif

		plugin_register("-accelerator", &s_accelerator);
		plugin_register("-timer", (void*)ContinuousActionTimer);
		SetConfig("tooltips", s_tooltip);

		if (g_actionInProgress->Init)
			g_actionInProgress->Init(false);
		g_actionInProgress = NULL;
	}

	return initSuccessful;
}

static int CompareActionsByCmd (const BR_ContinuousAction** action1, const BR_ContinuousAction** action2)
{
	return (*action1)->cmd - (*action2)->cmd;
}

bool ContinuousActionRegister (BR_ContinuousAction* action)
{
	if (action && kbd_getTextFromCmd(action->cmd, NULL) && g_actions.FindSorted(action, &CompareActionsByCmd) == -1)
	{
		g_actions.InsertSorted(action, &CompareActionsByCmd);
		return true;
	}
	else
		return false;
}

void ContinuousActionStopAll ()
{
	if (g_actionInProgress && g_actionInProgress->DoUndo && g_actionInProgress->DoUndo())
			Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(SWSGetCommandByID(g_actionInProgress->cmd)), UNDO_STATE_ALL, -1);
	ContinuousActionInit(false, 0, NULL);
}

bool ContinuousActionHook (int cmd, int flag)
{
	int id = g_actions.FindSorted(&BR_ContinuousAction(cmd, NULL, NULL, NULL), &CompareActionsByCmd);
	if (id != -1)
	{
		if (!g_actionInProgress)
		{
			if (ContinuousActionInit(true, cmd, g_actions.Get(id)))
				return false; // all good, let it execute from the shortcut the first time
			else
				return true;
		}
		else
		{
			if (flag == ACTION_FLAG && g_actionInProgress->cmd == cmd) return false; // continuous action called from timer, let it pass
			else                                                       return true;
		}
	}

	// not continuous action at all, let it pass
	return false;
}