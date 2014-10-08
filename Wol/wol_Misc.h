/******************************************************************************
/ wol_Misc.h
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

#pragma once

#include "../SnM/SnM_VWnd.h"

void MoveEditCursorToBeatN(COMMAND_T* ct);
void MoveEditCursorTo(COMMAND_T* ct);

void SelectAllTracksExceptFolderParents(COMMAND_T* ct);

void MoveEditCursorToNote(COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd);
void RandomizeSelectedMidiVelocitiesTool(COMMAND_T* ct);
int IsRandomizeSelectedMidiVelocitiesOpen(COMMAND_T* ct);
class RandomMidiVelWnd : public SWS_DockWnd
{
public:
	RandomMidiVelWnd();

	void Update();
	void SetUndoDescription(const char* desc) { m_undoDesc.clear(); m_undoDesc = desc; }

protected:
	void OnInitDlg();
	void OnDestroy();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	void DrawControls(LICE_IBitmap* bm, const RECT* r, int* tooltipHeight = NULL);
	INT_PTR OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	SNM_Knob m_min, m_max;
	SNM_KnobCaption m_minText, m_maxText;

	HWND m_btnL, m_btnR;

	string m_undoDesc;
};

void ScrollMixer(COMMAND_T* ct);



void wol_MiscInit();
void wol_MiscExit();