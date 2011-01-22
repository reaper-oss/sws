#ifndef _GROOVE_QUANTIZE_H_
#define _GROOVE_QUANTIZE_H_

#include <string>

class GrooveQuantize {
public:
	enum GrooveMarkerStart {EDITCURSOR, CURRENTBAR};

	GrooveQuantize();
	void extractGrooveFromSelected();
	void extractGrooveFromMidiEditor();
	void applyGroove(int beatDivider, double strength);
	void applyGrooveToMidiEditor(int beatDivider, double strength);
	bool userGrooveEmpty();
	bool saveGroove(std::string &fileName, std::string &errorMessage);
	bool loadGroove(std::string &fileName, std::string &errorMessage);
	void getGrooveString(std::string &grooveString);
	std::string getGrooveDir();
	void markGroove(int nRepeat, GrooveMarkerStart markerStart);
	bool grooveIsMarked();
	
private:

};

#endif /* _GROOVE_QUANTIZE_H_ */