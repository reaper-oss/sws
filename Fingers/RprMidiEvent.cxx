#include "stdafx.h"


#include "RprMidiEvent.hxx"
#include "RprNode.hxx"

#include "StringUtil.hxx"

static std::string toHex(unsigned char hex);

RprMidiBase::RprMidiBase() : mSelected(false), mMuted(false), mDelta(0)
{}

bool RprMidiBase::isSelected() const
{ return mSelected; }

void RprMidiBase::setSelected( bool selected)
{ mSelected = selected; }

bool RprMidiBase::isMuted() const
{ return mMuted; }

void RprMidiBase::setMuted(bool muted)
{ mMuted = muted; }
	
int RprMidiBase::getDelta() const
{ return mDelta; }

void RprMidiBase::setDelta(int delta)
{ mDelta = delta; }

int RprMidiBase::getOffset() const
{ return mOffset; }

void RprMidiBase::setOffset(int offset)
{ mOffset = offset; }


unsigned char RprMidiBase::getChannel() const
{ throw RprMidiBase::RprMidiException("Bad API call");}
void RprMidiBase::setChannel(unsigned char)
{ throw RprMidiBase::RprMidiException("Bad API call");}


unsigned char RprMidiBase::getValue1() const
{ throw RprMidiBase::RprMidiException("Bad API call");}
void RprMidiBase::setValue1(unsigned char)
{ throw RprMidiBase::RprMidiException("Bad API call");}

unsigned char RprMidiBase::getValue2() const
{ throw RprMidiBase::RprMidiException("Bad API call");}
void RprMidiBase::setValue2(unsigned char)
{ throw RprMidiBase::RprMidiException("Bad API call");}

int RprMidiBase::getUnquantizedOffset() const
{ throw RprMidiBase::RprMidiException("Bad API call");}

void RprMidiBase::setUnquantizedOffset(int offset)
{ throw RprMidiBase::RprMidiException("Bad API call");}

RprMidiBase::MessageType RprMidiBase::getMessageType() const
{return Unknown; }
void RprMidiBase::setMessageType(RprMidiBase::MessageType messageType)
{}

const std::string& RprMidiBase::getExtendedData() const
{ throw RprMidiBase::RprMidiException("Bad API call");}

RprMidiBase::RprMidiException::RprMidiException(const char *message) : mMessage(message)
{}
const char *RprMidiBase::RprMidiException::what()
{ return mMessage.c_str(); }
RprMidiBase::RprMidiException::~RprMidiException() throw()
{}



RprExtendedMidiEvent::RprExtendedMidiEvent() : RprMidiBase()
{}
const std::string& RprExtendedMidiEvent::getExtendedData()
{
	return mExtendedData;
}

void RprExtendedMidiEvent::setExtendedData(const std::string &data)
{
	mExtendedData = data;
}

RprMidiBase::MessageType RprExtendedMidiEvent::getMessageType() const
{
	if (mExtendedData.substr(0, 2) == "/w")
		return RprMidiBase::TextEvent;
	else
		return RprMidiBase::Sysex;
}

RprNode *RprExtendedMidiEvent::toReaper()
{
	
	std::stringstream oss;
	if(isSelected())
		oss << "x";
	else
		oss << "X";

	if(isMuted())
		oss << "m";
	oss << " ";
	oss << getDelta() << " 0";
	RprNode *node = new RprParentNode(oss.str().c_str());
	RprNode *childNode = new RprPropertyNode(mExtendedData);
	node->addChild(childNode);
	return node;
}

RprMidiEvent::RprMidiEvent() : RprMidiBase()
{
	mMidiMessage.resize(3);
	mQuantizeOffset = 0;
}

void RprMidiEvent::setMidiMessage(const std::vector<unsigned char> message)
{
	mMidiMessage = message;
}
const std::vector<unsigned char>& RprMidiEvent::getMidiMessage()
{
	return mMidiMessage;
}

unsigned char RprMidiEvent::getValue1() const
{ 
	return mMidiMessage[1];
}

void RprMidiEvent::setValue1(unsigned char value)
{ 
	mMidiMessage[1] = value;
}

static RprMidiBase::MessageType getMessageType(unsigned char message)
{
	message = (message & 0xF0) >> 4;
	switch(message) {
		case 8:
			return RprMidiBase::NoteOff;
		case 9:
			return RprMidiBase::NoteOn;
		case 0xA:
			return RprMidiBase::KeyPressure;
		case 0xB:
			return RprMidiBase::CC;
		case 0xC:
			return RprMidiBase::ProgramChange;
		case 0xD:
			return RprMidiBase::ChannelPressure;
		case 0xE:
			return RprMidiBase::PitchBend;
		default:
			return RprMidiBase::Unknown;
	}
}

RprMidiBase::MessageType RprMidiEvent::getMessageType() const
{
	return ::getMessageType(mMidiMessage[0]);
}

