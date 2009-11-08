/******************************************************************************
/ SnM_sends.cpp
/
/ Copyright (c) 2009 Tim Payne (SWS), JF Bédague (S&M)
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


WDL_String * m_sendout = new WDL_String();

bool addSend(MediaTrack * _srcTr, MediaTrack * _destTr, int _type)
{
	bool ok = false;

	int srcId = CSurf_TrackToID(_srcTr, false); // for (usefull?) check
	if (_srcTr && _destTr && _type && srcId >= 0)
	{
		char* cData = GetSetObjectState(_destTr, NULL);
		if (cData)
		{
			WDL_String curLine;
			m_sendout->Set("");
			char* pEOL = cData-1;
			int iDepth = 0;
			int rcvId = 0;
			do
			{
				char* pLine = pEOL+1;
				pEOL = strchr(pLine, '\n');
				curLine.Set(pLine, (int)(pEOL ? pEOL-pLine : 0));

				LineParser lp(false);
				lp.parse(curLine.Get());

				// Get the depth
				if (lp.getnumtokens())
				{
					if (lp.gettoken_str(0)[0] == '<')
						iDepth++;
					else if (lp.gettoken_str(0)[0] == '>')
						iDepth--;
				}

				// Append with some checks for "more acsendant compatibility"
				if (iDepth == 1 && strcmp(lp.gettoken_str(0), "AUXRECV") == 0)
				{
					rcvId++;
				}

				if (iDepth == 1 && lp.getnumtokens() == 2 && 
					strcmp(lp.gettoken_str(0), "MIDIOUT") == 0)
				{
					m_sendout->Append("AUXRECV ");
					m_sendout->AppendFormatted(2, "%d ", srcId-1);
					m_sendout->Append("0 1.00000000000000 0.00000000000000 0 0 0 0 0 -1.00000000000000 0 -1\n");
				}

				if (lp.getnumtokens())
				{
					m_sendout->Append(curLine.Get());
					m_sendout->Append("\n");
				}
			}
			while (pEOL);

			FreeHeapPtr(cData);

			// Sets the new state
			if (m_sendout->GetLength())
			{
				if (GetSetObjectState(_destTr, m_sendout->Get()) == 0)
				{
					// I DO THAT WAY TO ALSO FORCE REFRESH IN REAPER!!!
					GetSetTrackSendInfo(_destTr, -1, rcvId, "I_SENDMODE" , &_type);
					ok = true;
				}
			}
		}
	}
	return ok;
}

void cueTrack(COMMAND_T* _ct)
{
	MediaTrack * cueTr = NULL;
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i0);
			UpdateTimeline();

			// Add the cue track (if not already done)
			if (!cueTr)
			{
				InsertTrackAtIndex(GetNumTracks(), false);
				TrackList_AdjustWindows(false);
				cueTr = CSurf_TrackFromID(GetNumTracks(), false);
				GetSetMediaTrackInfo(cueTr, "P_NAME", "Cue Bus");
				UpdateTimeline();
			}

			// add a Pre-Fader-Post-FX send
			if (cueTr && tr != cueTr)
			{
				addSend(tr, cueTr, 3); // returned error not managed yet..
			}
		}
	}
	if (cueTr)
	{
		GetSetMediaTrackInfo(cueTr, "I_SELECTED", &g_i1);
		UpdateTimeline();

		// View I/O window for cue track
		int iCommand = 40293;
		Main_OnCommand(iCommand, 0);

		// Undo point
		Undo_OnStateChangeEx("Cuetrack from track selection", UNDO_STATE_TRACKCFG, -1);
	}
}
