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

BR_TempoChunk::BR_TempoChunk()
{
	count = CountTempoTimeSigMarkers(NULL);
	tempoPoints.reserve(count);
	selectedPoints.reserve(count);
	
	// Get preferences 
	preferences.envClickSegMode = *(int*)GetConfigVar("envclicksegmode"); 
	GetWindowRect(FindWindowEx(g_hwndParent,0,"REAPERTrackListWindow","trackview"), &preferences.r);
	GetSet_ArrangeView2(NULL, false, preferences.r.left, preferences.r.right, &preferences.start_time, &preferences.end_time);

	// Set preferences
	*(int*)GetConfigVar("envclicksegmode") = 1; //prevent reselection of points in time selection

	// Get tempo envelope
	envelope = SWS_GetTrackEnvelopeByName(CSurf_TrackFromID(0, false), "Tempo map" );
	char* envState = GetSetObjectState(envelope, "");
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
			TempoPoint tempoPoint;
			tempoPoint.position = lp.gettoken_float(1);
			tempoPoint.bpm 		= lp.gettoken_float(2);
			tempoPoint.shape 	= lp.gettoken_int(3);
			tempoPoint.sig 		= lp.gettoken_int(4);
			tempoPoint.selected = lp.gettoken_int(5);
			tempoPoint.partial 	= lp.gettoken_int(6);			
			tempoPoints.push_back(tempoPoint);
			if (tempoPoint.selected == 1)
				selectedPoints.push_back(ID);			
		}
		else
		{
			if (!start)
			{
				chunkStart.Append(token);
				chunkStart.Append("\n");
			}
			else
			{
				chunkEnd.Append(token);
				chunkEnd.Append("\n");
			}
		}			
		token = strtok(NULL, "\n");	
	}
	FreeHeapPtr(envState);
};

BR_TempoChunk::~BR_TempoChunk()
{
	// Restore preferences
	*(int*)GetConfigVar("envclicksegmode") = preferences.envClickSegMode;
	GetSet_ArrangeView2(NULL, true, preferences.r.left, preferences.r.right, &preferences.start_time, &preferences.end_time);
};

void BR_TempoChunk::Commit()
{
	// Update chunk
	vector<TempoPoint>::const_iterator it = tempoPoints.begin(), end = tempoPoints.end();
	for (; it != end; ++it)
		chunkStart.AppendFormatted(	256, 
									"PT %.16f %.16f %d %d %d %d\n",
									it->position,
									it->bpm, 
									it->shape, 
									it->sig, 
									it->selected, 
									it->partial
								);
	chunkStart.Append(chunkEnd.Get());	
	GetSetObjectState(envelope, chunkStart.Get());

	// Refresh tempo map (UpdateTimeline() doesn't work on it's own when setting chunk with edited values)
	double t, b; int n, d; bool s;
	GetTempoTimeSigMarker(NULL, 0, &t, NULL, NULL, &b, &n, &d, &s);
	SetTempoTimeSigMarker(NULL, 0, t, -1, -1, b, n, d, s);
	UpdateTimeline();
};

bool BR_TempoChunk::GetPoint(int id, double* position, double* bpm, bool* linear)
{
	if (id < 0 || id >= count)
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
			*position = tempoPoints[id].position;
		if (bpm != NULL)
			*bpm = tempoPoints[id].bpm;
		if (linear != NULL)
			*linear = !(tempoPoints[id].shape);
		return true;
	}
};

bool BR_TempoChunk::SetPoint(int id, double position, double bpm, bool linear)
{
	if (id < 0 || id >= count)
		return false;
	else
	{
		tempoPoints[id].position = position;
		tempoPoints[id].bpm = bpm;
		tempoPoints[id].shape = !linear;
		return true;
	}
};

bool BR_TempoChunk::CreatePoint(int id, double position, double bpm, bool linear)
{
	if (id < 0 || id >= count)
		return false;
	else
	{
		TempoPoint newPoint= {};
		newPoint.position = position;
		newPoint.bpm = bpm;
		newPoint.shape = !linear;
		tempoPoints.insert(tempoPoints.begin() + (id+1), newPoint);
		++count;
		return true;	
	}
};

bool BR_TempoChunk::DeletePoint(int id)
{
	if (id < 0 || id >= count)
		return false;
	else
	{
		tempoPoints.erase (tempoPoints.begin()+id);
		--count;
		return true;
	}
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

void FindMiddlePoint(double &middleTime, double &middleBpm, const double &measure, const double &startBpm, const double &endBpm, const double &startTime, const double &endTime)
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