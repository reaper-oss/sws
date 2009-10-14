/******************************************************************************
/ ItemTakeCommands.cpp
/
/ Copyright (c) 2009 Tim Payne (SWS), original code by Xenakios
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
#include "Parameters.h"

using namespace std;

int XenGetProjectItems(vector<MediaItem*>& TheItems,bool OnlySelectedItems, bool IncEmptyItems)
{
	TheItems.clear();
	MediaTrack *CurTrack;
	MediaItem *CurItem;
	int i;
	int j;
	for (i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		if (CurTrack!=0)
		{
			for (j=0;j<GetTrackNumMediaItems(CurTrack);j++)
			{
				CurItem=GetTrackMediaItem(CurTrack,j);
				if (CurItem!=0)
				{
					bool isSel=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
					int NumTakes=GetMediaItemNumTakes(CurItem);
					if (OnlySelectedItems)
					{
						if (isSel)
						{
							
							if (IncEmptyItems && NumTakes>=0) 
								TheItems.push_back(CurItem);
							if ((!IncEmptyItems) && NumTakes>0)
								TheItems.push_back(CurItem);
							
						}
						
					}
					if (!OnlySelectedItems)
					{
						if (IncEmptyItems && NumTakes==0)
							TheItems.push_back(CurItem);
						if (!IncEmptyItems && NumTakes>0)
							TheItems.push_back(CurItem);
					}
					
				}
			}
		}

	}
	return (int)TheItems.size();
}

int XenGetProjectTakes(vector<MediaItem_Take*>& TheTakes, bool OnlyActive,bool OnlyFromSelectedItems)
{
	MediaItem *CurItem;
	MediaItem_Take *CurTake;
	MediaTrack *CurTrack;
	TheTakes.clear();
	int i;
	int j;
	int k;
	for (i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		if (CurTrack!=0)
		{
			for (j=0;j<GetTrackNumMediaItems(CurTrack);j++)
			{
				CurItem=GetTrackMediaItem(CurTrack,j);
				if (CurItem!=0)
				{
					if (GetMediaItemNumTakes(CurItem)>0)
					{
						if (!OnlyFromSelectedItems || *(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL))
						{
							if (OnlyActive)
							{
								CurTake=0;
								CurTake=GetMediaItemTake(CurItem,-1);
								if (CurTake!=0)
									TheTakes.push_back(CurTake);
							} else
							{
								for (k=0;k<GetMediaItemNumTakes(CurItem);k++)
								{
									CurTake=0;
									CurTake=GetMediaItemTake(CurItem,k);
									if (CurTake!=0)
										TheTakes.push_back(CurTake);
								}
							}

						}
						if (!OnlyFromSelectedItems)
						{
							if (OnlyActive)
							{
								CurTake=0;
								CurTake=GetMediaItemTake(CurItem,-1);
								if (CurTake!=0)
									TheTakes.push_back(CurTake);
							} else
							{
								for (k=0;k<GetMediaItemNumTakes(CurItem);k++)
								{
									CurTake=0;
									CurTake=GetMediaItemTake(CurItem,k);
									if (CurTake!=0)
										TheTakes.push_back(CurTake);
								}
							}	
						}
						//if (OnlyFromSelectedItems	
					}
				}
			}
		}
	}
	return (int)TheTakes.size();
}

void DoMoveItemsLeftByItemLen(COMMAND_T*)
{
	MediaItem *CurItem;
	MediaTrack *CurTrack;
	vector<MediaItem*> VecItems;
	int i;
	int j;
	for (i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		for (j=0;j<GetTrackNumMediaItems(CurTrack);j++)
		{
			CurItem=GetTrackMediaItem(CurTrack,j);
			bool isSel=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (isSel) VecItems.push_back(CurItem);
		}
	}
	for (i=0;i<(int)VecItems.size();i++)
	{
		double CurPos=*(double*)GetSetMediaItemInfo(VecItems[i],"D_POSITION",NULL);
		double ItemLen=*(double*)GetSetMediaItemInfo(VecItems[i],"D_LENGTH",NULL);
		double NewPos=CurPos-ItemLen;
		GetSetMediaItemInfo(VecItems[i],"D_POSITION",&NewPos);
	}
	UpdateTimeline();
	Undo_OnStateChangeEx("Move item back by item length",4,-1);
}

int NumRepeatPasteRuns=0;

void DoToggleTakesNormalize(COMMAND_T*)
{
	//
	WDL_PtrList<MediaItem_Take> *TheTakes=new (WDL_PtrList<MediaItem_Take>);
	int NumActiveTakes=GetActiveTakes(TheTakes);
	int NumNormalizedTakes=0;
	int i;
	for (i=0;i<NumActiveTakes;i++)
	{
		//
		double TakeVol=*(double*)GetSetMediaItemTakeInfo(TheTakes->Get(i),"D_VOL",NULL);
		if (TakeVol!=1.0) NumNormalizedTakes++;
	}
	if (NumNormalizedTakes>0)
	{
		for (i=0;i<NumActiveTakes;i++)
		{
			//
			double TheGain=1.0;
			GetSetMediaItemTakeInfo(TheTakes->Get(i),"D_VOL",&TheGain);
		}
		Undo_OnStateChangeEx("Set Take(s) To Unity Gain",4,-1);
		UpdateTimeline();

	}
	else
	{
		Main_OnCommand(40108, 0);
		Undo_OnStateChangeEx("Normalize Take(s)",4,-1);
		UpdateTimeline();
	}

	delete TheTakes;
		
}


void DoSetVolPan(double Vol, double Pan, bool SetVol, bool SetPan)
{
	//
	vector<MediaItem_Take*> TheTakes;
	XenGetProjectTakes(TheTakes,true,true);
	double NewVol=Vol;
	double NewPan=Pan;
	int i;
	for (i=0;i<(int)TheTakes.size();i++)
	{
		if (SetVol) GetSetMediaItemTakeInfo(TheTakes[i],"D_VOL",&NewVol);
		if (SetPan) GetSetMediaItemTakeInfo(TheTakes[i],"D_PAN",&NewPan);

	}
	Undo_OnStateChangeEx("Set Take Volume And Pan",4,-1);
	UpdateTimeline();
	/*
	WDL_PtrList<MediaItem_Take> *TheTakes=new (WDL_PtrList<MediaItem_Take>);
	double NewVol=Vol;
	double NewPan=Pan;
	int NumActiveTakes=GetActiveTakes(TheTakes);
	int i;
	for (i=0;i<NumActiveTakes;i++)
	
	{
		//
		if (SetVol) GetSetMediaItemTakeInfo(TheTakes->Get(i),"D_VOL",&NewVol);
		if (SetPan) GetSetMediaItemTakeInfo(TheTakes->Get(i),"D_PAN",&NewPan);
	}
	Undo_OnStateChangeEx("Set Take Volume And Pan",4,-1);
	UpdateTimeline();
	delete TheTakes;
	*/
}


void GetDialogItemString(HWND DialogHandle, int DialogItem, WDL_String *TheString)
{
	//
	int len = GetWindowTextLength(GetDlgItem(DialogHandle, DialogItem));
	if(len > 0)
	{
				
		char* TempString;
		//TempString=(char*)GlobalAlloc(GPTR, len + 1);
		TempString=new char[len+1];
		GetDlgItemText(DialogHandle, DialogItem, TempString, len + 1);
		TheString->Set(TempString);
		
		//GlobalFree((HANDLE)TempString);
		delete[] TempString;
	} else TheString->Set("");

}

void DoSetItemVols(double theVol)
{
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	
	int numItems;;
	
	
	bool ItemSelected=false;
	
	bool NewSelectedStatus=false;
	int i;
	int j;
	for (i=0;i<GetNumTracks();i++)
	{
		MunRaita = CSurf_TrackFromID(i+1,FALSE);
		numItems=GetTrackNumMediaItems(MunRaita);
		for (j=0;j<numItems;j++)
		{
			CurItem = GetTrackMediaItem(MunRaita,j);
			//propertyName="D_";
			ItemSelected=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (ItemSelected==TRUE)
			{
				double NewVolume=theVol;
				GetSetMediaItemInfo(CurItem,"D_VOL",&NewVolume);
				
			} 


		}
	}
	Undo_OnStateChangeEx("Set Item Volume",4,-1);
	UpdateTimeline();
}


WDL_DLGRET ItemSetVolDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
    {
        case WM_INITDIALOG:
			{	
				SetDlgItemText(hwnd, IDC_VOLEDIT, "0.0");
#ifdef _WIN32
				SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwnd, IDC_VOLEDIT), TRUE);
#endif
				return 0;
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
					return 0;
					}
				case IDCANCEL:
					//MessageBox(hwnd,"Cancel pressed","info",MB_OK);
					EndDialog(hwnd,0);
					return 0;
			}
	}
	return 0;
}	

void DoShowItemVolumeDialog(COMMAND_T*)
{
	//
	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ITEMVOLUME), g_hwndParent, ItemSetVolDlgProc);
}

WDL_DLGRET ItemPanVolDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	//
	switch(Message)
    {
        case WM_INITDIALOG:
			
			SetDlgItemText(hwnd, IDC_VOLEDIT, "");
			SetDlgItemText(hwnd, IDC_PANEDIT, "");
#ifdef _WIN32
			SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwnd, IDC_VOLEDIT), TRUE);
#endif
        return 0;
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
					
					{
					
					char TempString[100];
					//TempString=new WDL_String;
					GetDlgItemText(hwnd,IDC_VOLEDIT,TempString,99);
					//char stringbuf[32];
					int VPFlag=0;
					bool SetVol=FALSE;
					bool SetPan=FALSE;
					double NewVol=strtod(TempString,NULL);
					
					if (strcmp(TempString,"")!=0)
					{
						if (NewVol>-144.0)
							NewVol=exp(NewVol*0.115129254);
								else NewVol=0;
						SetVol=TRUE;
					} 
					
					GetDlgItemText (hwnd,IDC_PANEDIT,TempString,99);
					double NewPan=strtod(TempString,NULL);
					if (strcmp(TempString,"")!=0) 
						SetPan=TRUE;
						
					DoSetVolPan(NewVol,NewPan/100.0,SetVol,SetPan);
					
					//
					
					EndDialog(hwnd,0);
					return 0;
					}
				case IDCANCEL:
					//MessageBox(hwnd,"Cancel pressed","info",MB_OK);
					EndDialog(hwnd,0);
					return 0;
			}
	}
	return 0;
}

