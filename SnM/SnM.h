/******************************************************************************
/ SnM.h
/
/ Copyright (c) 2009-2013 Jeffos
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


///////////////////////////////////////////////////////////////////////////////
// Activate/deactivate some features, traces, etc.
///////////////////////////////////////////////////////////////////////////////

//#define _SNM_DEBUG
//#define _SNM_SCREENSET_DEBUG
//#define _SNM_DYN_FONT_DEBUG
//#define _SNM_MISC				// not released, deprecated, tests, etc..
//#define _SNM_WDL				// if my wdl version is used
#define _SNM_HOST_AW			// hosts some of Adam's stuff
//#define _SNM_CSURF_PROXY


///////////////////////////////////////////////////////////////////////////////
// Just to ease sustaining vs S&M/API/REAPER/WDL bugs and/or issues: 
// #undef to activate the "expected code", #define to activate workarounds
///////////////////////////////////////////////////////////////////////////////

#define _SNM_REAPER_BUG			// workaround some API/REAPER bugs
								// Last test: REAPER v4.34rc1
								// API bug 1: CountActionShortcuts() can return > 0 although there is no shortcut
								// API bug 2: DoActionShortcutDialog() does not persist *all* learned shortcuts
								//            note: a fix announced in v4.33pre29 but still buggy
								// API bug 3: GetToggleCommandState2() does not work

#ifdef __APPLE__
  #define _SNM_SWELL_ISSUES		// workaround some SWELL issues
#endif							// Last test: WDL d1d8d2 - Dec. 20 2012
								// - issue 1: native font rendering won't draw multiple lines
								// - issue 2: missing some EN_CHANGE msg, see SnM_Notes.cpp
								// - issue 3: EN_SETFOCUS, EN_KILLFOCUS not yet supported

#define _SNM_NO_ASYNC_UPDT		// no async UI updates


///////////////////////////////////////////////////////////////////////////////
// Constants
///////////////////////////////////////////////////////////////////////////////

#define SNM_INI_FILE_VERSION		7

#ifdef _WIN32

#define SNM_FORMATED_INI_FILE		"%s\\S&M.ini"
#define SNM_OLD_FORMATED_INI_FILE	"%s\\Plugins\\S&M.ini"
#define SNM_ACTION_HELP_INI_FILE	"%s\\S&M_Action_help_en.ini"
#define SNM_CYCLACTION_INI_FILE		"%s\\S&M_Cyclactions.ini"
#define SNM_CYCLACTION_BAK_FILE		"%s\\S&M_Cyclactions.bak"
#define SNM_CYCLACTION_EXPORT_FILE	"%s\\S&M_Cyclactions_export.ini"
#define SNM_KB_INI_FILE				"%s\\reaper-kb.ini"
#define SNM_CONSOLE_FILE			"%s\\reaconsole_customcommands.txt"
#define SNM_REAPER_EXE_FILE			"%s\\reaper.exe"
#define SNM_FONT_NAME				"MS Shell Dlg"
#define SNM_FONT_HEIGHT				14
#define SNM_DYN_FONT_NAME			"Arial" // good default for UTF8
#define SNM_1PIXEL_Y				1
#define SNM_GUI_TOP_H				36
#define SNM_GUI_BOT_H				41
#define SNM_COL_RED_MONITOR			0x0000BE

#else

#define SNM_FORMATED_INI_FILE		"%s/S&M.ini"
#define SNM_OLD_FORMATED_INI_FILE	"%s/Plugins/S&M.ini"
#define SNM_ACTION_HELP_INI_FILE	"%s/S&M_Action_help_en.ini"
#define SNM_CYCLACTION_INI_FILE		"%s/S&M_Cyclactions.ini"
#define SNM_CYCLACTION_BAK_FILE		"%s/S&M_Cyclactions.bak"
#define SNM_CYCLACTION_EXPORT_FILE	"%s/S&M_Cyclactions_export.ini"
#define SNM_KB_INI_FILE				"%s/reaper-kb.ini"
#define SNM_CONSOLE_FILE			"%s/reaconsole_customcommands.txt"
#ifdef __LP64__
#define SNM_REAPER_EXE_FILE			"%s/REAPER64.app"
#else
#define SNM_REAPER_EXE_FILE			"%s/REAPER.app"
#endif
#define SNM_FONT_NAME				"Lucia grande"
#define SNM_FONT_HEIGHT				10
#define SNM_DYN_FONT_NAME			"Arial" // good default for UTF8
#define SNM_1PIXEL_Y				(-1)
#define SNM_GUI_TOP_H				37
#define SNM_GUI_BOT_H				43
#define SNM_COL_RED_MONITOR			0xBE0000

#endif

#define SNM_LIVECFG_NB_CONFIGS		4			//JFB do not commit a new value, contact me plz thx!
#define SNM_CSURF_RUN_TICK_MS		27.0   // 1 tick ~= 27ms (average I monitored)
#define SNM_DEF_TOOLBAR_RFRSH_FREQ	300    // default frequency in ms for the "auto-refresh toolbars" option 
#define SNM_FUDGE_FACTOR			0.0000000001
#define SNM_CSURF_EXT_UNREGISTER	0x00016666
#define SNM_REAPER_IMG_EXTS			"png,pcx,jpg,jpeg,jfif,ico,bmp" // img exts supported by REAPER (v4.32), can't get those at runtime yet
#define SNM_INI_EXT_LIST			"INI files (*.INI)\0*.INI\0All Files\0*.*\0"
#define SNM_SUB_EXT_LIST			"SubRip subtitle files (*.SRT)\0*.SRT\0"
#define SNM_TXT_EXT_LIST			"Text files (*.txt)\0*.txt\0All files (*.*)\0*.*\0"

#define SNM_MARKER_MASK				1
#define SNM_REGION_MASK				2

#define SNM_GUI_X_MARGIN			6
#define SNM_GUI_X_MARGIN_OLD		8
#define SNM_GUI_X_MARGIN_LOGO		SNM_GUI_X_MARGIN
#define SNM_GUI_Y_MARGIN			SNM_GUI_X_MARGIN
#define SNM_GUI_Y_MARGIN_LOGO		10
#define SNM_GUI_W_KNOB				26
#define SNM_3D_COLORS_DELTA			25

#define SNM_MAX_PATH				2048
#define SNM_MAX_HW_OUTS				8
#define SNM_MAX_CC_LANE_ID			133
#define SNM_MAX_CC_LANES_LEN		4096
#define SNM_MAX_TRACK_GROUPS		32
#define SNM_MAX_CUE_BUSS_CONFS		8
//#define SNM_MAX_TAKES				1024
#define SNM_MAX_FX					128
#define SNM_MAX_PRESETS				0xFFFF
#define SNM_MAX_INI_SECTION			0xFFFF // definitive limit for WritePrivateProfileSection

#define SNM_MAX_OSC_MSG_LEN			256
#define SNM_MAX_SECTION_NAME_LEN	64
#define SNM_MAX_SECTION_ACTIONS		128
#define SNM_MAX_ACTION_CUSTID_LEN	128
#define SNM_MAX_ACTION_NAME_LEN		128
#define SNM_MAX_ENV_CHUNKNAME_LEN	32
#define SNM_MAX_MARKER_NAME_LEN		64     // + regions
#define SNM_MAX_TRACK_NAME_LEN		128
#define SNM_MAX_PRESET_NAME_LEN		128
#define SNM_MAX_FX_NAME_LEN			128

// scheduled job ids
// [0 .. SNM_LIVECFG_NB_CONFIGS-1] are reserved for "apply live config" actions
// [SNM_LIVECFG_NB_CONFIGS .. 2*SNM_LIVECFG_NB_CONFIGS-1] are reserved for "preload live config" actions
#define SNM_SCHEDJOB_LIVECFG_APPLY			0
#define SNM_SCHEDJOB_LIVECFG_PRELOAD		SNM_SCHEDJOB_LIVECFG_APPLY + SNM_LIVECFG_NB_CONFIGS
#define SNM_SCHEDJOB_LIVECFG_UPDATE			SNM_SCHEDJOB_LIVECFG_PRELOAD + SNM_LIVECFG_NB_CONFIGS
#define SNM_SCHEDJOB_LIVECFG_FADE_UPDATE	SNM_SCHEDJOB_LIVECFG_UPDATE + 1
#define SNM_SCHEDJOB_UNDO					SNM_SCHEDJOB_LIVECFG_FADE_UPDATE + 1
#define SNM_SCHEDJOB_NOTES_UPDATE			SNM_SCHEDJOB_UNDO + 1
#define SNM_SCHEDJOB_SEL_PRJ				SNM_SCHEDJOB_NOTES_UPDATE + 1
#define SNM_SCHEDJOB_TRIG_PRESET			SNM_SCHEDJOB_SEL_PRJ + 1
#define SNM_SCHEDJOB_RES_ATTACH				SNM_SCHEDJOB_TRIG_PRESET + 1
#define SNM_SCHEDJOB_PLAYLIST_UPDATE		SNM_SCHEDJOB_RES_ATTACH + 1
#define SNM_SCHEDJOB_PRJ_ACTION				SNM_SCHEDJOB_PLAYLIST_UPDATE + 1
#define SNM_SCHEDJOB_OSX_FIX				SNM_SCHEDJOB_PRJ_ACTION + 1	//JFB!! removeme some day

#define SNM_SCHEDJOB_DEFAULT_DELAY	250
#define SNM_SCHEDJOB_SLOW_DELAY		500    // for non urgent stuff, e.g. UI update


///////////////////////////////////////////////////////////////////////////////
// Scheduled jobs
///////////////////////////////////////////////////////////////////////////////

class SNM_ScheduledJob {
public:
	SNM_ScheduledJob(int _id, int _approxDelayMs)
		: m_id(_id), m_tick((int)floor((_approxDelayMs/SNM_CSURF_RUN_TICK_MS) + 0.5)),m_isPerforming(false) {}
	virtual ~SNM_ScheduledJob() {}
	virtual void Perform() {}
	int m_id, m_tick;
	bool m_isPerforming;
};

// avoid undo points flooding
// i.e. async undo, somehow => handle with care!
class UndoJob : public SNM_ScheduledJob {
public:
	UndoJob(const char* _desc, int _flags, int _tr = -1) 
		: SNM_ScheduledJob(SNM_SCHEDJOB_UNDO, SNM_SCHEDJOB_DEFAULT_DELAY), 
		m_desc(_desc), m_flags(_flags), m_tr(_tr) {}
	void Perform() { Undo_OnStateChangeEx2(NULL, m_desc, m_flags, m_tr); }
private:
	const char* m_desc;
	int m_flags, m_tr;
};

class SNM_MidiActionJob : public SNM_ScheduledJob {
public:
	SNM_MidiActionJob(int _jobId, int _approxDelayMs, int _curCC, int _val, int _valhw, int _relmode, HWND _hwnd); 
	virtual void Perform() {}
	virtual int GetAbsoluteValue() { return m_absval; }
protected:
	int m_val, m_valhw, m_relmode; // values from the controller
	int m_absval; // internal absolute value (deduced from the above)
	HWND m_hwnd;
};

void SNM_AddOrReplaceScheduledJob(SNM_ScheduledJob* _job);


///////////////////////////////////////////////////////////////////////////////
// Common global funcs
///////////////////////////////////////////////////////////////////////////////

// fake action toggle states
int SNM_GetFakeToggleState(COMMAND_T*);

// S&M core stuff
int SNM_Init(reaper_plugin_info_t* _rec);
void SNM_Exit();

static void freecharptr(char* _p) { FREE_NULL(_p); }
static void deleteintptr(int* _p) { DELETE_NULL(_p); }
static void deletefaststrptr(WDL_FastString* _p) { DELETE_NULL(_p); }


///////////////////////////////////////////////////////////////////////////////
// Misc globals vars, structs & classes
///////////////////////////////////////////////////////////////////////////////

/* commented: replaced with a common SWS_CMD_SHORTNAME()
#define SNM_CMD_SHORTNAME(_ct) (GetLocalizedActionName(_ct->id, _ct->accel.desc) + 9) // +9 to skip "SWS/S&M: "
*/

