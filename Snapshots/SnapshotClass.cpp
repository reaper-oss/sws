/******************************************************************************
/ SnapshotClass.cpp
/
/ Copyright (c) 2009 Tim Payne (SWS)
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
#include "../Utility/Base64.h"
#include "SnapshotClass.h"

FXSnapshot::FXSnapshot(MediaTrack* tr, int fx)
{
	m_iCurParam = 0;
	TrackFX_GetFXName(tr, fx, m_cName, 256);
	m_iNumParams = TrackFX_GetNumParams(tr, fx);
	if (m_iNumParams)
		m_dParams = new double[m_iNumParams];
	else
		m_dParams = NULL;
	double d1, d2;
	for (int i = 0; i < m_iNumParams; i++)
		m_dParams[i] = TrackFX_GetParam(tr, fx, i, &d1, &d2);
}

FXSnapshot::FXSnapshot(LineParser* lp)
{
	m_iCurParam = 0;
	strncpy(m_cName, lp->gettoken_str(1), 256);
	m_cName[255] = 0;
	if (strcmp("<FX", lp->gettoken_str(0)) == 0)
		m_iNumParams = lp->gettoken_int(2);
	else
		m_iNumParams = lp->getnumtokens() - 2;

	if (m_iNumParams)
		m_dParams = new double[m_iNumParams];
	else
		m_dParams = NULL;

	if (strcmp("<FX", lp->gettoken_str(0))) // Old method!
		for (int i = 0; i < m_iNumParams; i++)
			m_dParams[i] = lp->gettoken_float(i+2);
}

FXSnapshot::~FXSnapshot()
{
	delete [] m_dParams;
}

char* FXSnapshot::ItemString(char* str, int maxLen, bool* bDone)
{
	static int iLine = 0;
	*bDone = false;
	if (iLine == 0)
	{
		sprintf(str, "<FX \"%s\" %d", m_cName, m_iNumParams);
	}
	else
	{
		int iDoublesLeft = m_iNumParams - (iLine-1)*DOUBLES_PER_LINE;
		if (iDoublesLeft < 0)
			iDoublesLeft = 0;
		Base64 b64;

		if (iDoublesLeft >= DOUBLES_PER_LINE)
			// Full line
			sprintf(str, b64.Encode((char*)(&m_dParams[(iLine-1)*DOUBLES_PER_LINE]), DOUBLES_PER_LINE * sizeof(double)));
		else if (iDoublesLeft)
			// Less than full line
			sprintf(str, b64.Encode((char*)(&m_dParams[(iLine-1)*DOUBLES_PER_LINE]), iDoublesLeft * sizeof(double)));
		else
		{
			*bDone = true;
			iLine = 0;
			return ">";
		}
	}

	iLine++;
	return str;
}

void FXSnapshot::RestoreParams(char* str)
{
	Base64 b64;
	int iLen;
	double* newDoubles = (double*)b64.Decode(str, &iLen);
	iLen /= sizeof(double);
	// Do nothing if there's an issue
	if (iLen == 0 || iLen > m_iNumParams - m_iCurParam)
		return;

	for (int i = 0; i < iLen; i++)
		m_dParams[m_iCurParam++] = newDoubles[i];
}

int FXSnapshot::UpdateReaper(MediaTrack* tr, bool* bMatched, int num)
{
	// Match the name and count of the FX
	int fx;
	char name[256];
	for (fx = 0; fx < num; fx++)
	{
		TrackFX_GetFXName(tr, fx, name, 256);
		if (!bMatched[fx] && strcmp(m_cName, name) == 0 && m_iNumParams == TrackFX_GetNumParams(tr, fx))
			break;
	}

	if (fx >= num)
		return -1;

	for (int i = 0; i < m_iNumParams; i++)
		TrackFX_SetParam(tr, fx, i, m_dParams[i]);

	return fx;
}

bool FXSnapshot::Exists(MediaTrack* tr)
{
	// Match the name and count of the FX
	int fx;
	char name[256];
	int num = TrackFX_GetCount(tr);
	for (fx = 0; fx < num; fx++)
	{
		TrackFX_GetFXName(tr, fx, name, 256);
		if (strcmp(m_cName, name) == 0 && m_iNumParams == TrackFX_GetNumParams(tr, fx))
			break;
	}

	return fx < num;
}

SendSnapshot::SendSnapshot(MediaTrack* tr, int send)
{
	MediaTrack* sendTr;
	sendTr = (MediaTrack*)GetSetTrackSendInfo(tr, 0, send, "P_DESTTRACK", NULL);
	m_dest = *(GUID*)GetSetMediaTrackInfo(sendTr, "GUID", NULL); 
	m_dVol = *(double*)GetSetTrackSendInfo(tr, 0, send, "D_VOL", NULL);
	m_dPan = *(double*)GetSetTrackSendInfo(tr, 0, send, "D_PAN", NULL);
	m_bMute = *(bool*)GetSetTrackSendInfo(tr, 0, send, "B_MUTE", NULL);
	m_iMode = *(int*)GetSetTrackSendInfo(tr, 0, send, "I_SENDMODE", NULL);
	m_iSrcChan = *(int*)GetSetTrackSendInfo(tr, 0, send, "I_SRCCHAN", NULL);
	m_iDstChan = *(int*)GetSetTrackSendInfo(tr, 0, send, "I_DSTCHAN", NULL);
	m_iMidiFlags = *(int*)GetSetTrackSendInfo(tr, 0, send, "I_MIDIFLAGS", NULL);
}

SendSnapshot::SendSnapshot(LineParser* lp)
{
	if (lp->getnumtokens() != 9)
		return;

	stringToGuid(lp->gettoken_str(1), &m_dest);
	m_dVol =  lp->gettoken_float(2);
	m_dPan =  lp->gettoken_float(3);
	m_bMute = lp->gettoken_int(4) ? true : false;
	m_iMode = lp->gettoken_int(5);
	m_iSrcChan = lp->gettoken_int(6);
	m_iDstChan = lp->gettoken_int(7);
	m_iMidiFlags = lp->gettoken_int(8);
}

SendSnapshot::~SendSnapshot()
{
}

char* SendSnapshot::ItemString(char* str, int maxLen)
{
	char guidStr[64];
	guidToString(&m_dest, guidStr);
	_snprintf(str, maxLen, "SEND %s %.14f %.14f %d %d %d %d %d", guidStr, m_dVol, m_dPan, m_bMute ? 1 : 0, m_iMode, m_iSrcChan, m_iDstChan, m_iMidiFlags);
	return str;
}

int SendSnapshot::UpdateReaper(MediaTrack* tr, bool* bMatched, int num)
{
	// Loop through all sends to see if the GUIDs match
	int send;
	for (send = 0; send < num; send++)
		if (!bMatched[send] && memcmp(&m_dest, GetSetMediaTrackInfo((MediaTrack*)GetSetTrackSendInfo(tr, 0, send, "P_DESTTRACK", NULL), "GUID", NULL), sizeof(GUID)) == 0)
			break;

	if (send >= num)
		return -1;

	GetSetTrackSendInfo(tr, 0, send, "D_VOL", &m_dVol);
	GetSetTrackSendInfo(tr, 0, send, "D_PAN", &m_dPan);
	GetSetTrackSendInfo(tr, 0, send, "B_MUTE", &m_bMute);
	GetSetTrackSendInfo(tr, 0, send, "I_SENDMODE", &m_iMode);
	GetSetTrackSendInfo(tr, 0, send, "I_SRCCHAN", &m_iSrcChan);
	GetSetTrackSendInfo(tr, 0, send, "I_DSTCHAN", &m_iDstChan);
	GetSetTrackSendInfo(tr, 0, send, "I_MIDIFLAGS", &m_iMidiFlags);

	return send;
}

bool SendSnapshot::Exists(MediaTrack* tr)
{
	int send, numSends;
	for (numSends = 0; GetSetTrackSendInfo(tr, 0, numSends, "P_SRCTRACK", NULL); numSends++);
	for (send = 0; send < numSends; send++)
		if (memcmp(&m_dest, GetSetMediaTrackInfo((MediaTrack*)GetSetTrackSendInfo(tr, 0, send, "P_DESTTRACK", NULL), "GUID", NULL), sizeof(GUID)) == 0)
			break;

	return (send < numSends);
}

TrackSnapshot::TrackSnapshot(MediaTrack* tr, int mask)
{
	if (CSurf_TrackToID(tr, false) == 0)
		m_guid = GUID_NULL;
	else
		m_guid = *(GUID*)GetSetMediaTrackInfo(tr, "GUID", NULL);

	m_dVol = *((double*)GetSetMediaTrackInfo(tr, "D_VOL", NULL));
	m_dPan = *((double*)GetSetMediaTrackInfo(tr, "D_PAN", NULL));
	m_bMute = *((bool*)GetSetMediaTrackInfo(tr, "B_MUTE", NULL));
	m_iSolo = *((int*)GetSetMediaTrackInfo(tr, "I_SOLO", NULL));
	m_iFXEn = *((int*)GetSetMediaTrackInfo(tr, "I_FXEN", NULL));
	m_iVis  = GetTrackVis(tr);
	m_iSel  = *((int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL));

	// Don't bother storing the sends if it's masked
	if (mask & SENDS_MASK)
		for (int i = 0; GetSetTrackSendInfo(tr, 0, i, "P_SRCTRACK", NULL); i++)
			m_sends.Add(new SendSnapshot(tr, i));

	// Same for the fx
	if (mask & FXATM_MASK)
		for (int i = 0; i < TrackFX_GetCount(tr); i++)
			m_fx.Add(new FXSnapshot(tr, i));
}

TrackSnapshot::TrackSnapshot(LineParser* lp)
{
	stringToGuid(lp->gettoken_str(1), &m_guid);
	m_dVol =  lp->gettoken_float(2);
	m_dPan =  lp->gettoken_float(3);
	m_bMute = lp->gettoken_int(4) ? true : false;
	m_iSolo = lp->gettoken_int(5);
	m_iFXEn = lp->gettoken_int(6);
	// For backward compat, flip the TCP vis bit
	m_iVis = lp->gettoken_int(7) ^ 2;
	m_iSel = lp->gettoken_int(8);
}

TrackSnapshot::~TrackSnapshot()
{
	m_sends.Empty(true);
	m_fx.Empty(true);
}

char* TrackSnapshot::ItemString(char* str, int maxLen)
{
	char guidStr[64];
	guidToString(&m_guid, guidStr);
	_snprintf(str, maxLen, "TRACK %s %.14f %.14f %d %d %d %d %d", guidStr, m_dVol, m_dPan, m_bMute ? 1 : 0, m_iSolo, m_iFXEn, m_iVis ^ 2, m_iSel);
	return str;
}

// Returns true if cannot find the track to update!
bool TrackSnapshot::UpdateReaper(int mask, int* sendErr, int* fxErr, bool bSelOnly)
{
	MediaTrack* tr = GetTrack();
	if (!tr)
		return true;

	int iSel = *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL);
	if (bSelOnly && !iSel)
		return false; // Ignore if the track isn't selected

	if (mask & VOL_MASK)
		GetSetMediaTrackInfo(tr, "D_VOL", &m_dVol);
	if (mask & PAN_MASK)
		GetSetMediaTrackInfo(tr, "D_PAN", &m_dPan);
	if (mask & MUTE_MASK)
		GetSetMediaTrackInfo(tr, "B_MUTE", &m_bMute);
	if (mask & SOLO_MASK)
		GetSetMediaTrackInfo(tr, "I_SOLO", &m_iSolo);
	if (mask & VIS_MASK)
		SetTrackVis(tr, m_iVis); // ignores master
	if (mask & SEL_MASK)
		GetSetMediaTrackInfo(tr, "I_SELECTED", &m_iSel);
	if (mask & FXATM_MASK)
	{
		GetSetMediaTrackInfo(tr, "I_FXEN", &m_iFXEn);
		int numFX = TrackFX_GetCount(tr);
		if (numFX)
		{
			bool* bMatched = new bool[numFX];
			memset(bMatched, 0, sizeof(bool) * numFX);
			for (int i = 0; i < m_fx.GetSize(); i++)
			{
				int match = m_fx.Get(i)->UpdateReaper(tr, bMatched, numFX);
				if (match >= 0)
					bMatched[match] = true;
				else
					(*fxErr)++;
			}
			delete [] bMatched;
		}
		else
			*fxErr += m_fx.GetSize();
	}
	if (mask & SENDS_MASK)
	{
		// Count the number of sends in reaper project
		int numSends;
		for (numSends = 0; GetSetTrackSendInfo(tr, 0, numSends, "P_SRCTRACK", NULL); numSends++);
			if (numSends)
			{
				bool* bMatched = new bool[numSends];
				memset(bMatched, 0, sizeof(bool) * numSends);
				for (int i = 0; i < m_sends.GetSize(); i++)
				{
					int match = m_sends.Get(i)->UpdateReaper(tr, bMatched, numSends);
					if (match >= 0)
						bMatched[match] = true;
					else
						(*sendErr)++;
				}
				delete [] bMatched;
			}
			else
				*sendErr += m_sends.GetSize();
	}
	
	return false;
}

bool TrackSnapshot::Cleanup()
{
	MediaTrack* tr = GetTrack();
	if (!tr)
		return true;

	for (int i = 0; i < m_fx.GetSize(); i++)
		if (!m_fx.Get(i)->Exists(tr))
		{
			m_fx.Delete(i, true);
			i--;
		}

	for (int i = 0; i < m_sends.GetSize(); i++)
		if (!m_sends.Get(i)->Exists(tr))
		{
			m_sends.Delete(i, true);
			i--;
		}
	
	return false;
}

MediaTrack* TrackSnapshot::GetTrack()
{
	MediaTrack* tr = NULL;
	if (m_guid == GUID_NULL)
		tr = CSurf_TrackFromID(0, false);
	else
	{
		int iTrack;
		for (iTrack = 1; iTrack <= GetNumTracks(); iTrack++)
		{
			tr = CSurf_TrackFromID(iTrack, false);
			if (memcmp((GUID*)GetSetMediaTrackInfo(tr, "GUID", NULL), &m_guid, sizeof(m_guid)) == 0)
				break;
		}
		if (iTrack > GetNumTracks())
			return NULL;
	}
	return tr;
}

Snapshot::Snapshot(int slot, int mask, bool bSelOnly, char* name)
{
	m_iSlot = slot;
	m_iMask = mask;
	m_cName = NULL;
	m_time = (int)time(NULL);
	SetName(name);

	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);

		if (!bSelOnly || *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			m_tracks.Add(new TrackSnapshot(tr, mask));
	}
}

Snapshot::Snapshot(int slot, int mask, char* name, int time)
{
	m_iSlot = slot;
	m_iMask = mask;
	m_cName = NULL;
	SetName(name);
	m_time = time;
}


Snapshot::~Snapshot()
{
	delete [] m_cName;
	m_tracks.Empty(true);
}

bool Snapshot::UpdateReaper(int mask, bool bSelOnly, bool bHideNewVis)
{
	char str[256];
	Undo_BeginBlock();
	int trackErr = 0, fxErr = 0, sendErr = 0;
	for (int i = 0; i < m_tracks.GetSize(); i++)
		if (m_tracks.Get(i)->UpdateReaper(mask & m_iMask, &sendErr, &fxErr, bSelOnly))
			trackErr++;
	if (mask & m_iMask & VIS_MASK)
	{
		if (!bSelOnly && bHideNewVis && GetNumTracks() > 1)
		{
			// Find tracks that aren't in the snapshot, and hide them
			// TODO - what type of hide??
			bool* bInSnapshot = new bool[GetNumTracks()];
			memset(bInSnapshot, 0, GetNumTracks() * sizeof(bool));
			for (int i = 0; i < m_tracks.GetSize(); i++)
			{
				int iTrack = CSurf_TrackToID(m_tracks.Get(i)->GetTrack(), false);
				if (iTrack >= 1)
					bInSnapshot[iTrack-1] = true;
			}
			for (int i = 0; i < GetNumTracks(); i++)
			{
				if (!bInSnapshot[i])
					SetTrackVis(CSurf_TrackFromID(i+1, false), 0);
			}
			delete [] bInSnapshot;
		}

		// Must manually redraw visibiliy changes, maybe others??
		TrackList_AdjustWindows(false);
		UpdateTimeline();
	}
	sprintf(str, "Load snapshot %s", m_cName);
	Undo_EndBlock(str, UNDO_STATE_ALL);

	if (trackErr || fxErr || sendErr)
	{
		char errString[512];
		int n = 0;
		if (trackErr)
			n += sprintf(errString + n, "%d track(s) from snapshot not found.", trackErr);
		if (fxErr)
			n += sprintf(errString + n, "%s%d FX from snapshot not found.", n ? "\n" : "", fxErr);
		if (sendErr)
			n += sprintf(errString + n, "%s%d send(s) from snapshot not found.", n ? "\n" : "", sendErr);
		sprintf(errString + n, "\nDelete abandonded items from snapshot?  (You cannot undo this operation!)");
		if (MessageBox(g_hwndParent, errString, "Snapshot recall error", MB_YESNO) == IDYES)
		{
			for (int i = 0; i < m_tracks.GetSize(); i++)
				if (m_tracks.Get(i)->Cleanup())
				{
					m_tracks.Delete(i, true);
					i--;
				}
			return true;
		}
	}
	return false;
}

char* Snapshot::Tooltip(char* str, int maxLen)
{
	int n = 0;
	// Look to see if the master track is included in the snapshot
	int i;
	for (i = 0; i < m_tracks.GetSize(); i++)
		if (CSurf_TrackToID(m_tracks.Get(i)->GetTrack(), false) == 0)
			break;

	if (i < m_tracks.GetSize())
		n = _snprintf(str, maxLen, "Master + %d tracks", m_tracks.GetSize() - 1);
	else
		n = _snprintf(str + n, maxLen - n, "%d tracks", m_tracks.GetSize());

	if (m_iMask & VOL_MASK && n < maxLen)
		n += _snprintf(str + n, maxLen - n, "%s", ", vol");
	if (m_iMask & PAN_MASK && n < maxLen)
		n += _snprintf(str + n, maxLen - n, "%s", ", pan");
	if (m_iMask & MUTE_MASK && n < maxLen)
		n += _snprintf(str + n, maxLen - n, "%s", ", mute");
	if (m_iMask & SOLO_MASK && n < maxLen)
		n += _snprintf(str + n, maxLen - n, "%s", ", solo");
	if (m_iMask & FXATM_MASK && n < maxLen)
		n += _snprintf(str + n, maxLen - n, "%s", ", fx");
	if (m_iMask & SENDS_MASK && n < maxLen)
		n += _snprintf(str + n, maxLen - n, "%s", ", sends");
	if (m_iMask & VIS_MASK && n < maxLen)
		n += _snprintf(str + n, maxLen - n, "%s", ", visibility");
	if (m_iMask & SEL_MASK && n < maxLen)
		n += _snprintf(str + n, maxLen - n, "%s", ", selection");

	return str;
}

void Snapshot::SetName(const char* name)
{
	if (m_cName)
		delete [] m_cName;
	if (!name)
	{
		char newName[20];
		switch(m_iMask)
		{
			case VOL_MASK:		sprintf(newName, "Vol %d", m_iSlot);	break;
			case PAN_MASK:		sprintf(newName, "Pan %d", m_iSlot);	break;
			case MUTE_MASK:		sprintf(newName, "Mute %d", m_iSlot);	break;
			case SOLO_MASK:		sprintf(newName, "Solo %d", m_iSlot);	break;
			case FXATM_MASK:	sprintf(newName, "FX %d", m_iSlot);		break;
			case SENDS_MASK:	sprintf(newName, "Sends %d", m_iSlot);	break;
			case VIS_MASK:		sprintf(newName, "Vis %d", m_iSlot);	break;
			case SEL_MASK:		sprintf(newName, "Sel %d", m_iSlot);	break;
			default:			sprintf(newName, "Mix %d", m_iSlot);	break;
		}
		m_cName = new char[strlen(newName)+1];
		strcpy(m_cName, newName);
	}
	else
	{
		m_cName = new char[strlen(name)+1];
		strcpy(m_cName, name);
	}
}

void Snapshot::AddSelTracks()
{
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			if (Find(tr) == -1)
				m_tracks.Add(new TrackSnapshot(tr, m_iMask));
	}
}

void Snapshot::DelSelTracks()
{
	for (int i = 0; i < m_tracks.GetSize(); i++)
	{
		MediaTrack* tr = m_tracks.Get(i)->GetTrack();
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			m_tracks.Delete(i, true);
			i--;
		}
	}
}

void Snapshot::SelectTracks()
{
	int iSel = 1;
	ClearSelected();
	for (int i = 0; i < m_tracks.GetSize(); i++)
	{
		MediaTrack* tr = m_tracks.Get(i)->GetTrack();
		if (tr)
			GetSetMediaTrackInfo(tr, "I_SELECTED", &iSel);
	}
}

int Snapshot::Find(MediaTrack* tr)
{
	for (int i = 0; i < m_tracks.GetSize(); i++)
		if (tr == m_tracks.Get(i)->GetTrack())
			return i;
	return -1;
}
