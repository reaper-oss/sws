/******************************************************************************
/ padreMidiItemGenerators.cpp
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
#include "padreMidiItemGenerators.h"

MidiGenControlChangeLfo::MidiGenControlChangeLfo()
: MidiGeneratorBase()
{
}

MidiGenControlChangeLfo::MidiGenControlChangeLfo(int cc, double freq, double strength, double offset)
: MidiGeneratorBase(), _cc(cc), _freq(freq), _strength(strength), _offset(offset)
{
}

void MidiGenControlChangeLfo::process(MIDI_eventlist* evts, int itemLengthSamples)
{
	int iPrecision = (int)(MIDIITEMPROC_DEFAULT_SAMPLERATE/50.0);

	double dOffset = 0.0;		// -1.0 to 1.0
	double dStrength = 1.0;		// 0.0 to 1.0
	//int iControlChange = MIDI_CC064_SUSTAIN;
	int midiChannel = 0;

	int statusByte = MIDI_CMD_CONTROL_CHANGE;
	double dMagnitude = dStrength * (1.0 - fabs(dOffset));

	for(int pos = 0; pos<itemLengthSamples; pos += iPrecision)
	{
		double t = (double)pos/MIDIITEMPROC_DEFAULT_SAMPLERATE;
		double dValue = dOffset + dMagnitude*sin(2.0*3.14*_freq*t);
		int iValue = (int)(127.0 * 0.5*(dValue + 1.0));
		MIDI_event_t evt;
		evt.frame_offset = pos;
		evt.size = 3;
		evt.midi_message[0] = midiChannel | statusByte;
		evt.midi_message[1] = _cc;
		evt.midi_message[2] = iValue;
		evts->AddItem(&evt);
	}
}



