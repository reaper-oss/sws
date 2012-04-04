/******************************************************************************
/ SnM_Item.cpp
/
/ Copyright (c) 2009-2012 Jeffos
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
#include "SnM.h"
#include "../Misc/Context.h"
#include "../reaper/localize.h"


///////////////////////////////////////////////////////////////////////////////
// General item helpers
///////////////////////////////////////////////////////////////////////////////

char* GetName(MediaItem* _item) {
	MediaItem_Take* tk = _item ? GetActiveTake(_item) : NULL;
	char* takeName = tk ? (char*)GetSetMediaItemTakeInfo(tk, "P_NAME", NULL) : NULL;
	return takeName;
}

// returns -1 if not found
int getTakeIndex(MediaItem* _item, MediaItem_Take* _take) {
	if (_item)
		for (int i=0; i < CountTakes(_item); i++)
			if (_take == GetTake(_item, i)) // note: NULL take is an empty take since v4
				return i;
	return -1;
}

// deletes an item if it's empty or if it's only made of NULL empty takes
// primitive func (i.e. no undo). returns true if deleted
bool deleteMediaItemIfNeeded(MediaItem* _item)
{
	bool deleted = false;
	MediaTrack* tr = _item ? GetMediaItem_Track(_item) : NULL;
	if (tr && _item)
	{
		int countTk = CountTakes(_item);
		if (!countTk) {
			deleted = true;
		}
		else {
			int i=0, countEmptyTk=0; 
			while(i < countTk && !GetMediaItemTake(_item, i++)) countEmptyTk++;
			deleted = (countTk == countEmptyTk);
		}
		if (deleted)
			deleted &= DeleteTrackMediaItem(tr, _item);
	}
	return deleted;
}

// get, set & clear item selection (primitives: no undo points)

void SNM_GetSelectedItems(ReaProject* _proj, WDL_PtrList<MediaItem>* _items, bool _onSelTracks)
{
	int count = _items ? CountTracks(_proj) : 0;
	for (int i=1; i <= count; i++) // skip master
		if (MediaTrack* tr = SNM_GetTrack(_proj, i))
			if (tr && (!_onSelTracks || (_onSelTracks && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))))
				for (int j=0; j < GetTrackNumMediaItems(tr); j++)
					if (MediaItem* item = GetTrackMediaItem(tr,j))
						if (*(bool*)GetSetMediaItemInfo(item, "B_UISEL", NULL))
							_items->Add(item);
}

void SNM_SetSelectedItems(ReaProject* _proj, WDL_PtrList<MediaItem>* _items, bool _onSelTracks)
{
	int count = _items && _items->GetSize() ? CountTracks(_proj) : 0;
	for (int i=1; i <= count; i++) // skip master
		if (MediaTrack* tr = SNM_GetTrack(_proj, i))
			if (!_onSelTracks || (_onSelTracks && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL)))
				for (int j=0; j < GetTrackNumMediaItems(tr); j++)
					if (MediaItem* item = GetTrackMediaItem(tr, j))
						for (int k=0; k < _items->GetSize(); k++)
							if (_items->Get(k) == item)
								GetSetMediaItemInfo(item, "B_UISEL", &g_bTrue);
}

void SNM_ClearSelectedItems(ReaProject* _proj, bool _onSelTracks)
{
	int count = CountTracks(_proj);
	for (int i=1; i <= count; i++) // skip master
		if (MediaTrack* tr = SNM_GetTrack(_proj, i))
			if (tr && (!_onSelTracks || (_onSelTracks && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))))
				for (int j=0; j < GetTrackNumMediaItems(tr); j++)
					if (MediaItem* item = GetTrackMediaItem(tr, j))
						GetSetMediaItemInfo(item, "B_UISEL", &g_bFalse);
}

bool ItemInInterval(MediaItem* _item, double _pos1, double _pos2)
{
	if (_item)
	{
		double pos = *(double*)GetSetMediaItemInfo(_item,"D_POSITION",NULL);
		double end = pos + *(double*)GetSetMediaItemInfo(_item,"D_LENGTH",NULL);
		if ((pos <= _pos1 && end >= _pos2) || // larger?
			(pos >= _pos1 && pos <= _pos2) || // starts within?
			(end >= _pos1 && end <= _pos2))   // ends within?
			return true;
	}
	return false;
}

bool ItemsInInterval(double _pos1, double _pos2)
{
	for (int i=1; i <= GetNumTracks(); i++) // skip master
	if (MediaTrack* tr = CSurf_TrackFromID(i, false))
		for (int j=0; tr && j < GetTrackNumMediaItems(tr); j++)
			if (ItemInInterval(GetTrackMediaItem(tr,j), _pos1, _pos2))
				return true;
	return false;
}


///////////////////////////////////////////////////////////////////////////////
// Split
///////////////////////////////////////////////////////////////////////////////

void splitMidiAudio(COMMAND_T* _ct)
{
	bool updated = false;
	for (int i = 1; i <= GetNumTracks(); i++) // skip master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false); 
		for (int j = 0; tr && j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* item = GetTrackMediaItem(tr,j);
			if (item && *(bool*)GetSetMediaItemInfo(item,"B_UISEL",NULL))
			{
				double pos = *(double*)GetSetMediaItemInfo(item,"D_POSITION",NULL);
				double end = pos + *(double*)GetSetMediaItemInfo(item,"D_LENGTH",NULL);
				bool toBeSplitted = (GetCursorPositionEx(NULL) > pos && GetCursorPositionEx(NULL) < end);

				if (!updated && toBeSplitted)
					Undo_BeginBlock();

				updated |= toBeSplitted;

				if (toBeSplitted)
				{
					bool split=false;
					MediaItem_Take* tk = GetActiveTake(item);
					if (tk)
					{
						PCM_source* pcm = (PCM_source*)GetSetMediaItemTakeInfo(tk, "P_SOURCE", NULL);
						if (pcm)
						{
							// valid src ? (rmk: "" for in-project)
							if (pcm->GetFileName()) 
								split = (!strcmp(pcm->GetType(), "MIDI") || !strcmp(pcm->GetType(), "MIDIPOOL"));
							// empty take
							else
								split = true;

							// "split prior zero crossing" in all other cases
							// Btw, includes all sources: "WAVE", "VORBIS", "SECTION",..
							if (!split)
								Main_OnCommand(40792,0);
							else
								SplitMediaItem(item, GetCursorPositionEx(NULL));
						}
					}
					// v4 empty takes are null
					else
						SplitMediaItem(item, GetCursorPositionEx(NULL));
				}
			}
		}
	}

	// Undo point
	if (updated)
	{
		UpdateTimeline();
		// hard coded undo label: action name too long + consistent 
		// with the unique native wording (whatever is the split action)
		Undo_EndBlock(__LOCALIZE("Split selected items","sws_undo"), UNDO_STATE_ALL);
	}
}

void smartSplitMidiAudio(COMMAND_T* _ct)
{
	double t1, t2;
	GetSet_LoopTimeRange(false, false, &t1, &t2, false);
	if (AreThereSelItemsInTimeSel() || (t1 != t2 && !CountSelectedMediaItems(NULL)))
		Main_OnCommand(40061, 0); // Split at time sel
	else
		splitMidiAudio(_ct);
}

#ifdef _SNM_MISC 
//  Deprecated: contrary to their native versions, the following code was spliting selected items *and only them*, 
//  see http://forum.cockos.com/showthread.php?t=51547.
//  Due to REAPER v3.67's new native pref "If no items are selected, some split/trim/delete actions affect all items at the edit cursor", 
//  those actions are less useful: they would still split only selected items, even if that native pref is ticked. 
//  Also removed because of the spam in the action list (many split actions).
void splitSelectedItems(COMMAND_T* _ct) {
	if (CountSelectedMediaItems(NULL))
		Main_OnCommand((int)_ct->user, 0);
}
#endif

// no undo point created: use native ones
// (SWS_CMD_SHORTNAME() will not work  "SWS/gofer")
void goferSplitSelectedItems(COMMAND_T* _ct) {
	if (CountSelectedMediaItems(NULL)) {
		Main_OnCommand(40513, 0); // move edit cursor to mouse cursor (obey snapping)
		Main_OnCommand(40757, 0); // split at edit cursor (no selection change)
	}
}

// _undoTitle: undo point name (NULL: no arrange refresh and no undo point)
bool SplitSelectAllItemsInInterval(const char* _undoTitle, double _pos1, double _pos2)
{
	bool updated = false;
	for (int i=1; i <= GetNumTracks(); i++) // skip master
		if (MediaTrack* tr = CSurf_TrackFromID(i, false))
			for (int j=0; tr && j < GetTrackNumMediaItems(tr); j++) // new items might be created during the loop!
				if (MediaItem* item = GetTrackMediaItem(tr,j))
				{
					updated |= (SplitMediaItem(item, _pos1) != NULL);
					updated |= (SplitMediaItem(item, _pos2) != NULL);

					double pos = *(double*)GetSetMediaItemInfo(item,"D_POSITION",NULL);
					double end = pos + *(double*)GetSetMediaItemInfo(item,"D_LENGTH",NULL);
					bool sel = *(bool*)GetSetMediaItemInfo(item,"B_UISEL",NULL);
					if ((pos+SNM_FUDGE_FACTOR) >= _pos1 && (end-SNM_FUDGE_FACTOR) <= _pos2) 
					{
						if (!sel) {
							updated = true;
							GetSetMediaItemInfo(item,"B_UISEL",&g_bTrue);
						}
					}
					else if (sel) {
						updated = true;
						GetSetMediaItemInfo(item,"B_UISEL",&g_bFalse);
					}
				}

	if (_undoTitle && updated) {
		UpdateTimeline();
		Undo_OnStateChangeEx(_undoTitle, UNDO_STATE_ALL, -1);
	}
	return updated;
}

void SplitSelectAllItemsInRegion(COMMAND_T* _ct) {
	double cursorPos = GetCursorPositionEx(NULL);
	int x=0, lastx=0; double dPos, dEnd; bool isRgn;
	while (x = EnumProjectMarkers2(NULL, x, &isRgn, &dPos, &dEnd, NULL, NULL)) {
		if (isRgn && cursorPos >= dPos && cursorPos <= dEnd) {
			SplitSelectAllItemsInInterval(SWS_CMD_SHORTNAME(_ct), dPos, dEnd);
			return; // !!
		}
		lastx=x;
	}
}


///////////////////////////////////////////////////////////////////////////////
// Takes
///////////////////////////////////////////////////////////////////////////////

WDL_FastString g_takeClipoard;

void copyCutTake(COMMAND_T* _ct)
{
	bool updated = false;
	g_takeClipoard.Set("");
	MediaItem* item = GetSelectedMediaItem(NULL, 0);
	if (item)
	{
		int activeTake = *(int*)GetSetMediaItemInfo(item, "I_CURTAKE", NULL);
		SNM_TakeParserPatcher p(item, CountTakes(item));
		if (p.GetTakeChunk(activeTake, &g_takeClipoard))
		{
			if ((int)_ct->user) // Cut take?
			{
				updated = p.RemoveTake(activeTake);
/*JFB TODO? leaves an empty item atm
				if (updated) {
					p.Commit();
					deleteMediaItemIfNeeded(item);
				}
*/
			}
		}
	}
	if (updated)
		Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}

