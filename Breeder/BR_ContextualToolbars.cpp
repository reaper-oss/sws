/******************************************************************************
/ BR_ContextualToolbars.cpp
/
/ Copyright (c) 2014-2015 Dominik Martin Drzic
/ http://forum.cockos.com/member.php?u=27094
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

#include "BR_ContextualToolbars.h"
#include "BR_EnvelopeUtil.h"
#include "BR_MouseUtil.h"
#include "BR_Util.h"
#include "../SnM/SnM_Dlg.h"
#include "../SnM/SnM_Util.h"

#include <WDL/localize/localize.h>

/******************************************************************************
* Constants                                                                   *
******************************************************************************/
const char* const CONTEXT_TOOLBARS_WND      = "BR - ContextualToolbars WndPos" ;
const char* const CONTEXT_TOOLBARS_VIEW_WND = "BR - ContextualToolbarsView WndPos";
const char* const INI_SECTION               = "ContextualToolbars";
const char* const INI_KEY_CONTEXTS          = "ContextsPreset_";
const char* const INI_KEY_OPTIONS           = "SettingsPreset_";
const char* const INI_KEY_AUTOCLOSE         = "AutoClosePreset_";
const char* const INI_KEY_POSITION          = "PositionOffsetPreset_";
const char* const INI_KEY_CURRENT_PRESET    = "DlgPreset";
const int PRESET_COUNT                      = 8;

// !WANT_LOCALIZE_STRINGS_BEGIN:sws_DLG_181
static SWS_LVColumn g_cols[] =
{
	{163, 0, "Context"},
	{105, 0, "Toolbar"},
	{62,  0, "Auto close"},
	{81,  0, "Position offset"},
};
// !WANT_LOCALIZE_STRINGS_END

enum
{
	COL_CONTEXT = 0,
	COL_TOOLBAR,
	COL_AUTOCLOSE,
	COL_POSITION,
	COL_COUNT
};

/******************************************************************************
* Globals                                                                     *
******************************************************************************/
SNM_WindowManager<BR_ContextualToolbarsWnd> g_contextToolbarsWndManager(CONTEXT_TOOLBARS_WND);
static BR_ContextualToolbarsManager         g_toolbarsManager;
static int                                  g_listViewContexts[CONTEXT_COUNT];

/******************************************************************************
* Context toolbars                                                            *
******************************************************************************/
BR_ContextualToolbar::BR_ContextualToolbar () :
m_contexts {}, m_options {}, m_mode (0)
{
	this->UpdateInternals();
}

BR_ContextualToolbar::BR_ContextualToolbar (const BR_ContextualToolbar& contextualToolbar) :
m_options (contextualToolbar.m_options),
m_mode (contextualToolbar.m_mode),
m_activeContexts (contextualToolbar.m_activeContexts)
{
	std::copy(contextualToolbar.m_contexts, contextualToolbar.m_contexts + CONTEXT_COUNT, m_contexts);
}

BR_ContextualToolbar::~BR_ContextualToolbar () = default;

BR_ContextualToolbar& BR_ContextualToolbar::operator= (const BR_ContextualToolbar& contextualToolbar)
{
	if (this == &contextualToolbar)
		return *this;

	std::copy(contextualToolbar.m_contexts, contextualToolbar.m_contexts + CONTEXT_COUNT, m_contexts);
	m_options        = contextualToolbar.m_options;
	m_activeContexts = contextualToolbar.m_activeContexts;
	m_mode           = contextualToolbar.m_mode;
	return *this;
}

bool BR_ContextualToolbar::operator== (const BR_ContextualToolbar& contextualToolbar)
{
	for (int i = CONTEXT_START; i < CONTEXT_COUNT; ++i)
	{
		if (!this->IsContextValid(i))
			continue;
		else
		{
			if (contextualToolbar.m_contexts[i].mouseAction     != m_contexts[i].mouseAction     ||
			    contextualToolbar.m_contexts[i].toggleAction    != m_contexts[i].toggleAction    ||
			    contextualToolbar.m_contexts[i].positionOffsetX != m_contexts[i].positionOffsetX ||
			    contextualToolbar.m_contexts[i].positionOffsetY != m_contexts[i].positionOffsetY ||
			    contextualToolbar.m_contexts[i].autoClose       != m_contexts[i].autoClose)
			  return false;
		}
	}

	return
		contextualToolbar.m_options.focus                  == m_options.focus                  &&
		contextualToolbar.m_options.position               == m_options.position               &&
		contextualToolbar.m_options.setToolbarToForeground == m_options.setToolbarToForeground &&
		contextualToolbar.m_options.autocloseInactive      == m_options.autocloseInactive      &&
		contextualToolbar.m_options.tcpTrack               == m_options.tcpTrack               &&
		contextualToolbar.m_options.tcpEnvelope            == m_options.tcpEnvelope            &&
		contextualToolbar.m_options.mcpTrack               == m_options.mcpTrack               &&
		contextualToolbar.m_options.arrangeTrack           == m_options.arrangeTrack           &&
		contextualToolbar.m_options.arrangeItem            == m_options.arrangeItem            &&
		contextualToolbar.m_options.arrangeTakeEnvelope    == m_options.arrangeTakeEnvelope    &&
		contextualToolbar.m_options.arrangeStretchMarker   == m_options.arrangeStretchMarker   &&
		contextualToolbar.m_options.arrangeTrackEnvelope   == m_options.arrangeTrackEnvelope   &&
		contextualToolbar.m_options.arrangeActTake         == m_options.arrangeActTake         &&
		contextualToolbar.m_options.midiSetCCLane          == m_options.midiSetCCLane          &&
		contextualToolbar.m_options.inlineItem             == m_options.inlineItem             &&
		contextualToolbar.m_options.inlineSetCCLane        == m_options.inlineSetCCLane
	;
}

bool BR_ContextualToolbar::operator!= (const BR_ContextualToolbar& contextualToolbar)
{
	return !(*this == contextualToolbar);
}

void BR_ContextualToolbar::LoadToolbar (bool exclusive)
{
	bool toolbarsOpen = false;
	if (this->AreAssignedToolbarsOpened())
	{
		if (!exclusive)
			this->CloseAllAssignedToolbars();
		toolbarsOpen = true;
	}

	if (!toolbarsOpen || exclusive)
	{
		BR_MouseInfo mouseInfo(m_mode, true);
		ExecuteOnToolbarLoad executeOnToolbarLoad{};

		int context = -1;
		if      (!strcmp(mouseInfo.GetWindow(), "transport"))   context = this->FindTransportToolbar(mouseInfo, executeOnToolbarLoad);
		else if (!strcmp(mouseInfo.GetWindow(), "ruler"))       context = this->FindRulerToolbar(mouseInfo, executeOnToolbarLoad);
		else if (!strcmp(mouseInfo.GetWindow(), "tcp"))         context = this->FindTcpToolbar(mouseInfo, executeOnToolbarLoad);
		else if (!strcmp(mouseInfo.GetWindow(), "mcp"))         context = this->FindMcpToolbar(mouseInfo, executeOnToolbarLoad);
		else if (!strcmp(mouseInfo.GetWindow(), "arrange"))     context = this->FindArrangeToolbar(mouseInfo, executeOnToolbarLoad);
		else if (!strcmp(mouseInfo.GetWindow(), "midi_editor")) context = (mouseInfo.IsInlineMidi()) ? this->FindInlineMidiToolbar(mouseInfo, executeOnToolbarLoad) :  this->FindMidiToolbar(mouseInfo, executeOnToolbarLoad);

		int mouseAction = (context == -1) ? DO_NOTHING : this->GetMouseAction(context);
		if (context == -1 || !this->IsToolbarAction(mouseAction))
		{
			if (exclusive && toolbarsOpen)
				this->CloseAllAssignedToolbars();
		}
		else
		{
			if (exclusive && GetToggleCommandState(this->GetToggleAction(context)))
			{
				this->CloseAllAssignedToolbars();
			}
			else
			{
				if (toolbarsOpen)
					this->CloseAllAssignedToolbars();

				// Fill in missing execution options (these repeat for all contexts so no need to repeat it in every find function)
				executeOnToolbarLoad.autoCloseToolbar       = GetAutoClose(context);
				executeOnToolbarLoad.makeTopMost            = !!(m_options.topmost & OPTION_ENABLED);
				executeOnToolbarLoad.setToolbarToForeground = !!(m_options.setToolbarToForeground & OPTION_ENABLED);
				executeOnToolbarLoad.positionOffsetX        = GetPositionOffsetX(context);
				executeOnToolbarLoad.positionOffsetY        = GetPositionOffsetY(context);
				executeOnToolbarLoad.positionOrientation    = ((m_options.position & OPTION_ENABLED) ? m_options.position : 0);
				executeOnToolbarLoad.positionOverride       = (executeOnToolbarLoad.positionOffsetX != 0 || executeOnToolbarLoad.positionOffsetY != 0 || executeOnToolbarLoad.positionOrientation != 0);

				PreventUIRefresh(1);

				// Execute these options before showing toolbar (otherwise toolbar can end up behind mixer/MIDI editor etc... due to focus changes)
				this->SetCCLaneClicked(executeOnToolbarLoad, mouseInfo);
				this->SetFocus(executeOnToolbarLoad);
				this->SelectEnvelope(executeOnToolbarLoad); // execute after focusing (since focus is tied to envelope selection)

				// Toggle toolbar and set it up if needed
				Main_OnCommand(mouseAction, 0);

				// Find toolbar hwnd to manage (in case toolbar is docked in toolbar docker)
				bool inDocker;
				HWND toolbarHwnd   = this->GetFloatingToolbarHwnd(mouseAction, &inDocker);
				HWND toolbarParent = toolbarHwnd;
				if (inDocker)
				{
					while (toolbarParent && GetParent(toolbarParent) != g_hwndParent)
						toolbarParent = GetParent(toolbarParent);
				}

				// Make sure toolbar is not out of monitor bounds (also reposition if allowed by preferences)
				this->RepositionToolbar(executeOnToolbarLoad, toolbarParent);

				// Set up toolbar window process for various options
				int toggleAction;
				this->GetReaperToolbar(this->GetToolbarId(mouseAction), NULL, &toggleAction, NULL, 0);
				this->SetToolbarWndProc(executeOnToolbarLoad, toolbarHwnd, toggleAction, toolbarParent);

				// Continue executing options
				bool undoTrack = this->SelectTrack(executeOnToolbarLoad);
				bool undoItem  = this->SelectItem(executeOnToolbarLoad);
				bool undoTake  = this->ActivateTake(executeOnToolbarLoad);

				// Create undo point
				const int undoMask = ConfigVar<int>("undomask").value_or(0);
				if (!GetBit(undoMask, 0)) // "Create undo points for item/track selection" is off
				{
					undoTrack = false;
					undoItem  = false;
				}
				if (undoTrack || undoItem || undoTake)
				{
					const char* undoMsg = NULL;
					if      ( undoTrack && !undoItem && !undoTake) undoMsg = __localizeFunc("Change track selection", "undo", 0);
					else if (!undoTrack &&  undoItem && !undoTake) undoMsg = __localizeFunc("Change media item selection", "undo", 0);
					else if (!undoTrack && !undoItem &&  undoTake) undoMsg = __localizeFunc("Change active take", "undo", 0);
					else if ( undoTrack &&  undoItem && !undoTake) undoMsg = __LOCALIZE("Change track and media item selection", "sws_undo");
					else if ( undoTrack && !undoItem &&  undoTake) undoMsg = __LOCALIZE("Change active take and track selection", "sws_undo");
					else if (!undoTrack &&  undoItem &&  undoTake) undoMsg = __LOCALIZE("Change active take and media item selection", "sws_undo");
					else if ( undoTrack &&  undoItem &&  undoTake) undoMsg = __LOCALIZE("Change active take, track and media item selection", "sws_undo");

					if (undoMsg) Undo_OnStateChangeEx2(NULL, undoMsg, UNDO_STATE_TRACKCFG | UNDO_STATE_ITEMS | UNDO_STATE_MISCCFG, -1);
				}

				PreventUIRefresh(-1);
				UpdateArrange();
			}
		}
	}
}

bool BR_ContextualToolbar::AreAssignedToolbarsOpened ()
{
	for (set<int>::iterator it = m_activeContexts.begin(); it != m_activeContexts.end(); ++it)
	{
		int toggleAction = this->GetToggleAction(*it);
		if (this->IsContextValid(*it) && this->IsToolbarAction(toggleAction))
		{
			if (GetToggleCommandState(toggleAction))
				return true;
		}
	}
	return false;
}

int BR_ContextualToolbar::CountToolbars ()
{
	return TOOLBAR_COUNT;
}

int BR_ContextualToolbar::GetToolbarType (int toolbarId)
{
	int mouseAction;
	if (this->GetReaperToolbar(toolbarId, &mouseAction, NULL, NULL, 0))
	{
		if      (mouseAction == DO_NOTHING)          return 0;
		else if (mouseAction == INHERIT_PARENT)      return 1;
		else if (mouseAction == FOLLOW_ITEM_CONTEXT) return 2;
		else                                         return 3;
	}
	return -1;
}

bool BR_ContextualToolbar::GetToolbarName (int toolbarId, char* toolbarName, int toolbarNameSz)
{
	if (this->GetReaperToolbar(toolbarId, NULL, NULL, toolbarName, toolbarNameSz))
		return true;
	else
		return false;
}

bool BR_ContextualToolbar::IsFirstToolbar (int toolbarId)
{
	int mouseAction;
	if (this->GetReaperToolbar(toolbarId, &mouseAction, NULL, NULL, 0))
		return (mouseAction == TOOLBAR_01_MOUSE) ? true : false;
	return false;
}

bool BR_ContextualToolbar::IsFirstMidiToolbar (int toolbarId)
{
	int mouseAction;
	if (this->GetReaperToolbar(toolbarId, &mouseAction, NULL, NULL, 0))
		return (mouseAction == MIDI_TOOLBAR_01_MOUSE) ? true : false;
	return false;
}

bool BR_ContextualToolbar::SetContext (int context, int* toolbarId, bool* autoClose, int* positionOffsetX, int* positionOffsetY)
{
	bool update = false;

	if (this->IsContextValid(context))
	{
		int mouseAction, toggleAction;
		if (toolbarId && this->GetReaperToolbar(*toolbarId, &mouseAction, &toggleAction, NULL, 0))
		{
			bool validToolbarId = true;
			if      (mouseAction == INHERIT_PARENT      && !this->CanContextInheritParent(context)) validToolbarId = false;
			else if (mouseAction == FOLLOW_ITEM_CONTEXT && !this->CanContextFollowItem(context))    validToolbarId = false;

			if (validToolbarId)
			{
				this->SetMouseAction(context, mouseAction);
				this->SetToggleAction(context, toggleAction);
				update = true;
			}
		}

		if (this->IsToolbarAction(this->GetMouseAction(context)))
		{
			if (autoClose)
			{
				SetAutoClose(context, *autoClose);
				update = true;
			}

			if (positionOffsetX || positionOffsetY)
			{
				int x = (positionOffsetX) ? *positionOffsetX : this->GetPositionOffsetX(context);
				int y = (positionOffsetY) ? *positionOffsetY : this->GetPositionOffsetY(context);
				this->SetPositionOffset(context, x, y);
				update = true;
			}
		}

		if (update) this->UpdateInternals();
	}

	return update;
}

bool BR_ContextualToolbar::GetContext (int context, int* toolbarId, bool* autoClose, int* positionOffsetX, int* positionOffsetY)
{
	if (this->IsContextValid(context))
	{
		int mouseAction = this->GetMouseAction(context);

		WritePtr(toolbarId,       this->GetToolbarId(mouseAction));
		WritePtr(autoClose,       this->IsToolbarAction(mouseAction) ? m_contexts[context].autoClose     : false);
		WritePtr(positionOffsetX, this->IsToolbarAction(mouseAction) ? this->GetPositionOffsetX(context) : 0);
		WritePtr(positionOffsetY, this->IsToolbarAction(mouseAction) ? this->GetPositionOffsetY(context) : 0);
		return true;
	}
	return false;
}

bool BR_ContextualToolbar::IsContextValid (int context)
{
	if (context >= CONTEXT_START && context < CONTEXT_COUNT)
	{
		return
			context != END_TRANSPORT   &&
			context != END_RULER       &&
			context != END_TCP         &&
			context != END_MCP         &&
			context != END_ARRANGE     &&
			context != END_MIDI_EDITOR
		;
	}

	return false;
}

bool BR_ContextualToolbar::CanContextInheritParent (int context)
{
	if (this->IsContextValid(context))
	{
		return
			context != TRANSPORT          &&
			context != RULER              &&
			context != TCP                &&
			context != MCP                &&
			context != ARRANGE            &&
			context != MIDI_EDITOR        &&
			context != INLINE_MIDI_EDITOR
		;
	}
	return false;
}

