/******************************************************************************
/ SNM_Chunk.cpp
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

#include "stdafx.h" 
#include "SnM_Actions.h"
#include "SNM_Chunk.h"

/*JFB TODO: 
- more m_breakParsePatch/_breakingKeyword
- check _mode < 0
- return &m_wdlStrings (rather than copies)
- use m_isParsingSource
- Parse(-1) > 0
*/

///////////////////////////////////////////////////////////////////////////////
// SNM_SendPatcher
// We don't need to implement NotifySkippedSubChunk() because we're working
// on single lines (not sub-chunks)
///////////////////////////////////////////////////////////////////////////////

bool SNM_SendPatcher::NotifyChunkLine(int _mode, 
	LineParser* _lp, const char* _parsedLine, int _linePos, 
	int _parsedOccurence, WDL_PtrList<WDL_String>* _parsedParents,  
	WDL_String* _newChunk, int _updates)
{
	bool update = false;
	switch(_mode)
	{
		// add rcv
		case -1:
		{
			char bufline[512] = "";
			int n = sprintf(bufline, 
				"AUXRECV %d %d %s %s 0 0 0 0 0 -1.00000000000000 0 -1\n%s\n", 
				m_srcId-1, m_sendType, 
				m_vol, m_pan,
				_parsedLine);
			_newChunk->Append(bufline,n);

			m_breakParsePatch = true;
			update = true;
		}
		break;

		// remove rcv
		case -2:
		{
			update = (m_srcId == -1 || _lp->gettoken_int(1) == (m_srcId - 1));
			// update => nothing! we do NOT re-copy this receive
		}
		break;

		// add "detailed" receive
		case -3:
		{
			char bufline[512] = "";
			int n = sprintf(bufline, 
				"AUXRECV %d %d %.14f %.14f %d %d %d %d %d %.14f %d %d\n%s\n", 
				m_srcId-1, 
				m_sendType, 
				m_sndRcv->m_vol, 
				m_sndRcv->m_pan,
				m_sndRcv->m_mute,
				m_sndRcv->m_mono,
				m_sndRcv->m_phase,
				m_sndRcv->m_srcChan,
				m_sndRcv->m_destChan,
				m_sndRcv->m_panl,
				m_sndRcv->m_midi,
				-1, //JFB: can't get -send/rcv- automation!
				_parsedLine);
			_newChunk->Append(bufline,n);

			update = true;
			m_breakParsePatch = true;
		}
		break;

		default:
			break;
	}
	return update; 
}

int SNM_SendPatcher::AddReceive(MediaTrack* _srcTr, int _sendType, char* _vol, char* _pan) 
{
	m_srcId = _srcTr ? CSurf_TrackToID(_srcTr, false) : -1;
	if (m_srcId == -1)
		return 0; 
	m_sendType = _sendType;
	m_vol = _vol;
	m_pan = _pan;
	m_sndRcv = NULL;
	return ParsePatch(-1, 1, "TRACK", "MIDIOUT");
}

bool SNM_SendPatcher::AddReceive(MediaTrack* _srcTr, SNM_SndRcv* _io)
{
	m_srcId = _srcTr ? CSurf_TrackToID(_srcTr, false) : -1;
	if (!_io || m_srcId == -1)
		return false; 
	m_sendType = _io->m_mode;
	m_vol = NULL;
	m_pan = NULL;
	m_sndRcv = _io;
	return (ParsePatch(-3, 1, "TRACK", "MIDIOUT") > 0);
}

int SNM_SendPatcher::RemoveReceives() 
{
	m_srcId = -1; // -1 to remove all receives
	m_sendType = 2; // deprecated
	m_vol = NULL;
	m_pan = NULL;
	m_sndRcv = NULL;
	return ParsePatch(-2, 1, "TRACK", "AUXRECV");
}

int SNM_SendPatcher::RemoveFirstReceive(MediaTrack* _srcTr) 
{
	m_srcId = _srcTr ? CSurf_TrackToID(_srcTr, false) : -1;
	if (m_srcId == -1)
		return 0; // 'cause what follow would remove all receives!
	m_sendType = 2; // deprecated
	m_vol = NULL;
	m_pan = NULL;
	m_sndRcv = NULL;
	return ParsePatch(-2, 1, "TRACK", "AUXRECV");
}

