/******************************************************************************
/ SnM_FXChain.h
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

#ifndef _SNM_FXCHAIN_H_
#define _SNM_FXCHAIN_H_


void MakeChunkTakeFX(WDL_FastString* _outTakeFX, const WDL_FastString* _inRfxChain);
int CopyTakeFXChain(WDL_FastString* _fxChain, int _startSelItem=0);
void PasteTakeFXChain(const char* _title, WDL_FastString* _chain, bool _activeOnly);
void SetTakeFXChain(const char* _title, WDL_FastString* _chain, bool _activeOnly);

void ClearTrackFXChain(COMMAND_T*);
void CopyTrackFXChain(COMMAND_T*);
void CutTrackFXChain(COMMAND_T*);
void PasteTrackFXChain(COMMAND_T*);
void SetTrackFXChain(COMMAND_T*);

void ClearTrackInputFXChain(COMMAND_T*);
void CopyTrackInputFXChain(COMMAND_T*);
void CutTrackInputFXChain(COMMAND_T*);
void PasteTrackInputFXChain(COMMAND_T*);
void SetTrackInputFXChain(COMMAND_T*);

void CopyTakeFXChain(COMMAND_T*);
void CutTakeFXChain(COMMAND_T*);
void PasteTakeFXChain(COMMAND_T*);
void SetTakeFXChain(COMMAND_T*);
void PasteAllTakesFXChain(COMMAND_T*);
void SetAllTakesFXChain(COMMAND_T*);
void ClearActiveTakeFXChain(COMMAND_T*);
void ClearAllTakesFXChain(COMMAND_T*);

void SmartCopyFXChain(COMMAND_T*);
void SmartPasteFXChain(COMMAND_T*);
void SmartPasteReplaceFXChain(COMMAND_T*);
void SmartCutFXChain(COMMAND_T*);

void PasteTrackFXChain(const char* _title, WDL_FastString* _chain, bool _inputFX);
void SetTrackFXChain(const char* _title, WDL_FastString* _chain, bool _inputFX);
int CopyTrackFXChain(WDL_FastString* _fxChain, bool _inputFX, int _startTr=0);

void ReassignLearntMIDICh(COMMAND_T*);

#endif
