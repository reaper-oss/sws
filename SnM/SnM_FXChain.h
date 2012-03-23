/******************************************************************************
/ SnM_FXChain.h
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

#ifndef _SNM_FXCHAIN_H_
#define _SNM_FXCHAIN_H_

void makeChunkTakeFX(WDL_FastString* _outTakeFX, const WDL_FastString* _inRfxChain);
int copyTakeFXChain(WDL_FastString* _fxChain, int _startSelItem=0);
void pasteTakeFXChain(const char* _title, WDL_FastString* _chain, bool _activeOnly);
void setTakeFXChain(const char* _title, WDL_FastString* _chain, bool _activeOnly);
void applyTakesFXChainSlot(int _slotType, const char* _title, int _slot, bool _activeOnly, bool _set);
bool autoSaveItemFXChainSlots(int _slotType, const char* _dirPath, char* _fn, int _fnSize, bool _nameFromFx);
void loadSetTakeFXChain(COMMAND_T*);
void loadPasteTakeFXChain(COMMAND_T*);
void loadSetAllTakesFXChain(COMMAND_T*);
void loadPasteAllTakesFXChain(COMMAND_T*);
void copyTakeFXChain(COMMAND_T*);
void cutTakeFXChain(COMMAND_T*);
void pasteTakeFXChain(COMMAND_T*);
void setTakeFXChain(COMMAND_T*);
void pasteAllTakesFXChain(COMMAND_T*);
void setAllTakesFXChain(COMMAND_T*);
void clearActiveTakeFXChain(COMMAND_T*);
void clearAllTakesFXChain(COMMAND_T*);
void pasteTrackFXChain(const char* _title, WDL_FastString* _chain, bool _inputFX);
void setTrackFXChain(const char* _title, WDL_FastString* _chain, bool _inputFX);
int copyTrackFXChain(WDL_FastString* _fxChain, bool _inputFX, int _startTr=0);
void applyTracksFXChainSlot(int _slotType, const char* _title, int _slot, bool _set, bool _inputFX);
bool autoSaveTrackFXChainSlots(int _slotType, const char* _dirPath, char* _fn, int _fnSize, bool _nameFromFx, bool _inputFX);
void loadSetTrackFXChain(COMMAND_T*);
void loadPasteTrackFXChain(COMMAND_T*);
void loadSetTrackInFXChain(COMMAND_T*);
void loadPasteTrackInFXChain(COMMAND_T*);
void clearTrackFXChain(COMMAND_T*);
void copyTrackFXChain(COMMAND_T*);
void cutTrackFXChain(COMMAND_T*);
void pasteTrackFXChain(COMMAND_T*);
void setTrackFXChain(COMMAND_T*);
void clearTrackInputFXChain(COMMAND_T*);
void copyTrackInputFXChain(COMMAND_T*);
void cutTrackInputFXChain(COMMAND_T*);
void pasteTrackInputFXChain(COMMAND_T*);
void setTrackInputFXChain(COMMAND_T*);
void copyFXChainSlotToClipBoard(int _slot);
void smartCopyFXChain(COMMAND_T*);
void smartPasteFXChain(COMMAND_T*);
void smartPasteReplaceFXChain(COMMAND_T*);
void smartCutFXChain(COMMAND_T*);
void reassignLearntMIDICh(COMMAND_T*);

#endif
