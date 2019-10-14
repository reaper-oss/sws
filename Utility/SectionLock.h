/******************************************************************************
/ SectionLock.h
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

// SWS_Mutex: OS independently wraps a mutex.  You can use this class alone, or
// for a slightly easier way, use SWS_SectionLock below.
// Note: SWS_Mutex is re-entrant (the same thread can acquire the lock multiple times)
class SWS_Mutex
{
#ifdef _WIN32
private:
	HANDLE m_hMutex;
public:
	SWS_Mutex()  { m_hMutex = CreateMutex(NULL, false, NULL); }
	~SWS_Mutex() { CloseHandle(m_hMutex); }
	bool Lock(DWORD dwTimeoutMs) { return WaitForSingleObject(m_hMutex, dwTimeoutMs) != WAIT_FAILED; }
	bool Unlock() { return ReleaseMutex(m_hMutex) ? true : false; }
#else
private:
	pthread_mutex_t m_mutex;
public:
	SWS_Mutex()
	{
		pthread_mutexattr_t attr; pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE); 
		pthread_mutex_init(&m_mutex, &attr);
	}
	~SWS_Mutex() { Unlock(); pthread_mutex_destroy(&m_mutex); }
	bool Lock(DWORD dwTimeoutMs)
	{
		// Try quickly to unlock first
		bool bLocked = pthread_mutex_trylock(&m_mutex) == 0;
		if (bLocked)
			return true;

		// Couldn't unlock immediately, so go into the wait loop
		DWORD t = GetTickCount();
		do
		{
			Sleep(1);
			bLocked = pthread_mutex_trylock(&m_mutex) == 0;
		}
		while (!bLocked && GetTickCount() - t < dwTimeoutMs);
		return bLocked;
	}
	bool Unlock() { return pthread_mutex_unlock(&m_mutex) == 0; }
#endif
};

// SWS_SectionLock is a "helper class" for SWS_Mutex - it auto-locks when instatiated and unlocks when it goes out of scope.
// That way, you can't forget to Unlock the mutex at the end of your function.
//
// Example:
// You have a class that you want to make thread safe
// add a member variable:
// SWS_Mutex m_mutex;
//
// Then, at the beginning of each function call you want to lock, add the line:
// SWS_SectionLock lock(&m_mutex);
//
#define SECLOCK_DEFAULT_TIMEOUT	 10000
#define SECLOCK_INFINITE_TIMEOUT INFINITE
#define SECLOCK_NO_INITIAL_LOCK  0
class SWS_SectionLock
{
private:
	SWS_Mutex* m_pMutex;

public:
	SWS_SectionLock(SWS_Mutex* lock, DWORD dwTimeoutMs = SECLOCK_DEFAULT_TIMEOUT):m_pMutex(lock)
	{	// Does caller want to acquire sync lock?
		if (dwTimeoutMs != SECLOCK_NO_INITIAL_LOCK)
			Lock(dwTimeoutMs);
	}
	~SWS_SectionLock() { Unlock(); }
	bool Lock(DWORD dwTimeoutMs = SECLOCK_DEFAULT_TIMEOUT) { return m_pMutex->Lock(dwTimeoutMs); }
	bool Unlock(void) { return m_pMutex->Unlock(); }
};
