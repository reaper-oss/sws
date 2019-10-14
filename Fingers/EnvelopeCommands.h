#ifndef _ENVELOPE_COMMANDS_H_
#define _ENVELOPE_COMMANDS_H_

#include "CommandHandler.h"

namespace EnvelopeCommands 
{
	void Init();
};

class TimeCompressExpandPoints : public RprCommand
{
public:
	TimeCompressExpandPoints(double dAmount) : m_dAmount(dAmount)
	{}
private:
	void doCommand(int flag);
	double m_dAmount;
};

enum PointProperty { POINTPOSITION, POINTTIME };

class AddToEnvPoints : public RprCommand
{
public:
	AddToEnvPoints(double dAmount, PointProperty pp ) : m_dAmount(dAmount), m_pp(pp)
	{}
private:
	virtual void doCommand(int flag);
	double m_dAmount;
	PointProperty m_pp;
};

class LinearShiftAmplitude : public RprCommand
{
public:
	LinearShiftAmplitude(double dAmount, bool bReverse) : m_dAmount(dAmount), m_bReverse(bReverse)
	{}
private:
	virtual void doCommand(int flag);
	double m_dAmount;
	bool m_bReverse;
};

class CompressExpandPoints : public RprCommand
{
public:
	CompressExpandPoints(double dAmount, double gradientFactor) : m_dAmount(dAmount), m_dGradientFactor(gradientFactor)
	{}
private:
	virtual void doCommand(int flag);

	double m_dAmount;
	double m_dGradientFactor;
	
};

#endif /* _ENVELOPE_COMMANDS_H_ */
