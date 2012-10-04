#include "stdafx.h"
#include "../reaper/localize.h"

#include "CommandHandler.h"
#include "GrooveTemplates.hxx"
#include "GrooveCommands.h"
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

static void GetGrooveFromItems(int flags, void *data)
{
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	me->GetGrooveFromItems();
}

static void GetGrooveFromMIDIEditor(int flags, void *data)
{
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	me->GetGrooveFromMidiEditor();
}

static void SaveGrooveToFile(int flags, void *data)
{
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	if(me->isGrooveEmpty())
	{
		MessageBox(GetMainHwnd(), __LOCALIZE("No groove loaded!","sws_mbox"), __LOCALIZE("FNG - Error","sws_mbox"), 0);
		return;
	}
	
	char cFilename[256];
	if (BrowseForSaveFile(__LOCALIZE("Select groove template","sws_mbox"), me->GetGrooveDir().c_str(), NULL,
						  "Reaper Groove Templates (*.rgt)\0*.rgt\0All Files (*.*)\0*.*\0", cFilename, 256))
	{
		std::string errMessage;
		std::string fName = cFilename;
		if(!me->SaveGroove(fName, errMessage))
			MessageBox(GetMainHwnd(), errMessage.c_str(), __LOCALIZE("FNG - Error","sws_mbox"), 0);
		else
			me->GetGrooveDialog()->Refresh();
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
			MessageBox(GetMainHwnd(), errMessage.c_str(), __LOCALIZE("FNG - Error","sws_mbox"), 0);
		free(cFilename);
	}
}

static void ShowGroove(int flags, void *data)
{
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	if(me->isGrooveEmpty())
	{
		MessageBox(GetMainHwnd(), __LOCALIZE("No groove loaded!","sws_mbox"), __LOCALIZE("FNG - Error","sws_mbox"),0);
		return;
	}
	MessageBox(GetMainHwnd(), me->GrooveToString().c_str(), __LOCALIZE("Groove","sws_mbox"), 0);
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

bool IsGrooveDialogOpen()
{
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	return me->GetGrooveDialog()->IsValidWindow();
}

//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
void GrooveCommands::Init()
{
	RprCommand::registerCommand("SWS/FNG: Apply groove to selected media items (within 16th)", "FNG_APPLY_GROOVE",&ApplyGroove, 16, UNDO_STATE_ITEMS);
	RprCommand::registerCommand("SWS/FNG: Apply groove to selected media items (within 32nd)", "FNG_APPLY_GROOVE_32", &ApplyGroove, 32, UNDO_STATE_ITEMS);
	RprCommand::registerCommand("SWS/FNG MIDI: Apply groove to selected MIDI notes in active MIDI editor (within 16th)", "FNG_APPLY_MIDI_GROOVE_16", &ApplyGrooveInMidiEditor, 16, UNDO_STATE_ITEMS);
	RprCommand::registerCommand("SWS/FNG MIDI: Apply groove to selected MIDI notes in active MIDI editor (within 32nd)", "FNG_APPLY_MIDI_GROOVE_32", &ApplyGrooveInMidiEditor, 32, UNDO_STATE_ITEMS);
	RprCommand::registerCommand("SWS/FNG: Get groove from selected media items", "FNG_GET_GROOVE", &GetGrooveFromItems, NO_UNDO);
	RprCommand::registerCommand("SWS/FNG: Get groove from selected MIDI notes in active MIDI editor", "FNG_GET_GROOVE_MIDI", &GetGrooveFromMIDIEditor, NO_UNDO);
	RprCommand::registerCommand("SWS/FNG: Save groove template to file", "FNG_SAVE_GROOVE", &SaveGrooveToFile, NO_UNDO);
	RprCommand::registerCommand("SWS/FNG: Load groove template from file", "FNG_LOAD_GROOVE",&LoadGrooveFromFile, NO_UNDO);
	RprCommand::registerCommand("SWS/FNG: Show current groove template", "FNG_SHOW_GROOVE", &ShowGroove, NO_UNDO);
	RprCommand::registerCommand("SWS/FNG: Toggle groove markers", "FNG_GROOVE_MARKERS", &MarkGroove, 1, NO_UNDO);
	RprCommand::registerCommand("SWS/FNG: Toggle groove markers 2x", "FNG_GROOVE_MARKERS_2", &MarkGroove, 2, NO_UNDO);
	RprCommand::registerCommand("SWS/FNG: Toggle groove markers 4x", "FNG_GROOVE_MARKERS_4", &MarkGroove, 4, NO_UNDO);
	RprCommand::registerCommand("SWS/FNG: Toggle groove markers 8x", "FNG_GROOVE_MARKERS_8", &MarkGroove, 8, NO_UNDO);
	RprCommand::registerCommand("SWS/FNG: Set groove marker start to edit cursor", "FNG_GROOVE_MARKER_START_CUR", &MarkGrooveStart, (int)GrooveTemplateHandler::EDITCURSOR,	NO_UNDO);
	RprCommand::registerCommand("SWS/FNG: Set groove marker start to current bar", "FNG_GROOVE_MARKER_START_BAR", &MarkGrooveStart, (int)GrooveTemplateHandler::CURRENTBAR,	NO_UNDO);
	RprCommand::registerToggleCommand("SWS/FNG: Show groove tool", "FNG_GROOVE_TOOL",&ShowGrooveDialog, IsGrooveDialogOpen, NO_UNDO);
}
//!WANT_LOCALIZE_1ST_STRING_END
