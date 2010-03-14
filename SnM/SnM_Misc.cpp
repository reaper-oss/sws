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

void moveTrack(int _src, int _dest)
{
	// to do _dest to check/clamp..
	InsertTrackAtIndex(_dest, false);
	TrackList_AdjustWindows(true);
	MediaTrack* destTr = CSurf_TrackFromID(_dest+1, false);
	MediaTrack* srcTr = CSurf_TrackFromID(_src, false);
	if (destTr && srcTr)
	{
		SNM_ChunkParserPatcher parser(srcTr);
		if (parser.RemoveIds() >= 0)
		{
			// Get the chunk without ids *but* with receives
			WDL_String updatedChunk(parser.GetChunk()->Get());

			// applies it to the dest. track
			if (updatedChunk.GetLength() && !GetSetObjectState(destTr, updatedChunk.Get()))
			{
				//copy sends
				int j=0;
				MediaTrack* sendDest = NULL;
				do
				{
					sendDest = (MediaTrack*)GetSetTrackSendInfo(srcTr, 0, j, "P_DESTTRACK", NULL);
					if (sendDest)
						addSend(destTr, sendDest, 0);
					j++;
				}
				while (sendDest);

				// remove the source track
				DeleteTrack(srcTr);
			}
		}
	}
}
void showFXChain(int _tr, int _fx)
{
	// to do _dest to check/clamp..
	int show= _fx+1;
	int lastSel=_fx;
	MediaTrack* tr = CSurf_TrackFromID(_tr, false);
	if (tr)
	{
		SNM_ChunkParserPatcher parser(tr);
/*JFB
		char floating[16] = "FLOAT";
		char nonFloating[16] = "FLOATPOS";
		parser.Parse(SET_CHUNK_INT, "FXCHAIN", 2, "LASTSEL",2,0,1,&lastSel);
		parser.Parse(SETALL_CHUNK_CHAR_EXCEPT, "FXCHAIN", 2, "FLOAT",5,255,0,&nonFloating);
		parser.Parse(SETALL_CHUNK_CHAR_EXCEPT, "FXCHAIN", 2, "FLOATPOS",5,_fx,0,&nonFloating, &floating);
*/
		char pShow[4] = "";
		sprintf(pShow,"%d", show);
		char pLastSel[4] = "";
		sprintf(pLastSel,"%d", lastSel);
		bool ok =
			parser.ParsePatch(SNM_SET_CHUNK_CHAR, 2, "FXCHAIN", "LASTSEL",2,0,1,pLastSel) >= 0 &&
			parser.ParsePatch(SNM_SET_CHUNK_CHAR, 2, "FXCHAIN", "SHOW",2,0,1,pShow) >= 0;
	}
}

void showFXChain(COMMAND_T* _ct)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			showFXChain(i,(int)_ct->user);
	}
}

void moveTest(COMMAND_T* _ct)
{
	moveTrack(1,4);
}
