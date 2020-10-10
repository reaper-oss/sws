/******************************************************************************
/ MoreItemCommands.cpp
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
#include "../SnM/SnM_Util.h"

#include <WDL/localize/localize.h>
#include <WDL/MersenneTwister.h>

using namespace std;

MTRand g_mtrand;

void(*g_KeyUpUndoHandler)()=0;

struct t_itemposremap_params
{
	double dCurve;
	std::vector<double> dStoredPositions;
};

t_itemposremap_params g_itemposremap_params;

void DoRemapItemPositions(bool bRestorePos)
{
	// Find minimum and maximum item positions from all selected
	double dMinTime = DBL_MAX;
	double dMaxTime = -DBL_MAX;
	WDL_TypedBuf<MediaItem*> items;
	SWS_GetSelectedMediaItems(&items);
	for (int i = 0; i < items.GetSize(); i++)
	{
		double dPos = *(double*)GetSetMediaItemInfo(items.Get()[i], "D_POSITION", NULL);
		dMinTime = std::min(dPos, dMinTime);
		dMaxTime = std::max(dPos, dMaxTime);
	}

	for (int i = 0; i < items.GetSize(); i++)
	{
		double dNormalizedTime = (1.0 / (dMaxTime - dMinTime)) * (g_itemposremap_params.dStoredPositions[i] - dMinTime);
		double dShapedVal;
		if (g_itemposremap_params.dCurve >= 1.0)
			dShapedVal = pow(dNormalizedTime, g_itemposremap_params.dCurve);
		else
			dShapedVal = 1.0 - pow(dNormalizedTime, 1.0 / g_itemposremap_params.dCurve);

		double dNewPos;
		if (!bRestorePos)
			dNewPos = dShapedVal * (dMaxTime - dMinTime) + dMinTime;
		else
			dNewPos = g_itemposremap_params.dStoredPositions[i];

		GetSetMediaItemInfo(items.Get()[i], "D_POSITION", &dNewPos);
	}
}

WDL_DLGRET ItemPosRemapDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static HWND hCurveSlider = NULL;

	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, Message, wParam, lParam))
		return r;

	switch(Message)
    {
        case WM_INITDIALOG:
		{
			char labtxt[314];
			sprintf(labtxt,"%.2f",g_itemposremap_params.dCurve);
			SetDlgItemText(hwnd,IDC_IPRCURVE,labtxt);
#ifdef _WIN32
			RECT r;
			GetWindowRect(GetDlgItem(hwnd, IDC_SLIDER1), &r);
			ScreenToClient(hwnd, (LPPOINT)&r);
			ScreenToClient(hwnd, ((LPPOINT)&r)+1);
			hCurveSlider = CreateWindowEx(WS_EX_LEFT, "REAPERhfader", "DLGFADER1",
				WS_CHILD | WS_VISIBLE | TBS_HORZ,
				r.left, r.top, r.right-r.left, r.bottom-r.top, hwnd, NULL, g_hInst, NULL);
#else
			hCurveSlider = GetDlgItem(hwnd, IDC_SLIDER1);
			ShowWindow(hCurveSlider, SW_SHOW);
#endif
			SetFocus(GetDlgItem(hwnd, IDC_IPRCURVE));
			SendMessage(GetDlgItem(hwnd, IDC_IPRCURVE), EM_SETSEL, 0, -1);
			break;
		}
        case WM_HSCROLL:
		{
			char TextBuf[314];
			int SliderPos=(int)SendMessage((HWND)lParam,TBM_GETPOS,0,0);
			double PosScalFact=.1+1.9*SliderPos/1000.0;
			sprintf(TextBuf,"%.2f",PosScalFact);
			SetDlgItemText(hwnd, IDC_IPRCURVE, TextBuf);
			break;
		}
		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case IDC_IPRCURVE:
					if (HIWORD(wParam) == EN_CHANGE)
					{
						char txtbuf[256];
						GetDlgItemText(hwnd, IDC_IPRCURVE, txtbuf, 256);
						g_itemposremap_params.dCurve = atof(txtbuf);
						SendMessage(hCurveSlider, TBM_SETPOS, 1, (int)(1000.0*(g_itemposremap_params.dCurve-0.1)/1.9));
					}
					break;
				case IDC_APPLY:
				{
					DoRemapItemPositions(false);
					UpdateTimeline();
					break;
				}
				case IDOK:
				{
					DoRemapItemPositions(false);
					UpdateTimeline();
					Undo_OnStateChangeEx(__LOCALIZE("Remap item positions","sws_undo"), 4, -1);
					EndDialog(hwnd, 0);
					break;
				}
				case IDCANCEL:
				{
					DoRemapItemPositions(true);
					UpdateTimeline();
					EndDialog(hwnd, 0);
					break;
				}
			}
			break;
		}
		case WM_DESTROY:
			DestroyWindow(hCurveSlider);
			break;
	}
	return 0;
}

void DoItemPosRemapDlg(COMMAND_T*)
{
	static bool bFirstRun = true;
	if (bFirstRun)
	{
		g_itemposremap_params.dCurve = 1.0;
		bFirstRun = false;
	}

	// Save the item positions
	WDL_TypedBuf<MediaItem*> items;
	SWS_GetSelectedMediaItems(&items);
	g_itemposremap_params.dStoredPositions.resize(items.GetSize());
	for (int i = 0; i < items.GetSize(); i++)
		g_itemposremap_params.dStoredPositions[i] = *(double*)GetSetMediaItemInfo(items.Get()[i], "D_POSITION", NULL);

	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ITEMPOSREMAP), g_hwndParent, ItemPosRemapDlgProc);
}

void ExtractFileNameEx(const char *FullFileName,char *Filename,bool StripExtension)
{
	const char* pFile = strrchr(FullFileName, PATH_SLASH_CHAR);
	if (!pFile)
		pFile = FullFileName;
	else
		pFile++;

	strcpy(Filename, pFile);

	if (StripExtension)
	{
		char* pExt = strrchr(Filename, '.');
		if (pExt)
			*pExt = 0;
	}
}


struct t_cuestruct
{
	double StartTime;
	double EndTime;
};

bool MyCueSortFunction (t_cuestruct a,t_cuestruct b)
{
	if (a.StartTime<b.StartTime) return true;
	return false;
}


void DoItemCueTransform(bool donextcue, int ToCueIndex, bool PreserveItemLen=false,bool RespectSnap=true)
{
	t_cuestruct NewCueStruct;
	vector<t_cuestruct> VecItemCues;

	MediaItem *CurItem = NULL;
	MediaItem_Take *CurTake = NULL;
	MediaTrack* CurTrack = NULL;
	for (int i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		int NumItems=GetTrackNumMediaItems(CurTrack);
		for (int j=0;j<NumItems;j++)
		{
			CurItem=GetTrackMediaItem(CurTrack,j);
			bool IsItemSelected=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (IsItemSelected==true)
			{
				double itemSnapOffs=*(double*)GetSetMediaItemInfo(CurItem,"D_SNAPOFFSET",0);
				VecItemCues.clear();
				CurTake=GetMediaItemTake(CurItem,-1);
				if (CurTake)
				{
					double TakePlayRate=*(double*)GetSetMediaItemTakeInfo(CurTake,"D_PLAYRATE",NULL);
					PCM_source *TakeSource=(PCM_source*)GetSetMediaItemTakeInfo(CurTake,"P_SOURCE",NULL);
					REAPER_cue *CurCue = NULL;
					int cueIndx=0;
					bool morecues=true;
					while (TakeSource && morecues)
					{
						int rc=TakeSource->Extended(PCM_SOURCE_EXT_ENUMCUES,(void *)(INT_PTR)cueIndx,&CurCue,NULL);
						if (rc==0) break;
						if (CurCue!=0)
						{
							NewCueStruct.StartTime=CurCue->m_time-(TakePlayRate* itemSnapOffs);
							if (NewCueStruct.StartTime<0.0) NewCueStruct.StartTime=0;
							VecItemCues.push_back(NewCueStruct);
						}
						cueIndx++;
						if (cueIndx>1000) break; // sanity check
					}
					if (VecItemCues.size()>0)
					{
						sort(VecItemCues.begin(),VecItemCues.end(),MyCueSortFunction);
						//TimeSort(VecItemCues);
						if (VecItemCues[0].StartTime>0.0)
						{
						NewCueStruct.StartTime=0.0;
						NewCueStruct.EndTime=VecItemCues[0].StartTime;
						VecItemCues.insert(VecItemCues.begin(),NewCueStruct);
						}	else VecItemCues[0].EndTime=VecItemCues[1].StartTime;
					//NewCueStruct.StartTime=VecItemCues[VecItemCues.size()-1].EndTime;
					//NewCueStruct.EndTime=TakeSource->GetLength();
					//VecItemCues.push_back(NewCueStruct);
						for (int idiot=1;idiot<(int)VecItemCues.size();idiot++)
						{
							if (idiot<(int)VecItemCues.size()-1)
							VecItemCues[idiot].EndTime=VecItemCues[idiot+1].StartTime;
								else VecItemCues[idiot].EndTime=TakeSource->GetLength();
						}

						int CurrentCueIndex=0;
						double MediaOffset=*(double*)GetSetMediaItemTakeInfo(CurTake,"D_STARTOFFS",NULL);
						for (int idiot=0;idiot<(int)VecItemCues.size();idiot++)
						{
							if (MediaOffset>=VecItemCues[idiot].StartTime && MediaOffset<VecItemCues[idiot].EndTime)
							{
								CurrentCueIndex=idiot;
								break;
							}
						}
						int NewCueIndex=0;
						if (ToCueIndex==-1) // -1 for next/previous cue
						{

							if (donextcue==true)
							{
								NewCueIndex=CurrentCueIndex+1;
								if (NewCueIndex>(int)VecItemCues.size()-1) NewCueIndex=0;
							}
							if (donextcue==false)
							{
								NewCueIndex=CurrentCueIndex-1;
								if (NewCueIndex<0) NewCueIndex=(int)VecItemCues.size()-1;
							}
						}
						if (ToCueIndex==-2) // full random
						{
							NewCueIndex=g_mtrand.randInt() % VecItemCues.size();
						}
						if (ToCueIndex>=0 && ToCueIndex<(int)VecItemCues.size())
							NewCueIndex=ToCueIndex;
						double NewTimeA=VecItemCues[NewCueIndex].StartTime;
						//if (RespectSnap)
						//  NewTimeA-=itemSnapOffs*TakePlayRate;
						//if (RespectSnap && donextcue==true)
						//	NewTimeA-=itemSnapOffs*TakePlayRate;
						//if (NewTimeA<0.0) NewTimeA=0.0;
						double NewTimeB=VecItemCues[NewCueIndex].EndTime;
						double NewLength=(NewTimeB*(1.0/TakePlayRate)) - (NewTimeA*(1.0/TakePlayRate));
						if (!PreserveItemLen)
							GetSetMediaItemInfo(CurItem,"D_LENGTH",&NewLength);
						GetSetMediaItemTakeInfo(CurTake,"D_STARTOFFS",&NewTimeA);

					}
				}
			}
		}
	}
	Undo_OnStateChangeEx(__LOCALIZE("Switch item contents based on cue","sws_undo"),UNDO_STATE_ITEMS,-1);
	UpdateTimeline();
}

void DoSwitchItemToNextCue(COMMAND_T*)
{
	DoItemCueTransform(true,-1);
}

void DoSwitchItemToPreviousCue(COMMAND_T*)
{
	DoItemCueTransform(false,-1);
}

void DoSwitchItemToNextCuePresvLen(COMMAND_T*)
{
	DoItemCueTransform(true,-1,true);
}

void DoSwitchItemToPreviousCuePresvLen(COMMAND_T*)
{
	DoItemCueTransform(false,-1,true);
}

void DoSwitchItemToFirstCue(COMMAND_T*)
{
	DoItemCueTransform(false,0);
}

void DoSwitchItemToRandomCue(COMMAND_T*)
{
	DoItemCueTransform(false,-2);
}

void DoSwitchItemToRandomCuePresvLen(COMMAND_T*)
{
	DoItemCueTransform(false,-2,true);
}

typedef struct
{
	MediaItem *ReaperItem;
	bool ItemSelected;
	int ActiveTake;
} t_ItemState;

vector <t_ItemState> g_VecItemStates;

void DoStoreSelectedTakes(COMMAND_T*)
{
	g_VecItemStates.clear();
	MediaItem *CurItem;
	MediaTrack *CurTrack;
	for (int i=0;i<GetNumTracks();i++)
	{

		CurTrack=CSurf_TrackFromID(i+1,false);
		for (int j=0;j<GetTrackNumMediaItems(CurTrack);j++)
		{
			CurItem=GetTrackMediaItem(CurTrack,j);
			bool IsSelected=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			//if (IsSelected==true)
			//{
				t_ItemState NewItemState;
				NewItemState.ItemSelected=IsSelected;
				NewItemState.ReaperItem=CurItem;
				int CurTakeIndex=*(int*)GetSetMediaItemInfo(CurItem,"I_CURTAKE",NULL);
				NewItemState.ActiveTake=CurTakeIndex;
				g_VecItemStates.push_back(NewItemState);
			//}
		}
	}

	//CurItem = GetTrackMediaItem(CurTrack,0);
	//CurTake = GetMediaItemTake(CurItem,-1);
}

void DoRecallSelectedTakes(COMMAND_T* ct)
{
	MediaItem *CurItem;
	MediaItem *CompareItem;
	MediaTrack *CurTrack;
	for (int i=0;i<(int)g_VecItemStates.size();i++)
	{
		int Matches=0;
		CurItem=g_VecItemStates[i].ReaperItem;
		for (int j=0;j<GetNumTracks();j++)
		{
			CurTrack=CSurf_TrackFromID(j+1,false);
			for (int k=0;k<GetTrackNumMediaItems(CurTrack);k++)
			{
				CompareItem=GetTrackMediaItem(CurTrack,k);
				if (CompareItem==CurItem)
					Matches++;
			}
		}
		if (Matches==0)
			g_VecItemStates.erase(g_VecItemStates.begin()+i);
	}

	PreventUIRefresh(1);
	int i;
	for (i=0;i<(int)g_VecItemStates.size();i++)
	{
		bool Dummybool=g_VecItemStates[i].ItemSelected;
		if (Dummybool==true)
		{
			int TakeIndex=g_VecItemStates[i].ActiveTake;
			CurItem=g_VecItemStates[i].ReaperItem;
			GetSetMediaItemInfo(CurItem,"B_UISEL",&Dummybool);
			GetSetMediaItemInfo(CurItem,"I_CURTAKE",&TakeIndex);
		}
	}
	PreventUIRefresh(-1);
	UpdateArrange();
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),4,-1);
}

void DoDeleteItemAndMedia(COMMAND_T*)
{
	MediaItem *CurItem;
	MediaItem_Take *CurTake;
	MediaTrack *CurTrack;
	Main_OnCommand(40100,0); // set all media offline
	for (int i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		for (int j=0;j<GetTrackNumMediaItems(CurTrack);j++)
		{
			CurItem=GetTrackMediaItem(CurTrack,j);
			bool IsSel=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (IsSel==true)
			{
				for (int k = 0; k < GetMediaItemNumTakes(CurItem); k++)
				{
					CurTake=GetMediaItemTake(CurItem,k);
					PCM_source *CurPCM;
					CurPCM=(PCM_source*)GetSetMediaItemTakeInfo(CurTake,"P_SOURCE",NULL);
					if (CurPCM && CurPCM->GetFileName() && FileExists(CurPCM->GetFileName()) && _stricmp(GetFileExtension(CurPCM->GetFileName()), "rpp"))
					{
						char buf[2000];
						snprintf(buf,sizeof(buf),__LOCALIZE_VERFMT("Do you really want to immediately delete file (NO UNDO) %s?","sws_mbox"),CurPCM->GetFileName());

						int rc=MessageBox(g_hwndParent,buf,__LOCALIZE("Xenakios - Info","sws_mbox"),MB_OKCANCEL);
						if (rc==IDOK)
						{
							DeleteFile(CurPCM->GetFileName());
							char fileName[512];
							strcpy(fileName, CurPCM->GetFileName());
							char* pEnd = fileName + strlen(fileName);
							strcpy(pEnd, ".reapeaks");
							DeleteFile(fileName);
							strcpy(pEnd, ".reapindex");
							DeleteFile(fileName);
						}
					}
				}
			}
		}
	}
	Main_OnCommand(40006,0); // remove selected items
	Main_OnCommand(40101,0); // set all media online
}

int SendFileToRecycleBin(const char *FileName)
{
#ifdef _WIN32
	int slen=(int)strlen(FileName);
	char *DoubleNullFN=new char[slen+3];
	memset(DoubleNullFN,0,slen+3);
	strcpy(DoubleNullFN,FileName);
	char* pC;
	while((pC = strchr(DoubleNullFN, '/')))
		*pC = '\\';
	SHFILEOPSTRUCT shOp;
	memset(&shOp,0,sizeof(SHFILEOPSTRUCT));
	shOp.wFunc=FO_DELETE;
	shOp.pFrom=DoubleNullFN;
	shOp.fFlags=FOF_FILESONLY|FOF_ALLOWUNDO|FOF_NOCONFIRMATION|FOF_NOERRORUI;
	int rc= SHFileOperation(&shOp);
	delete[] DoubleNullFN;
	return rc;
#else
	return 1;
#endif
}

void DoDelSelItemAndSendActiveTakeMediaToRecycler(COMMAND_T*)
{
	MediaItem *CurItem;
	MediaItem_Take *CurTake;
	MediaTrack *CurTrack;
	Main_OnCommand(40100,0); // set all media offline
	for (int i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		for (int j=0;j<GetTrackNumMediaItems(CurTrack);j++)
		{
			CurItem=GetTrackMediaItem(CurTrack,j);
			bool IsSel=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (IsSel==true)
			{
				for (int k=0; k < GetMediaItemNumTakes(CurItem); k++)
				{
					CurTake=GetMediaItemTake(CurItem,k);
					PCM_source* CurPCM = (PCM_source*)GetSetMediaItemTakeInfo(CurTake,"P_SOURCE",NULL);
					if (CurPCM && CurPCM->GetFileName() && FileExists(CurPCM->GetFileName()) && _stricmp(GetFileExtension(CurPCM->GetFileName()), "rpp"))
					{
						SendFileToRecycleBin(CurPCM->GetFileName());
						char fileName[512];
						strcpy(fileName, CurPCM->GetFileName());
						char* pEnd = fileName + strlen(fileName);
						strcpy(pEnd, ".reapeaks");
						SendFileToRecycleBin(fileName);
						strcpy(pEnd, ".reapindex");
						SendFileToRecycleBin(fileName);
					}
				}
			}
		}
	}
	Main_OnCommand(40006,0); // remove selected items
	Main_OnCommand(40101,0); // set all media online
}

void DoNukeTakeAndSourceMedia(COMMAND_T*)
{
	MediaItem *CurItem;
	MediaItem_Take *CurTake;
	MediaTrack *CurTrack;
	Main_OnCommand(40100,0); // set all media offline
	for (int i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		for (int j=0;j<GetTrackNumMediaItems(CurTrack);j++)
		{
			CurItem=GetTrackMediaItem(CurTrack,j);
			bool IsSel=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (IsSel==true)
			{
				CurTake=GetMediaItemTake(CurItem,-1);
				PCM_source *CurPCM;
				CurPCM=(PCM_source*)GetSetMediaItemTakeInfo(CurTake,"P_SOURCE",NULL);
				if (CurPCM && CurPCM->GetFileName() && FileExists(CurPCM->GetFileName()) && _stricmp(GetFileExtension(CurPCM->GetFileName()), "rpp"))
				{
					char buf[2000];
					snprintf(buf,sizeof(buf),__LOCALIZE_VERFMT("Do you really want to immediately delete file (NO UNDO) %s?","sws_mbox"),CurPCM->GetFileName());

					int rc=MessageBox(g_hwndParent,buf,__LOCALIZE("Xenakios - Info","sws_mbox"),MB_OKCANCEL);
					if (rc==IDOK)
					{
						DeleteFile(CurPCM->GetFileName());
						char fileName[512];
						strcpy(fileName, CurPCM->GetFileName());
						char* pEnd = fileName + strlen(fileName);
						strcpy(pEnd, ".reapeaks");
						DeleteFile(fileName);
						strcpy(pEnd, ".reapindex");
						DeleteFile(fileName);
					}
				}
			}
		}
	}
	Main_OnCommand(40129,0); // remove active takes of items
	Main_OnCommand(40101,0); // set all media online
}

void DoDeleteActiveTakeAndRecycleSourceMedia(COMMAND_T*)
{
	MediaItem *CurItem;
	MediaItem_Take *CurTake;
	MediaTrack *CurTrack;
	Main_OnCommand(40100,0); // set all media offline
	for (int i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		for (int j=0;j<GetTrackNumMediaItems(CurTrack);j++)
		{
			CurItem=GetTrackMediaItem(CurTrack,j);
			bool IsSel=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (IsSel==true)
			{
				CurTake=GetMediaItemTake(CurItem,-1);
				PCM_source *CurPCM=(PCM_source*)GetSetMediaItemTakeInfo(CurTake,"P_SOURCE",NULL);
				if (CurPCM && CurPCM->GetFileName() && FileExists(CurPCM->GetFileName()) && _stricmp(GetFileExtension(CurPCM->GetFileName()), "rpp"))
				{
					SendFileToRecycleBin(CurPCM->GetFileName());
					char fileName[512];
					strcpy(fileName, CurPCM->GetFileName());
					char* pEnd = fileName + strlen(fileName);
					strcpy(pEnd, ".reapeaks");
					SendFileToRecycleBin(fileName);
					strcpy(pEnd, ".reapindex");
					SendFileToRecycleBin(fileName);
				}
			}
		}
	}
	Main_OnCommand(40129,0); // remove active takes of items
	Main_OnCommand(40101,0); // set all media online
}

void DoSelectFirstItemInSelectedTrack(COMMAND_T*)
{
	MediaTrack *CurTrack;
	MediaItem *CurItem;

	PreventUIRefresh(1);
	Main_OnCommand(40289,0); // unselect all items
	int i;
	for (i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		int trackSel=*(int*)GetSetMediaTrackInfo(CurTrack,"I_SELECTED",NULL);
		if (trackSel==1)
		{
			int NumItems=GetTrackNumMediaItems(CurTrack);
			if (NumItems>0)
			{
				CurItem=GetTrackMediaItem(CurTrack,0);
				bool yeah=true;
				//if (CurItem!=0)
					GetSetMediaItemInfo(CurItem,"B_UISEL",&yeah);
			}
		}
	}
	PreventUIRefresh(-1);
	UpdateArrange();
}

struct t_PersistentTakeInfo
{
	GUID takeGUID;
	bool isOffline;
	double Volume;
};

struct t_PersistentItemInfo
{
	GUID itemGUID;
	bool isMuted;
	bool isSoloed;
};

void CycleItemsFadeShape(int whichfade, bool nextshape)
{
	MediaTrack *CurTrack;
	MediaItem *CurItem;
	int i;
	int j;
	for (i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		for (j=0;j<GetTrackNumMediaItems(CurTrack);j++)
		{
			CurItem=GetTrackMediaItem(CurTrack,j);
			bool IsSel=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (IsSel)
			{
				const char* cType = whichfade ? "C_FADEOUTSHAPE" : "C_FADEINSHAPE";
				char cShape = *(char*)GetSetMediaItemInfo(CurItem,cType,NULL) + (nextshape ? 1 : -1);
				if (cShape < 0)
					cShape = 6;
				else if (cShape > 6)
					cShape = 0;
				GetSetMediaItemInfo(CurItem,cType,&cShape);
			}
		}
	}
	Undo_OnStateChangeEx(__LOCALIZE("Cycle item fade shape","sws_undo"),UNDO_STATE_ITEMS,-1);
	UpdateTimeline();
}

void DoSetNextFadeInShape(COMMAND_T*)
{
	CycleItemsFadeShape(0,true);
}

void DoSetPrevFadeInShape(COMMAND_T*)
{
	CycleItemsFadeShape(0,false);
}

void DoSetNextFadeOutShape(COMMAND_T*)
{
	CycleItemsFadeShape(1,true);
}

void DoSetPrevFadeOutShape(COMMAND_T*)
{
	CycleItemsFadeShape(1,false);
}

void DoSetFadeToCrossfade(COMMAND_T* ct)
{
	const int cnt=CountSelectedMediaItems(NULL);
	for (int i = 0; i < cnt; i++)
	{
		MediaItem* item = GetSelectedMediaItem(0, i);
		double AutoFadeIn  = *(double*)GetSetMediaItemInfo(item, "D_FADEINLEN_AUTO",  NULL);
		double AutoFadeOut = *(double*)GetSetMediaItemInfo(item ,"D_FADEOUTLEN_AUTO", NULL);
		GetSetMediaItemInfo(item, "D_FADEINLEN",  &AutoFadeIn);
		GetSetMediaItemInfo(item, "D_FADEOUTLEN", &AutoFadeOut);
	}
	UpdateTimeline();
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),UNDO_STATE_ITEMS,-1);
}

void DoSetFadeToDefaultFade(COMMAND_T* ct)
{
	double* pdDefFade = ConfigVar<double>("deffadelen").get();
	double dZero = 0.0;
	const int cnt=CountSelectedMediaItems(NULL);
	for (int i = 0; i < cnt; i++)
	{
		MediaItem* item = GetSelectedMediaItem(0, i);
		GetSetMediaItemInfo(item, "D_FADEINLEN_AUTO",  &dZero);
		GetSetMediaItemInfo(item, "D_FADEOUTLEN_AUTO", &dZero);
		GetSetMediaItemInfo(item, "D_FADEINLEN",  pdDefFade);
		GetSetMediaItemInfo(item, "D_FADEOUTLEN", pdDefFade);
	}
	UpdateTimeline();
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),UNDO_STATE_ITEMS,-1);
}

void DoItemPitch2Playrate(COMMAND_T* ct)
{
	MediaItem *CurItem;
	MediaItem_Take *CurTake;
	MediaTrack *CurTrack;
	int i;
	int j;
	for (i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		for (j=0;j<GetTrackNumMediaItems(CurTrack);j++)
		{
			CurItem=GetTrackMediaItem(CurTrack,j);
			bool isSel=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (isSel)
			{
				CurTake=GetMediaItemTake(CurItem,-1);
				if (CurTake)
				{
					double CurPitch=*(double*)GetSetMediaItemTakeInfo(CurTake,"D_PITCH",NULL);
					double NewPitch=0.0;
					GetSetMediaItemTakeInfo(CurTake,"D_PITCH",&NewPitch);
					bool NewPreserve=false;
					GetSetMediaItemTakeInfo(CurTake,"B_PPITCH",&NewPreserve);
					double NewRate=pow(2.0,CurPitch/12.0);
					GetSetMediaItemTakeInfo(CurTake,"D_PLAYRATE",&NewRate);
					double ItemLen=*(double*)GetSetMediaItemInfo(CurItem,"D_LENGTH",NULL);
					ItemLen=ItemLen*(1.0/NewRate);
					GetSetMediaItemInfo(CurItem,"D_LENGTH",&ItemLen);
				}
			}
		}
	}
	UpdateTimeline();
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),UNDO_STATE_ITEMS,-1);
}

void DoItemPlayrate2Pitch(COMMAND_T* ct)
{
	MediaItem *CurItem;
	MediaItem_Take *CurTake;
	MediaTrack *CurTrack;
	int i;
	int j;
	for (i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		for (j=0;j<GetTrackNumMediaItems(CurTrack);j++)
		{
			CurItem=GetTrackMediaItem(CurTrack,j);
			bool isSel=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (isSel)
			{
				CurTake=GetMediaItemTake(CurItem,-1);
				if (CurTake)
				{
					double OldRate=*(double*)GetSetMediaItemTakeInfo(CurTake,"D_PLAYRATE",NULL);
					double NewRate=1.0;
					GetSetMediaItemTakeInfo(CurTake,"D_PLAYRATE",&NewRate);
					// 12.0*log(OldPlayRate)/log(2.0);
					double NewPitch=12.0*log(OldRate)/log(2.0);
					GetSetMediaItemTakeInfo(CurTake,"D_PITCH",&NewPitch);
					double ItemLen=*(double*)GetSetMediaItemInfo(CurItem,"D_LENGTH",NULL);
					ItemLen=ItemLen*(OldRate);
					GetSetMediaItemInfo(CurItem,"D_LENGTH",&ItemLen);
					/*
					double CurPitch=*(double*)GetSetMediaItemTakeInfo(CurTake,"D_PITCH",NULL);
					double NewPitch=0.0;
					GetSetMediaItemTakeInfo(CurTake,"D_PITCH",&NewPitch);
					bool NewPreserve=false;
					GetSetMediaItemTakeInfo(CurTake,"B_PPITCH",&NewPreserve);
					double NewRate=pow(2.0,CurPitch/12.0);
					GetSetMediaItemTakeInfo(CurTake,"D_PLAYRATE",&NewRate);
					double ItemLen=*(double*)GetSetMediaItemInfo(CurItem,"D_LENGTH",NULL);
					ItemLen=ItemLen*(1.0/NewRate);
					GetSetMediaItemInfo(CurItem,"D_LENGTH",&ItemLen);
					*/
				}
			}
		}
	}
	UpdateTimeline();
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),UNDO_STATE_ITEMS,-1);
}


