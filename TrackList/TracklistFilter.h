/******************************************************************************
/ TracklistFilter.h
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

typedef struct TrackVisState
{
	MediaTrack* tr;
	int iVis;
} TrackVisState;

class FilteredVisState
{
public:
	FilteredVisState() { m_parsedFilter = new LineParser(false); }
	~FilteredVisState() { m_filteredOut.Empty(true); delete m_parsedFilter; }
	void SetFilter(const char* cFilter);
	const char* GetFilter() { return m_sFilter.Get(); }
	void Init(LineParser* lp);
	char* ItemString(char* str, int maxLen, bool* bDone);
	WDL_PtrList<void>* GetFilteredTracks();
	bool UpdateReaper(bool bHideFiltered);

private:
	bool MatchesFilter(MediaTrack* tr);
	WDL_String m_sFilter;
	LineParser* m_parsedFilter;
	WDL_PtrList<TrackVisState> m_filteredOut;
};
