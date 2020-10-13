#include "stdafx.h"

#include "RprMidiEvent.h"
#include "RprNode.h"

#include "StringUtil.h"

#include <memory>
#include <WDL/localize/localize.h>

static std::string toHex(unsigned char hex);

RprMidiEvent::RprMidiEvent()
    : mSelected(false), mMuted(false), mDelta(0), mOffset(0), mQuantizeOffset(0)
{
    mMidiMessage.resize(3);
}

bool RprMidiEvent::isSelected() const
{
    return mSelected;
}

void RprMidiEvent::setSelected(bool selected)
{
    mSelected = selected;
}

bool RprMidiEvent::isMuted() const
{
    return mMuted;
}

void RprMidiEvent::setMuted(bool muted)
{
    mMuted = muted;
}

int RprMidiEvent::getDelta() const
{
    return mDelta;
}

void RprMidiEvent::setDelta(int delta)
{
    mDelta = delta;
}

int RprMidiEvent::getOffset() const
{
    return mOffset;
}

void RprMidiEvent::setOffset(int offset)
{
    mOffset = offset;

    for(RprMidiEvent *attachedEvent : mAttachedEvents)
        attachedEvent->setOffset(offset);
}

RprMidiEvent::RprMidiException::RprMidiException(const char *message) : mMessage(message)
{
}

const char *RprMidiEvent::RprMidiException::what()
{
    return mMessage.c_str();
}

RprMidiEvent::RprMidiException::~RprMidiException() throw()
{
}

RprExtendedMidiEvent::RprExtendedMidiEvent() : RprMidiEvent()
{
}

void RprExtendedMidiEvent::addExtendedData(const std::string &data)
{
    mExtendedData.push_back(data);
}

RprMidiEvent::MessageType RprExtendedMidiEvent::getMessageType() const
{
    if (mExtendedData.front().substr(0, 2) == "/w")
        return RprMidiEvent::TextEvent;
    else
        return RprMidiEvent::Sysex;
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
    std::auto_ptr<RprNode> node(new RprParentNode(oss.str().c_str()));
    for(std::list<std::string>::const_iterator i = mExtendedData.begin();
        i != mExtendedData.end(); ++i)
    {
        std::auto_ptr<RprNode> childNode(new RprPropertyNode(*i));
        node->addChild(childNode.release());
    }
    return node.release();
}

void RprMidiEvent::setMidiMessage(const std::vector<unsigned char> message)
{
    mMidiMessage = message;
}

const std::vector<unsigned char>& RprMidiEvent::getMidiMessage()
{
    return mMidiMessage;
}

bool RprMidiEvent::isAttachableTo(const RprMidiEvent *targetEvent) const
{
    // Notations Events are Text Events occuring right after a Note On.
    // Normal Text Events occur before any Note On at the same time position.
    // Therefore, this only matches Notation Events attached to a Note On.

    return
        getOffset() == targetEvent->getOffset() && // same absolute position
        getMessageType() == NotationEvent &&
        targetEvent->getMessageType() == RprMidiEvent::NoteOn;
}

void RprMidiEvent::addAttachedEvent(RprMidiEvent *attachedEvent)
{
    mAttachedEvents.push_back(attachedEvent);
}

void RprMidiEvent::addPropertyNode(const RprNode *node)
{
    mPropertyLines.push_back(node->getValue());
}

unsigned char RprMidiEvent::getValue1() const
{
    return mMidiMessage[1];
}

void RprMidiEvent::setValue1(unsigned char value)
{
    mMidiMessage[1] = value;
}

static RprMidiEvent::MessageType getMessageType(unsigned char message)
{
    message = (message & 0xF0) >> 4;
    switch(message) {
        case 8:
            return RprMidiEvent::NoteOff;
        case 9:
            return RprMidiEvent::NoteOn;
        case 0xA:
            return RprMidiEvent::KeyPressure;
        case 0xB:
            return RprMidiEvent::CC;
        case 0xC:
            return RprMidiEvent::ProgramChange;
        case 0xD:
            return RprMidiEvent::ChannelPressure;
        case 0xE:
            return RprMidiEvent::PitchBend;
        default:
            return RprMidiEvent::Unknown;
    }
}

