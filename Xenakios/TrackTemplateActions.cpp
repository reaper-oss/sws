/******************************************************************************
/ TrackTemplateActions.cpp
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

void SplitFileNameComponents(string FullFileName,vector<string>& FNComponents)
{
	FNComponents.clear();
	size_t LastBackSlashIndex=FullFileName.find_last_of("\\");
	size_t LastFwsSlashIndex=FullFileName.find_first_of("/");
	string FileNameWithExt=FullFileName.substr(LastBackSlashIndex+1,FullFileName.size()-LastBackSlashIndex);
	string JustPathToFile=FullFileName.substr(0,LastBackSlashIndex+1);
	size_t LastDotIndex=FileNameWithExt.find_last_of(".");
	string JustFileName=FileNameWithExt.substr(0,LastDotIndex);
	string JustExtension=FileNameWithExt.substr(LastDotIndex,FileNameWithExt.size());
	size_t FirstBackSlashIndex=FullFileName.find_first_of("\\");
	string JustDriveRoot=FullFileName.substr(0,FirstBackSlashIndex+1);
	FNComponents.push_back(JustPathToFile);
	FNComponents.push_back(JustFileName);
	FNComponents.push_back(JustExtension);
}

void DoOpenTrackTemplate(COMMAND_T* t)
{
	char templateFNbeginswith[10];
	sprintf(templateFNbeginswith, "%02d", t->user);
	vector<string> blah;
	string IniFileLoc;
	IniFileLoc.assign(get_ini_file());
	SplitFileNameComponents(IniFileLoc,blah);
	string TemplatesFolder;
	TemplatesFolder.assign(blah[0].c_str());
	TemplatesFolder.append("TrackTemplates");
	vector<string> Filut;
	SearchDirectory(Filut, TemplatesFolder.c_str(), "RTRACKTEMPLATE", true);
	if (Filut.size()==0)
	{
		MessageBox(g_hwndParent,"No templates at all were found!","Error",MB_OK);
		return;
	}
	int i;
	for (i=0;i<(int)Filut.size();i++)
	{
		SplitFileNameComponents(Filut[i],blah);
		if (strncmp(blah[1].c_str(), templateFNbeginswith, 2)==0)
		{
			Main_openProject((char*)Filut[i].c_str());
			return;
		}
	}
	MessageBox(g_hwndParent,"No matching template found!","Error",MB_OK);
}

void DoOpenProjectTemplate(COMMAND_T* t)
{
	char templateFNbeginswith[10];
	sprintf(templateFNbeginswith, "%02d", t->user);
	vector<string> blah;
	string IniFileLoc;
	IniFileLoc.assign(get_ini_file());
	SplitFileNameComponents(IniFileLoc,blah);
	string TemplatesFolder;
	TemplatesFolder.assign(blah[0].c_str());
	TemplatesFolder.append("ProjectTemplates");
	vector<string> Filut;
	SearchDirectory(Filut, TemplatesFolder.c_str(), "RPP", true);
	if (Filut.size()==0)
	{
		MessageBox(g_hwndParent,"No project templates at all were found!","Error",MB_OK);
		return;
	}
	int i;
	for (i=0;i<(int)Filut.size();i++)
	{
		SplitFileNameComponents(Filut[i],blah);
		if (strncmp(blah[1].c_str(), templateFNbeginswith, 2)==0)
		{
			Main_openProject((char*)Filut[i].c_str());
			return;
		}
	}
	MessageBox(g_hwndParent,"No matching project template found!","Error",MB_OK);
}