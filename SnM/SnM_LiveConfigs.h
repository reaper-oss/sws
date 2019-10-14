/******************************************************************************
/ SnM_LiveConfigs.h
/
/ Copyright (c) 2010 and later Jeffos
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

//#pragma once

#ifndef _SNM_LIVECONFIGS_H_
#define _SNM_LIVECONFIGS_H_

#include "SnM_CSurf.h"
#include "SnM_VWnd.h"


class PresetMsg {
public:
	PresetMsg(int _fx=-1, const char* _preset="") : m_fx(_fx), m_preset(_preset) {}
	int m_fx;
	WDL_FastString m_preset;
};


class LiveConfigItem {
public:
	LiveConfigItem(int _cc, const char* _desc="", MediaTrack* _track=NULL, 
		const char* _trTemplate="", const char* _fxChain="", const char* _presets="", 
		const char* _onAction="", const char* _offAction="");
	LiveConfigItem(LiveConfigItem* _item);
	void Paste(LiveConfigItem* _item);
	bool IsDefault(bool _ignoreComment);
	void Clear(bool _trDataOnly = false);
	bool Equals(LiveConfigItem* _item, bool _ignoreComment);
	void GetInfo(WDL_FastString* _info);
	int m_cc;
	MediaTrack* m_track; //JFB!! TODO: GUID instead (to handle track deletion + undo, etc)
	WDL_FastString m_desc, m_trTemplate, m_fxChain, m_presets, m_onAction, m_offAction;
};


class LiveConfig {
public:
	LiveConfig();
	~LiveConfig() { m_ccConfs.Empty(true); delete m_osc; }

	bool IsDefault(bool _ignoreComment);
	int CountTrackConfigs(MediaTrack* _tr);

	// GUID_NULL means "no track" here not "the master track", see GuidToTrack()
	MediaTrack* GetInputTrack() { return !GuidsEqual(&m_inputTr, &GUID_NULL) ? GuidToTrack(&m_inputTr) : NULL; }
	int SetInputTrack(MediaTrack* _newInputTr, bool _updateSends);
	GUID* GetInputTrackGUID() { return &m_inputTr; }
	void SetInputTrackGUID(GUID* _g) { memcpy(&m_inputTr, _g, sizeof(GUID)); }

	void cfg_InitWorkingVars()
	{
		m_cfg_done=false;
		m_cfg_last_mute_time=0.0;
		m_cfg_tracks.Empty();
		m_cfg_tracks_states.Empty();
	}  
	void cfg_SaveMuteStateAndMuteIfNeeded(MediaTrack* _tr, bool _force = false);
	void cfg_Mute(MediaTrack* _tr);
	void cfg_WaitForMuteSendCC123(MediaTrack* inputTr);
	void cfg_RestoreMuteStates(MediaTrack* activeTr, MediaTrack* inputTr);

	WDL_PtrList<LiveConfigItem> m_ccConfs;
	int m_options; // &1=mute all but active track
	               // &2=offline all but active track
	               // &4=disarm all but active track
	               // &8=send all-notes-off on config switch
	               // &16=ignore switches to empty configs
	               // &32=auto update sends
	               // &64=scroll to track on list view click
	int m_ccDelay, m_fade, m_enable;
	int m_activeMidiVal, m_curMidiVal, m_preloadMidiVal, m_curPreloadMidiVal;
	SNM_OscCSurf* m_osc;

private:
	GUID m_inputTr; // GUID rather than MediaTrack* (to handle undo of track deletion, etc)

  // working vars: set while appying/preloading configs
  WDL_PtrList<void> m_cfg_tracks; // same nb of items as m_cfg_tracks_states
  WDL_PtrList<bool> m_cfg_tracks_states; // can contain NULL items (mute state unchanged), same nb of items as m_cfg_tracks
  double m_cfg_last_mute_time;
  bool m_cfg_done;
};


class LiveConfigView : public SWS_ListView {
public:
	LiveConfigView(HWND hwndList, HWND hwndEdit);
protected:
	void GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax);
	void SetItemText(SWS_ListItem* item, int iCol, const char* str);
	void GetItemList(SWS_ListItemList* pList);
	void OnItemSelChanged(SWS_ListItem* item, int iState);
	void OnItemDblClk(SWS_ListItem* item, int iCol);
};


// edition window
class LiveConfigsWnd : public SWS_DockWnd {
public:
	LiveConfigsWnd();
	virtual ~LiveConfigsWnd();
	void Update();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	void GetMinSize(int* _w, int* _h) { *_w=156; *_h=140; }
	void FillComboInputTrack();
	bool SelectByCCValue(int _configId, int _cc, bool _selectOnly = true);
protected:
	void OnInitDlg();
	void OnDestroy();
	INT_PTR OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void AddPresetMenu(HMENU _menu, MediaTrack* _tr, WDL_FastString* _curPresetConf);
	void AddLearnMenu(HMENU _menu, bool _subItems);
	void AddOptionsMenu(HMENU _menu, bool _subItems);
	HMENU OnContextMenu(int x, int y, bool* wantDefaultItems);
	int OnKey(MSG* msg, int iKeyState);
	void DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight = NULL);
	bool GetToolTipString(int _xpos, int _ypos, char* _bufOut, int _bufOutSz);
	bool Insert(int _dir = 1);

	WDL_PtrList_DeleteOnDestroy<PresetMsg> m_lastPresetMsg;
	WDL_VirtualComboBox m_cbConfig, m_cbInputTr;
	WDL_VirtualIconButton m_btnEnable;
	WDL_VirtualStaticText m_txtInputTr;
	SNM_ToolbarButton m_btnOptions, m_btnLearn, m_btnMonitor;
	SNM_Knob m_knobCC, m_knobFade;
	SNM_KnobCaption m_vwndCC, m_vwndFade;
};


void UpdateMonitoring(int _cfgId, int _whatFlags, int _commitFlags, int _flags = 3);


// monitoring window (several instances)
class LiveConfigMonitorWnd : public SWS_DockWnd {
public:
	LiveConfigMonitorWnd(int _cfgId = -1); // default constructor not used, just to compil SNM_DynWindowManager..
	virtual ~LiveConfigMonitorWnd();

	SNM_FiveMonitors* GetMonitors() { return &m_mons; }
protected:
	void OnInitDlg();
	void OnDestroy();
	INT_PTR OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight = NULL);
	int OnMouseDown(int xpos, int ypos);
	bool OnMouseUp(int xpos, int ypos);
	int m_cfgId;
	SNM_FiveMonitors m_mons;
	SNM_DynSizedText m_txtMon[5];
};


class LiveConfigJob : public MidiOscActionJob {
public:
	LiveConfigJob(int _jobId, int _approxMs, int _val, int _valhw, int _relmode, int _cfgId) 
		: MidiOscActionJob(_jobId,_approxMs,_val,_valhw,_relmode), m_cfgId(_cfgId) {}
protected:
	double GetMinValue() { return 0.0; }
	double GetMaxValue() { return SNM_LIVECFG_NB_ROWS-1; }
	int m_cfgId;
};

class ApplyLiveConfigJob : public LiveConfigJob {
public:
	ApplyLiveConfigJob(int _cfgId, int _approxMs, int _val, int _valhw, int _relmode) 
		: LiveConfigJob(SNM_SCHEDJOB_LIVECFG_APPLY+_cfgId,_approxMs,_val,_valhw,_relmode,_cfgId) {}
	virtual void Init(ScheduledJob* _replacedJob = NULL);
protected:
	void Perform();
	double GetCurrentValue();
};

class PreloadLiveConfigJob : public LiveConfigJob {
public:
	PreloadLiveConfigJob(int _cfgId, int _approxMs, int _val, int _valhw, int _relmode)
		: LiveConfigJob(SNM_SCHEDJOB_LIVECFG_PRELOAD+_cfgId,_approxMs,_val,_valhw,_relmode,_cfgId)  {}
	virtual void Init(ScheduledJob* _replacedJob = NULL);
protected:
	void Perform();
	double GetCurrentValue();
};


class LiveConfigsUpdateEditorJob : public ScheduledJob {
public:
	LiveConfigsUpdateEditorJob(int _approxMs)
		: ScheduledJob(SNM_SCHEDJOB_LIVECFG_UPDATE, _approxMs) {}
protected:
	void Perform();
};


void LiveConfigsSetTrackTitle();
void LiveConfigsTrackListChange();

int LiveConfigInit();
void LiveConfigExit();
void OpenLiveConfig(COMMAND_T*);
int IsLiveConfigDisplayed(COMMAND_T*);

void ApplyLiveConfig(int _cfgId, int _val, bool _immediate, int _valhw = -1, int _relmode = 0);
void PreloadLiveConfig(int _cfgId, int _val, bool _immediate, int _valhw = -1, int _relmode = 0);

void OpenLiveConfigMonitorWnd(int _idx);
void OpenLiveConfigMonitorWnd(COMMAND_T*);
int IsLiveConfigMonitorWndDisplayed(COMMAND_T*);

void ApplyLiveConfig(COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd);
void PreloadLiveConfig(COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd);

void SwapCurrentPreloadLiveConfigs(COMMAND_T*);

void ApplyNextLiveConfig(COMMAND_T*);
void ApplyPreviousLiveConfig(COMMAND_T*);
void PreloadNextLiveConfig(COMMAND_T*);
void PreloadPreviousLiveConfig(COMMAND_T*);
bool NextPreviousLiveConfig(int _cfgId, bool _preload, int _step);

int IsLiveConfigEnabled(COMMAND_T*);
void UpdateEnableLiveConfig(int _cfgId, int _val);
void EnableLiveConfig(COMMAND_T*);
void DisableLiveConfig(COMMAND_T*);
void ToggleEnableLiveConfig(COMMAND_T*);

int IsMuteOthersLiveConfigEnabled(COMMAND_T*);
void EnableMuteOthersLiveConfig(COMMAND_T*);
void DisableMuteOthersLiveConfig(COMMAND_T*);
void ToggleMuteOthersLiveConfig(COMMAND_T*);

int IsOfflineOthersLiveConfigEnabled(COMMAND_T*);
void EnableOfflineOthersLiveConfig(COMMAND_T*);
void DisableOfflineOthersLiveConfig(COMMAND_T*);
void ToggleOfflineOthersLiveConfig(COMMAND_T*);

int IsDisarmOthersLiveConfigEnabled(COMMAND_T*);
void EnableDisarmOthersLiveConfig(COMMAND_T*);
void DisableDisarmOthersLiveConfig(COMMAND_T*);
void ToggleDisarmOthersLiveConfig(COMMAND_T*);

int IsAllNotesOffLiveConfigEnabled(COMMAND_T*);
void EnableAllNotesOffLiveConfig(COMMAND_T*);
void DisableAllNotesOffLiveConfig(COMMAND_T*);
void ToggleAllNotesOffLiveConfig(COMMAND_T*);

int IsTinyFadesLiveConfigEnabled(COMMAND_T*);
void EnableTinyFadesLiveConfig(COMMAND_T*);
void DisableTinyFadesLiveConfig(COMMAND_T*);
void ToggleTinyFadesLiveConfig(COMMAND_T*);

#endif
