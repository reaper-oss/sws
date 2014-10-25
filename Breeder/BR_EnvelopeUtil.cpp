/******************************************************************************
/ BR_EnvelopeUtil.cpp
/
/ Copyright (c) 2013-2014 Dominik Martin Drzic
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
#include "BR_EnvelopeUtil.h"
#include "BR_Util.h"
#include "../../WDL/lice/lice_bezier.h"
#include "../reaper/localize.h"

/******************************************************************************
* BR_EnvPoint                                                                 *
******************************************************************************/
BR_EnvPoint::BR_EnvPoint (double position) :
position (position),
value    (0),
shape    (0),
sig      (0),
selected (0),
partial  (0),
bezier   (0)
{
}

BR_EnvPoint::BR_EnvPoint (double position, double value, int shape, int sig, int selected, int partial, double bezier) :
position (position),
value    (value),
shape    (shape),
sig      (sig),
selected (selected),
partial  (partial),
bezier   (bezier)
{
}

bool BR_EnvPoint::ReadLine (const LineParser& lp)
{
	if (strcmp(lp.gettoken_str(0), "PT"))
		return false;
	else
	{
		this->position = lp.gettoken_float(1);
		this->value    = lp.gettoken_float(2);
		this->shape    = lp.gettoken_int(3);
		this->sig      = lp.gettoken_int(4);
		this->selected = lp.gettoken_int(5);
		this->partial  = lp.gettoken_int(6);
		this->bezier   = lp.gettoken_float(7);
		return true;
	}
}

void BR_EnvPoint::Append (WDL_FastString& string)
{
	string.AppendFormatted
	(
		256,
		"PT %.12lf %.10lf %d %d %d %d %.8lf\n",
		this->position,
		this->value,
		this->shape,
		this->sig,
		this->selected,
		this->partial,
		this->bezier
	);
}

/******************************************************************************
* BR_Envelope                                                                 *
******************************************************************************/
BR_Envelope::BR_Envelope () :
m_envelope      (NULL),
m_take          (NULL),
m_parent        (NULL),
m_tempoMap      (false),
m_update        (false),
m_sorted        (true),
m_takeEnvOffset (0),
m_takeEnvType   (UNKNOWN),
m_count         (0),
m_countSel      (0),
m_countConseq   (-1),
m_height        (-1),
m_yOffset       (-1)
{
}

BR_Envelope::BR_Envelope (TrackEnvelope* envelope, bool takeEnvelopesUseProjectTime /*=true*/) :
m_envelope      (envelope),
m_take          (NULL),
m_parent        (NULL),
m_tempoMap      (envelope == GetTempoEnv()),
m_update        (false),
m_sorted        (true),
m_takeEnvOffset (0),
m_takeEnvType   (UNKNOWN),
m_countConseq   (-1),
m_height        (-1),
m_yOffset       (-1)
{
	if (m_envelope)
	{
		char* envState = GetSetObjectState(m_envelope, "");
		if (!strncmp(envState, "<TRACK_ENVELOPE_UNKNOWN", sizeof("<TRACK_ENVELOPE_UNKNOWN")-1))
			m_take = GetTakeEnvParent(m_envelope, &m_takeEnvType);
		this->ParseState(envState, strlen(envState));
		FreeHeapPtr(envState);
	}

	m_count    = (int)m_points.size();
	m_countSel = (int)m_pointsSel.size();
	if (takeEnvelopesUseProjectTime && m_take) m_takeEnvOffset = GetMediaItemInfo_Value(GetMediaItemTake_Item(m_take), "D_POSITION");
}

BR_Envelope::BR_Envelope (MediaTrack* track, int envelopeId, bool takeEnvelopesUseProjectTime /*=true*/) :
m_envelope      (GetTrackEnvelope(track, envelopeId)),
m_take          (NULL),
m_parent        (track),
m_tempoMap      (m_envelope == GetTempoEnv()),
m_update        (false),
m_sorted        (true),
m_takeEnvOffset (0),
m_takeEnvType   (UNKNOWN),
m_countConseq   (-1),
m_height        (-1),
m_yOffset       (-1)
{
	if (m_envelope)
	{
		char* envState = GetSetObjectState(m_envelope, "");
		this->ParseState(envState, strlen(envState));
		FreeHeapPtr(envState);
	}

	m_count    = (int)m_points.size();
	m_countSel = (int)m_pointsSel.size();
	if (takeEnvelopesUseProjectTime && m_take) m_takeEnvOffset = GetMediaItemInfo_Value(GetMediaItemTake_Item(m_take), "D_POSITION");
}

BR_Envelope::BR_Envelope (MediaItem_Take* take, BR_EnvType envType, bool takeEnvelopesUseProjectTime /*=true*/) :
m_envelope      (GetTakeEnv(take, envType)),
m_take          (take),
m_parent        (NULL),
m_tempoMap      (false),
m_update        (false),
m_sorted        (true),
m_takeEnvOffset (0),
m_takeEnvType   (m_envelope ? envType : UNKNOWN),
m_countConseq   (-1),
m_height        (-1),
m_yOffset       (-1)
{
	if (m_envelope)
	{
		char* envState = GetSetObjectState(m_envelope, "");
		this->ParseState(envState, strlen(envState));
		FreeHeapPtr(envState);
	}

	m_count    = (int)m_points.size();
	m_countSel = (int)m_pointsSel.size();
	if (takeEnvelopesUseProjectTime && m_take) m_takeEnvOffset = GetMediaItemInfo_Value(GetMediaItemTake_Item(m_take), "D_POSITION");
}

BR_Envelope::BR_Envelope (const BR_Envelope& envelope) :
m_envelope      (envelope.m_envelope),
m_take          (envelope.m_take),
m_parent        (envelope.m_parent),
m_tempoMap      (envelope.m_tempoMap),
m_update        (envelope.m_update),
m_sorted        (envelope.m_sorted),
m_takeEnvOffset (envelope.m_takeEnvOffset),
m_takeEnvType   (envelope.m_takeEnvType),
m_countConseq   (envelope.m_countConseq),
m_height        (envelope.m_height),
m_yOffset       (envelope.m_yOffset),
m_count         (envelope.m_count),
m_countSel      (envelope.m_countSel),
m_points        (envelope.m_points),
m_pointsSel     (envelope.m_pointsSel),
m_pointsConseq  (envelope.m_pointsConseq),
m_chunkStart    (envelope.m_chunkStart),
m_chunkEnd      (envelope.m_chunkEnd),
m_envName       (envelope.m_envName),
m_properties    (envelope.m_properties)
{
}

BR_Envelope& BR_Envelope::operator= (const BR_Envelope& envelope)
{
	if (this == &envelope)
		return *this;

	m_envelope      = envelope.m_envelope;
	m_take          = envelope.m_take;
	m_parent        = envelope.m_parent;
	m_tempoMap      = envelope.m_tempoMap;
	m_update        = envelope.m_update;
	m_sorted        = envelope.m_sorted;
	m_takeEnvOffset = envelope.m_takeEnvOffset;
	m_takeEnvType   = envelope.m_takeEnvType;
	m_countConseq   = envelope.m_countConseq;
	m_height        = envelope.m_height;
	m_yOffset       = envelope.m_yOffset;
	m_count         = envelope.m_count;
	m_countSel      = envelope.m_countSel;
	m_points        = envelope.m_points;
	m_pointsSel     = envelope.m_pointsSel;
	m_pointsConseq  = envelope.m_pointsConseq;
	m_properties    = envelope.m_properties;
	m_chunkStart.Set(&envelope.m_chunkStart);
	m_chunkEnd.Set(&envelope.m_chunkEnd);
	m_envName.Set(&envelope.m_envName);

	return *this;
}

bool BR_Envelope::operator== (const BR_Envelope& envelope)
{
	if (this->m_tempoMap != envelope.m_tempoMap)
		return false;
	if (this->m_count    != envelope.m_count)
		return false;
	if (this->m_countSel != envelope.m_countSel)
		return false;

	if (this->m_properties.filled    == false)
		this->FillProperties();
	if (envelope.m_properties.filled == false)
		envelope.FillProperties();

	if (this->m_properties.active != envelope.m_properties.active)
		return false;

	for (int i = 0 ; i < this->m_count; ++i)
	{
		if (this->m_points[i].position != envelope.m_points[i].position)
			return false;
		if (this->m_points[i].value    != envelope.m_points[i].value)
			return false;
		if (this->m_points[i].bezier   != envelope.m_points[i].bezier)
			return false;
		if (this->m_points[i].shape    != envelope.m_points[i].shape)
			return false;
		if (this->m_points[i].sig      != envelope.m_points[i].sig)
			return false;
		if (this->m_points[i].selected != envelope.m_points[i].selected)
			return false;
		if (this->m_points[i].partial  != envelope.m_points[i].partial)
			return false;
	}
	return true;
}

bool BR_Envelope::operator!= (const BR_Envelope& envelope)
{
	return !(*this == envelope);
}

bool BR_Envelope::GetPoint (int id, double* position, double* value, int* shape, double* bezier)
{
	if (this->ValidateId(id))
	{
		WritePtr(position, m_points[id].position + m_takeEnvOffset);
		WritePtr(value,    m_points[id].value   );
		WritePtr(shape,    m_points[id].shape   );
		WritePtr(bezier,   m_points[id].bezier  );
		return true;
	}
	else
	{
		WritePtr(position, 0.0);
		WritePtr(value,    0.0);
		WritePtr(shape,    0  );
		WritePtr(bezier,   0.0);
		return false;
	}
}

bool BR_Envelope::SetPoint (int id, double* position, double* value, int* shape, double* bezier, bool checkPosition /*= false*/, bool snapValue /*= false*/)
{
	if (this->ValidateId(id))
	{
		if (this->IsTakeEnvelope() && position && checkPosition && !CheckBounds(*position - m_takeEnvOffset, 0.0, GetMediaItemInfo_Value(GetMediaItemTake_Item(m_take), "D_LENGTH")))
			return false;

		if (snapValue && value)
			WritePtr(value, this->SnapValue(*value));

		if (position) m_points[id].position = *position - m_takeEnvOffset;
		ReadPtr(value,    m_points[id].value);
		ReadPtr(shape,    m_points[id].shape);
		ReadPtr(bezier,   m_points[id].bezier);

		m_update = true;
		if (position) m_sorted = false;
		return true;
	}
	else
		return false;
}