// global/common S&M vars
extern SWS_Mutex g_SNM_JobsMutex;
extern WDL_PtrList_DeleteOnDestroy<SNM_ScheduledJob> g_SNM_Jobs;

extern int g_SNM_IniVersion;
extern int g_SNM_Beta;
extern WDL_FastString g_SNM_IniFn, g_SNM_CyclIniFn, g_SNM_DiffToolFn;

class SNM_TrackInt {
public:
	SNM_TrackInt(MediaTrack* _tr, int _i) : m_tr(_tr), m_int(_i) {}
	~SNM_TrackInt() {}
	MediaTrack* m_tr;
	int m_int;
};

typedef struct MIDI_COMMAND_T {
	gaccel_register_t accel;
	const char* id;
	void (*doCommand)(MIDI_COMMAND_T*,int,int,int,HWND);
	const char* menuText;
	INT_PTR user;
#ifdef _SNM_MISC
	int (*getEnabled)(MIDI_COMMAND_T*);
	bool fakeToggle;
#endif
} MIDI_COMMAND_T;

typedef struct SECTION_INFO_T {
	int unique_id;
	const char* ca_cust_id;
	const char* ca_ini_sec;
} SECTION_INFO_T;


///////////////////////////////////////////////////////////////////////////////
// Actions & sections (as of REAPER v4.32)
///////////////////////////////////////////////////////////////////////////////

