/******************************************************************************
/ BR_Loudness.cpp
/
/ Copyright (c) 2014 Dominik Martin Drzic
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
#include "BR_Loudness.h"
#include "BR_EnvTools.h"
#include "BR_Util.h"
#include "../SnM/SnM_Dlg.h"
#include "../SnM/SnM_Util.h"
#include "../SnM/SnM.h"
#include "../libebur128/ebur128.h"
#include "../reaper/localize.h"

/******************************************************************************
* Constants                                                                   *
******************************************************************************/
const char* const LOUDNESS_KEY      = "BR - AnalyzeLoudness";
const char* const LOUDNESS_WND      = "BR - AnalyzeLoudness WndPos" ;
const char* const LOUDNESS_VIEW_WND = "BR - AnalyzeLoudnessView WndPos";
const char* const NORMALIZE_KEY     = "BR - NormalizeLoudness";

// List view columns
static SWS_LVColumn g_cols[] =
{
	{25,  0, "#"},
	{110, 0, "Track"},
	{110, 0, "Take" },
	{80,  0, "Integrated" },
	{80,  0, "Range" },
	{120, 0, "Maximum short-term" },
	{120, 0, "Maximum momentary" },
};

enum
{
	COL_ID = 0,
	COL_TRACK,
	COL_TAKE,
	COL_INTEGRATED,
	COL_RANGE,
	COL_SHORTTERM,
	COL_MOMENTARY,
	COL_COUNT
};

// Analyze loudness window messages
const int SET_ANALYZE_TARGET          = 0xF001;
const int SET_ANALYZE_ON_NORMALIZE    = 0xF002;
const int SET_MIRROR_SELECTION        = 0xF003;
const int SET_DOUBLECLICK_GOTO_TARGET = 0xF004;
const int SET_TIMESEL_OVER_MAX        = 0xF005;
const int SET_CLEAR_ENVELOPE          = 0xF006;
const int SET_CLEAR_ON_ANALYZE        = 0xF007;
const int GO_TO_SHORTTERM             = 0xF008;
const int GO_TO_MOMENTARY             = 0xF009;
const int DRAW_SHORTTERM              = 0xF00A;
const int DRAW_MOMENTARY              = 0xF00B;
const int NORMALIZE                   = 0xF00C;
const int NORMALIZE_TO_23             = 0xF00D;
const int REANALYZE_ITEMS             = 0xF00E;
const int DELETE_ITEM                 = 0xF00F;

const int ANALYZE_TIMER     = 1;
const int REANALYZE_TIMER   = 2;
const int UPDATE_TIMER      = 3;
const int ANALYZE_TIMER_FREQ = 50;
const int UPDATE_TIMER_FREQ  = 200;

/******************************************************************************
* Globals                                                                     *
******************************************************************************/
SNM_WindowManager<BR_AnalyzeLoudnessWnd> g_loudnessWndManager(LOUDNESS_WND);
SWSProjConfig<WDL_PtrList_DeleteOnDestroy<BR_LoudnessObject> > g_analyzedObjects;

/******************************************************************************
* Loudness object                                                             *
******************************************************************************/
BR_LoudnessObject::BR_LoudnessObject (MediaTrack* track) :
m_track          (track),
m_take           (NULL),
m_takeGuid       (GUID_NULL),
m_trackGuid      (*(GUID*)GetSetMediaTrackInfo(track, "GUID", NULL)),
m_integrated     (NEGATIVE_INF),
m_shortTermMax   (NEGATIVE_INF),
m_momentaryMax   (NEGATIVE_INF),
m_range          (0),
m_progress       (0),
m_running        (false),
m_analyzed       (false),
m_killFlag       (false),
m_integratedOnly (false),
m_process        (NULL)
{
	this->CheckSetAudioData();
}

BR_LoudnessObject::BR_LoudnessObject (MediaItem_Take* take) :
m_track          (NULL),
m_take           (take),
m_takeGuid       (*(GUID*)GetSetMediaItemTakeInfo(take, "GUID", NULL)),
m_trackGuid      (GUID_NULL),
m_integrated     (NEGATIVE_INF),
m_shortTermMax   (NEGATIVE_INF),
m_momentaryMax   (NEGATIVE_INF),
m_range          (0),
m_progress       (0),
m_running        (false),
m_analyzed       (false),
m_killFlag       (false),
m_integratedOnly (false),
m_process        (NULL)
{
	this->CheckSetAudioData();
}

BR_LoudnessObject::~BR_LoudnessObject ()
{
	this->StopAnalyze();
	if (EnumProjects(0, NULL, 0)) // prevent destroying accessor on reaper exit (otherwise we get access violation)
		DestroyAudioAccessor(m_audioData.audio);
}

bool BR_LoudnessObject::Analyze (bool integratedOnly /*= false*/)
{
	this->StopAnalyze();
	this->SetIntegratedOnly(integratedOnly);

	// Is audio data still valid?
	if (this->CheckSetAudioData())
	{
		// Was this data already analyzed?
		if (!this->GetAnalyzedStatus())
		{
			this->SetRunning(true);
			this->SetProgress(0);
			this->SetProcess((HANDLE)_beginthreadex(NULL, 0, this->AnalyzeData, (void*)this, 0, NULL));
		}
		return true;
	}

	// Audio isn't valid and can't be retrieved (related track/take is no more)
	else
	{
		this->SetRunning(false);
		this->SetProgress(0);
		return false;
	}
}

bool BR_LoudnessObject::IsRunning ()
{
	SWS_SectionLock lock(&m_mutex);
	return m_running;
}

double BR_LoudnessObject::GetProgress ()
{
	SWS_SectionLock lock(&m_mutex);
	return m_progress;
}

void BR_LoudnessObject::StopAnalyze ()
{
	if (this->GetProcess())
	{
		this->SetKillFlag(true);
		WaitForSingleObject(this->GetProcess(), INFINITE);
		this->SetKillFlag(false);
		CloseHandle(this->GetProcess());

		this->SetProcess(NULL);
		this->SetRunning(false);
		this->SetProgress(0);
	}
}

void BR_LoudnessObject::NormalizeIntegrated (double targetdB)
{
	SWS_SectionLock lock(&m_mutex);

	if (!this->IsTargetValid())
		return;

	double integrated;
	this->GetAnalyzeData(&integrated, NULL, NULL, NULL);

	if (integrated > NEGATIVE_INF)
	{
		if (m_track)
		{
			double volume = *(double*)GetSetMediaTrackInfo(m_track, "D_VOL", NULL);
			volume *= DB2VAL(targetdB) / DB2VAL(integrated);
			GetSetMediaTrackInfo(m_track, "D_VOL", &volume);
		}
		else
		{
			double volume = *(double*)GetSetMediaItemTakeInfo(m_take, "D_VOL", NULL);
			volume *= DB2VAL(targetdB) / DB2VAL(integrated);
			GetSetMediaItemTakeInfo(m_take, "D_VOL", &volume);
		}
	}
}

bool BR_LoudnessObject::CreateGraph (BR_Envelope& envelope, double min, double max, bool momentary)
{
	SWS_SectionLock lock(&m_mutex);

	if (!this->IsTargetValid())
		return false;

	envelope.Sort();

	double start = (m_track) ? (m_audioData.audioStart) : (GetMediaItemInfo_Value(GetMediaItemTake_Item(m_take), "D_POSITION"));
	double end   = (m_track) ? (m_audioData.audioEnd)   : (start + GetMediaItemInfo_Value(GetMediaItemTake_Item(m_take), "D_LENGTH"));
	double newMin = envelope.LaneMinValue();
	double newMax = envelope.LaneMaxValue();

	envelope.DeletePointsInRange(start, end);

	if (momentary)
	{
		double position = start;
		envelope.CreatePoint(envelope.Count(), position, newMin, LINEAR, 0, false);
		position += 0.4;

		size_t size = m_momentaryValues.size();
		for (size_t i = 0; i < size; ++i)
		{
			double value = TranslateRange(m_momentaryValues[i], min, max, newMin, newMax);

			if (i != size-1)
			{
				envelope.CreatePoint(envelope.Count(), position, value, LINEAR, 0, false);
				position += 0.4;
			}
			else
			{
				// Reached the end, last point has to be square and end right where the audio ends
				if (position < end)
				{
					envelope.CreatePoint(envelope.Count(), position, value, LINEAR, 0, false);
					envelope.CreatePoint(envelope.Count(), end, value, LINEAR, 0, false);
					if (value > newMin)
						envelope.CreatePoint(envelope.Count(), end, newMin, SQUARE, 0, false);
				}
				else
				{
					if (position > end)
						position = end;

					if (value > newMin)
					{
						envelope.CreatePoint(envelope.Count(), position, value, LINEAR, 0, false);
						envelope.CreatePoint(envelope.Count(), end, newMin, SQUARE, 0, false);
					}
					else
					{
						envelope.CreatePoint(envelope.Count(), position, value, SQUARE, 0, false);
					}
				}
				break;
			}
		}
		// In case there are no momentary measurements (item too short) make sure graph ends with minimum
		if (size == 0)
			envelope.CreatePoint(envelope.Count(), end, newMin, SQUARE, 0, false);
	}
	else
	{
		double position = start;
		envelope.CreatePoint(envelope.Count(), position, newMin, LINEAR, 0, false);
		position += 3;
		size_t size = m_shortTermValues.size();
		for (size_t i = 0; i < size ; ++i)
		{
			double value = TranslateRange(m_shortTermValues[i], min, max, newMin, newMax);

			if (i != size-1)
			{
				envelope.CreatePoint(envelope.Count(), position, value, LINEAR, 0, false);
				position += 3;
			}
			// Reached the end, last point has to be square and end right where the audio ends
			else
			{
				if (position < end)
				{
					envelope.CreatePoint(envelope.Count(), position, value, LINEAR, 0, false);
					envelope.CreatePoint(envelope.Count(), end, value, LINEAR, 0, false);
					if (value > newMin)
						envelope.CreatePoint(envelope.Count(), end, newMin, SQUARE, 0, false);
				}
				else
				{
					if (position > end)
						position = end;

					if (value > newMin)
					{
						envelope.CreatePoint(envelope.Count(), position, value, LINEAR, 0, false);
						envelope.CreatePoint(envelope.Count(), end, newMin, SQUARE, 0, false);
					}
					else
					{
						envelope.CreatePoint(envelope.Count(), position, value, SQUARE, 0, false);
					}
				}
				break;
			}
		}
		// In case there are no short-term measurements (item too short) make sure graph ends with minimum
		if (size == 0)
			envelope.CreatePoint(envelope.Count(), end, newMin, SQUARE, 0, false);
	}

	return true;
}

