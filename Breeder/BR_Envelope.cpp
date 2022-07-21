/******************************************************************************
/ BR_Envelope.cpp
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

#include "BR_Envelope.h"
#include "BR_ContinuousActions.h"
#include "BR_EnvelopeUtil.h"
#include "BR_MidiEditor.h"
#include "BR_MouseUtil.h"
#include "BR_ProjState.h"
#include "BR_Util.h"

#include <WDL/localize/localize.h>

/******************************************************************************
* Commands: Envelope continuous actions                                       *
******************************************************************************/
static BR_Envelope* g_envMouseEnvelope = NULL;
static bool         g_envMouseDidOnce  = false;
static RECT         g_envMouseBounds;

static bool EnvMouseInit (COMMAND_T* ct, bool init)
{
	if (!init)
		delete g_envMouseEnvelope;

	g_envMouseEnvelope = NULL;
	g_envMouseDidOnce = false;
	SetAllCoordsToZero(&g_envMouseBounds);
	return true;
}

static int EnvMouseUndo (COMMAND_T* ct)
{
	return (g_envMouseDidOnce) ? (UNDO_STATE_TRACKCFG | UNDO_STATE_ITEMS) : (0);
}

static HCURSOR EnvMouseCursor (COMMAND_T* ct, int window)
{
	if (window == BR_ContinuousAction::MAIN_ARRANGE || window == BR_ContinuousAction::MAIN_RULER)
		return (abs((int)ct->user) == 3) ? GetSwsMouseCursor(CURSOR_ENV_PEN_GRID) : GetSwsMouseCursor(CURSOR_ENV_PT_ADJ_VERT);
	else
		return NULL;
}

static WDL_FastString EnvMouseTooltip (COMMAND_T* ct, int window, bool* setToBounds, RECT* bounds)
{
	WDL_FastString tooltip;

	if (g_envMouseEnvelope && !GetBit(ContinuousActionTooltips(), 0))
	{
		if (window == BR_ContinuousAction::MAIN_ARRANGE || window == BR_ContinuousAction::MAIN_RULER)
		{
			int envHeight, envY, yOffset;
			bool overRuler;
			double position = PositionAtMouseCursor(true, false, &yOffset, &overRuler);

			g_envMouseEnvelope->VisibleInArrange(&envHeight, &envY, true);
			yOffset = (overRuler) ? envY : SetToBounds(yOffset, envY, envY + envHeight);

			double displayValue = ((double)envHeight + (double)envY - (double)yOffset) / (double)envHeight;
			double value        = g_envMouseEnvelope->SnapValue(g_envMouseEnvelope->RealValue(displayValue));

			if (g_envMouseEnvelope->IsTempo())
				GetTempoTimeSigMarker(NULL, (abs((int)ct->user) == 1)  ? FindClosestTempoMarker(position) : FindPreviousTempoMarker(position), &position, NULL, NULL, NULL, NULL, NULL, NULL);
			else
				g_envMouseEnvelope->GetPoint((abs((int)ct->user) == 1) ? g_envMouseEnvelope->FindClosest(position) : g_envMouseEnvelope->FindPrevious(position), &position, NULL, NULL, NULL);

			static const char* s_format = __localizeFunc("Envelope: %s\n%s at %s", "tooltip", 0);
			tooltip.AppendFormatted(512, s_format, (g_envMouseEnvelope->GetName()).Get(), (g_envMouseEnvelope->FormatValue(value)).Get(), FormatTime(position).Get());

			if (AreAllCoordsZero(g_envMouseBounds))
				g_envMouseBounds = GetDrawableArrangeArea();
			WritePtr(bounds,      g_envMouseBounds);
			WritePtr(setToBounds, true);
		}
	}
	return tooltip;
}

