/******************************************************************************
/ BR_TempoTools.cpp
/
/ Copyright (c) 2013 Dominik Martin Drzic
/ http://forum.cockos.com/member.php?u=27094
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
#include "BR_TempoTools.h"

BR_TempoChunk::BR_TempoChunk(bool effCreate /*= false*/)
{
	m_count = CountTempoTimeSigMarkers(NULL);
	m_tempoPoints.reserve(m_count);
	m_selectedPoints.reserve(m_count);
	
	// Get preferences 
	m_preferences.envClickSegMode = *(int*)GetConfigVar("envclicksegmode"); 
	GetWindowRect(FindWindowEx(g_hwndParent,0,"REAPERTrackListWindow","trackview"), &m_preferences.r);
	GetSet_ArrangeView2(NULL, false, m_preferences.r.left, m_preferences.r.right, &m_preferences.start_time, &m_preferences.end_time);
	m_effCreate = effCreate;

	// Set preferences
	*(int*)GetConfigVar("envclicksegmode") = 1; //prevent reselection of points in time selection

	// Get tempo envelope
	m_envelope = SWS_GetTrackEnvelopeByName(CSurf_TrackFromID(0, false), "Tempo map" );
	char* envState = GetSetObjectState(m_envelope, "");
	char* token = strtok(envState, "\n");
	
	// Parse envelope
	LineParser lp(false);
	bool start = false;
	int ID = -1;
	while(token != NULL)
	{
		lp.parse(token);
		if (strcmp(lp.gettoken_str(0),  "PT") == 0)
		{	
			start = true;
			ID++;
			TempoPoint tempoPoint /*={} is not needed because unexisting tokens will be 0 anyways*/;
			tempoPoint.position = lp.gettoken_float(1);
			tempoPoint.bpm 		= lp.gettoken_float(2);
			tempoPoint.shape 	= lp.gettoken_int(3);
			tempoPoint.sig 		= lp.gettoken_int(4);
			tempoPoint.selected = lp.gettoken_int(5);
			tempoPoint.partial 	= lp.gettoken_int(6);			
			m_tempoPoints.push_back(tempoPoint);
			if (tempoPoint.selected == 1)
				m_selectedPoints.push_back(ID);			
		}
		else
		{
			if (!start)
			{
				m_chunkStart.Append(token);
				m_chunkStart.Append("\n");
			}
			else
			{
				m_chunkEnd.Append(token);
				m_chunkEnd.Append("\n");
			}
		}			
		token = strtok(NULL, "\n");	
	}
	FreeHeapPtr(envState);
};

BR_TempoChunk::~BR_TempoChunk()
{
	// Restore preferences
	*(int*)GetConfigVar("envclicksegmode") = m_preferences.envClickSegMode;
	GetSet_ArrangeView2(NULL, true, m_preferences.r.left, m_preferences.r.right, &m_preferences.start_time, &m_preferences.end_time);
};

void BR_TempoChunk::Commit()
{
	// Update chunk
	vector<TempoPoint>::const_iterator it = m_tempoPoints.begin(), end = m_tempoPoints.end();
	for (; it != end; ++it)
	{
		m_chunkStart.AppendFormatted (	
									 	256, 
									 	"PT %.16f %.16f %d %d %d %d\n",
									 	it->position,
									 	it->bpm, 
									 	it->shape, 
									 	it->sig, 
									 	it->selected, 
									 	it->partial
									 );
	}
	m_chunkStart.Append(m_chunkEnd.Get());	
	GetSetObjectState(m_envelope, m_chunkStart.Get());

	// Refresh tempo map (UpdateTimeline() doesn't work on it's own when setting chunk with edited values)
	double t, b; int n, d; bool s;
	GetTempoTimeSigMarker(NULL, 0, &t, NULL, NULL, &b, &n, &d, &s);
	SetTempoTimeSigMarker(NULL, 0, t, -1, -1, b, n, d, s);
	UpdateTimeline();
};