int LastNumRepeats=1;
double LastTimeIntervalForPaste=1.0;
double LastBeatIntervalForPaste=1.0;
int LastRepeatMode=0;

int PasteMultipletimes(int NumPastes,double TimeIntervalQN,int RepeatMode)
{
	//
	//TimeMap_QNToTime
	if (NumPastes>0)
	{
	if (RepeatMode>0)
	{
	Undo_BeginBlock();
	
	double TimeAccum=GetCursorPosition();
	double BeatAccum=TimeMap_timeToQN(TimeAccum);
	double OriginTimeBeats=BeatAccum;
	double OriginTimeSecs=TimeAccum;
	int i;
	for (i=0;i<NumPastes;i++)
	{
		//
		//if (i>0)
		//{
			if (RepeatMode==1)
			{
				SetEditCurPos(TimeAccum, FALSE, FALSE);
				Main_OnCommand(40058,0); // Paste
				TimeAccum=OriginTimeSecs+TimeIntervalQN*(1+i);
			}
			if (RepeatMode==2)
			{
				SetEditCurPos(TimeMap_QNToTime(BeatAccum), FALSE, FALSE);
				Main_OnCommand(40058,0); // Paste
				BeatAccum=OriginTimeBeats+TimeIntervalQN*(1+i);
			}
		//}
	}
	Undo_EndBlock("Repeat Paste",0);
	}
	}
	if (RepeatMode==0)
	{
		//
		Undo_BeginBlock();
	
	double TimeAccum=GetCursorPosition();
	double OriginTime=TimeAccum;
	double BeatAccum=TimeMap_timeToQN(TimeAccum);
	SetEditCurPos(TimeAccum, FALSE, FALSE);
	// this a hack to get the length of pasted data
	Main_OnCommand(40058,0); // Paste
	double OldTimeSelLeft;
	double OldTimeSelRight;
	GetSet_LoopTimeRange(false,false,&OldTimeSelLeft,&OldTimeSelRight,false);
	Main_OnCommand(40290,0); // set time selection to selected items
	double NewTimeSelLeft;
	double NewTimeSelRight;
	GetSet_LoopTimeRange(false,false,&NewTimeSelLeft,&NewTimeSelRight,false);
	TimeIntervalQN=NewTimeSelRight-NewTimeSelLeft;
	GetSet_LoopTimeRange(true,false,&OldTimeSelLeft,&OldTimeSelRight,false);
	int i;
	for (i=1;i<NumPastes;i++)
	{
		//
		//if (i>0)
		//{
			TimeAccum=OriginTime+TimeIntervalQN*i;
			SetEditCurPos(TimeAccum, FALSE, FALSE);
			Main_OnCommand(40058,0); // Paste
			
		//}
	}
	Undo_EndBlock("Repeat Paste",0);
	}

	return -666;
}


// TODO check mem alloc on JokuBuf!
char *JokuBuf;

WDL_DLGRET RepeatPasteDialogProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
    {
		case WM_INITDIALOG:
			{
			char TextBuf[32];
			//dtostr(
			sprintf(TextBuf,"%.2f",LastTimeIntervalForPaste);
			
			SetDlgItemText(hwnd, IDC_EDIT1, TextBuf);
			sprintf(TextBuf,"%d",LastNumRepeats);
			SetDlgItemText(hwnd, IDC_NUMREPEDIT, TextBuf);
#ifdef _WIN32
			SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwnd, IDC_NUMREPEDIT), TRUE);
#endif
			
			//WDL_String *TempString;
			//TempString=new WDL_String;
			//GetDialogItemString(hwnd,IDC_NOTEVALUECOMBO,TempString);
			//if (NumRepeatPasteRuns==0) JokuBuf="1"; else
				//JokuBuf=_strdup(TempString->Get());
			//delete TempString;
			InitFracBox(GetDlgItem(hwnd, IDC_NOTEVALUECOMBO),JokuBuf);
			
			//if (LastRepeatMode==0) Button_SetCheck(GetDlgItem(hwnd, IDC_RADIO1),BST_CHECKED);
			//if (LastRepeatMode==1) Button_SetCheck(GetDlgItem(hwnd, IDC_RADIO2),BST_CHECKED);
			//if (LastRepeatMode==2) Button_SetCheck(GetDlgItem(hwnd, IDC_RADIO3),BST_CHECKED);
			if (LastRepeatMode==0) CheckDlgButton(hwnd,IDC_RADIO1,BST_CHECKED);
			if (LastRepeatMode==1) CheckDlgButton(hwnd,IDC_RADIO2,BST_CHECKED);
			if (LastRepeatMode==2) CheckDlgButton(hwnd,IDC_RADIO3,BST_CHECKED);
			return 0;
			}
		case WM_COMMAND:
            switch(LOWORD(wParam))
            {
				case IDOK:
				{
				WDL_String *TempString;
				TempString=new WDL_String;
				GetDialogItemString(hwnd,IDC_NUMREPEDIT,TempString);
				int NumRepeats=atoi(TempString->Get());
				LRESULT Nappi1=Button_GetState(GetDlgItem(hwnd, IDC_RADIO1));
				LRESULT Nappi2=Button_GetState(GetDlgItem(hwnd, IDC_RADIO2));
				LRESULT Nappi3=Button_GetState(GetDlgItem(hwnd, IDC_RADIO3));
				if (Nappi1==BST_CHECKED) LastRepeatMode=0;
				if (Nappi2==BST_CHECKED) LastRepeatMode=1;
				if (Nappi3==BST_CHECKED) LastRepeatMode=2;
				double TimeInterval;
				double BeatInterval;
				if (LastRepeatMode==2)
				{
				
					GetDialogItemString(hwnd,IDC_NOTEVALUECOMBO,TempString);
					BeatInterval=parseFrac(TempString->Get());
					if (BeatInterval!=1.0) 
					{
						if (BeatInterval!=0)
							JokuBuf=_strdup(TempString->Get()); 
						else JokuBuf="1";
					}
						else JokuBuf="1";
				//double TimeInterval=strtod(TempString->Get(),NULL);
				//int ComboSelected=SendMessage(GetDlgItem(hwnd, IDC_NOTEVALUECOMBO),CB_GETCURSEL,0,0);
				//combobox_getText
					PasteMultipletimes(NumRepeats,BeatInterval,LastRepeatMode);
					LastBeatIntervalForPaste=BeatInterval;
					LastBeatIntervalForPaste=BeatInterval;
				}
				if (LastRepeatMode==1)
				{
					GetDialogItemString(hwnd,IDC_EDIT1,TempString);
					TimeInterval=atof(TempString->Get());
					PasteMultipletimes(NumRepeats,TimeInterval,LastRepeatMode);
					LastTimeIntervalForPaste=TimeInterval;
				}
				if (LastRepeatMode==0)
				{
					PasteMultipletimes(NumRepeats,TimeInterval,LastRepeatMode);
					TimeInterval=1.0;
				}
				LastNumRepeats=NumRepeats;
				
				
				delete TempString;
				EndDialog(hwnd,0);
				return 0;
				}
				case IDCANCEL:
					
				//delete JokuBuf;
				EndDialog(hwnd,0);
				return 0;
		}
	}
	return 0;
}


void DoShowVolPanDialog(COMMAND_T*)
{
	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ITEMPANVOLDIALOG), g_hwndParent, ItemPanVolDlgProc);
}

void DoChooseNewSourceFileForSelTakes(COMMAND_T*)
{
	//
	// SysListView321
	//HWND ReaperWindow=FindWindow("REAPERwnd",NULL);
	//HWND MediaExplorer=FindWindowEx(ReaperWindow,NULL,"SysListView321",NULL);
	//HWND MediaExplorerList=FindWindowEx (MediaExplorer,0,"SysListView321","");
	//if (MediaExplorer!=0) MessageBox(g_hwndParent,"Media Explorer Found!","info",MB_OK);
	WDL_PtrList<CHAR>* ReplaceFileNames;
	ReplaceFileNames=new(WDL_PtrList<CHAR>);
	//g_filenames->Empty(TRUE,free);
	
	
	WDL_PtrList<MediaItem_Take> *TheTakes=new (WDL_PtrList<MediaItem_Take>);
	PCM_source *ThePCMSource;
	int i;
	int NumActiveTakes=GetActiveTakes(TheTakes);
	if (NumActiveTakes>0)
	{
		BrowseForFiles(g_hwndParent,ReplaceFileNames,"Media Files\0*.wav;*.wv;*.ogg;*.mp3;*.aif;*.aiff;*.flac\0",NULL,FALSE,NULL,NULL);
		MediaItem_Take* CurTake;
		if (ReplaceFileNames->GetSize()>0)
		{
			Main_OnCommand(40440,0); // Selected Media Offline
			for (i=0;i<NumActiveTakes;i++)
			{
				CurTake=TheTakes->Get(i);
				ThePCMSource=(PCM_source*)GetSetMediaItemTakeInfo(CurTake,"P_SOURCE",NULL);
			
					//ThePCMSource->SetFileName(ReplaceFileNames->Get(0));
					if (strcmp(ThePCMSource->GetType(),"SECTION")!=0)
					{
						PCM_source *NewPCMSource=PCM_Source_CreateFromFile(ReplaceFileNames->Get(0));
						if (NewPCMSource!=0)
						{
							GetSetMediaItemTakeInfo(CurTake,"P_SOURCE",NewPCMSource);
							delete ThePCMSource;
						}
					} else
					{
						PCM_source *TheOtherPCM=ThePCMSource->GetSource();
						if (TheOtherPCM!=0)
						{
							PCM_source *NewPCMSource=PCM_Source_CreateFromFile(ReplaceFileNames->Get(0));
							ThePCMSource->SetSource(NewPCMSource);
							delete TheOtherPCM;
						}
					}

					
					
				
				//ThePCMSource->~PCM_source();
			//ThePCMSource->
		
			}
			Main_OnCommand(40047,0); // Build any missing peaks
			Main_OnCommand(40439,0); // Selected Media Online
			Undo_OnStateChangeEx("Replace Takes Source Files",4,-1);
			UpdateTimeline();
		}
	}
	delete TheTakes;
	ReplaceFileNames->Empty(true, free);
	delete ReplaceFileNames;
}