static void SetEnvPointMouseValue (COMMAND_T* ct)
{
	static int    s_lastEndId           = -1;
	static double s_lastEndPosition     = -1;
	static double s_lastEndNormVal      = -1;
	static bool   s_freehandInitDone    = false;

	int user = abs((int)ct->user);

	// Action called for the first time
	if (!g_envMouseEnvelope)
	{
		s_lastEndId        = -1;
		s_lastEndPosition  = -1;
		s_lastEndNormVal   = -1;
		s_freehandInitDone = (user == 3) ? false : true;

		if ((int)ct->user < 0)
		{
			TrackEnvelope* envelopeToSelect = NULL;

			BR_MouseInfo mouseInfo(BR_MouseInfo::MODE_ARRANGE | BR_MouseInfo::MODE_IGNORE_ENVELOPE_LANE_SEGMENT);
			if ((!strcmp(mouseInfo.GetWindow(),  "arrange")   && !strcmp(mouseInfo.GetSegment(), "envelope"))   ||
				(!strcmp(mouseInfo.GetDetails(), "env_point") || !strcmp(mouseInfo.GetDetails(), "env_segment"))
			)
				envelopeToSelect = mouseInfo.GetEnvelope();

			if (envelopeToSelect)
			{
				if (envelopeToSelect != GetSelectedEnvelope(NULL))
				{
					SetCursorContext(2, envelopeToSelect);
					UpdateArrange();
				}
			}
			else if (!GetSelectedEnvelope(NULL))
			{
				ContinuousActionStopAll();
				return;
			}
		}

		g_envMouseEnvelope = new (nothrow) BR_Envelope(GetSelectedEnvelope(NULL));

		// BR_Envelope does check for locking but we're also using tempo API here so check manually
		if (!g_envMouseEnvelope || g_envMouseEnvelope->CountPoints() <= 0 || PositionAtMouseCursor(false) == -1 || g_envMouseEnvelope->IsLocked())
		{
			ContinuousActionStopAll();
			return;
		}
	}
	bool pointsEdited     = false;
	bool isTempo          = g_envMouseEnvelope->IsTempo();
	TrackEnvelope* envPtr = g_envMouseEnvelope->GetPointer();

	// Check envelope is visible
	int envHeight, envY;                                                // caching values is not 100% correct, but we don't expect for envelope lane height to change during the action
	if (!g_envMouseEnvelope->VisibleInArrange(&envHeight, &envY, true)) // and it can speed things if dealing with big envelope that's in track lane (quite possible with tempo map)
		return;

	// Get mouse positions
	int yOffset; bool overRuler;
	double endPosition   = PositionAtMouseCursor(true, false, &yOffset, &overRuler);
	double startPosition = (s_lastEndPosition < 0) ? (endPosition) : (s_lastEndPosition);
	if (endPosition == -1)
	{
		// If mouse is over TCP, don't stop just yet...user might have just moved the mouse there while editing the envelope (unless action was called for the first time, then stop all always)
		int context; TrackAtMouseCursor(&context, NULL);
		if (s_lastEndPosition < 0 || context != 0)
			ContinuousActionStopAll();
		return;
	}

	// Take envelope corner-case (pretend mouse position never got behind item's start)
	if (g_envMouseEnvelope->IsTakeEnvelope())
	{
		if (MediaItem* item = GetMediaItemTake_Item(g_envMouseEnvelope->GetTake()))
		{
			double itemStart = GetMediaItemInfo_Value(item, "D_POSITION");
			if (endPosition < itemStart)
				endPosition = itemStart;
		}
	}

	// If calling for the first time when freehand drawing, unselect all points
	if (!s_freehandInitDone)
	{
		if (isTempo) UnselectAllTempoMarkers();
		else         g_envMouseEnvelope->UnselectAll();
		s_freehandInitDone = true;
	}

	// Get normalized mouse values
	yOffset = (overRuler) ? envY : SetToBounds(yOffset, envY, envY + envHeight);
	double endDisplayVal   = ((double)envHeight + (double)envY - (double)yOffset) / (double)envHeight;
	double startDisplayVal = (s_lastEndPosition < 0) ? (endDisplayVal) : (s_lastEndNormVal);

	PreventUIRefresh(1);

	// In case of freehand drawing, remove or add points (so we only have points on grid)...(because editing tempo points moves them, we will handle it afterwards)
	if (user == 3 && !isTempo)
	{
		// Make sure start and end position are not reversed
		double normStartPosition = startPosition;
		double normEndPosition   = endPosition;
		if (normEndPosition < normStartPosition)
			swap(normEndPosition, normStartPosition);

		// Get grid divisions
		vector<double> savedGridDivs;
		double gridDivPos = GetPrevGridDiv(GetNextGridDiv(normStartPosition)); // can't just do GetPrevGrid(normStartPosition) because if grid division is right at normStartPosition we would end up with grid division before startPosition
		while (gridDivPos < normEndPosition)
		{
			savedGridDivs.push_back(gridDivPos);
			gridDivPos = GetNextGridDiv(gridDivPos);
		}

		if (savedGridDivs.size() > 0)
		{
			// Get possible start point value before deleting points (but only if there are no points between first grid division and division before it)
			double prevPointVal = -1;
			double prevPointPos = -1;
			double prevGridDivPos = GetPrevGridDiv(savedGridDivs.front());
			if (prevGridDivPos != savedGridDivs.front())
			{
				double prevTempoPosition;
				if (!g_envMouseEnvelope->GetPoint(g_envMouseEnvelope->FindNext(prevGridDivPos), &prevTempoPosition, NULL, NULL, NULL) || prevTempoPosition >= savedGridDivs.front() - GRID_DIV_DELTA)
				{
					if (!g_envMouseEnvelope->ValidateId(g_envMouseEnvelope->Find(prevGridDivPos, GRID_DIV_DELTA)))
					{
						prevPointVal = g_envMouseEnvelope->ValueAtPosition(prevGridDivPos);
						prevPointPos = prevGridDivPos;
					}
				}
			}

			// Get possible end point value before deleting points (don't create end point if last grid division point is also last envelope point)
			double nextPointVal = -1;
			double nextPointPos = -1;
			double nextGridDivPos = GetNextGridDiv(savedGridDivs.back());
			if (!g_envMouseEnvelope->ValidateId(g_envMouseEnvelope->Find(nextGridDivPos, GRID_DIV_DELTA)))
			{
				if (g_envMouseEnvelope->ValidateId(g_envMouseEnvelope->FindNext(nextGridDivPos)))
				{
					nextPointVal = g_envMouseEnvelope->ValueAtPosition(savedGridDivs.back());
					nextPointPos = nextGridDivPos;
				}
			}

			// Delete all points between those grid divisions
			g_envMouseEnvelope->DeletePointsInRange(savedGridDivs.front(), GetNextGridDiv(savedGridDivs.back()) - GRID_DIV_DELTA);

			// Insert grid division points
			if (prevPointPos != -1) g_envMouseEnvelope->CreatePoint(g_envMouseEnvelope->CountPoints(), prevPointPos, prevPointVal, g_envMouseEnvelope->GetDefaultShape(), 0, false, true, true);
			if (nextPointPos != -1) g_envMouseEnvelope->CreatePoint(g_envMouseEnvelope->CountPoints(), nextPointPos, nextPointVal, g_envMouseEnvelope->GetDefaultShape(), 0, false, true, true);
			for (size_t i = 0; i < savedGridDivs.size(); ++i)
				g_envMouseEnvelope->CreatePoint(g_envMouseEnvelope->CountPoints(), savedGridDivs[i], 100, g_envMouseEnvelope->GetDefaultShape(), 0, true, true, true);

			g_envMouseEnvelope->Sort();
			pointsEdited = true;

			// Since points are added/removed don't rely on previous end id but find it again
			s_lastEndId = g_envMouseEnvelope->FindPrevious(startPosition);
			double nextPos;
			if (g_envMouseEnvelope->GetPoint(s_lastEndId+1, &nextPos, NULL, NULL, NULL) && nextPos == startPosition)
				++s_lastEndId;
		}
	}

	// Find all the points over which mouse passed
	int startId = -1;
	int endId   = -1;
	if (user == 1)
	{
		if (s_lastEndId < 0) startId = (isTempo) ? FindClosestTempoMarker(startPosition) : g_envMouseEnvelope->FindClosest(startPosition);
		else                 startId = s_lastEndId;
		endId  = (isTempo) ? FindClosestTempoMarker(endPosition) : g_envMouseEnvelope->FindClosest(endPosition);
	}
	else
	{
		if (s_lastEndId < 0)
		{
			startId = (isTempo) ? FindPreviousTempoMarker(startPosition) : g_envMouseEnvelope->FindPrevious(startPosition);
			double nextPos;
			if (isTempo && GetTempoTimeSigMarker(NULL, startId, &nextPos, NULL, NULL, NULL, NULL, NULL, NULL) && nextPos == startPosition)
				++startId;
			else if (!isTempo && g_envMouseEnvelope->GetPoint(startId+1, &nextPos, NULL, NULL, NULL) && nextPos == startPosition)
				++startId;
		}
		else
		{
			startId = s_lastEndId;
		}

		endId = (isTempo) ? FindPreviousTempoMarker(endPosition) : g_envMouseEnvelope->FindPrevious(endPosition);
		double nextPos;
		if (isTempo && GetTempoTimeSigMarker(NULL, endId, &nextPos, NULL, NULL, NULL, NULL, NULL, NULL) && nextPos == endPosition)
			++endId;
		else if (!isTempo && g_envMouseEnvelope->GetPoint(endId+1, &nextPos, NULL, NULL, NULL) && nextPos == endPosition)
			++endId;
	}

	// Prepare things before applying changes to envelope
	bool oppositeDirection = false;
	if (endId < startId || endPosition < startPosition)
	{
		swap(endId, startId);
		swap(endPosition,   startPosition);
		swap(endDisplayVal, startDisplayVal);
		oppositeDirection = true;
	}
	double endPosQN = isTempo ? TimeMap_timeToQN_abs(NULL, endPosition) : 0;

	// prevent edit cursor undo
	const ConfigVar<int> editCursorUndo("undomask");
	ConfigVarOverride<int> tempMask(editCursorUndo, ClearBit(editCursorUndo.value_or(0), 3));

	// prevent reselection
	const ConfigVar<int> envClickSegMode("envclicksegmode");
	ConfigVarOverride<int> tempSegMode(envClickSegMode, ClearBit(envClickSegMode.value_or(0), 6));

	// In case of tempo freehand drawing we simultaneously add/remove points and edit them (because editing tempo map makes things move and me must not allow any changes to anything in front of mouse cursor)
	if (user == 3 && isTempo)
	{
		// Get grid division info
		vector<pair<double,double> > savedGridDivs;
		double gridDivPos = GetPrevGridDiv(GetNextGridDiv(startPosition)); // can't just do GetPrevGrid(startPosition) because if grid division is right at startPosition we would end up with grid division before startPosition
		double prevPointBpm = -1; // if there are no points between previous grid division and first grid
		if (gridDivPos > 0)       // division at/after mouse, insert one more point on that previous grid division
		{
			double prevGridDivPos = GetPrevGridDiv(gridDivPos);
			double position;
			if (!GetTempoTimeSigMarker(NULL, FindNextTempoMarker(prevGridDivPos), &position, NULL, NULL, NULL, NULL, NULL, NULL) || position >= gridDivPos - GRID_DIV_DELTA)
			{
				int id = FindTempoMarker(prevGridDivPos, GRID_DIV_DELTA);
				if (id < 0 || id >= CountTempoTimeSigMarkers(NULL))
				{
					savedGridDivs.push_back(make_pair(prevGridDivPos, TimeMap_timeToQN_abs(NULL, prevGridDivPos)));
					prevPointBpm = TempoAtPosition(prevGridDivPos);
				}
			}
		}
		while (gridDivPos < endPosition)
		{
			savedGridDivs.push_back(make_pair(gridDivPos, TimeMap_timeToQN_abs(NULL, gridDivPos)));
			gridDivPos = GetNextGridDiv(gridDivPos);
		}

		// Iterate through all grid divisions and make sure points only exist on those divisions and nowhere else
		int pointsAdded = 0;
		bool breakEarly = false;
		for (size_t i = 0; i < savedGridDivs.size(); ++i)
		{
			// Get data about about this division
			double gridDiv = TimeMap_QNToTime_abs(NULL, savedGridDivs[i].second);
			double nextGridDivQN  = (i == savedGridDivs.size() - 1) ? TimeMap_timeToQN_abs(NULL, GetNextGridDiv(gridDiv)) : savedGridDivs[i+1].second;
			double nextGridDivBpm = TempoAtPosition(TimeMap_QNToTime_abs(NULL, nextGridDivQN)); // need to get this here, before editing points

			// Check if there's already an existing point on this grid division and calculate it's new value
			int id = FindTempoMarker(gridDiv, GRID_DIV_DELTA);
			double newVal;
			bool selectPoint = true;
			if (i == 0 && prevPointBpm != -1)
			{
				newVal      = prevPointBpm;
				selectPoint = false;
			}
			else
			{
				double posForVal = (savedGridDivs[i].first < startPosition) ? startPosition : (savedGridDivs[i].first > endPosition) ? endPosition : savedGridDivs[i].first;
				double newDisplayVal = (abs(endPosition - startPosition) == 0)
				                     ? startDisplayVal
				                     : startDisplayVal + (endDisplayVal - startDisplayVal) * ((posForVal - startPosition) / (endPosition - startPosition));
				newVal = g_envMouseEnvelope->RealValue(newDisplayVal);
			}

			// Add or edit point on grid division
			bool addedGridPoint = false;
			if (id != -1)
			{
				int currentTempoCount = CountTempoTimeSigMarkers(NULL);
				int num, den;
				double position;
				GetTempoTimeSigMarker(NULL, id, &position, NULL, NULL, NULL, &num, &den, NULL);
				InsertStretchMarkerInAllItems(position, true, GRID_DIV_DELTA / 2);

				if (den != 0)
				{
					double positionQN = TimeMap_timeToQN_abs(NULL, position);
					if (SetTempoTimeSigMarker(NULL, id, position, -1, -1, newVal, num, den, g_envMouseEnvelope->GetDefaultShape() == LINEAR))
					{
					    if (SetTempoTimeSigMarker(NULL, id, TimeMap_QNToTime_abs(NULL, positionQN), -1, -1, newVal, num, den, g_envMouseEnvelope->GetDefaultShape() == LINEAR))
							pointsEdited = true;
					}
				}
				else
				{
					int measure; double beats = TimeMap2_timeToBeats(NULL, gridDiv, &measure, NULL, NULL, NULL);
					if (SetTempoTimeSigMarker(NULL, id, -1, measure, beats, newVal, 0, 0, g_envMouseEnvelope->GetDefaultShape() == LINEAR))
						pointsEdited = true;
				}

				// Editing tempo markers can theoretically delete them if they end up too close - if that happens, we can't be sure of id, so search for it again
				if (currentTempoCount != CountTempoTimeSigMarkers(NULL))
					id = FindTempoMarker(TimeMap_QNToTime_abs(NULL, savedGridDivs[i].second), GRID_DIV_DELTA);
			}
			else
			{
				int measure; double beats = TimeMap2_timeToBeats(NULL, gridDiv, &measure, NULL, NULL, NULL);
				InsertStretchMarkerInAllItems(gridDiv, true, GRID_DIV_DELTA / 2);
				if (SetTempoTimeSigMarker(NULL, -1, -1, measure, beats, newVal, 0, 0, g_envMouseEnvelope->GetDefaultShape() == LINEAR))
				{
					pointsEdited   = true;
					addedGridPoint = true;
					id = FindTempoMarker(TimeMap_QNToTime_abs(NULL, savedGridDivs[i].second), GRID_DIV_DELTA);
				}
			}

			// Current grid division point was added/edited...do rest of the stuff
			if (id != -1)
			{
				// Make sure grid division point is selected
				if (addedGridPoint)
				{
					for (int j = CountTempoTimeSigMarkers(NULL) - 2; j >= id; --j) // adding points with SetTempoTimeSigMarker(id = -1) shifts selection, restore it back
					{
						bool selected;
						GetEnvelopePoint(envPtr, j,   NULL, NULL, NULL, NULL, &selected);
						SetEnvelopePoint(envPtr, j+1, NULL, NULL, NULL, NULL, &selected, NULL);
					}
				}
				if (selectPoint) SetEnvelopePoint(envPtr, id, NULL, NULL, NULL, NULL, &g_bTrue, NULL);

				// After setting grid division tempo marker make sure that grid division didn't end
				// up out of current start/end range due to tempo point position change (if so, leave it for next iteration)
				GetTempoTimeSigMarker(NULL, id, &gridDiv, NULL, NULL, NULL, NULL, NULL, NULL);
				if (!oppositeDirection && endId != startId && !CheckBounds(gridDiv, startPosition - (GRID_DIV_DELTA * 2), endPosition + (GRID_DIV_DELTA * 2))) // GRID_DIV_DELTA in CheckBounds is important, due to rounding errors first gridDiv could be before startPosition by a really small
				{                                                                                                                                              // amount and thus break processing on rest of the points in all function calls until mouse cursor changes movement direction
					// In case grid division moved in front of endPosition and there are no tempo points after it, delete grid div point (we never insert right point after edited point if there are no points in front it)
					if (id != 0 && gridDiv > endPosition && id == CountTempoTimeSigMarkers(NULL) - 1)
					{
						DeleteTempoTimeSigMarker(NULL, id);
						--id;
					}

					endId        = (id > 0) ? (id - 1) : (0);
					endPosQN     = savedGridDivs[i].second;
					endPosition  = gridDiv;
					double posForVal = (savedGridDivs[i].first < startPosition) ? startPosition : (savedGridDivs[i].first > endPosition) ? endPosition : savedGridDivs[i].first;
					endDisplayVal = (abs(endPosition - startPosition) == 0)
					              ? startDisplayVal
					              : startDisplayVal + (endDisplayVal - startDisplayVal) * ((posForVal - startPosition) / (endPosition - startPosition));

					pointsAdded = 0;   // breaking early with the right endId set, adding these points to endId later would screw everything
					breakEarly = true; // but do not break; just yet since we need to make sure there is point at grid division after edited points
				}

				// Delete points between these two grid divisions (if breaking early, leave those for next function call)
				int pointsDeleted = 0;
				if (!breakEarly)
				{
					int count = CountTempoTimeSigMarkers(NULL);
					for (int j = id + 1; j < count; ++j)
					{
						double position;
						if (!GetTempoTimeSigMarker(NULL, j - pointsDeleted, &position, NULL, NULL, NULL, NULL, NULL, NULL) || TimeMap_timeToQN_abs(NULL, position) >= nextGridDivQN - GRID_DIV_DELTA)
							break;
						else
						{
							if (DeleteTempoTimeSigMarker(NULL, j - pointsDeleted))
							{
								int tempoCount = CountTempoTimeSigMarkers(NULL);
								for (int h = j - pointsDeleted + 1; h < tempoCount ; ++h) // deleting points with DeleteTempoTimeSigMarker() shifts selection, restore it back
								{
									bool selected;
									GetEnvelopePoint(envPtr, h,   NULL, NULL, NULL, NULL, &selected);
									SetEnvelopePoint(envPtr, h-1, NULL, NULL, NULL, NULL, &selected, NULL);
								}
								++pointsDeleted;
							}
						}
					}
				}

				// Make sure there is tempo marker at next grid division (but if there are no more tempo markers after current tempo marker, skip this)
				bool addedNextGridPoint = false;
				if (id != CountTempoTimeSigMarkers(NULL) - 1)
				{
					int nextId = id + 1;

					double nextPosition;
					double nextGridDiv = TimeMap_QNToTime_abs(NULL, nextGridDivQN);
					if (!GetTempoTimeSigMarker(NULL, nextId, &nextPosition, NULL, NULL, NULL, NULL, NULL, NULL) || !IsEqual(nextGridDiv, nextPosition, GRID_DIV_DELTA))
					{
						int nextMeasure;
						double nextBeats = TimeMap2_timeToBeats(NULL, nextGridDiv, &nextMeasure, NULL, NULL, NULL);
						InsertStretchMarkerInAllItems(nextGridDiv, true, GRID_DIV_DELTA / 2);
						if (SetTempoTimeSigMarker(NULL, -1, -1, nextMeasure, nextBeats, nextGridDivBpm, 0, 0, g_envMouseEnvelope->GetDefaultShape() == LINEAR))
						{
							pointsEdited       = true;
							addedNextGridPoint = true;
							for (int j = CountTempoTimeSigMarkers(NULL) - 2; j >= nextId; --j) // adding points with SetTempoTimeSigMarker(id = -1) shifts selection, restore it back
							{
								bool selected;
								GetEnvelopePoint(envPtr, j,   NULL, NULL, NULL, NULL, &selected);
								SetEnvelopePoint(envPtr, j+1, NULL, NULL, NULL, NULL, &selected, NULL);
							}

						}
					}
				}

				// Account for all the added and removed points (if breaking early, endIs is already set at the right id, so skip this)
				if (!breakEarly)
				{
					if (addedGridPoint)     ++pointsAdded;
					if (addedNextGridPoint) ++pointsAdded;
					pointsAdded -= pointsDeleted;
				}

				if (breakEarly)
					break;
			}
		}
		endId += pointsAdded;
	}
	// Just edit points (in case of normal envelope freehand drawing we already added/removed points so we just edit them here)
	else
	{
		int pointCount = (isTempo) ? CountTempoTimeSigMarkers(NULL) : g_envMouseEnvelope->CountPoints();
		if (CheckBoundsEx(startId, -1, pointCount) && CheckBoundsEx(endId, -1, pointCount))
		{
			// Change points values
			for (int i = startId; i <= endId; ++i)
			{
				// Get current point's position
				double currentPointPos;
				if (isTempo)
				{
					GetTempoTimeSigMarker(NULL, i, &currentPointPos, NULL, NULL, NULL, NULL, NULL, NULL);

					// If going forward, make sure edited tempo points do not come in front of mouse cursor (if so, leave them for the next action call)
					if (!oppositeDirection && endId != startId && !CheckBounds(currentPointPos, startPosition, endPosition))
					{
						endId       = i;
						endPosQN    = TimeMap_timeToQN_abs(NULL, currentPointPos);
						endPosition = currentPointPos;
						endDisplayVal = (i == startId)
						              ? startDisplayVal
						              : startDisplayVal + (endDisplayVal - startDisplayVal) * ((currentPointPos - startPosition) / (endPosition - startPosition));
						break;
					}
				}
				else
					g_envMouseEnvelope->GetPoint(i, &currentPointPos, NULL, NULL, NULL);

				// Update current point
				double newDisplayVal = (i == startId)
				                     ? startDisplayVal
				                     : startDisplayVal + (endDisplayVal - startDisplayVal) * ((currentPointPos - startPosition) / (endPosition - startPosition));
				double newValue = g_envMouseEnvelope->RealValue(newDisplayVal);
				if (isTempo)
				{
					int measure, num, den; double position, beat; bool linear;
					GetTempoTimeSigMarker(NULL, i, &position, &measure, &beat, NULL, &num, &den, &linear);
					InsertStretchMarkerInAllItems(position, true, GRID_DIV_DELTA / 2);
					if (den != 0) // make sure any partial tempo markers don't loose their position
					{
						double positionQN = TimeMap_timeToQN_abs(NULL, position);
						if (SetTempoTimeSigMarker(NULL, i, position, -1, -1, newValue, num, den, linear))
						{
							if (SetTempoTimeSigMarker(NULL, i, TimeMap_QNToTime_abs(NULL, positionQN), -1, -1, newValue, num, den, linear))
								pointsEdited = true;
						}
					}
					else
					{
						if (SetTempoTimeSigMarker(NULL, i, -1, measure, beat, newValue, num, den, linear))
							pointsEdited = true;
					}
				}
				else
				{
					// When dealing with MUTE, snap to top or bottom
					if (g_envMouseEnvelope->Type() == MUTE)
						newValue = (newValue >= g_envMouseEnvelope->LaneCenterValue()) ? g_envMouseEnvelope->LaneMaxValue() : g_envMouseEnvelope->LaneMinValue();

					if (g_envMouseEnvelope->SetPoint(i, NULL, &newValue, NULL, NULL, false, true))
					{
						pointsEdited = true;
						if (user == 3) // in case of freehand drawing, selected edited point
							g_envMouseEnvelope->SetSelection(i, true);
					}
				}
			}
		}
	}

	if (pointsEdited)
	{
		if (isTempo)
		{
			endPosition = TimeMap_QNToTime_abs(NULL, endPosQN);
			UpdateTimeline();
		}
		else
			g_envMouseEnvelope->Commit();
		g_envMouseDidOnce = true;
	}

	// Swap values back before storing them
	if (oppositeDirection)
	{
		swap(endId, startId);
		swap(endPosition,   startPosition);
		swap(endDisplayVal, startDisplayVal);
	}
	s_lastEndId       = endId;
	s_lastEndPosition = endPosition;
	s_lastEndNormVal  = endDisplayVal;

	PreventUIRefresh(-1);
}

