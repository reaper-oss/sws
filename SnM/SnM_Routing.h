/******************************************************************************
/ SnM_Routing.h
/
/ Copyright (c) 2012-2013 Jeffos
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

void RefreshRoutingsUI();

void CopySendsReceives(bool _noIntra, WDL_PtrList<MediaTrack>* _trs, WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _snds,  WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _rcvs);
bool PasteSendsReceives(WDL_PtrList<MediaTrack>* _trs, WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _snds,  WDL_PtrList_DeleteOnDestroy<WDL_PtrList_DeleteOnDestroy<SNM_SndRcv> >* _rcvs, WDL_PtrList<SNM_ChunkParserPatcher>* _ps);
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

bool RemoveSends(WDL_PtrList<MediaTrack>* _trs, WDL_PtrList<SNM_ChunkParserPatcher>* _ps);
void RemoveSends(COMMAND_T*);
bool RemoveReceives(WDL_PtrList<MediaTrack>* _trs, WDL_PtrList<SNM_ChunkParserPatcher>* _ps);
void RemoveReceives(COMMAND_T*);
void RemoveRoutings(COMMAND_T*);

bool SNM_AddReceive(MediaTrack* _srcTr, MediaTrack* _destTr, int _type);
bool SNM_RemoveReceive(MediaTrack* _tr, int _rcvIdx);
bool SNM_RemoveReceivesFrom(MediaTrack* _tr, MediaTrack* _srcTr);

void SaveDefaultTrackSendPrefs(COMMAND_T*);
void RecallDefaultTrackSendPrefs(COMMAND_T*);
void SetDefaultTrackSendPrefs(COMMAND_T*);

bool MuteSends(MediaTrack* _src, MediaTrack* _dest, bool _mute);
bool MuteReceives(MediaTrack* _src, MediaTrack* _dest, bool _mute);
bool HasReceives(MediaTrack* _src, MediaTrack* _dest);

#endif