double BR_LoudnessObject::GetColumnVal (int column)
{
	SWS_SectionLock lock(&m_mutex);

	double returnVal = 0;
	switch (column)
	{
		case COL_INTEGRATED:
			this->GetAnalyzeData(&returnVal, NULL, NULL, NULL);
		break;

		case COL_RANGE:
			this->GetAnalyzeData(NULL, &returnVal, NULL, NULL);
		break;

		case COL_SHORTTERM:
			this->GetAnalyzeData(NULL, NULL, &returnVal, NULL);
		break;

		case COL_MOMENTARY:
			this->GetAnalyzeData(NULL, NULL, NULL, &returnVal);
		break;
	}

	return RoundToN(returnVal, 1);
}

void BR_LoudnessObject::GetColumnStr (int column, char* str, int strSz)
{
	SWS_SectionLock lock(&m_mutex);

	switch (column)
	{
		case COL_ID:
		{
			_snprintf(str, strSz, "%d", g_analyzedObjects.Get()->Find(this) + 1);
		}
		break;

		case COL_TRACK:
		{
			_snprintf(str, strSz, "%s", (this->GetTrackName()).Get());
		}
		break;

		case COL_TAKE:
		{
			_snprintf(str, strSz, "%s", (this->GetTakeName()).Get());
		}
		break;

		case COL_INTEGRATED:
		{
			double integrated;
			this->GetAnalyzeData(&integrated, NULL, NULL, NULL);

			if (integrated <= NEGATIVE_INF)
				_snprintf(str, strSz, "%s", __localizeFunc("-inf", "vol", 0));
			else
				_snprintf(str, strSz, "%.1lf %s", integrated, __LOCALIZE("LUFS","sws_DLG_174"));
		}
		break;

		case COL_RANGE:
		{
			double range;
			this->GetAnalyzeData(NULL, &range, NULL, NULL);

			if (range <= NEGATIVE_INF)
				_snprintf(str, strSz, "%s", __localizeFunc("-inf", "vol", 0));
			else
				_snprintf(str, strSz, "%.1lf %s", range, __LOCALIZE("LU","sws_DLG_174"));
		}
		break;

		case COL_SHORTTERM:
		{
			double shortTermMax;
			this->GetAnalyzeData(NULL, NULL, &shortTermMax, NULL);

			if (shortTermMax <= NEGATIVE_INF)
				_snprintf(str, strSz, "%s", __localizeFunc("-inf", "vol", 0));
			else
				_snprintf(str, strSz, "%.1lf %s", shortTermMax, __LOCALIZE("LUFS","sws_DLG_174"));
		}
		break;

		case COL_MOMENTARY:
		{
			double momentaryMax;
			this->GetAnalyzeData(NULL, NULL, NULL, &momentaryMax);

			if (momentaryMax <= NEGATIVE_INF)
				_snprintf(str, strSz, "%s", __localizeFunc("-inf", "vol", 0));
			else
				_snprintf(str, strSz, "%.1lf %s", momentaryMax, __LOCALIZE("LUFS","sws_DLG_174"));
		}
		break;
	}
}

double BR_LoudnessObject::GetAudioLength ()
{
	SWS_SectionLock lock(&m_mutex);
	return m_audioData.audioEnd - m_audioData.audioStart;
}

bool BR_LoudnessObject::CheckTarget (MediaTrack* track)
{
	if (this->IsTargetValid() && m_track && m_track == track)
		return true;
	else
		return false;
}

bool BR_LoudnessObject::CheckTarget (MediaItem_Take* take)
{
	if (this->IsTargetValid() && m_take && m_take == take)
		return true;
	else
		return false;
}

bool BR_LoudnessObject::IsTrack ()
{
	SWS_SectionLock lock(&m_mutex);
	if (m_track) return true;
	else         return false;
}

bool BR_LoudnessObject::IsTargetValid ()
{
	SWS_SectionLock lock(&m_mutex);

	if (m_track)
	{
		if (ValidatePtr(m_track, "MediaTrack*") && GuidsEqual(&m_trackGuid, (GUID*)GetSetMediaTrackInfo(m_track, "GUID", NULL)))
			return true;
		else
		{
			// Deleting and undoing track will naturally change item's pointer so find new one if possible
			if (MediaTrack* newTrack = GuidToTrack(&m_trackGuid))
			{
				if (GuidsEqual(&m_trackGuid, (GUID*)GetSetMediaTrackInfo(newTrack, "GUID", NULL)))
					m_track = newTrack;
				return true;
			}
		}
	}
	else
	{
		if (ValidatePtr(m_take, "MediaItem_Take*") && GuidsEqual(&m_takeGuid, (GUID*)GetSetMediaItemTakeInfo(m_take, "GUID", NULL)))
			return true;
		else
		{
			if (MediaItem_Take* newTake = GetMediaItemTakeByGUID(NULL, &m_takeGuid))
			{
				if (GuidsEqual(&m_takeGuid, (GUID*)GetSetMediaItemTakeInfo(newTake, "GUID", NULL)))
					m_take = newTake;
				return true;
			}
		}
	}

	return false;
}

bool BR_LoudnessObject::IsSelectedInProject ()
{
	SWS_SectionLock lock(&m_mutex);
	if (!this->IsTargetValid())
		return false;

	if (m_track)
		return !!(*(int*)GetSetMediaTrackInfo(m_track, "I_SELECTED", NULL));
	else
		return *(bool*)GetSetMediaItemInfo(GetMediaItemTake_Item(m_take), "B_UISEL", NULL);
}

void BR_LoudnessObject::SetSelectedInProject (bool selected)
{
	SWS_SectionLock lock(&m_mutex);

	if (!this->IsTargetValid())
		return;

	if (m_track)
	{
		int sel = (selected) ? (1) : (0);
		GetSetMediaTrackInfo(m_track, "I_SELECTED", &sel);
	}
	else
	{
		MediaItem* item = GetMediaItemTake_Item(m_take);
		GetSetMediaItemInfo(item, "B_UISEL", &selected);
		UpdateItemInProject(item);
	}
}

void BR_LoudnessObject::GotoMomentaryMax (bool timeSelection)
{
	SWS_SectionLock lock(&m_mutex);

	if (!this->IsTargetValid() && !m_momentaryValues.size())
		return;

	double newPosition = (m_track) ? (0) : (GetMediaItemInfo_Value(GetMediaItemTake_Item(m_take), "D_POSITION"));
	for (size_t i = 0; i < m_momentaryValues.size(); ++i)
	{
		if (m_momentaryValues[i] == m_momentaryMax)
			break;
		else
			newPosition += 0.4;
	}

	MediaTrack* track = (m_track) ? (m_track) : (GetMediaItemTake_Track(m_take));

	PreventUIRefresh(1);
	SetEditCurPos2(NULL, newPosition, true, false);
	ScrollToTrackIfNotInArrange(track);
	if (timeSelection)
	{
		double timeselEnd = newPosition + 0.4;
		GetSet_LoopTimeRange2(NULL, true, false, &newPosition, &timeselEnd, false);
	}
	PreventUIRefresh(-1);
}

void BR_LoudnessObject::GotoShortTermMax (bool timeSelection)
{
	SWS_SectionLock lock(&m_mutex);

	if (!this->IsTargetValid() && !m_shortTermValues.size())
		return;

	double newPosition = (m_track) ? (0) : (GetMediaItemInfo_Value(GetMediaItemTake_Item(m_take), "D_POSITION"));
	for (size_t i = 0; i < m_shortTermValues.size(); ++i)
	{
		if (m_shortTermValues[i] == m_shortTermMax)
			break;
		else
			newPosition += 3;
	}

	MediaTrack* track = (m_track) ? (m_track) : (GetMediaItemTake_Track(m_take));

	PreventUIRefresh(1);
	SetEditCurPos2(NULL, newPosition, true, false);
	ScrollToTrackIfNotInArrange(track);
	if (timeSelection)
	{
		double timeselEnd = newPosition + 3;
		GetSet_LoopTimeRange2(NULL, true, false, &newPosition, &timeselEnd, false);
	}
	PreventUIRefresh(-1);
}

void BR_LoudnessObject::GotoTarget ()
{
	SWS_SectionLock lock(&m_mutex);

	if (!this->IsTargetValid())
		return;

	MediaTrack* track = NULL;
	if (m_track)
		track = m_track;
	else
	{
		track = GetMediaItemTake_Track(m_take);
		SetArrangeStart(GetMediaItemInfo_Value(GetMediaItemTake_Item(m_take), "D_POSITION"));
	}
	ScrollToTrackIfNotInArrange(track);
}

