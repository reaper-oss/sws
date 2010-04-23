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


///////////////////////////////////////////////////////////////////////////////
// Cue bus 
///////////////////////////////////////////////////////////////////////////////

// Adds a receive (with vol & pan from source track for pre-fader)
// _srcTr:  source track (unchanged)
// _destTr: destination track
// _type:   reaper's type
//          0=Post-Fader (Post-Pan), 1=Pre-FX, 2=deprecated, 3=Pre-Fader (Post-FX)
bool addReceiveWithVolPan(MediaTrack * _srcTr, MediaTrack * _destTr, int _type, SNM_SendPatcher* _p)
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
bool cueTrack(char * _busName, int _type, const char * _undoMsg, 
			  bool _showRouting, int _soloGrp, 
			  WDL_String* _chunk, 
			  bool _sendToMaster, int* _hwOuts) //optional prms
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
				if (_chunk)
				{
					p->SetChunk(_chunk, 1);
					p->RemoveIds(); // *HOT*
				}
				updated = true;
			}

			// add a send
			if (cueTr && p && tr != cueTr)
			{
				int z = addReceiveWithVolPan(tr, cueTr, _type, p); 
				SNM_ChunkParserPatcher pSrc(tr);
				updated |= (addSoloToGroup(cueTr, _soloGrp, true, &pSrc) > 0); // nop if invalid prms
			}
		}
	}

	if (cueTr && p)
	{
		// add slave solo to track grouping
		updated |= (addSoloToGroup(cueTr, _soloGrp, false, p) > 0); // nop if invalid prms

		// send to master/parent init
		if (!_chunk)
		{
			WDL_String mainSend("MAINSEND 1");
			if (!_sendToMaster)
				 mainSend.Set("MAINSEND 0");

			// adds HW outputs
			if (_hwOuts)
			{
				int monoHWCount=0; 
				while (GetOutputChannelName(monoHWCount)) monoHWCount++;

				bool cr = false;			
				for(int i=0; i<SNM_MAX_HW_OUTS; i++)
				{
					if (_hwOuts[i])
					{
						if (!cr) {mainSend.Append("\n"); cr = true;};
						if (_hwOuts[i] >= (monoHWCount)) 
							mainSend.AppendFormatted(32, "HWOUT %d ", (_hwOuts[i]-monoHWCount) | 1024);
						else
							mainSend.AppendFormatted(32, "HWOUT %d ", _hwOuts[i]-1);
						mainSend.Append("0 1.00000000000000 0.00000000000000 0 0 0 -1.00000000000000 -1\n");
					}
				}
				if (!cr) mainSend.Append("\n"); // hot
			}

			// patch both together
			updated |= p->ReplaceLine("TRACK", "MAINSEND", 1, 0, mainSend.Get());
		}

		p->Commit();
		delete p;

		if (updated)
		{
			GetSetMediaTrackInfo(cueTr, "I_SELECTED", &g_i1);
			UpdateTimeline();
//			TrackList_AdjustWindows(false); // for io buttons, etc (but KO right now..)
			if (_showRouting) Main_OnCommand(40293, 0);

			// Undo point
			if (_undoMsg)
				Undo_OnStateChangeEx(_undoMsg, UNDO_STATE_ALL, -1);
		}
	}
	return updated;
}

void cueTrackPrompt(COMMAND_T* _ct) {
	static HWND hwnd = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_SNM_CUEBUS), g_hwndParent, CueBusDlgProc);
	ShowWindow(hwnd, SW_SHOW);
	SetFocus(hwnd);
}

void cueTrack(COMMAND_T* _ct) {
	cueTrack("Cue Bus", (int)_ct->user, SNM_CMD_SHORTNAME(_ct));
}


///////////////////////////////////////////////////////////////////////////////
// Cut/Copy/Paste: track with sends, routings
///////////////////////////////////////////////////////////////////////////////

WDL_PtrList<WDL_PtrList<t_SendRcv>> g_sndTrackClipboard; 
WDL_PtrList<WDL_PtrList<t_SendRcv>> g_rcvTrackClipboard; 
WDL_PtrList<WDL_PtrList<t_SendRcv>> g_sndClipboard;
WDL_PtrList<WDL_PtrList<t_SendRcv>> g_rcvClipboard;

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