bool BR_ContextualToolbar::CanContextFollowItem (int context)
{
	if (this->IsContextValid(context))
	{
		return
			context == ARRANGE_TRACK_ITEM_STRETCH_MARKER  ||
			context == ARRANGE_TRACK_TAKE_ENVELOPE        ||
			context == ARRANGE_TRACK_TAKE_ENVELOPE_VOLUME ||
			context == ARRANGE_TRACK_TAKE_ENVELOPE_PAN    ||
			context == ARRANGE_TRACK_TAKE_ENVELOPE_MUTE   ||
			context == ARRANGE_TRACK_TAKE_ENVELOPE_PITCH
		;
	}
	return false;
}

void BR_ContextualToolbar::ExportConfig (WDL_FastString& contextToolbars, WDL_FastString& contextAutoClose, WDL_FastString& contextPosition, WDL_FastString& options)
{
	for (int i = CONTEXT_START; i < (CONTEXT_COUNT + REMOVED_CONTEXTS); ++i)
	{
		int context = this->GetContextFromIniIndex(i);
		if (context != UNUSED_CONTEXT && context != -666)
		{
			contextToolbars.AppendFormatted(128,  "%d ",    this->TranslateAction(this->GetMouseAction(context), true));
			contextAutoClose.AppendFormatted(128, "%d ",    m_contexts[context].autoClose ? -1 : 0); // why -1 ? this option may one day get replaced with "action on button press" which could also close the toolbar, so use negative numbers for future proofness
			contextPosition.AppendFormatted(128,  "%d %d ", this->GetPositionOffsetX(context), this->GetPositionOffsetY(context));
		}
		else if (context == UNUSED_CONTEXT)
		{
			contextToolbars.AppendFormatted(128,  "%d ",    this->TranslateAction(DO_NOTHING, true));
			contextAutoClose.AppendFormatted(128, "%d ",    0);
			contextPosition.AppendFormatted(128,  "%d %d ", 0, 0);
		}
	}

	// Remove last " " from strings
	contextToolbars.DeleteSub(contextToolbars.GetLength()   - 1, 1);
	contextAutoClose.DeleteSub(contextAutoClose.GetLength() - 1, 1);
	contextPosition.DeleteSub(contextPosition.GetLength()   - 1, 1);

	options.AppendFormatted(1024,
		"%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
		m_options.focus,
		m_options.topmost,
		m_options.tcpTrack,
		m_options.tcpEnvelope,
		m_options.mcpTrack,
		m_options.arrangeTrack,
		m_options.arrangeItem,
		m_options.arrangeStretchMarker,
		m_options.arrangeTakeEnvelope,
		m_options.arrangeTrackEnvelope,
		m_options.arrangeActTake,
		m_options.midiSetCCLane,
		m_options.inlineItem,
		m_options.inlineSetCCLane,
		m_options.position,
		m_options.setToolbarToForeground,
		m_options.autocloseInactive
	);
}

void BR_ContextualToolbar::ImportConfig (const char* contextToolbars, const char* contextAutoClose, const char* contextPosition, const char* options)
{
	LineParser lpToolbars(false);  lpToolbars.parse(contextToolbars);
	LineParser lpAutoClose(false); lpAutoClose.parse(contextAutoClose);
	LineParser lpPosition(false);  lpPosition.parse(contextPosition);

	for (int i = CONTEXT_START; i < (CONTEXT_COUNT + REMOVED_CONTEXTS); ++i)
	{
		int context = this->GetContextFromIniIndex(i);
		if (context != UNUSED_CONTEXT && context != -666)
		{
			// Load actions for context
			int mouseAction = (lpToolbars.getnumtokens() > i)
			                  ? this->TranslateAction(lpToolbars.gettoken_int(i), false)
			                  : this->CanContextInheritParent(context) ? INHERIT_PARENT : DO_NOTHING;

			if (mouseAction == FOLLOW_ITEM_CONTEXT && !this->CanContextFollowItem(context))
				mouseAction = (this->CanContextInheritParent(context) ? INHERIT_PARENT : DO_NOTHING);
			else if (mouseAction == INHERIT_PARENT      && !this->CanContextInheritParent(context))
				mouseAction = DO_NOTHING;

			int toggleAction;
			this->GetReaperToolbar(this->GetToolbarId(mouseAction), NULL, &toggleAction, NULL, 0);

			this->SetMouseAction(context, mouseAction);
			this->SetToggleAction(context, toggleAction);

			// Load other context options
			bool autoClose = (lpAutoClose.getnumtokens() > i)           ? (lpAutoClose.gettoken_int(i) == -1 ? true : false) : false;
			int  positionX = (lpPosition.getnumtokens()  > (i * 2))     ? (lpPosition.gettoken_int(i * 2))                   : 0;
			int  positionY = (lpPosition.getnumtokens()  > (i * 2) + 1) ? (lpPosition.gettoken_int((i * 2) + 1))             : 0;

			this->SetAutoClose(context, autoClose);
			this->SetPositionOffset(context, positionX, positionY);
		}
	}

	LineParser lp(false);
	lp.parse(options);
	int i = -1;
	m_options = {};
	if (lp.getnumtokens() > ++i) m_options.focus                  = lp.gettoken_int(i);
	if (lp.getnumtokens() > ++i) m_options.topmost                = lp.gettoken_int(i);
	if (lp.getnumtokens() > ++i) m_options.tcpTrack               = lp.gettoken_int(i);
	if (lp.getnumtokens() > ++i) m_options.tcpEnvelope            = lp.gettoken_int(i);
	if (lp.getnumtokens() > ++i) m_options.mcpTrack               = lp.gettoken_int(i);
	if (lp.getnumtokens() > ++i) m_options.arrangeTrack           = lp.gettoken_int(i);
	if (lp.getnumtokens() > ++i) m_options.arrangeItem            = lp.gettoken_int(i);
	if (lp.getnumtokens() > ++i) m_options.arrangeStretchMarker   = lp.gettoken_int(i);
	if (lp.getnumtokens() > ++i) m_options.arrangeTakeEnvelope    = lp.gettoken_int(i);
	if (lp.getnumtokens() > ++i) m_options.arrangeTrackEnvelope   = lp.gettoken_int(i);
	if (lp.getnumtokens() > ++i) m_options.arrangeActTake         = lp.gettoken_int(i);
	if (lp.getnumtokens() > ++i) m_options.midiSetCCLane          = lp.gettoken_int(i);
	if (lp.getnumtokens() > ++i) m_options.inlineItem             = lp.gettoken_int(i);
	if (lp.getnumtokens() > ++i) m_options.inlineSetCCLane        = lp.gettoken_int(i);
	if (lp.getnumtokens() > ++i) m_options.position               = lp.gettoken_int(i);
	if (lp.getnumtokens() > ++i) m_options.setToolbarToForeground = lp.gettoken_int(i);
	if (lp.getnumtokens() > ++i) m_options.autocloseInactive      = lp.gettoken_int(i);

	this->UpdateInternals();
}

void BR_ContextualToolbar::Cleaup ()
{
	plugin_register("-timer",(void*)BR_ContextualToolbar::TooltipTimer);

	// Make sure all hooked windows procedures are removed
	for (int i = 0; i < m_callbackToolbars.GetSize(); ++i)
	{
		HWND hwnd       = m_callbackToolbars.Get(i)->hwnd;
		WNDPROC wndProc = m_callbackToolbars.Get(i)->wndProc;
		if (hwnd && wndProc)
			SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)wndProc);
	}
	m_callbackToolbars.EmptySafe(true);
}

void BR_ContextualToolbar::UpdateInternals ()
{
	const std::pair<BR_ToolbarContext, BR_MouseInfo::Modes> modes[] {
		{ TRANSPORT,                          BR_MouseInfo::MODE_TRANSPORT   },
		{ RULER,                              BR_MouseInfo::MODE_RULER       },
		{ RULER_REGIONS,                      BR_MouseInfo::MODE_RULER       },
		{ RULER_MARKERS,                      BR_MouseInfo::MODE_RULER       },
		{ RULER_TEMPO,                        BR_MouseInfo::MODE_RULER       },
		{ RULER_TIMELINE,                     BR_MouseInfo::MODE_RULER       },
		{ TCP,                                BR_MouseInfo::MODE_MCP_TCP     },
		{ TCP_EMPTY,                          BR_MouseInfo::MODE_MCP_TCP     },
		{ TCP_TRACK,                          BR_MouseInfo::MODE_MCP_TCP     },
		{ TCP_TRACK_MASTER,                   BR_MouseInfo::MODE_MCP_TCP     },
		{ TCP_ENVELOPE,                       BR_MouseInfo::MODE_MCP_TCP     },
		{ TCP_ENVELOPE_VOLUME,                BR_MouseInfo::MODE_MCP_TCP     },
		{ TCP_ENVELOPE_PAN,                   BR_MouseInfo::MODE_MCP_TCP     },
		{ TCP_ENVELOPE_WIDTH,                 BR_MouseInfo::MODE_MCP_TCP     },
		{ TCP_ENVELOPE_MUTE,                  BR_MouseInfo::MODE_MCP_TCP     },
		{ TCP_ENVELOPE_PLAYRATE,              BR_MouseInfo::MODE_MCP_TCP     },
		{ TCP_ENVELOPE_TEMPO,                 BR_MouseInfo::MODE_MCP_TCP     },
		{ MCP,                                BR_MouseInfo::MODE_MCP_TCP     },
		{ MCP_EMPTY,                          BR_MouseInfo::MODE_MCP_TCP     },
		{ MCP_TRACK,                          BR_MouseInfo::MODE_MCP_TCP     },
		{ MCP_TRACK_MASTER,                   BR_MouseInfo::MODE_MCP_TCP     },
		{ ARRANGE,                            BR_MouseInfo::MODE_ARRANGE     },
		{ ARRANGE_EMPTY,                      BR_MouseInfo::MODE_ARRANGE     },
		{ ARRANGE_TRACK,                      BR_MouseInfo::MODE_ARRANGE     },
		{ ARRANGE_TRACK_EMPTY,                BR_MouseInfo::MODE_ARRANGE     },
		{ ARRANGE_TRACK_ITEM,                 BR_MouseInfo::MODE_ARRANGE     },
		{ ARRANGE_TRACK_ITEM_AUDIO,           BR_MouseInfo::MODE_ARRANGE     },
		{ ARRANGE_TRACK_ITEM_MIDI,            BR_MouseInfo::MODE_ARRANGE     },
		{ ARRANGE_TRACK_ITEM_VIDEO,           BR_MouseInfo::MODE_ARRANGE     },
		{ ARRANGE_TRACK_ITEM_EMPTY,           BR_MouseInfo::MODE_ARRANGE     },
		{ ARRANGE_TRACK_ITEM_CLICK,           BR_MouseInfo::MODE_ARRANGE     },
		{ ARRANGE_TRACK_ITEM_TIMECODE,        BR_MouseInfo::MODE_ARRANGE     },
		{ ARRANGE_TRACK_ITEM_PROJECT,         BR_MouseInfo::MODE_ARRANGE     },
		{ ARRANGE_TRACK_ITEM_VIDEOFX,         BR_MouseInfo::MODE_ARRANGE     },
		{ ARRANGE_TRACK_ITEM_STRETCH_MARKER,  BR_MouseInfo::MODE_ARRANGE     },
		{ ARRANGE_TRACK_TAKE_ENVELOPE,        BR_MouseInfo::MODE_ARRANGE     },
		{ ARRANGE_TRACK_TAKE_ENVELOPE_VOLUME, BR_MouseInfo::MODE_ARRANGE     },
		{ ARRANGE_TRACK_TAKE_ENVELOPE_PAN,    BR_MouseInfo::MODE_ARRANGE     },
		{ ARRANGE_TRACK_TAKE_ENVELOPE_MUTE,   BR_MouseInfo::MODE_ARRANGE     },
		{ ARRANGE_TRACK_TAKE_ENVELOPE_PITCH,  BR_MouseInfo::MODE_ARRANGE     },
		{ ARRANGE_ENVELOPE_TRACK,             BR_MouseInfo::MODE_ARRANGE     },
		{ ARRANGE_ENVELOPE_TRACK_VOLUME,      BR_MouseInfo::MODE_ARRANGE     },
		{ ARRANGE_ENVELOPE_TRACK_PAN,         BR_MouseInfo::MODE_ARRANGE     },
		{ ARRANGE_ENVELOPE_TRACK_WIDTH,       BR_MouseInfo::MODE_ARRANGE     },
		{ ARRANGE_ENVELOPE_TRACK_MUTE,        BR_MouseInfo::MODE_ARRANGE     },
		{ ARRANGE_ENVELOPE_TRACK_PITCH,       BR_MouseInfo::MODE_ARRANGE     },
		{ ARRANGE_ENVELOPE_TRACK_PLAYRATE,    BR_MouseInfo::MODE_ARRANGE     },
		{ ARRANGE_ENVELOPE_TRACK_TEMPO,       BR_MouseInfo::MODE_ARRANGE     },
		{ MIDI_EDITOR,                        BR_MouseInfo::MODE_MIDI_EDITOR },
		{ MIDI_EDITOR_RULER,                  BR_MouseInfo::MODE_MIDI_EDITOR },
		{ MIDI_EDITOR_PIANO,                  BR_MouseInfo::MODE_MIDI_EDITOR },
		{ MIDI_EDITOR_PIANO_NAMED,            BR_MouseInfo::MODE_MIDI_EDITOR },
		{ MIDI_EDITOR_NOTES,                  BR_MouseInfo::MODE_MIDI_EDITOR },
		{ MIDI_EDITOR_CC_LANE,                BR_MouseInfo::MODE_MIDI_EDITOR },
		{ MIDI_EDITOR_CC_SELECTOR,            BR_MouseInfo::MODE_MIDI_EDITOR },
		{ INLINE_MIDI_EDITOR,                 BR_MouseInfo::MODE_MIDI_INLINE },
		{ INLINE_MIDI_EDITOR_PIANO,           BR_MouseInfo::MODE_MIDI_INLINE },
		{ INLINE_MIDI_EDITOR_NOTES,           BR_MouseInfo::MODE_MIDI_INLINE },
		{ INLINE_MIDI_EDITOR_CC_LANE,         BR_MouseInfo::MODE_MIDI_INLINE },
		{ INLINE_MIDI_EDITOR_CC_LANE,         BR_MouseInfo::MODE_MIDI_INLINE },
	};

	m_mode = BR_MouseInfo::MODE_IGNORE_ENVELOPE_LANE_SEGMENT;
	m_activeContexts.clear();

	// C++17: for (const auto [context, mode] : modes) {
	for (const auto &pair : modes) {
		if (IsToolbarAction(GetMouseAction(pair.first))) {
			m_mode |= pair.second;
			m_activeContexts.insert(pair.first);
		}
	}
}

int BR_ContextualToolbar::FindRulerToolbar (BR_MouseInfo& mouseInfo, ExecuteOnToolbarLoad& executeOnToolbarLoad)
{
	int context = -1;

	if      (!strcmp(mouseInfo.GetSegment(), "region_lane")) context = RULER_REGIONS;
	else if (!strcmp(mouseInfo.GetSegment(), "marker_lane")) context = RULER_MARKERS;
	else if (!strcmp(mouseInfo.GetSegment(), "tempo_lane"))  context = RULER_TEMPO;
	else if (!strcmp(mouseInfo.GetSegment(), "timeline"))    context = RULER_TIMELINE;

	// Get focus information
	if ((m_options.focus & OPTION_ENABLED) && ((m_options.focus & FOCUS_ALL) || (m_options.focus & FOCUS_MAIN)))
	{
		executeOnToolbarLoad.setFocus     = true;
		executeOnToolbarLoad.focusHwnd    = GetRulerWndAlt();
		executeOnToolbarLoad.focusContext = GetCursorContext2(true);
	}

	if (context != -1 && (this->GetMouseAction(context) == INHERIT_PARENT))
		context = RULER;
	return context;
}

int BR_ContextualToolbar::FindTransportToolbar (BR_MouseInfo& mouseInfo, ExecuteOnToolbarLoad& executeOnToolbarLoad)
{
	int context = TRANSPORT;

	// Get focus information
	if ((m_options.focus & OPTION_ENABLED) && ((m_options.focus & FOCUS_ALL) || (m_options.focus & FOCUS_MAIN)))
	{
		executeOnToolbarLoad.setFocus     = true;
		executeOnToolbarLoad.focusHwnd    = GetTransportWnd();
		executeOnToolbarLoad.focusContext = GetCursorContext2(true);
	}

	return context;
}

