/******************************************************************************
/ ItemTakeCommands.cpp
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
#include "Parameters.h"
#include "../Breeder/BR_Util.h"
#include "../SnM/SnM_Util.h"

#include <WDL/localize/localize.h>

using namespace std;

int XenGetProjectItems(vector<MediaItem*>& TheItems,bool OnlySelectedItems, bool IncEmptyItems)
{
	TheItems.clear();
	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* CurTrack = CSurf_TrackFromID(i+1, false);
		for (int j = 0; j < GetTrackNumMediaItems(CurTrack); j++)
		{
			MediaItem* CurItem = GetTrackMediaItem(CurTrack, j);
			if (CurItem && (!OnlySelectedItems || *(bool*)GetSetMediaItemInfo(CurItem, "B_UISEL", NULL)))
			{
				if (IncEmptyItems || GetMediaItemNumTakes(CurItem) > 0)
					TheItems.push_back(CurItem);
			}
		}
	}
	return (int)TheItems.size();
}

int XenGetProjectTakes(vector<MediaItem_Take*>& TheTakes, bool OnlyActive, bool OnlyFromSelectedItems)
{
	TheTakes.clear();
	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* CurTrack = CSurf_TrackFromID(i+1,false);
		for (int j = 0; j < GetTrackNumMediaItems(CurTrack); j++)
		{
			MediaItem* CurItem=GetTrackMediaItem(CurTrack,j);
			if (CurItem && (!OnlyFromSelectedItems || *(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL)))
			{
				if (OnlyActive)
				{
					MediaItem_Take* CurTake = GetMediaItemTake(CurItem, -1);
					if (CurTake)
						TheTakes.push_back(CurTake);
				}
				else for (int k = 0; k < GetMediaItemNumTakes(CurItem); k++)
				{
					MediaItem_Take* CurTake = GetMediaItemTake(CurItem, k);
					if (CurTake)
						TheTakes.push_back(CurTake);
				}
			}
		}
	}
	return (int)TheTakes.size();
}

void DoMoveItemsLeftByItemLen(COMMAND_T* ct)
{
	vector<MediaItem*> VecItems;
	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* CurTrack=CSurf_TrackFromID(i+1,false);
		for (int j = 0; j < GetTrackNumMediaItems(CurTrack); j++)
		{
			MediaItem* CurItem = GetTrackMediaItem(CurTrack,j);
			bool isSel = *(bool*)GetSetMediaItemInfo(CurItem, "B_UISEL", NULL);
			if (isSel) VecItems.push_back(CurItem);
		}
	}
	for (int i = 0; i < (int)VecItems.size(); i++)
	{
		double CurPos = *(double*)GetSetMediaItemInfo(VecItems[i], "D_POSITION", NULL);
		double dItemLen = *(double*)GetSetMediaItemInfo(VecItems[i], "D_LENGTH", NULL);
		double NewPos = CurPos - dItemLen;
		GetSetMediaItemInfo(VecItems[i], "D_POSITION", &NewPos);
	}
	UpdateTimeline();
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct), UNDO_STATE_ITEMS, -1);
}

void DoToggleTakesNormalize(COMMAND_T* ct)
{
	WDL_PtrList<MediaItem_Take> *TheTakes=new (WDL_PtrList<MediaItem_Take>);
	int NumActiveTakes=GetActiveTakes(TheTakes);
	int NumNormalizedTakes=0;
	for (int i=0;i<NumActiveTakes;i++)
	{
		if (TheTakes->Get(i))
		{
			double TakeVol=abs(*(double*)GetSetMediaItemTakeInfo(TheTakes->Get(i),"D_VOL",NULL));
			if (TakeVol!=1.0) NumNormalizedTakes++;
		}
	}
	if (NumNormalizedTakes>0)
	{
		for (int i=0;i<NumActiveTakes;i++)
		{
			if (MediaItem_Take* take = TheTakes->Get(i))
			{
				double TheGain = IsTakePolarityFlipped(take) ? -1.0 : 1.0;
				GetSetMediaItemTakeInfo(take,"D_VOL",&TheGain);
			}
		}
		Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),UNDO_STATE_ITEMS,-1);
		UpdateTimeline();
	}
	else
	{
		Undo_BeginBlock2(NULL);
		Main_OnCommand(40108, 0); // normalize items
		Undo_EndBlock2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ITEMS | UNDO_STATE_TRACKCFG | UNDO_STATE_MISCCFG);
		UpdateTimeline();
	}
	delete TheTakes;
}


void DoSetVolPan(double Vol, double Pan, bool SetVol, bool SetPan)
{
	vector<MediaItem_Take*> TheTakes;
	XenGetProjectTakes(TheTakes,true,true);
	double NewVol=Vol;
	double NewPan=Pan;
	for (int i=0;i<(int)TheTakes.size();i++)
	{
		if (SetVol)
		{
			MediaItem_Take* take = TheTakes[i];
			if (IsTakePolarityFlipped(take))
				NewVol = -NewVol;
			GetSetMediaItemTakeInfo(take, "D_VOL", &NewVol);
		}
		if (SetPan) GetSetMediaItemTakeInfo(TheTakes[i],"D_PAN",&NewPan);

	}
	Undo_OnStateChangeEx(__LOCALIZE("Set take vol/pan","sws_undo"),UNDO_STATE_ITEMS,-1);
	UpdateTimeline();
}

void DoSetItemVols(double theVol)
{
	for (int i=0;i<GetNumTracks();i++)
	{
		MediaTrack* pTrack = CSurf_TrackFromID(i+1, false);
		for (int j = 0; j < GetTrackNumMediaItems(pTrack); j++)
		{
			MediaItem* CurItem = GetTrackMediaItem(pTrack,j);
			if (*(bool*)GetSetMediaItemInfo(CurItem, "B_UISEL", NULL))
				GetSetMediaItemInfo(CurItem, "D_VOL", &theVol);
		}
	}
	Undo_OnStateChangeEx(__LOCALIZE("Set item volume","sws_undo"), 4, -1);
	UpdateTimeline();
}


WDL_DLGRET ItemSetVolDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, Message, wParam, lParam))
		return r;

	switch(Message)
    {
        case WM_INITDIALOG:
		{
			SetDlgItemText(hwnd, IDC_VOLEDIT, "0.0");
			SetFocus(GetDlgItem(hwnd, IDC_VOLEDIT));
			SendMessage(GetDlgItem(hwnd, IDC_VOLEDIT), EM_SETSEL, 0, -1);
			break;
		}
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
				{
					char tbuf[100];
					GetDlgItemText(hwnd,IDC_VOLEDIT,tbuf,99);
					double NewVol=1.0;
					bool WillSetVolume=false;
					if (strcmp(tbuf,"")!=0)
					{
						WillSetVolume=true;
						NewVol=atof(tbuf);
						if (NewVol>-144.0)
							NewVol=exp(NewVol*0.115129254);
						else NewVol=0.0;

					}
					if (WillSetVolume)
						DoSetItemVols(NewVol);

					EndDialog(hwnd,0);
					break;
				}
				case IDCANCEL:
					EndDialog(hwnd,0);
					break;
			}
	}
	return 0;
}

void DoShowItemVolumeDialog(COMMAND_T*)
{
	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ITEMVOLUME), g_hwndParent, ItemSetVolDlgProc);
}

WDL_DLGRET ItemPanVolDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, Message, wParam, lParam))
		return r;

	switch(Message)
    {
        case WM_INITDIALOG:

			SetDlgItemText(hwnd, IDC_VOLEDIT, "");
			SetDlgItemText(hwnd, IDC_PANEDIT, "");
			SetFocus(GetDlgItem(hwnd, IDC_VOLEDIT));
			SendMessage(GetDlgItem(hwnd, IDC_VOLEDIT), EM_SETSEL, 0, -1);
			break;
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
				{
					char TempString[100];
					GetDlgItemText(hwnd,IDC_VOLEDIT,TempString,99);
					bool SetVol=FALSE;
					bool SetPan=FALSE;
					double NewVol=strtod(TempString,NULL);

					if (strcmp(TempString,"")!=0)
					{
						if (NewVol>-144.0)
							NewVol=exp(NewVol*0.115129254);
						else
							NewVol=0;
						SetVol=TRUE;
					}

					GetDlgItemText (hwnd,IDC_PANEDIT,TempString,99);
					double NewPan=strtod(TempString,NULL);
					if (strcmp(TempString,"")!=0)
						SetPan=TRUE;
					DoSetVolPan(NewVol,NewPan/100.0,SetVol,SetPan);
					EndDialog(hwnd,0);
					break;
				}
				case IDCANCEL:
					EndDialog(hwnd,0);
					break;
			}
	}
	return 0;
}

void PasteMultipletimes(int NumPastes, double TimeIntervalQN, int RepeatMode)
{
	if (NumPastes > 0)
	{
		const ConfigVar<int> pCursorMode("itemclickmovecurs");
		double dStartTime = GetCursorPosition();

		Undo_BeginBlock();

		switch (RepeatMode)
		{
			case 0: // No gaps
			{
				// Cursor mode must set to move the cursor after paste
				// Clear bit 8 of itemclickmovecurs
				ConfigVarOverride<int> tmpCursorMode(pCursorMode, *pCursorMode & ~8);
				for (int i = 0; i < NumPastes; i++)
					Main_OnCommand(40058,0); // Paste
				break;
			}
			case 1: // Time interval
				for (int i = 0; i < NumPastes; i++)
				{
					Main_OnCommand(40058,0); // Paste
					SetEditCurPos(dStartTime + (i+1) * TimeIntervalQN, false, false);
				}
				break;
			case 2: // Beat interval
			{
				double dStartBeat = TimeMap_timeToQN(dStartTime);
				for (int i = 0; i < NumPastes; i++)
				{
					Main_OnCommand(40058,0); // Paste
					SetEditCurPos(TimeMap_QNToTime(dStartBeat + (i+1) * TimeIntervalQN), false, false);
				}
				break;
			}
		}
		if (*pCursorMode & 8) // If "don't move cursor after paste" move the cursor back
			SetEditCurPos(dStartTime, false, false);

		Undo_EndBlock(__LOCALIZE("Repeat paste","sws_undo"),0);
	}
}



WDL_DLGRET RepeatPasteDialogProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static double dTimeInterval = 1.0;
	static double dBeatInterval = 1.0;
	static char cBeatStr[50] = "1";
	static int iNumRepeats = 1;
	static int iRepeatMode = 2;

	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, Message, wParam, lParam))
		return r;

	switch(Message)
    {
		case WM_INITDIALOG:
		{
			char TextBuf[314];
			sprintf(TextBuf,"%.2f", dTimeInterval);
			SetDlgItemText(hwnd, IDC_EDIT1, TextBuf);

			sprintf(TextBuf,"%d", iNumRepeats);
			SetDlgItemText(hwnd, IDC_NUMREPEDIT, TextBuf);
			InitFracBox(GetDlgItem(hwnd, IDC_NOTEVALUECOMBO), cBeatStr);

			SetFocus(GetDlgItem(hwnd, IDC_NUMREPEDIT));
			SendMessage(GetDlgItem(hwnd, IDC_NUMREPEDIT), EM_SETSEL, 0, -1);

			switch (iRepeatMode)
			{
				case 0: CheckDlgButton(hwnd, IDC_RADIO1, BST_CHECKED); break;
				case 1: CheckDlgButton(hwnd, IDC_RADIO2, BST_CHECKED); break;
				case 2: CheckDlgButton(hwnd, IDC_RADIO3, BST_CHECKED); break;
			}
			break;
		}
		case WM_COMMAND:
            switch(LOWORD(wParam))
            {
				case IDOK:
				{
					char str[100];
					GetDlgItemText(hwnd, IDC_NUMREPEDIT, str, 100);
					iNumRepeats = atoi(str);

					if (IsDlgButtonChecked(hwnd, IDC_RADIO1) == BST_CHECKED)
						iRepeatMode = 0;
					else if (IsDlgButtonChecked(hwnd, IDC_RADIO2) == BST_CHECKED)
						iRepeatMode = 1;
					else if (IsDlgButtonChecked(hwnd, IDC_RADIO3) == BST_CHECKED)
						iRepeatMode = 2;

					if (iRepeatMode == 2)
					{
						GetDlgItemText(hwnd, IDC_NOTEVALUECOMBO, cBeatStr, 100);
						dBeatInterval = parseFrac(cBeatStr);
						PasteMultipletimes(iNumRepeats, dBeatInterval, iRepeatMode);
					}
					else if (iRepeatMode == 1)
					{
						GetDlgItemText(hwnd, IDC_EDIT1, str, 100);
						dTimeInterval = atof(str);
						PasteMultipletimes(iNumRepeats, dTimeInterval, iRepeatMode);
					}
					else if (iRepeatMode == 0)
						PasteMultipletimes(iNumRepeats, 1.0, iRepeatMode);

					EndDialog(hwnd,0);
					break;
				}
				case IDCANCEL:
					EndDialog(hwnd,0);
					break;
		}
	}
	return 0;
}


void DoShowVolPanDialog(COMMAND_T*)
{
	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ITEMPANVOLDIALOG), g_hwndParent, ItemPanVolDlgProc);
}

void DoChooseNewSourceFileForSelTakes(COMMAND_T* ct)
{
	WDL_PtrList<MediaItem_Take> *TheTakes=new (WDL_PtrList<MediaItem_Take>);
	PCM_source *ThePCMSource;
	int i;
	int NumActiveTakes=GetActiveTakes(TheTakes);
	if (NumActiveTakes>0)
	{
		MediaItem_Take* CurTake;
		char* cFileName = BrowseForFiles(__LOCALIZE("Choose new source file","sws_mbox"), NULL, NULL, false, plugin_getFilterList());
		if (cFileName)
		{
			Main_OnCommand(40440,0); // Selected Media Offline
			for (i=0;i<NumActiveTakes;i++)
			{
				CurTake=TheTakes->Get(i);
				ThePCMSource=(PCM_source*)GetSetMediaItemTakeInfo(CurTake,"P_SOURCE",NULL);
				if (ThePCMSource)
				{
					if (strcmp(ThePCMSource->GetType(), "SECTION") != 0)
					{
						PCM_source *NewPCMSource = PCM_Source_CreateFromFile(cFileName);
						if (NewPCMSource!=0)
						{
							GetSetMediaItemTakeInfo(CurTake,"P_SOURCE",NewPCMSource);
							delete ThePCMSource;
						}
					}
					else
					{
						PCM_source *TheOtherPCM=ThePCMSource->GetSource();
						if (TheOtherPCM!=0)
						{
							PCM_source *NewPCMSource=PCM_Source_CreateFromFile(cFileName);
							if (NewPCMSource)
							{
								ThePCMSource->SetSource(NewPCMSource);
								delete TheOtherPCM;
							}
						}
					}
				}
			}
			free(cFileName);
			Main_OnCommand(40047,0); // Build any missing peaks
			Main_OnCommand(40439,0); // Selected Media Online
			Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),UNDO_STATE_ITEMS,-1);
			UpdateTimeline();
		}
	}
	delete TheTakes;
}

void DoInvertItemSelection(COMMAND_T* _ct)
{
	PreventUIRefresh(1);
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, FALSE);
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* CurItem = GetTrackMediaItem(tr, j);
			if (*(bool*)GetSetMediaItemInfo(CurItem, "B_UISEL", NULL))
				GetSetMediaItemInfo(CurItem, "B_UISEL", &g_bFalse);
			else
				GetSetMediaItemInfo(CurItem, "B_UISEL", &g_bTrue);
		}
	}
	PreventUIRefresh(-1);
	UpdateArrange();
	Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}

bool DoLaunchExternalTool(const char *ExeFilename)
{
	if (!ExeFilename)
		return false;

#ifdef _WIN32
	STARTUPINFO          si = { sizeof(si) };
	PROCESS_INFORMATION  pi;
	char* cFile = _strdup(ExeFilename);
	bool bRet = CreateProcess(0, cFile, 0, 0, FALSE, 0, 0, 0, &si, &pi) ? true : false;
	free(cFile);
	return bRet;
#else
	MessageBox(g_hwndParent, __LOCALIZE("Not supported on OSX and Linux, sorry!", "sws_mbox"), __LOCALIZE("SWS - Error", "sws_mbox"), MB_OK);
	return false;
#endif
}

void DoRepeatPaste(COMMAND_T*)
{
	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_REPEATPASTEDLG), g_hwndParent, RepeatPasteDialogProc);
}

void DoSelectEveryNthItemOnSelectedTracks(int Step,int ItemOffset)
{
	PreventUIRefresh(1);

	Main_OnCommand(40289,0); // Unselect all items

	int flags;
	for (int i = 0; i < GetNumTracks(); i++)
	{
		GetTrackInfo(i,&flags);
		if (flags & 0x02)
		{
			MediaTrack* MunRaita = CSurf_TrackFromID(i+1,FALSE);
			int ItemCounter = 0;
			for (int j = 0; j < GetTrackNumMediaItems(MunRaita); j++)
			{
				MediaItem* CurItem = GetTrackMediaItem(MunRaita,j);

				if ((ItemCounter % Step) == ItemOffset)
					GetSetMediaItemInfo(CurItem,"B_UISEL",&g_bTrue);
				else
					GetSetMediaItemInfo(CurItem,"B_UISEL",&g_bFalse);

				ItemCounter++;
			}
		}
	}
	PreventUIRefresh(-1);
	UpdateArrange();
}

void DoSelectSkipSelectOnSelectedItems(int Step,int ItemOffset)
{
	MediaTrack* MunRaita;
	MediaItem* CurItem;

	int numItems;

	PreventUIRefresh(1);
	//Main_OnCommand(40289,0); // Unselect all items
	bool NewSelectedStatus=false;
	int flags, i, j;
	for (i=0;i<GetNumTracks();i++)
	{
		GetTrackInfo(i,&flags);
		//if (flags & 0x02)
		{
			MunRaita = CSurf_TrackFromID(i+1,FALSE);
			int ItemCounter=0;
			numItems=GetTrackNumMediaItems(MunRaita);
			for (j=0;j<numItems;j++)
			{
				CurItem = GetTrackMediaItem(MunRaita,j);
				bool ItemSelected=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
				if (ItemSelected)
				{
					if ((ItemCounter % Step)==ItemOffset)
					{
						NewSelectedStatus=TRUE;
						GetSetMediaItemInfo(CurItem,"B_UISEL",&NewSelectedStatus);
					}
					else
					{
						NewSelectedStatus=FALSE;
						GetSetMediaItemInfo(CurItem,"B_UISEL",&NewSelectedStatus);
					}
					ItemCounter++;
				}
			}
		}
	}
	PreventUIRefresh(-1);
	UpdateArrange();
}


int NumSteps=2;
int StepOffset=1;
int SkipItemSource=0; // 0 for all in selected tracks, 1 for items selected in selected tracks

WDL_DLGRET SelEveryNthDialogProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, Message, wParam, lParam))
		return r;

	switch(Message)
    {
        case WM_INITDIALOG:
			{
			//SetDlgItemText(hwnd, IDC_EDIT1, "2");
			//SetDlgItemText(hwnd, IDC_EDIT2, "0");
			char TextBuf[32];
			sprintf(TextBuf,"%d",NumSteps);

			SetDlgItemText(hwnd, IDC_EDIT1, TextBuf);
			sprintf(TextBuf,"%d",StepOffset);
			SetDlgItemText(hwnd, IDC_EDIT2, TextBuf);
			SetFocus(GetDlgItem(hwnd, IDC_EDIT1));
			SendMessage(GetDlgItem(hwnd, IDC_EDIT1), EM_SETSEL, 0, -1);
			return 0;
			}
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
					{
						char TempString[32];
						GetDlgItemText(hwnd,IDC_EDIT1,TempString,31);
						NumSteps=atoi(TempString);
						GetDlgItemText(hwnd,IDC_EDIT2,TempString,31);
						StepOffset=atoi(TempString);
						if (SkipItemSource==0)
							DoSelectEveryNthItemOnSelectedTracks(NumSteps,StepOffset);
						if (SkipItemSource==1)
							DoSelectSkipSelectOnSelectedItems(NumSteps,StepOffset);
						EndDialog(hwnd,0);
						return 0;
					}
				case IDCANCEL:
					{
						EndDialog(hwnd,0);
						return 0;
					}
			}
	}
	return 0;
}

void ShowSelectSkipFromSelectedItems(int SelectMode)
{
	SkipItemSource=SelectMode;
	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_SELECT_NTH_ITEMDLG), g_hwndParent, SelEveryNthDialogProc);
}

void DoSkipSelectAllItemsOnTracks(COMMAND_T*)
{
	ShowSelectSkipFromSelectedItems(0);
}

void DoSkipSelectFromSelectedItems(COMMAND_T*)
{
	ShowSelectSkipFromSelectedItems(1);
}

int *TakeIndexes;

bool GenerateShuffledTakeRandomTable(int *IntTable,int numItems,int badFirstNumber)
{
	int *CheckTable=new int[1024];
	bool GoodFound=FALSE;
	int IterCount=0;
	int rndInt;
	int i;
	for (i=0;i<1024;i++)
	{
		CheckTable[i]=0;
		IntTable[i]=0;
	}

	for (i=0;i<numItems;i++)
	{
		GoodFound=FALSE;
		while (!GoodFound)
		{
			rndInt=rand() % numItems;
			if ((CheckTable[rndInt]==0) && (rndInt!=badFirstNumber) && (i==0))
				GoodFound=TRUE;
			if ((CheckTable[rndInt]==0) && (i>0))
				GoodFound=TRUE;

			IterCount++;
			if (IterCount>1000000)
				break;
		}
		if (GoodFound)
		{
			IntTable[i]=rndInt;
			CheckTable[rndInt]=1;
		}
	}
	delete[] CheckTable;
	return FALSE;
}

void DoShuffleSelectTakesInItems(COMMAND_T* ct)
{
	const int cnt=CountSelectedMediaItems(NULL);
	for (int i = 0; i < cnt; ++i)
	{
		MediaItem* item = GetSelectedMediaItem(NULL, i);
		int takeCount = CountTakes(item);
		if (takeCount > 1)
		{
			int id = *(int*)GetSetMediaItemInfo(item,"I_CURTAKE",NULL);
			--takeCount; // for MTRand

			static int prevId = id;
			int newId = prevId;
			if (takeCount != 1)
			{
				while (newId == prevId)
					newId = g_MTRand.randInt(takeCount);
			}
			else
				newId = g_MTRand.randInt(takeCount);

			prevId = newId;

			GetSetMediaItemInfo(item,"I_CURTAKE",&newId);
		}
	}
	UpdateArrange();
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),UNDO_STATE_ITEMS,-1);
}

void DoMoveItemsToEditCursor(COMMAND_T* ct)
{
	double EditCurPos=GetCursorPosition();
	const int cnt=CountSelectedMediaItems(NULL);
	for (int i = 0; i < cnt; i++)
	{
		MediaItem* CurItem = GetSelectedMediaItem(0, i);
		double SnapOffset = *(double*)GetSetMediaItemInfo(CurItem, "D_SNAPOFFSET", NULL);
		double NewPos = EditCurPos - SnapOffset;
		GetSetMediaItemInfo(CurItem, "D_POSITION", &NewPos);
	}
	if (cnt)
	{
		Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct), UNDO_STATE_ITEMS, -1);
		UpdateTimeline();
	}
}

void DoRemoveItemFades(COMMAND_T* ct)
{
	double dFade = 0.0;
	const int cnt=CountSelectedMediaItems(NULL);
	for (int i = 0; i < cnt; i++)
	{
		MediaItem* item = GetSelectedMediaItem(0, i);
		GetSetMediaItemInfo(item, "D_FADEINLEN",  &dFade);
		GetSetMediaItemInfo(item, "D_FADEOUTLEN", &dFade);
		GetSetMediaItemInfo(item, "D_FADEINLEN_AUTO",  &dFade);
		GetSetMediaItemInfo(item, "D_FADEOUTLEN_AUTO", &dFade);
	}
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct), UNDO_STATE_ITEMS, -1);
	UpdateTimeline();
}


void DoTrimLeftEdgeToEditCursor(COMMAND_T* ct)
{
	double NewLeftEdge = GetCursorPosition();
	bool modified = false;
	const int cnt=CountSelectedMediaItems(NULL);
	for (int i = 0; i < cnt; i++)
	{
		MediaItem* CurItem = GetSelectedMediaItem(0, i);
		double OldLeftEdge = *(double*)GetSetMediaItemInfo(CurItem, "D_POSITION", NULL);
		double OldLength   = *(double*)GetSetMediaItemInfo(CurItem, "D_LENGTH", NULL);
		/* nothing to do if edit cursor is past item right edge */
		if(NewLeftEdge > OldLength + OldLeftEdge)
			continue;
		modified = true;
		double NewLength  = OldLength + (OldLeftEdge - NewLeftEdge);
		for (int k = 0; k < GetMediaItemNumTakes(CurItem); k++)
		{
			MediaItem_Take* CurTake = GetMediaItemTake(CurItem, k);
			if (CurTake)
			{
				double OldMediaOffset = *(double*)GetSetMediaItemTakeInfo(CurTake, "D_STARTOFFS", NULL);
				double playRate = *(double*)GetSetMediaItemTakeInfo(CurTake, "D_PLAYRATE", NULL);
				/* media offsets needs to be scaled by playRate */
				double NewMediaOffset = (OldMediaOffset / playRate - (OldLeftEdge - NewLeftEdge)) * playRate;
				if(NewMediaOffset < 0) {
					double shiftAmount = -NewMediaOffset / playRate;
					NewLeftEdge += shiftAmount;
					NewLength -= shiftAmount;
					NewMediaOffset = 0.0f;
				}
				GetSetMediaItemTakeInfo(CurTake,"D_STARTOFFS",&NewMediaOffset);
			}
		}
		GetSetMediaItemInfo(CurItem, "D_POSITION", &NewLeftEdge);
		GetSetMediaItemInfo(CurItem, "D_LENGTH", &NewLength);
	}
	if (modified)
	{
		Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct), UNDO_STATE_ITEMS, -1);
		UpdateTimeline();
	}
}

