/******************************************************************************
/ SnM_Actions.h
/
/ Copyright (c) 2009-2010 Tim Payne (SWS), JF Bédague 
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
// Definitions
///////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#define SNM_FORMATED_INI_FILE "%s\\S&M.ini"
#define SNM_OLD_FORMATED_INI_FILE "%s\\Plugins\\S&M.ini"
#define SNM_ACTION_HELP_INI_FILE "%s\\S&M_Action_help_en.ini"
#else
#define SNM_FORMATED_INI_FILE "%s/S&M.ini"
#define SNM_OLD_FORMATED_INI_FILE "%s/Plugins/S&M.ini"
#define SNM_ACTION_HELP_INI_FILE "%s/S&M_Action_help_en.ini"
#endif

#define MAX_ACTION_COUNT	0xFFFF
#define MAX_TRACK_GROUPS	32 
#define SNM_MAX_HW_OUTS		8
#define SNM_MAX_TAKES		128
#define SNM_MAX_FX			128
#define MAX_INI_SECTION		0xFFFF // definitive limit for WritePrivateProfileSection

#define SNM_CMD_SHORTNAME(_ct) (_ct->accel.desc + 9) // +9 to skip "SWS/S&M: "

// Scheduled job ids
#define SNM_SCHEDJOB_LIVECFG_TLCHANGE	8
#define SNM_SCHEDJOB_NOTEHLP_TLCHANGE	9
#define SNM_SCHEDJOB_LIVECFG_SELPRJ		10

#define SNM_SCHEDJOB_DEFAULT_DELAY		250

enum
{
  SNM_SLOT_TYPE_FX_CHAINS=0,
  SNM_SLOT_TYPE_TR_TEMPLATES,
  SNM_SLOT_TYPE_COUNT
};


///////////////////////////////////////////////////////////////////////////////
// Global types & classes
///////////////////////////////////////////////////////////////////////////////

typedef struct MIDI_COMMAND_T
{
	gaccel_register_t accel;
	const char* id;
	void (*doCommand)(MIDI_COMMAND_T*,int,int,int,HWND);
	const char* menuText;
	INT_PTR user;
	bool (*getEnabled)(MIDI_COMMAND_T*);
} MIDI_COMMAND_T;


class SNM_TrackNotes 
{
public:
	SNM_TrackNotes(MediaTrack* _tr, const char* _notes) 
		: m_tr(_tr) {m_notes.Set(_notes ? _notes : "");}
	MediaTrack* m_tr; WDL_String m_notes;
};


class SNM_FXSummary 
{
public:
	SNM_FXSummary(const char* _type, const char* _realName)
		: m_type(_type),m_realName(_realName){}
	virtual ~SNM_FXSummary() {}
	WDL_String m_type;WDL_String m_realName;
};


class SNM_ScheduledJob 
{
public:
	SNM_ScheduledJob(int _id, int _approxDelayMs)
	{
		m_id = _id;
		// 1 tick = 27ms or so (average monitored)
		m_tick = (int)floor((_approxDelayMs/27) + 0.5);
	}
	virtual ~SNM_ScheduledJob() {}
	virtual void Perform() {}
	int m_id, m_tick;
};

class SNM_SndRcv 
{
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


// Custom WDL UIs (SnM_Dlg.cpp)

class SNM_VirtualComboBox : public WDL_VirtualComboBox
{
  public:
	SNM_VirtualComboBox() : WDL_VirtualComboBox() {}
	~SNM_VirtualComboBox() {}
    void OnPaint(LICE_IBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect);
};


///////////////////////////////////////////////////////////////////////////////
// Global vars
///////////////////////////////////////////////////////////////////////////////

extern WDL_String g_SNMiniFilename;	// SnM_Actions.cpp
extern SWSProjConfig<WDL_PtrList_DeleteOnDestroy<SNM_TrackNotes> > g_pTracksNotes;	// SnM_NotesHelpView.cpp


///////////////////////////////////////////////////////////////////////////////
// Includes
///////////////////////////////////////////////////////////////////////////////

// *** SnM_Actions.cpp ***
void fakeToggleAction(COMMAND_T* _ct);
bool fakeIsToggledAction(COMMAND_T* _ct);
int SnMInit(reaper_plugin_info_t* _rec);
void SnMExit();
void AddOrReplaceScheduledJob(SNM_ScheduledJob* _job);
void DeleteScheduledJob(int _id);
void SnMCSurfRun();
void SnMCSurfSetTrackTitle();
void SnMCSurfSetTrackListChange();

// *** SnM_FX.cpp ***
bool patchSelTracksFXState(int _mode, int _token, int _fx, const char* _value, const char * _undoMsg);
void toggleFXOfflineSelectedTracks(COMMAND_T* _ct);
bool isFXOfflineSelectedTracks(COMMAND_T* _ct);
void toggleFXBypassSelectedTracks(COMMAND_T* _ct);
bool isFXBypassedSelectedTracks(COMMAND_T * _ct);
void toggleExceptFXOfflineSelectedTracks(COMMAND_T* _ct);
void toggleExceptFXBypassSelectedTracks(COMMAND_T* _ct);
void toggleAllFXsOfflineSelectedTracks(COMMAND_T* _ct);
void toggleAllFXsBypassSelectedTracks(COMMAND_T* _ct);
void setFXOfflineSelectedTracks(COMMAND_T* _ct); 
void setFXBypassSelectedTracks(COMMAND_T* _ct);
void setFXOnlineSelectedTracks(COMMAND_T* _ct);
void setFXUnbypassSelectedTracks(COMMAND_T* _ct);
void setAllFXsBypassSelectedTracks(COMMAND_T* _ct); // ..related online/offline actions natively implemented
int selectFX(MediaTrack* _tr, int _fx);
int getSelectedFX(MediaTrack* _tr);
void selectFX(COMMAND_T* _ct);
int getPresetNames(const char* _fxType, const char* _fxName, WDL_PtrList<WDL_String>* _names);
void UpdatePresetConf(int _fx, int _preset, WDL_String* _presetConf);
int GetSelPresetFromConf(int _fx, WDL_String* _curPresetConf, int _presetCount=0xFFFF);
void RenderPresetConf(WDL_String* _presetConf, WDL_String* _renderConf);

// *** SnM_FXChain.cpp ***
void loadSetTakeFXChain(COMMAND_T* _ct);
void loadSetAllTakesFXChain(COMMAND_T* _ct);
void loadSetPasteTakeFXChain(const char* _title, int _slot, bool _activeOnly, bool _set, bool _errMsg=false);
void copyTakeFXChain(COMMAND_T* _ct);
void cutTakeFXChain(COMMAND_T* _ct);
void pasteTakeFXChain(COMMAND_T* _ct);
void setTakeFXChain(COMMAND_T* _ct);
void pasteAllTakesFXChain(COMMAND_T* _ct);
void setAllTakesFXChain(COMMAND_T* _ct);
void clearActiveTakeFXChain(COMMAND_T* _ct);
void clearAllTakesFXChain(COMMAND_T* _ct);
void pasteTakeFXChain(const char* _title, int _slot, bool _activeOnly);
void setTakeFXChain(const char* _title, int _slot, bool _activeOnly, bool _clear);
void loadSetPasteTrackFXChain(const char* _title, int _slot, bool _set, bool _errMsg=false);
void pasteTrackFXChain(const char* _title, int _slot);
void setTrackFXChain(const char* _title, int _slot, bool _clear);
void loadSetTrackFXChain(COMMAND_T* _ct);
void clearTrackFXChain(COMMAND_T* _ct);
void copyTrackFXChain(COMMAND_T* _ct);
void cutTrackFXChain(COMMAND_T* _ct);
void pasteTrackFXChain(COMMAND_T* _ct);
void setTrackFXChain(COMMAND_T* _ct);
void makeChunkTakeFX(WDL_String* _outTakeFX, WDL_String* _inRfxChain);
void clearFXChainSlotPrompt(COMMAND_T* _ct);
void copySlotToClipBoard(int _slot);
void readSlotIniFile(const char* _key, int _slot, char* _path, int _pathSize, char* _desc, int _descSize);
void saveSlotIniFile(const char* _key, int _slot, const char* _path, const char* _desc);

// *** SnM_Windows.cpp ***
bool toggleShowHideWin(const char * _title);
bool closeWin(const char * _title);
void closeOrToggleAllWindows(bool _routing, bool _env, bool _toggle);
void closeAllRoutingWindows(COMMAND_T * _ct);
void closeAllEnvWindows(COMMAND_T * _ct);
void toggleAllRoutingWindows(COMMAND_T * _ct);
void toggleAllEnvWindows(COMMAND_T * _ct);
void showFXChain(COMMAND_T* _ct);
void hideFXChain(COMMAND_T* _ct);
void toggleFXChain(COMMAND_T* _ct);
bool isToggleFXChain(COMMAND_T * _ct);
void showAllFXChainsWindows(COMMAND_T* _ct);
void closeAllFXChainsWindows(COMMAND_T * _ct);
void toggleAllFXChainsWindows(COMMAND_T * _ct);
void toggleFloatFX(MediaTrack* _tr, int _fx);
void floatUnfloatFXs(MediaTrack* _tr, bool _all, int _showFlag, int _fx, bool _selTracks);
void floatUnfloatFXs(bool _all, int _showFlag, int _fx, bool _selTracks);
void floatFX(COMMAND_T* _ct);
void unfloatFX(COMMAND_T* _ct);
void toggleFloatFX(COMMAND_T* _ct);
void showAllFXWindows(COMMAND_T * _ct);
void closeAllFXWindows(COMMAND_T * _ct);
void closeAllFXWindowsExceptFocused(COMMAND_T * _ct);
void toggleAllFXWindows(COMMAND_T * _ct);
int getFocusedFX(MediaTrack* _tr, int _dir, int* _firstFound = NULL);
bool cycleFocusFXWnd(int _dir, bool _selectedTracks);
void cycleFocusFXWndSelTracks(COMMAND_T * _ct);
void cycleFocusFXWndAllTracks(COMMAND_T * _ct);
void cycleFloatFXWndSelTracks(COMMAND_T * _ct);
void cycleFocusFXAndMainWndAllTracks(COMMAND_T * _ct);
void cycleFocusFXMainWndSelTracks(COMMAND_T * _ct);
void cycleFocusWnd(COMMAND_T * _ct);
void setMainWindowActive(COMMAND_T* _ct);

// *** SnM_Sends.cpp ***
bool cueTrack(const char* _busName, int _type, const char* _undoMsg, bool _showRouting = true, int _soloDefeat = 1, char* _trTemplatePath = NULL, bool _sendToMaster = false, int* _hwOuts = NULL);
void cueTrackPrompt(COMMAND_T* _ct);
void cueTrack(COMMAND_T* _ct);
void copyWithIOs(COMMAND_T* _ct);
void cutWithIOs(COMMAND_T* _ct);
void pasteWithIOs(COMMAND_T* _ct);
void copyRoutings(COMMAND_T* _ct);
void cutRoutings(COMMAND_T* _ct);
void pasteRoutings(COMMAND_T* _ct);
void copySends(COMMAND_T* _ct);
void cutSends(COMMAND_T* _ct);
void pasteSends(COMMAND_T* _ct);
void copyReceives(COMMAND_T* _ct);
void cutReceives(COMMAND_T* _ct);
void pasteReceives(COMMAND_T* _ct);
int GetComboSendIdxType(int _reaType) ;
const char* GetSendTypeStr(int _type);
void removeSends(COMMAND_T* _ct);
void removeReceives(COMMAND_T* _ct);
void removeRouting(COMMAND_T* _ct);
void readCueBusIniFile(char* _busName, int* _reaType, bool* _trTemplate, char* _trTemplatePath, bool* _showRouting, int* _soloDefeat, bool* _sendToMaster, int* _hwOuts);
void saveCueBusIniFile(char* _busName, int _type, bool _trTemplate, char* _trTemplatePath, bool _showRouting, int _soloDefeat, bool _sendToMaster, int* _hwOuts);

// *** SnM_Item.cpp ***
void splitMidiAudio(COMMAND_T* _ct);
void smartSplitMidiAudio(COMMAND_T* _ct);
void splitSelectedItems(COMMAND_T* _ct);
void goferSplitSelectedItems(COMMAND_T* _ct);
bool isEmptyMidi(MediaItem_Take* _take);
void setEmptyTakeChunk(WDL_String* _chunk, int _recPass = -1, int _color = -1);
bool addEmptyTake(MediaItem* _item);
int buildLanes(const char* _undoTitle, int _mode);
bool removeEmptyTakes(MediaTrack* _tr, bool _empty, bool _midiEmpty, bool _trSel, bool _itemSel);
bool removeEmptyTakes(const char* _undoTitle, bool _empty, bool _midiEmpty, bool _trSel = false, bool _itemSel = true);
void clearTake(COMMAND_T* _ct);
void moveTakes(COMMAND_T* _ct);
void moveActiveTake(COMMAND_T* _ct);
void activateLaneFromSelItem(COMMAND_T* _ct);
void activateLaneUnderMouse(COMMAND_T* _ct);
void buildLanes(COMMAND_T* _ct);
void removeEmptyTakes(COMMAND_T* _ct);
void removeEmptyMidiTakes(COMMAND_T* _ct);
void removeAllEmptyTakes(COMMAND_T* _ct);
void deleteTakeAndMedia(COMMAND_T* _ct);
int getTakeIndex(MediaItem* _item, MediaItem_Take* _take);
void showHideTakeVolEnvelope(COMMAND_T* _ct); 
void showHideTakePanEnvelope(COMMAND_T* _ct);
void showHideTakeMuteEnvelope(COMMAND_T* _ct);
bool ShowTakeEnvVol(MediaItem_Take* _take);
bool ShowTakeEnvPan(MediaItem_Take* _take);
bool ShowTakeEnvMute(MediaItem_Take* _take);
void saveItemTakeTemplate(COMMAND_T* _ct);

// *** SnM_Track.cpp ***
#ifdef _SNM_TRACK_GROUP_EX
int addSoloToGroup(MediaTrack * _tr, int _group, bool _master, SNM_ChunkParserPatcher* _cpp);
#endif
void saveTracksFolderStates(COMMAND_T* _ct);
void restoreTracksFolderStates(COMMAND_T* _ct);
void setTracksFolderState(COMMAND_T* _ct);
void toggleArmTrackEnv(COMMAND_T* _ct);
int CountSelectedTracksWithMaster(ReaProject* _proj);
MediaTrack* GetSelectedTrackWithMaster(ReaProject* _proj, int _idx);
MediaTrack* GetFirstSelectedTrackWithMaster(ReaProject* _proj);
void loadSetOrAddTrackTemplate(const char* _title, bool _add, int _slot, bool _errMsg);
void loadSetTrackTemplate(COMMAND_T* _ct);
void loadAddTrackTemplate(COMMAND_T* _ct);
void clearTrackTemplateSlotPrompt(COMMAND_T* _ct);

// *** SNM_ResourceView.cpp ***
int ResourceViewInit();
void ResourceViewExit();
void OpenResourceView(COMMAND_T*);
bool IsResourceViewEnabled(COMMAND_T*);

// *** SnM_NotesHelpView.cpp ***
int NotesHelpViewInit();
void NotesHelpViewExit();
void OpenNotesHelpView(COMMAND_T*);
void SetActionHelpFilename(COMMAND_T*);
bool IsNotesHelpViewEnabled(COMMAND_T*);
void SwitchNotesHelpType(COMMAND_T* _ct);
void ToggleNotesHelpLock(COMMAND_T*);
bool IsNotesHelpLocked(COMMAND_T*);

// *** SnM_FindView.cpp ***
int FindViewInit();
void FindViewExit();
void OpenFindView(COMMAND_T*);
bool IsFindViewEnabled(COMMAND_T*);

// *** SnM_MidiLiveView.cpp ***
int MidiLiveViewInit();
void MidiLiveViewExit();
void OpenMidiLiveView(COMMAND_T*);
bool IsMidiLiveViewEnabled(COMMAND_T*);
void ApplyLiveConfig(MIDI_COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd);
void SelectProject(MIDI_COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd);

// *** SnM_Dlg.cpp ***
void fillHWoutDropDown(HWND _hwnd, int _idc);
WDL_DLGRET CueBusDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
WDL_DLGRET WaitDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

// *** SnM_ME.cpp ***
void MECreateCCLane(COMMAND_T* _ct);
void MEHideCCLanes(COMMAND_T* _ct);
void MESetCCLanes(COMMAND_T* _ct);
void MESaveCCLanes(COMMAND_T* _ct);

// *** SnM_Misc.cpp ***
void letREAPERBreathe(COMMAND_T* _ct);
void winWaitForEvent(DWORD _event, DWORD _timeOut=500, DWORD _minReTrigger=500);
void simulateMouseClick(COMMAND_T* _ct);
void dumpWikiActions2(COMMAND_T* _ct);
void SNM_ShowConsoleMsg(const char* _msg, const char* _title="", bool _clear=true); 
bool SNM_DeleteFile(const char* _filename);
HWND SearchWindow(const char* _title);
HWND GetActionListBox(char* _currentSection = NULL, int _sectionMaxSize = 0);
bool GetALRStartOfURL(const char* _section, char* _sectionURL, int _sectionURLMaxSize);
bool BrowseResourcePath(const char* _title, const char* _dir, const char* _fileFilters, char* _filename, int _maxFilename, bool _fullPath = false);
void GetShortResourcePath(const char* _resSubDir, const char* _longFilename, char* _filename, int _maxFilename);
void GetFullResourcePath(const char* _resSubDir, const char* _shortFilename, char* _filename, int _maxFilename);
bool LoadChunk(const char* _filename, WDL_String* _chunk);
void StringToExtensionConfig(char* _str, ProjectStateContext* _ctx);
void ExtensionConfigToString(WDL_String* _str, ProjectStateContext* _ctx);
#ifdef _SNM_MISC
void ShowTakeEnvPadreTest(COMMAND_T* _ct);
void setPresetTest(COMMAND_T* _ct);
void openStuff(COMMAND_T* _ct);
#endif


///////////////////////////////////////////////////////////////////////////////
// Inline funcs
///////////////////////////////////////////////////////////////////////////////

static void freecharptr(char *p) { free(p); }

