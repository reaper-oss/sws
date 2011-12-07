/******************************************************************************
/ SnM_Actions.h
/ JFB TODO? split into several .h + a better name would be "SnM.h"
/
/ Copyright (c) 2009-2011 Tim Payne (SWS), Jeffos
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

#include "SNM_ChunkParserPatcher.h"


///////////////////////////////////////////////////////////////////////////////
// Definitions, enums
///////////////////////////////////////////////////////////////////////////////

//#define _SNM_MISC
#define _SNM_CYCLACTION_OSX
#ifdef _WIN32
#define _SNM_PRESETS
//#define _SNM_LIST_THEMABLE1
//#define _SNM_EDIT_THEMABLE
#endif


#define SNM_CMD_SHORTNAME(_ct) (_ct->accel.desc + 9) // +9 to skip "SWS/S&M: "

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

#define SNM_INI_FILE_VERSION		3
#define SNM_INI_EXT_LIST			"INI files (*.INI)\0*.INI\0All Files\0*.*\0"
#define SNM_MAX_SECTION_NAME_LEN	64
#define SNM_MAX_SECTION_ACTIONS		128
#define SNM_MAX_ACTION_CUSTID_LEN	128
#define SNM_MAX_ACTION_NAME_LEN		128
#define SNM_MAX_MARKER_NAME_LEN		64     // + regions
#define SNM_MAX_TRACK_GROUPS		32
#define SNM_MAX_TRACK_GROUPS_STR	"32"
#define SNM_MAX_TRACK_NAME_LEN		128
#define SNM_MAX_HW_OUTS				8
#define SNM_MAX_TAKES				128
#define SNM_MAX_FX					128
#define SNM_MAX_INI_SECTION			0xFFFF // definitive limit for WritePrivateProfileSection
#define SNM_MAX_DYNAMIC_ACTIONS		99     // if > 99 the display of action names should be updated
#define SNM_MAX_CYCLING_ACTIONS		8
#define SNM_MAX_CYCLING_SECTIONS	3
#define SNM_MAX_ENV_SUBCHUNK_NAME	16
#define SNM_MAX_SLOT_TYPES			16
#define SNM_LET_BREATHE_MS			10
#define SNM_3D_COLORS_DELTA			25
#define SNM_CSURF_RUN_TICK_MS		27     // 1 tick = 27ms or so (average I monitored)
#define SNM_DEF_TOOLBAR_RFRSH_FREQ	300    // default frequency in ms for the "auto-refresh toolbars" option 
#define SNM_SCHEDJOB_DEFAULT_DELAY	250
#define SNM_DEF_VWND_X_STEP			12

// Scheduled job *RESERVED* ids
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


///////////////////////////////////////////////////////////////////////////////
// Global types & classes
///////////////////////////////////////////////////////////////////////////////

class SNM_TrackNotes {
public:
	SNM_TrackNotes(MediaTrack* _tr, const char* _notes)
		: m_tr(_tr),m_notes(_notes ? _notes : "") {}
	MediaTrack* m_tr; WDL_FastString m_notes;
};

class SNM_FXSummary {
public:
	SNM_FXSummary(const char* _type, const char* _realName)
		: m_type(_type),m_realName(_realName){}
	virtual ~SNM_FXSummary() {}
	WDL_FastString m_type, m_realName;
};

class SNM_ScheduledJob {
public:
	SNM_ScheduledJob(int _id, int _approxDelayMs)
		: m_id(_id), m_tick((int)floor((_approxDelayMs/SNM_CSURF_RUN_TICK_MS) + 0.5)) {}
	virtual ~SNM_ScheduledJob() {}
	virtual void Perform() {}
	int m_id, m_tick;
};

class SNM_SndRcv {
public:
	SNM_SndRcv() {}
	SNM_SndRcv(MediaTrack* _src, MediaTrack* _dest, bool _mute, int _phase, int _mono,
		double _vol, double _pan, double _panl, int _mode, int _srcChan, int _destChan, int _midi)
		: m_src(_src),m_dest(_dest),m_mute(_mute),m_phase(_phase),m_mono(_mono),
		m_vol(_vol),m_pan(_pan),m_panl(_panl),m_mode(_mode),m_srcChan(_srcChan),m_destChan(_destChan),m_midi(_midi) {}
	virtual ~SNM_SndRcv() {}
	MediaTrack* m_src; MediaTrack* m_dest;
	bool m_mute;
	int m_phase, m_mono, m_mode, m_srcChan, m_destChan, m_midi;
	double m_vol, m_pan, m_panl;
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
	int excecCount;
} MIDI_COMMAND_T;

class SNM_ImageVWnd : public WDL_VWnd {
public:
	SNM_ImageVWnd(LICE_IBitmap* _img) : WDL_VWnd() { m_img = _img; }
	virtual const char *GetType() { return "SNM_ImageVWnd"; }
	virtual int GetWidth();
	virtual int GetHeight();
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


///////////////////////////////////////////////////////////////////////////////
// Includes & externs
///////////////////////////////////////////////////////////////////////////////

// *** SnM_Actions.cpp ***
extern WDL_FastString g_SNMIniFn;
extern WDL_FastString g_SNMCyclactionIniFn;
extern int g_iniFileVersion;
extern int g_bSNMbeta;
void EnableToolbarsAutoRefesh(COMMAND_T*);
bool IsToolbarsAutoRefeshEnabled(COMMAND_T*);
void RefreshToolbars();
void FakeToggle(COMMAND_T*);
bool FakeIsToggleAction(COMMAND_T*);
#ifdef _SNM_MISC
bool FakeIsToggleMidiAction(MIDI_COMMAND_T*);
#endif
void SNM_ShowActionList(COMMAND_T*);
int SNMRegisterDynamicCommands(COMMAND_T* _pCommands);
int SnMInit(reaper_plugin_info_t* _rec);
void SnMExit();
void AddOrReplaceScheduledJob(SNM_ScheduledJob* _job);
void DeleteScheduledJob(int _id);
void SnMCSurfRun();
void SnMCSurfSetTrackTitle();
void SnMCSurfSetTrackListChange();

// *** SnM_Cyclactions.cpp
int RegisterCyclation(const char* _name, bool _toggle, int _type, int _cycleId, int _cmdId);
#ifdef _SNM_CYCLACTION_OSX
int CyclactionViewInit();
void CyclactionViewExit();
void OpenCyclactionView(COMMAND_T*);
bool IsCyclactionViewDisplayed(COMMAND_T*);
#else
int CyclactionsInit();
void openCyclactionsWnd(COMMAND_T*);
bool isCyclationsWndDisplayed(COMMAND_T*);
#endif

// *** SnM_Dlg.cpp ***
ColorTheme* SNM_GetColorTheme(bool _checkForSize = false);
IconTheme* SNM_GetIconTheme(bool _checkForSize = false);
LICE_CachedFont* SNM_GetThemeFont();
LICE_CachedFont* SNM_GetToolbarFont();
HBRUSH SNM_GetThemeBrush(int _col=-666);
void SNM_GetThemeListColors(int* _bg, int* _txt);
void SNM_GetThemeEditColors(int* _bg, int* _txt);
void SNM_ThemeListView(SWS_ListView* _lv);
/* defined on top, see SNM_ImageVWnd
LICE_IBitmap* SNM_GetThemeLogo();
*/
void SNM_SkinButton(WDL_VirtualIconButton* _btn, WDL_VirtualIconButton_SkinConfig* _skin, const char* _text);
void SNM_SkinToolbarButton(SNM_ToolbarButton* _btn, const char* _text);
bool SNM_AddLogo(LICE_IBitmap* _bm, const RECT* _r, int _x, int _h);
bool SNM_AddLogo2(SNM_Logo* _logo, const RECT* _r, int _x, int _h);
bool SNM_AutoVWndPosition(WDL_VWnd* _c, WDL_VWnd* _tiedComp, const RECT* _r, int* _x, int _y, int _h, int _xStep = SNM_DEF_VWND_X_STEP);
void SNM_UIInit();
void SNM_UIExit();
void SNM_ShowMsg(const char* _msg, const char* _title = "", HWND _hParent = NULL, bool _clear = true); 
int PromptForInteger(const char* _title, const char* _what, int _min, int _max);
void openCueBussWnd(COMMAND_T*);
bool isCueBussWndDisplayed(COMMAND_T*);
#ifdef _SNM_MISC
WDL_DLGRET WaitDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
#endif

