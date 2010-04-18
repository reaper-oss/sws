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
					
					// "split prior zero crossing" in all other cases
					// Btw, includes all sources: "WAVE", "VORBIS", "SECTION",..
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


///////////////////////////////////////////////////////////////////////////////
// Take lanes: clear take, build lanes, ...
///////////////////////////////////////////////////////////////////////////////

bool isEmptyMidi(MediaItem_Take* _take)
{
	bool emptyMidi = false;
	MidiItemProcessor p("S&M");
	if (_take && p.isMidiTake(_take))
	{
		MIDI_eventlist* evts = MIDI_eventlist_Create();
		if(p.getMidiEventsList(_take, evts))
		{
			int pos=0;
			emptyMidi = !evts->EnumItems(&pos);
			MIDI_eventlist_Destroy(evts);
		}
	}
	return emptyMidi;
}

void setEmptyTakeChunk(WDL_String* _chunk)
{
	_chunk->Set("TAKE\n");
    _chunk->Append("NAME \"\"\n");
    _chunk->Append("VOLPAN 1.000000 0.000000 1.000000 -1.000000\n");
    _chunk->Append("SOFFS 0.00000000000000\n");
    _chunk->Append("PLAYRATE 1.00000000000000 1 0.00000000000000 -1\n");
    _chunk->Append("CHANMODE 0\n");
    _chunk->Append("<SOURCE EMPTY\n");
    _chunk->Append(">\n");
}

bool addEmptyTake(MediaItem* _item) {
	return (AddTakeToMediaItem(_item) != NULL);
}

int findFirstTakeByFilename(
	MediaItem* _item, const char* _takeName, bool* _alreadyFound)
{
	if (_item && _takeName)
	{
		for (int j = 0; j < GetMediaItemNumTakes(_item); j++)
		{
			PCM_source* src = 
				(PCM_source*)GetSetMediaItemTakeInfo(GetMediaItemTake(_item, j), "P_SOURCE", NULL);
			if (!_alreadyFound[j] && src && 
				((!src->GetFileName() && !strlen(_takeName)) || 
				  (src->GetFileName() && !strcmp(src->GetFileName(), _takeName))))
			{
				_alreadyFound[j] = true;
				return j;
			}
		}			
	}
	return -1;
}

int countTakesByFilename(
	MediaItem* _item, const char* _takeName)
{
	int count = 0;
	if (_item && _takeName)
	{
		for (int j = 0; j < GetMediaItemNumTakes(_item); j++)
		{
			PCM_source* src = 
				(PCM_source*)GetSetMediaItemTakeInfo(GetMediaItemTake(_item, j), "P_SOURCE", NULL);
			if (src && src->GetFileName() && !strcmp(src->GetFileName(), _takeName))
				count++;
		}			
	}
	return count;
}