void pasteTake(COMMAND_T* _ct)
{
	bool updated = false;
	for (int i=1; g_takeClipoard.GetLength() && i <= GetNumTracks(); i++) // skip master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; tr && j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* item = GetTrackMediaItem(tr,j);
			if (item && *(bool*)GetSetMediaItemInfo(item,"B_UISEL",NULL))
			{
				SNM_TakeParserPatcher p(item, CountTakes(item));
				int activeTake = *(int*)GetSetMediaItemInfo(item, "I_CURTAKE", NULL) + (int)_ct->user;
				updated |= 	(p.InsertTake(activeTake, &g_takeClipoard) >= 0);
			}
		}
	}
	if (updated)
		Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}


///////////////////////////////////////////////////////////////////////////////
// Take lanes: clear take, build lanes, ...
///////////////////////////////////////////////////////////////////////////////

bool isEmptyMidi(MediaItem_Take* _take)
{
	bool emptyMidi = false;
	if (_take) // a v4 empty take isn't a empty *MIDI* take!
	{
		MidiItemProcessor p("S&M");
		//JFB!!! TODO: replace Padre's MidiItemProcessor which has bugs, e.g. slip edited MIDI items
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
	}
	return emptyMidi;
}

void setEmptyTakeChunk(WDL_FastString* _chunk, int _recPass, int _color, bool _v4style)
{
	// v4 empty take (but w/o take color)
	if (_v4style) 
	{
		_chunk->Set("TAKE NULL\n");
	}
	// v3 empty take style (take with empty source)
	else
	{
		_chunk->Set("TAKE\n");
		_chunk->Append("NAME \"\"\n");
		_chunk->Append("VOLPAN 1.000000 0.000000 1.000000 -1.000000\n");
		_chunk->Append("SOFFS 0.00000000000000\n");
		_chunk->Append("PLAYRATE 1.00000000000000 1 0.00000000000000 -1\n");
		_chunk->Append("CHANMODE 0\n");
		if (_color > 0)
			_chunk->AppendFormatted(32, "TAKECOLOR %d\n", _color);
		if (_recPass > 0)
			_chunk->AppendFormatted(32, "RECPASS %d\n", _recPass);
		_chunk->Append("<SOURCE EMPTY\n");
		_chunk->Append(">\n");
	}
}