// *** SnM_FindView.cpp ***
int FindViewInit();
void FindViewExit();
void OpenFindView(COMMAND_T*);
bool IsFindViewDisplayed(COMMAND_T*);
void FindNextPrev(COMMAND_T*);

// *** SnM_fx.cpp ***
extern int g_buggyPlugSupport;
void toggleFXOfflineSelectedTracks(COMMAND_T*);
bool isFXOfflineSelectedTracks(COMMAND_T*);
void toggleFXBypassSelectedTracks(COMMAND_T*);
bool isFXBypassedSelectedTracks(COMMAND_T*);
void toggleExceptFXOfflineSelectedTracks(COMMAND_T*);
void toggleExceptFXBypassSelectedTracks(COMMAND_T*);
void toggleAllFXsOfflineSelectedTracks(COMMAND_T*);
void toggleAllFXsBypassSelectedTracks(COMMAND_T*);
void setFXOfflineSelectedTracks(COMMAND_T*); 
void setFXBypassSelectedTracks(COMMAND_T*);
void setFXOnlineSelectedTracks(COMMAND_T*);
void setFXUnbypassSelectedTracks(COMMAND_T*);
void setAllFXsBypassSelectedTracks(COMMAND_T*);
void toggleAllFXsOfflineSelectedItems(COMMAND_T*);
void toggleAllFXsBypassSelectedItems(COMMAND_T*);
void setAllFXsOfflineSelectedItems(COMMAND_T*);
void setAllFXsBypassSelectedItems(COMMAND_T*);
void selectTrackFX(COMMAND_T*);
int getSelectedTrackFX(MediaTrack* _tr);
int getPresetNames(const char* _fxType, const char* _fxName, WDL_PtrList<WDL_FastString>* _names);
void UpdatePresetConf(int _fx, int _preset, WDL_FastString* _presetConf);
int GetPresetFromConf(int _fx, WDL_FastString* _presetConf, int _presetCount=0xFFFF);
void RenderPresetConf(WDL_FastString* _presetConf, WDL_FastString* _renderConf);
void RenderPresetConf2(MediaTrack* _tr, WDL_FastString* _presetConf, WDL_FastString* _renderConf);
int triggerFXPreset(MediaTrack* _tr, int _fxId, int _presetId, int _dir = 0, bool _userPreset = false, bool _selTracks = true);
void triggerFXPreset(int _fxId, int _presetId, int _dir=0);
void triggerNextPreset(COMMAND_T*);
void triggerPreviousPreset(COMMAND_T*);
bool triggerFXUserPreset(MediaTrack* _tr, WDL_FastString* _presetConf);
void TriggerFXPreset(MIDI_COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd);
void moveFX(COMMAND_T*);

