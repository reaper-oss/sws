/******************************************************************************
/ BR_EnvelopeUtil.h
/
/ Copyright (c) 2013-2015 Dominik Martin Drzic
/ http://forum.cockos.com/member.php?u=27094
/ https://code.google.com/p/sws-extension
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
const double MIN_BPM        = 1;
const double MAX_BPM        = 960;
const int    MIN_SIG        = 1;
const int    MAX_SIG        = 255;
const double MIN_TEMPO_DIST = 0.001;
const double MIN_ENV_DIST   = 0.000001;
const double MIN_GRID_DIST  = 0.00097;         // 1/256 at 960 BPM (can't set it to more than 1/256 in grid settings, but other actions like Adjust grid by 1/2 can)
const double MAX_GRID_DIV   = 1.0 / 256.0 * 4; // 1/256 because things can get broken after that (http://forum.cockos.com/project.php?issueid=5263)

/******************************************************************************
* Envelope shapes - this is how Reaper stores point shapes internally         *
******************************************************************************/
enum BR_EnvShape
{
	LINEAR         = 0,
	SQUARE         = 1,
	SLOW_START_END = 2,
	FAST_START     = 3,
	FAST_END       = 4,
	BEZIER         = 5
};

/******************************************************************************
* Envelope types - purely for readability and bitmasks, not tied to Reaper in *
* any way                                                                     *
******************************************************************************/
enum BR_EnvType
{
	UNKNOWN      = 0x1,
	VOLUME       = 0x2,
	VOLUME_PREFX = 0x4,
	PAN          = 0x8,
	PAN_PREFX    = 0x10,
	WIDTH        = 0x20,
	WIDTH_PREFX  = 0x40,
	MUTE         = 0x80,
	PITCH        = 0x100,
	PLAYRATE     = 0x200,
	TEMPO        = 0x400,
	PARAMETER    = 0x800
};

/******************************************************************************
* Class for managing track or take envelope                                   *
******************************************************************************/
class BR_Envelope
{
public:
	BR_Envelope ();
	BR_Envelope (TrackEnvelope* envelope, bool takeEnvelopesUseProjectTime = true); // for takeEnvelopesUseProjectTime explanation see declaration of SetTakeEnvelopeTimebase()
	BR_Envelope (MediaTrack* track, int envelopeId, bool takeEnvelopesUseProjectTime = true);
	BR_Envelope (MediaItem_Take* take, BR_EnvType envType, bool takeEnvelopesUseProjectTime = true);
	BR_Envelope (const BR_Envelope& envelope);
	~BR_Envelope ();
	BR_Envelope& operator=  (const BR_Envelope& envelope);
	bool         operator== (const BR_Envelope& envelope); // This only compares points and if envelope is active (aka things that affect playback) - other
	bool         operator!= (const BR_Envelope& envelope); // properties like type, envelope pointer, height, armed, default shape etc... are ignored)

	/* Direct point manipulation (returns false if id does not exist) (checkPosition only works for take envelopes, snapValue only for pitch envelopes) */
	bool GetPoint (int id, double* position, double* value, int* shape, double* bezier);
	bool SetPoint (int id, double* position, double* value, int* shape, double* bezier, bool checkPosition = false, bool snapValue = false);
	bool GetSelection (int id);
	bool SetSelection (int id, bool selected);
	bool CreatePoint (int id, double position, double value, int shape, double bezier, bool selected, bool checkPosition = false, bool snapValue = false);
	bool DeletePoint (int id);
	bool DeletePoints (int startId, int endId);
	bool GetTimeSig (int id, bool* sig, bool* partial, int* num, int* den);
	bool SetTimeSig (int id, bool sig, bool partial, int num, int den);
	bool SetCreateSortedPoint (int id, double position, double value, int shape, double bezier, bool selected); // for ReaScript export

	/* Selected points (never updated when editing, use UpdateSelected() if needed) */
	void UnselectAll ();
	void UpdateSelected ();                         // Update selected points' ids based on current state of things
	int CountSelected ();                           // Count selected points
	int GetSelected (int idx);                      // Get selected point based on count (idx is 0 based)
	int CountConseq ();                             // Count number of consequential selections (won't work with unsorted points)
	bool GetConseq (int idx, int* start, int* end); // Get consequential selection pair based on count (idx is 0 based)