//JFB TODO: in one go
// facility method
int SNM_SendPatcher::RemoveReceivesFrom(MediaTrack* _srcTr) 
{
	int updates = 0, lastUpdate = 0;
	if (_srcTr)
	{
		updates = lastUpdate = RemoveFirstReceive(_srcTr);
		while (lastUpdate > 0)
		{
			lastUpdate = RemoveFirstReceive(_srcTr);
			updates += lastUpdate;
		}
	}
	return updates;
}


///////////////////////////////////////////////////////////////////////////////
// SNM_FXChainTakePatcher
///////////////////////////////////////////////////////////////////////////////

bool SNM_FXChainTakePatcher::NotifyStartElement(int _mode, 
	LineParser* _lp, const char* _parsedLine, int _linePos, 
	WDL_PtrList<WDL_String>* _parsedParents, 
	WDL_String* _newChunk, int _updates)
{
	// start to *not* recopy
	if ((_mode == -1 || (m_activeTake && _mode == -2)) && !strcmp(GetParent(_parsedParents), "TAKEFX"))
	{
		m_removingTakeFx = true;
	}
	else if (_mode == -3 && m_activeTake && !strcmp(GetParent(_parsedParents), "TAKEFX"))
	{
		m_copyingTakeFx = true;
	}
	return m_removingTakeFx; 
}


bool SNM_FXChainTakePatcher::NotifyEndElement(int _mode, 
	LineParser* _lp, const char* _parsedLine, int _linePos, 
	WDL_PtrList<WDL_String>* _parsedParents, 
	WDL_String* _newChunk, int _updates)
{
	bool update = m_removingTakeFx;
	if ((_mode == -1 || (m_activeTake && _mode == -2)) && !strcmp(GetParent(_parsedParents), "SOURCE")) 
	{
		// If there's a chain to add, add it!
		if (m_fxChain != NULL)
		{
			update=true;
			_newChunk->Append(">\n");// ie write eof "SOURCE"
			makeChunkTakeFX(_newChunk, m_fxChain);
		}
	}
	else if (m_removingTakeFx && 
		(_mode == -1 || (m_activeTake && _mode == -2)) && !strcmp(GetParent(_parsedParents), "TAKEFX")) 
	{
		m_removingTakeFx = false; // re-enable copy
		// note: "update" stays true if m_removingFxChain was initially true, 
		// i.e. we don't copy the end of chunk ">"
	}
	else if (_mode == -3 && m_activeTake && !strcmp(GetParent(_parsedParents), "TAKEFX")) 
	{
		m_copyingTakeFx = false;
	}

	return update;
}

// _mode: -1 set active ALL takes FX chain, -2 set active take's FX chain, -3 copy active take's FX chain
bool SNM_FXChainTakePatcher::NotifyChunkLine(int _mode, 
	LineParser* _lp, const char* _parsedLine, int _linePos,
	int _parsedOccurence, WDL_PtrList<WDL_String>* _parsedParents, 
	WDL_String* _newChunk, int _updates)
{
	bool update = m_removingTakeFx;

	// "tag" the active take (m_activeTake must be initialized with CountTake == 1!)
	if((_mode == -2 || _mode == -3) && !strcmp(_lp->gettoken_str(0), "TAKE"))
		m_activeTake = (_lp->getnumtokens() > 1 && !strcmp(_lp->gettoken_str(1), "SEL"));

	// copy active FX chain
	if (_mode == -3 && m_copyingTakeFx)
	{
		// "<TAKEFX" not part of .RfxChain files
		if (strcmp(_lp->gettoken_str(0), "<TAKEFX") != 0) 
		{
			m_copiedFXChain.Append(_parsedLine);
			m_copiedFXChain.Append("\n");
		}
	}
	return update; 
}

bool SNM_FXChainTakePatcher::NotifySkippedSubChunk(int _mode, 
	const char* _subChunk, int _subChunkLength, int _subChunkPos,
	WDL_PtrList<WDL_String>* _parsedParents, 
	WDL_String* _newChunk, int _updates)
{
	if (_mode == -3 && m_copyingTakeFx)
		m_copiedFXChain.Insert(_subChunk, m_copiedFXChain.GetLength(), _subChunkLength);
	return m_removingTakeFx; 
}

