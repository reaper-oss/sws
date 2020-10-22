#include "stdafx.h"

#include "RprMidiTake.h"
#include "RprStateChunk.h"
#include "RprNode.h"
#include "RprTake.h"
#include "RprMidiEvent.h"
#include "StringUtil.h"
#include "RprItem.h"
#include "TimeMap.h"
#include "RprException.h"

#include <algorithm>
#include <WDL/localize/localize.h>
#include <WDL/ptrlist.h>

WDL_PtrList_DOD<RprMidiTake> g_script_miditakes; // just to validate function parameters

RprMidiTake* FNG_AllocMidiTake(MediaItem_Take* take)
{
    if (!take) return NULL;

    RprTake rprTake(take);
    if (rprTake.isMIDI() && !rprTake.isFile())
    {
        return g_script_miditakes.Add(new RprMidiTake(rprTake));
    }
    return NULL;
}

void FNG_FreeMidiTake(RprMidiTake* midiTake)
{
    int idx=g_script_miditakes.Find(midiTake);
    if (idx>=0) g_script_miditakes.Delete(idx, true);
}

// Count how many MIDI notes the take has
int FNG_CountMidiNotes(RprMidiTake* midiTake)
{
    if (midiTake && g_script_miditakes.Find(midiTake)>=0)
    {
        return midiTake->countNotes();
    }
    return 0;
}

// Get MIDI note from MIDI take at specified index
RprMidiNote* FNG_GetMidiNote(RprMidiTake* midiTake, int index)
{
    if (index < 0 || !midiTake || g_script_miditakes.Find(midiTake)<0 || index >= midiTake->countNotes())
    {
        return NULL;
    }
    return midiTake->getNoteAt(index);
}

#define FNG_STRCMP(x, static_y) ( strncmp(x, static_y, sizeof(static_y) - 1 ) == 0)

int FNG_GetMidiNoteIntProperty(RprMidiNote* midiNote, const char* property)
{
    if (!midiNote)
    {
        return 0;
    }

    if (FNG_STRCMP(property, "VELOCITY"))
    {
        return midiNote->getVelocity();
    }

    if (FNG_STRCMP(property, "PITCH"))
    {
        return midiNote->getPitch();
    }

    if (FNG_STRCMP(property, "POSITION"))
    {
        return midiNote->getItemPosition();
    }

    if (FNG_STRCMP(property, "LENGTH"))
    {
        return midiNote->getItemLength();
    }

    if (FNG_STRCMP(property, "CHANNEL"))
    {
        return midiNote->getChannel();
    }

    if (FNG_STRCMP(property, "SELECTED"))
    {
        return midiNote->isSelected() ? 1 : 0;
    }

    if (FNG_STRCMP(property, "MUTED"))
    {
        return midiNote->isMuted() ? 1 : 0;
    }
    return 0;
}

void FNG_SetMidiNoteIntProperty(RprMidiNote* midiNote, const char* property, int value)
{
    if (!midiNote)
    {
        return;
    }

    if (FNG_STRCMP(property, "VELOCITY"))
    {
        midiNote->setVelocity(value);
        return;
    }

    if (FNG_STRCMP(property, "PITCH"))
    {
        midiNote->setPitch(value);
        return;
    }

    if (FNG_STRCMP(property, "POSITION"))
    {
        midiNote->setItemPosition(value);
        return;
    }

    if (FNG_STRCMP(property, "LENGTH"))
    {
        midiNote->setItemLength(value);
        return;
    }

    if (FNG_STRCMP(property, "CHANNEL"))
    {
        midiNote->setChannel(value);
        return;
    }

    if (FNG_STRCMP(property, "SELECTED"))
    {
        midiNote->setSelected(value != 0);
        return;
    }

    if (FNG_STRCMP(property, "MUTED"))
    {
        midiNote->setMuted(value != 0);
        return;
    }
}

RprMidiNote* FNG_AddMidiNote(RprMidiTake* midiTake)
{
    if (midiTake && g_script_miditakes.Find(midiTake)>=0)
    {
        return midiTake->addNoteAt(0);
    }
    return NULL;
}

typedef std::list<RprMidiEvent *> RprMidiEvents;
typedef std::list<RprMidiEvent *>::iterator RprMidiEventsIter;
typedef std::list<RprMidiEvent *>::const_iterator RprMidiEventsCIter;

