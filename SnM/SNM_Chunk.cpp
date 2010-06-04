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
#include "SNM_Chunk.h"


///////////////////////////////////////////////////////////////////////////////
// SNM_SendPatcher
///////////////////////////////////////////////////////////////////////////////

bool SNM_SendPatcher::NotifyChunkLine(int _mode, LineParser* _lp, WDL_String* _parsedLine,  
	int _parsedOccurence, WDL_PtrList<WDL_String>* _parsedParents, const char* _parsedParent, 
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
				_parsedLine->Get());
			_newChunk->Append(bufline,n);
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
				m_sndRcv->vol, 
				m_sndRcv->pan,
				m_sndRcv->mute,
				m_sndRcv->mono,
				m_sndRcv->phase,
				m_sndRcv->srcChan,
				m_sndRcv->destChan,
				m_sndRcv->panl,
				m_sndRcv->midi,
				-1, // TODO: can't get -send/rcv- automation!
				_parsedLine->Get());
			_newChunk->Append(bufline,n);
			update = true;
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

int SNM_SendPatcher::AddReceive(MediaTrack* _srcTr, t_SendRcv* _io) 
{
	m_srcId = _srcTr ? CSurf_TrackToID(_srcTr, false) : -1;
	if (!_io || m_srcId == -1)
		return 0; 
	m_sendType = _io->mode;
	m_vol = NULL;
	m_pan = NULL;
	m_sndRcv = _io;
	return ParsePatch(-3, 1, "TRACK", "MIDIOUT");
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

bool SNM_FXChainTakePatcher::NotifyStartElement(int _mode, LineParser* _lp, WDL_String* _parsedLine,  
		WDL_PtrList<WDL_String>* _parsedParents, const char* _parsedParent, 
		WDL_String* _newChunk, int _updates)
{
	// start to *not* recopy
	if ((_mode == -1 || (m_activeTake && _mode == -2)) && !strcmp(_parsedParent, "TAKEFX"))
	{
		m_removingTakeFx = true;
	}
	else if (_mode == -3 && m_activeTake && !strcmp(_parsedParent, "TAKEFX"))
	{
		m_copyingTakeFx = true;
	}
	return m_removingTakeFx; 
}


bool SNM_FXChainTakePatcher::NotifyEndElement(int _mode, LineParser* _lp, WDL_String* _parsedLine,  
		WDL_PtrList<WDL_String>* _parsedParents, const char* _parsedParent, 
		WDL_String* _newChunk, int _updates)
{
	bool update = m_removingTakeFx;
	if ((_mode == -1 || (m_activeTake && _mode == -2)) && !strcmp(_parsedParent, "SOURCE")) 
	{
		// If there's a chain to add, add it!
		if (m_fxChain != NULL)
		{
			update=true;
			_newChunk->Append(">\n");// ie write eof "SOURCE"
			_newChunk->Append("<TAKEFX\n");
			_newChunk->Append("WNDRECT 0 0 0 0\n"); //38 647 485
			_newChunk->Append("SHOW 0\n"); // un-float fx chain window
			_newChunk->Append("LASTSEL 1\n");
			_newChunk->Append("DOCKED 0\n");
			_newChunk->Append(m_fxChain->Get());
			_newChunk->Append(">\n");
		}
	}
	else if (m_removingTakeFx && 
		(_mode == -1 || (m_activeTake && _mode == -2)) && !strcmp(_parsedParent, "TAKEFX")) 
	{
		m_removingTakeFx = false; // re-enable copy
		// note: "update" stays true if m_removingFxChain was initially true, 
		// i.e. we don't copy the end of chunk ">"
	}
	else if (_mode == -3 && m_activeTake && !strcmp(_parsedParent, "TAKEFX")) 
	{
/*not part of .rfxChain files
		m_copiedFXChain.Append(">\n");
*/
		m_copyingTakeFx = false;
	}

	return update;
}

bool SNM_FXChainTakePatcher::NotifyChunkLine(int _mode, LineParser* _lp, WDL_String* _parsedLine,  
	int _parsedOccurence, WDL_PtrList<WDL_String>* _parsedParents, const char* _parsedParent, 
	WDL_String* _newChunk, int _updates)
{
	bool update = m_removingTakeFx;

	// "tag" the active take (m_activeTake must be initialized with with CountTake == 1!)
	if((_mode == -2 || _mode == -3) && !strcmp(_lp->gettoken_str(0), "TAKE"))
		m_activeTake = (_lp->getnumtokens() > 1 && !strcmp(_lp->gettoken_str(1), "SEL"));

	// copy active FX chain
	if (_mode == -3 && m_copyingTakeFx)
	{
		if (_lp->getnumtokens() > 0 && 
			strcmp(_lp->gettoken_str(0), "FXID") != 0 && // don't copy FXIDs
			strcmp(_lp->gettoken_str(0), "<TAKEFX") != 0) // not part of .RfxChain files
		{
			m_copiedFXChain.Append(_parsedLine->Get());
			m_copiedFXChain.Append("\n");
		}
	}
	return update; 
}

int SNM_FXChainTakePatcher::SetFXChain(WDL_String* _fxChain, bool _activeTakeOnly)
{
	m_fxChain = _fxChain;
	m_removingTakeFx = false;
	m_activeTake = (*(int*)GetSetMediaItemInfo((MediaItem*)m_object, "I_CURTAKE", NULL) == 0);
	return ParsePatch((_activeTakeOnly && GetMediaItemNumTakes((MediaItem*)m_object) > 1) ? -2 : -1);
	return -1;
}

WDL_String* SNM_FXChainTakePatcher::GetFXChain()
{
	m_activeTake = (*(int*)GetSetMediaItemInfo((MediaItem*)m_object, "I_CURTAKE", NULL) == 0);
	m_copiedFXChain.Set("");
	m_copyingTakeFx = false;
	if (Parse(-3) >= 0)
		return &m_copiedFXChain;
	return NULL;
}


///////////////////////////////////////////////////////////////////////////////
// SNM_FXChainTrackPatcher
///////////////////////////////////////////////////////////////////////////////

bool SNM_FXChainTrackPatcher::NotifyStartElement(int _mode, LineParser* _lp, WDL_String* _parsedLine,  
		WDL_PtrList<WDL_String>* _parsedParents, const char* _parsedParent, 
		WDL_String* _newChunk, int _updates)
{
	// start to *not* recopy
	m_removingFxChain |= (_mode == -1 && !strcmp(_parsedParent, "FXCHAIN"));
	return m_removingFxChain; 
}


bool SNM_FXChainTrackPatcher::NotifyEndElement(int _mode, LineParser* _lp, WDL_String* _parsedLine,  
		WDL_PtrList<WDL_String>* _parsedParents, const char* _parsedParent, 
		WDL_String* _newChunk, int _updates)
{
	bool update = m_removingFxChain;
	if (m_removingFxChain && _mode == -1 && !strcmp(_parsedParent, "FXCHAIN"))
		m_removingFxChain = false; // re-enable copy
	return update; 

	// note: "update" is true if m_removingFxChain was initially true, 
	//       i.e. do not recopy end of chunk ">"
}

bool SNM_FXChainTrackPatcher::NotifyChunkLine(int _mode, LineParser* _lp, WDL_String* _parsedLine,  
	int _parsedOccurence, WDL_PtrList<WDL_String>* _parsedParents, const char* _parsedParent, 
	WDL_String* _newChunk, int _updates)
{
	bool update = m_removingFxChain;
	if(m_fxChain && _mode == -1 && !strcmp(_lp->gettoken_str(0), "MAINSEND"))
	{
		update=true;
		_newChunk->Append(_parsedLine->Get()); // write the "MAINSEND" line
		_newChunk->Append("\n");
		_newChunk->Append("<FXCHAIN\n");
//			_newChunk->Append("WNDRECT 0 0 0 0\n"); 
		_newChunk->Append("SHOW 0\n"); // un-float fx chain window
		_newChunk->Append("LASTSEL 1\n");
		_newChunk->Append("DOCKED 0\n");
		_newChunk->Append(m_fxChain->Get());
		_newChunk->Append(">\n");
	}
	return update; 
}

int SNM_FXChainTrackPatcher::SetFXChain(WDL_String* _fxChain)
{
	m_fxChain = _fxChain;
	m_removingFxChain = false;
	return ParsePatch(-1);
}

///////////////////////////////////////////////////////////////////////////////
// SNM_TakeParserPatcher
///////////////////////////////////////////////////////////////////////////////

bool SNM_TakeParserPatcher::NotifyEndElement(int _mode, LineParser* _lp, WDL_String* _parsedLine,  
		WDL_PtrList<WDL_String>* _parsedParents, const char* _parsedParent, 
		WDL_String* _newChunk, int _updates)
{
	bool update = m_removing;
	if (!strcmp(_parsedParent, "ITEM"))
	{
		if (m_removing && _mode == -1)
			m_removing = false; // re-enable copy
		else if (m_getting && _mode == -2)
			m_getting = false; 
	}
	return update; 

	// note: "update" is true if m_removing was initially true, 
	//       i.e. recopy end of chunk ">"
}

bool SNM_TakeParserPatcher::NotifyChunkLine(int _mode, LineParser* _lp, WDL_String* _parsedLine,  
	int _parsedOccurence, WDL_PtrList<WDL_String>* _parsedParents, const char* _parsedParent, 
	WDL_String* _newChunk, int _updates)
{
	if ((!m_lastTakeCount && !strcmp(_lp->gettoken_str(0), "NAME")) || //no "TAKE" in chunks for 1st take!
		(m_lastTakeCount && !strcmp(_lp->gettoken_str(0), "TAKE"))) 
	{
		if (_mode == -1)
			m_removing = (m_lastTakeCount == m_searchedTake);
		else if (_mode == -2)
			m_getting = (m_lastTakeCount == m_searchedTake);
		m_lastTakeCount++;
	}

	if (m_getting || m_removing)
		m_subchunk.AppendFormatted(_parsedLine->GetLength()+2, "%s\n", _parsedLine->Get()); //+2 for osx..

	return m_removing;
}

bool SNM_TakeParserPatcher::GetTakeChunk(int _take, WDL_String* _gettedChunk)
{
	m_searchedTake = _take;
	m_removing = false;
	m_getting = false; 
	m_lastTakeCount = 0;
	m_subchunk.Set("");

	RemoveIds();
	bool found = (Parse(-2) >= 0); //>= 0 'cause we get here; 
	if (m_subchunk.GetLength() > 0)
	{
		if (_gettedChunk)
		{
			if (!_take)
				m_subchunk.Insert("TAKE\n", 0, 5);
			_gettedChunk->Set(m_subchunk.Get());
		}
	}
	else found = false;
	return true; 
}

int SNM_TakeParserPatcher::CountTakes() 
{
	if (m_lastTakeCount < 0) {
		m_lastTakeCount = 0;
		Parse(-3, 1, "ITEM");
	}
	return m_lastTakeCount;
}

bool SNM_TakeParserPatcher::IsEmpty(int _take)
{
	char readSource[128];
	if (_take >=0 && _take < CountTakes() && 
		(Parse(SNM_GET_CHUNK_CHAR,2,"SOURCE","<SOURCE",-1,_take,1,readSource) > 0))
	{
		return !strcmp(readSource,"EMPTY");
	}
	return false;
}

// assumes the takes begins with "TAKE" and that ids have been removed
// in the provided chunk
bool SNM_TakeParserPatcher::AddLastTake(WDL_String* _chunk)
{
	bool updated = false;
	int length = _chunk->GetLength();
	if (_chunk && length)
	{
//		RemoveIds();
		WDL_String* chunk = GetChunk();
		chunk->Insert(_chunk->Get(), chunk->GetLength()-2, length); //-2: before ">\n"
		updated = true;

        // as we're directly working on the cached chunk..
		m_lastTakeCount++;
		m_updates++;
	}
	return updated;
}

// assumes _chunk always begins with "TAKE" (removed if needed, i.e. inserted as 1st take)
// and that ids have been removed in the provided chunk
bool SNM_TakeParserPatcher::InsertTake(int _takePos, WDL_String* _chunk)
{
	bool updated = false;
	int length = _chunk->GetLength();
	if (_chunk && length)
	{
		// last pos?
		if (_takePos >= CountTakes()) {
			updated = AddLastTake(_chunk);
		}
		// other pos
		else 
		{
			char tmp[64] = "";
			int pos = Parse(SNM_GET_CHUNK_CHAR, 1, "ITEM", 
				!_takePos ? "NAME" : "TAKE",
				-1, // 'cause variable nb of tokens
				!_takePos ? 0 : (_takePos-1), // -1 'cause 0 is for "NAME" only
				0, //1st token
				tmp);

			if (pos > 0)
			{
				pos--; // see SNM_ChunkParserPatcher..
				
				WDL_String chunk2(_chunk->Get());

				// delete 1st "TAKE...\n" if needed, but add it at the end
				if (!_takePos)
				{
					char* eol = strchr(chunk2.Get(), '\n');
					chunk2.DeleteSub(0,(int)(eol - chunk2.Get() + 1)); 
					chunk2.Append("TAKE\n");
				}
				GetChunk()->Insert(chunk2.Get(), pos);
//				RemoveIds();
				updated = true;

				// as we're directly working on the cached chunk..
				m_lastTakeCount++;
				m_updates++;
			}
		}
	}
	return updated;
}

// important: assumes there're at least 2 takes 
//            => DeleteTrackMediaItem() should be used otherwise
bool SNM_TakeParserPatcher::RemoveTake(int _take, WDL_String* _removedChunk)
{
	int updates = 0;
	if (_take >= 0)
	{
		m_searchedTake = _take;
		m_removing = false;
		m_getting = false; 
		m_lastTakeCount = 0;
		m_subchunk.Set("");

		RemoveIds();
		updates = ParsePatch(-1);
		if (updates > 0)
		{
			m_lastTakeCount--;
			if (_removedChunk) //optionnal
			{
				// add 'header' in the returned _removedChunk
				if (!_take)
					m_subchunk.Insert("TAKE\n", 0, 5);
				_removedChunk->Set(m_subchunk.Get());
			}

			// remove the 1st "TAKE" if needed
			if (!_take)
				ParsePatch(SNM_REPLACE_SUBCHUNK, 1,"ITEM","TAKE",-1,0,-1,(void*)"");
		}
	}
	return (updates > 0); 
}
