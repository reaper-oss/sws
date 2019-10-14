/******************************************************************************
/ ObjectState.h
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


#pragma once

class ObjectStateCache
{
public:
	ObjectStateCache();
	~ObjectStateCache();
	void WriteCache();
	void EmptyCache();
	const char* GetSetObjState(void* obj, const char* str, bool wantsMinimalState = false);
	int m_iUseCount;
private:
	WDL_PtrList<void> m_obj;
	WDL_PtrList<WDL_FastString> m_str;
	WDL_PtrList<char> m_orig;
};

const char* SWS_GetSetObjectState(void* obj, WDL_FastString* str, bool wantsMinimalState = false);
void SWS_FreeHeapPtr(void* ptr);
void SWS_FreeHeapPtr(const char* ptr);
void SWS_CacheObjectState(bool bStart);

bool GetChunkLine(const char* chunk, char* line, int iLineMax, int* pos, bool bNewLine);
void AppendChunkLine(WDL_FastString* chunk, const char* line);
bool GetChunkFromProjectState(const char* cSection, WDL_TypedBuf<char>* chunk, const char* line, ProjectStateContext *ctx);
