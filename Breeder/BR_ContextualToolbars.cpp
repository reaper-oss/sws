/******************************************************************************
/ BR_ContextualToolbars.cpp
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
#include "BR_ContextualToolbars.h"
#include "BR_EnvTools.h"
#include "BR_MouseUtil.h"
#include "BR_Util.h"
#include "BR.h"
#include "../SnM/SnM_Dlg.h"
#include "../SnM/SnM_Util.h"
#include "../reaper/localize.h"

/******************************************************************************
* Constants                                                                   *
******************************************************************************/
const char* const CONTEXT_TOOLBARS_WND      = "BR - ContextualToolbars WndPos" ;
const char* const CONTEXT_TOOLBARS_VIEW_WND = "BR - ContextualToolbarsView WndPos";
const char* const INI_SECTION               = "ContextualToolbars";
const char* const INI_KEY_CONTEXTS          = "ContextsPreset_";
const char* const INI_KEY_OPTIONS           = "SettingsPreset_";
const char* const INI_KEY_CURRENT_PRESET    = "DlgPreset";

const int PRESET_COUNT = 8;

// Built-in toolbar actions
const int DO_NOTHING            = 0xF001;
const int INHERIT_PARENT        = 0xF002;
const int FOLLOW_ITEM_CONTEXT   = 0xF003;
const int TOOLBAR_1_MOUSE       = 41111;
const int TOOLBAR_2_MOUSE       = 41112;
const int TOOLBAR_3_MOUSE       = 41113;
const int TOOLBAR_4_MOUSE       = 41114;
const int TOOLBAR_5_MOUSE       = 41655;
const int TOOLBAR_6_MOUSE       = 41656;
const int TOOLBAR_7_MOUSE       = 41657;
const int TOOLBAR_8_MOUSE       = 41658;
const int MIDI_TOOLBAR_1_MOUSE  = 41640;
const int MIDI_TOOLBAR_2_MOUSE  = 41641;
const int MIDI_TOOLBAR_3_MOUSE  = 41642;
const int MIDI_TOOLBAR_4_MOUSE  = 41643;
const int TOOLBAR_1_TOGGLE      = 41679;
const int TOOLBAR_2_TOGGLE      = 41680;
const int TOOLBAR_3_TOGGLE      = 41681;
const int TOOLBAR_4_TOGGLE      = 41682;
const int TOOLBAR_5_TOGGLE      = 41683;
const int TOOLBAR_6_TOGGLE      = 41684;
const int TOOLBAR_7_TOGGLE      = 41685;
const int TOOLBAR_8_TOGGLE      = 41686;
const int MIDI_TOOLBAR_1_TOGGLE = 41687;
const int MIDI_TOOLBAR_2_TOGGLE = 41688;
const int MIDI_TOOLBAR_3_TOGGLE = 41689;
const int MIDI_TOOLBAR_4_TOGGLE = 41690;

// Options flags (never change values - used for state saving to .ini)
const int OPTION_ENABLED          = 0x1;
const int SELECT_ITEM             = 0x2;
const int SELECT_TRACK            = 0x4;
const int SELECT_ENVELOPE         = 0x8;
const int CLEAR_ITEM_SELECTION    = 0x10;
const int CLEAR_TRACK_SELECTION   = 0x20;
const int FOCUS_ALL               = 0x40;
const int FOCUS_MAIN              = 0x80;
const int FOCUS_MIDI              = 0x100;

// List view columns
static SWS_LVColumn g_cols[] =
{
	{168, 0, "Context"},
	{155, 0, "Toolbar (right-click to change)"},
};

enum
{
	COL_CONTEXT = 0,
	COL_TOOLBAR,
	COL_COUNT
};

/******************************************************************************
* Globals                                                                     *
******************************************************************************/
static SNM_WindowManager<BR_ContextualToolbarsWnd> g_contextToolbarsWndManager(CONTEXT_TOOLBARS_WND);
static BR_ContextualToolbarsManager g_toolbarsManager;

/******************************************************************************
* Context toolbars                                                            *
******************************************************************************/
BR_ContextualToolbar::BR_ContextualToolbar () :
m_mode (0)
{
	for (int i = 0; i < CONTEXT_COUNT; ++i)
	{
		m_mouseActions[i]  = DO_NOTHING;
		m_toggleActions[i] = DO_NOTHING;
	}
	this->UpdateInternals();
}

BR_ContextualToolbar::BR_ContextualToolbar (const BR_ContextualToolbar& contextualToolbar) :
m_mode (contextualToolbar.m_mode)
{
	std::copy(contextualToolbar.m_mouseActions, contextualToolbar.m_mouseActions + CONTEXT_COUNT, m_mouseActions);
	std::copy(contextualToolbar.m_toggleActions, contextualToolbar.m_toggleActions + CONTEXT_COUNT, m_toggleActions);
	m_options        = contextualToolbar.m_options;
	m_activeContexts = contextualToolbar.m_activeContexts;
}

BR_ContextualToolbar::~BR_ContextualToolbar ()
{
}

BR_ContextualToolbar& BR_ContextualToolbar::operator= (const BR_ContextualToolbar& contextualToolbar)
{
	if (this == &contextualToolbar)
		return *this;

	std::copy(contextualToolbar.m_mouseActions, contextualToolbar.m_mouseActions + CONTEXT_COUNT, m_mouseActions);
	std::copy(contextualToolbar.m_toggleActions, contextualToolbar.m_toggleActions + CONTEXT_COUNT, m_toggleActions);
	m_options        = contextualToolbar.m_options;
	m_activeContexts = contextualToolbar.m_activeContexts;
	m_mode           = contextualToolbar.m_mode;

	return *this;
}

bool BR_ContextualToolbar::operator== (const BR_ContextualToolbar& contextualToolbar)
{
	for (int i = CONTEXT_START; i < CONTEXT_COUNT; ++i)
	{
		if (!g_toolbarsManager.IsContextValid(i))
			continue;
		else if (contextualToolbar.m_mouseActions[i] != m_mouseActions[i] || contextualToolbar.m_toggleActions[i] != m_toggleActions[i])
			return false;
	}

	if (contextualToolbar.m_options.focus                != m_options.focus)                return false;
	if (contextualToolbar.m_options.tcpTrack             != m_options.tcpTrack)             return false;
	if (contextualToolbar.m_options.tcpEnvelope          != m_options.tcpEnvelope)          return false;
	if (contextualToolbar.m_options.mcpTrack             != m_options.mcpTrack)             return false;
	if (contextualToolbar.m_options.arrangeTrack         != m_options.arrangeTrack)         return false;
	if (contextualToolbar.m_options.arrangeItem          != m_options.arrangeItem)          return false;
	if (contextualToolbar.m_options.arrangeTrackEnvelope != m_options.arrangeTrackEnvelope) return false;
	if (contextualToolbar.m_options.arrangeTakeEnvelope  != m_options.arrangeTakeEnvelope)  return false;
	if (contextualToolbar.m_options.arrangeActTake       != m_options.arrangeActTake)       return false;
	if (contextualToolbar.m_options.midiSetCCLane        != m_options.midiSetCCLane)        return false;
	if (contextualToolbar.m_options.inlineItem           != m_options.inlineItem)           return false;
	if (contextualToolbar.m_options.inlineSetCCLane      != m_options.inlineSetCCLane)      return false;

	return true;
}

bool BR_ContextualToolbar::operator!= (const BR_ContextualToolbar& contextualToolbar)
{
	return !(*this == contextualToolbar);
}

void BR_ContextualToolbar::LoadToolbar ()
{
	if (this->AreAssignedToolbarsOpened())
	{
		this->CloseAllAssignedToolbars();
	}
	else
	{
		BR_MouseContextInfo mouseInfo(m_mode, true);
		BR_ContextualToolbar::ExecuteOnToolbarLoad executeOnToolbarLoad;

		int context = -1;
		if      (!strcmp(mouseInfo.GetWindow(), "transport"))   context = this->FindTransportToolbar(mouseInfo, executeOnToolbarLoad);
		else if (!strcmp(mouseInfo.GetWindow(), "ruler"))       context = this->FindRulerToolbar(mouseInfo, executeOnToolbarLoad);
		else if (!strcmp(mouseInfo.GetWindow(), "tcp"))         context = this->FindTcpToolbar(mouseInfo, executeOnToolbarLoad);
		else if (!strcmp(mouseInfo.GetWindow(), "mcp"))         context = this->FindMcpToolbar(mouseInfo, executeOnToolbarLoad);
		else if (!strcmp(mouseInfo.GetWindow(), "arrange"))     context = this->FindArrangeToolbar(mouseInfo, executeOnToolbarLoad);
		else if (!strcmp(mouseInfo.GetWindow(), "midi_editor")) context = (mouseInfo.IsInlineMidi()) ? this->FindInlineMidiToolbar(mouseInfo, executeOnToolbarLoad) :  this->FindMidiToolbar(mouseInfo, executeOnToolbarLoad);

		if (context != -1 && m_mouseActions[context] != DO_NOTHING)
		{
			bool update = false;

			// Execute these before showing toolbar (otherwise toolbar can end up behind mixer/MIDi editor etc... due to focus changes)
			if (executeOnToolbarLoad.setCCLaneAsClicked)
				mouseInfo.SetDetectedCCLaneAsLastClicked();

			if (executeOnToolbarLoad.setFocus)
			{
				GetSetFocus(true, &executeOnToolbarLoad.focusHwnd, &executeOnToolbarLoad.focusContext);
			}

			if (executeOnToolbarLoad.envelopeToSelect)
			{
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
				update = true;
			}

			// Toggle toolbar
			Main_OnCommand(m_mouseActions[context], 0);

			// Track selection
			bool undoTrack = false;
			if (executeOnToolbarLoad.trackToSelect)
			{
				if (executeOnToolbarLoad.clearTrackSelection)
				{
					MediaTrack* masterTrack = GetMasterTrack(NULL);
					if (masterTrack != executeOnToolbarLoad.trackToSelect && GetMediaTrackInfo_Value(masterTrack, "I_SELECTED"))
					{
						SetMediaTrackInfo_Value(masterTrack, "I_SELECTED", 0);
						undoTrack = true;
					}
					for (int i = 0; i < CountTracks(NULL); ++i)
					{
						MediaTrack* track = GetTrack(NULL, i);
						if (track != executeOnToolbarLoad.trackToSelect && GetMediaTrackInfo_Value(track, "I_SELECTED"))
						{
							SetMediaTrackInfo_Value(track, "I_SELECTED", 0);
							undoTrack = true;
						}
					}
				}
				if (!GetMediaTrackInfo_Value(executeOnToolbarLoad.trackToSelect, "I_SELECTED"))
				{
					SetMediaTrackInfo_Value(executeOnToolbarLoad.trackToSelect, "I_SELECTED", 1);
					undoTrack = true;
				}
			}

			// Item selection
			bool undoItem = false;
			if (executeOnToolbarLoad.itemToSelect)
			{
				if (executeOnToolbarLoad.clearItemSelection)
				{
					for (int i = 0; i < CountMediaItems(NULL); ++i)
					{
						MediaItem* item = GetMediaItem(NULL, i);
						if (item != executeOnToolbarLoad.itemToSelect && GetMediaItemInfo_Value(item, "B_UISEL"))
						{
							SetMediaItemInfo_Value(item, "B_UISEL", false);
							undoItem = true;
						}
					}
				}

				if (!GetMediaItemInfo_Value(executeOnToolbarLoad.itemToSelect, "B_UISEL"))
				{
					SetMediaItemInfo_Value(executeOnToolbarLoad.itemToSelect, "B_UISEL", true);
					undoItem = true;
				}
				update = true;
			}

			// Activate take
			bool undoTake = false;
			if (executeOnToolbarLoad.takeIdToActivate != -1 && (int)GetMediaItemInfo_Value(executeOnToolbarLoad.takeParent, "I_CURTAKE") != executeOnToolbarLoad.takeIdToActivate)
			{
				GetSetMediaItemInfo(executeOnToolbarLoad.takeParent, "I_CURTAKE", &executeOnToolbarLoad.takeIdToActivate);
				undoTake = true;
				update = true;
			}

			// Create undo point if needed
			int undoMask; GetConfig("undomask", undoMask); // check "Create undo points for item/track selection" option
			if (!GetBit(undoMask, 0))
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

				if (undoMsg)
					Undo_OnStateChangeEx2(NULL, undoMsg, UNDO_STATE_ALL, -1);
			}

			// Make window always on top
			if ((m_options.topmost & OPTION_ENABLED))
				g_toolbarsManager.SetToolbarAlwaysOnTop(m_mouseActions[context]);

			if (update)
				UpdateArrange();
		}
	}
}

bool BR_ContextualToolbar::GetContext (int context, int* mouseAction, int* toggleAction)
{
	if (g_toolbarsManager.IsContextValid(context))
	{
		WritePtr(mouseAction, m_mouseActions[context]);
		WritePtr(toggleAction, m_toggleActions[context]);
		return true;
	}
	else
	{
		WritePtr(mouseAction, DO_NOTHING);
		WritePtr(toggleAction, DO_NOTHING);
		return false;
	}
}

