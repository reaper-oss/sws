/******************************************************************************
/ SnM_NotesHelpView.cpp
/
/ Copyright (c) 2009-2010 Tim Payne (SWS), Jeffos 
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

//JFB TODO?
// - action_help_t (even if not used yet ?)
// - drag'n'drop text
// - undo on *each* key stroke.. hum.. see "JFB2" tags
//   => fills undo history (+ can't restore caret pos.) but it fixes:
//   * SaveExtensionConfig() not called if no proj mods but notes may have been added..
//   * project switches

#include "stdafx.h"
#include "SnM_Actions.h"
#include "SNM_NotesHelpView.h"
#include "SNM_ChunkParserPatcher.h"

#define MAX_HELP_LENGTH				4096 // MAX_INI_SECTION;
#define SAVEWINDOW_POS_KEY			"S&M - Notes/help Save Window Position"

// Msgs
#define SET_ACTION_HELP_FILE_MSG	0x110001

enum {
  BUTTONID_LOCK=1000,
  COMBOID_TYPE,
  TXTID_LABEL,
  BUTTONID_ALR
};

enum {
  NOTES_HELP_DISABLED=0,
  ITEM_NOTES,
  TRACK_NOTES,
  ACTION_HELP // last item: no OSX support yet 
};

enum {
  REQUEST_REFRESH=0,
  REQUEST_REFRESH_EMPTY,
  NO_REFRESH
};

// Globals
SNM_NotesHelpWnd* g_pNotesHelpWnd = NULL;
SWSProjConfig<WDL_PtrList_DeleteOnDestroy<SNM_TrackNotes> > g_pTracksNotes;
SWSProjConfig<WDL_String> g_prjNotes;

//JFB member attributes?
int g_bDocked = -1, g_bLastDocked = 0; 
char g_locked = 1;
char g_lastText[MAX_HELP_LENGTH];

// action help tracking
//JFB TODO: cleanup when we'll be able to access all sections & custom ids
int g_lastActionListSel = -1;
DWORD g_lastActionListCmd = 0;
char g_lastActionListSection[64] = "";
char g_lastActionId[64] = "";
char g_lastActionDesc[128] = ""; 

MediaItem* g_mediaItemNote = NULL;
MediaTrack* g_trNote = NULL;


///////////////////////////////////////////////////////////////////////////////
// SNM_NotesHelpWnd
///////////////////////////////////////////////////////////////////////////////

SNM_NotesHelpWnd::SNM_NotesHelpWnd()
:SWS_DockWnd(IDD_SNM_NOTES_HELP, "Notes/Help", "SnMNotesHelp", 30007, SWSGetCommandID(OpenNotesHelpView))
{
	m_type = m_previousType = NOTES_HELP_DISABLED;

	// Get the action help file
	readActionHelpFilenameIniFile();
	m_internalTLChange = false;

	// GUI inits
	if (m_bShowAfterInit)
		Show(false, false);

	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
	if (GetDlgItem(m_hwnd, IDC_EDIT))
		SetWindowLongPtr(GetDlgItem(m_hwnd, IDC_EDIT), GWLP_USERDATA, 0xdeadf00b);
}

int SNM_NotesHelpWnd::GetType(){
	return m_type;
}

void SNM_NotesHelpWnd::SetType(int _type)
{
	int prev = m_previousType = m_type;
	m_type = _type;
	m_cbType.SetCurSel(_type);

	// force an initial refresh (when IDC_EDIT has the focus, re-enabling the timer 
	// isn't enough: Update() is skipped, see OnTimer() & IsActive()
	Update(); 

	if (prev == NOTES_HELP_DISABLED && prev != _type)
		SetTimer(m_hwnd, 1, 125, NULL);
}

void SNM_NotesHelpWnd::SetText(const char* _str) 
{
	if (_str)
	{
		char buf[MAX_HELP_LENGTH] = "";
		GetStringWithRN(_str, buf, MAX_HELP_LENGTH);
		strncpy(g_lastText, buf, MAX_HELP_LENGTH);
		SetDlgItemText(GetHWND(), IDC_EDIT, buf);
	}
}

void SNM_NotesHelpWnd::RefreshGUI(bool _emtpyNotes) 
{
	if (_emtpyNotes)
		SetText("");

	bool bHide = true;
	switch(GetType())
	{
		case ACTION_HELP:
			if (g_lastActionId && *g_lastActionId)
				bHide = false; // for copy/paste even if "Custom: ..."
			break;
		case ITEM_NOTES:
			if (g_mediaItemNote)
				bHide = false;
			break;
		case TRACK_NOTES:
			if (g_trNote)
				bHide = false;
			break;
		case NOTES_HELP_DISABLED:
			bHide = false;
			break;
		default:
			break;
	}
	ShowWindow(GetDlgItem(GetHWND(), IDC_EDIT), bHide ? SW_HIDE : SW_SHOW);
	m_parentVwnd.RequestRedraw(NULL); // WDL refresh
}

void SNM_NotesHelpWnd::CSurfSetTrackTitle() {
	if (m_type == TRACK_NOTES)
		RefreshGUI();
}

//JFB TODO? replace "timer-ish track sel change tracking" with this notif..
void SNM_NotesHelpWnd::CSurfSetTrackListChange() 
{
	// This is our only notification of active project tab change, so update everything
	// (we use a ScheduledJob because of possible multi-notifs)
	if (!m_internalTLChange)
	{
		SNM_NoteHlp_TLChangeSchedJob* job = new SNM_NoteHlp_TLChangeSchedJob();
		AddOrReplaceScheduledJob(job);
	}
	else
		m_internalTLChange = false;
}

void SNM_NotesHelpWnd::Update(bool _force)
{
	static bool updateReentrance;
	if (!updateReentrance)
	{
		updateReentrance = true; 

		// force refresh if needed (dock state changed, ..)
		bool saveCurrent = false; //JFB2 true;
		if (_force || g_bLastDocked != g_bDocked || m_type != m_previousType)
		{
/*JFB2			saveCurrentText(m_previousType);
			saveCurrent = false;
*/
			g_bLastDocked = g_bDocked;
			m_previousType = m_type;
			g_lastActionListSel = -1;
			*g_lastActionId = 0;
			*g_lastActionDesc = 0;
			g_lastActionListCmd = 0;
			*g_lastActionListSection = 0;
			g_mediaItemNote = NULL;
			g_trNote = NULL;
		}

		// Update
		int refreshType = NO_REFRESH;
		switch(m_type)
		{
			case ITEM_NOTES:
				refreshType = updateItemNotes(saveCurrent);
				if (refreshType == REQUEST_REFRESH_EMPTY) {
//JFB2					saveCurrentItemNotes();
					g_mediaItemNote = NULL;
				}
				// Concurent item note update ?
/*JFB kludge..
#ifdef _WIN32
				else if (refreshType == NO_REFRESH)
				{
					char name[BUFFER_SIZE] = "";
					MediaItem_Take* tk = GetActiveTake(g_mediaItemNote);
					if (tk)
					{
						char* tkName= (char*)GetSetMediaItemTakeInfo(tk, "P_NAME", NULL);
						sprintf(name, "Notes for \"%s\"", tkName);
						HWND w = SearchWindow(name);
						if (w)
							g_mediaItemNote = NULL; //will force refresh
					}
				}
#endif
*/
				break;

			case TRACK_NOTES:
				refreshType = updateTrackNotes(saveCurrent);
				if (refreshType == REQUEST_REFRESH_EMPTY) {
//JFB2					saveCurrentTrackNotes();
					g_trNote = NULL;
				}
				break;

			case ACTION_HELP:
				refreshType = updateActionHelp(saveCurrent);
				if (refreshType == REQUEST_REFRESH_EMPTY) {
//JFB2					saveCurrentHelp();
					g_lastActionListSel = -1;
					*g_lastActionId = 0;
					*g_lastActionDesc = 0;
					g_lastActionListCmd = 0;
					*g_lastActionListSection = 0;
				}
				break;

			case NOTES_HELP_DISABLED:
				KillTimer(m_hwnd, 1);
				SetText(g_prjNotes.Get()->Get());
				refreshType = REQUEST_REFRESH;
				break;

			default:
				break;
		}
		
		if (refreshType != NO_REFRESH)
			RefreshGUI(refreshType == REQUEST_REFRESH_EMPTY);
	}
	updateReentrance = false;
}

