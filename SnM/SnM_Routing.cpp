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

//JFB REAPER bug: best effort, see http://forum.cockos.com/project.php?issueid=2642
// (for easy relacement the day it will work..)
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
bool CueBuss(const char* _undoMsg, const char* _busName, int _type, bool _showRouting,
			 int _soloDefeat, char* _trTemplatePath, bool _sendToMaster, int* _hwOuts) 
{
	if (!SNM_CountSelectedTracks(NULL, false))
		return false;

	WDL_FastString tmplt;
	if (_trTemplatePath && (!FileExists(_trTemplatePath) || !LoadChunk(_trTemplatePath, &tmplt) || !tmplt.GetLength()))
	{
		char msg[SNM_MAX_PATH] = "";
		lstrcpyn(msg, __LOCALIZE("Cue buss not created!\nNo track template file defined","sws_DLG_149"), sizeof(msg));
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

			// add the buss track, done once!
			if (!cueTr)
			{
				InsertTrackAtIndex(GetNumTracks(), false);
				TrackList_AdjustWindows(false);
				cueTr = CSurf_TrackFromID(GetNumTracks(), false);
				GetSetMediaTrackInfo(cueTr, "P_NAME", (void*)_busName);

				p = new SNM_SendPatcher(cueTr);

				if (tmplt.GetLength())
				{
					WDL_FastString chunk;
					MakeSingleTrackTemplateChunk(&tmplt, &chunk, true, true, false);
					ApplyTrackTemplate(cueTr, &chunk, false, false, p);
				}
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
		if (!tmplt.GetLength())
		{
			// solo defeat
			if (_soloDefeat) {
				char one[2] = "1";
				updated |= (p->ParsePatch(SNM_SET_CHUNK_CHAR, 1, "TRACK", "MUTESOLO", 0, 3, one) > 0);
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
			ScrollSelTrack(true, true);
			if (_showRouting) 
				Main_OnCommand(40293, 0);
			if (_undoMsg)
				Undo_OnStateChangeEx2(NULL, _undoMsg, UNDO_STATE_ALL, -1);
		}
	}
	return updated;
}

int g_cueBussLastSettingsId = 0; // 0 for ascendant compatibility

bool CueBuss(const char* _undoMsg, int _confId)
{
	if (_confId<0 || _confId>=SNM_MAX_CUE_BUSS_CONFS)
		_confId = g_cueBussLastSettingsId;
	char busName[64]="", trTemplatePath[SNM_MAX_PATH]="";
	int reaType, soloGrp, hwOuts[SNM_MAX_HW_OUTS];
	bool trTemplate,showRouting,sendToMaster;
	ReadCueBusIniFile(_confId, busName, sizeof(busName), &reaType, &trTemplate, trTemplatePath, sizeof(trTemplatePath), &showRouting, &soloGrp, &sendToMaster, hwOuts);
	bool updated = CueBuss(_undoMsg, busName, reaType, showRouting, soloGrp, trTemplate?trTemplatePath:NULL, sendToMaster, hwOuts);
	g_cueBussLastSettingsId = _confId;
	return updated;
}

void CueBuss(COMMAND_T* _ct) {
	CueBuss(SWS_CMD_SHORTNAME(_ct), (int)_ct->user);
}

void ReadCueBusIniFile(int _confId, char* _busName, int _busNameSz, int* _reaType, bool* _trTemplate, char* _trTemplatePath, int _trTemplatePathSz, bool* _showRouting, int* _soloDefeat, bool* _sendToMaster, int* _hwOuts)
{
	if (_confId>=0 && _confId<SNM_MAX_CUE_BUSS_CONFS && _busName && _reaType && _trTemplate && _trTemplatePath && _showRouting && _soloDefeat && _sendToMaster && _hwOuts)
	{
		char iniSection[64]="";
		if (_snprintfStrict(iniSection, sizeof(iniSection), "CueBuss%d", _confId+1) > 0)
		{
			char buf[16]="", slot[16]="";
			GetPrivateProfileString(iniSection,"name","",_busName,_busNameSz,g_SNMIniFn.Get());
			GetPrivateProfileString(iniSection,"reatype","3",buf,16,g_SNMIniFn.Get());
			*_reaType = atoi(buf); // 0 if failed 
			GetPrivateProfileString(iniSection,"track_template_enabled","0",buf,16,g_SNMIniFn.Get());
			*_trTemplate = (atoi(buf) == 1);
			GetPrivateProfileString(iniSection,"track_template_path","",_trTemplatePath,_trTemplatePathSz,g_SNMIniFn.Get());
			GetPrivateProfileString(iniSection,"show_routing","1",buf,16,g_SNMIniFn.Get());
			*_showRouting = (atoi(buf) == 1);
			GetPrivateProfileString(iniSection,"send_to_masterparent","0",buf,16,g_SNMIniFn.Get());
			*_sendToMaster = (atoi(buf) == 1);
			GetPrivateProfileString(iniSection,"solo_defeat","1",buf,16,g_SNMIniFn.Get());
			*_soloDefeat = atoi(buf);
			for (int i=0; i<SNM_MAX_HW_OUTS; i++)
			{
				if (_snprintfStrict(slot, sizeof(slot), "hwout%d", i+1) > 0)
					GetPrivateProfileString(iniSection,slot,"0",buf,sizeof(buf),g_SNMIniFn.Get());
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
	if (_clipboard)
		_clipboard->Empty(true);
}

void CopySendsReceives(bool _noIntra, WDL_PtrList<MediaTrack>* _trs,
		WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _snds, 
		WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _rcvs)
{
	FlushClipboard(_snds);
	FlushClipboard(_rcvs);

	for (int i=0; i < _trs->GetSize(); i++)
	{
		if (MediaTrack* tr = _trs->Get(i))
		{
			// copy sends
			if (_snds)
			{
				_snds->Add(new WDL_PtrList_DeleteOnDestroy<SNM_SndRcv>());

				int idx=0;
				MediaTrack* dest = (MediaTrack*)GetSetTrackSendInfo(tr, 0, idx, "P_DESTTRACK", NULL);
				while (dest)
				{
					if (!_noIntra || (_noIntra && _trs->Find(dest)<0))
					{
						SNM_SndRcv* send = new SNM_SndRcv();
						if (send->FillIOFromReaper(tr, dest, 0, idx))
							_snds->Get(i)->Add(send);
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
					if (!_noIntra || (_noIntra && _trs->Find(src)<0))
					{
						SNM_SndRcv* rcv = new SNM_SndRcv();
						if (rcv->FillIOFromReaper(src, tr, -1, idx))
							_rcvs->Get(i)->Add(rcv);
					}
					src = (MediaTrack*)GetSetTrackSendInfo(tr, -1, ++idx, "P_SRCTRACK", NULL);
				}
			}
		}
	}
}

bool PasteSendsReceives(bool _send, MediaTrack* _tr, 
		WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _ios, 
		int _trIdx, WDL_PtrList<SNM_ChunkParserPatcher>* _ps)
{
	bool updated = false;
	if (_tr && _ps && _ios && _ios->Get(_trIdx))
	{
		for (int j=0; j < _ios->Get(_trIdx)->GetSize(); j++)
		{
			SNM_SndRcv* io = _ios->Get(_trIdx)->Get(j);
			if (MediaTrack* tr = GuidToTrack(_send ? &io->m_dest : &io->m_src))
			{
				SNM_SendPatcher* p = (SNM_SendPatcher*)SNM_FindCPPbyObject(_ps, _send ? tr : _tr);
				if (!p) {
					p = new SNM_SendPatcher(_send ? tr : _tr); 
					_ps->Add(p);
				}
				updated |= p->AddReceive(_send ? _tr : tr, io);
			}
		}
	}
	return updated;
}

bool PasteSendsReceives(WDL_PtrList<MediaTrack>* _trs,
		WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _snds, 
		WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _rcvs, 
		WDL_PtrList<SNM_ChunkParserPatcher>* _ps)
{
	bool updated = false;

	// a same track can be multi-patched
	// => patch everything in one go thanks to this list
	WDL_PtrList<SNM_ChunkParserPatcher>* ps = (_ps ? _ps : new WDL_PtrList<SNM_ChunkParserPatcher>);

	for (int i=0; i<_trs->GetSize(); i++)
	{
		// when the nb of copied tracks' routings == nb of dest tracks, paste them respectively
		if ((!_snds || (_snds && _trs->GetSize()==_snds->GetSize())) && 
			(!_rcvs || (_rcvs && _trs->GetSize()==_rcvs->GetSize())))
		{
			updated |= PasteSendsReceives(true, _trs->Get(i), _snds, i, ps);
			updated |= PasteSendsReceives(false, _trs->Get(i), _rcvs, i, ps);
		}
		// otherwise: merge all copied routings into dest tracks
		//JFB TODO? "intra pasted routings" to manage?
		else
		{
			for (int j=0; _snds && j<_snds->GetSize(); j++)
				updated |= PasteSendsReceives(true, _trs->Get(i), _snds, j, ps);
			for (int j=0; _rcvs && j<_rcvs->GetSize(); j++)
				updated |= PasteSendsReceives(false, _trs->Get(i), _rcvs, j, ps);
		}
	}
	if (!_ps) ps->Empty(true); // + auto commit if needed
	return updated;
}

void CopyWithIOs(COMMAND_T* _ct)
{
	WDL_PtrList<MediaTrack> trs;
	SNM_GetSelectedTracks(NULL, &trs, false);
	if (trs.GetSize()) {
		CopySendsReceives(true, &trs, &g_sndTrackClipboard, &g_rcvTrackClipboard); // true: do no copy routings between sel. tracks
		Main_OnCommand(40210, 0); // copy sel tracks
	}
}

void CutWithIOs(COMMAND_T* _ct)
{
	WDL_PtrList<MediaTrack> trs;
	SNM_GetSelectedTracks(NULL, &trs, false);
	if (trs.GetSize()) 
	{
		Undo_BeginBlock2(NULL);
		CopySendsReceives(true, &trs, &g_sndTrackClipboard, &g_rcvTrackClipboard); // true: do no copy routings between sel. tracks
		Main_OnCommand(40337, 0); // cut sel tracks
		RefreshRoutingsUI();
		Undo_EndBlock2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL);
	}
}

void PasteWithIOs(COMMAND_T* _ct)
{
	if (int iTracks = GetNumTracks())
	{
		Undo_BeginBlock2(NULL);
		Main_OnCommand(40058, 0); // native track paste - note: this only preserves routings between pasted tracks
		if (iTracks != GetNumTracks()) // new/pasted tracks?
		{
			WDL_PtrList<MediaTrack> trs;
			SNM_GetSelectedTracks(NULL, &trs, false);
			PasteSendsReceives(&trs, &g_sndTrackClipboard, &g_rcvTrackClipboard, NULL); // false: we keep intra routings between pasted tracks, see above
		}
		RefreshRoutingsUI();
		Undo_EndBlock2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL);
	}
}

// routing cut copy/paste
void CopyRoutings(COMMAND_T* _ct)
{
	WDL_PtrList<MediaTrack> trs;
	SNM_GetSelectedTracks(NULL, &trs, false);
	if (trs.GetSize())
		CopySendsReceives(false, &trs, &g_sndClipboard, &g_rcvClipboard);
}

void CutRoutings(COMMAND_T* _ct)
{
	bool updated = false;
	WDL_PtrList<MediaTrack> trs;
	SNM_GetSelectedTracks(NULL, &trs, false);
	if (trs.GetSize())
	{
		WDL_PtrList<SNM_ChunkParserPatcher> ps;
		CopySendsReceives(false, &trs, &g_sndClipboard, &g_rcvClipboard);
		updated |= RemoveSends(&trs, &ps);
		updated |= RemoveReceives(&trs, &ps);
		ps.Empty(true); // auto-commit, if needed
	}
	if (updated) {
		RefreshRoutingsUI();
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
}

void PasteRoutings(COMMAND_T* _ct)
{
	WDL_PtrList<MediaTrack> trs;
	SNM_GetSelectedTracks(NULL, &trs, false);
	if (trs.GetSize() && PasteSendsReceives(&trs, &g_sndClipboard, &g_rcvClipboard, NULL)) {
		RefreshRoutingsUI();
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
}

// sends cut copy/paste
void CopySends(COMMAND_T* _ct)
{
	WDL_PtrList<MediaTrack> trs;
	SNM_GetSelectedTracks(NULL, &trs, false);
	if (trs.GetSize())
		CopySendsReceives(false, &trs, &g_sndClipboard, NULL);
}

void CutSends(COMMAND_T* _ct)
{
	bool updated = false;
	WDL_PtrList<MediaTrack> trs;
	SNM_GetSelectedTracks(NULL, &trs, false);
	if (trs.GetSize()) {
		CopySendsReceives(false, &trs, &g_sndClipboard, NULL);
		updated |= RemoveSends(&trs, NULL);
	}
	if (updated) {
		RefreshRoutingsUI();
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
}

void PasteSends(COMMAND_T* _ct)
{
	WDL_PtrList<MediaTrack> trs;
	SNM_GetSelectedTracks(NULL, &trs, false);
	if (trs.GetSize() && PasteSendsReceives(&trs, &g_sndClipboard, NULL, NULL)) {
		RefreshRoutingsUI();
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
}

// receives cut copy/paste
void CopyReceives(COMMAND_T* _ct)
{
	WDL_PtrList<MediaTrack> trs;
	SNM_GetSelectedTracks(NULL, &trs, false);
	if (trs.GetSize())
		CopySendsReceives(false, &trs, NULL, &g_rcvClipboard);
}

void CutReceives(COMMAND_T* _ct)
{
	bool updated = false;
	WDL_PtrList<MediaTrack> trs;
	SNM_GetSelectedTracks(NULL, &trs, false);
	if (trs.GetSize()) {
		CopySendsReceives(false, &trs, NULL, &g_rcvClipboard);
		updated |= RemoveReceives(&trs, NULL);
	}
	if (updated) {
		RefreshRoutingsUI();
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
}

void PasteReceives(COMMAND_T* _ct)
{
	WDL_PtrList<MediaTrack> trs;
	SNM_GetSelectedTracks(NULL, &trs, false);
	if (trs.GetSize() && PasteSendsReceives(&trs, NULL, &g_rcvClipboard, NULL)) {
		RefreshRoutingsUI();
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
}


///////////////////////////////////////////////////////////////////////////////
// Remove routing
///////////////////////////////////////////////////////////////////////////////

// primitive
bool RemoveSends(WDL_PtrList<MediaTrack>* _trs, WDL_PtrList<SNM_ChunkParserPatcher>* _ps)
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
		updated = RemoveSends(&trs, NULL);
	if (updated) {
		RefreshRoutingsUI();
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
}

// primitive
bool RemoveReceives(WDL_PtrList<MediaTrack>* _trs, WDL_PtrList<SNM_ChunkParserPatcher>* _ps)
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
		updated = RemoveReceives(&trs, NULL);
	if (updated) {
		RefreshRoutingsUI();
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
}

void RemoveRoutings(COMMAND_T* _ct)
{
	bool updated = false;
	WDL_PtrList<MediaTrack> trs;
	SNM_GetSelectedTracks(NULL, &trs, false);
	if (trs.GetSize())
	{
		WDL_PtrList<SNM_ChunkParserPatcher> ps;
		updated |= RemoveSends(&trs, &ps);
		updated |= RemoveReceives(&trs, &ps);
		ps.Empty(true); // auto-commit, if needed
	}
	if (updated) {
		RefreshRoutingsUI();
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1); 
	}
}


///////////////////////////////////////////////////////////////////////////////
// Other routing funcs/commands
// Note: the pref "defsendflag" also includes the mode (post/pre fader, etc..)
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

// primitive (no undo)
bool MuteSends(MediaTrack* _src, MediaTrack* _dest, bool _mute)
{
	bool updated = false;
	if (_src && _dest && _src!=_dest)
	{
		int sndIdx=0;
		MediaTrack* destTr = (MediaTrack*)GetSetTrackSendInfo(_src, 0, sndIdx, "P_DESTTRACK", NULL);
		while(destTr)
		{
			if (destTr==_dest && _mute != *(bool*)GetSetTrackSendInfo(_src, 0, sndIdx, "B_MUTE", NULL)) {
				GetSetTrackSendInfo(_src, 0, sndIdx, "B_MUTE", &_mute);
				updated = true;
			}
			destTr = (MediaTrack*)GetSetTrackSendInfo(_src, 0, ++sndIdx, "P_DESTTRACK", NULL);
		}
	}
	return updated;
}

// primitive (no undo)
bool MuteReceives(MediaTrack* _src, MediaTrack* _dest, bool _mute)
{
	bool updated = false;
	if (_src && _dest && _src!=_dest)
	{
		int rcvIdx=0;
		MediaTrack* src = (MediaTrack*)GetSetTrackSendInfo(_dest, -1, rcvIdx, "P_SRCTRACK", NULL);
		while(src)
		{
			if (src == _src && _mute != *(bool*)GetSetTrackSendInfo(_dest, -1, rcvIdx, "B_MUTE", NULL)) {
				GetSetTrackSendInfo(_dest, -1, rcvIdx, "B_MUTE", &_mute);
				updated = true;
			}
			src = (MediaTrack*)GetSetTrackSendInfo(_dest, -1, ++rcvIdx, "P_SRCTRACK", NULL);
		}
	}
	return updated;
}

bool HasReceives(MediaTrack* _src, MediaTrack* _dest)
{
	if (_src && _dest && _src != _dest)
	{
		int rcvIdx=0;
		MediaTrack* src = (MediaTrack*)GetSetTrackSendInfo(_dest, -1, rcvIdx, "P_SRCTRACK", NULL);
		while(src)
		{
			if (src == _src)
				return true;
			src = (MediaTrack*)GetSetTrackSendInfo(_dest, -1, ++rcvIdx, "P_SRCTRACK", NULL);
		}
	}
	return false;
}
