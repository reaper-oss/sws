/******************************************************************************
** SNM_ChunkParserPatcher.h - v1.21
** Copyright (C) 2008-2011, Jeffos
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
**       in a product, an acknowledgment in the product and its documentation 
**       is required.
**    2. Altered source versions must be plainly marked as such, and must not be
**       misrepresented as being the original software.
**    3. This notice may not be removed or altered from any source distribution.
**
******************************************************************************/

// Here are some tools to parse, patch, process.. all RPP chunks and sub-chunks. 
// SNM_ChunkParserPatcher is a class that can be used either as a SAX-ish 
// parser (inheritance) or as a direct getter or alering tool, see ParsePatch() 
// and Parse().
//
// In both cases, it's also either attached to a WDL_String* (simple text 
// chunk parser/patcher) -OR- to a reaThing* (MediaTrack*, MediaItem*, ..). 
// In the latter case, a SNM_ChunkParserPatcher instance only gets (and sets, 
// if needed) the chunk once, in between, the user works on a cache. *If any* 
// the updates are automatically comitted when destroying a SNM_ChunkParserPatcher 
// instance (can also be forced/done manually, see m_autoCommit & Commit()).
//
// See many use-cases here: 
// http://code.google.com/p/sws-extension/source/browse/trunk#trunk/SnM
//
// This code uses/is tied to Cockos' WDL library: http://cockos.com/wdl
// Thank you Cockos!
// 
// Important: 
// - Chunks may be HUGE!
// - The code assumes getted/setted RPP chunks are consistent
// - For additional performance improvments, this WDL_String mod can be used:
//   http://code.google.com/p/sws-extension/source/browse/trunk/SnM/wdlstring.h 
//   (see details there, mods are plainly marked as required by the licensing)
//
// Changelog:
// v1.21
// - New helpers, new SNM_COUNT_KEYWORD parsing mode
// - Inheritance: Commit() & GetChunk() can be overrided
// - GetSubChunk() now returns the start position of the sub-chunk (or -1 if not found)
// v1.1
// - Fixes
// - Use SWS_GetSetObjectState if _SWS_EXTENSION is defined. This offers an additional cache system.
//   See http://code.google.com/p/sws-extension/source/browse/trunk/ObjectState/ObjectState.cpp
//   Note: SWS_GetSetObjectState == native GetSetObjectState if it is not surrounded with 
//   SWS_CacheObjectState(true)/SWS_CacheObjectState(false).
// v1.0 
// - Licensing update, see header
// - Performance improvments, more to come..
// - Safer commit of chunk updates (auto ids removal)

#pragma once

#ifndef _SNM_CHUNKPARSERPATCHER_H_
#define _SNM_CHUNKPARSERPATCHER_H_

#pragma warning(disable : 4267) // size_t to int warnings in x64

//#define _SNM_DEBUG
#define _SWS_EXTENSION

#ifdef _SWS_EXTENSION
#define SNM_FreeHeapPtr			SWS_FreeHeapPtr
#else
#define SNM_FreeHeapPtr			FreeHeapPtr
#endif


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
#define SNM_REPLACE_SUBCHUNK_OR_LINE	12
#define SNM_GET_SUBCHUNK_OR_LINE		13
#define SNM_GET_SUBCHUNK_OR_LINE_EOL	14
#define SNM_COUNT_KEYWORD				15

// Misc
#define SNM_MAX_CHUNK_LINE_LENGTH		2048 // some file paths may be long
#define SNM_MAX_CHUNK_KEYWORD_LENGTH	64
#define SNM_HEAPBUF_GRANUL				4096


///////////////////////////////////////////////////////////////////////////////
// Fast chunk processing helpers (some are defined at eof)
///////////////////////////////////////////////////////////////////////////////

static int RemoveChunkLines(char* _chunk, const char* _searchStr, 
							bool _checkBOL = false, int _checkEOLChar = 0, int* _len = NULL);
static int RemoveChunkLines(char* _chunk, WDL_PtrList<const char>* _searchStrs, 
							bool _checkBOL = false, int _checkEOLChar = 0, int* _len = NULL);
static int RemoveChunkLines(WDL_String* _chunk, const char* _searchStr,
							bool _checkBOL = false, int _checkEOLChar = 0, int* _len = NULL);
static int RemoveChunkLines(WDL_String* _chunk, WDL_PtrList<const char>* _searchStrs,
							bool _checkBOL = false, int _checkEOLChar = 0, int* _len = NULL);

static int RemoveAllIds(char* _chunk, int* _len = NULL){
	return RemoveChunkLines(_chunk, "ID {", false, '}', _len);
}

// Rmk: preserves POOLEDEVTS ids
static int RemoveAllIds(WDL_String* _chunk, int* _len = NULL){
	return RemoveChunkLines(_chunk, "ID {", false, '}', _len);
}


///////////////////////////////////////////////////////////////////////////////
// SNM_ChunkParserPatcher
///////////////////////////////////////////////////////////////////////////////

