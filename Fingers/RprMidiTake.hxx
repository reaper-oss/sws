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
class RprMidiLaneView;

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
	void setMuted(bool muted);

	int getItemPosition() const;
	int getItemLength() const;

	void setItemPosition(int position);
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
	
	double getPosition() const;
	void setPosition(double position);

	int getChannel() const;
	void setChannel(int channel);

	int getController() const;

    void setValue(int pitch);
    int getValue() const;

	bool isSelected() const;
	void setSelected(bool selected);

	bool isMuted() const;
	void setMuted(bool muted);

	int getItemPosition() const;
	void setItemPosition(int position);

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

	RprMidiNote *getNoteAt(int index) const;
	void removeNoteAt(int index);
	int countNotes() const;
	RprMidiNote *addNoteAt(int index);

	int countCCs(int controller) const;
	RprMidiCC *getCCAt(int controller, int index) const;

	void removeCCAt(int controller, int index);
	RprMidiCC *addCCAt(int controller, int index);

	bool hasEventType(RprMidiBase::MessageType);

	double getGridDivision();
	double getNoteDivision();

	static void openInEditor(RprTake &take);
	
	~RprMidiTake();

	class RprMidiTakeConversionException : public std::exception {
	public:
		RprMidiTakeConversionException(std::string message);
		const char *what();
		virtual ~RprMidiTakeConversionException() throw();
	private:
		std::string mMessage;
	};

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
