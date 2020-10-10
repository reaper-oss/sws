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
enum OptionFlags {
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

	/* Get toolbar info */
	int CountToolbars ();
	int GetToolbarType (int toolbarId); // -1 -> unknown, 0 -> do nothing, 1 -> inherit parent, 2 -> follow item context, 3 -> normal toolbar
	bool GetToolbarName (int toolbarId, char* toolbarName, int toolbarNameSz);
	bool IsFirstToolbar (int toolbarId);
	bool IsFirstMidiToolbar (int toolbarId);

	/* Get and set toolbar info for specific context (for list of contexts see BR_ToolbarContexts) */
	bool SetContext (int context, int* toolbarId, bool* autoClose, int* positionOffsetX, int* positionOffsetY);
	bool GetContext (int context, int* toolbarId, bool* autoclose, int* positionOffsetX, int* positionOffsetY);

	/* Check contexts for various types */
	bool IsContextValid (int context);          // Check if context is nonexistent or something like END_ARRANGE etc...
	bool CanContextInheritParent (int context); // Top contexts can't inherit parent since they have none
	bool CanContextFollowItem (int context);    // Check if context can follow item contexts

	/* Get and set options for toolbar loading */
	Options &GetOptions() { return m_options; }

	/* For serializing */
	void ExportConfig (WDL_FastString& contextToolbars, WDL_FastString& contextAutoClose, WDL_FastString& contextPosition, WDL_FastString& options);
	void ImportConfig (const char* contextToolbars, const char* contextAutoClose, const char* contextPosition, const char* options);

	/* Call when unloading extension to make sure everything is cleaned up properly */
	static void Cleaup ();

private:
	enum class AutoClose {
		Disabled, Command, Inactive,
	};
	struct ContextInfo
	{
		int mouseAction = DO_NOTHING, toggleAction = DO_NOTHING;
		int positionOffsetX, positionOffsetY;
		bool autoClose;
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
	enum ToolbarActions
	{
		/* If adding or removing new actions          *
		*  make sure to update GetReaperToolbar(),    *
		*  TranslateAction(), IsFirstToolbar(),       *
		*  IsFirstMidiToolbar() and IsToolbarAction() */

		TOOLBAR_COUNT         = 27, // includes both toolbars and other entries (inherit parent etc...)

		DO_NOTHING            = 1,
		INHERIT_PARENT        = 2,
		FOLLOW_ITEM_CONTEXT   = 3,
		TOOLBAR_01_MOUSE      = 41111,
		TOOLBAR_02_MOUSE      = 41112,
		TOOLBAR_03_MOUSE      = 41113,
		TOOLBAR_04_MOUSE      = 41114,
		TOOLBAR_05_MOUSE      = 41655,
		TOOLBAR_06_MOUSE      = 41656,
		TOOLBAR_07_MOUSE      = 41657,
		TOOLBAR_08_MOUSE      = 41658,
		TOOLBAR_09_MOUSE      = 41960,
		TOOLBAR_10_MOUSE      = 41961,
		TOOLBAR_11_MOUSE      = 41962,
		TOOLBAR_12_MOUSE      = 41963,
		TOOLBAR_13_MOUSE      = 41964,
		TOOLBAR_14_MOUSE      = 41965,
		TOOLBAR_15_MOUSE      = 41966,
		TOOLBAR_16_MOUSE      = 41967,
		MIDI_TOOLBAR_01_MOUSE = 41640,
		MIDI_TOOLBAR_02_MOUSE = 41641,
		MIDI_TOOLBAR_03_MOUSE = 41642,
		MIDI_TOOLBAR_04_MOUSE = 41643,
		MIDI_TOOLBAR_05_MOUSE = 41968,
		MIDI_TOOLBAR_06_MOUSE = 41969,
		MIDI_TOOLBAR_07_MOUSE = 41970,
		MIDI_TOOLBAR_08_MOUSE = 41971,
		TOOLBAR_01_TOGGLE     = 41679,
		TOOLBAR_02_TOGGLE     = 41680,
		TOOLBAR_03_TOGGLE     = 41681,
		TOOLBAR_04_TOGGLE     = 41682,
		TOOLBAR_05_TOGGLE     = 41683,
		TOOLBAR_06_TOGGLE     = 41684,
		TOOLBAR_07_TOGGLE     = 41685,
		TOOLBAR_08_TOGGLE     = 41686,
		TOOLBAR_09_TOGGLE     = 41936,
		TOOLBAR_10_TOGGLE     = 41937,
		TOOLBAR_11_TOGGLE     = 41938,
		TOOLBAR_12_TOGGLE     = 41939,
		TOOLBAR_13_TOGGLE     = 41940,
		TOOLBAR_14_TOGGLE     = 41941,
		TOOLBAR_15_TOGGLE     = 41942,
		TOOLBAR_16_TOGGLE     = 41943,
		MIDI_TOOLBAR_01_TOGGLE = 41687,
		MIDI_TOOLBAR_02_TOGGLE = 41688,
		MIDI_TOOLBAR_03_TOGGLE = 41689,
		MIDI_TOOLBAR_04_TOGGLE = 41690,
		MIDI_TOOLBAR_05_TOGGLE = 41944,
		MIDI_TOOLBAR_06_TOGGLE = 41945,
		MIDI_TOOLBAR_07_TOGGLE = 41946,
		MIDI_TOOLBAR_08_TOGGLE = 41947
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
	int GetMouseAction (int context);
	int GetToggleAction (int context);
	int GetPositionOffsetX (int context);
	int GetPositionOffsetY (int context);
	AutoClose GetAutoClose (int context);
	void SetMouseAction (int context, int mouseAction);
	void SetToggleAction (int context, int toggleAction);
	void SetPositionOffset (int context, int x, int y);
	void SetAutoClose (int context, bool autoClose);

	/* Used for serializing data */
	int TranslateAction (int mouseAction, bool toIni);
	int GetContextFromIniIndex (int index);

	/* Get toolbar info */
	bool GetReaperToolbar (int id, int* mouseAction, int* toggleAction, char* toolbarName, int toolbarNameSz); // id is 0-based, returns false if toolbar doesn't exist
	bool IsToolbarAction (int action);                                                                         // returns false in case action is something like DO_NOTHING etc..
	int GetToolbarId (int mouseAction);

	/* Toolbar manipulation */
	HWND GetFloatingToolbarHwnd (int mouseAction, bool* inDocker);
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
	bool GetPositionOffsetFromUser (int& x, int&y);
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