// !_undoTitle: no undo
int buildLanes(const char* _undoTitle) 
{
	int updates = removeEmptyTakes((const char*)NULL, true, false, true);
	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i+1,false); // doesn't include master
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			// sore selected items and store the item with max. takes
			WDL_PtrList<MediaItem> items; 
			WDL_PtrList<WDL_String> filenames;
			for (int j = 0; tr && j < GetTrackNumMediaItems(tr); j++)
			{
				MediaItem* item = GetTrackMediaItem(tr,j);
				if (item) 
				{
					items.Add(item);

					// Get the src filenames 
					int nbTakes = GetMediaItemNumTakes(item);
					WDL_String lastLoop;
					for (int k=0; k < nbTakes; k++)
					{
						PCM_source* src = 
							(PCM_source*)GetSetMediaItemTakeInfo(GetMediaItemTake(item, k), "P_SOURCE", NULL);
						// prefered order for takes with source file: begining of lane
						if (src && src->GetFileName() && strlen(src->GetFileName()))
						{
							if (!strcmp(lastLoop.Get(), src->GetFileName())) 
							{
//								filenames.Insert(0,new WDL_String(src->GetFileName()));
								filenames.Add(new WDL_String(src->GetFileName()));
							}
							else
							{
								int presentCount = 0;
								for (int l=0; l < filenames.GetSize(); l++)
									if(!strcmp(filenames.Get(l)->Get(), src->GetFileName()))
										presentCount++;
								if (!presentCount || 
									(presentCount && countTakesByFilename(item, src->GetFileName()) > presentCount))
								{
//									filenames.Insert(0,new WDL_String(src->GetFileName()));
									filenames.Add(new WDL_String(src->GetFileName()));
									lastLoop.Set(src->GetFileName());
								}
							}							
						}
						// prefered order for empty takes: end of lane
						else
						{
							filenames.Add(new WDL_String());
							lastLoop.Set("");
						}
					}	
				}
			}

			int nbLanes = filenames.GetSize();
			WDL_PtrList<SNM_TakeParserPatcher> ps;
			for (int j = 0; j < items.GetSize(); j++)
			{
				MediaItem* item = items.Get(j);
				WDL_PtrList<WDL_String> chunks;
				SNM_TakeParserPatcher* p = new SNM_TakeParserPatcher(item);
				ps.Add(p);

				// init (max. 128 takes should be enough)
				bool found[128]; 
				for (int k=0; k < nbLanes; k++)	{
					chunks.Add(new WDL_String());
					found[k] = false;
				}

				for (int k=0; k < nbLanes; k++)	{
					int first = findFirstTakeByFilename(item, filenames.Get(k)->Get(), found); 
					bool patched = false;
					if (first >= 0)
						patched = p->GetTakeChunk(first, chunks.Get(k));
					if (!patched)
						setEmptyTakeChunk(chunks.Get(k));
				}

				// insert correct takes 
				// so that there're (current nb of takes + maxTakes) lanes
				for (int k=0; k < nbLanes; k++)
					updates += p->InsertTake(k, chunks.Get(k));
				chunks.Empty(true);

				// remove the last (intial nb of takes) lanes
				// so that it remains (maxTakes + additionnal) lanes
				while (p->CountTakes() > nbLanes)
					updates += p->RemoveTake(p->CountTakes()-1);
			}

			// Remove "empty lanes" if needed
			int lane = 0;
			int emptyTakes = items.GetSize();
			while (emptyTakes == items.GetSize() || lane < nbLanes)
			{
				emptyTakes = 0;
				for (int j = 0; j < items.GetSize(); j++)
				{
					if (ps.Get(j)->IsEmpty(lane))
						emptyTakes++;
					else break;
				}
				if (emptyTakes == items.GetSize())
				{
					nbLanes--;
					for (int j = 0; j < items.GetSize(); j++)
						updates += ps.Get(j)->RemoveTake(lane);
				}
				else lane++;
			}
			filenames.Empty(true);
			ps.Empty(true); // + auto commit!
		}
	}

	// Undo point + UpdateTimeline
	if (updates > 0)
	{
		UpdateTimeline();
		if (_undoTitle)
			Undo_OnStateChangeEx(_undoTitle, UNDO_STATE_ALL, -1);
	}
	return updates;
}

// no undo point if !_undoTitle
bool removeEmptyTakes(const char* _undoTitle, bool _empty, bool _midiEmpty, bool _trSel)
{
	bool updated = false;
	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i+1,false); // doesn't include master
		for (int j = 0; tr && j < GetTrackNumMediaItems(tr); j++)
		{
			if (!_trSel || (_trSel && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL)))
			{
				MediaItem* item = GetTrackMediaItem(tr,j);
				if (item && (_trSel || (!_trSel && *(bool*)GetSetMediaItemInfo(item,"B_UISEL",NULL))))
				{					
					SNM_TakeParserPatcher p(item);
					int k=0, kOriginal=0;
					while (k < p.CountTakes() && p.CountTakes() > 1) // p.CountTakes() is a getter
					{
						if ((_empty && p.IsEmpty(k)) ||
							(_midiEmpty && isEmptyMidi(GetTake(item, kOriginal))))
						{
							bool removed = p.RemoveTake(k);
							if (removed) k--; //++ below!
							updated |= removed;
						}							
						k++;
						kOriginal++;
					}
					
					// Removes the item if needed
					if (p.CountTakes() == 1)
					{
						if ((_empty && p.IsEmpty(0)) ||
							(_midiEmpty && isEmptyMidi(GetTake(item, 0))))
						{
							// prevent a useless SNM_ChunkParserPatcher commit
							p.CancelUpdates();

							bool removed = DeleteTrackMediaItem(tr, item);
							if (removed) j--; //++ below!
							updated |= removed;
						}
					}
				}
			}
		}
	}

	// Undo point + UpdateTimeline
	if (updated)
	{
		UpdateTimeline();
		if (_undoTitle)
			Undo_OnStateChangeEx(_undoTitle, UNDO_STATE_ALL, -1);
	}
	return updated;
}


