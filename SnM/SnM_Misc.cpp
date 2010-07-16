/******************************************************************************
/ SnM_Misc.cpp
/
/ Copyright (c) 2009-2010 Tim Payne (SWS), JF Bédague 
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
#include "SnM_Actions.h"
#include "SNM_ChunkParserPatcher.h"


// see http://forum.cockos.com/showthread.php?t=60657
void letREAPERBreathe(COMMAND_T* _ct)
{
#ifdef _WIN32
  static bool hasinit;
  if (!hasinit) { hasinit=true; InitCommonControls();  }
#endif
	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_SNM_WAIT), GetForegroundWindow(), WaitDlgProc);
}

#ifdef _SNM_MISC

///////////////////////////////////////////////////////////////////////////////
// tests..
///////////////////////////////////////////////////////////////////////////////

void ShowTakeEnvPadreTest(COMMAND_T* _ct)
{
	bool updated = false;
	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i+1,false); // doesn't include master
		for (int j = 0; tr && j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* item = GetTrackMediaItem(tr,j);
			if (item && *(bool*)GetSetMediaItemInfo(item,"B_UISEL",NULL))
			{
				switch(_ct->user)
				{
					case 1: updated |= ShowTakeEnvPan(GetActiveTake(item)); break;
					case 2: updated |= ShowTakeEnvMute(GetActiveTake(item)); break;
					case 0: default: updated |= ShowTakeEnvVol(GetActiveTake(item)); break;
				}
			}
		}
	}
	if (updated)
	{
		UpdateTimeline();
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
}

void openStuff(COMMAND_T* _ct)
{
}

#endif

