/******************************************************************************
/ BR_EnvTools.cpp
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
#include "BR_EnvTools.h"
#include "BR_Util.h"
#include "../reaper/localize.h"

BR_EnvChunk::BR_EnvChunk (TrackEnvelope* envelope, bool effCreate /*= false*/) :
m_envelope	(envelope), 
m_effCreate	(effCreate), 
m_tempoMap	(envelope == GetTempoEnv()),
m_update	(false),
m_ordered	(true)
{
	// Get envelope
	char* envState = GetSetObjectState(m_envelope, "");
	size_t size = strlen(envState);
	char* token = strtok(envState, "\n");

	// Get preferences 
	m_preferences.envClickSegMode = *(int*)GetConfigVar("envclicksegmode"); 
	GetWindowRect(FindWindowEx(g_hwndParent,0,"REAPERTrackListWindow","trackview"), &m_preferences.r);
	GetSet_ArrangeView2(NULL, false, m_preferences.r.left, m_preferences.r.right, &m_preferences.start_time, &m_preferences.end_time);

	// Set preferences (prevent reselection of points in time selection)
	*(int*)GetConfigVar("envclicksegmode") = ClearBit(m_preferences.envClickSegMode, 6);

	// Reserve storage to reduce overhead (guessing for everything but tempo map)
	if (m_tempoMap)
	{
		int count = CountTempoTimeSigMarkers(NULL);
		m_points.reserve(count);
		m_selPoints.reserve(count);
	}
	else
	{
		m_points.reserve(size/25);
		m_selPoints.reserve(size/25);
	}	

	// Parse envelope
	LineParser lp(false);
	bool start = false;
	int id = -1;
	while (token != NULL)
	{
		lp.parse(token);
		BR_EnvPoint point;
		if (LineToPoint(lp, point))
		{	
			id++;
			start = true;
			m_points.push_back(point);
			if (point.selected == 1)
				m_selPoints.push_back(id);			
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

	m_count = (int)m_points.size();
};

BR_EnvChunk::~BR_EnvChunk ()
{
	*(int*)GetConfigVar("envclicksegmode") = m_preferences.envClickSegMode;	
};

bool BR_EnvChunk::GetPoint (int id, double* position, double* value, int* shape, double* bezier)
{
	if (id < 0 || id >= m_count)
	{
		if (position != NULL)
			*position = 0;
		if (value != NULL)
			*value = 0;
		if (shape != NULL)
			*shape = 0;
		if (bezier != NULL)
			*bezier = 0;
		return false;
	}	
	else
	{
		if (position != NULL)
			*position = m_points[id].position;
		if (value != NULL)
			*value = m_points[id].value;
		if (shape != NULL)
			*shape = m_points[id].shape;
		if (bezier != NULL)
			*bezier = m_points[id].bezier;
		return true;
	}	
};

bool BR_EnvChunk::SetPoint (int id, double* position, double* value, int* shape, double* bezier)
{
	if (id < 0 || id >= m_count)
		return false;
	else
	{
		if (position != NULL)
			m_points[id].position = *position;
		if (value != NULL)
			m_points[id].value = *value;
		if (shape != NULL)
			m_points[id].shape = *shape;
		if (bezier != NULL)
			m_points[id].bezier = *bezier;

		m_update = true;
		return true;
	}
};

bool BR_EnvChunk::GetSelection (int id)
{
	if (id < 0 || id >= m_count)
		return false;
	else
	{
		if (m_points[id].selected)
			return true;
		else
			return false;
	}
};

bool BR_EnvChunk::SetSelection (int id, bool selected)
{
	if (id < 0 || id >= m_count)
		return false;
	else
	{
		m_points[id].selected = selected;
		m_update = true;
		return true;
	}
};

bool BR_EnvChunk::CreatePoint (int id, double position, double value, int shape, double bezier, bool selected)
{
	if ((id < 0 || id >= m_count) && !m_effCreate) // id is not important in effCreate mode
		return false;
	else
	{
		BR_EnvPoint newPoint(position, value, shape, 0, selected, 0, bezier);
		if (m_effCreate)
		{
			m_points.push_back(newPoint);
			m_ordered = false;
		}
		else
			m_points.insert(m_points.begin() + (id+1), newPoint);				
	}
	++m_count;
	m_update = true;
	return true;
};

bool BR_EnvChunk::DeletePoint (int id)
{
	if (id < 0 || id >= m_count)
		return false;
	else
		m_points.erase(m_points.begin()+id);
		
	--m_count;
	m_update = true;
	return true;
};

const vector<int>* BR_EnvChunk::GetSelected () const
{
	return &m_selPoints;
};

void BR_EnvChunk::UpdateSelected ()
{
	m_selPoints.clear();
	m_selPoints.reserve(m_count);
	
	for (int i = 0; i < m_count; ++i)
		if (m_points[i].selected)
			m_selPoints.push_back(i);
};

void BR_EnvChunk::Sort ()
{
	stable_sort(m_points.begin(), m_points.end(), BR_EnvPoint::ComparePoints());
	m_ordered = true;
};

int BR_EnvChunk::FindNext (double position)
{
	// Sort if needed
	if (m_effCreate && !m_ordered)
		BR_EnvChunk::Sort();
	
	BR_EnvPoint val(position);
	return (int)(upper_bound(m_points.begin(), m_points.end(), val, BR_EnvPoint::ComparePoints()) - m_points.begin());
};

int BR_EnvChunk::FindPrevious (double position)
{
	// Sort if needed
	if (m_effCreate && !m_ordered)
		BR_EnvChunk::Sort();
	
	BR_EnvPoint val(position);
	return (int)(lower_bound(m_points.begin(), m_points.end(), val, BR_EnvPoint::ComparePoints()) - m_points.begin())-1;
};

int BR_EnvChunk::Count ()
{
	return m_count;
};

bool BR_EnvChunk::GetTimeSig (int id, bool* sig, int* num, int* den)
{
	if (id < 0 || id >= m_count || !m_tempoMap)
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
		// Check if requested point has time sig
		if (sig != NULL)
			if (m_points[id].sig == 0)
				*sig = false;
			else
				*sig = true;
		
		// Look for active time sig of a point
		if (num != NULL || den != NULL)
		{
			// Look backwards for the first tempo marker with time sig
			int effSig = 0;
			for (;id >= 0; --id)
			{
				if (m_points[id].sig != 0)
				{
					effSig = m_points[id].sig;
					break;
				}
			}

			// If no time signatures have been found return project time signature
			int effNum, effDen;			
			if (effSig == 0)
			{
				TimeMap_GetTimeSigAtTime(NULL, -1, &effNum, &effDen, NULL);
			}
			// Else extract info from sig token (num is low 16 bits, den high 16 bits)
			else
			{
				effNum = effSig & 0xff;
				effDen = effSig >> 16;
			}

			if (num != NULL)
				*num = effNum;
			if (den != NULL)
				*den = effDen;
		}
		return true;
	}
};

bool BR_EnvChunk::SetTimeSig (int id, bool sig, int num, int den)
{
	if (id < 0 || id >= m_count || !m_tempoMap)
		return false;
	else
	{
		int effSig;
		if (sig)
		{
			// Unlike native method, this function will fail when illegal num/den are requested
			if (num < MIN_SIG || num > MAX_SIG || den < MIN_SIG || den > MAX_SIG)
				return false;							
			
			effSig = (den << 16) + num;			// Partial token needs to be set in accordance with time sig:
			if (m_points[id].partial == 0) 		//	-sig  -partial
				m_points[id].partial = 1;  		//	+sig  -partial
			else if (m_points[id].partial == 4)	//	-sig  +partial	
				m_points[id].partial = 5;		//	+sig  +partial
		}
		else
		{
			effSig = 0;
			if (m_points[id].partial == 1) 		//	+sig  -partial
				m_points[id].partial = 0;  		//	-sig  -partial
			else if (m_points[id].partial == 5)	//	+sig  +partial	
				m_points[id].partial = 4;		//	-sig  +partial
		}
		m_points[id].sig = effSig;
	}
	return true;
};

bool BR_EnvChunk::Commit (bool force /*=false*/, bool arrange /*=true*/)
{
	if (m_update || force)	
	{
		
		// Update chunk
		WDL_FastString chunkStart = m_chunkStart;
		for (vector<BR_EnvPoint>::iterator i = m_points.begin(); i != m_points.end() ; ++i)
			AppendPoint(chunkStart, *i);
		chunkStart.Append(m_chunkEnd.Get());
		GetSetObjectState(m_envelope, chunkStart.Get());


		// Refresh tempo map (UpdateTimeline() doesn't work on it's own when setting chunk with edited values)
		if (m_tempoMap)
		{
			double t, b; int n, d; bool s;
			GetTempoTimeSigMarker(NULL, 0, &t, NULL, NULL, &b, &n, &d, &s);
			SetTempoTimeSigMarker(NULL, 0, t, -1, -1, b, n, d, s);
			UpdateTimeline();
		}

		// Reset arrange view if requested
		if (arrange)
			GetSet_ArrangeView2(NULL, true, m_preferences.r.left, m_preferences.r.right, &m_preferences.start_time, &m_preferences.end_time);
		
		m_update = false;
		return true;
	}
	return false;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//							Various Functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool LineToPoint (LineParser &lp, BR_EnvPoint &point)
{
	if (strcmp(lp.gettoken_str(0), "PT"))
		return false;
	else
	{
		point.position = lp.gettoken_float(1);
		point.value = lp.gettoken_float(2);
		point.shape = lp.gettoken_int(3);
		point.sig = lp.gettoken_int(4);
		point.selected = lp.gettoken_int(5);
		point.partial = lp.gettoken_int(6);
		point.bezier = lp.gettoken_float(7);
		return true;
	}
};

void AppendPoint (WDL_FastString &string, BR_EnvPoint &point)
{
	string.AppendFormatted
	(	
		256, 
		"PT %.14f %.14f %d %d %d %d %.14f\n",
		point.position,
		point.value, 
		point.shape, 
		point.sig, 
		point.selected, 
		point.partial,
		point.bezier
	);
};

bool GetEnvExtrema (char* token, double &min, double &max, double &center)
{
	string str(token, strlen(token)+1);

	if (str.find("PARMENV") != std::string::npos)
	{
		sscanf(token, "<PARMENV %*s %lf %lf %lf", &min, &max, &center);
	}
	else if (str.find("TEMPOENV"))
	{
		min = MIN_BPM;
		max = MAX_BPM;
		// Master_GetTempo() seems broken so use this method instead
		int den; TimeMap_GetTimeSigAtTime(NULL, -1, NULL, &den, NULL);
		GetProjectTimeSignature2(NULL, &center, NULL);
		center = center/den*4;
	}
	else if (str.find("VOLENV") != std::string::npos)
	{
		min = 0;
		max = 2;
		center = 1;
	}
	else if (str.find("PANENV") != std::string::npos || str.find("WIDTHENV") != std::string::npos)
	{
		min = -1;
		max = 1;
		center = 0;
	}
	else if (str.find("MUTEENV") != std::string::npos)
	{
		min = 0;
		max = 1;
		center = 0.5;
	}
	else if (str.find("SPEEDENV") != std::string::npos)
	{
		min = 0.1;
		max = 4;
		center = 1;
	}
	else
		return false;
	return true;
};

vector<int> GetSelEnvPoints (TrackEnvelope* envelope)
{
	char* envState = GetSetObjectState(envelope, "");
	char* token = strtok(envState, "\n");
	
	vector<int> selectedPoints;
	LineParser lp(false);
	int id = -1;

	while(token != NULL)
	{
		lp.parse(token);
		if (!strcmp(lp.gettoken_str(0), "PT"))
		{	
			++id;
			if (lp.gettoken_int(5))
				selectedPoints.push_back(id);
		}
		token = strtok(NULL, "\n");	
	}	
	FreeHeapPtr(envState);
	return selectedPoints;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//							Tempo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TrackEnvelope* GetTempoEnv ()
{
	return GetTrackEnvelopeByName(CSurf_TrackFromID(0, false),  __localizeFunc("Tempo map", "env", 0));
};

double AverageProjTempo ()
{
	if (!CountTempoTimeSigMarkers(NULL))
	{
		// Find end of project and end of tempo map and use whichever is bigger
		double t, b;
		GetTempoTimeSigMarker(NULL, CountTempoTimeSigMarkers(NULL)-1, &t, NULL, NULL, &b, NULL, NULL, NULL);
		double len = EndOfProject (false, false);
		if (t > len)
			len = t;

		// Calculate and return bpm
		if (len != 0)
			return TimeMap_timeToQN(len) * 60 / len;
		else
			return b;
	}
	else
	{
		// If for whatever reason there are no tempo markers, return project BPM
		double b;
		TimeMap_GetTimeSigAtTime(NULL, 0, NULL, NULL, &b);
		return b;
	}
};

double MeasureAtPosition (double startBpm, double endBpm, double timeLen, double targetTime)
{
	// Return number of measures counted from the musical position of startBpm
	return targetTime * (targetTime * (endBpm-startBpm) + 2*timeLen*startBpm) / (480*timeLen);
};

double PositionAtMeasure (double startBpm, double endBpm, double timeLen, double targetMeasure)
{
	// Return number of seconds counted from the time position of startBpm
	double a = startBpm - endBpm;
	double b = timeLen * startBpm;
	double c = 480 * timeLen * targetMeasure;
	return c / (b + sqrt(pow(b,2) - a*c));
};

double TempoAtPosition (double startBpm, double endBpm, double startTime, double endTime, double targetTime)
{
	return startBpm + (endBpm-startBpm) / (endTime-startTime) * (targetTime-startTime);
};

void FindMiddlePoint (double &middleTime, double &middleBpm, const double &measure, const double &startBpm, const double &endBpm, const double &startTime, const double &endTime)
{
	double f = 480 * (measure/2); 
	double a = startBpm - endBpm;
	double b = a*(startTime+endTime) / 2 + f;
	double c = a*(startTime*endTime) + f*(startTime+endTime);
	middleTime = c / (b + sqrt(pow(b,2) - a*c));
	middleBpm = f / (middleTime-startTime) - startBpm;
};

void SplitMiddlePoint (double &time1, double &time2, double &bpm1, double &bpm2, const double &splitRatio, const double &measure, const double &startBpm, const double &middleBpm, const double &endBpm, const double &startTime, const double &middleTime, const double &endTime)
{
	// First point
	double temp = measure * (1-splitRatio) / 2;
	time1 = PositionAtMeasure(startBpm, middleBpm, (middleTime-startTime), temp) + startTime;
	bpm1 = 480*temp / (time1-startTime) - startBpm;

	// Second point
	double f1 = 480 * measure*splitRatio; 
	double f2 = 480 * temp;
	double a = bpm1-endBpm;
	double b = (a*(time1+endTime) + f1+f2) / 2;
	double c = a*(time1*endTime) + f1*endTime + f2*time1;
	time2 = c / (b + sqrt(pow(b,2) - a*c));
	bpm2 = f1 / (time2-time1) - bpm1;
};