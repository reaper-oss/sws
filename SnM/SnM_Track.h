/******************************************************************************
/ SnM_Track.h
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

#ifndef _SNM_TRACK_H_
#define _SNM_TRACK_H_


void CopyCutTrackGrouping(COMMAND_T*);
void PasteTrackGrouping(COMMAND_T*);
void RemoveTrackGrouping(COMMAND_T*);
void SetTrackGroup(COMMAND_T*);
void SetTrackToFirstUnusedGroup(COMMAND_T*);

void SaveTracksFolderStates(COMMAND_T*);
void RestoreTracksFolderStates(COMMAND_T*);
void SetTracksFolderState(COMMAND_T*);

void SelOnlyTrackWithSelEnv(COMMAND_T*);

bool LookupTrackEnvName(const char* _str, bool _allEnvs);
void ToggleArmTrackEnv(COMMAND_T*);
void RemoveAllEnvsSelTracks(COMMAND_T*);
void RemoveAllEnvsSelTracksNoChunk(COMMAND_T*);
void ToggleWriteEnvExists(COMMAND_T*);
int WriteEnvExists(COMMAND_T*);

MediaTrack* SNM_GetTrack(ReaProject* _proj, int _idx);
int SNM_GetTrackId(ReaProject* _proj, MediaTrack* _tr);
int SNM_CountSelectedTracks(ReaProject* _proj, bool _master);
MediaTrack* SNM_GetSelectedTrack(ReaProject* _proj, int _idx, bool _withMaster);
void SNM_GetSelectedTracks(ReaProject* _proj, WDL_PtrList<MediaTrack>* _trs, bool _withMaster);
bool SNM_SetSelectedTracks(ReaProject* _proj, WDL_PtrList<MediaTrack>* _trs, bool _unselOthers, bool _withMaster = true);
bool SNM_SetSelectedTrack(ReaProject* _proj, MediaTrack* _tr, bool _unselOthers, bool _withMaster = true);
void SNM_ClearSelectedTracks(ReaProject* _proj, bool _withMaster);

bool MakeSingleTrackTemplateChunk(WDL_FastString* _in, WDL_FastString* _out, bool _delItems, bool _delEnvs, int _tmpltIdx = 0, bool _obeyOffset = true);
bool GetItemsSubChunk(WDL_FastString* _in, WDL_FastString* _out, int _tmpltIdx = 0);
bool ReplacePasteItemsFromTrackTemplate(MediaTrack* _tr, WDL_FastString* _tmpltItems, bool _paste, SNM_ChunkParserPatcher* _p = NULL);
bool ApplyTrackTemplate(MediaTrack* _tr, WDL_FastString* _tmplt, bool _itemsFromTmplt, bool _envsFromTmplt, void* _p = NULL);
void SaveSelTrackTemplates(bool _delItems, bool _delEnvs, WDL_FastString* _chunkOut);


void SetMIDIInputChannel(COMMAND_T*);
void RemapMIDIInputChannel(COMMAND_T*);

void StopTrackPreviewsRun();
bool SNM_PlayTrackPreview(MediaTrack* _tr, PCM_source* _src, bool _pause, bool _loop, double _msi);
bool SNM_PlayTrackPreview(MediaTrack* _tr, const char* _fn, bool _pause, bool _loop, double _msi);
void SNM_PlaySelTrackPreviews(const char* _fn, bool _pause, bool _loop, double _msi);
bool SNM_TogglePlaySelTrackPreviews(const char* _fn, bool _pause, bool _loop, double _msi = -1.0);
void StopTrackPreviews(bool _selTracksOnly);
void StopTrackPreviews(COMMAND_T*);

bool SendAllNotesOff(WDL_PtrList<void>* _trs, int _cc_flags = 1|2);
bool SendAllNotesOff(MediaTrack* _tr, int _cc_flags = 1|2);
void SendAllNotesOff(COMMAND_T*);

bool SNM_AddTCPFXParm(MediaTrack* _tr, int _fxId, int _prmId);

void ScrollSelTrack(bool _tcp, bool _mcp);
void ScrollSelTrack(COMMAND_T*);
void ScrollTrack(MediaTrack* _tr, bool _tcp, bool _mcp);
void ShowTrackRoutingWindow(MediaTrack* _tr);

#endif
