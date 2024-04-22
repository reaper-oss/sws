/******************************************************************************
/ BR_ContextualToolbars.h
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
#pragma once

class BR_MouseInfo;

/******************************************************************************
* Constants                                                                   *
******************************************************************************/
enum BR_ToolbarContext
{
	/* If adding new context group make sure to update IsContextValid() in    *
	*  BR_ContextualToolbar                                                   *
	*                                                                         *
	*  If adding new context or removing existing one from the existing group *
	*  update UpdateInternals(), GetContextFromIniIndex() and any other       *
	*  detection function in BR_ContextualToolbar. You also need to update    *
	*  GetItemText() in BR_ContextualToolbarsView                             *
	*                                                                         *
	*  If removing existing context, also increase REMOVED_CONTEXTS           */

	CONTEXT_START  = 0,
	UNUSED_CONTEXT = CONTEXT_START - 1,

	TRANSPORT      = CONTEXT_START,
	END_TRANSPORT,

	RULER,
	RULER_REGIONS,
	RULER_MARKERS,
	RULER_TEMPO,
	RULER_TIMELINE,
	END_RULER,

	TCP,
	TCP_EMPTY,
	TCP_TRACK,
	TCP_TRACK_MASTER,
	TCP_ENVELOPE,
	TCP_ENVELOPE_VOLUME,
	TCP_ENVELOPE_PAN,
	TCP_ENVELOPE_WIDTH,
	TCP_ENVELOPE_MUTE,
	TCP_ENVELOPE_PLAYRATE,
	TCP_ENVELOPE_TEMPO,
	END_TCP,

	MCP,
	MCP_EMPTY,
	MCP_TRACK,
	MCP_TRACK_MASTER,
	END_MCP,

	ARRANGE,
	ARRANGE_EMPTY,
	ARRANGE_TRACK,
	ARRANGE_TRACK_EMPTY,
	ARRANGE_TRACK_ITEM,
	ARRANGE_TRACK_ITEM_AUDIO,
	ARRANGE_TRACK_ITEM_MIDI,
	ARRANGE_TRACK_ITEM_VIDEO,
	ARRANGE_TRACK_ITEM_EMPTY,
	ARRANGE_TRACK_ITEM_CLICK,
	ARRANGE_TRACK_ITEM_TIMECODE,
	ARRANGE_TRACK_ITEM_PROJECT,
	ARRANGE_TRACK_ITEM_VIDEOFX,
	ARRANGE_TRACK_ITEM_STRETCH_MARKER,
	ARRANGE_TRACK_TAKE_ENVELOPE,
	ARRANGE_TRACK_TAKE_ENVELOPE_VOLUME,
	ARRANGE_TRACK_TAKE_ENVELOPE_PAN,
	ARRANGE_TRACK_TAKE_ENVELOPE_MUTE,
	ARRANGE_TRACK_TAKE_ENVELOPE_PITCH,
	ARRANGE_ENVELOPE_TRACK,
	ARRANGE_ENVELOPE_TRACK_VOLUME,
	ARRANGE_ENVELOPE_TRACK_PAN,
	ARRANGE_ENVELOPE_TRACK_WIDTH,
	ARRANGE_ENVELOPE_TRACK_MUTE,
	ARRANGE_ENVELOPE_TRACK_PITCH,
	ARRANGE_ENVELOPE_TRACK_PLAYRATE,
	ARRANGE_ENVELOPE_TRACK_TEMPO,
	END_ARRANGE,

	MIDI_EDITOR,
	MIDI_EDITOR_RULER,
	MIDI_EDITOR_PIANO,
	MIDI_EDITOR_PIANO_NAMED,
	MIDI_EDITOR_NOTES,
	MIDI_EDITOR_CC_LANE,
	MIDI_EDITOR_CC_SELECTOR,
	END_MIDI_EDITOR,

	INLINE_MIDI_EDITOR,
	INLINE_MIDI_EDITOR_PIANO,
	INLINE_MIDI_EDITOR_NOTES,
	INLINE_MIDI_EDITOR_CC_LANE,

	CONTEXT_COUNT,
	REMOVED_CONTEXTS = 1
};

// never change values - used for state saving to .ini
enum OptionFlags
{
	OPTION_ENABLED         = 0x1,
	SELECT_ITEM            = 0x2,
	SELECT_TRACK           = 0x4,
	SELECT_ENVELOPE        = 0x8,
	CLEAR_ITEM_SELECTION   = 0x10,
	CLEAR_TRACK_SELECTION  = 0x20,
	FOCUS_ALL              = 0x40,
	FOCUS_MAIN             = 0x80,
	FOCUS_MIDI             = 0x100,
	POSITION_H_LEFT        = 0x200,
	POSITION_H_MIDDLE      = 0x400,
	POSITION_H_RIGHT       = 0x800,
	POSITION_V_TOP         = 0x2000,
	POSITION_V_MIDDLE      = 0x4000,
	POSITION_V_BOTTOM      = 0x8000,
};

struct ContextAction;

