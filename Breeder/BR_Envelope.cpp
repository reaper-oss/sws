/******************************************************************************
/ BR_Envelope.cpp
/
/ Copyright (c) 2014 Dominik Martin Drzic
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
#include "stdafx.h"
#include "BR_Envelope.h"
#include "BR_ContinuousActions.h"
#include "BR_EnvelopeUtil.h"
#include "BR_MouseUtil.h"
#include "BR_ProjState.h"
#include "BR_Util.h"
#include "../reaper/localize.h"

/******************************************************************************
* Continuous action: set envelope point value to mouse                        *
******************************************************************************/
static BR_Envelope* g_envMouseEnvelope = NULL;
static bool         g_envMouseDidOnce  = false;
static int          g_envMouseMode     = -666;

static bool EnvMouseInit (bool init)
{
	static int s_editCursorUndo = 0;

	bool initSuccessful = true;
	if (init)
	{
		GetConfig("undomask", s_editCursorUndo);
		g_envMouseEnvelope = new (nothrow) BR_Envelope(GetSelectedEnvelope(NULL));
		initSuccessful = g_envMouseEnvelope && g_envMouseEnvelope->Count() && PositionAtMouseCursor(false) != -1;

		if (initSuccessful)
			SetConfig("undomask", ClearBit(s_editCursorUndo, 3));
		else
		{
			delete g_envMouseEnvelope;
			g_envMouseEnvelope = NULL;
		}
		GetConfig("undomask", s_editCursorUndo);
	}
	else
	{
		SetConfig("undomask", s_editCursorUndo);
		delete g_envMouseEnvelope;
		g_envMouseEnvelope = NULL;
	}

	g_envMouseDidOnce = false;
	g_envMouseMode    = -666;
	return initSuccessful;
}

static bool EnvMouseUndo ()
{
	return g_envMouseDidOnce;
}

static HCURSOR EnvMouseCursor (int window)
{
	static HCURSOR s_cursor = NULL;
	if (!s_cursor)
		s_cursor = LoadCursor(NULL, IDC_SIZENS);

	if (window == BR_ContinuousAction::ARRANGE || window == BR_ContinuousAction::RULER)
		return s_cursor;
	else
		return NULL;
}

static WDL_FastString EnvMouseTooltip (int window)
{
	WDL_FastString tooltip;

	if (g_envMouseEnvelope && !GetBit(ContinuousActionTooltips(), 0))
	{
		if (window == BR_ContinuousAction::ARRANGE || window == BR_ContinuousAction::RULER)
		{
			int envHeight, envY, yOffset;
			bool overRuler;
			double position = PositionAtMouseCursor(true, false, &yOffset, &overRuler);

			g_envMouseEnvelope->VisibleInArrange(&envHeight, &envY, true);
			yOffset = (overRuler) ? envY : SetToBounds(yOffset, envY, envY + envHeight);

			double normalizedValue = ((double)envHeight + (double)envY - (double)yOffset) / (double)envHeight;
			double value = g_envMouseEnvelope->SnapValue(g_envMouseEnvelope->RealDisplayValue(normalizedValue));
			if (g_envMouseEnvelope->IsTempo())
				GetTempoTimeSigMarker(NULL, (g_envMouseMode == 0) ? FindClosestTempoMarker(position) : FindPreviousTempoMarker(position), &position, NULL, NULL, NULL, NULL, NULL, NULL);
			else
				g_envMouseEnvelope->GetPoint((g_envMouseMode == 0) ? g_envMouseEnvelope->FindClosest(position) : g_envMouseEnvelope->FindPrevious(position), &position, NULL, NULL, NULL);

			static const char* s_format = __localizeFunc("Envelope: %s\n%s at %s", "tooltip", 0);
			tooltip.AppendFormatted(512, s_format, (g_envMouseEnvelope->GetName()).Get(), (g_envMouseEnvelope->FormatValue(value)).Get(), FormatTime(position).Get());
		}

	}
	return tooltip;
}

