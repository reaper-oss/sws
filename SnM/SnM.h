/******************************************************************************
/ SnM.h
/
/ Copyright (c) 2009-2012 Jeffos
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

//#pragma once

#ifndef _SNM_H_
#define _SNM_H_


// see eof for includes!


// to disable/enable some features..
//#define _SNM_MISC
//#define _SNM_MARKER_REGION_NAME
#define _SNM_PRESETS

#ifdef _WIN32
#define SNM_FORMATED_INI_FILE		"%s\\S&M.ini"
#define SNM_OLD_FORMATED_INI_FILE	"%s\\Plugins\\S&M.ini"
#define SNM_ACTION_HELP_INI_FILE	"%s\\S&M_Action_help_en.ini"
#define SNM_CYCLACTION_INI_FILE		"%s\\S&M_Cyclactions.ini"
#define SNM_CYCLACTION_BAK_FILE		"%s\\S&M_Cyclactions.bak"
#define SNM_CYCLACTION_EXPORT_FILE	"%s\\S&M_Cyclactions_export.ini"
#define SNM_FONT_NAME				"MS Shell Dlg"
#define SNM_FONT_HEIGHT				14
#else
#define SNM_FORMATED_INI_FILE		"%s/S&M.ini"
#define SNM_OLD_FORMATED_INI_FILE	"%s/Plugins/S&M.ini"
#define SNM_ACTION_HELP_INI_FILE	"%s/S&M_Action_help_en.ini"
#define SNM_CYCLACTION_INI_FILE		"%s/S&M_Cyclactions.ini"
#define SNM_CYCLACTION_BAK_FILE		"%s/S&M_Cyclactions.bak"
#define SNM_CYCLACTION_EXPORT_FILE	"%s/S&M_Cyclactions_export.ini"
#define SNM_FONT_NAME				"Arial"
#define SNM_FONT_HEIGHT				12
#endif

#define SNM_INI_FILE_VERSION		5
#define SNM_LIVECFG_NB_CONFIGS		8
#define SNM_LIVECFG_NB_CONFIGS_STR	"8"
#define SNM_MAX_TRACK_GROUPS		32
#define SNM_MAX_TRACK_GROUPS_STR	"32"
#define SNM_MAX_CUE_BUSS_CONFS		8
#define SNM_MAX_CUE_BUSS_CONFS_STR	"8"
#define SNM_INI_EXT_LIST			"INI files (*.INI)\0*.INI\0All Files\0*.*\0"
#define SNM_SUB_EXT_LIST			"SubRip subtitle files (*.SRT)\0*.SRT\0"
#define SNM_TXT_EXT_LIST			"Text files (*.txt)\0*.txt\0All files (*.*)\0*.*\0"
#define SNM_MAX_SECTION_NAME_LEN	64
#define SNM_MAX_SECTION_ACTIONS		128
#define SNM_MAX_ACTION_CUSTID_LEN	128
#define SNM_MAX_ACTION_NAME_LEN		128
#define SNM_MAX_MARKER_NAME_LEN		64     // + regions
#define SNM_MAX_TRACK_NAME_LEN		128
#define SNM_MAX_HW_OUTS				8
//#define SNM_MAX_TAKES				1024
#define SNM_MAX_FX					128
#define SNM_MAX_PRESETS				0xFFFF
#define SNM_MAX_PRESET_NAME_LEN		128
#define SNM_MAX_FX_NAME_LEN			128
#define SNM_MAX_INI_SECTION			0xFFFF // definitive limit for WritePrivateProfileSection
#define SNM_MAX_DYNAMIC_ACTIONS		99     // if > 99 the display of action names should be updated
#define SNM_MAX_CYCLING_ACTIONS		8
#define SNM_MAX_CYCLING_SECTIONS	3
#define SNM_MAX_ENV_SUBCHUNK_NAME	16
#define MAX_CC_LANE_ID				133
#define MAX_CC_LANES_LEN			4096
#define SNM_3D_COLORS_DELTA			25
#define SNM_CSURF_RUN_TICK_MS		27.0   // 1 tick = 27ms or so (average I monitored)
#define SNM_DEF_TOOLBAR_RFRSH_FREQ	300    // default frequency in ms for the "auto-refresh toolbars" option 
#define SNM_SCHEDJOB_DEFAULT_DELAY	250
#define SNM_DEF_VWND_X_STEP			12
#define SNM_FUDGE_FACTOR			0.0000000001

// various bitmask flags
#define SNM_MARKER_MASK				1
#define SNM_REGION_MASK				2

// scheduled jobs ids
// note: [0..7] are reserved for Live Configs MIDI CC actions
#define SNM_SCHEDJOB_LIVECFG_TLCHANGE	8
#define SNM_SCHEDJOB_NOTEHLP_UPDATE		9
#define SNM_SCHEDJOB_SEL_PRJ			10
#define SNM_SCHEDJOB_TRIG_PRESET		11
#define SNM_SCHEDJOB_CYCLACTION			12
#define SNM_SCHEDJOB_PLAYLIST_UPDATE	13

/* replaced with a common SWS_CMD_SHORTNAME
#define SNM_CMD_SHORTNAME(_ct) (GetLocalizedActionName(_ct->id, _ct->accel.desc) + 9) // +9 to skip "SWS/S&M: "
*/

