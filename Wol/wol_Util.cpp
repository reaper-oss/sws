/******************************************************************************
/ wol_Util.cpp
/
/ Copyright (c) 2014 wol
/ http://forum.cockos.com/member.php?u=70153
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
#include "wol_Util.h"
#include "../Breeder/BR_Util.h"
#include "../Breeder/BR_EnvelopeUtil.h"
#include "../SnM/SnM_Dlg.h"
#include "../SnM/SnM_Util.h"
#include "../reaper/localize.h"

#ifdef _WIN32
#define WOL_INI_FILE "%s\\wol.ini"
#else
#define WOL_INI_FILE "%s/wol.ini"
#endif

static WDL_FastString g_wolIni;



///////////////////////////////////////////////////////////////////////////////////////////////////
/// Track
///////////////////////////////////////////////////////////////////////////////////////////////////
static vector<MediaTrack*>* g_savedSelectedTracks = NULL;

bool SaveSelectedTracks()
{
	if (g_savedSelectedTracks)
		return false;
	g_savedSelectedTracks = new (nothrow)vector < MediaTrack* > ;
	if (g_savedSelectedTracks)
	{
		for (int i = 0; i < CountSelectedTracks(NULL); ++i)
			if (MediaTrack* tr = GetSelectedTrack(NULL, i))
				g_savedSelectedTracks->push_back(tr);
		return true;
	}
	return false;
}

bool RestoreSelectedTracks()
{
	if (!g_savedSelectedTracks)
		return false;
	for (size_t i = 0; i < g_savedSelectedTracks->size(); ++i)
		SetTrackSelected(g_savedSelectedTracks->at(i), true);
	g_savedSelectedTracks->clear();
	DELETE_NULL(g_savedSelectedTracks);
	return true;
}

void SetTrackHeight(MediaTrack* track, int height)
{
	GetSetMediaTrackInfo(track, "I_HEIGHTOVERRIDE", &height);
	PreventUIRefresh(1);
	Main_OnCommand(41327, 0);
	Main_OnCommand(41328, 0);
	PreventUIRefresh(-1);
}



///////////////////////////////////////////////////////////////////////////////////////////////////
/// Tcp
///////////////////////////////////////////////////////////////////////////////////////////////////
int GetTcpEnvMinHeight()
{
	if (IconTheme* it = SNM_GetIconTheme())
		return it->envcp_min_height;
	return 0;
}

int GetTcpTrackMinHeight()
{
	if (IconTheme* it = SNM_GetIconTheme())
		return it->tcp_small_height;
	return 0;
}

int GetCurrentTcpMaxHeight()
{
	RECT r;
	GetClientRect(GetArrangeWnd(), &r);
	return r.bottom - r.top;
}

// Stolen from BR_Util.cpp
void ScrollToTrackEnvelopeIfNotInArrange(TrackEnvelope* envelope)
{
	int offsetY;
	int height = GetTrackEnvHeight(envelope, &offsetY, false, NULL);

	HWND hwnd = GetArrangeWnd();
	SCROLLINFO si = { sizeof(SCROLLINFO), };
	si.fMask = SIF_ALL;
	CoolSB_GetScrollInfo(hwnd, SB_VERT, &si);

	int envEnd = offsetY + height;
	int pageEnd = si.nPos + (int)si.nPage + SCROLLBAR_W;

	if (offsetY < si.nPos || envEnd > pageEnd)
	{
		si.nPos = offsetY;
		CoolSB_SetScrollInfo(hwnd, SB_VERT, &si, true);
		SendMessage(hwnd, WM_VSCROLL, si.nPos << 16 | SB_THUMBPOSITION, NULL);
	}
}

// Overloads ScrollToTrackIfNotInArrange(MediaTrack* track) in BR_Util.cpp
void ScrollToTrackIfNotInArrange(TrackEnvelope* envelope)
{
	if (MediaTrack* tr = GetEnvParent(envelope))
		ScrollToTrackIfNotInArrange(tr);
	else if (MediaItem_Take* tk = GetTakeEnvParent(envelope, NULL))
		if (MediaTrack* tr = GetMediaItemTake_Track(tk))
			ScrollToTrackIfNotInArrange(tr);
}

void ScrollToTakeIfNotInArrange(MediaItem_Take* take)
{
	int offsetY;
	int height = GetTakeHeight(take, &offsetY);

	HWND hwnd = GetArrangeWnd();
	SCROLLINFO si = { sizeof(SCROLLINFO), };
	si.fMask = SIF_ALL;
	CoolSB_GetScrollInfo(hwnd, SB_VERT, &si);

	int envEnd = offsetY + height;
	int pageEnd = si.nPos + (int)si.nPage + SCROLLBAR_W;

	if (offsetY < si.nPos || envEnd > pageEnd)
	{
		si.nPos = offsetY;
		CoolSB_SetScrollInfo(hwnd, SB_VERT, &si, true);
		SendMessage(hwnd, WM_VSCROLL, si.nPos << 16 | SB_THUMBPOSITION, NULL);
	}
}



///////////////////////////////////////////////////////////////////////////////////////////////////
/// Envelope
///////////////////////////////////////////////////////////////////////////////////////////////////
int CountVisibleTrackEnvelopesInTrackLane(MediaTrack* track)
{
	int count = 0;
	for (int i = 0; i < CountTrackEnvelopes(track); i++)
	{
		bool visible, lane;
		visible = EnvVis(GetTrackEnvelope(track, i), &lane);
		if (visible && !lane)
			count++;
	}
	return count;
}

int GetEnvelopeOverlapState(TrackEnvelope* envelope, int* laneCount, int* envCount)
{
	bool lane, visible;
	visible = EnvVis(envelope, &lane);
	if (!visible || lane)
		return -1;

	int visEnvCount = CountVisibleTrackEnvelopesInTrackLane(GetEnvParent(envelope));
	int overlapMinHeight = *(int*)GetConfigVar("env_ol_minh");
	if (overlapMinHeight < 0)
		return (WritePtr(laneCount, visEnvCount), WritePtr(envCount, visEnvCount), 0);

	if (GetTrackEnvHeight(envelope, NULL, false) < overlapMinHeight)
		return (WritePtr(laneCount, 1), WritePtr(envCount, visEnvCount), (visEnvCount > 1) ? 2 : 1);

	return (WritePtr(laneCount, (visEnvCount > 1) ? visEnvCount : 1), WritePtr(envCount, (visEnvCount > 1) ? visEnvCount : 1), (visEnvCount > 1) ? 4 : 3);
}



///////////////////////////////////////////////////////////////////////////////////////////////////
/// Midi
///////////////////////////////////////////////////////////////////////////////////////////////////
// from SnM.cpp
// http://forum.cockos.com/project.php?issueid=4576
int AdjustRelative(int _adjmode, int _reladj)
{
	if (_adjmode == 1) { if (_reladj >= 0x40) _reladj |= ~0x3f; } // sign extend if 0x40 set
	else if (_adjmode == 2) { _reladj -= 0x40; } // offset by 0x40
	else if (_adjmode == 3) { if (_reladj & 0x40) _reladj = -(_reladj & 0x3f); } // 0x40 is sign bit
	else _reladj = 0;
	return _reladj;
}



///////////////////////////////////////////////////////////////////////////////////////////////////
/// Ini
///////////////////////////////////////////////////////////////////////////////////////////////////
const char* GetWolIni()
{
	return g_wolIni.Get();
}

void SaveIniSettings(const char* section, const char* key, bool val, const char* path)
{
	char tmp[256];
	_snprintfSafe(tmp, sizeof(tmp), "%d", val ? 1 : 0);
	WritePrivateProfileString(section, key, tmp, path);
}

void SaveIniSettings(const char* section, const char* key, int val, const char* path)
{
	char tmp[256];
	_snprintfSafe(tmp, sizeof(tmp), "%d", val);
	WritePrivateProfileString(section, key, tmp, path);
}

void SaveIniSettings(const char* section, const char* key, const char* val, const char* path)
{
	WritePrivateProfileString(section, key, val, path);
}

bool GetIniSettings(const char* section, const char* key, bool defVal, const char* path)
{
	return GetPrivateProfileInt(section, key, defVal ? 1 : 0, path) ? true : false;
}

int GetIniSettings(const char* section, const char* key, int defVal, const char* path)
{
	return GetPrivateProfileInt(section, key, defVal, path);
}

int GetIniSettings(const char* section, const char* key, const char* defVal, char* outBuf, int maxOutBufSize, const char* path)
{
	return GetPrivateProfileString(section, key, defVal, outBuf, maxOutBufSize, path);
}

void DeleteIniSection(const char* section, const char* path)
{
	WritePrivateProfileString(section, NULL, NULL, path);
}

void DeleteIniKey(const char* section, const char* key, const char* path)
{
	WritePrivateProfileString(section, key, NULL, path);
}

#ifdef _WIN32
void FlushIni(const char* path)
{
	WritePrivateProfileString(NULL, NULL, NULL, path);
}
#endif



///////////////////////////////////////////////////////////////////////////////////////////////////
/// Messages
///////////////////////////////////////////////////////////////////////////////////////////////////
int ShowMessageBox2(const char* msg, const char* title, UINT uType, UINT uIcon, HWND hwnd)
{
	return MessageBox(hwnd, msg, title, 
						uType
#ifdef _WIN32
						| uIcon
#endif
						);
}

int ShowErrorMessageBox(const char* msg,  UINT uType, HWND hwnd)
{
	const char* title = __LOCALIZE("SWS/wol - Error", "sws_mbox");
	return ShowMessageBox2(msg, title, uType,
#ifdef _WIN32
							MB_ICONERROR,
#else
							0,
#endif
							hwnd);
}

int ShowWarningMessageBox(const char* msg, UINT uType, HWND hwnd)
{
	const char* title = __LOCALIZE("SWS/wol - Warning", "sws_mbox");
	return ShowMessageBox2(msg, title, uType,
#ifdef _WIN32
							MB_ICONWARNING,
#else
							0,
#endif
							hwnd);
}



///////////////////////////////////////////////////////////////////////////////////////////////////
/// User input
///////////////////////////////////////////////////////////////////////////////////////////////////
#define MSG_KN1    0xF000
#define MSG_KN1TXT 0xF001
#define MSG_KN2    0xF002
#define MSG_KN2TXT 0xF003
#define MSG_OPTION 0xF004 

UserInputAndSlotsEditorWnd::UserInputAndSlotsEditorWnd(const char* wndtitle, const char* title, const char* id, int cmdId)
	: SWS_DockWnd(IDD_WOL_USRINSLTEDWND, title, id, cmdId)
{
	m_wndtitlebar = wndtitle;
	m_oktxt.clear();
	m_questiontxt.clear();
	m_questiontype = MB_OK;
	m_twoknobs = m_kn1rdy = m_kn2rdy = m_cbrdy = m_askquestion = m_realtimenotify = false;
	m_linkknobs = true;
	m_kn1oldval = m_kn2oldval = 0;
	m_options.clear();

	// Must call SWS_DockWnd::Init() to restore parameters and open the window if necessary
	Init();
}

void UserInputAndSlotsEditorWnd::SetupKnob1(int min, int max, int center, int pos, double factor, const char* title, const char* suffix, const char* zerotext)
{
	m_kn1.SetRangeFactor(min, max, center, factor);
	m_kn1.SetSliderPosition(pos);

	m_kn1Text.SetTitle(title);
	m_kn1Text.SetSuffix(suffix);
	m_kn1Text.SetZeroText(zerotext);

	m_kn1rdy = true;
	if (!m_twoknobs)
		m_kn2rdy = true;
}

void UserInputAndSlotsEditorWnd::SetupKnob2(int min, int max, int center, int pos, double factor, const char* title, const char* suffix, const char* zerotext)
{
	m_kn2.SetRangeFactor(min, max, center, factor);
	m_kn2.SetSliderPosition(pos);

	m_kn2Text.SetTitle(title);
	m_kn2Text.SetSuffix(suffix);
	m_kn2Text.SetZeroText(zerotext);

	m_kn2rdy = true;
}

void UserInputAndSlotsEditorWnd::SetupOnCommandCallback(void(*OnCommandCallback)(int cmd, int* kn1, int* kn2))
{
	m_OnCommandCallback = OnCommandCallback;

	m_cbrdy = true;
}

void UserInputAndSlotsEditorWnd::SetupOKText(const char* text)
{
	m_oktxt = text;
}

void UserInputAndSlotsEditorWnd::SetupQuestion(const char* questiontxt, const char* questiontitle, UINT type)
{
	m_askquestion = true;
	m_questiontxt = questiontxt;
	m_questiontitle = questiontitle;
	m_questiontype = type;
}

//void UserInputAndSlotsEditorWnd::SetupOptions(vector<UserInputAndSlotsEditorOption>* options)
//{
//	if (options)
//	{
//		for (size_t i = 0; i < options->size(); ++i)
//		{
//			if (options->at(i).cmd >= CMD_OPTION_MIN && options->at(i).cmd <= CMD_OPTION_MAX)
//				m_options.push_back(options->at(i));
//		}
//	}
//}

bool UserInputAndSlotsEditorWnd::SetupAddOption(int cmd, bool enabled, bool checked, string title)
{
	if (cmd >= CMD_OPTION_MIN && cmd <= CMD_OPTION_MAX)
	{
		UserInputAndSlotsEditorOption opt(cmd, enabled, checked, title);
		m_options.push_back(opt);
		return true;
	}
	return false;
}

bool UserInputAndSlotsEditorWnd::SetOptionState(int cmd, const bool* enabled, const bool* checked, const string* title)
{
	for (vector<UserInputAndSlotsEditorOption>::iterator opt = m_options.begin(); opt != m_options.end(); ++opt)
	{
		if (opt->cmd == cmd)
		{
			if (enabled)
				opt->enabled = *enabled;
			if (checked)
				opt->checked = *checked;
			if (title)
				opt->title = *title;
			return true;
		}
	}
	return false;
}

bool UserInputAndSlotsEditorWnd::SetOptionStateEnable(int cmd, bool enabled)
{
	for (vector<UserInputAndSlotsEditorOption>::iterator opt = m_options.begin(); opt != m_options.end(); ++opt)
	{
		if (opt->cmd == cmd)
		{
			opt->enabled = enabled;
			return true;
		}
	}
	return false;
}

bool UserInputAndSlotsEditorWnd::SetOptionStateChecked(int cmd, bool checked)
{
	for (vector<UserInputAndSlotsEditorOption>::iterator opt = m_options.begin(); opt != m_options.end(); ++opt)
	{
		if (opt->cmd == cmd)
		{
			opt->checked = checked;
			return true;
		}
	}
	return false;
}

bool UserInputAndSlotsEditorWnd::SetOptionStateTitle(int cmd, string title)
{
	for (vector<UserInputAndSlotsEditorOption>::iterator opt = m_options.begin(); opt != m_options.end(); ++opt)
	{
		if (opt->cmd == cmd)
		{
			opt->title = title;
			return true;
		}
	}
	return false;
}

bool UserInputAndSlotsEditorWnd::GetOptionState(int cmd, bool* enabled, bool* checked, string* title) const
{
	for (vector<UserInputAndSlotsEditorOption>::const_iterator opt = m_options.begin(); opt != m_options.end(); ++opt)
	{
		if (opt->cmd == cmd)
		{
			if (enabled)
				*enabled = opt->enabled;
			if (checked)
				*checked = opt->checked;
			if (title)
				*title = opt->title;
			return true;
		}
	}
	return false;
}

void UserInputAndSlotsEditorWnd::OnInitDlg()
{
	if (!m_kn1rdy || !m_kn2rdy || !m_cbrdy)
		DestroyWindow(m_hwnd);

	m_parentVwnd.SetRealParent(m_hwnd);
	m_vwnd_painter.SetGSC(WDL_STYLE_GetSysColor);

	m_kn1.SetID(MSG_KN1);
	m_kn1Text.AddChild(&m_kn1);

	m_kn1Text.SetID(MSG_KN1TXT);
	m_kn1Text.SetValue(m_kn1.GetSliderPosition());
	m_parentVwnd.AddChild(&m_kn1Text);

	m_kn2.SetID(MSG_KN2);
	m_kn2Text.AddChild(&m_kn2);

	m_kn2Text.SetID(MSG_KN2TXT);
	m_kn2Text.SetValue(m_kn2.GetSliderPosition());
	m_parentVwnd.AddChild(&m_kn2Text);

	m_btnL = GetDlgItem(m_hwnd, IDC_WOL_SLOT6S);
	m_btnR = GetDlgItem(m_hwnd, IDC_WOL_SLOT7L);

	if (m_wndtitlebar.size())
		SetWindowText(m_hwnd, m_wndtitlebar.c_str());

	if (m_oktxt.size())
		SetWindowText(GetDlgItem(m_hwnd, IDC_WOL_OK), m_oktxt.c_str());

	if (!m_options.size())
		EnableWindow(GetDlgItem(m_hwnd, IDC_WOL_OPTIONS), FALSE);
}

void UserInputAndSlotsEditorWnd::OnDestroy()
{
	m_kn1Text.RemoveAllChildren(false);
	m_kn2Text.RemoveAllChildren(false);
}

void UserInputAndSlotsEditorWnd::DrawControls(LICE_IBitmap* bm, const RECT* r, int* tooltipHeight)
{
	RECT r2 = { 0, 0, 0, 0 };
	RECT tmp = { 0, 0, 0, 0 };
	ColorTheme* ct = SNM_GetColorTheme();
	LICE_pixel col = ct ? LICE_RGBA_FROMNATIVE(ct->main_text, 255) : LICE_RGBA(255, 255, 255, 255);

	m_kn1.SetFGColors(col, col);
	SNM_SkinKnob(&m_kn1);
	m_kn1Text.DrawText(NULL, &r2, DT_NOPREFIX | DT_CALCRECT);
	GetWindowRect(m_btnL, &tmp);
	ScreenToClient(m_hwnd, (LPPOINT)&tmp);
	ScreenToClient(m_hwnd, (LPPOINT)&tmp + 1);
	if (m_twoknobs)
	{
		r2.left = tmp.right - r2.right - 19;
		r2.top = tmp.bottom + 10;
		r2.right = tmp.right;
		r2.bottom += r2.top;
	}
	else
	{
		r2.left = tmp.right + 2 - (r2.right / 2) - 9;
		r2.top = tmp.bottom + 10;
		r2.right = tmp.right + (r2.right / 2) + 10;
		r2.bottom += r2.top;
	}
	m_kn1Text.SetPosition(&r2);
	m_kn1Text.SetVisible(true);

	r2.left = r2.top = r2.right = r2.bottom = 0;
	m_kn2.SetFGColors(col, col);
	SNM_SkinKnob(&m_kn2);
	m_kn2Text.DrawText(NULL, &r2, DT_NOPREFIX | DT_CALCRECT);
	GetWindowRect(m_btnR, &tmp);
	ScreenToClient(m_hwnd, (LPPOINT)&tmp);
	ScreenToClient(m_hwnd, ((LPPOINT)&tmp) + 1);
	r2.left = tmp.left;
	r2.top = tmp.bottom + 10;
	r2.right += r2.left + 19;
	r2.bottom += r2.top;
	m_kn2Text.SetPosition(&r2);
	m_kn2Text.SetVisible(m_twoknobs);

}

void UserInputAndSlotsEditorWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case IDC_WOL_SLOT1L:
	case IDC_WOL_SLOT2L:
	case IDC_WOL_SLOT3L:
	case IDC_WOL_SLOT4L:
	case IDC_WOL_SLOT5L:
	case IDC_WOL_SLOT6L:
	case IDC_WOL_SLOT7L:
	case IDC_WOL_SLOT8L:
	{
		int kn1 = 0, kn2 = 0;
		m_OnCommandCallback(CMD_LOAD + LOWORD(wParam) - IDC_WOL_SLOT1L, &kn1, &kn2);
		m_kn1Text.SetValue(kn1);
		m_kn2Text.SetValue(kn2);
		Update();
		break;
	}
	case IDC_WOL_SLOT1S:
	case IDC_WOL_SLOT2S:
	case IDC_WOL_SLOT3S:
	case IDC_WOL_SLOT4S:
	case IDC_WOL_SLOT5S:
	case IDC_WOL_SLOT6S:
	case IDC_WOL_SLOT7S:
	case IDC_WOL_SLOT8S:
	{
		int kn1 = m_kn1.GetSliderPosition(), kn2 = m_kn2.GetSliderPosition();
		m_OnCommandCallback(CMD_SAVE + LOWORD(wParam) - IDC_WOL_SLOT1S, &kn1, &kn2);
		break;
	}
	case IDC_WOL_OK:
	{
		int kn1 = m_kn1.GetSliderPosition(), kn2 = m_kn2.GetSliderPosition();
		int answer = CMD_USERANSWER;
		if (m_askquestion)
			answer += MessageBox(m_hwnd, m_questiontxt.c_str(), m_questiontitle.c_str(), m_questiontype);
		else
			answer += IDOK;
		m_OnCommandCallback(answer, &kn1, &kn2);
		break;
	}
	case IDC_WOL_CLOSE:
	{
		int kn1 = m_kn1.GetSliderPosition(), kn2 = m_kn2.GetSliderPosition();
		m_OnCommandCallback(CMD_CLOSE, &kn1, &kn2);
		SendMessage(m_hwnd, WM_COMMAND, (WPARAM)IDCANCEL, (LPARAM)0);
		break;
	}
	case IDC_WOL_OPTIONS:
	{
		RECT r;
		GetWindowRect(GetDlgItem(m_hwnd, IDC_WOL_OPTIONS), &r);
		SendMessage(m_hwnd, WM_CONTEXTMENU, 0, MAKELPARAM((UINT)(r.left), (UINT)(r.bottom + 1)));
		break;
	}
	default:
	{
		int msg = LOWORD(wParam) - MSG_OPTION;
		if (msg >= CMD_OPTION_MIN && msg <= CMD_OPTION_MAX)
			m_OnCommandCallback(msg, NULL, NULL);
		break;
	}
	}
}

HMENU UserInputAndSlotsEditorWnd::OnContextMenu(int x, int y, bool* wantDefaultItems)
{
	HMENU hMenu = NULL;

	RECT r;
	GetWindowRect(GetDlgItem(m_hwnd, IDC_WOL_OPTIONS), &r);
	POINT p;
	GetCursorPos(&p);
	if (PtInRect(&r, p))
	{
		*wantDefaultItems = false;

		if (m_options.size())
		{
			hMenu = CreatePopupMenu();

			for (vector<UserInputAndSlotsEditorOption>::iterator opt = m_options.begin(); opt != m_options.end(); ++opt)
				AddToMenu(hMenu, opt->title.c_str(), opt->cmd + MSG_OPTION, -1, false, (opt->enabled ? MFS_ENABLED : MFS_GRAYED) | (opt->checked ? MFS_CHECKED : MFS_UNCHECKED));
		}
	}
	return hMenu;
}

INT_PTR UserInputAndSlotsEditorWnd::OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_VSCROLL)
	{
		switch (lParam)
		{
		case MSG_KN1:
		{
			m_kn1Text.SetValue(m_kn1.GetSliderPosition());
			if (m_linkknobs && (m_kn1.GetSliderPosition() > m_kn2.GetSliderPosition()))
				m_kn2Text.SetValue(m_kn1.GetSliderPosition());

			int kn1 = m_kn1.GetSliderPosition(), kn2 = m_kn2.GetSliderPosition();
			if (m_realtimenotify && kn1 != m_kn1oldval)
			{
				m_kn1oldval = kn1;
				m_OnCommandCallback(CMD_RT_KNOB1, &kn1, &kn2);
			}
			break;
		}
		case MSG_KN2:
		{
			m_kn2Text.SetValue(m_kn2.GetSliderPosition());
			if (m_linkknobs && (m_kn2.GetSliderPosition() < m_kn1.GetSliderPosition()))
				m_kn1Text.SetValue(m_kn2.GetSliderPosition());

			int kn1 = m_kn1.GetSliderPosition(), kn2 = m_kn2.GetSliderPosition();
			if (m_realtimenotify && kn2 != m_kn2oldval)
			{
				m_kn2oldval = kn2;
				m_OnCommandCallback(CMD_RT_KNOB2, &kn1, &kn2);
			}
			break;
		}
		}
	}
	else if (uMsg == WM_MOUSEWHEEL)
	{
		POINT mouse = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		ScreenToClient(m_hwnd, &mouse);
		m_parentVwnd.OnMouseWheel(mouse.x,
			mouse.y,
#ifdef _WIN32
			GET_WHEEL_DELTA_WPARAM(wParam)
#else
			(short)HIWORD(wParam)
#endif
			);
	}
	return 0;
}

void UserInputAndSlotsEditorWnd::Update()
{
	m_parentVwnd.RequestRedraw(NULL);
}



///////////////////////////////////////////////////////////////////////////////////////////////////
/// Math
///////////////////////////////////////////////////////////////////////////////////////////////////
int GetMean(vector<int> v)
{
	if (!v.size())
		return 0;

	int sum = 0;
	for (vector<int>::iterator i = v.begin(); i != v.end(); ++i)
		sum += *i;
	return sum / v.size();
}

int GetMedian(vector<int> v)
{
	size_t n = v.size() / 2;
	if (v.size() % 2 == 0)
	{
		sort(v.begin(), v.end());
		return (int)(((v[n - 1] + v[n]) / 2) + 0.5f);
	}
	else
	{
		nth_element(v.begin(), v.begin() + n, v.end());
		return v[n];
	}
}



///////////////////////////////////////////////////////////////////////////////////////////////////
/// 
///////////////////////////////////////////////////////////////////////////////////////////////////
void wol_UtilInit()
{
	g_wolIni.SetFormatted(2048, WOL_INI_FILE, GetResourcePath());
}