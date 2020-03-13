/******************************************************************************
/ SnM_FX.h
/
/ Copyright (c) 2012 and later Jeffos
/
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

//#pragma once

#ifndef _SNM_FX_H_
#define _SNM_FX_H_

#include "SnM.h"


extern int g_SNM_SupportBuggyPlug;

int GetFXByGUID(MediaTrack* _tr, GUID* _g);
int IsFXBypassedSelTracks(COMMAND_T*);
void ToggleExceptFXBypassSelTracks(COMMAND_T*);
void ToggleAllFXsBypassSelTracks(COMMAND_T*);
void ToggleFXBypassSelTracks(COMMAND_T*);
void BypassFXSelTracks(COMMAND_T*);
void UnbypassFXSelTracks(COMMAND_T*);
void UpdateAllFXsBypassSelTracks(COMMAND_T*);
void BypassAllFXsExceptSelTracks(COMMAND_T*);
void UnypassAllFXsExceptSelTracks(COMMAND_T*);

int IsFXOfflineSelTracks(COMMAND_T*);
void ToggleFXOfflineSelTracks(COMMAND_T*);
void ToggleExceptFXOfflineSelTracks(COMMAND_T*);
void ToggleAllFXsOfflineSelTracks(COMMAND_T*);
void SetFXOfflineSelTracks(COMMAND_T*); 
void SetFXOnlineSelTracks(COMMAND_T*);
void SetAllFXsOfflineExceptSelTracks(COMMAND_T* _ct);
void SetAllFXsOnlineExceptSelTracks(COMMAND_T* _ct);

void ToggleAllFXsOfflineSelItems(COMMAND_T*);
void UpdateAllFXsOfflineSelItems(COMMAND_T*);
void ToggleAllFXsBypassSelItems(COMMAND_T*);
void UpdateAllFXsBypassSelItems(COMMAND_T*);

void SelectTrackFX(COMMAND_T*);
bool SelectTrackFX(MediaTrack* _tr, int _fx);
int GetSelectedTrackFX(MediaTrack* _tr);
int GetSelectedTakeFX(MediaItem_Take* _take);

int GetUserPresetNames(MediaTrack* _tr, int _fx, WDL_PtrList<WDL_FastString>* _presetsOut);
int GetSetFXPresetSelTrack(int _fxId, int* _presetIdx, int* _numberOfPresetsOut = NULL);
bool TriggerFXPreset(MediaTrack* _tr, int _fxId, int _presetId, int _dir);
void NextPresetSelTracks(COMMAND_T*);
void PrevPresetSelTracks(COMMAND_T*);
void NextPrevPresetLastTouchedFX(COMMAND_T*);


class TriggerPresetJob : public MidiOscActionJob {
public:
	TriggerPresetJob(int _approxMs, int _val, int _valhw, int _relmode, int _fxId); 
protected:
	void Perform();
	double GetCurrentValue();
	double GetMinValue() { return 0.0; }
	double GetMaxValue();
	int m_fxId;
};

void TriggerFXPresetSelTrack(COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd);

bool SNM_MoveOrRemoveTrackFX(MediaTrack* _tr, int _fxId, int _what);
void MoveOrRemoveTrackFX(COMMAND_T*);

#endif