int BR_ContextualToolbar::FindTcpToolbar (BR_MouseInfo& mouseInfo, ExecuteOnToolbarLoad& executeOnToolbarLoad)
{
	int context = -1;

	// Empty TCP space
	if (!strcmp(mouseInfo.GetSegment(), "empty"))
	{
		context = TCP_EMPTY;
	}
	// Track control panel
	else if (!strcmp(mouseInfo.GetSegment(), "track"))
	{
		if (mouseInfo.GetTrack() == GetMasterTrack(NULL)) context = (this->GetMouseAction(TCP_TRACK_MASTER) == INHERIT_PARENT) ? TCP_TRACK : TCP_TRACK_MASTER;
		else                                              context = TCP_TRACK;

		// Check track options
		if (this->GetMouseAction(context) != DO_NOTHING && (m_options.tcpTrack & OPTION_ENABLED))
		{
			if ((m_options.tcpTrack & SELECT_TRACK))
				executeOnToolbarLoad.trackToSelect = mouseInfo.GetTrack();
			executeOnToolbarLoad.clearTrackSelection = !!(m_options.tcpTrack & CLEAR_TRACK_SELECTION);
		}
	}
	// Envelope control panel
	else if (!strcmp(mouseInfo.GetSegment(), "envelope"))
	{
		context = TCP_ENVELOPE;

		BR_EnvType type = GetEnvType(mouseInfo.GetEnvelope(), NULL, NULL);
		if      (type == VOLUME || type == VOLUME_PREFX) context = (this->GetMouseAction(TCP_ENVELOPE_VOLUME)   == INHERIT_PARENT) ? TCP_ENVELOPE : TCP_ENVELOPE_VOLUME;
		else if (type == PAN    || type == PAN_PREFX)    context = (this->GetMouseAction(TCP_ENVELOPE_PAN)      == INHERIT_PARENT) ? TCP_ENVELOPE : TCP_ENVELOPE_PAN;
		else if (type == WIDTH  || type == WIDTH_PREFX)  context = (this->GetMouseAction(TCP_ENVELOPE_WIDTH)    == INHERIT_PARENT) ? TCP_ENVELOPE : TCP_ENVELOPE_WIDTH;
		else if (type == MUTE)                           context = (this->GetMouseAction(TCP_ENVELOPE_MUTE)     == INHERIT_PARENT) ? TCP_ENVELOPE : TCP_ENVELOPE_MUTE;
		else if (type == PLAYRATE)                       context = (this->GetMouseAction(TCP_ENVELOPE_PLAYRATE) == INHERIT_PARENT) ? TCP_ENVELOPE : TCP_ENVELOPE_PLAYRATE;
		else if (type == TEMPO)                          context = (this->GetMouseAction(TCP_ENVELOPE_TEMPO)    == INHERIT_PARENT) ? TCP_ENVELOPE : TCP_ENVELOPE_TEMPO;

		// Check envelope options
		if (this->GetMouseAction(context) != DO_NOTHING && (m_options.tcpEnvelope & OPTION_ENABLED))
		{
			if ((m_options.tcpEnvelope & SELECT_ENVELOPE)) executeOnToolbarLoad.envelopeToSelect = mouseInfo.GetEnvelope();
			if ((m_options.tcpEnvelope & SELECT_TRACK))    executeOnToolbarLoad.trackToSelect    = GetEnvParent(mouseInfo.GetEnvelope());

			executeOnToolbarLoad.clearTrackSelection = !!(m_options.tcpEnvelope & CLEAR_TRACK_SELECTION);
		}
	}

	// Get focus information
	if ((m_options.focus & OPTION_ENABLED) && ((m_options.focus & FOCUS_ALL) || (m_options.focus & FOCUS_MAIN)))
	{
		executeOnToolbarLoad.setFocus     = true;
		executeOnToolbarLoad.focusHwnd    = g_hwndParent;
		executeOnToolbarLoad.focusContext = 0;
	}

	if (context != -1 && (this->GetMouseAction(context) == INHERIT_PARENT))
		context = TCP;
	return context;
}

int BR_ContextualToolbar::FindMcpToolbar (BR_MouseInfo& mouseInfo, ExecuteOnToolbarLoad& executeOnToolbarLoad)
{
	int context = -1;

	// Empty MCP space
	if (!strcmp(mouseInfo.GetSegment(), "empty"))
	{
		context = MCP_EMPTY;
	}
	// Track control panel
	else if (!strcmp(mouseInfo.GetSegment(), "track"))
	{
		if (mouseInfo.GetTrack() == GetMasterTrack(NULL)) context = (this->GetMouseAction(MCP_TRACK_MASTER) == INHERIT_PARENT) ? MCP_TRACK : MCP_TRACK_MASTER;
		else                                              context = MCP_TRACK;

		// Check track options
		if (this->GetMouseAction(context) != DO_NOTHING && (m_options.mcpTrack & OPTION_ENABLED))
		{
			if ((m_options.mcpTrack & SELECT_TRACK))
				executeOnToolbarLoad.trackToSelect = mouseInfo.GetTrack();
			executeOnToolbarLoad.clearTrackSelection = !!(m_options.mcpTrack & CLEAR_TRACK_SELECTION);
		}
	}

	// Get focus information
	if ((m_options.focus & OPTION_ENABLED) && ((m_options.focus & FOCUS_ALL) || (m_options.focus & FOCUS_MAIN)))
	{
		executeOnToolbarLoad.setFocus     = true;
		executeOnToolbarLoad.focusHwnd    = GetMixerWnd();
		executeOnToolbarLoad.focusContext = 0;
	}

	if (context != -1 && this->GetMouseAction(context) == INHERIT_PARENT)
		context = MCP;
	return context;
}

int BR_ContextualToolbar::FindArrangeToolbar (BR_MouseInfo& mouseInfo, ExecuteOnToolbarLoad& executeOnToolbarLoad)
{
	int context = -1;

	// Empty arrange space
	if (!strcmp(mouseInfo.GetSegment(), "empty"))
	{
		context = ARRANGE_EMPTY;
	}
	// Empty track lane
	else if (!strcmp(mouseInfo.GetSegment(), "track") && !strcmp(mouseInfo.GetDetails(), "empty"))
	{
		context = (this->GetMouseAction(ARRANGE_TRACK_EMPTY) == INHERIT_PARENT) ? ARRANGE_TRACK : ARRANGE_TRACK_EMPTY;

		if (this->GetMouseAction(context) != DO_NOTHING && (m_options.arrangeTrack & OPTION_ENABLED))
		{
			if ((m_options.arrangeTrack & SELECT_TRACK))
				executeOnToolbarLoad.trackToSelect = mouseInfo.GetTrack();
			executeOnToolbarLoad.clearTrackSelection = !!(m_options.arrangeTrack & CLEAR_TRACK_SELECTION);
		}
	}
	// Envelope (take and track)
	else if (!strcmp(mouseInfo.GetSegment(), "envelope") || ((!strcmp(mouseInfo.GetSegment(), "track") && (!strcmp(mouseInfo.GetDetails(), "env_point"))) || (!strcmp(mouseInfo.GetDetails(), "env_segment"))))
	{
		if (mouseInfo.IsTakeEnvelope())
		{
			context  = ARRANGE_TRACK_TAKE_ENVELOPE;
			BR_EnvType type = GetEnvType(mouseInfo.GetEnvelope(), NULL, NULL);

			if      (type == VOLUME || type == VOLUME_PREFX) context = (this->GetMouseAction(ARRANGE_TRACK_TAKE_ENVELOPE_VOLUME) == INHERIT_PARENT) ? ARRANGE_TRACK_TAKE_ENVELOPE : ARRANGE_TRACK_TAKE_ENVELOPE_VOLUME;
			else if (type == PAN    || type == PAN_PREFX)    context = (this->GetMouseAction(ARRANGE_TRACK_TAKE_ENVELOPE_PAN)    == INHERIT_PARENT) ? ARRANGE_TRACK_TAKE_ENVELOPE : ARRANGE_TRACK_TAKE_ENVELOPE_PAN;
			else if (type == MUTE)                           context = (this->GetMouseAction(ARRANGE_TRACK_TAKE_ENVELOPE_MUTE)   == INHERIT_PARENT) ? ARRANGE_TRACK_TAKE_ENVELOPE : ARRANGE_TRACK_TAKE_ENVELOPE_MUTE;
			else if (type == PITCH)                          context = (this->GetMouseAction(ARRANGE_TRACK_TAKE_ENVELOPE_PITCH)  == INHERIT_PARENT) ? ARRANGE_TRACK_TAKE_ENVELOPE : ARRANGE_TRACK_TAKE_ENVELOPE_PITCH;

			// Check envelope options
			if (this->GetMouseAction(context) != DO_NOTHING && this->GetMouseAction(context) != FOLLOW_ITEM_CONTEXT)
			{
				if ((m_options.arrangeTakeEnvelope & OPTION_ENABLED))
				{
					if ((m_options.arrangeTakeEnvelope & SELECT_ENVELOPE)) executeOnToolbarLoad.envelopeToSelect = mouseInfo.GetEnvelope();
					if ((m_options.arrangeTakeEnvelope & SELECT_ITEM))     executeOnToolbarLoad.itemToSelect     = mouseInfo.GetItem();

					executeOnToolbarLoad.clearItemSelection = !!(m_options.arrangeTakeEnvelope & CLEAR_ITEM_SELECTION);
				}
			}
		}
		else
		{
			context  = ARRANGE_ENVELOPE_TRACK;
			BR_EnvType type = GetEnvType(mouseInfo.GetEnvelope(), NULL, NULL);

			if      (type == VOLUME || type == VOLUME_PREFX) context = (this->GetMouseAction(ARRANGE_ENVELOPE_TRACK_VOLUME)   == INHERIT_PARENT) ? ARRANGE_ENVELOPE_TRACK : ARRANGE_ENVELOPE_TRACK_VOLUME;
			else if (type == PAN    || type == PAN_PREFX)    context = (this->GetMouseAction(ARRANGE_ENVELOPE_TRACK_PAN)      == INHERIT_PARENT) ? ARRANGE_ENVELOPE_TRACK : ARRANGE_ENVELOPE_TRACK_PAN;
			else if (type == WIDTH  || type == WIDTH_PREFX)  context = (this->GetMouseAction(ARRANGE_ENVELOPE_TRACK_WIDTH)    == INHERIT_PARENT) ? ARRANGE_ENVELOPE_TRACK : ARRANGE_ENVELOPE_TRACK_WIDTH;
			else if (type == MUTE)                           context = (this->GetMouseAction(ARRANGE_ENVELOPE_TRACK_MUTE)     == INHERIT_PARENT) ? ARRANGE_ENVELOPE_TRACK : ARRANGE_ENVELOPE_TRACK_MUTE;
			else if (type == PITCH)                          context = (this->GetMouseAction(ARRANGE_ENVELOPE_TRACK_PITCH)    == INHERIT_PARENT) ? ARRANGE_ENVELOPE_TRACK : ARRANGE_ENVELOPE_TRACK_PITCH;
			else if (type == PLAYRATE)                       context = (this->GetMouseAction(ARRANGE_ENVELOPE_TRACK_PLAYRATE) == INHERIT_PARENT) ? ARRANGE_ENVELOPE_TRACK : ARRANGE_ENVELOPE_TRACK_PLAYRATE;
			else if (type == TEMPO)                          context = (this->GetMouseAction(ARRANGE_ENVELOPE_TRACK_TEMPO)    == INHERIT_PARENT) ? ARRANGE_ENVELOPE_TRACK : ARRANGE_ENVELOPE_TRACK_TEMPO;

			// Check envelope options
			if (this->GetMouseAction(context) != DO_NOTHING)
			{
				if ((m_options.arrangeTrackEnvelope & OPTION_ENABLED))
				{
					if ((m_options.arrangeTrackEnvelope & SELECT_ENVELOPE)) executeOnToolbarLoad.envelopeToSelect = mouseInfo.GetEnvelope();
					if ((m_options.arrangeTrackEnvelope & SELECT_TRACK))    executeOnToolbarLoad.trackToSelect    = mouseInfo.GetTrack();

					executeOnToolbarLoad.clearTrackSelection = !!(m_options.arrangeTrackEnvelope & CLEAR_TRACK_SELECTION);
				}
			}
		}
	}
	// Stretch markers
	else if (!strcmp(mouseInfo.GetSegment(), "track") && (!strcmp(mouseInfo.GetDetails(), "item_stretch_marker")))
	{
		context = ARRANGE_TRACK_ITEM_STRETCH_MARKER;

		if (this->GetMouseAction(context) != DO_NOTHING && this->GetMouseAction(context) != FOLLOW_ITEM_CONTEXT)
		{
			if ((m_options.arrangeStretchMarker & OPTION_ENABLED))
			{
				if ((m_options.arrangeStretchMarker & SELECT_TRACK)) executeOnToolbarLoad.trackToSelect = mouseInfo.GetTrack();
				if ((m_options.arrangeStretchMarker & SELECT_ITEM))  executeOnToolbarLoad.itemToSelect  = mouseInfo.GetItem();

				executeOnToolbarLoad.clearTrackSelection = !!(m_options.arrangeStretchMarker & CLEAR_TRACK_SELECTION);
				executeOnToolbarLoad.clearItemSelection  = !!(m_options.arrangeStretchMarker & CLEAR_ITEM_SELECTION);
			}
		}
	}

	// Item (purposely not an "else if" because some contexts can follow item instead of their usual context)
	if ((context != -1 && this->GetMouseAction(context) == FOLLOW_ITEM_CONTEXT) || (!strcmp(mouseInfo.GetSegment(), "track") && (!strcmp(mouseInfo.GetDetails(), "item"))))
	{
		context = ARRANGE_TRACK_ITEM;

		// When determining take type be mindful of the option to activate take under mouse
		MediaItem_Take* take = ((m_options.arrangeActTake & OPTION_ENABLED)) ? mouseInfo.GetTake() : GetActiveTake(mouseInfo.GetItem());
		if (mouseInfo.GetItem() && !take)
		{
			context = (this->GetMouseAction(ARRANGE_TRACK_ITEM_EMPTY) == INHERIT_PARENT) ? ARRANGE_TRACK_ITEM : ARRANGE_TRACK_ITEM_EMPTY;
		}
		else
		{
			// C++11 (on macOS): make this static const with an initializer list
			std::map<SourceType, BR_ToolbarContext> contextMap;
			contextMap[SourceType::Audio]       = ARRANGE_TRACK_ITEM_AUDIO;
			contextMap[SourceType::MIDI]        = ARRANGE_TRACK_ITEM_MIDI;
			contextMap[SourceType::Video]       = ARRANGE_TRACK_ITEM_VIDEO;
			contextMap[SourceType::Click]       = ARRANGE_TRACK_ITEM_CLICK;
			contextMap[SourceType::Timecode]    = ARRANGE_TRACK_ITEM_TIMECODE;
			contextMap[SourceType::Project]     = ARRANGE_TRACK_ITEM_PROJECT;
			contextMap[SourceType::VideoEffect] = ARRANGE_TRACK_ITEM_VIDEOFX;

			const auto it = contextMap.find(GetSourceType(take));
			if (it != contextMap.end()) {
				const BR_ToolbarContext expectedContext = it->second;

				if (GetMouseAction(expectedContext) == INHERIT_PARENT)
					context = ARRANGE_TRACK_ITEM;
				else
					context = expectedContext;
			}
		}

		// Check item options
		if (this->GetMouseAction(context) != DO_NOTHING)
		{
			if ((m_options.arrangeItem & OPTION_ENABLED))
			{
				if ((m_options.arrangeItem & SELECT_TRACK)) executeOnToolbarLoad.trackToSelect = mouseInfo.GetTrack();
				if ((m_options.arrangeItem & SELECT_ITEM))  executeOnToolbarLoad.itemToSelect  = mouseInfo.GetItem();

				executeOnToolbarLoad.clearTrackSelection = !!(m_options.arrangeItem & CLEAR_TRACK_SELECTION);
				executeOnToolbarLoad.clearItemSelection  = !!(m_options.arrangeItem & CLEAR_ITEM_SELECTION);
			}
			if ((m_options.arrangeActTake & OPTION_ENABLED))
			{
				executeOnToolbarLoad.takeIdToActivate = mouseInfo.GetTakeId();
				executeOnToolbarLoad.takeParent       = mouseInfo.GetItem();
			}
		}

		if (this->GetMouseAction(context) == INHERIT_PARENT)
			context = ARRANGE_TRACK;
	}

	// Get focus information
	if ((m_options.focus & OPTION_ENABLED) && ((m_options.focus & FOCUS_ALL) || (m_options.focus & FOCUS_MAIN)))
	{
		executeOnToolbarLoad.setFocus     = true;
		executeOnToolbarLoad.focusHwnd    = g_hwndParent;
		executeOnToolbarLoad.focusContext = (GetSelectedEnvelope(NULL) && !executeOnToolbarLoad.trackToSelect && !executeOnToolbarLoad.itemToSelect) ? 2 : 1;
	}

	if (context != -1 && this->GetMouseAction(context) == INHERIT_PARENT)
		context = ARRANGE;
	return context;
}

