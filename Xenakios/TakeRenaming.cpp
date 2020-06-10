/******************************************************************************
/ TakeRenaming.cpp
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
#include "../reaper/localize.h"
#include "../Breeder/BR_MidiUtil.h" // IsMidi()
#include "../SnM/SnM_Dlg.h"	
#include "../SnM/SnM_Util.h" // SNM_DeletePeakFile()

using namespace std;

struct t_renameparams
{
	int mode; // 0 for rename take, 1 for rename media source, 2 for both
	string OldName;
	string NewName;
	int DialogRC;
	int takesToRename;
	int curTakeInx;
	bool batchnaming;

};

t_renameparams g_renameparams = {};

WDL_DLGRET RenameDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	char buf[2048];
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, Message, wParam, lParam))
		return r;
	
	if (Message == WM_INITDIALOG)
	{
		if (g_renameparams.mode==0 && !g_renameparams.batchnaming)
			sprintf(buf,"Rename take [%d / %d]", g_renameparams.curTakeInx, g_renameparams.takesToRename);		
		else if (g_renameparams.mode==0 && g_renameparams.batchnaming)
			sprintf(buf,"Rename %d takes", g_renameparams.takesToRename);
		else if (g_renameparams.mode==1)
			sprintf(buf,"Rename take source file [%d / %d]",g_renameparams.curTakeInx, g_renameparams.takesToRename);
		else if (g_renameparams.mode == 2)
			sprintf(buf,"Rename take and source file [%d / %d]",g_renameparams.curTakeInx, g_renameparams.takesToRename);

		SetWindowText(hwnd, buf);

		SetDlgItemText(hwnd,IDC_EDIT1, g_renameparams.OldName.c_str());
		HWND hEdit = GetDlgItem(hwnd, IDC_EDIT1);
		SetFocus(hEdit);
		SendMessage(hEdit, EM_SETSEL, 0, -1);
	}
	else if (Message == WM_COMMAND && LOWORD(wParam) == IDCANCEL)
	{
		g_renameparams.DialogRC = 1;
		EndDialog(hwnd, 0);
	}
	else if (Message == WM_COMMAND && LOWORD(wParam) == IDOK)
	{
		GetDlgItemText(hwnd, IDC_EDIT1, buf, 2047);
		if (strlen(buf) > 0)
		{
			g_renameparams.NewName.assign(buf);
			g_renameparams.DialogRC = 0;
			EndDialog(hwnd, 0);
		}
		else
		{
			MessageBox(hwnd, __LOCALIZE("Empty filename!","sws_mbox"), __LOCALIZE("Xenakios - Error","sws_mbox"), MB_OK);
			g_renameparams.DialogRC = 1;
		}
	}
	return 0;
}

#ifdef _XEN_LOG
ofstream *g_renamelogstream=0;
#endif

void AddToRenameLog(string &oldfilename,string &newfilename)
{
#ifdef _XEN_LOG
	if (!g_renamelogstream)
	{
		//std::ofstream os(OutFileName);	
		char logfilename[1024];
		snprintf(logfilename,sizeof(logfilename),"%s/Plugins/XC_Rename.log",GetExePath());
		g_renamelogstream=new ofstream(logfilename,ios_base::app);
	}
	if (g_renamelogstream)
		*g_renamelogstream << "\"" << oldfilename << "\" <renamed_to> \"" << newfilename << "\"" << endl;
#endif
}

void DoRenameTakeAndSourceFileDialog(COMMAND_T* ct)
{
	vector<MediaItem_Take*> thetakes;
	vector<MediaItem_Take*> alltakes;
	XenGetProjectTakes(alltakes,false,false);
	XenGetProjectTakes(thetakes,true,true);
	if (thetakes.size()==0) return;

	// store (ueser-)offlined takes so we can restore offlined state later
	vector<MediaItem_Take*> offlinedTakes;
	for (size_t i = 0; i < alltakes.size(); i++)
	{
		MediaItem_Take* take = alltakes[i];
		PCM_source* thesrc = (PCM_source*)GetSetMediaItemTakeInfo(take, "P_SOURCE", 0);
		if (thesrc && !thesrc->IsAvailable())
			offlinedTakes.push_back(take);
	}

	// store files which can't be renamed for displaying an error message
	vector<string> failedFiles;

	g_renameparams.takesToRename=(int)thetakes.size();
	if (ct->user == 0)// rename media source only
		g_renameparams.mode = 1;
	else if (ct->user == 1) // rename media source and take
		g_renameparams.mode = 2;
	bool takesRenamedOnly = true;
	bool isMIDItake, isInProjectMIDI = false;;

	// make sure all file handles created by REAPER are closed before renaming the file
	Main_OnCommand(40100, 0); // offline all media
	for (int i=0;i<(int)thetakes.size();i++)
	{
		PCM_source *thesrc = (PCM_source*)GetSetMediaItemTakeInfo(thetakes[i],"P_SOURCE",0);
		// skip if only renaming source and take is in-project MIDI (would have no effect)
		isMIDItake = IsMidi(thetakes[i], &isInProjectMIDI);
		if (ct->user == 0 && isMIDItake && isInProjectMIDI)
			thesrc = nullptr;
		if (thesrc)
		{
			char *oldtakename=(char*)GetSetMediaItemTakeInfo(thetakes[i],"P_NAME",0);
			g_renameparams.OldName.assign(oldtakename);
			g_renameparams.curTakeInx=i+1;
			DialogBox(g_hInst,MAKEINTRESOURCE(IDD_RENAMEDLG666), g_hwndParent, (DLGPROC)RenameDlgProc);
			if (g_renameparams.DialogRC==1)
				break;
			if (g_renameparams.DialogRC==0)
			{
				// Can only do the filename if it exists
				if (GetSourceFilename(thesrc) && !isInProjectMIDI)
				{
					string oldname;
					oldname.assign(GetSourceFilename(thesrc));
					vector<string> fnsplit;
					SplitFileNameComponents(oldname,fnsplit);
					string newfilename;
					newfilename.append(fnsplit[0]);
					newfilename.append(g_renameparams.NewName);

					// only append file extension if not already contained in new filename, #1140
					vector<string> newfnsplit;
					SplitFileNameComponents(newfilename, newfnsplit);
					if (newfnsplit[2].compare(fnsplit[2])!=0)
						newfilename.append(fnsplit[2]);

					
					if (!MoveFile(oldname.c_str(), newfilename.c_str()))
					{
						failedFiles.push_back(oldname);
						continue; // don't rename source if file can't be renamed, would cause mismatch
					}
					takesRenamedOnly = false;
					AddToRenameLog(oldname,newfilename);

					for (int j=0;j<(int)alltakes.size();j++)
					{
						PCM_source *thesrc = (PCM_source*)GetSetMediaItemTakeInfo(alltakes[j],"P_SOURCE",0);
						if (thesrc && GetSourceFilename(thesrc))
						{
							string fname;
							fname.assign(GetSourceFilename(thesrc));
							if (oldname.compare(fname)==0)
							{
								{	
									thesrc->SetFileName(newfilename.c_str());
									if (ct->user == 1) // also rename takes
										GetSetMediaItemTakeInfo(alltakes[j],"P_NAME",(char*)g_renameparams.NewName.c_str());
									SNM_DeletePeakFile(oldname.c_str(), true); // no delete check (peaks files can be absent)
								}
							}
						}
					}
				}
				else if (ct->user == 1)
					GetSetMediaItemTakeInfo(thetakes[i], "P_NAME", (char*)g_renameparams.NewName.c_str());
			}
		}
	}
	Main_OnCommand(40101, 0); // online all media

	// restore offlined takes
	for (size_t i = 0; i < offlinedTakes.size(); i++)
	{
		MediaItem_Take* take = offlinedTakes[i];
		PCM_source* thesrc = (PCM_source*)GetSetMediaItemTakeInfo(take, "P_SOURCE", 0);
		thesrc->SetAvailable(false);
	}

	// only create undo point if only takes are renamed
	// because undoing rename source filename wouldn't work (as we can't undo renaming the actual file)
	if (takesRenamedOnly)
		Undo_OnStateChangeEx("Rename takes", 4, -1);
	Main_OnCommand(40047, 0); // build any missing peaks
	UpdateTimeline();

	// display error message if files couldn't be renamed
	if (failedFiles.size())
	{
		std::ostringstream ss;
		ss << "The following files couldn't be renamed (maybe they're open in another application?):\n";
		for (size_t i = 0; i < failedFiles.size(); i++)
		{
			if (i > 0)
				ss << "\n";
			ss << failedFiles[i];
		}
		MessageBox(g_hwndParent, ss.str().c_str(), __LOCALIZE("Rename source files - Error", "sws_mbox"), MB_OK);
	}
}

void DoRenameTakeDialog666(COMMAND_T* ct)
{
	vector<MediaItem_Take*> thetakes;
	XenGetProjectTakes(thetakes,true,true);
	if (thetakes.size()==0) return;
	g_renameparams.batchnaming=false;
	g_renameparams.takesToRename=(int)thetakes.size();
	g_renameparams.mode=0;
	for (int i = 0; i < (int)thetakes.size(); i++)
	{
		char *oldtakename=(char*)GetSetMediaItemTakeInfo(thetakes[i],"P_NAME",0);
		g_renameparams.OldName.assign(oldtakename);
		g_renameparams.curTakeInx=i+1;
		DialogBox(g_hInst,MAKEINTRESOURCE(IDD_RENAMEDLG666), g_hwndParent, (DLGPROC)RenameDlgProc);
		if (g_renameparams.DialogRC==0)
			GetSetMediaItemTakeInfo(thetakes[i],"P_NAME",(char*)g_renameparams.NewName.c_str());
		if (g_renameparams.DialogRC==1)
			break;
	}
	UpdateTimeline();
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),4,-1);
}

void DoRenameTakeAllDialog666(COMMAND_T* ct)
{
	vector<MediaItem_Take*> thetakes;
	XenGetProjectTakes(thetakes,true,true);
	if (thetakes.size()==0) return;
	g_renameparams.takesToRename=(int)thetakes.size();
	g_renameparams.mode=0;
	g_renameparams.batchnaming=true;
	g_renameparams.OldName.assign(__LOCALIZE("New take name","sws_DLG_144"));
	DialogBox(g_hInst,MAKEINTRESOURCE(IDD_RENAMEDLG666), g_hwndParent, (DLGPROC)RenameDlgProc);
	if (g_renameparams.DialogRC==0)
	{
		for (int i = 0; i < (int)thetakes.size(); i++)
		{
			if (g_renameparams.DialogRC==0)
				GetSetMediaItemTakeInfo(thetakes[i],"P_NAME",(char*)g_renameparams.NewName.c_str());
		}
		UpdateTimeline();
		Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct),4,-1);
	}
}
