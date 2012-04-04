/******************************************************************************
/ SnM_FX.h
/
/ Copyright (c) 2012 Jeffos
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

//#pragma once

#ifndef _SNM_FX_H_
#define _SNM_FX_H_

extern int g_buggyPlugSupport;

int GetFXByGUID(MediaTrack* _tr, GUID* _g);
void toggleFXOfflineSelectedTracks(COMMAND_T*);
bool isFXOfflineSelectedTracks(COMMAND_T*);
void toggleFXBypassSelectedTracks(COMMAND_T*);
bool isFXBypassedSelectedTracks(COMMAND_T*);
void toggleExceptFXOfflineSelectedTracks(COMMAND_T*);
void toggleExceptFXBypassSelectedTracks(COMMAND_T*);
void toggleAllFXsOfflineSelectedTracks(COMMAND_T*);
void toggleAllFXsBypassSelectedTracks(COMMAND_T*);
void setFXOfflineSelectedTracks(COMMAND_T*); 
void setFXBypassSelectedTracks(COMMAND_T*);
void setFXOnlineSelectedTracks(COMMAND_T*);
void setFXUnbypassSelectedTracks(COMMAND_T*);
void setAllFXsBypassSelectedTracks(COMMAND_T*);
void toggleAllFXsOfflineSelectedItems(COMMAND_T*);
void toggleAllFXsBypassSelectedItems(COMMAND_T*);
void setAllFXsOfflineSelectedItems(COMMAND_T*);
void setAllFXsBypassSelectedItems(COMMAND_T*);
void selectTrackFX(COMMAND_T*);
int getSelectedTrackFX(MediaTrack* _tr);
#ifdef _WIN32
int GetUserPresetNames(const char* _fxType, const char* _fxName, WDL_PtrList<WDL_FastString>* _presetNames);
#endif
bool TriggerFXPreset(MediaTrack* _tr, int _fxId, int _presetId, int _dir);
void NextPresetSelTracks(COMMAND_T*);
void PrevPresetSelTracks(COMMAND_T*);
void NextPrevPresetLastTouchedFX(COMMAND_T*);
void TriggerFXPreset(MIDI_COMMAND_T* _ct, int _val, int _valhw, int _relmode, HWND _hwnd);
void moveFX(COMMAND_T*);

#endif