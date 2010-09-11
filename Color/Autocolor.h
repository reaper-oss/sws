/******************************************************************************
/ Autocolor.h
/
/ Copyright (c) 2010 Tim Payne (SWS)
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

class SWS_AutoColorItem
{
public:
	SWS_AutoColorItem(const char* str, int col, const char* icon):m_str(str),m_col(col),m_icon(icon) {}
	WDL_String m_str;
	int m_col;
	WDL_String m_icon;
};

class SWS_AutoColorTrack
{
public:
	SWS_AutoColorTrack(MediaTrack* tr, int col):m_pTr(tr), m_col(col), m_bColored(true) {}
	MediaTrack* m_pTr;
	int m_col;
	bool m_bColored;
};

class SWS_AutoColorView : public SWS_ListView
{
public:
	SWS_AutoColorView(HWND hwndList, HWND hwndEdit);

protected:
	void SetItemText(LPARAM item, int iCol, const char* str);
	void GetItemText(LPARAM item, int iCol, char* str, int iStrMax);
	void GetItemList(WDL_TypedBuf<LPARAM>* pBuf);
	void OnItemDblClk(LPARAM item, int iCol);
	void OnItemSelChanged(LPARAM item, int iState);
};

class SWS_AutoColorWnd : public SWS_DockWnd
{
public:
	SWS_AutoColorWnd();
	void Update();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	
protected:
	void OnInitDlg();
#ifndef _WIN32
	bool m_bSettingColor;
	void OnTimer();
	void OnDestroy();
#endif
	int OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);
	HMENU OnContextMenu(int x, int y);
	int OnKey(MSG* msg, int iKeyState);
};

int AutoColorInit();
void AutoColorExit();
void OpenAutoColor(COMMAND_T* = NULL);
void AutoColorRun(bool bForce);
