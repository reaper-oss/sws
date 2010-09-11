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

int getFXIdFromCommand(MediaTrack* _tr, int _fxCmdId)
{
	int fxId = -1;
	if (_tr)
	{
		fxId = _fxCmdId - 1;
		if (fxId == -1) // selected fx
		{
			fxId = getSelectedFX(_tr); //-1 on error
			if (fxId < 0) return -1;
		}
		if (fxId < 0)
			// Could support "second to last" action with -2, etc
			fxId = TrackFX_GetCount(_tr) + _fxCmdId; 
	}
	return fxId;
}

// Gets a toggle state
// _token: 1=bypass, 2=offline 
// note: if the user has a single track selection that changes, REAPER refreshes 
// refreshes the menu ticks, toolbar tooltips ("on"/"off") but not the button themselves..
bool isFXOfflineOrBypassedSelectedTracks(COMMAND_T * _ct, int _token) 
{
	int selTrCount = CountSelectedTracksWithMaster(NULL);
	// single track selection: we can return a toggle state
	if (selTrCount == 1)
	{
		MediaTrack* tr = GetFirstSelectedTrackWithMaster(NULL);
		int fxId = getFXIdFromCommand(tr, (int)_ct->user);
		if (tr && fxId >= 0)
		{
			char state[2];
			SNM_ChunkParserPatcher p(tr);
			if (p.ParsePatch(SNM_GET_CHUNK_CHAR, 2, "FXCHAIN", "BYPASS", 3, fxId, _token, state) > 0)
				return !strcmp(state,"1");
		}
	}
	// several tracks selected: possible mix of different state 
	// => return a fake toggle state (best effort)
	else if (selTrCount)
		return fakeIsToggledAction(_ct);
	return false;
}

// *** CORE FUNC ***
bool patchSelTracksFXState(int _mode, int _token, int _fxCmdId, const char* _value, const char * _undoMsg)
{
	bool updated = false;
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false); //include master
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			int fxId = getFXIdFromCommand(tr, _fxCmdId);
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
	return updated;
}

void toggleFXOfflineSelectedTracks(COMMAND_T* _ct) { 
	if (patchSelTracksFXState(SNM_TOGGLE_CHUNK_INT, 2, (int)_ct->user, NULL, SNM_CMD_SHORTNAME(_ct)) && 
		CountSelectedTracksWithMaster(NULL) > 1)
	{
		fakeToggleAction(_ct);
	}
} 

bool isFXOfflineSelectedTracks(COMMAND_T * _ct) {
	return isFXOfflineOrBypassedSelectedTracks(_ct, 2);
}

void toggleFXBypassSelectedTracks(COMMAND_T* _ct) { 
	if (patchSelTracksFXState(SNM_TOGGLE_CHUNK_INT, 1, (int)_ct->user, NULL, SNM_CMD_SHORTNAME(_ct)) &&
		CountSelectedTracksWithMaster(NULL) > 1)
	{
		fakeToggleAction(_ct);
	}
} 

bool isFXBypassedSelectedTracks(COMMAND_T * _ct) {
	return isFXOfflineOrBypassedSelectedTracks(_ct, 1);
}

void toggleExceptFXOfflineSelectedTracks(COMMAND_T* _ct) { 
	if (patchSelTracksFXState(SNM_TOGGLE_CHUNK_INT_EXCEPT, 2, (int)_ct->user, NULL, SNM_CMD_SHORTNAME(_ct)) &&
		CountSelectedTracksWithMaster(NULL) > 1)
	{
		fakeToggleAction(_ct);
	}
} 
  
void toggleExceptFXBypassSelectedTracks(COMMAND_T* _ct) { 
	if (patchSelTracksFXState(SNM_TOGGLE_CHUNK_INT_EXCEPT, 1, (int)_ct->user, NULL, SNM_CMD_SHORTNAME(_ct)) &&
		CountSelectedTracksWithMaster(NULL) > 1)
	{
		fakeToggleAction(_ct);
	}
} 
  
void toggleAllFXsOfflineSelectedTracks(COMMAND_T* _ct) { 
	// We use the "except mode" but with an unreachable fx number 
	if (patchSelTracksFXState(SNM_TOGGLE_CHUNK_INT_EXCEPT, 2, 0xFFFF, NULL, SNM_CMD_SHORTNAME(_ct)) &&
		CountSelectedTracksWithMaster(NULL) > 1)
	{
		fakeToggleAction(_ct);
	}
} 
  
