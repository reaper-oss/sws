/******************************************************************************
/ SnM_MidiLiveView.h
/ JFB TODO? now, SnM_LiveConfigsView.h would be a better name..
/
/ Copyright (c) 2010-2011 Tim Payne (SWS), Jeffos
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

#define SNM_LIVECFG_NB_CONFIGS					8

#define SNM_LIVECFG_CLEAR_CC_ROW_MSG			0x110001
#define SNM_LIVECFG_LOAD_TRACK_TEMPLATE_MSG		0x110002
#define SNM_LIVECFG_CLEAR_TRACK_TEMPLATE_MSG	0x110003
#define SNM_LIVECFG_LOAD_FXCHAIN_MSG			0x110004
#define SNM_LIVECFG_CLEAR_FXCHAIN_MSG			0x110005
#define SNM_LIVECFG_PERFORM_MSG					0x110006 
#define SNM_LIVECFG_EDIT_DESC_MSG				0x110007
#define SNM_LIVECFG_CLEAR_DESC_MSG				0x110008
#define SNM_LIVECFG_EDIT_ON_ACTION_MSG			0x110009
#define SNM_LIVECFG_CLEAR_ON_ACTION_MSG			0x11000A
#define SNM_LIVECFG_EDIT_OFF_ACTION_MSG			0x11000B
#define SNM_LIVECFG_CLEAR_OFF_ACTION_MSG		0x11000C 
#define SNM_LIVECFG_SET_TRACK_MSG				0x11000D // => 243 track max.
#define SNM_LIVECFG_CLEAR_TRACK_MSG				0x110100
#define SNM_LIVECFG_SET_PRESETS_MSG				0x110101 // => 3838 preset max.
#define SNM_LIVECFG_CLEAR_PRESETS_MSG			0x110FFF

#define SNM_LIVECFG_MAX_TRACK_COUNT				(SNM_LIVECFG_CLEAR_TRACK_MSG - SNM_LIVECFG_SET_TRACK_MSG)
#define SNM_LIVECFG_MAX_PRESET_COUNT			(SNM_LIVECFG_CLEAR_PRESETS_MSG - SNM_LIVECFG_SET_PRESETS_MSG)

#define SNM_LIVECFG_UNDO_STR					"Live configs edition"


class MidiLiveItem
{
public:
	MidiLiveItem(int _cc, const char* _desc, MediaTrack* _track, const char* _trTemplate, const char* _fxChain, const char* _presets, const char* _onAction, const char* _offAction)
		:m_cc(_cc),m_desc(_desc),m_track(_track),m_trTemplate(_trTemplate),m_fxChain(_fxChain),m_presets(_presets),m_onAction(_onAction),m_offAction(_offAction) {}
	bool IsDefault(){
		return (!m_track && !m_desc.GetLength() && !m_trTemplate.GetLength() && !m_fxChain.GetLength() && !m_presets.GetLength() && !m_onAction.GetLength() && !m_offAction.GetLength());
	}
	void Clear() {m_track=NULL; m_desc.Set(""); m_trTemplate.Set(""); m_fxChain.Set(""); m_presets.Set(""); m_onAction.Set(""); m_offAction.Set("");}

	int m_cc;
	WDL_String m_desc;
	MediaTrack* m_track;
	WDL_String m_trTemplate;
	WDL_String m_fxChain;
	WDL_String m_presets;
	WDL_String m_onAction;
	WDL_String m_offAction;
};

class MidiLiveConfig
{
public:
	MidiLiveConfig() {Clear();}
	void Clear() 
	{
		for (int i=0; i < SNM_LIVECFG_NB_CONFIGS; i++) 
		{
			m_enable[i] = 1;
			m_autoRcv[i] = 0; //JFB not released
			m_muteOthers[i] = 1;
			m_autoSelect[i] = 1;
			m_inputTr[i] = NULL;

			m_lastMIDIVal[i] = -1;
			m_lastDeactivateCmd[i][0] = -1;
		}
	}

	int m_enable[SNM_LIVECFG_NB_CONFIGS];
	int m_autoRcv[SNM_LIVECFG_NB_CONFIGS];
	int m_muteOthers[SNM_LIVECFG_NB_CONFIGS];
	int m_autoSelect[SNM_LIVECFG_NB_CONFIGS];
	MediaTrack* m_inputTr[SNM_LIVECFG_NB_CONFIGS];

	int m_lastMIDIVal[SNM_LIVECFG_NB_CONFIGS];
	int m_lastDeactivateCmd[SNM_LIVECFG_NB_CONFIGS][4];
};

class SNM_LiveConfigsView : public SWS_ListView
{
public:
	SNM_LiveConfigsView(HWND hwndList, HWND hwndEdit);
protected:
	void GetItemText(LPARAM item, int iCol, char* str, int iStrMax);
	void SetItemText(LPARAM item, int iCol, const char* str);
	void GetItemList(WDL_TypedBuf<LPARAM>* pBuf);
	void OnItemDblClk(LPARAM item, int iCol);
};

class SNM_LiveConfigsWnd : public SWS_DockWnd
{
public:
	SNM_LiveConfigsWnd();
	void Update();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	void CSurfSetTrackTitle();
	void CSurfSetTrackListChange();
	void FillComboInputTrack();
	void SelectByCCValue(int _configId, int _cc);

	int m_lastFXPresetMsg[2][SNM_LIVECFG_MAX_PRESET_COUNT];
	
protected:
	void OnInitDlg();
	HMENU OnContextMenu(int x, int y);
	void OnDestroy();
	int OnKey(MSG* msg, int iKeyState);
	int OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);

	// WDL UI
	WDL_VWnd_Painter m_vwnd_painter;
	WDL_VWnd m_parentVwnd; // owns all children windows
	WDL_VirtualComboBox m_cbConfig;
	WDL_VirtualIconButton m_btnEnable;
	WDL_VirtualIconButton m_btnAutoRcv;
	WDL_VirtualIconButton m_btnMuteOthers;
	WDL_VirtualIconButton m_btnAutoSelect;
	WDL_VirtualComboBox m_cbInputTr;
};

class SNM_MidiLiveScheduledJob : public SNM_ScheduledJob
{
public:
	SNM_MidiLiveScheduledJob(int _id, int _approxDelayMs, int _cfgId, int _val, int _valhw, int _relmode, HWND _hwnd) 
		: SNM_ScheduledJob(_id, _approxDelayMs) 
	{
		m_cfgId = _cfgId;
		m_val = _val;
		m_valhw = _valhw;
		m_relmode = _relmode;
		m_hwnd = _hwnd;
	}
	void Perform();

protected:
	int m_cfgId;
	int m_val;
	int m_valhw;
	int m_relmode;
	HWND m_hwnd;
};

class SNM_SelectProjectScheduledJob : public SNM_ScheduledJob
{
public:
	SNM_SelectProjectScheduledJob(int _approxDelayMs, int _val, int _valhw, int _relmode, HWND _hwnd) 
		: SNM_ScheduledJob(SNM_SCHEDJOB_LIVECFG_SELPRJ, _approxDelayMs) 
	{
		m_val = _val;
		m_valhw = _valhw;
		m_relmode = _relmode;
		m_hwnd = _hwnd;
	}
	void Perform();

protected:
	int m_val;
	int m_valhw;
	int m_relmode;
	HWND m_hwnd;
};

class SNM_LiveCfg_TLChangeSchedJob : public SNM_ScheduledJob
{
public:
	SNM_LiveCfg_TLChangeSchedJob() : SNM_ScheduledJob(SNM_SCHEDJOB_LIVECFG_TLCHANGE, 150) {}
	void Perform();
};
