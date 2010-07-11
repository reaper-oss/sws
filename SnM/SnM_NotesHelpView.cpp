/******************************************************************************
/ SnM_NotesHelpView.cpp
/
/ Copyright (c) 2009-2010 Tim Payne (SWS), JF Bédague 
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

//JFB TODO:
// - concurent item note updates
// - item images to test
// - action_help_t (even if not used yet)

#include "stdafx.h"
#include "SnM_Actions.h"
#include "SNM_NotesHelpView.h"


// Globals
SNM_NotesHelpWnd* g_pNotesHelpWnd = NULL;


///////////////////////////////////////////////////////////////////////////////
// SNM_NotesHelpWnd
///////////////////////////////////////////////////////////////////////////////

SNM_NotesHelpWnd::SNM_NotesHelpWnd()
:SWS_DockWnd(IDD_SNM_NOTES_HELP, "Notes/help", 30007, SWSGetCommandID(OpenNotesHelpView))
{
	// Get the action help file
	readActionHelpFilenameIniFile(m_actionHelpFilename, BUFFER_SIZE);

	// GUI inits
	if (m_bShowAfterInit)
		Show(false, false);

	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
	if (GetDlgItem(m_hwnd, IDC_EDIT))
		SetWindowLongPtr(GetDlgItem(m_hwnd, IDC_EDIT), GWLP_USERDATA, 0xdeadf00b);
}

// action help tracking
int g_lastSel = -1;
char g_cmdName[BUFFER_SIZE] = "";

// item notes tracking
MediaItem* g_mediaItemNote = NULL;

int g_bDocked = -1, g_bLastDocked = 0; 
bool g_updateReentrance = false;
void SNM_NotesHelpWnd::Update()
{
	if (!g_updateReentrance)
	{
		g_updateReentrance = true; //JFB TODO: quick & dirty prototype..

		// force refresh if needed (dock state changed, ..)
		if (g_bLastDocked != g_bDocked)
		{
			g_bLastDocked = g_bDocked;
			g_lastSel = -1;
			*g_cmdName = 0;
			g_mediaItemNote = NULL;
		}

		// Update
		if (updateActionHelp())	{
			g_mediaItemNote = NULL;
		}
		else if (updateItemNotes())	{
			g_lastSel = -1;
			*g_cmdName = 0;
		}
		else {
			//JFB would be great to see *project* notes here, alas..
		}

	}
	g_updateReentrance = false;
}

bool SNM_NotesHelpWnd::updateActionHelp()
{
	HWND actionsWnd = NULL;
	if (GetMainHwnd())
	{
		HWND w = GetWindow(GetMainHwnd(), GW_HWNDFIRST);
		while (w)
		{ 
			if (IsWindowVisible(w) && GetWindow(w, GW_OWNER) == GetMainHwnd())
			{
				char name[BUFFER_SIZE] = "";
				int iLenName = GetWindowText(w, name, BUFFER_SIZE);
				if (!strcmp(name, "Actions")) {
					actionsWnd = w;
					break;
				}
			}
			w = GetWindow(w, GW_HWNDNEXT);
		}
	}

	HWND h_list = (actionsWnd ? GetDlgItem(actionsWnd, 0x52B) : NULL);
	if (h_list && (GetFocus() == h_list))
	{
		//TODO: check displayed columns
		if (ListView_GetSelectedCount(h_list))
		{
			LVITEM li;
			li.mask = LVIF_STATE | LVIF_PARAM;
			li.stateMask = LVIS_SELECTED;
			li.iSubItem = 0;
			bool done = false; // done once in case of multi-selection
			for (int i = 0; !done && i < ListView_GetItemCount(h_list); i++)
			{
				li.iItem = i;
				ListView_GetItem(h_list, &li);
				if (li.state == LVIS_SELECTED)
				{
					if (i != g_lastSel)
					{
						g_lastSel = i;
						
						if (*g_cmdName)
						{
							char buf[MAX_HELP_LENGTH] = "";//JFB
							memset(buf, 0, sizeof(buf));
							GetDlgItemText(GetHWND(), IDC_EDIT, buf, MAX_HELP_LENGTH);
							if (*buf)
								saveHelp(g_cmdName, buf);
						}

						int cmdId = (int)li.lParam;
						LVITEM li1;
						li1.mask = LVIF_TEXT;
						li1.iItem = i;
						li1.iSubItem = 3;
						li1.pszText = g_cmdName;
						li1.cchTextMax = 256;
						ListView_GetItem(h_list, &li1);

						if (!*g_cmdName)
							sprintf(g_cmdName, "%d", cmdId);
							
						if (*g_cmdName)
						{
							char buf[MAX_HELP_LENGTH] = "";//JFB
							loadHelp(g_cmdName, buf, MAX_HELP_LENGTH);
							SetDlgItemText(GetHWND(), IDC_EDIT, buf);
						}

						return true;
					}
					// The same action still selected
					else
						return true;
				}
			}
		}
	}
	return false;
}

bool SNM_NotesHelpWnd::updateItemNotes()
{
	bool update = false;
	if (CountSelectedMediaItems(0) == 1)
	{
		MediaItem* selItem = GetSelectedMediaItem(0, 0);
		if (selItem != g_mediaItemNote)
		{
			char buf[MAX_HELP_LENGTH] = "";//JFB
			memset(buf, 0, sizeof(buf));
			GetDlgItemText(GetHWND(), IDC_EDIT, buf, MAX_HELP_LENGTH);
			if (*buf && g_mediaItemNote)
			{
				// Save current note
				SNM_ChunkParserPatcher p(g_mediaItemNote);
				p.RemoveIds();

				WDL_String newNotes("<NOTES\n|");
				int j=0;
				while (buf[j])
				{
					if (buf[j] == '\n') newNotes.Append("\n|");
					else newNotes.AppendFormatted(1, "%c", buf[j]);
					j++;
				}
				newNotes.Append("\n>");
				if (!p.ReplaceSubChunk("NOTES", 2, 0, newNotes.Get())) // JFB undo!!
					p.InsertAfter(newNotes.Get(), "ITEM", "IGUID", 1, 0); 
			}

			g_mediaItemNote = selItem;
			update = true;

			SNM_ChunkParserPatcher p(g_mediaItemNote);
			WDL_String notes;
			if (p.GetSubChunk("NOTES", 2, 0, &notes))
			{
				char buf[MAX_HELP_LENGTH] = "";//JFB
				memset(buf, 0, sizeof(buf));
				char* pNotes = (char*)(notes.Get()+8); // +8 to zaop "<NOTES\n|"
				int j=0, bufIdx = 0;
				bool lastWasReturn = false;
				while (pNotes[j] && j < MAX_HELP_LENGTH && bufIdx < MAX_HELP_LENGTH)
				{
					if (pNotes[j] == '\n') {
						buf[bufIdx++] = '\r';
						buf[bufIdx++] = '\n';
						lastWasReturn = true;
					}
					else if (pNotes[j] == '|' && lastWasReturn)	{
						lastWasReturn = false;
					}
					else {
						buf[bufIdx++] = pNotes[j];
						lastWasReturn = false;
					}
					j++;
				}
				buf[bufIdx-3] = 0; // remove last chunk's ">" (& indirectly ends with '\n')
				buf[MAX_HELP_LENGTH-1] = 0; // ensure safe truncation
				SetDlgItemText(GetHWND(), IDC_EDIT, buf);
			}
			else
				SetDlgItemText(GetHWND(), IDC_EDIT, "");
		}
	}
	return update;
}

void SNM_NotesHelpWnd::OnInitDlg()
{
	m_resize.init_item(IDC_EDIT, 0.0, 0.0, 1.0, 1.0);
	SetWindowLongPtr(GetDlgItem(m_hwnd, IDC_EDIT), GWLP_USERDATA, 0xdeadf00b);
	Update();
	SetTimer(m_hwnd, 1, 500, NULL);
}

void SNM_NotesHelpWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam == SET_ACTION_HELP_FILE_MSG)
		SetActionHelpFilename(NULL);
	else 
		Main_OnCommand((int)wParam, (int)lParam);
}

bool SNM_NotesHelpWnd::IsActive(bool bWantEdit) { 
	return (bWantEdit || GetForegroundWindow() == m_hwnd || GetFocus() == GetDlgItem(m_hwnd, IDC_EDIT)); 
}

HMENU SNM_NotesHelpWnd::OnContextMenu(int x, int y)
{
	HMENU hMenu = CreatePopupMenu();
/* avoid some strange popups
	LPARAM item = m_pLists.Get(0)->GetHitItem(x, y, NULL);
	if (item)
*/
	{
		AddToMenu(hMenu, "Set action help file...", SET_ACTION_HELP_FILE_MSG);
	}	
	return hMenu;
}

