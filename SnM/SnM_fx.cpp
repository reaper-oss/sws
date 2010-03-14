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

void patchSelTracksFXState(int _mode, int _token, int _fx, const char* _value, const char * _undoMsg)
{
	bool updated = false;
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			int fxId = _fx - 1;
			if (fxId < 0)
				fxId = TrackFX_GetCount(tr) + _fx; // Could support "second to last" action with -2, etc

			// get the current value
			SNM_ChunkParserPatcher p(tr);
			updated = (p.ParsePatch(_mode, 2, "FXCHAIN", "BYPASS", 3, fxId, _token, (void*)_value) > 0);
		}
	}

	// Undo point
	if (updated && _undoMsg)
		Undo_OnStateChangeEx(_undoMsg, UNDO_STATE_ALL/*UNDO_STATE_FX*/, -1);
}

void toggleFXOfflineSelectedTracks(COMMAND_T* _ct) { 
	patchSelTracksFXState(SNM_TOGGLE_CHUNK_INT, 2, (int)_ct->user, NULL, SNMSWS_ZAP(_ct)); 
} 
  
void toggleFXBypassSelectedTracks(COMMAND_T* _ct) { 
	patchSelTracksFXState(SNM_TOGGLE_CHUNK_INT, 1, (int)_ct->user, NULL, SNMSWS_ZAP(_ct)); 
} 
  
void toggleExceptFXOfflineSelectedTracks(COMMAND_T* _ct) { 
	patchSelTracksFXState(SNM_TOGGLE_CHUNK_INT_EXCEPT, 2, (int)_ct->user, NULL, SNMSWS_ZAP(_ct)); 
} 
  
void toggleExceptFXBypassSelectedTracks(COMMAND_T* _ct) { 
	patchSelTracksFXState(SNM_TOGGLE_CHUNK_INT_EXCEPT, 1, (int)_ct->user, NULL, SNMSWS_ZAP(_ct)); 
} 
  
void toggleAllFXsOfflineSelectedTracks(COMMAND_T* _ct) { 
	// We use the "except mode" but with an unreachable fx number 
	patchSelTracksFXState(SNM_TOGGLE_CHUNK_INT_EXCEPT, 2, 0xFFFF, NULL, SNMSWS_ZAP(_ct)); 
} 
  
void toggleAllFXsBypassSelectedTracks(COMMAND_T* _ct) { 
	// We use the "except mode" but with an unreachable fx number 
	patchSelTracksFXState(SNM_TOGGLE_CHUNK_INT_EXCEPT, 1, 0xFFFF, NULL, SNMSWS_ZAP(_ct)); 
} 

void setFXOfflineSelectedTracks(COMMAND_T* _ct) { 
	patchSelTracksFXState(SNM_SET_CHUNK_CHAR, 2, (int)_ct->user, "1", SNMSWS_ZAP(_ct)); 
} 
  
void setFXBypassSelectedTracks(COMMAND_T* _ct) { 
	patchSelTracksFXState(SNM_SET_CHUNK_CHAR, 1, (int)_ct->user, "1", SNMSWS_ZAP(_ct)); 
} 

void setFXOnlineSelectedTracks(COMMAND_T* _ct) { 
	patchSelTracksFXState(SNM_SET_CHUNK_CHAR, 2, (int)_ct->user, "0", SNMSWS_ZAP(_ct)); 
} 
  
void setFXUnbypassSelectedTracks(COMMAND_T* _ct) { 
	patchSelTracksFXState(SNM_SET_CHUNK_CHAR, 1, (int)_ct->user, "0", SNMSWS_ZAP(_ct)); 
} 

void setAllFXsBypassSelectedTracks(COMMAND_T* _ct) {
	char cInt[2] = "";
	sprintf(cInt, "%d", (int)_ct->user);
	// We use the "except mode" but with an unreachable fx number 
	patchSelTracksFXState(SNM_SETALL_CHUNK_CHAR_EXCEPT, 1, 0xFFFF, cInt, SNMSWS_ZAP(_ct)); 
}
 
