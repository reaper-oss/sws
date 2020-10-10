/******************************************************************************
/ TrackTemplateActions.cpp
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

#include <WDL/localize/localize.h>

using namespace std;

void SplitFileNameComponents(string FullFileName,vector<string>& FNComponents)
{
	FNComponents.clear();
	string FileNameWithExt;
	string JustFileName;
	string JustExtension;
	string JustPathToFile;

	size_t iLastSlash = FullFileName.find_last_of(PATH_SLASH_CHAR);
	if (iLastSlash != string::npos)
	{
		FileNameWithExt = FullFileName.substr(iLastSlash + 1);
		JustPathToFile  = FullFileName.substr(0, iLastSlash + 1);
	}
	else
		FileNameWithExt = FullFileName;

	size_t iLastDot = FileNameWithExt.find_last_of(".");
	if (iLastDot != string::npos)
	{
		JustExtension = FileNameWithExt.substr(iLastDot);
		JustFileName = FileNameWithExt.substr(0, iLastDot);
	}
	else
		JustFileName = FileNameWithExt;

	FNComponents.push_back(JustPathToFile);
	FNComponents.push_back(JustFileName);
	FNComponents.push_back(JustExtension);
}

void DoOpenTemplate(int iNum, bool bProject)
{
	char cPath[BUFFER_SIZE];
	snprintf(cPath, BUFFER_SIZE, "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, bProject ? "ProjectTemplates" : "TrackTemplates");
	vector<string> templates;
	SearchDirectory(templates, cPath, bProject ? "RPP" : "RTRACKTEMPLATE", true);
	if (templates.size() == 0)
	{
		MessageBox(g_hwndParent, __LOCALIZE("No templates at all were found!","sws_mbox"),__LOCALIZE("Xenakios - Error","sws_mbox"), MB_OK);
		return;
	}
	double runningReaVersion = atof(GetAppVersion());
	for (int i = 0; i < (int)templates.size(); i++)
	{
		const char* pFilename = strrchr(templates[i].c_str(), PATH_SLASH_CHAR);
		if (pFilename && pFilename[1] && iNum == atol(pFilename+1))
		{
			//  Main_openProject() supports noprompt: and template: prefixes since R5.983
			if (bProject && (runningReaVersion >= 5.983))
				templates[i].insert(0, "template:");
			Main_openProject((char*)templates[i].c_str());
			return;
		}
	}
	MessageBox(g_hwndParent, __LOCALIZE("No matching template found! Please name your template starting with a number.","sws_mbox"),__LOCALIZE("Xenakios - Error","sws_mbox"), MB_OK);
}

void DoOpenTrackTemplate(COMMAND_T* t)
{
	DoOpenTemplate((int)t->user, false);
}

void DoOpenProjectTemplate(COMMAND_T* t)
{
	DoOpenTemplate((int)t->user, true);
}
