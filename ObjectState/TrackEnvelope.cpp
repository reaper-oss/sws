/******************************************************************************
/ TrackEnvelope.cpp
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
#include "TrackEnvelope.h"

TrackEnvelopes::TrackEnvelopes():m_bLoaded(false),m_pTr(NULL)
{
}

TrackEnvelopes::~TrackEnvelopes()
{
	m_envelopes.Empty(true);
	m_bLoaded = false;
	m_pTr = NULL;
}

void TrackEnvelopes::Load()
{
	if (!GetSetObjectState || !FreeHeapPtr || !m_pTr)
		return;

	m_envelopes.Empty(true);

	char* cData = GetSetObjectState(m_pTr, NULL);
	WDL_String curLine;
	char* pEOL = cData-1;
	TrackEnvelope* te = NULL;
	int iDepth = 0;
	do
	{
		char* pLine = pEOL+1;
		pEOL = strchr(pLine, '\n');
		curLine.Set(pLine, (int)(pEOL ? pEOL-pLine : 0));

		LineParser lp(false);
		lp.parse(curLine.Get());
		if (lp.getnumtokens())
		{
			if (lp.gettoken_str(0)[0] == '<')
				iDepth++;
			else if (lp.gettoken_str(0)[0] == '>')
				iDepth--;
		}
		if (!te && iDepth == 2 &&
		   (strcmp(lp.gettoken_str(0), "<VOLENV") == 0 ||
			strcmp(lp.gettoken_str(0), "<VOLENV2") == 0 ||
			strcmp(lp.gettoken_str(0), "<PANENV") == 0 ||
			strcmp(lp.gettoken_str(0), "<PANENV2") == 0 ||
			strcmp(lp.gettoken_str(0), "<MUTEENV") == 0 ||
			strcmp(lp.gettoken_str(0), "<PARMENV") == 0))
		{
			te = m_envelopes.Add(new TrackEnvelope);
		}
		else if (te && lp.getnumtokens() == 4 && strcmp(lp.gettoken_str(0), "VIS") == 0)
		{
			if (lp.gettoken_int(1) == 1 && lp.gettoken_int(2) == 1)
				te->m_dHeight = lp.gettoken_float(3);
			else
				te->m_dHeight = 0.0;
			te = NULL;		
		}
	}
	while (pEOL);

	FreeHeapPtr(cData);
	m_bLoaded = true;
}

void TrackEnvelopes::Save()
{
}

int TrackEnvelopes::GetLanesHeight(int iTrackHeight)
{
	if (GetTrackVis(m_pTr) & 2)
	{
		if (!m_bLoaded)
			Load();
		int iHeight = 0;
		for (int i = 0; i < m_envelopes.GetSize(); i++)
		{
			int iLaneHeight = (int)(m_envelopes.Get(i)->m_dHeight * iTrackHeight);
			if (m_envelopes.Get(i)->m_dHeight != 0.0 && iLaneHeight < MINTRACKHEIGHT)
				iLaneHeight += MINTRACKHEIGHT;
			iHeight += iLaneHeight;
		}
		return iHeight;
	}
	else
		return 0;
}