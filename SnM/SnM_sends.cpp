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

#define MAX_COPY_PASTE_SND_RCV 64

///////////////////////////////////////////////////////////////////////////////
// Cue bus 
///////////////////////////////////////////////////////////////////////////////

// Adds a receive (with vol & pan from source track for pre-fader)
// _srcTr:  source track (unchanged)
// _destTr: destination track
// _type:   reaper's type
//          0=Post-Fader (Post-Pan), 1=Pre-FX, 2=deprecated, 3=Pre-Fader (Post-FX)
bool addReceive(MediaTrack * _srcTr, MediaTrack * _destTr, int _type, SNM_SendPatcher* _p)
{
	bool update = false;
	// if pre-fader, then re-copy track vol/pan
	if (_type == 3)
	{
		char vol[32];
		char pan[32];
		SNM_ChunkParserPatcher p1(_srcTr, false);
		if (p1.Parse(SNM_GET_CHUNK_CHAR, 1, "TRACK", "VOLPAN", 4, 0, 1, vol) > 0 &&
			p1.Parse(SNM_GET_CHUNK_CHAR, 1, "TRACK", "VOLPAN", 4, 0, 2, pan) > 0)
		{
			update = (_p->AddReceive(_srcTr, _type, vol, pan) > 0); // null values managed
		}
	}

	// default (or failure case: retry)
	if (!update)
		update = (_p->AddReceive(_srcTr, _type) > 0);

	return update;
}

// Adds a cue bus track from track selection
// _busName
// _type:   reaper's type
//          0=Post-Fader (Post-Pan), 1=Pre-FX, 2=deprecated, 3=Pre-Fader (Post-FX)
// _undoMsg NULL=no undo
void cueTrack(char * _busName, int _type, const char * _undoMsg)
{
	bool updated = false;
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
				updated = true;
			}

			// add a send
			if (cueTr && p && tr != cueTr)
				updated |= addReceive(tr, cueTr, _type, p); 
		}
	}

	if (cueTr && p)
	{
		p->Commit();
		delete p;

		if (updated)
		{
			GetSetMediaTrackInfo(cueTr, "I_SELECTED", &g_i1);
			UpdateTimeline();

			// View I/O window for cue track
			Main_OnCommand(40293, 0);

			// Undo point
			if (_undoMsg)
				Undo_OnStateChangeEx(_undoMsg, UNDO_STATE_ALL, -1);
		}
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


///////////////////////////////////////////////////////////////////////////////
// Copy/paste with sends
///////////////////////////////////////////////////////////////////////////////

WDL_PtrList<t_SendRcv> g_sendClipboard[MAX_COPY_PASTE_SND_RCV];
WDL_PtrList<t_SendRcv> g_receiveClipboard[MAX_COPY_PASTE_SND_RCV];

MediaTrack* SNM_GuidToTrack(const char* _guid)
{
	if (_guid)
	{
		for (int i = 1; i <= GetNumTracks(); i++) //doesn't include master
		{
			MediaTrack* tr = CSurf_TrackFromID(i, false);
			char guid[64] = "";
			SNM_ChunkParserPatcher p(tr, false);
			if (p.Parse(SNM_GET_CHUNK_CHAR,1,"TRACK","TRACKID",-1,0,1,guid) > 0)
				if (!strcmp(_guid, guid))
					return tr;
		}
	}
	return NULL;
}

SNM_ChunkParserPatcher* FindTrackCPPbyGUID(
	WDL_PtrList<SNM_ChunkParserPatcher>* _list, const char* _guid)
{
	if (_list && _guid)
	{
		for (int i=0; i < _list->GetSize(); i++)
		{
			MediaTrack* tr = (MediaTrack*)_list->Get(i)->GetObject();
			char guid[64] = "";
			if (_list->Get(i)->Parse(SNM_GET_CHUNK_CHAR,1,"TRACK","TRACKID",-1,0,1,guid) > 0)
				if (!strcmp(_guid, guid))
					return _list->Get(i);
//			if (GuidsEqual(GetTrackGUID((MediaTrack*)_list->Get(i)->GetObject()), _guid))
//				return _list->Get(i);
		}
	}
	return NULL;
}

bool FillIOFromReaper(t_SendRcv* send, MediaTrack* src, MediaTrack* dest, int categ, int idx)
{
	if (send && src && dest)
	{
		MediaTrack* tr = (categ == -1 ? dest : src);
		SNM_ChunkParserPatcher p1(src, false);
		p1.Parse(SNM_GET_CHUNK_CHAR,1,"TRACK","TRACKID",-1,0,1,send->srcGUID);
		SNM_ChunkParserPatcher p2(dest, false);
		p2.Parse(SNM_GET_CHUNK_CHAR,1,"TRACK","TRACKID",-1,0,1,send->destGUID);
//		send->srcGUID = GetTrackGUID(src);
//		send->destGUID = GetTrackGUID(dest);
		send->mute = *(bool*)GetSetTrackSendInfo(tr, categ, idx, "B_MUTE", NULL);
		send->phase = *(int*)GetSetTrackSendInfo(tr, categ, idx, "B_PHASE", NULL);
		send->mono = *(int*)GetSetTrackSendInfo(tr, categ, idx, "B_MONO", NULL);
		send->vol = *(double*)GetSetTrackSendInfo(tr, categ, idx, "D_VOL", NULL);
		send->pan = *(double*)GetSetTrackSendInfo(tr, categ, idx, "D_PAN", NULL);
		send->panl = *(double*)GetSetTrackSendInfo(tr, categ, idx, "D_PANLAW", NULL);
		send->mode = *(int*)GetSetTrackSendInfo(tr, categ, idx, "I_SENDMODE", NULL);
		send->srcChan = *(int*)GetSetTrackSendInfo(tr, categ, idx, "I_SRCCHAN", NULL);
		send->destChan = *(int*)GetSetTrackSendInfo(tr, categ, idx, "I_DSTCHAN", NULL);
		send->midi = *(int*)GetSetTrackSendInfo(tr, categ, idx, "I_MIDIFLAGS", NULL);
		return true;
	}
	return false;
}

void storeSendsReceives()
{
	// Clear the "clipboard"
	for (int j=0; j < MAX_COPY_PASTE_SND_RCV; j++)
	{
		g_sendClipboard[j].Empty(true, free);
		g_receiveClipboard[j].Empty(true, free);
	}

	int selTrackIdx = 0;
	for (int i = 1; i <= GetNumTracks() && //doesn't include master
		selTrackIdx < MAX_COPY_PASTE_SND_RCV; i++) 
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			// *** Copy sends ***
			int idx=0;
			MediaTrack* dest = (MediaTrack*)GetSetTrackSendInfo(tr, 0, idx, "P_DESTTRACK", NULL);
			while (dest)
			{
				// We do not copy cross-copy/pasted tracks' sends 
				// (not to duplicate with the following receives re-copy)
//				if (!(*(int*)GetSetMediaTrackInfo(dest, "I_SELECTED", NULL))) //not sel!
				{
					t_SendRcv* send = (t_SendRcv*)malloc(sizeof(t_SendRcv));
					if (FillIOFromReaper(send, tr, dest, 0, idx))
						g_sendClipboard[selTrackIdx].Add(send);
				}
				idx++;
				dest = (MediaTrack*)GetSetTrackSendInfo(tr, 0, idx, "P_DESTTRACK", NULL);
			}

			// *** Copy receives ***
			idx=0;
			MediaTrack* src = (MediaTrack*)GetSetTrackSendInfo(tr, -1, idx, "P_SRCTRACK", NULL);
			while (src)
			{
				t_SendRcv* rcv = (t_SendRcv*)malloc(sizeof(t_SendRcv));
				if (FillIOFromReaper(rcv, src, tr, -1, idx))
					g_receiveClipboard[selTrackIdx].Add(rcv);
				idx++;
				src = (MediaTrack*)GetSetTrackSendInfo(tr, -1, idx, "P_SRCTRACK", NULL);
			}

			selTrackIdx++;
		}
	}
}

