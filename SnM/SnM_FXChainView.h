/******************************************************************************
/ SnM_FXChainView.h
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

class SNM_FXChainView : public SWS_ListView
{
public:
	SNM_FXChainView(HWND hwndList, HWND hwndEdit);

protected:
	void GetItemText(LPARAM item, int iCol, char* str, int iStrMax);
	void SetItemText(LPARAM item, int iCol, const char* str);
	void OnItemDblClk(LPARAM item, int iCol);
	void GetItemList(WDL_TypedBuf<LPARAM>* pBuf);
	void OnBeginDrag(LPARAM item);
};

class SNM_FXChainWnd : public SWS_DockWnd
{
public:
	SNM_FXChainWnd();
	void Update();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	void SelectBySlot(int _slot);

	WDL_String m_filter;

protected:
	void OnInitDlg();
	HMENU OnContextMenu(int x, int y);
	void OnDestroy();
	int OnKey(MSG* msg, int iKeyState);
	void OnDroppedFiles(HDROP h);
	int OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);

	void AddSlot(bool _update);
	void InsertAtSelectedSlot(bool _update);
	void DeleteSelectedSlot(bool _update);

	// WDL UI
	WDL_VWnd_Painter m_vwnd_painter;
	WDL_VWnd m_parentVwnd; // owns all children windows
	SNM_VirtualComboBox m_cbDblClickType;
	SNM_VirtualComboBox m_cbDblClickTo;
};

extern SNM_FXChainWnd* g_pFXChainsWnd;