bool BR_Envelope::GetSelection (int id)
{
	if (this->ValidateId(id))
		return (m_points[id].selected) ? true : false;
	else
		return false;
}

bool BR_Envelope::SetSelection (int id, bool selected)
{
	if (this->ValidateId(id))
	{
		m_points[id].selected = selected;
		m_update = true;
		return true;
	}
	else
		return false;
}

bool BR_Envelope::CreatePoint (int id, double position, double value, int shape, double bezier, bool selected, bool checkPosition /*=false*/, bool snapValue /*= false*/)
{
	if (this->ValidateId(id) || id == m_count)
	{
		position -= m_takeEnvOffset;

		if (this->IsTakeEnvelope() && checkPosition && !CheckBounds(position, 0.0, GetMediaItemInfo_Value(GetMediaItemTake_Item(m_take), "D_LENGTH")))
			return false;

		BR_EnvPoint newPoint(position, (snapValue) ? (this->SnapValue(value)) : (value), shape, 0, selected, 0, bezier);
		m_points.insert(m_points.begin() + id, newPoint);

		++m_count;
		m_update = true;
		m_sorted = false;
		return true;
	}
	else
		return false;
}

bool BR_Envelope::DeletePoint (int id)
{
	if (this->ValidateId(id))
	{
		m_points.erase(m_points.begin() + id);

		--m_count;
		m_update = true;
		return true;
	}
	else
		return false;
}

bool BR_Envelope::DeletePoints (int startId, int endId)
{
	if (!this->ValidateId(startId) || !this->ValidateId(endId))
		return false;

	m_points.erase(m_points.begin() + startId, m_points.begin() + endId+1);

	m_count -= endId - startId + 1;
	m_update = true;
	return true;
}

bool BR_Envelope::GetTimeSig (int id, bool* sig, bool* partial, int* num, int* den)
{
	if (this->ValidateId(id) && m_tempoMap)
	{
		WritePtr(sig,     (m_points[id].sig)                ? (true) : (false));
		WritePtr(partial, (GetBit(m_points[id].partial, 2)) ? (true) : (false));

		if (num || den)
		{
			int effectiveTimeSig = 0;
			for (;id >= 0; --id)
			{
				if (m_points[id].sig != 0)
				{
					effectiveTimeSig = m_points[id].sig;
					break;
				}
			}

			int effNum, effDen;
			if (!effectiveTimeSig)
				TimeMap_GetTimeSigAtTime(NULL, -1, &effNum, &effDen, NULL);
			else
			{
				effNum = effectiveTimeSig & 0xFF; // num is low 16 bits
				effDen = effectiveTimeSig >> 16;  // den is high 16 bits
			}

			WritePtr(num,     effNum);
			WritePtr(den,     effDen);
		}
		return true;
	}
	else
	{
		WritePtr(sig, false);
		WritePtr(sig, false);
		WritePtr(num, 0);
		WritePtr(den, 0);
		return false;
	}
}

bool BR_Envelope::SetTimeSig (int id, bool sig, bool partial, int num, int den)
{
	if (this->ValidateId(id) && m_tempoMap)
	{
		if (sig && (!CheckBounds(num, MIN_SIG, MAX_SIG) || !CheckBounds(den, MIN_SIG, MAX_SIG)))
				return false;

		m_points[id].sig = (sig) ? ((den << 16) + num) : (0);
		m_points[id].partial = SetBit(m_points[id].partial, 0, sig);
		m_points[id].partial = SetBit(m_points[id].partial, 2, partial);
		return true;
	}
	else
		return false;
}

bool BR_Envelope::SetCreateSortedPoint (int id, double position, double value, int shape, double bezier, bool selected)
{
	position -= m_takeEnvOffset;

	if (id == -1)
	{
		id = this->FindNext(position, 0);
		BR_EnvPoint newPoint(position, value, (shape < 0 || shape > 5) ? this->DefaultShape() : shape, 0, selected, 0, (shape == 5) ? bezier : 0);
		m_points.insert(m_points.begin() + id, newPoint);

		++m_count;
		m_update = true;
		return true;
	}
	else if (this->ValidateId(id))
	{
		if ((this->ValidateId(id-1) && position < m_points[id-1].position) || (this->ValidateId(id+1) && position  > m_points[id+1].position))
			m_sorted = false;

		if (shape >= 0 && shape <= 5)
			m_points[id].shape = shape;

		m_points[id].position = position;
		m_points[id].value    = value;
		m_points[id].bezier   = (m_points[id].shape == 5) ? bezier : 0;
		m_points[id].selected = selected;

		m_update = true;
		return true;
	}
	else
	{
		return false;
	}
}

void BR_Envelope::UnselectAll ()
{
	for (int i = 0; i < m_count; ++i)
		m_points[i].selected = 0;
	m_update = true;
}

void BR_Envelope::UpdateSelected ()
{
	m_pointsSel.clear();
	m_pointsSel.reserve(m_count);

	for (int i = 0; i < m_count; ++i)
		if (m_points[i].selected)
			m_pointsSel.push_back(i);

	m_countSel = (int)m_pointsSel.size();
	m_countConseq = -1;
}

int BR_Envelope::CountSelected ()
{
	return m_countSel;
}

int BR_Envelope::GetSelected (int id)
{
	return m_pointsSel[id];
}

int BR_Envelope::CountConseq ()
{
	if (m_countConseq == -1)
		this->UpdateConsequential();
	return m_countConseq;
}

bool BR_Envelope::GetConseq (int idx, int* startId, int* endId)
{
	if (idx < 0 || idx >= this->CountConseq())
		return false;
	else
	{
		if (startId != NULL)
			*startId = m_pointsConseq[idx].first;
		if (endId != NULL)
			*endId = m_pointsConseq[idx].second;
		return true;
	}
}

bool BR_Envelope::ValidateId (int id)
{
	if (id < 0 || id >= m_count)
		return false;
	else
		return true;
}

void BR_Envelope::DeletePointsInRange (double start, double end)
{
	start -= m_takeEnvOffset;
	end   -= m_takeEnvOffset;

	if (m_sorted)
	{
		int startId = FindPrevious(start, 0);
		while (startId < m_count)
		{
			if (this->ValidateId(startId) && m_points[startId].position >= start)
				break;
			else
				++startId;
		}
		if (!this->ValidateId(startId))
			return;

		int endId = FindNext(end, 0);
		while (endId >= 0)
		{
			if (this->ValidateId(endId) && m_points[endId].position <= end)
				break;
			else
				--endId;
		}
		if (endId < startId)
			return;

		if (!this->ValidateId(endId))
			endId = m_count -1;
		if (this->DeletePoints(startId, endId))
			m_update = true;
	}
	else
	{
		for (vector<BR_EnvPoint>::iterator i = m_points.begin(); i != m_points.end();)
		{
			if (i->position >= start && i->position <= end)
			{
				i = m_points.erase(i);
				m_update = true;
			}
			else
				++i;
		}
	}

	m_count = m_points.size();
}

void BR_Envelope::DeleteAllPoints ()
{
	m_points.clear();
	m_sorted = true;
	m_update = true;
	m_count = 0;
}

void BR_Envelope::Sort ()
{
	if (!m_sorted)
	{
		stable_sort(m_points.begin(), m_points.end(), BR_EnvPoint::ComparePoints());
		m_sorted = true;
	}
}

int BR_Envelope::Count ()
{
	return m_count;
}

int BR_Envelope::Find (double position, double surroundingRange /*=0*/)
{
	position -= m_takeEnvOffset;
	int id = -1;

	if (m_sorted)
	{
		int prevId = this->FindPrevious(position, 0);
		int nextId = prevId + 1;
		double distanceFromPrev = (this->ValidateId(prevId)) ? (position - m_points[prevId].position) : (abs(surroundingRange) + 1);
		double distanceFromNext = (this->ValidateId(nextId)) ? (m_points[nextId].position - position) : (abs(surroundingRange) + 1);

		if (distanceFromPrev <= distanceFromNext)
		{
			if (distanceFromPrev <= surroundingRange)
				id = prevId;
		}
		else
		{
			if (distanceFromNext <= surroundingRange)
				id = nextId;
		}
	}
	else
	{
		int prevId = (m_sorted) ? (this->FindPrevious(position, 0)) : (0);

		for (vector<BR_EnvPoint>::iterator i = m_points.begin(); i != m_points.end() ; ++i)
		{
			if (i->position == position)
			{
				id = (int)(i - m_points.begin());
				break;
			}
		}

		// Search in range only if nothing has been found at the exact position
		if (id == -1 && surroundingRange != 0)
		{
			int nextId = this->FindNext(position, 0);
			double distanceFromPrev = (this->ValidateId(prevId)) ? (position - m_points[prevId].position) : (abs(surroundingRange) + 1);
			double distanceFromNext = (this->ValidateId(nextId)) ? (m_points[nextId].position - position) : (abs(surroundingRange) + 1);

			if (distanceFromPrev <= distanceFromNext)
			{
				if (distanceFromPrev <= surroundingRange)
					id = prevId;
			}
			else
			{
				if (distanceFromNext <= surroundingRange)
					id = nextId;
			}
		}
	}

	return id;
}

int BR_Envelope::FindNext (double position)
{
	return this->FindNext(position, m_takeEnvOffset);
}

int BR_Envelope::FindPrevious (double position)
{
	return this->FindPrevious(position, m_takeEnvOffset);
}

