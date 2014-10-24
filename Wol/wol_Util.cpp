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

//---------//

static WDL_FastString g_wolIni;

//---------//

void wol_UtilInit()
{
	g_wolIni.SetFormatted(2048, WOL_INI_FILE, GetResourcePath());
}

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
int ShowMessageBox2(const char* msg, const char* title, UINT uType, UINT uIcon, bool localizeMsg, bool localizeTitle, HWND hwnd)
{
#ifdef _WIN32
	return MessageBox(hwnd,
		localizeMsg ? __localizeFunc(msg, "sws_mbox", 0) : msg,
		localizeTitle ? __localizeFunc(title, "sws_mbox", 0) : title, uType | uIcon);
#else
	return MessageBox(hwnd,
		localizeMsg ? __localizeFunc(msg, "sws_mbox", 0) : msg,
		localizeTitle ? __localizeFunc(title, "sws_mbox", 0) : title, uType);
#endif
}

int ShowErrorMessageBox(const char* msg, const char* title, bool localizeMsg, bool localizeTitle, UINT uType, HWND hwnd)
{
#ifdef _WIN32
	return ShowMessageBox2(msg, title, uType, MB_ICONERROR, localizeMsg, localizeTitle, hwnd);
#else
	return ShowMessageBox2(msg, title, uType, 0, localizeMsg, localizeTitle, hwnd);
#endif
}

int ShowWarningMessageBox(const char* msg, const char* title, bool localizeMsg, bool localizeTitle, UINT uType, HWND hwnd)
{
#ifdef _WIN32
	return ShowMessageBox2(msg, title, uType, MB_ICONWARNING, localizeMsg, localizeTitle, hwnd);
#else
	return ShowMessageBox2(msg, title, uType, 0, localizeMsg, localizeTitle, hwnd);
#endif
}



///////////////////////////////////////////////////////////////////////////////////////////////////
/// User input
///////////////////////////////////////////////////////////////////////////////////////////////////
#define MSG_MIN    0xF000
#define MSG_MINTXT 0xF001
#define MSG_MAX    0xF002
#define MSG_MAXTXT 0xF003

UserInputAndSlotsEditorWnd::UserInputAndSlotsEditorWnd(const char* wndtitle, const char* title, const char* id, int cmdId)
	: SWS_DockWnd(IDD_WOL_USRINSLTEDWND, title, id, cmdId)
{
	m_wndtitlebar = wndtitle;
	m_oktxt.clear();
	m_questiontxt.clear();
	m_questiontype = MB_OK;
	m_twoknobs = m_kn1rdy = m_kn2rdy = m_cbrdy = m_askquestion = false;

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

void UserInputAndSlotsEditorWnd::SetupOnCommandCallback(void(*OnCommandCallback)(int cmd, int* min, int* max))
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

void UserInputAndSlotsEditorWnd::OnInitDlg()
{
	if (!m_kn1rdy || !m_kn2rdy || !m_cbrdy)
		DestroyWindow(m_hwnd);

	m_parentVwnd.SetRealParent(m_hwnd);
	m_vwnd_painter.SetGSC(WDL_STYLE_GetSysColor);

	m_kn1.SetID(MSG_MIN);
	m_kn1Text.AddChild(&m_kn1);

	m_kn1Text.SetID(MSG_MINTXT);
	m_kn1Text.SetValue(m_kn1.GetSliderPosition());
	m_parentVwnd.AddChild(&m_kn1Text);

	if (m_twoknobs)
	{
		m_kn2.SetID(MSG_MAX);
		m_kn2Text.AddChild(&m_kn2);

		m_kn2Text.SetID(MSG_MAXTXT);
		m_kn2Text.SetValue(m_kn2.GetSliderPosition());
		m_parentVwnd.AddChild(&m_kn2Text);
	}

	m_btnL = GetDlgItem(m_hwnd, IDC_WOL_SLOT6S);
	m_btnR = GetDlgItem(m_hwnd, IDC_WOL_SLOT7L);

	if (m_wndtitlebar.size())
		SetWindowText(m_hwnd, m_wndtitlebar.data());

	if (m_oktxt.size())
		SetWindowText(GetDlgItem(m_hwnd, IDC_WOL_OK), m_oktxt.data());
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

	if (m_twoknobs)
	{
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
		m_kn2Text.SetVisible(true);
	}
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
		int min = 0, max = 0;
		m_OnCommandCallback(CMD_LOAD + LOWORD(wParam) - IDC_WOL_SLOT1L, &min, &max);
		m_kn1Text.SetValue(min);
		m_kn2Text.SetValue(max);
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
		int min = m_kn1.GetSliderPosition(), max = m_kn2.GetSliderPosition();
		m_OnCommandCallback(CMD_SAVE + LOWORD(wParam) - IDC_WOL_SLOT1S, &min, &max);
		break;
	}
	case IDC_WOL_OK:
	{
		int min = m_kn1.GetSliderPosition(), max = m_kn2.GetSliderPosition();
		int answer = CMD_USERANSWER;
		if (m_askquestion)
			answer += MessageBox(m_hwnd, m_questiontxt.data(), m_questiontitle.data(), m_questiontype);
		else
			answer += MB_OK;
		m_OnCommandCallback(answer, &min, &max);
		break;
	}
	case IDC_WOL_CLOSE:
	{
		int min = m_kn1.GetSliderPosition(), max = m_kn2.GetSliderPosition();
		m_OnCommandCallback(CMD_CLOSE, &min, &max);
		SendMessage(m_hwnd, WM_COMMAND, (WPARAM)IDCANCEL, (LPARAM)0);
		break;
	}
	}
}

INT_PTR UserInputAndSlotsEditorWnd::OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_VSCROLL)
	{
		switch (lParam)
		{
		case MSG_MIN:
		{
			m_kn1Text.SetValue(m_kn1.GetSliderPosition());
			if (m_kn1.GetSliderPosition() > m_kn2.GetSliderPosition())
			{
				m_kn2Text.SetValue(m_kn2.GetSliderPosition());
			}
			break;
		}
		case MSG_MAX:
		{
			m_kn2Text.SetValue(m_kn2.GetSliderPosition());
			if (m_kn2.GetSliderPosition() < m_kn1.GetSliderPosition())
			{
				m_kn1Text.SetValue(m_kn1.GetSliderPosition());
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