void SetEnvPointMouseValueInit ()
{
	//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
	static COMMAND_T s_commandTable[] =
	{
		{ { DEFACCEL, "SWS/BR: Set closest envelope point's value to mouse cursor (perform until shortcut released)" },                  "BR_ENV_PT_VAL_CLOSEST_MOUSE",          SetEnvPointMouseValue, NULL, 1},
		{ { DEFACCEL, "SWS/BR: Set closest left side envelope point's value to mouse cursor (perform until shortcut released)" },        "BR_ENV_PT_VAL_CLOSEST_LEFT_MOUSE",     SetEnvPointMouseValue, NULL, 2},
		{ { DEFACCEL, "SWS/BR: Freehand draw envelope while snapping points to left side grid line (perform until shortcut released)" }, "BR_CONT_ENV_FREEHAND_SNAP_GRID_MOUSE", SetEnvPointMouseValue, NULL, 3},

		{ { DEFACCEL, "SWS/BR: Select envelope at mouse cursor and set closest envelope point's value to mouse cursor (perform until shortcut released)" },                  "BR_ENV_PT_VAL_CLOSEST_MOUSE_SEL_ENV",          SetEnvPointMouseValue, NULL, -1},
		{ { DEFACCEL, "SWS/BR: Select envelope at mouse cursor and set closest left side envelope point's value to mouse cursor (perform until shortcut released)" },        "BR_ENV_PT_VAL_CLOSEST_LEFT_MOUSE_SEL_ENV",     SetEnvPointMouseValue, NULL, -2},
		{ { DEFACCEL, "SWS/BR: Select envelope at mouse cursor and freehand draw envelope while snapping points to left side grid line (perform until shortcut released)" }, "BR_CONT_ENV_FREEHAND_SNAP_GRID_MOUSE_SEL_ENV", SetEnvPointMouseValue, NULL, -3},
		{ {}, LAST_COMMAND}
	};
	//!WANT_LOCALIZE_1ST_STRING_END

	int i = -1;
	while (s_commandTable[++i].id != LAST_COMMAND)
		ContinuousActionRegister(new BR_ContinuousAction(&s_commandTable[i], EnvMouseInit, EnvMouseUndo, EnvMouseCursor, EnvMouseTooltip));
}

