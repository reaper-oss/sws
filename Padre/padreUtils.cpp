/******************************************************************************
/ padreUtils.cpp
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

#include "stdafx.h"

#include <WDL/localize/localize.h>

const char* GetWaveShapeStr(WaveShape shape)
{
	switch(shape)
	{
		case eWAVSHAPE_SINE			: return __LOCALIZE("Sine","sws_mbox");		break;
		case eWAVSHAPE_TRIANGLE		: return __LOCALIZE("Triangle","sws_mbox");	break;
		case eWAVSHAPE_SQUARE		: return __LOCALIZE("Square","sws_mbox");	break;
		case eWAVSHAPE_RANDOM		: return __LOCALIZE("Random","sws_mbox");	break;
		case eWAVSHAPE_SAWUP		: return __LOCALIZE("Saw Up","sws_mbox");	break;
		case eWAVSHAPE_SAWDOWN		: return __LOCALIZE("Saw Down","sws_mbox");	break;

		case eWAVSHAPE_TRIANGLE_BEZIER		: return __LOCALIZE("Triangle (Bezier)","sws_mbox");	break;
		case eWAVSHAPE_RANDOM_BEZIER		: return __LOCALIZE("Random (Bezier)","sws_mbox");		break;
		case eWAVSHAPE_SAWUP_BEZIER			: return __LOCALIZE("Saw Up (Bezier)","sws_mbox");		break;
		case eWAVSHAPE_SAWDOWN_BEZIER		: return __LOCALIZE("Saw Down (Bezier)","sws_mbox");	break;

		default								: return "???";			break;
	}
}

const char* GetGridDivisionStr(GridDivision grid)
{
	switch(grid)
	{
		case eGRID_OFF		: return __LOCALIZE("<sync off>","sws_mbox");	break;
		case eGRID_128_1	: return "128/1";		break;
		case eGRID_128T_1	: return "128T/1";		break;
		case eGRID_64_1		: return "64/1";		break;
		case eGRID_64T_1	: return "64T/1";		break;
		case eGRID_32_1		: return "32/1";		break;
		case eGRID_32T_1	: return "32T/1";		break;
		case eGRID_16_1		: return "16/1";		break;
		case eGRID_16T_1	: return "16T/1";		break;
		case eGRID_8_1		: return "8/1";			break;
		case eGRID_8T_1		: return "8T/1";		break;
		case eGRID_4_1		: return "4/1";			break;
		case eGRID_4T_1		: return "4T/1";		break;
		case eGRID_2_1		: return "2/1";			break;
		case eGRID_2T_1		: return "2T/1";		break;
		case eGRID_1_1		: return "1/1";			break;
		case eGRID_1T_1		: return "1T/1";		break;
		case eGRID_1_2		: return "1/2";			break;
		case eGRID_1_2T		: return "1/2T";		break;
		case eGRID_1_4		: return "1/4";			break;
		case eGRID_1_4T		: return "1/4T";		break;
		case eGRID_1_8		: return "1/8";			break;
		case eGRID_1_8T		: return "1/8T";		break;
		case eGRID_1_16		: return "1/16";		break;
		case eGRID_1_16T	: return "1/16T";		break;
		case eGRID_1_32		: return "1/32";		break;
		case eGRID_1_32T	: return "1/32T";		break;
		case eGRID_1_64		: return "1/64";		break;
		case eGRID_1_64T	: return "1/64T";		break;
		case eGRID_1_128	: return "1/128";		break;
		case eGRID_1_128T	: return "1/128T";		break;
		default				: return "???";			break;
	}
}

double GetGridDivisionFactor(GridDivision grid)
{
	switch(grid)
	{
		case eGRID_128_1	: return 0.0078125;		break;
		case eGRID_128T_1	: return 3.0/256.0;		break;
		case eGRID_64_1		: return 0.015625;		break;
		case eGRID_64T_1	: return 3.0/128.0;		break;
		case eGRID_32_1		: return 0.03125;		break;
		case eGRID_32T_1	: return 3.0/64.0;		break;
		case eGRID_16_1		: return 0.0625;		break;
		case eGRID_16T_1	: return 3.0/32.0;		break;
		case eGRID_8_1		: return 0.125;			break;
		case eGRID_8T_1		: return 3.0/16.0;		break;
		case eGRID_4_1		: return 0.25;			break;
		case eGRID_4T_1		: return 3.0/8.0;		break;
		case eGRID_2_1		: return 0.5;			break;
		case eGRID_2T_1		: return 3.0/4.0;		break;
		case eGRID_1_1		: return 1.0;			break;
		case eGRID_1T_1		: return 3.0/2.0;		break;
		case eGRID_1_2		: return 2.0;			break;
		case eGRID_1_2T		: return 3.0;			break;
		case eGRID_1_4		: return 4.0;			break;
		case eGRID_1_4T		: return 6.0;			break;
		case eGRID_1_8		: return 8.0;			break;
		case eGRID_1_8T		: return 12.0;			break;
		case eGRID_1_16		: return 16.0;			break;
		case eGRID_1_16T	: return 24.0;			break;
		case eGRID_1_32		: return 32.0;			break;
		case eGRID_1_32T	: return 48.0;			break;
		case eGRID_1_64		: return 64.0;			break;
		case eGRID_1_64T	: return 96.0;			break;
		case eGRID_1_128	: return 128.0;			break;
		case eGRID_1_128T	: return 192.0;			break;
		default				: return -1.0;			break;
	}
}

const char* GetTakeEnvelopeStr(TakeEnvType type)
{
	switch(type)
	{
		case eTAKEENV_VOLUME	: return __LOCALIZE("Volume","sws_mbox");	break;
		case eTAKEENV_PAN		: return __LOCALIZE("Pan","sws_mbox");		break;
		case eTAKEENV_MUTE		: return __LOCALIZE("Mute","sws_mbox");		break;
		case eTAKEENV_PITCH		: return __LOCALIZE("Pitch","sws_mbox");	break;
		default					: return NULL;		break;
	}
}

double Sign(double value)
{
	if(value>0.0)
		return 1.0;
	else
		return -1.0;
}

double WaveformGeneratorSin(double t, double dFreq, double dDelay)
{
	return sin(2.0*PI*dFreq*(t+dDelay));
}

double WaveformGeneratorSquare(double t, double dFreq, double dDelay, bool &bFlipFlop)
{
	double dPhase = dFreq*(t+dDelay);
	dPhase = dPhase - (int)(dPhase);
	if(dPhase<0.0)
		dPhase += 1.0;
	if(dPhase<0.5)
	{
		bFlipFlop = false;
		return -1.0;
	}
	else
	{
		bFlipFlop = true;
		return 1.0;
	}
}

double WaveformGeneratorSquare(double t, double dFreq, double dDelay)
{
	 bool bFlipFlop;
	 return WaveformGeneratorSquare(t, dFreq, dDelay, bFlipFlop);
}

double WaveformGeneratorTriangle(double t, double dFreq, double dDelay, bool &bFlipFlop)
{
	double dPhase = dFreq*(t+dDelay);
	dPhase = dPhase - (int)(dPhase);
	if(dPhase<0.0)
		dPhase += 1.0;
	if(dPhase<0.5)
	{
		bFlipFlop = false;
		return -4.0*dPhase + 1.0;
	}
	else
	{
		bFlipFlop = true;
		return 4.0*dPhase - 3.0;
	}
}

double WaveformGeneratorTriangle(double t, double dFreq, double dDelay)
{
	 bool bFlipFlop;
	 return WaveformGeneratorTriangle(t, dFreq, dDelay, bFlipFlop);
}

double WaveformGeneratorSawUp(double t, double dFreq, double dDelay)
{
	double dPhase = dFreq*(t+dDelay);
	dPhase = dPhase - (int)(dPhase);
	if(dPhase<0.0)
		dPhase += 1.0;
	return 2.0*dPhase - 1.0;
}

double WaveformGeneratorSawDown(double t, double dFreq, double dDelay)
{
	double dPhase = dFreq*(t+dDelay);
	dPhase = dPhase - (int)(dPhase);
	if(dPhase<0.0)
		dPhase += 1.0;
	return -2.0*dPhase + 1.0;
}

//! \todo Make true Freq-dependent generator using clock_t
double WaveformGeneratorRandom(double t, double dFreq, double dDelay)
{
	return 2.0*(double)rand()/(double)(RAND_MAX) - 1.0;
}

double EnvSignalProcessorFade(double dPos, double dLength, double dStrength, bool bFadeIn)
{
	if(bFadeIn)
		return pow((dPos/dLength), dStrength);
	else
		return pow(((dLength - dPos)/dLength), dStrength);
}

void ShowConsoleMsgEx(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	char buffer[2048];
	vsnprintf(buffer, 2048, format, args);
	ShowConsoleMsg(buffer);
	va_end(args);
}

void GetTimeSegmentPositions(TimeSegment timeSegment, double &dStartPos, double &dEndPos, MediaItem* item)
{
	double dOrgCursorPos = GetCursorPositionEx(0);
	bool bRefreshCurPos = false;

	switch(timeSegment)
	{
		case eTIMESEGMENT_TIMESEL:
			//Main_OnCommandEx(ID_GOTO_TIMESEL_END, 0, 0);
			//dEndPos = GetCursorPositionEx(0);
			//Main_OnCommandEx(ID_GOTO_TIMESEL_START, 0, 0);
			//dStartPos = GetCursorPositionEx(0);
			GetSet_LoopTimeRange2(0, false, false, &dStartPos, &dEndPos, false);
		break;
		case eTIMESEGMENT_SELITEM:
			if(item != NULL)
			{
				dStartPos = GetMediaItemInfo_Value(item, "D_POSITION");
				dEndPos = dStartPos + GetMediaItemInfo_Value(item, "D_LENGTH");
			}
			else
			{
				Main_OnCommandEx(ID_GOTO_SELITEM_END, 0, 0);
				dEndPos = GetCursorPositionEx(0);
				Main_OnCommandEx(ID_GOTO_SELITEM_START, 0, 0);
				dStartPos = GetCursorPositionEx(0);
				bRefreshCurPos = true;
			}
		break;
		case eTIMESEGMENT_LOOP:
			//Main_OnCommandEx(ID_GOTO_LOOP_END, 0, 0);
			//dEndPos = GetCursorPositionEx(0);
			//Main_OnCommandEx(ID_GOTO_LOOP_START, 0, 0);
			//dStartPos = GetCursorPositionEx(0);
			GetSet_LoopTimeRange2(0, false, true, &dStartPos, &dEndPos, false);
		break;
		case eTIMESEGMENT_PROJECT:
			Main_OnCommandEx(ID_GOTO_PROJECT_END, 0, 0);
			dEndPos = GetCursorPositionEx(0);
			//Main_OnCommandEx(ID_GOTO_PROJECT_START, 0, 0);
			//dStartPos = GetCursorPositionEx(0);
			dStartPos = *ConfigVar<int>("projtimeoffs");
			bRefreshCurPos = true;
		break;
		//case eTIMESEGMENT_CURRENTMEASURE:
		//	Main_OnCommandEx(ID_GOTO_CURMEASURE_START, 0, 0);
		//	dStartPos = GetCursorPositionEx(0);
		//	Main_OnCommandEx(ID_GOTO_NEXTMEASURE_START, 0, 0);
		//	dEndPos = GetCursorPositionEx(0);
		//break;
		default:
		break;
	}

	if(bRefreshCurPos)
		SetEditCurPos2(0, dOrgCursorPos, true, false);
}

const char* GetTimeSegmentStr(TimeSegment timeSegment)
{
	switch(timeSegment)
	{
		case eTIMESEGMENT_TIMESEL			: return __LOCALIZE("Time selection","sws_mbox");	break;
		case eTIMESEGMENT_PROJECT			: return __LOCALIZE("Project","sws_mbox");			break;
		case eTIMESEGMENT_SELITEM			: return __LOCALIZE("Selected item","sws_mbox");	break;
		case eTIMESEGMENT_LOOP				: return __LOCALIZE("Loop","sws_mbox");				break;
		//case eTIMESEGMENT_CURRENTMEASURE	: return __LOCALIZE("Current measure","sws_mbox");	break;
		default								: return "???";										break;
	}
}

void GetSelectedMediaItems(list<MediaItem*> &items)
{
	items.clear();
	int itemIdx = 0;
	while(MediaItem* item = GetSelectedMediaItem(0, itemIdx))
	{
		items.push_back(item);
		itemIdx++;
	}
}

void GetMediaItemTakes(MediaItem* item, list<MediaItem_Take*> &takes, bool bActiveOnly)
{
	takes.clear();

	if(bActiveOnly)
	{
		MediaItem_Take* take = GetActiveTake(item);
		if(take)
			takes.push_back(take);
	}

	else
	{
		int takeIdx = 0;
		while(MediaItem_Take* take = GetTake(item, takeIdx))
		{
			if(take)
				takes.push_back(take);
			takeIdx++;
		}
	}
}

void GetSelectedMediaTakes(list<MediaItem_Take*> &takes, bool bActiveOnly)
{
	takes.clear();
	list<MediaItem*> items;
	GetSelectedMediaItems(items);

	for(list<MediaItem*>::iterator item = items.begin(); item != items.end(); item++)
		GetMediaItemTakes(*item, takes, bActiveOnly);
}
