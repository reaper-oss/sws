/******************************************************************************
/ BR_EnvTools.cpp
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
#include "BR_EnvTools.h"
#include "BR_Util.h"
#include "../../WDL/lice/lice_bezier.h"
#include "../reaper/localize.h"

/******************************************************************************
* BR_EnvPoint                                                                 *
******************************************************************************/
BR_EnvPoint::BR_EnvPoint (double position)
{
	this->position = position;
}

BR_EnvPoint::BR_EnvPoint (double position, double value, int shape, int sig, int selected, int partial, double bezier)
{
	this->position = position;
	this->value    = value;
	this->shape    = shape;
	this->sig      = sig;
	this->selected = selected;
	this->partial  = partial;
	this->bezier   = bezier;
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
		"PT %.14lf %.14lf %d %d %d %d %.14lf\n",
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
BR_Envelope::BR_Envelope (TrackEnvelope* envelope) :
m_envelope (envelope),
m_take     (NULL),
m_parent   (NULL),
m_tempoMap (envelope == GetTempoEnv()),
m_update   (false),
m_sorted   (true)
{
	if (m_envelope)
	{
		char* envState = GetSetObjectState(m_envelope, "");
		this->ParseState(envState, strlen(envState));
		FreeHeapPtr(envState);
	}

	this->SetCounts();
}

BR_Envelope::BR_Envelope (MediaItem_Take* take, BR_EnvType envType) :
m_envelope (GetTakeEnv(take, envType)),
m_take     (take),
m_parent   (NULL),
m_tempoMap (false),
m_update   (false),
m_sorted   (true)
{
	int size = 32768; // should fit around 1000 points (depending if bezier)
	while (char* envState = new (nothrow) char[size])
	{
		envState[0] = 0;
		if (GetSetEnvelopeState(m_envelope, envState, size))
		{
			int envSize = strlen(envState);
			if (envSize < size-1)
			{
				this->ParseState(envState, envSize);
				delete[] envState;
				break;
			}
		}
		else
		{
			delete[] envState;
			break;
		}
		size *= 2;
		delete[] envState;
	}

	this->SetCounts();
}

bool BR_Envelope::GetPoint (int id, double* position, double* value, int* shape, double* bezier)
{
	if (this->ValidateId(id))
	{
		WritePtr(position, m_points[id].position);
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

bool BR_Envelope::SetPoint (int id, double* position, double* value, int* shape, double* bezier)
{
	if (this->ValidateId(id))
	{
		ReadPtr(position, m_points[id].position);
		ReadPtr(value,    m_points[id].value   );
		ReadPtr(shape,    m_points[id].shape   );
		ReadPtr(bezier,   m_points[id].bezier  );
		m_update = true;
		if (position)
			m_sorted = false;
		return true;
	}
	else
		return false;
}

bool BR_Envelope::GetSelection (int id)
{
	if (this->ValidateId(id))
	{
		if (m_points[id].selected)
			return true;
		else
			return false;
	}
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

bool BR_Envelope::CreatePoint (int id, double position, double value, int shape, double bezier, bool selected)
{
	if (this->ValidateId(id) || id == m_count)
	{
		BR_EnvPoint newPoint(position, value, shape, 0, selected, 0, bezier);
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
		m_points.erase(m_points.begin()+id);
		--m_count;
		m_update = true;
		return true;
	}
	else
		return false;
}

bool BR_Envelope::GetTimeSig (int id, bool* sig, int* num, int* den)
{
	if (this->ValidateId(id) && m_tempoMap)
	{
		WritePtr(sig, (m_points[id].sig) ? (true) : (false));

		// Search for active time signature of a point
		if (num != NULL || den != NULL)
		{
			// Look backwards for the first tempo marker with time signature
			int effSig = 0;
			for (;id >= 0; --id)
			{
				if (m_points[id].sig != 0)
				{
					effSig = m_points[id].sig;
					break;
				}
			}

			int effNum, effDen;
			if (effSig == 0)
				TimeMap_GetTimeSigAtTime(NULL, -1, &effNum, &effDen, NULL);
			else
			{
				effNum = effSig & 0xff; // num is low 16 bits
				effDen = effSig >> 16;  // den is high 16 bits
			}
			WritePtr(num, effNum);
			WritePtr(den, effDen);
		}
		return true;
	}
	else
	{
		WritePtr(sig, false);
		WritePtr(num, 0);
		WritePtr(den, 0);
		return false;
	}
}

bool BR_Envelope::SetTimeSig (int id, bool sig, int num, int den)
{
	if (this->ValidateId(id) && m_tempoMap)
	{
		int effSig;
		if (sig)
		{
			// Unlike native method, this function will fail when illegal num/den are requested
			if (num < MIN_SIG || num > MAX_SIG || den < MIN_SIG || den > MAX_SIG)
				return false;

			effSig = (den << 16) + num;         // Partial token is set according to time signature:
			if (m_points[id].partial == 0)      // -sig -partial
				m_points[id].partial = 1;       // +sig -partial
			else if (m_points[id].partial == 4) // -sig +partial
				m_points[id].partial = 5;       // +sig +partial
		}
		else
		{
			effSig = 0;
			if (m_points[id].partial == 1)      // +sig -partial
				m_points[id].partial = 0;       // -sig -partial
			else if (m_points[id].partial == 5) // +sig +partial
				m_points[id].partial = 4;       // -sig +partial
		}
		m_points[id].sig = effSig;
		return true;
	}
	else
		return false;
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

void BR_Envelope::Sort ()
{
	stable_sort(m_points.begin(), m_points.end(), BR_EnvPoint::ComparePoints());
	m_sorted = true;
}

int BR_Envelope::Count ()
{
	return m_count;
}

int BR_Envelope::Find (double position, double surroundingRange /*=0*/)
{
	int id = -1;
	int prevId = (m_sorted) ? (this->FindPrevious(position)) : (0);

	// First search for the exact position
	if (m_sorted)
	{
		int nextId = prevId + 1;
		if (this->ValidateId(nextId) && m_points[nextId].position == position)
			id = nextId;
	}
	else
	{
		for (vector<BR_EnvPoint>::iterator i = m_points.begin(); i != m_points.end() ; ++i)
		{
			if (i->position == position)
			{
				id = (int)(i - m_points.begin());
				break;
			}
		}
	}

	// Search in range only if nothing has been found at the exact position
	if (id == -1 && surroundingRange != 0)
	{
		prevId     = (m_sorted) ? (prevId)     : (this->FindPrevious(position));
		int nextId = (m_sorted) ? (prevId + 1) : (this->FindNext(position));
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

	return id;
}

int BR_Envelope::FindNext (double position)
{
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

int BR_Envelope::FindPrevious (double position)
{
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

double BR_Envelope::ValueAtPosition (double position)
{
	int id = this->FindPrevious(position);

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
	int nextId = (m_sorted) ? (id+1) : this->FindNext(m_points[id].position);
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

		case SLOW_START_END:                            // f(x) = x^2 * (3-2x)
		{
			double t = (position - t1) / (t2 - t1);
			return v1 + (v2 - v1) * (pow(t, 2) * (3 - 2*t));
		}

		case BEZIER:
		{
			int id0 = (m_sorted) ? (id-1)     : (this->FindPrevious(t1));
			int id3 = (m_sorted) ? (nextId+1) : (this->FindNext(t2));
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

			x1 = CheckBounds(x1, t1, t2);
			x2 = CheckBounds(x2, t1, t2);
			y1 = CheckBounds(y1, this->MinValue(), this->MaxValue());
			y2 = CheckBounds(y2, this->MinValue(), this->MaxValue());
			return LICE_CBezier_GetY(t1, x1, x2, t2, v1, y1, y2, v2, position);
		}
	}
	return 0;
}

void BR_Envelope::MoveArrangeToPoint (int id, int referenceId)
{
	if (this->ValidateId(id))
	{
		double pos = m_points[id].position;
		if (this->ValidateId(referenceId))
			MoveArrangeToTarget(pos, m_points[referenceId].position);
		else
			CenterArrange(pos);
	}
}

bool BR_Envelope::VisibleInArrange ()
{
	if (!this->IsVisible())
		return false;

	HWND hwnd = GetArrangeWnd();
	SCROLLINFO si = { sizeof(SCROLLINFO), };
	si.fMask = SIF_ALL;
	CoolSB_GetScrollInfo(hwnd, SB_VERT, &si);

	if (!m_take)
	{
		int offset;
		int height = GetTrackEnvHeight (m_envelope, &offset, this->GetParent());
		int pageEnd = si.nPos + (int)si.nPage + VERT_SCROLL_W;
		int envelopeEnd = offset + height;

		if (offset >= si.nPos && offset <= pageEnd)
			return true;
		if (envelopeEnd >= si.nPos && envelopeEnd <= pageEnd)
			return true;
	}
	else
	{
		int offset;
		int height = GetTakeEnvHeight (m_take, &offset);
		int envelopeEnd = offset + height;
		int pageEnd = si.nPos + (int)si.nPage + VERT_SCROLL_W;

		if ((offset >= si.nPos && offset <= pageEnd) || (envelopeEnd >= si.nPos && envelopeEnd <= pageEnd))
		{
			double arrangeStart, arrangeEnd;
			RECT r; GetWindowRect(hwnd, &r);
			GetSet_ArrangeView2(NULL, false, r.left, r.right-VERT_SCROLL_W, &arrangeStart, &arrangeEnd);

			double itemStart = GetMediaItemInfo_Value(GetMediaItemTake_Item(m_take), "D_POSITION");
			double itemEnd = itemStart + GetMediaItemInfo_Value(GetMediaItemTake_Item(m_take), "D_LENGTH");

			if (itemStart <= arrangeEnd && itemEnd >= arrangeStart)
				return true;
		}
	}
	return false;
}

bool BR_Envelope::IsTempo ()
{
	return m_tempoMap;
}

MediaTrack* BR_Envelope::GetParent ()
{
	if (!m_parent)
	{
		if (m_take)
			m_parent = GetMediaItem_Track(GetMediaItemTake_Item(m_take));
		else
			m_parent = GetEnvParent(m_envelope);
	}
	return m_parent;
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

int BR_Envelope::LaneHeight ()
{
	this->FillProperties();
	return m_properties.height;
}

int BR_Envelope::Type ()
{
	this->FillProperties();
	return m_properties.type;
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
	if (!m_take)
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
	if (m_update || force)
	{
		// Prevents reselection of points in time selection
		int envClickSegMode; GetConfig("envclicksegmode", envClickSegMode);
		SetConfig("envclicksegmode", ClearBit(envClickSegMode, 6));

		// Build and commit chunk
		WDL_FastString chunkStart = (m_properties.changed) ? this->GetProperties() : m_chunkStart;
		for (vector<BR_EnvPoint>::iterator i = m_points.begin(); i != m_points.end() ; ++i)
			i->Append(chunkStart);
		chunkStart.Append(m_chunkEnd.Get());

		if (m_take) // cast: FastString is faster/we're done with the object anyway
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

void BR_Envelope::ParseState (char* envState, size_t size)
{
	// Reserve storage to reduce overhead (guessing for everything but tempo map)
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

	// Parse envelope
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
			id++;
			start = true;
			m_points.push_back(point);
			if (point.selected == 1)
				m_pointsSel.push_back(id);
		}
		else
		{
			if (!start)
			{
				m_chunkStart.Append(token);
				m_chunkStart.Append("\n");
			}
			else
			{
				m_chunkEnd.Append(token);
				m_chunkEnd.Append("\n");
			}
		}
		token = strtok(NULL, "\n");
	}
}

void BR_Envelope::SetCounts ()
{
	m_count = (int)m_points.size();
	m_countSel = (int)m_pointsSel.size();
	m_countConseq = -1;
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

void BR_Envelope::FillProperties ()
{
	if (!m_properties.filled)
	{
		size_t size = m_chunkStart.GetLength()+1;
		char* chunk = new (nothrow) char[size];
		if (chunk != NULL)
		{
			memcpy(chunk, m_chunkStart.Get(), size);

			char* token = strtok(chunk, "\n");
			while (token != NULL)
			{
				if (!strncmp(token, "ACT ", sizeof("ACT ")-1))
				{
					sscanf(token, "ACT %d", &m_properties.active);
				}
				else if (!strncmp(token, "VIS ", sizeof("VIS ")-1))
				{
					sscanf(token, "VIS %d %d %lf", &m_properties.visible, &m_properties.lane, &m_properties.visUnknown);
				}
				else if (!strncmp(token, "LANEHEIGHT ", sizeof("LANEHEIGHT ")-1))
				{
					sscanf(token, "LANEHEIGHT %d %d", &m_properties.height, &m_properties.heightUnknown);
				}
				else if (!strncmp(token, "ARM ", sizeof("ARM ")-1))
				{
					sscanf(token, "ARM %d", &m_properties.armed);
				}
				else if (!strncmp(token, "DEFSHAPE ", sizeof("DEFSHAPE ")-1))
				{
					sscanf(token, "DEFSHAPE %d %d %d", &m_properties.shape, &m_properties.shapeUnknown1, &m_properties.shapeUnknown2);
				}
				else if (strstr(token, "PARMENV"))
				{
					sscanf(token, "<PARMENV %*s %lf %lf %lf", &m_properties.minValue, &m_properties.maxValue, &m_properties.centerValue);
					m_properties.type = PARAMETER;
					m_properties.paramType.Set(token);
				}
				else if (strstr(token, "VOLENV"))
				{
					m_properties.minValue = 0;
					m_properties.maxValue = 2;
					m_properties.centerValue = 1;
					m_properties.type = VOLUME;
					m_properties.paramType.Set(token);
				}
				else if (strstr(token, "PANENV"))
				{
					m_properties.minValue = -1;
					m_properties.maxValue = 1;
					m_properties.centerValue = 0;
					m_properties.type = PAN;
					m_properties.paramType.Set(token);
				}
				else if (strstr(token, "WIDTHENV"))
				{
					m_properties.minValue = -1;
					m_properties.maxValue = 1;
					m_properties.centerValue = 0;
					m_properties.type = WIDTH;
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

BR_Envelope::EnvProperties::EnvProperties ()
{
	active = 0;
	visible = lane = 0;
	visUnknown = 0;
	height = heightUnknown = 0;
	armed = 0;
	shape = shapeUnknown1 = shapeUnknown2 = 0;
	type = 0;
	minValue = maxValue = centerValue = 0;
	filled = false;
	changed = false;
}

/******************************************************************************
* Miscellaneous                                                               *
******************************************************************************/
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

TrackEnvelope* GetTakeEnv (MediaItem_Take* take, BR_EnvType envelope)
{
	char* envName = NULL;
	if      (envelope == VOLUME) envName = "Volume";
	else if (envelope == PAN)    envName = "Pan";
	else if (envelope == MUTE)   envName = "Mute";
	else if (envelope == PITCH)  envName = "Pitch";

	return SWS_GetTakeEnvelopeByName(take, envName);
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

/******************************************************************************
* Tempo                                                                       *
******************************************************************************/
TrackEnvelope* GetTempoEnv ()
{
	return GetTrackEnvelopeByName(CSurf_TrackFromID(0, false),  __localizeFunc("Tempo map", "env", 0));
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
	double bpm = f / (pos-startTime) - startBpm; // when editing tempo we usually move forward so previous point remains
	WritePtr(middleTime, pos);                   // the same and calculating the middle point in relation to it will
	WritePtr(middleBpm, bpm);                    // make middle point land on the correct musical position
}

void SplitMiddlePoint (double* time1, double* time2, double* bpm1, double* bpm2, double splitRatio, double measure, double startTime, double middleTime, double endTime, double startBpm, double middleBpm, double endBpm)
{
	// First point
	double temp = measure * (1-splitRatio) / 2;
	double pos1 = PositionAtMeasure(startBpm, middleBpm, (middleTime-startTime), temp) + startTime;
	double val1 = 480*temp / (pos1-startTime) - startBpm;
	WritePtr (time1, pos1);
	WritePtr (bpm1, val1);

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