/******************************************************************************
** SNM_ChunkParserPatcher.h
**
** Copyright (C) 2010, JF Bédague 
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
*/

#pragma once

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
#define SNM_REPLACE_SUB_CHUNK			12
#define SNM_REPLACE_SUB_CHUNK_EXCEPT	13


// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Please contact me for any mod in this flie.
// Thanks, Jeff.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// SNM_ChunkParserPatcher is a class that offer some tools to parse, 
// patch, process.. all types of RPP chunks and sub-chunks. 
// It only gets and sets the chunk once, in beween the user works on a 
// cache (itself updated), thus preventing "long" processings.
// If any, the updates are automatically comitted when destroying a 
// SNM_ChunkParserPatcher instance (can be done manually, see m_autoCommit).
//
// This class can be used either as a SAX-ish parser (inheritance) or as 
// a direct getter or alering tool, see ParsePatch() and Parse().
//
// See use-cases here: 
// http://code.google.com/p/sws-extension/source/browse/trunk#trunk/SnM
//
// Important: the code assumes RPP chunks are consistent.
//
//

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
	return ParsePatch(true, _mode, _depth, _expectedParent, 
		_keyWord, _numTokens, _occurence, 
		_tokenPos, _value,_valueExcept);
}

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
	return ParsePatch(false, _mode, _depth, _expectedParent, 
		_keyWord, _numTokens, _occurence, 
		_tokenPos, _value,_valueExcept);
}

int ReplaceSubChunk(const char* _keyword, int _depth, int _occurence, 
	const char* _newSubChunk = "")
{
	if (_keyword && _depth > 0)
	{
		WDL_String startToken;
		startToken.SetFormatted((int)strlen(_keyword)+1, "<%s", _keyword);
		return ParsePatch(true, SNM_REPLACE_SUB_CHUNK, _depth, _keyword, startToken.Get(), -1, _occurence, -1, (void*)_newSubChunk);
	}
	return -1;
}

int RemoveIds()
{
	int updates = 0;

	// get/cache the chunk
	char* cData = GetChunk() ? GetChunk()->Get() : NULL;
	if (!cData)
		return -1;

	WDL_String curLine, newChunk("");
	char* pEOL = cData-1;
	for(;;)
	{
		char* pLine = pEOL+1;
		pEOL = strchr(pLine, '\n');
		if (!pEOL)
			break;
		curLine.Set(pLine, (int)(pEOL-pLine));
		LineParser lp(false);
		lp.parse(curLine.Get());
		if (lp.getnumtokens() > 0) {
			char* keyword = lp.gettoken_str(0);
			if (!strcmp(keyword, "FXID") || 
				!strcmp(keyword, "IGUID") || 
				!strcmp(keyword, "GUID") || 
				!strcmp(keyword, "TRACKID")) {
				updates++;
			}
			else {
				newChunk.Append(curLine.Get());
				newChunk.Append("\n");
			}
		}
	}

	// Update chunk cache (nop if empty chunk or no updates)
	UpdateChunk(&newChunk, updates);
	return updates;
}

// Get/cache the RPP chunk
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

void SetChunk(WDL_String* _newChunk, int _updates)
{
	SetChunk(_newChunk ? _newChunk->Get() : NULL, 
		_newChunk ? _updates : 0);
}

void UpdateChunk(WDL_String* _newChunk, int _updates)
{
	if (_updates && _newChunk->GetLength())
	{
		SetChunk(_newChunk ? _newChunk->Get() : NULL, 
			_newChunk ? (m_updates + _updates) : 0);
	}
}

void SetChunk(char* _newChunk, int _updates)
{
	m_chunk.Set("");
	m_updates = _updates;
	if (_newChunk)
		m_chunk.Set(_newChunk);
}