class SNM_ChunkParserPatcher
{
public:

// Attached to a reaThing* (MediaTrack*, MediaItem*, ..)
SNM_ChunkParserPatcher(void* _object, bool _autoCommit=true, bool _processBase64=false, bool _processInProjectMIDI=false)
{
	m_chunk = new WDL_String(SNM_HEAPBUF_GRANUL);
	m_object = _object;
	m_updates = 0;
	m_autoCommit = _autoCommit;
	m_breakParsePatch = false;
	m_processBase64 = _processBase64;
	m_processInProjectMIDI = _processInProjectMIDI;
}

// Simple text chunk parser/patcher
SNM_ChunkParserPatcher(WDL_String* _chunk, bool _processBase64=false, bool _processInProjectMIDI=false)
{
	m_chunk = new WDL_String(SNM_HEAPBUF_GRANUL);
	if (m_chunk && _chunk)
		m_chunk->Set(_chunk->Get());
	m_object = NULL;
	m_updates = 0;
	m_autoCommit = false;
	m_breakParsePatch = false;
	m_processBase64 = _processBase64;
	m_processInProjectMIDI = _processInProjectMIDI;
}

virtual ~SNM_ChunkParserPatcher() 
{
	if (m_autoCommit)
		Commit(); // nop if chunk not updated (or no valid m_object)

	if (m_chunk)
	{
		delete m_chunk;
		m_chunk = NULL;
	}
}

// See ParsePatchCore() comments
int ParsePatch(
	int _mode = SNM_PARSE_AND_PATCH, 
	int _depth = -1,
	const char* _expectedParent = NULL, 
	const char* _keyWord = NULL, 
	int _numTokens = -1, 
	int _occurence = -1, 
	int _tokenPos = -1, 
	void* _value = NULL,
	void* _valueExcept = NULL,
	const char* _breakKeyword = NULL)
{
	return ParsePatchCore(true, _mode, _depth, _expectedParent, _keyWord, _numTokens, _occurence, _tokenPos, _value,_valueExcept, _breakKeyword);
}

// See ParsePatchCore() comments
int Parse(
	int _mode = SNM_PARSE, 
	int _depth = -1, 
	const char* _expectedParent = NULL, 
	const char* _keyWord = NULL, 
	int _numTokens = -1, 
	int _occurence = -1, 
	int _tokenPos = -1, 
	void* _value = NULL,
	void* _valueExcept = NULL,
	const char* _breakKeyword = NULL)
{
	return ParsePatchCore(false, _mode, _depth, _expectedParent, _keyWord, _numTokens, _occurence, _tokenPos, _value,_valueExcept, _breakKeyword);
}

void* GetObject() {
	return m_object;
}

// Get and cache the RPP chunk
virtual WDL_String* GetChunk() 
{
	WDL_String* chunk = NULL;
	if (!m_chunk->GetLength())
	{
		char* cData = m_object ? SNM_GetSetObjectState(m_object, NULL) : NULL;
		if (cData)
		{
			m_chunk->Set(cData);
			SNM_FreeHeapPtr(cData);
			chunk = m_chunk;
		}
	}
	else
		chunk = m_chunk;
	return chunk;
}


// IMPORTANT: in order to keep 'm_updates' up-to-date, you *MUST* use 
// UpdateChunk() & SetChunk() methods when altering the cached chunk,
// nothing will be comitted otherwise 
// -OR- you *MUST* also manually alter 'm_updates'.

// clearing the cache is allowed
void SetChunk(const char* _newChunk, int _updates) {
	m_updates = _updates;
	m_chunk->Set(_newChunk ? _newChunk : "");
}

// for facility (hooked to above)
void SetChunk(WDL_String* _newChunk, int _updates) {
	SetChunk(_newChunk ? _newChunk->Get() : NULL, _newChunk && _newChunk->GetLength() ? _updates : 0);
}

void UpdateChunk(WDL_String* _newChunk, int _updates) {
	if (_updates && _newChunk && _newChunk->GetLength())
		SetChunk(_newChunk->Get(), m_updates + _updates);
}

int GetUpdates() {
	return m_updates;
}

int SetUpdates(int _updates) {
	m_updates = _updates;
	return m_updates; // for facility
}

// Commit: only if needed, i.e. if m_object has been set and if the chunk has been updated
// note: global protections here (temporary, I hope). In REAPER v3.66, it's safer NOT to 
// patch while recording and to remove all ids before patching, see http://forum.cockos.com/showthread.php?t=52002 
virtual bool Commit(bool _force = false)
{
	if (m_object && (m_updates || _force) && !(GetPlayState() & 4) && m_chunk->GetLength())	
	{
		if (!SNM_GetSetObjectState(m_object, m_chunk)) 
		{
			SetChunk("", 0);
			return true;
		}
	}
	return false;
}

const char* GetInfo() {
	return "SNM_ChunkParserPatcher - v1.21";
}

void SetProcessBase64(bool _enable) {
	m_processBase64 = _enable;
}

void SetProcessInProjectMIDI(bool _enable) {
	m_processInProjectMIDI = _enable;
}


//**********************
// Helper methods
//**********************

void CancelUpdates() {
	SetChunk("", 0);
}

// returns the start position of the sub-chunk or -1 if not found
// _chunk: output prm, the searched sub-chunk if found
// _breakKeyword: for optimization, optionnal
int GetSubChunk(const char* _keyword, int _depth, int _occurence, WDL_String* _chunk, const char* _breakKeyword = NULL)
{
	int posStartOfSubchunk = -1;
	if (_chunk && _keyword && _depth > 0)
	{
		_chunk->Set("");
		WDL_String startToken;
		startToken.SetFormatted((int)strlen(_keyword)+2, "<%s", _keyword);
		posStartOfSubchunk = Parse(SNM_GET_SUBCHUNK_OR_LINE, _depth, _keyword, startToken.Get(), -1, _occurence, -1, (void*)_chunk, NULL, _breakKeyword);
		if (posStartOfSubchunk <= 0)
		{
			_chunk->Set("");
			posStartOfSubchunk = -1; // force negative return value if 0 (not found)
		}
		else
			posStartOfSubchunk--; // see ParsePatchCore()
	}
	return posStartOfSubchunk;
}

// _newSubChunk: the replacing string (so "" will remove the sub-chunk)
// returns false if nothing done (e.g. sub-chunk not found)
bool ReplaceSubChunk(const char* _keyword, int _depth, int _occurence, const char* _newSubChunk, const char* _breakKeyword = NULL)
{
	if (_keyword && _depth > 0)
	{
		WDL_String startToken;
		startToken.SetFormatted((int)strlen(_keyword)+2, "<%s", _keyword);
		return (ParsePatch(SNM_REPLACE_SUBCHUNK_OR_LINE, _depth, _keyword, startToken.Get(), -1, _occurence, 0, (void*)_newSubChunk, NULL, _breakKeyword) > 0);
	}
	return false;
}

// returns false if nothing done (e.g. sub-chunk not found)
bool RemoveSubChunk(const char* _keyword, int _depth, int _occurence, const char* _breakKeyword = NULL) {
	return ReplaceSubChunk(_keyword, _depth, _occurence, "", _breakKeyword);
}

bool ReplaceLine(int _pos, const char* _str = NULL)
{
	if (_pos >=0 && GetChunk()->GetLength() > _pos) // + indirectly cache chunk if needed
	{
		int pos = _pos;
		char* pChunk = m_chunk->Get();
		while (pChunk[pos] && pChunk[pos] != '\n') pos++;
		if (pChunk[pos] == '\n')
		{
			m_chunk->DeleteSub(_pos, (pos+1) - _pos);
			if (_str && *_str)
				m_chunk->Insert(_str, _pos);
			m_updates++;
			return true;
		}
	}
	return false;
}

// This will replace the line(s) begining with _keyword
bool ReplaceLine(const char* _parent, const char* _keyword, int _depth, int _occurence, const char* _newSubChunk = "", const char* _breakKeyword = NULL)
{
	if (_keyword && _depth > 0)
		return (ParsePatch(SNM_REPLACE_SUBCHUNK_OR_LINE, _depth, _parent, _keyword, -1, _occurence, 0, (void*)_newSubChunk, NULL, _breakKeyword) > 0);
	return false;
}

// This will insert _str at the after (_dir=1) or before (_dir=0) _keyword (i.e. at next/previous start of line)
bool InsertAfterBefore(int _dir, const char* _str, const char* _parent, const char* _keyword, int _depth, int _occurence, const char* _breakKeyword = NULL)
{
	if (_str && *_str && _keyword && GetChunk()) // force cache if needed
	{
		int pos = GetLinePos(_dir, _parent, _keyword, _depth, _occurence, _breakKeyword);
		if (pos >= 0) {
			m_chunk->Insert(_str, pos);
			m_updates++;
			return true;
		}
	}
	return false;
}

int RemoveLines(const char* _removedKeyword, bool _checkBOL = true, int _checkEOLChar = 0) {
	return SetUpdates(RemoveChunkLines(GetChunk(), _removedKeyword, _checkBOL, _checkEOLChar));
}

int RemoveLines(WDL_PtrList<const char>* _removedKeywords, bool _checkBOL = true, int _checkEOLChar = 0) {
	return SetUpdates(RemoveChunkLines(GetChunk(), _removedKeywords, _checkBOL, _checkEOLChar));
}

// Notes:
// - m_updates is volontary ignored here, not considered as an user update (internal)
// - preserves POOLEDEVTS ids
int RemoveIds() {
	return RemoveChunkLines(GetChunk(), "ID {", false, '}');
}

// This will return the start of the current, next or previous line position for the searched _keyword
// _dir: -1 previous line, 0 current line, +1 next line
// _breakKeyword: for optimization, optionnal. If specified, the parser won't go further when this keyword is encountred, be carefull with that!
int GetLinePos(int _dir, const char* _parent, const char* _keyword, int _depth, int _occurence, const char* _breakKeyword = NULL)
{
	int pos = Parse(SNM_GET_CHUNK_CHAR, _depth, _parent, _keyword, -1, _occurence, 0, NULL, NULL, _breakKeyword);
	if (pos > 0)
	{
		pos--; // See ParsePatchCore()
		if (_dir == -1 || _dir == 1)
		{
			char* pChunk = m_chunk->Get();
			if (_dir == -1 && pos >= 2)
				pos-=2; // zap the previous '\n'
			while (pChunk[pos] && pChunk[pos] != '\n') pos += _dir;
			if (pChunk[pos] && pChunk[pos+1])
				return pos+1;
		}
		else
			return pos;
	}
	return -1;
}

const char* GetParent(WDL_PtrList<WDL_String>* _parents, int _ancestor=1) {
	int sz = _parents ? _parents->GetSize() : 0;
	if (sz >= _ancestor)
		return _parents->Get(sz-_ancestor)->Get();
	return "";
}


///////////////////////////////////////////////////////////////////////////////
protected:
	WDL_String* m_chunk;
	bool m_autoCommit;
	void* m_object;
	int m_updates;

