/******************************************************************************
/ SnM_Actions.h
/ JFB TODO? now, a better name would be "SnM.h"
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

//#define _SNM_MISC
//#define _SNM_TRACK_GROUP_EX
#define _SNM_PRESETS


///////////////////////////////////////////////////////////////////////////////
// Definitions, enums
///////////////////////////////////////////////////////////////////////////////

enum {
  SNM_ITEM_SEL_LEFT=0,
  SNM_ITEM_SEL_RIGHT,
  SNM_ITEM_SEL_UP,
  SNM_ITEM_SEL_DOWN
};

#define SNM_CMD_SHORTNAME(_ct) (_ct->accel.desc + 9) // +9 to skip "SWS/S&M: "

#ifdef _WIN32
#define SNM_FORMATED_INI_FILE		"%s\\S&M.ini"
#define SNM_OLD_FORMATED_INI_FILE	"%s\\Plugins\\S&M.ini"
#define SNM_ACTION_HELP_INI_FILE	"%s\\S&M_Action_help_en.ini"
#else
#define SNM_FORMATED_INI_FILE		"%s/S&M.ini"
#define SNM_OLD_FORMATED_INI_FILE	"%s/Plugins/S&M.ini"
#define SNM_ACTION_HELP_INI_FILE	"%s/S&M_Action_help_en.ini"
#endif
#define SNM_MAX_ACTION_COUNT		0xFFFF
#define SNM_MAX_TRACK_GROUPS		32 
#define SNM_MAX_HW_OUTS				8
#define SNM_MAX_TAKES				128
#define SNM_MAX_FX					128
#define SNM_MAX_INI_SECTION			0xFFFF // definitive limit for WritePrivateProfileSection
#define SNM_LET_BREATHE_MS			10
#define SNM_3D_COLORS_DELTA			25
#define SNM_CSURF_RUN_TICK_MS		27     // 1 tick = 27ms or so (average monitored)
#define SNM_CSURF_RUN_POLL_MS		1000

// Scheduled job *RESERVED* ids
// note: [0..7] are reserved for Live Configs MIDI CC actions
#define SNM_SCHEDJOB_LIVECFG_TLCHANGE	8
#define SNM_SCHEDJOB_NOTEHLP_TLCHANGE	9
#define SNM_SCHEDJOB_SEL_PRJ			10
#define SNM_SCHEDJOB_TRIG_PRESET		11

#define SNM_SCHEDJOB_DEFAULT_DELAY		250


///////////////////////////////////////////////////////////////////////////////
// Global types & classes
///////////////////////////////////////////////////////////////////////////////

typedef struct MIDI_COMMAND_T {
	gaccel_register_t accel;
	const char* id;
	void (*doCommand)(MIDI_COMMAND_T*,int,int,int,HWND);
	const char* menuText;
	INT_PTR user;
	bool (*getEnabled)(MIDI_COMMAND_T*);
} MIDI_COMMAND_T;

class SNM_TrackNotes {
public:
	SNM_TrackNotes(MediaTrack* _tr, const char* _notes) 
		: m_tr(_tr) {m_notes.Set(_notes ? _notes : "");}
	MediaTrack* m_tr; WDL_String m_notes;
};

class SNM_FXSummary {
public:
	SNM_FXSummary(const char* _type, const char* _realName)
		: m_type(_type),m_realName(_realName){}
	virtual ~SNM_FXSummary() {}
	WDL_String m_type;WDL_String m_realName;
};

class SNM_ScheduledJob {
public:
	SNM_ScheduledJob(int _id, int _approxDelayMs) : m_id(_id), m_tick((int)floor((_approxDelayMs/SNM_CSURF_RUN_TICK_MS) + 0.5)) {}
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

// SNM_FastListView (defined in SnM_Dlg.cpp)
class SNM_FastListView : public SWS_ListView {
public:
	SNM_FastListView(HWND _hwndList, HWND _hwndEdit, int _iCols, SWS_LVColumn* _pCols, const char* _cINIKey, bool _bTooltips)
	: SWS_ListView(_hwndList, _hwndEdit, _iCols, _pCols, _cINIKey, _bTooltips) {}
public:
	~SNM_FastListView() {}
	virtual void Update();
};

class SNM_TrackInt {
public:
	SNM_TrackInt(MediaTrack* _tr, int _i) : m_tr(_tr), m_int(_i) {}
	~SNM_TrackInt() {}
	MediaTrack* m_tr;
	int m_int;
};


///////////////////////////////////////////////////////////////////////////////
// Includes & externs
///////////////////////////////////////////////////////////////////////////////

// *** SnM_Actions.cpp ***
extern WDL_String g_SNMiniFilename;	
extern LICE_IBitmap* g_snmLogo;

void EnableToolbarsAutoRefesh(COMMAND_T*);
bool IsToolbarsAutoRefeshEnabled(COMMAND_T*);
void RefreshToolbars();

void fakeToggleAction(COMMAND_T*);
bool fakeIsToggledAction(COMMAND_T*);
int SnMInit(reaper_plugin_info_t* _rec);
void SnMExit();
void AddOrReplaceScheduledJob(SNM_ScheduledJob* _job);
void DeleteScheduledJob(int _id);
void SnMCSurfRun();
void SnMCSurfSetTrackTitle();
void SnMCSurfSetTrackListChange();

// *** SnM_fx.cpp ***
bool patchSelTracksFXState(int _mode, int _token, int _fx, const char* _value, const char * _undoMsg);
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
void setAllFXsBypassSelectedTracks(COMMAND_T*); // ..related online/offline actions natively implemented
int selectFX(MediaTrack* _tr, int _fx);
int getSelectedFX(MediaTrack* _tr);
void selectFX(COMMAND_T*);
int getPresetNames(const char* _fxType, const char* _fxName, WDL_PtrList<WDL_String>* _names);
void UpdatePresetConf(int _fx, int _preset, WDL_String* _presetConf);
int GetPresetFromConf(int _fx, WDL_String* _presetConf, int _presetCount=0xFFFF);
void RenderPresetConf(WDL_String* _presetConf, WDL_String* _renderConf);

int triggerFXPreset(MediaTrack* _tr, int _fxId, int _presetId, int _dir = 0, bool _userPreset = false);
void triggerFXPreset(int _fxId, int _presetId, int _dir=0);
void triggerNextPreset(COMMAND_T*);
void triggerPreviousPreset(COMMAND_T*);
bool triggerFXUserPreset(MediaTrack* _tr, WDL_String* _presetConf);
void TriggerFXPreset(MIDI_COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd);

void moveFX(COMMAND_T*);

// *** SnM_FXChain.cpp ***
void makeChunkTakeFX(WDL_String* _outTakeFX, WDL_String* _inRfxChain);
int copyTakeFXChain(WDL_String* _fxChain, int _startSelItem=0);
void applyTakesFXChainSlot(const char* _title, int _slot, bool _activeOnly, bool _set, bool _errMsg);
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
void pasteTakeFXChain(const char* _title, WDL_String* _chain, bool _activeOnly);
void setTakeFXChain(const char* _title, WDL_String* _chain, bool _activeOnly);
bool autoSaveItemFXChainSlots(int _slot, const char* _dirPath, char* _fn, int _fnMaxSize);

void applyTracksFXChainSlot(const char* _title, int _slot, bool _set, bool _inputFX, bool _errMsg);
void pasteTrackFXChain(const char* _title, WDL_String* _chain, bool _inputFX);
void setTrackFXChain(const char* _title, WDL_String* _chain, bool _inputFX);
int copyTrackFXChain(WDL_String* _fxChain, bool _inputFX, int _startTr=0);
bool autoSaveTrackFXChainSlots(int _slot, bool _inputFX, const char* _dirPath, char* _fn, int _fnMaxSize);
void loadSetTrackFXChain(COMMAND_T*);
void loadPasteTrackFXChain(COMMAND_T*);
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
void readSlotIniFile(const char* _key, int _slot, char* _path, int _pathSize, char* _desc, int _descSize);
void saveSlotIniFile(const char* _key, int _slot, const char* _path, const char* _desc);

void reassignLearntMIDICh(COMMAND_T*);

// *** SnM_Windows.cpp ***
bool IsChildOf(HWND _hChild, const char* _title, int _nComp = -1);
HWND GetReaWindowByTitle(const char* _title, int _nComp = -1);
HWND SearchWindow(const char* _title);
HWND GetActionListBox(char* _currentSection = NULL, int _sectionMaxSize = 0);
bool toggleShowHideWin(const char * _title);
bool closeWin(const char * _title);
void closeOrToggleAllWindows(bool _routing, bool _env, bool _toggle);
void closeAllRoutingWindows(COMMAND_T*);
void closeAllEnvWindows(COMMAND_T*);
void toggleAllRoutingWindows(COMMAND_T*);
void toggleAllEnvWindows(COMMAND_T*);
void showFXChain(COMMAND_T*);
void hideFXChain(COMMAND_T*);
void toggleFXChain(COMMAND_T*);
bool isToggleFXChain(COMMAND_T*);
void showAllFXChainsWindows(COMMAND_T*);
void closeAllFXChainsWindows(COMMAND_T*);
void toggleAllFXChainsWindows(COMMAND_T*);
void toggleFloatFX(MediaTrack* _tr, int _fx);
void floatUnfloatFXs(MediaTrack* _tr, bool _all, int _showFlag, int _fx, bool _selTracks);
void floatUnfloatFXs(bool _all, int _showFlag, int _fx, bool _selTracks);
void floatFX(COMMAND_T*);
void unfloatFX(COMMAND_T*);
void toggleFloatFX(COMMAND_T*);
void showAllFXWindows(COMMAND_T*);
void closeAllFXWindows(COMMAND_T*);
void closeAllFXWindowsExceptFocused(COMMAND_T*);
void toggleAllFXWindows(COMMAND_T*);
int getFocusedFX(MediaTrack* _tr, int _dir, int* _firstFound = NULL);
int getFirstFloatingFX(MediaTrack* _tr, int _dir);
bool cycleFocusFXWnd(int _dir, bool _selectedTracks);
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

// *** SnM_Sends.cpp ***
bool cueTrack(const char* _busName, int _type, const char* _undoMsg, bool _showRouting = true, int _soloDefeat = 1, char* _trTemplatePath = NULL, bool _sendToMaster = false, int* _hwOuts = NULL);
void cueTrack(COMMAND_T*);
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
int GetComboSendIdxType(int _reaType) ;
const char* GetSendTypeStr(int _type);
void removeSends(COMMAND_T*);
void removeReceives(COMMAND_T*);
void removeRouting(COMMAND_T*);
void readCueBusIniFile(char* _busName, int* _reaType, bool* _trTemplate, char* _trTemplatePath, bool* _showRouting, int* _soloDefeat, bool* _sendToMaster, int* _hwOuts);
void saveCueBusIniFile(char* _busName, int _type, bool _trTemplate, char* _trTemplatePath, bool _showRouting, int _soloDefeat, bool _sendToMaster, int* _hwOuts);

// *** SnM_Item.cpp ***
char* GetName(MediaItem* _item);
int getTakeIndex(MediaItem* _item, MediaItem_Take* _take);
bool deleteMediaItemIfNeeded(MediaItem* _item);
void splitMidiAudio(COMMAND_T*);
void smartSplitMidiAudio(COMMAND_T*);
#ifdef _SNM_MISC // Deprecated (v3.67)
void splitSelectedItems(COMMAND_T*);
#endif
void goferSplitSelectedItems(COMMAND_T*);
void copyCutTake(COMMAND_T*);
void pasteTake(COMMAND_T*);
bool isEmptyMidi(MediaItem_Take* _take);
void setEmptyTakeChunk(WDL_String* _chunk, int _recPass = -1, int _color = -1);
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
void panTakeEnvelope(COMMAND_T*);
void showHideTakeVolEnvelope(COMMAND_T*); 
void showHideTakePanEnvelope(COMMAND_T*);
void showHideTakeMuteEnvelope(COMMAND_T*);
void showHideTakePitchEnvelope(COMMAND_T*);
bool ShowTakeEnvVol(MediaItem_Take* _take);
bool ShowTakeEnvPan(MediaItem_Take* _take);
bool ShowTakeEnvMute(MediaItem_Take* _take);
bool ShowTakeEnvPitch(MediaItem_Take* _take);
void saveItemTakeTemplate(COMMAND_T*);
void setPan(COMMAND_T*);
void itemSelToolbarPoll();
void toggleItemSelExists(COMMAND_T*);
bool itemSelExists(COMMAND_T*);

// *** SnM_Track.cpp ***
#ifdef _SNM_TRACK_GROUP_EX
int addSoloToGroup(MediaTrack * _tr, int _group, bool _master, SNM_ChunkParserPatcher* _cpp);
#endif
void copyCutTrackGrouping(COMMAND_T*);
void pasteTrackGrouping(COMMAND_T*);
void saveTracksFolderStates(COMMAND_T*);
void restoreTracksFolderStates(COMMAND_T*);
void setTracksFolderState(COMMAND_T*);
void toggleArmTrackEnv(COMMAND_T*);

void toggleWriteEnvExists(COMMAND_T*);
bool writeEnvExists(COMMAND_T*);

int CountSelectedTracksWithMaster(ReaProject* _proj);
MediaTrack* GetSelectedTrackWithMaster(ReaProject* _proj, int _idx);
MediaTrack* GetFirstSelectedTrackWithMaster(ReaProject* _proj);
void applyOrImportTrackTemplate(const char* _title, bool _import, int _slot, bool _errMsg);
void replaceOrPasteItemsFromsTrackTemplate(const char* _title, bool _paste, int _slot, bool _errMsg);
void loadSetTrackTemplate(COMMAND_T*);
void loadImportTrackTemplate(COMMAND_T*);
bool autoSaveTrackTemplateSlots(int _slot, bool _delItems, const char* _dirPath, char* _fn, int _fnMaxSize);

// *** SnM_ResourceView.cpp ***
int ResourceViewInit();
void ResourceViewExit();
void OpenResourceView(COMMAND_T*);
bool IsResourceViewDisplayed(COMMAND_T*);
void ClearSlotPrompt(COMMAND_T*);

// *** SnM_NotesHelpView.cpp ***
extern SWSProjConfig<WDL_PtrList_DeleteOnDestroy<SNM_TrackNotes> > g_pTracksNotes;

void SetActionHelpFilename(COMMAND_T*);
int NotesHelpViewInit();
void NotesHelpViewExit();
void OpenNotesHelpView(COMMAND_T*);
bool IsNotesHelpViewDisplayed(COMMAND_T*);
void SwitchNotesHelpType(COMMAND_T*);
void ToggleNotesHelpLock(COMMAND_T*);
bool IsNotesHelpLocked(COMMAND_T*);

// *** SnM_FindView.cpp ***
int FindViewInit();
void FindViewExit();
void OpenFindView(COMMAND_T*);
bool IsFindViewDisplayed(COMMAND_T*);
void FindNextPrev(COMMAND_T*);

// *** SnM_LiveConfigsView.cpp ***
int LiveConfigViewInit();
void LiveConfigViewExit();
void OpenLiveConfigView(COMMAND_T*);
bool IsLiveConfigViewDisplayed(COMMAND_T*);
void ApplyLiveConfig(MIDI_COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd);
void ToggleEnableLiveConfig(COMMAND_T*);
bool IsLiveConfigEnabled(COMMAND_T*);

// *** SnM_Dlg.cpp ***
LICE_CachedFont* SNM_GetThemeFont();
void fillHWoutDropDown(HWND _hwnd, int _idc);
WDL_DLGRET CueBusDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
void openCueBussWnd(COMMAND_T*);
bool isCueBussWndDisplayed(COMMAND_T*);
WDL_DLGRET WaitDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

// *** SnM_ME.cpp ***
void MECreateCCLane(COMMAND_T*);
void MEHideCCLanes(COMMAND_T*);
void MESetCCLanes(COMMAND_T*);
void MESaveCCLanes(COMMAND_T*);

// *** SnM_Misc.cpp ***
bool SNM_DeleteFile(const char* _filename);
bool BrowseResourcePath(const char* _title, const char* _dir, const char* _fileFilters, char* _filename, int _maxFilename, bool _wantFullPath = false);
void GetShortResourcePath(const char* _resSubDir, const char* _fullFn, char* _shortFn, int _maxFn);
void GetFullResourcePath(const char* _resSubDir, const char* _shortFn, char* _fullFn, int _maxFn);
bool LoadChunk(const char* _fn, WDL_String* _chunk);
bool SaveChunk(const char* _fn, WDL_String* _chunk);
void GenerateFilename(const char* _dir, const char* _name, const char* _ext, char* _updatedFn, int _updatedSz);
void StringToExtensionConfig(char* _str, ProjectStateContext* _ctx);
void ExtensionConfigToString(WDL_String* _str, ProjectStateContext* _ctx);
int SNM_MinMax(int _val, int _min, int _max);
bool GetSectionName(bool _alr, const char* _section, char* _sectionURL, int _sectionURLMaxSize);
void SNM_ShowConsoleMsg(const char* _msg, const char* _title="", bool _clear=true); 
void SNM_ShowConsoleDbg(bool _clear, const char* format, ...);
int PromptForMIDIChannel(const char* _title);

bool FileExistsErrMsg(const char* _fn, bool _errMsg=true);
void LetREAPERBreathe(COMMAND_T*);
void WinWaitForEvent(DWORD _event, DWORD _timeOut=500, DWORD _minReTrigger=500);
void SimulateMouseClick(COMMAND_T*);
void DumpWikiActionList2(COMMAND_T*);
void DumpActionList(COMMAND_T*);

void SelectProject(MIDI_COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd);

#ifdef _SNM_MISC
void ShowTakeEnvPadreTest(COMMAND_T*);
void dumpWikiActionList(COMMAND_T*);
void OpenStuff(COMMAND_T*);
#endif


///////////////////////////////////////////////////////////////////////////////
// Inline funcs
///////////////////////////////////////////////////////////////////////////////

static void freecharptr(char *p) { free(p); }