/******************************************************************************
* Commands: Envelopes - Misc                                                  *
******************************************************************************/
void CursorToEnv1 (COMMAND_T* ct)
{
	TrackEnvelope* envelope = GetSelectedEnvelope(NULL);
	if (!envelope)
		return;
	char* envState = GetSetObjectState(envelope, "");
	char* token = strtok(envState, "\n");

	bool found = false;
	double pTime = 0, cTime = 0, nTime = 0;
	double cursor = GetCursorPositionEx(NULL);
	double takeEnvStartPos = (envelope == GetSelectedTrackEnvelope(NULL)) ? 0 : GetMediaItemInfo_Value(GetMediaItemTake_Item(GetTakeEnvParent(envelope, NULL)), "D_POSITION");
	cursor -= takeEnvStartPos;

	// Find previous
	if ((int)ct->user > 0)
	{
		while (token != NULL)
		{
			if (sscanf(token, "PT %20lf", &cTime))
			{
				if (cTime > cursor)
				{
					// fake next point if it doesn't exist
					token = strtok(NULL, "\n");
					if (!sscanf(token, "PT %20lf", &nTime))
						nTime = cTime + 0.001;

					found = true;
					break;
				}
				pTime = cTime;
			}
			token = strtok(NULL, "\n");
		}
	}

	// Find next
	else
	{
		double ppTime = -1;
		while (token != NULL)
		{
			if (sscanf(token, "PT %20lf", &cTime))
			{
				if (cTime >= cursor)
				{
					if (ppTime != -1)
					{
						found = true;
						nTime = cTime;
						cTime = pTime;
						pTime = ppTime;
						if (nTime == 0)
							nTime = cTime + 0.001;
					}
					break;
				}
				ppTime = pTime;
				pTime = cTime;
			}
			token = strtok(NULL, "\n");
		}

		// Fix for a case when cursor is after the last point
		if (!found && cursor > cTime)
		{
			found = true;
			nTime = cTime;
			cTime = pTime;
			pTime = ppTime;
			if (nTime == 0)
				nTime = cTime + 0.001;
		}
	}

	// Yes, this is a hack...however, it's faster than modifying an envelope
	// chunk. With this method chunk is parsed until target point is found, and
	// then via manipulation of the preferences and time selection, reaper does
	// the rest. Another good thing is that we escape envelope redrawing
	// issues: http://forum.cockos.com/project.php?issueid=4416
	if (found)
	{
		Undo_BeginBlock2(NULL);

		// Select point
		if (((int)ct->user == -2 || (int)ct->user == 2) && !IsLocked(TRACK_ENV))
		{
			PreventUIRefresh(1);

			// Get current time selection
			double tStart, tEnd, nStart, nEnd;
			GetSet_LoopTimeRange2(NULL, false, false, &tStart, &tEnd, false);

			// Enable selection of points in time selection
			ConfigVar<int> envClickSegMode("envclicksegmode");
			const int oldEnvClickSegMode = envClickSegMode.value_or(0);
			ConfigVarOverride<int> tmpEnvClickSegMode("envclicksegmode",
				SetBit(oldEnvClickSegMode, 6));

			// Set time selection that in turn sets new envelope selection
			nStart = (cTime + pTime) / 2 + takeEnvStartPos;
			nEnd = (nTime + cTime) / 2 + takeEnvStartPos;
			GetSet_LoopTimeRange2(NULL, true, false, &nStart, &nEnd, false);

			// Preserve point when previous time selection gets restored
			envClickSegMode.try_set(ClearBit(oldEnvClickSegMode, 6));
			GetSet_LoopTimeRange2(NULL, true, false, &tStart, &tEnd, false);

			PreventUIRefresh(-1);
		}

		SetEditCurPos(cTime + takeEnvStartPos, true, false);
		Undo_EndBlock2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG | UNDO_STATE_ITEMS |UNDO_STATE_MISCCFG);
	}
	FreeHeapPtr(envState);
}

void CursorToEnv2 (COMMAND_T* ct)
{
	BR_Envelope envelope(GetSelectedEnvelope(NULL));
	if (!envelope.CountPoints())
		return;

	int id;
	if ((int)ct->user > 0)
		id = envelope.FindNext(GetCursorPositionEx(NULL));
	else
		id = envelope.FindPrevious(GetCursorPositionEx(NULL));

	double targetPos;
	if (envelope.GetPoint(id, &targetPos, NULL, NULL, NULL))
	{
		// Select all points at the same time position
		double position;
		while (envelope.GetPoint(id, &position, NULL, NULL, NULL))
		{
			if (targetPos == position)
			{
				envelope.SetSelection(id, true);
				if ((int)ct->user > 0) {++id;} else {--id;}
			}
			else
				break;
		}

		SetEditCurPos(targetPos, true, false);
		envelope.Commit();
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG | UNDO_STATE_ITEMS |UNDO_STATE_MISCCFG, -1);
	}
}

void SelNextPrevEnvPoint (COMMAND_T* ct)
{
	BR_Envelope envelope(GetSelectedEnvelope(NULL));
	if (!envelope.CountSelected())
		return;

	int id;
	if ((int)ct->user > 0) id = envelope.GetSelected(envelope.CountSelected()-1) + 1;
	else                   id = envelope.GetSelected(0) - 1;

	if (envelope.ValidateId(id))
	{
		envelope.UnselectAll();
		envelope.SetSelection(id, true);

		if (envelope.Commit())
		{
			double pos, prevPos;
			envelope.GetPoint(id, &pos, NULL, NULL, NULL);
			envelope.GetPoint(((int)ct->user > 0) ? (id-1) : (id+1), &prevPos, NULL, NULL, NULL);
			MoveArrangeToTarget(pos, prevPos);

			Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG | UNDO_STATE_ITEMS, -1);
		}
	}
}

void ExpandEnvSel (COMMAND_T* ct)
{
	BR_Envelope envelope(GetSelectedEnvelope(NULL));
	if (!envelope.CountSelected())
		return;

	int id = -1;
	if ((int)ct->user > 0)
	{
		for (int i = 0; i < envelope.CountConseq(); ++i)
		{
			envelope.GetConseq(i, NULL, &id);
			envelope.SetSelection(id + 1, true);
		}
	}
	else
	{
		for (int i = 0; i < envelope.CountConseq(); ++i)
		{
			envelope.GetConseq(i, &id, NULL);
			envelope.SetSelection(id - 1, true);
		}
	}

	if (envelope.Commit())
	{
		if (envelope.CountConseq() == 1)
			envelope.MoveArrangeToPoint (id, ((int)ct->user > 0) ? (id-1) : (id+1));
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG | UNDO_STATE_ITEMS, -1);
	}
}

void ExpandEnvSelEnd (COMMAND_T* ct)
{
	BR_Envelope envelope(GetSelectedEnvelope(NULL));
	if (!envelope.CountSelected())
		return;

	int id;
	if ((int)ct->user > 0) id = envelope.GetSelected(envelope.CountSelected() - 1) + 1;
	else                   id = envelope.GetSelected(0) - 1;

	envelope.SetSelection(id, true);
	if (envelope.Commit())
	{
		envelope.MoveArrangeToPoint(id, ((int)ct->user > 0) ? (id-1) : (id+1));
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG | UNDO_STATE_ITEMS, -1);
	}
}

void ShrinkEnvSel (COMMAND_T* ct)
{
	BR_Envelope envelope(GetSelectedEnvelope(NULL));
	if (!envelope.CountSelected())
		return;

	int id = -1;
	if ((int)ct->user > 0)
	{
		for (int i = 0; i < envelope.CountConseq(); ++i)
		{
			envelope.GetConseq(i, NULL, &id);
			envelope.SetSelection(id, false);
		}
	}
	else
	{
		for (int i = 0; i < envelope.CountConseq(); ++i)
		{
			envelope.GetConseq(i, &id, NULL);
			envelope.SetSelection(id, false);
		}
	}

	if (envelope.Commit())
	{
		if (envelope.CountConseq() == 1)
			envelope.MoveArrangeToPoint (((int)ct->user > 0) ? (id-1) : (id+1), id);
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG | UNDO_STATE_ITEMS, -1);
	}
}

void ShrinkEnvSelEnd (COMMAND_T* ct)
{
	BR_Envelope envelope(GetSelectedEnvelope(NULL));
	if (!envelope.CountSelected())
		return;

	int id;
	if ((int)ct->user > 0) id = envelope.GetSelected(envelope.CountSelected() - 1);
	else                   id = envelope.GetSelected(0);

	envelope.SetSelection(id, false);
	if (envelope.Commit())
	{
		envelope.MoveArrangeToPoint (((int)ct->user > 0) ? (id-1) : (id+1), id);
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG | UNDO_STATE_ITEMS, -1);
	}
}