unsigned WINAPI BR_LoudnessObject::AnalyzeData (void* loudnessObject)
{
	// Analyze results that get saved at the end
	double integrated   = NEGATIVE_INF;
	double momentaryMax = NEGATIVE_INF;
	double shortTermMax = NEGATIVE_INF;
	double range        = 0;
	vector<double> momentaryValues;
	vector<double> shortTermValues;

	// Get take/track info
	BR_LoudnessObject* _this = (BR_LoudnessObject*)loudnessObject;
	AudioData* audioData     = 	_this->GetAudioData();
	AudioAccessor* audio     = audioData->audio;
	int samplerate           = audioData->samplerate;
	int channels             = audioData->channels;
	int channelMode          = audioData->channelMode;
	double startTime         = audioData->audioStart;
	double endTime           = audioData->audioEnd;
	double volume            = audioData->volume;
	double pan               = audioData->pan;

	BR_Envelope& volEnv      = audioData->volEnv;
	BR_Envelope& volEnvPreFX = audioData->volEnvPreFX;
	bool doPan               = (channels > 1 && pan != 0)                      ? (true) : (false); // tracks will always get false here (see CheckSetAudioData())
	bool doVolEnv            = (volEnv.Count()      && volEnv.IsActive())      ? (true) : (false);
	bool doVolPreFXEnv       = (volEnvPreFX.Count() && volEnvPreFX.IsActive()) ? (true) : (false);
	bool integratedOnly      = _this->GetIntegratedOnly();

	// Prepare ebur123_state
	ebur128_state* loudnessState = NULL;
	if (integratedOnly)
		loudnessState = ebur128_init((size_t)channels, (size_t)samplerate, EBUR128_MODE_I);
	else
		loudnessState = ebur128_init((size_t)channels, (size_t)samplerate, EBUR128_MODE_M | EBUR128_MODE_S | EBUR128_MODE_I | EBUR128_MODE_LRA);

	// Ignore channels according to channel mode. Note: we can't partially request samples, i.e. channel mode is mono, but take is stereo...asking for
	// 1 channel only won't work. We must always request the real channel count even though reaper interleaves active channels starting from 0
	if (channelMode > 1)
	{
		// Mono channel modes
		if (channelMode <= 66)
		{
			ebur128_set_channel(loudnessState, 0, EBUR128_LEFT);
			for (int i = 1; i <= channels; ++i)
				ebur128_set_channel(loudnessState, i, EBUR128_UNUSED);
		}
		// Stereo channel modes
		else
		{
			ebur128_set_channel(loudnessState, 0, EBUR128_LEFT);
			ebur128_set_channel(loudnessState, 1, EBUR128_RIGHT);
			for (int i = 2; i <= channels; ++i)
				ebur128_set_channel(loudnessState, i, EBUR128_UNUSED);
		}
	}
	else
	{
		ebur128_set_channel(loudnessState, 0, EBUR128_LEFT);
		ebur128_set_channel(loudnessState, 1, EBUR128_RIGHT);
		ebur128_set_channel(loudnessState, 2, EBUR128_CENTER);
		ebur128_set_channel(loudnessState, 3, EBUR128_LEFT_SURROUND);
		ebur128_set_channel(loudnessState, 4, EBUR128_RIGHT_SURROUND);
	}

	// This lets us get integrated reading even if the target is too short
	bool doShortTerm = true;
	bool doMomentary = true;
	double effectiveEndTime = endTime;
	if (endTime - startTime < 3)
	{
		doShortTerm = false; // momentary checking is done during calculation
		endTime = startTime + 3;
	}

	// Fill loudnessState with samples and get momentary/short-term measurements
	int sampleCount = samplerate/5;
	int bufSz = sampleCount*channels;
	int processedSamples = 0;
	double sampleTimeLen = 1/samplerate;
	double currentTime   = startTime;
	int i = 0;
	int momentaryFilled = 1;
	while (currentTime < endTime && !_this->GetKillFlag())
	{
		// Make sure we always fill our buffer exactly to audio end (and skip momentary/short-term intervals if not enough new samples)
		bool skipIntervals = false;
		if (endTime - currentTime < 0.2 + numeric_limits<double>::epsilon())
		{
			sampleCount = (int)(samplerate * (endTime - currentTime));
			bufSz = sampleCount * channels;
			skipIntervals = true;
		}

		// Get new 200 ms of samples
		double* buf = new double[bufSz];
		GetAudioAccessorSamples(audio, samplerate, channels, currentTime, sampleCount, buf);

		// Correct for volume, pan and volume envelopes
		int currentChannel = 1;
		double sampleTime = currentTime;
		for (int j = 0; j < bufSz; ++j)
		{
			double adjust = 1;

			// Adjust for volume envelopes
			if (doVolPreFXEnv || doVolEnv)
			{
				double nextSampleTime = (currentChannel+1 > channels) ? (sampleTime + sampleTimeLen) : (-1);

				if (doVolPreFXEnv) adjust *= volEnvPreFX.ValueAtPosition(sampleTime);
				if (doVolEnv)      adjust *= volEnv.ValueAtPosition(sampleTime);
				if (nextSampleTime != -1)
					sampleTime = nextSampleTime;
			}

			// Adjust for volume fader
			adjust *= volume;

			// Adjust for pan fader (takes only)
			if (doPan)
			{
				if (pan > 0 && (currentChannel % 2 == 1))
					adjust *= 1 - pan;                         // takes have no pan law!
				else if (pan < 0 && (currentChannel % 2 == 0))
					adjust *= 1 + pan;
			}

			buf[j] *= adjust;

			if (++currentChannel > channels)
				currentChannel = 1;
		}
		ebur128_add_frames_double(loudnessState, buf, sampleCount);
		delete[] buf;

		if (!integratedOnly && !skipIntervals)
		{
			if (!doShortTerm && doMomentary)
			{
				if (currentTime + 0.2 >= effectiveEndTime + numeric_limits<double>::epsilon())
					doMomentary = false;
			}

			// Momentary buffer (400 ms) filled
			if (i % 2 == momentaryFilled && doMomentary)
			{
				double momentary;
				ebur128_loudness_momentary(loudnessState, &momentary);
				if (momentary == -HUGE_VAL)
					momentary = NEGATIVE_INF;
				if (momentary > momentaryMax)
					momentaryMax = momentary;
				momentaryValues.push_back(momentary);
			}

			// Short-term buffer (3000 ms) filled
			if (i == 14 && doShortTerm)
			{
				double shortTerm;
				ebur128_loudness_shortterm(loudnessState, &shortTerm);
				if (shortTerm == -HUGE_VAL)
					shortTerm = NEGATIVE_INF;
				if (shortTerm > shortTermMax)
					shortTermMax = shortTerm;
				shortTermValues.push_back(shortTerm);
			}
		}

		// This is definitely more accurate than adding 0.2 seconds every time
		processedSamples += sampleCount;
		currentTime = startTime + ((double)processedSamples / (double)samplerate);

		_this->SetProgress ((currentTime-startTime) / (endTime - startTime) * 0.95); // loudness_global and loudness_range seem rather fast and since we currently
		if (++i == 15)                                                               // can't monitor their progress, leave last 10% of progress for them
		{
			i = 0;
			momentaryFilled = (momentaryFilled == 1) ? (0) : (1);
		}

		// We reached the end of the file, break without checking currentTime against endTime (rounding errors could make us go through loop one more time)
		if (skipIntervals)
			break;
	}

	// Get integrated and loudness range
	if (!_this->GetKillFlag())
	{
		ebur128_loudness_global(loudnessState, &integrated);
		if (!integratedOnly)
			ebur128_loudness_range(loudnessState, &range);
	}
	ebur128_destroy(&loudnessState);

	// Write analyze data
	if (!_this->GetKillFlag())
	{
		_this->SetAnalyzeData(integrated, range, shortTermMax, momentaryMax, shortTermValues, momentaryValues);
		_this->SetProgress(1);
		_this->SetRunning(false);
		if (!integratedOnly)
			_this->SetAnalyzedStatus(true);
	}
	return 0;
}

int BR_LoudnessObject::CheckSetAudioData ()
{
	SWS_SectionLock lock(&m_mutex);

	if (!this->IsTargetValid())
		return 0;

	char newHash[128]; memset(newHash, 0, 128);
	GetAudioAccessorHash(m_audioData.audio, newHash);

	double audioStart = GetAudioAccessorStartTime(m_audioData.audio);
	double audioEnd   = GetAudioAccessorEndTime(m_audioData.audio);
	int channels    = (m_track) ? ((int)GetMediaTrackInfo_Value(m_track, "I_NCHAN")) : ((GetMediaItemTake_Source(m_take))->GetNumChannels());
	int channelMode = (m_track) ? (0) : (*(int*)GetSetMediaItemTakeInfo(m_take, "I_CHANMODE", NULL));
	int samplerate; GetConfig("projsrate", samplerate);

	double volume = 0, pan = 0;
	BR_Envelope volEnv, volEnvPreFX;
	if (m_track)
	{
		volume = *(double*)GetSetMediaTrackInfo(m_track, "D_VOL", NULL);
		pan = 0; // ignore track pan
		volEnv = BR_Envelope(GetVolEnv(m_track));
		volEnvPreFX = BR_Envelope(GetVolEnvPreFX(m_track));
	}
	else
	{
		volume = (*(double*)GetSetMediaItemTakeInfo(m_take, "D_VOL", NULL)) * (*(double*)GetSetMediaItemInfo(GetMediaItemTake_Item(m_take), "D_VOL", NULL));
		pan = *(double*)GetSetMediaItemTakeInfo(m_take, "D_PAN", NULL);
		volEnv = BR_Envelope(m_take, VOLUME);
	}


	if (AudioAccessorValidateState(m_audioData.audio)     ||
	    strcmp(newHash, m_audioData.audioHash)            ||
	    audioStart  != m_audioData.audioStart             ||
	    audioEnd    != m_audioData.audioEnd               ||
	    channels    != m_audioData.channels               ||
	    channelMode != m_audioData.channelMode            ||
	    samplerate  != m_audioData.samplerate             ||
	    fabs(volume - m_audioData.volume) >= VOLUME_DELTA ||
	    fabs(pan    - m_audioData.pan)    >= PAN_DELTA    ||
	    volEnv      != m_audioData.volEnv                 ||
	    volEnvPreFX != m_audioData.volEnvPreFX
	    )
	{
		DestroyAudioAccessor(m_audioData.audio);
		m_audioData.audio = (m_track) ? (CreateTrackAudioAccessor(m_track)) : (CreateTakeAudioAccessor(m_take));
		memset(m_audioData.audioHash, 0, 128);
		GetAudioAccessorHash(m_audioData.audio, m_audioData.audioHash);

		m_audioData.audioStart  = GetAudioAccessorStartTime(m_audioData.audio);;
		m_audioData.audioEnd    = GetAudioAccessorEndTime(m_audioData.audio);
		m_audioData.channels    = channels;
		m_audioData.channelMode = channelMode;
		m_audioData.samplerate  = samplerate;
		m_audioData.volume      = volume;
		m_audioData.pan         = pan;
		m_audioData.volEnv      = volEnv;
		m_audioData.volEnvPreFX = volEnvPreFX;

		this->SetAnalyzedStatus(false);
		return 2;
	}
	else
		return 1;
}