// _fxChain == NULL clears current FX chain(s)
bool SNM_FXChainTakePatcher::SetFXChain(WDL_String* _fxChain, bool _activeTakeOnly)
{
	m_fxChain = _fxChain;
	m_removingTakeFx = false;
	m_activeTake = (*(int*)GetSetMediaItemInfo((MediaItem*)m_object, "I_CURTAKE", NULL) == 0);
	// ParsePatch(): we can't specify more parameters, the parser has to go in depth
	return (ParsePatch((_activeTakeOnly && GetMediaItemNumTakes((MediaItem*)m_object) > 1) ? -2 : -1) > 0);
	return false;
}

WDL_String* SNM_FXChainTakePatcher::GetFXChain()
{
	m_activeTake = (*(int*)GetSetMediaItemInfo((MediaItem*)m_object, "I_CURTAKE", NULL) == 0);
	m_copiedFXChain.Set("");
	m_copyingTakeFx = false;
	// Parse(): we can't specify more parameters, the parser has to go in depth
	if (Parse(-3) >= 0 && // We can't check  "> 0" as usual due to *READ-ONLY* custom negative mode..
		m_copiedFXChain.GetLength()) // .. here's the check
	{
		return &m_copiedFXChain;
	}
	return NULL;
}


///////////////////////////////////////////////////////////////////////////////
// SNM_FXChainTrackPatcher
///////////////////////////////////////////////////////////////////////////////

bool SNM_FXChainTrackPatcher::NotifyStartElement(int _mode, 
	LineParser* _lp, const char* _parsedLine, int _linePos,
	WDL_PtrList<WDL_String>* _parsedParents, 
	WDL_String* _newChunk, int _updates)
{
	// start to *not* recopy
	m_removingFxChain |= (_mode == -1 && !strcmp(GetParent(_parsedParents), "FXCHAIN"));
	return m_removingFxChain; 
}

bool SNM_FXChainTrackPatcher::NotifyEndElement(int _mode, 
	LineParser* _lp, const char* _parsedLine, int _linePos,
	WDL_PtrList<WDL_String>* _parsedParents, 
	WDL_String* _newChunk, int _updates)
{
	bool update = m_removingFxChain;
	if (m_removingFxChain && _mode == -1 && !strcmp(GetParent(_parsedParents), "FXCHAIN"))
	{
		m_removingFxChain = false; // re-enable copy
		m_breakParsePatch = true; // optmization
	}
	return update; 

	// note: "update" is true if m_removingFxChain was initially true, 
	//       i.e. do not recopy end of "FXCHAIN" chunk '>'
}

// _mode -1: set FX chain 
bool SNM_FXChainTrackPatcher::NotifyChunkLine(int _mode, 
	LineParser* _lp, const char* _parsedLine, int _linePos,
	int _parsedOccurence, WDL_PtrList<WDL_String>* _parsedParents, 
	WDL_String* _newChunk, int _updates)
{
	bool update = m_removingFxChain;
	if(_mode == -1 && !strcmp(_lp->gettoken_str(0), "MAINSEND"))
	{
		update=true;
		_newChunk->Append(_parsedLine); // write the "MAINSEND" line
		_newChunk->Append("\n");
		_newChunk->Append("<FXCHAIN\n");
//			_newChunk->Append("WNDRECT 0 0 0 0\n"); 
		_newChunk->Append("SHOW 0\n"); // un-float fx chain window
		_newChunk->Append("LASTSEL 1\n");
		_newChunk->Append("DOCKED 0\n");
		if (m_fxChain)
			_newChunk->Append(m_fxChain->Get());
		_newChunk->Append(">\n");
	}
	return update; 
}

bool SNM_FXChainTrackPatcher::NotifySkippedSubChunk(int _mode, 
	const char* _subChunk, int _subChunkLength, int _subChunkPos,
	WDL_PtrList<WDL_String>* _parsedParents, 
	WDL_String* _newChunk, int _updates)
{
	return m_removingFxChain;
}

// _fxChain == NULL clears current FX chain(s)
bool SNM_FXChainTrackPatcher::SetFXChain(WDL_String* _fxChain)
{
	m_fxChain = _fxChain;
	m_removingFxChain = false;

	// Parse(): we can't specify more parameters, the parser has to go in depth
	return (ParsePatch(-1) > 0); 
}


///////////////////////////////////////////////////////////////////////////////
// SNM_TakeParserPatcher
///////////////////////////////////////////////////////////////////////////////

