/******************************************************************************
/ TrackFX.cpp
/
/ Copyright (c) 2010 Tim Payne (SWS)
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
#include "TrackFX.h"
#include "../SnM/SnM_Chunk.h"

// Functions for getting/setting track FX chains
void GetFXChain(MediaTrack* tr, WDL_TypedBuf<char>* buf)
{
	SNM_ChunkParserPatcher p(tr);
	WDL_FastString chainChunk;
	if (p.GetSubChunk("FXCHAIN", 2, 0, &chainChunk, "<ITEM") > 0) {
		buf->Resize(chainChunk.GetLength() + 1);
		strcpy(buf->Get(), chainChunk.Get());
	}
}

void SetFXChain(MediaTrack* tr, const char* str)
{
	SNM_FXChainTrackPatcher p(tr);
	WDL_FastString chainChunk;
	// adapt FX chain format (the SNM_FXChainTrackPatcher uses the .RFXChain file format)
	if (str && !strncmp(str, "<FXCHAIN", 8)) {
		chainChunk.Set(strchr(str, '\n') + 1); // removes the line starting with "<FXCHAIN"
		chainChunk.SetLen(chainChunk.GetLength()-2); // remove trailing ">\n"
	}
	p.SetFXChain(str ? &chainChunk : NULL);
}