BR_LoudnessObject::AudioData* BR_LoudnessObject::GetAudioData ()
{
	SWS_SectionLock lock(&m_mutex);
	return &m_audioData;
}

void BR_LoudnessObject::SetRunning (bool running)
{
	SWS_SectionLock lock(&m_mutex);
	m_running = running;
}

void BR_LoudnessObject::SetProgress (double progress)
{
	SWS_SectionLock lock(&m_mutex);
	m_progress = progress;
}

void BR_LoudnessObject::SetAnalyzeData (double integrated, double range, double shortTermMax, double momentaryMax, vector<double>& shortTermValues, vector<double>& momentaryValues)
{
	SWS_SectionLock lock(&m_mutex);
	m_integrated   = integrated;
	m_shortTermMax = shortTermMax;
	m_momentaryMax = momentaryMax;
	m_range        = range;
	m_shortTermValues = shortTermValues;
	m_momentaryValues = momentaryValues;
}

void BR_LoudnessObject::GetAnalyzeData (double* integrated, double* range, double* shortTermMax, double* momentaryMax)
{
	SWS_SectionLock lock(&m_mutex);
	WritePtr(integrated,   m_integrated);
	WritePtr(range,        m_range);
	WritePtr(shortTermMax, m_shortTermMax);
	WritePtr(momentaryMax, m_momentaryMax);
}

void BR_LoudnessObject::SetAnalyzedStatus (bool analyzed)
{
	SWS_SectionLock lock(&m_mutex);
	m_analyzed = analyzed;
}

bool BR_LoudnessObject::GetAnalyzedStatus ()
{
	SWS_SectionLock lock(&m_mutex);
	return m_analyzed;
}

void BR_LoudnessObject::SetIntegratedOnly (bool integratedOnly)
{
	SWS_SectionLock lock(&m_mutex);
	m_integratedOnly = integratedOnly;
}

bool BR_LoudnessObject::GetIntegratedOnly ()
{
	SWS_SectionLock lock(&m_mutex);
	return m_integratedOnly;
}

void BR_LoudnessObject::SetKillFlag (bool killFlag)
{
	SWS_SectionLock lock(&m_mutex);
	m_killFlag = killFlag;
}

bool BR_LoudnessObject::GetKillFlag ()
{
	SWS_SectionLock lock(&m_mutex);
	return m_killFlag;
}

void BR_LoudnessObject::SetProcess (HANDLE process)
{
	SWS_SectionLock lock(&m_mutex);
	m_process = process;
}

HANDLE BR_LoudnessObject::GetProcess ()
{
	SWS_SectionLock lock(&m_mutex);
	return m_process;
}

WDL_FastString BR_LoudnessObject::GetTakeName ()
{
	SWS_SectionLock lock(&m_mutex);

	WDL_FastString takeName;
	if (m_track)
	{
		takeName.Set("-");
	}
	else
	{
		if (this->IsTargetValid())
			takeName.Set((char*)GetSetMediaItemTakeInfo(m_take, "P_NAME", NULL));
		else
			takeName.Set(__LOCALIZE("--NO TAKE FOUND--", "sws_DLG_174")); // Analyze loudness window should remove invalid objects, but leave this anyway
	}

	return takeName;
}

WDL_FastString BR_LoudnessObject::GetTrackName ()
{
	SWS_SectionLock lock(&m_mutex);

	WDL_FastString trackName;
	if (m_track)
	{
		if (this->IsTargetValid())
		{
			if (m_track == GetMasterTrack(NULL))
				trackName.Set(__localizeFunc("MASTER", "track", 0));
			else
				trackName.Set((char*)GetSetMediaTrackInfo(m_track, "P_NAME", NULL));
		}
		else
			trackName.Set(__LOCALIZE("--NO TRACK FOUND--", "sws_DLG_174")); // Analyze loudness window should remove invalid objects, but leave this anyway
	}
	else
	{
		if (this->IsTargetValid())
			trackName.Set((char*)GetSetMediaTrackInfo(GetMediaItemTake_Track(m_take), "P_NAME", NULL));
		else
			trackName.Set(__LOCALIZE("--NO TAKE FOUND--", "sws_DLG_174")); // Analyze loudness window should remove invalid objects, but leave this anyway
	}

	return trackName;
}

BR_LoudnessObject::AudioData::AudioData () :
audio       (NULL),
samplerate  (0),
channels    (0),
channelMode (0),
audioStart  (0),
audioEnd    (0),
volume      (0),
pan         (0)
{
	memset(audioHash, 0, 128);
}

/******************************************************************************
* Additional dialogs and normalize loudness                                   *
******************************************************************************/
struct BR_NormalizeData
{
	WDL_PtrList<BR_LoudnessObject>* items;
	double targetLufs;
	bool quickMode;   // analyze integrated loudness only (used when normalizing with actions)
	bool success;     // did items get successfully normalized?
	bool userCanceled;
};

static WDL_DLGRET NormalizeDialogProgressProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, uMsg, wParam, lParam))
		return r;

	static BR_NormalizeData* normalizeData = NULL;

	static BR_LoudnessObject* currentItem = NULL;
	static int currentItemId       = 0;
	static double itemsLen         = 0;
	static double currentItemLen   = 0;
	static double finishedItemsLen = 0;
	static bool analyzeInProgress  = false;

	#ifndef _WIN32
		static bool positionSet = false;
	#endif

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			// Reset variables
			normalizeData = (BR_NormalizeData*)lParam;
			if (!normalizeData || !normalizeData->items)
			{
				EndDialog(hwnd, 0);
				return 0;
			}

			currentItemId = 0;
			itemsLen = 0;
			currentItemLen = 0;
			finishedItemsLen = 0;
			analyzeInProgress = false;

			// Get progress data
			for (int i = 0; i < normalizeData->items->GetSize(); ++i)
			{
				if (BR_LoudnessObject* item = normalizeData->items->Get(i))
					itemsLen += item->GetAudioLength();
			}
			if (itemsLen == 0) itemsLen = 1; // to prevent division by zero


			#ifdef _WIN32
				CenterDialog(hwnd, g_hwndParent, HWND_TOPMOST);
			#else
				positionSet = false;
			#endif

			// Start normalizing
			SetTimer(hwnd, ANALYZE_TIMER_FREQ, 100, NULL);
		}
		break;

		#ifndef _WIN32
			case WM_ACTIVATE:
			{
				// SetWindowPos doesn't seem to work in WM_INITDIALOG on OSX
				// when creating a dialog with DialogBox so call here
				if (!positionSet)
					CenterDialog(hwnd, GetParent(hwnd), HWND_TOPMOST);
				positionSet = true;
			}
			break;
		#endif

		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case IDCANCEL:
				{
					KillTimer(hwnd, 1);
					normalizeData = NULL;
					if (currentItem)
						currentItem->StopAnalyze();
					EndDialog(hwnd, 0);
				}
				break;
			}
		}
		break;

		case WM_TIMER:
		{
			if (!normalizeData)
				return 0;

			if (!analyzeInProgress)
			{
				// No more objects to analyze, normalize them
				if (currentItemId >= normalizeData->items->GetSize())
				{
					bool undoTrack = false;
					for (int i = 0; i < normalizeData->items->GetSize(); ++i)
					{
						if (BR_LoudnessObject* item = normalizeData->items->Get(i))
						{
							item->NormalizeIntegrated(normalizeData->targetLufs);
							undoTrack = item->IsTrack();
						}
					}

					if (normalizeData->items->GetSize())
					{
						if (undoTrack)
							Undo_OnStateChangeEx2(NULL, __LOCALIZE("Normalize track loudness","sws_undo"), UNDO_STATE_ALL, -1);
						else
							Undo_OnStateChangeEx2(NULL, __LOCALIZE("Normalize item loudness","sws_undo"), UNDO_STATE_ITEMS, -1);
					}

					normalizeData->success = true;
					UpdateTimeline();
					EndDialog(hwnd, 0);
					return 0;
				}

				// Start analyzing next item
				if (currentItem = normalizeData->items->Get(currentItemId))
				{
					currentItemLen = currentItem->GetAudioLength();
					currentItem->Analyze(normalizeData->quickMode);
					analyzeInProgress = true;
				}
				else
					++currentItemId;
			}
			else
			{
				if (currentItem->IsRunning())
				{
					double progress = (finishedItemsLen + currentItemLen * currentItem->GetProgress()) / itemsLen;
					SendMessage(GetDlgItem(hwnd, IDC_PROGRESS), PBM_SETPOS, (int)(progress*100), 0);
				}
				else
				{
					finishedItemsLen += currentItemLen;
					double progress = finishedItemsLen / itemsLen;
					SendMessage(GetDlgItem(hwnd, IDC_PROGRESS), PBM_SETPOS, (int)(progress*100), 0);

					analyzeInProgress = false;
					++currentItemId;
				}
			}
		}
		break;

		case WM_DESTROY:
		{
			KillTimer(hwnd, 1);
			normalizeData = NULL;
			if (currentItem)
				currentItem->StopAnalyze();
		}
		break;
	}
	return 0;
}

