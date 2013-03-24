/******************************************************************************
/ SnM_LiveConfigs.cpp
/
/ Copyright (c) 2010-2013 Jeffos
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

#include "stdafx.h"
#include "SnM.h"
#include "../reaper/localize.h"
#include "../Prompt.h"


#define SNM_LIVECFG_VERSION						2

//#define SNM_LIVECFG_UNUSED					0xF001 
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
#define SNM_LIVECFG_MUTE_OTHERS_MSG				0xF011
#define SNM_LIVECFG_SELSCROLL_MSG				0xF012
#define SNM_LIVECFG_OFFLINE_OTHERS_MSG			0xF013
#define SNM_LIVECFG_CC123_MSG					0xF014
#define SNM_LIVECFG_IGNORE_EMPTY_MSG			0xF015
#define SNM_LIVECFG_AUTOSENDS_MSG				0xF016
#define SNM_LIVECFG_LEARN_APPLY_MSG				0xF017
#define SNM_LIVECFG_LEARN_PRELOAD_MSG			0xF018
#define SNM_LIVECFG_HELP_MSG					0xF019
#define SNM_LIVECFG_APPLY_MSG					0xF01A
#define SNM_LIVECFG_PRELOAD_MSG					0xF01B
#define SNM_LIVECFG_SHOW_FX_MSG					0xF01C
#define SNM_LIVECFG_SHOW_IO_MSG					0xF01D
#define SNM_LIVECFG_SHOW_FX_INPUT_MSG			0xF01E
#define SNM_LIVECFG_SHOW_IO_INPUT_MSG			0xF01F
#define SNM_LIVECFG_INS_UP_MSG					0xF020
#define SNM_LIVECFG_INS_DOWN_MSG				0xF021
#define SNM_LIVECFG_CUT_MSG						0xF022
#define SNM_LIVECFG_COPY_MSG					0xF023
#define SNM_LIVECFG_PASTE_MSG					0xF024
#define SNM_LIVECFG_CREATE_INPUT_MSG			0xF025
#define SNM_LIVECFG_LEARN_PRESETS_START_MSG		0xF100 // 255 fx learn max -->
#define SNM_LIVECFG_LEARN_PRESETS_END_MSG		0xF1FF // <--
#define SNM_LIVECFG_SET_TRACK_START_MSG			0xF200 // 255 track max -->
#define SNM_LIVECFG_SET_TRACK_END_MSG			0xF2FF // <--
#define SNM_LIVECFG_SET_PRESET_START_MSG		0xF300 // 3327 preset max -->
#define SNM_LIVECFG_SET_PRESET_END_MSG			0xFFFF // <--

#define SNM_LIVECFG_MAX_CC_DELAY				3000
#define SNM_LIVECFG_DEF_CC_DELAY				500
#define SNM_LIVECFG_MAX_FADE					100
#define SNM_LIVECFG_DEF_FADE					25

#define SNM_LIVECFG_MAX_PRESET_COUNT			(SNM_LIVECFG_SET_PRESET_END_MSG - SNM_LIVECFG_SET_PRESET_START_MSG)
#define SNM_LIVECFG_UNDO_STR					__LOCALIZE("Live Configs edition", "sws_undo")

#define SNM_APPLY_MASK							1
#define SNM_PRELOAD_MASK						2

#define SNM_APPLY_PRELOAD_HWND					NULL


enum {
  BTNID_ENABLE=2000, //JFB would be great to have _APS_NEXT_CONTROL_VALUE *always* defined
  CMBID_CONFIG,
  TXTID_INPUT_TRACK,
  CMBID_INPUT_TRACK,
  BTNID_LEARN,
  BTNID_OPTIONS,
  BTNID_MON,
  WNDID_CC_DELAY,
  KNBID_CC_DELAY,
  WNDID_FADE,
  KNBID_FADE
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


SNM_LiveConfigsWnd* g_pLiveConfigsWnd = NULL; // editor window
SNM_LiveConfigMonitorWnd* g_monitorWnds[SNM_LIVECFG_NB_CONFIGS]; // monitoring windows
SWSProjConfig<WDL_PtrList_DeleteOnDestroy<LiveConfig> > g_liveConfigs;
WDL_PtrList_DeleteOnDestroy<LiveConfigItem> g_clipboardConfigs; // for cut/copy/paste
int g_configId = 0; // the current *displayed/edited* config id

// prefs
char g_lcBigFontName[64] = SNM_DYN_FONT_NAME;


///////////////////////////////////////////////////////////////////////////////
// Presets helpers
///////////////////////////////////////////////////////////////////////////////

// updates a preset conf string
// _presetConf: in/out param
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

bool GetPresetConf(int _fx, const char* _presetConf, WDL_FastString* _presetName)
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

int GetPresetFromConfTokenV1(const char* _preset) {
	if (const char* p = strchr(_preset, '.'))
		return atoi(p+1);
	return 0;
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

				WDL_PtrList_DeleteOnDestroy<WDL_FastString> names;
				int presetCount = preset>=0 ? GetUserPresetNames(_tr, fx, &names) : 0;
				if (presetCount && preset>=0 && preset<presetCount) {
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
	if (int nbFx = ((_tr && _presetConf && _presetConf->GetLength()) ? TrackFX_GetCount(_tr) : 0))
	{
		WDL_FastString presetName;
		for (int i=0; i < nbFx; i++)
			if (GetPresetConf(i, _presetConf->Get(), &presetName))
				updated |= TrackFX_SetPreset(_tr, i, presetName.Get());
	}
	return updated;
}


///////////////////////////////////////////////////////////////////////////////
// LiveConfigItem
///////////////////////////////////////////////////////////////////////////////

LiveConfigItem::LiveConfigItem(int _cc, const char* _desc, MediaTrack* _track, 
	const char* _trTemplate, const char* _fxChain, const char* _presets, 
	const char* _onAction, const char* _offAction)
	: m_cc(_cc),m_desc(_desc),m_track(_track),
	m_trTemplate(_trTemplate),m_fxChain(_fxChain),m_presets(_presets),
	m_onAction(_onAction),m_offAction(_offAction) 
{
}

LiveConfigItem::LiveConfigItem(LiveConfigItem* _item)
{
	Paste(_item);
	m_cc = _item ? _item->m_cc : -1;
}

void LiveConfigItem::Paste(LiveConfigItem* _item)
{
	if (_item)
	{
		// paste everything except the cc value
//		m_cc = _item->m_cc;
		m_desc.Set(_item->m_desc.Get());
		m_track = _item->m_track;
		m_trTemplate.Set(_item->m_trTemplate.Get());
		m_fxChain.Set(_item->m_fxChain.Get());
		m_presets.Set(_item->m_presets.Get());
		m_onAction.Set(_item->m_onAction.Get());
		m_offAction.Set(_item->m_offAction.Get());
	}
	else
		Clear();
}

bool LiveConfigItem::IsDefault(bool _ignoreComment) 
{ 
	return (
		!m_track &&
		(_ignoreComment || (!_ignoreComment && !m_desc.GetLength())) &&
		!m_trTemplate.GetLength() &&
		!m_fxChain.GetLength() &&
		!m_presets.GetLength() && 
		!m_onAction.GetLength() &&
		!m_offAction.GetLength());
}

void LiveConfigItem::Clear(bool _trDataOnly)
{
	m_track=NULL; 
	m_trTemplate.Set("");
	m_fxChain.Set("");
	m_presets.Set("");
	if (!_trDataOnly)
	{
		m_desc.Set("");
		m_onAction.Set("");
		m_offAction.Set("");
	}
}

bool LiveConfigItem::Equals(LiveConfigItem* _item, bool _ignoreComment)
{
	return (_item &&
		m_track == _item->m_track &&
		(_ignoreComment || (!_ignoreComment && !strcmp(m_desc.Get(), _item->m_desc.Get()))) &&
		!strcmp(m_trTemplate.Get(), _item->m_trTemplate.Get()) &&
		!strcmp(m_fxChain.Get(), _item->m_fxChain.Get()) &&
		!strcmp(m_presets.Get(), _item->m_presets.Get()) &&
		!strcmp(m_onAction.Get(), _item->m_onAction.Get()) &&
		!strcmp(m_offAction.Get(), _item->m_offAction.Get()));
}

void LiveConfigItem::GetInfo(WDL_FastString* _info)
{
	if (!_info) return;

	_info->Set("");
	if (!IsDefault(true))
	{
		if (m_desc.GetLength())
		{
			_info->Set(m_desc.Get());
			_info->Ellipsize(24, 24);
		}
		else
		{
			if (m_track)
			{
				char* name = (char*)GetSetMediaTrackInfo(m_track, "P_NAME", NULL);
				if (name && *name) _info->Set(name);
				else _info->SetFormatted(32, "[%d]", CSurf_TrackToID(m_track, false));
			}
			_info->Ellipsize(24, 24);

			WDL_FastString line2;
			if (m_desc.GetLength()) line2.Append(m_desc.Get());
			else if (m_trTemplate.GetLength()) line2.Append(m_trTemplate.Get());
			else if (m_fxChain.GetLength()) line2.Append(m_fxChain.Get());
			else if (m_presets.GetLength()) line2.Append(m_presets.Get());
			else if (m_onAction.GetLength()) line2.Append(kbd_getTextFromCmd(NamedCommandLookup(m_onAction.Get()), NULL));
			else if (m_offAction.GetLength()) line2.Append(kbd_getTextFromCmd(NamedCommandLookup(m_offAction.Get()), NULL));
			line2.Ellipsize(24, 24);

			if (_info->GetLength()) _info->Append("\n");
			_info->Append(line2.Get());
		}
	}
	else
		_info->Set(__LOCALIZE("<EMPTY>","sws_DLG_169")); // sws_DLG_169 = monitoring wnd
}


///////////////////////////////////////////////////////////////////////////////
// LiveConfig
///////////////////////////////////////////////////////////////////////////////

LiveConfig::LiveConfig()
{ 
	m_version = 0;
	m_ccDelay = SNM_LIVECFG_DEF_CC_DELAY;
	m_fade = SNM_LIVECFG_DEF_FADE;
	m_enable = m_cc123 = m_autoSends = m_selScroll = 1;
	m_ignoreEmpty = m_muteOthers = m_offlineOthers = 0;
	m_inputTr = NULL;
	m_activeMidiVal = m_preloadMidiVal = m_curMidiVal = m_curPreloadMidiVal = -1;
	for (int j=0; j<128; j++)
		m_ccConfs.Add(new LiveConfigItem(j, "", NULL, "", "", "", "", ""));
}

// undo points must be crated by callers, if needed
int LiveConfig::SetInputTrack(MediaTrack* _newInputTr, bool _updateSends)
{
	int nbSends = 0;
	if (_updateSends)
	{
		WDL_PtrList<void> doneTrs;

		// list used to factorize chunk state updates, auto-destroy => auto-commit when exiting!
		WDL_PtrList_DeleteOnDestroy<SNM_ChunkParserPatcher> ps;

		// remove sends of the current input track
		if (m_inputTr && CSurf_TrackToID(m_inputTr, false) > 0 && 
			m_inputTr != _newInputTr) // do not re-create sends (i.e. do not scratch the user's send tweaks!)
		{
			for (int i=0; i < m_ccConfs.GetSize(); i++)
				if (LiveConfigItem* cfg = m_ccConfs.Get(i))
					if (cfg->m_track && cfg->m_track != _newInputTr && doneTrs.Find(cfg->m_track)<0)
					{
						doneTrs.Add(cfg->m_track);
						SNM_SendPatcher* p = (SNM_SendPatcher*)SNM_FindCPPbyObject(&ps, cfg->m_track);
						if (!p) {
							p = new SNM_SendPatcher(cfg->m_track); 
							ps.Add(p);
						}
						p->RemoveReceivesFrom(m_inputTr);
					}
		}

		// add sends to the new input track
		if (_newInputTr && CSurf_TrackToID(_newInputTr, false) > 0 &&
			m_inputTr != _newInputTr) // do not re-create sends (i.e. do not scratch users' tweaks)
		{
			doneTrs.Empty(false);
			for (int i=0; i < m_ccConfs.GetSize(); i++)
				if (LiveConfigItem* cfg = m_ccConfs.Get(i))
					if (cfg->m_track &&
						cfg->m_track != _newInputTr && 
						cfg->m_track != m_inputTr && // exclude the "old" input track 
						doneTrs.Find(cfg->m_track)<0 && // have to check this (HasReceives() is not aware of chunk updates that are occuring)
						!HasReceives(_newInputTr, cfg->m_track))
					{
						doneTrs.Add(cfg->m_track);
						SNM_SendPatcher* p = (SNM_SendPatcher*)SNM_FindCPPbyObject(&ps, cfg->m_track);
						if (!p) {
							p = new SNM_SendPatcher(cfg->m_track); 
							ps.Add(p);
						}
						char vol[32] = "1.00000000000000";
						char pan[32] = "0.00000000000000";
						_snprintfSafe(vol, sizeof(vol), "%.14f", *(double*)GetConfigVar("defsendvol"));
						if (p->AddReceive(_newInputTr, *(int*)GetConfigVar("defsendflag")&0xFF, vol, pan))
							nbSends++;
					}
		}
	}
	m_inputTr = _newInputTr;

	if (g_pLiveConfigsWnd)
		g_pLiveConfigsWnd->FillComboInputTrack();

	return nbSends;
}

bool LiveConfig::IsLastConfiguredTrack(MediaTrack* _tr)
{
	int cnt = 0;
	if (_tr)
		for (int i=0; i < m_ccConfs.GetSize(); i++)
			if (LiveConfigItem* cfg = m_ccConfs.Get(i))
				if (cfg->m_track == _tr)
					if (++cnt>1)
						return false;
	return (cnt==1);
}


///////////////////////////////////////////////////////////////////////////////
// SNM_LiveConfigView
///////////////////////////////////////////////////////////////////////////////

// !WANT_LOCALIZE_STRINGS_BEGIN:sws_DLG_155
static SWS_LVColumn g_liveCfgListCols[] = { 
	{95,2,"OSC/CC value"}, {150,1,"Comment"}, {150,2,"Track"}, {175,2,"Track template"}, {175,2,"FX Chain"}, {150,2,"FX presets"}, {150,0,"Activate action"}, {150,0,"Deactivate action"}};
// !WANT_LOCALIZE_STRINGS_END

SNM_LiveConfigView::SNM_LiveConfigView(HWND hwndList, HWND hwndEdit)
:SWS_ListView(hwndList, hwndEdit, COL_COUNT, g_liveCfgListCols, "LiveConfigsViewState", false, "sws_DLG_155")
{
}

void SNM_LiveConfigView::GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax)
{
	if (str) *str = '\0';
	if (LiveConfigItem* pItem = (LiveConfigItem*)item)
	{
		switch (iCol)
		{
			case COL_CC:
			{
				LiveConfig* lc = g_liveConfigs.Get()->Get(g_configId);
				_snprintfSafe(str, iStrMax, "%03d %s", pItem->m_cc, 
					lc ? (pItem->m_cc==lc->m_activeMidiVal ? UTF8_BULLET : 
						(pItem->m_cc == lc->m_preloadMidiVal ? UTF8_CIRCLE : " ")) : " ");
				break;
			}
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
			case COL_ACTION_OFF: {
				const char* custId = (iCol==COL_ACTION_ON ? pItem->m_onAction.Get() : pItem->m_offAction.Get());
				if (*custId) {
					int cmd = NamedCommandLookup(custId);
					lstrcpyn(str, cmd>0 ? kbd_getTextFromCmd(cmd, NULL) : custId, iStrMax);
				}
				break;
			}
		}
	}
}

void SNM_LiveConfigView::SetItemText(SWS_ListItem* _item, int _iCol, const char* _str)
{
	if (LiveConfigItem* pItem = (LiveConfigItem*)_item)
	{
		switch (_iCol)
		{
			case COL_COMMENT:
				// limit the comment size (for RPP files)
				pItem->m_desc.Set(_str);
				pItem->m_desc.Ellipsize(32,32);
				Update();
				Undo_OnStateChangeEx2(NULL, SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
				break;
#ifdef _SNM_MISC // can't edit these by hand anymore..
			case COL_ACTION_ON:
			case COL_ACTION_OFF:
				if (*_str && !NamedCommandLookup(_str)) {
					MessageBox(GetParent(m_hwndList), __LOCALIZE("Error: this action ID (or custom ID) does not exist!","sws_DLG_155"), __LOCALIZE("S&M - Error","sws_DLG_155"), MB_OK);
					return;
				}
				if (_iCol == COL_ACTION_ON) pItem->m_onAction.Set(_str);
				else pItem->m_offAction.Set(_str);
				Update();
				Undo_OnStateChangeEx2(NULL, SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
				break;
#endif
		}
	}
}

void SNM_LiveConfigView::GetItemList(SWS_ListItemList* pList)
{
	if (LiveConfig* lc = g_liveConfigs.Get()->Get(g_configId))
		if (WDL_PtrList<LiveConfigItem>* ccConfs = &lc->m_ccConfs)
			for (int i=0; i < ccConfs->GetSize(); i++)
				pList->Add((SWS_ListItem*)ccConfs->Get(i));
}

// gotcha! several calls for one selection change (eg. 3 unselected rows + 1 new selection)
void SNM_LiveConfigView::OnItemSelChanged(SWS_ListItem* item, int iState)
{
	LiveConfig* lc = g_liveConfigs.Get()->Get(g_configId);
	if (lc && lc->m_selScroll)
	{
		LiveConfigItem* pItem = (LiveConfigItem*)item;
		if (pItem && pItem->m_track)
		{
			if ((iState & LVIS_SELECTED ? true : false) != (*(int*)GetSetMediaTrackInfo(pItem->m_track, "I_SELECTED", NULL) ? true : false))
				GetSetMediaTrackInfo(pItem->m_track, "I_SELECTED", iState & LVIS_SELECTED ? &g_i1 : &g_i0);
			if (iState & LVIS_SELECTED)
				ScrollTrack(pItem->m_track, true, true);
		}
	}
}

void SNM_LiveConfigView::OnItemDblClk(SWS_ListItem* item, int iCol)
{
	if (LiveConfigItem* pItem = (LiveConfigItem*)item)
	{
		switch (iCol)
		{
			case COL_TRT:
				if (g_pLiveConfigsWnd && pItem->m_track)
					g_pLiveConfigsWnd->OnCommand(SNM_LIVECFG_LOAD_TRACK_TEMPLATE_MSG, 0);
				break;
			case COL_FXC:
				if (g_pLiveConfigsWnd && pItem->m_track)
					g_pLiveConfigsWnd->OnCommand(SNM_LIVECFG_LOAD_FXCHAIN_MSG, 0);
				break;
			default:
				if (g_pLiveConfigsWnd)
					g_pLiveConfigsWnd->OnCommand(SNM_LIVECFG_APPLY_MSG, 0);
				break;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// SNM_LiveConfigsWnd
///////////////////////////////////////////////////////////////////////////////

SNM_LiveConfigsWnd::SNM_LiveConfigsWnd()
	: SWS_DockWnd(IDD_SNM_LIVE_CONFIGS, __LOCALIZE("Live Configs","sws_DLG_155"), "SnMLiveConfigs", SWSGetCommandID(OpenLiveConfig))
{
	// Must call SWS_DockWnd::Init() to restore parameters and open the window if necessary
	Init();
}

void SNM_LiveConfigsWnd::OnInitDlg()
{
	m_resize.init_item(IDC_LIST, 0.0, 0.0, 1.0, 1.0);
	m_pLists.Add(new SNM_LiveConfigView(GetDlgItem(m_hwnd, IDC_LIST), GetDlgItem(m_hwnd, IDC_EDIT)));

	m_vwnd_painter.SetGSC(WDL_STYLE_GetSysColor);
	m_parentVwnd.SetRealParent(m_hwnd);

	m_btnEnable.SetID(BTNID_ENABLE);
	m_btnEnable.SetTextLabel(__LOCALIZE("Config #","sws_DLG_155"));
	m_parentVwnd.AddChild(&m_btnEnable);

	m_cbConfig.SetID(CMBID_CONFIG);
	char cfg[4] = "";
	for (int i=0; i < SNM_LIVECFG_NB_CONFIGS; i++) {
		_snprintfSafe(cfg, sizeof(cfg), "%d", i+1);
		m_cbConfig.AddItem(cfg);
	}
	m_cbConfig.SetCurSel(g_configId);
	m_parentVwnd.AddChild(&m_cbConfig);

	m_txtInputTr.SetID(TXTID_INPUT_TRACK);
	m_txtInputTr.SetText(__LOCALIZE("Input track:","sws_DLG_155"));
	m_parentVwnd.AddChild(&m_txtInputTr);

	m_cbInputTr.SetID(CMBID_INPUT_TRACK);
	FillComboInputTrack();
	m_parentVwnd.AddChild(&m_cbInputTr);

	m_btnLearn.SetID(BTNID_LEARN);
	m_parentVwnd.AddChild(&m_btnLearn);

	m_btnOptions.SetID(BTNID_OPTIONS);
	m_parentVwnd.AddChild(&m_btnOptions);

	m_btnMonitor.SetID(BTNID_MON);
	m_parentVwnd.AddChild(&m_btnMonitor);

	m_knobCC.SetID(KNBID_CC_DELAY);
	m_knobCC.SetRange(0, SNM_LIVECFG_MAX_CC_DELAY, SNM_LIVECFG_DEF_CC_DELAY);
	m_vwndCC.AddChild(&m_knobCC);

	m_vwndCC.SetID(WNDID_CC_DELAY);
	m_vwndCC.SetTitle(__LOCALIZE("Switch delay:","sws_DLG_155"));
	m_vwndCC.SetSuffix(__LOCALIZE("ms","sws_DLG_155"));
	m_vwndCC.SetZeroText(__LOCALIZE("immediate","sws_DLG_155"));
	m_parentVwnd.AddChild(&m_vwndCC);

	m_knobFade.SetID(KNBID_FADE);
	m_knobFade.SetRangeFactor(0, SNM_LIVECFG_MAX_FADE, SNM_LIVECFG_DEF_FADE, 10.0);
	m_vwndFade.AddChild(&m_knobFade);

	m_vwndFade.SetID(WNDID_FADE);
	m_vwndFade.SetTitle(__LOCALIZE("Tiny fades:","sws_DLG_155"));
	m_vwndFade.SetSuffix(__LOCALIZE("ms","sws_DLG_155"));
	m_vwndFade.SetZeroText(__LOCALIZE("disabled","sws_DLG_155"));
	m_parentVwnd.AddChild(&m_vwndFade);

	Update();
}

void SNM_LiveConfigsWnd::OnDestroy() 
{
	m_cbConfig.Empty();
	m_cbInputTr.Empty();
	m_vwndCC.RemoveAllChildren(false);
	m_vwndFade.RemoveAllChildren(false);
}

// ScheduledJob because of multi-notifs
void SNM_LiveConfigsWnd::CSurfSetTrackListChange() {
	AddOrReplaceScheduledJob(new LiveConfigsUpdateJob());
}

void SNM_LiveConfigsWnd::CSurfSetTrackTitle() {
	FillComboInputTrack();
	Update();
}

void SNM_LiveConfigsWnd::FillComboInputTrack()
{
	m_cbInputTr.Empty();
	m_cbInputTr.AddItem(__LOCALIZE("None","sws_DLG_155"));
	for (int i=1; i <= GetNumTracks(); i++) // excl. master
	{
		WDL_FastString ellips;
		char* name = (char*)GetSetMediaTrackInfo(CSurf_TrackFromID(i,false), "P_NAME", NULL);
		ellips.SetFormatted(SNM_MAX_TRACK_NAME_LEN, "[%d] \"%s\"", i, name?name:"");
		ellips.Ellipsize(24, 24);
		m_cbInputTr.AddItem(ellips.Get());
	}

	int sel=0;
	if (LiveConfig* lc = g_liveConfigs.Get()->Get(g_configId))
		for (int i=1; i <= GetNumTracks(); i++) // excl. master
			if (lc->m_inputTr == CSurf_TrackFromID(i,false)) {
				sel = i;
				break;
			}
	m_cbInputTr.SetCurSel(sel);
}

// "by cc" in order to support sort
bool SNM_LiveConfigsWnd::SelectByCCValue(int _configId, int _cc, bool _selectOnly)
{
	if (_configId == g_configId)
	{
		SWS_ListView* lv = m_pLists.Get(0);
		HWND hList = lv ? lv->GetHWND() : NULL;
		if (lv && hList) // this can be called when the view is closed!
		{
			for (int i=0; i < lv->GetListItemCount(); i++)
			{
				LiveConfigItem* item = (LiveConfigItem*)lv->GetListItem(i);
				if (item && item->m_cc == _cc) 
				{
					if (_selectOnly)
						ListView_SetItemState(hList, -1, 0, LVIS_SELECTED);
					ListView_SetItemState(hList, i, LVIS_SELECTED, LVIS_SELECTED); 
					ListView_EnsureVisible(hList, i, true);
//					Update();
					return true;
				}
			}
		}
	}
	return false;
}

void SNM_LiveConfigsWnd::Update()
{
	if (m_pLists.GetSize())
		m_pLists.Get(0)->Update();
	if (LiveConfig* lc = g_liveConfigs.Get()->Get(g_configId)) {
		m_vwndCC.SetValue(lc->m_ccDelay);
		m_vwndFade.SetValue(lc->m_fade);
//		AddOrReplaceScheduledJob(new LiveConfigsUpdateFadeJob(lc->m_fade)); // so that it works for undo..
	}
	m_parentVwnd.RequestRedraw(NULL);
}

void SNM_LiveConfigsWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	LiveConfig* lc = g_liveConfigs.Get()->Get(g_configId);
	if (!lc)
		return;

	int x=0;
	LiveConfigItem* item = (LiveConfigItem*)m_pLists.Get(0)->EnumSelected(&x);

	switch (LOWORD(wParam))
	{
		case SNM_LIVECFG_HELP_MSG:
			ShellExecute(m_hwnd, "open", "http://reaper.mj-s.com/S&M_LiveConfigs.pdf" , NULL, NULL, SW_SHOWNORMAL);
			break;
		case SNM_LIVECFG_CREATE_INPUT_MSG:
		{
			char trName[SNM_MAX_TRACK_NAME_LEN] = "";
			if (PromptUserForString(GetMainHwnd(), __LOCALIZE("S&M - Create input track","sws_DLG_155"), trName, sizeof(trName), true))
			{
				if (PreventUIRefresh)
					PreventUIRefresh(1);

				InsertTrackAtIndex(GetNumTracks(), false);
				MediaTrack* inputTr = CSurf_TrackFromID(GetNumTracks(), false);
				if (inputTr)
				{
					GetSetMediaTrackInfo(inputTr, "P_NAME", trName);
					GetSetMediaTrackInfo(inputTr, "B_MAINSEND", &g_bFalse);
					GetSetMediaTrackInfo(inputTr, "I_RECMON", &g_i1);
					GetSetMediaTrackInfo(inputTr, "I_RECARM", &g_i1);
					SNM_SetSelectedTrack(NULL, inputTr, true);
					ScrollSelTrack(true, true);
				}

				int nbSends = lc->SetInputTrack(inputTr, true); // always obey (ignore lc->m_autoSends)
				Undo_OnStateChangeEx2(NULL, SNM_LIVECFG_UNDO_STR, UNDO_STATE_ALL, -1); // UNDO_STATE_ALL: possible routing updates above

				if (PreventUIRefresh)
					PreventUIRefresh(-1);
//				TrackList_AdjustWindows(false);
				RefreshRoutingsUI();

				char msg[256] = "";
				if (nbSends > 0) 
					_snprintfSafe(msg, sizeof(msg), __LOCALIZE_VERFMT("Created track \"%s\" (%d sends, record armed, monitoring enabled, no master sends).\nPlease select a MIDI or audio input!","sws_DLG_155"), trName, nbSends);
				else
					_snprintfSafe(msg, sizeof(msg), __LOCALIZE_VERFMT("Created track \"%s\" (record armed, monitoring enabled, no master sends).\nPlease select a MIDI or audio input!","sws_DLG_155"), trName);
				MessageBox(GetHWND(), msg, __LOCALIZE("S&M - Create input track","sws_DLG_155"), MB_OK);
			}
			break;
		}
		case SNM_LIVECFG_SHOW_FX_MSG:
			if (item && item->m_track) TrackFX_Show(item->m_track, 0, 1); // no undo
			break;
		case SNM_LIVECFG_SHOW_IO_MSG:
			if (item && item->m_track) ShowTrackRoutingWindow(item->m_track);
			break;
		case SNM_LIVECFG_SHOW_FX_INPUT_MSG:
			if (lc->m_inputTr) TrackFX_Show(lc->m_inputTr, 0, 1); // no undo
			break;
		case SNM_LIVECFG_SHOW_IO_INPUT_MSG:
			if (lc->m_inputTr) ShowTrackRoutingWindow(lc->m_inputTr);
			break;
		
		case SNM_LIVECFG_MUTE_OTHERS_MSG:
			// check native pref
			if (!lc->m_muteOthers) // about to activate?
				if (int* dontProcessMutedTrs = (int*)GetConfigVar("norunmute"))
					if (!*dontProcessMutedTrs)
					{
						// do not enable in the user's back:
						int r = MessageBox(GetHWND(),
							__LOCALIZE("The preference \"Do not process muted tracks\" is disabled.\nDo you want to enable it for CPU savings?","sws_DLG_155"),
							__LOCALIZE("S&M - Question","sws_DLG_155"),
							MB_YESNOCANCEL);
						if (r==IDYES) *dontProcessMutedTrs = 1;
						else if (r==IDCANCEL) break;
					}
			lc->m_muteOthers = !lc->m_muteOthers;
			Undo_OnStateChangeEx2(NULL, SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
			break;
		case SNM_LIVECFG_SELSCROLL_MSG:
			lc->m_selScroll = !lc->m_selScroll;
			Undo_OnStateChangeEx2(NULL, SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
			break;
		case SNM_LIVECFG_OFFLINE_OTHERS_MSG:
			if (!lc->m_offlineOthers || lc->m_offlineOthers == 2) lc->m_offlineOthers = 1;
			else lc->m_offlineOthers = 0;
			Undo_OnStateChangeEx2(NULL, SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
			break;
		case SNM_LIVECFG_CC123_MSG:
			lc->m_cc123 = !lc->m_cc123;
			Undo_OnStateChangeEx2(NULL, SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
			break;
		case SNM_LIVECFG_IGNORE_EMPTY_MSG:
			lc->m_ignoreEmpty = !lc->m_ignoreEmpty;
			Undo_OnStateChangeEx2(NULL, SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
			break;
		case SNM_LIVECFG_AUTOSENDS_MSG:
			lc->m_autoSends = !lc->m_autoSends;
			Undo_OnStateChangeEx2(NULL, SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
			break;

		case SNM_LIVECFG_INS_UP_MSG:
			Insert(-1);
			break;
		case SNM_LIVECFG_INS_DOWN_MSG:
			Insert(1);
			break;
		case SNM_LIVECFG_PASTE_MSG:
			if (item)
			{
				int sortCol = m_pLists.Get(0)->GetSortColumn();
				if (sortCol && sortCol!=1) {
					//JFB lazy here.. better than confusing the user..
					MessageBox(GetHWND(), __LOCALIZE("The list view must be sorted by ascending OSC/CC values!","sws_DLG_155"), __LOCALIZE("S&M - Error","sws_DLG_155"), MB_OK);
					break;
				}
				bool updt = false, firstSel = true;
				int destIdx = lc->m_ccConfs.Find(item);
				if (destIdx>=0)
					for (int i=0; i<g_clipboardConfigs.GetSize() && (destIdx+i)<lc->m_ccConfs.GetSize(); i++)
						if (item = lc->m_ccConfs.Get(destIdx+i))
							if (LiveConfigItem* pasteItem = g_clipboardConfigs.Get(i))
							{
								item->Paste(pasteItem); // copy everything except the cc value
								updt = true;
								if (SelectByCCValue(g_configId, item->m_cc, firstSel)) firstSel = false;
							}
				if (updt) {
					Update(); // preserve list view selection
					Undo_OnStateChangeEx2(NULL, SNM_LIVECFG_UNDO_STR, UNDO_STATE_ALL, -1); // UNDO_STATE_ALL: possible routing updates above
				}
			}
			break;
		case SNM_LIVECFG_CUT_MSG:
		case SNM_LIVECFG_COPY_MSG:
			if (item)
			{
				g_clipboardConfigs.Empty(true);
				LiveConfigItem* item2 = item; // let "item" as it is (code fall through below..)
				int x2 = x;
				while (item2) {
					g_clipboardConfigs.Add(new LiveConfigItem(item2));
					item2 = (LiveConfigItem*)m_pLists.Get(0)->EnumSelected(&x2);
				}
				if (LOWORD(wParam) != SNM_LIVECFG_CUT_MSG) // cut => fall through
					break;
			}
			else
				break;
		case SNM_LIVECFG_CLEAR_CC_ROW_MSG:
		{
			bool updt = false;
			while (item)
			{
				if (lc->m_autoSends && item->m_track && lc->m_inputTr && lc->IsLastConfiguredTrack(item->m_track))
					SNM_RemoveReceivesFrom(item->m_track, lc->m_inputTr);
				item->Clear();
				updt = true;
				item = (LiveConfigItem*)m_pLists.Get(0)->EnumSelected(&x);
			}
			if (updt) {
				Update();
				Undo_OnStateChangeEx2(NULL, SNM_LIVECFG_UNDO_STR, UNDO_STATE_ALL, -1); // UNDO_STATE_ALL: possible routing updates above
			}
			break;
		}
		case SNM_LIVECFG_EDIT_DESC_MSG:
			if (item) m_pLists.Get(0)->EditListItem((SWS_ListItem*)item, COL_COMMENT);
			break;
		case SNM_LIVECFG_CLEAR_DESC_MSG:
		{
			bool updt = false;
			while (item) {
				item->m_desc.Set("");
				updt = true;
				item = (LiveConfigItem*)m_pLists.Get(0)->EnumSelected(&x);
			}
			if (updt) {
				Update();
				Undo_OnStateChangeEx2(NULL, SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
			}
			break;
		}

		case SNM_LIVECFG_APPLY_MSG:
			if (item) ApplyLiveConfig(g_configId, item->m_cc, -1, 0, SNM_APPLY_PRELOAD_HWND, true); // immediate
			break;
		case SNM_LIVECFG_PRELOAD_MSG:
			if (item) PreloadLiveConfig(g_configId, item->m_cc, -1, 0, SNM_APPLY_PRELOAD_HWND, true); // immediate
			break;

		case SNM_LIVECFG_LOAD_TRACK_TEMPLATE_MSG:
		{
			bool updt = false;
			if (item)
			{
				char fn[SNM_MAX_PATH]="";
				if (BrowseResourcePath(__LOCALIZE("S&M - Load track template","sws_DLG_155"), "TrackTemplates", "REAPER Track Template (*.RTrackTemplate)\0*.RTrackTemplate\0", fn, sizeof(fn)))
				{
					while(item) {
						item->m_trTemplate.Set(fn);
						item->m_fxChain.Set("");
						item->m_presets.Set("");
						updt = true;
						item = (LiveConfigItem*)m_pLists.Get(0)->EnumSelected(&x);
					}
				}
			}
			if (updt) {
				Update();
				Undo_OnStateChangeEx2(NULL, SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
			}
			break;
		}
		case SNM_LIVECFG_CLEAR_TRACK_TEMPLATE_MSG:
		{
			bool updt = false;
			while (item) {
				item->m_trTemplate.Set("");
				updt = true;
				item = (LiveConfigItem*)m_pLists.Get(0)->EnumSelected(&x);
			}
			if (updt) {
				Update();
				Undo_OnStateChangeEx2(NULL, SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
			}
			break;
		}
		case SNM_LIVECFG_LOAD_FXCHAIN_MSG:
		{
			bool updt = false;
			if (item)
			{
				char fn[SNM_MAX_PATH]="";
				if (BrowseResourcePath(__LOCALIZE("S&M - Load FX Chain","sws_DLG_155"), "FXChains", "REAPER FX Chain (*.RfxChain)\0*.RfxChain\0", fn, sizeof(fn)))
				{
					while(item) {
						item->m_fxChain.Set(fn);
						item->m_trTemplate.Set("");
						item->m_presets.Set("");
						updt = true;
						item = (LiveConfigItem*)m_pLists.Get(0)->EnumSelected(&x);
					}
				}
			}
			if (updt) {
				Update();
				Undo_OnStateChangeEx2(NULL, SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
			}
			break;
		}
		case SNM_LIVECFG_CLEAR_FXCHAIN_MSG:
		{
			bool updt = false;
			while (item)
			{
				item->m_fxChain.Set("");
				updt = true;
				item = (LiveConfigItem*)m_pLists.Get(0)->EnumSelected(&x);
			}
			if (updt) {
				Update();
				Undo_OnStateChangeEx2(NULL, SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
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
			if (item && GetSelectedAction(idstr, SNM_MAX_ACTION_CUSTID_LEN, __localizeFunc("Main","accel_sec",0)))
			{
				bool updt = false;
				while (item)
				{
					if (LOWORD(wParam)==SNM_LIVECFG_LEARN_ON_ACTION_MSG)
						item->m_onAction.Set(idstr);
					else
						item->m_offAction.Set(idstr);
					updt = true;
					item = (LiveConfigItem*)m_pLists.Get(0)->EnumSelected(&x);
				}
				if (updt) {
					Update();
					Undo_OnStateChangeEx2(NULL, SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
				}
			}
			break;
		}
		case SNM_LIVECFG_CLEAR_ON_ACTION_MSG:
		case SNM_LIVECFG_CLEAR_OFF_ACTION_MSG:
		{
			bool updt = false;
			while (item)
			{
				if (LOWORD(wParam)==SNM_LIVECFG_CLEAR_ON_ACTION_MSG)
					item->m_onAction.Set("");
				else
					item->m_offAction.Set("");
				updt = true;
				item = (LiveConfigItem*)m_pLists.Get(0)->EnumSelected(&x);
			}
			if (updt) {
				Update();
				Undo_OnStateChangeEx2(NULL, SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
			}
			break;
		}
		case SNM_LIVECFG_CLEAR_TRACK_MSG:
		{
			bool updt = false;
			while (item)
			{
				if (lc->m_autoSends && item->m_track && lc->m_inputTr && lc->IsLastConfiguredTrack(item->m_track))
					SNM_RemoveReceivesFrom(item->m_track, lc->m_inputTr);
				if (item->m_track) item->Clear(true);
				else item->m_track = NULL;
				updt = true;
				item = (LiveConfigItem*)m_pLists.Get(0)->EnumSelected(&x);
			}
			if (updt) {
				Update();
				Undo_OnStateChangeEx2(NULL, SNM_LIVECFG_UNDO_STR, UNDO_STATE_ALL, -1);  // UNDO_STATE_ALL: possible routing updates above
			}
			break;
		}
		case SNM_LIVECFG_CLEAR_PRESETS_MSG:
		{
			bool updt = false;
			while (item)
			{
				item->m_presets.Set("");
				updt = true;
				item = (LiveConfigItem*)m_pLists.Get(0)->EnumSelected(&x);
			}
			if (updt) {
				Update();
				Undo_OnStateChangeEx2(NULL, SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
			}
			break;
		}
		case BTNID_ENABLE:
			if (!HIWORD(wParam) || HIWORD(wParam)==600)
				UpdateEnableLiveConfig(g_configId, -1);
			break;
		case SNM_LIVECFG_LEARN_APPLY_MSG:
			LearnAction(&g_SNM_Section, SNM_SECTION_1ST_CMD_ID + g_configId);
			break;
		case SNM_LIVECFG_LEARN_PRELOAD_MSG:
			LearnAction(&g_SNM_Section, SNM_SECTION_1ST_CMD_ID + SNM_LIVECFG_NB_CONFIGS + g_configId);
			break;
		case BTNID_LEARN:
		{
			RECT r; m_btnLearn.GetPositionInTopVWnd(&r);
			ClientToScreen(m_hwnd, (LPPOINT)&r);
			ClientToScreen(m_hwnd, ((LPPOINT)&r)+1);
			SendMessage(m_hwnd, WM_CONTEXTMENU, 0, MAKELPARAM((UINT)(r.left), (UINT)(r.bottom+SNM_1PIXEL_Y)));
			break;
		}
		case BTNID_OPTIONS:
		{
			RECT r; m_btnOptions.GetPositionInTopVWnd(&r);
			ClientToScreen(m_hwnd, (LPPOINT)&r);
			ClientToScreen(m_hwnd, ((LPPOINT)&r)+1);
			SendMessage(m_hwnd, WM_CONTEXTMENU, 0, MAKELPARAM((UINT)(r.left), (UINT)(r.bottom+SNM_1PIXEL_Y)));
			break;
		}
		case BTNID_MON:
			OpenLiveConfigMonitorWnd(g_configId);
			break;
		case CMBID_INPUT_TRACK:
			if (HIWORD(wParam)==CBN_SELCHANGE)
			{
				lc->SetInputTrack(m_cbInputTr.GetCurSel() ? CSurf_TrackFromID(m_cbInputTr.GetCurSel(), false) : NULL, lc->m_autoSends==1);
				Undo_OnStateChangeEx2(NULL, SNM_LIVECFG_UNDO_STR, UNDO_STATE_ALL, -1); // UNDO_STATE_ALL: SetInputTrack() might update the project
			}
			break;
		case CMBID_CONFIG:
			if (HIWORD(wParam)==CBN_SELCHANGE)
			{
				// stop cell editing (changing the list content would be ignored otherwise => dropdown box & list box unsynchronized)
				m_pLists.Get(0)->EditListItemEnd(false);
				g_configId = m_cbConfig.GetCurSel();
				FillComboInputTrack();
				Update();
			}
			break;

		default:
			if (LOWORD(wParam)>=SNM_LIVECFG_SET_TRACK_START_MSG && LOWORD(wParam)<=SNM_LIVECFG_SET_TRACK_END_MSG) 
			{
				bool updt = false;
				while (item)
				{
					item->m_track = CSurf_TrackFromID((int)LOWORD(wParam) - SNM_LIVECFG_SET_TRACK_START_MSG, false);
					if (!(item->m_track)) {
						item->m_fxChain.Set("");
						item->m_trTemplate.Set("");
					}
					item->m_presets.Set("");
					if (lc->m_autoSends && item->m_track && lc->m_inputTr && !HasReceives(lc->m_inputTr, item->m_track))
						SNM_AddReceive(lc->m_inputTr, item->m_track, -1);
					updt = true;
					item = (LiveConfigItem*)m_pLists.Get(0)->EnumSelected(&x);
				}
				if (updt) {
					Update();
					Undo_OnStateChangeEx2(NULL, SNM_LIVECFG_UNDO_STR, UNDO_STATE_ALL, -1); // UNDO_STATE_ALL: possible routing updates above
				}
			}
			else if (item && LOWORD(wParam)>=SNM_LIVECFG_LEARN_PRESETS_START_MSG && LOWORD(wParam)<=SNM_LIVECFG_LEARN_PRESETS_END_MSG) 
			{
				if (item->m_track)
				{
					int fx = (int)LOWORD(wParam)-SNM_LIVECFG_LEARN_PRESETS_START_MSG;
					char buf[SNM_MAX_PRESET_NAME_LEN] = "";
					TrackFX_GetPreset(item->m_track, fx, buf, SNM_MAX_PRESET_NAME_LEN);
					UpdatePresetConf(&item->m_presets, fx+1, buf);
					item->m_fxChain.Set("");
					item->m_trTemplate.Set("");
					Update();
					Undo_OnStateChangeEx2(NULL, SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
				}
				break;
			}
			else if (item && LOWORD(wParam) >= SNM_LIVECFG_SET_PRESET_START_MSG && LOWORD(wParam) <= SNM_LIVECFG_SET_PRESET_END_MSG) 
			{
				if (PresetMsg* msg = m_lastPresetMsg.Get((int)LOWORD(wParam) - SNM_LIVECFG_SET_PRESET_START_MSG))
				{
					UpdatePresetConf(&item->m_presets, msg->m_fx+1, msg->m_preset.Get());
					item->m_fxChain.Set("");
					item->m_trTemplate.Set("");
					Update();
					Undo_OnStateChangeEx2(NULL, SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
				}
			}
			else
				Main_OnCommand((int)wParam, (int)lParam);
			break;
	}
}

INT_PTR SNM_LiveConfigsWnd::OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg==WM_VSCROLL)
		if (LiveConfig* lc = g_liveConfigs.Get()->Get(g_configId))
			switch (lParam)
			{
				case KNBID_CC_DELAY:
					lc->m_ccDelay = m_knobCC.GetSliderPosition();
					m_vwndCC.SetValue(lc->m_ccDelay);
					AddOrReplaceScheduledJob(new UndoJob(SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG));
					break;
				case KNBID_FADE:
					lc->m_fade = m_knobFade.GetSliderPosition();
					m_vwndFade.SetValue(lc->m_fade);
					AddOrReplaceScheduledJob(new UndoJob(SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG));
					AddOrReplaceScheduledJob(new LiveConfigsUpdateFadeJob(lc->m_fade));
					break;
			}
	return 0;
}

void SNM_LiveConfigsWnd::AddPresetSubMenu(HMENU _menu, MediaTrack* _tr, WDL_FastString* _curPresetConf)
{
	m_lastPresetMsg.Empty(true);

	int fxCount = (_tr ? TrackFX_GetCount(_tr) : 0);
	if (!fxCount) {
		AddToMenu(_menu, __LOCALIZE("(No FX on track)","sws_DLG_155"), 0, -1, false, MF_GRAYED);
		return;
	}
	
	char fxName[SNM_MAX_FX_NAME_LEN] = "";
	int msgCpt = 0;
	for(int fx=0; fx<fxCount; fx++) 
	{
		if(TrackFX_GetFXName(_tr, fx, fxName, SNM_MAX_FX_NAME_LEN))
		{
			HMENU fxSubMenu = CreatePopupMenu();
			WDL_FastString str, curPresetName;
			GetPresetConf(fx, _curPresetConf->Get(), &curPresetName);

			// learn current preset
			if ((SNM_LIVECFG_LEARN_PRESETS_START_MSG + fx) <= SNM_LIVECFG_LEARN_PRESETS_END_MSG)
			{
				char buf[SNM_MAX_PRESET_NAME_LEN] = "";
				TrackFX_GetPreset(_tr, fx, buf, SNM_MAX_PRESET_NAME_LEN);
				str.SetFormatted(256, __LOCALIZE_VERFMT("Learn current preset \"%s\"","sws_DLG_155"), *buf?buf:__LOCALIZE("undefined","sws_DLG_155"));
				AddToMenu(fxSubMenu, str.Get(), SNM_LIVECFG_LEARN_PRESETS_START_MSG + fx, -1, false, *buf?0:MF_GRAYED);
			}

			if (GetMenuItemCount(fxSubMenu))
				AddToMenu(fxSubMenu, SWS_SEPARATOR, 0);

			// user preset list
			WDL_PtrList_DeleteOnDestroy<WDL_FastString> names;
			int presetCount = GetUserPresetNames(_tr, fx, &names);

			str.SetFormatted(256, __LOCALIZE_VERFMT("[User presets (.rpl)%s]","sws_DLG_155"), presetCount?"":__LOCALIZE(": no .rpl file loaded","sws_DLG_155"));
			AddToMenu(fxSubMenu, str.Get(), 0, -1, false, presetCount?MF_DISABLED:MF_GRAYED);

			for(int j=0; j<presetCount && (SNM_LIVECFG_SET_PRESET_START_MSG + msgCpt) <= SNM_LIVECFG_SET_PRESET_END_MSG; j++)
				if (names.Get(j)->GetLength()) {
					AddToMenu(fxSubMenu, names.Get(j)->Get(), SNM_LIVECFG_SET_PRESET_START_MSG + msgCpt, -1, false, !strcmp(curPresetName.Get(), names.Get(j)->Get()) ? MFS_CHECKED : MFS_UNCHECKED);
					m_lastPresetMsg.Add(new PresetMsg(fx, names.Get(j)->Get()));
					msgCpt++;
				}

			if (GetMenuItemCount(fxSubMenu))
				AddToMenu(fxSubMenu, SWS_SEPARATOR, 0);

			// clear current fx preset
			if ((SNM_LIVECFG_SET_PRESET_START_MSG + msgCpt) <= SNM_LIVECFG_SET_PRESET_END_MSG)
			{
				AddToMenu(fxSubMenu, __LOCALIZE("None","sws_DLG_155"), SNM_LIVECFG_SET_PRESET_START_MSG + msgCpt, -1, false, !curPresetName.GetLength() ? MFS_CHECKED : MFS_UNCHECKED);
				m_lastPresetMsg.Add(new PresetMsg(fx, ""));
				msgCpt++;
			}

			// add fx sub menu
			str.SetFormatted(512, "FX%d: %s", fx+1, fxName);
			AddSubMenu(_menu, fxSubMenu, str.Get(), -1, GetMenuItemCount(fxSubMenu) ? MFS_ENABLED : MF_GRAYED);
		}
	}
}

void SNM_LiveConfigsWnd::AddLearnMenu(HMENU _menu, bool _subItems)
{
	if (g_liveConfigs.Get()->Get(g_configId))
	{
		char buf[128] = "";
		HMENU hOptMenu;
		if (_subItems) hOptMenu = CreatePopupMenu();
		else hOptMenu = _menu;

		_snprintfSafe(buf, sizeof(buf), __LOCALIZE_VERFMT("Learn \"Apply Live Config %d\" action...","sws_DLG_155"), g_configId+1);
		AddToMenu(hOptMenu, buf, SNM_LIVECFG_LEARN_APPLY_MSG);
		_snprintfSafe(buf, sizeof(buf), __LOCALIZE_VERFMT("Learn \"Preload Live Config %d\" action...","sws_DLG_155"), g_configId+1);
		AddToMenu(hOptMenu, buf, SNM_LIVECFG_LEARN_PRELOAD_MSG);
		if (_subItems && GetMenuItemCount(hOptMenu))
			AddSubMenu(_menu, hOptMenu, __LOCALIZE("Learn","sws_DLG_155"));
	}
}

void SNM_LiveConfigsWnd::AddOptionsMenu(HMENU _menu, bool _subItems)
{
	if (LiveConfig* lc = g_liveConfigs.Get()->Get(g_configId))
	{
		HMENU hOptMenu;
		if (_subItems) hOptMenu = CreatePopupMenu();
		else hOptMenu = _menu;

		AddToMenu(hOptMenu, __LOCALIZE("Mute all but active track (CPU savings)","sws_DLG_155"), SNM_LIVECFG_MUTE_OTHERS_MSG, -1, false, lc->m_muteOthers ? MFS_CHECKED : MFS_UNCHECKED);
		AddToMenu(hOptMenu, __LOCALIZE("Offline all but active/preloaded tracks (RAM savings)","sws_DLG_155"), SNM_LIVECFG_OFFLINE_OTHERS_MSG, -1, false, lc->m_offlineOthers ? MFS_CHECKED : MFS_UNCHECKED);
		AddToMenu(hOptMenu, SWS_SEPARATOR, 0);
		AddToMenu(hOptMenu, __LOCALIZE("Ignore switches to empty configs","sws_DLG_155"), SNM_LIVECFG_IGNORE_EMPTY_MSG, -1, false, lc->m_ignoreEmpty ? MFS_CHECKED : MFS_UNCHECKED);
		AddToMenu(hOptMenu, __LOCALIZE("Send all notes off when switching configs","sws_DLG_155"), SNM_LIVECFG_CC123_MSG, -1, false, lc->m_cc123 ? MFS_CHECKED : MFS_UNCHECKED);
		AddToMenu(hOptMenu, SWS_SEPARATOR, 0);
		AddToMenu(hOptMenu, __LOCALIZE("Automatically update sends from the input track (if any)","sws_DLG_155"), SNM_LIVECFG_AUTOSENDS_MSG, -1, false, lc->m_autoSends ? MFS_CHECKED : MFS_UNCHECKED);
		AddToMenu(hOptMenu, __LOCALIZE("Select/scroll to track on list view click","sws_DLG_155"), SNM_LIVECFG_SELSCROLL_MSG, -1, false, lc->m_selScroll ? MFS_CHECKED : MFS_UNCHECKED);

		if (_subItems && GetMenuItemCount(hOptMenu))
			AddSubMenu(_menu, hOptMenu, __LOCALIZE("Options","sws_DLG_155"));
	}
}

HMENU SNM_LiveConfigsWnd::OnContextMenu(int x, int y, bool* wantDefaultItems)
{
	HMENU hMenu = CreatePopupMenu();

	// specific context menu for the option button
	POINT p; GetCursorPos(&p);
	ScreenToClient(m_hwnd, &p);
	if (WDL_VWnd* v = m_parentVwnd.VirtWndFromPoint(p.x,p.y,1))
		switch (v->GetID())
		{
			case BTNID_LEARN:
				*wantDefaultItems = false;
				AddLearnMenu(hMenu, false);
				return hMenu;
			case BTNID_OPTIONS:
				*wantDefaultItems = false;
				AddOptionsMenu(hMenu, false);
				return hMenu;
			case TXTID_INPUT_TRACK:
			case CMBID_INPUT_TRACK:
				if (LiveConfig* lc = g_liveConfigs.Get()->Get(g_configId))
				{
					*wantDefaultItems = false;
					AddToMenu(hMenu, __LOCALIZE("Show FX chain...","sws_DLG_155"), SNM_LIVECFG_SHOW_FX_INPUT_MSG, -1, false, lc->m_inputTr ? MF_ENABLED : MF_GRAYED);
					AddToMenu(hMenu, __LOCALIZE("Show routing window...","sws_DLG_155"), SNM_LIVECFG_SHOW_IO_INPUT_MSG, -1, false, lc->m_inputTr ? MF_ENABLED : MF_GRAYED);
					return hMenu;
				}
				break;
		}

	int iCol;
	if (LiveConfigItem* item = (LiveConfigItem*)m_pLists.Get(0)->GetHitItem(x, y, &iCol))
	{
		*wantDefaultItems = (iCol < 0);
		switch(iCol)
		{
			case COL_COMMENT:
				AddToMenu(hMenu, __LOCALIZE("Edit comment","sws_DLG_155"), SNM_LIVECFG_EDIT_DESC_MSG);
				AddToMenu(hMenu, __LOCALIZE("Clear comments","sws_DLG_155"), SNM_LIVECFG_CLEAR_DESC_MSG);
				break;
			case COL_TR:
				if (GetNumTracks() > 0)
				{
					HMENU hTracksMenu = CreatePopupMenu();
					for (int i=1; i <= GetNumTracks(); i++)
					{
						char str[SNM_MAX_TRACK_NAME_LEN] = "";
						char* name = (char*)GetSetMediaTrackInfo(CSurf_TrackFromID(i,false), "P_NAME", NULL);
						_snprintfSafe(str, sizeof(str), "[%d] \"%s\"", i, name?name:"");
						AddToMenu(hTracksMenu, str, SNM_LIVECFG_SET_TRACK_START_MSG + i);
					}
					AddSubMenu(hMenu, hTracksMenu, __LOCALIZE("Set tracks","sws_DLG_155"));
				}
				AddToMenu(hMenu, __LOCALIZE("Clear tracks","sws_DLG_155"), SNM_LIVECFG_CLEAR_TRACK_MSG);

				if (GetMenuItemCount(hMenu))
					AddToMenu(hMenu, SWS_SEPARATOR, 0);
				AddToMenu(hMenu, __LOCALIZE("Show FX chain...","sws_DLG_155"), SNM_LIVECFG_SHOW_FX_MSG, -1, false, item->m_track ? MF_ENABLED : MF_GRAYED);
				AddToMenu(hMenu, __LOCALIZE("Show routing window...","sws_DLG_155"), SNM_LIVECFG_SHOW_IO_MSG, -1, false, item->m_track ? MF_ENABLED : MF_GRAYED);
				break;
			case COL_TRT:
				if (item->m_track) {
					if (item->m_fxChain.GetLength())
						AddToMenu(hMenu, __LOCALIZE("(FX Chain overrides)","sws_DLG_155"), 0, -1, false, MFS_GRAYED);
					else {
						AddToMenu(hMenu, __LOCALIZE("Load track template...","sws_DLG_155"), SNM_LIVECFG_LOAD_TRACK_TEMPLATE_MSG);
						AddToMenu(hMenu, __LOCALIZE("Clear track templates","sws_DLG_155"), SNM_LIVECFG_CLEAR_TRACK_TEMPLATE_MSG);
					}
				}
				else
					AddToMenu(hMenu, __LOCALIZE("(No track)","sws_DLG_155"), 0, -1, false, MF_GRAYED);
				break;
			case COL_FXC:
				if (item->m_track) {
					if (!item->m_trTemplate.GetLength()) {
						AddToMenu(hMenu, __LOCALIZE("Load FX Chain...","sws_DLG_155"), SNM_LIVECFG_LOAD_FXCHAIN_MSG);
						AddToMenu(hMenu, __LOCALIZE("Clear FX Chains","sws_DLG_155"), SNM_LIVECFG_CLEAR_FXCHAIN_MSG);
					}
					else AddToMenu(hMenu, __LOCALIZE("(Track template overrides)","sws_DLG_155"), 0, -1, false, MF_GRAYED);
				}
				else
					AddToMenu(hMenu, __LOCALIZE("(No track)","sws_DLG_155"), 0, -1, false, MF_GRAYED);
				break;
			case COL_PRESET:
				if (item->m_track)
				{
					if (item->m_trTemplate.GetLength() && !item->m_fxChain.GetLength())
						AddToMenu(hMenu, __LOCALIZE("(Track template overrides)","sws_DLG_155"), 0, -1, false, MFS_GRAYED);
					else if (item->m_fxChain.GetLength())
						AddToMenu(hMenu, __LOCALIZE("(FX Chain overrides)","sws_DLG_155"), 0, -1, false, MFS_GRAYED);
					else {
						// just to be clear w/ the user
						HMENU setPresetMenu = CreatePopupMenu();
						AddPresetSubMenu(setPresetMenu, item->m_track, &item->m_presets);
						AddSubMenu(hMenu, setPresetMenu, __LOCALIZE("Set preset","sws_DLG_155"));
						AddToMenu(hMenu, __LOCALIZE("Clear FX presets","sws_DLG_155"), SNM_LIVECFG_CLEAR_PRESETS_MSG);
					}
				}
				else
					AddToMenu(hMenu, __LOCALIZE("(No track)","sws_DLG_155"), 0, -1, false, MFS_GRAYED);
				break;
			case COL_ACTION_ON:
				AddToMenu(hMenu, __LOCALIZE("Set selected action (in the Actions window)","sws_DLG_155"), SNM_LIVECFG_LEARN_ON_ACTION_MSG);
				AddToMenu(hMenu, __LOCALIZE("Clear actions","sws_DLG_155"), SNM_LIVECFG_CLEAR_ON_ACTION_MSG);
				break;
			case COL_ACTION_OFF:
				AddToMenu(hMenu, __LOCALIZE("Set selected action (in the Actions window)","sws_DLG_155"), SNM_LIVECFG_LEARN_OFF_ACTION_MSG);
				AddToMenu(hMenu, __LOCALIZE("Clear actions","sws_DLG_155"), SNM_LIVECFG_CLEAR_OFF_ACTION_MSG);
				break;
		}

		// common menu items (if a row is selected)
		if (GetMenuItemCount(hMenu))
			AddToMenu(hMenu, SWS_SEPARATOR, 0);
		AddToMenu(hMenu, __LOCALIZE("Apply config","sws_DLG_155"), SNM_LIVECFG_APPLY_MSG);
		AddToMenu(hMenu, __LOCALIZE("Preload config","sws_DLG_155"), SNM_LIVECFG_PRELOAD_MSG);
		AddToMenu(hMenu, SWS_SEPARATOR, 0);
		AddToMenu(hMenu, __LOCALIZE("Copy configs","sws_DLG_155"), SNM_LIVECFG_COPY_MSG);
		AddToMenu(hMenu, __LOCALIZE("Cut configs","sws_DLG_155"), SNM_LIVECFG_CUT_MSG);
		AddToMenu(hMenu, __LOCALIZE("Paste configs","sws_DLG_155"), SNM_LIVECFG_PASTE_MSG, -1, false, g_clipboardConfigs.GetSize() ? MF_ENABLED : MF_GRAYED);
		AddToMenu(hMenu, SWS_SEPARATOR, 0);
		AddToMenu(hMenu, __LOCALIZE("Insert config (shift rows down)","sws_DLG_155"), SNM_LIVECFG_INS_DOWN_MSG);
		AddToMenu(hMenu, __LOCALIZE("Insert config (shift rows up)","sws_DLG_155"), SNM_LIVECFG_INS_UP_MSG);
	}

	// useful when the wnd is resized (buttons not visible..)
	if (*wantDefaultItems)
	{
		AddToMenu(hMenu, __LOCALIZE("Create input track...","sws_DLG_155"), SNM_LIVECFG_CREATE_INPUT_MSG);
		AddToMenu(hMenu, SWS_SEPARATOR, 0);
		AddLearnMenu(hMenu, true);
		AddOptionsMenu(hMenu, true);
		AddToMenu(hMenu, __LOCALIZE("Online help...","sws_DLG_155"), SNM_LIVECFG_HELP_MSG);
	}
	return hMenu;
}

int SNM_LiveConfigsWnd::OnKey(MSG* _msg, int _iKeyState) 
{
	if (_msg->message == WM_KEYDOWN)
	{
		if (!_iKeyState)
		{
			switch(_msg->wParam)
			{
				case VK_F2:		OnCommand(SNM_LIVECFG_EDIT_DESC_MSG, 0);	return 1;
				case VK_RETURN: OnCommand(SNM_LIVECFG_APPLY_MSG, 0);		return 1;
				case VK_DELETE:	OnCommand(SNM_LIVECFG_CLEAR_CC_ROW_MSG, 0);	return 1;
				case VK_INSERT:	OnCommand(SNM_LIVECFG_INS_DOWN_MSG, 0);		return 1;
			}
		}
		else if (_iKeyState == LVKF_CONTROL)
		{
			switch(_msg->wParam) {
				case 'X': OnCommand(SNM_LIVECFG_CUT_MSG, 0);	return 1;
				case 'C': OnCommand(SNM_LIVECFG_COPY_MSG, 0);	return 1;
				case 'V': OnCommand(SNM_LIVECFG_PASTE_MSG, 0);	return 1;
			}
		}
	}
	return 0;
}

void SNM_LiveConfigsWnd::DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight)
{
	LiveConfig* lc = g_liveConfigs.Get()->Get(g_configId);
	if (!lc)
		return;

	int x0=_r->left+SNM_GUI_X_MARGIN, h=SNM_GUI_TOP_H;
	if (_tooltipHeight)
		*_tooltipHeight = h;
	
	LICE_CachedFont* font = SNM_GetThemeFont();

	m_btnEnable.SetCheckState(lc->m_enable);
	m_btnEnable.SetFont(font);
	if (SNM_AutoVWndPosition(DT_LEFT, &m_btnEnable, NULL, _r, &x0, _r->top, h, 4))
	{
		m_cbConfig.SetFont(font);
		if (SNM_AutoVWndPosition(DT_LEFT, &m_cbConfig, &m_btnEnable, _r, &x0, _r->top, h))
		{
			m_txtInputTr.SetFont(font);
			if (SNM_AutoVWndPosition(DT_LEFT, &m_txtInputTr, NULL, _r, &x0, _r->top, h, 4))
			{
				m_cbInputTr.SetFont(font);
				if (SNM_AutoVWndPosition(DT_LEFT, &m_cbInputTr, &m_txtInputTr, _r, &x0, _r->top, h, 4))
				{
					ColorTheme* ct = SNM_GetColorTheme();
					LICE_pixel col = ct ? LICE_RGBA_FROMNATIVE(ct->main_text,255) : LICE_RGBA(255,255,255,255);
					m_knobCC.SetFGColors(col, col);
					if (SNM_AutoVWndPosition(DT_LEFT, &m_vwndCC, NULL, _r, &x0, _r->top, h, 0))
					{
						m_knobFade.SetFGColors(col, col);
						if (SNM_AutoVWndPosition(DT_LEFT, &m_vwndFade, NULL, _r, &x0, _r->top, h))
							SNM_AddLogo(_bm, _r, x0, h);
					}
				}
			}
		}
	}

	// 2nd row of controls (bottom)
	x0 = _r->left+SNM_GUI_X_MARGIN; h=SNM_GUI_BOT_H;
	int y0 = _r->bottom-h;

	SNM_SkinToolbarButton(&m_btnMonitor, __LOCALIZE("Monitor...","sws_DLG_155"));
	if (!SNM_AutoVWndPosition(DT_LEFT, &m_btnMonitor, NULL, _r, &x0, y0, h, 4))
		return;

	SNM_SkinToolbarButton(&m_btnOptions, __LOCALIZE("Options","sws_DLG_155"));
	if (!SNM_AutoVWndPosition(DT_LEFT, &m_btnOptions, NULL, _r, &x0, y0, h, 5))
		return;

	SNM_SkinToolbarButton(&m_btnLearn, __LOCALIZE("Learn","sws_DLG_155"));
	if (!SNM_AutoVWndPosition(DT_LEFT, &m_btnLearn, NULL, _r, &x0, y0, h, 4))
		return;
}

bool SNM_LiveConfigsWnd::GetToolTipString(int _xpos, int _ypos, char* _bufOut, int _bufOutSz)
{
	if (WDL_VWnd* v = m_parentVwnd.VirtWndFromPoint(_xpos,_ypos,1))
	{
		switch (v->GetID())
		{
			case BTNID_ENABLE:
				if (LiveConfig* lc = g_liveConfigs.Get()->Get(g_configId))
					_snprintfSafe(_bufOut, _bufOutSz, __LOCALIZE_VERFMT("Live Config #%d: %s","sws_DLG_155"), g_configId+1, lc->m_enable?__LOCALIZE("on","sws_DLG_155"):__LOCALIZE("off","sws_DLG_155"));
				return true;
			case TXTID_INPUT_TRACK:
			case CMBID_INPUT_TRACK:
				_snprintfSafe(_bufOut, _bufOutSz, __LOCALIZE_VERFMT("Input track for Live Config #%d (optional)","sws_DLG_155"), g_configId+1);
				return true;
			case WNDID_CC_DELAY:
			case KNBID_CC_DELAY:
				// keep messages on a single line (for the langpack generator)
				lstrcpyn(_bufOut, __LOCALIZE("Optional delay when applying/preloading configs","sws_DLG_155"), _bufOutSz);
				return true;
			case WNDID_FADE:
			case KNBID_FADE:
				// keep messages on a single line (for the langpack generator)
				lstrcpyn(_bufOut, __LOCALIZE("Optional fades out/in when deactivating/activating configs\n(ensure glitch-free switches)","sws_DLG_155"), _bufOutSz);
				return true;
		}
	}
	return false;
}

bool SNM_LiveConfigsWnd::Insert(int _dir)
{
	int sortCol = m_pLists.Get(0)->GetSortColumn();
	if (sortCol && sortCol!=1) {
		//JFB lazy here.. better than confusing the user..
		MessageBox(GetHWND(), __LOCALIZE("The list view must be sorted by ascending OSC/CC values!","sws_DLG_155"), __LOCALIZE("S&M - Error","sws_DLG_155"), MB_OK);
		return false;
	}
	if (LiveConfig* lc = g_liveConfigs.Get()->Get(g_configId))
	{
		if (WDL_PtrList<LiveConfigItem>* ccConfs = &lc->m_ccConfs)
		{
			int pos=0;
			if (LiveConfigItem* item = (LiveConfigItem*)m_pLists.Get(0)->EnumSelected(&pos))
			{
				if (_dir>0)
				{
					pos--; // always >0, see EnumSelected()
//					if (pos==ccConfs->GetSize()-1) return false;
					for(int i=pos; i < ccConfs->GetSize()-1; i++) // <size-1!
						ccConfs->Get(i)->m_cc++;
					ccConfs->Insert(pos, new LiveConfigItem(pos));
				}
				else
				{
//					if (!pos) return false;
					ccConfs->Insert(pos, new LiveConfigItem(pos));
					for(int i=pos; i>0 ; i--) // >0 !
						ccConfs->Get(i)->m_cc--;
				}

				int delItemIdx = _dir>0 ? ccConfs->GetSize()-1 : 0;
				item = ccConfs->Get(delItemIdx);
				ccConfs->Delete(delItemIdx, false); // do not delete now (still displayed..)
				Update();
				Undo_OnStateChangeEx2(NULL, SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
				DELETE_NULL(item); // ok to del now that the update is done
				SelectByCCValue(g_configId, _dir>0 ? pos : pos-1);
				return true;
			}
		}
	}
	return false;
}


///////////////////////////////////////////////////////////////////////////////

void LiveConfigsUpdateJob::Perform()
{
	// check consistency of all live configs
	for (int i=0; i<g_liveConfigs.Get()->GetSize(); i++)
	{
		if (LiveConfig* lc = g_liveConfigs.Get()->Get(i))
		{
			bool sndUpdate = false;
			MediaTrack* inputTr = lc->m_inputTr;
			for (int j=0; j<lc->m_ccConfs.GetSize(); j++)
				if (LiveConfigItem* item = lc->m_ccConfs.Get(j))
					if (item->m_track && CSurf_TrackToID(item->m_track, false) <= 0) {
						item->Clear(true);
						sndUpdate = true;
					}
			if (lc->m_inputTr && CSurf_TrackToID(lc->m_inputTr, false) <= 0) {
				inputTr = NULL;
				sndUpdate = true;
			}
			if (sndUpdate)
				lc->SetInputTrack(inputTr, lc->m_autoSends==1);
		}
	}

	if (g_pLiveConfigsWnd) {
		g_pLiveConfigsWnd->FillComboInputTrack();
		g_pLiveConfigsWnd->Update();
	}
}


// moderate write access to reaper.ini
void LiveConfigsUpdateFadeJob::Perform() {
	if (m_value>0) // let the user pref as it is otherwise
		if (int* fadeLenPref = (int*)GetConfigVar("mutefadems10"))
			*fadeLenPref = m_value * 10;
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

		if (LiveConfig* lc = g_liveConfigs.Get()->Get(configId))
		{
			// default values should be consistent with the LiveConfig() constructor

			int success; // for asc. compatibility
			lc->m_enable = lp.gettoken_int(2);
			lc->m_version = lp.gettoken_int(3);
			lc->m_muteOthers = lp.gettoken_int(4);
			stringToGuid(lp.gettoken_str(5), &g);
			// do not set lc->m_inputTr right now but via SetInputTrack(), see below
			MediaTrack* inputTr = GuidsEqual(&g, &GUID_NULL) ? NULL : GuidToTrack(&g); // GUID_NULL would affect the master track otherwise!
			lc->m_selScroll = lp.gettoken_int(6, &success);
			if (!success) lc->m_selScroll = 1;
			lc->m_offlineOthers = lp.gettoken_int(7, &success);
			if (!success) lc->m_offlineOthers = 0;
			lc->m_cc123 = lp.gettoken_int(8, &success);
			if (!success) lc->m_cc123 = 1;
			lc->m_ignoreEmpty = lp.gettoken_int(9, &success);
			if (!success) lc->m_ignoreEmpty = 0;
			lc->m_autoSends = lp.gettoken_int(10, &success);
			if (!success) lc->m_autoSends = 1;
			lc->m_ccDelay = lp.gettoken_int(11, &success);
			if (!success) lc->m_ccDelay = SNM_LIVECFG_DEF_CC_DELAY;
			lc->m_fade = lp.gettoken_int(12, &success);
			if (!success) {
				if (int* fadeLenPref = (int*)GetConfigVar("mutefadems10"))
					lc->m_fade = int(*fadeLenPref/10 + 0.5);
				else
					lc->m_fade = SNM_LIVECFG_DEF_FADE;
			}

			char linebuf[SNM_MAX_CHUNK_LINE_LENGTH];
			while(true)
			{
				if (!ctx->GetLine(linebuf,sizeof(linebuf)) && !lp.parse(linebuf))
				{
					if (lp.gettoken_str(0)[0] == '>')
						break;
					else if (LiveConfigItem* item = lc->m_ccConfs.Get(lp.gettoken_int(0)))
					{
						item->m_cc = lp.gettoken_int(0);
						item->m_desc.Set(lp.gettoken_str(1));
						stringToGuid(lp.gettoken_str(2), &g);
						item->m_track = GuidsEqual(&g, &GUID_NULL) ? NULL : GuidToTrack(&g); // GUID_NULL would affect the master track otherwise!
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

			// once the config as been filled..
			// note: do not manage sends (restored via "normal" undos or via rpp file loading)
			lc->SetInputTrack(inputTr, false);

			// refresh monitoring window (well, it clears ATM)
			//JFB TODO? refresh config infos on undo?
			// (not done because the undo system should be disabled for live use)
			if (g_monitorWnds[configId]) {
				int nbrows = g_monitorWnds[configId]->GetMonitors()->GetRows();
				g_monitorWnds[configId]->Update(
					SNM_APPLY_MASK | (nbrows>1 ? SNM_PRELOAD_MASK : 0), 
					SNM_APPLY_MASK | (nbrows>1 ? SNM_PRELOAD_MASK : 0));
			}
		}

		// refresh editor
		if (g_pLiveConfigsWnd)
			g_pLiveConfigsWnd->Update();

		return true;
	}
	return false;
}

static void SaveExtensionConfig(ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	GUID g; 
	char strId[128] = "";
	WDL_FastString escStrs[6];
	bool firstCfg = true; // will avoid useless data in RPP files
	for (int i=0; i < g_liveConfigs.Get()->GetSize(); i++)
	{
		LiveConfig* lc = g_liveConfigs.Get()->Get(i);
		for (int j=0; lc && j < lc->m_ccConfs.GetSize(); j++)
		{
			LiveConfigItem* item = lc->m_ccConfs.Get(j);
			if (item && !item->IsDefault(false))
			{
				if (firstCfg)
				{
					firstCfg = false;

					if (lc->m_inputTr && CSurf_TrackToID(lc->m_inputTr, false) > 0) 
						g = *(GUID*)GetSetMediaTrackInfo(lc->m_inputTr, "GUID", NULL);
					else 
						g = GUID_NULL;
					guidToString(&g, strId);
					ctx->AddLine("<S&M_MIDI_LIVE %d %d %d %d %s %d %d %d %d %d %d %d", 
						i+1,
						lc->m_enable,
						SNM_LIVECFG_VERSION,
						lc->m_muteOthers,
						strId,
						lc->m_selScroll,
						lc->m_offlineOthers,
						lc->m_cc123,
						lc->m_ignoreEmpty,
						lc->m_autoSends,
						lc->m_ccDelay,
						lc->m_fade);
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
		g_liveConfigs.Get()->Add(new LiveConfig());
}

static project_config_extension_t g_projectconfig = {
	ProcessExtensionLine, SaveExtensionConfig, BeginLoadProjectState, NULL
};

int LiveConfigInit()
{
	GetPrivateProfileString("LiveConfigs", "BigFontName", SNM_DYN_FONT_NAME, g_lcBigFontName, sizeof(g_lcBigFontName), g_SNM_IniFn.Get());

	// instanciate the editor, if needed
	if (SWS_LoadDockWndState("SnMLiveConfigs"))
		g_pLiveConfigsWnd = new SNM_LiveConfigsWnd();

	// instanciate monitors, if needed
	char id[64]="";
	for (int i=0; i<SNM_LIVECFG_NB_CONFIGS; i++) {
		_snprintfSafe(id, sizeof(id), "SnMLiveConfigMonitor%d", i+1);
		g_monitorWnds[i] = SWS_LoadDockWndState(id) ? new SNM_LiveConfigMonitorWnd(i) : NULL;
	}

	if (!plugin_register("projectconfig", &g_projectconfig))
		return 0;

	return 1;
}

void LiveConfigExit()
{
	WritePrivateProfileString("LiveConfigs", "BigFontName", g_lcBigFontName, g_SNM_IniFn.Get());
	DELETE_NULL(g_pLiveConfigsWnd);
}

void OpenLiveConfig(COMMAND_T*)
{
	if (!g_pLiveConfigsWnd)
		g_pLiveConfigsWnd = new SNM_LiveConfigsWnd();
	if (g_pLiveConfigsWnd)
		g_pLiveConfigsWnd->Show(true, true);
}

int IsLiveConfigDisplayed(COMMAND_T*) {
	return (g_pLiveConfigsWnd && g_pLiveConfigsWnd->IsValidWindow());
}


///////////////////////////////////////////////////////////////////////////////
// Apply/preload configs
///////////////////////////////////////////////////////////////////////////////

// muting receives from the input track would be useless: the track is muted anyway
bool TimedMuteIfNeeded(MediaTrack* _tr, DWORD* _muteTime, int _fadeLen = -1)
{
	if (_fadeLen<0)
		if (int* fadeLenPref = (int*)GetConfigVar("mutefadems10"))
			_fadeLen = int(*fadeLenPref/10 + 0.5);

	bool mute = *(bool*)GetSetMediaTrackInfo(_tr, "B_MUTE", NULL);
	if (_fadeLen>0 && _tr && !mute)
	{
		GetSetMediaTrackInfo(_tr, "B_MUTE", &g_bTrue);
		*_muteTime = GetTickCount();
#ifdef _SNM_DEBUG
		char dbg[256] = "";
		_snprintfSafe(dbg, sizeof(dbg), "TimedMuteIfNeeded() - Muted: %s\n", (char*)GetSetMediaTrackInfo(_tr, "P_NAME", NULL));
		OutputDebugString(dbg);
#endif
	}
	return mute;
}

// no-op if _muteTime==0
void WaitForTinyFade(DWORD* _muteTime)
{
	int fadelen = 0;
	if (int* fadeLenPref = (int*)GetConfigVar("mutefadems10"))
		fadelen = int(*fadeLenPref/10 + 0.5);

	if (fadelen>0 && _muteTime && *_muteTime)
	{
		int sleeps = 0;
		while (int(GetTickCount()-*_muteTime) < fadelen && sleeps < 1000) // timeout safety ~ 1s
		{
/* KO of course
			Main_OnCommand(2088, 0); // wait 0.1s before next action
			Main_OnCommand(65535, 0); // no-op
*/
#ifdef _WIN32
			// keep the UI updating
			MSG msg;
			while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
				DispatchMessage(&msg);