void DoSpreadItemsOverTracks(int NumTracks, int StartTrack, int Mode)
{
	vector<MediaItem*> VecSelItems;
	MediaTrack *CurTrack;
	MediaItem *CurItem;
	VecSelItems.clear();
	int i;
	int j;
	int IndexOfFirstSelTrack=-1;
	for (i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		for (j=0;j<GetTrackNumMediaItems(CurTrack);j++)
		{
			CurItem=GetTrackMediaItem(CurTrack,j);
			if (CurItem!=0)
			{
				bool isSel=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
				if (isSel)
				{
					VecSelItems.push_back(CurItem);
					if (IndexOfFirstSelTrack==-1) IndexOfFirstSelTrack=i+1;
				}
			}
		}
	}
	int TrackOffset=StartTrack;
	int TargetTrackID;
	if (Mode==0)
	{
		for (i=0;i<(int)VecSelItems.size();i++)
		{
			CurTrack=(MediaTrack*)GetSetMediaItemInfo(VecSelItems[i],"P_TRACK",NULL);
			TargetTrackID=CSurf_TrackToID(CurTrack,false)+TrackOffset;
			CurTrack=CSurf_TrackFromID(TargetTrackID,false);
			MoveMediaItemToTrack(VecSelItems[i],CurTrack);
			TrackOffset++;
			if (TrackOffset==NumTracks) TrackOffset=0;
		}
	}
	if (Mode==1)
	{
		for (i=0;i<(int)VecSelItems.size();i++)
		{
			TargetTrackID=(rand() % NumTracks)+IndexOfFirstSelTrack;
			CurTrack=CSurf_TrackFromID(TargetTrackID,false);
			MoveMediaItemToTrack(VecSelItems[i],CurTrack);

		}
	}
	UpdateTimeline();
	Undo_OnStateChangeEx(__LOCALIZE("Spread items over tracks","sws_undo"),UNDO_STATE_ITEMS,-1);
}