RprMidiEvent::MessageType RprMidiEvent::getMessageType() const
{
    return ::getMessageType(mMidiMessage[0]);
}

void RprMidiEvent::setMessageType(RprMidiEvent::MessageType messageType)
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
        default:
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
{ 
    return (mMidiMessage[0] & 0x0F);
}

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

    for(const std::string &propertyLine : mPropertyLines)
        oss << '\n' << propertyLine;

    std::auto_ptr<RprNode> node(new RprPropertyNode(oss.str()));
    return node.release();
}

static bool isExtended(const char* inStr)
{
    if(inStr[0] == 0)
        throw RprMidiEvent::RprMidiException(__LOCALIZE("Error parsing MIDI data","sws_mbox"));
    if(inStr[0] == 'x')
        return true;
    if(inStr[0] == 'X')
        return true;
    return false;
}

static bool isSelected(const char* inStr)
{
    if(inStr[0] == 0)
        throw RprMidiEvent::RprMidiException(__LOCALIZE("Error parsing MIDI data","sws_mbox"));
    if(inStr[0] == 'E')
        return false;
    if(inStr[0] == 'e')
        return true;
    if(inStr[0] == 'x')
        return true;
    if(inStr[0] == 'X')
        return false;

    throw RprMidiEvent::RprMidiException(__LOCALIZE("Error parsing MIDI data","sws_mbox"));
}

static bool isMuted(const char* inStr)
{
    if(inStr[0] == 0 || inStr[1] == 0)
        return false;
    if(inStr[1] == 'm')
        return true;

    throw RprMidiEvent::RprMidiException(__LOCALIZE("Error parsing MIDI data","sws_mbox"));
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
    if(getMessageType(midiMessage[0]) == RprMidiEvent::NoteOn)
        return true;
    if(getMessageType(midiMessage[0]) == RprMidiEvent::NoteOff)
        return true;
    return false;
}

RprMidiEventCreator::RprMidiEventCreator(RprNode *node)
{
    StringVector tokens(node->getValue());

    if(tokens.empty())
        throw RprMidiEvent::RprMidiException(__LOCALIZE("Error parsing MIDI data","sws_mbox"));

    int delta = (int)strtoul(tokens.at(1), 0, 10);
    bool selected = isSelected(tokens.at(0));
    bool muted = isMuted(tokens.at(0));

    if(isExtended(tokens.at(0)))
    {
        mXEvent.reset(new RprExtendedMidiEvent());
        mXEvent->setDelta(delta);

        for(int i = 0; i < node->childCount(); ++i)
        {
            mXEvent->addExtendedData(node->getChild(i)->getValue());
        }

        mXEvent->setMuted(muted);
        mXEvent->setSelected(selected);
        return;
    }
    mEvent.reset(new RprMidiEvent());
    mEvent->setSelected(selected);
    mEvent->setMuted(muted);
    mEvent->setDelta(delta);
    std::vector<unsigned char> midiMessage;
    for(unsigned int i = 2; i < tokens.size(); i++) {

        if(i == 5 && isNote(midiMessage)) {
            mEvent->setUnquantizedOffset(::atoi(tokens.at(i)));
        } else {
            midiMessage.push_back(fromHex(tokens.at(i)));
        }

    }
    mEvent->setMidiMessage(midiMessage);
}

RprMidiEvent *RprMidiEventCreator::collectEvent()
{
    if (mEvent.get())
        return mEvent.release();

    if (mXEvent.get())
        return mXEvent.release();

    throw RprMidiEvent::RprMidiException(__LOCALIZE("Error parsing MIDI data","sws_mbox"));
}

RprMidiEventCreator::~RprMidiEventCreator()
{
}
