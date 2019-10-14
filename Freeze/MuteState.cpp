/******************************************************************************
/ MuteState.cpp
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
#include "MuteState.h"

//*****************************************************
//Globals
SWSProjConfig<WDL_PtrList_DOD<MuteState> > g_muteStates;

//*****************************************************
// MuteItem class
MuteItem::MuteItem(GUID* pGuid, bool bState)
{
	m_guid = *pGuid;
	m_bState = bState;
}

MuteItem::MuteItem(LineParser* lp)
{
	stringToGuid(lp->gettoken_str(1), &m_guid);
	m_bState = lp->gettoken_int(2) ? true : false;
}

//*****************************************************
// MuteState class
MuteState::MuteState(MediaTrack* tr)
{	// Remember states of this track
	int iType;
	MediaTrack* gfd = NULL;
	int iFolder = GetFolderDepth(tr, &iType, &gfd);
	if (iType == 1)
	{
		int iChild = CSurf_TrackToID(tr, false) + 1;
		while(true)
		{
			MediaTrack* trChild = CSurf_TrackFromID(iChild++, false);
			if (!trChild)
				break;
			int iChildFolder = GetFolderDepth(trChild, &iType, &gfd);
			GUID* guid = (GUID*)GetSetMediaTrackInfo(trChild, "GUID", NULL);
			bool bMute = *(bool*)GetSetMediaTrackInfo(trChild, "B_MUTE", NULL);
			m_children.Add(new MuteItem(guid, bMute));
			if (iChildFolder + iType <= iFolder)
				break;
		}
	}

	for (int i = 0; GetSetTrackSendInfo(tr, -1, i, "P_SRCTRACK", NULL); i++)
	{
		GUID* guid = (GUID*)GetSetMediaTrackInfo((MediaTrack*)GetSetTrackSendInfo(tr, -1, i, "P_SRCTRACK", NULL), "GUID", NULL);
		bool bMute = *(bool*)GetSetTrackSendInfo(tr, -1, i, "B_MUTE", NULL);
		m_receives.Add(new MuteItem(guid, bMute));
	}
	m_guid = *(GUID*)GetSetMediaTrackInfo(tr, "GUID", NULL);
	m_bMute = *(bool*)GetSetMediaTrackInfo(tr, "B_MUTE", NULL);
	m_iSendToParent = *(bool*)GetSetMediaTrackInfo(tr, "B_MAINSEND", NULL) ? 1 : 0;
}

MuteState::MuteState(LineParser* lp)
{
	stringToGuid(lp->gettoken_str(1), &m_guid);
	m_bMute = lp->gettoken_int(2) ? true : false;
	int iSuccess;
	m_iSendToParent = lp->gettoken_int(3, &iSuccess);
	if (!iSuccess)
		m_iSendToParent = -1;
}

MuteState::~MuteState()
{
	m_children.Empty(true);
	m_receives.Empty(true);
}

void MuteState::Restore(MediaTrack* tr)
{	// Assume tr matches GUID
	GetSetMediaTrackInfo(tr, "B_MUTE", &m_bMute);
	if (m_iSendToParent != -1)
	{
		bool m_bSTP = m_iSendToParent ? true : false;
		GetSetMediaTrackInfo(tr, "B_MAINSEND", &m_bSTP);
	}
	if (m_children.GetSize())
	{
		int iType;
		MediaTrack* gfd = NULL;
		int iFolder = GetFolderDepth(tr, &iType, &gfd);
		if (iType == 1)
		{
			int iChild = CSurf_TrackToID(tr, false) + 1;
			while(true)
			{
				MediaTrack* trChild = CSurf_TrackFromID(iChild++, false);
				if (!trChild)
					break;
				int iChildFolder = GetFolderDepth(trChild, &iType, &gfd);
				for (int i = 0; i < m_children.GetSize(); i++)
					if (TrackMatchesGuid(trChild, &m_children.Get(i)->m_guid))
						GetSetMediaTrackInfo(trChild, "B_MUTE", &m_children.Get(i)->m_bState);
				if (iChildFolder + iType <= iFolder)
					break;
			}
		}
	}
	// Now match up receives
	if (m_receives.GetSize())
	{
		bool* bUsed = new bool[m_receives.GetSize()];
		memset(bUsed, 0, sizeof(bool) * m_receives.GetSize());
		int i = 0;
		while(true)
		{
			MediaTrack* trSrc = (MediaTrack*)GetSetTrackSendInfo(tr, -1, i, "P_SRCTRACK", NULL);
			if (!trSrc)
				break;
			GUID* guid = (GUID*)GetSetMediaTrackInfo(trSrc, "GUID", NULL);
			for (int j = 0; j < m_receives.GetSize(); j++)
			{
				if (!bUsed[j] && GuidsEqual(guid, &m_receives.Get(j)->m_guid))
				{
					bUsed[j] = true;
					GetSetTrackSendInfo(tr, -1, i, "B_MUTE", &m_receives.Get(j)->m_bState);
					break;
				}
			}
			i++;
		}
		delete[] bUsed;
	}
}

char* MuteState::ItemString(char* str, int maxLen, bool* bDone)
{
	static int iState = 0;
	static int iIndex = 0;
	*bDone = false;
	char guidStr[64];
	switch(iState)
	{
	case 0: // Track GUID
		guidToString(&m_guid, guidStr);
		snprintf(str, maxLen, "<MUTESTATE %s %d %d", guidStr, m_bMute, m_iSendToParent);
		iState = 1;
		break;
	case 1: // Children GUIDs/states
		if (iIndex < m_children.GetSize())
		{
			guidToString(&m_children.Get(iIndex)->m_guid, guidStr);
			snprintf(str, maxLen, "CHILD %s %d", guidStr, m_children.Get(iIndex)->m_bState);
			iIndex++;
			break;
		}
		else
		{
			iIndex = 0;
			iState = 2;
		}
	case 2: // Receive GUIDs/states
		if (iIndex < m_receives.GetSize())
		{
			guidToString(&m_receives.Get(iIndex)->m_guid, guidStr);
			snprintf(str, maxLen, "RECEIVE %s %d", guidStr, m_receives.Get(iIndex)->m_bState);
			iIndex++;
			break;
		}
		else
		{
			*bDone = true;
			iIndex = 0;
			iState = 0;
			snprintf(str, maxLen, ">");
		}
	}
	return str;
}

//*****************************************************
// Global Functions
void SaveMutes(COMMAND_T*)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			// Delete GUID out of save array if it exists
			for (int j = 0; j < g_muteStates.Get()->GetSize(); j++)
				if (TrackMatchesGuid(tr, &g_muteStates.Get()->Get(j)->m_guid))
					g_muteStates.Get()->Delete(j--, true);
			g_muteStates.Get()->Add(new MuteState(tr));
		}
	}
}

void RestoreMutes(COMMAND_T*)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			for (int j = 0; j < g_muteStates.Get()->GetSize(); j++)
				if (TrackMatchesGuid(tr, &g_muteStates.Get()->Get(j)->m_guid))
					g_muteStates.Get()->Get(j)->Restore(tr);
	}
}