void SNM_NotesHelpWnd::saveCurrentText(int _type) 
{
	if (g_pNotesHelpWnd)
	{
		switch(_type) {
			case ITEM_NOTES: 
				m_internalTLChange = true; // item note updates lead to SetTrackListChange() CSurf notif !
				saveCurrentItemNotes(); 
				break;
			case TRACK_NOTES: saveCurrentTrackNotes(); break;
			case ACTION_HELP: saveCurrentHelp(); break;
			case NOTES_HELP_DISABLED: saveCurrentPrjNotes(); break;
			default:break;
		}
	}
}

void SNM_NotesHelpWnd::saveCurrentPrjNotes()
{
	char buf[MAX_HELP_LENGTH] = "";
	memset(buf, 0, sizeof(buf));
	GetDlgItemText(GetHWND(), IDC_EDIT, buf, MAX_HELP_LENGTH);
	if (strncmp(g_lastText, buf, MAX_HELP_LENGTH)) {
		g_prjNotes.Get()->Set(buf);
		Undo_OnStateChangeEx("Edit project notes", UNDO_STATE_MISCCFG, -1);
	}
}

void SNM_NotesHelpWnd::saveCurrentHelp()
{
	if (*g_lastActionId)
	{
		if (g_lastActionDesc && *g_lastActionDesc && _strnicmp(g_lastActionDesc, "Custom:", 7))
		{
			char buf[MAX_HELP_LENGTH] = "";
			memset(buf, 0, sizeof(buf));
			GetDlgItemText(GetHWND(), IDC_EDIT, buf, MAX_HELP_LENGTH);
			if (strncmp(g_lastText, buf, MAX_HELP_LENGTH))
				saveHelp(g_lastActionId, buf);
		}
	}
}

