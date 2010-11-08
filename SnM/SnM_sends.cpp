/******************************************************************************
/ SnM_Sends.cpp
/
/ Copyright (c) 2009-2010 Tim Payne (SWS), Jeffos
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


WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> > g_sndTrackClipboard; 
WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> > g_rcvTrackClipboard; 
WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> > g_sndClipboard;
WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> > g_rcvClipboard;

HWND g_cueBussHwnd = NULL;


///////////////////////////////////////////////////////////////////////////////
// Cue buss 
///////////////////////////////////////////////////////////////////////////////

// Adds a receive (with vol & pan from source track for pre-fader)
// _srcTr:  source track (unchanged)
// _destTr: destination track
// _type:   reaper's type
//          0=Post-Fader (Post-Pan), 1=Pre-FX, 2=deprecated, 3=Pre-Fader (Post-FX)
// _p:      for multi-patch optimization, current destination track's SNM_SendPatcher
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

// Adds a cue buss track from track selection
// _type:   reaper's type
//          0=Post-Fader (Post-Pan), 1=Pre-FX, 2=deprecated, 3=Pre-Fader (Post-FX)
// _undoMsg NULL=no undo
bool cueTrack(const char* _busName, int _type, const char* _undoMsg, 
			  bool _showRouting, int _soloDefeat, 
			  char* _trTemplatePath, 
			  bool _sendToMaster, int* _hwOuts) 
{
	bool updated = false;

	WDL_String chunk;
	if (_trTemplatePath && !LoadChunk(_trTemplatePath, &chunk))
	{
		char err[512] = "";
		sprintf(err, "Cue buss not created!\nInvalid track template file: %s", _trTemplatePath);
		MessageBox(GetMainHwnd(), err, "Error", MB_OK);
		return false;
	}

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
				GetSetMediaTrackInfo(cueTr, "P_NAME", (void*)_busName);
				p = new SNM_SendPatcher(cueTr);
				if (chunk.GetLength())
					p->SetChunk(&chunk, 1);
				updated = true;
			}

			// add a send
			if (cueTr && p && tr != cueTr)
			{
				addReceiveWithVolPan(tr, cueTr, _type, p); 
#ifdef _SNM_TRACK_GROUP_EX
				SNM_ChunkParserPatcher pSrc(tr);
				updated |= (addSoloToGroup(cueTr, _soloGrp, true, &pSrc) > 0); // nop if invalid prms
#endif
			}
		}
	}

	if (cueTr && p)
	{
#ifdef _SNM_TRACK_GROUP_EX
		// add slave solo to track grouping
		updated |= (addSoloToGroup(cueTr, _soloGrp, false, p) > 0); // nop if invalid prms
#endif
		// send to master/parent init
		if (!chunk.GetLength())
		{
			// solo defeat
			if (_soloDefeat)
			{
				char c1[2] = "1";
				updated |= (p->ParsePatch(SNM_SET_CHUNK_CHAR, 1, "TRACK", "MUTESOLO", -1, 0, 3, c1) > 0);
			}
			
			// master/parend send
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

void openCueBussWnd(COMMAND_T* _ct) {
	static HWND hwnd = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_SNM_CUEBUS), g_hwndParent, CueBusDlgProc);

	// Toggle
	if (g_cueBussHwnd) {
		g_cueBussHwnd = NULL;
		ShowWindow(hwnd, SW_HIDE);
	}
	else {
		g_cueBussHwnd = hwnd;
		ShowWindow(hwnd, SW_SHOW);
		SetFocus(hwnd);
	}
}

bool isCueBussWndDisplayed(COMMAND_T* _ct) {
	return (g_cueBussHwnd && IsWindow(g_cueBussHwnd) ? true : false);
}

void cueTrack(COMMAND_T* _ct) 
{
	char busName[BUFFER_SIZE] = "";
	char trTemplatePath[BUFFER_SIZE] = "";
	int reaType, soloGrp, hwOuts[8];
	bool trTemplate,showRouting,sendToMaster;
	readCueBusIniFile(busName, &reaType, &trTemplate, trTemplatePath, &showRouting, &soloGrp, &sendToMaster, hwOuts);

	cueTrack(
		busName, 
		((int)_ct->user < 0 ? reaType : (int)_ct->user), 
		SNM_CMD_SHORTNAME(_ct), 
		showRouting, 
		soloGrp, 
		trTemplate ? trTemplatePath : NULL, 
		sendToMaster, 
		hwOuts);
}


///////////////////////////////////////////////////////////////////////////////
// Cut/Copy/Paste: track with sends, routings
///////////////////////////////////////////////////////////////////////////////

void flushClipboard(WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _clipboard) {
	if (_clipboard)
		_clipboard->Empty(true);
}

bool FillIOFromReaper(SNM_SndRcv* _send, MediaTrack* _src, MediaTrack* _dest, int _categ, int _idx)
{
	if (_send && _src && _dest)
	{
		MediaTrack* tr = (_categ == -1 ? _dest : _src);
		_send->m_src = _src;
		_send->m_dest = _dest;
		_send->m_mute = *(bool*)GetSetTrackSendInfo(tr, _categ, _idx, "B_MUTE", NULL);
		_send->m_phase = *(bool*)GetSetTrackSendInfo(tr, _categ, _idx, "B_PHASE", NULL);
		_send->m_mono = *(bool*)GetSetTrackSendInfo(tr, _categ, _idx, "B_MONO", NULL);
		_send->m_vol = *(double*)GetSetTrackSendInfo(tr, _categ, _idx, "D_VOL", NULL);
		_send->m_pan = *(double*)GetSetTrackSendInfo(tr, _categ, _idx, "D_PAN", NULL);
		_send->m_panl = *(double*)GetSetTrackSendInfo(tr, _categ, _idx, "D_PANLAW", NULL);
		_send->m_mode = *(int*)GetSetTrackSendInfo(tr, _categ, _idx, "I_SENDMODE", NULL);
		_send->m_srcChan = *(int*)GetSetTrackSendInfo(tr, _categ, _idx, "I_SRCCHAN", NULL);
		_send->m_destChan = *(int*)GetSetTrackSendInfo(tr, _categ, _idx, "I_DSTCHAN", NULL);
		_send->m_midi = *(int*)GetSetTrackSendInfo(tr, _categ, _idx, "I_MIDIFLAGS", NULL);
		return true;
	}
	return false;
}

void copySendsReceives(bool _cut, 
		WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _sends, 
		WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _rcvs)
{
	// Clear the "clipboards" if needed
	flushClipboard(_sends);
	flushClipboard(_rcvs);

	int selTrackIdx = 0;
	for (int i = 1; i <= GetNumTracks(); i++) //doesn't include master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			// *** Copy sends ***
			if (_sends)
			{
				_sends->Add(new WDL_PtrList_DeleteOnDestroy<SNM_SndRcv>());

				int idx=0;
				MediaTrack* dest = (MediaTrack*)GetSetTrackSendInfo(tr, 0, idx, "P_DESTTRACK", NULL);
				while (dest)
				{
					// We do not store cross-cut/pasted tracks' sends ('cause they'll keep the same track id after paste)
					// not to duplicate with the following receives re-copy
					if (!_cut ||
						(_cut && !(*(int*)GetSetMediaTrackInfo(dest, "I_SELECTED", NULL))))
					{
						SNM_SndRcv* send = new SNM_SndRcv();
						if (send && FillIOFromReaper(send, tr, dest, 0, idx))
							_sends->Get(selTrackIdx)->Add(send);
					}
					dest = (MediaTrack*)GetSetTrackSendInfo(tr, 0, ++idx, "P_DESTTRACK", NULL);
				}
			}

			// *** Copy receives ***
			if (_rcvs)
			{
				_rcvs->Add(new WDL_PtrList_DeleteOnDestroy<SNM_SndRcv>());

				int idx=0;
				MediaTrack* src = (MediaTrack*)GetSetTrackSendInfo(tr, -1, idx, "P_SRCTRACK", NULL);
				while (src)
				{
					SNM_SndRcv* rcv = new SNM_SndRcv();
					if (rcv && FillIOFromReaper(rcv, src, tr, -1, idx))
						_rcvs->Get(selTrackIdx)->Add(rcv);
					src = (MediaTrack*)GetSetTrackSendInfo(tr, -1, ++idx, "P_SRCTRACK", NULL);
				}
			}
			selTrackIdx++;
		}
	}
}

// Paste stored sends and/or receives to the selected tracks
bool pasteSendsReceives(WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _sends, 
		WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _rcvs, bool _rcvReset)
{
	bool updated = false;

	// As a same track can be multi-patched
	// => we'll patch all tracks in one go thanks to this list
	WDL_PtrList<SNM_ChunkParserPatcher> ps;

	// 1st loop to remove the native receives 
	for (int i = 1; i <= GetNumTracks(); i++)  //doesn't include master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			SNM_SendPatcher* p = new SNM_SendPatcher(tr); 
			ps.Add(p);
			if (_rcvReset)
				updated |= (p->RemoveReceives() > 0);
		}
	}

	// 2nd one: to add ours
	int selTrackIdx = 0;
	for (int i = 1; i <= GetNumTracks(); i++) // doesn't include master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			// *** Paste sends ***
			if (_sends)
			{
				for (int k=0; k < _sends->GetSize(); k++)
				{
					for (int j=0; j < _sends->Get(k)->GetSize(); j++)
					{
						SNM_SndRcv* send = _sends->Get(k)->Get(j);
						MediaTrack* sendDest = send->m_dest;
						if (sendDest) 
						{
							SNM_SendPatcher* pRcv = (SNM_SendPatcher*)FindChunkParserPatcherByObject(&ps, send->m_dest);
							if (!pRcv) {
								pRcv = new SNM_SendPatcher(sendDest); 
								ps.Add(pRcv);
							}
							updated |= pRcv->AddReceive(tr, send);
						}
					}
				}
			}

			// *** Paste receives ***
			if (_rcvs)
			{
				for (int k=0; k < _rcvs->GetSize(); k++)
				{
					for (int j=0; j < _rcvs->Get(k)->GetSize(); j++)
					{
						SNM_SndRcv* rcv = _rcvs->Get(k)->Get(j);
						MediaTrack* rcvSrc = rcv->m_src;
						if (rcvSrc) 
							updated |= ((SNM_SendPatcher*)ps.Get(selTrackIdx))->AddReceive(rcvSrc, rcv);
					}
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
	if (iTracks != GetNumTracks()) // see if tracks were pasted
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

int GetComboSendIdxType(int _reaType) 
{
	switch(_reaType)
	{
		case 0: return 1;
		case 3: return 2; 
		case 1: return 3; 
		default: return 1;
	}
	return 1; // in case _reaType comes from mars
}

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

void readCueBusIniFile(char* _busName, int* _reaType, bool* _trTemplate, char* _trTemplatePath, bool* _showRouting, int* _soloDefeat, bool* _sendToMaster, int* _hwOuts)
{
	if (_busName && _reaType && _trTemplate && _trTemplatePath && _showRouting && _soloDefeat && _sendToMaster && _hwOuts)
	{
		GetPrivateProfileString("LAST_CUEBUS","NAME","",_busName,BUFFER_SIZE,g_SNMiniFilename.Get());

		char reaType[16] = "";
		GetPrivateProfileString("LAST_CUEBUS","REATYPE","3",reaType,16,g_SNMiniFilename.Get());
		*_reaType = atoi(reaType); // 0 if failed 

		char trTemplate[16] = "";
		GetPrivateProfileString("LAST_CUEBUS","TRACK_TEMPLATE_ENABLED","0",trTemplate,16,g_SNMiniFilename.Get());
		*_trTemplate = (atoi(trTemplate) == 1); // 0 if failed 

		GetPrivateProfileString("LAST_CUEBUS","TRACK_TEMPLATE_PATH","",_trTemplatePath,BUFFER_SIZE,g_SNMiniFilename.Get());

		char showRouting[16] = "";
		GetPrivateProfileString("LAST_CUEBUS","SHOW_ROUTING","1",showRouting,16,g_SNMiniFilename.Get());
		*_showRouting = (atoi(showRouting) == 1); // 0 if failed 

		char sendToMaster[16] = "";
		GetPrivateProfileString("LAST_CUEBUS","SEND_TO_MASTERPARENT","0",sendToMaster,16,g_SNMiniFilename.Get());
		*_sendToMaster = (atoi(sendToMaster) == 1); // 0 if failed 

		char soloDefeat[16] = "";
		GetPrivateProfileString("LAST_CUEBUS","SOLO_DEFEAT","1",soloDefeat,16,g_SNMiniFilename.Get());
		*_soloDefeat = atoi(soloDefeat); // 0 if failed 

		for (int i=0; i<SNM_MAX_HW_OUTS; i++) 
		{
			char slot[16] = "";
			sprintf(slot,"HWOUT%d",i+1);

			char hwOut[16] = "";
			GetPrivateProfileString("LAST_CUEBUS",slot,"0",hwOut,BUFFER_SIZE,g_SNMiniFilename.Get());
			_hwOuts[i] = atoi(hwOut); // 0 if failed 
		}
	}
}

void saveCueBusIniFile(char* _busName, int _type, bool _trTemplate, char* _trTemplatePath, bool _showRouting, int _soloDefeat, bool _sendToMaster, int* _hwOuts)
{
	if (_busName && _trTemplatePath && _hwOuts)
	{
		WritePrivateProfileString("LAST_CUEBUS","NAME",_busName,g_SNMiniFilename.Get());
		char type[16] = "";
		sprintf(type,"%d",_type);
		WritePrivateProfileString("LAST_CUEBUS","REATYPE",type,g_SNMiniFilename.Get());
		WritePrivateProfileString("LAST_CUEBUS","TRACK_TEMPLATE_ENABLED",_trTemplate ? "1" : "0",g_SNMiniFilename.Get());
		WritePrivateProfileString("LAST_CUEBUS","TRACK_TEMPLATE_PATH",_trTemplatePath,g_SNMiniFilename.Get());
		WritePrivateProfileString("LAST_CUEBUS","SHOW_ROUTING",_showRouting ? "1" : "0",g_SNMiniFilename.Get());
		WritePrivateProfileString("LAST_CUEBUS","SEND_TO_MASTERPARENT",_sendToMaster ? "1" : "0",g_SNMiniFilename.Get());

		char soloDefeat[16] = "";
		sprintf(soloDefeat,"%d",_soloDefeat);
		WritePrivateProfileString("LAST_CUEBUS","SOLO_DEFEAT",soloDefeat,g_SNMiniFilename.Get());

		for (int i=0; i<SNM_MAX_HW_OUTS; i++) 
		{
			char slot[16] = "";
			sprintf(slot,"HWOUT%d",i+1);

			char hwOut[16] = "";
			sprintf(hwOut,"%d",_hwOuts[i]);
			WritePrivateProfileString("LAST_CUEBUS",slot,hwOut,g_SNMiniFilename.Get());
		}
	}
}
