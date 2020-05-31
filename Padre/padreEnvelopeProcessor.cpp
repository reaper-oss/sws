/******************************************************************************
/ padreEnvelopeProcessor.cpp
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

#include "padreEnvelopeProcessor.h"
#include "../SnM/SnM_Item.h"

#include <WDL/localize/localize.h>

const char* GetEnvTypeStr(EnvType type)
{
	switch(type)
	{
		case eENVTYPE_TRACK			: return __LOCALIZE("Selected track envelope","sws_DLG_148");	break;
		case eENVTYPE_TAKE			: return __LOCALIZE("Selected take(s)","sws_DLG_148");			break;
		case eENVTYPE_MIDICC		: return __LOCALIZE("Selected take(s) (MIDI)","sws_DLG_148");	break;
		default						: return NULL;													break;
	}
}

const char* GetEnvModTypeStr(EnvModType type)
{
	switch(type)
	{
		case eENVMOD_FADEIN			: return __LOCALIZE("Fade in","sws_DLG_148");	break;
		case eENVMOD_FADEOUT		: return __LOCALIZE("Fade out","sws_DLG_148");	break;
		case eENVMOD_AMPLIFY		: return __LOCALIZE("Amplify","sws_DLG_148");	break;
		default						: return NULL;									break;
	}
}

LfoWaveParams::LfoWaveParams()
: shape(eWAVSHAPE_SINE), freqBeat(eGRID_1_1), freqHz(0.0), delayBeat(eGRID_OFF), delayMsec(0.0), strength(1.0), offset(0.0)
{
}

LfoWaveParams& LfoWaveParams::operator=(const LfoWaveParams &params)
{
	this->shape = params.shape;
	this->freqBeat = params.freqBeat;
	this->freqHz = params.freqHz;
	this->delayBeat = params.delayBeat;
	this->delayMsec = params.delayMsec;
	this->strength = params.strength;
	this->offset = params.offset;
	return *this;
}

EnvLfoParams::EnvLfoParams()
: waveParams(), precision(0.05), midiCc(7), takeEnvType(eTAKEENV_VOLUME), envType(eENVTYPE_TRACK), timeSegment(eTIMESEGMENT_TIMESEL), activeTakeOnly(true)
, freqModulator()
{
}

EnvLfoParams& EnvLfoParams::operator=(const EnvLfoParams &params)
{
	this->waveParams = params.waveParams;
	this->precision = params.precision;
	this->midiCc = params.midiCc;
	this->envType = params.envType;
	this->takeEnvType = params.takeEnvType;
	this->timeSegment = params.timeSegment;
	this->activeTakeOnly = params.activeTakeOnly;
this->freqModulator = params.freqModulator;
	return *this;
}

EnvModParams::EnvModParams()
: envType(eENVTYPE_TRACK), timeSegment(eTIMESEGMENT_TIMESEL), activeTakeOnly(true), takeEnvType(eTAKEENV_VOLUME)
, type(eENVMOD_FADEIN), offset(0.0), strength(1.0)
{
}

EnvModParams& EnvModParams::operator=(const EnvModParams &params)
{
	this->type = params.type;
	this->offset = params.offset;
	this->strength = params.strength;
	return *this;
}

EnvelopeProcessor* EnvelopeProcessor::_pInstance = NULL;

EnvelopeProcessor* EnvelopeProcessor::getInstance()
{
	if(!_pInstance)
		_pInstance = new EnvelopeProcessor();
	return _pInstance;
}

EnvelopeProcessor::EnvelopeProcessor()
: _parameters(), _envModParams()
{
	_midiProcessor = new MidiItemProcessor("MIDI Item LFO Generator");
	_midiProcessor->addFilter(new MidiCcRemover(&_parameters.midiCc));
	_midiProcessor->addGenerator(new MidiCcLfo(&_parameters));
}

EnvelopeProcessor::~EnvelopeProcessor()
{
	delete _midiProcessor;
}

void EnvelopeProcessor::errorHandlerDlg(HWND hwnd, ErrorCode err)
{
	switch(err)
	{
		case EnvelopeProcessor::eERRORCODE_NOENVELOPE:
			MessageBox(hwnd, __LOCALIZE("No envelope selected!","sws_DLG_148"), __LOCALIZE("SWS/Padre - Error","sws_DLG_148"), MB_OK);
		break;
		case EnvelopeProcessor::eERRORCODE_NULLTIMESELECTION:
			MessageBox(hwnd, __LOCALIZE("No time selection!","sws_DLG_148"), __LOCALIZE("SWS/Padre - Error","sws_DLG_148"), MB_OK);
		break;
		case EnvelopeProcessor::eERRORCODE_NOITEMSELECTED:
			MessageBox(hwnd, __LOCALIZE("No item selected!","sws_DLG_148"), __LOCALIZE("SWS/Padre - Error","sws_DLG_148"), MB_OK);
		break;
		case EnvelopeProcessor::eERRORCODE_NOOBJSTATE:
			MessageBox(hwnd, __LOCALIZE("Could not retrieve envelope object state!","sws_DLG_148"), __LOCALIZE("SWS/Padre - Error","sws_DLG_148"), MB_OK);
		break;
		case EnvelopeProcessor::eERRORCODE_UNKNOWN:
			MessageBox(hwnd, __LOCALIZE("Could not generate envelope!","sws_DLG_148"), __LOCALIZE("SWS/Padre - Error","sws_DLG_148"), MB_OK);
		break;
		default:
		break;
	}
}


void EnvelopeProcessor::getFreqDelay(LfoWaveParams &waveParams, double &dFreq, double &dDelay)
{
	double tempoBpm, tempoBpi;
	GetProjectTimeSignature2(0, &tempoBpm, &tempoBpi);

	if(waveParams.freqBeat != eGRID_OFF)
		dFreq = GetGridDivisionFactor(waveParams.freqBeat)*tempoBpm/60.0;
	else
		dFreq = waveParams.freqHz;

	if(waveParams.delayBeat != eGRID_OFF)
		dDelay = 1000.0*60.0/tempoBpm/GetGridDivisionFactor(waveParams.delayBeat);
	else
		dDelay = waveParams.delayMsec;
}

EnvelopeProcessor::ErrorCode EnvelopeProcessor::getTrackEnvelopeMinMax(TrackEnvelope* envelope, double &dEnvMinVal, double &dEnvMaxVal)
{
	if(!envelope)
		return eERRORCODE_NOENVELOPE;

	char* envState = GetSetObjectState(envelope, "");
	if(!envState)
		return eERRORCODE_NOOBJSTATE;

	dEnvMinVal = 0.0;
	dEnvMaxVal = 1.0;
	double dTmp[2];
	int iTmp;

	stringstream envStateStream;
	envStateStream << envState;
	std::vector<string> tokens;
	while(!envStateStream.eof())
	{
		string str;
		getline(envStateStream, str, '\n');
		tokens.push_back(str);
	}

	for(vector<string>::const_iterator it = tokens.begin(); it !=tokens.end(); ++it)
	{
		// Track envelope: Volume
		if(!strcmp(it->c_str(), "<VOLENV") || !strcmp(it->c_str(), "<VOLENV2"))
		{
			dEnvMaxVal = 2.0;
			break;
		}

		// Track envelope: Pan
		if(!strcmp(it->c_str(), "<PANENV") || !strcmp(it->c_str(), "<PANENV2"))
		{
			dEnvMinVal = -1.0;
			break;
		}

		// Track envelope: Param (VST index, min value, max value)
		if(sscanf(it->c_str(), "<PARMENV %d %lf %lf", &iTmp, &dTmp[0], &dTmp[1]) == 3)
		{
			dEnvMinVal = dTmp[0];
			dEnvMaxVal = dTmp[1];
			break;
		}

		if(!strcmp(it->c_str(), ">"))
			break;
	}

	FreeHeapPtr(envState);
	return eERRORCODE_OK;
}

void EnvelopeProcessor::writeLfoPoints(MediaItem_Take* take, string &envState, double dStartTime, double dEndTime, double dValMin, double dValMax, LfoWaveParams &waveParams, double dPrecision, LfoWaveParams* freqModulator)
{
	double dFreq, dDelay;
	getFreqDelay(waveParams, dFreq, dDelay);

	if (take) // account for take playrate, #1158
		dFreq /= GetMediaItemTakeInfo_Value(take, "D_PLAYRATE");

	double dMagnitude = waveParams.strength * (1.0 - fabs(waveParams.offset));
	double dOff = 0.5*(dValMax+dValMin);
	double dScale = dValMax - dOff;
	double dSamplerate;
	double dValue = 0.0;
	char buffer[BUFFER_SIZE];

	EnvShape tEnvShape = eENVSHAPE_LINEAR;
	switch(waveParams.shape)
	{
		case eWAVSHAPE_TRIANGLE :
		case eWAVSHAPE_SAWUP :
		case eWAVSHAPE_SAWDOWN :
			tEnvShape = eENVSHAPE_LINEAR;
		break;

		case eWAVSHAPE_SQUARE :
		case eWAVSHAPE_RANDOM :
			tEnvShape = eENVSHAPE_SQUARE;
		break;

		case eWAVSHAPE_SINE :
		case eWAVSHAPE_TRIANGLE_BEZIER :
		case eWAVSHAPE_RANDOM_BEZIER :
		case eWAVSHAPE_SAWUP_BEZIER :
		case eWAVSHAPE_SAWDOWN_BEZIER :
			tEnvShape = eENVSHAPE_BEZIER;
		break;
		default:
			return;
		break;
	}

	double dPhase = 0.001*dDelay*dFreq;
	dPhase = dPhase - (int)(dPhase);
	if(dPhase<0.0)
		dPhase += 1.0;
	double dDelaySec = dPhase/dFreq;

	double dCarrierStart, dCarrierEnd;
	bool bFlipFlop;
	double dFlipFlop;
	double dLength = dEndTime - dStartTime;

	switch(waveParams.shape)
	{
		case eWAVSHAPE_SINE :
			dSamplerate = dPrecision/dFreq;
			dCarrierStart = WaveformGeneratorSin(0.0, dFreq, dDelaySec);
			dCarrierEnd = WaveformGeneratorSin(dLength, dFreq, dDelaySec);
		break;

		case eWAVSHAPE_TRIANGLE :
		case eWAVSHAPE_TRIANGLE_BEZIER :
			dSamplerate = 0.5/dFreq;
			dCarrierEnd = WaveformGeneratorTriangle(dLength, dFreq, dDelaySec, bFlipFlop);
			dCarrierStart = WaveformGeneratorTriangle(0.0, dFreq, dDelaySec, bFlipFlop);
			dFlipFlop = 2.0*bFlipFlop - 1.0;
		break;

		case eWAVSHAPE_SQUARE :
			dSamplerate = 0.5/dFreq;
			dCarrierEnd = WaveformGeneratorSquare(dLength, dFreq, dDelaySec, bFlipFlop);
			dCarrierStart = WaveformGeneratorSquare(0.0, dFreq, dDelaySec, bFlipFlop);
			dFlipFlop = -2.0*bFlipFlop + 1.0;
		break;

		case eWAVSHAPE_RANDOM :
		case eWAVSHAPE_RANDOM_BEZIER :
			dSamplerate = 1.0/dFreq;
			dCarrierStart = WaveformGeneratorRandom(0.0, dFreq, dDelaySec);
			dCarrierEnd = WaveformGeneratorRandom(dLength, dFreq, dDelaySec);
		break;

		case eWAVSHAPE_SAWUP :
		case eWAVSHAPE_SAWUP_BEZIER :
			dSamplerate = 1.0/dFreq;
			dCarrierStart = WaveformGeneratorSawUp(0.0, dFreq, dDelaySec);
			dCarrierEnd = WaveformGeneratorSawUp(dLength, dFreq, dDelaySec);
			dFlipFlop = 1.0;
		break;

		case eWAVSHAPE_SAWDOWN :
		case eWAVSHAPE_SAWDOWN_BEZIER :
			dSamplerate = 1.0/dFreq;
			dCarrierStart = WaveformGeneratorSawDown(0.0, dFreq, dDelaySec);
			dCarrierEnd = WaveformGeneratorSawDown(dLength, dFreq, dDelaySec);
			dFlipFlop = -1.0;
		break;

		default:
			return;
		break;
	}

	double dValueStart = waveParams.offset + dMagnitude*dCarrierStart;
	dValueStart = dScale*dValueStart + dOff;
	double dValueEnd = waveParams.offset + dMagnitude*dCarrierEnd;
	dValueEnd = dScale*dValueEnd + dOff;

	sprintf(buffer, "PT %lf %lf %d\n", dStartTime, dValueStart, tEnvShape);
	envState.append(buffer);

//double dFreqMod = dFreq;
//freqModulator = new LfoWaveParams();
//freqModulator->freqHz = dFreq/100.0;
//freqModulator->strength = 0.6;

	switch(waveParams.shape)
	{
		case eWAVSHAPE_SINE :
		{
			for(double t = -dDelaySec; t<dLength; t += dSamplerate)
			{
				if(t>0.0)
				{
//if(freqModulator)
//{
//	double dFreqCarrier = WaveformGeneratorSawUp(t, freqModulator->freqHz, 1000.0*freqModulator->delayMsec);
//	dFreqMod = dFreq*(1.0 + freqModulator->strength * dFreqCarrier);
//	// Watch out for NaNs !!!
////	dSamplerate = dPrecision/dFreqMod;
//	dSamplerate = 0.01;
//}

					dValue = waveParams.offset + dMagnitude*WaveformGeneratorSin(t, dFreq, dDelaySec);
//dValue = waveParams.offset + dMagnitude*WaveformGeneratorSin(t, dFreqMod, dDelaySec);
					dValue = dScale*dValue + dOff;
					sprintf(buffer, "PT %lf %lf %d\n", t+dStartTime, dValue, tEnvShape);
					envState.append(buffer);
				}
			}
		}
		break;

		case eWAVSHAPE_TRIANGLE :
		case eWAVSHAPE_TRIANGLE_BEZIER :
		case eWAVSHAPE_SQUARE :
		{
			//for(double t = dStartTime-dDelaySec; t < dEndTime; t += dSamplerate)
			for(double t = -dDelaySec; t<dLength; t += dSamplerate)
			{
				//if(t<=dStartTime)
					//continue;
				if(t>0.0)
				{
					dValue = waveParams.offset + dMagnitude*dFlipFlop;
					dValue = dScale*dValue + dOff;
					sprintf(buffer, "PT %lf %lf %d\n", t+dStartTime, dValue, tEnvShape);
					envState.append(buffer);
					dFlipFlop = -dFlipFlop;
				}
			}
		}
		break;

		case eWAVSHAPE_SAWUP :
		case eWAVSHAPE_SAWDOWN :
		case eWAVSHAPE_SAWUP_BEZIER :
		case eWAVSHAPE_SAWDOWN_BEZIER :
		{
			for(double t = -dDelaySec; t<dLength; t += dSamplerate)
			{
				if(t>0.0)
				{
					for(int i=0; i<2; i++)
					{
						dValue = waveParams.offset + dMagnitude*dFlipFlop;
						dValue = dScale*dValue + dOff;
						sprintf(buffer, "PT %lf %lf %d\n", t+dStartTime, dValue, tEnvShape);
						envState.append(buffer);
						dFlipFlop = -dFlipFlop;
					}
				}
			}

			if(fabs(dCarrierEnd) > 0.99)
			{
				dValue = waveParams.offset + dMagnitude*dFlipFlop;
				dValueEnd = dScale*dValue + dOff;
			}
		}
		break;

		case eWAVSHAPE_RANDOM :
		case eWAVSHAPE_RANDOM_BEZIER :
		{
			for(double t = -dDelaySec; t<dLength; t += dSamplerate)
			{
				if(t>0.0)
				{
					dValue = waveParams.offset + dMagnitude*WaveformGeneratorRandom(t, dFreq, dDelaySec);
					dValue = dScale*dValue + dOff;
					sprintf(buffer, "PT %lf %lf %d\n", t+dStartTime, dValue, tEnvShape);
					envState.append(buffer);
				}
			}
		}
		break;

		default :
		break;
	}

	sprintf(buffer, "PT %lf %lf %d\n", dEndTime, dValueEnd, tEnvShape);
	envState.append(buffer);
}

EnvelopeProcessor::ErrorCode EnvelopeProcessor::processPoints(char* envState, string &newState, double dStartPos, double dEndPos, double dValMin, double dValMax, EnvModType envModType, double dStrength, double dOffset)
{
	//if(!envelope)
	//	return eERRORCODE_NOENVELOPE;

	if(dStartPos==dEndPos)
		return eERRORCODE_NULLTIMESELECTION;
	double dLength = dEndPos-dStartPos;

	double dEnvOffset = 0.5*(dValMin+dValMax);
	double dEnvMagnitude = 0.5*(dValMax-dValMin);

	//char* envState = GetSetObjectState(envelope, "");
	if(!envState)
		return eERRORCODE_NOOBJSTATE;

	//string newState;
	newState.clear();
	char cPtValue[318];

	double position, value;
	int iTmp;
	char* token = strtok(envState, "\n");

	while(token != NULL)
	{
		// Position, value, shape
		if(sscanf(token, "PT %lf %lf %d\n", &position, &value, &iTmp) == 3)
		{
			if(position>=dEndPos)
				break;
			if(position>dStartPos)
			{
				stringstream currentLine;
				currentLine << token;
				std::vector<string> lineTokens;
				while(!currentLine.eof())
				{
					string str;
					getline(currentLine, str, ' ');
					lineTokens.push_back(str);
				}

				string newLine;
				int linePos = 0;
				for(vector<string>::const_iterator it = lineTokens.begin(); it !=lineTokens.end(); ++it)
				{
					if(linePos == 2)
					{
						double dEnvNormValue = (value-dEnvOffset)/dEnvMagnitude;
						double dCarrier = 1.0;
						switch(envModType)
						{
							case eENVMOD_FADEIN :
								dCarrier = (position-dStartPos)/dLength;
								dCarrier = pow(dCarrier, dStrength);
								//dCarrier = EnvSignalProcessorFade((position-dStartPos), (dEndPos-dStartPos), true);
								dEnvNormValue = dCarrier*(dEnvNormValue - dOffset) + dOffset;
							break;
							case eENVMOD_FADEOUT :
								dCarrier = (dEndPos-position)/dLength;
								dCarrier = pow(dCarrier, dStrength);
								//dCarrier = EnvSignalProcessorFade((position-dStartPos), (dEndPos-dStartPos), false);
								dEnvNormValue = dCarrier*(dEnvNormValue - dOffset) + dOffset;
							break;
							case eENVMOD_AMPLIFY :
								dEnvNormValue = dStrength*(dEnvNormValue + dOffset);
							break;
							default :
							break;
						}

						value = dEnvMagnitude*dEnvNormValue + dEnvOffset;
						if(value < dValMin)
							value = dValMin;
						if(value > dValMax)
							value = dValMax;

						sprintf(cPtValue, "%lf", value);
						newLine.append(cPtValue);
					}

					else
						newLine.append(*it);

					newLine.append(" ");
					linePos++;
				}
				newState.append(newLine);
				newState.append("\n");

				token = strtok(NULL, "\n");
				continue;
			}
		}

		if(!strcmp(token, ">"))
			break;

		newState.append(token);
		newState.append("\n");
		token = strtok(NULL, "\n");
	}

	newState.append(token);
	newState.append("\n");
	token = strtok(NULL, "\n");
	while(token != NULL)
	{
		newState.append(token);
		newState.append("\n");
		token = strtok(NULL, "\n");
	}

//	FreeHeapPtr(envState);
//
//	if(GetSetObjectState(envelope, newState.c_str()))
//		return eERRORCODE_UNKNOWN;
//Undo_OnStateChangeEx("Envelope Processor", UNDO_STATE_ALL, -1);

	return eERRORCODE_OK;
}

EnvelopeProcessor::ErrorCode EnvelopeProcessor::generateTrackLfo(TrackEnvelope* envelope, double dStartPos, double dEndPos, LfoWaveParams &waveParams, double dPrecision)
{
	if(!envelope)
		return eERRORCODE_NOENVELOPE;

	if(dStartPos==dEndPos)
		return eERRORCODE_NULLTIMESELECTION;

	char* envState = GetSetObjectState(envelope, "");
	if(!envState)
		return eERRORCODE_NOOBJSTATE;

	string newState;
	double dValMin = 0.0;
	double dValMax = 1.0;
	double dTmp[2];
	int iTmp;

	//! \todo Use with GetMinMax()
	char* token = strtok(envState, "\n");
	while(token != NULL)
	{
		if(!strcmp(token, "<VOLENV") || !strcmp(token, "<VOLENV2"))
			dValMax = 2.0;

		if(!strcmp(token, "<PANENV") || !strcmp(token, "<PANENV2"))
			dValMin = -1.0;

		// VST index, min value, max value
		if(sscanf(token, "<PARMENV %d %lf %lf", &iTmp, &dTmp[0], &dTmp[1]) == 3)
		{
			dValMin = dTmp[0];
			dValMax = dTmp[1];
		}

		// Position, value, shape
		if(sscanf(token, "PT %lf %lf %d\n", &dTmp[0], &dTmp[1], &iTmp) == 3)
		{
			if(dTmp[0]>=dEndPos)
				break;
			if(dTmp[0]>dStartPos)
			{
				token = strtok(NULL, "\n");
				continue;
			}
		}

		if(!strcmp(token, ">"))
			break;

		newState.append(token);
		newState.append("\n");
		token = strtok(NULL, "\n");
	}

	writeLfoPoints(nullptr, newState, dStartPos, dEndPos, dValMin, dValMax, waveParams, dPrecision);

	newState.append(token);
	newState.append("\n");
	token = strtok(NULL, "\n");
	while(token != NULL)
	{
		newState.append(token);
		newState.append("\n");
		token = strtok(NULL, "\n");
	}

	FreeHeapPtr(envState);

	if(GetSetObjectState(envelope, newState.c_str()))
		return eERRORCODE_UNKNOWN;

/* JFB commented: leads to "recursive" undo point, enabled at top level
	Undo_OnStateChangeEx("Track Envelope LFO", UNDO_STATE_ALL, -1);
*/
	return eERRORCODE_OK;
}