void copyWithIOs(COMMAND_T* _ct)
{
	storeSendsReceives();
	Main_OnCommand(40210, 0); // Copy sel tracks
}

void cutWithIOs(COMMAND_T* _ct)
{
	storeSendsReceives();
	Main_OnCommand(40337, 0); // Cut sel tracks
}

// we do not track updates here 'cause we use an undo block
void pasteWithIOs(COMMAND_T* _ct)
{
	Undo_BeginBlock();

	int iTracks = GetNumTracks();

	// native paste (depends on context)
	Main_OnCommand(40058, 0);

	if (iTracks != GetNumTracks()) // See if tracks were pasted
	{
		// As a same track can be multi-patched
		// => we'll patch tracks in one go thanks to this list
		WDL_PtrList<SNM_ChunkParserPatcher> ps;

		// 1st loop not to remove the receives (can't be done in the 
		// 2nd one: could remove those we're currently adding)
		int selTrackIdx = 0;
		for (int i = 1; i <= GetNumTracks() &&  //doesn't include master
			selTrackIdx < MAX_COPY_PASTE_SND_RCV; i++)
		{
			MediaTrack* tr = CSurf_TrackFromID(i, false);
			if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			{
				SNM_SendPatcher* p = new SNM_SendPatcher(tr); 
				ps.Add(p);
				p->RemoveReceives();
				selTrackIdx++;
			}
		}

		selTrackIdx = 0;
		for (int i = 1; i <= GetNumTracks() &&  //doesn't include master
			selTrackIdx < MAX_COPY_PASTE_SND_RCV; i++)
		{
			MediaTrack* tr = CSurf_TrackFromID(i, false);
			if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			{
				// *** Paste sends ***
				for (int j=0; j < g_sendClipboard[selTrackIdx].GetSize(); j++)
				{
					t_SendRcv* send = g_sendClipboard[selTrackIdx].Get(j);
					MediaTrack* sendDest = SNM_GuidToTrack(send->destGUID);
					if (sendDest) 
					{
						SNM_SendPatcher* pRcv = (SNM_SendPatcher*)FindTrackCPPbyGUID(&ps, send->destGUID);
						if (!pRcv)
						{
							pRcv = new SNM_SendPatcher(sendDest); 
							ps.Add(pRcv);
						}
						pRcv->AddReceive(tr, send);
					}
				}

				// *** Paste receives ***
				for (int j=0; j < g_receiveClipboard[selTrackIdx].GetSize(); j++)
				{
					t_SendRcv* rcv = g_receiveClipboard[selTrackIdx].Get(j);
					MediaTrack* rcvSrc = SNM_GuidToTrack(rcv->srcGUID);
					if (rcvSrc) 
						((SNM_SendPatcher*)ps.Get(selTrackIdx))->AddReceive(rcvSrc, rcv);
				}

				selTrackIdx++;
			}
		}
		ps.Empty(true); // + auto commit all chunks!
	}
	Undo_EndBlock(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL);
}


///////////////////////////////////////////////////////////////////////////////
// Common
///////////////////////////////////////////////////////////////////////////////

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
