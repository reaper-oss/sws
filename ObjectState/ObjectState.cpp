/******************************************************************************
/ ObjectState.cpp
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
#include "../SnM/SnM_ChunkParserPatcher.h"

// GetSetObjectState can take a very long time to read and/or write, especially with some FX.
// To mitigate these issues when dealing with operations that require many reads/writes,
// Call SWS_GetSetObjectState and SWS_FreeHeapPtr instead.  When you want to cache your
// state reads and writes call SWS_CacheObjectState(true).  When done call SWS_CacheObjectState(false)
// and any changes will be written out.
//
// See Snapshots for an example of use

//#define GOS_DEBUG

ObjectStateCache::ObjectStateCache():m_iUseCount(1)
{
}

ObjectStateCache::~ObjectStateCache()
{
	EmptyCache();
}

void ObjectStateCache::WriteCache()
{
#ifdef GOS_DEBUG
	int iCount = 0;
#endif
	for (int i = 0; i < m_obj.GetSize(); i++)
	{
		if (m_str.Get(i)->GetLength() && m_orig.Get(i) && strcmp(m_str.Get(i)->Get(), m_orig.Get(i)))
		{
			int fxstate = SNM_PreObjectState(m_str.Get(i), false);
			GetSetObjectState(m_obj.Get(i), m_str.Get(i)->Get());
			SNM_PostObjectState(fxstate);
#ifdef GOS_DEBUG
			iCount++;
#endif
		}
	}
#ifdef GOS_DEBUG
	dprintf("ObjectStateCache::WriteCache applied %d chunks.\n", iCount);
#endif

	EmptyCache();
}

void ObjectStateCache::EmptyCache()
{
	m_obj.Empty();
	m_str.Empty(true);
	m_orig.Empty(true, FreeHeapPtr);
}

const char* ObjectStateCache::GetSetObjState(void* obj, const char* str, bool wantsMinimalState)
{
	int i = m_obj.Find(obj);
	if (i < 0)
	{
		i = m_obj.GetSize();
		m_obj.Add(obj);
		m_str.Add(new WDL_FastString);
		if (str && str[0])
			m_orig.Add(NULL);
		else
		{
			int fxstate = SNM_PreObjectState(NULL, wantsMinimalState);
			char* p = GetSetObjectState(obj, NULL);
			SNM_PostObjectState(fxstate);
			m_orig.Add(p);
		}
	}
	if (str && str[0])
	{
		m_str.Get(i)->Set(str);
		return NULL;
	}

	if (m_str.Get(i)->GetLength())
		return m_str.Get(i)->Get();
	else
		return m_orig.Get(i);
}

ObjectStateCache* g_objStateCache = NULL;

const char* SWS_GetSetObjectState(void* obj, WDL_FastString* str, bool wantsMinimalState)
{
	const char* ret;
	
	if (g_objStateCache)
		ret = g_objStateCache->GetSetObjState(obj, str ? str->Get() : NULL, wantsMinimalState);
	else
	{
		int fxstate = SNM_PreObjectState(str, wantsMinimalState);
		ret = GetSetObjectState(obj, str ? str->Get() : NULL);
		SNM_PostObjectState(fxstate);
	}

#ifdef GOS_DEBUG
	char debugStr[4096];
	snprintf(debugStr, 4096, "GetSetObjectState call, obj %08X, IN:\n%s\n\nOUT:\n%s\n\n", obj, str ? str->Get() : "NULL", ret ? ret : "NULL");
	OutputDebugString(debugStr);
#endif

	return ret;
}


void SWS_FreeHeapPtr(void* ptr)
{
	// Ignore frees on cached object states, 
	if (!g_objStateCache)
		FreeHeapPtr(ptr);
}

void SWS_FreeHeapPtr(const char* ptr)
{
	SWS_FreeHeapPtr((void*)ptr);
}

void SWS_CacheObjectState(bool bStart)
{
	static SWS_Mutex mutex;
	if (bStart)
	{
		SWS_SectionLock lock(&mutex);
		if (g_objStateCache)
			g_objStateCache->m_iUseCount++;
		else
			g_objStateCache = new ObjectStateCache;
	}
	else if (g_objStateCache)
	{
		SWS_SectionLock lock(&mutex);
		if (g_objStateCache->m_iUseCount <= 1)
		{
			ObjectStateCache* cache = g_objStateCache;
			g_objStateCache = NULL;
			lock.Unlock(); // Don't maintain the lock when writing the cache
			cache->WriteCache();
			delete cache;
		}
		else
			g_objStateCache->m_iUseCount--;
	}
}

// Helper function for parsing object "chunks" into more useful lines
// newlines are retained.  Caller allocates the WDL_FastString necessary for the output
// pos stores the state of the line parsing, set to zero to return the first line
// Return of true means valid line, false is "end of chunk"
bool GetChunkLine(const char* chunk, char* line, int iLineMax, int* pos, bool bNewLine)
{
	const char* cStart = (chunk + *pos);
	line[0] = 0;
	// Don't error check on strlen(chunk) because chunk can be *huge*

	// Skip blank lines at beginning
	while (*(chunk + *pos) == '\n')
		(*pos)++;

	char c = *(chunk + *pos);
	// Check for end-of-string
	if(!c)
		return false;

	// Find newline or null terminator
	while(c)
	{
		(*pos)++;
		if (c == '\n')
			break;
		c = *(chunk + *pos);
	}

	int iCount = (int)((chunk + *pos) - cStart + (bNewLine ? 0 : -1) + 1);
	if (iCount > iLineMax)
		iCount = iLineMax;
	if (iCount > 0)
		lstrcpyn(line, cStart, iCount);
	
	return true;
}

void AppendChunkLine(WDL_FastString* chunk, const char* line)
{
	// Insert a line into the chunk before the closing >
	const char* pIns = strrchr(chunk->Get(), '>');
	if (!pIns)
		return;
	int pos = (int)(pIns - chunk->Get());
	if (line[strlen(line)-1] != '\n')
		chunk->Insert("\n", pos);
	chunk->Insert(line, pos);
}

bool GetChunkFromProjectState(const char* cSection, WDL_TypedBuf<char>* chunk, const char* firstline, ProjectStateContext *ctx)
{
	if (strncmp(firstline, cSection, strlen(cSection)) == 0)
	{
		// Add first line
		int iLen = (int)strlen(firstline);
		chunk->Resize(iLen + 2);
		strcpy(chunk->Get(), firstline);
		strcpy(chunk->Get()+iLen, "\n");
		iLen++;
		
		int iDepth = 1;
		char linebuf[4096];

		while (iDepth)
		{
			ctx->GetLine(linebuf, 4096);
			int iNewLen = iLen + (int)strlen(linebuf);
			chunk->Resize(iNewLen + 2);
			strcpy(chunk->Get()+iLen, linebuf);
			strcpy(chunk->Get()+iNewLen, "\n");
			iLen = iNewLen + 1;
			if (linebuf[0] == '<')
				iDepth++;
			else if (linebuf[0] == '>')
				iDepth--;
		}
		return true;
	}
	return false;
}
