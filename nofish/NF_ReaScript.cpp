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


// Track/TakeFX_Get/SetOffline
bool NF_TrackFX_GetOffline(MediaTrack* track, int fx)
{
	char state[2] = "0";
	SNM_ChunkParserPatcher p(track);
	p.SetWantsMinimalState(true);
	if (p.Parse(SNM_GET_CHUNK_CHAR, 2, "FXCHAIN", "BYPASS", fx, 2, state) > 0)
		return !strcmp(state, "1");

	return false;
}

void NF_TrackFX_SetOffline(MediaTrack* track, int fx, bool offline)
{
	SNM_ChunkParserPatcher p(track);
	p.SetWantsMinimalState(true);

	bool updt = false;
	if (offline) {
		updt = (p.ParsePatch(SNM_SET_CHUNK_CHAR, 2, "FXCHAIN", "BYPASS", fx, 2, (void*)"1") > 0);

		// chunk BYPASS 3rd value isn't updated when offlining via chunk. So set it if FX is floating when offlining,
		// to match REAPER behaviour
		// https://forum.cockos.com/showpost.php?p=1901608&postcount=12
		if (updt) {
			HWND hwnd = NULL;
			hwnd = TrackFX_GetFloatingWindow(track, fx);

			if (hwnd) // FX is currently floating, set 3rd value
				p.ParsePatch(SNM_SET_CHUNK_CHAR, 2, "FXCHAIN", "BYPASS", fx, 3, (void*)"1");
			else // not floating
				p.ParsePatch(SNM_SET_CHUNK_CHAR, 2, "FXCHAIN", "BYPASS", fx, 3, (void*)"0");
		}

		// close the GUI for buggy plugins (before chunk update)
		// http://github.com/reaper-oss/sws/issues/317
		int supportBuggyPlug = GetPrivateProfileInt("General", "BuggyPlugsSupport", 0, g_SNM_IniFn.Get());
		if (updt && supportBuggyPlug)
			TrackFX_SetOpen(track, fx, false);
	}

	else { // set online
		updt = (p.ParsePatch(SNM_SET_CHUNK_CHAR, 2, "FXCHAIN", "BYPASS", fx, 2, (void*)"0") > 0);

		if (updt) {
			bool committed = false;
			committed = p.Commit(false); // need to commit immediately, otherwise below making FX float wouldn't work 

			if (committed) {
				// check if it should float when onlining
				char state[2] = "0";
				if ((p.Parse(SNM_GET_CHUNK_CHAR, 2, "FXCHAIN", "BYPASS", fx, 3, state) > 0)) {
					if (!strcmp(state, "1"))
						TrackFX_Show(track, fx, 3); // show floating
				}
			}
		}
	}

}

// in chunk take FX Id's are counted consecutively across takes (zero-based)
int FindTakeFXId_inChunk(MediaItem* parentItem, MediaItem_Take* takeIn, int fxIn);

bool NF_TakeFX_GetOffline(MediaItem_Take* takeIn, int fxIn)
{
	// find chunk fx id
	MediaItem* parentItem = GetMediaItemTake_Item(takeIn);
	int takeFXId_inChunk = FindTakeFXId_inChunk(parentItem, takeIn, fxIn);

	if (takeFXId_inChunk >= 0) {
		char state[2] = "0";
		SNM_ChunkParserPatcher p(parentItem);
		p.SetWantsMinimalState(true);
		if (p.Parse(SNM_GET_CHUNK_CHAR, 2, "TAKEFX", "BYPASS", takeFXId_inChunk, 2, state) > 0)
			return !strcmp(state, "1");
	}

	return false;
}

void NF_TakeFX_SetOffline(MediaItem_Take* takeIn, int fxIn, bool offline)
{
	// find chunk fx id
	MediaItem* parentItem = GetMediaItemTake_Item(takeIn);
	int takeFXId_inChunk = FindTakeFXId_inChunk(parentItem, takeIn, fxIn);

	if (takeFXId_inChunk >= 0) {
		SNM_ChunkParserPatcher p(parentItem);
		p.SetWantsMinimalState(true);
		bool updt = false;

		// buggy plugins workaround not necessary for take FX, according to
		// https://github.com/reaper-oss/sws/blob/b3ad441575f075d9232872f7873a6af83bb9de61/SnM/SnM_FX.cpp#L299

		if (offline) {
			updt = (p.ParsePatch(SNM_SET_CHUNK_CHAR, 2, "TAKEFX", "BYPASS", takeFXId_inChunk, 2, (void*)"1") >0);
			if (updt) {
				// check for BYPASS 3rd value
				HWND hwnd = NULL;
				hwnd = TakeFX_GetFloatingWindow(takeIn, fxIn);

				if (hwnd) // FX is currently floating, set 3rd value
					p.ParsePatch(SNM_SET_CHUNK_CHAR, 2, "TAKEFX", "BYPASS", takeFXId_inChunk, 3, (void*)"1");
				else // not floating
					p.ParsePatch(SNM_SET_CHUNK_CHAR, 2, "TAKEFX", "BYPASS", takeFXId_inChunk, 3, (void*)"0");
			}
			Main_OnCommand(41204, 0); // fully unload unloaded VSTs
		}

		else { // set online
			updt = (p.ParsePatch(SNM_SET_CHUNK_CHAR, 2, "TAKEFX", "BYPASS", takeFXId_inChunk, 2, (void*)"0") > 0);

			if (updt) {
				bool committed = false;
				committed = p.Commit(false); // need to commit immediately, otherwise below making FX float wouldn't work 

				if (committed) {
					// check if it should float when onlining
					char state[2] = "0";
					if ((p.Parse(SNM_GET_CHUNK_CHAR, 2, "TAKEFX", "BYPASS", takeFXId_inChunk, 3, state) > 0)) {
						if (!strcmp(state, "1"))
							TakeFX_Show(takeIn, fxIn, 3); // show floating
					}
				}
			}

		}

	}
}

