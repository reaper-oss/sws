/******************************************************************************
/ padreMidiItemFilters.h
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

class MidiFilterDeleteNotes : public MidiFilterBase
{
	public:
		MidiFilterDeleteNotes();

		virtual void process(MIDI_event_t* evt, MIDI_eventlist* evts, int &curPos, int &nextPos, int itemLengthSamples);
};

class MidiFilterDeleteControlChanges : public MidiFilterBase
{
	private:
		set<int> _ccList;		// Empty: filter all

	public:
		MidiFilterDeleteControlChanges();

		void addCc(int cc);
		void removeCc(int cc);
		virtual void process(MIDI_event_t* evt, MIDI_eventlist* evts, int &curPos, int &nextPos, int itemLengthSamples);
};

class MidiFilterTranspose : public MidiFilterBase
{
	private:
		int _offset;

		MidiFilterTranspose();

	public:
		MidiFilterTranspose(int offset);

		virtual void process(MIDI_event_t* evt, MIDI_eventlist* evts, int &curPos, int &nextPos, int itemLengthSamples);
};

class MidiFilterRandomNotePos : public MidiFilterBase
{
	public:
		MidiFilterRandomNotePos();

		virtual void process(MIDI_event_t* evt, MIDI_eventlist* evts, int &curPos, int &nextPos, int itemLengthSamples);
};

class MidiFilterShortenEndEvents : public MidiFilterBase
{
	public:
		MidiFilterShortenEndEvents();

		virtual void process(MIDI_event_t* evt, MIDI_eventlist* evts, int &curPos, int &nextPos, int itemLengthSamples);
};



class MidiMessage
{
	private:
		unsigned char m_status;
		unsigned char m_data;
		unsigned char m_value;

		MidiMessage();
		bool isEqual(MidiMessage &msg, bool bSameChannel = true, bool bSameData = true, bool bSameValue = true);

	public:
		MidiMessage(unsigned char status, unsigned char m_data = 0x00);

		int getChannel();
		int getType();
};

//class MidiMessageRemover : public MidiFilterBase
//{
//	private:
//		set<MidiMessage*> _msgList;
//
//	public:
//		MidiMessageRemover();
//
//		void addMsg(MidiMessage* msg);
//		void removeMsg(MidiMessage* msg);
//		virtual void process(MIDI_event_t* evt, MIDI_eventlist* evts, int &curPos, int &nextPos, int itemLengthSamples);
//};
