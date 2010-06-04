/******************************************************************************
/ SnM_Misc.cpp
/
/ Copyright (c) 2009-2010 Tim Payne (SWS), JF Bédague 
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
#include "SnM_Actions.h"
#include "SNM_ChunkParserPatcher.h"


#ifdef _SNM_MISC

bool isLoopOrInProjectTakes(MediaItem* _item, int _take)
{
	if (_item)
	{
		PCM_source* srcTake = 
			(PCM_source*)GetSetMediaItemTakeInfo(GetMediaItemTake(_item, _take), "P_SOURCE", NULL);

		// in-project ?
		if (srcTake && !srcTake->GetFileName())
			return true;

		// look to -1, +1 rather than looking to all takes
		// => manage the case a mess has been introduced between looped takes
		if (_take > 0)
		{
			PCM_source* src = 
				(PCM_source*)GetSetMediaItemTakeInfo(GetMediaItemTake(_item, _take-1), "P_SOURCE", NULL);
			if (src && src->GetFileName() && 
				!strcmp(src->GetFileName(), srcTake->GetFileName()))
				return true;
		}
		if (_take < (CountTakes(_item)-1))
		{
			PCM_source* src = 
				(PCM_source*)GetSetMediaItemTakeInfo(GetMediaItemTake(_item, _take+1), "P_SOURCE", NULL);
			if (src && src->GetFileName() && 
				!strcmp(src->GetFileName(), srcTake->GetFileName()))
				return true;
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// Select item by name
///////////////////////////////////////////////////////////////////////////////
bool selectItemsByName(const char* _undoMsg, char* _name)
{
	bool update = false;
	if (CountMediaItems && GetMediaItem && _name && strlen(_name) > 0)
	{
		for (int i=0; i < CountMediaItems(0); i++)
		{
			MediaItem* item = GetMediaItem(0, i);
			if (item)
			{
				for (int j=0; j < GetMediaItemNumTakes(item); j++)
				{
					MediaItem_Take* t = GetMediaItemTake(item, j);
					if (t)
					{
						update = true;
						char* takeName =
							(char*)GetSetMediaItemTakeInfo(t, "P_NAME", NULL);
						bool sel = takeName && (strstr(takeName, _name) != NULL);
						GetSetMediaItemInfo(item, "B_UISEL", &sel);
						break;
					}
				}
			}
		}
	}
	if (update)
	{
		UpdateTimeline();

		// Undo point
		if (_undoMsg)
			Undo_OnStateChangeEx(_undoMsg, UNDO_STATE_ALL, -1);
	}
	return update;
}

// display a search dlg until the user press cancel (ok=search)
bool selectItemsByNamePrompt(const char* _caption, char * _reply)
{
	if (GetUserInputs && GetUserInputs(_caption, 1, "Take name (case sensitive):", _reply, 128))
		return selectItemsByName(_caption, _reply);
	return false;
}

void selectItemsByNamePrompt(COMMAND_T* _ct)
{
	char reply[128] = ""; // initial default search string
	while (selectItemsByNamePrompt(SNM_CMD_SHORTNAME(_ct), reply));
}


///////////////////////////////////////////////////////////////////////////////
// tests..
///////////////////////////////////////////////////////////////////////////////

void ShowTakeEnvPadreTest(COMMAND_T* _ct)
{
	bool updated = false;
	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i+1,false); // doesn't include master
		for (int j = 0; tr && j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* item = GetTrackMediaItem(tr,j);
			if (item && *(bool*)GetSetMediaItemInfo(item,"B_UISEL",NULL))
			{
				switch(_ct->user)
				{
					case 1: updated |= ShowTakeEnvPan(GetActiveTake(item)); break;
					case 2: updated |= ShowTakeEnvMute(GetActiveTake(item)); break;
					case 0: default: updated |= ShowTakeEnvVol(GetActiveTake(item)); break;
				}
			}
		}
	}
	if (updated)
	{
		UpdateTimeline();
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
}

#endif

