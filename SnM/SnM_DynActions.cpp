/******************************************************************************
 / SnM_DynActions.cpp
 /
 / Copyright (c) 2016 and later Jeffos
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


#include "stdafx.h"
#include "SnM.h"
#include "SnM_Dlg.h"
#include "SnM_DynActions.h"
#include "SnM_Util.h"
#include "../Prompt.h"
#include "../reaper/localize.h"

#define DYNACTIONWND_POS_KEY	"DynActioWndPos"

// !WANT_LOCALIZE_STRINGS_BEGIN:sws_DLG_188
static SWS_LVColumn s_cols[] = { { 544, 0, "Action" }, { 60, 1, "Amount" }, { 60, 0, "Max" }, };
// !WANT_LOCALIZE_STRINGS_END

DynActionListView::DynActionListView(HWND hwndList, HWND hwndEdit)
	: SWS_ListView(hwndList, hwndEdit, 3, s_cols, "DynActionViewState", false, "sws_DLG_188")
{
}

void DynActionListView::GetItemText(SWS_ListItem* _item, int _col, char* _str, int _strMax)
{
	_str[0] = '\0';

	DYN_COMMAND_T* item = (DYN_COMMAND_T*)_item;  
	if (!item) return;
  
	switch(_col)
	{
    case 0:
      lstrcpyn(_str, item->desc, _strMax);
      ReplaceWithChar(_str, "%d", 'n');
      break;
    case 1:
      _snprintfSafe(_str, _strMax, "%d", item->count);
			break;
    case 2:
      _snprintfSafe(_str, _strMax, "%d", item->max);
			break;
	}
}

void DynActionListView::SetItemText(SWS_ListItem* _item, int _col, const char* _str)
{
	DYN_COMMAND_T* item = (DYN_COMMAND_T*)_item;  
  if (item && _col==1 && _str && (!strcmp(_str, "0") || atoi(_str)))
  {
    int n = BOUNDED(atoi(_str), item->min, item->max);
    if (n != item->count)
    {
      item->count = n;
      EnableWindow(GetDlgItem(GetParent(m_hwndList), IDOK), true);
    }
  }
}

void DynActionListView::GetItemList(SWS_ListItemList* pList)
{
  for (int i=0; ; i++)
  {
    if (DYN_COMMAND_T* da = GetDynamicAction(i))
    {
      if (da->desc == LAST_COMMAND) break;
      pList->Add((SWS_ListItem*)da);
    }
  }
}

INT_PTR WINAPI DynActionWndProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static WDL_WndSizer resize;
	static DynActionListView* lv = NULL;

  if (SWS_THEMING)
	{
    if (INT_PTR r = SNM_HookThemeColorsMessage(hwndDlg, uMsg, wParam, lParam))
		  return r;
  
    if (lv && ListView_HookThemeColorsMessage(hwndDlg, uMsg, lParam, lv->GetOldColors(), GetWindowLong(lv->GetHWND(),GWL_ID), 0, lv->HideGridLines() ? 0 : lv->GetColumnCount()))
      return 1;
  }

	switch (uMsg)
	{
		case WM_INITDIALOG:
			resize.init(hwndDlg);
			resize.init_item(IDC_LIST);
			resize.init_item(IDCANCEL, 1.0, 1.0, 1.0, 1.0);
			resize.init_item(IDOK, 1.0, 1.0, 1.0, 1.0);
      EnableWindow(GetDlgItem(hwndDlg, IDOK), false);
			RestoreWindowPos(hwndDlg, DYNACTIONWND_POS_KEY);

			lv = new DynActionListView(GetDlgItem(hwndDlg, IDC_LIST), GetDlgItem(hwndDlg, IDC_EDIT));
			if (SWS_THEMING)
			{
#ifndef _WIN32
        // override list view props for grid line theming
        ListView_SetExtendedListViewStyleEx(lv->GetHWND(), LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT|LVS_EX_HEADERDRAGDROP, LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT|LVS_EX_HEADERDRAGDROP);
#endif
        SNM_ThemeListView(lv); // initial theming, then ListView_HookThemeColorsMessage() does the job
			}
			lv->Update();
			return 0;
		case WM_TIMER:
			if (lv && wParam == 0x1000) // 0x1000 == CELL_EDIT_TIMER
        lv->OnEditingTimer();
      break;
		case WM_CONTEXTMENU:
			break;
    case WM_NOTIFY:
		{
			NMHDR* hdr = (NMHDR*)lParam;
			if (lv && hdr->hwndFrom == lv->GetHWND())
				return lv->OnNotify(wParam, lParam);
			break;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam))
      {
        case IDOK:
          UnregisterDynamicActions();
          SaveDynamicActions();
          RegisterDynamicActions();
          // fall through!

        case IDCANCEL:
          SaveWindowPos(hwndDlg, DYNACTIONWND_POS_KEY);
          EndDialog(hwndDlg,0);
          break;
      }
			return 0;
		case WM_SIZE:
			if (wParam != SIZE_MINIMIZED)
			{
				static int size_reent;
				if (!size_reent)
				{
					++size_reent;
					if (lv) lv->EditListItemEnd(true);
					resize.onResize();
					--size_reent;
				}
			}
			break;
		case WM_DESTROY:
			if (lv) 
      {
        lv->EditListItemEnd(false);
        lv->OnDestroy();
      }
      DELETE_NULL(lv);
			resize.init(NULL);
			break;
	}
	return 0;
}

void ShowDynActionWnd(COMMAND_T* _ct)
{
	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_SNM_DYNACTION), g_hwndParent, DynActionWndProc);
}

