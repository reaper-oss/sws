/******************************************************************************
/ padreUtils.h
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

using namespace std;

#define		MIDIITEMPROC_DEFAULT_SAMPLERATE		48000.0
#define		MIDIITEMPROC_DEFAULT_NBCHANNELS		2

#define PI	3.14159265358979323846264338327950288

#define ID_ENVELOPE_SELECT_ALL_POINTS_TIMESEL	40330
#define ID_ENVELOPE_DELETE_ALL_SELECTED_POINTS	40333
#define ID_ENVELOPE_INSERT_POINT				40915
#define ID_ENVELOPE_INSERT_4POINTS_TIMESEL		40726
#define ID_ENVELOPE_DEFAULT_SHAPE_LINEAR		40187
#define ID_MOVE_CURSOR_GRID_DIVISION_LEFT		40646
#define ID_MOVE_CURSOR_GRID_DIVISION_RIGHT		40647
#define ID_MOVE_CURSOR_LEFT						40104
#define ID_MOVE_CURSOR_RIGHT					40105

#define ID_ENVELOPE_DELETE_ALL_POINTS_TIMESEL	40089
#define ID_GOTO_TIMESEL_START					40630
#define ID_GOTO_TIMESEL_END						40631
#define ID_GOTO_SELITEM_START					41173
#define ID_GOTO_SELITEM_END						41174
#define ID_GOTO_CURMEASURE_START				41041
#define ID_GOTO_NEXTMEASURE_START				41040
#define ID_GOTO_LOOP_START						40632
#define ID_GOTO_LOOP_END						40633
#define ID_GOTO_PROJECT_START					40042
#define ID_GOTO_PROJECT_END						40043

#define ID_MOVE_TIMESEL_NUDGE_LEFTEDGE_LEFT		40320
#define ID_MOVE_TIMESEL_NUDGE_LEFTEDGE_RIGHT	40321
#define ID_MOVE_TIMESEL_NUDGE_RIGHTEDGE_LEFT	40322
#define ID_MOVE_TIMESEL_NUDGE_RIGHTEDGE_RIGHT	40323

#define ID_TAKEENV_VOLUME_TOGGLE				40693
#define ID_TAKEENV_PAN_TOGGLE					40694
#define ID_TAKEENV_MUTE_TOGGLE					40695

enum WaveShape {eWAVSHAPE_SINE = 0, eWAVSHAPE_TRIANGLE, eWAVSHAPE_SQUARE, eWAVSHAPE_RANDOM, eWAVSHAPE_SAWUP, eWAVSHAPE_SAWDOWN,
				eWAVSHAPE_TRIANGLE_BEZIER, eWAVSHAPE_RANDOM_BEZIER, eWAVSHAPE_SAWUP_BEZIER, eWAVSHAPE_SAWDOWN_BEZIER };

enum TakeEnvType {eTAKEENV_VOLUME, eTAKEENV_PAN, eTAKEENV_MUTE, eTAKEENV_PITCH};

enum GridDivision {	eGRID_OFF = 0, eGRID_128_1, eGRID_128T_1, eGRID_64_1, eGRID_64T_1, eGRID_32_1, eGRID_32T_1, eGRID_16_1, eGRID_16T_1, eGRID_8_1, eGRID_8T_1, eGRID_4_1, eGRID_4T_1, eGRID_2_1, eGRID_2T_1, eGRID_1_1, eGRID_1T_1, eGRID_1_2, eGRID_1_2T, eGRID_1_4, eGRID_1_4T, eGRID_1_8, eGRID_1_8T, eGRID_1_16, eGRID_1_16T,
					eGRID_1_32, eGRID_1_32T, eGRID_1_64, eGRID_1_64T, eGRID_1_128, eGRID_1_128T, eGRID_LAST };

enum EnvShape {eENVSHAPE_LINEAR = 0, eENVSHAPE_SQUARE = 1, eENVSHAPE_SLOW = 2, eENVSHAPE_FASTSTART = 3, eENVSHAPE_FASTEND = 4, eENVSHAPE_BEZIER = 5};

enum TimeSegment {eTIMESEGMENT_TIMESEL, eTIMESEGMENT_PROJECT, eTIMESEGMENT_SELITEM, eTIMESEGMENT_LOOP, eTIMESEGMENT_LAST };

const char* GetWaveShapeStr(WaveShape shape);
const char* GetGridDivisionStr(GridDivision grid);
double GetGridDivisionFactor(GridDivision grid);
const char* GetTakeEnvelopeStr(TakeEnvType type);
double Sign(double value);

double WaveformGeneratorSin(double t, double dFreq, double dDelay);
double WaveformGeneratorSquare(double t, double dFreq, double dDelay, bool &bFlipFlop);
double WaveformGeneratorSquare(double t, double dFreq, double dDelay);
double WaveformGeneratorTriangle(double t, double dFreq, double dDelay, bool &bFlipFlop);
double WaveformGeneratorTriangle(double t, double dFreq, double dDelay);
double WaveformGeneratorSawUp(double t, double dFreq, double dDelay);
double WaveformGeneratorSawDown(double t, double dFreq, double dDelay);
double WaveformGeneratorRandom(double t, double dFreq, double dDelay);

double EnvSignalProcessorFade(double dPos, double dLength, double dStrength, bool bFadeIn);

void ShowConsoleMsgEx(const char* format, ...);

void GetTimeSegmentPositions(TimeSegment timeSegment, double &dStartPos, double &dEndPos, MediaItem* item = NULL);
const char* GetTimeSegmentStr(TimeSegment timeSegment);

void GetSelectedMediaItems(list<MediaItem*> &items);
void GetMediaItemTakes(MediaItem* item, list<MediaItem_Take*> &takes, bool bActiveOnly = true);
void GetSelectedMediaTakes(list<MediaItem_Take*> &takes, bool bActiveOnly = true);

