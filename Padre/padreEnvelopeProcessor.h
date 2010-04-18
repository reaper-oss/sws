/******************************************************************************
/ padreEnvelopeProcessor.h
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

using namespace std;

enum EnvType { eENVTYPE_TRACK=0, eENVTYPE_TAKE=1, eENVTYPE_MIDICC=2 };
enum EnvModType { eENVMOD_FADEIN, eENVMOD_FADEOUT, eENVMOD_AMPLIFY, eENVMOD_LAST };

const char* GetEnvTypeStr(EnvType type);
const char* GetEnvModTypeStr(EnvModType type);

struct EnvLfoParams
{
	WaveShape waveShape;
	GridDivision freqBeat;
	double freqHz;
	GridDivision delayBeat;
	double delayMsec;
	double strength;
	double offset;
	double precision;
	int midiCc;
	TakeEnvType takeEnvType;
	EnvType envType;
	TimeSegment timeSegment;
	bool activeTakeOnly;

	EnvLfoParams();
	EnvLfoParams& operator=(const EnvLfoParams &parameters);
};

struct EnvModParams
{
	EnvModType type;
	double offset;
	double strength;

	EnvModParams();
	EnvModParams& operator=(const EnvModParams &params);
};

class EnvelopeProcessor
{
	private:
		static EnvelopeProcessor* _pInstance;

		EnvelopeProcessor();
		~EnvelopeProcessor();

		class MidiCcRemover : public MidiFilterBase
		{
			private:
				int *_pMidiCc;

			public:
				MidiCcRemover(int* pMidiCc);
				virtual void process(MIDI_event_t* evt, MIDI_eventlist* evts, int &curPos, int &nextPos, int itemLengthSamples);
		};

		class MidiCcLfo : public MidiGeneratorBase
		{
			private:
				EnvLfoParams* _pParameters;

			public:
				MidiCcLfo(EnvLfoParams* pParameters);

				void process(MIDI_eventlist* evts, int itemLengthSamples);
		};

	public:
		static EnvelopeProcessor* getInstance();

		enum ErrorCode { eERRORCODE_OK = 0, eERRORCODE_NOENVELOPE, eERRORCODE_NULLTIMESELECTION, eERRORCODE_NOOBJSTATE, eERRORCODE_NOITEMSELECTED, eERRORCODE_UNKNOWN };
		static void errorHandlerDlg(HWND hwnd, ErrorCode err);

		EnvLfoParams _parameters;
		EnvModParams _envModParams;
		MidiItemProcessor* _midiProcessor;

		void createMidiProcessor();
		void destroyMidiProcessor();

		ErrorCode generateSelectedTrackEnvLfo();
		ErrorCode generateSelectedTakesLfo();
ErrorCode processSelectedTrackEnv();

		ErrorCode generateSelectedMidiTakeLfo();



	protected:
		static void getFreqDelay(EnvLfoParams &lfoParams, double &dFreq, double &dDelay);
		static ErrorCode getEnvelopeMinMax(TrackEnvelope* envelope, double &dEnvMinVal, double &dEnvMaxVal);
		static void writeLfoPoints(string &envState, double dStartTime, double dEndTime, double dValMin, double dValMax, double dFreq, double dStrength = 1.0, double dOffset = 0.0, double dDelay = 0.0, WaveShape tWaveShape = eWAVSHAPE_SINE, double dPrecision = 0.1);
		static ErrorCode processPoints(TrackEnvelope* envelope, double dStartPos, double dEndPos, EnvModType envModType, double dStrength = 1.0, double dOffset = 0.0);

		static ErrorCode generateTrackLfo(TrackEnvelope* envelope, double dStartPos, double dEndPos, double dFreq, double dStrength = 1.0, double dOffset = 0.0, double dDelay = 0.0, WaveShape tWaveShape = eWAVSHAPE_SINE, double dPrecision = 0.1);
		static ErrorCode generateTakeLfo(MediaItem_Take* take, double dStartPos, double dEndPos, TakeEnvType tTakeEnvType, double dFreq, double dStrength = 1.0, double dOffset = 0.0, double dDelay = 0.0, WaveShape tWaveShape = eWAVSHAPE_SINE, double dPrecision = 0.1);

		ErrorCode generateTakeLfo(MediaItem_Take* take);







};