int BR_Envelope::FindClosest (double position)
{
	int prevId = this->FindPrevious(position, m_takeEnvOffset);
	if (!this->ValidateId(prevId))
	{
		int nextId = this->FindNext(position, m_takeEnvOffset);
		if (this->ValidateId(nextId))
			return nextId;
		else
			return -1;
	}
	else
	{
		int nextId = (m_sorted) ? prevId + 1 : this->FindNext(position, m_takeEnvOffset);
		if (!this->ValidateId(nextId))
			return prevId;
		else
		{
			double len1 = position - (m_points[prevId].position + m_takeEnvOffset);
			double len2 = (m_points[nextId].position + m_takeEnvOffset) - position;

			if (len1 >= len2)
				return nextId;
			else
				return prevId;
		}
	}
}

int BR_Envelope::GetSendId ()
{
	int id = -1;

	if (!this->IsTakeEnvelope())
	{
		MediaTrack* track    = this->GetParent();
		const char* sendType = (this->Type() == VOLUME) ? ("<VOLENV") : ((this->Type() == PAN) ? ("<PANENV") : ("<MUTEENV"));

		for (int i = 0; i < GetTrackNumSends(track, 0); ++i)
		{
			if (m_envelope == (TrackEnvelope*)GetSetTrackSendInfo(track, 0, i, "P_ENV", (void*)sendType))
			{
				id = i;
				break;
			}
		}
	}
	return id;
}

double BR_Envelope::ValueAtPosition (double position)
{
	position -= m_takeEnvOffset;
	int	id = this->FindPrevious(position, 0);

	// No previous point?
	if (!this->ValidateId(id))
	{
		int nextId = this->FindFirstPoint();
		if (this->ValidateId(nextId))
			return m_points[nextId].value;
		else
			return this->CenterValue();
	}

	// No next point?
	int nextId = (m_sorted) ? (id+1) : this->FindNext(m_points[id].position, 0);
	if (!this->ValidateId(nextId))
		return m_points[id].value;

	// Position at the end of transition ?
	if (m_points[nextId].position == position)
		return m_points[this->LastPointAtPos(nextId)].value;

	double t1 = m_points[id].position;
	double t2 = m_points[nextId].position;
	double v1 = m_points[id].value;
	double v2 = m_points[nextId].value;

	switch (m_points[id].shape)
	{
		case SQUARE:
		{
			return v1;
		}

		case LINEAR:
		{
			double t = (position - t1) / (t2 - t1);
			return (!m_tempoMap) ? (v1 + (v2 - v1) * t) : TempoAtPosition(v1, v2, t1, t2, position);
		}

		case FAST_END:                                 // f(x) = x^3
		{
			double t = (position - t1) / (t2 - t1);
			return v1 + (v2 - v1) * pow(t, 3);
		}

		case FAST_START:                               // f(x) = 1 - (1 - x)^3
		{
			double t = (position - t1) / (t2 - t1);
			return v1 + (v2 - v1) * (1 - pow(1-t, 3));
		}

		case SLOW_START_END:                           // f(x) = x^2 * (3-2x)
		{
			double t = (position - t1) / (t2 - t1);
			return v1 + (v2 - v1) * (pow(t, 2) * (3 - 2*t));
		}

		case BEZIER:
		{
			int id0 = (m_sorted) ? (id-1)     : (this->FindPrevious(t1, 0));
			int id3 = (m_sorted) ? (nextId+1) : (this->FindNext(t2, 0));
			double t0 = (!this->ValidateId(id0)) ? (t1) : (m_points[id0].position);
			double v0 = (!this->ValidateId(id0)) ? (v1) : (m_points[id0].value);
			double t3 = (!this->ValidateId(id3)) ? (t2) : (m_points[id3].position);
			double v3 = (!this->ValidateId(id3)) ? (v2) : (m_points[id3].value);

			double x1, x2, y1, y2, empty;
			LICE_Bezier_FindCardinalCtlPts(0.25, t0, t1, t2, v0, v1, v2, &empty, &x1, &empty, &y1);
			LICE_Bezier_FindCardinalCtlPts(0.25, t1, t2, t3, v1, v2, v3, &x2, &empty, &y2, &empty);

			double tension = m_points[id].bezier;
			x1 += tension * ((tension > 0) ? (t2-x1) : (x1-t1));
			x2 += tension * ((tension > 0) ? (t2-x2) : (x2-t1));
			y1 -= tension * ((tension > 0) ? (y1-v1) : (v2-y1));
			y2 -= tension * ((tension > 0) ? (y2-v1) : (v2-y2));

			x1 = SetToBounds(x1, t1, t2);
			x2 = SetToBounds(x2, t1, t2);
			y1 = SetToBounds(y1, this->MinValue(), this->MaxValue());
			y2 = SetToBounds(y2, this->MinValue(), this->MaxValue());
			return LICE_CBezier_GetY(t1, x1, x2, t2, v1, y1, y2, v2, position);
		}
	}
	return 0;
}

double BR_Envelope::NormalizedDisplayValue (double value)
{
	double min = this->LaneMinValue();
	double max = this->LaneMaxValue();
	double displayValue = SetToBounds(value, min, max);

	if (this->Type() == PLAYRATE)
	{
		if (displayValue > 1)
			return (2 + displayValue) / 6;      // original formula (max is always known so optimized): 0.5 + (displayValue - 1) / (max - 1) * 0.5
		else
			return (displayValue - 0.1) / 1.8 ; // original formula (min is always known so optimized): (displayValue - min) / (1 - min) * 0.5;
	}
	else
		return (displayValue - min) / (max - min);
}

double BR_Envelope::NormalizedDisplayValue (int id)
{
	if (this->ValidateId(id))
		return this->NormalizedDisplayValue(m_points[id].value);
	else
		return 0;
}

double BR_Envelope::RealDisplayValue (double normalizedValue)
{
	double min = this->LaneMinValue();
	double max = this->LaneMaxValue();
	normalizedValue = SetToBounds(normalizedValue, 0.0, 1.0);

	if (this->Type() == PLAYRATE)
	{
		if (normalizedValue > 0.5)
			return 6 * normalizedValue - 2;     // see BR_Envelope::NormalizedDisplayValue() for original formula
		else
			return 1.8 * normalizedValue + 0.1;
	}
	else
		return min + normalizedValue * (max - min);
}

double BR_Envelope::SnapValue (double value)
{
	if (this->Type() == PITCH)
	{
		int pitchenvrange; GetConfig("pitchenvrange", pitchenvrange);
		int mode = pitchenvrange >> 8; // range is in high 8 bits

		if      (mode == 0) return value;
		else if (mode == 1) return Round(value);             // 1 semitone
		else if (mode == 2) return Round(value * 2) / 2;     // 50 cent
		else if (mode == 3) return Round(value * 4) / 4;     // 25 cent
		else if (mode == 4) return Round(value * 10) / 10;   // 10 cent
		else if (mode == 5) return Round(value * 20) / 20;   // 5  cent
		else                return Round(value * 100) / 100; // 1 cents
	}
	else
	{
		return value;
	}
}

bool BR_Envelope::IsTempo ()
{
	return m_tempoMap;
}

bool BR_Envelope::IsTakeEnvelope ()
{
	if (m_take)
		return true;
	else
		return false;
}

bool BR_Envelope::GetPointsInTimeSelection (int* startId, int* endId, double* tStart /*=NULL*/, double* tEnd /*=NULL*/)
{
	double start, end;
	GetSet_LoopTimeRange2(NULL, false, false, &start, &end, false);
	WritePtr(tStart, start);
	WritePtr(tEnd, end);

	if (start != end)
	{
		double offset = (this->IsTakeEnvelope()) ? GetMediaItemInfo_Value(GetMediaItemTake_Item(m_take), "D_POSITION") : 0;

		if (startId)
		{
			int id = this->FindPrevious(start, offset) + 1;


			while (this->ValidateId(id))
			{
				if (m_points[id].position >= start)
					break;
				++id;
			}

			*startId = (this->ValidateId(id)) ? id : -1;
		}

		if (endId)
		{
			int id = this->FindNext(end, offset) - 1;

			while (this->ValidateId(id))
			{
				if (m_points[id].position <= end)
					break;
				--id;
			}

			*endId = (this->ValidateId(id)) ? id : -1;
		}

		return true;
	}
	else
	{
		WritePtr(startId, -1);
		WritePtr(endId,   -1);
		return false;
	}
}

bool BR_Envelope::VisibleInArrange (int* envHeight /*=NULL*/, int* yOffset /*= NULL*/, bool cacheValues /*=false*/)
{
	if (!this->IsVisible())
		return false;

	HWND hwnd = GetArrangeWnd();
	SCROLLINFO si = { sizeof(SCROLLINFO), };
	si.fMask = SIF_ALL;
	CoolSB_GetScrollInfo(hwnd, SB_VERT, &si);

	if (this->IsTakeEnvelope())
	{
		if (!cacheValues || (cacheValues && m_height == -1))
		{
			m_yOffset;
			m_height = GetTakeEnvHeight(m_take, &m_yOffset);
		}

		WritePtr(envHeight, m_height);
		WritePtr(yOffset, m_yOffset);

		int envelopeEnd = m_yOffset + m_height;
		int pageEnd = si.nPos + (int)si.nPage + SCROLLBAR_W;


		if (AreOverlappedEx(m_yOffset, envelopeEnd, si.nPos, pageEnd))
		{
			double arrangeStart, arrangeEnd;
			RECT r; GetWindowRect(hwnd, &r);
			GetSet_ArrangeView2(NULL, false, r.left, r.right-SCROLLBAR_W, &arrangeStart, &arrangeEnd);

			double itemStart = GetMediaItemInfo_Value(GetMediaItemTake_Item(m_take), "D_POSITION");
			double itemEnd = itemStart + GetMediaItemInfo_Value(GetMediaItemTake_Item(m_take), "D_LENGTH");

			if (AreOverlappedEx(itemStart, itemEnd, arrangeStart, arrangeEnd))
				return true;
		}
	}
	else
	{
		if (!cacheValues || (cacheValues && m_height == -1))
		{
			m_yOffset;
			m_height = GetTrackEnvHeight(m_envelope, &m_yOffset, true, this->GetParent());
		}

		WritePtr(envHeight, m_height);
		WritePtr(yOffset,   m_yOffset);

		if (m_height > 0)
		{
			int pageEnd = si.nPos + (int)si.nPage + SCROLLBAR_W;
			int envelopeEnd = m_yOffset + m_height;

			if (AreOverlappedEx(m_yOffset, envelopeEnd, si.nPos, pageEnd))
				return true;
		}
	}

	return false;
}

