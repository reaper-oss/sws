/******************************************************************************
/ BR_ContextualToolbars.h
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
#pragma once

class BR_MouseContextInfo;

/******************************************************************************
* Constants                                                                   *
******************************************************************************/
enum BR_ToolbarContexts
{
	CONTEXT_START = 1,             // make sure first enumerator is always 1, since they
	TRANSPORT     = CONTEXT_START, // are stored in the list view where 0 means invalid item
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
	INLINE_MIDI_EDITOR_PIANO_NAMED,
	INLINE_MIDI_EDITOR_NOTES,
	INLINE_MIDI_EDITOR_CC_LANE,

	CONTEXT_COUNT
};

/******************************************************************************
* Contextual toolbar                                                          *
******************************************************************************/
class BR_ContextualToolbar
{
public:
	BR_ContextualToolbar ();
	BR_ContextualToolbar (const BR_ContextualToolbar& contextualToolbar);
	~BR_ContextualToolbar ();
	BR_ContextualToolbar& operator=  (const BR_ContextualToolbar& contextualToolbar);
	bool                  operator== (const BR_ContextualToolbar& contextualToolbar);
	bool                  operator!= (const BR_ContextualToolbar& contextualToolbar);

	/* Toggle appropriate toolbar under mouse cursor */
	void LoadToolbar ();

	/* Get toolbar info */
	int CountToolbars ();
	int GetToolbarType (int toolbarId); // -1 -> unknown, 0 -> do nothing, 1 -> inherit parent, 2 -> follow item context, 3 -> normal toolbar
	bool GetToolbarName (int toolbarId, char* toolbarName, int toolbarNameSz);
	bool IsFirstToolbar (int toolbarId);
	bool IsFirstMidiToolbar (int toolbarId);

	/* Get and set toolbar info for specific context (for list of contexts see BR_ToolbarContexts) */
	bool SetContext (int context, int toolbarId);
	bool GetContext (int context, int* toolbarId);

	/* Check contexts for various types */
	bool IsContextValid (int context);          // Check if context is nonexistent or something like END_ARRANGE etc...
	bool CanContextInheritParent (int context); // Top contexts can't inherit parent since they have none
	bool CanContextFollowItem (int context);    // Check if contexts can follow item contexts

	/* Get and set options for toolbar loading */
	void SetOptionsAll (int* focus, int* topmost);
	void GetOptionsAll (int* focus, int* topmost);
	void SetOptionsTcp (int* track, int* envelope);
	void GetOptionsTcp (int* track, int* envelope);
	void SetOptionsMcp (int* track);
	void GetOptionsMcp (int* track);
	void SetOptionsArrange (int* track, int* item, int* stretchMarker, int* takeEnvelope, int* trackEnvelope, int* activateTake);
	void GetOptionsArrange (int* track, int* item, int* stretchMarker, int* takeEnvelope, int* trackEnvelope, int* activateTake);
	void SetOptionsMIDI (int* setCCLaneAsClicked);
	void GetOptionsMIDI (int* setCCLaneAsClicked);
	void SetOptionsInline (int* item, int* setCCLaneAsClicked);
	void GetOptionsInline (int* item, int* setCCLaneAsClicked);

	/* For serializing */
	void ExportConfig (char* contexts, int contextsSz, char* options, int optionsSz);
	void ImportConfig (const char* contexts, const char* options);

private:
	struct Options
	{
		int focus;
		int topmost;
		int tcpTrack, tcpEnvelope;
		int mcpTrack;
		int arrangeTrack, arrangeItem, arrangeStretchMarker, arrangeTakeEnvelope, arrangeTrackEnvelope, arrangeActTake;
		int midiSetCCLane;
		int inlineItem, inlineSetCCLane;
		Options ();
	};
	struct ExecuteOnToolbarLoad
	{
		HWND focusHwnd;
		MediaTrack* trackToSelect;
		TrackEnvelope* envelopeToSelect;
		MediaItem* itemToSelect;
		MediaItem* takeParent;
		int takeIdToActivate;
		int focusContext;
		bool setFocus;
		bool clearTrackSelection;
		bool clearItemSelection;
		bool setCCLaneAsClicked;
		ExecuteOnToolbarLoad();
	};
	struct ToolbarWndData
	{
		HWND hwnd;
		WNDPROC wndProc;
		#ifndef _WIN32
			int level;
		#endif;
	};
	enum ToolbarActions
	{
		DO_NOTHING            = 0xF001,
		INHERIT_PARENT        = 0xF002,
		FOLLOW_ITEM_CONTEXT   = 0xF003,
		TOOLBAR_1_MOUSE       = 41111,
		TOOLBAR_2_MOUSE       = 41112,
		TOOLBAR_3_MOUSE       = 41113,
		TOOLBAR_4_MOUSE       = 41114,
		TOOLBAR_5_MOUSE       = 41655,
		TOOLBAR_6_MOUSE       = 41656,
		TOOLBAR_7_MOUSE       = 41657,
		TOOLBAR_8_MOUSE       = 41658,
		MIDI_TOOLBAR_1_MOUSE  = 41640,
		MIDI_TOOLBAR_2_MOUSE  = 41641,
		MIDI_TOOLBAR_3_MOUSE  = 41642,
		MIDI_TOOLBAR_4_MOUSE  = 41643,
		TOOLBAR_1_TOGGLE      = 41679,
		TOOLBAR_2_TOGGLE      = 41680,
		TOOLBAR_3_TOGGLE      = 41681,
		TOOLBAR_4_TOGGLE      = 41682,
		TOOLBAR_5_TOGGLE      = 41683,
		TOOLBAR_6_TOGGLE      = 41684,
		TOOLBAR_7_TOGGLE      = 41685,
		TOOLBAR_8_TOGGLE      = 41686,
		MIDI_TOOLBAR_1_TOGGLE = 41687,
		MIDI_TOOLBAR_2_TOGGLE = 41688,
		MIDI_TOOLBAR_3_TOGGLE = 41689,
		MIDI_TOOLBAR_4_TOGGLE = 41690,