// *** SnM_FXChain.cpp ***
void makeChunkTakeFX(WDL_FastString* _outTakeFX, const WDL_FastString* _inRfxChain);
int copyTakeFXChain(WDL_FastString* _fxChain, int _startSelItem=0);
void pasteTakeFXChain(const char* _title, WDL_FastString* _chain, bool _activeOnly);
void setTakeFXChain(const char* _title, WDL_FastString* _chain, bool _activeOnly);
void applyTakesFXChainSlot(int _slotType, const char* _title, int _slot, bool _activeOnly, bool _set);
bool autoSaveItemFXChainSlots(int _slotType, const char* _dirPath, char* _fn, int _fnSize);
void loadSetTakeFXChain(COMMAND_T*);
void loadPasteTakeFXChain(COMMAND_T*);
void loadSetAllTakesFXChain(COMMAND_T*);
void loadPasteAllTakesFXChain(COMMAND_T*);
void copyTakeFXChain(COMMAND_T*);
void cutTakeFXChain(COMMAND_T*);
void pasteTakeFXChain(COMMAND_T*);
void setTakeFXChain(COMMAND_T*);
void pasteAllTakesFXChain(COMMAND_T*);
void setAllTakesFXChain(COMMAND_T*);
void clearActiveTakeFXChain(COMMAND_T*);
void clearAllTakesFXChain(COMMAND_T*);
void pasteTrackFXChain(const char* _title, WDL_FastString* _chain, bool _inputFX);
void setTrackFXChain(const char* _title, WDL_FastString* _chain, bool _inputFX);
int copyTrackFXChain(WDL_FastString* _fxChain, bool _inputFX, int _startTr=0);
void applyTracksFXChainSlot(int _slotType, const char* _title, int _slot, bool _set, bool _inputFX);
bool autoSaveTrackFXChainSlots(int _slotType, const char* _dirPath, char* _fn, int _fnSize, bool _inputFX);
void loadSetTrackFXChain(COMMAND_T*);
void loadPasteTrackFXChain(COMMAND_T*);
void loadSetTrackInFXChain(COMMAND_T*);
void loadPasteTrackInFXChain(COMMAND_T*);
void clearTrackFXChain(COMMAND_T*);
void copyTrackFXChain(COMMAND_T*);
void cutTrackFXChain(COMMAND_T*);
void pasteTrackFXChain(COMMAND_T*);
void setTrackFXChain(COMMAND_T*);
void clearTrackInputFXChain(COMMAND_T*);
void copyTrackInputFXChain(COMMAND_T*);
void cutTrackInputFXChain(COMMAND_T*);
void pasteTrackInputFXChain(COMMAND_T*);
void setTrackInputFXChain(COMMAND_T*);
void copyFXChainSlotToClipBoard(int _slot);
void smartCopyFXChain(COMMAND_T*);
void smartPasteFXChain(COMMAND_T*);
void smartPasteReplaceFXChain(COMMAND_T*);
void smartCutFXChain(COMMAND_T*);
void reassignLearntMIDICh(COMMAND_T*);

