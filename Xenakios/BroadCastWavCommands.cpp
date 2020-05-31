/******************************************************************************
/ BroadCastWavCommands.cpp
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

#include <WDL/localize/localize.h>

using namespace std;

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

void DoRenameTakesWithBWAVDesc(COMMAND_T* ct)
{
	WDL_TypedBuf<MediaItem*> selectedItems;
	SWS_GetSelectedMediaItems(&selectedItems);
	
	for (int i = 0; i < selectedItems.GetSize(); i++)
	{
		for (int iTake = 0; iTake < GetMediaItemNumTakes(selectedItems.Get()[i]); iTake++)
		{
			MediaItem_Take* curTake = GetMediaItemTake(selectedItems.Get()[i], iTake);
			PCM_source* pSrc = (PCM_source*)GetSetMediaItemTakeInfo(curTake, "P_SOURCE", NULL);
			if (!pSrc) 
				break;
			
			char buf[8192];
			int sz = pSrc->Extended(PCM_SOURCE_EXT_GETMETADATA, (void*)"DESC", buf, (void*)(INT_PTR)(int)sizeof(buf));
			if (sz > 0 && buf[0])
			{
				SanitizeString(buf);
				GetSetMediaItemTakeInfo(curTake, "P_NAME", buf);
			}
			else
				MessageBox(g_hwndParent, __LOCALIZE("Take source media has no Broadcast Info Description","sws_mbox"), __LOCALIZE("Xenakios - Error","sws_mbox"),MB_OK);
		}
	}	
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct), UNDO_STATE_ITEMS, -1);
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
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, Message, wParam, lParam))
		return r;

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
				if (ThePCM!=0 && ThePCM->GetFileName())
				{
					if (strcmp(ThePCM->GetType(),"SECTION")!=0)
					{
						SetDlgItemText(hwnd,IDC_FULLFILENAME, ThePCM->GetFileName());
						strcpy(FullFileName, ThePCM->GetFileName());
						ExtractFileNameEx(FullFileName,ShortFileName,true);
						SetDlgItemText(hwnd,IDC_FILENAME_EDIT,ShortFileName);
					} 
					else
					{
						PCM_source *TheOtherPCM=0;
						TheOtherPCM= ThePCM->GetSource();
						if (TheOtherPCM!=0 && TheOtherPCM->GetFileName())
						{
							SetDlgItemText(hwnd,IDC_FULLFILENAME, TheOtherPCM->GetFileName());
							strcpy(FullFileName,TheOtherPCM->GetFileName());
							ExtractFileNameEx(FullFileName,ShortFileName,true);
							SetDlgItemText(hwnd,IDC_FILENAME_EDIT,ShortFileName);

						}
					}
				}
				char wintitle[200];
				SetFocus(GetDlgItem(hwnd, IDC_TAKENAME_EDIT));
				SendMessage(GetDlgItem(hwnd, IDC_TAKENAME_EDIT), EM_SETSEL, 0, -1);
				snprintf(wintitle, sizeof(wintitle), __LOCALIZE_VERFMT("Rename take %d / %d","sws_DLG_116"),g_takerenameParams.RenameTakeNumber, g_takerenameParams.TakesToRename);
				SetWindowText(hwnd,wintitle);
				EnableWindow(GetDlgItem(hwnd,ID_TAKEANDSOURCE), 0);
				EnableWindow(GetDlgItem(hwnd,IDC_RENAMEMEDIA), 0);
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

void ExtractFilePath(const char *FullFileName,char *FilePath)
{
	strcpy(FilePath, FullFileName);
	char* pEnd = strrchr(FilePath, PATH_SLASH_CHAR);
	if (pEnd)
		*pEnd = 0;
}

void ExtractFileExtension(const char *FullFileName, char *FileExtension)
{
	char* pExt = strrchr((char*)FullFileName, '.');
	if (!pExt)
		*FileExtension = 0;
	else
		strcpy(FileExtension, pExt+1);
}

int ReplaceProjectMedia(char *OldFileName,char *NewFileName)
{
	MediaItem *CurItem;
	MediaItem_Take *CurTake;
	MediaTrack *CurTrack;
	int i, j, k;
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
				if (ThePCM!=0 && ThePCM->GetFileName())
					if (strcmp(ThePCM->GetType(),"SECTION")!=0)
						if (strcmp(ThePCM->GetFileName(),OldFileName)==0)
							ThePCM->SetFileName(NewFileName);	
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
				if (CurTake)
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
	Undo_OnStateChangeEx(__LOCALIZE("Rename take(s)","sws_undo"),4,-1);
}

void DoOpenRPPofBWAVdesc(COMMAND_T*)
{
	WDL_TypedBuf<MediaItem*> selectedItems;
	SWS_GetSelectedMediaItems(&selectedItems);
	if (selectedItems.GetSize() != 1)
	{
		MessageBox(g_hwndParent, __LOCALIZE("Please select exactly one item.","sws_mbox"), __LOCALIZE("Xenakios - Error","sws_mbox"), MB_OK);
		return;
	}
	
	MediaItem_Take* take = GetMediaItemTake(selectedItems.Get()[0], -1);
	if (!take)
	{
		MessageBox(g_hwndParent, __LOCALIZE("Active take is empty.","sws_mbox"), __LOCALIZE("Xenakios - Error","sws_mbox"), MB_OK);
		return;
	}
	
	PCM_source* pSrc = (PCM_source*)GetSetMediaItemTakeInfo(take, "P_SOURCE", NULL);
	if (pSrc)
	{
		char buf[8192];
		int sz = pSrc->Extended(PCM_SOURCE_EXT_GETMETADATA, (void*)"DESC", buf, (void*)(INT_PTR)(int)sizeof(buf));
		if (sz > 0 && buf[0])
		{
			string RPPFileName;
			string RPPdesc;
			RPPdesc.assign(buf);
			RPPFileName = RPPdesc.substr(4, RPPdesc.size());
			if (FileExists(RPPFileName.c_str()))
			{
				char RPPFileNameBuf[1024];
				snprintf(RPPFileNameBuf, sizeof(RPPFileNameBuf), "%s\\reaper.exe \"%s\"", GetExePath(), RPPFileName.c_str());
				if (!DoLaunchExternalTool(RPPFileNameBuf))
					MessageBox(g_hwndParent, __LOCALIZE("Could not launch REAPER.","sws_mbox"), __LOCALIZE("Xenakios - Error","sws_mbox"), MB_OK);
			}
			else
			{
				bool bRppFound = false;
				char RppSearchFolderName[1024];
				if (BrowseForDirectory(__LOCALIZE("Select folder with RPP files","sws_mbox"), NULL, RppSearchFolderName, 1024))
				{
					vector<string> FoundRPPs;
					SearchDirectory(FoundRPPs, RppSearchFolderName, "RPP", true);
					vector<string> FnCompos;
					SplitFileNameComponents(RPPFileName, FnCompos);
					for (int  i = 0; i < (int)FoundRPPs.size(); i++)
					{
						int x = (int)FoundRPPs[i].find(FnCompos[1]);
						if (x != string::npos)
						{
							char RPPFileNameBuf[1024];
							snprintf(RPPFileNameBuf, sizeof(RPPFileNameBuf), "%s\\reaper.exe \"%s\"", GetExePath(), FoundRPPs[i].c_str());
							if (!DoLaunchExternalTool(RPPFileNameBuf))
								MessageBox(g_hwndParent, __LOCALIZE("Could not launch REAPER.","sws_mbox"), __LOCALIZE("Xenakios - Error","sws_mbox"), MB_OK);
							bRppFound = true;
							break;
						}

					}
					if (!bRppFound)
						MessageBox(g_hwndParent, __LOCALIZE("RPP was not found in selected folder.","sws_mbox"), __LOCALIZE("Xenakios - Error","sws_mbox"), MB_OK);
				}
			}
		}
		else
			MessageBox(g_hwndParent, __LOCALIZE("No BWAV info found in the active take.","sws_mbox"), __LOCALIZE("Xenakios - Error","sws_mbox"), MB_OK);
	}
}

class section_source_data
{
public:
	virtual void foo()=0;
	double m_length WDL_FIXALIGN;
	double m_startpos;
	double m_edgeoverlap_time;
	PCM_source *m_srcs[2];
	void *m_itemptr;
	void *m_takeptr;
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

void DoNudgeSectionLoopStartPlus(COMMAND_T* c)
{
	PerformSectionLoopNudge(0,g_command_params.SectionLoopNudgeSecs); // 1 for section loop len
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(c),4,-1);
}

void DoNudgeSectionLoopStartMinus(COMMAND_T* c)
{
	PerformSectionLoopNudge(0,-g_command_params.SectionLoopNudgeSecs); // 1 for section loop len
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(c),4,-1);
}

void DoNudgeSectionLoopLenPlus(COMMAND_T* c)
{
	PerformSectionLoopNudge(1,g_command_params.SectionLoopNudgeSecs); // 1 for section loop len
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(c),4,-1);
}

void DoNudgeSectionLoopLenMinus(COMMAND_T* c)
{
	PerformSectionLoopNudge(1,-g_command_params.SectionLoopNudgeSecs); // 1 for section loop len
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(c),4,-1);
}

void DoNudgeSectionLoopOlapPlus(COMMAND_T* c)
{
	PerformSectionLoopNudge(2,g_command_params.SectionLoopNudgeSecs); // 2 for section loop overlap
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(c),4,-1);
}

void DoNudgeSectionLoopOlapMinus(COMMAND_T* c)
{
	PerformSectionLoopNudge(2,-g_command_params.SectionLoopNudgeSecs); // 2 for section loop overlap
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(c),4,-1);
}
