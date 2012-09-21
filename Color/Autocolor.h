/******************************************************************************
/ Autocolor.h
/
/ Copyright (c) 2010-2012 Tim Payne (SWS), Jeffos
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

class SWS_RuleItem
{
public:
    // @param type
    //     The type of the item that you want to change the color
    //     Options Track, Region or Marker
    // @param filter
    //     The string that is used to filter the different tracks, regions or markers
    // @param color
    //     The color of the item
    // @param icon
    //     The icon used for this item (tracks only)
	SWS_RuleItem(const char* type, const char* filter, int _color, const char* icon)
        :
    m_str_type(type),
    m_str_filter(filter),
    m_color(_color),
    m_icon(icon) 
    {}
	
    WDL_String m_str_type;
    WDL_String m_str_filter;
	int m_color;
	WDL_String m_icon;
};

class SWS_RuleTrack
{
public:
	SWS_RuleTrack(MediaTrack* tr)
        :
    m_pTr(tr), 
    m_col(0), 
    m_bColored(false), 
    m_bIconed(false) 
    {}
	
    MediaTrack* m_pTr;
	int m_col;
	bool m_bColored;
	WDL_String m_icon;
	bool m_bIconed;
};

class SWS_AutoColorView : public SWS_ListView
{
public:
	SWS_AutoColorView(HWND hwndList, HWND hwndEdit);
	void OnDrag();

protected:
	void SetItemText(SWS_ListItem* item, int iCol, const char* str);
	void GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax);
	void GetItemList(SWS_ListItemList* pList);
	void OnItemDblClk(SWS_ListItem* item, int iCol);
	void OnItemSelChanged(SWS_ListItem* item, int iState);
	void OnBeginDrag(SWS_ListItem* item);
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
	void OnTimer(WPARAM wParam=0);
	void OnDestroy();
#endif
	INT_PTR OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);
	HMENU OnContextMenu(int x, int y, bool* wantDefaultItems);
	int OnKey(MSG* msg, int iKeyState);
	SWS_AutoColorView* m_pView;
};

int AutoColorInit();
void AutoColorExit();
void OpenAutoColor(COMMAND_T* = NULL);
void AutoColorRun(bool bForce);
