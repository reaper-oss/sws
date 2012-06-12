/******************************************************************************
/ SnM_LiveConfigs.cpp
/
/ Copyright (c) 2010-2012 Jeffos
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

// JFB TODO?
// - max nb of tracks & presets to check
// - FX chains: + S&M's FX chain view slots
// - full release? auto send creation, etc..

#include "stdafx.h"
#include "SnM.h"
#include "../reaper/localize.h"


#define SNM_LIVECFG_VERSION						2 

#define SNM_LIVECFG_PERFORM_MSG					0xF001 
#define SNM_LIVECFG_CLEAR_CC_ROW_MSG			0xF002
#define SNM_LIVECFG_CLEAR_TRACK_MSG				0xF003
#define SNM_LIVECFG_CLEAR_FXCHAIN_MSG			0xF004
#define SNM_LIVECFG_CLEAR_TRACK_TEMPLATE_MSG	0xF005
#define SNM_LIVECFG_CLEAR_DESC_MSG				0xF006
#define SNM_LIVECFG_CLEAR_PRESETS_MSG			0xF007
#define SNM_LIVECFG_CLEAR_ON_ACTION_MSG			0xF008
#define SNM_LIVECFG_CLEAR_OFF_ACTION_MSG		0xF009 
#define SNM_LIVECFG_LOAD_TRACK_TEMPLATE_MSG		0xF00A
#define SNM_LIVECFG_LOAD_FXCHAIN_MSG			0xF00B
#define SNM_LIVECFG_EDIT_DESC_MSG				0xF00C
#define SNM_LIVECFG_EDIT_ON_ACTION_MSG			0xF00D
#define SNM_LIVECFG_EDIT_OFF_ACTION_MSG			0xF00E
#define SNM_LIVECFG_LEARN_ON_ACTION_MSG			0xF00F
#define SNM_LIVECFG_LEARN_OFF_ACTION_MSG		0xF010
#define SNM_LIVECFG_LEARN_PRESETS_START_MSG		0xF100 // 255 fx learn max -->
#define SNM_LIVECFG_LEARN_PRESETS_END_MSG		0xF1FF // <--
#define SNM_LIVECFG_SET_TRACK_START_MSG			0xF200 // 255 track max -->
#define SNM_LIVECFG_SET_TRACK_END_MSG			0xF2FF // <--
#define SNM_LIVECFG_SET_PRESET_START_MSG		0xF300 // 3327 preset max -->
#define SNM_LIVECFG_SET_PRESET_END_MSG			0xFFFF // <--

#define SNM_LIVECFG_MAX_PRESET_COUNT			(SNM_LIVECFG_SET_PRESET_END_MSG - SNM_LIVECFG_SET_PRESET_START_MSG)
#define SAVEWINDOW_POS_KEY						"S&M - Live Configs List Save Window Position"
#define SNM_LIVECFG_UNDO_STR					__LOCALIZE("Live Configs edition", "sws_undo")


enum {
  TXTID_CONFIG=2000, //JFB would be great to have _APS_NEXT_CONTROL_VALUE *always* defined
  COMBOID_CONFIG,
  BUTTONID_ENABLE,
  TXTID_INPUT_TRACK,
  COMBOID_INPUT_TRACK,
  BUTTONID_MUTE_OTHERS,
  BUTTONID_AUTO_SELECT,
  KNOBID_CC_DELAY
};

enum {
  COL_CC=0,
  COL_COMMENT,
  COL_TR,
  COL_TRT,
  COL_FXC,
  COL_PRESET,
  COL_ACTION_ON,
  COL_ACTION_OFF,
  COL_COUNT
};


SNM_LiveConfigsWnd* g_pLiveConfigsWnd = NULL;
SWSProjConfig<WDL_PtrList_DeleteOnDestroy<MidiLiveConfig> > g_liveConfigs;
int g_configId = 0; // the current *displayed* config id
int g_approxDelayMsCC = SNM_SCHEDJOB_DEFAULT_DELAY;


///////////////////////////////////////////////////////////////////////////////
// presets helpers
///////////////////////////////////////////////////////////////////////////////

// updates a preset conf string
// _presetConf: in & out param
// _fx: 1-based
// _preset: new preset name or NULL or "" to remove
void UpdatePresetConf(WDL_FastString* _presetConf, int _fx, const char* _preset)
{
	WDL_FastString newConf, escStr, escPresetName;

	if (_preset)
		makeEscapedConfigString(_preset, &escPresetName);

	LineParser lp(false);
	if (_presetConf && !lp.parse(_presetConf->Get()))
	{
		bool found = false;
		char fxBuf[32]="";
		if (_snprintfStrict(fxBuf, sizeof(fxBuf), "FX%d:", _fx) > 0)
		{
			for (int i=0; i < lp.getnumtokens(); i++)
			{
				if (!strcmp(lp.gettoken_str(i), fxBuf))
				{
					// set the new preset, if not removed
					if (_preset && *_preset)
					{
						if ((i+1) < lp.getnumtokens())
						{
							found = true;
							if (newConf.GetLength())
								newConf.Append(" ");
							newConf.Append(lp.gettoken_str(i));
							newConf.Append(" ");
							newConf.Append(escPresetName.Get());
						}
					}
				}
				else if (!strncmp(lp.gettoken_str(i), "FX", 2))
				{
					if ((i+1) < lp.getnumtokens())
					{
						if (newConf.GetLength())
							newConf.Append(" ");
						newConf.Append(lp.gettoken_str(i));
						newConf.Append(" ");
						makeEscapedConfigString(lp.gettoken_str(i+1), &escStr);
						newConf.Append(escStr.Get());
					}
				}
				i++; // skip next token
			}
		}

		if (!found && _preset && *_preset) {
			if (newConf.GetLength())
				newConf.Append(" ");
			newConf.AppendFormatted(256, "FX%d: ", _fx);
			newConf.Append(escPresetName.Get());
		}
	}
	if (_presetConf)
		_presetConf->Set(&newConf);
}

int GetPresetFromConfTokenV1(const char* _preset) {
	if (const char* p = strchr(_preset, '.'))
		return atoi(p+1);
	return 0;
}

// returns the preset number (1-based, 0 if failed) from a preset conf string
// _fx: 0-based, _presetConf: stored as "fx.preset", both 1-based
// _presetCount: for check ("optional", i.e. default = 0xFFFF)
int GetPresetFromConfV1(int _fx, const char* _presetConf, int _presetCount)
{
	LineParser lp(false);
	if (_presetConf && !lp.parse(_presetConf))
	{
		for (int i=0; i < lp.getnumtokens(); i++)
		{
			int fx = (int)floor(lp.gettoken_float(i));
			if (_fx == (fx-1)) {
				int preset = GetPresetFromConfTokenV1(lp.gettoken_str(i));
				return (preset>_presetCount ? 0 : preset);
			}
		}
	}
	return 0;
}

bool GetPresetFromConfV2(int _fx, const char* _presetConf, WDL_FastString* _presetName)
{
	if (_presetName)
		_presetName->Set("");

	LineParser lp(false);
	if (_presetConf && _presetName && !lp.parse(_presetConf))
	{
		char fxBuf[32]="";
		if (_snprintfStrict(fxBuf, sizeof(fxBuf), "FX%d:", _fx+1) > 0)
		{
			for (int i=0; i < lp.getnumtokens(); i++)
			{
				if (!strcmp(lp.gettoken_str(i), fxBuf))
				{
					if (i+1 < lp.getnumtokens()) {
						_presetName->Set(lp.gettoken_str(i+1));
						return true;
					}
					break;
				}
			}
		}
	}
	return false;
}

void UpgradePresetConfV1toV2(MediaTrack* _tr, const char* _confV1, WDL_FastString* _confV2)
{
	if (_tr && _confV1 && *_confV1 && _confV2 && TrackFX_GetCount(_tr))
	{
		_confV2->Set("");

#ifdef _WIN32 // fx presets were not available on OSX before v2
		LineParser lp(false);
		if (!lp.parse(_confV1))
		{
			WDL_FastString escStr;
			for (int i=0; i < lp.getnumtokens(); i++)
			{
				if (_confV2->GetLength())
					_confV2->Append(" ");

				int success, fx = (int)floor(lp.gettoken_float(i, &success)) - 1; // float because token is like "1.3"
				int preset = success ? GetPresetFromConfTokenV1(lp.gettoken_str(i)) - 1 : -1;

				// avoid parsing as far as possible!
				SNM_FXSummaryParser p(_tr);
				WDL_PtrList<SNM_FXSummary>* summaries = (preset >= 0 ? p.GetSummaries() : NULL);
				SNM_FXSummary* sum = summaries ? summaries->Get(fx) : NULL;

				WDL_PtrList_DeleteOnDestroy<WDL_FastString> names;
				int presetCount = (sum ? GetUserPresetNames(sum->m_type.Get(), sum->m_realName.Get(), &names) : 0);
				if (presetCount && preset >=0 && preset < presetCount) {
					makeEscapedConfigString(names.Get(preset)->Get(), &escStr);
					_confV2->AppendFormatted(256, "FX%d: %s", fx+1, escStr.Get());
				}
			}
		}
#endif
	}
}

// trigger several track fx presets
// _presetConf: "fx.preset", both 1-based - e.g. "2.9 3.2" => FX2 preset 9, FX3 preset 2 
bool TriggerFXPresets(MediaTrack* _tr, WDL_FastString* _presetConf)
{
	bool updated = false;
	if (int nbFx = ((_tr&&_presetConf) ? TrackFX_GetCount(_tr) : 0))
	{
		WDL_FastString presetName;
		for (int i=0; i < nbFx; i++)
			if (GetPresetFromConfV2(i, _presetConf->Get(), &presetName))
				updated |= TrackFX_SetPreset(_tr, i, presetName.Get());
	}
	return updated;
}


///////////////////////////////////////////////////////////////////////////////
// SNM_LiveConfigsView
///////////////////////////////////////////////////////////////////////////////

// !WANT_LOCALIZE_STRINGS_BEGIN:sws_DLG_155
static SWS_LVColumn g_liveCfgListCols[] = { 
	{95,2,"CC value"}, {150,1,"Comment"}, {150,2,"Track"}, {175,2,"Track template"}, {175,2,"FX Chain"}, {150,2,"FX presets"}, {150,0,"Activate action"}, {150,0,"Deactivate action"}};
// !WANT_LOCALIZE_STRINGS_END

SNM_LiveConfigsView::SNM_LiveConfigsView(HWND hwndList, HWND hwndEdit)
:SWS_ListView(hwndList, hwndEdit, COL_COUNT, g_liveCfgListCols, "LiveConfigsViewState", false, "sws_DLG_155")
{
}

void SNM_LiveConfigsView::GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax)
{
	if (str) *str = '\0';
	if (MidiLiveItem* pItem = (MidiLiveItem*)item)
	{
		switch (iCol)
		{
			case COL_CC:
				_snprintfSafe(str, iStrMax, "%03d %s", pItem->m_cc, pItem->m_cc == g_liveConfigs.Get()->Get(g_configId)->m_lastMIDIVal ? UTF8_BULLET : " ");
				break;
			case COL_COMMENT:
				lstrcpyn(str, pItem->m_desc.Get(), iStrMax);
				break;
			case COL_TR:
				if (pItem->m_track)
					if (char* name = (char*)GetSetMediaTrackInfo(pItem->m_track, "P_NAME", NULL))
						_snprintfSafe(str, iStrMax, "[%d] \"%s\"", CSurf_TrackToID(pItem->m_track, false), name);
				break;
			case COL_TRT:
				GetFilenameNoExt(pItem->m_trTemplate.Get(), str, iStrMax);
				break;
			case COL_FXC:
				GetFilenameNoExt(pItem->m_fxChain.Get(), str, iStrMax);
				break;
			case COL_PRESET:
				lstrcpyn(str, pItem->m_presets.Get(), iStrMax);
				break;
			case COL_ACTION_ON:
			case COL_ACTION_OFF: 
			if (iCol==COL_ACTION_ON ? pItem->m_onAction.GetLength()>0 : pItem->m_offAction.GetLength()>0)
			{
				int cmd = NamedCommandLookup(iCol==COL_ACTION_ON ? pItem->m_onAction.Get() : pItem->m_offAction.Get());
				lstrcpyn(str, cmd>0 ? kbd_getTextFromCmd(cmd, NULL) : "?", iStrMax);
				break;
			}
		}
	}
}

void SNM_LiveConfigsView::SetItemText(SWS_ListItem* _item, int _iCol, const char* _str)
{
	if (MidiLiveItem* pItem = (MidiLiveItem*)_item)
	{
		switch (_iCol)
		{
			case COL_COMMENT:
				// limit the comment size (for RPP files)
				pItem->m_desc.Set(_str);
				pItem->m_desc.Ellipsize(32,32);
				Update();
				Undo_OnStateChangeEx(SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
				break;

			case COL_ACTION_ON:
			case COL_ACTION_OFF:
				if (*_str && !NamedCommandLookup(_str)) {
					MessageBox(GetParent(m_hwndList), __LOCALIZE("Error: this action ID (or custom ID) does not exist!","sws_DLG_155"), __LOCALIZE("S&M - Error","sws_DLG_155"), MB_OK);
					return;
				}
				if (_iCol == COL_ACTION_ON) pItem->m_onAction.Set(_str);
				else pItem->m_offAction.Set(_str);
				Update();
				Undo_OnStateChangeEx(SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
				break;
		}
	}
}

void SNM_LiveConfigsView::GetItemList(SWS_ListItemList* pList)
{
	if (WDL_PtrList<MidiLiveItem>* ccConfs = &g_liveConfigs.Get()->Get(g_configId)->m_ccConfs)
		for (int i=0; i < ccConfs->GetSize(); i++)
			pList->Add((SWS_ListItem*)ccConfs->Get(i));
}

// gotcha! several calls for one selection change (eg. 3 unselected rows + 1 new selection)
void SNM_LiveConfigsView::OnItemSelChanged(SWS_ListItem* item, int iState)
{
	bool updated = false;
	if (iState & LVIS_SELECTED)
	{
		MidiLiveItem* pItem = (MidiLiveItem*)item;
		if (pItem && pItem->m_track)
		{
			for (int i=0; i <= GetNumTracks(); i++)
			{
				if (MediaTrack* tr = CSurf_TrackFromID(i, false))
				{
					if (tr == pItem->m_track)
					{
						if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL) == 0) {
							GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i1);
							updated = true;
						}
					}
					else if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL)) {
						GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i0);
						updated = true;
					}
				}
			}
		}
	}
	if (updated) {
		ScrollSelTrack(NULL, true, true);
		if ((*(int*)GetConfigVar("undomask")) & 1)
			Undo_OnStateChangeEx(__localizeFunc("Change Track Selection","undo",0), UNDO_STATE_ALL, -1);
	}
}


void SNM_LiveConfigsView::OnItemDblClk(SWS_ListItem* item, int iCol)
{
	if (MidiLiveItem* pItem = (MidiLiveItem*)item)
	{
		switch(iCol)
		{
			case COL_CC:
				if (g_pLiveConfigsWnd)
					g_pLiveConfigsWnd->OnCommand(SNM_LIVECFG_PERFORM_MSG, 0);
				break;
			case COL_TRT:
				if (g_pLiveConfigsWnd && pItem->m_track)
					g_pLiveConfigsWnd->OnCommand(SNM_LIVECFG_LOAD_TRACK_TEMPLATE_MSG, 0);
				break;
			case COL_FXC:
				if (g_pLiveConfigsWnd && pItem->m_track)
					g_pLiveConfigsWnd->OnCommand(SNM_LIVECFG_LOAD_FXCHAIN_MSG, 0);
				break;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// SNM_LiveConfigsWnd
///////////////////////////////////////////////////////////////////////////////

SNM_LiveConfigsWnd::SNM_LiveConfigsWnd()
	: SWS_DockWnd(IDD_SNM_LIVE_CONFIGS, __LOCALIZE("Live Configs","sws_DLG_155"), "SnMLiveConfigs", 30009, SWSGetCommandID(OpenLiveConfigView))
{
	// Must call SWS_DockWnd::Init() to restore parameters and open the window if necessary
	Init();
}

void SNM_LiveConfigsWnd::OnInitDlg()
{
	m_resize.init_item(IDC_LIST, 0.0, 0.0, 1.0, 1.0);
	m_pLists.Add(new SNM_LiveConfigsView(GetDlgItem(m_hwnd, IDC_LIST), GetDlgItem(m_hwnd, IDC_EDIT)));
	SNM_ThemeListView(m_pLists.Get(0));

	// load prefs 
	g_approxDelayMsCC = BOUNDED(GetPrivateProfileInt("LIVE_CONFIGS", "CC_DELAY", SNM_SCHEDJOB_DEFAULT_DELAY, g_SNMIniFn.Get()), 0, 500);

	// WDL UI init
	m_vwnd_painter.SetGSC(WDL_STYLE_GetSysColor);
	m_parentVwnd.SetRealParent(m_hwnd);

	m_txtConfig.SetID(TXTID_CONFIG);
	m_txtConfig.SetText(__LOCALIZE("Config:","sws_DLG_155"));
	m_parentVwnd.AddChild(&m_txtConfig);

	m_cbConfig.SetID(COMBOID_CONFIG);
	char cfg[8] = "";
	for (int i=0; i < SNM_LIVECFG_NB_CONFIGS; i++) {
		_snprintfSafe(cfg, sizeof(cfg), "%d", i+1);
		m_cbConfig.AddItem(cfg);
	}
	m_cbConfig.SetCurSel(g_configId);
	m_parentVwnd.AddChild(&m_cbConfig);

	m_btnEnable.SetID(BUTTONID_ENABLE);
	m_parentVwnd.AddChild(&m_btnEnable);

	m_txtInputTr.SetID(TXTID_INPUT_TRACK);
	m_txtInputTr.SetText(__LOCALIZE("Input track:","sws_DLG_155"));
	m_parentVwnd.AddChild(&m_txtInputTr);

	m_cbInputTr.SetID(COMBOID_INPUT_TRACK);
	FillComboInputTrack();
	m_parentVwnd.AddChild(&m_cbInputTr);

	m_btnMuteOthers.SetID(BUTTONID_MUTE_OTHERS);
	m_parentVwnd.AddChild(&m_btnMuteOthers);

	m_btnAutoSelect.SetID(BUTTONID_AUTO_SELECT);
	m_parentVwnd.AddChild(&m_btnAutoSelect);

#ifdef _SNM_MISC // wip
	m_knob.SetID(KNOBID_CC_DELAY);
	m_parentVwnd.AddChild(&m_knob);
#endif
	Update();
}

void SNM_LiveConfigsWnd::OnDestroy() 
{
	char delay[8]="";
	if (_snprintfStrict(delay, sizeof(delay), "%d", g_approxDelayMsCC) > 0)
		WritePrivateProfileString("LIVE_CONFIGS", "CC_DELAY", delay, g_SNMIniFn.Get()); 
	m_cbConfig.Empty();
	m_cbInputTr.Empty();
}

INT_PTR SNM_LiveConfigsWnd::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static int sListOldColors[LISTVIEW_COLORHOOK_STATESIZE];
	if (ListView_HookThemeColorsMessage(m_hwnd, uMsg, lParam, sListOldColors, IDC_LIST, 0, COL_COUNT))
		return 1;
	return SWS_DockWnd::WndProc(uMsg, wParam, lParam);
}

// ScheduledJob because of possible multi-notifs
void SNM_LiveConfigsWnd::CSurfSetTrackListChange() {
	AddOrReplaceScheduledJob(new SNM_LiveCfg_TLChangeSchedJob());
}

void SNM_LiveConfigsWnd::CSurfSetTrackTitle() {
	FillComboInputTrack();
	Update();
}

void SNM_LiveConfigsWnd::FillComboInputTrack() {
	m_cbInputTr.Empty();
	m_cbInputTr.AddItem(__LOCALIZE("None","sws_DLG_155"));
	for (int i=1; i <= GetNumTracks(); i++)
	{
		WDL_FastString ellips;
		char* name = (char*)GetSetMediaTrackInfo(CSurf_TrackFromID(i,false), "P_NAME", NULL);
		ellips.SetFormatted(SNM_MAX_TRACK_NAME_LEN, "[%d] \"%s\"", i, name?name:"");
		ellips.Ellipsize(24, 24);
		m_cbInputTr.AddItem(ellips.Get());
	}
}

void SNM_LiveConfigsWnd::SelectByCCValue(int _configId, int _cc)
{
	if (_configId == g_configId)
	{
		SWS_ListView* lv = m_pLists.Get(0);
		HWND hList = lv ? lv->GetHWND() : NULL;
		if (lv && hList) // this can be called when the view is closed!
		{
			for (int i=0; i < lv->GetListItemCount(); i++)
			{
				MidiLiveItem* item = (MidiLiveItem*)lv->GetListItem(i);
				if (item && item->m_cc == _cc) 
				{
					ListView_SetItemState(hList, -1, 0, LVIS_SELECTED);
					ListView_SetItemState(hList, i, LVIS_SELECTED, LVIS_SELECTED); 
					ListView_EnsureVisible(hList, i, true);
					Update(); // just to set the bullet
					break;
				}
			}
		}
	}
}

void SNM_LiveConfigsWnd::Update()
{
	if (m_pLists.GetSize())
		m_pLists.Get(0)->Update();
	m_parentVwnd.RequestRedraw(NULL);
}

void SNM_LiveConfigsWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	MidiLiveConfig* lc = g_liveConfigs.Get()->Get(g_configId);
	if (!lc)
		return;

	int x=0;
	MidiLiveItem* item = (MidiLiveItem*)m_pLists.Get(0)->EnumSelected(&x);

	switch (LOWORD(wParam))
	{
		case SNM_LIVECFG_CLEAR_CC_ROW_MSG:
		{
			bool updt = false;
			while(item) {
				item->Clear();
				updt = true;
				item = (MidiLiveItem*)m_pLists.Get(0)->EnumSelected(&x);
			}
			if (updt) {
				Update();
				Undo_OnStateChangeEx(SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
			}
			break;
		}
		case SNM_LIVECFG_PERFORM_MSG:
			if (item) {
				// Immediate job
				SNM_MidiLiveScheduledJob* job = 
					new SNM_MidiLiveScheduledJob(g_configId, 0, g_configId, item->m_cc, -1, 0, GetMainHwnd());
				AddOrReplaceScheduledJob(job);
			}
			break;
		case SNM_LIVECFG_EDIT_DESC_MSG:
			if (item) 
				m_pLists.Get(0)->EditListItem((SWS_ListItem*)item, COL_COMMENT);
			break;
		case SNM_LIVECFG_CLEAR_DESC_MSG:
		{
			bool updt = false;
			while(item) {
				item->m_desc.Set("");
				updt = true;
				item = (MidiLiveItem*)m_pLists.Get(0)->EnumSelected(&x);
			}
			if (updt) {
				Update();
				Undo_OnStateChangeEx(SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
			}
			break;
		}
		case SNM_LIVECFG_LOAD_TRACK_TEMPLATE_MSG:
		{
			bool updt = false;
			if (item)
			{
				char filename[BUFFER_SIZE];
				if (BrowseResourcePath(__LOCALIZE("S&M - Load track template","sws_DLG_155"), "TrackTemplates", "REAPER Track Template (*.RTrackTemplate)\0*.RTrackTemplate\0", filename, BUFFER_SIZE))
				{
					while(item) {
						item->m_trTemplate.Set(filename);
						item->m_fxChain.Set("");
						item->m_presets.Set("");
						updt = true;
						item = (MidiLiveItem*)m_pLists.Get(0)->EnumSelected(&x);
					}
				}
			}
			if (updt) {
				Update();
				Undo_OnStateChangeEx(SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
			}
			break;
		}
		case SNM_LIVECFG_CLEAR_TRACK_TEMPLATE_MSG:
		{
			bool updt = false;
			while(item) {
				item->m_trTemplate.Set("");
				updt = true;
				item = (MidiLiveItem*)m_pLists.Get(0)->EnumSelected(&x);
			}
			if (updt) {
				Update();
				Undo_OnStateChangeEx(SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
			}
			break;
		}
		case SNM_LIVECFG_LOAD_FXCHAIN_MSG:
		{
			bool updt = false;
			if (item)
			{
				char filename[BUFFER_SIZE];
				if (BrowseResourcePath(__LOCALIZE("S&M - Load FX Chain","sws_DLG_155"), "FXChains", "REAPER FX Chain (*.RfxChain)\0*.RfxChain\0", filename, BUFFER_SIZE))
				{
					while(item) {
						item->m_fxChain.Set(filename);
						item->m_trTemplate.Set("");
						item->m_presets.Set("");
						updt = true;
						item = (MidiLiveItem*)m_pLists.Get(0)->EnumSelected(&x);
					}
				}
			}
			if (updt) {
				Update();
				Undo_OnStateChangeEx(SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
			}
			break;
		}
		case SNM_LIVECFG_CLEAR_FXCHAIN_MSG:
		{
			bool updt = false;
			while(item) {
				item->m_fxChain.Set("");
				updt = true;
				item = (MidiLiveItem*)m_pLists.Get(0)->EnumSelected(&x);
			}
			if (updt) {
				Update();
				Undo_OnStateChangeEx(SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
			}
			break;
		}
		case SNM_LIVECFG_EDIT_ON_ACTION_MSG:
		case SNM_LIVECFG_EDIT_OFF_ACTION_MSG:
			if (item) 
				m_pLists.Get(0)->EditListItem((SWS_ListItem*)item, LOWORD(wParam)==SNM_LIVECFG_EDIT_ON_ACTION_MSG ? COL_ACTION_ON : COL_ACTION_OFF);
			break;
		case SNM_LIVECFG_LEARN_ON_ACTION_MSG:
		case SNM_LIVECFG_LEARN_OFF_ACTION_MSG:
		{
			char idstr[SNM_MAX_ACTION_CUSTID_LEN] = "";
			if (item && LearnAction(idstr, SNM_MAX_ACTION_CUSTID_LEN, __localizeFunc("Main","accel_sec",0)))
			{
				bool updt = false;
				while(item)
				{
					if (LOWORD(wParam)==SNM_LIVECFG_LEARN_ON_ACTION_MSG)
						item->m_onAction.Set(idstr);
					else
						item->m_offAction.Set(idstr);
					updt = true;
					item = (MidiLiveItem*)m_pLists.Get(0)->EnumSelected(&x);
				}
				if (updt) {
					Update();
					Undo_OnStateChangeEx(SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
				}
			}
			break;
		}
		case SNM_LIVECFG_CLEAR_ON_ACTION_MSG:
		case SNM_LIVECFG_CLEAR_OFF_ACTION_MSG:
		{
			bool updt = false;
			while(item)
			{
				if (LOWORD(wParam)==SNM_LIVECFG_CLEAR_ON_ACTION_MSG)
					item->m_onAction.Set("");
				else
					item->m_offAction.Set("");
				updt = true;
				item = (MidiLiveItem*)m_pLists.Get(0)->EnumSelected(&x);
			}
			if (updt) {
				Update();
				Undo_OnStateChangeEx(SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
			}
			break;
		}
		case SNM_LIVECFG_CLEAR_TRACK_MSG:
		{
			bool updt = false;
			while(item) {
				item->m_track = NULL;
				item->m_trTemplate.Set("");
				item->m_fxChain.Set("");
				item->m_presets.Set("");
				updt = true;
				item = (MidiLiveItem*)m_pLists.Get(0)->EnumSelected(&x);
			}
			if (updt) {
				Update();
				Undo_OnStateChangeEx(SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
			}
			break;
		}
		case SNM_LIVECFG_CLEAR_PRESETS_MSG:
		{
			bool updt = false;
			while(item) {
				item->m_presets.Set("");
				updt = true;
				item = (MidiLiveItem*)m_pLists.Get(0)->EnumSelected(&x);
			}
			if (updt) {
				Update();
				Undo_OnStateChangeEx(SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
			}
			break;
		}
		case BUTTONID_ENABLE:
			if (!HIWORD(wParam) || HIWORD(wParam)==600)
			{
				lc->m_enable = !(lc->m_enable);
				Undo_OnStateChangeEx(SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
				// Retreive the related toggle action and refresh toolbars
				char custId[SNM_MAX_ACTION_CUSTID_LEN];
				_snprintfSafe(custId, sizeof(custId), "%s%d", "_S&M_TOGGLE_LIVE_CFG", g_configId+1);
				RefreshToolbar(NamedCommandLookup(custId));
			}
			break;
		case BUTTONID_MUTE_OTHERS:
			if (!HIWORD(wParam) || HIWORD(wParam)==600) {
				lc->m_muteOthers = !(lc->m_muteOthers);
				Undo_OnStateChangeEx(SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
			}
			break;
		case BUTTONID_AUTO_SELECT:
			if (!HIWORD(wParam) || HIWORD(wParam)==600) {
				lc->m_autoSelect = !(lc->m_autoSelect);
				Undo_OnStateChangeEx(SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
			}
			break;
		case COMBOID_INPUT_TRACK:
			if (HIWORD(wParam)==CBN_SELCHANGE)
			{
				if (!m_cbInputTr.GetCurSel())
					lc->m_inputTr = NULL;
				else
					lc->m_inputTr = CSurf_TrackFromID(m_cbInputTr.GetCurSel(), false);
				m_parentVwnd.RequestRedraw(NULL);
				Undo_OnStateChangeEx(SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
			}
			break;
		case COMBOID_CONFIG:
			if (HIWORD(wParam)==CBN_SELCHANGE)
			{
				// stop cell editing (changing the list content would be ignored otherwise 
				// => dropdown box & list box unsynchronized)
				m_pLists.Get(0)->EditListItemEnd(false);
				g_configId = m_cbConfig.GetCurSel();
				Update();
			}
			break;

		default:
			if (LOWORD(wParam) >= SNM_LIVECFG_SET_TRACK_START_MSG && LOWORD(wParam) <= SNM_LIVECFG_SET_TRACK_END_MSG) 
			{
				bool updt = false;
				while(item)
				{
					item->m_track = CSurf_TrackFromID((int)LOWORD(wParam) - SNM_LIVECFG_SET_TRACK_START_MSG, false);
					if (!(item->m_track)) {
						item->m_fxChain.Set("");
						item->m_trTemplate.Set("");
					}
					item->m_presets.Set("");
					updt = true;
					item = (MidiLiveItem*)m_pLists.Get(0)->EnumSelected(&x);
				}
				if (updt) {
					Update();
					Undo_OnStateChangeEx(SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
				}
			}
			else if (LOWORD(wParam) >= SNM_LIVECFG_LEARN_PRESETS_START_MSG && LOWORD(wParam) <= SNM_LIVECFG_LEARN_PRESETS_END_MSG) 
			{
				bool updt = false;
				while(item)
				{
					if (item->m_track)
					{
						int fx = (int)LOWORD(wParam)-SNM_LIVECFG_LEARN_PRESETS_START_MSG;
						char buf[SNM_MAX_PRESET_NAME_LEN] = "";
						TrackFX_GetPreset(item->m_track, fx, buf, SNM_MAX_PRESET_NAME_LEN);
						UpdatePresetConf(&item->m_presets, fx+1, buf);
						item->m_fxChain.Set("");
						item->m_trTemplate.Set("");
						updt = true;
					}
					item = (MidiLiveItem*)m_pLists.Get(0)->EnumSelected(&x);
				}
				if (updt) {
					Update();
					Undo_OnStateChangeEx(SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
				}
				break;
			}
			else if (LOWORD(wParam) >= SNM_LIVECFG_SET_PRESET_START_MSG && LOWORD(wParam) <= SNM_LIVECFG_SET_PRESET_END_MSG) 
			{
				bool updt = false;
				while(item)
				{
					if (PresetMsg* msg = m_lastPresetMsg.Get((int)LOWORD(wParam) - SNM_LIVECFG_SET_PRESET_START_MSG))
					{
						UpdatePresetConf(&item->m_presets, msg->m_fx+1, msg->m_preset.Get());
						item->m_fxChain.Set("");
						item->m_trTemplate.Set("");
						updt = true;
					}
					item = (MidiLiveItem*)m_pLists.Get(0)->EnumSelected(&x);
				}
				if (updt) {
					Update();
					Undo_OnStateChangeEx(SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
				}
			}
			else
				Main_OnCommand((int)wParam, (int)lParam);
			break;
	}
}

void SNM_LiveConfigsWnd::AddFXSubMenu(HMENU _menu, MediaTrack* _tr, WDL_FastString* _curPresetConf)
{
	m_lastPresetMsg.Empty(true);

	int fxCount = (_tr ? TrackFX_GetCount(_tr) : 0);
	if (!fxCount) {
		AddToMenu(_menu, __LOCALIZE("(No FX on track)","sws_DLG_155"), 0, -1, false, MF_GRAYED);
		return;
	}

	SNM_FXSummaryParser p(_tr);
	WDL_PtrList<SNM_FXSummary>* summaries = p.GetSummaries();
	if (summaries && fxCount == summaries->GetSize())
	{
		char fxName[SNM_MAX_FX_NAME_LEN] = "";
#ifdef _WIN32
		int msgCpt = 0; //JFB TODO? check max. msgCpt value?
#endif
		for(int i = 0; i < fxCount; i++) 
		{
			if(TrackFX_GetFXName(_tr, i, fxName, SNM_MAX_FX_NAME_LEN))
			{
				HMENU fxSubMenu = CreatePopupMenu();
				WDL_FastString str;

				// learn
				char buf[SNM_MAX_PRESET_NAME_LEN] = "";
				TrackFX_GetPreset(_tr, i, buf, SNM_MAX_PRESET_NAME_LEN);
				str.SetFormatted(256, __LOCALIZE_VERFMT("Learn current preset: %s","sws_DLG_155"), *buf?buf:__LOCALIZE("undefined","sws_DLG_155"));
				AddToMenu(fxSubMenu, str.Get(), SNM_LIVECFG_LEARN_PRESETS_START_MSG + i, -1, false, *buf?0:MF_GRAYED);
#ifdef _WIN32
				// preset list
				WDL_PtrList_DeleteOnDestroy<WDL_FastString> names;
				SNM_FXSummary* sum = summaries->Get(i);
				if (int presetCount = (sum ? GetUserPresetNames(sum->m_type.Get(), sum->m_realName.Get(), &names) : 0))
				{
					if (GetMenuItemCount(fxSubMenu))
						AddToMenu(fxSubMenu, SWS_SEPARATOR, 0);

					WDL_FastString curPresetName;
					GetPresetFromConfV2(i, _curPresetConf->Get(), &curPresetName);
					AddToMenu(fxSubMenu, __LOCALIZE("None","sws_DLG_155"), SNM_LIVECFG_SET_PRESET_START_MSG + msgCpt, -1, false, !curPresetName.GetLength() ? MFS_CHECKED : MFS_UNCHECKED);
					m_lastPresetMsg.Add(new PresetMsg(i, ""));
					msgCpt++;

					for(int j=0; j < presetCount; j++)
					{
						if (names.Get(j)->GetLength())
						{
							AddToMenu(fxSubMenu, names.Get(j)->Get(), SNM_LIVECFG_SET_PRESET_START_MSG + msgCpt, -1, false, !strcmp(curPresetName.Get(), names.Get(j)->Get()) ? MFS_CHECKED : MFS_UNCHECKED);
							m_lastPresetMsg.Add(new PresetMsg(i, names.Get(j)->Get()));
							msgCpt++;
						}
					}
				}
#endif
				// add fx sub menu
				str.SetFormatted(512, "FX%d: %s", i+1, fxName);
				AddSubMenu(_menu, fxSubMenu, str.Get(), -1, GetMenuItemCount(fxSubMenu) ? MFS_ENABLED : MF_GRAYED);
			}
		}
	}
	else
		AddToMenu(_menu, __LOCALIZE("(Unknown FX on track)","sws_DLG_155"), 0, -1, false, MF_GRAYED);
}

HMENU SNM_LiveConfigsWnd::OnContextMenu(int x, int y, bool* wantDefaultItems)
{
	HMENU hMenu = NULL;
	int iCol;
	if (MidiLiveItem* item = (MidiLiveItem*)m_pLists.Get(0)->GetHitItem(x, y, &iCol))
	{
		*wantDefaultItems = (iCol < 0);
		switch(iCol)
		{
			case COL_CC:
				hMenu = CreatePopupMenu();
				AddToMenu(hMenu, __LOCALIZE("Activate","sws_DLG_155"), SNM_LIVECFG_PERFORM_MSG);
				break;
			case COL_COMMENT:
				hMenu = CreatePopupMenu();
				AddToMenu(hMenu, __LOCALIZE("Edit comment","sws_DLG_155"), SNM_LIVECFG_EDIT_DESC_MSG);
				AddToMenu(hMenu, __LOCALIZE("Clear comments","sws_DLG_155"), SNM_LIVECFG_CLEAR_DESC_MSG);
				break;
			case COL_TR:
				hMenu = CreatePopupMenu();
				for (int i=1; i <= GetNumTracks(); i++)
				{
					char str[SNM_MAX_TRACK_NAME_LEN] = "";
					char* name = (char*)GetSetMediaTrackInfo(CSurf_TrackFromID(i,false), "P_NAME", NULL);
					_snprintfSafe(str, sizeof(str), "[%d] \"%s\"", i, name?name:"");
					AddToMenu(hMenu, str, SNM_LIVECFG_SET_TRACK_START_MSG + i);
				}
				AddToMenu(hMenu, SWS_SEPARATOR, 0);
				AddToMenu(hMenu, __LOCALIZE("Clear tracks","sws_DLG_155"), SNM_LIVECFG_CLEAR_TRACK_MSG);
				break;
			case COL_TRT:
				hMenu = CreatePopupMenu();
				if (item->m_track) {
					if (item->m_fxChain.GetLength())
						AddToMenu(hMenu, __LOCALIZE("(FX Chain overrides)","sws_DLG_155"), 0, -1, false, MFS_GRAYED);
					else {
						AddToMenu(hMenu, __LOCALIZE("Load track template...","sws_DLG_155"), SNM_LIVECFG_LOAD_TRACK_TEMPLATE_MSG);
						AddToMenu(hMenu, __LOCALIZE("Clear track templates","sws_DLG_155"), SNM_LIVECFG_CLEAR_TRACK_TEMPLATE_MSG);
					}
				}
				else AddToMenu(hMenu, __LOCALIZE("(No track)","sws_DLG_155"), 0, -1, false, MF_GRAYED);
				break;
			case COL_FXC:
				hMenu = CreatePopupMenu();
				if (item->m_track) {
					if (!item->m_trTemplate.GetLength()) {
						AddToMenu(hMenu, __LOCALIZE("Load FX Chain...","sws_DLG_155"), SNM_LIVECFG_LOAD_FXCHAIN_MSG);
						AddToMenu(hMenu, __LOCALIZE("Clear FX Chains","sws_DLG_155"), SNM_LIVECFG_CLEAR_FXCHAIN_MSG);
					}
					else AddToMenu(hMenu, __LOCALIZE("(Track template overrides)","sws_DLG_155"), 0, -1, false, MF_GRAYED);
				}
				else AddToMenu(hMenu, __LOCALIZE("(No track)","sws_DLG_155"), 0, -1, false, MF_GRAYED);
				break;
			case COL_PRESET:
				hMenu = CreatePopupMenu();
				if (item->m_track)
				{
					if (item->m_trTemplate.GetLength() && !item->m_fxChain.GetLength())
						AddToMenu(hMenu, __LOCALIZE("(Track template overrides)","sws_DLG_155"), 0, -1, false, MFS_GRAYED);
					else if (item->m_fxChain.GetLength())
						AddToMenu(hMenu, __LOCALIZE("(FX Chain overrides)","sws_DLG_155"), 0, -1, false, MFS_GRAYED);
					else {
						AddFXSubMenu(hMenu, item->m_track, &item->m_presets);
						AddToMenu(hMenu, SWS_SEPARATOR, 0);
						AddToMenu(hMenu, __LOCALIZE("Clear presets","sws_DLG_155"), SNM_LIVECFG_CLEAR_PRESETS_MSG);
					}
				}
				else AddToMenu(hMenu, __LOCALIZE("(No track)","sws_DLG_155"), 0, -1, false, MFS_GRAYED);
				break;
			case COL_ACTION_ON:
			{
				hMenu = CreatePopupMenu();
				AddToMenu(hMenu, __LOCALIZE("Learn from Actions window","sws_DLG_155"), SNM_LIVECFG_LEARN_ON_ACTION_MSG);
				AddToMenu(hMenu, __LOCALIZE("Clear actions/macros","sws_DLG_155"), SNM_LIVECFG_CLEAR_ON_ACTION_MSG);
				break;
			}
			case COL_ACTION_OFF:
			{
				hMenu = CreatePopupMenu();
				AddToMenu(hMenu, __LOCALIZE("Learn from Actions window","sws_DLG_155"), SNM_LIVECFG_LEARN_OFF_ACTION_MSG);
				AddToMenu(hMenu, __LOCALIZE("Clear actions/macros","sws_DLG_155"), SNM_LIVECFG_CLEAR_OFF_ACTION_MSG);
				break;
			}
		}
	}
	return hMenu;
}

int SNM_LiveConfigsWnd::OnKey(MSG* _msg, int _iKeyState) 
{
	if (_msg->message == WM_KEYDOWN && !_iKeyState)
	{
		switch(_msg->wParam)
		{
			case VK_F2:
				OnCommand(SNM_LIVECFG_EDIT_DESC_MSG, 0);
				return 1;
			case VK_RETURN:
				OnCommand(SNM_LIVECFG_PERFORM_MSG, 0);
				return 1;
			case VK_DELETE:
				OnCommand(SNM_LIVECFG_CLEAR_CC_ROW_MSG, 0);
				return 1;
			case VK_INSERT:
				if (Insert())
					return 1;
				break;
			}
	}
	return 0;
}

void SNM_LiveConfigsWnd::DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight)
{
	if (!g_liveConfigs.Get()->Get(g_configId))
		return;

	int x0=_r->left+10, h=35;
	if (_tooltipHeight)
		*_tooltipHeight = h;
	
	LICE_CachedFont* font = SNM_GetThemeFont();

	m_txtConfig.SetFont(font);
	if (!SNM_AutoVWndPosition(&m_txtConfig, NULL, _r, &x0, _r->top, h, 5))
		return;

	m_cbConfig.SetFont(font);
	if (!SNM_AutoVWndPosition(&m_cbConfig, &m_txtConfig, _r, &x0, _r->top, h))
		return;

	m_btnEnable.SetCheckState(g_liveConfigs.Get()->Get(g_configId)->m_enable);
	m_btnEnable.SetTextLabel(__LOCALIZE("Enable","sws_DLG_155"), -1, font);
	if (!SNM_AutoVWndPosition(&m_btnEnable, NULL, _r, &x0, _r->top, h, 5))
		return;

	m_btnMuteOthers.SetCheckState(g_liveConfigs.Get()->Get(g_configId)->m_muteOthers);
	m_btnMuteOthers.SetTextLabel(__LOCALIZE("Mute all but active track","sws_DLG_155"), -1, font);
	if (!SNM_AutoVWndPosition(&m_btnMuteOthers, NULL, _r, &x0, _r->top, h, 5))
		return;

	m_btnAutoSelect.SetCheckState(g_liveConfigs.Get()->Get(g_configId)->m_autoSelect);
	m_btnAutoSelect.SetTextLabel(__LOCALIZE("Unselect all but active track","sws_DLG_155"), -1, font);
	if (!SNM_AutoVWndPosition(&m_btnAutoSelect, NULL, _r, &x0, _r->top, h))
		return;

	m_txtInputTr.SetFont(font);
	if (!SNM_AutoVWndPosition(&m_txtInputTr, NULL, _r, &x0, _r->top, h, 5))
		return;

	m_cbInputTr.SetFont(font);
	int sel=0;
	for (int i=1; i <= GetNumTracks(); i++) {
		if (g_liveConfigs.Get()->Get(g_configId)->m_inputTr == CSurf_TrackFromID(i,false)) {
			sel = i;
			break;
		}
	}
	m_cbInputTr.SetCurSel(sel);
	if (!SNM_AutoVWndPosition(&m_cbInputTr, &m_txtInputTr, _r, &x0, _r->top, h))
		return;

#ifdef _SNM_MISC // wip
	ColorTheme* ct = SNM_GetColorTheme();
	LICE_pixel col = ct ? LICE_RGBA_FROMNATIVE(ct->main_text,255) : LICE_RGBA(255,255,255,255);
	m_knob.SetFGColors(col, col);
	if (!SNM_AutoVWndPosition(&m_knob, NULL, _r, &x0, _r->top, h))
		return;
#endif
	SNM_AddLogo(_bm, _r, x0, h);
}

HBRUSH SNM_LiveConfigsWnd::OnColorEdit(HWND _hwnd, HDC _hdc)
{
	if (_hwnd == GetDlgItem(m_hwnd, IDC_EDIT))
	{
		int bg, txt;
		SNM_GetThemeListColors(&bg, &txt); // not SNM_GetThemeEditColors (list's IDC_EDIT)
		SetBkColor(_hdc, bg);
		SetTextColor(_hdc, txt);
		return SNM_GetThemeBrush(bg);
	}
	return 0;
}

bool SNM_LiveConfigsWnd::Insert()
{
	if (WDL_PtrList<MidiLiveItem>* ccConfs = &g_liveConfigs.Get()->Get(g_configId)->m_ccConfs)
	{
		int pos=0;
		if (MidiLiveItem* item = (MidiLiveItem*)m_pLists.Get(0)->EnumSelected(&pos))
		{
			pos--;
			for(int i=pos; i < ccConfs->GetSize(); i++) ccConfs->Get(i)->m_cc++;
			ccConfs->Insert(pos, new MidiLiveItem(pos));
			item = ccConfs->Get(ccConfs->GetSize()-1);
			ccConfs->Delete(ccConfs->GetSize()-1, false); // do not delete now (still displayed..)
			Update();
			Undo_OnStateChangeEx(SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
			DELETE_NULL(item); // ok to del now that the update is done
			return true;
		}
	}
	return false;
}


///////////////////////////////////////////////////////////////////////////////

void SNM_LiveCfg_TLChangeSchedJob::Perform()
{
	// check or model consistency against the track list update
	for (int i=0; i < g_liveConfigs.Get()->GetSize(); i++)
	{
		MidiLiveConfig* lc = g_liveConfigs.Get()->Get(i);
		for (int j = 0; lc && j < lc->m_ccConfs.GetSize(); j++)
		{
			MidiLiveItem* item = lc->m_ccConfs.Get(j);
			if (item && item->m_track && CSurf_TrackToID(item->m_track, false) <= 0) // bad track (or master)
				item->m_track = NULL;
		}
		if (lc->m_inputTr && CSurf_TrackToID(g_liveConfigs.Get()->Get(g_configId)->m_inputTr, false) <= 0)
			lc->m_inputTr = NULL;

//		g_lastPerformedMIDIVal[i] = -1;
//		g_lastDeactivateCmd[i][0] = -1;
	}

	if (g_pLiveConfigsWnd) {
		g_pLiveConfigsWnd->FillComboInputTrack();
		g_pLiveConfigsWnd->Update();
	}
}


///////////////////////////////////////////////////////////////////////////////

static bool ProcessExtensionLine(const char *line, ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	LineParser lp(false);
	if (lp.parse(line) || lp.getnumtokens() < 2)
		return false;

	if (!strcmp(lp.gettoken_str(0), "<S&M_MIDI_LIVE"))
	{
		GUID g;	
		int configId = lp.gettoken_int(1) - 1;

		if (MidiLiveConfig* lc = g_liveConfigs.Get()->Get(configId))
		{
			int success;
			lc->m_enable = lp.gettoken_int(2);
			lc->m_version = lp.gettoken_int(3);
			lc->m_muteOthers = lp.gettoken_int(4);
			stringToGuid(lp.gettoken_str(5), &g);
			lc->m_inputTr = GuidToTrack(&g);
			lc->m_autoSelect = lp.gettoken_int(6, &success);
			if (!success) // for historical reasons..
				lc->m_autoSelect = 1;

			char linebuf[SNM_MAX_CHUNK_LINE_LENGTH];
			while(true)
			{
				if (!ctx->GetLine(linebuf,sizeof(linebuf)) && !lp.parse(linebuf))
				{
					if (lp.gettoken_str(0)[0] == '>')
						break;
					else if (MidiLiveItem* item = lc->m_ccConfs.Get(lp.gettoken_int(0)))
					{
						item->m_cc = lp.gettoken_int(0);
						item->m_desc.Set(lp.gettoken_str(1));
						stringToGuid(lp.gettoken_str(2), &g);
						item->m_track = GuidToTrack(&g);
						item->m_trTemplate.Set(lp.gettoken_str(3));
						item->m_fxChain.Set(lp.gettoken_str(4));
						item->m_presets.Set(lp.gettoken_str(5));
						item->m_onAction.Set(lp.gettoken_str(6));
						item->m_offAction.Set(lp.gettoken_str(7));
						
						// upgrade if needed
						if (lc->m_version < SNM_LIVECFG_VERSION)
							UpgradePresetConfV1toV2(item->m_track, lp.gettoken_str(5), &item->m_presets);
					}
				}
				else
					break;
			}
		}
		if (g_pLiveConfigsWnd)
			g_pLiveConfigsWnd->Update();
		return true;
	}
	return false;
}

static void SaveExtensionConfig(ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	WDL_FastString escStrs[6];
	char strId[128] = "";
	GUID g; 
	bool firstCfg = true; // will avoid useless data in RPP files
	for (int i=0; i < g_liveConfigs.Get()->GetSize(); i++)
	{
		MidiLiveConfig* lc = g_liveConfigs.Get()->Get(i);
		for (int j=0; lc && j < lc->m_ccConfs.GetSize(); j++)
		{
			MidiLiveItem* item = lc->m_ccConfs.Get(j);
			if (item && !item->IsDefault())
			{
				if (firstCfg)
				{
					firstCfg = false;

					if (lc->m_inputTr && CSurf_TrackToID(lc->m_inputTr, false) > 0) 
						g = *(GUID*)GetSetMediaTrackInfo(lc->m_inputTr, "GUID", NULL);
					else 
						g = GUID_NULL;
					guidToString(&g, strId);
					ctx->AddLine("<S&M_MIDI_LIVE %d %d %d %d %s %d", 
						i+1,
						lc->m_enable,
						SNM_LIVECFG_VERSION,
						lc->m_muteOthers,
						strId,
						lc->m_autoSelect);
				}

				if (item->m_track && CSurf_TrackToID(item->m_track, false) > 0) 
					g = *(GUID*)GetSetMediaTrackInfo(item->m_track, "GUID", NULL);
				else 
					g = GUID_NULL;
				guidToString(&g, strId);

				// make sure strings containing " ' etc.. will be properly saved
				makeEscapedConfigString(item->m_desc.Get(), &escStrs[0]);
				makeEscapedConfigString(item->m_trTemplate.Get(), &escStrs[1]);
				makeEscapedConfigString(item->m_fxChain.Get(), &escStrs[2]);
				makeEscapedConfigString(item->m_presets.Get(), &escStrs[3]);
				makeEscapedConfigString(item->m_onAction.Get(), &escStrs[4]);
				makeEscapedConfigString(item->m_offAction.Get(), &escStrs[5]);

				ctx->AddLine("%d %s %s %s %s %s %s %s", 
					item->m_cc, 
					escStrs[0].Get(), 
					strId, 
					escStrs[1].Get(),
					escStrs[2].Get(), 
					escStrs[3].Get(), 
					escStrs[4].Get(), 
					escStrs[5].Get());
			}
		}
		if (!firstCfg)
			ctx->AddLine(">");
		firstCfg = true;
	}
}

static void BeginLoadProjectState(bool isUndo, struct project_config_extension_t *reg)
{
	g_liveConfigs.Cleanup();
	g_liveConfigs.Get()->Empty(true);
	for (int i=0; i < SNM_LIVECFG_NB_CONFIGS; i++) 
		g_liveConfigs.Get()->Add(new MidiLiveConfig());
}

static project_config_extension_t g_projectconfig = {
	ProcessExtensionLine, SaveExtensionConfig, BeginLoadProjectState, NULL
};

int LiveConfigViewInit() {
	g_pLiveConfigsWnd = new SNM_LiveConfigsWnd();
	if (!g_pLiveConfigsWnd || !plugin_register("projectconfig", &g_projectconfig))
		return 0;
	return 1;
}

void LiveConfigViewExit() {
	DELETE_NULL(g_pLiveConfigsWnd);
}

void OpenLiveConfigView(COMMAND_T*) {
	if (g_pLiveConfigsWnd)
		g_pLiveConfigsWnd->Show(true, true);
}

bool IsLiveConfigViewDisplayed(COMMAND_T*) {
	return (g_pLiveConfigsWnd && g_pLiveConfigsWnd->IsValidWindow());
}


///////////////////////////////////////////////////////////////////////////////
// Live Configs commands/processing
///////////////////////////////////////////////////////////////////////////////

// val/valhw are used for midi stuff.
// val=[0..127] and valhw=-1 (midi CC),
// valhw >=0 (midi pitch (valhw | val<<7)),
// relmode absolute (0) or 1/2/3 for relative adjust modes
void ApplyLiveConfig(int _cfgId, int _val, int _valhw, int _relmode, HWND _hwnd) 
{
	// asolute CC only
	if (!_relmode && _valhw < 0)
	{
		// scheduled jobs ids == config id!
		// scheduled jobs ids [0..7] are reserved for Live Configs MIDI CC actions
		SNM_MidiLiveScheduledJob* job = 
			new SNM_MidiLiveScheduledJob(_cfgId, g_approxDelayMsCC, _cfgId, _val, _valhw, _relmode, _hwnd);

		// delay: avoid to stuck REPAER when a bunch of CCs are received (which is the standard case with 
		// HW knobs, faders, ..) but just process the last "stable" one
		if (g_approxDelayMsCC) {
			AddOrReplaceScheduledJob(job);
		}
		// immediate
		else {
			job->Perform();
			delete job;
		}
	}
}

void ApplyLiveConfig(MIDI_COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd) {
	ApplyLiveConfig((int)_ct->user, _val, _valhw, _relmode, _hwnd);
}

void NextLiveConfig(COMMAND_T* _ct)
{
	int cfgId = (int)_ct->user;
	if (MidiLiveConfig* lc = g_liveConfigs.Get()->Get(cfgId))
		if (lc->m_lastMIDIVal < 128)
			ApplyLiveConfig(cfgId, lc->m_lastMIDIVal+1, -1, 0, GetMainHwnd());
}

void PreviousLiveConfig(COMMAND_T* _ct)
{
	int cfgId = (int)_ct->user;
	if (MidiLiveConfig* lc = g_liveConfigs.Get()->Get(cfgId))
		if (lc->m_lastMIDIVal > 0)
			ApplyLiveConfig(cfgId, lc->m_lastMIDIVal-1, -1, 0, GetMainHwnd());
}

// the meat!
void SNM_MidiLiveScheduledJob::Perform()
{
	MidiLiveConfig* lc = g_liveConfigs.Get()->Get(m_cfgId);
	if (!lc || m_val == lc->m_lastMIDIVal) 
		return;
	else
		lc->m_lastMIDIVal = m_val;

	MidiLiveItem* cfg = lc->m_ccConfs.Get(m_val);
	if (cfg && !cfg->IsDefault())
	{
		if (lc->m_enable)
		{
			// note: a same track can be present in several rows
			WDL_PtrList<MediaTrack> otherConfigTracks;
			for (int i=0; i < lc->m_ccConfs.GetSize(); i++) {
				MidiLiveItem* cfgOther = lc->m_ccConfs.Get(i);
				if (cfgOther && cfgOther->m_track && cfgOther->m_track != cfg->m_track && otherConfigTracks.Find(cfgOther->m_track) == -1)
					otherConfigTracks.Add(cfgOther->m_track);
			}

			// save selected tracks, unselect all tracks
			WDL_PtrList<MediaTrack> selTracks;
			if (lc->m_autoSelect)
				for (int i=0; i <= GetNumTracks(); i++) // incl. master
				{
					MediaTrack* tr = CSurf_TrackFromID(i, false);
					if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL)) {
						if (tr != cfg->m_track && otherConfigTracks.Find(tr) == -1) 
							selTracks.Add(tr);
						GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i0);	
					}
				}

			// run desactivate action (of the previous config)
			// if auto-selection is enabled, only the active track is selected while 
			// performing the action. other tracks selections are restored later
			if (lc->m_lastDeactivateCmd[0] > 0)
			{
				if (lc->m_lastActivatedTr && lc->m_autoSelect)
					GetSetMediaTrackInfo(lc->m_lastActivatedTr, "I_SELECTED", &g_i1); 
				if (!KBD_OnMainActionEx(lc->m_lastDeactivateCmd[0], lc->m_lastDeactivateCmd[1], lc->m_lastDeactivateCmd[2], lc->m_lastDeactivateCmd[3], GetMainHwnd(), NULL))
					Main_OnCommand(lc->m_lastDeactivateCmd[0],0);
				if (lc->m_lastActivatedTr && lc->m_autoSelect)
					GetSetMediaTrackInfo(lc->m_lastActivatedTr, "I_SELECTED", &g_i0); // no track selected after that..
				lc->m_lastDeactivateCmd[0] = -1;
			}

			// send all notes-off on the deactivated track (if any)
			if (lc->m_lastActivatedTr)
				CC123Track(lc->m_lastActivatedTr);

			if (cfg->m_track)
			{
				// avoid glitches AFAP: in any case (even if the option "mute all but active" is enabled), we mute!
				// we'll restore mute state of the focused track after processing
				DWORD muteTime = 0;
				bool mute = *(bool*)GetSetMediaTrackInfo(cfg->m_track, "B_MUTE", NULL);
				if (!mute) {
					GetSetMediaTrackInfo(cfg->m_track, "B_MUTE", &g_bTrue);
					muteTime = GetTickCount();
				}

				// mute all other tracks of that config
				if (lc->m_muteOthers)
					for (int i=0; i < otherConfigTracks.GetSize(); i++) {
						MediaTrack* cfgOtherTr = otherConfigTracks.Get(i);
						if (cfgOtherTr) {
							GetSetMediaTrackInfo(cfgOtherTr, "B_MUTE", &g_bTrue);
							MuteReceives(lc->m_inputTr, cfgOtherTr, true);
						}
					}

				// track configuration
				SNM_ChunkParserPatcher* p = NULL;
				if (cfg->m_trTemplate.GetLength())
				{
					p = new SNM_SendPatcher(cfg->m_track);
					WDL_FastString tmplt;
					char fn[BUFFER_SIZE] = "";
					GetFullResourcePath("TrackTemplates", cfg->m_trTemplate.Get(), fn, BUFFER_SIZE);
					if (LoadChunk(fn, &tmplt) && tmplt.GetLength())
					{
						WDL_FastString chunk;
						MakeSingleTrackTemplateChunk(&tmplt, &chunk, true, true, false);
						ApplyTrackTemplate(cfg->m_track, &chunk, false, false, (SNM_SendPatcher*)p);
					}
				}
				else if (cfg->m_fxChain.GetLength())
				{
					p = new SNM_FXChainTrackPatcher(cfg->m_track);
					WDL_FastString chunk;
					char filename[BUFFER_SIZE];
					GetFullResourcePath("FXChains", cfg->m_fxChain.Get(), filename, BUFFER_SIZE);
					if (LoadChunk(filename, &chunk) && chunk.GetLength())
						((SNM_FXChainTrackPatcher*)p)->SetFXChain(&chunk);
				}
				else if (cfg->m_presets.GetLength()) {
					WaitForTrackMute(&muteTime); // 0 is ignored
					TriggerFXPresets(cfg->m_track, &(cfg->m_presets));
				}

				WaitForTrackMute(&muteTime); // 0 is ignored
				if (p) delete p; // + auto commit, if needed

				// select track (the only one selected at this point, if auto-selection is enabled)
				if (lc->m_autoSelect)
					GetSetMediaTrackInfo(cfg->m_track, "I_SELECTED", &g_i1);

				//	unmute the config track (or restore its mute state)			
				if (lc->m_muteOthers) {
					GetSetMediaTrackInfo(cfg->m_track, "B_MUTE", &g_bFalse);
					MuteReceives(lc->m_inputTr, cfg->m_track, false);
				}
				else
					GetSetMediaTrackInfo(cfg->m_track, "B_MUTE", &mute);

			} // end of track processing

			// perform activate action
			// if auto-selection is used, only the related track is selected at this point
			if (cfg->m_onAction.GetLength()) {
				int cmd = NamedCommandLookup(cfg->m_onAction.Get());
				if (cmd > 0 && !KBD_OnMainActionEx(cmd, m_val, m_valhw, m_relmode, GetMainHwnd(), NULL))
					Main_OnCommand(cmd,0);
			}

			// restore other (i.e. not part of this config) selected tracks
			if (lc->m_autoSelect)
				for (int k=0; k < selTracks.GetSize(); k++)
					GetSetMediaTrackInfo(selTracks.Get(k), "I_SELECTED", &g_i1);

			// prepare desactivate action
			lc->m_lastDeactivateCmd[0] = -1;
			if (cfg->m_offAction.GetLength()) {
				int cmd = NamedCommandLookup(cfg->m_offAction.Get());
				if (cmd > 0) {
					lc->m_lastDeactivateCmd[0] = cmd;
					lc->m_lastDeactivateCmd[1] = m_val;
					lc->m_lastDeactivateCmd[2] = m_valhw;
					lc->m_lastDeactivateCmd[3] = m_relmode;
				}
			}

			// store the last activated track (might be NULL)
			lc->m_lastActivatedTr = cfg->m_track;

		} // if (lc->m_enable)

	} // if (cfg && !cfg->IsDefault())

	if (g_pLiveConfigsWnd)
		g_pLiveConfigsWnd->SelectByCCValue(m_cfgId, m_val);
}

void ToggleEnableLiveConfig(COMMAND_T* _ct) {
	int cfgId = (int)_ct->user;
	g_liveConfigs.Get()->Get(cfgId)->m_enable = !(g_liveConfigs.Get()->Get(cfgId)->m_enable);
	if (g_pLiveConfigsWnd && (g_configId == cfgId))
		g_pLiveConfigsWnd->Update();
}

bool IsLiveConfigEnabled(COMMAND_T* _ct) {
	return (g_liveConfigs.Get()->Get((int)_ct->user)->m_enable == 1);
}
