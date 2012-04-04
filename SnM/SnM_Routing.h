/******************************************************************************
/ SnM_Routing.h
/
/ Copyright (c) 2012 Jeffos
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

//#pragma once

#ifndef _SNM_ROUTING_H_
#define _SNM_ROUTING_H_

#include "SnM_ChunkParserPatcher.h" 


class SNM_SndRcv
{
public:
	SNM_SndRcv() {
		memcpy(&m_src, &GUID_NULL, sizeof(GUID));
		memcpy(&m_dest, &GUID_NULL, sizeof(GUID));
	}
	virtual ~SNM_SndRcv() {}
	bool FillIOFromReaper(MediaTrack* _src, MediaTrack* _dest, int _categ, int _idx) {
		memcpy(&m_src, CSurf_TrackToID(_src, false)>=0 ? (GUID*)GetSetMediaTrackInfo(_src, "GUID", NULL) : &GUID_NULL, sizeof(GUID));
		memcpy(&m_dest, CSurf_TrackToID(_dest, false)>=0 ? (GUID*)GetSetMediaTrackInfo(_dest, "GUID", NULL) : &GUID_NULL, sizeof(GUID));
		if (MediaTrack* tr = (_categ == -1 ? _dest : _src)) {
			m_mute = *(bool*)GetSetTrackSendInfo(tr, _categ, _idx, "B_MUTE", NULL);
			m_phase = *(bool*)GetSetTrackSendInfo(tr, _categ, _idx, "B_PHASE", NULL);
			m_mono = *(bool*)GetSetTrackSendInfo(tr, _categ, _idx, "B_MONO", NULL);
			m_vol = *(double*)GetSetTrackSendInfo(tr, _categ, _idx, "D_VOL", NULL);
			m_pan = *(double*)GetSetTrackSendInfo(tr, _categ, _idx, "D_PAN", NULL);
			m_panl = *(double*)GetSetTrackSendInfo(tr, _categ, _idx, "D_PANLAW", NULL);
			m_mode = *(int*)GetSetTrackSendInfo(tr, _categ, _idx, "I_SENDMODE", NULL);
			m_srcChan = *(int*)GetSetTrackSendInfo(tr, _categ, _idx, "I_SRCCHAN", NULL);
			m_destChan = *(int*)GetSetTrackSendInfo(tr, _categ, _idx, "I_DSTCHAN", NULL);
			m_midi = *(int*)GetSetTrackSendInfo(tr, _categ, _idx, "I_MIDIFLAGS", NULL);
			return true;
		}
		return false;
	}
	GUID m_src, m_dest;
	bool m_mute;
	int m_phase, m_mono, m_mode, m_srcChan, m_destChan, m_midi;
	double m_vol, m_pan, m_panl;
};

bool cueTrack(const char* _undoMsg, const char* _busName, int _type, bool _showRouting = true, int _soloDefeat = 1, char* _trTemplatePath = NULL, bool _sendToMaster = false, int* _hwOuts = NULL);
bool cueTrack(const char* _undoMsg, int _confId);
void cueTrack(COMMAND_T*);
void readCueBusIniFile(int _confId, char* _busName, int* _reaType, bool* _trTemplate, char* _trTemplatePath, bool* _showRouting, int* _soloDefeat, bool* _sendToMaster, int* _hwOuts);
void saveCueBusIniFile(int _confId, const char* _busName, int _type, bool _trTemplate, const char* _trTemplatePath, bool _showRouting, int _soloDefeat, bool _sendToMaster, int* _hwOuts);
void copySendsReceives(bool _cut, WDL_PtrList<MediaTrack>* _trs, WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _sends,  WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _rcvs);
bool pasteSendsReceives(WDL_PtrList<MediaTrack>* _trs, WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _sends,  WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _rcvs, bool _rcvReset, WDL_PtrList<SNM_ChunkParserPatcher>* _ps);
void copyWithIOs(COMMAND_T*);
void cutWithIOs(COMMAND_T*);
void pasteWithIOs(COMMAND_T*);
void copyRoutings(COMMAND_T*);
void cutRoutings(COMMAND_T*);
void pasteRoutings(COMMAND_T*);
void copySends(COMMAND_T*);
void cutSends(COMMAND_T*);
void pasteSends(COMMAND_T*);
void copyReceives(COMMAND_T*);
void cutReceives(COMMAND_T*);
void pasteReceives(COMMAND_T*);
bool removeSnd(WDL_PtrList<MediaTrack>* _trs, WDL_PtrList<SNM_ChunkParserPatcher>* _ps);
void removeSends(COMMAND_T*);
bool removeRcv(WDL_PtrList<MediaTrack>* _trs, WDL_PtrList<SNM_ChunkParserPatcher>* _ps);
void removeReceives(COMMAND_T*);
void removeRouting(COMMAND_T*);
void muteReceives(MediaTrack* _source, MediaTrack* _dest, bool _mute);

#endif
