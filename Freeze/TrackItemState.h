/******************************************************************************
/ TrackItemState.h
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

class ItemState
{
public:
	ItemState(LineParser* lp);
	ItemState(MediaItem* mi);
	MediaItem* FindItem(MediaTrack* tr);
	void Restore(MediaTrack* tr, bool bSelOnly);
    char* ItemString(char* str, int maxLen);
	void Select(MediaTrack* tr);

	GUID m_guid;
	bool m_bMute;
	bool m_bSel;
	float m_fFIPMy;
	float m_fFIPMh;
	int m_iColor;
	double m_dVol;
	double m_dFadeIn;
	double m_dFadeOut;
};

class TrackState
{
public:
	TrackState(MediaTrack* tr, bool bSelOnly);
	TrackState(LineParser* lp);
	~TrackState();
	void AddSelItems(MediaTrack* tr);
	void UnselAllItems(MediaTrack* tr);
	void Restore(MediaTrack* tr, bool bSelOnly);
    char* ItemString(char* str, int maxLen);
	void SelectItems(MediaTrack* tr);

	WDL_PtrList<ItemState> m_items;
	GUID m_guid;
	bool m_bFIPM;
	int m_iColor;
};

extern SWSProjConfig<WDL_PtrList_DOD<TrackState> > g_tracks;

void SaveTrack(COMMAND_T* ct);
void RestoreTrack(COMMAND_T* ct);
void SelItemsWithState(COMMAND_T* ct);
void SaveSelOnTrack(COMMAND_T* ct);
void RestoreSelOnTrack(COMMAND_T* ct);
