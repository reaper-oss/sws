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

static struct wol_Zoom_Ini {
	const char* Section = "Zoom";
	const char* EnvelopeExtendedZoom = "EnvelopeExtendedZoomInTrackLane";
	static const char* EnvelopeHeightSlots[8];
} wol_Zoom_Ini;
const char* wol_Zoom_Ini::EnvelopeHeightSlots[8] = {
	"EnvelopeHeightSlot1",
	"EnvelopeHeightSlot2",
	"EnvelopeHeightSlot3",
	"EnvelopeHeightSlot4",
	"EnvelopeHeightSlot5",
	"EnvelopeHeightSlot6",
	"EnvelopeHeightSlot7",
	"EnvelopeHeightSlot8",
};

//---------//

static bool g_EnvelopesExtendedZoom = false;
void ToggleEnableEnvelopesExtendedZoom(COMMAND_T*)
{
	g_EnvelopesExtendedZoom = !g_EnvelopesExtendedZoom;
	SaveIniSettings(wol_Zoom_Ini.Section, wol_Zoom_Ini.EnvelopeExtendedZoom, g_EnvelopesExtendedZoom);
}

int IsEnvelopesExtendedZoomEnabled(COMMAND_T*)
{
	return g_EnvelopesExtendedZoom;
}

//---------//

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

//---------//

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

//---------//

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

//---------//

static int g_SavedEnvelopeOverlapSettings = 0;
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

//---------//

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

//---------//

void SetVerticalZoomCenter(COMMAND_T* ct)
{
	SetConfig("vzoommode", (int)ct->user);
	SaveIniSettings("reaper", "vzoommode", (int)ct->user, get_ini_file());
}

void SetHorizontalZoomCenter(COMMAND_T* ct)
{
	SetConfig("zoommode", (int)ct->user);
	SaveIniSettings("reaper", "zoommode", (int)ct->user, get_ini_file());
}

//---------//

static int EnvH[8];
void SaveRestoreSelectedEnvelopeHeightSlot(COMMAND_T* ct)
{
	if (TrackEnvelope* env = GetSelectedEnvelope(NULL))
	{
		if ((int)ct->user < 8)
		{
			int s = (int)ct->user;
			BR_Envelope brEnv(env);
			if (brEnv.IsInLane())
				//if (brEnv.IsTakeEnvelope())
				//	EnvH[s] = GetTrackHeight(brEnv.GetParent(), 0);
				//else
					EnvH[s] = brEnv.LaneHeight();
			else
			{
				int trackGapTop, trackGapBottom, mul, laneCount, envCount;
				EnvH[s] = GetTrackHeight(brEnv.GetParent(), NULL, &trackGapTop, &trackGapBottom);
				EnvH[s] -= (trackGapTop + trackGapBottom);
				GetEnvelopeOverlapState(env, &laneCount, &envCount);
				mul = g_EnvelopesExtendedZoom ? laneCount : 1;
				EnvH[s] /= mul;
			}
			SaveIniSettings(wol_Zoom_Ini.Section, wol_Zoom_Ini.EnvelopeHeightSlots[s], EnvH[s]);
		}
		else if (((int)ct->user - 8) < 8)
		{
			int s = (int)ct->user - 8;
			if (EnvH[s] == 0)
				return;
			BR_Envelope brEnv(env);
			void(*ScrollTo)(TrackEnvelope*) = ScrollToTrackEnvelopeIfNotInArrange;
			if (brEnv.IsInLane())
			{
				//if (brEnv.IsTakeEnvelope())
				//{
				//	SetTrackHeight(brEnv.GetParent(), EnvH[s]);
				//	ScrollTo = ScrollToTrackIfNotInArrange;
				//}
				//else
				//{
					brEnv.SetLaneHeight(EnvH[s]);
					brEnv.Commit();
				//}
			}
			else
			{
				int trackGapTop, trackGapBottom, mul, laneCount, envCount;
				GetTrackHeight(brEnv.GetParent(), NULL, &trackGapTop, &trackGapBottom);
				GetEnvelopeOverlapState(env, &laneCount, &envCount);
				mul = g_EnvelopesExtendedZoom ? laneCount : 1;
				if (mul == 1)
					ScrollTo = ScrollToTrackIfNotInArrange;

				SetTrackHeight(brEnv.GetParent(), SetToBounds(EnvH[s] * mul, GetTcpTrackMinHeight(), (g_EnvelopesExtendedZoom ? GetCurrentTcpMaxHeight() * envCount + trackGapTop + trackGapBottom : GetCurrentTcpMaxHeight())));
			}
			ScrollTo(env);
		}
	}
}

//---------//

