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

#ifndef _SNM_LIVECFGVIEW_H_
#define _SNM_LIVECFGVIEW_H_


class PresetMsg {
public:
	PresetMsg(int _fx=-1, const char* _preset="") : m_fx(_fx), m_preset(_preset) {}
	int m_fx;
	WDL_FastString m_preset;
};


class MidiLiveItem {
public:
	MidiLiveItem(int _cc, const char* _desc="", MediaTrack* _track=NULL, const char* _trTemplate="", const char* _fxChain="", const char* _presets="", const char* _onAction="", const char* _offAction="")
		: m_cc(_cc),m_desc(_desc),m_track(_track),m_trTemplate(_trTemplate),m_fxChain(_fxChain),m_presets(_presets),m_onAction(_onAction),m_offAction(_offAction) {}
	bool IsDefault(){return (!m_track && !m_desc.GetLength() && !m_trTemplate.GetLength() && !m_fxChain.GetLength() && !m_presets.GetLength() && !m_onAction.GetLength() && !m_offAction.GetLength());}
	void Clear() {m_track=NULL; m_desc.Set(""); m_trTemplate.Set(""); m_fxChain.Set(""); m_presets.Set(""); m_onAction.Set(""); m_offAction.Set("");}

	int m_cc;
	MediaTrack* m_track;
	WDL_FastString m_desc, m_trTemplate, m_fxChain, m_presets, m_onAction, m_offAction;
};


class MidiLiveConfig {
public:
	MidiLiveConfig()
	{ 
		m_version = 0;
		m_enable = 1;
		m_muteOthers = 1;
		m_autoSelect = 1;
		m_inputTr = NULL;
		m_lastActivatedTr = NULL;
		m_lastMIDIVal = -1;
		m_lastDeactivateCmd[0] = -1;
		for (int j=0; j < 128; j++)
			m_ccConfs.Add(new MidiLiveItem(j, "", NULL, "", "", "", "", ""));
	}
	~MidiLiveConfig() { m_ccConfs.Empty(true); }

	WDL_PtrList<MidiLiveItem> m_ccConfs;
	int m_version, m_enable, m_muteOthers, m_autoSelect;
	MediaTrack* m_inputTr, *m_lastActivatedTr;
	int m_lastMIDIVal;
	int m_lastDeactivateCmd[4]; // [0]=cmd,[1]=val,[2]=valhw,[3]=relmode
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
protected:
	void OnInitDlg();
	void OnDestroy();
	INT_PTR WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void AddFXSubMenu(HMENU _menu, MediaTrack* _tr, WDL_FastString* _curPresetConf);
	HMENU OnContextMenu(int x, int y, bool* wantDefaultItems);
	int OnKey(MSG* msg, int iKeyState);
	void DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight = NULL);
	HBRUSH OnColorEdit(HWND _hwnd, HDC _hdc);
	bool Insert();

	WDL_PtrList_DeleteOnDestroy<PresetMsg> m_lastPresetMsg;

	WDL_VirtualComboBox m_cbConfig, m_cbInputTr;
	WDL_VirtualIconButton m_btnEnable, m_btnMuteOthers, m_btnAutoSelect;
	WDL_VirtualStaticText m_txtConfig, m_txtInputTr;
#ifdef _SNM_MISC // wip
	SNM_MiniKnob m_knob;
#endif
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


#endif