#define SNM_SECTION_ID				0x10000101		// "< 0x10000000 for cockos use only plzk thx"
#define SNM_SECTION_1ST_CMD_ID		40000

// section indexes, order does not matter here 
// important: keep s_sectionInfos in sync!
enum {
  SNM_SEC_IDX_MAIN=0,
  SNM_SEC_IDX_MAIN_ALT,
  SNM_SEC_IDX_EPXLORER,
  SNM_SEC_IDX_ME,
  SNM_SEC_IDX_ME_EL,
  SNM_SEC_IDX_ME_INLINE,
  SNM_SEC_IDX_SNM,
  SNM_NUM_MANAGED_SECTIONS
};

static SECTION_INFO_T s_SNM_sectionInfos[] = {
	{0,		"S&M_CYCLACTION_",			"Main_Cyclactions"},
	{100,	"S&M_MAIN_ALT_CYCLACTION",	"MainAlt_Cyclactions"},
	{32063,	"S&M_MEDIAEX_CYCLACTION",	"MediaEx_Cyclactions"},
	{32060,	"S&M_ME_PIANO_CYCLACTION",	"ME_Piano_Cyclactions"},
	{32061,	"S&M_ME_LIST_CYCLACTION",	"ME_List_Cyclactions"},
	{32062,	"S&M_ME_INLINE_CYCLACTION", "ME_Inline_Cyclactions"},
	{SNM_SECTION_ID, "", ""}
};