	// 'Advanced' optionnal optimization flags 
	// ------------------------------------------------------------------------
	bool m_breakParsePatch; // this one can be enabled to break parsing (and patching: bulk recopy)

	// Base64 sub-chunks and in-project MIDI data are skipped by default (can be HUGE,
	// i.e. several Mo). You can enable these attributes to force their processing
	bool m_processBase64;
	bool m_processInProjectMIDI;

	bool m_isParsingSource; // this one is READ-ONLY (automatically set when parsing)


char* SNM_GetSetObjectState(void* _obj, WDL_String* _str)
{
#ifdef _SWS_EXTENSION
	return SWS_GetSetObjectState(_obj, _str);
#else
	if (_str)
	{
		RemoveAllIds(_str);
#ifdef _SNM_DEBUG
		char filename[BUFFER_SIZE] = "";
		sprintf(filename, "%s%cSNM_ChunkParserPatcher_lastCommit.txt", GetExePath(), PATH_SLASH_CHAR);
		FILE* f = fopen(filename, "w"); 
		if (f) {
			fputs(_str->Get(), f);
			fclose(f);
		}
#endif
	}
	return GetSetObjectState(_obj, _str ? _str->Get() : NULL);
#endif
}

	// Parsing callbacks (to be implemented for SAX-ish parsing style)
	// ------------------------------------------------------------------------
	// Parameters:
	// _mode: parsing mode, see ParsePatchCore(), <0 for custom parsing modes
	// _lp: the line being parsed as a LineParser
	// _parsedLine: the line being parsed (for facility: can be built from _lp)
	// _linePos: start position in the original chunk of the line being parsed 
	// _parsedParents: the parsed line's parent, grand-parent, etc.. up to the root. 
	//                 The number of items is also the parsed depth (so 1-based)
	// _newChunk: the chunk beeing built (while parsing)
	// _updates: number of updates in comparison with the original chunk
	//
	// Return values: 
	// - true if the chunk has been altered 
	//   => THE LINE BEING PARSED IS REPLACED WITH _newChunk
	// - false otherwise
	//   => THE LINE BEING PARSED REMAINS AS IT IS
	//
	// Those callbacks are *always* triggered, execpt NotifyChunkLine() that 
	// is triggered depending on Parse() or ParsePatch() parameters/criteria 
	// => for optimization: the more criteria, the less calls!