void DoInvertItemSelection(COMMAND_T*)
{
	//
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	
	int numItems;;
	
	
	bool ItemSelected=false;
	
	bool NewSelectedStatus=false;
	int i;
	int j;
	for (i=0;i<GetNumTracks();i++)
	{
		MunRaita = CSurf_TrackFromID(i+1,FALSE);
		numItems=GetTrackNumMediaItems(MunRaita);
		for (j=0;j<numItems;j++)
		{
			CurItem = GetTrackMediaItem(MunRaita,j);
			//propertyName="D_";
			ItemSelected=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (ItemSelected==TRUE)
			{
				//int Dummy=0;
				//GetSetMediaItemInfo(CurItem,"I_GROUPID",&Dummy);
				
				NewSelectedStatus=FALSE;
				GetSetMediaItemInfo(CurItem,"B_UISEL",&NewSelectedStatus);
				
				
			} else
			{
				NewSelectedStatus=TRUE;
				GetSetMediaItemInfo(CurItem,"B_UISEL",&NewSelectedStatus);	
				int Dummy=0;
				//GetSetMediaItemInfo(CurItem,"I_GROUPID",&Dummy);
			}


		}
	}
	UpdateTimeline();
}

bool DoLaunchExternalTool(const char *ExeFilename)
{
	if (!ExeFilename)
		return false;

	STARTUPINFO          si = { sizeof(si) };
	PROCESS_INFORMATION  pi;
	char* cFile = _strdup(ExeFilename);
	bool bRet = CreateProcess(0, cFile, 0, 0, FALSE, 0, 0, 0, &si, &pi) ? true : false;
	free(cFile);
	return bRet;
}

void DoRepeatPaste(COMMAND_T*)
{
	if (NumRepeatPasteRuns==0)
	{
		JokuBuf="1";
	}
	
	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_REPEATPASTEDLG), g_hwndParent, RepeatPasteDialogProc);
	NumRepeatPasteRuns++;
}

void DoSelectEveryNthItemOnSelectedTracks(int Step,int ItemOffset)
{
	//
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	
	int numItems;;
	
	Main_OnCommand(40289,0); // Unselect all items
	bool ItemSelected=false;
	
	bool NewSelectedStatus=false;
	int flags;
	int i;
	int j;
	for (i=0;i<GetNumTracks();i++)
	{
		GetTrackInfo(i,&flags);
		if (flags & 0x02)
		{ 
			MunRaita = CSurf_TrackFromID(i+1,FALSE);
			int ItemCounter=0;
			numItems=GetTrackNumMediaItems(MunRaita);
			for (j=0;j<numItems;j++)
			{
				CurItem = GetTrackMediaItem(MunRaita,j);
				
				if ((ItemCounter % Step)==ItemOffset)
				{
					//
					NewSelectedStatus=TRUE;
					GetSetMediaItemInfo(CurItem,"B_UISEL",&NewSelectedStatus);
				} else
				{
					NewSelectedStatus=FALSE;
					GetSetMediaItemInfo(CurItem,"B_UISEL",&NewSelectedStatus);	
				}

				ItemCounter++;
				


		}
		}
	}
	UpdateTimeline();

}

void DoSelectSkipSelectOnSelectedItems(int Step,int ItemOffset)
{
	//
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	
	int numItems;;
	
	//Main_OnCommand(40289,0); // Unselect all items
	bool ItemSelected=false;
	
	bool NewSelectedStatus=false;
	int flags; 
	int i;
	int j;
	for (i=0;i<GetNumTracks();i++)
	{
		GetTrackInfo(i,&flags);
		//if (flags & 0x02)
		if (TRUE==TRUE)
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
					//
					NewSelectedStatus=TRUE;
					GetSetMediaItemInfo(CurItem,"B_UISEL",&NewSelectedStatus);
				} else
				{
					NewSelectedStatus=FALSE;
					GetSetMediaItemInfo(CurItem,"B_UISEL",&NewSelectedStatus);	
				}

				ItemCounter++;
				}


		}
		}
	}
	UpdateTimeline();
}


int NumSteps=2;
int StepOffset=1;
int SkipItemSource=0; // 0 for all in selected tracks, 1 for items selected in selected tracks

