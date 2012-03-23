/******************************************************************************
/ SnM_Item.h
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

#pragma once

#ifndef _SNM_ITEM_H_
#define _SNM_ITEM_H_

char* GetName(MediaItem* _item);
int getTakeIndex(MediaItem* _item, MediaItem_Take* _take);
bool deleteMediaItemIfNeeded(MediaItem* _item);
void SNM_GetSelectedItems(ReaProject* _proj, WDL_PtrList<MediaItem>* _items, bool _onSelTracks = false);
void SNM_SetSelectedItems(ReaProject* _proj, WDL_PtrList<MediaItem>* _items, bool _onSelTracks = false);
void SNM_ClearSelectedItems(ReaProject* _proj, bool _onSelTracks = false);
bool ItemsInInterval(double _pos1, double _pos2);
void splitMidiAudio(COMMAND_T*);
void smartSplitMidiAudio(COMMAND_T*);
#ifdef _SNM_MISC // deprecated (v3.67)
void splitSelectedItems(COMMAND_T*);
#endif
void goferSplitSelectedItems(COMMAND_T*);
bool SplitSelectAllItemsInRegion(const char* _undoTitle, int _rgnIdx);
void SplitSelectAllItemsInRegion(COMMAND_T*);
void copyCutTake(COMMAND_T*);
void pasteTake(COMMAND_T*);
bool isEmptyMidi(MediaItem_Take* _take);
void setEmptyTakeChunk(WDL_FastString* _chunk, int _recPass = -1, int _color = -1, bool _v4style = true);
int buildLanes(const char* _undoTitle, int _mode);
bool removeEmptyTakes(MediaTrack* _tr, bool _empty, bool _midiEmpty, bool _trSel, bool _itemSel);
bool removeEmptyTakes(const char* _undoTitle, bool _empty, bool _midiEmpty, bool _trSel = false, bool _itemSel = true);
void clearTake(COMMAND_T*);
#ifdef _SNM_MISC // deprecated (v3.67)
void moveTakes(COMMAND_T*);
#endif
void moveActiveTake(COMMAND_T*);
void activateLaneFromSelItem(COMMAND_T*);
void activateLaneUnderMouse(COMMAND_T*);
void buildLanes(COMMAND_T*);
void removeEmptyTakes(COMMAND_T*);
void removeEmptyMidiTakes(COMMAND_T*);
void removeAllEmptyTakes(COMMAND_T*);
void deleteTakeAndMedia(COMMAND_T*);
int getPitchTakeEnvRangeFromPrefs();
void panTakeEnvelope(COMMAND_T*);
void showHideTakeVolEnvelope(COMMAND_T*); 
void showHideTakePanEnvelope(COMMAND_T*);
void showHideTakeMuteEnvelope(COMMAND_T*);
void showHideTakePitchEnvelope(COMMAND_T*);
bool ShowTakeEnvVol(MediaItem_Take* _take);
bool ShowTakeEnvPan(MediaItem_Take* _take);
bool ShowTakeEnvMute(MediaItem_Take* _take);
bool ShowTakeEnvPitch(MediaItem_Take* _take);
#ifdef _SNM_MISC
void saveItemTakeTemplate(COMMAND_T*);
#endif
void itemSelToolbarPoll();
void toggleItemSelExists(COMMAND_T*);
bool itemSelExists(COMMAND_T*);
void scrollToSelItem(MediaItem* _item);
void scrollToSelItem(COMMAND_T*);
void setPan(COMMAND_T*);
void PlaySelTrackMediaSlot(COMMAND_T*);
void LoopSelTrackMediaSlot(COMMAND_T*);
void SyncPlaySelTrackMediaSlot(COMMAND_T*);
void SyncLoopSelTrackMediaSlot(COMMAND_T*);
bool TogglePlaySelTrackMediaSlot(int _slotType, const char* _title, int _slot, bool _pause, bool _loop, double _msi = -1.0);
void TogglePlaySelTrackMediaSlot(COMMAND_T*);
void ToggleLoopSelTrackMediaSlot(COMMAND_T*);
void TogglePauseSelTrackMediaSlot(COMMAND_T*);
void ToggleLoopPauseSelTrackMediaSlot(COMMAND_T*);
#ifdef _SNM_MISC
void SyncTogglePlaySelTrackMediaSlot(COMMAND_T*);
void SyncToggleLoopSelTrackMediaSlot(COMMAND_T*);
void SyncTogglePauseSelTrackMediaSlot(COMMAND_T*);
void SyncToggleLoopPauseSelTrackMediaSlot(COMMAND_T*);
#endif
void InsertMediaSlot(int _slotType, const char* _title, int _slot, int _insertMode);
void InsertMediaSlotCurTr(COMMAND_T*);
void InsertMediaSlotNewTr(COMMAND_T*);
void InsertMediaSlotTakes(COMMAND_T*);
bool autoSaveMediaSlot(int _slotType, const char* _dirPath, char* _fn, int _fnSize);

#endif