bool BR_ContextualToolbar::SetContext (int context, int mouseAction, int toggleAction)
{
	if (g_toolbarsManager.IsContextValid(context))
	{
		if (mouseAction == INHERIT_PARENT && !g_toolbarsManager.CanContextInheritParent(context))
			return false;
		else if (mouseAction == FOLLOW_ITEM_CONTEXT && !g_toolbarsManager.CanContextFollowItem(context))
			return false;
		else
		{
			m_mouseActions[context]  = mouseAction;
			m_toggleActions[context] = toggleAction;
			this->UpdateInternals();
			return true;
		}
	}
	return false;
}

bool  BR_ContextualToolbar::DisableContext (int context, bool inheritParent)
{
	if (g_toolbarsManager.IsContextValid(context))
	{
		m_mouseActions[context]  = (inheritParent) ? INHERIT_PARENT : DO_NOTHING;
		m_toggleActions[context] = (inheritParent) ? INHERIT_PARENT : DO_NOTHING;
		return true;
	}
	return false;
}

bool BR_ContextualToolbar::GetToolbarName (int context, char* name, int nameSz)
{
	_snprintf(name, nameSz, "%s", "");

	if (g_toolbarsManager.IsContextValid(context))
	{
		if (m_mouseActions[context] == DO_NOTHING)
		{
			_snprintf(name, nameSz, "%s", __LOCALIZE("Do nothing", "sws_DLG_181"));
			return true;
		}
		else if (m_mouseActions[context] == INHERIT_PARENT)
		{
			_snprintf(name, nameSz, "%s", __LOCALIZE("Inherit parent", "sws_DLG_181"));
			return true;
		}
		else if (m_mouseActions[context] == FOLLOW_ITEM_CONTEXT)
		{
			_snprintf(name, nameSz, "%s", __LOCALIZE("Follow item context", "sws_DLG_181"));
			return true;
		}
		else if (g_toolbarsManager.GetToolbarName(m_mouseActions[context], name, nameSz))
		{
			return true;
		}
	}

	return false;
}

void BR_ContextualToolbar::SetOptionsAll (int* focus, int* topmost)
{
	ReadPtr(focus,   m_options.focus);
	ReadPtr(topmost, m_options.topmost);
}

void BR_ContextualToolbar::GetOptionsAll (int* focus, int* topmost)
{
	WritePtr(focus,   m_options.focus);
	WritePtr(topmost, m_options.topmost);
}

void BR_ContextualToolbar::SetOptionsTcp (int* track, int* envelope)
{
	ReadPtr(track,    m_options.tcpTrack);
	ReadPtr(envelope, m_options.tcpEnvelope);
}

void BR_ContextualToolbar::GetOptionsTcp (int* track, int* envelope)
{
	WritePtr(track,    m_options.tcpTrack);
	WritePtr(envelope, m_options.tcpEnvelope);
}

void BR_ContextualToolbar::SetOptionsMcp (int* track)
{
	ReadPtr(track, m_options.mcpTrack);
}

void BR_ContextualToolbar::GetOptionsMcp (int* track)
{
	WritePtr(track, m_options.mcpTrack);
}

void BR_ContextualToolbar::SetOptionsArrange (int* track, int* item, int* trackEnvelope, int* takeEnvelope, int* activateTake)
{
	ReadPtr(track,           m_options.arrangeTrack);
	ReadPtr(item,            m_options.arrangeItem);
	ReadPtr(trackEnvelope,   m_options.arrangeTrackEnvelope);
	ReadPtr(takeEnvelope,    m_options.arrangeTakeEnvelope);
	ReadPtr(activateTake,    m_options.arrangeActTake);
}

void BR_ContextualToolbar::GetOptionsArrange (int* track, int* item, int* trackEnvelope, int* takeEnvelope, int* activateTake)
{
	WritePtr(track,           m_options.arrangeTrack);
	WritePtr(item,            m_options.arrangeItem);
	WritePtr(trackEnvelope,   m_options.arrangeTrackEnvelope);
	WritePtr(takeEnvelope,    m_options.arrangeTakeEnvelope);
	WritePtr(activateTake,    m_options.arrangeActTake);
}

void BR_ContextualToolbar::SetOptionsMIDI (int* setCCLaneAsClicked)
{
	ReadPtr(setCCLaneAsClicked, m_options.midiSetCCLane);
}

void BR_ContextualToolbar::GetOptionsMIDI (int* setCCLaneAsClicked)
{
	WritePtr(setCCLaneAsClicked, m_options.midiSetCCLane);
}

void BR_ContextualToolbar::SetOptionsInline (int* item, int* setCCLaneAsClicked)
{
	ReadPtr(item,               m_options.inlineItem);
	ReadPtr(setCCLaneAsClicked, m_options.inlineSetCCLane);
}

void BR_ContextualToolbar::GetOptionsInline (int* item, int* setCCLaneAsClicked)
{
	WritePtr(item,               m_options.inlineItem);
	WritePtr(setCCLaneAsClicked, m_options.inlineSetCCLane);
}

void BR_ContextualToolbar::ExportConfig (char* contexts, int contextsSz, char* options, int optionsSz)
{
	if (contexts)
	{
		int mouseActions[CONTEXT_COUNT];
		for (int i = CONTEXT_START; i < CONTEXT_COUNT; ++i)
		{
			if      (m_mouseActions[i] == DO_NOTHING)           mouseActions[i] = 1;
			else if (m_mouseActions[i] == INHERIT_PARENT)       mouseActions[i] = 2;
			else if (m_mouseActions[i] == FOLLOW_ITEM_CONTEXT)  mouseActions[i] = 3;
			else if (m_mouseActions[i] == TOOLBAR_1_MOUSE)      mouseActions[i] = 4;
			else if (m_mouseActions[i] == TOOLBAR_2_MOUSE)      mouseActions[i] = 5;
			else if (m_mouseActions[i] == TOOLBAR_3_MOUSE)      mouseActions[i] = 6;
			else if (m_mouseActions[i] == TOOLBAR_4_MOUSE)      mouseActions[i] = 7;
			else if (m_mouseActions[i] == TOOLBAR_5_MOUSE)      mouseActions[i] = 8;
			else if (m_mouseActions[i] == TOOLBAR_6_MOUSE)      mouseActions[i] = 9;
			else if (m_mouseActions[i] == TOOLBAR_7_MOUSE)      mouseActions[i] = 10;
			else if (m_mouseActions[i] == TOOLBAR_8_MOUSE)      mouseActions[i] = 11;
			else if (m_mouseActions[i] == MIDI_TOOLBAR_1_MOUSE) mouseActions[i] = 12;
			else if (m_mouseActions[i] == MIDI_TOOLBAR_2_MOUSE) mouseActions[i] = 13;
			else if (m_mouseActions[i] == MIDI_TOOLBAR_3_MOUSE) mouseActions[i] = 14;
			else if (m_mouseActions[i] == MIDI_TOOLBAR_4_MOUSE) mouseActions[i] = 15;
			else                                                mouseActions[i] = 1;
		}

		_snprintfSafe(
					  contexts,
					  contextsSz,
					  "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
					  mouseActions[TRANSPORT],
					  mouseActions[RULER],
					  mouseActions[RULER_REGIONS],
					  mouseActions[RULER_MARKERS],
					  mouseActions[RULER_TEMPO],
					  mouseActions[RULER_TIMELINE],
					  mouseActions[TCP],
					  mouseActions[TCP_EMPTY],
					  mouseActions[TCP_TRACK],
					  mouseActions[TCP_TRACK_MASTER],
					  mouseActions[TCP_ENVELOPE],
					  mouseActions[TCP_ENVELOPE_VOLUME],
					  mouseActions[TCP_ENVELOPE_PAN],
					  mouseActions[TCP_ENVELOPE_WIDTH],
					  mouseActions[TCP_ENVELOPE_MUTE],
					  mouseActions[TCP_ENVELOPE_PLAYRATE],
					  mouseActions[TCP_ENVELOPE_TEMPO],
					  mouseActions[MCP],
					  mouseActions[MCP_EMPTY],
					  mouseActions[MCP_TRACK],
					  mouseActions[MCP_TRACK_MASTER],
					  mouseActions[ARRANGE],
					  mouseActions[ARRANGE_EMPTY],
					  mouseActions[ARRANGE_TRACK],
					  mouseActions[ARRANGE_TRACK_EMPTY],
					  mouseActions[ARRANGE_TRACK_ITEM],
					  mouseActions[ARRANGE_TRACK_ITEM_AUDIO],
					  mouseActions[ARRANGE_TRACK_ITEM_MIDI],
					  mouseActions[ARRANGE_TRACK_ITEM_VIDEO],
					  mouseActions[ARRANGE_TRACK_ITEM_EMPTY],
					  mouseActions[ARRANGE_TRACK_ITEM_CLICK],
					  mouseActions[ARRANGE_TRACK_ITEM_TIMECODE],
					  mouseActions[ARRANGE_TRACK_ITEM_STRETCH_MARKER],
					  mouseActions[ARRANGE_TRACK_TAKE_ENVELOPE],
					  mouseActions[ARRANGE_TRACK_TAKE_ENVELOPE_VOLUME],
					  mouseActions[ARRANGE_TRACK_TAKE_ENVELOPE_PAN],
					  mouseActions[ARRANGE_TRACK_TAKE_ENVELOPE_MUTE],
					  mouseActions[ARRANGE_TRACK_TAKE_ENVELOPE_PITCH],
					  mouseActions[ARRANGE_ENVELOPE_TRACK],
					  mouseActions[ARRANGE_ENVELOPE_TRACK_VOLUME],
					  mouseActions[ARRANGE_ENVELOPE_TRACK_PAN],
					  mouseActions[ARRANGE_ENVELOPE_TRACK_WIDTH],
					  mouseActions[ARRANGE_ENVELOPE_TRACK_MUTE],
					  mouseActions[ARRANGE_ENVELOPE_TRACK_PITCH],
					  mouseActions[ARRANGE_ENVELOPE_TRACK_PLAYRATE],
					  mouseActions[ARRANGE_ENVELOPE_TRACK_TEMPO],
					  mouseActions[MIDI_EDITOR],
					  mouseActions[MIDI_EDITOR_RULER],
					  mouseActions[MIDI_EDITOR_PIANO],
					  mouseActions[MIDI_EDITOR_PIANO_NAMED],
					  mouseActions[MIDI_EDITOR_NOTES],
					  mouseActions[MIDI_EDITOR_CC_LANE],
					  mouseActions[MIDI_EDITOR_CC_SELECTOR],
					  mouseActions[INLINE_MIDI_EDITOR],
					  mouseActions[INLINE_MIDI_EDITOR_PIANO],
					  mouseActions[INLINE_MIDI_EDITOR_PIANO_NAMED],
					  mouseActions[INLINE_MIDI_EDITOR_NOTES],
					  mouseActions[INLINE_MIDI_EDITOR_CC_LANE]
		);
	}

	if (options)
	{
		_snprintfSafe(
					  options,
					  optionsSz,
					  "%d %d %d %d %d %d %d %d %d %d %d %d %d",
					  m_options.focus,
					  m_options.topmost,
					  m_options.tcpTrack,
					  m_options.tcpEnvelope,
					  m_options.mcpTrack,
					  m_options.arrangeTrack,
					  m_options.arrangeItem,
					  m_options.arrangeTrackEnvelope,
					  m_options.arrangeTakeEnvelope,
					  m_options.arrangeActTake,
					  m_options.midiSetCCLane,
					  m_options.inlineItem,
					  m_options.inlineSetCCLane
		);
	}
}