// !_undoTitle: no undo
// _mode: 0=for selected tracks, 1=for selected items
int buildLanes(const char* _undoTitle, int _mode) 
{
	int updates = removeEmptyTakes((const char*)NULL, true, false, (_mode==0), (_mode==1));
	bool badRecPass = false;
	for (int i = 1; i <= GetNumTracks(); i++) // skip master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && _mode || (!_mode && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL)))
		{
			WDL_PtrList<void> items;
			WDL_IntKeyedArray<int> recPassColors; 
			WDL_PtrList_DeleteOnDestroy<int> itemRecPasses(free);
			int maxRecPass = -1;

			// Get items, record pass ids & take colors in one go
			for (int j = 0; tr && j < GetTrackNumMediaItems(tr); j++)
			{
				MediaItem* item = GetTrackMediaItem(tr,j);
				if (item && !_mode || (_mode && *(bool*)GetSetMediaItemInfo(item,"B_UISEL",NULL))) 
				{
					int* recPasses = new int[SNM_RECPASSPARSER_MAX_TAKES];
					int takeColors[SNM_RECPASSPARSER_MAX_TAKES];

					SNM_RecPassParser p(item, CountTakes(item));
					int itemMaxRecPass = p.GetMaxRecPass(recPasses, takeColors); 
					maxRecPass = max(itemMaxRecPass, maxRecPass);

					// 1st loop to check rec pass ids
					bool badRecPassItem = false;
					for (int k=0; !badRecPassItem && k < CountTakes(item) && k < SNM_RECPASSPARSER_MAX_TAKES; k++)
						badRecPassItem = (recPasses[k] == 0);

					badRecPass |= badRecPassItem;
					if (!badRecPassItem)
					{
						items.Add(item);
						itemRecPasses.Add(recPasses);
						// 2nd loop to *update* colors by rec pass id
						for (int k=0; k < CountTakes(item) && k < SNM_RECPASSPARSER_MAX_TAKES; k++)
								recPassColors.Insert(recPasses[k], takeColors[k]);
					}
				}
			}

			WDL_PtrList_DeleteOnDestroy<SNM_TakeParserPatcher> ps; // auto commit on destroy!
			for (int j = 0; j < items.GetSize(); j++)
			{
				MediaItem* item = (MediaItem*)items.Get(j);
				SNM_TakeParserPatcher* p = new SNM_TakeParserPatcher(item, CountTakes(item));
				ps.Add(p);

				WDL_PtrList_DeleteOnDestroy<WDL_FastString> chunks;
				for (int k=0; k < maxRecPass; k++)
					chunks.Add(new WDL_FastString());

				// Optimz: use a cache for take chunks
				WDL_IntKeyedArray<char*> oldChunks(freecharptr);
				int tkIdx = 0;					
				while (tkIdx < CountTakes(item)) {
					WDL_FastString c;
					oldChunks.Insert(tkIdx, _strdup(p->GetTakeChunk(tkIdx, &c) ? c.Get() : ""));
					tkIdx++;
				}

				// Create 'maxRecPass' take chunks (ordered, stuffing with empty takes)
				int* recPasses = itemRecPasses.Get(j);
				for (int k=1; k <= maxRecPass; k++)
				{
					tkIdx = 0;					
					bool found = false;
					while (tkIdx < CountTakes(item)) 
					{
						if (recPasses[tkIdx] == k) {
							chunks.Get(k-1)->Set(oldChunks.Get(tkIdx));
							found = true;
							break;
						}
						tkIdx++;
					}

					if (!found)
						setEmptyTakeChunk(chunks.Get(k-1), k, recPassColors.Get(k));
				}

				// insert re-ordered takes..
				int insertPos=-1, k2=0;
				for (int k=0; k < chunks.GetSize(); k++) 
				{					
					if (recPassColors.Get(k+1, -1) != -1) //.. but skip empty lanes
					{
						insertPos = p->InsertTake(k2, chunks.Get(k), insertPos);
						if (insertPos > 0)	{
							updates++;
							k2++;
						}
					}
				}

				// remove old takes
				if (updates)
					p->GetChunk()->DeleteSub(insertPos, strlen((char*)ps.Get(j)->GetChunk()->Get()+insertPos) - 2); //-2 for >\n
			}
		}
	}
	if (updates > 0) {
		UpdateTimeline();
		if (_undoTitle)
			Undo_OnStateChangeEx(_undoTitle, UNDO_STATE_ALL, -1);
	}
	return (badRecPass ? -1 : updates);
}

// primitive (no undo)
bool removeEmptyTakes(MediaTrack* _tr, bool _empty, bool _midiEmpty, bool _trSel, bool _itemSel)
{
	bool updated = false;
	for (int j = 0; _tr && j < GetTrackNumMediaItems(_tr); j++)
	{
		if (!_trSel || (_trSel && *(int*)GetSetMediaTrackInfo(_tr, "I_SELECTED", NULL)))
		{
			MediaItem* item = GetTrackMediaItem(_tr,j);
			if (item && (!_itemSel || (_itemSel && *(bool*)GetSetMediaItemInfo(item,"B_UISEL",NULL))))
			{					
				SNM_TakeParserPatcher p(item, CountTakes(item));
				int k=0, kOriginal=0;
				while (k < p.CountTakesInChunk())
				{
					if ((_empty && p.IsEmpty(k)) ||	(_midiEmpty && isEmptyMidi(GetTake(item, kOriginal))))
					{
						bool removed = p.RemoveTake(k);
						if (removed) k--; //++ below!
						updated |= removed;
					}							
					k++;
					kOriginal++;
				}

				// Removes the item if needed
				if (p.CountTakesInChunk() == 0)
				{
					p.CancelUpdates(); // prevent a useless SNM_ChunkParserPatcher commit
					bool removed = DeleteTrackMediaItem(_tr, item);
					if (removed) j--; 
					updated |= removed;
				}
				// in case we removed empty *MIDI* items but the only remaining takes
				// are empty (i.e. NULL) ones
				else if (p.Commit() && deleteMediaItemIfNeeded(item))
				{
					j--;
					updated = true;
				}
			}
		}
	}
	return updated;
}

// no undo point if !_undoTitle
bool removeEmptyTakes(const char* _undoTitle, bool _empty, bool _midiEmpty, bool _trSel, bool _itemSel)
{
	bool updated = false;
	for (int i = 0; i < GetNumTracks(); i++)
		updated |= removeEmptyTakes(CSurf_TrackFromID(i+1,false), _empty, _midiEmpty, _trSel, _itemSel);
	if (updated) {
		UpdateTimeline();
		if (_undoTitle)
			Undo_OnStateChangeEx(_undoTitle, UNDO_STATE_ALL, -1);
	}
	return updated;
}

void clearTake(COMMAND_T* _ct)
{
	bool updated = false;
	for (int i = 1; i <= GetNumTracks(); i++) // skip master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; tr && j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* item = GetTrackMediaItem(tr,j);
			if (item && *(bool*)GetSetMediaItemInfo(item,"B_UISEL",NULL))
			{
				int activeTake = *(int*)GetSetMediaItemInfo(item, "I_CURTAKE", NULL);
				// v4 empty take
				if (true)
				{
					SNM_TakeParserPatcher p(item, CountTakes(item));
					int pos, len;
					WDL_FastString emptyTk("TAKE NULL SEL\n");
					if (p.GetTakeChunkPos(activeTake, &pos, &len))
					{
						updated |= p.ReplaceTake(pos, len, &emptyTk);

						// empty takes only => remove the item
						if (!strstr(p.GetChunk()->Get(), "\nNAME \""))
						{	
							p.CancelUpdates(); // prevent a useless SNM_ChunkParserPatcher commit
							if (DeleteTrackMediaItem(tr, item)) {
								j--; 
								updated = true;
							}
						}
					}
				}
				// v3 empty take (take with empty source
				else
				{
					SNM_ChunkParserPatcher p(item);
					// no break keyword here: we're already at the end of the item..
					updated |= p.ReplaceSubChunk("SOURCE", 2, activeTake, "<SOURCE EMPTY\n>\n");
				}
			}
		}
	}
	if (updated) {
		UpdateTimeline();
		Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
}