void SetEnvPointMouseValueInit ()
{
	ContinuousActionRegister(new BR_ContinuousAction(NamedCommandLookup("_BR_ENV_PT_VAL_CLOSEST_MOUSE"),      &EnvMouseInit, &EnvMouseUndo, &EnvMouseCursor, EnvMouseTooltip));
	ContinuousActionRegister(new BR_ContinuousAction(NamedCommandLookup("_BR_ENV_PT_VAL_CLOSEST_LEFT_MOUSE"), &EnvMouseInit, &EnvMouseUndo, &EnvMouseCursor, EnvMouseTooltip));
}

/******************************************************************************
* Commands: Envelopes - Misc                                                  *
******************************************************************************/
void SetEnvPointMouseValue (COMMAND_T* ct)
{
	static int    s_lastEndId       = -1;
	static double s_lastEndPosition = -1;
	static double s_lastEndNormVal  = -1;

	// Action called for the first time
	if (g_envMouseMode == -666)
	{
		s_lastEndId       = -1;
		s_lastEndPosition = -1;
		s_lastEndNormVal  = -1;
		g_envMouseMode  = (int)ct->user;

		// BR_Envelope does check for locking but we're also using tempo API here so check manually
		if (g_envMouseEnvelope->IsLocked())
		{
				ContinuousActionStopAll();
				return;
		}
	}

	// Check envelope is visible
	int envHeight, envY;                                                // caching values is not 100% correct, but we don't expect for envelope lane height to change during the action
	if (!g_envMouseEnvelope->VisibleInArrange(&envHeight, &envY, true)) // and it can speed things if dealing with big envelope that's in track lane (quite possible with tempo map)
		return;

	// Get mouse positions
	int yOffset; bool overRuler;
	double endPosition   = PositionAtMouseCursor(true, false, &yOffset, &overRuler);
	double startPosition = (s_lastEndPosition == -1) ? (endPosition) : (s_lastEndPosition);
	if (endPosition == -1)
	{
		ContinuousActionStopAll();
		return;
	}

	// Get normalized mouse values
	yOffset = (overRuler) ? envY : SetToBounds(yOffset, envY, envY + envHeight);
	double endNormVal   = ((double)envHeight + (double)envY - (double)yOffset) / (double)envHeight;
	double startNormVal = (s_lastEndPosition == -1) ? (endNormVal) : (s_lastEndNormVal);

	// Find all the point over which mouse passed
	int startId = -1;
	int endId   = -1;
	if ((int)ct->user == 0)
	{
		if (s_lastEndId == -1)
			startId = (g_envMouseEnvelope->IsTempo()) ? FindClosestTempoMarker(startPosition) : g_envMouseEnvelope->FindClosest(startPosition);
		else
			startId = s_lastEndId;

		endId  = (g_envMouseEnvelope->IsTempo()) ? FindClosestTempoMarker(endPosition) : g_envMouseEnvelope->FindClosest(endPosition);
	}
	else
	{
		if (s_lastEndId == -1)
		{
			startId = (g_envMouseEnvelope->IsTempo()) ? FindPreviousTempoMarker(startPosition) : g_envMouseEnvelope->FindPrevious(startPosition);
			double nextPos;
			if (g_envMouseEnvelope->IsTempo() && GetTempoTimeSigMarker(NULL, startId, &nextPos, NULL, NULL, NULL, NULL, NULL, NULL) && nextPos == startPosition)
				++startId;
			else if (!g_envMouseEnvelope->IsTempo() && g_envMouseEnvelope->GetPoint(startId+1, &nextPos, NULL, NULL, NULL) && nextPos == startPosition)
				++startId;
		}
		else
		{
			startId = s_lastEndId;
		}

		endId = (g_envMouseEnvelope->IsTempo()) ? FindPreviousTempoMarker(endPosition) : g_envMouseEnvelope->FindPrevious(endPosition);
		double nextPos;
		if (g_envMouseEnvelope->IsTempo() && GetTempoTimeSigMarker(NULL, endId, &nextPos, NULL, NULL, NULL, NULL, NULL, NULL) && nextPos == endPosition)
			++endId;
		else if (!g_envMouseEnvelope->IsTempo() && g_envMouseEnvelope->GetPoint(endId+1, &nextPos, NULL, NULL, NULL) && nextPos == endPosition)
			++endId;
	}

	// Apply changes to all the points
	if (g_envMouseEnvelope->ValidateId(startId) && g_envMouseEnvelope->ValidateId(endId))
	{
		bool oppositeDirection = false;
		if (endId < startId)
		{
			swap(endId, startId);
			swap(endPosition, startPosition);
			swap(endNormVal,  startNormVal);
			oppositeDirection = true;
		}

		bool pointsMoved = false;
		for (int i = startId; i <= endId; ++i)
		{
			// Get current point's position
			double currentPointPos;
			if (g_envMouseEnvelope->IsTempo())
			{
				GetTempoTimeSigMarker(NULL, i, &currentPointPos, NULL, NULL, NULL, NULL, NULL, NULL);

				// If going forward, make sure edited tempo points do not come in front of mouse cursor (if so, leave them for the next action call)
				if (!oppositeDirection && endId != startId && !CheckBounds(currentPointPos, startPosition, endPosition))
				{
					endId = i;
					endPosition = currentPointPos;
					break;
				}
			}
			else
				g_envMouseEnvelope->GetPoint(i, &currentPointPos, NULL, NULL, NULL);

			// Find new value
			double value = 0;
			if  (i == startId)
				value = g_envMouseEnvelope->RealDisplayValue(startNormVal);
			else if (i == endId)
				value = g_envMouseEnvelope->RealDisplayValue(endNormVal);
			else
			{
				double t = (currentPointPos - startPosition) / (endPosition - startPosition);
				double currentNormVal = startNormVal + (endNormVal - startNormVal) * t;
				value = g_envMouseEnvelope->RealDisplayValue(currentNormVal);
			}

			// Update current point
			if (g_envMouseEnvelope->IsTempo())
			{
				int measure, num, den;
				double beat;
				bool linear;
				GetTempoTimeSigMarker(NULL, i, NULL, &measure, &beat, NULL, &num, &den, &linear);
				if (SetTempoTimeSigMarker(NULL, i, -1, measure, beat, value, num, den, linear))
					pointsMoved = true;
			}
			else
			{
				if (g_envMouseEnvelope->SetPoint(i, NULL, &value, NULL, NULL, false, true))
					pointsMoved = true;
			}
		}

		if (pointsMoved)
		{
			if (g_envMouseEnvelope->IsTempo()) UpdateTimeline();
			else                               g_envMouseEnvelope->Commit();

			g_envMouseDidOnce = pointsMoved;
		}

		// Swap values back before storing them
		if (oppositeDirection)
		{
			swap(endId, startId);
			swap(endPosition, startPosition);
			swap(endNormVal,  startNormVal);
		}
		s_lastEndId       = endId;
		s_lastEndPosition = endPosition;
		s_lastEndNormVal  = endNormVal;
	}
}

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
			if (sscanf(token, "PT %.20lf", &cTime))
			{
				if (cTime > cursor)
				{
					// fake next point if it doesn't exist
					token = strtok(NULL, "\n");
					if (!sscanf(token, "PT %.20lf", &nTime))
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
			if (sscanf(token, "PT %.20lf", &cTime))
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
		if (!found && cursor > cTime )
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
			int envClickSegMode; GetConfig("envclicksegmode", envClickSegMode);
			SetConfig("envclicksegmode", SetBit(envClickSegMode, 6));

			// Set time selection that in turn sets new envelope selection
			nStart = (cTime + pTime) / 2 + takeEnvStartPos;
			nEnd = (nTime + cTime) / 2 + takeEnvStartPos;
			GetSet_LoopTimeRange2(NULL, true, false, &nStart, &nEnd, false);

			// Preserve point when previous time selection gets restored
			SetConfig("envclicksegmode", ClearBit(envClickSegMode, 6));
			GetSet_LoopTimeRange2(NULL, true, false, &tStart, &tEnd, false);

			// Restore preferences
			SetConfig("envclicksegmode", envClickSegMode);

			PreventUIRefresh(-1);
		}

		SetEditCurPos(cTime + takeEnvStartPos, true, false);
		Undo_EndBlock2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);
	}
	FreeHeapPtr(envState);
}

