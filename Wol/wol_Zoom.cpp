/******************************************************************************
/ wol_Zoom.cpp
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
#include "wol_Zoom.h"
#include "../sws_util.h"
#include "../SnM/SnM_Dlg.h"
#include "../SnM/SnM_Util.h"
#include "../Breeder/BR_Util.h"
#include "../Breeder/BR_EnvTools.h"


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

int GetTcpMinHeight()
{
	IconTheme* theme = SNM_GetIconTheme();
	return theme->tcp_small_height;
}

int GetCurrentTcpMaxHeight()
{
	RECT r;
	GetClientRect(GetArrangeWnd(), &r);
	return r.bottom - r.top;
}

void VerticalZoomSelectedEnvelope(COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd)
{
	if (relmode > 0)
	{
		if (TrackEnvelope* env = GetSelectedTrackEnvelope(NULL))
		{
			int maxHeight = GetCurrentTcpMaxHeight();
			int minHeight = GetTcpMinHeight();
			BR_Envelope brEnv(env);
			int height = brEnv.LaneHeight() + AdjustRelative(relmode, (valhw == -1) ? BOUNDED(val, -1, 128) : (int)BOUNDED(16384.0 - (valhw | val << 7), 0.0, 16383.0));
			if (height < minHeight) height = minHeight;
			if (height > maxHeight) height = maxHeight;
			brEnv.SetLaneHeight(height);
			brEnv.Commit();
		}
	}
}

void SetVerticalZoomSelectedEnvelope(COMMAND_T* ct)
{
	if (TrackEnvelope* env = GetSelectedTrackEnvelope(NULL))
	{
		BR_Envelope brEnv(env);
		brEnv.SetLaneHeight(((int)ct->user == 0) ? 0 : GetTcpMinHeight());
		brEnv.Commit();
	}
}

void SetVerticalZoomCenter(COMMAND_T* ct)
{
	SetConfig("vzoommode", (int)ct->user);
}

void SetHorizontalZoomCenter(COMMAND_T* ct)
{
	SetConfig("zoommode", (int)ct->user);
}