bool BR_TempoChunk::GetPoint(int id, double* position, double* bpm, bool* linear)
{
	if (id < 0 || id >= m_count)
	{
		if (position != NULL)
			*position = 0;
		if (bpm != NULL)
			*bpm = 0;
		if (linear != NULL)
			*linear = false;
		return false;
	}	
	else
	{
		if (position != NULL)
			*position = m_tempoPoints[id].position;
		if (bpm != NULL)
			*bpm = m_tempoPoints[id].bpm;
		if (linear != NULL)
			*linear = !(m_tempoPoints[id].shape);		
	}
	return true;
};

bool BR_TempoChunk::SetPoint(int id, double position, double bpm, bool linear)
{
	if (id < 0 || id >= m_count)
		return false;
	else
	{
		m_tempoPoints[id].position = position;
		m_tempoPoints[id].bpm = bpm;
		m_tempoPoints[id].shape = !linear;		
	}
	return true;
};

bool BR_TempoChunk::GetTimeSig(int id, bool* sig, int* num, int* den)
{
	if (id < 0 || id >= m_count)
	{
		if (sig != NULL)
			*sig = false;
		if (num != NULL)
			*num = 0;
		if (den != NULL)
			*den = 0;
		return false;
	}
	else
	{
		if (sig != NULL)
			if (m_tempoPoints[id].sig == 0)
				*sig = false;
			else
				*sig = true;
		
		if (num != NULL || den != NULL)
		{
			__int32 effSig = 0;
			for (;id >= 0; --id)
			{
				if (m_tempoPoints[id].sig != 0)
				{
					effSig = m_tempoPoints[id].sig;
					break;
				}
			}

			int effNum, effDen;
			if (effSig != 0)
			{
				effNum = effSig & 0xff;
				effDen = effSig >> 16;
			}
			else // using time lower than 0 will give project time signature set in project preferences
				TimeMap_GetTimeSigAtTime(NULL, -1, &effNum, &effDen, NULL);

			if (num != NULL)
				*num = effNum;
			if (den != NULL)
				*den = effDen;
		}
		return true;
	}
};

bool BR_TempoChunk::SetTimeSig(int id, bool sig, int num, int den)
{
	if (id < 0 || id >= m_count)
		return false;
	else
	{
		_int32 effSig;
		if (sig)
		{
			// Unlike native method, this function will fail when illegal num/den are requested
			if (num <= 0 || num > 255 || den <= 0 || den > 255)
				return false;							
			effSig = (den << 16) + num;					// Partial token needs to be set in accordance with time sig:
			if (m_tempoPoints[id].partial == 0) 		//	-sig  -partial
				m_tempoPoints[id].partial = 1;  		//	+sig  -partial
			else if (m_tempoPoints[id].partial == 4)	//	-sig  +partial	
				m_tempoPoints[id].partial = 5;			//	+sig  +partial
		}
		else
		{
			effSig = 0;
			if (m_tempoPoints[id].partial == 1) 		//	+sig  -partial
				m_tempoPoints[id].partial = 0;  		//	-sig  -partial
			else if (m_tempoPoints[id].partial == 5)	//	+sig  +partial	
				m_tempoPoints[id].partial = 4;			//	-sig  +partial
		}
		m_tempoPoints[id].sig = effSig;
	}
	return true;
};

bool BR_TempoChunk::GetSelection (int id)
{
	if (id < 0 || id >= m_count)
		return false;
	else
		return !!m_tempoPoints[id].selected;
};

bool BR_TempoChunk::SetSelection (int id, bool select)
{
	if (id < 0 || id >= m_count)
		return false;
	else
		m_tempoPoints[id].selected = select;
	return true;
};

bool BR_TempoChunk::CreatePoint(int id, double position, double bpm, bool linear)
{
	if ((id < 0 || id >= m_count) && !m_effCreate)
		return false;
	else
	{
		TempoPoint newPoint= {};
		newPoint.position = position;
		newPoint.bpm = bpm;
		newPoint.shape = !linear;

		if (m_effCreate)
			m_tempoPoints.push_back(newPoint);
		else
			m_tempoPoints.insert(m_tempoPoints.begin() + (id+1), newPoint);			
		++m_count;

		return true;	
	}
};

