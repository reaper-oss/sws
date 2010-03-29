/******************************************************************************
/ padreMidiItemProcBase.h
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

#include "padreUtils.h"

//#include <stdio.h>
//#include <math.h>
//
//#include <vector>
//#include <set>
//#include <list>
//#include <map>
//using namespace std;
//
//class ReaProject;
//class TrackEnvelope;
//#include "reaper_plugin.h"
//#include "reaper_plugin_functions.h"
//
//#include <stdarg.h>
//void ShowConsoleMsgEx(const char* format, ...);
//
//#include "midi.h"
//
//#define BUFFER_SIZE		2048
//
//#define		MIDIITEMPROC_DEFAULT_SAMPLERATE		48000.0
//#define		MIDIITEMPROC_DEFAULT_NBCHANNELS		2

//static map<MIDI_event_t*, bool> selectedNotes;

struct MidiNoteKey
{
	int _frameOffset;
	unsigned char _status;
	unsigned char _data1;

	MidiNoteKey(int frameOffset, unsigned char status, unsigned char data1);
	MidiNoteKey(MIDI_event_t &evt);

	bool operator==(const MidiNoteKey& other) const;
	bool operator<(const MidiNoteKey& other) const;
};

class MidiFilterBase
{
	protected:
		MidiFilterBase();

	public:
		virtual ~MidiFilterBase();

		virtual void process(MIDI_event_t* evt, MIDI_eventlist* evts, int &curPos, int &nextPos, int itemLengthSamples = -1) = 0;
};

class MidiGeneratorBase
{
	protected:
		MidiGeneratorBase();

	public:
		virtual ~MidiGeneratorBase();

		virtual void process(MIDI_eventlist* evts, int itemLengthSamples) = 0;
};

class MidiItemProcessor
{
	public:
		enum MidiItemType { MIDI_ITEM_INPROJECT, MIDI_ITEM_IGNTEMPO, MIDI_ITEM_FILE };

	private:
		string _name;
		vector<MidiFilterBase*> _filters;
		vector<MidiGeneratorBase*> _generators;

		MidiItemProcessor();

		void clearFilters();
		void clearGenerators();

		void filterMidiEvents(MIDI_eventlist* evts, int itemLengthSamples);
		void generateMidiEvents(MIDI_eventlist* evts, int itemLengthSamples);
		void processTake(MediaItem_Take* take);

	public:
		static bool getMidiEventsList(MediaItem_Take* take, MIDI_eventlist* evts);
		static void getSelectedMediaItems(list<MediaItem*> &items);
		static bool isMidiTake(MediaItem_Take* take);
		static void getMediaItemTakes(MediaItem* item, list<MediaItem_Take*> &takes, bool bMidiOnly);
		static MidiItemType getMidiItemType(MediaItem* item);
		static void getSelectedMidiNotes(MediaItem* item, MIDI_eventlist* evts, map<MIDI_event_t*, bool> &selectedNotes);

	public:
		MidiItemProcessor(const char* name);
		virtual ~MidiItemProcessor();

		void rename(const char* name);
		void addFilter(MidiFilterBase* filter);
		void addGenerator(MidiGeneratorBase* generator);
		void clear();

		void processSelectedMidiTakes(bool bActiveOnly = true);
};

