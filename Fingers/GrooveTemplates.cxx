#include "stdafx.h"

#include "GrooveTemplates.hxx"

#include "CommandHandler.h"
#include "FNG_Settings.h"
#include "FileUtil.hxx"
#include "TimeMap.h"

#include "RprItem.hxx"
#include "RprTake.hxx"
#include "RprMidiTake.hxx"

template<class T>
struct subtract : public std::unary_function <T, T>
{
	subtract(T rhs) : _rhs(rhs) {}
	T operator() (T lhs) { return lhs - _rhs; }
	T _rhs;
};

GrooveTemplateHandler *GrooveTemplateHandler::instance = NULL;

GrooveTemplateHandler *GrooveTemplateHandler::Instance()
{
	if(instance == NULL)
		instance = new GrooveTemplateHandler();
	return instance;
}

GrooveTemplateHandler::GrooveTemplateHandler()
{}

void GrooveTemplateHandler::ClearGroove()
{ 
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	me->grooveInBeats.clear();
}

static bool GetGrooveBeatPosition(double currentBeatPosition, double maxBeatDistance, double strength, std::vector<GrooveItem> *grooveInBeats, GrooveItem &newGroove)
{
	/* get max distance position */
	double minDistance = maxBeatDistance;
	bool positive = true;
	for(std::vector<GrooveItem>::iterator it = grooveInBeats->begin(); it != grooveInBeats->end(); it++)
	{
		double distance = currentBeatPosition - it->position;
		if( abs(distance) < minDistance) {
			positive = distance > 0 ? true : false;
			minDistance = abs(distance);
			newGroove = *it;
		}
	}
	if(minDistance >= maxBeatDistance) {
		return false;
	}

	double distance = minDistance * (positive ? 1.0 : -1.0);
	newGroove.position = currentBeatPosition - distance * strength;
	newGroove.amplitude *= strength;
	return true;
}

void GrooveTemplateHandler::Init()
{
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();

	me->grooveMarkersHelper.BeginLoadProjectState = GrooveTemplateHandler::BeginLoadProjectState;
	me->grooveMarkersHelper.ProcessExtensionLine = GrooveTemplateHandler::ProcessExtensionLine;
	me->grooveMarkersHelper.SaveExtensionConfig = GrooveTemplateHandler::SaveGrooveMarkers;
	CReaperCommandHandler::AddProjectConfig(&me->grooveMarkersHelper);
	CReaperCommandHandler *rprHandler = CReaperCommandHandler::Instance();
	me->mGrooveDialog = new GrooveDialog();
}

void GrooveTemplateHandler::showGrooveDialog()
{
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	me->mGrooveDialog->Show(true, true);
}

std::string GrooveTemplateHandler::GetGrooveDir()
{
	return getReaperProperty("groove_dir");
}

int GrooveTemplateHandler::GetGrooveStrength()
{
	std::string strength =  getReaperProperty("groove_strength");
	if (strength.empty())
		return 100;
	return ::atoi(strength.c_str());
}

void GrooveTemplateHandler::SetGrooveStrength(int val)
{
	setReaperProperty("groove_strength", val);
}

int GrooveTemplateHandler::GetGrooveTarget()
{
	std::string target = getReaperProperty("groove_target");
	if(target.empty())
		return 0;
	return ::atoi(target.c_str());
}

void GrooveTemplateHandler::SetGrooveTarget(int val)
{
	setReaperProperty("groove_target", val);
}


int GrooveTemplateHandler::GetGrooveTolerance()
{
	std::string tolerance =  getReaperProperty("groove_tolerance");
	if (tolerance.empty())
		return 16;
	return ::atoi(tolerance.c_str());
}

void GrooveTemplateHandler::SetGrooveDir(std::string &dir)
{
	setReaperProperty<>("groove_dir", dir.c_str());
}

void GrooveTemplateHandler::SetGrooveTolerance(int tol)
{
	setReaperProperty("groove_tolerance", tol);
}

