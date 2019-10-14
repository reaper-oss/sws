#ifndef __RPR_TRACK_HXX
#define __RPR_TRACK_HXX

#include "RprContainer.h"

class MediaTrack;
class RprTrackCtr;

typedef std::auto_ptr<RprTrackCtr> RprTrackCtrPtr;

namespace RprTrackCollec {
    RprTrackCtrPtr getSelected();
    RprTrackCtrPtr getAll();
};

class RprTrack {
public:
    RprTrack(MediaTrack *track);
    MediaTrack *toReaper() const;
    GUID *getGUID();

    bool isMuted();
    bool isSoloed();
    void setMuted(bool muted);
    void setSoloed(bool soloed);

    int getTrackIndex();

    const char *getName();

    unsigned long getColour();

    bool operator==(RprTrack &rhs);

private:
    MediaTrack *mTrack;
};

class RprTrackCtr : public RprContainer<RprTrack> {
public:
    RprTrackCtr() {}
    ~RprTrackCtr() {}
private:
    void doSort() {}
};

#endif /* __RPR_TRACK_HXX */
