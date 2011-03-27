#include "stdafx.h"

#include "RprMidiTake.hxx"
#include "RprStateChunk.hxx"
#include "RprNode.hxx"
#include "RprTake.hxx"
#include "RprMidiEvent.hxx"
#include "StringUtil.hxx"
#include "RprItem.hxx"
#include "TimeMap.h"
#include "RprException.hxx"

typedef std::list<RprMidiBase *> RprMidiEvents;
typedef std::list<RprMidiBase *>::iterator RprMidiEventsIter;
typedef std::list<RprMidiBase *>::const_iterator RprMidiEventsCIter;

/* Class to auto cleanup pointers to RprMidiBase objects stored in a list */
class RprTempMidiEvents {
public:
	RprTempMidiEvents() {}
	RprMidiEvents &get() { return mEvents; }
	~RprTempMidiEvents()
	{
		for(RprMidiEventsIter i = mEvents.begin(); i != mEvents.end(); ++i) {
			delete *i;
		}
	}

private:
	RprMidiEvents mEvents;
};

class RprMidiContext {
	
public:
	static RprMidiContext *createMidiContext(double playRate, double startOffset, int ticksPerQN)
	{
		RprMidiContext *context = new RprMidiContext;
		context->mPlayRate = playRate;
		context->mStartOffset = startOffset;
		context->mTicksPerQN = ticksPerQN;
		return context;
	}

	~RprMidiContext() {}

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
	RprMidiContext() {}
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
	noteOnEvent->setMessageType(RprMidiBase::NoteOn);
	mNoteOn = noteOnEvent;

	RprMidiEvent *noteOffEvent = new RprMidiEvent();
	noteOffEvent->setMessageType(RprMidiBase::NoteOff);
	mNoteOff = noteOffEvent;
	mContext = context;
}

RprMidiNote::RprMidiNote(RprMidiBase *noteOn, RprMidiBase *noteOff, RprMidiContext *context)
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
	int len = getItemLength();

	int unquantizedNoteOn = mNoteOn->getOffset() + mNoteOn->getUnquantizedOffset();
	int unquantizedNoteOff = mNoteOff->getOffset() + mNoteOff->getUnquantizedOffset();

	mNoteOn->setOffset(position);
	mNoteOff->setOffset(position + len);

	mNoteOn->setUnquantizedOffset(unquantizedNoteOn - position);
	mNoteOff->setUnquantizedOffset( unquantizedNoteOff - (position + len));
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
	setItemLength( (int)((rightEdgeOffset - leftEdgeOffset) * (double)mContext->getTicksPerQN() + 0.5));
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
		pitch = 127;
	if (pitch < 0)
		pitch = 0;
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
		velocity = 127;
	if (velocity < 0)
		velocity = 0;
	mNoteOn->setValue2((unsigned char)velocity);
	if(mNoteOff->getMessageType() == RprMidiBase::NoteOn && mNoteOff->getValue2() == 0) {
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
	mCC->setMessageType(RprMidiBase::CC);
	if(controller > 127)
		controller = 127;
	if(controller < 0)
		controller = 0;
	mCC->setValue1((unsigned char)controller);
	mContext = context;
}
RprMidiCC::RprMidiCC(RprMidiBase *cc, RprMidiContext *context)
{
	mCC = cc;
	mContext = context;
}

double RprMidiCC::getPosition() const
{
	return getPositionMidiOffset(mContext, mCC->getOffset());
}

void RprMidiCC::setPosition(double position)
{
	mCC->setOffset(getMidiOffsetPosition(mContext, position));
}

int RprMidiCC::getChannel() const
{
	return mCC->getChannel() + 1;	
}

void RprMidiCC::setChannel(int channel)
{
	mCC->setChannel(channel - 1);
}

int RprMidiCC::getController() const
{
	return mCC->getValue1();
}

void RprMidiCC::setValue(int value)
{
	if (value > 127)
		value = 127;
	if (value < 0)
		value = 0;
	mCC->setValue2((unsigned char)value);
}

int RprMidiCC::getValue() const
{
	return mCC->getValue2();
}