	virtual void NotifyStartChunk(int _mode) {}

	virtual void NotifyEndChunk(int _mode) {}

	virtual bool NotifyStartElement	(int _mode, 
		LineParser* _lp, const char* _parsedLine,  int _linePos, 
		WDL_PtrList<WDL_String>* _parsedParents, 
		WDL_String* _newChunk, int _updates) {return false;} // no update

	virtual bool NotifyEndElement(int _mode, 
		LineParser* _lp, const char* _parsedLine, int _linePos,
		WDL_PtrList<WDL_String>* _parsedParents, 
		WDL_String* _newChunk, int _updates) {return false;} // no update

	virtual bool NotifyChunkLine(int _mode, 
		LineParser* _lp, const char* _parsedLine, int _linePos, 
		int _parsedOccurence, WDL_PtrList<WDL_String>* _parsedParents, 
		WDL_String* _newChunk, int _updates) {return false;} // no update

	virtual bool NotifySkippedSubChunk(int _mode, 
		const char* _subChunk, int _subChunkLength, int _subChunkPos,
		WDL_PtrList<WDL_String>* _parsedParents, 
		WDL_String* _newChunk, int _updates) {return false;} // no update


///////////////////////////////////////////////////////////////////////////////
private:

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
			_chunkLine->Append(i == (numtokens-1) ? "\n" : " ");
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

