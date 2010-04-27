/******************************************************************************
/ SnM_FX.cpp
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


///////////////////////////////////////////////////////////////////////////////
// FX online/offline, bypass/unbypass
///////////////////////////////////////////////////////////////////////////////

void patchSelTracksFXState(int _mode, int _token, int _fx, const char* _value, const char * _undoMsg)
{
	bool updated = false;
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false); //include master
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			int fxId = _fx - 1;
			if (fxId == -1) // selected fx
				fxId = getSelectedFX(tr); //-1 on error
			if (fxId < 0)
				// Could support "second to last" action with -2, etc
				fxId = TrackFX_GetCount(tr) + _fx; 

			if (fxId >= 0)
			{
				SNM_ChunkParserPatcher p(tr);
				updated = (p.ParsePatch(_mode, 2, "FXCHAIN", "BYPASS", 3, fxId, _token, (void*)_value) > 0);
			}
		}
	}

	// Undo point
	if (updated && _undoMsg)
		Undo_OnStateChangeEx(_undoMsg, UNDO_STATE_ALL/*UNDO_STATE_FX*/, -1);
}

void toggleFXOfflineSelectedTracks(COMMAND_T* _ct) { 
	patchSelTracksFXState(SNM_TOGGLE_CHUNK_INT, 2, (int)_ct->user, NULL, SNM_CMD_SHORTNAME(_ct)); 
} 
  
void toggleFXBypassSelectedTracks(COMMAND_T* _ct) { 
	patchSelTracksFXState(SNM_TOGGLE_CHUNK_INT, 1, (int)_ct->user, NULL, SNM_CMD_SHORTNAME(_ct)); 
} 
  
void toggleExceptFXOfflineSelectedTracks(COMMAND_T* _ct) { 
	patchSelTracksFXState(SNM_TOGGLE_CHUNK_INT_EXCEPT, 2, (int)_ct->user, NULL, SNM_CMD_SHORTNAME(_ct)); 
} 
  
void toggleExceptFXBypassSelectedTracks(COMMAND_T* _ct) { 
	patchSelTracksFXState(SNM_TOGGLE_CHUNK_INT_EXCEPT, 1, (int)_ct->user, NULL, SNM_CMD_SHORTNAME(_ct)); 
} 
  
void toggleAllFXsOfflineSelectedTracks(COMMAND_T* _ct) { 
	// We use the "except mode" but with an unreachable fx number 
	patchSelTracksFXState(SNM_TOGGLE_CHUNK_INT_EXCEPT, 2, 0xFFFF, NULL, SNM_CMD_SHORTNAME(_ct)); 
} 
  
void toggleAllFXsBypassSelectedTracks(COMMAND_T* _ct) { 
	// We use the "except mode" but with an unreachable fx number 
	patchSelTracksFXState(SNM_TOGGLE_CHUNK_INT_EXCEPT, 1, 0xFFFF, NULL, SNM_CMD_SHORTNAME(_ct)); 
} 

void setFXOfflineSelectedTracks(COMMAND_T* _ct) { 
	patchSelTracksFXState(SNM_SET_CHUNK_CHAR, 2, (int)_ct->user, "1", SNM_CMD_SHORTNAME(_ct)); 
} 
  
void setFXBypassSelectedTracks(COMMAND_T* _ct) { 
	patchSelTracksFXState(SNM_SET_CHUNK_CHAR, 1, (int)_ct->user, "1", SNM_CMD_SHORTNAME(_ct)); 
} 

void setFXOnlineSelectedTracks(COMMAND_T* _ct) { 
	patchSelTracksFXState(SNM_SET_CHUNK_CHAR, 2, (int)_ct->user, "0", SNM_CMD_SHORTNAME(_ct)); 
} 
  
void setFXUnbypassSelectedTracks(COMMAND_T* _ct) { 
	patchSelTracksFXState(SNM_SET_CHUNK_CHAR, 1, (int)_ct->user, "0", SNM_CMD_SHORTNAME(_ct)); 
} 

void setAllFXsBypassSelectedTracks(COMMAND_T* _ct) {
	char cInt[2] = "";
	sprintf(cInt, "%d", (int)_ct->user);
	// We use the "except mode" but with an unreachable fx number 
	patchSelTracksFXState(SNM_SETALL_CHUNK_CHAR_EXCEPT, 1, 0xFFFF, cInt, SNM_CMD_SHORTNAME(_ct)); 
}


///////////////////////////////////////////////////////////////////////////////
// FX selection
///////////////////////////////////////////////////////////////////////////////

int selectFX(MediaTrack* _tr, int _fx)
{
	int updates = 0;
	if (_tr && _fx >=0 && _fx < TrackFX_GetCount(_tr))
	{
		SNM_ChunkParserPatcher p(_tr);
		char pLastSel[4] = ""; // 4 if there're many FXs
		sprintf(pLastSel,"%d", _fx);
		char pShow[4] = ""; // 4 if there're many FXs
		if (p.Parse(SNM_GET_CHUNK_CHAR,2,"FXCHAIN","SHOW",2,0,1,&pShow) > 0)
		{
			// Also patch the shown FX if the fx chain dlg is opened
			if (strcmp(pShow,"0") != 0)
			{
				sprintf(pShow,"%d", _fx+1);
				updates = p.ParsePatch(SNM_SET_CHUNK_CHAR,2,"FXCHAIN","SHOW",2,0,1,&pShow);
			}
			updates = p.ParsePatch(SNM_SET_CHUNK_CHAR,2,"FXCHAIN","LASTSEL",2,0,1,&pLastSel);
		}
	}
	return updates;
}

int getSelectedFX(MediaTrack* _tr)
{
	if (_tr)
	{
		// Avoid a useless parsing (but what does TrackFX_GetChainVisible ?)
		// TrackFX_GetChainVisible: returns index of effect visible in chain, or -1 for chain hidden, or -2 for chain visible but no effect selected
		int currentFX = TrackFX_GetChainVisible(_tr);
		if (currentFX >= 0)
			return currentFX;

		SNM_ChunkParserPatcher p(_tr);
		char pLastSel[4] = ""; // 4 if there're many FXs
		p.Parse(SNM_GET_CHUNK_CHAR,2,"FXCHAIN","LASTSEL",2,0,1,&pLastSel); 
		return atoi(pLastSel);
	}
	return -1;
}

void selectFX(COMMAND_T* _ct) 
{
	bool updated = false;
	int fx = (int)_ct->user;
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false); //include master
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			int sel = -1;
			int nbFx = -1;
			switch(fx)
			{
				// sel. previous
				case -2:
					sel = getSelectedFX(tr); // -1 on error
					nbFx = TrackFX_GetCount(tr);
					if (sel > 0) sel--;
					else if (sel == 0) sel = nbFx-1;
					break;

				// sel. next
				case -1:
					sel = getSelectedFX(tr); // -1 on error
					nbFx = TrackFX_GetCount(tr);
					if (sel >= 0 && sel < (nbFx-1)) sel++;
					else if (sel == (nbFx-1)) sel = 0;
					break;
					
				default:
					sel=fx;
					break;
			}

			if (sel >=0)
				updated = (selectFX(tr, sel) > 0);
		}
	}
	// Undo point
	if (updated)
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}

