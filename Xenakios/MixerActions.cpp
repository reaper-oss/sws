/******************************************************************************
/ MixerActions.cpp
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

int XenGetProjectTracks(t_vect_of_Reaper_tracks &RefVecTracks,bool OnlySelected)
{
	MediaTrack *CurTrack;
	RefVecTracks.clear();
	int i;
	for (i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		if (CurTrack)
		{
			int issel=*(int*)GetSetMediaTrackInfo(CurTrack,"I_SELECTED",NULL);
			if (OnlySelected && issel==1) RefVecTracks.push_back(CurTrack);
			if (!OnlySelected) RefVecTracks.push_back(CurTrack);
		}
	}
	return (int)RefVecTracks.size();
}

void DoSelectTrack(int tkOffset, bool KeepCurrent)
{
	vector<int> OldSelectedTracks;
	MediaTrack *CurTrack;
	int i;
	for (i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		int tkSel=*(int*)GetSetMediaTrackInfo(CurTrack,"I_SELECTED",NULL);
		if (tkSel==1)
		{
			int tkIndex=i;
			OldSelectedTracks.push_back(tkIndex);
			int newSel=0;
			GetSetMediaTrackInfo(CurTrack,"I_SELECTED",&newSel);
		}
	}
	vector<int> NewSelectedTracks;
	for (i=0;i<(int)OldSelectedTracks.size();i++)
	{
		int NewIndex=OldSelectedTracks[i]+tkOffset;
		if ((NewIndex>=0) && (NewIndex<GetNumTracks()))
			NewSelectedTracks.push_back(NewIndex);
	}
	for (i=0;i<(int)NewSelectedTracks.size();i++)
	{
		CurTrack=CSurf_TrackFromID(NewSelectedTracks[i]+1,false);	
		int newSel=1;
		GetSetMediaTrackInfo(CurTrack,"I_SELECTED",&newSel);
	}
	if (KeepCurrent==true)
	{
		for (i=0;i<(int)OldSelectedTracks.size();i++)
		{
			CurTrack=CSurf_TrackFromID(OldSelectedTracks[i]+1,false);	
			int newSel=1;
			GetSetMediaTrackInfo(CurTrack,"I_SELECTED",&newSel);	
		}
	}
}


void DoSelectNextTrack(COMMAND_T*)
{
	DoSelectTrack(1,false);
}

void DoSelectPreviousTrack(COMMAND_T*)
{
	DoSelectTrack(-1,false);
}

void DoSelectNextTrackKeepCur(COMMAND_T*)
{
	DoSelectTrack(1,true);
}

void DoSelectPrevTrackKeepCur(COMMAND_T*)
{
	DoSelectTrack(-1,true);
}

void DoToggleTraxVisMixer(COMMAND_T*)
{
	//
	MediaTrack *CurTrack;
	int i;
	for (i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		int TkSelected=*(int*)GetSetMediaTrackInfo(CurTrack,"I_SELECTED",NULL);
		if (TkSelected==1)
		{
			bool ShowInMixer=*(bool*)GetSetMediaTrackInfo(CurTrack,"B_SHOWINMIXER",NULL);
			ShowInMixer=!ShowInMixer;
			GetSetMediaTrackInfo(CurTrack,"B_SHOWINMIXER",&ShowInMixer);
		}
	}
	TrackList_AdjustWindows(false);
	UpdateTimeline();
}

void DoTraxPanLawDefault(COMMAND_T*)
{
	//
	MediaTrack *CurTrack;
	int i;
	for (i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		int TkSelected=*(int*)GetSetMediaTrackInfo(CurTrack,"I_SELECTED",NULL);
		if (TkSelected==1)
		{
			//bool ShowInMixer=*(bool*)GetSetMediaTrackInfo(CurTrack,"B_SHOWINMIXER",NULL);
			//ShowInMixer=!ShowInMixer;
			double NewLaw=-1.0;
			GetSetMediaTrackInfo(CurTrack,"D_PANLAW",&NewLaw);
		}



	}
	UpdateTimeline();
}

void DoTraxPanLawZero(COMMAND_T*)
{
	//
	MediaTrack *CurTrack;
	int i;
	for (i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		int TkSelected=*(int*)GetSetMediaTrackInfo(CurTrack,"I_SELECTED",NULL);
		if (TkSelected==1)
		{
			//bool ShowInMixer=*(bool*)GetSetMediaTrackInfo(CurTrack,"B_SHOWINMIXER",NULL);
			//ShowInMixer=!ShowInMixer;
			double NewLaw=1.0;
			GetSetMediaTrackInfo(CurTrack,"D_PANLAW",&NewLaw);
		}



	}
	UpdateTimeline();
}

void DoTraxRecArmed(COMMAND_T*)
{
	MediaTrack *CurTrack;
	int i;
	for (i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		int TkSelected=*(int*)GetSetMediaTrackInfo(CurTrack,"I_SELECTED",NULL);
		if (TkSelected==1)
		{
			//bool ShowInMixer=*(bool*)GetSetMediaTrackInfo(CurTrack,"B_SHOWINMIXER",NULL);
			//ShowInMixer=!ShowInMixer;
			int NewArm=1;
			GetSetMediaTrackInfo(CurTrack,"I_RECARM",&NewArm);
		}



	}
	UpdateTimeline();	
}

void DoTraxRecUnArmed(COMMAND_T*)
{
	MediaTrack *CurTrack;
	int i;
	for (i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		int TkSelected=*(int*)GetSetMediaTrackInfo(CurTrack,"I_SELECTED",NULL);
		if (TkSelected==1)
		{
			//bool ShowInMixer=*(bool*)GetSetMediaTrackInfo(CurTrack,"B_SHOWINMIXER",NULL);
			//ShowInMixer=!ShowInMixer;
			int NewArm=0;
			GetSetMediaTrackInfo(CurTrack,"I_RECARM",&NewArm);
		}



	}
	UpdateTimeline();	
}

void SetTrackFXOnOff(bool IsBypass)
{
	MediaTrack *CurTrack;
	int i;
	for (i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		int IsSel=*(int*)GetSetMediaTrackInfo(CurTrack,"I_SELECTED",NULL);
		if (IsSel==1)
		{
			int FxEn=1;
			if (IsBypass)
				FxEn=0;
			GetSetMediaTrackInfo(CurTrack,"I_FXEN",&FxEn);
		}
	}
	UpdateTimeline();
	Undo_OnStateChangeEx("Set track fx enabled/disabled",UNDO_STATE_TRACKCFG,-1);
}

void DoBypassFXofSelTrax(COMMAND_T*)
{
	SetTrackFXOnOff(true);
}

void DoUnBypassFXofSelTrax(COMMAND_T*)
{
	SetTrackFXOnOff(false);	
}

void DoResetTracksVolPan(COMMAND_T*)
{
	
	Undo_BeginBlock();
	MediaTrack *CurTrack;
	int i;
	for (i=0;i<GetNumTracks();i++)
	{
		//
		CurTrack=CSurf_TrackFromID(i+1,false);
		int TkSelected=*(int*)GetSetMediaTrackInfo(CurTrack,"I_SELECTED",NULL);
		if (TkSelected==1)
		{
			double NewPan=0;
			GetSetMediaTrackInfo(CurTrack,"D_PAN",&NewPan);
			double NewVol=1.0;
			GetSetMediaTrackInfo(CurTrack,"D_VOL",&NewVol);
		}
	}
	//CSurf_FlushUndo(TRUE);
	Undo_EndBlock("Reset track volume and pan",UNDO_STATE_TRACKCFG);
}

void DoSetSymmetricalpansL2R(COMMAND_T*)
{
	double newPan;
	MediaTrack* MunRaita;
	// let's get the selected tracks
	int* SelectedTracks;
	SelectedTracks=new int[1024]; // geez, CAN'T be more tracks. Ever :)
	int n=GetNumTracks();
	
	int flags; 
	
	int numselectedTracks=0;
	int i;
	for (i=0; i<n; i++)
	{
		GetTrackInfo(i,&flags);
		if (flags & 0x02)
		{ 
		  SelectedTracks[numselectedTracks]=i;
		  numselectedTracks++;
		}
	}
	int seltrackID;
	Undo_BeginBlock();
	if (numselectedTracks>1)
	{
		for (i=0; i<numselectedTracks; i++)
	
		{
			seltrackID=SelectedTracks[i];
			//seltrackID=i;
			MunRaita = CSurf_TrackFromID(seltrackID+1,TRUE); // in this case 0=Master Track	
			newPan=-1.0+((2.0/double((numselectedTracks-1)))*double(i));
			//newPan=-1.0+(1.0/RAND_MAX)*rand()*2.0;
			// gotta unselect track to be panned to prevent Reaper's ganged behaviour
			CSurf_OnSelectedChange(MunRaita, 0);
			CSurf_OnPanChange(MunRaita,newPan,FALSE);
		}
	} else
	{
		if (numselectedTracks==1)
		{
		seltrackID=SelectedTracks[0];
		//seltrackID=i;
		MunRaita = CSurf_TrackFromID(seltrackID+1,TRUE); // in this case 0=Master Track	
		newPan=0;
		//newPan=-1.0+(1.0/RAND_MAX)*rand()*2.0;
		CSurf_OnSelectedChange(MunRaita, 0);
		CSurf_OnPanChange(MunRaita,newPan,FALSE);	
		}
	}
	if (numselectedTracks==0) MessageBox(g_hwndParent,"No tracks selected!","Info",MB_OK);
	// let's restore the selected status for the tracks
	for (i=0; i<numselectedTracks; i++)
	{
		seltrackID=SelectedTracks[i];
		MunRaita = CSurf_TrackFromID(seltrackID+1,TRUE); // in this case 0=Master Track	
		CSurf_OnSelectedChange(MunRaita, 1);	
	}

	delete[] SelectedTracks;
	CSurf_FlushUndo(TRUE);
	Undo_EndBlock("Pan tracks, left to right",0);	
}


void DoSetSymmetricalpansR2L(COMMAND_T*)
{
	double newPan;
	MediaTrack* MunRaita;
	// let's get the selected tracks
	int* SelectedTracks;
	SelectedTracks=new int[1024]; // geez, CAN'T be more tracks. Ever :)
	int n=GetNumTracks();
	
	int flags; 
	
	int numselectedTracks=0;
	int i;
	for (i=0; i<n; i++)
	{
		GetTrackInfo(i,&flags);
		if (flags & 0x02)
		{ 
		  SelectedTracks[numselectedTracks]=i;
		  numselectedTracks++;
		}
	}
	int seltrackID;
	Undo_BeginBlock();
	if (numselectedTracks>1)
	{
		for (i=0; i<numselectedTracks; i++)
	
		{
			seltrackID=SelectedTracks[i];
			//seltrackID=i;
			MunRaita = CSurf_TrackFromID(seltrackID+1,TRUE); // in this case 0=Master Track	
			newPan=1.0-((2.0/double((numselectedTracks-1)))*double(i));
			//newPan=-1.0+(1.0/RAND_MAX)*rand()*2.0;
			// gotta unselect track to be panned to prevent Reaper's ganged behaviour
			CSurf_OnSelectedChange(MunRaita, 0);
			CSurf_OnPanChange(MunRaita,newPan,FALSE);
		}
	} else
	{
		if (numselectedTracks==1)
		{
		seltrackID=SelectedTracks[0];
		//seltrackID=i;
		MunRaita = CSurf_TrackFromID(seltrackID+1,TRUE); // in this case 0=Master Track	
		newPan=0;
		//newPan=-1.0+(1.0/RAND_MAX)*rand()*2.0;
		CSurf_OnSelectedChange(MunRaita, 0);
		CSurf_OnPanChange(MunRaita,newPan,FALSE);	
		}
	}
	if (numselectedTracks==0) MessageBox(g_hwndParent,"No tracks selected!","Info",MB_OK);
	// let's restore the selected status for the tracks
	for (i=0; i<numselectedTracks; i++)
	{
		seltrackID=SelectedTracks[i];
		MunRaita = CSurf_TrackFromID(seltrackID+1,TRUE); // in this case 0=Master Track	
		CSurf_OnSelectedChange(MunRaita, 1);	
	}

	delete[] SelectedTracks;
	CSurf_FlushUndo(TRUE);
	Undo_EndBlock("Pan tracks, right to left",0);
}

void DoPanTracksRandom(COMMAND_T*)
{
	
	Undo_BeginBlock();
	MediaTrack *CurTrack;
	int i;
	for (i=0;i<GetNumTracks();i++)
	{
		//
		CurTrack=CSurf_TrackFromID(i+1,false);
		int TkSelected=*(int*)GetSetMediaTrackInfo(CurTrack,"I_SELECTED",NULL);
		if (TkSelected==1)
		{
			double NewPan=-1.0+(1.0/RAND_MAX)*rand()*2.0;
			GetSetMediaTrackInfo(CurTrack,"D_PAN",&NewPan);
		}
	}
	//CSurf_FlushUndo(TRUE);
	Undo_EndBlock("Pan tracks randomly",UNDO_STATE_TRACKCFG);
	//CSurf_FlushUndo(TRUE);
	//Undo_OnStateChangeEx("Pan tracks randomly",UNDO_STATE_TRACKCFG,-1);
	//UpdateTimeline();
	/*
	double newPan;
	MediaTrack* MunRaita;
	// let's get the selected tracks
	int* SelectedTracks;
	SelectedTracks=new int[1024]; // geez, CAN'T be more tracks. Ever :)
	int n=GetNumTracks();
	
	int flags; 
	
	int numselectedTracks=0;
	 
	for (int i=0; i<n; i++)
	{
		GetTrackInfo(i,&flags);
		if (flags & 0x02)
		{ 
		  SelectedTracks[numselectedTracks]=i;
		  numselectedTracks++;
		}
	}
	int seltrackID;
	Undo_BeginBlock();
	if (numselectedTracks>1)
	{
		for (i=0; i<numselectedTracks; i++)
	
		{
			seltrackID=SelectedTracks[i];
			//seltrackID=i;
			MunRaita = CSurf_TrackFromID(seltrackID+1,TRUE); // in this case 0=Master Track	
			//newPan=1.0-((2.0/double((numselectedTracks-1)))*double(i));
			newPan=-1.0+(1.0/RAND_MAX)*rand()*2.0;
			// gotta unselect track to be panned to prevent Reaper's ganged behaviour
			CSurf_OnSelectedChange(MunRaita, 0);
			CSurf_OnPanChange(MunRaita,newPan,FALSE);
		}
	} else
	{
		if (numselectedTracks==1)
		{
		seltrackID=SelectedTracks[0];
		//seltrackID=i;
		MunRaita = CSurf_TrackFromID(seltrackID+1,TRUE); // in this case 0=Master Track	
		newPan=0;
		newPan=-1.0+(1.0/RAND_MAX)*rand()*2.0;
		CSurf_OnSelectedChange(MunRaita, 0);
		CSurf_OnPanChange(MunRaita,newPan,FALSE);	
		}
	}
	if (numselectedTracks==0) MessageBox(g_hwndParent,"No tracks selected!","Info",MB_OK);
	// let's restore the selected status for the tracks
	for (i=0; i<numselectedTracks; i++)
	{
		seltrackID=SelectedTracks[i];
		MunRaita = CSurf_TrackFromID(seltrackID+1,TRUE); // in this case 0=Master Track	
		CSurf_OnSelectedChange(MunRaita, 1);	
	}

	delete[] SelectedTracks;
	CSurf_FlushUndo(TRUE);
	Undo_EndBlock("Pan tracks randomly",0);	
	*/

}


