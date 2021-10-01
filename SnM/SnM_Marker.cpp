/******************************************************************************
/ SnM_Marker.cpp
/
/ Copyright (c) 2013 and later Jeffos
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

#include "stdafx.h" 

#include "SnM.h"
#include "SnM_Marker.h"

#include <WDL/localize/localize.h>

///////////////////////////////////////////////////////////////////////////////
// Marker and region update listener
///////////////////////////////////////////////////////////////////////////////

DWORD g_mkrRgnNotifyTime = 0; // really approx (updated on timer)
WDL_PtrList<MarkerRegion> g_mkrRgnCache;
WDL_PtrList<SNM_MarkerRegionListener> g_mkrRgnListeners;

void RegisterToMarkerRegionUpdates(SNM_MarkerRegionListener* _listener)
{
	if (_listener && g_mkrRgnListeners.Find(_listener) < 0)
		g_mkrRgnListeners.Add(_listener);
}

void UnregisterToMarkerRegionUpdates(SNM_MarkerRegionListener* _listener)
{
	int idx = _listener ? g_mkrRgnListeners.Find(_listener) : -1;
	if (idx >= 0)
		g_mkrRgnListeners.Delete(idx, false);
}

// return a bitmask: &SNM_MARKER_MASK: marker update, &SNM_REGION_MASK: region update
int UpdateMarkerRegionCache()
{
	int updateFlags=0;
	int i=0, x=0, num, col; double pos, rgnend; const char* name; bool isRgn;

	// added/updated markers/regions?
	while ((x = EnumProjectMarkers3(NULL, x, &isRgn, &pos, &rgnend, &name, &num, &col)))
	{
		MarkerRegion* m = g_mkrRgnCache.Get(i);
		if (!m || (m && !m->Compare(isRgn, pos, rgnend, name, num, col)))
		{
			if (m) g_mkrRgnCache.Delete(i, true);
			g_mkrRgnCache.Insert(i, new MarkerRegion(isRgn, pos, rgnend, name, num, col));
			updateFlags |= (isRgn ? SNM_REGION_MASK : SNM_MARKER_MASK);
		}
		i++;
	}
	// removed markers/regions?
	for (int j=g_mkrRgnCache.GetSize()-1; j>=i; j--) {
		if (MarkerRegion* m = g_mkrRgnCache.Get(j))
			updateFlags |= (m->IsRegion() ? SNM_REGION_MASK : SNM_MARKER_MASK);
		g_mkrRgnCache.Delete(j, true);
	}
	// project time mode update?
	static int sPrevTimemode = *ConfigVar<int>("projtimemode");
	if (updateFlags != (SNM_MARKER_MASK|SNM_REGION_MASK))
		if (const ConfigVar<int> timemode = "projtimemode")
			if (*timemode != sPrevTimemode) {
				sPrevTimemode = *timemode;
				return SNM_MARKER_MASK|SNM_REGION_MASK;
			}
	return updateFlags;
}

// notify marker/region listeners?
// polled via SNM_CSurfRun()
void UpdateMarkerRegionRun()
{
	if (GetTickCount() > g_mkrRgnNotifyTime)
	{
		g_mkrRgnNotifyTime = GetTickCount() + SNM_MKR_RGN_UPDATE_FREQ;
		
		if (int sz=g_mkrRgnListeners.GetSize())
			if (int updateFlags = UpdateMarkerRegionCache())
				for (int i=sz-1; i>=0; i--)
					g_mkrRgnListeners.Get(i)->NotifyMarkerRegionUpdate(updateFlags);
	}
}


///////////////////////////////////////////////////////////////////////////////
// Marker/region helpers
///////////////////////////////////////////////////////////////////////////////

// returns the 1st marker or region index found at _pos
// note: relies on markers & regions indexed by positions
// _flags: &SNM_MARKER_MASK=marker, &SNM_REGION_MASK=region
int FindMarkerRegion(ReaProject* _proj, double _pos, int _flags, int* _idOut)
{
	bool isrgn;
	double dPos, dEnd;
	int x=0, lastx=0, num, foundId=-1, foundx=-1;
	while ((x = EnumProjectMarkers3(_proj, x, &isrgn, &dPos, &dEnd, NULL, &num, NULL)))
	{
		if ((!isrgn && _flags&SNM_MARKER_MASK) || (isrgn && _flags&SNM_REGION_MASK && _pos<=dEnd))
		{
			if (_pos >= dPos) {
				foundId = MakeMarkerRegionId(num, isrgn);
				foundx = lastx;
			}
			else
				break;
		}
		lastx=x;
	}
	if (_idOut) *_idOut = foundId;
	return foundx;
}


///////////////////////////////////////////////////////////////////////////////
// Marker/region "IDs"
///////////////////////////////////////////////////////////////////////////////

int MakeMarkerRegionId(int _num, bool _isrgn)
{
	// note: MSB is ignored so that the encoded number is always positive
	if (_num >= 0 && _num <= 0x3FFFFFFF) {
		_num |= ((_isrgn?1:0) << 30);
		return _num;
	}
	return -1;
}

int GetMarkerRegionIdFromIndex(ReaProject* _proj, int _idx)
{
	if (_idx >= 0)
	{
		int num; bool isrgn;
		if (EnumProjectMarkers2(_proj, _idx, &isrgn, NULL, NULL, NULL, &num))
			return MakeMarkerRegionId(num, isrgn);
	}
	return -1;
}

int GetMarkerRegionIndexFromId(ReaProject* _proj, int _id) 
{
	if (_id > 0)
	{
		int x=0, lastx=0, num=(_id&0x3FFFFFFF), num2; 
		bool isrgn = IsRegion(_id), isrgn2;
		while ((x = EnumProjectMarkers3(_proj, x, &isrgn2, NULL, NULL, NULL, &num2, NULL))) {
			if (num == num2 && isrgn == isrgn2)
				return lastx;
			lastx=x;
		}
	}
	return -1;
}

int GetMarkerRegionNumFromId(int _id) {
	return _id>0 ? (_id&0x3FFFFFFF) : -1;
}

bool IsRegion(int _id) {
	return (_id > 0 && (_id&0x40000000) != 0);
}

int EnumMarkerRegionById(ReaProject* _proj, int _id, bool* _isrgn, double* _pos, double* _end, const char** _name, int* _num, int* _color)
{
	if (_id > 0)
	{
		const char* name2;
		double pos2, end2;
		bool isrgn = IsRegion(_id), isrgn2;
		int  num=(_id&0x3FFFFFFF), x=0, lastx=0, num2, col2;
		while ((x = EnumProjectMarkers3(_proj, x, &isrgn2, &pos2, &end2, &name2, &num2, &col2)))
		{
			if (num == num2 && isrgn == isrgn2)
			{
				if (_isrgn)	*_isrgn = isrgn2;
				if (_pos)	*_pos = pos2;
				if (_end)	*_end = end2;
				if (_name)	*_name = name2;
				if (_num)	*_num = num2;
				if (_color)	*_color = col2;
				return lastx;
			}
            lastx=x;
		}
	}
	return -1;
}


///////////////////////////////////////////////////////////////////////////////
// Marker/region descs, menus, etc..
///////////////////////////////////////////////////////////////////////////////

bool GetMarkerRegionDesc(const char* _name, bool _isrgn, int _num, double _pos, double _end, int _flags, bool _wantNum, bool _wantName, bool _wantTime, char* _descOut, int _outSz)
{
	if (_descOut && _outSz &&
		((_isrgn && _flags&SNM_REGION_MASK) || (!_isrgn && _flags&SNM_MARKER_MASK)))
	{
		WDL_FastString desc;
		bool comma = !_wantNum;

		if (_wantNum)
			desc.SetFormatted(64, "%d", _num);

		if (_wantName && _name && *_name)
		{
			if (!comma) { desc.Append(": "); comma = true; }
			desc.Append(_name);
		}

		if (_wantTime)
		{
			if (!comma) { desc.Append(": "); comma = true; }
			char timeStr[64] = "";
			format_timestr_pos(_pos, timeStr, sizeof(timeStr), -1);
			desc.Append(" [");
			desc.Append(timeStr);
			if (_isrgn)
			{
				desc.Append(" -> ");
				format_timestr_pos(_end, timeStr, sizeof(timeStr), -1);
				desc.Append(timeStr);
			}
			desc.Append("]");
		}
		lstrcpyn(_descOut, desc.Get(), _outSz);
		return true;
	}
	return false;
}

// enumerates by id, returns next region/marker index or 0 when finished enumerating
// text formating inspired by the native popup "jump to marker"
// note: when _mask != SNM_MARKER_MASK|SNM_REGION_MASK callers must also check !*_descOut
int EnumMarkerRegionDescById(ReaProject* _proj, int _id, char* _descOut, int _outSz, int _flags, bool _wantNum, bool _wantName, bool _wantTime)
{
	if (_descOut && _outSz && _id > 0)
	{
		*_descOut = '\0';
		double pos, end; int num; bool isrgn; const char* name;
		int idx = EnumMarkerRegionById(_proj, _id, &isrgn, &pos, &end, &name, &num, NULL);
		if (idx>=0)
        {
            GetMarkerRegionDesc(name, isrgn, num, pos, end, _flags, _wantNum, _wantName, _wantTime, _descOut, _outSz);
		    return idx;
        }
	}
	return -1;
}

// enumerates by index, see remarks above
int EnumMarkerRegionDesc(ReaProject* _proj, int _idx, char* _descOut, int _outSz, int _flags, bool _wantNum, bool _wantName, bool _wantTime)
{
	if (_descOut && _outSz && _idx >= 0)
	{
		*_descOut = '\0';
		double pos, end; int num; bool isrgn; const char* name;
		int nextIdx = EnumProjectMarkers2(_proj, _idx, &isrgn, &pos, &end, &name, &num);
		if (nextIdx>0) GetMarkerRegionDesc(name, isrgn, num, pos, end, _flags, _wantNum, _wantName, _wantTime, _descOut, _outSz);
		return nextIdx;
	}
	return 0;
}

// _flags: &1=marker, &2=region
void FillMarkerRegionMenu(ReaProject* _proj, HMENU _menu, int _msgStart, int _flags, UINT _uiState)
{
	int x=0, lastx=0;
	char desc[SNM_MAX_MARKER_NAME_LEN]="";
	while ((x = EnumMarkerRegionDesc(_proj, x, desc, SNM_MAX_MARKER_NAME_LEN, _flags, true, true, true))) {
		if (*desc) AddToMenu(_menu, desc, _msgStart+lastx, -1, false, _uiState);
		lastx=x;
	}
	if (!GetMenuItemCount(_menu))
		AddToMenu(_menu, __LOCALIZE("(No region!)","sws_menu"), 0, -1, false, MF_GRAYED);
}


///////////////////////////////////////////////////////////////////////////////
// Marker/region actions
///////////////////////////////////////////////////////////////////////////////

bool GotoMarkerRegion(ReaProject* _proj, int _num, int _flags, bool _select = false)
{
	bool isrgn; double pos, end;
	int x=0, n; 
	while ((x = EnumProjectMarkers3(_proj, x, &isrgn, &pos, &end, NULL, &n, NULL)))
		if (n == _num && ((!isrgn && _flags&SNM_MARKER_MASK) || (isrgn && _flags&SNM_REGION_MASK)))
		{
			PreventUIRefresh(1);

			if (_select && isrgn && (_flags&SNM_REGION_MASK))
				GetSet_LoopTimeRange2(NULL, true, true, &pos, &end, false); // seek is managed below

			const int opt = ConfigVar<int>("smoothseek").value_or(0); // obeys smooth seek
			SetEditCurPos2(_proj, pos, true, opt); // includes an undo point, if enabled in prefs

			PreventUIRefresh(-1);

			return true;
		}
	return false;
}

void GotoMarker(COMMAND_T* _ct) {
	if (GotoMarkerRegion(NULL, ((int)_ct->user)+1, SNM_MARKER_MASK))
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}

void GotoRegion(COMMAND_T* _ct) {
	if (GotoMarkerRegion(NULL, ((int)_ct->user)+1, SNM_REGION_MASK))
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}

void GotoAnsSelectRegion(COMMAND_T* _ct) {
	if (GotoMarkerRegion(NULL, ((int)_ct->user)+1, SNM_REGION_MASK, true))
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}

void InsertMarker(COMMAND_T* _ct)
{
	AddProjectMarker2(NULL, false, (int)_ct->user && (GetPlayStateEx(NULL)&1) ? GetPlayPositionEx(NULL) : GetCursorPositionEx(NULL), 0.0, "", -1, 0);
	UpdateTimeline();
	Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_MISCCFG, -1);
}

TempoMarker::TempoMarker(double _dTimePos, int _measurePos, double _dBeatPos, double _dBpm, int _timesig_num, int _timesig_denom, bool _bLinearTempo, RawTempoMarkerData _rawData)
{
	m_dTimePos = _dTimePos;
	m_measurePos = _measurePos;
	m_dBeatPos = _dBeatPos;
	m_dBpm = _dBpm;
	m_timesig_num = _timesig_num;
	m_timesig_denom = _timesig_denom;
	m_bLinearTempo = _bLinearTempo;
	m_rawData = _rawData;
}

bool GetAllTempoMarkers(WDL_PtrList<void>* _tempoMarkers, ReaProject* _proj)
{
	if (_tempoMarkers)
	{
		_tempoMarkers->Empty();
		double dTimePosOut;
		int measurePosOut;
		double dBeatPosOut;
		double dBpmOut;
		int timesig_numOut;
		int timesig_denomOut;
		bool bLinearTempoOut;

		map<int, RawTempoMarkerData> tempoDataMap;

		MediaTrack* masterTrack = GetMasterTrack(_proj);
		TrackEnvelope* tempoEnvEx = GetTrackEnvelope(masterTrack, 0);
		string tempoStateStr = envelope::GetEnvelopeStateChunkBig(tempoEnvEx);
		char* envState = &tempoStateStr[0];
		int index = 0;

		char* token = strtok(envState, "\n");

		while (token != NULL)
		{
			RawTempoMarkerData newTempoMarker = RawTempoMarkerData();
			if (sscanf(token, "PT %lf %lf %d %u %d %d %d %s %u %u", &newTempoMarker.position, &newTempoMarker.bpm, &newTempoMarker.linearBool, &newTempoMarker.beatDivision, &newTempoMarker.intData4, &newTempoMarker.settingsBitmask, &newTempoMarker.intData6, newTempoMarker.stringData7, &newTempoMarker.uintData8, &newTempoMarker.metronomePattern) >= 2)
			{
				tempoDataMap[index] = newTempoMarker;
				index++;
			}
			token = strtok(NULL, "\n");
		}

		for (int i = 0; i <= CountTempoTimeSigMarkers(_proj); i++)
		{
			if (GetTempoTimeSigMarker(_proj, i, &dTimePosOut, &measurePosOut, &dBeatPosOut, &dBpmOut, &timesig_numOut, &timesig_denomOut, &bLinearTempoOut))
			{
				if (tempoDataMap.find(i) != tempoDataMap.end())
				{
					TempoMarker* tempoMarker = new TempoMarker(dTimePosOut, measurePosOut, dBeatPosOut, dBpmOut, timesig_numOut, timesig_denomOut, bLinearTempoOut, tempoDataMap.find(i)->second);
					_tempoMarkers->Add(tempoMarker);
				}
			}
		}
	}
	return (_tempoMarkers && _tempoMarkers->GetSize());
}

bool GetTempoMarkersInInterval(WDL_PtrList<void>* _tempoMarkers, ReaProject* _proj, double _pos1, double _pos2)
{
	if (_tempoMarkers)
	{
		_tempoMarkers->Empty();
		double dTimePosOut;
		int measurePosOut;
		double dBeatPosOut;
		double dBpmOut;
		int timesig_numOut;
		int timesig_denomOut;
		bool bLinearTempoOut;

		map<int, RawTempoMarkerData> tempoDataMap;

		MediaTrack* masterTrack = GetMasterTrack(_proj);
		TrackEnvelope* tempoEnvEx = GetTrackEnvelope(masterTrack, 0);
		string tempoStateStr = envelope::GetEnvelopeStateChunkBig(tempoEnvEx);
		char* envState = &tempoStateStr[0];
		int index = 0;

		char* token = strtok(envState, "\n");

		while (token != NULL)
		{
			RawTempoMarkerData newTempoMarker = RawTempoMarkerData();
			if (sscanf(token, "PT %lf %lf %d %u %d %d %d %s %u %u", &newTempoMarker.position, &newTempoMarker.bpm, &newTempoMarker.linearBool, &newTempoMarker.beatDivision, &newTempoMarker.intData4, &newTempoMarker.settingsBitmask, &newTempoMarker.intData6, newTempoMarker.stringData7, &newTempoMarker.uintData8, &newTempoMarker.metronomePattern) >= 2)
			{
				tempoDataMap[index] = newTempoMarker;
				index++;
			}
			token = strtok(NULL, "\n");
		}

		for (int i = 0; i <= CountTempoTimeSigMarkers(_proj); i++)
		{
			if (GetTempoTimeSigMarker(_proj, i, &dTimePosOut, &measurePosOut, &dBeatPosOut, &dBpmOut, &timesig_numOut, &timesig_denomOut, &bLinearTempoOut))
			{
				if (tempoDataMap.find(i) != tempoDataMap.end())
				{
					TempoMarker* tempoMarker = new TempoMarker(dTimePosOut, measurePosOut, dBeatPosOut, dBpmOut, timesig_numOut, timesig_denomOut, bLinearTempoOut, tempoDataMap.find(i)->second);
					if (IsTempoMarkerInInterval(tempoMarker, _pos1, _pos2))
						_tempoMarkers->Add(tempoMarker);
				}
			}
		}			
	}
	return (_tempoMarkers && _tempoMarkers->GetSize());
}

bool IsTempoMarkerInInterval(TempoMarker* _tempoMarker, double _pos1, double _pos2)
{
	if (_tempoMarker)
	{
		double pos = _tempoMarker->GetTimePosition();
		if (pos >= _pos1 && pos <= _pos2) // is within?
			return true;
	}
	return false;
}

bool DuplicateTempoMarkers(WDL_PtrList<void>* _tempoMarkers, ReaProject* _proj, const char* _undoTitle, double _nudgePos)
{
	bool updated = false;

	if (_undoTitle)
		Undo_BeginBlock2(NULL);

	if (_tempoMarkers)
	{
		std::map<double, RawTempoMarkerData> newMarkersPosition;

		WDL_PtrList<void> oldTempoMarkers = *_tempoMarkers;

		for (int i = 0; i < oldTempoMarkers.GetSize(); i++)
		{
			if (TempoMarker* oldTempoMarker = (TempoMarker*)oldTempoMarkers.Get(i)) 
			{
				double oldPos = oldTempoMarker->GetTimePosition();
				double newPos = oldPos + _nudgePos;
				SetTempoTimeSigMarker(_proj, -1, newPos,-1,-1, oldTempoMarker->GetBPM(), oldTempoMarker->GetTimeSignatureNumerator(), oldTempoMarker->GetTimeSignatureDenominator(), oldTempoMarker->IsLinearTempo());

				newMarkersPosition[newPos]= oldTempoMarker->GetRawMarkerData();
			}
		}

		double dTimePosOut;
		int measurePosOut;
		double dBeatPosOut;
		double dBpmOut;
		int timesig_numOut;
		int timesig_denomOut;
		bool bLinearTempoOut;
		std::map<int, RawTempoMarkerData> newMarkersId;
		for (int i = 0; i < CountTempoTimeSigMarkers(_proj); i++)
		{
			if (GetTempoTimeSigMarker(_proj, i, &dTimePosOut, &measurePosOut, &dBeatPosOut, &dBpmOut, &timesig_numOut, &timesig_denomOut, &bLinearTempoOut)) 
			{
				if (newMarkersPosition.find(dTimePosOut) != newMarkersPosition.end()) 
				{
					newMarkersId[i] = newMarkersPosition.find(dTimePosOut)->second;
				}
			}
		}

		MediaTrack* masterTrack = GetMasterTrack(_proj);
		TrackEnvelope* tempoEnvEx = GetTrackEnvelope(masterTrack, 0);
		string tempoStateStr = envelope::GetEnvelopeStateChunkBig(tempoEnvEx);
		char* envState = &tempoStateStr[0];
		string newState = "";
		int index = 0;
		double position;
		int data5;
		char* token = strtok(envState, "\n");

		while (token != NULL)
		{
			int foundValues = sscanf(token, "PT %lf %*lf %*d %*u %*d %d %*d %*s %*u %*u", &position, &data5);
			if (foundValues >= 1)
			{
				std::map<int, RawTempoMarkerData>::iterator findResult = newMarkersId.find(index);

				if (findResult != newMarkersId.end())
				{
					string newToken = token;
					char appendString[2048];
					if (foundValues == 1)
					{
						sprintf(appendString, " %u %d", findResult->second.beatDivision, findResult->second.intData4);
						newToken.append(appendString);
					}
					else
					{
						newToken = newToken.substr(0, newToken.size() - 2);
					}

					sprintf(appendString, " %d %d %s %u %u", findResult->second.settingsBitmask, findResult->second.intData6, findResult->second.stringData7, findResult->second.uintData8, findResult->second.metronomePattern);
					newToken.append(appendString);
					newState.append(newToken);
					newState.append("\n");
					token = strtok(NULL, "\n");
					index++;
					continue;
				}

				index++;
			}
			newState.append(token);
			newState.append("\n");
			token = strtok(NULL, "\n");
		}

		SetEnvelopeStateChunk(tempoEnvEx, newState.c_str(), false);

	}

	if (_undoTitle) {
		UpdateArrange();
		Undo_EndBlock2(NULL, _undoTitle, UNDO_STATE_ALL);
	}
	return updated;
}

