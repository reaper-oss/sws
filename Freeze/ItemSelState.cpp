/******************************************************************************
/ ItemSelState.cpp
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

#include "../Utility/Base64.h"
#include "ItemSelState.h"

#include <WDL/localize/localize.h>

//*****************************************************
//Globals

SWSProjConfig<WDL_PtrList_DOD<SelItemsTrack> > g_selItemsTrack;
SWSProjConfig<SelItems> g_selItems;

//*****************************************************
// Local defines

#define GUIDS_PER_LINE 4

//*****************************************************
// SelItems class

void SelItems::Save(MediaTrack* tr)
{
	m_selItems.Empty(true);
	if (tr == NULL)
		for (int i = 1; i <= GetNumTracks(); i++)
			Add(CSurf_TrackFromID(i, false));
	else
		Add(tr);
}

void SelItems::Add(LineParser* lp)
{
	Base64 b64;
	int bufSize;
	GUID* guidBuf = (GUID*)b64.Decode(lp->gettoken_str(0), &bufSize);
	for (unsigned int i = 0; i < bufSize / sizeof(GUID); i++)
	{
		GUID* g = new GUID;
		*g = guidBuf[i];
		m_selItems.Add(g);
	}
}

void SelItems::Add(MediaTrack* tr)
{
	for (int i = 0; i < GetTrackNumMediaItems(tr); i++)
	{
		MediaItem* mi = GetTrackMediaItem(tr, i);
		if (*(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL))
		{
			GUID* g = new GUID;
			*g = *(GUID*)GetSetMediaItemInfo(mi, "GUID", NULL);
			m_selItems.Add(g);
		}
	}
}

// bUsed is an array of bools with size == m_selItems.GetSize(), used to "check off" when an item from that list is used
// caller initializes bUsed
void SelItems::Match(MediaTrack* tr, bool* bUsed)
{
	PreventUIRefresh(1);
	int nbitems=GetTrackNumMediaItems(tr);
	for (int i = 0; i < nbitems; i++)
	{
		MediaItem* mi = GetTrackMediaItem(tr, i);
		GetSetMediaItemInfo(mi, "B_UISEL", &g_bFalse);
		GUID* g = (GUID*)GetSetMediaItemInfo(mi, "GUID", NULL);
		for (int j = 0; j < m_selItems.GetSize(); j++)
			if (!bUsed[j] && GuidsEqual(m_selItems.Get(j), g))
			{
				bUsed[j] = true;
				GetSetMediaItemInfo(mi, "B_UISEL", &g_bTrue);
				break;
			}
	}
	PreventUIRefresh(-1);
}

void SelItems::Restore(MediaTrack* tr)
{
	bool* bUsed = NULL;
	if (m_selItems.GetSize())
	{
		bUsed = new bool[m_selItems.GetSize()];
		memset(bUsed, 0, sizeof(bool) * m_selItems.GetSize());
	}

	if (tr == NULL)
	{
		PreventUIRefresh(1);
		for (int i = 1; i <= GetNumTracks(); i++)
			Match(CSurf_TrackFromID(i, false), bUsed);
		PreventUIRefresh(-1);
	}
	else
		Match(tr, bUsed);

	// Delete unused items
	for (int i = m_selItems.GetSize()-1; i >= 0 ; i--)
		if (!bUsed[i])
			m_selItems.Delete(i, true);

	delete[] bUsed;
}

char* SelItems::ItemString(char* str, int maxLen, bool* bDone)
{
	static int iLine = 0;
	*bDone = false;
	int iGUIDs = m_selItems.GetSize() - iLine*GUIDS_PER_LINE;
	if (iGUIDs <= 0)
	{
		*bDone = true;
		iLine = 0;
		lstrcpyn(str, ">", maxLen);
	}
	else
	{
		Base64 b64;
		if (iGUIDs > GUIDS_PER_LINE)
			iGUIDs = GUIDS_PER_LINE;
		char pGUIDs[GUIDS_PER_LINE*sizeof(GUID)];
		for (int i = 0; i < iGUIDs; i++)
			memcpy(pGUIDs+i*sizeof(GUID), m_selItems.Get(iLine*GUIDS_PER_LINE+i), sizeof(GUID));
		lstrcpyn(str, b64.Encode(pGUIDs, sizeof(GUID)*iGUIDs), maxLen);
		iLine++;
	}
	return str;
}

//*****************************************************
// SelItemsTrack class

SelItemsTrack::SelItemsTrack(MediaTrack* tr)
{
	m_guid = *(GUID*)GetSetMediaTrackInfo(tr, "GUID", NULL);
	memset(m_selItems, 0, sizeof(m_selItems));
	m_lastSel = NULL;
}

SelItemsTrack::SelItemsTrack(LineParser* lp)
{
	stringToGuid(lp->gettoken_str(1), &m_guid);
	memset(m_selItems, 0, sizeof(m_selItems));
	m_lastSel = NULL;
}

SelItemsTrack::~SelItemsTrack()
{
	for (int i = 0; i < SEL_SLOTS; i++)
		delete m_selItems[i];
	delete m_lastSel;
}

void SelItemsTrack::Add(LineParser* lp, int iSlot)
{
	if (!m_selItems[iSlot])
		m_selItems[iSlot] = new SelItems();
	m_selItems[iSlot]->Add(lp);
}

void SelItemsTrack::SaveTemp(MediaTrack* tr)
{
	if (m_lastSel)
		delete m_lastSel;
	m_lastSel = new SelItems();
	m_lastSel->Save(tr);
}

void SelItemsTrack::Save(MediaTrack* tr, int iSlot)
{
	if (m_selItems[iSlot])
		delete m_selItems[iSlot];
	m_selItems[iSlot] = new SelItems();
	m_selItems[iSlot]->Save(tr);
	SaveTemp(tr);
}

void SelItemsTrack::Restore(MediaTrack* tr, int iSlot)
{
	SaveTemp(tr);
	if (m_selItems[iSlot])
		m_selItems[iSlot]->Restore(tr);
}

void SelItemsTrack::PreviousSelection(MediaTrack* tr)
{
	SelItems* curSel = new SelItems();
	curSel->Save(tr);
	if (m_lastSel)
	{
		m_lastSel->Restore(tr);
		delete m_lastSel;
	}
	m_lastSel = curSel;
}

char* SelItemsTrack::ItemString(char* str, int maxLen, bool* bDone)
{
	static int iState = 0;
	static int iSlot = 0;
	*bDone = false;
	bool bSubDone;
	char guidStr[64];
	switch(iState)
	{
	case 0: // Track GUID
		guidToString(&m_guid, guidStr);
		snprintf(str, maxLen, "<SELTRACKITEMSELSTATE %s", guidStr);
		iState = 1;
		break;
	case 1: // Slot text
		while (iSlot < SEL_SLOTS && !m_selItems[iSlot])
			iSlot++;
		if (iSlot == SEL_SLOTS)
		{
			*bDone = true;
			iSlot  = 0;
			iState = 0;
			snprintf(str, maxLen, ">");
		}
		else
		{
			snprintf(str, maxLen, "<SLOT %d", iSlot+1);
			iState = 2;
		}
		break;
	case 2: // Write out GUIDs
		m_selItems[iSlot]->ItemString(str, maxLen, &bSubDone);
		if (bSubDone)
		{
			iSlot++;
			iState = 1;
		}
		break;
	}
	return str;
}

//*****************************************************
// Global functions
void SaveSelTrackSelItems(int iSlot)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			GUID* g = (GUID*)GetSetMediaTrackInfo(tr, "GUID", NULL);
			int j;
			for (j = 0; j < g_selItemsTrack.Get()->GetSize(); j++)
				if (GuidsEqual(g, &g_selItemsTrack.Get()->Get(j)->m_guid))
				{
					g_selItemsTrack.Get()->Get(j)->Save(tr, iSlot);
					break;
				}
			if (j == g_selItemsTrack.Get()->GetSize())
			{
				g_selItemsTrack.Get()->Add(new SelItemsTrack(tr));
				g_selItemsTrack.Get()->Get(g_selItemsTrack.Get()->GetSize()-1)->Save(tr, iSlot);
			}
		}
	}
}

void RestoreSelTrackSelItems(int iSlot)
{
	PreventUIRefresh(1);
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			GUID* g = (GUID*)GetSetMediaTrackInfo(tr, "GUID", NULL);
			for (int j = 0; j < g_selItemsTrack.Get()->GetSize(); j++)
				if (GuidsEqual(g, &g_selItemsTrack.Get()->Get(j)->m_guid))
				{
					g_selItemsTrack.Get()->Get(j)->Restore(tr, iSlot);
					break;
				}
		}
	}
	PreventUIRefresh(-1);

	char cUndoText[256];
	snprintf(cUndoText, sizeof(cUndoText), __LOCALIZE_VERFMT("Restore selected track(s) selected item(s), slot %d","sws_undo"), iSlot+1);
	Undo_OnStateChangeEx(cUndoText, UNDO_STATE_ITEMS, -1);
	UpdateArrange();
}

void RestoreLastSelItemTrack(COMMAND_T* ct)
{
	PreventUIRefresh(1);
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			GUID* g = (GUID*)GetSetMediaTrackInfo(tr, "GUID", NULL);
			for (int j = 0; j < g_selItemsTrack.Get()->GetSize(); j++)
				if (GuidsEqual(g, &g_selItemsTrack.Get()->Get(j)->m_guid))
				{
					g_selItemsTrack.Get()->Get(j)->PreviousSelection(tr);
					break;
				}
		}
	}
	PreventUIRefresh(-1);
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct), UNDO_STATE_ITEMS, -1);
	UpdateArrange();
}

void SaveSelItems(COMMAND_T* ct)
{
	g_selItems.Get()->Save(NULL);
}

void RestoreSelItems(COMMAND_T* ct)
{
	g_selItems.Get()->Restore(NULL);
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct), UNDO_STATE_ITEMS, -1);
	UpdateTimeline();
}

void SaveSelTrackSelItems(COMMAND_T* ct) { SaveSelTrackSelItems((int)ct->user); }
void RestoreSelTrackSelItems(COMMAND_T* ct) { RestoreSelTrackSelItems((int)ct->user); }