void DoSetSymmetricalpans(COMMAND_T*)
{
	double newPan;
	MediaTrack* MunRaita;
	// let's get the selected tracks
	int* SelectedTracks;
	SelectedTracks=new int[1024]; // geez, CAN'T be more tracks. Ever :)
	int n=GetNumTracks();
	
	int flags; 
	
	int numselectedTracks=0;
	int i; 
	for (i=0; i<n; i++)
	{
		GetTrackInfo(i,&flags);
		if (flags & 0x02)
		{ 
		  SelectedTracks[numselectedTracks]=i;
		  numselectedTracks++;
		}
	}
	int seltrackID;
	Undo_BeginBlock();
	if (numselectedTracks>1)
	{
		for (i=0; i<numselectedTracks; i++)
	
		{
			seltrackID=SelectedTracks[i];
			//seltrackID=i;
			MunRaita = CSurf_TrackFromID(seltrackID+1,TRUE); // in this case 0=Master Track	
			newPan=-1.0+((2.0/double((numselectedTracks-1)))*double(i));
			//newPan=-1.0+(1.0/RAND_MAX)*rand()*2.0;
			// gotta unselect track to be panned to prevent Reaper's ganged behaviour
			CSurf_OnSelectedChange(MunRaita, 0);
			CSurf_OnPanChange(MunRaita,newPan,FALSE);
		}
	} else
	{
		if (numselectedTracks==1)
		{
		seltrackID=SelectedTracks[0];
		//seltrackID=i;
		MunRaita = CSurf_TrackFromID(seltrackID+1,TRUE); // in this case 0=Master Track	
		newPan=0;
		//newPan=-1.0+(1.0/RAND_MAX)*rand()*2.0;
		CSurf_OnSelectedChange(MunRaita, 0);
		CSurf_OnPanChange(MunRaita,newPan,FALSE);	
		}
	}
	if (numselectedTracks==0) MessageBox(g_hwndParent,"No tracks selected!","Info",MB_OK);
	// let's restore the selected status for the tracks
	for (i=0; i<numselectedTracks; i++)
	{
		seltrackID=SelectedTracks[i];
		MunRaita = CSurf_TrackFromID(seltrackID+1,TRUE); // in this case 0=Master Track	
		CSurf_OnSelectedChange(MunRaita, 1);	
	}

	delete[] SelectedTracks;
	CSurf_FlushUndo(TRUE);
	Undo_EndBlock("Set symmetric pans for tracks",0);
}

