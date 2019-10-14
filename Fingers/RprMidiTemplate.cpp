#include "stdafx.h"

#include "RprMidiTemplate.h"
#include "RprNode.h"
#include "RprItem.h"
#include "RprTake.h"
#include "RprStateChunk.h"

static RprNode *findTakeSource(RprNode *parent, const std::string &takeGUID )
{
    std::string guidString = "GUID " + takeGUID;
    int i = 0;
    for(; i < parent->childCount(); i++) {
        RprNode *childNode = parent->getChild(i);
        if(childNode->getValue() == guidString)
            break;
    }
    for(; i < parent->childCount(); i++) {
        RprNode *childNode = parent->getChild(i);
        if(childNode->getValue().substr(0,6) == "SOURCE") {
            return childNode;
        }
    }
    return NULL;
}

RprMidiTemplate::RprMidiTemplate(const RprTake &take, bool readOnly)
: mTake(take)
{
    mReadOnly = readOnly;
    mSetNewTakeOffset = false;
    mInErrorState = false;
    mParent.reset(new RprItem(take.getParent()));
    RprStateChunkPtr chunk = mParent->getReaperState();
    mItemNode.reset(RprParentNode::createItemStateTree(chunk->get()));
    char guid[256];
    guidToString(take.getGUID(), guid);
    mMidiSourceNode = findTakeSource(mItemNode.get(), guid);
}

RprMidiTemplate::~RprMidiTemplate()
{
    if(mItemNode.get() == NULL || mParent.get() == NULL)
        return;

    if(!mInErrorState && !mReadOnly) {
        std::string itemState = mItemNode->toReaper();
        GetSetObjectState(mParent->toReaper(), itemState.c_str());
        if (mSetNewTakeOffset)
        {
            mTake.setStartOffset(mNewTakeOffset);
        }
    }
}