#endif
			Sleep(1);
			sleeps++;
		}
#ifdef _SNM_DEBUG
		char dbg[256] = "";
		_snprintfSafe(dbg, sizeof(dbg), "WaitForTrackMuteFade() - Approx wait time: %d ms\n", GetTickCount() - *_muteTime);
		OutputDebugString(dbg);
#endif
		*_muteTime = 0;
	}
}

void WaitForAllNotesOff()
{
	DWORD startWaitTime = GetTickCount();
	while (TrackPreviewsHasAllNotesOff() && 
		(GetTickCount() - startWaitTime) < 1000) // timeout safety
	{
#ifdef _WIN32
		// keep the UI updating
		MSG msg;
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
			DispatchMessage(&msg);
#endif
		Sleep(1);
	}
#ifdef _SNM_DEBUG
	char dbg[256] = "";
	_snprintfSafe(dbg, sizeof(dbg), "WaitForAllNotesOff() - Approx wait time: %d ms\n", GetTickCount() - startWaitTime);
	OutputDebugString(dbg);
#endif
}


// just to simplify ApplyPreloadLiveConfig()
void MuteAndInitCC123(LiveConfig* _lc, MediaTrack* _tr, DWORD* _muteTime, WDL_PtrList<void>* _muteTracks, WDL_PtrList<void>* _cc123Tracks, WDL_PtrList<bool>* _muteStates, bool _always = false)
{
	if (_always || _lc->m_fade>0 || _lc->m_cc123) // receives from the input track must be muted before sending all notes off
	{
		_muteStates->Add(TimedMuteIfNeeded(_tr, _muteTime, _always ? -1 : _lc->m_fade) ? &g_bTrue : &g_bFalse);
		_muteTracks->Add(_tr);
		if (_lc->m_cc123)
			_cc123Tracks->Add(_tr);
	}
}