void DoSelRandomTrack(COMMAND_T*)
{
	Main_OnCommand(40297,0); // unselect all tracks
	MediaTrack *CurTrack;
	int n=GetNumTracks();
	int x=rand() % n;
	CurTrack= CSurf_TrackFromID(x+1,false);
	CSurf_OnSelectedChange(CurTrack, 1);
	//SetTrackSelected(CurTrack,true);
	//for (i=0;i<x;i++)
	//	Main_OnCommand(40285,0);
	//
	//
	//CSurf_OnTrackSelection(CurTrack);
	//SetTrackSelected(CurTrack,true);
	//UpdateTimeline();
	// 40285 go to next track
	TrackList_AdjustWindows(false);
}

typedef struct 
{
	GUID guid;
	int Heigth;
} t_trackheight_struct;

vector<t_trackheight_struct> g_vec_trackheighs;

void DoSelTraxHeightA(COMMAND_T*)
{
	
	MediaTrack *CurTrack;
	int i;
	for (i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		int issel=*(int*)GetSetMediaTrackInfo(CurTrack,"I_SELECTED",NULL);
		if (issel==1)
		{
			GetSetMediaTrackInfo(CurTrack,"I_HEIGHTOVERRIDE",&g_command_params.TrackHeightA);
		}
	}
	TrackList_AdjustWindows(false);
	UpdateTimeline();
}

void DoSelTraxHeightB(COMMAND_T*)
{
	MediaTrack *CurTrack;
	int i;
	for (i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		int issel=*(int*)GetSetMediaTrackInfo(CurTrack,"I_SELECTED",NULL);
		if (issel==1)
		{
			GetSetMediaTrackInfo(CurTrack,"I_HEIGHTOVERRIDE",&g_command_params.TrackHeightB);
		}
	}
	TrackList_AdjustWindows(false);
	UpdateTimeline();
}

void DoStoreSelTraxHeights(COMMAND_T*)
{
	g_vec_trackheighs.clear();
	MediaTrack *CurTrack;
	int i;
	for (i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		int issel=*(int*)GetSetMediaTrackInfo(CurTrack,"I_SELECTED",NULL);
		if (issel==1)
		{
			int curhei=*(int*)GetSetMediaTrackInfo(CurTrack,"I_HEIGHTOVERRIDE",NULL);
			GUID curGUID=*(GUID*)GetSetMediaTrackInfo(CurTrack,"GUID",NULL);
			t_trackheight_struct blah;
			blah.guid=curGUID;
			blah.Heigth=curhei;
			g_vec_trackheighs.push_back(blah);
		}
	}
}


void DoRecallSelectedTrackHeights(COMMAND_T*)
{
	MediaTrack *CurTrack;
	int i;
	for (i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		int issel=*(int*)GetSetMediaTrackInfo(CurTrack,"I_SELECTED",NULL);
		GUID curGUID=*(GUID*)GetSetMediaTrackInfo(CurTrack,"GUID",NULL);
		if (issel==1)
		{
			int j;
			for (j=0;j<(int)g_vec_trackheighs.size();j++)
			{
				if (memcmp(&curGUID,&g_vec_trackheighs[j].guid,sizeof(GUID))==0)
				{
					GetSetMediaTrackInfo(CurTrack,"I_HEIGHTOVERRIDE",&g_vec_trackheighs[j].Heigth);	
				}
			}
		}
	}
	TrackList_AdjustWindows(false);
}

void DoSelectTracksWithNoItems(COMMAND_T*)
{
	Main_OnCommand(40297,0); // unselect all tracks
	MediaTrack *CurTrack;
	int i;
	for (i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		int j=GetTrackNumMediaItems(CurTrack);
		if (j==0)
		{
			int sele=1;
			GetSetMediaTrackInfo(CurTrack,"I_SELECTED",&sele);
		}
	}
}

void DoSelectTracksContainingString(string AString, bool DoUnselect,bool UnselectAllFirst)
{
	if (UnselectAllFirst)
		Main_OnCommand(40297,0); // unselect all tracks
	MediaTrack *CurTrack;
	int i;
	for (i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		string compString;
		compString.assign((char*)GetSetMediaTrackInfo(CurTrack,"P_NAME",NULL));
		//std::transform(paskaKopio.begin(), paskaKopio.end(), paskaKopio.begin(), (int(*)(int)) toupper);
		int x=(int)compString.find(AString);
		if (x!=string::npos)
		{
			int blah;
			if (!DoUnselect)
				blah=1; else blah=0;
			GetSetMediaTrackInfo(CurTrack,"I_SELECTED",&blah);
		}
	}
}

void DoSelectTracksContainingBuss(COMMAND_T*)
{
	DoSelectTracksContainingString("BUSS",false,true);
}

void DoUnSelectTracksContainingBuss(COMMAND_T*)
{
	DoSelectTracksContainingString("BUSS",true,false);
}

