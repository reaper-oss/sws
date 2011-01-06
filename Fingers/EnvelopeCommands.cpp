#include "stdafx.h"

#include "Envelope.h"
#include "EnvelopeCommands.h"
#include "CommandHandler.h"
#include "reaper_plugin_functions.h"
#include "reaper_helper.h"

#include <numeric>
#include <math.h>

void EnvelopeCommands::Init()
{
	static CReaperCmdReg CommandTable[] =
	{
	  CReaperCmdReg("SWS/FNG: move selected envelope points right (16th)", "FNG_ENVRIGHT_16", (CReaperCommand *)new AddToEnvPoints(1.0/16.0, POINTTIME),UNDO_STATE_TRACKCFG),
	  CReaperCmdReg("SWS/FNG: move selected envelope points left (16th)", "FNG_ENVLEFT_16", (CReaperCommand *)new AddToEnvPoints(-1.0/16.0, POINTTIME),UNDO_STATE_TRACKCFG),
	  
	  CReaperCmdReg("SWS/FNG: move selected envelope points right (32nd)", "FNG_ENVRIGHT_32",	(CReaperCommand *)new AddToEnvPoints(1.0/32.0, POINTTIME),UNDO_STATE_TRACKCFG),
	  CReaperCmdReg("SWS/FNG: move selected envelope points left (32nd)", "FNG_ENVLEFT_32", (CReaperCommand *)new AddToEnvPoints(-1.0/32.0, POINTTIME),UNDO_STATE_TRACKCFG),
		
	  CReaperCmdReg("SWS/FNG: move selected envelope points up", "FNG_ENVUP", (CReaperCommand *)new AddToEnvPoints( 1.0, POINTPOSITION),UNDO_STATE_TRACKCFG),
	  CReaperCmdReg("SWS/FNG: move selected envelope points down", "FNG_ENVDOWN", (CReaperCommand *)new AddToEnvPoints(  -1.0, POINTPOSITION),UNDO_STATE_TRACKCFG),
		
	  CReaperCmdReg("SWS/FNG: shift selected envelope points up on right", "FNG_ENV_LINEARADD", (CReaperCommand *)new LinearShiftAmplitude( 1.0, false),UNDO_STATE_TRACKCFG),
	  CReaperCmdReg("SWS/FNG: shift selected envelope points down on right", "FNG_ENV_LINEARSUB", (CReaperCommand *)new LinearShiftAmplitude( -1.0, false),UNDO_STATE_TRACKCFG),
	  CReaperCmdReg("SWS/FNG: shift selected envelope points up on left", "FNG_ENV_LINEARADD_REV", (CReaperCommand *)new LinearShiftAmplitude( 1.0, true),UNDO_STATE_TRACKCFG),
	  CReaperCmdReg("SWS/FNG: shift selected envelope points down on left", "FNG_ENV_LINEARSUB_REV", (CReaperCommand *)new LinearShiftAmplitude( -1.0, true),UNDO_STATE_TRACKCFG),
	  
	  CReaperCmdReg("SWS/FNG: expand amplitude of selected envelope points around midpoint", "FNG_ENV_EXP_MID", (CReaperCommand *)new CompressExpandPoints( 1.02, 0.0),UNDO_STATE_TRACKCFG),
	  CReaperCmdReg("SWS/FNG: compress amplitude of selected envelope points around midpoint", "FNG_ENV_COMPR_MID", (CReaperCommand *)new CompressExpandPoints( 0.98, 0.0),UNDO_STATE_TRACKCFG),

	  CReaperCmdReg("SWS/FNG: time compress selected envelope points", "FNG_ENV_TIME_COMP", (CReaperCommand *)new TimeCompressExpandPoints( -0.05),UNDO_STATE_TRACKCFG),
	  CReaperCmdReg("SWS/FNG: time stretch selected envelope points", "FNG_ENV_TIME_STRETCH", (CReaperCommand *)new TimeCompressExpandPoints( 0.05),UNDO_STATE_TRACKCFG),

	};

	CReaperCommandHandler *Handler = CReaperCommandHandler::Instance();
	Handler->AddCommands(CommandTable, __ARRAY_SIZE(CommandTable));
}

