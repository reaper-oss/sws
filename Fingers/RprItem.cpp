#include "stdafx.h"

#include "RprItem.h"
#include "RprTake.h"
#include "RprTrack.h"
#include "RprStateChunk.h"


template
<typename T>
static void setOnMediaItem(MediaItem* item, const char *prop, T newValue)
{
    GetSetMediaItemInfo(item, prop, (void *)&newValue);
}

static double getDoubleMediaItem(MediaItem* item, const char *prop)
{
    return *(double*)GetSetMediaItemInfo(item, prop, NULL);
}

static int getIntMediaItem(MediaItem* item, const char *prop)
{
    return *(int*)GetSetMediaItemInfo(item, prop, NULL);
}

RprItem::RprItem(MediaItem *item)
{
    mItem = item;
}

MediaItem *RprItem::toReaper() const
{
    return mItem;
}

double RprItem::getPosition() const
{
    return getDoubleMediaItem(mItem, "D_POSITION");
}

void RprItem::setPosition(double position)
{
    setOnMediaItem(mItem, "D_POSITION", position);
}

double RprItem::getLength() const
{
    return getDoubleMediaItem(mItem, "D_LENGTH");
}
void RprItem::setLength(double length)
{
    setOnMediaItem(mItem, "D_LENGTH", length);
}

int RprItem::takeCount() const
{
    return CountTakes(mItem);
}

RprTake RprItem::getActiveTake() const
{
    RprTake activeTake(GetActiveTake(mItem));
    return activeTake;
}

int RprItem::getActiveTakeIndex() const
{
    return getIntMediaItem(mItem, "I_CURTAKE");
}

RprStateChunkPtr RprItem::getReaperState() const
{
    RprStateChunkPtr ptr(new RprStateChunk(GetSetObjectState(mItem, "")));
    return ptr;
}

void RprItem::setReaperState(const RprStateChunkPtr &stateChunk)
{
    setReaperState(stateChunk->get());
}

void RprItem::setReaperState(const char *stateChunk)
{
    GetSetObjectState(mItem, stateChunk);
    UpdateItemInProject(mItem);
}

RprTrack RprItem::getTrack()
{
    return RprTrack((MediaTrack *)GetSetMediaItemInfo(mItem, "P_TRACK", NULL));
}

RprItem::RprItem(const RprItem &item)
{
    mItem = item.toReaper();
}

void RprItem::setSelected(bool selected)
{
    setOnMediaItem(mItem, "B_UISEL", selected);
}

void RprItem::setTrack(const RprTrack &track)
{
    MoveMediaItemToTrack(mItem, track.toReaper());
}

double RprItem::getSnapOffset() const
{
    return getDoubleMediaItem(mItem, "D_SNAPOFFSET");
}

class RprItemCtrPriv {
public:
    std::vector<RprItem> mItems;
    void sort();
};

static bool sortByPosition(const RprItem &lhs, const RprItem &rhs)
{
    return lhs.getPosition() < rhs.getPosition();
}

void RprItemCtr::doSort()
{
    std::sort(mItems.begin(), mItems.end(), sortByPosition);
}

RprItemCtrPtr RprItemCollec::getSelected()
{
    const int count = CountSelectedMediaItems(0);
    
    RprItemCtrPtr ctr(new RprItemCtr);
    for(int i = 0; i < count; i++) {
        RprItem item(GetSelectedMediaItem(0, i));
        ctr->add(item);         
    }
    ctr->sort();
    return ctr;
}

RprItemCtrPtr RprItemCollec::getOnTrack(RprTrack &track)
{
    int count = CountTrackMediaItems(track.toReaper());

    RprItemCtrPtr ctr(new RprItemCtr);
    
    for(int i = 0; i < count; i++) {
        RprItem item(GetTrackMediaItem(track.toReaper(), i));
        ctr->add(item);         
    }
    ctr->sort();
    return ctr;

}