EnvelopeProcessor::ErrorCode EnvelopeProcessor::generateSelectedTrackEnvLfo()
{
	TrackEnvelope* envelope = GetSelectedTrackEnvelope(0);
	if(!envelope)
		return eERRORCODE_NOENVELOPE;

	Undo_BeginBlock2(0);

	//! \todo: use insert/goto actions AFTER error returns
	double dStartPos, dEndPos;
	GetTimeSegmentPositions(_parameters.timeSegment, dStartPos, dEndPos);

	if(dStartPos==dEndPos)
		return eERRORCODE_NULLTIMESELECTION;

	double dOrgCursorPos = GetCursorPositionEx(0);
	SetEditCurPos2(0, dStartPos-EPSILON_TIME, false, false);
	Main_OnCommandEx(ID_ENVELOPE_INSERT_POINT, 0, 0);
	SetEditCurPos2(0, dEndPos+EPSILON_TIME, false, false);
	Main_OnCommandEx(ID_ENVELOPE_INSERT_POINT, 0, 0);
	SetEditCurPos2(0, dOrgCursorPos, false, false);

	//Main_OnCommandEx(ID_MOVE_TIMESEL_NUDGE_LEFTEDGE_LEFT, 0, 0);
	//Main_OnCommandEx(ID_MOVE_TIMESEL_NUDGE_RIGHTEDGE_RIGHT, 0, 0);
	//Main_OnCommandEx(ID_GOTO_TIMESEL_START, 0, 0);
	//Main_OnCommandEx(ID_ENVELOPE_INSERT_POINT, 0, 0);
	//Main_OnCommandEx(ID_GOTO_TIMESEL_END, 0, 0);
	//Main_OnCommandEx(ID_ENVELOPE_INSERT_POINT, 0, 0);
	//Main_OnCommandEx(ID_MOVE_TIMESEL_NUDGE_LEFTEDGE_RIGHT, 0, 0);
	//Main_OnCommandEx(ID_MOVE_TIMESEL_NUDGE_RIGHTEDGE_LEFT, 0, 0);
	//Main_OnCommandEx(ID_ENVELOPE_DELETE_ALL_POINTS_TIMESEL, 0, 0);

	ErrorCode res = generateTrackLfo(envelope, dStartPos, dEndPos, _parameters.waveParams, _parameters.precision);
//UpdateTimeline();

	Undo_EndBlock2(NULL, __LOCALIZE("Track envelope LFO","sws_undo"), UNDO_STATE_TRACKCFG);
	return res;
}

