/******************************************************************************
/ SNM_Chunk.h
/
/ Copyright (c) 2009 Tim Payne (SWS), JF Bédague
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

#pragma once
#ifndef _SNM_CHUNK_H_
#define _SNM_CHUNK_H_

#include "SNM_ChunkParserPatcher.h"


///////////////////////////////////////////////////////////////////////////////
// SNM_SendPatcher
///////////////////////////////////////////////////////////////////////////////

class SNM_SendPatcher : public SNM_ChunkParserPatcher
{
public:
	SNM_SendPatcher(MediaTrack* _destTrack)	: SNM_ChunkParserPatcher(_destTrack) 
	{
		m_srcId = -1;
		m_sendType = 2; // voluntary deprecated
		m_vol = NULL;
		m_pan = NULL;
	}
	~SNM_SendPatcher() {}
	int AddSend(MediaTrack* _srcTr, int _sendType, char* _vol=NULL, char* _pan=NULL);
	int RemoveReceives();

protected:
	bool NotifyChunkLine(int _mode, LineParser* _lp, WDL_String* _parsedLine,  
		int _parsedOccurence, WDL_PtrList<WDL_String>* _parsedParents, const char* _parsedParent, 
		WDL_String* _newChunk, int _updates);

	int m_srcId;
	int m_sendType;
	char* m_vol;
	char* m_pan;
};


///////////////////////////////////////////////////////////////////////////////
// SNM_FXChainTakePatcher
///////////////////////////////////////////////////////////////////////////////

class SNM_FXChainTakePatcher : public SNM_ChunkParserPatcher
{
public:
	SNM_FXChainTakePatcher(MediaItem* _item) : SNM_ChunkParserPatcher(_item) 
	{
		m_fxChain = NULL;
		m_removingTakeFx = false;
		m_activeTake = false;
	}
	~SNM_FXChainTakePatcher() {}

	int SetFXChain(WDL_String* _fxChain, bool _activeTakeOnly);
	WDL_String* GetFXChain();

protected:
	bool NotifyStartElement(int _mode, LineParser* _lp, WDL_String* _parsedLine,  
		WDL_PtrList<WDL_String>* _parsedParents, const char* _parsedParent, 
		WDL_String* _newChunk, int _updates);

	bool NotifyEndElement(int _mode, LineParser* _lp, WDL_String* _parsedLine,  
		WDL_PtrList<WDL_String>* _parsedParents, const char* _parsedParent, 
		WDL_String* _newChunk, int _updates);

	bool NotifyChunkLine(int _mode, LineParser* _lp, WDL_String* _parsedLine,  
		int _parsedOccurence, WDL_PtrList<WDL_String>* _parsedParents, const char* _parsedParent, 
		WDL_String* _newChunk, int _updates);

	WDL_String* m_fxChain;
	WDL_String m_copiedFXChain;
	bool m_removingTakeFx;
	bool m_copyingTakeFx;
	bool m_activeTake;
};


///////////////////////////////////////////////////////////////////////////////
// SNM_FXChainTrackPatcher
///////////////////////////////////////////////////////////////////////////////

class SNM_FXChainTrackPatcher : public SNM_ChunkParserPatcher
{
public:
	SNM_FXChainTrackPatcher(MediaTrack* _tr) : SNM_ChunkParserPatcher(_tr) 
	{
		m_fxChain = NULL;
		m_removingFxChain = false;
	}
	~SNM_FXChainTrackPatcher() {}

	int SetFXChain(WDL_String* _fxChain);

protected:
	bool NotifyStartElement(int _mode, LineParser* _lp, WDL_String* _parsedLine,  
		WDL_PtrList<WDL_String>* _parsedParents, const char* _parsedParent, 
		WDL_String* _newChunk, int _updates);

	bool NotifyEndElement(int _mode, LineParser* _lp, WDL_String* _parsedLine,  
		WDL_PtrList<WDL_String>* _parsedParents, const char* _parsedParent, 
		WDL_String* _newChunk, int _updates);

	bool NotifyChunkLine(int _mode, LineParser* _lp, WDL_String* _parsedLine,  
		int _parsedOccurence, WDL_PtrList<WDL_String>* _parsedParents, const char* _parsedParent, 
		WDL_String* _newChunk, int _updates);

	WDL_String* m_fxChain;
	bool m_removingFxChain;
	bool m_copyingFxChain;
};

///////////////////////////////////////////////////////////////////////////////
// SNM_TakeParserPatcher
// important: it assumes there're at least 2 takes!
///////////////////////////////////////////////////////////////////////////////

class SNM_TakeParserPatcher : public SNM_ChunkParserPatcher
{
public:
	SNM_TakeParserPatcher(MediaItem* _tr) : SNM_ChunkParserPatcher(_tr) 
	{
		m_searchedTake = -1;
		m_removing = false;
		m_occurence = 0;
		m_removing = false;
		m_getting = false;
	}
	~SNM_TakeParserPatcher() {}

	int RemoveTake(int _take);
	int GetTakeChunk(int _take, WDL_String* _chunk);

protected:
	bool NotifyEndElement(int _mode, LineParser* _lp, WDL_String* _parsedLine,  
		WDL_PtrList<WDL_String>* _parsedParents, const char* _parsedParent, 
		WDL_String* _newChunk, int _updates);

	bool NotifyChunkLine(int _mode, LineParser* _lp, WDL_String* _parsedLine,  
		int _parsedOccurence, WDL_PtrList<WDL_String>* _parsedParents, const char* _parsedParent, 
		WDL_String* _newChunk, int _updates);

	bool m_removing;
	bool m_getting;
	int m_searchedTake;
	int m_occurence;
	WDL_String m_subchunk;
};


#endif