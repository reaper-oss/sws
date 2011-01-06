#include "stdafx.h"

#include <ctime>

#include "FNG_client.h"
#include "reaper_helper.h"
#include "CommandHandler.h"
#include "EnvelopeCommands.h"
#include "MediaItemCommands.h"
#include "GrooveTemplates.hxx"
#include "GrooveCommands.hxx"
#include "MiscCommands.hxx"
#include "MidiLaneCommands.hxx"

#ifdef notfinished
#include "SFZExport.hxx"
#endif

int FNGExtensionInit(REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t *rec)
{
	std::srand((unsigned int)time(NULL));
	CReaperCommandHandler::Init(rec, hInstance);
	EnvelopeCommands::Init();
	MediaItemCommands::Init();
	MiscCommands::Init();
	GrooveTemplateHandler::Init();
	GrooveCommands::Init();
	MidiLaneCommands::Init();
#ifdef notfinished
	SFZExporter::Init();
#endif
	return 1;
}