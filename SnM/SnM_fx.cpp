/******************************************************************************
/ SnM_FX.cpp
/
/ Copyright (c) 2009-2011 Tim Payne (SWS), Jeffos
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


#define SNM_REA_PRESET_CB_ENDING		")  ----"
#define SNM_REA_USERPRESET_CB_ENDING	"(.rpl)  ----" 


int g_buggyPlugSupport = 0; // defined in S&M.ini


///////////////////////////////////////////////////////////////////////////////
// *TRACK* FX online/offline, bypass/unbypass
///////////////////////////////////////////////////////////////////////////////

int getTrackFXIdFromCmd(MediaTrack* _tr, int _fxCmdId)
{
	int fxId = -1;
	if (_tr)
	{
		fxId = _fxCmdId - 1;
		if (fxId == -1) // selected fx
		{
			fxId = getSelectedTrackFX(_tr); //-1 on error
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
bool isFXOfflineOrBypassedSelectedTracks(COMMAND_T * _ct, int _token) 
{
	int selTrCount = CountSelectedTracksWithMaster(NULL);
	// single track selection: we can return a toggle state
	if (selTrCount == 1)
	{
		MediaTrack* tr = GetFirstSelectedTrackWithMaster(NULL);
		int fxId = getTrackFXIdFromCmd(tr, (int)_ct->user);
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
	for (int i = 0; i <= GetNumTracks(); i++) // include master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			int fxId = getTrackFXIdFromCmd(tr, _fxCmdId);
			int shownFxId = -1;
			if (fxId >= 0)
			{
				SNM_ChunkParserPatcher p(tr);
				bool updt = (p.ParsePatch(_mode, 2, "FXCHAIN", "BYPASS", 3, fxId, _token, (void*)_value) > 0);
				updated |= updt;

				// close the GUI for buggy plugins
				// http://code.google.com/p/sws-extension/issues/detail?id=317
				if (updt && g_buggyPlugSupport && _token == 2)
				{
					p.ParsePatch(SNM_SETALL_CHUNK_CHAR_EXCEPT,2,"FXCHAIN","FLOAT",5,255,0,(void*)"FLOATPOS"); //unfloat all

					char pIdx[4] = "";
					int pos = p.ParsePatch(SNM_GET_CHUNK_CHAR, 2, "FXCHAIN", "SHOW",2,0,1,(void*)pIdx);
					if (pos > 0)
					{
						shownFxId = atoi(pIdx);
						if (shownFxId)
						{
							p.GetChunk()->DeleteSub(--pos,1);
							p.GetChunk()->Insert("0", pos);
						}
					}
				}
			} // chunk auto commit
			
			if (shownFxId > 0) // buggy plugs only: restore unfloated GUI (like REAPER does)
				TrackFX_Show(tr, shownFxId-1, 1);
		}
	}

	// Undo point
	if (updated && _undoMsg)
		Undo_OnStateChangeEx(_undoMsg, UNDO_STATE_ALL, -1); // using UNDO_STATE_FX crashes 
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
	if (patchSelTracksFXState(SNM_TOGGLE_CHUNK_INT_EXCEPT, 2, (int)_ct->user, NULL, SNM_CMD_SHORTNAME(_ct)))
		fakeToggleAction(_ct);
} 
  
void toggleExceptFXBypassSelectedTracks(COMMAND_T* _ct) { 
	if (patchSelTracksFXState(SNM_TOGGLE_CHUNK_INT_EXCEPT, 1, (int)_ct->user, NULL, SNM_CMD_SHORTNAME(_ct)))
		fakeToggleAction(_ct);
} 
  
void toggleAllFXsOfflineSelectedTracks(COMMAND_T* _ct) { 
	// We use the "except mode" but with an unreachable fx number 
	if (patchSelTracksFXState(SNM_TOGGLE_CHUNK_INT_EXCEPT, 2, 0xFFFF, NULL, SNM_CMD_SHORTNAME(_ct)))
		fakeToggleAction(_ct);
} 
  
void toggleAllFXsBypassSelectedTracks(COMMAND_T* _ct) { 
	// We use the "except mode" but with an unreachable fx number 
	if (patchSelTracksFXState(SNM_TOGGLE_CHUNK_INT_EXCEPT, 1, 0xFFFF, NULL, SNM_CMD_SHORTNAME(_ct)))
		fakeToggleAction(_ct);
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
// *TAKE* FX online/offline, bypass/unbypass
///////////////////////////////////////////////////////////////////////////////

// *** CORE FUNC ***
// note: all takes are patched atm (i.e. nothing specific to active takes yet)
bool patchSelItemsFXState(int _mode, int _token, int _fxId, const char* _value, const char * _undoMsg)
{
	bool updated = false;
	for (int i = 1; i <= GetNumTracks(); i++) // skip master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; tr && j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* item = GetTrackMediaItem(tr,j);
			if (item && *(bool*)GetSetMediaItemInfo(item,"B_UISEL",NULL))
			{
				SNM_ChunkParserPatcher p(item);
				bool updt = (p.ParsePatch(_mode, 2, "TAKEFX", "BYPASS", 3, _fxId, _token, (void*)_value) > 0);
				updated |= updt;

/*JFB not used: doesn't seem to occur with take FX
				// close the GUI for buggy plugins
				// http://code.google.com/p/sws-extension/issues/detail?id=317
				// API limitation: can't restore shown FX here (contrary to track FX)
				if (updt && g_buggyPlugSupport && _token == 2)
				{
					p.ParsePatch(SNM_SETALL_CHUNK_CHAR_EXCEPT,2,"TAKEFX","FLOAT",5,255,0,(void*)"FLOATPOS"); //unfloat all
					p.ParsePatch(SNM_SET_CHUNK_CHAR, 2, "TAKEFX", "SHOW",2,0,1,(void*)"0"); // no FX shown..
				}
*/
			}
		}
	}

	// Undo point
	if (updated && _undoMsg)
		Undo_OnStateChangeEx(_undoMsg, UNDO_STATE_ALL, -1);
	return updated;
}