bool GrooveTemplateHandler::isGrooveEmpty()
{
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	return me->grooveInBeats.empty();
}

static bool convertToInProjectMidi(RprItemCtrPtr &ctr)
{
	bool hasMidiFile = false;
	for(int i = 0; i < ctr->size(); i++) {
		RprTake take(ctr->getAt(i).getActiveTake());
		if(!take.isMIDI())
			continue;
		if(take.isFile()) {
			hasMidiFile = true;
			break;
		}
	}
	if(hasMidiFile) {
		if(MessageBox(GetMainHwnd(),
			"Current selection has takes with midi files.\r\nTo apply this action these takes to be converted to in-project takes.\r\nDo you want to continue?",
			"Warning", MB_YESNO) == IDNO) {
			return false;
		}
		Main_OnCommandEx(40684, 0 , 0);
	}
	return true;
}

static void applyGrooveToMidiTake(RprMidiTake &midiTake, double beatDivider, double strength, std::vector<GrooveItem> &grooveBeats, bool selectedOnly)
{
	RprItem rprItem = *midiTake.getParent();
	for(int i = 0; i < midiTake.countNotes(); i++) {
		RprMidiNote *note = midiTake.getNoteAt(i);
		if(selectedOnly && !note->isSelected())
			continue;
		double noteBeat = TimeToBeat(note->getPosition());
		GrooveItem grooveItem;
		if(!GetGrooveBeatPosition(noteBeat, BeatsInMeasure(BeatToMeasure(noteBeat)) / beatDivider, strength, &grooveBeats, grooveItem))
			continue;

		double itemFirstBeat = TimeToBeat(rprItem.getPosition());
		double itemLastBeat = TimeToBeat(rprItem.getPosition() + rprItem.getLength());
		if(grooveItem.position >= itemFirstBeat && grooveItem.position < itemLastBeat) {
			note->setPosition(BeatToTime(grooveItem.position));
			if(grooveItem.amplitude >= 0.0)
				note->setVelocity((unsigned int)(grooveItem.amplitude * 127.5));
		}
	}
}

static double getRightEdgeOfContainer(RprItemCtrPtr &ctr)
{
	double rightEdge = 0.0;
	for(int i = 0; i < ctr->size(); i++) {
		double testRightEdge = ctr->getAt(i).getPosition() + ctr->getAt(i).getLength();
		if(testRightEdge > rightEdge)
			rightEdge = testRightEdge;
	}
	return rightEdge;
}

static double getRightEdgeOfMidiTake(RprMidiTakePtr &take)
{
	double rightEdge = 0.0;
	for(int i = 0; i < take->countNotes(); i++) {
		double testRightEdge = take->getNoteAt(i)->getPosition() + take->getNoteAt(i)->getLength();
		if(testRightEdge > rightEdge)
			rightEdge = testRightEdge;
	}
	return rightEdge;
}

static void createGrooveVector(double leftEdge, double rightEdge, std::vector<GrooveItem> &inputGrooveBeats, int nBeatsInGroove, std::vector<GrooveItem> &outputGrooveBeats)
{
	/* create vector of positions which is longer then the total length of the items */
	int beatCount = (int)ceil(TimeToBeat(rightEdge) - TimeToBeat(leftEdge));
	int firstMeasure = TimeToMeasure(leftEdge);
	double beatsTillFirstMeasure = BeatsTillMeasure(firstMeasure);
	
	for(int i = -nBeatsInGroove; i < beatCount + nBeatsInGroove; i += nBeatsInGroove) {
		for(std::vector<GrooveItem>::iterator j = inputGrooveBeats.begin(); j != inputGrooveBeats.end(); j++) {
			double grooveBeatPosition = j->position + i + beatsTillFirstMeasure;
			if(grooveBeatPosition >= 0.0f) {
				GrooveItem grooveItem = *j;
				grooveItem.position = grooveBeatPosition;
				outputGrooveBeats.push_back(grooveItem);
			}
		}
	}
}

