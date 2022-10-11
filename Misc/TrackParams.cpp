/******************************************************************************
/ TrackParams.cpp
/
/ Copyright (c) 2010 Tim Payne (SWS)
/
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

//JFB most of actions in there do not set undo points: normal?

#include "stdafx.h"

#include "TrackParams.h"
#include "TrackSel.h"

#include <WDL/localize/localize.h>

void EnableMPSend(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			GetSetMediaTrackInfo(tr, "B_MAINSEND", &g_bTrue);
	}
}
void DisableMPSend(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			GetSetMediaTrackInfo(tr, "B_MAINSEND", &g_bFalse);
	}
}

void TogMPSend(COMMAND_T* = NULL)
{
	bool bMPSend;
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			bMPSend = !*(bool*)GetSetMediaTrackInfo(tr, "B_MAINSEND", NULL);
			GetSetMediaTrackInfo(tr, "B_MAINSEND", &bMPSend);
		}
	}
}

// ct->user: 0 == mono, 1 == stereo
void SetMasterMono(COMMAND_T* ct)
{
	bool bMono = GetMasterMuteSoloFlags() & 4 ? true : false;
	if ((bMono && ct->user) || (!bMono && !ct->user))
		Main_OnCommand(40917, 0);
}

void SetAllMasterOutputMutes(COMMAND_T* ct)
{
	MediaTrack* tr = CSurf_TrackFromID(0, false);
	int i = 0;
	bool bMute = ct->user ? true : false;
	while (GetSetTrackSendInfo(tr, 1, i, "B_MUTE", NULL))
	{
		GetSetTrackSendInfo(tr, 1, i++, "B_MUTE", &bMute);
	}
	if (i)
		Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG, 0);
}

void NudgeMasterOutputVol(COMMAND_T* ct)
{
	MediaTrack* tr = CSurf_TrackFromID(0, false);
	int iOutput = abs((int)ct->user) >> 8;
	double *pVol = (double*)GetSetTrackSendInfo(tr, 1, iOutput, "D_VOL", NULL);
	if (pVol)
	{
		double dVol = DB2VAL(VAL2DB(*pVol) + (abs((int)ct->user) & 0xFF) * (ct->user >= 0 ? 1 : -1));
		GetSetTrackSendInfo(tr, 1, iOutput, "D_VOL", &dVol);
		TrackList_AdjustWindows(false);
		Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG, 0);
	}
}

void SetMasterOutputVol(COMMAND_T* ct)
{
	MediaTrack* tr = CSurf_TrackFromID(0, false);
	int iOutput = abs((int)ct->user) >> 8;
	if (GetSetTrackSendInfo(tr, 1, iOutput, "D_VOL", NULL))
	{
		double dVol = DB2VAL((double)((abs((int)ct->user) & 0xFF) * (ct->user >= 0 ? 1 : -1)));
		GetSetTrackSendInfo(tr, 1, iOutput, "D_VOL", &dVol);
		TrackList_AdjustWindows(false);
		Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG, 0);
	}
}

int IsMasterOutputMuted(COMMAND_T* ct)
{
	void* muted = GetSetTrackSendInfo(CSurf_TrackFromID(0,false), 1, (int)ct->user, "B_MUTE", NULL);
	if (muted) return *(bool*)muted;
	return false;
}

void MasterOutputMute(COMMAND_T* ct, int mode) // mode 0==toggle, 1==set mute, 2==set unmute
{
	MediaTrack* tr = CSurf_TrackFromID(0, false);
	bool* pMute;
	if ((pMute = (bool*)GetSetTrackSendInfo(tr, 1, (int)ct->user, "B_MUTE", NULL)))
	{
		bool bMute;
		if (mode == 0)
			bMute = *pMute ? false : true;
		else
			bMute = mode == 1 ? true : false;
		GetSetTrackSendInfo(tr, 1, (int)ct->user, "B_MUTE", &bMute);
		Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG, 0);
	}
}

void TogMasterOutputMute(COMMAND_T* ct)		{ MasterOutputMute(ct, 0); }
void SetMasterOutputMute(COMMAND_T* ct)		{ MasterOutputMute(ct, 1); }
void UnSetMasterOutputMute(COMMAND_T* ct)	{ MasterOutputMute(ct, 2); }

void MuteRecvs(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			for (int j = 0; GetSetTrackSendInfo(tr, -1, j, "P_SRCTRACK", NULL); j++)
				GetSetTrackSendInfo(tr, -1, j, "B_MUTE", &g_bTrue);
	}
}

void UnmuteRecvs(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			for (int j = 0; GetSetTrackSendInfo(tr, -1, j, "P_SRCTRACK", NULL); j++)
				GetSetTrackSendInfo(tr, -1, j, "B_MUTE", &g_bFalse);
	}
}

void TogMuteRecvs(COMMAND_T* = NULL)
{
	bool bMute;
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			for (int j = 0; GetSetTrackSendInfo(tr, -1, j, "P_SRCTRACK", NULL); j++)
			{
				bMute = !*(bool*)GetSetTrackSendInfo(tr, -1, j, "B_MUTE", NULL);
				GetSetTrackSendInfo(tr, -1, j, "B_MUTE", &bMute);
			}
	}
}

void MuteSends(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			for (int j = 0; GetSetTrackSendInfo(tr, 0, j, "P_DESTTRACK", NULL); j++)
				GetSetTrackSendInfo(tr, 0, j, "B_MUTE", &g_bTrue);
	}
}

void UnmuteSends(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			for (int j = 0; GetSetTrackSendInfo(tr, 0, j, "P_DESTTRACK", NULL); j++)
				GetSetTrackSendInfo(tr, 0, j, "B_MUTE", &g_bFalse);
	}
}

void BypassFX(COMMAND_T* = NULL)
{
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			GetSetMediaTrackInfo(tr, "I_FXEN", &g_i0);
	}
}

void UnbypassFX(COMMAND_T* = NULL)
{
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			GetSetMediaTrackInfo(tr, "I_FXEN", &g_i1);
	}
}

static int g_iMasterFXEn = 1;
void SaveMasterFXEn(COMMAND_T*)  { g_iMasterFXEn = *(int*)GetSetMediaTrackInfo(CSurf_TrackFromID(0, false), "I_FXEN", NULL); }
void RestMasterFXEn(COMMAND_T*)  { GetSetMediaTrackInfo(CSurf_TrackFromID(0, false), "I_FXEN", &g_iMasterFXEn); }
void EnableMasterFX(COMMAND_T*)  { GetSetMediaTrackInfo(CSurf_TrackFromID(0, false), "I_FXEN", &g_i1); }
void DisableMasterFX(COMMAND_T*) { GetSetMediaTrackInfo(CSurf_TrackFromID(0, false), "I_FXEN", &g_i0); }

void MinimizeTracks(COMMAND_T* = NULL)
{
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			GetSetMediaTrackInfo(tr, "I_HEIGHTOVERRIDE", &g_i1);
	}
	TrackList_AdjustWindows(false);
	UpdateTimeline();
}

int IsMinimizeTracks(COMMAND_T* ct)
{
	int mins=0, seltrs=0;
	for (int i=0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			seltrs++;
			if (*(int*)GetSetMediaTrackInfo(tr, "I_HEIGHTOVERRIDE", NULL)==1)
				mins++;
		}
	}
	return seltrs && seltrs==mins;
}

void RecSrcOut(COMMAND_T* = NULL)
{
	// Set the rec source to output
	// Set to mono or stereo based on first item
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		int iMode = 1; // 5 = mono, 1 = stereo
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			if (GetTrackNumMediaItems(tr))
			{
				MediaItem* mi = GetTrackMediaItem(tr, 0);
				if (GetMediaItemNumTakes(mi))
				{
					MediaItem_Take* mit = GetMediaItemTake(mi, 0);
					PCM_source* p = (PCM_source*)GetSetMediaItemTakeInfo(mit, "P_SOURCE", NULL);
					if (p && p->GetNumChannels() == 1)
						iMode = 5;

				}
			}
			GetSetMediaTrackInfo(tr, "I_RECMODE", &iMode);
		}
	}
}

void SetMonMedia(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			GetSetMediaTrackInfo(tr, "I_RECMONITEMS", &g_i1);
	}
}

void UnsetMonMedia(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			GetSetMediaTrackInfo(tr, "I_RECMONITEMS", &g_i0);
	}
}

void InsertTrkAbove(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			InsertTrackAtIndex(i-1, true);
			ClearSelected();
			tr = CSurf_TrackFromID(i, false);
			TrackList_AdjustWindows(false);
			GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i1);
			UpdateTimeline();
			Undo_OnStateChangeEx(__LOCALIZE("Insert track above selected track","sws_undo"), UNDO_STATE_ALL, -1);
			return;
		}
	}
}

void CreateTrack1(COMMAND_T* = NULL)
{
	ClearSelected();
	InsertTrackAtIndex(0, true);
	TrackList_AdjustWindows(false);
	GetSetMediaTrackInfo(CSurf_TrackFromID(1, false), "I_SELECTED", &g_i1);
	SetLTT();
}

void DelTracksChild(COMMAND_T* = NULL)
{
	int iRet = MessageBox(g_hwndParent, __LOCALIZE("Delete track(s) children too?","sws_mbox"), __LOCALIZE("Delete track(s)","sws_mbox"), MB_YESNOCANCEL);
	if (iRet == IDCANCEL)
		return;
	if (iRet == IDYES)
		SelChildren();
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			DeleteTrack(tr);
			i--;
		}
	}
	Undo_OnStateChangeEx(__LOCALIZE("Delete track(s)","sws_undo"), UNDO_STATE_TRACKCFG, -1);
}

void NameTrackLikeItem(COMMAND_T* ct)
{
	WDL_TypedBuf<MediaTrack*> tracks;
	SWS_GetSelectedTracks(&tracks);
	bool bUndo = false;
	for (int i = 0; i < tracks.GetSize(); i++)
	{
		for (int j = 0; j < GetTrackNumMediaItems(tracks.Get()[i]); j++)
		{
			MediaItem* mi = GetTrackMediaItem(tracks.Get()[i], j);
			if (*(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL) && GetMediaItemNumTakes(mi))
			{
				MediaItem_Take* take = GetMediaItemTake(mi, -1);
				if (take)
				{
					const char* pName = (const char*)GetSetMediaItemTakeInfo(take, "P_NAME", NULL);
					if (pName && strlen(pName))
					{
						char* pNameNoExt = new char[strlen(pName)+1];
						strcpy(pNameNoExt, pName);
						char* pDot = strrchr(pNameNoExt, '.');
						if (pDot && IsMediaExtension(pDot+1, false))
							*pDot = 0;
						GetSetMediaTrackInfo(tracks.Get()[i], "P_NAME", (void*)pNameNoExt);
						bUndo = true;
						delete [] pNameNoExt;
					}
					break;
				}
			}
		}
	}
	if (bUndo)
		Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG, -1);
}

void NameTrackLikeFirstItem(COMMAND_T* ct)
{
	// Get the first item's take
	MediaItem* item = GetSelectedMediaItem(NULL, 0);
	if (!item || !GetMediaItemNumTakes(item))
		return;
	MediaItem_Take* take = GetMediaItemTake(item, -1);
	if (!take)
		return;
	// Get the first item's name
	const char* pName = (const char*)GetSetMediaItemTakeInfo(take, "P_NAME", NULL);
	if (!pName || !strlen(pName))
		return;

	// Strip out extension
	char* pNameNoExt = new char[strlen(pName)+1];
	strcpy(pNameNoExt, pName);
	char* pDot = strrchr(pNameNoExt, '.');
	if (pDot && IsMediaExtension(pDot+1, false))
		*pDot = 0;

	// Set all sel tracks to that name
	WDL_TypedBuf<MediaTrack*> tracks;
	SWS_GetSelectedTracks(&tracks);
	for (int i = 0; i < tracks.GetSize(); i++)
		GetSetMediaTrackInfo(tracks.Get()[i], "P_NAME", (void*)pNameNoExt);
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG, -1);
	delete [] pNameNoExt;
}

void UpdateTrackSolo()
{
	static int cmdID = 0;
	if (!cmdID)
		cmdID = NamedCommandLookup("_SWS_SOLOTOGGLE");
	RefreshToolbar(cmdID);
}

void UpdateTrackArm()
{
	static int cmdID = 0;
	if (!cmdID)
		cmdID = NamedCommandLookup("_SWS_ARMTOGGLE");
	RefreshToolbar(cmdID);
}

void UpdateTrackMute()
{
	static int cmdID = 0;
	if (!cmdID)
		cmdID = NamedCommandLookup("_SWS_MUTETOGGLE");
	RefreshToolbar(cmdID);
}

int CheckTrackParam(COMMAND_T* ct)
{
	int iNumTracks = GetNumTracks();
	for (int i = 1; i <= iNumTracks; i++)
		if (GetMediaTrackInfo_Value(CSurf_TrackFromID(i, false), (const char*)ct->user) != 0.0)
			return true;
	return false;
}

void ToolbarToggle(COMMAND_T* ct, WDL_TypedBuf<TrackInfo>* pState)
{
	if (CheckTrackParam(ct))
	{
		pState->Resize(GetNumTracks(), false);
		for (int i = 1; i <= GetNumTracks(); i++)
		{
			MediaTrack* tr = CSurf_TrackFromID(i, false);
			pState->Get()[i-1].guid = *(GUID*)GetSetMediaTrackInfo(tr, "GUID", NULL);
			pState->Get()[i-1].param = GetMediaTrackInfo_Value(tr, (const char*)ct->user);
			SetMediaTrackInfo_Value(tr, (const char*)ct->user, 0.0);
		}
	}
	else
	{	// Restore state
		for (int i = 0; i < pState->GetSize(); i++)
		{
			MediaTrack* tr = GuidToTrack(&pState->Get()[i].guid);
			if (tr)
				SetMediaTrackInfo_Value(tr, (const char*)ct->user, pState->Get()[i].param);
		}
	}
	TrackList_AdjustWindows(false);
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG, -1);
}

void ArmToggle(COMMAND_T* ct)
{
	static WDL_TypedBuf<TrackInfo> armState;
	ToolbarToggle(ct, &armState);
}

void SoloToggle(COMMAND_T* ct)
{
	static WDL_TypedBuf<TrackInfo> soloState;
	ToolbarToggle(ct, &soloState);
}

void MuteToggle(COMMAND_T* ct)
{
	static WDL_TypedBuf<TrackInfo> muteState;
	ToolbarToggle(ct, &muteState);
}

void InputMatch(COMMAND_T* ct)
{
	int iInput = -2; // Would use -1, but that's for "Input: None"
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			if (iInput == -2)
				iInput = *(int*)GetSetMediaTrackInfo(tr, "I_RECINPUT", NULL);
			else
				GetSetMediaTrackInfo(tr, "I_RECINPUT", &iInput);
		}
	}
	TrackList_AdjustWindows(false);
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG, -1);
}

//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] =
{
	// Master/parent send
	{ { DEFACCEL, "SWS: Enable master/parent send on selected track(s)" },		"SWS_ENMPSEND",		EnableMPSend,		},
	{ { DEFACCEL, "SWS: Disable master/parent send on selected track(s)" },		"SWS_DISMPSEND",	DisableMPSend,		},
	{ { DEFACCEL, "SWS: Toggle master/parent send on selected track(s)" },		"SWS_TOGMPSEND",	TogMPSend,			},
	// Master/parent mono/stereo
	{ { DEFACCEL, "SWS: Set master mono" },										"SWS_MASTERMONO",	SetMasterMono, NULL, 0	},
	{ { DEFACCEL, "SWS: Set master stereo" },									"SWS_MASTERSTEREO",	SetMasterMono, NULL, 1	},
	// Master outputs (moved from Xen-land)
	{ { DEFACCEL, "SWS: Toggle master track output 1 mute" },					"XEN_TOG_MAS_SEND0MUTE",		TogMasterOutputMute,	NULL, 0, IsMasterOutputMuted },
	{ { DEFACCEL, "SWS: Toggle master track output 2 mute" },					"XEN_TOG_MAS_SEND1MUTE",		TogMasterOutputMute,	NULL, 1, IsMasterOutputMuted },
	{ { DEFACCEL, "SWS: Toggle master track output 3 mute" },					"XEN_TOG_MAS_SEND2MUTE",		TogMasterOutputMute,	NULL, 2, IsMasterOutputMuted },
	{ { DEFACCEL, "SWS: Toggle master track output 4 mute" },					"XEN_TOG_MAS_SEND3MUTE",		TogMasterOutputMute,	NULL, 3, IsMasterOutputMuted },
	{ { DEFACCEL, "SWS: Toggle master track output 5 mute" },					"XEN_TOG_MAS_SEND4MUTE",		TogMasterOutputMute,	NULL, 4, IsMasterOutputMuted },
	{ { DEFACCEL, "SWS: Toggle master track output 6 mute" },					"XEN_TOG_MAS_SEND5MUTE",		TogMasterOutputMute,	NULL, 5, IsMasterOutputMuted },
	{ { DEFACCEL, "SWS: Toggle master track output 7 mute" },					"XEN_TOG_MAS_SEND6MUTE",		TogMasterOutputMute,	NULL, 6, IsMasterOutputMuted },
	{ { DEFACCEL, "SWS: Toggle master track output 8 mute" },					"XEN_TOG_MAS_SEND7MUTE",		TogMasterOutputMute,	NULL, 7, IsMasterOutputMuted },
	{ { DEFACCEL, "SWS: Toggle master track output 9 mute" },					"XEN_TOG_MAS_SEND8MUTE",		TogMasterOutputMute,	NULL, 8, IsMasterOutputMuted },
	{ { DEFACCEL, "SWS: Toggle master track output 10 mute" },					"XEN_TOG_MAS_SEND9MUTE",		TogMasterOutputMute,	NULL, 9, IsMasterOutputMuted },
	{ { DEFACCEL, "SWS: Toggle master track output 11 mute" },					"XEN_TOG_MAS_SEND10MUTE",		TogMasterOutputMute,	NULL, 10, IsMasterOutputMuted },
	{ { DEFACCEL, "SWS: Toggle master track output 12 mute" },					"XEN_TOG_MAS_SEND11MUTE",		TogMasterOutputMute,	NULL, 11, IsMasterOutputMuted },
	{ { DEFACCEL, "SWS: Set master track output 1 muted" },						"XEN_SET_MAS_SEND0MUTE",		SetMasterOutputMute,	NULL, 0 },
	{ { DEFACCEL, "SWS: Set master track output 2 muted" },						"XEN_SET_MAS_SEND1MUTE",		SetMasterOutputMute,	NULL, 1 },
	{ { DEFACCEL, "SWS: Set master track output 3 muted" },						"XEN_SET_MAS_SEND2MUTE",		SetMasterOutputMute,	NULL, 2 },
	{ { DEFACCEL, "SWS: Set master track output 4 muted" },						"XEN_SET_MAS_SEND3MUTE",		SetMasterOutputMute,	NULL, 3 },
	{ { DEFACCEL, "SWS: Set master track output 5 muted" },						"XEN_SET_MAS_SEND4MUTE",		SetMasterOutputMute,	NULL, 4 },
	{ { DEFACCEL, "SWS: Set master track output 1 unmuted" },					"XEN_UNSET_MAS_SEND0MUTE",		UnSetMasterOutputMute,	NULL, 0 },
	{ { DEFACCEL, "SWS: Set master track output 2 unmuted" },					"XEN_UNSET_MAS_SEND1MUTE",		UnSetMasterOutputMute,	NULL, 1 },
	{ { DEFACCEL, "SWS: Set master track output 3 unmuted" },					"XEN_UNSET_MAS_SEND2MUTE",		UnSetMasterOutputMute,	NULL, 2 },
	{ { DEFACCEL, "SWS: Set master track output 4 unmuted" },					"XEN_UNSET_MAS_SEND3MUTE",		UnSetMasterOutputMute,	NULL, 3 },
	{ { DEFACCEL, "SWS: Set master track output 5 unmuted" },					"XEN_UNSET_MAS_SEND4MUTE",		UnSetMasterOutputMute,	NULL, 4 },
	{ { DEFACCEL, "SWS: Set all master track outputs unmuted" },				"XEN_SET_MAS_SENDALLUNMUTE",	SetAllMasterOutputMutes,NULL, 0 },
	{ { DEFACCEL, "SWS: Set all master track outputs muted" },					"XEN_SET_MAS_SENDALLMUTE",		SetAllMasterOutputMutes,NULL, 1 },
	// Master output nudge/set functions support more that just output 1, the channel is passed in left shifted by 8
	{ { DEFACCEL, "SWS: Nudge master output 1 volume +1db" },					"SWS_MAST_O1_1",				NudgeMasterOutputVol,	NULL, 1 },
	{ { DEFACCEL, "SWS: Nudge master output 1 volume -1db" },					"SWS_MAST_O1_-1",				NudgeMasterOutputVol,	NULL, -1 },
	{ { DEFACCEL, "SWS: Set master output 1 volume to 0db" },					"SWS_MAST_O1_0",				SetMasterOutputVol,		NULL, 0 },

	// Send/recvs
	{ { DEFACCEL, "SWS: Mute all receives for selected track(s)" },				"SWS_MUTERECVS",	MuteRecvs,			},
	{ { DEFACCEL, "SWS: Unmute all receives for selected track(s)" },			"SWS_UNMUTERECVS",	UnmuteRecvs,		},
	{ { DEFACCEL, "SWS: Toggle mute on receives for selected track(s)" },		"SWS_TOGMUTERECVS",	TogMuteRecvs,		},
	{ { DEFACCEL, "SWS: Mute all sends from selected track(s)" },				"SWS_MUTESENDS",	MuteSends,			},
	{ { DEFACCEL, "SWS: Unmute all sends from selected track(s)" },				"SWS_UNMUTESENDS",	UnmuteSends,		},

	// FX bypass
	{ { DEFACCEL, "SWS: Bypass FX on selected track(s)" },						"SWS_BYPASSFX",		BypassFX,			},
	{ { DEFACCEL, "SWS: Unbypass FX on selected track(s)" },					"SWS_UNBYPASSFX",	UnbypassFX,			},
	{ { DEFACCEL, "SWS: Save master FX enabled state" },						"SWS_SAVEMSTFXEN",	SaveMasterFXEn,		},
	{ { DEFACCEL, "SWS: Restore master FX enabled state" },						"SWS_RESTMSTFXEN",	RestMasterFXEn,		},
	{ { DEFACCEL, "SWS: Enable master FX" },									"SWS_ENMASTERFX",	EnableMasterFX,		},
	{ { DEFACCEL, "SWS: Disable master FX" },									"SWS_DISMASTERFX",	DisableMasterFX,	},

	// Size
	{ { DEFACCEL, "SWS: Minimize selected track(s)" },							"SWS_MINTRACKS",	MinimizeTracks,		NULL, 0, IsMinimizeTracks},

	// Rec options
	{ { DEFACCEL, "SWS: Set selected track(s) record output mode based on items" },		"SWS_SETRECSRCOUT",		RecSrcOut,		},
	{ { DEFACCEL, "SWS: Set selected track(s) monitor track media while recording" },	"SWS_SETMONMEDIA",		SetMonMedia,	},
	{ { DEFACCEL, "SWS: Unset selected track(s) monitor track media while recording" },	"SWS_UNSETMONMEDIA",	UnsetMonMedia,	},
	{ { DEFACCEL, "SWS: Toolbar arm toggle" },									"SWS_ARMTOGGLE",  ArmToggle, NULL, (INT_PTR)"I_RECARM", CheckTrackParam, },

	// Add/remove tracks
	{ { DEFACCEL, "SWS: Insert track above selected tracks" },					"SWS_INSRTTRKABOVE",InsertTrkAbove,		},
	{ { DEFACCEL, "SWS: Create and select first track" },						"SWS_CREATETRK1",	CreateTrack1,		},
	{ { DEFACCEL, "SWS: Delete track(s) with children (prompt)" },				"SWS_DELTRACKCHLD",	DelTracksChild,		},

	// Name
	{ { DEFACCEL, "SWS: Set track name from first selected item on track" },    "SWS_NAMETKLIKEITEM",  NameTrackLikeItem,	},
	{ { DEFACCEL, "SWS: Set track name from first selected item in project" },	"SWS_NAMETKLIKEFIRST", NameTrackLikeFirstItem,	},

	// Solo
	{ { DEFACCEL, "SWS: Toolbar solo toggle" },									"SWS_SOLOTOGGLE", SoloToggle, NULL, (INT_PTR)"I_SOLO", CheckTrackParam, },

	// Inputs
	{ { DEFACCEL, "SWS: Set all selected tracks inputs to match first selected track" },	"SWS_INPUTMATCH", InputMatch, },

	// Mute
	{ { DEFACCEL, "SWS: Toolbar mute toggle" },									"SWS_MUTETOGGLE", MuteToggle, NULL, (INT_PTR)"B_MUTE", CheckTrackParam, },

	{ {}, LAST_COMMAND, }, // Denote end of table
};
//!WANT_LOCALIZE_1ST_STRING_END

int TrackParamsInit()
{
	SWSRegisterCommands(g_commandTable);
	return 1;
}