/***********/
// Commands
/***********/

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

void moveTakes(COMMAND_T* _ct)
{
	bool updated = false;
	int dir = (int)_ct->user;
	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i+1,false); // doesn't include master
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			for (int j = 0; tr && j < GetTrackNumMediaItems(tr); j++)
			{
				MediaItem* item = GetTrackMediaItem(tr,j);
				int newActive = 0;
				if (item && *(bool*)GetSetMediaItemInfo(item,"B_UISEL",NULL))
				{		
					SNM_TakeParserPatcher p(item);
					int active = *(int*)GetSetMediaItemInfo(item, "I_CURTAKE", NULL);
					int nbTakes = CountTakes(item);
					if (dir == 1)
					{
						newActive = (active == (nbTakes-1) ? 0 : (active+1));
						// Remove last take and re-add it as 1st one
						WDL_String chunk;
						updated = p.RemoveTake(nbTakes-1, &chunk);
						if (updated)
							p.InsertTake(0, &chunk);						
					}
					else if (dir == -1)
					{
						newActive = (!active ? (nbTakes-1) : (active-1));
						// Remove 1st take and re-add it as last one
						WDL_String chunk;
						updated = p.RemoveTake(0, &chunk);
						if (updated)
							p.AddLastTake(&chunk);						
					}
				}
				GetSetMediaItemInfo(item, "I_CURTAKE", &newActive);
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

void moveActiveTake(COMMAND_T* _ct)
{
	bool updated = false;
	int dir = (int)_ct->user;
	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i+1,false); // doesn't include master
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			for (int j = 0; tr && j < GetTrackNumMediaItems(tr); j++)
			{
				MediaItem* item = GetTrackMediaItem(tr,j);
				int active = *(int*)GetSetMediaItemInfo(item, "I_CURTAKE", NULL);
				if (item && *(bool*)GetSetMediaItemInfo(item,"B_UISEL",NULL))
				{
					int initialNbbTakes = CountTakes(item);

					// cycle (last<->first) ?
					bool swapFirstLast = 
						((dir == 1 && active == (initialNbbTakes-1)) ||
						(dir == -1 && !active));

					WDL_String removedChunk;
					SNM_TakeParserPatcher p(item);
					if (swapFirstLast) {
						updated |= p.RemoveTake(dir == -1 ? 0 : (initialNbbTakes-1), &removedChunk);
						updated |= p.InsertTake(dir == -1 ? initialNbbTakes : 0, &removedChunk);
						active = (dir == 1 ? 0 : (dir == -1 ? (initialNbbTakes-1) : 0));
					}
					else {
						updated |= p.RemoveTake(active, &removedChunk);
						updated |= p.InsertTake(active+dir, &removedChunk);
						active += dir;
					}
				}
				GetSetMediaItemInfo(item, "I_CURTAKE", &active);
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

void buildLanes(COMMAND_T* _ct) {
	buildLanes(SNM_CMD_SHORTNAME(_ct));
}

void selectTakeLane(COMMAND_T* _ct)
{
	bool updated = false;
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
					// "active" validity check relies on GetSetMediaItemInfo()
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

void removeEmptyTakes(COMMAND_T* _ct) {
	removeEmptyTakes(SNM_CMD_SHORTNAME(_ct), true, false);
}

void removeEmptyMidiTakes(COMMAND_T* _ct) {
	removeEmptyTakes(SNM_CMD_SHORTNAME(_ct), false, true);
}

void removeAllEmptyTakes(COMMAND_T* _ct) {
	removeEmptyTakes(SNM_CMD_SHORTNAME(_ct), true, true);
}

