/******************************************************************************
/ BR_Util.cpp
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
#include "stdafx.h"
#include "BR_Util.h"
#include "BR_EnvTools.h"
#include "../SnM/SnM_Dlg.h"
#include "../SnM/SnM_Util.h"
#include "../reaper/localize.h"

/******************************************************************************
* Constants                                                                   *
******************************************************************************/
const int ITEM_LABEL_MIN_HEIGHT = 28;
const int TCP_MASTER_GAP        = 5;
const int ENV_GAP               = 4;  // bottom gap may seem like 3 when selected, but that
const int ENV_LINE_WIDTH        = 1;  // first pixel is used to "bold" selected envelope

const int TAKE_MIN_HEIGHT_COUNT = 10;
const int TAKE_MIN_HEIGHT_HIGH  = 12; // min height when take count <= TAKE_MIN_HEIGHT_COUNT
const int TAKE_MIN_HEIGHT_LOW   = 6;  // min height when take count >  TAKE_MIN_HEIGHT_COUNT

const int ENV_HIT_POINT         = 5;
const int ENV_HIT_POINT_LEFT    = 6;  // envelope point doesn't have middle pixel so hit point is different for one side
const int ENV_HIT_POINT_DOWN    = 6;  // +1 because lower part is tracked starting 1 pixel bellow line (so when envelope is active, hit points appear the same)

const int RULER_WIDTH_REGIONS   = 10;
const int RULER_WIDTH_MARKERS   = 12;
const int RULER_WIDTH_TEMPO     = 12;
const int RULER_WIDTH_TIMELINE  = 35;

/******************************************************************************
* Miscellaneous                                                               *
******************************************************************************/
bool IsFraction (char* str, double& convertedFraction)
{
	replace(str, str+strlen(str), ',', '.' );
	char* buf = strstr(str,"/");

	if (!buf)
	{
		convertedFraction = atof(str);
		_snprintfSafe(str, strlen(str)+1, "%g", convertedFraction);
		return false;
	}
	else
	{
		int num = atoi(str);
		int den = atoi(buf+1);
		_snprintfSafe(str, strlen(str)+1, "%d/%d", num, den);
		if (den != 0)
			convertedFraction = (double)num/(double)den;
		else
			convertedFraction = 0;
		return true;
	}
}

double AltAtof (char* str)
{
	replace(str, str+strlen(str), ',', '.' );
	return atof(str);
}

void ReplaceAll (string& str, string oldStr, string newStr)
{
	if (oldStr.empty())
		return;

	size_t pos = 0, oldLen = oldStr.length(), newLen = newStr.length();
	while ((pos = str.find(oldStr, pos)) != std::string::npos)
	{
		str.replace(pos, oldLen, newStr);
		pos += newLen;
	}
}

int Round (double val)
{
	if (val < 0)
		return (int)(val - 0.5);
	else
		return (int)(val + 0.5);
}

int GetBit (int val, int pos)
{
	return (val & 1 << pos) != 0;
}

int SetBit (int val, int pos)
{
	return val | 1 << pos;
}

int ToggleBit (int val, int pos)
{
	return val ^= 1 << pos;
}

int ClearBit (int val, int pos)
{
	return val & ~(1 << pos);
}

int GetFirstDigit (int val)
{
	val = abs(val);
	while (val >= 10)
		val /= 10;
	return val;
}

int GetLastDigit (int val)
{
	return abs(val) % 10;
}

vector<int> GetDigits(int val)
{
	int count = (int)(log10((float)val)) + 1;

	vector<int> digits;
	digits.resize(count);

	for (int i = count-1; i >= 0; --i)
	{
		digits[i] = GetLastDigit(val);
		val /= 10;
	}
	return digits;
}

/******************************************************************************
* General                                                                     *
******************************************************************************/
vector<MediaItem*> GetSelItems (MediaTrack* track)
{
	vector<MediaItem*> items;
	int count = CountTrackMediaItems(track);
	for (int i = 0; i < count ; ++i)
		if (*(bool*)GetSetMediaItemInfo(GetTrackMediaItem(track, i), "B_UISEL", NULL))
			items.push_back(GetTrackMediaItem(track, i));
	return items;
}

vector<double> GetProjectMarkers (bool timeSel)
{
	double tStart, tEnd;
	GetSet_LoopTimeRange2(NULL, false, false, &tStart, &tEnd, false);

	int i = 0;
	bool region;
	double pos;
	vector<double> markers;
	while (EnumProjectMarkers2(NULL, i, &region, &pos, NULL, NULL, NULL))
	{
		bool pRegion;
		double pPos;
		int previous = EnumProjectMarkers2(NULL, i-1, &pRegion, &pPos, NULL, NULL, NULL);
		if (!region)
		{
			// In certain corner cases involving regions, reaper will output the same marker more than
			// once. This should prevent those duplicate values from being written into the vector.
			// Note: this might have been fixed in 4.60, but it doesn't hurt to leave it...
			if (pos != pPos || (pos == pPos && pRegion) || !previous)
			{
				if (timeSel)
				{
					if (pos > tEnd)
						break;
					if (pos >= tStart)
						markers.push_back(pos);
				}
				else
					markers.push_back(pos);
			}
		}
		++i;
	}
	return markers;
}

double GetClosestGrid (double position)
{
	int snap; GetConfig("projshowgrid", snap);
	SetConfig("projshowgrid", ClearBit(snap, 8));

	double grid = SnapToGrid(NULL, position);
	SetConfig("projshowgrid", snap);

	return grid;
}

double GetClosestMeasureGrid (double position)
{
	double grid; GetConfig("projgriddiv", grid);
	if (grid/4 > 1) // if grid division is bigger than one 1 measure, let reaper handle it for us
		return GetClosestGrid(position);

	int prevMeasure;
	TimeMap2_timeToBeats(NULL, position, &prevMeasure, NULL, NULL, NULL);
	int nextMeasure = prevMeasure + 1;

	double prevPos = TimeMap2_beatsToTime(NULL, 0, &prevMeasure);
	double nextPos = TimeMap2_beatsToTime(NULL, 0, &nextMeasure);
	return ((position-prevPos) > (nextPos-position)) ? (nextPos) : (prevPos);
}

double EndOfProject (bool markers, bool regions)
{
	double projEnd = 0;
	int tracks = GetNumTracks();
	for (int i = 0; i < tracks; i++)
	{
		MediaTrack* track = GetTrack(0, i);
		MediaItem* item = GetTrackMediaItem(track, GetTrackNumMediaItems(track) - 1);
		double itemEnd = GetMediaItemInfo_Value(item, "D_POSITION") + GetMediaItemInfo_Value(item, "D_LENGTH");
		if (itemEnd > projEnd)
			projEnd = itemEnd;
	}

	if (markers || regions)
	{
		bool region; double start, end; int i = 0;
		while (i = EnumProjectMarkers(i, &region, &start, &end, NULL, NULL))
		{
			if (regions)
				if (region && end > projEnd)
					projEnd = end;
			if (markers)
				if (!region && start > projEnd)
					projEnd = start;
		}
	}
	return projEnd;
}

double GetProjectSettingsTempo (int* num, int* den)
{
	int _den;
	TimeMap_GetTimeSigAtTime(NULL, -1, num, &_den, NULL);
	double bpm;
	GetProjectTimeSignature2(NULL, &bpm, NULL);

	WritePtr(den, _den);
	return bpm/_den*4;
}

bool TcpVis (MediaTrack* track)
{
	if ((GetMasterTrack(NULL) == track))
	{
		int master; GetConfig("showmaintrack", master);
		if (master == 0 || (int)GetMediaTrackInfo_Value(track, "I_WNDH") == 0)
			return false;
		else
			return true;
	}
	else
	{
		if ((int)GetMediaTrackInfo_Value(track, "B_SHOWINTCP") == 0 || (int)GetMediaTrackInfo_Value(track, "I_WNDH") == 0)
			return false;
		else
			return true;
	}
}