void EnvPointsGrid (COMMAND_T* ct)
{
	BR_Envelope envelope(GetSelectedEnvelope(NULL));
	if (!envelope.CountPoints())
		return;

	// Get options and the range to work on
	bool timeSel      = !!GetBit((int)ct->user, 0);
	bool deletePoints = !!GetBit((int)ct->user, 1);
	bool unselect     = !!GetBit((int)ct->user, 2);
	bool onGrid       = !!GetBit((int)ct->user, 3);

	int startId, endId;
	if (!timeSel || !envelope.GetPointsInTimeSelection(&startId, &endId))
	{
		startId = 0;
		endId   = envelope.CountPoints()-1;
	}
	else if (!envelope.ValidateId(startId) || !envelope.ValidateId(endId))
		return;

	if (unselect)
		envelope.UnselectAll();

	// Select/insert or save for deletion all the points satisfying criteria
	double grid = -1, nextGrid = -1;
	vector<pair<int, int> > pointsToDelete; // store start and end of consequential points
	for (int i = startId ; i <= endId; ++i)
	{
		double position;
		envelope.GetPoint(i, &position, NULL, NULL, NULL);

		while (position > nextGrid)
		{
			grid     = GetNextGridDiv(position - (MAX_GRID_DIV/2));
			nextGrid = GetNextGridDiv(grid);
		}

		bool doesPointPass = (onGrid) ? (IsEqual(position, grid, GRID_DIV_DELTA)  ||  IsEqual(position, nextGrid, GRID_DIV_DELTA))
									  : (!IsEqual(position, grid, GRID_DIV_DELTA) && !IsEqual(position, nextGrid, GRID_DIV_DELTA));

		if (doesPointPass)
		{
			if (deletePoints)
			{
				if (pointsToDelete.size() == 0 || pointsToDelete.back().second != i -1)
					pointsToDelete.push_back(make_pair(i, i));
				pointsToDelete.back().second = i;
			}
			else
				envelope.SetSelection(i, true);
		}
	}

	// Readjust tempo points
	if (deletePoints && pointsToDelete.size() > 0)
	{
		const int timeBase = ConfigVar<int>("tempoenvtimelock").value_or(0);
		if ((envelope.IsTempo() && timeBase != 0))
		{
			double offset = 0;
			for (size_t i = 0; i < pointsToDelete.size(); ++i)
			{
				int conseqStart = pointsToDelete[i].first;
				int conseqEnd   = pointsToDelete[i].second;
				if (conseqStart == 0 && (++conseqStart > conseqEnd)) continue; // skip first point

				double t0, t1, b0, b1; int s0;
				envelope.GetPoint(conseqStart - 1, &t0, &b0, &s0, NULL);

				if (envelope.GetPoint(conseqEnd + 1, &t1, &b1, NULL, NULL))
				{
					t0 -= offset; // last unselected point before next selection - readjust position to original (earlier iterations moved it)

					int startMeasure, endMeasure, num, den;
					double startBeats = TimeMap2_timeToBeats(NULL, t0, &startMeasure, &num, NULL, &den);
					double endBeats   = TimeMap2_timeToBeats(NULL, t1, &endMeasure, NULL, NULL, NULL);
					double beatCount = endBeats - startBeats + num * (endMeasure - startMeasure);

					if (s0 == SQUARE)
						offset += (t0 + (240*beatCount) / (den * b0))        - t1;  // num and den can be different due to some time signatures being scheduled for deletion but reaper does it in the same manner so leave it
					else
						offset += (t0 + (480*beatCount) / (den * (b0 + b1))) - t1;

					int nextPoint = (i == pointsToDelete.size() - 1) ? envelope.CountPoints() : pointsToDelete[i+1].first;
					while (++conseqEnd < nextPoint)
					{
						double t;
						envelope.GetPoint(conseqEnd, &t, NULL, NULL, NULL);
						t += offset;
						envelope.SetPoint(conseqEnd, &t, NULL, NULL, NULL);
					}
				}
			}
		}

		int idOffset = 0;
		for (size_t i = 0; i < pointsToDelete.size(); ++i)
		{
			int conseqStart = pointsToDelete[i].first;
			int conseqEnd   = pointsToDelete[i].second;
			if (envelope.IsTempo() && conseqStart == 0 && (++conseqStart > conseqEnd)) continue; // skip first tempo point

			conseqStart -= idOffset;
			conseqEnd   -= idOffset;
			if (envelope.DeletePoints(conseqStart, conseqEnd))
				idOffset += conseqEnd - conseqStart + 1;
		}
	}

	if (envelope.Commit())
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG | UNDO_STATE_ITEMS, -1);
}

void CreateEnvPointsGrid (COMMAND_T* ct)
{
	BR_Envelope envelope(GetSelectedEnvelope(NULL));
	if (!envelope.CountPoints())
		return;

	bool timeSel = ((int)ct->user == 1) ? true : false;
	int startId, endId;
	double tStart, tEnd;
	if (!timeSel || !envelope.GetPointsInTimeSelection(&startId, &endId, &tStart, &tEnd))
	{
		startId = 0;
		endId   = envelope.CountPoints()-1;
		tStart  = 0;
		tEnd    = EndOfProject(true, true);
	}
	else
	{
		if (!envelope.ValidateId(startId))
		{
			int newId = envelope.FindPrevious(tStart);
			startId = (envelope.ValidateId(newId)) ? (newId) : (0);
		}
		endId = (envelope.ValidateId(endId)) ? (endId) : (startId);
	}

	vector<double> position, value, bezier;
	vector<int> shape;
	bool doPointsBeforeFirstPoint = true;
	for (int i = startId; i <= endId; ++i)
	{
		double t0, t1, v0, b0; int s0;

		envelope.GetPoint(i, &t0, &v0, &s0, &b0);
		if (!envelope.GetPoint(i+1, &t1, NULL, NULL, NULL))
			t1 = tEnd;
		else
			t1 = (t1 < tEnd) ? t1 : tEnd;

		// If first point is not right at the start, make sure points are created before it
		if (i == startId)
		{
			if (t0 > tStart && doPointsBeforeFirstPoint)
			{
				t1 = t0;
				t0 = 0;
				s0 = envelope.GetDefaultShape();
				b0 = 0;
				--i; // make loop process first point one more time, creating points AFTER first point
			}
			doPointsBeforeFirstPoint = false;
		}

		double previousGridLine = t0;
		double gridLine = t0;
		while (true)
		{
			while (gridLine < previousGridLine + MAX_GRID_DIV)
				gridLine = GetNextGridDiv(gridLine + (MAX_GRID_DIV/2));

			// Make sure points are created after time selection only
			if (gridLine >= tStart)
			{
				if (gridLine <= t1 - (MAX_GRID_DIV/2) || (timeSel && i == endId && gridLine <= t1))
				{
					position.push_back(gridLine);
					value.push_back(envelope.ValueAtPosition(gridLine, true)); // ValueAtPosition can be much faster when dealing with sorted
					shape.push_back(s0);                                       // points and inserting points will make envelope unsorted
					bezier.push_back(b0);
				}
				else
					break;
			}
			previousGridLine = gridLine;
		}
	}

	for (size_t i = 0; i < position.size(); ++i)
		envelope.CreatePoint(envelope.CountPoints(), position[i], value[i], shape[i], bezier[i], false, true);

	bool stretchMarkersInserted = ((envelope.IsTempo()) ? InsertStretchMarkersInAllItems(position) : false);
	if (envelope.Commit() || stretchMarkersInserted)
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG | UNDO_STATE_ITEMS, -1);
}

void EnvPointsToCC (COMMAND_T* ct)
{
	ME_EnvPointsToCC (ct, 0, 0, 0, NULL);
}

void ShiftEnvSelection (COMMAND_T* ct)
{
	BR_Envelope envelope(GetSelectedEnvelope(NULL));
	if (!envelope.CountSelected())
		return;

	if ((int)ct->user < 0)
	{
		for (int i = 0; i < envelope.CountSelected(); ++i)
		{
			int id = envelope.GetSelected(i);
			envelope.SetSelection(id-1, true);
			envelope.SetSelection(id,   false);
		}
	}
	else
	{
		for (int i = envelope.CountSelected()-1; i >= 0; --i)
		{
			int id = envelope.GetSelected(i);
			envelope.SetSelection(id+1, true);
			envelope.SetSelection(id,   false);
		}
	}

	if (envelope.Commit())
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG | UNDO_STATE_ITEMS, -1);
}

void PeaksDipsEnv (COMMAND_T* ct)
{
	BR_Envelope envelope(GetSelectedEnvelope(NULL));
	if (!envelope.CountPoints())
		return;

	if ((int)ct->user == -2 || (int)ct->user == 2)
		envelope.UnselectAll();

	for (int i = 1; i < envelope.CountPoints()-1 ; ++i)
	{
		double v0, v1, v2;
		envelope.GetPoint(i-1, NULL, &v0, NULL, NULL);
		envelope.GetPoint(i  , NULL, &v1, NULL, NULL);
		envelope.GetPoint(i+1, NULL, &v2, NULL, NULL);

		if ((int)ct->user < 0)
		{
			if (v1 < v0 && v1 < v2)
				envelope.SetSelection(i, true);
		}
		else
		{
			if (v1 > v0 && v1 > v2)
				envelope.SetSelection(i, true);
		}
	}

	if (envelope.Commit())
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG | UNDO_STATE_ITEMS, -1);
}

void SelEnvTimeSel (COMMAND_T* ct)
{
	double tStart, tEnd;
	GetSet_LoopTimeRange2(NULL, false, false, &tStart, &tEnd, false);
	if (tStart == tEnd)
		return;

	BR_Envelope envelope(GetSelectedEnvelope(NULL));
	if (!envelope.CountPoints())
		return;

	for (int i = 0; i < envelope.CountPoints() ; ++i)
	{
		double point;
		envelope.GetPoint(i, &point, NULL, NULL, NULL);

		if ((int)ct->user < 0)
		{
			if (point < tStart || point > tEnd)
				envelope.SetSelection(i, false);
		}
		else
		{
			if (point >= tStart && point <= tEnd)
				envelope.SetSelection(i, false);
		}
	}
	if (envelope.Commit())
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG | UNDO_STATE_ITEMS, -1);
}

void SetEnvValToNextPrev (COMMAND_T* ct)
{
	BR_Envelope envelope(GetSelectedEnvelope(NULL));
	if (!envelope.CountSelected())
		return;

	// Next/previous point
	if (abs((int)ct->user) == 1)
	{
		for (int i = 0; i < envelope.CountConseq(); ++i)
		{
			int start, end;	envelope.GetConseq(i, &start, &end);
			int referenceId = ((int)ct->user > 0) ? (end+1) : (start-1);

			double newVal;
			if (envelope.GetPoint(referenceId, NULL, &newVal, NULL, NULL))
			{
				for (int i = start; i <= end; ++i)
					envelope.SetPoint(i, NULL, &newVal, NULL, NULL);
			}
		}
	}
	// First/last selected point
	else
	{
		int referenceId = envelope.GetSelected(((int)ct->user < 0) ? 0 : envelope.CountSelected()-1);
		double newVal; envelope.GetPoint(referenceId, NULL, &newVal, NULL, NULL);

		for (int i = 0; i < envelope.CountSelected(); ++i)
			envelope.SetPoint(envelope.GetSelected(i), NULL, &newVal, NULL, NULL);
	}

	if (envelope.Commit())
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG | UNDO_STATE_ITEMS, -1);
}

