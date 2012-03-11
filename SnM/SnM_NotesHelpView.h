/******************************************************************************
/ SnM_NotesHelpView.h
/
/ Copyright (c) 2010-2011 Tim Payne (SWS), Jeffos
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

#ifndef _SNM_NOTESVIEW_H_
#define _SNM_NOTESVIEW_H_


enum {
  SNM_NOTES_PROJECT=0,
  SNM_NOTES_ITEM,
  SNM_NOTES_TRACK,
#ifdef _SNM_MARKER_REGION_NAME
  SNM_NOTES_REGION_NAME,
#endif
  SNM_NOTES_REGION_SUBTITLES,
  SNM_NOTES_ACTION_HELP // must remain the last item: no OSX support yet 
};


class NoteHelp_UpdateJob : public SNM_ScheduledJob {
public:
	NoteHelp_UpdateJob() : SNM_ScheduledJob(SNM_SCHEDJOB_NOTEHLP_TLCHANGE, 150) {}
	void Perform();
};

class NoteHelp_MarkerRegionSubscriber : public SNM_MarkerRegionSubscriber {
public:
	NoteHelp_MarkerRegionSubscriber() : SNM_MarkerRegionSubscriber() {}
	void NotifyMarkerRegionUpdate(int _updateFlags);
};

class SNM_NotesHelpWnd : public SWS_DockWnd
{
public:
	SNM_NotesHelpWnd();
	~SNM_NotesHelpWnd();

	void SetType(int _type);
	void SetText(const char* _str);
	void RefreshGUI(bool _emtpyNotes = false);
	void CSurfSetTrackTitle();
	void CSurfSetTrackListChange();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	void ToggleLock();

	void saveCurrentText(int _type);
	void saveCurrentPrjNotes();
	void saveCurrentHelp();
	void saveCurrentItemNotes();
	void saveCurrentTrackNotes();
	void saveCurrentMkrRgnNameOrNotes(bool _name);

	void Update(bool _force = false);
	int updateActionHelp();
	int updateItemNotes();
	int updateTrackNotes();
	int updateMkrRgnNameOrNotes(bool _name);

protected:
	void OnInitDlg();
	void OnDestroy();
/*JFB r376	
	bool IsActive(bool bWantEdit = false);
*/
	HMENU OnContextMenu(int x, int y, bool* wantDefaultItems);
	int OnKey(MSG* msg, int iKeyState);
	void OnTimer(WPARAM wParam=0);
	void OnResize();
	void DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight = NULL);
	bool GetToolTipString(int _xpos, int _ypos, char* _bufOut, int _bufOutSz);
	HBRUSH OnColorEdit(HWND _hwnd, HDC _hdc);

	WDL_VirtualComboBox m_cbType;
	WDL_VirtualIconButton m_btnLock;
	SNM_ToolbarButton m_btnAlr, m_btnActionList, m_btnImportSub, m_btnExportSub;
	WDL_VirtualStaticText m_txtLabel;

	bool m_internalTLChange;
	NoteHelp_MarkerRegionSubscriber m_mkrRgnSubscriber;
};


void loadHelp(const char* _cmdName, char* _buf, int _bufSize);
void saveHelp(const char* _cmdName, const char* _help);
bool GetStringFromNotesChunk(WDL_FastString* _notes, char* _buf, int _bufMaxSize);
bool GetNotesChunkFromString(const char* _buf, WDL_FastString* _notes, const char* _startLine = NULL);

extern SWSProjConfig<WDL_PtrList_DeleteOnDestroy<SNM_TrackNotes> > g_pTrackNotes;

void SetActionHelpFilename(COMMAND_T*);
int NotesHelpViewInit();
void NotesHelpViewExit();
void ImportSubTitleFile(COMMAND_T*);
void ExportSubTitleFile(COMMAND_T*);
void OpenNotesHelpView(COMMAND_T*);
bool IsNotesHelpViewDisplayed(COMMAND_T*);
void ToggleNotesHelpLock(COMMAND_T*);
bool IsNotesHelpLocked(COMMAND_T*);

#endif