	/* All points */
	bool ValidateId (int id);
	void DeletePointsInRange (double start, double end);
	void DeleteAllPoints ();
	void Sort ();                                            // Sort points by position
	int CountPoints ();                                      // Count existing points
	int Find (double position, double surroundingRange = 0); // All find functions will be more efficient if points are sorted.
	int FindNext (double position);                          // When point's position is edited or new point is created, code
	int FindPrevious (double position);                      // assumes they are not sorted unless Sort() is used afterwards,
	int FindClosest (double position);                       // note that caller needs to check if returned id exists

	/* Points properties */
	double ValueAtPosition (double position, bool fastMode = false); // fastMode will not use native API which is more accurate in some cases (noticed it with bezier curves), but much slower with high point count (accuracy difference should be minimal but still important when dealing with things like mouse detection where every pixel counts!)
	double NormalizedDisplayValue (double value);                    // Convert point value to 0.0 - 1.0 range as displayed in arrange
	double RealValue (double normalizedDisplayValue);                // Convert normalized display value in range 0.0 - 1.0 to real envelope value
	double SnapValue (double value);                                 // Snaps value to current settings (only relevant for take pitch envelope)
	void GetSelectedPointsExtrema (double* minimum, double* maximum);
	bool GetPointsInTimeSelection (int* startId, int* endId, double* tStart = NULL, double* tEnd = NULL); // Presumes points are sorted, returns false if there is no time selection (if there are no points in time selection, both ids will be -1)

	/* Miscellaneous */
	bool VisibleInArrange (int* envHeight, int* yOffset, bool cacheValues = false); // Is part of the envelope visible in arrange (height calculation can be intensive (envelopes in track lane), use cacheValues if situation allows)
	void MoveArrangeToPoint (int id, int referenceId);                              // Moves arrange horizontally if needed so point is visible
	void SetTakeEnvelopeTimebase (bool useProjectTime);                             // By setting this to true, project time can be used everywhere when dealing with take envelopes. If take changes position, just call again.
	WDL_FastString FormatValue (double value);
	WDL_FastString GetName ();
	MediaItem_Take* GetTake ();
	MediaTrack* GetParent ();
	TrackEnvelope* GetPointer ();

	/* Get envelope properties */
	bool IsTempo ();
	bool IsTakeEnvelope ();
	bool IsLocked (); // Checks lock settings for specific envelope type
	bool IsActive ();
	bool IsVisible ();
	bool IsInLane ();
	bool IsArmed ();
	bool IsScaledToFader ();
	int GetLaneHeight ();
	int GetDefaultShape ();
	int Type ();       // See BR_EnvType for types
	int GetFxId ();    // returns -1 if not FX envelope
	int GetParamId (); // returns -1 if not FX envelope
	int GetSendId ();  // returns -1 if not send envelope, otherwise send id for it's parent track
	double MinValue ();
	double MaxValue ();
	double CenterValue ();
	double LaneMaxValue ();
	double LaneMinValue ();

	/* Set envelope properties */
	void SetActive (bool active);
	void SetVisible (bool visible);
	void SetInLane (bool lane);
	void SetArmed (bool armed);
	void SetLaneHeight (int height);
	void SetDefaultShape (int shape);
	void SetScalingToFader (bool faderScaling);

	/* Committing - does absolutely nothing if there are no edits or locking is turned on (unless forced) */
	bool Commit (bool force = false);

private:
	struct IdPair
	{
		int first, second;
	};

	struct EnvProperties
	{
		int active;
		int visible, lane, visUnknown;
		int height, heightUnknown;
		int armed;
		int shape, shapeUnknown1, shapeUnknown2;
		int faderMode;
		int type;
		double minValue, maxValue, centerValue;
		int paramId, fxId;
		bool filled, changed;
		WDL_FastString paramType;
		EnvProperties ();
		EnvProperties (const EnvProperties& properties);
		EnvProperties& operator=  (const EnvProperties& properties);
	} mutable m_properties; // access through separate class methods (they make sure data is read and written correctly) - mutable because FillProperties must be const (to make operator== const) but still be able to change m_properties

	struct BR_EnvPoint
	{
		double position;
		double value;
		double bezier;
		bool selected;
		int shape;
		int sig;
		int partial;
		unsigned int metronome1;
		unsigned int metronome2;
		WDL_FastString tempoStr;

