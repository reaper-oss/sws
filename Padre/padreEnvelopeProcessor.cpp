/******************************************************************************
/ padreEnvelopeProcessor.cpp
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

#include "stdafx.h"
#include "padreEnvelopeProcessor.h"

const char* GetEnvTypeStr(EnvType type)
{
	switch(type)
	{
		case eENVTYPE_TRACK			: return "Selected Track Envelope";		break;
		case eENVTYPE_TAKE			: return "Selected Take(s)";			break;
		case eENVTYPE_MIDICC		: return "Selected Take(s) (MIDI)";		break;
		default						: return NULL;							break;
	}
}

const char* GetEnvModTypeStr(EnvModType type)
{
	switch(type)
	{
		case eENVMOD_FADEIN			: return "Fade In";		break;
		case eENVMOD_FADEOUT		: return "Fade Out";	break;
		case eENVMOD_AMPLIFY		: return "Amplify";		break;
		default						: return NULL;			break;
	}
}

EnvLfoParams::EnvLfoParams()
: waveShape(eWAVSHAPE_SINE), freqBeat(eGRID_1_1), freqHz(0.0), delayBeat(eGRID_OFF), delayMsec(0.0), strength(1.0), offset(0.0), precision(0.05)
, midiCc(7), takeEnvType(eTAKEENV_VOLUME), envType(eENVTYPE_TRACK), timeSegment(eTIMESEGMENT_TIMESEL), activeTakeOnly(true)
{
}

EnvLfoParams& EnvLfoParams::operator=(const EnvLfoParams &parameters)
{
	this->waveShape = parameters.waveShape;
	this->freqBeat = parameters.freqBeat;
	this->freqHz = parameters.freqHz;
	this->delayBeat = parameters.delayBeat;
	this->delayMsec = parameters.delayMsec;
	this->strength = parameters.strength;
	this->offset = parameters.offset;
	this->precision = parameters.precision;
	this->midiCc = parameters.midiCc;
	this->envType = parameters.envType;
	this->takeEnvType = parameters.takeEnvType;
	this->timeSegment = parameters.timeSegment;
	this->activeTakeOnly = parameters.activeTakeOnly;
		return *this;
}

EnvModParams::EnvModParams()
: type(eENVMOD_FADEIN), offset(0.0), strength(1.0)
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
			MessageBox(hwnd, "No envelope selected!", "Error", MB_OK);
		break;
		case EnvelopeProcessor::eERRORCODE_NULLTIMESELECTION:
			MessageBox(hwnd, "No time selection!", "Error", MB_OK);
		break;
		case EnvelopeProcessor::eERRORCODE_NOITEMSELECTED:
			MessageBox(hwnd, "No item selected!", "Error", MB_OK);
		break;
		case EnvelopeProcessor::eERRORCODE_NOOBJSTATE:
			MessageBox(hwnd, "Could not retrieve envelope object state!", "Error", MB_OK);
		break;
		case EnvelopeProcessor::eERRORCODE_UNKNOWN:
			MessageBox(hwnd, "Could not generate envelope! (Unknown Error)", "Error", MB_OK);
		break;
		default:
		break;
	}
}


void EnvelopeProcessor::getFreqDelay(EnvLfoParams &lfoParams, double &dFreq, double &dDelay)
{
	double tempoBpm, tempoBpi;
	GetProjectTimeSignature2(0, &tempoBpm, &tempoBpi);

	if(lfoParams.freqBeat != eGRID_OFF)
		dFreq = GetGridDivisionFactor(lfoParams.freqBeat)*tempoBpm/60.0;
	else
		dFreq = lfoParams.freqHz;

	if(lfoParams.delayBeat != eGRID_OFF)
		dDelay = 1000.0*60.0/tempoBpm/GetGridDivisionFactor(lfoParams.delayBeat);
	else
		dDelay = lfoParams.delayMsec;
}

EnvelopeProcessor::ErrorCode EnvelopeProcessor::getEnvelopeMinMax(TrackEnvelope* envelope, double &dEnvMinVal, double &dEnvMaxVal)
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
			dEnvMaxVal = 2.0;

		// Track envelope: Pan
		if(!strcmp(it->c_str(), "<PANENV") || !strcmp(it->c_str(), "<PANENV2"))
			dEnvMinVal = -1.0;

		// Track envelope: Param (VST index, min value, max value)
		if(sscanf(it->c_str(), "<PARMENV %d %lf %lf", &iTmp, &dTmp[0], &dTmp[1]) == 3)
		{
			dEnvMinVal = dTmp[0];
			dEnvMaxVal = dTmp[1];
		}

		if(!strcmp(it->c_str(), ">"))
			break;
	}

	FreeHeapPtr(envState);
	return eERRORCODE_OK;
}

void EnvelopeProcessor::writeLfoPoints(string &envState, double dStartTime, double dEndTime, double dValMin, double dValMax, double dFreq, double dStrength, double dOffset, double dDelay, WaveShape tWaveShape, double dPrecision)
{
	double dMagnitude = dStrength * (1.0 - fabs(dOffset));
	double dOff = 0.5*(dValMax+dValMin);
	double dScale = dValMax - dOff;
	double dSamplerate;
	double dValue = 0.0;
	char buffer[BUFFER_SIZE];

	EnvShape tEnvShape = eENVSHAPE_LINEAR;
	switch(tWaveShape)
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

	switch(tWaveShape)
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

	double dValueStart = dOffset + dMagnitude*dCarrierStart;
	dValueStart = dScale*dValueStart + dOff;
	double dValueEnd = dOffset + dMagnitude*dCarrierEnd;
	dValueEnd = dScale*dValueEnd + dOff;

	sprintf(buffer, "PT %lf %lf %d\n", dStartTime, dValueStart, tEnvShape);
	envState.append(buffer);

	switch(tWaveShape)
	{
		case eWAVSHAPE_SINE :
		{
			for(double t = -dDelaySec; t<dLength; t += dSamplerate)
			{
				if(t>0.0)
				{
					dValue = dOffset + dMagnitude*WaveformGeneratorSin(t, dFreq, dDelaySec);
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
					dValue = dOffset + dMagnitude*dFlipFlop;
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
						dValue = dOffset + dMagnitude*dFlipFlop;
						dValue = dScale*dValue + dOff;
						sprintf(buffer, "PT %lf %lf %d\n", t+dStartTime, dValue, tEnvShape);
						envState.append(buffer);
						dFlipFlop = -dFlipFlop;
					}
				}
			}

			if(fabs(dCarrierEnd) > 0.99)
			{
				dValue = dOffset + dMagnitude*dFlipFlop;
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
					dValue = dOffset + dMagnitude*WaveformGeneratorRandom(t, dFreq, dDelaySec);
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

EnvelopeProcessor::ErrorCode EnvelopeProcessor::processPoints(TrackEnvelope* envelope, double dStartPos, double dEndPos, EnvModType envModType, double dStrength, double dOffset)
{
	if(!envelope)
		return eERRORCODE_NOENVELOPE;

	if(dStartPos==dEndPos)
		return eERRORCODE_NULLTIMESELECTION;
	double dLength = dEndPos-dStartPos;

	double dEnvMin, dEnvMax;
	ErrorCode res = getEnvelopeMinMax(envelope, dEnvMin, dEnvMax);
	if(res != eERRORCODE_OK)
		return res;
	double dEnvOffset = 0.5*(dEnvMin+dEnvMax);
	double dEnvMagnitude = 0.5*(dEnvMax-dEnvMin);

	char* envState = GetSetObjectState(envelope, "");
	if(!envState)
		return eERRORCODE_NOOBJSTATE;

	string newState;
	char cPtValue[128];

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
						}

						value = dEnvMagnitude*dEnvNormValue + dEnvOffset;
						if(value < dEnvMin)
							value = dEnvMin;
						if(value > dEnvMax)
							value = dEnvMax;

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

	FreeHeapPtr(envState);
	
	if(GetSetObjectState(envelope, newState.c_str()))
		return eERRORCODE_UNKNOWN;
	

	return eERRORCODE_OK;
}




EnvelopeProcessor::ErrorCode EnvelopeProcessor::generateTrackLfo(TrackEnvelope* envelope, double dStartPos, double dEndPos, double dFreq, double dStrength, double dOffset, double dDelay, WaveShape tWaveShape, double dPrecision)
{
	//TrackEnvelope* envelope = GetSelectedTrackEnvelope(0);
	if(!envelope)
		return eERRORCODE_NOENVELOPE;

//double dTimeSelStartPosition, dTimeSelEndPosition;
//GetTimeSegmentPositions(timeSegment, dTimeSelStartPosition, dTimeSelEndPosition);

	//Main_OnCommandEx(ID_GOTO_TIMESEL_END, 0, 0);
	//double dTimeSelEndPosition = GetCursorPositionEx(0);
	//Main_OnCommandEx(ID_GOTO_TIMESEL_START, 0, 0);
	//double dTimeSelStartPosition = GetCursorPositionEx(0);

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

	//ShowConsoleMsgEx("MIN = %lf, MAX = %lf\n", dValMin, dValMax);
	//ShowConsoleMsg(newState);

	writeLfoPoints(newState, dStartPos, dEndPos, dValMin, dValMax, dFreq, dStrength, dOffset, dDelay, tWaveShape, dPrecision);

	newState.append(token);
	newState.append("\n");
	token = strtok(NULL, "\n");
	while(token != NULL)
	{
		newState.append(token);
		newState.append("\n");
		token = strtok(NULL, "\n");
	}

//	ShowConsoleMsg(newState);

	FreeHeapPtr(envState);

	if(GetSetObjectState(envelope, newState.c_str()))
		return eERRORCODE_UNKNOWN;

	//Undo_OnStateChangeEx("Generate Track LFO", UNDO_STATE_ALL, -1);
	return eERRORCODE_OK;
}

EnvelopeProcessor::ErrorCode EnvelopeProcessor::generateSelectedTrackEnvLfo()
{
	double dFreq, dDelay;
	getFreqDelay(_parameters, dFreq, dDelay);

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

	ErrorCode res = generateTrackLfo(envelope, dStartPos, dEndPos, dFreq, _parameters.strength, _parameters.offset, dDelay, _parameters.waveShape, _parameters.precision);
//UpdateTimeline();

	Undo_EndBlock2(0, "Track Envelope LFO", 0);
	return res;
}

EnvelopeProcessor::ErrorCode EnvelopeProcessor::generateTakeLfo(MediaItem_Take* take, double dStartPos, double dEndPos, TakeEnvType tTakeEnvType, double dFreq, double dStrength, double dOffset, double dDelay, WaveShape tWaveShape, double dPrecision)
{
	TrackEnvelope* envelope = GetTakeEnvelopeByName(take, GetTakeEnvelopeStr(tTakeEnvType));
	double dValMin = 0.0;
	double dValMax = 1.0;
	//! \todo Force "show item vol/pan/mute envelope" (toggle trick doesn't work with existing/hidden envelopes)
	switch(tTakeEnvType)
	{
		case eTAKEENV_VOLUME :
			dValMax = 2.0;
			if(!envelope)
				Main_OnCommandEx(ID_TAKEENV_VOLUME_TOGGLE, 0, 0);
		break;
		case eTAKEENV_PAN :
			dValMin = -1.0;
			if(!envelope)
				Main_OnCommandEx(ID_TAKEENV_PAN_TOGGLE, 0, 0);
		break;
		case eTAKEENV_MUTE :
			if(!envelope)
				Main_OnCommandEx(ID_TAKEENV_MUTE_TOGGLE, 0, 0);
		break;
		default:
		break;
	}

	envelope = GetTakeEnvelopeByName(take, GetTakeEnvelopeStr(tTakeEnvType));
	if(!envelope)
		return eERRORCODE_NOENVELOPE;

	//if(dStartPos==dEndPos)
	//	return eERRORCODE_NULLTIMESELECTION;

	string newState;
	char* envState = PadresGetEnvelopeState(envelope);
	if(!envState)
		return eERRORCODE_NOOBJSTATE;
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

	writeLfoPoints(newState, dStartPos, dEndPos, dValMin, dValMax, dFreq, dStrength, dOffset, dDelay, tWaveShape, dPrecision);

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

	//ShowConsoleMsg(newState.c_str());
	char* cNewState = new char[newState.size()];
	strcpy(cNewState, newState.c_str());

	if(!GetSetEnvelopeState(envelope, cNewState, (int)newState.size()))
		return eERRORCODE_UNKNOWN;

//! \bug I can't/don't have to delete cNewState ???
//delete[] cNewState;
//FreeHeapPtr(cNewState);

	return eERRORCODE_OK;
}

EnvelopeProcessor::ErrorCode EnvelopeProcessor::generateTakeLfo(MediaItem_Take* take)
{
	double dFreq, dDelay;
	getFreqDelay(_parameters, dFreq, dDelay);

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

	return generateTakeLfo(take, dStartPos, dEndPos, _parameters.takeEnvType, dFreq, _parameters.strength, _parameters.offset, dDelay, _parameters.waveShape, _parameters.precision);
}

EnvelopeProcessor::ErrorCode EnvelopeProcessor::generateSelectedTakesLfo()
{
	list<MediaItem*> items;
	GetSelectedMediaItems(items);
	if(items.empty())
		return eERRORCODE_NOITEMSELECTED;

	Undo_BeginBlock2(0);

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
	Undo_EndBlock2(0, "Take Envelope LFO", 0);

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
double dMagnitude = _pParameters->strength * (1.0 - fabs(_pParameters->offset));
double dOff = 0.5*(dValMax+dValMin);
double dScale = dValMax - dOff;
double dFreq, dDelay;
EnvelopeProcessor::getFreqDelay(*_pParameters, dFreq, dDelay);

	double (*WaveformGenerator)(double t, double dFreq, double dDelay);
	int iPrecision;
	switch(_pParameters->waveShape)
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

//dValue = _pParameters->offset + dMagnitude*WaveformGeneratorSin(t, dFreq, 0.001*dDelay);
dValue = _pParameters->offset + dMagnitude*WaveformGenerator(t, dFreq, 0.001*dDelay);
dValue = dScale*dValue + dOff;
iValue = (int)(127.0*dValue);

		//double dValue = dOffset + dMagnitude*sin(2.0*3.14*_freq*t);
		//int iValue = (int)(127.0 * 0.5*(dValue + 1.0));
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
	Undo_BeginBlock2(0);

	TrackEnvelope* envelope = GetSelectedTrackEnvelope(0);
	if(!envelope)
		return eERRORCODE_NOENVELOPE;

	Main_OnCommandEx(ID_GOTO_TIMESEL_END, 0, 0);
	double dEndPos = GetCursorPositionEx(0);
	Main_OnCommandEx(ID_GOTO_TIMESEL_START, 0, 0);
	double dStartPos = GetCursorPositionEx(0);

	Undo_EndBlock2(0, "Padre's Envelope Processor", 0);

	return processPoints(envelope, dStartPos, dEndPos, _envModParams.type, _envModParams.strength, _envModParams.offset);
}


