/******************************************************************************
/ ProjPrefs.cpp
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

#include "stdafx.h"
#include "ProjPrefs.h"

static int g_prevautoxfade = 0;

#define SAVED_DEF_FADE_LEN_KEY "deffadelen"
static double g_dDefFadeLen = 0.01;

void SaveXFade(COMMAND_T* = NULL)
{
	if (const ConfigVar<int> autoxfade = "autoxfade")
		g_prevautoxfade = *autoxfade;
}

void RestoreXFade(COMMAND_T* = NULL)
{
	const ConfigVar<int> autoxfade("autoxfade");
	if (autoxfade && g_prevautoxfade != *autoxfade)
		Main_OnCommand(40041, 0);
}

void XFadeOn(COMMAND_T* = NULL)      { if (!GetToggleCommandState(40041)) Main_OnCommand(40041, 0); }
void XFadeOff(COMMAND_T* = NULL)     { if (GetToggleCommandState(40041)) Main_OnCommand(40041, 0); }
void MEPWIXOn(COMMAND_T* = NULL)     { if (!GetToggleCommandState(40070)) Main_OnCommand(40070, 0); }
void MEPWIXOff(COMMAND_T* = NULL)    { if (GetToggleCommandState(40070)) Main_OnCommand(40070, 0); }

void TogOnRecStopMoveCursor(COMMAND_T*) { if(ConfigVar<int> cv = "itemclickmovecurs") *cv ^= 16; }
int IsOnRecStopMoveCursor(COMMAND_T*)   { return ConfigVar<int>("itemclickmovecurs").value_or(0) & 16; }

void TogSeekMode(COMMAND_T* ct) { if(ConfigVar<int> cv = "seekmodes") *cv ^= ct->user; }
int IsSeekMode(COMMAND_T* ct)   { return ConfigVar<int>("seekmodes").value_or(0) & ct->user; }

void TogAutoAddEnvs(COMMAND_T*) { if (ConfigVar<int> cv = "env_autoadd") *cv ^= 1; }
int IsAutoAddEnvs(COMMAND_T*)   { return ConfigVar<int>("env_autoadd").value_or(0) & 1; }

void TogGridOverUnder(COMMAND_T*) { if (ConfigVar<int> cv = "gridinbg") { *cv = *cv == 2 ? 0 : 2; UpdateArrange(); } }
int IsGridOver(COMMAND_T*)        { return !ConfigVar<int>("gridinbg").value_or(1); }

void TogSelGroupMode(COMMAND_T*) { if (ConfigVar<int> cv = "projgroupsel") *cv = !*cv; }
int IsSelGroupMode(COMMAND_T*)   { return ConfigVar<int>("projgroupsel").value_or(0); }

void SwitchGridSpacing(COMMAND_T*)
{
	// TODO
	/*
	const double dSpacings[] = { 0.0208331, };
	double div = *ConfigVar<double>("projgriddiv");
	char debugStr[64];
	sprintf(debugStr, "Div is %.14f\n", div);
	OutputDebugString(debugStr);
	 */
}

void RecToggle(COMMAND_T* = NULL)
{
	// 1016 == stop
	// 1013 == record
	if (!(GetPlayState() & 4))
		Main_OnCommand(1013, 0);
	else
		Main_OnCommand(1016, 0);
}

static bool g_bRepeat = false;
void SaveRepeat(COMMAND_T* = NULL)	{ g_bRepeat = GetSetRepeat(-1) ? true : false; }
void RestRepeat(COMMAND_T* = NULL)	{ GetSetRepeat(g_bRepeat); }
void SetRepeat(COMMAND_T* = NULL)	{ GetSetRepeat(1); }
void UnsetRepeat(COMMAND_T* = NULL)	{ GetSetRepeat(0); }

void ShowDock(COMMAND_T* = NULL)
{
	if (!GetToggleCommandState(40279))
		Main_OnCommand(40279, 0);
}

void HideDock(COMMAND_T* = NULL)
{
	if (GetToggleCommandState(40279))
		Main_OnCommand(40279, 0);
}

void ShowMaster(COMMAND_T* = NULL)
{
	if (!ConfigVar<int>("showmaintrack").value_or(1))
		Main_OnCommand(40075, 0);
}

void HideMaster(COMMAND_T* = NULL)
{
	if (ConfigVar<int>("showmaintrack").value_or(0))
		Main_OnCommand(40075, 0);
}

void TogDefFadeZero(COMMAND_T*)
{
	ConfigVar<double> pdDefFade("deffadelen");
	if (*pdDefFade != 0.0)
	{
		if (*pdDefFade != g_dDefFadeLen)
		{
			char str[320];
			sprintf(str, "%.8f", *pdDefFade);
			WritePrivateProfileString(SWS_INI, SAVED_DEF_FADE_LEN_KEY, str, get_ini_file());
			g_dDefFadeLen = *pdDefFade;
		}
		*pdDefFade = 0.0;
	}
	else
	{
		*pdDefFade = g_dDefFadeLen;
	}
}