/* Class to auto cleanup pointers to RprMidiEvent objects stored in a list */
class RprTempMidiEvents
{
public:
    RprTempMidiEvents()
    {
    }

    RprMidiEvents &get()
    {
        return mEvents;
    }

    ~RprTempMidiEvents()
    {
        for(RprMidiEventsIter i = mEvents.begin(); i != mEvents.end(); ++i)
        {
            delete *i;
        }
    }

private:
    RprMidiEvents mEvents;
};

class RprMidiContext
{
public:
    static RprMidiContext *createMidiContext(double playRate, double startOffset, int ticksPerQN)
    {
        RprMidiContext *context = new RprMidiContext;
        context->mPlayRate = playRate;
        context->mStartOffset = startOffset;
        context->mTicksPerQN = ticksPerQN;
        return context;
    }

    ~RprMidiContext()
    {
    }

    double getPlayRate() const
    {
        return mPlayRate;
    }

    double getStartOffset() const
    {
        return mStartOffset;
    }

    int getTicksPerQN() const
    {
        return mTicksPerQN;
    }

private:

    RprMidiContext()
    {
    }

    int mTicksPerQN;
    double mStartOffset;
    double mPlayRate;
};

template
<typename T>
static bool compareMidiPositions(T *lhs, T *rhs)
{
    return lhs->getItemPosition() < rhs->getItemPosition();
}

RprMidiNote::RprMidiNote(RprMidiContext *context)
{
    RprMidiEvent *noteOnEvent = new RprMidiEvent();
    noteOnEvent->setMessageType(RprMidiEvent::NoteOn);
    mNoteOn = noteOnEvent;

    RprMidiEvent *noteOffEvent = new RprMidiEvent();
    noteOffEvent->setMessageType(RprMidiEvent::NoteOff);
    mNoteOff = noteOffEvent;
    mContext = context;
}

RprMidiNote::RprMidiNote(RprMidiEvent *noteOn, RprMidiEvent *noteOff, RprMidiContext *context)
{
    mNoteOn = noteOn;
    mNoteOff = noteOff;
    mContext = context;
}

static double getPositionMidiOffset(const RprMidiContext *context, int offset)
{
    double offsetQN = TimeToQN(context->getStartOffset());
    double midiNoteQN = (double)offset / (double)context->getTicksPerQN();
    midiNoteQN /= context->getPlayRate();
    offsetQN += midiNoteQN;
    return QNtoTime(offsetQN);
}

static int getMidiOffsetPosition(const RprMidiContext *context, double position)
{
    double posQN = TimeToQN(position);
    double startQN = TimeToQN(context->getStartOffset());
    double itemQN = posQN - startQN;
    itemQN *= context->getPlayRate();
    return (int)(itemQN * (double)context->getTicksPerQN() + 0.5);
}

double RprMidiNote::getPosition() const
{
    return getPositionMidiOffset(mContext, mNoteOn->getOffset());
}

void RprMidiNote::setPosition(double position)
{
    int unQuantizedNoteOn = mNoteOn->getOffset() + mNoteOn->getUnquantizedOffset();
    int unQuantizedNoteOff = mNoteOff->getOffset() + mNoteOff->getUnquantizedOffset();

    int noteOnOffset = getMidiOffsetPosition(mContext, position);
    int noteOffOffset = noteOnOffset + mNoteOff->getOffset() - mNoteOn->getOffset();

    mNoteOn->setOffset(noteOnOffset);
    mNoteOff->setOffset(noteOffOffset);

    mNoteOn->setUnquantizedOffset(unQuantizedNoteOn - noteOnOffset);
    mNoteOff->setUnquantizedOffset(unQuantizedNoteOff - noteOffOffset);
}

bool RprMidiNote::isSelected() const
{
    return mNoteOn->isSelected();
}

bool RprMidiNote::isMuted() const
{
    return mNoteOn->isMuted();
}

void RprMidiNote::setMuted(bool muted)
{
    mNoteOn->setMuted(muted);
    mNoteOff->setMuted(muted);
}

void RprMidiNote::setSelected(bool selected)
{
    mNoteOn->setSelected(selected);
    mNoteOff->setSelected(selected);
}

int RprMidiNote::getItemPosition() const
{
    return mNoteOn->getOffset();
}