#ifdef _SNM_MISC // deprecated: native actions "Rotate take lanes forward/backward" added in REAPER v3.67
void moveTakes(COMMAND_T* _ct)
{
	bool updated = false;
	int dir = (int)_ct->user;
	for (int i = 1; i <= GetNumTracks(); i++) // skip master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			for (int j = 0; tr && j < GetTrackNumMediaItems(tr); j++)
			{
				MediaItem* item = GetTrackMediaItem(tr,j);
				int newActive = 0;
				if (item && *(bool*)GetSetMediaItemInfo(item,"B_UISEL",NULL))
				{		
					SNM_TakeParserPatcher p(item, CountTakes(item));
					int active = *(int*)GetSetMediaItemInfo(item, "I_CURTAKE", NULL);
					int nbTakes = CountTakes(item);
					if (dir == 1)
					{
						newActive = (active == (nbTakes-1) ? 0 : (active+1));
						// Remove last take and re-add it as 1st one
						WDL_FastString chunk;
						updated = p.RemoveTake(nbTakes-1, &chunk);
						if (updated)
							p.InsertTake(0, &chunk);						
					}
					else if (dir == -1)
					{
						newActive = (!active ? (nbTakes-1) : (active-1));
						// Remove 1st take and re-add it as last one
						WDL_FastString chunk;
						updated = p.RemoveTake(0, &chunk);
						if (updated)
							p.AddLastTake(&chunk);						
					}
				}
				GetSetMediaItemInfo(item, "I_CURTAKE", &newActive);
			}
		}
	}
	if (updated) {
		UpdateTimeline();
		Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
}
#endif

void moveActiveTake(COMMAND_T* _ct)
{
	bool updated = false;
	int dir = (int)_ct->user;
	for (int i = 1; i <= GetNumTracks(); i++) // skip master
	{
		if (MediaTrack* tr = CSurf_TrackFromID(i, false))
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

					WDL_FastString removedChunk;
					SNM_TakeParserPatcher p(item, CountTakes(item));
					if (swapFirstLast) {
						updated |= p.RemoveTake(dir == -1 ? 0 : (initialNbbTakes-1), &removedChunk);
						updated |= (p.InsertTake(dir == -1 ? initialNbbTakes : 0, &removedChunk) > 0);
						active = (dir == 1 ? 0 : (dir == -1 ? (initialNbbTakes-1) : 0));
					}
					else {
						updated |= p.RemoveTake(active, &removedChunk);
						updated |= (p.InsertTake(active+dir, &removedChunk) > 0);
						active += dir;
					}
				}
				GetSetMediaItemInfo(item, "I_CURTAKE", &active);
			}
		}
	}
	if (updated) {
		UpdateTimeline();
		Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
}

void buildLanes(COMMAND_T* _ct) {
	if (buildLanes(SWS_CMD_SHORTNAME(_ct), (int)_ct->user) < 0)
		MessageBox(GetMainHwnd(), __LOCALIZE("Some items were ignored, probable causes:\n- Items not recorded or recorded before REAPER v3.66 (no record pass id)\n- Imploded takes with duplicated record pass ids","sws_mbox"), __LOCALIZE("S&M - Warning","sws_mbox"), MB_OK);
}

void activateLaneFromSelItem(COMMAND_T* _ct)
{
	bool updated = false;
	for (int i=1; i <= GetNumTracks(); i++) // skip master
	{
		if (MediaTrack* tr = CSurf_TrackFromID(i, false))
		{
			// get the 1st selected item and its active take
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

			// activate take for all items
			if (tr && active >= 0)
			{
				for (int j=0; j < GetTrackNumMediaItems(tr); j++)
				{
					MediaItem* item = GetTrackMediaItem(tr,j);
					// "active" validity check relies on GetSetMediaItemInfo()
					if (item && active < CountTakes(item) && (*(int*)GetSetMediaItemInfo(item,"I_CURTAKE",NULL)) != active)
					{
						GetSetMediaItemInfo(item,"I_CURTAKE",&active);
						updated = true;
					}
				}
			}		
		}
	}
	if (_ct && updated) {
		UpdateTimeline();
		Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
}

//JFB undo point even if nothing updated here.. minor
void activateLaneUnderMouse(COMMAND_T* _ct)
{
	Undo_BeginBlock();

	WDL_PtrList<MediaItem> selItems;
	SNM_GetSelectedItems(NULL, &selItems);

	Main_OnCommand(40528,0); // select item under mouse cursor
	Main_OnCommand(41342,0); // activate take under mouse cursor
	activateLaneFromSelItem(NULL);

	SNM_ClearSelectedItems(NULL);
	SNM_SetSelectedItems(NULL, &selItems);
	UpdateTimeline();

	Undo_EndBlock(SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL);
}

void removeEmptyTakes(COMMAND_T* _ct) {
	removeEmptyTakes(SWS_CMD_SHORTNAME(_ct), true, false);
}

void removeEmptyMidiTakes(COMMAND_T* _ct) {
	removeEmptyTakes(SWS_CMD_SHORTNAME(_ct), false, true);
}

void removeAllEmptyTakes(COMMAND_T* _ct) {
	removeEmptyTakes(SWS_CMD_SHORTNAME(_ct), true, true);
}

// note: no undo due to file deletion
bool deleteTakeAndMedia(int _mode)
{
	bool deleteFileOK = true;
	WDL_StringKeyedArray<int> removeFiles;
	for (int i = 1; i <= GetNumTracks(); i++) // skip master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		WDL_PtrList<void> removedItems; 
		bool cancel = false;
		for (int j=0; !cancel && tr && j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* item = GetTrackMediaItem(tr,j);
			if (item && *(bool*)GetSetMediaItemInfo(item,"B_UISEL",NULL))
			{
				int originalTkIdx = 0;
				int originalActiveTkIdx = *(int*)GetSetMediaItemInfo(item, "I_CURTAKE", NULL);
				int nbRemainingTakes = GetMediaItemNumTakes(item);
				for (int k = 0; k < GetMediaItemNumTakes(item); k++) // nb of takes changes here!
				{
					MediaItem_Take* tk = GetMediaItemTake(item,k);
					if ((_mode == 1 || _mode == 2 || // all takes
						((_mode == 3 || _mode == 4) && originalActiveTkIdx == k))) // active take only
					{
						char tkDisplayName[BUFFER_SIZE] = "[empty]"; // no localization here..
						PCM_source* pcm = tk ? (PCM_source*)GetSetMediaItemTakeInfo(tk,"P_SOURCE",NULL) : NULL;
						if (pcm)
						{
							if (pcm->GetFileName() && *(pcm->GetFileName()))
								lstrcpyn(tkDisplayName, pcm->GetFileName(), BUFFER_SIZE);
							else if (pcm->GetFileName() && !strlen(pcm->GetFileName()))
								lstrcpyn(tkDisplayName, (char*)GetSetMediaItemTakeInfo(tk,"P_NAME",NULL), BUFFER_SIZE);
						}

						// not already removed ?
						int rc = removeFiles.Get(tkDisplayName, -1);
						if (rc == -1)
						{							
							if (_mode == 1 || _mode == 3)
							{
								char buf[BUFFER_SIZE*2];

								if (pcm && pcm->GetFileName() && strlen(pcm->GetFileName())) 
									_snprintf(buf, BUFFER_SIZE*2, __LOCALIZE_VERFMT("[Track %d, item %d] Delete take %d and its media file %s ?","sws_mbox"), i, j+1, originalTkIdx+1, tkDisplayName);
								else if (pcm && pcm->GetFileName() && !strlen(pcm->GetFileName())) 
									_snprintf(buf, BUFFER_SIZE*2, __LOCALIZE_VERFMT("[Track %d, item %d] Delete take %d (%s, in-project) ?","sws_mbox"), i, j+1, originalTkIdx+1, tkDisplayName);
								else 
									_snprintf(buf, BUFFER_SIZE*2, __LOCALIZE_VERFMT("[Track %d, item %d] Delete take %d (empty take) ?","sws_mbox"), i, j+1, originalTkIdx+1); // v3 or v4 empty takes

								rc = MessageBox(GetMainHwnd(), buf, __LOCALIZE("S&M - Delete take and source files (no undo!)","sws_mbox"), MB_YESNOCANCEL);
								if (rc == IDCANCEL) {
									cancel = true;
									break;
								}
							}
							else
								rc = IDYES;
							removeFiles.Insert(tkDisplayName, rc);
						}

						if (rc==IDYES)
						{
							nbRemainingTakes--;
							if (pcm && pcm->GetFileName() && strlen(pcm->GetFileName()) && FileExists(pcm->GetFileName()))
							{
								// set all media offline (yeah, EACH TIME! Fails otherwise: http://code.google.com/p/sws-extension/issues/detail?id=175#c3)
								Main_OnCommand(40100,0); 
								if (SNM_DeleteFile(pcm->GetFileName(), true))
								{
									char peakFn[BUFFER_SIZE] = "";
									GetPeakFileName(pcm->GetFileName(), peakFn, BUFFER_SIZE);
									if (peakFn && *peakFn != '\0')
										SNM_DeleteFile(peakFn, true); // no delete check (peaks files can be absent)
								}
								else
									deleteFileOK = false;
							}

							// removes the take (cannot factorize chunk updates here..)
							int cntTakes = CountTakes(item);
							SNM_TakeParserPatcher p(item, cntTakes);
							if (cntTakes > 1 && p.RemoveTake(k)) // > 1 because item removed otherwise
							{
								// active tale only?
								if (_mode == 3 || _mode == 4) break;
								else k--; 
							}
						}
					}
					originalTkIdx++;
				}

				if (!nbRemainingTakes)
					removedItems.Add(item);
			}
		}
		
		// if needed, delete items for this track
		for (int j=0;j<removedItems.GetSize();j++)
			DeleteTrackMediaItem(tr, (MediaItem*)removedItems.Get(j));

		removedItems.Empty(false);
	}
	removeFiles.DeleteAll();
	Main_OnCommand(40101,0); // set all media online
	return deleteFileOK;
}