bool BR_TempoChunk::DeletePoint(int id)
{
	if (id < 0 || id >= m_count)
		return false;
	else
	{
		m_tempoPoints.erase (m_tempoPoints.begin()+id);
		--m_count;
	}
	return true;
};

int BR_TempoChunk::CountPoints()
{
	return (int)m_tempoPoints.size();
};

const vector<int>* BR_TempoChunk::GetSelected() const
{
	return &m_selectedPoints;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//							VARIOUS FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
vector<int> GetSelectedTempo ()
{
	vector<int> selectedTempoPoints;
	LineParser lp(false);
	int ID = -1;
		
	TrackEnvelope* envelope = SWS_GetTrackEnvelopeByName(CSurf_TrackFromID(0, false), "Tempo map" );
	char* envState = GetSetObjectState(envelope, "");
	char* token = strtok(envState, "\n");
	
	while(token != NULL)
	{
		lp.parse(token);
		if (strcmp(lp.gettoken_str(0),  "PT") == 0)
		{	
			++ID;
			if (lp.gettoken_int(5) == 1)
				selectedTempoPoints.push_back(ID);
		}
		token = strtok(NULL, "\n");	
	}
	
	FreeHeapPtr(envState);
	return selectedTempoPoints;
};

double TempoAtPosition (double startBpm, double endBpm, double startTime, double endTime, double targetTime)
{
	if (startTime >= endTime || targetTime < startTime || targetTime > endTime)
		return -1;
	if (startBpm == endBpm)
		return startBpm;

	double bpmDiff = (endBpm-startBpm) / (endTime-startTime) * (targetTime-startTime);
	return startBpm + bpmDiff;	
};

double MeasureAtPosition (double startBpm, double endBpm, double length, double targetTime)
{
	// return number of measures counted from the musical position of startBpm
	double mLength = (startBpm+endBpm)*length / 480;
	return targetTime * (length-targetTime) * (startBpm-endBpm) / (480*length) + (mLength*targetTime/length);	
};

double PositionAtMeasure (double startBpm, double endBpm, double length, double targetMeasure)
{
	// return number of seconds counted from the time position of startBpm
	if (startBpm == endBpm)
		return 240*targetMeasure / startBpm;
	else
	{
		double mLength = (startBpm+endBpm)*length / 480;
		double a = (startBpm-endBpm);
		double b = length*a + 480*mLength;
		double c = 480*targetMeasure*length;
		return (b - sqrt(pow(b,2) - 4*a*c)) / (2*a);
	}
};

void FindMiddlePoint (double &middleTime, double &middleBpm, const double &measure, const double &startBpm, const double &endBpm, const double &startTime, const double &endTime)
{
	double f = 480 * (measure/2); 
	double bp = startBpm - endBpm; 
	if (bp == 0)
		bp -= 0.0000001;
	double a = bp*(startTime+endTime) + f*2;
	double b = bp*(startTime*endTime) + f*(startTime+endTime);
	middleTime = (a - sqrt(pow(a,2) - 4*bp*b)) / (2*bp);
	middleBpm = f / (middleTime-startTime) - startBpm;
};

void SplitMiddlePoint (double &time1, double &time2, double &bpm1, double &bpm2, const double &splitRatio, const double &measure, const double &startBpm, const double &middleBpm, const double &endBpm, const double &startTime, const double &middleTime, const double &endTime)
{
	double temp = measure*(1-splitRatio)/2;

	// First point
	time1 = startTime + PositionAtMeasure (startBpm, middleBpm, (middleTime - startTime), temp);
	bpm1 = TempoAtPosition (startBpm, middleBpm, startTime, middleTime, time1);
			
	// Second point		
	double f1 = 480 * (measure - temp*2); 
	double f2 = 480 * temp;
	double bp = bpm1 - endBpm;
	if (bp == 0)
		bp -= 0.0000001;
	double a = bp*(time1+endTime) + f1+f2;
	double b = bp*(time1*endTime) + f1*endTime + f2*time1;
	time2 = (a - sqrt(pow(a,2) - 4*bp*b)) / (2*bp);
	bpm2 = f1 / (time2-time1) - bpm1;				
};