void RprMidiNote::setItemPosition(int position)
{
    int noteOffPosition = mNoteOff->getOffset() - mNoteOn->getOffset() + position;
    mNoteOn->setOffset(position);
    mNoteOff->setOffset(noteOffPosition);
}

int RprMidiNote::getChannel() const
{
    return (int)(mNoteOn->getChannel() + 1);
}

void RprMidiNote::setChannel(int channel)
{
    mNoteOn->setChannel(channel - 1);
    mNoteOff->setChannel(channel - 1);
}

double RprMidiNote::getLength() const
{
    return getPositionMidiOffset(mContext, mNoteOff->getOffset()) -
        getPositionMidiOffset(mContext, mNoteOn->getOffset());
}

void RprMidiNote::setLength(double length)
{
    double pos = getPosition();
    double rightEdgeOffset = TimeToQN(pos + length);
    double leftEdgeOffset = TimeToQN(pos);
    setItemLength( (int)((rightEdgeOffset - leftEdgeOffset) *
        (double)mContext->getTicksPerQN() + 0.5));
}

int RprMidiNote::getItemLength() const
{
    return mNoteOff->getOffset() - mNoteOn->getOffset();
}

void RprMidiNote::setItemLength(int len)
{
    int offset = mNoteOn->getOffset();
    int unquantizedOffset = offset + mNoteOff->getUnquantizedOffset();
    offset += len;
    mNoteOff->setOffset(offset);
    mNoteOff->setUnquantizedOffset(unquantizedOffset - offset);
}

void RprMidiNote::setPitch(int pitch)
{
    if (pitch > 127)
    {
        pitch = 127;
    }

    if (pitch < 0)
    {
        pitch = 0;
    }
    mNoteOn->setValue1((unsigned char)pitch);
    mNoteOff->setValue1((unsigned char)pitch);
}

int RprMidiNote::getPitch() const
{
    return (int)mNoteOn->getValue1();
}

void RprMidiNote::setVelocity(int velocity)
{
    if (velocity > 127)
    {
        velocity = 127;
    }

    if (velocity < 0)
    {
        velocity = 0;
    }

    mNoteOn->setValue2((unsigned char)velocity);
    if(mNoteOff->getMessageType() == RprMidiEvent::NoteOn &&
       mNoteOff->getValue2() == 0)
    {
        return;
    }
    mNoteOff->setValue2((unsigned char)velocity);
}

int RprMidiNote::getVelocity() const
{
    return (int)mNoteOn->getValue2();
}

RprMidiNote::~RprMidiNote()
{
    delete mNoteOn;
    delete mNoteOff;
}

RprMidiCC::RprMidiCC(RprMidiContext *context, int controller)
{
    mCC = new RprMidiEvent();
    mCC->setMessageType(RprMidiEvent::CC);

    // Probably should throw an exception here if
    // controller is invalid.
    if(controller > 127)
    {
        controller = 127;
    }

    if(controller < 0)
    {
        controller = 0;
    }

    mCC->setValue1((unsigned char)controller);
    mContext = context;
}
RprMidiCC::RprMidiCC(RprMidiEvent *cc, RprMidiContext *context)
{
    mCC = cc;
    mContext = context;
}

int RprMidiCC::getChannel() const
{
    return mCC->getChannel() + 1;
}

int RprMidiCC::getItemPosition() const
{
    return mCC->getOffset();
}

RprMidiCC::~RprMidiCC()
{
    delete mCC;
}

static int getQNValue(RprNode *midiNode)
{
    const std::string &hasdata = midiNode->findChildByToken("HASDATA")->getValue();
    return ::atoi(StringVector{hasdata}.at(2));
}

static bool sortMidiBase(const RprMidiEvent *lhs, const RprMidiEvent *rhs)
{
    if (rhs->getOffset() == lhs->getOffset())
    {
        if (lhs->getMessageType() == RprMidiEvent::NoteOn && 
            rhs->getMessageType() == RprMidiEvent::NoteOn)
        {
            // Order by increasing velocity so 0 velocity notes
            // appear first
            return lhs->getValue2() < rhs->getValue2();
        }
        // Order by message type so note-offs appear first
        return lhs->getMessageType() < rhs->getMessageType();
    }
    return lhs->getOffset() < rhs->getOffset();
}

