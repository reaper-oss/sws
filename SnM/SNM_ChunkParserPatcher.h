/******************************************************************************
** SNM_ChunkParserPatcher.h
** Copyright (C) 2009-2010, JF Bédague 
**
**    This software is provided 'as-is', without any express or implied
**    warranty.  In no event will the authors be held liable for any damages
**    arising from the use of this software.
**
**    Permission is granted to anyone to use this software for any purpose,
**    including commercial applications, and to alter it and redistribute it
**    freely, subject to the following restrictions:
**
**    1. The origin of this software must not be misrepresented; you must not
**       claim that you wrote the original software. If you use this software
**       in a product, an acknowledgment in the product documentation would be
**       appreciated but is not required.
**    2. Altered source versions must be plainly marked as such, and must not be
**       misrepresented as being the original software.
**    3. This notice may not be removed or altered from any source distribution.
**
******************************************************************************/

// Here are some tools to parse, patch, process.. all types of RPP chunks 
// and sub-chunks.
// A SNM_ChunkParserPatcher instance only gets and sets (if needed) 
// the chunk once, in between, the user works on a cache (itself updated), 
// thus preventing "long" processings.
// If any, the updates are automatically comitted when destroying a 
// SNM_ChunkParserPatcher instance (can be done manually, see m_autoCommit).
//
// This class can be used either as a SAX-ish parser (inheritance) or as 
// a direct getter or alering tool, see ParsePatch() and Parse().
//
// See use-cases here: 
// http://code.google.com/p/sws-extension/source/browse/trunk#trunk/SnM
//
// Important: 
// - MORE PERF. IMPROVMENT TO COME (replacing WDL_String)
// - the code assumes getted/setted RPP chunks are consistent
// - chunks may be HUGE!

#pragma once
#ifndef _SNM_CHUNKPARSERPATCHER_H_
#define _SNM_CHUNKPARSERPATCHER_H_


// The differents modes for ParsePatch() and Parse()
#define SNM_PARSE_AND_PATCH				0
#define SNM_PARSE_AND_PATCH_EXCEPT		1
#define SNM_PARSE						2
#define SNM_PARSE_EXCEPT				3
#define SNM_GET_CHUNK_CHAR				6
#define SNM_SET_CHUNK_CHAR				7
#define SNM_SETALL_CHUNK_CHAR_EXCEPT	8
#define SNM_GETALL_CHUNK_CHAR_EXCEPT	9
#define SNM_TOGGLE_CHUNK_INT			10
#define SNM_TOGGLE_CHUNK_INT_EXCEPT		11
#define SNM_REPLACE_SUBCHUNK			12
#define SNM_REPLACE_SUBCHUNK_EXCEPT		13
#define SNM_GET_SUBCHUNK				14
#define SNM_REPLACE_LINE				15


///////////////////////////////////////////////////////////////////////////////
// Static utility functions
///////////////////////////////////////////////////////////////////////////////

// Removes lines from a chunk in one-go and without chunk recopy
// Note: we parse rather than use a strchr() solution 'cause searched keywords
// may be present at "unexpected places" (e.g. set by the user). Will re-work that..
static int RemoveChunkLines(WDL_String* _chunk, WDL_PtrList<char>* _removedKeywords)
{
	int updates = 0;
	char* cData = _chunk ? _chunk->Get() : NULL;
	if (!cData)
		return -1;

	char* pEOL = cData-1;
	for(;;) {
		char* pLine = pEOL+1;
		pEOL = strchr(pLine, '\n');
		if (!pEOL)
			break;
		int curLineLength = (int)(pEOL-pLine);
		WDL_String curLine(pLine, curLineLength);
		LineParser lp(false);
		lp.parse(curLine.Get());
		if (lp.getnumtokens() > 0) 
		{
			char* keyword = lp.gettoken_str(0);
			bool match = false;
			for (int i=0; !match && i < _removedKeywords->GetSize(); i++)
				match = !strcmp(keyword, _removedKeywords->Get(i));
			if (match) 
			{
				updates++;
				_chunk->DeleteSub((int)(pLine-cData), curLineLength+1); //+1 for '\n'
				cData = _chunk->Get();
				pEOL = pLine-1;
			}
		}
	}
	return updates;
}

