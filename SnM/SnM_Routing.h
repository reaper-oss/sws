/******************************************************************************
/ SnM_Routing.h
/
/ Copyright (c) 2012 and later Jeffos
/
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


class SNM_SndRcv
{
public:
	SNM_SndRcv() {
		memcpy(&m_src, &GUID_NULL, sizeof(GUID));
		memcpy(&m_dest, &GUID_NULL, sizeof(GUID));
	}
	virtual ~SNM_SndRcv() {}
	bool FillIOFromReaper(MediaTrack* _src, MediaTrack* _dest, int _categ, int _idx) {
		memcpy(&m_src, _src && CSurf_TrackToID(_src, false)>=0 ? (GUID*)GetSetMediaTrackInfo(_src, "GUID", NULL) : &GUID_NULL, sizeof(GUID));
		memcpy(&m_dest, _dest && CSurf_TrackToID(_dest, false)>=0 ? (GUID*)GetSetMediaTrackInfo(_dest, "GUID", NULL) : &GUID_NULL, sizeof(GUID));
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


typedef WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv>> SndRcvClipboard;
void CopySendsReceives(bool _noIntra, WDL_PtrList<MediaTrack>* _trs, SndRcvClipboard* _snds, SndRcvClipboard* _rcvs, bool forceFlush = true);
bool PasteSendsReceives(WDL_PtrList<MediaTrack>* _trs, SndRcvClipboard* _snds, SndRcvClipboard* _rcvs, WDL_PtrList<SNM_ChunkParserPatcher>* _ps);
void CopyWithIOs(COMMAND_T*);
void CutWithIOs(COMMAND_T*);
void PasteWithIOs(COMMAND_T*);
void CopyRoutings(COMMAND_T*);
void CutRoutings(COMMAND_T*);
void PasteRoutings(COMMAND_T*);
void CopySends(COMMAND_T*);
void CutSends(COMMAND_T*);
void PasteSends(COMMAND_T*);
void CopyReceives(COMMAND_T*);
void CutReceives(COMMAND_T*);
void PasteReceives(COMMAND_T*);

bool RemoveSends(WDL_PtrList<MediaTrack>* _trs, WDL_PtrList<SNM_ChunkParserPatcher>* _ps);
void RemoveSends(COMMAND_T*);
bool RemoveReceives(WDL_PtrList<MediaTrack>* _trs, WDL_PtrList<SNM_ChunkParserPatcher>* _ps);
void RemoveReceives(COMMAND_T*);
void RemoveRoutings(COMMAND_T*);

// reascript export
bool SNM_AddReceive(MediaTrack* _srcTr, MediaTrack* _destTr, int _type);
bool SNM_RemoveReceive(MediaTrack* _tr, int _rcvIdx);
bool SNM_RemoveReceivesFrom(MediaTrack* _tr, MediaTrack* _srcTr);

void SaveDefaultTrackSendPrefs(COMMAND_T*);
void RecallDefaultTrackSendPrefs(COMMAND_T*);
void SetDefaultTrackSendPrefs(COMMAND_T*);

bool MuteSends(MediaTrack* _src, MediaTrack* _dest, bool _mute);
bool MuteReceives(MediaTrack* _src, MediaTrack* _dest, bool _mute);
bool HasReceives(MediaTrack* _src, MediaTrack* _dest);

#endif