void deleteTakeAndMedia(COMMAND_T* _ct) {
	if (!deleteTakeAndMedia((int)_ct->user))
		MessageBox(GetMainHwnd(), __LOCALIZE("Warning: at least one file couldn't be deleted.\nTips: are you an administrator? File used by another process?","sws_mbox"), __LOCALIZE("S&M - Delete take and source files","sws_mbox"), MB_OK);
}


///////////////////////////////////////////////////////////////////////////////
// Take envs
///////////////////////////////////////////////////////////////////////////////

int getPitchTakeEnvRangeFromPrefs()
{
	int range = *(int*)GetConfigVar("pitchenvrange");
	// "snap to semitones" bit set ?
	if (range > 0xFF)
		range &= 0xFF;
	return min(231, range); // clamp (like REAPER does)
}

// callers must use UpdateTimeline() at some point if it returns true..
bool patchTakeEnvelopeVis(MediaItem* _item, int _takeIdx, const char* _envKeyword, char* _vis, const WDL_FastString* _defaultPoint, bool _reset)
{
	bool updated = false;
	if (_item)
	{					
		SNM_TakeParserPatcher p(_item, CountTakes(_item));

		WDL_FastString takeChunk;
		int tkPos, tklen;
		if (p.GetTakeChunk(_takeIdx, &takeChunk, &tkPos, &tklen))
		{
			bool takeUpdate = false, buildDefaultEnv = false;
			char vis[2] = {'\0','\0'}; 
			*vis = *_vis;

			// env. already exists?
			if (strstr(takeChunk.Get(), _envKeyword))
			{
				if (_reset)
				{
					SNM_ChunkParserPatcher ptk(&takeChunk);
					takeUpdate = ptk.RemoveSubChunk(_envKeyword, 1, -1);
					buildDefaultEnv = true;
				}
				else
				{
					// toggle ?
					if (!strlen(vis))
					{
						SNM_ChunkParserPatcher ptk(&takeChunk);
						char currentVis[32];
						if (ptk.Parse(SNM_GET_CHUNK_CHAR, 1, _envKeyword, "VIS", 0, 1, (void*)currentVis) > 0)
						{
							// skip if visibility is different from 0 or 1
							if (*currentVis == '1') *vis = '0';
							else if (*currentVis == '0') *vis = '1';
						}
						// just in case..
						if (!vis[0]) 
							return false;
					}

					// prepare the new visibility (in one go)
					{
						SNM_TakeEnvParserPatcher pEnv(&takeChunk);
						takeUpdate = pEnv.SetVis(_envKeyword, atoi(vis));
					}
				}
			}
			else
				buildDefaultEnv = true;

			// build a default env. (if needed)
			if (buildDefaultEnv)
			{						
				if (!vis[0]) 
					*vis = '1'; // toggle ?
				if (*vis == '1')
				{
					takeChunk.Append("<");
					takeChunk.Append(_envKeyword);
					takeChunk.Append("\nACT ");
					takeChunk.Append(vis);
					takeChunk.Append("\nVIS ");
					takeChunk.Append(vis);
					takeChunk.Append(" 1 1.000000\nLANEHEIGHT 0 0\nARM ");
					takeChunk.Append(vis);
					takeChunk.Append("\nDEFSHAPE 0\n");
					takeChunk.Append(_defaultPoint);
					takeChunk.Append("\n>\n");
					takeUpdate = true;
				}
			}

			// update take (with new visibility)
			if (takeUpdate)
				updated = p.ReplaceTake(tkPos, tklen, &takeChunk);
		}
	}
	return updated;
}