void CursorToEnv2 (COMMAND_T* ct)
{
	BR_Envelope envelope(GetSelectedEnvelope(NULL));
	if (!envelope.Count())
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
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
	}
}

void SelNextPrevEnvPoint (COMMAND_T* ct)
{
	BR_Envelope envelope(GetSelectedEnvelope(NULL));
	if (!envelope.CountSelected())
		return;

	int id;
	if ((int)ct->user > 0)
		id = envelope.GetSelected(envelope.CountSelected()-1) + 1;
	else
		id = envelope.GetSelected(0) - 1;

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

			Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
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
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
	}
}

void ExpandEnvSelEnd (COMMAND_T* ct)
{
	BR_Envelope envelope(GetSelectedEnvelope(NULL));
	if (!envelope.CountSelected())
		return;

	int id;
	if ((int)ct->user > 0)
		id = envelope.GetSelected(envelope.CountSelected() - 1) + 1;
	else
		id = envelope.GetSelected(0) - 1;
	envelope.SetSelection(id, true);

	if (envelope.Commit())
	{
		envelope.MoveArrangeToPoint(id, ((int)ct->user > 0) ? (id-1) : (id+1));
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
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
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
	}
}

void ShrinkEnvSelEnd (COMMAND_T* ct)
{
	BR_Envelope envelope(GetSelectedEnvelope(NULL));
	if (!envelope.CountSelected())
		return;

	int id;
	if ((int)ct->user > 0)
		id = envelope.GetSelected(envelope.CountSelected() - 1);
	else
		id = envelope.GetSelected(0);
	envelope.SetSelection(id, false);

	if (envelope.Commit())
	{
		envelope.MoveArrangeToPoint (((int)ct->user > 0) ? (id-1) : (id+1), id);
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
	}
}