static void AddChunkIds(WDL_PtrList<char>* _removedKeywords)
{
	_removedKeywords->Add("IGUID");
	_removedKeywords->Add("GUID");
	_removedKeywords->Add("FXID");
	_removedKeywords->Add("TRACKID");
}


///////////////////////////////////////////////////////////////////////////////
// SNM_ChunkParserPatcher
///////////////////////////////////////////////////////////////////////////////

class SNM_ChunkParserPatcher
{
public:

SNM_ChunkParserPatcher(void* _object, bool _autoCommit = true)
{
	m_chunk.Set("");
	m_object = _object;
	m_updates = 0;
	m_autoCommit = _autoCommit;
}

virtual ~SNM_ChunkParserPatcher() 
{
	if (m_autoCommit)
		Commit(); // nop if chunk not updated
}

// See ParsePatchCore() comments
int ParsePatch(
	int _mode = SNM_PARSE_AND_PATCH, // can be <0 for custom modes (inheritance)
	int _depth = -1, // 1-based
	const char* _expectedParent = NULL, 
	const char* _keyWord = NULL, 
	int _numTokens = -1, // 0-based
	int _occurence = -1, // 0-based
	int _tokenPos = -1, // 0-based
	void* _value = NULL,
	void* _valueExcept = NULL)
{
	return ParsePatchCore(true, _mode, _depth, _expectedParent, 
		_keyWord, _numTokens, _occurence, 
		_tokenPos, _value,_valueExcept);
}

// See ParsePatchCore() comments
int Parse(
	int _mode = SNM_PARSE, // can be <0 for custom modes (inheritance)
	int _depth = -1, // 1-based
	const char* _expectedParent = NULL, 
	const char* _keyWord = NULL, 
	int _numTokens = -1, // 0-based
	int _occurence = -1, // 0-based
	int _tokenPos = -1, // 0-based
	void* _value = NULL,
	void* _valueExcept = NULL)
{
	return ParsePatchCore(false, _mode, _depth, _expectedParent, 
		_keyWord, _numTokens, _occurence, 
		_tokenPos, _value,_valueExcept);
}

// Get and cache the RPP chunk
WDL_String* GetChunk() 
{
	WDL_String* chunk = NULL;
	if (!m_chunk.GetLength())
	{
		char* cData = m_object ? GetSetObjectState(m_object, NULL) : NULL;
		if (cData)
		{
			m_chunk.Set(cData);
			FreeHeapPtr(cData);
			chunk = &m_chunk;
		}
	}
	else
		chunk = &m_chunk;
	return chunk;
}

void SetChunk(WDL_String* _newChunk, int _updates) {
	SetChunk(_newChunk ? _newChunk->Get() : NULL, _newChunk ? _updates : 0);
}

void UpdateChunk(WDL_String* _newChunk, int _updates) {
	if (_updates && _newChunk->GetLength())
		SetChunk(_newChunk ? _newChunk->Get() : NULL, _newChunk ? (m_updates + _updates) : 0);
}

// clearing the cache is allowed
void SetChunk(char* _newChunk, int _updates) {
	m_updates = _updates;
	m_chunk.Set(_newChunk ? _newChunk : "");
}

void SetUpdates(int _updates) {
	m_updates = _updates;
}

// Commit (if needed)
// note: in REAPER v3.35 it's safer *not* to patch while recording,
//       see http://forum.cockos.com/showthread.php?t=52002 so there's
//       a global protection here (temporary, I hope).
bool Commit()
{
	if (m_object && m_updates && 
		!(GetPlayState() & 4) && // prevent patches while recording
		m_chunk.GetLength() && 
		!GetSetObjectState(m_object, m_chunk.Get()))
	{
// Cool point for debug!
//		ShowConsoleMsg(m_chunk.Get());
//		MessageBox(0,"Chunk commit","dbg",1);
		SetChunk("", 0);
		return true;
	}
	return false;
}


//**********************
// Facility methods
//**********************

bool GetSubChunk(const char* _keyword, int _depth, int _occurence, WDL_String* _chunk) 
{
	if (_chunk && _keyword && _depth > 0)
	{
		_chunk->Set("");
		WDL_String startToken;
		startToken.SetFormatted((int)strlen(_keyword)+1, "<%s", _keyword);
		if (Parse(SNM_GET_SUBCHUNK, _depth, _keyword, startToken.Get(), 
			-1, _occurence, -1, (void*)_chunk) <= 0)
		{
			_chunk->Set("");
		}
		return (_chunk->GetLength() > 0);
	}
	return false;
}

bool ReplaceSubChunk(const char* _keyword, int _depth, int _occurence, 
	const char* _newSubChunk = "")
{
	if (_keyword && _depth > 0)
	{
		WDL_String startToken;
		startToken.SetFormatted((int)strlen(_keyword)+1, "<%s", _keyword);
		return (ParsePatch(SNM_REPLACE_SUBCHUNK, _depth, _keyword, startToken.Get(), -1, 
			_occurence, -1, (void*)_newSubChunk) > 0);
	}
	return false;
}

int RemoveLines(WDL_PtrList<char>* _removedKeywords) 
{
	WDL_String newChunk;
	int updates = RemoveChunkLines(GetChunk(),_removedKeywords);
	UpdateChunk(&newChunk, updates); //nop if no updates
	return updates;
}

int RemoveIds() 
{
	WDL_PtrList<char> removedKeywords;
	AddChunkIds(&removedKeywords);

	WDL_String newChunk;
	int updates = RemoveChunkLines(GetChunk(), &removedKeywords);
	UpdateChunk(&newChunk, updates); //nop if no updates
	return updates;
}


///////////////////////////////////////////////////////////////////////////////
protected:
	void* m_object;