void DoSelectLastTrackOfFolder(COMMAND_T*)
{
	MessageBox(g_hwndParent,"Not currently implemented!","Sorry!",MB_OK);
	return;
	MediaTrack *CurTrack;
	int i;
	int IDofFirstSel=-1;
	for (i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		int isSel=*(int*)GetSetMediaTrackInfo(CurTrack,"I_SELECTED",NULL);
		if (isSel==1)
		{
			IDofFirstSel=i;
			break;
		}
	}
	if (IDofFirstSel!=-1)
	{
		Main_OnCommand(40297,0); // unselect all tracks
		for (i=IDofFirstSel;i<GetNumTracks();i++)
		{
			CurTrack=CSurf_TrackFromID(i+1,false);
			int folderStatus=*(int*)GetSetMediaTrackInfo(CurTrack,"I_ISFOLDER",NULL);
			if (folderStatus==2)
			{
				int selStat=1;
				GetSetMediaTrackInfo(CurTrack,"I_SELECTED",&selStat);
			}

		}
	} else MessageBox(g_hwndParent,"No tracks selected!","Error",MB_OK);
}

void DoSetSelectedTrackNormal(COMMAND_T*)
{
	/*
	// this is for reaper<=2.53
	MediaTrack *CurTrack;
	int i;
	for (i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		int issel=*(int*)GetSetMediaTrackInfo(CurTrack,"I_SELECTED",NULL);
		if (issel==1)
		{
			int folderstatus=0;
			GetSetMediaTrackInfo(CurTrack,"I_ISFOLDER",&folderstatus);
			break;
		}
	}
	*/
	// new stuff for reaper>=2.99
	// procedure to "dismantle" folders :
	// -find first selected track
	// -check if I_FOLDERDEPTH == 1 (is first parent then), add track pointer to vector
	// -start iterating following tracks, accumulating the I_FOLDERDEPTH value to a int variable (initialized to 1)
	// -if encountering tracks with I_FOLDERDEPTH of 1 or -1, add track pointer to vector
	// -once the variable ends up at 0, we know it's the end of the current folder structure
	// track pointer vector now has tracks that need their I_FOLDERDEPTHS to be set to 0
	// ??? WORKS ???
	t_vect_of_Reaper_tracks TheTracks;
	XenGetProjectTracks(TheTracks,true);
	t_vect_of_Reaper_tracks TracksToReset;
	if (TheTracks.size()==1)
	{
		int foldepth=*(int*)GetSetMediaTrackInfo(TheTracks[0],"I_FOLDERDEPTH",0);
		if (foldepth==1)
		{
			TracksToReset.push_back(TheTracks[0]);
			int foldepAccum=1;
			int tkIDX=CSurf_TrackToID(TheTracks[0],false)+1;
			TheTracks.clear();
			while (foldepAccum>0)
			{
				MediaTrack *tk=CSurf_TrackFromID(tkIDX,false);
				foldepth=*(int*)GetSetMediaTrackInfo(tk,"I_FOLDERDEPTH",0);
				if (foldepth!=0) TracksToReset.push_back(tk);
				foldepAccum+=foldepth;
				tkIDX++;
				if (tkIDX>GetNumTracks())
				{
					MessageBox(g_hwndParent,"track count sanity check failed when dismantling folder!","ERROR",MB_OK);
					break;
				}
			}
			int i;
			for (i=0;i<(int)TracksToReset.size();i++)
			{
				int foldepth=0;
				GetSetMediaTrackInfo(TracksToReset[i],"I_FOLDERDEPTH",&foldepth);
			}
			Undo_OnStateChangeEx("Dismantle folder",UNDO_STATE_TRACKCFG,-1);
		}
		
	}
}

void DoSetSelectedTracksAsFolder(COMMAND_T*)
{
	MediaTrack *CurTrack;
	vector<MediaTrack*> VecSelTracks;
	int i;
	for (i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		int issel=*(int*)GetSetMediaTrackInfo(CurTrack,"I_SELECTED",NULL);
		if (issel==1)
		{
			VecSelTracks.push_back(CurTrack);
		}
	}
	if (VecSelTracks.size()>1)
	{
	//int folderstatus=1;
	//GetSetMediaTrackInfo(VecSelTracks[0],"I_ISFOLDER",&folderstatus);
	//folderstatus=2;
	//GetSetMediaTrackInfo(VecSelTracks[VecSelTracks.size()-1],"I_ISFOLDER",&folderstatus);
	int foldepth=1;
	GetSetMediaTrackInfo(VecSelTracks[0],"I_FOLDERDEPTH",&foldepth);
	foldepth=-1;
	GetSetMediaTrackInfo(VecSelTracks[VecSelTracks.size()-1],"I_FOLDERDEPTH",&foldepth);
	Undo_OnStateChangeEx("Create folder of selected tracks",UNDO_STATE_TRACKCFG,-1);
	} else MessageBox(g_hwndParent,"Less than 2 tracks selected!","Error",MB_OK);
}

string g_OldTrackName;
string g_NewTrackName;
bool g_MultipleSelected;
bool g_RenaTraxDialogSetAutorename;
bool g_RenaTraxDialogCancelled;
int g_RenaSelTrax=0;
int g_RenaCurTrack=0;

WDL_DLGRET RenameTraxDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	//
	switch(Message)
    {
        case WM_INITDIALOG:
			{
				SetDlgItemText(hwnd,IDC_EDIT1,g_OldTrackName.c_str());
				SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwnd, IDC_EDIT1), TRUE);
				if (g_MultipleSelected)
					Button_Enable(GetDlgItem(hwnd,IDC_CHECK1),true);
				else Button_Enable(GetDlgItem(hwnd,IDC_CHECK1),false);
				if (g_RenaTraxDialogSetAutorename) Button_SetCheck(GetDlgItem(hwnd,IDC_CHECK1),BST_CHECKED);
				else Button_SetCheck(GetDlgItem(hwnd,IDC_CHECK1),BST_UNCHECKED);
				char buf[500];
				sprintf(buf,"Rename track %d / %d",g_RenaCurTrack,g_RenaSelTrax);
				SetWindowText(hwnd,buf);
				return 0;
			}
		case WM_COMMAND:
			{
				if (LOWORD(wParam)==IDOK)
				{
					char buf[500];
					GetDlgItemText(hwnd,IDC_EDIT1,buf,499);
					g_NewTrackName.assign(buf);
					LRESULT bleh=Button_GetCheck(GetDlgItem(hwnd,IDC_CHECK1));
					g_RenaTraxDialogSetAutorename=false;
					if (bleh==BST_CHECKED) g_RenaTraxDialogSetAutorename=true;
					//if (bleh==BST
					EndDialog(hwnd,0);
					return 0;
				}
				if (LOWORD(wParam)==IDCANCEL)
				{
					g_RenaTraxDialogCancelled=true;
					EndDialog(hwnd,0);
					return 0;
				}
				return 0;
			}
	}
	return 0;
}

void DoRenameTracksDlg(COMMAND_T*)
{
	//IDD_RNMTRAX	
	static bool FirstRun=true;
	if (FirstRun)
	{
		FirstRun=false;
		g_RenaTraxDialogSetAutorename=false;
	}

	MediaTrack *CurTrack;
	vector<MediaTrack*> VecSelTracks;
	int i;
	g_MultipleSelected=false;
	for (i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		int isSel=*(int*)GetSetMediaTrackInfo(CurTrack,"I_SELECTED",NULL);
		if (isSel==1)
			VecSelTracks.push_back(CurTrack);
	}
	//g_RenaTraxDialogSetAutorename=false;
	if (VecSelTracks.size()==0) 
	{
		MessageBox(g_hwndParent,"No tracks selected!","Info",MB_OK);
		return;
	}
	if (VecSelTracks.size()>1) g_MultipleSelected=true;
	g_RenaSelTrax=(int)VecSelTracks.size();
	for (i=0;i<(int)VecSelTracks.size();i++)
	{
		g_OldTrackName.assign((char*)GetSetMediaTrackInfo(VecSelTracks[i],"P_NAME",NULL));
		g_RenaCurTrack=i+1;
		g_RenaTraxDialogCancelled=false;
		DialogBox(g_hInst, MAKEINTRESOURCE(IDD_RNMTRAX), g_hwndParent, RenameTraxDlgProc);
		if (!g_RenaTraxDialogCancelled && !g_RenaTraxDialogSetAutorename)
			GetSetMediaTrackInfo(VecSelTracks[i],"P_NAME",(char*)g_NewTrackName.c_str());
		else
			break;
	}
	if (g_RenaTraxDialogSetAutorename && !g_RenaTraxDialogCancelled)
	{
		string AutogenName;
		for (i=0;i<(int)VecSelTracks.size();i++)
		{
			//
			char argh[10];
			sprintf(argh,"-%.2d",i+1);
			AutogenName.assign(g_NewTrackName);
			AutogenName.append(argh);
			GetSetMediaTrackInfo(VecSelTracks[i],"P_NAME",(char*)AutogenName.c_str());
		}
	}
	if (!g_RenaTraxDialogCancelled && VecSelTracks.size()>0)
		Undo_OnStateChangeEx("Rename track(s)",UNDO_STATE_TRACKCFG,-1);
}