// *** SnM_Item.cpp ***
char* GetName(MediaItem* _item);
int getTakeIndex(MediaItem* _item, MediaItem_Take* _take);
bool deleteMediaItemIfNeeded(MediaItem* _item);
void SNM_GetSelectedItems(ReaProject* _proj, WDL_PtrList<MediaItem>* _items, bool _onSelTracks = false);
void SNM_SetSelectedItems(ReaProject* _proj, WDL_PtrList<MediaItem>* _items, bool _onSelTracks = false);
void SNM_ClearSelectedItems(ReaProject* _proj, bool _onSelTracks = false);
void splitMidiAudio(COMMAND_T*);
void smartSplitMidiAudio(COMMAND_T*);
#ifdef _SNM_MISC // Deprecated (v3.67)
void splitSelectedItems(COMMAND_T*);
#endif
void goferSplitSelectedItems(COMMAND_T*);
void copyCutTake(COMMAND_T*);
void pasteTake(COMMAND_T*);
bool isEmptyMidi(MediaItem_Take* _take);
void setEmptyTakeChunk(WDL_FastString* _chunk, int _recPass = -1, int _color = -1);
int buildLanes(const char* _undoTitle, int _mode);
bool removeEmptyTakes(MediaTrack* _tr, bool _empty, bool _midiEmpty, bool _trSel, bool _itemSel);
bool removeEmptyTakes(const char* _undoTitle, bool _empty, bool _midiEmpty, bool _trSel = false, bool _itemSel = true);
void clearTake(COMMAND_T*);
#ifdef _SNM_MISC // Deprecated (v3.67)
void moveTakes(COMMAND_T*);
#endif
void moveActiveTake(COMMAND_T*);
void activateLaneFromSelItem(COMMAND_T*);
void activateLaneUnderMouse(COMMAND_T*);
void buildLanes(COMMAND_T*);
void removeEmptyTakes(COMMAND_T*);
void removeEmptyMidiTakes(COMMAND_T*);
void removeAllEmptyTakes(COMMAND_T*);
void deleteTakeAndMedia(COMMAND_T*);
int getPitchTakeEnvRangeFromPrefs();
void panTakeEnvelope(COMMAND_T*);
void showHideTakeVolEnvelope(COMMAND_T*); 
void showHideTakePanEnvelope(COMMAND_T*);
void showHideTakeMuteEnvelope(COMMAND_T*);
void showHideTakePitchEnvelope(COMMAND_T*);
bool ShowTakeEnvVol(MediaItem_Take* _take);
bool ShowTakeEnvPan(MediaItem_Take* _take);
bool ShowTakeEnvMute(MediaItem_Take* _take);
bool ShowTakeEnvPitch(MediaItem_Take* _take);
#ifdef _SNM_MISC
void saveItemTakeTemplate(COMMAND_T*);
#endif
void itemSelToolbarPoll();
void toggleItemSelExists(COMMAND_T*);
bool itemSelExists(COMMAND_T*);
void scrollToSelItem(MediaItem* _item);
void scrollToSelItem(COMMAND_T*);
void setPan(COMMAND_T*);
void PlaySelTrackMediaSlot(const char* _title, int _slot, bool _loop);
void PlaySelTrackMediaSlot(COMMAND_T*);
void LoopSelTrackMediaSlot(COMMAND_T*);
bool TogglePlaySelTrackMediaSlot(int _slotType, const char* _title, int _slot, bool _loop);
void TogglePlaySelTrackMediaSlot(COMMAND_T*);
void ToggleLoopSelTrackMediaSlot(COMMAND_T*);
void InsertMediaSlot(int _slotType, const char* _title, int _slot, int _insertMode);
void InsertMediaSlotCurTr(COMMAND_T*);
void InsertMediaSlotNewTr(COMMAND_T*);
void InsertMediaSlotTakes(COMMAND_T*);
bool autoSaveMediaSlot(int _slotType, const char* _dirPath, char* _fn, int _fnSize);

