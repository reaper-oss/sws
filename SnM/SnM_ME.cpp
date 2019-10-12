/******************************************************************************
/ SnM_ME.cpp
/
/ Copyright (c) 2010 and later Jeffos
/
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
#include "SnM_Chunk.h"
#include "SnM_Item.h"
#include "SnM_ME.h"
#include "SnM_Util.h"


void MainCreateCCLane(COMMAND_T* _ct)
{
	bool updated = false;
	HWND me = MIDIEditor_GetActive();
	MediaItem_Take* tk = me ? MIDIEditor_GetTake(me) : NULL;
	if (tk)
	{
		MediaItem* item = GetMediaItemTake_Item(tk);
		int tkIdx = GetTakeIndex(item, tk); // null item managed there
		if (tkIdx >= 0)
		{
			SNM_TakeParserPatcher p(item, CountTakes(item));
			WDL_FastString takeChunk;
			int tkPos, tklen;
			if (p.GetTakeChunk(tkIdx, &takeChunk, &tkPos, &tklen))
			{
				SNM_ChunkParserPatcher ptk(&takeChunk, false);

				// check current lanes
				bool lanes[SNM_MAX_CC_LANE_ID+1];
				int i=0; while(i <= SNM_MAX_CC_LANE_ID) lanes[i++]=false;
				char lastLaneId[4] = ""; //max in v3.6: "133"
				int tkFirstPos = 0, laneCpt = 0;
				int pos = ptk.Parse(SNM_GET_CHUNK_CHAR, 1, "SOURCE", "VELLANE", laneCpt, 1, lastLaneId);
				while (pos > 0) 
				{
					if (!tkFirstPos) tkFirstPos = pos;
					lanes[atoi(lastLaneId)] = true; // atoi: 0 on failure, lane 0 won't be used anyway..
					pos = ptk.Parse(SNM_GET_CHUNK_CHAR, 1, "SOURCE", "VELLANE", ++laneCpt, 1, lastLaneId);
				}

				if (tkFirstPos > 0)
				{
					tkFirstPos--; // see SNM_ChunkParserPatcher
					// find the first unused index
					i=1; while(lanes[i] && i <= SNM_MAX_CC_LANE_ID) i++;
					char newLane[SNM_MAX_CHUNK_LINE_LENGTH] = "";
					if (snprintfStrict(newLane, sizeof(newLane), "VELLANE %d 50 0\n", i) > 0)
						ptk.GetChunk()->Insert(newLane, tkFirstPos);

					// "update" take
					updated = p.ReplaceTake(tkPos, tklen, ptk.GetChunk());
				}
			}
		}
	}
	if (updated)
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}

void MECreateCCLane(COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd) {
	MainCreateCCLane(_ct);
}

bool replaceCCLanes(const char* _newCClanes)
{
	bool updated = false;
	HWND me = MIDIEditor_GetActive();
	MediaItem_Take* tk = me ? MIDIEditor_GetTake(me) : NULL;
	if (tk)
	{
		MediaItem* item = GetMediaItemTake_Item(tk);
		int tkIdx = GetTakeIndex(item, tk); // NULL item managed
		if (tkIdx >= 0)
		{
			SNM_TakeParserPatcher p(item, CountTakes(item));
			WDL_FastString takeChunk;

			int tkPos, tklen;
			if (p.GetTakeChunk(tkIdx, &takeChunk, &tkPos, &tklen))
			{
				SNM_ChunkParserPatcher ptk(&takeChunk, false);
				int pos = ptk.Parse(SNM_GET_CHUNK_CHAR, 1, "SOURCE", "VELLANE", 0, 0);
				if (pos > 0)
				{
					pos--; // see SNM_ChunkParserPatcher

					// Remove all lanes for this take
					if (ptk.RemoveLines("VELLANE"))
					{	
						ptk.GetChunk()->Insert(_newCClanes, pos); // default lane (min sized)
						updated = p.ReplaceTake(tkPos, tklen, ptk.GetChunk());
					}
				}
			}
		}
	}
	return updated;
}

void MainHideCCLanes(COMMAND_T* _ct)
{
	if (replaceCCLanes("VELLANE -1 0 0\n")) 
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}

void MEHideCCLanes(COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd) {
	MainHideCCLanes(_ct);
}


void MainSetCCLanes(COMMAND_T* _ct)
{
	HWND me = MIDIEditor_GetActive();
	MediaItem_Take* tk = me ? MIDIEditor_GetTake(me) : NULL;
	if (tk)
	{
		// recall lanes
		char laneSlot[SNM_MAX_CC_LANES_LEN], slot[32] = "";
		if (snprintfStrict(slot, sizeof(slot), "cc_lanes_slot%d", (int)_ct->user + 1) > 0)
		{
			GetPrivateProfileString("MidiEditor", slot, "", laneSlot, SNM_MAX_CC_LANES_LEN, g_SNM_IniFn.Get());

			int i=0; 
			while (laneSlot[i] && i < (SNM_MAX_CC_LANES_LEN-2)) // -2: see string termination
			{
				if (laneSlot[i] == '|')
					laneSlot[i] = '\n';
				i++;
			}
			laneSlot[i++] = '\n';
			laneSlot[i] = 0;

			if (replaceCCLanes(laneSlot)) 
				Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
		}
	}
}

void MESetCCLanes(COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd) {
	MainSetCCLanes(_ct);
}


void MainSaveCCLanes(COMMAND_T* _ct)
{
	HWND me = MIDIEditor_GetActive();
	MediaItem_Take* tk = me ? MIDIEditor_GetTake(me) : NULL;
	if (tk)
	{
		MediaItem* item = GetMediaItemTake_Item(tk);
		int tkIdx = GetTakeIndex(item, tk); // NULL item managed
		if (tkIdx >= 0)
		{
			SNM_TakeParserPatcher p(item, CountTakes(item));
			WDL_FastString takeChunk;
			if (p.GetTakeChunk(tkIdx, &takeChunk))
			{
				SNM_ChunkParserPatcher ptk(&takeChunk, false);

				// check start/end position of lanes in the chunk
				int firstPos = 0, lastPos = 0, laneCpt = 0;
				int pos = ptk.Parse(SNM_GET_CHUNK_CHAR, 1, "SOURCE", "VELLANE", laneCpt, 0);
				while (pos > 0) 
				{
					lastPos = pos;
					if (!firstPos) firstPos = pos;
					pos = ptk.Parse(SNM_GET_CHUNK_CHAR, 1, "SOURCE", "VELLANE", ++laneCpt, 0);
				}

				if (firstPos > 0)
				{
					firstPos--; // see SNM_ChunkParserPatcher

					char laneSlot[SNM_MAX_CC_LANES_LEN] = "";
					int eolLastPos = lastPos;
					const char* pp = ptk.GetChunk()->Get(); //ok 'cause read only
					while (pp[eolLastPos] && pp[eolLastPos] != '\n') eolLastPos++;

					int i = firstPos, j=0;
					while (pp[i] && i<eolLastPos && j < (SNM_MAX_CC_LANES_LEN-1) ) { //-1 see string termination
						if (pp[i] != '\n') laneSlot[j++] = pp[i];
						else laneSlot[j++] = '|';
						i++;
					}
					laneSlot[j] = 0;

					// store lanes
					char slot[32] = "";
					if (snprintfStrict(slot, sizeof(slot), "cc_lanes_slot%d", (int)_ct->user + 1) > 0)
						WritePrivateProfileString("MidiEditor", slot, laneSlot, g_SNM_IniFn.Get());
				}
			}
		}
	}
}

void MESaveCCLanes(COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd) {
	MainSaveCCLanes(_ct);
}
