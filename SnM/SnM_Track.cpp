/******************************************************************************
/ SnM_Track.cpp
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


///////////////////////////////////////////////////////////////////////////////
// Track grouping
///////////////////////////////////////////////////////////////////////////////

// just did what I need for now..
int addSoloToGroup(MediaTrack * _tr, int _group, bool _master, SNM_ChunkParserPatcher* _cpp)
{
	int updates = 0;
	if (_tr && _cpp && _group > 0 && _group <= 32)
	{
		WDL_String grpLine;
		double grpMask = pow(2.0,(_group-1)*1.0);

		// no track grouping yet ?
		if (!_cpp->Parse(SNM_GET_SUBCHUNK_OR_LINE, 1, "TRACK", "GROUP_FLAGS", -1 , 0, 0, &grpLine))
		{
			char tmp[64] = "";
			int patchPos = _cpp->Parse(SNM_GET_CHUNK_CHAR, 1, "TRACK", "TRACKHEIGHT", -1, 0, 0, tmp);
			if (patchPos > 0)
			{
				patchPos--; // see SNM_ChunkParserPatcher..
				WDL_String s;
				s.Append("GROUP_FLAGS 0 0 0 0 0 0");
				s.AppendFormatted(128, _master ? " %d 0 " : " 0 %d ", (int)grpMask);
				s.Append("0 0 0 0 0 0 0 0 0\n");
				_cpp->GetChunk()->Insert(s.Get(), patchPos);				

				// as we're directly working on the cached chunk..
				updates = _cpp->SetUpdates(1);
			}
		}
		// track grouping already exist => patch only what's needed
		else
		{
			// complete the line if needed
			while (grpLine.GetLength() < 64)
			{
				if (grpLine.Get()[grpLine.GetLength()-1] != ' ')
					grpLine.Append(" ");
				grpLine.Append("0 0");
			}

			LineParser lp(false);
			lp.parse(grpLine.Get());
			WDL_String newFlags;
			for (int i=0; i < lp.getnumtokens(); i++)
			{
				if ((i==7 && _master) || (i==8 && !_master))
					newFlags.AppendFormatted(128, "%d", ((int)grpMask) | lp.gettoken_int(i));
				else
					newFlags.Append(lp.gettoken_str(i));
				if (i != (lp.getnumtokens()-1))
					newFlags.Append(" ");
			}
			updates = _cpp->ReplaceLine("TRACK","GROUP_FLAGS", 1, 0, newFlags.Get());
		}
	}
	return updates;
}


bool loadTrackTemplate(char* _filename, WDL_String* _chunk)
{
	if (_filename && *_filename)
	{
		FILE* f = fopen(_filename, "r");
		if (f)
		{
			_chunk->Set("");
			char str[4096];
			while(fgets(str, 4096, f))
				_chunk->Append(str);
			fclose(f);
			return true;
		}
	}
	return false;
}