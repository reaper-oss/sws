/******************************************************************************
/ padreActions.h
/
/ Copyright (c) 2009-2010 Tim Payne (SWS), Jeffos (S&M), P. Bourdon
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

#pragma once

//! \todo Code cleanup /guidelines: m_/g_ prefixes, etc.
//! \todo Update with sws/wdl API functions (ObjState access, etc)
//! \todo General code cleanup, renaming
//! \todo LFO frequency modulator
//! \todo Env processor: track/take/take (MIDI) combo box
//! \todo Actions for my own use (e.g. 'shrink item' bugfix): #define-based bypass to skip in release mode? 
//! \todo MIDI LFO new options: channel, pitch
//! \bug Env processor: process 1st point
//! \todo RME Fireface Mixer actions: separate .dll
//! \todo FR Cockos: Insert Env Point should preserve envelope form (use next Pt shape instead of default?)

//! \note I usually keep beta stuff/tests undented

int PadreInit();
void PadreExit();

WDL_DLGRET EnvelopeLfoDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void EnvelopeLfo(COMMAND_T* _ct);

//! \note Quick one to fix Reaper MIDI bug in loop mode
void ShrinkSelectedTakes(int nbSamples = 512, bool bActiveOnly = true);
void ShrinkSelItems(COMMAND_T* _ct);

//! \note Proof of concept action. Deprecated (Cockos' Humanize is better)
void RandomizeMidiNotePos(COMMAND_T* _ct);

void DoEnvelopeProcessor(COMMAND_T* _ct);

WDL_DLGRET EnvelopeProcessorDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void RmeTotalmixProc(COMMAND_T* _ct);