/******************************************************************************
* Height helpers                                                              *
******************************************************************************/
static void GetTrackGap (int trackHeight, int* top, int* bottom)
{
	if (top)
	{
		int label; GetConfig("labelitems2", label);
		//   draw mode        ||   name                 pitch,              gain (opposite)
		if (!GetBit(label, 3) || (!GetBit(label, 0) && !GetBit(label, 2) && GetBit(label, 4)))
			*top = 0;
		else
		{
			int labelH = abs(SNM_GetColorTheme()->mediaitem_font.lfHeight) * 11/8;
			int labelMinH; GetConfig("itemlabel_minheight", labelMinH);

			if (trackHeight - labelH < labelMinH)
			{
				*top = 0;
			}
			else
			{
				// draw label inside item if it's making item too short
				if (GetBit(label, 5) && trackHeight - labelH < ITEM_LABEL_MIN_HEIGHT)
					*top = 0;
				else
					*top = labelH;
			}
		}
	}
	if (bottom)
		GetConfig("trackitemgap", *bottom);
}

static int GetEffectiveCompactLevel (MediaTrack* track)
{
	/* Real compact level is determined by the highest compact level *
	*  of any successive parent - important for determining height   *
	*  of the track                                                  */

	int compact = 0;
	MediaTrack* parent = track;
	while (parent = (MediaTrack*)GetSetMediaTrackInfo(parent, "P_PARTRACK", NULL))
	{
		int tmp = (int)GetMediaTrackInfo_Value(parent, "I_FOLDERCOMPACT");
		if (tmp > compact)
		{
			compact = tmp;
			if (compact == 2)
				break;
		}
	}
	return compact;
}

static int GetMinTakeHeight (MediaTrack* track, int takeCount, int trackHeight, int itemHeight)
{
	/* Take lanes have minimum height, after which they are hidden and *
	*  and only active take is displayed. This height is determined on *
	*  per track basis                                                 */

	int trackTop, trackBottom;
	GetTrackGap(trackHeight, &trackTop, &trackBottom);

	int overlappingItems; GetConfig("projtakelane", overlappingItems);
	if (GetBit(overlappingItems, 1))
	{
		if (itemHeight == (trackHeight - trackTop - trackBottom))
			overlappingItems = 0;
	}
	else
		overlappingItems = 0;

	int takeH;
	if (*(bool*)GetSetMediaTrackInfo(track, "B_FREEMODE", NULL) || overlappingItems)
	{
		int maxTakeH = (trackHeight - trackTop - trackBottom) / takeCount;

		// If max take height is over limits, reaper lets takes get
		// resized to 1 px in FIPM or when in overlapping lane
		if (takeCount > TAKE_MIN_HEIGHT_COUNT)
		{
			if (maxTakeH < TAKE_MIN_HEIGHT_LOW)
				takeH = TAKE_MIN_HEIGHT_LOW;
			else
				takeH = 1;
		}
		else
		{
			if (maxTakeH < TAKE_MIN_HEIGHT_HIGH)
				takeH = TAKE_MIN_HEIGHT_HIGH;
			else
				takeH = 1;
		}
	}
	else
	{
		if (takeCount > TAKE_MIN_HEIGHT_COUNT)
			takeH = TAKE_MIN_HEIGHT_LOW;
		else
			takeH = TAKE_MIN_HEIGHT_HIGH;
	}

	return takeH;
}

static int GetEffectiveTakeId (MediaItem_Take* take, MediaItem* item, int id, int* effectiveTakeCount)
{
	/* Since empty takes could be hidden, displayed id and real id can *
	*  differ which is important when determining take heights         */

	MediaItem*      validItem = (item) ? (item) : (GetMediaItemTake_Item(take));
	MediaItem_Take* validTake = (take) ? (take) : (GetTake(validItem, id));

	int count = CountTakes(validItem);
	int effectiveCount = 0;
	int effectiveId = -1;

	// Empty takes not displayed
	int emptyLanes; GetConfig("takelanes", emptyLanes);
	if (GetBit(emptyLanes, 2))
	{
		for (int i = 0; i < count; ++i)
		{
			if (MediaItem_Take* currentTake = GetTake(validItem, i))
			{
				if (currentTake == validTake)
					effectiveId = effectiveCount;
				++effectiveCount;
			}
		}
	}
	// Empty takes displayed
	else
	{
		if (take)
		{
			for (int i = 0; i < count; ++i)
			{
				if (GetTake(validItem, i) == validTake)
				{
					effectiveId = i;
					break;
				}
			}
		}
		else
		{
			effectiveId = id;
		}

		effectiveCount = count;
	}

	WritePtr(effectiveTakeCount, effectiveCount);
	return effectiveId;
}

static bool IsLastTakeTooTall (int itemHeight, int averageTakeHeight, int effectiveTakeCount, int* lastTakeHeight)
{
	/* When placing takes into item, reaper just divides item height by number of takes *
	*  losing division reminder - last take gets all of those reminders. However, if    *
	*  it ends up to tall (>= 1.5 of average take height) it gets cropped               */

	int lastTakeH = itemHeight - averageTakeHeight * (effectiveTakeCount - 1);
	if (lastTakeH < (int)(averageTakeHeight * 1.5))
	{
		WritePtr(lastTakeHeight, lastTakeH);
		return false;
	}
	else
	{
		WritePtr(lastTakeHeight, averageTakeHeight);
		return true;
	}
}

/******************************************************************************
* Height                                                                      *
******************************************************************************/
static int GetItemHeight (MediaItem* item, int* offsetY, int trackHeight)
{
	// Reaper takes into account FIPM and overlapping media item lanes
	// so no gotchas here besides track height checking since I_LASTH
	// might return wrong value when track height is 0
	if (trackHeight == 0)
		return 0;

	// supplied offsetY MUST hold item's track offset
	if (offsetY) *offsetY = *offsetY + *(int*)GetSetMediaItemInfo(item, "I_LASTY", NULL);
	return *(int*)GetSetMediaItemInfo(item, "I_LASTH", NULL);
}

static int GetTakeHeight (MediaItem_Take* take, MediaItem* item, int id, int* offsetY, bool averagedLast, int trackHeight, int trackOffset)
{
	// Function is for internal use, so it isn't possible to have both take and item valid, but opposite must be checked
	if (!take && !item)
	{
		WritePtr(offsetY, 0);
		return 0;
	}
	MediaItem_Take* validTake = (take) ? (take) : (GetTake(item, id));
	MediaItem* validItem = (item) ? (item) : (GetMediaItemTake_Item(take));
	MediaTrack* track = GetMediaItem_Track(validItem);

	// This gets us the offset to start of the item
	int takeOffset = trackOffset;
	int itemH = GetItemHeight(validItem, &takeOffset, trackHeight);

	int takeH;
	int takeLanes; GetConfig("projtakelane", takeLanes);

	// Take lanes displayed
	if (GetBit(takeLanes, 0))
	{
		int effTakeCount;
		int effTakeId = GetEffectiveTakeId(take, item, id, &effTakeCount); // using original pointers on purpose here
		takeH = itemH / effTakeCount; // average take height

		if (takeH < GetMinTakeHeight(track, effTakeCount, trackHeight, itemH))
		{
			if (GetActiveTake(validItem) == validTake)
				takeH = itemH;
			else
				takeH = 0;
		}
		else
		{
			if (effTakeId >= 0)
				takeOffset = takeOffset + takeH * effTakeId;

			// Last take gets leftover pixels from integer division
			if (!averagedLast && (effTakeId == effTakeCount - 1))
				IsLastTakeTooTall(itemH, takeH, effTakeCount, &takeH);
		}
	}
	// Take lanes not displayed
	else
	{
		if (GetActiveTake(validItem) == validTake)
			takeH = itemH;
		else
			takeH = 0;
	}

	WritePtr(offsetY, takeOffset);
	return takeH;
}

static int GetTakeHeight (MediaItem_Take* take, MediaItem* item, int id, int* offsetY, bool averagedLast)
{
	MediaItem* validItem = (item) ? (item) : (GetMediaItemTake_Item(take));
	int trackOffset;
	int trackHeight = GetTrackHeight(GetMediaItem_Track(validItem), &trackOffset);
	return GetTakeHeight (take, item, id, offsetY, averagedLast, trackHeight, trackOffset);
}

