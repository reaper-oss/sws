/******************************************************************************
/ BroadCastWavCommands.cpp
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

int GetNumSelectedItems()
{
	MediaTrack* CurTrack;
	MediaItem* CurItem;
	int ItemCount=0;
	int trackID;
	int itemID;
	for (trackID=0;trackID<GetNumTracks();trackID++)
	{
		CurTrack=CSurf_TrackFromID(trackID+1,FALSE);
		int numItems=GetTrackNumMediaItems(CurTrack);
		for (itemID=0;itemID<numItems;itemID++)
		{
			CurItem = GetTrackMediaItem(CurTrack,itemID);
			bool ItemSelected=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (ItemSelected==TRUE) ItemCount++;
		}
	}
	return ItemCount;
			
}

void SanitizeString(char *StringToSanitize)
{
	//
	int LenOfString=(int)strlen(StringToSanitize);
	int i;
	for (i=0;i<LenOfString;i++)
	{
		char X=StringToSanitize[i];
		bool CharOk=false;
		if ((X>='A' && X<='Z') || (X>='a' && X<='z') || (X>='0' && X<='9') || (X==' '))
			CharOk=true;
		if (CharOk==false)
			StringToSanitize[i]='_';
	}

}

void DoRenameTakesWithBWAVDesc(COMMAND_T*)
{
// #define OVECCOUNT 30;
	MediaTrack* CurTrack;
	MediaItem* CurItem;
	MediaItem_Take* CurTake;
	PCM_source *ThePCMSource;
	bool ItemSelected;
	int numItems=-666;
	int numTakes=-666;
	int TakeToSelect=0;
	char Buf[8192];
	int sz=8192;
	int trackID;
	int itemID;
	for (trackID=0;trackID<GetNumTracks();trackID++)
	{
		CurTrack=CSurf_TrackFromID(trackID+1,FALSE);
		numItems=GetTrackNumMediaItems(CurTrack);
		for (itemID=0;itemID<numItems;itemID++)
		{
			CurItem = GetTrackMediaItem(CurTrack,itemID);
			ItemSelected=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (ItemSelected==TRUE)
			{
				//
				numTakes=GetMediaItemNumTakes(CurItem);
				if (numTakes>0)
				{
					//
					int takeInd;
					for (takeInd=0;takeInd<numTakes;takeInd++)
					{
						//
						sz=8192;
						CurTake=GetMediaItemTake(CurItem,takeInd);
						ThePCMSource=(PCM_source*)GetSetMediaItemTakeInfo(CurTake,"P_SOURCE",NULL);
						sz=ThePCMSource->Extended(PCM_SOURCE_EXT_GETMETADATA,"DESC",Buf,(void *)sz);
						if (sz>0)
						{
							//MessageBox(g_hwndParent,Buf,"Broadcast Wave Desc:",MB_OK); 
							SanitizeString(Buf);
							GetSetMediaItemTakeInfo(CurTake,"P_NAME",Buf);
							
							//delete Newbuf;
							
						}
						else MessageBox(g_hwndParent,"Take source media has no Broadcast Info Description","Info",MB_OK);
					}
				}
			}
		}
	}	
	Undo_OnStateChangeEx("Rename Takes With BWAV Description",4,-1);
	UpdateTimeline();
}

HWND h_renameDialog;

struct t_takerenameParams
{
	string NewTakeName;
	string NewFileName;
	MediaItem_Take *TakeToRename;
	bool RenameWasCancelled;
	bool DoSourceRename;
	bool SourceNameFromTakeName;
	int RenameTakeNumber;
	int TakesToRename;
	bool doOnlySourceRename;
};

t_takerenameParams  g_takerenameParams;

WDL_DLGRET NewRenameDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	//static WDL_WndSizer resizer;

	switch(Message)
    {
        case WM_INITDIALOG:
			{
				  
			   	char FullFileName[1024];
				char ShortFileName[1024];
				char *TxtBuf=(char*)GetSetMediaItemTakeInfo(g_takerenameParams.TakeToRename,"P_NAME",NULL);
				ExtractFileNameEx(TxtBuf,ShortFileName,true); // we don't like take names with .wavs and shit at the end
				SetDlgItemText(hwnd,IDC_TAKENAME_EDIT,TxtBuf);
				PCM_source *ThePCM=0;
				ThePCM=(PCM_source*)GetSetMediaItemTakeInfo(g_takerenameParams.TakeToRename,"P_SOURCE",NULL);
				if (ThePCM!=0)
				{
					if (strcmp(ThePCM->GetType(),"SECTION")!=0)
					{
						SetDlgItemText(hwnd,IDC_FULLFILENAME,ThePCM->GetFileName());
				
						strcpy(FullFileName,ThePCM->GetFileName());
						ExtractFileNameEx(FullFileName,ShortFileName,true);
						SetDlgItemText(hwnd,IDC_FILENAME_EDIT,ShortFileName);
					} else
					{
						PCM_source *TheOtherPCM=0;
						TheOtherPCM= ThePCM->GetSource();
						if (TheOtherPCM!=0)
						{
							SetDlgItemText(hwnd,IDC_FULLFILENAME,TheOtherPCM->GetFileName());
							strcpy(FullFileName,TheOtherPCM->GetFileName());
							ExtractFileNameEx(FullFileName,ShortFileName,true);
							SetDlgItemText(hwnd,IDC_FILENAME_EDIT,ShortFileName);

						}
					}
				}
				char wintitle[200];
				//SendMessage(GetDlgItem(hwnd,ID_TAKEONLY), DM_SETDEFID, ID_TAKEONLY,0);
				SendMessage(hwnd, DM_SETDEFID, ID_TAKEONLY,0);
				
				Button_SetStyle(GetDlgItem(hwnd,ID_TAKEONLY) ,BS_DEFPUSHBUTTON ,true);
				Button_SetStyle(GetDlgItem(hwnd,ID_TAKEANDSOURCE) ,BS_PUSHBUTTON,true);
				Button_SetStyle(GetDlgItem(hwnd,IDC_RENAMEMEDIA) ,BS_PUSHBUTTON,true);
				SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwnd, IDC_TAKENAME_EDIT), TRUE);
				sprintf(wintitle,"Rename take %d / %d",g_takerenameParams.RenameTakeNumber,g_takerenameParams.TakesToRename);
				SetWindowText(hwnd,wintitle);
				Button_Enable(GetDlgItem(hwnd,ID_TAKEANDSOURCE),0);
				Button_Enable(GetDlgItem(hwnd,IDC_RENAMEMEDIA),0);
				Button_SetStyle(GetDlgItem(hwnd,IDCANCEL) ,BS_PUSHBUTTON,true);
				return 0;
			}
		case WM_COMMAND:
			{
				switch(LOWORD(wParam))
				{
					case IDCANCEL:
						{
							
							g_takerenameParams.RenameWasCancelled=true;

							EndDialog(hwnd,0);
							return 0;
						}
					case IDOK:
						{
							EndDialog(hwnd,0);
							return 0;
						}
					case ID_TAKEONLY:
						{
							char namebuf[1024];
							GetDlgItemText(hwnd,IDC_TAKENAME_EDIT,namebuf,1023);
							g_takerenameParams.NewTakeName.assign(namebuf);
							g_takerenameParams.RenameWasCancelled=false;
							g_takerenameParams.DoSourceRename=false;
							g_takerenameParams.SourceNameFromTakeName=false;
							g_takerenameParams.doOnlySourceRename=false;
							EndDialog(hwnd,0);
							return 0;
						}
					case ID_TAKEANDSOURCE:
						{
							char namebuf[1024];
							GetDlgItemText(hwnd,IDC_TAKENAME_EDIT,namebuf,1023);
							g_takerenameParams.NewTakeName.assign(namebuf);
							g_takerenameParams.doOnlySourceRename=false;
							g_takerenameParams.RenameWasCancelled=false;
							g_takerenameParams.DoSourceRename=true;
							g_takerenameParams.SourceNameFromTakeName=true;
							EndDialog(hwnd,0);
							return 0;	
						}
					case IDC_RENAMEMEDIA:
						{
							char namebuf[1024];
							GetDlgItemText(hwnd,IDC_FILENAME_EDIT,namebuf,1023);
							g_takerenameParams.doOnlySourceRename=true;
							g_takerenameParams.NewTakeName.assign(namebuf);
							g_takerenameParams.RenameWasCancelled=false;
							g_takerenameParams.DoSourceRename=true;
							g_takerenameParams.SourceNameFromTakeName=true;
							EndDialog(hwnd,0);
							return 0;	
						}
				}
			}
	}
	return 0;
}

void ExtractFilePath(char *FullFileName,char *FilePath)
{
	//
	int LastDelimiternIndx=-1;
	int FileExtensionIndex=-1;
	int FullFileNameLen=(int)strlen(FullFileName);
	int i;
	for (i=FullFileNameLen;i>=0;i--)
	{
		char x=FullFileName[i];
		if (x=='\\')
		{
				LastDelimiternIndx=i;
				break;
		}
		

	}
	int j=0;
	for (i=0;i<LastDelimiternIndx+1;i++)
	{
		FilePath[j]=FullFileName[i];
		j++;
	}
	FilePath[j]=0;

}

void ExtractFileExtension(char *FullFileName, char *FileExtension)
{
	//
	int LastDelimiternIndx=-1;
	int FileExtensionIndex=-1;
	int FullFileNameLen=(int)strlen(FullFileName);
	int i;
	for (i=FullFileNameLen;i>=0;i--)
	{
		char x=FullFileName[i];
		if (FileExtensionIndex==-1 && x=='.')
		{
			FileExtensionIndex=i;
			break;
		}


	}
	
	//if (StripExtension==false) FileExtensionIndex=FullFileNameLen;
	int j=0;
	for (i=FileExtensionIndex+0;i<FullFileNameLen;i++)
	{
		FileExtension[j]=FullFileName[i];
		j++;
	}
	FileExtension[j]=0;
}

int ReplaceProjectMedia(char *OldFileName,char *NewFileName)
{
	MediaItem *CurItem;
	MediaItem_Take *CurTake;
	MediaTrack *CurTrack;
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
				PCM_source *ThePCM=0;
				ThePCM=(PCM_source*)GetSetMediaItemTakeInfo(CurTake,"P_SOURCE",NULL);
				if (ThePCM!=0)
				{
					if (strcmp(ThePCM->GetType(),"SECTION")!=0)
					{
						if (strcmp(ThePCM->GetFileName(),OldFileName)==0)
						{
							ThePCM->SetFileName(NewFileName);	
						}
					}
				}
			}
		}
	}
	return -666;

}

void DoRenameTakeDlg(COMMAND_T*)
{
	vector<MediaItem_Take*> VecTakesToRename;
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
			bool IsSel=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (IsSel)
			{
				CurTake=GetMediaItemTake(CurItem,-1);
				VecTakesToRename.push_back(CurTake);
			}
		}
	}
	for (i=0;i<(int)VecTakesToRename.size();i++)
	{
		g_takerenameParams.TakeToRename=VecTakesToRename[i];
		g_takerenameParams.TakesToRename=(int)VecTakesToRename.size();
		g_takerenameParams.RenameTakeNumber=i+1;
		DialogBox(g_hInst, MAKEINTRESOURCE(IDD_RENAMETAKEDLG), g_hwndParent, NewRenameDlgProc);
		if (g_takerenameParams.RenameWasCancelled) return;
		GetSetMediaItemTakeInfo(VecTakesToRename[i],"P_NAME",(void*)g_takerenameParams.NewTakeName.c_str());
	}
	UpdateTimeline();
	Undo_OnStateChangeEx("Rename take(s)",4,-1);
}

int SearchDirectoryForRPPs(std::vector<std::string> &refvecFiles,
                    const std::string        &refcstrRootDirectory,
                    const std::string        &refcstrExtension,
                    bool                     bSearchSubdirectories = true)
{
  std::string     strFilePath;             // Filepath
  std::string     strPattern;              // Pattern
  std::string     strExtension;            // Extension
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
        strFilePath.erase();
        //strFilePath = refcstrRootDirectory + FileInformation.cFileName;
		strFilePath = refcstrRootDirectory + "\\" + FileInformation.cFileName;
        if(FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
          if(bSearchSubdirectories)
          {
            // Search subdirectory
            int iRC = SearchDirectoryForRPPs(refvecFiles,
                                      strFilePath,
                                      refcstrExtension,
                                      bSearchSubdirectories);
            if(iRC)
              return iRC;
          }
        }
        else
        {
          // Check extension
          strExtension = FileInformation.cFileName;
          strExtension = strExtension.substr(strExtension.rfind(".") + 1);
			std::transform(strExtension.begin(), strExtension.end(), strExtension.begin(), (int(*)(int)) toupper);	
			//std::transform(refcstrExtension.begin(), refcstrExtension.end(), refcstrExtension.begin(), (int(*)(int)) toupper);
			std::string paskaKopio;
			paskaKopio=refcstrExtension;
			std::transform(paskaKopio.begin(), paskaKopio.end(), paskaKopio.begin(), (int(*)(int)) toupper);
		int extMatch=0;
		if (strExtension=="RPP")
			extMatch++;
		
		if (extMatch>0)
            // Save filename
            refvecFiles.push_back(strFilePath);
        }
      }
    } while(::FindNextFile(hFile, &FileInformation) == TRUE);

    // Close handle
    ::FindClose(hFile);

    DWORD dwError = ::GetLastError();
    if(dwError != ERROR_NO_MORE_FILES)
      return dwError;
  }

  return 0;
}

void DoOpenRPPofBWAVdesc(COMMAND_T*)
{
	vector<MediaItem_Take*> TheTakes;
	XenGetProjectTakes(TheTakes,true,true);
	if (TheTakes.size()==0)
	{
		MessageBox(g_hwndParent,"No item selected!","Error",MB_OK);
		return;
	}
	if (TheTakes.size()>1) 
	{
		MessageBox(g_hwndParent,"Only 1 selected item supported!","Error",MB_OK);
		return;
	}
	MediaItem_Take *CurTake;
	CurTake=TheTakes[0];
	PCM_source *ThePCM=(PCM_source*)GetSetMediaItemTakeInfo(CurTake,"P_SOURCE",NULL);
	if (ThePCM)
	{
		int sz=8192;
		char Buf[8192];		
						
		sz=ThePCM->Extended(PCM_SOURCE_EXT_GETMETADATA,"DESC",Buf,(void *)sz);
		if (sz>0)
		{
			string RPPFileName;
			string RPPdesc;
			RPPdesc.assign(Buf);
			RPPFileName=RPPdesc.substr(4,RPPdesc.size());
			int rc=PathFileExists(RPPFileName.c_str());
			if (rc==TRUE)
			{
				char RPPFileNameBuf[1024];
				sprintf(RPPFileNameBuf,"%s\\reaper.exe \"%s\"",GetExePath(),RPPFileName.c_str());
				if (!DoLaunchExternalTool(RPPFileNameBuf)) MessageBox(g_hwndParent,"Could not launch REAPER.","Error",MB_OK);
				
			} else
			{
				bool FolderCancelled=false;
				char RppSearchFolderName[1024];
				LPITEMIDLIST Folder_pidl=NULL;
				BROWSEINFO bi;
				
				bi.pszDisplayName=RppSearchFolderName;
				bi.pidlRoot=Folder_pidl;
				bi.hwndOwner=g_hwndParent;
				bi.lpszTitle="Select folder to find RPP files...";
				bi.ulFlags=0x0040|0x0200;
				bi.lpfn=NULL;
				bi.lParam=NULL;
				bi.iImage=0;
				LPITEMIDLIST pidl     = NULL;
				bool bResult;

				if ((pidl = SHBrowseForFolder(&bi)) != NULL)
				{
					bResult = SHGetPathFromIDList(pidl, RppSearchFolderName) ? true : false;
					
					CoTaskMemFree(pidl);
				} else FolderCancelled=true;
				if (!FolderCancelled)
				{
					vector<string> FoundRPPs;
					SearchDirectoryForRPPs(FoundRPPs,RppSearchFolderName,"pelle",true);
					int i;
					vector<string> FnCompos;
					SplitFileNameComponents(RPPFileName,FnCompos);
					bool TheRppWasFound=false;
					for (i=0;i<(int)FoundRPPs.size();i++)
					{
						int x=(int)FoundRPPs[i].find(FnCompos[1]);
						if (x!=string::npos)
						{
							char RPPFileNameBuf[1024];
					
							sprintf(RPPFileNameBuf,"%s\\reaper.exe \"%s\"",GetExePath(),FoundRPPs[i].c_str());
							if (!DoLaunchExternalTool(RPPFileNameBuf)) MessageBox(g_hwndParent,"Could not launch REAPER.","Error",MB_OK);
							TheRppWasFound=true;
							break;
						}

					}
					if (!TheRppWasFound)
					{
						MessageBox(g_hwndParent,"RPP was not found from selected folder, please try another folder.","Error",MB_OK);
					}
				}
			}
		}
	}
}

class section_source_data
{
public:
  virtual void foo()=0;
  double m_length;
  double m_startpos;
  double m_edgeoverlap_time;
  PCM_source *m_srcs[2];
 
  void *m_itemptr, *m_takeptr;
  int m_mode; // &1 = no-section, &2=reverse
 
};

void PerformSectionLoopNudge(int paramToNudge,double nudgeAmount)
{
	vector<MediaItem_Take*> TheTakes;
	XenGetProjectTakes(TheTakes,true,true);
	int i;
	for (i=0;i<(int)TheTakes.size();i++)
	{
		PCM_source *pcmSource=(PCM_source*)GetSetMediaItemTakeInfo(TheTakes[i],"P_SOURCE",0);
		if (pcmSource)
		{
			if (strcmp(pcmSource->GetType(),"SECTION")==0)
			{
				// cast the pcm_source to reaper's section source class, this might go BROKEN if Cockos changes the class!
				// well, that happened...
				// changed in 26th August 2009 to use section_source_data
				section_source_data *sectiosoorssi=(section_source_data*)pcmSource;
				// TODO : clamping/etc for params, Reaper might do nasties if these are not set to sensible values
				if (paramToNudge==0)
					sectiosoorssi->m_startpos=sectiosoorssi->m_startpos+nudgeAmount;
				if (paramToNudge==1)
					sectiosoorssi->m_length=sectiosoorssi->m_length+nudgeAmount;
				if (paramToNudge==2)
					sectiosoorssi->m_edgeoverlap_time =sectiosoorssi->m_edgeoverlap_time+nudgeAmount;			
			}
		}
	}
	Main_OnCommand(40441,0); // rebuild selected items' peaks
	UpdateTimeline();
}

void DoNudgeSectionLoopStartPlus(COMMAND_T*)
{
	PerformSectionLoopNudge(0,g_command_params.SectionLoopNudgeSecs); // 1 for section loop len
	Undo_OnStateChangeEx("Nudge section loop start offset positive",4,-1);
}

void DoNudgeSectionLoopStartMinus(COMMAND_T*)
{
	PerformSectionLoopNudge(0,-g_command_params.SectionLoopNudgeSecs); // 1 for section loop len
	Undo_OnStateChangeEx("Nudge section loop start offset negative",4,-1);
}

void DoNudgeSectionLoopLenPlus(COMMAND_T*)
{
	PerformSectionLoopNudge(1,g_command_params.SectionLoopNudgeSecs); // 1 for section loop len
	Undo_OnStateChangeEx("Nudge section loop length positive",4,-1);
}

void DoNudgeSectionLoopLenMinus(COMMAND_T*)
{
	PerformSectionLoopNudge(1,-g_command_params.SectionLoopNudgeSecs); // 1 for section loop len
	Undo_OnStateChangeEx("Nudge section loop length negative",4,-1);
}

void DoNudgeSectionLoopOlapPlus(COMMAND_T*)
{
	PerformSectionLoopNudge(2,g_command_params.SectionLoopNudgeSecs); // 2 for section loop overlap
	Undo_OnStateChangeEx("Nudge section loop overlap positive",4,-1);
}

void DoNudgeSectionLoopOlapMinus(COMMAND_T*)
{
	PerformSectionLoopNudge(2,-g_command_params.SectionLoopNudgeSecs); // 2 for section loop overlap
	Undo_OnStateChangeEx("Nudge section loop overlap negative",4,-1);
}