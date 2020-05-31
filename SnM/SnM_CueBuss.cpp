/******************************************************************************
/ SnM_CueBuss.cpp
/
/ Copyright (c) 2009 and later Jeffos
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

#include "stdafx.h"

#include "SnM.h"
#include "SnM_Chunk.h"
#include "SnM_CueBuss.h"
#include "SnM_Dlg.h"
#include "SnM_Routing.h"
#include "SnM_Track.h"
#include "SnM_Util.h"

#include <WDL/localize/localize.h>
#include <WDL/projectcontext.h>

HWND g_cueBussHwnd = NULL;
int g_cueBussConfId = 0; // not saved in prefs yet
int g_cueBussLastSettingsId = 0; // 0 for ascendant compatibility
bool g_cueBussDisableSave = false;

///////////////////////////////////////////////////////////////////////////////
// Cue buss 
///////////////////////////////////////////////////////////////////////////////

// adds a receive (with vol & pan from source track for pre-fader)
// _srcTr:  source track (unchanged)
// _destTr: destination track
// _type:   reaper's type
//          0=Post-Fader (Post-Pan), 1=Pre-FX, 2=deprecated, 3=Pre-Fader (Post-FX)
// _p:      for multi-patch optimization, current destination track's SNM_SendPatcher
bool AddReceiveWithVolPan(MediaTrack * _srcTr, MediaTrack * _destTr, int _type, SNM_SendPatcher* _p)
{
	bool update = false;
	char vol[32] = "1.00000000000000";
	char pan[32] = "0.00000000000000";

	// if pre-fader, then re-copy track vol/pan
	if (_type == 3)
	{
		SNM_ChunkParserPatcher p(_srcTr, false);
		p.SetWantsMinimalState(true);
		if (p.Parse(SNM_GET_CHUNK_CHAR, 1, "TRACK", "VOLPAN", 0, 1, vol, NULL, "MUTESOLO") > 0 &&
			p.Parse(SNM_GET_CHUNK_CHAR, 1, "TRACK", "VOLPAN", 0, 2, pan, NULL, "MUTESOLO") > 0)
		{
			update = _p->AddReceive(_srcTr, _type, vol, pan) > 0;
		}
	}

	// default volume
	if (!update && snprintfStrict(vol, sizeof(vol), "%.14f", *ConfigVar<double>("defsendvol")) > 0)
		update = _p->AddReceive(_srcTr, _type, vol, pan) > 0;
	return update;
}

// _type: 0=Post-Fader (Post-Pan), 1=Pre-FX, 2=deprecated, 3=Pre-Fader (Post-FX)
// _undoMsg: NULL=no undo
bool CueBuss(const char* _undoMsg, const char* _busName, int _type, bool _showRouting,
			 int _soloDefeat, char* _trTemplatePath, bool _sendToMaster, int* _hwOuts) 
{
	if (!SNM_CountSelectedTracks(NULL, false))
		return false;

	WDL_FastString tmplt;
	if (_trTemplatePath && (!FileOrDirExists(_trTemplatePath) || !LoadChunk(_trTemplatePath, &tmplt) || !tmplt.GetLength()))
	{
		char msg[SNM_MAX_PATH] = "";
		lstrcpyn(msg, __LOCALIZE("Cue buss not created!\nNo track template file defined","sws_DLG_149"), sizeof(msg));
		if (*_trTemplatePath)
			snprintf(msg, sizeof(msg), __LOCALIZE_VERFMT("Cue buss not created!\nTrack template not found (or empty): %s","sws_DLG_149"), _trTemplatePath);
		MessageBox(GetMainHwnd(), msg, __LOCALIZE("S&M - Error","sws_DLG_149"), MB_OK);
		return false;
	}

	bool updated = false;
	MediaTrack * cueTr = NULL;
	SNM_SendPatcher* p = NULL;
	for (int i=1; i <= GetNumTracks(); i++) // skip master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i0);

			// add the buss track, done once!
			if (!cueTr)
			{
				InsertTrackAtIndex(GetNumTracks(), false);
				TrackList_AdjustWindows(false);
				cueTr = CSurf_TrackFromID(GetNumTracks(), false);
				GetSetMediaTrackInfo(cueTr, "P_NAME", (void*)_busName);

				p = new SNM_SendPatcher(cueTr);

				if (tmplt.GetLength())
				{
					WDL_FastString chunk;
					MakeSingleTrackTemplateChunk(&tmplt, &chunk, true, true, false);
					ApplyTrackTemplate(cueTr, &chunk, false, false, p);
				}
				updated = true;
			}

			// add a send
			if (cueTr && p && tr != cueTr)
				AddReceiveWithVolPan(tr, cueTr, _type, p);
		}
	}

	if (cueTr && p)
	{
		// send to master/parent init
		if (!tmplt.GetLength())
		{
			// solo defeat
			if (_soloDefeat) {
				char one[2] = "1";
				updated |= (p->ParsePatch(SNM_SET_CHUNK_CHAR, 1, "TRACK", "MUTESOLO", 0, 3, one) > 0);
			}
			
			// master/parend send
			WDL_FastString mainSend;
			mainSend.SetFormatted(SNM_MAX_CHUNK_LINE_LENGTH, "MAINSEND %d 0", _sendToMaster?1:0);

			// adds hw outputs
			if (_hwOuts)
			{
				int monoHWCount=0; 
				while (GetOutputChannelName(monoHWCount)) monoHWCount++;

				bool cr = false;
				for(int i=0; i<SNM_MAX_HW_OUTS; i++)
				{
					if (_hwOuts[i])
					{
						if (!cr) {
							mainSend.Append("\n"); 
							cr = true;
						}
						if (_hwOuts[i] >= monoHWCount) 
							mainSend.AppendFormatted(32, "HWOUT %d ", (_hwOuts[i]-monoHWCount) | 1024);
						else
							mainSend.AppendFormatted(32, "HWOUT %d ", _hwOuts[i]-1);

						mainSend.Append("0 ");
						mainSend.AppendFormatted(20, "%.14f ", *ConfigVar<double>("defhwvol"));
						mainSend.Append("0.00000000000000 0 0 0 -1.00000000000000 -1\n");
					}
				}
				if (!cr)
					mainSend.Append("\n"); // hot
			}

			// patch both updates (no break keyword here: new empty track)
			updated |= p->ReplaceLine("TRACK", "MAINSEND", 1, 0, mainSend.Get());
		}

		p->Commit();
		delete p;

		if (updated)
		{
			GetSetMediaTrackInfo(cueTr, "I_SELECTED", &g_i1);
			UpdateTimeline();
			ScrollSelTrack(true, true);
			if (_showRouting) 
				Main_OnCommand(40293, 0);
			if (_undoMsg)
				Undo_OnStateChangeEx2(NULL, _undoMsg, UNDO_STATE_ALL, -1);
		}
	}
	return updated;
}

bool CueBuss(const char* _undoMsg, int _confId)
{
	if (_confId<0 || _confId>=SNM_MAX_CUE_BUSS_CONFS)
		_confId = g_cueBussLastSettingsId;
	char busName[64]="", trTemplatePath[SNM_MAX_PATH]="";
	int reaType, soloGrp, hwOuts[SNM_MAX_HW_OUTS];
	bool trTemplate,showRouting,sendToMaster;
	ReadCueBusIniFile(_confId, busName, sizeof(busName), &reaType, &trTemplate, trTemplatePath, sizeof(trTemplatePath), &showRouting, &soloGrp, &sendToMaster, hwOuts);
	bool updated = CueBuss(_undoMsg, busName, reaType, showRouting, soloGrp, trTemplate?trTemplatePath:NULL, sendToMaster, hwOuts);
	g_cueBussLastSettingsId = _confId;
	return updated;
}

void CueBuss(COMMAND_T* _ct) {
	CueBuss(SWS_CMD_SHORTNAME(_ct), (int)_ct->user);
}

void ReadCueBusIniFile(int _confId, char* _busName, int _busNameSz, int* _reaType, bool* _trTemplate, char* _trTemplatePath, int _trTemplatePathSz, bool* _showRouting, int* _soloDefeat, bool* _sendToMaster, int* _hwOuts)
{
	if (_confId>=0 && _confId<SNM_MAX_CUE_BUSS_CONFS && _busName && _reaType && _trTemplate && _trTemplatePath && _showRouting && _soloDefeat && _sendToMaster && _hwOuts)
	{
		char iniSection[64]="";
		if (snprintfStrict(iniSection, sizeof(iniSection), "CueBuss%d", _confId+1) > 0)
		{
			char buf[16]="", slot[16]="";
			GetPrivateProfileString(iniSection,"name","",_busName,_busNameSz,g_SNM_IniFn.Get());
			GetPrivateProfileString(iniSection,"reatype","3",buf,16,g_SNM_IniFn.Get());
			*_reaType = atoi(buf); // 0 if failed 
			GetPrivateProfileString(iniSection,"track_template_enabled","0",buf,16,g_SNM_IniFn.Get());
			*_trTemplate = (atoi(buf) == 1);
			GetPrivateProfileString(iniSection,"track_template_path","",_trTemplatePath,_trTemplatePathSz,g_SNM_IniFn.Get());
			GetPrivateProfileString(iniSection,"show_routing","1",buf,16,g_SNM_IniFn.Get());
			*_showRouting = (atoi(buf) == 1);
			GetPrivateProfileString(iniSection,"send_to_masterparent","0",buf,16,g_SNM_IniFn.Get());
			*_sendToMaster = (atoi(buf) == 1);
			GetPrivateProfileString(iniSection,"solo_defeat","1",buf,16,g_SNM_IniFn.Get());
			*_soloDefeat = atoi(buf);
			for (int i=0; i<SNM_MAX_HW_OUTS; i++)
			{
				if (snprintfStrict(slot, sizeof(slot), "hwout%d", i+1) > 0)
					GetPrivateProfileString(iniSection,slot,"0",buf,sizeof(buf),g_SNM_IniFn.Get());
				else
					*buf = '\0';
				_hwOuts[i] = atoi(buf);
			}
		}
	}
}

void SaveCueBusIniFile(int _confId, const char* _busName, int _type, bool _trTemplate, const char* _trTemplatePath, bool _showRouting, int _soloDefeat, bool _sendToMaster, int* _hwOuts)
{
	if (_confId>=0 && _confId<SNM_MAX_CUE_BUSS_CONFS && _busName && _trTemplatePath && _hwOuts)
	{
		char iniSection[64]="";
		if (snprintfStrict(iniSection, sizeof(iniSection), "CueBuss%d", _confId+1) > 0)
		{
			char buf[16]="", slot[16]="";
			WDL_FastString escapedStr;
			escapedStr.SetFormatted(256, "\"%s\"", _busName);
			WritePrivateProfileString(iniSection,"name",escapedStr.Get(),g_SNM_IniFn.Get());
			if (snprintfStrict(buf, sizeof(buf), "%d" ,_type) > 0)
				WritePrivateProfileString(iniSection,"reatype",buf,g_SNM_IniFn.Get());
			WritePrivateProfileString(iniSection,"track_template_enabled",_trTemplate ? "1" : "0",g_SNM_IniFn.Get());
			escapedStr.SetFormatted(SNM_MAX_PATH, "\"%s\"", _trTemplatePath);
			WritePrivateProfileString(iniSection,"track_template_path",escapedStr.Get(),g_SNM_IniFn.Get());
			WritePrivateProfileString(iniSection,"show_routing",_showRouting ? "1" : "0",g_SNM_IniFn.Get());
			WritePrivateProfileString(iniSection,"send_to_masterparent",_sendToMaster ? "1" : "0",g_SNM_IniFn.Get());
			if (snprintfStrict(buf, sizeof(buf), "%d", _soloDefeat) > 0)
				WritePrivateProfileString(iniSection,"solo_defeat",buf,g_SNM_IniFn.Get());
			for (int i=0; i<SNM_MAX_HW_OUTS; i++) 
				if (snprintfStrict(slot, sizeof(slot), "hwout%d", i+1) > 0 && snprintfStrict(buf, sizeof(buf), "%d", _hwOuts[i]) > 0)
					WritePrivateProfileString(iniSection,slot,_hwOuts[i]?buf:NULL,g_SNM_IniFn.Get());
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// Cue buss dialog box
///////////////////////////////////////////////////////////////////////////////

int GetComboSendIdxType(int _reaType)
{
	switch(_reaType) {
		case 0: return 1;
		case 3: return 2; 
		case 1: return 3; 
		default: return 1;
	}
	return 1; // in case _reaType comes from mars
}

const char* GetSendTypeStr(int _type) 
{
	switch(_type) {
		case 1: return __localizeFunc("Post-Fader (Post-Pan)","common",0);
		case 2: return __localizeFunc("Pre-Fader (Post-FX)","common",0);
		case 3: return __localizeFunc("Pre-FX","common",0);
		default: return "";
	}
}

void FillHWoutDropDown(HWND _hwnd, int _idc)
{
	char buf[128] = "";
	lstrcpyn(buf, __LOCALIZE("None","sws_DLG_149"), sizeof(buf));
	SendDlgItemMessage(_hwnd,_idc,CB_ADDSTRING,0,(LPARAM)buf);
	SendDlgItemMessage(_hwnd,_idc,CB_SETITEMDATA,0,0);
	
	// get mono outputs
	WDL_PtrList<WDL_FastString> monos;
	int monoIdx=0;
	while (GetOutputChannelName(monoIdx)) {
		monos.Add(new WDL_FastString(GetOutputChannelName(monoIdx)));
		monoIdx++;
	}

	// add stereo outputs
	WDL_PtrList<WDL_FastString> stereos;
	if (monoIdx)
	{
		for(int i=0; i < (monoIdx-1); i++)
		{
			WDL_FastString* hw = new WDL_FastString();
			hw->SetFormatted(256, "%s / %s", monos.Get(i)->Get(), monos.Get(i+1)->Get());
			stereos.Add(hw);
		}
	}

	// fill dropdown
	for(int i=0; i < stereos.GetSize(); i++) {
		SendDlgItemMessage(_hwnd,_idc,CB_ADDSTRING,0,(LPARAM)stereos.Get(i)->Get());
		SendDlgItemMessage(_hwnd,_idc,CB_SETITEMDATA,i,i+1); // +1 for <none>
	}
	for(int i=0; i < monos.GetSize(); i++) {
		SendDlgItemMessage(_hwnd,_idc,CB_ADDSTRING,0,(LPARAM)monos.Get(i)->Get());
		SendDlgItemMessage(_hwnd,_idc,CB_SETITEMDATA,i,i+1); // +1 for <none>
	}

//	SendDlgItemMessage(_hwnd,_idc,CB_SETCURSEL,x0,0);
	monos.Empty(true);
	stereos.Empty(true);
}

void FillCueBussDlg(HWND _hwnd = NULL)
{
	HWND hwnd = _hwnd?_hwnd:g_cueBussHwnd;
	if (!hwnd)
		return;

	g_cueBussDisableSave=true;
	char busName[64]="", trTemplatePath[SNM_MAX_PATH]="";
	int reaType, userType, soloDefeat, hwOuts[8];
	bool trTemplate, showRouting, sendToMaster;
	ReadCueBusIniFile(g_cueBussConfId, busName, sizeof(busName), &reaType, &trTemplate, trTemplatePath, sizeof(trTemplatePath), &showRouting, &soloDefeat, &sendToMaster, hwOuts);
	userType = GetComboSendIdxType(reaType);
	SetWindowLongPtr(GetDlgItem(hwnd, IDC_SNM_CUEBUS_NAME), GWLP_USERDATA, 0xdeadf00b);
	SetDlgItemText(hwnd, IDC_SNM_CUEBUS_NAME, busName);

	for(int i=0; i<3; i++) {
		if (_hwnd) SendDlgItemMessage(hwnd,IDC_SNM_CUEBUS_TYPE,CB_ADDSTRING,0,(LPARAM)GetSendTypeStr(i+1)); // do it once (WM_INITDIALOG)
		if (userType==(i+1)) SendDlgItemMessage(hwnd,IDC_SNM_CUEBUS_TYPE,CB_SETCURSEL,i,0);
	}

	SetWindowLongPtr(GetDlgItem(hwnd, IDC_SNM_CUEBUS_TEMPLATE), GWLP_USERDATA, 0xdeadf00b);
	SetDlgItemText(hwnd, IDC_SNM_CUEBUS_TEMPLATE, trTemplatePath);
	CheckDlgButton(hwnd, IDC_CHECK1, sendToMaster);
	CheckDlgButton(hwnd, IDC_CHECK2, showRouting);
	CheckDlgButton(hwnd, IDC_CHECK3, trTemplate);
	CheckDlgButton(hwnd, IDC_CHECK4, (soloDefeat == 1));

	for(int i=0; i < SNM_MAX_HW_OUTS; i++) {
		if (_hwnd) FillHWoutDropDown(hwnd,IDC_SNM_CUEBUS_HWOUT1+i); // do it once (WM_INITDIALOG)
		SendDlgItemMessage(hwnd,IDC_SNM_CUEBUS_HWOUT1+i,CB_SETCURSEL,hwOuts[i],0);
	}

//	SetFocus(GetDlgItem(hwnd, IDC_SNM_CUEBUS_NAME));
	PostMessage(hwnd, WM_COMMAND, IDC_CHECK3, 0); // enable//disable state

	g_cueBussDisableSave = false;
}

void SaveCueBussSettings()
{
	if (!g_cueBussHwnd || g_cueBussDisableSave)
		return;

	char busName[64]="";
	GetDlgItemText(g_cueBussHwnd,IDC_SNM_CUEBUS_NAME,busName,sizeof(busName));

	int userType=2, reaType;
	int combo = (int)SendDlgItemMessage(g_cueBussHwnd,IDC_SNM_CUEBUS_TYPE,CB_GETCURSEL,0,0);
	if(combo != CB_ERR)
		userType = combo+1;
	switch(userType) {
		case 1: reaType=0; break;
		case 2: reaType=3; break;
		case 3: reaType=1; break;
		default: reaType=3; break;
	}

	int sendToMaster = IsDlgButtonChecked(g_cueBussHwnd, IDC_CHECK1);
	int showRouting = IsDlgButtonChecked(g_cueBussHwnd, IDC_CHECK2);
	int trTemplate = IsDlgButtonChecked(g_cueBussHwnd, IDC_CHECK3);
	int soloDefeat = IsDlgButtonChecked(g_cueBussHwnd, IDC_CHECK4);

	char trTemplatePath[SNM_MAX_PATH]="";
	GetDlgItemText(g_cueBussHwnd,IDC_SNM_CUEBUS_TEMPLATE,trTemplatePath,sizeof(trTemplatePath));

	int hwOuts[SNM_MAX_HW_OUTS];
	for (int i=0; i<SNM_MAX_HW_OUTS; i++) {
		hwOuts[i] = (int)SendDlgItemMessage(g_cueBussHwnd,IDC_SNM_CUEBUS_HWOUT1+i,CB_GETCURSEL,0,0);
		if(hwOuts[i] == CB_ERR)	hwOuts[i] = '\0';
	}
	SaveCueBusIniFile(g_cueBussConfId, busName, reaType, (trTemplate == 1), trTemplatePath, (showRouting == 1), soloDefeat, (sendToMaster == 1), hwOuts);
}

WDL_DLGRET CueBussDlgProc(HWND _hwnd, UINT _uMsg, WPARAM _wParam, LPARAM _lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(_hwnd, _uMsg, _wParam, _lParam))
		return r;

	const char cWndPosKey[] = "CueBus Window Pos";
	switch(_uMsg)
	{
		case WM_INITDIALOG:
		{
			WDL_UTF8_HookComboBox(GetDlgItem(_hwnd, IDC_SNM_CUEBUS_TYPE));
			WDL_UTF8_HookComboBox(GetDlgItem(_hwnd, IDC_COMBO));
			for(int i=0; i < SNM_MAX_HW_OUTS; i++)
				WDL_UTF8_HookComboBox(GetDlgItem(_hwnd, IDC_SNM_CUEBUS_HWOUT1+i));

			RestoreWindowPos(_hwnd, cWndPosKey, false);
			char buf[16] = "";
			for(int i=0; i < SNM_MAX_CUE_BUSS_CONFS; i++)
				if (snprintfStrict(buf,sizeof(buf),"%d",i+1) > 0)
					SendDlgItemMessage(_hwnd,IDC_COMBO,CB_ADDSTRING,0,(LPARAM)buf);
			SendDlgItemMessage(_hwnd,IDC_COMBO,CB_SETCURSEL,0,0);
			FillCueBussDlg(_hwnd);
			return 0;
		}
		break;

		case WM_CLOSE :
			g_cueBussHwnd = NULL; // for proper toggle state report, see openCueBussWnd()
			break;

		case WM_COMMAND :
			switch(LOWORD(_wParam))
			{
				case IDC_COMBO:
					if(HIWORD(_wParam) == CBN_SELCHANGE) // config id update?
					{ 
						int id = (int)SendDlgItemMessage(_hwnd, IDC_COMBO, CB_GETCURSEL, 0, 0);
						if (id != CB_ERR) {
							g_cueBussConfId = id;
							FillCueBussDlg();
						}
					}
					break;
				case IDOK:
					CueBuss(__LOCALIZE("Create cue buss from track selection","sws_undo"), g_cueBussConfId);
					return 0;
				case IDCANCEL:
					g_cueBussHwnd = NULL; // for proper toggle state report, see openCueBussWnd()
					ShowWindow(_hwnd, SW_HIDE);
					return 0;
				case IDC_FILES: {
					char curPath[SNM_MAX_PATH]="";
					GetDlgItemText(_hwnd, IDC_SNM_CUEBUS_TEMPLATE, curPath, sizeof(curPath));
					if (!*curPath || !FileOrDirExists(curPath))
						if (snprintfStrict(curPath, sizeof(curPath), "%s%cTrackTemplates", GetResourcePath(), PATH_SLASH_CHAR) <= 0)
							*curPath = '\0';
					if (char* fn = BrowseForFiles(__LOCALIZE("S&M - Load track template","sws_DLG_149"), curPath, NULL, false, "REAPER Track Template (*.RTrackTemplate)\0*.RTrackTemplate\0")) {
						SetDlgItemText(_hwnd,IDC_SNM_CUEBUS_TEMPLATE,fn);
						free(fn);
						SaveCueBussSettings();
					}
					break;
				}
				case IDC_CHECK3: {
					bool templateEnable = (IsDlgButtonChecked(_hwnd, IDC_CHECK3) == 1);
					EnableWindow(GetDlgItem(_hwnd, IDC_SNM_CUEBUS_TEMPLATE), templateEnable);
					EnableWindow(GetDlgItem(_hwnd, IDC_FILES), templateEnable);
					EnableWindow(GetDlgItem(_hwnd, IDC_SNM_CUEBUS_NAME), !templateEnable);
					for(int k=0; k < SNM_MAX_HW_OUTS ; k++)
						EnableWindow(GetDlgItem(_hwnd, IDC_SNM_CUEBUS_HWOUT1+k), !templateEnable);
					EnableWindow(GetDlgItem(_hwnd, IDC_CHECK1), !templateEnable);
					EnableWindow(GetDlgItem(_hwnd, IDC_CHECK4), !templateEnable);
//					SetFocus(GetDlgItem(_hwnd, templateEnable ? IDC_SNM_CUEBUS_TEMPLATE : IDC_SNM_CUEBUS_NAME));
					SaveCueBussSettings();
					break;
				}
				case IDC_SNM_CUEBUS_SOLOGRP:
				case IDC_CHECK1:
				case IDC_CHECK2:
				case IDC_CHECK4:
					SaveCueBussSettings();
					break;
				case IDC_SNM_CUEBUS_TYPE:
				case IDC_SNM_CUEBUS_HWOUT1:
				case IDC_SNM_CUEBUS_HWOUT2:
				case IDC_SNM_CUEBUS_HWOUT3:
				case IDC_SNM_CUEBUS_HWOUT4:
				case IDC_SNM_CUEBUS_HWOUT5:
				case IDC_SNM_CUEBUS_HWOUT6:
				case IDC_SNM_CUEBUS_HWOUT7:
				case IDC_SNM_CUEBUS_HWOUT8:
					if(HIWORD(_wParam) == CBN_SELCHANGE)
						SaveCueBussSettings();
					break;
				case IDC_SNM_CUEBUS_TEMPLATE:
				case IDC_SNM_CUEBUS_NAME:
					if (HIWORD(_wParam)==EN_CHANGE)
						SaveCueBussSettings();
					break;
			}
			break;

		case WM_DESTROY:
			SaveWindowPos(_hwnd, cWndPosKey);
			break;
	}
	return 0;
}

void OpenCueBussDlg(COMMAND_T* _ct)
{
	static HWND sHwnd = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_SNM_CUEBUSS), GetMainHwnd(), CueBussDlgProc);
	if (g_cueBussHwnd) {
		g_cueBussHwnd = NULL;
		ShowWindow(sHwnd, SW_HIDE);
	}
	else {
		g_cueBussHwnd = sHwnd;
		ShowWindow(sHwnd, SW_SHOW);
		SetFocus(sHwnd);
	}
}

int IsCueBussDlgDisplayed(COMMAND_T* _ct) {
	return (g_cueBussHwnd && SWS_IsWindow(g_cueBussHwnd) && IsWindowVisible(g_cueBussHwnd) ? true : false);
}

// pass-through to main window
//static int translateAccel(MSG* _msg, accelerator_register_t* _ctx) { return SNM_IsActiveWindow(g_cueBussHwnd) ? -666 : 0; }
//static accelerator_register_t s_ar = { translateAccel, TRUE, NULL };

int CueBussInit() {
//	return plugin_register("accelerator", &s_ar);
	return 1;
}