		BR_EnvPoint ();
		BR_EnvPoint (double position);
		BR_EnvPoint (double position, double value, int shape, int sig, bool selected, int partial, double bezier);
		bool ReadLine (const LineParser& lp); // use only once per object (for efficiency, tempoStr is never deleted, only appended too)
		void Append (WDL_FastString& string, bool tempoPoint);
		struct ComparePoints
		{
			bool operator() (const BR_EnvPoint& first, const BR_EnvPoint& second)
			{
				return (first.position < second.position);
			}
		};
	};

	int FindFirstPoint ();
	int LastPointAtPos (int id);
	int FindNext (double position, double offset);     // used for internal stuff since position
	int FindPrevious (double position, double offset); // offset of take envelopes has to be tracked
	void Build (bool takeEnvelopesUseProjectTime);
	void UpdateConsequential ();
	void FillFxInfo ();
	void FillProperties () const; // to make operator== const (yes, m_properties does get modified but only if not cached already)
	WDL_FastString GetProperties ();
	TrackEnvelope* m_envelope;
	MediaTrack* m_parent;
	MediaItem_Take* m_take;
	bool m_tempoMap;
	bool m_update;
	bool m_sorted;
	bool m_pointsEdited; // tells us if we can use Envelope_Evaluate() in this->ValueAtPosition()
	double m_takeEnvOffset;
	int m_sampleRate;
	int m_takeEnvType;
	int m_countConseq;
	int m_height;
	int m_yOffset;
	int m_count;
	int m_countSel;
	vector<BR_EnvPoint> m_points;
	vector<int> m_pointsSel;
	vector<IdPair> m_pointsConseq;
	WDL_FastString m_chunkProperties;
	WDL_FastString m_envName;
};

/******************************************************************************
* Miscellaneous                                                               *
******************************************************************************/
TrackEnvelope* GetTempoEnv ();
TrackEnvelope* GetVolEnv (MediaTrack* track);
TrackEnvelope* GetVolEnvPreFX (MediaTrack* track);
TrackEnvelope* GetTakeEnv (MediaItem_Take* take, BR_EnvType envelope);
MediaItem_Take* GetTakeEnvParent (TrackEnvelope* envelope, int* type); // for type see BR_EnvType
MediaTrack* GetEnvParent (TrackEnvelope* envelope);
vector<int> GetSelPoints (TrackEnvelope* envelope);
WDL_FastString ConstructReceiveEnv (int type, double firstPointValue); // for type see BR_EnvType
bool ToggleShowSendEnvelope (MediaTrack* track, int sendId, int type); // for type see BR_EnvType
bool ShowSendEnvelopes (vector<MediaTrack*>& tracks, int envelopes);   // envelopes is a bitmask (see BR_EnvType)
bool EnvVis (TrackEnvelope* envelope, bool* lane);
int GetEnvId (TrackEnvelope* envelope, MediaTrack* parent = NULL);
int GetDefaultPointShape ();                                      // see BR_EnvShape for return values;
int GetEnvType (TrackEnvelope* envelope, bool* isSend);           // for return type see BR_EnvType (note: function relies on envelope names, localization could theoretically break it)
int GetCurrentAutomationMode (MediaTrack* track);                 // takes global override into account
int CountTrackEnvelopePanels (MediaTrack* track);

/******************************************************************************
* Tempo                                                                       *
******************************************************************************/
int FindPreviousTempoMarker (double position);
int FindNextTempoMarker (double position);
int FindClosestTempoMarker (double position);
int FindTempoMarker (double position, double surroundingRange = 0);
double AverageProjTempo ();
double TempoAtPosition (double startBpm, double endBpm, double startTime, double endTime, double targetTime);
double MeasureAtPosition (double startBpm, double endBpm, double timeLen, double targetTime);
double PositionAtMeasure (double startBpm, double endBpm, double timeLen, double targetMeasure);
void FindMiddlePoint (double* middleTime, double* middleBpm, double measure, double startTime, double endTime, double startBpm, double endBpm);
void SplitMiddlePoint (double* time1, double* time2, double* bpm1, double* bpm2, double splitRatio, double measure, double startTime, double middleTime, double endTime, double startBpm, double middleBpm, double endBpm);