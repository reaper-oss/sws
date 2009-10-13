/******************************************************************************
/ SectionLock.cpp
/
/ Copyright (c) 2009 Tim Payne (SWS)
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
#include "SectionLock.h"

SectionLock::SectionLock(HANDLE& pMutex, DWORD dwTimeoutMs)
{
	if (pMutex == NULL)
		pMutex = CreateMutex(NULL, false, NULL);

	m_hMutex = pMutex;

	// Does caller want to acquire sync lock?
	if (dwTimeoutMs != SECLOCK_NO_INITIAL_LOCK)
		WaitForSingleObject(pMutex, dwTimeoutMs);
}

SectionLock::~SectionLock()
{
	Unlock();
}

bool SectionLock::Lock(DWORD dwTimeoutMs)
{
	return WaitForSingleObject(m_hMutex, dwTimeoutMs) != WAIT_FAILED;
}

bool SectionLock::Unlock(void)
{
	return ReleaseMutex(m_hMutex) ? true : false;
}