void SNM_NotesHelpWnd::saveCurrentItemNotes()
{
	if (g_mediaItemNote && GetMediaItem_Track(g_mediaItemNote))
	{
		char buf[MAX_HELP_LENGTH] = "";
		memset(buf, 0, sizeof(buf));
		GetDlgItemText(GetHWND(), IDC_EDIT, buf, MAX_HELP_LENGTH);
		if (strncmp(g_lastText, buf, MAX_HELP_LENGTH))
		{
			bool update = false;
			WDL_String newNotes;
			if (!*buf || GetNotesFromString(buf, &newNotes))
			{
				SNM_ChunkParserPatcher p(g_mediaItemNote);
				bool update = p.ReplaceSubChunk("NOTES", 2, 0, newNotes.Get()); // can be "", ie remove notes
				if (!update && *buf)
					update = p.InsertAfterBefore(1, newNotes.Get(), "ITEM", "IGUID", 1, 0);
			}
			// the patch has occured
			if (update)
			{
				UpdateItemInProject(g_mediaItemNote);
				UpdateTimeline();
				Undo_OnStateChangeEx("Edit item notes", UNDO_STATE_ALL, -1);
//JFB weird..				Undo_OnStateChange_Item(NULL, "Edit item notes", g_mediaItemNote);
			}
		}
	}
}

void SNM_NotesHelpWnd::saveCurrentTrackNotes()
{
	if (g_trNote && CSurf_TrackToID(g_trNote, false) >= 0)
	{
		char buf[MAX_HELP_LENGTH] = "";
		memset(buf, 0, sizeof(buf));
		GetDlgItemText(GetHWND(), IDC_EDIT, buf, MAX_HELP_LENGTH);
		if (strncmp(g_lastText, buf, MAX_HELP_LENGTH))
		{
			bool found = false;
			for (int i = 0; i < g_pTracksNotes.Get()->GetSize(); i++) 
			{
				if (g_pTracksNotes.Get()->Get(i)->m_tr == g_trNote) {
					g_pTracksNotes.Get()->Get(i)->m_notes.Set(buf);
					found = true;
					break;
				}
			}
			if (!found)
				g_pTracksNotes.Get()->Add(new SNM_TrackNotes(g_trNote, buf));
			Undo_OnStateChangeEx("Edit track notes", UNDO_STATE_MISCCFG, -1);
		}
	}
}

int SNM_NotesHelpWnd::updateActionHelp(bool _savePrevious)
{
	int refreshType = REQUEST_REFRESH_EMPTY;
	HWND hList = GetActionListBox(g_lastActionListSection, 64);
	if (hList && ListView_GetSelectedCount(hList))
	{
		LVITEM li;
		li.mask = LVIF_STATE | LVIF_PARAM;
		li.stateMask = LVIS_SELECTED;
		li.iSubItem = 0;
		for (int i = 0; i < ListView_GetItemCount(hList); i++)
		{
			li.iItem = i;
			ListView_GetItem(hList, &li);
			if (li.state == LVIS_SELECTED)
			{
				if (i != g_lastActionListSel)
				{
					g_lastActionListSel = i;
/*JFB2
					if (*g_lastActionId && _savePrevious)
						saveCurrentHelp();
*/
					int cmdId = (int)li.lParam;
					g_lastActionListCmd = cmdId; 

					//JFB TODO: cleanup when we'll be able to access all sections & custom ids
/*JFB!!!
					LVITEM li0, li1;

					li0.mask = LVIF_TEXT;
					li0.iItem = i;
					li0.iSubItem = 1;
					li0.pszText = g_lastActionDesc;
					li0.cchTextMax = 128;
					ListView_GetItem(hList, &li0);

					li1.mask = LVIF_TEXT;
					li1.iItem = i;
					li1.iSubItem = 3;
					li1.pszText = g_lastActionId;
					li1.cchTextMax = 64;
					ListView_GetItem(hList, &li1);
*/
					ListView_GetItemText(hList,i,1,g_lastActionDesc,128);
					ListView_GetItemText(hList,i,3,g_lastActionId,64);

					if (g_lastActionId && !*g_lastActionId)
						sprintf(g_lastActionId, "%d", cmdId);
					
					if (g_lastActionId && *g_lastActionId && g_lastActionDesc && *g_lastActionDesc )
					{
						if (!_strnicmp(g_lastActionDesc, "Custom:", 7))
						{
							SetText(g_lastActionId);
						}
						else
						{
							char buf[MAX_HELP_LENGTH] = "";
							loadHelp(g_lastActionId, buf, MAX_HELP_LENGTH);
							SetText(buf);
						}
						refreshType = REQUEST_REFRESH;
					}
				}
				else
					refreshType = NO_REFRESH;
				break;
			}
		}
	}
	return refreshType;
}

int SNM_NotesHelpWnd::updateItemNotes(bool _savePrevious)
{
	int refreshType = REQUEST_REFRESH_EMPTY;
	if (CountSelectedMediaItems(NULL))
	{
		MediaItem* selItem = GetSelectedMediaItem(0, 0);
		if (selItem != g_mediaItemNote)
		{
/*JFB2
			if (_savePrevious)
				saveCurrentItemNotes();
*/
			g_mediaItemNote = selItem;

			SNM_ChunkParserPatcher p(g_mediaItemNote);
			WDL_String notes;
			char buf[MAX_HELP_LENGTH] = "";
			if (p.GetSubChunk("NOTES", 2, 0, &notes))
				GetStringFromNotes(&notes, buf, MAX_HELP_LENGTH);
			SetText(buf);
			refreshType = REQUEST_REFRESH;
		} 
		else
			refreshType = NO_REFRESH;
	}
	return refreshType;
}