int IsDefFadeOverriden(COMMAND_T*)
{
	double dDefFade = *ConfigVar<double>("deffadelen");
	return g_dDefFadeLen != 0.0 && dDefFade != g_dDefFadeLen;
}

void MetronomeOn(COMMAND_T*)
{
	if (!(*ConfigVar<int>("projmetroen") & 1))
		Main_OnCommand(40364, 0);
}

void MetronomeOff(COMMAND_T*)
{
	if (*ConfigVar<int>("projmetroen") & 1)
		Main_OnCommand(40364, 0);
}

//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "SWS: Save auto crossfade state" },								"SWS_SAVEXFD",			SaveXFade,			},
	{ { DEFACCEL, "SWS: Restore auto crossfade state" },							"SWS_RESTOREXFD",		RestoreXFade,		},
	{ { DEFACCEL, "SWS: Set auto crossfade on" },									"SWS_XFDON",			XFadeOn,			},
	{ { DEFACCEL, "SWS: Set auto crossfade off" },									"SWS_XFDOFF",			XFadeOff,			},
	{ { DEFACCEL, "SWS: Set move envelope points with items on" },					"SWS_MVPWIDON",			MEPWIXOn,			},
	{ { DEFACCEL, "SWS: Set move envelope points with items off" },					"SWS_MVPWIDOFF",		MEPWIXOff,			},
	{ { DEFACCEL, "SWS: Toggle move cursor to end of recorded media on stop" },		"SWS_TOGRECMOVECUR",	TogOnRecStopMoveCursor, NULL, 0, IsOnRecStopMoveCursor },
	{ { DEFACCEL, "SWS: Toggle seek playback on item move/size" },					"SWS_TOGSEEKMODE1",		TogSeekMode, NULL, 65536, IsSeekMode },
	{ { DEFACCEL, "SWS: Toggle seek playback on loop point change" },				"SWS_TOGSEEKMODE2",		TogSeekMode, NULL, 8, IsSeekMode },
	{ { DEFACCEL, "SWS: Toggle auto add envelopes when tweaking in write mode" },	"SWS_TOGAUTOADDENVS",	TogAutoAddEnvs, NULL, 0, IsAutoAddEnvs },
	{ { DEFACCEL, "SWS: Toggle grid lines over/under items" },						"SWS_TOGGRID_OU",		TogGridOverUnder, NULL, 0, IsGridOver },
	{ { DEFACCEL, "SWS: Toggle selecting one grouped item selects group" },			"SWS_TOGSELGROUP",      TogSelGroupMode, NULL, 0, IsSelGroupMode },
	//TODO
	//{ { DEFACCEL, "SWS: Switch grid spacing" },										"SWS_GRIDSPACING",		SwitchGridSpacing,	},
	{ { DEFACCEL, "SWS: Transport: Record/stop" },									"SWS_RECTOGGLE",		RecToggle,			},
	{ { DEFACCEL, "SWS: Save transport repeat state" },								"SWS_SAVEREPEAT",		SaveRepeat,			},
	{ { DEFACCEL, "SWS: Restore transport repeat state" },							"SWS_RESTREPEAT",		RestRepeat,			},
	{ { DEFACCEL, "SWS: Set transport repeat state" },								"SWS_SETREPEAT",		SetRepeat,			},
	{ { DEFACCEL, "SWS: Unset transport repeat state" },							"SWS_UNSETREPEAT",		UnsetRepeat,		},
	{ { DEFACCEL, "SWS: Show dockers" },											"SWS_SHOWDOCK",			ShowDock,			},
	{ { DEFACCEL, "SWS: Hide dockers" },											"SWS_HIDEDOCK",			HideDock,			},
	{ { DEFACCEL, "SWS: Show master track in track control panel" },				"SWS_SHOWMASTER",		ShowMaster,			},
	{ { DEFACCEL, "SWS: Hide master track in track control panel" },				"SWS_HIDEMASTER",		HideMaster,			},
	{ { DEFACCEL, "SWS: Toggle default fade time to zero" },						"SWS_TOGDEFFADEZERO",	TogDefFadeZero,		NULL, 0, IsDefFadeOverriden },
	{ { DEFACCEL, "SWS: Metronome enable" },										"SWS_METROON",			MetronomeOn,		},
	{ { DEFACCEL, "SWS: Metronome disable" },										"SWS_METROOFF",			MetronomeOff,		},
#ifdef ACTION_DEBUG
	{ { DEFACCEL, "SWS: Write SWS actions to sws_actions.csv" },					"SWS_ACTIONS",			ActionsList,		},
#endif
	{ {}, LAST_COMMAND, }, // Denote end of table
};
//!WANT_LOCALIZE_1ST_STRING_END

int ProjPrefsInit()
{
	SWSRegisterCommands(g_commandTable);

	g_prevautoxfade = *ConfigVar<int>("autoxfade");

	// Get saved default fade length
	char str[320], strDef[320];
	sprintf(strDef, "%.8f", *ConfigVar<double>("deffadelen"));
	GetPrivateProfileString(SWS_INI, SAVED_DEF_FADE_LEN_KEY, strDef, str, sizeof(str), get_ini_file());
	g_dDefFadeLen = atof(str);

	return 1;
}