void DoSelectFirstOfSelectedTracks(COMMAND_T*)
{
	t_vect_of_Reaper_tracks TheTracks;
	XenGetProjectTracks(TheTracks,true);
	if (TheTracks.size()>0)
	{
		Main_OnCommand(40297,0); // unselect all tracks
		int isSel=1;
		GetSetMediaTrackInfo(TheTracks[0],"I_SELECTED",&isSel);
	}
	//GetSetMediaTrackInfo(TheTracks[TheTracks.size()-1],"I_SELECTED",&isSel);

}

void DoSelectLastOfSelectedTracks(COMMAND_T*)
{
	t_vect_of_Reaper_tracks TheTracks;
	XenGetProjectTracks(TheTracks,true);
	if (TheTracks.size()>0)
	{
		Main_OnCommand(40297,0); // unselect all tracks
		int isSel=1;
		//GetSetMediaTrackInfo(TheTracks[0],"I_SELECTED",&isSel);
		GetSetMediaTrackInfo(TheTracks[TheTracks.size()-1],"I_SELECTED",&isSel);
	}

}

void DoInsertNewTrackAtTop(COMMAND_T*)
{
	InsertTrackAtIndex(0,false);
	TrackList_AdjustWindows(false);
}

void DoLabelTraxDefault(COMMAND_T*)
{
	vector<MediaTrack*> TheTracks;
	XenGetProjectTracks(TheTracks,true);
	int i;
	for (i=0;i<(int)TheTracks.size();i++)
	{
		char buf[512];
		strcpy(buf,g_command_params.DefaultTrackLabel.c_str());
		GetSetMediaTrackInfo(TheTracks[i],"P_NAME",&buf);
	}
	Undo_OnStateChangeEx("Default label on tracks",UNDO_STATE_TRACKCFG,-1);
}

void DoTraxLabelPrefix(COMMAND_T*)
{
	vector<MediaTrack*> TheTracks;
	XenGetProjectTracks(TheTracks,true);
	int i;
	for (i=0;i<(int)TheTracks.size();i++)
	{
		string NewLabel;
		NewLabel.assign(g_command_params.TrackLabelPrefix);
		NewLabel.append((char*)GetSetMediaTrackInfo(TheTracks[i],"P_NAME",NULL));
		char buf[512];
		strcpy(buf,NewLabel.c_str());
		GetSetMediaTrackInfo(TheTracks[i],"P_NAME",&buf);
	}
	Undo_OnStateChangeEx("Insert prefix to track label(s)",UNDO_STATE_TRACKCFG,-1);
}

void DoTraxLabelSuffix(COMMAND_T*)
{
	vector<MediaTrack*> TheTracks;
	XenGetProjectTracks(TheTracks,true);
	int i;
	for (i=0;i<(int)TheTracks.size();i++)
	{
		string NewLabel;
		NewLabel.assign((char*)GetSetMediaTrackInfo(TheTracks[i],"P_NAME",NULL));
			
		NewLabel.append(g_command_params.TrackLabelSuffix);
		char buf[512];
		strcpy(buf,NewLabel.c_str());
		GetSetMediaTrackInfo(TheTracks[i],"P_NAME",&buf);
	}
	Undo_OnStateChangeEx("Insert suffix to track label(s)",UNDO_STATE_TRACKCFG,-1);
}

void DoMinMixSendPanelH(COMMAND_T*)
{
	vector<MediaTrack*> TheTracks;
	XenGetProjectTracks(TheTracks,true);
	int i;
	for (i=0;i<(int)TheTracks.size();i++)
	{
		float NewScale=0.0;
		GetSetMediaTrackInfo(TheTracks[i],"F_MCP_SENDRGN_SCALE",&NewScale);
	}
}

void DoMinMixSendAndFxPanelH(COMMAND_T*)
{
	vector<MediaTrack*> TheTracks;
	XenGetProjectTracks(TheTracks,true);
	int i;
	for (i=0;i<(int)TheTracks.size();i++)
	{
		float NewScale=0.0;
		GetSetMediaTrackInfo(TheTracks[i],"F_MCP_FXSEND_SCALE",&NewScale);
	}
}

void DoMaxMixFxPanHeight(COMMAND_T*)
{
	vector<MediaTrack*> TheTracks;
	XenGetProjectTracks(TheTracks,true);
	int i;
	for (i=0;i<(int)TheTracks.size();i++)
	{
		float NewScale=1.0;
		GetSetMediaTrackInfo(TheTracks[i],"F_MCP_FXSEND_SCALE",&NewScale);
		NewScale=0.0;
		GetSetMediaTrackInfo(TheTracks[i],"F_MCP_SENDRGN_SCALE",&NewScale);
		
		
	}	
}

vector<GUID> g_MediaItemsSelected;
bool g_EnvEditOn=false;

void DoToggleEnvEditMode(COMMAND_T*)
{
	if (g_EnvEditOn)
	{
		g_EnvEditOn=false;
		int i;
		int j;
		Main_OnCommand(40289,0); // unselect all items
		vector<MediaItem*> TheItems;
		XenGetProjectItems(TheItems,false,false);
		GUID compaGUID;
		for (i=0;i<(int)TheItems.size();i++)
		{
			compaGUID=*(GUID*)GetSetMediaItemInfo(TheItems[i],"GUID",0);
			for (j=0;j<(int)g_MediaItemsSelected.size();j++)
			{
				if (memcmp(&compaGUID,&g_MediaItemsSelected[j],sizeof(GUID))==0)
				{
					bool crap=true;
					GetSetMediaItemInfo(TheItems[i],"B_UISEL",&crap);
					break;
				}
			}
			
		}
		Main_OnCommand(40567,0);
		Main_OnCommand(40570,0);
	} else
	{
		g_EnvEditOn=true;
		vector<MediaItem*> TheItems;
		g_MediaItemsSelected.clear();
		XenGetProjectItems(TheItems);
		int i=0;
		for (i=0;i<(int)TheItems.size();i++)
		{
			g_MediaItemsSelected.push_back(*(GUID*)GetSetMediaItemInfo(TheItems[i],"GUID",0));
		}
		
		Main_OnCommand(40289,0);
		Main_OnCommand(40567,0);
		Main_OnCommand(40574,0);
		Main_OnCommand(40569,0);
	}
}

void DoRemoveTimeSelectionLeaveLoop(COMMAND_T*)
{
	int sz=0; int *locklooptotime = (int *)get_config_var("locklooptotime",&sz);
    int OldLockLoopToTime=*locklooptotime;
	if (sz==sizeof(int) && locklooptotime) 
	{ 
		
	int newLockLooptotime=0;
	*locklooptotime=newLockLooptotime; /* update reaper's
              copy */
	}
	double a=0.0;
	double b=0.0;
	 GetSet_LoopTimeRange(false, true, &a,&b,false); // get current loop
	 double c=0;
	 double d=0;
	 GetSet_LoopTimeRange(true, false, &c,&d,false); // set time sel to 0 and 0
	 GetSet_LoopTimeRange(true, true, &a,&b,false); // restore loop
	*locklooptotime=OldLockLoopToTime;

}

