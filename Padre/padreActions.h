/******************************************************************************
/ padreActions.h
/
/ Copyright (c) 2009-2010 Tim Payne (SWS), JF Bédague, P. Bourdon
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

#include "../resource.h"
#include "padreEnvelopeProcessor.h"

//! \todo Change 'Padre' module name/file prefix + merge into other files after Tim has taken a look ("LfoGenerator" module?)
//! \todo Actions for my own use (e.g. 'shrink item' bugfix): #define-based bypass to skip in release mode? 
//! \todo code cleanup/guidelines: m_/g_ prefixes
//! \todo Update with sws/wdl API functions (ObjState access, etc)
//! \todo MIDI LFO new options: channel, pitch
//! \todo Use S&M ObjectState access
//! \todo RME Fireface Mixer actions
//! \todo Ask Cockos for GetPos() functions + SetCursorPos() + Insert Env Point bug in step mode + int positions (precision vs double)

//! \note I usually keep beta stuff/tests undented

#define PADRE_CMD_SHORTNAME(_ct) (_ct->accel.desc + 11) // +11 to skip "SWS/PADRE: "
#define PADRE_FORMATED_INI_FILE "%s\\Plugins\\PADRE.ini"

int PadreInit();
void PadreExit();

WDL_DLGRET EnvelopeLfoDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
void EnvelopeLfo(COMMAND_T* _ct);

//! \note Quick one to fix Reaper MIDI bug in loop mode
void ShrinkSelectedTakes(int nbSamples = 512, bool bActiveOnly = true);
void ShrinkSelItems(COMMAND_T* _ct);

void RandomizeMidiNotePos(COMMAND_T* _ct);

//void EnvelopeFader(COMMAND_T* _ct);
void EnvelopeProcessor(COMMAND_T* _ct);

WDL_DLGRET EnvelopeProcessorDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
