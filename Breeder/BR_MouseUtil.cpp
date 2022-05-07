/******************************************************************************
/ BR_MouseUtil.cpp
/
/ Copyright (c) 2014-2015 Dominik Martin Drzic
/ http://forum.cockos.com/member.php?u=27094
/ http://github.com/reaper-oss/sws
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

#include "BR_MouseUtil.h"
#include "BR_EnvelopeUtil.h"
#include "BR_MidiUtil.h"
#include "BR_Util.h"
#include "cfillion/cfillion.hpp" // CF_GetScrollInfo

/******************************************************************************
* Constants                                                                   *
******************************************************************************/
// const int ITEM_LABEL_MIN_HEIGHT     = 28; NF: not referenced currently

const int ENV_GAP                   = 4; // bottom gap may seem like 3 when selected, but that
const int ENV_LINE_WIDTH            = 1; // first pixel is used to "bold" selected envelope
const int ENV_HIT_POINT             = 5;
const int ENV_HIT_POINT_LEFT        = 6; // envelope point doesn't always have middle pixel so hit point is different for one side
const int ENV_HIT_POINT_DOWN        = 6; // +1 because lower part is tracked starting 1 pixel below line (so when envelope is active, hit points appear the same)

const int STRETCH_M_HIT_POINT       = 6;
const int STRETCH_M_MIN_TAKE_HEIGHT = 8;

// Not tied to Reaper, purely for readability
const int MIDI_WND_NOTEVIEW     = 1;
const int MIDI_WND_KEYBOARD     = 2;
const int MIDI_WND_UNKNOWN      = 3;

/******************************************************************************
* Helper functions                                                            *
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
			height += GetMasterTcpGap();
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

static MediaItem_Take* GetTakeFromItemY (MediaItem* item, int itemY, MediaTrack* track, int trackHeight, int* takeId)
{
	/* Does not check if Y is within the item, caller should handle that */

	if (!CountTakes(item))
		return NULL;

	MediaItem_Take* take = NULL;
	WritePtr(takeId, -1);

	const int takeLanes = ConfigVar<int>("projtakelane").value_or(0);

	// Take lanes displayed
	if (GetBit(takeLanes, 0))
	{
		int itemH = GetItemHeight(item, NULL, trackHeight, 0);
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
				WritePtr(takeId, id);

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
							WritePtr(takeId, i);

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
				{
					WritePtr(takeId, -1);
					take = NULL;
				}
		}
	}
	// Take lanes not displayed
	else
	{
		take = GetActiveTake(item);
		WritePtr(takeId, (int)GetMediaItemInfo_Value(item, "I_CURTAKE"));
	}

	return take;
}

static MediaItem* GetItemFromY (int y, double position, MediaItem_Take** take, int* takeId)
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
				int yStart = offset;                                                     // due to FIMP/overlapping items in lanes, check every
				int yEnd = GetItemHeight(currentItem, &yStart, trackH, yStart) + yStart; // item - last one closest to cursor and whose height
				if (y > yStart && y < yEnd)                                              // overlaps with cursor Y position is the correct one
				{
					item = currentItem;
					itemYStart = yStart;
				}
			}
			iStartLast = iStart;
		}
	}

	WritePtr(take, GetTakeFromItemY(item, y-itemYStart, track, trackH, takeId));
	return item;
}

static bool IsPointInArrange (const POINT& p, bool checkPointVisibilty = true, HWND* wndFromPoint = NULL)
{
	/* Check if point is in arrange. Since WindowFromPoint is sometimes     *
	*  called before this function, caller can choose not to check it but   *
	*  only if point's coordinates are in arrange (disregarding scrollbars) */

	HWND hwnd = GetArrangeWnd();
	RECT r; GetWindowRect(hwnd, &r);
	#ifdef _WIN32
		r.right  -= SCROLLBAR_W + 1;
		r.bottom -= SCROLLBAR_W + 1;
	#else
		r.right  -= SCROLLBAR_W;
		if (r.top > r.bottom)
		{
			swap(r.top, r.bottom);
			r.top += SCROLLBAR_W;
		}
		else
			r.bottom -= SCROLLBAR_W;
	#endif

	bool pointInArrange = p.x >= r.left && p.x <= r.right && p.y >= r.top && p.y <= r.bottom;

	if (checkPointVisibilty)
	{
		if (pointInArrange)
		{
			HWND hwndPt = WindowFromPoint(p);
			WritePtr(wndFromPoint, hwndPt);
			if (hwnd == hwndPt)
				return true;
			else
				return false;
		}
		else
		{
			WritePtr(wndFromPoint, WindowFromPoint(p));
			return false;
		}
	}
	else
	{
		WritePtr(wndFromPoint, WindowFromPoint(p));
		return pointInArrange;
	}
}

static double PositionAtArrangePoint (POINT p, double* arrangeStart = NULL, double* arrangeEnd = NULL, double* hZoom = NULL)
{
	/* Does not check if point is in arrange, to make it work, it *
	*  is enough to have p.x in arrange, p.y can be somewhere     *
	*  else like ruler etc...                                     */

	HWND hwnd = GetArrangeWnd();
	ScreenToClient(hwnd, &p);

	double start, end;
	RECT r; GetWindowRect(hwnd, &r);
	GetSet_ArrangeView2(NULL, false, r.left, r.right-SCROLLBAR_W, &start, &end);
	GetClientRect(hwnd, &r);
	double zoom = GetHZoomLevel();

	WritePtr(arrangeStart, start);
	WritePtr(arrangeEnd, end);
	WritePtr(hZoom, zoom);
	return start  + (r.left + p.x) / zoom;
}

static int TranslatePointToArrangeScrollY (POINT p)
{
	HWND hwnd = GetArrangeWnd();
	ScreenToClient(hwnd, &p);

	SCROLLINFO si = { sizeof(SCROLLINFO), SIF_POS };
	CF_GetScrollInfo(hwnd, SB_VERT, &si);

	return (int)p.y + si.nPos;
}

static int TranslatePointToArrangeDisplayX (POINT p)
{
	HWND hwnd = GetArrangeWnd();
	ScreenToClient(hwnd, &p);
	return (int)p.x;
}

static int FindXOnSegment(int x1, int y1, int x2, int y2, int y)
{
	/* stolen from WDL's lice_line.cpp */
	if (y1 > y2)
	{
		swap(x1, x2);
		swap(y1, y2);
	}

	if (y <= y1) return x1;
	if (y >= y2) return x2;
	return x1 + (int)((float)(y - y1) * ((float)(x2 - x1) / (float)(y2 - y1)));
}