void DoTrimRightEdgeToEditCursor(COMMAND_T* ct)
{
	double dRightEdge = GetCursorPosition();
	bool modified = false;
	const int cnt=CountSelectedMediaItems(NULL);
	for (int i = 0; i < cnt; i++)
	{
		modified = true;
		MediaItem* CurItem = GetSelectedMediaItem(0, i);
		double LeftEdge  = *(double*)GetSetMediaItemInfo(CurItem, "D_POSITION", NULL);
		double NewLength = dRightEdge - LeftEdge;
		GetSetMediaItemInfo(CurItem, "D_LENGTH", &NewLength);
	}
	if (modified)
	{
		Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct), UNDO_STATE_ITEMS, -1);
		UpdateTimeline();
	}
}

void DoResetItemRateAndPitch(COMMAND_T* ct)
{
	Undo_BeginBlock();
	Main_OnCommand(40652, 0); // set item rate to 1.0
	Main_OnCommand(40653, 0); // reset item pitch to 0.0
	Undo_EndBlock(SWS_CMD_SHORTNAME(ct),0);
}

void DoApplyTrackFXStereoAndResetVol(COMMAND_T* ct)
{
	Undo_BeginBlock();
	Main_OnCommand(40209,0); // apply track fx in stereo to items
	double dVol = 1.0;
	const int cnt=CountSelectedMediaItems(NULL);
	for (int i = 0; i < cnt; i++)
		GetSetMediaItemInfo(GetSelectedMediaItem(0, i), "D_VOL", &dVol);
	Undo_EndBlock(SWS_CMD_SHORTNAME(ct), UNDO_STATE_ITEMS);
}