bool treatAsMidiTake(RprMidiTake &midiTake)
{
	if(!(midiTake.countNotes() == 1 && midiTake.getNoteAt(0)->getPosition() == 0.0))
		return true;
	return false;
}

void applyGrooveToItem(RprItem &rprItem, double beatDivider, double strength, std::vector<GrooveItem> &grooveBeats)
{
	double beatPosition = TimeToBeat(rprItem.getPosition() + rprItem.getSnapOffset());
	GrooveItem grooveItem;
	if(!GetGrooveBeatPosition(beatPosition, BeatsInMeasure(BeatToMeasure(beatPosition)) / beatDivider, strength, &grooveBeats, grooveItem))
		return;

	double timePosition = BeatToTime(grooveItem.position) - rprItem.getSnapOffset();
	/* Change amplitude for items?? Maybe in the future...*/
	/* How does velocity map to item volumes and vice versa... */
	if(timePosition >= 0.0f)
		rprItem.setPosition(timePosition);
}

void GrooveTemplateHandler::ApplyGrooveToMidiEditor(int beatDivider, double strength)
{
	RprMidiTakePtr takePtr = RprMidiTake::createFromMidiEditor(false);
	if(takePtr->countNotes() == 0)
		return;

	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	if(me->grooveInBeats.size() == 0)
		return;

	std::vector<GrooveItem> grooveBeats;
	createGrooveVector(takePtr->getNoteAt(0)->getPosition(),
					   getRightEdgeOfMidiTake(takePtr),
					   me->grooveInBeats,
					   me->nBeatsInGroove,
					   grooveBeats);
	applyGrooveToMidiTake(*takePtr.get(), (double)beatDivider, strength, grooveBeats, true);
}


void GrooveTemplateHandler::ApplyGroove(int beatDivider, double strength)
{
	RprItemCtrPtr ctr = RprItemCollec::getSelected();

	if(ctr->size() == 0)
		return;

	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();

	if(me->grooveInBeats.size() == 0)
		return;

	/* warn if we have ghost copyable midi items selected as it stuffs up the ghost copy */
	if(!convertToInProjectMidi(ctr))
		return;

	ctr->sort();
	std::vector<GrooveItem> grooveBeats;
	
	createGrooveVector(ctr->first().getPosition() + ctr->first().getSnapOffset(),
					   getRightEdgeOfContainer(ctr),
					   me->grooveInBeats,
					   me->nBeatsInGroove,
					   grooveBeats);
	
	/* apply groove to midi notes and media items */
	for(int i = 0; i < ctr->size(); i++) {
		RprItem rprItem = ctr->getAt(i);
		if(!rprItem.getActiveTake().isMIDI()) {
			applyGrooveToItem(rprItem, (double)beatDivider, strength, grooveBeats);
			continue;
		}

		RprMidiTake midiTake(rprItem.getActiveTake());
		if(treatAsMidiTake(midiTake))
			applyGrooveToMidiTake(midiTake, (double)beatDivider, strength, grooveBeats, false);
		else
			applyGrooveToItem(rprItem, (double)beatDivider, strength, grooveBeats);
			
	}
	UpdateTimeline();
}

std::string GrooveTemplateHandler::GetGrooveString(int index)
{
	std::ostringstream oss;
	oss.unsetf(std::ios::floatfield);
	oss.precision(10);
	std::vector<GrooveItem>::iterator it = grooveInBeats.begin() + index;
	if(it != grooveInBeats.end())
		oss << it->position;
	return oss.str();
}

std::string GrooveTemplateHandler::GetGrooveMarkerString(int index)
{
	std::vector<GrooveMarker>::iterator i;
	i = grooveMarkers.begin() + index;

	if( i != grooveMarkers.end()) {
		std::ostringstream oss;
		oss.unsetf(std::ios::floatfield);
		oss.precision(10);
		oss << (*i).name << " "
			<< (*i).index << " "
			<< (*i).pos;
		return oss.str();
	}
	return "";
}

