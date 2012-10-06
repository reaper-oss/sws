#ifndef __RPRMIDIEVENT_HXX
#define __RPRMIDIEVENT_HXX

#include <memory>

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

	virtual int getUnquantizedOffset() const;
	virtual void setUnquantizedOffset(int offset);

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
	void addExtendedData(const std::string &data);
	MessageType getMessageType() const;
	RprNode *toReaper();
private:
	std::list<std::string> mExtendedData;
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

	int getUnquantizedOffset() const;
	void setUnquantizedOffset(int offset);

	unsigned char getChannel() const;
	void setChannel(unsigned char channel);

	RprNode *toReaper();

private:
	std::vector<unsigned char> mMidiMessage;
	int mQuantizeOffset;
};

class RprMidiEventCreator {
public:
	RprMidiEventCreator(RprNode *node);
	/* Collect midi Event. Once you have collected it you
	 * take ownership of it. */
	RprMidiBase *collectEvent();
	~RprMidiEventCreator();
private:
	std::auto_ptr<RprMidiEvent> mEvent;
	std::auto_ptr<RprExtendedMidiEvent> mXEvent;
};

#endif /*__RPRMIDIEVENT_HXX */