void BR_ContextualToolbar::ImportConfig (const char* contexts, const char* options)
{
	if (contexts)
	{
		LineParser lp(false);
		lp.parse(contexts);

		m_mouseActions[TRANSPORT]                          = (lp.getnumtokens() > 0)  ? lp.gettoken_int(0)  : 1;
		m_mouseActions[RULER]                              = (lp.getnumtokens() > 1)  ? lp.gettoken_int(1)  : 1;
		m_mouseActions[RULER_REGIONS]                      = (lp.getnumtokens() > 2)  ? lp.gettoken_int(2)  : 2;
		m_mouseActions[RULER_MARKERS]                      = (lp.getnumtokens() > 3)  ? lp.gettoken_int(3)  : 2;
		m_mouseActions[RULER_TEMPO]                        = (lp.getnumtokens() > 4)  ? lp.gettoken_int(4)  : 2;
		m_mouseActions[RULER_TIMELINE]                     = (lp.getnumtokens() > 5)  ? lp.gettoken_int(5)  : 2;
		m_mouseActions[TCP]                                = (lp.getnumtokens() > 6)  ? lp.gettoken_int(6)  : 1;
		m_mouseActions[TCP_EMPTY]                          = (lp.getnumtokens() > 7)  ? lp.gettoken_int(7)  : 2;
		m_mouseActions[TCP_TRACK]                          = (lp.getnumtokens() > 8)  ? lp.gettoken_int(8)  : 2;
		m_mouseActions[TCP_TRACK_MASTER]                   = (lp.getnumtokens() > 9)  ? lp.gettoken_int(9)  : 2;
		m_mouseActions[TCP_ENVELOPE]                       = (lp.getnumtokens() > 10) ? lp.gettoken_int(10) : 2;
		m_mouseActions[TCP_ENVELOPE_VOLUME]                = (lp.getnumtokens() > 11) ? lp.gettoken_int(11) : 2;
		m_mouseActions[TCP_ENVELOPE_PAN]                   = (lp.getnumtokens() > 12) ? lp.gettoken_int(12) : 2;
		m_mouseActions[TCP_ENVELOPE_WIDTH]                 = (lp.getnumtokens() > 13) ? lp.gettoken_int(13) : 2;
		m_mouseActions[TCP_ENVELOPE_MUTE]                  = (lp.getnumtokens() > 14) ? lp.gettoken_int(14) : 2;
		m_mouseActions[TCP_ENVELOPE_PLAYRATE]              = (lp.getnumtokens() > 15) ? lp.gettoken_int(15) : 2;
		m_mouseActions[TCP_ENVELOPE_TEMPO]                 = (lp.getnumtokens() > 16) ? lp.gettoken_int(16) : 2;
		m_mouseActions[MCP]                                = (lp.getnumtokens() > 17) ? lp.gettoken_int(17) : 1;
		m_mouseActions[MCP_EMPTY]                          = (lp.getnumtokens() > 18) ? lp.gettoken_int(18) : 2;
		m_mouseActions[MCP_TRACK]                          = (lp.getnumtokens() > 19) ? lp.gettoken_int(19) : 2;
		m_mouseActions[MCP_TRACK_MASTER]                   = (lp.getnumtokens() > 20) ? lp.gettoken_int(20) : 2;
		m_mouseActions[ARRANGE]                            = (lp.getnumtokens() > 21) ? lp.gettoken_int(21) : 1;
		m_mouseActions[ARRANGE_EMPTY]                      = (lp.getnumtokens() > 22) ? lp.gettoken_int(22) : 2;
		m_mouseActions[ARRANGE_TRACK]                      = (lp.getnumtokens() > 23) ? lp.gettoken_int(23) : 2;
		m_mouseActions[ARRANGE_TRACK_EMPTY]                = (lp.getnumtokens() > 24) ? lp.gettoken_int(24) : 2;
		m_mouseActions[ARRANGE_TRACK_ITEM]                 = (lp.getnumtokens() > 25) ? lp.gettoken_int(25) : 2;
		m_mouseActions[ARRANGE_TRACK_ITEM_AUDIO]           = (lp.getnumtokens() > 26) ? lp.gettoken_int(26) : 2;
		m_mouseActions[ARRANGE_TRACK_ITEM_MIDI]            = (lp.getnumtokens() > 27) ? lp.gettoken_int(27) : 2;
		m_mouseActions[ARRANGE_TRACK_ITEM_VIDEO]           = (lp.getnumtokens() > 28) ? lp.gettoken_int(28) : 2;
		m_mouseActions[ARRANGE_TRACK_ITEM_EMPTY]           = (lp.getnumtokens() > 29) ? lp.gettoken_int(29) : 2;
		m_mouseActions[ARRANGE_TRACK_ITEM_CLICK]           = (lp.getnumtokens() > 30) ? lp.gettoken_int(30) : 2;
		m_mouseActions[ARRANGE_TRACK_ITEM_TIMECODE]        = (lp.getnumtokens() > 31) ? lp.gettoken_int(31) : 2;
		m_mouseActions[ARRANGE_TRACK_ITEM_STRETCH_MARKER]  = (lp.getnumtokens() > 32) ? lp.gettoken_int(32) : 2;
		m_mouseActions[ARRANGE_TRACK_TAKE_ENVELOPE]        = (lp.getnumtokens() > 33) ? lp.gettoken_int(33) : 2;
		m_mouseActions[ARRANGE_TRACK_TAKE_ENVELOPE_VOLUME] = (lp.getnumtokens() > 34) ? lp.gettoken_int(34) : 2;
		m_mouseActions[ARRANGE_TRACK_TAKE_ENVELOPE_PAN]    = (lp.getnumtokens() > 35) ? lp.gettoken_int(35) : 2;
		m_mouseActions[ARRANGE_TRACK_TAKE_ENVELOPE_MUTE]   = (lp.getnumtokens() > 36) ? lp.gettoken_int(36) : 2;
		m_mouseActions[ARRANGE_TRACK_TAKE_ENVELOPE_PITCH]  = (lp.getnumtokens() > 37) ? lp.gettoken_int(37) : 2;
		m_mouseActions[ARRANGE_ENVELOPE_TRACK]             = (lp.getnumtokens() > 38) ? lp.gettoken_int(38) : 2;
		m_mouseActions[ARRANGE_ENVELOPE_TRACK_VOLUME]      = (lp.getnumtokens() > 39) ? lp.gettoken_int(39) : 2;
		m_mouseActions[ARRANGE_ENVELOPE_TRACK_PAN]         = (lp.getnumtokens() > 40) ? lp.gettoken_int(40) : 2;
		m_mouseActions[ARRANGE_ENVELOPE_TRACK_WIDTH]       = (lp.getnumtokens() > 41) ? lp.gettoken_int(41) : 2;
		m_mouseActions[ARRANGE_ENVELOPE_TRACK_MUTE]        = (lp.getnumtokens() > 42) ? lp.gettoken_int(42) : 2;
		m_mouseActions[ARRANGE_ENVELOPE_TRACK_PITCH]       = (lp.getnumtokens() > 43) ? lp.gettoken_int(43) : 2;
		m_mouseActions[ARRANGE_ENVELOPE_TRACK_PLAYRATE]    = (lp.getnumtokens() > 44) ? lp.gettoken_int(44) : 2;
		m_mouseActions[ARRANGE_ENVELOPE_TRACK_TEMPO]       = (lp.getnumtokens() > 45) ? lp.gettoken_int(45) : 2;
		m_mouseActions[MIDI_EDITOR]                        = (lp.getnumtokens() > 46) ? lp.gettoken_int(46) : 1;
		m_mouseActions[MIDI_EDITOR_RULER]                  = (lp.getnumtokens() > 47) ? lp.gettoken_int(47) : 2;
		m_mouseActions[MIDI_EDITOR_PIANO]                  = (lp.getnumtokens() > 48) ? lp.gettoken_int(48) : 2;
		m_mouseActions[MIDI_EDITOR_PIANO_NAMED]            = (lp.getnumtokens() > 49) ? lp.gettoken_int(49) : 2;
		m_mouseActions[MIDI_EDITOR_NOTES]                  = (lp.getnumtokens() > 50) ? lp.gettoken_int(50) : 2;
		m_mouseActions[MIDI_EDITOR_CC_LANE]                = (lp.getnumtokens() > 51) ? lp.gettoken_int(51) : 2;
		m_mouseActions[MIDI_EDITOR_CC_SELECTOR]            = (lp.getnumtokens() > 52) ? lp.gettoken_int(52) : 2;
		m_mouseActions[INLINE_MIDI_EDITOR]                 = (lp.getnumtokens() > 53) ? lp.gettoken_int(53) : 1;
		m_mouseActions[INLINE_MIDI_EDITOR_PIANO]           = (lp.getnumtokens() > 54) ? lp.gettoken_int(54) : 2;
		m_mouseActions[INLINE_MIDI_EDITOR_PIANO_NAMED]     = (lp.getnumtokens() > 55) ? lp.gettoken_int(55) : 2;
		m_mouseActions[INLINE_MIDI_EDITOR_NOTES]           = (lp.getnumtokens() > 56) ? lp.gettoken_int(56) : 2;
		m_mouseActions[INLINE_MIDI_EDITOR_CC_LANE]         = (lp.getnumtokens() > 57) ? lp.gettoken_int(57) : 2;

		for (int i = CONTEXT_START; i < CONTEXT_COUNT; ++i)
		{
			if      (m_mouseActions[i] == 1)  {m_mouseActions[i] = DO_NOTHING;           m_toggleActions[i] = DO_NOTHING;}
			else if (m_mouseActions[i] == 2)  {m_mouseActions[i] = INHERIT_PARENT;       m_toggleActions[i] = INHERIT_PARENT;}
			else if (m_mouseActions[i] == 3)  {m_mouseActions[i] = FOLLOW_ITEM_CONTEXT;  m_toggleActions[i] = FOLLOW_ITEM_CONTEXT;}
			else if (m_mouseActions[i] == 4)  {m_mouseActions[i] = TOOLBAR_1_MOUSE;      m_toggleActions[i] = TOOLBAR_1_TOGGLE;}
			else if (m_mouseActions[i] == 5)  {m_mouseActions[i] = TOOLBAR_2_MOUSE;      m_toggleActions[i] = TOOLBAR_2_TOGGLE;}
			else if (m_mouseActions[i] == 6)  {m_mouseActions[i] = TOOLBAR_3_MOUSE;      m_toggleActions[i] = TOOLBAR_3_TOGGLE;}
			else if (m_mouseActions[i] == 7)  {m_mouseActions[i] = TOOLBAR_4_MOUSE;      m_toggleActions[i] = TOOLBAR_4_TOGGLE;}
			else if (m_mouseActions[i] == 8)  {m_mouseActions[i] = TOOLBAR_5_MOUSE;      m_toggleActions[i] = TOOLBAR_5_TOGGLE;}
			else if (m_mouseActions[i] == 9)  {m_mouseActions[i] = TOOLBAR_6_MOUSE;      m_toggleActions[i] = TOOLBAR_6_TOGGLE;}
			else if (m_mouseActions[i] == 10) {m_mouseActions[i] = TOOLBAR_7_MOUSE;      m_toggleActions[i] = TOOLBAR_7_TOGGLE;}
			else if (m_mouseActions[i] == 11) {m_mouseActions[i] = TOOLBAR_8_MOUSE;      m_toggleActions[i] = TOOLBAR_8_TOGGLE;}
			else if (m_mouseActions[i] == 12) {m_mouseActions[i] = MIDI_TOOLBAR_1_MOUSE; m_toggleActions[i] = MIDI_TOOLBAR_1_TOGGLE;}
			else if (m_mouseActions[i] == 13) {m_mouseActions[i] = MIDI_TOOLBAR_2_MOUSE; m_toggleActions[i] = MIDI_TOOLBAR_2_TOGGLE;}
			else if (m_mouseActions[i] == 14) {m_mouseActions[i] = MIDI_TOOLBAR_3_MOUSE; m_toggleActions[i] = MIDI_TOOLBAR_3_TOGGLE;}
			else if (m_mouseActions[i] == 15) {m_mouseActions[i] = MIDI_TOOLBAR_4_MOUSE; m_toggleActions[i] = MIDI_TOOLBAR_4_TOGGLE;}
			else                              {m_mouseActions[i] = DO_NOTHING;           m_toggleActions[i] = DO_NOTHING;}

			if (m_mouseActions[i] == INHERIT_PARENT && !g_toolbarsManager.CanContextInheritParent(i))
			{
				m_mouseActions[i] = DO_NOTHING;
				m_toggleActions[i] = DO_NOTHING;
			}
			if (m_mouseActions[i] == FOLLOW_ITEM_CONTEXT && !g_toolbarsManager.CanContextFollowItem(i))
			{
				m_mouseActions[i] = INHERIT_PARENT;
				m_toggleActions[i] = INHERIT_PARENT;
			}
		}
	}

	if (options)
	{
		LineParser lp(false);
		lp.parse(options);
		BR_ContextualToolbar::Options cleanOptions;

		m_options.focus                = (lp.getnumtokens() > 0)  ? lp.gettoken_int(0)  : cleanOptions.focus;
		m_options.topmost              = (lp.getnumtokens() > 1)  ? lp.gettoken_int(1)  : cleanOptions.topmost;
		m_options.tcpTrack             = (lp.getnumtokens() > 2)  ? lp.gettoken_int(2)  : cleanOptions.tcpTrack;
		m_options.tcpEnvelope          = (lp.getnumtokens() > 3)  ? lp.gettoken_int(3)  : cleanOptions.tcpEnvelope;
		m_options.mcpTrack             = (lp.getnumtokens() > 4)  ? lp.gettoken_int(4)  : cleanOptions.mcpTrack;
		m_options.arrangeTrack         = (lp.getnumtokens() > 5)  ? lp.gettoken_int(5)  : cleanOptions.arrangeTrack;
		m_options.arrangeItem          = (lp.getnumtokens() > 6)  ? lp.gettoken_int(6)  : cleanOptions.arrangeItem;
		m_options.arrangeTrackEnvelope = (lp.getnumtokens() > 7)  ? lp.gettoken_int(7)  : cleanOptions.arrangeTrackEnvelope;
		m_options.arrangeTakeEnvelope  = (lp.getnumtokens() > 8)  ? lp.gettoken_int(8)  : cleanOptions.arrangeTakeEnvelope ;
		m_options.arrangeActTake       = (lp.getnumtokens() > 9)  ? lp.gettoken_int(9)  : cleanOptions.arrangeActTake;
		m_options.midiSetCCLane        = (lp.getnumtokens() > 10) ? lp.gettoken_int(10) : cleanOptions.midiSetCCLane;
		m_options.inlineItem           = (lp.getnumtokens() > 11) ? lp.gettoken_int(11) : cleanOptions.inlineItem;
		m_options.inlineSetCCLane      = (lp.getnumtokens() > 12) ? lp.gettoken_int(12) : cleanOptions.inlineSetCCLane;
	}

	this->UpdateInternals();
}