// just to simplify ApplyPreloadLiveConfig()
void MuteAndInitCC123AllConfigs(LiveConfig* _lc, DWORD* _muteTime, WDL_PtrList<void>* _muteTracks, WDL_PtrList<void>* _cc123Tracks, WDL_PtrList<bool>* _muteStates)
{
	for (int i=0; i<_lc->m_ccConfs.GetSize(); i++)
		if (LiveConfigItem* item = _lc->m_ccConfs.Get(i))
			if (item->m_track && _muteTracks->Find(item->m_track) < 0)
/*JFB!!! no! use-case ex: just switching fx presets (with all options disabled)
				MuteAndInitCC123(_lc, item->m_track, _muteTime, _muteTracks, _cc123Tracks, _muteStates, true); // always mute
*/
				MuteAndInitCC123(_lc, item->m_track, _muteTime, _muteTracks, _cc123Tracks, _muteStates);
}

// just to simplify ApplyPreloadLiveConfig()
// note: no tiny fades for sends unfortunately (last check v4.31) => fade dest tracks instead
void WaitForMuteAndSendCC123(LiveConfig* _lc, LiveConfigItem* _cfg, DWORD* _muteTime, WDL_PtrList<void>* _muteTracks, WDL_PtrList<void>* _cc123Tracks)
{
	WaitForTinyFade(_muteTime); // no-op if muteTime==0

	if (!_lc->m_inputTr || (_lc->m_inputTr && (_cfg->m_track != _lc->m_inputTr)))
		for (int i=0; i<_muteTracks->GetSize(); i++)
			MuteSends(_lc->m_inputTr, (MediaTrack*)_muteTracks->Get(i), true);

	if (_lc->m_cc123 && SendAllNotesOff(_cc123Tracks)) {
		WaitForAllNotesOff();
		_cc123Tracks->Empty(false);
	}
}