WDL_DLGRET SelEveryNthDialogProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	//
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
			SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwnd, IDC_EDIT1), TRUE);
			return 0;
			}
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
					{
						//MessageBox(g_hwndParent,"LoL","info",MB_OK);
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
	//
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
	//
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
		//
		GoodFound=FALSE;
		while (!GoodFound)
		{
			rndInt=rand() % numItems;
			if ((CheckTable[rndInt]==0) && (rndInt!=badFirstNumber) && (i==0)) GoodFound=TRUE;
			if ((CheckTable[rndInt]==0) && (i>0)) GoodFound=TRUE;
			
			
			IterCount++;
			if (IterCount>1000000) 
			{
				MessageBox(g_hwndParent,"Shuffle Random Table Generator Probably Failed, over 1000000 iterations!","Error",MB_OK);
				break;
			}
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

void DoShuffleSelectTakesInItems(COMMAND_T*)
{
	//
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	
	int numItems;;
	
	
	bool ItemSelected=false;
	
	bool NewSelectedStatus=false;
	
	int ValidNumTakesInItems=0;
	int NumSelectedItemsFound=0;
	int TestNumTakes=0;
	bool ValidNumTakes=false;
	int i;
	int j;
	for (i=0;i<GetNumTracks();i++)
	{
		MunRaita = CSurf_TrackFromID(i+1,FALSE);
		numItems=GetTrackNumMediaItems(MunRaita);
		for (j=0;j<numItems;j++)
		{
			CurItem = GetTrackMediaItem(MunRaita,j);
			//propertyName="D_";
			ItemSelected=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (ItemSelected==TRUE)
			{
				if (NumSelectedItemsFound==0) 
				{
					ValidNumTakesInItems=GetMediaItemNumTakes(CurItem);
				}
				if (NumSelectedItemsFound>0)
				{
					TestNumTakes=GetMediaItemNumTakes(CurItem);
					if (TestNumTakes!=ValidNumTakesInItems)
					{
						ValidNumTakes=false;
						break;
					} else ValidNumTakes=true;
				}

				NumSelectedItemsFound++;

				//NewSelectedStatus=FALSE;
				//GetSetMediaItemInfo(CurItem,"B_UISEL",&NewSelectedStatus);
				
			}

			


		}
	}
	int TakeToChoose=0;
	if (ValidNumTakes==false) MessageBox(g_hwndParent,"Non-valid number of takes in items!","Error",MB_OK);
	
	if (ValidNumTakes==true)
	{
		//
		TakeIndexes=new int[1024];
		GenerateShuffledTakeRandomTable(TakeIndexes,(ValidNumTakesInItems-0),-1);
		//MessageBox(g_hwndParent,"first take shuffler ok!","info",MB_OK);
		int ShuffledTakesGenerated=0;
		for (i=0;i<GetNumTracks();i++)
		{
			MunRaita = CSurf_TrackFromID(i+1,FALSE);
			numItems=GetTrackNumMediaItems(MunRaita);
			for (j=0;j<numItems;j++)
			{
				CurItem = GetTrackMediaItem(MunRaita,j);
				//propertyName="D_";
				ItemSelected=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
				if (ItemSelected==TRUE)
				{
					// set the take etc
					TakeToChoose=TakeIndexes[ShuffledTakesGenerated];
					if (GetMediaItemNumTakes(CurItem)>0)
					{
						GetSetMediaItemInfo(CurItem,"I_CURTAKE",&TakeToChoose);
						ShuffledTakesGenerated++;
						if (ShuffledTakesGenerated==ValidNumTakesInItems)
						{
						//
							GenerateShuffledTakeRandomTable(TakeIndexes,ValidNumTakesInItems,TakeToChoose);
							ShuffledTakesGenerated=0;
						}
					}
				}
			}
		}
		delete[] TakeIndexes;
		Undo_OnStateChangeEx("Select Takes In Selected Items, Shuffled Random",4,-1);
		UpdateTimeline();
	}
	
}

void DoMoveItemsToEditCursor(COMMAND_T*)
{
	//
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	
	int numItems;;
	
	
	bool ItemSelected=false;
	
	bool NewSelectedStatus=false;
	double EditCurPos=GetCursorPosition();
	
	//MediaItem** MediaItemsOnTrack;
	int i;
	int j;
	for (i=0;i<GetNumTracks();i++)
	{
		MunRaita = CSurf_TrackFromID(i+1,FALSE);
		numItems=GetTrackNumMediaItems(MunRaita);
		//MediaItem* **MediaItemsOnTrack = new (MediaItem*)[numItems];
		MediaItem** MediaItemsOnTrack = new MediaItem*[numItems];

		for (j=0;j<numItems;j++)
		{
			CurItem = GetTrackMediaItem(MunRaita,j);
			MediaItemsOnTrack[j]=CurItem;
		}
		for (j=0;j<numItems;j++)
		{
			ItemSelected=*(bool*)GetSetMediaItemInfo(MediaItemsOnTrack[j],"B_UISEL",NULL);
			if (ItemSelected==TRUE)
			{
				double SnapOffset=*(double*)GetSetMediaItemInfo(CurItem,"D_SNAPOFFSET",NULL);
				double NewPos=EditCurPos-SnapOffset;
				double OldPos=*(double*)GetSetMediaItemInfo(CurItem,"D_POSITION",NULL);
				//if (OldPos!=NewPos)
				//{					
					GetSetMediaItemInfo(MediaItemsOnTrack[j],"D_POSITION",&NewPos);
					//j=0;
				//}
				
			} 


		}
	delete[] MediaItemsOnTrack;

	}
	Undo_OnStateChangeEx("Move Selected Items To Edit Cursor",4,-1);
	UpdateTimeline();
}

void DoRemoveItemFades(COMMAND_T*)
{
	//
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	
	int numItems;;
	
	
	bool ItemSelected=false;
	
	bool NewSelectedStatus=false;
	int i;
	int j;
	for (i=0;i<GetNumTracks();i++)
	{
		MunRaita = CSurf_TrackFromID(i+1,FALSE);
		numItems=GetTrackNumMediaItems(MunRaita);
		for (j=0;j<numItems;j++)
		{
			CurItem = GetTrackMediaItem(MunRaita,j);
			//propertyName="D_";
			ItemSelected=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (ItemSelected==TRUE)
			{
				double NewFadeTime=0;
				GetSetMediaItemInfo(CurItem,"D_FADEINLEN",&NewFadeTime);
				GetSetMediaItemInfo(CurItem,"D_FADEOUTLEN",&NewFadeTime);
				
			} 

		}
	}
	Undo_OnStateChangeEx("Set Item Fades To 0",4,-1);
	UpdateTimeline();
	
}


void DoTrimLeftEdgeToEditCursor(COMMAND_T*)
{
	//
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	MediaItem_Take* CurTake;
	int numItems;;
	
	
	bool ItemSelected=false;
	
	bool NewSelectedStatus=false;
	double EditcurPos=GetCursorPosition();
	int i;
	int j;
	int k;
	for (i=0;i<GetNumTracks();i++)
	{
		MunRaita = CSurf_TrackFromID(i+1,FALSE);
		numItems=GetTrackNumMediaItems(MunRaita);
		for (j=0;j<numItems;j++)
		{
			CurItem = GetTrackMediaItem(MunRaita,j);
			//propertyName="D_";
			ItemSelected=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (ItemSelected==TRUE)
			{
				double OldLeftEdge=*(double*)GetSetMediaItemInfo(CurItem,"D_POSITION",NULL);
				double OldLength=*(double*)GetSetMediaItemInfo(CurItem,"D_LENGTH",NULL);
				double NewLeftEdge=EditcurPos;
				double NewLength=OldLength+(OldLeftEdge-NewLeftEdge);
				for (k=0;k<GetMediaItemNumTakes(CurItem);k++)
				{
					CurTake=GetMediaItemTake(CurItem,k);
					double OldMediaOffset=*(double*)GetSetMediaItemTakeInfo(CurTake,"D_STARTOFFS",NULL);
					double NewMediaOffset=OldMediaOffset-(OldLeftEdge-NewLeftEdge);
					GetSetMediaItemTakeInfo(CurTake,"D_STARTOFFS",&NewMediaOffset);
				}

				GetSetMediaItemInfo(CurItem,"D_POSITION",&NewLeftEdge);
				GetSetMediaItemInfo(CurItem,"D_LENGTH",&NewLength);
				
			} 

		}
	}
	Undo_OnStateChangeEx("Trim/Untrim Item Left Edge",4,-1);
	UpdateTimeline();
}

void DoTrimRightEdgeToEditCursor(COMMAND_T*)
{
	//
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	int numItems;;
	
	bool ItemSelected=false;
	
	bool NewSelectedStatus=false;
	double EditcurPos=GetCursorPosition();
	int i;
	int j;
	for (i=0;i<GetNumTracks();i++)
	{
		MunRaita = CSurf_TrackFromID(i+1,FALSE);
		numItems=GetTrackNumMediaItems(MunRaita);
		for (j=0;j<numItems;j++)
		{
			CurItem = GetTrackMediaItem(MunRaita,j);
			//propertyName="D_";
			ItemSelected=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (ItemSelected==TRUE)
			{
				double OldPos=*(double*)GetSetMediaItemInfo(CurItem,"D_POSITION",NULL);
				double OldLength=*(double*)GetSetMediaItemInfo(CurItem,"D_LENGTH",NULL);
				double OldRightEdge=OldPos+OldLength;
				double NewRightEdge=EditcurPos;
				double NewLength=NewRightEdge-OldPos;
				//GetSetMediaItemInfo(CurItem,"D_POSITION",&NewLeftEdge);
				GetSetMediaItemInfo(CurItem,"D_LENGTH",&NewLength);
				
			} 

		}
	}
	Undo_OnStateChangeEx("Trim/Untrim Item Right Edge",4,-1);
	UpdateTimeline();
}

void DoResetItemRateAndPitch(COMMAND_T*)
{
	//
	Undo_BeginBlock();
	Main_OnCommand(40652, 0); // set item rate to 1.0
	Main_OnCommand(40653, 0); // reset item pitch to 0.0
	Undo_EndBlock("Reset Item Pitch And Rate",0);
}

void DoApplyTrackFXStereoAndResetVol(COMMAND_T*)
{
	//
	Undo_BeginBlock();
	Main_OnCommand(40209,0); // apply track fx in stereo to items
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	
	int numItems;;
	
	
	bool ItemSelected=false;
	
	bool NewSelectedStatus=false;
	double EditcurPos=GetCursorPosition();
	int i;
	int j;
	for (i=0;i<GetNumTracks();i++)
	{
		MunRaita = CSurf_TrackFromID(i+1,FALSE);
		numItems=GetTrackNumMediaItems(MunRaita);
		for (j=0;j<numItems;j++)
		{
			CurItem = GetTrackMediaItem(MunRaita,j);
			//propertyName="D_";
			ItemSelected=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (ItemSelected==TRUE)
			{
				double NewVol=1.0;
				GetSetMediaItemInfo(CurItem,"D_VOL",&NewVol);
				
			} 

		}
	}
	Undo_EndBlock("Apply Track FX To Items And Reset Volume",0);
}

void DoApplyTrackFXMonoAndResetVol(COMMAND_T*)
{
	Undo_BeginBlock();
	Main_OnCommand(40361,0); // apply track fx in mono to items
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	
	int numItems;;
	
	
	bool ItemSelected=false;
	
	bool NewSelectedStatus=false;
	double EditcurPos=GetCursorPosition();	
	int i;
	int j;
	for (i=0;i<GetNumTracks();i++)
	{
		MunRaita = CSurf_TrackFromID(i+1,FALSE);
		numItems=GetTrackNumMediaItems(MunRaita);
		for (j=0;j<numItems;j++)
		{
			CurItem = GetTrackMediaItem(MunRaita,j);
			//propertyName="D_";
			ItemSelected=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (ItemSelected==TRUE)
			{
				double NewVol=1.0;
				GetSetMediaItemInfo(CurItem,"D_VOL",&NewVol);
				
			} 

		}
	}
	Undo_EndBlock("Apply Track FX To Items (Mono) And Reset Volume",0);
}

int AnalyzePCMSourceForPeaks(PCM_source *APCMSource,double *PeakLeft,double *PeakRight,double ItemLen,double MediaStart,double *RMSLeft,double *RMSRight)
{
	//
	int AudioBufLenFrames=1024;
	double LenFile=APCMSource->GetLength();
	LenFile=ItemLen;
	int NumChans=APCMSource->GetNumChannels();
	double SampleRate=APCMSource->GetSampleRate();
	double *BufL=new double[AudioBufLenFrames];
	double *BufR=new double[AudioBufLenFrames];
	double *AudioBuffer = new double[AudioBufLenFrames*NumChans];
	double sumSquaresL=0.0;
	double sumSquaresR=0.0;
	PCM_source_transfer_t TransferBlock={0,};
	
	TransferBlock.length=AudioBufLenFrames;
	TransferBlock.time_s=MediaStart;
	TransferBlock.samplerate=SampleRate;
	TransferBlock.samples=AudioBuffer;
	TransferBlock.nch=NumChans;
	//APCMSource->
	int NumFramesInFile=(int)(LenFile*SampleRate);
	int NumBlocksToProcess=NumFramesInFile/AudioBufLenFrames;
	INT64 SampleCounter=0;
	double TempMaxLeft=-666;
	double TempMaxRight=0.0;
	int j;
	//for (i=0;i<NumBlocksToProcess;i++)
	//while ((TransferBlock.time_s-MediaStart+(TransferBlock.length/(double)SampleRate))<=LenFile)
	while (SampleCounter+TransferBlock.length<NumFramesInFile)
	{
		//
		APCMSource->GetSamples(&TransferBlock);	
		//dpos = floor(dpos*sr + bsize + 0.5)/(double)sr;
		TransferBlock.time_s+=TransferBlock.length/(double)SampleRate;
		int BufSamplesToProcess=AudioBufLenFrames;
		if (TransferBlock.time_s>ItemLen+MediaStart)
		{
			BufSamplesToProcess=(int)((((TransferBlock.time_s-MediaStart))-(ItemLen))*SampleRate);
			if (BufSamplesToProcess<0) BufSamplesToProcess=0;
			if (BufSamplesToProcess>AudioBufLenFrames) BufSamplesToProcess=AudioBufLenFrames;
		}
		//TransferBlock.time_s+=floor(TransferBlock.time_s*SampleRate+AudioBufLenFrames+0.5)/(double)SampleRate;
		if (NumChans==1)
		{
			//if (i==0) MessageBox(g_hwndParent,"it's mono","info",MB_OK);
			for (j=0;j<BufSamplesToProcess;j++)
			{
				//
				SampleCounter++;
				double SampleValue=fabs(AudioBuffer[j]);
				sumSquaresL+=AudioBuffer[j]*AudioBuffer[j];
				if (SampleValue>TempMaxLeft) TempMaxLeft=SampleValue;
			}
		}
		
		  
		if (NumChans==2)
		{
			//if (i==0) MessageBox(g_hwndParent,"it's stereo","info",MB_OK);
			for (j=0;j<BufSamplesToProcess;j++)
			{
				//
				SampleCounter++;
				double SampleValue=fabs(AudioBuffer[j*2]);
				sumSquaresL+=AudioBuffer[j*2]*AudioBuffer[j*2];
				sumSquaresR+=AudioBuffer[j*2+1]*AudioBuffer[j*2+1];
				if (SampleValue>TempMaxLeft) TempMaxLeft=SampleValue;
				SampleValue=fabs(AudioBuffer[j*2+1]);
				if (SampleValue>TempMaxRight) TempMaxRight=SampleValue;
			}
		}
		

	}
	*PeakLeft=TempMaxLeft;
	*PeakRight=TempMaxRight;
	*RMSLeft=sqrt(sumSquaresL/SampleCounter);
	*RMSRight=sqrt(sumSquaresR/SampleCounter);
	delete[] AudioBuffer;
	delete[] BufL;
	delete[] BufR;
	return 777;
}


void DoAnalyzeAndShowPeakInItemMedia(COMMAND_T*)
{
	WDL_PtrList<MediaItem_Take> *TheTakes=new (WDL_PtrList<MediaItem_Take>);
	PCM_source *ThePCMSource;
	int NumActiveTakes=GetActiveTakes(TheTakes);
	int i;
	if (NumActiveTakes>0)
	{
		MediaItem_Take* CurTake;
		MediaItem* CurItem;
		for (i=0;i<NumActiveTakes;i++)
		{
			CurTake=TheTakes->Get(i);
			ThePCMSource=(PCM_source*)GetSetMediaItemTakeInfo(CurTake,"P_SOURCE",NULL);
			if (strcmp(ThePCMSource->GetType(),"MIDI")!=0)
			{
				PCM_source *DuplPCMSource=0;
				DuplPCMSource=ThePCMSource->Duplicate();
				if (DuplPCMSource!=NULL)
				{
					double PeakLeft=69;
					double PeakRight=69;
					double rmsL=-666.0;
					double rmsR=666.0;
					CurItem=(MediaItem*)GetSetMediaItemTakeInfo(CurTake,"P_ITEM",NULL);
					double ItemLen=*(double*)GetSetMediaItemInfo(CurItem,"D_LENGTH",NULL);
					double MediaOffset=*(double*)GetSetMediaItemTakeInfo(CurTake,"D_STARTOFFS",NULL);
					int joops=AnalyzePCMSourceForPeaks(DuplPCMSource,&PeakLeft,&PeakRight,ItemLen,MediaOffset,&rmsL,&rmsR);
					char MesBuf[256];
					double PeakLeftDB=20.0*(log10(PeakLeft));
					double PeakRightDB=20.0*(log10(PeakRight));
					double RmsLeftDB=20.0*(log10(rmsL));
					double RmsRightDB=20.0*(log10(rmsR));

					if (DuplPCMSource->GetNumChannels()==1)
						sprintf(MesBuf,"Peak level of mono item = %f dB\nRMS level of mono item= %.2f dB",PeakLeftDB,RmsLeftDB);
					
					if (DuplPCMSource->GetNumChannels()==2)
						sprintf(MesBuf,"Peak levels for stereo item, Left = %.2f dB , Right = %.2f dB\nRMS levels of stereo item, Left=%.2f dB, Right =%.2f dB",PeakLeftDB,PeakRightDB,RmsLeftDB,RmsRightDB);
					if (DuplPCMSource->GetNumChannels()>2 || DuplPCMSource->GetNumChannels()<1)
						sprintf(MesBuf,"Non-supported channel count!");
					MessageBox(g_hwndParent, MesBuf, "Item peak gain", MB_OK);
				}
				delete DuplPCMSource;
			}
		}
	}
}

void DoSelItemsToEndOfTrack(COMMAND_T*)
{
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	
	int numItems;
	bool ItemSelected=false;
	bool NewSelectedStatus=false;
	int flags;
	int i;
	int j;
	for (i=0;i<GetNumTracks();i++)
	{
		MunRaita = CSurf_TrackFromID(i+1,FALSE);
		GetTrackInfo(i,&flags);
		//if (flags & 0x02)
		//{ 
		numItems=GetTrackNumMediaItems(MunRaita);
		int LastSelItemIndex=-1;
		for (j=0;j<numItems;j++)
		{
			CurItem = GetTrackMediaItem(MunRaita,j);
			//propertyName="D_";
			ItemSelected=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (ItemSelected==TRUE)
			{
				LastSelItemIndex=j;
				break;
			}


		}
		if (LastSelItemIndex>=0)
		{
			//
			for (j=0;j<GetTrackNumMediaItems(MunRaita);j++)
			{
				CurItem = GetTrackMediaItem(MunRaita,j);
				if (j>=LastSelItemIndex && j<GetTrackNumMediaItems(MunRaita))
					ItemSelected=true; else ItemSelected=false;
				
				GetSetMediaItemInfo(CurItem,"B_UISEL",&ItemSelected);
			}

		}
		//}


	}
	UpdateTimeline();
}

void DoSelItemsToStartOfTrack(COMMAND_T*)
{
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	
	int numItems;
	
	
	bool ItemSelected=false;
	
	bool NewSelectedStatus=false;
	int flags;
	int j;
	int i;
	for (i=0;i<GetNumTracks();i++)
	{
		MunRaita = CSurf_TrackFromID(i+1,FALSE);
		GetTrackInfo(i,&flags);
		//if (flags & 0x02)
		//{ 
		numItems=GetTrackNumMediaItems(MunRaita);
		int LastSelItemIndex=-1;
		for (j=0;j<numItems;j++)
		{
			CurItem = GetTrackMediaItem(MunRaita,j);
			//propertyName="D_";
			ItemSelected=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (ItemSelected==TRUE)
			{
				LastSelItemIndex=j;
				//break;
			}


		}
		if (LastSelItemIndex>=0)
		{
			//
			for (j=0;j<GetTrackNumMediaItems(MunRaita);j++)
			{
				CurItem = GetTrackMediaItem(MunRaita,j);
				if (j>=0 && j<=LastSelItemIndex)
					ItemSelected=true; else ItemSelected=false;
				
				GetSetMediaItemInfo(CurItem,"B_UISEL",&ItemSelected);
			}

		}
		//}


	}
	UpdateTimeline();
}

void DoSetAllTakesPlay()
{
	//
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	//MediaItem_Take* CurTake;
	int numItems;;
	
	
	bool ItemSelected=false;
	
	int i;
	int j;
	
	for (i=0;i<GetNumTracks();i++)
	{
		MunRaita = CSurf_TrackFromID(i+1,FALSE);
		numItems=GetTrackNumMediaItems(MunRaita);
		for (j=0;j<numItems;j++)
		{
			CurItem = GetTrackMediaItem(MunRaita,j);
			//propertyName="D_";
			ItemSelected=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (ItemSelected==TRUE)
			{
				bool PlayAllTakes=true;
				GetSetMediaItemInfo(CurItem,"B_ALLTAKESPLAY",&PlayAllTakes);
				
			} 

		}
	}
	//Undo_OnStateChangeEx("Trim/Untrim Item Right Edge",4,-1);
	//UpdateTimeline();
}


void DoPanTakesOfItemSymmetrically()
{
	//
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	MediaItem_Take* CurTake;
	int numItems;;
	
	bool ItemSelected=false;
	
	bool NewSelectedStatus=false;
	//double EditCurPos=GetCursorPosition();
	
	//MediaItem** MediaItemsOnTrack;
	int i;
	int j;
	int k;
	for (i=0;i<GetNumTracks();i++)
	{
		MunRaita = CSurf_TrackFromID(i+1,FALSE);
		numItems=GetTrackNumMediaItems(MunRaita);
		//MediaItem* **MediaItemsOnTrack = new (MediaItem*)[numItems];
		

		
		for (j=0;j<numItems;j++)
		{
			CurItem = GetTrackMediaItem(MunRaita,j);
			ItemSelected=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (ItemSelected==TRUE)
			{
				if (GetMediaItemNumTakes(CurItem)>0)
				{
					int NumTakes=GetMediaItemNumTakes(CurItem);
					for (k=0;k<NumTakes;k++)
					{
					CurTake=GetMediaItemTake(CurItem,k);
				
					//double SnapOffset=*(double*)GetSetMediaItemInfo(CurItem,"D_SNAPOFFSET",NULL);
					double NewPan=-1.0+((2.0/(NumTakes-1))*k);
										
					GetSetMediaItemTakeInfo(CurTake,"D_PAN",&NewPan);
					}
				}
				//j=0;
				//}
				
			} 


		}
	

	}
	//Undo_OnStateChangeEx("Set Pan Of Takes In Item",4,-1);
	//UpdateTimeline();
}

void DoPanTakesSymmetricallyWithUndo(COMMAND_T*)
{
	DoPanTakesOfItemSymmetrically();
	Undo_OnStateChangeEx("Set Pan Of Takes In Item",4,-1);
	UpdateTimeline();
}


void DoImplodeTakesSetPlaySetSymPans(COMMAND_T*)
{
	//
	Undo_BeginBlock();
	
	Main_OnCommand(40438,0); // implode items across tracks into takes
	DoSetAllTakesPlay();
	DoPanTakesOfItemSymmetrically();
	Undo_EndBlock("Implode Items And Pan Symmetrically",0);
	//UpdateTimeline();
}

char *prop_strs[]=
{
  "Length",
  "Item Vol",
  
  "Take Vol",
  "Take Pan",
  "Take Pitch",
  "Take Pitch Resampled",
  "Take Media Offset",
  "Item FIPM Y-Pos",
  NULL
};

typedef struct
{
	char *Parameter;
	double StartVal;
	double EndVal;
	double Curve;
	double RandRange;
} t_InterPolateParams;


void InitPropBox(HWND hwnd, char *buf)
{

  int x;
  int a=0;
  for (x = 0; prop_strs[x]; x ++)
  {
    int r=(int)SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)prop_strs[x]);
    if (!a && !strcmp(buf,prop_strs[x]))
    {
      a=1;
      SendMessage(hwnd,CB_SETCURSEL,r,0);
    }
  }

  if (!a)
    SetWindowText(hwnd,buf);
}