BR_ContextualToolbar::Options::Options () :
focus                (OPTION_ENABLED|FOCUS_ALL),
topmost              (0),
tcpTrack             (0),
tcpEnvelope          (0),
mcpTrack             (0),
arrangeTrack         (0),
arrangeItem          (0),
arrangeTrackEnvelope (0),
arrangeTakeEnvelope  (0),
arrangeActTake       (false),
midiSetCCLane        (false),
inlineItem           (0),
inlineSetCCLane      (false)
{
}

BR_ContextualToolbar::ExecuteOnToolbarLoad::ExecuteOnToolbarLoad () :
focusHwnd           (NULL),
trackToSelect       (NULL),
envelopeToSelect    (NULL),
itemToSelect        (NULL),
takeParent          (NULL),
takeIdToActivate    (-1),
focusContext        (-1),
setFocus            (false),
clearTrackSelection (false),
clearItemSelection  (false),
setCCLaneAsClicked  (false)
{
}

int BR_ContextualToolbar::FindRulerToolbar (BR_MouseContextInfo& mouseInfo, BR_ContextualToolbar::ExecuteOnToolbarLoad& executeOnToolbarLoad)
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

	if (context != -1 && (m_mouseActions[context] == INHERIT_PARENT))
		context = RULER;
	return context;
}

int BR_ContextualToolbar::FindTransportToolbar (BR_MouseContextInfo& mouseInfo, BR_ContextualToolbar::ExecuteOnToolbarLoad& executeOnToolbarLoad)
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

int BR_ContextualToolbar::FindTcpToolbar (BR_MouseContextInfo& mouseInfo,  BR_ContextualToolbar::ExecuteOnToolbarLoad& executeOnToolbarLoad)
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
		if (mouseInfo.GetTrack() == GetMasterTrack(NULL)) context = (m_mouseActions[TCP_TRACK_MASTER] == INHERIT_PARENT) ? TCP_TRACK : TCP_TRACK_MASTER;
		else                                              context = TCP_TRACK;

		// Check track options
		if (m_mouseActions[context] != DO_NOTHING && (m_options.tcpTrack & OPTION_ENABLED))
		{
			if ((m_options.tcpTrack & SELECT_TRACK)) executeOnToolbarLoad.trackToSelect = mouseInfo.GetTrack();
			executeOnToolbarLoad.clearTrackSelection = !!(m_options.tcpTrack & CLEAR_TRACK_SELECTION);
		}
	}
	// Envelope control panel
	else if (!strcmp(mouseInfo.GetSegment(), "envelope"))
	{
		context = TCP_ENVELOPE;

		int type = GetEnvType(mouseInfo.GetEnvelope(), false);
		if      (type == VOLUME || type == VOLUME_PREFX) context = (m_mouseActions[TCP_ENVELOPE_VOLUME]   == INHERIT_PARENT) ? TCP_ENVELOPE : TCP_ENVELOPE_VOLUME;
		else if (type == PAN    || type == PAN_PREFX)    context = (m_mouseActions[TCP_ENVELOPE_PAN]      == INHERIT_PARENT) ? TCP_ENVELOPE : TCP_ENVELOPE_PAN;
		else if (type == WIDTH  || type == WIDTH_PREFX)  context = (m_mouseActions[TCP_ENVELOPE_WIDTH]    == INHERIT_PARENT) ? TCP_ENVELOPE : TCP_ENVELOPE_WIDTH;
		else if (type == MUTE)                           context = (m_mouseActions[TCP_ENVELOPE_MUTE]     == INHERIT_PARENT) ? TCP_ENVELOPE : TCP_ENVELOPE_MUTE;
		else if (type == PLAYRATE)                       context = (m_mouseActions[TCP_ENVELOPE_PLAYRATE] == INHERIT_PARENT) ? TCP_ENVELOPE : TCP_ENVELOPE_PLAYRATE;
		else if (type == TEMPO)                          context = (m_mouseActions[TCP_ENVELOPE_TEMPO]    == INHERIT_PARENT) ? TCP_ENVELOPE : TCP_ENVELOPE_TEMPO;

		// Check envelope options
		if (m_mouseActions[context] != DO_NOTHING && (m_options.tcpEnvelope & OPTION_ENABLED))
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

	if (context != -1 && (m_mouseActions[context] == INHERIT_PARENT))
		context = TCP;
	return context;
}

int BR_ContextualToolbar::FindMcpToolbar (BR_MouseContextInfo& mouseInfo, BR_ContextualToolbar::ExecuteOnToolbarLoad& executeOnToolbarLoad)
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
		if (mouseInfo.GetTrack() == GetMasterTrack(NULL)) context = (m_mouseActions[MCP_TRACK_MASTER] == INHERIT_PARENT) ? MCP_TRACK : MCP_TRACK_MASTER;
		else                                              context = MCP_TRACK;

		// Check track options
		if (m_mouseActions[context] != DO_NOTHING && (m_options.mcpTrack & OPTION_ENABLED))
		{
			if ((m_options.mcpTrack & SELECT_TRACK)) executeOnToolbarLoad.trackToSelect = mouseInfo.GetTrack();
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

	if (context != -1 && m_mouseActions[context] == INHERIT_PARENT)
		context = MCP;
	return context;
}

int BR_ContextualToolbar::FindArrangeToolbar (BR_MouseContextInfo& mouseInfo, BR_ContextualToolbar::ExecuteOnToolbarLoad& executeOnToolbarLoad)
{
	bool focusEnvelope = false;
	int context = -1;

	// Empty arrange space
	if (!strcmp(mouseInfo.GetSegment(), "empty"))
	{
		context = ARRANGE_EMPTY;
	}
	// Empty track lane
	else if (!strcmp(mouseInfo.GetSegment(), "track") && !strcmp(mouseInfo.GetDetails(), "empty"))
	{
		context = (m_mouseActions[ARRANGE_TRACK_EMPTY] == INHERIT_PARENT) ? ARRANGE_TRACK : ARRANGE_TRACK_EMPTY;

		if (m_mouseActions[context] != DO_NOTHING && (m_options.arrangeTrack & OPTION_ENABLED))
		{
			if ((m_options.arrangeTrack & SELECT_TRACK)) executeOnToolbarLoad.trackToSelect = mouseInfo.GetTrack();
			executeOnToolbarLoad.clearTrackSelection = !!(m_options.arrangeTrack & CLEAR_TRACK_SELECTION);
		}
	}
	// Envelope
	else if (!strcmp(mouseInfo.GetSegment(), "envelope") || (!strcmp(mouseInfo.GetSegment(), "track") && (!strcmp(mouseInfo.GetDetails(), "env_point")) || (!strcmp(mouseInfo.GetDetails(), "env_segment"))))
	{
		if (mouseInfo.IsTakeEnvelope())
		{
			context = ARRANGE_TRACK_TAKE_ENVELOPE;
			int type = GetEnvType(mouseInfo.GetEnvelope(), false);
			if      (type == VOLUME || type == VOLUME_PREFX) context = (m_mouseActions[ARRANGE_TRACK_TAKE_ENVELOPE_VOLUME] == INHERIT_PARENT) ? ARRANGE_TRACK_TAKE_ENVELOPE : ARRANGE_TRACK_TAKE_ENVELOPE_VOLUME;
			else if (type == PAN    || type == PAN_PREFX)    context = (m_mouseActions[ARRANGE_TRACK_TAKE_ENVELOPE_PAN]    == INHERIT_PARENT) ? ARRANGE_TRACK_TAKE_ENVELOPE : ARRANGE_TRACK_TAKE_ENVELOPE_PAN;
			else if (type == MUTE)                           context = (m_mouseActions[ARRANGE_TRACK_TAKE_ENVELOPE_MUTE]   == INHERIT_PARENT) ? ARRANGE_TRACK_TAKE_ENVELOPE : ARRANGE_TRACK_TAKE_ENVELOPE_MUTE;
			else if (type == PITCH)                          context = (m_mouseActions[ARRANGE_TRACK_TAKE_ENVELOPE_PITCH]  == INHERIT_PARENT) ? ARRANGE_TRACK_TAKE_ENVELOPE : ARRANGE_TRACK_TAKE_ENVELOPE_PITCH;

			// Check envelope options
			if (m_mouseActions[context] != DO_NOTHING)
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
			context = ARRANGE_ENVELOPE_TRACK;
			int type = GetEnvType(mouseInfo.GetEnvelope(), false);
			if      (type == VOLUME || type == VOLUME_PREFX) context = (m_mouseActions[ARRANGE_ENVELOPE_TRACK_VOLUME]   == INHERIT_PARENT) ? ARRANGE_ENVELOPE_TRACK : ARRANGE_ENVELOPE_TRACK_VOLUME;
			else if (type == PAN    || type == PAN_PREFX)    context = (m_mouseActions[ARRANGE_ENVELOPE_TRACK_PAN]      == INHERIT_PARENT) ? ARRANGE_ENVELOPE_TRACK : ARRANGE_ENVELOPE_TRACK_PAN;
			else if (type == WIDTH  || type == WIDTH_PREFX)  context = (m_mouseActions[ARRANGE_ENVELOPE_TRACK_WIDTH]    == INHERIT_PARENT) ? ARRANGE_ENVELOPE_TRACK : ARRANGE_ENVELOPE_TRACK_WIDTH;
			else if (type == MUTE)                           context = (m_mouseActions[ARRANGE_ENVELOPE_TRACK_MUTE]     == INHERIT_PARENT) ? ARRANGE_ENVELOPE_TRACK : ARRANGE_ENVELOPE_TRACK_MUTE;
			else if (type == PITCH)                          context = (m_mouseActions[ARRANGE_ENVELOPE_TRACK_PITCH]    == INHERIT_PARENT) ? ARRANGE_ENVELOPE_TRACK : ARRANGE_ENVELOPE_TRACK_PITCH;
			else if (type == PLAYRATE)                       context = (m_mouseActions[ARRANGE_ENVELOPE_TRACK_PLAYRATE] == INHERIT_PARENT) ? ARRANGE_ENVELOPE_TRACK : ARRANGE_ENVELOPE_TRACK_PLAYRATE;
			else if (type == TEMPO)                          context = (m_mouseActions[ARRANGE_ENVELOPE_TRACK_TEMPO]    == INHERIT_PARENT) ? ARRANGE_ENVELOPE_TRACK : ARRANGE_ENVELOPE_TRACK_TEMPO;

			// Check envelope options
			if (m_mouseActions[context] != DO_NOTHING)
			{
				if ((m_options.arrangeTrackEnvelope & OPTION_ENABLED))
				{
					if ((m_options.arrangeTrackEnvelope & SELECT_ENVELOPE)) executeOnToolbarLoad.envelopeToSelect = mouseInfo.GetEnvelope();
					if ((m_options.arrangeTrackEnvelope & SELECT_TRACK))    executeOnToolbarLoad.trackToSelect    = mouseInfo.GetTrack() ? mouseInfo.GetTrack() : GetEnvParent(mouseInfo.GetEnvelope());
					executeOnToolbarLoad.clearTrackSelection = !!(m_options.arrangeTrackEnvelope & CLEAR_TRACK_SELECTION);
				}
			}
		}

		focusEnvelope = true;
	}

	// Item (purposely not an "else if" because some contexts can follow item instead of theirs usual context)
	if ((context != -1 && m_mouseActions[context] == FOLLOW_ITEM_CONTEXT) || (!strcmp(mouseInfo.GetSegment(), "track") && (!strcmp(mouseInfo.GetDetails(), "item") || !strcmp(mouseInfo.GetDetails(), "stretch_marker"))))
	{
		if (!strcmp(mouseInfo.GetDetails(), "stretch_marker") && m_mouseActions[ARRANGE_TRACK_ITEM_STRETCH_MARKER] != FOLLOW_ITEM_CONTEXT)
		{
			context = ARRANGE_TRACK_ITEM_STRETCH_MARKER;
		}
		else
		{
			context = ARRANGE_TRACK_ITEM;

			// When determining take type be mindful of the option to activate take under mouse
			MediaItem_Take* take = ((m_options.arrangeActTake & OPTION_ENABLED)) ? mouseInfo.GetTake() : GetActiveTake(mouseInfo.GetItem());
			if (mouseInfo.GetItem() && !take)
			{
				context = (m_mouseActions[ARRANGE_TRACK_ITEM_EMPTY] == INHERIT_PARENT) ? ARRANGE_TRACK_ITEM : ARRANGE_TRACK_ITEM_EMPTY;
			}
			else
			{
				int type = GetTakeType(take);
				if      (type == 0) context = (m_mouseActions[ARRANGE_TRACK_ITEM_AUDIO]    == INHERIT_PARENT) ? ARRANGE_TRACK_ITEM : ARRANGE_TRACK_ITEM_AUDIO;
				else if (type == 1) context = (m_mouseActions[ARRANGE_TRACK_ITEM_MIDI]     == INHERIT_PARENT) ? ARRANGE_TRACK_ITEM : ARRANGE_TRACK_ITEM_MIDI;
				else if (type == 2) context = (m_mouseActions[ARRANGE_TRACK_ITEM_VIDEO]    == INHERIT_PARENT) ? ARRANGE_TRACK_ITEM : ARRANGE_TRACK_ITEM_VIDEO;
				else if (type == 3) context = (m_mouseActions[ARRANGE_TRACK_ITEM_CLICK]    == INHERIT_PARENT) ? ARRANGE_TRACK_ITEM : ARRANGE_TRACK_ITEM_CLICK;
				else if (type == 4) context = (m_mouseActions[ARRANGE_TRACK_ITEM_TIMECODE] == INHERIT_PARENT) ? ARRANGE_TRACK_ITEM : ARRANGE_TRACK_ITEM_TIMECODE;
			}
		}

		// Check item options
		if (m_mouseActions[context] != DO_NOTHING)
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

		if (m_mouseActions[context] == INHERIT_PARENT)
			context = ARRANGE_TRACK;
	}

	// Get focus information
	if ((m_options.focus & OPTION_ENABLED) && ((m_options.focus & FOCUS_ALL) || (m_options.focus & FOCUS_MAIN)))
	{
		executeOnToolbarLoad.setFocus     = true;
		executeOnToolbarLoad.focusHwnd    = g_hwndParent;
		executeOnToolbarLoad.focusContext = (focusEnvelope && GetSelectedEnvelope(NULL)) ? 2 : 1;
	}

	if (context != -1 && m_mouseActions[context] == INHERIT_PARENT)
		context = ARRANGE;
	return context;
}

