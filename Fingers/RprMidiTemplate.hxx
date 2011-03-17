#ifndef __RPRMIDITEMPLATE_HXX
#define __RPRMIDITEMPLATE_HXX

class RprTake;
class RprItem;
class RprNode;

class RprMidiTemplate {
public:
	RprMidiTemplate(const RprTake &take, bool readOnly);

	RprItem *getParent() {return mParent; }

	virtual ~RprMidiTemplate();
protected:
	RprNode *getMidiSourceNode() { return mMidiSourceNode; }
	void errorOccurred() { mInErrorState = true; }
private:
	RprItem *mParent;
	RprNode *mItemNode;
	RprNode *mMidiSourceNode;
	bool mReadOnly;
	bool mInErrorState;
};

#endif /*__RPRMIDITEMPLATE_HXX */