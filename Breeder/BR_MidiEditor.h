/******************************************************************************
/ BR_MidiEditor.h
/
/ Copyright (c) 2014-2015 Dominik Martin Drzic
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
* Commands: MIDI editor - Item preview                                        *
******************************************************************************/
void ME_StopMidiTakePreview (COMMAND_T*, int, int, int, HWND);
void ME_PreviewActiveTake (COMMAND_T*, int, int, int, HWND);

/******************************************************************************
* Commands: MIDI editor - Misc                                                *
******************************************************************************/
void ME_ToggleMousePlayback (COMMAND_T*, int, int, int, HWND);
void ME_PlaybackAtMouseCursor (COMMAND_T*, int, int, int, HWND);
void ME_CCEventAtEditCursor (COMMAND_T*, int, int, int, HWND);
void ME_ShowUsedCCLanesDetect14Bit (COMMAND_T*, int, int, int, HWND);
void ME_CreateCCLaneLastClicked (COMMAND_T*, int, int, int, HWND);
void ME_MoveCCLaneUpDown (COMMAND_T*, int, int, int, HWND);
void ME_MoveActiveWndToMouse (COMMAND_T*, int, int, int, HWND);
void ME_SetAllCCLanesHeight (COMMAND_T*, int, int, int, HWND);
void ME_IncDecAllCCLanesHeight (COMMAND_T*, int, int, int, HWND);
void ME_HideCCLanes (COMMAND_T*, int, int, int, HWND);
void ME_ToggleHideCCLanes (COMMAND_T*, int, int, int, HWND);
void ME_CCToEnvPoints (COMMAND_T*, int, int, int, HWND);
void ME_EnvPointsToCC (COMMAND_T*, int, int, int, HWND);
void ME_CopySelCCToLane (COMMAND_T*, int, int, int, HWND);
void ME_DeleteEventsLastClickedLane (COMMAND_T*, int, int, int, HWND);
void ME_SaveCursorPosSlot (COMMAND_T*, int, int, int, HWND);
void ME_RestoreCursorPosSlot (COMMAND_T*, int, int, int, HWND);
void ME_SaveNoteSelSlot (COMMAND_T*, int, int, int, HWND);
void ME_RestoreNoteSelSlot (COMMAND_T*, int, int, int, HWND);
void ME_SaveCCEventsSlot (COMMAND_T*, int, int, int, HWND);
void ME_RestoreCCEventsSlot (COMMAND_T*, int, int, int, HWND);
void ME_RestoreCCEvents2Slot (COMMAND_T*, int, int, int, HWND);

/******************************************************************************
* Toggle states: MIDI editor - Misc                                           *
******************************************************************************/
int ME_IsToggleHideCCLanesOn (COMMAND_T*);
