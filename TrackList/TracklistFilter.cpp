/******************************************************************************
/ TracklistFilter.cpp
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


#include "stdafx.h"
#include "TracklistFilter.h"

// TODO UTF8 support here
void FilteredVisState::SetFilter(const char* cFilter)
{
	if (cFilter && cFilter[0])
		m_sFilter.Set(cFilter);
	else
		m_sFilter.Set("");
	static WDL_String sLCFilter;
	sLCFilter.Set(m_sFilter.Get());
	for (int i = 0; i < sLCFilter.GetLength(); i++)
		sLCFilter.Get()[i] = tolower(sLCFilter.Get()[i]);
	m_parsedFilter->parse(sLCFilter.Get());
}

void FilteredVisState::Init(LineParser* lp)
{
	if (lp->getnumtokens() != 2 || !lp->gettoken_int(0))
		return;
	TrackVisState* tvs = new TrackVisState;
	tvs->tr = CSurf_TrackFromID(lp->gettoken_int(0), false);
	tvs->iVis = lp->gettoken_int(1);
	if (tvs->tr)
		m_filteredOut.Add(tvs);
}

char* FilteredVisState::ItemString(char* str, int maxLen, bool* bDone)
{
	static int iState = 0;
	static int iIndex = 0;
	if (!m_sFilter.Get()[0])
		return NULL;
	*bDone = false;
	if (iState == 0)
	{
		snprintf(str, maxLen, "<SWSTRACKFILTER %s", m_sFilter.Get());
		iState = 1;
		iIndex = 0;
	}
	else
	{
		if (iIndex < m_filteredOut.GetSize())
		{
			snprintf(str, maxLen, "%d %d", CSurf_TrackToID(m_filteredOut.Get(iIndex)->tr, false), m_filteredOut.Get(iIndex)->iVis);
			iIndex++;
		}
		else
		{
			*bDone = true;
			iState = 0;
			snprintf(str, maxLen, ">");
		}
	}
	return str;
}

WDL_PtrList<void>* FilteredVisState::GetFilteredTracks()
{
	static WDL_PtrList<void> tracks;
	tracks.Empty();
	
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (MatchesFilter(tr))
			tracks.Add(tr);
	}
	return &tracks;
}

bool FilteredVisState::UpdateReaper(bool bHideFiltered)
{
	bool bChanged = false;

	// Remove tracks from filteredOut that aren't in the project
	for (int i = 0; i < m_filteredOut.GetSize(); i++)
		if (CSurf_TrackToID(m_filteredOut.Get(i)->tr, false) <= 0)
			m_filteredOut.Delete(i--, true);

	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		int iVis = GetTrackVis(tr);
		int iNewVis = iVis;
		bool bShow = !bHideFiltered || MatchesFilter(tr);

		// Is this track in the filteredOut list?
		int j;
		for (j = 0; j < m_filteredOut.GetSize(); j++)
			if (m_filteredOut.Get(j)->tr == tr)
				break;
		
		if (j < m_filteredOut.GetSize())
		{
			if (bShow)
			{
				iNewVis = m_filteredOut.Get(j)->iVis;
				m_filteredOut.Delete(j, true);
			}
			else
				iNewVis = 0;
		}
		else if (!bShow)
		{
			iNewVis = 0;
			TrackVisState* tvs = m_filteredOut.Add(new TrackVisState);
			tvs->tr = tr;
			tvs->iVis = iVis;
		}

		if (iVis != iNewVis)
		{
			SetTrackVis(tr, iNewVis);
			bChanged = true;
		}
	}

	if (bChanged)
	{
		TrackList_AdjustWindows(false);
		UpdateTimeline();
	}

	return bChanged;
}

bool FilteredVisState::MatchesFilter(MediaTrack* tr)
{
	static WDL_String sTrackName;
	if (!m_parsedFilter->getnumtokens())
		return true;
	sTrackName.Set((char*)GetSetMediaTrackInfo(tr, "P_NAME", NULL));
	if (!sTrackName.GetLength())
		return false;
	for (int i = 0; i < sTrackName.GetLength(); i++)
		sTrackName.Get()[i] = tolower(sTrackName.Get()[i]);
	for (int j = 0; j < m_parsedFilter->getnumtokens(); j++)
		if (strstr(sTrackName.Get(), m_parsedFilter->gettoken_str(j)))
			return true;
	return false;
}