void DoSpreadSelItemsOver4Tracks(COMMAND_T*)
{
	DoSpreadItemsOverTracks(4,0,0);
}

WDL_DLGRET ItemSpreadDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, Message, wParam, lParam))
		return r;

	switch(Message)
    {
        case WM_INITDIALOG:
			SetDlgItemText(hwnd,IDC_EDIT1,"4");
			SetDlgItemText(hwnd,IDC_EDIT2,"1");
			return 0;
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDCANCEL:
					EndDialog(hwnd,0);
					return 0;
				case IDOK:
				{
					char buf[50];
					GetDlgItemText(hwnd,IDC_EDIT1,buf,49);
					int NumTracks=atoi(buf);
					GetDlgItemText(hwnd,IDC_EDIT2,buf,49);
					int StartTrack=atoi(buf);
					if (StartTrack<1) StartTrack=1;
					if (StartTrack>NumTracks) StartTrack=NumTracks;
					int butstat= IsDlgButtonChecked(hwnd,IDC_CHECK1);
					if (butstat==BST_CHECKED)
						DoSpreadItemsOverTracks(NumTracks,StartTrack-1,1);
					else DoSpreadItemsOverTracks(NumTracks,StartTrack-1,0);
					EndDialog(hwnd,0);
					return 0;
				}
			}
			break;
	}
	return 0;
}