	if (_expectedNumTokens > 0 && _expectedNumTokens != _parsedNumTokens)
		return;

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
			else if ((_expectedNumTokens == -1 || _expectedNumTokens == _parsedNumTokens) && !strcmp(_parsedKeyword, _expectedKeyword)) {
				*_strictMatch = *_tolerantMatch = true;
/*JFB OK
			else if (!strcmp(_parsedKeyword, _expectedKeyword)) {
				*_strictMatch = *_tolerantMatch = 
					(_expectedNumTokens == -1 || _expectedNumTokens == _parsedNumTokens);
*/
			}
		}
	}
}

// *** ParsePatchCore() ***
//
// Globaly, the method is tolerant; the less parameters provided, the more parsed
// lines will be notified to inherited instances (through NotifyChunkLine()) or, 
// when it's used direcly, the more lines will be read/altered.
// Examples: parse all lines, is the n-th FX bypassed under parent 'FXCHAIN'? etc..
// Note: sometimes there are dependencies between parameters (most of the time with
//       _mode), must return -1 if it's not respected.
//
// Parameters: see below. 
//
// Return values:
//   Always return -1 on error/bad usage,
//   or returns the number of updates done when altering (0 nothing done), 
//   or returns the first found position +1 in the chunk (0 reserved for "not found")
//
int ParsePatchCore(
	bool _write,        // optimization flag (if false: no re-copy)
	int _mode,          // can be <0 for custom modes (usefull for inheritance)
	int _depth,         // 1-based!
	const char* _expectedParent, 
	const char* _keyWord, 
	int _numTokens,     // 1-based (-1: ignored)
	int _occurence,     // 0-based (-1: ignored, all occurences notified)
	int _tokenPos,      // 0-based (-1: ignored, may be mandatory depending on _mode)
	void* _value,       // value to get/set (NULL: ignored)
	void* _valueExcept, // value to get/set for the "except case" (NULL: ignored)
	const char* _breakKeyword = NULL) // // for optimization, optionnal (if specified, processing is stopped when this keyword is encountred - be carefull with that!)
{
#ifdef _SNM_DEBUG 
	// check params
	if ((!_value || _tokenPos < 0) && 
			(_mode == SNM_SET_CHUNK_CHAR || _mode == SNM_SETALL_CHUNK_CHAR_EXCEPT || _mode == SNM_GETALL_CHUNK_CHAR_EXCEPT))
		return -1;
	if (_tokenPos < 0 && _mode == SNM_GET_CHUNK_CHAR)
		return -1;

	// sub-chunk processing: depth must always be provided but sometimes _value can be optionnal
	if ((!_value || _depth <= 0) && _mode == SNM_REPLACE_SUBCHUNK_OR_LINE)
		return -1;
	// sub-chunk processing: depth must always be provided 
	if (_depth <= 0 && (_mode == SNM_GET_SUBCHUNK_OR_LINE || _mode == SNM_GET_SUBCHUNK_OR_LINE_EOL))
		return -1;
	// count keywords: no given occurence should be provided
	if (_mode == SNM_COUNT_KEYWORD && _occurence != -1)
		return -1;
#endif

	// get/cache the chunk
	char* cData = GetChunk() ? GetChunk()->Get() : NULL;
	if (!cData)
		return -1;

	// Start of chunk parsing
	NotifyStartChunk(_mode);

	WDL_String* newChunk = NULL; 
	if (_write)
		newChunk = new WDL_String(SNM_HEAPBUF_GRANUL);

	char curLine[SNM_MAX_CHUNK_LINE_LENGTH] = "";
	int updates = 0, occurence = 0, posStartOfSubchunk = -1;
	WDL_String* subChunkKeyword = NULL;
	WDL_PtrList_DeleteOnDestroy<WDL_String> parents;
	m_isParsingSource = false; // avoids many strcmp() calls
	char* pEOL = cData-1;

	// OK, big stuff begins
	for(;;)
	{
		char* pLine = pEOL+1;
		pEOL = strchr(pLine, '\n');
		// Break conditions
		if (!pEOL) break;
		if (m_breakParsePatch) {
			if (_write) newChunk->Append(pLine);
			break;
		}
		int curLineLength = (int)(pEOL-pLine); // avoids many strlen() calls


		// *** Major optimization: skip some sub-chunks (optionnal) ***
		char* pEOSkippedChunk = NULL;

		// skip base64 sub-chunks (e.g. FX states, sysEx events, ..)
		if (!m_processBase64 &&
			curLineLength>2 && *(pEOL-1)=='=' && *(pEOL-2)=='=')
		{
			pEOSkippedChunk = strchr(pLine, '>');
		}
		// skip in-project MIDI
		else if (!m_processInProjectMIDI && m_isParsingSource &&
			(curLineLength>2 && !_strnicmp(pLine, "E ", 2)) ||
			(curLineLength>3 && !_strnicmp(pLine, "Em ", 3)))
		{
			pEOSkippedChunk = strstr(pLine, "GUID {");
		}
		
		if (pEOSkippedChunk) 
		{
			bool alter = NotifySkippedSubChunk(_mode, pLine, (int)(pEOSkippedChunk-pLine), (int)(pLine-cData), &parents, newChunk, updates);
			alter |= (subChunkKeyword && _mode == SNM_REPLACE_SUBCHUNK_OR_LINE);
			if (_write && !alter)
				newChunk->Insert(pLine, newChunk->GetLength(), (int)(pEOSkippedChunk-pLine));
			if ((_mode == SNM_GET_SUBCHUNK_OR_LINE || _mode == SNM_GET_SUBCHUNK_OR_LINE_EOL) && subChunkKeyword && _value)
				((WDL_String*)_value)->Insert(pLine, ((WDL_String*)_value)->GetLength(), (int)(pEOSkippedChunk-pLine));

			pLine = pEOSkippedChunk;
			pEOL = strchr(pEOSkippedChunk, '\n');
			curLineLength = (int)(pEOL-pLine);
		}


		// *** Next line parsing ***
		bool alter = false;
		memcpy(curLine, pLine, curLineLength);
		curLine[curLineLength] = '\0'; // min(SNM_MAX_CHUNK_LINE_LENGTH-1, curLineLength) ?
		int linePos = (int)(pLine-cData);

		const char* keyword = NULL;
		LineParser lp(false);
		lp.parse(curLine);
		int lpNumTokens=lp.getnumtokens();
		if (lpNumTokens > 0)
		{
			keyword = lp.gettoken_str(0);
			if (*keyword == '<')
			{
				m_isParsingSource |= !strcmp(keyword+1, "SOURCE");

				// notify & update parent list
				parents.Add(new WDL_String(keyword+1)); // +1 to zap '<'
				alter = NotifyStartElement(_mode, &lp, curLine, linePos, &parents, newChunk, updates);
			}
			else if (*keyword == '>')
			{
				// end of processed sub-chunk ?
				if (subChunkKeyword && _depth == parents.GetSize() && 
					subChunkKeyword == parents.Get(_depth-1)) // => valid _depth must be provided
				{
					subChunkKeyword = NULL;

					// don't recopy '>' depending on the modes..
					if (_mode == SNM_REPLACE_SUBCHUNK_OR_LINE) {
						alter = true;
						m_breakParsePatch = true;
					}
					// .. but recopy it for some
					else if (_mode == SNM_GET_SUBCHUNK_OR_LINE || _mode == SNM_GET_SUBCHUNK_OR_LINE_EOL)
					{
						if (_value) 
							((WDL_String*)_value)->Append(">\n",2);

						// * SNM_GET_SUBCHUNK_OR_LINE:
						// returns 1st *KEYWORD* position of the sub-chunk + 1 ('cause 0 reserved for "not found")
						// * Otherwise:
						// return the *EOL* position of the sub-chunk + 1 ('cause 0 reserved for "not found")
						return (_mode == SNM_GET_SUBCHUNK_OR_LINE ? posStartOfSubchunk : (int)(pEOL-cData+1));
					}
				}

				if (m_isParsingSource) 
					m_isParsingSource = !!strcmp(GetParent(&parents), "SOURCE");

				// notify & update parent list
				alter |= NotifyEndElement(_mode, &lp, curLine, linePos, &parents, newChunk, updates);
				parents.Delete(parents.GetSize()-1, true);
			}
		}

		// end of chunk lines (">") are not processed/notified (but copied if needed)
		if (parents.GetSize())
		{
			WDL_String* currentParent = parents.Get(parents.GetSize()-1);

			bool tolerantMatch, strictMatch;
			IsMatchingParsedLine(
				&tolerantMatch, &strictMatch, 
				_depth, parents.GetSize(), 
				_expectedParent, currentParent->Get(), 
				_keyWord, keyword, 
				_numTokens, lpNumTokens);

			if (tolerantMatch && _mode < 0)
			{
				if (_occurence == occurence || _occurence == -1)
					alter |= NotifyChunkLine(_mode, &lp, curLine, linePos, occurence, &parents, newChunk, updates); 
				occurence++;
			}
			else if (strictMatch && _mode >= 0)
			{
				// this occurence match
				if (_occurence == occurence)
				{
					switch (_mode)
					{
						case SNM_GET_CHUNK_CHAR:
						{
							if (_value) strcpy((char*)_value, lp.gettoken_str(_tokenPos));
							char* p = strstr(pLine, _keyWord);
							// returns the *KEYWORD* position + 1 ('cause 0 reserved for "not found")
							return (p ? ((int)(p-cData+1)) : -1); 
						}
						case SNM_SET_CHUNK_CHAR:
							alter |= WriteChunkLine(newChunk, (char*)_value, _tokenPos, &lp); 
							m_breakParsePatch = true;
							break;
						case SNM_SETALL_CHUNK_CHAR_EXCEPT:
						case SNM_TOGGLE_CHUNK_INT_EXCEPT:
							if (_valueExcept){
								if (strcmp((char*)_valueExcept, lp.gettoken_str(_tokenPos)) != 0)
									alter |= WriteChunkLine(newChunk, (char*)_valueExcept, _tokenPos, &lp); 
							}
							break;
						case SNM_PARSE_AND_PATCH:
						case SNM_PARSE:
							alter |= NotifyChunkLine(_mode, &lp, curLine, linePos, occurence, &parents, newChunk, updates);
							m_breakParsePatch = true;
							break;
						case SNM_TOGGLE_CHUNK_INT:
						{
							char bufConversion[16] = "";
							sprintf(bufConversion, "%d", !lp.gettoken_int(_tokenPos));
							alter |= WriteChunkLine(newChunk, bufConversion, _tokenPos, &lp); 
							m_breakParsePatch = true;
						}
						break; 
						case SNM_REPLACE_SUBCHUNK_OR_LINE:
							newChunk->Append((char*)_value);
							if (*_keyWord == '<') subChunkKeyword = currentParent;
							alter=true;
							break;
						case SNM_GET_SUBCHUNK_OR_LINE:
						{
							if (_value) ((WDL_String*)_value)->AppendFormatted(curLineLength+2, "%s\n", curLine);
							char* pSub = strstr(pLine, _keyWord);
							// *KEYWORD* position + 1 ('cause 0 reserved for "not found")
							posStartOfSubchunk = (pSub ? ((int)(pSub-cData+1)) : -1);
							if (*_keyWord == '<') subChunkKeyword = currentParent;
							else return posStartOfSubchunk; 
						}
						break;
						case SNM_GET_SUBCHUNK_OR_LINE_EOL:
						{							
							if (_value) ((WDL_String*)_value)->AppendFormatted(curLineLength+2, "%s\n", curLine);
							if (*_keyWord == '<') subChunkKeyword = currentParent;
							else return ((int)(pEOL-cData+1)); // *EOL* position + 1 ('cause 0 reserved for "not found")
						}
						break;
						default: // for custom _mode (e.g. <0)
							break;
					}
				}
				// this occurence doesn't match (or _occurence == -1)
				else 
				{
					switch (_mode)
					{
						case SNM_COUNT_KEYWORD:
							break;
						case SNM_SETALL_CHUNK_CHAR_EXCEPT:
							if (strcmp((char*)_value, lp.gettoken_str(_tokenPos)) != 0)
								alter |= WriteChunkLine(newChunk, (char*)_value, _tokenPos, &lp); 
							break;
						case SNM_GETALL_CHUNK_CHAR_EXCEPT:
							if (strcmp((char*)_value, lp.gettoken_str(_tokenPos)) != 0)
								return 0;
							break;
						case SNM_PARSE_AND_PATCH_EXCEPT:
						case SNM_PARSE_EXCEPT:
							alter |= NotifyChunkLine(_mode, &lp, curLine, linePos, occurence, &parents, newChunk, updates); 
							break;
						case SNM_TOGGLE_CHUNK_INT_EXCEPT:
						{
							char bufConversion[16] = "";
							sprintf(bufConversion, "%d", !lp.gettoken_int(_tokenPos));
							alter |= WriteChunkLine(newChunk, bufConversion, _tokenPos, &lp); 
						}
						break; 
						default: // for custom _mode (e.g. <0)
							break;
					}
				}
				occurence++;
			}
			else if (!subChunkKeyword && _keyWord && _breakKeyword && !strcmp(keyword, _breakKeyword))
			{
				m_breakParsePatch = true;
			}
			// are we in a processed sub-chunk?
			else if (subChunkKeyword) 
			{
				alter = (_mode == SNM_REPLACE_SUBCHUNK_OR_LINE);
				if (_value && (_mode == SNM_GET_SUBCHUNK_OR_LINE || _mode == SNM_GET_SUBCHUNK_OR_LINE_EOL))
					((WDL_String*)_value)->AppendFormatted(curLineLength+2, "%s\n", curLine);
			}
		}
		updates += (_write && alter);

		// copy current line if RW mode & if it wasn't altered above & if inerited classes 
		// authorize it (for optimization when using matching criteria: depth, parent,..)
		if (_write && !alter && lpNumTokens)
			newChunk->AppendFormatted(curLineLength+2, "%s\n", curLine);
	}

	// Update chunk cache if needed
	if (_write && newChunk)
	{
		if (updates && newChunk->GetLength()) 
		{
			m_updates += updates;
			// avoids buffer re-copy
			WDL_String* oldChunk = m_chunk;
			m_chunk = newChunk; 
			delete oldChunk;
		}
		else
			delete newChunk;
	}

	// End of chunk parsing
	NotifyEndChunk(_mode);

	// return values
	int retVal = -1;
	switch (_mode)
	{
		// *** READ ONLY ***
		case SNM_GET_CHUNK_CHAR:
		case SNM_GET_SUBCHUNK_OR_LINE:
		case SNM_GET_SUBCHUNK_OR_LINE_EOL:
			retVal = 0; // if we're here: not found
			break;
		case SNM_GETALL_CHUNK_CHAR_EXCEPT:
			retVal = 1; // if we're here: found (returns 0 on 1st unmatching)
			break;
		case SNM_PARSE:
		case SNM_PARSE_EXCEPT:
			retVal = 1; // read ok..
			break;
		case SNM_COUNT_KEYWORD:
			retVal = occurence;
			break;

		// *** R/W ***
		case SNM_PARSE_AND_PATCH:
		case SNM_PARSE_AND_PATCH_EXCEPT:
		case SNM_SET_CHUNK_CHAR:
		case SNM_SETALL_CHUNK_CHAR_EXCEPT:
		case SNM_TOGGLE_CHUNK_INT:
		case SNM_TOGGLE_CHUNK_INT_EXCEPT:
		case SNM_REPLACE_SUBCHUNK_OR_LINE:
			retVal = updates;
			break;

		default: // for custom _mode (e.g. <0)
			retVal = (_write ? updates : 0);
			break;
	}

	// Safer (if inherited classes forget to do it)
	m_breakParsePatch = false;

	return retVal;
}
};


