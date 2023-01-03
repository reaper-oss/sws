/******************************************************************************
/ Adam.h
/
/ Copyright (c) 2010 Adam Wathan
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

#pragma once

int AdamInit();
void AWFillGapsAdv(const char* title, char* retVals);
void UpdateGridToolbar();
void UpdateTrackTimebaseToolbar();
void UpdateItemTimebaseToolbar();
void UpdateTimebaseToolbar();
void AWDoAutoGroup(bool rec);

// for SNM_RefreshToolbars
int IsGridSwing(COMMAND_T*);
int IsProjectTimebase(COMMAND_T*);
int IsSelTracksTimebase(COMMAND_T*);
int IsSelItemsTimebase(COMMAND_T*);

// #587
void NFDoAutoGroupTakesMode(WDL_TypedBuf<MediaItem*> selItems);

// Auto group in takes mode helper functions
MediaItem* GetSelectedItemOnTrack_byIndex(WDL_TypedBuf<MediaItem*> origSelItems,  vector<int> selItemsPerTrackCount, 
	int selTrackIdx, int column);
void SelectRecArmedTracksOfSelItems(WDL_TypedBuf<MediaItem*> selItems);
int GetMaxSelItemsPerTrackCount(vector<int> selItemsPerTrackCount);
void UnselectAllTracks();  void UnselectAllItems();
void RestoreOrigTracksAndItemsSelection(WDL_TypedBuf<MediaTrack*> origSelTracks, WDL_TypedBuf<MediaItem*> origSelItems);
bool IsMoreThanOneTrackRecArmed();
