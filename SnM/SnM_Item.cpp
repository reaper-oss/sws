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
#include "SNM_Chunk.h"


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
// Take lanes: clear take, build lanes, ...
///////////////////////////////////////////////////////////////////////////////

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
				updated |= p.ReplaceSubChunk("SOURCE", 2, activeTake, "<SOURCE EMPTY\n>\n");
			}
		}
	}
	// Undo point
	if (updated)
	{
		UpdateTimeline();
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
}

bool addEmptyTake(MediaItem* _item)
{
	return (AddTakeToMediaItem(_item) != NULL);
}

bool addTake(MediaItem* _item, WDL_String* _chunk)
{
	bool updated = false;
	int length = _chunk->GetLength();
	if (_item && _chunk && length)
	{
		SNM_ChunkParserPatcher p(_item);
		WDL_String* chunk = p.GetChunk();
		chunk->Insert(_chunk->Get(), chunk->GetLength()-2, length); //-2: before ">\n"
		p.SetUpdates(1);// force commit as we're directly working on the cached chunk
		updated = true;
	}
	return updated;
}

// optionnal SNM_ChunkParserPatcher (for optimization)
// important: it assumes there're at least 2 takes 
//            => DeleteTrackMediaItem() should be used otherwise
bool removeTake(MediaItem* _item, int _take)
{
	bool updated = false;
	if (_item && _take >= 0)
	{
		if (CountTakes(_item) > 1)
		{
			SNM_TakeParserPatcher p(_item); // no pb: chunk not getted yet
			updated |= (p.RemoveTake(_take) > 0);
			// remove the 1st "TAKE" if needed
			if (updated && !_take)
				updated |= (p.ParsePatch(SNM_REPLACE_LINE, 1,"ITEM","TAKE",-1,0,-1,(void*)"") > 0);
		}
	}
	return updated;
}

bool removeTakeOrItem(MediaTrack* _tr, MediaItem* _item, int _take)
{
	bool updated = removeTake(_item, _take);
	if (!_take && CountTakes(_item) == 1)
		updated |= DeleteTrackMediaItem(_tr, _item);
	return updated;
}

void moveTake(COMMAND_T* _ct)
{
	bool updated = false;
	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i+1,false); // doesn't include master
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			for (int j = 0; tr && j < GetTrackNumMediaItems(tr); j++)
			{
				MediaItem* item = GetTrackMediaItem(tr,j);
				if (item && *(bool*)GetSetMediaItemInfo(item,"B_UISEL",NULL))
				{		
					// temp..
					int ntimes = 1;
					if (((int)_ct->user) == 1)
						ntimes = CountTakes(item)-1;

					for (int k=0; k < ntimes; k++)
					{
						// Remove 1st take and re-add it as last one
						SNM_TakeParserPatcher p(item);
						WDL_String chunk;
						if (p.GetTakeChunk(0, &chunk) > 0)
						{
							if (chunk.GetLength() && removeTake(item, 0))
							{
								updated = true;
								p.Commit();
								chunk.Insert("TAKE\n", 0, 5);
								updated |= addTake(item, &chunk);						
							}
						}
					}
/*TODO (see "temp.." workaround above)
					// Remove last take and re-add it as 1st one
					else if (((int)_ct->user) == 1)
					{
						SNM_TakeRemover p(item);
						int lastPos = CountTakes(item)-1;
						if (p.GetTakeChunk(lastPos, &chunk) > 0)
						{
							if (chunk.GetLength() && removeTake(item, lastPos))
							{
								updated = true;
								p.Commit();
								chunk.Insert("TAKE\n", 0, 5);
								updated |= addTake(item, &chunk, 0); //ok, too lazy to do this one..
							}
						}
					}
*/
				}
			}
		}
	}

	// Undo point + UpdateTimeline
	if (updated)
	{
		UpdateTimeline();
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
}

int makeTakeLanesSelectedTracks(const char* _undoTitle) //null: no undo
{
	int updates = 0;
	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i+1,false); // doesn't include master
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			int maxTakes = 0;
			WDL_PtrList<MediaItem> items; // avoids 2 loops on tracks
			for (int j = 0; tr && j < GetTrackNumMediaItems(tr); j++)
			{
				MediaItem* item = GetTrackMediaItem(tr,j);
				if (item)
				{
					maxTakes = max(maxTakes, CountTakes(item));
					items.Add(item);
				}
			}

			for (int j = 0; maxTakes && j < items.GetSize(); j++)
			{
				MediaItem* item = items.Get(j);
				int nbTakes = CountTakes(item);
				for (int k=nbTakes; k < maxTakes; k++)
					updates += addEmptyTake(item);
			}
		}
	}
	// Undo point + UpdateTimeline
	if (_undoTitle && updates > 0)
	{
		UpdateTimeline();
		Undo_OnStateChangeEx(_undoTitle, UNDO_STATE_ALL, -1);
	}
	return updates;
}

void makeTakeLanesSelectedTracks(COMMAND_T* _ct) {
	makeTakeLanesSelectedTracks(SNM_CMD_SHORTNAME(_ct));
}

void selectTakeLane(COMMAND_T* _ct)
{
	bool updated = (makeTakeLanesSelectedTracks((const char *)NULL) > 0);
	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i+1,false); // doesn't include master
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			// Get the 1st selected + active take
			int active = -1;
			for (int j = 0; tr && active == -1 && j < GetTrackNumMediaItems(tr); j++)
			{
				MediaItem* item = GetTrackMediaItem(tr,j);
				if (item && *(bool*)GetSetMediaItemInfo(item,"B_UISEL",NULL))
				{					
					active = *(int*)GetSetMediaItemInfo(item,"I_CURTAKE",NULL);
					break;
				}
			}

			// Sel active take for all items
			if (tr && active >= 0)
			{
				for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
				{
					MediaItem* item = GetTrackMediaItem(tr,j);
					if (item && (*(int*)GetSetMediaItemInfo(item,"I_CURTAKE",NULL)) != active)
					{
						GetSetMediaItemInfo(item,"I_CURTAKE",&active);
						updated = true;
					}
				}
			}		
		}
	}

	// Undo point + UpdateTimeline
	if (updated)
	{
		UpdateTimeline();
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
}

//TODO: optimization..
void removeEmptyTakes(COMMAND_T* _ct)
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
				int k=0;
				char readSource[32] = "";
				while (k < CountTakes(item) && CountTakes(item) > 1)
				{
					SNM_ChunkParserPatcher p(item);
					if (p.Parse(SNM_GET_CHUNK_CHAR,2,"SOURCE","<SOURCE",-1,k,1,readSource) > 0)
					{
						if (!strcmp(readSource,"EMPTY"))
						{
							bool removed = removeTake(item, k);
							if (removed) k--; //++ below!
							updated |= removed;
						}
					}							
					k++;
				}
				
				// Removes the item if needed
				SNM_ChunkParserPatcher p(item);
				if (CountTakes(item) == 1 && 
					(p.Parse(SNM_GET_CHUNK_CHAR,2,"SOURCE","<SOURCE",-1,0,1,readSource) > 0))
				{
					if (!strcmp(readSource,"EMPTY"))
					{
						updated |= DeleteTrackMediaItem(tr, item);
						if (updated) j--;
					}
				}
			}
		}
	}

	// Undo point + UpdateTimeline
	if (updated)
	{
		UpdateTimeline();
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Split MIDI/Audio
///////////////////////////////////////////////////////////////////////////////

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
		Undo_EndBlock(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL);
	}
}