void BR_Envelope::MoveArrangeToPoint (int id, int referenceId)
{
	if (this->ValidateId(id))
	{
		double takePosOffset = (!this->IsTakeEnvelope()) ? (0) : GetMediaItemInfo_Value(GetMediaItemTake_Item(m_take), "D_POSITION");

		double pos = m_points[id].position + takePosOffset;
		if (this->ValidateId(referenceId))
			MoveArrangeToTarget(pos, m_points[referenceId].position + takePosOffset);
		else
			CenterArrange(pos);
	}
}

void BR_Envelope::SetTakeEnvelopeTimebase (bool useProjectTime)
{
	m_takeEnvOffset = (useProjectTime && this->IsTakeEnvelope()) ? (GetMediaItemInfo_Value(GetMediaItemTake_Item(m_take), "D_POSITION")) : (0);
}

void BR_Envelope::AddToPoints (double* position, double* value)
{
	for (vector<BR_EnvPoint>::iterator i = m_points.begin(); i != m_points.end(); ++i)
	{
		if (position)
		{
			i->position += *position;
			m_update = true;
		}
		if (value)
		{
			i->value += *value;
			m_update = true;
		}
	}
}

void BR_Envelope::AddToSelectedPoints (double* position, double* value)
{
	if (m_update)
	{
		for (size_t i = 0; i < m_points.size(); ++i)
		{
			if (m_points[i].selected)
			{
				if (position)
				{
					m_points[i].position += *position;
					m_sorted = false;
					m_update = true;
				}
				if (value)
				{
					m_points[i].value += *value;
					SetToBounds(m_points[i].value, this->LaneMinValue(), this->LaneMaxValue());
					m_update = true;
				}
			}
		}
	}
	else
	{
		for (size_t i = 0; i < m_pointsSel.size(); ++i)
		{
			int id = m_pointsSel[i];
			if (position)
			{
				m_points[id].position += *position;
				m_sorted = false;
				m_update = true;
			}
			if (value)
			{
				m_points[id].value += *value;
				m_update = true;
			}
		}
	}
}

void BR_Envelope::GetSelectedPointsExtrema (double* minimum, double* maximum)
{
	double minVal = 0;
	double maxVal = 0;

	if (m_update)
	{
		bool found = false;
		for (size_t i = 0; i < m_points.size(); ++i)
		{
			if (m_points[i].selected)
			{
				if (!found)
				{
					found = true;
					maxVal = m_points[i].value;
					minVal = m_points[i].value;
				}
				else
				{
					if (m_points[i].value > maxVal) maxVal = m_points[i].value;
					if (m_points[i].value < minVal) minVal = m_points[i].value;
				}
			}
		}
	}
	else
	{
		bool found = false;
		for (size_t i = 0; i < m_pointsSel.size(); ++i)
		{
			int id = m_pointsSel[i];
			if (!found)
			{
				found = true;
				maxVal = m_points[id].value;
				minVal = m_points[id].value;
			}
			else
			{
				if (m_points[id].value > maxVal) maxVal = m_points[id].value;
				if (m_points[id].value < minVal) minVal = m_points[id].value;
			}
		}
	}

	WritePtr(minimum, minVal);
	WritePtr(maximum, maxVal);
}

WDL_FastString BR_Envelope::FormatValue (double value)
{
	WDL_FastString formatedValue;

	if (this->Type() == VOLUME || this->Type() == VOLUME_PREFX)
	{
		static const char* s_unit = __localizeFunc("dB", "common", 0);

		value = VAL2DB(value);
		if (value == NEGATIVE_INF)
		{
			static const char* s_negativeInf = __localizeFunc("-inf", "vol", 0);
			formatedValue.AppendFormatted(256, "%s %s", s_negativeInf, s_unit);
		}
		else if (value == 0)
			formatedValue.AppendFormatted(256, "%#.2lg%s", value, s_unit);
		else if (value > -1 && value < 1)
			formatedValue.AppendFormatted(256, "%#+.2lg%s", value, s_unit);
		else
			formatedValue.AppendFormatted(256, "%#+.3lg%s", value, s_unit);
	}

	else if (this->Type() == PAN || this->Type() == PAN_PREFX)
	{
		static const char* s_unit   = __localizeFunc("%", "common", 0);
		if (value == 0)
		{
			static const char* s_center = __localizeFunc("center", "pan", 0);
			formatedValue.AppendFormatted(256, "%s", s_center);
		}
		else if (value > 0)
		{
			static const char* s_left   = __localizeFunc("L", "pan", 0);
			formatedValue.AppendFormatted(256, "%d%s%s", (int)(value*100), s_unit, s_left);
		}
		else
		{
			static const char* s_right  = __localizeFunc("R", "pan", 0);
			formatedValue.AppendFormatted(256, "%d%s%s", (int)(-value*100), s_unit, s_right);
		}
	}

	else if (this->Type() == WIDTH || this->Type() == WIDTH_PREFX)
	{
		static const char* s_unit = __localizeFunc("%", "common", 0);

		formatedValue.AppendFormatted(256, "%.1lf%s", value*100, s_unit);
	}

	else if (this->Type() == MUTE)
	{
		static const char* s_mute   = __localizeFunc("MUTE", "env", 0);
		static const char* s_unmute = __localizeFunc("UNMUTE", "env", 0);

		formatedValue.AppendFormatted(256, "%s", (value < 0.25) ? s_mute : s_unmute);
	}

	else if (this->Type() == PITCH)
	{
		static const char* s_unit = __localizeFunc("semitones", "env", 0);

		formatedValue.AppendFormatted(256, "%+.4lf %s", value, s_unit);
	}

	else if (this->Type() == PLAYRATE)
	{
		formatedValue.AppendFormatted(256, "%.2lf%s", value, "x");
	}

	else if (this->Type() == TEMPO)
	{
		static const char* s_unit = __localizeFunc(" bpm", "env", 0);
		formatedValue.AppendFormatted(256, "%.3lf%s", value, s_unit);
	}

	else if (this->Type() == PARAMETER)
	{
		formatedValue.AppendFormatted(256, "%lf", value);
	}

	return formatedValue;
}

WDL_FastString BR_Envelope::GetName ()
{
	if (!m_envName.GetLength())
	{
		char envName[64];
		GetEnvelopeName(m_envelope, envName, sizeof(envName));
		m_envName.AppendFormatted(sizeof(envName), "%s", envName);
	}
	return m_envName;
}

MediaItem_Take* BR_Envelope::GetTake ()
{
	return m_take;
}

MediaTrack* BR_Envelope::GetParent ()
{
	if (!m_parent)
	{
		if (this->IsTakeEnvelope())
			m_parent = GetMediaItemTake_Track(m_take);
		else
			m_parent = GetEnvParent(m_envelope);
	}
	return m_parent;
}

TrackEnvelope* BR_Envelope::GetPointer ()
{
	return m_envelope;
}

bool BR_Envelope::IsActive ()
{
	this->FillProperties();
	return !!m_properties.active;
}

bool BR_Envelope::IsVisible ()
{
	this->FillProperties();
	return !!m_properties.visible;
}

bool BR_Envelope::IsInLane ()
{
	this->FillProperties();
	return !!m_properties.lane;
}

bool BR_Envelope::IsArmed ()
{
	this->FillProperties();
	return !!m_properties.armed;
}

bool BR_Envelope::IsLocked ()
{
	bool locked = false;
	if (IsLockingActive())
	{
		if      (m_take)     locked = ::IsLocked(TAKE_ENV);
		else if (m_tempoMap) locked = ::IsLocked(this->IsVisible() ? TRACK_ENV : TEMPO_MARKERS);
		else                 locked = ::IsLocked(TRACK_ENV);
	}
	return locked;
}

int BR_Envelope::Type ()
{
	this->FillProperties();
	return m_properties.type;
}

int BR_Envelope::ParamId ()
{
	this->FillProperties();
	return m_properties.paramId;
}

int BR_Envelope::LaneHeight ()
{
	this->FillProperties();
	return m_properties.height;
}

int BR_Envelope::DefaultShape ()
{
	this->FillProperties();
	return m_properties.shape;
}

double BR_Envelope::MinValue ()
{
	this->FillProperties();
	return m_properties.minValue;
}

double BR_Envelope::MaxValue ()
{
	this->FillProperties();
	return m_properties.maxValue;
}

double BR_Envelope::CenterValue ()
{
	this->FillProperties();
	return m_properties.centerValue;
}

double BR_Envelope::LaneMaxValue ()
{
	if (m_tempoMap)
	{
		int max; GetConfig("tempoenvmax", max);
		return (double)max;
	}
	else if (this->Type() == VOLUME || this->Type() == VOLUME_PREFX)
	{
		int max; GetConfig("volenvrange", max);
		if (max == 1) return 1;
	}
	else if (this->Type() == PITCH)
	{
		int max; GetConfig("pitchenvrange", max);
		return max & 0x0F; // range is in low 8 bits
	}
	return this->MaxValue();
}

double BR_Envelope::LaneMinValue ()
{
	if (m_tempoMap)
	{
		int min; GetConfig("tempoenvmin", min);
		return (double)min;
	}
	else if (this->Type() == PITCH)
	{
		int min; GetConfig("pitchenvrange", min);
		return -(min & 0x0F); // range is in low 8 bits
	}
	return this->MinValue();
}

void BR_Envelope::SetActive (bool active)
{
	if (!m_properties.filled)
		this->FillProperties();
	m_properties.active = active;
	m_properties.changed = true;
	m_update = true;
}

void BR_Envelope::SetVisible (bool visible)
{
	if (!m_properties.filled)
		this->FillProperties();
	m_properties.visible = visible;
	m_properties.changed = true;
	m_update = true;
}

