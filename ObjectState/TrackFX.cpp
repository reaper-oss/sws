/******************************************************************************
/ TrackFX.cpp
/
/ Copyright (c) 2010 Tim Payne (SWS)
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
#include "TrackFX.h"

// Functions for getting/setting track FX chains
void GetFXChain(MediaTrack* tr, WDL_TypedBuf<char>* buf)
{
	buf->Resize(0); // Empty
	char* chunk = SWS_GetSetObjectState(tr, NULL);
	int pos = 0;
	int iDepth = 0;
	char line[4096];

	while (GetChunkLine(chunk, line, 4096, &pos, true))
	{
		if (strncmp(line, "<FXCHAIN", 8) == 0)
		{
			buf->Resize((int)strlen(line)+1);
			strcpy(buf->Get(), line);
			iDepth = 1;
		}
		else if (iDepth)
		{
			int iPos = buf->GetSize() - 1;
			buf->Resize(iPos + (int)strlen(line) + 1);
			strcpy(buf->Get()+iPos, line);
			if (line[0] == '<')
				iDepth++;
			else if (line[0] == '>')
				iDepth--;
		}
	}

	SWS_FreeHeapPtr(chunk);
}

void SetFXChain(MediaTrack* tr, const char* str)
{
	WDL_String newChunk;
	char line[4096];
	char* chunk = SWS_GetSetObjectState(tr, NULL);
	int pos = 0;
	int iDepth = 0;

	// Fill new chunk with everything except the FX chain that may or may not already exist
	while (GetChunkLine(chunk, line, 4096, &pos, true))
	{
		if (strncmp(line, "<FXCHAIN", 8) == 0)
			iDepth = 1;
		else if (iDepth)
		{
			if (line[0] == '<')
				iDepth++;
			else if (line[0] == '>')
				iDepth--;
		}
		else
			newChunk.Append(line);
	}
	SWS_FreeHeapPtr(chunk);

	if (str)
		AppendChunkLine(&newChunk, str);
	SWS_GetSetObjectState(tr, &newChunk);
}