bool SNM_TakeParserPatcher::NotifyEndElement(int _mode, 
	LineParser* _lp, const char* _parsedLine, int _linePos,
	WDL_PtrList<WDL_String>* _parsedParents, 
	WDL_String* _newChunk, int _updates)
{
	if (_mode < 0 && !strcmp(GetParent(_parsedParents), "ITEM"))
	{
		if (m_removing && _mode == -1)
			m_removing = false; // recopy end of "ITEM" chunk, i.e. '>'
		else if (m_getting && _mode == -2)
			m_getting = false; 
	}
	return m_removing; 
}

// _mode: -1 remove, -2 getTake, -3 countTakes
bool SNM_TakeParserPatcher::NotifyChunkLine(int _mode, 
	LineParser* _lp, const char* _parsedLine, int _linePos,
	int _parsedOccurence, WDL_PtrList<WDL_String>* _parsedParents, 
	WDL_String* _newChunk, int _updates)
{
	if (_mode < 0)
	{
		bool wasWorking = (m_getting || m_removing);
		if ((!m_takeCounter && !strcmp(_lp->gettoken_str(0), "NAME")) || //no "TAKE" in chunks for 1st take!
			(m_takeCounter && !strcmp(_lp->gettoken_str(0), "TAKE"))) 
		{
			if (_mode == -1)
			{
				m_removing = (m_takeCounter == m_searchedTake);
				if (m_removing && m_foundPos == -1)
					m_foundPos = _linePos;
			}
			else if (_mode == -2)
			{
				m_getting = (m_takeCounter == m_searchedTake);
				if (m_getting && m_foundPos == -1)
					m_foundPos = _linePos;
			}
			else if (_mode == -3)
				m_lastTakeCount++; // *always* reflect the nb of takes in the *chunk*

			m_takeCounter++; 
		}

		if (m_getting || m_removing)
			m_subchunk.AppendFormatted(strlen(_parsedLine)+2, "%s\n", _parsedLine); //+2 for osx..
		else if (wasWorking)
			m_breakParsePatch = true; // optimization (job done)
	}
	return m_removing;
}


bool SNM_TakeParserPatcher::NotifySkippedSubChunk(int _mode, 
	const char* _subChunk, int _subChunkLength, int _subChunkPos,
	WDL_PtrList<WDL_String>* _parsedParents, 
	WDL_String* _newChunk, int _updates)
{
	if ((_mode == -1 || _mode == -2) && (m_getting || m_removing))
		m_subchunk.Insert(_subChunk, m_subchunk.GetLength(), _subChunkLength);
	return m_removing;
}

// _pos: if valid, it'll be set to the start position of the _gettedChunk (if found, -1 otherwise)
bool SNM_TakeParserPatcher::GetTakeChunk(int _takeIdx, WDL_String* _gettedChunk, int* _pos, int* _originalLength)
{
	m_searchedTake = _takeIdx;
	m_removing = false;
	m_getting = false; 
	m_foundPos = -1;
	m_takeCounter = 0; 
	m_subchunk.Set("");

	bool found = false;
	if (_gettedChunk &&
		// Parse(): we can't specify more parameters, the parser has to go in depth
		Parse(-2) >= 0 && // We can't check  "> 0" as usual due to *READ-ONLY* custom negative mode..
		m_subchunk.GetLength() > 0) // .. here's the check
	{
		found = true;

		if(_originalLength)
			*_originalLength = m_subchunk.GetLength(); // before adding anything

		if (!_takeIdx) 
			m_subchunk.Insert("TAKE\n", 0, 5);

		_gettedChunk->Set(m_subchunk.Get());

		if (_pos)
			*_pos = m_foundPos;
	}
	return found; 
}

// Different from the API's CountTakes(MediaItem*), this ones
// applies on the chunk (which perharps not yet commited !)
int SNM_TakeParserPatcher::CountTakes() 
{
	if (m_lastTakeCount < 0) { 
		m_takeCounter = 0; // not really necessary..
		m_lastTakeCount = 0;
		Parse(-3, 1, "ITEM"); // we just look for "TAKE" (or "NAME") here) 
	}
	return m_lastTakeCount;
}

