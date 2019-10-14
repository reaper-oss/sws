#ifndef __RPRMIDICCLANE_HXX
#define __RPRMIDICCLANE_HXX

#include "RprMidiTemplate.h"

class RprNode;
class RprItem;
class RprMidiCCLane;
class RprTake;

typedef std::auto_ptr<RprMidiCCLane> RprMidiCCLanePtr;

class RprMidiCCLane : public RprMidiTemplate {
public:
	static RprMidiCCLanePtr createFromMidiEditor(bool readOnly = false);
	RprMidiCCLane(RprTake &midiTake, bool readOnly = false);

	bool isShown(int id) const;

	int getHeight(int id) const;

	int countShown() const;

	int getIdAt(int index) const;
	void setIdAt(int index, int id);

	int getHeightAt(int index) const;
	void setHeightAt(int index, int height);

	void remove(int index);
	void append(int id, int height);

	void toReaper(RprNode *midiNode);
	~RprMidiCCLane();

private:
	class RprMidiLane {
	public:
		RprMidiLane();
		int laneId;
		int height;
		RprNode *toReaper();
	};
	std::vector<RprMidiLane> mMidiLanes;
};

#endif /*__RPRMIDICCLANE_HXX */