static int GetMidiBeatPositions(RprMidiTake &midiTake, const RprItem &parent, std::vector<GrooveItem> &vPositions, bool selectedOnly)
{
	double endLen = parent.getPosition() + parent.getLength();
	for(int i = 0; i < midiTake.countNotes(); i++) {
		RprMidiNote *note = midiTake.getNoteAt(i);
		if(selectedOnly && !note->isSelected())
			continue;
		double notePosition = note->getPosition();
		double noteVelocityNorm = (double)note->getVelocity() / 127.0;
		if (notePosition < endLen) {
			GrooveItem grooveItem;
			grooveItem.amplitude = noteVelocityNorm;
			grooveItem.position = TimeToBeat(notePosition);
			vPositions.push_back(grooveItem);
		}
	}
	return (int)vPositions.size();
}

static bool sortGrooveItems(const GrooveItem &lhs, const GrooveItem &rhs)
{
	return lhs.position < rhs.position;
}

static bool isGrooveItemUnique(const GrooveItem &lhs, const GrooveItem &rhs)
{
	return lhs.position == rhs.position;
}

static void finalizeGroove(int &beatsInGroove, std::vector<GrooveItem> &grooveInBeats)
{
	std::sort(grooveInBeats.begin(), grooveInBeats.end(), sortGrooveItems);

	/* subtract number of beats up till first measure and work out number of beats in groove, and remove redundant beats */
	std::vector<GrooveItem>::const_iterator i = grooveInBeats.begin();
	double beatsTillStartOfGrooveMeasure = BeatsTillMeasure(BeatToMeasure(i->position));

	i = grooveInBeats.end() - 1;
	double beatsTillOneAfterEndOfGrooveMeasure = BeatsTillMeasure(BeatToMeasure(i->position) + 1);

	double dBeatsInGroove = beatsTillOneAfterEndOfGrooveMeasure - beatsTillStartOfGrooveMeasure;

	beatsInGroove = (int)std::floor(dBeatsInGroove + 0.5);

	for(std::vector<GrooveItem>::iterator j = grooveInBeats.begin(); j != grooveInBeats.end(); ++j) {
		j->position -= beatsTillStartOfGrooveMeasure;
	}

	grooveInBeats = std::vector<GrooveItem>( grooveInBeats.begin(), std::unique(grooveInBeats.begin(), grooveInBeats.end(), isGrooveItemUnique));

}

void GrooveTemplateHandler::StoreGrooveFromMidiEditor()
{
	RprMidiTakePtr takePtr = RprMidiTake::createFromMidiEditor(true);
	if(takePtr->countNotes() == 0)
		return;
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	GrooveTemplateHandler::ClearGroove();

	GetMidiBeatPositions(*takePtr.get(), *takePtr->getParent(), me->grooveInBeats, true);
	finalizeGroove(me->nBeatsInGroove, me->grooveInBeats);
}

GrooveItem createGrooveItemFromItem(const RprItem &rprItem)
{
	GrooveItem grooveItem;
	grooveItem.amplitude = 1.0;
	grooveItem.position = TimeToBeat(rprItem.getPosition() + rprItem.getSnapOffset());
	return grooveItem;
}

void GrooveTemplateHandler::StoreGroove()
{
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	RprItemCtrPtr ctr = RprItemCollec::getSelected();

	if(ctr->size() == 0)
	{
		MessageBox(GetMainHwnd(), "No items selected", "Error", 0);
		return;
	}
	GrooveTemplateHandler::ClearGroove();

	for(int i = 0; i < ctr->size(); i++) {
		RprItem rprItem = ctr->getAt(i);
		if (rprItem.getActiveTake().isMIDI()) {
			RprMidiTake midiTake(rprItem.getActiveTake(),true);
			/* add item position if no notes in midi item */
			if(GetMidiBeatPositions(midiTake, rprItem, me->grooveInBeats, false) == 0) {
				me->grooveInBeats.push_back(createGrooveItemFromItem(rprItem));
			}
		}
		else {
			me->grooveInBeats.push_back(createGrooveItemFromItem(rprItem));
		}
	}
	finalizeGroove(me->nBeatsInGroove, me->grooveInBeats);
	
}

