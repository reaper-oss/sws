#include "stdafx.h"

#include "FNG_client.h"
#include "CommandHandler.h"
#include "EnvelopeCommands.h"
#include "MediaItemCommands.h"
#include "GrooveTemplates.h"
#include "GrooveCommands.h"
#include "MiscCommands.h"
#include "MidiLaneCommands.h"


int FNGExtensionInit()
{
    std::srand((unsigned int)time(NULL));
    EnvelopeCommands::Init();
    MediaItemCommands::Init();
    MiscCommands::Init();
    GrooveCommands::Init();
    GrooveTemplateHandler::Init();
    MidiLaneCommands::Init();

    return 1;
}

void FNGExtensionExit()
{
    GrooveTemplateHandler::Exit();
}

