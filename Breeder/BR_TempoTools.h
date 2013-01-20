/******************************************************************************
/ BR_TempoTools.h
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
#pragma once
 
// Class used in BR_Tempo.cpp to help process things faster when there are a lot of selected points to manipulate 
//	- nothing is recalculated (as it is when using Reaper's API - basically a reason for this class)  
//	- slower on small amount (<10) of points compared to Reaper's API (not really noticeable in real life unless tempo map is HUGE)
//	- excels with a lot of points (you do not want to delete/create 1000 points using Reaper's API - and it is a possible scenario in real life)
//	- selected points' ID is not updated when adding/deleting points (BR_Tempo.cpp handles it with offset value so no need to update vector every time)
//  
//	- TODO: add modes for alternate operation to gain efficiency (to escape expensive inserting and removing from vector)
//			methods for handling time signatures (http://askjf.com/index.php?q=2212s) (not needed unless explicitly editing time signatures)

class BR_TempoChunk
{
public:
	BR_TempoChunk();
	~BR_TempoChunk();

	void Commit();
	bool GetPoint(int id, double* position, double* bpm, bool* linear);
	bool SetPoint(int id, double position, double bpm, bool linear);
	bool CreatePoint(int id, double position, double bpm, bool linear);
	bool DeletePoint(int id);
	vector<int> GetSelected() {return selectedPoints;};

private:
	struct TempoPoint
	{
		double position;
		double bpm;
		int shape;
		int sig;
		int selected;
		int partial;
	};

	TrackEnvelope* envelope;
	WDL_FastString chunkStart;
	WDL_FastString chunkEnd;
	vector<TempoPoint> tempoPoints;
	vector<int> selectedPoints;
	int count;

	struct Preferences
	{
		int envClickSegMode;
		double start_time;
		double end_time;
		RECT r;
		bool altDelete;
		bool altCreate;
	}preferences;	
};

// Some additional functions used in BR_Tempo.cpp
vector<int> GetSelectedTempo ();
double TempoAtPosition (double startBpm, double endBpm, double startTime, double endTime, double targetTime);
double MeasureAtPosition (double startBpm, double endBpm, double length, double targetTime);
double PositionAtMeasure (double startBpm, double endBpm, double length, double targetMeasure);
void FindMiddlePoint(double &middleTime, double &middleBpm, const double &measure, const double &startBpm, const double &endBpm, const double &startTime, const double &endTime);
void SplitMiddlePoint (double &time1, double &time2, double &bpm1, double &bpm2, const double &splitRatio, const double &measure, const double &startBpm, const double &middleBpm, const double &endBpm, const double &startTime, const double &middleTime, const double &endTime);