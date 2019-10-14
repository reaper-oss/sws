/******************************************************************************
/ FloatingInspector.cpp
/
/ Copyright (c) 2009 Tim Payne (SWS), original code by Xenakios
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
#include "../SnM/SnM_Dlg.h"

using namespace std;

HWND g_hItemInspector=NULL;
bool g_ItemInspectorVisible=false;

WDL_DLGRET MyItemInspectorDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static HMENU g_hItemInspCtxMenu=0;
	static int g_InspshowMode=1;
	
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, Message, wParam, lParam))
		return r;
	
	switch(Message)
    {
        case WM_INITDIALOG:
			g_hItemInspCtxMenu = CreatePopupMenu();
			AddToMenu(g_hItemInspCtxMenu, "Show number of selected items/tracks", 666);
			AddToMenu(g_hItemInspCtxMenu, "Show item properties", 667);
			break;
		case WM_RBUTTONUP:
		{
			POINT pieru;
			pieru.x=GET_X_LPARAM(lParam);
			pieru.y=GET_Y_LPARAM(lParam);
			ClientToScreen(hwnd,&pieru);
			
			int ContextResult = TrackPopupMenu(g_hItemInspCtxMenu,TPM_LEFTALIGN|TPM_RETURNCMD,pieru.x,pieru.y,0,hwnd,NULL);
				
			if (ContextResult == 666)
				g_InspshowMode = 0;
			else if (ContextResult == 667)
				g_InspshowMode = 1;

			break;
		}
		case WM_COMMAND:
			if (LOWORD(wParam)==IDCANCEL)
			{
				KillTimer(g_hItemInspector, 1);
				ShowWindow(g_hItemInspector, SW_HIDE);
				g_ItemInspectorVisible=false;	
			}
			break;
		case WM_TIMER:
		{
			ostringstream infoText;
			const int NumSelItems=CountSelectedMediaItems(NULL);
			if (g_InspshowMode==0)
			{
				if (NumSelItems>0)
				{
					if (NumSelItems>1)
						infoText << NumSelItems << " items selected ";
					else
						infoText << "1 item selected ";
				}
				else
					infoText << "No items selected";
				//SetDlgItemText(hwnd,IDC_IISTATIC1,textbuf);
				//IDC_IISTATIC2
				int i;
				MediaTrack *CurTrack;
				int n=0;
				for (i=0;i<GetNumTracks();i++)
				{
					CurTrack=CSurf_TrackFromID(i+1,false);
					int isSel=*(int*)GetSetMediaTrackInfo(CurTrack,"I_SELECTED",NULL);
					if (isSel==1)
						n++;
						
				}
				if (n>0)
				{
					if (n>1)
						infoText << "\t" << n << " tracks selected"; //sprintf(textbuf,"%d tracks selected",n);
					else
						infoText << "\t1 track selected";
				}
				else
					infoText << "\tNo tracks selected";
				SetDlgItemText(hwnd,IDC_IISTATIC1,infoText.str().c_str());
			}
			if (g_InspshowMode==1)
			{
				vector<MediaItem_Take*> TheTakes;
				XenGetProjectTakes(TheTakes,true,true);
				if (TheTakes.size()>0)
				{
					infoText << (char*)GetSetMediaItemTakeInfo(TheTakes[0],"P_NAME",NULL) << " Pitch : ";
					infoText << std::setprecision(3) << *(double*)GetSetMediaItemTakeInfo(TheTakes[0],"D_PITCH",NULL);
					infoText << "\tPlayrate : " << *(double*)GetSetMediaItemTakeInfo(TheTakes[0],"D_PLAYRATE",NULL);
					MediaItem* CurItem=(MediaItem*)GetSetMediaItemTakeInfo(TheTakes[0],"P_ITEM",NULL);
					if (CurItem)
					{
						int curTakeInd=666;
						curTakeInd= *(int*)GetSetMediaItemInfo(CurItem,"I_CURTAKE",NULL);
						infoText << " Take " << curTakeInd+1;
						infoText << " / " << GetMediaItemNumTakes(CurItem);
					}
					SetDlgItemText(hwnd,IDC_IISTATIC1,infoText.str().c_str());	
				}
				else
					SetDlgItemText(hwnd,IDC_IISTATIC1,"No item selected");
			}
			break;
		}
		case WM_DESTROY:
			g_hItemInspector = NULL;
			DestroyMenu(g_hItemInspCtxMenu);
			break;
	}
	return 0;
}

void DoTglFltItemInspector(COMMAND_T*)
{
	if (g_ItemInspectorVisible)
	{
		g_ItemInspectorVisible = false;
		KillTimer(g_hItemInspector, 1);
		ShowWindow(g_hItemInspector, SW_HIDE);
	}
	else
	{
		g_ItemInspectorVisible = true;
		SetTimer(g_hItemInspector, 1, 200, NULL);
		ShowWindow(g_hItemInspector, SW_SHOW);
	}
}