// different from checking the PCM source: this one can 
// apply on a chunk that's not committed yet
bool SNM_TakeParserPatcher::IsEmpty(int _takeIdx)
{
	char readSource[128];
	if (_takeIdx >=0 && _takeIdx < CountTakes() && 
		(Parse(SNM_GET_CHUNK_CHAR,2,"SOURCE","<SOURCE",2,_takeIdx,1,readSource) > 0))
	{
		return !strcmp(readSource,"EMPTY");
	}
	return false;
}

// assumes the takes begins with "TAKE" and that ids have been removed
// in the provided chunk
// returns the end position after insertion (or -1 if failed)
int SNM_TakeParserPatcher::AddLastTake(WDL_String* _chunk)
{
	int afterPos = -1;
	int length = _chunk->GetLength();
	if (_chunk && length)
	{
		WDL_String* chunk = GetChunk();
		chunk->Insert(_chunk->Get(), chunk->GetLength()-2, length); //-2: before ">\n"
		afterPos = chunk->GetLength()-2;

		// as we're directly working on the cached chunk..
		m_lastTakeCount++; // *always* reflect the nb of takes in the *chunk*
		m_updates++;
	}
	return afterPos;
}

// assumes _chunk always begins with "TAKE" (removed if needed, i.e. inserted as 1st take)
// and that ids have been removed in the provided chunk
// _pos: start pos of the take if known, for optimization (-1 if unknown)
// returns the end position after insertion (or -1 if failed)
int SNM_TakeParserPatcher::InsertTake(int _takeIdx, WDL_String* _chunk, int _pos)
{
	int afterPos = -1;
	int length = _chunk->GetLength();
	if (_chunk && length)
	{
		// last pos?
		if (_takeIdx >= CountTakes()) {
			afterPos = AddLastTake(_chunk);
		}
		// other pos
		else 
		{
			int pos = _pos;
			if (pos < 0)
			{
				//JFB TODO: SNM_CPP.FindPos();
				pos = Parse(SNM_GET_CHUNK_CHAR, 1, "ITEM", 
					!_takeIdx ? "NAME" : "TAKE", -1, // 'cause variable nb of tokens
					!_takeIdx ? 0 : (_takeIdx-1), // -1 'cause 0 is for "NAME" only
					0); //1st token
				if (pos > 0)
					pos--; // see SNM_ChunkParserPatcher..
			}

			if (pos >= 0)
			{
				if (!_takeIdx)
				{
					GetChunk()->Insert("TAKE\n", pos, 5); // will add it at the end (i.e. begining of the 2nd take)
					char* eol = strchr(_chunk->Get(), '\n'); // zap "TAKE...bla..bla...\n"
					GetChunk()->Insert(eol+1, pos);
					afterPos = pos + strlen(eol) - 1;
				}
				else
				{
					GetChunk()->Insert(_chunk->Get(), pos);
					afterPos = pos + _chunk->GetLength();
				}

				// as we're directly working on the cached chunk..
				m_lastTakeCount++; // *always* reflect the nb of takes in the *chunk*
				m_updates++;
			}
		}
	}
	return afterPos;
}

// important: assumes there're at least 2 takes 
//            => DeleteTrackMediaItem() should be used otherwise
bool SNM_TakeParserPatcher::RemoveTake(int _takeIdx, WDL_String* _removedChunk, int* _removedStartPos)
{
	int updates = 0;
	if (_takeIdx >= 0) 
	{
		m_searchedTake = _takeIdx;
		m_removing = false;
		m_getting = false; 
		m_foundPos = -1;
		m_takeCounter = 0; 
		m_subchunk.Set("");

		// Parse(): we can't specify more parameters, the parser has to go in depth
		updates = ParsePatch(-1);
		if (updates > 0)
		{
			m_lastTakeCount--; // *always* reflect the nb of takes in the *chunk*
			if (_removedChunk) //optionnal
			{
				// add 'header' in the returned _removedChunk
				if (!_takeIdx)	m_subchunk.Insert("TAKE\n", 0, 5);
				_removedChunk->Set(m_subchunk.Get());
			}

			if (_removedStartPos) //optionnal
				*_removedStartPos = m_foundPos;

			// remove the 1st "TAKE" if needed
			if (!_takeIdx)
			{
				if (m_foundPos >= 0)
				{
					char* eol = strchr((char*)(GetChunk()->Get()+m_foundPos), '\n'); // zap "NAME...bla..bla...\n"
					GetChunk()->DeleteSub(m_foundPos, (int)(eol-GetChunk()->Get()+1)-m_foundPos);
				}
			}
		}
	}
	return (updates > 0); 
}

