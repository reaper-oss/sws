/******************************************************************************
/ BR_EnvTools.h
/
/ Copyright (c) 2013-2014 Dominik Martin Drzic
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

/******************************************************************************
* Constants                                                                   *
******************************************************************************/
const int    MIN_BPM        = 1;
const int    MAX_BPM        = 960;
const int    MIN_SIG        = 1;
const int    MAX_SIG        = 255;
const double MIN_TEMPO_DIST = 0.001;
const double MIN_ENV_DIST   = 0.000001;
const double MAX_GRID_DIV   = 0.00097; // 1/256 at 960 BPM

/******************************************************************************
* Envelope shapes - this is how Reaper stores point shapes internally         *
******************************************************************************/
enum BR_EnvShape
{
	LINEAR = 0,
	SQUARE,
	SLOW_START_END,
	FAST_START,
	FAST_END,
	BEZIER
};

/******************************************************************************
* Envelope types - purely for readability, not tied to Reaper in any way      *
******************************************************************************/
enum BR_EnvType
{
	VOLUME = 0,
	PAN,
	MUTE,
	PITCH,
	WIDTH,
	PLAYRATE,
	TEMPO,
	PARAMETER
};

/******************************************************************************
* Envelope point class, mostly used internally by BR_Envelope                 *
******************************************************************************/
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
	BR_EnvPoint (double position);
	BR_EnvPoint (double position, double value, int shape, int sig, int selected, int partial, double bezier);
	bool ReadLine (const LineParser& lp);
	void Append (WDL_FastString& string);

	struct ComparePoints
	{
		bool operator() (const BR_EnvPoint& first, const BR_EnvPoint& second)
		{
			return (first.position < second.position);
		}
	};
};

/******************************************************************************
* Class for managing track or take envelope                                   *
******************************************************************************/
class BR_Envelope
{
public:
	BR_Envelope (TrackEnvelope* envelope);
	BR_Envelope (MediaItem_Take* take, BR_EnvType envType);
	~BR_Envelope () {}

	/* Direct point manipulation (returns false if id does not exist) */
	bool GetPoint (int id, double* position, double* value, int* shape, double* bezier);
	bool SetPoint (int id, double* position, double* value, int* shape, double* bezier);
	bool GetSelection (int id);
	bool SetSelection (int id, bool selected);
	bool CreatePoint (int id, double position, double value, int shape, double bezier, bool selected);
	bool DeletePoint (int id);
	bool GetTimeSig (int id, bool* sig, int* num, int* den);
	bool SetTimeSig (int id, bool sig, int num, int den);

	/* Selected points (never updated when editing, use UpdateSelected() if needed) */
	void UnselectAll ();
	void UpdateSelected ();                         // Update selected points' ids based on current state of things
	int CountSelected ();                           // Count selected points
	int GetSelected (int idx);                      // Get selected point based on selected point count (idx is 0 based);
	int CountConseq ();	                            // Count number of consequential selections
	bool GetConseq (int idx, int* start, int* end); // Get consequential selection pair based on count (idx is 0 based)

	/* All points */
	bool ValidateId (int id);
	void Sort ();                                            // Sort points by position
	int Count ();                                            // Count existing points
	int Find (double position, double surroundingRange = 0); // All find functions will be more efficient if points are sorted. When point's
	int FindNext (double position);                          // position is edited or new point is created, code assumes they are not sorted
	int FindPrevious (double position);                      // unless Sort() is used afterwards. Caller needs to check if returned id exists

	/* Miscellaneous */
	double ValueAtPosition (double position);          // Using find functionality, so efficiency may vary (see comment about Find())
	void MoveArrangeToPoint (int id, int referenceId); // Moves arrange horizontally if needed so point is visible
	bool VisibleInArrange ();                          // Check if arrange scroll position allows envelope to be shown
	bool IsTempo ();
	MediaTrack* GetParent ();

	/* Get envelope properties */
	bool IsActive ();
	bool IsVisible ();
	bool IsInLane ();
	bool IsArmed ();
	int LaneHeight ();
	int Type ();          // See BR_EnvType for types
	int DefaultShape ();
	double MinValue ();
	double MaxValue ();
	double CenterValue ();

	/* Set envelope properties */
	void SetActive (bool active);
	void SetVisible (bool visible);
	void SetInLane (bool lane);
	void SetArmed (bool armed);
	void SetLaneHeight (int height);
	void SetDefaultShape (int shape);

	/* Committing - does absolutely nothing if there are no edits (unless forced) */
	bool Commit (bool force = false);

private:
	struct IdPair { int first, second; };
	void ParseState (char* envState, size_t size);
	void SetCounts ();
	void UpdateConsequential ();
	void FillProperties ();
	WDL_FastString GetProperties ();
	int FindFirstPoint ();
	int LastPointAtPos (int id);
	TrackEnvelope* m_envelope;
	MediaItem_Take* m_take;
	MediaTrack* m_parent;
	bool m_tempoMap;
	bool m_update;
	bool m_sorted;
 	int m_count;
	int m_countSel;
	int m_countConseq;
	vector<BR_EnvPoint> m_points;
	vector<int> m_pointsSel;
	vector<IdPair> m_pointsConseq;
	WDL_FastString m_chunkStart;
	WDL_FastString m_chunkEnd;
	struct EnvProperties
	{
		WDL_FastString paramType;
		int active;
		int visible, lane;
		double visUnknown;
		int height, heightUnknown;
		int armed;
		int shape, shapeUnknown1, shapeUnknown2;
		int type;
		double minValue, maxValue, centerValue;
		bool filled, changed;
		EnvProperties ();
	} m_properties;
};

/******************************************************************************
* Miscellaneous                                                               *
******************************************************************************/
bool EnvVis (TrackEnvelope* envelope, bool* lane);
int GetEnvId (TrackEnvelope* envelope, MediaTrack* parent = NULL);
TrackEnvelope* GetTakeEnv (MediaItem_Take* take, BR_EnvType envelope);
vector<int> GetSelPoints (TrackEnvelope* envelope);
MediaTrack* GetEnvParent (TrackEnvelope* envelope);

/******************************************************************************
* Tempo                                                                       *
******************************************************************************/
TrackEnvelope* GetTempoEnv ();
double AverageProjTempo ();
double TempoAtPosition (double startBpm, double endBpm, double startTime, double endTime, double targetTime);
double MeasureAtPosition (double startBpm, double endBpm, double timeLen, double targetTime);
double PositionAtMeasure (double startBpm, double endBpm, double timeLen, double targetMeasure);
void FindMiddlePoint (double* middleTime, double* middleBpm, double measure, double startTime, double endTime, double startBpm, double endBpm);
void SplitMiddlePoint (double* time1, double* time2, double* bpm1, double* bpm2, double splitRatio, double measure, double startTime, double middleTime, double endTime, double startBpm, double middleBpm, double endBpm);