void MoveEnvPointToEditCursor (COMMAND_T* ct)
{
	double cursor = GetCursorPositionEx(NULL);
	int id = -1;

	BR_Envelope envelope(GetSelectedEnvelope(NULL));
	if (!envelope.CountPoints())
		return;

	// Find closest
	if ((int)ct->user == 0)
	{
		id = envelope.FindClosest(cursor);
		if (!envelope.ValidateId(id))
			return;
	}

	// Find closest selected
	else
	{
		id = envelope.GetSelected(0);
		double currentLen;
		if (!envelope.GetPoint(id, &currentLen, NULL, NULL, NULL))
			return;
		currentLen = abs(currentLen - cursor);

		for (int i = 0; i < envelope.CountSelected(); ++i)
		{
			int currentId = envelope.GetSelected(i);
			double currentPos;
			envelope.GetPoint(currentId, &currentPos, NULL, NULL, NULL);

			double len = abs(cursor - currentPos);
			if (len < currentLen)
			{
				currentLen = len;
				id = currentId;
			}
		}
	}

	if (id != -1)
	{
		double position;
		envelope.GetPoint(id, &position, NULL, NULL, NULL);
		if (position != cursor)
		{
			envelope.SetPoint(id, &cursor, NULL, NULL, NULL, true);
			if (envelope.Commit())
				Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG | UNDO_STATE_ITEMS, -1);
		}
	}
}

void Insert2EnvPointsTimeSelection (COMMAND_T* ct)
{
	bool selEnvOnly   = ((int)ct->user == 0) ? true : false;
	bool selTrackOnly = ((int)ct->user == 2) ? true : false;

	double tStart, tEnd;
	GetSet_LoopTimeRange2(NULL, false, false, &tStart, &tEnd, false);

	if (tStart + MIN_ENV_DIST >= tEnd)
		return;

	bool update = false;

	PreventUIRefresh(1);
	int trackCount = (selEnvOnly) ? (1) : (GetNumTracks() + 1);
	for (int i = 0; i < trackCount; i++)
	{
		MediaTrack* track = CSurf_TrackFromID(i, false);
		if (selTrackOnly && !(int)GetMediaTrackInfo_Value(track, "I_SELECTED"))
			track = NULL;

		if (track)
		{
			int envelopeCount = selEnvOnly ? 1: CountTrackEnvelopes(track);
			vector<double> stretchMarkers;
			for (int j = 0; j < envelopeCount; ++j)
			{
				BR_Envelope envelope(selEnvOnly ? GetSelectedEnvelope(NULL) : GetTrackEnvelope(track, j));

				if (envelope.IsVisible())
				{
					int startId = envelope.Find(tStart, MIN_ENV_DIST);
					int endId   = envelope.Find(tEnd, MIN_ENV_DIST);
					int defaultShape = envelope.GetDefaultShape();
					envelope.UnselectAll();

					// Create left-side point only if surrounding points are not too close, otherwise just move existing
					if (envelope.ValidateId(startId))
					{
						double position; envelope.GetPoint(startId, &position, NULL, NULL, NULL);
						if (envelope.SetPoint(startId, &tStart, NULL, &defaultShape, 0, true))
						{
							stretchMarkers.push_back(position);
							envelope.SetSelection(startId, true);
						}
					}
					else
					{
						if (envelope.CreatePoint(envelope.CountPoints(), tStart, envelope.ValueAtPosition(tStart), defaultShape, 0, true, true) && envelope.IsTempo())
							stretchMarkers.push_back(tStart);

					}

					// Create right-side point only if surrounding points are not too close, otherwise just move existing
					if (envelope.ValidateId(endId))
					{
						double position; envelope.GetPoint(endId, &position, NULL, NULL, NULL);
						if (envelope.SetPoint(endId, &tEnd, NULL, &defaultShape, 0, true))
						{
							stretchMarkers.push_back(position);
							envelope.SetSelection(endId, true);
						}
					}
					else
					{
						if (envelope.CreatePoint(envelope.CountPoints(), tEnd, envelope.ValueAtPosition(tEnd), defaultShape, 0, true, true) && envelope.IsTempo())
							stretchMarkers.push_back(tEnd);
					}

					if (envelope.IsTempo() && InsertStretchMarkersInAllItems(stretchMarkers))
						update = true;

					if (envelope.Commit())
						update = true;
				}
			}
		}
	}
	PreventUIRefresh(-1);

	if (update)
	{
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_MISCCFG | UNDO_STATE_TRACKCFG | UNDO_STATE_ITEMS, -1);
		UpdateArrange();
	}
}

void CopyEnvPoints (COMMAND_T* ct)
{
	BR_Envelope envelope(GetSelectedEnvelope(NULL));

	bool recordArm      = !!GetBit((int)ct->user, 0);
	bool doTimeSel      = !!GetBit((int)ct->user, 1);
	bool fromCursor     = !!GetBit((int)ct->user, 2);
	bool copyToMouseEnv = !!GetBit((int)ct->user, 3);

	double startTime = 0;
	double endTime   = 0;
	vector<int> idsToCopy;

	if (doTimeSel)
	{
		double tStart, tEnd;
		GetSet_LoopTimeRange2(NULL, false, false, &tStart, &tEnd, false);

		if (tStart != tEnd)
		{
			int startId = envelope.FindPrevious(tStart) + 1;
			int endId   = envelope.FindNext(tEnd)       - 1;

			if (envelope.ValidateId(startId) && envelope.ValidateId(endId))
			{
				for (int i = startId; i <= endId; ++i)
					idsToCopy.push_back(i);
			}
		}
		startTime = tStart;
		endTime   = tEnd;
	}
	else
	{
		for (int i = 0; i < envelope.CountSelected(); ++i)
			idsToCopy.push_back(envelope.GetSelected(i));

		if (idsToCopy.size())
		{
			envelope.GetPoint(idsToCopy.front(), &startTime, NULL, NULL, NULL);
			envelope.GetPoint(idsToCopy.back(),  &endTime,   NULL, NULL, NULL);
		}
	}

	if (idsToCopy.size() == 0)
		return;

	double positionOffset = 0;
	if (fromCursor)
	{
		double position;
		envelope.GetPoint(idsToCopy.front(), &position, NULL, NULL, NULL);
		positionOffset = GetCursorPositionEx(NULL) - position;

		startTime += positionOffset;
		endTime   += positionOffset;
	}


	vector<TrackEnvelope*> envelopes;
	if (copyToMouseEnv)
	{
		BR_MouseInfo mouseInfo(BR_MouseInfo::MODE_MCP_TCP | BR_MouseInfo::MODE_ARRANGE | BR_MouseInfo::MODE_IGNORE_ENVELOPE_LANE_SEGMENT);

		if ((!strcmp(mouseInfo.GetWindow(), "tcp") && !strcmp(mouseInfo.GetSegment(), "envelope")) ||
			(!strcmp(mouseInfo.GetWindow(), "arrange") && !strcmp(mouseInfo.GetSegment(), "envelope")) ||
			(!strcmp(mouseInfo.GetDetails(), "env_point") || !strcmp(mouseInfo.GetDetails(), "env_segment"))
			)
		{
			if (mouseInfo.GetEnvelope())
				envelopes.push_back(mouseInfo.GetEnvelope());
		}
	}
	else
	{
		const int trSelCnt = CountSelectedTracks(NULL);
		for (int i = -1; i < trSelCnt; ++i)
		{
			MediaTrack* track = NULL;
			if (i == -1 && *(int*)GetSetMediaTrackInfo(GetMasterTrack(NULL), "I_SELECTED", NULL))
				track = GetMasterTrack(NULL);
			else
				track = GetSelectedTrack(NULL, i);

			for (int j = 0; j < CountTrackEnvelopes(track); ++j)
			{
				TrackEnvelope* envPtr = GetTrackEnvelope(track, j);
				if ((positionOffset != 0 || envPtr != envelope.GetPointer()) && envPtr != GetTempoEnv())
					envelopes.push_back(envPtr);
			}
		}
	}

	PreventUIRefresh(1);
	bool update = false;
	bool triedToCopyToTempo = false;
	for (size_t i = 0; i < envelopes.size(); ++i)
	{
		TrackEnvelope* envPtr = envelopes[i];;
		if ((positionOffset != 0 || envPtr != envelope.GetPointer()) && envPtr != GetTempoEnv())
		{
			BR_Envelope targetEnv(envPtr);

			if (targetEnv.IsVisible() && (!recordArm || targetEnv.IsArmed()))
			{
				targetEnv.UnselectAll();
				targetEnv.DeletePointsInRange(startTime, endTime);

				for (size_t h = 0; h < idsToCopy.size(); ++h)
				{
					double position, value, bezier;
					int shape;
					envelope.GetPoint(idsToCopy[h], &position, &value, &shape, &bezier);
					targetEnv.CreatePoint(targetEnv.CountPoints(), position + positionOffset,
					                      targetEnv.RealValue(envelope.NormalizedDisplayValue(value)),
					                      shape, bezier, true, true, false);
				}
				if (targetEnv.Commit())
					update = true;
			}
		}
		else if (envPtr == GetTempoEnv())
		{
			BR_Envelope tempoMap(envPtr);
			if (tempoMap.IsVisible() && !recordArm)
				triedToCopyToTempo = true;
		}
	}
	PreventUIRefresh(-1);

	if (update)
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG | UNDO_STATE_ITEMS, -1);

	if (triedToCopyToTempo)
		MessageBox(g_hwndParent, __LOCALIZE("Can't copy to tempo map", "sws_mbox"), __LOCALIZE("SWS/BR - Warning", "sws_mbox"), MB_OK);
}