EnvelopeProcessor::ErrorCode EnvelopeProcessor::generateTakeLfo(MediaItem_Take* take, double dStartPos, double dEndPos, TakeEnvType tTakeEnvType, LfoWaveParams &waveParams, double dPrecision)
{
	double dValMin = 0.0;
	double dValMax = 1.0;
	bool bNeedUpdate = false;
	switch(tTakeEnvType)
	{
		case eTAKEENV_VOLUME :
			dValMax = 2.0;
			bNeedUpdate = ShowTakeEnvVol(take);
		break;
		case eTAKEENV_PAN :
			dValMin = -1.0;
			bNeedUpdate = ShowTakeEnvPan(take);
		break;
		case eTAKEENV_MUTE :
			bNeedUpdate = ShowTakeEnvMute(take);
		break;
		case eTAKEENV_PITCH:
			dValMax = (double)GetPitchTakeEnvRangeFromPrefs();
			dValMin = dValMax * (-1);
			bNeedUpdate = ShowTakeEnvPitch(take);
		break;
		default:
		break;
	}

	if(bNeedUpdate)
		UpdateTimeline();

	TrackEnvelope* envelope = SWS_GetTakeEnvelopeByName(take, GetTakeEnvelopeStr(tTakeEnvType));
	if(!envelope)
		return eERRORCODE_NOENVELOPE;

	//if(dStartPos==dEndPos)
	//	return eERRORCODE_NULLTIMESELECTION;

	string newState;
	string envStateStr;
	try {
		envStateStr = envelope::GetEnvelopeStateChunkBig(envelope);
	}
	catch (const envelope::bad_get_env_chunk_big &) {
		return eERRORCODE_NOOBJSTATE;
	}
	
	char* envState = &envStateStr[0];
	double dTmp[2];
	int iTmp;
	char* token = strtok(envState, "\n");
	while(token != NULL)
	{
		// Position, value, shape
		if(sscanf(token, "PT %lf %lf %d\n", &dTmp[0], &dTmp[1], &iTmp) == 3)
		{
			if(dTmp[0]>=dEndPos)
				break;
			if(dTmp[0]>dStartPos)
			{
				token = strtok(NULL, "\n");
				continue;
			}
		}

		if(!strcmp(token, ">"))
			break;

		newState.append(token);
		newState.append("\n");
		token = strtok(NULL, "\n");
	}

	writeLfoPoints(take, newState, dStartPos, dEndPos, dValMin, dValMax, waveParams, dPrecision);

	newState.append(token);
	newState.append("\n");
	token = strtok(NULL, "\n");
	while(token != NULL)
	{
		newState.append(token);
		newState.append("\n");
		token = strtok(NULL, "\n");
	}



//
//ShowConsoleMsg(envState);

/*
//double dCursorPos = GetCursorPositionEx(0);
	Main_OnCommandEx(ID_GOTO_SELITEM_END, 0, 0);
	double dEndTime = GetCursorPositionEx(0);
	Main_OnCommandEx(ID_GOTO_SELITEM_START, 0, 0);
	double dStartTime = GetCursorPositionEx(0);
//SetEditCurPos2(0, dCursorPos, true, false);

	if (dStartTime==dEndTime)
		return eERRORCODE_NULLTIMESELECTION;

	double dLength = dEndTime - dStartTime;
*/

// Doesn't work when item is looped
//PCM_source* source = GetMediaItemTake_Source(take);
//double dLength = source->GetLength();
//if(dLength <= 0.0)
//	return eERRORCODE_NULLTIMESELECTION;

/*
	string newState;
	newState.append("<TRACK_ENVELOPE_UNKNOWN\n");
	newState.append("ACT 1\n");
	newState.append("VIS 1 1 1.000000\n");
	newState.append("LANEHEIGHT 0 0\n");
	newState.append("ARM 1\n");
	newState.append("DEFSHAPE 0\n");
	//writeLfoPoints(newState, 0.0, dLength, dValMin, dValMax, dFreq, dStrength, dOffset, dDelay, tWaveShape, dPrecision);
writeLfoPoints(newState, dStartPos, dEndPos, dValMin, dValMax, dFreq, dStrength, dOffset, dDelay, tWaveShape, dPrecision);
	newState.append(">\n");
*/
	if(!newState.size() || !GetSetEnvelopeState(envelope, (char*)newState.c_str(), (int)newState.size()))
		return eERRORCODE_UNKNOWN;

/* JFB commented: "recursive" undo point, enabled at top level
	Undo_OnStateChangeEx("Take Envelope LFO", UNDO_STATE_ALL, -1);
*/
	return eERRORCODE_OK;
}