void DoApplyTrackFXMonoAndResetVol(COMMAND_T* ct)
{
	Undo_BeginBlock();
	Main_OnCommand(40361,0); // apply track fx in mono to items
	double dVol = 1.0;
	const int cnt=CountSelectedMediaItems(NULL);
	for (int i = 0; i < cnt; i++)
		GetSetMediaItemInfo(GetSelectedMediaItem(0, i), "D_VOL", &dVol);
	Undo_EndBlock(SWS_CMD_SHORTNAME(ct), UNDO_STATE_ITEMS);
}

void DoSelItemsToEndOfTrack(COMMAND_T*)
{
	PreventUIRefresh(1);
	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i+1, false);
		bool bSel = false;
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* mi = GetTrackMediaItem(tr, j);
			if (!bSel && *(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL))
				bSel = true;
			else if (bSel)
				GetSetMediaItemInfo(mi, "B_UISEL", &g_bTrue);
		}
	}
	PreventUIRefresh(-1);
	UpdateArrange();
}

void DoSelItemsToStartOfTrack(COMMAND_T* _ct)
{
	PreventUIRefresh(1);
	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i+1, false);
		int iLastSelItem = -1;
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
			if (*(bool*)GetSetMediaItemInfo(GetTrackMediaItem(tr, j), "B_UISEL", NULL))
				iLastSelItem = j;

		for (int j = 0; j < iLastSelItem; j++)
			GetSetMediaItemInfo(GetTrackMediaItem(tr, j), "B_UISEL", &g_bTrue);
	}
	PreventUIRefresh(-1);
	UpdateArrange();
	Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}