#define SNM_MACRO_CUSTID_LEN		32
#define SNM_NUM_NATIVE_SECTIONS		SNM_SEC_IDX_SNM
#define SNM_MAX_CYCLING_SECTIONS	SNM_NUM_NATIVE_SECTIONS
#define SNM_MAX_DYNAMIC_ACTIONS		99 // if > 99, the "dynamic action" code must be updated


///////////////////////////////////////////////////////////////////////////////
// Misc common data
///////////////////////////////////////////////////////////////////////////////

// make sure the last MIDI event is a (dummy) CC123!
#define SNM_CC123_MID_STATE "HASDATA 1 960 QN\n\
E 0 b0 7b 00\n\
E 0 b1 7b 00\n\
E 0 b2 7b 00\n\
E 0 b3 7b 00\n\
E 0 b4 7b 00\n\
E 0 b5 7b 00\n\
E 0 b6 7b 00\n\
E 0 b7 7b 00\n\
E 0 b8 7b 00\n\
E 0 b9 7b 00\n\
E 0 ba 7b 00\n\
E 0 bb 7b 00\n\
E 0 bc 7b 00\n\
E 0 bd 7b 00\n\
E 0 be 7b 00\n\
E 0 bf 7b 00\n\
E 1 b0 7b 00\n\
IGNTEMPO 0 120.00000000 4 4\n\
>\n"

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


#endif
