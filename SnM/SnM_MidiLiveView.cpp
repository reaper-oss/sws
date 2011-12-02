/******************************************************************************
/ SnM_MidiLiveView.cpp
/ JFB TODO? now, SnM_LiveConfigs.cpp/.h would be better names..
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

// JFB TODO?
// - max nb of tracks & presets to check
// - FX chains: + S&M's FX chain view slots
// - full release? auto send creation, etc..

#include "stdafx.h"
#include "SnM_Actions.h"
#include "SnM_MidiLiveView.h"
#include "SnM_Chunk.h"


#define SAVEWINDOW_POS_KEY		"S&M - Live Configs List Save Window Position"
#define NB_CC_VALUES			128

#define SNM_LIVECFG_CLEAR_CC_ROW_MSG			0xF001
#define SNM_LIVECFG_LOAD_TRACK_TEMPLATE_MSG		0xF002
#define SNM_LIVECFG_CLEAR_TRACK_TEMPLATE_MSG	0xF003
#define SNM_LIVECFG_LOAD_FXCHAIN_MSG			0xF004
#define SNM_LIVECFG_CLEAR_FXCHAIN_MSG			0xF005
#define SNM_LIVECFG_PERFORM_MSG					0xF006 
#define SNM_LIVECFG_EDIT_DESC_MSG				0xF007
#define SNM_LIVECFG_CLEAR_DESC_MSG				0xF008
#define SNM_LIVECFG_EDIT_ON_ACTION_MSG			0xF009
#define SNM_LIVECFG_CLEAR_ON_ACTION_MSG			0xF00A
#define SNM_LIVECFG_EDIT_OFF_ACTION_MSG			0xF00B
#define SNM_LIVECFG_CLEAR_OFF_ACTION_MSG		0xF00C 
#define SNM_LIVECFG_SET_TRACK_MSG				0xF00D // => 243 track max.
#define SNM_LIVECFG_CLEAR_TRACK_MSG				0xF100
#define SNM_LIVECFG_SET_PRESETS_MSG				0xF101 // => theorically 3838 preset max, but see SNM_LIVECFG_MAX_PRESET_COUNT
#define SNM_LIVECFG_CLEAR_PRESETS_MSG			0xFFFF

#define SNM_LIVECFG_UNDO_STR					"Live configs edition"

enum {
  TXTID_CONFIG=2000, //JFB would be great to have _APS_NEXT_CONTROL_VALUE *always* defined
  COMBOID_CONFIG,
  BUTTONID_ENABLE,
  TXTID_INPUT_TRACK,
  COMBOID_INPUT_TRACK,
  BUTTONID_MUTE_OTHERS,
  BUTTONID_AUTO_SELECT,
};

enum {
  COL_CC=0,
  COL_COMMENT,
  COL_TR,
  COL_TRT,
  COL_FXC,
#ifdef _SNM_PRESETS
  COL_PRESET,
#endif
  COL_ACTION_ON,
  COL_ACTION_OFF
};


/*JFB static*/ SNM_LiveConfigsWnd* g_pLiveConfigsWnd = NULL;
SWSProjConfig<MidiLiveConfig> g_liveConfigs;
SWSProjConfig<WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<MidiLiveItem> > > g_liveCCConfigs;

int g_configId = 0; // the current *displayed* config id
int g_approxDelayMsCC = 250;


///////////////////////////////////////////////////////////////////////////////
// SNM_LiveConfigsView
///////////////////////////////////////////////////////////////////////////////

#ifdef _SNM_PRESETS
static SWS_LVColumn g_midiLiveCols[] = { {95,2,"CC value"}, {150,1,"Comment"}, {150,2,"Track"}, {175,2,"Track template"}, {175,2,"FX Chain"}, {150,2,"FX user presets"}, {150,1,"Activate action"}, {150,1,"Deactivate action"}};
#else
static SWS_LVColumn g_midiLiveCols[] = { {95,2,"CC value"}, {150,1,"Comment"}, {150,2,"Track"}, {175,2,"Track template"}, {175,2,"FX Chain"}, {150,1,"Activate action"}, {150,1,"Deactivate action"}};
#endif


SNM_LiveConfigsView::SNM_LiveConfigsView(HWND hwndList, HWND hwndEdit)
#ifdef _SNM_PRESETS
:SWS_ListView(hwndList, hwndEdit, 8, g_midiLiveCols, "Live Configs View State", false)
#else
:SWS_ListView(hwndList, hwndEdit, 7, g_midiLiveCols, "Live Configs View State", false)
#endif
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
				_snprintf(str, iStrMax, "%03d %c", pItem->m_cc, pItem->m_cc == g_liveConfigs.Get()->m_lastMIDIVal[g_configId] ? '*' : ' ');
				break;
			case COL_COMMENT:
				lstrcpyn(str, pItem->m_desc.Get(), iStrMax);
				break;
			case COL_TR:
			{
				if (pItem->m_track)
					if (char* name = (char*)GetSetMediaTrackInfo(pItem->m_track, "P_NAME", NULL))
						_snprintf(str, iStrMax, "[%d] \"%s\"", CSurf_TrackToID(pItem->m_track, false), name);
				break;
			}
			case COL_TRT:
				lstrcpyn(str, pItem->m_trTemplate.Get(), iStrMax);
				break;
			case COL_FXC:
				lstrcpyn(str, pItem->m_fxChain.Get(), iStrMax);
				break;
