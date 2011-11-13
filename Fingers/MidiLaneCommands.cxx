#include "stdafx.h"

#include "MidiLaneCommands.hxx"
#include "CommandHandler.h"
#include "RprItem.hxx"
#include "RprTake.hxx"
#include "RprMidiCCLane.hxx"
#include "RprMidiEvent.hxx"
#include "RprMidiTake.hxx"

static const int defaultHeight = 67;

static void CycleThroughMidiLanes(int flag, void *data);
static void ShowUsedCCLanes(int flag, void *data);
static void HideUnusedCCLanes(int flag, void *data);
static void ShowOnlyTopCCLane(int flag, void *data);

void MidiLaneCommands::Init()
{
	RprCommand::registerCommand("SWS/FNG MIDI: cycle through CC lanes", "FNG_CYCLE_CC_LANE", &CycleThroughMidiLanes, 0, UNDO_STATE_ITEMS);
	RprCommand::registerCommand("SWS/FNG MIDI: cycle through CC lanes (keep lane heights constant)", "FNG_CYCLE_CC_LANE_KEEP_HEIGHT", &CycleThroughMidiLanes, 1, UNDO_STATE_ITEMS);
	RprCommand::registerCommand("SWS/FNG MIDI: show only used CC lanes", "FNG_SHOW_USED_CC_LANES", &ShowUsedCCLanes, UNDO_STATE_ITEMS);
	RprCommand::registerCommand("SWS/FNG MIDI: hide unused CC lanes", "FNG_HIDE_UNUSED_CC_LANES", &HideUnusedCCLanes, UNDO_STATE_ITEMS);
	RprCommand::registerCommand("SWS/FNG MIDI: show only top CC lane", "FNG_TOP_CC_LANE", &ShowOnlyTopCCLane, UNDO_STATE_ITEMS);
}

static const struct {
		RprMidiBase::MessageType messageType;
		int index;
	} 
	eventIndexTable[] = 
	{
		{RprMidiBase::PitchBend, 128 },
		{RprMidiBase::ProgramChange, 129 },
		{RprMidiBase::ChannelPressure, 130 },
		{RprMidiBase::ProgramChange, 131 },
		{RprMidiBase::Sysex, 133 },
		{RprMidiBase::TextEvent, 132 },
	};

static void CycleThroughMidiLanes(int flag, void *data)
{
	bool keepHeights = (*(int *)data) == 1;
	RprMidiCCLanePtr laneView = RprMidiCCLane::createFromMidiEditor();

	if(laneView->countShown() < 2)
		return;

	int tempLaneId, tempHeight;
	int tempLaneId2, tempHeight2;

	int i = laneView->countShown() - 1;
	tempLaneId = laneView->getIdAt(i);
	tempHeight = laneView->getHeightAt(i);
	
	for(; i > 0 ; --i) {
		
		tempLaneId2 = laneView->getIdAt(i - 1);
		tempHeight2 = laneView->getHeightAt(i - 1);

		laneView->setIdAt(i - 1, tempLaneId);
		if (!keepHeights)
			laneView->setHeightAt(i - 1, tempHeight);

		tempLaneId = tempLaneId2;
		tempHeight = tempHeight2;
	}

	laneView->setIdAt(laneView->countShown() - 1, tempLaneId);
	if (!keepHeights)
		laneView->setHeightAt(laneView->countShown() - 1, tempHeight);
}

static int getLaneHeight(RprMidiCCLanePtr &laneView, int id)
{
	if(laneView->isShown(id))
		return laneView->getHeight(id);

	return defaultHeight;
}

static bool hasEventsofType(const RprMidiTakePtr &midiTake, int cc)
{
	if( cc > 120) {
		for(int i = 0; i < __ARRAY_SIZE(eventIndexTable); ++i) {
			if( cc != eventIndexTable[i].index)
				continue;
			if(midiTake->hasEventType(eventIndexTable[i].messageType))
				return true;
			return false;
		}
		return false;
	}
	if(cc == -1) {
		if(midiTake->countNotes() > 0)
			return true;
		return false;
	}
	if(midiTake->countCCs(cc) > 0)
		return true;
	return false;
}

static void ShowUsedCCLanes(int flag, void *data)
{
	
	RprMidiCCLanePtr laneView = RprMidiCCLane::createFromMidiEditor();
	RprMidiTakePtr midiTake = RprMidiTake::createFromMidiEditor(true);

	/* remove ununsed lanes */
	for(int i = 0; i < laneView->countShown(); ++i) {
		int cc = laneView->getIdAt(i);
		if(!hasEventsofType(midiTake, cc)) {
			laneView->remove(i--);
		}
	}

	/* add new lanes at bottom for used ccs if not already shown */
	if(midiTake->countNotes() > 0 && !laneView->isShown(-1)) {
		laneView->append(-1, defaultHeight);
	}
	
	for(int i = 0; i < __ARRAY_SIZE(eventIndexTable); ++i) {
		if (midiTake->hasEventType(eventIndexTable[i].messageType) && !laneView->isShown(eventIndexTable[i].index)) {
			laneView->append(eventIndexTable[i].index, defaultHeight);
		}
	}

    /* Special case: Bank select (CC0) and lane 131 (Program change) */
    if (midiTake->countCCs(0) > 0 && !laneView->isShown(131)) {
        laneView->append(131, defaultHeight);
    }

	for(int i = 0; i < 120; ++i) {
		if (midiTake->countCCs(i) > 0 && !laneView->isShown(i)) {
			laneView->append(i, defaultHeight);
		}
	}
}

static void HideUnusedCCLanes(int flag, void *data)
{
	RprMidiCCLanePtr laneView = RprMidiCCLane::createFromMidiEditor();
	RprMidiTakePtr midiTake = RprMidiTake::createFromMidiEditor(true);
	std::list<int> ccIndices;
	for(int i = 0; i < laneView->countShown(); ++i) {
		int index = laneView->getIdAt(i);
        /* Special case for bank-select and program change events */
        if (index == 131 || index == 0) {
            if (midiTake->countCCs(0) == 0 && midiTake->hasEventType(RprMidiBase::ProgramChange)) {
                ccIndices.push_back(index);
            }
            continue;
        }

		for(int j = 0; j < __ARRAY_SIZE(eventIndexTable); ++j) {
			if(index == eventIndexTable[j].index) {
				if(!midiTake->hasEventType(eventIndexTable[j].messageType))
					ccIndices.push_back(index);
			}
		}

        if( index > 0 && index <= 119) {
			if(midiTake->countCCs(index) == 0)
				ccIndices.push_back(index);
		}
		if( index == -1 && midiTake->countNotes() == 0)
			ccIndices.push_back(index);
	}

	for(std::list<int>::const_iterator i = ccIndices.begin(); i != ccIndices.end(); ++i) {
		for(int j = 0; j < laneView->countShown(); ++j) {
			if (laneView->getIdAt(j) == *i) {
				laneView->remove(j);
				break;
			}
		}
	}
}

static void ShowOnlyTopCCLane(int flag, void *data)
{
	RprMidiCCLanePtr laneView = RprMidiCCLane::createFromMidiEditor();
	while(laneView->countShown() > 1) {
		laneView->remove(1);
	}
}

