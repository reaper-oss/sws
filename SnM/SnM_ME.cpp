/******************************************************************************
/ SnM_ME.cpp
/
/ Copyright (c) 2010 Tim Payne (SWS), Jeffos
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

#define MAX_CC_LANE_ID 133
#define MAX_CC_LANES_SLOT 4096


void MECreateCCLane(COMMAND_T* _ct)
{
	bool updated = false;
	void* me = MIDIEditor_GetActive();
	MediaItem_Take* tk = me ? MIDIEditor_GetTake(me) : NULL;
	if (tk)
	{
		MediaItem* item = GetMediaItemTake_Item(tk);
		int tkIdx = getTakeIndex(item, tk); // NULL item managed
		if (tkIdx >= 0)
		{
			SNM_TakeParserPatcher p(item, CountTakes(item));
			WDL_String takeChunk;
			int tkPos, tkOriginalLength;
			if (p.GetTakeChunk(tkIdx, &takeChunk, &tkPos, &tkOriginalLength))
			{
				SNM_ChunkParserPatcher ptk(&takeChunk);

				// Check current displayed lanes
				bool lanes[MAX_CC_LANE_ID+1];
				int i=0; while(i <= MAX_CC_LANE_ID) lanes[i++]=false;
				char lastLaneId[4] = ""; //max in v3.6: "133"
				int tkFirstPos = 0, laneCpt = 0;
				int pos = ptk.Parse(SNM_GET_CHUNK_CHAR, 1, "SOURCE", "VELLANE", -1, laneCpt, 1, lastLaneId);
				while (pos > 0) 
				{
					if (!tkFirstPos) tkFirstPos = pos;
					lanes[atoi(lastLaneId)] = true; // atoi: 0 on failure, lane 0 won't be used anyway..
					pos = ptk.Parse(SNM_GET_CHUNK_CHAR, 1, "SOURCE", "VELLANE", -1, ++laneCpt, 1, lastLaneId);
				}

				if (tkFirstPos > 0)
				{
					tkFirstPos--; // see SNM_ChunkParserPatcher
					// find the first unused index
					i=1; while(lanes[i] && i <= MAX_CC_LANE_ID) i++;
					char newLane[BUFFER_SIZE] = "";
					sprintf(newLane, "VELLANE %d 50 0\n", i);
					ptk.GetChunk()->Insert(newLane, tkFirstPos);

					// "Update" take (invalid tkIdx is managed)
					updated = p.ReplaceTake(tkIdx, tkPos, tkOriginalLength, ptk.GetChunk());
				}
			}
		}
	}
	if (updated)
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}

bool replaceCCLanes(const char* _newCClanes)
{
	bool updated = false;
	void* me = MIDIEditor_GetActive();
	MediaItem_Take* tk = me ? MIDIEditor_GetTake(me) : NULL;
	if (tk)
	{
		MediaItem* item = GetMediaItemTake_Item(tk);
		int tkIdx = getTakeIndex(item, tk); // NULL item managed
		if (tkIdx >= 0)
		{
			SNM_TakeParserPatcher p(item, CountTakes(item));
			WDL_String takeChunk;

			int tkPos, tkOriginalLength;
			if (p.GetTakeChunk(tkIdx, &takeChunk, &tkPos, &tkOriginalLength))
			{
				SNM_ChunkParserPatcher ptk(&takeChunk);
				int pos = ptk.Parse(SNM_GET_CHUNK_CHAR, 1, "SOURCE", "VELLANE", 4, 0, 0);
				if (pos > 0)
				{
					pos--; // see SNM_ChunkParserPatcher

					// Remove all lanes for this take
					WDL_PtrList<const char> removedKeywords;
					removedKeywords.Add("VELLANE");
					if (ptk.RemoveLines(&removedKeywords))
					{
						// default lane (min sized)
						ptk.GetChunk()->Insert(_newCClanes, pos);
						// "Update" take (invalid tkIdx is managed)
						updated = p.ReplaceTake(tkIdx, tkPos, tkOriginalLength, ptk.GetChunk());
					}
				}
			}
		}
	}
	return updated;
}

void MEHideCCLanes(COMMAND_T* _ct)
{
	if (replaceCCLanes("VELLANE -1 0 0\n")) 
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}


void MESetCCLanes(COMMAND_T* _ct)
{
	void* me = MIDIEditor_GetActive();
	MediaItem_Take* tk = me ? MIDIEditor_GetTake(me) : NULL;
	if (tk)
	{
		// Read stored lanes
		char laneSlot[MAX_CC_LANES_SLOT], slot[20] = "";
		sprintf(slot, "CC_LANES_SLOT%d", _ct->user + 1);
		GetPrivateProfileString("MIDI_EDITOR", slot, "", laneSlot, MAX_CC_LANES_SLOT, g_SNMiniFilename.Get());

		int i=0; 
		while (laneSlot[i] && i < (MAX_CC_LANES_SLOT-2)) // -2: see string termination
		{ 
			if (laneSlot[i] == '|') laneSlot[i] = '\n';
			i++;
		}
		laneSlot[i++] = '\n';
		laneSlot[i] = 0;

		if (replaceCCLanes(laneSlot)) 
			Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
}

void MESaveCCLanes(COMMAND_T* _ct)
{
	void* me = MIDIEditor_GetActive();
	MediaItem_Take* tk = me ? MIDIEditor_GetTake(me) : NULL;
	if (tk)
	{
		MediaItem* item = GetMediaItemTake_Item(tk);
		int tkIdx = getTakeIndex(item, tk); // NULL item managed
		if (tkIdx >= 0)
		{
			SNM_TakeParserPatcher p(item, CountTakes(item));
			WDL_String takeChunk;
			if (p.GetTakeChunk(tkIdx, &takeChunk))
			{
				SNM_ChunkParserPatcher ptk(&takeChunk);

				// Check start/end position of lanes in the chunk
				int firstPos = 0, lastPos = 0, laneCpt = 0;
				int pos = ptk.Parse(SNM_GET_CHUNK_CHAR, 1, "SOURCE", "VELLANE", -1, laneCpt, 0);
				while (pos > 0) 
				{
					lastPos = pos;
					if (!firstPos) firstPos = pos;
					pos = ptk.Parse(SNM_GET_CHUNK_CHAR, 1, "SOURCE", "VELLANE", -1, ++laneCpt, 0);
				}

				if (firstPos > 0)
				{
					firstPos--; // see SNM_ChunkParserPatcher

					char laneSlot[MAX_CC_LANES_SLOT] = "";
					int eolLastPos = lastPos;
					char* pp = ptk.GetChunk()->Get();
					while (pp[eolLastPos] && pp[eolLastPos] != '\n') eolLastPos++;

					int i = firstPos, j=0;
					while (pp[i] && i<eolLastPos && j < (MAX_CC_LANES_SLOT-1) ) { //-1 see string termination
						if (pp[i] != '\n') laneSlot[j++] = pp[i];
						else laneSlot[j++] = '|';
						i++;
					}
					laneSlot[j] = 0;

					// Store the lanes
					char slot[20];
					sprintf(slot, "CC_LANES_SLOT%d", _ct->user + 1);
					WritePrivateProfileString("MIDI_EDITOR", slot, laneSlot, g_SNMiniFilename.Get());
				}
			}
		}
	}
}