void EnvPointsGrid (COMMAND_T* ct)
{
	BR_Envelope envelope(GetSelectedEnvelope(NULL));
	if (!envelope.Count())
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
		endId   = envelope.Count()-1;
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

		bool doesPointPass = (onGrid) ? (IsEqual(position, grid, MIN_ENV_DIST)  ||  IsEqual(position, nextGrid, MIN_ENV_DIST))
									  : (!IsEqual(position, grid, MIN_ENV_DIST) && !IsEqual(position, nextGrid, MIN_ENV_DIST));

		if (doesPointPass)
		{
			if (deletePoints)
			{
				if (pointsToDelete.size() == 0 || pointsToDelete.back().second != i -1)
				{
					pair<int, int> newPair(i, i);
					pointsToDelete.push_back(newPair);
				}
				pointsToDelete.back().second = i;

			}
			else
				envelope.SetSelection(i, true);
		}
	}

	// Readjust tempo points
	if (deletePoints && pointsToDelete.size() > 0)
	{
		int timeBase; GetConfig("tempoenvtimelock", timeBase);
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

					int nextPoint = (i == pointsToDelete.size() - 1) ? envelope.Count() : pointsToDelete[i+1].first;
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
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
}

void CreateEnvPointsGrid (COMMAND_T* ct)
{
	BR_Envelope envelope(GetSelectedEnvelope(NULL));
	if (!envelope.Count())
		return;

	bool timeSel = ((int)ct->user == 1) ? true : false;
	int startId, endId;
	double tStart, tEnd;
	if (!timeSel || !envelope.GetPointsInTimeSelection(&startId, &endId, &tStart, &tEnd))
	{
		startId = 0;
		endId   = envelope.Count()-1;
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
				s0 = envelope.DefaultShape();
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
				if (gridLine < t1 - (MAX_GRID_DIV/2))
				{
					position.push_back(gridLine);
					value.push_back(envelope.ValueAtPosition(gridLine)); // ValueAtPosition is much more faster when dealing with sorted
					shape.push_back(s0);                                 // points and inserting points will make envelope unsorted
					bezier.push_back(b0);
				}
				else
					break;
			}
			previousGridLine = gridLine;
		}
	}

	for (size_t i = 0; i < position.size(); ++i)
		envelope.CreatePoint(envelope.Count(), position[i], value[i], shape[i], bezier[i], false, true);


	if (envelope.Commit())
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
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
			envelope.SetSelection(id, false);
		}
	}
	else
	{
		for (int i = envelope.CountSelected()-1; i >= 0; --i)
		{
			int id = envelope.GetSelected(i);
			envelope.SetSelection(id+1, true);
			envelope.SetSelection(id, false);
		}
	}

	if (envelope.Commit())
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
}

