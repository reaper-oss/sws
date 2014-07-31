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
#include "BR_EnvTools.h"
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
* Commands                                                                    *
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
	}

	// Check envelope is visible
	int envHeight, envY;
	if (!g_envMouseEnvelope->VisibleInArrange(&envHeight, &envY, true)) // caching values is not 100% correct, but we don't expect for envelope lane height to change during the action
		return;                                                         // and it can speed things if dealing with big envelope that's in track lane (quite possible with tempo map)

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
			if (sscanf(token, "PT %lf", &cTime))
			{
				if (cTime > cursor)
				{
					// fake next point if it doesn't exist
					token = strtok(NULL, "\n");
					if (!sscanf(token, "PT %lf", &nTime))
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
			if (sscanf(token, "PT %lf", &cTime))
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
		if ((int)ct->user == -2 || (int)ct->user == 2 )
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

		Undo_BeginBlock2(NULL);
		SetEditCurPos(targetPos, true, false);
		envelope.Commit();
		Undo_EndBlock2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);
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
	double tStart, tEnd;
	GetSet_LoopTimeRange2(NULL, false, false, &tStart, &tEnd, false);

	if (tStart + MIN_ENV_DIST >= tEnd)
		return;

	BR_Envelope envelope(GetSelectedEnvelope(NULL));
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

void ShowActiveTrackEnvOnly (COMMAND_T* ct)
{
	TrackEnvelope* env = GetSelectedTrackEnvelope(NULL);
	if (!env || ((int)ct->user == 1 && !CountSelectedTracks(NULL)))
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

	if ((int)ct->user == 0)
		Main_OnCommand(41150, 0); // hide all
	else
		Main_OnCommand(40889 ,0); // hide for selected tracks

	if (flag)
		envelope.DeletePoint(envelope.Count()-1);

	envelope.Commit(true);
	Undo_EndBlock2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL);
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
				g_envSel.Get()->Get(i)->Restore(envelope);
				Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
				break;
			}
		}
	}
}