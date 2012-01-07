#ifndef __RPRMIDITAKE_H
#define __RPRMIDITAKE_H

#include "RprMidiEvent.hxx"
#include "RprMidiTemplate.hxx"

class RprTake;
class RprMidiEvent;
class RprMidiBase;
class RprMidiContext;
class RprItem;
class RprNode;
class RprMidiTake;

typedef std::auto_ptr<RprMidiTake> RprMidiTakePtr;

class RprMidiNote {
public:
	RprMidiNote(RprMidiContext *context);
	RprMidiNote(RprMidiBase *noteOn, RprMidiBase *noteOff, RprMidiContext *context);
	
	double getPosition() const;
	void setPosition(double position);

	int getChannel() const;
	void setChannel(int channel);

	double getLength() const;
    void setLength(double length);

    void setPitch(int pitch);
    int getPitch() const;

    void setVelocity(int velocity);
    int getVelocity() const;

	bool isSelected() const;
	void setSelected(bool selected);

	bool isMuted() const;
	
	int getItemPosition() const;
	int getItemLength() const;

	void setItemLength(int);

	~RprMidiNote();
private:
	friend class RprMidiTake;
	RprMidiBase *mNoteOn;
	RprMidiBase *mNoteOff;
	RprMidiContext *mContext;
};

class RprMidiCC {
public:
	RprMidiCC(RprMidiContext *context, int controller);
	RprMidiCC(RprMidiBase *cc, RprMidiContext *context);
	
	int getChannel() const;

	int getItemPosition() const;
	
	~RprMidiCC();
private:
	friend class RprMidiTake;
	RprMidiBase *mCC;
	RprMidiContext *mContext;
};

class RprMidiTake : public RprMidiTemplate {
public:
	static RprMidiTakePtr createFromMidiEditor(bool readOnly = false);
	RprMidiTake(const RprTake &take, bool readOnly = false);
    ~RprMidiTake();

	RprMidiNote *getNoteAt(int index) const;
	int countNotes() const;
	RprMidiNote *addNoteAt(int index);

	int countCCs(int controller) const;
	
	bool hasEventType(RprMidiBase::MessageType);

private:
	void cleanup();
	bool apply();

	std::vector<RprMidiNote *> mNotes;
	std::vector<RprMidiCC *> mCCs[128];
	std::vector<RprMidiBase *> mOtherEvents;
	RprMidiContext *mContext;
	int mMidiEventsOffset;
};

#endif