void BR_Envelope::SetInLane (bool lane)
{
	if (!this->IsTakeEnvelope())
	{
		if (!m_properties.filled)
			this->FillProperties();
		m_properties.lane = lane;
		m_properties.changed = true;
		m_update = true;
	}
}

void BR_Envelope::SetArmed (bool armed)
{
	if (!m_properties.filled)
		this->FillProperties();
	m_properties.armed = armed;
	m_properties.changed = true;
	m_update = true;
}

void BR_Envelope::SetLaneHeight (int height)
{
	if (!m_properties.filled)
		this->FillProperties();
	m_properties.height = height;
	m_properties.changed = true;
	m_update = true;
}

void BR_Envelope::SetDefaultShape (int shape)
{
	if (!m_properties.filled)
		this->FillProperties();
	m_properties.shape = shape;
	m_properties.changed = true;
	m_update = true;
}

bool BR_Envelope::Commit (bool force /*=false*/)
{
	if (force || (m_update && !this->IsLocked()))
	{
		// Prevents reselection of points in time selection
		int envClickSegMode; GetConfig("envclicksegmode", envClickSegMode);
		SetConfig("envclicksegmode", ClearBit(envClickSegMode, 6));

		WDL_FastString chunkStart = (m_properties.changed) ? this->GetProperties() : m_chunkStart;
		for (vector<BR_EnvPoint>::iterator i = m_points.begin(); i != m_points.end() ; ++i)
			i->Append(chunkStart);
		chunkStart.Append(m_chunkEnd.Get());

		if (this->IsTakeEnvelope()) // cast: FastString is faster/we're done with the object anyway
			GetSetEnvelopeState(m_envelope, const_cast<char*>(chunkStart.Get()), 0);
		else
			GetSetObjectState(m_envelope, chunkStart.Get());

		// UpdateTimeline() doesn't work when setting chunk with edited values
		if (m_tempoMap)
		{
			double t, b; int n, d; bool s;
			GetTempoTimeSigMarker(NULL, 0, &t, NULL, NULL, &b, &n, &d, &s);
			SetTempoTimeSigMarker(NULL, 0, t, -1, -1, b, n, d, s);
			UpdateTimeline();
		}

		SetConfig("envclicksegmode", envClickSegMode);
		m_update = false;
		return true;
	}
	return false;
}

int BR_Envelope::FindFirstPoint ()
{
	if (m_count <= 0)
		return -1;

	if (m_sorted)
		return 0;
	else
	{
		int id = 0;
		double first = m_points[id].position;
		for (vector<BR_EnvPoint>::iterator i = m_points.begin(); i != m_points.end() ; ++i)
			if (i->position < first)
				id = (int)(i - m_points.begin());
		return id;
	}
}

int BR_Envelope::LastPointAtPos (int id)
{
	/* no bounds checking - internal function so caller handles before calling */
	double position = m_points[id].position;
	int lastId = id;

	if (m_sorted)
	{
		for (vector<BR_EnvPoint>::iterator i = id + m_points.begin(); i != m_points.end() ; ++i)
		{
			if (i->position == position)
				lastId = (int)(i - m_points.begin());
			else
				break;
		}
	}
	else
	{
		for (vector<BR_EnvPoint>::iterator i = m_points.begin(); i != m_points.end() ; ++i)
			if (i->position == position)
				lastId = (int)(i - m_points.begin());
	}

	return lastId;
}

int BR_Envelope::FindNext (double position, double offset)
{
	position -= offset;

	if (m_sorted)
	{
		BR_EnvPoint val(position);
		return (int)(upper_bound(m_points.begin(), m_points.end(), val, BR_EnvPoint::ComparePoints()) - m_points.begin());
	}
	else
	{
		int id = -1;
		double nextPos = 0;
		bool foundFirst = false;
		for (vector<BR_EnvPoint>::iterator i = m_points.begin(); i != m_points.end() ; ++i)
		{
			double currentPos = i->position;
			if (currentPos > position)
			{
				if (foundFirst)
				{
					if (currentPos < nextPos)
					{
						nextPos = currentPos;
						id = (int)(i - m_points.begin());
					}
				}
				else
				{
					nextPos = currentPos;
					id = (int)(i - m_points.begin());
					foundFirst = true;
				}
			}
		}
		return id;
	}
}

int BR_Envelope::FindPrevious (double position, double offset)
{
	position -= offset;

	if (m_sorted)
	{
		BR_EnvPoint val(position);
		return (int)(lower_bound(m_points.begin(), m_points.end(), val, BR_EnvPoint::ComparePoints()) - m_points.begin())-1;
	}
	else
	{
		int id = -1;
		double prevPos = 0;
		bool foundFirst = false;
		for (vector<BR_EnvPoint>::iterator i = m_points.begin(); i != m_points.end() ; ++i)
		{
			double currentPos = i->position;
			if (currentPos < position)
			{
				if (foundFirst)
				{
					if (currentPos >= prevPos)
					{
						prevPos = currentPos;
						id = (int)(i - m_points.begin());
					}
				}
				else
				{
					prevPos = currentPos;
					id = (int)(i - m_points.begin());
					foundFirst = true;
				}
			}
		}
		return id;
	}
}

void BR_Envelope::ParseState (char* envState, size_t size)
{
	if (m_tempoMap)
	{
		int count = CountTempoTimeSigMarkers(NULL);
		m_points.reserve(count);
		m_pointsSel.reserve(count);
	}
	else
	{
		m_points.reserve(size/25);
		m_pointsSel.reserve(size/25);
	}

	char* token = strtok(envState, "\n");
	LineParser lp(false);
	bool start = false;
	int id = -1;
	while (token != NULL)
	{
		lp.parse(token);
		BR_EnvPoint point;
		if (point.ReadLine(lp))
		{
			++id;
			start = true;
			m_points.push_back(point);
			if (point.selected == 1)
				m_pointsSel.push_back(id);
		}
		else
		{
			if (!start)
				AppendLine(m_chunkStart, token);
			else
				AppendLine(m_chunkEnd, token);
		}
		token = strtok(NULL, "\n");
	}
}

void BR_Envelope::UpdateConsequential ()
{
	for (size_t i = 0; i < m_pointsSel.size(); ++i)
	{
		int startId = m_pointsSel[i];
		if (this->GetSelection(startId))
		{
			for (size_t j = i; j < m_pointsSel.size(); ++j)
			{
				int endId = m_pointsSel[j];
				if (this->GetSelection(endId) && !this->GetSelection(endId+1))
				{
					BR_Envelope::IdPair selection = {startId, endId};
					m_pointsConseq.push_back(selection);
					i += j-i;
					break;
				}
			}
		}
		m_countConseq = (int)m_pointsConseq.size();
	}
}

void BR_Envelope::FillProperties () const
{
	if (!m_properties.filled)
	{
		size_t size = m_chunkStart.GetLength()+1;
		char* chunk = new (nothrow) char[size];
		if (chunk != NULL)
		{
			memcpy(chunk, m_chunkStart.Get(), size);

			LineParser lp(false);
			char* token = strtok(chunk, "\n");
			while (token != NULL)
			{
				if (!strncmp(token, "ACT ", sizeof("ACT ")-1))
				{
					lp.parse(token);
					m_properties.active = lp.gettoken_int(1);
				}
				else if (!strncmp(token, "VIS ", sizeof("VIS ")-1))
				{
					lp.parse(token);
					m_properties.visible    = lp.gettoken_int(1);
					m_properties.lane       = lp.gettoken_int(2);
					m_properties.visUnknown = lp.gettoken_float(3);
				}
				else if (!strncmp(token, "LANEHEIGHT ", sizeof("LANEHEIGHT ")-1))
				{
					lp.parse(token);
					m_properties.height        = lp.gettoken_int(1);
					m_properties.heightUnknown = lp.gettoken_int(2);
				}
				else if (!strncmp(token, "ARM ", sizeof("ARM ")-1))
				{
					lp.parse(token);
					m_properties.armed = lp.gettoken_int(1);
				}
				else if (!strncmp(token, "DEFSHAPE ", sizeof("DEFSHAPE ")-1))
				{
					lp.parse(token);
					m_properties.shape         = lp.gettoken_int(1);
					m_properties.shapeUnknown1 = lp.gettoken_int(2);
					m_properties.shapeUnknown2 = lp.gettoken_int(3);
				}
				else if (strstr(token, "PARMENV"))
				{
					lp.parse(token);
					m_properties.paramId     = lp.gettoken_int(1);
					m_properties.minValue    = lp.gettoken_float(2);
					m_properties.maxValue    = lp.gettoken_float(3);
					m_properties.centerValue = lp.gettoken_float(4);
					m_properties.type = PARAMETER;
					m_properties.paramType.Set(token);
				}
				else if (strstr(token, "VOLENV"))
				{
					m_properties.minValue = 0;
					m_properties.maxValue = 2;
					m_properties.centerValue = 1;
					m_properties.type = (strstr(token, "AUXVOLENV") || strstr(token, "VOLENV2")) ? VOLUME : VOLUME_PREFX;
					m_properties.paramType.Set(token);
				}
				else if (strstr(token, "PANENV"))
				{
					m_properties.minValue = -1;
					m_properties.maxValue = 1;
					m_properties.centerValue = 0;
					m_properties.type = (strstr(token, "AUXPANENV") || strstr(token, "PANENV2")) ? PAN : PAN_PREFX;
					m_properties.paramType.Set(token);
				}
				else if (strstr(token, "WIDTHENV"))
				{
					m_properties.minValue = -1;
					m_properties.maxValue = 1;
					m_properties.centerValue = 0;
					m_properties.type = (strstr(token, "WIDTHENV2")) ? WIDTH : WIDTH_PREFX;
					m_properties.paramType.Set(token);
				}
				else if (strstr(token, "MUTEENV"))
				{
					m_properties.minValue = 0;
					m_properties.maxValue = 1;
					m_properties.centerValue = 0.5;
					m_properties.type = MUTE;
					m_properties.paramType.Set(token);
				}
				else if (strstr(token, "SPEEDENV"))
				{
					m_properties.minValue = 0.1;
					m_properties.maxValue = 4;
					m_properties.centerValue = 1;
					m_properties.type = PLAYRATE;
					m_properties.paramType.Set(token);
				}
				else if (strstr(token, "TEMPOENV"))
				{
					m_properties.minValue = MIN_BPM;
					m_properties.maxValue = MAX_BPM;
					m_properties.centerValue = GetProjectSettingsTempo(NULL, NULL);
					m_properties.type = TEMPO;
					m_properties.paramType.Set(token);
				}
				else if (m_take && strstr(token, "TRACK_ENVELOPE_UNKNOWN"))
				{
					if (m_takeEnvType == VOLUME)
					{
						m_properties.minValue = 0;
						m_properties.maxValue = 2;
						m_properties.centerValue = 1;
					}
					else if (m_takeEnvType == PAN)
					{
						m_properties.minValue = -1;
						m_properties.maxValue = 1;
						m_properties.centerValue = 0;

					}
					else if (m_takeEnvType == MUTE)
					{
						m_properties.minValue = 0;
						m_properties.maxValue = 1;
						m_properties.centerValue = 0.5;
					}
					else if (m_takeEnvType == PITCH)
					{
						m_properties.minValue = -255;
						m_properties.maxValue = 255;
						m_properties.centerValue = 0;
					}

					m_properties.type = m_takeEnvType;
					m_properties.paramType.Set(token);
				}

				token = strtok(NULL, "\n");
			}
			delete [] chunk;
			m_properties.filled = true;
		}
	}
}