// *** SnM_LiveConfigsView.cpp ***
int LiveConfigViewInit();
void LiveConfigViewExit();
void OpenLiveConfigView(COMMAND_T*);
bool IsLiveConfigViewDisplayed(COMMAND_T*);
void ApplyLiveConfig(MIDI_COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd);
void ToggleEnableLiveConfig(COMMAND_T*);
bool IsLiveConfigEnabled(COMMAND_T*);

// *** SnM_ME.cpp ***
void MECreateCCLane(COMMAND_T*);
void MEHideCCLanes(COMMAND_T*);
void MESetCCLanes(COMMAND_T*);
void MESaveCCLanes(COMMAND_T*);

// *** SnM_Misc.cpp ***
const char* GetFileExtension(const char* _fn);
void GetFilenameNoExt(const char* _fullFn, char* _fn, int _fnSz);
const char* GetFilenameWithExt(const char* _fullFn);
bool FileExistsErrMsg(const char* _fn, bool _errMsg=true);
bool SNM_DeleteFile(const char* _filename, bool _recycleBin);
bool SNM_CopyFile(const char* _destFn, const char* _srcFn);
bool BrowseResourcePath(const char* _title, const char* _dir, const char* _fileFilters, char* _fn, int _fnSize, bool _wantFullPath = false);
void GetShortResourcePath(const char* _resSubDir, const char* _fullFn, char* _shortFn, int _fnSize);
void GetFullResourcePath(const char* _resSubDir, const char* _shortFn, char* _fullFn, int _fnSize);
bool LoadChunk(const char* _fn, WDL_FastString* _chunk, bool _trim = true, int _maxlen = 0);
bool SaveChunk(const char* _fn, WDL_FastString* _chunk, bool _indent);
WDL_HeapBuf* LoadBin(const char* _fn);
bool SaveBin(const char* _fn, const WDL_HeapBuf* _hb);
bool TranscodeFileToFile64(const char* _outFn, const char* _inFn);
WDL_HeapBuf* TranscodeStr64ToHeapBuf(const char* _str64);
void GenerateFilename(const char* _dir, const char* _name, const char* _ext, char* _updatedFn, int _updatedSz);
void StringToExtensionConfig(WDL_FastString* _str, ProjectStateContext* _ctx);
void ExtensionConfigToString(WDL_FastString* _str, ProjectStateContext* _ctx);
void SaveIniSection(const char* _iniSectionName, WDL_FastString* _iniSection, const char* _iniFn);
void UpdatePrivateProfileSection(const char* _oldAppName, const char* _newAppName, const char* _iniFn, const char* _newIniFn = NULL);
void UpdatePrivateProfileString(const char* _appName, const char* _oldKey, const char* _newKey, const char* _iniFn, const char* _newIniFn = NULL);
void SNM_UpgradeIniFiles();
void ScanFiles(WDL_PtrList<WDL_String>* _files, const char* _initDir, const char* _ext, bool _subdirs);
int FindMarkerRegion(double _pos);
int FindMarkerRegionNum(double _pos);
void makeUnformatedConfigString(const char* _in, WDL_FastString* _out);
bool GetStringWithRN(const char* _bufSrc, char* _buf, int _bufSize);
void ShortenStringToFirstRN(char* _str);
void ReplaceStringFormat(char* _str, char _replaceCh);
bool GetSectionName(bool _alr, const char* _section, char* _sectionURL, int _sectionURLSize);
bool WaitForTrackMute(DWORD* _muteTime);
#ifdef _SNM_MISC
void LetREAPERBreathe(COMMAND_T*);
#endif
void WinWaitForEvent(DWORD _event, DWORD _timeOut=500, DWORD _minReTrigger=500);
void SimulateMouseClick(COMMAND_T*);
void DumpWikiActionList2(COMMAND_T*);
void DumpActionList(COMMAND_T*);
#ifdef _WIN32
void LoadThemeSlot(int _slotType, const char* _title, int _slot);
void LoadThemeSlot(COMMAND_T* _ct);
#endif
#ifdef _SNM_MISC
void ShowTakeEnvPadreTest(COMMAND_T*);
void dumpWikiActionList(COMMAND_T*);
void OpenStuff(COMMAND_T*);
void TestWDLString(COMMAND_T*);
#endif