/******************************************************************************
* Contextual toolbar                                                          *
******************************************************************************/
class BR_ContextualToolbar
{
public:
	struct Options
	{
		int focus = OPTION_ENABLED | FOCUS_ALL, topmost, position,
				setToolbarToForeground, autocloseInactive;
		int tcpTrack, tcpEnvelope;
		int mcpTrack;
		int arrangeTrack, arrangeItem, arrangeStretchMarker, arrangeTakeEnvelope,
				arrangeTrackEnvelope, arrangeActTake;
		int midiSetCCLane;
		int inlineItem, inlineSetCCLane;
	};

	BR_ContextualToolbar ();
	BR_ContextualToolbar (const BR_ContextualToolbar& contextualToolbar);
	~BR_ContextualToolbar ();
	BR_ContextualToolbar& operator=  (const BR_ContextualToolbar& contextualToolbar);
	bool                  operator== (const BR_ContextualToolbar& contextualToolbar);
	bool                  operator!= (const BR_ContextualToolbar& contextualToolbar);

	/* Toggle appropriate toolbar under mouse cursor */
	void LoadToolbar (bool exclusive);
	bool AreAssignedToolbarsOpened ();

	/* Get and set toolbar info for specific context (for list of contexts see BR_ToolbarContexts) */
	bool SetContext (int context, const ContextAction*, const bool* autoClose, const int* positionOffsetX, const int* positionOffsetY);
	bool GetContext (int context, const ContextAction**, bool* autoclose, int* positionOffsetX, int* positionOffsetY);
	const ContextAction &GetContextAction (int context) const;

	/* Get and set options for toolbar loading */
	Options &GetOptions() { return m_options; }

	/* For serializing */
	void ExportConfig (WDL_FastString& contextToolbars, WDL_FastString& contextAutoClose, WDL_FastString& contextPosition, WDL_FastString& options);
	void ImportConfig (const char* contextToolbars, const char* contextAutoClose, const char* contextPosition, const char* options);

	/* Call when unloading extension to make sure everything is cleaned up properly */
	static void Cleaup ();

private:
	struct ContextInfo
	{
		ContextInfo();
		const ContextAction *action; // non-null
		int positionOffsetX, positionOffsetY;
		bool autoClose;
	};
	enum class AutoClose {
		Disabled, Command, Inactive,
	};
	struct ExecuteOnToolbarLoad
	{
		HWND focusHwnd;
		MediaTrack* trackToSelect;
		TrackEnvelope* envelopeToSelect;
		MediaItem* itemToSelect;
		MediaItem* takeParent;
		int takeIdToActivate = -1, focusContext = -1, positionOffsetX, positionOffsetY, positionOrientation;
		bool positionOverride, setFocus, clearTrackSelection, clearItemSelection, setCCLaneAsClicked, makeTopMost, setToolbarToForeground;
		AutoClose autoCloseToolbar;
	};
	struct ToolbarWndData
	{
		HWND hwnd;
		HWND lastFocusedHwnd;
		WNDPROC wndProc;
		bool keepOnTop;
		AutoClose autoClose;
		int toggleAction = -1;
		#ifndef _WIN32
			int level;
		#endif
	};

	/* Call every time contexts change */
	void UpdateInternals ();

	/* Find context under mouse cursor */
	int FindRulerToolbar      (BR_MouseInfo& mouseInfo, BR_ContextualToolbar::ExecuteOnToolbarLoad& executeOnToolbarLoad);
	int FindTransportToolbar  (BR_MouseInfo& mouseInfo, BR_ContextualToolbar::ExecuteOnToolbarLoad& executeOnToolbarLoad);
	int FindTcpToolbar        (BR_MouseInfo& mouseInfo, BR_ContextualToolbar::ExecuteOnToolbarLoad& executeOnToolbarLoad);
	int FindMcpToolbar        (BR_MouseInfo& mouseInfo, BR_ContextualToolbar::ExecuteOnToolbarLoad& executeOnToolbarLoad);
	int FindArrangeToolbar    (BR_MouseInfo& mouseInfo, BR_ContextualToolbar::ExecuteOnToolbarLoad& executeOnToolbarLoad);
	int FindMidiToolbar       (BR_MouseInfo& mouseInfo, BR_ContextualToolbar::ExecuteOnToolbarLoad& executeOnToolbarLoad);
	int FindInlineMidiToolbar (BR_MouseInfo& mouseInfo, BR_ContextualToolbar::ExecuteOnToolbarLoad& executeOnToolbarLoad);

