/******************************************************************************
/ padreMidiItemFilters.cpp
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
#include "padreMidiItemFilters.h"

MidiFilterDeleteNotes::MidiFilterDeleteNotes()
: MidiFilterBase()
{
}

void MidiFilterDeleteNotes::process(MIDI_event_t* evt, MIDI_eventlist* evts, int &curPos, int &nextPos, int itemLengthSamples)
{
	int statusByte = evt->midi_message[0] & 0xf0;
	//int midiChannel = evt->midi_message[0] & 0x0f;

	switch(statusByte)
	{
		case MIDI_CMD_NOTE_ON :
		case MIDI_CMD_NOTE_OFF :
		{
			evts->DeleteItem(curPos);
			nextPos = curPos;
		}
		break;

		default :
		break;
	}
}

MidiFilterDeleteControlChanges::MidiFilterDeleteControlChanges()
: MidiFilterBase()
{
}

void MidiFilterDeleteControlChanges::addCc(int cc)
{
	_ccList.insert(cc);
}

void MidiFilterDeleteControlChanges::removeCc(int cc)
{
	_ccList.erase(cc);
}

void MidiFilterDeleteControlChanges::process(MIDI_event_t* evt, MIDI_eventlist* evts, int &curPos, int &nextPos, int itemLengthSamples)
{
	int statusByte = evt->midi_message[0] & 0xf0;
	//int midiChannel = evt->midi_message[0] & 0x0f;

	switch(statusByte)
	{
		case MIDI_CMD_CONTROL_CHANGE :
		{
			if(_ccList.empty())
			{
				evts->DeleteItem(curPos);
				nextPos = curPos;
			}

			else
			{
				for(set<int>::iterator cc = _ccList.begin(); cc != _ccList.end(); cc++)
				{
					if(evt->midi_message[1] == *cc)
					{
						evts->DeleteItem(curPos);
						nextPos = curPos;
					}
				}
			}
		}
		break;

		default :
		break;
	}
}

MidiFilterTranspose::MidiFilterTranspose()
: MidiFilterBase()
{
}

MidiFilterTranspose::MidiFilterTranspose(int offset)
: MidiFilterBase(), _offset(offset)
{
}

void MidiFilterTranspose::process(MIDI_event_t* evt, MIDI_eventlist* evts, int &curPos, int &nextPos, int itemLengthSamples)
{
	int statusByte = evt->midi_message[0] & 0xf0;
	//int midiChannel = evt->midi_message[0] & 0x0f;

	switch(statusByte)
	{
		case MIDI_CMD_NOTE_ON :
		case MIDI_CMD_NOTE_OFF :
		{
			//int note = evt->midi_message[1];
			//int velocity = evt->midi_message[2];
			evt->midi_message[1] += _offset;
			if(evt->midi_message[1]>127)
				evt->midi_message[1] = 127;
		}
		break;

		default :
		break;
	}
}

MidiFilterRandomNotePos::MidiFilterRandomNotePos()
: MidiFilterBase()
{
}

void MidiFilterRandomNotePos::process(MIDI_event_t* evt, MIDI_eventlist* evts, int &curPos, int &nextPos, int itemLengthSamples)
{
	int statusByte = evt->midi_message[0] & 0xf0;
	//int midiChannel = evt->midi_message[0] & 0x0f;

	switch(statusByte)
	{
		case MIDI_CMD_NOTE_ON :
		case MIDI_CMD_NOTE_OFF :
		{
			evt->frame_offset += (rand()-RAND_MAX/2) / 8;
		}
		break;

		//case MIDI_CMD_CONTROL_CHANGE :
		//break;

		default :
		break;
	}
}

MidiFilterShortenEndEvents::MidiFilterShortenEndEvents()
: MidiFilterBase()
{
}

void MidiFilterShortenEndEvents::process(MIDI_event_t* evt, MIDI_eventlist* evts, int &curPos, int &nextPos, int itemLengthSamples)
{
	int length = 4096 + 64;

	if(evt->frame_offset > (itemLengthSamples - length))
		evt->frame_offset = (itemLengthSamples - length);

	//if(evt->frame_offset > (itemLengthSamples - length))
	//{
	//	evts->DeleteItem(curPos);
	//	nextPos = curPos;
	//}

	//int statusByte = evt->midi_message[0] & 0xf0;
	////int midiChannel = evt->midi_message[0] & 0x0f;

	//switch(statusByte)
	//{
	//	case MIDI_CMD_NOTE_ON :
	//	case MIDI_CMD_NOTE_OFF :
	//	{
	//		evt->frame_offset += (rand()-RAND_MAX/2) / 8;
	//	}
	//	break;

	//	case MIDI_CMD_CONTROL_CHANGE :
	//		evts->DeleteItem(curPos);
	//		nextPos = curPos;
	//	break;

	//	default :
	//	break;
	//}



	//int statusByte = evt->midi_message[0] & 0xf0;
	//int cc = evt->midi_message[1];
	//if( (statusByte == MIDI_CMD_CONTROL_CHANGE) && (cc == MIDI_CC123_ALL_NOTES_OFF) )
	//{
	//}
}




























MidiMessage::MidiMessage()
{
}

MidiMessage::MidiMessage(unsigned char status, unsigned char data)
: m_status(status), m_data(data)
{
}

int MidiMessage::getChannel()
{
	return m_status&0xf;
}

int MidiMessage::getType()
{
	return m_status&0xf0;
}

bool MidiMessage::isEqual(MidiMessage &msg, bool bSameChannel, bool bSameData, bool bSameValue)
{
	if(bSameChannel && (getChannel() != msg.getChannel()))
		return false;
	if(getType() != msg.getType())
		return false;
	switch(getType())
	{
		case MIDI_CMD_NONE:
		break;
		case MIDI_CMD_NOTE_ON:
		case MIDI_CMD_NOTE_OFF:
		case MIDI_CMD_PROGRAM_CHANGE:
		//	if(bSameValue && (m_data != msg.m_data))
		//		return false;
		//break;
		case MIDI_CMD_CONTROL_CHANGE:
			if(bSameData && (m_data != msg.m_data))
				return false;
			if(bSameValue && (m_value != msg.m_value))
				return false;
		break;
		case MIDI_CMD_NOTE_PRESSURE:
		case MIDI_CMD_CHANNEL_PRESSURE:
		case MIDI_CMD_PITCHBEND:
		default:
			//! \todo handle other messages
			return false;
		break;
	}

	return true;
}

//MidiMessageRemover::MidiMessageRemover()
//: MidiFilterBase()
//{
//}
//
//void MidiMessageRemover::addMsg(MidiMessage* msg)
//{
//	_msgList.insert(msg);
//}
//
//void MidiMessageRemover::removeMsg(MidiMessage* msg)
//{
//	_msgList.erase(msg);
//}
//
//void MidiMessageRemover::process(MIDI_event_t* evt, MIDI_eventlist* evts, int &curPos, int &nextPos, int itemLengthSamples)
//{
//	int statusByte = evt->midi_message[0] & 0xf0;
//	//int midiChannel = evt->midi_message[0] & 0x0f;
//
//	switch(statusByte)
//	{
//		case MIDI_CMD_CONTROL_CHANGE :
//		{
//			if(_ccList.empty())
//			{
//				evts->DeleteItem(curPos);
//				nextPos = curPos;
//			}
//
//			else
//			{
//				for(set<int>::iterator cc = _ccList.begin(); cc != _ccList.end(); cc++)
//				{
//					if(evt->midi_message[1] == *cc)
//					{
//						evts->DeleteItem(curPos);
//						nextPos = curPos;
//					}
//				}
//			}
//		}
//		break;
//
//		default :
//		break;
//	}
//}
