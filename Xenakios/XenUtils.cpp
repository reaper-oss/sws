/******************************************************************************
/ XenUtils.cpp
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

// Globals
WDL_PtrList<CHAR>* g_filenames;

int GetActiveTakes(WDL_PtrList<MediaItem_Take> *MediaTakes)
{
	MediaTrack* CurTrack;
	MediaItem* CurItem;
	MediaItem_Take* CurTake;
	int TakeCount=0;
	int trackID;
	int itemID;
	for (trackID=0;trackID<GetNumTracks();trackID++)
	{
		CurTrack=CSurf_TrackFromID(trackID+1,false);
		int numItems=GetTrackNumMediaItems(CurTrack);
		for (itemID=0;itemID<numItems;itemID++)
		{
			CurItem = GetTrackMediaItem(CurTrack,itemID);
			bool ItemSelected=*(bool*)GetSetMediaItemInfo(CurItem,"B_UISEL",NULL);
			if (ItemSelected) 
			{ 
				if (GetMediaItemNumTakes(CurItem))
				{
					int currentTakeIndex=*(int*)GetSetMediaItemInfo(CurItem,"I_CURTAKE",NULL);
					CurTake=GetMediaItemTake(CurItem,currentTakeIndex);
					MediaTakes->Add(CurTake);
					TakeCount++;
				}
			}
		}
	}
	return TakeCount;	
}

// Returns number of files found with desired extension
// if extension is == NULL, looks for Media Files with Reaper's builtin IsMediaExtension
char g_CurrentScanFile[1024] = "";
bool g_bAbortScan = false;
int SearchDirectory(vector<string> &refvecFiles, const char* cDir, const char* cExt, bool bSubdirs)
{
	WDL_DirScan ds;
	int iRet = ds.First(cDir);
	int found = 0;
	g_bAbortScan = false;
	if (!iRet)
	{
		do
		{
			if (strcmp(ds.GetCurrentFN(), ".") == 0 || strcmp(ds.GetCurrentFN(), "..") == 0)
				continue;
			WDL_String foundFile;
			ds.GetCurrentFullFN(&foundFile);
			lstrcpyn(g_CurrentScanFile, foundFile.Get(), 1024);
			if (bSubdirs && ds.GetCurrentIsDirectory())
			{
				found += SearchDirectory(refvecFiles, foundFile.Get(), cExt, true);
			}
			else
			{
				char* cFoundExt  = strrchr(foundFile.Get(), '.');
				if (cFoundExt)
				{
					cFoundExt++;
					if ((!cExt && IsMediaExtension(cFoundExt, false)) || (cExt && _stricmp(cFoundExt, cExt) == 0))
					{
						refvecFiles.push_back(foundFile.Get());
						found++;
					}
				}
			}
		}
		while(!ds.Next() && !g_bAbortScan);
	}
	return found;
}

#ifdef _WIN32

char *BrowseForFiles(const char *text, const char *initialdir, const char *initialfile, bool allowmul, const char *extlist)
{
	int rv = 0;
	int temp_size = allowmul ? (MAX_PATH * 512) : MAX_PATH * 2;
	char *temp=(char *)malloc(temp_size);
	if (!temp)
		return NULL;

	memset(temp, 0, temp_size);

	OPENFILENAME l = {sizeof(l),};
	l.hwndOwner = g_hwndParent;
	l.lpstrFilter = extlist;
	l.lpstrFile = temp;
	l.nMaxFile = temp_size;
	l.lpstrTitle = text;
	l.lpstrDefExt = extlist;
	l.lpstrInitialDir = initialdir;
	l.Flags = OFN_DONTADDTORECENT | OFN_EXPLORER | (allowmul ? OFN_ALLOWMULTISELECT : 0) | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&l)) 
		return temp;

	free(temp);
	return NULL;
}

bool BrowseForSaveFile(const char *text, const char *initialdir, const char *initialfile, const char *extlist, char *fn, int fnsize)
{
	if (initialfile)
		lstrcpyn(fn, initialfile, fnsize);
	else
		memset(fn,0,fnsize);

	OPENFILENAME l={sizeof(l),};
	l.hwndOwner = g_hwndParent;
	l.lpstrFilter = extlist;
	l.lpstrFile = fn;
	l.nMaxFile = fnsize-1;
	l.lpstrTitle = text;
	l.lpstrDefExt = extlist;
	l.lpstrInitialDir = initialdir;
	
	l.Flags = OFN_DONTADDTORECENT | OFN_HIDEREADONLY | OFN_EXPLORER;

	if (GetSaveFileName(&l))
		return true;
	else
		return false;	
}

bool BrowseForDirectory(const char *text, const char *initialdir, char *fn, int fnsize)
{
	BROWSEINFO bi;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = NULL;
	bi.hwndOwner = g_hwndParent;
	bi.lpszTitle = text;
	bi.ulFlags = BIF_NEWDIALOGSTYLE;
	bi.lpfn = NULL;
	bi.iImage = 0;

	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
	if (pidl)
	{
		char path[MAX_PATH];
		BOOL bRet = SHGetPathFromIDList(pidl, path);
		CoTaskMemFree(pidl);
		if (bRet)
		{
			lstrcpyn(fn, path, fnsize);
			return true;
		}
	}
	return false;
}

#endif

bool FileExists(const char* file)
{
#ifdef _WIN32
	struct _stat s;
	return _stat(file, &s) == 0;
#else
	struct stat s;
	return stat(file, &s) == 0;
#endif
}
