#ifndef __RPRMIDITEMPLATE_HXX
#define __RPRMIDITEMPLATE_HXX

#include <memory>

class RprTake;
class RprItem;
class RprNode;

class RprMidiTemplate {
public:
	RprMidiTemplate(const RprTake &take, bool readOnly);

	RprItem *getParent() {return mParent.get(); }

	virtual ~RprMidiTemplate();
protected:
	RprNode *getMidiSourceNode() { return mMidiSourceNode; }
	void errorOccurred() { mInErrorState = true; }
private:

	std::auto_ptr<RprItem> mParent;
	std::auto_ptr<RprNode> mItemNode;

	RprNode *mMidiSourceNode;

	bool mReadOnly;
	bool mInErrorState;
};

#endif /*__RPRMIDITEMPLATE_HXX */