int BR_ContextualToolbar::FindMidiToolbar (BR_MouseInfo& mouseInfo, ExecuteOnToolbarLoad& executeOnToolbarLoad)
{
	int context = -1;

	if      (!strcmp(mouseInfo.GetSegment(), "ruler"))   context = MIDI_EDITOR_RULER;
	else if (!strcmp(mouseInfo.GetSegment(), "piano"))   context = (mouseInfo.GetPianoRollMode() == 1) ? ((this->GetMouseAction(MIDI_EDITOR_PIANO_NAMED) == INHERIT_PARENT) ? MIDI_EDITOR_PIANO : MIDI_EDITOR_PIANO_NAMED) : MIDI_EDITOR_PIANO;
	else if (!strcmp(mouseInfo.GetSegment(), "notes"))   context = MIDI_EDITOR_NOTES;
	else if (!strcmp(mouseInfo.GetSegment(), "cc_lane"))
	{
		if      (!strcmp(mouseInfo.GetDetails(), "cc_lane"))     context = MIDI_EDITOR_CC_LANE;
		else if (!strcmp(mouseInfo.GetDetails(), "cc_selector")) context = MIDI_EDITOR_CC_SELECTOR;

		// Check MIDI editor options
		if (context != -1 && this->GetMouseAction(context) != DO_NOTHING && (m_options.midiSetCCLane & OPTION_ENABLED))
			executeOnToolbarLoad.setCCLaneAsClicked = true;
	}

	// Get focus information
	if ((m_options.focus & OPTION_ENABLED) && ((m_options.focus & FOCUS_ALL) || (m_options.focus & FOCUS_MIDI)))
	{
		executeOnToolbarLoad.setFocus     = true;
		executeOnToolbarLoad.focusHwnd    = (HWND)mouseInfo.GetMidiEditor();
		executeOnToolbarLoad.focusContext = GetCursorContext2(true);
	}

	if (context != -1 && this->GetMouseAction(context) == INHERIT_PARENT)
		context = MIDI_EDITOR;
	return context;
}

int BR_ContextualToolbar::FindInlineMidiToolbar (BR_MouseInfo& mouseInfo, ExecuteOnToolbarLoad& executeOnToolbarLoad)
{
	int context = -1;

	if      (!strcmp(mouseInfo.GetSegment(), "notes"))   context = INLINE_MIDI_EDITOR_NOTES;
	else if (!strcmp(mouseInfo.GetSegment(), "piano"))   context = INLINE_MIDI_EDITOR_PIANO;
	else if (!strcmp(mouseInfo.GetSegment(), "cc_lane")) context = INLINE_MIDI_EDITOR_CC_LANE;

	// Check inline MIDI editor options
	if (context != -1 && this->GetMouseAction(context) != DO_NOTHING)
	{
		if ((m_options.inlineSetCCLane & OPTION_ENABLED))
			executeOnToolbarLoad.setCCLaneAsClicked = true;

		if ((m_options.inlineItem & OPTION_ENABLED))
		{
			if ((m_options.inlineItem & SELECT_TRACK)) executeOnToolbarLoad.trackToSelect = mouseInfo.GetTrack();
			if ((m_options.inlineItem & SELECT_ITEM))  executeOnToolbarLoad.itemToSelect  = mouseInfo.GetItem();

			executeOnToolbarLoad.clearTrackSelection = !!(m_options.inlineItem & CLEAR_TRACK_SELECTION);
			executeOnToolbarLoad.clearItemSelection  = !!(m_options.inlineItem & CLEAR_ITEM_SELECTION);
		}
	}

	// Get focus information
	if ((m_options.focus & OPTION_ENABLED) && ((m_options.focus & FOCUS_ALL) || (m_options.focus & FOCUS_MAIN)))
	{
		executeOnToolbarLoad.setFocus     = true;
		executeOnToolbarLoad.focusHwnd    = g_hwndParent;
		executeOnToolbarLoad.focusContext = 1;
	}

	if (context != -1 && this->GetMouseAction(context) == INHERIT_PARENT)
		context = INLINE_MIDI_EDITOR;
	return context;
}

bool BR_ContextualToolbar::SelectTrack (ExecuteOnToolbarLoad& executeOnToolbarLoad)
{
	bool undo = false;
	if (executeOnToolbarLoad.trackToSelect)
	{
		if (executeOnToolbarLoad.clearTrackSelection)
		{
			MediaTrack* masterTrack = GetMasterTrack(NULL);
			if (masterTrack != executeOnToolbarLoad.trackToSelect && GetMediaTrackInfo_Value(masterTrack, "I_SELECTED"))
			{
				SetMediaTrackInfo_Value(masterTrack, "I_SELECTED", 0);
				undo = true;
			}
			for (int i = 0; i < CountTracks(NULL); ++i)
			{
				MediaTrack* track = GetTrack(NULL, i);
				if (track != executeOnToolbarLoad.trackToSelect && GetMediaTrackInfo_Value(track, "I_SELECTED"))
				{
					SetMediaTrackInfo_Value(track, "I_SELECTED", 0);
					undo = true;
				}
			}
		}
		if (!GetMediaTrackInfo_Value(executeOnToolbarLoad.trackToSelect, "I_SELECTED"))
		{
			SetMediaTrackInfo_Value(executeOnToolbarLoad.trackToSelect, "I_SELECTED", 1);
			undo = true;
		}
	}
	return undo;
}

bool BR_ContextualToolbar::SelectEnvelope (ExecuteOnToolbarLoad& executeOnToolbarLoad)
{
	bool undo = false;
	if (executeOnToolbarLoad.envelopeToSelect)
	{
		if (executeOnToolbarLoad.envelopeToSelect != GetSelectedEnvelope(NULL))
			undo = true;

		if (!executeOnToolbarLoad.setFocus)
		{
			HWND hwnd;
			GetSetFocus(false, &hwnd, NULL);
			SetCursorContext(g_i2, executeOnToolbarLoad.envelopeToSelect);
			GetSetFocus(true, &hwnd, &g_i2); // don't let other window lose focus, but make sure context is set to envelope!
		}
		else
		{
			SetCursorContext(2, executeOnToolbarLoad.envelopeToSelect);
		}
	}

	return undo;
}

bool BR_ContextualToolbar::SelectItem (ExecuteOnToolbarLoad& executeOnToolbarLoad)
{
	bool undo = false;
	if (executeOnToolbarLoad.itemToSelect)
	{
		if (executeOnToolbarLoad.clearItemSelection)
		{
			const int itemCount = CountMediaItems(NULL);
			for (int i = 0; i < itemCount; ++i)
			{
				MediaItem* item = GetMediaItem(NULL, i);
				if (item != executeOnToolbarLoad.itemToSelect && GetMediaItemInfo_Value(item, "B_UISEL"))
				{
					SetMediaItemInfo_Value(item, "B_UISEL", false);
					undo = true;
				}
			}
		}

		if (!GetMediaItemInfo_Value(executeOnToolbarLoad.itemToSelect, "B_UISEL"))
		{
			SetMediaItemInfo_Value(executeOnToolbarLoad.itemToSelect, "B_UISEL", true);
			undo = true;
		}
	}
	return undo;
}

bool BR_ContextualToolbar::ActivateTake (ExecuteOnToolbarLoad& executeOnToolbarLoad)
{
	bool undo = false;
	if (executeOnToolbarLoad.takeIdToActivate != -1 && (int)GetMediaItemInfo_Value(executeOnToolbarLoad.takeParent, "I_CURTAKE") != executeOnToolbarLoad.takeIdToActivate)
	{
		GetSetMediaItemInfo(executeOnToolbarLoad.takeParent, "I_CURTAKE", &executeOnToolbarLoad.takeIdToActivate);
		undo = true;
	}

	return undo;
}

void BR_ContextualToolbar::SetFocus (ExecuteOnToolbarLoad& executeOnToolbarLoad)
{
	if (executeOnToolbarLoad.setFocus)
		GetSetFocus(true, &executeOnToolbarLoad.focusHwnd, &executeOnToolbarLoad.focusContext);
}

void BR_ContextualToolbar::SetCCLaneClicked (ExecuteOnToolbarLoad& executeOnToolbarLoad, BR_MouseInfo& mouseInfo)
{
	if (executeOnToolbarLoad.setCCLaneAsClicked)
		mouseInfo.SetDetectedCCLaneAsLastClicked();
}

