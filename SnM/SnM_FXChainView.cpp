/******************************************************************************
/ SnM_FXChainView.cpp
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

#include "stdafx.h"
#include "SnM_Actions.h"
#include "../Utility/SectionLock.h"
#include "SnM_FXChainView.h"

#define SAVEWINDOW_POS_KEY "S&M - FX Chain List Save Window Position"

#define CLEAR_MSG					0x10001
#define LOAD_MSG					0x10002
#define LOAD_APPLY_TRACK_MSG		0x10103
#define LOAD_APPLY_TAKE_MSG			0x10104
#define LOAD_APPLY_ALL_TAKES_MSG	0x10105
#define COPY_MSG					0x10106
#define DISPLAY_MSG					0x10107


// Globals
SNM_FXChainWnd* g_pFXChainsWnd = NULL;
int g_fxChainSlots[MAX_FXCHAIN_SLOTS]; // Awesome model (just to store some int*)
HANDLE g_fxChainSlotsMutex;
static SWS_LVColumn g_fxChainListCols[] = { {50,2,"Slot"}, {100,2,"Name"}, {250,2,"Path"} };

SNM_FXChainView::SNM_FXChainView(HWND hwndList, HWND hwndEdit)
:SWS_ListView(hwndList, hwndEdit, 3, g_fxChainListCols, "S&M - FX Chain View State", false) 
{}

void SNM_FXChainView::GetItemText(LPARAM item, int iCol, char* str, int iStrMax)
{
	int* slot = (int*)item;
	if (slot)
	{
		switch (iCol)
		{
			case 0:
				_snprintf(str, iStrMax, "%2.d", (*slot) + 1);
				break;
			case 1:
				{
					char buf[BUFFER_SIZE];
					ExtractFileNameEx(g_fxChainList[*slot].Get(), buf, true);
					lstrcpyn(str, buf, iStrMax);
				}
				break;
			case 2:
				lstrcpyn(str, g_fxChainList[*slot].Get(), iStrMax);				
				break;
		}
	}
}

void SNM_FXChainView::OnItemDblClk(LPARAM item, int iCol)
{
	int* slot = (int*)item;
	if (g_pFXChainsWnd && !g_fxChainList[*slot].GetLength())
		g_pFXChainsWnd->OnCommand(LOAD_MSG, item);
}

void SNM_FXChainView::GetItemList(WDL_TypedBuf<LPARAM>* pBuf)
{
	SectionLock lock(g_fxChainSlotsMutex);
	pBuf->Resize(MAX_FXCHAIN_SLOTS);
	for (int i = 0; i < pBuf->GetSize(); i++)
		pBuf->Get()[i] = (LPARAM)&g_fxChainSlots[i];
}

SNM_FXChainWnd::SNM_FXChainWnd()
:SWS_DockWnd(IDD_SNM_FXCHAINLIST, "FX Chains", 30002/*JFB ?*/)
{
	if (m_bShowAfterInit)
		Show(false, false);
}

void SNM_FXChainWnd::Update()
{
	if (m_pLists.GetSize())
	{
		SectionLock lock(g_fxChainSlotsMutex); 
		m_pLists.Get(0)->Update();
	}
}

void SNM_FXChainWnd::OnInitDlg()
{
	m_resize.init_item(IDC_LIST, 0.0, 0.0, 1.0, 1.0);
	m_pLists.Add(new SNM_FXChainView(GetDlgItem(m_hwnd, IDC_LIST), GetDlgItem(m_hwnd, IDC_EDIT)));
	Update();
//	SetTimer(m_hwnd, 1, 1000, NULL);
}

