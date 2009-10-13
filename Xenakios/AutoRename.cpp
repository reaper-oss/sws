/******************************************************************************
/ AutoRename.cpp
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

using namespace std;

string g_FormattingString;
HWND g_hAutoNameDlg;

vector<MediaItem_Take*> g_VecTakesToRename;
vector<MediaItem_Take*> g_VecTakesToRenameTimeOrdered;

void StripFileExtensionFromString(string& TheString)
{
	string::size_type pos = TheString.find_last_of('.');
	if (pos != string::npos)
		TheString.erase(pos);
}

int GetTrackIDofTake(MediaItem_Take *TheTake)
{
	int i;
	MediaTrack *CurTrack;
	MediaTrack *CompaTrack;
	CompaTrack=(MediaTrack*)GetSetMediaItemTakeInfo(TheTake,"P_TRACK",NULL);
	for (i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		if (CurTrack==CompaTrack)
			return i+1;
	}
	return -1;
}

string GetNewTakeNameFromFormatString(const char *InFormatString,MediaItem_Take *TheTake,int TakeNumber)
{
	vector<string> FormatTokens;
	TokenizeString(InFormatString,FormatTokens,"%");
	
	string NewTakeName;
	char* TrackName = NULL;
	char* TakeName = NULL;
	char* FolderTrackName = NULL;
	char TakeGUID[64];
	MediaItem *CurItem;
	MediaTrack *CurTrack;
	MediaTrack *CurFolderTrack;
	if (TheTake!=0)
	{
		CurItem=(MediaItem*)GetSetMediaItemTakeInfo(TheTake,"P_ITEM",NULL);
		CurTrack=(MediaTrack*)GetSetMediaItemInfo(CurItem,"P_TRACK",NULL);
		TrackName=(char*)GetSetMediaTrackInfo(CurTrack,"P_NAME",NULL);
		TakeName=(char*)GetSetMediaItemTakeInfo(TheTake,"P_NAME",NULL);
		CurFolderTrack=(MediaTrack*)GetSetMediaTrackInfo(CurTrack,"P_PARTRACK",NULL);
		if (CurFolderTrack)
			FolderTrackName = (char*)GetSetMediaTrackInfo(CurFolderTrack,"P_NAME",NULL);
		GUID ptakeguid=*(GUID*)GetSetMediaItemTakeInfo(TheTake,"GUID",NULL);
		guidToString(&ptakeguid,TakeGUID);
	}
	bool StripFileExtension=false;
	int j;
	for (j=0;j<(int)FormatTokens.size();j++)
	{
		bool NotFormatToken=true;
		
		if (FormatTokens[j].compare("[NOEXT]")==0)
		{
			StripFileExtension=true;
			NotFormatToken=false;
		}
		if (FormatTokens[j].compare("[GUID]")==0)
		{
			NewTakeName.append(TakeGUID);
			NotFormatToken=false;
		}
		if (FormatTokens[j].compare("[tracknum]")==0)
		{
			char nt[64];
			sprintf(nt,"%.2d",GetTrackIDofTake(TheTake));
			NewTakeName.append(nt);
			NotFormatToken=false;
		}
		if (FormatTokens[j].compare("[foldername]")==0)
		{
			if (FolderTrackName)
				NewTakeName.append(FolderTrackName);
			NotFormatToken=false;
		}
		if (FormatTokens[j].compare("[trackname]")==0)
		{
			if (TrackName)
				NewTakeName.append(TrackName);
			NotFormatToken=false;
		}
		if (FormatTokens[j].compare("[takename]")==0)
		{
			if (TakeName)
				NewTakeName.append(TakeName);
			NotFormatToken=false;
		}
		if (FormatTokens[j].compare("[inctrackorder]")==0)
		{
			char tn[8];
			sprintf(tn,"%.2d",TakeNumber);
			NewTakeName.append(tn);
			NotFormatToken=false;
		}
		if (FormatTokens[j].compare("[inctimeorder]")==0)
		{
			char tn[8];
			sprintf(tn,"%.2d",TakeNumber);
			NewTakeName.append(tn);
			NotFormatToken=false;
		}
		if (NotFormatToken)
			NewTakeName.append(FormatTokens[j]);
	}
	if (StripFileExtension)
		StripFileExtensionFromString(NewTakeName);
	return NewTakeName;
}

bool g_AutonameCancelled=false;

void UpdateSampleNameList()
{
	vector<string> FormatTokens;
	TokenizeString(g_FormattingString,FormatTokens,"%");
	bool UseTimeSortedTakes=false;
	int x;
	for (x=0;x<(int)FormatTokens.size();x++)
		if (FormatTokens[x].compare("[inctimeorder]")==0)
			UseTimeSortedTakes=true;
	ListView_DeleteAllItems(GetDlgItem(g_hAutoNameDlg,IDC_AUTONAMEOUTPUT));
	int TrackItemCounter=0;
	MediaTrack *CurTrack;
	MediaTrack *PrevTrack=0;
	MediaItem *CurItem;
	if (!UseTimeSortedTakes)
	{
		int i;
		for (i=0;i<(int)g_VecTakesToRename.size();i++)
		{
			CurItem=(MediaItem*)GetSetMediaItemTakeInfo(g_VecTakesToRename[i],"P_ITEM",NULL);
			CurTrack=(MediaTrack*)GetSetMediaItemInfo(CurItem,"P_TRACK",NULL);
			if (CurTrack!=PrevTrack) 
			{
				TrackItemCounter=0;
				PrevTrack=CurTrack;
			}
			LVITEM item;
			item.mask=LVIF_TEXT;
			item.iItem=i;
			item.iSubItem = 0;
			item.pszText=(char*)GetSetMediaItemTakeInfo(g_VecTakesToRename[i],"P_NAME",NULL);
			ListView_InsertItem(GetDlgItem(g_hAutoNameDlg,IDC_AUTONAMEOUTPUT),&item);
			string SampleName=GetNewTakeNameFromFormatString(g_FormattingString.c_str(),g_VecTakesToRename[i],TrackItemCounter+1);
			ListView_SetItemText(GetDlgItem(g_hAutoNameDlg,IDC_AUTONAMEOUTPUT),i,1,(LPSTR)SampleName.c_str());
			TrackItemCounter++;
		}
	}
	else
	{
		int i;
		for (i=0;i<(int)g_VecTakesToRenameTimeOrdered.size();i++)
		{
			
			CurItem=(MediaItem*)GetSetMediaItemTakeInfo(g_VecTakesToRenameTimeOrdered[i],"P_ITEM",NULL);
			CurTrack=(MediaTrack*)GetSetMediaItemInfo(CurItem,"P_TRACK",NULL);
			
			LVITEM item;
			item.mask=LVIF_TEXT;
			item.iItem=i;
			item.iSubItem = 0;
			item.pszText=(char*)GetSetMediaItemTakeInfo(g_VecTakesToRenameTimeOrdered[i],"P_NAME",NULL);
			ListView_InsertItem(GetDlgItem(g_hAutoNameDlg,IDC_AUTONAMEOUTPUT),&item);
			string SampleName=GetNewTakeNameFromFormatString(g_FormattingString.c_str(),g_VecTakesToRenameTimeOrdered[i],TrackItemCounter+1);
			ListView_SetItemText(GetDlgItem(g_hAutoNameDlg,IDC_AUTONAMEOUTPUT),i,1,(LPSTR)SampleName.c_str());
			TrackItemCounter++;	
		}
	}
}

typedef struct 
{
	string Description;
	string FormattingString;
} t_autorenamepreset;

vector<t_autorenamepreset> g_AutoNamePresets;

WDL_DLGRET AutoRenameDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
    {
        case WM_INITDIALOG:
			{
				g_AutoNamePresets.clear();
				t_autorenamepreset newAutonamePreset;
				newAutonamePreset.Description.assign("Increasing suffix, track-by-track");
				newAutonamePreset.FormattingString.assign("[takename]%-%[inctrackorder]");
				g_AutoNamePresets.push_back(newAutonamePreset);
				newAutonamePreset.Description.assign("Increasing suffix, time order across all tracks");
				newAutonamePreset.FormattingString.assign("[takename]%-%[inctimeorder]");
				g_AutoNamePresets.push_back(newAutonamePreset);
				newAutonamePreset.Description.assign("Folder.Track.Increasing suffix");
				newAutonamePreset.FormattingString.assign("[foldername]%.%[trackname]%.%[inctrackorder]");
				g_AutoNamePresets.push_back(newAutonamePreset);
				newAutonamePreset.Description.assign("Strip file extension");
				newAutonamePreset.FormattingString.assign("[takename]%[NOEXT]");
				g_AutoNamePresets.push_back(newAutonamePreset);
				
				int i;
				for (i=0;i<(int)g_AutoNamePresets.size();i++)
				{
					// fill preset combobox
					//
#ifdef _WIN32
					ComboBox_AddString(GetDlgItem(hwnd,IDC_AUTONAMEPRESETS),g_AutoNamePresets[i].Description.c_str());
					ComboBox_SetItemData(GetDlgItem(hwnd,IDC_AUTONAMEPRESETS),i,(LPARAM)&g_AutoNamePresets[i].FormattingString);
#else
					SWELL_CB_AddString(hwnd,IDC_AUTONAMEPRESETS,g_AutoNamePresets[i].Description.c_str());
					SWELL_CB_SetItemData(hwnd, IDC_AUTONAMEPRESETS, i, (LONG)&g_AutoNamePresets[i].FormattingString);
#endif
				}
				
				SetDlgItemText(hwnd,IDC_EDIT1,"[trackname]%.My take.%[inctrackorder]");
				SetDlgItemText(hwnd,IDC_SAMPLEOUT,"Kick My take 01");
				
				g_hAutoNameDlg=hwnd;
				//
				LVCOLUMN col;
				col.mask=LVCF_TEXT|LVCF_WIDTH;
				col.cx=250;
				
				col.pszText=("Current take name");
				//col.
				
				ListView_InsertColumn(GetDlgItem(hwnd,IDC_AUTONAMEOUTPUT), 0 , &col);
				
				col.mask=LVCF_TEXT|LVCF_WIDTH;
				col.cx=250;
				col.pszText="New take name";
				ListView_InsertColumn(GetDlgItem(hwnd,IDC_AUTONAMEOUTPUT), 1 , &col);
				
				UpdateSampleNameList();
				
				return 0;
			}
		case WM_COMMAND:
			{
				if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDC_EDIT1)
					{
						char buf[500];
						GetDlgItemText(hwnd,IDC_EDIT1,buf,500);
						g_FormattingString.assign(buf);
						string SampleTakeName=GetNewTakeNameFromFormatString(buf,g_VecTakesToRename[0],1);
						SetDlgItemText(hwnd,IDC_SAMPLEOUT,SampleTakeName.c_str());
						UpdateSampleNameList();
						return 0;
					}
				if (HIWORD(wParam)==CBN_SELCHANGE && LOWORD(wParam)==IDC_AUTONAMEPRESETS)
				{
					string *formattingString;
					int comboIndex=ComboBox_GetCurSel(GetDlgItem(hwnd,IDC_AUTONAMEPRESETS));
					if (comboIndex>=0 && comboIndex<(int)g_AutoNamePresets.size())
					{
						formattingString=(string*)(LPARAM)ComboBox_GetItemData(GetDlgItem(hwnd,IDC_AUTONAMEPRESETS),comboIndex);
						if (formattingString!=0)
						{
							SetDlgItemText(hwnd,IDC_EDIT1,formattingString->c_str());
							g_FormattingString.assign(*formattingString);
						}
					}
					return 0;
				}
				switch(LOWORD(wParam))
				{
					
					
					case IDCANCEL:
						{
							
							g_AutonameCancelled=true;
							EndDialog(hwnd,0);
							return 0;
						}
					case IDOK:
						{
							char buf[500];
							GetDlgItemText(hwnd,IDC_EDIT1,buf,500);
							g_FormattingString.assign(buf);
							g_AutonameCancelled=false;
							EndDialog(hwnd,0);
							return 0;
						}
					return 0;
				}
			}
	}
	return 0;
}

bool MyTakeSortByTimeFunc (MediaItem_Take *a,MediaItem_Take *b) 
{ 
	MediaItem *CompaItem;
	CompaItem=(MediaItem*)GetSetMediaItemTakeInfo(a,"P_ITEM",NULL);
	double ItemPosA=*(double*)GetSetMediaItemInfo(CompaItem,"D_POSITION",NULL);
	CompaItem=(MediaItem*)GetSetMediaItemTakeInfo(b,"P_ITEM",NULL);
	double ItemPosB=*(double*)GetSetMediaItemInfo(CompaItem,"D_POSITION",NULL);
	if (ItemPosA<ItemPosB) return true;
	return false;
}

void DoAutoRename(COMMAND_T*)
{
	
	MediaItem *CurItem=0;
	MediaItem_Take *CurTake=0;
	MediaTrack *CurTrack=0;
	MediaTrack *PrevTrack=0;
	g_VecTakesToRename.clear();
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
				if (GetMediaItemNumTakes(CurItem)>0)
				{
					CurTake=GetMediaItemTake(CurItem,-1);
					g_VecTakesToRename.push_back(CurTake);
				}
			}
		}
	}
	if (g_VecTakesToRename.size()>0)
	{
		g_VecTakesToRenameTimeOrdered.assign(g_VecTakesToRename.begin(),g_VecTakesToRename.end());
		sort(g_VecTakesToRenameTimeOrdered.begin(),g_VecTakesToRenameTimeOrdered.end(),MyTakeSortByTimeFunc);
		DialogBox(g_hInst, MAKEINTRESOURCE(IDD_AUTORENAMETAKES), g_hwndParent, (DLGPROC)AutoRenameDlgProc);
		if (!g_AutonameCancelled)
		{
			vector<string> FormatTokens;
			TokenizeString(g_FormattingString,FormatTokens,"+");
			string NewTakeName;
			
			//TokenizeString(g_FormattingString,FormatTokens,"%");
			bool UseTimeSortedTakes=false;
			char argh[1024];
			int TrackItemCounter=0;
			int x;
			for (x=0;x<(int)FormatTokens.size();x++)
				if (FormatTokens[x].compare("[inctimeorder]")==0)
					UseTimeSortedTakes=true;
			if (!UseTimeSortedTakes)
			{
				for (i=0;i<(int)g_VecTakesToRename.size();i++)
				{
				
					NewTakeName.assign("");
					CurItem=(MediaItem*)GetSetMediaItemTakeInfo(g_VecTakesToRename[i],"P_ITEM",NULL);
					CurTrack=(MediaTrack*)GetSetMediaItemInfo(CurItem,"P_TRACK",NULL);
					if (PrevTrack && CurTrack!=PrevTrack) 
					{
						TrackItemCounter=0;
						PrevTrack=CurTrack;
					}
					strcpy(argh,g_FormattingString.c_str());
					string SampleName=GetNewTakeNameFromFormatString(argh,g_VecTakesToRename[i],TrackItemCounter+1);
					strcpy(argh,SampleName.c_str());
					GetSetMediaItemTakeInfo(g_VecTakesToRename[i],"P_NAME",argh);
					TrackItemCounter++;
					//PerformFormatStringParse((char*)g_FormattingString.c_str(),VecTakesToRename[i],i+1);
				}
			
			}
			Undo_OnStateChangeEx("Autorename takes",4,-1);
			UpdateTimeline();
		}
	} else MessageBox(g_hwndParent,"No items selected!","Error",MB_OK);
}