static bool isMidiEvent(const std::string &eventStr) {

    if(eventStr.size() <= 3)
    {
        return false;
    }

    switch(eventStr[0])
    {
        case 'e':
        case 'x':
        case 'X':
        case 'E':
            break;
        default:
            return false;
    }

    switch(eventStr[1])
    {
        case ' ':
            return true;
        case 'm':
            break;
        default:
            return false;
    }

    switch(eventStr[2]) {
        case ' ':
            return true;
        default:
            return false;
    }

    return false;
}

static bool isEventProperty(const std::string &eventStr)
{
    return eventStr.rfind("ENV ", 0) == 0; // CC shape/bezier tension
}

static int clearMidiEventsFromMidiNode(RprNode *parent)
{
    int i = 0;
    for(; i < parent->childCount(); ++i)
    {
        const std::string &value = parent->getChild(i)->getValue();
        if(isMidiEvent(value) || isEventProperty(value))
            break;
    }

    const int offset = i;
    while(true)
    {
        const std::string &value = parent->getChild(i)->getValue();
        if(!isMidiEvent(value) && !isEventProperty(value))
            break;

        parent->removeChild(i);
    }

    return offset;
}

static void midiEventsToMidiNode(std::vector< RprMidiEvent *> &midiEvents, RprNode *midiNode, 
                                 int offset)
{
    int index = offset;
    for(std::vector<RprMidiEvent *>::iterator i = midiEvents.begin(); i != midiEvents.end(); i++)
    {
        RprMidiEvent *current = *i;
        midiNode->addChild(current->toReaper(), index++);
    }
}

static void getMidiEvents(RprNode *midiNode, RprMidiEvents &midiEvents)
{
    int offset = 0;
    for(int i = 1; i < midiNode->childCount(); i++)
    {
        RprNode *subNode = midiNode->getChild(i);
        const std::string &value = subNode->getValue();

        if(isEventProperty(value))
        {
            if(!midiEvents.empty())
                midiEvents.back()->addPropertyNode(subNode);

            continue;
        }
        else if(!isMidiEvent(value))
            continue;

        RprMidiEventCreator creator(subNode);
        RprMidiEvent *midiEvent = creator.collectEvent();
        offset += midiEvent->getDelta();
        midiEvent->setOffset(offset);

        if(!midiEvents.empty() && midiEvent->isAttachableTo(midiEvents.back()))
            midiEvents.back()->addAttachedEvent(midiEvent);

        midiEvents.push_back(midiEvent);
    }
}

static bool noteEventsMatch(const RprMidiEvent *noteOn, const RprMidiEvent *noteOff)
{
    if(noteOn->getChannel() != noteOff->getChannel())
    {
        return false;
    }

    if(noteOn->getValue1() != noteOff->getValue1())
    {
        return false;
    }

    if(noteOn->getOffset() > noteOff->getOffset())
    {
        return false;
    }
    return true;
}

static bool getMidiCCs(RprMidiEvents &midiEvents,
                       std::vector<RprMidiCC *> *midiCCs,
                       RprMidiContext *context)
{
    RprMidiEvents other;
    for(RprMidiEventsCIter i = midiEvents.begin(); i != midiEvents.end(); ++i)
    {
        RprMidiEvent *current = *i;

        if(current->getMessageType() == RprMidiEvent::CC)
        {
            midiCCs[current->getValue1()].push_back(new RprMidiCC(current, context));
        }
        else
        {
            other.push_back(current);
        }
    }
    midiEvents = other;
    return true;
}

