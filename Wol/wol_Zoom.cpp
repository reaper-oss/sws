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
#include "wol_Util.h"
#include "../Breeder/BR_Util.h"
#include "../Breeder/BR_EnvTools.h"
#include "../SnM/SnM_Util.h"



bool g_EnvelopesExtendedZoom = false;
int g_SavedEnvelopeOverlapSettings = 0;



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
				mul = g_EnvelopesExtendedZoom ? laneCount : 1;
				if (mul == 1)
					ScrollTo = ScrollToTrackIfNotInArrange;

				SetTrackHeight(brEnv.GetParent(), SetToBounds(height * mul + currentHeight, GetTcpTrackMinHeight(), (g_EnvelopesExtendedZoom ? GetCurrentTcpMaxHeight() * envCount + trackGapTop + trackGapBottom : GetCurrentTcpMaxHeight())));
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
				if (g_EnvelopesExtendedZoom)
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
	g_EnvelopesExtendedZoom = !g_EnvelopesExtendedZoom;
	char str[32];
	sprintf(str, "%d", g_EnvelopesExtendedZoom);
	WritePrivateProfileString(SWS_INI, "WOLExtZoomEnvInTrLane", str, get_ini_file());
}

int IsEnvelopesExtendedZoomEnabled(COMMAND_T*)
{
	return g_EnvelopesExtendedZoom;
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
			g_SavedEnvelopeOverlapSettings = *(int*)GetConfigVar("env_ol_minh");
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
		SetConfig("env_ol_minh", g_SavedEnvelopeOverlapSettings);
		TrackList_AdjustWindows(false);
		UpdateTimeline();
		RefreshToolbar(SWSGetCommandID(ToggleEnableEnvelopeOverlap));
	}
}

void ZoomSelectedEnvelopeTimeSelection(COMMAND_T* ct)
{
	double d1, d2;
	GetSet_LoopTimeRange(false, false, &d1, &d2, false);
	//TrackEnvelope* env = GetSelectedEnvelope(NULL);
	if (/*env &&*/ (d1 != d2))
	{
		Main_OnCommand(40031, 0);
		if ((int)ct->user == 1)
			Main_OnCommand(SWSGetCommandID(SetVerticalZoomSelectedEnvelope, 2), 0);
	}
}

void SetVerticalZoomCenter(COMMAND_T* ct)
{
	SetConfig("vzoommode", (int)ct->user);

	char tmp[256];
	_snprintfSafe(tmp, sizeof(tmp), "%d", (int)ct->user);
	WritePrivateProfileString("reaper", "vzoommode", tmp, get_ini_file());
}

void SetHorizontalZoomCenter(COMMAND_T* ct)
{
	SetConfig("zoommode", (int)ct->user);

	char tmp[256];
	_snprintfSafe(tmp, sizeof(tmp), "%d", (int)ct->user);
	WritePrivateProfileString("reaper", "zoommode", tmp, get_ini_file());
}

void wol_ZoomInit()
{
	g_EnvelopesExtendedZoom = GetPrivateProfileInt(SWS_INI, "WOLExtZoomEnvInTrLane", 0, get_ini_file()) ? true : false;
	g_SavedEnvelopeOverlapSettings = *(int*)GetConfigVar("env_ol_minh");
}