bool patchTakeEnvelopeVis(const char* _undoTitle, const char* _envKeyword, char* _vis2, const WDL_FastString* _defaultPoint, bool _reset) 
{
	bool updated = false;
	for (int i = 1; i <= GetNumTracks(); i++) // skip master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; tr && j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* item = GetTrackMediaItem(tr,j);
			if (item && *(bool*)GetSetMediaItemInfo(item,"B_UISEL",NULL))
				updated |= patchTakeEnvelopeVis(item, *(int*)GetSetMediaItemInfo(item,"I_CURTAKE",NULL), _envKeyword, _vis2, _defaultPoint, _reset);
		}
	}

	if (updated)
	{
		UpdateTimeline();
		if (_undoTitle)
			Undo_OnStateChangeEx(_undoTitle, UNDO_STATE_ALL, -1);
	}
	return updated;
}

void panTakeEnvelope(COMMAND_T* _ct) 
{
	WDL_FastString defaultPoint("PT 0.000000 ");
	defaultPoint.AppendFormatted(128, "%d.000000 0", (int)_ct->user);
	patchTakeEnvelopeVis(SWS_CMD_SHORTNAME(_ct), "PANENV", "1", &defaultPoint, true);
}

void showHideTakeVolEnvelope(COMMAND_T* _ct) 
{
	char cVis[2] = ""; //empty means toggle
	int value = (int)_ct->user;
	if (value >= 0)
		_snprintf(cVis, 2, "%d", value);
	WDL_FastString defaultPoint("PT 0.000000 1.000000 0");
	if (patchTakeEnvelopeVis(SWS_CMD_SHORTNAME(_ct), "VOLENV", cVis, &defaultPoint, false) && value < 0) // toggle
		FakeToggle(_ct);
}

void showHideTakePanEnvelope(COMMAND_T* _ct) 
{
	char cVis[2] = ""; //empty means toggle
	int value = (int)_ct->user;
	if (value >= 0)
		_snprintf(cVis, 2, "%d", value);
	WDL_FastString defaultPoint("PT 0.000000 0.000000 0");
	if (patchTakeEnvelopeVis(SWS_CMD_SHORTNAME(_ct), "PANENV", cVis, &defaultPoint, false) && value < 0) // toggle
		FakeToggle(_ct);
}

void showHideTakeMuteEnvelope(COMMAND_T* _ct) 
{
	char cVis[2] = ""; //empty means toggle
	int value = (int)_ct->user;
	if (value >= 0)
		_snprintf(cVis, 2, "%d", value);
	WDL_FastString defaultPoint("PT 0.000000 1.000000 1");
	if (patchTakeEnvelopeVis(SWS_CMD_SHORTNAME(_ct), "MUTEENV", cVis, &defaultPoint, false) && value < 0) // toggle
		FakeToggle(_ct);
}

void showHideTakePitchEnvelope(COMMAND_T* _ct) 
{
	char cVis[2] = ""; //empty means toggle
	int value = (int)_ct->user;
	if (value >= 0)
		_snprintf(cVis, 2, "%d", value);
	WDL_FastString defaultPoint("PT 0.000000 0.000000 0");
	if (patchTakeEnvelopeVis(SWS_CMD_SHORTNAME(_ct), "PITCHENV", cVis, &defaultPoint, false) && value < 0) // toggle
		FakeToggle(_ct);
}

// *** some wrappers for Padre ***
bool ShowTakeEnv(MediaItem_Take* _take, const char* _envKeyword, const WDL_FastString* _defaultPoint)
{
	bool shown = false;
	MediaItem* item = (_take ? GetMediaItemTake_Item(_take) : NULL);
	if (item) 
	{
		int idx = getTakeIndex(item, _take);
		if (idx >= 0) 
			shown = patchTakeEnvelopeVis(item, idx, _envKeyword , "1", _defaultPoint, false);
	}
	return shown;
}

bool ShowTakeEnvVol(MediaItem_Take* _take) {
	WDL_FastString defaultPoint("PT 0.000000 1.000000 0");
	return ShowTakeEnv(_take, "VOLENV", &defaultPoint);
}

bool ShowTakeEnvPan(MediaItem_Take* _take) {
	WDL_FastString defaultPoint("PT 0.000000 0.000000 0");
	return ShowTakeEnv(_take, "PANENV", &defaultPoint);
}

bool ShowTakeEnvMute(MediaItem_Take* _take) {
	WDL_FastString defaultPoint("PT 0.000000 1.000000 1");
	return ShowTakeEnv(_take, "MUTEENV", &defaultPoint);
}

bool ShowTakeEnvPitch(MediaItem_Take* _take) {
	WDL_FastString defaultPoint("PT 0.000000 0.000000 0");
	return ShowTakeEnv(_take, "PITCHENV", &defaultPoint);
}


///////////////////////////////////////////////////////////////////////////////
// Item/take template slots
///////////////////////////////////////////////////////////////////////////////

#ifdef _SNM_MISC
void saveItemTakeTemplate(COMMAND_T* _ct)
{
	for (int i = 1; i <= GetNumTracks(); i++) // skip master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; tr && j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* item = GetTrackMediaItem(tr,j);
			if (item && *(bool*)GetSetMediaItemInfo(item,"B_UISEL",NULL))
			{
				char filename[BUFFER_SIZE] = "", defaultPath[BUFFER_SIZE];
				//JFB TODO: ItemTakeTemplates -> const
				_snprintf(defaultPath, BUFFER_SIZE, "%s%cItemTakeTemplates", GetResourcePath(), PATH_SLASH_CHAR);
				if (BrowseForSaveFile(__LOCALIZE("S&M - Save item/take template","sws_mbox"), defaultPath, NULL, "REAPER item/take template (*.RItemTakeTemplate)\0*.RItemTakeTemplate\0", filename, BUFFER_SIZE))
				{
					// Item/take template: keep the active take
					if (FILE* f = fopenUTF8(filename, "w"))
					{
						int activeTk = *(int*)GetSetMediaItemInfo(item, "I_CURTAKE", NULL);
						if (activeTk >= 0)
						{
							WDL_FastString tkChunk;
							SNM_TakeParserPatcher p(item, CountTakes(item));
							if (p.GetTakeChunk(activeTk, &tkChunk))
							{
								fputs(p.GetChunk()->Get(), f);
								fclose(f);
							}
						}
					}
					return;
				}
			}
		}
	}
}
#endif


///////////////////////////////////////////////////////////////////////////////
// Toolbar item selection toggles
///////////////////////////////////////////////////////////////////////////////

WDL_PtrList<void> g_toolbarItemSel[4];
WDL_PtrList<void> g_toolbarItemSelToggle[4];
SWS_Mutex g_toolbarItemSelLock;

