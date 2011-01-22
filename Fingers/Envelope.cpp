#include "stdafx.h"

#include "Envelope.h"

std::string CEnvelopePoint::ToString()
{
	std::ostringstream oss;
	oss << "PT"
		<< " " << GetTime()
		<< " " << GetValue()
		<< " " << GetShape();

		if(nSelected == 1)
		{ 
			oss << " " << nSelUnknown
			<< " " << nSelected;
		}

		if(dBezierTension != 0.0)
		{ 
			if(nSelected == 0)
			{
				oss << " " << nSelUnknown
				<< " " << nSelected;
			}
			oss << " " << nBezUnknown
			<< " " << GetBezierTension();
		}
		oss << std::endl;
		return oss.str();
}

void CEnvelopePoint::FromString(char *stateData)
{
	sscanf(stateData, "PT %lf %lf %d %d %d %d %lf", &dTime,
				&dParameterValue, &nEnvShape, &nSelUnknown,
				&nSelected, &nBezUnknown, &dBezierTension);
}

CEnvelope::CEnvelope(TrackEnvelope* env) :
dParamMin(0.0), dParamMax(0.0), m_env(NULL), dNeutralVal(0.0),
nParamIndex(0)
{
	if(env == NULL)
		return;

	char *pEnv = GetSetObjectState(env,"");
	m_env = env;

	int status;
	char *envPtr = pEnv;
	char title[256];
	status = sscanf(envPtr, "<%s %d %lf %lf %lf", &title, &nParamIndex, &dParamMin,
		&dParamMax, &dNeutralVal);

	m_szTitle = title;

	if(m_szTitle.find("VOLENV") != std::string::npos)
	{
		dParamMax = 2.0;
		dParamMin = 0.0;
	}

	if(m_szTitle.find("PANENV") != std::string::npos)
	{
		dParamMax = 1.0;
		dParamMin = -1.0;
	}

	if(m_szTitle == "MUTEENV")
	{
		dParamMax = 1.0;
		dParamMin = 0.0;
	}

	envPtr = strchr(envPtr, '\n') + 1;
	if(envPtr[0] == '\0') {
		FreeHeapPtr(pEnv);
		return;
	}
	
	status = sscanf(envPtr, "ACT %d", &nActive);
	
	envPtr = strchr(envPtr, '\n') + 1;
	if(envPtr[0] == '\0') {
		FreeHeapPtr(pEnv);
		return;
	}
	
	status = sscanf(envPtr, "VIS %d %d %lf", &nVisible, &nAutomationInLane, &dVISUnknown);

	envPtr = strchr(envPtr, '\n') + 1;
	if(envPtr[0] == '\0') {
		FreeHeapPtr(pEnv);
		return;
	}
	
	status = sscanf(envPtr, "LANEHEIGHT %d %d", &nLaneHeight, &nLHUnknown);

	envPtr = strchr(envPtr, '\n') + 1;
	if(envPtr[0] == '\0') {
		FreeHeapPtr(pEnv);
		return;
	}

	status = sscanf(envPtr, "ARM %d", &nArm);

	envPtr = strchr(envPtr, '\n') + 1;
	if(envPtr[0] == '\0') {
		FreeHeapPtr(pEnv);
		return;
	}

	status = sscanf(envPtr, "DEFSHAPE %d", &nDefaultShape);

	envPtr = strchr(envPtr, '\n') + 1;
	
	while(envPtr[0] != '\0' && envPtr[0] != '>')
	{
		CEnvelopePoint pt(this);
		pt.FromString(envPtr);
		vPoints.push_back(pt);
		envPtr = strchr(envPtr, '\n') + 1;
	}
}

void CEnvelope::Write()
{
	std::sort(vPoints.begin(), vPoints.end(), CEnvelopePoint::ComparePointPos);

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
		oss << "<" << m_szTitle << std::endl;

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

	for(std::vector<CEnvelopePoint>::iterator it = vPoints.begin();
		it != vPoints.end(); it++)
		oss << (*it).ToString();
		
	oss << ">" << std::endl;
	std::string strOut = oss.str();
	GetSetObjectState(m_env, strOut.c_str());
}

void CEnvelopePoint::SetValue(double value)
{
	dParameterValue = value;
	if(pParent) {
		double maxValue = pParent->GetMax();
		if( dParameterValue > maxValue)
			dParameterValue = maxValue;
		double minValue = pParent->GetMin();
		if( dParameterValue < minValue)
			dParameterValue = minValue;
	}
}

void CEnvelope::Add(CEnvelopePoint &point)
{
	vPoints.push_back(point);
}


CEnvelope::~CEnvelope()
{
	
}