#include "stdafx.h"

#include <vector>
#include <algorithm>

#include "RprItem.hxx"
#include "RprTake.hxx"
#include "RprTrack.hxx"
#include "RprStateChunk.hxx"

#ifndef NULL
#define NULL 0
#endif

class ReaProject;
class MediaItem_Take;


// P_TRACK : MediaTrack * (read only)
// B_MUTE : bool * to muted state
// B_LOOPSRC : bool * to loop source
// B_ALLTAKESPLAY : bool * to all takes play
// B_UISEL : bool * to ui selected
// C_BEATATTACHMODE : char * to one char of beat attached mode, -1=def, 0=time, 1=allbeats, 2=beatsosonly
// C_LOCK : char * to one char of lock flags (&1 is locked, currently)
// D_VOL : double * of item volume (volume bar)
// D_POSITION : double * of item position (seconds)
// D_LENGTH : double * of item length (seconds)
// D_SNAPOFFSET : double * of item snap offset (seconds)
// D_FADEINLEN : double * of item fade in length (manual, seconds)
// D_FADEOUTLEN : double * of item fade out length (manual, seconds)
// D_FADEINLEN_AUTO : double * of item autofade in length (seconds, -1 for no autofade set)
// D_FADEOUTLEN_AUTO : double * of item autofade out length (seconds, -1 for no autofade set)
// C_FADEINSHAPE : int * to fadein shape, 0=linear, ...
// C_FADEOUTSHAPE : int * to fadeout shape
// I_GROUPID : int * to group ID (0 = no group)
// I_LASTY : int * to last y position in track (readonly)
// I_LASTH : int * to last height in track (readonly)
// I_CUSTOMCOLOR : int * : custom color, windows standard color order (i.e. RGB(r,g,b)|0x100000). if you do not |0x100000, then it will not be used (though will store the color anyway)
// I_CURTAKE : int * to active take
// F_FREEMODE_Y : float * to free mode y position (0..1)
// F_FREEMODE_H : float * to free mode height (0..1)
extern void* (*GetSetMediaItemInfo)(MediaItem* item, const char* parmname, void* setNewValue);
extern int (*CountTakes)(MediaItem* item);
extern MediaItem_Take* (*GetActiveTake)(MediaItem* item);
extern MediaItem* (*GetSelectedMediaItem)(ReaProject* proj, int selitem);
extern int (*CountSelectedMediaItems)(ReaProject* proj);
extern char* (*GetSetObjectState)(void* obj, const char* str);
extern void (*UpdateItemInProject)(MediaItem* item);
extern bool (*MoveMediaItemToTrack)(MediaItem* item, MediaTrack* desttr);
extern MediaItem* (*GetTrackMediaItem)(MediaTrack* tr, int itemidx);
extern int (*CountTrackMediaItems)(MediaTrack* track);

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
	setReaperState(stateChunk->toReaper());
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
	int count = CountSelectedMediaItems(0);
	
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