void TimeCompressExpandPoints::DoCommand(int flag)
{
	TrackEnvelope *env = GetSelectedTrackEnvelope(0);
	if(env == NULL)
		return;
	CEnvelope Cenv(env);
	EnvPoints &points = Cenv.GetPoints();
	bool firstPointFound = false;
	double firstPointTime = 0.0f;
	for(EnvPointsIter i = points.begin(); i != points.end(); i++) {
		if (i->IsSelected()) {
			if(firstPointFound)
				i->SetTime( i->GetTime() + (i->GetTime() - firstPointTime) * m_dAmount);
			else {
				firstPointFound = true;
				firstPointTime = i->GetTime();
			}
		}
	}
	Cenv.Write();
}

void AddToEnvPoints::DoCommand(int flag)
{
	TrackEnvelope *env = GetSelectedTrackEnvelope(0);
	if(env == NULL)
		return;
	double cursorPos = GetCursorPosition();
	CEnvelope Cenv(env);
	if( m_pp == POINTTIME)
	{
		double beat = (240.0 / TimeMap2_GetDividedBpmAtTime(0, cursorPos));
		AddToPointPosition op(beat * m_dAmount);
		Cenv.ApplyToPoints(op);
	}
	else
	{
		double amt = (Cenv.GetMax() - Cenv.GetMin()) / 100 * m_dAmount;
		Cenv.ApplyToPoints(AddToValue(amt));
	}
	Cenv.Write();
}

bool isSelected(CEnvelopePoint &p)
{ return p.IsSelected();}


void LinearShiftAmplitude::DoCommand(int flag)
{
	TrackEnvelope *env = GetSelectedTrackEnvelope(0);
	if(env == NULL)
		return;
	CEnvelope Cenv(env);
	std::vector<CEnvelopePoint> &points = Cenv.GetPoints();
	std::vector<CEnvelopePoint>::iterator it = std::find_if(points.begin(),
		points.end(), isSelected);

	if(it == points.end()) return;

	double amt = (Cenv.GetMax() - Cenv.GetMin()) / 100 * m_dAmount;

	double p0 = it->GetTime();
	std::vector<CEnvelopePoint>::reverse_iterator rit;
	rit = std::find_if(points.rbegin(), points.rend(), isSelected);
	double pN = rit->GetTime();
	if (p0 == pN)
		return; // same point in time

	double m = amt / (pN - p0);
	double b;
	
	if(m_bReverse) 
		m *= -1.0;

	if(m_bReverse)
		b = amt - m *p0;
	else
		b = -m * p0;

	Cenv.ApplyToPoints(AddToValue(m, b));
	Cenv.Write();
}

void BoundsOfSelectedPoints(EnvPoints &points, double *min, double *max)
{
	for(std::vector<CEnvelopePoint>::iterator i = points.begin(); i != points.end(); i++) {
		if( i->IsSelected()) {
			if( i->GetValue() > *max)
				*max = i->GetValue();
			if( i->.GetValue() < *min)
				*min = i->GetValue();
		}
	}
}

void CompressExpandPoints::DoCommand(int flag)
{
	TrackEnvelope *env = GetSelectedTrackEnvelope(0);
	if(env == NULL)
		return;
	CEnvelope Cenv(env);
	std::vector<CEnvelopePoint> &points = Cenv.GetPoints();
	std::vector<CEnvelopePoint>::iterator it = std::find_if(points.begin(),
		points.end(), isSelected);

	if(it == points.end()) return;

	double maxVal = Cenv.GetMin();
	double minVal = Cenv.GetMax();
	BoundsOfSelectedPoints(points, &minVal, &maxVal);
	double midPoint = (maxVal + minVal) / 2;

	double p0 = it->GetTime();
	std::vector<CEnvelopePoint>::reverse_iterator rit;
	rit = std::find_if(points.rbegin(), points.rend(), isSelected);
	double pN = rit->GetTime();
	if(p0 == pN)
		return;

	double m = m_dGradientFactor * (m_dAmount - 1) * (pN - p0);

	for(std::vector<CEnvelopePoint>::iterator i = points.begin(); i != points.end(); i++) {
		if( i->IsSelected()) {
			if( i->GetValue() == midPoint)
				continue;
			double extremePoint;
			if( i->GetValue() > midPoint)
				extremePoint = maxVal;
			else
				extremePoint = minVal;
			
			double normalized = (i->GetValue() - midPoint) / (extremePoint - midPoint);
			normalized *= m * (i->GetTime() - p0) + m_dAmount;
			(*i).SetValue(normalized * ( extremePoint - midPoint) + midPoint);
		}
	}
	Cenv.Write();
}
