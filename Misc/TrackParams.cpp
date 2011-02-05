/******************************************************************************
/ TrackParams.cpp
/
/ Copyright (c) 2010 Tim Payne (SWS)
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
#include "TrackParams.h"
#include "TrackSel.h"

typedef struct TrackInfo
{
	GUID guid;
	int i;
} TrackInfo;

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
	//JFB no undo point?
}

void UnbypassFX(COMMAND_T* = NULL)
{
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			GetSetMediaTrackInfo(tr, "I_FXEN", &g_i1);
	}
	//JFB no undo point?
}

static int g_iMasterFXEn = 1;
void SaveMasterFXEn(COMMAND_T*)  { g_iMasterFXEn = *(int*)GetSetMediaTrackInfo(CSurf_TrackFromID(0, false), "I_FXEN", NULL); }
void RestMasterFXEn(COMMAND_T*)  { GetSetMediaTrackInfo(CSurf_TrackFromID(0, false), "I_FXEN", &g_iMasterFXEn); }
void EnableMasterFX(COMMAND_T*)  { GetSetMediaTrackInfo(CSurf_TrackFromID(0, false), "I_FXEN", &g_i1); }
void DisableMasterFX(COMMAND_T*) { GetSetMediaTrackInfo(CSurf_TrackFromID(0, false), "I_FXEN", &g_i0); }

void MinimizeTracks(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			GetSetMediaTrackInfo(tr, "I_HEIGHTOVERRIDE", &g_i1);
	}
	TrackList_AdjustWindows(false);
	UpdateTimeline();
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
			InsertTrackAtIndex(i-1, false);
			ClearSelected();
			tr = CSurf_TrackFromID(i, false);
			TrackList_AdjustWindows(false);
			GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i1);
			UpdateTimeline();
			Undo_OnStateChangeEx("Insert track above selected track", UNDO_STATE_ALL, -1);
			return;
		}
	}
}

void CreateTrack1(COMMAND_T* = NULL)
{
	ClearSelected();
	InsertTrackAtIndex(0, false);
	TrackList_AdjustWindows(false);
	GetSetMediaTrackInfo(CSurf_TrackFromID(1, false), "I_SELECTED", &g_i1);
	SetLTT();
}

void DelTracksChild(COMMAND_T* = NULL)
{
	int iRet = MessageBox(g_hwndParent, "Delete track(s) children too?", "Delete track(s)", MB_YESNOCANCEL);
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
	Undo_OnStateChangeEx("Delete track(s)", UNDO_STATE_TRACKCFG, -1);
}

void NameTrackLikeItem(COMMAND_T*)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
			{
				MediaItem* mi = GetTrackMediaItem(tr, j);
				if (*(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL) && GetMediaItemNumTakes(mi))
				{
					MediaItem_Take* mit = GetMediaItemTake(mi, -1);
					PCM_source* src = (PCM_source*)GetSetMediaItemTakeInfo(mit, "P_SOURCE", NULL);
					if (src && src->GetFileName())
					{
						const char* cFilename = src->GetFileName();
						const char* pC = strrchr(cFilename, PATH_SLASH_CHAR);
						if (pC)
							cFilename = pC + 1;
						GetSetMediaTrackInfo(tr, "P_NAME", (void*)cFilename);
					}
					break;
				}
			}
		}
	}

	// TODO undo?
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

bool CheckTrackParam(COMMAND_T* ct)
{
	int iNumTracks = GetNumTracks();
	for (int i = 1; i <= iNumTracks; i++)
		if (*(int*)GetSetMediaTrackInfo(CSurf_TrackFromID(i, false), (const char*)ct->user, NULL))
			return true;
	return false;
}

void ToolbarToggle(COMMAND_T* ct)
{
	static WDL_TypedBuf<TrackInfo> soloState;
	static WDL_TypedBuf<TrackInfo> armState;
	WDL_TypedBuf<TrackInfo>* pState;
	if (strcmp("I_SOLO", (const char*)ct->user) == 0)
		pState = &soloState;
	else
		pState = &armState;

	if (CheckTrackParam(ct))
	{
		pState->Resize(GetNumTracks(), false);
		for (int i = 1; i <= GetNumTracks(); i++)
		{
			MediaTrack* tr = CSurf_TrackFromID(i, false);
			pState->Get()[i-1].guid = *(GUID*)GetSetMediaTrackInfo(tr, "GUID", NULL);
			pState->Get()[i-1].i = *(int*)GetSetMediaTrackInfo(tr, (const char*)ct->user, NULL);
			GetSetMediaTrackInfo(tr, (const char*)ct->user, &g_i0);
		}
	}
	else
	{	// Restore state
		for (int i = 0; i < pState->GetSize(); i++)
		{
			MediaTrack* tr = GuidToTrack(&pState->Get()[i].guid);
			if (tr)
				GetSetMediaTrackInfo(tr, (const char*)ct->user, &pState->Get()[i].i);
		}
	}
	TrackList_AdjustWindows(false);
	Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct), UNDO_STATE_TRACKCFG, -1);
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

static COMMAND_T g_commandTable[] = 
{
	// Master/parent send
	{ { DEFACCEL, "SWS: Enable master/parent send on selected track(s)" },		"SWS_ENMPSEND",		EnableMPSend,		},
	{ { DEFACCEL, "SWS: Disable master/parent send on selected track(s)" },		"SWS_DISMPSEND",	DisableMPSend,		},
	{ { DEFACCEL, "SWS: Toggle master/parent send on selected track(s)" },		"SWS_TOGMPSEND",	TogMPSend,			},

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
	{ { DEFACCEL, "SWS: Minimize selected track(s)" },							"SWS_MINTRACKS",	MinimizeTracks,		},

	// Rec options
	{ { DEFACCEL, "SWS: Set selected track(s) record output mode based on items" },		"SWS_SETRECSRCOUT",		RecSrcOut,		},
	{ { DEFACCEL, "SWS: Set selected track(s) monitor track media while recording" },	"SWS_SETMONMEDIA",		SetMonMedia,	},
	{ { DEFACCEL, "SWS: Unset selected track(s) monitor track media while recording" },	"SWS_UNSETMONMEDIA",	UnsetMonMedia,	},
	{ { DEFACCEL, "SWS: Toolbar arm toggle" },									"SWS_ARMTOGGLE",  ToolbarToggle, NULL, (INT_PTR)"I_RECARM", CheckTrackParam, },

	// Add/remove tracks
	{ { DEFACCEL, "SWS: Insert track above selected tracks" },					"SWS_INSRTTRKABOVE",InsertTrkAbove,		},
	{ { DEFACCEL, "SWS: Create and select first track" },						"SWS_CREATETRK1",	CreateTrack1,		},
	{ { DEFACCEL, "SWS: Delete track(s) with children (prompt)" },				"SWS_DELTRACKCHLD",	DelTracksChild,		},

	// Name
	{ { DEFACCEL, "SWS: Name selected track(s) like first sel item" },			"SWS_NAMETKLIKEITEM", NameTrackLikeItem,	},

	// Solo
	{ { DEFACCEL, "SWS: Toolbar solo toggle" },									"SWS_SOLOTOGGLE", ToolbarToggle, NULL, (INT_PTR)"I_SOLO",   CheckTrackParam, },

	// Inputs
	{ { DEFACCEL, "SWS: Set all sel tracks inputs to match first sel track" },	"SWS_INPUTMATCH", InputMatch, },

	{ {}, LAST_COMMAND, }, // Denote end of table
};

int TrackParamsInit()
{
	SWSRegisterCommands(g_commandTable);
	return 1;
}