static WDL_DLGRET NormalizeDialogProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, uMsg, wParam, lParam))
		return r;

	static BR_NormalizeData* normalizeData = NULL;
	#ifndef _WIN32
		static bool positionSet = false;
	#endif

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			char tmp[256];
			GetPrivateProfileString("SWS", NORMALIZE_KEY, "-23", tmp, 256, get_ini_file());
			double value = AltAtof(tmp);
			sprintf(tmp, "%.6g", value);
			SetDlgItemText(hwnd, IDC_EDIT, tmp);

			// Reset variables
			normalizeData = (BR_NormalizeData*)lParam;
			if (!normalizeData)
				EndDialog(hwnd, 0);

			#ifdef _WIN32
				CenterDialog(hwnd, g_hwndParent, HWND_TOPMOST);
			#else
				positionSet = false;
			#endif
		}
		break;

		#ifndef _WIN32
			case WM_ACTIVATE:
			{
				// SetWindowPos doesn't seem to work in WM_INITDIALOG on OSX
				// when creating a dialog with DialogBox so call here
				if (!positionSet)
					CenterDialog(hwnd, GetParent(hwnd), HWND_TOPMOST);
				positionSet = true;
			}
			break;
		#endif

		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case IDOK:
				{
					char target[128];
					GetDlgItemText(hwnd, IDC_EDIT, target, 128);

					normalizeData->userCanceled = false;
					normalizeData->targetLufs = AltAtof(target);
					EndDialog(hwnd, 0);
				}
				break;

				case IDCANCEL:
				{
					normalizeData->userCanceled = true;
					EndDialog(hwnd, 0);
				}
				break;
			}
		}
		break;

		case WM_DESTROY:
		{
			normalizeData = NULL;
			char target[128];
			GetDlgItemText(hwnd, IDC_EDIT, target, 128);
			WritePrivateProfileString("SWS", NORMALIZE_KEY, target, get_ini_file());
		}
		break;
	}
	return 0;
}

static WDL_DLGRET GraphDialogProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwnd, uMsg, wParam, lParam))
		return r;

	static bool* userCanceled = NULL;
	#ifndef _WIN32
		static bool positionSet = false;
	#endif

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			userCanceled = (bool*)lParam;
			if (!userCanceled)
				EndDialog(hwnd, 0);

			char tmp[128];
			sprintf(tmp, "%.6g", g_loudnessWndManager.Get()->m_properties.graphMin);
			SetDlgItemText(hwnd, IDC_MIN, tmp);
			sprintf(tmp, "%.6g", g_loudnessWndManager.Get()->m_properties.graphMax);
			SetDlgItemText(hwnd, IDC_MAX, tmp);

			#ifdef _WIN32
				CenterDialog(hwnd, g_hwndParent, HWND_TOPMOST);
			#else
				positionSet = false;
			#endif
		}
		break;

		#ifndef _WIN32
			case WM_ACTIVATE:
			{
				// SetWindowPos doesn't seem to work in WM_INITDIALOG on OSX
				// when creating a dialog with DialogBox so call here
				if (!positionSet)
					CenterDialog(hwnd, GetParent(hwnd), HWND_TOPMOST);
				positionSet = true;
			}
			break;
		#endif

		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case IDOK:
				{
					*userCanceled = false;
					EndDialog(hwnd, 0);
				}
				break;

				case IDCANCEL:
				{
					*userCanceled = true;
					EndDialog(hwnd, 0);
				}
				break;
			}
		}
		break;


		case WM_DESTROY:
		{
			char eMin[128], eMax[128];
			GetDlgItemText(hwnd, IDC_MIN, eMin, 128);
			GetDlgItemText(hwnd, IDC_MAX, eMax, 128);
			double min = AltAtof(eMin);
			double max = AltAtof(eMax);
			if (min > max) swap(min, max);
			g_loudnessWndManager.Get()->m_properties.graphMin = min;
			g_loudnessWndManager.Get()->m_properties.graphMax = max;
		}
		break;
	}
	return 0;
}

static void Normalize (BR_NormalizeData* normalizeData)
{
	DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_BR_NORMALIZE_LOUDNESS), g_hwndParent, NormalizeDialogProgressProc, (LPARAM)normalizeData);
}

static void GetUserNormalizeValue (BR_NormalizeData* normalizeData)
{
	DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_BR_NORMALIZE_LOUDNESS_USER), g_hwndParent, NormalizeDialogProc, (LPARAM)normalizeData);
}

/******************************************************************************
* Analyze loudness list view                                                  *
******************************************************************************/
BR_AnalyzeLoudnessView::BR_AnalyzeLoudnessView (HWND hwndList, HWND hwndEdit)
:SWS_ListView(hwndList, hwndEdit, COL_COUNT, g_cols, LOUDNESS_VIEW_WND, false, "sws_DLG_174")
{
}

void BR_AnalyzeLoudnessView::GetItemText (SWS_ListItem* item, int iCol, char* str, int iStrMax)
{
	WritePtr(str, '\0');
	if (BR_LoudnessObject* listItem = (BR_LoudnessObject*)item)
		listItem->GetColumnStr(iCol, str, iStrMax);
}

void BR_AnalyzeLoudnessView::GetItemList (SWS_ListItemList* pList)
{
	for (int i = 0; i < g_analyzedObjects.Get()->GetSize(); i++)
		pList->Add((SWS_ListItem*)g_analyzedObjects.Get()->Get(i));
}

void BR_AnalyzeLoudnessView::OnItemSelChanged (SWS_ListItem* item, int iState)
{
	if (g_loudnessWndManager.Get()->m_properties.mirrorSelection && item)
	{
		BR_LoudnessObject* listItem = (BR_LoudnessObject*)item;
		listItem->SetSelectedInProject(!!iState);
	}
}

void BR_AnalyzeLoudnessView::OnItemDblClk (SWS_ListItem* item, int iCol)
{
	if (BR_LoudnessObject* listItem = (BR_LoudnessObject*)item)
	{
		if (iCol == COL_SHORTTERM)
			listItem->GotoShortTermMax(g_loudnessWndManager.Get()->m_properties.timeSelectionOverMax);
		else if (iCol == COL_MOMENTARY)
			listItem->GotoMomentaryMax(g_loudnessWndManager.Get()->m_properties.timeSelectionOverMax);
		else if (g_loudnessWndManager.Get()->m_properties.doubleClickGotoTarget)
			listItem->GotoTarget();
	}
}

int BR_AnalyzeLoudnessView::OnItemSort (SWS_ListItem* item1, SWS_ListItem* item2)
{
	int column = abs(m_iSortCol)-1;

	if ((item1 && item2) && (column == COL_INTEGRATED || column == COL_RANGE || column == COL_SHORTTERM || column == COL_MOMENTARY))
	{
		double i1 = ((BR_LoudnessObject*)item1)->GetColumnVal(column);
		double i2 = ((BR_LoudnessObject*)item2)->GetColumnVal(column);

		int iRet = 0;
		if (i1 > i2)      iRet = 1;
		else if (i1 < i2) iRet = -1;

		if (m_iSortCol < 0) return -iRet;
		else                return  iRet;
	}
	else
		return SWS_ListView::OnItemSort(item1, item2);
}

/******************************************************************************
* Analyze loudness window                                                     *
******************************************************************************/
BR_AnalyzeLoudnessWnd::BR_AnalyzeLoudnessWnd () :
SWS_DockWnd(IDD_BR_ANALYZE_LOUDNESS, __LOCALIZE("Loudness","sws_DLG_174"), "", SWSGetCommandID(AnalyzeLoudness)),
m_objectsLen        (0),
m_currentObjectId   (0),
m_analyzeInProgress (false),
m_list              (NULL)
{
	m_id.Set(LOUDNESS_WND);
	Init(); // Must call SWS_DockWnd::Init() to restore parameters and open the window if necessary
}

void BR_AnalyzeLoudnessWnd::StopAnalyze ()
{
	KillTimer(m_hwnd, ANALYZE_TIMER);
	ShowWindow(GetDlgItem(m_hwnd, IDC_OPTIONS), SW_SHOW);
	ShowWindow(GetDlgItem(m_hwnd, IDC_PROGRESS), SW_HIDE);
	SendMessage(GetDlgItem(m_hwnd, IDC_PROGRESS), PBM_SETPOS, 0, 0);

	m_analyzeQueue.Empty(true);
	m_analyzeInProgress = false;
	m_objectsLen      = 0;
	m_currentObjectId = 0;
}