int GetTrackHeight (MediaTrack* track, int* offsetY)
{
	bool master = (GetMasterTrack(NULL) == track) ? (true) : (false);
	IconTheme* theme = SNM_GetIconTheme();

	// Get track's start Y coordinate
	if (offsetY)
	{
		int offset = 0;
		if (!master)
		{
			int count = CSurf_TrackToID(track, false) - 1;
			if (count >= -1)
			{
				for (int i = 0; i < count; ++i)
					offset += (int)GetMediaTrackInfo_Value(GetTrack(NULL, i), "I_WNDH");
				if (TcpVis(GetMasterTrack(NULL)))
					offset += (int)GetMediaTrackInfo_Value(GetMasterTrack(NULL), "I_WNDH") + TCP_MASTER_GAP;
			}
		}
		*offsetY = offset;
	}

	if (!TcpVis(track))
		return 0;

	int compact = GetEffectiveCompactLevel(track); // Track can't get resized at compact 2 and "I_HEIGHTOVERRIDE" could return
	if (compact == 2)                              // wrong value if track was resized prior to compacting so return here
		return theme->tcp_supercollapsed_height;

	// Get track height
	int height = (int)GetMediaTrackInfo_Value(track, "I_HEIGHTOVERRIDE");
	if (height == 0)
	{
		// Compacted track is not affected by all the zoom settings if there is no height override
		if (compact == 1)
			height = theme->tcp_small_height;
		else
		{
			// Get track height from vertical zoom
			int verticalZoom;
			GetConfig("vzoom2", verticalZoom);
			if (verticalZoom <= 4)
			{
				int fullArm; GetConfig("zoomshowarm", fullArm);
				int armed =  (int)GetMediaTrackInfo_Value(track, "I_RECARM");

				if (master)                 height = theme->tcp_master_min_height;
				else if (fullArm && armed)  height = theme->tcp_full_height;
				else if (verticalZoom == 0) height = theme->tcp_small_height;
				else if (verticalZoom == 1) height = (theme->tcp_small_height + theme->tcp_medium_height) / 2;
				else if (verticalZoom == 2) height = theme->tcp_medium_height;
				else if (verticalZoom == 3) height = (theme->tcp_medium_height + theme->tcp_full_height) / 2;
				else if (verticalZoom == 4) height = theme->tcp_full_height;
			}
			else
			{
				// Get max and min height
				RECT r; GetClientRect(GetArrangeWnd(), &r);
				int maxHeight = r.bottom - r.top;
				int minHeight = theme->tcp_full_height;

				// Calculate track height (method stolen from Zoom.cpp - Thanks Tim!)
				int normalizedZoom = (verticalZoom < 30) ? (verticalZoom - 4) : (30 + 3*(verticalZoom - 30) - 4);
				height = minHeight + ((maxHeight-minHeight)*normalizedZoom) / 56;
			}
		}
	}
	else
	{
		// If track is compacted by 1 level and it was manually resized just prior to compacting, I_HEIGHTOVERRIDE
		// will return previous size and not 0 even though track is behaving as so. Then again, user could have
		// resized track in compact mode so returned I_HEIGHTOVERRIDE would be correct in that case.
		// So get track's HWND in TCP and get height from it
		if (compact == 1)
		{
			RECT r;
			GetClientRect(GetTcpTrackWnd(track), &r);
			height = r.bottom - r.top;
		}
		// Check I_HEIGHTOVERRIDE against minimum height (i.e. changing theme resizes tracks that are below
		// minimum, but I_HEIGHTOVERRIDE doesn't get updated)
		else
		{

			if (master)
			{
				if (height < theme->tcp_master_min_height)
					height = theme->tcp_master_min_height;
			}
			else
			{
				if (height < theme->tcp_small_height)
					height = theme->tcp_small_height;
			}
		}
	}
	return height;
}

int GetItemHeight (MediaItem* item, int* offsetY)
{
	int trackH = GetTrackHeight(GetMediaItem_Track(item), offsetY);
	return GetItemHeight(item, offsetY, trackH);
}

int GetTakeHeight (MediaItem_Take* take, int* offsetY)
{
	return GetTakeHeight(take, NULL, 0, offsetY, false);
}

int GetTakeHeight (MediaItem* item, int id, int* offsetY)
{
	return GetTakeHeight(NULL, item, id, offsetY, false);
}

int GetTakeEnvHeight (MediaItem_Take* take, int* offsetY)
{
	// Last take may have different height then the others, but envelope is always positioned based
	// on take height of other takes so we get average take height (relevant for last take only)
	int envelopeH = GetTakeHeight(take, NULL, 0, offsetY, true) - 2*ENV_GAP;
	envelopeH = (envelopeH > ENV_LINE_WIDTH) ? envelopeH : 0;

	if (take && offsetY) *offsetY = *offsetY + ENV_GAP;
	return envelopeH;
}

int GetTakeEnvHeight (MediaItem* item, int id, int* offsetY)
{
	// Last take may have different height then the others, but envelope is always positioned based
	// on take height of other takes so we get average take height (relevant for last take only)
	int envelopeH = GetTakeHeight(NULL, item, id, offsetY, true) - 2*ENV_GAP;
	envelopeH = (envelopeH > ENV_LINE_WIDTH) ? envelopeH : 0;

	if (item && offsetY) *offsetY = *offsetY + ENV_GAP;
	return envelopeH;
}

int GetTrackEnvHeight (TrackEnvelope* envelope, int* offsetY, MediaTrack* parent /*=NULL*/)
{
	// If parent is NULL, search for it
	MediaTrack* track = (parent) ? (parent) : (GetEnvParent(envelope));
	if (!track || !envelope)
	{
		WritePtr(offsetY, 0);
		return 0;
	}

	// Prepare return variables and get track's height and offset
	int envOffset = 0;
	int envHeight = 0;
	int trackOffset = 0;
	int trackHeight = GetTrackHeight(track, (offsetY) ? (&trackOffset) : (NULL));

	// Get first envelope's lane hwnd and cycle through the rest
	HWND hwnd = GetWindow(GetTcpTrackWnd(track), GW_HWNDNEXT);
	MediaTrack* nextTrack = CSurf_TrackFromID(1 + CSurf_TrackToID(track, false), false);
	list<TrackEnvelope*> checkedEnvs;

	bool found = false;
	int envelopeId = GetEnvId (envelope, track);
	int count = CountTrackEnvelopes(track);
	for (int i = 0; i < count; ++i)
	{
		LONG_PTR hwndData = GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if ((MediaTrack*)hwndData == nextTrack)
			break;
		TrackEnvelope* currentEnvelope = (TrackEnvelope*)hwndData;

		// Envelope's lane has been found
		if (currentEnvelope == envelope)
		{
			RECT r; GetClientRect(hwnd, &r);
			envHeight = r.bottom - r.top - 2*ENV_GAP;
			found = true;
		}
		// No match, add to offset
		else
		{
			// In certain cases (depending if user scrolled up or down), envelope
			// hwnds (got by GW_HWNDNEXT) won't be in order they are drawn, so make
			// sure requested envelope is drawn under envelope currently in loop
			if (envelopeId > GetEnvId(currentEnvelope, track))
			{
				RECT r; GetClientRect(hwnd, &r);
				envOffset += r.bottom-r.top;
				checkedEnvs.push_back(currentEnvelope);
			}
		}
		hwnd = GetWindow(hwnd, GW_HWNDNEXT);
	}

	// Envelope lane has been found...just calculate the offset
	if (found)
	{
		envOffset = trackOffset + trackHeight + envOffset + ENV_GAP;
	}
	// Envelope lane wasn't found...so it's either hidden or drawn over track
	else
	{
		if (!EnvVis(envelope, NULL))
		{
			envHeight = 0;
			envOffset = 0;
		}
		else
		{
			int overlapLimit; GetConfig("env_ol_minh", overlapLimit);
			bool overlapEnv = (overlapLimit >= 0) ? (true) : (false);

			int trackGapTop, trackGapBottom;
			GetTrackGap(trackHeight, &trackGapTop, &trackGapBottom);

			int envLaneFull = trackHeight - trackGapTop - trackGapBottom;
			int envLaneCount = 0;
			int envLanePos = 0;
			bool foundEnv = false;

			int count = CountTrackEnvelopes(track);
			for (int i = 0; i < count; ++i)
			{
				TrackEnvelope* currentEnv = GetTrackEnvelope(track, i);
				std::list<TrackEnvelope*>::iterator it = find(checkedEnvs.begin(), checkedEnvs.end(), currentEnv);

				// Envelope was already encountered, no need to check visibility (chunks are expensive!) so just skip it
				if (it != checkedEnvs.end())
					checkedEnvs.erase(it); // and this is why we use list
				else
				{
					if (currentEnv != envelope)
					{
						if (EnvVis(currentEnv, NULL))
						{
							++envLaneCount;
							if (!foundEnv)
								++envLanePos;
						}
					}
					else
					{
						++envLaneCount;
						foundEnv = true;
					}
				}

				// Check current height
				if (envLaneCount != 0)
				{
					int envLaneH = envLaneFull / envLaneCount; // div reminder isn't important
					if (overlapEnv && envLaneH < overlapLimit)
					{
						envHeight = envLaneFull - 2*ENV_GAP;
						envOffset = trackOffset + trackGapTop + ENV_GAP;
						break;
					}

					// No need to calculate return values every iteration
					if (i == count-1)
					{
						envHeight = envLaneH - 2*ENV_GAP;
						envOffset = trackOffset + trackGapTop + envLanePos*envLaneH + ENV_GAP;
					}
				}
			}
		}
	}

	if (envHeight <= ENV_LINE_WIDTH)
		envHeight = 0;
	WritePtr (offsetY, envOffset);
	return envHeight;
}