void DoShowSpreadItemsDlg(COMMAND_T*)
{
	DialogBox(g_hInst,MAKEINTRESOURCE(IDD_SPREADITEMS), g_hwndParent, ItemSpreadDlgProc);
}

struct t_item_solostate
{
	bool isMuted;
	bool isSoloed;
	GUID guid;
} ;

double g_itemseltogprob=0.5;
vector<MediaItem*> g_vectogSelItems;
void DoTogSelItemsRandomly(bool isrestore,double togprob)
{
	for (int i=0;i<(int)g_vectogSelItems.size();i++)
	{
		bool uisel=false;

		if (isrestore) uisel=true;
		if (!isrestore)
		{
			double rando = g_mtrand.rand();
			if (rando<togprob) uisel=true;
		}
		GetSetMediaItemInfo(g_vectogSelItems[i],"B_UISEL",&uisel);
	}
}

WDL_DLGRET TogItemsSelDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static bool Applied=false;
	char tbuf[200];
	static stringstream dlgSS;

	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, Message, wParam, lParam))
		return r;

	if (Message==WM_INITDIALOG)
	{
		dlgSS.str("");
		Applied=false;
		g_vectogSelItems.clear();
		XenGetProjectItems(g_vectogSelItems,true,true);
		dlgSS << g_itemseltogprob*100.0;
		//sprintf(tbuf,"%.1f",g_itemseltogprob*100.0);
		SetDlgItemText(hwnd,IDC_EDIT1,dlgSS.str().c_str());
		SetFocus(GetDlgItem(hwnd, IDC_EDIT1));
		SendMessage(GetDlgItem(hwnd, IDC_EDIT1), EM_SETSEL, 0, -1);
		return 0;
	}
	if (Message==WM_COMMAND && LOWORD(wParam)==IDCANCEL)
	{
		DoTogSelItemsRandomly(true,0.0);
		UpdateTimeline();
		EndDialog(hwnd,0);
		return 0;
	}
	if (Message==WM_COMMAND && LOWORD(wParam)==IDC_BUTTON1)
	{
		GetDlgItemText(hwnd, IDC_EDIT1, tbuf, 199);
		g_itemseltogprob=atof(tbuf)/100.0;
		if (g_itemseltogprob<0.0) g_itemseltogprob=0.0;
		if (g_itemseltogprob>1.0) g_itemseltogprob=1.0;
		DoTogSelItemsRandomly(false, g_itemseltogprob);
		UpdateTimeline();
		Applied=true;
	}
	if (Message==WM_COMMAND && LOWORD(wParam)==IDOK)
	{
		if (!Applied)
		{
			GetDlgItemText(hwnd,IDC_EDIT1,tbuf,199);
			g_itemseltogprob=atof(tbuf)/100.0;
			if (g_itemseltogprob<0.0) g_itemseltogprob=0.0;
			if (g_itemseltogprob>1.0) g_itemseltogprob=1.0;
			DoTogSelItemsRandomly(false,g_itemseltogprob);
			UpdateTimeline();
		}
		EndDialog(hwnd,0);
		return 0;
	}
	return 0;
}