int g_CurTrackHeightIdx=0;

void DoToggleTrackHeightAB(COMMAND_T*)
{
	t_vect_of_Reaper_tracks TheTracks;
	XenGetProjectTracks(TheTracks,true);
	int i;
	int newHei=0;
		if (g_CurTrackHeightIdx==0)
		{
			g_CurTrackHeightIdx=1;
			newHei=g_command_params.TrackHeightB;
		} else
		{
			g_CurTrackHeightIdx=0;
			newHei=g_command_params.TrackHeightA;
		}
	for (i=0;i<(int)TheTracks.size();i++)
	{
		
		GetSetMediaTrackInfo(TheTracks[i],"I_HEIGHTOVERRIDE",&newHei);
	}
	TrackList_AdjustWindows(false);
	UpdateTimeline();
}

void DoFolderDepthDump(COMMAND_T*)
{
	
	vector<MediaTrack*> TheTracks;
	XenGetProjectTracks(TheTracks,true);
	int foldep=*(int*)GetSetMediaTrackInfo(TheTracks[0],"I_FOLDERDEPTH",NULL);
	if (foldep==1)
	{
		int childIdx=CSurf_TrackToID(TheTracks[0],false)+1;
		foldep=0;
		while (foldep!=-1)
		{
			MediaTrack *CurTrack=CSurf_TrackFromID(childIdx,false);
			if (CurTrack)
			{
				foldep=*(int*)GetSetMediaTrackInfo(CurTrack,"I_FOLDERDEPTH",NULL);
				
				bool visi=*(bool*)GetSetMediaTrackInfo(CurTrack,"B_SHOWINMIXER",NULL);
				if (visi) visi=false; else visi=true;
				if (foldep<=0) GetSetMediaTrackInfo(CurTrack,"B_SHOWINMIXER",&visi);
				if (foldep==-1) break;
				childIdx++;
			}
		}
	}
}

void DoPanTracksCenter(COMMAND_T*)
{
	vector<MediaTrack*> TheTracks;
	XenGetProjectTracks(TheTracks,true);
	int i;
	for (i=0;i<(int)TheTracks.size();i++)
	{
		double newPan=0.0;
		GetSetMediaTrackInfo(TheTracks[i],"D_PAN",&newPan);

	}
	Undo_OnStateChangeEx("Set track pan to center",UNDO_STATE_TRACKCFG,-1);
}

void DoPanTracksLeft(COMMAND_T*)
{
	vector<MediaTrack*> TheTracks;
	XenGetProjectTracks(TheTracks,true);
	int i;
	for (i=0;i<(int)TheTracks.size();i++)
	{
		double newPan=-1.0;
		GetSetMediaTrackInfo(TheTracks[i],"D_PAN",&newPan);

	}
	Undo_OnStateChangeEx("Set track pan to left",UNDO_STATE_TRACKCFG,-1);
}

void DoPanTracksRight(COMMAND_T*)
{
	vector<MediaTrack*> TheTracks;
	XenGetProjectTracks(TheTracks,true);
	int i;
	for (i=0;i<(int)TheTracks.size();i++)
	{
		double newPan=1.0;
		GetSetMediaTrackInfo(TheTracks[i],"D_PAN",&newPan);

	}
	Undo_OnStateChangeEx("Set track pan to right",UNDO_STATE_TRACKCFG,-1);
}

void DoSetTrackVolumeToZero(COMMAND_T*)
{
	vector<MediaTrack*> TheTracks;
	XenGetProjectTracks(TheTracks,true);
	int i;
	for (i=0;i<(int)TheTracks.size();i++)
	{
		double newVol=1.0;
		GetSetMediaTrackInfo(TheTracks[i],"D_VOL",&newVol);

	}
	Undo_OnStateChangeEx("Set track volume to 0.0dB",UNDO_STATE_TRACKCFG,-1);
}

void DoRenderReceivesAsStems(COMMAND_T*)
{
	vector<MediaTrack*> TheTracks;
	XenGetProjectTracks(TheTracks,true);
	if (TheTracks.size()==1)
	{
		int i;
		int j;
		// 40405 render postfader stereo stems
		GUID orselTrackGUID=*(GUID*)GetSetMediaTrackInfo(TheTracks[0],"GUID",0);
		MediaTrack *CurTrack;
		MediaTrack *SourceTrack=0;
		for (i=0;i<(int)TheTracks.size();i++)
		{
			
			bool blah=false;
			
			Main_OnCommand(40297,0); // unselect all tracks
			int meh=1;
			j=0;
			SourceTrack=0;
			for (j=0;j<GetNumTracks();j++)
			{
				CurTrack=CSurf_TrackFromID(j+1,false);
				if (CurTrack)
				{
					GUID compaGUID=*(GUID*)GetSetMediaTrackInfo(CurTrack,"GUID",0);
					if (memcmp(&compaGUID,&orselTrackGUID,sizeof(GUID))==0)
					{
						SourceTrack=CurTrack;
						break;
					}
				}
			}
			GetSetMediaTrackInfo(SourceTrack,"B_MUTE",&blah); // render stems action mutes original track, so counteract here
			GetSetMediaTrackInfo(SourceTrack,"I_SELECTED",&meh);
			
			vector<bool> receiveMutes;
			receiveMutes.clear();
			j=0;
			while (GetSetTrackSendInfo(SourceTrack,-1,j,"B_MUTE",0))
			{
				bool receiveMute=*(bool*)GetSetTrackSendInfo(SourceTrack,-1,j,"B_MUTE",0);
				receiveMutes.push_back(receiveMute);
				j++;
			}
			for (j=0;j<(int)receiveMutes.size();j++)
			{
				// solo receive j by muting all other except receive j
				int k;
				for (k=0;k<(int)receiveMutes.size();k++)
				{
					
					if (j!=k)
					{
						blah=true;
						GetSetTrackSendInfo(SourceTrack,-1,k,"B_MUTE",&blah);
					} else
					{
						blah=false;
						GetSetTrackSendInfo(SourceTrack,-1,k,"B_MUTE",&blah);
					}
				}
				if (!receiveMutes[j])
				{
					// we presume there's something to do only if the receive is not muted originally
					Main_OnCommand(40297,0); // unselect all tracks
					GetSetMediaTrackInfo(SourceTrack,"I_SELECTED",&meh);
					Main_OnCommand(40405,0);
					// stem render messes up selected track etc, so need to do this crap
					
					Main_OnCommand(40297,0); // unselect all tracks
					int l;
					for (l=0;l<GetNumTracks();l++)
					{
						CurTrack=CSurf_TrackFromID(l+1,false);
						if (CurTrack)
						{
							GUID compaGUID=*(GUID*)GetSetMediaTrackInfo(CurTrack,"GUID",0);
							if (memcmp(&compaGUID,&orselTrackGUID,sizeof(GUID))==0)
							{
								meh=1;
								blah=false;
								GetSetMediaTrackInfo(CurTrack,"I_SELECTED",&meh);
								GetSetMediaTrackInfo(CurTrack,"B_MUTE",&blah);
								SourceTrack=CurTrack;
								break;
							}
						}
					}
				}
			}
			// finally restore mute states of receives
			for (j=0;j<(int)receiveMutes.size();j++)
			{
				blah=receiveMutes[j];
				GetSetTrackSendInfo(SourceTrack,-1,j,"B_MUTE",&blah);
			}
			blah=true;
			GetSetMediaTrackInfo(SourceTrack,"B_MUTE",&blah);
		}
	}
	else
		MessageBox(g_hwndParent,"Only one selected track supported!","Error",MB_OK);
}