/******************************************************************************
* Helpers for getting TCP/Arrange elements from Y                             *
******************************************************************************/
static MediaTrack* GetTrackAreaFromY (int y, int* offset)
{
	/* Check if Y is in some TCP track or it's envelopes, *
	*  returned offset is always for returned track       */

	MediaTrack* master = GetMasterTrack(NULL);
	MediaTrack* track = NULL;

	int trackEnd = 0;
	int trackOffset = 0;
	for (int i = 0; i <= GetNumTracks(); ++i)
	{
		MediaTrack* currentTrack = CSurf_TrackFromID(i, false);
		int height = *(int*)GetSetMediaTrackInfo(currentTrack, "I_WNDH", NULL);
		if (currentTrack == master && TcpVis(master))
			height += TCP_MASTER_GAP;
		trackEnd += height;

		if (y >= trackOffset && y < trackEnd)
		{
			track = currentTrack;
			break;
		}
		trackOffset += height;
	}

	WritePtr(offset, (track) ? (trackOffset) : (0));
	return track;
}

static MediaTrack* GetTrackFromY (int y, int* trackHeight, int* offset)
{
	int trackOffset = 0;
	int trackH = 0;
	MediaTrack* track = GetTrackAreaFromY(y, &trackOffset);
	if (track)
	{
		trackH = GetTrackHeight(track, NULL);
		if (y < trackOffset || y >= trackOffset + trackH)
			track = NULL;
	}

	WritePtr(trackHeight, (track) ? (trackH)      : (0));
	WritePtr(offset,      (track) ? (trackOffset) : (0));
	return track;
}

static MediaItem_Take* GetTakeFromItemY (MediaItem* item, int itemY, MediaTrack* track, int trackHeight)
{
	/* Does not check if Y is within the item, caller should handle that */

	if (!CountTakes(item))
		return NULL;

	MediaItem_Take* take = NULL;
	int takeLanes;
	GetConfig("projtakelane", takeLanes);

	// Take lanes displayed
	if (GetBit(takeLanes, 0))
	{
		int itemH = GetItemHeight(item, NULL, trackHeight);
		int effTakeCount;
		GetEffectiveTakeId(NULL, item, 0, &effTakeCount);
		int takeH = itemH / effTakeCount; // average take height

		if (takeH < GetMinTakeHeight(track, effTakeCount, trackHeight, itemH))
			take = GetActiveTake(item);
		else
		{
			int takeCount = CountTakes(item);
			int effTakeId = 0;

			// No hidden empty takes
			if (effTakeCount == takeCount)
			{
				int id = itemY / takeH;
				if (id > takeCount-1) // last take might be taller
					id = takeCount-1;

				take = GetTake(item, id);
				effTakeId = id;
			}
			// Empty takes hidden
			else
			{
				int visibleTakes = 0;
				for (int i = 0; i < takeCount; ++i)
				{
					if (MediaItem_Take* currentTake = GetTake(item, i))
					{
						int yStart = visibleTakes * takeH;
						int yEnd = yStart + ((i == takeCount-1) ? (takeH*2) : (takeH)); // last take might be taller
						if (itemY > yStart && itemY <= yEnd)
						{
							take = currentTake;
							break;
						}
						++visibleTakes;
					}
				}
				effTakeId = visibleTakes;
			}

			// If last take, check it's not cropped
			if (effTakeId == effTakeCount - 1 && IsLastTakeTooTall(itemH, takeH, effTakeCount, NULL))
				if (itemY > takeH * effTakeCount) // take cropped - check Y is not outside it
					take = NULL;
		}
	}
	// Take lanes not displayed
	else
		take = GetActiveTake(item);

	return take;
}

static MediaItem* GetItemFromY (int y, double position, MediaItem_Take** take)
{
	int trackH, offset;
	MediaTrack* track = GetTrackFromY(y, &trackH, &offset);
	MediaItem* item = NULL;
	int itemYStart = 0;

	double iStartLast = GetMediaItemInfo_Value(GetTrackMediaItem(track, 0), "D_POSITION");
	int count = CountTrackMediaItems(track);
	for (int i = 0; i < count ; ++i)
	{
		MediaItem* currentItem = GetTrackMediaItem(track, i);
		double iStart = GetMediaItemInfo_Value(currentItem, "D_POSITION");
		double iEnd = GetMediaItemInfo_Value(currentItem, "D_LENGTH") + iStart;

		if (position >= iStart && position <= iEnd)
		{
			if (iStart >= iStartLast)
			{
				int yStart = offset;                                             // due to FIMP/overlapping items in lanes, check every
				int yEnd = GetItemHeight(currentItem, &yStart, trackH) + yStart; // item - last one closest to cursor and whose height
				if (y > yStart && y < yEnd)                                      // overlaps with cursor Y position is the correct one
				{
					item = currentItem;
					itemYStart = yStart;
				}

			}
			iStartLast = iStart;
		}
	}

	WritePtr(take, GetTakeFromItemY(item, y-itemYStart, track, trackH));
	return item;
}

static bool SortEnvHeightsById (const pair<int,int>& left, const pair<int,int>& right)
{
	return left.second < right.second;
}

static void GetTrackOrEnvelopeFromY (int y, TrackEnvelope** _envelope, MediaTrack** _track, list<TrackEnvelope*>& envelopes, int* height, int* offset)
{
	/* If Y is at track get track pointer and all envelopes that have  *
	*  control panels in TCP. If Y is at envelope get the envelope and *
	*  it's track. Height and offset are returned for element under Y  *
	*                                                                  *
	*  The purpose of list of all envelopes in lanes is to skip        *
	*  checking their visibility via chunk parsing (expensive!)        */

	int elementOffset = 0;
	int elementHeight = 0;
	MediaTrack* track = GetTrackAreaFromY(y, &elementOffset);
	TrackEnvelope* envelope = NULL;
	if (track)
	{
		elementHeight = GetTrackHeight(track, NULL);
		vector<pair<int,int> > envHeights;
		bool yInTrack = (y < elementOffset + elementHeight) ? true : false;

		// Get first envelope's lane hwnd and cycle through the rest
		HWND hwnd = GetWindow(GetTcpTrackWnd(track), GW_HWNDNEXT);
		MediaTrack* nextTrack = CSurf_TrackFromID(1 + CSurf_TrackToID(track, false), false);
		int count = CountTrackEnvelopes(track);
		for (int i = 0; i < count; ++i)
		{
			LONG_PTR hwndData = GetWindowLongPtr(hwnd, GWLP_USERDATA);
			if ((MediaTrack*)hwndData == nextTrack)
				break;

			if (yInTrack)
				envelopes.push_back((TrackEnvelope*)hwndData);
			else
			{
				RECT r; GetClientRect(hwnd, &r);
				int envHeight = r.bottom - r.top;
				int envId = GetEnvId((TrackEnvelope*)hwndData, track);
				envHeights.push_back(make_pair(envHeight, envId));
			}

			hwnd = GetWindow(hwnd, GW_HWNDNEXT);
		}

		if (!yInTrack)
		{
			// Envelopes hwnds don't have to be in order they are drawn so need to sort them by id before searching
			std::sort(envHeights.begin(), envHeights.end(), SortEnvHeightsById);
			int envelopeStart = elementOffset + elementHeight;
			for (size_t i = 0; i < envHeights.size(); ++i)
			{
				int envelopeEnd = envelopeStart + envHeights[i].first;
				if (y >= envelopeStart && y < envelopeEnd)
				{
					envelope = GetTrackEnvelope(track, envHeights[i].second);
					elementHeight = envHeights[i].first;
					elementOffset = envelopeStart;
					break;
				}
				envelopeStart += envHeights[i].first;
			}
		}
	}

	WritePtr(_envelope, envelope);
	WritePtr(_track,    track);
	WritePtr(height,    elementHeight);
	WritePtr(offset,    elementOffset);
}

