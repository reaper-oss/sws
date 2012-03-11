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
#include "SnM_Chunk.h"


///////////////////////////////////////////////////////////////////////////////
// Track FX helpers
///////////////////////////////////////////////////////////////////////////////

// return -1 on error
int GetFXByGUID(MediaTrack* _tr, GUID* _g)
{
	if (_tr)
	{
		int fx=0, cntFX = TrackFX_GetCount(_tr);
		while (fx < cntFX) {
			if (GuidsEqual(TrackFX_GetFXGUID(_tr, fx), _g))
				return fx;
			fx++;
		}
	}
	return -1;
}


///////////////////////////////////////////////////////////////////////////////
// *TRACK* FX online/offline, bypass/unbypass
///////////////////////////////////////////////////////////////////////////////

int g_buggyPlugSupport = 0; // defined in S&M.ini

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
			// could support "second to last" action with -2, etc
			fxId = TrackFX_GetCount(_tr) + _fxCmdId; 
	}
	return fxId;
}

// Gets a toggle state
// _token: 1=bypass, 2=offline 
bool isFXOfflineOrBypassedSelectedTracks(COMMAND_T * _ct, int _token) 
{
	int selTrCount = SNM_CountSelectedTracks(NULL, true);
	// single track selection: we can return a toggle state
	if (selTrCount == 1)
	{
		MediaTrack* tr = SNM_GetSelectedTrack(NULL, 0, true);
		int fxId = getTrackFXIdFromCmd(tr, (int)_ct->user);
		if (tr && fxId >= 0)
		{
			/////////////////////////////////////////////////
			// dedicated API for fx bypass since v4.16pre
			if (_token==1 && TrackFX_GetEnabled)
				return TrackFX_GetEnabled(tr, fxId);

			/////////////////////////////////////////////////
			// chunk parsing otherwise
			else
			{
				char state[2] = "0";
				SNM_ChunkParserPatcher p(tr);
				p.SetWantsMinimalState(true);
				if (p.Parse(SNM_GET_CHUNK_CHAR, 2, "FXCHAIN", "BYPASS", fxId, _token, state) > 0)
					return !strcmp(state,"1");
			}
		}
	}
	// several tracks selected: possible mix of different state 
	// => return a fake toggle state (best effort)
	else if (selTrCount)
		return FakeIsToggleAction(_ct);
	return false;
}

// *** CORE FUNC ***
//JFB!!! TODO: dedicated API for fx bypass since v4.16pre
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
				bool updt = (p.ParsePatch(_mode, 2, "FXCHAIN", "BYPASS", fxId, _token, (void*)_value) > 0);
				updated |= updt;

				//JFB!!! close the GUI for buggy plugins
				// http://code.google.com/p/sws-extension/issues/detail?id=317
				if (updt && g_buggyPlugSupport && _token == 2)
				{
					p.ParsePatch(SNM_SETALL_CHUNK_CHAR_EXCEPT,2,"FXCHAIN","FLOAT",5,0xFFFF,0,(void*)"FLOATPOS"); // unfloat all

					char pIdx[4] = "";
					int pos = p.Parse(SNM_GET_CHUNK_CHAR,2,"FXCHAIN","SHOW",0,1,(void*)pIdx);
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
	if (updated && _undoMsg)
		Undo_OnStateChangeEx(_undoMsg, UNDO_STATE_ALL, -1); // using UNDO_STATE_FX crashes 
	return updated;
}

void toggleFXOfflineSelectedTracks(COMMAND_T* _ct) { 
	if (patchSelTracksFXState(SNM_TOGGLE_CHUNK_INT, 2, (int)_ct->user, NULL, SNM_CMD_SHORTNAME(_ct)) && 
		SNM_CountSelectedTracks(NULL, true) > 1)
	{
		FakeToggle(_ct);
	}
} 

bool isFXOfflineSelectedTracks(COMMAND_T * _ct) {
	return isFXOfflineOrBypassedSelectedTracks(_ct, 2);
}

void toggleFXBypassSelectedTracks(COMMAND_T* _ct) { 
	if (patchSelTracksFXState(SNM_TOGGLE_CHUNK_INT, 1, (int)_ct->user, NULL, SNM_CMD_SHORTNAME(_ct)) &&
		SNM_CountSelectedTracks(NULL, true) > 1)
	{
		FakeToggle(_ct);
	}
} 