void DoSetRenderSpeedToRealtime2(COMMAND_T*)
{
	int sz=0; int *renderspeedmode = (int *)get_config_var("workrender",&sz);
    if (sz==sizeof(int) && renderspeedmode) 
	{ 
		bitset<32> blah((ULONG)*renderspeedmode);	
		blah.set(3);
		*renderspeedmode=blah.to_ulong();
	}
	else
		MessageBox(g_hwndParent,"Error getting configuration variable from REAPER","Error",MB_OK);
}

void DoSetRenderSpeedToNonLim(COMMAND_T*)
{
	int sz=0; int *renderspeedmode = (int *)get_config_var("workrender",&sz);
    if (sz==sizeof(int) && renderspeedmode) 
	{ 
		bitset<32> blah((ULONG)*renderspeedmode);	
		blah.reset(3);
		*renderspeedmode=blah.to_ulong();
	}
	else
		MessageBox(g_hwndParent,"Error getting configuration variable from REAPER","Error",MB_OK);
}

int g_renderspeed=-1;

void DoStoreRenderSpeed(COMMAND_T*)
{
	int sz=0; int *renderspeedmode = (int *)get_config_var("workrender",&sz);
    if (sz==sizeof(int) && renderspeedmode) 
	{ 
		g_renderspeed=*renderspeedmode;
		
		
	} else MessageBox(g_hwndParent,"Error getting configuration variable from REAPER","Error",MB_OK);
}

void DoRecallRenderSpeed(COMMAND_T*)
{
	int sz=0; int *renderspeedmode = (int *)get_config_var("workrender",&sz);
    if (sz==sizeof(int) && renderspeedmode) 
	{ 
		if (g_renderspeed>=0)
			*renderspeedmode=g_renderspeed;
		else MessageBox(g_hwndParent,"Render speed has not been stored","Info",MB_OK);
	} else MessageBox(g_hwndParent,"Error getting configuration variable from REAPER","Error",MB_OK);
} 

typedef struct
{
	GUID trackGUID;
	int solostate;
} t_track_solostate;

MediaTrack* g_ReferenceTrack=0;
bool g_ReferenceTrackSolo=false;
double g_RefMasterVolume=1.0;
GUID g_RefTrackGUID=GUID_NULL;
vector<t_track_solostate> g_RefTrackSolostates;

MediaTrack* GetTrackWithGUID(GUID theguid)
{
	if (theguid == GUID_NULL)
		return NULL;
	for (int i=0;i<GetNumTracks();i++)
	{
		MediaTrack* tk=CSurf_TrackFromID(i+1,false);
		if (tk)
		{
			if (memcmp((GUID*)GetSetMediaTrackInfo(tk,"GUID",0), &theguid, sizeof(GUID)) == 0)
			{
				return tk;
			}
		}
	}
	return NULL;
}

void DoSetSelTrackAsRefTrack(COMMAND_T*)
{
	t_vect_of_Reaper_tracks thetracks;
	XenGetProjectTracks(thetracks,true);
	if (thetracks.size()>0)
	{
		g_RefTrackGUID=*(GUID*)GetSetMediaTrackInfo(thetracks[0],"GUID",0);
		bool bmute=true;
		GetSetMediaTrackInfo(thetracks[0],"B_MUTE",&bmute);
		MediaTrack *mtk=CSurf_TrackFromID(0,false);
		if (mtk) g_RefMasterVolume=*(double*)GetSetMediaTrackInfo(mtk,"D_VOL",0);
	}
}

void DoStoreProjectSoloStates()
{
	int i;
	g_RefTrackSolostates.clear();
	for (i=0;i<GetNumTracks();i++)
	{
		MediaTrack *tk=CSurf_TrackFromID(i+1,false);
		if (tk)
		{
			t_track_solostate glah;
			glah.solostate=*(int*)GetSetMediaTrackInfo(tk,"I_SOLO",0);
			glah.trackGUID=*(GUID*)GetSetMediaTrackInfo(tk,"GUID",0);
			g_RefTrackSolostates.push_back(glah);
		}
	}
}

void RecallTrackSoloStates()
{
	int i;
	for (i=0;i<(int)g_RefTrackSolostates.size();i++)
	{
		MediaTrack *tk=GetTrackWithGUID(g_RefTrackSolostates[i].trackGUID);
		if (tk)
			GetSetMediaTrackInfo(tk,"I_SOLO",&g_RefTrackSolostates[i].solostate);
	}
}

void SetAllTrackSolos(int solomode)
{
	int i;

	for (i=0;i<GetNumTracks();i++)
	{
		MediaTrack *tk=CSurf_TrackFromID(i+1,false);
		if (tk)
			GetSetMediaTrackInfo(tk,"I_SOLO",&solomode);	
	}
}

void DoToggleReferenceTrack(COMMAND_T*)
{
	int isolo;
	int ifxen;
	bool bmute;
	MediaTrack* reftk=0;
	if (!g_ReferenceTrackSolo)
	{
		g_ReferenceTrackSolo=true;
		reftk=GetTrackWithGUID(g_RefTrackGUID);
		if (reftk)
		{
		DoStoreProjectSoloStates();
		SetAllTrackSolos(0);
		isolo=1;
		bmute=false;
		GetSetMediaTrackInfo(reftk,"B_MUTE",&bmute);
		GetSetMediaTrackInfo(reftk,"I_SOLO",&isolo);
		MediaTrack* mtk=CSurf_TrackFromID(0,false);
		ifxen=0;
		if (mtk)
		{
			//GetSetMediaTrackInfo(mtk,"I_FXEN",&ifxen);
			g_RefMasterVolume=*(double*)GetSetMediaTrackInfo(mtk,"D_VOL",0);
			double mvol=1.0;
			GetSetMediaTrackInfo(mtk,"D_VOL",&mvol);
		}
		} else MessageBox(g_hwndParent,"Reference track doesn't seem to exist in this project.\nMaybe it's in another project tab?","Error",MB_OK);
	} else
	{
		reftk=GetTrackWithGUID(g_RefTrackGUID);
		if (reftk)
		{
		RecallTrackSoloStates();
		g_ReferenceTrackSolo=false;
		isolo=0;
		bmute=true;
		GetSetMediaTrackInfo(reftk,"B_MUTE",&bmute);
		GetSetMediaTrackInfo(reftk,"I_SOLO",&isolo);
		MediaTrack* mtk=CSurf_TrackFromID(0,false);
		ifxen=1;
		if (mtk)
		{
			//GetSetMediaTrackInfo(mtk,"I_FXEN",&ifxen);	
			GetSetMediaTrackInfo(mtk,"D_VOL",&g_RefMasterVolume);
		}
		} else MessageBox(g_hwndParent,"Reference track doesn't seem to exist in this project.\nMaybe it's in another project tab?","Error",MB_OK);
	}
}