EnvelopeProcessor::ErrorCode EnvelopeProcessor::generateTakeLfo(MediaItem_Take* take)
{
	MediaItem* parentItem = GetMediaItemTake_Item(take);
	double dItemStartPos = GetMediaItemInfo_Value(parentItem, "D_POSITION");
	double dItemEndPos = dItemStartPos + GetMediaItemInfo_Value(parentItem, "D_LENGTH");

	double dStartPos, dEndPos;
	GetTimeSegmentPositions(_parameters.timeSegment, dStartPos, dEndPos, parentItem);

	if(dEndPos>dItemEndPos)
		dEndPos = dItemEndPos;

	if(dStartPos<dItemStartPos)
		dStartPos = dItemStartPos;

	dStartPos -= dItemStartPos;
	dEndPos -= dItemStartPos;

	return generateTakeLfo(take, dStartPos, dEndPos, _parameters.takeEnvType, _parameters.waveParams, _parameters.precision);
}

EnvelopeProcessor::ErrorCode EnvelopeProcessor::generateSelectedTakesLfo()
{
	list<MediaItem*> items;
	GetSelectedMediaItems(items);
	if(items.empty())
		return eERRORCODE_NOITEMSELECTED;

	Undo_BeginBlock2(NULL);

	for(list<MediaItem*>::iterator item = items.begin(); item != items.end(); item++)
	{
		list<MediaItem_Take*> takes;
		GetMediaItemTakes(*item, takes, _parameters.activeTakeOnly);
		//if(takes.empty())
		//	return eERRORCODE_NOITEMSELECTED;

		for(list<MediaItem_Take*>::iterator take = takes.begin(); take != takes.end(); take++)
		{
			ErrorCode res = generateTakeLfo(*take);
			UpdateItemInProject(*item);
			if(res != eERRORCODE_OK)
				return res;
		}
	}

	//Undo_OnStateChangeEx("Item Envelope LFO", UNDO_STATE_ALL, -1);
//	UpdateTimeline();
	Undo_EndBlock2(NULL, __LOCALIZE("Take envelope LFO","sws_undo"), UNDO_STATE_TRACKCFG);

	return eERRORCODE_OK;
}