/******************************************************************************
* Miscellaneous                                                               *
******************************************************************************/
double PositionAtMouseCursor (bool checkRuler, bool checkCursorVisibility /*=true*/, int* yOffset /*=NULL*/, bool* overRuler /*= NULL*/)
{
	POINT p; HWND hwnd;
	GetCursorPos(&p);
	if (IsPointInArrange(p, checkCursorVisibility, &hwnd))
	{
		WritePtr(yOffset, TranslatePointToArrangeScrollY(p));
		WritePtr(overRuler, false);
		return PositionAtArrangePoint(p);
	}
	else if (checkRuler && GetRulerWndAlt() == hwnd)
	{
		WritePtr(overRuler, true);
		double position = PositionAtArrangePoint(p);
		if (yOffset)
		{
			ScreenToClient(hwnd, &p);
			*yOffset = (int)p.y;
		}
		return position;
	}
	else
	{
		WritePtr(yOffset, -1);
		WritePtr(overRuler, false);
		return -1;
	}
}

MediaItem* ItemAtMouseCursor (double* position, MediaItem_Take** takeAtMouse /*= NULL*/)
{
	POINT p; GetCursorPos(&p);
	if (IsPointInArrange(p))
	{
		int y = TranslatePointToArrangeScrollY(p);
		double pos = PositionAtArrangePoint(p);

		WritePtr(position, pos);
		return GetItemFromY(y, pos, takeAtMouse, NULL);
	}
	WritePtr(position, -1.0);
	return NULL;
}

