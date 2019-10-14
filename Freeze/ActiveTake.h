/******************************************************************************
/ ActiveTake.h
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

class ActiveTake
{
public:
	ActiveTake(MediaItem* mi);
	ActiveTake(LineParser* lp);
	bool Restore(MediaItem* mi);
    char* ItemString(char* str, int maxLen);

	GUID m_item;
	GUID m_activeTake;
};

class ActiveTakeTrack
{
public:
	ActiveTakeTrack(MediaTrack* tr);
	ActiveTakeTrack(LineParser* lp);
	~ActiveTakeTrack();
	void Restore(MediaTrack* tr);
    char* ItemString(char* str, int maxLen);

	WDL_PtrList<ActiveTake> m_items;
	GUID m_guid;
};

extern SWSProjConfig<WDL_PtrList_DOD<ActiveTakeTrack> > g_activeTakeTracks;

void SaveActiveTakes(COMMAND_T* = NULL);
void RestoreActiveTakes(COMMAND_T* = NULL);