EnvelopeProcessor::MidiCcRemover::MidiCcRemover(int* pMidiCc)
: MidiFilterBase(), _pMidiCc(pMidiCc)
{
}

void EnvelopeProcessor::MidiCcRemover::process(MIDI_event_t* evt, MIDI_eventlist* evts, int &curPos, int &nextPos, int itemLengthSamples)
{
	int statusByte = evt->midi_message[0] & 0xf0;
	//int midiChannel = evt->midi_message[0] & 0x0f;

	switch(statusByte)
	{
		case MIDI_CMD_CONTROL_CHANGE :
		{
			if(evt->midi_message[1] == *_pMidiCc)
			{
				evts->DeleteItem(curPos);
				nextPos = curPos;
			}
		}
		break;

		default :
		break;
	}
}

EnvelopeProcessor::MidiCcLfo::MidiCcLfo(EnvLfoParams* pParameters)
: MidiGeneratorBase(), _pParameters(pParameters)
{
}

void EnvelopeProcessor::MidiCcLfo::process(MIDI_eventlist* evts, int itemLengthSamples)
{
	//int iPrecision = (int)(MIDIITEMPROC_DEFAULT_SAMPLERATE/50.0);

	int midiChannel = 0;
	int statusByte = MIDI_CMD_CONTROL_CHANGE;

double dValMin = 0.0;
double dValMax = 1.0;
double dMagnitude = _pParameters->waveParams.strength * (1.0 - fabs(_pParameters->waveParams.offset));
double dOff = 0.5*(dValMax+dValMin);
double dScale = dValMax - dOff;
double dFreq, dDelay;
EnvelopeProcessor::getFreqDelay(_pParameters->waveParams, dFreq, dDelay);

	double (*WaveformGenerator)(double t, double dFreq, double dDelay);
	int iPrecision;
	switch(_pParameters->waveParams.shape)
	{
		case eWAVSHAPE_SINE :
			WaveformGenerator = &WaveformGeneratorSin;
			iPrecision = (int)(_pParameters->precision * MIDIITEMPROC_DEFAULT_SAMPLERATE/dFreq);
		break;
		case eWAVSHAPE_TRIANGLE :
			WaveformGenerator = &WaveformGeneratorTriangle;
			iPrecision = (int)(_pParameters->precision * MIDIITEMPROC_DEFAULT_SAMPLERATE/dFreq);
		break;
		case eWAVSHAPE_SQUARE :
			WaveformGenerator = &WaveformGeneratorSquare;
			iPrecision = (int)(_pParameters->precision * MIDIITEMPROC_DEFAULT_SAMPLERATE/dFreq);
		break;
		case eWAVSHAPE_RANDOM :
			WaveformGenerator = &WaveformGeneratorRandom;
			iPrecision = (int)(MIDIITEMPROC_DEFAULT_SAMPLERATE/dFreq);
		break;
		case eWAVSHAPE_SAWUP :
			WaveformGenerator = &WaveformGeneratorSawUp;
			iPrecision = (int)(_pParameters->precision * MIDIITEMPROC_DEFAULT_SAMPLERATE/dFreq);
		break;
		case eWAVSHAPE_SAWDOWN :
			WaveformGenerator = &WaveformGeneratorSawDown;
			iPrecision = (int)(_pParameters->precision * MIDIITEMPROC_DEFAULT_SAMPLERATE/dFreq);
		break;
		default:
			return;
		break;
	}

	if(iPrecision<1)
		iPrecision = 1;

	double t, dValue;
	int iValue;
	for(int pos = 0; pos<itemLengthSamples; pos += iPrecision)
	{
		t = (double)pos/MIDIITEMPROC_DEFAULT_SAMPLERATE;

		dValue = _pParameters->waveParams.offset + dMagnitude*WaveformGenerator(t, dFreq, 0.001*dDelay);
		dValue = dScale*dValue + dOff;
		iValue = (int)(127.0*dValue);

		MIDI_event_t evt;
		evt.frame_offset = pos;
		evt.size = 3;
		evt.midi_message[0] = midiChannel | statusByte;
		evt.midi_message[1] = _pParameters->midiCc;
		evt.midi_message[2] = iValue;
		evts->AddItem(&evt);
	}
}