bool RprMidiCC::isSelected() const
{
	return mCC->isSelected();
}
void RprMidiCC::setSelected(bool selected)
{
	mCC->setSelected(selected);
}

bool RprMidiCC::isMuted() const
{
	return mCC->isMuted();
}

void RprMidiCC::setMuted(bool muted)
{
	mCC->setMuted(muted);
}

int RprMidiCC::getItemPosition() const
{
	return mCC->getOffset();
}

void RprMidiCC::setItemPosition(int position)
{
	mCC->setOffset(position);
}

RprMidiCC::~RprMidiCC()
{
	delete mCC;
}

static int getQNValue(RprNode *midiNode)
{
	const std::string &qnString = midiNode->getChild(0)->getValue();
	std::auto_ptr< std::vector<std::string> > tokens(stringTokenize(qnString));
	return ::atoi(tokens->at(2).c_str());
}

static bool sortMidiBase(const RprMidiBase *lhs, const RprMidiBase *rhs)
{
	return rhs->getOffset() > lhs->getOffset();
}

static void finalizeMidiEvents(std::vector< RprMidiBase *> &midiEvents)
{
	std::sort(midiEvents.begin(), midiEvents.end(), sortMidiBase);
	int offset = 0;
	for(std::vector<RprMidiBase *>::iterator i = midiEvents.begin(); i != midiEvents.end(); i++) {
		RprMidiBase *current = *i;
		int delta = current->getOffset() - offset;
		current->setDelta(delta);
		offset += delta;
	}
}