// *** SnM_NotesHelpView.cpp ***
extern SWSProjConfig<WDL_PtrList_DeleteOnDestroy<SNM_TrackNotes> > g_pTracksNotes;
void SetActionHelpFilename(COMMAND_T*);
int NotesHelpViewInit();
void NotesHelpViewExit();
void OpenNotesHelpView(COMMAND_T*);
bool IsNotesHelpViewDisplayed(COMMAND_T*);
void ToggleNotesHelpLock(COMMAND_T*);
bool IsNotesHelpLocked(COMMAND_T*);

// *** SnM_Project.cpp ***
void SelectProject(MIDI_COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd);
void loadOrSelectProjectSlot(int _slotType, const char* _title, int _slot, bool _newTab);
bool autoSaveProjectSlot(int _slotType, const char* _dirPath, char* _fn, int _fnSize, bool _saveCurPrj);
void loadOrSelectProjectSlot(COMMAND_T*);
void loadOrSelectProjectTabSlot(COMMAND_T*);
bool isProjectLoaderConfValid();
void projectLoaderConf(COMMAND_T*);
void loadOrSelectNextPreviousProject(COMMAND_T*);
void openProjectPathInExplorerFinder(COMMAND_T*);

// *** SnM_ResourceView.cpp ***
int ResourceViewInit();
void ResourceViewExit();
void OpenResourceView(COMMAND_T*);
bool IsResourceViewDisplayed(COMMAND_T*);
void ResourceViewClearSlotPrompt(COMMAND_T*);
void ResourceViewAutoSaveFXChain(COMMAND_T*);
void ResourceViewAutoSaveTrTemplate(COMMAND_T*);
void ResourceViewAutoSave(COMMAND_T*);