void toggleAllFXsBypassSelectedTracks(COMMAND_T* _ct) { 
	// We use the "except mode" but with an unreachable fx number 
	if (patchSelTracksFXState(SNM_TOGGLE_CHUNK_INT_EXCEPT, 1, 0xFFFF, NULL, SNM_CMD_SHORTNAME(_ct)) && 
		CountSelectedTracksWithMaster(NULL) > 1)
	{
		fakeToggleAction(_ct);
	}
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


///////////////////////////////////////////////////////////////////////////////
// FX preset
///////////////////////////////////////////////////////////////////////////////

//JFB TODO: cache?
int getPresetNames(const char* _fxType, const char* _fxName, WDL_PtrList<WDL_String>* _names)
{
	int nbPresets = 0;
	if (_fxType && _fxName && _names)
	{
		char iniFilename[BUFFER_SIZE], buf[256];

		// *** Get ini filename *** //

		// Process FX name
		strncpy(buf, _fxName, 256);

		// remove ".dll"
		//JFB!! OSX: to check
		if (!_stricmp(_fxType, "VST"))
		{
			const char* p = stristr(buf,".dll");
			if (p) buf[(int)(p-buf)] = '\0';
		}

		// replace special chars
		int i=0;
		while (buf[i])
		{
			if (buf[i] == '.' || buf[i] == '/')
				buf[i] = '_';
			i++;
		}

		char* fxType = _strdup(_fxType);
		for (i = 0; i < (int)strlen(fxType); i++)
			fxType[i] = tolower(fxType[i]);
		_snprintf(iniFilename, BUFFER_SIZE, "%s%cpresets%c%s-%s.ini", GetResourcePath(), PATH_SLASH_CHAR, PATH_SLASH_CHAR, fxType, buf);
		if (!FileExists(iniFilename))
			_snprintf(iniFilename, BUFFER_SIZE, "%s%cpresets-%s-%s.ini", GetResourcePath(), PATH_SLASH_CHAR, fxType, buf);
		free(fxType);

		// *** Get presets *** //

		if (FileExists(iniFilename))
		{
			char key[32];
			GetPrivateProfileString("General", "NbPresets", "0", buf, 5, iniFilename);
			nbPresets = atoi(buf);
			for (int i=0; i < nbPresets; i++)
			{
				_snprintf(key, 32, "Preset%d", i);
				GetPrivateProfileString(key, "Name", "", buf, 256, iniFilename);
				_names->Add(new WDL_String(buf));
			}
		}
	}
	return nbPresets;
}

int GetPresetFromConfToken(const char* _preset)
{
	const char* p = strchr(_preset, '.');
	if (p)
		return atoi(p+1);
	return 0;
}

// _fx: 1-based, _preset: 1-based with 0=remove
// _presetConf (in & out param): stored as "fx.preset" both 1-based
void UpdatePresetConf(int _fx, int _preset, WDL_String* _presetConf)
{
	WDL_String newConf;
	LineParser lp(false);
	if (_presetConf && !lp.parse(_presetConf->Get()))
	{
		bool found = false;
		for (int i=0; i < lp.getnumtokens(); i++)
		{
			int fx = (int)floor(lp.gettoken_float(i));
			if (_fx == fx)
			{
				// Set the new preset (if not removed)
				if (_preset) {
					found = true;
					if (newConf.GetLength()) newConf.Append(" ");
					newConf.AppendFormatted(256, "%d.%d", _fx, _preset);
				}
			}
			else {
				if (newConf.GetLength()) newConf.Append(" ");
				newConf.Append(lp.gettoken_str(i));
			}
		}

		if (!found && _preset) {
			if (newConf.GetLength()) newConf.Append(" ");
			newConf.AppendFormatted(256, "%d.%d", _fx, _preset);
		}

		_presetConf->Set(newConf.Get());
	}
}

// _fx: 0-based, _presetConf: stored as "fx.preset", both 1-based
// _presetCount: for optionnal check
int GetSelPresetFromConf(int _fx, WDL_String* _curPresetConf, int _presetCount)
{
	LineParser lp(false);
	if (_curPresetConf && !lp.parse(_curPresetConf->Get()))
	{
		for (int i=0; i < lp.getnumtokens(); i++)
		{
			int fx = (int)floor(lp.gettoken_float(i));
			int preset = GetPresetFromConfToken(lp.gettoken_str(i));
			if (_fx == (fx-1))
				return (preset > _presetCount) ? 0 : preset;
		}
	}
	return 0;
}

// _fx: 1-based, _preset: 1-based with 0=remove
// _presetConf: in param, _renderConf: out param
void RenderPresetConf(WDL_String* _presetConf, WDL_String* _renderConf)
{
	LineParser lp(false);
	if (_presetConf && _renderConf && !lp.parse(_presetConf->Get()))
	{
		for (int i=0; i < lp.getnumtokens(); i++)
		{
			int fx = (int)floor(lp.gettoken_float(i));
			int preset = GetPresetFromConfToken(lp.gettoken_str(i));
			if (_renderConf->GetLength()) _renderConf->Append(", ");
			_renderConf->AppendFormatted(256, "FX%d: %d", fx, preset);
		}	
	}
}

