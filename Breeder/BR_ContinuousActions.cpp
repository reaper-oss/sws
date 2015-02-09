/******************************************************************************
/ BR_ContinuousActions.cpp
/
/ Copyright (c) 2014-2015 Dominik Martin Drzic
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
const int ACTION_FLAG      = -666;
const int TOOLTIP_X_OFFSET = 30;

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
static int                              g_continuousCmdLo  = -1;
static int                              g_continuousCmdHi  = -1;

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

static void SetTooltip (const char* text, POINT* p, RECT* bounds)
{
	if (text)
	{
		if (!g_tooltipBm)
			g_tooltipBm = new (nothrow) LICE_SysBitmap();

		if (g_tooltipBm)
		{
			#ifdef _WIN32
				static int s_yOffset = 0;
			#else
				static int s_yOffset = -27;
			#endif

			RECT r;
			r.left = p->x + TOOLTIP_X_OFFSET;
			r.top  = p->y + s_yOffset;

			bool init = false;
			if (!g_tooltipWnd)
			{
				g_tooltipWnd = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_BR_TOOLTIP), g_hwndParent, TooltipWnd);
				SetWindowLongPtr(g_tooltipWnd,GWL_STYLE,GetWindowLongPtr(g_tooltipWnd,GWL_STYLE)&~WS_CAPTION);
				EnableWindow(g_tooltipWnd, false);				
				init = true;
			}
			else
			{
				DrawTooltip(g_tooltipBm, text);
			}

			DrawTooltip(g_tooltipBm, text);
			r.bottom = r.top  + g_tooltipBm->getHeight();
			r.right  = r.left + g_tooltipBm->getWidth();
			if (bounds)
			{
				// Move tooltip to left from mouse cursor if needed
				if (r.right > bounds->right)
				{
					r.right = p->x - TOOLTIP_X_OFFSET;
					r.left = r.right - g_tooltipBm->getWidth();
				}
				BoundToRect(*bounds, &r);
			}

			SetWindowPos(g_tooltipWnd, (init ? HWND_TOPMOST : 0), r.left, r.top, (r.right - r.left), (r.bottom - r.top), (SWP_NOACTIVATE | (init ? 0 : SWP_NOZORDER)));
			if (init) ShowWindow(g_tooltipWnd, SW_SHOWNOACTIVATE);
			else      InvalidateRect(g_tooltipWnd, NULL, TRUE);
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
		RECT r = {0,0,0,0};
		WDL_FastString tooltip = g_actionInProgress->SetTooltip(BR_ContinuousAction::ARRANGE, r);
		if (tooltip.GetLength())
		{
			POINT p = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			ClientToScreen(hwnd, &p);
			SetTooltip(tooltip.Get(), &p, (AreAllCoordsZero(r) ? NULL : &r));
		}
		else
		{
			SetTooltip(NULL, NULL, NULL);
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
		RECT r = {0,0,0,0};
		WDL_FastString tooltip = g_actionInProgress->SetTooltip(BR_ContinuousAction::RULER, r);
		if (tooltip.GetLength())
		{
			POINT p = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			ClientToScreen(hwnd, &p);
			SetTooltip(tooltip.Get(), &p, (AreAllCoordsZero(r) ? NULL : &r));
		}
		else
		{
			SetTooltip(NULL, NULL, NULL);
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
		TrackEnvelope* envelope = GetSelectedEnvelope(NULL);
		SetCursorContext(envelope ? 2 : 1, envelope);

		// Make sure tooltip is not displayed if mouse is over another window (tooltip only follows mouse movements in arrange/ruler)
		if (g_actionInProgress->SetTooltip)
		{
			POINT p;
			GetCursorPos(&p);
			HWND hwnd = WindowFromPoint(p);
			if (hwnd != GetArrangeWnd() && hwnd != GetRulerWnd() && hwnd != g_tooltipWnd)
				SetTooltip(NULL, NULL, NULL);
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
						RECT r = {0,0,0,0};
						WDL_FastString tooltip = g_actionInProgress->SetTooltip(BR_ContinuousAction::ARRANGE, r);
						if (tooltip.GetLength())
						{
							POINT p; GetCursorPos(&p);
							SetTooltip(tooltip.Get(), &p, (AreAllCoordsZero(r) ? NULL : &r));
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
						RECT r = {0,0,0,0};
						WDL_FastString tooltip = g_actionInProgress->SetTooltip(BR_ContinuousAction::RULER, r);
						if (tooltip.GetLength())
						{
							POINT p; GetCursorPos(&p);
							SetTooltip(tooltip.Get(), &p, (AreAllCoordsZero(r) ? NULL : &r));
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

		SetTooltip(NULL, NULL, NULL);

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
		g_continuousCmdLo = g_actions.Get(0)->cmd;
		g_continuousCmdHi = g_actions.Get(g_actions.GetSize() - 1)->cmd;
		return true;
	}
	else
		return false;
}

void ContinuousActionStopAll ()
{
	if (g_actionInProgress && g_actionInProgress->DoUndo)
	{
		int undoFlag = g_actionInProgress->DoUndo();
		if (undoFlag != 0)
			Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(SWSGetCommandByID(g_actionInProgress->cmd)), undoFlag, -1);
	}
	ContinuousActionInit(false, 0, NULL);
}

int  ContinuousActionTooltips ()
{
	return g_tooltips;
}

bool ContinuousActionHook (int cmd, int flag)
{
	if (cmd >= g_continuousCmdLo && cmd <= g_continuousCmdHi) // instead of searching the list every time, first check if cmd is even within range of the list
	{
		BR_ContinuousAction key(cmd, NULL, NULL, NULL, NULL);
		int id = g_actions.FindSorted(&key, &CompareActionsByCmd);
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
	}
	return false;
}