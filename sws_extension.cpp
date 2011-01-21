/******************************************************************************
/ sws_extension.cpp
/
/ Copyright (c) 2010 Tim Payne (SWS)
/ http://www.standingwaterstudios.com/reaper
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
#include "Console/Console.h"
#include "Freeze/Freeze.h"
#include "MarkerActions/MarkerActions.h"
#include "Snapshots/SnapshotClass.h"
#include "Snapshots/Snapshots.h"
#include "Zoom.h"
#include "Misc/Misc.h"
#include "Color/Color.h"
#include "Color/Autocolor.h"
#include "MarkerList/MarkerListClass.h"
#include "MarkerList/MarkerList.h"
#include "TrackList/TrackListFilter.h"
#include "TrackList/Tracklist.h"
#include "MediaPool/MediaPool.h"
#include "Projects/ProjectMgr.h"
#include "Projects/ProjectList.h"
#include "SnM/SnM_Actions.h"
#include "Padre/padreActions.h"
#include "Autorender/Autorender.h"

// Globals
REAPER_PLUGIN_HINSTANCE g_hInst = NULL;
HWND g_hwndParent = NULL;
static WDL_PtrList<COMMAND_T> g_commands;
static WDL_PtrList<WDL_String> g_cmdFile;
// Not strictly necessary but saves going through the whole
// command array when asking about toggle state
static WDL_PtrList<COMMAND_T> g_toggles;
int g_iFirstCommand = 0;
int g_iLastCommand = 0;
bool g_bv4 = false;

bool hookCommandProc(int command, int flag)
{
	static bool bReentrancyCheck = false;
	if (bReentrancyCheck)
		return false;

	// for Xen extensions
	g_KeyUpUndoHandler=0;

	// "Hack" to make actions will #s less than 1000 work with SendMessage (AHK)
	if (command < 1000)
	{
		bReentrancyCheck = true;	
		KBD_OnMainActionEx(command, 0, 0, 0, g_hwndParent, NULL);
		bReentrancyCheck = false;
		return true;
	}

	// Ignore commands that don't have anything to do with us from this point forward
	if (command < g_iFirstCommand || command > g_iLastCommand)
		return false;

	for (int i = 0; i < g_commands.GetSize(); i++)
	{
		if (g_commands.Get(i)->accel.accel.cmd && command == g_commands.Get(i)->accel.accel.cmd)
		{
			bReentrancyCheck = true;
			g_commands.Get(i)->doCommand(g_commands.Get(i));
			bReentrancyCheck = false;
			return true;
		}
	}
	return false;
}

// 1) Get command ID from Reaper
// 2) Add keyboard accelerator and add to the "action" list
int SWSRegisterCommand2(COMMAND_T* pCommand, const char* cFile)
{
	if (pCommand->doCommand)
	{
		if (!(pCommand->accel.accel.cmd = plugin_register("command_id", (void*)pCommand->id)))
			return 0;
		if (!plugin_register("gaccel",&pCommand->accel))
			return 0;
		if (pCommand->getEnabled)
			g_toggles.Add(pCommand);
		if (!g_iFirstCommand && g_iFirstCommand > pCommand->accel.accel.cmd)
			g_iFirstCommand = pCommand->accel.accel.cmd;
		if (pCommand->accel.accel.cmd > g_iLastCommand)
			g_iLastCommand = pCommand->accel.accel.cmd;
		g_commands.Add(pCommand);
		g_cmdFile.Add(new WDL_String(cFile));
	}
	return 1;
}

// For each item in table call SWSRegisterCommand
int SWSRegisterCommands2(COMMAND_T* pCommands, const char* cFile)
{
	// Register our commands from table
	int i = 0;
	while(pCommands[i].id != LAST_COMMAND)
	{
		SWSRegisterCommand2(&pCommands[i], cFile);
		i++;
	}
	return 1;
}

// Returns the COMMAND_T entry so it can be deleted if necessary
COMMAND_T* SWSUnregisterCommand(int id)
{
	// First check the toggle command list
	for (int i = 0; i < g_toggles.GetSize(); i++)
		if (g_toggles.Get(i)->accel.accel.cmd == id)
			g_toggles.Delete(i--);

	for (int i = 0; i < g_commands.GetSize(); i++)
	{
		if (g_commands.Get(i)->accel.accel.cmd == id)
		{
			COMMAND_T* cmd = g_commands.Get(i);
			plugin_register("-gaccel", &cmd->accel);
			//plugin_register("-command_id", cmd->id); // Appears to be unnecessary
			g_commands.Delete(i);
			g_cmdFile.Delete(i);
			return cmd;
		}
	}
	return NULL;
}

void ActionsList(COMMAND_T*)
{
	// Load files from the "database"
	char cBuf[512];
	strncpy(cBuf, get_ini_file(), 256);
	char* pC = strrchr(cBuf, PATH_SLASH_CHAR);
	if (pC)
	{
		strcpy(pC+1, "sws_actions.csv");
		FILE* f = fopenUTF8(cBuf, "w");
		fputs("Action,File,CmdID,CmdStr\n", f);
		if (f)
		{
			for (int i = 0; i < g_commands.GetSize(); i++)
			{
				COMMAND_T* cmd = g_commands.Get(i);
				sprintf(cBuf, "\"%s\",%s,%d,%s\n", cmd->accel.desc, g_cmdFile.Get(i)->Get(), cmd->accel.accel.cmd, cmd->id);
				fputs(cBuf, f);
			}
			fclose(f);
		}
	}
}

int SWSGetCommandID(void (*cmdFunc)(COMMAND_T*), INT_PTR user, const char** pMenuText)
{
	for (int i = 0; i < g_commands.GetSize(); i++)
	{
		if (g_commands.Get(i)->doCommand == cmdFunc && g_commands.Get(i)->user == user)
		{
			if (pMenuText)
				*pMenuText = g_commands.Get(i)->menuText;
			return g_commands.Get(i)->accel.accel.cmd;
		}
	}
	return 0;
}

HMENU SWSCreateMenu(COMMAND_T pCommands[], HMENU hMenu, int* iIndex)
{
	// Add menu items
	if (!hMenu)
		hMenu = CreatePopupMenu();
	int i = 0;
	if (iIndex)
		i = *iIndex;

	while (pCommands[i].id != LAST_COMMAND && pCommands[i].id != SWS_ENDSUBMENU)
	{
		if (pCommands[i].id == SWS_STARTSUBMENU)
		{
			const char* subMenuName = pCommands[i].menuText;
			i++;
			HMENU hSubMenu = SWSCreateMenu(pCommands, NULL, &i);
			AddSubMenu(hMenu, hSubMenu, subMenuName);
		}
		else
			AddToMenu(hMenu, pCommands[i].menuText, pCommands[i].accel.accel.cmd);

		i++;
	}
	
	if (iIndex)
		*iIndex = i;
	return hMenu;
}

int SWSGetMenuPosFromID(HMENU hMenu, UINT id)
{	// Replacement for deprecated windows func GetMenuPosFromID
	MENUITEMINFO mi={sizeof(MENUITEMINFO),};
	mi.fMask = MIIM_ID;
	for (int i = 0; i < GetMenuItemCount(hMenu); i++)
	{
		GetMenuItemInfo(hMenu, i, true, &mi);
		if (mi.wID == id)
			return i;
	}
	return -1;
}

// returns:
// -1 = action does not belong to this extension, or does not toggle
//  0 = action belongs to this extension and is currently set to "off"
//  1 = action belongs to this extension and is currently set to "on"
int toggleActionHook(int iCmd)
{
	for (int i = 0; i < g_toggles.GetSize(); i++)
		if (g_toggles.Get(i)->accel.accel.cmd == iCmd)
			return g_toggles.Get(i)->getEnabled(g_toggles.Get(i)) ? 1 : 0;
	return -1;
}

// This function handles checking menu items.
// Reaper automatically checks menu items of customized menus using toggleActionHook above,
// but since we can't tell if a menu is customized we always check either way.
static void toggleMenuHook(const char* menustr, HMENU hMenu, int flag)
{
	// Handle checked menu items
	if (flag == 1)
	{
		// Go through every menu item - see if it exists in the table, then check if necessary
		MENUITEMINFO mi={sizeof(MENUITEMINFO),};
		mi.fMask = MIIM_ID | MIIM_SUBMENU;
		for (int i = 0; i < GetMenuItemCount(hMenu); i++)
		{
			GetMenuItemInfo(hMenu, i, true, &mi);
			if (mi.hSubMenu)
				toggleMenuHook(menustr, mi.hSubMenu, flag);
			else if (mi.wID >= (UINT)g_iFirstCommand && mi.wID <= (UINT)g_iLastCommand)
				for (int j = 0; j < g_toggles.GetSize(); j++)
				{
					COMMAND_T* t = g_toggles.Get(j);
					if (t->accel.accel.cmd == mi.wID)
						CheckMenuItem(hMenu, i, MF_BYPOSITION | (t->getEnabled(t) ? MF_CHECKED : MF_UNCHECKED));
				}
		}
	}
}

// Fake control surface just to get a low priority periodic time slice from Reaper
class SWSTimeSlice : public IReaperControlSurface
{
public:
	const char *GetTypeString() { return ""; }
	const char *GetDescString() { return ""; }
	const char *GetConfigString() { return ""; }

	bool m_bChanged;
	int m_iACIgnore;
	SWSTimeSlice() : m_bChanged(false), m_iACIgnore(0) {}

	void Run()
	{
		ZoomSlice();
		MarkerActionSlice();
		ItemPreviewSlice();
		PlayItemsOnceSlice();
		ColorSlice();
		MiscSlice();

		SnMCSurfRun();

		if (m_bChanged)
		{
			m_bChanged = false;
			ScheduleTracklistUpdate();
			g_pMarkerList->Update();
			UpdateSnapshotsDialog();
			MediaPoolUpdate();
			ProjectListUpdate();
		}
	}

	// This is our only notification of active project tab change, so update everything
	void SetTrackListChange()
	{
		m_bChanged = true;
		AutoColorRun(false);
		SnMCSurfSetTrackListChange();
		m_iACIgnore = GetNumTracks() + 1;
	}
	// For every SetTrackListChange we get NumTracks+1 SetTrackTitle calls, but we only
	// want to call AutoColorRun once, so ignore those n+1.
	// However, we still need to trap track name changes with no track list change.
	void SetTrackTitle(MediaTrack *tr, const char *c)
	{
		ScheduleTracklistUpdate();
		if (!m_iACIgnore)
		{
			AutoColorRun(false);
			SnMCSurfSetTrackTitle();
		}
		else
			m_iACIgnore--;
	}

	// The rest only are applicable only to the TrackList
	void SetSurfaceSelected(MediaTrack *tr, bool bSel)	{ ScheduleTracklistUpdate(); }
	void SetSurfaceMute(MediaTrack *tr, bool mute)		{ ScheduleTracklistUpdate(); }
	void SetSurfaceSolo(MediaTrack *tr, bool solo)		{ ScheduleTracklistUpdate(); }
	void SetSurfaceRecArm(MediaTrack *tr, bool arm)		{ ScheduleTracklistUpdate(); }
};

// WDL Stuff
bool WDL_STYLE_GetBackgroundGradient(double *gradstart, double *gradslope) { return false; }
int WDL_STYLE_GetSysColor(int i) { if (GSC_mainwnd) return GSC_mainwnd(i); else return GetSysColor(i); }
int WDL_STYLE_WantGlobalButtonBorders() { return 0; }
bool WDL_STYLE_WantGlobalButtonBackground(int *col) { return false; }
void WDL_STYLE_ScaleImageCoords(int *x, int *y) { }

// Main DLL entry point
#define ERR_RETURN(a) { return 0; }
#define OK_RETURN(a)  { return 1; }
//#define ERR_RETURN(a) { FILE* f = fopen("c:\\swserror.txt", "a"); if (f) { fprintf(f, a); fclose(f); } return 0; }
//#define OK_RETURN(a)  { FILE* f = fopen("c:\\swserror.txt", "a"); if (f) { fprintf(f, a); fclose(f); } return 1; }
extern "C"
{
	REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t *rec)
	{
		if (!rec)
		{
			SnapshotsExit();
			TrackListExit();
			MarkerListExit();
			MediaPoolExit();
			AutoColorExit();
			ProjectListExit();
			MiscExit();
			PadreExit();
			SnMExit();
			ERR_RETURN("Exiting Reaper.\n")
		}
		if (rec->caller_version != REAPER_PLUGIN_VERSION)
		{
			ERR_RETURN("Wrong REAPER_PLUGIN_VERSION!\n");
		}
		if (!rec->GetFunc)
		{
			ERR_RETURN("Null rec->GetFunc ptr\n")
		}

		int errcnt=0;
		IMPAPI(AddExtensionsMainMenu);
		IMPAPI(AddMediaItemToTrack);
		IMPAPI(AddProjectMarker);
		IMPAPI(AddTakeToMediaItem);
		IMPAPI(adjustZoom);
		IMPAPI(Audio_RegHardwareHook);
		IMPAPI(CoolSB_GetScrollInfo);
		IMPAPI(CoolSB_SetScrollInfo);
		IMPAPI(CountMediaItems);
		IMPAPI(CountSelectedMediaItems);
		IMPAPI(CountSelectedTracks);
		IMPAPI(CountTakes);
		IMPAPI(CountTracks);
		IMPAPI(CountTrackMediaItems);
		IMPAPI(CountTrackEnvelopes);
		IMPAPI(CSurf_FlushUndo);
		IMPAPI(CSurf_GoEnd);
		IMPAPI(CSurf_OnMuteChange);
		IMPAPI(CSurf_OnPanChange);
		IMPAPI(CSurf_OnSelectedChange);
		IMPAPI(CSurf_OnTrackSelection);
		IMPAPI(CSurf_OnVolumeChange);
		IMPAPI(CSurf_TrackFromID);
		IMPAPI(CSurf_TrackToID);
		IMPAPI(DeleteProjectMarker);
		IMPAPI(DeleteTrack);
		IMPAPI(DeleteTrackMediaItem);
		IMPAPI(DockWindowActivate);
		IMPAPI(DockWindowAdd);
		*(void**)&DockWindowAddEx = rec->GetFunc("DockWindowAddEx"); // v4 only
		IMPAPI(DockWindowRefresh);
		IMPAPI(DockWindowRemove);
		IMPAPI(EnsureNotCompletelyOffscreen);
		IMPAPI(EnumProjectMarkers);
		IMPAPI(EnumProjects);
		IMPAPI(format_timestr);
		IMPAPI(format_timestr_pos);
		IMPAPI(FreeHeapPtr);
		IMPAPI(GetActiveTake)
		IMPAPI(GetColorThemeStruct);
		IMPAPI(GetContextMenu);
		IMPAPI(GetCursorContext);
		IMPAPI(GetCursorPosition);
		IMPAPI(GetCursorPositionEx);
		IMPAPI(GetIconThemeStruct);
		IMPAPI(GetItemEditingTime2);
		IMPAPI(GetEnvelopeName);
		IMPAPI(GetExePath);
		IMPAPI(GetHZoomLevel);
		IMPAPI(GetInputChannelName);
		IMPAPI(GetLastTouchedTrack);
		IMPAPI(GetMainHwnd);
		IMPAPI(GetMasterTrack);
		IMPAPI(GetMediaItem);
		IMPAPI(GetMediaItem_Track);
		IMPAPI(GetMediaItemInfo_Value);
		IMPAPI(GetMediaItemNumTakes);
		IMPAPI(GetMediaItemTake);
		IMPAPI(GetMediaItemTake_Item);
		IMPAPI(GetMediaItemTake_Source);
		IMPAPI(GetMediaItemTake_Track);
		IMPAPI(GetMediaItemTakeInfo_Value);
		IMPAPI(GetNumTracks);
		IMPAPI(GetOutputChannelName);
		IMPAPI(GetPeaksBitmap);
		IMPAPI(GetPlayPosition);
		IMPAPI(GetPlayPosition2);
		IMPAPI(GetPlayPositionEx);
		IMPAPI(GetPlayPosition2Ex);
		IMPAPI(GetPlayState);
		IMPAPI(GetProjectPath);
		IMPAPI(GetProjectTimeSignature2);
		IMPAPI(GetResourcePath);
		IMPAPI(GetSelectedMediaItem);
		IMPAPI(GetSelectedTrack);
		IMPAPI(GetSelectedTrackEnvelope);
		IMPAPI(GetSet_ArrangeView2);
		IMPAPI(GetSetEnvelopeState);
		IMPAPI(GetSetMediaItemInfo);
		IMPAPI(GetSetMediaItemTakeInfo);
		IMPAPI(GetSetMediaTrackInfo);
		IMPAPI(GetSetObjectState);
		IMPAPI(GetSetRepeat);
		IMPAPI(GetTakeEnvelopeByName);
		IMPAPI(GetSetTrackSendInfo);
		IMPAPI(GetSetTrackState);
		IMPAPI(GetSet_LoopTimeRange);
		IMPAPI(GetSet_LoopTimeRange2);
		IMPAPI(GetTake);
		IMPAPI(GetTrack);
		IMPAPI(GetTrackGUID);
		IMPAPI(GetTrackEnvelope);
		IMPAPI(GetTrackEnvelopeByName);
		IMPAPI(GetTrackInfo);
		IMPAPI(GetTrackMediaItem);
		IMPAPI(GetTrackNumMediaItems);
		IMPAPI(GetTrackUIVolPan);
		IMPAPI(GetUserInputs);
		IMPAPI(get_config_var);
		IMPAPI(get_ini_file);
		IMPAPI(GSC_mainwnd);
		IMPAPI(guidToString);
//		IMPAPI(Help_Set);
		IMPAPI(InsertMedia);
		IMPAPI(InsertTrackAtIndex);
		IMPAPI(IsMediaExtension);
		IMPAPI(kbd_enumerateActions);
		IMPAPI(kbd_formatKeyName); 
		IMPAPI(kbd_getCommandName);
		IMPAPI(kbd_getTextFromCmd);
		IMPAPI(KBD_OnMainActionEx);
		IMPAPI(kbd_reprocessMenu);
		IMPAPI(kbd_RunCommandThroughHooks);
		IMPAPI(kbd_translateAccelerator);
		IMPAPI(Main_OnCommand);
		IMPAPI(Main_OnCommandEx);
		IMPAPI(Main_openProject);
		IMPAPI(MIDIEditor_GetActive);
		IMPAPI(MIDIEditor_GetMode);
		IMPAPI(MIDIEditor_GetTake);
		IMPAPI(MIDIEditor_OnCommand);
		IMPAPI(MIDI_eventlist_Create);
		IMPAPI(MIDI_eventlist_Destroy);
		IMPAPI(mkpanstr);
		IMPAPI(mkvolpanstr);
		IMPAPI(mkvolstr);
		IMPAPI(MoveEditCursor);
		IMPAPI(MoveMediaItemToTrack);
		IMPAPI(NamedCommandLookup);
		IMPAPI(parse_timestr_pos);
		IMPAPI(PCM_Sink_Create);
		IMPAPI(PCM_Source_CreateFromFile);
		IMPAPI(PCM_Source_CreateFromSimple);
		IMPAPI(PCM_Source_CreateFromType);
		IMPAPI(PlayPreview);
		IMPAPI(PlayTrackPreview);
		IMPAPI(plugin_getFilterList);
		IMPAPI(plugin_register);
		IMPAPI(projectconfig_var_addr);
		IMPAPI(projectconfig_var_getoffs);
		IMPAPI(RefreshToolbar);
		IMPAPI(Resampler_Create);
		IMPAPI(screenset_register);
		IMPAPI(ShowConsoleMsg);
		IMPAPI(SelectProjectInstance);
		IMPAPI(SetEditCurPos);
		IMPAPI(SetEditCurPos2);
		IMPAPI(SetMediaItemInfo_Value);
		IMPAPI(SetMediaItemTakeInfo_Value);
		IMPAPI(SetProjectMarker);
		IMPAPI(SetTrackSelected);
		IMPAPI(ShowActionList);
		IMPAPI(SplitMediaItem);
		IMPAPI(StopPreview);
		IMPAPI(StopTrackPreview);
		IMPAPI(stringToGuid);
		IMPAPI(TimeMap_GetDividedBpmAtTime);
		IMPAPI(TimeMap_QNToTime);
		IMPAPI(TimeMap_timeToQN);
		IMPAPI(TimeMap2_beatsToTime);
		IMPAPI(TimeMap2_QNToTime);
		IMPAPI(TimeMap2_timeToBeats);
		IMPAPI(TrackFX_FormatParamValue);
		IMPAPI(TrackFX_GetChainVisible);
		IMPAPI(TrackFX_GetFloatingWindow);
		IMPAPI(TrackFX_GetCount);
		IMPAPI(TrackFX_GetFXName);
		IMPAPI(TrackFX_GetNumParams);
		IMPAPI(TrackFX_GetParam);
		IMPAPI(TrackFX_GetParamName);
		IMPAPI(TrackFX_SetParam);
		IMPAPI(TrackFX_Show);
		IMPAPI(TrackList_AdjustWindows);
		IMPAPI(Undo_BeginBlock);
		IMPAPI(Undo_BeginBlock2);
		IMPAPI(Undo_EndBlock);
		IMPAPI(Undo_EndBlock2);
		IMPAPI(Undo_OnStateChange);
		IMPAPI(Undo_OnStateChange_Item);
		IMPAPI(Undo_OnStateChange2);
		IMPAPI(Undo_OnStateChangeEx);
		IMPAPI(UpdateArrange);
		IMPAPI(UpdateItemInProject);
		IMPAPI(UpdateTimeline);
		IMPAPI(ValidatePtr);


		g_hInst = hInstance;
		g_hwndParent = GetMainHwnd();

		// TODO remove when v4 only:
		g_bv4 = DockWindowAddEx != NULL;

		if (errcnt)
		{
			MessageBox(g_hwndParent, "The version of SWS extension you have installed is incompatible with your version of Reaper.  You probably have a Reaper version less than 3.66 installed. "
				"Please install the latest version of Reaper from www.reaper.fm.", "Version Incompatibility", MB_OK);
			return 0;
		}

		if (!rec->Register("hookcommand",(void*)hookCommandProc))
			ERR_RETURN("hook command error\n")
		if (!rec->Register("hookcustommenu", (void*)toggleMenuHook))
			ERR_RETURN("Menu hook error\n")
		if (!rec->Register("toggleaction", (void*)toggleActionHook))
			ERR_RETURN("Toggle action hook error\n")

		// Call plugin specific init
		if (!AutoColorInit())
			ERR_RETURN("Auto Color init error\n")
		if (!ColorInit())
			ERR_RETURN("Color init error\n")
		if (!MarkerListInit())
			ERR_RETURN("Marker list init error\n")
		if (!MarkerActionsInit())
			ERR_RETURN("Marker action init error\n")
		if (!MediaPoolInit())
			ERR_RETURN("Mediapool init error\n")
		if (!ConsoleInit())
			ERR_RETURN("ReaConsole init error\n")
		if (!FreezeInit())
			ERR_RETURN("Freeze init error\n")
		if (!SnapshotsInit())
			ERR_RETURN("Snapshots init error\n")
		if (!TrackListInit())
			ERR_RETURN("Tracklist init error\n")
		if (!ProjectListInit())
			ERR_RETURN("Project List init error\n")
		if (!ProjectMgrInit())
			ERR_RETURN("Project Mgr init error\n")
		if (!XenakiosInit())
			ERR_RETURN("Xenakios init error\n")
		if (!MiscInit())
			ERR_RETURN("Misc init error\n")
		if (!SnMInit(rec))
			ERR_RETURN("SnM init error\n")
		if (!AboutBoxInit())
			ERR_RETURN("About box init error\n")
		if (!PadreInit())
			ERR_RETURN("Padre init error\n")
		if (!AutorenderInit())
			ERR_RETURN("Autorender init error\n")

		SWSTimeSlice* ts = new SWSTimeSlice();
		if (!rec->Register("csurf_inst", ts))
		{
			delete ts;
			ERR_RETURN("TimeSlice init error\n")
		}

		OK_RETURN("SWS Extension successfully loaded.\n");
	}
};   // end extern C

#ifndef _WIN32 // MAC resources
#include "../WDL/swell/swell-dlggen.h"
#include "sws_extension.rc_mac_dlg"
#undef BEGIN
#undef END
#include "../WDL/swell/swell-menugen.h"
#include "sws_extension.rc_mac_menu"
#endif
