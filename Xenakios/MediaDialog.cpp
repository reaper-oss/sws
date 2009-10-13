/******************************************************************************
/ MediaDialog.cpp
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
				t_project_take TakeBlah;
				bool pureMIDItake=false;

				if (strcmp(ThePCM->GetType(),"SECTION")!=0)
				{
					TakeBlah.FileName=ThePCM->GetFileName();
				} else
				{
					PCM_source *SectionFilePCM=ThePCM->GetSource();
					TakeBlah.FileName=SectionFilePCM->GetFileName();
				}
				if (strcmp(ThePCM->GetType(),"MIDI")==0)
					{
						if (ThePCM->GetFileName()==0)
							pureMIDItake=true;
					}
				TakeBlah.TheTake=CurTake;
				TakeBlah.FileMissing=false;
				//if (PathFileExists(ThePCM->GetFileName())==0)
				if (!pureMIDItake)
				{
					if (PathFileExists(TakeBlah.FileName.c_str())==0)
						TakeBlah.FileMissing=true;

					ATakeList.push_back(TakeBlah);
				}
				//string FName;
				//FName.assign(ThePCM->GetFileName());
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
					if (CurTake!=0)
					{
						PCM_source *ThePCM=(PCM_source*)GetSetMediaItemTakeInfo(CurTake,"P_SOURCE",NULL);
						string FName;
						bool ispurelyMIDI=false;
						if (ThePCM!=0)
						{
							if (strcmp(ThePCM->GetType(),"SECTION")!=0)
							{
								if (strcmp(ThePCM->GetType(),"MIDI")==0 && strcmp(ThePCM->GetFileName(),"")==0)
									ispurelyMIDI=true;

								if (ThePCM->GetFileName())
									FName.assign(ThePCM->GetFileName());
								else
									ispurelyMIDI=true;
							}
							else
							{
								PCM_source *TheOtherPCM=ThePCM->GetSource();
								if (TheOtherPCM!=0)
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
		newstatus.IsOnline = PathFileExists(TempList[i].c_str()) ? true : false;
		AMediaList.push_back(newstatus);
	}
}

string g_CurrentScanFile;

bool g_scan_aborted=false;

int SearchDirectory(vector<string> &refvecFiles, const string &refcstrRootDirectory,
	const string &refcstrExtension, bool bSearchSubdirectories)
{
	string     strFilePath;             // Filepath
	string     strPattern;              // Pattern
	string     strExtension;            // Extension
	HANDLE          hFile;                   // Handle to file
	WIN32_FIND_DATA FileInformation;         // File information

	strPattern = refcstrRootDirectory + "\\*.*";

	hFile = ::FindFirstFile(strPattern.c_str(), &FileInformation);
	if(hFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if(FileInformation.cFileName[0] != '.')
			{
				if (g_scan_aborted)
					return 0;
				strFilePath.erase();
				strFilePath = refcstrRootDirectory + "\\" + FileInformation.cFileName;
				if(FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if(bSearchSubdirectories)
					{
						// Search subdirectory
						int iRC = SearchDirectory(refvecFiles, strFilePath, refcstrExtension, bSearchSubdirectories);
						if(iRC)
							return iRC;
					}
				}
				else
				{
					// Check extension
					strExtension = FileInformation.cFileName;
					strExtension = strExtension.substr(strExtension.rfind(".") + 1);
					transform(strExtension.begin(), strExtension.end(), strExtension.begin(), (int(*)(int)) toupper);	
					//transform(refcstrExtension.begin(), refcstrExtension.end(), refcstrExtension.begin(), (int(*)(int)) toupper);
					string paskaKopio;
					paskaKopio=refcstrExtension;
					transform(paskaKopio.begin(), paskaKopio.end(), paskaKopio.begin(), (int(*)(int)) toupper);
					int extMatch=0;
					if (strExtension=="WAV") extMatch++;
					//if (strExtension=="RPP") extMatch++;
					if (strExtension=="MID") extMatch++;
					if (strExtension=="AIF") extMatch++;
					if (strExtension=="MP3") extMatch++;
					if (strExtension=="WV") extMatch++;
					if (strExtension=="FLAC") extMatch++;
					if (strExtension=="APE") extMatch++;
					if (strExtension=="OGG") extMatch++;
					if (extMatch>0)
					{
						// Save filename
						refvecFiles.push_back(strFilePath);
						g_CurrentScanFile.assign(strFilePath);
					}
				}
			}
		}
		while(::FindNextFile(hFile, &FileInformation) == TRUE);

		// Close handle
		::FindClose(hFile);

		DWORD dwError = ::GetLastError();
		if(dwError != ERROR_NO_MORE_FILES)
		return dwError;
	}

	return 0;
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
				
	char buf2[2048];
	sprintf(buf2,"%s",buf);
	SearchDirectory(g_ProjFolFiles,buf2,"WAVA",true);
	//SearchDirectory(ProjFolFiles,buf2,"RPP",true);
	int j=0;
	bool HidePaths=false;
	if (Button_GetCheck(GetDlgItem(g_hMediaDlg,IDC_HIDEPATHS))==BST_CHECKED)
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
			if (UsedInProject)
				ListView_SetItemText(hList,j,1,"Yes")
			else ListView_SetItemText(hList,j,1,"No");
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
		} else
		{
			//MessageBox(g_hMediaDlg,"it's a section!","jep",MB_OK);
			PCM_source *TheOtherPCM=ThePCM->GetSource();
			if (TheOtherPCM!=0)
			{
				
				TheOtherPCM->SetFileName(TheNewFile.c_str());
			}
		}
		return 0;
	}
	return -666;
}

BOOL WINAPI MulMatchesFoundDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	//static WDL_WndSizer resizer;

	switch(Message)
    {
        case WM_INITDIALOG:
			{
				LVCOLUMN col;
				col.mask=LVCF_TEXT|LVCF_WIDTH;
				col.cx=425;
				col.pszText=TEXT("File name");
				ListView_InsertColumn(GetDlgItem(hwnd,IDC_MULMATCHLIST), 0 , &col);
				col.cx=100;
				col.pszText=TEXT("Date Modified");
				ListView_InsertColumn(GetDlgItem(hwnd,IDC_MULMATCHLIST), 1 , &col);
				col.cx=75;
				col.pszText=TEXT("Size (MB)");
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
					WIN32_FILE_ATTRIBUTE_DATA FileAttribs={0,};
					GetFileAttributesEx(g_MatchingFiles[i].c_str(),GetFileExInfoStandard,&FileAttribs);
					SYSTEMTIME FileDateTime;
					FileTimeToSystemTime(&FileAttribs.ftLastWriteTime,&FileDateTime);
					sprintf(buf,"%d/%d/%d",FileDateTime.wDay,FileDateTime.wMonth,FileDateTime.wYear);
					ListView_SetItemText(GetDlgItem(hwnd,IDC_MULMATCHLIST),i,1,buf);
				}
				return 0;
			}
		case WM_COMMAND:
			{
				switch(LOWORD(wParam))
				{
					case IDCANCEL:
						{
							EndDialog(hwnd,0);
							return 0;
						}
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

void DirScanThreadFunc(void *Param)
{
	FoundMediaFiles.clear();
	SearchDirectory(FoundMediaFiles,g_FolderName,"blah",true);
	g_ScanStatus=0;
}

BOOL WINAPI ScanProgDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
    {
        case WM_INITDIALOG:
			{
				g_ScanStatus=1;
				g_scan_aborted=false;
				_beginthread(DirScanThreadFunc, 0, NULL);
				SetTimer(hwnd,1717,250,NULL);
				return 0;
			}
		case WM_TIMER:
			{
				if (wParam==1717)
				{
					SetDlgItemText(hwnd,IDC_SCANFILE,g_CurrentScanFile.c_str());
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
				return 0;
			}
		case WM_COMMAND:
			{
				if (LOWORD(wParam==IDC_ABORT))
				{
					g_scan_aborted=true;
					return 0;
				}
				if (wParam==0xff)
				{
					SetDlgItemText(g_hScanProgressDlg,IDC_SCANFILE,g_CurrentScanFile.c_str());
					RedrawWindow(g_hScanProgressDlg, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
				}
				return 0;
			}
	}
	return 0;
}



void FindMissingFiles()
{
	bool FolderCancelled=false;
	BROWSEINFO bi;
	bi.pszDisplayName=g_FolderName;
	bi.pidlRoot=NULL;
	bi.hwndOwner=g_hMediaDlg;
	bi.lpszTitle="Select search folder";
	bi.ulFlags=0x0040|0x0200;
	bi.lpfn=NULL;
	bi.lParam=NULL;
	bi.iImage=0;
	LPITEMIDLIST pidl     = NULL;
	bool bResult;

	if ((pidl = SHBrowseForFolder(&bi)) != NULL)
	{
		bResult = SHGetPathFromIDList(pidl, g_FolderName) ? true : false;
		
		CoTaskMemFree(pidl);
	} else FolderCancelled=true;

	if (FolderCancelled==false)
	{
	
		char FullFilename[2048];
		char FullFilenameB[2048];
		char Shortfilename[2048];
		char ShortfilenameB[2048];
		
		//newstatus.OnlyFileName.assign(Shortfilename);
	
	DWORD TickA=GetTickCount();
	DialogBox(g_hInst,MAKEINTRESOURCE(IDD_SCANPROGR),g_hMediaDlg,(DLGPROC)ScanProgDlgProc);
	g_ScanStatus=0;
	g_ScanFinished=true;
	SetForegroundWindow(g_hMediaDlg);
	DWORD TickB=GetTickCount();
	DWORD TickC=TickB-TickA;
// 	char buf[2058];
	vector<t_project_take> ProjectTakes;
	vector<t_project_take> TakesMissingFiles;
	GetAllProjectTakes(ProjectTakes);
	int i;
	for (i=0;i<(int)ProjectTakes.size();i++)
		if (ProjectTakes[i].FileMissing==true)
			TakesMissingFiles.push_back(ProjectTakes[i]);
	int OfflineTakes=0;
	int MediasFound=0;
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

int NumTimesFileUsedInProject(string &fn)
{
	vector<MediaItem_Take*> thetakes;
	XenGetProjectTakes(thetakes,false,false);
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
				cmpfn.assign(src2->GetFileName());
			}
			if (fn.compare(cmpfn)==0)
				matches++;
		}
	}
	return matches;
}

void PopulateProjectUsedList(bool HidePaths)
{
	ListView_DeleteAllItems(GetDlgItem(g_hMediaDlg,IDC_PROJFILES_USED));
	LVITEM item;
	char buf[2048];
	int i;
	for (i=0;i<(int)g_RProjectFiles.size();i++)
	{
					item.mask=LVIF_TEXT;
					strcpy(buf,g_RProjectFiles[i].FileName.c_str());
					if (HidePaths)
					{
						char ShortName[1024];
						ExtractFileNameEx(buf,ShortName,false);
						item.pszText=ShortName;
					} else item.pszText=buf;
					item.iItem=i;
					item.iSubItem = 0;
					ListView_InsertItem(GetDlgItem(g_hMediaDlg,IDC_PROJFILES_USED),&item);
					if (g_RProjectFiles[i].IsOnline==true)
						ListView_SetItemText(GetDlgItem(g_hMediaDlg,IDC_PROJFILES_USED),i,2,"Online")
					else ListView_SetItemText(GetDlgItem(g_hMediaDlg,IDC_PROJFILES_USED),i,2,"Missing");
					char ynh[20];
					sprintf(ynh,"%d",NumTimesFileUsedInProject(g_RProjectFiles[i].FileName));
					ListView_SetItemText(GetDlgItem(g_hMediaDlg,IDC_PROJFILES_USED),i,1,ynh);
	}	
}

WDL_WndSizer g_wdlresizer;

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
	if (Button_GetCheck(GetDlgItem(g_hMediaDlg,IDC_SHOWUNUS))==BST_CHECKED)
								UpdateProjFolderList(GetDlgItem(g_hMediaDlg,IDC_PROJFOLMEDLIST),true);
							else UpdateProjFolderList(GetDlgItem(g_hMediaDlg,IDC_PROJFOLMEDLIST),false);
#else
	MessageBox(g_hwndParent,"Not implemented for OS-X","Sorry... :,(",MB_OK);
#endif
}

BOOL WINAPI ProjMediaDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	//static WDL_WndSizer resizer;

	switch(Message)
    {
        case WM_INITDIALOG:
			{
				RECT r;
				r.left=10;
				r.right=300;
				r.top=50;
				r.bottom=300;
				g_wdlresizer.init(hwnd);
				//resizer.init_item(IDOK,0.0,1.0,0.0f,1.0f);
				r.left=10;
				r.right=20;
				r.top=5;
				r.bottom=60;
				//SendMessage(GetDlgItem(hwnd,IDC_SHOWUNUS),WM_CHANGEUISTATE,
				
				//SetParent(GetDlgItem(hwnd,IDC_SHOWUNUS), GetDlgItem(hwnd,IDC_GROUP2));
				//SetParent(GetDlgItem(hwnd,IDC_DELFILESBUT), GetDlgItem(hwnd,IDC_GROUP2));
				//SetParent(GetDlgItem(hwnd,IDC_PROJFOLMEDLIST), GetDlgItem(hwnd,IDC_GROUP2));
				//SendMessage(GetDlgItem(hwnd,IDC_SHOWUNUS),WM_CHANGEUISTATE,
				

				//g_wdlresizer.init_item(IDC_GROUP1,0.5,0.0,1.0,0.0);
				g_wdlresizer.init_item(IDC_STATIC2,0.5,0.0,0.5,0.0);
				g_wdlresizer.init_item(ID_CLOSE,0.0f,1.0f,0.0f,1.0f);
				g_wdlresizer.init_item(IDC_DELFILESBUT,0.5,1.0,0.5,1.0);
				g_wdlresizer.init_item(IDC_SHOWUNUS,0.5,1.0,0.5,1.0);
				g_wdlresizer.init_item(IDC_FINDMISSING,0,1,0,1);
				g_wdlresizer.init_item(IDC_PROJFILES_USED,0.0,0.0,0.5,1.0);
				g_wdlresizer.init_item(IDC_PROJFOLMEDLIST,0.5,0.0,1.0,1.0);
				g_wdlresizer.init_item(IDC_DELFILESBUT2,0.5,1.0,0.5,1.0);
				g_wdlresizer.init_item(IDC_COPYTOPROJFOL,0,1,0,1);
				ListView_SetExtendedListViewStyle(GetDlgItem(hwnd, IDC_PROJFILES_USED), LVS_EX_FULLROWSELECT);
				ListView_SetExtendedListViewStyle(GetDlgItem(hwnd, IDC_PROJFOLMEDLIST), LVS_EX_FULLROWSELECT);
				
				//g_wdlresizer.init_item(IDC_DELFILESBUT,0.5,0.0,0.0,0.0);

				//g_wdlresizer.init_itemhwnd(GetDlgItem(hwnd,ID_CLOSE),1.0,1.0,1.0,1.0);
				g_hMediaDlg=hwnd;
				
				GetProjectFileList(g_RProjectFiles);
				LVCOLUMN col;
				col.mask=LVCF_TEXT|LVCF_WIDTH;
				col.cx=295;
				col.pszText=TEXT("File name");
				ListView_InsertColumn(GetDlgItem(hwnd,IDC_PROJFILES_USED), 0 , &col);
				col.cx=50;
				col.pszText=TEXT("Used");
				ListView_InsertColumn(GetDlgItem(hwnd,IDC_PROJFILES_USED), 1 , &col);
				col.cx=50;
				col.pszText=TEXT("Status");
				ListView_InsertColumn(GetDlgItem(hwnd,IDC_PROJFILES_USED), 2 , &col);
					
				col.cx=345;
				col.pszText=TEXT("File name");
				ListView_InsertColumn(GetDlgItem(hwnd,IDC_PROJFOLMEDLIST), 0 , &col);

				col.cx=50;
				col.pszText=TEXT("Used in project");
				ListView_InsertColumn(GetDlgItem(hwnd,IDC_PROJFOLMEDLIST), 1 , &col);

				
				if (Button_GetCheck(GetDlgItem(hwnd,IDC_HIDEPATHS))==BST_CHECKED)
					PopulateProjectUsedList(true); else PopulateProjectUsedList(false);
				
				if (Button_GetCheck(GetDlgItem(hwnd,IDC_SHOWUNUS))==BST_CHECKED)
								UpdateProjFolderList(GetDlgItem(hwnd,IDC_PROJFOLMEDLIST),true);
							else UpdateProjFolderList(GetDlgItem(hwnd,IDC_PROJFOLMEDLIST),false);

				return 0;
			}
		case WM_SIZE:
		{
			if (wParam != SIZE_MINIMIZED) 
			{
				g_wdlresizer.onResize(); 
			}		
		return 0;
		}

		case WM_COMMAND:
			{
				switch(LOWORD(wParam))
				{
					case IDCANCEL:
						{
							EndDialog(hwnd,0);
							return 0;
						}
					case ID_CLOSE:
						{
							
							EndDialog(hwnd,0);
							return 0;
						}
					case IDC_SHOWUNUS:
						{
							if (Button_GetCheck(GetDlgItem(hwnd,IDC_SHOWUNUS))==BST_CHECKED)
								UpdateProjFolderList(GetDlgItem(hwnd,IDC_PROJFOLMEDLIST),true);
							else UpdateProjFolderList(GetDlgItem(hwnd,IDC_PROJFOLMEDLIST),false);
							return 0;
						}
					case IDC_FINDMISSING:
						{
							FindMissingFiles();
							return 0;
						}
					case IDC_HIDEPATHS:
						{
							
							if (Button_GetCheck(GetDlgItem(hwnd,IDC_SHOWUNUS))==BST_CHECKED)
								UpdateProjFolderList(GetDlgItem(hwnd,IDC_PROJFOLMEDLIST),true);
							else UpdateProjFolderList(GetDlgItem(hwnd,IDC_PROJFOLMEDLIST),false);
							if (Button_GetCheck(GetDlgItem(hwnd,IDC_HIDEPATHS))==BST_CHECKED)
								PopulateProjectUsedList(true); else
									PopulateProjectUsedList(false);
							return 0;
						
						}
					case IDC_DELFILESBUT:
						{
							MessageBox(hwnd,"Delete files!","debug",MB_OK);
							return 0;
						}
					case IDC_DELFILESBUT2:
						
						{
							SendSelectedProjFolFilesToRecycleBin();
							//MessageBox(hwnd,"Send files to recycle bin!","debug",MB_OK);
							return 0;
						}

				}
			}

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