bool GrooveTemplateHandler::LoadGroove(std::string &fileName, std::string &errorMessage)
{
	std::vector<GrooveItem> newGroove;
	int beatsInGroove;

	std::ifstream f;
	f.exceptions ( std::ifstream::failbit | std::ifstream::badbit );
	try
	{
		f.open(fileName.c_str());
		if(!f.is_open())
		{
			errorMessage = "Unable to open file";
			return false;
		}
		int nGrooveVersion = 0;
		std::string szLine;
		std::getline(f, szLine);
		if(sscanf(szLine.c_str(), "Version: %d", &nGrooveVersion) <= 0)
		{
			errorMessage = "Error loading groove from file";
			return false;
		}

		if(nGrooveVersion == 0 || nGrooveVersion == 1)
		{
			std::getline(f, szLine);
			if(sscanf(szLine.c_str(), "Number of beats in groove: %d", &beatsInGroove)<= 0)
			{
				errorMessage = "Error loading groove from file";
				return false;
			}
			std::getline(f, szLine); /* Groove: x positions */
			int nPosCount = 0;
			if(sscanf(szLine.c_str(), "Groove: %d positions", &nPosCount) <= 0)
			{
				errorMessage = "Error loading groove from file";
				return false;
			}
			int i = 0;
			while(!f.eof() && i++ < nPosCount)
			{
				GrooveItem grooveItem;
				f >> grooveItem.position;
				
				if(nGrooveVersion == 0) {
					grooveItem.amplitude = -1.0;
				} else {
					f >> grooveItem.amplitude;
				}
				newGroove.push_back(grooveItem);
			}
		}
		else
		{
			errorMessage = "Error loading groove from file";
			return false;
		}
	}
	catch (std::ifstream::failure e)
	{
		errorMessage = "Error reading file";
		return false;
	}
	
	/* update if everything went okay */
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	me->grooveInBeats = newGroove;
	me->nBeatsInGroove = beatsInGroove;
	
	f.close();
	return true;
}

bool GrooveTemplateHandler::SaveGroove(std::string &fileName, std::string &errorMessage)
{
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	if(me->grooveInBeats.size() == 0)
	{
		errorMessage = "No groove stored";
		return false;
	}

	std::ofstream f;
	f.open(fileName.c_str());
	if(!f.is_open())
	{
		errorMessage = "Unable to open file";
		return false;
	}

	std::string szGroove = me->GrooveToString();
	f << szGroove;
	
	f.close();
	return true;
}

void GrooveTemplateHandler::MarkGroove(int multiple)
{
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	
	if(!me->grooveMarkers.empty())
	{
		for(std::vector<GrooveMarker>::iterator it = me->grooveMarkers.begin(); it != me->grooveMarkers.end(); it++)
			DeleteProjectMarker(0, (*it).index, false);
		me->grooveMarkers.clear();
		return;
	}
	if(me->grooveInBeats.size() == 0)
	{
		MessageBox(GetMainHwnd(), "No groove stored", "Error",0);
		return;
	}
	
	double dOffset = 0.0;
	if(me->grooveMarkerStart == CURRENTBAR)
	{
		double pos = GetCursorPosition();
		dOffset = MeasureToTime(TimeToMeasure(pos));
	}
	else /* current position */
	{
		double pos = GetCursorPosition();
		dOffset = TimeToBeat(pos);
		/* remove position of first beat so the first beat starts at the edit cursor */
		dOffset -= me->grooveInBeats.begin()->position;
	}

	int num = 0;
	for(int i = 0; i < multiple; i++)
	{
		for(std::vector<GrooveItem>::iterator it = me->grooveInBeats.begin(); it != me->grooveInBeats.end(); it++)
		{
			std::stringstream oss;
			double beat = dOffset + it->position;
			double pos = BeatToTime(beat);
			oss << "GRV_" << num;
			GrooveMarker mark;
			mark.index = num + 100;
			mark.name = oss.str();
			mark.pos = pos;
			mark.index = AddProjectMarker(0, false, mark.pos, 0.0, mark.name.c_str(), mark.index);
			me->grooveMarkers.push_back(mark);
			num++;
		}
		dOffset += me->nBeatsInGroove;
	}
}