void DoToggleSelectedItemsRndDlg(COMMAND_T*)
{
	DialogBox(g_hInst,MAKEINTRESOURCE(IDD_TOGSELRAND), g_hwndParent, TogItemsSelDlgProc);
}

void SlipItemKeyUpHandler()
{
	Undo_OnStateChangeEx(__LOCALIZE("Nudge item contents (undo test)","sws_undo"),UNDO_STATE_ITEMS,-1);
	g_KeyUpUndoHandler=0;
}

accelerator_register_t g_myundoacreg;

int MyUndoAccelTranslateFunc(MSG *msg, accelerator_register_t *ctx)
{
	if (msg->message==WM_KEYUP)
	{
		if (g_KeyUpUndoHandler)
			g_KeyUpUndoHandler();
	}
	return 0;
}


void InitUndoKeyUpHandler01()
{
	g_myundoacreg.isLocal=TRUE;
	g_myundoacreg.translateAccel=MyUndoAccelTranslateFunc;
	g_myundoacreg.user=g_hwndParent;
	plugin_register("accelerator",&g_myundoacreg);
}

void RemoveUndoKeyUpHandler01()
{
	plugin_register("-accelerator",&g_myundoacreg);
}

void DoSlipItemContents(int timebase,double amount,bool slipalltakes)
{
	if (!g_KeyUpUndoHandler) g_KeyUpUndoHandler=SlipItemKeyUpHandler;
	vector<MediaItem_Take*> thetakes;
	XenGetProjectTakes(thetakes,!slipalltakes,true);
	int i;
	for (i=0;i<(int)thetakes.size();i++)
	{
		double mediaoffset=*(double*)GetSetMediaItemTakeInfo(thetakes[i],"D_STARTOFFS",0);
		PCM_source *src=(PCM_source*)GetSetMediaItemTakeInfo(thetakes[i],"P_SOURCE",0);
		double srate=44100.0;
		if (src) srate=src->GetSampleRate();
		double newoffset=mediaoffset+(1.0/srate);
		if (amount<0) newoffset=mediaoffset-(1.0/srate);
		GetSetMediaItemTakeInfo(thetakes[i],"D_STARTOFFS",&newoffset);
	}
	UpdateTimeline();
}

