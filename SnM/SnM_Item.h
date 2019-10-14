/******************************************************************************
/ SnM_Item.h
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

#ifndef _SNM_ITEM_H_
#define _SNM_ITEM_H_


enum {
  SNM_ITEM_SEL_LEFT=0,
  SNM_ITEM_SEL_RIGHT,
  SNM_ITEM_SEL_UP,
  SNM_ITEM_SEL_DOWN,
  SNM_ITEM_SEL_COUNT
};

class SNM_ItemChunk {
public:
	SNM_ItemChunk(MediaItem* _item) : m_item(_item) {
		if (_item) {
			SNM_ChunkParserPatcher p(_item);
			m_chunk.Set(p.GetChunk());
		}
	}
	~SNM_ItemChunk() {}
	MediaItem* m_item;
	WDL_FastString m_chunk;
};

char* GetName(MediaItem* _item);
int GetTakeIndex(MediaItem* _item, MediaItem_Take* _take);
bool DeleteMediaItemIfNeeded(MediaItem* _item);
bool DeleteMediaItemsByName(const char* tkname);
void SNM_GetSelectedItems(ReaProject* _proj, WDL_PtrList<MediaItem>* _items, bool _onSelTracks = false);
bool SNM_SetSelectedItems(ReaProject* _proj, WDL_PtrList<MediaItem>* _items, bool _onSelTracks = false);
bool SNM_ClearSelectedItems(ReaProject* _proj, bool _onSelTracks = false);
bool IsItemInInterval(MediaItem* _item, double _pos1, double _pos2, bool _inclusive);
bool GetItemsInInterval(WDL_PtrList<void>* _items, double _pos1, double _pos2, bool _inclusive);
bool GenerateItemsInInterval(WDL_PtrList<void>* _items, double _pos1, double _pos2, const char* tkname=NULL);
void GetAllItemPointers(WDL_PtrList<void>* _items);
void DiffItemPointers(WDL_PtrList<void>* _oldItemsIn, WDL_PtrList<void>* _newItemsOut);
bool DupSelItems(const char* _undoTitle, double _nudgePos, WDL_PtrList<void>* _newItemsOut = NULL);

void SplitMidiAudio(COMMAND_T*);
void SmartSplitMidiAudio(COMMAND_T*);
#ifdef _SNM_MISC // deprecated (v3.67)
void SplitSelectedItems(COMMAND_T*);
#endif
void GoferSplitSelectedItems(COMMAND_T*);
bool SplitSelectItemsInInterval(MediaTrack* _tr, double _pos1, double _pos2, WDL_PtrList<void>* _newItemsOut = NULL);
bool SplitSelectItemsInInterval(const char* _undoTitle, double _pos1, double _pos2, bool _selTracks = false, WDL_PtrList<void>* _newItemsOut = NULL);
void SplitSelectAllItemsInRegion(COMMAND_T*);

void CopyCutTakes(COMMAND_T*);
void PasteTakes(COMMAND_T*);

bool IsEmptyMidi(MediaItem_Take* _take);
void SetEmptyTakeChunk(WDL_FastString* _chunk, int _recPass = -1, int _color = -1, bool _v4style = true);
bool RemoveEmptyTakes(MediaTrack* _tr, bool _empty, bool _midiEmpty, bool _trSel, bool _itemSel);
bool RemoveEmptyTakes(const char* _undoTitle, bool _empty, bool _midiEmpty, bool _trSel = false, bool _itemSel = true);
#ifdef _SNM_MISC
int BuildLanes(const char* _undoTitle, int _mode);
void MoveTakes(COMMAND_T*);
void BuildLanes(COMMAND_T*);
#endif
void ClearTake(COMMAND_T*);
void MoveActiveTake(COMMAND_T*);
void ActivateLaneFromSelItem(COMMAND_T*);
void ActivateLaneUnderMouse(COMMAND_T*);
void RemoveEmptyTakes(COMMAND_T*);
void RemoveEmptyMidiTakes(COMMAND_T*);
void RemoveAllEmptyTakes(COMMAND_T*);
void DeleteTakeAndMedia(COMMAND_T*);

int GetPitchTakeEnvRangeFromPrefs();
void PanTakeEnvelope(COMMAND_T*);
void BypassUnbypassShowHideTakeVolEnvelope(COMMAND_T*); 
void BypassUnbypassShowHideTakePanEnvelope(COMMAND_T*);
void BypassUnbypassShowHideTakeMuteEnvelope(COMMAND_T*);
void BypassUnbypassShowHideTakePitchEnvelope(COMMAND_T*);

void ShowHideTakeVolEnvelope(COMMAND_T*);
void ShowHideTakePanEnvelope(COMMAND_T*);
void ShowHideTakeMuteEnvelope(COMMAND_T*);
void ShowHideTakePitchEnvelope(COMMAND_T*);

bool ShowTakeEnvVol(MediaItem_Take* _take);
bool ShowTakeEnvPan(MediaItem_Take* _take);
bool ShowTakeEnvMute(MediaItem_Take* _take);
bool ShowTakeEnvPitch(MediaItem_Take* _take);

void RefreshOffscreenItems();
void ToggleOffscreenSelItems(COMMAND_T*);
int HasOffscreenSelItems(COMMAND_T*);
void UnselectOffscreenItems(COMMAND_T*);

void ScrollToSelItem(MediaItem* _item);
void ScrollToSelItem(COMMAND_T*);
void SetPan(COMMAND_T*);
void OpenMediaPathInExplorerFinder(COMMAND_T*);


#endif