HWND hInterpolateDlg;
bool g_InterpDlgFirstRun=true;
t_InterPolateParams LastInterpolateRunParams;

void GetSelItemsMinMaxTimes(double *MinTime,double *MaxTime)
{
	//
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	
	int numItems;;
	
	bool FirstSelFound=false;
	double FirstSelStart=-666.0;
	double LastSelStart=-666.0;
	bool ItemSelected=false;
	
	bool NewSelectedStatus=false;
	
	double MaxItemEnd=-1.0;
	double MinItemPos;
	int i;
	int j;
	for (i=0;i<GetNumTracks();i++)
	{
		MunRaita = CSurf_TrackFromID(i+1,FALSE);
		numItems=GetTrackNumMediaItems(MunRaita);
		for (j=0;j<numItems;j++)
		{
			CurItem = GetTrackMediaItem(MunRaita,j);
			double CurItemStart=*(double*)GetSetMediaItemInfo(CurItem,"D_POSITION",NULL);
			double CurItemLength=*(double*)GetSetMediaItemInfo(CurItem,"D_LENGTH",NULL);
			//propertyName="D_";
			ItemSelected=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (ItemSelected==TRUE)
			{
				
				LastSelStart=CurItemStart;
				if (LastSelStart>MaxItemEnd) MaxItemEnd=LastSelStart;
			}


		}
	}
	MinItemPos=LastSelStart;
	for (i=0;i<GetNumTracks();i++)
	{
		MunRaita = CSurf_TrackFromID(i+1,FALSE);
		numItems=GetTrackNumMediaItems(MunRaita);
		for (j=0;j<numItems;j++)
		{
			CurItem = GetTrackMediaItem(MunRaita,j);
			double CurItemStart=*(double*)GetSetMediaItemInfo(CurItem,"D_POSITION",NULL);
			double CurItemLength=*(double*)GetSetMediaItemInfo(CurItem,"D_LENGTH",NULL);
			//propertyName="D_";
			ItemSelected=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (ItemSelected==TRUE)
			{
				
				LastSelStart=CurItemStart+CurItemLength;
				if (CurItemStart<MinItemPos) MinItemPos=CurItemStart;
			}


		}
	}
	*MinTime=MinItemPos;
	*MaxTime=MaxItemEnd;
	//g_FirstSelectedItemPos=MinItemPos;
	//g_LastSelectedItemEnd=MaxItemEnd;
	//if (SetTheLoop)
		//GetSet_LoopTimeRange(true,true,&MinItemPos,&MaxItemEnd,false);
	
}