void DoSetAllTakesPlay()
{
	WDL_TypedBuf<MediaItem*> items;
	SWS_GetSelectedMediaItems(&items);
	for (int i = 0; i < items.GetSize(); i++)
		GetSetMediaItemInfo(items.Get()[i], "B_ALLTAKESPLAY", &g_bTrue);
}


void DoPanTakesOfItemSymmetrically()
{
	WDL_TypedBuf<MediaItem*> items;
	SWS_GetSelectedMediaItems(&items);
	for (int i = 0; i < items.GetSize(); i++)
	{
		int iTakes = GetMediaItemNumTakes(items.Get()[i]);
		for (int j = 0; j < iTakes; j++)
		{
			MediaItem_Take* take = GetMediaItemTake(items.Get()[i], j);
			if (take)
			{
				double dNewPan= -1.0 + ((2.0 / (iTakes - 1)) * j);
				GetSetMediaItemTakeInfo(take, "D_PAN", &dNewPan);
			}
		}
	}
	UpdateTimeline();
}

void DoPanTakesSymmetricallyWithUndo(COMMAND_T* ct)
{
	DoPanTakesOfItemSymmetrically();
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),4,-1);
}


void DoImplodeTakesSetPlaySetSymPans(COMMAND_T* ct)
{
	Undo_BeginBlock();
	Main_OnCommand(40438,0); // implode items across tracks into takes
	DoSetAllTakesPlay();
	DoPanTakesOfItemSymmetrically();
	Undo_EndBlock(SWS_CMD_SHORTNAME(ct),0);
}