void FitEnvPointsToTimeSel (COMMAND_T* ct)
{
	double tStart, tEnd;
	GetSet_LoopTimeRange2(NULL, false, false, &tStart, &tEnd, false);

	BR_Envelope envelope(GetSelectedEnvelope(NULL));
	if (envelope.CountSelected() < 2)
		return;

	if (envelope.IsTakeEnvelope())
	{
		double itemStart = GetMediaItemInfo_Value(GetMediaItemTake_Item(envelope.GetTake()), "D_POSITION");
		double itemEnd   = GetMediaItemInfo_Value(GetMediaItemTake_Item(envelope.GetTake()), "D_LENGTH") + itemStart;
		tStart = SetToBounds(tStart, itemStart, itemEnd);
		tEnd   = SetToBounds(tEnd,   itemStart, itemEnd);
	}

	if (tStart + MIN_ENV_DIST >= tEnd)
		return;

	int firstId = envelope.GetSelected(0);
	int lastId  = envelope.GetSelected(envelope.CountSelected()-1);

	double firstPos; envelope.GetPoint(firstId, &firstPos, NULL, NULL, NULL);
	double lastPos;  envelope.GetPoint(lastId,  &lastPos,  NULL, NULL, NULL);

	for (int i = 0; i < envelope.CountSelected(); ++i)
	{
		int id = envelope.GetSelected(i);
		double position;
		envelope.GetPoint(id, &position, NULL, NULL, NULL);

		position = TranslateRange(position, firstPos, lastPos, tStart, tEnd);
		envelope.SetPoint(id, &position, NULL, NULL, NULL);
	}

	if (envelope.Commit())
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG | UNDO_STATE_ITEMS, -1);
}

void CreateEnvPointMouse (COMMAND_T* ct)
{
	BR_Envelope envelope(GetSelectedEnvelope(NULL));
	double position = PositionAtMouseCursor(false);

	// For take envelopes check cursor position here in case it gets snapped later
	if (envelope.IsTakeEnvelope())
	{
		double start = GetMediaItemInfo_Value(GetMediaItemTake_Item(envelope.GetTake()), "D_POSITION");
		double end   = start +  GetMediaItemInfo_Value(GetMediaItemTake_Item(envelope.GetTake()), "D_LENGTH");
		if (!CheckBounds(position, start, end))
			return;
	}

	if (position != -1 && envelope.VisibleInArrange(NULL, NULL))
	{
		position = SnapToGrid(NULL, position);
		double fudgeFactor = (envelope.IsTempo()) ? (MIN_TEMPO_DIST) : (MIN_ENV_DIST);
		if (!envelope.ValidateId(envelope.Find(position, fudgeFactor)))
		{
			double value = envelope.ValueAtPosition(position);
			if (envelope.IsTempo())
			{
				bool insertedStretchMarkers = InsertStretchMarkerInAllItems(position);
				if (SetTempoTimeSigMarker(NULL, -1, position, -1, -1, value, 0, 0, !envelope.GetDefaultShape()) || insertedStretchMarkers)
				{
					UpdateTimeline();
					Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG | UNDO_STATE_ITEMS | UNDO_STATE_MISCCFG, -1);
				}
			}
			else
			{
				envelope.CreatePoint(envelope.CountPoints(), position, value, envelope.GetDefaultShape(), 0, false, true);
				if (envelope.Commit())
					Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG | UNDO_STATE_ITEMS, -1);
			}
		}
	}
}

void IncreaseDecreaseVolEnvPoints (COMMAND_T* ct)
{
	BR_Envelope envelope (GetSelectedEnvelope(NULL));
	if (envelope.CountSelected() > 0 && (envelope.Type() == VOLUME || envelope.Type() == VOLUME_PREFX))
	{
		double trim = DB2VAL(((double)ct->user) / 10);
		for (int i = 0; i < envelope.CountSelected(); ++i)
		{
			double value;
			if (envelope.GetPoint(envelope.GetSelected(i), NULL, &value, NULL, NULL))
			{
				value = SetToBounds(value * trim, envelope.LaneMinValue(), envelope.LaneMaxValue());
				envelope.SetPoint(envelope.GetSelected(i), NULL, &value, NULL, NULL);
			}
		}
		if (envelope.Commit())
			Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG | UNDO_STATE_ITEMS, -1);
	}
}

void SelectEnvelopeUnderMouse (COMMAND_T* ct)
{
	BR_MouseInfo mouseInfo(BR_MouseInfo::MODE_MCP_TCP | BR_MouseInfo::MODE_ARRANGE | BR_MouseInfo::MODE_IGNORE_ENVELOPE_LANE_SEGMENT);

	if ((!strcmp(mouseInfo.GetWindow(),  "tcp")       && !strcmp(mouseInfo.GetSegment(), "envelope"))   ||
		(!strcmp(mouseInfo.GetWindow(),  "arrange")   && !strcmp(mouseInfo.GetSegment(), "envelope"))   ||
		(!strcmp(mouseInfo.GetDetails(), "env_point") || !strcmp(mouseInfo.GetDetails(), "env_segment"))
	)
	{
		if (mouseInfo.GetEnvelope() && mouseInfo.GetEnvelope() != GetSelectedEnvelope(NULL))
		{
			SetCursorContext(2, mouseInfo.GetEnvelope());
			UpdateArrange();
		}
	}
}

void SelectDeleteEnvPointUnderMouse (COMMAND_T* ct)
{
	BR_MouseInfo mouseInfo(BR_MouseInfo::MODE_ARRANGE);
	if (!strcmp(mouseInfo.GetDetails(), "env_point"))
	{
		if (mouseInfo.GetEnvelope() && (abs((int)ct->user) == 2 || (abs((int)ct->user) == 1 && mouseInfo.GetEnvelope() == GetSelectedEnvelope(NULL))))
		{
			BR_Envelope envelope(mouseInfo.GetEnvelope(), false);
			bool tempoMapEdited = false;

			if ((int)ct->user > 0 && !envelope.GetSelection(mouseInfo.GetEnvelopePoint()))
				envelope.SetSelection(mouseInfo.GetEnvelopePoint(), true);
			else if ((int)ct->user < 0)
			{
				if (envelope.IsTempo())
				{
					int id = mouseInfo.GetEnvelopePoint();
					if (id > 0 && DeleteTempoTimeSigMarker(NULL, id))
					{
						UpdateTimeline();
						tempoMapEdited = true;
					}
				}
				else
				{
					envelope.DeletePoint(mouseInfo.GetEnvelopePoint());
					if (envelope.CountPoints() == 0) // in case there are no more points left, envelope will get removed - so insert default point back
						envelope.CreatePoint(0, 0, envelope.LaneCenterValue(), envelope.GetDefaultShape(), 0, false); // position = 0 is why we created BR_Envelope with !takeEnvelopesUseProjectTime
				}
			}

			if (envelope.Commit() || tempoMapEdited)
				Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG, -1);
		}
	}
}

void UnselectEnvelope (COMMAND_T* ct)
{
	if (GetSelectedEnvelope(NULL))
	{
		HWND hwnd; int context;
		GetSetFocus(false, &hwnd, &context);

		SetCursorContext(2, NULL);
		GetSetFocus(true, &hwnd, &context);
		UpdateArrange();
	}
}

void ApplyNextCmdToMultiEnvelopes (COMMAND_T* ct)
{
	static bool s_reentrancy = false;
	static const int s_thisCommandCmdsCount = 4;
	static const int s_thisCommandCmds[s_thisCommandCmdsCount] = {NamedCommandLookup("_BR_NEXT_CMD_SEL_TK_VIS_ENVS"),
	                                                              NamedCommandLookup("_BR_NEXT_CMD_SEL_TK_REC_ENVS"),
	                                                              NamedCommandLookup("_BR_NEXT_CMD_SEL_TK_VIS_ENVS_NOSEL"),
	                                                              NamedCommandLookup("_BR_NEXT_CMD_SEL_TK_REC_ENVS_NOSEL")};

	if (s_reentrancy)
		return;

	int cmd = BR_GetNextActionToApply();
	if (cmd == 0)
		return;

	for (size_t i = 0; i < s_thisCommandCmdsCount; ++i)
	{
		if (cmd == s_thisCommandCmds[i])
			return;
	}

	if ((int)ct->user < 0)
	{
		if (GetSelectedTrackEnvelope(NULL))
		{
			s_reentrancy = true;
			Main_OnCommand(cmd, 0);
			s_reentrancy = false;
			return;
		}
	}

	TrackEnvelope* selectedEnvelope = GetSelectedEnvelope(NULL);
	bool recordArm = (abs((int)ct->user) == 2);
	bool updated   = false;

	PreventUIRefresh(1);
	Undo_BeginBlock2(NULL);
	const int trSelCnt = CountSelectedTracks(NULL);
	for (int i = -1; i < trSelCnt; ++i)
	{
		MediaTrack* track = NULL;
		if (i == -1 && *(int*)GetSetMediaTrackInfo(GetMasterTrack(NULL), "I_SELECTED", NULL))
			track = GetMasterTrack(NULL);
		else
			track = GetSelectedTrack(NULL, i);

		for (int j = 0; j < CountTrackEnvelopes(track); ++j)
		{
			BR_Envelope envelope(GetTrackEnvelope(track, j));
			if (envelope.IsVisible() && (!recordArm || envelope.IsArmed()))
			{
				SetCursorContext(2, envelope.GetPointer());
				updated = true;

				s_reentrancy = true;
				Main_OnCommand(cmd, 0);
				s_reentrancy = false;
			}
		}

	}
	if (updated)
	{
		if (selectedEnvelope)
			SetCursorContext(2, selectedEnvelope);
		else
			SetCursorContext(1, NULL);
	}
	PreventUIRefresh(-1);

	WDL_FastString undoMsg;
	undoMsg.AppendFormatted(256, "%s %s", (((int)ct->user == 0) ? __LOCALIZE("Apply to visible envelopes of selected tracks:", "sws_undo") : __LOCALIZE("Apply to visible record-armed envelopes of selected tracks:", "sws_undo")), kbd_getTextFromCmd(cmd, NULL));
	Undo_EndBlock2(NULL, undoMsg.Get(), UNDO_STATE_ALL);
}

void SaveEnvSelSlot (COMMAND_T* ct)
{
	if (TrackEnvelope* envelope = GetSelectedEnvelope(NULL))
	{
		int slot = (int)ct->user;

		for (int i = 0; i < g_envSel.Get()->GetSize(); ++i)
		{
			if (slot == g_envSel.Get()->Get(i)->GetSlot())
				return g_envSel.Get()->Get(i)->Save(envelope);
		}
		g_envSel.Get()->Add(new BR_EnvSel(slot, envelope));
	}
}

