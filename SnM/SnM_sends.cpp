/******************************************************************************
/ SnM_Sends.cpp
/
/ Copyright (c) 2009-2010 Tim Payne (SWS), JF Bédague
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
#include "SnM_Actions.h"
#include "SNM_Chunk.h"


// Adds a send
// _srcTr:  source track (unchanged)
// _destTr: destination track
// _type:   reaper's type
//          0=Post-Fader (Post-Pan), 1=Pre-FX, 2=deprecated, 3=Pre-Fader (Post-FX)
bool addSend(MediaTrack * _srcTr, MediaTrack * _destTr, int _type, SNM_SendPatcher* _p)
{
	bool update = false;
	char vol[32] = "1.00000000000000";
	char pan[32] = "0.00000000000000";
	{
		// if pre-fader, then re-copy track vol/pan
		if (_type == 3)
		{
			//e.g VOLPAN 1.00000000000000 -0.45000000000000 -1.00000000000000
			SNM_ChunkParserPatcher p1(_srcTr, false);
			if (!(p1.Parse(SNM_GET_CHUNK_CHAR, 1, "TRACK", "VOLPAN", 4, 0, 1, vol) > 0 &&
				p1.Parse(SNM_GET_CHUNK_CHAR, 1, "TRACK", "VOLPAN", 4, 0, 2, pan) > 0))
			{
				// failed => restore default
				strcpy(vol, "1.00000000000000");
				strcpy(pan, "0.00000000000000");
			}
		}
		update = (_p->AddSend(_srcTr, _type, vol, pan) > 0); // null values managed
	} 
	return update;
}

// Adds a cue bus track from track selection
// _busName
// _type:   reaper's type
//          0=Post-Fader (Post-Pan), 1=Pre-FX, 2=deprecated, 3=Pre-Fader (Post-FX)
// _undoMsg NULL=no undo
void cueTrack(char * _busName, int _type, const char * _undoMsg)
{
	MediaTrack * cueTr = NULL;
	SNM_SendPatcher* p = NULL;
	for (int i = 1; i <= GetNumTracks(); i++) //doesn't include master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i0);

			// Add the cue track (if not already done)
			if (!cueTr)
			{
				InsertTrackAtIndex(GetNumTracks(), false);
				TrackList_AdjustWindows(false);
				cueTr = CSurf_TrackFromID(GetNumTracks(), false);
				GetSetMediaTrackInfo(cueTr, "P_NAME", _busName);
				p = new SNM_SendPatcher(cueTr);
			}

			// add a send
			if (cueTr && p && tr != cueTr)
				addSend(tr, cueTr, _type, p); //JFB: returned error not managed yet..
		}
	}

	if (cueTr && p)
	{
		p->Commit();
		delete p;

		GetSetMediaTrackInfo(cueTr, "I_SELECTED", &g_i1);
		UpdateTimeline();

		// View I/O window for cue track
		int iCommand = 40293;
		Main_OnCommand(iCommand, 0);

		// Undo point
		if (_undoMsg)
			Undo_OnStateChangeEx(_undoMsg, UNDO_STATE_ALL, -1);

	}
}

// Returns true for input error, false for good
bool cueTrackPrompt(const char* cCaption)
{
	char reply[128]= ",2"; // default bus name and user type
	if (GetUserInputs && GetUserInputs(cCaption, 2, "Bus name:,Send type (1 to 3. Other=help):", reply, 128))
	{
		char* pComma = strrchr(reply, ',');
		if (pComma)
		{
			int userType = pComma[1] - 0x30;
			int reaType = -1;
			switch(userType)
			{
				case 1: reaType=0; break;
				case 2: reaType=3; break;
				case 3: reaType=1; break;
			}
			if (reaType == -1 || pComma[2])
			{
				MessageBox(GetMainHwnd(), 
					"Valid send types:\n1=Post-Fader (Post-Pan)\n2=Pre-Fader (Post-FX)\n3=Pre-FX", 
					cCaption, /*MB_ICONERROR | */MB_OK);
				return true;
			}

			pComma[0] = 0;
			cueTrack(reply, reaType, cCaption);
		}
	}
	return false;
}

void cueTrackPrompt(COMMAND_T* _ct) {
	while (cueTrackPrompt(SNM_CMD_SHORTNAME(_ct)));
}

void cueTrack(COMMAND_T* _ct) {
	cueTrack("Cue Bus", (int)_ct->user, SNM_CMD_SHORTNAME(_ct));
}

void removeReceives(COMMAND_T* _ct)
{
	bool updated = false;
	for (int i = 1; i <= GetNumTracks(); i++) //doesn't include master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			SNM_SendPatcher p(tr); 
			updated |= (p.RemoveReceives() > 0); 
		}
	}
	// Undo point
	if (updated)
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}
