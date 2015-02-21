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
static int                              g_continuousCmdLo  = -1;
static int                              g_continuousCmdHi  = -1;

static WNDPROC                          g_arrangeWndProc    = NULL;
static WNDPROC                          g_rulerWndProc      = NULL;
static WNDPROC                          g_notesWndProc      = NULL;
static WNDPROC                          g_pianoWndProc      = NULL;
static HWND                             g_midiEditorWnd     = NULL;
static HWND                             g_notesWnd          = NULL;
static HWND                             g_pianoWnd          = NULL;
static list<HWND>                       g_registeredWindows;

static HWND                             g_tooltipWnd           = NULL;
static LICE_SysBitmap*                  g_tooltipBm            = NULL;
static int                              g_tooltips             = -1;
static bool                             g_removedNativeTooltip = false;

#ifdef _WIN32
static HHOOK g_keyboardHook = NULL;
#else
static HWND g_midiEditorLastFocusedWnd = NULL;
#endif

/******************************************************************************
* BR_ContinuousAction and it's and some functions to deal with it             *
******************************************************************************/
BR_ContinuousAction::BR_ContinuousAction (COMMAND_T* ct, bool (*Init)(bool), int (*DoUndo)(), HCURSOR (*SetMouseCursor)(int), WDL_FastString (*SetTooltip)(int,bool*,RECT*)) :
Init           (Init),
DoUndo         (DoUndo),
SetMouseCursor (SetMouseCursor),
SetTooltip     (SetTooltip),
cmd            ((ct->uniqueSectionId == SECTION_MAIN || ct->uniqueSectionId == SECTION_MAIN_ALT || ct->uniqueSectionId == SECTION_MIDI_EDITOR) ? SWSRegisterCmd(ct, __FILE__) : 0),
section        (ct->uniqueSectionId)
{
}

static int CompareActionsByCmd (const BR_ContinuousAction** action1, const BR_ContinuousAction** action2)
{
	return (*action1)->cmd - (*action2)->cmd;
}

static int FindActionFromCmd (WDL_PtrList<BR_ContinuousAction>* actionList, int cmd)
{
	int a = 0;
	int c = actionList->GetSize();
	while (a != c)
	{
		int b = (a+c)/2;
		int cmp = cmd - actionList->Get(b)->cmd;

		if      (cmp > 0) a = b+1;
		else if (cmp < 0) c = b;
		else
		{
		  return b;
		}
	 }
	return -1;
}

/******************************************************************************
* Tooltip drawing, mouse cursor and key release detection mechanism           *
*                                                                             *
* Tooltip is drawn as topmost windows and other wndProc functions watch for   *
* mouse move messages to move tooltip window.                                 *
* These wndProc functions also change mouse cursor if needed.                 *
*                                                                             *
* Regarding detection of key up messages (which tell us to terminate action   *
* execution), we register keyboard accelerator with REAPER which only works   *
* for the main window.                                                        *
* On windows we also register keyboard process so we can check for key up     *
* messages in all windows (since we can't do that on OSX that means that      *
* while the continuous actions is active, we need to focus arrange to catch   *
* any key up message and reset focus back once the action                     *
* execution has been stopped                                                  *
******************************************************************************/
static WDL_DLGRET TooltipWnd (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

static void SetTooltip (const char* text, POINT* p, bool setToBounds, RECT* bounds)
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
				g_tooltipWnd = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_BR_TOOLTIP), (g_midiEditorWnd ? g_midiEditorWnd : g_hwndParent), TooltipWnd);
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
			if (setToBounds && bounds)
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

			#ifndef _WIN32
				if (init && g_midiEditorWnd)
					SWELL_SetWindowLevel(g_tooltipWnd, 3); // NSFloatingWindowLevel
			#endif
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

static void KillNativeTooltip ()
{
	if (g_actionInProgress && !g_removedNativeTooltip)
	{
		SetConfig("tooltips", SetBit(SetBit(SetBit(g_tooltips, 0), 1), 2));
		for (list<HWND>::const_iterator it = g_registeredWindows.begin(), end = g_registeredWindows.end(); it != end; ++it)
			InvalidateRect(*it,  NULL, TRUE);
		g_removedNativeTooltip = true;
	}
}

