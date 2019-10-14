/******************************************************************************
/ TrackEnvelope.cpp
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


#include "stdafx.h"
#include "TrackEnvelope.h"

#ifdef SWS_DEPRECEATED

SWS_TrackEnvelope::SWS_TrackEnvelope(TrackEnvelope* te):m_pTe(te), m_bLoaded(false), m_iHeightOverride(0), m_cEnv(NULL), m_bVis(false)
{
}

SWS_TrackEnvelope::~SWS_TrackEnvelope()
{
	free(m_cEnv);
}

void SWS_TrackEnvelope::Load()
{
	if (!m_pTe)
		return;
	free(m_cEnv);
	m_cEnv = NULL;
	int iStrSize = 256;
	while(true)
	{
		m_cEnv = (char*)realloc(m_cEnv, iStrSize);
		m_cEnv[0] = 0;
		bool bRet = GetSetEnvelopeState(m_pTe, m_cEnv, iStrSize);

		if (bRet && strlen(m_cEnv) != iStrSize-1)
			break;

		if (!bRet || iStrSize >= ENV_MAX_SIZE)
		{
			free(m_cEnv);
			m_cEnv = NULL;
			return;
		}
			
		iStrSize *= 2;
	}

	// Add types of wanted information here
	char* cLaneHeight = strstr(m_cEnv, "LANEHEIGHT ");
	if (cLaneHeight)
		m_iHeightOverride = atol(cLaneHeight + 11);
	else
		m_iHeightOverride = 0;
	char* cVis = strstr(m_cEnv, "VIS ");
	if (cVis)
		m_bVis = cVis[4] == '1';

	m_bLoaded = true;
}

int SWS_TrackEnvelope::GetHeight(int iTrackHeight)
{
	if (!m_bLoaded)
		Load();

	if (!m_bVis && iTrackHeight)
		return 0;
	else if (m_iHeightOverride || iTrackHeight == 0)
		return m_iHeightOverride;
	else
	{
		iTrackHeight = (int)(iTrackHeight * ENV_HEIGHT_MULTIPLIER);
		if (iTrackHeight < SNM_GetIconTheme()->envcp_min_height)
			iTrackHeight =  SNM_GetIconTheme()->envcp_min_height;
		return iTrackHeight;
	}
}

void SWS_TrackEnvelope::SetHeight(int iHeight)
{
	if (!m_bLoaded)
		Load();
	if (!m_pTe || !m_cEnv || !m_bVis || m_iHeightOverride == iHeight)
		return;

	char* pLH = strstr(m_cEnv, "LANEHEIGHT");
	if (!pLH)
		return;

	char* cOldState = m_cEnv;
	m_cEnv = (char*)malloc(strlen(cOldState) + 10);
	lstrcpyn(m_cEnv, cOldState, (int)(pLH-cOldState + 12));
	sprintf(m_cEnv+strlen(m_cEnv), "%d", iHeight);
	strcpy(m_cEnv+strlen(m_cEnv), strchr(pLH+11, ' '));
	GetSetEnvelopeState(m_pTe, m_cEnv, (int)strlen(m_cEnv));
	free(cOldState);
}

bool SWS_TrackEnvelope::GetVis()
{
	if (!m_bLoaded)
		Load();
	return m_bVis;
}

void SWS_TrackEnvelope::SetVis(bool bVis)
{
	if (!m_bLoaded)
		Load();
	if (!m_pTe || !m_cEnv || m_bVis == bVis)
		return;

	char* pLH = strstr(m_cEnv, "VIS ");
	if (!pLH)
		return;

	pLH[4] = bVis ? '1' : '0';
	GetSetEnvelopeState(m_pTe, m_cEnv, (int)strlen(m_cEnv));
}

SWS_TrackEnvelopes::SWS_TrackEnvelopes():m_pTr(NULL)
{
}

SWS_TrackEnvelopes::~SWS_TrackEnvelopes()
{
}

int SWS_TrackEnvelopes::GetLanesHeight(int iTrackHeight)
{
	int iHeight = 0;

	if (m_pTr && GetTrackVis(m_pTr) & 2)
	{
		for (int i = 0; i < CountTrackEnvelopes(m_pTr); i++)
		{
			SWS_TrackEnvelope te(GetTrackEnvelope(m_pTr, i));
			iHeight += te.GetHeight(iTrackHeight);
		}
	}
	return iHeight;
}

#endif
