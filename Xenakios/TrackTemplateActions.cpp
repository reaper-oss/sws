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

int SearchDirectoryForTrackTemplates(vector<string> &refvecFiles,
                    const string        &refcstrRootDirectory,
                    const string        &refcstrExtension,
                    bool                     bSearchSubdirectories = true)
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
        strFilePath.erase();
		strFilePath = refcstrRootDirectory + "\\" + FileInformation.cFileName;
        if(FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
          if(bSearchSubdirectories)
          {
            // Search subdirectory
            int iRC = SearchDirectoryForTrackTemplates(refvecFiles,
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
			transform(strExtension.begin(), strExtension.end(), strExtension.begin(), (int(*)(int)) toupper);	
			//transform(refcstrExtension.begin(), refcstrExtension.end(), refcstrExtension.begin(), (int(*)(int)) toupper);
			string paskaKopio;
			paskaKopio=refcstrExtension;
			transform(paskaKopio.begin(), paskaKopio.end(), paskaKopio.begin(), (int(*)(int)) toupper);
		int extMatch=0;
		if (strExtension=="RTRACKTEMPLATE")
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
	SearchDirectoryForTrackTemplates(Filut,TemplatesFolder,"jeesus",true);
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

int SearchDirectoryForProjectTemplates(vector<string> &refvecFiles,
                    const string        &refcstrRootDirectory,
                    const string        &refcstrExtension,
                    bool                     bSearchSubdirectories = true)
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
        strFilePath.erase();
		strFilePath = refcstrRootDirectory + "\\" + FileInformation.cFileName;
        if(FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
          if(bSearchSubdirectories)
          {
            // Search subdirectory
            int iRC = SearchDirectoryForTrackTemplates(refvecFiles,
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
			transform(strExtension.begin(), strExtension.end(), strExtension.begin(), (int(*)(int)) toupper);	
			//transform(refcstrExtension.begin(), refcstrExtension.end(), refcstrExtension.begin(), (int(*)(int)) toupper);
			string paskaKopio;
			paskaKopio=refcstrExtension;
			transform(paskaKopio.begin(), paskaKopio.end(), paskaKopio.begin(), (int(*)(int)) toupper);
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
	SearchDirectoryForProjectTemplates(Filut,TemplatesFolder,"jeesus",true);
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