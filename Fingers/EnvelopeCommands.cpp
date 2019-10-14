#include "stdafx.h"

#include "Envelope.h"
#include "EnvelopeCommands.h"
#include "CommandHandler.h"
#include "../Breeder/BR_EnvelopeUtil.h"
#include "../Breeder/BR_Util.h"


//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
void EnvelopeCommands::Init()
{
    RprCommand::registerCommand("SWS/FNG: Move selected envelope points right (16th)", "FNG_ENVRIGHT_16", new AddToEnvPoints(1.0/16.0, POINTTIME), UNDO_STATE_TRACKCFG);
    RprCommand::registerCommand("SWS/FNG: Move selected envelope points left (16th)", "FNG_ENVLEFT_16", new AddToEnvPoints(-1.0/16.0, POINTTIME),UNDO_STATE_TRACKCFG);

    RprCommand::registerCommand("SWS/FNG: Move selected envelope points right (32nd)", "FNG_ENVRIGHT_32", new AddToEnvPoints(1.0/32.0, POINTTIME),UNDO_STATE_TRACKCFG);
    RprCommand::registerCommand("SWS/FNG: Move selected envelope points left (32nd)", "FNG_ENVLEFT_32", new AddToEnvPoints(-1.0/32.0, POINTTIME),UNDO_STATE_TRACKCFG);

    RprCommand::registerCommand("SWS/FNG: Move selected envelope points up", "FNG_ENVUP", new AddToEnvPoints( 1.0, POINTPOSITION),UNDO_STATE_TRACKCFG);
    RprCommand::registerCommand("SWS/FNG: Move selected envelope points down", "FNG_ENVDOWN", new AddToEnvPoints(  -1.0, POINTPOSITION),UNDO_STATE_TRACKCFG);

    RprCommand::registerCommand("SWS/FNG: Shift selected envelope points up on right", "FNG_ENV_LINEARADD", new LinearShiftAmplitude( 1.0, false),UNDO_STATE_TRACKCFG);
    RprCommand::registerCommand("SWS/FNG: Shift selected envelope points down on right", "FNG_ENV_LINEARSUB", new LinearShiftAmplitude( -1.0, false),UNDO_STATE_TRACKCFG);
    RprCommand::registerCommand("SWS/FNG: Shift selected envelope points up on left", "FNG_ENV_LINEARADD_REV", new LinearShiftAmplitude( 1.0, true),UNDO_STATE_TRACKCFG);
    RprCommand::registerCommand("SWS/FNG: Shift selected envelope points down on left", "FNG_ENV_LINEARSUB_REV", new LinearShiftAmplitude( -1.0, true),UNDO_STATE_TRACKCFG);

    RprCommand::registerCommand("SWS/FNG: Expand amplitude of selected envelope points around midpoint", "FNG_ENV_EXP_MID", new CompressExpandPoints( 1.02, 0.0),UNDO_STATE_TRACKCFG);
    RprCommand::registerCommand("SWS/FNG: Compress amplitude of selected envelope points around midpoint", "FNG_ENV_COMPR_MID", new CompressExpandPoints( 0.98, 0.0),UNDO_STATE_TRACKCFG);

    RprCommand::registerCommand("SWS/FNG: Time compress selected envelope points", "FNG_ENV_TIME_COMP", new TimeCompressExpandPoints( -0.05),UNDO_STATE_TRACKCFG);
    RprCommand::registerCommand("SWS/FNG: Time stretch selected envelope points", "FNG_ENV_TIME_STRETCH", new TimeCompressExpandPoints( 0.05),UNDO_STATE_TRACKCFG);

}
//!WANT_LOCALIZE_1ST_STRING_END

typedef std::vector<RprEnvelopePoint> EnvPoints;
typedef std::vector<RprEnvelopePoint>::iterator EnvPointsIter;

class RprEnvelopePointOp {
public:
    void operator() (RprEnvelopePoint &p)
    {Do(p);}
private:
    virtual void Do(RprEnvelopePoint &p) = 0;
};

class AddToPointPosition : public RprEnvelopePointOp
{
public:
    AddToPointPosition(double amt)
    { m_dAmt = amt;}

private:
    void Do(RprEnvelopePoint &p)
    {
        if (p.selected())
        {
            p.setTime( p.time() + m_dAmt);
        }
    }

    double m_dAmt;
};

class AddToValue : public RprEnvelopePointOp
{
public:
    AddToValue(double amt)
    { m_db = amt; m_dm = 0.0; }

    AddToValue(double m, double b)
    { m_dm = m; m_db = b; }

private:
    void Do(RprEnvelopePoint &p)
    {
        if (p.selected())
        {
            p.setValue( p.value() + p.time() * m_dm + m_db);
        }
    }

    double m_dm;
    double m_db;
};

void TimeCompressExpandPoints::doCommand(int flag)
{
    BR_Envelope envelope(GetSelectedEnvelope(NULL));
    if (envelope.CountSelected() < 2)
        return;

    double firstPointTime;
    envelope.GetPoint(envelope.GetSelected(0), &firstPointTime, NULL, NULL, NULL);
    for (int i = 1; i < envelope.CountSelected(); ++i)
    {
        double position;
        envelope.GetPoint(envelope.GetSelected(i), &position, NULL, NULL, NULL);
        position += (position - firstPointTime) * m_dAmount;
        envelope.SetPoint(envelope.GetSelected(i), &position, NULL, NULL, NULL);
    }

    envelope.Commit();
}