static void freecharptr(char* _p) { FREE_NULL(_p); }
static void deleteintptr(int* _p) { DELETE_NULL(_p); }
static void deletefaststrptr(WDL_FastString* _p) { DELETE_NULL(_p); }


///////////////////////////////////////////////////////////////////////////////

class SNM_ScheduledJob {
public:
	SNM_ScheduledJob(int _id, int _approxDelayMs)
		: m_id(_id), m_tick((int)floor((_approxDelayMs/SNM_CSURF_RUN_TICK_MS) + 0.5)) {}
	virtual ~SNM_ScheduledJob() {}
	virtual void Perform() {}
	int m_id, m_tick;
};

class SNM_MarkerRegionSubscriber {
public:
	SNM_MarkerRegionSubscriber() {}
	virtual ~SNM_MarkerRegionSubscriber() {}
	// _updateFlags: 1 marker update, 2 region update, 3 both region & marker updates
	virtual void NotifyMarkerRegionUpdate(int _updateFlags) {}
};

typedef struct MIDI_COMMAND_T {
	gaccel_register_t accel;
	const char* id;
	void (*doCommand)(MIDI_COMMAND_T*,int,int,int,HWND);
	const char* menuText;
	INT_PTR user;
	bool (*getEnabled)(MIDI_COMMAND_T*);
} MIDI_COMMAND_T;


///////////////////////////////////////////////////////////////////////////////

extern bool g_lastPlayState, g_lastPauseState, g_lastRecState;
extern WDL_FastString g_SNMIniFn;
extern WDL_FastString g_SNMCyclactionIniFn;
extern MIDI_COMMAND_T g_SNMSection_cmdTable[];
extern int g_SNMIniFileVersion;
extern int g_SNMbeta;

void EnableToolbarsAutoRefesh(COMMAND_T*);
bool IsToolbarsAutoRefeshEnabled(COMMAND_T*);
void RefreshToolbars();
void FakeToggle(COMMAND_T*);
bool FakeIsToggleAction(COMMAND_T*);
void SNM_ShowActionList(COMMAND_T*);
int SNMRegisterDynamicCommands(COMMAND_T* _pCommands);
int SnMInit(reaper_plugin_info_t* _rec);
void SnMExit();
void AddOrReplaceScheduledJob(SNM_ScheduledJob* _job);
void DeleteScheduledJob(int _id);
bool RegisterToMarkerRegionUpdates(SNM_MarkerRegionSubscriber* _sub);
bool UnregisterToMarkerRegionUpdates(SNM_MarkerRegionSubscriber* _sub) ;
void SnMCSurfRun();
void SnMCSurfSetTrackTitle();
void SnMCSurfSetTrackListChange();
void SnMCSurfSetPlayState(bool _play, bool _pause, bool _rec);
int SnMCSurfExtended(int _call, void* _parm1, void* _parm2, void* _parm3);


///////////////////////////////////////////////////////////////////////////////

#define SNM_CC123_MID_FILE "TVRoZAAAAAYAAAABA8BNVHJrAAAASQCwewACsXsAAbJ7AAGzewACtA==\n\
ewABtXsAAbZ7AAK3ewABuHsAAbl7AAK6ewABu3sAAbx7AAK9ewABvg==\n\
ewABv3sAhyywewAA/y8A\n"