// assumes _newTakeChunk always begins with "TAKE" (removed if needed, i.e. inserted as 1st take)
bool SNM_TakeParserPatcher::ReplaceTake(int _takeIdx, int _startTakePos, int _takeLength, WDL_String* _newTakeChunk)
{
	bool updated = false;
	if (GetChunk() && _newTakeChunk && _startTakePos >= 0) 
	{
		int prevLgth = GetChunk()->GetLength(); // simple getter
		GetChunk()->DeleteSub(_startTakePos, _takeLength);
		if (prevLgth > GetChunk()->GetLength()) // see WDL_String.DeleteSub()
		{
			//JFB TODO: search for '\n' rather than +5 (safer)
			GetChunk()->Insert(!_takeIdx ? _newTakeChunk->Get()+5 : _newTakeChunk->Get(), _startTakePos, _newTakeChunk->GetLength()); // +5 for "TAKE\n"
			SetUpdates(1); // as we're directly working on the cached chunk..
			updated = true;
		}
	}
	return updated;
}


///////////////////////////////////////////////////////////////////////////////
// SNM_RecPassParser
///////////////////////////////////////////////////////////////////////////////

bool SNM_RecPassParser::NotifyChunkLine(int _mode, 
	LineParser* _lp, const char* _parsedLine, int _linePos,
	int _parsedOccurence, WDL_PtrList<WDL_String>* _parsedParents, 
	WDL_String* _newChunk, int _updates)
{
	if (_mode == -1)
	{
		if ((!m_takeCounter && !strcmp(_lp->gettoken_str(0), "NAME")) || //no "TAKE" in chunks for 1st take!
			(m_takeCounter && !strcmp(_lp->gettoken_str(0), "TAKE"))) 
		{
			m_takeCounter++;
		}
		else if (!strcmp(_lp->gettoken_str(0), "RECPASS"))
		{
			int success, currentRecPass = _lp->gettoken_int(1, &success);
			if (success)
			{
				m_maxRecPass = max(currentRecPass, m_maxRecPass);
				// check if that RECPASS isn't duplicated
				bool duplicated = false;
				for (int i=0; i < m_takeCounter; i++) {
					if (m_recPasses[i] == currentRecPass) {
						duplicated = true;
						break;
					}
				}

				if (!duplicated)
					m_recPasses[m_takeCounter-1] = currentRecPass;
			}
		}
		else if (!strcmp(_lp->gettoken_str(0), "TAKECOLOR"))
		{
			int success, currentColor = _lp->gettoken_int(1, &success);
			if (success)
				m_takeColors[m_takeCounter-1] = currentColor;
		}
	}
	return false;
}

int SNM_RecPassParser::GetMaxRecPass(int* _recPasses, int* _takeColors)
{
	m_maxRecPass = -1;
	m_takeCounter = 0;
	memset(m_recPasses, 0, SNM_MAX_TAKES*sizeof(int));
	memset(m_takeColors, 0, SNM_MAX_TAKES*sizeof(int));
	if (Parse(-1, 1, "ITEM") >= 0)
	{
		if (_recPasses)
			memcpy(_recPasses, m_recPasses, SNM_MAX_TAKES*sizeof(int));
		if (_takeColors)
			memcpy(_takeColors, m_takeColors, SNM_MAX_TAKES*sizeof(int));
		return m_maxRecPass;
	}
	return -1;
}


///////////////////////////////////////////////////////////////////////////////
// SNM_ArmEnvParserPatcher
///////////////////////////////////////////////////////////////////////////////