static LRESULT CALLBACK GenericWndProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int window      = 0;
	WNDPROC wndProc = NULL;

	if      (hwnd == GetArrangeWnd()) {window = BR_ContinuousAction::ARRANGE;         wndProc = g_arrangeWndProc;}
	else if (hwnd == GetRulerWnd())   {window = BR_ContinuousAction::RULER;           wndProc = g_rulerWndProc;}
	else if (hwnd == g_notesWnd)      {window = BR_ContinuousAction::MIDI_NOTES_VIEW; wndProc = g_notesWndProc;}
	else if (hwnd == g_pianoWnd)      {window = BR_ContinuousAction::MIDI_PIANO;      wndProc = g_pianoWndProc;}

	if (!wndProc)
		return 0;

	if (uMsg == WM_MOUSEMOVE && g_actionInProgress && g_actionInProgress->SetTooltip)
	{
		// Both piano and notes view windows receive this message even when mouse
		// is over notes view so make sure we set tooltip only for window under mouse
		bool drawTooltip = true;
		if (hwnd == g_notesWnd || hwnd == g_pianoWnd)
		{
			POINT p; GetCursorPos(&p);
			if (hwnd != WindowFromPoint(p))
				drawTooltip = false;
		}

		if (drawTooltip)
		{
			bool setToBounds = false; RECT r;
			WDL_FastString tooltip = g_actionInProgress->SetTooltip(window, &setToBounds, &r);
			if (tooltip.GetLength())
			{
				POINT p = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
				ClientToScreen(hwnd, &p);
				SetTooltip(tooltip.Get(), &p, setToBounds, &r);
				KillNativeTooltip();
			}
			else
			{
				SetTooltip(NULL, NULL, false, NULL);
			}
		}
	}
	else if (uMsg == WM_SETCURSOR && g_actionInProgress && g_actionInProgress->SetMouseCursor)
	{
		if (HCURSOR cursor = g_actionInProgress->SetMouseCursor(window))
		{
			SetCursor(cursor);
			return 1; // without this SetCursor won't work on OSX
		}
	}

	return wndProc(hwnd, uMsg, wParam, lParam);
}

static int TranslateAccel (MSG* msg, accelerator_register_t* ctx)
{
	if (msg->message == WM_KEYUP || msg->message == WM_SYSKEYUP)
		ContinuousActionStopAll();

	// On OSX, we focus arrange for the duration of the action so we need to eat all keystrokes because they will get forwarded to arrange and
	// may end up running something that the user didn't want to run in the first place (see ContinuousActionTimer() and ContinuousActionInit())
	#ifndef _WIN32
		if (g_actionInProgress && g_actionInProgress->section == SECTION_MIDI_EDITOR)
			return 1; // eat the keystroke
	#endif
	return 0;
}

#ifdef _WIN32
static LRESULT CALLBACK KeyboardProc (int code, WPARAM wParam, LPARAM lParam)
{
	if (code >= 0 && GetBit((int)lParam, 31) == 1)
		ContinuousActionStopAll();
	return CallNextHookEx(g_keyboardHook, code, wParam, lParam);
}
#endif

/******************************************************************************
* Command timer and init functions that starts and stops the whole show       *
*                                                                             *
* Timer executes the command every so often and it also watches for the focus *
* so we don't loose the keyboard input which we need to watch our to detect   *
* key up messages to stop the action execution                                *
* Init function registers the timer, window procedures, keyboard hooks etc... *
******************************************************************************/
static void ContinuousActionTimer ()
{
	if (g_actionInProgress)
	{
		// Check MIDI editor is still valid
		if (g_actionInProgress->section == SECTION_MIDI_EDITOR && (!g_midiEditorWnd || MIDIEditor_GetMode(g_midiEditorWnd) != 0))
		{
			ContinuousActionStopAll();
			return;
		}

		// Don't let other windows steal our keyboard accelerator (on windows we registered keyboard hook so no need to worry about this, see TranslateAccel() and ContinuousActionInit())
		#ifndef _WIN32
			TrackEnvelope* envelope = GetSelectedEnvelope(NULL);
			SetCursorContext(envelope ? 2 : 1, envelope);
		#endif

		// Make sure tooltip is not displayed if mouse is over unknown window (tooltip can only follow mouse movements in windows which have our window processes registered)
		if (g_actionInProgress->SetTooltip)
		{
			POINT p;
			GetCursorPos(&p);
			HWND hwnd = WindowFromPoint(p);
			if (find(g_registeredWindows.begin(), g_registeredWindows.end(), hwnd) == g_registeredWindows.end() && hwnd != g_tooltipWnd)
				SetTooltip(NULL, NULL, false, NULL);
		}

		// Run the action with our custom flag (ContinuousActionHook() checks for it to know if it should swallow the action in the hook)
		if (g_actionInProgress->section == SECTION_MIDI_EDITOR)
		{
			int relmode = ACTION_FLAG;
			int cmd     = g_actionInProgress->cmd;
			kbd_RunCommandThroughHooks(SectionFromUniqueID(SECTION_MIDI_EDITOR), &cmd, NULL, NULL, &relmode, g_midiEditorWnd);
		}
		else
			Main_OnCommand(g_actionInProgress->cmd, ACTION_FLAG);
	}
}