EnvelopeProcessor::ErrorCode EnvelopeProcessor::generateSelectedMidiTakeLfo()
{
	_midiProcessor->processSelectedMidiTakes(_parameters.activeTakeOnly);
	return eERRORCODE_OK;
}

EnvelopeProcessor::ErrorCode EnvelopeProcessor::processSelectedTrackEnv()
{
	TrackEnvelope* envelope = GetSelectedTrackEnvelope(0);
	if(!envelope)
		return eERRORCODE_NOENVELOPE;

	Undo_BeginBlock2(0);

	double dStartPos, dEndPos;
	GetTimeSegmentPositions(_envModParams.timeSegment, dStartPos, dEndPos);

	if(dStartPos==dEndPos)
		return eERRORCODE_NULLTIMESELECTION;

	double dValMin, dValMax;
	ErrorCode res = getTrackEnvelopeMinMax(envelope, dValMin, dValMax);
	if(res != eERRORCODE_OK)
		return res;

	char* envState = GetSetObjectState(envelope, "");
	string newState;
	res = processPoints(envState, newState, dStartPos, dEndPos, dValMin, dValMax, _envModParams.type, _envModParams.strength, _envModParams.offset);
	if(GetSetObjectState(envelope, newState.c_str()))
		res = eERRORCODE_UNKNOWN;
	else
		FreeHeapPtr(envState);

//UpdateTimeline();
/* JFB "recursive" undo point, enabled at top level
	Undo_OnStateChangeEx("Envelope Processor", UNDO_STATE_ALL, -1);
*/
	Undo_EndBlock2(NULL, __LOCALIZE("Track envelope processor","sws_undo"), UNDO_STATE_TRACKCFG);
	return res;
}