#ifdef _SNM_PRESETS
			case COL_PRESET:
			{
				WDL_FastString renderConf;
//				RenderPresetConf(&(pItem->m_presets), &renderConf);
				RenderPresetConf2(pItem->m_track, &(pItem->m_presets), &renderConf);
				lstrcpyn(str, renderConf.Get(), iStrMax);
				break;
			}
#endif
			//JFB TODO? actions: kbd_getTextFromCmd, tooltip?
			case COL_ACTION_ON:
				lstrcpyn(str, pItem->m_onAction.Get(), iStrMax);
				break;
			case COL_ACTION_OFF:
				lstrcpyn(str, pItem->m_offAction.Get(), iStrMax);
				break;
		}
	}
}

void SNM_LiveConfigsView::SetItemText(SWS_ListItem* item, int iCol, const char* str)
{
	if (MidiLiveItem* pItem = (MidiLiveItem*)item)
	{
		if (iCol==COL_COMMENT)
		{
			// limit the comment size (for RPP files)
			pItem->m_desc.Set(str);
			pItem->m_desc.Ellipsize(32,32);
			Update();
			Undo_OnStateChangeEx(SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
		}
		else if (iCol==COL_ACTION_ON || iCol == COL_ACTION_OFF)
		{
			if (*str != 0 && !NamedCommandLookup(str)) {
				MessageBox(GetParent(m_hwndList), "Error: this action ID (or custom ID) doesn't exists.", "S&M - Live Configs", MB_OK);
				return;
			}

			if (iCol==COL_ACTION_ON)
				pItem->m_onAction.Set(str);
			else
				pItem->m_offAction.Set(str);

			Update();
			Undo_OnStateChangeEx(SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
		}
	}
}

void SNM_LiveConfigsView::GetItemList(SWS_ListItemList* pList)
{
	if (WDL_PtrList<MidiLiveItem>* configs = g_liveCCConfigs.Get()->Get(g_configId))
		for (int i=0; i < configs->GetSize(); i++)
			pList->Add((SWS_ListItem*)configs->Get(i));
}

void SNM_LiveConfigsView::OnItemSelChanged(SWS_ListItem* item, int iState)
{
	// can lead to confusion with auto track selection ticked
	if (!g_liveConfigs.Get()->m_autoSelect[g_configId])
	{
//JFB!!! restore sel ko, wtf!?? => undos commented
//		Undo_BeginBlock2(NULL);
		for (int i=0; i <= GetNumTracks(); i++) {
			MediaTrack* tr = CSurf_TrackFromID(i, false);
			if (tr) GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i0);
		}
		int x=0;
		while(MidiLiveItem* item = (MidiLiveItem*)EnumSelected(&x))
			if (item->m_track)
				GetSetMediaTrackInfo(item->m_track, "I_SELECTED", &g_i1);
		TrackList_AdjustWindows(false);
		Main_OnCommand(40913,0); // scroll to selected tracks
//		Undo_EndBlock2(NULL, "Change Track Selection", UNDO_STATE_ALL);
//		Undo_OnStateChangeEx("Change Track Selection", , -1);
	}
}


void SNM_LiveConfigsView::OnItemDblClk(SWS_ListItem* item, int iCol)
{
	if (MidiLiveItem* pItem = (MidiLiveItem*)item)
	{
		switch(iCol)
		{
			case COL_CC:
				g_pLiveConfigsWnd->OnCommand(SNM_LIVECFG_PERFORM_MSG, (LPARAM)pItem);
				break;
			case COL_TRT:
				if (pItem->m_track)
					g_pLiveConfigsWnd->OnCommand(SNM_LIVECFG_LOAD_TRACK_TEMPLATE_MSG, (LPARAM)pItem);
				break;
			case COL_FXC:
				if (pItem->m_track)
					g_pLiveConfigsWnd->OnCommand(SNM_LIVECFG_LOAD_FXCHAIN_MSG, (LPARAM)pItem);
				break;
		}
	}
}

// faster: default SWS_ListItem's sort algo but don't use GetItemText() for presets
// (so biaised for presets which are sorted by fx# + preset#)
int SNM_LiveConfigsView::OnItemSort(SWS_ListItem* _item1, SWS_ListItem* _item2)
{
#ifdef _SNM_PRESETS
	char str1[64]=""; char str2[64]="";

	if (abs(m_iSortCol)-1 != COL_PRESET)
		GetItemText(_item1, abs(m_iSortCol)-1, str1, 64);
	else
		lstrcpyn(str1, ((MidiLiveItem*)_item1)->m_presets.Get(), 64);

	if (abs(m_iSortCol)-1 != COL_PRESET)
		GetItemText(_item2, abs(m_iSortCol)-1, str2, 64);
	else
		lstrcpyn(str2, ((MidiLiveItem*)_item2)->m_presets.Get(), 64);

	// If strings are purely numbers, sort numerically
	char* pEnd1, *pEnd2;
	int i1 = strtol(str1, &pEnd1, 0);
	int i2 = strtol(str2, &pEnd2, 0);
	int iRet = 0;
	if ((i1 || i2) && !*pEnd1 && !*pEnd2) {
		if (i1 > i2) iRet = 1;
		else if (i1 < i2) iRet = -1;
	}
	else
		iRet = strcmp(str1, str2);
	
	if (m_iSortCol < 0) return -iRet;
	else return iRet;
	return 0;
#else
	return SWS_ListView::OnItemSort(_item1, _item2);
#endif
}


///////////////////////////////////////////////////////////////////////////////
// SNM_LiveConfigsWnd
///////////////////////////////////////////////////////////////////////////////

SNM_LiveConfigsWnd::SNM_LiveConfigsWnd()
	: SWS_DockWnd(IDD_SNM_MIDI_LIVE, "Live Configs", "SnMLiveConfigs", 30009, SWSGetCommandID(OpenLiveConfigView))
{
	// Must call SWS_DockWnd::Init() to restore parameters and open the window if necessary
	Init();
}

INT_PTR SNM_LiveConfigsWnd::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static int sListOldColors[LISTVIEW_COLORHOOK_STATESIZE];
	if (ListView_HookThemeColorsMessage(m_hwnd, uMsg, lParam, sListOldColors, IDC_LIST, 0, 0))
		return 1;
	return SWS_DockWnd::WndProc(uMsg, wParam, lParam);
}

