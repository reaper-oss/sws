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

string GetDialogItemString(HWND hDlg,int ItemID)
{
	char buf[2048];
	string result;
	GetDlgItemText(hDlg,ItemID,buf,2047);
	result.assign(buf);
	return result;	
}

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