// *** SnM_Sends.cpp ***
bool cueTrack(const char* _busName, int _type, const char* _undoMsg, bool _showRouting = true, int _soloDefeat = 1, char* _trTemplatePath = NULL, bool _sendToMaster = false, int* _hwOuts = NULL);
void cueTrack(COMMAND_T*);
void readCueBusIniFile(char* _busName, int* _reaType, bool* _trTemplate, char* _trTemplatePath, bool* _showRouting, int* _soloDefeat, bool* _sendToMaster, int* _hwOuts);
void saveCueBusIniFile(const char* _busName, int _type, bool _trTemplate, const char* _trTemplatePath, bool _showRouting, int _soloDefeat, bool _sendToMaster, int* _hwOuts);
void copyWithIOs(COMMAND_T*);
void cutWithIOs(COMMAND_T*);
void pasteWithIOs(COMMAND_T*);
void copyRoutings(COMMAND_T*);
void cutRoutings(COMMAND_T*);
void pasteRoutings(COMMAND_T*);
void copySends(COMMAND_T*);
void cutSends(COMMAND_T*);
void pasteSends(COMMAND_T*);
void copyReceives(COMMAND_T*);
void cutReceives(COMMAND_T*);
void pasteReceives(COMMAND_T*);
void removeSends(COMMAND_T*);
void removeReceives(COMMAND_T*);
void removeRouting(COMMAND_T*);
void muteReceives(MediaTrack* _source, MediaTrack* _dest, bool _mute);