void DoSlipItemContentsOneSampleLeft(COMMAND_T*)
{
	DoSlipItemContents(0,-1,false);
}

void DoSlipItemContentsOneSampleRight(COMMAND_T*)
{
	DoSlipItemContents(0,1,false);
}

void ReplaceItemSourceFileFromFolder(bool askforFolder,int mode,int param,bool onlyRPP=false)
{
	vector<MediaItem_Take*> thetakes;
	XenGetProjectTakes(thetakes,true,true);
	int i;
	vector<string> fncompns;
	vector<string> foundfiles;
	for (i=0;i<(int)thetakes.size();i++)
	{
		PCM_source* src=(PCM_source*)GetSetMediaItemTakeInfo(thetakes[i],"P_SOURCE",0);
		if (!src || !src->GetFileName())
			break;

		string newsrcfn;
		// get source file's directory, find it's media files...
		string srcfn(src->GetFileName());

		fncompns.clear();
		SplitFileNameComponents(srcfn,fncompns);
		foundfiles.clear();
		SearchDirectory(foundfiles, fncompns[0].c_str(), onlyRPP ? "RPP" : NULL, false);
		if (mode==0) // sequential mode
			std::sort(foundfiles.begin(), foundfiles.end());
		int j;
		// find index take's source file in the found files
		int fileindx=-1;
		for (j=0;j<(int)foundfiles.size();j++)
		{
			string temps;

			temps.assign(RemoveDoubleBackSlashes(foundfiles[j]));
			foundfiles[j].assign(temps);
			if (foundfiles[j].compare(srcfn)==0)
			{
				fileindx=j;
				//break;
			}
		}
		if (fileindx>=0)
		{
			int newfileindx=fileindx;
			if (mode==0 && param==1)
				newfileindx=fileindx+1;
			if (mode==0 && param==-1)
				newfileindx=fileindx-1;
			if (mode==1)
				newfileindx=rand() % foundfiles.size();
			if (newfileindx<0) newfileindx=(int)foundfiles.size()-1;
			if (newfileindx>(int)foundfiles.size()-1) newfileindx=0;
			newsrcfn.assign(foundfiles[newfileindx]);
		}
		PCM_source *newsrc=PCM_Source_CreateFromFile(newsrcfn.c_str());
		if (newsrc)
		{
			GetSetMediaItemTakeInfo(thetakes[i],"P_SOURCE",newsrc);
			fncompns.clear();
			SplitFileNameComponents(newsrcfn,fncompns);
			GetSetMediaItemTakeInfo(thetakes[i],"P_NAME",(char*)fncompns[1].c_str());
			delete src;
		}
	}
	UpdateTimeline();
	Main_OnCommand(40047,0); // build missing peaks
	Undo_OnStateChangeEx(__LOCALIZE("Change source files of items","sws_undo"),UNDO_STATE_ITEMS,-1);
}