/******************************************************************************
* Arrange                                                                     *
******************************************************************************/
static bool IsPointInArrange (POINT* p, bool checkPointVisibilty = true)
{
	/* Check if point is in arrange. Since WindowFromPoint is sometimes   *
	*  called before this function, caller can choose not to check it but *
	*  only if point point is over arrange (disregarding scrollbars)      */

	HWND hwnd = GetArrangeWnd();
	RECT r; GetWindowRect(hwnd, &r);
	#ifndef _WIN32
	if (r.top > r.bottom)
	{
		int tmp = r.top;
		r.top = r.bottom;
		r.bottom = tmp;
	}
	#endif

	r.right  -= VERT_SCROLL_W + 1;
	r.bottom -= VERT_SCROLL_W + 1;
	bool pointInArrange = p->x >= r.left && p->x <= r.right && p->y >= r.top && p->y <= r.bottom;

	if (checkPointVisibilty)
		return pointInArrange && hwnd == WindowFromPoint(*p);
	else
		return pointInArrange;
}

static double PositionAtArrangePoint (POINT p, double* arrangeStart = NULL, double* arrangeEnd = NULL, double* hZoom = NULL)
{
	/* Does not check if point is in arrange, to make it work it *
	*  it is enough to have p.x in arrange, p.y can be somewhere *
	*  else like ruler etc...                                    */

	HWND hwnd = GetArrangeWnd();
	ScreenToClient(hwnd, &p);

	double start, end;
	RECT r; GetWindowRect(hwnd, &r);
	GetSet_ArrangeView2(NULL, false, r.left, r.right-VERT_SCROLL_W, &start, &end);
	GetClientRect(hwnd, &r);
	double zoom = GetHZoomLevel();

	WritePtr(arrangeStart, start);
	WritePtr(arrangeEnd, end);
	WritePtr(hZoom, zoom);
	return start  + (r.left + p.x) / zoom;
}

static int TranslatePointToArrangeScrollY (POINT p)
{
	/* returns real, scrollbar Y, not displayed Y */

	HWND hwnd = GetArrangeWnd();
	ScreenToClient(hwnd, &p);

	SCROLLINFO si = { sizeof(SCROLLINFO), };
	si.fMask = SIF_ALL;
	CoolSB_GetScrollInfo(hwnd, SB_VERT, &si);

	return (int)p.y + si.nPos;
}

static int TranslatePointToArrangeX (POINT p)
{
	/* returns displayed X, not real, scrollbar X */

	HWND hwnd = GetArrangeWnd();
	ScreenToClient(hwnd, &p);
	return (int)p.x;
}

void MoveArrange (double amountTime)
{
	RECT r;
	double startTime, endTime;
	GetWindowRect(GetArrangeWnd(), &r);

	GetSet_ArrangeView2(NULL, false, r.left, r.right-VERT_SCROLL_W, &startTime, &endTime);
	startTime += amountTime;
	endTime += amountTime;
	GetSet_ArrangeView2(NULL, true, r.left, r.right-VERT_SCROLL_W, &startTime, &endTime);
}

void CenterArrange (double position)
{
	RECT r;
	double startTime, endTime;
	GetWindowRect(GetArrangeWnd(), &r);

	GetSet_ArrangeView2(NULL, false, r.left, r.right-VERT_SCROLL_W, &startTime, &endTime);
	double halfSpan = (endTime - startTime) / 2;
	startTime = position - halfSpan;
	endTime = position + halfSpan;
	GetSet_ArrangeView2(NULL, true, r.left, r.right-VERT_SCROLL_W, &startTime, &endTime);
}

void MoveArrangeToTarget (double target, double reference)
{
	if (!IsOffScreen(target))
	{
		if (IsOffScreen(reference))
			MoveArrange(target - reference);
		else
			CenterArrange(target);
	}
}

bool IsOffScreen (double position)
{
	RECT r;
	double startTime, endTime;
	GetWindowRect(GetArrangeWnd(), &r);
	GetSet_ArrangeView2(NULL, false, r.left, r.right-VERT_SCROLL_W, &startTime, &endTime);

	if (position >= startTime && position <= endTime)
		return true;
	else
		return false;
}

/******************************************************************************
* Mouse cursor                                                                *
******************************************************************************/
static int IsMouseOverEnvelopeLine (BR_Envelope& envelope, int drawableEnvHeight, int yOffset, int mouseY, int mouseX, double mousePos, double arrangeStart, double arrangeEnd, double arrangeZoom)
{
	/* mouseX is displayed X, not taking scrollbars into account. mouseY on the  *
	*  other hand has to take scrollbar position into account                    *
	*                                                                            *
	*  Return values: 0 -> no hit, 1 -> over point, 2 - > over segment           */

	int mouseHit = 0;

	// Check if mouse is in drawable part of envelope lane where line resides
	if (mouseY >= yOffset && mouseY < yOffset + drawableEnvHeight)
	{
		double takePosOffset = (!envelope.IsTakeEnvelope()) ? (0) : GetMediaItemInfo_Value(GetMediaItemTake_Item(envelope.GetTake()), "D_POSITION");
		int prevId = envelope.FindPrevious(mousePos - takePosOffset);
		int nextId = prevId + 1;
		int tempoHit = (envelope.IsTempo()) ? (ENV_HIT_POINT) : (0); // for some reason, tempo points have double up/down hit area
		bool found = false;

		// Check surrounding points first - they take priority over segment
		if (!found)
		{
			double prevPos;
			if (envelope.GetPoint(prevId, &prevPos, NULL, NULL, NULL) && prevPos >= arrangeStart && prevPos <= arrangeEnd)
			{
				prevPos += takePosOffset;
				int x = Round(arrangeZoom * (prevPos - arrangeStart));
				int y = yOffset + drawableEnvHeight - Round(envelope.NormalizedDisplayValue(prevId) * drawableEnvHeight);
				if (CheckBounds(mouseX, x - ENV_HIT_POINT, x + ENV_HIT_POINT_LEFT) && CheckBounds(mouseY, y - ENV_HIT_POINT - tempoHit, y + ENV_HIT_POINT_DOWN + tempoHit))
				{
					mouseHit = 1;
					found = true;
				}
			}
		}
		if (!found)
		{
			double nextPos;
			if (envelope.GetPoint(nextId, &nextPos, NULL, NULL, NULL) && nextPos >= arrangeStart && nextPos <= arrangeEnd)
			{
				nextPos += takePosOffset;
				int x = Round(arrangeZoom * (nextPos - arrangeStart));
				int y = yOffset + drawableEnvHeight - Round(envelope.NormalizedDisplayValue(nextId) * drawableEnvHeight);
				if (CheckBounds(mouseX, x - ENV_HIT_POINT, x + ENV_HIT_POINT_LEFT) && CheckBounds(mouseY, y - ENV_HIT_POINT - tempoHit, y + ENV_HIT_POINT_DOWN + tempoHit))
				{
					mouseHit = 1;
					found = true;
				}
			}
		}

		// Not over points, check segment
		if (!found)
		{
			double mouseValue = envelope.ValueAtPosition(mousePos - takePosOffset);
			int x = Round(arrangeZoom * (mousePos - arrangeStart));
			int y = yOffset + drawableEnvHeight - Round(envelope.NormalizedDisplayValue(mouseValue) * drawableEnvHeight);
			if (CheckBounds(mouseX, x - ENV_HIT_POINT, x + ENV_HIT_POINT) && CheckBounds(mouseY, y - ENV_HIT_POINT, y + ENV_HIT_POINT_DOWN))
			{
				mouseHit = 2;
			}
		}
	}
	return mouseHit;
}

