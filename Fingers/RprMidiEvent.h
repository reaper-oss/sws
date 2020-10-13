#ifndef __RPRMIDIEVENT_HXX
#define __RPRMIDIEVENT_HXX

#include <memory>

class RprNode;

class RprMidiEvent {
public:
    enum MessageType { NoteOff, NoteOn, KeyPressure, CC, ProgramChange, ChannelPressure, PitchBend,
                       Sysex, TextEvent, NotationEvent = TextEvent, Unknown };
    RprMidiEvent();

    bool isSelected() const;
    void setSelected( bool selected);

    bool isMuted() const;
    void setMuted(bool muted);

    int getDelta() const;
    void setDelta(int delta);

    virtual MessageType getMessageType() const;
    void setMessageType(MessageType messageType);

    int getOffset() const;
    void setOffset(int offset);

    void setChannel(unsigned char channel);
    unsigned char getChannel() const;

    unsigned char getValue1() const;
    void setValue1(unsigned char value);

    unsigned char getValue2() const;
    void setValue2(unsigned char value);

    int getUnquantizedOffset() const;
    void setUnquantizedOffset(int offset);

    void setMidiMessage(const std::vector<unsigned char> message);
    const std::vector<unsigned char>& getMidiMessage();

    bool isAttachableTo(const RprMidiEvent *) const;
    void addAttachedEvent(RprMidiEvent *);

    void addPropertyNode(const RprNode *);

    virtual RprNode *toReaper();

    virtual ~RprMidiEvent() {}

    class RprMidiException {
    public:
        RprMidiException(const char *message);
        const char *what();
        virtual ~RprMidiException() throw();
    private:
        std::string mMessage;
    };

private:
    std::vector<unsigned char> mMidiMessage;
    std::list<RprMidiEvent *> mAttachedEvents;
    std::list<std::string> mPropertyLines;

    int mQuantizeOffset;
    int mDelta;
    int mOffset;
    bool mMuted;
    bool mSelected;
};

class RprExtendedMidiEvent : public RprMidiEvent
{
public:
    RprExtendedMidiEvent();

    void addExtendedData(const std::string &data);

    virtual MessageType getMessageType() const;

    virtual RprNode *toReaper();

private:
    std::list<std::string> mExtendedData;
};

class RprMidiEventCreator
{
public:
    RprMidiEventCreator(RprNode *node);
    /* Collect midi Event. Once you have collected it you
     * take ownership of it. */
    RprMidiEvent *collectEvent();
    ~RprMidiEventCreator();
private:
    std::auto_ptr<RprMidiEvent> mEvent;
    std::auto_ptr<RprExtendedMidiEvent> mXEvent;
};

#endif /*__RPRMIDIEVENT_HXX */
