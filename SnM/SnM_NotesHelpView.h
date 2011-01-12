/******************************************************************************
/ SnM_NotesHelpView.h
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


#pragma once

bool GetStringWithRN(const char* _bufSrc, char* _buf, int _bufMaxSize);
bool GetStringFromNotes(WDL_String* _notes, char* _buf, int _bufMaxSize);
bool GetNotesFromString(const char* _buf, WDL_String* _notes, const char* _startLine = NULL);

class SNM_NotesHelpWnd : public SWS_DockWnd
{
public:
	SNM_NotesHelpWnd();
	int GetType();
	void SetType(int _type);
	void SetText(const char* _str);
	void RefreshGUI(bool _emtpyNotes = false);
	void CSurfSetTrackTitle();
	void CSurfSetTrackListChange();
	void Update(bool _force=false);
	void OnCommand(WPARAM wParam, LPARAM lParam);
	bool IsActive(bool bWantEdit = false);

	void saveCurrentText(int _type);
	void saveCurrentPrjNotes();
	void saveCurrentHelp();
	void saveCurrentItemNotes();
	void saveCurrentTrackNotes();

	const char* getActionHelpFilename();
	void setActionHelpFilename(const char* _filename);
	void readActionHelpFilenameIniFile();
	void saveActionHelpFilenameIniFile();

protected:
	void OnInitDlg();
	HMENU OnContextMenu(int x, int y);
	void OnDestroy();
	int OnKey(MSG* msg, int iKeyState);
	void OnTimer();
	int OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);

	int updateItemNotes(bool _savePrevious = true);
	int updateTrackNotes(bool _savePrevious = true);
	int updateActionHelp(bool _savePrevious = true);

	void loadHelp(const char* _cmdName, char* _buf, int _bufSize);
	void saveHelp(const char* _cmdName, const char* _help);

	// WDL UI
	WDL_VWnd_Painter m_vwnd_painter;
	WDL_VWnd m_parentVwnd; // owns all children windows
	WDL_VirtualComboBox m_cbType;
	WDL_VirtualIconButton m_btnLock;
	WDL_VirtualIconButton m_btnAlr;
	WDL_VirtualStaticText m_txtLabel;

	WDL_String m_actionHelpFilename;
	int m_type, m_previousType;
	bool m_internalTLChange;
};


class SNM_NoteHlp_TLChangeSchedJob : public SNM_ScheduledJob
{
public:
	SNM_NoteHlp_TLChangeSchedJob() : SNM_ScheduledJob(SNM_SCHEDJOB_NOTEHLP_TLCHANGE, 150) {}
	void Perform();
};
