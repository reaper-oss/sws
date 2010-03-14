/******************************************************************************
/ SnM_Item.cpp
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

///////////////////////
// Select item by name
///////////////////////
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
	while (selectItemsByNamePrompt(SNMSWS_ZAP(_ct), reply));
}


///////////////////////
// Clear take
///////////////////////

void clearTake(COMMAND_T* _ct)
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
				int activeTake = *(int*)GetSetMediaItemInfo(item, "I_CURTAKE", NULL);
				SNM_ChunkParserPatcher p(item);
				updated |= (p.ReplaceSubChunk("SOURCE", 2, activeTake, "<SOURCE EMPTY\n>\n") > 0);
			}
		}
	}
	// Undo point
	if (updated)
		Undo_OnStateChangeEx(SNMSWS_ZAP(_ct), UNDO_STATE_ALL, -1);
}

void splitMidiAudio(COMMAND_T* _ct)
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
				double pos = *(double*)GetSetMediaItemInfo(item,"D_POSITION",NULL);
				double end = pos + *(double*)GetSetMediaItemInfo(item,"D_LENGTH",NULL);

				// to be splitted ?
				bool toBeSplitted = (GetCursorPosition() > pos && GetCursorPosition() < end);

				if (!updated && toBeSplitted)
					Undo_BeginBlock();
				updated |= toBeSplitted;

				if (toBeSplitted)
				{
					int activeTake = *(int*)GetSetMediaItemInfo(item, "I_CURTAKE", NULL);
					SNM_ChunkParserPatcher p(item);
					char readSource[32] = "";
					bool split=false;
					if (p.Parse(SNM_GET_CHUNK_CHAR,2,"SOURCE","<SOURCE",-1,activeTake,1,readSource) > 0)
					{
						if (!strcmp(readSource,"MIDI") || !strcmp(readSource,"EMPTY"))
						{
							SplitMediaItem(item, GetCursorPosition());
							split = true;
						}
					}
					
					// split prior zero crossing
					if (!split)
						Main_OnCommand(40792,0);
				}
			}
		}
	}

	// Undo point
	if (updated)
	{
		UpdateTimeline();
		Undo_EndBlock(SNMSWS_ZAP(_ct), UNDO_STATE_ALL);
	}
}