void itemSelToolbarPoll()
{
	SWS_SectionLock lock(&g_toolbarItemSelLock);

	for(int i=0; i < SNM_ITEM_SEL_DOWN; i++)
		g_toolbarItemSel[i].Empty(false);

	if (int countSelItems = CountSelectedMediaItems(NULL))
	{
		// left/right item sel.
		double pos,len,start_time,end_time;
		bool horizontal = false;

		//JFB relies on (incomplete?) SWELL's FindWindowEx here (rather than GetTrackWnd()) 
		if (HWND w = FindWindowEx(GetMainHwnd(), 0, "REAPERTrackListWindow", "trackview"))
		{
			RECT r; GetWindowRect(w, &r);
			GetSet_ArrangeView2(NULL, false, r.left, r.right-17, &start_time, &end_time); // -17 = width of the vert. scrollbar
			horizontal = true;
		}

		// up/down item sel.
		WDL_PtrList<void> trList;
		GetVisibleTCPTracks(&trList);
		bool vertical = (trList.GetSize() > 0);

		for (int i=1; (horizontal || vertical) && i <= GetNumTracks(); i++) // skip master
		{
			MediaTrack* tr = CSurf_TrackFromID(i, false);
			for (int j = 0; tr && j < GetTrackNumMediaItems(tr); j++)
			{
				MediaItem* item = GetTrackMediaItem(tr,j);
				if (item && *(bool*)GetSetMediaItemInfo(item,"B_UISEL",NULL))
				{
					if (horizontal) 
					{
						pos = *(double*)GetSetMediaItemInfo(item, "D_POSITION", NULL);
						if (end_time < pos)
							g_toolbarItemSel[SNM_ITEM_SEL_RIGHT].Add(item);

						len = *(double*)GetSetMediaItemInfo(item, "D_LENGTH", NULL);
						if (start_time > (pos + len))
							g_toolbarItemSel[SNM_ITEM_SEL_LEFT].Add(item);
					}
					if (vertical)
					{
						int minVis=0xFFFF, maxVis=-1;
						for (int k=0; k < trList.GetSize(); k++) 
						{
							// TRP changed to CSurf_TrackToID from GetSetMediaTrackInfo(, "IP_TRACKNUMBER") as it's a casting nightmare on OSX
							//     and CSurf_TrackToID is what's used everywhere else in the project
							int trIdx = CSurf_TrackToID((MediaTrack*)trList.Get(k), false);
							if (trIdx > 0 && trIdx < minVis) minVis = trIdx;
							if (trIdx > 0 && trIdx > maxVis) maxVis = trIdx;
						}

						MediaTrack* tr = GetMediaItem_Track(item);
						if (tr && trList.Find((void*)tr) == -1)
						{
							int trIdx = CSurf_TrackToID(tr, false);
							if (trIdx <= minVis)
								g_toolbarItemSel[SNM_ITEM_SEL_UP].Add(item);
							else if (trIdx >= maxVis)
								g_toolbarItemSel[SNM_ITEM_SEL_DOWN].Add(item);
						}
					}
				}
			}
		}
	}
}

// deselects items out of the scope and -on toggle- reselects those items
void toggleItemSelExists(COMMAND_T* _ct)
{
	bool updated = false, toggle = false;
	int dir = (int)_ct->user;
	WDL_PtrList<void>* l1 = NULL;
	WDL_PtrList<void>* l2 = NULL;

	SWS_SectionLock lock(&g_toolbarItemSelLock);

	if (g_toolbarItemSel[dir].GetSize()) 
	{
		l1 = &(g_toolbarItemSel[dir]);
		l2 = &(g_toolbarItemSelToggle[dir]);
	}
	else if (g_toolbarItemSelToggle[dir].GetSize())
	{
		l2 = &(g_toolbarItemSel[dir]);
		l1 = &(g_toolbarItemSelToggle[dir]);
		toggle = true;
	}

	if (l1 && l2) 
	{
		l2->Empty(false);
		for (int i=0; i < l1->GetSize(); i++)
		{
			GetSetMediaItemInfo((MediaItem*)l1->Get(i), "B_UISEL", &toggle);
			l2->Add((MediaItem*)l1->Get(i));
			updated = true;
		}
		l1->Empty(false);
	}

	if (updated)
	{
		UpdateTimeline();
		Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);

		// in case auto refresh toolbar bar option is off..
		char cmdCustId[SNM_MAX_ACTION_CUSTID_LEN] = "";
		if (_snprintf(cmdCustId, SNM_MAX_ACTION_CUSTID_LEN, "_S&M_TOOLBAR_ITEM_SEL%d", dir) > 0)
			RefreshToolbar(NamedCommandLookup(cmdCustId));
	}
}

// returns the toggle state as fast as possible: background job done in itemSelToolbarPoll() 
bool itemSelExists(COMMAND_T* _ct) {
	SWS_SectionLock lock(&g_toolbarItemSelLock);
	return (g_toolbarItemSel[(int)_ct->user].GetSize() > 0);
}


///////////////////////////////////////////////////////////////////////////////
// Others
///////////////////////////////////////////////////////////////////////////////

// scroll to item, no undo!
//JFB!!! SetOnlyTrackSelected() ?
void scrollToSelItem(MediaItem* _item)
{
	if (_item)
	{
		// horizontal scroll to selected item
		double curPos = GetCursorPositionEx(NULL);
		SetEditCurPos2(NULL, *(double*)GetSetMediaItemInfo(_item, "D_POSITION", NULL), true, false);
		SetEditCurPos2(NULL, curPos, false, false);

		// vertical scroll to selected item
		if (MediaTrack* tr = GetMediaItem_Track(_item))
		{
			//JFB change restore sel programatically => not cool for controle surfaces
			WDL_PtrList<MediaTrack> selTrs;
			SNM_GetSelectedTracks(NULL, &selTrs, true);
			// select only track
			SNM_ClearSelectedTracks(NULL, true);
			GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i1);
			ScrollSelTrack(NULL, true, false);
			SNM_SetSelectedTracks(NULL, &selTrs, true);
		}
		UpdateTimeline();
	}
}

void scrollToSelItem(COMMAND_T* _ct) {
	scrollToSelItem(GetSelectedMediaItem(NULL, 0));
}

// pan take
void setPan(COMMAND_T* _ct)
{
	bool updated = false;
	double value = (double)((int)_ct->user/100);

	for (int i=1; i <= GetNumTracks(); i++) // skip master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j=0; tr && j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* item = GetTrackMediaItem(tr,j);
			if (item && *(bool*)GetSetMediaItemInfo(item,"B_UISEL",NULL))
			{
				MediaItem_Take* tk = GetActiveTake(item);
				if (tk)
				{
					double curValue = *(double*)GetSetMediaItemTakeInfo(tk, "D_PAN", NULL);
					if (fabs(curValue - value) > 0.0001)
					{
						GetSetMediaItemTakeInfo(tk, "D_PAN", &value);
						updated = true;
					}
				}
			}
		}
	}
	if (updated)
		Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}


///////////////////////////////////////////////////////////////////////////////
// Media file slots (Resources view)
///////////////////////////////////////////////////////////////////////////////

void PlaySelTrackMediaSlot(int _slotType, const char* _title, int _slot, bool _pause, bool _loop, double _msi) {
	if (WDL_FastString* fnStr = g_slots.Get(_slotType)->GetOrPromptOrBrowseSlot(_title, _slot)) {
		SNM_PlaySelTrackPreviews(fnStr->Get(), _pause, _loop, _msi);
		delete fnStr;
	}
}

void PlaySelTrackMediaSlot(COMMAND_T* _ct) {
	PlaySelTrackMediaSlot(g_tiedSlotActions[SNM_SLOT_MEDIA], SWS_CMD_SHORTNAME(_ct), (int)_ct->user, false, false, -1.0);
}

