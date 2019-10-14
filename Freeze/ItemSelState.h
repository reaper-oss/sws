/******************************************************************************
/ ItemSelState.h
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

#define SEL_SLOTS 5

class SelItems
{
public:
	SelItems() {};
	~SelItems() { Empty(); }
	void Save(MediaTrack* tr);
	void Add(LineParser* lp);
	void Restore(MediaTrack* tr);
	char* ItemString(char* str, int maxLen, bool* bDone);
	int NumItems() { return m_selItems.GetSize(); }
	void Empty() { m_selItems.Empty(true); }

private:
	void Add(MediaTrack* tr);
	void Match(MediaTrack* tr, bool* bUsed);
	WDL_PtrList<GUID> m_selItems;
};

class SelItemsTrack
{
public:
	SelItemsTrack(MediaTrack* tr);
	SelItemsTrack(LineParser* lp);
	~SelItemsTrack();
	void Add(LineParser* lp, int iSlot);
	void SaveTemp(MediaTrack* tr);
	void Save(MediaTrack* tr, int iSlot);
	void Restore(MediaTrack* tr, int iSlot);
	void PreviousSelection(MediaTrack* tr);
    char* ItemString(char* str, int maxLen, bool* bDone);

	SelItems* m_selItems[SEL_SLOTS];
	SelItems* m_lastSel;
	GUID m_guid;
};

extern SWSProjConfig<WDL_PtrList_DOD<SelItemsTrack> > g_selItemsTrack;
extern SWSProjConfig<SelItems> g_selItems;

// "Exported" functions
void RestoreSelTrackSelItems(COMMAND_T* = NULL);
void SaveSelTrackSelItems(COMMAND_T*);
void RestoreLastSelItemTrack(COMMAND_T*);
void SaveSelItems(COMMAND_T*);
void RestoreSelItems(COMMAND_T*);