static bool ContinuousActionInit (bool init, int cmd, HWND hwnd, BR_ContinuousAction* action)
{
	static accelerator_register_t s_accelerator = { TranslateAccel, TRUE, NULL };

	// Stop any running action in progress if needed
	if (init && g_actionInProgress)
		ContinuousActionInit(false, 0, NULL, NULL);

	// Make sure supplied continuous actions and hwnd is valid
	if (!action || (action->section == SECTION_MIDI_EDITOR && MIDIEditor_GetMode(hwnd) != 0))
		init = false;
	bool initSuccessful = init;

	// Try to initialize...in case of failure initSuccessful will be end up false
	if (initSuccessful)
	{
		// Reset variables
		GetConfig("tooltips", g_tooltips);
		g_actionInProgress     = action;
		g_removedNativeTooltip = false;
		g_midiEditorWnd        = (action->section == SECTION_MIDI_EDITOR) ? hwnd                                 : NULL;
		g_notesWnd             = (g_midiEditorWnd)                        ? GetNotesView((void*)g_midiEditorWnd) : NULL;
		g_pianoWnd             = (g_midiEditorWnd)                        ? GetPianoView((void*)g_midiEditorWnd) : NULL;
		g_registeredWindows.clear();
		#ifndef _WIN32
			if (g_midiEditorWnd)
				g_midiEditorLastFocusedWnd = GetFocus();
		#endif

		// Let the action know we're starting
		if (g_actionInProgress->Init)
			initSuccessful = g_actionInProgress->Init(true);

		// Action says everything is ok
		if (initSuccessful)
		{
			// Register window procedures that change mouse cursor and set tooltip
			for (int i = 0; i < 4; ++i)
			{
				WNDPROC* wndProc = NULL;
				HWND    hwnd     = NULL;
				if      (i == 0) {wndProc = &g_arrangeWndProc; hwnd = GetArrangeWnd();}
				else if (i == 1) {wndProc = &g_rulerWndProc;   hwnd = GetRulerWndAlt();}
				else if (i == 2) {wndProc = &g_notesWndProc;   hwnd = g_notesWnd;}
				else if (i == 3) {wndProc = &g_pianoWndProc;   hwnd = g_pianoWnd;}

				if (*wndProc == NULL && hwnd)
				{
					*wndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)GenericWndProc);
					if (g_actionInProgress->SetMouseCursor)
						SendMessage(hwnd, WM_SETCURSOR, (WPARAM)hwnd, 0);
					g_registeredWindows.push_back(hwnd);
				}
			}

			// Register keyboard accelerator that watches for key up messages in main window and timer that will execute the action
			if (!plugin_register("accelerator", &s_accelerator) || !plugin_register("timer", (void*)ContinuousActionTimer))
				initSuccessful = false;

			// On windows we register keyboard hook so we can catch key up messages in all windows so we don't have to worry about focus issues
			#ifdef _WIN32
				if (initSuccessful)
				{
					g_keyboardHook = SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)KeyboardProc, NULL, GetCurrentThreadId());
					if (!g_keyboardHook) initSuccessful = false;
				}
			#endif

			// Make sure tooltip is draw immediately (WM_MOUSEMOVE message won't get sent until the mouse moves again, see GenericWndProc())
			if (g_actionInProgress->SetTooltip)
			{
				POINT p; GetCursorPos(&p);
				SetCursorPos(p.x, p.y);
			}
		}
	}

	// Initialization failed or termination was requested...reset and deregister everything
	if (!initSuccessful)
	{
		// Remove all hooked window procedures
		for (int i = 0; i < 4; ++i)
		{
			WNDPROC* wndProc = NULL;
			HWND    hwnd     = NULL;
			if      (i == 0) {wndProc = &g_arrangeWndProc; hwnd = GetArrangeWnd();}
			else if (i == 1) {wndProc = &g_rulerWndProc;   hwnd = GetRulerWndAlt();}
			else if (i == 2) {wndProc = &g_notesWndProc;   hwnd = g_notesWnd;}
			else if (i == 3) {wndProc = &g_pianoWndProc;   hwnd = g_pianoWnd;}

			if (*wndProc != NULL && hwnd)
			{
				SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)*wndProc);
				SendMessage(hwnd, WM_SETCURSOR, (WPARAM)hwnd, 0);
				InvalidateRect(hwnd, NULL, FALSE);
				*wndProc = NULL;
			}
		}

		// Reset tooltips options in case they were changed and remove our tooltip
		if (g_removedNativeTooltip)
		{
			SetConfig("tooltips", g_tooltips);

			// Make sure native tooltip is draw before hiding our tooltip
			POINT p; GetCursorPos(&p);
			SetCursorPos(p.x, p.y);
		}
		SetTooltip(NULL, NULL, false, NULL);
		g_removedNativeTooltip = false;

		// Deregister timer and keyboard accelerator/hook
		plugin_register("-accelerator", &s_accelerator);
		plugin_register("-timer", (void*)ContinuousActionTimer);
		#ifdef _WIN32
			UnhookWindowsHookEx(g_keyboardHook);
			g_keyboardHook  = NULL;
		#endif

		// Restore focus back in case of MIDI editor on OSX
		#ifndef _WIN32
			if (g_midiEditorLastFocusedWnd)
				SetFocus(g_midiEditorLastFocusedWnd);
			else if (g_actionInProgress && g_actionInProgress->section == SECTION_MIDI_EDITOR)
				SetFocus(g_midiEditorWnd);
		#endif

		// Reset variables
		g_midiEditorWnd = NULL;
		g_notesWnd      = NULL;
		g_pianoWnd      = NULL;
		g_registeredWindows.clear();
		#ifndef _WIN32
			g_midiEditorLastFocusedWnd = NULL;
		#endif

		// Let continuous action know we're done
		if (g_actionInProgress && g_actionInProgress->Init)
			g_actionInProgress->Init(false);
		g_actionInProgress = NULL;
	}

	return initSuccessful;
}

