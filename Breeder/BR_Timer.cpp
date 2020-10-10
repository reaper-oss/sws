/******************************************************************************
/ BR_Timer.cpp
/
/ Copyright (c) 2014 Dominik Martin Drzic
/ http://forum.cockos.com/member.php?u=27094
/ http://github.com/reaper-oss/sws
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
#include "BR_Timer.h"

void CommandTimer (COMMAND_T* ct, int val /*= 0*/, int valhw /*= 0*/, int relmode /*= 0*/, HWND hwnd /*= NULL*/, bool commandHook2 /*= false*/)
{
	#ifdef WIN32
		LARGE_INTEGER ticks, start, end;
		QueryPerformanceFrequency(&ticks);
		QueryPerformanceCounter(&start);
	#else
		timeval start, end;
		gettimeofday(&start, NULL);
	#endif

	if (commandHook2)
		ct->onAction(ct, val, valhw, relmode, hwnd);
	else
		ct->doCommand(ct);

	#ifdef WIN32
		QueryPerformanceCounter(&end);
		int msTime = (int)((double)(end.QuadPart - start.QuadPart) * 1000 / (double)ticks.QuadPart + 0.5);
	#else
		gettimeofday(&end, NULL);
		int msTime = (int)((double)(end.tv_sec - start.tv_sec) * 1000 + (double)(end.tv_usec - start.tv_usec) / 1000 + 0.5);
	#endif

	WDL_FastString string;
	string.AppendFormatted(256, "%d ms to execute: %s\n", msTime, ct->accel.desc);
	ShowConsoleMsg(string.Get());
}

#ifdef BR_DEBUG_PERFORMANCE_TIMER
	BR_Timer::BR_Timer (const char* message, bool autoPrint /*= true*/, bool autoStart /*= true*/) : m_message(message), m_autoPrint(autoPrint), m_paused(!autoStart)
	{
		if (autoStart)
		{
			#ifdef WIN32
				QueryPerformanceCounter(&m_start);
			#else
				gettimeofday(&m_start, NULL);
			#endif
		}
		else
		{
			this->Pause();
			this->Reset();
		}
	}

	BR_Timer::~BR_Timer ()
	{
		if (m_autoPrint)
			this->Progress(NULL);
	}

	void BR_Timer::Pause ()
	{
		if (m_paused)
			return;

		#ifdef WIN32
			QueryPerformanceCounter(&m_pause);
		#else
			gettimeofday(&m_pause, NULL);
		#endif

		m_paused = true;
	}

	void BR_Timer::Resume ()
	{
		if (!m_paused)
			return;

		#ifdef WIN32
			LARGE_INTEGER end;
			QueryPerformanceCounter(&end);
			m_start.QuadPart += end.QuadPart - m_pause.QuadPart;
		#else
			timeval end;
			gettimeofday(&end, NULL);
			timeval temp;
			timersub(&end, &m_pause, &temp);
			timeradd(&m_start, &temp, &m_start);
		#endif

		m_paused = false;
	}

	void BR_Timer::Reset ()
	{
		#ifdef WIN32
			QueryPerformanceCounter(&m_start);
			if (m_paused)
				m_pause.QuadPart = m_start.QuadPart;
		#else
			gettimeofday(&m_start, NULL);
			if (m_paused)
			{
				m_pause.tv_sec = m_start.tv_sec;
				m_pause.tv_usec = m_start.tv_usec;
			}
		#endif
	}

	void BR_Timer::Progress (const char* message /*= NULL*/)
	{
		#ifdef WIN32
			LARGE_INTEGER end, ticks;
			QueryPerformanceCounter(&end);
			QueryPerformanceFrequency(&ticks);
			LARGE_INTEGER start = m_start;
			if (m_paused) // in case timer is still paused
				start.QuadPart += end.QuadPart - m_pause.QuadPart;

			double msTime = (double)(end.QuadPart - start.QuadPart) * 1000 / (double)ticks.QuadPart;
		#else
			timeval end;
			gettimeofday(&end, NULL);
			timeval start = m_start;
			if (m_paused) // in case timer is still paused
			{
				timeval temp;
				timersub(&end, &m_pause, &temp);
				timeradd(&m_start, &temp, &start);
			}

			double msTime = (double)(end.tv_sec - start.tv_sec) * 1000 + (double)(end.tv_usec - start.tv_usec) / 1000;
		#endif

		bool paused = m_paused;
		this->Pause();  // ShowConsoleMsg wastes time and Progress can be called at any time during measurement

		WDL_FastString string;
		string.AppendFormatted(256, "%.3f ms to execute: %s\n", msTime, message ? message : m_message.Get());
		ShowConsoleMsg(string.Get());

		if (!paused)  // resume only if time was not paused in the first place
			this->Resume();
	}
#else
	BR_Timer::BR_Timer (const char* message, bool autoPrint /*= false*/) {}
	BR_Timer::~BR_Timer () {}
	void BR_Timer::Pause () {}
	void BR_Timer::Resume () {}
	void BR_Timer::Reset () {}
	void BR_Timer::Progress (const char* message /*= NULL*/) {}
#endif