void BR_AnalyzeLoudnessWnd::StopReanalyze ()
{
	KillTimer(m_hwnd, REANALYZE_TIMER);
	ShowWindow(GetDlgItem(m_hwnd, IDC_OPTIONS), SW_SHOW);
	ShowWindow(GetDlgItem(m_hwnd, IDC_PROGRESS), SW_HIDE);
	SendMessage(GetDlgItem(m_hwnd, IDC_PROGRESS), PBM_SETPOS, 0, 0);

	m_reanalyzeQueue.Empty(false);
	m_analyzeInProgress = false;
	m_objectsLen      = 0;
	m_currentObjectId = 0;
}

void BR_AnalyzeLoudnessWnd::ClearList ()
{
	g_analyzedObjects.Get()->Empty(true);
	m_list->Update();
}

bool BR_AnalyzeLoudnessWnd::IsObjectInAnalyzeQueue (MediaTrack* track)
{
	for (int i = 0; i < m_analyzeQueue.GetSize(); ++i)
	{
		if (BR_LoudnessObject* object = m_analyzeQueue.Get(i))
		{
			if (object->CheckTarget(track))
				return true;
		}
	}
	return false;
}

bool BR_AnalyzeLoudnessWnd::IsObjectInAnalyzeQueue (MediaItem_Take* take)
{
	for (int i = 0; i < m_analyzeQueue.GetSize(); ++i)
	{
		if (BR_LoudnessObject* object = m_analyzeQueue.Get(i))
		{
			if (object->CheckTarget(take))
				return true;
		}
	}
	return false;
}

void BR_AnalyzeLoudnessWnd::OnInitDlg ()
{
	m_parentVwnd.SetRealParent(m_hwnd);
	m_vwnd_painter.SetGSC(WDL_STYLE_GetSysColor);

	m_properties.Load();

	m_resize.init_item(IDC_LIST, 0.0, 0.0, 1.0, 1.0);
	m_resize.init_item(IDC_PROGRESS, 0.0, 1.0, 1.0, 1.0);
	m_resize.init_item(IDC_ANALYZE, 0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_CANCEL, 0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_OPTIONS, 1.0, 1.0, 1.0, 1.0);
	ShowWindow(GetDlgItem(m_hwnd, IDC_PROGRESS), SW_HIDE);
	ShowWindow(GetDlgItem(m_hwnd, IDC_OPTIONS), SW_SHOW);

	m_list = new BR_AnalyzeLoudnessView(GetDlgItem(m_hwnd, IDC_LIST), GetDlgItem(m_hwnd, IDC_EDIT));
	m_pLists.Add(m_list);
	m_list->Update();

	SetTimer(m_hwnd, UPDATE_TIMER, UPDATE_TIMER_FREQ, NULL);
}

void BR_AnalyzeLoudnessWnd::OnCommand (WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
		case IDC_ANALYZE:
		{
			this->StopAnalyze();
			this->StopReanalyze();

			if (!m_properties.clearAnalyzed)
			{
				for (int i = 0; i < g_analyzedObjects.Get()->GetSize(); ++i)
				{
					if (BR_LoudnessObject* object = g_analyzedObjects.Get()->Get(i))
						m_analyzeQueue.Add(object);
				}
				g_analyzedObjects.Get()->Empty(false);
			}
			else
				this->ClearList();

			// Add currently selected tracks/takes to queue
			if (m_properties.analyzeTracks)
			{
				// Can't analyze master for now (accessor doesn't take receives into account) but leave in case this changes
				if (*(int*)GetSetMediaTrackInfo(GetMasterTrack(NULL), "I_SELECTED", NULL))
				{
					if (m_properties.clearAnalyzed || !this->IsObjectInAnalyzeQueue(GetMasterTrack(NULL)))
						m_analyzeQueue.Add(new BR_LoudnessObject(GetMasterTrack(NULL)));
				}
				for (int i = 0; i < CountSelectedTracks(NULL); ++i)
				{
					if (m_properties.clearAnalyzed || !this->IsObjectInAnalyzeQueue(GetSelectedTrack(NULL, i)))
						m_analyzeQueue.Add(new BR_LoudnessObject(GetSelectedTrack(NULL, i)));
				}
			}
			else
			{
				for (int i = 0; i < CountSelectedMediaItems(NULL); ++i)
				{
					if (MediaItem_Take* take = GetActiveTake(GetSelectedMediaItem(NULL, i)))
					{
						if (m_properties.clearAnalyzed || !this->IsObjectInAnalyzeQueue(take))
							m_analyzeQueue.Add(new BR_LoudnessObject(take));
					}
				}
			}

			if (m_analyzeQueue.GetSize())
			{
				for (int i = 0; i < m_analyzeQueue.GetSize(); ++i)
					m_objectsLen += m_analyzeQueue.Get(i)->GetAudioLength();

				// Prevent division by zero when calculating progress
				if (m_objectsLen == 0) m_objectsLen = 1;

				// Start timer which will analyze each object and finally update the list view
				SendMessage(GetDlgItem(m_hwnd, IDC_PROGRESS), PBM_SETPOS, 0, 0);
				ShowWindow(GetDlgItem(m_hwnd, IDC_PROGRESS), SW_SHOW);
				ShowWindow(GetDlgItem(m_hwnd, IDC_OPTIONS), SW_HIDE);
				SetTimer(m_hwnd, ANALYZE_TIMER, ANALYZE_TIMER_FREQ, NULL);
			}
		}
		break;

		case REANALYZE_ITEMS:
		{
			this->StopAnalyze();
			this->StopReanalyze();
			m_list->Update();

			int x = 0;
			while (BR_LoudnessObject* listItem = (BR_LoudnessObject*)m_list->EnumSelected(&x))
				m_reanalyzeQueue.Add(listItem);

			if (m_reanalyzeQueue.GetSize())
			{
				for (int i = 0; i < m_reanalyzeQueue.GetSize(); ++i)
					m_objectsLen += m_reanalyzeQueue.Get(i)->GetAudioLength();

				// Prevent division by zero when calculating progress
				if (m_objectsLen == 0) m_objectsLen = 1;

				// Start timer which will analyze each object and finally update the list view
				SendMessage(GetDlgItem(m_hwnd, IDC_PROGRESS), PBM_SETPOS, 0, 0);
				ShowWindow(GetDlgItem(m_hwnd, IDC_PROGRESS), SW_SHOW);
				ShowWindow(GetDlgItem(m_hwnd, IDC_OPTIONS), SW_HIDE);
				SetTimer(m_hwnd, REANALYZE_TIMER, ANALYZE_TIMER_FREQ, NULL);
			}
		}
		break;

		case NORMALIZE:
		case NORMALIZE_TO_23:
		{
			this->StopAnalyze();
			this->StopReanalyze();
			m_list->Update();

			WDL_PtrList<BR_LoudnessObject> itemsToNormalize;
			int x = 0;
			while (BR_LoudnessObject* listItem = (BR_LoudnessObject*)m_list->EnumSelected(&x))
				itemsToNormalize.Add(listItem);
			BR_NormalizeData normalizeData = {&itemsToNormalize, 0, false, false, false};
			if (wParam == NORMALIZE)
				GetUserNormalizeValue(&normalizeData);
			else
				normalizeData.targetLufs = -23;

			if (!normalizeData.userCanceled)
			{
				Normalize(&normalizeData);
				m_list->Update();
				if (m_properties.analyzeOnNormalize && normalizeData.success)
					this->OnCommand(REANALYZE_ITEMS, 0);
			}
		}
		break;

		case DRAW_SHORTTERM:
		case DRAW_MOMENTARY:
		{
			// Stop any work in progress
			this->StopAnalyze();
			this->StopReanalyze();
			m_list->Update();

			bool userCanceled = true;
			DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_BR_GRAPH_LOUDNESS), g_hwndParent, GraphDialogProc, (LPARAM)&userCanceled);
			if (!userCanceled)
			{
				if (TrackEnvelope* env = GetSelectedTrackEnvelope(NULL))
				{
					BR_Envelope envelope(env);
					if (m_properties.clearEnvelope)
					{
						envelope.DeletePoints(1, envelope.Count()-1);
						envelope.UnselectAll();
						double position = 0;
						double value = envelope.LaneMinValue();
						double bezier = 0;
						int shape = SQUARE;
						envelope.SetPoint(0, &position, &value, &shape, &bezier);
					}

					bool momentary = (wParam == DRAW_MOMENTARY) ? (true) : (false);
					bool success = false;
					int x = 0;
					while (BR_LoudnessObject* listItem = (BR_LoudnessObject*)m_list->EnumSelected(&x))
						success = listItem->CreateGraph(envelope, this->m_properties.graphMin, this->m_properties.graphMax, momentary);

					if (success && envelope.Commit())
						Undo_OnStateChangeEx2(NULL, __LOCALIZE("Draw loudness graph in active envelope","sws_undo"), UNDO_STATE_ALL, -1);
				}
			}
		}
		break;

		case DELETE_ITEM:
		{
			int x = 0;
			while (BR_LoudnessObject* listItem = (BR_LoudnessObject*)m_list->EnumSelected(&x))
			{
				m_reanalyzeQueue.Delete(m_reanalyzeQueue.Find(listItem), false);
				m_analyzeQueue.Delete(m_analyzeQueue.Find(listItem), true);

				int id = g_analyzedObjects.Get()->Find(listItem);
				if (id >= 0)
					g_analyzedObjects.Get()->Delete(id, true);
			}
			m_list->Update();
		}
		break;

		case IDC_OPTIONS:
		{
			HWND hwnd = GetDlgItem(m_hwnd, IDC_OPTIONS);
			RECT r;  GetClientRect(hwnd, &r);
			ClientToScreen(hwnd, (LPPOINT)&r);
			ClientToScreen(hwnd, ((LPPOINT)&r)+1);
			SendMessage(m_hwnd, WM_CONTEXTMENU, 0, MAKELPARAM((UINT)(r.left), (UINT)(r.bottom+SNM_1PIXEL_Y)));
			break;
		}

		case SET_ANALYZE_TARGET:
		{
			m_properties.analyzeTracks = !m_properties.analyzeTracks;
		}
		break;

		case SET_MIRROR_SELECTION:
		{
			m_properties.mirrorSelection = !m_properties.mirrorSelection;
		}
		break;

		case SET_ANALYZE_ON_NORMALIZE:
		{
			m_properties.analyzeOnNormalize = !m_properties.analyzeOnNormalize;
		}
		break;

		case SET_CLEAR_ON_ANALYZE:
		{
			m_properties.clearAnalyzed = !m_properties.clearAnalyzed;
		}
		break;

		case SET_DOUBLECLICK_GOTO_TARGET:
		{
			m_properties.doubleClickGotoTarget = !m_properties.doubleClickGotoTarget;
		}
		break;

		case SET_TIMESEL_OVER_MAX:
		{
			m_properties.timeSelectionOverMax = !m_properties.timeSelectionOverMax;
		}
		break;

		case SET_CLEAR_ENVELOPE:
		{
			m_properties.clearEnvelope = !m_properties.clearEnvelope;
		}
		break;

		case GO_TO_SHORTTERM:
		{
			if (BR_LoudnessObject* listItem = (BR_LoudnessObject*)m_list->EnumSelected(NULL))
				listItem->GotoShortTermMax(this->m_properties.timeSelectionOverMax);
		}
		break;

		case GO_TO_MOMENTARY:
		{
			if (BR_LoudnessObject* listItem = (BR_LoudnessObject*)m_list->EnumSelected(NULL))
				listItem->GotoMomentaryMax(this->m_properties.timeSelectionOverMax);
		}
		break;

		case IDC_CANCEL:
		{
			if (m_analyzeQueue.GetSize())
			{
				this->StopAnalyze();
				this->ClearList();
			}
			if (m_reanalyzeQueue.GetSize())
			{
				this->StopReanalyze();
			}
			m_list->Update();
		}
		break;
	}
}

