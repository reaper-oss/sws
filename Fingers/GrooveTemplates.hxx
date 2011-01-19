#ifndef _GROOVE_TEMPLATES_H_
#define _GROOVE_TEMPLATES_H_

#include <string>
#include <vector>
#include "reaper_plugin_functions.h"
#include "GrooveDialog.hxx"



struct GrooveItem {
	double position;
	double amplitude;

	GrooveItem operator-(const GrooveItem &lhs)
	{ 
		GrooveItem newItem;
		newItem.position = position - lhs.position;
		newItem.amplitude = amplitude;
		return newItem;
	}
};

class GrooveTemplateMemento {
public:
	int nBeatsInGroove;
	std::vector<GrooveItem> grooveInBeats;
};

class GrooveTemplateHandler
{
public:
	enum GrooveMarkerStart {EDITCURSOR, CURRENTBAR};

	static GrooveTemplateHandler *Instance();

	static void ClearGroove();
	static void StoreGroove();
	static void StoreGrooveFromMidiEditor();
	static void ApplyGroove(int beatDivider, double strength);
	static void ApplyGrooveToMidiEditor(int beatDivider, double strength);

	static bool isGrooveEmpty();

	static bool LoadGroove(std::string &fileName, std::string &errorMessage);
	static bool SaveGroove(std::string &fileName, std::string &errorMessage);
		
	static void MarkGroove(int multiple);
	static void SetMarkerStart(GrooveMarkerStart markerStart);

	static GrooveTemplateMemento GetMemento();
	static void SetMemento(GrooveTemplateMemento &memento);

	std::string GrooveToString();
	
	std::string GetGrooveDir();
	void SetGrooveDir(std::string&);
	
	int GetGrooveTolerance();
	void SetGrooveTolerance(int);

	void resetAmplitudes();

	int GetGrooveStrength();
	void SetGrooveStrength(int);

	int GetGrooveTarget();
	void SetGrooveTarget(int);


	void showGrooveDialog();
	void toggleGrooveDialog();

	static void Init();

private:
	GrooveTemplateHandler();

	void AddGrooveMarker(int index, double pos, char *name);

	std::string GetGrooveString(int index); /* 0 to n-1. nth will return "" */
	std::string GetGrooveMarkerString(int index); /* 0 to n-1. nth will return "" */

	static bool ProcessExtensionLine(const char *line, ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg);
	static void SaveGrooveMarkers(ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg);
	static void BeginLoadProjectState(bool isUndo, struct project_config_extension_t *reg);

	struct GrooveMarker {
		int index;
		std::string name;
		double pos;
	};

	static GrooveTemplateHandler *instance;
	int nBeatsInGroove;
	std::vector<GrooveItem> grooveInBeats;

	std::vector<GrooveMarker> grooveMarkers;
	GrooveMarkerStart grooveMarkerStart;
	project_config_extension_t grooveMarkersHelper;
	GrooveDialog *mGrooveDialog;
};

#endif /*_GROOVE_TEMPLATES_H_*/