int BR_ContextualToolbar::FindMidiToolbar (BR_MouseContextInfo& mouseInfo, BR_ContextualToolbar::ExecuteOnToolbarLoad& executeOnToolbarLoad)
{
	int context = -1;

	if      (!strcmp(mouseInfo.GetSegment(), "ruler"))   context = MIDI_EDITOR_RULER;
	else if (!strcmp(mouseInfo.GetSegment(), "piano"))   context = (mouseInfo.GetPianoRollMode() == 1) ? ((m_mouseActions[MIDI_EDITOR_PIANO_NAMED] == INHERIT_PARENT) ? MIDI_EDITOR_PIANO : MIDI_EDITOR_PIANO_NAMED) : MIDI_EDITOR_PIANO;
	else if (!strcmp(mouseInfo.GetSegment(), "notes"))   context = MIDI_EDITOR_NOTES;
	else if (!strcmp(mouseInfo.GetSegment(), "cc_lane"))
	{
		if      (!strcmp(mouseInfo.GetDetails(), "cc_lane"))     context = MIDI_EDITOR_CC_LANE;
		else if (!strcmp(mouseInfo.GetDetails(), "cc_selector")) context = MIDI_EDITOR_CC_SELECTOR;

		// Check MIDI editor options
		if (context != -1 && m_mouseActions[context] != DO_NOTHING && (m_options.midiSetCCLane & OPTION_ENABLED))
			executeOnToolbarLoad.setCCLaneAsClicked = true;
	}

	// Get focus information
	if ((m_options.focus & OPTION_ENABLED) && ((m_options.focus & FOCUS_ALL) || (m_options.focus & FOCUS_MIDI)))
	{
		executeOnToolbarLoad.setFocus     = true;
		executeOnToolbarLoad.focusHwnd    = (HWND)mouseInfo.GetMidiEditor();
		executeOnToolbarLoad.focusContext = GetCursorContext2(true);
	}

	if (context != -1 && m_mouseActions[context] == INHERIT_PARENT)
		context = MIDI_EDITOR;
	return context;
}

int BR_ContextualToolbar::FindInlineMidiToolbar (BR_MouseContextInfo& mouseInfo, BR_ContextualToolbar::ExecuteOnToolbarLoad& executeOnToolbarLoad)
{
	int context = -1;

	if      (!strcmp(mouseInfo.GetSegment(), "piano"))   context = INLINE_MIDI_EDITOR_PIANO;
	else if (!strcmp(mouseInfo.GetSegment(), "notes"))   context = (mouseInfo.GetPianoRollMode() == 1) ? ((m_mouseActions[INLINE_MIDI_EDITOR_PIANO_NAMED] == INHERIT_PARENT) ? INLINE_MIDI_EDITOR_PIANO : INLINE_MIDI_EDITOR_PIANO_NAMED) : INLINE_MIDI_EDITOR_PIANO;
	else if (!strcmp(mouseInfo.GetSegment(), "cc_lane")) context = INLINE_MIDI_EDITOR_CC_LANE;

	// Check inline MIDI editor options
	if (context != -1 && m_mouseActions[context] != DO_NOTHING)
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

	if (context != -1 && m_mouseActions[context] == INHERIT_PARENT)
		context = INLINE_MIDI_EDITOR;
	return context;
}

bool BR_ContextualToolbar::AreAssignedToolbarsOpened ()
{
	for (set<int>::iterator it = m_activeContexts.begin(); it != m_activeContexts.end(); ++it)
	{
		int toggleAction = m_toggleActions[*it];
		if (g_toolbarsManager.IsContextValid(*it) && g_toolbarsManager.IsToolbarAction(toggleAction))
		{
			if (GetToggleCommandState(toggleAction))
				return true;
		}
	}
	return false;
}

void BR_ContextualToolbar::CloseAllAssignedToolbars ()
{
	for (set<int>::iterator it = m_activeContexts.begin(); it != m_activeContexts.end(); ++it)
	{
		int toggleAction = m_toggleActions[*it];
		if (g_toolbarsManager.IsContextValid(*it) && g_toolbarsManager.IsToolbarAction(toggleAction))
		{
			if (GetToggleCommandState(m_toggleActions[*it]))
				Main_OnCommand(m_toggleActions[*it], 0);
		}
	}
}

void BR_ContextualToolbar::UpdateInternals ()
{
	m_mode = 0;
	m_activeContexts.clear();

	for (int i = CONTEXT_START; i < CONTEXT_COUNT; ++i)
	{
		int action = m_mouseActions[i];
		if      (i == TRANSPORT                          && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_TRANSPORT;          m_activeContexts.insert(i);}
		else if (i == RULER                              && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_RULER;              m_activeContexts.insert(i);}
		else if (i == RULER_REGIONS                      && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_RULER;              m_activeContexts.insert(i);}
		else if (i == RULER_MARKERS                      && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_RULER;              m_activeContexts.insert(i);}
		else if (i == RULER_TEMPO                        && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_RULER;              m_activeContexts.insert(i);}
		else if (i == RULER_TIMELINE                     && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_RULER;              m_activeContexts.insert(i);}
		else if (i == TCP                                && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_MCP_TCP;            m_activeContexts.insert(i);}
		else if (i == TCP_EMPTY                          && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_MCP_TCP;            m_activeContexts.insert(i);}
		else if (i == TCP_TRACK                          && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_MCP_TCP;            m_activeContexts.insert(i);}
		else if (i == TCP_TRACK_MASTER                   && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_MCP_TCP;            m_activeContexts.insert(i);}
		else if (i == TCP_ENVELOPE                       && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_MCP_TCP;            m_activeContexts.insert(i);}
		else if (i == TCP_ENVELOPE_VOLUME                && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_MCP_TCP;            m_activeContexts.insert(i);}
		else if (i == TCP_ENVELOPE_PAN                   && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_MCP_TCP;            m_activeContexts.insert(i);}
		else if (i == TCP_ENVELOPE_WIDTH                 && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_MCP_TCP;            m_activeContexts.insert(i);}
		else if (i == TCP_ENVELOPE_MUTE                  && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_MCP_TCP;            m_activeContexts.insert(i);}
		else if (i == TCP_ENVELOPE_PLAYRATE              && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_MCP_TCP;            m_activeContexts.insert(i);}
		else if (i == TCP_ENVELOPE_TEMPO                 && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_MCP_TCP;            m_activeContexts.insert(i);}
		else if (i == MCP                                && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_MCP_TCP;            m_activeContexts.insert(i);}
		else if (i == MCP_EMPTY                          && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_MCP_TCP;            m_activeContexts.insert(i);}
		else if (i == MCP_TRACK                          && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_MCP_TCP;            m_activeContexts.insert(i);}
		else if (i == MCP_TRACK_MASTER                   && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_MCP_TCP;            m_activeContexts.insert(i);}
		else if (i == ARRANGE                            && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_ARRANGE;            m_activeContexts.insert(i);}
		else if (i == ARRANGE_EMPTY                      && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_ARRANGE;            m_activeContexts.insert(i);}
		else if (i == ARRANGE_TRACK                      && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_ARRANGE;            m_activeContexts.insert(i);}
		else if (i == ARRANGE_TRACK_EMPTY                && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_ARRANGE;            m_activeContexts.insert(i);}
		else if (i == ARRANGE_TRACK_ITEM                 && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_ARRANGE;            m_activeContexts.insert(i);}
		else if (i == ARRANGE_TRACK_ITEM_AUDIO           && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_ARRANGE;            m_activeContexts.insert(i);}
		else if (i == ARRANGE_TRACK_ITEM_MIDI            && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_ARRANGE;            m_activeContexts.insert(i);}
		else if (i == ARRANGE_TRACK_ITEM_VIDEO           && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_ARRANGE;            m_activeContexts.insert(i);}
		else if (i == ARRANGE_TRACK_ITEM_EMPTY           && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_ARRANGE;            m_activeContexts.insert(i);}
		else if (i == ARRANGE_TRACK_ITEM_CLICK           && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_ARRANGE;            m_activeContexts.insert(i);}
		else if (i == ARRANGE_TRACK_ITEM_TIMECODE        && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_ARRANGE;            m_activeContexts.insert(i);}
		else if (i == ARRANGE_TRACK_ITEM_STRETCH_MARKER  && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_ARRANGE;            m_activeContexts.insert(i);}
		else if (i == ARRANGE_TRACK_TAKE_ENVELOPE        && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_ARRANGE;            m_activeContexts.insert(i);}
		else if (i == ARRANGE_TRACK_TAKE_ENVELOPE_VOLUME && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_ARRANGE;            m_activeContexts.insert(i);}
		else if (i == ARRANGE_TRACK_TAKE_ENVELOPE_PAN    && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_ARRANGE;            m_activeContexts.insert(i);}
		else if (i == ARRANGE_TRACK_TAKE_ENVELOPE_MUTE   && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_ARRANGE;            m_activeContexts.insert(i);}
		else if (i == ARRANGE_TRACK_TAKE_ENVELOPE_PITCH  && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_ARRANGE;            m_activeContexts.insert(i);}
		else if (i == ARRANGE_ENVELOPE_TRACK             && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_ARRANGE;            m_activeContexts.insert(i);}
		else if (i == ARRANGE_ENVELOPE_TRACK_VOLUME      && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_ARRANGE;            m_activeContexts.insert(i);}
		else if (i == ARRANGE_ENVELOPE_TRACK_PAN         && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_ARRANGE;            m_activeContexts.insert(i);}
		else if (i == ARRANGE_ENVELOPE_TRACK_WIDTH       && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_ARRANGE;            m_activeContexts.insert(i);}
		else if (i == ARRANGE_ENVELOPE_TRACK_MUTE        && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_ARRANGE;            m_activeContexts.insert(i);}
		else if (i == ARRANGE_ENVELOPE_TRACK_PITCH       && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_ARRANGE;            m_activeContexts.insert(i);}
		else if (i == ARRANGE_ENVELOPE_TRACK_PLAYRATE    && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_ARRANGE;            m_activeContexts.insert(i);}
		else if (i == ARRANGE_ENVELOPE_TRACK_TEMPO       && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_ARRANGE;            m_activeContexts.insert(i);}
		else if (i == MIDI_EDITOR                        && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_MIDI_EDITOR;        m_activeContexts.insert(i);}
		else if (i == MIDI_EDITOR_RULER                  && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_MIDI_EDITOR;        m_activeContexts.insert(i);}
		else if (i == MIDI_EDITOR_PIANO                  && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_MIDI_EDITOR;        m_activeContexts.insert(i);}
		else if (i == MIDI_EDITOR_PIANO_NAMED            && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_MIDI_EDITOR;        m_activeContexts.insert(i);}
		else if (i == MIDI_EDITOR_NOTES                  && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_MIDI_EDITOR;        m_activeContexts.insert(i);}
		else if (i == MIDI_EDITOR_CC_LANE                && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_MIDI_EDITOR;        m_activeContexts.insert(i);}
		else if (i == MIDI_EDITOR_CC_SELECTOR            && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_MIDI_EDITOR;        m_activeContexts.insert(i);}
		else if (i == INLINE_MIDI_EDITOR                 && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_MIDI_INLINE;        m_activeContexts.insert(i);}
		else if (i == INLINE_MIDI_EDITOR_PIANO           && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_MIDI_INLINE;        m_activeContexts.insert(i);}
		else if (i == INLINE_MIDI_EDITOR_PIANO_NAMED     && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_MIDI_INLINE;        m_activeContexts.insert(i);}
		else if (i == INLINE_MIDI_EDITOR_NOTES           && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_MIDI_INLINE;        m_activeContexts.insert(i);}
		else if (i == INLINE_MIDI_EDITOR_CC_LANE         && action != DO_NOTHING && action != INHERIT_PARENT) {m_mode |= BR_MouseContextInfo::MODE_MIDI_INLINE;        m_activeContexts.insert(i);}
	}

	m_mode |= BR_MouseContextInfo::MODE_ENV_LANE_NO_SEGMENT;
}

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
		if (BR_ContextualToolbar* contextualToolbars = m_contextualToolbars.Get(id))
		{
			char contexts[512];
			char options[512];
			GetPrivateProfileString(INI_SECTION, this->GetContextKey(id).Get(), "", contexts, sizeof(contexts), BR_GetIniFile());
			GetPrivateProfileString(INI_SECTION, this->GetOptionsKey(id).Get(), "", options, sizeof(options), BR_GetIniFile());

			contextualToolbars->ImportConfig(contexts, options);
		}
	}

	return m_contextualToolbars.Get(id);
}

