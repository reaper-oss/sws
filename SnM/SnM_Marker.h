/******************************************************************************
/ SnM_Marker.h
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

//#pragma once

#ifndef _SNM_MARKER_H_
#define _SNM_MARKER_H_

#include "../MarkerList/MarkerListClass.h"


// register/unregister to marker/region changes
class SNM_MarkerRegionListener {
public:
	SNM_MarkerRegionListener() {}
	virtual ~SNM_MarkerRegionListener() {}
	// _updateFlags: &1 marker update, &2 region update
	virtual void NotifyMarkerRegionUpdate(int _updateFlags) {}
};

void RegisterToMarkerRegionUpdates(SNM_MarkerRegionListener* _sub);
void UnregisterToMarkerRegionUpdates(SNM_MarkerRegionListener* _sub) ;
void UpdateMarkerRegionRun();

int FindMarkerRegion(ReaProject* _proj, double _pos, int _flags, int* _idOut = NULL);
int MakeMarkerRegionId(int _num, bool _isRgn);
int GetMarkerRegionIdFromIndex(ReaProject* _proj, int _idx);
int GetMarkerRegionIndexFromId(ReaProject* _proj, int _id);
int GetMarkerRegionNumFromId(int _id);
bool IsRegion(int _id);
int EnumMarkerRegionById(ReaProject* _proj, int _id, bool* _isrgn, double* _pos, double* _end, const char** _name, int* _num, int* _color);
int EnumMarkerRegionDescById(ReaProject* _proj, int _id, char* _descOut, int _outSz, int _flags, bool _wantNum, bool _wantName, bool _wantTime = true);
int EnumMarkerRegionDesc(ReaProject* _proj, int _idx, char* _descOut, int _outSz, int _flags, bool _wantNum, bool _wantName, bool _wantTime = true);
void FillMarkerRegionMenu(ReaProject* _proj, HMENU _menu, int _msgStart, int _flags, UINT _uiState = 0);

void GotoMarker(COMMAND_T*);
void GotoRegion(COMMAND_T*);
void GotoAnsSelectRegion(COMMAND_T*);
void InsertMarker(COMMAND_T*);

class MarkerRegion : public MarkerItem {
public:
	MarkerRegion(bool _bReg, double _dPos, double _dRegEnd, const char* _cName, int _num, int _color)
		: MarkerItem(_bReg, _dPos, _dRegEnd, _cName, _num, _color) { m_id=MakeMarkerRegionId(_num, _bReg); }
	int GetId() { return m_id; }
protected:
	int m_id;
};

//Handle tempo markers for region playlist
struct RawTempoMarkerData
{
	double position = 0;
	double bpm = 0;
	int linearBool = 0;
	unsigned int beatDivision = 0; //High 16 bits are time signature bottom, low 16 bits are time signature top
	int selectionBool = 0; //Is this point selected
	int settingsBitmask = 0;
	int bezierTension = 0; //Not used for tempo markers
	char quantizationSettings[10]; //Not sure how this works but I guess that if you quantize the tempo marker it would change this.
	unsigned int metronomePatternL32 = 0;
	unsigned int metronomePatternH32 = 0;
	int beatBase = 0;
};

class TempoMarker
{
public:
	TempoMarker(double _dTimePos, int _measurePos, double _dBeatPos, double _dBpm, int _timesig_num, int _timesig_denom, bool _bLinearTempo, RawTempoMarkerData _rawData);
	~TempoMarker() {}

	double GetTimePosition() { return m_dTimePos; }
	void SetTimePosition(double dTimePos) { m_dTimePos = dTimePos; }
	int GetMeasurePosition() { return m_measurePos; }
	void SetMeasurePosition(int measurePos) { m_measurePos = measurePos; }
	double GetBeatPosition() { return m_dBeatPos; }
	void SetBeatPosition(double dBeatPos) { m_dBeatPos = dBeatPos; }
	double GetBPM() { return m_dBpm; }
	void SetBPM(double dBpm) { m_dBpm = dBpm; }
	int GetTimeSignatureNumerator() { return m_timesig_num; }
	void SetTimeSignatureNumerator(int timesig_num) { m_timesig_num = timesig_num; }
	int GetTimeSignatureDenominator() { return m_timesig_denom; }
	void SetTimeSignatureDenominator(int timesig_denom) { m_timesig_denom = timesig_denom; }
	bool IsLinearTempo() { return m_bLinearTempo; }
	void SetIsLinearTempo(bool bLinearTempo) { m_bLinearTempo = bLinearTempo; }
	RawTempoMarkerData GetRawMarkerData() { return m_rawData; }
	void SetRawMarkerData(RawTempoMarkerData rawData) { m_rawData = rawData; };

protected:
	double m_dTimePos;
	int m_measurePos;
	double m_dBeatPos;
	double m_dBpm;
	int m_timesig_num;
	int m_timesig_denom;
	bool m_bLinearTempo;
	RawTempoMarkerData m_rawData;

};



bool GetAllTempoMarkers(WDL_PtrList<void>* _tempoMarkers, ReaProject* _proj);
bool GetTempoMarkersInInterval(WDL_PtrList<void>* _tempoMarkers, ReaProject* _proj, double _pos1, double _pos2);
bool IsTempoMarkerInInterval(TempoMarker* _tempoMarker, double _pos1, double _pos2);
bool DuplicateTempoMarkers(WDL_PtrList<void>* _tempoMarkers, ReaProject* _proj, const char* _undoTitle, double _nudgePos);
#endif