double g_lastTailLen;

void DoWorkForRenderItemsWithTail(double TailLen)
{
	// Unselect all tracks
	const int msTail = static_cast<int>(1000 * TailLen);
	ConfigVarOverride<int> fxtail("itemfxtail", msTail);

	bool ItemSelected=false;
	for (int i=0;i<GetNumTracks();i++)
	{
		MediaTrack *MunRaita = CSurf_TrackFromID(i+1,false);
		const int numItems=GetTrackNumMediaItems(MunRaita);
		Main_OnCommand(40635,0);
		for (int j=0;j<numItems;j++)
		{
			MediaItem *CurItem = GetTrackMediaItem(MunRaita,j);
			//propertyName="D_";
			ItemSelected=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (ItemSelected)
			{
				Main_OnCommand(40601,0); // render items as new take
			}
		}
	}
	UpdateTimeline();
}

WDL_DLGRET RenderItemsWithTailDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, Message, wParam, lParam))
		return r;

	switch(Message)
    {
        case WM_INITDIALOG:
		{
			char TextBuf[314];

			sprintf(TextBuf,"%.2f",g_lastTailLen);
			SetDlgItemText(hwnd, IDC_EDIT1, TextBuf);
			SetFocus(GetDlgItem(hwnd, IDC_EDIT1));
			SendMessage(GetDlgItem(hwnd, IDC_EDIT1), EM_SETSEL, 0, -1);
			break;
		}
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
				{
					char textbuf[100];
					GetDlgItemText(hwnd,IDC_EDIT1,textbuf,sizeof(textbuf));
					g_lastTailLen=atof(textbuf);
					DoWorkForRenderItemsWithTail(g_lastTailLen);
					EndDialog(hwnd,0);
					break;
				}
				case IDCANCEL:
					EndDialog(hwnd,0);
					break;
			}
	}
	return 0;
}

