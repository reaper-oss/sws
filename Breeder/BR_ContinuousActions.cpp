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
#include "../../WDL/lice/lice.h"

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
static HWND                             g_tooltipWnd       = NULL;
static LICE_SysBitmap*                  g_tooltipBm        = NULL;
static int                              g_tooltips         = -1;

/******************************************************************************
* Tooltip and mouse cursor mechanism                                          *
******************************************************************************/
static WDL_DLGRET TooltipWnd(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_PAINT:
		{
			if (g_tooltipBm)
			{
				PAINTSTRUCT ps;
				HDC dc = BeginPaint(hwnd, &ps);
				BitBlt(dc, 0, 0, g_tooltipBm->getWidth(), g_tooltipBm->getHeight(), g_tooltipBm->getDC(), 0, 0, SRCCOPY);
				EndPaint(hwnd, &ps);
				return 0;
			}
		}
		break;

		case WM_ERASEBKGND:
		{
			return TRUE;
		}
		break;
	}

	return 0;
}

static void SetTooltip (const char* text, POINT* p)
{
	#ifdef _WIN32
		static int yOffset = 0;
	#else
		static int yOffset = -27;
	#endif

	if (text)
	{
		if (!g_tooltipBm)
			g_tooltipBm = new (nothrow) LICE_SysBitmap();

		if (g_tooltipBm)
		{
			if (!g_tooltipWnd)
			{
				g_tooltipWnd = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_BR_TOOLTIP), g_hwndParent, TooltipWnd);
				SetWindowLongPtr(g_tooltipWnd,GWL_STYLE,GetWindowLongPtr(g_tooltipWnd,GWL_STYLE)&~WS_CAPTION);

				EnableWindow(g_tooltipWnd, false);
				DrawTooltip(g_tooltipBm, text);
				SetWindowPos(g_tooltipWnd, HWND_TOPMOST, p->x + 30, p->y + yOffset, g_tooltipBm->getWidth(), g_tooltipBm->getHeight(), SWP_NOACTIVATE);
				ShowWindow(g_tooltipWnd, SW_SHOWNOACTIVATE);
			}
			else
			{
				DrawTooltip(g_tooltipBm, text);
				SetWindowPos(g_tooltipWnd, 0, p->x + 30, p->y + yOffset, g_tooltipBm->getWidth(), g_tooltipBm->getHeight(), SWP_NOACTIVATE|SWP_NOZORDER);
				InvalidateRect(g_tooltipWnd, NULL, TRUE);
			}
		}
	}
	else
	{
		DestroyWindow(g_tooltipWnd);
		delete g_tooltipBm;
		g_tooltipWnd = NULL;
		g_tooltipBm  = NULL;
	}
}

static LRESULT CALLBACK ArrangeWndProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	if (uMsg == WM_SETCURSOR && g_actionInProgress && g_actionInProgress->SetMouseCursor)
	{
		if (HCURSOR cursor = g_actionInProgress->SetMouseCursor(BR_ContinuousAction::ARRANGE))
		{
			SetCursor(cursor);
			return 1; // without this SetCursor won't work on OSX
		}
	}
	else if (uMsg == WM_MOUSEMOVE && g_actionInProgress && g_actionInProgress->SetTooltip)
	{
		WDL_FastString tooltip = g_actionInProgress->SetTooltip(BR_ContinuousAction::ARRANGE);
		if (tooltip.GetLength())
		{
			POINT p = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			ClientToScreen(hwnd, &p);
			SetTooltip(tooltip.Get(), &p);
		}
		else
		{
			SetTooltip(NULL, NULL);
		}
	}

	return g_arrangeWndProc(hwnd, uMsg, wParam, lParam);
}

static LRESULT CALLBACK RulerWndProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_SETCURSOR && g_actionInProgress && g_actionInProgress->SetMouseCursor)
	{
		if (HCURSOR cursor = g_actionInProgress->SetMouseCursor(BR_ContinuousAction::RULER))
		{
			SetCursor(cursor);
			return 1;
		}
	}
	else if (uMsg == WM_MOUSEMOVE && g_actionInProgress && g_actionInProgress->SetTooltip)
	{
		WDL_FastString tooltip = g_actionInProgress->SetTooltip(BR_ContinuousAction::RULER);
		if (tooltip.GetLength())
		{
			POINT p = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			ClientToScreen(hwnd, &p);
			SetTooltip(tooltip.Get(), &p);
		}
		else
		{
			SetTooltip(NULL, NULL);
		}
	}
	return g_rulerWndProc(hwnd, uMsg, wParam, lParam);
}

/******************************************************************************
* Continuous actions internals                                                *
******************************************************************************/
static int CompareActionsByCmd (const BR_ContinuousAction** action1, const BR_ContinuousAction** action2)
{
	return (*action1)->cmd - (*action2)->cmd;
}