WDL_FastString BR_Envelope::GetProperties ()
{
	WDL_FastString properties;
	properties.AppendFormatted(256, "%s\n", m_properties.paramType.Get());
	properties.AppendFormatted(256, "ACT %d\n", m_properties.active);
	properties.AppendFormatted(256, "VIS %d %d %lf\n", m_properties.visible, m_properties.lane, m_properties.visUnknown);
	properties.AppendFormatted(256, "LANEHEIGHT %d %d\n", m_properties.height, m_properties.heightUnknown);
	properties.AppendFormatted(256, "ARM %d\n", m_properties.armed);
	properties.AppendFormatted(256, "DEFSHAPE %d %d %d\n", m_properties.shape, m_properties.shapeUnknown1, m_properties.shapeUnknown2);
	return properties;
}

BR_Envelope::EnvProperties::EnvProperties () :
active        (0),
visible       (0),
lane          (0),
visUnknown    (0),
height        (0),
heightUnknown (0),
armed         (0),
shape         (0),
shapeUnknown1 (0),
shapeUnknown2 (0),
type          (0),
minValue      (0),
maxValue      (0),
centerValue   (0),
paramId       (-1),
filled        (false),
changed       (false)
{
}

BR_Envelope::EnvProperties::EnvProperties (const EnvProperties& properties) :
active        (properties.active),
visible       (properties.visible),
lane          (properties.lane),
visUnknown    (properties.visUnknown),
height        (properties.height),
heightUnknown (properties.heightUnknown),
armed         (properties.armed),
shape         (properties.shape),
shapeUnknown1 (properties.shapeUnknown1),
shapeUnknown2 (properties.shapeUnknown2),
type          (properties.type),
minValue      (properties.minValue),
maxValue      (properties.maxValue),
centerValue   (properties.centerValue),
paramId       (properties.paramId),
filled        (properties.filled),
changed       (properties.changed),
paramType     (properties.paramType)
{
}

BR_Envelope::EnvProperties& BR_Envelope::EnvProperties::operator= (const EnvProperties& properties)
{
	if (this == &properties)
		return *this;

	active        = properties.active;
	visible       = properties.visible;
	lane          = properties.lane;
	visUnknown    = properties.visUnknown;
	height        = properties.height;
	heightUnknown = properties.heightUnknown;
	armed         = properties.armed;
	shape         = properties.shape;
	shapeUnknown1 = properties.shapeUnknown1;
	shapeUnknown2 = properties.shapeUnknown2;
	type          = properties.type;
	minValue      = properties.minValue;
	maxValue      = properties.maxValue;
	centerValue   = properties.centerValue;
	paramId       = properties.paramId;
	filled        = properties.filled;
	changed       = properties.changed;
	paramType.Set(&properties.paramType);

	return *this;
}

/******************************************************************************
* Miscellaneous                                                               *
******************************************************************************/
TrackEnvelope* GetTempoEnv ()
{
	return GetTrackEnvelopeByName(CSurf_TrackFromID(0, false),  __localizeFunc("Tempo map", "env", 0));
}

TrackEnvelope* GetVolEnv (MediaTrack* track)
{
	return SWS_GetTrackEnvelopeByName(track, "Volume");
}

TrackEnvelope* GetVolEnvPreFX (MediaTrack* track)
{
	return SWS_GetTrackEnvelopeByName(track, "Volume (Pre-FX)");
}

TrackEnvelope* GetTakeEnv (MediaItem_Take* take, BR_EnvType envelope)
{
	char* envName = NULL;
	if      (envelope == VOLUME) envName = "Volume";
	else if (envelope == PAN)    envName = "Pan";
	else if (envelope == MUTE)   envName = "Mute";
	else if (envelope == PITCH)  envName = "Pitch";

	return SWS_GetTakeEnvelopeByName(take, envName);
}

MediaItem_Take* GetTakeEnvParent (TrackEnvelope* envelope, int* type)
{
	if (envelope)
	{
		int itemCount = CountMediaItems(NULL);
		for (int i = 0; i < itemCount; ++i)
		{
			MediaItem* item = GetMediaItem(NULL, i);
			int takeCount = CountTakes(item);
			for (int j = 0; j < takeCount; ++j)
			{
				MediaItem_Take* take = GetTake(item, j);
				int returnType = UNKNOWN;

				if      (GetTakeEnv(take, VOLUME) == envelope) returnType = VOLUME;
				else if (GetTakeEnv(take, PAN)    == envelope) returnType = PAN;
				else if (GetTakeEnv(take, MUTE)   == envelope) returnType = MUTE;
				else if (GetTakeEnv(take, PITCH)  == envelope) returnType = PITCH;

				if (returnType != UNKNOWN)
				{
					WritePtr(type, returnType);
					return take;
				}
			}
		}
	}

	WritePtr(type, (int)UNKNOWN);
	return NULL;
}

MediaTrack* GetEnvParent (TrackEnvelope* envelope)
{
	int count = CountTracks(NULL);
	for (int i = -1; i < count; ++i)
	{
		MediaTrack* track = (i == -1) ? (GetMasterTrack(NULL)) : (GetTrack(NULL, i));

		int count = CountTrackEnvelopes(track);
		for (int i = 0; i < count; ++i)
			if (envelope == GetTrackEnvelope(track, i))
				return track;
	}
	return NULL;
}

vector<int> GetSelPoints (TrackEnvelope* envelope)
{
	char* envState = GetSetObjectState(envelope, "");
	char* token = strtok(envState, "\n");

	vector<int> selectedPoints;
	LineParser lp(false);
	int id = -1;
	while (token != NULL)
	{
		lp.parse(token);
		if (!strcmp(lp.gettoken_str(0), "PT"))
		{
			++id;
			if (lp.gettoken_int(5))
				selectedPoints.push_back(id);
		}
		token = strtok(NULL, "\n");
	}
	FreeHeapPtr(envState);
	return selectedPoints;
}

WDL_FastString ConstructReceiveEnv (int type, double firstPointValue)
{
	WDL_FastString envelope;

	if (type != VOLUME && type != PAN && type != MUTE)
		return envelope;

	int defAutoMode; GetConfig("defautomode", defAutoMode);
	int envLanes;    GetConfig("envlanes", envLanes);
	int defShape = (type == MUTE) ? SQUARE : GetDefaultPointShape();

	if      (type == VOLUME) AppendLine(envelope, "<AUXVOLENV");
	else if (type == PAN)    AppendLine(envelope, "<AUXPANENV");
	else if (type == MUTE)   AppendLine(envelope, "<AUXMUTEENV");

	envelope.AppendFormatted(128, "%s %d\n",             "ACT", 1);
	envelope.AppendFormatted(128, "%s %d %d %d\n",       "VIS", 1, GetBit(envLanes, 0), 1);
	envelope.AppendFormatted(128, "%s %d %d\n",          "LANEHEIGHT", 0, 0);
	envelope.AppendFormatted(128, "%s %d\n",             "ARM", !GetBit(defAutoMode, 9));
	envelope.AppendFormatted(128, "%s %d %d %d\n",       "DEFSHAPE", defShape, -1, -1);
	envelope.AppendFormatted(128, "%s %.8lf %.8lf %d\n", "PT", 0.0, firstPointValue, defShape);
	AppendLine(envelope, ">");

	return envelope;
}

