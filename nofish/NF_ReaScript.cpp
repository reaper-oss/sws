/******************************************************************************
/ NF_ReaScript.cpp
/
/ Copyright (c) 2018 nofish
/ http://forum.cockos.com/member.php?u=6870
/ http://github.com/reaper-oss/sws
/
/ Permission is hereby granted, free of charge, to any person obtaining a copy
/ of this software and associated documentation files (the "Software"), to deal
/ in the Software without restriction, including without limitation the rights to
/ use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
/ of the Software, and to permit persons to whom the Software is furnished to
/ do so, subject to the following conditions:
/
/ The above copyright notice and this permission notice shall be included in all
/ copies or substantial portions of the Software.
/
/ THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
/ EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
/ OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
/ NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
/ HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
/ WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/ FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
/ OTHER DEALINGS IN THE SOFTWARE.
/
******************************************************************************/


#include "stdafx.h"
#include "NF_ReaScript.h"
#include "../SnM/SnM.h"
#include "../SnM/SnM_Chunk.h"
#include "../SnM/SnM_Util.h"

#include "../Misc/Analysis.h" // #781
#include "../Breeder/BR_Loudness.h" // #880
#include "../SnM/SnM_Notes.h" // #755
#include "../SnM/SnM_Project.h" // #974


// #781
double NF_GetMediaItemMaxPeak(MediaItem* item)
{
	double maxPeak = GetMediaItemMaxPeak(item);
	return maxPeak;
}

double NF_GetMediaItemMaxPeakAndMaxPeakPos(MediaItem* item, double* maxPeakPosOut) // #953
{
	double maxPeak = GetMediaItemMaxPeakAndMaxPeakPos(item, maxPeakPosOut);
	return maxPeak;
}

double NF_GetMediaItemPeakRMS_Windowed(MediaItem* item)
{
	double peakRMS = GetMediaItemPeakRMS_Windowed(item);
	return peakRMS;
}

double NF_GetMediaItemPeakRMS_NonWindowed(MediaItem* item)
{
	double peakRMSperChannel = GetMediaItemPeakRMS_NonWindowed(item);
	return peakRMSperChannel;
}

double NF_GetMediaItemAverageRMS(MediaItem* item)
{
	double averageRMS = GetMediaItemAverageRMS(item);
	return averageRMS;

}


// #880
bool NF_AnalyzeTakeLoudness_IntegratedOnly(MediaItem_Take * take, double* lufsIntegratedOut)
{
	return NFDoAnalyzeTakeLoudness_IntegratedOnly(take, lufsIntegratedOut);
}

bool NF_AnalyzeTakeLoudness(MediaItem_Take * take, bool analyzeTruePeak, double* lufsIntegratedOut, double* rangeOut, double* truePeakOut, double* truePeakPosOut, double* shorTermMaxOut, double* momentaryMaxOut)
{
	return NFDoAnalyzeTakeLoudness(take, analyzeTruePeak, lufsIntegratedOut, rangeOut, truePeakOut, truePeakPosOut, shorTermMaxOut, momentaryMaxOut);
}

bool NF_AnalyzeTakeLoudness2(MediaItem_Take * take, bool analyzeTruePeak, double* lufsIntegratedOut, double* rangeOut, double* truePeakOut, double* truePeakPosOut, double* shorTermMaxOut, double* momentaryMaxOut, double* shortTermMaxPosOut, double* momentaryMaxPosOut)
{
	return NFDoAnalyzeTakeLoudness2(take, analyzeTruePeak, lufsIntegratedOut, rangeOut, truePeakOut, truePeakPosOut, shorTermMaxOut, momentaryMaxOut, shortTermMaxPosOut, momentaryMaxPosOut);
}


// #755
const char* NF_GetSWSTrackNotes(MediaTrack* track, WDL_FastString* trackNoteOut)
{
	return NFDoGetSWSTrackNotes(track, trackNoteOut);
}

void NF_SetSWSTrackNotes(MediaTrack* track, const char* buf)
{
	NFDoSetSWSTrackNotes(track, buf);
}

const char* NF_GetSWSMarkerRegionSub(WDL_FastString* regionSubOut, int regionIdx)
{
	return NFDoGetSWSMarkerRegionSub(regionSubOut, regionIdx);
}

