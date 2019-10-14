/******************************************************************************
/ MuteState.h
/
/ Copyright (c) 2009 Tim Payne (SWS)
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

class MuteItem
{
public:
	MuteItem(GUID* pGuid, bool bState);
	MuteItem(LineParser* lp);

	GUID m_guid;
	bool m_bState;
};

class MuteState
{
public:
	MuteState(MediaTrack* tr);
	MuteState(LineParser* lp);
	~MuteState();
	void Restore(MediaTrack* tr);
    char* ItemString(char* str, int maxLen, bool* bDone);

	WDL_PtrList<MuteItem> m_children;
	WDL_PtrList<MuteItem> m_receives;
	GUID m_guid;
	bool m_bMute;
	int m_iSendToParent;
};

extern SWSProjConfig<WDL_PtrList_DOD<MuteState> > g_muteStates;

void SaveMutes(COMMAND_T* = NULL);
void RestoreMutes(COMMAND_T* = NULL);
