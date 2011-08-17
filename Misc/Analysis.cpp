/******************************************************************************
/ Analysis.cpp
/
/ Copyright (c) 2011 Tim Payne (SWS)
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

#include "stdafx.h"
#include "Analysis.h"
#include "../sws_waitdlg.h"

// dPeakVal, dRMS, and peakSample are arrays of iChannels size.
// iChannels can be != pSrc channels.
// dProgress is a double from 0.0-1.0 for threaded progress bar generation
void AnalyzePCMSource(PCM_source *pSrc, int iChannels, double dPeakVal[], double dRMS[], INT64 peakSample[], double* dProgress = NULL)
{
	if (!pSrc)
		return;

	const int iFrameLen = 16384;
	ReaSample* buf = new ReaSample[iFrameLen * pSrc->GetNumChannels()];
	double* dSumSquares = new double[iChannels];
	for (int i = 0; i < iChannels; i++)
	{
		if (dPeakVal) dPeakVal[i] = 0.0;
		if (dRMS) dRMS[i] = 0.0;
		if (peakSample) peakSample[i] = 0;
		dSumSquares[i] = 0.0;
	}
	if (dProgress) *dProgress = 0.0;
	
	PCM_source_transfer_t transferBlock={0,};
	transferBlock.length = iFrameLen;
	transferBlock.samplerate = pSrc->GetSampleRate();
	transferBlock.samples = buf;
	transferBlock.nch = pSrc->GetNumChannels();
	transferBlock.time_s = 0.0;

	INT64 sampleCounter = 0;
	INT64 totalSamples = pSrc->GetLength() * pSrc->GetSampleRate();
	int iFrame = 0;

	pSrc->GetSamples(&transferBlock);
	while (transferBlock.samples_out)
	{
		for (int samp = 0; samp < transferBlock.samples_out; samp++)
		{
			for (int chan = 0; chan < transferBlock.nch && chan < iChannels; chan++)
			{
				int i = samp*transferBlock.nch + chan;
				if (dRMS)
					dSumSquares[chan] += buf[i] * buf[i];
				if (dPeakVal && fabs(buf[i]) > dPeakVal[chan])
				{
					dPeakVal[chan] = fabs(buf[i]);
					if (peakSample)
						peakSample[chan] = sampleCounter;
				}
			}
			sampleCounter++;
		}

		if (dProgress) *dProgress = (double)sampleCounter / totalSamples;
		
		iFrame++;
		transferBlock.time_s = (double)iFrameLen * iFrame / pSrc->GetSampleRate();
		pSrc->GetSamples(&transferBlock);
	}

	if (dRMS && sampleCounter)
		for (int i = 0; i < iChannels; i++)
			dRMS[i] = sqrt(dSumSquares[i] / sampleCounter);

	delete[] buf;
	delete[] dSumSquares;

	// Ensure dProgress is exactly 1.0
	if (dProgress) *dProgress = 1.0;
}

typedef struct ANALYZE_PCM
{
	PCM_source* pcm;
	int iChannels;
	double* dPeakVal;
	double* dRMS;
	INT64* peakSample;
	double dProgress;
} ANALYZE_PCM;

DWORD WINAPI AnalyzePCMThread(void* pAnalyze)
{
	ANALYZE_PCM* p = (ANALYZE_PCM*)pAnalyze;
	AnalyzePCMSource(p->pcm, p->iChannels, p->dPeakVal, p->dRMS, p->peakSample, &p->dProgress);
	return 0;
}

void AnalyzePCM(PCM_source *pSrc, int iChannels, double dPeakVal[], double dRMS[], INT64 peakSample[], const char* cName)
{
	ANALYZE_PCM a;
	a.pcm = pSrc;
	a.iChannels = iChannels;
	a.dPeakVal = dPeakVal;
	a.dRMS = dRMS;
	a.peakSample = peakSample;
	a.dProgress = 0.0;
	CreateThread(NULL, 0, AnalyzePCMThread, &a, 0, NULL);

	WDL_String title;
	title.AppendFormatted(100, "Please wait, analyzing %s...", cName ? cName : "item");
	SWS_WaitDlg wait(title.Get(), &a.dProgress);
}

void AnalyzeItem(COMMAND_T*)
{
	WDL_TypedBuf<MediaItem*> items;
	SWS_GetSelectedMediaItems(&items);
	bool bDidWork = false;
	for (int i = 0; i < items.GetSize(); i++)
	{
		MediaItem* mi = items.Get()[i];
		PCM_source* pSrc = (PCM_source*)mi;
		if (pSrc && strcmp(pSrc->GetType(), "MIDI") && strcmp(pSrc->GetType(), "MIDIPOOL"))
		{
			pSrc = pSrc->Duplicate();
			if (pSrc && pSrc->GetNumChannels())
			{
				bDidWork = true;
				double* dPeakVal = new double[pSrc->GetNumChannels()];
				double* dRMS = new double[pSrc->GetNumChannels()];

				double dZero = 0.0;
				GetSetMediaItemInfo((MediaItem*)pSrc, "D_POSITION", &dZero);
				
				// Do the work!
				const char* cName = NULL;
				MediaItem_Take* take = GetMediaItemTake(mi, -1);
				if (take)
					cName = (const char*)GetSetMediaItemTakeInfo(take, "P_NAME", NULL);

				AnalyzePCM(pSrc, pSrc->GetNumChannels(), dPeakVal, dRMS, NULL, cName); 

				WDL_String str;
				str.Set("Peak level:");
				for (int i = 0; i < pSrc->GetNumChannels(); i++)
					str.AppendFormatted(50, " Channel %d = %.2f dB", i+1, VAL2DB(dPeakVal[i]));
				str.Append("\nRMS level:");
				for (int i = 0; i < pSrc->GetNumChannels(); i++)
					str.AppendFormatted(50, " Channel %d = %.2f dB", i+1, VAL2DB(dRMS[i]));
				MessageBox(g_hwndParent, str.Get(), "Item analysis", MB_OK);
				delete pSrc;
				delete [] dPeakVal;
				delete [] dRMS;
			}
		}
	}
	if (!bDidWork)
	{
		MessageBox(NULL, "No items selected to analyze.", "Error", MB_OK);
		return;
	}
}

void FindItemPeak(COMMAND_T*)
{
	// Just use the first item
	MediaItem* mi = GetSelectedMediaItem(NULL, 0);
	if (mi)
	{
		PCM_source* pSrc = (PCM_source*)mi;
		if (pSrc && strcmp(pSrc->GetType(), "MIDI") && strcmp(pSrc->GetType(), "MIDIPOOL"))
		{
			pSrc = pSrc->Duplicate();
			if (pSrc && pSrc->GetNumChannels())
			{
				double* dPeakVal = new double[pSrc->GetNumChannels()];
				INT64* peakSample = new INT64[pSrc->GetNumChannels()];

				double dZero = 0.0;
				GetSetMediaItemInfo((MediaItem*)pSrc, "D_POSITION", &dZero);

				// Do the work!
				const char* cName = NULL;
				MediaItem_Take* take = GetMediaItemTake(mi, -1);
				if (take)
					cName = (const char*)GetSetMediaItemTakeInfo(take, "P_NAME", NULL);
				AnalyzePCM(pSrc, pSrc->GetNumChannels(), dPeakVal, NULL, peakSample, cName); 
			
				int iMaxChan = 0;
				double dMaxPeak = 0.0;
				for (int i = 0; i < pSrc->GetNumChannels(); i++)
					if (dPeakVal[i] > dMaxPeak)
						iMaxChan = i;

				double dSrate = pSrc->GetSampleRate();
				double dPos = *(double*)GetSetMediaItemInfo(mi, "D_POSITION", NULL);
				dPos += peakSample[iMaxChan] / dSrate;
				SetEditCurPos(dPos, true, false);

				delete pSrc;
				delete [] dPeakVal;
				delete [] peakSample;
			}
		}
	}
	else
		MessageBox(NULL, "No items selected to analyze.", "Error", MB_OK);
}

void OrganizeByRMS(COMMAND_T* ct)
{
	for (int iTrack = 1; iTrack <= GetNumTracks(); iTrack++)
	{
		WDL_TypedBuf<MediaItem*> items;
		SWS_GetSelectedMediaItemsOnTrack(&items, CSurf_TrackFromID(iTrack, false));
		if (items.GetSize() > 1)
		{
			double dStart = *(double*)GetSetMediaItemInfo(items.Get()[0], "D_POSITION", NULL);
			double* pRMS = new double[items.GetSize()];
			for (int i = 0; i < items.GetSize(); i++)
			{
				PCM_source* pSrc = (PCM_source*)items.Get()[i];
				pRMS[i] = -1.0;
				if (pSrc && strcmp(pSrc->GetType(), "MIDI") && strcmp(pSrc->GetType(), "MIDIPOOL"))
				{
					pSrc = pSrc->Duplicate();
					if (pSrc && pSrc->GetNumChannels())
					{
						double dZero = 0.0;
						GetSetMediaItemInfo((MediaItem*)pSrc, "D_POSITION", &dZero);
						double* dRMS = new double[pSrc->GetNumChannels()];
						const char* cName = NULL;
						MediaItem_Take* take = GetMediaItemTake(items.Get()[i], -1);
						if (take)
							cName = (const char*)GetSetMediaItemTakeInfo(take, "P_NAME", NULL);
						AnalyzePCM(pSrc, pSrc->GetNumChannels(), NULL, dRMS, NULL, cName);
						for (int j = 0; j < pSrc->GetNumChannels(); j++)
							if (dRMS[j] > pRMS[i])
								pRMS[i] = dRMS[j];
						delete pSrc;
						delete [] dRMS;
					}
				}
			}
			// Sort and arrange items from min to max RMS
			while (true)
			{
				int iItem = -1;
				double dMinRMS = 1e99;
				for (int i = 0; i < items.GetSize(); i++)
					if (pRMS[i] >= 0.0 && pRMS[i] < dMinRMS)
					{
						dMinRMS = pRMS[i];
						iItem = i;
					}
				if (iItem == -1)
					break;
				pRMS[iItem] = -1.0;
				GetSetMediaItemInfo(items.Get()[iItem], "D_POSITION", &dStart);
				dStart += *(double*)GetSetMediaItemInfo(items.Get()[iItem], "D_LENGTH", NULL);
			}
			delete [] pRMS;
			UpdateTimeline();
			Undo_OnStateChangeEx(SWS_CMD_SHORTNAME(ct), UNDO_STATE_ITEMS, -1);
		}
	}
}

static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "SWS: Analyze and display item peak and RMS" },	"SWS_ANALYZEITEM",		AnalyzeItem,	NULL, },
	{ { DEFACCEL, "SWS: Move cursor to item peak amplitude" },		"SWS_FINDITEMPEAK",		FindItemPeak,	NULL, },
	{ { DEFACCEL, "SWS: Organize items by RMS" },					"SWS_RMSORGANIZE",		OrganizeByRMS,	NULL, },

	{ {}, LAST_COMMAND, }, // Denote end of table
};

int AnalysisInit()
{
	SWSRegisterCommands(g_commandTable);

	return 1;
}