void RestoreEnvSelSlot (COMMAND_T* ct)
{
	if (TrackEnvelope* envelope = GetSelectedEnvelope(NULL))
	{
		int slot = (int)ct->user;

		for (int i = 0; i < g_envSel.Get()->GetSize(); ++i)
		{
			if (slot == g_envSel.Get()->Get(i)->GetSlot())
			{
				if (g_envSel.Get()->Get(i)->Restore(envelope))
					Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_MISCCFG, -1);
				break;
			}
		}
	}
}

/******************************************************************************
* Commands: Envelopes - Visibility                                            *
******************************************************************************/
void ShowActiveTrackEnvOnly (COMMAND_T* ct)
{
	TrackEnvelope* envPtr = GetSelectedTrackEnvelope(NULL);
	const int trackCount = ((int)ct->user > 0) ? (CountTracks(NULL)) : (CountSelectedTracks(NULL));
	if (trackCount <= 0)
		return;

	WDL_PtrList_DeleteOnDestroy<BR_Envelope> envelopes;

	// First save selected envelope
	if (envPtr)
	{
		if (BR_Envelope* envelope = new (nothrow) BR_Envelope(envPtr))
		{
			envelope->SetVisible(true);

			// If envelope has only one point, REAPER will not show it after hiding all
			// envelopes and committing with vis = 1, so we create another artificial point
			if (envelope->CountPoints() <= 1)
			{
				double position, value; int shape;
				envelope->GetPoint(envelope->CountPoints() - 1, &position, &value, &shape, NULL);
				envelope->CreatePoint(envelope->CountPoints(), position+1, value, shape, 0, false);
				envelope->SetData((void*)2);
			}
			envelopes.Add(envelope);
		}
	}

	// Check other envelopes
	if (abs((int)ct->user) != 1)
	{
		for (int i = 0; i < trackCount; ++i)
		{
			MediaTrack* track = ((int)ct->user > 0) ? (GetTrack(NULL, i)) : (GetSelectedTrack(NULL, i));
			int envelopeCount = CountTrackEnvelopes(track);
			for (int j = 0; j < envelopeCount; ++j)
			{
				TrackEnvelope* currentEnvPtr = GetTrackEnvelope(track, j);
				if (currentEnvPtr != envPtr)
				{
					if (BR_Envelope* envelope = new (nothrow) BR_Envelope(currentEnvPtr))
					{
						envelopes.Add(envelope);
						if (envelope->IsVisible() && ((abs((int)ct->user) == 2) ? (envelope->IsInLane()) : (!envelope->IsInLane())))
						{
							envelope->SetVisible(true);
							if (envelope->CountPoints() <= 1)
							{
								double position, value; int shape;
								envelope->GetPoint(envelope->CountPoints() - 1, &position, &value, &shape, NULL);
								envelope->CreatePoint(envelope->CountPoints(), position+1, value, shape, 0, false);
								envelope->SetData((void*)2);
							}
						}
						else
						{
							envelopes.Delete(envelopes.GetSize() - 1, true);
						}
					}
				}
			}
		}
	}

	if (envelopes.GetSize() > 0 || (!envPtr && abs((int)ct->user) == 1))
	{
		Undo_BeginBlock2(NULL);
		PreventUIRefresh(1);

		// Commit with fake point so we don't loose the envelope
		for (int i = 0; i < envelopes.GetSize(); ++i)
		{
			if (envelopes.Get(i)->GetData() == (void*)2)
				envelopes.Get(i)->Commit(true);
		}

		// Hide envelopes using actions
		if ((int)ct->user > 0)
			Main_OnCommand(41150, 0); // hide all
		else
			Main_OnCommand(40889 ,0); // hide for selected tracks

		for (int i = 0; i < envelopes.GetSize(); ++i)
		{
			BR_Envelope* envelope = envelopes.Get(i);
			if (envelope->GetData() == (void*)2)
				envelope->DeletePoint(envelope->CountPoints()-1);
			envelope->Commit(true);
		}

		SetCursorContext(2, envPtr); // hiding envelopes with actions will unselect current envelope
		PreventUIRefresh(-1);
		Undo_EndBlock2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG | UNDO_STATE_MISCCFG);
	}
}

void ShowLastAdjustedSendEnv (COMMAND_T* ct)
{
	MediaTrack* track;
	int sendId;
	BR_EnvType type = UNKNOWN;
	GetSetLastAdjustedSend(false, &track, &sendId, &type);
	if      ((int)ct->user == 1) type = VOLUME;
	else if ((int)ct->user == 2) type = PAN;

	if (track)
	{
		if (ToggleShowSendEnvelope(track, sendId, type))
			Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG | UNDO_STATE_MISCCFG, -1);
	}
}

void ShowHideFxEnv (COMMAND_T* ct)
{
	bool hide       = !!GetBit((int)ct->user, 0);
	bool activeOnly = !!GetBit((int)ct->user, 1);
	bool toggle     = !!GetBit((int)ct->user, 2);

	// Get envelopes
	WDL_PtrList_DeleteOnDestroy<BR_Envelope> envelopes;
	const int trSelCnt = CountSelectedTracks(NULL);
	for (int i = 0; i < trSelCnt; ++i)
	{
		MediaTrack* track = GetSelectedTrack(NULL, i);
		for (int j = 0; j < CountTrackEnvelopes(track); ++j)
		{
			TrackEnvelope* envPtr = GetTrackEnvelope(track, j);
			if (GetEnvType(envPtr, NULL, NULL) == PARAMETER)
				envelopes.Add(new BR_Envelope(envPtr));
		}
	}

	// In case of toggle, check visibility status
	if (toggle)
	{
		bool allVisible = true;
		for (int i = 0; i < envelopes.GetSize(); ++i)
		{
			BR_Envelope* envelope = envelopes.Get(i);
			if ((!activeOnly || activeOnly == envelope->IsActive()) && !envelope->IsVisible())
			{
				allVisible = false;
				break;
			}
		}
		hide = (allVisible) ? true : false;
	}

	// Set new visibility
	PreventUIRefresh(1);
	bool update = false;
	for (int i = 0; i < envelopes.GetSize(); ++i)
	{
		BR_Envelope* envelope = envelopes.Get(i);
		if (hide == envelope->IsVisible() && (!activeOnly || activeOnly == envelope->IsActive()))
		{
			envelope->SetVisible(!hide);
			envelope->Commit(true);
			update = true;
		}
	}
	if (update)
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG | UNDO_STATE_MISCCFG, -1);
	PreventUIRefresh(-1);
}

void ShowHideSendEnv (COMMAND_T* ct)
{
	bool hide       = !!GetBit((int)ct->user, 0);
	bool activeOnly = !!GetBit((int)ct->user, 1);
	bool toggle     = !!GetBit((int)ct->user, 2);

	BR_EnvType mode = UNKNOWN;
	if (GetBit((int)ct->user, 3)) mode = static_cast<BR_EnvType>(mode | VOLUME);
	if (GetBit((int)ct->user, 4)) mode = static_cast<BR_EnvType>(mode | PAN);
	if (GetBit((int)ct->user, 5)) mode = static_cast<BR_EnvType>(mode | MUTE);

	// Get envelopes
	WDL_PtrList_DeleteOnDestroy<BR_Envelope> envelopes;
	const int trSelCnt = CountSelectedTracks(NULL);
	for (int i = 0; i < trSelCnt; ++i)
	{
		MediaTrack* track = GetSelectedTrack(NULL, i);
		for (int j = 0; j < CountTrackEnvelopes(track); ++j)
		{
			bool send   = false;
			bool hwSend = false;
			if ((mode & GetEnvType(GetTrackEnvelope(track, j), &send, &hwSend)) && (send || hwSend))
				envelopes.Add(new BR_Envelope(track, j));
		}
	}

	// In case of toggle, check visibility status
	if (toggle)
	{
		if (envelopes.GetSize())
		{
			bool allVisible = true;
			for (int i = 0; i < envelopes.GetSize(); ++i)
			{
				BR_Envelope* envelope = envelopes.Get(i);
				if ((!activeOnly || activeOnly == envelope->IsActive()) && !envelope->IsVisible())
				{
					allVisible = false;
					break;
				}
			}
			hide = (allVisible) ? true : false;
		}
		else
			hide = false;
	}

	// Set new visibility
	PreventUIRefresh(1);
	bool update = false;
	if (!hide && !activeOnly)
	{
		vector<MediaTrack*> selectedTracks;
		const int trSelCnt = CountSelectedTracks(NULL);
		for (int i = 0; i < trSelCnt; ++i)
			selectedTracks.push_back(GetSelectedTrack(NULL, i));
		update = ShowSendEnvelopes(selectedTracks, mode);
	}
	else
	{
		for (int i = 0; i < envelopes.GetSize(); ++i)
		{
			BR_Envelope* envelope = envelopes.Get(i);

			if (hide == envelope->IsVisible() && envelope->IsActive())
			{
				envelope->SetVisible(!hide);
				if (!envelope->IsVisible() && envelope->CountPoints() <= 1 && !CountAutomationItems(envelope->GetPointer()))
				{
					double value;
					const int trimMode = ConfigVar<int>("envtrimadjmode").value_or(0);
					if (envelope->GetPoint(0, NULL, &value, NULL, NULL) && (trimMode == 0 || (trimMode == 1 && GetEffectiveAutomationMode(envelope->GetParent()) != 0)))
					{
						if      (envelope->Type() == VOLUME) SetTrackSendUIVol(envelope->GetParent(), envelope->GetSendId(), value, 0); // don't do mute (REAPER skips it too)
						else if (envelope->Type() == PAN)    SetTrackSendUIPan(envelope->GetParent(), envelope->GetSendId(), -value, 0);
					}

					envelope->DeletePoint(0);   // this will completely destroy the envelope (Reaper
					envelope->SetActive(false); // does the same things when hiding envelopes)
				}

				envelope->Commit(true);
				update = true;
			}
		}
	}

	if (update)
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG | UNDO_STATE_MISCCFG, -1);
	PreventUIRefresh(-1);
}
