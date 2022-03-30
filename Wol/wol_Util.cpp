/******************************************************************************
/ wol_Util.cpp
/
/ Copyright (c) 2014 and later wol
/ http://forum.cockos.com/member.php?u=70153
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
#include "wol_Util.h"
#include "../Breeder/BR_Util.h"
#include "../cfillion/cfillion.hpp" // CF_GetScrollInfo
#include "../SnM/SnM_Dlg.h"
#include "../SnM/SnM_Util.h"



///////////////////////////////////////////////////////////////////////////////////////////////////
/// Track
///////////////////////////////////////////////////////////////////////////////////////////////////
static vector<MediaTrack*>* g_savedSelectedTracks = NULL;

bool SaveSelectedTracks()
{
	if (g_savedSelectedTracks)
		return false;
	g_savedSelectedTracks = new (nothrow)vector < MediaTrack* > ;
	if (g_savedSelectedTracks)
	{
		const int count = CountSelectedTracks(NULL);
		for (int i = 0; i < count; ++i)
			if (MediaTrack* tr = GetSelectedTrack(NULL, i))
				g_savedSelectedTracks->push_back(tr);
		return true;
	}
	return false;
}

bool RestoreSelectedTracks()
{
	if (!g_savedSelectedTracks)
		return false;
	for (size_t i = 0; i < g_savedSelectedTracks->size(); ++i)
		SetTrackSelected(g_savedSelectedTracks->at(i), true);
	g_savedSelectedTracks->clear();
	DELETE_NULL(g_savedSelectedTracks);
	return true;
}

void SetTrackHeight(MediaTrack* track, int height, bool useChunk)
{
	if (!useChunk)
	{
		GetSetMediaTrackInfo(track, "I_HEIGHTOVERRIDE", &height);

		TrackList_AdjustWindows(false);
	}
	else
	{
		SNM_ChunkParserPatcher p(track);
		char pTrackLine[BUFFER_SIZE] = "";

		if (p.Parse(SNM_GET_CHUNK_CHAR, 1, "TRACK", "TRACKHEIGHT", 0, 1, pTrackLine))
		{
			snprintf(pTrackLine, BUFFER_SIZE, "%d", height);
			p.ParsePatch(SNM_SET_CHUNK_CHAR, 1, "TRACK", "TRACKHEIGHT", 0, 1, pTrackLine);
		}
	}
}



///////////////////////////////////////////////////////////////////////////////////////////////////
/// Arrange/TCP
///////////////////////////////////////////////////////////////////////////////////////////////////
int GetTcpEnvMinHeight()
{
	if (IconTheme* it = SNM_GetIconTheme())
		return it->envcp_min_height;
	return 0;
}

int GetTcpTrackMinHeight()
{
	if (IconTheme* it = SNM_GetIconTheme())
		return it->tcp_small_height;
	return 0;
}

int GetCurrentTcpMaxHeight()
{
	RECT r;
	GetClientRect(GetArrangeWnd(), &r);
	return r.bottom - r.top;
}

void SetArrangeScroll(int offsetY, int height, VerticalZoomCenter center)
{
	HWND hwnd = GetArrangeWnd();
	SCROLLINFO si = { sizeof(SCROLLINFO), };
	si.fMask = SIF_ALL;
	CF_GetScrollInfo(hwnd, SB_VERT, &si);

	int newPos = si.nPos;
	const int objUpperHalf = offsetY + (height / 4);
	const int objMid = offsetY + (height / 2);
	const int objLowerHalf = offsetY + (height / 4) * 3;
	const int objEnd = offsetY + height;
	const int pageEnd = si.nPos + (int)si.nPage + SCROLLBAR_W;

	switch (center)
	{
	case CLIP_IN_ARRANGE:
	{
		if (offsetY < si.nPos)
			newPos = offsetY;
		else if (objEnd > pageEnd)
			newPos = objEnd - (int)si.nPage - SCROLLBAR_W;
		break;
	}
	case MID_ARRANGE:
	{
		newPos = objMid - (int)(si.nPage / 2 + 0.5f) - (int)(SCROLLBAR_W / 2 + 0.5f);
		break;
	}
	case MOUSE_CURSOR:
	{
		POINT cr;
		GetCursorPos(&cr);
		ScreenToClient(hwnd, &cr);
		newPos = objMid - cr.y;
		break;
	}
	case UPPER_HALF:
	{
		newPos = objUpperHalf - (int)(si.nPage / 2 + 0.5f) - (int)(SCROLLBAR_W / 2 + 0.5f);
		break;
	}
	case LOWER_HALF:
	{
		newPos = objLowerHalf - (int)(si.nPage / 2 + 0.5f) - (int)(SCROLLBAR_W / 2 + 0.5f);
		break;
	}
	}

	if (newPos == si.nPos)
		return;
	else if (newPos < si.nMin)
		newPos = si.nMin;
	else if (newPos > si.nMax)
		newPos = si.nMax - si.nPage + 1;

	si.nPos = newPos;
	CoolSB_SetScrollInfo(hwnd, SB_VERT, &si, true);
	SendMessage(hwnd, WM_VSCROLL, si.nPos << 16 | SB_THUMBPOSITION, 0);
}

void SetArrangeScrollTo(MediaTrack* track, VerticalZoomCenter center)
{
	int offsetY;
	int height = GetTrackHeight(track, &offsetY);
	SetArrangeScroll(offsetY, height, center);
}

inline void SetArrangeScrollTo(MediaItem* item, VerticalZoomCenter center)
{
	SetArrangeScrollTo(GetMediaItemTrack(item), center);
}

inline void SetArrangeScrollTo(MediaItem_Take* take, VerticalZoomCenter center)
{
	SetArrangeScrollTo(GetMediaItemTake_Item(take), center);
}

// With check = false the envelope is assumed to be in its lane
void SetArrangeScrollTo(TrackEnvelope* envelope, bool check, VerticalZoomCenter center)
{
	bool doScroll = !check;
	if (check)
	{
		BR_Envelope env(envelope);
		if (env.IsInLane())
		{
			if (env.IsTakeEnvelope())
			{
				if (MediaItem_Take* tk = env.GetTake())
					SetArrangeScrollTo(tk, center);
			}
			else
				doScroll = true;
		}
		else
			if (MediaTrack* tr = env.GetParent())
				SetArrangeScrollTo(tr, center);
	}

	if (doScroll)
	{
		int offsetY;
		int height = GetTrackEnvHeight(envelope, &offsetY, false, NULL);
		SetArrangeScroll(offsetY, height, center);
	}
}

void SetArrangeScrollTo(BR_Envelope* envelope, VerticalZoomCenter center)
{
	if (envelope->IsInLane())
	{
		if (envelope->IsTakeEnvelope())
		{
			if (MediaItem_Take* tk = envelope->GetTake())
				SetArrangeScrollTo(tk, center);
		}
		else
			SetArrangeScrollTo(envelope->GetPointer(), false, center);
	}
	else
		if (MediaTrack* tr = envelope->GetParent())
			SetArrangeScrollTo(tr, center);
}



///////////////////////////////////////////////////////////////////////////////////////////////////
/// Envelope
///////////////////////////////////////////////////////////////////////////////////////////////////
int CountVisibleTrackEnvelopesInTrackLane(MediaTrack* track)
{
	int count = 0;
	for (int i = 0; i < CountTrackEnvelopes(track); i++)
	{
		bool visible, lane;
		visible = EnvVis(GetTrackEnvelope(track, i), &lane);
		if (visible && !lane)
			count++;
	}
	return count;
}

int GetEnvelopeOverlapState(TrackEnvelope* envelope, int* laneCount, int* envCount)
{
	bool lane, visible;
	visible = EnvVis(envelope, &lane);
	if (!visible || lane)
		return -1;

	int visEnvCount = CountVisibleTrackEnvelopesInTrackLane(GetEnvParent(envelope));
	int overlapMinHeight = *ConfigVar<int>("env_ol_minh");
	if (overlapMinHeight < 0)
		return (WritePtr(laneCount, visEnvCount), WritePtr(envCount, visEnvCount), 0);

	if (GetTrackEnvHeight(envelope, NULL, false) < overlapMinHeight)
		return (WritePtr(laneCount, 1), WritePtr(envCount, visEnvCount), (visEnvCount > 1) ? 2 : 1);

	return (WritePtr(laneCount, (visEnvCount > 1) ? visEnvCount : 1), WritePtr(envCount, (visEnvCount > 1) ? visEnvCount : 1), (visEnvCount > 1) ? 4 : 3);
}

void PutSelectedEnvelopeInLane(COMMAND_T* ct)
{
	if (TrackEnvelope* env = GetSelectedEnvelope(NULL))
	{
		BR_Envelope brEnv(env);
		brEnv.SetInLane(!!(int)ct->user);
		brEnv.Commit();
	}
}

bool IsEnvelopeInMediaLane(TrackEnvelope* env)
{
	if (!env)
	{
		env = GetSelectedEnvelope(NULL);
		if (!env)
			return false;
	}

	BR_Envelope brEnv(env);
	return !(brEnv.IsInLane());
}



///////////////////////////////////////////////////////////////////////////////////////////////////
/// Midi
///////////////////////////////////////////////////////////////////////////////////////////////////
// from SnM.cpp
// http://forum.cockos.com/project.php?issueid=4576
int AdjustRelative(int _adjmode, int _reladj)
{
	if (_adjmode == 1) { if (_reladj >= 0x40) _reladj |= ~0x3f; } // sign extend if 0x40 set
	else if (_adjmode == 2) { _reladj -= 0x40; } // offset by 0x40
	else if (_adjmode == 3) { if (_reladj & 0x40) _reladj = -(_reladj & 0x3f); } // 0x40 is sign bit
	else _reladj = 0;
	return _reladj;
}
