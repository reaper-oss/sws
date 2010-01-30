/******************************************************************************
/ TrackSends.cpp
/
/ Copyright (c) 2010 Tim Payne (SWS)
/ http://www.standingwaterstudios.com/reaper
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
#include "TrackSends.h"

// Functions for getting/setting track sends

TrackSend::TrackSend(GUID* guid, const char* str)
{
	m_destGuid = *guid;
	// Strip out the "AUXRECV X" header
	const char* p = strchr(str + 8, ' ') + 1;
	m_str.Set(p);
}

// For backward compat only, or perhaps for creating new sends?
TrackSend::TrackSend(GUID* guid, int iMode, double dVol, double dPan, int iMute, int iMono, int iPhase, int iSrc, int iDest, int iMidi, int iAuto)
{
	m_destGuid = *guid;
	m_str.SetFormatted(200, "%d %.14f %.14f %d %d %d %d %d -1.0 %d %d\n",
		iMode, dVol, dPan, iMute, iMono, iPhase, iSrc, iDest, iMidi, iAuto);
}

// Set the AUXRECV string using the second parameter in the string as the destination track GUID
TrackSend::TrackSend(const char* str)
{
	LineParser lp(false);
	lp.parse(str);
	stringToGuid(lp.gettoken_str(1), &m_destGuid);
	// Strip out the "AUXRECV X" header
	const char* p = strchr(str + 8, ' ') + 1;
	m_str.Set(p);
}

TrackSend::TrackSend(TrackSend& ts)
{
	m_destGuid = ts.m_destGuid;
	m_str.Set(ts.m_str.Get());
}

WDL_String* TrackSend::AuxRecvString(MediaTrack* srcTr, WDL_String* str)
{
	int iSrc = CSurf_TrackToID(srcTr, false) - 1;
	str->SetFormatted(20, "AUXRECV %d ", iSrc);
	str->Append(m_str.Get());
	return str;
}

void TrackSend::GetChunk(WDL_String* chunk)
{
	char guidStr[64];
	guidToString(&m_destGuid, guidStr);
	chunk->AppendFormatted(chunk->GetLength() + 150, "AUXSEND %s %s\n", guidStr, m_str.Get());
}

TrackSends::TrackSends(TrackSends& ts)
{
	for (int i = 0; i < ts.m_hwSends.GetSize(); i++)
		m_hwSends.Add(new WDL_String(ts.m_hwSends.Get(i)->Get()));
	for (int i = 0; i < ts.m_sends.GetSize(); i++)
		m_sends.Add(new TrackSend(*ts.m_sends.Get(i)));
}

TrackSends::~TrackSends()
{
	m_hwSends.Empty(true);
	m_sends.Empty(true);
}

void TrackSends::Build(MediaTrack* tr)
{
	// Get the HW sends from the track object string
	char* trackStr = SWS_GetSetObjectState(tr, NULL);
	WDL_String line;
	int pos = 0;
	while (GetChunkLine(trackStr, &line, &pos, false))
	{
		if (strncmp(line.Get(), "HWOUT", 5) == 0)
			m_hwSends.Add(new WDL_String(line));
	}
	SWS_FreeHeapPtr(trackStr);

	// Get the track sends from the cooresponding track object string
	MediaTrack* pDest;
	int idx = 0;
	char searchStr[20];
	sprintf(searchStr, "AUXRECV %d", CSurf_TrackToID(tr, false) - 1);
	while ((pDest = (MediaTrack*)GetSetTrackSendInfo(tr, 0, idx++, "P_DESTTRACK", NULL)))
	{
		GUID guid = *(GUID*)GetSetMediaTrackInfo(pDest, "GUID", NULL);
		// Have we already parsed this particular dest track string?
		bool bParsed = false;
		for (int i = 0; i < m_sends.GetSize(); i++)
			if (memcmp(m_sends.Get(i)->GetGuid(), &guid, sizeof(GUID)) == 0)
			{
				bParsed = true;
				break;
			}
		if (bParsed)
			break;

		// We haven't parsed yet!
		trackStr = SWS_GetSetObjectState(pDest, NULL);
		pos = 0;
		while (GetChunkLine(trackStr, &line, &pos, false))
		{
			if (strncmp(line.Get(), searchStr, strlen(searchStr)) == 0)
				m_sends.Add(new TrackSend(&guid, line.Get()));
		}
		SWS_FreeHeapPtr(trackStr);
	}
}

void TrackSends::UpdateReaper(MediaTrack* tr)
{
	// First replace all the hw sends with the stored
	char* trackStr = SWS_GetSetObjectState(tr, NULL);
	WDL_String newTrackStr;
	WDL_String line;
	int pos = 0;
	bool bChanged = false;
	while (GetChunkLine(trackStr, &line, &pos, true))
	{
		if (strncmp(line.Get(), "HWOUT", 5) != 0)
			newTrackStr.Append(line.Get());
		else
			bChanged = true;
	}
	for (int i = 0; i < m_hwSends.GetSize(); i++)
	{
		bChanged = true;
		AppendChunkLine(&newTrackStr, m_hwSends.Get(i)->Get());
	}

	SWS_FreeHeapPtr(trackStr);
	if (bChanged)
		SWS_GetSetObjectState(tr, newTrackStr.Get());

	// Now, delete any existing sends and add as necessary
	// Loop through each track
	char searchStr[20];
	sprintf(searchStr, "AUXRECV %d", CSurf_TrackToID(tr, false) - 1);
	WDL_String sendStr;
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* pDest = CSurf_TrackFromID(i, false);
		newTrackStr.Set("");
		trackStr = NULL;
		MediaTrack* pSrc;
		int idx = 0;
		while ((pSrc = (MediaTrack*)GetSetTrackSendInfo(pDest, -1, idx++, "P_SRCTRACK", NULL)))
			if (pSrc == tr)
			{
				trackStr = SWS_GetSetObjectState(pDest, NULL);
				pos = 0;
				// Remove existing recvs from the src track
				while (GetChunkLine(trackStr, &line, &pos, true))
				{
					if (strncmp(line.Get(), searchStr, strlen(searchStr)) != 0)
						newTrackStr.Append(line.Get());
				}
				break;
			}

		GUID guid = *(GUID*)GetSetMediaTrackInfo(pDest, "GUID", NULL);
		for (int i = 0; i < m_sends.GetSize(); i++)
		{
			if (memcmp(&guid, m_sends.Get(i)->GetGuid(), sizeof(GUID)) == 0)
			{
				if (!trackStr)
				{
					trackStr = SWS_GetSetObjectState(pDest, NULL);
					newTrackStr.Set(trackStr);
				}
				AppendChunkLine(&newTrackStr, m_sends.Get(i)->AuxRecvString(tr, &sendStr)->Get());
			}
		}

		if (trackStr)
		{
			SWS_GetSetObjectState(pDest, newTrackStr.Get());
			SWS_FreeHeapPtr(trackStr);
		}
	}
}

void TrackSends::GetChunk(WDL_String* chunk)
{
	for (int i = 0; i < m_hwSends.GetSize(); i++)
	{
		chunk->Append(m_hwSends.Get(i)->Get());
		chunk->Append("\n");
	}
	for (int i = 0; i < m_sends.GetSize(); i++)
		m_sends.Get(i)->GetChunk(chunk);
}
