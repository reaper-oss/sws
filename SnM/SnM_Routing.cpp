/******************************************************************************
/ SnM_Routing.cpp
/
/ Copyright (c) 2009-2012 Jeffos
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
#include "SnM.h"
#include "../reaper/localize.h"


///////////////////////////////////////////////////////////////////////////////
// General routing helpers
///////////////////////////////////////////////////////////////////////////////

//JFB REAPER bug:  best effort, see http://forum.cockos.com/project.php?issueid=2642
void RefreshRoutingsUI() {
	TrackList_AdjustWindows(true);
}


///////////////////////////////////////////////////////////////////////////////
// Cue buss 
///////////////////////////////////////////////////////////////////////////////

// adds a receive (with vol & pan from source track for pre-fader)
// _srcTr:  source track (unchanged)
// _destTr: destination track
// _type:   reaper's type
//          0=Post-Fader (Post-Pan), 1=Pre-FX, 2=deprecated, 3=Pre-Fader (Post-FX)
// _p:      for multi-patch optimization, current destination track's SNM_SendPatcher
bool AddReceiveWithVolPan(MediaTrack * _srcTr, MediaTrack * _destTr, int _type, SNM_SendPatcher* _p)
{
	bool update = false;
	char vol[32] = "1.00000000000000";
	char pan[32] = "0.00000000000000";

	// if pre-fader, then re-copy track vol/pan
	if (_type == 3)
	{
		SNM_ChunkParserPatcher p(_srcTr, false);
		p.SetWantsMinimalState(true);
		if (p.Parse(SNM_GET_CHUNK_CHAR, 1, "TRACK", "VOLPAN", 0, 1, vol, NULL, "MUTESOLO") > 0 &&
			p.Parse(SNM_GET_CHUNK_CHAR, 1, "TRACK", "VOLPAN", 0, 2, pan, NULL, "MUTESOLO") > 0)
		{
			update = (_p->AddReceive(_srcTr, _type, vol, pan) > 0);
		}
	}

	// default volume
	if (!update && _snprintfStrict(vol, sizeof(vol), "%.14f", *(double*)GetConfigVar("defsendvol")) > 0)
		update = (_p->AddReceive(_srcTr, _type, vol, pan) > 0);
	return update;
}

// _type: 0=Post-Fader (Post-Pan), 1=Pre-FX, 2=deprecated, 3=Pre-Fader (Post-FX)
// _undoMsg: NULL=no undo
bool CueTrack(const char* _undoMsg, const char* _busName, int _type, bool _showRouting,
			  int _soloDefeat, char* _trTemplatePath, bool _sendToMaster, int* _hwOuts) 
{
	if (!SNM_CountSelectedTracks(NULL, false))
		return false;

	WDL_FastString tmpltChunk;
	if (_trTemplatePath && (!FileExists(_trTemplatePath) || !LoadChunk(_trTemplatePath, &tmpltChunk) || !tmpltChunk.GetLength()))
	{
		char msg[BUFFER_SIZE];
		lstrcpyn(msg, __LOCALIZE("Cue buss not created!\nNo track template file defined","sws_DLG_149"), BUFFER_SIZE);
		if (*_trTemplatePath)
			_snprintfSafe(msg, sizeof(msg), __LOCALIZE_VERFMT("Cue buss not created!\nTrack template not found (or empty): %s","sws_DLG_149"), _trTemplatePath);
		MessageBox(GetMainHwnd(), msg, __LOCALIZE("S&M - Error","sws_DLG_149"), MB_OK);
		return false;
	}

	bool updated = false;
	MediaTrack * cueTr = NULL;
	SNM_SendPatcher* p = NULL;
	for (int i=1; i <= GetNumTracks(); i++) // skip master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i0);

			// add the buss track (if not already done)
			if (!cueTr)
			{
				InsertTrackAtIndex(GetNumTracks(), false);
				TrackList_AdjustWindows(false);
				cueTr = CSurf_TrackFromID(GetNumTracks(), false);
				GetSetMediaTrackInfo(cueTr, "P_NAME", (void*)_busName);
				p = new SNM_SendPatcher(cueTr);
				if (tmpltChunk.GetLength())
					ApplyTrackTemplate(cueTr, &tmpltChunk, true, false, p);
				updated = true;
			}

			// add a send
			if (cueTr && p && tr != cueTr)
				AddReceiveWithVolPan(tr, cueTr, _type, p);
		}
	}

	if (cueTr && p)
	{
		// send to master/parent init
		if (!tmpltChunk.GetLength())
		{
			// solo defeat
			if (_soloDefeat) {
				char c1[2] = "1";
				updated |= (p->ParsePatch(SNM_SET_CHUNK_CHAR, 1, "TRACK", "MUTESOLO", 0, 3, c1) > 0);
			}
			
			// master/parend send
			WDL_FastString mainSend;
			mainSend.SetFormatted(SNM_MAX_CHUNK_LINE_LENGTH, "MAINSEND %d 0", _sendToMaster?1:0);

			// adds hw outputs
			if (_hwOuts)
			{
				int monoHWCount=0; 
				while (GetOutputChannelName(monoHWCount)) monoHWCount++;

				bool cr = false;			
				for(int i=0; i<SNM_MAX_HW_OUTS; i++)
				{
					if (_hwOuts[i])
					{
						if (!cr) {
							mainSend.Append("\n"); 
							cr = true;
						}
						if (_hwOuts[i] >= monoHWCount) 
							mainSend.AppendFormatted(32, "HWOUT %d ", (_hwOuts[i]-monoHWCount) | 1024);
						else
							mainSend.AppendFormatted(32, "HWOUT %d ", _hwOuts[i]-1);

						mainSend.Append("0 ");
						mainSend.AppendFormatted(20, "%.14f ", *(double*)GetConfigVar("defhwvol"));
						mainSend.Append("0.00000000000000 0 0 0 -1.00000000000000 -1\n");
					}
				}
				if (!cr)
					mainSend.Append("\n"); // hot
			}

			// patch both updates (no break keyword here: new empty track)
			updated |= p->ReplaceLine("TRACK", "MAINSEND", 1, 0, mainSend.Get());
		}

		p->Commit();
		delete p;

		if (updated)
		{
			GetSetMediaTrackInfo(cueTr, "I_SELECTED", &g_i1);
			UpdateTimeline();
			RefreshRoutingsUI();
			ScrollSelTrack(NULL, true, true);
			if (_showRouting) 
				Main_OnCommand(40293, 0);
			if (_undoMsg)
				Undo_OnStateChangeEx(_undoMsg, UNDO_STATE_ALL, -1);
		}
	}
	return updated;
}

int g_cueBusslastSettingsId = 0; // 0 for ascendant compatibility

bool CueTrack(const char* _undoMsg, int _confId)
{
	if (_confId<0 || _confId>=SNM_MAX_CUE_BUSS_CONFS)
		_confId = g_cueBusslastSettingsId;
	char busName[BUFFER_SIZE]="", trTemplatePath[BUFFER_SIZE]="";
	int reaType, soloGrp, hwOuts[8];
	bool trTemplate,showRouting,sendToMaster;
	ReadCueBusIniFile(_confId, busName, &reaType, &trTemplate, trTemplatePath, &showRouting, &soloGrp, &sendToMaster, hwOuts);
	bool updated = CueTrack(_undoMsg, busName, reaType, showRouting, soloGrp, trTemplate?trTemplatePath:NULL, sendToMaster, hwOuts);
	g_cueBusslastSettingsId = _confId;
	return updated;
}

void CueTrack(COMMAND_T* _ct) {
	CueTrack(SWS_CMD_SHORTNAME(_ct), (int)_ct->user);
}

void ReadCueBusIniFile(int _confId, char* _busName, int* _reaType, bool* _trTemplate, char* _trTemplatePath, bool* _showRouting, int* _soloDefeat, bool* _sendToMaster, int* _hwOuts)
{
	if (_confId>=0 && _confId<SNM_MAX_CUE_BUSS_CONFS && _busName && _reaType && _trTemplate && _trTemplatePath && _showRouting && _soloDefeat && _sendToMaster && _hwOuts)
	{
		char iniSection[64]="";
		if (_snprintfStrict(iniSection, sizeof(iniSection), "CueBuss%d", _confId+1) > 0)
		{
			char buf[16]="", slot[16]="";
			GetPrivateProfileString(iniSection,"name","",_busName,BUFFER_SIZE,g_SNMIniFn.Get());
			GetPrivateProfileString(iniSection,"reatype","3",buf,16,g_SNMIniFn.Get());
			*_reaType = atoi(buf); // 0 if failed 
			GetPrivateProfileString(iniSection,"track_template_enabled","0",buf,16,g_SNMIniFn.Get());
			*_trTemplate = (atoi(buf) == 1);
			GetPrivateProfileString(iniSection,"track_template_path","",_trTemplatePath,BUFFER_SIZE,g_SNMIniFn.Get());
			GetPrivateProfileString(iniSection,"show_routing","1",buf,16,g_SNMIniFn.Get());
			*_showRouting = (atoi(buf) == 1);
			GetPrivateProfileString(iniSection,"send_to_masterparent","0",buf,16,g_SNMIniFn.Get());
			*_sendToMaster = (atoi(buf) == 1);
			GetPrivateProfileString(iniSection,"solo_defeat","1",buf,16,g_SNMIniFn.Get());
			*_soloDefeat = atoi(buf);
			for (int i=0; i<SNM_MAX_HW_OUTS; i++)
			{
				if (_snprintfStrict(slot, sizeof(slot), "hwout%d", i+1) > 0)
					GetPrivateProfileString(iniSection,slot,"0",buf,BUFFER_SIZE,g_SNMIniFn.Get());
				else
					*buf = '\0';
				_hwOuts[i] = atoi(buf);
			}
		}
	}
}

void SaveCueBusIniFile(int _confId, const char* _busName, int _type, bool _trTemplate, const char* _trTemplatePath, bool _showRouting, int _soloDefeat, bool _sendToMaster, int* _hwOuts)
{
	if (_confId>=0 && _confId<SNM_MAX_CUE_BUSS_CONFS && _busName && _trTemplatePath && _hwOuts)
	{
		char iniSection[64]="";
		if (_snprintfStrict(iniSection, sizeof(iniSection), "CueBuss%d", _confId+1) > 0)
		{
			char buf[16]="", slot[16]="";
			WDL_FastString escapedStr;
			makeEscapedConfigString(_busName, &escapedStr);
			WritePrivateProfileString(iniSection,"name",escapedStr.Get(),g_SNMIniFn.Get());
			if (_snprintfStrict(buf, sizeof(buf), "%d" ,_type) > 0)
				WritePrivateProfileString(iniSection,"reatype",buf,g_SNMIniFn.Get());
			WritePrivateProfileString(iniSection,"track_template_enabled",_trTemplate ? "1" : "0",g_SNMIniFn.Get());
			makeEscapedConfigString(_trTemplatePath, &escapedStr);
			WritePrivateProfileString(iniSection,"track_template_path",escapedStr.Get(),g_SNMIniFn.Get());
			WritePrivateProfileString(iniSection,"show_routing",_showRouting ? "1" : "0",g_SNMIniFn.Get());
			WritePrivateProfileString(iniSection,"send_to_masterparent",_sendToMaster ? "1" : "0",g_SNMIniFn.Get());
			if (_snprintfStrict(buf, sizeof(buf), "%d", _soloDefeat) > 0)
				WritePrivateProfileString(iniSection,"solo_defeat",buf,g_SNMIniFn.Get());
			for (int i=0; i<SNM_MAX_HW_OUTS; i++) 
				if (_snprintfStrict(slot, sizeof(slot), "hwout%d", i+1) > 0 && _snprintfStrict(buf, sizeof(buf), "%d", _hwOuts[i]) > 0)
					WritePrivateProfileString(iniSection,slot,_hwOuts[i]?buf:NULL,g_SNMIniFn.Get());
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// Cut/copy/paste routings + track with routings
///////////////////////////////////////////////////////////////////////////////

WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> > g_sndTrackClipboard;
WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> > g_rcvTrackClipboard; 
WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> > g_sndClipboard;
WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> > g_rcvClipboard;

void FlushClipboard(WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _clipboard) {
	if (_clipboard) _clipboard->Empty(true);
}

void CopySendsReceives(bool _cut, WDL_PtrList<MediaTrack>* _trs,
		WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _sends, 
		WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _rcvs)
{
	// clear the "clipboards" if needed
	FlushClipboard(_sends);
	FlushClipboard(_rcvs);

	for (int i=0; i < _trs->GetSize(); i++)
	{
		if (MediaTrack* tr = _trs->Get(i))
		{
			// copy sends
			if (_sends)
			{
				_sends->Add(new WDL_PtrList_DeleteOnDestroy<SNM_SndRcv>());

				int idx=0;
				MediaTrack* dest = (MediaTrack*)GetSetTrackSendInfo(tr, 0, idx, "P_DESTTRACK", NULL);
				while (dest)
				{
					// do not store cross-cut/pasted tracks' sends (duplicated with the following receives re-copy)
					if (!_cut || (_cut && _trs->Find(dest)<0))
					{
						SNM_SndRcv* send = new SNM_SndRcv();
						if (send->FillIOFromReaper(tr, dest, 0, idx))
							_sends->Get(i)->Add(send);
					}
					dest = (MediaTrack*)GetSetTrackSendInfo(tr, 0, ++idx, "P_DESTTRACK", NULL);
				}
			}

			// copy receives
			if (_rcvs)
			{
				_rcvs->Add(new WDL_PtrList_DeleteOnDestroy<SNM_SndRcv>());

				int idx=0;
				MediaTrack* src = (MediaTrack*)GetSetTrackSendInfo(tr, -1, idx, "P_SRCTRACK", NULL);
				while (src)
				{
					SNM_SndRcv* rcv = new SNM_SndRcv();
					if (rcv ->FillIOFromReaper(src, tr, -1, idx))
						_rcvs->Get(i)->Add(rcv);
					src = (MediaTrack*)GetSetTrackSendInfo(tr, -1, ++idx, "P_SRCTRACK", NULL);
				}
			}
		}
	}
}

bool PasteSendsReceives(WDL_PtrList<MediaTrack>* _trs,
		WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _sends, 
		WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _rcvs, 
		bool _rcvReplace, WDL_PtrList<SNM_ChunkParserPatcher>* _ps)
{
	bool updated = false;

	// a same track can be multi-patched
	// => patch everything in one go thanks to this list
	WDL_PtrList<SNM_ChunkParserPatcher>* ps = (_ps ? _ps : new WDL_PtrList<SNM_ChunkParserPatcher>);

	// 1st loop: remove existing receives, if needed
	for (int i=0; _rcvReplace && i < _trs->GetSize(); i++)
		if (MediaTrack* tr = _trs->Get(i))
		{
			SNM_SendPatcher* p = (SNM_SendPatcher*)SNM_FindCPPbyObject(ps, tr);
			if (!p) {
				p = new SNM_SendPatcher(tr); 
				ps->Add(p);
			}
			updated |= (p->RemoveReceives() > 0);
		}

	// 2nd one: to add ours
	for (int i=0; i < _trs->GetSize(); i++)
	{
		if (MediaTrack* tr = _trs->Get(i))
		{
			// paste sends
			if (_sends && _sends->Get(i))
			{
				for (int j=0; j < _sends->Get(i)->GetSize(); j++)
				{
					SNM_SndRcv* send = _sends->Get(i)->Get(j);
					if (MediaTrack* trDest = GuidToTrack(&send->m_dest))
					{
						SNM_SendPatcher* p = (SNM_SendPatcher*)SNM_FindCPPbyObject(ps, trDest);
						if (!p) {
							p = new SNM_SendPatcher(trDest); 
							ps->Add(p);
						}
						updated |= p->AddReceive(tr, send);
					}
				}
			}

			// paste receives
			if (_rcvs && _rcvs->Get(i))
			{
					for (int j=0; j < _rcvs->Get(i)->GetSize(); j++)
					{
						SNM_SndRcv* rcv = _rcvs->Get(i)->Get(j);
						if (MediaTrack* trSrc = GuidToTrack(&rcv->m_src))
						{
							SNM_SendPatcher* p = (SNM_SendPatcher*)SNM_FindCPPbyObject(ps, tr);
							if (!p) {
								p = new SNM_SendPatcher(tr); 
								ps->Add(p);
							}
							updated |= p->AddReceive(trSrc, rcv);
						}
					}
			}
		}
	}
	if (!_ps) ps->Empty(true); // + auto commit if needed
	return updated;
}

void CopyWithIOs(COMMAND_T* _ct) {
	WDL_PtrList<MediaTrack> trs;
	SNM_GetSelectedTracks(NULL, &trs, false);
	if (trs.GetSize()) {
		CopySendsReceives(false, &trs, &g_sndTrackClipboard, &g_rcvTrackClipboard);
		Main_OnCommand(40210, 0); // copy sel tracks
	}
}

void CutWithIOs(COMMAND_T* _ct) {
	WDL_PtrList<MediaTrack> trs;
	SNM_GetSelectedTracks(NULL, &trs, false);
	if (trs.GetSize()) {
		Undo_BeginBlock2(NULL);
		CopySendsReceives(true, &trs, &g_sndTrackClipboard, &g_rcvTrackClipboard);
		Main_OnCommand(40337, 0); // cut sel tracks
		RefreshRoutingsUI();
		if (_ct) Undo_EndBlock2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL);
	}
}

void PasteWithIOs(COMMAND_T* _ct) {
	if (int iTracks = GetNumTracks())
	{
		Undo_BeginBlock2(NULL);
		Main_OnCommand(40058, 0); // native track paste
		if (iTracks != GetNumTracks()) { // see if tracks were pasted
			WDL_PtrList<MediaTrack> trs;
			SNM_GetSelectedTracks(NULL, &trs, false);
			PasteSendsReceives(&trs, &g_sndTrackClipboard, &g_rcvTrackClipboard, true, NULL);
		}
		RefreshRoutingsUI();
		if (_ct) Undo_EndBlock2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL);
	}
}

// routing cut copy/paste
void CopyRoutings(COMMAND_T* _ct) {
	WDL_PtrList<MediaTrack> trs;
	SNM_GetSelectedTracks(NULL, &trs, false);
	if (trs.GetSize())
		CopySendsReceives(false, &trs, &g_sndClipboard, &g_rcvClipboard);
}

void CutRoutings(COMMAND_T* _ct) {
	bool updated = false;
	WDL_PtrList<MediaTrack> trs;
	SNM_GetSelectedTracks(NULL, &trs, false);
	if (trs.GetSize()) {
		WDL_PtrList<SNM_ChunkParserPatcher> ps;
		CopySendsReceives(false, &trs, &g_sndClipboard, &g_rcvClipboard);
		updated |= RemoveSnd(&trs, &ps);
		updated |= RemoveRcv(&trs, &ps);
		ps.Empty(true); // auto-commit, if needed
	}
	if (updated) {
		RefreshRoutingsUI();
		if (_ct) Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1); // nop if nothing done
	}
}

void PasteRoutings(COMMAND_T* _ct) {
	WDL_PtrList<MediaTrack> trs;
	SNM_GetSelectedTracks(NULL, &trs, false);
	if (trs.GetSize() && PasteSendsReceives(&trs, &g_sndClipboard, &g_rcvClipboard, false, NULL)) {
		RefreshRoutingsUI();
		if (_ct) Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
}

// sends cut copy/paste
void CopySends(COMMAND_T* _ct) {
	WDL_PtrList<MediaTrack> trs;
	SNM_GetSelectedTracks(NULL, &trs, false);
	if (trs.GetSize())
		CopySendsReceives(false, &trs, &g_sndClipboard, NULL);
}

void CutSends(COMMAND_T* _ct) {
	bool updated = false;
	WDL_PtrList<MediaTrack> trs;
	SNM_GetSelectedTracks(NULL, &trs, false);
	if (trs.GetSize()) {
		CopySendsReceives(false, &trs, &g_sndClipboard, NULL);
		updated |= RemoveSnd(&trs, NULL);
	}
	if (updated) {
		RefreshRoutingsUI();
		if (_ct) Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1); // nop if nothing done
	}
}

void PasteSends(COMMAND_T* _ct) {
	WDL_PtrList<MediaTrack> trs;
	SNM_GetSelectedTracks(NULL, &trs, false);
	if (trs.GetSize() && PasteSendsReceives(&trs, &g_sndClipboard, NULL, false, NULL)) {
		RefreshRoutingsUI();
		if (_ct) Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
}

// receives cut copy/paste
void CopyReceives(COMMAND_T* _ct) {
	WDL_PtrList<MediaTrack> trs;
	SNM_GetSelectedTracks(NULL, &trs, false);
	if (trs.GetSize())
		CopySendsReceives(false, &trs, NULL, &g_rcvClipboard);
}

void CutReceives(COMMAND_T* _ct) {
	bool updated = false;
	WDL_PtrList<MediaTrack> trs;
	SNM_GetSelectedTracks(NULL, &trs, false);
	if (trs.GetSize()) {
		CopySendsReceives(false, &trs, NULL, &g_rcvClipboard);
		updated |= RemoveRcv(&trs, NULL);
	}
	if (updated) {
		RefreshRoutingsUI();
		if (_ct) Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1); // nop if nothing done
	}
}

void PasteReceives(COMMAND_T* _ct) {
	WDL_PtrList<MediaTrack> trs;
	SNM_GetSelectedTracks(NULL, &trs, false);
	if (trs.GetSize() && PasteSendsReceives(&trs, NULL, &g_rcvClipboard, false, NULL)) {
		RefreshRoutingsUI();
		if (_ct) Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
}


///////////////////////////////////////////////////////////////////////////////
// Remove routing
///////////////////////////////////////////////////////////////////////////////

bool RemoveSnd(WDL_PtrList<MediaTrack>* _trs, WDL_PtrList<SNM_ChunkParserPatcher>* _ps)
{
	bool updated = false;
	WDL_PtrList<SNM_ChunkParserPatcher>* ps = (_ps ? _ps : new WDL_PtrList<SNM_ChunkParserPatcher>);
	for (int j=1; j <= GetNumTracks(); j++) // skip master
	{
		if (MediaTrack* trSrc = CSurf_TrackFromID(j, false))
		{
			SNM_SendPatcher* p = (SNM_SendPatcher*)SNM_FindCPPbyObject(ps, trSrc);
			if (!p) {
				p = new SNM_SendPatcher(trSrc); 
				ps->Add(p);
			}
			for (int i=0; i < _trs->GetSize(); i++)
//				if (_trs->Get(i) != trSrc)
					updated |= (p->RemoveReceivesFrom(_trs->Get(i)) > 0);
		}
	}
	if (!_ps) ps->Empty(true); // auto-commit if needed
	return updated;
}

void RemoveSends(COMMAND_T* _ct)
{
	bool updated = false;
	WDL_PtrList<MediaTrack> trs;
	SNM_GetSelectedTracks(NULL, &trs, false);
	if (trs.GetSize())
		updated = RemoveSnd(&trs, NULL);
	if (updated) {
		RefreshRoutingsUI();
		if (_ct) Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
}

bool RemoveRcv(WDL_PtrList<MediaTrack>* _trs, WDL_PtrList<SNM_ChunkParserPatcher>* _ps)
{
	bool updated = false;
	WDL_PtrList<SNM_ChunkParserPatcher>* ps = (_ps ? _ps : new WDL_PtrList<SNM_ChunkParserPatcher>);
	for (int i=0; i < _trs->GetSize(); i++)
		if (MediaTrack* tr = _trs->Get(i))
		{
			SNM_SendPatcher* p = (SNM_SendPatcher*)SNM_FindCPPbyObject(ps, tr);
			if (!p) {
				p = new SNM_SendPatcher(tr); 
				ps->Add(p);
			}
			updated |= (p->RemoveReceives() > 0); 
		}
	if (!_ps) ps->Empty(true); // auto-commit if needed
	return updated;
}

void RemoveReceives(COMMAND_T* _ct)
{
	bool updated = false;
	WDL_PtrList<MediaTrack> trs;
	SNM_GetSelectedTracks(NULL, &trs, false);
	if (trs.GetSize())
		updated = RemoveRcv(&trs, NULL);
	if (updated) {
		RefreshRoutingsUI();
		if (_ct) Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
}

void RemoveRouting(COMMAND_T* _ct)
{
	bool updated = false;
	WDL_PtrList<MediaTrack> trs;
	SNM_GetSelectedTracks(NULL, &trs, false);
	if (trs.GetSize()) {
		WDL_PtrList<SNM_ChunkParserPatcher> ps;
		updated |= RemoveSnd(&trs, &ps);
		updated |= RemoveRcv(&trs, &ps);
		ps.Empty(true); // auto-commit, if needed
	}
	if (updated) {
		RefreshRoutingsUI();
		if (_ct) Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1); 
	}
}


///////////////////////////////////////////////////////////////////////////////
// Other routing funcs/commands
///////////////////////////////////////////////////////////////////////////////

int g_defSndFlags = -1;

void SaveDefaultTrackSendPrefs(COMMAND_T*) {
	if (int* flags = (int*)GetConfigVar("defsendflag"))
		g_defSndFlags = *flags;
}

void RecallDefaultTrackSendPrefs(COMMAND_T*) {
	if (g_defSndFlags>=0)
		if (int* flags = (int*)GetConfigVar("defsendflag"))
			*flags = g_defSndFlags;
}

// flags != _ct->user because of the weird state of midi+audio which is not 0x300 but 0 (!)
void SetDefaultTrackSendPrefs(COMMAND_T* _ct) {
	if (int* flags = (int*)GetConfigVar("defsendflag")) {
		switch((int)_ct->user) {
			case 0: *flags&=0xFF; break; // both
			case 1: *flags&=0xF0FF; *flags|=0x100; break; // audio
			case 2: *flags&=0xF0FF; *flags|=0x200; break; // midi
		}
	}
}

void MuteReceives(MediaTrack* _source, MediaTrack* _dest, bool _mute)
{
	if (_source && _dest && _source != _dest)
	{
		int rcvIdx=0;
		MediaTrack* rcvTr = (MediaTrack*)GetSetTrackSendInfo(_dest, -1, rcvIdx, "P_SRCTRACK", NULL);
		while(rcvTr)
		{
			if (rcvTr == _source)
				GetSetTrackSendInfo(_dest, -1, rcvIdx, "B_MUTE", &_mute);
			rcvTr = (MediaTrack*)GetSetTrackSendInfo(_dest, -1, ++rcvIdx, "P_SRCTRACK", NULL);
		}
	}
}