void LoopSelTrackMediaSlot(COMMAND_T* _ct) {
	PlaySelTrackMediaSlot(g_tiedSlotActions[SNM_SLOT_MEDIA], SWS_CMD_SHORTNAME(_ct), (int)_ct->user, false, true, -1.0);
}

void SyncPlaySelTrackMediaSlot(COMMAND_T* _ct) {
	PlaySelTrackMediaSlot(g_tiedSlotActions[SNM_SLOT_MEDIA], SWS_CMD_SHORTNAME(_ct), (int)_ct->user, false, false, 1.0);
}

void SyncLoopSelTrackMediaSlot(COMMAND_T* _ct) {
	PlaySelTrackMediaSlot(g_tiedSlotActions[SNM_SLOT_MEDIA], SWS_CMD_SHORTNAME(_ct), (int)_ct->user, false, true, 1.0);
}

// note: no per slot "pause" actions (does not make sense..)

// returns true if something done
bool TogglePlaySelTrackMediaSlot(int _slotType, const char* _title, int _slot, bool _pause, bool _loop, double _msi)
{
	bool done = false;
	if (WDL_FastString* fnStr = g_slots.Get(_slotType)->GetOrPromptOrBrowseSlot(_title, _slot)) {
		done = SNM_TogglePlaySelTrackPreviews(fnStr->Get(), _pause, _loop, _msi);
		delete fnStr;
	}
	return done;
}

// no sync
void TogglePlaySelTrackMediaSlot(COMMAND_T* _ct) {
	if (TogglePlaySelTrackMediaSlot(g_tiedSlotActions[SNM_SLOT_MEDIA], SWS_CMD_SHORTNAME(_ct), (int)_ct->user, false, false))
		FakeToggle(_ct);
}

void ToggleLoopSelTrackMediaSlot(COMMAND_T* _ct) {
	if (TogglePlaySelTrackMediaSlot(g_tiedSlotActions[SNM_SLOT_MEDIA], SWS_CMD_SHORTNAME(_ct), (int)_ct->user, false, true))
		FakeToggle(_ct);
}

void TogglePauseSelTrackMediaSlot(COMMAND_T* _ct) {
	if (TogglePlaySelTrackMediaSlot(g_tiedSlotActions[SNM_SLOT_MEDIA], SWS_CMD_SHORTNAME(_ct), (int)_ct->user, true, false))
		FakeToggle(_ct);
}

void ToggleLoopPauseSelTrackMediaSlot(COMMAND_T* _ct) {
	if (TogglePlaySelTrackMediaSlot(g_tiedSlotActions[SNM_SLOT_MEDIA], SWS_CMD_SHORTNAME(_ct), (int)_ct->user, true, true))
		FakeToggle(_ct);
}

// with sync
#ifdef _SNM_MISC
void SyncTogglePlaySelTrackMediaSlot(COMMAND_T* _ct) {
	if (TogglePlaySelTrackMediaSlot(g_tiedSlotActions[SNM_SLOT_MEDIA], SWS_CMD_SHORTNAME(_ct), (int)_ct->user, false, false, 1.0))
		FakeToggle(_ct);
}

void SyncToggleLoopSelTrackMediaSlot(COMMAND_T* _ct) {
	if (TogglePlaySelTrackMediaSlot(g_tiedSlotActions[SNM_SLOT_MEDIA], SWS_CMD_SHORTNAME(_ct), (int)_ct->user, false, true, 1.0))
		FakeToggle(_ct);
}

void SyncTogglePauseSelTrackMediaSlot(COMMAND_T* _ct) {
	if (TogglePlaySelTrackMediaSlot(g_tiedSlotActions[SNM_SLOT_MEDIA], SWS_CMD_SHORTNAME(_ct), (int)_ct->user, true, false, 1.0))
		FakeToggle(_ct);
}

void SyncToggleLoopPauseSelTrackMediaSlot(COMMAND_T* _ct) {
	if (TogglePlaySelTrackMediaSlot(g_tiedSlotActions[SNM_SLOT_MEDIA], SWS_CMD_SHORTNAME(_ct), (int)_ct->user, true, true, 1.0))
		FakeToggle(_ct);
}
#endif

// _insertMode: 0=add to current track, 1=add new track, 3=add to selected items as takes, &4=stretch/loop to fit time sel, &8=try to match tempo 1x, &16=try to match tempo 0.5x, &32=try to match tempo 2x
void InsertMediaSlot(int _slotType, const char* _title, int _slot, int _insertMode) {
	if (WDL_FastString* fnStr = g_slots.Get(_slotType)->GetOrPromptOrBrowseSlot(_title, _slot)) {
		InsertMedia((char*)fnStr->Get(), _insertMode); //JFB includes undo => _title no used..
		delete fnStr;
	}
}

void InsertMediaSlotCurTr(COMMAND_T* _ct) {
	InsertMediaSlot(g_tiedSlotActions[SNM_SLOT_MEDIA], SWS_CMD_SHORTNAME(_ct), (int)_ct->user, 0);
}

void InsertMediaSlotNewTr(COMMAND_T* _ct) {
	InsertMediaSlot(g_tiedSlotActions[SNM_SLOT_MEDIA], SWS_CMD_SHORTNAME(_ct), (int)_ct->user, 1);
}

void InsertMediaSlotTakes(COMMAND_T* _ct) {
	InsertMediaSlot(g_tiedSlotActions[SNM_SLOT_MEDIA], SWS_CMD_SHORTNAME(_ct), (int)_ct->user, 3);
}

bool autoSaveMediaSlot(int _slotType, const char* _dirPath, char* _fn, int _fnSize)
{
	bool updated = false;
	for (int i = 1; i <= GetNumTracks(); i++) // skip master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false); 
		for (int j = 0; tr && j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* item = GetTrackMediaItem(tr,j);
			if (item && *(bool*)GetSetMediaItemInfo(item, "B_UISEL", NULL))
				if (MediaItem_Take* tk = GetActiveTake(item))
					if (PCM_source* src = (PCM_source*)GetSetMediaItemTakeInfo(tk, "P_SOURCE", NULL))
						if (src->GetFileName())
						{
							char name[SNM_MAX_FX_NAME_LEN] = "";
							if(*src->GetFileName()) {
								GetFilenameNoExt(src->GetFileName(), name, SNM_MAX_FX_NAME_LEN);
								GenerateFilename(_dirPath, name, GetFileExtension(src->GetFileName()), _fn, _fnSize);
								updated |= (SNM_CopyFile(_fn, src->GetFileName()) && g_slots.Get(_slotType)->AddSlot(_fn));
							}
							else { // MIDI in-project
								GetFilenameNoExt((char*)GetSetMediaItemTakeInfo(tk, "P_NAME", NULL), name, SNM_MAX_FX_NAME_LEN);
								Filenamize(name);
								GenerateFilename(_dirPath, name, "mid", _fn, _fnSize);
								src->Extended(PCM_SOURCE_EXT_EXPORTTOFILE, _fn, NULL, NULL);
								updated |= (g_slots.Get(_slotType)->AddSlot(_fn) != NULL);
							}
						}
						// else: v4 empty take
		}
	}
	return updated;
}
