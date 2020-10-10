#include "stdafx.h"

#include "RprMidiCCLane.h"

#include "RprNode.h"
#include "RprItem.h"
#include "RprTake.h"
#include "RprException.h"
#include "StringUtil.h"

#include <WDL/localize/localize.h>

RprMidiCCLane::RprMidiCCLane(RprTake &midiTake, bool readOnly) : RprMidiTemplate(midiTake, readOnly)
{
    RprNode *midiNode = RprMidiTemplate::getMidiSourceNode();
    for(int i = 0; i < midiNode->childCount(); i++) {
        /* VELLANE 16 56 0 */
        if(midiNode->getChild(i)->getValue().substr(0, 7) == "VELLANE") {
            RprMidiLane midiLane;
            StringVector velLane(midiNode->getChild(i)->getValue());
            midiLane.laneId = ::atoi( velLane.at(1));
            midiLane.height = ::atoi( velLane.at(2));
            mMidiLanes.push_back(midiLane);
        }
    }

    for(int i = 0; i < midiNode->childCount(); i++) {
        if(midiNode->getChild(i)->getValue().substr(0, 7) == "VELLANE") {
            midiNode->removeChild(i);
            i--;
        }
    }
}

RprMidiCCLane::~RprMidiCCLane()
{
    RprNode *midiNode = RprMidiTemplate::getMidiSourceNode();
    int j = midiNode->childCount() - 1;
    for(; j >= 0; j--) {
        if(midiNode->getChild(j)->getValue().find("CFGEDITVIEW ") != std::string::npos)
            break;
    }

    if( j < 0) {
        RprMidiTemplate::errorOccurred();
        return;
    }

    for(std::vector<RprMidiLane>::iterator i = mMidiLanes.begin(); i != mMidiLanes.end(); i++)
        midiNode->addChild(i->toReaper(), j++);
}

bool RprMidiCCLane::isShown(int id) const
{
    for(std::vector<RprMidiLane>::const_iterator i = mMidiLanes.begin(); i != mMidiLanes.end(); i++)
        if(i->laneId == id)
            return true;
    return false;
}

int RprMidiCCLane::countShown() const
{
    return (int)mMidiLanes.size();
}

int RprMidiCCLane::getIdAt(int index) const
{
    const RprMidiLane &lane = *(mMidiLanes.begin() + index);
    return lane.laneId;
}


void RprMidiCCLane::setIdAt(int index, int id)
{
    RprMidiLane &lane = *(mMidiLanes.begin() + index);
    lane.laneId = id;
}

int RprMidiCCLane::getHeightAt(int index) const
{
    const RprMidiLane &lane = *(mMidiLanes.begin() + index);
    return lane.height;
}

void RprMidiCCLane::setHeightAt(int index, int height)
{
    RprMidiLane &lane = *(mMidiLanes.begin() + index);
    lane.height = height;
}

void RprMidiCCLane::remove(int index)
{
    mMidiLanes.erase(mMidiLanes.begin() + index);
}

void RprMidiCCLane::append(int id, int height)
{
    RprMidiLane midiLane;
    midiLane.laneId = id;
    midiLane.height = height;
    mMidiLanes.push_back(midiLane);
}

RprMidiCCLane::RprMidiLane::RprMidiLane() : laneId(false), height(0)
{}

RprNode *RprMidiCCLane::RprMidiLane::toReaper()
{
    std::ostringstream oss;
    oss << "VELLANE";
    oss << " " << laneId;
    oss << " " << height;
    oss << " " << 0;
    return new RprPropertyNode(oss.str());
}

RprMidiCCLanePtr RprMidiCCLane::createFromMidiEditor(bool readOnly)
{
    HWND midiEditor = MIDIEditor_GetActive();
    if(midiEditor == NULL)
        throw RprLibException(__LOCALIZE("No active MIDI editor","sws_mbox"), true);
    RprTake take(MIDIEditor_GetTake(midiEditor));
    RprMidiCCLanePtr laneViewPtr(new RprMidiCCLane(take, readOnly));
    return laneViewPtr;
}

int RprMidiCCLane::getHeight(int id) const
{
    for(std::vector<RprMidiLane>::const_iterator i = mMidiLanes.begin(); i != mMidiLanes.end(); i++)
        if(i->laneId == id)
            return i->height;
    return 0;
}