	// Parsing callbacks (to be implemented for SAX-ish parsing style)
	// ------------------------------------------------------------------------
	// Parameters:
	// _mode: parsing mode, see ParsePatchCore(), <0 for custom parsing modes
	// _lp: the line beeing parsed as a LineParser
	// _parsedLine: : the line beeing parsed as a SNM_String
	// _parsedParents: the parsed line's parent (last item) and its the grand-parent 
	//                 and so one to the root. The number of items is also the parsed 
	//                 depth (so it's 1-based)
	// _parsedParent: the parsed line's parent (for facility)
	// _newChunk: the chunk beeing built (while parsing)
	// _updates: number of updates in comparison with the original chunk
	// Note all available params are provided for facility but:
	// _parsedLine: can be built from _lp
	// _parsedParent: can be retrieved from _parsedParents
	//
	// Return values: 
	// true if chunk has been altered
	//
	virtual void NotifyStartChunk(int _mode) {}
	virtual void NotifyEndChunk(int _mode) {}
	virtual bool NotifyStartElement(int _mode, LineParser* _lp, WDL_String* _parsedLine,  
		WDL_PtrList<WDL_String>* _parsedParents, const char* _parsedParent, 
		WDL_String* _newChunk, int _updates) {return false;} // no update
	virtual bool NotifyEndElement(int _mode, LineParser* _lp, WDL_String* _parsedLine,  
		WDL_PtrList<WDL_String>* _parsedParents, const char* _parsedParent, 
		WDL_String* _newChunk, int _updates) {return false;} // no update
	virtual bool NotifyChunkLine(int _mode, LineParser* _lp, WDL_String* _parsedLine,  
		int _parsedOccurence, WDL_PtrList<WDL_String>* _parsedParents, const char* _parsedParent, 
		WDL_String* _newChunk, int _updates) {return false;} // no update


///////////////////////////////////////////////////////////////////////////////
private:
	WDL_String m_chunk;
	int m_updates;
	bool m_autoCommit;

bool WriteChunkLine(WDL_String* _chunkLine, const char* _value, int _tokenPos, LineParser* _lp)
{
	bool updated = false;
	if (strcmp(_lp->gettoken_str(_tokenPos), _value) != 0) //update *IF* needed
	{
		int numtokens = _lp->getnumtokens();
		for (int i=0; i < numtokens; i++) 
		{
			if (i == _tokenPos) {
				_chunkLine->Append(_value);
				updated = true;
			}
			else _chunkLine->Append(_lp->gettoken_str(i));
			_chunkLine->Append(i == (numtokens-1) ? "\n" : " ", 1);
		}
	}
	return updated;
}

// just to avoid many duplicate strcmp() calls in ParsePatchCore()
void IsMatchingParsedLine(bool* _tolerantMatch, bool* _strictMatch, 
		int _expectedDepth, int _parsedDepth,
		const char* _expectedParent, const char* _parsedParent,
		const char* _expectedKeyword, const char* _parsedKeyword,
		int _expectedNumTokens, int _parsedNumTokens)
{
	*_tolerantMatch = false;
	*_strictMatch = false;

	if (_expectedDepth == -1) {
		*_tolerantMatch = true;
	}
	else if (_expectedDepth == _parsedDepth) {
		if (!_expectedParent) {
			*_tolerantMatch = true;
		}
		else if (!strcmp(_parsedParent, _expectedParent)) {
			if (!_expectedKeyword) {
				*_tolerantMatch = true;
			}
			else if (!strcmp(_parsedKeyword, _expectedKeyword)) {
				*_strictMatch = *_tolerantMatch = 
					(_expectedNumTokens == -1 || _expectedNumTokens == _parsedNumTokens);
			}
		}
	}
}

// *** ParsePatchCore() ***
//
// Parameters: 
// See bellow. Globaly, the method is tolerant; the less parameters 
// provided (i.e. different from their default values), the more parsed 
// lines will be notified to inherited instances (through Notifyxxx()), 
// or, when it's used direcly, the more parsed lines will be read/altered.
// Examples: parse all lines, parse only lines of 2nd depth, under 
// parent "FXCHAIN", do all the lines begining with "<SOURCE" have 
// "MIDI" as 2nd token ? Etc..
// Note: sometimes there's a dependency between the params to be provided
// (most of the time with _mode). Should returns -1 if it's not respected.
//
// Return values:
// Always return -1 on error/bad usage, or
// returns the number of updates done when altering (i.e. 0 nothing done), or
// returns 1 (found) or 0 (not found) when getting
//
int ParsePatchCore(
	bool _write, // optimization flag (if false: no re-copy)
	int _mode, // can be <0 for custom modes (usefull for inheritance)
	int _depth, // 1-based
	const char* _expectedParent, 
	const char* _keyWord, 
	int _numTokens, // 0-based
	int _occurence, // 0-based
	int _tokenPos, // 0-based
	void* _value, // value to get/set
	void* _valueExcept)
{
	// check params (not to do it in the parsing loop/switch-cases)
	if ((!_value || _tokenPos < 0) && 
			(_mode == SNM_GET_CHUNK_CHAR || _mode == SNM_SET_CHUNK_CHAR || 
			_mode == SNM_SETALL_CHUNK_CHAR_EXCEPT || _mode == SNM_GETALL_CHUNK_CHAR_EXCEPT))
		return -1;

	// sub-cunk processing: depth must be provided 
	if ((!_value || _depth <= 0) &&
		(_mode == SNM_GET_SUBCHUNK || _mode == SNM_REPLACE_LINE ||
		 _mode == SNM_REPLACE_SUBCHUNK || _mode == SNM_REPLACE_SUBCHUNK_EXCEPT))
		return -1;

	// get/cache the chunk
	char* cData = GetChunk() ? GetChunk()->Get() : NULL;
	if (!cData)
		return -1;

	// Start of chunk parsing
	NotifyStartChunk(_mode);

	int updates = 0, occurence = 0;
	WDL_String newChunk("");
	WDL_String* subChunkKeyword = NULL;
	WDL_PtrList<WDL_String> parents;
	char* pEOL = cData-1;

	// OK, big stuff begins
	for(;;)
	{
		char* pLine = pEOL+1;
		pEOL = strchr(pLine, '\n');
		if (!pEOL)
			break;

		bool alter = false;
		char* keyword = NULL;
		int curLineLength = (int)(pEOL-pLine); // will avoid many strlen() calls..
		WDL_String curLine(pLine, curLineLength);
		LineParser lp(false);
		lp.parse(curLine.Get());
		int lpNumTokens=lp.getnumtokens();
		if (lpNumTokens > 0)
		{
			keyword = lp.gettoken_str(0);
			if (*keyword == '<')
			{
				// notify & update parent list
				parents.Add(new WDL_String(keyword+1)); // +1 to zap '<'
				alter = NotifyStartElement(_mode, &lp, &curLine, &parents, keyword+1, &newChunk, updates);
			}
			else if (*keyword == '>')
			{
				// end of processed sub-chunk ?
				if (subChunkKeyword && _depth == parents.GetSize() && 
						subChunkKeyword == parents.Get(_depth-1)) // => valid _depth must be provided
				{
					subChunkKeyword = NULL;

					// don't recopy '>' depending on the modes..
					alter = (_mode == SNM_REPLACE_SUBCHUNK || SNM_REPLACE_SUBCHUNK_EXCEPT);
					// .. but recopy it for some
					if (_mode == SNM_GET_SUBCHUNK)
					{
						((WDL_String*)_value)->Append(">\n",2);
						return 1; // TODO: return position instead
					}
				}

				// notify & update parent list
				alter |= NotifyEndElement(_mode, &lp, &curLine, &parents, 
					parents.Get(parents.GetSize()-1)->Get(), &newChunk, updates);
				parents.Delete(parents.GetSize()-1, true);
			}
		}

		// end of chunk lines (">") are not processed/notified (but copied if needed)
		if (parents.GetSize())
		{
			WDL_String* currentParent = parents.Get(parents.GetSize()-1);

			// just to avoid many duplicate strcmp() calls..
			bool tolerantMatch, strictMatch;
			IsMatchingParsedLine(&tolerantMatch, &strictMatch,
				_depth, parents.GetSize(), _expectedParent, currentParent->Get(), 
				_keyWord, keyword, _numTokens, lpNumTokens);
			if (tolerantMatch && _mode < 0)
			{
				if (_occurence == occurence || _occurence == -1)
					alter |= NotifyChunkLine(_mode, &lp, &curLine, occurence, 
						&parents, currentParent->Get(), &newChunk, updates); 
				occurence++;
			}
			else if (strictMatch && _mode >= 0)
			{
				char bufConversion[16] = ""; // Can't put it in the switch! 

				// This occurence match
				// note: _occurence == -1 is a valid parameter, processed as "except"
				if (_occurence == occurence)
				{
					switch (_mode)
					{
						case SNM_GET_CHUNK_CHAR:
							strcpy((char*)_value, lp.gettoken_str(_tokenPos));
							return 1; // TODO: return position instead
						case SNM_SET_CHUNK_CHAR:
							alter |= WriteChunkLine(&newChunk, (char*)_value, _tokenPos, &lp); 
							break;
						case SNM_SETALL_CHUNK_CHAR_EXCEPT:
						case SNM_TOGGLE_CHUNK_INT_EXCEPT:
							if (_valueExcept){
								if (strcmp((char*)_valueExcept, lp.gettoken_str(_tokenPos)) != 0)
									alter |= WriteChunkLine(&newChunk, (char*)_valueExcept, _tokenPos, &lp); 
							}
							break;
						case SNM_PARSE_AND_PATCH:
						case SNM_PARSE:
							alter |= NotifyChunkLine(_mode, &lp, &curLine, occurence, 
								&parents, currentParent->Get(), &newChunk, updates);
							break;
						case SNM_TOGGLE_CHUNK_INT:
							sprintf(bufConversion, "%d", !lp.gettoken_int(_tokenPos));
							alter |= WriteChunkLine(&newChunk, bufConversion, _tokenPos, &lp); 
                            break; 
						case SNM_REPLACE_LINE:
							newChunk.Append((char*)_value);
							alter=true;
							break;
						case SNM_REPLACE_SUBCHUNK:
							newChunk.Append((char*)_value);
							if (*_keyWord == '<') subChunkKeyword = currentParent;
							alter=true;
							break;
						case SNM_GET_SUBCHUNK:
							((WDL_String*)_value)->AppendFormatted(curLineLength+1, "%s\n", curLine.Get());
							if (*_keyWord == '<') subChunkKeyword = currentParent;
							break;
						case SNM_REPLACE_SUBCHUNK_EXCEPT:
							if (_valueExcept) {
								newChunk.Append((char*)_valueExcept);
								if (*_keyWord == '<') subChunkKeyword = currentParent;
								alter=true;
							}
							break;
 						default:
							break;
					}
				}
				// this occurence doesn't match
				else 
				{
					switch (_mode)
					{
						case SNM_SETALL_CHUNK_CHAR_EXCEPT:
							if (strcmp((char*)_value, lp.gettoken_str(_tokenPos)) != 0)
								alter |= WriteChunkLine(&newChunk, (char*)_value, _tokenPos, &lp); 
							break;
						case SNM_GETALL_CHUNK_CHAR_EXCEPT:
							if (strcmp((char*)_value, lp.gettoken_str(_tokenPos)) != 0)
								return 0;
							break;
						case SNM_PARSE_AND_PATCH_EXCEPT:
						case SNM_PARSE_EXCEPT:
							alter |= NotifyChunkLine(_mode, &lp, &curLine, occurence, 
								&parents, currentParent->Get(), &newChunk, updates); 
							break;
						case SNM_TOGGLE_CHUNK_INT_EXCEPT:
							sprintf(bufConversion, "%d", !lp.gettoken_int(_tokenPos));
							alter |= WriteChunkLine(&newChunk, bufConversion, _tokenPos, &lp); 
                            break; 
						case SNM_REPLACE_SUBCHUNK_EXCEPT:
							newChunk.Append((char*)_value);
							if (*_keyWord == '<') subChunkKeyword = currentParent;
							alter=true;
							break;
						default:
							break;
					}
				}
				occurence++;
			}
			// are we in a processed sub-chunk?
			else if (subChunkKeyword) 
			{
				alter = (_mode == SNM_REPLACE_SUBCHUNK || SNM_REPLACE_SUBCHUNK_EXCEPT);
				if (_mode == SNM_GET_SUBCHUNK)
					((WDL_String*)_value)->AppendFormatted(curLineLength+1, "%s\n", curLine.Get());
			}
		}
		updates += (_write && alter);

		// copy current line if it wasn't altered
		if (_write && !alter && lpNumTokens)
#ifdef _WIN32
			newChunk.AppendFormatted(curLineLength+1, "%s\n", curLine.Get());
#else
			newChunk.AppendFormatted(curLineLength+2, "%s\n", curLine.Get());
#endif

	}

	// Update chunk cache (nop if empty chunk or no updates)
	UpdateChunk(&newChunk, updates);

	// End of chunk parsing
	NotifyEndChunk(_mode);

	// return values
	switch (_mode)
	{
		// *** READ ONLY ***
		case SNM_GET_CHUNK_CHAR:
		case SNM_GET_SUBCHUNK:
			return 0; // if we're here: not found
		case SNM_GETALL_CHUNK_CHAR_EXCEPT:
			return 1; // if we're here: found (returns 0 on 1st unmatching)
		case SNM_PARSE:
		case SNM_PARSE_EXCEPT:
			return 1; // read ok..

		// *** R/W ***
		case SNM_PARSE_AND_PATCH:
		case SNM_PARSE_AND_PATCH_EXCEPT:
		case SNM_SET_CHUNK_CHAR:
		case SNM_SETALL_CHUNK_CHAR_EXCEPT:
		case SNM_TOGGLE_CHUNK_INT:
		case SNM_TOGGLE_CHUNK_INT_EXCEPT:
		case SNM_REPLACE_SUBCHUNK:
		case SNM_REPLACE_SUBCHUNK_EXCEPT:
		case SNM_REPLACE_LINE:
		default: // for custom _mode (e.g. <0)
			return updates;
	}
	return -1; //in case _mode comes from mars
}
};

#endif