void DoInterpolateItemProperty(double StartValue,double EndValue,double Curve,double RandRange)
{
	//
	char TempString[64];
	int len = GetWindowTextLength(GetDlgItem(hInterpolateDlg, IDC_COMBO1));
	if(len > 0)
	{
		GetDlgItemText(hInterpolateDlg,  IDC_COMBO1, TempString, 63);
	} 
	bool IsForTakeProperty=false;
	char* ParamStr = NULL;
	
	MediaItem *CurItem;
	//"Take Pitch"
	if (strcmp(TempString,"Length")==0) { ParamStr="D_LENGTH";};
	if (strcmp(TempString,"Take Vol")==0) {ParamStr="D_VOL";IsForTakeProperty=true;};
	if (strcmp(TempString,"Take Pan")==0) {ParamStr="D_PAN";IsForTakeProperty=true;};
	if (strcmp(TempString,"Item FIPM Y-Pos")==0) {ParamStr="F_FREEMODE_Y";};
	if (strcmp(TempString,"Take Pitch")==0) {ParamStr="D_PITCH";IsForTakeProperty=true;};
	if (strcmp(TempString,"Take Pitch Resampled")==0) {ParamStr="D_PLAYRATE";IsForTakeProperty=true;};
	if (strcmp(TempString,"Item Vol")==0) {ParamStr="D_VOL";};
	if (strcmp(TempString,"Take Media Offset")==0) {ParamStr="D_STARTOFFS";IsForTakeProperty=true;}
	else LastInterpolateRunParams.Parameter=TempString;
	MediaTrack* MunRaita;
	double FirstItemStart=0.0;
	double LastItemStart=0.0;
    
	GetSelItemsMinMaxTimes(&FirstItemStart,&LastItemStart);
	
	double InterpolationDuration=LastItemStart-FirstItemStart;
	
	
	int numItems;
	
	

	bool ItemSelected=false;
	
	bool NewSelectedStatus=false;
	PCM_source *ThePCMSource;
	//Undo_BeginBlock();
	int i;
	int j;
	for (i=0;i<GetNumTracks();i++)
	{
		MunRaita = CSurf_TrackFromID(i+1,FALSE);
		numItems=GetTrackNumMediaItems(MunRaita);
		for (j=0;j<numItems;j++)
		{
			CurItem = GetTrackMediaItem(MunRaita,j);
			
			ItemSelected=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (ItemSelected==TRUE)
			{
				if (IsForTakeProperty==false)
				{
					double ItemPos=*(double*)GetSetMediaItemInfo(CurItem,"D_POSITION",NULL);
					double NormalizedTime=(1.0/(LastItemStart-FirstItemStart))*(ItemPos-FirstItemStart);
					ItemPos=ItemPos-FirstItemStart;
					double ValueRange=EndValue-StartValue;
					double NewPropertyValue=StartValue+((EndValue-StartValue)/InterpolationDuration)*ItemPos;
					double NormalizedValue;
					double Booh=pow(NormalizedTime,Curve);
					// TODO - the below 3 lines were commented out before - why??
					if (EndValue>StartValue)
						NormalizedValue=(1.0/(EndValue-StartValue))*NewPropertyValue;
					else	NormalizedValue=1.0+((1.0/(EndValue-StartValue))*NewPropertyValue);	 
					double ShapedVal=pow(NormalizedValue,Curve);
					if (EndValue>StartValue)
						ShapedVal=StartValue+((EndValue-StartValue)*ShapedVal);
					else ShapedVal=StartValue+((EndValue-StartValue)*ShapedVal);
					ShapedVal=StartValue+(ValueRange*Booh);
					double RandDelta=-(RandRange)+(1.0/RAND_MAX)*rand()*(2.0*RandRange);
					ShapedVal=ShapedVal+RandDelta;
					if (strcmp(ParamStr,"F_FREEMODE_Y")!=0)
					{
						//Math.pow(2,(centin/100/12));
						
						GetSetMediaItemInfo(CurItem,ParamStr,&ShapedVal);
					}
					else
					{
						//
						float yPos=(float)ShapedVal;
						GetSetMediaItemInfo(CurItem,ParamStr,&yPos);
					}

				} else
				{
					//
					
					
					if (GetMediaItemNumTakes(CurItem)>0)
					{
					MediaItem_Take *CurTake=GetMediaItemTake(CurItem,-1);
					ThePCMSource=(PCM_source*)GetSetMediaItemTakeInfo(CurTake,"P_SOURCE",NULL);
					double SourceMediaLen=ThePCMSource->GetLength();
					
					double ItemPos=*(double*)GetSetMediaItemInfo(CurItem,"D_POSITION",NULL);
					double NormalizedTime=(1.0/(LastItemStart-FirstItemStart))*(ItemPos-FirstItemStart);
					ItemPos=ItemPos-FirstItemStart;
					
					double Poo=pow(NormalizedTime,Curve);
					double ValueRange;
					//if (EndValue>StartValue)
						ValueRange=EndValue-StartValue;
					//else ValueRange=fabs(StartValue)-fabs(EndValue);

					double NewPropertyValue=StartValue+((ValueRange)/InterpolationDuration)*ItemPos;
					double NormalizedValue;
					double MaxBooh=pow(1.0,Curve);
					double Booh=pow(NormalizedTime,Curve);
					//Booh=(1.0/MaxBooh)*Booh;
					// (startVal-EndVal)*f(t)+startVal

					//if (EndValue>StartValue)
						NormalizedValue=(1.0/ValueRange)*(NewPropertyValue-StartValue);
					//else	NormalizedValue=1.0+((1.0/(EndValue-StartValue))*NewPropertyValue);	 
					double ShapedVal=pow(NormalizedValue,Curve);
					//if (EndValue>StartValue)
						ShapedVal=StartValue+(ValueRange*ShapedVal);
					//else ShapedVal=StartValue+(ValueRange*(ShapedVal));
					ShapedVal=StartValue+(ValueRange*Booh);
					double RandDelta=-(RandRange)+(1.0/RAND_MAX)*rand()*(2.0*RandRange);
					ShapedVal=ShapedVal+RandDelta;
					if (strcmp(TempString,"Take Pitch Resampled")==0)
						{
							ShapedVal=pow(2.0,ShapedVal/12.0);
							bool Preserve=false;
							GetSetMediaItemTakeInfo(CurTake,"B_PPITCH",&Preserve);
						}
					if (strcmp(TempString,"Take Media Offset")==0)
						{
							ShapedVal=(SourceMediaLen/100.0)*ShapedVal;
						}
					GetSetMediaItemTakeInfo(CurTake,ParamStr,&ShapedVal);
					}
				}

				
			} 


		}
	}
	//delete ThePCMSource;
	Undo_OnStateChangeEx("Interpolate Item Property Over Time",4,-1);
	//Undo_EndBlock("Interpolate Item Property Over Time",0);
	UpdateTimeline();
	delete[] ParamStr;
}

HWND *g_hPropNameLabels;
HWND *g_hStartValueEdits;
HWND *g_hEndValueEdits;

