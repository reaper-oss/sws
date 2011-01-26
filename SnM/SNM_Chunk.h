/******************************************************************************
/ SNM_Chunk.h
/
/ Copyright (c) 2009 Tim Payne (SWS), Jeffos
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

#include "SnM_Actions.h"
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
		m_sndRcv = NULL;
	}
	~SNM_SendPatcher() {}
	int AddReceive(MediaTrack* _srcTr, int _sendType, char* _vol, char* _pan);
	bool AddReceive(MediaTrack* _srcTr, SNM_SndRcv* _io);
	int RemoveReceives();
	int RemoveReceivesFrom(MediaTrack* _srcTr);

protected:
	bool NotifyChunkLine(int _mode, 
		LineParser* _lp, const char* _parsedLine, int _linePos, 
		int _parsedOccurence, WDL_PtrList<WDL_String>* _parsedParents, 
		WDL_String* _newChunk, int _updates);

	int m_srcId;
	int m_sendType;
	char* m_vol;
	char* m_pan;
	SNM_SndRcv* m_sndRcv;
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

	bool SetFXChain(WDL_String* _fxChain, bool _activeTakeOnly);
	WDL_String* GetFXChain();

protected:
	bool NotifyStartElement(int _mode, 
		LineParser* _lp, const char* _parsedLine, int _linePos, 
		WDL_PtrList<WDL_String>* _parsedParents, 
		WDL_String* _newChunk, int _updates);

	bool NotifyEndElement(int _mode, 
		LineParser* _lp, const char* _parsedLine, int _linePos,
		WDL_PtrList<WDL_String>* _parsedParents, 
		WDL_String* _newChunk, int _updates);

	bool NotifyChunkLine(int _mode, 
		LineParser* _lp, const char* _parsedLine, int _linePos,
		int _parsedOccurence, WDL_PtrList<WDL_String>* _parsedParents, 
		WDL_String* _newChunk, int _updates);

	bool NotifySkippedSubChunk(int _mode, 
		const char* _subChunk, int _subChunkLength, int _subChunkPos,
		WDL_PtrList<WDL_String>* _parsedParents, 
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

	bool SetFXChain(WDL_String* _fxChain);

protected:
	bool NotifyStartElement(int _mode, 
		LineParser* _lp, const char* _parsedLine, int _linePos, 
		WDL_PtrList<WDL_String>* _parsedParents, 
		WDL_String* _newChunk, int _updates);

	bool NotifyEndElement(int _mode, 
		LineParser* _lp, const char* _parsedLine, int _linePos,
		WDL_PtrList<WDL_String>* _parsedParents, 
		WDL_String* _newChunk, int _updates);

	bool NotifyChunkLine(int _mode, 
		LineParser* _lp, const char* _parsedLine, int _linePos, 
		int _parsedOccurence, WDL_PtrList<WDL_String>* _parsedParents, 
		WDL_String* _newChunk, int _updates);

	bool SNM_FXChainTrackPatcher::NotifySkippedSubChunk(int _mode, 
		const char* _subChunk, int _subChunkLength, int _subChunkPos,
		WDL_PtrList<WDL_String>* _parsedParents, 
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
	SNM_TakeParserPatcher(MediaItem* _item, int _countTakes) : SNM_ChunkParserPatcher(_item) 
	{
		m_searchedTake = -1;
		m_removing = false;
		m_takeCounter = -1;
		m_lastTakeCount = _countTakes; // -1 would force a get, initialized with REAPER's Countakes() for optimization
		m_removing = false;
		m_getting = false; 
		m_foundPos = -1;
	}
	~SNM_TakeParserPatcher() {}

	bool GetTakeChunk(int _takeIdx, WDL_String* _gettedChunk, int* _pos=NULL, int* _originalLength=NULL);
	int CountTakes();
	bool IsEmpty(int _takeIdx);
	int AddLastTake(WDL_String* _chunk);
	int InsertTake(int _takeIdx, WDL_String* _chunk, int _pos = -1);
	bool RemoveTake(int _takeIdx, WDL_String* _removedChunk = NULL, int* _removedStartPos=NULL);
	bool ReplaceTake(int _takeIdx, int _startTakePos, int _takeLength, WDL_String* _newTakeChunk);

protected:
	bool NotifyEndElement(int _mode, 
		LineParser* _lp, const char* _parsedLine, int _linePos, 
		WDL_PtrList<WDL_String>* _parsedParents, 
		WDL_String* _newChunk, int _updates);

	bool NotifyChunkLine(int _mode, 
		LineParser* _lp, const char* _parsedLine, int _linePos, 
		int _parsedOccurence, WDL_PtrList<WDL_String>* _parsedParents, 
		WDL_String* _newChunk, int _updates);

	bool NotifySkippedSubChunk(int _mode, 
		const char* _subChunk, int _subChunkLength, int _subChunkPos, 
		WDL_PtrList<WDL_String>* _parsedParents, 
		WDL_String* _newChunk, int _updates);

	bool m_removing;
	bool m_getting;
	int m_searchedTake;
	int m_foundPos;
	WDL_String m_subchunk;
	int m_lastTakeCount; // Avoids many parsings: should *always* reflect the nb of takes in the *chunk*
						 // (may be different than REAPER's ones)

private:
	int m_takeCounter;	
};


///////////////////////////////////////////////////////////////////////////////
// SNM_RecPassParser
///////////////////////////////////////////////////////////////////////////////

class SNM_RecPassParser : public SNM_ChunkParserPatcher
{
public:
	SNM_RecPassParser(MediaItem* _item) : SNM_ChunkParserPatcher(_item) 
	{
		m_maxRecPass = -1;
		m_takeCounter = 0;
	}
	~SNM_RecPassParser() {}
	int GetMaxRecPass(int* _recPasses=NULL, int* _takeColors=NULL);

protected:
	bool NotifyChunkLine(int _mode, 
		LineParser* _lp, const char* _parsedLine, int _linePos,
		int _parsedOccurence, WDL_PtrList<WDL_String>* _parsedParents,
		WDL_String* _newChunk, int _updates);

	int m_maxRecPass;
	int m_recPasses[SNM_MAX_TAKES];
	int m_takeColors[SNM_MAX_TAKES];

private:
	int m_takeCounter;
};


///////////////////////////////////////////////////////////////////////////////
// SNM_ArmEnvParserPatcher
///////////////////////////////////////////////////////////////////////////////

class SNM_ArmEnvParserPatcher : public SNM_ChunkParserPatcher
{
public:
	SNM_ArmEnvParserPatcher(MediaTrack* _tr) : SNM_ChunkParserPatcher(_tr) {
		m_newValue = -1; // i.e. toggle
	}
	~SNM_ArmEnvParserPatcher() {}
	void SetNewValue(int _newValue) {m_newValue = _newValue;}

protected:
	bool NotifyChunkLine(int _mode, 
		LineParser* _lp, const char* _parsedLine, int _linePos,
		int _parsedOccurence, WDL_PtrList<WDL_String>* _parsedParents,
		WDL_String* _newChunk, int _updates);
	bool IsParentEnv(const char* _parent);
private:
	int m_newValue;
};


///////////////////////////////////////////////////////////////////////////////
// SNM_FXSummaryParser
///////////////////////////////////////////////////////////////////////////////

class SNM_FXSummaryParser : public SNM_ChunkParserPatcher
{
public:
	SNM_FXSummaryParser(MediaTrack* _tr) : SNM_ChunkParserPatcher(_tr) {}
	~SNM_FXSummaryParser() {}
	WDL_PtrList<SNM_FXSummary>* GetSummaries();

protected:
	bool NotifyStartElement(int _mode, 
		LineParser* _lp, const char* _parsedLine, int _linePos,
		WDL_PtrList<WDL_String>* _parsedParents, 
		WDL_String* _newChunk, int _updates);
	bool NotifyEndElement(int _mode, 
		LineParser* _lp, const char* _parsedLine, int _linePos,
		WDL_PtrList<WDL_String>* _parsedParents, 
		WDL_String* _newChunk, int _updates);

	WDL_PtrList_DeleteOnDestroy<SNM_FXSummary> m_summaries;
};


///////////////////////////////////////////////////////////////////////////////
// SNM_FXPresetParserPatcher
//
// SetPresets(): e.g. "1.4 2.2" => FX1 preset4, FX2 preset2 
// "1.0 2.0" could clear FX1 and FX2 presets, not yet managed though..
//
///////////////////////////////////////////////////////////////////////////////

class SNM_FXPresetParserPatcher : public SNM_ChunkParserPatcher
{
public:
	SNM_FXPresetParserPatcher(MediaTrack* _tr) : SNM_ChunkParserPatcher(_tr) {
		m_presetFound = false;
		m_fx = 0;
	}
	~SNM_FXPresetParserPatcher() {}
	bool SetPresets(WDL_String* _presetConf);

protected:
	bool NotifyEndElement(int _mode, 
		LineParser* _lp, const char* _parsedLine, int _linePos,
		WDL_PtrList<WDL_String>* _parsedParents, 
		WDL_String* _newChunk, int _updates);
	bool NotifyChunkLine(int _mode, 
		LineParser* _lp, const char* _parsedLine, int _linePos,
		int _parsedOccurence, WDL_PtrList<WDL_String>* _parsedParents,
		WDL_String* _newChunk, int _updates);
private:
	WDL_String* m_presetConf;
	bool m_presetFound;
	int m_fx;
};


///////////////////////////////////////////////////////////////////////////////
// SNM_TakeEnvParserPatcher
///////////////////////////////////////////////////////////////////////////////

class SNM_TakeEnvParserPatcher : public SNM_ChunkParserPatcher
{
public:
	SNM_TakeEnvParserPatcher(WDL_String* _tkChunk) : SNM_ChunkParserPatcher(_tkChunk) {m_vis = -1;}
	~SNM_TakeEnvParserPatcher() {}
	bool SetVis(const char* _envKeyWord, int _vis);

protected:
	bool NotifyChunkLine(int _mode, 
		LineParser* _lp, const char* _parsedLine, int _linePos,
		int _parsedOccurence, WDL_PtrList<WDL_String>* _parsedParents,
		WDL_String* _newChunk, int _updates);
private:
	int m_vis;
};


///////////////////////////////////////////////////////////////////////////////
// SNM_FXKnobParser
///////////////////////////////////////////////////////////////////////////////

class SNM_FXKnobParser : public SNM_ChunkParserPatcher
{
public:
	SNM_FXKnobParser(MediaTrack* _tr) : SNM_ChunkParserPatcher(_tr) {m_fx = -1;}
	~SNM_FXKnobParser() {}
	bool SNM_FXKnobParser::GetKnobs(WDL_PtrList<WDL_IntKeyedArray<int> >* _knobs);

protected:
	bool SNM_FXKnobParser::NotifyEndElement(int _mode, 
		LineParser* _lp, const char* _parsedLine, int _linePos,
		WDL_PtrList<WDL_String>* _parsedParents, 
		WDL_String* _newChunk, int _updates);

	bool NotifyChunkLine(int _mode, 
		LineParser* _lp, const char* _parsedLine, int _linePos,
		int _parsedOccurence, WDL_PtrList<WDL_String>* _parsedParents,
		WDL_String* _newChunk, int _updates);

private:
	int m_fx;
	WDL_PtrList<WDL_IntKeyedArray<int> >* m_knobs;
};


#endif