void SNM_NotesHelpWnd::OnDestroy() {
	KillTimer(m_hwnd, 1);
}

int SNM_NotesHelpWnd::OnKey(MSG* msg, int iKeyState) 
{
	// Ensure the return key is catched when docked
	if (m_bDocked && (msg->message == WM_KEYDOWN || msg->message == WM_CHAR) &&
		msg->wParam == VK_RETURN && GetDlgItem(m_hwnd, IDC_EDIT) == msg->hwnd)
	{
		return -1;
	}
	return 0; 
}

void SNM_NotesHelpWnd::OnTimer()
{
	if (!IsActive())
		Update();
}

int SNM_NotesHelpWnd::OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	return 0;
}

void SNM_NotesHelpWnd::loadHelp(char* _cmdName, char* _buf, int _bufSize)
{
	if (_cmdName && _buf)
	{
		char buf[MAX_HELP_LENGTH] = "";//JFB
//JFB not part of swell		
		int size = GetPrivateProfileSection(_cmdName,buf,MAX_HELP_LENGTH,m_actionHelpFilename);
//KO		int size = GetPrivateProfileStruct(_cmdName, NULL, buf, MAX_HELP_LENGTH, m_actionHelpFilename);
		if (size)
		{
			int j = 0;
			for (int i=0; i<(size-1) && j < _bufSize && i < MAX_HELP_LENGTH; i++) //JFB
			{
				if (!buf[i]) {
					_buf[j++] = '\r';
					_buf[j] = '\n';
				}
				else _buf[j] = buf[i];
				j++;
			}
		}
		_buf[_bufSize-1] = 0; // ensure safe truncation..
	}
}