void GrooveTemplateHandler::SetMarkerStart(GrooveMarkerStart markerStart)
{
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	me->grooveMarkerStart = markerStart;
}

std::string GrooveTemplateHandler::GrooveToString()
{
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	std::stringstream oss;
	/* Groove version is 1. Just in case I want to change it but
	 * still allow old groove styles. Old version is 0 which didn't have 
	 * velocity */
	oss << "Version: " << 1 << "\n";
	oss << "Number of beats in groove: " << me->nBeatsInGroove << "\n";
	oss << "Groove: " << me->grooveInBeats.size() << " positions\n";
	std::vector<GrooveItem>::iterator it;
	for(it = me->grooveInBeats.begin(); it != me->grooveInBeats.end(); it++)
		oss << it->position << " " << it->amplitude << "\n";
	return oss.str();
}

void GrooveTemplateHandler::BeginLoadProjectState(bool isUndo, struct project_config_extension_t *reg)
{

}

bool GrooveTemplateHandler::ProcessExtensionLine(const char *line, ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	if(!isUndo) {
		if (strcmp(line, "<FNGGROOVE") == 0) {
			GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
			char markerBuf[256];
			int status = ctx->GetLine(markerBuf,256);
			while(markerBuf[0] != '>' && status == 0) {
				
				if (strcmp(markerBuf, "<GROOVEMARKERS") == 0) {
					status = ctx->GetLine(markerBuf,256);
					while(markerBuf[0] != '>' && status == 0) {
						char name[256];
						int index;
						double pos;
						sscanf(markerBuf, "%s %d %lf", name, &index, &pos);
						me->AddGrooveMarker(index, pos, name);
						status = ctx->GetLine(markerBuf,256);
					}
				}
				status = ctx->GetLine(markerBuf,256);
			}
			return true;
		}
	}
	return false;
}

void GrooveTemplateHandler::AddGrooveMarker(int index, double pos, char *name)
{
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	GrooveMarker mark;
	mark.index = index;
	mark.pos = pos;
	mark.name = name;
	me->grooveMarkers.push_back(mark);
}

void GrooveTemplateHandler::SaveGrooveMarkers(ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	if(!isUndo) {
		GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
		int i = 0;
		std::string markerdata = me->GetGrooveMarkerString(i++);
		if(markerdata.empty())
			return;

		ctx->AddLine("<FNGGROOVE");
		ctx->AddLine("<GROOVEMARKERS");
		while(!markerdata.empty()) {
			ctx->AddLine(markerdata.c_str());
			markerdata = me->GetGrooveMarkerString(i++);
		}
		ctx->AddLine(">");
		ctx->AddLine(">");
	}
}

GrooveTemplateMemento GrooveTemplateHandler::GetMemento()
{
	GrooveTemplateMemento memento;
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	memento.grooveInBeats = me->grooveInBeats;
	memento.nBeatsInGroove = me->nBeatsInGroove;
	return memento;
}

void GrooveTemplateHandler::SetMemento(GrooveTemplateMemento &memento)
{
	GrooveTemplateHandler *me = GrooveTemplateHandler::Instance();
	me->grooveInBeats = memento.grooveInBeats;
	me->nBeatsInGroove = memento.nBeatsInGroove;
}

void GrooveTemplateHandler::resetAmplitudes()
{
	for(std::vector<GrooveItem>::iterator i = grooveInBeats.begin(); i != grooveInBeats.end(); ++i) {
		i->amplitude = -1.0;
	}
	
}