static const int envListVer = 1;
static vector<EnvelopeHeight> envList;
void EnvelopeHeightList(COMMAND_T* ct)
{
	if (TrackEnvelope* env = GetSelectedEnvelope(NULL))
	{
		if ((int)ct->user == 0)
		{
			EnvelopeHeight tmp;
			tmp.env = env;
			BR_Envelope brEnv(env);
			if (brEnv.IsInLane())
			{
				//if (brEnv.IsTakeEnvelope())
				//	tmp.h = GetTrackHeight(brEnv.GetParent(), 0);
				//else
				tmp.h = brEnv.LaneHeight();
			}
			else
			{
				int trackGapTop, trackGapBottom, mul, laneCount, envCount;
				tmp.h = GetTrackHeight(brEnv.GetParent(), NULL, &trackGapTop, &trackGapBottom);
				tmp.h -= (trackGapTop + trackGapBottom);
				GetEnvelopeOverlapState(env, &laneCount, &envCount);
				mul = g_EnvelopesExtendedZoom ? laneCount : 1;
				tmp.h /= mul;
			}
			bool added = false;
			for (size_t i = 0; i < envList.size(); ++i)
			{
				if (env == envList[i].env)
				{
					envList[i].h = tmp.h;
					added = true;
					break;
				}
			}
			if (!added)
				envList.push_back(tmp);
		}
		else if ((int)ct->user == 1)
		{
			for (size_t i = 0; i < envList.size(); ++i)
			{
				if (env == envList[i].env)
				{
					BR_Envelope brEnv(env);
					void(*ScrollTo)(TrackEnvelope*) = ScrollToTrackEnvelopeIfNotInArrange;
					if (brEnv.IsInLane())
					{
						//if (brEnv.IsTakeEnvelope())
						//{
						//	SetTrackHeight(brEnv.GetParent(), envlist[i].h);
						//	ScrollTo = ScrollToTrackIfNotInArrange;
						//}
						//else
						//{
							brEnv.SetLaneHeight(envList[i].h);
							brEnv.Commit();
						//}
					}
					else
					{
						int trackGapTop, trackGapBottom, mul, laneCount, envCount;
						GetTrackHeight(brEnv.GetParent(), NULL, &trackGapTop, &trackGapBottom);
						GetEnvelopeOverlapState(env, &laneCount, &envCount);
						mul = g_EnvelopesExtendedZoom ? laneCount : 1;
						if (mul == 1)
							ScrollTo = ScrollToTrackIfNotInArrange;

						SetTrackHeight(brEnv.GetParent(), SetToBounds(envList[i].h * mul, GetTcpTrackMinHeight(), (g_EnvelopesExtendedZoom ? GetCurrentTcpMaxHeight() * envCount + trackGapTop + trackGapBottom : GetCurrentTcpMaxHeight())));
					}
					ScrollTo(env);
					break;
				}
			}
		}
		else if ((int)ct->user == 2)
		{
			for (vector<EnvelopeHeight>::iterator it = envList.begin(); it != envList.end(); ++it)
			{
				if (env == it->env)
				{
					envList.erase(it);
					break;
				}
			}
		}
	}
}

static bool ProcessExtensionLine(const char *line, ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	LineParser lp(false);
	if (lp.parse(line) || lp.getnumtokens() < 1)
		return false;
	if (strcmp(lp.gettoken_str(0), "<SWSWOLENVHLIST") == 0)
	{
		char linebuf[4096];
		while (true)
		{
			if (!ctx->GetLine(linebuf, sizeof(linebuf)) && !lp.parse(linebuf))
			{
				if (lp.gettoken_str(0)[0] == '>')
					break;

				int loadedVer = 0;
				if (lp.getnumtokens() == 2 && !strcmp(lp.gettoken_str(0), "VER"))
				{
					loadedVer = lp.gettoken_int(1);
					continue;
				}

				GUID g;
				stringToGuid(lp.gettoken_str(0), &g);
				MediaTrack* tr = GuidToTrack(&g);
				if (tr)
				{
					EnvelopeHeight tmp;
					tmp.env = GetTrackEnvelope(tr, lp.gettoken_int(1));
					tmp.h = lp.gettoken_int(2);
					envList.push_back(tmp);
				}
			}
			else
				break;
		}
		return true;
	}
	return false;
}

static void SaveExtensionConfig(ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	if (envList.size())
	{
		ctx->AddLine("<SWSWOLENVHLIST");
		ctx->AddLine("VER %d", envListVer);
		for (size_t i = 0; i < envList.size(); ++i)
		{
			BR_Envelope env(envList[i].env);
			if (MediaTrack* tr = env.GetParent())
			{
				int idx = -1;
				for (int j = 0; j < CountTrackEnvelopes(tr); ++j)
				{
					if (env.GetPointer() == GetTrackEnvelope(tr, j))
					{
						idx = j;
						break;
					}
				}
				const GUID* g = TrackToGuid(tr);
				if (g && idx != -1)
				{
					char guid[128] = "";
					guidToString(g, guid);
					ctx->AddLine("\"%s\" %d %d", guid, idx, envList[i].h);
				}
			}
		}
		ctx->AddLine(">");
	}
}

static void BeginLoadProjectState(bool isUndo, struct project_config_extension_t *reg)
{
	envList.clear();
}

static project_config_extension_t g_projectconfig = { ProcessExtensionLine, SaveExtensionConfig, BeginLoadProjectState, NULL };

//---------//

bool wol_ZoomInit()
{
	if (GetIniSettings(SWS_INI, "WOLExtZoomEnvInTrLane", 2, get_ini_file()) == 2)
		DeleteIniKey(SWS_INI, "WOLExtZoomEnvInTrLane", get_ini_file());
	
	g_EnvelopesExtendedZoom = GetIniSettings(wol_Zoom_Ini.Section, wol_Zoom_Ini.EnvelopeExtendedZoom, false);
	g_SavedEnvelopeOverlapSettings = *(int*)GetConfigVar("env_ol_minh");

	for (int i = 0; i < 8; ++i)
		EnvH[i] = GetIniSettings(wol_Zoom_Ini.Section, wol_Zoom_Ini.EnvelopeHeightSlots[i], 0);

	if (!plugin_register("projectconfig", &g_projectconfig))
		return false;

	return true;
}