// the meat! handle with care!
void ApplyPreloadLiveConfig(bool _apply, int _cfgId, int _val, LiveConfigItem* _lastCfg)
{
	LiveConfig* lc = g_liveConfigs.Get()->Get(_cfgId);
	if (!lc) return;

	LiveConfigItem* cfg = lc->m_ccConfs.Get(_val);
	if (!cfg) return;

	// save selected tracks
	WDL_PtrList<MediaTrack> selTracks;
	SNM_GetSelectedTracks(NULL, &selTracks, true);


	// run desactivate action of the previous config *when it has no track*
	// we ensure that no track is selected when performing the action
	if (_apply && _lastCfg && !_lastCfg->m_track && _lastCfg->m_offAction.GetLength())
		if (int cmd = NamedCommandLookup(_lastCfg->m_offAction.Get()))
		{
			SNM_SetSelectedTrack(NULL, NULL, true, true);
			Main_OnCommand(cmd, 0);
			SNM_GetSelectedTracks(NULL, &selTracks, true); // selection may have changed
		}

	bool preloaded = (_apply && lc->m_preloadMidiVal>=0 && lc->m_preloadMidiVal==_val);
	if (cfg->m_track)
	{
		// --------------------------------------------------------------------
		// mute (according to user options)

		DWORD muteTime=0;
		WDL_PtrList<void> muteTracks, cc123Tracks;
		WDL_PtrList<bool> muteStates;

		// preloading?
		if (!_apply)
		{
			// mute things before reconfiguration (in order to trigger tiny fades, optional)
			// note: no preload on input track
			if (!lc->m_inputTr || (lc->m_inputTr && cfg->m_track != lc->m_inputTr))
				MuteAndInitCC123(lc, cfg->m_track, &muteTime, &muteTracks, &cc123Tracks, &muteStates); 
		}

		// applying?
		// note: kinda repeating code patterns here, but it is for better for lisibilty
		// (maintaining all possible option combinaitions in a single loop was a nightmare..)
		else 
		{	
			// mute things before reconfiguration (in order to trigger tiny fades), optional
			MuteAndInitCC123(lc, cfg->m_track, &muteTime, &muteTracks, &cc123Tracks, &muteStates); 

			// mute (and later unmute) tracks to be set offline - optional
			if (lc->m_offlineOthers)
				for (int i=0; i<lc->m_ccConfs.GetSize(); i++)
					if (LiveConfigItem* item = lc->m_ccConfs.Get(i))
						if (item->m_track && 
							item->m_track != cfg->m_track && 
							(!lc->m_inputTr || (lc->m_inputTr && item->m_track != lc->m_inputTr)) &&
							muteTracks.Find(item->m_track) < 0)
						{
							MuteAndInitCC123(lc, item->m_track, &muteTime, &muteTracks, &cc123Tracks, &muteStates); 
						}

			// first activation: cleanup *everything* as we do not know the initial state
			if (!_lastCfg)
			{
				MuteAndInitCC123AllConfigs(lc, &muteTime, &muteTracks, &cc123Tracks, &muteStates);
			}
			else
			{
				if (_lastCfg->m_track && _lastCfg->m_track != cfg->m_track && muteTracks.Find(_lastCfg->m_track) < 0)
				{
					if (!lc->m_inputTr || (lc->m_inputTr && _lastCfg->m_track != lc->m_inputTr))
						MuteAndInitCC123(lc, _lastCfg->m_track, &muteTime, &muteTracks, &cc123Tracks, &muteStates);
					else if (lc->m_inputTr && _lastCfg->m_track == lc->m_inputTr) // corner case fix
						MuteAndInitCC123AllConfigs(lc, &muteTime, &muteTracks, &cc123Tracks, &muteStates);
				}
			}

			// end with mute states that will not be restored
			if (lc->m_muteOthers &&
				(!lc->m_inputTr || (lc->m_inputTr && cfg->m_track != lc->m_inputTr)))
			{
				for (int i=0; i<lc->m_ccConfs.GetSize(); i++)
					if (LiveConfigItem* item = lc->m_ccConfs.Get(i))
						if (item->m_track && 
							item->m_track != cfg->m_track && 
							(!lc->m_inputTr || (lc->m_inputTr && item->m_track != lc->m_inputTr)))
						{
							TimedMuteIfNeeded(item->m_track, &muteTime, -1); // -1 => always obey
							int idx = muteTracks.Find(item->m_track);
							if (idx<0)
							{
								muteTracks.Add(item->m_track);
								if (lc->m_cc123)
									cc123Tracks.Add(item->m_track);
							}
							else
							{
								muteStates.Delete(idx, false);
								muteStates.Insert(idx, NULL);
							}
						}
			}
		}

		// --------------------------------------------------------------------
		// reconfiguration

		// run desactivate action of the previous config *when it has a track*
		// we ensure that the only selected track when performing the action is the deactivated track,
		// the track can also be muted depending on user options
		if (_apply && _lastCfg && _lastCfg->m_track && _lastCfg->m_offAction.GetLength())
			if (int cmd = NamedCommandLookup(_lastCfg->m_offAction.Get()))
			{
				WaitForMuteAndSendCC123(lc, cfg, &muteTime, &muteTracks, &cc123Tracks);

				SNM_SetSelectedTrack(NULL, _lastCfg->m_track, true, true);
				Main_OnCommand(cmd, 0);
				SNM_GetSelectedTracks(NULL, &selTracks, true); // selection may have changed
			}


		// reconfiguration via state chunk updates
		if (!preloaded)
		{
			// apply tr template (preserves routings, folder states, etc..)
			// if the patched track has sends, these will be glitch free too: me mute this track (source track)
			if (cfg->m_trTemplate.GetLength()) 
			{
				WDL_FastString tmplt; char fn[SNM_MAX_PATH] = "";
				GetFullResourcePath("TrackTemplates", cfg->m_trTemplate.Get(), fn, sizeof(fn));
				if (LoadChunk(fn, &tmplt) && tmplt.GetLength())
				{
					SNM_SendPatcher p(cfg->m_track);  // auto-commit on destroy
					
					WDL_FastString chunk; MakeSingleTrackTemplateChunk(&tmplt, &chunk, true, true, false);
					if (ApplyTrackTemplate(cfg->m_track, &chunk, false, false, &p))
					{
						// make sure the track will be restored with its current name 
						WDL_FastString trNameEsc("");
						if (char* name = (char*)GetSetMediaTrackInfo(cfg->m_track, "P_NAME", NULL))
							if (*name)
								makeEscapedConfigString(name, &trNameEsc);
						p.ParsePatch(SNM_SET_CHUNK_CHAR,1,"TRACK","NAME",0,1,(void*)trNameEsc.Get());

						// make sure the track will be restored with proper mute state
						char onoff[2]; strcpy(onoff, *(bool*)GetSetMediaTrackInfo(cfg->m_track, "B_MUTE", NULL) ? "1" : "0");
						p.ParsePatch(SNM_SET_CHUNK_CHAR,1,"TRACK","MUTESOLO",0,1,onoff);

						WaitForMuteAndSendCC123(lc, cfg, &muteTime, &muteTracks, &cc123Tracks);
					}
				} // auto-commit
			}
			// fx chain reconfiguration via state chunk update
			else if (cfg->m_fxChain.GetLength())
			{
				char fn[SNM_MAX_PATH]=""; WDL_FastString chunk;
				GetFullResourcePath("FXChains", cfg->m_fxChain.Get(), fn, sizeof(fn));
				if (LoadChunk(fn, &chunk) && chunk.GetLength())
				{
					SNM_FXChainTrackPatcher p(cfg->m_track); // auto-commit on destroy
					if (p.SetFXChain(&chunk))
						WaitForMuteAndSendCC123(lc, cfg, &muteTime, &muteTracks, &cc123Tracks);
				}

			} // auto-commit

		} // if (!preloaded)


		// make sure fx are online for the activated/preloaded track
		// note: done here because fx may have been set offline just above
		if ((!_apply ||	(_apply && lc->m_offlineOthers)) && 
			(!lc->m_inputTr || (lc->m_inputTr && cfg->m_track!=lc->m_inputTr)))
		{
			if (!preloaded && TrackFX_GetCount(cfg->m_track))
			{
				SNM_ChunkParserPatcher p(cfg->m_track);
				p.SetWantsMinimalState(true); // ok 'cause read-only
				
				 // some fx offline on the activated track? 
				// (macro-ish but better than using a SNM_ChunkParserPatcher)
				char zero[2] = "0";
				if (!p.Parse(SNM_GETALL_CHUNK_CHAR_EXCEPT, 2, "FXCHAIN", "BYPASS", 0xFFFF, 2, zero))
				{
					WaitForMuteAndSendCC123(lc, cfg, &muteTime, &muteTracks, &cc123Tracks);
					SNM_SetSelectedTrack(NULL, cfg->m_track, true, true);
					Main_OnCommand(40536, 0); // online
				}
			}
		}

		// offline others but active/preloaded tracks
		if (_apply && lc->m_offlineOthers &&
			(!lc->m_inputTr || (lc->m_inputTr && cfg->m_track!=lc->m_inputTr)))
		{
			MediaTrack* preloadTr = NULL;
			if (preloaded && _lastCfg)
				preloadTr = _lastCfg->m_track; // because preload & current are swapped
			else if (LiveConfigItem* preloadCfg = lc->m_ccConfs.Get(lc->m_preloadMidiVal)) // can be <0
				preloadTr = preloadCfg->m_track;

			// select tracks to be set offline (already muted above)
			SNM_SetSelectedTrack(NULL, NULL, true, true);
			for (int i=0; i<lc->m_ccConfs.GetSize(); i++)
				if (LiveConfigItem* item = lc->m_ccConfs.Get(i))
					if (item->m_track && 
						item->m_track != cfg->m_track && // excl. the activated track
						(!lc->m_inputTr || (lc->m_inputTr && item->m_track != lc->m_inputTr)) && // excl. the input track
						(!preloadTr || (preloadTr && preloadTr != item->m_track))) // excl. the preloaded track
					{
						GetSetMediaTrackInfo(item->m_track, "I_SELECTED", &g_i1);
					}
		
			WaitForMuteAndSendCC123(lc, cfg, &muteTime, &muteTracks, &cc123Tracks);

			// set all fx offline for sel tracks, no-op if already offline
			// (macro-ish but better than using a SNM_ChunkParserPatcher for each track..)
			Main_OnCommand(40535, 0);
			Main_OnCommand(41204, 0); // fully unload unloaded VSTs
		}


		// track reconfiguration: fx presets
		// note: exclusive w/ template & fx chain but done here because fx may have been set online just above
		if (!preloaded && cfg->m_presets.GetLength())
		{
			WaitForMuteAndSendCC123(lc, cfg, &muteTime, &muteTracks, &cc123Tracks);
			TriggerFXPresets(cfg->m_track, &(cfg->m_presets));
		}

		// perform activate action
		if (_apply &&
			cfg->m_onAction.GetLength())
			if (int cmd = NamedCommandLookup(cfg->m_onAction.Get()))
			{
				WaitForMuteAndSendCC123(lc, cfg, &muteTime, &muteTracks, &cc123Tracks);
				SNM_SetSelectedTrack(NULL, cfg->m_track, true, true);
				Main_OnCommand(cmd, 0);
				SNM_GetSelectedTracks(NULL, &selTracks, true); // selection may have changed
			}


		// --------------------------------------------------------------------
		// unmute things

		WaitForMuteAndSendCC123(lc, cfg, &muteTime, &muteTracks, &cc123Tracks);

		if (!_apply)
		{
			if (muteStates.Get(0)) // just in case..
				GetSetMediaTrackInfo(cfg->m_track, "B_MUTE", muteStates.Get(0));
		}
		else
		{
			// unmute all sends from the input track except those sending to the new active track
			if (lc->m_inputTr && lc->m_inputTr!=cfg->m_track)
			{
				int sndIdx=0;
				MediaTrack* destTr = (MediaTrack*)GetSetTrackSendInfo(lc->m_inputTr, 0, sndIdx, "P_DESTTRACK", NULL);
				while(destTr)
				{
					bool mute = (destTr!=cfg->m_track);
					if (*(bool*)GetSetTrackSendInfo(lc->m_inputTr, 0, sndIdx, "B_MUTE", NULL) != mute)
						GetSetTrackSendInfo(lc->m_inputTr, 0, sndIdx, "B_MUTE", &mute);
					destTr = (MediaTrack*)GetSetTrackSendInfo(lc->m_inputTr, 0, ++sndIdx, "P_DESTTRACK", NULL);
				}
			}

			// restore mute states, if needed
			for (int i=0; i < muteTracks.GetSize(); i++)
				if (MediaTrack* tr = (MediaTrack*)muteTracks.Get(i))
					if (tr != cfg->m_track && tr != lc->m_inputTr)
						if (bool* mute = muteStates.Get(i))
							if (*(bool*)GetSetMediaTrackInfo(tr, "B_MUTE", NULL) != *mute)
								GetSetMediaTrackInfo(tr, "B_MUTE", mute);

			// unmute the input track , whatever is lc->m_muteOthers
			if (lc->m_inputTr)
				if (*(bool*)GetSetMediaTrackInfo(lc->m_inputTr, "B_MUTE", NULL))
					GetSetMediaTrackInfo(lc->m_inputTr, "B_MUTE", &g_bFalse);

			// unmute the config track, whatever is lc->m_muteOthers
			if (*(bool*)GetSetMediaTrackInfo(cfg->m_track, "B_MUTE", NULL))
				GetSetMediaTrackInfo(cfg->m_track, "B_MUTE", &g_bFalse);
		}

	} // if (cfg->m_track)
	else
	{
		// perform activate action
		if (_apply && cfg->m_onAction.GetLength())
		{
			if (int cmd = NamedCommandLookup(cfg->m_onAction.Get()))
			{
				SNM_SetSelectedTrack(NULL, NULL, true, true);
				Main_OnCommand(cmd, 0);
				SNM_GetSelectedTracks(NULL, &selTracks, true); // selection may have changed
			}
		}
	}

	// restore selected tracks
	SNM_SetSelectedTracks(NULL, &selTracks, true, true);
}


