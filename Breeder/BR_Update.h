/******************************************************************************
/ BR_Update.h
/
/ Copyright (c) 2013 and later Dominik Martin Drzic
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
* Version check init/exit                                                     *
******************************************************************************/
int VersionCheckInitExit (bool init);

/******************************************************************************
* Command and preferences (used from About dialog)                            *
******************************************************************************/
void VersionCheckAction (COMMAND_T*);
void VersionCheckDialog (HWND hwnd);
void GetStartupSearchOptions (bool* official, bool* beta, unsigned int* lastTime);
void SetStartupSearchOptions (bool official, bool beta, unsigned int lastTime);

/******************************************************************************
* Holds version information                                                   *
******************************************************************************/
struct BR_Version
{
	int maj;        // major
	int min;        // minor
	int rev;        // revision
	int build;      // build
	BR_Version () {maj = min = rev = build = 0;}
};

/******************************************************************************
* To start search, simply create the object and use GetStatus to query        *
* results from the search thread.                                             *
******************************************************************************/
class BR_SearchObject
{
public:
	explicit BR_SearchObject (bool startup = false);
	~BR_SearchObject ();
	int GetStatus (BR_Version* official, BR_Version* beta);
	double GetProgress ();
	void RestartSearch ();

private:
	static unsigned WINAPI StartSearch (void* searchObject);
	void SetStatus (BR_Version official, BR_Version beta, int status);
	void SetProgress (double progress);
	bool GetKillFlag ();
	void SetKillFlag (bool killFlag);
	HANDLE GetProcess ();
	void SetProcess (HANDLE process);
	void EndSearch ();
	bool IsStartup ();
	int CompareVersion (BR_Version one, BR_Version two);
	const bool m_startup;
	int m_status;
	double m_progress;
	BR_Version m_official;
	BR_Version m_beta;
	SWS_Mutex m_mutex;
	static bool m_killFlag;
	static HANDLE m_process;
};
