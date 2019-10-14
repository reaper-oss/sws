/******************************************************************************
/ BR.h
/
/ Copyright (c) 2012-2015 Dominik Martin Drzic
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
* Command hook                                                                *
******************************************************************************/
bool BR_GlobalActionHook (int cmd, int val, int valhw, int relmode, HWND hwnd);
bool BR_SwsActionHook (COMMAND_T* ct, int flagOrRelmode, HWND hwnd);
void BR_MenuHook (COMMAND_T* ct, HMENU menu, int id);
int  BR_GetNextActionToApply ();

/******************************************************************************
* Csurf                                                                       *
******************************************************************************/
void BR_CSurf_SetPlayState (bool play, bool pause, bool rec);
void BR_CSurf_OnTrackSelection (MediaTrack* track);
int  BR_CSurf_Extended (int call, void* parm1, void* parm2, void* parm3);

/******************************************************************************
* Continuous actions                                                          *
******************************************************************************/
void BR_RegisterContinuousActions ();

/******************************************************************************
* BR init/exit                                                                *
******************************************************************************/
int  BR_Init ();
int  BR_InitPost ();
void BR_Exit ();