void BR_ContextualToolbar::RepositionToolbar (ExecuteOnToolbarLoad& executeOnToolbarLoad, HWND toolbarHwnd)
{
	if (toolbarHwnd)
	{
		RECT r;  GetWindowRect(toolbarHwnd, &r);
		POINT p; GetCursorPos(&p);
		if (executeOnToolbarLoad.positionOverride)
		{
			int vert = -666;
			int horz = -666;

			if      ((executeOnToolbarLoad.positionOrientation & POSITION_H_RIGHT))  horz = 1;
			else if ((executeOnToolbarLoad.positionOrientation & POSITION_H_MIDDLE)) horz = 0;
			else if ((executeOnToolbarLoad.positionOrientation & POSITION_H_LEFT))   horz = -1;

			if      ((executeOnToolbarLoad.positionOrientation & POSITION_V_BOTTOM)) vert = -1;
			else if ((executeOnToolbarLoad.positionOrientation & POSITION_V_MIDDLE)) vert = 0;
			else if ((executeOnToolbarLoad.positionOrientation & POSITION_V_TOP))    vert = 1;

			CenterOnPoint(&r, p, horz, vert, executeOnToolbarLoad.positionOffsetX, executeOnToolbarLoad.positionOffsetY);
		}

		RECT screen;
		GetMonitorRectFromPoint(p, true, &screen);
		BoundToRect(screen, &r);
		SetWindowPos(toolbarHwnd, NULL, r.left, r.top, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
	}
}

void BR_ContextualToolbar::SetToolbarWndProc (ExecuteOnToolbarLoad& executeOnToolbarLoad, HWND toolbarHwnd, int toggleAction, HWND toolbarParent)
{
	if (toolbarHwnd)
	{
		// Check if toolbar is already assigned callback
		int id = -1;
		for (int i = 0; i < m_callbackToolbars.GetSize(); ++i)
		{
			if (m_callbackToolbars.Get(i)->hwnd == toolbarHwnd)
			{
				id = i;
				break;
			}
		}

		// Assign callback or just enable functionality if already has callback
		ToolbarWndData* toolbarWndData  = NULL;
		if (id == -1)
		{
			if ((toolbarWndData = m_callbackToolbars.Add(new ToolbarWndData())))
			{
				if (WNDPROC wndProc = (WNDPROC)SetWindowLongPtr(toolbarHwnd, GWLP_WNDPROC, (LONG_PTR)BR_ContextualToolbar::ToolbarWndCallback))
				{
					toolbarWndData->hwnd         = toolbarHwnd;
					toolbarWndData->wndProc      = wndProc;
					toolbarWndData->keepOnTop    = executeOnToolbarLoad.makeTopMost;
					toolbarWndData->autoClose    = executeOnToolbarLoad.autoCloseToolbar;
					toolbarWndData->toggleAction = toggleAction;
				}
				else
				{
					m_callbackToolbars.Delete(m_callbackToolbars.GetSize() - 1, true);
					toolbarWndData = NULL;
				}
			}
		}
		else if ((toolbarWndData = m_callbackToolbars.Get(id)))
		{
			toolbarWndData->keepOnTop = executeOnToolbarLoad.makeTopMost;
			toolbarWndData->autoClose = executeOnToolbarLoad.autoCloseToolbar;
		}

		// Make it always on top
		if (toolbarWndData && toolbarWndData->keepOnTop)
		{
			#ifdef _WIN32
				SetWindowPos(toolbarWndData->hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
			#else
				toolbarWndData->level = SWELL_SetWindowLevel(toolbarWndData->hwnd, 25); // NSStatusWindowLevel
			#endif
		}

		// Set to foreground after hooking into toolbar wnd procedure (because we want for toolbarWndData->lastFocusedHwnd to be saved before focusing toolbar)
		if (executeOnToolbarLoad.setToolbarToForeground && toolbarParent)
			::SetFocus(toolbarParent);
	}
}

int BR_ContextualToolbar::GetMouseAction (int context)
{
	return m_contexts[context].mouseAction;
}

int BR_ContextualToolbar::GetToggleAction (int context)
{
	return m_contexts[context].toggleAction;
}

int BR_ContextualToolbar::GetPositionOffsetX (int context)
{
	return m_contexts[context].positionOffsetX;
}

int BR_ContextualToolbar::GetPositionOffsetY (int context)
{
	return m_contexts[context].positionOffsetY;
}

auto BR_ContextualToolbar::GetAutoClose (int context) -> AutoClose
{
	if (!m_contexts[context].autoClose)
		return AutoClose::Disabled;

	return m_options.autocloseInactive ? AutoClose::Inactive : AutoClose::Command;
}

void BR_ContextualToolbar::SetMouseAction (int context, int mouseAction)
{
	m_contexts[context].mouseAction = mouseAction;
}

void BR_ContextualToolbar::SetToggleAction (int context, int toggleAction)
{
	m_contexts[context].toggleAction = toggleAction;
}

void BR_ContextualToolbar::SetPositionOffset (int context, int x, int y)
{
	m_contexts[context].positionOffsetX = x;
	m_contexts[context].positionOffsetY = y;
}

void BR_ContextualToolbar::SetAutoClose (int context, bool autoClose)
{
	m_contexts[context].autoClose = autoClose;
}

int BR_ContextualToolbar::TranslateAction (int mouseAction, bool toIni)
{
	if (toIni)
	{
		switch (mouseAction)
		{
			case DO_NOTHING:            return 1;
			case INHERIT_PARENT:        return 2;
			case FOLLOW_ITEM_CONTEXT:   return 3;
			case TOOLBAR_01_MOUSE:      return 4;
			case TOOLBAR_02_MOUSE:      return 5;
			case TOOLBAR_03_MOUSE:      return 6;
			case TOOLBAR_04_MOUSE:      return 7;
			case TOOLBAR_05_MOUSE:      return 8;
			case TOOLBAR_06_MOUSE:      return 9;
			case TOOLBAR_07_MOUSE:      return 10;
			case TOOLBAR_08_MOUSE:      return 11;
			case TOOLBAR_09_MOUSE:      return 16;
			case TOOLBAR_10_MOUSE:      return 17;
			case TOOLBAR_11_MOUSE:      return 18;
			case TOOLBAR_12_MOUSE:      return 19;
			case TOOLBAR_13_MOUSE:      return 20;
			case TOOLBAR_14_MOUSE:      return 21;
			case TOOLBAR_15_MOUSE:      return 22;
			case TOOLBAR_16_MOUSE:      return 23;
			case MIDI_TOOLBAR_01_MOUSE: return 12;
			case MIDI_TOOLBAR_02_MOUSE: return 13;
			case MIDI_TOOLBAR_03_MOUSE: return 14;
			case MIDI_TOOLBAR_04_MOUSE: return 15;
			case MIDI_TOOLBAR_05_MOUSE: return 24;
			case MIDI_TOOLBAR_06_MOUSE: return 25;
			case MIDI_TOOLBAR_07_MOUSE: return 26;
			case MIDI_TOOLBAR_08_MOUSE: return 27;
			default:                    return 1;
		}
	}
	else
	{
		switch (mouseAction)
		{
			case 1:  return DO_NOTHING;
			case 2:  return INHERIT_PARENT;
			case 3:  return FOLLOW_ITEM_CONTEXT;
			case 4:  return TOOLBAR_01_MOUSE;
			case 5:  return TOOLBAR_02_MOUSE;
			case 6:  return TOOLBAR_03_MOUSE;
			case 7:  return TOOLBAR_04_MOUSE;
			case 8:  return TOOLBAR_05_MOUSE;
			case 9:  return TOOLBAR_06_MOUSE;
			case 10: return TOOLBAR_07_MOUSE;
			case 11: return TOOLBAR_08_MOUSE;
			case 12: return MIDI_TOOLBAR_01_MOUSE;
			case 13: return MIDI_TOOLBAR_02_MOUSE;
			case 14: return MIDI_TOOLBAR_03_MOUSE;
			case 15: return MIDI_TOOLBAR_04_MOUSE;
			case 16: return TOOLBAR_09_MOUSE;
			case 17: return TOOLBAR_10_MOUSE;
			case 18: return TOOLBAR_11_MOUSE;
			case 19: return TOOLBAR_12_MOUSE;
			case 20: return TOOLBAR_13_MOUSE;
			case 21: return TOOLBAR_14_MOUSE;
			case 22: return TOOLBAR_15_MOUSE;
			case 23: return TOOLBAR_16_MOUSE;
			case 24: return MIDI_TOOLBAR_05_MOUSE;
			case 25: return MIDI_TOOLBAR_06_MOUSE;
			case 26: return MIDI_TOOLBAR_07_MOUSE;
			case 27: return MIDI_TOOLBAR_08_MOUSE;
			default: return DO_NOTHING;
		}
	}
}

int BR_ContextualToolbar::GetContextFromIniIndex (int index)
{
	switch (index)
	{
		case 0:  return TRANSPORT;                        case 1:  return RULER;                              case 2:  return RULER_REGIONS;
		case 3:  return RULER_MARKERS;                    case 4:  return RULER_TEMPO;                        case 5:  return RULER_TIMELINE;
		case 6:  return TCP;                              case 7:  return TCP_EMPTY;                          case 8:  return TCP_TRACK;
		case 9:  return TCP_TRACK_MASTER;                 case 10: return TCP_ENVELOPE;                       case 11: return TCP_ENVELOPE_VOLUME;
		case 12: return TCP_ENVELOPE_PAN;                 case 13: return TCP_ENVELOPE_WIDTH;                 case 14: return TCP_ENVELOPE_MUTE;
		case 15: return TCP_ENVELOPE_PLAYRATE;            case 16: return TCP_ENVELOPE_TEMPO;                 case 17: return MCP;
		case 18: return MCP_EMPTY;                        case 19: return MCP_TRACK;                          case 20: return MCP_TRACK_MASTER;
		case 21: return ARRANGE;                          case 22: return ARRANGE_EMPTY;                      case 23: return ARRANGE_TRACK;
		case 24: return ARRANGE_TRACK_EMPTY;              case 25: return ARRANGE_TRACK_ITEM;                 case 26: return ARRANGE_TRACK_ITEM_AUDIO;
		case 27: return ARRANGE_TRACK_ITEM_MIDI;          case 28: return ARRANGE_TRACK_ITEM_VIDEO;           case 29: return ARRANGE_TRACK_ITEM_EMPTY;
		case 30: return ARRANGE_TRACK_ITEM_CLICK;         case 31: return ARRANGE_TRACK_ITEM_TIMECODE;        case 32: return ARRANGE_TRACK_ITEM_STRETCH_MARKER;
		case 33: return ARRANGE_TRACK_TAKE_ENVELOPE;      case 34: return ARRANGE_TRACK_TAKE_ENVELOPE_VOLUME; case 35: return ARRANGE_TRACK_TAKE_ENVELOPE_PAN;
		case 36: return ARRANGE_TRACK_TAKE_ENVELOPE_MUTE; case 37: return ARRANGE_TRACK_TAKE_ENVELOPE_PITCH;  case 38: return ARRANGE_ENVELOPE_TRACK;
		case 39: return ARRANGE_ENVELOPE_TRACK_VOLUME;    case 40: return ARRANGE_ENVELOPE_TRACK_PAN;         case 41: return ARRANGE_ENVELOPE_TRACK_WIDTH;
		case 42: return ARRANGE_ENVELOPE_TRACK_MUTE;      case 43: return ARRANGE_ENVELOPE_TRACK_PITCH;       case 44: return ARRANGE_ENVELOPE_TRACK_PLAYRATE;
		case 45: return ARRANGE_ENVELOPE_TRACK_TEMPO;     case 46: return MIDI_EDITOR;                        case 47: return MIDI_EDITOR_RULER;
		case 48: return MIDI_EDITOR_PIANO;                case 49: return MIDI_EDITOR_PIANO_NAMED;            case 50: return MIDI_EDITOR_NOTES;
		case 51: return MIDI_EDITOR_CC_LANE;              case 52: return MIDI_EDITOR_CC_SELECTOR;            case 53: return INLINE_MIDI_EDITOR;
		case 54: return INLINE_MIDI_EDITOR_PIANO;         case 55: return UNUSED_CONTEXT;                     case 56: return INLINE_MIDI_EDITOR_NOTES;
		case 57: return INLINE_MIDI_EDITOR_CC_LANE;       case 58: return ARRANGE_TRACK_ITEM_PROJECT;         case 59: return ARRANGE_TRACK_ITEM_VIDEOFX;
	}

	return -666;
}

bool BR_ContextualToolbar::GetReaperToolbar (int id, int* mouseAction, int* toggleAction, char* toolbarName, int toolbarNameSz)
{
	if (id >= 0 && id < TOOLBAR_COUNT)
	{
		if      (id == 0)  {WritePtr(mouseAction, (int)DO_NOTHING);            WritePtr(toggleAction, (int)DO_NOTHING);}
		else if (id == 1)  {WritePtr(mouseAction, (int)INHERIT_PARENT);        WritePtr(toggleAction, (int)INHERIT_PARENT);}
		else if (id == 2)  {WritePtr(mouseAction, (int)FOLLOW_ITEM_CONTEXT);   WritePtr(toggleAction, (int)FOLLOW_ITEM_CONTEXT);}
		else if (id == 3)  {WritePtr(mouseAction, (int)TOOLBAR_01_MOUSE);      WritePtr(toggleAction, (int)TOOLBAR_01_TOGGLE);}
		else if (id == 4)  {WritePtr(mouseAction, (int)TOOLBAR_02_MOUSE);      WritePtr(toggleAction, (int)TOOLBAR_02_TOGGLE);}
		else if (id == 5)  {WritePtr(mouseAction, (int)TOOLBAR_03_MOUSE);      WritePtr(toggleAction, (int)TOOLBAR_03_TOGGLE);}
		else if (id == 6)  {WritePtr(mouseAction, (int)TOOLBAR_04_MOUSE);      WritePtr(toggleAction, (int)TOOLBAR_04_TOGGLE);}
		else if (id == 7)  {WritePtr(mouseAction, (int)TOOLBAR_05_MOUSE);      WritePtr(toggleAction, (int)TOOLBAR_05_TOGGLE);}
		else if (id == 8)  {WritePtr(mouseAction, (int)TOOLBAR_06_MOUSE);      WritePtr(toggleAction, (int)TOOLBAR_06_TOGGLE);}
		else if (id == 9)  {WritePtr(mouseAction, (int)TOOLBAR_07_MOUSE);      WritePtr(toggleAction, (int)TOOLBAR_07_TOGGLE);}
		else if (id == 10) {WritePtr(mouseAction, (int)TOOLBAR_08_MOUSE);      WritePtr(toggleAction, (int)TOOLBAR_08_TOGGLE);}
		else if (id == 11) {WritePtr(mouseAction, (int)TOOLBAR_09_MOUSE);      WritePtr(toggleAction, (int)TOOLBAR_09_TOGGLE);}
		else if (id == 12) {WritePtr(mouseAction, (int)TOOLBAR_10_MOUSE);      WritePtr(toggleAction, (int)TOOLBAR_10_TOGGLE);}
		else if (id == 13) {WritePtr(mouseAction, (int)TOOLBAR_11_MOUSE);      WritePtr(toggleAction, (int)TOOLBAR_11_TOGGLE);}
		else if (id == 14) {WritePtr(mouseAction, (int)TOOLBAR_12_MOUSE);      WritePtr(toggleAction, (int)TOOLBAR_12_TOGGLE);}
		else if (id == 15) {WritePtr(mouseAction, (int)TOOLBAR_13_MOUSE);      WritePtr(toggleAction, (int)TOOLBAR_13_TOGGLE);}
		else if (id == 16) {WritePtr(mouseAction, (int)TOOLBAR_14_MOUSE);      WritePtr(toggleAction, (int)TOOLBAR_14_TOGGLE);}
		else if (id == 17) {WritePtr(mouseAction, (int)TOOLBAR_15_MOUSE);      WritePtr(toggleAction, (int)TOOLBAR_15_TOGGLE);}
		else if (id == 18) {WritePtr(mouseAction, (int)TOOLBAR_16_MOUSE);      WritePtr(toggleAction, (int)TOOLBAR_16_TOGGLE);}
		else if (id == 19) {WritePtr(mouseAction, (int)MIDI_TOOLBAR_01_MOUSE); WritePtr(toggleAction, (int)MIDI_TOOLBAR_01_TOGGLE);}
		else if (id == 20) {WritePtr(mouseAction, (int)MIDI_TOOLBAR_02_MOUSE); WritePtr(toggleAction, (int)MIDI_TOOLBAR_02_TOGGLE);}
		else if (id == 21) {WritePtr(mouseAction, (int)MIDI_TOOLBAR_03_MOUSE); WritePtr(toggleAction, (int)MIDI_TOOLBAR_03_TOGGLE);}
		else if (id == 22) {WritePtr(mouseAction, (int)MIDI_TOOLBAR_04_MOUSE); WritePtr(toggleAction, (int)MIDI_TOOLBAR_04_TOGGLE);}
		else if (id == 23) {WritePtr(mouseAction, (int)MIDI_TOOLBAR_05_MOUSE); WritePtr(toggleAction, (int)MIDI_TOOLBAR_05_TOGGLE);}
		else if (id == 24) {WritePtr(mouseAction, (int)MIDI_TOOLBAR_06_MOUSE); WritePtr(toggleAction, (int)MIDI_TOOLBAR_06_TOGGLE);}
		else if (id == 25) {WritePtr(mouseAction, (int)MIDI_TOOLBAR_07_MOUSE); WritePtr(toggleAction, (int)MIDI_TOOLBAR_07_TOGGLE);}
		else if (id == 26) {WritePtr(mouseAction, (int)MIDI_TOOLBAR_08_MOUSE); WritePtr(toggleAction, (int)MIDI_TOOLBAR_08_TOGGLE);}

		if (toolbarName)
		{
			if (id >= 0 && id <= 2)
			{
				if      (id == 0) snprintf(toolbarName, toolbarNameSz, "%s", __LOCALIZE("Do nothing", "sws_DLG_181"));
				else if (id == 1) snprintf(toolbarName, toolbarNameSz, "%s", __LOCALIZE("Inherit parent", "sws_DLG_181"));
				else if (id == 2) snprintf(toolbarName, toolbarNameSz, "%s", __LOCALIZE("Follow item context", "sws_DLG_181"));
			}
			else
			{
				bool midiToolbar = (id >= 19 && id <= 26) ? (true)    : (false);
				int toolbarId    = (id >= 19 && id <= 26) ? (id - 18) : (id - 2);

				WDL_FastString defaultName, section;
				if (midiToolbar)
				{
					section.AppendFormatted(512, "%s %d", "Floating MIDI toolbar", toolbarId);
					defaultName.AppendFormatted(512, "%s %d", "MIDI", toolbarId);
				}
				else
				{
					section.AppendFormatted(512, "%s %d", "Floating toolbar", toolbarId);
					defaultName.AppendFormatted(512, "%s %d", "Toolbar", toolbarId);
				}

				GetPrivateProfileString(section.Get(), "title", __localizeFunc(defaultName.Get(), "MENU_349", 0), toolbarName, toolbarNameSz, GetReaperMenuIni());
			}
		}

		return true;
	}

	return false;
}

bool BR_ContextualToolbar::IsToolbarAction (int action)
{
	if (action != DO_NOTHING && action != INHERIT_PARENT && action != FOLLOW_ITEM_CONTEXT)
		return true;
	else
		return false;
}

int BR_ContextualToolbar::GetToolbarId (int mouseAction)
{
	int id = -1;
	int currentMouseAction;
	while (this->GetReaperToolbar(++id, &currentMouseAction, NULL, NULL, 0))
	{
		if (mouseAction == currentMouseAction)
			return id;
	}
	return -1;
}

HWND BR_ContextualToolbar::GetFloatingToolbarHwnd (int mouseAction, bool* inDocker)
{
	HWND hwnd = NULL;
	bool docker = false;
	int toolbarId = this->GetToolbarId(mouseAction);

	char toolbarName[256];
	if (this->GetReaperToolbar(toolbarId, NULL,NULL, toolbarName, sizeof(toolbarName)))
	{
		hwnd = FindFloatingToolbarWndByName(toolbarName);
		docker = GetParent(hwnd) != g_hwndParent;
	}

	WritePtr(inDocker, docker);
	return hwnd;
}

void BR_ContextualToolbar::CloseAllAssignedToolbars ()
{
	for (set<int>::iterator it = m_activeContexts.begin(); it != m_activeContexts.end(); ++it)
	{
		int toggleAction = this->GetToggleAction(*it);
		if (this->IsContextValid(*it) && this->IsToolbarAction(toggleAction))
		{
			if (GetToggleCommandState(toggleAction))
				Main_OnCommand(toggleAction, 0);
		}
	}
}

LRESULT CALLBACK BR_ContextualToolbar::ToolbarWndCallback (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int id = -1;
	for (int i = 0; i < m_callbackToolbars.GetSize(); ++i)
	{
		if (ToolbarWndData* toolbarWndData = m_callbackToolbars.Get(i))
		{
			if (toolbarWndData->hwnd == hwnd)
			{
				id = i;
				break;
			}
		}
	}

	if (id == -1)
	{
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	else
	{
		ToolbarWndData* toolbarWndData = m_callbackToolbars.Get(id);
		WNDPROC wndProc = toolbarWndData->wndProc; // save wndProc in case toolbarWndData gets deleted before returning

		if (uMsg == WM_SHOWWINDOW || uMsg == WM_ACTIVATEAPP)
		{
			if (toolbarWndData->keepOnTop)
			{
				if (wParam == TRUE)
				{

					#ifdef _WIN32
						SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
					#else
						SWELL_SetWindowLevel(hwnd, 25); // NSStatusWindowLevel
					#endif
				}
				else
				{
					#ifdef _WIN32
						SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
					#else
						SWELL_SetWindowLevel(hwnd, toolbarWndData->level);
					#endif
				}
			}
		}
		else if (uMsg == WM_MOUSEMOVE && toolbarWndData->keepOnTop && !tooltipTimerActive)
		{
			plugin_register("timer",(void*)BR_ContextualToolbar::TooltipTimer);
		}
		else if ((toolbarWndData->autoClose == AutoClose::Command  && uMsg == WM_COMMAND) ||
		         (toolbarWndData->autoClose == AutoClose::Inactive && uMsg == WM_ACTIVATE && LOWORD(wParam) == WA_INACTIVE))
		{
			Main_OnCommand(toolbarWndData->toggleAction, 0);

			SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)wndProc);
			// focus last focused window when toolbar is auto closed, p=2171694
			if (toolbarWndData->lastFocusedHwnd)
				::SetFocus(toolbarWndData->lastFocusedHwnd);
			m_callbackToolbars.Delete(id, true);
		}
		else if (uMsg == WM_ACTIVATE)
		{
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				if (IsWindowVisible(hwnd))
					toolbarWndData->lastFocusedHwnd = NULL;
			}
			else
			{
				if (!toolbarWndData->lastFocusedHwnd)
					toolbarWndData->lastFocusedHwnd = (HWND)lParam;
			}
		}
		else if (uMsg == WM_DESTROY)
		{
			SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)wndProc);
			if (toolbarWndData->lastFocusedHwnd)
				::SetFocus(toolbarWndData->lastFocusedHwnd);
			m_callbackToolbars.Delete(id, true);
		}

		return wndProc(hwnd, uMsg, wParam, lParam);
	}
}