bool Commit()
{
	if (m_object && m_updates && m_chunk.GetLength() && 
		!GetSetObjectState(m_object, m_chunk.Get()))
	{
//dbg		ShowConsoleMsg(m_chunk.Get());
		SetChunk("", 0);
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
protected:
	void* m_object;

	// Parsing callbacks (to be inherited for SAX-ish parsing style)
	// All available params are provided for facility
	// Notes:
	// _parsedLine (can be built from _lp)
	// _parsedParent (can be retrieved from _parsedParents)
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

bool WriteChunkLine(
	WDL_String* _chunkLine, const char* _value, int _tokenPos, LineParser* _lp)
{
	int numtokens = (_lp ? _lp->getnumtokens() : 0);
	for (int i=0; i < numtokens; i++)
	{
		if (i == _tokenPos)
			_chunkLine->Append(_value);
		else
			_chunkLine->Append(_lp->gettoken_str(i));
		_chunkLine->Append(i == (numtokens-1) ? "\n" : " ");
	}
	return true;
}

void IsMatchingParsedLine(bool* _tolerantMatch, bool* _strictMatch, 
		int _expectedDepth, int _parsedDepth,
		const char* _expectedParent, const char* _parsedParent,
		const char* _expectedKeyword, const char* _parsedKeyword,
		int _expectedNumTokens, int _parsedNumTokens)
{
	*_tolerantMatch = false;
	*_strictMatch = false;

	if (_expectedDepth == -1)
	{
		*_tolerantMatch = true;
	}
	else if (_expectedDepth == _parsedDepth) 
	{
		if (!_expectedParent)
		{
			*_tolerantMatch = true;
		}
		else if (!strcmp(_parsedParent, _expectedParent)) 
		{
			if (!_expectedKeyword)
			{
				*_tolerantMatch = true;
			}
			else if (!strcmp(_parsedKeyword, _expectedKeyword)) 
			{
				*_strictMatch = *_tolerantMatch = 
					(_expectedNumTokens == -1 || _expectedNumTokens == _parsedNumTokens);
			}
		}
	}
}

int ParsePatch(bool _write, int _mode = SNM_PARSE_AND_PATCH, 
	int _depth = -1, const char* _expectedParent = NULL, 
	const char* _keyWord = NULL, int _numTokens = -1, 
	int _occurence = -1, 
	int _tokenPos = -1, void* _value = NULL,
	void* _valueExcept = NULL)
{
	// check params (not to do it in the parsing loop/switch-case)
	if ((!_value || _tokenPos < 0) && 
			(_mode == SNM_GET_CHUNK_CHAR || _mode == SNM_SET_CHUNK_CHAR || 
			_mode == SNM_SETALL_CHUNK_CHAR_EXCEPT || _mode == SNM_GETALL_CHUNK_CHAR_EXCEPT))
		return -1;

	// sub-cunk processing: depth must be provided 
	if ((_mode == SNM_REPLACE_SUB_CHUNK || _mode == SNM_REPLACE_SUB_CHUNK_EXCEPT) && 
			(!_value || _depth <=0))
		return -1;

	// get/cache the chunk
	char* cData = GetChunk() ? GetChunk()->Get() : NULL;
	if (!cData)
		return -1;

	// Start of chunk parsing
	NotifyStartChunk(_mode);

	int updates = 0, occurence = 0;
	WDL_String curLine, newChunk("");
	WDL_String* subChunkKeyword = NULL;
	WDL_PtrList<WDL_String> parents;
	char* pEOL = cData-1;
	for(;;)
	{
		char* pLine = pEOL+1;
		pEOL = strchr(pLine, '\n');
		if (!pEOL)
			break;

		bool alter = false;
		char* keyword = NULL;

		curLine.Set(pLine, (int)(pEOL-pLine));
		LineParser lp(false);
		lp.parse(curLine.Get());
		int lpNumTokens=lp.getnumtokens();
		if (lpNumTokens > 0)
		{
			keyword = lp.gettoken_str(0);

			// Get the depth
			if (*keyword == '<')
			{
				parents.Add(new WDL_String(keyword+1)); // +1 to zap '<'
				alter = NotifyStartElement(_mode, &lp, &curLine, &parents, keyword+1, &newChunk, updates);
			}
			else if (*keyword == '>')
			{
				// end of processed sub-chunk (valid _depth must be provided) 
				if (subChunkKeyword && _depth == parents.GetSize() && 
						subChunkKeyword == parents.Get(_depth-1))
				{
					subChunkKeyword = NULL;
					// don't recopy '>' depending on the modes
					alter = (_mode == SNM_REPLACE_SUB_CHUNK);
				}

				alter |= NotifyEndElement(_mode, &lp, &curLine, &parents, 
					parents.Get(parents.GetSize()-1)->Get(), &newChunk, updates);
				parents.Delete(parents.GetSize()-1, true);
			}
		}

		// end of chunk lines (">") are not processed/notified
		// (but copied if needed)
		if (parents.GetSize())
		{
			WDL_String* currentParent = parents.Get(parents.GetSize()-1);

			// Does the line match ?
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

				// _occurence == -1 is a valid parameter, processed as "except"
				if (_occurence == occurence)
				{
					switch (_mode)
					{
						case SNM_GET_CHUNK_CHAR:
							strcpy((char*)_value, lp.gettoken_str(_tokenPos));
							return 1;
						case SNM_SET_CHUNK_CHAR:
							alter |= WriteChunkLine(
								&newChunk, (char*)_value, _tokenPos, &lp); 
							break;
						case SNM_SETALL_CHUNK_CHAR_EXCEPT:
						case SNM_TOGGLE_CHUNK_INT_EXCEPT:
							if (_valueExcept){
								if (strcmp((char*)_valueExcept, lp.gettoken_str(_tokenPos)) != 0)
									alter |= WriteChunkLine(
										&newChunk, (char*)_valueExcept, _tokenPos, &lp); 
							}
							break;
						case SNM_PARSE_AND_PATCH:
						case SNM_PARSE:
							alter |= NotifyChunkLine(_mode, &lp, &curLine, occurence, 
								&parents, currentParent->Get(), &newChunk, updates);
							break;
						case SNM_TOGGLE_CHUNK_INT:
							sprintf(bufConversion, "%d", !lp.gettoken_int(_tokenPos));
							alter |= WriteChunkLine(
								&newChunk, bufConversion, _tokenPos, &lp); 
                             break; 
						case SNM_REPLACE_SUB_CHUNK:
							newChunk.Append((char*)_value);
							subChunkKeyword = currentParent;
							alter=true;
						break;
						case SNM_REPLACE_SUB_CHUNK_EXCEPT:
							if (_valueExcept) {
								newChunk.Append((char*)_valueExcept);
								subChunkKeyword = currentParent;
								alter=true;
							}
						break;
 						default:
							break;
					}
				}
				else 
				{
					switch (_mode)
					{
						case SNM_SETALL_CHUNK_CHAR_EXCEPT:
							if (strcmp((char*)_value, lp.gettoken_str(_tokenPos)) != 0)
								alter |= WriteChunkLine( 
									&newChunk, (char*)_value, _tokenPos, &lp); 
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
							alter |= WriteChunkLine(
								&newChunk, bufConversion, _tokenPos, &lp); 
                            break; 
						case SNM_REPLACE_SUB_CHUNK_EXCEPT:
							newChunk.Append((char*)_value);
							subChunkKeyword = currentParent;
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
				alter = (_mode == SNM_REPLACE_SUB_CHUNK);
		}
		updates += (_write && alter);

		// copy current line if it wasn't altered
		if (_write && !alter && lpNumTokens)
		{
			newChunk.Append(curLine.Get());
			newChunk.Append("\n");
		}
	}

	// Update chunk cache (nop if empty chunk or no updates)
	UpdateChunk(&newChunk, updates);

	// End of chunk parsing
	NotifyEndChunk(_mode);

	// return values
	switch (_mode)
	{
		case SNM_GET_CHUNK_CHAR:
			return 0;
		case SNM_GETALL_CHUNK_CHAR_EXCEPT:
			return 1;
		case SNM_PARSE:
		case SNM_PARSE_EXCEPT:
			return 1;
		case SNM_PARSE_AND_PATCH:
		case SNM_PARSE_AND_PATCH_EXCEPT:
		case SNM_SET_CHUNK_CHAR:
		case SNM_SETALL_CHUNK_CHAR_EXCEPT:
		case SNM_TOGGLE_CHUNK_INT:
		case SNM_TOGGLE_CHUNK_INT_EXCEPT:
		case SNM_REPLACE_SUB_CHUNK:
		case SNM_REPLACE_SUB_CHUNK_EXCEPT:
		default: // for custom _mode (e.g. <0)
			return updates;
	}
	return -1;
}
};