void PeaksDipsEnv (COMMAND_T* ct)
{
	BR_Envelope envelope(GetSelectedEnvelope(NULL));
	if (!envelope.Count())
		return;

	if ((int)ct->user == -2 || (int)ct->user == 2)
		envelope.UnselectAll();

	for (int i = 1; i < envelope.Count()-1 ; ++i)
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
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
}

void SelEnvTimeSel (COMMAND_T* ct)
{
	double tStart, tEnd;
	GetSet_LoopTimeRange2(NULL, false, false, &tStart, &tEnd, false);
	if (tStart == tEnd)
		return;

	BR_Envelope envelope(GetSelectedEnvelope(NULL));
	if (!envelope.Count())
		return;

	for (int i = 0; i < envelope.Count() ; ++i)
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
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
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
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
}

void MoveEnvPointToEditCursor (COMMAND_T* ct)
{
	double cursor = GetCursorPositionEx(NULL);
	int id = -1;

	BR_Envelope envelope(GetSelectedEnvelope(NULL));
	if (!envelope.Count())
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
				Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
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
			for (int j = 0; j < envelopeCount; ++j)
			{
				BR_Envelope envelope(selEnvOnly ? GetSelectedEnvelope(NULL) : GetTrackEnvelope(track, j));

				if (envelope.IsVisible())
				{
					int startId = envelope.Find(tStart, MIN_ENV_DIST);
					int endId   = envelope.Find(tEnd, MIN_ENV_DIST);
					int defaultShape = envelope.DefaultShape();
					envelope.UnselectAll();

					// Create left-side point only if surrounding points are not too close, otherwise just move existing
					if (envelope.ValidateId(startId))
					{
						if (envelope.SetPoint(startId, &tStart, NULL, &defaultShape, 0, true))
							envelope.SetSelection(startId, true);
					}
					else
						envelope.CreatePoint(envelope.Count(), tStart, envelope.ValueAtPosition(tStart), defaultShape, 0, true, true);

					// Create right-side point only if surrounding points are not too close, otherwise just move existing
					if (envelope.ValidateId(endId))
					{
						if (envelope.SetPoint(endId, &tEnd, NULL, &defaultShape, 0, true))
							envelope.SetSelection(endId, true);
					}
					else
						envelope.CreatePoint(envelope.Count(), tEnd, envelope.ValueAtPosition(tEnd), defaultShape, 0, true, true);

					if (envelope.Commit())
						update = true;
				}
			}
		}
	}
	PreventUIRefresh(-1);

	if (update)
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
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
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
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

	if (position != -1 && envelope.VisibleInArrange())
	{
		position = SnapToGrid(NULL, position);
		double fudgeFactor = (envelope.IsTempo()) ? (MIN_TEMPO_DIST) : (MIN_ENV_DIST);
		if (!envelope.ValidateId(envelope.Find(position, fudgeFactor)))
		{
			double value = envelope.ValueAtPosition(position);
			if (envelope.IsTempo())
			{
				if (SetTempoTimeSigMarker(NULL, -1, position, -1, -1, value, 0, 0, !envelope.DefaultShape()))
				{
					UpdateTimeline();
					Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
				}
			}
			else
			{
				envelope.CreatePoint(envelope.Count(), position, value, envelope.DefaultShape(), 0, false, true);
				if (envelope.Commit())
					Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
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
			Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
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
					Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
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
	TrackEnvelope* env = GetSelectedTrackEnvelope(NULL);
	if ((int)ct->user == 1 && !CountSelectedTracks(NULL))
		return;
	BR_Envelope envelope(env);

	Undo_BeginBlock2(NULL);

	// If envelope has only one point, Reaper will not show it after hiding all
	// envelopes and committing with vis = 1, so we create another artificial point
	bool flag = false;
	if (envelope.Count() <= 1)
	{
		int id = envelope.Count() - 1;
		double position, value; int shape;
		envelope.GetPoint(id, &position, &value, &shape, NULL);
		envelope.CreatePoint(id+1, position+1, value, shape, 0, false);
		envelope.Commit();
		flag = true;
	}

	PreventUIRefresh(1);
	if ((int)ct->user == 0)
		Main_OnCommand(41150, 0); // hide all
	else
		Main_OnCommand(40889 ,0); // hide for selected tracks

	if (flag)
		envelope.DeletePoint(envelope.Count()-1);

	envelope.Commit(true);
	PreventUIRefresh(-1);
	Undo_EndBlock2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);
}

void ShowLastAdjustedSendEnv (COMMAND_T* ct)
{
	MediaTrack* track; int sendId, type;
	GetSetLastAdjustedSend(false, &track, &sendId, &type);
	if      ((int)ct->user == 1) type = VOLUME;
	else if ((int)ct->user == 2) type = PAN;

	if (track)
	{
		if (ToggleShowSendEnvelope(track, sendId, type))
			Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
	}
}

void ShowHideFxEnv (COMMAND_T* ct)
{
	bool hide       = !!GetBit((int)ct->user, 0);
	bool activeOnly = !!GetBit((int)ct->user, 1);
	bool toggle     = !!GetBit((int)ct->user, 2);

	// Get envelopes
	WDL_PtrList_DeleteOnDestroy<BR_Envelope> envelopes;
	for (int i = 0; i < CountSelectedTracks(NULL); ++i)
	{
		MediaTrack* track = GetSelectedTrack(NULL, i);
		for (int j = 0; j < CountTrackEnvelopes(track); ++j)
		{
			TrackEnvelope* envPtr = GetTrackEnvelope(track, j);
			if (GetEnvType(envPtr, NULL) == PARAMETER)
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
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
	PreventUIRefresh(-1);
}

void ShowHideSendEnv (COMMAND_T* ct)
{
	bool hide       = !!GetBit((int)ct->user, 0);
	bool activeOnly = !!GetBit((int)ct->user, 1);
	bool toggle     = !!GetBit((int)ct->user, 2);
	int mode        = (GetBit((int)ct->user, 3) ? VOLUME : 0) | (GetBit((int)ct->user, 4) ? PAN : 0) | (GetBit((int)ct->user, 5) ? MUTE : 0);

	// Get envelopes
	WDL_PtrList_DeleteOnDestroy<BR_Envelope> envelopes;
	for (int i = 0; i < CountSelectedTracks(NULL); ++i)
	{
		MediaTrack* track = GetSelectedTrack(NULL, i);
		for (int j = 0; j < CountTrackEnvelopes(track); ++j)
		{
			bool send = false;
			if ((mode & GetEnvType(GetTrackEnvelope(track, j), &send)) && send)
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
		for (int i = 0; i < CountSelectedTracks(NULL); ++i)
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
				if (!envelope->IsVisible() && envelope->Count() <= 1)
				{
					double value;
					if (envelope->GetPoint(0, NULL, &value, NULL, NULL) && GetCurrentAutomationMode(envelope->GetParent()) != 0)
					{
						if      (envelope->Type() == VOLUME) SetTrackSendUIVol(envelope->GetParent(), envelope->GetSendId(), value, 0); // don't do mute (reaper skips it too)
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
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
	PreventUIRefresh(-1);
}