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

#pragma once

#ifndef _SNM_H_
#define _SNM_H_


///////////////////////////////////////////////////////////////////////////////
// Definitions, enums
///////////////////////////////////////////////////////////////////////////////

//#define _SNM_MISC
//#define _SNM_MARKER_REGION_NAME
#define _SNM_PRESETS
//#define SWS_CMD_SHORTNAME(_ct) (GetLocalizedActionName(_ct->id, _ct->accel.desc) + 9) // +9 to skip "SWS/S&M: "

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
#define SNM_MAX_TAKES				128
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

// scheduled jobs ids
// note: [0..7] are reserved for Live Configs MIDI CC actions
#define SNM_SCHEDJOB_LIVECFG_TLCHANGE	8
#define SNM_SCHEDJOB_NOTEHLP_TLCHANGE	9
#define SNM_SCHEDJOB_SEL_PRJ			10
#define SNM_SCHEDJOB_TRIG_PRESET		11
#define SNM_SCHEDJOB_CYCLACTION			12

enum {
  SNM_ITEM_SEL_LEFT=0,
  SNM_ITEM_SEL_RIGHT,
  SNM_ITEM_SEL_UP,
  SNM_ITEM_SEL_DOWN
};

static void freecharptr(char* _p) { FREE_NULL(_p); }
static void deleteintptr(int* _p) { DELETE_NULL(_p); }
static void deletefaststrptr(WDL_FastString* _p) { DELETE_NULL(_p); }


///////////////////////////////////////////////////////////////////////////////
// Global types & classes
///////////////////////////////////////////////////////////////////////////////

class SNM_TrackNotes {
public:
	SNM_TrackNotes(MediaTrack* _tr, const char* _notes)
		: m_tr(_tr),m_notes(_notes ? _notes : "") {}
	MediaTrack* m_tr; WDL_FastString m_notes;
};

class SNM_RegionSubtitle {
public:
	SNM_RegionSubtitle(int _id, const char* _notes) 
		: m_id(_id),m_notes(_notes ? _notes : "") {}
	int m_id; WDL_FastString m_notes;
};

class SNM_FXSummary {
public:
	SNM_FXSummary(const char* _type, const char* _name, const char* _realName)
		: m_type(_type),m_name(_name),m_realName(_realName){}
	WDL_FastString m_type, m_name, m_realName;
};

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
	bool (*getEnabled)(MIDI_COMMAND_T*);
} MIDI_COMMAND_T;

class SNM_ImageVWnd : public WDL_VWnd {
public:
	SNM_ImageVWnd(LICE_IBitmap* _img = NULL) : WDL_VWnd() { SetImage(_img); }
	virtual const char *GetType() { return "SNM_ImageVWnd"; }
	virtual int GetWidth();
	virtual int GetHeight();
	virtual void SetImage(LICE_IBitmap* _img) { m_img = _img; }
	virtual void OnPaint(LICE_IBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect);
protected:
	LICE_IBitmap* m_img;
};

LICE_IBitmap* SNM_GetThemeLogo();

class SNM_Logo : public SNM_ImageVWnd {
public:
	SNM_Logo() : SNM_ImageVWnd(SNM_GetThemeLogo()) {}
	virtual const char *GetType() { return "SNM_Logo"; }
	virtual bool GetToolTipString(int xpos, int ypos, char* bufOut, int bufOutSz) { lstrcpyn(bufOut, "Strong & Mighty", bufOutSz); return true; }
};

class SNM_AddDelButton : public WDL_VWnd {
public:
	SNM_AddDelButton() : WDL_VWnd() { m_add=true; m_en=true; }
	virtual const char *GetType() { return "SNM_AddDelButton"; }
	virtual void OnPaint(LICE_IBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect);
	virtual void SetAdd(bool _add) { m_add=_add; }
	virtual void SetEnabled(bool _en) { m_en=_en; }
	virtual int OnMouseDown(int xpos, int ypos) { return m_en?1:0;	}
	virtual void OnMouseUp(int xpos, int ypos) { if (m_en) SendCommand(WM_COMMAND,GetID(),0,this); }
protected:
	bool m_add, m_en;
};

class SNM_MiniAddDelButtons : public WDL_VWnd {
public:
	SNM_MiniAddDelButtons() : WDL_VWnd()
	{
		m_btnPlus.SetID(0xF666); // temp ids.. SetIDs() must be used
		m_btnMinus.SetID(0xF667);
		m_btnPlus.SetAdd(true);
		m_btnMinus.SetAdd(false); 
		AddChild(&m_btnPlus);
		AddChild(&m_btnMinus);
	}
	~SNM_MiniAddDelButtons() { RemoveAllChildren(false); }
	virtual void SetIDs(int _id, int _addId, int _delId) { SetID(_id); m_btnPlus.SetID(_addId); m_btnMinus.SetID(_delId); }
	virtual const char *GetType() { return "SNM_MiniAddDelButtons"; }
	virtual void SetPosition(const RECT *r)
	{
		m_position=*r; 
		RECT rr = {0, 0, 9, 9};
		m_btnPlus.SetPosition(&rr);
		RECT rr2 = {0, 10, 9, 19};
		m_btnMinus.SetPosition(&rr2);
	}
protected:
	SNM_AddDelButton m_btnPlus, m_btnMinus;
};

class SNM_ToolbarButton : public WDL_VirtualIconButton {
public:
	SNM_ToolbarButton() : WDL_VirtualIconButton() {}
	const char *GetType() { return "SNM_ToolbarButton"; }
	void SetGrayed(bool grayed) { WDL_VirtualIconButton::SetGrayed(grayed); if (grayed) m_pressed=0; } // avoid stuck overlay when mousedown leads to grayed button
	void GetPositionPaintOverExtent(RECT *r) { *r=m_position; }
	void OnPaintOver(LICE_IBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect);
};

class SNM_MiniKnob : public WDL_VirtualSlider {
public:
	SNM_MiniKnob() : WDL_VirtualSlider() {}
	const char *GetType() { return "SNM_MiniKnob"; }
};


///////////////////////////////////////////////////////////////////////////////

extern bool g_lastPlayState, g_lastPauseState, g_lastRecState;
extern WDL_FastString g_SNMIniFn;
extern WDL_FastString g_SNMCyclactionIniFn;
extern MIDI_COMMAND_T g_SNMSection_cmdTable[];
extern int g_SNMIniFileVersion;
extern int g_SNMbeta;
extern bool g_SNMClearType;

void EnableToolbarsAutoRefesh(COMMAND_T*);
bool IsToolbarsAutoRefeshEnabled(COMMAND_T*);
void RefreshToolbars();
void FakeToggle(COMMAND_T*);
bool FakeIsToggleAction(COMMAND_T*);
#ifdef _SNM_MISC
bool FakeIsToggleAction(MIDI_COMMAND_T*);
#endif
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
// Base64 stuff
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

#endif