// _mode: -1 all envelopes, -2 receive vol envelopes, -3 receive pan envelopes, -4 receive mute envelopes
bool SNM_ArmEnvParserPatcher::NotifyChunkLine(int _mode, 
	LineParser* _lp, const char* _parsedLine, int _linePos,
	int _parsedOccurence, WDL_PtrList<WDL_String>* _parsedParents, 
	WDL_String* _newChunk, int _updates)
{
	bool updated = false;
	if (_mode < 0)
	{
		if (_lp->getnumtokens() == 2 && _parsedParents->GetSize() > 1 && 
			!strcmp(_lp->gettoken_str(0), "ARM"))
		{
			const char* grandpa = GetParent(_parsedParents,2);
			if (!strcmp(grandpa, "TRACK") || !strcmp(grandpa, "FXCHAIN")) //not to scratch item env for ex.
			{
				const char* parent = GetParent(_parsedParents);
				// most "frequent" first!
				if ((_mode == -1 && IsParentEnv(parent)) || 
					(_mode == -2 && !strcmp(parent, "AUXVOLENV")) ||
					(_mode == -3 && !strcmp(parent, "AUXPANENV")) ||
					(_mode == -4 && !strcmp(parent, "AUXMUTEENV")) ||
					(_mode == -5 && !strcmp(parent, "PARMENV")))
				{
					if (m_newValue == -1)
						_newChunk->AppendFormatted(5+2, "ARM %d\n", !_lp->gettoken_int(1) ? 1 : 0); //+2 for OSX
					else
						_newChunk->AppendFormatted(5+2, "ARM %d\n", m_newValue); //+2 for OSX
					updated = true;
				}
			}
		}
	}
	return updated;
}

bool SNM_ArmEnvParserPatcher::IsParentEnv(const char* _parent)
{
	return (!strcmp(_parent, "PARMENV") ||
		!strcmp(_parent, "VOLENV2") ||
		!strcmp(_parent, "PANENV2") ||
		!strcmp(_parent, "VOLENV") ||
		!strcmp(_parent, "PANENV") ||
		!strcmp(_parent, "MUTEENV") ||
		!strcmp(_parent, "AUXVOLENV") ||
		!strcmp(_parent, "AUXPANENV") ||
		!strcmp(_parent, "AUXMUTEENV") ||
		!strcmp(_parent, "MASTERVOLENV") ||
		!strcmp(_parent, "MASTERPANENV") ||
		!strcmp(_parent, "MASTERVOLENV2") ||
		!strcmp(_parent, "MASTERPANENV2"));
}


///////////////////////////////////////////////////////////////////////////////
// SNM_FXSummaryParser
///////////////////////////////////////////////////////////////////////////////

bool SNM_FXSummaryParser::NotifyStartElement(int _mode, 
	LineParser* _lp, const char* _parsedLine, int _linePos,
	WDL_PtrList<WDL_String>* _parsedParents, 
	WDL_String* _newChunk, int _updates)
{
	if (_mode == -1)
	{
		if (_lp->getnumtokens() >= 3 && _parsedParents->GetSize() == 3)
		{
			if (!strcmp(_lp->gettoken_str(0), "<VST"))
				m_summaries.Add(new SNM_FXSummary(_lp->gettoken_str(0)+1, _lp->gettoken_str(2)));
			else if (!strcmp(_lp->gettoken_str(0), "<JS") || !strcmp(_lp->gettoken_str(0), "<DX"))
				m_summaries.Add(new SNM_FXSummary(_lp->gettoken_str(0)+1, _lp->gettoken_str(1)));
		}
	}
	return false;
}

bool SNM_FXSummaryParser::NotifyEndElement(int _mode, 
	LineParser* _lp, const char* _parsedLine, int _linePos,
	WDL_PtrList<WDL_String>* _parsedParents, 
	WDL_String* _newChunk, int _updates)
{
	if (_mode == -1 && !strcmp(GetParent(_parsedParents), "FXCHAIN"))
		m_breakParsePatch = true; // optmization
	return false; 
}

WDL_PtrList<SNM_FXSummary>* SNM_FXSummaryParser::GetSummaries()
{
	m_summaries.Empty(true);
	if (Parse(-1) >= 0)
		return &m_summaries;
	return NULL;
}


///////////////////////////////////////////////////////////////////////////////
// SNM_FXPresetParserPatcher
///////////////////////////////////////////////////////////////////////////////

bool SNM_FXPresetParserPatcher::NotifyEndElement(int _mode, 
	LineParser* _lp, const char* _parsedLine, int _linePos,
	WDL_PtrList<WDL_String>* _parsedParents, 
	WDL_String* _newChunk, int _updates)
{
	if (_mode == -1 && !strcmp(GetParent(_parsedParents), "FXCHAIN"))
		m_breakParsePatch = true; // optmization
	return false; 
}