bool NF_SetSWSMarkerRegionSub(WDL_FastString* regionSub, int regionIdx)
{
	return NFDoSetSWSMarkerRegionSub(regionSub, regionIdx);
}

void NF_UpdateSWSMarkerRegionSubWindow()
{
	NF_DoUpdateSWSMarkerRegionSubWindow();
}

// #974
/*
extern WDL_FastString g_globalStartupAction; extern SWSProjConfig<WDL_FastString> g_prjLoadActions;

// desc == true: return action text, false: return command ID number (native actions) or named command (extension/ReaScript)
void NF_GetGlobalStartupAction(char* buf, int bufSize, bool desc)
{
	WDL_FastString fs;

	if (!g_globalStartupAction.Get()) 
	{
		if (desc)
			fs.Set("");
		else
			fs.Set("0");

		snprintf(buf, bufSize, "%s", fs.Get());
		return;
	}

	if  (int cmdId = SNM_NamedCommandLookup(g_globalStartupAction.Get())) 
	{
		if (desc)
			fs.Set(kbd_getTextFromCmd(cmdId, NULL));
		else
			fs.Set(g_globalStartupAction.Get());
	}
	else
	{
		if (desc)
			fs.Set("");
		else
			fs.Set("0");
	}
	snprintf(buf, bufSize, "%s", fs.Get());
}

void NF_GetGlobalStartupAction_Desc(char *buf, int bufSize)
{
	NF_GetGlobalStartupAction(buf, bufSize, true);
}
void NF_GetGlobalStartupAction_CmdID(char *buf, int bufSize)
{
	NF_GetGlobalStartupAction(buf, bufSize, false);
}


bool NF_SetGlobalStartupAction(const char * buf)
{
	if (!g_globalStartupAction.Get())
		return false;
		
	if (int cmdId = SNM_NamedCommandLookup(buf))
	{
		// more checks: http://forum.cockos.com/showpost.php?p=1252206&postcount=1618
		if (int tstNum = CheckSwsMacroScriptNumCustomId(buf))
		{
			return false;
		}
		else
		{
			g_globalStartupAction.Set(buf);
			WritePrivateProfileString("Misc", "GlobalStartupAction", buf, g_SNM_IniFn.Get());
			return true;
		}
	}
	else
	{
		return false;
	}
}

bool NF_ClearGlobalStartupAction()
{
	if (g_globalStartupAction.Get()) {
		g_globalStartupAction.Set("");
		WritePrivateProfileString("Misc", "GlobalStartupAction", NULL, g_SNM_IniFn.Get());
		return true;
	}
	return false;
}


void NF_GetProjectStartupAction(char* buf, int bufSize, bool desc)
{
	WDL_FastString fs;
	if (!g_prjLoadActions.Get()->Get())
	{
		if (desc)
			fs.Set("");
		else
			fs.Set("0");

		snprintf(buf, bufSize, "%s", fs.Get());
		return;
	}

	if (int cmdId = SNM_NamedCommandLookup(g_prjLoadActions.Get()->Get()))
	{
		if (desc)
			fs.Set(kbd_getTextFromCmd(cmdId, NULL));
		else
			fs.Set(g_prjLoadActions.Get()->Get());
	}
	else
	{
		if (desc)
			fs.Set("");
		else
			fs.Set("0");
	}
	snprintf(buf, bufSize, "%s", fs.Get());
}

void NF_GetProjectStartupAction_Desc(char *buf, int bufSize)
{
	NF_GetProjectStartupAction(buf, bufSize, true);
}
void NF_GetProjectStartupAction_CmdID(char *buf, int bufSize)
{
	NF_GetProjectStartupAction(buf, bufSize, false);
}

bool NF_SetProjectStartupAction(const char* buf)
{
	if (!g_prjLoadActions.Get())
		return false;

	if (int cmdId = SNM_NamedCommandLookup(buf))
	{
		// more checks: http://forum.cockos.com/showpost.php?p=1252206&postcount=1618
		if (int tstNum = CheckSwsMacroScriptNumCustomId(buf))
		{
			return false;
		}
		else
		{
			g_prjLoadActions.Get()->Set(buf);
			return true;
		}
	}
	else
	{
		return false;
	}
}

bool NF_ClearProjectStartupAction()
{
	if (g_prjLoadActions.Get()) {
		g_prjLoadActions.Get()->Set("");
		return true;
	}
	return false;
}
*/