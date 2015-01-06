/******************************************************************************
/ wol_Util.cpp
/
/ Copyright (c) 2014 wol
/ http://forum.cockos.com/member.php?u=70153
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
#include "../Breeder/BR_EnvelopeUtil.h"
#include "../SnM/SnM_Dlg.h"



///////////////////////////////////////////////////////////////////////////////////////////////////
/// Track
///////////////////////////////////////////////////////////////////////////////////////////////////
static vector<MediaTrack*>* g_savedSelectedTracks = NULL;
static bool g_free;

bool SaveSelectedTracks()
{
	if (g_savedSelectedTracks)
		return false;
	g_savedSelectedTracks = new (nothrow)vector < MediaTrack* > ;
	if (g_savedSelectedTracks)
	{
		for (int i = 0; i < CountSelectedTracks(NULL); ++i)
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

void SetTrackHeight(MediaTrack* track, int height)
{
	GetSetMediaTrackInfo(track, "I_HEIGHTOVERRIDE", &height);
	PreventUIRefresh(1);
	Main_OnCommand(41327, 0);
	Main_OnCommand(41328, 0);
	PreventUIRefresh(-1);
}



///////////////////////////////////////////////////////////////////////////////////////////////////
/// Tcp
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

// Stolen from BR_Util.cpp
void ScrollToTrackEnvelopeIfNotInArrange(TrackEnvelope* envelope)
{
	int offsetY;
	int height = GetTrackEnvHeight(envelope, &offsetY, false, NULL);

	HWND hwnd = GetArrangeWnd();
	SCROLLINFO si = { sizeof(SCROLLINFO), };
	si.fMask = SIF_ALL;
	CoolSB_GetScrollInfo(hwnd, SB_VERT, &si);

	int envEnd = offsetY + height;
	int pageEnd = si.nPos + (int)si.nPage + SCROLLBAR_W;

	if (offsetY < si.nPos || envEnd > pageEnd)
	{
		si.nPos = offsetY;
		CoolSB_SetScrollInfo(hwnd, SB_VERT, &si, true);
		SendMessage(hwnd, WM_VSCROLL, si.nPos << 16 | SB_THUMBPOSITION, NULL);
	}
}

// Overloads ScrollToTrackIfNotInArrange(MediaTrack* track) in BR_Util.cpp
void ScrollToTrackIfNotInArrange(TrackEnvelope* envelope)
{
	if (MediaTrack* tr = GetEnvParent(envelope))
		ScrollToTrackIfNotInArrange(tr);
	else if (MediaItem_Take* tk = GetTakeEnvParent(envelope, NULL))
		if (MediaTrack* tr = GetMediaItemTake_Track(tk))
			ScrollToTrackIfNotInArrange(tr);
}

void ScrollToTakeIfNotInArrange(MediaItem_Take* take)
{
	int offsetY;
	int height = GetTakeHeight(take, &offsetY);

	HWND hwnd = GetArrangeWnd();
	SCROLLINFO si = { sizeof(SCROLLINFO), };
	si.fMask = SIF_ALL;
	CoolSB_GetScrollInfo(hwnd, SB_VERT, &si);

	int envEnd = offsetY + height;
	int pageEnd = si.nPos + (int)si.nPage + SCROLLBAR_W;

	if (offsetY < si.nPos || envEnd > pageEnd)
	{
		si.nPos = offsetY;
		CoolSB_SetScrollInfo(hwnd, SB_VERT, &si, true);
		SendMessage(hwnd, WM_VSCROLL, si.nPos << 16 | SB_THUMBPOSITION, NULL);
	}
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

// Return   -1 for envelope not in track lane or not visible, *laneCount and *envCount not modified
//			 0 for overlap disabled, *laneCount = *envCount = number of lanes (same as visible envelopes)
//			 1 for overlap enabled, envelope height < overlap limit -> single lane (one envelope visible), *laneCount = *envCount = 1
//			 2 for overlap enabled, envelope height < overlap limit -> single lane (overlapping), *laneCount = 1, *envCount = number of visible envelopes
//			 3 for overlap enabled, envelope height > overlap limit -> single lane (one envelope visible), *laneCount = *envCount = 1
//			 4 for overlap enabled, envelope height > overlap limit -> multiple lanes, *laneCount = *envCount = number of lanes (same as visible envelopes)
int GetEnvelopeOverlapState(TrackEnvelope* envelope, int* laneCount, int* envCount)
{
	bool lane, visible;
	visible = EnvVis(envelope, &lane);
	if (!visible || lane)
		return -1;

	int visEnvCount = CountVisibleTrackEnvelopesInTrackLane(GetEnvParent(envelope));
	int overlapMinHeight = *(int*)GetConfigVar("env_ol_minh");
	if (overlapMinHeight < 0)
		return (WritePtr(laneCount, visEnvCount), WritePtr(envCount, visEnvCount), 0);

	if (GetTrackEnvHeight(envelope, NULL, false) < overlapMinHeight)
		return (WritePtr(laneCount, 1), WritePtr(envCount, visEnvCount), (visEnvCount > 1) ? 2 : 1);

	return (WritePtr(laneCount, (visEnvCount > 1) ? visEnvCount : 1), WritePtr(envCount, (visEnvCount > 1) ? visEnvCount : 1), (visEnvCount > 1) ? 4 : 3);
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