static int IsMouseOverEnvelopeLineTrackLane (MediaTrack* track, int trackHeight, int trackOffset, list<TrackEnvelope*>& laneEnvs, int mouseY, int mouseX, double mousePos, double arrangeStart, double arrangeEnd, double arrangeZoom, TrackEnvelope** trackEnvelope)
{
	/* laneEnv list should hold all track envelopes that have their own lane so *
	*  we don't have to check for their visibility in track lane (chunks are    *
	*  expensive). Also note that laneEnvs WILL get modified during the process *
	*                                                                           *
	*  MouseX is displayed X, not taking scrollbars into account. MouseY on the *
	*  other hand has to take scrollbar position into account                   *
	*                                                                           *
	*  Return values: 0 -> no hit, 1 -> over point, 2 - > over segment          *
	*  If there is a hit, trackEnvelope will hold envelope                      */

	int mouseHit = 0;
	TrackEnvelope* envelopeUnderMouse = NULL;

	// Get all track envelopes that appear in track lane
	vector<TrackEnvelope*> trackLaneEnvs;
	int count = CountTrackEnvelopes(track);
	for (int i = 0; i < count; ++i)
	{
		TrackEnvelope* envelope = GetTrackEnvelope(track, i);
		std::list<TrackEnvelope*>::iterator it = find(laneEnvs.begin(), laneEnvs.end(), envelope);
		if (it != laneEnvs.end())
		{
			laneEnvs.erase(it);
			continue;
		}

		if (EnvVis(envelope, NULL))
			trackLaneEnvs.push_back(envelope);
	}

	int overlapLimit,trackGapTop, trackGapBottom;
	GetConfig("env_ol_minh", overlapLimit);
	GetTrackGap(trackHeight, &trackGapTop, &trackGapBottom);

	int envLaneFull = trackHeight - trackGapTop - trackGapBottom;
	int envLaneCount = (int)trackLaneEnvs.size();
	if (envLaneCount > 0)
	{
		bool envelopesOverlapping = (overlapLimit >= 0 && envLaneFull / envLaneCount < overlapLimit) ? (true) : (false);
		if (!envelopesOverlapping)
		{
			int envLaneH = envLaneFull / envLaneCount;
			int envHeight = envLaneH - 2*ENV_GAP;
			if (envHeight > ENV_LINE_WIDTH)
			{
				for (int i = 0; i < envLaneCount; ++i)
				{
					int envelopeStart = trackOffset + trackGapTop + envLaneH * i;
					int envelopeEnd = envelopeStart + envLaneH;
					if (mouseY >= envelopeStart && mouseY < envelopeEnd)
					{
						int envOffset = trackOffset + trackGapTop + i*envLaneH + ENV_GAP;
						BR_Envelope envelope(trackLaneEnvs[i]);

						mouseHit = IsMouseOverEnvelopeLine(envelope, envHeight, envOffset, mouseY, mouseX, mousePos, arrangeStart, arrangeEnd, arrangeZoom);
						if (mouseHit != 0)
							envelopeUnderMouse = envelope.GetPointer();
						break;
					}
				}
			}
		}
		else
		{
			int envHeight = envLaneFull - 2*ENV_GAP;
			if (envHeight > ENV_LINE_WIDTH)
			{
				for (int i = 0; i < envLaneCount; ++i)
				{
					int envOffset = trackOffset + trackGapTop + ENV_GAP;
					BR_Envelope envelope(trackLaneEnvs[i]);

					mouseHit = IsMouseOverEnvelopeLine(envelope, envHeight, envOffset, mouseY, mouseX, mousePos, arrangeStart, arrangeEnd, arrangeZoom);
					if (mouseHit != 0)
					{
						envelopeUnderMouse = envelope.GetPointer();
						break;
					}
				}
			}
		}
	}

	WritePtr(trackEnvelope, envelopeUnderMouse);
	return mouseHit;
}

static int IsMouseOverEnvelopeLineTake (MediaItem_Take* take, int takeHeight, int takeOffset, int mouseY, int mouseX, double mousePos, double arrangeStart, double arrangeEnd, double arrangeZoom, TrackEnvelope** trackEnvelope)
{
	/* MouseX is displayed X, not taking  scrollbars into account. MouseY on the *
	*  other hand has to take scrollbar position into account                    *
	*                                                                            *
	*  Return values: 0 -> no hit, 1 -> over point, 2 - > over segment           *
	*  If there is a hit, trackEnvelope will hold envelope                       */

	int mouseHit = 0;
	TrackEnvelope* envelopeUnderMouse = NULL;

	takeHeight -= 2*ENV_GAP;
	takeOffset += ENV_GAP;
	bool found = false;

	for (int i = 0; i < 4; ++i)
	{
		BR_EnvType type = VOLUME;    // gotcha: the order is important (it is the same reaper uses)
		if      (i == 1) type = PAN;
		else if (i == 2) type = MUTE;
		else if (i == 3) type = PITCH;

		BR_Envelope envelope (take, type);
		if (envelope.IsVisible())
			mouseHit = IsMouseOverEnvelopeLine(envelope, takeHeight, takeOffset, mouseY, mouseX, mousePos, arrangeStart, arrangeEnd, arrangeZoom);

		if (mouseHit != 0)
		{
			envelopeUnderMouse = envelope.GetPointer();
			break;
		}
	}

	WritePtr(trackEnvelope, envelopeUnderMouse);
	return mouseHit;
}