///////////////////////////////////////////////////////////////////////////////
// Fast chunk processing helpers
///////////////////////////////////////////////////////////////////////////////

static SNM_ChunkParserPatcher* FindChunkParserPatcherByObject(WDL_PtrList<SNM_ChunkParserPatcher>* _list, void* _object)
{
	if (_list && _object) {
		for (int i=0; i < _list->GetSize(); i++)
			if (_list->Get(i)->GetObject() == _object)
				return _list->Get(i);
	}
	return NULL;
}

// _len: optionnal IN (initial length) -and- OUT (length after processing) param
static int RemoveChunkLines(char* _chunk, const char* _searchStr, bool _checkBOL, int _checkEOLChar, int* _len)
{
	if (!_chunk || !_searchStr)
		return 0;

	int updates = 0;
	int len = (!_len ? strlen(_chunk) : *_len);
	char* idStr = strstr(_chunk, _searchStr);
	while(idStr) 
	{
		char* eol = strchr(idStr, '\n');
		char* bol = idStr;
		while (*bol && bol > _chunk && *bol != '\n') bol--;
		if (eol && bol && (*bol == '\n' || bol == _chunk) &&
			// additionnal optionnal checks (safety)
			 (!_checkEOLChar || (_checkEOLChar && *((char*)(eol-1)) == _checkEOLChar)) &&
			 (!_checkBOL || (_checkBOL && idStr == (char*)(bol + ((bol == _chunk ? 0 : 1))))))
		{
			updates++; eol++; 
			if (bol != _chunk) bol++;
			// we avoid a bunch of strlen(eol) here
			memmove(bol, eol, len - ((int)(eol-_chunk)) + 1); // +1 for ending '\0'
			len -= (int)(eol-bol);
			idStr = strstr(bol, _searchStr);
		}
		else 
			idStr = strstr((char*)(idStr+1), _searchStr);
	}
	if (_len) *_len = len;
	return updates;
}

