/******************************************************************************
/ Analysis.h
/
/ Copyright (c) 2011 Tim Payne (SWS)
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

#define SWS_RMS_KEY "RMS normalize params"

// Data passing to/from the Analyze functions.
// All array pointers are caller alloc'ed and optional (NULL)
typedef struct ANALYZE_PCM
{
	PCM_source* pcm;        // in  Pointer to zero-based PCM source (required)
	int iChannels;          // in  Dimension of the channel arrays.  This can be zero to disable, or < number of actual item channels
	double* dPeakVals;      // i/o Array of channel peaks, caller alloc, iChannel size (optional)
	double dPeakVal;        // out Maximum peak valume over all channels
	double* dRMSs;          // i/o Array of channel RMS values
	double dRMS;            // out RMS of all channels
	INT64* peakRMSsamples;  // i/o Array of channel RMS peak locations (not calculated if dRMSs is omitted), -666 if dWindowSize == 0.0 or >= item length 
	INT64 peakRMSsample;    // out Position of overall peak RMS in windowed mode, -666 if dWindowSize == 0.0 or >= item length 
	INT64* peakSamples;     // i/o Array of channel peak locations (not calculated if dPeakVals is omitted)
	INT64 peakSample;       // out Position of max peak
	double dProgress;       // out Analysis progress, 0.0-1.0 for 0-100%
	INT64 sampleCount;      // out # of samples analyzed
	double dWindowSize;     // RMS window in seconds.  If this is != 0.0, then RMS is calculated/returned as max within window
	bool success;
} ANALYZE_PCM;

int AnalysisInit();

bool AnalyzeItem(MediaItem* mi, ANALYZE_PCM* a);

// #781 Export to ReaScript
void NF_GetRMSOptions(double *targetOut, double *winSizeOut);
bool NF_SetRMOptions(double target, double windowSize);