void GetMouseCursorContext (const char** window, const char** segment, const char** details, BR_MouseContextInfo* info)
{
	// Return values:
	/*********************************************************************
	| window       | segment       | details                             |
	|______________|_______________|_____________________________________|
	| unknown      | ""            | ""                                  |
	|______________|_______________|_____________________________________|
	| ruler        | regions       | ""                                  |
	|              | markers       | ""                                  |
	|              | tempo         | ""                                  |
	|              | timeline      | ""                                  |
	|______________|_______________|_____________________________________|
	| transport    | ""            | ""                                  |
	|______________|_______________|_____________________________________|
	| tcp          | track         | ""                                  |
	|              | envelope      | ""                                  |
	|              | empty         | ""                                  |
	|______________|_______________|_____________________________________|
	| mcp          | track         | ""                                  |
	|              | empty         | ""                                  |
	|______________|_______________|_____________________________________|
	| arrange      | track         | item, env_point, env_segment, empty |
	|              | envelope      | env_point, env_segment, empty       |
	|              | empty         | ""                                  |
	*********************************************************************/

	POINT p; GetCursorPos(&p);
	double arrangeStart, arrangeEnd, arrangeZoom;
	double mousePos = PositionAtArrangePoint(p, &arrangeStart, &arrangeEnd, &arrangeZoom);
	int mouseX = TranslatePointToArrangeX(p);
	HWND hwnd = WindowFromPoint(p);

	const char* returnWindow  = "unknown";
	const char* returnSegment = "";
	const char* returnDetails = "";
	BR_MouseContextInfo returnInfo;

	bool found = false;

	// Check ruler
	if (!found)
	{
		HWND ruler = GetRulerWndAlt();
		if (ruler == hwnd)
		{
			returnWindow = "ruler";
			ScreenToClient(ruler, &p);

			int limitL = 0;
			int limitH = 0;
			for (int i = 0; i < 4; ++i)
			{
				if (i == 0)      {limitL = limitH; limitH += RULER_WIDTH_REGIONS;  returnSegment = "regions"; }
				else if (i == 1) {limitL = limitH; limitH += RULER_WIDTH_MARKERS;  returnSegment = "marker";  }
				else if (i == 2) {limitL = limitH; limitH += RULER_WIDTH_TEMPO;    returnSegment = "tempo";   }
				else if (i == 3) {limitL = limitH; limitH += RULER_WIDTH_TIMELINE; returnSegment = "timeline";}

				if (p.y >= limitL && p.y <= limitH )
					break;
			}
			returnInfo.position = mousePos;
			found = true;
		}
	}

	// Check transport
	if (!found)
	{
		HWND transport = GetTransportWnd();
		if (transport == hwnd || transport == GetParent(hwnd)) // mouse may be over time status, child of transport
		{
			returnWindow = "transport";
			found = true;
		}
	}

	// Check MCP and TCP
	if (!found)
	{
		int context;
		if (returnInfo.track = HwndToTrack(hwnd, &context))
		{
			returnWindow = (context == 1) ? "tcp" : "mcp";
			returnSegment = "track";
			found = true;
		}
		else if (returnInfo.envelope = HwndToEnvelope(hwnd))
		{
			returnWindow = "tcp";
			returnSegment = "envelope";
			found = true;
		}
		else if (context != 0)
		{
			returnWindow = (context == 1) ? "tcp" : "mcp";
			returnSegment = "empty";
			found = true;
		}
	}

	// Check Arrange
	if (!found)
	{
		if (hwnd == GetArrangeWnd() && IsPointInArrange(&p, false))
		{
			returnWindow = "arrange";
			returnInfo.position = mousePos;

			int mouseY = TranslatePointToArrangeScrollY(p);
			list<TrackEnvelope*> laneEnvs;
			int height, offset;
			GetTrackOrEnvelopeFromY(mouseY, &returnInfo.envelope, &returnInfo.track, laneEnvs, &height, &offset);

			// Mouse cursor is in envelope lane
			if (returnInfo.envelope)
			{
				returnSegment = "envelope";

				BR_Envelope envelope(returnInfo.envelope);
				int trackEnvHit = IsMouseOverEnvelopeLine(envelope, height-2*ENV_GAP, offset+ENV_GAP, mouseY, mouseX, mousePos, arrangeStart, arrangeEnd, arrangeZoom);
				if (trackEnvHit == 1)
					returnDetails = "env_point";
				else if (trackEnvHit == 2)
					returnDetails = "env_segment";
				else
					returnDetails = "empty";

				returnInfo.track = NULL;
			}

			// Mouse cursor is in track lane
			else if (returnInfo.track)
			{
				returnSegment = "track";

				// Check envelope lane for track envelope, items and take envelopes
				int trackEnvHit = IsMouseOverEnvelopeLineTrackLane (returnInfo.track, height, offset, laneEnvs, mouseY, mouseX, mousePos, arrangeStart, arrangeEnd, arrangeZoom, &returnInfo.envelope);
				returnInfo.item = GetItemFromY(mouseY, mousePos, &returnInfo.take);
				int takeEnvHit = 0;
				if (trackEnvHit == 0 && returnInfo.take)
				{
					int takeOffset;
					int takeHeight = GetTakeHeight(returnInfo.take, NULL, 0, &takeOffset, true, height, offset);
					takeEnvHit = IsMouseOverEnvelopeLineTake(returnInfo.take, takeHeight, takeOffset, mouseY, mouseX, mousePos, arrangeStart, arrangeEnd, arrangeZoom, &returnInfo.envelope);
				}

				// Track envelope takes priority
				if (trackEnvHit != 0)
				{
					if (trackEnvHit == 1)
						returnDetails = "env_point";
					else if (trackEnvHit == 2)
						returnDetails = "env_segment";
				}
				// No track envelope found, check item and take envelopes
				else if (returnInfo.item)
				{
					if (takeEnvHit != 0)
					{
						if (takeEnvHit == 1)
							returnDetails = "env_point";
						else if (takeEnvHit == 2)
							returnDetails = "env_segment";
						returnInfo.takeEnvelope = true;
					}
					else
						returnDetails = "item";
				}
				else
					returnDetails = "empty";
			}
			else
				returnSegment = "empty";

			found = true;
		}
	}
	WritePtr(window, returnWindow);
	WritePtr(segment, returnSegment);
	WritePtr(details, returnDetails);
	WritePtr(info, returnInfo);
}

double PositionAtMouseCursor (bool checkRuler)
{
	POINT p; GetCursorPos(&p);
	if (IsPointInArrange(&p))
		return PositionAtArrangePoint(p);
	else if (checkRuler && GetRulerWnd() == WindowFromPoint(p))
		return PositionAtArrangePoint(p);
	else
		return -1;
}

MediaItem* ItemAtMouseCursor (double* position)
{
	POINT p; GetCursorPos(&p);
	if (IsPointInArrange(&p))
	{
		int y = TranslatePointToArrangeScrollY(p);
		double pos = PositionAtArrangePoint(p);

		WritePtr(position, pos);
		return GetItemFromY(y, pos, NULL);
	}
	WritePtr(position, -1.0);
	return NULL;
}

MediaItem_Take* TakeAtMouseCursor (double* position)
{
	POINT p; GetCursorPos(&p);
	if (IsPointInArrange(&p))
	{
		MediaItem_Take* take = NULL;
		int y = TranslatePointToArrangeScrollY(p);
		double pos = PositionAtArrangePoint(p);

		WritePtr(position, pos);
		GetItemFromY(y, pos, &take);
		return take;
	}
	WritePtr(position, -1.0);
	return NULL;
}

MediaTrack* TrackAtMouseCursor (int* context, double* position)
{
	POINT p; GetCursorPos(&p);
	HWND hwnd = WindowFromPoint(p);
	MediaTrack* track = NULL;
	int trackContext = -1;
	double cursorPosition = -1;

	// Check TCP/MCP
	int tcp;
	track = HwndToTrack(hwnd, &tcp);
	if (track)
		trackContext = (tcp == 1) ? (0) : (1);

	// Nothing in TCP/MCP, check arrange
	if (!track && hwnd == GetArrangeWnd() && IsPointInArrange(&p, false))
	{
		track = GetTrackFromY(TranslatePointToArrangeScrollY(p), NULL, NULL);
		if (track)
			trackContext = 2;
		cursorPosition = PositionAtArrangePoint(p);
	}

	WritePtr(context, trackContext);
	WritePtr(position, cursorPosition);
	return track;
}

/******************************************************************************
* Window                                                                      *
******************************************************************************/
static HWND SearchFloatingDockers (const char* name, const char* dockerName)
{
	HWND docker = FindWindowEx(NULL, NULL, NULL, dockerName);
	while(docker)
	{
		if (GetParent(docker) == g_hwndParent)
		{
			HWND insideDocker = FindWindowEx(docker, NULL, NULL, "REAPER_dock");
			while(insideDocker)
			{
				if (HWND w = FindWindowEx(insideDocker, NULL, NULL, name))
					return w;
				insideDocker = FindWindowEx(docker, insideDocker, NULL, "REAPER_dock");
			}
		}
		docker = FindWindowEx(NULL, docker, NULL, dockerName);
	}
	return NULL;
}

HWND FindInFloatingDockers (const char* name)
{
	#ifdef _WIN32
		HWND hwnd = SearchFloatingDockers(name, NULL);
	#else
		HWND hwnd = SearchFloatingDockers(name, __localizeFunc("Docker", "docker", 0));
		if (!hwnd)
		{
			WDL_FastString dockerName;
			dockerName.AppendFormatted(256, "%s%s", name, __localizeFunc(" (docked)", "docker", 0));
			hwnd = SearchFloatingDockers(name, dockerName.Get());
		}
		if (!hwnd)
			hwnd =SearchFloatingDockers(name, __localizeFunc("Toolbar Docker", "docker", 0));
	#endif

	return hwnd;
}