void toggleAllFXsOfflineSelectedItems(COMMAND_T* _ct) { 
	// We use the "except mode" but with an unreachable fx number 
	if (patchSelItemsFXState(SNM_TOGGLE_CHUNK_INT_EXCEPT, 2, 0xFFFF, NULL, SNM_CMD_SHORTNAME(_ct)))
		fakeToggleAction(_ct);
} 
  
void toggleAllFXsBypassSelectedItems(COMMAND_T* _ct) { 
	// We use the "except mode" but with an unreachable fx number 
	if (patchSelItemsFXState(SNM_TOGGLE_CHUNK_INT_EXCEPT, 1, 0xFFFF, NULL, SNM_CMD_SHORTNAME(_ct)))
		fakeToggleAction(_ct);
} 

void setAllFXsOfflineSelectedItems(COMMAND_T* _ct) {
	char cInt[2] = "";
	sprintf(cInt, "%d", (int)_ct->user);
	// We use the "except mode" but with an unreachable fx number 
	patchSelItemsFXState(SNM_SETALL_CHUNK_CHAR_EXCEPT, 2, 0xFFFF, cInt, SNM_CMD_SHORTNAME(_ct));
}

void setAllFXsBypassSelectedItems(COMMAND_T* _ct) {
	char cInt[2] = "";
	sprintf(cInt, "%d", (int)_ct->user);
	// We use the "except mode" but with an unreachable fx number 
	patchSelItemsFXState(SNM_SETALL_CHUNK_CHAR_EXCEPT, 1, 0xFFFF, cInt, SNM_CMD_SHORTNAME(_ct));
}


///////////////////////////////////////////////////////////////////////////////
// Track FX selection
///////////////////////////////////////////////////////////////////////////////

int selectTrackFX(MediaTrack* _tr, int _fx)
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

