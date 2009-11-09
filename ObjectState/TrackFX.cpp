/******************************************************************************
/ TrackFX.cpp
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
#include "TrackFX.h"

// Functions for getting/setting track FX chains

TrackFX::TrackFX(MediaTrack* tr):m_pTr(tr),m_cTrackState(NULL),m_cFXString(NULL),m_cStateSuffix(NULL)
{
}

TrackFX::~TrackFX()
{
	// Keep the buffer around, maybe we'll add functions later that ref the same string
	FreeHeapPtr(m_cTrackState);
}

const char* TrackFX::GetFXString()
{
	UpdateState();
	return m_cFXString;
}

void TrackFX::SetFXString(const char* cFX)
{
	UpdateState();
	if (strcmp(cFX, m_cFXString) == 0)
		return;

	char* cNewState = new char[strlen(m_cTrackState) + strlen(cFX) + strlen(m_cStateSuffix) + 3];
	
	strcpy(cNewState, m_cTrackState);
	strcat(cNewState, "\n");
	strcat(cNewState, cFX);
	strcat(cNewState, "\n");
	strcat(cNewState, m_cStateSuffix);

	GetSetObjectState(m_pTr, cNewState);
	
	delete [] cNewState;
}

void TrackFX::UpdateState()
{
	FreeHeapPtr(m_cTrackState);
	m_cTrackState = GetSetObjectState(m_pTr, NULL);
	
	if (!m_cTrackState || !strlen(m_cTrackState))
	{
		m_cFXString = NULL;
		m_cStateSuffix = NULL;
		return;
	}

	// Parse out the string into the member vars
	m_cFXString = strstr(m_cTrackState, "<FXCHAIN");
	if (!m_cFXString)
	{
		m_cStateSuffix = strrchr(m_cTrackState, '>');
		m_cFXString = m_cStateSuffix - 1;
		*m_cFXString = 0;
		return;
	}
	else
		m_cFXString[-1] = 0;

	int iDepth = 1;
	char* pStr = m_cFXString + 8;
	while (iDepth)
	{
		if (*pStr == '<')
			iDepth++;
		else if (*pStr == '>')
			iDepth--;
		pStr++;
	}
	pStr[0] = 0;
	m_cStateSuffix = pStr + 1;
}