void BR_ContextualToolbar::TooltipTimer()
{
	POINT p;
	GetCursorPos(&p);
	if (HWND hwnd = WindowFromPoint(p))
	{
		bool toolbarFound = false;
		for (int i = 0; i < m_callbackToolbars.GetSize(); ++i)
		{
			if (m_callbackToolbars.Get(i)->hwnd == hwnd && m_callbackToolbars.Get(i)->keepOnTop)
			{
				toolbarFound = true;
				if (HWND tooltip = GetTooltipWindow())
				{
					#ifdef _WIN32
						SetWindowPos(tooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
					#else
						SWELL_SetWindowLevel(tooltip, 25); // NSStatusWindowLevel
					#endif
				}
				break;
			}
		}

		if (!toolbarFound)
		{
			plugin_register("-timer",(void*)BR_ContextualToolbar::TooltipTimer);
			tooltipTimerActive = false;
		}
	}
}

WDL_PtrList<BR_ContextualToolbar::ToolbarWndData> BR_ContextualToolbar::m_callbackToolbars;
bool BR_ContextualToolbar::tooltipTimerActive = false;

/******************************************************************************
* Contextual toolbars manager                                                 *
******************************************************************************/
BR_ContextualToolbarsManager::BR_ContextualToolbarsManager()
{
}

BR_ContextualToolbarsManager::~BR_ContextualToolbarsManager()
{
}

BR_ContextualToolbar* BR_ContextualToolbarsManager::GetContextualToolbar (int id)
{
	if (id < 0)
		return NULL;

	if (!m_contextualToolbars.Get(id))
	{
		// Fill to the top
		if (id > m_contextualToolbars.GetSize())
		{
			for (int i = 0; i < id; ++i)
			{
				if (!m_contextualToolbars.Get(i))
					m_contextualToolbars.Insert(i, NULL);
			}
		}

		m_contextualToolbars.Insert(id, new BR_ContextualToolbar());
		if (BR_ContextualToolbar* contextualToolbar = m_contextualToolbars.Get(id))
		{
			const size_t size = 1024;
			char contextToolbars[size];
			char contextAutoClose[size];
			char contextPosition[size];
			char options[size];
			GetPrivateProfileString(INI_SECTION, this->GetKeyForPreset(INI_KEY_CONTEXTS,  id).Get(), "", contextToolbars,  size, GetIniFileBR());
			GetPrivateProfileString(INI_SECTION, this->GetKeyForPreset(INI_KEY_AUTOCLOSE, id).Get(), "", contextAutoClose, size, GetIniFileBR());
			GetPrivateProfileString(INI_SECTION, this->GetKeyForPreset(INI_KEY_POSITION,  id).Get(), "", contextPosition,  size, GetIniFileBR());
			GetPrivateProfileString(INI_SECTION, this->GetKeyForPreset(INI_KEY_OPTIONS,   id).Get(), "", options,          size, GetIniFileBR());

			contextualToolbar->ImportConfig(contextToolbars, contextAutoClose, contextPosition, options);
		}
	}

	return m_contextualToolbars.Get(id);
}

void BR_ContextualToolbarsManager::SetContextualToolbar (int id, BR_ContextualToolbar& contextualToolbar)
{
	if (BR_ContextualToolbar* currentContextualToolbar = this->GetContextualToolbar(id))
	{
		*currentContextualToolbar = contextualToolbar;

		WDL_FastString contextToolbars;
		WDL_FastString contextAutoClose;
		WDL_FastString contextPosition;
		WDL_FastString options;
		currentContextualToolbar->ExportConfig(contextToolbars, contextAutoClose, contextPosition, options);

		WritePrivateProfileString(INI_SECTION, this->GetKeyForPreset(INI_KEY_CONTEXTS,  id).Get(), contextToolbars.Get(),  GetIniFileBR());
		WritePrivateProfileString(INI_SECTION, this->GetKeyForPreset(INI_KEY_AUTOCLOSE, id).Get(), contextAutoClose.Get(), GetIniFileBR());
		WritePrivateProfileString(INI_SECTION, this->GetKeyForPreset(INI_KEY_POSITION, id).Get(),  contextPosition.Get(),  GetIniFileBR());
		WritePrivateProfileString(INI_SECTION, this->GetKeyForPreset(INI_KEY_OPTIONS, id).Get(),   options.Get(),          GetIniFileBR());
	}
}

WDL_FastString BR_ContextualToolbarsManager::GetKeyForPreset(const char* key, int id)
{
	WDL_FastString string;
	string.AppendFormatted(256, "%s%.2d", key, id);
	return string;
}

WDL_PtrList<BR_ContextualToolbar> BR_ContextualToolbarsManager::m_contextualToolbars;

/******************************************************************************
* Context toolbars list view                                                  *
******************************************************************************/
BR_ContextualToolbarsView::BR_ContextualToolbarsView (HWND hwndList, HWND hwndEdit)
:SWS_ListView(hwndList, hwndEdit, COL_COUNT, g_cols, CONTEXT_TOOLBARS_VIEW_WND, false, "sws_DLG_181")
{
}

void BR_ContextualToolbarsView::GetItemText (SWS_ListItem* item, int iCol, char* str, int iStrMax)
{
	WritePtr(str, '\0');
	if (!item)
		return;

	int context = *(int*)item;
	switch (iCol)
	{
		case COL_CONTEXT:
		{
			int indentation = 0;
			const char* description = NULL;

			if      (context == TRANSPORT)                          {description =__LOCALIZE("Transport",               "sws_DLG_181");  indentation = 0;}
			else if (context == END_TRANSPORT)                      {description = "";                                                   indentation = 0;}

			else if (context == RULER)                              {description = __LOCALIZE("Ruler",                  "sws_DLG_181");  indentation = 0;}
			else if (context == RULER_REGIONS)                      {description = __LOCALIZE("Regions lane",           "sws_DLG_181");  indentation = 1;}
			else if (context == RULER_MARKERS)                      {description = __LOCALIZE("Marker lane",            "sws_DLG_181");  indentation = 1;}
			else if (context == RULER_TEMPO)                        {description = __LOCALIZE("Tempo lane",             "sws_DLG_181");  indentation = 1;}
			else if (context == RULER_TIMELINE)                     {description = __LOCALIZE("Timeline",               "sws_DLG_181");  indentation = 1;}
			else if (context == END_RULER)                          {description = "";                                                   indentation = 0;}

			else if (context == TCP)                                {description = __LOCALIZE("Track control panel",    "sws_DLG_181");  indentation = 0;}
			else if (context == TCP_EMPTY)                          {description = __LOCALIZE("Empty",                  "sws_DLG_181");  indentation = 1;}
			else if (context == TCP_TRACK)                          {description = __LOCALIZE("Track panel",            "sws_DLG_181");  indentation = 1;}
			else if (context == TCP_TRACK_MASTER)                   {description = __LOCALIZE("Master track panel",     "sws_DLG_181");  indentation = 2;}
			else if (context == TCP_ENVELOPE)                       {description = __LOCALIZE("Envelope panel",         "sws_DLG_181");  indentation = 1;}
			else if (context == TCP_ENVELOPE_VOLUME)                {description = __LOCALIZE("Volume envelope",        "sws_DLG_181");  indentation = 2;}
			else if (context == TCP_ENVELOPE_PAN)                   {description = __LOCALIZE("Pan envelope",           "sws_DLG_181");  indentation = 2;}
			else if (context == TCP_ENVELOPE_WIDTH)                 {description = __LOCALIZE("Width envelope",         "sws_DLG_181");  indentation = 2;}
			else if (context == TCP_ENVELOPE_MUTE)                  {description = __LOCALIZE("Mute envelope",          "sws_DLG_181");  indentation = 2;}
			else if (context == TCP_ENVELOPE_PLAYRATE)              {description = __LOCALIZE("Playrate envelope",      "sws_DLG_181");  indentation = 2;}
			else if (context == TCP_ENVELOPE_TEMPO)                 {description = __LOCALIZE("Tempo envelope",         "sws_DLG_181");  indentation = 2;}
			else if (context == END_TCP)                            {description = "";                                                   indentation = 0;}

			else if (context == MCP)                                {description = __LOCALIZE("Mixer control panel",     "sws_DLG_181"); indentation = 0;}
			else if (context == MCP_EMPTY)                          {description = __LOCALIZE("Empty",                   "sws_DLG_181"); indentation = 1;}
			else if (context == MCP_TRACK)                          {description = __LOCALIZE("Track panel",             "sws_DLG_181"); indentation = 1;}
			else if (context == MCP_TRACK_MASTER)                   {description = __LOCALIZE("Master track",            "sws_DLG_181"); indentation = 2;}
			else if (context == END_MCP)                            {description = "";                                                   indentation = 0;}

			else if (context == ARRANGE)                            {description = __LOCALIZE("Arrange",                 "sws_DLG_181"); indentation = 0;}
			else if (context == ARRANGE_EMPTY)                      {description = __LOCALIZE("Empty arrange area",      "sws_DLG_181"); indentation = 1;}
			else if (context == ARRANGE_TRACK)                      {description = __LOCALIZE("Track lane",              "sws_DLG_181"); indentation = 1;}
			else if (context == ARRANGE_TRACK_EMPTY)                {description = __LOCALIZE("Empty track area",        "sws_DLG_181"); indentation = 2;}
			else if (context == ARRANGE_TRACK_ITEM)                 {description = __LOCALIZE("Item",                    "sws_DLG_181"); indentation = 2;}
			else if (context == ARRANGE_TRACK_ITEM_AUDIO)           {description = __LOCALIZE("Audio item",              "sws_DLG_181"); indentation = 3;}
			else if (context == ARRANGE_TRACK_ITEM_MIDI)            {description = __LOCALIZE("MIDI item",               "sws_DLG_181"); indentation = 3;}
			else if (context == ARRANGE_TRACK_ITEM_VIDEO)           {description = __LOCALIZE("Video item",              "sws_DLG_181"); indentation = 3;}
			else if (context == ARRANGE_TRACK_ITEM_EMPTY)           {description = __LOCALIZE("Empty item",              "sws_DLG_181"); indentation = 3;}
			else if (context == ARRANGE_TRACK_ITEM_CLICK)           {description = __LOCALIZE("Click source item",       "sws_DLG_181"); indentation = 3;}
			else if (context == ARRANGE_TRACK_ITEM_TIMECODE)        {description = __LOCALIZE("Timecode generator item", "sws_DLG_181"); indentation = 3;}
			else if (context == ARRANGE_TRACK_ITEM_PROJECT)         {description = __LOCALIZE("Subproject item",         "sws_DLG_181"); indentation = 3;}
			else if (context == ARRANGE_TRACK_ITEM_VIDEOFX)         {description = __LOCALIZE("Video processor item",    "sws_DLG_181"); indentation = 3;}
			else if (context == ARRANGE_TRACK_ITEM_STRETCH_MARKER)  {description = __LOCALIZE("Item stretch marker",     "sws_DLG_181"); indentation = 2;}
			else if (context == ARRANGE_TRACK_TAKE_ENVELOPE)        {description = __LOCALIZE("Take envelope",           "sws_DLG_181"); indentation = 2;}
			else if (context == ARRANGE_TRACK_TAKE_ENVELOPE_VOLUME) {description = __LOCALIZE("Volume envelope",         "sws_DLG_181"); indentation = 3;}
			else if (context == ARRANGE_TRACK_TAKE_ENVELOPE_PAN)    {description = __LOCALIZE("Pan envelope",            "sws_DLG_181"); indentation = 3;}
			else if (context == ARRANGE_TRACK_TAKE_ENVELOPE_MUTE)   {description = __LOCALIZE("Mute envelope",           "sws_DLG_181"); indentation = 3;}
			else if (context == ARRANGE_TRACK_TAKE_ENVELOPE_PITCH)  {description = __LOCALIZE("Pitch envelope",          "sws_DLG_181"); indentation = 3;}
			else if (context == ARRANGE_ENVELOPE_TRACK)             {description = __LOCALIZE("Track envelope lane",     "sws_DLG_181"); indentation = 1;}
			else if (context == ARRANGE_ENVELOPE_TRACK_VOLUME)      {description = __LOCALIZE("Volume envelope",         "sws_DLG_181"); indentation = 2;}
			else if (context == ARRANGE_ENVELOPE_TRACK_PAN)         {description = __LOCALIZE("Pan envelope",            "sws_DLG_181"); indentation = 2;}
			else if (context == ARRANGE_ENVELOPE_TRACK_WIDTH)       {description = __LOCALIZE("Width envelope",          "sws_DLG_181"); indentation = 2;}
			else if (context == ARRANGE_ENVELOPE_TRACK_MUTE)        {description = __LOCALIZE("Mute envelope",           "sws_DLG_181"); indentation = 2;}
			else if (context == ARRANGE_ENVELOPE_TRACK_PITCH)       {description = __LOCALIZE("Pitch envelope",          "sws_DLG_181"); indentation = 2;}
			else if (context == ARRANGE_ENVELOPE_TRACK_PLAYRATE)    {description = __LOCALIZE("Playrate envelope",       "sws_DLG_181"); indentation = 2;}
			else if (context == ARRANGE_ENVELOPE_TRACK_TEMPO)       {description = __LOCALIZE("Tempo envelope",          "sws_DLG_181"); indentation = 2;}
			else if (context == END_ARRANGE)                        {description = "";                                                   indentation = 0;}

			else if (context == MIDI_EDITOR)                        {description = __LOCALIZE("MIDI editor",             "sws_DLG_181"); indentation = 0;}
			else if (context == MIDI_EDITOR_RULER)                  {description = __LOCALIZE("Ruler",                   "sws_DLG_181"); indentation = 1;}
			else if (context == MIDI_EDITOR_PIANO)                  {description = __LOCALIZE("Piano roll",              "sws_DLG_181"); indentation = 1;}
			else if (context == MIDI_EDITOR_PIANO_NAMED)            {description = __LOCALIZE("Named notes mode",        "sws_DLG_181"); indentation = 2;}
			else if (context == MIDI_EDITOR_NOTES)                  {description = __LOCALIZE("Notes editing area",      "sws_DLG_181"); indentation = 1;}
			else if (context == MIDI_EDITOR_CC_LANE)                {description = __LOCALIZE("CC lane",                 "sws_DLG_181"); indentation = 1;}
			else if (context == MIDI_EDITOR_CC_SELECTOR)            {description = __LOCALIZE("CC selector area",        "sws_DLG_181"); indentation = 1;}
			else if (context == END_MIDI_EDITOR)                    {description = "";                                                   indentation = 0;}

			else if (context == INLINE_MIDI_EDITOR)                 {description = __LOCALIZE("Inline MIDI editor",      "sws_DLG_181"); indentation = 0;}
			else if (context == INLINE_MIDI_EDITOR_PIANO)           {description = __LOCALIZE("Piano roll",              "sws_DLG_181"); indentation = 1;}
			else if (context == INLINE_MIDI_EDITOR_NOTES)           {description = __LOCALIZE("Notes editing area",      "sws_DLG_181"); indentation = 1;}
			else if (context == INLINE_MIDI_EDITOR_CC_LANE)         {description = __LOCALIZE("CC lane",                 "sws_DLG_181"); indentation = 1;}

			if (description)
			{
				WDL_FastString indentationString;
				for (int i = 0; i < indentation; ++i)
				{
					if      (i == 0 && indentation == 1) indentationString.AppendFormatted(128, "%s%s%s", "  ", UTF8_BULLET, "  ");
					else if (i == 1 && indentation == 2) indentationString.AppendFormatted(128, "%s%s%s", "  ", UTF8_CIRCLE, "  ");
					else                                 indentationString.Append("    ");
				}
				snprintf(str, iStrMax, "%s%s", indentationString.Get(), description);
			}
		}
		break;

		case COL_TOOLBAR:
		case COL_AUTOCLOSE:
		case COL_POSITION:
		{
			if (BR_ContextualToolbarsWnd* wnd = g_contextToolbarsWndManager.Get())
			{
				if (BR_ContextualToolbar* contextualToolbar = wnd->GetCurrentContextualToolbar())
				{
					bool autoClose;
					int toolbarId, positionX, positionY;
					if (contextualToolbar->GetContext(context, &toolbarId, &autoClose, &positionX, &positionY))
					{
						if (iCol == COL_TOOLBAR)
						{
							contextualToolbar->GetToolbarName(toolbarId, str, iStrMax);
						}
						else if (iCol == COL_AUTOCLOSE)
						{
							if (contextualToolbar->GetToolbarType(toolbarId) == 3)
								snprintf(str, iStrMax, "%s", autoClose ? UTF8_CHECKMARK : UTF8_MULTIPLICATION);
							else
								snprintf(str, iStrMax, "%s", "-");
						}
						else if (iCol == COL_POSITION)
						{
							if (contextualToolbar->GetToolbarType(toolbarId) == 3 && (positionX != 0 || positionY != 0))
								snprintf(str, iStrMax, "%s%d; %s%d", __LOCALIZE("X:","sws_DLG_181"), positionX, __LOCALIZE("Y:","sws_DLG_181"), positionY);
							else
								snprintf(str, iStrMax, "%s", "-");
						}
					}
				}
			}
		}
		break;
	}
}

void BR_ContextualToolbarsView::GetItemList (SWS_ListItemList* pList)
{
	for (int i = CONTEXT_START; i < CONTEXT_COUNT; i++)
	{
		int* context = &(g_listViewContexts[i]);
		pList->Add((SWS_ListItem*)context);
	}
}

bool BR_ContextualToolbarsView::OnItemSelChanging (SWS_ListItem* item, bool bSel)
{
	if (int* context = (int*)item)
	{
		if (BR_ContextualToolbar* toolbar = g_contextToolbarsWndManager.Get()->GetCurrentContextualToolbar())
			return toolbar->IsContextValid(*context) ? FALSE : TRUE;
	}
	return FALSE;
}

void BR_ContextualToolbarsView::OnItemDblClk (SWS_ListItem* item, int iCol)
{
	if (int* context = (int*)item)
	{
		if (BR_ContextualToolbarsWnd* wnd = g_contextToolbarsWndManager.Get())
		{
			if (BR_ContextualToolbar* toolbar = wnd->GetCurrentContextualToolbar())
			{
				int toolbarId;
				toolbar->GetContext(*context, &toolbarId, NULL, NULL, NULL);
				SWS_ListView* listView = wnd->GetListView();

				switch (iCol)
				{
					case COL_TOOLBAR:
					case COL_CONTEXT:
					{
						POINT p;
						GetCursorPos(&p);
						SendMessage(wnd->GetHWND(), WM_CONTEXTMENU, (WPARAM)wnd->GetListView(), MAKELPARAM(p.x, p.y));
					}
					break;

					case COL_AUTOCLOSE:
					{
						bool autoClose;
						toolbar->GetContext(*context, NULL, &autoClose, NULL, NULL);
						autoClose = !autoClose;

						int x = 0;
						while (int* selectedContext = (int*)listView->EnumSelected(&x)) // double click on OSX won't unselect other items so go through all
							toolbar->SetContext(*selectedContext, NULL, &autoClose, NULL, NULL);
					}
					break;

					case COL_POSITION:
					{
						if (toolbar->GetToolbarType(toolbarId) == 3)
						{
							int posX, posY;
							toolbar->GetContext(*context, NULL, NULL, &posX, &posY);

							if (wnd->GetPositionOffsetFromUser(posX, posY))
							{
								int x = 0;
								while (int* selectedContext = (int*)listView->EnumSelected(&x))
									toolbar->SetContext(*selectedContext, NULL, NULL, &posX, &posY);
							}
						}
					}
					break;
				}

				listView->Update();
			}
		}
	}
}

bool BR_ContextualToolbarsView::HideGridLines ()
{
	return true;
}

int BR_ContextualToolbarsView::OnItemSort (SWS_ListItem* item1, SWS_ListItem* item2)
{
	if (item1 && item2)
	{
		double i1 = *(int*)item1;
		double i2 = *(int*)item2;
		return (i1 > i2) ? 1 : (i1 < i2) ? - 1 : 0; // don't allow sorting
	}
	return 0;
}

/******************************************************************************
* Contextual toolbars window                                                  *
******************************************************************************/
BR_ContextualToolbarsWnd::BR_ContextualToolbarsWnd () :
SWS_DockWnd(IDD_BR_CONTEXTUAL_TOOLBARS, __LOCALIZE("Contextual toolbars","sws_DLG_181"), ""),
m_list           (NULL),
m_currentPreset  (0),
m_contextMenuCol (0)
{
	m_id.Set(CONTEXT_TOOLBARS_WND);
	Init();
}

BR_ContextualToolbarsWnd::~BR_ContextualToolbarsWnd ()
{
}

void BR_ContextualToolbarsWnd::Update ()
{
	int currentPreset = (int)SendDlgItemMessage(m_hwnd, IDC_PRESET, CB_GETCURSEL, 0, 0);
	BR_ContextualToolbar* currentToolbar = g_toolbarsManager.GetContextualToolbar(currentPreset);
	if (!currentToolbar)
		return;

	m_currentToolbar = *currentToolbar;
	m_currentPreset = currentPreset;
	m_list->Update();

	const auto &options = m_currentToolbar.GetOptions();
	const struct { int checkbox; int combo[2]; int value; } uiOptions[]{
		// Process check boxes without paired combos
		{ IDC_ALL_TOPMOST,            {}, options.topmost                },
		{ IDC_ALL_FOREGROUND,         {}, options.setToolbarToForeground },
		{ IDC_AUTOCLOSE_INACTIVE,     {}, options.autocloseInactive      },
		{ IDC_ARG_TAKE_ACTIVATE,      {}, options.arrangeActTake         },
		{ IDC_MIDI_CC_LANE_CLICKED,   {}, options.midiSetCCLane          },
		{ IDC_INLINE_CC_LANE_CLICKED, {}, options.inlineSetCCLane        },

		// Process check boxes paired with combos
		{ IDC_ALL_FOCUS,     { IDC_ALL_FOCUS_COMBO                      }, options.focus                },
		{ IDC_ALL_POS,       { IDC_ALL_POS_H_COMBO, IDC_ALL_POS_V_COMBO }, options.position             },
		{ IDC_TCP_TRACK,     { IDC_TCP_TRACK_COMBO                      }, options.tcpTrack             },
		{ IDC_TCP_ENV,       { IDC_TCP_ENV_COMBO                        }, options.tcpEnvelope          },
		{ IDC_MCP_TRACK,     { IDC_MCP_TRACK_COMBO                      }, options.mcpTrack             },
		{ IDC_ARG_TRACK,     { IDC_ARG_TRACK_COMBO                      }, options.arrangeTrack         },
		{ IDC_ARG_ITEM,      { IDC_ARG_ITEM_COMBO                       }, options.arrangeItem          },
		{ IDC_ARG_STRETCH,   { IDC_ARG_STRETCH_COMBO                    }, options.arrangeStretchMarker },
		{ IDC_ARG_TAKE_ENV,  { IDC_ARG_TAKE_ENV_COMBO                   }, options.arrangeTakeEnvelope  },
		{ IDC_ARG_TRACK_ENV, { IDC_ARG_TRACK_ENV_COMBO                  }, options.arrangeTrackEnvelope },
		{ IDC_INLINE_ITEM,   { IDC_INLINE_ITEM_COMBO                    }, options.inlineItem           },
	};

	for (const auto &option : uiOptions)
	{
		CheckDlgButton(m_hwnd, option.checkbox, option.value & OPTION_ENABLED);

		for (const int &currentCombo : option.combo)
		{
			if (!currentCombo)
				continue;

			EnableWindow(GetDlgItem(m_hwnd, currentCombo), option.value & OPTION_ENABLED);

			// Drop downs never contain information if option is enabled so remove it before comparing
			int currentOption = option.value & (~OPTION_ENABLED);

			// In case of two combos, eliminate options that belong to the other combo
			if (option.combo[0] && option.combo[1])
			{
				int comboOptionsToRemove = option.combo[&currentCombo == &option.combo[0] ? 1 : 0];
				int count = (int)SendDlgItemMessage(m_hwnd, comboOptionsToRemove, CB_GETCOUNT, 0, 0);
				for (int i = 0; i < count; ++i)
					currentOption &= ~((int)SendDlgItemMessage(m_hwnd, comboOptionsToRemove, CB_GETITEMDATA, i, 0));
			}

			// Finally find the appropriate selection
			int count = (int)SendDlgItemMessage(m_hwnd, currentCombo, CB_GETCOUNT, 0, 0);
			for (int i = 0; i < count; ++i)
			{
				if (currentOption == (int)SendDlgItemMessage(m_hwnd, currentCombo, CB_GETITEMDATA, i, 0))
				{
					SendDlgItemMessage(m_hwnd, currentCombo, CB_SETCURSEL, i, 0);
					break;
				}
				else if (i == count - 1)
					SendDlgItemMessage(m_hwnd, currentCombo, CB_SETCURSEL, 0, 0); // in case nothing is found, default to first entry
			}
		}
	}
}

bool BR_ContextualToolbarsWnd::CheckForModificationsAndSave (bool onClose)
{
	bool closeWnd = true;
	if (m_currentToolbar != *g_toolbarsManager.GetContextualToolbar(m_currentPreset))
	{
		WDL_FastString message; message.AppendFormatted(256, "%s%s%s%s%s", __LOCALIZE("Save changes to","sws_DLG_181"), " \'", this->GetPresetName(m_currentPreset).Get(), "\' ",  onClose ? __LOCALIZE("before closing?","sws_DLG_181") : __LOCALIZE("before switching presets?","sws_DLG_181"));
		int answer = MessageBox(m_hwnd, message.Get(), __LOCALIZE("SWS/BR - Warning","sws_mbox"), MB_YESNOCANCEL);
		if (answer == IDYES || answer == IDNO)
		{
			if (answer == IDYES)
				g_toolbarsManager.SetContextualToolbar(m_currentPreset, m_currentToolbar);
		}
		else if (answer == IDCANCEL)
		{
			SendDlgItemMessage(m_hwnd, IDC_PRESET, CB_SETCURSEL, m_currentPreset, 0);
			closeWnd = false;
		}
	}

	return closeWnd;
}

bool BR_ContextualToolbarsWnd::GetPositionOffsetFromUser (int& x, int&y)
{
	pair<int,int> positionOffset(x, y);
	INT_PTR result = DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_BR_CONTEXTUAL_TOOLBARS_POS), m_hwnd, BR_ContextualToolbarsWnd::PositionOffsetDialogProc, (LPARAM)&positionOffset);

	x = positionOffset.first;
	y = positionOffset.second;
	return (result == IDOK);
}

