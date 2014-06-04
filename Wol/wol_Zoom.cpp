/******************************************************************************
/ wol_Zoom.cpp
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
#include "wol_Zoom.h"
#include "../sws_util.h"
#include "../SnM/SnM_Dlg.h"
#include "../Breeder/BR_Util.h"
#include "../Breeder/BR_EnvTools.h"

bool EnvelopesExtendedZoom = false;
int SavedEnvelopeOverlapSettings = 0;



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

int GetTcpEnvMinHeight()
{
	return SNM_GetIconTheme()->envcp_min_height;
}

int GetTcpTrackMinHeight()
{
	return SNM_GetIconTheme()->tcp_small_height;
}

int GetCurrentTcpMaxHeight()
{
	RECT r;
	GetClientRect(GetArrangeWnd(), &r);
	return r.bottom - r.top;
}

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

// Return    -1 for envelope not in track lane or not visible, *laneCount and *envCount not modified
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

void SetTrackHeight(MediaTrack* track, int height)
{
	GetSetMediaTrackInfo(track, "I_HEIGHTOVERRIDE", &height);
	PreventUIRefresh(1);
	Main_OnCommand(41327, 0);
	Main_OnCommand(41328, 0);
	PreventUIRefresh(-1);
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
	ScrollToTrackIfNotInArrange(GetEnvParent(envelope));
}

void AdjustSelectedEnvelopeOrTrackHeight(COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd)
{
	if (relmode > 0)
	{
		void(*ScrollTo)(TrackEnvelope*) = ScrollToTrackEnvelopeIfNotInArrange;
		int height = AdjustRelative(relmode, (valhw == -1) ? BOUNDED(val, 0, 127) : (int)BOUNDED(16384.0 - (valhw | val << 7), 0.0, 16383.0));
		if (TrackEnvelope* env = GetSelectedEnvelope(NULL))
		{
			BR_Envelope brEnv(env);
			if (brEnv.IsInLane())
			{
				if (brEnv.IsTakeEnvelope())
				{
					SetTrackHeight(brEnv.GetParent(), SetToBounds(height + GetTrackHeight(brEnv.GetParent(), NULL), GetTcpTrackMinHeight(), GetCurrentTcpMaxHeight()));
					ScrollTo = ScrollToTrackIfNotInArrange;
				}
				else
				{
					brEnv.SetLaneHeight(SetToBounds(height + GetTrackEnvHeight(env, NULL, false), GetTcpEnvMinHeight(), GetCurrentTcpMaxHeight()));
					brEnv.Commit();
				}
			}
			else
			{
				int trackGapTop, trackGapBottom, mul, currentHeight, laneCount, envCount;
				currentHeight = GetTrackHeight(brEnv.GetParent(), NULL, &trackGapTop, &trackGapBottom);
				GetEnvelopeOverlapState(env, &laneCount, &envCount);
				mul = EnvelopesExtendedZoom ? laneCount : 1;
				if (mul == 1)
					ScrollTo = ScrollToTrackIfNotInArrange;

				SetTrackHeight(brEnv.GetParent(), SetToBounds(height * mul + currentHeight, GetTcpTrackMinHeight(), (EnvelopesExtendedZoom ? GetCurrentTcpMaxHeight() * envCount + trackGapTop + trackGapBottom : GetCurrentTcpMaxHeight())));
			}
			ScrollTo(env);
		}
		else if ((int)ct->user == 1)
		{
			if (MediaTrack* tr = GetLastTouchedTrack())
			{
				SetTrackHeight(tr, SetToBounds(height + GetTrackHeight(tr, NULL), GetTcpTrackMinHeight(), GetCurrentTcpMaxHeight()));
				ScrollToTrackIfNotInArrange(tr);
			}
		}
	}
}

void SetVerticalZoomSelectedEnvelope(COMMAND_T* ct)
{
	if (TrackEnvelope* env = GetSelectedEnvelope(NULL))
	{
		BR_Envelope brEnv(env);
		void(*ScrollTo)(TrackEnvelope*) = ScrollToTrackEnvelopeIfNotInArrange;
		if (brEnv.IsInLane())
		{
			if (brEnv.IsTakeEnvelope())
			{
				SetTrackHeight(brEnv.GetParent(), ((int)ct->user == 0) ? 0 : ((int)ct->user == 1) ? GetTcpTrackMinHeight() : GetCurrentTcpMaxHeight());
				ScrollTo = ScrollToTrackIfNotInArrange;
			}
			else
			{
				brEnv.SetLaneHeight(((int)ct->user == 0) ? 0 : ((int)ct->user == 1) ? GetTcpEnvMinHeight() : GetCurrentTcpMaxHeight());
				brEnv.Commit();
			}
		}
		else
		{
			int height, trackGapTop, trackGapBottom;
			int overlapMinHeight = *(int*)GetConfigVar("env_ol_minh");
			int mul = CountVisibleTrackEnvelopesInTrackLane(brEnv.GetParent());
			GetTrackHeight(brEnv.GetParent(), NULL, &trackGapTop, &trackGapBottom);
			if ((int)ct->user == 2 )
			{
				if (EnvelopesExtendedZoom)
				{
					if (overlapMinHeight >= 0)
					{
						if (GetCurrentTcpMaxHeight() > overlapMinHeight)
						{
							height = GetCurrentTcpMaxHeight()  * mul + trackGapTop + trackGapBottom;
						}
						else
						{
							height = GetCurrentTcpMaxHeight();
							ScrollTo = ScrollToTrackIfNotInArrange;
						}
					}
					else
					{
						if (mul == 1)
						{
							height = GetCurrentTcpMaxHeight();
							ScrollTo = ScrollToTrackIfNotInArrange;
						}
						else
						{
							height = GetCurrentTcpMaxHeight()  * mul + trackGapTop + trackGapBottom;
						}
					}
				}
				else
				{
					height = GetCurrentTcpMaxHeight();
					ScrollTo = ScrollToTrackIfNotInArrange;
				}
			}
			else
			{
				height = 0;
				ScrollTo = ScrollToTrackIfNotInArrange;
			}
			SetTrackHeight(brEnv.GetParent(), height);
		}
		ScrollTo(env);
	}
}

void ToggleEnableEnvelopesExtendedZoom(COMMAND_T*)
{
	EnvelopesExtendedZoom = !EnvelopesExtendedZoom;
	char str[32];
	sprintf(str, "%d", EnvelopesExtendedZoom);
	WritePrivateProfileString(SWS_INI, "WOLExtZoomEnvInTrLane", str, get_ini_file());
}

int IsEnvelopesExtendedZoomEnabled(COMMAND_T*)
{
	return EnvelopesExtendedZoom;
}

void ToggleEnableEnvelopeOverlap(COMMAND_T*)
{
	SetConfig("env_ol_minh", -((*(int*)GetConfigVar("env_ol_minh")) + 1));
	TrackList_AdjustWindows(false);
	UpdateTimeline();
}

int IsEnvelopeOverlapEnabled(COMMAND_T*)
{
	return (*(int*)GetConfigVar("env_ol_minh") >= 0);
}

void ForceEnvelopeOverlap(COMMAND_T* ct)
{
	if ((int)ct->user == 0)
	{
		if (TrackEnvelope* env = GetSelectedTrackEnvelope(NULL))
		{
			SavedEnvelopeOverlapSettings = *(int*)GetConfigVar("env_ol_minh");
			bool lane; EnvVis(env, &lane);
			if ((CountVisibleTrackEnvelopesInTrackLane(GetEnvParent(env)) > 1) && !lane)
			{
				int envHeight = GetTrackEnvHeight(env, NULL, false);
				SetConfig("env_ol_minh", envHeight + 1);
				TrackList_AdjustWindows(false);
				UpdateTimeline();
				RefreshToolbar(SWSGetCommandID(ToggleEnableEnvelopeOverlap));
			}
		}
	}
	else
	{
		SetConfig("env_ol_minh", SavedEnvelopeOverlapSettings);
		TrackList_AdjustWindows(false);
		UpdateTimeline();
		RefreshToolbar(SWSGetCommandID(ToggleEnableEnvelopeOverlap));
	}
}

void ZoomSelectedEnvelopeTimeSelection(COMMAND_T* ct)
{
	double d1, d2;
	GetSet_LoopTimeRange(false, false, &d1, &d2, false);
	TrackEnvelope* env = GetSelectedEnvelope(NULL);
	if (env && (d1 != d2))
	{
		Main_OnCommand(40031, 0);
		if ((int)ct->user == 1)
			Main_OnCommand(SWSGetCommandID(SetVerticalZoomSelectedEnvelope, 2), 0);
	}
}

void SetVerticalZoomCenter(COMMAND_T* ct)
{
	SetConfig("vzoommode", (int)ct->user);
}

void SetHorizontalZoomCenter(COMMAND_T* ct)
{
	SetConfig("zoommode", (int)ct->user);
}

void wol_ZoomInit()
{
	EnvelopesExtendedZoom = GetPrivateProfileInt(SWS_INI, "WOLExtZoomEnvInTrLane", 0, get_ini_file()) ? true : false;
	SavedEnvelopeOverlapSettings = *(int*)GetConfigVar("env_ol_minh");
}