void DoReplaceItemFileWithNextInFolder(COMMAND_T*)
{
	ReplaceItemSourceFileFromFolder(false,0,1); // don't ask folder, mode next/prev, 1=next
}

void DoReplaceItemFileWithPrevInFolder(COMMAND_T*)
{
	ReplaceItemSourceFileFromFolder(false,0,-1); // don't ask folder mode next/prev, -1 prev file in folder
}

void DoReplaceItemFileWithRandomInFolder(COMMAND_T*)
{
	ReplaceItemSourceFileFromFolder(false,1,-666); // random file
}

void DoReplaceItemFileWithNextRPPInFolder(COMMAND_T*)
{
	ReplaceItemSourceFileFromFolder(false,0,1,true); // don't ask folder, mode next/prev, 1=next, only look for rpp files
}

void DoReplaceItemFileWithPrevRPPInFolder(COMMAND_T*)
{
	ReplaceItemSourceFileFromFolder(false,0,-1,true); // don't ask folder mode next/prev, -1 prev file in folder, only look for rpp files
}

void DoReverseItemOrder(COMMAND_T* ct)
{
	vector<MediaItem*> theitems;
	XenGetProjectItems(theitems,true,true);
	vector<double> origtimes;
	int i;
	for (i=0;i<(int)theitems.size();i++)
	{
		double itempos=*(double*)GetSetMediaItemInfo(theitems[i],"D_POSITION",0);
		origtimes.push_back(itempos);
	}
	for (i=0;i<(int)theitems.size();i++)
	{
		double itempos=origtimes[origtimes.size()-i-1];
		GetSetMediaItemInfo(theitems[i],"D_POSITION",&itempos);
	}
	UpdateTimeline();
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),UNDO_STATE_ITEMS,-1);
}

typedef struct t_itemorderstruct
{
	MediaItem* pitem;
	double deltatimepos;
	double timepos;
	double itemlen;
} t_itemorderstruct;

void DoShuffleItemOrder(COMMAND_T* ct)
{
	vector<MediaItem*> theitems;
	XenGetProjectItems(theitems,true,true);
	vector<double> origtimes;
	int i;
	vector<t_itemorderstruct> vec_yeah;
	for (i=0;i<(int)theitems.size();i++)
	{
		double itempos=*(double*)GetSetMediaItemInfo(theitems[i],"D_POSITION",0);
		origtimes.push_back(itempos);
	}
	MediaItem* pItem=0;
	i=0;
	while (theitems.size()>0)
	{
		int indx=rand() % theitems.size();
		pItem=theitems[indx];
		double itempos=origtimes[i];
		GetSetMediaItemInfo(pItem,"D_POSITION",&itempos);
		theitems.erase(theitems.begin()+indx);
		i++;
	}
	UpdateTimeline();
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),UNDO_STATE_ITEMS,-1);
}

void DoShuffleItemOrder2(COMMAND_T* ct)
{
	vector<MediaItem*> theitems;
	XenGetProjectItems(theitems,true,true);
	if (theitems.size()>0)
	{
		vector<double> origtimes;
		int i;
		double prevtime=*(double*)GetSetMediaItemInfo(theitems[0],"D_POSITION",0);;
		vector<t_itemorderstruct> vec_yeah;
		t_itemorderstruct uh;
		for (i=0;i<(int)theitems.size();i++)
		{
			double itempos=*(double*)GetSetMediaItemInfo(theitems[i],"D_POSITION",0);
			uh.timepos=itempos;
			uh.itemlen=*(double*)GetSetMediaItemInfo(theitems[i],"D_LENGTH",0);
			double deltatime=(itempos+uh.itemlen)-prevtime;
			uh.deltatimepos=deltatime;
			uh.pitem=theitems[i];
			vec_yeah.push_back(uh);
			prevtime=itempos+uh.itemlen;
		}
		MediaItem* pItem=0;
		i=0;
		double itempos=vec_yeah[0].timepos;
		while (vec_yeah.size()>0)
		{
			int indx=rand() % vec_yeah.size();
			pItem=vec_yeah[indx].pitem;
			GetSetMediaItemInfo(pItem,"D_POSITION",&itempos);
			itempos+=vec_yeah[indx].deltatimepos;
			vec_yeah.erase(vec_yeah.begin()+indx);
			i++;
		}
		UpdateTimeline();
		Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),UNDO_STATE_ITEMS,-1);
	}
}

bool MySortItemsByTimeFunc (MediaItem* i,MediaItem* j)
{
	double timea=*(double*)GetSetMediaItemInfo(i,"D_POSITION",0);
	double timeb=*(double*)GetSetMediaItemInfo(j,"D_POSITION",0);
	return (timea<timeb);
}

void DoCreateMarkersFromSelItems1(COMMAND_T* ct)
{
	vector<MediaItem*> theitems;
	XenGetProjectItems(theitems,true,true);
	if (theitems.size()>0)
	{
		sort(theitems.begin(),theitems.end(),MySortItemsByTimeFunc);

		PreventUIRefresh(1);

		Undo_BeginBlock2(NULL);

		for (int i=0;i<(int)theitems.size();i++)
		{
			double itempos=*(double*)GetSetMediaItemInfo(theitems[i],"D_POSITION",0);

			vector<string> fncomps;
			if (MediaItem_Take* ptake = GetMediaItemTake(theitems[i], -1))
				if (PCM_source* src = (PCM_source*)GetSetMediaItemTakeInfo(ptake,"P_SOURCE",0))
					if (src->GetFileName())
						SplitFileNameComponents(src->GetFileName(),fncomps);
			AddProjectMarker(NULL, false, itempos, 0.0, fncomps.size()>1 ? fncomps[1].c_str() : "", -1);
		}

		PreventUIRefresh(-1);
		UpdateTimeline();

		Undo_EndBlock2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_MISCCFG);    
	}
}