MediaItem_Take* TakeAtMouseCursor (double* position)
{
	POINT p; GetCursorPos(&p);
	if (IsPointInArrange(p))
	{
		MediaItem_Take* take = NULL;
		int y = TranslatePointToArrangeScrollY(p);
		double pos = PositionAtArrangePoint(p);

		WritePtr(position, pos);
		GetItemFromY(y, pos, &take, NULL);
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
	track = HwndToTrack(hwnd, &tcp, p);
	if (track)
		trackContext = (tcp == 1) ? (0) : (1);

	// Nothing in TCP/MCP, check arrange
	if (!track && hwnd == GetArrangeWnd() && IsPointInArrange(p, false))
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
* BR_MouseInfo                                                                *
******************************************************************************/
BR_MouseInfo::BR_MouseInfo (int mode /*= BR_MouseInfo::MODE_ALL*/, bool updateNow /*= true*/) :
m_midiEditorPianoWnd (NULL)
{
	m_mode = mode;
	if (updateNow)
		this->Update();
}

BR_MouseInfo::~BR_MouseInfo ()
{
}

void BR_MouseInfo::Update (const POINT* p /*=NULL*/)
{
	POINT currentPoint;
	if (p)
		currentPoint = *p;
	else
		GetCursorPos(&currentPoint);

	m_midiEditorPianoWnd = NULL;
	this->GetContext(currentPoint);
}

void BR_MouseInfo::SetMode (int mode)
{
	m_mode = mode;
}

const char* BR_MouseInfo::GetWindow ()
{
	return m_mouseInfo.window;
}

const char* BR_MouseInfo::GetSegment ()
{
	return m_mouseInfo.segment;
}

const char* BR_MouseInfo::GetDetails ()
{
	return m_mouseInfo.details;
}

MediaTrack* BR_MouseInfo::GetTrack ()
{
	return m_mouseInfo.track;
}

MediaItem* BR_MouseInfo::GetItem ()
{
	return m_mouseInfo.item;
}

MediaItem_Take* BR_MouseInfo::GetTake()
{
	return m_mouseInfo.take;
}

TrackEnvelope* BR_MouseInfo::GetEnvelope ()
{
	return m_mouseInfo.envelope;
}

int BR_MouseInfo::GetTakeId ()
{
	return m_mouseInfo.takeId;
}

int BR_MouseInfo::GetEnvelopePoint ()
{
	return m_mouseInfo.envPointId;
}

int BR_MouseInfo::GetStretchMarker ()
{
	return m_mouseInfo.stretchMarkerId;
}

bool BR_MouseInfo::IsTakeEnvelope ()
{
	return m_mouseInfo.takeEnvelope;
}

HWND BR_MouseInfo::GetMidiEditor ()
{
	return m_mouseInfo.midiEditor;
}

bool BR_MouseInfo::IsInlineMidi ()
{
	return m_mouseInfo.inlineMidi;
}

bool BR_MouseInfo::GetCCLane (int* ccLane, int* ccLaneVal, int* ccLaneId)
{
	if (m_mouseInfo.ccLaneId == -1)
	{
		WritePtr(ccLane,    -2);
		WritePtr(ccLaneVal, -1);
		WritePtr(ccLaneId,  -1);
		return false;
	}
	else
	{
		WritePtr(ccLane,    m_mouseInfo.ccLane);
		WritePtr(ccLaneId,  m_mouseInfo.ccLaneId);
		WritePtr(ccLaneVal, m_mouseInfo.ccLaneVal);
		return true;
	}
}

int BR_MouseInfo::GetNoteRow ()
{
	return m_mouseInfo.noteRow;
}

double BR_MouseInfo::GetPosition ()
{
	return m_mouseInfo.position;
}

int BR_MouseInfo::GetPianoRollMode ()
{
	return m_mouseInfo.pianoRollMode;
}

bool BR_MouseInfo::SetDetectedCCLaneAsLastClicked ()
{
	bool update = false;
	if (m_mouseInfo.ccLaneId != -1)
	{
		if (m_mouseInfo.midiEditor || m_mouseInfo.inlineMidi)
		{
			POINT point = m_ccLaneClickPoint;

			const int lockSettings = ConfigVar<int>("projsellock").value_or(0);
			char itemLock = m_mouseInfo.inlineMidi ? (char)GetMediaItemInfo_Value(GetMediaItemTake_Item(m_mouseInfo.take), "C_LOCK") : 0;

			// Get HWND and update point if needed
			HWND hwnd = NULL;
			if (m_mouseInfo.inlineMidi)
			{
				hwnd = GetArrangeWnd();
				SCROLLINFO si = { sizeof(SCROLLINFO), };
				si.fMask = SIF_ALL;
				CF_GetScrollInfo(hwnd, SB_VERT, &si);
				point.y -= si.nPos;

				CoolSB_GetScrollInfo(hwnd, SB_HORZ, &si);
				double hZoom = GetHZoomLevel();

				double itemStart = GetMediaItemInfo_Value(m_mouseInfo.item, "D_POSITION");
				double itemEnd   = itemStart + GetMediaItemInfo_Value(m_mouseInfo.item, "D_LENGTH");
				int itemStartPx = RoundToInt(itemStart * hZoom) - si.nPos; if (itemStartPx < 0)                         itemStartPx = 0;
				int itemEndPx   = RoundToInt(itemEnd   * hZoom) - si.nPos; if (itemEndPx   > (int)(si.nPos + si.nPage)) itemEndPx   = si.nPos + si.nPage;

				// REAPER gives priority to what's inside rather than the edges (so if item's too short, edge hit-points won't stretch inside the item)
				if (!CheckBounds((int)((itemStartPx + itemEndPx) / 2), itemStartPx, itemEndPx))
				{
					hwnd = NULL;
				}
				else
				{
					ConfigVar<int>("projsellock").try_set(23492); // lock item edges, fades, volume handles, stretch markers, item movement, take and track envelopes
					SetMediaItemInfo_Value(GetMediaItemTake_Item(m_mouseInfo.take), "C_LOCK", 0);
				}
			}
			else
			{
				hwnd = m_midiEditorPianoWnd ? m_midiEditorPianoWnd : GetPianoView(m_mouseInfo.midiEditor);
			}

			// Simulate mouse click
			if (hwnd)
			{
				SimulateMouseClick(hwnd, point, true);
				if (m_mouseInfo.inlineMidi)
				{
					ConfigVar<int>("projsellock").try_set(lockSettings);
					SetMediaItemInfo_Value(GetMediaItemTake_Item(m_mouseInfo.take), "C_LOCK", (double)itemLock);
				}
				update = true;
			}
		}
	}
	return update;
}

BR_MouseInfo::MouseInfo::MouseInfo () :
window          ("unknown"),
segment         (""),
details         (""),
track           (NULL),
item            (NULL),
take            (NULL),
envelope        (NULL),
midiEditor      (NULL),
takeEnvelope    (false),
inlineMidi      (false),
position        (-1),
takeId          (-1),
envPointId      (-1),
stretchMarkerId (-1),
noteRow         (-1),
ccLaneVal       (-1),
ccLaneId        (-1),
ccLane          (-2), // because -1 stands for velocity lane
pianoRollMode   (-1)
{
	/* Note: other parts of the code rely on these *
	*  default values. Be careful if changing them */
}

void BR_MouseInfo::GetContext (const POINT& p)
{
	HWND hwnd = WindowFromPoint(p);

	BR_MouseInfo::MouseInfo mouseInfo;
	if (hwnd)
	{
		double arrangeStart, arrangeEnd, arrangeZoom;
		double mousePos      = PositionAtArrangePoint(p, &arrangeStart, &arrangeEnd, &arrangeZoom);
		int    mouseDisplayX = TranslatePointToArrangeDisplayX(p);

		bool found = false;

		// Ruler
		if (!found && ((m_mode & BR_MouseInfo::MODE_ALL) || (m_mode & BR_MouseInfo::MODE_RULER)))
		{
			HWND ruler = GetRulerWndAlt();
			if (ruler == hwnd)
			{
				mouseInfo.window = "ruler";

				POINT rulerP = p; ScreenToClient(ruler, &rulerP);
				RECT r; GetClientRect(ruler, &r);

				int rulerH = r.bottom-r.top;
				int limitL = 0;
				int limitH = 0;
				for (int i = 0; i < 4; ++i)
				{
					if      (i == 0) {limitL = limitH; limitH += this->GetRulerLaneHeight(rulerH, i); mouseInfo.segment = "region_lane";}
					else if (i == 1) {limitL = limitH; limitH += this->GetRulerLaneHeight(rulerH, i); mouseInfo.segment = "marker_lane";}
					else if (i == 2) {limitL = limitH; limitH += this->GetRulerLaneHeight(rulerH, i); mouseInfo.segment = "tempo_lane"; }
					else if (i == 3) {limitL = limitH; limitH += this->GetRulerLaneHeight(rulerH, i); mouseInfo.segment = "timeline";   }

					if (rulerP.y >= limitL && rulerP.y < limitH )
						break;
				}
				mouseInfo.position = mousePos;
				found = true;
			}
		}

		// Transport
		if (!found && ((m_mode & BR_MouseInfo::MODE_ALL) || (m_mode & BR_MouseInfo::MODE_TRANSPORT)))
		{
			HWND transport = GetTransportWnd();
			if (transport == hwnd || transport == GetParent(hwnd)) // mouse may be over time status, child of transport
			{
				mouseInfo.window = "transport";
				found = true;
			}
		}

		// MCP and TCP
		if (!found && ((m_mode & BR_MouseInfo::MODE_ALL) || (m_mode & BR_MouseInfo::MODE_MCP_TCP)))
		{
			int context;
			if ((mouseInfo.track = HwndToTrack(hwnd, &context, p)))
			{
				mouseInfo.window = (context == 1) ? "tcp" : "mcp";
				mouseInfo.segment = "track";
				found = true;
			}
			else if ((mouseInfo.envelope = HwndToEnvelope(hwnd, p)))
			{
				mouseInfo.window = "tcp";
				mouseInfo.segment = "envelope";
				found = true;
			}
			else if (context != 0)
			{
				mouseInfo.window = (context == 1) ? "tcp" : "mcp";
				mouseInfo.segment = "empty";
				found = true;
			}
		}

		// Arrange
		if (!found && ((m_mode & BR_MouseInfo::MODE_ALL) || (m_mode & BR_MouseInfo::MODE_ARRANGE) || (m_mode & BR_MouseInfo::MODE_MIDI_INLINE)))
		{
			if (hwnd == GetArrangeWnd() && IsPointInArrange(p, false))
			{
				int mouseY = TranslatePointToArrangeScrollY(p);
				list<TrackEnvelope*> laneEnvs;
				int height, offset;
				this->GetTrackOrEnvelopeFromY(mouseY, &mouseInfo.envelope, &mouseInfo.track, ((m_mode & BR_MouseInfo::MODE_ALL) || (m_mode & BR_MouseInfo::MODE_ARRANGE)) ? &laneEnvs : NULL, &height, &offset);

				if ((m_mode & BR_MouseInfo::MODE_ALL) || (m_mode & BR_MouseInfo::MODE_ARRANGE))
				{
					mouseInfo.window   = "arrange";
					mouseInfo.position = mousePos;

					// Mouse cursor is in envelope lane
					if (mouseInfo.envelope)
					{
						mouseInfo.segment = "envelope";

						int trackEnvHit = 0;
						if (!(m_mode & BR_MouseInfo::MODE_IGNORE_ENVELOPE_LANE_SEGMENT))
						{
							BR_Envelope envelope(mouseInfo.envelope);
							trackEnvHit = this->IsMouseOverEnvelopeLine(envelope, height-2*ENV_GAP, offset+ENV_GAP, mouseDisplayX, mouseY, mousePos, arrangeStart, arrangeZoom, &mouseInfo.envPointId);
						}

						if      (trackEnvHit == 1) mouseInfo.details = "env_point";
						else if (trackEnvHit == 2) mouseInfo.details = "env_segment";
						else                       mouseInfo.details = "empty";
					}

					// Mouse cursor is in track lane
					else if (mouseInfo.track)
					{
						mouseInfo.segment = "track";
						mouseInfo.item = GetItemFromY(mouseY, mousePos, &mouseInfo.take, &mouseInfo.takeId);

						int trackEnvHit      = 0;
						int takeEnvHit       = 0;
						int stretchMarkerHit = -1;
						if (!(m_mode & BR_MouseInfo::MODE_IGNORE_ALL_TRACK_LANE_ELEMENTS_BUT_ITEMS))
						{
							// Check track lane for track envelope
							MediaItem_Take* activeTake = GetActiveTake(mouseInfo.item);
							trackEnvHit = (IsLocked(TRACK_ENV)) ? 0 : this->IsMouseOverEnvelopeLineTrackLane(mouseInfo.track, height, offset, laneEnvs, mouseDisplayX, mouseY, mousePos, arrangeStart, arrangeZoom, &mouseInfo.envelope, &mouseInfo.envPointId);

							// Check track lane for take envelope (only if take is active - REAPER doesn't allow editing of envelopes of inactive takes)
							int takeHeight = -666;
							int takeOffset = -666;
							if (trackEnvHit == 0 && mouseInfo.take && activeTake == mouseInfo.take && !IsLocked(TAKE_ENV))
							{
								if (takeHeight == -666)
									takeHeight = GetTakeHeight(mouseInfo.take, NULL, 0, &takeOffset, true, height, offset);
								takeEnvHit = this->IsMouseOverEnvelopeLineTake(mouseInfo.take, takeHeight, takeOffset, mouseDisplayX, mouseY, mousePos, arrangeStart, arrangeZoom, &mouseInfo.envelope, &mouseInfo.envPointId);
							}

							// Check for stretch markers (again, only active take)
							stretchMarkerHit = -1;
							if (trackEnvHit == 0 && takeEnvHit == 0 && mouseInfo.take && activeTake == mouseInfo.take && !IsLocked(STRETCH_MARKERS) && !IsItemLocked(mouseInfo.item) && !IsLocked(ITEM_FULL))
							{
								if (takeHeight == -666)
									takeHeight = GetTakeHeight(mouseInfo.take, NULL, 0, &takeOffset, true, height, offset);
								stretchMarkerHit = this->IsMouseOverStretchMarker(mouseInfo.item, mouseInfo.take, takeHeight, takeOffset, mouseDisplayX, mouseY, mousePos, arrangeStart, arrangeZoom);
							}
						}

						// Track envelope takes priority
						if (trackEnvHit != 0)
						{
							if      (trackEnvHit == 1) mouseInfo.details = "env_point";
							else if (trackEnvHit == 2) mouseInfo.details = "env_segment";
						}
						// Item and things inside it
						else if (mouseInfo.item)
						{
							// Take envelope takes priority
							if (takeEnvHit != 0)
							{
								if      (takeEnvHit == 1) mouseInfo.details = "env_point";
								else if (takeEnvHit == 2) mouseInfo.details = "env_segment";
								mouseInfo.takeEnvelope = true;
							}
							// Stretch marker next
							else if (stretchMarkerHit != -1)
							{
								mouseInfo.details = "item_stretch_marker";
								mouseInfo.stretchMarkerId = stretchMarkerHit;
							}
							// And finally item itself
							else
							{
								if (IsOpenInInlineEditor(mouseInfo.take))
								{
									int takeOffset;
									int takeHeight = GetTakeHeight(mouseInfo.take, NULL, mouseInfo.takeId, &takeOffset, false, height, offset);
									if (!this->GetContextMIDIInline(mouseInfo, mouseDisplayX, mouseY, takeHeight, takeOffset))
										mouseInfo.details = "item";
								}
								else
								{
									mouseInfo.details = "item";
								}
							}
						}
						// No items, no envelopes, no nothing
						else
						{
							mouseInfo.details = "empty";
						}
					}

					// Mouse cursor is in empty arrange space
					else
						mouseInfo.segment = "empty";
				}
				else
				{
					mouseInfo.envelope = NULL; // in case GetTrackOrEnvelopeFromY() found something
					GetItemFromY(mouseY, mousePos, &mouseInfo.take, nullptr);
					if (mouseInfo.track && IsOpenInInlineEditor(mouseInfo.take))
					{
						int takeOffset;
						int takeHeight = GetTakeHeight(mouseInfo.take, NULL, mouseInfo.takeId, &takeOffset, false, height, offset);
						if (!this->GetContextMIDIInline(mouseInfo, mouseDisplayX, mouseY, takeHeight, takeOffset))
						{
							mouseInfo.window  = "arrange";
							mouseInfo.segment = "track";
							mouseInfo.details = "item";
							mouseInfo.position = mousePos;
						}
					}
				}

				found = true;
			}
		}

		// MIDI editor
		if (!found && ((m_mode & BR_MouseInfo::MODE_ALL) || (m_mode & BR_MouseInfo::MODE_MIDI_EDITOR)))
			this->GetContextMIDI(p, hwnd, mouseInfo);
	}

	m_mouseInfo = mouseInfo;
}

bool BR_MouseInfo::GetContextMIDI (POINT p, HWND hwnd, BR_MouseInfo::MouseInfo& mouseInfo)
{
	HWND segmentHwnd = NULL;
	if (int cursorSegment = this->IsHwndMidiEditor(hwnd, &mouseInfo.midiEditor, &segmentHwnd))
	{
		BR_MidiEditor midiEditor(mouseInfo.midiEditor);
		if (!midiEditor.IsValid())
			return false;

		mouseInfo.window = "midi_editor";
		ScreenToClient(segmentHwnd, &p);
		RECT r; GetClientRect(segmentHwnd, &r);

		// ignoring extra flags from GetPianoRoll() (eg. custom note order mode=0x10000)
		mouseInfo.pianoRollMode = midiEditor.GetPianoRoll() & 0xffff;

		// Make sure mouse is really in note editing area
		if (cursorSegment == MIDI_WND_NOTEVIEW)
		{
			if      (p.x > (r.right - r.left - 1)) cursorSegment = MIDI_WND_UNKNOWN;
			else if (p.y > (r.bottom - r.top - 1)) cursorSegment = MIDI_WND_UNKNOWN;
		}

		if (cursorSegment == MIDI_WND_NOTEVIEW || cursorSegment == MIDI_WND_KEYBOARD)
		{
			// Get mouse cursor time position
			if (cursorSegment == MIDI_WND_NOTEVIEW)
			{
				if (midiEditor.GetTimebase() == PROJECT_BEATS || midiEditor.GetTimebase() == SOURCE_BEATS)
				{
					double ppqPos = midiEditor.GetStartPos() + ((r.left + p.x) / midiEditor.GetHZoom());
					mouseInfo.position = MIDI_GetProjTimeFromPPQPos(midiEditor.GetActiveTake(), ppqPos);
				}
				else
				{
					double timePos = MIDI_GetProjTimeFromPPQPos(midiEditor.GetActiveTake(), midiEditor.GetStartPos()) + ((r.left + p.x) / midiEditor.GetHZoom());
					mouseInfo.position = timePos;
				}
			}

			// Check ruler
			if (p.y < MIDI_RULER_H)
			{
				if      (cursorSegment == MIDI_WND_NOTEVIEW) mouseInfo.segment = "ruler";
				else if (cursorSegment == MIDI_WND_KEYBOARD) mouseInfo.segment = "unknown";
			}
			// Check others
			else
			{
				int viewHeight = abs(r.bottom - r.top);
				int ccFullHeight = midiEditor.GetCCLanesFullheight(cursorSegment == MIDI_WND_KEYBOARD);

				// Over CC lanes
				if (p.y > (viewHeight - ccFullHeight))
				{
					mouseInfo.segment = "cc_lane";

					if      (cursorSegment == MIDI_WND_NOTEVIEW) mouseInfo.details = "cc_lane";
					else if (cursorSegment == MIDI_WND_KEYBOARD) mouseInfo.details = "cc_selector";

					// Get CC lane number
					int ccStart = viewHeight - ccFullHeight + ((cursorSegment == MIDI_WND_KEYBOARD) ? (1) : (0));
					for (int i = 0; i < midiEditor.CountCCLanes(); ++i)
					{
						int ccEnd = ccStart + midiEditor.GetCCLaneHeight(i);
						if (i == midiEditor.CountCCLanes() - 1)
							ccEnd += (cursorSegment == MIDI_WND_KEYBOARD) ? (SCROLLBAR_W) : (0);

						if (p.y >= ccStart && p.y < ccEnd)
						{
							mouseInfo.ccLane = midiEditor.GetCCLane(i);
							mouseInfo.ccLaneId = i;
							m_ccLaneClickPoint.y = ccStart + MIDI_CC_LANE_CLICK_Y_OFFSET;
							m_ccLaneClickPoint.x = 0;
							m_midiEditorPianoWnd = (cursorSegment == MIDI_WND_KEYBOARD) ? segmentHwnd        : NULL;

							// Get CC value at Y position
							if (cursorSegment == MIDI_WND_NOTEVIEW)
							{
								ccStart += MIDI_LANE_DIVIDER_H;
								if (p.y >= ccStart)
								{
									int mouse = ccEnd - p.y;
									int laneHeight = ccEnd - ccStart;
									double steps = (mouseInfo.ccLane == CC_PITCH || mouseInfo.ccLane >= CC_14BIT_START) ? (16383) : (128);
									mouseInfo.ccLaneVal = (int)(steps / laneHeight * mouse);
									if (steps == 128 && mouseInfo.ccLaneVal == 128)
										mouseInfo.ccLaneVal = 127;
								}
							}
							break;
						}
						else
						{
							ccStart = ccEnd;
						}
					}
				}

				// Over note view/keyboard
				else
				{
					if      (cursorSegment == MIDI_WND_KEYBOARD) mouseInfo.segment = "piano";
					else if (cursorSegment == MIDI_WND_NOTEVIEW) mouseInfo.segment = "notes";

					// This is mouse Y position counting from the bottom - make sure it's not outside valid, drawable note range
					int realMouseY = (MIDI_RULER_H - p.y) - ((midiEditor.GetVPos() - 128) * midiEditor.GetVZoom());
					if (realMouseY > 0)
					{
						bool processKeyboardSeparately = true;
						const vector<int> visibleNoteRows = midiEditor.GetUsedNamedNotes();

						if (cursorSegment == MIDI_WND_NOTEVIEW)
							processKeyboardSeparately = false;
						else if ((midiEditor.GetPianoRoll() != 0) || (midiEditor.GetNoteshow() != SHOW_ALL_NOTES && visibleNoteRows.size()))
							processKeyboardSeparately = false;

						if (!processKeyboardSeparately)
						{
							int noteRow = (realMouseY - 1) / midiEditor.GetVZoom();

							if (midiEditor.GetNoteshow() == SHOW_ALL_NOTES)
							{
								mouseInfo.noteRow = noteRow;
							}
							else
							{
								int firstNoteRow = 128 - (int)visibleNoteRows.size();
								if (noteRow >= firstNoteRow)
									mouseInfo.noteRow = visibleNoteRows[noteRow - firstNoteRow];
							}
						}
						else
						{
							int noteRow = (realMouseY) / midiEditor.GetVZoom();
							bool doWhiteKeys = (p.x < MIDI_BLACK_KEYS_W) ? (false) : (true);

							if (!doWhiteKeys && IsMidiNoteBlack(noteRow))
								mouseInfo.noteRow = noteRow;
							else
								doWhiteKeys = true;

							if (doWhiteKeys)
							{
								int octave = noteRow / 12;
								int octaveStart = octave * midiEditor.GetVZoom() * 12;

								// First part of the octave (C D E)
								if (realMouseY >= octaveStart && realMouseY < (octaveStart + midiEditor.GetVZoom() * 5))
								{
									int fullHeight = midiEditor.GetVZoom() * 5;
									int keyHeight = fullHeight / 3;

									int leftOver = 0;
									if      ((fullHeight % 3) == 1) leftOver = 0 | 1;
									else if ((fullHeight % 3) == 2) leftOver = 0 | 3;

									int keyStart = octaveStart;
									for (int i = 0; i < 3; ++i)
									{
										int keyEnd = keyStart + keyHeight + GetBit(leftOver, i) + ((i == 0) ? (1) : (0));

										if (realMouseY >= keyStart && realMouseY < keyEnd)
										{
											mouseInfo.noteRow = (octave * 12) + (i * 2);
											break;
										}
										else
											keyStart = keyEnd;
									}
								}
								// Second part of the octave (F G A H)
								else
								{
									int fullHeight = midiEditor.GetVZoom() * 7;
									int keyHeight = fullHeight / 4;

									int leftOver = 0;
									if      ((fullHeight % 4) == 1) leftOver = 0 | 1;
									else if ((fullHeight % 4) == 2) leftOver = 0 | 5;
									else if ((fullHeight % 4) == 3) leftOver = 0 | 7;

									int keyStart = octaveStart + midiEditor.GetVZoom() * 5;
									for (int i = 0; i < 4; ++i)
									{
										int keyEnd = keyStart + keyHeight + GetBit(leftOver, i) + ((i == 0) ? (1) : (0));

										if (realMouseY >= keyStart && realMouseY < keyEnd)
										{
											mouseInfo.noteRow = (octave * 12) + (i * 2) + 5;
											break;
										}
										else
											keyStart = keyEnd;
									}
								}
							}
						}
					}
				}
			}
		}
		else
		{
			mouseInfo.segment = "unknown";
		}

		return true;
	}
	else
	{
		return false;
	}
}

bool BR_MouseInfo::GetContextMIDIInline (BR_MouseInfo::MouseInfo& mouseInfo, int mouseDisplayX, int mouseY, int takeHeight, int takeOffset)
{
	/* Returns false is there isn't inline CC lane, note row or keyboard under  *
	*  mouse cursor                                                             */

	if (takeHeight < INLINE_MIDI_MIN_H)
		return false;

	bool topBar       = (takeHeight - INLINE_MIDI_TOP_BAR_H < INLINE_MIDI_MIN_H) ? false : true;
	int editorOffsetY = takeOffset + ((topBar) ? INLINE_MIDI_TOP_BAR_H : 0);
	int editorHeight  = takeHeight - ((topBar) ? INLINE_MIDI_TOP_BAR_H : 0);

	if (mouseY < editorOffsetY || mouseY > takeOffset + takeHeight)
		return false;

	BR_MidiEditor midiEditor(mouseInfo.take);
	if (midiEditor.IsValid())
	{
		mouseInfo.window = "midi_editor";
		mouseInfo.inlineMidi = true;

		mouseInfo.pianoRollMode = midiEditor.GetPianoRoll() & 0xffff;

		// Get heights of various elements
		int ccFullHeight = midiEditor.GetCCLanesFullheight(false);

		int pianoRollFullHeight = editorHeight - ccFullHeight;
		if (pianoRollFullHeight < INLINE_MIDI_MIN_NOTEVIEW_H)
		{
			pianoRollFullHeight += ccFullHeight;
			ccFullHeight = 0;
		}

		// Over CC lanes
		if (mouseY > editorOffsetY + pianoRollFullHeight)
		{
			mouseInfo.segment = "cc_lane";
			mouseInfo.details = "cc_lane";

			// Get CC lane number
			int ccStart = editorOffsetY + pianoRollFullHeight;
			for (int i = 0; i < midiEditor.CountCCLanes(); ++i)
			{
				int ccEnd = ccStart + midiEditor.GetCCLaneHeight(i);

				if (mouseY >= ccStart && mouseY < ccEnd)
				{
					mouseInfo.ccLane   = midiEditor.GetCCLane(i);
					mouseInfo.ccLaneId = i;

					m_ccLaneClickPoint.y = ccStart + INLINE_MIDI_CC_LANE_CLICK_Y_OFFSET;
					m_ccLaneClickPoint.x = mouseDisplayX;
					m_midiEditorPianoWnd = NULL;

					// Get CC value at Y position
					ccStart += INLINE_MIDI_LANE_DIVIDER_H;
					if (mouseY >= ccStart)
					{
						int mouse = ccEnd - mouseY;
						int laneHeight = ccEnd - ccStart;
						double steps = (mouseInfo.ccLane == CC_PITCH || mouseInfo.ccLane >= CC_14BIT_START) ? (16383) : (128);
						mouseInfo.ccLaneVal = (int)(steps / laneHeight * mouse);
						if (steps == 128 && mouseInfo.ccLaneVal == 128)
							mouseInfo.ccLaneVal = 127;
					}
					break;
				}
				else
				{
					ccStart = ccEnd;
				}
			}
		}

		// Over note view / keyboard
		else
		{
			// Check if mouse is over keyboard or notes view
			RECT r; GetWindowRect(GetArrangeWnd(), &r);
			double arrangeStartTime, arrangeEndTime;
			double itemStartTime = GetMediaItemInfo_Value(mouseInfo.item, "D_POSITION");
			double itemEndTime   = GetMediaItemInfo_Value(mouseInfo.item, "D_LENGTH") + itemStartTime;
			GetSetArrangeView(NULL, false, &arrangeStartTime, &arrangeEndTime);

			int itemStartX = (itemStartTime <= arrangeStartTime) ? (0)                                                                  : ((int)((itemStartTime - arrangeStartTime) * midiEditor.GetHZoom()));
			int itemEndX   = (itemEndTime   >  arrangeEndTime  ) ? ((int)((arrangeEndTime - arrangeStartTime) * midiEditor.GetHZoom())) : ((int)((itemEndTime   - arrangeStartTime) * midiEditor.GetHZoom()));

			bool pianoVisible = (itemEndX - itemStartX - INLINE_MIDI_KEYBOARD_W > INLINE_MIDI_KEYBOARD_W) ? (true) : (false);
			if (mouseDisplayX >= itemStartX && mouseDisplayX < itemStartX + INLINE_MIDI_KEYBOARD_W)
				mouseInfo.segment = ((pianoVisible) ? ("piano") : ("notes"));
			else
				mouseInfo.segment = "notes";

			// Get note row
			int realMouseY = (editorOffsetY -mouseY) - ((midiEditor.GetVPos() - 128) * midiEditor.GetVZoom()); // Mouse Y position counting from the bottom
			if (realMouseY > 0 && midiEditor.GetVZoom())
			{
				if (midiEditor.GetNoteshow() == SHOW_ALL_NOTES)
					mouseInfo.noteRow = (realMouseY - 1) / midiEditor.GetVZoom();
				else
				{
					/* Due to chunk values, inline editor vertical zoom and vertical position can't be known directly at all *
					*  times so don't check note row  (it can probably get deduced, but we'll do this some other time)       */
				}
			}
		}

		return true;
	}
	return false;
}

bool BR_MouseInfo::IsStretchMarkerVisible (MediaItem_Take* take, int id, double takePlayrate, double arrangeZoom)
{
	/* This checks if stretch marker is so close *
	*  to item edge that REAPER hides it         */

	bool visible = false;
	if (MediaItem* item = GetMediaItemTake_Item(take))
	{
		double stretchMarkerPos;
		GetTakeStretchMarker(take, id, &stretchMarkerPos, NULL);
		stretchMarkerPos = ItemTimeToProjectTime(item, stretchMarkerPos / takePlayrate);

		double itemStart = GetMediaItemInfo_Value(item, "D_POSITION");
		double itemEnd   = GetMediaItemInfo_Value(item, "D_LENGTH") + itemStart;

		int x0 = RoundToInt(itemStart * arrangeZoom);
		int x2 = TruncToInt(itemEnd   * arrangeZoom);
		int x1 = TruncToInt(stretchMarkerPos * arrangeZoom);

		if (CheckBounds(x1, x0, x2))
			visible = true;
	}

	return visible;
}

int BR_MouseInfo::IsMouseOverStretchMarker (MediaItem* item, MediaItem_Take* take, int takeHeight, int takeOffset, int mouseDisplayX, int mouseY, double mousePos, double arrangeStart, double arrangeZoom)
{
	int returnId = -1;

	if (takeHeight >= STRETCH_M_MIN_TAKE_HEIGHT)
	{
		double takePlayrate = GetMediaItemTakeInfo_Value(take, "D_PLAYRATE");
		// Mouse is within stretch marker Y range, look for X axis of a closest stretch marker
		int id = FindClosestStretchMarker(take, ProjectTimeToItemTime(item, mousePos) * takePlayrate);
		if (id != -1)
		{
			int count = GetTakeNumStretchMarkers(take);
			while (id < count && !this->IsStretchMarkerVisible(take, id, takePlayrate, arrangeZoom))
				id++;

			double stretchMarkerPos;
			GetTakeStretchMarker(take, id, &stretchMarkerPos, NULL);
			stretchMarkerPos = ItemTimeToProjectTime(item, stretchMarkerPos / takePlayrate) - arrangeStart; // convert to "displayed" time

			if (stretchMarkerPos > 0)
			{
				int x1 = RoundToInt(stretchMarkerPos * arrangeZoom);
				int x0 = x1 - STRETCH_M_HIT_POINT;
				int x2 = x1 + STRETCH_M_HIT_POINT + 1;
				if (CheckBounds(mouseDisplayX, x0, x2))
					returnId = id;
			}
		}
	}

	return returnId;
}

int BR_MouseInfo::IsMouseOverEnvelopeLine (BR_Envelope& envelope, int drawableEnvHeight, int yOffset, int mouseDisplayX, int mouseY, double mousePos, double arrangeStart, double arrangeZoom, int* pointUnderMouse)
{
	/*  Return values: 0 -> no hit, 1 -> over point, 2 - > over segment */

	int mouseHit = 0;
	int pointId  = -1;

	// Check if mouse is in drawable part of envelope lane where line resides
	if (mouseY >= yOffset && mouseY < yOffset + drawableEnvHeight)
	{
		double mousePosLeft = mousePos - 1/arrangeZoom * ENV_HIT_POINT*2;
		double mousePosRight = mousePos + 1/arrangeZoom * ENV_HIT_POINT*2;

		int prevId = envelope.FindPrevious(mousePos);
		int nextId = prevId + 1;

		int tempoHit = (envelope.IsTempo()) ? (ENV_HIT_POINT) : (0); // for some reason, tempo points have double up/down hit area
		bool found = false;

		// Check surrounding points first - they take priority over segment
		if (!found)
		{
			// Check all the points around mouse cursor position
			// gotcha: since point can be partially visible even when it's position is not within arrange start/end we don't check if within bounds
			double prevPos, prevVal;
			while (envelope.GetPoint(prevId, &prevPos, &prevVal, NULL, NULL) && CheckBounds(prevPos, mousePosLeft, mousePosRight))
			{
				int x = RoundToInt(arrangeZoom * (prevPos - arrangeStart));
				int y = yOffset + drawableEnvHeight - RoundToInt(envelope.NormalizedDisplayValue(prevVal) * drawableEnvHeight);
				if (CheckBounds(mouseDisplayX, x - ENV_HIT_POINT, x + ENV_HIT_POINT_LEFT) && CheckBounds(mouseY, y - ENV_HIT_POINT - tempoHit, y + ENV_HIT_POINT_DOWN + tempoHit))
				{
					mouseHit = 1;
					pointId = prevId;
					found = true;
					break;
				}
				--prevId;
			}
		}
		if (!found)
		{
			double nextPos, nextVal;
			while (envelope.GetPoint(nextId, &nextPos, &nextVal, NULL, NULL) && CheckBounds(nextPos, mousePosLeft, mousePosRight))
			{
				int x = RoundToInt(arrangeZoom * (nextPos - arrangeStart));
				int y = yOffset + drawableEnvHeight - RoundToInt(envelope.NormalizedDisplayValue(nextVal) * drawableEnvHeight);
				if (CheckBounds(mouseDisplayX, x - ENV_HIT_POINT, x + ENV_HIT_POINT_LEFT) && CheckBounds(mouseY, y - ENV_HIT_POINT - tempoHit, y + ENV_HIT_POINT_DOWN + tempoHit))
				{
					mouseHit = 1;
					pointId = nextId;
					found = true;
					break;
				}
				++nextId;
			}
		}

		// Not over points, check segment
		if (!found)
		{
			double mouseValue = envelope.ValueAtPosition(mousePos);
			int x = RoundToInt(arrangeZoom * (mousePos - arrangeStart));
			int y = yOffset + drawableEnvHeight - RoundToInt(envelope.NormalizedDisplayValue(mouseValue) * drawableEnvHeight);
			if (CheckBounds(mouseDisplayX, x - ENV_HIT_POINT, x + ENV_HIT_POINT) && CheckBounds(mouseY, y - ENV_HIT_POINT, y + ENV_HIT_POINT_DOWN))
			{
				mouseHit = 2;
			}
		}
	}
	WritePtr(pointUnderMouse, pointId);
	return mouseHit;
}

int BR_MouseInfo::IsMouseOverEnvelopeLineTrackLane (MediaTrack* track, int trackHeight, int trackOffset, list<TrackEnvelope*>& laneEnvs, int mouseDisplayX, int mouseY, double mousePos, double arrangeStart, double arrangeZoom, TrackEnvelope** trackEnvelope, int* pointUnderMouse)
{
	/* laneEnv list should hold all track envelopes that have their own lane so *
	*  we don't have to check for their visibility in track lane (chunks are    *
	*  expensive). Also note that laneEnvs WILL get modified during the process *
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

	// Find envelope lane in track lane at mouse cursor and check mouse cursor against it
	int envLaneCount = (int)trackLaneEnvs.size();
	if (envLaneCount > 0)
	{
		int overlapLimit,trackGapTop, trackGapBottom;
		overlapLimit = ConfigVar<int>("env_ol_minh").value_or(0);
		GetTrackGap(trackHeight, &trackGapTop, &trackGapBottom);

		int envLaneFull = trackHeight - trackGapTop - trackGapBottom;
		bool envelopesOverlapping = (overlapLimit >= 0 && envLaneFull / envLaneCount < overlapLimit) ? (true) : (false);

		// Each envelope has it's own lane, find the right one
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

						mouseHit = this->IsMouseOverEnvelopeLine(envelope, envHeight, envOffset, mouseDisplayX, mouseY, mousePos, arrangeStart, arrangeZoom, pointUnderMouse);
						if (mouseHit != 0)
							envelopeUnderMouse = envelope.GetPointer();
						break;
					}
				}
			}
		}
		// Envelopes are overlapping each other, search them all
		else
		{
			int envHeight = envLaneFull - 2*ENV_GAP;
			if (envHeight > ENV_LINE_WIDTH)
			{
				for (int i = 0; i < envLaneCount; ++i)
				{
					int envOffset = trackOffset + trackGapTop + ENV_GAP;
					BR_Envelope envelope(trackLaneEnvs[i]);

					mouseHit = this->IsMouseOverEnvelopeLine(envelope, envHeight, envOffset, mouseDisplayX, mouseY, mousePos, arrangeStart, arrangeZoom, pointUnderMouse);
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

int BR_MouseInfo::IsMouseOverEnvelopeLineTake (MediaItem_Take* take, int takeHeight, int takeOffset, int mouseDisplayX, int mouseY, double mousePos, double arrangeStart, double arrangeZoom, TrackEnvelope** trackEnvelope, int* pointUnderMouse)
{
	/* Return values: 0 -> no hit, 1 -> over point, 2 - > over segment *
	*  If there is a hit, trackEnvelope will hold envelope             */

	int mouseHit = 0;
	TrackEnvelope* envelopeUnderMouse = NULL;

	// Get all visible take envelopes
	vector<TrackEnvelope*> envelopes;
	const int count = CountTakeEnvelopes(take);
	for (int i = 0; i < count; ++i)
	{
		TrackEnvelope* envelope = GetTakeEnvelope(take, i);
		if (EnvVis(envelope, NULL))
			envelopes.push_back(envelope);
	}

	// Find envelope under mouse cursor
	int envelopeCount = (int)envelopes.size();
	if (envelopeCount > 0)
	{
		const int overlapLimit = ConfigVar<int>("env_ol_minh").value_or(0);
		bool envelopesOverlapping = (overlapLimit >= 0 && takeHeight / envelopeCount < overlapLimit) ? (true) : (false);

		// Each envelope has it's own lane, find the right one
		if (!envelopesOverlapping)
		{
			int envLaneH = takeHeight / envelopeCount;
			int envHeight = envLaneH - 2*ENV_GAP;
			if (envHeight > ENV_LINE_WIDTH)
			{
				for (int i = 0; i < envelopeCount; ++i)
				{
					int envelopeStart = takeOffset + ENV_GAP + envLaneH * i;
					int envelopeEnd = envelopeStart + envLaneH;
					if (mouseY >= envelopeStart && mouseY < envelopeEnd)
					{
						int envOffset = takeOffset + ENV_GAP + + envLaneH * i;
						BR_Envelope envelope(envelopes[i]);

						mouseHit = this->IsMouseOverEnvelopeLine(envelope, envHeight, envOffset, mouseDisplayX, mouseY, mousePos, arrangeStart, arrangeZoom, pointUnderMouse);
						if (mouseHit != 0)
							envelopeUnderMouse = envelope.GetPointer();
						break;
					}
				}
			}
		}
		// Envelopes are overlapping each other, search them all
		else
		{
			int envHeight = takeHeight - 2*ENV_GAP;
			if (envHeight > ENV_LINE_WIDTH)
			{
				for (int i = 0; i < envelopeCount; ++i)
				{
					int envOffset = takeOffset + ENV_GAP;
					BR_Envelope envelope(envelopes[i]);

					mouseHit = this->IsMouseOverEnvelopeLine(envelope, envHeight, envOffset, mouseDisplayX, mouseY, mousePos, arrangeStart, arrangeZoom, pointUnderMouse);
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

int BR_MouseInfo::GetRulerLaneHeight (int rulerH, int lane)
{
	/* lane: 0 -> regions  *
	*        1 -> markers  *
	*        2 -> tempo    *
	*        3 -> timeline */

	int timeline = RoundToInt((double)rulerH / 2);
	int markers = TruncToInt((double)timeline / 3) + 1;

	if (lane == 0)
		return rulerH - markers*2 - timeline;
	if (lane == 1 || lane == 2)
		return markers;
	if (lane == 3)
		return timeline;
	return 0;
}

int BR_MouseInfo::IsHwndMidiEditor (HWND hwnd, HWND* midiEditor, HWND* subView)
{
	int status = 0;

	if (MIDIEditor_GetMode(hwnd) != -1)
	{
		status = MIDI_WND_UNKNOWN;
		WritePtr(midiEditor, hwnd);
		WritePtr(subView, (HWND)NULL);
	}
	else
	{
		while (hwnd)
		{
			HWND subWnd = hwnd;
			hwnd = GetParent(hwnd);
			if (MIDIEditor_GetMode(hwnd) != -1)
			{
				WritePtr(midiEditor, hwnd);
				WritePtr(subView, subWnd);

				if      (subWnd == GetNotesView(hwnd)) {status = MIDI_WND_NOTEVIEW; break;}
				else if (subWnd == GetPianoView(hwnd)) {status = MIDI_WND_KEYBOARD; break;}
				else                                   {status = MIDI_WND_UNKNOWN;  break;}
			}
		}
	}
	return status;
}

bool BR_MouseInfo::SortEnvHeightsById (const pair<int,int>& left, const pair<int,int>& right)
{
	return left.second < right.second;
}

void BR_MouseInfo::GetTrackOrEnvelopeFromY (int y, TrackEnvelope** _envelope, MediaTrack** _track, list<TrackEnvelope*>* envelopes, int* height, int* offset)
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
		vector<pair<int,int>> envHeights;
		bool yInTrack = (y < elementOffset + elementHeight) ? true : false;

		int count = CountTrackEnvelopes(track);
		for (int i = 0; i < count; ++i)
		{
			TrackEnvelope *env = GetTrackEnvelope(track, i);

			if (GetEnvelopeInfo_Value(env,"I_TCPH") < 1.0) continue;
			if (GetEnvelopeInfo_Value(env,"I_TCPY") < elementHeight) continue; // does not have an envcp

			if (yInTrack)
			{
				if (envelopes)
					envelopes->push_back(env);
			}
			else
			{
				const int envHeight = static_cast<int>(GetEnvelopeInfo_Value(env,"I_TCPH"));
				const int envId = i;
				envHeights.push_back(make_pair(envHeight, envId));
			}
		}

		if (!yInTrack)
		{
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