int SNM_NotesHelpWnd::updateTrackNotes(bool _savePrevious)
{
	int refreshType = REQUEST_REFRESH_EMPTY;
	if (CountSelectedTracksWithMaster(NULL))
	{
		MediaTrack* selTr = GetSelectedTrackWithMaster(NULL, 0);
		if (selTr != g_trNote)
		{
/*JFB2
			if (_savePrevious)
				saveCurrentTrackNotes();
*/
			g_trNote = selTr;

			WDL_String* notes = NULL;
			for (int i = 0; i < g_pTracksNotes.Get()->GetSize(); i++) {
				if (g_pTracksNotes.Get()->Get(i)->m_tr == g_trNote) {
					notes = &(g_pTracksNotes.Get()->Get(i)->m_notes);
					break;
				}
			}
			if (!notes) {
				SNM_TrackNotes* tn = new SNM_TrackNotes(g_trNote, "");
				g_pTracksNotes.Get()->Add(tn);
				notes = &(tn->m_notes);
			}
			SetText(notes->Get());
			refreshType = REQUEST_REFRESH;
		} 
		else
			refreshType = NO_REFRESH;
	}
	return refreshType;
}

void SNM_NotesHelpWnd::OnInitDlg()
{
	m_resize.init_item(IDC_EDIT, 0.0, 0.0, 1.0, 1.0);
	SetWindowLongPtr(GetDlgItem(m_hwnd, IDC_EDIT), GWLP_USERDATA, 0xdeadf00b);

	// Load prefs 
	m_type = GetPrivateProfileInt("NOTES_HELP_VIEW", "TYPE", 0, g_SNMiniFilename.Get());
	g_locked = GetPrivateProfileInt("NOTES_HELP_VIEW", "LOCK", 1, g_SNMiniFilename.Get());

	// WDL GUI init
    m_parentVwnd.SetRealParent(m_hwnd);

	m_btnLock.SetID(BUTTONID_LOCK);
	m_btnLock.SetRealParent(m_hwnd);
	m_parentVwnd.AddChild(&m_btnLock);

	m_btnAlr.SetID(BUTTONID_ALR);
	m_btnAlr.SetRealParent(m_hwnd);
	m_parentVwnd.AddChild(&m_btnAlr);

	m_cbType.SetID(COMBOID_TYPE);
	m_cbType.SetRealParent(m_hwnd);
	m_cbType.AddItem("Disable (project notes)");
	m_cbType.AddItem("Item notes");
	m_cbType.AddItem("Track notes");
#ifdef _WIN32
	m_cbType.AddItem("Action help");
#endif
	m_cbType.SetCurSel(min(m_cbType.GetCount(), m_type)); // safety for SWS beta <-> SWS official
	m_parentVwnd.AddChild(&m_cbType);

	m_txtLabel.SetID(TXTID_LABEL);
	m_txtLabel.SetRealParent(m_hwnd);
	m_parentVwnd.AddChild(&m_txtLabel);
	
	m_previousType = -1; // will force refresh
	Update();

	if (m_type != NOTES_HELP_DISABLED)
		SetTimer(m_hwnd, 1, 125, NULL);
}

void SNM_NotesHelpWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam == (IDC_EDIT | (EN_CHANGE << 16)))
		saveCurrentText(m_type); // + undos
	else if (wParam == SET_ACTION_HELP_FILE_MSG)
		SetActionHelpFilename(NULL);
	else if (HIWORD(wParam)==0)	
	{
		switch(LOWORD(wParam))
		{
			case BUTTONID_LOCK:
			{
				g_locked = !g_locked;
//*focus
				if (!g_locked)
					SetFocus(GetDlgItem(m_hwnd, IDC_EDIT));
				else
					SetFocus(GetMainHwnd());
//*/
				RefreshToolbar(NamedCommandLookup("_S&M_ACTIONHELPTGLOCK"));
			}
			break;
			case BUTTONID_ALR:
			{
				if (g_lastActionId && *g_lastActionId && g_lastActionDesc && 
					*g_lastActionDesc && _strnicmp(g_lastActionDesc, "Custom:", 7))
				{
					char cLink[512] = "";
					char sectionURL[64] = "";
					if (GetALRStartOfURL(g_lastActionListSection, sectionURL, 64))
					{					
						_snprintf(cLink, 512, "http://www.cockos.com/wiki/index.php/%s_%s", sectionURL, g_lastActionId);
						ShellExecute(m_hwnd, "open", cLink , NULL, NULL, SW_SHOWNORMAL);
					}
				}
				else
					ShellExecute(m_hwnd, "open", "http://wiki.cockos.com/wiki/index.php/Action_List_Reference" , NULL, NULL, SW_SHOWNORMAL);
			}
			break;
		}
	}
	else if (HIWORD(wParam)==CBN_SELCHANGE && LOWORD(wParam)==COMBOID_TYPE)	
	{
		SetType(m_cbType.GetCurSel());
//*focus
		if (!g_locked)
			SetFocus(GetDlgItem(m_hwnd, IDC_EDIT));
//*/
	}
	else 
		Main_OnCommand((int)wParam, (int)lParam);
}