void DoSaveItemAsFile1(COMMAND_T*)
{
	vector<MediaItem*> theitems;
	XenGetProjectItems(theitems,true,false);
	if (theitems.size()>0)
	{
		Main_OnCommand(40601,0); // render items as new takes
		char ppath[MAX_PATH];
		char savedlgtitle[2048];
		char newfilename[512];
		GetProjectPath(ppath,2048);
		int i;
		for (i=0;i<(int)theitems.size();i++)
		{
			MediaItem_Take* ptake=GetMediaItemTake(theitems[i],-1);
			PCM_source *src=(PCM_source*)GetSetMediaItemTakeInfo(ptake,"P_SOURCE",0);
			if (src && src->GetFileName())
			{
				snprintf(savedlgtitle, 2048, __LOCALIZE_VERFMT("Save item \"%s\" as","sws_mbox"), (char*)GetSetMediaItemTakeInfo(ptake, "P_NAME", 0));
				if (BrowseForSaveFile(savedlgtitle, ppath, NULL, "WAV files\0*.wav\0", newfilename, 512))
				{
					Main_OnCommand(40440,0); // set selected media offline
					MoveFile(src->GetFileName(), newfilename);
				}
			}
		}
		Main_OnCommand(40129,0); // delete active takes of items
		Main_OnCommand(40439,0); // set selected media online
		Main_OnCommand(40029,0); // undo ??
		Main_OnCommand(40029,0); // undo
	}
}

void DoSelectItemUnderEditCursorOnSelTrack(COMMAND_T* _ct)
{
	//XenGetProjectTracks(txs,true);
	vector<MediaItem*> theitems;
	XenGetProjectItems(theitems,false,true);

	PreventUIRefresh(1);
	Main_OnCommand(40289,0); // unselect all items
	int i;
	double curpos=GetCursorPosition();
	for (i=0;i<(int)theitems.size();i++)
	{
		MediaTrack* comptrack=(MediaTrack*)GetSetMediaItemInfo(theitems[i],"P_TRACK",0);
		int issel=*(int*)GetSetMediaTrackInfo(comptrack,"I_SELECTED",NULL);
		if (issel)
		{
			double itemstart=*(double*)GetSetMediaItemInfo(theitems[i],"D_POSITION",0);
			double itemlen=*(double*)GetSetMediaItemInfo(theitems[i],"D_LENGTH",0);
			double itemend=itemstart+itemlen;
			if (curpos>=itemstart && curpos <=itemend)
			{
				bool blah=true;
				GetSetMediaItemInfo(theitems[i],"B_UISEL",&blah);
			}
		}
	}
	PreventUIRefresh(-1);
	UpdateArrange();
	Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}

void DoNormalizeSelTakesTodB(COMMAND_T* ct)
{
	vector<MediaItem_Take*> seltakes;
	XenGetProjectTakes(seltakes,true,true);
	if (seltakes.size()>0)
	{
		char buf[100];
		strcpy(buf,"0.00");
		if (XenSingleStringQueryDlg(g_hwndParent,__LOCALIZE("Normalize items to dB value","sws_mbox"),buf,100)==0)
		{
			double relgain=atof(buf);
			Undo_BeginBlock();
			Main_OnCommand(40108,0); // Normalize items
			int i;
			for (i=0;i<(int)seltakes.size();i++)
			{
				MediaItem_Take* take = seltakes[i];
				double curgain=abs(*(double*)GetSetMediaItemTakeInfo(take,"D_VOL",0));
				double newdb=VAL2DB(curgain)+relgain;
				double newgain=IsTakePolarityFlipped(take) ? -DB2VAL(newdb) : DB2VAL(newdb);
				GetSetMediaItemTakeInfo(take,"D_VOL",&newgain);
			}
			Undo_EndBlock(SWS_CMD_SHORTNAME(ct),0);
			UpdateTimeline();
		}
	}
}

void DoResetItemsOffsetAndLength(COMMAND_T* ct)
{
	vector<MediaItem_Take*> thetakes;
	XenGetProjectTakes(thetakes,true,true);
	int i;
	for (i=0;i<(int)thetakes.size();i++)
	{
		PCM_source *src=(PCM_source*)GetSetMediaItemTakeInfo(thetakes[i],"P_SOURCE",0);
		if (src)
		{
			MediaItem *pit=(MediaItem*)GetSetMediaItemTakeInfo(thetakes[i],"P_ITEM",0);
			if (pit)
			{
				double takeplayrate=*(double*)GetSetMediaItemTakeInfo(thetakes[i],"D_PLAYRATE",0);
				double newitemlen=src->GetLength()*(1.0/takeplayrate);
				double newsrcoffs=0.0;
				GetSetMediaItemInfo(pit,"D_LENGTH",&newitemlen);
				GetSetMediaItemTakeInfo(thetakes[i],"D_STARTOFFS",&newsrcoffs);
			}
		}
	}
	UpdateTimeline();
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),UNDO_STATE_ITEMS,-1);
}

void SetSelItemsFadesToConf(const char *confID)
{
	vector<MediaItem*> selitems;
	XenGetProjectItems(selitems, true, false);
	char resultString[512];
	GetPrivateProfileString("XENAKIOSCOMMANDS",confID,"0.005 1 0.005 1",resultString,512,g_XenIniFilename.Get());
	LineParser lp(false);
	lp.parse(resultString);
	double fadeinlen    = lp.gettoken_float(0);
	char   fadeinshape  = lp.gettoken_int(1);
	double fadeoutlen   = lp.gettoken_float(2);
	char   fadeoutshape = lp.gettoken_int(3);
	double dZero = 0.0;
	for (int i = 0; i < (int)selitems.size(); i++)
	{
		GetSetMediaItemInfo(selitems[i], "D_FADEINLEN",  &fadeinlen);
		GetSetMediaItemInfo(selitems[i], "D_FADEOUTLEN", &fadeoutlen);
		GetSetMediaItemInfo(selitems[i], "D_FADEINLEN_AUTO",  &dZero);
		GetSetMediaItemInfo(selitems[i], "D_FADEOUTLEN_AUTO", &dZero);
		GetSetMediaItemInfo(selitems[i], "C_FADEINSHAPE",  &fadeinshape);
		GetSetMediaItemInfo(selitems[i], "C_FADEOUTSHAPE", &fadeoutshape);
	}
	UpdateTimeline();
	Undo_OnStateChangeEx(__LOCALIZE("Set item fades","sws_undo"),UNDO_STATE_ITEMS,-1);
}

void DoFadesOfSelItemsToConfC(COMMAND_T*)
{
	SetSelItemsFadesToConf("FADECONF_C");
}

void DoFadesOfSelItemsToConfD(COMMAND_T*)
{
	SetSelItemsFadesToConf("FADECONF_D");
}

void DoFadesOfSelItemsToConfE(COMMAND_T*)
{
	SetSelItemsFadesToConf("FADECONF_E");
}

void DoFadesOfSelItemsToConfF(COMMAND_T*)
{
	SetSelItemsFadesToConf("FADECONF_F");
}