void selectTrackFX(COMMAND_T* _ct) 
{
	bool updated = false;
	int fx = (int)_ct->user;
	for (int i = 0; i <= GetNumTracks(); i++)  // include master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			int sel = -1;
			switch(fx)
			{
				// sel. last
				case -3:
					sel = TrackFX_GetCount(tr) - 1; // -1 on error
					break;

				// sel. previous
				case -2:
					sel = getSelectedTrackFX(tr); // -1 on error
					if (sel > 0) sel--;
					else if (sel == 0) sel = TrackFX_GetCount(tr)-1;
					break;

				// sel. next
				case -1:
					sel = getSelectedTrackFX(tr); // -1 on error
					if (sel >= 0 && sel < (TrackFX_GetCount(tr)-1)) sel++;
					else if (sel == (TrackFX_GetCount(tr)-1)) sel = 0;
					break;
					
				default:
					sel=fx;
					break;
			}

			if (sel >=0)
				updated = (selectTrackFX(tr, sel) > 0);
		}
	}
	// Undo point
	if (updated)
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}

int getSelectedTrackFX(MediaTrack* _tr)
{
	if (_tr)
	{
		// Avoids a useless parsing (1st try)
		if (TrackFX_GetCount(_tr) == 1)
			return 0;

		// Avoids a useless parsing (2nd try) //JFB but what really does TrackFX_GetChainVisible ?
		// TrackFX_GetChainVisible: returns index of effect visible in chain, or -1 for chain hidden, or -2 for chain visible but no effect selected
		int currentFX = TrackFX_GetChainVisible(_tr);
		if (currentFX >= 0)
			return currentFX;

		// the 2 attempts above failed => no choice: parse to get the selected FX
		SNM_ChunkParserPatcher p(_tr);
		char pLastSel[4] = ""; // 4: if there're many, many FXs
		p.Parse(SNM_GET_CHUNK_CHAR,2,"FXCHAIN","LASTSEL",2,0,1,&pLastSel);
		return atoi(pLastSel); // return 0 (i.e. 1st FX) if failed
	}
	return -1;
}


///////////////////////////////////////////////////////////////////////////////
// Track FX presets
///////////////////////////////////////////////////////////////////////////////