EnvelopeProcessor::ErrorCode EnvelopeProcessor::processSelectedTakes()
{
	ErrorCode res = eERRORCODE_OK;
	list<MediaItem*> items;
	GetSelectedMediaItems(items);
	if(items.empty())
		return eERRORCODE_NOITEMSELECTED;

	Undo_BeginBlock2(0);

	for(list<MediaItem*>::iterator item = items.begin(); res == eERRORCODE_OK && item != items.end(); item++)
	{
		list<MediaItem_Take*> takes;
		GetMediaItemTakes(*item, takes, _envModParams.activeTakeOnly);
		//if(takes.empty())
		//	return eERRORCODE_NOITEMSELECTED;

		for(list<MediaItem_Take*>::iterator take = takes.begin(); take != takes.end(); take++)
		{
			res = processTakeEnv(*take);
			UpdateItemInProject(*item);
			if (res != eERRORCODE_OK)
				break;
		}
	}

	//Undo_OnStateChangeEx("Item Envelope LFO", UNDO_STATE_ALL, -1);
//	UpdateTimeline();
	Undo_EndBlock2(NULL, __LOCALIZE("Take envelope processor","sws_undo"), UNDO_STATE_TRACKCFG);

	return eERRORCODE_OK;
}

EnvelopeProcessor::ErrorCode EnvelopeProcessor::processTakeEnv(MediaItem_Take* take)
{
	MediaItem* parentItem = GetMediaItemTake_Item(take);
	double dItemStartPos = GetMediaItemInfo_Value(parentItem, "D_POSITION");
	double dItemEndPos = dItemStartPos + GetMediaItemInfo_Value(parentItem, "D_LENGTH");

	double dStartPos, dEndPos;
	GetTimeSegmentPositions(_envModParams.timeSegment, dStartPos, dEndPos, parentItem);

	if(dEndPos>dItemEndPos)
		dEndPos = dItemEndPos;

	if(dStartPos<dItemStartPos)
		dStartPos = dItemStartPos;

	dStartPos -= dItemStartPos;
	dEndPos -= dItemStartPos;

	return processTakeEnv(take, dStartPos, dEndPos, _envModParams.takeEnvType, _envModParams.type, _envModParams.strength, _envModParams.offset);
}

