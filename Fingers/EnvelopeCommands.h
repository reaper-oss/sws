#ifndef _ENVELOPE_COMMANDS_H_
#define _ENVELOPE_COMMANDS_H_
#include "reaper_plugin_functions.h"
//#include "reaper_plugin.h"
#include "CommandHandler.h"
//#include "MGButton.hxx"

namespace EnvelopeCommands {
	void Init();
};

class TestEnvCommand : CReaperCommand
{
public:
	TestEnvCommand() {}
private:
	virtual void DoCommand(int flag);
};

class PulseEnvCommand : CReaperCommand
{
public:
	enum PulseType {SQUARE, BEZIER_1};
	PulseEnvCommand(int nDivisor) : m_nDivisor(nDivisor)
	{}
private:
	virtual void DoCommand(int flag);
	int m_nDivisor;
};

class TimeCompressExpandPoints : CReaperCommand
{
public:
	TimeCompressExpandPoints(double dAmount) : m_dAmount(dAmount)
	{}
private:
	void DoCommand(int flag);
	double m_dAmount;
};



class SineEnvCommand : CReaperCommand
{
public:
	SineEnvCommand(int nDivisor) : m_nDivisor(nDivisor)	{}
private:
	virtual void DoCommand(int flag);
	int m_nDivisor;
};

enum PointProperty { POINTPOSITION, POINTTIME };

class AddToEnvPoints : CReaperCommand
{
public:
	AddToEnvPoints(double dAmount, PointProperty pp ) : m_dAmount(dAmount), m_pp(pp)
	{}
private:
	virtual void DoCommand(int flag);
	double m_dAmount;
	PointProperty m_pp;
};

class LinearShiftAmplitude : CReaperCommand
{
public:
	LinearShiftAmplitude(double dAmount, bool bReverse) : m_dAmount(dAmount), m_bReverse(bReverse)
	{}
private:
	virtual void DoCommand(int flag);
	double m_dAmount;
	bool m_bReverse;
};

//class CustomToolbarDialog;
//class MGButton;
//
//class EnvelopeGui : CReaperCommand, ClickCommand
//{
//public:
//	EnvelopeGui();
//private:
//	void DoCommand(int flag);
//	void DoClick(int ident);
//	CustomToolbarDialog *dialog;
//};

class CompressExpandPoints : CReaperCommand
{
public:
	CompressExpandPoints(double dAmount, double gradientFactor) : m_dAmount(dAmount), m_dGradientFactor(gradientFactor)
	{}
private:
	virtual void DoCommand(int flag);

	double m_dAmount;
	double m_dGradientFactor;
	
};

#endif /* _ENVELOPE_COMMANDS_H_ */