// *** SnM_Track.cpp ***
void copyCutTrackGrouping(COMMAND_T*);
void pasteTrackGrouping(COMMAND_T*);
void removeTrackGrouping(COMMAND_T*);
void SetTrackGroup(COMMAND_T*);
void SetTrackToFirstUnusedGroup(COMMAND_T*);
void saveTracksFolderStates(COMMAND_T*);
void restoreTracksFolderStates(COMMAND_T*);
void setTracksFolderState(COMMAND_T*);
int trackEnvelopesCount();
const char* trackEnvelope(int _i);
bool trackEnvelopesLookup(const char* _str);
void toggleArmTrackEnv(COMMAND_T*);
void toggleWriteEnvExists(COMMAND_T*);
bool writeEnvExists(COMMAND_T*);
int SNM_GetNumTracks(ReaProject* _proj);
MediaTrack* SNM_GetTrack(ReaProject* _proj, int _idx);
int SNM_GetTrackId(ReaProject* _proj, MediaTrack* _tr);
int SNM_CountSelectedTracks(ReaProject* _proj, bool _master);
MediaTrack* SNM_GetSelectedTrack(ReaProject* _proj, int _idx, bool _master);
void SNM_GetSelectedTracks(ReaProject* _proj, WDL_PtrList<MediaTrack>* _trs);
void SNM_SetSelectedTracks(ReaProject* _proj, WDL_PtrList<MediaTrack>* _trs);
void SNM_ClearSelectedTracks(ReaProject* _proj);
bool makeSingleTrackTemplateChunk(WDL_FastString* _inRawChunk, WDL_FastString* _out, bool _delItems, bool _delEnvs);
bool applyTrackTemplate(MediaTrack* _tr, WDL_FastString* _tmpltChunk, bool _rawChunk, SNM_ChunkParserPatcher* _p = NULL, bool _itemsFromTmplt = false, bool _envsFromTmplt = false);
void applyOrImportTrackSlot(int _slotType, const char* _title, int _slot, bool _import, bool _itemsFromTmplt, bool _envsFromTmplt);
void replaceOrPasteItemsFromTrackSlot(int _slotType, const char* _title, int _slot, bool _paste);
void loadSetTrackTemplate(COMMAND_T*);
void loadImportTrackTemplate(COMMAND_T*);
bool autoSaveTrackSlots(int _slotType, const char* _dirPath, char* _fn, int _fnSize, bool _delItems, bool _delEnvs);
void setMIDIInputChannel(COMMAND_T*);
void remapMIDIInputChannel(COMMAND_T*);
bool SNM_PlayTrackPreview(MediaTrack* _tr, PCM_source* _src, bool _loop);
bool SNM_PlayTrackPreview(MediaTrack* _tr, const char* _fn, bool _loop);
bool SNM_TogglePlaySelTrackPreviews(const char* _fn, bool _loop);
void StopTrackPreviewsRun();
void StopSelTrackPreview(COMMAND_T*);
void CC123Tracks(WDL_PtrList<void>* _trs);
void CC123SelTracks(COMMAND_T*);

// *** SnM_Windows.cpp ***
bool SNM_IsActiveWindow(HWND _h);
bool IsChildOf(HWND _hChild, const char* _title, int _nComp = -1);
HWND GetReaWindowByTitle(const char* _title, int _nComp = -1);
HWND GetActionListBox(char* _currentSection = NULL, int _sectionMaxSize = 0);
int GetSelectedAction(char* _section, int _secSize, int* _cmdId, char* _id, int _idSize, char* _desc = NULL, int _descSize = -1);
void showFXChain(COMMAND_T*);
void hideFXChain(COMMAND_T*);
void toggleFXChain(COMMAND_T*);
bool isToggleFXChain(COMMAND_T*);
void showAllFXChainsWindows(COMMAND_T*);
void closeAllFXChainsWindows(COMMAND_T*);
void toggleAllFXChainsWindows(COMMAND_T*);
void floatUnfloatFXs(MediaTrack* _tr, bool _all, int _showFlag, int _fx, bool _selTracks);
void floatFX(COMMAND_T*);
void unfloatFX(COMMAND_T*);
void toggleFloatFX(COMMAND_T*);
void showAllFXWindows(COMMAND_T*);
void closeAllFXWindows(COMMAND_T*);
void closeAllFXWindowsExceptFocused(COMMAND_T*);
void toggleAllFXWindows(COMMAND_T*);
int getFocusedFX(MediaTrack* _tr, int _dir, int* _firstFound = NULL);
#ifdef _SNM_MISC
void cycleFocusFXWndSelTracks(COMMAND_T*);
void cycleFocusFXWndAllTracks(COMMAND_T*);
#endif
void cycleFloatFXWndSelTracks(COMMAND_T*);
void cycleFocusFXMainWndAllTracks(COMMAND_T*);
void cycleFocusFXMainWndSelTracks(COMMAND_T*);
void cycleFocusWnd(COMMAND_T*);
void cycleFocusHideOthersWnd(COMMAND_T*);
void focusMainWindow(COMMAND_T*);
void focusMainWindowCloseOthers(COMMAND_T*);
void ShowThemeHelper(COMMAND_T*);
void GetVisibleTCPTracks(WDL_PtrList<void>* _trList);


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

