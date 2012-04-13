/******************************************************************************
/ SnM_Routing.h
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

//#pragma once

#ifndef _SNM_ROUTING_H_
#define _SNM_ROUTING_H_

#include "SnM_Chunk.h" 

bool cueTrack(const char* _undoMsg, const char* _busName, int _type, bool _showRouting = true, int _soloDefeat = 1, char* _trTemplatePath = NULL, bool _sendToMaster = false, int* _hwOuts = NULL);
bool cueTrack(const char* _undoMsg, int _confId);
void cueTrack(COMMAND_T*);
void readCueBusIniFile(int _confId, char* _busName, int* _reaType, bool* _trTemplate, char* _trTemplatePath, bool* _showRouting, int* _soloDefeat, bool* _sendToMaster, int* _hwOuts);
void saveCueBusIniFile(int _confId, const char* _busName, int _type, bool _trTemplate, const char* _trTemplatePath, bool _showRouting, int _soloDefeat, bool _sendToMaster, int* _hwOuts);
void copySendsReceives(bool _cut, WDL_PtrList<MediaTrack>* _trs, WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _sends,  WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _rcvs);
bool pasteSendsReceives(WDL_PtrList<MediaTrack>* _trs, WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _sends,  WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _rcvs, bool _rcvReset, WDL_PtrList<SNM_ChunkParserPatcher>* _ps);
void copyWithIOs(COMMAND_T*);
void cutWithIOs(COMMAND_T*);
void pasteWithIOs(COMMAND_T*);
void copyRoutings(COMMAND_T*);
void cutRoutings(COMMAND_T*);
void pasteRoutings(COMMAND_T*);
void copySends(COMMAND_T*);
void cutSends(COMMAND_T*);
void pasteSends(COMMAND_T*);
void copyReceives(COMMAND_T*);
void cutReceives(COMMAND_T*);
void pasteReceives(COMMAND_T*);
bool removeSnd(WDL_PtrList<MediaTrack>* _trs, WDL_PtrList<SNM_ChunkParserPatcher>* _ps);
void removeSends(COMMAND_T*);
bool removeRcv(WDL_PtrList<MediaTrack>* _trs, WDL_PtrList<SNM_ChunkParserPatcher>* _ps);
void removeReceives(COMMAND_T*);
void removeRouting(COMMAND_T*);
void muteReceives(MediaTrack* _source, MediaTrack* _dest, bool _mute);

#endif
