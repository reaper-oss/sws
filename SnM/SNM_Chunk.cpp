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
		// add send
		case -1:
			if (m_srcId > 0)
			{
				_newChunk->Append("AUXRECV ");
				_newChunk->AppendFormatted(4, "%d %d ", m_srcId-1, m_sendType);
				_newChunk->AppendFormatted(33, "%s ", m_vol ? m_vol : "1.00000000000000");
				_newChunk->AppendFormatted(33, "%s ", m_pan ? m_pan : "0.00000000000000");
				_newChunk->Append("0 0 0 0 0 -1.00000000000000 0 -1\n");

				_newChunk->Append(_parsedLine->Get());
				_newChunk->Append("\n");
				update = true;
			}
			break;

		// remove send
		case -2:
			update = (m_srcId == -1 || _lp->gettoken_int(1) == m_srcId);
			// update => nothing! we do NOT re-copy this receive
			break;

		default:
			break;
	}
	return update; 
}

int SNM_SendPatcher::AddSend(
	MediaTrack* _srcTr, int _sendType, char* _vol, char* _pan) 
{
	m_srcId = _srcTr ? CSurf_TrackToID(_srcTr, false) : -1;
	m_sendType = _sendType;
	m_vol = _vol;
	m_pan = _pan;
	return ParsePatch(-1, 1, "TRACK", "MIDIOUT");
}

int SNM_SendPatcher::RemoveReceives() 
{
	m_srcId = -1;
	m_sendType = 2; // deprecated
	m_vol = NULL;
	m_pan = NULL;
	return ParsePatch(-2, 1, "TRACK", "AUXRECV");
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
// important: it assumes there're at least 2 takes (item should be removed otherwise)
///////////////////////////////////////////////////////////////////////////////

bool SNM_TakeParserPatcher::NotifyEndElement(int _mode, LineParser* _lp, WDL_String* _parsedLine,  
		WDL_PtrList<WDL_String>* _parsedParents, const char* _parsedParent, 
		WDL_String* _newChunk, int _updates)
{
	bool update = m_removing;
	if (m_removing && _mode == -1 && !strcmp(_parsedParent, "SOURCE"))
	{
		m_removing = false; // re-enable copy
	}
	else if (m_getting && _mode == -2 && !strcmp(_parsedParent, "SOURCE"))
	{
		m_subchunk.Append(">\n",2);
		m_getting = false; 
	}
	return update; 

	// note: "update" is true if m_removing was initially true, 
	//       i.e. do not recopy end of chunk ">"
}

bool SNM_TakeParserPatcher::NotifyChunkLine(int _mode, LineParser* _lp, WDL_String* _parsedLine,  
	int _parsedOccurence, WDL_PtrList<WDL_String>* _parsedParents, const char* _parsedParent, 
	WDL_String* _newChunk, int _updates)
{
	if (_mode == -1 && 		
		(!m_occurence && !strcmp(_lp->gettoken_str(0), "NAME")) ||
		(m_occurence && !strcmp(_lp->gettoken_str(0), "TAKE"))) //no "TAKE" in chunks for 1st take!
	{
		if (!m_removing && m_occurence == m_searchedTake) 
			m_removing = true;
		m_occurence++;
	}
	else if (_mode == -2 && 		
		(m_occurence && !strcmp(_lp->gettoken_str(0), "TAKE")) ||
		(!m_occurence && !strcmp(_lp->gettoken_str(0), "NAME"))) //no "TAKE" in chunks for 1st take!
	{
/*
		char dbg[128] = "";
		sprintf(dbg,"%s\n, m_occurence: %d, m_searchedTake: %d", _parsedLine->Get(), m_occurence, m_searchedTake);
		MessageBox(0,dbg,"",1);
*/
		if (!m_getting && m_occurence == m_searchedTake) 
			m_getting = true;
		m_occurence++;
	}

	if (m_getting)
	{
		m_subchunk.Append(_parsedLine->Get());
		m_subchunk.Append("\n");
	}

	return m_removing;
}

int SNM_TakeParserPatcher::RemoveTake(int _take)
{
	m_searchedTake = _take;
	m_removing = false;
	m_getting = false; 
	m_occurence = 0;
//	m_subchunk.Set("");

	RemoveIds();
	return ParsePatch(-1); 
}

int SNM_TakeParserPatcher::GetTakeChunk(int _take, WDL_String* _chunk)
{
	m_searchedTake = _take;
	m_removing = false;
	m_getting = false; 
	m_occurence = 0;
	m_subchunk.Set("");

	RemoveIds();
	if (_chunk && Parse(-2) >= 0)
	{
		_chunk->Set(m_subchunk.Get());
		return 1;
	}
	return 0;
}