void DoRenderItemsWithTail(COMMAND_T*)
{
	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_RENDITEMS), g_hwndParent, RenderItemsWithTailDlgProc);
}

void DoOpenAssociatedRPP(COMMAND_T*)
{
	WDL_PtrList<MediaItem_Take> *TheTakes=new (WDL_PtrList<MediaItem_Take>);
	PCM_source *ThePCMSource;
	MediaItem_Take* CurTake;
	int NumActiveTakes=GetActiveTakes(TheTakes);
	if (NumActiveTakes==1)
	{
		CurTake=TheTakes->Get(0); // we will only support first selected item for now
		ThePCMSource=(PCM_source*)GetSetMediaItemTakeInfo(CurTake,"P_SOURCE",NULL);

		if (ThePCMSource && ThePCMSource->GetFileName())
		{
			char RPPFileNameBuf[1024];
			snprintf(RPPFileNameBuf,sizeof(RPPFileNameBuf),"%s\\reaper.exe \"%s.RPP\"",GetExePath(), ThePCMSource->GetFileName());
			if (!DoLaunchExternalTool(RPPFileNameBuf))
				MessageBox(g_hwndParent, __LOCALIZE("Could not launch REAPER!","sws_mbox"), __LOCALIZE("Xenakios - Error","sws_mbox"), MB_OK);
		}
	}
	else
		MessageBox(g_hwndParent, __LOCALIZE("None or more than 1 item selected","sws_mbox"),__LOCALIZE("Xenakios - Error","sws_mbox") ,MB_OK);
	delete TheTakes;
}

typedef struct
{
	double Gap;
	bool bEnd;
} t_ReposItemsParams;

t_ReposItemsParams g_ReposItemsParams = { 1.0, false };

void RepositionItems(double theGap, bool bEnd) // bEnd = true gap from end else start
{
	WDL_TypedBuf<MediaItem*> items;

	int numTracks = CountTracks(NULL);
	for (int i = 0; i < numTracks; i++)
	{
		MediaTrack* track = CSurf_TrackFromID(i + 1, false);
		SWS_GetSelectedMediaItemsOnTrack(&items, track);

		for (int j = 1; j < items.GetSize(); j++)
		{
			double dPrevItemStart = *(double*)GetSetMediaItemInfo(items.Get()[j - 1], "D_POSITION", NULL);
			double dNewPos = dPrevItemStart + theGap;
			if (bEnd)  // From the previous selected item end, add the prev item length
				dNewPos += *(double*)GetSetMediaItemInfo(items.Get()[j-1], "D_LENGTH", NULL);

			GetSetMediaItemInfo(items.Get()[j], "D_POSITION", &dNewPos);
		}
	}
}