/******************************************************************************
* Functions for usage by continuous actions                                   *
*                                                                             *
* Just one gotcha here - when registering actions, they are stored in a       *
* sorted manner using their cmds so in the action hook we can do 2            *
* comparisons only in case the action is not in the list (otherwise we would  *
* have to search the list for every action called, clogging the action hook   *
* unnecessarily)                                                              *
******************************************************************************/
bool ContinuousActionRegister (BR_ContinuousAction* action)
{
	if (action && action->cmd > 0 && g_actions.FindSorted(action, &CompareActionsByCmd) == -1)
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
	ContinuousActionInit(false, 0, NULL, NULL);
}

int ContinuousActionTooltips ()
{
	return g_tooltips;
}

bool ContinuousActionHook (int cmd, int flag, HWND hwnd)
{
	bool swallow = false;
	if (cmd >= g_continuousCmdLo && cmd <= g_continuousCmdHi) // instead of searching the list every time, first check if cmd is even within range of the list
	{
		// Check if the action is continuous and then let it pass if it was sent for the first time or when run through timer
		if (BR_ContinuousAction* action = g_actions.Get(FindActionFromCmd(&g_actions, cmd)))
			swallow = (g_actionInProgress) ? (flag != ACTION_FLAG || g_actionInProgress->cmd != cmd) : (!ContinuousActionInit(true, cmd, hwnd, action));
	}
	return swallow;
}