static bool getMidiNotes(RprMidiEvents &midiEvents,
                         std::vector<RprMidiNote *> &midiNotes,
                         RprMidiContext *context)
{
    RprMidiEvents noteOns;
    RprMidiEvents noteOffs;
    RprMidiEvents other;

    /* categorize notes into note-ons and note-offs */
    for(RprMidiEventsCIter i = midiEvents.begin(); i != midiEvents.end(); ++i)
    {
        RprMidiEvent *current = *i;
        if(current->getMessageType() == RprMidiEvent::NoteOn && current->getValue2() != 0)
        {
            noteOns.push_back(current);
        }
        else if(current->getMessageType() == RprMidiEvent::NoteOff ||
               (current->getMessageType() == RprMidiEvent::NoteOn &&
                current->getValue2() == 0))
        {
            noteOffs.push_back(current);
        }
        else
        {
            other.push_back(current);
        }
    }
    midiEvents.clear();

    /* match note-ons and note-offs, removing zero length notes */
    RprMidiEventsIter i = noteOns.begin();
    while(i != noteOns.end())
    {
        RprMidiEventsIter j = noteOffs.begin();
        while(j != noteOffs.end() && !noteEventsMatch(*i, *j))
        {
            ++j;
        }
        /* no match so add noteOn to other events */
        if(j == noteOffs.end())
        {
            other.push_back(*i);
            ++i;
            continue;
        }

        RprMidiEvent *noteOn = *i;
        RprMidiEvent *noteOff = *j;
        /* delete zero length notes */
        if(noteOn->getOffset() == noteOff->getOffset())
        {
            delete noteOn;
            delete noteOff;
            noteOns.erase(i++);
            noteOffs.erase(j);
            continue;
        }

        RprMidiNote *newNote = new RprMidiNote(noteOn, noteOff, context);
        midiNotes.push_back(newNote);
        noteOffs.erase(j);
        ++i;
    }

    /* put non-note events back onto midiEvents list */
    for(RprMidiEventsCIter j = noteOffs.begin(); j != noteOffs.end(); j++)
    {
        midiEvents.push_back(*j);
    }

    for(RprMidiEventsCIter j = other.begin(); j != other.end(); j++)
    {
        midiEvents.push_back(*j);
    }
    return true;
}

static void removeDuplicates(std::vector<RprMidiCC *> *midiCCs)
{
    for(int i = 0; i < 128; i++)
    {
        for(int ccOffset = 0; (midiCCs[i].begin() + ccOffset) != midiCCs[i].end(); ++ccOffset)
        {
            std::vector<RprMidiCC *>::iterator j = midiCCs[i].begin() + ccOffset;
            for(std::vector<RprMidiCC *>::iterator k = j + 1; k != midiCCs[i].end(); ++k)
            {
                if( (*k)->getItemPosition() > (*j)->getItemPosition())
                {
                    break;
                }

                if( (*k)->getItemPosition() != (*j)->getItemPosition())
                {
                    continue;
                }

                if( (*k)->getChannel() != (*j)->getChannel())
                {
                    continue;
                }

                delete *k;
                k = midiCCs[i].erase(k);
                j = midiCCs[i].begin() + ccOffset;
                if (ccOffset+1 >= (int)midiCCs[i].size())
                    break;
            }
        }
    }
}

static void removeDuplicates(std::vector<RprMidiNote *> &midiNotes)
{
    for(int noteOffset = 0; (midiNotes.begin() + noteOffset) != midiNotes.end(); noteOffset++)
    {
		std::vector<RprMidiNote *>::iterator i = midiNotes.begin() + noteOffset;
        RprMidiNote *lhs = *i;
        for(std::vector<RprMidiNote *>::iterator j = i + 1; j != midiNotes.end(); j++)
        {
            RprMidiNote *rhs = *j;
            if (rhs->getItemPosition() > lhs->getItemPosition())
            {
                break;
            }

            if(lhs->getItemPosition() != rhs->getItemPosition())
            {
                continue;
            }
            if(lhs->getPitch() != rhs->getPitch())
            {
                continue;
            }
            if(lhs->getChannel() != rhs->getChannel())
            {
                continue;
            }
            if(lhs->getItemLength() > rhs->getItemLength())
            {
                delete rhs;
                j = midiNotes.erase(j);
                if (noteOffset+1 >= (int)midiNotes.size())
                    break;
            }
            else
            {
                delete lhs;
                i = midiNotes.erase(i);
                noteOffset--;
                break;
            }
        }
    }
}

static void removeOverlaps(std::vector<RprMidiNote *> &midiNotes)
{
    for(int noteOffset = 0; (midiNotes.begin() + noteOffset) != midiNotes.end(); ++noteOffset)
    {
        std::vector<RprMidiNote *>::iterator i = midiNotes.begin() + noteOffset;
        RprMidiNote *lhs = *i;
        for(std::vector<RprMidiNote *>::iterator j = i + 1; j != midiNotes.end(); j++)
        {
            RprMidiNote *rhs = *j;
            if(rhs->getItemPosition() >= lhs->getItemPosition() + lhs->getItemLength())
            {
                break;
            }
            if(lhs->getPitch() != rhs->getPitch())
            {
                continue;
            }
            if(lhs->getChannel() != rhs->getChannel())
            {
                continue;
            }
            if(lhs->getItemPosition() + lhs->getItemLength() >= rhs->getItemPosition())
            {
                int lhsLength = rhs->getItemPosition() - lhs->getItemPosition();
                if(lhsLength <= 0)
                {
                    delete lhs;
                    i = midiNotes.erase(i);
                    noteOffset--;
                }
                else
                {
                    lhs->setItemLength(lhsLength);
                }
                break;
            }
        }
    }
}