void SNM_NotesHelpWnd::saveHelp(char* _cmdName, const char* _help)
{
	if (_cmdName && _help)
	{
		WritePrivateProfileStruct(_cmdName, NULL, NULL, 0, m_actionHelpFilename); //flush section
//JFB not part of swell		
		WritePrivateProfileSection(_cmdName, _help, m_actionHelpFilename);
//KO		WritePrivateProfileStruct(_cmdName, NULL, (LPVOID)_help, strlen(_help), m_actionHelpFilename);
	}
}

void SNM_NotesHelpWnd::readActionHelpFilenameIniFile(char* _buf, int _bufSize)
{
	char iniFilePath[BUFFER_SIZE] = "";
	sprintf(iniFilePath, SNM_FORMATED_INI_FILE, GetExePath());
	char defaultHelpPath[BUFFER_SIZE] = "";
	sprintf(defaultHelpPath, SNM_ACTION_HELP_INI_FILE, GetExePath());
	GetPrivateProfileString("ACTION_HELP", "PATH", defaultHelpPath, _buf, _bufSize, iniFilePath);
}

void SNM_NotesHelpWnd::saveActionHelpFilenameIniFile()
{
	char iniFilePath[BUFFER_SIZE] = "";
	sprintf(iniFilePath, SNM_FORMATED_INI_FILE, GetExePath());
	WritePrivateProfileString("ACTION_HELP", "PATH", m_actionHelpFilename, iniFilePath);
}

char* SNM_NotesHelpWnd::getActionHelpFilename() {
	return m_actionHelpFilename;
}

void SNM_NotesHelpWnd::setActionHelpFilename(const char* _filename) {
	strncpy(m_actionHelpFilename, _filename, BUFFER_SIZE);
}


///////////////////////////////////////////////////////////////////////////////

int NotesHelpViewInit()
{
	g_pNotesHelpWnd = new SNM_NotesHelpWnd();
	return 1;
}

void NotesHelpViewExit() {
	if (g_pNotesHelpWnd)
	{
		g_pNotesHelpWnd->saveActionHelpFilenameIniFile();
		delete g_pNotesHelpWnd;
	}
}

void OpenNotesHelpView(COMMAND_T*) {
	if (g_pNotesHelpWnd)
		g_pNotesHelpWnd->Show(true, true);
}

void SetActionHelpFilename(COMMAND_T*) 
{
	//JFB OVERWRITE...
	char filename[BUFFER_SIZE];
	if (g_pNotesHelpWnd && BrowseForSaveFile("Set action help file",
			g_pNotesHelpWnd->getActionHelpFilename(), "", 
			"INI files\0*.ini\0", filename, BUFFER_SIZE))
	{
		g_pNotesHelpWnd->setActionHelpFilename(filename);
	}
}