// _len: optionnal IN (initial length) -and- OUT (length after processing) param
static int RemoveChunkLines(char* _chunk, WDL_PtrList<const char>* _searchStrs, bool _checkBOL, int _checkEOLChar, int* _len)
{
	if (!_chunk || !_searchStrs)
		return 0;

	int updates = 0;
	int len = (!_len ? strlen(_chunk) : *_len);

	// faster than parsing+check each keyword
	for (int i=0; i < _searchStrs->GetSize(); i++)
		updates += RemoveChunkLines(_chunk, _searchStrs->Get(i), _checkBOL, _checkEOLChar, &len);

	if (_len) *_len = len;
	return updates;
}

static int RemoveChunkLines(WDL_String* _chunk, const char* _searchStr, bool _checkBOL, int _checkEOLChar, int* _len)
{
	if (!_chunk || !_searchStr)
		return 0;

	int len = (!_len ? _chunk->GetLength() : *_len);
	int updates = RemoveChunkLines(_chunk->Get(), _searchStr, _checkBOL, _checkEOLChar, &len);
	if (updates > 0)
		_chunk->SetLen(len, true); // in case my WDL_String mod is used..
	if (_len) *_len = len;
	return updates;
}

static int RemoveChunkLines(WDL_String* _chunk, WDL_PtrList<const char>* _searchStrs, bool _checkBOL, int _checkEOLChar, int* _len)
{
	if (!_chunk || !_searchStrs)
		return 0;

	int len = (!_len ? _chunk->GetLength() : *_len);
	int updates = RemoveChunkLines(_chunk->Get(), _searchStrs, _checkBOL, _checkEOLChar, &len);
	if (updates > 0)
		_chunk->SetLen(len, true); // in case my WDL_String mod is used..
	if (_len) *_len = len;
	return updates;
}

#endif
