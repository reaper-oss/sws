/******************************************************************************
/ TrackItemState.cpp
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
#include "TrackItemState.h"

//*****************************************************
// Globals
SWSProjConfig<WDL_PtrList_DOD<TrackState> > g_tracks;

//*****************************************************
// ItemState Class
ItemState::ItemState(LineParser* lp)
{
	if (lp->getnumtokens() < 5)
		return;

	stringToGuid(lp->gettoken_str(1), &m_guid);
	m_bMute  = lp->gettoken_int(2) ? true : false;
	m_fFIPMy = (float)lp->gettoken_float(3);
	m_fFIPMh = (float)lp->gettoken_float(4);
	m_bSel = lp->gettoken_int(5) ? true : false;
	m_iColor = lp->gettoken_int(6) ? true : false;
	int iSuccess;
	m_dVol = lp->gettoken_float(7, &iSuccess);
	if (!iSuccess)
		m_dVol = -1.0;
	m_dFadeIn = lp->gettoken_float(8, &iSuccess);
	if (!iSuccess)
		m_dFadeIn = -1.0;
	m_dFadeOut = lp->gettoken_float(9, &iSuccess);
	if (!iSuccess)
		m_dFadeOut = -1.0;
}

ItemState::ItemState(MediaItem* mi)
{
	m_guid     = *(GUID*)GetSetMediaItemInfo(mi, "GUID", NULL);
	m_bMute    = *(bool*)GetSetMediaItemInfo(mi, "B_MUTE", NULL);
	m_fFIPMy   = *(float*)GetSetMediaItemInfo(mi, "F_FREEMODE_Y", NULL);
	m_fFIPMh   = *(float*)GetSetMediaItemInfo(mi, "F_FREEMODE_H", NULL);
	m_bSel     = *(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL);
	m_iColor   = *(int*)GetSetMediaItemInfo(mi, "I_CUSTOMCOLOR", NULL);
	m_dVol     = *(double*)GetSetMediaItemInfo(mi, "D_VOL", NULL);
	m_dFadeIn  = *(double*)GetSetMediaItemInfo(mi, "D_FADEINLEN", NULL);
	m_dFadeOut = *(double*)GetSetMediaItemInfo(mi, "D_FADEOUTLEN", NULL);
}

MediaItem* ItemState::FindItem(MediaTrack* tr)
{
	// Find the media item in the track
	MediaItem* mi = NULL;
	for (int i = 0; i < GetTrackNumMediaItems(tr); i++)
	{
		mi = GetTrackMediaItem(tr, i);
		if (GuidsEqual(&m_guid, (GUID*)GetSetMediaItemInfo(mi, "GUID", NULL)))
			return mi;
	}
	return NULL;
}

void ItemState::Restore(MediaTrack* tr, bool bSelOnly)
{
	// First gotta find the media item in the track
	MediaItem* mi = FindItem(tr);
	if (mi == NULL)
		return;
	if (!bSelOnly || *(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL))
	{
		GetSetMediaItemInfo(mi, "B_MUTE", &m_bMute);
		GetSetMediaItemInfo(mi, "F_FREEMODE_Y", &m_fFIPMy);
		GetSetMediaItemInfo(mi, "F_FREEMODE_H", &m_fFIPMh);
		if (!bSelOnly)
			GetSetMediaItemInfo(mi, "B_UISEL", &m_bSel);
		GetSetMediaItemInfo(mi, "I_CUSTOMCOLOR", &m_iColor);
		if (m_dVol >= 0.0)
			GetSetMediaItemInfo(mi, "D_VOL", &m_dVol);
		if (m_dFadeIn >= 0.0)
			GetSetMediaItemInfo(mi, "D_FADEINLEN", &m_dFadeIn);
		if (m_dFadeOut >= 0.0)
			GetSetMediaItemInfo(mi, "D_FADEOUTLEN", &m_dFadeOut);
	}
}

char* ItemState::ItemString(char* str, int maxLen)
{
	char guidStr[64];
	guidToString(&m_guid, guidStr);
	snprintf(str, maxLen, "ITEMSTATE %s %d %.5f %.5f %d %d %.14f %.6f %.6f", guidStr, m_bMute ? 1 : 0, m_fFIPMy, m_fFIPMh, m_bSel ? 1 : 0, m_iColor, m_dVol, m_dFadeIn, m_dFadeOut);
	return str;
}

void ItemState::Select(MediaTrack* tr)
{
	MediaItem* mi = FindItem(tr);
	if (mi == NULL)
		return;
	GetSetMediaItemInfo(mi, "B_UISEL", &g_bTrue);
}

//*****************************************************
// TrackState Class
TrackState::TrackState(MediaTrack* tr, bool bSelOnly)
{	// Remember item states on this track
	m_guid   = *(GUID*)GetSetMediaTrackInfo(tr, "GUID", NULL);
	m_bFIPM  = *(bool*)GetSetMediaTrackInfo(tr, "B_FREEMODE", NULL);
	m_iColor = *(int*)GetSetMediaTrackInfo(tr, "I_CUSTOMCOLOR", NULL);
	for (int i = 0; i < GetTrackNumMediaItems(tr); i++)
	{
		MediaItem* mi = GetTrackMediaItem(tr, i);
		if (!bSelOnly || *(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL))
			m_items.Add(new ItemState(mi));
	}
}

TrackState::TrackState(LineParser* lp)
{
	if (lp->getnumtokens() < 2)
		return;

	stringToGuid(lp->gettoken_str(1), &m_guid);
	int success;
	m_bFIPM = lp->gettoken_int(2, &success) ? true : false;
	if (!success)
		m_bFIPM = true;
	m_iColor = lp->gettoken_int(3);
}

TrackState::~TrackState()
{
	m_items.Empty(true);
}

void TrackState::AddSelItems(MediaTrack* tr)
{
	for (int i = 0; i < GetTrackNumMediaItems(tr); i++)
	{
		MediaItem* mi = GetTrackMediaItem(tr, i);
		if (*(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL))
		{
			int j;
			for (j = 0; j < m_items.GetSize(); j++)
				if (mi == m_items.Get(j)->FindItem(tr))
				{
					ItemState* is = m_items.Get(j);
					m_items.Set(j, new ItemState(mi));
					delete is;
					break;
				}
			if (j == m_items.GetSize())
				m_items.Add(new ItemState(mi));
		}
	}
}

void TrackState::UnselAllItems(MediaTrack* tr)
{
	for (int i = 0; i < GetTrackNumMediaItems(tr); i++)
		GetSetMediaItemInfo(GetTrackMediaItem(tr, i), "B_UISEL", &g_bFalse);
}

void TrackState::Restore(MediaTrack* tr, bool bSelOnly)
{
	// The level above Restore already knows the MediaTrack* so
	// pass it in, even though we can get it ourselves from the
	// GUID
	if (!bSelOnly)
	{
		UnselAllItems(tr);
		GetSetMediaTrackInfo(tr, "B_FREEMODE", &m_bFIPM);
		GetSetMediaTrackInfo(tr, "I_CUSTOMCOLOR", &m_iColor);
	}
	for (int i = 0; i < m_items.GetSize(); i++)
		m_items.Get(i)->Restore(tr, bSelOnly);
}

char* TrackState::ItemString(char* str, int maxLen)
{
	char guidStr[64];
	guidToString(&m_guid, guidStr);
	snprintf(str, maxLen, "<TRACKSTATE %s %d %d", guidStr, m_bFIPM, m_iColor);
	return str;
}

void TrackState::SelectItems(MediaTrack* tr)
{
	UnselAllItems(tr);
	for (int i = 0; i < m_items.GetSize(); i++)
		m_items.Get(i)->Select(tr);
}

//*****************************************************
// Global Functions
void SaveTrack(COMMAND_T*)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			// First see if this track is saved already
			int j;
			for (j = 0; j < g_tracks.Get()->GetSize(); j++)
			{
				if (TrackMatchesGuid(tr, &g_tracks.Get()->Get(j)->m_guid))
				{
					TrackState* ts = g_tracks.Get()->Get(j);
					g_tracks.Get()->Set(j, new TrackState(tr, false));
					delete ts;
					break;
				}
			}
			if (j == g_tracks.Get()->GetSize())
				g_tracks.Get()->Add(new TrackState(tr, false));
		}
	}
}

void RestoreTrack(COMMAND_T* _ct)
{
	PreventUIRefresh(1);
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			// Find the saved track
			for (int j = 0; j < g_tracks.Get()->GetSize(); j++)
				if (TrackMatchesGuid(tr, &g_tracks.Get()->Get(j)->m_guid))
					g_tracks.Get()->Get(j)->Restore(tr, false);
	}
	PreventUIRefresh(-1);
	UpdateTimeline();
	Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}

void SelItemsWithState(COMMAND_T* _ct)
{
	PreventUIRefresh(1);
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			// Find the saved track
			for (int j = 0; j < g_tracks.Get()->GetSize(); j++)
				if (TrackMatchesGuid(tr, &g_tracks.Get()->Get(j)->m_guid))
					g_tracks.Get()->Get(j)->SelectItems(tr);
	}
	PreventUIRefresh(-1);
	UpdateArrange();
	Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}

void SaveSelOnTrack(COMMAND_T*)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			// First see if this track is saved already
			int j;
			for (j = 0; j < g_tracks.Get()->GetSize(); j++)
				if (TrackMatchesGuid(tr, &g_tracks.Get()->Get(j)->m_guid))
				{
					g_tracks.Get()->Get(j)->AddSelItems(tr);
					break;
				}
			if (j == g_tracks.Get()->GetSize())
				g_tracks.Get()->Add(new TrackState(tr, true));
		}
	}
}

void RestoreSelOnTrack(COMMAND_T* _ct)
{
	PreventUIRefresh(1);
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			// Find the saved track
			for (int j = 0; j < g_tracks.Get()->GetSize(); j++)
				if (TrackMatchesGuid(tr, &g_tracks.Get()->Get(j)->m_guid))
					g_tracks.Get()->Get(j)->Restore(tr, true);
	}
	PreventUIRefresh(-1);
	UpdateTimeline();
	Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}