//JFB cache?
int getPresetNames(const char* _fxType, const char* _fxName, WDL_PtrList<WDL_String>* _names)
{
	int nbPresets = 0;
	if (_fxType && _fxName && _names)
	{
		char iniFilename[BUFFER_SIZE], buf[256];

		// *** Get ini filename *** //

		// Process FX name
		lstrcpyn(buf, _fxName, 256);

		// remove ".dll"
		//JFB OSX: to check
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

// Updates a preset conf string
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

// Returns the preset number (1-based, 0 if failed) from a preset conf string
// _fx: 0-based, _presetConf: stored as "fx.preset", both 1-based
// _presetCount: for optionnal check
int GetPresetFromConf(int _fx, WDL_String* _presetConf, int _presetCount)
{
	LineParser lp(false);
	if (_presetConf && !lp.parse(_presetConf->Get()))
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

// Returns a human readable preset conf string
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

// Switch presets
//
// Primitive to trigger a given preset 
// No proper API yet => POOR'S MAN'S SOLUTION!
//
// So, it simulates an user action: show fx (if needed) -> dropdown event -> close fx (if needed)
// Notes
// - undo indirectly managed (i.e.native "Undo FX parameter adjustement"
// - when triggering the "Restore factory setting" preset (1st item in the dropdown), then
//   REAPER automatically re-update the preset. In other words, depending on the FX, setting the 
//   1st preset may result in selecting the 7th preset (i.e. which is the factory setting preset)
//
// _userPreset: if true, _presetId is a user preset index. Index in the preset dropdown if false  
// _presetId: preset index to be triggered (only taken into account when _dir == 0, see below)
// _dir: +1 or -1 switches to next or previous preset (_presetId is ignored then)
// returns the triggered preset id or -1 if failed
int triggerFXPreset(MediaTrack* _tr, int _fxId, int _presetId, int _dir, bool _userPreset)
{
	int nbFx = _tr ? TrackFX_GetCount(_tr) : 0;
	if (nbFx > 0 && _fxId < nbFx)
	{
		bool presetOk = false, floated = false;

		// float the FX if needed
		// stupid if no preset exposed, of course..
		HWND hFX = TrackFX_GetFloatingWindow(_tr, _fxId);
		if (!hFX) {
			floatUnfloatFXs(_tr, false, 3, _fxId, true); 
			hFX = TrackFX_GetFloatingWindow(_tr, _fxId);
			floated = (hFX != NULL);
		}

		HWND hFX2 = hFX ? GetDlgItem(hFX, 0) : NULL;
		HWND cbPreset = hFX2 ? GetDlgItem(hFX2, 0x3E8) : NULL;
		if (cbPreset)
		{
			int presetCount = (int)SendMessage(cbPreset, CB_GETCOUNT, 0, 0);
			int currentPreset = (int)SendMessage(cbPreset, CB_GETCURSEL, 0, 0);

			// user preset only ?
			if (_userPreset)
			{
				int deltaIdx = 0;
				char buf[256] = "";
				while ((deltaIdx+1) < presetCount && (strlen(buf) < strlen(SNM_REA_USERPRESET_CB_ENDING) || strcmp((char*)(buf+strlen(buf)-strlen(SNM_REA_USERPRESET_CB_ENDING)), SNM_REA_USERPRESET_CB_ENDING)))
				{
					SendMessage(cbPreset, CB_SETCURSEL, ++deltaIdx, 0); // GUI update only
					GetWindowText(cbPreset, buf, 256);
				}
				if ((deltaIdx+1) < presetCount) _presetId += (deltaIdx+1);
				else _presetId = -1; // no user presets
				SendMessage(cbPreset, CB_SETCURSEL, currentPreset, 0); // restore the current preset (GUI update only)
			}

			if (currentPreset >= 0 && _presetId >= 0 && 
				((_dir && currentPreset < presetCount) || (!_dir && _presetId < presetCount && _presetId != currentPreset)))
			{
				char buf[256] = SNM_REA_PRESET_CB_ENDING;
				if (_dir) _presetId = currentPreset;
				presetOk = true;
				while (strstr(buf, "No preset") || (strlen(buf) >= strlen(SNM_REA_PRESET_CB_ENDING) && !strcmp((char*)(buf+strlen(buf)-strlen(SNM_REA_PRESET_CB_ENDING)), SNM_REA_PRESET_CB_ENDING)))
				{
					_presetId += _dir;
					if (_presetId < 0 || _presetId >= presetCount) {
						presetOk = false;
						break;
					}		
					SendMessage(cbPreset, CB_SETCURSEL, _presetId, 0); // GUI update only
					GetWindowText(cbPreset, buf, 256);
					if (!_dir) _dir = 1;
				}
			}

			// switch the preset for real
			if (presetOk)
				SendMessage(hFX2, WM_COMMAND, MAKELONG(GetWindowLong(cbPreset, GWL_ID), CBN_SELCHANGE), (LPARAM)_presetId);
			// restore the current preset (GUI update only)
			else
				SendMessage(cbPreset, CB_SETCURSEL, currentPreset, 0);
		}

		// unfloat the FX if needed
		if (floated)
			floatUnfloatFXs(_tr, false, 2, _fxId, true);

		if (presetOk)
			return _presetId;
	}
	return -1;
}

void triggerFXPreset(int _fxId, int _presetId, int _dir)
{
	for (int i = 0; g_bv4 && i <= GetNumTracks(); i++) // include master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			int fxId = _fxId;
			if (fxId == -1)
				fxId = getSelectedTrackFX(tr);
			if (fxId >= 0)
				triggerFXPreset(tr, fxId, _presetId, _dir);
		}
	}
}

void triggerNextPreset(COMMAND_T* _ct) {
	triggerFXPreset((int)_ct->user, 0, 1);
}

void triggerPreviousPreset(COMMAND_T* _ct) {
	triggerFXPreset((int)_ct->user, 0, -1);
}


// trigger several *USER* presets from several FXs on a same track 
// _presetConf: "fx.preset", both 1-based - e.g. "1.4 2.2" => FX1 user preset 4, FX2 user preset 2 
bool triggerFXUserPreset(MediaTrack* _tr, WDL_String* _presetConf)
{
	bool updated = false;
	int nbFx = _tr ? TrackFX_GetCount(_tr) : 0;
	if (nbFx)
	{
		for (int i=0; i < nbFx; i++)
		{
			int preset = GetPresetFromConf(i, _presetConf);
			if (preset)
				updated |= (triggerFXPreset(_tr, i, preset-1, 0, true) != -1);
		}
	}
	return updated;
}


// *** Trigger preset through MIDI CC action ***

class SNM_TriggerPresetScheduledJob : public SNM_ScheduledJob {
public:
	SNM_TriggerPresetScheduledJob(int _approxDelayMs, int _val, int _valhw, int _relmode, HWND _hwnd, int _fxId) 
		: SNM_ScheduledJob(SNM_SCHEDJOB_TRIG_PRESET, _approxDelayMs),m_val(_val),m_valhw(_valhw),m_relmode(_relmode),m_hwnd(_hwnd),m_fxId(_fxId) {}
	void Perform() {triggerFXPreset(m_fxId, m_val, 0);}
protected:
	int m_val, m_valhw, m_relmode, m_fxId;
	HWND m_hwnd;
};

void TriggerFXPreset(MIDI_COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd) 
{
	// Absolute CC only
	if (!_relmode && _valhw < 0 && g_bv4)
	{
		SNM_TriggerPresetScheduledJob* job = 
			new SNM_TriggerPresetScheduledJob(SNM_SCHEDJOB_DEFAULT_DELAY, _val, _valhw, _relmode, _hwnd, (int)_ct->user);
		AddOrReplaceScheduledJob(job);
	}
}


///////////////////////////////////////////////////////////////////////////////
// Other
///////////////////////////////////////////////////////////////////////////////

// http://code.google.com/p/sws-extension/issues/detail?id=258
// pretty brutal code (we'd need a dedicated parser/patcher here..)
void moveFX(COMMAND_T* _ct)
{
	bool updated = false;
	int dir = (int)_ct->user;
	for (int i = 0; i <= GetNumTracks(); i++) // include master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			int nbFx = TrackFX_GetCount(tr);
			if (nbFx > 0)
			{
				int sel = getSelectedTrackFX(tr);
				if (sel >= 0 && ((dir == 1 && sel < (nbFx-1)) || (dir == -1 && sel > 0)))
				{
					SNM_ChunkParserPatcher p(tr);
					WDL_String chainChunk;
					if (p.GetSubChunk("FXCHAIN", 2, 0, &chainChunk, "<ITEM") > 0)
					{
						int originalChainLen = chainChunk.GetLength();
						SNM_ChunkParserPatcher pfxc(&chainChunk);
						int p1 = pfxc.Parse(SNM_GET_CHUNK_CHAR,1,"FXCHAIN","BYPASS",3,sel,0);
						if (p1>0)
						{
							p1--; 
							int p2 = pfxc.Parse(SNM_GET_CHUNK_CHAR,1,"FXCHAIN","BYPASS",3,sel+1,0);
							if (p2 > 0)	p2--;
							else p2 = chainChunk.GetLength()-2; // -2 for ">\n"

							// store & cut fx
							WDL_String fxChunk;
							fxChunk.Set(pfxc.GetChunk()->Get()+p1, p2-p1);
							pfxc.GetChunk()->DeleteSub(p1, p2-p1);
							
							// move fx
							int p3 = pfxc.Parse(SNM_GET_CHUNK_CHAR,1,"FXCHAIN","BYPASS",3,sel+dir,0);
							if (p3>0) pfxc.GetChunk()->Insert(fxChunk.Get(), --p3);
							else if (dir == 1) pfxc.GetChunk()->Insert(fxChunk.Get(), pfxc.GetChunk()->GetLength()-2);

							// patch updated chunk
							if (p.ReplaceSubChunk("FXCHAIN", 2, 0, pfxc.GetChunk()->Get(), "<ITEM"))
							{
								updated = true;
								p.Commit(true);
								selectTrackFX(tr, sel + dir);
							}
						}
					}
				}
			}
		}
	}

	// Undo point
	if (updated)
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}