HWND FindInReaperDockers (const char* name)
{
	HWND docker = FindWindowEx(g_hwndParent, NULL, NULL, "REAPER_dock");
	while (docker)
	{
		if (HWND hwnd = FindWindowEx(docker, NULL, NULL , (LPCSTR)name))
			return hwnd;
		docker = FindWindowEx(g_hwndParent, docker, NULL, "REAPER_dock");
	}
	return NULL;
}

HWND FindFloating (const char* name)
{
	HWND hwnd = FindWindowEx(NULL, NULL, NULL, name);
	while (hwnd)
	{
		if (GetParent(hwnd) == g_hwndParent)
			return hwnd;
		hwnd = FindWindowEx(NULL, hwnd, NULL, name);
	}
	return NULL;
}

HWND FindInReaper (const char* name)
{
	return FindWindowEx(g_hwndParent, NULL, NULL , name);
}

HWND GetMixerWnd ()
{
	const char* name = __localizeFunc("Mixer", "DLG_151", 0);
	HWND hwnd = FindInReaperDockers(name);
	if (!hwnd) hwnd = FindFloating(name);
	if (!hwnd) hwnd = FindInFloatingDockers(name);
	if (!hwnd) hwnd = FindInReaper(name);
	return hwnd;
}

HWND GetMixerMasterWnd ()
{
	const char* name = __localizeFunc("Mixer Master", "mixer", 0);
	HWND hwnd = FindInReaperDockers(name);
	if (!hwnd) hwnd = FindFloating(name);
	if (!hwnd) hwnd = FindInFloatingDockers(name);
	if (!hwnd) hwnd = FindInReaper(name);
	return hwnd;
}

HWND GetArrangeWnd ()
{
	static HWND hwnd = GetTrackWnd();
	return hwnd;
}

HWND GetTransportWnd ()
{
	const char* name = __localizeFunc("Transport", "DLG_188", 0);
	HWND hwnd = FindInReaper(name);
	if (!hwnd) hwnd = FindInReaperDockers(name);
	if (!hwnd) hwnd = FindInFloatingDockers(name);
	if (!hwnd) hwnd = FindFloating(name); // transport can't float by itself but search in case this changes in the future
	return hwnd;
}

HWND GetRulerWndAlt ()
{
	static HWND hwnd = NULL;
	if (!hwnd) hwnd = GetRulerWnd();
	return hwnd;
}

HWND GetMcpWnd ()
{
	if (HWND mixer = GetMixerWnd())
	{
		HWND hwnd = FindWindowEx(mixer, NULL, NULL, NULL);
		while (hwnd)
		{
			if ((MediaTrack*)GetWindowLongPtr(hwnd, GWLP_USERDATA) != GetMasterTrack(NULL)) // skip master track
				return hwnd;
			hwnd = FindWindowEx(mixer, hwnd, NULL, NULL);
		}
	}
	return NULL;
}

HWND GetTcpWnd ()
{
	static HWND hwnd = NULL;
	if (!hwnd)
	{
		MediaTrack* track = GetTrack(NULL, 0);
		MediaTrack* master = GetMasterTrack(NULL);
		bool IsMasterVisible = TcpVis(master) ? true : false;

		// Can't find tcp if there are no tracks in project, kinda silly to expect no tracks in a DAW but oh well... :)
		MediaTrack* firstTrack = (IsMasterVisible) ? (master) : (track ? track : master);
		if (!track && !IsMasterVisible)
		{
			PreventUIRefresh(1);
			Main_OnCommandEx(40075, 0, NULL);
		}

		// TCP hierarchy: TCP owner -> TCP hwnd -> tracks
		bool found = false;
		HWND tcpOwner = FindWindowEx(g_hwndParent, NULL, NULL, "");
		while (tcpOwner && !found)
		{
			HWND tcp = FindWindowEx(tcpOwner, NULL, NULL, "");
			while (tcp && !found)
			{
				HWND track = FindWindowEx(tcp, NULL, NULL, "");
				while (track && !found)
				{
					if ((MediaTrack*)GetWindowLongPtr(track, GWLP_USERDATA) == firstTrack)
					{
						hwnd = tcp;
						found = true;
					}
					track = FindWindowEx(tcp, track, NULL, "");
				}
				tcp = FindWindowEx(tcpOwner, tcp, NULL, "");
			}
			tcpOwner = FindWindowEx(g_hwndParent, tcpOwner, NULL, "");
		}

		// Restore TCP master to previous state
		if (!track && !IsMasterVisible)
		{
			PreventUIRefresh(-1);
			Main_OnCommandEx(40075, 0, NULL);
		}
	}
	return hwnd;
}

HWND GetTcpTrackWnd (MediaTrack* track)
{
	HWND hwnd = GetWindow(GetTcpWnd(), GW_CHILD);
	do
	{
		if ((MediaTrack*)GetWindowLongPtr(hwnd, GWLP_USERDATA) == track)
			return hwnd;
	}
	while (hwnd = GetWindow(hwnd, GW_HWNDNEXT));

	return NULL;
}

MediaTrack* HwndToTrack (HWND hwnd, int* hwndContext)
{
	MediaTrack* track = NULL;
	HWND hwndParent = GetParent(hwnd);
	int context = 0;

	if (!track)
	{
		HWND tcp = GetTcpWnd();
		if (hwndParent == tcp)                                                             // hwnd is a track
			track = (MediaTrack*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		else if (GetParent(hwndParent) == tcp)                                             // hwnd is vu meter inside track
			track = (MediaTrack*)GetWindowLongPtr(hwndParent, GWLP_USERDATA);

		if (track)
			context = 1;
		else if (hwnd == tcp)
			context = 1;
	}

	if (!track)
	{
		HWND mcp = GetMcpWnd();
		HWND mixer = GetParent(mcp);
		HWND mixerMaster = GetMixerMasterWnd();
		HWND hwndPParent = GetParent(hwndParent);

		if (hwndParent == mcp || hwndParent == mixer || hwndParent == mixerMaster)         // hwnd is a track
			track = (MediaTrack*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		else if (hwndPParent == mcp || hwndPParent == mixer || hwndPParent == mixerMaster) // hwnd is vu meter inside track
			track = (MediaTrack*)GetWindowLongPtr(hwndParent, GWLP_USERDATA);

		if (track)
			context = 2;
		else if (hwnd == mcp || hwnd == mixerMaster)
			context = 2;
	}

	if (!ValidatePtr(track, "MediaTrack*"))
		track = NULL;

	WritePtr(hwndContext, context);
	return track;
}

TrackEnvelope* HwndToEnvelope (HWND hwnd)
{
	if (GetParent(hwnd) == GetTcpWnd())
	{
		TrackEnvelope* envelope = (TrackEnvelope*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (ValidatePtr(envelope, "TrackEnvelope*"))
			return envelope;
	}
	return NULL;
}

void CenterDialog (HWND hwnd, HWND target, HWND zOrder)
{
	RECT r; GetWindowRect(hwnd, &r);
	RECT r1; GetWindowRect(target, &r1);

	int hwndW = r.right - r.left;
	int hwndH = r.bottom - r.top;
	int targW = r1.right - r1.left;
	int targH = r1.bottom - r1.top;

	r.left = r1.left + (targW-hwndW)/2;
	r.top = r1.top + (targH-hwndH)/2;

	EnsureNotCompletelyOffscreen(&r);
	SetWindowPos(hwnd, zOrder, r.left, r.top, 0, 0, SWP_NOSIZE);
}

/******************************************************************************
* Theming                                                                     *
******************************************************************************/
void ThemeListViewOnInit (HWND list)
{
	if (SWS_THEMING)
	{
		SWS_ListView listView(list, NULL, 0, NULL, NULL, false, NULL, true);
		SNM_ThemeListView(&listView);
	}
}

bool ThemeListViewInProc (HWND hwnd, int uMsg, LPARAM lParam, HWND list, bool grid)
{
	if (SWS_THEMING)
	{
		int colors[LISTVIEW_COLORHOOK_STATESIZE];
		int columns = (grid) ? (Header_GetItemCount(ListView_GetHeader(list))) : (0);
		if (ListView_HookThemeColorsMessage(hwnd, uMsg, lParam, colors, GetWindowLong(list,GWL_ID), 0, columns))
			return true;
	}
	return false;
}