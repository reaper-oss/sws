/******************************************************************************
/ BR_EnvTools.h
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

#define MIN_BPM			1
#define MAX_BPM			960
#define MIN_SIG			1
#define MAX_SIG			255
#define MIN_TEMPO_DIST	0.001
#define MAX_GRID_DIV	0.00097

struct BR_EnvPoint
{
	double position;
	double value;
	double bezier;
	int shape;
	int sig;
	int selected;
	int partial;

	BR_EnvPoint () {}
	BR_EnvPoint (double position) : position(position) {}
	BR_EnvPoint (double position, double value, int shape, int sig, int selected, int partial, double bezier)
				: position(position), value(value), shape(shape), sig(sig), selected(selected), partial(partial), bezier(bezier) {}

	struct ComparePoints
	{
		bool operator() (const BR_EnvPoint &first, const BR_EnvPoint &second) { return (first.position < second.position);}
	};
};

bool LineToPoint (LineParser &lp, BR_EnvPoint &point);
void AppendPoint (WDL_FastString &string, BR_EnvPoint &point);
bool GetEnvExtrema (char* token, double &min, double &max, double &center);
vector<int> GetSelEnvPoints (TrackEnvelope* envelope);


class BR_EnvChunk
{

// When effCreate is set to true, creation of a lot of points will be faster. New points will always be written at the
// end of the container. They will get "sorted" by GetSetObjectState when committing changes.
//	- Point will always get created no matter the supplied ID so CreatePoint will always return true
//	- Other member functions can still be used on those points with ID but in the order they were created
//	- Points can be sorted by Sort()
//	- Using FindNext() and FindPrevious() will automatically perform Sort() if points were previously created

public:
	// Constructor/destructor - envclicksegmode is temporary changed during object's existence to prevent reselection of points in time selection
	BR_EnvChunk (TrackEnvelope* envelope, bool effCreate = false);
	~BR_EnvChunk ();
	
	// Direct point manipulation (return false if ID does not exist)
	bool GetPoint (int id, double* position, double* value, int* shape, double* bezier);
	bool SetPoint (int id, double* position, double* value, int* shape, double* bezier);
	bool GetSelection (int id);
	bool SetSelection (int id, bool selected);
	bool CreatePoint (int id, double position, double value, int shape, double bezier, bool selected);
	bool DeletePoint (int id);
	
	// Misc
	const vector<int>* GetSelected () const;	// selected points' ID is not updated when editing points (deal with it by offset or UpdateSelected)
	void UpdateSelected ();			 			// update selected points' ID
	void Sort (); 			 					// sort points by position
	int Count ();								// count existing points
	int FindNext (double position);				// both find functions require sorted points to work, caller should check if returned id exists
	int FindPrevious (double position);

	// Tempo map specific
	bool GetTimeSig (int id, bool* sig, int* num, int* den); 
	bool SetTimeSig (int id, bool sig, int num, int den);

	// Committing - does absolutely nothing if there are no edits (unless forced), can set arrange view as it was when object was created
	bool Commit (bool force = false, bool arrange = true);

private:
	TrackEnvelope* m_envelope;
	bool m_tempoMap;
	bool m_effCreate;
	bool m_update;
	bool m_ordered;
	int m_count;
	vector<BR_EnvPoint> m_points;
	vector<int> m_selPoints;	
	WDL_FastString m_chunkStart;
	WDL_FastString m_chunkEnd;
	struct Preferences
	{
		RECT r;
		double start_time;
		double end_time;
		int envClickSegMode;		
	} m_preferences;
};

// Additional functions used in BR_Tempo.cpp
TrackEnvelope* GetTempoEnv ();
double AverageProjTempo ();
double MeasureAtPosition (double startBpm, double endBpm, double timeLen, double targetTime);
double PositionAtMeasure (double startBpm, double endBpm, double timeLen, double targetMeasure);
double TempoAtPosition (double startBpm, double endBpm, double startTime, double endTime, double targetTime);
void FindMiddlePoint (double &middleTime, double &middleBpm, const double &measure, const double &startBpm, const double &endBpm, const double &startTime, const double &endTime);
void SplitMiddlePoint (double &time1, double &time2, double &bpm1, double &bpm2, const double &splitRatio, const double &measure, const double &startBpm, const double &middleBpm, const double &endBpm, const double &startTime, const double &middleTime, const double &endTime);