void SNM_FXChainWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam >= CLEAR_MSG && wParam <= DISPLAY_MSG)
	{
		if (ListView_GetSelectedCount(m_pLists.Get(0)->GetHWND()))
		{
			LVITEM li;
			li.mask = LVIF_STATE | LVIF_PARAM;
			li.stateMask = LVIS_SELECTED;
			li.iSubItem = 0;
			bool done = false; // done once in case of multi-selection
			for (int i = 0; !done && i < ListView_GetItemCount(m_pLists.Get(0)->GetHWND()); i++)
			{
				li.iItem = i;
				ListView_GetItem(m_pLists.Get(0)->GetHWND(), &li);
				if (li.state == LVIS_SELECTED)
				{
					int* slot = (int*)li.lParam;
					switch (wParam)
					{
						case LOAD_MSG:
							browseStoreFXChain(*slot);
							break;
						case CLEAR_MSG:
							clearFXChainSlot(*slot);
							break;
						case COPY_MSG:
							copySlotToClipBoard(*slot);
							break;
						case DISPLAY_MSG:
							displayFXChain(*slot);
							break;
						case LOAD_APPLY_TRACK_MSG:
							loadPasteTrackFXChain("Load/Apply FX Chain to selected tracks(s)", *slot);
							done = true;
							break;
						case LOAD_APPLY_TAKE_MSG:
							loadPasteTakeFXChain("Load/Apply FX Chain to selected item(s)", *slot, true);
							done = true;
							break;
						case LOAD_APPLY_ALL_TAKES_MSG:
							loadPasteTakeFXChain("Load/Apply FX Chain to selected item(s), all takes", *slot, false);
							done = true;
							break;
						default:
							break;
					}
				}
				Update(); //for multi-selection..
			}
		}
	}
	else 
		Main_OnCommand((int)wParam, (int)lParam);
}

HMENU SNM_FXChainWnd::OnContextMenu(int x, int y)
{
	HMENU hMenu = CreatePopupMenu();
/* avoid some strange popups
	LPARAM item = m_pLists.Get(0)->GetHitItem(x, y, NULL);
	if (item)
*/
	{
		AddToMenu(hMenu, "Load slot(s)...", LOAD_MSG);
		AddToMenu(hMenu, "Clear slot(s)", CLEAR_MSG);
		AddToMenu(hMenu, SWS_SEPARATOR, 0);
		AddToMenu(hMenu, "Copy", COPY_MSG);
		AddToMenu(hMenu, SWS_SEPARATOR, 0);
		AddToMenu(hMenu, "Load/Apply to selected tracks", LOAD_APPLY_TRACK_MSG);
		AddToMenu(hMenu, SWS_SEPARATOR, 0);
		AddToMenu(hMenu, "Load/Apply to selected items", LOAD_APPLY_TAKE_MSG);
		AddToMenu(hMenu, "Load/Apply selected items (all takes)", LOAD_APPLY_ALL_TAKES_MSG);
		AddToMenu(hMenu, SWS_SEPARATOR, 0);
		AddToMenu(hMenu, "Display FX Chain...", DISPLAY_MSG);
	}	
	return hMenu;
}

void SNM_FXChainWnd::OnDestroy() {
//	KillTimer(m_hwnd, 1);
}

int SNM_FXChainWnd::OnKey(MSG* msg, int iKeyState) {
	if (msg->message == WM_KEYDOWN && msg->wParam == VK_DELETE && !iKeyState)
	{
		OnCommand(CLEAR_MSG, 0);
		return 1;
	}
	return 0;
}

int FXChainListInit()
{
	for (int i=0; i < MAX_FXCHAIN_SLOTS; i++)
	{
		SectionLock lock(g_fxChainSlotsMutex);
		char path[BUFFER_SIZE] = "";
		readFXChainSlotIniFile(i, path, BUFFER_SIZE);
		g_fxChainList[i].Set(path);
		g_fxChainSlots[i] = i;
	}

	g_pFXChainsWnd = new SNM_FXChainWnd();
	return 1;
}

void FXChainListExit() {
	delete g_pFXChainsWnd;
}

void OpenFXChainList(COMMAND_T*) {
	g_pFXChainsWnd->Show(true, true);
}