bool isFXBypassedSelectedTracks(COMMAND_T * _ct) {
	return isFXOfflineOrBypassedSelectedTracks(_ct, 1);
}

void toggleExceptFXOfflineSelectedTracks(COMMAND_T* _ct) { 
	if (patchSelTracksFXState(SNM_TOGGLE_CHUNK_INT_EXCEPT, 2, (int)_ct->user, NULL, SNM_CMD_SHORTNAME(_ct)))
		FakeToggle(_ct);
} 
  
void toggleExceptFXBypassSelectedTracks(COMMAND_T* _ct) { 
	if (patchSelTracksFXState(SNM_TOGGLE_CHUNK_INT_EXCEPT, 1, (int)_ct->user, NULL, SNM_CMD_SHORTNAME(_ct)))
		FakeToggle(_ct);
} 
  
void toggleAllFXsOfflineSelectedTracks(COMMAND_T* _ct) { 
	// We use the "except mode" but with an unreachable fx number 
	if (patchSelTracksFXState(SNM_TOGGLE_CHUNK_INT_EXCEPT, 2, 0xFFFF, NULL, SNM_CMD_SHORTNAME(_ct)))
		FakeToggle(_ct);
} 
  
void toggleAllFXsBypassSelectedTracks(COMMAND_T* _ct) { 
	// We use the "except mode" but with an unreachable fx number 
	if (patchSelTracksFXState(SNM_TOGGLE_CHUNK_INT_EXCEPT, 1, 0xFFFF, NULL, SNM_CMD_SHORTNAME(_ct)))
		FakeToggle(_ct);
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
	_snprintf(cInt, 2, "%d", (int)_ct->user);
	// we use the "except mode" but with an unreachable fx number 
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
				bool updt = (p.ParsePatch(_mode, 2, "TAKEFX", "BYPASS", _fxId, _token, (void*)_value) > 0);
				updated |= updt;

/*JFB not used: doesn't seem to occur with take FX
				// close the GUI for buggy plugins
				// http://code.google.com/p/sws-extension/issues/detail?id=317
				// API LIMITATION: cannot restore shown FX here (contrary to track FX)
				if (updt && g_buggyPlugSupport && _token == 2)
				{
					p.ParsePatch(SNM_SETALL_CHUNK_CHAR_EXCEPT,2,"TAKEFX","FLOAT",5,255,0,(void*)"FLOATPOS"); //unfloat all
					p.ParsePatch(SNM_SET_CHUNK_CHAR, 2, "TAKEFX", "SHOW",2,0,1,(void*)"0"); // no FX shown..
				}
*/
			}
		}
	}

	if (updated && _undoMsg)
		Undo_OnStateChangeEx(_undoMsg, UNDO_STATE_ALL, -1);
	return updated;
}

void toggleAllFXsOfflineSelectedItems(COMMAND_T* _ct) { 
	// "except mode" but with an unreachable fx number 
	if (patchSelItemsFXState(SNM_TOGGLE_CHUNK_INT_EXCEPT, 2, 0xFFFF, NULL, SNM_CMD_SHORTNAME(_ct)))
		FakeToggle(_ct);
} 
  
void toggleAllFXsBypassSelectedItems(COMMAND_T* _ct) { 
	// "except mode" but with an unreachable fx number 
	if (patchSelItemsFXState(SNM_TOGGLE_CHUNK_INT_EXCEPT, 1, 0xFFFF, NULL, SNM_CMD_SHORTNAME(_ct)))
		FakeToggle(_ct);
} 

void setAllFXsOfflineSelectedItems(COMMAND_T* _ct) {
	char cInt[2] = "";
	_snprintf(cInt, 2, "%d", (int)_ct->user);
	// "except mode" but with an unreachable fx number 
	patchSelItemsFXState(SNM_SETALL_CHUNK_CHAR_EXCEPT, 2, 0xFFFF, cInt, SNM_CMD_SHORTNAME(_ct));
}

