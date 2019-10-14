 /******************************************************************************
/ BR_Timer.h
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
#pragma once

/******************************************************************************
* Uncomment do enable timer functionality                                     *
******************************************************************************/
//#define BR_DEBUG_PERFORMANCE_ACTIONS
#define BR_DEBUG_PERFORMANCE_TIMER

/******************************************************************************
* Used in command hook in sws_extension.cpp. If BR_DEBUG_PERFORMANCE_ACTIONS  *
* is defined the execution time of SWS actions gets printed to the console    *
*******************************************************************************/
void CommandTimer (COMMAND_T* ct, int val = 0, int valhw = 0, int relmode = 0, HWND hwnd = NULL, bool commandHook2 = false);

/******************************************************************************
* Creating the object starts the timer (if autoStart is true). When the       *
* object goes out of scope, elapsed time is printed to the console along with *
* the message.                                                                *
* To prevent printing when going out of scope, supply autoPrint as false and  *
* use Progress(message) to manually print current progress.                   *
* Calling Progress with NULL or no parameters will use the message supplied   *
* at object creation.                                                         *
* When BR_DEBUG_PERFORMANCE_TIMER is not defined the class is empty so you    *
* can quickly enable/disable timer when needed                                *
******************************************************************************/
class BR_Timer
{
public:
	BR_Timer (const char* message, bool autoPrint = true, bool autoStart = true);
	~BR_Timer ();
	void Pause ();
	void Resume ();
	void Reset ();
	void Progress (const char* message = NULL);

#ifdef BR_DEBUG_PERFORMANCE_TIMER
	private:
		bool m_paused, m_autoPrint;
		WDL_FastString m_message;
		#ifdef WIN32
			LARGE_INTEGER m_start, m_pause;
		#else
			timeval m_start, m_pause;
		#endif
#endif
};