void copySendsReceives(bool _cut, 
		WDL_PtrList<WDL_PtrList<t_SendRcv>>* _sends, 
		WDL_PtrList<WDL_PtrList<t_SendRcv>>* _rcvs)
{
	// Clear the "clipboards"
	if (_sends)	{
		for (int j=0; j < _sends->GetSize(); j++)
			_sends->Get(j)->Empty(true, free);
		_sends->Empty(true);
	}

	if (_rcvs) {
		for (int j=0; j < _rcvs->GetSize(); j++)
			_rcvs->Get(j)->Empty(true, free);
		_rcvs->Empty(true);
	}

	int selTrackIdx = 0;
	for (int i = 1; i <= GetNumTracks(); i++) //doesn't include master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			// *** Copy sends ***
			if (_sends)
			{
				_sends->Add(new WDL_PtrList<t_SendRcv>());

				int idx=0;
				MediaTrack* dest = (MediaTrack*)GetSetTrackSendInfo(tr, 0, idx, "P_DESTTRACK", NULL);
				while (dest)
				{
					// We do not store cross-cut/pasted tracks' sends ('cause they'll keep the same track id after paste)
					// not to duplicate with the following receives re-copy
					if (!_cut ||
						(_cut && !(*(int*)GetSetMediaTrackInfo(dest, "I_SELECTED", NULL))))
					{
						t_SendRcv* send = (t_SendRcv*)malloc(sizeof(t_SendRcv));
						if (FillIOFromReaper(send, tr, dest, 0, idx))
							_sends->Get(selTrackIdx)->Add(send);
					}
					dest = (MediaTrack*)GetSetTrackSendInfo(tr, 0, ++idx, "P_DESTTRACK", NULL);
				}
			}

			// *** Copy receives ***
			if (_rcvs)
			{
				_rcvs->Add(new WDL_PtrList<t_SendRcv>());

				int idx=0;
				MediaTrack* src = (MediaTrack*)GetSetTrackSendInfo(tr, -1, idx, "P_SRCTRACK", NULL);
				while (src)
				{
					t_SendRcv* rcv = (t_SendRcv*)malloc(sizeof(t_SendRcv));
					if (FillIOFromReaper(rcv, src, tr, -1, idx))
						_rcvs->Get(selTrackIdx)->Add(rcv);
					src = (MediaTrack*)GetSetTrackSendInfo(tr, -1, ++idx, "P_SRCTRACK", NULL);
				}
			}
			selTrackIdx++;
		}
	}
}

// Paste stored sends and/or receives to the selected tracks
bool pasteSendsReceives(WDL_PtrList<WDL_PtrList<t_SendRcv>>* _sends, 
		WDL_PtrList<WDL_PtrList<t_SendRcv>>* _rcvs,
		bool _rcvReset)
{
	bool updated = false;

	// As a same track can be multi-patched
	// => we'll patch all tracks in one go thanks to this list
	WDL_PtrList<SNM_ChunkParserPatcher> ps;

	// 1st loop not to remove the receives (can't be done in the 
	// 2nd one: could remove those we're currently adding)
	int selTrackIdx = 0;
	for (int i = 1; i <= GetNumTracks(); i++)  //doesn't include master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			SNM_SendPatcher* p = new SNM_SendPatcher(tr); 
			ps.Add(p);
			if (_rcvReset)
				updated |= (p->RemoveReceives() > 0);
			selTrackIdx++;
		}
	}

	selTrackIdx = 0;
	for (int i = 1; i <= GetNumTracks(); i++)  //doesn't include master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			// *** Paste sends ***
			if (_sends)
			{
				for (int j=0; selTrackIdx < _sends->GetSize() && j < _sends->Get(selTrackIdx)->GetSize(); j++)
				{
					t_SendRcv* send = _sends->Get(selTrackIdx)->Get(j);
					MediaTrack* sendDest = SNM_GuidToTrack(send->destGUID);
					if (sendDest) 
					{
						SNM_SendPatcher* pRcv = (SNM_SendPatcher*)FindTrackCPPbyGUID(&ps, send->destGUID);
						if (!pRcv)
						{
							pRcv = new SNM_SendPatcher(sendDest); 
							ps.Add(pRcv);
						}
						updated |= (pRcv->AddReceive(tr, send) > 0);
					}
				}
			}

			// *** Paste receives ***
			if (_rcvs)
			{
				for (int j=0; selTrackIdx < _rcvs->GetSize() && j < _rcvs->Get(selTrackIdx)->GetSize(); j++)
				{
					t_SendRcv* rcv = _rcvs->Get(selTrackIdx)->Get(j);
					MediaTrack* rcvSrc = SNM_GuidToTrack(rcv->srcGUID);
					if (rcvSrc) 
						updated |= (((SNM_SendPatcher*)ps.Get(selTrackIdx))->AddReceive(rcvSrc, rcv) > 0);
				}
			}

			selTrackIdx++;
		}
	}
	ps.Empty(true); // + auto commit all chunks (if something to commit) !
	return updated;
}