///////////////////////////////////////////////////////////////////////////////
// Apply configs
///////////////////////////////////////////////////////////////////////////////

void ApplyLiveConfig(int _cfgId, int _val, int _valhw, int _relmode, HWND _hwnd, bool _immediate)
{
	LiveConfig* lc = g_liveConfigs.Get()->Get(_cfgId);
	if (lc && lc->m_enable) 
	{
		if (ApplyLiveConfigJob* job = new ApplyLiveConfigJob(SNM_SCHEDJOB_LIVECFG_APPLY+_cfgId, lc->m_ccDelay, lc->m_curMidiVal, _val, _valhw, _relmode, _hwnd, _cfgId))
		{
			lc->m_curMidiVal = job->GetAbsoluteValue();
			if (g_monitorWnds[_cfgId])
				g_monitorWnds[_cfgId]->Update(SNM_APPLY_MASK, 0);

			if (!_immediate && lc->m_ccDelay)
			{
				AddOrReplaceScheduledJob(job);
			}
			else 
			{
				job->Perform();
				delete job;
			}
		}
	}
}

void ApplyLiveConfigJob::Perform()
{
	LiveConfig* lc = g_liveConfigs.Get()->Get(m_cfgId);
	if (!lc) return;

	// swap preload/current configs?
	bool preloaded = (lc->m_preloadMidiVal>=0 && lc->m_preloadMidiVal==m_absval);

	LiveConfigItem* cfg = lc->m_ccConfs.Get(m_absval);
	if (cfg && lc->m_enable && 
		m_absval!=lc->m_activeMidiVal &&
		(!lc->m_ignoreEmpty || (lc->m_ignoreEmpty && !cfg->IsDefault(true)))) // ignore empty configs
	{
		LiveConfigItem* lastCfg = lc->m_ccConfs.Get(lc->m_activeMidiVal); // can be <0
		if (!lastCfg || (lastCfg && !lastCfg->Equals(cfg, true)))
		{
			Undo_BeginBlock2(NULL);

			if (PreventUIRefresh)
				PreventUIRefresh(1);

			ApplyPreloadLiveConfig(true, m_cfgId, m_absval, lastCfg);

			if (PreventUIRefresh)
				PreventUIRefresh(-1);

			{
				char buf[SNM_MAX_ACTION_NAME_LEN]="";
				_snprintfSafe(buf, sizeof(buf), __LOCALIZE_VERFMT("Apply Live Config %d (OSC/CC %d)","sws_undo"), m_cfgId+1, m_absval);
				Undo_EndBlock2(NULL, buf, UNDO_STATE_ALL);
			}
		} 

		// done
		if (preloaded) {
			lc->m_preloadMidiVal = lc->m_curPreloadMidiVal = lc->m_activeMidiVal;
			lc->m_activeMidiVal = lc->m_curMidiVal = m_absval;
		}
		else
			lc->m_activeMidiVal = m_absval;
	}

	// update GUIs in any case, e.g. tweaking (gray cc value) to same value (=> black)
	if (g_pLiveConfigsWnd) {
		g_pLiveConfigsWnd->Update();
//		g_pLiveConfigsWnd->SelectByCCValue(m_cfgId, lc->m_activeMidiVal);
	}

	// swap preload/current configs => update both preload & current panels
	if (g_monitorWnds[m_cfgId])
		g_monitorWnds[m_cfgId]->Update(
			SNM_APPLY_MASK | (preloaded ? SNM_PRELOAD_MASK : 0), 
			SNM_APPLY_MASK | (preloaded ? SNM_PRELOAD_MASK : 0));
}