void BR_ContextualToolbarsManager::SetContextualToolbar (int id, BR_ContextualToolbar& contextualToolbar)
{
	if (BR_ContextualToolbar* currentContextualToolbar = this->GetContextualToolbar(id))
	{
		*currentContextualToolbar = contextualToolbar;

		char contexts[512];
		char options[512];
		currentContextualToolbar->ExportConfig(contexts, sizeof(contexts), options, sizeof(options));

		WritePrivateProfileString(INI_SECTION, this->GetContextKey(id).Get(), contexts, BR_GetIniFile());
		WritePrivateProfileString(INI_SECTION, this->GetOptionsKey(id).Get(), options, BR_GetIniFile());
	}
}

bool BR_ContextualToolbarsManager::GetReaperToolbar (int id, int* mouseAction, int* toggleAction, char* toolbarName, int toolbarNameSz)
{
	if (id >= 0 && id <= 11)
	{
		if (id == 0)
		{
			WritePtr(mouseAction, TOOLBAR_1_MOUSE);
			WritePtr(toggleAction, TOOLBAR_1_TOGGLE);
			if (toolbarName)
				GetPrivateProfileString("Floating toolbar 1", "title", __localizeFunc("Toolbar 1", "MENU_349", 0), toolbarName, toolbarNameSz, GetReaperMenuIniPath().Get());
		}
		else if (id == 1)
		{
			WritePtr(mouseAction, TOOLBAR_2_MOUSE);
			WritePtr(toggleAction, TOOLBAR_2_TOGGLE);
			if (toolbarName)
				GetPrivateProfileString("Floating toolbar 2", "title", __localizeFunc("Toolbar 2", "MENU_349", 0), toolbarName, toolbarNameSz, GetReaperMenuIniPath().Get());
		}
		else if (id == 2)
		{
			WritePtr(mouseAction, TOOLBAR_3_MOUSE);
			WritePtr(toggleAction, TOOLBAR_3_TOGGLE);
			if (toolbarName)
				GetPrivateProfileString("Floating toolbar 3", "title", __localizeFunc("Toolbar 3", "MENU_349", 0), toolbarName, toolbarNameSz, GetReaperMenuIniPath().Get());
		}
		else if (id == 3)
		{
			WritePtr(mouseAction, TOOLBAR_4_MOUSE);
			WritePtr(toggleAction, TOOLBAR_4_TOGGLE);
			if (toolbarName)
				GetPrivateProfileString("Floating toolbar 4", "title", __localizeFunc("Toolbar 4", "MENU_349", 0), toolbarName, toolbarNameSz, GetReaperMenuIniPath().Get());
		}
		else if (id == 4)
		{
			WritePtr(mouseAction, TOOLBAR_5_MOUSE);
			WritePtr(toggleAction, TOOLBAR_5_TOGGLE);
			if (toolbarName)
				GetPrivateProfileString("Floating toolbar 5", "title", __localizeFunc("Toolbar 5", "MENU_349", 0), toolbarName, toolbarNameSz, GetReaperMenuIniPath().Get());
		}
		else if (id == 5)
		{
			WritePtr(mouseAction, TOOLBAR_6_MOUSE);
			WritePtr(toggleAction, TOOLBAR_6_TOGGLE);
			if (toolbarName)
				GetPrivateProfileString("Floating toolbar 6", "title", __localizeFunc("Toolbar 6", "MENU_349", 0), toolbarName, toolbarNameSz, GetReaperMenuIniPath().Get());
		}
		else if (id == 6)
		{
			WritePtr(mouseAction, TOOLBAR_7_MOUSE);
			WritePtr(toggleAction, TOOLBAR_7_TOGGLE);
			if (toolbarName)
				GetPrivateProfileString("Floating toolbar 7", "title", __localizeFunc("Toolbar 7", "MENU_349", 0), toolbarName, toolbarNameSz, GetReaperMenuIniPath().Get());
		}
		else if (id == 7)
		{
			WritePtr(mouseAction, TOOLBAR_8_MOUSE);
			WritePtr(toggleAction, TOOLBAR_8_TOGGLE);
			if (toolbarName)
				GetPrivateProfileString("Floating toolbar 8", "title", __localizeFunc("Toolbar 8", "MENU_349", 0), toolbarName, toolbarNameSz, GetReaperMenuIniPath().Get());
		}
		else if (id == 8)
		{
			WritePtr(mouseAction, MIDI_TOOLBAR_1_MOUSE);
			WritePtr(toggleAction, MIDI_TOOLBAR_1_TOGGLE);
			if (toolbarName)
				GetPrivateProfileString("Floating MIDI Toolbar 1", "title", __localizeFunc("MIDI 1", "MENU_349", 0), toolbarName, toolbarNameSz, GetReaperMenuIniPath().Get());
		}
		else if (id == 9)
		{
			WritePtr(mouseAction, MIDI_TOOLBAR_2_MOUSE);
			WritePtr(toggleAction, MIDI_TOOLBAR_2_TOGGLE);
			if (toolbarName)
				GetPrivateProfileString("Floating MIDI Toolbar 2", "title", __localizeFunc("MIDI 2", "MENU_349", 0), toolbarName, toolbarNameSz, GetReaperMenuIniPath().Get());
		}
		else if (id == 10)
		{
			WritePtr(mouseAction, MIDI_TOOLBAR_3_MOUSE);
			WritePtr(toggleAction, MIDI_TOOLBAR_3_TOGGLE);
			if (toolbarName)
				GetPrivateProfileString("Floating MIDI Toolbar 3", "title", __localizeFunc("MIDI 3", "MENU_349", 0), toolbarName, toolbarNameSz, GetReaperMenuIniPath().Get());
		}
		else if (id == 11)
		{
			WritePtr(mouseAction, MIDI_TOOLBAR_4_MOUSE);
			WritePtr(toggleAction, MIDI_TOOLBAR_4_TOGGLE);
			if (toolbarName)
				GetPrivateProfileString("Floating MIDI Toolbar 4", "title", __localizeFunc("MIDI 4", "MENU_349", 0), toolbarName, toolbarNameSz, GetReaperMenuIniPath().Get());
		}
		return true;
	}

	return false;
}

bool BR_ContextualToolbarsManager::GetToolbarName (int mouseAction, char* toolbarName, int toolbarNameSz)
{
	int id = -1;
	int currentMouseAction;
	while (this->GetReaperToolbar(++id, &currentMouseAction, NULL, NULL, 0))
	{
		if (mouseAction == currentMouseAction)
		{
			this->GetReaperToolbar(id, NULL, NULL, toolbarName, toolbarNameSz);
			return true;
		}
	}
	return false;
}

int BR_ContextualToolbarsManager::GetToggleToolbarAction (int mouseAction)
{
	if (!this->IsToolbarAction(mouseAction))
		return mouseAction;

	int id = -1;
	int toolbarAction, currentMouseAction;
	while (g_toolbarsManager.GetReaperToolbar(++id, &currentMouseAction, &toolbarAction, NULL, 0))
	{
		if (mouseAction == currentMouseAction)
			return toolbarAction;
	}
	return DO_NOTHING;
}

bool BR_ContextualToolbarsManager::IsContextValid (int context)
{
	if (context >= CONTEXT_START && context < CONTEXT_COUNT)
	{
		if (context != END_TRANSPORT   &&
			context != END_RULER       &&
			context != END_TCP         &&
			context != END_MCP         &&
			context != END_ARRANGE     &&
			context != END_MIDI_EDITOR
		)
		{
			return true;
		}
	}
	return false;
}

bool BR_ContextualToolbarsManager::CanContextInheritParent (int context)
{
	if (this->IsContextValid(context))
	{
		if (context == TRANSPORT          ||
			context == RULER              ||
			context == TCP                ||
			context == MCP                ||
			context == ARRANGE            ||
			context == MIDI_EDITOR        ||
			context == INLINE_MIDI_EDITOR
		)
		{
			return false;
		}
		else
		{
			return true;
		}
	}
	return false;
}

bool BR_ContextualToolbarsManager::CanContextFollowItem (int context)
{
	if (this->IsContextValid(context))
	{
		if (context == ARRANGE_TRACK_ITEM_STRETCH_MARKER  ||
			context == ARRANGE_TRACK_TAKE_ENVELOPE        ||
			context == ARRANGE_TRACK_TAKE_ENVELOPE_VOLUME ||
			context == ARRANGE_TRACK_TAKE_ENVELOPE_PAN    ||
			context == ARRANGE_TRACK_TAKE_ENVELOPE_MUTE   ||
			context == ARRANGE_TRACK_TAKE_ENVELOPE_PITCH
		)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	return false;
}

bool BR_ContextualToolbarsManager::IsToolbarAction (int action)
{
	if (action != DO_NOTHING && action != INHERIT_PARENT && action != FOLLOW_ITEM_CONTEXT)
		return true;
	else
		return false;
}

void BR_ContextualToolbarsManager::SetToolbarAlwaysOnTop (int mouseAction)
{
	char toolbarName[256];
	if (this->GetToolbarName(mouseAction, toolbarName, sizeof(toolbarName)))
	{
		if (HWND hwnd = FindReaperWndByTitle(toolbarName))
		{
			bool toolbarProcessed = false;
			for (int i = 0; i < m_topToolbars.GetSize(); ++i)
			{
				if (hwnd == m_topToolbars.Get(i)->hwnd)
				{
					toolbarProcessed = true;
					break;
				}
			}

			if (!toolbarProcessed)
			{
				#ifdef _WIN32
					SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
				#else
					int level = SWELL_SetWindowLevel(hwnd, 25); // NSStatusWindowLevel
				#endif

				BR_ContextualToolbarsManager::ToolbarWndData* toolbarWndData = m_topToolbars.Add(new BR_ContextualToolbarsManager::ToolbarWndData);
				toolbarWndData->hwnd    = hwnd;
				toolbarWndData->wndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)BR_ContextualToolbarsManager::ToolbarWndCallback);

				#ifndef _WIN32
					toolbarWndData->level = level;
				#endif
			}
		}
	}
}

WDL_FastString BR_ContextualToolbarsManager::GetContextKey (int id)
{
	WDL_FastString key;
	key.AppendFormatted(256, "%s%.2d", INI_KEY_CONTEXTS, id);
	return key;
}

WDL_FastString BR_ContextualToolbarsManager::GetOptionsKey (int id)
{
	WDL_FastString key;
	key.AppendFormatted(256, "%s%.2d", INI_KEY_OPTIONS, id);
	return key;
}

LRESULT CALLBACK BR_ContextualToolbarsManager::ToolbarWndCallback (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	WNDPROC wndProc = NULL;
	int id = -1;
	#ifndef _WIN32
		int level = 0;
	#endif

	for (int i = 0; i < m_topToolbars.GetSize(); ++i)
	{
		if (BR_ContextualToolbarsManager::ToolbarWndData* toolbarWndData = m_topToolbars.Get(i))
		{
			if (toolbarWndData->hwnd == hwnd)
			{
				wndProc = m_topToolbars.Get(i)->wndProc;
				#ifndef _WIN32
					level = toolbarWndData->level;
				#endif
				id = i;
				break;
			}
		}
	}

	if (wndProc)
	{
		if (uMsg == WM_SHOWWINDOW)
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
					SWELL_SetWindowLevel(hwnd, level);
				#endif
			}
		}
		else if (uMsg == WM_DESTROY)
		{
			m_topToolbars.Delete(id, true);
		}

		return wndProc(hwnd, uMsg, wParam, lParam);
	}
	else
		return 0;
}

WDL_PtrList<BR_ContextualToolbarsManager::ToolbarWndData> BR_ContextualToolbarsManager::m_topToolbars;

/******************************************************************************
* Context toolbars list view                                                  *
******************************************************************************/
BR_ContextualToolbarsView::BR_ContextualToolbarsView (HWND hwndList, HWND hwndEdit)
:SWS_ListView(hwndList, hwndEdit, COL_COUNT, g_cols, CONTEXT_TOOLBARS_VIEW_WND, false, "sws_DLG_181")
{
}

