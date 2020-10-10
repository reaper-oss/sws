/******************************************************************************
/ padreMidiItemProcBase.cpp
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

MidiNoteKey::MidiNoteKey(int frameOffset, unsigned char status, unsigned char data1)
: _frameOffset(frameOffset), _status(status), _data1(data1)
{
}

MidiNoteKey::MidiNoteKey(MIDI_event_t &evt)
: _frameOffset(evt.frame_offset), _status(evt.midi_message[0]), _data1(evt.midi_message[1])
{
}

bool MidiNoteKey::operator==(const MidiNoteKey& other) const
{
	return (memcmp(this, &other, sizeof(MidiNoteKey)) == 0);
}

bool MidiNoteKey::operator<(const MidiNoteKey& other) const
{
	if(this->_frameOffset < other._frameOffset)
		return true;

	if(this->_frameOffset == other._frameOffset)
	{
		if(this->_status < other._status)
			return true;

		if(this->_status == other._status)
		{
			if(this->_data1 < other._data1)
				return true;
			return false;
		}

		return false;
	}

	return false;
}

MidiFilterBase::MidiFilterBase()
{
}

MidiFilterBase::~MidiFilterBase()
{
}

MidiGeneratorBase::MidiGeneratorBase()
{
}

MidiGeneratorBase::~MidiGeneratorBase()
{
}

MidiItemProcessor::MidiItemProcessor()
{
}

MidiItemProcessor::MidiItemProcessor(const char* name) : _name(name)
{
}

MidiItemProcessor::~MidiItemProcessor()
{
	clear();
}

void MidiItemProcessor::rename(const char* name)
{
	_name = name;
}

void MidiItemProcessor::addFilter(MidiFilterBase* filter)
{
	_filters.push_back(filter);
}

void MidiItemProcessor::addGenerator(MidiGeneratorBase* generator)
{
	_generators.push_back(generator);
}

void MidiItemProcessor::clearFilters()
{
	for(vector<MidiFilterBase*>::iterator filter = _filters.begin(); filter != _filters.end(); filter++)
	{
		if(!*filter)
		{
			delete *filter;
			*filter = NULL;
		}
	}
	_filters.clear();
}

void MidiItemProcessor::clearGenerators()
{
	for(vector<MidiGeneratorBase*>::iterator generator = _generators.begin(); generator != _generators.end(); generator++)
	{
		if(!*generator)
		{
			delete *generator;
			*generator = NULL;
		}
	}
	_generators.clear();
}

void MidiItemProcessor::clear()
{
	clearGenerators();
	clearFilters();
}

//JFB TODO: support for playrate, forced source BPM, etc..
bool MidiItemProcessor::getMidiEventsList(MediaItem_Take* take, MIDI_eventlist* evts)
{
	PCM_source* source = take ? GetMediaItemTake_Source(take) : NULL;
	MediaItem* item = take ? GetMediaItemTake_Item(take) : NULL;
	if(!source || !item)
		return false;

	double itemLength = *(double*)GetSetMediaItemInfo(item, "D_LENGTH", NULL);
	double startOffs = *(double*)GetSetMediaItemTakeInfo(take, "D_STARTOFFS", NULL);

	PCM_source_transfer_t transferBlock;
	transferBlock.time_s						= startOffs;
	transferBlock.samplerate					= MIDIITEMPROC_DEFAULT_SAMPLERATE;
	transferBlock.nch							= 2;
	transferBlock.length						= (int)(transferBlock.samplerate * itemLength);
	transferBlock.samples						= NULL;
	transferBlock.samples_out					= 0;
	transferBlock.midi_events					= evts;
	transferBlock.approximate_playback_latency	= 0.0;
	transferBlock.absolute_time_s				= 0.0;
	transferBlock.force_bpm						= 0.0;

	source->Extended(PCM_SOURCE_EXT_GETRAWMIDIEVENTS, &transferBlock, NULL, NULL);
	return true;
}

void MidiItemProcessor::getSelectedMediaItems(list<MediaItem*> &items)
{
	int itemIdx = 0;
	while(MediaItem* item = GetSelectedMediaItem(0, itemIdx))
	{
		items.push_back(item);
		itemIdx++;
	}
}

bool MidiItemProcessor::isMidiTake(MediaItem_Take* take)
{
	if (take)
	{
		PCM_source* source = GetMediaItemTake_Source(take);
		if (source && (!strcmp(source->GetType(), "MIDI") || !strcmp(source->GetType(), "MIDIPOOL")))
			return true;
	}
	return false;
}

void MidiItemProcessor::getMediaItemTakes(MediaItem* item, list<MediaItem_Take*> &takes, bool bMidiOnly)
{
	int takeIdx = 0;
	while(MediaItem_Take* take = GetTake(item, takeIdx))
	{
		if(!bMidiOnly || isMidiTake(take))
			takes.push_back(take);
		takeIdx++;
	}
}

MidiItemProcessor::MidiItemType MidiItemProcessor::getMidiItemType(MediaItem* item)
{
	char* state = GetSetObjectState(item, NULL);
//ShowConsoleMsg(state);
	int iIgnoreTempo = 0;
	bool bIsFile = false;
	int tmpi[4];
	double tmpd[4];

	char* token = strtok(state, "\n");
	while(token != NULL)
	{
		if(sscanf(token, "IGNTEMPO %d %lf %d %d\n", &tmpi[0], &tmpd[0], &tmpi[1], &tmpi[2]) == 4)
			iIgnoreTempo = tmpi[0];

		else if( strstr(token, "FILE \"") && strstr(token, "\" QN") )
			bIsFile = true;

		token = strtok(NULL, "\n");
	}
	FreeHeapPtr(state);

	if(bIsFile)
		return MIDI_ITEM_FILE;
	else if(iIgnoreTempo)
		return MIDI_ITEM_IGNTEMPO;
	else
		return MIDI_ITEM_INPROJECT;
}

//JFB: not localized, kind of poc/test code..
void MidiItemProcessor::getSelectedMidiNotes(MediaItem* item, MIDI_eventlist* evts, map<MIDI_event_t*, bool> &selectedNotes)
{
	set<MidiNoteKey> objStateSelectedNotes;

	char* state = GetSetObjectState(item, NULL);
//ShowConsoleMsg(state);
	int iTicksPerQuarterNote = 960;
	int iCurrentTick = 0;
	int iTranspose = 0;

	int tmpi[4];
	char* token = strtok(state, "\n");
	while(token != NULL)
	{
		if(sscanf(token, "HASDATA %d %d QN\n", &tmpi[0], &tmpi[1]) == 2)
		{
			// iHasData = tmpi[0];
			iTicksPerQuarterNote = tmpi[1];
		}

		else if(sscanf(token, "TRANSPOSE %d\n", &tmpi[0]) == 1)
		{
			iTranspose = tmpi[0];
		}

		token = strtok(NULL, "\n");
	}
	FreeHeapPtr(state);

	state = GetSetObjectState(item, NULL);
	token = strtok(state, "\n");
	while(token != NULL)
	{
		if(sscanf(token, "E %d %x %x %x\n", &tmpi[0], &tmpi[1], &tmpi[2], &tmpi[3]) == 4)
		{
			iCurrentTick += tmpi[0];
		}

		else if(sscanf(token, "e %d %x %x %x\n", &tmpi[0], &tmpi[1], &tmpi[2], &tmpi[3]) == 4)
		{
			iCurrentTick += tmpi[0];
			int status = tmpi[1];
			int midiData1 = tmpi[2];
			//int midiData2 = tmpi[3];

//			ShowConsoleMsgEx("Token = %s\n", token);
			double dCurrentQuarterNote = (double)iCurrentTick/(double)iTicksPerQuarterNote;
			ShowConsoleMsgEx("Selected event = %.2lf : 0x%02x\n", dCurrentQuarterNote, status);

			switch(status & 0xf0)
			{
				case MIDI_CMD_NOTE_ON :
				case MIDI_CMD_NOTE_OFF :
				{
					double dTimeSec = TimeMap2_QNToTime(NULL, dCurrentQuarterNote);
					int frameOffset = (int)(dTimeSec * MIDIITEMPROC_DEFAULT_SAMPLERATE);
					midiData1 += iTranspose;
//ShowConsoleMsgEx("item frameoffset = %d\n", frameOffset);
					MidiNoteKey key(frameOffset, (unsigned char)(status), (unsigned char)(midiData1));
					if(objStateSelectedNotes.count(key) != 0)
						ShowConsoleMsgEx("Watch out: note duplicates!\n");
					objStateSelectedNotes.insert(key);
				}
				break;
				default :
				break;
			}
		}

		token = strtok(NULL, "\n");
	}
	FreeHeapPtr(state);

	int pos = 0;
	while(MIDI_event_t* evt = evts->EnumItems(&pos))
	{
		int statusByte = evt->midi_message[0] & 0xf0;
		switch(statusByte)
		{
			case MIDI_CMD_NOTE_ON :
			case MIDI_CMD_NOTE_OFF :
			{
				MidiNoteKey key(*evt);
				if(objStateSelectedNotes.count(key) != 0)
				{
//ShowConsoleMsgEx("note frameoffset = %d\n", evt->frame_offset);
					selectedNotes[evt] = true;
				}
			}
			break;

			default :
			break;
		}
	}
}

void MidiItemProcessor::filterMidiEvents(MIDI_eventlist* evts, int itemLengthSamples)
{
	for(vector<MidiFilterBase*>::iterator filter = _filters.begin(); filter != _filters.end(); filter++)
	{
		int nextPos = 0;
		int curPos = 0;

		while(MIDI_event_t* evt = evts->EnumItems(&nextPos))
		{
			(*filter)->process(evt, evts, curPos, nextPos, itemLengthSamples);
			curPos = nextPos;
		}
	}
}

void MidiItemProcessor::generateMidiEvents(MIDI_eventlist* evts, int itemLengthSamples)
{
	for(vector<MidiGeneratorBase*>::iterator generator = _generators.begin(); generator != _generators.end(); generator++)
		(*generator)->process(evts, itemLengthSamples);
}

void MidiItemProcessor::processTake(MediaItem_Take* take)
{
	MIDI_eventlist* evts = MIDI_eventlist_Create();

	if(getMidiEventsList(take, evts))
	{
		if(PCM_source* source = GetMediaItemTake_Source(take))
		{
			MediaItem* item = GetMediaItemTake_Item(take);

			double itemLength = *(double*)GetSetMediaItemInfo(item, "D_LENGTH", NULL);
			int itemLengthSamples = (int)(MIDIITEMPROC_DEFAULT_SAMPLERATE * itemLength);

//map<MIDI_event_t*, bool> selectedNotes;
//selectedNotes.clear();
//MidiItemProcessor::getSelectedMidiNotes(item, evts, selectedNotes);

			filterMidiEvents(evts, itemLengthSamples);
			generateMidiEvents(evts, itemLengthSamples);

			midi_realtime_write_struct_t midiBlock;
			midiBlock.global_time       = 0.0;
			midiBlock.global_item_time  = 0.0;
			midiBlock.srate             = MIDIITEMPROC_DEFAULT_SAMPLERATE;
			midiBlock.length            = (int)(midiBlock.srate * itemLength);
			//midiBlock.overwritemode     = -1;
			midiBlock.overwritemode		= 1;		// replace flag
			midiBlock.events            = evts;
			midiBlock.item_playrate     = 1.0;
			midiBlock.latency           = 0.0;
			midiBlock.overwrite_actives = NULL;

			source->Extended(PCM_SOURCE_EXT_ADDMIDIEVENTS, &midiBlock, NULL, NULL);
		}

		MIDI_eventlist_Destroy(evts);
	}
}

void MidiItemProcessor::processSelectedMidiTakes(bool bActiveOnly)
{
	list<MediaItem*> items;
	getSelectedMediaItems(items);

	for(list<MediaItem*>::iterator item = items.begin(); item != items.end(); item++)
	{
		switch(getMidiItemType(*item))
		{
			case MIDI_ITEM_INPROJECT :
			break;
			case MIDI_ITEM_IGNTEMPO :
			{
				HWND hMainHwnd = GetMainHwnd();
				MessageBox(hMainHwnd, __LOCALIZE("Cannot process MIDI items with 'ignore project tempo information' option","sws_mbox"), __LOCALIZE("Padre - Error","sws_mbox"), MB_OK);
				continue;
			}
			break;
			case MIDI_ITEM_FILE :
			{
				HWND hMainHwnd = GetMainHwnd();
				MessageBox(hMainHwnd, __LOCALIZE("Cannot process external MIDI files","sws_mbox"), __LOCALIZE("Padre - Error","sws_mbox"), MB_OK);
				continue;
			}
			break;
			default :
			break;
		}

		list<MediaItem_Take*> takes;

		if(bActiveOnly)
		{
			MediaItem_Take* take = GetActiveTake(*item);
			if(take)
				takes.push_back(take);
		}

		else
			getMediaItemTakes(*item, takes, true);

		for(list<MediaItem_Take*>::iterator take = takes.begin(); take != takes.end(); take++)
			processTake(*take);

		UpdateItemInProject(*item);
//		Undo_OnStateChange_Item(0, _name.c_str(), *item);
	}

//	Undo_OnStateChangeEx(_name.c_str(), UNDO_STATE_ITEMS, -1);
	Undo_OnStateChangeEx(_name.c_str(), UNDO_STATE_ITEMS | UNDO_STATE_TRACKCFG | UNDO_STATE_MISCCFG, -1);

	UpdateTimeline();
}