void setAllFXsBypassSelectedItems(COMMAND_T* _ct) {
	char cInt[2] = "";
	_snprintf(cInt, 2, "%d", (int)_ct->user);
	// "except mode" but with an unreachable fx number 
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
		_snprintf(pLastSel, 4, "%d", _fx);
		char pShow[4] = ""; // 4 if there're many FXs
		if (p.Parse(SNM_GET_CHUNK_CHAR,2,"FXCHAIN","SHOW",0,1,&pShow) > 0)
		{
			// patch the shown FX if the fx chain dlg is opened
			if (strcmp(pShow, "0")) {
				_snprintf(pShow, 4, "%d", _fx+1);
				updates = p.ParsePatch(SNM_SET_CHUNK_CHAR,2,"FXCHAIN","SHOW",0,1,&pShow);
			}
			updates = p.ParsePatch(SNM_SET_CHUNK_CHAR,2,"FXCHAIN","LASTSEL",0,1,&pLastSel);
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
	if (updated)
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}

int getSelectedTrackFX(MediaTrack* _tr)
{
	if (_tr)
	{
		// avoids useless parsing (1st try)
		if (TrackFX_GetCount(_tr) == 1)
			return 0;

		// avoids useless parsing (2nd try)
		int currentFX = TrackFX_GetChainVisible(_tr);
		if (currentFX >= 0)
			return currentFX;

		// the 2 attempts above failed => no choice: parse to get the selected FX
		SNM_ChunkParserPatcher p(_tr);
		p.SetWantsMinimalState(true);
		char pLastSel[4] = ""; // 4: if there're many, many FXs
		p.Parse(SNM_GET_CHUNK_CHAR,2,"FXCHAIN","LASTSEL",0,1,&pLastSel);
		return atoi(pLastSel); // return 0 (first FX) if failed
	}
	return -1;
}


///////////////////////////////////////////////////////////////////////////////
// Track FX presets
///////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32 //JFB OSX: todo/cannot test

// _fxType: as defined in FX chunk ("VST", "AU", etc..)
// _fxName: as defined in FX chunk ("foo.dll", etc..)
int GetUserPresetNames(const char* _fxType, const char* _fxName, WDL_PtrList<WDL_FastString>* _presetNames)
{
	int nbPresets = 0;
	if (_fxType && _fxName && _presetNames)
	{
		char iniFn[BUFFER_SIZE]="", buf[256]="";

		// *** get ini filename ***
		lstrcpyn(buf, _fxName, 256);

		// remove ".dll"
		if (!_stricmp(_fxType, "VST"))
			if (const char* p = stristr(buf,".dll"))
				buf[(int)(p-buf)] = '\0';

		// replace special chars
		int i=0;
		while (buf[i]) {
			if (buf[i] == '.' || buf[i] == '/') buf[i] = '_';
			i++;
		}

		char* fxType = _strdup(_fxType);
		for (i = 0; i < (int)strlen(fxType); i++)
			fxType[i] = tolower(fxType[i]);
		_snprintf(iniFn, BUFFER_SIZE, "%s%cpresets%c%s-%s.ini", GetResourcePath(), PATH_SLASH_CHAR, PATH_SLASH_CHAR, fxType, buf);

		bool exitTst = FileExists(iniFn);
		if (!exitTst)
			_snprintf(iniFn, BUFFER_SIZE, "%s%cpresets-%s-%s.ini", GetResourcePath(), PATH_SLASH_CHAR, fxType, buf);
		free(fxType);

		// *** get presets ***
		if (exitTst || (!exitTst && FileExists(iniFn)))
		{
			GetPrivateProfileString("General", "NbPresets", "0", buf, 5, iniFn);
			nbPresets = atoi(buf);
			char sec[32];
			for (int i=0; i < nbPresets; i++)
			{
				_snprintf(sec, 32, "Preset%d", i);
				GetPrivateProfileString(sec, "Name", "", buf, 256, iniFn);
				_presetNames->Add(new WDL_FastString(buf));
			}
		}
	}
	return nbPresets;
}
#endif

///////////////////////////////////////////////////////////////////////////////

// _presetId: only taken into account when _dir == 0, see below
// _dir: +1 or -1 switches to next or previous preset (_presetId is ignored)
// returns true if preset changed
bool TriggerFXPreset(MediaTrack* _tr, int _fxId, int _presetId, int _dir)
{
	// dedicated API since v4.16pre
	if (TrackFX_NavigatePresets && TrackFX_SetPreset)
	{
		int nbFx = _tr ? TrackFX_GetCount(_tr) : 0;
		if (nbFx > 0 && _fxId < nbFx)
		{
			if (_dir)
				return TrackFX_NavigatePresets(_tr, _fxId, _dir);
			else if (_presetId) {
				TrackFX_SetPreset(_tr, _fxId, "");
				return TrackFX_NavigatePresets(_tr, _fxId, _presetId);
			}
		}
	}
	return false;
}

void TriggerFXPresetSelTracks(int _fxId, int _presetId, int _dir)
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
				TriggerFXPreset(tr, fxId, _presetId, _dir);
		}
	}
}