bool SNM_FXPresetParserPatcher::NotifyChunkLine(int _mode, 
	LineParser* _lp, const char* _parsedLine, int _linePos,
	int _parsedOccurence, WDL_PtrList<WDL_String>* _parsedParents, 
	WDL_String* _newChunk, int _updates)
{
	bool updated = false;
	if (_mode == -1)
	{
		if (_lp->getnumtokens() == 2 && !strcmp(_lp->gettoken_str(0), "LASTPRESET"))
		{
			int preset = GetSelPresetFromConf(m_fx, m_presetConf);
			if (preset && preset != _lp->gettoken_int(1)) 
			{
				_newChunk->AppendFormatted(32, "LASTPRESET %d\n", preset);
				updated = true;
			}
			m_presetFound = true;
		}
		else if (_lp->getnumtokens() == 5 && !strcmp(_lp->gettoken_str(0), "FLOATPOS"))
		{
			if (!m_presetFound)
			{
				int preset = GetSelPresetFromConf(m_fx, m_presetConf);
				if (preset)
				{
					_newChunk->AppendFormatted(128, "LASTPRESET %d\n%s\n", preset, _parsedLine);
					updated = true;
				}
			}

			// prepare following fx
			m_fx++;
			m_presetFound = false;
		}
	}
	return updated;
}

bool SNM_FXPresetParserPatcher::SetPresets(WDL_String* _presetConf)
{
	m_presetConf = _presetConf;
	m_presetFound = false;
	m_fx = 0;

	if (ParsePatch(-1,2,"FXCHAIN") > 0)
		return true;
	return false;
}


///////////////////////////////////////////////////////////////////////////////
// SNM_TakeEnvParserPatcher
///////////////////////////////////////////////////////////////////////////////

bool SNM_TakeEnvParserPatcher::NotifyChunkLine(int _mode, 
	LineParser* _lp, const char* _parsedLine, int _linePos,
	int _parsedOccurence, WDL_PtrList<WDL_String>* _parsedParents, 
	WDL_String* _newChunk, int _updates)
{
	bool updated = false;
	if (_mode == -1)
	{
		bool arm = false;
		updated = (!strcmp(_lp->gettoken_str(0), "ACT") || !strcmp(_lp->gettoken_str(0), "VIS"));
		arm = (strcmp(_lp->gettoken_str(0), "ARM") == 0);
		updated |= arm;
		if (updated) {
			_newChunk->AppendFormatted(32, "%s %d\n", _lp->gettoken_str(0), m_vis); // 32 for OSX
			m_breakParsePatch = arm;
		}
	}
	return updated;
}

bool SNM_TakeEnvParserPatcher::SetVis(const char* _envKeyWord, int _vis) {
	m_vis = _vis;
	return (ParsePatch(-1, 1, _envKeyWord) > 0);
}


///////////////////////////////////////////////////////////////////////////////
// SNM_FXKnobParser
///////////////////////////////////////////////////////////////////////////////

bool SNM_FXKnobParser::NotifyEndElement(int _mode, 
	LineParser* _lp, const char* _parsedLine, int _linePos,
	WDL_PtrList<WDL_String>* _parsedParents, 
	WDL_String* _newChunk, int _updates)
{
	if (_mode == -1 && !strcmp(GetParent(_parsedParents), "FXCHAIN"))
		m_breakParsePatch = true; // optmization
	return false; 
}

bool SNM_FXKnobParser::NotifyChunkLine(int _mode, 
	LineParser* _lp, const char* _parsedLine, int _linePos,
	int _parsedOccurence, WDL_PtrList<WDL_String>* _parsedParents, 
	WDL_String* _newChunk, int _updates)
{
	bool updated = false;
	if (_mode == -1)
	{
		if (_lp->getnumtokens() == 2 && !strcmp(_lp->gettoken_str(0), "FXID"))
		{
			m_knobs->Add(new WDL_IntKeyedArray<int>());
			m_fx++;
		}
		if (_lp->getnumtokens() >= 2 && !strcmp(_lp->gettoken_str(0), "PARM_TCP"))
		{
			for (int i=1; i < _lp->getnumtokens(); i++)
				m_knobs->Get(m_fx)->Insert(i-1, _lp->gettoken_int(i));
		}
	}
	return updated;
}

bool SNM_FXKnobParser::GetKnobs(WDL_PtrList<WDL_IntKeyedArray<int> >* _knobs)
{
	m_fx = -1;
	if (_knobs)
	{
		m_knobs = _knobs;
		m_knobs->Empty(true);
		return (ParsePatch(-1,2,"FXCHAIN") >= 0);
	}
	return false;
}