void RprMidiEvent::setMessageType(RprMidiBase::MessageType messageType)
{
	unsigned char messageNibble = 0x0;
	switch(messageType) {
		case NoteOff:
			messageNibble = 0x8;
			break;
		case NoteOn:
			messageNibble = 0x9;
			break;
		case CC:
			messageNibble = 0xB;
			break;
		case ProgramChange:
			messageNibble = 0xC;
			break;
		case PitchBend:
			messageNibble = 0xE;
			break;
	}
	mMidiMessage[0] &= 0x0F;
	mMidiMessage[0] |= (messageNibble << 4);
}

unsigned char RprMidiEvent::getValue2() const
{ 
	return mMidiMessage[2];
}

void RprMidiEvent::setValue2(unsigned char value)
{ 
	mMidiMessage[2] = value;
}

int RprMidiEvent::getUnquantizedOffset() const
{
	return mQuantizeOffset;
}

void RprMidiEvent::setUnquantizedOffset(int offset)
{
	mQuantizeOffset = offset;
}

unsigned char RprMidiEvent::getChannel() const
{ return (mMidiMessage[0] & 0x0F);}

void RprMidiEvent::setChannel(unsigned char channel)
{
	mMidiMessage[0] &= 0xF0;
	mMidiMessage[0] |= channel;
}

RprNode *RprMidiEvent::toReaper()
{
	std::stringstream oss;
	if(isSelected())
		oss << "e";
	else
		oss << "E";

	if(isMuted())
		oss << "m";
	oss << " ";
	oss << getDelta();
	for(std::vector<unsigned char>::iterator i = mMidiMessage.begin(); i != mMidiMessage.end(); i++)
		oss << " " << toHex(*i);

	if(getMessageType() == NoteOn || getMessageType() == NoteOff) {
		if(mQuantizeOffset != 0) {
			oss << " " << mQuantizeOffset;
		}
	}
	RprNode *node = new RprPropertyNode(oss.str());
	return node;
}

static bool isExtended(const std::string &inStr)
{
	if(inStr.empty())
		throw RprMidiBase::RprMidiException("Error parsing MIDI data");
	if(inStr[0] == 'x')
		return true;
	if(inStr[0] == 'X')
		return true;
	return false;
}

static bool isSelected(const std::string &inStr)
{
	if(inStr.empty())
		throw RprMidiBase::RprMidiException("Error parsing MIDI data");
	if(inStr[0] == 'E')
		return false;
	if(inStr[0] == 'e')
		return true;
	if(inStr[0] == 'x')
		return true;
	if(inStr[0] == 'X')
		return false;

	throw RprMidiBase::RprMidiException("Error parsing MIDI data");
}

static bool isMuted(const std::string &inStr)
{
	if(inStr.size() <= 1)
		return false;
	if(inStr[1] == 'm')
		return true;
	
	throw RprMidiBase::RprMidiException("Error parsing MIDI data");
}

static unsigned char fromHex(const std::string &inStr)
{
	unsigned int v;
	std::istringstream iss(inStr);
	iss >> std::setbase(16) >> v;
	return (unsigned char)v;
}

static std::string toHex(unsigned char hex)
{
	std::ostringstream oss;
	oss << std::setbase(16) << std::setw(2) << std::setfill('0') << (int)hex;
	return oss.str();
}

static bool isNote(std::vector<unsigned char> &midiMessage)
{
	if(midiMessage.empty())
		return false;
	if(getMessageType(midiMessage[0]) == RprMidiBase::NoteOn)
		return true;
	if(getMessageType(midiMessage[0]) == RprMidiBase::NoteOff)
		return true;
	return false;
}

RprMidiEventCreator::RprMidiEventCreator(RprNode *node)
{
	mXEvent = NULL;
	mEvent = NULL;

	std::auto_ptr<std::vector<std::string> > tokens(stringTokenize(node->getValue()));
	if(tokens->empty())
		throw RprMidiBase::RprMidiException("Error parsing MIDI data");

	int delta = ::atoi(tokens->at(1).c_str());
	bool selected = isSelected(tokens->at(0));
	bool muted = isMuted(tokens->at(0));
	if(isExtended(tokens->at(0))) {
		if(node->childCount() != 1)
			throw RprMidiBase::RprMidiException("Error parsing MIDI data");
		mXEvent = new RprExtendedMidiEvent();
		mXEvent->setDelta(delta);
		mXEvent->setExtendedData(node->getChild(0)->getValue());
		mXEvent->setMuted(muted);
		mXEvent->setSelected(selected);
		return;
	}
	mEvent = new RprMidiEvent();
	mEvent->setSelected(selected);	
	mEvent->setMuted(muted);
	mEvent->setDelta(delta);
	std::vector<unsigned char> midiMessage;
	for(unsigned int i = 2; i < tokens->size(); i++) {
		if(i == 5 && isNote(midiMessage)) {
			mEvent->setUnquantizedOffset(::atoi(tokens->at(i).c_str()));
		} else {
			midiMessage.push_back(fromHex(tokens->at(i)));
		}
	}
	mEvent->setMidiMessage(midiMessage);
}

RprExtendedMidiEvent *RprMidiEventCreator::getExtended()
{
	return mXEvent;
}

RprMidiEvent *RprMidiEventCreator::getEvent()
{
	return mEvent;
}

RprMidiBase *RprMidiEventCreator::getBaseEvent()
{
	if (mEvent)
		return mEvent;
	return mXEvent;
}







