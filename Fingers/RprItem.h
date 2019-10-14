#ifndef __RPR_ITEM_HXX
#define __RPR_ITEM_HXX

#include <memory>

#include "RprContainer.h"

class MediaItem;
class RprItemCtr;
class RprTake;
class RprStateChunk;
class RprTrack;

typedef std::auto_ptr<RprItemCtr> RprItemCtrPtr;
typedef std::auto_ptr<RprStateChunk> RprStateChunkPtr;

namespace RprItemCollec {
	RprItemCtrPtr getSelected();
	RprItemCtrPtr getOnTrack(RprTrack &track);
};

class RprItem {
public:
	RprItem(MediaItem *item);
	RprItem(const RprItem &item);
	
	/* Conversion */
	MediaItem *toReaper() const;

	double getPosition() const;
	void setPosition(double position);

	double getLength() const;
	void setLength(double length);

	double getSnapOffset() const;

	void setSelected(bool selected);

	int takeCount() const;
	int getActiveTakeIndex() const;
	RprTake getActiveTake() const;

	RprStateChunkPtr getReaperState() const;
	void setReaperState(const RprStateChunkPtr &stateChunk);
	void setReaperState(const char *stateChunk);

	RprTrack getTrack();
	void setTrack(const RprTrack &track);

private:
	MediaItem *mItem;
};

class RprItemCtr : public RprContainer<RprItem> {
public:
	RprItemCtr() {}
	~RprItemCtr() {}
private:
	void doSort();
};

#endif /*__RPR_ITEM_HXX */