void copyWithIOs(COMMAND_T* _ct) {
	copySendsReceives(false, &g_sndTrackClipboard, &g_rcvTrackClipboard);
	Main_OnCommand(40210, 0); // Copy sel tracks
}

void cutWithIOs(COMMAND_T* _ct) {
	copySendsReceives(true, &g_sndTrackClipboard, &g_rcvTrackClipboard);
	Main_OnCommand(40337, 0); // Cut sel tracks
}

void pasteWithIOs(COMMAND_T* _ct) {
	Undo_BeginBlock();
	int iTracks = GetNumTracks();	
	Main_OnCommand(40058, 0); // native track paste
	if (iTracks != GetNumTracks()) // See if tracks were pasted
		pasteSendsReceives(&g_sndTrackClipboard, &g_rcvTrackClipboard, true);
	// for io buttons, etc (but KO right now..)
//	TrackList_AdjustWindows(false); 
	Undo_EndBlock(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL);
}

// ** routing cut copy/paste **
void copyRoutings(COMMAND_T* _ct) {
	copySendsReceives(false, &g_sndClipboard, &g_rcvClipboard);
}

void cutRoutings(COMMAND_T* _ct) {
	copySendsReceives(false, &g_sndClipboard, &g_rcvClipboard);
	removeSends(NULL);
	removeReceives(NULL);
	// for io buttons, etc (but KO right now..)
//	TrackList_AdjustWindows(false);
	// nop if nothing done
	Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1); 
}

void pasteRoutings(COMMAND_T* _ct) {
	if (pasteSendsReceives(&g_sndClipboard, &g_rcvClipboard, false))
	{
		// for io buttons, etc (but KO right now..)
//		TrackList_AdjustWindows(false);
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
}

// ** sends cut copy/paste **
void copySends(COMMAND_T* _ct) {
	copySendsReceives(false, &g_sndClipboard, NULL);
}

void cutSends(COMMAND_T* _ct) {
	copySendsReceives(false, &g_sndClipboard, NULL);
	removeSends(NULL);
	// for io buttons, etc (but KO right now..)
//	TrackList_AdjustWindows(false);
	// nop if nothing done
	Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}

void pasteSends(COMMAND_T* _ct) {
	if (pasteSendsReceives(&g_sndClipboard, NULL, false))
	{
		// for io buttons, etc (but KO right now..)
//		TrackList_AdjustWindows(false);
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
}

// ** receives cut copy/paste **
void copyReceives(COMMAND_T* _ct) {
	copySendsReceives(false, NULL, &g_rcvClipboard);
}

void cutReceives(COMMAND_T* _ct) {
	copySendsReceives(false, NULL, &g_rcvClipboard);
	removeReceives(NULL);
	// for io buttons, etc (but KO right now..)
//	TrackList_AdjustWindows(false);
	// nop if nothing done
	Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}

void pasteReceives(COMMAND_T* _ct) {
	if (pasteSendsReceives(NULL, &g_rcvClipboard, false))
	{
		// for io buttons, etc (but KO right now..)
//		TrackList_AdjustWindows(false);
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
}


///////////////////////////////////////////////////////////////////////////////
// Common
///////////////////////////////////////////////////////////////////////////////


const char* GetSendTypeStr(int _type) 
{
	switch(_type)
	{
		case 1: return "Post-Fader (Post-Pan)";
		case 2: return "Pre-Fader (Post-FX)";
		case 3: return "Pre-FX";
		default: return NULL;
	}
}

void removeSends(COMMAND_T* _ct)
{
	bool updated = false;
	for (int i = 1; i <= GetNumTracks(); i++) //doesn't include master
	{
		MediaTrack* trDest = CSurf_TrackFromID(i, false);
		if (trDest)
		{
			SNM_SendPatcher p(trDest); // nothing done yet
			for (int j = 1; j <= GetNumTracks(); j++) 
			{
				MediaTrack* trSrc = CSurf_TrackFromID(j, false);
				if (trSrc && *(int*)GetSetMediaTrackInfo(trSrc, "I_SELECTED", NULL))
					updated |= (p.RemoveReceivesFrom(trSrc) > 0);
			}
		}
	}
	// Undo point
	if (_ct && updated)
	{
		// for io buttons, etc (but KO right now..)
//		TrackList_AdjustWindows(false);
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
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
	if (_ct && updated)
	{
		// for io buttons, etc (but KO right now..)
//		TrackList_AdjustWindows(false);
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
}

void removeRouting(COMMAND_T* _ct)
{
	// TODO: optimization (a same track can be multi-patched here..)
	removeSends(NULL);
	removeReceives(NULL);
	// for io buttons, etc (but KO right now..)
//	TrackList_AdjustWindows(false);
	// nop if nothing done
	Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1); 
}