WDL_DLGRET InterpolateItemPropDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	//
	
	switch(Message)
    {
        case WM_INITDIALOG:
			{
			char TextBuf[64];
			//
			//for (int i=0;i<sizeof(prop_strs)-2;i++)
			bool PropEnd=false;
			int i=0;
			g_hPropNameLabels=new HWND[32];
			g_hStartValueEdits=new HWND[32];
			g_hEndValueEdits=new HWND[32];
			while (!PropEnd)
			{
				if (prop_strs[i]!=NULL)
				{
					sprintf(TextBuf,"PROPLABEL%d",i);
					g_hPropNameLabels[i] = CreateWindowEx(WS_EX_LEFT, "Static", TextBuf, 
					WS_CHILD | WS_VISIBLE, 
					10, i*22+30, 90, 20, hwnd, NULL, g_hInst, NULL);
					SetWindowText(g_hPropNameLabels[i],prop_strs[i]);
					sprintf(TextBuf,"STARTVALBOX%d",i);
					g_hStartValueEdits[i] = CreateWindowEx(WS_EX_LEFT, "Edit", TextBuf, 
					WS_CHILD | WS_VISIBLE, 
					110, i*22+30, 40, 18, hwnd, NULL, g_hInst, NULL);
					SetWindowText(g_hStartValueEdits[i],"0.0");

					sprintf(TextBuf,"ENDVALBOX%d",i);
					g_hEndValueEdits[i] = CreateWindowEx(WS_EX_LEFT, "Edit", TextBuf, 
					WS_CHILD | WS_VISIBLE, 
					160, i*22+30, 40, 18, hwnd, NULL, g_hInst, NULL);
					SetWindowText(g_hEndValueEdits[i],"0.0");
				}
				else PropEnd=true;
				i++;
				if (i>50) break;
			}
			sprintf(TextBuf,"%.2f",LastInterpolateRunParams.StartVal);
			SetDlgItemText(hwnd, IDC_EDIT1, TextBuf);
			sprintf(TextBuf,"%.2f",LastInterpolateRunParams.EndVal);
			SetDlgItemText(hwnd, IDC_EDIT2, TextBuf);
			sprintf(TextBuf,"%.2f",LastInterpolateRunParams.Curve);
			SetDlgItemText(hwnd, IDC_EDIT3, TextBuf);
			sprintf(TextBuf,"%.2f",LastInterpolateRunParams.RandRange);
			SetDlgItemText(hwnd, IDC_EDIT4, TextBuf);
			InitPropBox(GetDlgItem(hwnd,IDC_COMBO1),LastInterpolateRunParams.Parameter);
			hInterpolateDlg=hwnd;
			SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwnd, IDC_EDIT1), TRUE);
			return 0;
			}
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
					{
						double StartValue=0.0;
						double EndValue=0.0;
						double Curve=1.0;
						double RandRange=0.0;
						//WDL_String *TempString=new WDL_String;
						char TempString[101];
						//GetDialogItemString(hInterpolateDlg,IDC_EDIT1,TempString);
						GetDlgItemText(hwnd,IDC_EDIT1,TempString,100);
						StartValue=atof(TempString);
						GetDlgItemText(hwnd,IDC_EDIT2,TempString,100);
						EndValue=atof(TempString);
						GetDlgItemText(hwnd,IDC_EDIT3,TempString,100);
						Curve=atof(TempString);
						GetDlgItemText(hwnd,IDC_EDIT4,TempString,100);
						RandRange=atof(TempString);
						
						//GetDialogItemString(hInterpolateDlg,IDC_EDIT2,TempString);
						//EndValue=strtod(TempString->Get(),NULL);
						//GetDialogItemString(hInterpolateDlg,IDC_EDIT3,TempString);
						//Curve=strtod(TempString->Get(),NULL);
						//GetDialogItemString(hInterpolateDlg,IDC_EDIT4,TempString);
						//RandRange=strtod(TempString->Get(),NULL);
						DoInterpolateItemProperty(StartValue,EndValue,Curve,RandRange);
						//MessageBox(hwnd,"ok pressed","info",MB_OK);
						LastInterpolateRunParams.StartVal=StartValue;
						LastInterpolateRunParams.EndVal=EndValue;
						LastInterpolateRunParams.Curve=Curve;
						LastInterpolateRunParams.RandRange=RandRange;
						EndDialog(hwnd,0);
						return 0;
					}
				case IDCANCEL:
					{
						//MessageBox(hwnd,"cancel pressed","info",MB_OK);
						EndDialog(hwnd,0);
						return 0;
					}
			}
	}
	return 0;
}

void DoInterpolateItemPropertyOverTime(COMMAND_T*)
{
	//
	if (g_InterpDlgFirstRun)
	{
		//
		LastInterpolateRunParams.Curve=1.0;
		LastInterpolateRunParams.StartVal=1.0;
		LastInterpolateRunParams.EndVal=1.0;
		LastInterpolateRunParams.RandRange=0.0;
		LastInterpolateRunParams.Parameter="Item Vol";
		g_InterpDlgFirstRun=false;
	}

	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_INTERPOLATEITEMPROP), g_hwndParent, InterpolateItemPropDlgProc);
}

double g_lastTailLen;

void DoWorkForRenderItemsWithTail(double TailLen)
{
	// Unselect all tracks
	
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	
	int numItems;;
	
	char textbuf[100];
	int sz=0; int *fxtail = (int *)get_config_var("itemfxtail",&sz);
    int OldTail=*fxtail;
	if (sz==sizeof(int) && fxtail) 
	{ 
			
		int msTail=int(1000*TailLen);
		*fxtail=msTail;
		sprintf(textbuf,"%d",msTail);
	}

	bool ItemSelected=false;
	int NewTrackSelStatus=1;
	bool NewSelectedStatus=false;
	int i;
	int j;
	for (i=0;i<GetNumTracks();i++)
	{
		MunRaita = CSurf_TrackFromID(i+1,FALSE);
		numItems=GetTrackNumMediaItems(MunRaita);
		Main_OnCommand(40635,0);
		for (j=0;j<numItems;j++)
		{
			CurItem = GetTrackMediaItem(MunRaita,j);
			//propertyName="D_";
			ItemSelected=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (ItemSelected==TRUE)
			{
				//
				double ItemStart=*(double*)GetSetMediaItemInfo(CurItem,"D_POSITION",NULL);
				double ItemLen=*(double*)GetSetMediaItemInfo(CurItem,"D_LENGTH",NULL);
				double ItemEnd=ItemStart+ItemLen;
				double EndOfEmptyItem=ItemEnd+TailLen;
				
				//GetSet_LoopTimeRange(true,false,&ItemEnd,&EndOfEmptyItem,false);
				Main_OnCommand(40601,0); // render items as new take


			} 


		}
		
	}
	*fxtail=OldTail;
	UpdateTimeline();
}

WDL_DLGRET RenderItemsWithTailDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	//
	switch(Message)
    {
        case WM_INITDIALOG:
			{
			char TextBuf[32];
			
			sprintf(TextBuf,"%.2f",g_lastTailLen);
			SetDlgItemText(hwnd, IDC_EDIT1, TextBuf);
			
			SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwnd, IDC_EDIT1), TRUE);
			//SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_SETCURSEL, 0, 0);
			//delete TextBuf;
			return 0;
			}
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
					{
						char textbuf[100];
						GetDlgItemText(hwnd,IDC_EDIT1,textbuf,100);
						g_lastTailLen=atof(textbuf);
						DoWorkForRenderItemsWithTail(g_lastTailLen);
						EndDialog(hwnd,0);
						return 0;
					}
				case IDCANCEL:
					{
						//MessageBox(hwnd,"cancel pressed","info",MB_OK);
						EndDialog(hwnd,0);
						return 0;
					}
			}
	}
	return 0;
}


void DoRenderItemsWithTail(COMMAND_T*)
{
	//
	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_RENDITEMS), g_hwndParent, RenderItemsWithTailDlgProc);	
}

void DoOpenAssociatedRPP(COMMAND_T*)
{
	//
	WDL_PtrList<MediaItem_Take> *TheTakes=new (WDL_PtrList<MediaItem_Take>);
	PCM_source *ThePCMSource;
	MediaItem_Take* CurTake;
	int NumActiveTakes=GetActiveTakes(TheTakes);
	if (NumActiveTakes==1)
	{
		
				CurTake=TheTakes->Get(0); // we will only support first selected item for now
				ThePCMSource=(PCM_source*)GetSetMediaItemTakeInfo(CurTake,"P_SOURCE",NULL);
				char RPPFileNameBuf[1024];
				
				sprintf(RPPFileNameBuf,"%s\\reaper.exe \"%s.RPP\"",GetExePath(), ThePCMSource->GetFileName());
				if (!DoLaunchExternalTool(RPPFileNameBuf)) MessageBox(g_hwndParent,"Could not launch REAPER.","Error",MB_OK);
				//PCM_source *NewPCMSource=PCM_Source_CreateFromFile(ReplaceFileNames->Get(0));
				
				//ThePCMSource->~PCM_source();
			//ThePCMSource->
		
			
	
			
		
	} else MessageBox(g_hwndParent,"None or more than 1 item selected","Error",MB_OK);
	delete TheTakes;
	//delete ReplaceFileNames;
}

typedef struct 
{
	double Gap;
	int ModeA;
	int ModeB;
	int ModeC;
} t_ReposItemsParams;

t_ReposItemsParams g_ReposItemsParams;

bool g_FirstReposItemsRun=true;

void RepositionItems(double theGap,int ModeA,int ModeB,int ModeC) // ModeA : gap from item starts/end... ModeB=per track/all items...ModeC=seconds/beats...
{
	//
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	
	int numItems;
	bool ItemSelected=false;
	bool NewSelectedStatus=false;
	int ItemCounter=0;
	int PrevSelItemInd=-1;
	int FirstSelItemInd=-1;
	int i;
	int j;
	for (i=0;i<GetNumTracks();i++)
	{
		MunRaita = CSurf_TrackFromID(i+1,FALSE);
		numItems=GetTrackNumMediaItems(MunRaita);
		//MediaItem* **MediaItemsOnTrack = new (MediaItem*)[numItems];
		MediaItem** MediaItemsOnTrack = new MediaItem*[numItems];
		
		for (j=0;j<numItems;j++)
		{
			CurItem = GetTrackMediaItem(MunRaita,j);
			MediaItemsOnTrack[j]=CurItem;
			bool X;
			X=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if ((FirstSelItemInd==-1) && (X==true)) FirstSelItemInd=j;
		}
		PrevSelItemInd=FirstSelItemInd;
		for (j=0;j<numItems;j++)
		{
			ItemSelected=*(bool*)GetSetMediaItemInfo(MediaItemsOnTrack[j],"B_UISEL",NULL);
			if (ItemSelected==TRUE)
			{
				//double SnapOffset=*(double*)GetSetMediaItemInfo(CurItem,"D_SNAPOFFSET",NULL);
				double NewPos;
				double OldPos=*(double*)GetSetMediaItemInfo(MediaItemsOnTrack[j],"D_POSITION",NULL);
				//double OldPos=g_StoredPositions[ItemCounter];
				if (ModeA==0)
				{
					if (j==FirstSelItemInd) NewPos=OldPos;
					if (j>FirstSelItemInd) 
					{
						OldPos=*(double*)GetSetMediaItemInfo(MediaItemsOnTrack[PrevSelItemInd],"D_POSITION",NULL);
						NewPos=OldPos+theGap;
					}
				
				}
				if (ModeA==1)
				{
					if (j==FirstSelItemInd) NewPos=OldPos;
					if (j>FirstSelItemInd) 
					{
						OldPos=*(double*)GetSetMediaItemInfo(MediaItemsOnTrack[PrevSelItemInd],"D_POSITION",NULL);
						double PrevLen=*(double*)GetSetMediaItemInfo(MediaItemsOnTrack[PrevSelItemInd],"D_LENGTH",NULL);
						double PrevEnd=OldPos+PrevLen;
						NewPos=PrevEnd+theGap;
					}
				
				}
								
				GetSetMediaItemInfo(MediaItemsOnTrack[j],"D_POSITION",&NewPos);
				PrevSelItemInd=j;
				
				
			} 


		}
	delete[] MediaItemsOnTrack;

	}
}


