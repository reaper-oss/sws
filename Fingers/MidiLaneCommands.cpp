#include "stdafx.h"

#include "MidiLaneCommands.h"
#include "CommandHandler.h"
#include "RprItem.h"
#include "RprTake.h"
#include "RprMidiCCLane.h"
#include "RprMidiEvent.h"
#include "RprMidiTake.h"
#include "../Breeder/BR_MidiUtil.h"

static const int defaultHeight = 67;

static void CycleThroughMidiLanes(int flag, void *data);
static void ShowUsedCCLanes(int flag, void *data);
static void HideUnusedCCLanes(int flag, void *data);
static void ShowOnlyTopCCLane(int flag, void *data);

static void CycleThroughMidiLanes(COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd);
static void ShowUsedCCLanes(COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd);
static void HideUnusedCCLanes(COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd);
static void ShowOnlyTopCCLane(COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd);

//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
void MidiLaneCommands::Init()
{
    RprCommand::registerCommand("SWS/FNG: Cycle through CC lanes in active MIDI editor", "FNG_CYCLE_CC_LANE", &CycleThroughMidiLanes, 0, UNDO_STATE_ITEMS);
    RprCommand::registerCommand("SWS/FNG: Cycle through CC lanes in active MIDI editor (keep lane heights constant)", "FNG_CYCLE_CC_LANE_KEEP_HEIGHT", &CycleThroughMidiLanes, 1, UNDO_STATE_ITEMS);
    RprCommand::registerCommand("SWS/FNG: Show only used CC lanes in active MIDI editor", "FNG_SHOW_USED_CC_LANES", &ShowUsedCCLanes, UNDO_STATE_ITEMS);
    RprCommand::registerCommand("SWS/FNG: Hide unused CC lanes in active MIDI editor", "FNG_HIDE_UNUSED_CC_LANES", &HideUnusedCCLanes, UNDO_STATE_ITEMS);
    RprCommand::registerCommand("SWS/FNG: Show only top CC lane in active MIDI editor", "FNG_TOP_CC_LANE", &ShowOnlyTopCCLane, UNDO_STATE_ITEMS);

	// BR: rather crude (not following how Fingers did it, but is simple, works and should be easily changeable in case Fingers decides to upgrade his RprCommand class)
	static COMMAND_T g_commandTable[] =
	{
		{ { DEFACCEL, "SWS/FNG: Cycle through CC lanes" }, "FNG_ME_CYCLE_CC_LANE",  NULL, NULL, 0, NULL, 32060, CycleThroughMidiLanes},
		{ { DEFACCEL, "SWS/FNG: Cycle through CC lanes (keep lane heights constant)" }, "FNG_ME_CYCLE_CC_LANE_KEEP_HEIGHT",  NULL, NULL, 1, NULL, 32060, CycleThroughMidiLanes},
		{ { DEFACCEL, "SWS/FNG: Show only used CC lanes" }, "FNG_ME_SHOW_USED_CC_LANES",  NULL, NULL, 0, NULL, 32060, ShowUsedCCLanes},
		{ { DEFACCEL, "SWS/FNG: Hide unused CC lanes" }, "FNG_ME_HIDE_UNUSED_CC_LANES",  NULL, NULL, 0, NULL, 32060, HideUnusedCCLanes},
		{ { DEFACCEL, "SWS/FNG: Show only top CC lane" }, "FNG_ME_TOP_CC_LANE",  NULL, NULL, 0, NULL, 32060, ShowOnlyTopCCLane},
		{ {}, LAST_COMMAND, },
	};
	SWSRegisterCommands(g_commandTable);

}
//!WANT_LOCALIZE_1ST_STRING_END

static const struct {
        RprMidiEvent::MessageType messageType;
        int index;
    }
    eventIndexTable[] =
    {
        {RprMidiEvent::PitchBend, 128 },
        {RprMidiEvent::ProgramChange, 129 },
        {RprMidiEvent::ChannelPressure, 130 },
        {RprMidiEvent::ProgramChange, 131 },
        {RprMidiEvent::Sysex, 133 },
        {RprMidiEvent::TextEvent, 132 },
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
	set<int> usedCC = GetUsedCCLanes(MIDIEditor_GetActive(), 0, false); // faster than RprMidiTake::createFromMidiEditor(true);

	 /* remove ununsed lanes */
	for (int i = 0; i < laneView->countShown(); ++i)
		if (usedCC.find(laneView->getIdAt(i)) == usedCC.end())
			laneView->remove(i--);

	 /* Special case: Bank select (CC0) and lane 131 (Program change) */
	if (usedCC.find(0) != usedCC.end() && usedCC.find(CC_BANK_SELECT) != usedCC.end() && !laneView->isShown(131))
		laneView->append(131, defaultHeight);

	/* add new lanes at bottom for used ccs if not already shown */
	for (set<int>::iterator it = usedCC.begin(); it != usedCC.end(); ++it)
	{
		if (!laneView->isShown(*it))
			laneView->append(*it, defaultHeight);
		else
		{
			for (int i = 0; i < laneView->countShown(); ++i)
			{
				if (laneView->getIdAt(i) == *it && laneView->getHeight(i) == 0)
					laneView->setHeightAt(i, defaultHeight);
			}
		}
	}

	/* BR: If all lanes are removed it won't really work - we need to leave one lane with height 0 */
	if (laneView->countShown() == 0)
		laneView->append(-1, 0);
}

static void HideUnusedCCLanes(int flag, void *data)
{
	RprMidiCCLanePtr laneView = RprMidiCCLane::createFromMidiEditor();
	set<int> usedCC = GetUsedCCLanes(MIDIEditor_GetActive(), 1, false);

	for(int i = 0; i < laneView->countShown(); ++i) {
		if (usedCC.find(laneView->getIdAt(i)) == usedCC.end())
			laneView->remove(i--);
	}

	/* BR: If all lanes are removed it won't really work - we need to leave one lane with height 0 */
	if (laneView->countShown() == 0)
		laneView->append(-1, 0);
}

static void ShowOnlyTopCCLane(int flag, void *data)
{
    RprMidiCCLanePtr laneView = RprMidiCCLane::createFromMidiEditor();
    while(laneView->countShown() > 1) {
        laneView->remove(1);
    }
}

static void CycleThroughMidiLanes(COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd)
{
	CycleThroughMidiLanes(UNDO_STATE_ITEMS, &ct->user);
	Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ITEMS, -1);
}

static void ShowUsedCCLanes(COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd)
{
	ShowUsedCCLanes(UNDO_STATE_ITEMS, &ct->user);
	Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ITEMS, -1);
}

static void HideUnusedCCLanes(COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd)
{
	HideUnusedCCLanes(UNDO_STATE_ITEMS, &ct->user);
	Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ITEMS, -1);
}

static void ShowOnlyTopCCLane(COMMAND_T* ct, int val, int valhw, int relmode, HWND hwnd)
{
	ShowOnlyTopCCLane(UNDO_STATE_ITEMS, &ct->user);
	Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ITEMS, -1);
}
