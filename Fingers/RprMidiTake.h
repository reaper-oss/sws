#ifndef __RPRMIDITAKE_H
#define __RPRMIDITAKE_H

#include "RprMidiEvent.h"
#include "RprMidiTemplate.h"

class RprMidiEvent;
class RprMidiContext;
class RprItem;
class RprNode;
class RprMidiTake;
class RprMidiNote;

// Reascript functions

// Allocates a RprMidiTake from a take pointer. Returns
// 0 if take is not a MIDI take.
RprMidiTake* FNG_AllocMidiTake(MediaItem_Take* take);

// Deletes and commits any changes to the RprMidiTake
void FNG_FreeMidiTake(RprMidiTake* midiTake);

// Count how many MIDI notes are in the midi take
int FNG_CountMidiNotes(RprMidiTake* midiTake);

// Get MIDI note from MIDI take at specified index
RprMidiNote* FNG_GetMidiNote(RprMidiTake* midiTake, int index);

// Get and set properties on a Midi Note
int FNG_GetMidiNoteIntProperty(RprMidiNote* midiNote, const char* property);
void FNG_SetMidiNoteIntProperty(RprMidiNote* midiNote, const char* property, int value);

// Add a midi note to the take
RprMidiNote* FNG_AddMidiNote(RprMidiTake* midiTake);


typedef std::auto_ptr<RprMidiTake> RprMidiTakePtr;

class RprMidiNote
{
public:
    RprMidiNote(RprMidiContext *context);
    RprMidiNote(RprMidiEvent *noteOn, RprMidiEvent *noteOff, RprMidiContext *context);

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
    void setItemPosition(int position);

    int getItemLength() const;
    void setItemLength(int);

    ~RprMidiNote();
private:
    friend class RprMidiTake;

    RprMidiEvent *mNoteOn;
    RprMidiEvent *mNoteOff;
    RprMidiContext *mContext;
};

class RprMidiCC
{
public:
    RprMidiCC(RprMidiContext *context, int controller);
    RprMidiCC(RprMidiEvent *cc, RprMidiContext *context);

    int getChannel() const;

    int getItemPosition() const;

    ~RprMidiCC();
private:
    friend class RprMidiTake;
    RprMidiEvent *mCC;
    RprMidiContext *mContext;
};

class RprMidiTake : public RprMidiTemplate
{
public:
    static RprMidiTakePtr createFromMidiEditor(bool readOnly = false);
    RprMidiTake(const RprTake &take, bool readOnly = false);
    ~RprMidiTake();

    RprMidiNote *getNoteAt(int index) const;
    int countNotes() const;
    RprMidiNote *addNoteAt(int index);

    int countCCs(int controller) const;

    bool hasEventType(RprMidiEvent::MessageType);

    std::string poolGuid() const;

private:
    void cleanup();
    bool apply();

    std::vector<RprMidiNote *> mNotes;
    std::vector<RprMidiCC *> mCCs[128];
    std::vector<RprMidiEvent *> mOtherEvents;
    RprMidiContext *mContext;
    int mMidiEventsOffset;
};

#endif
