#include "stdafx.h"

#include "RprTake.h"
#include "RprItem.h"
#include "RprTrack.h"
#include "RprException.h"

#include <WDL/localize/localize.h>

RprTake::RprTake(MediaItem_Take *take)
{
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

void RprTake::setStartOffset(double offset)
{
    GetSetMediaItemTakeInfo(mTake, "D_STARTOFFS", &offset);
}

GUID *RprTake::getGUID() const
{
    return (GUID *)GetSetMediaItemTakeInfo(mTake, "GUID", NULL);
}

bool RprTake::isFile()
{
    PCM_source *source = getSource();
    if (!source)
    {
        return false;
    }
    std::string fileName(source->GetFileName());
    return !fileName.empty();
}

bool RprTake::isMIDI()
{
    PCM_source *source = getSource();
    if (!source)
    {
        return false;
    }
    std::string takeType(source->GetType());
    return takeType == "MIDI" || takeType == "MIDIPOOL";
}

void RprTake::setName(const char *name)
{
    GetSetMediaItemTakeInfo(mTake, "P_NAME", (void *)name);
}

const char *RprTake::getName()
{
    return (const char *)GetSetMediaItemTakeInfo(mTake, "P_NAME", NULL);
}

RprTake RprTake::createFromMidiEditor()
{
    RprTake take(MIDIEditor_GetTake(MIDIEditor_GetActive()));
    return take;
}
