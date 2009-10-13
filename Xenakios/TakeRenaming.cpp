/******************************************************************************
/ TakeRenaming.cpp
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

typedef struct 
{
	int mode; // 0 for rename take, 1 for rename media source, 2 for both, 666 for ?
	string OldFileName;
	string OldTakeName;
	string NewFileName;
	string NewTakeName;
	int DialogRC;
	int takesToRename;
	int curTakeInx;
	bool batchnaming;

} t_renameparams;

t_renameparams g_renameparams;

BOOL WINAPI RenameDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	char buf[2048];
	if (Message==WM_INITDIALOG)
	{
		if (g_renameparams.mode==0)
		{
			if (!g_renameparams.batchnaming)
				sprintf(buf,"Rename take [%d / %d]",g_renameparams.curTakeInx,g_renameparams.takesToRename);		
			else sprintf(buf,"Rename %d takes",g_renameparams.takesToRename);
			SetDlgItemText(hwnd,IDC_EDIT1,g_renameparams.OldTakeName.c_str());
		}
		if (g_renameparams.mode==1)
		{
			sprintf(buf,"Rename take source file [%d / %d]",g_renameparams.curTakeInx,g_renameparams.takesToRename);
			SetDlgItemText(hwnd,IDC_EDIT1,g_renameparams.OldFileName.c_str());
		}
		SetWindowText(hwnd,buf);
		
		
		SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwnd, IDC_EDIT1), TRUE);
		return 0;
	}
	if (Message==WM_COMMAND && LOWORD(wParam)==IDCANCEL)
	{
		g_renameparams.DialogRC=1;
		EndDialog(hwnd,0);
		return 0;
	}
	if (Message==WM_COMMAND && LOWORD(wParam)==IDOK)
	{
		GetDlgItemText(hwnd,IDC_EDIT1,buf,2047);
		if (strlen(buf)>0)
		{
		if (g_renameparams.mode==0)
		{
			g_renameparams.NewTakeName.assign(buf);
		}
		if (g_renameparams.mode==1)
		{
			g_renameparams.NewFileName.assign(buf);
			
		}
		if (g_renameparams.mode==2)
		{
			g_renameparams.NewFileName.assign(buf);
			
		}
		g_renameparams.DialogRC=0;
		EndDialog(hwnd,0);
		return 0;
		} else
		{
			MessageBox(hwnd,"New file name empty","Error",MB_OK);
			g_renameparams.DialogRC=1;
		}
		
		return 0;
	}
	return 0;
}

ofstream *g_renamelogstream=0;

void AddToRenameLog(string &oldfilename,string &newfilename)
{
	if (!g_renamelogstream)
	{
		//std::ofstream os(OutFileName);	
		char logfilename[1024];
		sprintf(logfilename,"%s/Plugins/XC_Rename.log",GetExePath());
		g_renamelogstream=new ofstream(logfilename,ios_base::app);
	}
	if (g_renamelogstream)
		*g_renamelogstream << "\"" << oldfilename << "\" <renamed_to> \"" << newfilename << "\"" << endl;
}

void DoRenameSourceFileDialog666(COMMAND_T*)
{
	vector<MediaItem_Take*> thetakes;
	vector<MediaItem_Take*> alltakes;
	XenGetProjectTakes(alltakes,false,false);
	XenGetProjectTakes(thetakes,true,true);
	if (thetakes.size()==0) return;
	g_renameparams.takesToRename=(int)thetakes.size();
	g_renameparams.mode=1;
	int i;
	for (i=0;i<(int)thetakes.size();i++)
	{
		
		PCM_source *thesrc=(PCM_source*)GetSetMediaItemTakeInfo(thetakes[i],"P_SOURCE",0);
		if (strcmp(thesrc->GetType(),"SECTION")!=0)
		{
		g_renameparams.curTakeInx=i+1;
		string oldname;
		oldname.assign(thesrc->GetFileName());
		vector<string> fnsplit;
		SplitFileNameComponents(oldname,fnsplit);
		g_renameparams.OldFileName.assign(fnsplit[1]);

		DialogBox(g_hInst,MAKEINTRESOURCE(IDD_RENAMEDLG666), g_hwndParent, (DLGPROC)RenameDlgProc);
		if (g_renameparams.DialogRC==1)
		{
			break;
		}
		if (g_renameparams.DialogRC==0)
		{
			Main_OnCommand(40100,0); // offline all media
			string newfilename;
			newfilename.append(fnsplit[0]);
			newfilename.append(g_renameparams.NewFileName);
			newfilename.append(fnsplit[2]);
			MoveFile(oldname.c_str(),newfilename.c_str());
			AddToRenameLog(oldname,newfilename);
			int j;
			for (j=0;j<(int)alltakes.size();j++)
			{
				PCM_source *thesrc=(PCM_source*)GetSetMediaItemTakeInfo(alltakes[j],"P_SOURCE",0);
				if (strcmp(thesrc->GetType(),"SECTION")!=0)
				{
					string fname;
					fname.assign(thesrc->GetFileName());
					if (oldname.compare(fname)==0)
					{
						PCM_source *newsrc=PCM_Source_CreateFromFile(newfilename.c_str());
						GetSetMediaItemTakeInfo(alltakes[j],"P_SOURCE",newsrc);
						
						delete thesrc;
					}
				}
			}
		}
			Main_OnCommand(40047,0); // build any missing peaks
			Main_OnCommand(40101,0); // online all media
			
			
		}
	}
	UpdateTimeline();
	Undo_OnStateChangeEx("Rename take source file (doesn't really undo file rename!!!)",4,-1);	
}

void DoRenameTakeAndSourceFileDialog(COMMAND_T*)
{
	vector<MediaItem_Take*> thetakes;
	vector<MediaItem_Take*> alltakes;
	XenGetProjectTakes(alltakes,false,false);
	XenGetProjectTakes(thetakes,true,true);
	if (thetakes.size()==0) return;
	g_renameparams.takesToRename=(int)thetakes.size();
	g_renameparams.mode=1;
	int i;
	for (i=0;i<(int)thetakes.size();i++)
	{
		
		PCM_source *thesrc=(PCM_source*)GetSetMediaItemTakeInfo(thetakes[i],"P_SOURCE",0);
		if (strcmp(thesrc->GetType(),"SECTION")!=0)
		{
			g_renameparams.curTakeInx=i+1;
			string oldname;
			oldname.assign(thesrc->GetFileName());
			vector<string> fnsplit;
			SplitFileNameComponents(oldname,fnsplit);
			g_renameparams.OldFileName.assign(fnsplit[1]);

			DialogBox(g_hInst,MAKEINTRESOURCE(IDD_RENAMEDLG666), g_hwndParent, (DLGPROC)RenameDlgProc);
			if (g_renameparams.DialogRC==1)
			{
				break;
			}
			if (g_renameparams.DialogRC==0)
			{
				Main_OnCommand(40100,0); // offline all media
				string newfilename;
				newfilename.append(fnsplit[0]);
				newfilename.append(g_renameparams.NewFileName);
				newfilename.append(fnsplit[2]);
				MoveFile(oldname.c_str(),newfilename.c_str());
				AddToRenameLog(oldname,newfilename);
				int j;
				for (j=0;j<(int)alltakes.size();j++)
				{
					PCM_source *thesrc=(PCM_source*)GetSetMediaItemTakeInfo(alltakes[j],"P_SOURCE",0);
					if (strcmp(thesrc->GetType(),"SECTION")!=0)
					{
						string fname;
						fname.assign(thesrc->GetFileName());
						if (oldname.compare(fname)==0)
						{
							PCM_source *newsrc=PCM_Source_CreateFromFile(newfilename.c_str());
							GetSetMediaItemTakeInfo(alltakes[j],"P_SOURCE",newsrc);
							GetSetMediaItemTakeInfo(alltakes[j],"P_NAME",(char*)g_renameparams.NewFileName.c_str());
							delete thesrc;
						}
					}
				}
			}
			Main_OnCommand(40047,0); // build any missing peaks
			Main_OnCommand(40101,0); // online all media
		}
	}
	UpdateTimeline();
	Undo_OnStateChangeEx("Rename take(s) and source file(s) (doesn't really undo file rename!!!)",4,-1);
}

void DoRenameTakeDialog666(COMMAND_T*)
{
	vector<MediaItem_Take*> thetakes;
	//vector<MediaItem_Take*> alltakes;
	//XenGetProjectTakes(alltakes,false,false);
	XenGetProjectTakes(thetakes,true,true);
	if (thetakes.size()==0) return;
	g_renameparams.batchnaming=false;
	g_renameparams.takesToRename=(int)thetakes.size();
	g_renameparams.mode=0;
	int i;
	for (i=0;i<(int)thetakes.size();i++)
	{
		
		//PCM_source *thesrc=(PCM_source*)GetSetMediaItemTakeInfo(thetakes[i],"P_SOURCE",0);
		char *oldtakename=(char*)GetSetMediaItemTakeInfo(thetakes[i],"P_NAME",0);
		g_renameparams.OldTakeName.assign(oldtakename);
		g_renameparams.curTakeInx=i+1;
		DialogBox(g_hInst,MAKEINTRESOURCE(IDD_RENAMEDLG666), g_hwndParent, (DLGPROC)RenameDlgProc);
		if (g_renameparams.DialogRC==0)
			GetSetMediaItemTakeInfo(thetakes[i],"P_NAME",(char*)g_renameparams.NewTakeName.c_str());
		if (g_renameparams.DialogRC==1)
			break;
		//Main_OnCommand(40047,0); // build any missing peaks
		//Main_OnCommand(40101,0); // online all media
	}
	UpdateTimeline();
	Undo_OnStateChangeEx("Rename take(s)",4,-1);
}

void DoRenameTakeAllDialog666(COMMAND_T*)
{
	vector<MediaItem_Take*> thetakes;
	//vector<MediaItem_Take*> alltakes;
	//XenGetProjectTakes(alltakes,false,false);
	XenGetProjectTakes(thetakes,true,true);
	if (thetakes.size()==0) return;
	g_renameparams.takesToRename=(int)thetakes.size();
	g_renameparams.mode=0;
	g_renameparams.batchnaming=true;
	g_renameparams.OldTakeName.assign("New take name");
	DialogBox(g_hInst,MAKEINTRESOURCE(IDD_RENAMEDLG666), g_hwndParent, (DLGPROC)RenameDlgProc);
	int i;
	if (g_renameparams.DialogRC==0)
	{
		for (i=0;i<(int)thetakes.size();i++)
		{
			
			//PCM_source *thesrc=(PCM_source*)GetSetMediaItemTakeInfo(thetakes[i],"P_SOURCE",0);
			//char *oldtakename=(char*)GetSetMediaItemTakeInfo(thetakes[i],"P_NAME",0);
			//g_renameparams.OldTakeName.assign(oldtakename);

			//DialogBox(g_hInst,MAKEINTRESOURCE(IDD_RENAMEDLG666), g_hwndParent, RenameDlgProc);
			if (g_renameparams.DialogRC==0)
				GetSetMediaItemTakeInfo(thetakes[i],"P_NAME",(char*)g_renameparams.NewTakeName.c_str());
			//Main_OnCommand(40047,0); // build any missing peaks
			//Main_OnCommand(40101,0); // online all media
		}
		UpdateTimeline();
		Undo_OnStateChangeEx("Rename take(s)",4,-1);
	}
}