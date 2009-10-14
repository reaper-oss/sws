/******************************************************************************
/ ExoticCommands.cpp
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

char NudgeComboText[200] = "";
HWND g_hNudgeComboBox = NULL;

bool g_ScaleItemPosFirstRun=true;

void DoJumpEditCursorByRandomAmount(COMMAND_T*)
{
	double RandomMean;
	RandomMean=g_command_params.EditCurRndMean;
	double RandExp=-log((1.0/RAND_MAX)*rand())*RandomMean;
	double NewCurPos=GetCursorPosition()+RandExp;
	SetEditCurPos(NewCurPos,false,false);
}

void DoNudgeSelectedItemsPositions(bool UseConfig,bool Positive,double NudgeTime)
{
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	
	int numItems=666;
	bool ItemSelected=false;
	
	bool NewSelectedStatus=false;
	double EditCurPos=GetCursorPosition();
	
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
				//double SnapOffset=*(double*)GetSetMediaItemInfo(CurItem,"D_SNAPOFFSET",NULL);
				double NewPos;
				double OldPos=*(double*)GetSetMediaItemInfo(MediaItemsOnTrack[j],"D_POSITION",NULL);
				if (Positive)
					NewPos=OldPos+g_command_params.ItemPosNudgeSecs; else NewPos=OldPos-g_command_params.ItemPosNudgeSecs;
				//if (OldPos!=NewPos)
				//{					
					GetSetMediaItemInfo(MediaItemsOnTrack[j],"D_POSITION",&NewPos);
					//j=0;
				//}
				
			} 


		}
	delete[] MediaItemsOnTrack;

	}
	Undo_OnStateChangeEx("Nudge item position(s)",4,-1);
	UpdateTimeline();
}

void DoSetItemFadesConfLen(COMMAND_T*)
{
	double NewFadeInLen;
	double NewFadeOutLen;
	NewFadeInLen=g_command_params.CommandFadeInA;
	NewFadeOutLen=g_command_params.CommandFadeOutA;
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	int numItems=666;
	
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
				double ItemLen=*(double*)GetSetMediaItemInfo(CurItem,"D_LENGTH",NULL);
				if ((NewFadeInLen+NewFadeOutLen)>ItemLen)
				{
					//NewFadeInLen=0;
					NewFadeOutLen=ItemLen-(NewFadeInLen+0.05);
				}

				GetSetMediaItemInfo(CurItem,"I_FADEINSHAPE",&g_command_params.CommandFadeInShapeA);
				GetSetMediaItemInfo(CurItem,"I_FADEOUTSHAPE",&g_command_params.CommandFadeOutShapeA);
				GetSetMediaItemInfo(CurItem,"D_FADEINLEN",&NewFadeInLen);
				GetSetMediaItemInfo(CurItem,"D_FADEOUTLEN",&NewFadeOutLen);
				
			} 

		}
	}
	Undo_OnStateChangeEx("Set item fades to configured lenghts",4,-1);
	UpdateTimeline();
}

void DoSetItemFadesConfLenB(COMMAND_T*)
{
	double NewFadeInLen;
	double NewFadeOutLen;
	NewFadeInLen=g_command_params.CommandFadeInB;
	NewFadeOutLen=g_command_params.CommandFadeOutB;
	MediaTrack* pTrack;
	MediaItem* CurItem;
	int numItems=666;
	bool ItemSelected=false;
	
	bool NewSelectedStatus=false;
	int i;
	int j;
	for (i=0;i<GetNumTracks();i++)
	{
		pTrack = CSurf_TrackFromID(i+1,FALSE);
		numItems=GetTrackNumMediaItems(pTrack);
		for (j=0;j<numItems;j++)
		{
			CurItem = GetTrackMediaItem(pTrack,j);
			//propertyName="D_";
			ItemSelected=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (ItemSelected==TRUE)
			{
				double ItemLen=*(double*)GetSetMediaItemInfo(CurItem,"D_LENGTH",NULL);
				if ((NewFadeInLen+NewFadeOutLen)>ItemLen)
				{
					//NewFadeInLen=0;
					NewFadeOutLen=ItemLen-(NewFadeInLen+0.05);
				}

				GetSetMediaItemInfo(CurItem,"I_FADEINSHAPE",&g_command_params.CommandFadeInShapeB);
				GetSetMediaItemInfo(CurItem,"I_FADEOUTSHAPE",&g_command_params.CommandFadeOutShapeB);
				GetSetMediaItemInfo(CurItem,"D_FADEINLEN",&NewFadeInLen);
				GetSetMediaItemInfo(CurItem,"D_FADEOUTLEN",&NewFadeOutLen);
			} 
		}
	}
	Undo_OnStateChangeEx("Set Item Fades To Configured Lenghts",4,-1);
	UpdateTimeline();
}


void DoNudgeItemPitches(double NudgeAmount,bool Resampled)
{
	vector<MediaItem_Take*> ProjectTakes;
	XenGetProjectTakes(ProjectTakes,true,true);
	MediaItem *CurItem;
	int i;
	for (i=0;i<(int)ProjectTakes.size();i++)
	{
		if (Resampled==false)
		{
			double OldPitchAdjust=*(double*)GetSetMediaItemTakeInfo(ProjectTakes[i],"D_PITCH",NULL);
			double NewPitchAdjust=OldPitchAdjust+NudgeAmount;
			GetSetMediaItemTakeInfo(ProjectTakes[i],"D_PITCH",&NewPitchAdjust);
		}
		if (Resampled==true)
		{
			CurItem=(MediaItem*)GetSetMediaItemTakeInfo(ProjectTakes[i],"P_ITEM",NULL);
			double OldPlayRate=*(double*)GetSetMediaItemTakeInfo(ProjectTakes[i],"D_PLAYRATE",NULL);
			double OldPitchSemis=12.0*log(OldPlayRate)/log(2.0);
			//double OldItemSnapsOffs=*(double*)GetSetMediaItemInfo(CurItem,"D_SNAPOFFSET",0);
			double OldItemPos=*(double*)GetSetMediaItemInfo(CurItem,"D_POSITION",0);
			double NewPitchSemis=OldPitchSemis+NudgeAmount;
			double NewPlayRate=pow(2.0,NewPitchSemis/12.0);
			double NewItemPos=OldItemPos*(NewPlayRate/OldPlayRate);
			double OldLength=*(double*)GetSetMediaItemInfo(CurItem,"D_LENGTH",NULL);
			double NewLength=OldLength*(1.0/(NewPlayRate/OldPlayRate));
			//double NewItemSnapOffs=OldItemSnapsOffs*NewPlayRate;
			bool DummyB=false;
			double DummyD=0.0;
			GetSetMediaItemInfo(CurItem,"D_LENGTH",&NewLength);
			//GetSetMediaItemInfo(CurItem,"D_POSITION",&NewItemPos);
			GetSetMediaItemTakeInfo(ProjectTakes[i],"B_PPITCH",&DummyB);
			GetSetMediaItemTakeInfo(ProjectTakes[i],"D_PITCH",&DummyD);
			GetSetMediaItemTakeInfo(ProjectTakes[i],"D_PLAYRATE",&NewPlayRate);
		}
	}
	Undo_OnStateChangeEx("Nudge Item Pitch",4,-1);
	UpdateTimeline();
}

void DoNudgeItemPitchesDown(COMMAND_T*)
{
	DoNudgeItemPitches(-g_command_params.ItemPitchNudgeA,false);
}

void DoNudgeItemPitchesUp(COMMAND_T*)
{
	DoNudgeItemPitches(g_command_params.ItemPitchNudgeA,false);
}

void DoNudgeItemPitchesDownB(COMMAND_T*)
{
	DoNudgeItemPitches(-g_command_params.ItemPitchNudgeB,false);
}	

void DoNudgeItemPitchesUpB(COMMAND_T*)
{
	DoNudgeItemPitches(g_command_params.ItemPitchNudgeB,false);	
}

void DoNudgeUpTakePitchResampledA(COMMAND_T*)
{
	DoNudgeItemPitches(g_command_params.ItemPitchNudgeA,true);
}

void DoNudgeDownTakePitchResampledA(COMMAND_T*)
{
	DoNudgeItemPitches(-g_command_params.ItemPitchNudgeA,true);	
}

void DoNudgeUpTakePitchResampledB(COMMAND_T*)
{
	DoNudgeItemPitches(g_command_params.ItemPitchNudgeB,true);
}

void DoNudgeDownTakePitchResampledB(COMMAND_T*)
{
	DoNudgeItemPitches(-g_command_params.ItemPitchNudgeB,true);	
}

void DoNudgeItemsBeatsBased(bool UseConf,bool Positive, double theNudgeAmount)
{
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	
	int numItems=666;
	double NudgeAmount=g_command_params.ItemPosNudgeBeats;
	bool ItemSelected=false;
	
	bool NewSelectedStatus=false;
	int i;
	int j;
	for (i=0;i<GetNumTracks();i++)
	{
		MunRaita = CSurf_TrackFromID(i+1,FALSE);
		numItems=GetTrackNumMediaItems(MunRaita);
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
				double NewPos;
				double OldPos=*(double*)GetSetMediaItemInfo(MediaItemsOnTrack[j],"D_POSITION",NULL);
				double OldPosBeats=TimeMap_timeToQN(OldPos);
				double NewPosBeats;
				if (Positive)
					NewPosBeats=OldPosBeats+NudgeAmount; else NewPosBeats=OldPosBeats-NudgeAmount;
				NewPos=TimeMap_QNToTime(NewPosBeats);
				GetSetMediaItemInfo(MediaItemsOnTrack[j],"D_POSITION",&NewPos);
			} 
		}
		delete[] MediaItemsOnTrack;
	}
	Undo_OnStateChangeEx("Nudge Item Position(s), Beat Based",4,-1);
	UpdateTimeline();
}


void DoNudgeItemsLeftBeatsAndConfBased(COMMAND_T*)
{
	DoNudgeItemsBeatsBased(true,false,0.0);
}

void DoNudgeItemsRightBeatsAndConfBased(COMMAND_T*)
{
	DoNudgeItemsBeatsBased(true,true,0.0);
}

void DoSplitItemsAtTransients(COMMAND_T*)
{
	//
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	//int numItems=666;
	bool ItemSelected=false;
	
	bool NewSelectedStatus=false;
	//double EditCurPos=GetCursorPosition();
	
	//MediaItem** MediaItemsOnTrack;
	//double CurrentHZoom=GetHZoomLevel();
	//adjustZoom(10, 0, false, -1);
	// Find minimum item position from all selected
	int ItemCounter=0;
	int numItems=GetNumSelectedItems();
	MediaItem** MediaItemsInProject = new MediaItem*[numItems];
	int i;
	int j;
	for (i=0;i<GetNumTracks();i++)
	{
		MunRaita = CSurf_TrackFromID(i+1,FALSE);
		int numItemsTrack=GetTrackNumMediaItems(MunRaita);
		//MediaItem* **MediaItemsOnTrack = new (MediaItem*)[numItems];
		
		
		for (j=0;j<numItemsTrack;j++)
		{
			CurItem = GetTrackMediaItem(MunRaita,j);
			ItemSelected=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (ItemSelected==TRUE)
			{
				
				//CurrentPos=*(double*)GetSetMediaItemInfo(CurItem,"D_POSITION",NULL);
				//if (ItemCounter==0) MinTime=CurrentPos;
				//if (ItemCounter>0 && CurrentPos<MinTime) MinTime=CurrentPos;
				MediaItemsInProject[ItemCounter]=CurItem;
				ItemCounter++;
			}
			
			
		}
	}
	//MessageBox(g_hwndParent,"min search complete","info",MB_OK);
	//double yyy;
	Undo_BeginBlock();
	for (i=0;i<numItems;i++)
	{
		//
		Main_OnCommand(40289,0); // unselect all items
		CurItem=MediaItemsInProject[i];
		bool SelStatus=true;
		GetSetMediaItemInfo(CurItem,"B_UISEL",&SelStatus);
		double SelItemPos=*(double*)GetSetMediaItemInfo(CurItem,"D_POSITION",NULL);
		double SelItemLen=*(double*)GetSetMediaItemInfo(CurItem,"D_LENGTH",NULL);
		SetEditCurPos(SelItemPos,false,false); // edit cursor now at the beginning of selected item
		int NumSplits=0;
		double CurPos=GetCursorPosition();
		double LastCurPos;
		while (CurPos<=(SelItemLen+SelItemPos))
		{
			//
			//if (NumSplits>0) 
			LastCurPos=CurPos;
			Main_OnCommand(40375,0); // go to next transient in item
			Main_OnCommand(40012,0); // split at edit cursor
			CurPos=GetCursorPosition();
			
			if (LastCurPos==CurPos) NumSplits++;
				
			if (NumSplits>3) // we will believe after 3 futile iterations that this IS the last transient of the item 
			{
				//MessageBox(g_hwndParent,"Too many splits, likely an error!","Warning!",MB_OK);
				break;
			}
		}


	}
	//MessageBox(g_hwndParent,"no joo t‰‰ll‰ ollaan","Warning!",MB_OK);
	
	delete[] MediaItemsInProject;
	//adjustZoom(CurrentHZoom, 0, false, -1);
	UpdateTimeline();
	Undo_EndBlock("Split Items At Transients",0);
	//Undo_OnStateChangeEx("Split Items At Transients",4,-1);
	
}

void DoNudgeItemVols(bool UseConf,bool Positive,double TheNudgeAmount)
{
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	
	int numItems;
	double NudgeAmount=g_command_params.ItemVolumeNudge;
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
			ItemSelected=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (ItemSelected==TRUE)
			{
				double NewVol;
				double OldVol=*(double*)GetSetMediaItemInfo(CurItem,"D_VOL",NULL);
				double OldVolDB=20*(log10(OldVol));
				if (Positive)
					NewVol=OldVolDB+NudgeAmount; else NewVol=OldVolDB-NudgeAmount;
				double NewVolGain;
				if (NewVol>-144.0)
					NewVolGain=exp(NewVol*0.115129254);
				else
					NewVolGain=0;					
				GetSetMediaItemInfo(CurItem,"D_VOL",&NewVolGain);
			} 
		}
	}
	Undo_OnStateChangeEx("Nudge Item Volume",4,-1);
	UpdateTimeline();
}


void DoNudgeItemVolsDown(COMMAND_T*)
{
	DoNudgeItemVols(true,false,0.0);
}

void DoNudgeItemVolsUp(COMMAND_T*)
{
	DoNudgeItemVols(true,true,0.0);
}

void DoNudgeTakeVols(bool UseConf,bool Positive,double TheNudgeAmount)
{
	double NudgeAmount=g_command_params.ItemVolumeNudge;
	vector<MediaItem_Take*> ProjectTakes;
	XenGetProjectTakes(ProjectTakes,true,true);
	MediaItem_Take *CurTake;
	int i;
	for (i=0;i<(int)ProjectTakes.size();i++)
	{
		CurTake=ProjectTakes[i];
		double NewVol;
		double OldVol=*(double*)GetSetMediaItemTakeInfo(CurTake,"D_VOL",NULL);
		double OldVolDB=20*(log10(OldVol));
		if (Positive)
			NewVol=OldVolDB+NudgeAmount; else NewVol=OldVolDB-NudgeAmount;
		double NewVolGain;
		if (NewVol>-144.0)
			NewVolGain=exp(NewVol*0.115129254);
			else NewVolGain=0;					
		GetSetMediaItemTakeInfo(CurTake,"D_VOL",&NewVolGain);	
	}
	Undo_OnStateChangeEx("Nudge Take Volume",4,-1);
	UpdateTimeline();
}

void DoNudgeTakeVolsDown(COMMAND_T*)
{
	DoNudgeTakeVols(true,false,0.0);
}

void DoNudgeTakeVolsUp(COMMAND_T*)
{
	DoNudgeTakeVols(true,true,0.0);
}

void DoResetItemVol(COMMAND_T*)
{
	//
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	int numItems=666;
	bool ItemSelected=false;
	
	bool NewSelectedStatus=false;
	int i;
	int j;
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
				double NewVol=1.0;
				GetSetMediaItemInfo(CurItem,"D_VOL",&NewVol);
				
			} 


		}
	

	}
	Undo_OnStateChangeEx("Reset Item Volume To 0.0",4,-1);
	UpdateTimeline();
}

void DoResetTakeVol(COMMAND_T*)
{
	vector<MediaItem_Take*> ProjectTakes;
	XenGetProjectTakes(ProjectTakes,true,true);
	int i;
	for (i=0;i<(int)ProjectTakes.size();i++)
	{
		double NewVol=1.0;
		GetSetMediaItemTakeInfo(ProjectTakes[i],"D_VOL",&NewVol);
	}
	Undo_OnStateChangeEx("Reset Take Volume To 0.0",4,-1);
	UpdateTimeline();
}

char *FFT_sizes_strs[]=
{
  "256",
  "512",
  "1024",
  "2048",
  "4096",
  "8192",
  "16384",
  NULL
};

// TODO maybe eliminate this?
char *fftSizeTxt;

typedef struct
{
	int fftSize;
	double StretchFact;
	double Transpose;
} t_pvoc_params;

t_pvoc_params g_last_PVOC_Params;

bool g_PVOC_firstrun=true;
HWND g_hPVOCconfDlg=0;


void InitFFTWNDBox(HWND hwnd, char *buf)
{

  int x;
  int a=0;
  for (x = 0; FFT_sizes_strs[x]; x ++)
  {
    int r=(int)SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)FFT_sizes_strs[x]);
    if (!a && !strcmp(buf,FFT_sizes_strs[x]))
    {
      a=1;
      SendMessage(hwnd,CB_SETCURSEL,r,0);
    }
  }

  if (!a)
    SetWindowText(hwnd,buf);
}

void DoCSoundPvoc()
{
	//
	
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	MediaItem_Take* CurTake;
	PCM_source *ThePCMSource;
	int numItems=666;
	
	bool ItemSelected=false;
	
	bool NewSelectedStatus=false;
	bool FirstSelFound=false;
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
				if (GetMediaItemNumTakes(CurItem)>0)
				{
					CurTake=GetMediaItemTake(CurItem,-1);
				
					double NewVol=1.0;
					FirstSelFound=true;				
					
					ThePCMSource=(PCM_source*)GetSetMediaItemTakeInfo(CurTake,"P_SOURCE",NULL);
				}
				
			} 

			if (FirstSelFound) break;
		}
		
		if (FirstSelFound) break;
	}

	double StretchFact=g_last_PVOC_Params.StretchFact;
	double PitchFact=pow(2.0,g_last_PVOC_Params.Transpose/12.0);
	
	STARTUPINFO          si = { sizeof(si) };
	PROCESS_INFORMATION  pi;
	//char                 szExe[] = "cmd.exe";
	char CsAnalFilePath[1024];
	char ProjectPath[1024];
	char CsCMDline[512];
	char CsAnalFileName[512] ="/PVOCTEMPANALFILE.pvx";
	GetProjectPath(ProjectPath,1024);
	strcat(ProjectPath,CsAnalFileName);
	strcpy(CsAnalFilePath, ProjectPath);
	// fJvCreateProcess.CommandLine:='csound -U pvanal -n'+inttostr(fWindowsize)+' -d0.0 "'+FInputFile +'" "'+FAnalysisFileName+'"';
	double ItemLen=*(double*)GetSetMediaItemInfo(CurItem,"D_LENGTH",NULL);
	double OutDur=StretchFact*ItemLen; 
	//CsCMDline="csound -U pvanal -n4096 -d0.0 "" ""PVOCTestFile1.pvx""";
	// -b media offset -d dur to analyze
	
	//PCM_source *ThePCMSource=(PCM_source*)GetSetMediaItemTakeInfo(CurTake,"P_SOURCE",NULL);
	int SourceChans=ThePCMSource->GetNumChannels();
	double MediaOffset=*(double*)GetSetMediaItemTakeInfo(CurTake,"D_STARTOFFS",NULL);
	MediaItem_Take *OldTake=CurTake;
	//MessageBox(g_hwndParent,g_last_PVOC_Params.fftSize,"info",MB_OK);
	int FFTSize=g_last_PVOC_Params.fftSize;
	//
	sprintf(CsCMDline,"csound -d -U pvanal -n%d -b%f -d%f \"%s\" \"%s\"",FFTSize, MediaOffset,ItemLen,ThePCMSource->GetFileName(), CsAnalFilePath);
	bool CsoundSuccesfull=false;
	//MessageBox(g_hwndParent,CsCMDline,"info",MB_OK);
	SetWindowText(g_hPVOCconfDlg,"Analyzing sound...");

	if(CreateProcess(0, CsCMDline, 0, 0, FALSE, CREATE_NO_WINDOW, 0, 0, &si, &pi))
	{
		DWORD TheResult;
		// optionally wait for process to finish
		TheResult=WaitForSingleObject(pi.hProcess, 30000); // we will consider over 30 seconds too long time to take for the analysis
		if (TheResult==WAIT_TIMEOUT) 
		{
			CsoundSuccesfull=false;
		}
		//MessageBox(g_hwndParent,"csound anal finished!","info",MB_OK);
		SetWindowText(g_hPVOCconfDlg,"Analyzing finished!");
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		//CsCMDline="csound --omacro:STRETCHFACT=2.0 --smacro:OUTDUR=12.5 --omacro:PITCHFACT=1.0 -o""C:/ReaperPlugins/koe1.wav"" ""C:/ReaperPlugins/pieru1.csd""";
		
		char buf[2048];
		
		int x;
		for (x = 1; x < 1000; x ++) 
		{
			GetProjectPath(ProjectPath,1024);
			sprintf(buf,"\\PVOCrender %03d.wav",x);
			strcat(ProjectPath,buf);
			if (!PathFileExists(ProjectPath)) 
			{ 
				break;
			}
		}

		char CSDFileName[512];
		sprintf(CSDFileName,"\"%s/Plugins/PVOCStretch.csd\"",GetExePath());
		sprintf(CsCMDline,"csound --omacro:STRETCHFACT=%f --omacro:INPVFILE=\"%s\" --smacro:NUMINCHANS=%d --smacro:OUTDUR=%f --omacro:PITCHFACT=%f -fo\"%s\" %s",
			StretchFact,CsAnalFilePath,SourceChans,OutDur,PitchFact,ProjectPath,CSDFileName);
		SetWindowText(g_hPVOCconfDlg,"Processing sound...");
		Sleep(300);
		if(CreateProcess(0, CsCMDline, 0, 0, FALSE, CREATE_NO_WINDOW, 0, 0, &si, &pi))
		{
			// optionally wait for process to finish
			TheResult=WaitForSingleObject(pi.hProcess, 120000); // 120 seconds should be enough we are stuck with csound processing
			if (TheResult==WAIT_TIMEOUT) 
			{
				CsoundSuccesfull=false;
			}
			//MessageBox(g_hwndParent,"csound process finished!","info",MB_OK);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			if (TheResult==WAIT_OBJECT_0)
			{
				SetWindowText(g_hPVOCconfDlg,"Processing finished!");
					int LastTake=GetMediaItemNumTakes(CurItem);
				//
					MediaItem_Take *NewMediaTake=AddTakeToMediaItem(CurItem);
				PCM_source *NewPCMSource=PCM_Source_CreateFromFile(ProjectPath);
				NewPCMSource->Peaks_Clear(true);
				GetSetMediaItemTakeInfo(NewMediaTake,"P_SOURCE",NewPCMSource);
				GetSetMediaItemInfo(CurItem,"I_CURTAKE",&LastTake);
				GetSetMediaItemInfo(CurItem,"D_LENGTH",&OutDur);
				char BetterTakeName[512];
				char *OldTakeName=(char*)GetSetMediaItemTakeInfo(OldTake,"P_NAME",NULL);
				sprintf(BetterTakeName,"%s_PVOC_win%d_%.2fx_%.2fsemitones",OldTakeName,g_last_PVOC_Params.fftSize, StretchFact,g_last_PVOC_Params.Transpose);
				GetSetMediaItemTakeInfo(NewMediaTake,"P_NAME",BetterTakeName);
				Main_OnCommand(40047,0); // build any missing peaks
				SetForegroundWindow(g_hwndParent);
				Undo_OnStateChangeEx("Phase Vocode Item As New Take",4,-1);
			} else MessageBox(g_hwndParent,"Csound processed too long!","Error",MB_OK);
			//InsertMedia("C:/ReaperPlugins/koe1.wav",0);
		}
	}
	UpdateTimeline();
}

WDL_DLGRET CSPVOCItemDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	//
	switch(Message)
    {
        case WM_INITDIALOG:
			{
			char TextBuf[32];
			
			sprintf(TextBuf,"%.2f",g_last_PVOC_Params.StretchFact);
			SetDlgItemText(hwnd, IDC_EDIT1, TextBuf);
			sprintf(TextBuf,"%.2f",g_last_PVOC_Params.Transpose);
			SetDlgItemText(hwnd, IDC_EDIT2, TextBuf);
			//MessageBox(g_hwndParent,g_last_PVOC_Params.fftSize,"info",MB_OK);
			InitFFTWNDBox (GetDlgItem(hwnd,IDC_COMBO1),fftSizeTxt);
			g_hPVOCconfDlg=hwnd;
			//MessageBox(g_hwndParent,g_last_PVOC_Params.fftSize,"info",MB_OK);	
#ifdef _WIN32
			SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwnd, IDC_EDIT1), TRUE);
#endif
			//SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_SETCURSEL, 0, 0);
			//delete TextBuf;
			return 0;
			}
		case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
					{
						
						WDL_String *TempString=new WDL_String;
						//g_last_PVOC_Params.fftSize=
						GetDialogItemString(g_hPVOCconfDlg,IDC_COMBO1,TempString);
						//strcpy(fftSizeTxt,TempString->Get());
						if (fftSizeTxt) free (fftSizeTxt);
						fftSizeTxt=_strdup(TempString->Get());
						//strcpy(g_last_PVOC_Params.fftSize, TempString->Get());
						g_last_PVOC_Params.fftSize=atoi(TempString->Get());
						//MessageBox(g_hwndParent,g_last_PVOC_Params.fftSize,"info",MB_OK);
						GetDialogItemString(g_hPVOCconfDlg,IDC_EDIT1,TempString);
						g_last_PVOC_Params.StretchFact =strtod(TempString->Get(),NULL);
						GetDialogItemString(g_hPVOCconfDlg,IDC_EDIT2,TempString);
						g_last_PVOC_Params.Transpose =strtod(TempString->Get(),NULL);
						DoCSoundPvoc();
						delete TempString;
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



void DoShowPVocDlg(COMMAND_T*)
{
	//
	if (g_PVOC_firstrun==true)
	{
		fftSizeTxt="2048";
		g_last_PVOC_Params.fftSize=2048;
		g_last_PVOC_Params.StretchFact=2.0;
		g_last_PVOC_Params.Transpose=0.0;
		g_PVOC_firstrun=false;
	}

	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_CSPVOC_CONF), g_hwndParent, CSPVOCItemDlgProc);
}

typedef struct 
{
	double Scaling;
	double LengthScaling;
} t_ScaleItemPosParams;

t_ScaleItemPosParams g_last_ScaleItemPosParams;

double *g_StoredPositions;
double *g_StoredLengths;

void DoScaleItemPosStatic2(double TheScaling,double TheLengthScaling,bool RestorePos)
{
	//
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	
	int numItems=666;
	
	double PositionScalingFactor=TheScaling/100.0;
	double LenScalingFactor=TheLengthScaling/100.0;
	if (PositionScalingFactor<0.0001) PositionScalingFactor=0.0001;
	//if (PositionScalingFactor>1) PositionScalingFactor=1.0;

	
	bool ItemSelected=false;
	
	bool NewSelectedStatus=false;
	//double EditCurPos=GetCursorPosition();
	
	//MediaItem** MediaItemsOnTrack;
	
	// Find minimum item position from all selected
	double MinTime;
	double CurrentPos;
	int ItemCounter=0;
	int i;
	int j;
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
				
				CurrentPos=*(double*)GetSetMediaItemInfo(CurItem,"D_POSITION",NULL);
				if (ItemCounter==0) MinTime=CurrentPos;
				if (ItemCounter>0 && CurrentPos<MinTime) MinTime=CurrentPos;
				ItemCounter++;
			}
			
			
		}
	}
	//MessageBox(g_hwndParent,"min search complete","info",MB_OK);
	//double yyy;
	ItemCounter=0;
	

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
				//double SnapOffset=*(double*)GetSetMediaItemInfo(CurItem,"D_SNAPOFFSET",NULL);
				
				double TimeX;
				double NewPos;
				double NewLen;
				//double OldPos=*(double*)GetSetMediaItemInfo(MediaItemsOnTrack[j],"D_POSITION",NULL);
				double OldPos=g_StoredPositions[ItemCounter];
				double OldLen=g_StoredLengths[ItemCounter];
				if (RestorePos==false)
				{
					TimeX=OldPos-MinTime;
					NewPos=TimeX*PositionScalingFactor;
					NewPos=NewPos+MinTime;
					NewLen=OldLen*LenScalingFactor;
				} else 
				{ 
					NewPos=OldPos;
					NewLen=OldLen;
				}
					ItemCounter++;
				//if (Positive)
					//NewPos=OldPos+NudgeAmount; else NewPos=OldPos-NudgeAmount;
				//NewPos=MinTime+( (PositionScalingFactor)*(OldPos));
								
				GetSetMediaItemInfo(MediaItemsOnTrack[j],"D_POSITION",&NewPos);
				GetSetMediaItemInfo(MediaItemsOnTrack[j],"D_LENGTH",&NewLen);
				//GetSetMediaItemInfo(
				
				
			} 


		}
	delete[] MediaItemsOnTrack;

	}
	
}

HWND hPosScaler=0;
HWND hLenScaler=0;

WDL_DLGRET ScaleItemPosDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	//
	switch(Message)
    {
        case WM_INITDIALOG:
			{
#ifdef _WIN32
			hPosScaler = CreateWindowEx(WS_EX_LEFT, "REAPERhfader", "DLGFADER1", 
				WS_CHILD | WS_VISIBLE | TBS_VERT, 
				175, 5, 175, 25, hwnd, NULL, g_hInst, NULL);
			char TextBuf[32];
			hLenScaler = CreateWindowEx(WS_EX_LEFT, "REAPERhfader", "DLGFADER2", 
				WS_CHILD | WS_VISIBLE | TBS_VERT, 
				175, 40, 175, 25, hwnd, NULL, g_hInst, NULL);
			SendMessage(hPosScaler,TBM_SETTIC,0,500);
			SendMessage(hLenScaler,TBM_SETTIC,0,500);
			sprintf(TextBuf,"%.2f",g_last_ScaleItemPosParams.Scaling);
			SetDlgItemText(hwnd, IDC_EDIT1, TextBuf);
			sprintf(TextBuf,"%.2f",g_last_ScaleItemPosParams.LengthScaling);
			SetDlgItemText(hwnd, IDC_EDIT5, TextBuf);

			SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwnd, IDC_EDIT1), TRUE);
			
			return 0;
#else
			hPosScaler= SWELL_MakeControl("DLGFADER1", 667, "REAPERhfader", 0, 110, 5, 100, 10, 0);
			char TextBuf[32];
			sprintf(TextBuf,"%.2f",g_last_ScaleItemPosParams.Scaling);
			SetDlgItemText(hwnd, IDC_EDIT1, TextBuf);
#endif
			}
        case WM_HSCROLL:
			{
				
				char TextBuf[128];
				int SliderPos=(int)SendMessage((HWND)lParam,TBM_GETPOS,0,0);
				double PosScalFact=10+((190.0/1000)*SliderPos);
				if ((HWND)lParam==hPosScaler)
				{
					g_last_ScaleItemPosParams.Scaling=PosScalFact;
					sprintf(TextBuf,"%.2f",g_last_ScaleItemPosParams.Scaling);
					SetDlgItemText(hwnd, IDC_EDIT1, TextBuf);
				}
				if ((HWND)lParam==hLenScaler)
				{
					g_last_ScaleItemPosParams.LengthScaling=PosScalFact;
					sprintf(TextBuf,"%.2f",g_last_ScaleItemPosParams.LengthScaling);
					SetDlgItemText(hwnd, IDC_EDIT5, TextBuf);
				}
				DoScaleItemPosStatic2(g_last_ScaleItemPosParams.Scaling,g_last_ScaleItemPosParams.LengthScaling, false);
				UpdateTimeline();
				return 0;
			}

		case WM_COMMAND:
            switch(LOWORD(wParam))
            {
				case IDC_APPLY:
					{
						char textbuf[100];
						GetDlgItemText(hwnd,IDC_EDIT1,textbuf,100);
						g_last_ScaleItemPosParams.Scaling=atof(textbuf);
						GetDlgItemText(hwnd,IDC_EDIT5,textbuf,100);
						g_last_ScaleItemPosParams.LengthScaling=atof(textbuf);
						DoScaleItemPosStatic2(g_last_ScaleItemPosParams.Scaling,g_last_ScaleItemPosParams.LengthScaling, false);
						UpdateTimeline();
						return 0;
					}

				case IDOK:
					{
						char textbuf[100];
						GetDlgItemText(hwnd,IDC_EDIT1,textbuf,100);
						g_last_ScaleItemPosParams.Scaling=atof(textbuf);
						GetDlgItemText(hwnd,IDC_EDIT5,textbuf,100);
						g_last_ScaleItemPosParams.LengthScaling=atof(textbuf);
						DoScaleItemPosStatic2(g_last_ScaleItemPosParams.Scaling,g_last_ScaleItemPosParams.LengthScaling, false);
						Undo_OnStateChangeEx("Scale Item Positions By Static Percentage",4,-1);
						UpdateTimeline();
						EndDialog(hwnd,0);
						return 0;
					}
				case IDCANCEL:
					{
						//MessageBox(hwnd,"cancel pressed","info",MB_OK);
						DoScaleItemPosStatic2(g_last_ScaleItemPosParams.Scaling,g_last_ScaleItemPosParams.LengthScaling,true);
						UpdateTimeline();
						EndDialog(hwnd,0);
						return 0;
					}
			}
	}
	return 0;
}


void DoScaleItemPosStaticDlg(COMMAND_T*)
{
	if (g_ScaleItemPosFirstRun)
	{
		//
		g_last_ScaleItemPosParams.Scaling=100.0;
		g_ScaleItemPosFirstRun=false;
		
	}
	int NumSelItems=GetNumSelectedItems();
	g_StoredPositions=new double [NumSelItems];
	g_StoredLengths=new double[NumSelItems];
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	
	int numItems=666;
	
	
	bool ItemSelected=false;
	int ItemCounter=0;
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
				g_StoredPositions[ItemCounter]=*(double*)GetSetMediaItemInfo(CurItem,"D_POSITION",NULL);
				g_StoredLengths[ItemCounter]=*(double*)GetSetMediaItemInfo(CurItem,"D_LENGTH",NULL);
				ItemCounter++;
			}
		}
	}
	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_SCALEITEMPOS), g_hwndParent, ScaleItemPosDlgProc);
	delete[] g_StoredPositions;
	delete[] g_StoredLengths;
}

typedef struct 
{
	double RandRange;
} t_itemposrandparams;

t_itemposrandparams g_last_RandomizeItemPosParams;

bool g_RandItemPosFirstRun=true;

void DoRandomizePositions2()
{
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	int numItems;
	bool ItemSelected=false;
	double ItemPosition=-666;
	double NewPosition=0.0;
	double RandomSpread=0.5;
	RandomSpread=g_last_RandomizeItemPosParams.RandRange;
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
				ItemPosition=*(double*)GetSetMediaItemInfo(CurItem,"D_POSITION",NULL);
				double PositionDelta=-(RandomSpread)+(1.0/RAND_MAX)*rand()*(2.0*RandomSpread);
				NewPosition=ItemPosition+PositionDelta;
				if (NewPosition<0.0) NewPosition=0.0;
				GetSetMediaItemInfo(CurItem,"D_POSITION",&NewPosition);
			}

		}
	}
	Undo_OnStateChangeEx("Randomize Item Positions",4,-1);
	UpdateTimeline();
}

WDL_DLGRET RandomizeItemPosDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	//
	switch(Message)
    {
        case WM_INITDIALOG:
			{
			char TextBuf[32];
			
			sprintf(TextBuf,"%.2f",g_last_RandomizeItemPosParams.RandRange);
			SetDlgItemText(hwnd, IDC_EDIT1, TextBuf);
#ifdef _WIN32
			SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwnd, IDC_EDIT1), TRUE);
#endif
			return 0;
			}
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
					{
						char textbuf[100];
						GetDlgItemText(hwnd,IDC_EDIT1,textbuf,100);
						g_last_RandomizeItemPosParams.RandRange=atof(textbuf);
						DoRandomizePositions2();
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

void DoRandomizePositionsDlg(COMMAND_T*)
{
	if (g_RandItemPosFirstRun)
	{
		//
		g_last_RandomizeItemPosParams.RandRange=0.1;
		g_RandItemPosFirstRun=false;
		
	}
	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_RANDTEMPOS), g_hwndParent, RandomizeItemPosDlgProc);
}

typedef struct
{
	double TimeStep;
	double TimeRand;
	double MinValue;
	double MaxValue;
	double RndWalkSpread;
	int PointShape;
	int GenType;

} t_auto_gen_params;

t_auto_gen_params g_AutoGenParams;

char *g_ClipBFullBuf=NULL;

// TODO probably should fine-tooth this one
void CreateAutomationData()
{
	char LineBuf[257];
	if (!g_ClipBFullBuf)
		g_ClipBFullBuf = new char[500001];
	//memset(FullBuf,0,500001);
	//memset(LineBuf,0,256);
	int j=0;
	strcpy(LineBuf, "<ENVPTS");
	//
	//strcat(FullBuf,LineBuf);
	
	int Perse=(int)strlen(LineBuf);
	int i=0;
	for (i=0;i<Perse;i++) g_ClipBFullBuf[i]=LineBuf[i];
	
	j=j+Perse;
	g_ClipBFullBuf[j+1]='\0';
	j++;
	double TimeSelStart;
	double TimeSelEnd;
	
	GetSet_LoopTimeRange(false,false,&TimeSelStart,&TimeSelEnd,false);
	SetEditCurPos(TimeSelStart,false,false);
	double GenRange=TimeSelEnd-TimeSelStart;
	int GenNumPoints=(int)floor(GenRange/g_AutoGenParams.TimeStep)+1;
	for (i=0;i<GenNumPoints;i++)
	{
		double TimeRandRange=(g_AutoGenParams.TimeRand/100.0)*(g_AutoGenParams.TimeStep/2.0);
		double TimeRandDelta=TimeRandRange+((1.0/RAND_MAX)*rand()*(TimeRandRange*2));
		double PTTime=i*g_AutoGenParams.TimeStep+TimeRandDelta;
		double PTValue;
		if (g_AutoGenParams.GenType==0)
		{
			double RandRange=g_AutoGenParams.MaxValue-g_AutoGenParams.MinValue;
			PTValue=g_AutoGenParams.MinValue+((1.0/RAND_MAX)*rand()*(RandRange));
		}
		int PTSel=0;
		int PTShape=g_AutoGenParams.PointShape;		
		sprintf(LineBuf,"  PT %f %f %d %d",PTTime,PTValue,PTShape,PTSel);
		Perse=(int)strlen(LineBuf);
		int k;
		for (k=0;k<Perse;k++) 
		{ 
			g_ClipBFullBuf[j+k]=LineBuf[k];
			//j++;
		}
		j=j+Perse;
		g_ClipBFullBuf[j+1]='\0';
		j++;
		if (j>500000) MessageBox(g_hwndParent,"too much into clipboard buffer!","Error",MB_OK);
		//strncat(FullBuf,LineBuf,256);
		
	}
	strcpy(LineBuf,">");
	
	Perse=(int)strlen(LineBuf);
	//for (i=0;i<Perse;i++) FullBuf[i+j]=LineBuf[i];
	g_ClipBFullBuf[j]='>';
	//j=j+Perse;
	g_ClipBFullBuf[j+1]='\0';
	g_ClipBFullBuf[j+2]='\0';
	
	//strncat(FullBuf,LineBuf,256);
	//j=j+strlen(LineBuf)+1;

	UINT rmformat = RegisterClipboardFormat("REAPERmedia");

	if(OpenClipboard(g_hwndParent)>0)
	{
		//make some dummy data
		HGLOBAL clipbuffer;
		EmptyClipboard();
		clipbuffer = GlobalAlloc(GMEM_MOVEABLE, j+2);
		//char *buffer = (char*)GlobalLock(clipbuffer);
		//GlobalLock(clipbuffer);
		//put the data into that memory

		//buffer = FullBuf;
		memcpy(GlobalLock(clipbuffer),g_ClipBFullBuf,j+2);
		//Put it on the clipboard

		GlobalUnlock(clipbuffer);
		SetClipboardData(rmformat,clipbuffer);
		CloseClipboard();
		
		//
		Main_OnCommand(40058,0); // paste
		GlobalFree(clipbuffer);
		//delete buffer;
	}
}

bool g_autogen_firstrun=true;

WDL_DLGRET GenEnvesDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	//
	switch(Message)
    {
        case WM_INITDIALOG:
			{
			char TextBuf[32];
			sprintf(TextBuf,"%.2f",g_AutoGenParams.MaxValue);
			SetDlgItemText(hwnd, IDC_EDIT1, TextBuf);
			sprintf(TextBuf,"%.2f",g_AutoGenParams.MinValue);
			SetDlgItemText(hwnd, IDC_EDIT2, TextBuf);
			sprintf(TextBuf,"%.2f",1.0/g_AutoGenParams.TimeStep);
			SetDlgItemText(hwnd, IDC_EDIT4, TextBuf);
			sprintf(TextBuf,"%.2f",g_AutoGenParams.TimeRand);
			SetDlgItemText(hwnd, IDC_EDIT5, TextBuf);
			sprintf(TextBuf,"%d",g_AutoGenParams.PointShape);
			SetDlgItemText(hwnd, IDC_EDIT3, TextBuf);
			//sprintf(TextBuf,"%.2f",g_last_RandomizeItemPosParams.RandRange);
			//SetDlgItemText(hwnd, IDC_EDIT1, "440.0");
#ifdef _WIN32
			SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwnd, IDC_EDIT2), TRUE);
#endif
			return 0;
			}
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
					{
						char textbuf[100];
						GetDlgItemText(hwnd,IDC_EDIT1,textbuf,100);
						g_AutoGenParams.MaxValue=atof(textbuf);
						GetDlgItemText(hwnd,IDC_EDIT2,textbuf,100);
						
						g_AutoGenParams.MinValue=atof(textbuf);
						GetDlgItemText(hwnd,IDC_EDIT4,textbuf,100);
						g_AutoGenParams.TimeStep=1.0/atof(textbuf);
						GetDlgItemText(hwnd,IDC_EDIT3,textbuf,100);
						g_AutoGenParams.PointShape=atoi(textbuf);
						GetDlgItemText(hwnd,IDC_EDIT5,textbuf,100);
						g_AutoGenParams.TimeRand=atof(textbuf);
						CreateAutomationData();
						//GetDlgItemText(hwnd,IDC_EDIT1,textbuf,100);
						//g_last_RandomizeItemPosParams.RandRange=atof(textbuf);
						//DoRandomizePositions2();
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

void DoInsertRandomEnvelopePoints(COMMAND_T*)
{
	if (!g_ClipBFullBuf)
		g_ClipBFullBuf=new char[500001];
	if (g_autogen_firstrun)
	{
		g_autogen_firstrun=false;
		g_AutoGenParams.GenType=0;
		g_AutoGenParams.MinValue=0;
		g_AutoGenParams.MaxValue=1;
		g_AutoGenParams.PointShape=0;
		g_AutoGenParams.RndWalkSpread=0.1;
		g_AutoGenParams.TimeStep=0.5;
		g_AutoGenParams.TimeRand=0.0;
	}

	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_GEN_ENV1), g_hwndParent, GenEnvesDlgProc);
	delete[] g_ClipBFullBuf;
	g_ClipBFullBuf = NULL;
}

#define IDC_FIRSTMIXSTRIP 101

typedef struct
{
	int NumTakes;
	HWND *g_hVolSliders;
	HWND *g_hPanSliders;
	HWND g_hItemVolSlider;
	HWND *g_hNumLabels;
	HWND *g_hMedOffsSliders;
	double *StoredVolumes;
	double *StoredPans;
	double StoredItemVolume;
	double *StoredMedOffs;
	bool AllTakesPlay;
} t_TakeMixerState;

t_TakeMixerState g_TakeMixerState;

MediaItem *g_TargetItem;

void UpdateTakeMixerSliders()
{
	//
	int SliPos;
	int i;
	for (i=0;i<g_TakeMixerState.NumTakes;i++)
	{
		//
		SliPos=(int)(1000.0/2*g_TakeMixerState.StoredVolumes[i]);
		//SliPos=SliPos;
		SendMessage(g_TakeMixerState.g_hVolSliders[i],TBM_SETPOS,(WPARAM) (BOOL)true,SliPos);
		double PanPos=g_TakeMixerState.StoredPans[i];
		PanPos=PanPos+1.0;
		SliPos=(int)(1000.0/2*PanPos);
		SendMessage(g_TakeMixerState.g_hPanSliders[i],TBM_SETPOS,(WPARAM) (BOOL)true,SliPos);
	}
	SliPos=(int)(1000*g_TakeMixerState.StoredItemVolume);
	//SliPos=100-SliPos;
	SendMessage(g_TakeMixerState.g_hItemVolSlider,TBM_SETPOS,(WPARAM) (BOOL)true,SliPos);
}


void On_SliderMove(HWND theHwnd,WPARAM wParam,LPARAM lParam,HWND SliderHandle,int TheSlipos)
{
	int x=-1;
	int movedParam=-1; // 0 vol , 1 pan
	int i;
	for (i=0;i<g_TakeMixerState.NumTakes;i++)
	{
		//if ((HWND)thelParam==g_TakeMixerState.g_hVolSliders[i])
		if (SliderHandle==g_TakeMixerState.g_hVolSliders[i])
		
		{
			movedParam=0;
			x=i;
			break;
		}
		if (SliderHandle==g_TakeMixerState.g_hPanSliders[i])
		{
			movedParam=1;
			x=i;
			break;
		}
	}
	MediaItem_Take *CurTake;
	if (x!=-1)
	{
		CurTake=GetMediaItemTake(g_TargetItem,x);
		GetSetMediaItemInfo(g_TargetItem,"I_CURTAKE",&x);
		if (movedParam==0)
		{
			//double NewVol=2.0-( 2.0/100*HIWORD(thewParam));
			double NewVol=( 2.0/1000*TheSlipos);
			
			GetSetMediaItemTakeInfo(CurTake,"D_VOL",&NewVol);
		}
		if (movedParam==1)
		{
			//double NewPan=-1.0+(2.0/100*HIWORD(thewParam));
			double NewPan=-1.0+(2.0/1000*TheSlipos);
			GetSetMediaItemTakeInfo(CurTake,"D_PAN",&NewPan);
		}

			//double NewPan=1.0-(2.0/100*HIWORD(
		UpdateTimeline();
	}
	if (SliderHandle==g_TakeMixerState.g_hItemVolSlider)
	{
		//
		double NewVol=( 1.0/1000*TheSlipos);
		GetSetMediaItemInfo(g_TargetItem,"D_VOL",&NewVol);
		UpdateTimeline();
	}
}

void TakeMixerResetTakes(bool ResetVol=false,bool ResetPan=false)
{
	//
	MediaItem_Take *CurTake;
	int i;
	for (i=0;i<GetMediaItemNumTakes(g_TargetItem);i++)
	{
		//
		CurTake=GetMediaItemTake(g_TargetItem,i);
		double NewVolume=1.0;
		double NewPan=0.0;
		if (ResetVol==true)
			GetSetMediaItemTakeInfo(CurTake,"D_VOL",&NewVolume);
		if (ResetPan==true)
			GetSetMediaItemTakeInfo(CurTake,"D_PAN",&NewPan);
	}
	int SliPos;
	
	for (i=0;i<g_TakeMixerState.NumTakes;i++)
	{
		//
		SliPos=(int)(1000.0/2*1.0);
		//SliPos=100-SliPos;
		if (ResetVol==true)
			SendMessage(g_TakeMixerState.g_hVolSliders[i],TBM_SETPOS,(WPARAM) (BOOL)true,SliPos);
		double PanPos;
		PanPos=1.0;
		SliPos=(int)(1000.0/2*PanPos);
		if (ResetPan==true)	
			SendMessage(g_TakeMixerState.g_hPanSliders[i],TBM_SETPOS,(WPARAM) (BOOL)true,SliPos);
	}

}


WDL_DLGRET TakeMixerDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
    {
        case WM_INITDIALOG:
			{
			RECT MyRect;
			GetWindowRect(hwnd,&MyRect);
			RECT MyRect2;
			GetWindowRect(GetDlgItem(hwnd,IDCANCEL),&MyRect2);
			int MinDialogWidth=MyRect2.right;
			MinDialogWidth=185;
			int NewDialogWidth=(1+g_TakeMixerState.NumTakes)*50+55;
			if (NewDialogWidth<MinDialogWidth) NewDialogWidth=MinDialogWidth;
			SetWindowPos(hwnd,HWND_TOP,0,0,NewDialogWidth,MyRect.bottom-MyRect.top, SWP_NOMOVE);
			GetWindowRect(GetDlgItem(hwnd,IDC_STRIPCONTAINER),&MyRect);
			SetWindowPos(GetDlgItem(hwnd,IDC_STRIPCONTAINER) ,HWND_TOP,0,0,NewDialogWidth-20,MyRect.bottom-MyRect.top, SWP_NOMOVE);
			char textbuf[300];
			int i;
			for (i=0;i<g_TakeMixerState.NumTakes;i++)
			{
				
				sprintf(textbuf,"TAKESTATIC_%d",i+1);
				g_TakeMixerState.g_hNumLabels[i] = CreateWindowEx(WS_EX_LEFT, "STATIC", textbuf, 
				WS_CHILD | WS_VISIBLE, 
				45+(1+i)*50, 170, 40, 13, hwnd, NULL, g_hInst, NULL);
				sprintf(textbuf,"%d",i+1);
				SetWindowText(g_TakeMixerState.g_hNumLabels[i],textbuf);
				
				sprintf(textbuf,"VOLSLIDER_%d",i+1);
				g_TakeMixerState.g_hVolSliders[i] = CreateWindowEx(WS_EX_LEFT, "REAPERvfader", textbuf, 
				WS_CHILD | WS_VISIBLE | TBS_VERT, 
				40+(1+i)*50, 95, 30, 120, hwnd, NULL, g_hInst, NULL);
				SendMessage(g_TakeMixerState.g_hVolSliders[i],TBM_SETTIC,0,500);
				
				sprintf(textbuf,"PANSLIDER_%d",i+1);
				g_TakeMixerState.g_hPanSliders[i] = CreateWindowEx(WS_EX_LEFT, "REAPERhfader", textbuf,
				WS_CHILD | WS_VISIBLE | TBS_HORZ, 
				33+(1+i)*50, 55, 45, 25, hwnd, NULL, g_hInst, NULL);
				SendMessage(g_TakeMixerState.g_hPanSliders[i],TBM_SETTIC,0,500);
				
				sprintf(textbuf,"OFFSETSLIDER_%d",i+1);
				g_TakeMixerState.g_hMedOffsSliders[i] = CreateWindowEx(WS_EX_LEFT, "REAPERhfader", textbuf, 
				WS_CHILD | WS_VISIBLE | TBS_HORZ, 
				33+(1+i)*50, 25, 45, 25, hwnd, NULL, g_hInst, NULL);
				
			}

			g_TakeMixerState.g_hItemVolSlider = CreateWindowEx(WS_EX_LEFT, "REAPERvfader", "ITEMVOLSLIDER", 
			WS_CHILD | WS_VISIBLE | TBS_VERT, 
			40, 95, 30, 120, hwnd, NULL, g_hInst, NULL);
			SendMessage(g_TakeMixerState.g_hItemVolSlider,TBM_SETTIC,0,500);
			UpdateTakeMixerSliders();
			return 0;
			}
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
				case IDC_BUTTON1:
					{
						TakeMixerResetTakes(true,false); // true for volume, false for pan
						UpdateTimeline();
						return 0;
					}
				case IDC_BUTTON2:
					{
						TakeMixerResetTakes(false,true); // true for volume, false for pan
						UpdateTimeline();
						return 0;
					}

				case IDOK:
					{
						//MessageBox(hwnd,"ok pressed","Info",MB_OK);
						Undo_OnStateChangeEx("Change take vol/pan",4,-1);
						EndDialog(hwnd,0);
						return 0;

					}
				case IDCANCEL:
					{
						//MessageBox(hwnd,"cancel pressed","Info",MB_OK);	
						MediaItem_Take *CurTake;
						int i;
						for (i=0;i<g_TakeMixerState.NumTakes;i++)
						{
							//
							CurTake=GetMediaItemTake(g_TargetItem,i);
							double NewVol=g_TakeMixerState.StoredVolumes[i];
							GetSetMediaItemTakeInfo(CurTake,"D_VOL",&NewVol);
							double NewPan=g_TakeMixerState.StoredPans[i];
							GetSetMediaItemTakeInfo(CurTake,"D_PAN",&NewPan);
						}
						GetSetMediaItemInfo(g_TargetItem,"B_ALLTAKESPLAY",&g_TakeMixerState.AllTakesPlay);
						GetSetMediaItemInfo(g_TargetItem,"D_VOL",&g_TakeMixerState.StoredItemVolume);
						UpdateTimeline();
						EndDialog(hwnd,0);
						return 0;
					}
			}
		case WM_VSCROLL:
			{
				int ThePos=(int)SendMessage((HWND)lParam,TBM_GETPOS,0,0);
				On_SliderMove(hwnd,wParam,lParam,(HWND)lParam,ThePos);
				switch(LOWORD(wParam))
				{
					case SB_THUMBTRACK:
						{
							//
							//On_SliderMove(hwnd,wParam,lParam);
							return 0;								
						}
					case SB_THUMBPOSITION:
						{
							//On_SliderMove(hwnd,wParam,lParam);
							return 0;
						}
				}
			}
		case WM_HSCROLL:
			{
				int ThePos=(int)SendMessage((HWND)lParam,TBM_GETPOS,0,0);
				On_SliderMove(hwnd,wParam,lParam,(HWND)lParam,ThePos);
				switch(LOWORD(wParam))
				{
					case SB_THUMBPOSITION:
						{
							//On_SliderMove(hwnd,wParam,lParam);
							return 0;
						}
				}
			}
	}
	return 0;
}

void DoShowTakeMixerDlg(COMMAND_T*)
{
	//
	g_TargetItem=NULL;
	int numSel=GetNumSelectedItems();
	
	if (numSel==1)
	{
		MediaTrack* MunRaita;
	MediaItem* CurItem;
	
	int numItems=666;
	
	
	bool ItemSelected=false;
	
	bool NewSelectedStatus=false;
	bool FirstItemFound=false;
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
				FirstItemFound=true;
				break;
				//NewSelectedStatus=FALSE;
				//GetSetMediaItemInfo(CurItem,"B_UISEL",&NewSelectedStatus);
				
			} 

			
		}
		if (FirstItemFound==true) break;

	}
	g_TargetItem=CurItem;	
	g_TakeMixerState.NumTakes=GetMediaItemNumTakes(CurItem);
	g_TakeMixerState.AllTakesPlay=*(bool*)GetSetMediaItemInfo(CurItem,"B_ALLTAKESPLAY",NULL);
	bool DummyBool=true;
	GetSetMediaItemInfo(CurItem,"B_ALLTAKESPLAY",&DummyBool);
	g_TakeMixerState.StoredItemVolume=*(double*)GetSetMediaItemInfo(CurItem,"D_VOL",NULL);
	g_TakeMixerState.g_hVolSliders=new HWND[g_TakeMixerState.NumTakes+1];
	g_TakeMixerState.g_hNumLabels=new HWND[g_TakeMixerState.NumTakes];
	g_TakeMixerState.g_hPanSliders=new HWND[g_TakeMixerState.NumTakes];
	g_TakeMixerState.g_hMedOffsSliders=new HWND[g_TakeMixerState.NumTakes];
	g_TakeMixerState.StoredVolumes=new double[g_TakeMixerState.NumTakes];
	g_TakeMixerState.StoredPans=new double[g_TakeMixerState.NumTakes];
	g_TakeMixerState.StoredMedOffs=new double[g_TakeMixerState.NumTakes];
		MediaItem_Take *CurTake;
		for (i=0;i<g_TakeMixerState.NumTakes;i++)
		{
			//
			CurTake=GetMediaItemTake(CurItem,i);
			double TheVol=*(double*)GetSetMediaItemTakeInfo(CurTake,"D_VOL",NULL);
			double ThePan=*(double*)GetSetMediaItemTakeInfo(CurTake,"D_PAN",NULL);
			double TheOffs=*(double*)GetSetMediaItemTakeInfo(CurTake,"D_STARTOFFS",NULL);
			g_TakeMixerState.StoredVolumes[i]=TheVol;
			g_TakeMixerState.StoredPans[i]=ThePan;
			g_TakeMixerState.StoredMedOffs[i]=TheOffs;
		}

		DialogBox(g_hInst,MAKEINTRESOURCE(IDD_TAKEMIXER), g_hwndParent, TakeMixerDlgProc);
		for (i=0;i<g_TakeMixerState.NumTakes;i++)
		{
			DestroyWindow(g_TakeMixerState.g_hVolSliders[i]);
			DestroyWindow(g_TakeMixerState.g_hPanSliders[i]);
			DestroyWindow(g_TakeMixerState.g_hNumLabels[i]);
			DestroyWindow(g_TakeMixerState.g_hMedOffsSliders[i]);
		}
		DestroyWindow(g_TakeMixerState.g_hItemVolSlider);
		delete[] g_TakeMixerState.g_hVolSliders;
		delete[] g_TakeMixerState.g_hPanSliders;
		delete[] g_TakeMixerState.g_hNumLabels;
		delete[] g_TakeMixerState.g_hMedOffsSliders;
		delete[] g_TakeMixerState.StoredPans;
		delete[] g_TakeMixerState.StoredVolumes;
		delete[] g_TakeMixerState.StoredMedOffs;
	} else MessageBox(g_hwndParent,"Currently only 1 selected item supported!","Info",MB_OK);
	
}

double g_PrevEditCursorPos=-1.0;

void DoSplitAndChangeLen(bool BeatBased)
{
	double StoredTimeSelStart;
	double StoredTimeSelEnd;
	GetSet_LoopTimeRange(false,false,&StoredTimeSelStart,&StoredTimeSelEnd,false);
	Undo_BeginBlock();
	Main_OnCommand(40012,0); // split item at edit cursor
	if (BeatBased==false)
	{
		double NewStart=GetCursorPosition();
		double NewEnd=NewStart+g_command_params.ItemPosNudgeSecs;
		GetSet_LoopTimeRange(true,false,&NewStart,&NewEnd,false);
	}
	if (BeatBased==true)
	{
					//
		double NewStartSecs=GetCursorPosition();
		double NewStartBeats=TimeMap_timeToQN(NewStartSecs);
		//ComboBox_GetText(g_hNudgeComboBox,NudgeComboText,200);
		double BeatNudgeValFromCombo;
		int NudgeComboSel=ComboBox_GetCurSel(g_hNudgeComboBox);
		ComboBox_GetText(g_hNudgeComboBox,NudgeComboText,200);
		NudgeComboSel=ComboBox_FindStringExact(g_hNudgeComboBox,-1,NudgeComboText);
		//t_Notevalue_struct paska;
		//paska=g_NoteValues[NudgeComboSel];
		if (NudgeComboSel!=CB_ERR)
			BeatNudgeValFromCombo=GetBeatValueFromTable(NudgeComboSel);
		else BeatNudgeValFromCombo=parseFrac(NudgeComboText);
		double NewEndBeats=NewStartBeats+BeatNudgeValFromCombo;
		//double NewEndBeats=NewStartBeats+g_command_params.ItemPosNudgeBeats;
		double NewEndSecs=TimeMap_QNToTime(NewEndBeats);
					
		GetSet_LoopTimeRange(true,false,&NewStartSecs,&NewEndSecs,false);
	}


	Main_OnCommand(40061,0); // split item at time selection
	Main_OnCommand(40006,0); // remove selected items
	Undo_EndBlock("Erase time from item",0);
	//Main_OnCommand(40635,0); // remove time selection
	GetSet_LoopTimeRange(true,false,&StoredTimeSelStart,&StoredTimeSelEnd,false);
}


void DoHoldKeyTest1(COMMAND_T*)
{
	double EditCurPos=GetCursorPosition();
	if (EditCurPos!=g_PrevEditCursorPos)
	{
		DoSplitAndChangeLen(false);
		g_PrevEditCursorPos=EditCurPos;
	}
}

void DoHoldKeyTest2(COMMAND_T*)
{
	double EditCurPos=GetCursorPosition();
	if (EditCurPos!=g_PrevEditCursorPos)
	{
		DoSplitAndChangeLen(true);
		g_PrevEditCursorPos=EditCurPos;
	} 
}

void DoInsertMediaFromClipBoard(COMMAND_T*)
{
	char clipstring[4096];
	
	if ( OpenClipboard(g_hwndParent) ) 
	{
	//get the buffer

		HANDLE hData = GetClipboardData(CF_TEXT);
		char * buffer = (char *)GlobalLock( hData );

		//make a local copy
		strcpy(clipstring,buffer);
		
		GlobalUnlock( hData );
		CloseClipboard();
		InsertMedia(clipstring,0);
	}
}

typedef struct
{
	bool MatchesCrit;
	int MatchingTake;
	MediaItem *ReaperItem;
	
	MediaItem_Take **Takes;
} t_reaper_item;

t_reaper_item *g_Project_Items;
int g_NumProjectItems=0;

void ConstructItemDatabase()
{
	//
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	MediaItem_Take* CurTake;
	//int numItems=666;
	
	
	bool ItemSelected=false;
	
	bool NewSelectedStatus=false;
	//double EditcurPos=GetCursorPosition();
	int NumItems=0;
	int i;
	for (i=0;i<GetNumTracks();i++)
	{
		MunRaita = CSurf_TrackFromID(i+1,FALSE);
		NumItems+=GetTrackNumMediaItems(MunRaita);
	}
	
	g_Project_Items=new t_reaper_item[NumItems];
	g_NumProjectItems=NumItems;
	NumItems=0;
	int ItemCounter=0;
	for (i=0;i<GetNumTracks();i++)
	{
		MunRaita = CSurf_TrackFromID(i+1,FALSE);
		NumItems=GetTrackNumMediaItems(MunRaita);
		int j;
		for (j=0;j<NumItems;j++)
		{
			CurItem = GetTrackMediaItem(MunRaita,j);
			//propertyName="D_";
			int NumTakes=GetMediaItemNumTakes(CurItem);
			g_Project_Items[ItemCounter].ReaperItem=CurItem;
			g_Project_Items[ItemCounter].Takes=new MediaItem_Take*[NumTakes];
			g_Project_Items[ItemCounter].MatchesCrit=false;
			g_Project_Items[ItemCounter].MatchingTake=-1;
			int k;
			for (k=0;k<NumTakes;k++)
			{
				CurTake=GetMediaItemTake(CurItem,k);
				g_Project_Items[ItemCounter].Takes[k]=CurTake;
			}
			ItemCounter++;
				
				
		} 

		
	}
	
	
}

void TokenizeString(const string& str, vector<string>& tokens, const string& delimiters)
{
    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    string::size_type pos     = str.find_first_of(delimiters, lastPos);

    while (string::npos != pos || string::npos != lastPos)
    {
        // Found a token, add it to the vector.
		string tulos;
		tulos=str.substr(lastPos, pos - lastPos);
		tokens.push_back(str.substr(lastPos, pos - lastPos));
		
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}


int PerformTakeSearch(char *SearchString)
{
	char *TakeName;
	MediaItem_Take *CurTake;
	int NumMatches=0;
	string MyString;
	string MyString2;
	vector<string> filtertokens;
	MyString2.assign(SearchString);
	transform(MyString2.begin(), MyString2.end(), MyString2.begin(), (int(*)(int)) toupper);
	TokenizeString(MyString2,filtertokens);
	int i;
	int k;
	int tokenCount;
	for (i=0;i<g_NumProjectItems;i++)
	{
		int NumTakes=GetMediaItemNumTakes(g_Project_Items[i].ReaperItem);
		for (k=0;k<NumTakes;k++)
		{
			CurTake=g_Project_Items[i].Takes[k];
			TakeName=(char*)GetSetMediaItemTakeInfo(CurTake,"P_NAME",NULL);
			MyString.assign(TakeName);
			transform(MyString.begin(), MyString.end(), MyString.begin(), (int(*)(int)) toupper);
			int NumTokenMatches=0;
			for (tokenCount=0;tokenCount<(int)filtertokens.size();tokenCount++)
			{
				if (MyString.find(filtertokens[tokenCount],0)!=string::npos) 
					NumTokenMatches++;
				else
					break; // any mismatch makes more work redundant
			}
			if (NumTokenMatches==filtertokens.size())
			{
				NumMatches++;
				bool dummyBool=true;
				GetSetMediaItemInfo(g_Project_Items[i].ReaperItem,"B_UISEL",&dummyBool);
			} else
			{
				bool dummyBool=false;
				GetSetMediaItemInfo(g_Project_Items[i].ReaperItem,"B_UISEL",&dummyBool);
			}
		}
	}
	UpdateTimeline();
	return NumMatches;
}


WDL_DLGRET TakeFinderDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
    {
        case WM_INITDIALOG:
			{
				char labtxt[256];
#ifdef _WIN32
				SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwnd, IDC_EDIT1), TRUE);				
#endif
				sprintf(labtxt,"Search from %d items",g_NumProjectItems);
				SetDlgItemText(hwnd,IDC_STATIC1,labtxt);
				return 0;
			}
		case WM_COMMAND:
			if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDC_EDIT1)
				{
					Main_OnCommand(40289,0); // unselect all items
					char labtxt[256];
					GetDlgItemText(hwnd,IDC_EDIT1,labtxt,256);
					int len=(int)strlen(labtxt);
					if (len>0)
					{
						int NumMatches=PerformTakeSearch(labtxt);
						sprintf(labtxt,"Found %d matching items",NumMatches);
						SetDlgItemText(hwnd,IDC_STATIC1,labtxt);
						if (NumMatches>0) 
						{
							Main_OnCommand(40290,0); // time selection to selected items
							Main_OnCommand(40031,0); // zoom to time selection
						}

					}
					return 0;
				}
			switch(LOWORD(wParam))
            {
				case IDOK:
					
					{
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


void DoSearchTakesDLG(COMMAND_T*)
{
	//
	ConstructItemDatabase();
	DialogBox(g_hInst,MAKEINTRESOURCE(IDD_TAKEFINDER), g_hwndParent, TakeFinderDlgProc);
	delete[] g_Project_Items;
}

void DoMoveCurConfPixRight(COMMAND_T*)
{
	//
	int i;
	for (i=0;i<g_command_params.PixAmount;i++)
	{
		Main_OnCommand(40105,0);
	}
}

void DoMoveCurConfPixLeft(COMMAND_T*)
{
	int i;
	for (i=0;i<g_command_params.PixAmount;i++)
	{
		Main_OnCommand(40104,0);
	}
}

void DoMoveCurConfPixRightCts(COMMAND_T*)
{
	int i;
	for (i=0;i<g_command_params.PixAmount;i++)
	{
		Main_OnCommand(40103,0);
	}
}

void DoMoveCurConfPixLeftCts(COMMAND_T*)
{
	int i;
	for (i=0;i<g_command_params.PixAmount;i++)
	{
		Main_OnCommand(40102,0);
	}	
}

void DoMoveCurConfSecsLeft(COMMAND_T*)
{
	double CurPos=GetCursorPosition();
	double NewPos=CurPos-g_command_params.CurPosSecsAmount;
	SetEditCurPos(NewPos,false,false);

}

void DoMoveCurConfSecsRight(COMMAND_T*)
{
	double CurPos=GetCursorPosition();
	double NewPos=CurPos+g_command_params.CurPosSecsAmount;
	SetEditCurPos(NewPos,false,false);
}

double g_StoreEditCurPos=0.0;

void DoStoreEditCursorPosition(COMMAND_T*)
{
	g_StoreEditCurPos=GetCursorPosition();	
}

void DoRecallEditCursorPosition(COMMAND_T*)
{
	SetEditCurPos(g_StoreEditCurPos,false,false);	
}