#define SNM_LOGO_PNG_FILE "iVBORw0KGgoAAAANSUhEUgAAADIAAAAUCAMAAAGPE64+AAAAFXRFWA==\n\
dENyZWF0aW9uIFRpbWUAB9oHEQEwJvx8Q0oAAAAHdElNRQfaBxAXJA==\n\
C+Ibk70AAAAJcEhZcwAACvAAAArwAUKsNJgAAAMAUExURQAAAAgICA==\n\
EBAQGBgYISEhKSkpMTExOTk5QkJCSkpKUlJSWlpaY2Nja2trc3Nzew==\n\
e3uEhISMjIyUlJScnJylpaWtra21tbW9vb3GxsbOzs7W1tbe3t7n5w==\n\
5+/v7/f39////////////////////////////////////////////w==\n\
/////////////////////////////////////////////////////w==\n\
/////////////////////////////////////////////////////w==\n\
/////////////////////////////////////////////////////w==\n\
/////////////////////////////////////////////////////w==\n\
/////////////////////////////////////////////////////w==\n\
/////////////////////////////////////////////////////w==\n\
/////////////////////////////////////////////////////w==\n\
/////////////////////////////////////////////////////w==\n\
/////////////////////////////////////////////////////w==\n\
/////////////////////////////////////////////////////w==\n\
/////////////////////////////////////////////////////w==\n\
/////////////////////////////////////////////////////w==\n\
/////////////////////////////////////////////////////w==\n\
/////////////////////////////////////////////////////w==\n\
/////////////////////////////////////////////////////w==\n\
/////////////////////////////////////////////////////w==\n\
//9msyl1AAAAAnRSTlP/AOW3MEoAAAGoSURBVHjanVLbcqswDJQwlA==\n\
W0KggYCBeP//L7symLYv58zU4xhHK8mrlUSPJQp9AvzuvXc1v4c5Fw==\n\
OEAm3Yve0UgX21tvriLACA2QZwpI2VpUFSwphGfHoLyeGguircWO3g==\n\
HpPQKkrvc/FnApF4XGf8cWNzN4bOllQQAn+IiE5gphF4ZygXsIYWaw==\n\
RAJdlgKBpkww3PBwaPU3Uf3N+p9IYpVoSuL2NgZ2PpSlwVR5RmRFVQ==\n\
1NpBsKh6IvdwMgDWTPVmRVDhFTk6uIhkTPZBHPmbcXdW00NOBg0Gag==\n\
TcxV1oa7PxAHvm2t2LHpbDfuyLre8KLLhufkKbP2rfL8mwb/CxHXvQ==\n\
1nUeijg2g9+W+4G4kauX89LJFVLjWN4wk4+dWiP8aeadV5PWZD1Dcg==\n\
/puT7HTa3EIZDDLZFms0K+a1/O7aK77xbizkFvO2wuEbZWCbODfFDA==\n\
z64F+VlLOQZ6Dua4dpaajpNwpPDxaQ9twrF7XSHyaDJROqMpYYyy1Q==\n\
3iwJ0Fssw64SzlIi+8dZ/OISxbhGsZobtpoDrVUq5ZrAYwjPaYw7LQ==\n\
vezn/Q+t/AIQiCv/Q4iRxAAAAABJRU5ErkJggg==\n"


///////////////////////////////////////////////////////////////////////////////

#include "SnM_ChunkParserPatcher.h" 
#include "SnM_Chunk.h"
#include "SnM_Routing.h"
#include "SnM_Dlg.h"
#include "SnM_VWnd.h"
#include "SnM_Cyclactions.h" // from this point order does not matter anymore..
#include "SnM_FindView.h"
#include "SnM_fx.h"
#include "SnM_FXChain.h"
#include "SnM_Item.h"
#include "SnM_LiveConfigs.h"
#include "SnM_ME.h"
#include "SnM_Misc.h"
#include "SnM_NotesHelpView.h"
#include "SnM_Project.h"
#include "SnM_Resources.h"
#include "SnM_RgnPlaylistView.h"
#include "SnM_Track.h"
#include "SnM_Util.h"
#include "SnM_Window.h"

#endif
