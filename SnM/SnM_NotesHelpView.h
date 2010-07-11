/******************************************************************************
/ SnM_NotesHelpView.h
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


#pragma once

#define MAX_HELP_LENGTH 0xFFFF // i.e. limitation of WritePrivateProfileSection 
#define SAVEWINDOW_POS_KEY			"S&M - Notes/help Save Window Position"
#define SET_ACTION_HELP_FILE_MSG	0x10001


class SNM_NotesHelpWnd : public SWS_DockWnd
{
public:
	SNM_NotesHelpWnd();
	void Update();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	bool IsActive(bool bWantEdit = false);
	char* getActionHelpFilename();
	void setActionHelpFilename(const char* _filename);
	void readActionHelpFilenameIniFile(char* _buf, int _bufSize);
	void saveActionHelpFilenameIniFile();

protected:
	void OnInitDlg();
	HMENU OnContextMenu(int x, int y);
	void OnDestroy();
	int OnKey(MSG* msg, int iKeyState);
	void OnTimer();
	int OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);
	bool updateItemNotes();
	bool updateActionHelp();
	void loadHelp(char* _cmdName, char* _buf, int _bufSize);
	void saveHelp(char* _cmdName, const char* _help);

	char m_actionHelpFilename[BUFFER_SIZE];
};