	/* Execute options, returns true if undo point is needed */
	bool SelectTrack           (BR_ContextualToolbar::ExecuteOnToolbarLoad& executeOnToolbarLoad);
	bool SelectItem            (BR_ContextualToolbar::ExecuteOnToolbarLoad& executeOnToolbarLoad);
	bool SelectEnvelope        (BR_ContextualToolbar::ExecuteOnToolbarLoad& executeOnToolbarLoad);
	bool ActivateTake          (BR_ContextualToolbar::ExecuteOnToolbarLoad& executeOnToolbarLoad);
	void SetFocus              (BR_ContextualToolbar::ExecuteOnToolbarLoad& executeOnToolbarLoad);
	void SetCCLaneClicked      (BR_ContextualToolbar::ExecuteOnToolbarLoad& executeOnToolbarLoad, BR_MouseInfo& mouseInfo);
	void RepositionToolbar     (BR_ContextualToolbar::ExecuteOnToolbarLoad& executeOnToolbarLoad, HWND toolbarHwnd);
	void SetToolbarWndProc (BR_ContextualToolbar::ExecuteOnToolbarLoad& executeOnToolbarLoad, HWND toolbarHwnd, int toggleAction, HWND toolbarParent);

	/* Set and get info for various contexts */
	int GetPositionOffsetX (int context);
	int GetPositionOffsetY (int context);
	AutoClose GetAutoClose (int context);
	void SetContextAction (int context, const ContextAction &);
	void SetPositionOffset (int context, int x, int y);
	void SetAutoClose (int context, bool autoClose);

	/* Used for serializing data */
	int GetContextFromIniIndex (int index);

	/* Toolbar manipulation */
	HWND GetFloatingToolbarHwnd (const ContextAction&, bool* inDocker);
	void CloseAllAssignedToolbars ();

	/* Ensures that toolbar is always on top or auto closed on button press */
	static LRESULT CALLBACK ToolbarWndCallback (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static void TooltipTimer ();

	ContextInfo m_contexts[CONTEXT_COUNT];
	Options m_options;
	int m_mode;
	std::set<int> m_activeContexts;
	static WDL_PtrList<BR_ContextualToolbar::ToolbarWndData> m_callbackToolbars;
	static bool tooltipTimerActive;
};

/******************************************************************************
* Contextual toolbars manager                                                 *
******************************************************************************/
class BR_ContextualToolbarsManager
{
public:
	BR_ContextualToolbarsManager ();
	~BR_ContextualToolbarsManager ();

	/* Get and set contextual toolbar by id (will serialize state) */
	BR_ContextualToolbar* GetContextualToolbar (int id);
	void                  SetContextualToolbar (int id, BR_ContextualToolbar& contextualToolbars);

private:
	WDL_FastString GetKeyForPreset(const char* key, int id);
	static WDL_PtrList<BR_ContextualToolbar> m_contextualToolbars;
};

/******************************************************************************
* Context toolbars list view                                                  *
******************************************************************************/
class BR_ContextualToolbarsView : public SWS_ListView
{
public:
	BR_ContextualToolbarsView (HWND hwndList, HWND hwndEdit);

protected:
	virtual void GetItemText (SWS_ListItem* item, int iCol, char* str, int iStrMax);
	virtual void GetItemList (SWS_ListItemList* pList);
	virtual bool OnItemSelChanging (SWS_ListItem* item, bool bSel);
	virtual void OnItemDblClk (SWS_ListItem* item, int iCol);
	virtual bool HideGridLines ();
	virtual int OnItemSort (SWS_ListItem* item1, SWS_ListItem* item2);
};

/******************************************************************************
* Context toolbars window                                                     *
******************************************************************************/
class BR_ContextualToolbarsWnd : public SWS_DockWnd
{
public:
	BR_ContextualToolbarsWnd ();
	~BR_ContextualToolbarsWnd ();

	void Update ();
	bool CheckForModificationsAndSave (bool onClose);
	void SetPositionOffsetFromUser ();
	BR_ContextualToolbar* GetCurrentContextualToolbar ();
	WDL_FastString GetPresetName (int preset);

protected:
	static WDL_DLGRET PositionOffsetDialogProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void OnInitDlg ();
	virtual void OnCommand (WPARAM wParam, LPARAM lParam);
	virtual bool CloseOnCancel ();
	virtual void OnDestroy ();
	virtual void GetMinSize (int* w, int* h);
	virtual int OnKey (MSG* msg, int iKeyState);
	virtual bool ReprocessContextMenu ();
	virtual bool NotifyOnContextMenu ();
	virtual HMENU OnContextMenu (int x, int y, bool* wantDefaultItems);
	virtual void ContextMenuReturnId (int id);

	BR_ContextualToolbarsView* m_list;
	BR_ContextualToolbar m_currentToolbar;
	int m_currentPreset;
	int m_contextMenuCol;
};

/******************************************************************************
* Contextual toolbars init/exit                                               *
******************************************************************************/
int ContextualToolbarsInitExit (bool init);

/******************************************************************************
* Commands                                                                    *
******************************************************************************/
void ContextualToolbarsOptions (COMMAND_T*);
void ToggleContextualToolbar (COMMAND_T*);
void ToggleContextualToolbar (COMMAND_T*, int, int, int, HWND);

/******************************************************************************
* Toggle states                                                               *
******************************************************************************/
int IsContextualToolbarsOptionsVisible (COMMAND_T*);
int IsContextualToolbarVisible (COMMAND_T*);
