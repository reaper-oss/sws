/******************************************************************************
/ wol_Misc.cpp
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
#include "wol_Misc.h"
#include "wol_Util.h"
#include "../SnM/SnM_Dlg.h"
#include "../SnM/SnM_Util.h"
#include "../reaper/localize.h"

static struct wol_Misc_Ini {
	static const char* Section;
	static const char* RandomizerSlotsMin[8];
	static const char* RandomizerSlotsMax[8];
} wol_Misc_Ini;
const char* wol_Misc_Ini::Section = "Misc";
const char* wol_Misc_Ini::RandomizerSlotsMin[8] = {
	"RandomizerSlot1Min",
	"RandomizerSlot2Min",
	"RandomizerSlot3Min",
	"RandomizerSlot4Min",
	"RandomizerSlot5Min",
	"RandomizerSlot6Min",
	"RandomizerSlot7Min",
	"RandomizerSlot8Min",
};
const char* wol_Misc_Ini::RandomizerSlotsMax[8] = {
	"RandomizerSlot1Max",
	"RandomizerSlot2Max",
	"RandomizerSlot3Max",
	"RandomizerSlot4Max",
	"RandomizerSlot5Max",
	"RandomizerSlot6Max",
	"RandomizerSlot7Max",
	"RandomizerSlot8Max",
};



#define RANDMIDIVELWND_ID "WolRandMidiVelWnd"

static SNM_WindowManager<RandomMidiVelWnd> g_RandMidiVelWndMgr(RANDMIDIVELWND_ID);



///////////////////////////////////////////////////////////////////////////////////////////////////
// Navigation
///////////////////////////////////////////////////////////////////////////////////////////////////
void MoveEditCursorToBeatN(COMMAND_T* ct)
{
	int beat = (int)ct->user - 1;
	int meas = 0, cml = 0;
	TimeMap2_timeToBeats(NULL, GetCursorPosition(), &meas, &cml, 0, 0);
	if (beat >= cml)
		beat = cml - 1;
	SetEditCurPos(TimeMap2_beatsToTime(0, (double)beat, &meas), 1, 0);
}

void MoveEditCursorTo(COMMAND_T* ct)
{
	if ((int)ct->user == 0)
	{
		int meas = 0;
		SetEditCurPos(TimeMap2_beatsToTime(NULL, (double)(int)(TimeMap2_timeToBeats(NULL, GetCursorPosition(), &meas, NULL, NULL, NULL) + 0.5), &meas), true, false);
	}
	else if ((int)ct->user > 0)
	{
		if ((int)ct->user == 3 && GetToggleCommandState(40370) == 1)
			ApplyNudge(NULL, 2, 6, 18, 1.0f, false, 0);
		else
		{
			int meas = 0, cml = 0;
			int beat = (int)TimeMap2_timeToBeats(NULL, GetCursorPosition(), &meas, &cml, NULL, NULL);
			if (beat == cml - 1)
			{
				if ((int)ct->user > 1)
					++meas;
				SetEditCurPos(TimeMap2_beatsToTime(NULL, 0.0f, &meas), true, false);
			}
			else
				SetEditCurPos(TimeMap2_beatsToTime(NULL, (double)(beat + 1), &meas), true, false);
		}
	}
	else
	{
		if ((int)ct->user == -3 && GetToggleCommandState(40370) == 1)
			ApplyNudge(NULL, 2, 6, 18, 1.0f, true, 0);
		else
		{
			int meas = 0, cml = 0;
			int beat = (int)TimeMap2_timeToBeats(NULL, GetCursorPosition(), &meas, &cml, NULL, NULL);
			if (beat == 0)
			{
				if ((int)ct->user < -1)
					--meas;
				SetEditCurPos(TimeMap2_beatsToTime(NULL, (double)(cml - 1), &meas), true, false);
			}
			else
				SetEditCurPos(TimeMap2_beatsToTime(NULL, (double)(beat - 1), &meas), true, false);
		}
	}
}



///////////////////////////////////////////////////////////////////////////////////////////////////
// Track
///////////////////////////////////////////////////////////////////////////////////////////////////
void SelectAllTracksExceptFolderParents(COMMAND_T* ct)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		GetSetMediaTrackInfo(tr, "I_SELECTED", *(int*)GetSetMediaTrackInfo(tr, "I_FOLDERDEPTH", NULL) == 1 ? &g_i0 : &g_i1);
	}
	Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
}



///////////////////////////////////////////////////////////////////////////////////////////////////
// Midi
///////////////////////////////////////////////////////////////////////////////////////////////////
void MoveEditCursorToNote(COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd)
{
	if (MediaItem_Take* take = MIDIEditor_GetTake(MIDIEditor_GetActive()))
	{
		int note_count = 0;
		MIDI_CountEvts(take, &note_count, NULL, NULL);
		int i = 0;
		for (i = 0; i < note_count; ++i)
		{
			double start_pos = 0.0f;
			if (MIDI_GetNote(take, i, NULL, NULL, &start_pos, NULL, NULL, NULL, NULL))
			{
				if (((int)ct->user < 0) ? MIDI_GetProjTimeFromPPQPos(take, start_pos) >= GetCursorPosition() : MIDI_GetProjTimeFromPPQPos(take, start_pos) > GetCursorPosition())
				{
					if ((int)ct->user < 0 && i > 0)
						MIDI_GetNote(take, i - 1, NULL, NULL, &start_pos, NULL, NULL, NULL, NULL);
					SetEditCurPos(MIDI_GetProjTimeFromPPQPos(take, start_pos), true, false);
					break;
				}
			}
		}
		if ((int)ct->user < 0 && i == note_count)
		{
			double start_pos = 0.0f;
			MIDI_GetNote(take, i - 1, NULL, NULL, &start_pos, NULL, NULL, NULL, NULL);
			SetEditCurPos(MIDI_GetProjTimeFromPPQPos(take, start_pos), true, false);
		}
	}
}

//---------//

#define MSG_MIN    0xF000
#define MSG_MINTXT 0xF001
#define MSG_MAX    0xF002
#define MSG_MAXTXT 0xF003

struct RandomizerSlots {
	int min;
	int max;
};

static RandomizerSlots g_RandomizerSlots[8];

bool RandomizeSelectedMidiVelocities(int min, int max);

RandomMidiVelWnd::RandomMidiVelWnd()
	: SWS_DockWnd(IDD_WOL_RANDOMMIDIVEL, __LOCALIZE("Random midi velocities tool", "sws_DLG_181"), "", SWSGetCommandID(RandomizeSelectedMidiVelocitiesTool))
{
	m_id.Set(RANDMIDIVELWND_ID);

	// Must call SWS_DockWnd::Init() to restore parameters and open the window if necessary
	Init();
}

void RandomMidiVelWnd::OnInitDlg()
{
	m_parentVwnd.SetRealParent(m_hwnd);
	m_vwnd_painter.SetGSC(WDL_STYLE_GetSysColor);

	m_min.SetID(MSG_MIN);
	m_min.SetRangeFactor(1, 127, 64, 12.0f);
	m_min.SetSliderPosition(1);
	m_minText.AddChild(&m_min);

	m_minText.SetID(MSG_MINTXT);
	m_minText.SetTitle(__LOCALIZE("Min value:", "sws_DLG_181"));
	m_minText.SetSuffix(__LOCALIZE("", "sws_DLG_181"));
	m_minText.SetZeroText(__LOCALIZE("1", "sws_DLG_181"));
	m_minText.SetValue(m_min.GetSliderPosition());
	m_parentVwnd.AddChild(&m_minText);

	m_max.SetID(MSG_MAX);
	m_max.SetRangeFactor(1, 127, 64, 12.0f);
	m_max.SetSliderPosition(127);
	m_maxText.AddChild(&m_max);

	m_maxText.SetID(MSG_MAXTXT);
	m_maxText.SetTitle(__LOCALIZE("Max value:", "sws_DLG_181"));
	m_maxText.SetSuffix(__LOCALIZE("", "sws_DLG_181"));
	m_maxText.SetZeroText(__LOCALIZE("1", "sws_DLG_181"));
	m_maxText.SetValue(m_max.GetSliderPosition());
	m_parentVwnd.AddChild(&m_maxText);

	m_btnL = GetDlgItem(m_hwnd, IDC_WOL_SLOT6S);
	m_btnR = GetDlgItem(m_hwnd, IDC_WOL_SLOT7L);
}

void RandomMidiVelWnd::OnDestroy()
{
	m_minText.RemoveAllChildren(false);
	m_maxText.RemoveAllChildren(false);
}

void RandomMidiVelWnd::DrawControls(LICE_IBitmap* bm, const RECT* r, int* tooltipHeight)
{
	RECT r2 = { 0, 0, 0, 0 };
	RECT tmp = { 0, 0, 0, 0 };
	ColorTheme* ct = SNM_GetColorTheme();
	LICE_pixel col = ct ? LICE_RGBA_FROMNATIVE(ct->main_text, 255) : LICE_RGBA(255, 255, 255, 255);

	m_min.SetFGColors(col, col);
	SNM_SkinKnob(&m_min);
	m_minText.DrawText(NULL, &r2, DT_NOPREFIX | DT_CALCRECT);
	GetWindowRect(m_btnL, &tmp);
	ScreenToClient(m_hwnd, (LPPOINT)&tmp);
	ScreenToClient(m_hwnd, (LPPOINT)&tmp + 1);
	r2.left += tmp.right - r2.right - 19;
	r2.top += tmp.bottom + 10;
	r2.right = tmp.right;
	r2.bottom += r2.top;
	m_minText.SetPosition(&r2);
	m_minText.SetVisible(true);

	r2.left = r2.top = r2.right = r2.bottom = 0;
	m_max.SetFGColors(col, col);
	SNM_SkinKnob(&m_max);
	m_maxText.DrawText(NULL, &r2, DT_NOPREFIX | DT_CALCRECT);
	GetWindowRect(m_btnR, &tmp);
	ScreenToClient(m_hwnd, (LPPOINT)&tmp);
	ScreenToClient(m_hwnd, ((LPPOINT)&tmp) + 1);
	r2.left = tmp.left;
	r2.top += tmp.bottom + 10;
	r2.right += r2.left + 19;
	r2.bottom += r2.top;
	m_maxText.SetPosition(&r2);
	m_maxText.SetVisible(true);
}

void RandomMidiVelWnd::OnCommand(WPARAM wParam, LPARAM lParam)
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
		m_min.SetSliderPosition(g_RandomizerSlots[LOWORD(wParam) - IDC_WOL_SLOT1L].min);
		m_max.SetSliderPosition(g_RandomizerSlots[LOWORD(wParam) - IDC_WOL_SLOT1L].max);
		m_minText.SetValue(g_RandomizerSlots[LOWORD(wParam) - IDC_WOL_SLOT1L].min);
		m_maxText.SetValue(g_RandomizerSlots[LOWORD(wParam) - IDC_WOL_SLOT1L].max);
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
		SaveIniSettings(wol_Misc_Ini.Section, wol_Misc_Ini.RandomizerSlotsMin[LOWORD(wParam) - IDC_WOL_SLOT1S], m_min.GetSliderPosition());
		SaveIniSettings(wol_Misc_Ini.Section, wol_Misc_Ini.RandomizerSlotsMax[LOWORD(wParam) - IDC_WOL_SLOT1S], m_max.GetSliderPosition());
		g_RandomizerSlots[LOWORD(wParam) - IDC_WOL_SLOT1S].min = m_min.GetSliderPosition();
		g_RandomizerSlots[LOWORD(wParam) - IDC_WOL_SLOT1S].max = m_max.GetSliderPosition();
		break;
	}
	case IDC_WOL_RANDOMIZE:
	{
		if (RandomizeSelectedMidiVelocities(m_min.GetSliderPosition(), m_max.GetSliderPosition()))
			Undo_OnStateChangeEx2(NULL, m_undoDesc.data(), UNDO_STATE_ALL, -1);
		break;
	}
	}
}

INT_PTR RandomMidiVelWnd::OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_VSCROLL)
	{
		switch (lParam)
		{
		case MSG_MIN:
		{
			m_minText.SetValue(m_min.GetSliderPosition());
			if (m_min.GetSliderPosition() > m_max.GetSliderPosition())
			{
				m_max.SetSliderPosition(m_min.GetSliderPosition());
				m_maxText.SetValue(m_max.GetSliderPosition());
			}
			break;
		}
		case MSG_MAX:
		{
			m_maxText.SetValue(m_max.GetSliderPosition());
			if (m_max.GetSliderPosition() < m_min.GetSliderPosition())
			{
				m_min.SetSliderPosition(m_max.GetSliderPosition());
				m_minText.SetValue(m_min.GetSliderPosition());
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

void RandomMidiVelWnd::Update()
{
	m_parentVwnd.RequestRedraw(NULL);
}

void RandomizeSelectedMidiVelocitiesTool(COMMAND_T* ct)
{
	if ((int)ct->user == 0)
	{
		if (RandomMidiVelWnd* w = g_RandMidiVelWndMgr.Create())
		{
			w->Show(true, true);
			w->SetUndoDescription(SWS_CMD_SHORTNAME(ct));
		}
	}
	else
	{
		if (RandomizeSelectedMidiVelocities(g_RandomizerSlots[(int)ct->user - 1].min, g_RandomizerSlots[(int)ct->user - 1].max))
			Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ALL, -1);
	}
}

int IsRandomizeSelectedMidiVelocitiesOpen(COMMAND_T* ct)
{
	if ((int)ct->user == 0)
		if (RandomMidiVelWnd* w = g_RandMidiVelWndMgr.Get())
			return w->IsValidWindow();
	return 0;
}

bool RandomizeSelectedMidiVelocities(int min, int max)
{
	if (MediaItem_Take* take = MIDIEditor_GetTake(MIDIEditor_GetActive()))
	{
		if (min > 0 && max < 128)
		{
			if (min < max)
			{
				int i = -1;
				while ((i = MIDI_EnumSelNotes(take, i)) != -1)
				{
					int vel = min + (int)(g_MTRand.rand()*(max - min) + 0.5);
					MIDI_SetNote(take, i, NULL, NULL, NULL, NULL, NULL, NULL, &vel);
				}
			}
			else
			{
				int i = -1;
				while ((i = MIDI_EnumSelNotes(take, i)) != -1)
					MIDI_SetNote(take, i, NULL, NULL, NULL, NULL, NULL, NULL, &min);
			}
			return true;
		}
	}
	return false;
}



///////////////////////////////////////////////////////////////////////////////////////////////////
// Mixer
///////////////////////////////////////////////////////////////////////////////////////////////////
void ScrollMixer(COMMAND_T* ct)
{
	MediaTrack* tr = GetMixerScroll();
	if (tr)
	{
		int count = CountTracks(NULL);
		int i = (int)GetMediaTrackInfo_Value(tr, "IP_TRACKNUMBER") - 1;
		MediaTrack* nTr = NULL;
		if ((int)ct->user == 0)
		{
			for (int k = 1; k <= i; ++k)
			{
				if (IsTrackVisible(GetTrack(NULL, i - k), true))
				{
					nTr = GetTrack(NULL, i - k);
					break;
				}
			}
		}
		else
		{
			for (int k = 1; k < count - i; ++k)
			{
				if (IsTrackVisible(GetTrack(NULL, i + k), true))
				{
					nTr = GetTrack(NULL, i + k);
					break;
				}
			}
		}
		if (nTr && nTr != tr)
			SetMixerScroll(nTr);
	}
}



///////////////////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void wol_MiscInit()
{
	for (int i = 0; i < 8; ++i)
	{
		g_RandomizerSlots[i].min = GetIniSettings(wol_Misc_Ini.Section, wol_Misc_Ini.RandomizerSlotsMin[i], 1);
		g_RandomizerSlots[i].max = GetIniSettings(wol_Misc_Ini.Section, wol_Misc_Ini.RandomizerSlotsMax[i], 127);
	}		

	g_RandMidiVelWndMgr.Init();
}

void wol_MiscExit()
{
	g_RandMidiVelWndMgr.Delete();
}