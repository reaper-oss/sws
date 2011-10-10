/******************************************************************************
/ SNM_Chunk.h
/ 
/ Some "SAX-ish like" parser classes inheriting SNM_ChunkParserPatcher
/
/ Copyright (c) 2009-2011 Tim Payne (SWS), Jeffos
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

	bool SetFXChain(WDL_String* _fxChain, bool _inputFX = false);
	int GetInputFXCount();

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
	bool m_removingFxChain;
	bool m_copyingFxChain;
};


///////////////////////////////////////////////////////////////////////////////
// SNM_TakeParserPatcher
// JFB TODO? 
//  maintain m_activeTakeIdx when updating takes
//  + GetSetMediaItemInfo((MediaItem*)m_object, "I_CURTAKE", &m_activeTakeIdx)
//  on commit ? ATM, we let REAPER manage the active take, seems ok so far..
///////////////////////////////////////////////////////////////////////////////

class SNM_TakeParserPatcher : public SNM_ChunkParserPatcher
{
public:
	SNM_TakeParserPatcher(MediaItem* _item, int _countTakes = -1) : SNM_ChunkParserPatcher(_item) {
		m_currentTakeCount = _countTakes; // lazy init through CountTakesInChunk() for optimization
		m_fakeTake = false;
	}
	// Call to Commit(): when a constructor or destructor calls a virtual 
	// function it calls the function defined for the type whose constructor
	// or destructor is currently being run
	~SNM_TakeParserPatcher() {
		if (m_autoCommit)
			Commit(); // no-op if chunk not updated (or no valid m_object)
	}
	WDL_String* GetChunk();
	bool Commit(bool _force = false);
	bool GetTakeChunkPos(int _takeIdx, int* _pos, int* _len = NULL);
	bool GetTakeChunk(int _takeIdx, WDL_String* _gettedChunk, int* _pos = NULL, int* _len = NULL);
	int CountTakesInChunk();
	bool IsEmpty(int _takeIdx);
	int AddLastTake(WDL_String* _tkChunk);
	int InsertTake(int _takeIdx, WDL_String* _chunk, int _pos = -1);
	bool RemoveTake(int _takeIdx, WDL_String* _removedChunk = NULL, int* _removedStartPos = NULL);
	bool ReplaceTake(int _startTakePos, int _takeLength, WDL_String* _newTakeChunk);
protected:
	int m_currentTakeCount; // nb of takes in the *chunk* (may be different than REAPER's ones)
//	int m_activeTakeIdx;    // active take in the *chunk* (may be different than REAPER's ones)
private:
	// check that _pLine is indeed the 1st line of a new take in an item chunk
	// remarks:
	// _pLine *MUST* start with "\nTAKE" (see private usages)
	// also, we assume we're processing a valid chunk here (i.e. doesn't end with "\nTAKE")
	bool IsValidTakeChunkLine(const char* _pLine) {return (_pLine[5] && (_pLine[5] == '\n' || _pLine[5] == ' '));}
	bool m_fakeTake;
};


///////////////////////////////////////////////////////////////////////////////
// SNM_RecPassParser
// Inherits SNM_TakeParserPatcher in order to ease processing
///////////////////////////////////////////////////////////////////////////////

class SNM_RecPassParser : public SNM_TakeParserPatcher
{
public:
	SNM_RecPassParser(MediaItem* _item, int _countTakes) : SNM_TakeParserPatcher(_item, _countTakes) {
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
// SNM_EnvRemover
///////////////////////////////////////////////////////////////////////////////

class SNM_EnvRemover : public SNM_ChunkParserPatcher
{
public:
	SNM_EnvRemover(MediaTrack* _tr) : SNM_ChunkParserPatcher(_tr) {
		m_removingEnv = false;
	}
	~SNM_EnvRemover() {}
	bool RemoveEnvelopes();
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
private:
	bool m_removingEnv;
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
private:
	int m_newValue;
};


///////////////////////////////////////////////////////////////////////////////
// SNM_LearnMIDIChPatcher
///////////////////////////////////////////////////////////////////////////////

class SNM_LearnMIDIChPatcher : public SNM_ChunkParserPatcher
{
public:
	SNM_LearnMIDIChPatcher(MediaTrack* _tr) : SNM_ChunkParserPatcher(_tr) {
		m_newChannel = -1;
		m_fx = -1; // i.e. all FX
		m_currentFx = -1;
	}
	~SNM_LearnMIDIChPatcher() {}
	bool SetChannel(int _newValue, int _fx);
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
	int m_newChannel, m_fx, m_currentFx;
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


#ifdef _SNM_MISC // deprecated since v4: GetTCPFXParm(), etc..

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

#endif