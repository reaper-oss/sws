/******************************************************************************
/ ExoticCommands.cpp
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
#include "../SnM/SnM_Util.h"

#include <WDL/localize/localize.h>

using namespace std;

extern MTRand g_mtrand;

void DoJumpEditCursorByRandomAmount(COMMAND_T*)
{
	double RandomMean = g_command_params.EditCurRndMean;
	double RandExp=-log(g_mtrand.rand()) * RandomMean;
	double NewCurPos=GetCursorPosition() + RandExp;
	SetEditCurPos(NewCurPos, false, false);
}

void DoNudgeSelectedItemsPositions(bool UseConfig, bool Positive, double NudgeTime)
{
	const int cnt=CountSelectedMediaItems(NULL);
	for (int i = 0; i < cnt; i++)
	{
		MediaItem* item = GetSelectedMediaItem(0, i);
		double NewPos;
		double OldPos = *(double*)GetSetMediaItemInfo(item, "D_POSITION", NULL);
		if (Positive)
			NewPos = OldPos + g_command_params.ItemPosNudgeSecs;
		else
			NewPos = OldPos - g_command_params.ItemPosNudgeSecs;

		GetSetMediaItemInfo(item, "D_POSITION", &NewPos);
	}
	Undo_OnStateChangeEx("Nudge item position(s)", UNDO_STATE_ITEMS, -1);
	UpdateTimeline();
}

void DoSetItemFadesConfLen(COMMAND_T* ct)
{
	double dFadeInLen  = ct->user == 0 ? g_command_params.CommandFadeInA  : g_command_params.CommandFadeInB;
	double dFadeOutLen = ct->user == 0 ? g_command_params.CommandFadeOutA : g_command_params.CommandFadeOutB;
	double dZero = 0.0;
	WDL_TypedBuf<MediaItem*> items;
	SWS_GetSelectedMediaItems(&items);
	for (int i = 0; i < items.GetSize(); i++)
	{
		double dItemLen= *(double*)GetSetMediaItemInfo(items.Get()[i], "D_LENGTH", NULL);
		double dNewFadeInLen = dFadeInLen;
		double dNewFadeOutLen = dFadeOutLen;
		double dNoFadeTime = 0.01; // 10 ms - makes fade handles actually grabable
		if ((dNewFadeInLen + dNewFadeOutLen + dNoFadeTime) > dItemLen)
		{
			if (dNoFadeTime > dItemLen / 2.0) // Don't let the no-fade-time be more than 50% of the item
				dNoFadeTime = dItemLen / 2.0;
			dNewFadeInLen  = (dItemLen - dNoFadeTime) * dFadeInLen  / (dFadeInLen + dFadeOutLen);
			dNewFadeOutLen = (dItemLen - dNoFadeTime) * dFadeOutLen / (dFadeInLen + dFadeOutLen);
		}

		GetSetMediaItemInfo(items.Get()[i], "C_FADEINSHAPE",  &(ct->user == 0 ? g_command_params.CommandFadeInShapeA  : g_command_params.CommandFadeInShapeB));
		GetSetMediaItemInfo(items.Get()[i], "C_FADEOUTSHAPE", &(ct->user == 0 ? g_command_params.CommandFadeOutShapeA : g_command_params.CommandFadeOutShapeB));
		GetSetMediaItemInfo(items.Get()[i], "D_FADEINLEN",  &dNewFadeInLen);
		GetSetMediaItemInfo(items.Get()[i], "D_FADEOUTLEN", &dNewFadeOutLen);
		GetSetMediaItemInfo(items.Get()[i], "D_FADEINLEN_AUTO",  &dZero);
		GetSetMediaItemInfo(items.Get()[i], "D_FADEOUTLEN_AUTO", &dZero);
	}
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct), UNDO_STATE_ITEMS, -1);
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
			//double OldItemPos=*(double*)GetSetMediaItemInfo(CurItem,"D_POSITION",0);
			double NewPitchSemis=OldPitchSemis+NudgeAmount;
			double NewPlayRate=pow(2.0,NewPitchSemis/12.0);
			//double NewItemPos=OldItemPos*(NewPlayRate/OldPlayRate);
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
	Undo_OnStateChangeEx("Nudge item pitch",4,-1);
	UpdateTimeline();
}

void DoNudgeItemPitchesDown(COMMAND_T*)  { DoNudgeItemPitches(-g_command_params.ItemPitchNudgeA, false); }
void DoNudgeItemPitchesUp(COMMAND_T*)    { DoNudgeItemPitches(g_command_params.ItemPitchNudgeA,  false); }
void DoNudgeItemPitchesDownB(COMMAND_T*) { DoNudgeItemPitches(-g_command_params.ItemPitchNudgeB, false); }
void DoNudgeItemPitchesUpB(COMMAND_T*)   { DoNudgeItemPitches(g_command_params.ItemPitchNudgeB,  false); }
void DoNudgeUpTakePitchResampledA(COMMAND_T*)   { DoNudgeItemPitches(g_command_params.ItemPitchNudgeA,  true); }
void DoNudgeDownTakePitchResampledA(COMMAND_T*) { DoNudgeItemPitches(-g_command_params.ItemPitchNudgeA, true); }
void DoNudgeUpTakePitchResampledB(COMMAND_T*)   { DoNudgeItemPitches(g_command_params.ItemPitchNudgeB,  true); }
void DoNudgeDownTakePitchResampledB(COMMAND_T*) { DoNudgeItemPitches(-g_command_params.ItemPitchNudgeB, true); }

void DoNudgeItemsBeatsBased(bool UseConf, bool Positive, double theNudgeAmount)
{
	MediaTrack* MunRaita;
	MediaItem* CurItem;

	int numItems;
	double NudgeAmount=g_command_params.ItemPosNudgeBeats;
	bool ItemSelected=false;

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
	Undo_OnStateChangeEx(__LOCALIZE("Nudge item position(s), beat based","sws_undo"),4,-1);
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

void DoNudgeSamples(COMMAND_T* ct)
{
	double dSrate = (double)(*ConfigVar<int>("projsrate"));
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* mi = GetTrackMediaItem(tr, j);
			if (*(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL))
			{
				double dPos = *(double*)GetSetMediaItemInfo(mi, "D_POSITION", NULL);
				dPos += (double)ct->user * 1.0 / dSrate;
				GetSetMediaItemInfo(mi, "D_POSITION", &dPos);
			}
		}
	}
	UpdateTimeline();
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct), UNDO_STATE_ITEMS, -1);
}

void DoSplitItemsAtTransients(COMMAND_T* ct)
{
	MediaTrack* MunRaita;
	MediaItem* CurItem;
	bool ItemSelected=false;

	int ItemCounter=0;
	const int numItems=CountSelectedMediaItems(NULL);
	MediaItem** MediaItemsInProject = new MediaItem*[numItems];
	int i, j;
	for (i=0;i<GetNumTracks();i++)
	{
		MunRaita = CSurf_TrackFromID(i+1,FALSE);
		int numItemsTrack=GetTrackNumMediaItems(MunRaita);

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

	Undo_BeginBlock();
	for (i=0;i<numItems;i++)
	{
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
			LastCurPos=CurPos;
			Main_OnCommand(40375,0); // go to next transient in item
			Main_OnCommand(40012,0); // split at edit cursor
			CurPos=GetCursorPosition();

			if (LastCurPos==CurPos)
				NumSplits++;

			if (NumSplits>3) // we will believe after 3 futile iterations that this IS the last transient of the item
				break;
		}
	}

	delete[] MediaItemsInProject;
	//adjustZoom(CurrentHZoom, 0, false, -1);
	UpdateTimeline();
	Undo_EndBlock(SWS_CMD_SHORTNAME(ct),0);
}

void DoNudgeItemVols(bool UseConf,bool Positive,double TheNudgeAmount)
{
	MediaTrack* MunRaita;
	MediaItem* CurItem;

	int numItems;
	double NudgeAmount=g_command_params.ItemVolumeNudge;
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
	Undo_OnStateChangeEx(__LOCALIZE("Nudge item volume","sws_undo"),4,-1);
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
		double OldVol=abs(*(double*)GetSetMediaItemTakeInfo(CurTake,"D_VOL",NULL));
		double OldVolDB=20*(log10(OldVol));
		if (Positive)
			NewVol=OldVolDB+NudgeAmount; else NewVol=OldVolDB-NudgeAmount;
		double NewVolGain;
		if (NewVol>-144.0)
			NewVolGain=exp(NewVol*0.115129254);
			else NewVolGain=0;
		if (IsTakePolarityFlipped(CurTake))
			NewVolGain = -NewVolGain;
		GetSetMediaItemTakeInfo(CurTake,"D_VOL",&NewVolGain);
	}
	Undo_OnStateChangeEx(__LOCALIZE("Nudge take volume","sws_undo"),4,-1);
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

void DoResetItemVol(COMMAND_T* ct)
{
	double NewVol=1.0;
	const int cnt=CountSelectedMediaItems(NULL);
	for (int i = 0; i < cnt; i++)
		GetSetMediaItemInfo(GetSelectedMediaItem(0, i), "D_VOL", &NewVol);
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct), UNDO_STATE_ITEMS, -1);
	UpdateTimeline();
}

void DoResetTakeVol(COMMAND_T* ct)
{
	vector<MediaItem_Take*> ProjectTakes;
	XenGetProjectTakes(ProjectTakes,true,true);
	int i;
	double NewVol;
	for (i = 0; i < (int)ProjectTakes.size(); i++)
	{
		MediaItem_Take* take = ProjectTakes[i];
		IsTakePolarityFlipped(take) ? NewVol = -1.0 : NewVol = 1.0;
		GetSetMediaItemTakeInfo(take, "D_VOL", &NewVol);
	}
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),4,-1);
	UpdateTimeline();
}


typedef struct
{
	double dScaling;
	double dLengthScaling;
} t_ScaleItemPosParams;

t_ScaleItemPosParams g_last_ScaleItemPosParams;

static double *g_StoredPositions;
static double *g_StoredLengths;

void DoScaleItemPosStatic2(double dTheScaling, double dTheLengthScaling, bool bRestorePos)
{
	double dPositionScalingFactor = dTheScaling / 100.0;
	double dLenScalingFactor = dTheLengthScaling / 100.0;
	if (dPositionScalingFactor < 0.0001)
		dPositionScalingFactor = 0.0001;

	// Find minimum item position from all selected
	double dMinTime = DBL_MAX;
	for (int i = 0; i < GetNumTracks(); i++)
	{
		WDL_TypedBuf<MediaItem*> items;
		SWS_GetSelectedMediaItemsOnTrack(&items, CSurf_TrackFromID(i+1, false));
		for (int j = 0; j < items.GetSize(); j++)
		{
			double dPos = *(double*)GetSetMediaItemInfo(items.Get()[j], "D_POSITION", NULL);
			if (dPos < dMinTime)
				dMinTime = dPos;
		}
	}

	int iItem = 0;

	for (int i = 0; i < GetNumTracks(); i++)
	{
		WDL_TypedBuf<MediaItem*> items;
		SWS_GetSelectedMediaItemsOnTrack(&items, CSurf_TrackFromID(i+1, false));
		for (int j = 0; j < items.GetSize(); j++)
		{
			double dNewPos;
			double dNewLen;
			if (!bRestorePos)
			{
				dNewPos = (g_StoredPositions[iItem] - dMinTime) * dPositionScalingFactor + dMinTime;
				dNewLen = g_StoredLengths[iItem] * dLenScalingFactor;
			}
			else
			{
				dNewPos = g_StoredPositions[iItem];
				dNewLen = g_StoredLengths[iItem];
			}
			iItem++;

			GetSetMediaItemInfo(items.Get()[j], "D_POSITION", &dNewPos);
			GetSetMediaItemInfo(items.Get()[j], "D_LENGTH",   &dNewLen);
		}
	}
}

WDL_DLGRET ScaleItemPosDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static HWND hPosScaler = NULL;
	static HWND hLenScaler = NULL;

	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, Message, wParam, lParam))
		return r;

	switch(Message)
    {
        case WM_INITDIALOG:
		{
#ifdef _WIN32
			RECT r;
			GetWindowRect(GetDlgItem(hwnd, IDC_SLIDER1), &r);
			ScreenToClient(hwnd, (LPPOINT)&r);
			ScreenToClient(hwnd, ((LPPOINT)&r)+1);
			hPosScaler = CreateWindowEx(WS_EX_LEFT, "REAPERhfader", "DLGFADER1",
				WS_CHILD | WS_VISIBLE | TBS_VERT,
				r.left, r.top, r.right-r.left, r.bottom-r.top, hwnd, NULL, g_hInst, NULL);
			GetWindowRect(hPosScaler, &r);
			ScreenToClient(hwnd, (LPPOINT)&r);
			ScreenToClient(hwnd, ((LPPOINT)&r)+1);

			GetWindowRect(GetDlgItem(hwnd, IDC_SLIDER2), &r);
			ScreenToClient(hwnd, (LPPOINT)&r);
			ScreenToClient(hwnd, ((LPPOINT)&r)+1);
			hLenScaler = CreateWindowEx(WS_EX_LEFT, "REAPERhfader", "DLGFADER2",
				WS_CHILD | WS_VISIBLE | TBS_HORZ,
				r.left, r.top, r.right-r.left, r.bottom-r.top, hwnd, NULL, g_hInst, NULL);
#else
			hPosScaler = GetDlgItem(hwnd, IDC_SLIDER1);
			hLenScaler = GetDlgItem(hwnd, IDC_SLIDER2);
			ShowWindow(hPosScaler, SW_SHOW);
			ShowWindow(hLenScaler, SW_SHOW);
#endif
			SendMessage(hPosScaler,TBM_SETTIC,0,473);
			SendMessage(hLenScaler,TBM_SETTIC,0,473);
			char TextBuf[314];
			sprintf(TextBuf,"%.2f",g_last_ScaleItemPosParams.dScaling);
			SetDlgItemText(hwnd, IDC_EDIT1, TextBuf);
			sprintf(TextBuf,"%.2f",g_last_ScaleItemPosParams.dLengthScaling);
			SetDlgItemText(hwnd, IDC_EDIT5, TextBuf);
			SetFocus(GetDlgItem(hwnd, IDC_EDIT1));
			SendMessage(GetDlgItem(hwnd, IDC_EDIT1), EM_SETSEL, 0, -1);
			return 0;
		}
        case WM_HSCROLL:
		{
			char TextBuf[314];
			int SliderPos = (int)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
			double dPosScalFact = 10+((190.0/1000)*SliderPos);
			if ((HWND)lParam == hPosScaler)
			{
				g_last_ScaleItemPosParams.dScaling = dPosScalFact;
				sprintf(TextBuf, "%.2f", g_last_ScaleItemPosParams.dScaling);
				SetDlgItemText(hwnd, IDC_EDIT1, TextBuf);
			}
			if ((HWND)lParam==hLenScaler)
			{
				g_last_ScaleItemPosParams.dLengthScaling = dPosScalFact;
				sprintf(TextBuf, "%.2f", g_last_ScaleItemPosParams.dLengthScaling);
				SetDlgItemText(hwnd, IDC_EDIT5, TextBuf);
			}
			DoScaleItemPosStatic2(g_last_ScaleItemPosParams.dScaling, g_last_ScaleItemPosParams.dLengthScaling, false);
			UpdateTimeline();
			break;
		}

		case WM_COMMAND:
		{
            switch(LOWORD(wParam))
            {
				case IDC_EDIT1:
					if (HIWORD(wParam) == EN_CHANGE)
					{
						char textbuf[100];
						GetDlgItemText(hwnd, IDC_EDIT1, textbuf, 100);
						g_last_ScaleItemPosParams.dScaling = atof(textbuf);
						SendMessage(hPosScaler, TBM_SETPOS, 1, (int)(1000.0*(g_last_ScaleItemPosParams.dScaling-10.0)/190));
					}
					break;
				case IDC_EDIT5:
					if (HIWORD(wParam) == EN_CHANGE)
					{
						char textbuf[100];
						GetDlgItemText(hwnd, IDC_EDIT5, textbuf, 100);
						g_last_ScaleItemPosParams.dLengthScaling = atof(textbuf);
						SendMessage(hLenScaler, TBM_SETPOS, 1, (int)(1000.0 * (g_last_ScaleItemPosParams.dLengthScaling-10.0) / 190));
					}
					break;
				case IDC_APPLY:
					DoScaleItemPosStatic2(g_last_ScaleItemPosParams.dScaling, g_last_ScaleItemPosParams.dLengthScaling, false);
					UpdateTimeline();
					break;
				case IDOK:
					DoScaleItemPosStatic2(g_last_ScaleItemPosParams.dScaling, g_last_ScaleItemPosParams.dLengthScaling, false);
					Undo_OnStateChangeEx("Scale Item Positions By Percentage", 4, -1);
					UpdateTimeline();
					EndDialog(hwnd, 0);
					break;
				case IDCANCEL:
					DoScaleItemPosStatic2(g_last_ScaleItemPosParams.dScaling, g_last_ScaleItemPosParams.dLengthScaling, true);
					UpdateTimeline();
					EndDialog(hwnd, 0);
					break;
			}
			break;
		}
		case WM_DESTROY:
		{
			DestroyWindow(hPosScaler);
			DestroyWindow(hLenScaler);
			break;
		}
	}
	return 0;
}


void DoScaleItemPosStaticDlg(COMMAND_T*)
{
	static bool bScaleItemPosFirstRun = true;
	if (bScaleItemPosFirstRun)
	{
		g_last_ScaleItemPosParams.dScaling = 100.0;
		g_last_ScaleItemPosParams.dLengthScaling = 100.0;
		bScaleItemPosFirstRun = false;
	}
	WDL_TypedBuf<MediaItem*> items;
	SWS_GetSelectedMediaItems(&items);
	g_StoredPositions = new double[items.GetSize()];
	g_StoredLengths = new double[items.GetSize()];
	for (int i = 0; i < items.GetSize(); i++)
	{
		g_StoredPositions[i] = *(double*)GetSetMediaItemInfo(items.Get()[i], "D_POSITION", NULL);
		g_StoredLengths[i]   = *(double*)GetSetMediaItemInfo(items.Get()[i], "D_LENGTH", NULL);
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
int g_last_RandomizeItemsPosGroup;
bool g_RandItemPosFirstRun = true;

void DoRandomizePositions2(int obeyGroup)
{
	double dSpread = g_last_RandomizeItemPosParams.RandRange;

	if (obeyGroup == 0)
	{
		const int cnt=CountSelectedMediaItems(NULL);
		for (int i = 0; i < cnt; i++)
		{
			MediaItem* mi = GetSelectedMediaItem(NULL, i);
			double dItemPos = *(double*)GetSetMediaItemInfo(mi, "D_POSITION", NULL);
			dItemPos += -dSpread + (2.0/RAND_MAX)*rand()*dSpread;
			GetSetMediaItemInfo(mi, "D_POSITION", &dItemPos);
		}
	}

	else
	{
		WDL_TypedBuf <int> processed;
		const int itemCount = CountMediaItems(NULL);
		const int itemSelCount=CountSelectedMediaItems(NULL);
		for (int i = 0; i < itemSelCount; i++)
		{
			MediaItem* mi = GetSelectedMediaItem(NULL, i);
			double posDiff = -dSpread + (2.0/RAND_MAX)*rand()*dSpread;
			int group = *(int*)GetSetMediaItemInfo(mi, "I_GROUPID", NULL);

			if (group != 0 && processed.Find(group) == -1)
			{
				for (int j = 0; j < itemCount; j++)
				{
					MediaItem* mi = GetMediaItem(NULL, j);
					if (group == *(int*)GetSetMediaItemInfo(mi, "I_GROUPID", NULL))
					{
						double dItemPos = *(double*)GetSetMediaItemInfo(mi, "D_POSITION", NULL);
						dItemPos += posDiff;
						GetSetMediaItemInfo(mi, "D_POSITION", &dItemPos);
					}
				}
				int pos = processed.GetSize();
				processed.Resize(pos + 1);
				processed.Get()[pos] = group;
			}

			else if (group == 0)
			{
				double dItemPos = *(double*)GetSetMediaItemInfo(mi, "D_POSITION", NULL);
				dItemPos += posDiff;
				GetSetMediaItemInfo(mi, "D_POSITION", &dItemPos);
			}
		}
	}
	UpdateTimeline();
	Undo_OnStateChangeEx(__LOCALIZE("Randomize item positions","sws_undo"),4,-1);
}

WDL_DLGRET RandomizeItemPosDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, Message, wParam, lParam))
		return r;

	switch(Message)
    {
        case WM_INITDIALOG:
		{
			char TextBuf[314];

			sprintf(TextBuf,"%.2f",g_last_RandomizeItemPosParams.RandRange);
			SetDlgItemText(hwnd, IDC_EDIT1, TextBuf);
			CheckDlgButton(hwnd, IDC_CHECK1, !!g_last_RandomizeItemsPosGroup);
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
					GetDlgItemText(hwnd,IDC_EDIT1,textbuf,100);
					g_last_RandomizeItemPosParams.RandRange=atof(textbuf);
					g_last_RandomizeItemsPosGroup = IsDlgButtonChecked(hwnd, IDC_CHECK1);
					if (g_last_RandomizeItemsPosGroup == 0)
						DoRandomizePositions2(0);
					else
						DoRandomizePositions2(1);
					EndDialog(hwnd,0);
					break;
				}
				case IDCANCEL:
					EndDialog(hwnd,0);
					break;
			}
			break;
	}
	return 0;
}

void DoRandomizePositionsDlg(COMMAND_T*)
{
	if (g_RandItemPosFirstRun)
	{
		g_last_RandomizeItemPosParams.RandRange=0.1;
		g_last_RandomizeItemsPosGroup = 1;
		g_RandItemPosFirstRun=false;


	}
	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_RANDTEMPOS), g_hwndParent, RandomizeItemPosDlgProc);
}

#define IDC_FIRSTMIXSTRIP 101

typedef struct
{
	int NumTakes;
	HWND *g_hVolSliders;
	HWND *g_hPanSliders;
	HWND g_hItemVolSlider;
	HWND *g_hNumLabels;
	double *StoredVolumes;
	double *StoredPans;
	double StoredItemVolume;
	bool AllTakesPlay;
} t_TakeMixerState;

t_TakeMixerState g_TakeMixerState;

MediaItem *g_TargetItem;

void UpdateTakeMixerSliders()
{
	int SliPos;
	for (int i = 0; i < g_TakeMixerState.NumTakes; i++)
	{
		SliPos=(int)(1000.0/2*g_TakeMixerState.StoredVolumes[i]);
		SendMessage(g_TakeMixerState.g_hVolSliders[i],TBM_SETPOS,(WPARAM) (BOOL)true,SliPos);
		double PanPos=g_TakeMixerState.StoredPans[i];
		PanPos=PanPos+1.0;
		SliPos=(int)(1000.0/2*PanPos);
		SendMessage(g_TakeMixerState.g_hPanSliders[i],TBM_SETPOS,(WPARAM) (BOOL)true,SliPos);
	}
	SliPos=(int)(1000*g_TakeMixerState.StoredItemVolume);
	SendMessage(g_TakeMixerState.g_hItemVolSlider,TBM_SETPOS,(WPARAM) (BOOL)true,SliPos);
}

void On_SliderMove(HWND theHwnd,WPARAM wParam,LPARAM lParam,HWND SliderHandle,int TheSlipos)
{
	int x=-1;
	int movedParam=-1; // 0 vol , 1 pan
	int i;
	for (i=0;i<g_TakeMixerState.NumTakes;i++)
	{
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
			double NewVol=( 2.0/1000*TheSlipos);
			if (CurTake)
			{
				if (IsTakePolarityFlipped(CurTake))
				{
					if (TheSlipos == 0)
						NewVol = -0.00000001; // trick to prevent polarity reset if take vol. is set to 0.0 (-inf)
					else 
						NewVol = -NewVol;
				}
				GetSetMediaItemTakeInfo(CurTake, "D_VOL", &NewVol);
			}
		}
		if (movedParam==1)
		{
			double NewPan=-1.0+(2.0/1000*TheSlipos);
			if (CurTake)
				GetSetMediaItemTakeInfo(CurTake,"D_PAN",&NewPan);
		}
		UpdateTimeline();
	}
	if (SliderHandle==g_TakeMixerState.g_hItemVolSlider)
	{
		double NewVol=( 1.0/1000*TheSlipos);
		GetSetMediaItemInfo(g_TargetItem,"D_VOL",&NewVol);
		UpdateTimeline();
	}
}

void TakeMixerResetTakes(bool ResetVol=false,bool ResetPan=false)
{
	MediaItem_Take *CurTake;
	for (int i = 0; i < GetMediaItemNumTakes(g_TargetItem); i++)
	{
		CurTake=GetMediaItemTake(g_TargetItem,i);
		double NewVolume = IsTakePolarityFlipped(CurTake) ? -1.0 : 1.0;
		double NewPan=0.0;
		if (CurTake && ResetVol==true)
			GetSetMediaItemTakeInfo(CurTake,"D_VOL",&NewVolume);
		if (CurTake && ResetPan==true)
			GetSetMediaItemTakeInfo(CurTake,"D_PAN",&NewPan);
	}
	int SliPos;

	for (int i = 0; i < g_TakeMixerState.NumTakes; i++)
	{
		SliPos=(int)(1000.0/2*1.0);
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
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, Message, wParam, lParam))
		return r;

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
#ifdef _WIN32
			HFONT hFont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
#endif
			for (int i=0;i<g_TakeMixerState.NumTakes;i++)
			{
#ifdef _WIN32
				sprintf(textbuf,"TAKESTATIC_%d",i+1);
				g_TakeMixerState.g_hNumLabels[i] = CreateWindowEx(WS_EX_LEFT, WC_STATIC, textbuf,
				WS_CHILD | WS_VISIBLE,
				45+(1+i)*50, 20, 40, 20, hwnd, NULL, g_hInst, NULL);

				sprintf(textbuf,"VOLSLIDER_%d",i+1);
				g_TakeMixerState.g_hVolSliders[i] = CreateWindowEx(WS_EX_LEFT, "REAPERvfader", textbuf,
				WS_CHILD | WS_VISIBLE | TBS_VERT,
				40+(1+i)*50, 90, 30, 120, hwnd, NULL, g_hInst, NULL);

				sprintf(textbuf,"PANSLIDER_%d",i+1);
				g_TakeMixerState.g_hPanSliders[i] = CreateWindowEx(WS_EX_LEFT, "REAPERhfader", textbuf,
				WS_CHILD | WS_VISIBLE | TBS_HORZ,
				33+(1+i)*50, 55, 45, 25, hwnd, NULL, g_hInst, NULL);
				SendMessage(g_TakeMixerState.g_hNumLabels[i],WM_SETFONT, (WPARAM)hFont, 0);
#else
				g_TakeMixerState.g_hNumLabels[i]  = SWELL_MakeLabel(0, "STATIC", i*4+1, 12+(1+i)*32, 14, 27, 9, 0);
				g_TakeMixerState.g_hVolSliders[i] = SWELL_MakeControl("DLGFADER1", i*4+2, "REAPERhfader", 0, 15+(1+i)*32, 50, 20, 80, 0);
				g_TakeMixerState.g_hPanSliders[i] = SWELL_MakeControl("DLGFADER1", i*4+3, "REAPERhfader", 0, 10+(1+i)*32, 27, 30, 17, 0);
#endif

				sprintf(textbuf,"%d",i+1);
				SetWindowText(g_TakeMixerState.g_hNumLabels[i],textbuf);
				SendMessage(g_TakeMixerState.g_hVolSliders[i],TBM_SETTIC,0,500);
				SendMessage(g_TakeMixerState.g_hPanSliders[i],TBM_SETTIC,0,500);
			}
#ifdef _WIN32
			g_TakeMixerState.g_hItemVolSlider = CreateWindowEx(WS_EX_LEFT, "REAPERvfader", "ITEMVOLSLIDER",
			WS_CHILD | WS_VISIBLE | TBS_VERT,
			40, 90, 30, 120, hwnd, NULL, g_hInst, NULL);
#else
			g_TakeMixerState.g_hItemVolSlider = SWELL_MakeControl("DLGFADER1", 666, "REAPERhfader", 0, 12, 50, 20, 80, 0);
#endif
			SendMessage(g_TakeMixerState.g_hItemVolSlider,TBM_SETTIC,0,500);
			UpdateTakeMixerSliders();
			return 0;
		}
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
				case IDC_BUTTON1:
					TakeMixerResetTakes(true,false); // true for volume, false for pan
					UpdateTimeline();
					break;
				case IDC_BUTTON2:
					TakeMixerResetTakes(false,true); // true for volume, false for pan
					UpdateTimeline();
					break;
				case IDOK:
					Undo_OnStateChangeEx(__LOCALIZE("Set take vol/pan","sws_undo"),4,-1);
					EndDialog(hwnd,0);
					break;
				case IDCANCEL:
				{
					MediaItem_Take *CurTake;
					int i;
					for (i=0;i<g_TakeMixerState.NumTakes;i++)
					{
						//
						CurTake=GetMediaItemTake(g_TargetItem,i);
						double NewVol=g_TakeMixerState.StoredVolumes[i];
						if (CurTake)
						{
							if (IsTakePolarityFlipped(CurTake)) // we stored absolute value in DoShowTakeMixerDlg()
								NewVol = -NewVol;
							GetSetMediaItemTakeInfo(CurTake, "D_VOL", &NewVol);
						}
							
						double NewPan=g_TakeMixerState.StoredPans[i];
						if (CurTake)
							GetSetMediaItemTakeInfo(CurTake,"D_PAN",&NewPan);
					}
					GetSetMediaItemInfo(g_TargetItem,"B_ALLTAKESPLAY",&g_TakeMixerState.AllTakesPlay);
					GetSetMediaItemInfo(g_TargetItem,"D_VOL",&g_TakeMixerState.StoredItemVolume);
					UpdateTimeline();
					EndDialog(hwnd,0);
					break;
				}
			}
			break;
		case WM_VSCROLL:
		{
			int ThePos=(int)SendMessage((HWND)lParam,TBM_GETPOS,0,0);
			On_SliderMove(hwnd,wParam,lParam,(HWND)lParam,ThePos);
			break;
		}
		case WM_HSCROLL:
		{
			int ThePos=(int)SendMessage((HWND)lParam,TBM_GETPOS,0,0);
			On_SliderMove(hwnd,wParam,lParam,(HWND)lParam,ThePos);
			break;
		}
		case WM_DESTROY:
			DestroyWindow(g_TakeMixerState.g_hItemVolSlider);
			for (int i = 0; i < g_TakeMixerState.NumTakes; i++)
			{
				DestroyWindow(g_TakeMixerState.g_hNumLabels[i]);
				DestroyWindow(g_TakeMixerState.g_hVolSliders[i]);
				DestroyWindow(g_TakeMixerState.g_hPanSliders[i]);
			}
			break;
	}
	return 0;
}

void DoShowTakeMixerDlg(COMMAND_T*)
{
	g_TargetItem = NULL;

	if (CountSelectedMediaItems(NULL) != 1) {
		MessageBox(g_hwndParent, __LOCALIZE("Please select only one item","sws_mbox"), __LOCALIZE("Xenakios - Error","sws_mbox"), MB_OK);
		return;
	}

	g_TargetItem = GetSelectedMediaItem(0, 0);
	g_TakeMixerState.NumTakes			= GetMediaItemNumTakes(g_TargetItem);
	g_TakeMixerState.AllTakesPlay		= *(bool*)GetSetMediaItemInfo(g_TargetItem ,"B_ALLTAKESPLAY", NULL);
	GetSetMediaItemInfo(g_TargetItem, "B_ALLTAKESPLAY", &g_bTrue);
	g_TakeMixerState.StoredItemVolume	= *(double*)GetSetMediaItemInfo(g_TargetItem ,"D_VOL", NULL);
	g_TakeMixerState.g_hVolSliders		= new HWND[g_TakeMixerState.NumTakes+1];
	g_TakeMixerState.g_hNumLabels		= new HWND[g_TakeMixerState.NumTakes];
	g_TakeMixerState.g_hPanSliders		= new HWND[g_TakeMixerState.NumTakes];
	g_TakeMixerState.StoredVolumes		= new double[g_TakeMixerState.NumTakes];
	g_TakeMixerState.StoredPans			= new double[g_TakeMixerState.NumTakes];

	for (int i = 0; i < g_TakeMixerState.NumTakes; i++)
	{
		MediaItem_Take* CurTake = GetMediaItemTake(g_TargetItem, i);
		g_TakeMixerState.StoredVolumes[i]= CurTake ? abs(*(double*)GetSetMediaItemTakeInfo(CurTake,"D_VOL",NULL)) : 0.0;
		g_TakeMixerState.StoredPans[i]= CurTake ? *(double*)GetSetMediaItemTakeInfo(CurTake,"D_PAN",NULL) : 0.0;
	}

	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_TAKEMIXER), g_hwndParent, TakeMixerDlgProc);
	delete[] g_TakeMixerState.g_hVolSliders;
	delete[] g_TakeMixerState.g_hPanSliders;
	delete[] g_TakeMixerState.g_hNumLabels;
	delete[] g_TakeMixerState.StoredPans;
	delete[] g_TakeMixerState.StoredVolumes;
}

double g_PrevEditCursorPos=-1.0;

void DoSplitAndChangeLen(bool BeatBased)
{
	double StoredTimeSelStart;
	double StoredTimeSelEnd;
	GetSet_LoopTimeRange(false,false,&StoredTimeSelStart,&StoredTimeSelEnd,false);
	Undo_BeginBlock();
	Main_OnCommand(40012,0); // split item at edit cursor
	if (!BeatBased)
	{
		double NewStart=GetCursorPosition();
		double NewEnd=NewStart+g_command_params.ItemPosNudgeSecs;
		GetSet_LoopTimeRange(true,false,&NewStart,&NewEnd,false);
	}
	else
	{
		double NewStartSecs=GetCursorPosition();
		double NewStartBeats=TimeMap_timeToQN(NewStartSecs);
		double NewEndBeats=NewStartBeats+g_command_params.ItemPosNudgeBeats;
		double NewEndSecs=TimeMap_QNToTime(NewEndBeats);
		GetSet_LoopTimeRange(true,false,&NewStartSecs,&NewEndSecs,false);
	}

	Main_OnCommand(40061,0); // split item at time selection
	Main_OnCommand(40006,0); // remove selected items
	Undo_EndBlock(__LOCALIZE("Erase time from item","sws_undo"),0);
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
#ifdef _WIN32
	if (OpenClipboard(g_hwndParent))
	{
		char clipstring[4096] = "";
		HANDLE hData = GetClipboardData(CF_TEXT);
		if (hData)
		{
			char* buffer = (char*)GlobalLock(hData);
			lstrcpyn(clipstring,buffer,4096);
			GlobalUnlock(hData);
		}
		else
		{
			HANDLE hData = GetClipboardData(CF_HDROP);
			if (hData)
			{
				char* pFile = (char*)GlobalLock(hData);
				pFile += *pFile;
				int i = 0;
				while (*pFile)
				{
					clipstring[i++] = *pFile;
					pFile += 2;
				}

				GlobalUnlock(hData);
			}
		}

		if (clipstring[0] && FileExists(clipstring))
			InsertMedia(clipstring,0);
		CloseClipboard();
	}
#else
	MessageBox(g_hwndParent, __LOCALIZE("Not supported on OSX and Linux, sorry!", "sws_mbox"), __LOCALIZE("SWS - Error", "sws_mbox"), MB_OK);
#endif
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
	int NumItems = 0;
	for (int i = 0; i < GetNumTracks(); i++)
		NumItems += GetTrackNumMediaItems(CSurf_TrackFromID(i+1, false));

	g_Project_Items = new t_reaper_item[NumItems];
	g_NumProjectItems = NumItems;
	int ItemCounter = 0;
	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* MunRaita = CSurf_TrackFromID(i+1, false);
		NumItems = GetTrackNumMediaItems(MunRaita);
		for (int j = 0; j < NumItems; j++)
		{
			MediaItem* CurItem = GetTrackMediaItem(MunRaita, j);
			int NumTakes = GetMediaItemNumTakes(CurItem);
			g_Project_Items[ItemCounter].ReaperItem = CurItem;
			g_Project_Items[ItemCounter].Takes = new MediaItem_Take*[NumTakes];
			g_Project_Items[ItemCounter].MatchesCrit = false;
			g_Project_Items[ItemCounter].MatchingTake = -1;
			for (int k = 0; k < NumTakes; k++)
				g_Project_Items[ItemCounter].Takes[k] = GetMediaItemTake(CurItem, k);

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
	PreventUIRefresh(1);
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
	PreventUIRefresh(-1);
	UpdateArrange();
	return NumMatches;
}


WDL_DLGRET TakeFinderDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, Message, wParam, lParam))
		return r;

	switch(Message)
    {
        case WM_INITDIALOG:
		{
			char labtxt[256];
			SetFocus(GetDlgItem(hwnd, IDC_EDIT1));
			SendMessage(GetDlgItem(hwnd, IDC_EDIT1), EM_SETSEL, 0, -1);
			snprintf(labtxt,sizeof(labtxt),__LOCALIZE_VERFMT("Search from %d items","sws_DLG_130"),g_NumProjectItems);
			SetDlgItemText(hwnd,IDC_STATIC1,labtxt);
			SetFocus(GetDlgItem(hwnd, IDC_EDIT1));
			SendMessage(GetDlgItem(hwnd, IDC_EDIT1), EM_SETSEL, 0, -1);
			break;
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
					snprintf(labtxt,sizeof(labtxt),__LOCALIZE_VERFMT("Found %d matching items","sws_DLG_130"),NumMatches);
					SetDlgItemText(hwnd,IDC_STATIC1,labtxt);
					if (NumMatches>0)
					{
						Main_OnCommand(40290,0); // time selection to selected items
						Main_OnCommand(40031,0); // zoom to time selection
					}
				}
				break;
			}
			switch(LOWORD(wParam))
            {
				case IDOK:
				case IDCANCEL:
					EndDialog(hwnd,0);
					break;
			}
			break;
	}
	return 0;
}


void DoSearchTakesDLG(COMMAND_T*)
{
	ConstructItemDatabase();
	DialogBox(g_hInst,MAKEINTRESOURCE(IDD_TAKEFINDER), g_hwndParent, TakeFinderDlgProc);
	delete[] g_Project_Items;
}

void DoMoveCurConfPixRight(COMMAND_T*)		{ for (int i=0; i < g_command_params.PixAmount; i++) Main_OnCommand(40105,0); }
void DoMoveCurConfPixLeft(COMMAND_T*)		{ for (int i=0; i < g_command_params.PixAmount; i++) Main_OnCommand(40104,0); }
void DoMoveCurConfPixRightCts(COMMAND_T*)	{ for (int i=0; i < g_command_params.PixAmount; i++) Main_OnCommand(40103,0); }
void DoMoveCurConfPixLeftCts(COMMAND_T*)	{ for (int i=0; i < g_command_params.PixAmount; i++) Main_OnCommand(40102,0); }

void DoMoveCurConfSecsLeft(COMMAND_T*)
{
	MoveEditCursor(-g_command_params.CurPosSecsAmount, false);
}

void DoMoveCurConfSecsRight(COMMAND_T*)
{
	MoveEditCursor(g_command_params.CurPosSecsAmount, false);
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