bool Ref_track_PEL(const char *line, ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg) // returns BOOL if line (and optionally subsequent lines) processed
{
  bool commentign=false;
  LineParser lp(commentign);
  if (lp.parse(line) || lp.getnumtokens()<1) return false;
  if (strcmp(lp.gettoken_str(0),"<XENAKIOSCOMMANDS")) return false; // only look for <XENAKIOSCOMMANDS lines
  
  int child_count=1;
  for (;;)
  {
    char linebuf[4096];
    if (ctx->GetLine(linebuf,sizeof(linebuf))) break;
    if (lp.parse(linebuf)||lp.getnumtokens()<=0) continue;

    if (child_count == 1)
    {
      if (lp.gettoken_str(0)[0] == '>') break;

      if (!strcmp(lp.gettoken_str(0),"REFTRACK"))
      {
        //lstrcpyn(g_last_insertfilename,lp.gettoken_str(1),sizeof(g_last_insertfilename)); // load config
		  int trackindex=lp.gettoken_int(2);
		  int reftkmode=lp.gettoken_int(1);
		  g_RefMasterVolume=lp.gettoken_float(3);
		  if (reftkmode==0) g_ReferenceTrackSolo=false; else g_ReferenceTrackSolo=true;
		  MediaTrack *tk=CSurf_TrackFromID(trackindex,false);
		  if (tk) g_RefTrackGUID=*(GUID*)GetSetMediaTrackInfo(tk,"GUID",0);
		  int i;
		  g_RefTrackSolostates.clear();
		  char solostring[4096];
		  strcpy(solostring,lp.gettoken_str(4));
		  for (i=0;i<(int)strlen(solostring);i++)
		  {
			  MediaTrack *tk=CSurf_TrackFromID(i+1,false);
			  if (tk)
			  {
				t_track_solostate glah;
				glah.trackGUID=*(GUID*)GetSetMediaTrackInfo(tk,"GUID",0);
				if (solostring[i]=='0') glah.solostate=0;
				if (solostring[i]=='1') glah.solostate=1;
				if (solostring[i]=='2') glah.solostate=2;
				if (solostring[i]=='x') glah.solostate=0;
				g_RefTrackSolostates.push_back(glah);
			  }

		  }
		  //MediaTrack *mtk=CSurf_TrackFromID(0,false);
		  //if (mtk) GetSetMediaTrackInfo
      }
      else if (lp.gettoken_str(0)[0] == '<') child_count++; // ignore block
      else  // ignore unknown line
      {
      }
    }
    else if (lp.gettoken_str(0)[0] == '<') child_count++; // track subblocks
    else if (lp.gettoken_str(0)[0] == '>') child_count--;

  }
  return true;
}

void Ref_track_SaveExtensionConfig(ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
  if (!isUndo)
  {
	int mode;
	if (g_ReferenceTrackSolo) mode=1; else mode=0;
	MediaTrack *tk=GetTrackWithGUID(g_RefTrackGUID);
	if (tk)
	{
		ctx->AddLine("<XENAKIOSCOMMANDS");
		int trackindex=CSurf_TrackToID(tk ,false);
		char buf[4096];
		int i;
		int ntk=GetNumTracks();
		if (ntk>4095) ntk=4095;
		

		for (i=0;i<ntk;i++)
		{
			MediaTrack *tk=CSurf_TrackFromID(i+1,false);
			if (tk)
			{
				int solostate=*(int*)GetSetMediaTrackInfo(tk,"I_SOLO",0);
				if (solostate==0) buf[i]='0';
				if (solostate==1) buf[i]='1';
				if (solostate==2) buf[i]='2';
				if (solostate<0 || solostate>2) buf[i]='x';
			}
		}
		buf[ntk]=0;
		ctx->AddLine("REFTRACK %d %d %f \"%s\"",mode,trackindex,g_RefMasterVolume,buf);
		ctx->AddLine(">");
	}
  }
}

void Ref_track_BeginLoadProjectState(bool isUndo, struct project_config_extension_t *reg)
{
  // set defaults here (since we know that ProcessExtensionLine could come up)
}


project_config_extension_t xen_reftrack_pcreg={
	Ref_track_PEL,
	Ref_track_SaveExtensionConfig,
	Ref_track_BeginLoadProjectState, 
	NULL,
};

void NudgeTrackVolumeDB(int tkIndex,double decibel)
{
	MediaTrack* pTk=CSurf_TrackFromID(tkIndex,false);
	if (pTk)
	{
		double curgain=*(double*)GetSetMediaTrackInfo(pTk,"D_VOL",0);
		double curvol=VAL2DB(curgain);
		curvol+=decibel;
		double newgain= exp((curvol)*0.11512925464970228420089957273422);
		GetSetMediaTrackInfo(pTk,"D_VOL",&newgain);

	}
}

void DoNudgeMasterVol1dbUp(COMMAND_T*)
{
	NudgeTrackVolumeDB(0,1.0);
	Undo_OnStateChangeEx("Nudge Master volume up",UNDO_STATE_TRACKCFG,-1);
}

void DoNudgeMasterVol1dbDown(COMMAND_T*)
{
	NudgeTrackVolumeDB(0,-1.0);
	Undo_OnStateChangeEx("Nudge Master volume down",UNDO_STATE_TRACKCFG,-1);
}

void DoNudgeSelTrackVolumeUp(COMMAND_T*)
{
	t_vect_of_Reaper_tracks thetracks;
	XenGetProjectTracks(thetracks,true);
	for (int i=0;i<(int)thetracks.size();i++)
	{
		int index=CSurf_TrackToID(thetracks[i],false);
		NudgeTrackVolumeDB(index,g_command_params.TrackVolumeNudge);
	}
	Undo_OnStateChangeEx("Nudge selected track(s) volume(s) up",UNDO_STATE_TRACKCFG,-1);
}

void DoNudgeSelTrackVolumeDown(COMMAND_T*)
{
	t_vect_of_Reaper_tracks thetracks;
	XenGetProjectTracks(thetracks,true);
	for (int i=0;i<(int)thetracks.size();i++)
	{
		int index=CSurf_TrackToID(thetracks[i],false);
		NudgeTrackVolumeDB(index,-g_command_params.TrackVolumeNudge);
	}
	Undo_OnStateChangeEx("Nudge selected track(s) volume(s) down",UNDO_STATE_TRACKCFG,-1);
}

void DoSetMasterToZeroDb(COMMAND_T*)
{
	MediaTrack* ptk=CSurf_TrackFromID(0,false);
	if (ptk)
	{
		double newgain=1.0;
		GetSetMediaTrackInfo(ptk,"D_VOL",&newgain);
		Undo_OnStateChangeEx("Reset Master volume",UNDO_STATE_TRACKCFG,-1);
	}
}

void SetAllMasterSendsMutes(bool muted)
{
	MediaTrack* Ptk=CSurf_TrackFromID(0,false);
	if (Ptk)
	{
		int indx=0;
		while (GetSetTrackSendInfo(Ptk,1,indx,"B_MUTE",0))
		{
			bool smute=muted;
			GetSetTrackSendInfo(Ptk,1,indx,"B_MUTE",&smute);
			indx++;
		}
	}
	Undo_OnStateChangeEx("Set all Master track sends muted/unmuted",UNDO_STATE_TRACKCFG,-1);
}

void ToggleMasterSendMute(int indx,int mode) // mode 0==toggle, 1==set mute, 2==set unmute
{
	MediaTrack* Ptk=CSurf_TrackFromID(0,false);
	if (Ptk && GetSetTrackSendInfo(Ptk,1,indx,"B_MUTE",0))
	{
		if (mode==0)
		{
			bool smute=*(bool*)GetSetTrackSendInfo(Ptk,1,indx,"B_MUTE",0);
			smute=!smute;
			GetSetTrackSendInfo(Ptk,1,indx,"B_MUTE",&smute);
			Undo_OnStateChangeEx("Toggle Master track send mute",UNDO_STATE_TRACKCFG,-1);
		}
		if (mode>0)
		{
			bool smute=true;
			if (mode==2) smute=false;
			GetSetTrackSendInfo(Ptk,1,indx,"B_MUTE",&smute);
			Undo_OnStateChangeEx("Set/unset Master track send mute",UNDO_STATE_TRACKCFG,-1);
		}
	}
}

void DoToggleMasterSendMute(COMMAND_T* t)		{ ToggleMasterSendMute(t->user,0); }
void DoSetMasterSendMute(COMMAND_T* t)			{ ToggleMasterSendMute(t->user,1); }
void DoUnSetMasterSendMute(COMMAND_T* t)		{ ToggleMasterSendMute(t->user,2); }
void DoSetAllMastersSendsMuted(COMMAND_T*)		{ SetAllMasterSendsMutes(true);  }
void DoSetAllMastersSendsUnMuted(COMMAND_T*)	{ SetAllMasterSendsMutes(false); }
