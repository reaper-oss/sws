#include "stdafx.h"

#include "RprItem.h"
#include "RprTrack.h"


RprTrack::RprTrack(MediaTrack *track)
{
    mTrack = track;
}

MediaTrack* RprTrack::toReaper() const
{
    return mTrack;
}
GUID *RprTrack::getGUID()
{
    return GetTrackGUID(mTrack);
}

bool RprTrack::isMuted()
{
    return *(bool *)GetSetMediaTrackInfo(mTrack, "B_MUTE", NULL);
}

bool RprTrack::isSoloed()
{
    return *(int *)GetSetMediaTrackInfo(mTrack, "I_SOLO", NULL) > 0;
}

void RprTrack::setMuted(bool muted)
{
    GetSetMediaTrackInfo(mTrack, "B_MUTE", (void *)&muted);
}

void RprTrack::setSoloed(bool soloed)
{
    int solo = soloed ? 1 : 0;
    GetSetMediaTrackInfo(mTrack, "I_SOLO", (void *)&solo);
}

bool RprTrack::operator==(RprTrack &rhs)
{
    GUID *lhsGuid = getGUID();
    GUID *rhsGuid = rhs.getGUID();

    return GuidsEqual(lhsGuid,rhsGuid);
}

int RprTrack::getTrackIndex()
{
    return CSurf_TrackToID(mTrack, false);
}

const char *RprTrack::getName()
{
    return (const char *)GetSetMediaTrackInfo(mTrack, "P_NAME", NULL);
}

unsigned long RprTrack::getColour()
{
    unsigned int color = *(unsigned int *)GetSetMediaTrackInfo(mTrack, "I_CUSTOMCOLOR", NULL);
    //color &= (~0x100000);
    return color;
}

RprTrackCtrPtr RprTrackCollec::getSelected()
{
    const int count = CountSelectedTracks(0);

    RprTrackCtrPtr ctr(new RprTrackCtr);
    for(int i = 0; i < count; i++) {
        RprTrack track(GetSelectedTrack(0, i));
        ctr->add(track);
    }
    ctr->sort();
    return ctr;
}

RprTrackCtrPtr RprTrackCollec::getAll()
{
    int count = CountTracks(0);

    RprTrackCtrPtr ctr(new RprTrackCtr);
    for(int i = 0; i < count; i++) {
        RprTrack track(GetTrack(0, i));
        ctr->add(track);
    }
    ctr->sort();
    return ctr;
}