bool ToggleShowSendEnvelope (MediaTrack* track, int sendId, int type)
{

	MediaTrack* receiveTrack = (MediaTrack*)GetSetTrackSendInfo(track, 0, sendId, "P_DESTTRACK", NULL);

	bool update = false;
	if (receiveTrack && (type == VOLUME || type == PAN || type == MUTE))
	{
		int sendTrackId = CSurf_TrackToID(track, false) - 1; // -1 so it's the same id receives use

		int sendNum = 0; // in case there are multiple sends to same receive track
		for (int i = 0; i < GetTrackNumSends(track, 0); ++i)
		{
			if (i > sendId)
				break;

			if ((MediaTrack*)GetSetTrackSendInfo(track, 0, i, "P_DESTTRACK", NULL) == receiveTrack)
				++sendNum;
		}

		bool stateUpdated = false;
		WDL_FastString newState;
		char* trackState = GetSetObjectState(receiveTrack, "");

		if (trackState)
		{
			int blockCount = 0;
			int currentSendNum = 0;

			LineParser lp(false);
			char* token = strtok(trackState, "\n");
			while (token != NULL)
			{
				lp.parse(token);
				if      (lp.gettoken_str(0)[0] == '<')  ++blockCount;
				else if (lp.gettoken_str(0)[0] == '>')  --blockCount;

				if (blockCount == 1 && !strcmp(lp.gettoken_str(0), "AUXRECV") && lp.gettoken_int(1) == sendTrackId)
				{
					++currentSendNum;
					if (currentSendNum == sendNum)
					{
						int sendTrackId   = lp.gettoken_int(1);
						double sendVolume = lp.gettoken_float(3);
						double sendPan    = -lp.gettoken_float(4);
						double sendMute   = (double)!lp.gettoken_float(5);

						WDL_FastString receiveLine, volEnv, panEnv, muteEnv;
						receiveLine.Set(token);

						token = strtok(NULL, "\n");
						while (token != NULL)
						{
							lp.parse(token);
							if (!strcmp(lp.gettoken_str(0), "<AUXVOLENV") || !strcmp(lp.gettoken_str(0), "<AUXPANENV") || !strcmp(lp.gettoken_str(0), "<AUXMUTEENV"))
							{
								WDL_FastString* currentEnvState = NULL;
								int currentEnvType = 0;

								if      (!strcmp(lp.gettoken_str(0), "<AUXVOLENV"))  {currentEnvType = VOLUME; currentEnvState = &volEnv;}
								else if (!strcmp(lp.gettoken_str(0), "<AUXPANENV"))  {currentEnvType = PAN;    currentEnvState = &panEnv;}
								else if (!strcmp(lp.gettoken_str(0), "<AUXMUTEENV")) {currentEnvType = MUTE;   currentEnvState = &muteEnv;}

								// Save current send envelope
								while (token != NULL)
								{
									lp.parse(token);

									if (!strcmp(lp.gettoken_str(0), "VIS") && (currentEnvType == type))
									{
										for (int i = 0; i < lp.getnumtokens(); ++i)
										{
											if (i == 1)
											{
												if (lp.gettoken_int(1) == 0)
													currentEnvState->Append("1");
												else
													currentEnvState->Append("0");
												stateUpdated = true;
											}
											else
												currentEnvState->Append(lp.gettoken_str(i));

											currentEnvState->Append(" ");
										}
										currentEnvState->Append("\n");
									}
									else
										AppendLine(*currentEnvState, token);

									if (lp.gettoken_str(0)[0] == '>')
										break;
									else
										token = strtok(NULL, "\n");
								}
							}
							else
								break;

							token = strtok(NULL, "\n");
						}

						bool trim = false;
						int trimMode; GetConfig("envtrimadjmode", trimMode);
						if (trimMode == 0 || (trimMode == 1 && GetCurrentAutomationMode(CSurf_TrackFromID(sendTrackId + 1, false)) != 0))
						{
							trim = true;

							WDL_FastString newReceiveLine;
							lp.parse(receiveLine.Get());
							for (int i = 0; i < lp.getnumtokens(); ++i)
							{
								if      (i == 3 && (type == VOLUME) && !volEnv.GetLength())  newReceiveLine.Append("1");
								else if (i == 4 && (type == PAN)    && !panEnv.GetLength())  newReceiveLine.Append("0");
								else                                                         newReceiveLine.Append(lp.gettoken_str(i));

								newReceiveLine.Append(" ");
							}
							newReceiveLine.Append("\n");

							AppendLine(newState, newReceiveLine.Get());
						}
						else
							AppendLine(newState, receiveLine.Get());

						if ((type == VOLUME) && !volEnv.GetLength())  {volEnv  = ConstructReceiveEnv(VOLUME, trim ? sendVolume : 1); stateUpdated = true;}
						if ((type == PAN)    && !panEnv.GetLength())  {panEnv  = ConstructReceiveEnv(PAN,    trim ? sendPan : 0);    stateUpdated = true;}
						if ((type == MUTE)   && !muteEnv.GetLength()) {muteEnv = ConstructReceiveEnv(MUTE,   sendMute);              stateUpdated = true;}

						if (volEnv.GetLength())  newState.Append(volEnv.Get());
						if (panEnv.GetLength())  newState.Append(panEnv.Get());
						if (muteEnv.GetLength()) newState.Append(muteEnv.Get());
					}
				}

				AppendLine(newState, token);
				token = strtok(NULL, "\n");
			}
		}

		if (stateUpdated)
		{
			GetSetObjectState(receiveTrack, newState.Get());
			update = true;
		}
		FreeHeapPtr(trackState);
	}

	return update;
}

bool ShowSendEnvelopes (vector<MediaTrack*>& tracks, int envelopes)
{
	if (!(envelopes & MUTE) && !(envelopes & PAN) && !(envelopes & VOLUME))
		return false;

	set<int> trackIds;
	vector<MediaTrack*> receiveTracks;
	for (size_t i = 0; i < tracks.size(); ++i)
	{
		MediaTrack* track = tracks[i];
		int id = CSurf_TrackToID(track, false);
		if (id > 0)
		{
			trackIds.insert(id - 1);  // -1 so it's the same id receives use

			for (int j = 0; j < GetTrackNumSends(track, 0); ++j)
				receiveTracks.push_back((MediaTrack*)GetSetTrackSendInfo(track, 0, j, "P_DESTTRACK", NULL));
		}
	}
	if (trackIds.size() == 0)
		return false;


	int trimMode; GetConfig("envtrimadjmode", trimMode);
	bool update = false;

	for (size_t i = 0; i < receiveTracks.size(); i++)
	{
		if (MediaTrack* track = receiveTracks[i])
		{
			bool stateUpdated = false;
			WDL_FastString newState;

			char* trackState = GetSetObjectState(track, "");
			if (trackState)
			{
				LineParser lp(false);
				int blockCount = 0;

				char* token = strtok(trackState, "\n");
				while (token != NULL)
				{
					lp.parse(token);
					if      (lp.gettoken_str(0)[0] == '<')  ++blockCount;
					else if (lp.gettoken_str(0)[0] == '>')  --blockCount;

					if (blockCount == 1 && !strcmp(lp.gettoken_str(0), "AUXRECV") && trackIds.find(lp.gettoken_int(1)) != trackIds.end())
					{
						int sendTrackId   = lp.gettoken_int(1);
						double sendVolume = lp.gettoken_float(3);
						double sendPan    = -lp.gettoken_float(4);
						double sendMute   = (double)!lp.gettoken_float(5);

						WDL_FastString receiveLine, volEnv, panEnv, muteEnv;
						receiveLine.Set(token);

						token = strtok(NULL, "\n");
						while (token != NULL)
						{
							lp.parse(token);
							if (!strcmp(lp.gettoken_str(0), "<AUXVOLENV") || !strcmp(lp.gettoken_str(0), "<AUXPANENV") || !strcmp(lp.gettoken_str(0), "<AUXMUTEENV"))
							{
								WDL_FastString* currentEnvState = NULL;
								int currentEnv = 0;

								if      (!strcmp(lp.gettoken_str(0), "<AUXVOLENV"))  {currentEnv = VOLUME; currentEnvState = &volEnv;}
								else if (!strcmp(lp.gettoken_str(0), "<AUXPANENV"))  {currentEnv = PAN;    currentEnvState = &panEnv;}
								else if (!strcmp(lp.gettoken_str(0), "<AUXMUTEENV")) {currentEnv = MUTE;   currentEnvState = &muteEnv;}

								// Save current send envelope
								while (token != NULL)
								{
									lp.parse(token);

									if (!strcmp(lp.gettoken_str(0), "VIS") && (envelopes & currentEnv))
									{
										for (int i = 0; i < lp.getnumtokens(); ++i)
										{
											if (i == 1)
											{
												if (lp.gettoken_int(1) == 0)
													stateUpdated = true;
												currentEnvState->Append("1");
											}
											else
												currentEnvState->Append(lp.gettoken_str(i));

											currentEnvState->Append(" ");
										}
										currentEnvState->Append("\n");
									}
									else
										AppendLine(*currentEnvState, token);

									if (lp.gettoken_str(0)[0] == '>')
										break;
									else
										token = strtok(NULL, "\n");
								}
							}
							else
								break;

							token = strtok(NULL, "\n");
						}

						bool trim = false;
						if (trimMode == 0 || (trimMode == 1 && GetCurrentAutomationMode(CSurf_TrackFromID(sendTrackId + 1, false)) != 0))
						{
							trim = true;

							WDL_FastString newReceiveLine;
							lp.parse(receiveLine.Get());
							for (int i = 0; i < lp.getnumtokens(); ++i)
							{
								if      (i == 3 && (envelopes & VOLUME) && !volEnv.GetLength())  newReceiveLine.Append("1");
								else if (i == 4 && (envelopes & PAN)    && !panEnv.GetLength())  newReceiveLine.Append("0");
								else                                                             newReceiveLine.Append(lp.gettoken_str(i));

								newReceiveLine.Append(" ");
							}
							newReceiveLine.Append("\n");

							AppendLine(newState, newReceiveLine.Get());
						}
						else
							AppendLine(newState, receiveLine.Get());

						if ((envelopes & VOLUME) && !volEnv.GetLength())  {volEnv  = ConstructReceiveEnv(VOLUME, trim ? sendVolume : 1); stateUpdated = true;}
						if ((envelopes & PAN)    && !panEnv.GetLength())  {panEnv  = ConstructReceiveEnv(PAN,    trim ? sendPan : 0);    stateUpdated = true;}
						if ((envelopes & MUTE)   && !muteEnv.GetLength()) {muteEnv = ConstructReceiveEnv(MUTE,   sendMute);              stateUpdated = true;}

						if (volEnv.GetLength())  newState.Append(volEnv.Get());
						if (panEnv.GetLength())  newState.Append(panEnv.Get());
						if (muteEnv.GetLength()) newState.Append(muteEnv.Get());
					}

					AppendLine(newState, token);
					token = strtok(NULL, "\n");
				}
			}

			if (stateUpdated)
			{
				GetSetObjectState(track, newState.Get());
				update = true;
			}
			FreeHeapPtr(trackState);
		}
	}

	return update;
}

