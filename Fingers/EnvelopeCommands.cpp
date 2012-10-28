#include "stdafx.h"

#include "Envelope.h"
#include "EnvelopeCommands.h"
#include "CommandHandler.h"


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
        TrackEnvelope *trackEnvelope = GetSelectedTrackEnvelope(0);
        if (trackEnvelope == NULL)
    {
                return;
    }

    RprEnvelope envelope(trackEnvelope);

    bool firstPointFound = false;
        double firstPointTime = 0.0;

        for (RprEnvelope::iterator i = envelope.begin(); i != envelope.end(); i++)
    {
                if (i->selected())
        {
                        if (firstPointFound)
            {
                                i->setTime( i->time() + (i->time() - firstPointTime) * m_dAmount);
            }
                        else
            {
                                firstPointFound = true;
                                firstPointTime = i->time();
                        }
                }
        }

        envelope.Write();
}

void AddToEnvPoints::doCommand(int flag)
{
        TrackEnvelope *trackEnvelope = GetSelectedTrackEnvelope(0);
        if (trackEnvelope == NULL)
    {
                return;
    }
    RprEnvelope envelope(trackEnvelope);

        double cursorPos = GetCursorPosition();

    if( m_pp == POINTTIME)
        {
                double beat = (240.0 / TimeMap2_GetDividedBpmAtTime(0, cursorPos));
                AddToPointPosition op(beat * m_dAmount);
                envelope.ApplyToPoints(op);
        }
        else
        {
                double amt = (envelope.GetMax() - envelope.GetMin()) / 100 * m_dAmount;
                AddToValue op(amt);
                envelope.ApplyToPoints(op);
        }
        envelope.Write();
}

bool selected(RprEnvelopePoint &p)
{ return p.selected();}


void LinearShiftAmplitude::doCommand(int flag)
{
        TrackEnvelope *trackEnvelope = GetSelectedTrackEnvelope(0);
        if (trackEnvelope == NULL)
    {
                return;
    }
    RprEnvelope envelope(trackEnvelope);

        std::vector<RprEnvelopePoint>::iterator it = std::find_if(envelope.begin(),
                envelope.end(), selected);

        if (it == envelope.end())
    {
        return;
    }

        double amt = (envelope.GetMax() - envelope.GetMin()) / 100 * m_dAmount;

        double p0 = it->time();
        std::vector<RprEnvelopePoint>::reverse_iterator rit;
        rit = std::find_if(envelope.rbegin(), envelope.rend(), selected);
        double pN = rit->time();
        if (p0 == pN)
    {
                return; // same point in time
    }

        double m = amt / (pN - p0);
        double b;

        if (m_bReverse)
    {
                m *= -1.0;
    }

        if (m_bReverse)
    {
                b = amt - m *p0;
    }
        else
    {
                b = -m * p0;
    }

        AddToValue op(m, b);
        envelope.ApplyToPoints(op);
        envelope.Write();
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
        TrackEnvelope *env = GetSelectedTrackEnvelope(0);
        if(env == NULL)
                return;
        RprEnvelope Cenv(env);
        std::vector<RprEnvelopePoint>::iterator it = std::find_if(Cenv.begin(),
                Cenv.end(), selected);

        if(it == Cenv.end()) return;

        double maxVal = Cenv.GetMin();
        double minVal = Cenv.GetMax();
        BoundsOfSelectedPoints(Cenv.begin(), Cenv.end(), &minVal, &maxVal);
        double midPoint = (maxVal + minVal) / 2;

        double p0 = it->time();
        std::vector<RprEnvelopePoint>::reverse_iterator rit;
        rit = std::find_if(Cenv.rbegin(), Cenv.rend(), selected);
        double pN = rit->time();
        if(p0 == pN)
                return;

        double m = m_dGradientFactor * (m_dAmount - 1) * (pN - p0);

        for(RprEnvelope::iterator i = Cenv.begin(); i != Cenv.end(); i++)
    {
                if (i->selected())
        {
                        if (i->value() == midPoint)
            {
                                continue;
            }

                        double extremePoint;
                        if( i->value() > midPoint)
            {
                                extremePoint = maxVal;
            }
                        else
            {
                                extremePoint = minVal;
            }

                        double normalized = (i->value() - midPoint) / (extremePoint - midPoint);
                        normalized *= m * (i->time() - p0) + m_dAmount;
                        i->setValue(normalized * ( extremePoint - midPoint) + midPoint);
                }
        }
        Cenv.Write();
}