static bool isMidiEvent(const std::string &eventStr) {
	
	if(eventStr.size() <= 3)
		return false;

	switch(eventStr[0]) {
		case 'e':
		case 'x':
		case 'X':
		case 'E':
			break;
		default:
			return false;
	}
	switch(eventStr[1]) {
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

static int clearMidiEventsFromMidiNode(RprNode *parent)
{
	int i = 0;
	for(; i < parent->childCount(); ++i)
		if(isMidiEvent(parent->getChild(i)->getValue()))
			break;
	int offset = i;

	while (isMidiEvent(parent->getChild(i)->getValue())){
		parent->removeChild(i);
	}
	return offset;
}

static void midiEventsToMidiNode(std::vector< RprMidiBase *> &midiEvents, RprNode *midiNode, int offset)
{
	int index = offset;
	for(std::vector<RprMidiBase *>::iterator i = midiEvents.begin(); i != midiEvents.end(); i++) {
		RprMidiBase *current = *i;
		midiNode->addChild(current->toReaper(), index++);	
	}
}

static void getMidiEvents(RprNode *midiNode, RprMidiEvents &midiEvents)
{
	int offset = 0;
	for(int i = 1; i < midiNode->childCount(); i++) {
		if(!isMidiEvent(midiNode->getChild(i)->getValue()))
			continue;
		RprMidiEventCreator creator(midiNode->getChild(i));
		RprMidiBase *midiEvent = creator.collectEvent();
		offset += midiEvent->getDelta();
		midiEvent->setOffset(offset);
		midiEvents.push_back(midiEvent);
	}
}

static bool noteEventsMatch(const RprMidiBase *noteOn, const RprMidiBase *noteOff)
{
	if(noteOn->getChannel() != noteOff->getChannel())
		return false;
	if(noteOn->getValue1() != noteOff->getValue1())
		return false;
	if(noteOn->getOffset() > noteOff->getOffset())
		return false;
	return true;
}

static bool getMidiCCs(RprMidiEvents &midiEvents, std::vector<RprMidiCC *> *midiCCs, RprMidiContext *context)
{
	RprMidiEvents other;
	for(RprMidiEventsCIter i = midiEvents.begin(); i != midiEvents.end(); ++i) {
		RprMidiBase *current = *i;
		if(current->getMessageType() == RprMidiBase::CC) {
			midiCCs[current->getValue1()].push_back(new RprMidiCC(current, context));
		} else {
			other.push_back(current);
		}
	}
	midiEvents = other;
	return true;
}

static bool getMidiNotes(RprMidiEvents &midiEvents, std::vector<RprMidiNote *> &midiNotes, RprMidiContext *context)
{
	RprMidiEvents noteOns;
	RprMidiEvents noteOffs;
	RprMidiEvents other;

	/* categorize notes into note-ons and note-offs */
	for(RprMidiEventsCIter i = midiEvents.begin(); i != midiEvents.end(); ++i) {
		RprMidiBase *current = *i;
		if(current->getMessageType() == RprMidiBase::NoteOn && current->getValue2() != 0)
			noteOns.push_back(current);
		else if(current->getMessageType() == RprMidiBase::NoteOff || (current->getMessageType() == RprMidiBase::NoteOn && current->getValue2() == 0)) 
			noteOffs.push_back(current);
	    else
			other.push_back(current);
	}
	midiEvents.clear();
	
	/* match note-ons and note-offs, removing zero length notes */
	// SWS Jan 26 2011 - GCC doesn't like the const_iterators here for the .erase() calls, but MSVC does.  I don't think it's too important.
	RprMidiEventsIter i = noteOns.begin();
	while(i != noteOns.end()) {
		RprMidiEventsIter j = noteOffs.begin();
		while(j != noteOffs.end() && !noteEventsMatch(*i, *j)) {
			++j;
		}
		/* no match so add noteOn to other events */
		if(j == noteOffs.end()) {
			other.push_back(*i);
			++i;
			continue;
		}

		RprMidiBase *noteOn = *i;
		RprMidiBase *noteOff = *j;
		/* delete zero length notes */
		if(noteOn->getOffset() == noteOff->getOffset()) {
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
		midiEvents.push_back(*j);
	for(RprMidiEventsCIter j = other.begin(); j != other.end(); j++)
		midiEvents.push_back(*j);
	return true;
}

template
<typename T>
class vectorRemoval {
public:
	vectorRemoval(std::vector<T *> *sourceVector)
	{ mSourceVector = sourceVector; }

	void push(T *item)
	{
		toDelete.push_back(item);								
	}
	
	~vectorRemoval()
	{
		if(toDelete.empty())
			return;

		for(typename std::list<T *>::iterator i = toDelete.begin(); i != toDelete.end(); ++i) {
			typename std::vector<T *>::iterator j = std::find(mSourceVector->begin(), mSourceVector->end(), *i);
			if(j != mSourceVector->end())
				mSourceVector->erase(j);
		}

		while(!toDelete.empty()) {
			typename std::list<T *>::iterator i = toDelete.begin();
			delete *i;
			toDelete.erase(i);
		}
	}
private:
	std::list<T *> toDelete;
	std::vector<T *> *mSourceVector;
};

static void removeDuplicates(std::vector<RprMidiCC *> *midiCCs)
{
	for(int i = 0; i < 128; i++) {
		vectorRemoval<RprMidiCC> removal(&midiCCs[i]);
		for(std::vector<RprMidiCC *>::iterator j = midiCCs[i].begin(); j != midiCCs[i].end(); ++j) {
			for(std::vector<RprMidiCC *>::iterator k = j + 1; k != midiCCs[i].end(); ++k) {
				if( (*k)->getItemPosition() != (*j)->getItemPosition())
					break;
				if( (*k)->getChannel() != (*j)->getChannel())
					break;
				removal.push(*k);
			}
		}
	}
}

static void removeDuplicates(std::vector<RprMidiNote *> &midiNotes)
{
	vectorRemoval<RprMidiNote> removal(&midiNotes);

	for(std::vector<RprMidiNote *>::iterator i = midiNotes.begin(); i != midiNotes.end(); i++) {
		RprMidiNote *lhs = *i;
		for(std::vector<RprMidiNote *>::iterator j = i + 1; j != midiNotes.end(); j++) {
			RprMidiNote *rhs = *j;
			if(lhs->getItemPosition() != rhs->getItemPosition())
				continue;
			if(lhs->getPitch() != rhs->getPitch())
				continue;
			if(lhs->getChannel() != rhs->getChannel())
				continue;
			if(lhs->getItemLength() > rhs->getItemLength())
				removal.push(rhs);
			else
				removal.push(lhs);
		}
	}
}

static void removeOverlaps(std::vector<RprMidiNote *> &midiNotes)
{
	vectorRemoval<RprMidiNote> removal(&midiNotes);
	for(std::vector<RprMidiNote *>::iterator i = midiNotes.begin(); i != midiNotes.end(); i++) {
		RprMidiNote *lhs = *i;
		for(std::vector<RprMidiNote *>::iterator j = i + 1; j != midiNotes.end(); j++) {
			RprMidiNote *rhs = *j;
			if(lhs->getPitch() != rhs->getPitch())
				continue;
			if(lhs->getChannel() != rhs->getChannel())
				continue;
			if(lhs->getItemPosition() + lhs->getItemLength() >= rhs->getItemPosition()) {
				int lhsLength = rhs->getItemPosition() - lhs->getItemPosition() - 1;
				if(lhsLength <= 0) {
					removal.push(lhs);
				} else {
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

void RprMidiTake::removeNoteAt(int index)
{
	delete mNotes.at(index);
	mNotes.erase(mNotes.begin() + index);
}

void RprMidiTake::removeCCAt(int controller, int index)
{
	delete mCCs[controller].at(index);
	mCCs[controller].erase(mCCs[controller].begin() + index);
}

RprMidiCC *RprMidiTake::addCCAt(int controller, int index)
{
	RprMidiCC *cc = new RprMidiCC(mContext, controller);
	mCCs[controller].insert(mCCs[controller].begin() + index , cc);
	return cc;
}

RprMidiTake::RprMidiTake(const RprTake &take, bool readOnly) : RprMidiTemplate(take, readOnly)
{
	try {
		mContext = NULL;
		RprNode *sourceNode = RprMidiTemplate::getMidiSourceNode();
	
		RprTempMidiEvents tempMidiEvents;
		getMidiEvents(sourceNode, tempMidiEvents.get());
		mContext = RprMidiContext::createMidiContext(take.getPlayRate(),
		getParent()->getPosition() - take.getStartOffset(),
		getQNValue(sourceNode));
	
		if(!getMidiNotes(tempMidiEvents.get(), mNotes, mContext)) {
			throw RprLibException("Unable to parse MIDI data");
		}
		if(!getMidiCCs(tempMidiEvents.get(), mCCs, mContext)) {
			throw RprLibException("Unable to parse MIDI data");
		}
	
		mOtherEvents.resize(tempMidiEvents.get().size());
		std::copy(tempMidiEvents.get().begin(), tempMidiEvents.get().end(), mOtherEvents.begin());
		tempMidiEvents.get().clear();
		mMidiEventsOffset = clearMidiEventsFromMidiNode(sourceNode);

	} catch (RprMidiBase::RprMidiException &e) {
		cleanup();
		/* throw RprLibException so we let the user know something bad
		 * happened. */
		throw RprLibException(e.what(), true);
	}
}

template
<typename T>
void cleanUpPointers(std::vector<T *> &pointers)
{
	for(typename std::vector<T *>::iterator i = pointers.begin(); i != pointers.end(); i++)
		delete *i;
	pointers.clear();
}

RprMidiTake::~RprMidiTake()
{
	std::vector<RprMidiBase *> midiEvents;
	std::sort(mNotes.begin(), mNotes.end(), compareMidiPositions<RprMidiNote>);
	for(int i = 0; i < 128; i++)
		std::sort(mCCs[i].begin(), mCCs[i].end(), compareMidiPositions<RprMidiCC>);
	removeDuplicates(mNotes);
	removeDuplicates(mCCs);
	removeOverlaps(mNotes);
	midiEvents.reserve(mNotes.size() * 2 + mOtherEvents.size());
	for(std::vector<RprMidiNote *>::const_iterator i = mNotes.begin(); i != mNotes.end(); ++i) {
		midiEvents.push_back((*i)->mNoteOn);
		midiEvents.push_back((*i)->mNoteOff);
	}
	for(int j = 0; j < 128; j++) {
		for(std::vector<RprMidiCC *>::const_iterator i = mCCs[j].begin(); i != mCCs[j].end(); ++i)
			midiEvents.push_back((*i)->mCC);
	}
	for(std::vector<RprMidiBase *>::const_iterator i = mOtherEvents.begin(); i != mOtherEvents.end(); ++i)
		midiEvents.push_back(*i);
	finalizeMidiEvents(midiEvents);
	midiEventsToMidiNode(midiEvents, RprMidiTemplate::getMidiSourceNode(), mMidiEventsOffset);

	cleanup();
}

void RprMidiTake::cleanup()
{
	if(mContext)
		delete mContext;
	mContext = NULL;

	for(int j = 0; j < 128; j++)
		cleanUpPointers(mCCs[j]);
	cleanUpPointers(mOtherEvents);
	cleanUpPointers(mNotes);
}


RprMidiTake::RprMidiTakeConversionException::RprMidiTakeConversionException(std::string message)
{
	mMessage = message;
}
const char *RprMidiTake::RprMidiTakeConversionException::what()
{
	return mMessage.c_str();
}

RprMidiTake::RprMidiTakeConversionException::~RprMidiTakeConversionException() throw()
{}

RprMidiTakePtr RprMidiTake::createFromMidiEditor(bool readOnly)
{
	void *midiEditor = MIDIEditor_GetActive();
	if(midiEditor == NULL)
		throw RprLibException("No active MIDI editor", true);
	RprTake take(MIDIEditor_GetTake(midiEditor));
	const char *sourceFilename = take.getSource()->GetFileName();
	if(!*sourceFilename) {
		RprMidiTakePtr takePtr(new RprMidiTake(take, readOnly));
		return takePtr;
	}
	throw RprLibException("Only in-project MIDI can be modified", true);
}

double RprMidiTake::getGridDivision()
{
	RprNode *midiNode = RprMidiTemplate::getMidiSourceNode();
	for(int i = 0; i < midiNode->childCount(); i++) {
		if(midiNode->getChild(i)->getValue().find("CFGEDIT ") != std::string::npos) {
			const std::string &cfgedit = midiNode->getChild(i)->getValue();
			double gridDivision = 0.0;
			
			sscanf(cfgedit.c_str(), "CFGEDIT %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %lf", &gridDivision);
			return gridDivision;
		}
	}
	return 0.0;
}

double RprMidiTake::getNoteDivision()
{
	RprNode *midiNode = RprMidiTemplate::getMidiSourceNode();
	for(int i = 0; i < midiNode->childCount(); i++) {
		if(midiNode->getChild(i)->getValue().find("CFGEDIT ") != std::string::npos) {
			const std::string &cfgedit = midiNode->getChild(i)->getValue();
			double noteDivision = 0.0;
			sscanf(cfgedit.c_str(), "CFGEDIT %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*f %*d %*d %*d %*d %*d %*d %*d %*d %lf", &noteDivision);
			return noteDivision;
		}
	}
	return 0.0;
}

int RprMidiTake::countCCs(int controller) const
{
	return (int)mCCs[controller].size();
}

static bool hasEvent(std::vector<RprMidiBase *> &midiEvents, RprMidiBase::MessageType messageType)
{
	for(std::vector<RprMidiBase *>::const_iterator i = midiEvents.begin(); i != midiEvents.end(); ++i) {
		RprMidiBase *midiEvent = *i;
		if(midiEvent->getMessageType() == messageType)
			return true;
	}
	return false;
}

bool RprMidiTake::hasEventType(RprMidiBase::MessageType messageType)
{
	if(messageType == RprMidiBase::NoteOn || messageType == RprMidiBase::NoteOff)
		return countNotes() > 0;

	if(messageType == RprMidiBase::CC) {
		for(int i = 0; i < 128; ++i)
			if( countCCs(i) > 0)
				return true;
	}
	return hasEvent(mOtherEvents, messageType);
}

RprMidiCC *RprMidiTake::getCCAt(int controller, int index) const
{
	return mCCs[controller].at(index);
}