void BR_ContextualToolbarsView::GetItemText (SWS_ListItem* item, int iCol, char* str, int iStrMax)
{
	int context = (int)item;
	WritePtr(str, '\0');

	if (iCol == COL_CONTEXT)
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
		else if (context == INLINE_MIDI_EDITOR_PIANO_NAMED)     {description = __LOCALIZE("Named notes mode",        "sws_DLG_181"); indentation = 2;}
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
			_snprintf(str, iStrMax, "%s%s", indentationString.Get(), description);
		}
	}
	else if (iCol == COL_TOOLBAR)
	{
		if (BR_ContextualToolbarsWnd* wnd = g_contextToolbarsWndManager.Get())
		{
			if (BR_ContextualToolbar* contextualToolbar = wnd->GetCurrentContextualToolbar())
				contextualToolbar->GetToolbarName(context, str, iStrMax);
		}
	}
}

void BR_ContextualToolbarsView::GetItemList (SWS_ListItemList* pList)
{
	for (int i = CONTEXT_START; i < CONTEXT_COUNT; i++)
		pList->Add((SWS_ListItem*)(i));
}

bool BR_ContextualToolbarsView::OnItemSelChanging(SWS_ListItem* item, bool bSel)
{
	if (int context = (int)item)
		return g_toolbarsManager.IsContextValid(context) ? FALSE : TRUE;
	return FALSE;
}

bool BR_ContextualToolbarsView::HideGridLines ()
{
	return true;
}

int BR_ContextualToolbarsView::OnItemSort (SWS_ListItem* item1, SWS_ListItem* item2)
{
	if (item1 && item2)
	{
		double i1 = (int)item1;
		double i2 = (int)item2;
		return (i1 > i2) ? 1 : (i1 < i2) ? - 1 : 0; // don't allow sorting
	}
	return 0;
}

/******************************************************************************
* Contextual toolbars window                                                  *
******************************************************************************/
BR_ContextualToolbarsWnd::BR_ContextualToolbarsWnd () :
SWS_DockWnd(IDD_BR_CONTEXTUAL_TOOLBARS, __LOCALIZE("Contextual toolbars","sws_DLG_181"), "", SWSGetCommandID(ContextToolbarsOptions)),
m_list          (NULL),
m_currentPreset (0)
{
	m_id.Set(CONTEXT_TOOLBARS_WND);
	Init();
}

void BR_ContextualToolbarsWnd::Update ()
{
	int currentPreset = (int)SendDlgItemMessage(m_hwnd, IDC_PRESET, CB_GETCURSEL, 0, 0);
	if (BR_ContextualToolbar* currentToolbar = g_toolbarsManager.GetContextualToolbar(currentPreset))
	{
		m_currentToolbar = *currentToolbar;
		m_currentPreset = currentPreset;
		m_list->Update();

		int x = 0;
		while (true)
		{
			int checkBox = -1;
			int combo    = -1;
			int option   = 0;

			if      (x == 0) {checkBox = IDC_ALL_FOCUS;     combo = IDC_ALL_FOCUS_COMBO;     m_currentToolbar.GetOptionsAll(&option, NULL);}
			else if (x == 1) {checkBox = IDC_TCP_TRACK;     combo = IDC_TCP_TRACK_COMBO;     m_currentToolbar.GetOptionsTcp(&option, NULL);}
			else if (x == 2) {checkBox = IDC_TCP_ENV;       combo = IDC_TCP_ENV_COMBO;       m_currentToolbar.GetOptionsTcp(NULL, &option);}
			else if (x == 3) {checkBox = IDC_MCP_TRACK;     combo = IDC_MCP_TRACK_COMBO;     m_currentToolbar.GetOptionsMcp(&option);}
			else if (x == 4) {checkBox = IDC_ARG_TRACK;     combo = IDC_ARG_TRACK_COMBO;     m_currentToolbar.GetOptionsArrange(&option, NULL, NULL, NULL, NULL);}
			else if (x == 5) {checkBox = IDC_ARG_ITEM;      combo = IDC_ARG_ITEM_COMBO;      m_currentToolbar.GetOptionsArrange(NULL, &option, NULL, NULL, NULL);}
			else if (x == 6) {checkBox = IDC_ARG_TRACK_ENV; combo = IDC_ARG_TRACK_ENV_COMBO; m_currentToolbar.GetOptionsArrange(NULL, NULL, &option, NULL, NULL);}
			else if (x == 7) {checkBox = IDC_ARG_TAKE_ENV;  combo = IDC_ARG_TAKE_ENV_COMBO;  m_currentToolbar.GetOptionsArrange(NULL, NULL, NULL, &option, NULL);}
			else if (x == 8) {checkBox = IDC_INLINE_ITEM;   combo = IDC_INLINE_ITEM_COMBO;   m_currentToolbar.GetOptionsInline(&option, NULL);}

			if (checkBox != -1 && combo != -1)
			{
				CheckDlgButton(m_hwnd, checkBox,        (option & OPTION_ENABLED));
				EnableWindow(GetDlgItem(m_hwnd, combo), (option & OPTION_ENABLED));

				int count = (int)SendDlgItemMessage(m_hwnd, combo, CB_GETCOUNT, 0, 0);
				option &= ~OPTION_ENABLED; // drop downs never contain information if option is enabled so remove it before comparing
				for (int i = 0; i < count; ++i)
				{
					if (option == (int)SendDlgItemMessage(m_hwnd, combo, CB_GETITEMDATA, i, 0))
					{
						SendDlgItemMessage(m_hwnd, combo, CB_SETCURSEL, i, 0);
						break;
					}
					else if (i == count - 1)
						SendDlgItemMessage(m_hwnd, combo, CB_SETCURSEL, 0, 0); // in case nothing is found, default to first entry
				}
			}
			else
				break;
			++x;
		}

		int option;
		m_currentToolbar.GetOptionsAll(NULL, &option);                       CheckDlgButton(m_hwnd, IDC_ALL_TOPMOST,            (option & OPTION_ENABLED));
		m_currentToolbar.GetOptionsArrange(NULL, NULL, NULL, NULL, &option); CheckDlgButton(m_hwnd, IDC_ARG_TAKE_ACTIVATE,      (option & OPTION_ENABLED));
		m_currentToolbar.GetOptionsMIDI(&option);                            CheckDlgButton(m_hwnd, IDC_MIDI_CC_LANE_CLICKED,   (option & OPTION_ENABLED)),
		m_currentToolbar.GetOptionsInline(NULL, &option);                    CheckDlgButton(m_hwnd, IDC_INLINE_CC_LANE_CLICKED, (option & OPTION_ENABLED));
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

WDL_FastString BR_ContextualToolbarsWnd::GetPresetName (int preset)
{
	WDL_FastString presetName;
	presetName.AppendFormatted(256, "%s %.2d", __LOCALIZE("Preset", "sws_DLG_181"), preset + 1);
	return presetName;
}

BR_ContextualToolbar* BR_ContextualToolbarsWnd::GetCurrentContextualToolbar ()
{
	return &m_currentToolbar;
}

void BR_ContextualToolbarsWnd::OnInitDlg ()
{
	char currentPresetStr[64];
	GetPrivateProfileString(INI_SECTION, INI_KEY_CURRENT_PRESET, "0", currentPresetStr, sizeof(currentPresetStr), BR_GetIniFile());
	m_currentPreset = atoi(currentPresetStr);
	if (m_currentPreset < 0 || m_currentPreset > PRESET_COUNT)
		m_currentPreset = 0;

	m_parentVwnd.SetRealParent(m_hwnd);
	m_vwnd_painter.SetGSC(WDL_STYLE_GetSysColor);

	m_list = new BR_ContextualToolbarsView(GetDlgItem(m_hwnd, IDC_LIST), GetDlgItem(m_hwnd, IDC_EDIT));
	m_pLists.Add(m_list);

	m_resize.init_item(IDC_LIST, 0.0, 0.0, 1.0, 1.0);
	m_resize.init_item(IDC_PRESET, 0.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_ALL_BOX, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_ALL_FOCUS, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_ALL_FOCUS_COMBO, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_ALL_TOPMOST, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_TCP_BOX, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_TCP_TRACK, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_TCP_TRACK_COMBO, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_TCP_ENV, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_TCP_ENV_COMBO, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_MCP_BOX, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_MCP_TRACK, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_MCP_TRACK_COMBO, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_ARG_BOX, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_ARG_TRACK, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_ARG_TRACK_COMBO, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_ARG_ITEM, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_ARG_ITEM_COMBO, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_ARG_TRACK_ENV, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_ARG_TRACK_ENV_COMBO, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_ARG_TAKE_ENV, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_ARG_TAKE_ENV_COMBO, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_ARG_TAKE_ACTIVATE, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_MIDI_BOX, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_MIDI_CC_LANE_CLICKED, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_INLINE_BOX, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_INLINE_ITEM, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_INLINE_ITEM_COMBO, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_INLINE_CC_LANE_CLICKED, 1.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_HELP_WIKI, 1.0, 1.0, 1.0, 1.0);
	m_resize.init_item(IDC_SAVE, 1.0, 1.0, 1.0, 1.0);

	// Preset dropdown
	WDL_UTF8_HookComboBox(GetDlgItem(m_hwnd, IDC_PRESET));
	for (int i = 0; i < PRESET_COUNT; ++i)
		SendDlgItemMessage(m_hwnd, IDC_PRESET, CB_ADDSTRING, 0, (LPARAM)this->GetPresetName(i).Get());
	SendDlgItemMessage(m_hwnd, IDC_PRESET, CB_SETCURSEL, m_currentPreset, 0);

	// Options: All
	int x;
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ALL_FOCUS_COMBO, CB_ADDSTRING, 0, (LPARAM)"All");
	SendDlgItemMessage(m_hwnd, IDC_ALL_FOCUS_COMBO, CB_SETITEMDATA, x, (LPARAM)(FOCUS_ALL));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ALL_FOCUS_COMBO, CB_ADDSTRING, 0, (LPARAM)"Main window");
	SendDlgItemMessage(m_hwnd, IDC_ALL_FOCUS_COMBO, CB_SETITEMDATA, x, (LPARAM)(FOCUS_MAIN));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ALL_FOCUS_COMBO, CB_ADDSTRING, 0, (LPARAM)"MIDI editor");
	SendDlgItemMessage(m_hwnd, IDC_ALL_FOCUS_COMBO, CB_SETITEMDATA, x, (LPARAM)(FOCUS_MIDI));

	// Options: TCP
	x = (int)SendDlgItemMessage(m_hwnd, IDC_TCP_TRACK_COMBO, CB_ADDSTRING, 0, (LPARAM)"Select track");
	SendDlgItemMessage(m_hwnd, IDC_TCP_TRACK_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK|CLEAR_TRACK_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_TCP_TRACK_COMBO, CB_ADDSTRING, 0, (LPARAM)"Add track to selection");
	SendDlgItemMessage(m_hwnd, IDC_TCP_TRACK_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK));

	x = (int)SendDlgItemMessage(m_hwnd, IDC_TCP_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)"Select envelope");
	SendDlgItemMessage(m_hwnd, IDC_TCP_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ENVELOPE));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_TCP_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)"Select envelope and parent track");
	SendDlgItemMessage(m_hwnd, IDC_TCP_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ENVELOPE|SELECT_TRACK|CLEAR_TRACK_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_TCP_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)"Select envelope and add parent track to selection");
	SendDlgItemMessage(m_hwnd, IDC_TCP_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ENVELOPE|SELECT_TRACK));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_TCP_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)"Select parent track");
	SendDlgItemMessage(m_hwnd, IDC_TCP_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK|CLEAR_TRACK_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_TCP_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)"Add parent track to selection");
	SendDlgItemMessage(m_hwnd, IDC_TCP_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK));

	// Options: MCP
	x = (int)SendDlgItemMessage(m_hwnd, IDC_MCP_TRACK_COMBO, CB_ADDSTRING, 0, (LPARAM)"Select track");
	SendDlgItemMessage(m_hwnd, IDC_MCP_TRACK_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK|CLEAR_TRACK_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_MCP_TRACK_COMBO, CB_ADDSTRING, 0, (LPARAM)"Add track to selection");
	SendDlgItemMessage(m_hwnd, IDC_MCP_TRACK_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK));

	// Options: Arrange
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_TRACK_COMBO, CB_ADDSTRING, 0, (LPARAM)"Select track");
	SendDlgItemMessage(m_hwnd, IDC_ARG_TRACK_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK|CLEAR_TRACK_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_TRACK_COMBO, CB_ADDSTRING, 0, (LPARAM)"Add track to selection");
	SendDlgItemMessage(m_hwnd, IDC_ARG_TRACK_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK));

	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)"Select item");
	SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM|CLEAR_ITEM_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)"Select item and parent track");
	SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM|CLEAR_ITEM_SELECTION|SELECT_TRACK|CLEAR_TRACK_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)"Select item and add parent track to selection");
	SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM|CLEAR_ITEM_SELECTION|SELECT_TRACK));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)"Select parent track");
	SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK|CLEAR_TRACK_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)"Add item to selection");
	SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)"Add item to selection and select parent track");
	SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM|SELECT_TRACK|CLEAR_TRACK_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)"Add item and parent track to selection");
	SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM|SELECT_TRACK));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)"Add parent track to selection");
	SendDlgItemMessage(m_hwnd, IDC_ARG_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK));

	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_TRACK_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)"Select envelope");
	SendDlgItemMessage(m_hwnd, IDC_ARG_TRACK_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ENVELOPE));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_TRACK_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)"Select envelope and parent track");
	SendDlgItemMessage(m_hwnd, IDC_ARG_TRACK_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ENVELOPE|SELECT_TRACK|CLEAR_TRACK_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_TRACK_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)"Select envelope and add parent track to selection");
	SendDlgItemMessage(m_hwnd, IDC_ARG_TRACK_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ENVELOPE|SELECT_TRACK));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_TRACK_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)"Select parent track");
	SendDlgItemMessage(m_hwnd, IDC_ARG_TRACK_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK|CLEAR_TRACK_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_TRACK_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)"Add parent track to selection");
	SendDlgItemMessage(m_hwnd, IDC_ARG_TRACK_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK));

	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_TAKE_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)"Select envelope");
	SendDlgItemMessage(m_hwnd, IDC_ARG_TAKE_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ENVELOPE));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_TAKE_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)"Select envelope and parent item");
	SendDlgItemMessage(m_hwnd, IDC_ARG_TAKE_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ENVELOPE|SELECT_ITEM|CLEAR_ITEM_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_TAKE_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)"Select envelope and add parent item to selection");
	SendDlgItemMessage(m_hwnd, IDC_ARG_TAKE_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ENVELOPE|SELECT_ITEM));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_TAKE_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)"Select parent item");
	SendDlgItemMessage(m_hwnd, IDC_ARG_TAKE_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM|CLEAR_ITEM_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_ARG_TAKE_ENV_COMBO, CB_ADDSTRING, 0, (LPARAM)"Add parent item to selection");
	SendDlgItemMessage(m_hwnd, IDC_ARG_TAKE_ENV_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM));

	// Options: Inline MIDI editor
	x = (int)SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)"Select item");
	SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM|CLEAR_ITEM_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)"Select item and parent track");
	SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM|CLEAR_ITEM_SELECTION|SELECT_TRACK|CLEAR_TRACK_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)"Select item and add parent track to selection");
	SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM|CLEAR_ITEM_SELECTION|SELECT_TRACK));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)"Select parent track");
	SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK|CLEAR_TRACK_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)"Add item to selection");
	SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)"Add item to selection and select parent track");
	SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM|SELECT_TRACK|CLEAR_TRACK_SELECTION));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)"Add item and parent track to selection");
	SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_ITEM|SELECT_TRACK));
	x = (int)SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_ADDSTRING, 0, (LPARAM)"Add parent track to selection");
	SendDlgItemMessage(m_hwnd, IDC_INLINE_ITEM_COMBO, CB_SETITEMDATA, x, (LPARAM)(SELECT_TRACK));

	this->Update();
}