WDL_DLGRET ReposItemsDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, Message, wParam, lParam))
		return r;

	switch(Message)
    {
		case WM_INITDIALOG:
		{
			if (!g_ReposItemsParams.bEnd)
				CheckDlgButton(hwnd, IDC_RADIO1, BST_CHECKED);
			else
				CheckDlgButton(hwnd, IDC_RADIO2, BST_CHECKED);
			char TextBuf[314];

			sprintf(TextBuf, "%.2f", g_ReposItemsParams.Gap);
			SetDlgItemText(hwnd, IDC_EDIT1, TextBuf);
			SetFocus(GetDlgItem(hwnd, IDC_EDIT1));
			SendMessage(GetDlgItem(hwnd, IDC_EDIT1), EM_SETSEL, 0, -1);
			break;
		}
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
				{
					char textbuf[30];
					GetDlgItemText(hwnd, IDC_EDIT1, textbuf, 30);
					double theGap = atof(textbuf);
					bool bEnd = IsDlgButtonChecked(hwnd, IDC_RADIO2) == BST_CHECKED;
					RepositionItems(theGap, bEnd);
					UpdateTimeline();
					Undo_OnStateChangeEx(__LOCALIZE("Reposition items", "sws_undo"), UNDO_STATE_ITEMS, -1);
					g_ReposItemsParams.Gap = theGap;
					g_ReposItemsParams.bEnd = bEnd;

					EndDialog(hwnd, 0);
					break;
				}
				case IDCANCEL:
					EndDialog(hwnd, 0);
					break;
			}
	}
	return 0;
}

void DoReposItemsDlg(COMMAND_T*)
{
	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_REPOSITEMS), g_hwndParent, ReposItemsDlgProc);
}

void DoSpeadSelItemsOverNewTx(COMMAND_T* ct)
{
	if (CountSelectedMediaItems(NULL) > 1)
	{
		for (int i = 0; i < CountTracks(NULL) ; ++i)
		{
			MediaTrack* track = GetTrack(NULL, i);
			int id = CSurf_TrackToID(track, false);
			int depth = *(int*)GetSetMediaTrackInfo(track, "I_FOLDERDEPTH", NULL);
			vector<MediaItem*> items = GetSelItems(track);

			bool found = false;
			for (int j =0; j < (int)items.size() ; ++j)
			{
				if (*(bool*)GetSetMediaItemInfo(items[j], "B_UISEL", NULL))
				{
					found = true;
					InsertTrackAtIndex(id,true); ++id;
					MoveMediaItemToTrack(items[j], CSurf_TrackFromID(id, false));
					++i; // needed for the first FOR loop since new tracks are getting added
				}
			}
			if (found && depth != 1) // make children out of newly created tracks
			{
				depth -= 1;
				GetSetMediaTrackInfo(CSurf_TrackFromID(id, false),"I_FOLDERDEPTH",&depth);
				depth = 1;
				GetSetMediaTrackInfo(track,"I_FOLDERDEPTH",&depth);
			}
		}
		TrackList_AdjustWindows(false);
		UpdateArrange();
		Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),UNDO_STATE_ALL,-1);
	}
	else
		MessageBox(g_hwndParent, __LOCALIZE("No or only one item selected!","sws_mbox"), __LOCALIZE("Xenakios - Error","sws_mbox"), MB_OK);

}

int OpenInExtEditor(int editorIdx)
{
	vector<MediaItem_Take*> TheTakes;
	XenGetProjectTakes(TheTakes,true,true);
	if (TheTakes.size()==1)
	{
		MediaItem* CurItem=(MediaItem*)GetSetMediaItemTakeInfo(TheTakes[0],"P_ITEM",NULL);

		//Main_OnCommand(40639,0); // duplicate active take
		Main_OnCommand(40601,0); // render items and add as new take
		//int curTakeIndex=*(int*)GetSetMediaItemInfo(CurItem,"I_CURTAKE",NULL);
		MediaItem_Take *CopyTake=GetMediaItemTake(CurItem,-1);
		PCM_source *ThePCM=(PCM_source*)GetSetMediaItemTakeInfo(CopyTake,"P_SOURCE",NULL);
		if (ThePCM && ThePCM->GetFileName())
		{
			char ExeString[2048];
			if (editorIdx==0 && g_external_app_paths.PathToAudioEditor1)
				snprintf(ExeString,sizeof(ExeString),"\"%s\" \"%s\"",g_external_app_paths.PathToAudioEditor1,ThePCM->GetFileName());
			else if (editorIdx==1 && g_external_app_paths.PathToAudioEditor2)
				snprintf(ExeString,sizeof(ExeString),"\"%s\" \"%s\"",g_external_app_paths.PathToAudioEditor2,ThePCM->GetFileName());
			DoLaunchExternalTool(ExeString);
		}
	}
	return -666;
}

void DoOpenInExtEditor1(COMMAND_T*) { OpenInExtEditor(0); }
void DoOpenInExtEditor2(COMMAND_T*) { OpenInExtEditor(1); }

void DoMatrixItemImplode(COMMAND_T* ct)
{
	vector<MediaItem*> TheItems;
	XenGetProjectItems(TheItems,true,false);
	vector<double> OldItemPositions;
	vector<double> OlddItemLens;
	int i;
	for (i=0;i<(int)TheItems.size();i++)
	{
		double itemPos=*(double*)GetSetMediaItemInfo(TheItems[i],"D_POSITION",NULL);
		OldItemPositions.push_back(itemPos);
		double dItemLen=*(double*)GetSetMediaItemInfo(TheItems[i],"D_LENGTH",NULL);
		OlddItemLens.push_back(dItemLen);
	}
	//Main_OnCommand(40289,0); // unselect all
	Undo_BeginBlock();
	Main_OnCommand(40543,0); // implode selected items to item
	Main_OnCommand(40698,0); // copy item
	vector<MediaItem*> PastedItems;
	vector<MediaItem*> TempItem; // oh fuck what shit this is
	for (i=1;i<(int)OldItemPositions.size();i++)
	{
		SetEditCurPos(OldItemPositions[i],false,false);
		Main_OnCommand(40058,0);
		XenGetProjectItems(TempItem,true,false); // basically a way to get the first selected item, messsyyyyy
		double newdItemLen=OlddItemLens[i];
		GetSetMediaItemInfo(TempItem[0],"D_LENGTH",&newdItemLen);
		//PastedItems.push_back(TempItem[0]);
	}
	/*
	for (i=0;i<PastedItems.size();i++)
	{
		double newdItemLen=OlddItemLens[i+1];
		GetSetMediaItemInfo(PastedItems[i],"D_LENGTH",&newdItemLen);
	}
	*/
	//40058 // paste
	Undo_EndBlock(SWS_CMD_SHORTNAME(ct),0);
}

