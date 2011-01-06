#include "stdafx.h"

#include <string>

#include "RprTake.hxx"
#include "RprItem.hxx"
#include "RprTrack.hxx"
#include "RprException.hxx"

#ifndef NULL
#define NULL 0
#endif

// P_TRACK : pointer to MediaTrack (read-only)
// P_ITEM : pointer to MediaItem (read-only)
// P_SOURCE : PCM_source *. Note that if setting this, you should first retrieve the old source, set the new, THEN delete the old.
// GUID : GUID * : 16-byte GUID, can query or update
// P_NAME : char * to take name
// D_STARTOFFS : double *, start offset in take of item
// D_VOL : double *, take volume
// D_PAN : double *, take pan
// D_PANLAW : double *, take pan law (-1.0=default, 0.5=-6dB, 1.0=+0dB, etc)
// D_PLAYRATE : double *, take playrate (1.0=normal, 2.0=doublespeed, etc)
// D_PITCH : double *, take pitch adjust (in semitones, 0.0=normal, +12 = one octave up, etc)
// B_PPITCH, bool *, preserve pitch when changing rate
// I_CHANMODE, int *, channel mode (0=normal, 1=revstereo, 2=downmix, 3=l, 4=r)
// I_PITCHMODE, int *, pitch shifter mode, -1=proj default, otherwise high word=shifter low word = parameter
extern void* (*GetSetMediaItemTakeInfo)(MediaItem_Take* tk, const char* parmname, void* setNewValue);
extern PCM_source* (*PCM_Source_CreateFromFile)(const char* filename);
extern void* (*MIDIEditor_GetActive)();
extern MediaItem_Take* (*MIDIEditor_GetTake)(void* midieditor);

RprTake::RprTake(MediaItem_Take *take)
{
	if(take == NULL)
		throw RprLibException("Media Item take is NULL", false);
	mTake = take;
}

MediaItem_Take* RprTake::toReaper() const
{
	return mTake;
}

double RprTake::getPlayRate() const
{
	double playRate = *(double *)GetSetMediaItemTakeInfo(mTake, "D_PLAYRATE", NULL);
	return playRate;
}

void RprTake::setPlayRate(double playRate)
{
	GetSetMediaItemTakeInfo(mTake, "D_PLAYRATE", (void *)&playRate);
}

RprItem RprTake::getParent() const
{
	RprItem item((MediaItem *)GetSetMediaItemTakeInfo(mTake, "P_ITEM", NULL));
	return item;
}

PCM_source *RprTake::getSource()
{
	return (PCM_source *)GetSetMediaItemTakeInfo(mTake, "P_SOURCE", NULL);
}

double RprTake::getStartOffset() const
{
	return *(double *)GetSetMediaItemTakeInfo(mTake, "D_STARTOFFS", NULL);
}

GUID *RprTake::getGUID() const
{
	return (GUID *)GetSetMediaItemTakeInfo(mTake, "GUID", NULL);
}

bool RprTake::isFile()
{
	PCM_source *source = getSource();
	std::string fileName(source->GetFileName());
	return !fileName.empty();
}

bool RprTake::isMIDI()
{
	PCM_source *source = getSource();
	std::string takeType(source->GetType());
	return takeType == "MIDI";
}

void RprTake::setName(const char *name)
{
	GetSetMediaItemTakeInfo(mTake, "P_NAME", (void *)name);
}

PCM_source *RprTake::createSource(const char *fileName)
{
	return PCM_Source_CreateFromFile(fileName);
}

void RprTake::setSource(PCM_source *source, bool keepOld)
{
	PCM_source *oldSource = getSource();
	GetSetMediaItemTakeInfo(mTake, "P_SOURCE", (void *)source);
	if(!keepOld)
		delete oldSource;
}

void RprTake::openEditor()
{
	void *midiEditor = MIDIEditor_GetActive();
	if(isMIDI() && midiEditor) {
		getSource()->Extended(PCM_SOURCE_EXT_OPENEDITOR, midiEditor, (void *)getParent().getTrack().getTrackIndex(), (void *)getName());
	}
}

const char *RprTake::getName()
{
	return (const char *)GetSetMediaItemTakeInfo(mTake, "P_NAME", NULL);
}

RprTake RprTake::createFromMidiEditor()
{
	void *midiEditor = MIDIEditor_GetActive();
	RprTake take(MIDIEditor_GetTake(midiEditor));
	return take;
}
