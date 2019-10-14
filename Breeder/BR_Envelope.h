/******************************************************************************
/ BR_Envelope.h
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
* Commands: Envelope continuous actions                                       *
******************************************************************************/
void SetEnvPointMouseValueInit ();

/******************************************************************************
* Commands: Envelopes - Misc                                                  *
******************************************************************************/
void CursorToEnv1 (COMMAND_T*);
void CursorToEnv2 (COMMAND_T*);
void SelNextPrevEnvPoint (COMMAND_T*);
void ExpandEnvSel (COMMAND_T*);
void ExpandEnvSelEnd (COMMAND_T*);
void ShrinkEnvSel (COMMAND_T*);
void ShrinkEnvSelEnd (COMMAND_T*);
void EnvPointsGrid (COMMAND_T*);
void CreateEnvPointsGrid (COMMAND_T*);
void EnvPointsToCC (COMMAND_T*);
void ShiftEnvSelection (COMMAND_T*);
void PeaksDipsEnv (COMMAND_T*);
void SelEnvTimeSel (COMMAND_T*);
void SetEnvValToNextPrev (COMMAND_T*);
void MoveEnvPointToEditCursor (COMMAND_T*);
void Insert2EnvPointsTimeSelection (COMMAND_T*);
void CopyEnvPoints (COMMAND_T*);
void FitEnvPointsToTimeSel (COMMAND_T*);
void CreateEnvPointMouse (COMMAND_T*);
void IncreaseDecreaseVolEnvPoints (COMMAND_T*);
void SelectEnvelopeUnderMouse (COMMAND_T*);
void SelectDeleteEnvPointUnderMouse (COMMAND_T*);
void UnselectEnvelope (COMMAND_T*);
void ApplyNextCmdToMultiEnvelopes (COMMAND_T*);
void SaveEnvSelSlot (COMMAND_T*);
void RestoreEnvSelSlot (COMMAND_T*);

/******************************************************************************
* Commands: Envelopes - Visibility                                            *
******************************************************************************/
void ShowActiveTrackEnvOnly (COMMAND_T*);
void ShowLastAdjustedSendEnv (COMMAND_T*);
void ShowHideFxEnv (COMMAND_T*);
void ShowHideSendEnv (COMMAND_T*);
