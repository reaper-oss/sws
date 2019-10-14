/******************************************************************************
/ BR_Tempo.h
/
/ Copyright (c) 2013-2015 Dominik Martin Drzic
/ http://forum.cockos.com/member.php?u=27094
/ http://github.com/reaper-oss/sws
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
* Commands: Tempo continuous actions                                          *
******************************************************************************/
void MoveGridToMouseInit ();

/******************************************************************************
* Commands: Tempo                                                             *
******************************************************************************/
void MoveGridToEditPlayCursor (COMMAND_T*);
void MoveTempo (COMMAND_T*);
void EditTempo (COMMAND_T*);
void EditTempoGradual (COMMAND_T*);
void DeleteTempo (COMMAND_T*);
void DeleteTempoPreserveItems (COMMAND_T*);
void TempoAtGrid (COMMAND_T*);
void SelectMovePartialTimeSig (COMMAND_T*);
void TempoShapeLinear (COMMAND_T*);
void TempoShapeSquare (COMMAND_T*);
void OpenTempoWiki (COMMAND_T*);

/******************************************************************************
* Dialogs                                                                     *
******************************************************************************/
void ConvertMarkersToTempoDialog (COMMAND_T*);
void SelectAdjustTempoDialog (COMMAND_T*);
void RandomizeTempoDialog (COMMAND_T*);
void TempoShapeOptionsDialog (COMMAND_T*);
int IsConvertMarkersToTempoVisible (COMMAND_T*);
int IsSelectAdjustTempoVisible (COMMAND_T*);
int IsTempoShapeOptionsVisible (COMMAND_T*);