WDL_FastString BR_ContextualToolbarsWnd::GetPresetName (int preset)
{
	WDL_FastString presetName;
	presetName.AppendFormatted(256, "%s %.2d", __LOCALIZE("Preset", "sws_DLG_181"), preset + 1);
	return presetName;
}

WDL_DLGRET BR_ContextualToolbarsWnd::PositionOffsetDialogProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, uMsg, wParam, lParam))
		return r;

	static pair<int,int>* s_positionOffset = NULL;
	#ifndef _WIN32
		static bool s_positionSet = false;
	#endif

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			s_positionOffset = (pair<int,int>*)lParam;
			#ifdef _WIN32
				CenterDialog(hwnd, GetParent(hwnd), HWND_TOPMOST);
			#else
				s_positionSet = false;
			#endif

			char tmp[128];
			snprintf(tmp, sizeof(tmp), "%d", s_positionOffset->first); SetDlgItemText(hwnd, IDC_EDIT1, tmp);
			snprintf(tmp, sizeof(tmp), "%d", s_positionOffset->second); SetDlgItemText(hwnd, IDC_EDIT2, tmp);
		}
		break;

		#ifndef _WIN32
			case WM_ACTIVATE:
			{
				// SetWindowPos doesn't seem to work in WM_INITDIALOG on OSX
				// when creating a dialog with DialogBox so call here
				if (!s_positionSet)
					CenterDialog(hwnd, GetParent(hwnd), HWND_TOPMOST);
				s_positionSet = true;
			}
			break;
		#endif

		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case IDOK:
				{
					char tmp[128];
					GetDlgItemText(hwnd, IDC_EDIT1, tmp, 128); s_positionOffset->first  = atoi(tmp);
					GetDlgItemText(hwnd, IDC_EDIT2, tmp, 128); s_positionOffset->second = atoi(tmp);
					EndDialog(hwnd, IDOK);
				}
				break;

				case IDCANCEL:
				{
					EndDialog(hwnd, IDCANCEL);
				}
				break;
			}
		}
		break;

		case WM_DESTROY:
		{
			s_positionOffset = NULL;
		}
		break;
	}
	return 0;
}

BR_ContextualToolbar* BR_ContextualToolbarsWnd::GetCurrentContextualToolbar ()
{
	return &m_currentToolbar;
}

void BR_ContextualToolbarsWnd::OnInitDlg ()
{
	char currentPresetStr[64];
	GetPrivateProfileString(INI_SECTION, INI_KEY_CURRENT_PRESET, "0", currentPresetStr, sizeof(currentPresetStr), GetIniFileBR());
	m_currentPreset = atoi(currentPresetStr);
	if (m_currentPreset < 0 || m_currentPreset > PRESET_COUNT)
		m_currentPreset = 0;

	m_parentVwnd.SetRealParent(m_hwnd);
	m_vwnd_painter.SetGSC(WDL_STYLE_GetSysColor);

	m_list = new BR_ContextualToolbarsView(GetDlgItem(m_hwnd, IDC_LIST), GetDlgItem(m_hwnd, IDC_EDIT));
	m_pLists.Add(m_list);

	m_resize.init_item(IDC_LIST, 0.0, 0.0, 1.0, 1.0);
	m_resize.init_item(IDC_PRESET, 0.0, 0.0, 1.0, 0.0);

	const int fixedControls[] {
		IDC_ALL_BOX, IDC_ALL_FOCUS, IDC_ALL_FOCUS_COMBO, IDC_ALL_POS,
		IDC_ALL_POS_H_COMBO, IDC_ALL_POS_V_COMBO, IDC_ALL_TOPMOST,
		IDC_ALL_FOREGROUND, IDC_AUTOCLOSE_INACTIVE, IDC_TCP_BOX, IDC_TCP_TRACK,
		IDC_TCP_TRACK_COMBO, IDC_TCP_ENV, IDC_TCP_ENV_COMBO, IDC_MCP_BOX,
		IDC_MCP_TRACK, IDC_MCP_TRACK_COMBO, IDC_ARG_BOX, IDC_ARG_TRACK,
		IDC_ARG_TRACK_COMBO, IDC_ARG_ITEM, IDC_ARG_ITEM_COMBO, IDC_ARG_STRETCH,
		IDC_ARG_STRETCH_COMBO, IDC_ARG_TAKE_ENV, IDC_ARG_TAKE_ENV_COMBO,
		IDC_ARG_TRACK_ENV, IDC_ARG_TRACK_ENV_COMBO, IDC_ARG_TAKE_ACTIVATE,
		IDC_MIDI_BOX, IDC_MIDI_CC_LANE_CLICKED, IDC_INLINE_BOX, IDC_INLINE_ITEM,
		IDC_INLINE_ITEM_COMBO, IDC_INLINE_CC_LANE_CLICKED,
	};

	for (const int control : fixedControls)
		m_resize.init_item(control, 1.0, 0.0, 1.0, 0.0);

	m_resize.init_item(IDC_HELP_WIKI, 1.0, 1.0, 1.0, 1.0);
	m_resize.init_item(IDC_SAVE, 1.0, 1.0, 1.0, 1.0);

	// Preset dropdown
	WDL_UTF8_HookComboBox(GetDlgItem(m_hwnd, IDC_PRESET));
	for (int i = 0; i < PRESET_COUNT; ++i)
		SendDlgItemMessage(m_hwnd, IDC_PRESET, CB_ADDSTRING, 0, (LPARAM)this->GetPresetName(i).Get());
	SendDlgItemMessage(m_hwnd, IDC_PRESET, CB_SETCURSEL, m_currentPreset, 0);

	// Options: All
	int x;
	WDL_UTF8_HookComboBox(GetDlgItem(m_hwnd, IDC_ALL_FOCUS_COMBO));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ALL_FOCUS_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("All", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ALL_FOCUS_COMBO, CB_SETITEMDATA, x, (LPARAM)(FOCUS_ALL));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ALL_FOCUS_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Main window", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ALL_FOCUS_COMBO, CB_SETITEMDATA, x, (LPARAM)(FOCUS_MAIN));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ALL_FOCUS_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("MIDI editor", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ALL_FOCUS_COMBO, CB_SETITEMDATA, x, (LPARAM)(FOCUS_MIDI));

	WDL_UTF8_HookComboBox(GetDlgItem(m_hwnd, IDC_ALL_POS_H_COMBO));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ALL_POS_H_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Horizontal: left", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ALL_POS_H_COMBO, CB_SETITEMDATA, x, (LPARAM)(POSITION_H_LEFT));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ALL_POS_H_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Horizontal: middle", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ALL_POS_H_COMBO, CB_SETITEMDATA, x, (LPARAM)(POSITION_H_MIDDLE));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ALL_POS_H_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Horizontal: right", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ALL_POS_H_COMBO, CB_SETITEMDATA, x, (LPARAM)(POSITION_H_RIGHT));

	WDL_UTF8_HookComboBox(GetDlgItem(m_hwnd, IDC_ALL_POS_V_COMBO));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ALL_POS_V_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Vertical: bottom", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ALL_POS_V_COMBO, CB_SETITEMDATA, x, (LPARAM)(POSITION_V_BOTTOM));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ALL_POS_V_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Vertical: middle", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ALL_POS_V_COMBO, CB_SETITEMDATA, x, (LPARAM)(POSITION_V_MIDDLE));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ALL_POS_V_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Vertical: top", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ALL_POS_V_COMBO, CB_SETITEMDATA, x, (LPARAM)(POSITION_V_TOP));

	// Options: TCP
	WDL_UTF8_HookComboBox(GetDlgItem(m_hwnd, IDC_TCP_TRACK_COMBO));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_TCP_TRACK_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Select track", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_TCP_TRACK_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK|CLEAR_TRACK_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_TCP_TRACK_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Add track to selection", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_TCP_TRACK_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK));

	WDL_UTF8_HookComboBox(GetDlgItem(m_hwnd, IDC_TCP_ENV_COMBO));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_TCP_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Select envelope", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_TCP_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ENVELOPE));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_TCP_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Select envelope and parent track", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_TCP_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ENVELOPE|SELECT_TRACK|CLEAR_TRACK_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_TCP_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Select envelope and add parent track to selection", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_TCP_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ENVELOPE|SELECT_TRACK));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_TCP_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Select parent track", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_TCP_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK|CLEAR_TRACK_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_TCP_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Add parent track to selection", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_TCP_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK));

	// Options: MCP
	WDL_UTF8_HookComboBox(GetDlgItem(m_hwnd, IDC_MCP_TRACK_COMBO));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_MCP_TRACK_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Select track", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_MCP_TRACK_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK|CLEAR_TRACK_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_MCP_TRACK_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Add track to selection", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_MCP_TRACK_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK));

	// Options: Arrange
	WDL_UTF8_HookComboBox(GetDlgItem(m_hwnd, IDC_ARG_TRACK_COMBO));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_TRACK_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Select track", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ARG_TRACK_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK|CLEAR_TRACK_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_TRACK_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Add track to selection", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ARG_TRACK_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK));

	WDL_UTF8_HookComboBox(GetDlgItem(m_hwnd, IDC_ARG_ITEM_COMBO));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Select item", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM|CLEAR_ITEM_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Select item and parent track", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM|CLEAR_ITEM_SELECTION|SELECT_TRACK|CLEAR_TRACK_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Select item and add parent track to selection", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM|CLEAR_ITEM_SELECTION|SELECT_TRACK));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Select parent track", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK|CLEAR_TRACK_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Add item to selection", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Add item to selection and select parent track", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM|SELECT_TRACK|CLEAR_TRACK_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Add item and parent track to selection", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM|SELECT_TRACK));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Add parent track to selection", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK));

	WDL_UTF8_HookComboBox(GetDlgItem(m_hwnd, IDC_ARG_STRETCH_COMBO));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_STRETCH_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Select item", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ARG_STRETCH_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM|CLEAR_ITEM_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_STRETCH_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Select item and parent track", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ARG_STRETCH_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM|CLEAR_ITEM_SELECTION|SELECT_TRACK|CLEAR_TRACK_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_STRETCH_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Select item and add parent track to selection", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ARG_STRETCH_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM|CLEAR_ITEM_SELECTION|SELECT_TRACK));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_STRETCH_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Select parent track", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ARG_STRETCH_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK|CLEAR_TRACK_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_STRETCH_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Add item to selection", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ARG_STRETCH_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_STRETCH_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Add item to selection and select parent track", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ARG_STRETCH_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM|SELECT_TRACK|CLEAR_TRACK_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_STRETCH_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Add item and parent track to selection", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ARG_STRETCH_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM|SELECT_TRACK));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_STRETCH_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Add parent track to selection", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ARG_STRETCH_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK));

	WDL_UTF8_HookComboBox(GetDlgItem(m_hwnd, IDC_ARG_TAKE_ENV_COMBO));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_TAKE_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Select envelope", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ARG_TAKE_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ENVELOPE));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_TAKE_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Select envelope and parent item", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ARG_TAKE_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ENVELOPE|SELECT_ITEM|CLEAR_ITEM_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_TAKE_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Select envelope and add parent item to selection", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ARG_TAKE_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ENVELOPE|SELECT_ITEM));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_TAKE_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Select parent item", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ARG_TAKE_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM|CLEAR_ITEM_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_TAKE_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Add parent item to selection", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ARG_TAKE_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM));

	WDL_UTF8_HookComboBox(GetDlgItem(m_hwnd, IDC_ARG_TRACK_ENV_COMBO));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_TRACK_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Select envelope", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ARG_TRACK_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ENVELOPE));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_TRACK_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Select envelope and parent track", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ARG_TRACK_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ENVELOPE|SELECT_TRACK|CLEAR_TRACK_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_TRACK_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Select envelope and add parent track to selection", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ARG_TRACK_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ENVELOPE|SELECT_TRACK));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_TRACK_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Select parent track", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ARG_TRACK_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK|CLEAR_TRACK_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_TRACK_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Add parent track to selection", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_ARG_TRACK_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK));

	// Options: Inline MIDI editor
	WDL_UTF8_HookComboBox(GetDlgItem(m_hwnd, IDC_INLINE_ITEM_COMBO));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Select item", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM|CLEAR_ITEM_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Select item and parent track", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM|CLEAR_ITEM_SELECTION|SELECT_TRACK|CLEAR_TRACK_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Select item and add parent track to selection", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM|CLEAR_ITEM_SELECTION|SELECT_TRACK));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Select parent track", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK|CLEAR_TRACK_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Add item to selection", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Add item to selection and select parent track", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM|SELECT_TRACK|CLEAR_TRACK_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Add item and parent track to selection", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM|SELECT_TRACK));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Add parent track to selection", "sws_DLG_181"));
	SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK));

	this->Update();
}