int FindTakeFXId_inChunk(MediaItem* parentItem, MediaItem_Take* takeIn, int fxIn)
{
	int takesCount = CountTakes(parentItem);
	int totalTakeFX = 0; int takeFXId_inChunk = -1;

	for (int i = 0; i < takesCount; i++) {
		MediaItem_Take* curTake = GetTake(parentItem, i);

		int FXcountCurTake = TakeFX_GetCount(curTake);

		if (curTake == takeIn) {
			takeFXId_inChunk = totalTakeFX + fxIn;
			break;
		}

		totalTakeFX += FXcountCurTake;
	}
	return takeFXId_inChunk;
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
extern WDL_FastString g_globalAction; extern SWSProjConfig<WDL_FastString> g_prjActions;;

// desc == true: return action text, false: return command ID number (native actions) or named command (extension/ReaScript)
void NF_GetGlobalStartupAction(char* buf, int bufSize, bool desc)
{
	WDL_FastString fs;

	if (!g_globalAction.Get()) 
	{
		if (desc)
			fs.Set("");
		else
			fs.Set("0");

		snprintf(buf, bufSize, "%s", fs.Get());
		return;
	}

	if  (int cmdId = SNM_NamedCommandLookup(g_globalAction.Get())) 
	{
		if (desc)
			fs.Set(kbd_getTextFromCmd(cmdId, NULL));
		else
			fs.Set(g_globalAction.Get());
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
	if (!g_globalAction.Get())
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
			g_globalAction.Set(buf);
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
	if (g_globalAction.Get()) {
		g_globalAction.Set("");
		WritePrivateProfileString("Misc", "GlobalStartupAction", NULL, g_SNM_IniFn.Get());
		return true;
	}
	return false;
}


void NF_GetProjectStartupAction(char* buf, int bufSize, bool desc)
{
	WDL_FastString fs;
	if (!g_prjActions.Get()->Get())
	{
		if (desc)
			fs.Set("");
		else
			fs.Set("0");

		snprintf(buf, bufSize, "%s", fs.Get());
		return;
	}

	if (int cmdId = SNM_NamedCommandLookup(g_prjActions.Get()->Get()))
	{
		if (desc)
			fs.Set(kbd_getTextFromCmd(cmdId, NULL));
		else
			fs.Set(g_prjActions.Get()->Get());
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
	if (!g_prjActions.Get())
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
			g_prjActions.Get()->Set(buf);
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
	if (g_prjActions.Get()) {
		g_prjActions.Get()->Set("");
		return true;
	}
	return false;
}

/*
sectionIdx, from SnM.h:
SNM_SEC_IDX_MAIN=0,
SNM_SEC_IDX_MAIN_ALT,
SNM_SEC_IDX_EPXLORER,
SNM_SEC_IDX_ME,
SNM_SEC_IDX_ME_EL,
SNM_SEC_IDX_ME_INLINE,
SNM_NUM_MANAGED_SECTIONS
*/
void NF_GetActionDescFromCmdID(const char* cmdID, int secID, char* buf, int buf_sz)
{
	WDL_FastString fs;

	if (secID < 0 || secID > 5) 
	{
		fs.Set("0");
		snprintf(buf, buf_sz, "%s", fs.Get());
		return;
	}

	KbdSectionInfo* kbdSec = SNM_GetActionSection(secID);

	if (kbdSec)
	{
		if (int cmdIDNr = SNM_NamedCommandLookup(cmdID, kbdSec))
			fs.Set(kbd_getTextFromCmd(cmdIDNr, kbdSec));
		else
			fs.Set("0");
	} 
	else
		fs.Set("0");

	snprintf(buf, buf_sz, "%s", fs.Get());
}

