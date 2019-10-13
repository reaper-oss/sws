#include "stdafx.h"
#include "Envelope.h"
#include "RprStateChunk.h"

RprEnvelopePoint::RprEnvelopePoint(RprEnvelope *parent /* = NULL */) :
    m_time(0.0), m_parameterValue(0.0), m_envelopeShape(Linear),
    m_selUnknown(0), m_selected(0), m_bezierUnknown(0),
    m_bezierTension(0.0), m_parent(parent)
{
}

void
RprEnvelopePoint::setTime(double time)
{
    m_time = time;
}

double
RprEnvelopePoint::time() const
{
    return m_time;
}

double
RprEnvelopePoint::value() const
{
    return m_parameterValue;
}

RprEnvelopePoint::EnvShape
RprEnvelopePoint::shape() const
{
    return m_envelopeShape;
}

bool
RprEnvelopePoint::selected() const
{
    return !!m_selected;
}

double
RprEnvelopePoint::bezierTension() const
{
    return m_bezierTension;
}

bool
RprEnvelopePoint::operator<(const RprEnvelopePoint& rhs) const
{
    return m_time < rhs.m_time;
}

std::string
RprEnvelopePoint::toString() const
{
    std::ostringstream oss;
    oss << "PT" << " " << time() << " " << value() << " " << shape();

    if (m_selected)
    {
        oss << " " << m_selUnknown << " 1";
    }

    if (m_bezierTension != 0.0)
    {
        if (!m_selected)
        {
            oss << " " << m_selUnknown << " 0";
        }
        oss << " " << m_bezierUnknown
            << " " << bezierTension();
    }
    oss << std::endl;
    return oss.str();
}

void
RprEnvelopePoint::fromString(const char *stateData)
{
    int selflag=0;
    sscanf(stateData, "PT %lf %lf %d %d %d %d %lf", &m_time,
        &m_parameterValue, (int*)&m_envelopeShape, &m_selUnknown,
        &selflag, &m_bezierUnknown, &m_bezierTension);
    m_selected = (selflag&1)==1;
}

RprEnvelope::RprEnvelope(TrackEnvelope* env) :
dParamMin(0.0), dParamMax(0.0), m_env(NULL), dNeutralVal(0.0),
nParamIndex(0)
{
    if (env == NULL)
    {
        return;
    }
    m_env = env;

    RprStateChunk chunk(GetSetObjectState(m_env, ""));
    const char *pEnv = chunk.get();

    const char *envPtr = pEnv;
    char title[256];

    sscanf(envPtr, "<%s %d %lf %lf %lf", (char*)&title, &nParamIndex, &dParamMin,
        &dParamMax, &dNeutralVal);

    m_szTitle = title;

    if (m_szTitle.find("VOLENV") != std::string::npos)
    {
        dParamMax = 2.0;
        dParamMin = 0.0;
    }

    if (m_szTitle.find("PANENV") != std::string::npos || m_szTitle.find("WIDTHENV") != std::string::npos )
    {
        dParamMax = 1.0;
        dParamMin = -1.0;
    }
    
    if (m_szTitle.find("PLAYSPEEDENV") != std::string::npos)
    {
        dParamMax = 4.0;
        dParamMin = 0.1;
    }

    if (m_szTitle == "MUTEENV")
    {
        dParamMax = 1.0;
        dParamMin = 0.0;
    }

    envPtr = strchr(envPtr, '\n') + 1;
    if (envPtr[0] == '\0')
    {
        return;
    }

    sscanf(envPtr, "ACT %d", &nActive);

    envPtr = strchr(envPtr, '\n') + 1;

    if (envPtr[0] == '\0')
    {
        return;
    }

    sscanf(envPtr, "VIS %d %d %lf", &nVisible, &nAutomationInLane, &dVISUnknown);

    envPtr = strchr(envPtr, '\n') + 1;
    if (envPtr[0] == '\0')
    {
        return;
    }

    sscanf(envPtr, "LANEHEIGHT %d %d", &nLaneHeight, &nLHUnknown);

    envPtr = strchr(envPtr, '\n') + 1;
    if(envPtr[0] == '\0')
    {
        return;
    }

    sscanf(envPtr, "ARM %d", &nArm);

    envPtr = strchr(envPtr, '\n') + 1;
    if(envPtr[0] == '\0')
    {
        return;
    }

    sscanf(envPtr, "DEFSHAPE %d", &nDefaultShape);

    envPtr = strchr(envPtr, '\n') + 1;

    while(envPtr[0] != '\0' && envPtr[0] != '>')
    {
        RprEnvelopePoint pt(this);
        pt.fromString(envPtr);
        vPoints.push_back(pt);
        envPtr = strchr(envPtr, '\n') + 1;
    }
}

void
RprEnvelope::Write()
{
    std::sort(vPoints.begin(), vPoints.end());

    std::stringstream oss;
    oss.unsetf(std::ios::floatfield);
    oss.precision(10);
    if( m_szTitle == "PARAMENV")
    {
        oss << "<" << m_szTitle
            << " " << nParamIndex
            << " " << dParamMin
            << " " << dParamMax
            << " " << dNeutralVal
            << std::endl;
    }
    else
    {
        oss << "<" << m_szTitle << std::endl;
    }

    oss << "ACT"
        << " " << nActive
        << std::endl;

    oss << "VIS"
        << " " << nVisible
        << " " << nAutomationInLane
        << " " << dVISUnknown
        << std::endl;

    oss << "LANEHEIGHT"
        << " " << nLaneHeight
        << " " << nLHUnknown
        << std::endl;

    oss << "ARM"
        << " " << nArm
        << std::endl;

    oss << "DEFSHAPE"
        << " " << nDefaultShape
        << std::endl;

    for (std::vector<RprEnvelopePoint>::iterator it = vPoints.begin(); it != vPoints.end(); ++it)
    {
        oss << it->toString();
    }
    oss << ">" << std::endl;
    std::string envelopeState = oss.str();
    GetSetObjectState(m_env, envelopeState.c_str());
}

void
RprEnvelopePoint::setValue(double value)
{
    m_parameterValue = value;
    if (m_parent)
    {
        double maxValue = m_parent->GetMax();
        if (m_parameterValue > maxValue)
        {
            m_parameterValue = maxValue;
        }

        double minValue = m_parent->GetMin();
        if(m_parameterValue < minValue)
        {
            m_parameterValue = minValue;
        }
    }
}

void
RprEnvelope::Add(RprEnvelopePoint &point)
{
    vPoints.push_back(point);
}

RprEnvelope::~RprEnvelope()
{}
