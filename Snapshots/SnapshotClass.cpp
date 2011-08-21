/******************************************************************************
/ SnapshotClass.cpp
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
#include "../Utility/Base64.h"
#include "SnapshotClass.h"
#include "Snapshots.h"

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

FXSnapshot::FXSnapshot(FXSnapshot& fx)
{
	m_iNumParams = fx.m_iNumParams;
	m_iCurParam  = fx.m_iCurParam;
	strcpy(m_cName, fx.m_cName);
	if (m_iNumParams)
	{
		m_dParams = new double[m_iNumParams];
		memcpy(m_dParams, fx.m_dParams, m_iNumParams * sizeof(double));
	}
	else
		m_dParams = NULL;
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

void FXSnapshot::GetChunk(WDL_String *chunk)
{
	chunk->AppendFormatted(chunk->GetLength()+100, "<FX \"%s\" %d\n", m_cName, m_iNumParams);
	int iDoublesLeft = m_iNumParams;
	int iLine = 0;
	Base64 b64;
	while (iDoublesLeft)
	{
		int iDoubles = iDoublesLeft;
		if (iDoublesLeft >= DOUBLES_PER_LINE)
			iDoubles = DOUBLES_PER_LINE;
		chunk->AppendFormatted(chunk->GetLength()+100, b64.Encode((char*)(&m_dParams[iLine*DOUBLES_PER_LINE]), iDoubles * sizeof(double)));
		chunk->Append("\n");

		iDoublesLeft -= iDoubles;
		if (iDoublesLeft < 0)
			iDoublesLeft = 0;
	}
	chunk->Append(">\n");
}

void FXSnapshot::RestoreParams(const char* str)
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

TrackSnapshot::TrackSnapshot(MediaTrack* tr, int mask)
{
	m_iTrackNum = CSurf_TrackToID(tr, false);
	char* cName = (char*)GetSetMediaTrackInfo(tr, "P_NAME", NULL);
	m_sName.Set(cName ? cName : "");

	if (!m_iTrackNum)
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

	if (g_bv4)
	{
		m_iPanMode	= *((int*)GetSetMediaTrackInfo(tr, "I_PANMODE", NULL));
		m_dPanWidth	= *((double*)GetSetMediaTrackInfo(tr, "D_WIDTH", NULL));
		m_dPanL		= *((double*)GetSetMediaTrackInfo(tr, "D_DUALPANL", NULL));
		m_dPanR		= *((double*)GetSetMediaTrackInfo(tr, "D_DUALPANR", NULL));
	}
	else
	{
		m_iPanMode = -1; // Project default
		m_dPanWidth = m_dPanL = m_dPanR = 0.0;
	}	
	m_dPanLaw = *((double*)GetSetMediaTrackInfo(tr, "D_PANLAW", NULL));

	// Don't bother storing the sends if it's masked
	if (mask & SENDS_MASK)
		m_sends.Build(tr);

	// Same for the fx
	// DEPRECATED
	if (mask & FXATM_MASK)
		for (int i = 0; i < TrackFX_GetCount(tr); i++)
			m_fx.Add(new FXSnapshot(tr, i));

	// and the full FX chain
	if (mask & FXCHAIN_MASK)
		GetFXChain(tr, &m_sFXChain);
	
	// Get the "std" envelopes
	if (mask & VOL_MASK)
	{
		GetSetEnvelope(tr, &m_sVolEnv, "Volume (Pre-FX)", false);
		GetSetEnvelope(tr, &m_sVolEnv2, "Volume", false);
	}
	if (mask & PAN_MASK)
	{
		GetSetEnvelope(tr, &m_sPanEnv, "Pan (Pre-FX)", false);
		GetSetEnvelope(tr, &m_sPanEnv2, "Pan", false);
		GetSetEnvelope(tr, &m_sWidthEnv, "Width (Pre-FX)", false);
		GetSetEnvelope(tr, &m_sWidthEnv2, "Width", false);
	}
	if (mask & MUTE_MASK)
		GetSetEnvelope(tr, &m_sMuteEnv, "Mute", false);
}

// "Copy" constructor with mask large items don't get copied too
TrackSnapshot::TrackSnapshot(TrackSnapshot& ts):m_sends(ts.m_sends)
{
	m_guid = ts.m_guid;
	m_dVol = ts.m_dVol;
	m_dPan = ts.m_dPan;
	m_bMute = ts.m_bMute;
	m_iSolo = ts.m_iSolo;
	m_iFXEn = ts.m_iFXEn;
	m_iVis = ts.m_iVis;
	m_iSel = ts.m_iSel;
	for (int i = 0; i < ts.m_fx.GetSize(); i++)
		m_fx.Add(new FXSnapshot(*ts.m_fx.Get(i)));
	if (ts.m_sFXChain.GetSize())
	{
		m_sFXChain.Resize(ts.m_sFXChain.GetSize());
		memcpy(m_sFXChain.Get(), ts.m_sFXChain.Get(), m_sFXChain.GetSize());
	}
	m_sName.Set(ts.m_sName.Get());
	m_iTrackNum = ts.m_iTrackNum;
	m_iPanMode	= ts.m_iPanMode;
	m_dPanWidth	= ts.m_dPanWidth;
	m_dPanL		= ts.m_dPanL;
	m_dPanR		= ts.m_dPanR;
	m_dPanLaw	= ts.m_dPanLaw;
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
	m_iPanMode	= lp->getnumtokens() < 10 ? -1 : lp->gettoken_int(9); // If loading old format, set pan mode to -1 for proj default
	m_dPanWidth	= lp->gettoken_float(10);
	m_dPanL		= lp->gettoken_float(11);
	m_dPanR		= lp->gettoken_float(12);
	m_dPanLaw   = lp->getnumtokens() < 14 ? -100.0 : lp->gettoken_float(13); // If loading old format, set law to -2 for "ignore"

	// Set the track name "early" for backward compat
	MediaTrack* tr = GuidToTrack(&m_guid);
	if (tr)
	{
		char* cName = (char*)GetSetMediaTrackInfo(tr, "P_NAME", NULL);
		m_sName.Set(cName ? cName : "");
		m_iTrackNum = CSurf_TrackToID(tr, false);
	}
	else
		m_iTrackNum = -1;
}

TrackSnapshot::~TrackSnapshot()
{
	m_fx.Empty(true);
}

// Returns true if cannot find the track to update!
bool TrackSnapshot::UpdateReaper(int mask, bool bSelOnly, int* fxErr, WDL_PtrList<TrackSendFix>* pFix)
{
	MediaTrack* tr = GuidToTrack(&m_guid);
	if (!tr)
		return true;

	int iSel = *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL);
	if (bSelOnly && !iSel)
		return false; // Ignore if the track isn't selected

	if (mask & VOL_MASK)
	{
		GetSetMediaTrackInfo(tr, "D_VOL", &m_dVol);
		GetSetEnvelope(tr, &m_sVolEnv, "Volume (Pre-FX)", true);
		GetSetEnvelope(tr, &m_sVolEnv2, "Volume", true);
	}
	if (mask & PAN_MASK)
	{
		GetSetMediaTrackInfo(tr, "D_PAN", &m_dPan);
		if (g_bv4)
		{
			GetSetMediaTrackInfo(tr, "I_PANMODE", &m_iPanMode);
			GetSetMediaTrackInfo(tr, "D_WIDTH", &m_dPanWidth);
			GetSetMediaTrackInfo(tr, "D_DUALPANL", &m_dPanL);
			GetSetMediaTrackInfo(tr, "D_DUALPANR", &m_dPanR);
		}
		if (m_dPanLaw != -100.0)
			GetSetMediaTrackInfo(tr, "D_PANLAW", &m_dPanLaw);
		GetSetEnvelope(tr, &m_sPanEnv, "Pan (Pre-FX)", true);
		GetSetEnvelope(tr, &m_sPanEnv2, "Pan", true);
		GetSetEnvelope(tr, &m_sWidthEnv, "Width (Pre-FX)", true);
		GetSetEnvelope(tr, &m_sWidthEnv2, "Width", true);
	}
	if (mask & MUTE_MASK)
	{
		GetSetMediaTrackInfo(tr, "B_MUTE", &m_bMute);
		GetSetEnvelope(tr, &m_sMuteEnv, "Mute", true);
	}
	if (mask & SOLO_MASK)
		GetSetMediaTrackInfo(tr, "I_SOLO", &m_iSolo);
	if (mask & VIS_MASK)
		SetTrackVis(tr, m_iVis); // ignores master
	if (mask & SEL_MASK)
		GetSetMediaTrackInfo(tr, "I_SELECTED", &m_iSel);
	if (mask & FXATM_MASK) // DEPRECATED, keep for previously saved snapshots
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
	if (mask & FXCHAIN_MASK)
	{
		GetSetMediaTrackInfo(tr, "I_FXEN", &m_iFXEn);
		SetFXChain(tr, m_sFXChain.Get());
	}
	if (mask & SENDS_MASK)
	{
		m_sends.UpdateReaper(tr, pFix);
	}
	
	return false;
}

bool TrackSnapshot::Cleanup()
{
	MediaTrack* tr = GuidToTrack(&m_guid);
	if (!tr)
		return true;

	for (int i = 0; i < m_fx.GetSize(); i++)
		if (!m_fx.Get(i)->Exists(tr))
		{
			m_fx.Delete(i, true);
			i--;
		}
	
	return false;
}

// Only append, don't overwrite the chunk string
void TrackSnapshot::GetChunk(WDL_String* chunk)
{
	char guidStr[64];
	guidToString(&m_guid, guidStr);
	chunk->AppendFormatted(chunk->GetLength()+250, "<TRACK %s %.14f %.14f %d %d %d %d %d %d %.14f %.14f %.14f %.14f\n", guidStr, m_dVol, m_dPan, m_bMute ? 1 : 0, m_iSolo, m_iFXEn, m_iVis ^ 2, m_iSel, m_iPanMode, m_dPanWidth, m_dPanL, m_dPanR, m_dPanLaw);
	chunk->AppendFormatted(chunk->GetLength()+100, "NAME \"%s\" %d\n", m_sName.Get(), m_iTrackNum);
	
	m_sends.GetChunk(chunk);
	for (int i = 0; i < m_fx.GetSize(); i++)
		m_fx.Get(i)->GetChunk(chunk);
	if (m_sFXChain.GetSize())
		chunk->Append(m_sFXChain.Get());
	if (m_sVolEnv.GetLength())
		chunk->Append(m_sVolEnv.Get());
	if (m_sVolEnv2.GetLength())
		chunk->Append(m_sVolEnv2.Get());
	if (m_sPanEnv.GetLength())
		chunk->Append(m_sPanEnv.Get());
	if (m_sPanEnv2.GetLength())
		chunk->Append(m_sPanEnv2.Get());
	if (m_sWidthEnv.GetLength())
		chunk->Append(m_sWidthEnv.Get());
	if (m_sWidthEnv2.GetLength())
		chunk->Append(m_sWidthEnv2.Get());
	if (m_sMuteEnv.GetLength())
		chunk->Append(m_sMuteEnv.Get());
	chunk->Append(">\n");
}

void TrackSnapshot::GetDetails(WDL_String* details, int iMask)
{
	MediaTrack* tr = GuidToTrack(&m_guid);

	if (m_iTrackNum == 0)
		details->Append("Master Track:\r\n");
	else if (tr)
	{
		char* cName = (char*)GetSetMediaTrackInfo(tr, "P_NAME", NULL);
		int iNum = CSurf_TrackToID(tr, false);
		if (strcmp(cName, m_sName.Get()) == 0 && m_iTrackNum == iNum)
			details->AppendFormatted(100, "Track #%d \"%s\":\r\n", iNum, cName);
		else
			details->AppendFormatted(100, "Track #%d \"%s\", originally #%d \"%s\":\r\n", iNum, cName, m_iTrackNum, m_sName.Get());
	}
	else
		details->AppendFormatted(100, "Track #%d \"%s\" (not in current project!):\r\n", m_iTrackNum, m_sName.Get());

	if (iMask & VOL_MASK)
	{
		details->AppendFormatted(50, "Volume: %.2fdb\r\n", VAL2DB(m_dVol));
		if (m_sVolEnv.GetLength())
			details->AppendFormatted(50, "Volume (Pre-FX) envelope\r\n");
		if (m_sVolEnv2.GetLength())
			details->AppendFormatted(50, "Volume envelope\r\n");
	}
	if (iMask & PAN_MASK)
	{
		int iPanMode = m_iPanMode;
		if (g_bv4 && iPanMode == -1)
			iPanMode = *(int*)GetConfigVar("panmode"); // display pan in project format

		if (iPanMode != 6)
		{
			if (m_dPan == 0.0)
				details->Append("Pan: center");
			else
				details->AppendFormatted(50, "Pan: %d%% %s", abs((int)(m_dPan * 100.0)), m_dPan < 0.0 ? "left" : "right");
		}
		if (iPanMode == 5) // stereo pan
			details->AppendFormatted(50, ", width %d%%", (int)(m_dPanWidth * 100.0));
		else if (iPanMode == 6) // dual pan
			details->AppendFormatted(50, "Left pan: %d%%%c, Right pan: %d%%%c", abs((int)(m_dPanL * 100.0)), m_dPanL == 0.0 ? 'C' : (m_dPanL < 0.0 ? 'L' : 'R'), abs((int)(m_dPanR * 100.0)), m_dPanR == 0.0 ? 'C' : (m_dPanR < 0.0 ? 'L' : 'R'));

		if (m_dPanLaw == -1.0)
			details->Append(", default pan law\r\n");
		else if (m_dPanLaw != -100.0)
			details->AppendFormatted(50, ", Pan law %.4f\r\n", m_dPanLaw);
		else
			details->Append("\r\n");

		if (m_sPanEnv.GetLength())
			details->AppendFormatted(50, "Pan (Pre-FX) envelope\r\n");
		if (m_sPanEnv2.GetLength())
			details->AppendFormatted(50, "Pan envelope\r\n");
		if (m_sWidthEnv.GetLength())
			details->AppendFormatted(50, "Width (Pre-FX) envelope\r\n");
		if (m_sWidthEnv2.GetLength())
			details->AppendFormatted(50, "Width envelope\r\n");
	}
	if (iMask & MUTE_MASK)
	{
		details->Append(m_bMute ? "Mute: on\r\n" : "Mute: off\r\n");
		if (m_sMuteEnv.GetLength())
			details->AppendFormatted(50, "Mute envelope\r\n");
	}
	if (iMask & SOLO_MASK)
		details->Append(m_iSolo ? "Solo: on\r\n" : "Solo: off\r\n");
	if (iMask & SEL_MASK)
		details->Append(m_iSel ? "Selected: yes\r\n" : "Selected: no\r\n");
	if (iMask & VIS_MASK)
	{
		details->Append("Visibility: ");
		switch (m_iVis)
		{
			case 0: details->Append("invisible\r\n"); break;
			case 1: details->Append("MCP only\r\n"); break;
			case 2: details->Append("TCP only\r\n"); break;
			case 3: details->Append("full\r\n"); break;
		}
	}
	if (iMask & FXCHAIN_MASK)
	{
		details->Append(m_iFXEn ? "FX bypass: off\r\n" : "FX bypass: on\r\n");
		if (!m_sFXChain.GetSize())
			details->Append("Empty FX chain\r\n");
		else
		{
			details->Append("FX chain:\r\n");
			char line[4096];
			int pos = 0;
			LineParser lp(false);
			while (GetChunkLine(m_sFXChain.Get(), line, 4096, &pos, false))
			{
				if (!lp.parse(line) && lp.getnumtokens() >= 2)
				{
					if (strncmp(lp.gettoken_str(0), "<VST", 4) == 0 || strncmp(lp.gettoken_str(0), "<DX", 3) == 0)
						details->AppendFormatted(50, "\t%s\r\n", lp.gettoken_str(1));
					else if (strcmp(lp.gettoken_str(0), "<JS") == 0)
						details->AppendFormatted(50, "\tJS: %s\r\n", lp.gettoken_str(1));
				}
			}
		}
	}
	if (iMask & SENDS_MASK)
	{
		if (m_sends.m_hwSends.GetSize())
		{
			details->Append("Hardware outputs:\r\n");
			LineParser lp(false);
			for (int i = 0; i < m_sends.m_hwSends.GetSize(); i++)
			{
				lp.parse(m_sends.m_hwSends.Get(i)->Get());
				details->AppendFormatted(50, "\t%s\r\n", GetOutputChannelName(lp.gettoken_int(1) & 0x3FF));
			}
		}
		if (m_sends.m_sends.GetSize())
		{
			details->Append("Sends:\r\n");
			LineParser lp(false);
			for (int i = 0; i < m_sends.m_sends.GetSize(); i++)
			{
				MediaTrack* tr = GuidToTrack(m_sends.m_sends.Get(i)->GetGuid());
				if (tr)
				{
					char* cName = (char*)GetSetMediaTrackInfo(tr, "P_NAME", NULL);
					details->AppendFormatted(100, "\tTo track #%d \"%s\"\r\n", CSurf_TrackToID(tr, false), cName);
				}
				else
					details->Append("\tTo unknown track!\r\n");
			}
		}
		else
			details->Append("No sends\r\n");
	}
}

void TrackSnapshot::GetSetEnvelope(MediaTrack* tr, WDL_String* str, const char* env, bool bSet)
{
	TrackEnvelope* te = GetTrackEnvelopeByName(tr, env);
	if (!bSet)
	{	// Get envelope from REAPER
		if (te)
		{
			char envStr[262144] = "";
			GetSetEnvelopeState(te, envStr, 262144);
			str->Set(envStr);
		}
		else
			str->Set("");
	}
	else if (str->GetLength())
	{	// Set envelope
		if (te)
			GetSetEnvelopeState(te, str->Get(), 0);
		else
		{
			WDL_String state;
			state.Set(SWS_GetSetObjectState(tr, NULL));
			*strrchr(state.Get(), '>') = 0; // Remove the last >
			// Do a little dance to set the length properly
			WDL_String newState;
			newState.Set(state.Get());
			newState.Append(str->Get());
			newState.Append(">\n");
			SWS_GetSetObjectState(tr, &newState);
		}
	}
}

bool TrackSnapshot::ProcessEnv(const char* chunk, char* line, int iLineMax, int* pos, const char* env, WDL_String* str)
{
	if (strcmp(env, line) == 0)
	{
		str->Set(line);
		str->Append("\n");
		int iDepth = 1;
		while (iDepth && GetChunkLine(chunk, line, iLineMax, pos, true))
		{
			str->Append(line);
			if (line[0] == '<')
				iDepth++;
			else if (line[0] == '>')
				iDepth--;
		}
		return true;
	}
	return false;
}

Snapshot::Snapshot(int slot, int mask, bool bSelOnly, const char* name)
{
	m_iSlot = slot;
	m_iMask = mask;
	m_time = (int)time(NULL);
	m_cName = NULL;
	SetName(name);

	SWS_CacheObjectState(true);
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);

		if (!bSelOnly || *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			m_tracks.Add(new TrackSnapshot(tr, mask));
	}
	SWS_CacheObjectState(false);

	char undoStr[128];
	sprintf(undoStr, "Save snapshot %d", slot);
	Undo_OnStateChangeEx(undoStr, UNDO_STATE_MISCCFG, -1);
	RegisterGetCommand(slot);
}

// Build a snapshot from an XML/RPP chunk from the clipboard/RPP/undo/etc
Snapshot::Snapshot(const char* chunk)
{
	char line[4096];
	int pos = 0;
	LineParser lp(false);
	TrackSnapshot* ts = NULL;
	m_cName = NULL;

	while(GetChunkLine(chunk, line, 4096, &pos, false))
	{
		if (lp.parse(line))
			break;

		if (strcmp(lp.gettoken_str(0), "<SWSSNAPSHOT") == 0)
		{
			m_time = 0;
			if (lp.getnumtokens() == 6) // Old-style time, convert
			{
		#ifdef _WIN32
				FILETIME ft;
				SYSTEMTIME st;
				ft.dwHighDateTime = strtoul(lp.gettoken_str(4), NULL, 10);
				ft.dwLowDateTime  = strtoul(lp.gettoken_str(5), NULL, 10);
				FileTimeToSystemTime(&ft, &st);
				struct tm pt;
				pt.tm_sec  = st.wSecond;
				pt.tm_min  = st.wMinute;
				pt.tm_hour = st.wHour;
				pt.tm_mday = st.wDay;
				pt.tm_mon  = st.wMonth - 1;
				pt.tm_year = st.wYear - 1900;
				pt.tm_isdst = -1;
				m_time = (int)mktime(&pt);
		#endif
			}
			else
				m_time = lp.gettoken_int(4);

			m_iSlot = lp.gettoken_int(2);
			m_iMask = lp.gettoken_int(3);
			SetName(lp.gettoken_str(1));
		}
		else if (strcmp("TRACK", lp.gettoken_str(0)) == 0 || strcmp("<TRACK", lp.gettoken_str(0)) == 0)
		{
			ts = m_tracks.Add(new TrackSnapshot(&lp));
		}
		else if (ts)
		{
			if (strcmp("NAME", lp.gettoken_str(0)) == 0)
			{
				ts->m_sName.Set(lp.gettoken_str(1));
				if (lp.getnumtokens() >= 3)
					ts->m_iTrackNum = lp.gettoken_int(2);
			}
			else if (strcmp("SEND", lp.gettoken_str(0)) == 0)
			{	// Handle deprecated stored send information
				GUID guid;
				stringToGuid(lp.gettoken_str(1), &guid);
				ts->m_sends.m_sends.Add(new TrackSend(&guid, lp.gettoken_int(5), lp.gettoken_float(2), lp.gettoken_float(3),
					lp.gettoken_int(4), 0, 0, lp.gettoken_int(6), lp.gettoken_int(7), lp.gettoken_int(8), -1));
			}
			else if (strcmp("AUXSEND", lp.gettoken_str(0)) == 0)
			// Same format as AUXRECV but on the send track, with second param as recv GUID
				ts->m_sends.m_sends.Add(new TrackSend(line));
			else if (strcmp("HWOUT", lp.gettoken_str(0)) == 0)
				ts->m_sends.m_hwSends.Add(new WDL_String(line));
			else if (strcmp("FX", lp.gettoken_str(0)) == 0) // "One liner"
				ts->m_fx.Add(new FXSnapshot(&lp));
			else if (strcmp("<FX", lp.gettoken_str(0)) == 0) // Multiple lines
			{
				FXSnapshot* fx = ts->m_fx.Add(new FXSnapshot(&lp));
				while(GetChunkLine(chunk, line, 4096, &pos, false))
				{
					if (lp.parse(line) || lp.gettoken_str(0)[0] == '>')
						break;
					fx->RestoreParams(lp.gettoken_str(0));
				}
			}
			else if (strcmp("<FXCHAIN", lp.gettoken_str(0)) == 0) // Multiple lines
			{
				int iLen = (int)strlen(line);
				ts->m_sFXChain.Resize(iLen + 2);
				strcpy(ts->m_sFXChain.Get(), line);
				strcpy(ts->m_sFXChain.Get()+iLen, "\n");
				iLen++;

				int iDepth = 1;
				while(iDepth && GetChunkLine(chunk, line, 4096, &pos, true))
				{
					int iNewLen = iLen + (int)strlen(line);
					ts->m_sFXChain.Resize(iNewLen + 1);
					strcpy(ts->m_sFXChain.Get()+iLen, line);
					iLen = iNewLen;

					if (line[0] == '>')
						iDepth--;
					else if (line[0] == '<')
						iDepth++;
				}
			}
			// Yuck, not too happy with the below code, but it works.
			else if (ts->ProcessEnv(chunk, line, 4096, &pos, "<VOLENV", &ts->m_sVolEnv)) {}
			else if (ts->ProcessEnv(chunk, line, 4096, &pos, "<VOLENV2", &ts->m_sVolEnv2)) {}
			else if (ts->ProcessEnv(chunk, line, 4096, &pos, "<PANENV", &ts->m_sPanEnv)) {}
			else if (ts->ProcessEnv(chunk, line, 4096, &pos, "<PANENV2", &ts->m_sPanEnv2)) {}
			else if (ts->ProcessEnv(chunk, line, 4096, &pos, "<WIDTHENV", &ts->m_sWidthEnv)) {}
			else if (ts->ProcessEnv(chunk, line, 4096, &pos, "<WIDTHENV2", &ts->m_sWidthEnv2)) {}
			else if (ts->ProcessEnv(chunk, line, 4096, &pos, "<MUTEENV", &ts->m_sMuteEnv)) {}
		}
	}
	RegisterGetCommand(m_iSlot);
}

Snapshot::~Snapshot()
{
	delete [] m_cName;
	m_tracks.Empty(true);
}

bool Snapshot::UpdateReaper(int mask, bool bSelOnly, bool bHideNewVis)
{
	char str[256];
	//Undo_BeginBlock();
	int trackErr = 0, fxErr = 0;
	WDL_PtrList<TrackSendFix> sendFixes;

	// Do "non-chunk" stuff first
	for (int i = 0; i < m_tracks.GetSize(); i++)
		m_tracks.Get(i)->UpdateReaper(mask & m_iMask & ~CHUNK_MASK, bSelOnly, &fxErr, &sendFixes);
	
	// Then cache all ObjectState changes for the chunk updating
	SWS_CacheObjectState(true);
	for (int i = 0; i < m_tracks.GetSize(); i++)
		if (m_tracks.Get(i)->UpdateReaper(mask & m_iMask & CHUNK_MASK, bSelOnly, &fxErr, &sendFixes))
			trackErr++;
	SWS_CacheObjectState(false);

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
				int iTrack = CSurf_TrackToID(GuidToTrack(&m_tracks.Get(i)->m_guid), false);
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
	Undo_OnStateChangeEx(str, UNDO_STATE_ALL, -1);

	if (trackErr || fxErr)
	{
		char errString[512];
		int n = 0;
		if (trackErr)
			n += sprintf(errString + n, "%d track(s) from snapshot not found.", trackErr);
		if (fxErr)
			n += sprintf(errString + n, "%s%d FX from snapshot not found.", n ? "\n" : "", fxErr);
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
		if (GuidsEqual(&m_tracks.Get(i)->m_guid, &GUID_NULL))
			break;

	if (i < m_tracks.GetSize())
		n = _snprintf(str, maxLen, "Master + %d track%s", m_tracks.GetSize() - 1, m_tracks.GetSize() - 1 > 1 ? "s" : "");
	else
		n = _snprintf(str + n, maxLen - n, "%d track%s", m_tracks.GetSize(), m_tracks.GetSize() > 1 ? "s" : "");

	if (m_iMask & VOL_MASK && n < maxLen)
		n += _snprintf(str + n, maxLen - n, "%s", ", vol");
	if (m_iMask & PAN_MASK && n < maxLen)
		n += _snprintf(str + n, maxLen - n, "%s", ", pan");
	if (m_iMask & MUTE_MASK && n < maxLen)
		n += _snprintf(str + n, maxLen - n, "%s", ", mute");
	if (m_iMask & SOLO_MASK && n < maxLen)
		n += _snprintf(str + n, maxLen - n, "%s", ", solo");
	if (m_iMask & FXATM_MASK && n < maxLen)
		n += _snprintf(str + n, maxLen - n, "%s", ", fx (old style)");
	if (m_iMask & FXCHAIN_MASK && n < maxLen)
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
			case FXATM_MASK: // fallthrough
			case FXCHAIN_MASK:	sprintf(newName, "FX %d", m_iSlot);		break;
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
		MediaTrack* tr = GuidToTrack(&m_tracks.Get(i)->m_guid);
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
		MediaTrack* tr = GuidToTrack(&m_tracks.Get(i)->m_guid);
		if (tr)
			GetSetMediaTrackInfo(tr, "I_SELECTED", &iSel);
	}
}

int Snapshot::Find(MediaTrack* tr)
{
	for (int i = 0; i < m_tracks.GetSize(); i++)
		if (tr == GuidToTrack(&m_tracks.Get(i)->m_guid))
			return i;
	return -1;
}

void Snapshot::RegisterGetCommand(int iSlot) // Slot is 1-based index.
{
	static int iLastRegistered = 0;
	if (iSlot > iLastRegistered)
	{
		char cID[BUFFER_SIZE];
		char cDesc[BUFFER_SIZE];
		_snprintf(cID, BUFFER_SIZE, "SWSSNAPSHOT_GET%d", iSlot);
		_snprintf(cDesc, BUFFER_SIZE, "SWS: Recall snapshot %d", iSlot);
		SWSRegisterCommandExt(GetSnapshot, cID, cDesc, iSlot);
		iLastRegistered = iSlot;
	}
}

char* Snapshot::GetTimeString(char* str, int iStrMax, bool bDate)
{
	str[0] = 0;

#ifdef _WIN32
	SYSTEMTIME st, st2;
	int t = m_time;
	struct tm pt;
	int err = _gmtime32_s(&pt, (__time32_t*)&t);
	if (!err)
	{
		st.wMilliseconds = 0;
		st.wSecond = pt.tm_sec;
		st.wMinute = pt.tm_min;
		st.wHour   = pt.tm_hour;
		st.wDay    = pt.tm_mday;
		st.wMonth  = pt.tm_mon + 1;
		st.wYear   = pt.tm_year + 1900;
		st.wDayOfWeek = pt.tm_wday;
		SystemTimeToTzSpecificLocalTime(NULL, &st, &st2);		
		
		if (bDate)
			GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st2, NULL, str, iStrMax);
		else
			GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st2, NULL, str, iStrMax);
	}
#else
	if (bDate)
		SWS_GetDateString(m_time, str, iStrMax);
	else
		SWS_GetTimeString(m_time, str, iStrMax);
#endif
	return str;
}

// Get chunk for writing out
void Snapshot::GetChunk(WDL_String* chunk)
{
	chunk->SetFormatted(chunk->GetLength()+100, "<SWSSNAPSHOT \"%s\" %d %d %d\n", m_cName, m_iSlot, m_iMask, m_time);
	for (int i = 0; i < m_tracks.GetSize(); i++)
		m_tracks.Get(i)->GetChunk(chunk);
	chunk->Append(">\n");
}

// Get a human-readable string that explains the snapshot
void Snapshot::GetDetails(WDL_String* details)
{
	char cTemp[100];
	details->AppendFormatted(100, "Snapshot %d \"%s\", stored ", m_iSlot, m_cName);
	GetTimeString(cTemp, 100, true);
	details->Append(cTemp);
	details->Append(" ");
	GetTimeString(cTemp, 100, false);
	details->Append(cTemp);
	details->Append("\r\n");
	char cSummary[100];
	details->Append(Tooltip(cSummary, 100));
	details->Append("\r\n");

	for (int i = 0; i < m_tracks.GetSize(); i++)
	{
		details->Append("\r\n");
		m_tracks.Get(i)->GetDetails(details, m_iMask);
	}
}

bool Snapshot::IncludesSelTracks()
{
	for (int i = 0; i < m_tracks.GetSize(); i++)
	{
		MediaTrack* tr = GuidToTrack(&m_tracks.Get(i)->m_guid);
		if (tr && *(bool*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			return true;
	}
	return false;
}