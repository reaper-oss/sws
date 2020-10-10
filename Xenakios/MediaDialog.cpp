/******************************************************************************
/ MediaDialog.cpp
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

#include "../Breeder/BR_Util.h"
#include "../SnM/SnM_Dlg.h"

#include <WDL/localize/localize.h>

using namespace std;

HWND g_hMediaDlg=0;
HWND g_hScanProgressDlg=0;
int g_ScanStatus=0;

struct t_mediafile_status
{
	string FileName;
	bool IsOnline;
};

struct t_project_take
{
	MediaItem_Take *TheTake;
	string FileName;
	bool FileMissing;
};

vector<t_mediafile_status> g_RProjectFiles;

string RemoveDoubleBackSlashes(string TheFileName)
{
	string ResultString;
	int j=0;
	int i;
	for (i=0;i<(int)TheFileName.size();i++)
	{
		if (j==TheFileName.size()) break;
		char x=TheFileName[j];
		if (TheFileName.compare(j,2,"\\\\")==0)
		{
			ResultString.append("\\");
			j+=2;
		} else
		{
			ResultString.append(1,x);
			j++;
		}
	}
	return ResultString;
}

void GetAllProjectTakes(vector<t_project_take>& ATakeList)
{
	ATakeList.clear();
	MediaTrack *CurTrack;
	MediaItem *CurItem;
	MediaItem_Take *CurTake;
	int i;
	int j;
	int k;
	for (i=0;i<GetNumTracks();i++)
	{
		CurTrack=CSurf_TrackFromID(i+1,false);
		for (j=0;j<GetTrackNumMediaItems(CurTrack);j++)
		{
			CurItem=GetTrackMediaItem(CurTrack,j);
			for (k=0;k<GetMediaItemNumTakes(CurItem);k++)
			{
				CurTake=GetMediaItemTake(CurItem,k);
				PCM_source *ThePCM=(PCM_source*)GetSetMediaItemTakeInfo(CurTake,"P_SOURCE",NULL);
				if (!ThePCM)
					break;
				t_project_take TakeBlah;
				bool pureMIDItake=false;

				if (strcmp(ThePCM->GetType(),"SECTION")!=0)
				{
					if (ThePCM->GetFileName())
						TakeBlah.FileName=ThePCM->GetFileName();
				}
				else
				{
					PCM_source *SectionFilePCM=ThePCM->GetSource();
					TakeBlah.FileName=(SectionFilePCM->GetFileName() ? SectionFilePCM->GetFileName() : ""); //JFB: dunno what it is behind (but safer I think..)
				}
				if (!strcmp(ThePCM->GetType(), "MIDI") || !strcmp(ThePCM->GetType(), "MIDIPOOL"))
				{
					if (ThePCM->GetFileName()==0)
						pureMIDItake=true;
				}
				TakeBlah.TheTake=CurTake;
				TakeBlah.FileMissing=false;
				if (!pureMIDItake)
				{
					if (!FileExists(TakeBlah.FileName.c_str()))
						TakeBlah.FileMissing=true;

					ATakeList.push_back(TakeBlah);
				}
			}
		}
	}
}

void GetProjectFileList(vector<t_mediafile_status>& AMediaList)
{
	vector<string> ProjectFiles;
	vector<string> TempList;
	vector<int> vecusedcount;
	int i;
	int j;
	int k;
	for (i=0;i<GetNumTracks();i++)
	{
		MediaTrack *CurTrack = CSurf_TrackFromID(i+1,false);
		for (j=0;j<GetTrackNumMediaItems(CurTrack);j++)
		{
			MediaItem *CurItem = GetTrackMediaItem(CurTrack,j);
			if (CurItem!=0)
			{
				for (k=0;k<GetMediaItemNumTakes(CurItem);k++)
				{
					MediaItem_Take *CurTake = GetMediaItemTake(CurItem,k);
					if (CurTake)
					{
						PCM_source *ThePCM=(PCM_source*)GetSetMediaItemTakeInfo(CurTake,"P_SOURCE",NULL);
						string FName;
						bool ispurelyMIDI=false;
						if (ThePCM)
						{
							if (strcmp(ThePCM->GetType(),"SECTION")!=0)
							{
								if (ThePCM->GetFileName() && !strcmp(ThePCM->GetFileName(),"") && (!strcmp(ThePCM->GetType(), "MIDI") || !strcmp(ThePCM->GetType(), "MIDIPOOL")))
									ispurelyMIDI=true;

								if (ThePCM->GetFileName())
									FName.assign(ThePCM->GetFileName());
								else
									ispurelyMIDI=true;
							}
							else
							{
								PCM_source *TheOtherPCM=ThePCM->GetSource();
								if (TheOtherPCM!=0 && TheOtherPCM->GetFileName())
									FName.assign(TheOtherPCM->GetFileName());
								else
									FName.assign("no file...");
							}
							if (!ispurelyMIDI)
							{
								int Matches=0;
								for (int L=0;L<(int)TempList.size();L++)
								{
									if (FName.compare(TempList[L])==0)
										Matches++;
								}
								if (Matches==0)
									TempList.push_back(FName);
							}
						}
					}
				}
			}
		}
	}

	AMediaList.clear();
	for (i=0;i<(int)TempList.size();i++)
	{
		t_mediafile_status newstatus;
		newstatus.FileName.assign(TempList[i]);
		string::size_type pos;
		while ((pos = newstatus.FileName.find('/')) != string::npos)
			newstatus.FileName[pos] = '\\';
		newstatus.IsOnline = FileExists(TempList[i].c_str());
		AMediaList.push_back(newstatus);
	}
}

int IsFileUsedInProject(string AFile)
{
	int Matches=0;
	int i;
	for (i=0;i<(int)g_RProjectFiles.size();i++)
	{
		if (AFile.compare(g_RProjectFiles[i].FileName)==0) Matches++;
	}
	return Matches;
	//if (Matches>0) return true; else return false;
}

vector<string> g_ProjFolFiles;

void UpdateProjFolderList(HWND hList, bool onlyUnused)
{
	LVITEM item;

	int NumListEntries=ListView_GetItemCount(GetDlgItem(g_hMediaDlg,IDC_PROJFOLMEDLIST));
	int i=0;

	for (i=0;i<NumListEntries;i++)
	{
		item.mask=LVIF_PARAM;
		item.iItem=i;
		item.iSubItem=0;
		ListView_GetItem(GetDlgItem(g_hMediaDlg,IDC_PROJFOLMEDLIST),&item);
		if (item.lParam!=0)
		{
			string *blah=(string*)item.lParam;


			delete blah;
			blah=0;
		}
	}
	item.mask=LVIF_TEXT|LVIF_PARAM;
	char buf[2048];

	g_ProjFolFiles.clear();
	ListView_DeleteAllItems(hList);
	GetProjectPath(buf,2048);

	SearchDirectory(g_ProjFolFiles,buf,NULL,true);
	int j=0;
	bool HidePaths=false;
	if (IsDlgButtonChecked(g_hMediaDlg,IDC_HIDEPATHS) == BST_CHECKED)
		HidePaths=true;
	for (i=0;i<(int)g_ProjFolFiles.size();i++)
	{
		string *TempString=new string;
		//TempString.resize(0);
		TempString->assign(RemoveDoubleBackSlashes(g_ProjFolFiles[i]));
		strcpy(buf,TempString->c_str());
		item.lParam=(LPARAM)TempString;
		if (HidePaths)
		{
			char ShortName[1024];

			ExtractFileNameEx(buf,ShortName,false);
			item.pszText=ShortName;
		} else
			item.pszText=buf;

		item.iItem=j;
		item.iSubItem = 0;
		int UsedInProject=IsFileUsedInProject(g_ProjFolFiles[i]);
		if (onlyUnused)
		{
			if (UsedInProject==0)
			{
				ListView_InsertItem(hList,&item);
				ListView_SetItemText(hList,j,1,"No");
				j++;
			}
		} else
		{
			ListView_InsertItem(hList,&item);
			ListView_SetItemText(hList,j,1,UsedInProject ? "Yes" : "No");
			j++;
		}
	}
}

vector<string> g_MatchingFiles;
int g_SelectedMatchFile;

int ReplaceTakeSourceFile(MediaItem_Take *TheTake,string TheNewFile)
{

	PCM_source *ThePCM=0;
	ThePCM=(PCM_source*)GetSetMediaItemTakeInfo(TheTake,"P_SOURCE",NULL);
	if (ThePCM!=0)
	{
		if (strcmp(ThePCM->GetType(),"SECTION")!=0)
		{
			ThePCM->SetFileName(TheNewFile.c_str());
		}
		else
		{
			//MessageBox(g_hMediaDlg,"it's a section!","jep",MB_OK);
			PCM_source *TheOtherPCM=ThePCM->GetSource();
			if (TheOtherPCM!=0)
				TheOtherPCM->SetFileName(TheNewFile.c_str());
		}
		return 0;
	}
	return -666;
}

WDL_DLGRET MulMatchesFoundDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (ThemeListViewInProc(hwnd, Message, lParam, GetDlgItem(hwnd,IDC_MULMATCHLIST), true))
		return 1;
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, Message, wParam, lParam))
		return r;

	switch(Message)
    {
        case WM_INITDIALOG:
		{
			if (SWS_THEMING) SNM_ThemeListView(GetDlgItem(hwnd,IDC_MULMATCHLIST));
			LVCOLUMN col;
			col.mask=LVCF_TEXT|LVCF_WIDTH;
			col.cx=425;
			col.pszText=(char*)"File name";
			ListView_InsertColumn(GetDlgItem(hwnd,IDC_MULMATCHLIST), 0 , &col);
			col.cx=100;
			col.pszText=(char*)"Date Modified";
			ListView_InsertColumn(GetDlgItem(hwnd,IDC_MULMATCHLIST), 1 , &col);
			col.cx=75;
			col.pszText=(char*)"Size (MB)";
			ListView_InsertColumn(GetDlgItem(hwnd,IDC_MULMATCHLIST), 2 , &col);
			LVITEM item;
			char buf[2048];
			int i;
			for (i=0;i<(int)g_MatchingFiles.size();i++)
			{
				item.mask=LVIF_TEXT;
				strcpy(buf,g_MatchingFiles[i].c_str());
				item.pszText=buf;
				item.iItem=i;
				item.iSubItem = 0;
				ListView_InsertItem(GetDlgItem(hwnd,IDC_MULMATCHLIST),&item);
#ifdef _WIN32 // TODO mac file attributes
				WIN32_FILE_ATTRIBUTE_DATA FileAttribs={0,};
				GetFileAttributesEx(g_MatchingFiles[i].c_str(),GetFileExInfoStandard,&FileAttribs);
				SYSTEMTIME FileDateTime;
				FileTimeToSystemTime(&FileAttribs.ftLastWriteTime,&FileDateTime);
				sprintf(buf,"%d/%d/%d",FileDateTime.wDay,FileDateTime.wMonth,FileDateTime.wYear);
				ListView_SetItemText(GetDlgItem(hwnd,IDC_MULMATCHLIST),i,1,buf);
#endif
			}
			return 0;
		}
		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case IDCANCEL:
					EndDialog(hwnd,0);
					return 0;
				case IDOK:
				{
					int i;
					for (i=0;i<ListView_GetItemCount(GetDlgItem(hwnd,IDC_MULMATCHLIST));i++)
					{
						int rc=ListView_GetItemState(GetDlgItem(hwnd,IDC_MULMATCHLIST),i,LVIS_SELECTED);
						if (rc==LVIS_SELECTED)
						{
							g_SelectedMatchFile=i;
							break;
						}
					}

					EndDialog(hwnd,0);
					return 0;
				}
			}
		}
	}
	return 0;
}


bool g_ScanFinished=false;

vector<string> FoundMediaFiles;
char g_FolderName[1024] = "";

unsigned int WINAPI DirScanThreadFunc(void*)
{
	FoundMediaFiles.clear();
	SearchDirectory(FoundMediaFiles, g_FolderName, NULL, true);
	g_ScanStatus = 0;
	return 0;
}

WDL_DLGRET ScanProgDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static HANDLE hThread = NULL;

	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, Message, wParam, lParam))
		return r;

	switch(Message)
    {
        case WM_INITDIALOG:
			{
				g_ScanStatus=1;
				g_bAbortScan=false;
				hThread = (HANDLE)_beginthreadex(NULL, 0, DirScanThreadFunc, 0, 0, 0);
				SetTimer(hwnd,1717,250,NULL);
				return 0;
			}
		case WM_TIMER:
			{
				if (wParam==1717)
				{
					SetDlgItemText(hwnd,IDC_SCANFILE,g_CurrentScanFile);
					if (g_ScanStatus==0)
					{
						KillTimer(hwnd,1717);
						EndDialog(hwnd,0);
					}
				}
				return 0;
			}
		case WM_DESTROY:
			{
				g_hScanProgressDlg=0;
				EndDialog(hwnd,0);
				if (hThread)
					CloseHandle(hThread);
				return 0;
			}
		case WM_COMMAND:
			{
				if (LOWORD(wParam==IDC_ABORT))
				{
					g_bAbortScan=true;
					return 0;
				}
				if (wParam==0xff)
				{
					SetDlgItemText(g_hScanProgressDlg,IDC_SCANFILE,g_CurrentScanFile);
#ifdef _WIN32 // TODO is this necessary?  what to do for OSX?
					RedrawWindow(g_hScanProgressDlg, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
#else
					InvalidateRect(g_hScanProgressDlg, NULL, 0);
#endif
				}
				return 0;
			}
	}
	return 0;
}



void FindMissingFiles()
{
	if (BrowseForDirectory("Select search folder", NULL, g_FolderName, 1024))
	{
		char FullFilename[2048];
		char FullFilenameB[2048];
		char Shortfilename[2048];
		char ShortfilenameB[2048];

		DialogBox(g_hInst,MAKEINTRESOURCE(IDD_SCANPROGR),g_hMediaDlg,(DLGPROC)ScanProgDlgProc);
		g_ScanStatus=0;
		g_ScanFinished=true;
		SetForegroundWindow(g_hMediaDlg);
		vector<t_project_take> ProjectTakes;
		vector<t_project_take> TakesMissingFiles;
		GetAllProjectTakes(ProjectTakes);
		int i;
		for (i=0;i<(int)ProjectTakes.size();i++)
			if (ProjectTakes[i].FileMissing==true)
				TakesMissingFiles.push_back(ProjectTakes[i]);
		Main_OnCommand(40100,0); // set all media offline
		int SanityCheck=(int)TakesMissingFiles.size();
		i=0;

		bool TakesLeft=true;
		if (TakesMissingFiles.size()==0) TakesLeft=false;
		while (TakesLeft==true)
		{
			t_project_take TempTake;
			TempTake=TakesMissingFiles[i];
			int FileMatches=0;
			strcpy(FullFilename,TakesMissingFiles[i].FileName.c_str());
			ExtractFileNameEx(FullFilename,Shortfilename,false);
			g_MatchingFiles.clear();
			int j;
			for (j=0;j<(int)FoundMediaFiles.size();j++)
			{
				strcpy(FullFilenameB,FoundMediaFiles[j].c_str());
				ExtractFileNameEx(FullFilenameB,ShortfilenameB,false);
				if (strcmp(Shortfilename,ShortfilenameB)==0)
				{
					FileMatches++;
					g_MatchingFiles.push_back(FoundMediaFiles[j]);
				}
			}
			string TheMatchingFile;
			TheMatchingFile.assign(TakesMissingFiles[i].FileName);
			if (g_MatchingFiles.size()==0)
				TakesMissingFiles.erase(TakesMissingFiles.begin()+i);
			else if (g_MatchingFiles.size()==1)
			{
				TheMatchingFile.assign(g_MatchingFiles[0]);
				ReplaceTakeSourceFile(TakesMissingFiles[i].TheTake,TheMatchingFile);
				TakesMissingFiles.erase(TakesMissingFiles.begin()+i);
			}
			else if (g_MatchingFiles.size()>1)
			{
				g_SelectedMatchFile=-1;
				DialogBox(g_hInst,MAKEINTRESOURCE(IDD_MULMATCH),g_hMediaDlg , (DLGPROC)MulMatchesFoundDlgProc);
				if (g_SelectedMatchFile>=0)
				{
					TheMatchingFile.assign(g_MatchingFiles[g_SelectedMatchFile]);
					ReplaceTakeSourceFile(TakesMissingFiles[i].TheTake,TheMatchingFile);
				}
				TakesMissingFiles.erase(TakesMissingFiles.begin()+i);
			}
			// Searching remaining takes using the same file...
			int k;
			for (k=0;k<(int)TakesMissingFiles.size();k++)
			{
				if (TakesMissingFiles[k].FileName.compare(FullFilename)==0)
				{
					// replace file
					ReplaceTakeSourceFile(TakesMissingFiles[k].TheTake,TheMatchingFile);
					TakesMissingFiles.erase(TakesMissingFiles.begin()+k);
				}
			}
			if (TakesMissingFiles.size()==0) TakesLeft=false;
			SanityCheck--;
			if (SanityCheck<0)
			{
				// Sanity check, exiting while (TakesMissingFiles.size()>0)
				break;
			}
		}
		Main_OnCommand(40101,0); // set all media online
		Main_OnCommand(40047,0); // build any missing peaks
		Undo_OnStateChangeEx("Find missing project media",4,-1);
	}
}

int NumTimesFileUsedInProject(string &fn, vector<MediaItem_Take*>& thetakes)
{
	int i;
	int matches=0;
	string cmpfn;
	for (i=0;i<(int)thetakes.size();i++)
	{
		PCM_source *src=(PCM_source*)GetSetMediaItemTakeInfo(thetakes[i],"P_SOURCE",0);
		if (src)
		{
			cmpfn.assign("not a valid filename 45FF78E");
			if (src->GetFileName() && strlen(src->GetFileName())>0)
				cmpfn.assign(src->GetFileName());
			if (strcmp(src->GetType(),"SECTION")==0)
			{
				PCM_source *src2=src->GetSource();
				if (src2)
					cmpfn.assign(src2->GetFileName() ? src2->GetFileName() : "");
			}
			if (fn.compare(cmpfn)==0)
				matches++;
		}
	}
	return matches;
}

void PopulateProjectUsedList(bool HidePaths)
{
	ListView_DeleteAllItems(GetDlgItem(g_hMediaDlg, IDC_PROJFILES_USED));
	LVITEM item;
	char buf[2048];
	vector<MediaItem_Take*> thetakes;
	XenGetProjectTakes(thetakes, false, false);

	for (int i = 0; i < (int)g_RProjectFiles.size(); i++)
	{
		item.mask = LVIF_TEXT;
		strcpy(buf, g_RProjectFiles[i].FileName.c_str());
		if (HidePaths)
		{
			char ShortName[1024];
			ExtractFileNameEx(buf, ShortName, false);
			item.pszText = ShortName;
		}
		else
			item.pszText = buf;
		item.iItem = i;
		item.iSubItem = 0;
		ListView_InsertItem(GetDlgItem(g_hMediaDlg, IDC_PROJFILES_USED), &item);
		ListView_SetItemText(GetDlgItem(g_hMediaDlg,IDC_PROJFILES_USED), i, 2, g_RProjectFiles[i].IsOnline ? "Online" : "Missing");
		char ynh[20];
		sprintf(ynh, "%d", NumTimesFileUsedInProject(g_RProjectFiles[i].FileName, thetakes));
		ListView_SetItemText(GetDlgItem(g_hMediaDlg, IDC_PROJFILES_USED), i, 1, ynh);
	}
}

void SendSelectedProjFolFilesToRecycleBin()
{
#ifdef _WIN32
	int NumListEntries=ListView_GetItemCount(GetDlgItem(g_hMediaDlg,IDC_PROJFOLMEDLIST));
	int i;
	for (i=0;i<NumListEntries;i++)
	{
		if (ListView_GetItemState(GetDlgItem(g_hMediaDlg,IDC_PROJFOLMEDLIST),i,LVIS_SELECTED))
		{
			LVITEM item;
			item.mask=LVIF_PARAM;
			item.iItem=i;
			item.iSubItem=0;
			ListView_GetItem(GetDlgItem(g_hMediaDlg,IDC_PROJFOLMEDLIST),&item);
			string *oih=(string*)item.lParam;
			SendFileToRecycleBin(oih->c_str());
		}
	}
	UpdateProjFolderList(GetDlgItem(g_hMediaDlg,IDC_PROJFOLMEDLIST), IsDlgButtonChecked(g_hMediaDlg,IDC_SHOWUNUS) == BST_CHECKED);
#else
	MessageBox(g_hwndParent, __LOCALIZE("Not supported on OSX and Linux, sorry!", "sws_mbox"), __LOCALIZE("SWS - Error", "sws_mbox"), MB_OK);
#endif
}

WDL_DLGRET ProjMediaDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static WDL_WndSizer resizer;

	if (ThemeListViewInProc(hwnd, Message, lParam, GetDlgItem(hwnd,IDC_PROJFILES_USED), true))
		return 1;
	if (ThemeListViewInProc(hwnd, Message, lParam, GetDlgItem(hwnd,IDC_PROJFOLMEDLIST), true))
		return 1;
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, Message, wParam, lParam))
		return r;

	switch(Message)
    {
        case WM_INITDIALOG:
		{
			if (SWS_THEMING)
			{
				SNM_ThemeListView(GetDlgItem(hwnd,IDC_PROJFILES_USED));
				SNM_ThemeListView(GetDlgItem(hwnd,IDC_PROJFOLMEDLIST));
			}

			resizer.init(hwnd);
			//resizer.init_item(IDOK,0.0,1.0,0.0f,1.0f);

			//SendMessage(GetDlgItem(hwnd,IDC_SHOWUNUS),WM_CHANGEUISTATE,
			//SetParent(GetDlgItem(hwnd,IDC_SHOWUNUS), GetDlgItem(hwnd,IDC_GROUP2));
			//SetParent(GetDlgItem(hwnd,IDC_DELFILESBUT), GetDlgItem(hwnd,IDC_GROUP2));
			//SetParent(GetDlgItem(hwnd,IDC_PROJFOLMEDLIST), GetDlgItem(hwnd,IDC_GROUP2));
			//SendMessage(GetDlgItem(hwnd,IDC_SHOWUNUS),WM_CHANGEUISTATE,


			//resizer.init_item(IDC_GROUP1,0.5,0.0,1.0,0.0);
			resizer.init_item(IDC_STATIC2,0.5,0.0,0.5,0.0);
			resizer.init_item(ID_CLOSE,0.0f,1.0f,0.0f,1.0f);
			resizer.init_item(IDC_DELFILESBUT,0.5,1.0,0.5,1.0);
			resizer.init_item(IDC_SHOWUNUS,0.5,1.0,0.5,1.0);
			resizer.init_item(IDC_FINDMISSING,0,1,0,1);
			resizer.init_item(IDC_PROJFILES_USED,0.0,0.0,0.5,1.0);
			resizer.init_item(IDC_PROJFOLMEDLIST,0.5,0.0,1.0,1.0);
			resizer.init_item(IDC_DELFILESBUT2,0.5,1.0,0.5,1.0);
			resizer.init_item(IDC_COPYTOPROJFOL,0,1,0,1);
#ifdef _WIN32
			ListView_SetExtendedListViewStyle(GetDlgItem(hwnd, IDC_PROJFILES_USED), LVS_EX_FULLROWSELECT);
			ListView_SetExtendedListViewStyle(GetDlgItem(hwnd, IDC_PROJFOLMEDLIST), LVS_EX_FULLROWSELECT);
#endif
			//resizer.init_item(IDC_DELFILESBUT,0.5,0.0,0.0,0.0);

			//resizer.init_itemhwnd(GetDlgItem(hwnd,ID_CLOSE),1.0,1.0,1.0,1.0);
			g_hMediaDlg=hwnd;

			GetProjectFileList(g_RProjectFiles);
			LVCOLUMN col;
			col.mask=LVCF_TEXT|LVCF_WIDTH;
			col.cx=295;
			col.pszText=(char*)"File name";
			ListView_InsertColumn(GetDlgItem(hwnd,IDC_PROJFILES_USED), 0 , &col);
			col.cx=50;
			col.pszText=(char*)"Used";
			ListView_InsertColumn(GetDlgItem(hwnd,IDC_PROJFILES_USED), 1 , &col);
			col.cx=50;
			col.pszText=(char*)"Status";
			ListView_InsertColumn(GetDlgItem(hwnd,IDC_PROJFILES_USED), 2 , &col);

			col.cx=345;
			col.pszText=(char*)"File name";
			ListView_InsertColumn(GetDlgItem(hwnd,IDC_PROJFOLMEDLIST), 0 , &col);

			col.cx=50;
			col.pszText=(char*)"Used in project";
			ListView_InsertColumn(GetDlgItem(hwnd,IDC_PROJFOLMEDLIST), 1 , &col);

			PopulateProjectUsedList(IsDlgButtonChecked(hwnd, IDC_HIDEPATHS) == BST_CHECKED);
			UpdateProjFolderList(GetDlgItem(hwnd,IDC_PROJFOLMEDLIST), IsDlgButtonChecked(hwnd, IDC_SHOWUNUS) == BST_CHECKED);

			break;
		}
		case WM_SIZE:
			if (wParam != SIZE_MINIMIZED)
			{
				resizer.onResize();
			}
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDCANCEL:
					EndDialog(hwnd,0);
					break;
				case ID_CLOSE:
					EndDialog(hwnd,0);
					break;
				case IDC_SHOWUNUS:
					UpdateProjFolderList(GetDlgItem(hwnd, IDC_PROJFOLMEDLIST), IsDlgButtonChecked(hwnd, IDC_SHOWUNUS) == BST_CHECKED);
					break;
				case IDC_FINDMISSING:
					FindMissingFiles();
					break;
				case IDC_HIDEPATHS:
					UpdateProjFolderList(GetDlgItem(hwnd, IDC_PROJFOLMEDLIST), IsDlgButtonChecked(hwnd, IDC_SHOWUNUS) == BST_CHECKED);
					PopulateProjectUsedList(IsDlgButtonChecked(hwnd, IDC_HIDEPATHS) == BST_CHECKED);
					break;
				case IDC_DELFILESBUT:
					MessageBox(hwnd,"Delete files!","debug",MB_OK);
					break;
				case IDC_DELFILESBUT2:
					SendSelectedProjFolFilesToRecycleBin();
					//MessageBox(hwnd,"Send files to recycle bin!","debug",MB_OK);
					break;
			}
			break;
	}
	return 0;
}

void DoShowProjectMediaDlg(COMMAND_T*)
{
	DialogBox(g_hInst,MAKEINTRESOURCE(IDD_MEDIADLG), g_hwndParent, (DLGPROC)ProjMediaDlgProc);
}

void DoFindMissingMedia(COMMAND_T*)
{
	GetProjectFileList(g_RProjectFiles);
	g_hMediaDlg=g_hwndParent;
	FindMissingFiles();
}
