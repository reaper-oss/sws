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


#define SNM_LIVECFG_NB_CONFIGS			8
#define SNM_LIVECFG_MAX_PRESET_COUNT	2048 //JFB would be ideal: (SNM_LIVECFG_CLEAR_PRESETS_MSG - SNM_LIVECFG_SET_PRESETS_MSG)


class MidiLiveItem {
public:
	MidiLiveItem(int _cc, const char* _desc, MediaTrack* _track, const char* _trTemplate, const char* _fxChain, const char* _presets, const char* _onAction, const char* _offAction)
		: m_cc(_cc),m_desc(_desc),m_track(_track),m_trTemplate(_trTemplate),m_fxChain(_fxChain),m_presets(_presets),m_onAction(_onAction),m_offAction(_offAction) {}
	bool IsDefault(){return (!m_track && !m_desc.GetLength() && !m_trTemplate.GetLength() && !m_fxChain.GetLength() && !m_presets.GetLength() && !m_onAction.GetLength() && !m_offAction.GetLength());}
	void Clear() {m_track=NULL; m_desc.Set(""); m_trTemplate.Set(""); m_fxChain.Set(""); m_presets.Set(""); m_onAction.Set(""); m_offAction.Set("");}

	int m_cc;
	MediaTrack* m_track;
	WDL_FastString m_desc, m_trTemplate, m_fxChain, m_presets, m_onAction, m_offAction;
};


class MidiLiveConfig {
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

	// [0] = cmd, [1] = val, [2] = valhw, [3] = relmode, [4] = track index
	int m_lastDeactivateCmd[SNM_LIVECFG_NB_CONFIGS][5];
};


class SNM_LiveConfigsView : public SWS_ListView {
public:
	SNM_LiveConfigsView(HWND hwndList, HWND hwndEdit);
protected:
	void GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax);
	void SetItemText(SWS_ListItem* item, int iCol, const char* str);
	void GetItemList(SWS_ListItemList* pList);
	void OnItemSelChanged(SWS_ListItem* item, int iState);
	void OnItemDblClk(SWS_ListItem* item, int iCol);
	int OnItemSort(SWS_ListItem* item1, SWS_ListItem* item2);
};


class SNM_LiveConfigsWnd : public SWS_DockWnd {
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
	INT_PTR WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void OnInitDlg();
	HMENU OnContextMenu(int x, int y);
	void OnDestroy();
	int OnKey(MSG* msg, int iKeyState);
	void DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight = NULL);
	HBRUSH OnColorEdit(HWND _hwnd, HDC _hdc);

	WDL_VirtualComboBox m_cbConfig, m_cbInputTr;
	WDL_VirtualIconButton m_btnEnable, m_btnMuteOthers, m_btnAutoSelect;
	WDL_VirtualStaticText m_txtConfig, m_txtInputTr;
};


class SNM_MidiLiveScheduledJob : public SNM_ScheduledJob {
public:
	SNM_MidiLiveScheduledJob(int _id, int _approxDelayMs, int _cfgId, int _val, int _valhw, int _relmode, HWND _hwnd) 
		: SNM_ScheduledJob(_id, _approxDelayMs),m_cfgId(_cfgId),m_val(_val),m_valhw(_valhw),m_relmode(_relmode),m_hwnd(_hwnd) {}
	void Perform();
protected:
	int m_cfgId, m_val, m_valhw, m_relmode;
	HWND m_hwnd;
};


class SNM_LiveCfg_TLChangeSchedJob : public SNM_ScheduledJob {
public:
	SNM_LiveCfg_TLChangeSchedJob() : SNM_ScheduledJob(SNM_SCHEDJOB_LIVECFG_TLCHANGE, 150) {}
	void Perform();
};