EnvelopeProcessor::ErrorCode EnvelopeProcessor::processTakeEnv(MediaItem_Take* take, double dStartPos, double dEndPos, TakeEnvType tTakeEnvType, EnvModType envModType, double dStrength, double dOffset)
{
	double dValMin = 0.0;
	double dValMax = 1.0;
	bool bNeedUpdate = false;
	switch(tTakeEnvType)
	{
		case eTAKEENV_VOLUME :
			dValMax = 2.0;
			bNeedUpdate = ShowTakeEnvVol(take);
		break;
		case eTAKEENV_PAN :
			dValMin = -1.0;
			bNeedUpdate = ShowTakeEnvPan(take);
		break;
		case eTAKEENV_MUTE :
			bNeedUpdate = ShowTakeEnvMute(take);
		break;
		case eTAKEENV_PITCH :
			dValMax = (double)GetPitchTakeEnvRangeFromPrefs();
			dValMin = dValMax * (-1);
			bNeedUpdate = ShowTakeEnvPitch(take);
		break;
		default:
		break;
	}

	if(bNeedUpdate)
		UpdateTimeline();

	TrackEnvelope* envelope = SWS_GetTakeEnvelopeByName(take, GetTakeEnvelopeStr(tTakeEnvType));
	if(!envelope)
		return eERRORCODE_NOENVELOPE;

	string envStateStr = envelope::GetEnvelopeStateChunkBig(envelope);
	char* envState = &envStateStr[0];
	string newState;
	ErrorCode res = processPoints(envState, newState, dStartPos, dEndPos, dValMin, dValMax, envModType, dStrength, dOffset);

	if(!newState.size() || !GetSetEnvelopeState(envelope, (char*)newState.c_str(), (int)newState.size()))
		res = eERRORCODE_UNKNOWN;

/*JFB
	Undo_OnStateChangeEx("Take Envelope LFO", UNDO_STATE_ALL, -1);
*/
	return res;
}
