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

bool CueBuss(const char* _undoMsg, const char* _busName, int _type, bool _showRouting = true, int _soloDefeat = 1, char* _trTemplatePath = NULL, bool _sendToMaster = false, int* _hwOuts = NULL);
bool CueBuss(const char* _undoMsg, int _confId);
void CueBuss(COMMAND_T*);
void ReadCueBusIniFile(int _confId, char* _busName, int _busNameSz, int* _reaType, bool* _trTemplate, char* _trTemplatePath, int _trTemplatePathSz, bool* _showRouting, int* _soloDefeat, bool* _sendToMaster, int* _hwOuts);
void SaveCueBusIniFile(int _confId, const char* _busName, int _type, bool _trTemplate, const char* _trTemplatePath, bool _showRouting, int _soloDefeat, bool _sendToMaster, int* _hwOuts);
void CopySendsReceives(bool _noIntra, WDL_PtrList<MediaTrack>* _trs, WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _sends,  WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _rcvs);
bool PasteSendsReceives(WDL_PtrList<MediaTrack>* _trs, WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _sends,  WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _rcvs, bool _rcvReplace, WDL_PtrList<SNM_ChunkParserPatcher>* _ps);
void CopyWithIOs(COMMAND_T*);
void CutWithIOs(COMMAND_T*);
void PasteWithIOs(COMMAND_T*);
void CopyRoutings(COMMAND_T*);
void CutRoutings(COMMAND_T*);
void PasteRoutings(COMMAND_T*);
void CopySends(COMMAND_T*);
void CutSends(COMMAND_T*);
void PasteSends(COMMAND_T*);
void CopyReceives(COMMAND_T*);
void CutReceives(COMMAND_T*);
void PasteReceives(COMMAND_T*);

bool RemoveSnd(WDL_PtrList<MediaTrack>* _trs, WDL_PtrList<SNM_ChunkParserPatcher>* _ps);
void RemoveSends(COMMAND_T*);
bool RemoveRcv(WDL_PtrList<MediaTrack>* _trs, WDL_PtrList<SNM_ChunkParserPatcher>* _ps);
void RemoveReceives(COMMAND_T*);
void RemoveRoutings(COMMAND_T*);

void SaveDefaultTrackSendPrefs(COMMAND_T*);
void RecallDefaultTrackSendPrefs(COMMAND_T*);
void SetDefaultTrackSendPrefs(COMMAND_T*);

void MuteReceives(MediaTrack* _source, MediaTrack* _dest, bool _mute);

#endif