bool EnvVis (TrackEnvelope* envelope, bool* lane)
{
	if (char* tmp = new (nothrow) char[128])
	{
		tmp[0] = 0;
		GetSetEnvelopeState(envelope, tmp, 128);
		LineParser lp(false);

		char* token = strtok(tmp, "\n");
		while (token != NULL)
		{
			lp.parse(token);
			if (!strcmp(lp.gettoken_str(0), "VIS"))
			{
				delete[] tmp;
				WritePtr(lane, !!lp.gettoken_int(2));
				return !!lp.gettoken_int(1);
			}
			token = strtok(NULL, "\n");
		}
		delete[] tmp;
	}

	WritePtr(lane, false);
	return false;
}

int GetEnvId (TrackEnvelope* envelope, MediaTrack* parent /*= NULL*/)
{
	MediaTrack* track = (parent) ? (parent) : (GetEnvParent(envelope));

	int count = CountTrackEnvelopes(track);
	for (int i = 0; i < count; ++i)
	{
		if (envelope == GetTrackEnvelope(track, i))
			return i;
	}
	return -1;
}

int GetDefaultPointShape ()
{
	int defEnvs; GetConfig("defenvs", defEnvs);
	return defEnvs >> 16;
}

int GetEnvType (TrackEnvelope* envelope, bool* isSend)
{
	static const char* volumePreFX = __localizeFunc("Volume (Pre-FX)", "envname", 0);
	static const char* panPreFX    = __localizeFunc("Pan (Pre-FX)", "envname", 0);
	static const char* widthPreFX  = __localizeFunc("Width (Pre-FX)", "envname", 0);
	static const char* volume      = __localizeFunc("Volume", "envname", 0);
	static const char* pan         = __localizeFunc("Pan", "envname", 0);
	static const char* width       = __localizeFunc("Width", "envname", 0);
	static const char* mute        = __localizeFunc("Mute", "envname", 0);
	static const char* sendVolume  = __localizeFunc("Send Volume", "envname", 0);
	static const char* sendPan     = __localizeFunc("Send Pan", "envname", 0);
	static const char* sendMute    = __localizeFunc("Send Mute", "envname", 0);
	static const char* takePitch   = __localizeFunc("Pitch", "item", 0);
	static const char* takeVolume  = __localizeFunc("Volume", "item", 0);
	static const char* takePan     = __localizeFunc("Pan", "item", 0);
	static const char* takeMute    = __localizeFunc("Mute", "item", 0);
	static const char* playrate    = __localizeFunc("Playrate", "env", 0);
	static const char* tempo       = __localizeFunc("Tempo map", "env", 0);

	char name[512];
	GetEnvelopeName(envelope, name, sizeof(name));

	int type  = PARAMETER;
	bool send = false;
	if      (!strcmp(name, volumePreFX)) type = VOLUME_PREFX;
	else if (!strcmp(name, panPreFX))    type = PAN_PREFX;
	else if (!strcmp(name, widthPreFX))  type = WIDTH_PREFX;
	else if (!strcmp(name, volume))      type = VOLUME;
	else if (!strcmp(name, pan))         type = PAN;
	else if (!strcmp(name, width))       type = WIDTH;
	else if (!strcmp(name, mute))        type = MUTE;
	else if (!strcmp(name, sendVolume))  {type = VOLUME; send = true;}
	else if (!strcmp(name, sendPan))     {type = PAN;    send = true;}
	else if (!strcmp(name, sendMute))    {type = MUTE;   send = true;}
	else if (!strcmp(name, takePitch))   type = PITCH;
	else if (!strcmp(name, takeVolume))  type = VOLUME;
	else if (!strcmp(name, takePan))     type = PAN;
	else if (!strcmp(name, takeMute))    type = MUTE;
	else if (!strcmp(name, playrate))    type = PLAYRATE;
	else if (!strcmp(name, tempo))       type = TEMPO;
	else                                 type = PARAMETER;

	WritePtr(isSend, send);
	return type;
}

int GetCurrentAutomationMode (MediaTrack* track)
{
	int override = GetGlobalAutomationOverride();
	if (override == -1 || override == 5)
		return (int)GetMediaTrackInfo_Value(track, "I_AUTOMODE");
	else
		return override;
}

/******************************************************************************
* Tempo                                                                       *
******************************************************************************/
int FindPreviousTempoMarker (double position)
{
	int first = 0;
	int last = CountTempoTimeSigMarkers(NULL);

	while (first != last)
	{
		int mid = (first + last) / 2;
		double currentPos; GetTempoTimeSigMarker(NULL, mid, &currentPos, NULL, NULL, NULL, NULL, NULL, NULL);

		if (currentPos < position) first = mid + 1;
		else                       last  = mid;

	}
	return first - 1;
}

int FindNextTempoMarker (double position)
{
	int first = 0;
	int last = CountTempoTimeSigMarkers(NULL);
	int count = last;

	while (first != last)
	{
		int mid = (first + last) /2;
		double currentPos; GetTempoTimeSigMarker(NULL, mid, &currentPos, NULL, NULL, NULL, NULL, NULL, NULL);

		if (position >= currentPos) first = mid + 1;
		else                        last  = mid;

	}
	return (first < count) ? first: -1;
}

int FindClosestTempoMarker (double position)
{
	int prevId = FindPreviousTempoMarker(position);
	int nextId = prevId + 1;

	int count = CountTempoTimeSigMarkers(NULL);
	if (prevId == -1)
	{
		return (nextId < count) ? nextId : -1;
	}
	else
	{
		if (nextId < count)
		{

			double prevPos, nextPos;
			GetTempoTimeSigMarker(NULL, prevId, &prevPos, NULL, NULL, NULL, NULL, NULL, NULL);
			GetTempoTimeSigMarker(NULL, nextId, &nextPos, NULL, NULL, NULL, NULL, NULL, NULL);

			double len1 = position - prevPos;
			double len2 = nextPos - position;

			if (len1 >= len2)
				return nextId;
			else
				return prevId;
		}
		else
		{
			return prevId;
		}
	}
}

int FindTempoMarker (double position, double surroundingRange /*= 0*/)
{
	int count = CountTempoTimeSigMarkers(NULL);
	if (count == 0)
		return -1;

	int prevId = FindPreviousTempoMarker(position);
	int nextId = prevId + 1;

	double prevPos, nextPos;
	GetTempoTimeSigMarker(NULL, prevId, &prevPos, NULL, NULL, NULL, NULL, NULL, NULL);
	GetTempoTimeSigMarker(NULL, nextId, &nextPos, NULL, NULL, NULL, NULL, NULL, NULL);

	double distanceFromPrev = (CheckBounds(prevId, 0, count-1)) ? (position - prevPos) : (abs(surroundingRange) + 1);
	double distanceFromNext = (CheckBounds(nextId, 0, count-1)) ? (nextPos - position) : (abs(surroundingRange) + 1);

	int id = -1;
	if (distanceFromPrev <= distanceFromNext)
	{
		if (distanceFromPrev <= surroundingRange)
			id = prevId;
	}
	else
	{
		if (distanceFromNext <= surroundingRange)
			id = nextId;
	}
	return (id < count) ? id : -1;
}

double AverageProjTempo ()
{
	if (int count = CountTempoTimeSigMarkers(NULL))
	{
		double t, b; int lastPoint = count - 1;
		GetTempoTimeSigMarker(NULL, lastPoint, &t, NULL, NULL, &b, NULL, NULL, NULL);
		double len = EndOfProject(false, false);
		if (t > len)
			len = t;

		if (len != 0)
			return TimeMap_timeToQN(len) * 60 / len;
		else
			return b;
	}
	else
	{
		double b;
		TimeMap_GetTimeSigAtTime(NULL, 0, NULL, NULL, &b);
		return b;
	}
}

double TempoAtPosition (double startBpm, double endBpm, double startTime, double endTime, double targetTime)
{
	return startBpm + (endBpm-startBpm) / (endTime-startTime) * (targetTime-startTime);
}

double MeasureAtPosition (double startBpm, double endBpm, double timeLen, double targetTime)
{
	// Return number of measures counted from the musical position of startBpm
	return targetTime * (targetTime * (endBpm-startBpm) + 2*timeLen*startBpm) / (480*timeLen);
}

double PositionAtMeasure (double startBpm, double endBpm, double timeLen, double targetMeasure)
{
	// Return number of seconds counted from the time position of startBpm
	double a = startBpm - endBpm;
	double b = timeLen * startBpm;
	double c = 480 * timeLen * targetMeasure;
	return c / (b + sqrt(pow(b,2) - a*c));  // more stable solution for quadratic equation
}

void FindMiddlePoint (double* middleTime, double* middleBpm, double measure, double startTime, double endTime, double startBpm, double endBpm)
{
	double f = 480 * (measure/2);
	double a = startBpm - endBpm;
	double b = a*(startTime+endTime) / 2 + f;
	double c = a*(startTime*endTime) + f*(startTime+endTime);

	double pos = c / (b + sqrt(pow(b,2) - a*c));
	double bpm = f / (pos-startTime) - startBpm; // when editing tempo we usually move forward so previous point
	WritePtr(middleTime, pos);                   // remains the same and calculating the middle point in relation
	WritePtr(middleBpm, bpm);                    // to it will make middle point land on the correct musical position
}

void SplitMiddlePoint (double* time1, double* time2, double* bpm1, double* bpm2, double splitRatio, double measure, double startTime, double middleTime, double endTime, double startBpm, double middleBpm, double endBpm)
{
	// First point
	double temp = measure * (1-splitRatio) / 2;
	double pos1 = PositionAtMeasure(startBpm, middleBpm, (middleTime-startTime), temp) + startTime;
	double val1 = 480*temp / (pos1-startTime) - startBpm;
	WritePtr(time1, pos1);
	WritePtr(bpm1, val1);

	// Second point
	double f1 = 480 * measure*splitRatio;
	double f2 = 480 * temp;
	double a = val1-endBpm;
	double b = (a*(pos1+endTime) + f1+f2) / 2;
	double c = a*(pos1*endTime) + f1*endTime + f2*pos1;

	double pos2 = c / (b + sqrt(pow(b,2) - a*c));
	double val2 = f1 / (pos2-pos1) - val1;
	WritePtr(time2, pos2);
	WritePtr(bpm2, val2);
}