void BR_AnalyzeLoudnessWnd::OnTimer (WPARAM wParam)
{
	static BR_LoudnessObject* currentObject      = NULL;
	static double             currentObjectLen   = 0;
	static double             finishedObjectsLen = 0;

	if (wParam == ANALYZE_TIMER)
	{
		if (!m_analyzeInProgress)
		{
			// New analyze task began, reset variables
			if (m_currentObjectId == 0)
				finishedObjectsLen = 0;

			if (!m_analyzeQueue.GetSize())
			{
				// Make sure list view isn't populated with invalid items (i.e. user could have deleted them during analyzing)
				for (int i = 0; i < g_analyzedObjects.Get()->GetSize(); ++i)
				{
					if (BR_LoudnessObject* object = g_analyzedObjects.Get()->Get(i))
					{	if (!object->IsTargetValid())
						{
							g_analyzedObjects.Get()->Delete(i, true);
							--i;
						}
					}
				}
				m_list->Update();

				m_analyzeInProgress = false;
				ShowWindow(GetDlgItem(m_hwnd, IDC_OPTIONS), SW_SHOW);
				ShowWindow(GetDlgItem(m_hwnd, IDC_PROGRESS), SW_HIDE);
				SendMessage(GetDlgItem(m_hwnd, IDC_PROGRESS), PBM_SETPOS, 0, 0);
				KillTimer(m_hwnd, ANALYZE_TIMER);
				return;
			}
			else
			{
				if (currentObject = m_analyzeQueue.Get(0))
				{
					currentObjectLen = currentObject->GetAudioLength();
					currentObject->Analyze();
					m_analyzeInProgress = true;
				}
				else
					m_analyzeQueue.Delete(0, true);
				++m_currentObjectId;
			}
		}
		else
		{
			// Make sure our object is still here (user can't delete it since it's not yet visible but leave for safety)
			if (m_analyzeQueue.Find(currentObject) != -1 && currentObject->IsRunning())
			{
				double progress = (finishedObjectsLen + currentObjectLen * currentObject->GetProgress()) / m_objectsLen;
				SendMessage(GetDlgItem(m_hwnd, IDC_PROGRESS), PBM_SETPOS, (int)(progress*100), 0);
			}
			else
			{
				if (m_analyzeQueue.Find(currentObject) != -1)
				{
					g_analyzedObjects.Get()->Add(currentObject);
					m_analyzeQueue.Delete(m_analyzeQueue.Find(currentObject), false);
				}

				finishedObjectsLen += currentObjectLen;
				double progress = finishedObjectsLen / m_objectsLen;
				SendMessage(GetDlgItem(m_hwnd, IDC_PROGRESS), PBM_SETPOS, (int)(progress*100), 0);
				m_analyzeInProgress = false;
			}
		}
	}
	else if (wParam == REANALYZE_TIMER)
	{
		if (!m_analyzeInProgress)
		{
			// New analyze task began, reset variables
			if (m_currentObjectId == 0)
				finishedObjectsLen = 0;

			if (!m_reanalyzeQueue.GetSize())
			{
				m_list->Update();
				m_analyzeInProgress = false;
				ShowWindow(GetDlgItem(m_hwnd, IDC_OPTIONS), SW_SHOW);
				ShowWindow(GetDlgItem(m_hwnd, IDC_PROGRESS), SW_HIDE);
				SendMessage(GetDlgItem(m_hwnd, IDC_PROGRESS), PBM_SETPOS, 0, 0);
				KillTimer(m_hwnd, ANALYZE_TIMER);
				return;
			}
			else
			{
				if (currentObject = m_reanalyzeQueue.Get(0))
				{
					currentObjectLen = currentObject->GetAudioLength();
					currentObject->Analyze();
					m_analyzeInProgress = true;
				}
				else
					m_reanalyzeQueue.Delete(0, false);
				++m_currentObjectId;
			}
		}
		else
		{
			// Make sure our object is still here (user could have deleted it)
			if (m_reanalyzeQueue.Find(currentObject) != -1 && currentObject->IsRunning())
			{
				double progress = (finishedObjectsLen + currentObjectLen * currentObject->GetProgress()) / m_objectsLen;
				SendMessage(GetDlgItem(m_hwnd, IDC_PROGRESS), PBM_SETPOS, (int)(progress*100), 0);
			}
			else
			{
				if (m_reanalyzeQueue.Find(currentObject) != -1)
					m_reanalyzeQueue.Delete(m_reanalyzeQueue.Find(currentObject), false);

				finishedObjectsLen += currentObjectLen;
				double progress = finishedObjectsLen / m_objectsLen;
				SendMessage(GetDlgItem(m_hwnd, IDC_PROGRESS), PBM_SETPOS, (int)(progress*100), 0);
				m_analyzeInProgress = false;
			}
		}
	}
	else if (wParam == UPDATE_TIMER)
	{
		static ReaProject* project = NULL;
		static ReaProject* currentProject = NULL;

		// Check user didn't change project tab
		currentProject = EnumProjects(-1, NULL, 0);
		if (currentProject != project)
		{
			project = currentProject;
			m_list->Update();
			return;
		}

		// Check for take/track name, selection, deletion updates
		if (!m_analyzeQueue.GetSize())
		{
			HWND hwnd = m_list->GetHWND();
			bool update = false;
			for (int i = 0; i < ListView_GetItemCount(hwnd); ++i)
			{
				if (BR_LoudnessObject* listItem = (BR_LoudnessObject*)m_list->GetListItem(i))
				{
					if (listItem->IsTargetValid())
					{
						char listName[CELL_MAX_LEN] = "";
						char realName[CELL_MAX_LEN] = "";

						ListView_GetItemText(hwnd, i, COL_TRACK, listName, sizeof(listName));
						listItem->GetColumnStr(COL_TRACK, realName, sizeof(realName));
						if (strcmp(listName, realName))
							update = true;

						ListView_GetItemText(hwnd, i, COL_TAKE, listName, sizeof(listName));
						listItem->GetColumnStr(COL_TAKE, realName, sizeof(realName));
						if (strcmp(listName, realName))
							update = true;

						if (m_properties.mirrorSelection)
						{
							if (listItem->IsSelectedInProject())
							{
								ListView_SetItemState(hwnd, i, LVIS_SELECTED, LVIS_SELECTED);
							}
							else
							{
								ListView_SetItemState(hwnd, i, 0, LVIS_SELECTED);
							}
						}
					}
					else
					{
						// Remove from reanalyze and analyze queues first!
						m_reanalyzeQueue.Delete(m_reanalyzeQueue.Find(listItem), false);
						m_analyzeQueue.Delete(m_analyzeQueue.Find(listItem), true);

						int id = g_analyzedObjects.Get()->Find(listItem);
						g_analyzedObjects.Get()->Delete(id, true);
						update = true;
					}
				}
			}
			if (update)
				m_list->Update();
		}
	}
}

void BR_AnalyzeLoudnessWnd::OnDestroy ()
{
	this->StopAnalyze();
	this->StopReanalyze();
	m_properties.Save();
	KillTimer(m_hwnd, UPDATE_TIMER);
}

void BR_AnalyzeLoudnessWnd::GetMinSize (int* w, int* h)
{
	*w = 275;
	*h = 160;
}

int BR_AnalyzeLoudnessWnd::OnKey (MSG* msg, int iKeyState)
{
	if (msg->message == WM_KEYDOWN && msg->wParam == VK_DELETE && !iKeyState)
	{
		this->OnCommand(DELETE_ITEM, 0);
		return 1;
	}
	return 0;
}

