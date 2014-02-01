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

/******************************************************************************
* Constants                                                                   *
******************************************************************************/
const int LABEL_ABOVE_MIN_HEIGHT = 28;
const int TCP_MASTER_GAP         = 5;
const int ENV_GAP                = 4;  // bottom gap may seem like 3 when selected, but that
const int ENV_LINE_WIDTH         = 1;  // first pixel is used to "bold" selected envelope

const int TAKE_MIN_HEIGHT_COUNT = 10;
const int TAKE_MIN_HEIGHT_HIGH  = 12; // min height when take count <= TAKE_MIN_HEIGHT_COUNT
const int TAKE_MIN_HEIGHT_LOW   = 6;  // min height when take count >  TAKE_MIN_HEIGHT_COUNT

/******************************************************************************
* Miscellaneous                                                               *
******************************************************************************/
bool IsThisFraction (char* str, double& convertedFraction)
{
	replace(str, str+strlen(str), ',', '.' );
	char* buf = strstr(str,"/");

	if (!buf)
	{
		convertedFraction = atof(str);
		_snprintf(str, strlen(str)+1, "%g", convertedFraction);
		return false;
	}
	else
	{
		int num = atoi(str);
		int den = atoi(buf+1);
		_snprintf(str, strlen(str)+1, "%d/%d", num, den);
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
			// In certain corner cases involving regions, reaper will output the same marker more
			// than once. This should prevent those duplicate values from being written into the vector.
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
* Height                                                                      *
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
				// draw label inside item if it's making item too short (reaper version 4.59 and above)
				if (GetBit(label, 5) && trackHeight - labelH < LABEL_ABOVE_MIN_HEIGHT)
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

static int GetItemHeight (MediaItem* item, int* offsetY, int trackHeight)
{
	// Reaper takes into account FIPM and overlapping media item lanes
	// so no gotchas here besides track height checking since I_LASTH
	// might return wrong value when track height is 0
	if (trackHeight == 0)
		return 0;

	// supplied offsetY NEEDS to hold item's track offset
	WritePtr(offsetY, *offsetY + *(int*)GetSetMediaItemInfo(item, "I_LASTY", NULL));
	return *(int*)GetSetMediaItemInfo(item, "I_LASTH", NULL);
}

static int GetTakeHeight (MediaItem_Take* take, MediaItem* item, int id, int* offsetY, bool averagedLast)
{
	// Function is for internal use, so it isn't possible to have both take and item valid
	if (!take && !item)
	{
		WritePtr(offsetY, 0);
		return 0;
	}
	MediaItem_Take* validTake = (take) ? (take) : (GetTake(item, id));
	MediaItem* validItem = (item) ? (item) : (GetMediaItemTake_Item(take));
	MediaTrack* track = GetMediaItem_Track(item);

	// This gets us the offset to start of the item
	int trackH = GetTrackHeight (track, offsetY);
	int itemH = GetItemHeight(item, offsetY, trackH);

	int takeH;
	int takeLanes; GetConfig("projtakelane", takeLanes);

	// Take lanes displayed
	if (GetBit(takeLanes, 0))
	{
		int takeCount;
		int takeId = GetEffectiveTakeId(take ,item, id, &takeCount);
		takeH = itemH / takeCount; // average take height

		if (takeH < GetMinTakeHeight(track, takeCount, trackH, itemH))
		{
			if (GetActiveTake(validItem) == validTake)
				takeH = itemH;
			else
				takeH = 0;
		}
		else
		{
			if (takeId >= 0)
				WritePtr(offsetY, *offsetY + takeH * takeId);

			// Last take gets leftover pixels from integer division
			if (!averagedLast && (takeId == takeCount - 1))
			{
				int tmp = itemH - takeH * takeId; // if last take is too tall, Reaper
				if (tmp < (int)(takeH * 1.5))     // crops it to height of other takes
					takeH = tmp;
			}
		}
	}
	// Take lanes not displayed
	else
	{
		if (GetActiveTake(item) == validTake)
			takeH = itemH;
		else
			takeH = 0;
	}

	return takeH;
}

static MediaTrack* GetTrackFromY (int y, int* trackHeight, int* offset)
{
	MediaTrack* master = GetMasterTrack(NULL);
	MediaTrack* track = NULL;

	int trackEnd = 0;
	int trackOffset = 0;
	for (int i = 0; i <= GetNumTracks(); ++i)
	{
		MediaTrack* currentTrack = CSurf_TrackFromID(i, false);
		int height = *(int*)GetSetMediaTrackInfo(currentTrack, "I_WNDH", NULL);
		if (currentTrack == master && TcpVis(master)) height += TCP_MASTER_GAP;
		trackEnd += height;

		if (y >= trackOffset && y < trackEnd)
		{
			track = currentTrack;
			break;
		}
		trackOffset += height;
	}

	int trackH = 0;
	if (track)
	{
		trackH = GetTrackHeight(track, NULL);

		if (y < trackOffset || y >= trackOffset + trackH)
			track = NULL;
	}

	WritePtr(trackHeight, (track) ? (trackH) : (0));
	WritePtr(offset, (track) ? (trackOffset) : (0));
	return track;
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

	int compact = GetEffectiveCompactLevel(track); // Track can't get resized and "I_HEIGHTOVERRIDE" could return wrong
	if (compact == 2)                              // value if track was resized prior to compacting so return here
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

	if (take) WritePtr(offsetY, *offsetY + ENV_GAP);
	return envelopeH;
}

int GetTakeEnvHeight (MediaItem* item, int id, int* offsetY)
{
	// Last take may have different height then the others, but envelope is always positioned based
	// on take height of other takes so we get average take height (relevant for last take only)
	int envelopeH = GetTakeHeight(NULL, item, id, offsetY, true) - 2*ENV_GAP;
	envelopeH = (envelopeH > ENV_LINE_WIDTH) ? envelopeH : 0;

	if (item) WritePtr(offsetY, *offsetY + ENV_GAP);
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

	// Get track's height and offset
	int trackOffset = 0;
	int trackHeight = GetTrackHeight(track, (offsetY) ? (&trackOffset) : (NULL));

	// Prepare return variables
	int envOffset = 0;
	int envHeight = 0;

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
			// In certain cases, envelope hwnds (got by GW_HWNDNEXT)
			// won't be in order they are drawn so make sure requested
			// envelope is drawn under envelope currently in loop
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

				// Envelope was already encountered, no need to check chunk (expensive!) so just skip it
				if (it != checkedEnvs.end())
				{
					checkedEnvs.erase(it);
				}
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
* Arrange                                                                     *
******************************************************************************/
static bool IsPointInArrange (POINT* p)
{
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
	return p->x >= r.left && p->x <= r.right && p->y >= r.top && p->y <= r.bottom;
}

static double PositionAtArrangePoint (POINT p)
{
	HWND hwnd = GetArrangeWnd();
	ScreenToClient(hwnd, &p);

	double arrangeStart, arrangeEnd;
	RECT r; GetWindowRect(hwnd, &r);
	GetSet_ArrangeView2(NULL, false, r.left, r.right-VERT_SCROLL_W, &arrangeStart, &arrangeEnd);

	GetClientRect(hwnd, &r);
	return arrangeStart  + (r.left + p.x) / GetHZoomLevel();
}

static int TranslatePointToArrangeY (POINT p)
{
	HWND hwnd = GetArrangeWnd();
	ScreenToClient(hwnd, &p);

	SCROLLINFO si = { sizeof(SCROLLINFO), };
	si.fMask = SIF_ALL;
	CoolSB_GetScrollInfo(hwnd, SB_VERT, &si);

	return (int)p.y + si.nPos;
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

bool PositionAtMouseCursor (double* position)
{
	POINT p; GetCursorPos(&p);
	if (IsPointInArrange(&p))
	{
		WritePtr(position, PositionAtArrangePoint(p));
		return true;
	}
	else
	{
		WritePtr(position, -1.0);
		return false;
	}
}

MediaItem* ItemAtMouseCursor (double* position)
{
	POINT p; GetCursorPos(&p);
	if (!IsPointInArrange(&p))
		return NULL;

	int cursorY = TranslatePointToArrangeY(p);
	double cursorPosition = PositionAtArrangePoint(p);

	int trackH, offset;
	MediaTrack* track = GetTrackFromY(cursorY, &trackH, &offset);
	MediaItem* item = NULL;

	double iStartLast = GetMediaItemInfo_Value(GetTrackMediaItem(track, 0), "D_POSITION");
	int count = CountTrackMediaItems(track);
	for (int i = 0; i < count ; ++i)
	{
		MediaItem* currentItem = GetTrackMediaItem(track, i);
		double iStart = GetMediaItemInfo_Value(currentItem, "D_POSITION");
		double iEnd = GetMediaItemInfo_Value(currentItem, "D_LENGTH") + iStart;

		if (cursorPosition >= iStart && cursorPosition <= iEnd)
		{
			if (iStart >= iStartLast)
			{
				int yStart = offset;                                             // due to FIMP/overlapping items in lanes, we check every
				int yEnd = GetItemHeight(currentItem, &yStart, trackH) + yStart; // item - the last one that is closest to cursor and whose
				if (cursorY > yStart && cursorY < yEnd)                          // height overlaps with cursor Y position is the correct one
					item = currentItem;
			}
			iStartLast = iStart;

		}
	}

	WritePtr(position, (item) ? (cursorPosition) : (-1));
	return item;
}

/******************************************************************************
* Window                                                                      *
******************************************************************************/
struct HwndTrackPair
{
	HWND hwnd;
	MediaTrack* track;
};

static BOOL CALLBACK FindTcp (HWND hwnd, LPARAM lParam)
{
	HwndTrackPair* param = (HwndTrackPair*)lParam;

	if ((MediaTrack*)GetWindowLongPtr(hwnd, GWLP_USERDATA) == param->track)
	{
		param->hwnd = GetParent(hwnd);
		return FALSE;
	}
	return TRUE;
}

HWND GetTcpWnd ()
{
	static HWND hwnd = NULL;

	if (!hwnd)
	{
		MediaTrack* track = GetTrack(NULL, 0);
		HwndTrackPair param = {NULL, track ? track : GetMasterTrack(NULL)};

		EnumChildWindows(g_hwndParent, FindTcp, (LPARAM)&param);
		hwnd = param.hwnd;
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

HWND GetArrangeWnd ()
{
	static HWND hwnd = GetTrackWnd();
	return hwnd;
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

/******************************************************************************
* ReaScript                                                                   *
******************************************************************************/
bool BR_SetTakeSourceFromFile(MediaItem_Take* take, const char* filename, bool inProjectData)
{
	if (take && file_exists(filename))
	{
		if (PCM_source* oldSource = (PCM_source*)GetSetMediaItemTakeInfo(take,"P_SOURCE",NULL))
		{
			PCM_source* newSource = PCM_Source_CreateFromFileEx(filename, !inProjectData);
			GetSetMediaItemTakeInfo(take,"P_SOURCE", newSource);
			delete oldSource;
			return true;
		}
	}
	return false;
}