///////////////////////////////////////////////////////////////////////////////
// Preload configs
///////////////////////////////////////////////////////////////////////////////

void PreloadLiveConfig(int _cfgId, int _val, int _valhw, int _relmode, HWND _hwnd, bool _immediate)
{
	LiveConfig* lc = g_liveConfigs.Get()->Get(_cfgId);
	if (lc && lc->m_enable)
	{
		if (PreloadLiveConfigJob* job = new PreloadLiveConfigJob(SNM_SCHEDJOB_LIVECFG_PRELOAD+_cfgId, lc->m_ccDelay, lc->m_curPreloadMidiVal, _val, _valhw, _relmode, _hwnd, _cfgId))
		{
			lc->m_curPreloadMidiVal = job->GetAbsoluteValue();
			if (g_monitorWnds[_cfgId])
				g_monitorWnds[_cfgId]->Update(SNM_PRELOAD_MASK, 0);

			if (!_immediate && lc->m_ccDelay)
			{
				AddOrReplaceScheduledJob(job);
			}
			else
			{
				job->Perform();
				delete job;
			}
		}
	}
}

void PreloadLiveConfigJob::Perform()
{
	LiveConfig* lc = g_liveConfigs.Get()->Get(m_cfgId);
	if (!lc) return;

	LiveConfigItem* cfg = lc->m_ccConfs.Get(m_absval);
	LiveConfigItem* lastCfg = lc->m_ccConfs.Get(lc->m_activeMidiVal); // can be <0
	if (cfg && lc->m_enable && 
		m_absval!=lc->m_preloadMidiVal &&
		(!lc->m_ignoreEmpty || (lc->m_ignoreEmpty && !cfg->IsDefault(true))) && // ignore empty configs
		(!lastCfg || (lastCfg && (!cfg->m_track || !lastCfg->m_track || cfg->m_track!=lastCfg->m_track)))) // ignore preload over the active track
	{
		LiveConfigItem* lastPreloadCfg = lc->m_ccConfs.Get(lc->m_preloadMidiVal); // can be <0
		if (cfg->m_track && // ATM preload only makes sense for configs for which a track is defined
/*JFB no, always obey!
			lc->m_offlineOthers &&
*/
			(!lc->m_inputTr || (lc->m_inputTr && cfg->m_track!=lc->m_inputTr)) && // no preload for the input track
			(!lastCfg || (lastCfg && !lastCfg->Equals(cfg, true))) &&
			(!lastPreloadCfg || (lastPreloadCfg && !lastPreloadCfg->Equals(cfg, true))))
		{
			Undo_BeginBlock2(NULL);

			if (PreventUIRefresh)
				PreventUIRefresh(1);

			ApplyPreloadLiveConfig(false, m_cfgId, m_absval, lastCfg);

			if (PreventUIRefresh)
				PreventUIRefresh(-1);

			{
				char buf[SNM_MAX_ACTION_NAME_LEN]="";
				_snprintfSafe(buf, sizeof(buf), __LOCALIZE_VERFMT("Preload Live Config %d (OSC/CC %d)","sws_undo"), m_cfgId+1, m_absval);
				Undo_EndBlock2(NULL, buf, UNDO_STATE_ALL);
			}
		}

		// done
		lc->m_preloadMidiVal = m_absval;
	}

	// update GUIs in any case
	if (g_pLiveConfigsWnd) {
		g_pLiveConfigsWnd->Update();
//		g_pLiveConfigsWnd->SelectByCCValue(m_cfgId, lc->m_preloadMidiVal);
	}
	if (g_monitorWnds[m_cfgId])
		g_monitorWnds[m_cfgId]->Update(SNM_PRELOAD_MASK, SNM_PRELOAD_MASK);
}