bool SNM_NotesHelpWnd::IsActive(bool bWantEdit) { 
	return (bWantEdit || GetForegroundWindow() == m_hwnd || GetFocus() == GetDlgItem(m_hwnd, IDC_EDIT)); 
}

HMENU SNM_NotesHelpWnd::OnContextMenu(int x, int y)
{
	HMENU hMenu = CreatePopupMenu();
	AddToMenu(hMenu, "Set action help file...", SET_ACTION_HELP_FILE_MSG);
	return hMenu;
}

void SNM_NotesHelpWnd::OnDestroy() 
{
	KillTimer(m_hwnd, 1);

	// when docking/undocking for e.g. (text lost otherwise)
//JFB2	saveCurrentText(m_type);

	// save prefs
	char cType[2], cLock[2];
	sprintf(cType, "%d", m_type);
	sprintf(cLock, "%d", g_locked);
	WritePrivateProfileString("NOTES_HELP_VIEW", "TYPE", cType, g_SNMiniFilename.Get()); 
	WritePrivateProfileString("NOTES_HELP_VIEW", "LOCK", cLock, g_SNMiniFilename.Get()); 

	m_previousType = -1;
	m_cbType.Empty();
	m_parentVwnd.RemoveAllChildren(false);
	m_parentVwnd.SetRealParent(NULL);
}

// we don't check iKeyState in order to catch (almost) everything
// some key masks won't pass here though (e.g. Ctrl+Shift)
int SNM_NotesHelpWnd::OnKey(MSG* msg, int iKeyState) 
{
	// Ensure the return key is catched when docked
	if (GetDlgItem(m_hwnd, IDC_EDIT) == msg->hwnd)
	{
		if (m_bDocked && (msg->message == WM_KEYDOWN || msg->message == WM_CHAR) &&
			msg->wParam == VK_RETURN)
		{
			return (g_locked ? 1 : -1);
		}
		return (g_locked ? 1 : 0); //eat keystroke if locked
	}
	return 0; 
}

void SNM_NotesHelpWnd::OnTimer() {
	if (!IsActive()) // no update while the user edits..
		Update();
}

static void DrawControls(WDL_VWnd_Painter *_painter, RECT _r, WDL_VWnd* _parentVwnd)
{
	if (!g_pNotesHelpWnd) // SWS Can't draw before wnd initialized - why isn't this a member func??
		return;			  // JFB TODO yes, I was planing a kind of SNM_Wnd at some point..
	
	int xo=0, yo=0;
    LICE_IBitmap *bm = _painter->GetBuffer(&xo,&yo);
	if (bm)
	{
		LICE_CachedFont* font = SNM_GetThemeFont();
		int x0=_r.left+10; int y0=_r.top+5;
		int w=25, h=25; //default width/height

		// Lock button
		IconTheme* it = (IconTheme*)GetIconThemeStruct(NULL);// returns the whole icon theme (icontheme.h) and the size
		WDL_VirtualIconButton* btnVwnd = (WDL_VirtualIconButton*)_parentVwnd->GetChildByID(BUTTONID_LOCK);
		if (btnVwnd)
		{
			WDL_VirtualIconButton_SkinConfig* img=NULL;
			img = it ? &(it->toolbar_lock[!g_locked]) : NULL;
			if (img) {
				btnVwnd->SetIcon(img);
				w = img->image->getWidth() / 3;
				h = img->image->getHeight();
			}
			else {
				btnVwnd->SetTextLabel("Lock", 0, font);
				btnVwnd->SetForceBorder(true);
				w = 50;
			}

//JFB			RECT tr2={x0,y0,x0+w,y0+h};
			RECT tr2={x0, y0 - (img ? 2:-3), x0 + w, y0 - (img ? 2:1) + h};
			btnVwnd->SetPosition(&tr2);
			x0 += 5+w;
			w=25, h=25; // restore default width/height
		}

		// Dropdown
		WDL_VirtualComboBox* cbVwnd = (WDL_VirtualComboBox*)_parentVwnd->GetChildByID(COMBOID_TYPE);
		if (cbVwnd)
		{
			x0 += 5;
			RECT tr2={x0,y0+3,x0+138,y0+h-2};
			x0 = tr2.right+5;
			cbVwnd->SetPosition(&tr2);
			cbVwnd->SetFont(font);
		}

		// online help
		btnVwnd = (WDL_VirtualIconButton*)_parentVwnd->GetChildByID(BUTTONID_ALR);
		if (btnVwnd)
		{
			if (g_pNotesHelpWnd->GetType() == ACTION_HELP)
			{
				x0 += 5;
				RECT tr2={x0,y0+3,x0+30,y0+h-1};
				x0 = tr2.right+5;
				btnVwnd->SetPosition(&tr2);
				btnVwnd->SetTextLabel("ALR", 0, font);
				btnVwnd->SetForceBorder(true);
			}
			btnVwnd->SetVisible(g_pNotesHelpWnd->GetType() == ACTION_HELP);
		}

		// Label
		WDL_VirtualStaticText* txtVwnd = (WDL_VirtualStaticText*)_parentVwnd->GetChildByID(TXTID_LABEL);
		if (txtVwnd)
		{
			char str[512] = "No selection!";
			switch(g_pNotesHelpWnd->GetType())
			{
				case ACTION_HELP:
					if (g_lastActionDesc && *g_lastActionDesc && g_lastActionListSection && *g_lastActionListSection)
						_snprintf(str, 512, " [%s]  %s", g_lastActionListSection, g_lastActionDesc);
/*JFB TODO: use smthg like that when we'll be able to access all sections
						strncpy(str, kbd_getTextFromCmd(g_lastActionListCmd, NULL), 512);
*/
					break;
				case ITEM_NOTES:
					if (g_mediaItemNote)
					{
						MediaItem_Take* tk = GetActiveTake(g_mediaItemNote);
						char* tkName= tk ? (char*)GetSetMediaItemTakeInfo(tk, "P_NAME", NULL) : NULL;
						strncpy(str, tkName ? tkName : "",512);
					}
					break;
				case TRACK_NOTES:
					if (g_trNote)
					{
						int id = CSurf_TrackToID(g_trNote, false);
						if (id > 0)
							_snprintf(str, 512, " [%d]  %s", id, (char*)GetSetMediaTrackInfo(g_trNote, "P_NAME", NULL));
						else if (id == 0)
							strcpy(str, "[MASTER]");
					}
					break;
				case NOTES_HELP_DISABLED:
					//JFB would be nice to display the project name here, alas..
					*str = '\0';
					break;
				default:
					break;
			}
			txtVwnd->SetText(str);

			x0 += 5;
			RECT tr2={x0,y0,_r.right-(g_snmLogo?g_snmLogo->getWidth():0)-8-8,y0+h};
			x0 = tr2.right+5;
			txtVwnd->SetPosition(&tr2);
			txtVwnd->SetFont(font);
		}

	    _painter->PaintVirtWnd(_parentVwnd);

//		if ((_r.right - _r.left) > (x0+g_snmLogo->getWidth()))
		if (g_snmLogo && (_r.right - _r.left) > 300)
			LICE_Blit(bm,g_snmLogo,_r.right-g_snmLogo->getWidth()-8,y0+3,NULL,0.125f,LICE_BLIT_MODE_ADD|LICE_BLIT_USE_ALPHA);
	}
}

