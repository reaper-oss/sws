#include "stdafx.h"

#include "CommandHandler.h"
#include "GrooveTemplates.hxx"
#include "GrooveCommands.hxx"
#include "GrooveDialog.hxx"
#include "FNG_Settings.h"
#include "../Xenakios/XenakiosExts.h"

static void ApplyGroove(int flags, void *data)
{
	int *beatDivider = (int *)data;
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	me->ApplyGroove(*beatDivider, 1.0, 1.0);
}

static void ApplyGrooveInMidiEditor(int flags, void *data)
{
	int *beatDivider = (int *)data;
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	me->ApplyGrooveToMidiEditor(*beatDivider, 1.0, 1.0);
}

static void StoreGroove(int flags, void *data)
{
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	me->StoreGroove();
}

static void StoreGrooveFromMIDIEditor(int flags, void *data)
{
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	me->StoreGrooveFromMidiEditor();
}

static void SaveGrooveToFile(int flags, void *data)
{
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	if(me->isGrooveEmpty())
	{
		MessageBox(GetMainHwnd(), "No groove stored", "Error",0);
		return;
	}
	
	char cFilename[256];
	if (BrowseForSaveFile("Select groove template", me->GetGrooveDir().c_str(), NULL,
						  "Reaper Groove Templates (*.rgt)\0*.rgt\0All Files (*.*)\0*.*\0", cFilename, 256))
	{
		std::string errMessage;
		std::string fName = cFilename;
		if(!me->SaveGroove(fName, errMessage))
			MessageBox(GetMainHwnd(), errMessage.c_str(), "Error", 0);
	}
}

static void LoadGrooveFromFile(int flags, void *data)
{
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	char* cFilename = BrowseForFiles("Select groove template", me->GetGrooveDir().c_str(), NULL, false,
									 "Reaper Groove Templates (*.rgt)\0*.rgt\0All Files (*.*)\0*.*\0");
	if (cFilename)
	{
		std::string errMessage;
		std::string fName = cFilename;
		if(!me->LoadGroove(fName, errMessage))
			MessageBox(GetMainHwnd(), errMessage.c_str(), "Error", 0);
		free(cFilename);
	}
}

static void ShowGroove(int flags, void *data)
{
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	if(me->isGrooveEmpty())
	{
		MessageBox(GetMainHwnd(), "No groove stored", "Error",0);
		return;
	}
	MessageBox(GetMainHwnd(),me->GrooveToString().c_str(), "Groove",0);
}

static void MarkGroove(int flags, void *data)
{
	int *multiple = (int *)data;
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	me->MarkGroove(*multiple);
}

static void MarkGrooveStart(int flags, void *data)
{
	int *start = (int *)data;
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	me->SetMarkerStart((GrooveTemplateHandler::GrooveMarkerStart)*start);
}

void ShowGrooveDialog(int flags, void *data)
{
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	me->showGrooveDialog();	
}

void GrooveCommands::Init()
{
	static CReaperCmdReg CommandTable[] =
	{
	 CReaperCmdReg(
		"SWS/FNG: Apply groove to selected media items (within 16th)", "FNG_APPLY_GROOVE",
		(CReaperCommand *)new CReaperCommand(&ApplyGroove, 16),
		UNDO_STATE_ITEMS
		),

	  CReaperCmdReg(
		"SWS/FNG: Apply groove to selected media items (within 32nd)", "FNG_APPLY_GROOVE_32",
		(CReaperCommand *)new CReaperCommand(&ApplyGroove, 32),
		UNDO_STATE_ITEMS
		),

	  CReaperCmdReg(
		"SWS/FNG MIDI: Apply groove to selected MIDI notes in active MIDI editor (within 16th)", "FNG_APPLY_MIDI_GROOVE_16",
		(CReaperCommand *)new CReaperCommand(&ApplyGrooveInMidiEditor, 16),
		UNDO_STATE_ITEMS
		),

	  CReaperCmdReg(
		"SWS/FNG MIDI: Apply groove to selected MIDI notes in active MIDI editor (within 32nd)", "FNG_APPLY_MIDI_GROOVE_32",
		(CReaperCommand *)new CReaperCommand(&ApplyGrooveInMidiEditor, 32),
		UNDO_STATE_ITEMS
		),

	  CReaperCmdReg(
		"SWS/FNG: Get groove from selected media items", "FNG_GET_GROOVE",
		(CReaperCommand *)new CReaperCommand(&StoreGroove),
		NO_UNDO
		),
	  CReaperCmdReg(
		"SWS/FNG: Get groove from selected MIDI notes in active MIDI editor", "FNG_GET_GROOVE_MIDI",
		(CReaperCommand *)new CReaperCommand(&StoreGrooveFromMIDIEditor),
		NO_UNDO
		),

	  CReaperCmdReg(
		"SWS/FNG: Save groove template to file", "FNG_SAVE_GROOVE",
		(CReaperCommand *)new CReaperCommand(&SaveGrooveToFile),
		NO_UNDO
		),
		
	  CReaperCmdReg(
		"SWS/FNG: Load groove template from file", "FNG_LOAD_GROOVE",
		(CReaperCommand *)new CReaperCommand(&LoadGrooveFromFile),
		NO_UNDO
		),

	  CReaperCmdReg(
		"SWS/FNG: Show current groove template", "FNG_SHOW_GROOVE",
		(CReaperCommand *)new CReaperCommand(&ShowGroove),
		NO_UNDO
		),

	  CReaperCmdReg(
		"SWS/FNG: Toggle groove markers", "FNG_GROOVE_MARKERS", 
		(CReaperCommand *)new CReaperCommand(&MarkGroove, 1),
		NO_UNDO
		),

	  CReaperCmdReg(
		"SWS/FNG: Toggle groove markers 2x", "FNG_GROOVE_MARKERS_2", 
		(CReaperCommand *)new CReaperCommand(&MarkGroove, 2),
		NO_UNDO
		),

	  CReaperCmdReg(
		"SWS/FNG: Toggle groove markers 4x", "FNG_GROOVE_MARKERS_4",
		(CReaperCommand *)new CReaperCommand(&MarkGroove, 4),
		NO_UNDO
		),
	  
	  CReaperCmdReg(
		"SWS/FNG: Toggle groove markers 8x", "FNG_GROOVE_MARKERS_8",
		(CReaperCommand *)new CReaperCommand(&MarkGroove, 8),
		NO_UNDO
		),

	  CReaperCmdReg(
		"SWS/FNG: Set groove marker start to edit cursor", "FNG_GROOVE_MARKER_START_CUR",
		(CReaperCommand *)new CReaperCommand(&MarkGrooveStart, (int)GrooveTemplateHandler::EDITCURSOR),
		NO_UNDO
		),

	  CReaperCmdReg(
		"SWS/FNG: Set groove marker start to current bar", "FNG_GROOVE_MARKER_START_BAR",
		(CReaperCommand *)new CReaperCommand(&MarkGrooveStart, (int)GrooveTemplateHandler::CURRENTBAR),
		NO_UNDO
		),

	  CReaperCmdReg(
		"SWS/FNG: Show groove tool...", "FNG_GROOVE_TOOL",
		(CReaperCommand *)new CReaperCommand(&ShowGrooveDialog),
		NO_UNDO
		),
	};
	
	CReaperCommandHandler *cmdHandler = CReaperCommandHandler::Instance();
	cmdHandler->AddCommands(CommandTable, __ARRAY_SIZE(CommandTable));
}