double g_swingBase=1.0/4.0;
double g_swingAmt=0.2;

void PerformSwingItemPositions(double swingBase,double swingAmt)
{
	// 14th October 2009 (X)
	// too bad this doesn't work so well
	// maybe someone could make it work :)
	vector<MediaItem*> TheItems;
	XenGetProjectItems(TheItems,true,false);
	int i;
	//double temvo=TimeMap_GetDividedBpmAtTime(0.0);
	for (i=0;i<(int)TheItems.size();i++)
	{
		double itempos=*(double*)GetSetMediaItemInfo(TheItems[i],"D_POSITION",0);
		double itemposQN=TimeMap_timeToQN(itempos);
		int itemposSXTHN=(int)((itemposQN*4.0)+0.5);
		//double itemposinsixteenths=itemposQN*4.0;
		int oddOreven=itemposSXTHN % 2;
		double yay=fabs(swingAmt);
		double newitemposQN=itemposQN;
		if (swingAmt>0)
			newitemposQN=itemposQN+(1.0/4.0*yay); // to right
		if (swingAmt<0)
			newitemposQN=itemposQN-(1.0/4.0*yay); // to left
		itempos=TimeMap_QNToTime(newitemposQN);
		if (oddOreven==1)
			GetSetMediaItemInfo(TheItems[i],"D_POSITION",&itempos);
	}
	UpdateTimeline();
	Undo_OnStateChangeEx(__LOCALIZE("Swing item positions","sws_undo"),UNDO_STATE_ITEMS,-1);
}

WDL_DLGRET SwingItemsDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	char tbuf[314];

	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, Message, wParam, lParam))
		return r;

	if (Message==WM_INITDIALOG)
	{
		sprintf(tbuf,"%.2f",g_swingAmt*100.0);
		SetDlgItemText(hwnd,IDC_EDIT1,tbuf);
		return 0;
	}
	if (Message==WM_COMMAND && LOWORD(wParam)==IDOK)
	{
		GetDlgItemText(hwnd,IDC_EDIT1,tbuf,199);
		g_swingAmt=atof(tbuf)/100.0;
		if (g_swingAmt<-0.95) g_swingAmt=-0.95; // come on let's be sensible, why anyone would swing more, or even close to this!
		if (g_swingAmt>0.95) g_swingAmt=0.95;
		PerformSwingItemPositions(g_swingBase,g_swingAmt);
		EndDialog(hwnd,0);
		return 0;
	}
	if (Message==WM_COMMAND && LOWORD(wParam)==IDCANCEL)
	{
		EndDialog(hwnd,0);
		return 0;
	}
	return 0;
}

void DoSwingItemPositions(COMMAND_T*)
{
	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_SWINGITEMPOS), g_hwndParent, SwingItemsDlgProc);
}

void DoTimeSelAdaptDelete(COMMAND_T*)
{
	vector<MediaItem*> SelItems;
	vector<MediaItem*> ItemsInTimeSel;
	XenGetProjectItems(SelItems,true,true);
	double OldTimeSelLeft=0.0;
	double OldTimeSelRight=0.0;
	GetSet_LoopTimeRange(false,false,&OldTimeSelLeft,&OldTimeSelRight,false);
	int i;
	if (OldTimeSelRight-OldTimeSelLeft>0.0)
	{
		for (i=0;i<(int)SelItems.size();i++)
		{
			double itempos=*(double*)GetSetMediaItemInfo(SelItems[i],"D_POSITION",0);
			double dItemLen=*(double*)GetSetMediaItemInfo(SelItems[i],"D_LENGTH",0);
			bool itemsel=*(bool*)GetSetMediaItemInfo(SelItems[i],"B_UISEL",0);
			if (itemsel)
			{
				int intersectmatches=0;
				if (OldTimeSelLeft>=itempos && OldTimeSelRight<=itempos+dItemLen)
					intersectmatches++;
				if (itempos>=OldTimeSelLeft && itempos+dItemLen<=OldTimeSelRight)
					intersectmatches++;
				if (OldTimeSelLeft<=itempos+dItemLen && OldTimeSelRight>=itempos+dItemLen)
					intersectmatches++;
				if (OldTimeSelRight>=itempos && OldTimeSelLeft<itempos)
					intersectmatches++;
				if (intersectmatches>0)
					ItemsInTimeSel.push_back(SelItems[i]);
			}
		}
	}
	// 40312
	if (ItemsInTimeSel.size()>0)
	{
		Main_OnCommand(40312,0);
	}
	if (ItemsInTimeSel.size()==0)
	{
		Main_OnCommand(40006,0);
	}
}

void DoDeleteMutedItems(COMMAND_T* ct)
{
	Undo_BeginBlock();
	vector<MediaItem*> pitems;
	XenGetProjectItems(pitems,false,true);
	int i;
	for (i=0;i<(int)pitems.size();i++)
	{
		bool muted=*(bool*)GetSetMediaItemInfo(pitems[i],"B_MUTE",0);
		if (muted)
			DeleteTrackMediaItem (GetMediaItem_Track(pitems[i]), pitems[i]);
	}
	Undo_EndBlock(SWS_CMD_SHORTNAME(ct),UNDO_STATE_ITEMS);
	UpdateTimeline();
}