RprMidiNote *RprMidiTake::getNoteAt(int index) const
{
    return mNotes.at(index);
}

RprMidiNote *RprMidiTake::addNoteAt(int index)
{
    RprMidiNote *note = new RprMidiNote(mContext);
    mNotes.insert(mNotes.begin() + index, note);
    return note;
}

int RprMidiTake::countNotes() const
{
    return (int)mNotes.size();
}

RprMidiTake::RprMidiTake(const RprTake &take, bool readOnly)
: RprMidiTemplate(take, readOnly)
{
    try
    {
        mContext = NULL;
        RprNode *sourceNode = RprMidiTemplate::getMidiSourceNode();

        RprTempMidiEvents tempMidiEvents;
        getMidiEvents(sourceNode, tempMidiEvents.get());
        mContext = RprMidiContext::createMidiContext(take.getPlayRate(),
            getParent()->getPosition() - (take.getStartOffset() / take.getPlayRate()),
            getQNValue(sourceNode));

        if (!getMidiNotes(tempMidiEvents.get(), mNotes, mContext))
        {
            throw RprLibException(__LOCALIZE("Unable to parse MIDI data","sws_mbox"));
        }

        if(!getMidiCCs(tempMidiEvents.get(), mCCs, mContext))
        {
            throw RprLibException(__LOCALIZE("Unable to parse MIDI data","sws_mbox"));
        }

        mOtherEvents.reserve(tempMidiEvents.get().size());
        std::copy(tempMidiEvents.get().begin(), tempMidiEvents.get().end(),
            std::back_inserter(mOtherEvents));
        tempMidiEvents.get().clear();
        mMidiEventsOffset = clearMidiEventsFromMidiNode(sourceNode);

    }
    catch (RprMidiEvent::RprMidiException &e)
    {
        cleanup();
        // Throw RprLibException so we let the user know something bad
        // happened.
        throw RprLibException(e.what(), true);
    }
}

template
<typename T>
void cleanUpPointers(std::vector<T *> &pointers)
{
    for(typename std::vector<T *>::iterator i = pointers.begin(); i != pointers.end(); i++)
    {
        delete *i;
    }
    pointers.clear();
}