void AddToEnvPoints::doCommand(int flag)
{
    BR_Envelope envelope(GetSelectedEnvelope(NULL));
    if (!envelope.CountSelected())
        return;
    double cursorPos = GetCursorPosition();

    if( m_pp == POINTTIME)
	{
		double beat = (240.0 / TimeMap2_GetDividedBpmAtTime(0, cursorPos));
		double amount = beat * m_dAmount;

		for (int i = 0; i < envelope.CountSelected(); ++i)
		{
			int id = envelope.GetSelected(i);
			double position;
			envelope.GetPoint(id, &position, NULL, NULL, NULL);
			position += amount;
			envelope.SetPoint(id, &position, NULL, NULL, NULL);
		}
	}
	else
	{
		double amount = (envelope.LaneMaxValue() - envelope.LaneMinValue()) / 100 * m_dAmount;

		for (int i = 0; i < envelope.CountSelected(); ++i)
		{
			int id = envelope.GetSelected(i);
			double value;
			envelope.GetPoint(id, NULL, &value, NULL, NULL);
			value += amount;
			SetToBounds(value, envelope.LaneMinValue(), envelope.LaneMaxValue());
			envelope.SetPoint(id, NULL, &value, NULL, NULL);
		}
	}
	envelope.Commit();
}

bool selected(RprEnvelopePoint &p)
{ return p.selected();}


void LinearShiftAmplitude::doCommand(int flag)
{
    BR_Envelope envelope(GetSelectedEnvelope(NULL));
    if (envelope.CountSelected() < 2)
        return;

    double amt = (envelope.LaneMaxValue() - envelope.LaneMinValue()) / 100 * m_dAmount;

    double p0; envelope.GetPoint(envelope.GetSelected(0), &p0, NULL, NULL, NULL);
    double pN; envelope.GetPoint(envelope.GetSelected(envelope.CountSelected()-1), &pN, NULL, NULL, NULL);
    if (p0 == pN)
        return; // same point in time

    double m = amt / (pN - p0);
    double b;

    if (m_bReverse)
        m *= -1.0;
    if (m_bReverse)
        b = amt - m *p0;
    else
        b = -m * p0;

    for (int i = 0; i < envelope.CountSelected(); ++i)
    {
        double position, value;
        envelope.GetPoint(envelope.GetSelected(i), &position, &value, NULL, NULL);

        value = SetToBounds(value + position * m + b, envelope.LaneMinValue(), envelope.LaneMaxValue());
        envelope.SetPoint(envelope.GetSelected(i), NULL, &value, NULL, NULL);
    }
    envelope.Commit();
}

void BoundsOfSelectedPoints(RprEnvelope::iterator start, RprEnvelope::iterator end, double *min, double *max)
{
        for(RprEnvelope::iterator i = start; i != end; ++i)
    {
                if( i->selected())
        {
                        if( i->value() > *max)
            {
                                *max = i->value();
            }
                        if( i->value() < *min)
            {
                                *min = i->value();
            }
                }
        }
}

void CompressExpandPoints::doCommand(int flag)
{
    BR_Envelope envelope(GetSelectedEnvelope(NULL));
    if (envelope.CountSelected() < 2)
        return;

    double minVal, maxVal;
    envelope.GetSelectedPointsExtrema(&minVal, &maxVal);
    double midPoint = (maxVal + minVal) / 2;

    double p0; envelope.GetPoint(envelope.GetSelected(0), &p0, NULL, NULL, NULL);
    double pN; envelope.GetPoint(envelope.GetSelected(envelope.CountSelected()-1), &pN, NULL, NULL, NULL);
    if (p0 == pN)
        return; // same point in time

    double m = m_dGradientFactor * (m_dAmount - 1) * (pN - p0);

    if (envelope.IsTempo() && *ConfigVar<int>("tempoenvtimelock") == 1)
    {
        double t0, b0; int s0;
        envelope.GetPoint(0, &t0, &b0, &s0, NULL);
        double t0_old = t0;
        double b0_old = b0;

        for (int i = 0; i < envelope.CountPoints(); ++i)
        {
            double t1, b1; int s1;
            envelope.GetPoint(i, &t1, &b1, &s1, NULL);

            double newValue = b1;
            if (envelope.GetSelection(i) && b1 != midPoint)
            {
                double extremePoint = (b1 > midPoint) ? maxVal : minVal;
                double normalized = (b1 - midPoint) / (extremePoint - midPoint);
                normalized *= m * (t1 - p0) + m_dAmount;
                newValue = SetToBounds(normalized * ( extremePoint - midPoint) + midPoint, envelope.LaneMinValue(), envelope.LaneMaxValue());
            }

            double newTime = t1;
            if (s0 == SQUARE)
                newTime = t0 + ((t1 - t0_old) * b0_old) / b0;
            else
                newTime = t0 + ((t1 - t0_old) * (b0_old + b1)) / (b0 + newValue);

            t0 = newTime;
            b0 = newValue;
            s0 = s1;
            t0_old = t1;
            b0_old = b1;

            envelope.SetPoint(i, &newTime, &newValue, NULL, NULL);
        }
    }
    else
    {
        for (int i = 0; i < envelope.CountSelected(); ++i)
        {
            double position, value;
            envelope.GetPoint(envelope.GetSelected(i), &position, &value, NULL, NULL);

            if (value == midPoint)
               continue;

            double extremePoint = (value > midPoint) ? maxVal : minVal;
            double normalized = (value - midPoint) / (extremePoint - midPoint);
            normalized *= m * (position - p0) + m_dAmount;
            value = SetToBounds(normalized * ( extremePoint - midPoint) + midPoint, envelope.LaneMinValue(), envelope.LaneMaxValue());
            envelope.SetPoint(envelope.GetSelected(i), NULL, &value, NULL, NULL);
        }
    }

    envelope.Commit();
}
