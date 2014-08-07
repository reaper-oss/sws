/******************************************************************************
/ BR_Misc.h
/
/ Copyright (c) 2013-2014 Dominik Martin Drzic
/ http://forum.cockos.com/member.php?u=27094
/ https://code.google.com/p/sws-extension
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

/******************************************************************************
* Commands: Misc                                                              *
******************************************************************************/
void SplitItemAtTempo (COMMAND_T*);
void MarkersAtTempo (COMMAND_T*);
void MidiItemTempo (COMMAND_T*);
void MidiItemTrim (COMMAND_T*);
void MarkersAtNotes (COMMAND_T*);
void MarkersAtStretchMarkers (COMMAND_T*);
void MarkerAtMouse (COMMAND_T*);
void MarkersRegionsAtItems (COMMAND_T*);
void SnapFollowsGridVis (COMMAND_T*);
void TrimNewVolPanEnvs (COMMAND_T*);
void PlaybackFollowsTempoChange (COMMAND_T*);
void CycleRecordModes (COMMAND_T*);
void FocusArrange (COMMAND_T*);
void ToggleItemOnline (COMMAND_T*);
void ItemSourcePathToClipBoard (COMMAND_T*);
void PlaybackAtMouseCursor (COMMAND_T*);
void SelectItemsByType (COMMAND_T*);
void SaveCursorPosSlot (COMMAND_T*);
void RestoreCursorPosSlot (COMMAND_T*);

/******************************************************************************
* Commands: Misc - Media item preview                                         *
******************************************************************************/
void PreviewItemAtMouse (COMMAND_T*);

/******************************************************************************
* Toggle states: Misc                                                         *
******************************************************************************/
int IsSnapFollowsGridVisOn (COMMAND_T*);
int IsTrimNewVolPanEnvsOn (COMMAND_T*);
int IsPlaybackFollowingTempoChange (COMMAND_T*);