int SNM_NotesHelpWnd::OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_PAINT:
		{
			RECT r;
			GetClientRect(m_hwnd,&r);		
	        m_parentVwnd.SetPosition(&r);

			ColorTheme* ct = (ColorTheme*)GetColorThemeStruct(NULL);
			if (ct)	m_vwnd_painter.PaintBegin(m_hwnd, ct->tracklistbg_color);
			else m_vwnd_painter.PaintBegin(m_hwnd, LICE_RGBA(0,0,0,255));
			DrawControls(&m_vwnd_painter, r, &m_parentVwnd);
			m_vwnd_painter.PaintEnd();
		}
		break;

		case WM_LBUTTONDOWN:
		{
			SetFocus(g_pNotesHelpWnd->GetHWND());
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);
			if (m_parentVwnd.OnMouseDown(x, y))
				SetCapture(g_pNotesHelpWnd->GetHWND());
			else 
			{
				WDL_VWnd *w = m_parentVwnd.VirtWndFromPoint(x,y);
				if (w && w->GetID() == TXTID_LABEL && GetType() == ACTION_HELP)
					SetCapture(g_pNotesHelpWnd->GetHWND());
			}
		}
		break;

		case WM_LBUTTONUP:
			if (GetCapture()==g_pNotesHelpWnd->GetHWND()) 
			{
				int x = GET_X_LPARAM(lParam);
				int y = GET_Y_LPARAM(lParam);
				m_parentVwnd.OnMouseUp(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
				ReleaseCapture();
			}
			break;

		case WM_MOUSEMOVE:
		{
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);
//			if (GetCapture()==g_pNotesHelpWnd->GetHWND())
				m_parentVwnd.OnMouseMove(x, y);
		}
		break;
	}
	return 0;
}


///////////////////////////////////////////////////////////////////////////////

void SNM_NotesHelpWnd::loadHelp(const char* _cmdName, char* _buf, int _bufSize)
{
	if (_cmdName && *_cmdName && _buf)
	{
		memset(_buf, 0, sizeof(_buf));

		char buf[MAX_HELP_LENGTH] = "";
		int j=0, size = GetPrivateProfileSection(_cmdName,buf,_bufSize,m_actionHelpFilename.Get());
		for (int i=0; i < (size-1)  && i < _bufSize; i++) // size-1, double null terminated
		{
			if (!buf[i]) 
				_buf[j++] = '\n';
			else if (buf[i] != '|') // see saveHelp()
				_buf[j++] = buf[i];
		}
		buf[_bufSize-1] = 0; //just in case..
	}
}