void SNM_LiveConfigsWnd::CSurfSetTrackListChange() {
	// we use a ScheduledJob because of possible multi-notifs
	SNM_LiveCfg_TLChangeSchedJob* job = new SNM_LiveCfg_TLChangeSchedJob();
	AddOrReplaceScheduledJob(job);
}

void SNM_LiveConfigsWnd::CSurfSetTrackTitle() {
	FillComboInputTrack();
	Update();
}

void SNM_LiveConfigsWnd::FillComboInputTrack() {
	m_cbInputTr.Empty();
	m_cbInputTr.AddItem("None");
	for (int i=1; i <= GetNumTracks(); i++)
	{
		WDL_FastString ellips;
		char* name = (char*)GetSetMediaTrackInfo(CSurf_TrackFromID(i,false), "P_NAME", NULL);
		ellips.SetFormatted(SNM_MAX_TRACK_NAME_LEN, "[%d] \"%s\"", i, name);
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
			for (int i = 0; i < lv->GetListItemCount(); i++)
			{
				MidiLiveItem* item = (MidiLiveItem*)lv->GetListItem(i);
				if (item && item->m_cc == _cc) 
				{
					ListView_SetItemState(hList, -1, 0, LVIS_SELECTED);
					ListView_SetItemState(hList, i, LVIS_SELECTED, LVIS_SELECTED); 
					ListView_EnsureVisible(hList, i, true);
					Update(); // just usefull to set the "*" when triggering via dbl-click
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

void SNM_LiveConfigsWnd::OnInitDlg()
{
	m_resize.init_item(IDC_LIST, 0.0, 0.0, 1.0, 1.0);
	m_pLists.Add(new SNM_LiveConfigsView(GetDlgItem(m_hwnd, IDC_LIST), GetDlgItem(m_hwnd, IDC_EDIT)));
	SNM_ThemeListView(m_pLists.Get(0));

	// Load prefs 
	g_approxDelayMsCC = GetPrivateProfileInt("LIVE_CONFIGS", "CC_DELAY", 250, g_SNMIniFn.Get());

	// WDL GUI init
	m_vwnd_painter.SetGSC(WDL_STYLE_GetSysColor);
	m_parentVwnd.SetRealParent(m_hwnd);

	m_txtConfig.SetID(TXTID_CONFIG);
	m_txtConfig.SetText("Config:");
	m_parentVwnd.AddChild(&m_txtConfig);

	m_cbConfig.SetID(COMBOID_CONFIG);
	for (int i=0; i < SNM_LIVECFG_NB_CONFIGS; i++)
	{
		char cfg[8] = "";
		_snprintf(cfg, 8, "%d", i+1);
		m_cbConfig.AddItem(cfg);
	}
	m_cbConfig.SetCurSel(g_configId);
	m_parentVwnd.AddChild(&m_cbConfig);

	m_btnEnable.SetID(BUTTONID_ENABLE);
	m_parentVwnd.AddChild(&m_btnEnable);

	m_txtInputTr.SetID(TXTID_INPUT_TRACK);
	m_txtInputTr.SetText("Input track:");
	m_parentVwnd.AddChild(&m_txtInputTr);

	m_cbInputTr.SetID(COMBOID_INPUT_TRACK);
	FillComboInputTrack();
	m_parentVwnd.AddChild(&m_cbInputTr);

	m_btnMuteOthers.SetID(BUTTONID_MUTE_OTHERS);
	m_parentVwnd.AddChild(&m_btnMuteOthers);

	m_btnAutoSelect.SetID(BUTTONID_AUTO_SELECT);
	m_parentVwnd.AddChild(&m_btnAutoSelect);

	Update();
}

void SNM_LiveConfigsWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	MidiLiveConfig* lc = g_liveConfigs.Get();
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
					new SNM_MidiLiveScheduledJob(g_configId, 0, g_configId, item->m_cc, -1, 0, g_hwndParent);
				AddOrReplaceScheduledJob(job);
			}
			break;
		case SNM_LIVECFG_EDIT_DESC_MSG:
			if (item) 
				m_pLists.Get(0)->EditListItem((SWS_ListItem*)item, 1);
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
				if (BrowseResourcePath("S&M - Load track template", "TrackTemplates", "REAPER Track Template (*.RTrackTemplate)\0*.RTrackTemplate\0", filename, BUFFER_SIZE))
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
				if (BrowseResourcePath("S&M - FX Chain", "FXChains", "REAPER FX Chain (*.RfxChain)\0*.RfxChain\0", filename, BUFFER_SIZE))
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
			if (item) 
				m_pLists.Get(0)->EditListItem((SWS_ListItem*)item, 5);
			break;
		case SNM_LIVECFG_CLEAR_ON_ACTION_MSG:
		{
			bool updt = false;
			while(item) {
				item->m_onAction.Set("");
				updt = true;
				item = (MidiLiveItem*)m_pLists.Get(0)->EnumSelected(&x);
			}
			if (updt) {
				Update();
				Undo_OnStateChangeEx(SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
			}
			break;
		}
		case SNM_LIVECFG_EDIT_OFF_ACTION_MSG:
			if (item) 
				m_pLists.Get(0)->EditListItem((SWS_ListItem*)item, 6);
			break;
		case SNM_LIVECFG_CLEAR_OFF_ACTION_MSG:
		{
			bool updt = false;
			while(item) {
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
				item->m_track= NULL;
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
				lc->m_enable[g_configId] = !(lc->m_enable[g_configId]);
				Undo_OnStateChangeEx(SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
				// Retreive the related toggle action and refresh toolbars
				char customCmdId[SNM_MAX_ACTION_CUSTID_LEN];
				_snprintf(customCmdId, SNM_MAX_ACTION_CUSTID_LEN, "%s%d", "_S&M_TOGGLE_LIVE_CFG", g_configId+1);
				RefreshToolbar(NamedCommandLookup(customCmdId));
			}
			break;
		case BUTTONID_MUTE_OTHERS:
			if (!HIWORD(wParam) || HIWORD(wParam)==600)
			{
				lc->m_muteOthers[g_configId] = !(lc->m_muteOthers[g_configId]);
				Undo_OnStateChangeEx(SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
			}
			break;
		case BUTTONID_AUTO_SELECT:
			if (!HIWORD(wParam) || HIWORD(wParam)==600)
			{
				lc->m_autoSelect[g_configId] = !(lc->m_autoSelect[g_configId]);
				Undo_OnStateChangeEx(SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
			}
			break;
		case COMBOID_INPUT_TRACK:
			if (HIWORD(wParam)==CBN_SELCHANGE)
			{
				if (!m_cbInputTr.GetCurSel())
					lc->m_inputTr[g_configId] = NULL;
				else
					lc->m_inputTr[g_configId] = CSurf_TrackFromID(m_cbInputTr.GetCurSel(), false);
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
			if (wParam >= SNM_LIVECFG_SET_TRACK_MSG && wParam < SNM_LIVECFG_CLEAR_TRACK_MSG) 
			{
				bool updt = false;
				while(item)
				{
					item->m_track = CSurf_TrackFromID((int)wParam - SNM_LIVECFG_SET_TRACK_MSG, false);
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
#ifdef _SNM_PRESETS
			else if (wParam >= SNM_LIVECFG_SET_PRESETS_MSG && wParam < SNM_LIVECFG_CLEAR_PRESETS_MSG) 
			{
				bool updt = false;
				while(item)
				{
					int fx = m_lastFXPresetMsg[0][(int)wParam - SNM_LIVECFG_SET_PRESETS_MSG];
					int preset = m_lastFXPresetMsg[1][(int)wParam - SNM_LIVECFG_SET_PRESETS_MSG];
					UpdatePresetConf(fx+1, preset, &(item->m_presets));
					item->m_fxChain.Set("");
					item->m_trTemplate.Set("");
					updt = true;
					item = (MidiLiveItem*)m_pLists.Get(0)->EnumSelected(&x);
				}
				if (updt) {
					Update();
					Undo_OnStateChangeEx(SNM_LIVECFG_UNDO_STR, UNDO_STATE_MISCCFG, -1);
				}
			}
#endif
			else
				Main_OnCommand((int)wParam, (int)lParam);
			break;
	}
}

void AddFXSubMenu(HMENU* _menu, MediaTrack* _tr, WDL_FastString* _curPresetConf)
{
	memset(g_pLiveConfigsWnd->m_lastFXPresetMsg[0], -1, SNM_LIVECFG_MAX_PRESET_COUNT * sizeof(int));
	memset(g_pLiveConfigsWnd->m_lastFXPresetMsg[1], -1, SNM_LIVECFG_MAX_PRESET_COUNT * sizeof(int));

	int fxCount = (_tr ? TrackFX_GetCount(_tr) : 0);
	if (!fxCount) {
		AddToMenu(*_menu, "(No FX on track)", 0, -1, false, MF_GRAYED);
		return;
	}

	SNM_FXSummaryParser p(_tr);
	WDL_PtrList<SNM_FXSummary>* summaries = p.GetSummaries();
	if (summaries && fxCount == summaries->GetSize())
	{
		char fxName[512];
		int msgCpt = 0;
		//JFB TODO: check max. msgCpt value..
		for(int i = 0; i < fxCount; i++) 
		{
			if(TrackFX_GetFXName(_tr, i, fxName, 512))
			{
				HMENU fxSubMenu = CreatePopupMenu();
				WDL_PtrList_DeleteOnDestroy<WDL_FastString> names;
				SNM_FXSummary* sum = summaries->Get(i);
				int presetCount = (sum ? getPresetNames(sum->m_type.Get(), sum->m_realName.Get(), &names) : 0);
				if (presetCount)
				{
					int curSel = GetPresetFromConf(i, _curPresetConf, presetCount);
					AddToMenu(fxSubMenu, "None (unchanged)", SNM_LIVECFG_SET_PRESETS_MSG + msgCpt, -1, false, !curSel ? MFS_CHECKED : MFS_UNCHECKED);
					g_pLiveConfigsWnd->m_lastFXPresetMsg[0][msgCpt] = i; 
					g_pLiveConfigsWnd->m_lastFXPresetMsg[1][msgCpt++] = 0; 
					for(int j = 0; j < presetCount; j++) {
						AddToMenu(fxSubMenu, names.Get(j)->Get(), SNM_LIVECFG_SET_PRESETS_MSG + msgCpt, -1, false, ((j+1)==curSel) ? MFS_CHECKED : MFS_UNCHECKED);
						g_pLiveConfigsWnd->m_lastFXPresetMsg[0][msgCpt] = i; 
						g_pLiveConfigsWnd->m_lastFXPresetMsg[1][msgCpt++] = j+1; 
					}
				}
				WDL_FastString fxNameId;
				fxNameId.SetFormatted(512, "FX %d - %s", i+1, fxName);
				AddSubMenu(*_menu, fxSubMenu, fxNameId.Get(), -1, presetCount ? MFS_ENABLED : MF_GRAYED);
			}
		}
	}
	else
		AddToMenu(*_menu, "(Unknown FX on track)", 0, -1, false, MF_GRAYED);
}

HMENU SNM_LiveConfigsWnd::OnContextMenu(int x, int y)
{
	HMENU hMenu = NULL;
	int iCol;
	if (MidiLiveItem* item = (MidiLiveItem*)m_pLists.Get(0)->GetHitItem(x, y, &iCol))
	{
		switch(iCol)
		{
			case COL_CC:
			{
				hMenu = CreatePopupMenu();
				AddToMenu(hMenu, "Perform", SNM_LIVECFG_PERFORM_MSG);
				break;
			}
			case COL_COMMENT:
			{
				hMenu = CreatePopupMenu();
				AddToMenu(hMenu, "Clear", SNM_LIVECFG_CLEAR_DESC_MSG);
				AddToMenu(hMenu, "Edit", SNM_LIVECFG_EDIT_DESC_MSG);
				break;
			}
			case COL_TR:
			{
				hMenu = CreatePopupMenu();
				AddToMenu(hMenu, "Clear", SNM_LIVECFG_CLEAR_TRACK_MSG);
				for (int i=1; i <= GetNumTracks(); i++)
				{
					char str[SNM_MAX_TRACK_NAME_LEN] = "";
					char* name = (char*)GetSetMediaTrackInfo(CSurf_TrackFromID(i,false), "P_NAME", NULL);
					_snprintf(str, SNM_MAX_TRACK_NAME_LEN, "[%d] \"%s\"", i, name);
					AddToMenu(hMenu, str, SNM_LIVECFG_SET_TRACK_MSG + i);
				}
				break;
			}
			case COL_TRT:
			{
				hMenu = CreatePopupMenu();
				if (item->m_track) {
					AddToMenu(hMenu, "Clear", SNM_LIVECFG_CLEAR_TRACK_TEMPLATE_MSG);
					AddToMenu(hMenu, "Load track template...", SNM_LIVECFG_LOAD_TRACK_TEMPLATE_MSG);
				}
				else AddToMenu(hMenu, "(No track)", 0, -1, false, MF_GRAYED);
				break;
			}
			case COL_FXC:
			{
				hMenu = CreatePopupMenu();
				if (item->m_track) {
					if (!item->m_trTemplate.GetLength()) {
						AddToMenu(hMenu, "Clear", SNM_LIVECFG_CLEAR_FXCHAIN_MSG);
						AddToMenu(hMenu, "Load FX Chain...", SNM_LIVECFG_LOAD_FXCHAIN_MSG);
					}
					else AddToMenu(hMenu, "(Track template overrides)", 0, -1, false, MF_GRAYED);
				}
				else AddToMenu(hMenu, "(No track)", 0, -1, false, MF_GRAYED);
				break;
			}
#ifdef _SNM_PRESETS
			case COL_PRESET:
			{
				hMenu = CreatePopupMenu();
				if (item->m_track) {
					if (item->m_trTemplate.GetLength() && !item->m_fxChain.GetLength())
						AddToMenu(hMenu, "(Track template overrides)", 0, -1, false, MFS_GRAYED);
					else if (item->m_fxChain.GetLength())
						AddToMenu(hMenu, "(FX Chain overrides)", 0, -1, false, MFS_GRAYED);
					else {
						AddToMenu(hMenu, "Clear all presets (unchanged)", SNM_LIVECFG_CLEAR_PRESETS_MSG);
						AddToMenu(hMenu, SWS_SEPARATOR, 0);
						AddFXSubMenu(&hMenu, item->m_track, &(item->m_presets));
					}
				}
				else AddToMenu(hMenu, "(No track)", 0, -1, false, MFS_GRAYED);
				break;
			}
#endif
			case COL_ACTION_ON:
			{
				hMenu = CreatePopupMenu();
				AddToMenu(hMenu, "Clear", SNM_LIVECFG_CLEAR_ON_ACTION_MSG);
				AddToMenu(hMenu, "Edit", SNM_LIVECFG_EDIT_ON_ACTION_MSG);
				break;
			}
			case COL_ACTION_OFF:
			{
				hMenu = CreatePopupMenu();
				AddToMenu(hMenu, "Clear", SNM_LIVECFG_CLEAR_OFF_ACTION_MSG);
				AddToMenu(hMenu, "Edit", SNM_LIVECFG_EDIT_OFF_ACTION_MSG);
				break;
			}
		}
	}
	return hMenu;
}

void SNM_LiveConfigsWnd::OnDestroy() 
{
	// save prefs
	char cDelay[8];
	_snprintf(cDelay, 8, "%d", g_approxDelayMsCC);
	WritePrivateProfileString("LIVE_CONFIGS", "CC_DELAY", cDelay, g_SNMIniFn.Get()); 

	m_cbConfig.Empty();
	m_cbInputTr.Empty();
}

int SNM_LiveConfigsWnd::OnKey(MSG* msg, int iKeyState) 
{
	if (msg->message == WM_KEYDOWN && msg->wParam == VK_DELETE && !iKeyState)
	{
		this->OnCommand(SNM_LIVECFG_CLEAR_CC_ROW_MSG, 0);
		return 1;
	}
	return 0;
}


void SNM_LiveConfigsWnd::DrawControls(LICE_IBitmap* _bm, const RECT* _r, int* _tooltipHeight)
{
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

	m_btnEnable.SetCheckState(g_liveConfigs.Get()->m_enable[g_configId]);
	m_btnEnable.SetTextLabel("Enable", -1, font);
	if (!SNM_AutoVWndPosition(&m_btnEnable, NULL, _r, &x0, _r->top, h, 5))
		return;

	m_txtInputTr.SetFont(font);
	if (!SNM_AutoVWndPosition(&m_txtInputTr, NULL, _r, &x0, _r->top, h, 5))
		return;

	m_cbInputTr.SetFont(font);
	int sel=0;
	for (int i=1; i <= GetNumTracks(); i++) {
		if (g_liveConfigs.Get()->m_inputTr[g_configId] == CSurf_TrackFromID(i,false)) {
			sel = i;
			break;
		}
	}
	m_cbInputTr.SetCurSel(sel);
	if (!SNM_AutoVWndPosition(&m_cbInputTr, &m_txtInputTr, _r, &x0, _r->top, h))
		return;

	m_btnMuteOthers.SetCheckState(g_liveConfigs.Get()->m_muteOthers[g_configId]);
	m_btnMuteOthers.SetTextLabel("Mute all but active track", -1, font);
	if (!SNM_AutoVWndPosition(&m_btnMuteOthers, NULL, _r, &x0, _r->top, h, 5))
		return;

	m_btnAutoSelect.SetCheckState(g_liveConfigs.Get()->m_autoSelect[g_configId]);
	m_btnAutoSelect.SetTextLabel("Auto track selection", -1, font);
	if (!SNM_AutoVWndPosition(&m_btnAutoSelect, NULL, _r, &x0, _r->top, h, 5))
		return;

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


///////////////////////////////////////////////////////////////////////////////

void SNM_LiveCfg_TLChangeSchedJob::Perform()
{
	// check or model consistency against the track list update
	for (int i=0; i < g_liveCCConfigs.Get()->GetSize(); i++)
	{
		for (int j = 0; j < g_liveCCConfigs.Get()->Get(i)->GetSize(); j++)
		{
			MidiLiveItem* item = g_liveCCConfigs.Get()->Get(i)->Get(j);
			if (item && item->m_track && CSurf_TrackToID(item->m_track, false) <= 0) // bad track (or master)
				item->m_track = NULL;
		}
		if (g_liveConfigs.Get()->m_inputTr[i] && CSurf_TrackToID(g_liveConfigs.Get()->m_inputTr[i], false) <= 0)
			g_liveConfigs.Get()->m_inputTr[i] = NULL;

//		g_lastPerformedMIDIVal[i] = -1;
//		g_lastDeactivateCmd[i][0] = -1;
	}

	if (g_pLiveConfigsWnd)
	{
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

		if (MidiLiveConfig* lc = g_liveConfigs.Get())
		{
			int success;
			lc->m_enable[configId] = lp.gettoken_int(2);
			lc->m_autoRcv[configId] = lp.gettoken_int(3);
			lc->m_muteOthers[configId] = lp.gettoken_int(4);
			stringToGuid(lp.gettoken_str(5), &g);
			lc->m_inputTr[configId] = GuidToTrack(&g);
			lc->m_autoSelect[configId] = lp.gettoken_int(6, &success);
			if (!success) // for historical reasons..
				lc->m_autoSelect[configId] = 1;
		}

		if (WDL_PtrList<MidiLiveItem>* ccConfigs = g_liveCCConfigs.Get()->Get(configId))
		{
			char linebuf[4096];
			while(true)
			{
				if (!ctx->GetLine(linebuf,sizeof(linebuf)) && !lp.parse(linebuf))
				{
					if (lp.gettoken_str(0)[0] == '>')
						break;						
					else
					{
						MidiLiveItem* item = ccConfigs->Get(lp.gettoken_int(0));
						if (item)
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
						}
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
	char curLine[SNM_MAX_CHUNK_LINE_LENGTH] = "", strId[128] = "";
	GUID g; 
	bool firstCfg = true;
	for (int i=0; i < g_liveCCConfigs.Get()->GetSize(); i++)
	{
	  for (int j = 0; j < g_liveCCConfigs.Get()->Get(i)->GetSize(); j++)
	  {
			MidiLiveItem* item = g_liveCCConfigs.Get()->Get(i)->Get(j);
			if (item && !item->IsDefault()) // avoid useless data in RPP files
			{
				if (firstCfg)
				{
					firstCfg = false;
					if (MidiLiveConfig* lc = g_liveConfigs.Get())
					{
						if (lc->m_inputTr[i] && CSurf_TrackToID(lc->m_inputTr[i], false) > 0) 
							g = *(GUID*)GetSetMediaTrackInfo(lc->m_inputTr[i], "GUID", NULL);
						else 
							g = GUID_NULL;
						guidToString(&g, strId);

						_snprintf(curLine, SNM_MAX_CHUNK_LINE_LENGTH, "<S&M_MIDI_LIVE %d %d %d %d \"%s\" %d", 
							i+1,
							lc->m_enable[i],
							lc->m_autoRcv[i],
							lc->m_muteOthers[i],
							strId,
							lc->m_autoSelect[i]);
						ctx->AddLine(curLine);
					}
				}

				if (item->m_track && CSurf_TrackToID(item->m_track, false) > 0) 
					g = *(GUID*)GetSetMediaTrackInfo(item->m_track, "GUID", NULL);
				else 
					g = GUID_NULL;
				guidToString(&g, strId);

				_snprintf(curLine, SNM_MAX_CHUNK_LINE_LENGTH, "%d \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"", 
					item->m_cc, 
					item->m_desc.Get(), 
					strId, 
					item->m_trTemplate.Get(), 
					item->m_fxChain.Get(), 
					item->m_presets.Get(), 
					item->m_onAction.Get(), 
					item->m_offAction.Get());
				ctx->AddLine(curLine);
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
	g_liveConfigs.Get()->Clear();

	g_liveCCConfigs.Cleanup();
	g_liveCCConfigs.Get()->Empty(true);
	for (int i=0; i < SNM_LIVECFG_NB_CONFIGS; i++) 
	{
		g_liveCCConfigs.Get()->Add(new WDL_PtrList_DeleteOnDestroy<MidiLiveItem>);
		for (int j = 0; j < NB_CC_VALUES; j++)
			g_liveCCConfigs.Get()->Get(i)->Add(new MidiLiveItem(j, "", NULL, "", "", "", "", ""));
	}
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
void ApplyLiveConfig(MIDI_COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd) 
{
	// Absolute CC only
	if (!_relmode && _valhw < 0)
	{
		SNM_MidiLiveScheduledJob* job = 
			new SNM_MidiLiveScheduledJob((int)_ct->user, g_approxDelayMsCC, (int)_ct->user, _val, _valhw, _relmode, _hwnd);

		// delay:
		// Avoid to stuck REPAER when a bunch of CCs are received (which is the standard case with HW knobs, faders, ..) 
		// but just process the last "stable" one => we do this thanks to SNM_ScheduledJob
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

// Here we go
void SNM_MidiLiveScheduledJob::Perform()
{
	MidiLiveConfig* lc = g_liveConfigs.Get();
	if (!lc || m_val == lc->m_lastMIDIVal[m_cfgId]) 
		return;
	else
		lc->m_lastMIDIVal[m_cfgId] = m_val;

	MidiLiveItem* cfg = g_liveCCConfigs.Get()->Get(m_cfgId)->Get(m_val);
	if (cfg && !cfg->IsDefault())
	{
		if (lc->m_enable[m_cfgId])
		{
			WDL_PtrList<MediaTrack> otherConfigTracks;
			for (int i=0; i < g_liveCCConfigs.Get()->Get(m_cfgId)->GetSize(); i++)
			{
				MidiLiveItem* cfgOther = g_liveCCConfigs.Get()->Get(m_cfgId)->Get(i);
				// note: a same track can be present in several rows
				if (cfgOther && cfgOther->m_track && cfgOther->m_track != cfg->m_track && otherConfigTracks.Find(cfgOther->m_track) == -1)
					otherConfigTracks.Add(cfgOther->m_track);
			}

			// save selected tracks, unselect all track
			WDL_PtrList<MediaTrack> selTracks;
			if (lc->m_autoSelect[m_cfgId])
			{
				for (int i=0; i <= GetNumTracks(); i++) // include master
				{
					MediaTrack* tr = CSurf_TrackFromID(i, false);
					if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL)) {
						if (tr != cfg->m_track && otherConfigTracks.Find(tr) == -1) 
							selTracks.Add(tr);
						GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i0);	
					}
				}
			}

			// run desactivate action (of the previous config)
			// if auto-selection is enabled, only the related track is selected while 
			// performing the action. other tracks selections are restored later
			if (lc->m_lastDeactivateCmd[m_cfgId][0] > 0)
			{
				MediaTrack* deactiveTr = lc->m_lastDeactivateCmd[m_cfgId][4] > 0 ? CSurf_TrackFromID(lc->m_lastDeactivateCmd[m_cfgId][4], false) : NULL;
				if (deactiveTr && lc->m_autoSelect[m_cfgId])
					GetSetMediaTrackInfo(deactiveTr, "I_SELECTED", &g_i1); 

				if (!KBD_OnMainActionEx(lc->m_lastDeactivateCmd[m_cfgId][0], lc->m_lastDeactivateCmd[m_cfgId][1], lc->m_lastDeactivateCmd[m_cfgId][2], lc->m_lastDeactivateCmd[m_cfgId][3], g_hwndParent, NULL))
					Main_OnCommand(lc->m_lastDeactivateCmd[m_cfgId][0],0);

				if (deactiveTr && lc->m_autoSelect[m_cfgId])
					GetSetMediaTrackInfo(deactiveTr, "I_SELECTED", &g_i0); // no track selected after that..

				lc->m_lastDeactivateCmd[m_cfgId][0] = -1;
			}

			if (cfg->m_track)
			{
				// avoid glitches AFAP: in any case (mute pref ignored), we mute!
				// we'll restore mute state of the focused track after processing
				DWORD muteTime = 0;
				bool mute = *(bool*)GetSetMediaTrackInfo(cfg->m_track, "B_MUTE", NULL);
				if (!mute) {
					GetSetMediaTrackInfo(cfg->m_track, "B_MUTE", &g_bTrue);
					muteTime = GetTickCount();
				}

				// Mute all other tracks of that config
				if (lc->m_muteOthers[m_cfgId]) {
					for (int i=0; i < otherConfigTracks.GetSize(); i++) {
						MediaTrack* cfgOtherTr = otherConfigTracks.Get(i);
						if (cfgOtherTr) {
							GetSetMediaTrackInfo(cfgOtherTr, "B_MUTE", &g_bTrue);
							muteReceives(lc->m_inputTr[m_cfgId], cfgOtherTr, true);
						}
					}
				}

				// track configuration
				SNM_ChunkParserPatcher* p = NULL;
				if (cfg->m_trTemplate.GetLength())
				{
					p = new SNM_ChunkParserPatcher(cfg->m_track);
					WDL_FastString chunk;
					char filename[BUFFER_SIZE];
					GetFullResourcePath("TrackTemplates", cfg->m_trTemplate.Get(), filename, BUFFER_SIZE);
					if (LoadChunk(filename, &chunk) && chunk.GetLength())
						applyTrackTemplate(cfg->m_track, &chunk, true, p);
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
#ifdef _SNM_PRESETS
				else if (cfg->m_presets.GetLength())
				{
					WaitForTrackMute(&muteTime); // 0 is ignored
					triggerFXUserPreset(cfg->m_track, &(cfg->m_presets));
				}
#endif
				WaitForTrackMute(&muteTime); // 0 is ignored
				delete p; // + auto commit (if needed)


				// select track (the only one selected at this point, if auto-selection is enabled)
				if (lc->m_autoSelect[m_cfgId])
					GetSetMediaTrackInfo(cfg->m_track, "I_SELECTED", &g_i1);

				//	unmute the config track (or restore its mute state)			
				if (lc->m_muteOthers[m_cfgId]) {
					GetSetMediaTrackInfo(cfg->m_track, "B_MUTE", &g_bFalse);
					muteReceives(lc->m_inputTr[m_cfgId], cfg->m_track, false);
				}
				else
					GetSetMediaTrackInfo(cfg->m_track, "B_MUTE", &mute);

			} // end of track processing

			// perform activate action
			// if auto-selection is used, only the related track is selected at this point
			if (cfg->m_onAction.GetLength())
			{
				int cmd = NamedCommandLookup(cfg->m_onAction.Get());
				if (cmd > 0 && !KBD_OnMainActionEx(cmd, m_val, m_valhw, m_relmode, g_hwndParent, NULL))
					Main_OnCommand(cmd,0);
			}

			// restore other (i.e. not part of this config) selected tracks
			if (lc->m_autoSelect[m_cfgId])
				for (int k=0; k < selTracks.GetSize(); k++)
					GetSetMediaTrackInfo(selTracks.Get(k), "I_SELECTED", &g_i1);

			// (just) prepare desactivate action
			if (cfg->m_offAction.GetLength())
			{
				int cmd = NamedCommandLookup(cfg->m_offAction.Get());
				if (cmd > 0)
				{
					lc->m_lastDeactivateCmd[m_cfgId][0] = cmd;
					lc->m_lastDeactivateCmd[m_cfgId][1] = m_val;
					lc->m_lastDeactivateCmd[m_cfgId][2] = m_valhw;
					lc->m_lastDeactivateCmd[m_cfgId][3] = m_relmode;
					lc->m_lastDeactivateCmd[m_cfgId][4] = cfg->m_track ? CSurf_TrackToID(cfg->m_track, false) : -1;
				}
			}
			else
				lc->m_lastDeactivateCmd[m_cfgId][0] = -1;

		} // if (lc->m_enable[m_cfgId])

	} // if (cfg && !cfg->IsDefault())

	if (g_pLiveConfigsWnd)
		g_pLiveConfigsWnd->SelectByCCValue(m_cfgId, m_val);
}

void ToggleEnableLiveConfig(COMMAND_T* _ct) {
	g_liveConfigs.Get()->m_enable[(int)_ct->user] = !(g_liveConfigs.Get()->m_enable[(int)_ct->user]);
	if (g_pLiveConfigsWnd && (g_configId == (int)_ct->user))
		g_pLiveConfigsWnd->Update();
}

bool IsLiveConfigEnabled(COMMAND_T* _ct) {
	return (g_liveConfigs.Get()->m_enable[(int)_ct->user] == 1);
}
