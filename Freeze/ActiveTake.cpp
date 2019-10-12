/******************************************************************************
/ ActiveTake.cpp
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
#include "ActiveTake.h"

//*****************************************************
// Globals
SWSProjConfig<WDL_PtrList_DOD<ActiveTakeTrack> > g_activeTakeTracks;

//*****************************************************
// ActiveTake Class
ActiveTake::ActiveTake(MediaItem* mi)
{
	// Make sure an active take exists before calling this!
	// 	GetMediaItemNumTakes(mi) != 0
	m_item = *(GUID*)GetSetMediaItemInfo(mi, "GUID", NULL);
	m_activeTake = *(GUID*)GetSetMediaItemTakeInfo(GetMediaItemTake(mi, -1), "GUID", NULL);
}

ActiveTake::ActiveTake(LineParser* lp)
{
	stringToGuid(lp->gettoken_str(1), &m_item);
	stringToGuid(lp->gettoken_str(2), &m_activeTake);
}

bool ActiveTake::Restore(MediaItem* mi)
{
	int i;
	for (i = 0; i < GetMediaItemNumTakes(mi); i++)
		if (GuidsEqual(&m_activeTake, (GUID*)GetSetMediaItemTakeInfo(GetMediaItemTake(mi, i), "GUID", NULL)))
		{
			GetSetMediaItemInfo(mi, "I_CURTAKE", &i);
			return true;
		}

	return false;
}

char* ActiveTake::ItemString(char* str, int maxLen)
{
	char g1[64], g2[64];
	guidToString(&m_item, g1);
	guidToString(&m_activeTake, g2);
	snprintf(str, maxLen, "ITEM %s %s", g1, g2);
	return str;
}

//*****************************************************
// ActiveTakeTrack Class

ActiveTakeTrack::ActiveTakeTrack(MediaTrack* tr)
{
	m_guid = *(GUID*)GetSetMediaTrackInfo(tr, "GUID", NULL);
	for (int i = 0; i < GetTrackNumMediaItems(tr); i++)
	{
		MediaItem* mi = GetTrackMediaItem(tr, i);
		if (GetMediaItemNumTakes(mi))
			m_items.Add(new ActiveTake(mi));
	}
}

ActiveTakeTrack::ActiveTakeTrack(LineParser* lp)
{
	stringToGuid(lp->gettoken_str(1), &m_guid);
}

ActiveTakeTrack::~ActiveTakeTrack()
{
	m_items.Empty(true);
}

void ActiveTakeTrack::Restore(MediaTrack* tr)
{
	// TODO : if no matched items, return false and delete from level above???
	if (m_items.GetSize() == 0 || !GetTrackNumMediaItems(tr))
		return;

	bool* bUsed = new bool[m_items.GetSize()];
	memset(bUsed, 0, sizeof(bool) * m_items.GetSize());

	for (int i = 0; i < GetTrackNumMediaItems(tr); i++)
	{
		MediaItem* mi = GetTrackMediaItem(tr, i);
		GUID* g = (GUID*)GetSetMediaItemInfo(mi, "GUID", NULL);
		for (int j = 0; j < m_items.GetSize(); j++)
		{
			if (!bUsed[j] && GuidsEqual(g, &m_items.Get(j)->m_item))
			{
				// TODO check this logic under a bunch of situs
				if (!m_items.Get(j)->Restore(mi))
					m_items.Delete(j--, true);
				else
					bUsed[j] = true;
				break;
			}
		}
	}

	delete[] bUsed;
}

char* ActiveTakeTrack::ItemString(char* str, int maxLen)
{
	char guidStr[64];
	guidToString(&m_guid, guidStr);
	snprintf(str, maxLen, "<ACTIVETAKESTRACK %s", guidStr);
	return str;
}

//*****************************************************
// Global Functions
void SaveActiveTakes(COMMAND_T*)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			// Delete if exists
			for (int j = 0; j < g_activeTakeTracks.Get()->GetSize(); j++)
				if (TrackMatchesGuid(tr, &g_activeTakeTracks.Get()->Get(j)->m_guid))
				{
					g_activeTakeTracks.Get()->Delete(j--, true);
					break;
				}
			g_activeTakeTracks.Get()->Add(new ActiveTakeTrack(tr));
		}
	}
}

void RestoreActiveTakes(COMMAND_T*)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			// Find the saved track
			for (int j = 0; j < g_activeTakeTracks.Get()->GetSize(); j++)
				if (TrackMatchesGuid(tr, &g_activeTakeTracks.Get()->Get(j)->m_guid))
					g_activeTakeTracks.Get()->Get(j)->Restore(tr);
	}
	UpdateArrange();
}