///////////////////////////////////////////////////////////////////////////////
// SNM_LiveConfigMonitorWnd (several instances!)
///////////////////////////////////////////////////////////////////////////////

enum {
  WNDID_MONITORS=2000, //JFB would be great to have _APS_NEXT_CONTROL_VALUE *always* defined
  TXTID_MON0,
  TXTID_MON1,
  TXTID_MON2,
  TXTID_MON3,
  TXTID_MON4
};

SNM_LiveConfigMonitorWnd::SNM_LiveConfigMonitorWnd(int _cfgId)
	: SWS_DockWnd() // must use the default constructor to create multiple instances
{
	m_cfgId = _cfgId;

	char title[64]="", dockName[64]="";
	_snprintfSafe(title, sizeof(title), __LOCALIZE_VERFMT("Live Config #%d Monitor","sws_DLG_169"), _cfgId+1);
	_snprintfSafe(dockName, sizeof(dockName), "SnMLiveConfigMonitor%d", _cfgId+1);

	// see SWS_DockWnd()
	m_hwnd=NULL;
	m_iResource=IDD_SNM_LIVE_CONFIG_MON;
	m_wndTitle.Set(title);
	m_id.Set(dockName);
	m_bUserClosed = false;
	m_iCmdID = SWSGetCommandID(OpenLiveConfigMonitorWnd, (INT_PTR)_cfgId);
	m_bLoadingState = false;
	screenset_registerNew((char*)dockName, screensetCallback, this);

	// Must call SWS_DockWnd::Init() to restore parameters and open the window if necessary
	Init();
}

void SNM_LiveConfigMonitorWnd::OnInitDlg()
{
	m_vwnd_painter.SetGSC(WDL_STYLE_GetSysColor);
	m_parentVwnd.SetRealParent(m_hwnd);

	for (int i=0; i<5; i++)
		m_txtMon[i].SetID(TXTID_MON0+i);
	m_mons.AddMonitors(&m_txtMon[0], &m_txtMon[1], &m_txtMon[2], &m_txtMon[3], &m_txtMon[4]);
	m_mons.SetID(WNDID_MONITORS);
	m_mons.SetRows(1);
	m_parentVwnd.AddChild(&m_mons);

	{
		char buf[64]="";
		_snprintfSafe(buf, sizeof(buf), __LOCALIZE_VERFMT("Live Config #%d","sws_DLG_169"), m_cfgId+1);
		m_mons.SetTitles(__LOCALIZE("CURRENT","sws_DLG_169"), buf, __LOCALIZE("PRELOAD","sws_DLG_169"), " "); // " " trick to get a lane
		_snprintfSafe(buf, sizeof(buf), "#%d", m_cfgId+1);
		m_mons.SetFontName(g_lcBigFontName);
		m_mons.SetText(0, buf, 0, 16);
	}

	Update(SNM_APPLY_MASK|SNM_PRELOAD_MASK, SNM_APPLY_MASK|SNM_PRELOAD_MASK);
}

void SNM_LiveConfigMonitorWnd::OnDestroy() {
	m_mons.RemoveAllChildren(false);
}

