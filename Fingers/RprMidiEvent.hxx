#ifndef __RPRMIDIEVENT_HXX
#define __RPRMIDIEVENT_HXX
#include <vector>
#include <string>
#include <exception>

class RprNode;

class RprMidiBase {
public:
	enum MessageType {NoteOff, NoteOn, KeyPressure, CC, ProgramChange, ChannelPressure, PitchBend, Sysex, TextEvent, Unknown};
	RprMidiBase();

	bool isSelected() const;
	void setSelected( bool selected);
	
	bool isMuted() const;
	void setMuted(bool muted);
	
	int getDelta() const;
	void setDelta(int delta);

	virtual MessageType getMessageType() const;
	virtual void setMessageType(MessageType messageType);

	int getOffset() const;
	void setOffset(int offset);

	virtual void setChannel(unsigned char channel);
	virtual unsigned char getChannel() const;

	virtual unsigned char getValue1() const;
	virtual void setValue1(unsigned char value);

	virtual unsigned char getValue2() const;
	virtual void setValue2(unsigned char value);

	const std::string& getExtendedData() const;

	virtual RprNode *toReaper() = 0;

	virtual ~RprMidiBase() {}

	class RprMidiException {
	public:
		RprMidiException(const char *message);
		const char *what();
		virtual ~RprMidiException() throw();
	private:
		std::string mMessage;
	};

private:
	int mDelta;
	int mOffset;
	bool mMuted;
	bool mSelected;
};

class RprExtendedMidiEvent : public RprMidiBase {
public:
	RprExtendedMidiEvent();
	const std::string& getExtendedData();
	void setExtendedData(const std::string &data);
	MessageType getMessageType() const;
	RprNode *toReaper();
private:
	std::string mExtendedData;
};

class RprMidiEvent : public RprMidiBase {
public:
	RprMidiEvent();
	void setMidiMessage(const std::vector<unsigned char> message);
	const std::vector<unsigned char>& getMidiMessage();

	MessageType getMessageType() const;
	void setMessageType(MessageType messageType);

	unsigned char getValue1() const;
	void setValue1(unsigned char value);
	unsigned char getValue2() const;
	void setValue2(unsigned char value);

	unsigned char getChannel() const;
	void setChannel(unsigned char channel);

	RprNode *toReaper();

private:
	std::vector<unsigned char> mMidiMessage;
};

class RprMidiEventCreator {
public:
	RprMidiEventCreator(RprNode *node);
	RprExtendedMidiEvent *getExtended();
	RprMidiEvent *getEvent();
	RprMidiBase *getBaseEvent();
private:
	RprMidiEvent *mEvent;
	RprExtendedMidiEvent *mXEvent;
};

#endif /*__RPRMIDIEVENT_HXX */