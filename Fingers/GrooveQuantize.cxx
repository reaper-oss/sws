#include "stdafx.h"

#include "GrooveQuantize.hxx"
#include "GrooveTemplates.hxx"

GrooveQuantize::GrooveQuantize()
{
}

void GrooveQuantize::extractGrooveFromSelected()
{
	
	GrooveTemplateHandler::StoreGroove();
}

void GrooveQuantize::extractGrooveFromMidiEditor()
{
	GrooveTemplateHandler::StoreGrooveFromMidiEditor();
}

void GrooveQuantize::applyGroove(int beatDivider, double strength)
{
	GrooveTemplateHandler::ApplyGroove(beatDivider, strength);
}

void GrooveQuantize::applyGrooveToMidiEditor(int beatDivider, double strength)
{
	GrooveTemplateHandler::ApplyGrooveToMidiEditor(beatDivider, strength);
}

std::string GrooveQuantize::getGrooveDir()
{
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	return me->GetGrooveDir2();
}

bool GrooveQuantize::userGrooveEmpty()
{
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	return me->isGrooveEmpty();
}

bool GrooveQuantize::saveGroove(std::string &fileName, std::string &errorMessage)
{
	return GrooveTemplateHandler::SaveGroove2(fileName, errorMessage);
}

bool GrooveQuantize::loadGroove(std::string &fileName, std::string &errorMessage)
{
	return GrooveTemplateHandler::LoadGroove2(fileName, errorMessage);
}

void GrooveQuantize::getGrooveString(std::string &grooveString)
{
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	grooveString = me->GrooveToString();
}

void GrooveQuantize::markGroove(int nRepeat, GrooveMarkerStart markerStart)
{
	GrooveTemplateHandler::SetMarkerStart((GrooveTemplateHandler::GrooveMarkerStart)markerStart);
	GrooveTemplateHandler::MarkGroove(nRepeat);
}

bool GrooveQuantize::grooveIsMarked()
{
	return GrooveTemplateHandler::GrooveIsMarked();
}