RprMidiTake::~RprMidiTake()
{
    if (isReadOnly())
    {
        cleanup();
        return;
    }

    std::vector<RprMidiEvent *> midiEvents;
    std::sort(mNotes.begin(), mNotes.end(), compareMidiPositions<RprMidiNote>);
    for(int i = 0; i < 128; i++)
    {
        std::sort(mCCs[i].begin(), mCCs[i].end(), compareMidiPositions<RprMidiCC>);
    }
    removeDuplicates(mNotes);
    removeDuplicates(mCCs);
    removeOverlaps(mNotes);
    midiEvents.reserve(mNotes.size() * 2 + mOtherEvents.size());

    RprMidiEvent* allNotesOffEvent = NULL;
    if (!mCCs[0x7b].empty())
    {
        allNotesOffEvent = (*mCCs[0x7b].begin())->mCC;
    }

    for(std::vector<RprMidiNote *>::const_iterator i = mNotes.begin();
        i != mNotes.end(); ++i)
    {
        RprMidiNote* note = *i;
        if (allNotesOffEvent)
        {
            if (note->getItemPosition() >= allNotesOffEvent->getOffset())
            {
                continue;
            }

            if (note->getItemPosition() + note->getItemLength() > allNotesOffEvent->getOffset())
            {
                note->setItemLength(allNotesOffEvent->getOffset() - note->getItemPosition()); 
            }
        }
        midiEvents.push_back((*i)->mNoteOn);
        midiEvents.push_back((*i)->mNoteOff);
    }

    for(int j = 0; j < 128; j++)
    {
        // ignore all-notes-off event, handle this separately below
        if (j == 0x7b)
        {
            continue;
        }

        for(std::vector<RprMidiCC *>::const_iterator i = mCCs[j].begin();
            i != mCCs[j].end(); ++i)
        {
            midiEvents.push_back((*i)->mCC);
        }
    }

    for(std::vector<RprMidiEvent *>::const_iterator i = mOtherEvents.begin();
        i != mOtherEvents.end(); ++i)
    {
        midiEvents.push_back(*i);
    }

    std::sort(midiEvents.begin(), midiEvents.end(), sortMidiBase);

    if (allNotesOffEvent)
    {
        midiEvents.push_back(allNotesOffEvent);
    }

    int firstEventOffset = 0;
    if (!midiEvents.empty())
    {
        firstEventOffset = (*midiEvents.begin())->getOffset();
    }

    int offset = 0;
    if (firstEventOffset < 0)
    {
        double takeStartPosition = mTake.getParent().getPosition() -
            mTake.getStartOffset() / mContext->getPlayRate();

        // convert to Quarter notes and subtract first event offset
        double newTakeQNStartPosition = TimeToQN(takeStartPosition) + ((double)firstEventOffset /
            mContext->getTicksPerQN()) / mContext->getPlayRate();
        //convert back to seconds / playrate
        mNewTakeOffset = takeStartPosition - QNtoTime(newTakeQNStartPosition) +
            mTake.getStartOffset() / mContext->getPlayRate();
        //convert to seconds
        mNewTakeOffset *= mContext->getPlayRate();
        // Hack! Have to set start offset after setting item state so
        // set a flag to indicate we need a new start offset set
        mSetNewTakeOffset = true;
        // set initial offset to -ve value to get rid of -ve deltas
        offset = firstEventOffset;
    }

    for(std::vector<RprMidiEvent *>::iterator i = midiEvents.begin(); i != midiEvents.end(); ++i)
    {
        RprMidiEvent *current = *i;
        int delta = current->getOffset() - offset;
        current->setDelta(delta);
        offset += delta;
    }

    midiEventsToMidiNode(midiEvents, RprMidiTemplate::getMidiSourceNode(),
        mMidiEventsOffset);

    cleanup();
}

void RprMidiTake::cleanup()
{
    if(mContext)
    {
        delete mContext;
    }
    mContext = NULL;

    for(int j = 0; j < 128; j++)
    {
        cleanUpPointers(mCCs[j]);
    }
    cleanUpPointers(mOtherEvents);
    cleanUpPointers(mNotes);
}

RprMidiTakePtr RprMidiTake::createFromMidiEditor(bool readOnly)
{
    HWND midiEditor = MIDIEditor_GetActive();
    if(midiEditor == NULL)
    {
        throw RprLibException(__LOCALIZE("No active MIDI editor","sws_mbox"), true);
    }

    RprTake take(MIDIEditor_GetTake(midiEditor));
    const char *sourceFilename = take.getSource()->GetFileName();
    if(!*sourceFilename)
    {
        RprMidiTakePtr takePtr(new RprMidiTake(take, readOnly));
        return takePtr;
    }
    throw RprLibException(__LOCALIZE("Only in-project MIDI can be modified","sws_mbox"), true);
}

int RprMidiTake::countCCs(int controller) const
{
    return (int)mCCs[controller].size();
}

static bool hasEvent(std::vector<RprMidiEvent *> &midiEvents, RprMidiEvent::MessageType messageType)
{
    for(std::vector<RprMidiEvent *>::const_iterator i = midiEvents.begin();
        i != midiEvents.end(); ++i)
    {
        RprMidiEvent *midiEvent = *i;
        if(midiEvent->getMessageType() == messageType)
        {
            return true;
        }
    }
    return false;
}

bool RprMidiTake::hasEventType(RprMidiEvent::MessageType messageType)
{
    if(messageType == RprMidiEvent::NoteOn || messageType == RprMidiEvent::NoteOff)
    {
        return countNotes() > 0;
    }

    if(messageType == RprMidiEvent::CC)
    {
        for(int i = 0; i < 128; ++i)
        {
            if( countCCs(i) > 0)
            {
                return true;
            }
        }
    }
    return hasEvent(mOtherEvents, messageType);
}

std::string RprMidiTake::poolGuid() const
{
    const std::string &pooledevts = getMidiSourceNode()->findChildByToken("POOLEDEVTS")->getValue();
    return StringVector{pooledevts}.at(1);
}
