/******************************************************************************
/ MixerActions.cpp
/
/ Copyright (c) 2010 Tim Payne (SWS), original code by Xenakios
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
#include "../nofish/nofish.h" // NF_IsObeyTrackHeightLockEnabled

#include <WDL/localize/localize.h>

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

void DoSelectTrack(int tkOffset, bool keepCurrent)
{
	vector<int> oldSelectedTracks;
	WDL_TypedBuf<int> selTracks;
	selTracks.Resize(GetNumTracks());
	for (int i = 0; i < GetNumTracks(); i++)
	{
		selTracks.Get()[i] = 0;
		if (*(int*)GetSetMediaTrackInfo(CSurf_TrackFromID(i+1, false), "I_SELECTED", NULL))
			oldSelectedTracks.push_back(i);
	}
	
	int iNewSel = 0;
	for (int i = 0; i < (int)oldSelectedTracks.size(); i++)
	{
		int newIndex= oldSelectedTracks[i] + tkOffset;
		if (newIndex >= 0 && newIndex < GetNumTracks())
		{
			iNewSel++;
			selTracks.Get()[newIndex] = 1;
		}
	}
	
	if (keepCurrent)
		for (int i = 0; i < (int)oldSelectedTracks.size(); i++)
			selTracks.Get()[oldSelectedTracks[i]] = 1;
	else if (iNewSel == 0) // Don't remove everything!
		return;		
	
	for (int i = 0; i < GetNumTracks(); i++)
	{	// Only change the sel at the end
		MediaTrack* tr = CSurf_TrackFromID(i+1, false);
		if (selTracks.Get()[i] != *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			GetSetMediaTrackInfo(tr, "I_SELECTED", &(selTracks.Get()[i]));
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

void DoTraxPanLaw(COMMAND_T* ct)
{
	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i+1, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			double dLawGain = -1.0; // < 0 == default
			if (ct->user != 666) // Magic # for "default"
			{
				double dLawDb = (double)ct->user / 10.0;
				dLawGain = DB2VAL(dLawDb);
			}
			GetSetMediaTrackInfo(tr, "D_PANLAW", &dLawGain);
		}
	}
	Undo_OnStateChange(SWS_CMD_SHORTNAME(ct));
}

void DoTraxRecArmed(COMMAND_T* ct)
{
	int iRecArm = (int)ct->user;
	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* CurTrack = CSurf_TrackFromID(i+1, false);
		if (*(int*)GetSetMediaTrackInfo(CurTrack,"I_SELECTED",NULL))
			GetSetMediaTrackInfo(CurTrack, "I_RECARM", &iRecArm);
	}
	UpdateTimeline();	
}

//JFB dup with SWS 
void DoBypassFXofSelTrax(COMMAND_T* ct)
{
	int iBypass = (int)ct->user;
	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* CurTrack = CSurf_TrackFromID(i+1, false);
		if (*(int*)GetSetMediaTrackInfo(CurTrack,"I_SELECTED",NULL))
			GetSetMediaTrackInfo(CurTrack, "I_FXEN", &iBypass);
	}
	UpdateTimeline();
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),UNDO_STATE_TRACKCFG,-1);
}

void DoResetTracksVolPan(COMMAND_T* ct)
{
	Undo_BeginBlock();
	for (int i=0;i<GetNumTracks();i++)
	{
		MediaTrack* CurTrack=CSurf_TrackFromID(i+1,false);
		if (*(int*)GetSetMediaTrackInfo(CurTrack,"I_SELECTED",NULL))
		{
			double NewPan=0;
			GetSetMediaTrackInfo(CurTrack,"D_PAN",&NewPan);
			double NewVol=1.0;
			GetSetMediaTrackInfo(CurTrack,"D_VOL",&NewVol);
		}
	}
	Undo_EndBlock(SWS_CMD_SHORTNAME(ct),UNDO_STATE_TRACKCFG);
}

void DoSetSymmetricalpansL2R(COMMAND_T* ct)
{
	int numselectedTracks = NumSelTracks();
	if (numselectedTracks > 1)
	{
		Undo_BeginBlock();
		int iSel = 0;
		for (int i=0;i<GetNumTracks();i++)
		{
			MediaTrack* CurTrack=CSurf_TrackFromID(i+1,false);
			if (*(int*)GetSetMediaTrackInfo(CurTrack,"I_SELECTED",NULL))
			{
				double newpan = -1.0+((2.0/double((numselectedTracks-1)))*double(iSel++));
				GetSetMediaTrackInfo(CurTrack, "D_PAN", &newpan);
			}
		}
		Undo_EndBlock(SWS_CMD_SHORTNAME(ct),0);	
	}
}


void DoSetSymmetricalpansR2L(COMMAND_T* ct)
{
	int numselectedTracks = NumSelTracks();
	if (numselectedTracks > 1)
	{
		Undo_BeginBlock();
		int iSel = 0;
		for (int i=0;i<GetNumTracks();i++)
		{
			MediaTrack* CurTrack=CSurf_TrackFromID(i+1,false);
			if (*(int*)GetSetMediaTrackInfo(CurTrack,"I_SELECTED",NULL))
			{
				double newpan = 1.0-((2.0/double((numselectedTracks-1)))*double(iSel++));
				GetSetMediaTrackInfo(CurTrack, "D_PAN", &newpan);
			}
		}
		Undo_EndBlock(SWS_CMD_SHORTNAME(ct),0);	
	}
}

void DoPanTracksRandom(COMMAND_T* ct)
{
	Undo_BeginBlock();
	for (int i=0;i<GetNumTracks();i++)
	{
		MediaTrack* CurTrack=CSurf_TrackFromID(i+1,false);
		if (*(int*)GetSetMediaTrackInfo(CurTrack,"I_SELECTED",NULL))
		{
			double NewPan=-1.0+(1.0/RAND_MAX)*rand()*2.0;
			GetSetMediaTrackInfo(CurTrack,"D_PAN",&NewPan);
		}
	}
	Undo_EndBlock(SWS_CMD_SHORTNAME(ct),UNDO_STATE_TRACKCFG);
}

void DoSelRandomTrack(COMMAND_T*)
{
	Main_OnCommand(40297,0); // unselect all tracks
	int x = rand() % GetNumTracks();
	MediaTrack* tr = CSurf_TrackFromID(x+1, false);
	GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i1);
}

typedef struct 
{
	GUID guid;
	int Heigth;
} t_trackheight_struct;

vector<t_trackheight_struct> g_vec_trackheighs;

void DoSelTraxHeight(COMMAND_T* ct)
{
	const bool obeyHeightLock = NF_IsObeyTrackHeightLockEnabled();

	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* CurTrack = CSurf_TrackFromID(i+1, false);

		if (GetMediaTrackInfo_Value(CurTrack, "I_SELECTED") && (!obeyHeightLock || !GetMediaTrackInfo_Value(CurTrack, "B_HEIGHTLOCK")))
			SetMediaTrackInfo_Value(CurTrack, "I_HEIGHTOVERRIDE", g_command_params.TrackHeight[ct->user]);
	}
	TrackList_AdjustWindows(false);
	UpdateTimeline();
}

void DoStoreSelTraxHeights(COMMAND_T*)
{
	g_vec_trackheighs.clear();
	for (int i=0;i<GetNumTracks();i++)
	{
		MediaTrack* CurTrack=CSurf_TrackFromID(i+1,false);
		if (*(int*)GetSetMediaTrackInfo(CurTrack,"I_SELECTED",NULL))
		{
			t_trackheight_struct th;
			th.guid=*(GUID*)GetSetMediaTrackInfo(CurTrack,"GUID",NULL);
			th.Heigth=*(int*)GetSetMediaTrackInfo(CurTrack,"I_HEIGHTOVERRIDE",NULL);
			g_vec_trackheighs.push_back(th);
		}
	}
}


void DoRecallSelectedTrackHeights(COMMAND_T*)
{
	const bool obeyHeightLock = NF_IsObeyTrackHeightLockEnabled();

	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* CurTrack = CSurf_TrackFromID(i+1, false);
		if (!GetMediaTrackInfo_Value(CurTrack, "I_SELECTED") || (obeyHeightLock && GetMediaTrackInfo_Value(CurTrack, "B_HEIGHTLOCK")))
			continue;

		GUID curGUID = *(GUID*)GetSetMediaTrackInfo(CurTrack, "GUID", NULL);
		for (int j = 0; j < (int)g_vec_trackheighs.size(); j++)
			if (GuidsEqual(&curGUID, &g_vec_trackheighs[j].guid))
				GetSetMediaTrackInfo(CurTrack, "I_HEIGHTOVERRIDE", &g_vec_trackheighs[j].Heigth);
	}
	TrackList_AdjustWindows(false);
	UpdateTimeline();
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

void DoSetSelectedTrackNormal(COMMAND_T* ct)
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
					break;
			}
			int i;
			for (i=0;i<(int)TracksToReset.size();i++)
			{
				int foldepth=0;
				GetSetMediaTrackInfo(TracksToReset[i],"I_FOLDERDEPTH",&foldepth);
			}
			Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),UNDO_STATE_TRACKCFG,-1);
		}
		
	}
}

void DoSetSelectedTracksAsFolder(COMMAND_T* ct)
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
		Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),UNDO_STATE_TRACKCFG,-1);
	}
	else
		MessageBox(g_hwndParent, __LOCALIZE("Less than 2 selected tracks!","sws_mbox"), __LOCALIZE("Xenakios - Error","sws_mbox"), MB_OK);
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
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, Message, wParam, lParam))
		return r;

	switch(Message)
    {
        case WM_INITDIALOG:
		{
			SetDlgItemText(hwnd,IDC_EDIT1,g_OldTrackName.c_str());
			SetFocus(GetDlgItem(hwnd, IDC_EDIT1));
			SendMessage(GetDlgItem(hwnd, IDC_EDIT1), EM_SETSEL, 0, -1);
			EnableWindow(GetDlgItem(hwnd, IDC_CHECK1), g_MultipleSelected);
			CheckDlgButton(hwnd, IDC_CHECK1, g_RenaTraxDialogSetAutorename ? BST_CHECKED : BST_UNCHECKED);
			char buf[500];
			snprintf(buf,sizeof(buf),__LOCALIZE_VERFMT("Rename track %d / %d","sws_DLG_139"),g_RenaCurTrack,g_RenaSelTrax);
			SetWindowText(hwnd,buf);
			break;
		}
		case WM_COMMAND:
		{
			if (LOWORD(wParam)==IDOK)
			{
				char buf[500];
				GetDlgItemText(hwnd, IDC_EDIT1, buf, 499);
				g_NewTrackName.assign(buf);
				g_RenaTraxDialogSetAutorename = IsDlgButtonChecked(hwnd, IDC_CHECK1) == BST_CHECKED;
				EndDialog(hwnd,0);
				break;
			}
			else if (LOWORD(wParam)==IDCANCEL)
			{
				g_RenaTraxDialogCancelled = true;
				EndDialog(hwnd,0);
				break;
			}
		}
	}
	return 0;
}

void DoRenameTracksDlg(COMMAND_T* ct)
{
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
	if (VecSelTracks.size()==0) 
	{
		MessageBox(g_hwndParent, __LOCALIZE("No selected track!","sws_mbox"),__LOCALIZE("Xenakios - Error","sws_mbox"), MB_OK);
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
			char argh[13];
			sprintf(argh,"-%.2d",i+1);
			AutogenName.assign(g_NewTrackName);
			AutogenName.append(argh);
			GetSetMediaTrackInfo(VecSelTracks[i],"P_NAME",(char*)AutogenName.c_str());
		}
	}
	if (!g_RenaTraxDialogCancelled && VecSelTracks.size()>0)
		Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),UNDO_STATE_TRACKCFG,-1);
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
}

void DoSelectLastOfSelectedTracks(COMMAND_T*)
{
	t_vect_of_Reaper_tracks TheTracks;
	XenGetProjectTracks(TheTracks,true);
	if (TheTracks.size()>0)
	{
		Main_OnCommand(40297,0); // unselect all tracks
		int isSel=1;
		GetSetMediaTrackInfo(TheTracks[TheTracks.size()-1],"I_SELECTED",&isSel);
	}
}

void DoInsertNewTrackAtTop(COMMAND_T*)
{
	InsertTrackAtIndex(0,false);
	TrackList_AdjustWindows(false);
}

void DoLabelTraxDefault(COMMAND_T* ct)
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
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),UNDO_STATE_TRACKCFG,-1);
}

void DoTraxLabelPrefix(COMMAND_T* ct)
{
	if (!g_command_params.TrackLabelPrefix.size())
	{
		MessageBox(g_hwndParent, __LOCALIZE("Please enter a prefix in the command parameters window first.","sws_mbox"), __LOCALIZE("Xenakios - Error","sws_mbox"), MB_OK);
		return;
	}
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
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),UNDO_STATE_TRACKCFG,-1);
}

void DoTraxLabelSuffix(COMMAND_T* ct)
{
	if (!g_command_params.TrackLabelSuffix.size())
	{
		MessageBox(g_hwndParent, __LOCALIZE("Please enter a suffix in the command parameters window first.","sws_mbox"), __LOCALIZE("Xenakios - Error","sws_mbox"), MB_OK);
		return;
	}
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
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),UNDO_STATE_TRACKCFG,-1);
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

void DoRemoveTimeSelectionLeaveLoop(COMMAND_T*)
{
	ConfigVarOverride<int> locklooptotime("locklooptotime", 0);

	double a=0.0;
	double b=0.0;
	GetSet_LoopTimeRange(false, true, &a,&b,false); // get current loop
	double c=0;
	double d=0;
	GetSet_LoopTimeRange(true, false, &c,&d,false); // set time sel to 0 and 0
	GetSet_LoopTimeRange(true, true, &a,&b,false); // restore loop
}

int g_CurTrackHeightIdx=0;

void DoToggleTrackHeightAB(COMMAND_T*)
{
	t_vect_of_Reaper_tracks TheTracks;
	XenGetProjectTracks(TheTracks, true);

	g_CurTrackHeightIdx = !g_CurTrackHeightIdx;
	const int newHeight = g_command_params.TrackHeight[g_CurTrackHeightIdx];
	const bool obeyHeightLock = NF_IsObeyTrackHeightLockEnabled();

	for (size_t i = 0; i < TheTracks.size(); i++)
	{
		if (!obeyHeightLock || !GetMediaTrackInfo_Value(TheTracks[i], "B_HEIGHTLOCK"))
			SetMediaTrackInfo_Value(TheTracks[i], "I_HEIGHTOVERRIDE", newHeight);
	}

	TrackList_AdjustWindows(false);
	UpdateTimeline();
}

void DoPanTracksCenter(COMMAND_T* ct)
{
	vector<MediaTrack*> TheTracks;
	XenGetProjectTracks(TheTracks,true);
	int i;
	for (i=0;i<(int)TheTracks.size();i++)
	{
		double newPan=0.0;
		GetSetMediaTrackInfo(TheTracks[i],"D_PAN",&newPan);
	}
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),UNDO_STATE_TRACKCFG,-1);
}

void DoPanTracksLeft(COMMAND_T* ct)
{
	vector<MediaTrack*> TheTracks;
	XenGetProjectTracks(TheTracks,true);
	int i;
	for (i=0;i<(int)TheTracks.size();i++)
	{
		double newPan=-1.0;
		GetSetMediaTrackInfo(TheTracks[i],"D_PAN",&newPan);
	}
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),UNDO_STATE_TRACKCFG,-1);
}

void DoPanTracksRight(COMMAND_T* ct)
{
	vector<MediaTrack*> TheTracks;
	XenGetProjectTracks(TheTracks,true);
	int i;
	for (i=0;i<(int)TheTracks.size();i++)
	{
		double newPan=1.0;
		GetSetMediaTrackInfo(TheTracks[i],"D_PAN",&newPan);
	}
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),UNDO_STATE_TRACKCFG,-1);
}

void DoSetTrackVolumeToZero(COMMAND_T* ct)
{
	vector<MediaTrack*> TheTracks;
	XenGetProjectTracks(TheTracks,true);
	int i;
	for (i=0;i<(int)TheTracks.size();i++)
	{
		double newVol=1.0;
		GetSetMediaTrackInfo(TheTracks[i],"D_VOL",&newVol);
	}
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),UNDO_STATE_TRACKCFG,-1);
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
				if (CurTrack && TrackMatchesGuid(CurTrack,&orselTrackGUID))
				{
					SourceTrack=CurTrack;
					break;
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
					}
					else
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
							if (GuidsEqual(&compaGUID,&orselTrackGUID))
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
		MessageBox(g_hwndParent, __LOCALIZE("Only one selected track is supported!","sws_mbox"), __LOCALIZE("Xenakios - Error","sws_mbox"), MB_OK);
}

void DoSetRenderSpeedToRealtime2(COMMAND_T*)
{
	if (ConfigVar<int> renderspeedmode = "workrender")
	{ 
		bitset<32> blah(*renderspeedmode);
		blah.set(3);
		*renderspeedmode=blah.to_ulong();
	}
	else
		MessageBox(g_hwndParent, __LOCALIZE("Error getting configuration variable from REAPER","sws_mbox"), __LOCALIZE("Xenakios - Error","sws_mbox"), MB_OK);
}

void DoSetRenderSpeedToNonLim(COMMAND_T*)
{
	if (ConfigVar<int> renderspeedmode = "workrender")
	{ 
		bitset<32> blah(*renderspeedmode);
		blah.reset(3);
		*renderspeedmode=blah.to_ulong();
	}
	else
		MessageBox(g_hwndParent, __LOCALIZE("Error getting configuration variable from REAPER","sws_mbox"), __LOCALIZE("Xenakios - Error","sws_mbox"), MB_OK);
}

int g_renderspeed=-1;

void DoStoreRenderSpeed(COMMAND_T*)
{
	if (const ConfigVar<int> renderspeedmode = "workrender")
	{ 
		g_renderspeed=*renderspeedmode;
	}
	else
		MessageBox(g_hwndParent, __LOCALIZE("Error getting configuration variable from REAPER","sws_mbox"), __LOCALIZE("Xenakios - Error","sws_mbox"), MB_OK);
}

void DoRecallRenderSpeed(COMMAND_T*)
{
	if (ConfigVar<int> renderspeedmode = "workrender")
	{ 
		if (g_renderspeed>=0)
			*renderspeedmode=g_renderspeed;
		else
			MessageBox(g_hwndParent, __LOCALIZE("Render speed has not been stored","sws_mbox"),__LOCALIZE("Xenakios - Info","sws_mbox"), MB_OK);
	} 
	else
		MessageBox(g_hwndParent, __LOCALIZE("Error getting configuration variable from REAPER","sws_mbox"), __LOCALIZE("Xenakios - Error","sws_mbox"), MB_OK);
} 

typedef struct
{
	GUID trackGUID;
	int solostate;
} t_track_solostate;

MediaTrack* g_ReferenceTrack=0;
bool g_ReferenceTrackSolo=false;
double g_RefMasterVolume=1.0;
// Set to invalid GUID (not zero because GuidToTrack would return the master track instead of NULL)
GUID g_RefTrackGUID { ~0u };
vector<t_track_solostate> g_RefTrackSolostates;

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
		MediaTrack *tk=GuidToTrack(&g_RefTrackSolostates[i].trackGUID);
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

int IsRefTrack(COMMAND_T*) { return g_ReferenceTrackSolo; }

void DoToggleReferenceTrack(COMMAND_T*)
{
	int isolo;
	bool bmute;
	MediaTrack* reftk=0;
	if (!g_ReferenceTrackSolo)
	{
		g_ReferenceTrackSolo=true;
		reftk=GuidToTrack(&g_RefTrackGUID);
		if (reftk)
		{
			DoStoreProjectSoloStates();
			SetAllTrackSolos(0);
			isolo=1;
			bmute=false;
			GetSetMediaTrackInfo(reftk,"B_MUTE",&bmute);
			GetSetMediaTrackInfo(reftk,"I_SOLO",&isolo);
			MediaTrack* mtk=CSurf_TrackFromID(0,false);
			if (mtk)
			{
				g_RefMasterVolume=*(double*)GetSetMediaTrackInfo(mtk,"D_VOL",0);
				double mvol=1.0;
				GetSetMediaTrackInfo(mtk,"D_VOL",&mvol);
			}
		} 
		else
			MessageBox(g_hwndParent, __LOCALIZE("Reference track does not exist in this project.\nMaybe it is in another project tab?","sws_mbox"),__LOCALIZE("Xenakios - Error","sws_mbox"), MB_OK);
	}
	else
	{
		reftk=GuidToTrack(&g_RefTrackGUID);
		if (reftk)
		{
			RecallTrackSoloStates();
			g_ReferenceTrackSolo=false;
			isolo=0;
			bmute=true;
			GetSetMediaTrackInfo(reftk,"B_MUTE",&bmute);
			GetSetMediaTrackInfo(reftk,"I_SOLO",&isolo);
			MediaTrack* mtk=CSurf_TrackFromID(0,false);
			if (mtk)
			{
				GetSetMediaTrackInfo(mtk,"D_VOL",&g_RefMasterVolume);
			}
		} 
		else
			MessageBox(g_hwndParent, __LOCALIZE("Reference track does not exist in this project.\nMaybe it is in another project tab?","sws_mbox"),__LOCALIZE("Xenakios - Error","sws_mbox"), MB_OK);
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
	MediaTrack *tk=GuidToTrack(&g_RefTrackGUID);
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

void DoNudgeMasterVol1dbUp(COMMAND_T* ct)
{
	NudgeTrackVolumeDB(0,1.0);
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),UNDO_STATE_TRACKCFG,-1);
}

void DoNudgeMasterVol1dbDown(COMMAND_T* ct)
{
	NudgeTrackVolumeDB(0,-1.0);
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),UNDO_STATE_TRACKCFG,-1);
}

void DoNudgeSelTrackVolumeUp(COMMAND_T* ct)
{
	t_vect_of_Reaper_tracks thetracks;
	XenGetProjectTracks(thetracks,true);
	for (int i=0;i<(int)thetracks.size();i++)
	{
		int index=CSurf_TrackToID(thetracks[i],false);
		NudgeTrackVolumeDB(index,g_command_params.TrackVolumeNudge);
	}
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),UNDO_STATE_TRACKCFG,-1);
}

void DoNudgeSelTrackVolumeDown(COMMAND_T* ct)
{
	t_vect_of_Reaper_tracks thetracks;
	XenGetProjectTracks(thetracks,true);
	for (int i=0;i<(int)thetracks.size();i++)
	{
		int index=CSurf_TrackToID(thetracks[i],false);
		NudgeTrackVolumeDB(index,-g_command_params.TrackVolumeNudge);
	}
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),UNDO_STATE_TRACKCFG,-1);
}

void DoSetMasterToZeroDb(COMMAND_T* ct)
{
	MediaTrack* ptk=CSurf_TrackFromID(0,false);
	if (ptk)
	{
		double newgain=1.0;
		GetSetMediaTrackInfo(ptk,"D_VOL",&newgain);
		Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),UNDO_STATE_TRACKCFG,-1);
	}
}