void BR_ContextualToolbarsWnd::OnCommand (WPARAM wParam, LPARAM lParam)
{
	const int ctrl = LOWORD(wParam);

	switch (ctrl)
	{
		case IDC_PRESET:
		{
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				this->CheckForModificationsAndSave(false);
				this->Update();
			}
		}
		break;

		case IDC_SAVE:
		{
			g_toolbarsManager.SetContextualToolbar(m_currentPreset, m_currentToolbar);
			this->Update();
		}
		break;

		case IDC_HELP_WIKI:
		{
			ShellExecute(NULL, "open", "http://wiki.cockos.com/wiki/index.php/Contextual_toolbars_with_SWS", NULL, NULL, SW_SHOWNORMAL);
		}
		break;

		default:
		{
			const bool selChange = HIWORD(wParam) == CBN_SELCHANGE;
			auto &options = m_currentToolbar.GetOptions();
			struct { int checkbox; int combo[2]; int *value; } option{};

			if      (ctrl == IDC_ALL_TOPMOST)            option = { ctrl, {}, &options.topmost                };
			else if (ctrl == IDC_ALL_FOREGROUND)         option = { ctrl, {}, &options.setToolbarToForeground };
			else if (ctrl == IDC_AUTOCLOSE_INACTIVE)     option = { ctrl, {}, &options.autocloseInactive      };
			else if (ctrl == IDC_ARG_TAKE_ACTIVATE)      option = { ctrl, {}, &options.arrangeActTake         };
			else if (ctrl == IDC_MIDI_CC_LANE_CLICKED)   option = { ctrl, {}, &options.midiSetCCLane          };
			else if (ctrl == IDC_INLINE_CC_LANE_CLICKED) option = { ctrl, {}, &options.inlineSetCCLane        };
			else if (ctrl == IDC_ALL_FOCUS     || (ctrl == IDC_ALL_FOCUS_COMBO     && selChange)) option = { IDC_ALL_FOCUS,     { IDC_ALL_FOCUS_COMBO     }, &options.focus                     };
			else if (ctrl == IDC_TCP_TRACK     || (ctrl == IDC_TCP_TRACK_COMBO     && selChange)) option = { IDC_TCP_TRACK,     { IDC_TCP_TRACK_COMBO     }, &options.tcpTrack                  };
			else if (ctrl == IDC_TCP_ENV       || (ctrl == IDC_TCP_ENV_COMBO       && selChange)) option = { IDC_TCP_ENV,       { IDC_TCP_ENV_COMBO       }, &options.tcpEnvelope               };
			else if (ctrl == IDC_MCP_TRACK     || (ctrl == IDC_MCP_TRACK_COMBO     && selChange)) option = { IDC_MCP_TRACK,     { IDC_MCP_TRACK_COMBO     }, &options.mcpTrack                  };
			else if (ctrl == IDC_ARG_TRACK     || (ctrl == IDC_ARG_TRACK_COMBO     && selChange)) option = { IDC_ARG_TRACK,     { IDC_ARG_TRACK_COMBO     }, &options.arrangeTrack              };
			else if (ctrl == IDC_ARG_ITEM      || (ctrl == IDC_ARG_ITEM_COMBO      && selChange)) option = { IDC_ARG_ITEM,      { IDC_ARG_ITEM_COMBO      }, &options.arrangeItem               };
			else if (ctrl == IDC_ARG_STRETCH   || (ctrl == IDC_ARG_STRETCH_COMBO   && selChange)) option = { IDC_ARG_STRETCH,   { IDC_ARG_STRETCH_COMBO   }, &options.arrangeStretchMarker      };
			else if (ctrl == IDC_ARG_TAKE_ENV  || (ctrl == IDC_ARG_TAKE_ENV_COMBO  && selChange)) option = { IDC_ARG_TAKE_ENV,  { IDC_ARG_TAKE_ENV_COMBO  }, &options.arrangeTakeEnvelope       };
			else if (ctrl == IDC_ARG_TRACK_ENV || (ctrl == IDC_ARG_TRACK_ENV_COMBO && selChange)) option = { IDC_ARG_TRACK_ENV, { IDC_ARG_TRACK_ENV_COMBO }, &options.arrangeTrackEnvelope      };
			else if (ctrl == IDC_INLINE_ITEM   || (ctrl == IDC_INLINE_ITEM_COMBO   && selChange)) option = { IDC_INLINE_ITEM,   { IDC_INLINE_ITEM_COMBO   }, &options.inlineItem                };
			else if (ctrl == IDC_ALL_POS       || (ctrl == IDC_ALL_POS_H_COMBO     && selChange)
			                                   || (ctrl == IDC_ALL_POS_V_COMBO     && selChange)) option = { IDC_ALL_POS,       { IDC_ALL_POS_H_COMBO, IDC_ALL_POS_V_COMBO }, &options.position };
			else
				break;

			*option.value = !!IsDlgButtonChecked(m_hwnd, option.checkbox) ? OPTION_ENABLED : 0;

			for (const int combo : option.combo) {
				if (!combo)
					continue;

				EnableWindow(GetDlgItem(m_hwnd, combo), IsDlgButtonChecked(m_hwnd, option.checkbox));
				*option.value |= SendDlgItemMessage(m_hwnd, combo, CB_GETITEMDATA, SendDlgItemMessage(m_hwnd, combo, CB_GETCURSEL, 0, 0), 0);
			}
		}
		break;
	}
}

bool BR_ContextualToolbarsWnd::CloseOnCancel ()
{
	return this->CheckForModificationsAndSave(true);
}

void BR_ContextualToolbarsWnd::OnDestroy ()
{
	char currentPresetStr[512];
	snprintf(currentPresetStr, sizeof(currentPresetStr), "%d", m_currentPreset);
	WritePrivateProfileString(INI_SECTION, INI_KEY_CURRENT_PRESET, currentPresetStr, GetIniFileBR());
}

void BR_ContextualToolbarsWnd::GetMinSize (int* w, int* h)
{
	static int height = -1;

	if (height == -1)
	{
		RECT r;
		GetWindowRect(GetDlgItem(m_hwnd, IDC_SAVE), &r);
		height = abs(r.bottom - r.top); // abs because of OSX

		ScreenToClient(m_hwnd, (LPPOINT)&r);
		height += r.top + 4;
	}

	WritePtr(w, 490);
	WritePtr(h, height);
}

int BR_ContextualToolbarsWnd::OnKey (MSG* msg, int iKeyState)
{
	return 0;
}

bool BR_ContextualToolbarsWnd::ReprocessContextMenu()
{
	return false;
}

bool BR_ContextualToolbarsWnd::NotifyOnContextMenu ()
{
	return false;
}

HMENU BR_ContextualToolbarsWnd::OnContextMenu (int x, int y, bool* wantDefaultItems)
{
	HMENU menu = NULL;
	int column;
	if ((int*)m_list->GetHitItem(x, y, &column))
	{
		menu             = CreatePopupMenu();
		m_contextMenuCol = column;
		WritePtr(wantDefaultItems, false);

		switch (column)
		{
			case COL_AUTOCLOSE:
			{
				int x = 0;
				int toolbarCount = 0;
				int enabledCount = 0;
				while (int* selectedContext = (int*)m_list->EnumSelected(&x))
				{
					int toolbarId; bool autoClose;
					if (m_currentToolbar.GetContext(*selectedContext, &toolbarId, &autoClose, NULL, NULL))
					{
						if (m_currentToolbar.GetToolbarType(toolbarId) == 3)
						{
							++toolbarCount;
							if (autoClose) ++enabledCount;
						}
					}
				}

				bool autoCloseEnabled = (toolbarCount > 0 && enabledCount == toolbarCount);
				AddToMenu(menu, __LOCALIZE("Enable toolbar auto close", "sws_DLG_181"), (autoCloseEnabled ? 1 : 2), -1, false, (autoCloseEnabled ? MF_CHECKED : MF_UNCHECKED));
			}
			break;

			case COL_POSITION:
			{
				AddToMenu(menu, __LOCALIZE("Set toolbar position offset...", "sws_DLG_181"), 1, -1, false);
			}
			break;

			case COL_CONTEXT:
			case COL_TOOLBAR:
			{
				for (int i = 0; i < m_currentToolbar.CountToolbars(); ++i)
				{
					bool validEntry = true;

					// Check if selected contexts can inherit parent
					if (m_currentToolbar.GetToolbarType(i) == 1)
					{
						int x = 0;
						bool inheritParent = false;
						while (int* selectedContext = (int*)m_list->EnumSelected(&x))
						{
							if (m_currentToolbar.CanContextInheritParent(*selectedContext))
							{
								inheritParent = true;
								break;
							}
						}
						validEntry = inheritParent;
					}
					// Check if selected contexts can follow item context
					else if (m_currentToolbar.GetToolbarType(i) == 2)
					{
						int x = 0;
						bool followItem = true;
						while (int* selectedContext = (int*)m_list->EnumSelected(&x))
						{
							if (!m_currentToolbar.CanContextFollowItem(*selectedContext))
							{
								followItem = false;
								break;
							}
						}
						validEntry = followItem;
					}

					if (validEntry)
					{
						if (m_currentToolbar.IsFirstToolbar(i) || m_currentToolbar.IsFirstMidiToolbar(i))
							AddToMenu(menu, SWS_SEPARATOR, 0);

						char toolbarName[512];
						if (m_currentToolbar.GetToolbarName(i, toolbarName, sizeof(toolbarName)))
							AddToMenu(menu, toolbarName, i + 1, -1, false); // i + 1 -> because context values can only be > 0
					}
				}
			}
			break;
		}
	}
	return menu;
}

void BR_ContextualToolbarsWnd::ContextMenuReturnId (int id)
{
	if (id)
	{
		switch (m_contextMenuCol)
		{
			case COL_AUTOCLOSE:
			{
				bool autoClose = (id == 2);
				int x = 0;
				while (int* context = (int*)m_list->EnumSelected(&x))
				{
					int toolbarId;
					if (m_currentToolbar.GetContext(*context, &toolbarId, NULL, NULL, NULL))
						m_currentToolbar.SetContext(*context, NULL, &autoClose, NULL, NULL);
				}
			}
			break;

			case COL_POSITION:
			{
				int positionX = 0;
				int positionY = 0;

				// Find "default" position settings for the dialog
				int x = 0;
				while (int* context = (int*)m_list->EnumSelected(&x))
				{
					int toolbarId, posX, posY;
					if (m_currentToolbar.GetContext(*context, &toolbarId, NULL, &posX, &posY) && m_currentToolbar.GetToolbarType(toolbarId) == 3)
					{
						positionX = posX;
						positionY = posY;
						break;
					}
				}

				// Update
				if (this->GetPositionOffsetFromUser(positionX, positionY))
				{
					int x = 0;
					while (int* context = (int*)m_list->EnumSelected(&x))
					{
						int toolbarId;
						if (m_currentToolbar.GetContext(*context, &toolbarId, NULL, NULL, NULL))
							m_currentToolbar.SetContext(*context, NULL, NULL, &positionX, &positionY);
					}
				}
			}
			break;

			case COL_CONTEXT:
			case COL_TOOLBAR:
			{
				int toolbarId = id - 1; // id - 1 -> see this->OnContextMenu()
				int x = 0;
				while (int* context = (int*)m_list->EnumSelected(&x))
					m_currentToolbar.SetContext(*context, &toolbarId, NULL, NULL, NULL);
			}
			break;
		}
	}
	m_list->Update();
}

/******************************************************************************
* Contextual toolbars init/exit                                               *
******************************************************************************/
int ContextualToolbarsInitExit (bool init)
{
	if (init)
	{
		for (int i = CONTEXT_START; i < CONTEXT_COUNT; ++i)
			g_listViewContexts[i] = i;
		g_contextToolbarsWndManager.Init();

	}
	else
	{
		g_contextToolbarsWndManager.Delete();
		BR_ContextualToolbar::Cleaup();
	}
	return 1;
}

/******************************************************************************
* Commands                                                                    *
******************************************************************************/
void ContextualToolbarsOptions (COMMAND_T* ct)
{
	if (BR_ContextualToolbarsWnd* dialog = g_contextToolbarsWndManager.Create())
		dialog->Show(true, true);
}

void ToggleContextualToolbar (COMMAND_T* ct)
{
	int id         = abs((int)ct->user) - 1;
	bool exclusive = ((int)ct->user < 0);
	if (BR_ContextualToolbar* contextualToolbar = g_toolbarsManager.GetContextualToolbar(id))
		contextualToolbar->LoadToolbar(exclusive);
}

void ToggleContextualToolbar (COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd)
{
	ToggleContextualToolbar(ct);
}

/******************************************************************************
* Toggle states                                                               *
******************************************************************************/
int IsContextualToolbarsOptionsVisible (COMMAND_T* ct)
{
	if (BR_ContextualToolbarsWnd* dialog = g_contextToolbarsWndManager.Get())
		return (int)dialog->IsWndVisible();
	return 0;
}

int IsContextualToolbarVisible (COMMAND_T* ct)
{
	if (BR_ContextualToolbar* contextualToolbar = g_toolbarsManager.GetContextualToolbar(abs((int)ct->user) - 1))
		return (int)contextualToolbar->AreAssignedToolbarsOpened();
	return 0;
}