void NextPresetSelTracks(COMMAND_T* _ct) {
	TriggerFXPresetSelTracks((int)_ct->user, 0, 1);
}

void PrevPresetSelTracks(COMMAND_T* _ct) {
	TriggerFXPresetSelTracks((int)_ct->user, 0, -1);
}

void NextPrevPresetLastTouchedFX(COMMAND_T* _ct) {
	if (GetLastTouchedFX) { // dedicated API since v4.16pre
		int trId, fxId;
		if (GetLastTouchedFX(&trId, &fxId, NULL))
			if (MediaTrack* tr = CSurf_TrackFromID(trId, false))
				TriggerFXPreset(tr, fxId, 0, (int)_ct->user);
	}
}


///////////////////////////////////////////////////////////////////////////////

// trigger preset through MIDI CC action
class SNM_TriggerPresetScheduledJob : public SNM_ScheduledJob {
public:
	SNM_TriggerPresetScheduledJob(int _approxDelayMs, int _val, int _valhw, int _relmode, HWND _hwnd, int _fxId) 
		: SNM_ScheduledJob(SNM_SCHEDJOB_TRIG_PRESET, _approxDelayMs),m_val(_val),m_valhw(_valhw),m_relmode(_relmode),m_hwnd(_hwnd),m_fxId(_fxId) {}
	void Perform() {TriggerFXPresetSelTracks(m_fxId, m_val, 0);}
protected:
	int m_val, m_valhw, m_relmode, m_fxId;
	HWND m_hwnd;
};

// absolute CC only
void TriggerFXPreset(MIDI_COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd) 
{
	if (!_relmode && _valhw < 0 && g_bv4) {
		SNM_TriggerPresetScheduledJob* job =
			new SNM_TriggerPresetScheduledJob(SNM_SCHEDJOB_DEFAULT_DELAY, _val, _valhw, _relmode, _hwnd, (int)_ct->user);
		AddOrReplaceScheduledJob(job);
	}
}


///////////////////////////////////////////////////////////////////////////////
// Other
///////////////////////////////////////////////////////////////////////////////

// http://code.google.com/p/sws-extension/issues/detail?id=258
// brutal code (we'd need a dedicated parser/patcher here..)
void moveFX(COMMAND_T* _ct)
{
	bool updated = false;
	int dir = (int)_ct->user;
	for (int i=0; i <= GetNumTracks(); i++) // include master
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
					WDL_FastString chainChunk;
					if (p.GetSubChunk("FXCHAIN", 2, 0, &chainChunk, "<ITEM") > 0)
					{
						int originalChainLen = chainChunk.GetLength();
						SNM_ChunkParserPatcher pfxc(&chainChunk, false);
						int p1 = pfxc.Parse(SNM_GET_CHUNK_CHAR,1,"FXCHAIN","BYPASS",3,sel,0);
						if (p1>0)
						{
							p1--;

							// locate end of fx
							int p2 = pfxc.Parse(SNM_GET_CHUNK_CHAR,1,"FXCHAIN","BYPASS",3,sel+1,0);
							if (p2 > 0)	p2--;
							else p2 = pfxc.GetChunk()->GetLength()-2; // -2 for ">\n"

							// store & cut fx
							WDL_FastString fxChunk;
							fxChunk.Set((const char*)(pfxc.GetChunk()->Get()+p1), p2-p1);
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
	if (updated)
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}
