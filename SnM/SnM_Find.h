/******************************************************************************
/ SnM_Find.h
/
/ Copyright (c) 2010 and later Jeffos
/
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

//#pragma once

#ifndef _SNM_FIND_H_
#define _SNM_FIND_H_

#include "SnM_VWnd.h"


class FindWnd : public SWS_DockWnd
{
public:
	FindWnd();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	void GetMinSize(int* _w, int* _h) { *_w=297; *_h=100; }
	bool Find(int _mode);
	MediaItem* FindPrevNextItem(int _dir, MediaItem* _item);
	bool FindMediaItem(int _dir, bool _allTakes, bool (*jobTake)(MediaItem_Take*,const char*), bool (*jobItem)(MediaItem*,const char*) = NULL);
	bool FindTrack(int _dir, bool (*job)(MediaTrack*,const char*));
	bool FindMarkerRegion(int _dir);
	void UpdateNotFoundMsg(bool _found);
protected:
	void OnInitDlg();
	void OnDestroy();
	int OnKey(MSG* msg, int iKeyState) ;
	void DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight = NULL);

	WDL_VirtualComboBox m_cbType;
	WDL_VirtualIconButton m_btnEnableZommScroll;
	WDL_VirtualStaticText m_txtScope, m_txtResult;
	SNM_ToolbarButton m_btnFind, m_btnPrev, m_btnNext;

	int m_type;
	bool m_zoomSrollItems;
};

int FindInit();
void FindExit();
void OpenFind(COMMAND_T*);
int IsFindDisplayed(COMMAND_T*);
void FindNextPrev(COMMAND_T*);

#endif