void SNM_NotesHelpWnd::saveHelp(const char* _cmdName, const char* _help)
{
	if (_cmdName && *_cmdName && _help) 
	{
		char buf[MAX_HELP_LENGTH] = "";
		memset(buf, 0, sizeof(buf));

		int i=0, j=0;
		bool lastCR = false;
		while (_help[i] && i < MAX_HELP_LENGTH && j < MAX_HELP_LENGTH)
		{
			if (_help[i] == '[') {
				buf[j++] = '(';
				lastCR = false;
			}
			else if (_help[i] == ']') {
				buf[j++] = ')';
				lastCR = false;
			}
			else if (_help[i] != '\r' && _help[i] != '|')
			{
				if (_help[i] == '\n' && lastCR) {
					buf[j++] = '|'; // allows empty lines, scratched by GetPrivateProfileSection() otherwise!
					buf[j++] = '\n';
					// lastCR remains true
				}
				else {
					buf[j++] = _help[i];
					lastCR = (_help[i] == '\n');
				}
			}
//KO			lastCR = (_help[i] == '\n');
			i++;
		}
		buf[MAX_HELP_LENGTH-1] = 0; //just in case..

		WritePrivateProfileStruct(_cmdName, NULL, NULL, 0, m_actionHelpFilename.Get()); //flush section
		WritePrivateProfileSection(_cmdName, buf, m_actionHelpFilename.Get());
	}
} 

void SNM_NotesHelpWnd::readActionHelpFilenameIniFile()
{
	char defaultHelpPath[BUFFER_SIZE], buf[BUFFER_SIZE];
	_snprintf(defaultHelpPath, BUFFER_SIZE, SNM_ACTION_HELP_INI_FILE, GetResourcePath());
	GetPrivateProfileString("NOTES_HELP_VIEW", "ACTION_HELP_FILE", defaultHelpPath, buf, BUFFER_SIZE, g_SNMiniFilename.Get());
	m_actionHelpFilename.Set(buf);
}

void SNM_NotesHelpWnd::saveActionHelpFilenameIniFile() {
	WritePrivateProfileString("NOTES_HELP_VIEW", "ACTION_HELP_FILE", m_actionHelpFilename.Get(), g_SNMiniFilename.Get());
}

const char* SNM_NotesHelpWnd::getActionHelpFilename() {
	return m_actionHelpFilename.Get();
}

void SNM_NotesHelpWnd::setActionHelpFilename(const char* _filename) {
	m_actionHelpFilename.Set(_filename);
}

void SetActionHelpFilename(COMMAND_T*) 
{
	//JFB BrowseForSaveFile() always asks for overwrite: painful!
	char filename[BUFFER_SIZE] = "";
	if (g_pNotesHelpWnd && BrowseForSaveFile("Set action help file",
			g_pNotesHelpWnd->getActionHelpFilename(), "", "INI files\0*.ini\0", filename, BUFFER_SIZE))
	{
		g_pNotesHelpWnd->setActionHelpFilename(filename);
	}
}

///////////////////////////////////////////////////////////////////////////////

bool GetStringWithRN(const char* _bufSrc, char* _buf, int _bufMaxSize)
{
	if (!_buf || !_bufSrc)
		return false;

	memset(_buf, 0, sizeof(_buf));

	int i=0, j=0;
	while (_bufSrc[i] && i < _bufMaxSize && j < _bufMaxSize)
	{
		if (_bufSrc[i] == '\n') {
			_buf[j++] = '\r';
			_buf[j++] = '\n';
		}
		else if (_bufSrc[i] != '\r')
			_buf[j++] = _bufSrc[i];
		i++;
	}
	_buf[_bufMaxSize-1] = 0; //just in case..
	return true;
}

bool GetStringFromNotes(WDL_String* _notes, char* _buf, int _bufMaxSize)
{
	if (!_buf || !_notes)
		return false;

	memset(_buf, 0, sizeof(_buf));
	char* pNotes = _notes->Get();

	// find 1st '|'
	int i=0, j;	while (*pNotes && pNotes[i] != '|') i++;

	if (*pNotes) j = i+1;
	else return true;

	int bufIdx = 0;
	while (pNotes[j] && j < _bufMaxSize && bufIdx < _bufMaxSize)
	{
		if (pNotes[j] != '|') 
			_buf[bufIdx++] = pNotes[j];
		j++;
	}
	_buf[bufIdx-3] = 0; // remove last chunk's "\n>\n"
	return true;
}

bool GetNotesFromString(const char* _buf, WDL_String* _notes, const char* _startLine)
{
	if (_notes && _buf)
	{
		// Save current note
		if (!_startLine) _notes->Set("<NOTES\n|");
		else _notes->Set(_startLine);

		int j=0;
		while (_buf[j]) {
			if (_buf[j] == '\n') 
				_notes->Append("\n|");
			else if (_buf[j] != '\r') 
				_notes->AppendFormatted(2, "%c", _buf[j]); 
			j++;
		}
		_notes->Append("\n>\n");
	}
	else
		return false;
	return true;
}


///////////////////////////////////////////////////////////////////////////////

void SNM_NoteHlp_TLChangeSchedJob::Perform()
{
	if (g_pNotesHelpWnd)
		g_pNotesHelpWnd->Update(true);
}


///////////////////////////////////////////////////////////////////////////////