void SNM_LiveConfigMonitorWnd::Update(int _whatFlags, int _commitFlags)
{
	if (LiveConfig* lc = g_liveConfigs.Get()->Get(m_cfgId))
	{
		if (lc->m_enable)
		{
			char buf[64] = "";
			WDL_FastString info;
			if (_whatFlags & SNM_APPLY_MASK)
			{
				int val = (_commitFlags & SNM_APPLY_MASK ? lc->m_activeMidiVal : lc->m_curMidiVal);
				if (val>=0) _snprintfSafe(buf, sizeof(buf), "%03d", val);
				m_mons.SetText(1, buf, 0, _commitFlags & SNM_APPLY_MASK ? 255 : 143);
				if (LiveConfigItem* item = lc->m_ccConfs.Get(val)) {
					item->GetInfo(&info);
					m_mons.SetText(2, info.Get(), 0, _commitFlags & SNM_APPLY_MASK ? 255 : 143);
				}
				else
					m_mons.SetText(2, "");
			}

			if (_whatFlags & SNM_PRELOAD_MASK)
			{
				*buf = '\0';
				int val = (_commitFlags & SNM_PRELOAD_MASK ? lc->m_preloadMidiVal : lc->m_curPreloadMidiVal);
				if (val>=0) _snprintfSafe(buf, sizeof(buf), "%03d", val);
				m_mons.SetText(3, buf, 0, _commitFlags & SNM_PRELOAD_MASK ? 255 : 143);
				if (LiveConfigItem* item = lc->m_ccConfs.Get(val)) {
					item->GetInfo(&info);
					m_mons.SetText(4, info.Get(), 0, _commitFlags & SNM_PRELOAD_MASK ? 255 : 143);
				}
				else
					m_mons.SetText(4, "");

				// (force) display of the preload area
				if (m_mons.GetRows()<2)
					m_mons.SetRows(2);
			}
		}
		else
		{
			m_mons.SetText(1, "---", SNM_COL_RED_MONITOR);
			m_mons.SetText(2, __LOCALIZE("<DISABLED>","sws_DLG_169"), SNM_COL_RED_MONITOR);
			m_mons.SetText(3, "---", SNM_COL_RED_MONITOR);
			m_mons.SetText(4, __LOCALIZE("<DISABLED>","sws_DLG_169"), SNM_COL_RED_MONITOR);

			// (force) hide the preload area
//			if (m_mons.GetRows()>=2)
//				m_mons.SetRows(1);
		}
	}
}

INT_PTR SNM_LiveConfigMonitorWnd::OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_MOUSEWHEEL:
			if (LiveConfig* lc = g_liveConfigs.Get()->Get(m_cfgId))
			{
				int val = (short)HIWORD(wParam);
#ifdef _SNM_DEBUG
				char dbg[256] = "";
				_snprintfSafe(dbg, sizeof(dbg), "WM_MOUSEWHEEL - val: %d\n", val);
				OutputDebugString(dbg);
#endif
				val = lc->m_curMidiVal + (val>0 ? -1 : val<0 ? 1 : 0);
				if (val>127) val-=128;
				else if (val<0) val=128+val;

				if (WDL_VWnd* mon0 = m_parentVwnd.GetChildByID(TXTID_MON0))
				{
					bool vis = mon0->IsVisible();
					mon0->SetVisible(false); // just a setter.. to exclude from VirtWndFromPoint()

					POINT p; GetCursorPos(&p);
					ScreenToClient(m_hwnd, &p);
					if (WDL_VWnd* v = m_parentVwnd.VirtWndFromPoint(p.x,p.y,1))
						switch (v->GetID())
						{
							case TXTID_MON1:
							case TXTID_MON2:
								ApplyLiveConfig(m_cfgId, val, -1, 0, SNM_APPLY_PRELOAD_HWND);
								break;
							case TXTID_MON3:
							case TXTID_MON4:
								val = (short)HIWORD(wParam);
								val = lc->m_curPreloadMidiVal + (val>0 ? -1 : val<0 ? 1 : 0);
								if (val>127) val-=128;
								else if (val<0) val=128+val;
								PreloadLiveConfig(m_cfgId, val, -1, 0, SNM_APPLY_PRELOAD_HWND);
								break;
						}
					mon0->SetVisible(vis);
				}
				else
					ApplyLiveConfig(m_cfgId, val, -1, 0, SNM_APPLY_PRELOAD_HWND);
			}
			return -1;
	}
	return 0;
}

void SNM_LiveConfigMonitorWnd::DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight)
{
	m_mons.SetPosition(_r);
	m_mons.SetVisible(true);
	SNM_AddLogo(_bm, _r);
}

// handle mouse up/down events at window level (rather than inheriting VWnds)
int SNM_LiveConfigMonitorWnd::OnMouseDown(int _xpos, int _ypos)
{
	if (m_parentVwnd.VirtWndFromPoint(_xpos,_ypos,1) != NULL)
		return 1;
	return 0;
}

bool SNM_LiveConfigMonitorWnd::OnMouseUp(int _xpos, int _ypos)
{
	if (LiveConfig* lc = g_liveConfigs.Get()->Get(m_cfgId))
	{
		if (WDL_VWnd* mon0 = m_parentVwnd.GetChildByID(TXTID_MON0))
		{
			bool vis = mon0->IsVisible();
			mon0->SetVisible(false); // just a setter.. to exclude from VirtWndFromPoint()
			if (WDL_VWnd* v = m_parentVwnd.VirtWndFromPoint(_xpos,_ypos,1))
				switch (v->GetID())
				{
					// tgl between 1 & 2 rows
					case TXTID_MON1:
					case TXTID_MON2:
						m_mons.SetRows(m_mons.GetRows()==1 ? 2 : 1);
						break;
					// preload
					case TXTID_MON3:
					case TXTID_MON4:
						if (lc->m_preloadMidiVal>=0)
							ApplyLiveConfig(m_cfgId, lc->m_preloadMidiVal, -1, 0, SNM_APPLY_PRELOAD_HWND, true); // immediate
						break;
				}
			mon0->SetVisible(vis);
		}
	}
	return true;
}

void OpenLiveConfigMonitorWnd(int _idx)
{
	if (_idx>=0 && _idx<SNM_LIVECFG_NB_CONFIGS)
	{
		if (!g_monitorWnds[_idx])
			g_monitorWnds[_idx] = new SNM_LiveConfigMonitorWnd(_idx);
		if (g_monitorWnds[_idx])
			g_monitorWnds[_idx]->Show(true, true);
	}
}

void OpenLiveConfigMonitorWnd(COMMAND_T* _ct) {
	OpenLiveConfigMonitorWnd((int)_ct->user);
}

int IsLiveConfigMonitorWndDisplayed(COMMAND_T* _ct) {
	int wndId = (int)_ct->user;
	return (wndId>=0 && wndId<SNM_LIVECFG_NB_CONFIGS && g_monitorWnds[wndId] && g_monitorWnds[wndId]->IsValidWindow());
}


///////////////////////////////////////////////////////////////////////////////
// Actions
///////////////////////////////////////////////////////////////////////////////

void ApplyLiveConfig(MIDI_COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd) {
	ApplyLiveConfig((int)_ct->user, _val, _valhw, _relmode, _hwnd);
}

void PreloadLiveConfig(MIDI_COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd) {
	PreloadLiveConfig((int)_ct->user, _val, _valhw, _relmode, _hwnd);
}

/////

void NextLiveConfig(COMMAND_T* _ct)
{
	int cfgId = (int)_ct->user;
	if (LiveConfig* lc = g_liveConfigs.Get()->Get(cfgId))
	{
		int i = lc->m_curMidiVal; // and not m_activeMidiVal, cc "smoothing" should apply here too!
		while (++i < lc->m_ccConfs.GetSize())
			if (LiveConfigItem* item = lc->m_ccConfs.Get(i))
				if (!lc->m_ignoreEmpty || (lc->m_ignoreEmpty && !item->IsDefault(true))) {
					ApplyLiveConfig(cfgId, i, -1, 0, SNM_APPLY_PRELOAD_HWND);
					return;
				}
		// 2nd try (cycle)
		i = -1;
		while (++i < lc->m_curMidiVal)
			if (LiveConfigItem* item = lc->m_ccConfs.Get(i))
				if (!lc->m_ignoreEmpty || (lc->m_ignoreEmpty && !item->IsDefault(true))) {
					ApplyLiveConfig(cfgId, i, -1, 0, SNM_APPLY_PRELOAD_HWND);
					return;
				}
	}
}

void PreviousLiveConfig(COMMAND_T* _ct)
{
	int cfgId = (int)_ct->user;
	if (LiveConfig* lc = g_liveConfigs.Get()->Get(cfgId))
	{
		int i = lc->m_curMidiVal; // and not m_activeMidiVal, cc "smoothing" should apply here too!
		while (--i >= 0)
			if (LiveConfigItem* item = lc->m_ccConfs.Get(i))
				if (!lc->m_ignoreEmpty || (lc->m_ignoreEmpty && !item->IsDefault(true))) {
					ApplyLiveConfig(cfgId, i, -1, 0, SNM_APPLY_PRELOAD_HWND);
					return;
				}
		// 2nd try (cycle)
		i = lc->m_ccConfs.GetSize();
		while (--i > lc->m_curMidiVal)
			if (LiveConfigItem* item = lc->m_ccConfs.Get(i))
				if (!lc->m_ignoreEmpty || (lc->m_ignoreEmpty && !item->IsDefault(true))) {
					ApplyLiveConfig(cfgId, i, -1, 0, SNM_APPLY_PRELOAD_HWND);
					return;
				}
	}
}

void SwapCurrentPreloadLiveConfigs(COMMAND_T* _ct)
{
	int cfgId = (int)_ct->user;
	if (LiveConfig* lc = g_liveConfigs.Get()->Get(cfgId))
		ApplyLiveConfig(cfgId, lc->m_preloadMidiVal, -1, 0, SNM_APPLY_PRELOAD_HWND, true); // immediate
}

/////

int IsLiveConfigEnabled(COMMAND_T* _ct) {
	if (LiveConfig* lc = g_liveConfigs.Get()->Get((int)_ct->user))
		return (lc->m_enable == 1);
	return false;
}

// _val: 0, 1 or <0 to toggle
void UpdateEnableLiveConfig(int _cfgId, int _val)
{
	if (LiveConfig* lc = g_liveConfigs.Get()->Get(_cfgId))
	{
		lc->m_enable = _val<0 ? !lc->m_enable : _val;
		if (!lc->m_enable)
			lc->m_curMidiVal = lc->m_activeMidiVal = lc->m_curPreloadMidiVal = lc->m_preloadMidiVal = -1;
		Undo_OnStateChangeEx2(NULL, SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);

		if (g_pLiveConfigsWnd && (g_configId == _cfgId))
			g_pLiveConfigsWnd->Update();
		if (g_monitorWnds[_cfgId])
			g_monitorWnds[_cfgId]->Update(SNM_APPLY_MASK|SNM_PRELOAD_MASK, 0);

		// RefreshToolbar()) not required..
	}
}

void EnableLiveConfig(COMMAND_T* _ct) {
	UpdateEnableLiveConfig((int)_ct->user, 1);
}

void DisableLiveConfig(COMMAND_T* _ct) {
	UpdateEnableLiveConfig((int)_ct->user, 0);
}

void ToggleEnableLiveConfig(COMMAND_T* _ct) {
	UpdateEnableLiveConfig((int)_ct->user, -1);
}

/////

int IsMuteOthersLiveConfigEnabled(COMMAND_T* _ct) {
	if (LiveConfig* lc = g_liveConfigs.Get()->Get((int)_ct->user))
		return (lc->m_muteOthers != 0);
	return false;
}

// _val: 0, 1 or <0 to toggle
// gui refresh not needed (only in ctx menu)
void UpdateMuteOthers(COMMAND_T* _ct, int _val)
{
	if (LiveConfig* lc = g_liveConfigs.Get()->Get((int)_ct->user)) {
		lc->m_muteOthers = _val<0 ? !lc->m_muteOthers : _val;
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_MISCCFG, -1);
		// RefreshToolbar()) not required..
	}
}

void EnableMuteOthersLiveConfig(COMMAND_T* _ct) {
	UpdateMuteOthers(_ct, 1);
}

void DisableMuteOthersLiveConfig(COMMAND_T* _ct) {
	UpdateMuteOthers(_ct, 0);
}

void ToggleMuteOthersLiveConfig(COMMAND_T* _ct) {
	UpdateMuteOthers(_ct, -1);
}

/////

int IsOfflineOthersLiveConfigEnabled(COMMAND_T* _ct) {
	if (LiveConfig* lc = g_liveConfigs.Get()->Get((int)_ct->user))
		return (lc->m_offlineOthers == 1);
	return false;
}

// _val: 0, 1 or <0 to toggle
// gui refresh not needed (only in ctx menu)
void UpdateOfflineOthers(COMMAND_T* _ct, int _val)
{
	if (LiveConfig* lc = g_liveConfigs.Get()->Get((int)_ct->user)) {
		lc->m_offlineOthers = _val<0 ? !lc->m_offlineOthers : _val;
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_MISCCFG, -1);
		// RefreshToolbar()) not required..
	}
}

void EnableOfflineOthersLiveConfig(COMMAND_T* _ct) {
	UpdateOfflineOthers(_ct, 1);
}

void DisableOfflineOthersLiveConfig(COMMAND_T* _ct) {
	UpdateOfflineOthers(_ct, 0);
}

void ToggleOfflineOthersLiveConfig(COMMAND_T* _ct) {
	UpdateOfflineOthers(_ct, -1);
}

/////

int IsAllNotesOffLiveConfigEnabled(COMMAND_T* _ct) {
	if (LiveConfig* lc = g_liveConfigs.Get()->Get((int)_ct->user))
		return (lc->m_cc123 == 1);
	return false;
}

// _val: 0, 1 or <0 to toggle
// gui refresh not needed (only in ctx menu)
void UpdateAllNotesOff(COMMAND_T* _ct, int _val)
{
	if (LiveConfig* lc = g_liveConfigs.Get()->Get((int)_ct->user)) {
		lc->m_cc123 = _val<0 ? !lc->m_cc123 : _val;
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_MISCCFG, -1);
		// RefreshToolbar()) not required..
	}
}

void EnableAllNotesOffLiveConfig(COMMAND_T* _ct) {
	UpdateAllNotesOff(_ct, 1);
}

void DisableAllNotesOffLiveConfig(COMMAND_T* _ct) {
	UpdateAllNotesOff(_ct, 0);
}

void ToggleAllNotesOffLiveConfig(COMMAND_T* _ct) {
	UpdateAllNotesOff(_ct, -1);
}

/////

int IsTinyFadesLiveConfigEnabled(COMMAND_T* _ct) {
	if (LiveConfig* lc = g_liveConfigs.Get()->Get((int)_ct->user))
		return (lc->m_fade > 0);
	return false;
}

// _val: 0, 1 or <0 to toggle
// gui refresh not needed (only in ctx menu)
void UpdateTinyFadesLiveConfig(COMMAND_T* _ct, int _val)
{
	if (LiveConfig* lc = g_liveConfigs.Get()->Get((int)_ct->user))
	{
		lc->m_fade = _val<0 ? (lc->m_fade>0 ? 0 : SNM_LIVECFG_DEF_FADE) : _val;
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_MISCCFG, -1);

		LiveConfigsUpdateFadeJob* job = new LiveConfigsUpdateFadeJob(lc->m_fade);
		job->Perform();	delete job;
		if (g_pLiveConfigsWnd)
			g_pLiveConfigsWnd->Update();
		// RefreshToolbar()) not required..
	}
}

void EnableTinyFadesLiveConfig(COMMAND_T* _ct) {
	if (LiveConfig* lc = g_liveConfigs.Get()->Get((int)_ct->user))
		if (lc->m_fade <= 0)
			UpdateTinyFadesLiveConfig(_ct, SNM_LIVECFG_DEF_FADE);
}

void DisableTinyFadesLiveConfig(COMMAND_T* _ct) {
	if (LiveConfig* lc = g_liveConfigs.Get()->Get((int)_ct->user))
		if (lc->m_fade > 0)
			UpdateTinyFadesLiveConfig(_ct, 0);
}

void ToggleTinyFadesLiveConfig(COMMAND_T* _ct) {
	UpdateTinyFadesLiveConfig(_ct, -1);
}