HMENU BR_AnalyzeLoudnessWnd::OnContextMenu (int x, int y, bool* wantDefaultItems)
{
	HMENU menu = CreatePopupMenu();

	int column;
	if ((BR_LoudnessObject*)m_pLists.Get(0)->GetHitItem(x, y, &column))
	{
		WritePtr(wantDefaultItems, false);
		AddToMenu(menu, __LOCALIZE("Normalize to -23 LUFS", "sws_DLG_174"), NORMALIZE_TO_23, -1, false);
		AddToMenu(menu, __LOCALIZE("Normalize...", "sws_DLG_174"), NORMALIZE, -1, false);
		AddToMenu(menu, __LOCALIZE("Analyze", "sws_DLG_174"), REANALYZE_ITEMS, -1, false);
		AddToMenu(menu, SWS_SEPARATOR, 0);
		AddToMenu(menu, __LOCALIZE("Go to maximum short-term", "sws_DLG_174"), GO_TO_SHORTTERM, -1, false);
		AddToMenu(menu, __LOCALIZE("Go to maximum momentary", "sws_DLG_174"), GO_TO_MOMENTARY, -1, false);
		AddToMenu(menu, SWS_SEPARATOR, 0);
		AddToMenu(menu, __LOCALIZE("Create short-term graph in selected envelope", "sws_DLG_174"), DRAW_SHORTTERM, -1, false);
		AddToMenu(menu, __LOCALIZE("Create momentary graph in selected envelope", "sws_DLG_174"), DRAW_MOMENTARY, -1, false);
		AddToMenu(menu, SWS_SEPARATOR, 0);
		AddToMenu(menu, __LOCALIZE("Remove", "sws_DLG_174"), DELETE_ITEM, -1, false);
	}
	else
	{
		bool button = false;
		RECT r;	GetWindowRect(GetDlgItem(m_hwnd, IDC_OPTIONS), &r);
		POINT pt = {x, y + 3*(SNM_1PIXEL_Y*(-1))}; // +/- 3 trick by Jeffos, see OnCommand()
		if (PtInRect(&r, pt))
		{
			WritePtr(wantDefaultItems, false);
			button = true;
		}
		AddToMenu(menu, __LOCALIZE("Analyze selected tracks", "sws_DLG_174"), SET_ANALYZE_TARGET, -1, false, m_properties.analyzeTracks ?  MF_CHECKED : MF_UNCHECKED);
		AddToMenu(menu, __LOCALIZE("Analyze selected items", "sws_DLG_174"), SET_ANALYZE_TARGET, -1, false, !m_properties.analyzeTracks ?  MF_CHECKED : MF_UNCHECKED);
		AddToMenu(menu, SWS_SEPARATOR, 0);

		HMENU optionsMenu = CreatePopupMenu();
		AddToMenu((button ? menu : optionsMenu), __LOCALIZE("Clear list when analyzing", "sws_DLG_174"), SET_CLEAR_ON_ANALYZE, -1, false, m_properties.clearAnalyzed ?  MF_CHECKED : MF_UNCHECKED);
		AddToMenu((button ? menu : optionsMenu), __LOCALIZE("Analyze after normalizing", "sws_DLG_174"), SET_ANALYZE_ON_NORMALIZE, -1, false, m_properties.analyzeOnNormalize ?  MF_CHECKED : MF_UNCHECKED);
		AddToMenu((button ? menu : optionsMenu), SWS_SEPARATOR, 0);
		AddToMenu((button ? menu : optionsMenu), __LOCALIZE("Mirror project selection", "sws_DLG_174"), SET_MIRROR_SELECTION, -1, false, m_properties.mirrorSelection ?  MF_CHECKED : MF_UNCHECKED);
		AddToMenu((button ? menu : optionsMenu), __LOCALIZE("Double-click moves arrange to track/take", "sws_DLG_174"), SET_DOUBLECLICK_GOTO_TARGET, -1, false, m_properties.doubleClickGotoTarget ?  MF_CHECKED : MF_UNCHECKED);
		AddToMenu((button ? menu : optionsMenu), __LOCALIZE("Double-click maximum short-term/momentary to create time selection", "sws_DLG_174"), SET_TIMESEL_OVER_MAX, -1, false, m_properties.timeSelectionOverMax ?  MF_CHECKED : MF_UNCHECKED);
		AddToMenu((button ? menu : optionsMenu), SWS_SEPARATOR, 0);
		AddToMenu((button ? menu : optionsMenu), __LOCALIZE("Clear envelope when creating loudness graph", "sws_DLG_174"), SET_CLEAR_ENVELOPE, -1, false, m_properties.clearEnvelope ?  MF_CHECKED : MF_UNCHECKED);
		if (!button)
			AddSubMenu(menu, optionsMenu, __LOCALIZE("Options", "sws_menu"), 0);
	}
	return menu;
}

BR_AnalyzeLoudnessWnd::Properties::Properties () :
analyzeTracks         (false),
analyzeOnNormalize    (true),
mirrorSelection       (true),
doubleClickGotoTarget (true),
timeSelectionOverMax  (false),
clearEnvelope         (true),
clearAnalyzed         (true),
graphMin              (-41),
graphMax              (-14)
{
}

void BR_AnalyzeLoudnessWnd::Properties::Load ()
{
	int analyzeTracksInt         = 0;
	int analyzeOnNormalizeInt    = 1;
	int mirrorSelectionInt       = 1;
	int doubleClickGotoTargetInt = 1;
	int timeSelectionOverMaxInt  = 0;
	int clearEnvelopeInt         = 1;
	int clearAnalyzedInt         = 1;

	char tmp[256];
	GetPrivateProfileString("SWS", LOUDNESS_KEY, "0 1 1 1 0 1 -41 -14 1", tmp, 256, get_ini_file());
	sscanf(tmp, "%d %d %d %d %d %d %lf %lf %d", &analyzeTracksInt, &analyzeOnNormalizeInt, &mirrorSelectionInt, &doubleClickGotoTargetInt, &timeSelectionOverMaxInt, &clearEnvelopeInt, &graphMin, &graphMax, &clearAnalyzedInt);

	analyzeTracks         = !!analyzeTracksInt;
	analyzeOnNormalize    = !!analyzeOnNormalizeInt;
	mirrorSelection       = !!mirrorSelectionInt;
	doubleClickGotoTarget = !!doubleClickGotoTargetInt;
	timeSelectionOverMax  = !!timeSelectionOverMaxInt;
	clearEnvelope         = !!clearEnvelopeInt;
	clearAnalyzed         = !!clearAnalyzedInt;
}

void BR_AnalyzeLoudnessWnd::Properties::Save ()
{
	int analyzeTracksInt         = analyzeTracks;
	int analyzeOnNormalizeInt    = analyzeOnNormalize;
	int mirrorSelectionInt       = mirrorSelection;
	int doubleClickGotoTargetInt = doubleClickGotoTarget;
	int timeSelectionOverMaxInt  = timeSelectionOverMax;
	int clearEnvelopeInt         = clearEnvelope;
	int clearAnalyzedInt         = clearAnalyzed;
	char tmp[256];
	_snprintfSafe(tmp, sizeof(tmp), "%d %d %d %d %d %d %lf %lf %d", analyzeTracksInt, analyzeOnNormalizeInt, mirrorSelectionInt, doubleClickGotoTargetInt, timeSelectionOverMaxInt, clearEnvelopeInt, graphMin, graphMax, clearAnalyzedInt);
	WritePrivateProfileString("SWS", LOUDNESS_KEY, tmp, get_ini_file());
}

/******************************************************************************
* Dialog management                                                           *
******************************************************************************/
void AnalyzeLoudness (COMMAND_T* ct)
{
	if (BR_AnalyzeLoudnessWnd* dialog = g_loudnessWndManager.Create())
		dialog->Show(true, true);
}

int IsAnalyzeLoudnessVisible (COMMAND_T* ct)
{
	if (BR_AnalyzeLoudnessWnd* dialog = g_loudnessWndManager.Get())
		return dialog->IsValidWindow();
	return 0;
}

int AnalyzeLoudnessInit ()
{
	g_loudnessWndManager.Init();
	return 1;
}

void AnalyzeLoudnessExit ()
{
	g_loudnessWndManager.Delete();
}

void CleanAnalyzeLoudnessProjectData ()
{
	g_analyzedObjects.Get()->Empty(true);
	g_analyzedObjects.Cleanup();
}

/******************************************************************************
* Commands                                                                    *
******************************************************************************/
void NormalizeLoudness (COMMAND_T* ct)
{
	BR_NormalizeData normalizeData = {NULL, -23, true, false, false};

	if ((int)ct->user == 0 || (int)ct->user == 2)
		GetUserNormalizeValue(&normalizeData);

	if (!normalizeData.userCanceled)
	{
		WDL_PtrList_DeleteOnDestroy<BR_LoudnessObject> objects;

		if ((int)ct->user == 0 || (int)ct->user == 1)
		{
			// Can't analyze master for now (accessor doesn't take receives into account) but leave in case this changes
			if (*(int*)GetSetMediaTrackInfo(GetMasterTrack(NULL), "I_SELECTED", NULL))
				objects.Add(new BR_LoudnessObject(GetMasterTrack(NULL)));

			for (int i = 0; i < CountSelectedTracks(NULL); ++i)
				objects.Add(new BR_LoudnessObject(GetSelectedTrack(NULL, i)));
		}
		else
		{
			for (int i = 0; i < CountSelectedMediaItems(NULL); ++i)
			{
				if (MediaItem_Take* take = GetActiveTake(GetSelectedMediaItem(NULL, i)))
					objects.Add(new BR_LoudnessObject(take));
			}
		}

		normalizeData.items = &objects;
		Normalize(&normalizeData);
	}
}