static bool ProcessExtensionLine(const char *line, ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	LineParser lp(false);
	if (lp.parse(line) || lp.getnumtokens() < 1)
		return false;

	// Load project notes
	if (!strcmp(lp.gettoken_str(0), "<S&M_PROJNOTES"))
	{
		WDL_String notes;
		ExtensionConfigToString(&notes, ctx);

		char buf[MAX_HELP_LENGTH] = "";
		GetStringFromNotes(&notes, buf, MAX_HELP_LENGTH);
		g_prjNotes.Get()->Set(buf);
		if (g_pNotesHelpWnd && g_pNotesHelpWnd->GetType() == NOTES_HELP_DISABLED)
			g_pNotesHelpWnd->Update();

		return true;
	}
	// Load track notes
	else if (!strcmp(lp.gettoken_str(0), "<S&M_TRACKNOTES"))
	{
		GUID g;		
		stringToGuid(lp.gettoken_str(1), &g);
		MediaTrack* tr = GuidToTrack(&g);
		if (tr)
		{
			WDL_String notes;
			ExtensionConfigToString(&notes, ctx);

			char buf[MAX_HELP_LENGTH] = "";
			if (GetStringFromNotes(&notes, buf, MAX_HELP_LENGTH))
				g_pTracksNotes.Get()->Add(new SNM_TrackNotes(tr, buf));

			return true;
		}
	}
	return false;
}

static void SaveExtensionConfig(ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	// first delete unused tracks
	for (int i = 0; i < g_pTracksNotes.Get()->GetSize(); i++)
		if (CSurf_TrackToID(g_pTracksNotes.Get()->Get(i)->m_tr, false) < 0)
			g_pTracksNotes.Get()->Delete(i--, true);

	char startLine[4096], strId[128];
	WDL_String formatedNotes;

	// Save project notes
	if (g_prjNotes.Get()->GetLength())
	{
		strcpy(startLine, "<S&M_PROJNOTES\n|");
		if (GetNotesFromString(g_prjNotes.Get()->Get(), &formatedNotes, startLine))
			StringToExtensionConfig(formatedNotes.Get(), ctx);
	}

	// Save track notes
	for (int i = 0; i < g_pTracksNotes.Get()->GetSize(); i++)
	{
		if (g_pTracksNotes.Get()->Get(i)->m_notes.GetLength())
		{
			GUID g;
			if (CSurf_TrackToID(g_pTracksNotes.Get()->Get(i)->m_tr, false))
				g = *(GUID*)GetSetMediaTrackInfo(g_pTracksNotes.Get()->Get(i)->m_tr, "GUID", NULL);
			else
				g = GUID_NULL;
			guidToString(&g, strId);
			sprintf(startLine, "<S&M_TRACKNOTES %s\n|", strId);

			if (GetNotesFromString(g_pTracksNotes.Get()->Get(i)->m_notes.Get(), &formatedNotes, startLine))
				StringToExtensionConfig(formatedNotes.Get(), ctx);
		}
	}
}

static void BeginLoadProjectState(bool isUndo, struct project_config_extension_t *reg)
{
	g_prjNotes.Cleanup();
	g_prjNotes.Get()->Set("");
	g_pTracksNotes.Cleanup();
	g_pTracksNotes.Get()->Empty(true);
}

static project_config_extension_t g_projectconfig = {
	ProcessExtensionLine, SaveExtensionConfig, BeginLoadProjectState, NULL
};

static void menuhook(const char* menustr, HMENU hMenu, int flag)
{
	if (!strcmp(menustr, "Main view") && !flag)
	{
		int cmd = NamedCommandLookup("_S&M_SHOWNOTESHELP");
		if (cmd > 0)
			AddToMenu(hMenu, "S&&M Notes/Help", cmd);
	}
}

int NotesHelpViewInit() {
	g_pNotesHelpWnd = new SNM_NotesHelpWnd();
	if (!g_pNotesHelpWnd || 
		!plugin_register("hookcustommenu", (void*)menuhook) ||
		!plugin_register("projectconfig",&g_projectconfig))
		return 0;
	return 1;
}

void NotesHelpViewExit() {
	if (g_pNotesHelpWnd)
		g_pNotesHelpWnd->saveActionHelpFilenameIniFile();
	delete g_pNotesHelpWnd;
	g_pNotesHelpWnd = NULL;
}

void OpenNotesHelpView(COMMAND_T* _ct) {
	if (g_pNotesHelpWnd) {
		g_pNotesHelpWnd->Show(g_pNotesHelpWnd->GetType() == (int)_ct->user /* i.e toggle */, true);
		g_pNotesHelpWnd->SetType((int)_ct->user);
		if (!g_locked)
			SetFocus(GetDlgItem(g_pNotesHelpWnd->GetHWND(), IDC_EDIT));
	}
}

bool IsNotesHelpViewDisplayed(COMMAND_T* _ct) {
	return (g_pNotesHelpWnd && g_pNotesHelpWnd->IsValidWindow());
}

void SwitchNotesHelpType(COMMAND_T* _ct){
	g_pNotesHelpWnd->SetType((int)_ct->user);
	if (!g_locked)
		SetFocus(GetDlgItem(g_pNotesHelpWnd->GetHWND(), IDC_EDIT));
}

void ToggleNotesHelpLock(COMMAND_T*) {

	g_locked = !g_locked;
	if (g_pNotesHelpWnd)
	{
		g_pNotesHelpWnd->RefreshGUI();
		if (!g_locked)
			SetFocus(GetDlgItem(g_pNotesHelpWnd->GetHWND(), IDC_EDIT));
	}
}

bool IsNotesHelpLocked(COMMAND_T*) {
	return (g_locked == 1 ? true : false); // get rid of warning C4800
}