WDL_DLGRET ReposItemsDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
    {
	case WM_INITDIALOG:
			{
			if (g_ReposItemsParams.ModeA==0)  SendMessage(GetDlgItem(hwnd,IDC_RADIO1),BM_CLICK,0,0);
			if (g_ReposItemsParams.ModeA==1)  SendMessage(GetDlgItem(hwnd,IDC_RADIO2),BM_CLICK,0,0);
			char TextBuf[32];
			
			sprintf(TextBuf,"%.2f",g_ReposItemsParams.Gap);
			SetDlgItemText(hwnd, IDC_EDIT1, TextBuf);
			
			
			SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwnd, IDC_EDIT1), TRUE);
			//SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_SETCURSEL, 0, 0);
			//IDC_RADIO1
			
			return 0;
			}
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
					{
						char textbuf[30];
						GetDlgItemText(hwnd,IDC_EDIT1,textbuf,30);
						double theGap=atof(textbuf);
						int modeA=0;
						if (Button_GetCheck(GetDlgItem(hwnd,IDC_RADIO1))==BST_CHECKED) modeA=0;
						if (Button_GetCheck(GetDlgItem(hwnd,IDC_RADIO2))==BST_CHECKED) modeA=1;
						RepositionItems(theGap,modeA,0,0);
						UpdateTimeline();
						Undo_OnStateChangeEx("Reposition Items",4,-1);
						g_ReposItemsParams.Gap=theGap;
						g_ReposItemsParams.ModeA=modeA;
						g_ReposItemsParams.ModeB=0;
						g_ReposItemsParams.ModeC=0;
						
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

void DoReposItemsDlg(COMMAND_T*)
{
	//
	if (g_FirstReposItemsRun==true)
	{
		g_ReposItemsParams.Gap=1.0;
		g_ReposItemsParams.ModeA=1; // item starts based
		g_ReposItemsParams.ModeB=0; // per track based
		g_ReposItemsParams.ModeC=0; // seconds
		g_FirstReposItemsRun=false;
	}

	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_REPOSITEMS), g_hwndParent, ReposItemsDlgProc);
}

void DoSpeadSelItemsOverNewTx(COMMAND_T*)
{
	vector<MediaItem*> TheItems;
	XenGetProjectItems(TheItems,true,false);
	if (TheItems.size()>1)
	{
		MediaTrack* OriginTrack=(MediaTrack*)GetSetMediaItemInfo(TheItems[0],"P_TRACK",NULL);
		int OriginTrackID=CSurf_TrackToID(OriginTrack,false);
		int newDep=1;
		GetSetMediaTrackInfo(OriginTrack,"I_FOLDERDEPTH",&newDep);
		int i;
		// we make new tracks first to not make reaper or ourselves confused
		for (i=0;i<(int)TheItems.size();i++)
			InsertTrackAtIndex(OriginTrackID+i+1,true);

		// then we "drop" the items
		OriginTrack=(MediaTrack*)GetSetMediaItemInfo(TheItems[0],"P_TRACK",NULL);
		OriginTrackID=CSurf_TrackToID(OriginTrack,false);
		
		MediaTrack* DestTrack=0;
		for (i=0;i<(int)TheItems.size();i++)
		{
			int NewTrackIndex=OriginTrackID+i+1;
			DestTrack=CSurf_TrackFromID(NewTrackIndex,false);
			if (DestTrack)
				MoveMediaItemToTrack(TheItems[i],DestTrack);
		}
		newDep=-1;
		GetSetMediaTrackInfo(DestTrack,"I_FOLDERDEPTH",&newDep);
		TrackList_AdjustWindows(false);
		Undo_OnStateChangeEx("Spread selected items on new tracks",UNDO_STATE_ALL,-1);

	} else MessageBox(g_hwndParent,"No or only one item selected!","Error",MB_OK);
		
}

int OpenInExtEditor(int editorIdx)
{
	vector<MediaItem_Take*> TheTakes;
	XenGetProjectTakes(TheTakes,true,true);
	if (TheTakes.size()==1)
	{
		MediaItem* CurItem=(MediaItem*)GetSetMediaItemTakeInfo(TheTakes[0],"P_ITEM",NULL);
		//
		
		//Main_OnCommand(40639,0); // duplicate active take
		Main_OnCommand(40601,0); // render items and add as new take
		int curTakeIndex=*(int*)GetSetMediaItemInfo(CurItem,"I_CURTAKE",NULL);
		MediaItem_Take *CopyTake=GetMediaItemTake(CurItem,-1);
		PCM_source *ThePCM=(PCM_source*)GetSetMediaItemTakeInfo(CopyTake,"P_SOURCE",NULL);
		char ExeString[2048];
		if (editorIdx==0 && g_external_app_paths.PathToAudioEditor1)
			sprintf(ExeString,"\"%s\" \"%s\"",g_external_app_paths.PathToAudioEditor1,ThePCM->GetFileName());
		if (editorIdx==1 && g_external_app_paths.PathToAudioEditor2)
			sprintf(ExeString,"\"%s\" \"%s\"",g_external_app_paths.PathToAudioEditor2,ThePCM->GetFileName());
		
		DoLaunchExternalTool(ExeString);
	}

	return -666;	
}

void DoOpenInExtEditor1(COMMAND_T*) { OpenInExtEditor(0); }
void DoOpenInExtEditor2(COMMAND_T*) { OpenInExtEditor(1); }

void DoMatrixItemImplode(COMMAND_T*)
{
	vector<MediaItem*> TheItems;
	XenGetProjectItems(TheItems,true,false);
	vector<double> OldItemPositions;
	vector<double> OldItemLens;
	int i;
	for (i=0;i<(int)TheItems.size();i++)
	{
		double itemPos=*(double*)GetSetMediaItemInfo(TheItems[i],"D_POSITION",NULL);
		OldItemPositions.push_back(itemPos);
		double itemLen=*(double*)GetSetMediaItemInfo(TheItems[i],"D_LENGTH",NULL);
		OldItemLens.push_back(itemLen);
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
		double newItemLen=OldItemLens[i];
		GetSetMediaItemInfo(TempItem[0],"D_LENGTH",&newItemLen);
		//PastedItems.push_back(TempItem[0]);
	}
	/*
	for (i=0;i<PastedItems.size();i++)
	{
		double newItemLen=OldItemLens[i+1];
		GetSetMediaItemInfo(PastedItems[i],"D_LENGTH",&newItemLen);
	}
	*/
	//40058 // paste
	Undo_EndBlock("Matrix implode items",0);
	//Undo_OnStateChangeEx("Matrix implode items",4,-1);
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
	double temvo=TimeMap_GetDividedBpmAtTime(0.0);
	for (i=0;i<(int)TheItems.size();i++)
	{
		double itempos=*(double*)GetSetMediaItemInfo(TheItems[i],"D_POSITION",0);
		double itemposQN=TimeMap_timeToQN(itempos);
		int itemposSXTHN=(int)((itemposQN*4.0)+0.5);
		double itemposinsixteenths=itemposQN*4.0;
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
	Undo_OnStateChangeEx("Swing item positions",4,-1);
}

WDL_DLGRET SwingItemsDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	char tbuf[200];
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
	static bool firstRun=true;
	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_SWINGITEMPOS), g_hwndParent, SwingItemsDlgProc);
	
}

void DoTimeSelAdaptDelete(COMMAND_T*)
{
	vector<MediaItem*> SelItems;
	vector<MediaItem*> ItemsInTimeSel;
	XenGetProjectItems(SelItems,true,false);
	double OldTimeSelLeft=0.0;
	double OldTimeSelRight=0.0;
	GetSet_LoopTimeRange(false,false,&OldTimeSelLeft,&OldTimeSelRight,false);
	int i;
	if (OldTimeSelRight-OldTimeSelLeft>0.0)
	{
		for (i=0;i<(int)SelItems.size();i++)
		{
			double itempos=*(double*)GetSetMediaItemInfo(SelItems[i],"D_POSITION",0);
			double itemlen=*(double*)GetSetMediaItemInfo(SelItems[i],"D_LENGTH",0);
			bool itemsel=*(bool*)GetSetMediaItemInfo(SelItems[i],"B_UISEL",0);
			if (itemsel)
			{
				int intersectmatches=0;
				if (OldTimeSelLeft>=itempos && OldTimeSelRight<=itempos+itemlen)
					intersectmatches++;
				if (itempos>=OldTimeSelLeft && itempos+itemlen<=OldTimeSelRight)
					intersectmatches++;
				if (OldTimeSelLeft<=itempos+itemlen && OldTimeSelRight>=itempos+itemlen)
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

void DoDeleteMutedItems(COMMAND_T*)
{
	
	Undo_BeginBlock();
	Main_OnCommand(40289,0); // unselect all items
	vector<MediaItem*> pitems;
	XenGetProjectItems(pitems,false,false);
	int i;
	for (i=0;i<(int)pitems.size();i++)
	{
		bool muted=*(bool*)GetSetMediaItemInfo(pitems[i],"B_MUTE",0);
		if (muted)
		{
			bool uisel=true;
			GetSetMediaItemInfo(pitems[i],"B_UISEL",&uisel);
		}
	}
	Main_OnCommand(40006,0);
	Undo_EndBlock("Remove muted items",0);
	UpdateTimeline();
}