void BR_ContextualToolbarsWnd::OnCommand (WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
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

		case IDC_ALL_FOCUS:
		case IDC_ALL_FOCUS_COMBO:
		case IDC_TCP_TRACK:
		case IDC_TCP_TRACK_COMBO:
		case IDC_TCP_ENV:
		case IDC_TCP_ENV_COMBO:
		case IDC_MCP_TRACK:
		case IDC_MCP_TRACK_COMBO:
		case IDC_ARG_TRACK:
		case IDC_ARG_TRACK_COMBO:
		case IDC_ARG_ITEM:
		case IDC_ARG_ITEM_COMBO:
		case IDC_ARG_TRACK_ENV:
		case IDC_ARG_TRACK_ENV_COMBO:
		case IDC_ARG_TAKE_ENV:
		case IDC_ARG_TAKE_ENV_COMBO:
		case IDC_INLINE_ITEM:
		case IDC_INLINE_ITEM_COMBO:
		{
			int checkBox = 0;
			int combo    = 0;
			if      (LOWORD(wParam) == IDC_ALL_FOCUS     || (LOWORD(wParam) == IDC_ALL_FOCUS_COMBO     && HIWORD(wParam) == CBN_SELCHANGE)) {checkBox = IDC_ALL_FOCUS;     combo = IDC_ALL_FOCUS_COMBO;}
			else if (LOWORD(wParam) == IDC_TCP_TRACK     || (LOWORD(wParam) == IDC_TCP_TRACK_COMBO     && HIWORD(wParam) == CBN_SELCHANGE)) {checkBox = IDC_TCP_TRACK;     combo = IDC_TCP_TRACK_COMBO;}
			else if (LOWORD(wParam) == IDC_TCP_ENV       || (LOWORD(wParam) == IDC_TCP_ENV_COMBO       && HIWORD(wParam) == CBN_SELCHANGE)) {checkBox = IDC_TCP_ENV;       combo = IDC_TCP_ENV_COMBO;}
			else if (LOWORD(wParam) == IDC_MCP_TRACK     || (LOWORD(wParam) == IDC_MCP_TRACK_COMBO     && HIWORD(wParam) == CBN_SELCHANGE)) {checkBox = IDC_MCP_TRACK;     combo = IDC_MCP_TRACK_COMBO;}
			else if (LOWORD(wParam) == IDC_ARG_TRACK     || (LOWORD(wParam) == IDC_ARG_TRACK_COMBO     && HIWORD(wParam) == CBN_SELCHANGE)) {checkBox = IDC_ARG_TRACK;     combo = IDC_ARG_TRACK_COMBO;}
			else if (LOWORD(wParam) == IDC_ARG_ITEM      || (LOWORD(wParam) == IDC_ARG_ITEM_COMBO      && HIWORD(wParam) == CBN_SELCHANGE)) {checkBox = IDC_ARG_ITEM;      combo = IDC_ARG_ITEM_COMBO;}
			else if (LOWORD(wParam) == IDC_ARG_TRACK_ENV || (LOWORD(wParam) == IDC_ARG_TRACK_ENV_COMBO && HIWORD(wParam) == CBN_SELCHANGE)) {checkBox = IDC_ARG_TRACK_ENV; combo = IDC_ARG_TRACK_ENV_COMBO;}
			else if (LOWORD(wParam) == IDC_ARG_TAKE_ENV  || (LOWORD(wParam) == IDC_ARG_TAKE_ENV_COMBO  && HIWORD(wParam) == CBN_SELCHANGE)) {checkBox = IDC_ARG_TAKE_ENV;  combo = IDC_ARG_TAKE_ENV_COMBO;}
			else if (LOWORD(wParam) == IDC_INLINE_ITEM   || (LOWORD(wParam) == IDC_INLINE_ITEM_COMBO   && HIWORD(wParam) == CBN_SELCHANGE)) {checkBox = IDC_INLINE_ITEM;   combo = IDC_INLINE_ITEM_COMBO;}

			if (checkBox && combo)
			{
				EnableWindow(GetDlgItem(m_hwnd, combo), IsDlgButtonChecked(m_hwnd, checkBox));

				int option = 0;
				option |= (!!IsDlgButtonChecked(m_hwnd, checkBox)) ? OPTION_ENABLED : 0;
				option |= SendDlgItemMessage(m_hwnd, combo, CB_GETITEMDATA, SendDlgItemMessage(m_hwnd, combo, CB_GETCURSEL, 0, 0), 0);


				if      (checkBox == IDC_ALL_FOCUS)     m_currentToolbar.SetOptionsAll(&option, NULL);
				else if (checkBox == IDC_TCP_TRACK)     m_currentToolbar.SetOptionsTcp(&option, NULL);
				else if (checkBox == IDC_TCP_ENV)       m_currentToolbar.SetOptionsTcp(NULL, &option);
				else if (checkBox == IDC_MCP_TRACK)     m_currentToolbar.SetOptionsMcp(&option);
				else if (checkBox == IDC_ARG_TRACK)     m_currentToolbar.SetOptionsArrange(&option, NULL, NULL, NULL, NULL);
				else if (checkBox == IDC_ARG_ITEM)      m_currentToolbar.SetOptionsArrange(NULL, &option, NULL, NULL, NULL);
				else if (checkBox == IDC_ARG_TRACK_ENV) m_currentToolbar.SetOptionsArrange(NULL, NULL, &option, NULL, NULL);
				else if (checkBox == IDC_ARG_TAKE_ENV)  m_currentToolbar.SetOptionsArrange(NULL, NULL, NULL, &option, NULL);
				else if (checkBox == IDC_INLINE_ITEM)   m_currentToolbar.SetOptionsInline(&option, NULL);
			}
		}
		break;

		case IDC_ALL_TOPMOST:
		case IDC_ARG_TAKE_ACTIVATE:
		case IDC_MIDI_CC_LANE_CLICKED:
		case IDC_INLINE_CC_LANE_CLICKED:
		{
			int checkBox = LOWORD(wParam);
			int option = (IsDlgButtonChecked(m_hwnd, checkBox)) ? OPTION_ENABLED : 0;

			if      (checkBox == IDC_ALL_TOPMOST)            m_currentToolbar.SetOptionsAll(NULL, &option);
			else if (checkBox == IDC_ARG_TAKE_ACTIVATE)      m_currentToolbar.SetOptionsArrange(NULL, NULL, NULL, NULL, &option);
			else if (checkBox == IDC_MIDI_CC_LANE_CLICKED)   m_currentToolbar.SetOptionsMIDI(&option);
			else if (checkBox == IDC_INLINE_CC_LANE_CLICKED) m_currentToolbar.SetOptionsInline(NULL, &option);

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
	_snprintfSafe(currentPresetStr, sizeof(currentPresetStr), "%d", m_currentPreset);
	WritePrivateProfileString(INI_SECTION, INI_KEY_CURRENT_PRESET, currentPresetStr, BR_GetIniFile());
}

void BR_ContextualToolbarsWnd::GetMinSize (int* w, int* h)
{
	*w = 500;
	#ifdef _WIN32
		*h = 610;
	#else
		*h = 636;
	#endif
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
	if (int context = (int)m_list->GetHitItem(x, y, &column))
	{
		menu = CreatePopupMenu();
		WritePtr(wantDefaultItems, false);

		AddToMenu(menu, __LOCALIZE("Do nothing", "sws_DLG_181"), DO_NOTHING, -1, false);
		if (m_list->CountSelected() > 1 || g_toolbarsManager.CanContextInheritParent(context))
			AddToMenu(menu, __LOCALIZE("Inherit parent", "sws_DLG_181"), INHERIT_PARENT, -1, false);

		int x = 0;
		bool followItem = true;
		while (int selectedContext = (int)m_list->EnumSelected(&x))
		{
			if (!g_toolbarsManager.CanContextFollowItem(selectedContext))
			{
				followItem = false;
				break;
			}
		}
		if (followItem)
			AddToMenu(menu, __LOCALIZE("Follow item context", "sws_DLG_181"), FOLLOW_ITEM_CONTEXT, -1, false);

		AddToMenu(menu, SWS_SEPARATOR, 0);

		int id = -1;
		int mouseAction;
		char toolbarName[256];
		while (g_toolbarsManager.GetReaperToolbar(++id, &mouseAction, NULL, toolbarName, sizeof(toolbarName)))
		{
			AddToMenu(menu, toolbarName, mouseAction, -1, false);
			if (mouseAction == TOOLBAR_8_MOUSE)
				AddToMenu(menu, SWS_SEPARATOR, 0);
		}
	}
	return menu;
}

void BR_ContextualToolbarsWnd::ContextMenuReturnId (int id)
{
	if (id)
	{
		int toggleAction = g_toolbarsManager.GetToggleToolbarAction(id);

		int x = 0;
		while (int context = (int)m_list->EnumSelected(&x))
		{
			m_currentToolbar.SetContext(context, id, toggleAction);
		}
		m_list->Update();
	}
}

/******************************************************************************
* Contextual toolbars init/exit                                               *
******************************************************************************/
int ContextToolbarsInit ()
{
	g_contextToolbarsWndManager.Init();
	return 1;
}

void ContextToolbarsExit ()
{
	g_contextToolbarsWndManager.Delete();
}

/******************************************************************************
* Commands                                                                    *
******************************************************************************/
void ContextToolbarsOptions (COMMAND_T* ct)
{
	if (BR_ContextualToolbarsWnd* dialog = g_contextToolbarsWndManager.Create())
		dialog->Show(true, true);
}

void ToggleContextualToolbar (COMMAND_T* ct)
{
	if (BR_ContextualToolbar* contextualToolbar = g_toolbarsManager.GetContextualToolbar((int)ct->user))
		contextualToolbar->LoadToolbar();
}

void ToggleContextualToolbar (COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd)
{
	ToggleContextualToolbar(ct);
}
/******************************************************************************
* Toggle states                                                               *
******************************************************************************/
int IsContextToolbarsOptionsVisible (COMMAND_T* ct)
{
	if (BR_ContextualToolbarsWnd* dialog = g_contextToolbarsWndManager.Get())
		return dialog->IsValidWindow();
	return 0;
}