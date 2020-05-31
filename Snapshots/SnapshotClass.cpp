/******************************************************************************
/ SnapshotClass.cpp
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
#include "SnapshotClass.h"
#include "Snapshots.h"

#include <WDL/projectcontext.h>
#include <WDL/localize/localize.h>

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

void FXSnapshot::GetChunk(WDL_FastString *chunk)
{
	chunk->AppendFormatted(SNM_MAX_CHUNK_LINE_LENGTH, "<FX \"%s\" %d\n", m_cName, m_iNumParams);
	int iDoublesLeft = m_iNumParams;
	int iLine = 0;
	Base64 b64;
	while (iDoublesLeft)
	{
		int iDoubles = iDoublesLeft;
		if (iDoublesLeft >= DOUBLES_PER_LINE)
			iDoubles = DOUBLES_PER_LINE;
		chunk->AppendFormatted(SNM_MAX_CHUNK_LINE_LENGTH, "%s", b64.Encode((char*)(&m_dParams[iLine*DOUBLES_PER_LINE]), iDoubles * sizeof(double)));
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

	m_dVol            = *((double*)GetSetMediaTrackInfo(tr, "D_VOL", NULL));
	m_dPan            = *((double*)GetSetMediaTrackInfo(tr, "D_PAN", NULL));
	m_bMute           = *((bool*)GetSetMediaTrackInfo(tr, "B_MUTE", NULL));
	m_iSolo           = *((int*)GetSetMediaTrackInfo(tr, "I_SOLO", NULL));
	m_iFXEn           = *((int*)GetSetMediaTrackInfo(tr, "I_FXEN", NULL));
	m_iVis            = GetTrackVis(tr);
	m_iSel            = *((int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL));
	m_iPanMode        = *((int*)GetSetMediaTrackInfo(tr, "I_PANMODE", NULL));
	m_dPanWidth       = *((double*)GetSetMediaTrackInfo(tr, "D_WIDTH", NULL));
	m_dPanL           = *((double*)GetSetMediaTrackInfo(tr, "D_DUALPANL", NULL));
	m_dPanR           = *((double*)GetSetMediaTrackInfo(tr, "D_DUALPANR", NULL));
	m_dPanLaw         = *((double*)GetSetMediaTrackInfo(tr, "D_PANLAW", NULL));
	m_bPhase          = *((bool*)GetSetMediaTrackInfo(tr, "B_PHASE", NULL));
	m_iPlayOffsetFlag = static_cast<int>(GetMediaTrackInfo_Value(tr, "I_PLAY_OFFSET_FLAG"));
	m_dPlayOffset     = GetMediaTrackInfo_Value(tr, "D_PLAY_OFFSET");

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
	// JFB note: localized env names are retrieved in GetSetEnvelope()
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
	m_guid            = ts.m_guid;
	m_dVol            = ts.m_dVol;
	m_dPan            = ts.m_dPan;
	m_bMute           = ts.m_bMute;
	m_iSolo           = ts.m_iSolo;
	m_iFXEn           = ts.m_iFXEn;
	m_iVis            = ts.m_iVis;
	m_iSel            = ts.m_iSel;
	for (int i = 0; i < ts.m_fx.GetSize(); i++)
		m_fx.Add(new FXSnapshot(*ts.m_fx.Get(i)));
	if (ts.m_sFXChain.GetSize())
	{
		m_sFXChain.Resize(ts.m_sFXChain.GetSize());
		memcpy(m_sFXChain.Get(), ts.m_sFXChain.Get(), m_sFXChain.GetSize());
	}
	m_sName.Set(ts.m_sName.Get());
	m_iTrackNum       = ts.m_iTrackNum;
	m_iPanMode        = ts.m_iPanMode;
	m_dPanWidth       = ts.m_dPanWidth;
	m_dPanL           = ts.m_dPanL;
	m_dPanR           = ts.m_dPanR;
	m_dPanLaw         = ts.m_dPanLaw;
	m_bPhase          = ts.m_bPhase;
	m_iPlayOffsetFlag = ts.m_iPlayOffsetFlag;
	m_dPlayOffset     = ts.m_dPlayOffset;
}

TrackSnapshot::TrackSnapshot(LineParser* lp)
{
	stringToGuid(lp->gettoken_str(1), &m_guid);
	m_dVol            = lp->gettoken_float(2);
	m_dPan            = lp->gettoken_float(3);
	m_bMute           = lp->gettoken_int(4) ? true : false;
	m_iSolo           = lp->gettoken_int(5);
	m_iFXEn           = lp->gettoken_int(6);
	// For backward compat, flip the TCP vis bit
	m_iVis            = lp->gettoken_int(7) ^ 2;
	m_iSel            = lp->gettoken_int(8);
	m_iPanMode        = lp->getnumtokens() < 10 ? -1 : lp->gettoken_int(9); // If loading old format, set pan mode to -1 for proj default
	m_dPanWidth       = lp->gettoken_float(10);
	m_dPanL           = lp->gettoken_float(11);
	m_dPanR           = lp->gettoken_float(12);
	m_dPanLaw         = lp->getnumtokens() < 14 ? -100.0 : lp->gettoken_float(13); // If loading old format, set law to -2 for "ignore"
	m_bPhase          = lp->gettoken_int(14) ? true : false;
	m_iPlayOffsetFlag = lp->gettoken_int(15);
	m_dPlayOffset     = lp->gettoken_float(16);


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
bool TrackSnapshot::UpdateReaper(int mask, bool bSelOnly, int* fxErr, bool wantChunk, WDL_PtrList<TrackSendFix>* pFix)
{
	MediaTrack* tr = GuidToTrack(&m_guid);
	if (!tr)
		return true;

	int iSel = *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL);
	if (bSelOnly && !iSel)
		return false; // Ignore if the track isn't selected

	PreventUIRefresh(1);

	if (mask & VOL_MASK)
	{
		GetSetMediaTrackInfo(tr, "D_VOL", &m_dVol);
		GetSetEnvelope(tr, &m_sVolEnv, "Volume (Pre-FX)", true);
		GetSetEnvelope(tr, &m_sVolEnv2, "Volume", true);
	}
	if (mask & PAN_MASK)
	{
		GetSetMediaTrackInfo(tr, "D_PAN", &m_dPan);
		GetSetMediaTrackInfo(tr, "I_PANMODE", &m_iPanMode);
		GetSetMediaTrackInfo(tr, "D_WIDTH", &m_dPanWidth);
		GetSetMediaTrackInfo(tr, "D_DUALPANL", &m_dPanL);
		GetSetMediaTrackInfo(tr, "D_DUALPANR", &m_dPanR);
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
		if (wantChunk) SetFXChain(tr, m_sFXChain.Get());
	}
	if (mask & SENDS_MASK)
	{
		if (wantChunk) m_sends.UpdateReaper(tr, pFix);
	}
	if (mask & PHASE_MASK)
	{
		GetSetMediaTrackInfo(tr, "B_PHASE", &m_bPhase);
	}
	if (mask & PLAY_OFFSET_MASK)
	{
		SetMediaTrackInfo_Value(tr, "I_PLAY_OFFSET_FLAG", m_iPlayOffsetFlag);
		SetMediaTrackInfo_Value(tr, "D_PLAY_OFFSET", m_dPlayOffset);
	}

	PreventUIRefresh(-1);

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
void TrackSnapshot::GetChunk(WDL_FastString* chunk)
{
	char guidStr[64];
	guidToString(&m_guid, guidStr);
	chunk->AppendFormatted(SNM_MAX_CHUNK_LINE_LENGTH, "<TRACK %s %.14f %.14f %d %d %d %d %d %d %.14f %.14f %.14f %.14f %d %d %.14f\n", 
		guidStr, m_dVol, m_dPan, m_bMute ? 1 : 0, m_iSolo, m_iFXEn, m_iVis ^ 2, m_iSel, m_iPanMode, m_dPanWidth, m_dPanL, m_dPanR, m_dPanLaw, m_bPhase ? 1 : 0, m_iPlayOffsetFlag, m_dPlayOffset);
	chunk->AppendFormatted(SNM_MAX_CHUNK_LINE_LENGTH, "NAME \"%s\" %d\n", m_sName.Get(), m_iTrackNum);
	
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

void TrackSnapshot::GetDetails(WDL_FastString* details, int iMask)
{
	MediaTrack* tr = GuidToTrack(&m_guid);

	if (m_iTrackNum == 0)
	{
		details->Append(__LOCALIZE("Master Track","sws_DLG_101"));
	}
	else if (tr)
	{
		char* cName = (char*)GetSetMediaTrackInfo(tr, "P_NAME", NULL);
		int iNum = CSurf_TrackToID(tr, false);
		if (strcmp(cName, m_sName.Get()) == 0 && m_iTrackNum == iNum)
			details->AppendFormatted(100, __LOCALIZE_VERFMT("Track #%d \"%s\"","sws_DLG_101"), iNum, cName);
		else
			details->AppendFormatted(100, __LOCALIZE_VERFMT("Track #%d \"%s\", originally #%d \"%s\"","sws_DLG_101"), iNum, cName, m_iTrackNum, m_sName.Get());
	}
	else
		details->AppendFormatted(100, __LOCALIZE_VERFMT("Track #%d \"%s\" (not in current project!)","sws_DLG_101"), m_iTrackNum, m_sName.Get());
	details->Append("\r\n");

	if (iMask & VOL_MASK)
	{
		details->AppendFormatted(50, __LOCALIZE_VERFMT("Volume: %.2fdb","sws_DLG_101"), VAL2DB(m_dVol));
		details->Append("\r\n");
		if (m_sVolEnv.GetLength()) {
			details->Append(__LOCALIZE("Volume (Pre-FX) envelope","sws_DLG_101"));
			details->Append("\r\n");
		}
		if (m_sVolEnv2.GetLength()) {
			details->Append(__LOCALIZE("Volume envelope","sws_DLG_101"));
			details->Append("\r\n");
		}
	}
	if (iMask & PAN_MASK)
	{
		int iPanMode = m_iPanMode;
		if (iPanMode == -1)
			iPanMode = *ConfigVar<int>("panmode"); // display pan in project format

		if (iPanMode != 6)
		{
			if (m_dPan == 0.0)
				details->Append(__LOCALIZE("Pan: center","sws_DLG_101"));
			else
				details->AppendFormatted(50, __LOCALIZE_VERFMT("Pan: %d%% %s","sws_DLG_101"), abs((int)(m_dPan * 100.0)), m_dPan < 0.0 ? __LOCALIZE("left","sws_DLG_101") : __LOCALIZE("right","sws_DLG_101"));
		}
		if (iPanMode == 5) // stereo pan
		{
			details->Append(", ");
			details->AppendFormatted(50, __LOCALIZE_VERFMT("width %d%%","sws_DLG_101"), (int)(m_dPanWidth * 100.0));
		}
		else if (iPanMode == 6) // dual pan
			details->AppendFormatted(50, __LOCALIZE_VERFMT("Left pan: %d%%%s, Right pan: %d%%%s","sws_DLG_101"), abs((int)(m_dPanL * 100.0)), m_dPanL == 0.0 ? __LOCALIZE("C","sws_DLG_101") : (m_dPanL < 0.0 ? __LOCALIZE("L","sws_DLG_101") : __LOCALIZE("R","sws_DLG_101")), abs((int)(m_dPanR * 100.0)), m_dPanR == 0.0 ? __LOCALIZE("C","sws_DLG_101") : (m_dPanR < 0.0 ? __LOCALIZE("L","sws_DLG_101") : __LOCALIZE("R","sws_DLG_101")));

		if (m_dPanLaw == -1.0)
		{
			details->Append(", ");
			details->Append(__LOCALIZE("default pan law","sws_DLG_101"));
			details->Append("\r\n");
		}
		else if (m_dPanLaw != -100.0)
		{
			details->Append(", ");
			details->AppendFormatted(50, __LOCALIZE_VERFMT("Pan law %.4f","sws_DLG_101"), m_dPanLaw);
			details->Append("\r\n");
		}
		else
			details->Append("\r\n");

		if (m_sPanEnv.GetLength()) {
			details->Append(__LOCALIZE("Pan (Pre-FX) envelope","sws_DLG_101"));
			details->Append("\r\n");
		}
		if (m_sPanEnv2.GetLength()) {
			details->Append(__LOCALIZE("Pan envelope","sws_DLG_101"));
			details->Append("\r\n");
		}
		if (m_sWidthEnv.GetLength()) {
			details->Append(__LOCALIZE("Width (Pre-FX) envelope","sws_DLG_101"));
			details->Append("\r\n");
		}
		if (m_sWidthEnv2.GetLength()) {
			details->Append(__LOCALIZE("Width envelope","sws_DLG_101"));
			details->Append("\r\n");
		}
	}
	if (iMask & MUTE_MASK)
	{
		details->Append(__LOCALIZE("Mute","sws_DLG_101"));
		details->Append(": ");
		details->Append(m_bMute ? __LOCALIZE("on","sws_DLG_101") : __LOCALIZE("off","sws_DLG_101"));
		details->Append("\r\n");

		if (m_sMuteEnv.GetLength()) {
			details->Append(__LOCALIZE("Mute envelope","sws_DLG_101"));
			details->Append("\r\n");
		}
	}
	if (iMask & SOLO_MASK)
	{
		details->Append(__LOCALIZE("Solo","sws_DLG_101"));
		details->Append(": ");
		details->Append(m_iSolo ? __LOCALIZE("on","sws_DLG_101") : __LOCALIZE("off","sws_DLG_101"));
		details->Append("\r\n");
	}
	if (iMask & SEL_MASK)
	{
		details->Append(__LOCALIZE("Selected","sws_DLG_101"));
		details->Append(": ");
		details->Append(m_iSel ? __LOCALIZE("yes","sws_DLG_101") : __LOCALIZE("no","sws_DLG_101"));
		details->Append("\r\n");
	}
	if (iMask & VIS_MASK)
	{
		details->Append(__LOCALIZE("Visibility","sws_DLG_101"));
		details->Append(": ");
		switch (m_iVis)
		{
			case 0: details->Append(__LOCALIZE("invisible","sws_DLG_101")); break;
			case 1: details->Append(__LOCALIZE("MCP only","sws_DLG_101")); break;
			case 2: details->Append(__LOCALIZE("TCP only","sws_DLG_101")); break;
			case 3: details->Append(__LOCALIZE("full","sws_DLG_101")); break;
		}
		details->Append("\r\n");
	}
	if (iMask & FXCHAIN_MASK)
	{
		details->Append(__LOCALIZE("FX bypass","sws_DLG_101"));
		details->Append(": ");
		details->Append(m_iFXEn ? __LOCALIZE("on","sws_DLG_101") : __LOCALIZE("off","sws_DLG_101"));
		details->Append("\r\n");

		if (!m_sFXChain.GetSize())
		{
			details->Append(__LOCALIZE("Empty FX chain","sws_DLG_101"));
			details->Append("\r\n");
		}
		else
		{
			details->Append(__LOCALIZE("FX chain","sws_DLG_101"));
			details->Append(":\r\n");

			char line[4096];
			int pos = 0;
			LineParser lp(false);
			while (GetChunkLine(m_sFXChain.Get(), line, 4096, &pos, false))
			{
				if (!lp.parse(line) && lp.getnumtokens() >= 2)
				{
					bool foundfx = false;
					if (strncmp(lp.gettoken_str(0), "<VST", 4) == 0 || 
						strncmp(lp.gettoken_str(0), "<AU", 3) == 0 ||
						strncmp(lp.gettoken_str(0), "<DX", 3) == 0)
					{
						foundfx = true;
						details->AppendFormatted(50, "\t%s\r\n", lp.gettoken_str(1));
					}
					else if (strcmp(lp.gettoken_str(0), "<JS") == 0)
					{
						foundfx = true;
						details->AppendFormatted(50, "\tJS: %s\r\n", lp.gettoken_str(1));
					}
					if (foundfx)
						details->Append("\r\n");
				}
			}
		}
	}
	if (iMask & SENDS_MASK)
	{
		if (m_sends.m_hwSends.GetSize())
		{
			details->Append(__LOCALIZE("Hardware outputs","sws_DLG_101"));
			details->Append(":\r\n");

			LineParser lp(false);
			for (int i = 0; i < m_sends.m_hwSends.GetSize(); i++)
			{
				lp.parse(m_sends.m_hwSends.Get(i)->Get());
				details->AppendFormatted(50, "\t%s\r\n", GetOutputChannelName(lp.gettoken_int(1) & 0x3FF));
			}
		}
		if (m_sends.m_sends.GetSize())
		{
			details->Append(__LOCALIZE("Sends","sws_DLG_101"));
			details->Append(":\r\n");

			LineParser lp(false);
			for (int i = 0; i < m_sends.m_sends.GetSize(); i++)
			{
				MediaTrack* tr = GuidToTrack(m_sends.m_sends.Get(i)->GetGuid());
				details->Append("\t");
				if (tr)
				{
					char* cName = (char*)GetSetMediaTrackInfo(tr, "P_NAME", NULL);
					details->AppendFormatted(100, __LOCALIZE_VERFMT("To track #%d \"%s\"","sws_DLG_101"), CSurf_TrackToID(tr, false), cName);
				}
				else
					details->Append(__LOCALIZE("To unknown track!","sws_DLG_101"));
				details->Append("\r\n");
			}
		}
		else {
			details->Append(__LOCALIZE("No sends","sws_DLG_101"));
			details->Append("\r\n");
		}
	}
	if (iMask & PHASE_MASK)
	{
		if (m_iTrackNum != 0) // no phase switch on master
		{
			details->Append(__LOCALIZE("Phase", "sws_DLG_101"));
			details->Append(": ");
			details->Append(m_bPhase ? __LOCALIZE("inverted", "sws_DLG_101") : __LOCALIZE("normal", "sws_DLG_101"));
			details->Append("\r\n");
		}

	}
	if (iMask & PLAY_OFFSET_MASK)
	{
		details->Append(__LOCALIZE("Playback offset", "sws_DLG_101"));
		details->Append(": ");
		if (m_iPlayOffsetFlag & 1)
			details->Append(__LOCALIZE("bypassed, ", "sws_DLG_101"));

		if (m_iPlayOffsetFlag & 2)
			details->Append(__LOCALIZE("samples: ", "sws_DLG_101"));
		else 
			details->Append(__LOCALIZE("ms: ", "sws_DLG_101"));	
		// it's possible to type in (lots of) decimals in the playback offset textbox
		// would be cluttery in the snapshot details so truncate to 2 decimals
		std::stringstream ss;
		ss << std::fixed << std::setprecision(2) << (m_iPlayOffsetFlag & 2 ? m_dPlayOffset : m_dPlayOffset*1000);
		details->Append(ss.str().c_str());
	}
}

void TrackSnapshot::GetSetEnvelope(MediaTrack* tr, WDL_FastString* str, const char* env, bool bSet)
{
	TrackEnvelope* te = SWS_GetTrackEnvelopeByName(tr, env);
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
			GetSetEnvelopeState(te, (char*)str->Get(), 0);
		else
		{
			WDL_FastString state;
			state.Set(SWS_GetSetObjectState(tr, NULL));
			// Remove the last >
			const char* p = strrchr(state.Get(), '>');
			state.DeleteSub((int)(p - state.Get()), state.GetLength());
			SWS_GetSetObjectState(tr, &state);
		}
	}
}

bool TrackSnapshot::ProcessEnv(const char* chunk, char* line, int iLineMax, int* pos, const char* env, WDL_FastString* str)
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

Snapshot::Snapshot(int slot, int mask, bool bSelOnly, const char* name, const char* notes)
{
	m_iSlot = slot;
	m_iMask = mask;
	m_time = (int)time(NULL);
	m_cName = NULL;
	m_cNotes = NULL;
	SetName(name);
	SetNotes(notes);

	SWS_CacheObjectState(true);
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);

		if (!bSelOnly || *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			m_tracks.Add(new TrackSnapshot(tr, mask));
	}
	SWS_CacheObjectState(false);

	char undoStr[128];
	snprintf(undoStr, sizeof(undoStr), __LOCALIZE_VERFMT("Save snapshot %d","sws_undo"), slot);
	Undo_OnStateChangeEx(undoStr, UNDO_STATE_MISCCFG, -1);
	RegisterSnapshotSlot(slot);
}

// Build a snapshot from an XML/RPP chunk from the clipboard/RPP/undo/etc
Snapshot::Snapshot(const char* chunk)
{
	char line[4096];
	int pos = 0;
	LineParser lp(false);
	TrackSnapshot* ts = NULL;
	m_cName = NULL;
	m_cNotes = NULL;

	int auxEnvsOccurence = -1;

	while(GetChunkLine(chunk, line, 4096, &pos, false))
	{
		if (lp.parse(line))
			break;

		if (strcmp(lp.gettoken_str(0), "<SWSSNAPSHOT") == 0)
		{
			m_time = 0;
			if (lp.getnumtokens() == 6 && isdigit(lp.gettoken_str(5)[0])) // Old-style time, convert
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
			if (!isdigit(lp.gettoken_str(5)[0])) // Don't set the notes to old-format time
				SetNotes(lp.gettoken_str(5));
			else
				SetNotes("");
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
			else if (strcmp("AUXSEND", lp.gettoken_str(0)) == 0) {
				// Same format as AUXRECV but on the send track, with second param as recv GUID

				// NF: also get the AUXVOLENV etc. subchunks
				auxEnvsOccurence += 1;
				WDL_FastString chunkStr;
				chunkStr.Set(chunk);
				SNM_ChunkParserPatcher p(&chunkStr);
				WDL_FastString AUXVOL, AUXPAN, AUXMUTE;

				p.GetSubChunk("AUXVOLENV", 3, auxEnvsOccurence, &AUXVOL, "FXCHAIN");
				p.GetSubChunk("AUXPANENV", 3, auxEnvsOccurence, &AUXPAN, "FXCHAIN");
				p.GetSubChunk("AUXMUTEENV", 3, auxEnvsOccurence, &AUXMUTE, "FXCHAIN");

				ts->m_sends.m_sends.Add(new TrackSend(line, AUXVOL.Get(), AUXPAN.Get(), AUXMUTE.Get()));
			}

			else if (strcmp("HWOUT", lp.gettoken_str(0)) == 0)
				ts->m_sends.m_hwSends.Add(new WDL_FastString(line));
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
	RegisterSnapshotSlot(m_iSlot);
}

Snapshot::~Snapshot()
{
	delete [] m_cName;
	delete [] m_cNotes;
	m_tracks.Empty(true);
}

void DeleteTracksFromSnapshotOnError(WDL_PtrList<TrackSnapshot>& m_tracks)
{
	for (int i = 0; i < m_tracks.GetSize(); i++)
	{
		if (m_tracks.Get(i)->Cleanup())
		{
			m_tracks.Delete(i, true);
			i--;
		}
	}
}

bool Snapshot::UpdateReaper(int mask, bool bSelOnly, bool bHideNewVis)
{
	char str[256];
	int trackErr = 0, fxErr = 0;
	WDL_PtrList<TrackSendFix> sendFixes;

	PreventUIRefresh(1);

	// Do "non-chunk" stuff first
	for (int i = 0; i < m_tracks.GetSize(); i++)
		m_tracks.Get(i)->UpdateReaper(mask & m_iMask, bSelOnly, &fxErr, false, &sendFixes);

	// Then cache all ObjectState changes for the chunk updating
	SWS_CacheObjectState(true);
	for (int i = 0; i < m_tracks.GetSize(); i++)
		if (m_tracks.Get(i)->UpdateReaper(mask & m_iMask, bSelOnly, &fxErr, true, &sendFixes))
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
	}

	PreventUIRefresh(-1);

	snprintf(str, sizeof(str), __LOCALIZE_VERFMT("Load snapshot %s","sws_undo"), m_cName);
	Undo_OnStateChangeEx(str, UNDO_STATE_ALL, -1);

	if ((trackErr && SWS_SnapshotsWnd::GetPromptOnDeletedTracks()) || fxErr)
	{
		WDL_FastString errString;
		int n = 0;
		if (trackErr)
			errString.AppendFormatted(512, __LOCALIZE_VERFMT("%d track(s) from snapshot not found.","sws_DLG_101"), trackErr);
		if (fxErr)
			errString.AppendFormatted(512, __LOCALIZE_VERFMT("%s%d FX from snapshot not found.","sws_DLG_101"), n ? "\n" : "", fxErr);
		errString.AppendFormatted(512, "%s", __LOCALIZE("\nDelete abandonded items from snapshot? (You cannot undo this operation!)","sws_DLG_101"));
		if (MessageBox(g_hwndParent, errString.Get(), __LOCALIZE("Snapshot recall error","sws_DLG_101"), MB_YESNO) == IDYES)
		{
			DeleteTracksFromSnapshotOnError(m_tracks);
			return true;
		}
	}
	else if (trackErr) // prompt on recall deleted tracks disabled
	{
		DeleteTracksFromSnapshotOnError(m_tracks);
		return true;
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
		n = snprintf(str, maxLen, __LOCALIZE_VERFMT("Master + %d track(s)","sws_DLG_101"), m_tracks.GetSize() - 1);
	else
		n = snprintf(str + n, maxLen - n, __LOCALIZE_VERFMT("%d track(s)","sws_DLG_101"), m_tracks.GetSize());

	if (m_iMask & VOL_MASK && n < maxLen) {
		n += snprintf(str + n, maxLen - n, "%s", ", ");
		n += snprintf(str + n, maxLen - n, "%s", __LOCALIZE("vol","sws_DLG_101"));
	}
	if (m_iMask & PAN_MASK && n < maxLen) {
		n += snprintf(str + n, maxLen - n, "%s", ", ");
		n += snprintf(str + n, maxLen - n, "%s", __LOCALIZE("pan","sws_DLG_101"));
	}
	if (m_iMask & MUTE_MASK && n < maxLen) {
		n += snprintf(str + n, maxLen - n, "%s", ", ");
		n += snprintf(str + n, maxLen - n, "%s", __LOCALIZE("mute","sws_DLG_101"));
	}
	if (m_iMask & SOLO_MASK && n < maxLen) {
		n += snprintf(str + n, maxLen - n, "%s", ", ");
		n += snprintf(str + n, maxLen - n, "%s", __LOCALIZE("solo","sws_DLG_101"));
	}
	if (m_iMask & FXATM_MASK && n < maxLen) {
		n += snprintf(str + n, maxLen - n, "%s", ", ");
		n += snprintf(str + n, maxLen - n, "%s", __LOCALIZE("fx (old style)","sws_DLG_101"));
	}
	if (m_iMask & FXCHAIN_MASK && n < maxLen) {
		n += snprintf(str + n, maxLen - n, "%s", ", ");
		n += snprintf(str + n, maxLen - n, "%s", __LOCALIZE("fx","sws_DLG_101"));
	}
	if (m_iMask & SENDS_MASK && n < maxLen) {
		n += snprintf(str + n, maxLen - n, "%s", ", ");
		n += snprintf(str + n, maxLen - n, "%s", __LOCALIZE("sends","sws_DLG_101"));
	}
	if (m_iMask & VIS_MASK && n < maxLen) {
		n += snprintf(str + n, maxLen - n, "%s", ", ");
		n += snprintf(str + n, maxLen - n, "%s", __LOCALIZE("visibility","sws_DLG_101"));
	}
	if (m_iMask & SEL_MASK && n < maxLen) {
		n += snprintf(str + n, maxLen - n, "%s", ", ");
		n += snprintf(str + n, maxLen - n, "%s", __LOCALIZE("selection","sws_DLG_101"));
	}
	if (m_iMask & PHASE_MASK && n < maxLen) {
		n += snprintf(str + n, maxLen - n, "%s", ", ");
		n += snprintf(str + n, maxLen - n, "%s", __LOCALIZE("phase", "sws_DLG_101"));
	}
	if (m_iMask & PLAY_OFFSET_MASK && n < maxLen) {
		n += snprintf(str + n, maxLen - n, "%s", ", ");
		n += snprintf(str + n, maxLen - n, "%s", __LOCALIZE("playback offset", "sws_DLG_101"));
	}
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
			case VOL_MASK:         snprintf(newName, sizeof(newName), "%s %d", __LOCALIZE("Vol","sws_DLG_101"), m_iSlot);    break;
			case PAN_MASK:         snprintf(newName, sizeof(newName), "%s %d", __LOCALIZE("Pan","sws_DLG_101"), m_iSlot);    break;
			case MUTE_MASK:        snprintf(newName, sizeof(newName), "%s %d", __LOCALIZE("Mute","sws_DLG_101"), m_iSlot);   break;
			case SOLO_MASK:        snprintf(newName, sizeof(newName), "%s %d", __LOCALIZE("Solo","sws_DLG_101"), m_iSlot);   break;
			case FXATM_MASK: // fallthrough
			case FXCHAIN_MASK:     snprintf(newName, sizeof(newName), "%s %d", __LOCALIZE("FX","sws_DLG_101"), m_iSlot);     break;
			case SENDS_MASK:       snprintf(newName, sizeof(newName), "%s %d", __LOCALIZE("Sends","sws_DLG_101"), m_iSlot);  break;
			case VIS_MASK:         snprintf(newName, sizeof(newName), "%s %d", __LOCALIZE("Vis","sws_DLG_101"), m_iSlot);    break;
			case SEL_MASK:         snprintf(newName, sizeof(newName), "%s %d", __LOCALIZE("Sel","sws_DLG_101"), m_iSlot);    break;
			case PHASE_MASK:       snprintf(newName, sizeof(newName), "%s %d", __LOCALIZE("Phase", "sws_DLG_101"), m_iSlot); break;
			case PLAY_OFFSET_MASK: snprintf(newName, sizeof(newName), "%s %d", __LOCALIZE("Playback offset", "sws_DLG_101"), m_iSlot); break;
			default:               snprintf(newName, sizeof(newName), "%s %d", __LOCALIZE("Mix","sws_DLG_101"), m_iSlot);    break;
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

void Snapshot::SetNotes(const char* notes)
{
	delete [] m_cNotes;
	if (!notes)
		notes = "";
	m_cNotes = new char[strlen(notes)+1];
	strcpy(m_cNotes, notes);
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
void Snapshot::GetChunk(WDL_FastString* chunk)
{
	WDL_FastString notes;
	makeEscapedConfigString(m_cNotes, &notes);
	chunk->SetFormatted(SNM_MAX_CHUNK_LINE_LENGTH, "<SWSSNAPSHOT \"%s\" %d %d %d %s\n", m_cName, m_iSlot, m_iMask, m_time, notes.Get());
	for (int i = 0; i < m_tracks.GetSize(); i++)
		m_tracks.Get(i)->GetChunk(chunk);
	chunk->Append(">\n");
}

// Get a human-readable string that explains the snapshot
void Snapshot::GetDetails(WDL_FastString* details)
{
	char cTemp[100];
	details->AppendFormatted(100, __LOCALIZE_VERFMT("Snapshot %d \"%s\", stored","sws_DLG_101"), m_iSlot, m_cName);
	details->Append(" ");
	GetTimeString(cTemp, 100, true);
	details->Append(cTemp);
	details->Append(" ");
	GetTimeString(cTemp, 100, false);
	details->Append(cTemp);
	details->Append("\r\n");
	char cSummary[100];
	details->Append(Tooltip(cSummary, 100));
	details->Append("\r\n");
	details->Append("Notes: ");
	details->Append(m_cNotes);
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
