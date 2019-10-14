#ifndef _MEDIA_ITEM_COMMANDS_H_
#define _MEDIA_ITEM_COMMANDS_H_

#include <list>
#include <algorithm>
#include <string>

#include "CommandHandler.h"

namespace MediaItemCommands {
	void Init();
};

class CmdExpandItems : public RprCommand
{
public:
	CmdExpandItems(double dAmount) : m_dAmount(dAmount)
	{}
private:
	void doCommand(int flag);
	double m_dAmount;
};

class CmdRotateItems : public RprCommand
{
public:
	CmdRotateItems(bool bRotateLengths, bool reverse) : 
	m_bRotateLengths(bRotateLengths), m_bReverse(reverse)
	{}
private:
		virtual void doCommand(int flag);
		bool m_bRotateLengths;
		bool m_bReverse;
};

class CmdExpandItemsToBar: public RprCommand
{
public:
	CmdExpandItemsToBar(int nBars) : m_nBars(nBars)
	{}
private:
	virtual void doCommand(int flag);
	int m_nBars;
};


class CmdSelectAllItemsOnTrackBetweenLoopPoints: public RprCommand
{
public:
	CmdSelectAllItemsOnTrackBetweenLoopPoints(bool allInLoop) {m_allInLoop = allInLoop;}
private:
	virtual void doCommand(int flag);
	bool m_allInLoop;
};

class CmdInsertMidiNote : public RprCommand
{
public:
	CmdInsertMidiNote() {}
private:
	virtual void doCommand(int flag);

};

class CmdPitchUpMidi : public RprCommand
{
public:
	CmdPitchUpMidi(int amt) {m_pitchAmt = amt;}
private:
	virtual void doCommand(int flag);
	int m_pitchAmt;

};

class CmdVelChangeMidi : public RprCommand
{
public:
	CmdVelChangeMidi(int amt) {m_velAmt = amt;}
private:
	virtual void doCommand(int flag);
	int m_velAmt;
};

class CmdAddToColourComponents  : public RprCommand
{
public:
	CmdAddToColourComponents(char r, char g, char b) {m_r = r; m_g = g; m_b = b;}
private:
	virtual void doCommand(int flag);
	char m_r;
	char m_g;
	char m_b;
};

class CmdSetItemNameMidi : public RprCommand
{
public:
	CmdSetItemNameMidi(bool trackMidiName) {m_trackMidiName = trackMidiName;}
private:
	virtual void doCommand(int flag);
	bool m_trackMidiName;

};

class CmdIncreaseItemRate : public RprCommand
{
public:
	CmdIncreaseItemRate(double amt) {m_amt = amt;}
private:
	virtual void doCommand(int flag);
	double m_amt;
};

class CmdRemoveAllMarkersWithPrefix : public RprCommand
{
public:
	CmdRemoveAllMarkersWithPrefix(std::string prefix) {m_prefix = prefix;}
private:
	virtual void doCommand(int flag);
	std::string m_prefix;
};


class CmdDeselectIfNotStartInTimeSelection : public RprCommand
{
public:
	CmdDeselectIfNotStartInTimeSelection() {}
private:
	virtual void doCommand(int flag);
};

#endif /* _MEDIA_ITEM_COMMANDS_H_ */