static void ContinuousActionTimer ()
{
	if (g_actionInProgress && g_actionInProgress->cmd)
	{
		// Don't let other windows steal our keyboard accelerator
		SetFocus(GetArrangeWnd());

		// Make sure tooltip is not displayed if mouse is over another window (tooltip only follows mouse movements in arrange/ruler)
		if (g_actionInProgress->SetTooltip)
		{
			POINT p;
			GetCursorPos(&p);
			HWND hwnd = WindowFromPoint(p);
			if (hwnd != GetArrangeWnd() && hwnd != GetRulerWnd())
				SetTooltip(NULL, NULL);
		}
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

	bool initSuccessful = true;
	if (init)
	{
		GetConfig("tooltips", g_tooltips);
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
			if (g_actionInProgress->SetMouseCursor || g_actionInProgress->SetTooltip)
			{
				SetConfig("tooltips", SetBit(SetBit(SetBit(g_tooltips, 0), 1), 2));

				if (!g_arrangeWndProc && GetArrangeWnd())
				{
					if (g_actionInProgress->SetTooltip)
					{
						WDL_FastString tooltip = g_actionInProgress->SetTooltip(BR_ContinuousAction::ARRANGE);
						if (tooltip.GetLength())
						{
							POINT p; GetCursorPos(&p);
							SetTooltip(tooltip.Get(), &p);
						}
					}
					g_arrangeWndProc = (WNDPROC)SetWindowLongPtr(GetArrangeWnd(), GWLP_WNDPROC, (LONG_PTR)ArrangeWndProc);
					if (g_actionInProgress->SetMouseCursor && g_actionInProgress->SetMouseCursor(BR_ContinuousAction::ARRANGE))
						SendMessage(GetArrangeWnd(), WM_SETCURSOR, (WPARAM)GetArrangeWnd(), 0);
					InvalidateRect(GetArrangeWnd(), NULL, TRUE); // kill existing native tooltip, if any
				}

				if (!g_rulerWndProc && GetRulerWndAlt())
				{
					if (g_actionInProgress->SetTooltip)
					{
						WDL_FastString tooltip = g_actionInProgress->SetTooltip(BR_ContinuousAction::RULER);
						if (tooltip.GetLength())
						{
							POINT p; GetCursorPos(&p);
							SetTooltip(tooltip.Get(), &p);
						}
					}

					g_rulerWndProc = (WNDPROC)SetWindowLongPtr(GetRulerWndAlt(), GWLP_WNDPROC, (LONG_PTR)RulerWndProc);
					if (g_actionInProgress->SetMouseCursor && g_actionInProgress->SetMouseCursor(BR_ContinuousAction::RULER))
						SendMessage(GetRulerWndAlt(), WM_SETCURSOR, (WPARAM)GetRulerWndAlt(), 0);
					InvalidateRect(GetRulerWndAlt(), NULL, TRUE);
				}
			}

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
		SetConfig("tooltips", g_tooltips);

		if (g_arrangeWndProc && GetArrangeWnd())
		{
			SetWindowLongPtr(GetArrangeWnd(), GWLP_WNDPROC, (LONG_PTR)g_arrangeWndProc);
			SendMessage(GetArrangeWnd(), WM_SETCURSOR, (WPARAM)GetArrangeWnd(), 0);
			InvalidateRect(GetArrangeWnd(), NULL, FALSE);
			g_arrangeWndProc = NULL;
		}

		if (g_rulerWndProc && GetRulerWndAlt())
		{
			SetWindowLongPtr(GetRulerWndAlt(), GWLP_WNDPROC, (LONG_PTR)g_rulerWndProc);
			SendMessage(GetRulerWndAlt(), WM_SETCURSOR, (WPARAM)GetRulerWndAlt(), 0);
			InvalidateRect(GetRulerWndAlt(), NULL, FALSE);
			g_rulerWndProc = NULL;
		}

		SetTooltip(NULL, NULL);

		plugin_register("-accelerator", &s_accelerator);
		plugin_register("-timer", (void*)ContinuousActionTimer);

		if (g_actionInProgress->Init)
			g_actionInProgress->Init(false);
		g_actionInProgress = NULL;
	}

	return initSuccessful;
}

/******************************************************************************
* Continuous actions                                                          *
******************************************************************************/
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

int  ContinuousActionTooltips ()
{
	return g_tooltips;
}

bool ContinuousActionHook (int cmd, int flag)
{
	int id = g_actions.FindSorted(&BR_ContinuousAction(cmd, NULL, NULL, NULL, NULL), &CompareActionsByCmd);
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
	return false;
}