		TOOLBAR_COUNT         = 15
	};

	void UpdateInternals (); // call every time contexts change
	void SetToolbarAlwaysOnTop (int toolbarId);
	void CloseAllAssignedToolbars ();
	bool AreAssignedToolbarsOpened ();
	bool GetReaperToolbar (int id, int* mouseAction, int* toggleAction, char* toolbarName, int toolbarNameSz); // id is 0-based, returns false if toolbar doesn't exist
	bool IsToolbarAction (int action);                                                                         // returns false in case action is something like DO_NOTHING etc..
	int GetToolbarId (int mouseAction);
	int FindRulerToolbar      (BR_MouseContextInfo& mouseInfo, BR_ContextualToolbar::ExecuteOnToolbarLoad& executeOnToolbarLoad);
	int FindTransportToolbar  (BR_MouseContextInfo& mouseInfo, BR_ContextualToolbar::ExecuteOnToolbarLoad& executeOnToolbarLoad);
	int FindTcpToolbar        (BR_MouseContextInfo& mouseInfo, BR_ContextualToolbar::ExecuteOnToolbarLoad& executeOnToolbarLoad);
	int FindMcpToolbar        (BR_MouseContextInfo& mouseInfo, BR_ContextualToolbar::ExecuteOnToolbarLoad& executeOnToolbarLoad);
	int FindArrangeToolbar    (BR_MouseContextInfo& mouseInfo, BR_ContextualToolbar::ExecuteOnToolbarLoad& executeOnToolbarLoad);
	int FindMidiToolbar       (BR_MouseContextInfo& mouseInfo, BR_ContextualToolbar::ExecuteOnToolbarLoad& executeOnToolbarLoad);
	int FindInlineMidiToolbar (BR_MouseContextInfo& mouseInfo, BR_ContextualToolbar::ExecuteOnToolbarLoad& executeOnToolbarLoad);
	static LRESULT CALLBACK ToolbarWndCallback (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	BR_ContextualToolbar::Options m_options;
	int m_mouseActions[CONTEXT_COUNT];
	int m_toggleActions[CONTEXT_COUNT];
	int m_mode;
	set<int> m_activeContexts;
	static WDL_PtrList<BR_ContextualToolbar::ToolbarWndData> m_topToolbars;
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
	WDL_FastString GetContextKey (int id);
	WDL_FastString GetOptionsKey (int id);
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
	~BR_ContextualToolbarsWnd () {};

	void Update ();
	bool CheckForModificationsAndSave (bool onClose);
	BR_ContextualToolbar* GetCurrentContextualToolbar ();
	WDL_FastString GetPresetName (int preset);

protected:
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
};

/******************************************************************************
* Contextual toolbars init/exit                                               *
******************************************************************************/
int ContextToolbarsInit ();
void ContextToolbarsExit ();

/******************************************************************************
* Commands                                                                    *
******************************************************************************/
void ContextToolbarsOptions (COMMAND_T*);
void ToggleContextualToolbar (COMMAND_T*);
void ToggleContextualToolbar (COMMAND_T*, int, int, int, HWND);

/******************************************************************************
* Toggle states                                                               *
******************************************************************************/
int IsContextToolbarsOptionsVisible (COMMAND_T*);