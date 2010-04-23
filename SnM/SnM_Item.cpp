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


///////////////////////////////////////////////////////////////////////////////
// Take envs: Show/hide take vol/pan/mute envs
///////////////////////////////////////////////////////////////////////////////

void patchTakeEnvelopeVis(const char* _undoTitle, const char* _envKeyword, char* _vis2, WDL_String* _defaultPoint) 
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
				int active = *(int*)GetSetMediaItemInfo(item,"I_CURTAKE",NULL);
				SNM_TakeParserPatcher p(item);
				WDL_String takeChunk;
				if (p.GetTakeChunk(active, &takeChunk))
				{
					SNM_ChunkParserPatcher ptk(&takeChunk);
					bool takeUpdate = false;
					char vis[2]; strcpy(vis, _vis2);

					// env. already exists
					if (strstr(takeChunk.Get(), _envKeyword))
					{
						// toggle ?
						if (!strlen(vis))
						{
							char currentVis[32];
							if (ptk.Parse(SNM_GET_CHUNK_CHAR, 1, _envKeyword, "VIS", -1, 0, 1, (void*)currentVis) > 0)
							{
								if (!strcmp(currentVis, "1")) strcpy(vis, "0");
								else if (!strcmp(currentVis, "0")) strcpy(vis, "1");
							}
							// just in case..
							if (!strlen(vis)) break;
						}

						// prepare the new visibility
						if (ptk.ParsePatch(SNM_SET_CHUNK_CHAR, 1, _envKeyword, "ACT", -1, 0, 1, (void*)vis) > 0)
							if (ptk.ParsePatch(SNM_SET_CHUNK_CHAR, 1, _envKeyword, "VIS", -1, 0, 1, (void*)vis) > 0)
								takeUpdate |= (ptk.ParsePatch(SNM_SET_CHUNK_CHAR, 1, _envKeyword, "ARM", -1, 0, 1, (void*)vis) > 0);

						takeChunk.Set(ptk.GetChunk()->Get());
					}
					// env. doesn't already exists => build a default one (if needed)
					else
					{						
						if (!strlen(vis)) strcpy(vis, "1"); // toggle ?
						if (!strcmp(vis, "1"))
						{
							// TODO: AppendFormatted with SNM_String
							// (skipped for OSX for now..)
							takeChunk.Append("<");
							takeChunk.Append(_envKeyword);
							takeChunk.Append("\nACT ");
							takeChunk.Append(vis);
							takeChunk.Append("\nVIS ");
							takeChunk.Append(vis);
							takeChunk.Append(" 1 1.000000\n");
							takeChunk.Append("LANEHEIGHT 0 0\n");
							takeChunk.Append("ARM ");
							takeChunk.Append(vis);
							takeChunk.Append("\nDEFSHAPE 0\n");
							takeChunk.Append(_defaultPoint->Get());
							takeChunk.Append("\n>\n");
							takeUpdate = true;
						}
					}
					
					// Patch new visibility as a new take, removed the previous one
					if (takeUpdate && p.InsertTake(active, &takeChunk))
						updated |= p.RemoveTake(active+1);
				}
			}
		}
	}

	if (updated)
	{
		UpdateTimeline();
		if (_undoTitle)
			Undo_OnStateChangeEx(_undoTitle, UNDO_STATE_ALL, -1);
	}
}

void showHideTakeVolEnvelope(COMMAND_T* _ct) 
{
	char cVis[2] = ""; //empty means toggle
	if (_ct->user >= 0)
		sprintf(cVis, "%d", _ct->user);
	WDL_String defaultPoint("PT 0.000000 1.000000 0");
	patchTakeEnvelopeVis(SNM_CMD_SHORTNAME(_ct), "VOLENV", cVis, &defaultPoint);
}

void showHideTakePanEnvelope(COMMAND_T* _ct) 
{
	char cVis[2] = ""; //empty means toggle
	if (_ct->user >= 0)
		sprintf(cVis, "%d", _ct->user);
	WDL_String defaultPoint("PT 0.000000 0.000000 0");
	patchTakeEnvelopeVis(SNM_CMD_SHORTNAME(_ct), "PANENV", cVis, &defaultPoint);
}

void showHideTakeMuteEnvelope(COMMAND_T* _ct) 
{
	char cVis[2] = ""; //empty means toggle
	if (_ct->user >= 0)
		sprintf(cVis, "%d", _ct->user);
	WDL_String defaultPoint("PT 0.000000 1.000000 1");
	patchTakeEnvelopeVis(SNM_CMD_SHORTNAME(_ct), "MUTEENV", cVis, &defaultPoint);
}
