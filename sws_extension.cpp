/******************************************************************************
/ sws_extension.cpp
/
/ Copyright (c) 2012 Tim Payne (SWS), Jeffos
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
#include "Misc/RecCheck.h"
#include "Misc/Adam.h"
#include "Color/Color.h"
#include "Color/Autocolor.h"
#include "MarkerList/MarkerListClass.h"
#include "MarkerList/MarkerList.h"
#include "TrackList/TracklistFilter.h"
#include "TrackList/Tracklist.h"
#include "Projects/ProjectMgr.h"
#include "Projects/ProjectList.h"
#include "SnM/SnM.h"
#include "SnM/SnM_CSurf.h"
#include "SnM/SnM_Dlg.h"
#include "Padre/padreActions.h"
#include "Fingers/FNG_client.h"
#include "Autorender/Autorender.h"
#include "IX/IX.h"
#include "Breeder/BR.h" 

#define LOCALIZE_IMPORT_PREFIX "sws_"
#ifdef LOCALIZE_IMPORT_PREFIX
#include "./reaper/localize-import.h"
#endif
#include "./reaper/localize.h"


// Globals
REAPER_PLUGIN_HINSTANCE g_hInst = NULL;
HWND g_hwndParent = NULL;
reaper_plugin_info_t* g_rec = NULL;

#ifdef ACTION_DEBUG
void freeCmdFilesValue(WDL_String* p) {delete p;}
static WDL_IntKeyedArray<WDL_String*> g_cmdFiles(freeCmdFilesValue);
#endif
static WDL_IntKeyedArray<COMMAND_T*> g_commands; // no valdispose (cmds can be allocated in different ways)

int g_iFirstCommand = 0;
int g_iLastCommand = 0;


bool hookCommandProc(int iCmd, int flag)
{
	static bool bReentrancyCheck = false;
	if (bReentrancyCheck)
		return false;

	// for Xen extensions
	g_KeyUpUndoHandler=0;

	// "Hack" to make actions will #s less than 1000 work with SendMessage (AHK)
	if (iCmd < 1000)
	{
		bReentrancyCheck = true;	
		KBD_OnMainActionEx(iCmd, 0, 0, 0, g_hwndParent, NULL);
		bReentrancyCheck = false;
		return true;
	}

	// Special case for checking recording
	if (iCmd == 1013 && !RecordInputCheck())
		return true;

	// Ignore commands that don't have anything to do with us from this point forward
	if (COMMAND_T* cmd = SWSGetCommandByID(iCmd))
	{
		if (cmd->accel.accel.cmd==iCmd && cmd->doCommand && cmd->doCommand!=SWS_NOOP)
		{
			bReentrancyCheck = true;
			cmd->fakeToggle = !cmd->fakeToggle;
			cmd->doCommand(cmd);
			bReentrancyCheck = false;
			return true;
		}
	}
	return false;
}

// Returns:
// -1 = action does not belong to this extension, or does not toggle
//  0 = action belongs to this extension and is currently set to "off"
//  1 = action belongs to this extension and is currently set to "on"
int toggleActionHook(int iCmd)
{
	static bool bReentrancyCheck = false;
	if (bReentrancyCheck)
		return -1;

	if (COMMAND_T* cmd = SWSGetCommandByID(iCmd))
	{
		if (cmd->accel.accel.cmd==iCmd && cmd->getEnabled && cmd->doCommand!=SWS_NOOP)
		{
			bReentrancyCheck = true;
			int state = cmd->getEnabled(cmd);
			bReentrancyCheck = false;
			return state;
		}
	}
	return -1;
}

// 1) Get command ID from Reaper
// 2) Add keyboard accelerator (with localized action name) and add to the "action" list
int SWSRegisterCmd(COMMAND_T* pCommand, const char* cFile, int cmdId, bool localize)
{
	if (pCommand->doCommand)
	{
		// SWS - Unfortunately can't check for duplicate actions here because when commands are used in the mouse editor
		//   they have a command ID (53000+) assigned before SWS is even loaded.
		// char pId[128];
		// if (_snprintf(pId, 128, "_%s", pCommand->id)<=0 || NamedCommandLookup(pId))
		//	return 0; // duplicated action

		if (!cmdId && !(cmdId = plugin_register("command_id", (void*)pCommand->id)))
			return 0;

		pCommand->accel.accel.cmd = cmdId;

		// localized action name, if needed
		const char* defaultName = pCommand->accel.desc;
		if (localize)
			pCommand->accel.desc = GetLocalizedActionName(pCommand->accel.desc); // no alloc + no-op when no LangPack file is defined
		
		if (!plugin_register("gaccel", &pCommand->accel))
			return 0;

		// now that it is registered, restore the default action name
		if (pCommand->accel.desc != defaultName)
			pCommand->accel.desc = defaultName;

		if (!g_iFirstCommand || g_iFirstCommand > cmdId)
			g_iFirstCommand = cmdId;
		if (cmdId > g_iLastCommand)
			g_iLastCommand = cmdId;

		g_commands.Insert(cmdId, pCommand);
#ifdef ACTION_DEBUG
		g_cmdFiles.Insert(cmdId, new WDL_String(cFile));
#endif
	}
	return pCommand->accel.accel.cmd;
}

// For each item in table call SWSRegisterCommand
int SWSRegisterCmds(COMMAND_T* pCommands, const char* cFile, bool localize)
{
	int i = 0;
	while(pCommands[i].id != LAST_COMMAND)
		SWSRegisterCmd(&pCommands[i++], cFile, 0, localize);
	return 1;
}

// Make and register a dynamic action (created at runtime)
// If cmdId==0, get command ID from Reaper (use the provided cmdId otherwise)
// Note: SWSFreeUnregisterDynamicCmd() can be used to free/unregister such an action
int SWSCreateRegisterDynamicCmd(int cmdId, void (*doCommand)(COMMAND_T*), int (*getEnabled)(COMMAND_T*), const char* cID, const char* cDesc, INT_PTR user, const char* cFile, bool localize)
{
	COMMAND_T* ct = new COMMAND_T;
	memset(ct, 0, sizeof(COMMAND_T));
	ct->accel.desc = _strdup(cDesc);
	ct->id = _strdup(cID);
	ct->doCommand = doCommand;
	ct->getEnabled = getEnabled;
	ct->user = user;
	return SWSRegisterCmd(ct, cFile, cmdId, localize);
}

void SWSFreeUnregisterDynamicCmd(int id)
{
	if (COMMAND_T* ct = SWSUnregisterCmd(id))
	{
		free((void*)ct->accel.desc);
		free((void*)ct->id);
		DELETE_NULL(ct);
	}
}

// Returns the COMMAND_T entry (so it can be freed if necessary)
COMMAND_T* SWSUnregisterCmd(int id)
{
	if (COMMAND_T* ct = g_commands.Get(id, NULL))
	{
		plugin_register("-gaccel", &ct->accel);
		plugin_register("-command_id", &id);
		g_commands.Delete(id);
#ifdef ACTION_DEBUG
		g_cmdFiles.Delete(id);
#endif
		return ct;
	}
	return NULL;
}

#ifdef ACTION_DEBUG
void ActionsList(COMMAND_T*)
{
	// Output sws_actions.csv
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
				if (COMMAND_T* cmd = g_commands.Enumerate(i, NULL, NULL))
				{
					WDL_String* pFn = g_cmdFiles.Get(cmd->accel.accel.cmd, NULL);
					sprintf(cBuf, "\"%s\",%s,%d,_%s\n", cmd->accel.desc, pFn ? pFn->Get() : "", cmd->accel.accel.cmd, cmd->id);
					fputs(cBuf, f);
				}
			}
			fclose(f);
		}
	}
}
#endif

int SWSGetCommandID(void (*cmdFunc)(COMMAND_T*), INT_PTR user, const char** pMenuText)
{
	for (int i=0; i<g_commands.GetSize(); i++)
	{
		if (COMMAND_T* cmd = g_commands.Enumerate(i, NULL, NULL))
		{
			if (cmd->doCommand == cmdFunc && cmd->user == user)
			{
				if (pMenuText)
					*pMenuText = cmd->menuText;
				return cmd->accel.accel.cmd;
			}
		}
	}
	return 0;
}

COMMAND_T* SWSGetCommandByID(int cmdId) {
	if (cmdId >= g_iFirstCommand && cmdId <= g_iLastCommand) // true but not enough to ensure it is a sws action
		return g_commands.Get(cmdId, NULL);
	return NULL;
}

int IsSwsAction(const char* _actionName)
{
	if (_actionName)
		if (const char* p = strstr(_actionName, ": ")) // no strchr() here: make sure p[2] is not out of bounds
			if (const char* tag = strstr(_actionName, "SWS")) // make sure it is a sws tag
				if (tag < p) // make really sure
					return ((int)(p+2-_actionName));
	return 0;
}

HMENU SWSCreateMenuFromCommandTable(COMMAND_T pCommands[], HMENU hMenu, int* iIndex)
{
	if (!hMenu)
		hMenu = CreatePopupMenu();
	int i = 0;
	if (iIndex)
		i = *iIndex;

	while (pCommands[i].id != LAST_COMMAND && pCommands[i].id != SWS_ENDSUBMENU)
	{
		if (const char* name = pCommands[i].menuText)
		{
			if (pCommands[i].id == SWS_STARTSUBMENU)
			{
				i++;
				HMENU hSubMenu = SWSCreateMenuFromCommandTable(pCommands, NULL, &i);
				AddSubMenu(hMenu, hSubMenu, __localizeFunc(name,"sws_menu",0));
			}
			else
				AddToMenu(hMenu, __localizeFunc(name,"sws_menu",0), pCommands[i].accel.accel.cmd);
		}
		i++;
	}
	
	if (iIndex)
		*iIndex = i;
	return hMenu;
}

// This function creates the extension menu (flag==0) and handles checking menu items (flag==1).
// Reaper automatically checks menu items of customized menus using toggleActionHook above,
// but since we can't tell if a menu is customized we always check either way.
static void swsMenuHook(const char* menustr, HMENU hMenu, int flag)
{
	if (flag == 1)
	{
		// Handle checked menu items
		// Go through every menu item - see if it exists in the table, then check if necessary
		MENUITEMINFO mi={sizeof(MENUITEMINFO),};
		mi.fMask = MIIM_ID | MIIM_SUBMENU;
		for (int i = 0; i < GetMenuItemCount(hMenu); i++)
		{
			GetMenuItemInfo(hMenu, i, true, &mi);
			if (mi.hSubMenu)
				swsMenuHook(menustr, mi.hSubMenu, flag);
			else if (mi.wID >= (UINT)g_iFirstCommand && mi.wID <= (UINT)g_iLastCommand) {
				if (COMMAND_T* t = g_commands.Get(mi.wID, NULL))
					CheckMenuItem(hMenu, i, MF_BYPOSITION | (t->getEnabled && t->getEnabled(t) ? MF_CHECKED : MF_UNCHECKED));
			}
		}
	}
	else if (!strcmp(menustr, "Main extensions"))
		SWSCreateExtensionsMenu(hMenu);
}



// Fake control surface to get a low priority periodic time slice from Reaper
// and callbacks for some "track params have changed"
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
		SNM_CSurfRun();
		ZoomSlice();
		MarkerActionSlice();
		ItemPreviewSlice();
		PlayItemsOnceSlice();
		ColorSlice();
		MiscSlice();

		if (m_bChanged)
		{
			m_bChanged = false;
			ScheduleTracklistUpdate();
			g_pMarkerList->Update();
			UpdateSnapshotsDialog();
			ProjectListUpdate();
		}
	}

	void SetPlayState(bool play, bool pause, bool rec)
	{
		SNM_CSurfSetPlayState(play, pause, rec);
		AWDoAutoGroup(rec);
	}

	// This is our only notification of active project tab change, so update everything
	void SetTrackListChange()
	{
		m_bChanged = true;
		AutoColorTrack(false);
		AutoColorMarkerRegion(false);
		SNM_CSurfSetTrackListChange();
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
			AutoColorTrack(false);
			SNM_CSurfSetTrackTitle();
		}
		else
			m_iACIgnore--;
	}

	void SetSurfaceSelected(MediaTrack *tr, bool bSel)	{ ScheduleTracklistUpdate(); UpdateSnapshotsDialog(true); }
	void SetSurfaceMute(MediaTrack *tr, bool mute)		{ ScheduleTracklistUpdate(); UpdateTrackMute(); }
	void SetSurfaceSolo(MediaTrack *tr, bool solo)		{ ScheduleTracklistUpdate(); UpdateTrackSolo(); }
	void SetSurfaceRecArm(MediaTrack *tr, bool arm)		{ ScheduleTracklistUpdate(); UpdateTrackArm(); }
	int Extended(int call, void *parm1, void *parm2, void *parm3) { return SNM_CSurfExtended(call, parm1, parm2, parm3); }
};

// WDL Stuff
bool WDL_STYLE_GetBackgroundGradient(double *gradstart, double *gradslope) { return false; }
bool WDL_STYLE_AllowSliderMouseWheel() { return true; }
LICE_IBitmap *WDL_STYLE_GetSliderBitmap2(bool vert) { return NULL; }
int WDL_STYLE_GetSliderDynamicCenterPos() { return 500; }
int WDL_STYLE_GetSysColor(int i) 
{
	int col = GSC_mainwnd(i); 

	// check & "fix" 3D colors that aren't distinguished in many themes..
#ifdef _WIN32
	if (i == COLOR_3DSHADOW || i == COLOR_3DLIGHT || i == COLOR_3DHILIGHT)
#else
	if (i == COLOR_3DSHADOW || i == COLOR_3DHILIGHT)
#endif
	{
		int col3ds,col3dl,bgcol=GSC_mainwnd(COLOR_WINDOW);
		ColorTheme* ct = SNM_GetColorTheme();
		if (ct)
		{
			col3dl = ct->io_3d[0];
			col3ds = ct->io_3d[1];
			if (i == COLOR_3DSHADOW) col = col3ds;
			else col = col3dl;
		}
		else
		{
			col3dl = GSC_mainwnd(COLOR_3DHILIGHT);
			col3ds = GSC_mainwnd(COLOR_3DSHADOW);
		}

		if (col3ds == col3dl || col3ds == bgcol || col3dl == bgcol)
		{
			int colDelta = SNM_3D_COLORS_DELTA * (i == COLOR_3DSHADOW ? -1 : 1);
			col = RGB(
				BOUNDED(LICE_GETR(bgcol) + colDelta, 0, 0xFF),
				BOUNDED(LICE_GETG(bgcol) + colDelta, 0, 0xFF),
				BOUNDED(LICE_GETB(bgcol) + colDelta, 0, 0xFF));
		}
	}
	return col;
}
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
			g_rec = NULL;
			SnapshotsExit();
			TrackListExit();
			MarkerListExit();
			AutoColorExit();
			ProjectListExit();
			ConsoleExit();
			MiscExit();
			PadreExit();
			SNM_Exit();
			ERR_RETURN("Exiting Reaper.\n")
		}

		if (rec->caller_version != REAPER_PLUGIN_VERSION)
			ERR_RETURN("Wrong REAPER_PLUGIN_VERSION!\n");

		if (!rec->GetFunc)
			ERR_RETURN("Null rec->GetFunc ptr\n")

#ifdef _SWS_LOCALIZATION
		IMPORT_LOCALIZE_RPLUG(rec);
#endif

		int errcnt=0;
		IMPAPI(AddExtensionsMainMenu);
		IMPAPI(AddMediaItemToTrack);
		IMPAPI(AddProjectMarker);
		IMPAPI(AddProjectMarker2);
		IMPAPI(AddTakeToMediaItem);
		IMPAPI(AddTempoTimeSigMarker);
		IMPAPI(adjustZoom);
		IMPAPI(ApplyNudge);
		IMPAPI(AttachWindowTopmostButton);
		IMPAPI(AttachWindowResizeGrip);
		IMPAPI(Audio_RegHardwareHook);
		IMPAPI(CoolSB_GetScrollInfo);
		IMPAPI(CoolSB_SetScrollInfo);
		*(void**)&CountActionShortcuts = rec->GetFunc("CountActionShortcuts");
		IMPAPI(CountMediaItems);
		IMPAPI(CountSelectedMediaItems);
		IMPAPI(CountSelectedTracks);
		IMPAPI(CountTakes);
		IMPAPI(CountTCPFXParms);
		IMPAPI(CountTempoTimeSigMarkers);
		IMPAPI(CountTracks);
		IMPAPI(CountTrackMediaItems);
		IMPAPI(CountTrackEnvelopes);
		IMPAPI(CreateLocalOscHandler);
		IMPAPI(CreateNewMIDIItemInProj);
		IMPAPI(CSurf_FlushUndo);
		IMPAPI(CSurf_GoEnd);
		IMPAPI(CSurf_OnMuteChange);
		IMPAPI(CSurf_OnPanChange);
		IMPAPI(CSurf_OnSelectedChange);
		IMPAPI(CSurf_OnTrackSelection);
		IMPAPI(CSurf_OnVolumeChange);
		IMPAPI(CSurf_TrackFromID);
		IMPAPI(CSurf_TrackToID);
		*(void**)&DeleteActionShortcut = rec->GetFunc("DeleteActionShortcut");
		IMPAPI(DeleteProjectMarker);
		IMPAPI(DeleteTrack);
		IMPAPI(DeleteTrackMediaItem);
		IMPAPI(DestroyLocalOscHandler);
		*(void**)&DoActionShortcutDialog = rec->GetFunc("DoActionShortcutDialog");
		IMPAPI(Dock_UpdateDockID);
		IMPAPI(DockIsChildOfDock);
		IMPAPI(DockWindowActivate);
		IMPAPI(DockWindowAdd);
		IMPAPI(DockWindowAddEx);
		IMPAPI(DockWindowRefresh);
		IMPAPI(DockWindowRemove);
		IMPAPI(EnsureNotCompletelyOffscreen);
		IMPAPI(EnumProjectMarkers);
		IMPAPI(EnumProjectMarkers2);
		IMPAPI(EnumProjectMarkers3);
		IMPAPI(EnumProjects);
		IMPAPI(file_exists);
		IMPAPI(format_timestr);
		IMPAPI(format_timestr_pos);
		IMPAPI(format_timestr_len);	
		IMPAPI(FreeHeapPtr);
		*(void**)&GetActionShortcutDesc = rec->GetFunc("GetActionShortcutDesc");
		IMPAPI(GetActiveTake);
		IMPAPI(GetAppVersion);
		IMPAPI(GetColorThemeStruct);
		IMPAPI(GetContextMenu);
		IMPAPI(GetCurrentProjectInLoadSave);
		IMPAPI(GetCursorContext);
		IMPAPI(GetCursorPosition);
		IMPAPI(GetCursorPositionEx);
		IMPAPI(GetEnvelopeName);
		IMPAPI(GetExePath);
		IMPAPI(GetFocusedFX);
		IMPAPI(GetHZoomLevel);
		IMPAPI(GetIconThemePointer);
		IMPAPI(GetIconThemeStruct);
		IMPAPI(GetInputChannelName);
		IMPAPI(GetItemEditingTime2);
		IMPAPI(GetLastTouchedFX);
		IMPAPI(GetLastTouchedTrack);
		IMPAPI(GetMainHwnd);
		IMPAPI(GetMasterMuteSoloFlags);
		IMPAPI(GetMasterTrack);
		IMPAPI(GetMediaItem);
		IMPAPI(GetMediaItem_Track);
		IMPAPI(GetMediaItemInfo_Value);
		IMPAPI(GetMediaItemNumTakes);
		IMPAPI(GetMediaItemTake);
		IMPAPI(GetMediaItemTakeByGUID);
		IMPAPI(GetMediaItemTake_Item);
		IMPAPI(GetMediaItemTake_Source);
		IMPAPI(GetMediaItemTake_Track);
		IMPAPI(GetMediaItemTakeInfo_Value);
		IMPAPI(GetMediaSourceFileName);
		IMPAPI(GetMediaSourceType);
		IMPAPI(GetMediaTrackInfo_Value);
		IMPAPI(GetNumTracks);
		IMPAPI(GetOutputChannelName);
		IMPAPI(GetPeakFileName);
		IMPAPI(GetPeaksBitmap);
		IMPAPI(GetPlayPosition);
		IMPAPI(GetPlayPosition2);
		IMPAPI(GetPlayPositionEx);
		IMPAPI(GetPlayPosition2Ex);
		IMPAPI(GetPlayState);
		IMPAPI(GetPlayStateEx);
		IMPAPI(GetProjectPath);
		*(void**)&GetProjectStateChangeCount = rec->GetFunc("GetProjectStateChangeCount");
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
		IMPAPI(GetSetObjectState2);
		IMPAPI(GetSetRepeat);
		IMPAPI(GetTempoTimeSigMarker);
		IMPAPI(GetTakeEnvelopeByName);
		IMPAPI(GetSetTrackSendInfo);
		IMPAPI(GetSetTrackState);
		IMPAPI(GetSet_LoopTimeRange);
		IMPAPI(GetSet_LoopTimeRange2);
		IMPAPI(GetSubProjectFromSource);
		IMPAPI(GetTake);
		IMPAPI(GetTCPFXParm);
		IMPAPI(GetToggleCommandState);
		*(void**)&GetToggleCommandState2 = rec->GetFunc("GetToggleCommandState2");
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
		IMPAPI(GR_SelectColor);
		IMPAPI(GSC_mainwnd);
		IMPAPI(guidToString);
		IMPAPI(Help_Set);
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
		IMPAPI(MainThread_LockTracks);
		IMPAPI(MainThread_UnlockTracks);
		IMPAPI(MIDIEditor_GetActive);
		IMPAPI(MIDIEditor_GetMode);
		IMPAPI(MIDIEditor_GetTake);
		IMPAPI(MIDIEditor_LastFocused_OnCommand);
		IMPAPI(MIDIEditor_OnCommand);
		IMPAPI(MIDI_eventlist_Create);
		IMPAPI(MIDI_eventlist_Destroy);
		IMPAPI(mkpanstr);
		IMPAPI(mkvolpanstr);
		IMPAPI(mkvolstr);
		IMPAPI(MoveEditCursor);
		IMPAPI(MoveMediaItemToTrack);
		IMPAPI(OnPlayButton);
		IMPAPI(OnStopButton);
		IMPAPI(NamedCommandLookup);
		IMPAPI(parse_timestr_len);
		IMPAPI(parse_timestr_pos);
		IMPAPI(PCM_Sink_Create);
		IMPAPI(PCM_Source_CreateFromFile);
		IMPAPI(PCM_Source_CreateFromFileEx);
		IMPAPI(PCM_Source_CreateFromSimple);
		IMPAPI(PCM_Source_CreateFromType);
		IMPAPI(PlayPreview);
		IMPAPI(PlayTrackPreview);
		IMPAPI(PlayTrackPreview2Ex);
		IMPAPI(plugin_getFilterList);
		IMPAPI(plugin_register);
		*(void**)&PreventUIRefresh = rec->GetFunc("PreventUIRefresh");
		IMPAPI(projectconfig_var_addr);
		IMPAPI(projectconfig_var_getoffs);
		IMPAPI(RefreshToolbar);
#ifdef _WIN32
		IMPAPI(RemoveXPStyle);
#endif
		IMPAPI(Resampler_Create);
		IMPAPI(screenset_register);
		IMPAPI(screenset_registerNew);
		IMPAPI(screenset_unregister);
		*(void**)&SectionFromUniqueID = rec->GetFunc("SectionFromUniqueID");
		IMPAPI(SelectProjectInstance);
		IMPAPI(SendLocalOscMessage);
		IMPAPI(SetCurrentBPM);
		IMPAPI(SetEditCurPos);
		IMPAPI(SetEditCurPos2);
		IMPAPI(SetMediaItemInfo_Value);
		IMPAPI(SetMediaItemLength);
		IMPAPI(SetMediaItemPosition);
		IMPAPI(SetMediaItemTakeInfo_Value);
		IMPAPI(SetMediaTrackInfo_Value);
		IMPAPI(SetMixerScroll);
		IMPAPI(SetOnlyTrackSelected);
		IMPAPI(SetProjectMarker);
		IMPAPI(SetProjectMarker2);
		IMPAPI(SetProjectMarker3);
		IMPAPI(SetTempoTimeSigMarker);
		IMPAPI(SetTrackSelected);
		IMPAPI(ShowActionList);
		IMPAPI(ShowConsoleMsg);
		IMPAPI(ShowMessageBox);
		IMPAPI(SnapToGrid);
		IMPAPI(SplitMediaItem);
		IMPAPI(StopPreview);
		IMPAPI(StopTrackPreview);
		IMPAPI(stringToGuid);
		IMPAPI(TimeMap_GetDividedBpmAtTime);
		IMPAPI(TimeMap_GetTimeSigAtTime);
		IMPAPI(TimeMap_QNToTime);
		IMPAPI(TimeMap_timeToQN);
		IMPAPI(TimeMap2_beatsToTime);
		IMPAPI(TimeMap2_GetDividedBpmAtTime);
		IMPAPI(TimeMap2_GetNextChangeTime);
		IMPAPI(TimeMap2_QNToTime);
		IMPAPI(TimeMap2_timeToBeats);
		IMPAPI(TimeMap2_timeToQN);
		IMPAPI(TrackFX_FormatParamValue);
		IMPAPI(TrackFX_GetByName);
		IMPAPI(TrackFX_GetChainVisible);
		IMPAPI(TrackFX_GetEnabled);
		IMPAPI(TrackFX_GetFloatingWindow);
		IMPAPI(TrackFX_GetCount);
		IMPAPI(TrackFX_GetFXName);
		IMPAPI(TrackFX_GetFXGUID);
		IMPAPI(TrackFX_GetNumParams);
		IMPAPI(TrackFX_GetOpen);
		IMPAPI(TrackFX_GetParam);
		IMPAPI(TrackFX_GetParamName);
		IMPAPI(TrackFX_GetPreset);
		*(void**)&TrackFX_GetPresetIndex = rec->GetFunc("TrackFX_GetPresetIndex");
		IMPAPI(TrackFX_NavigatePresets);
		IMPAPI(TrackFX_SetEnabled);
		IMPAPI(TrackFX_SetOpen);
		IMPAPI(TrackFX_SetParam);
		IMPAPI(TrackFX_SetPreset);
		*(void**)&TrackFX_SetPresetByIndex = rec->GetFunc("TrackFX_SetPresetByIndex");
		IMPAPI(TrackFX_Show);
		IMPAPI(TrackList_AdjustWindows);
		IMPAPI(Undo_BeginBlock);
		IMPAPI(Undo_BeginBlock2);
		IMPAPI(Undo_CanRedo2);
		IMPAPI(Undo_CanUndo2);
		IMPAPI(Undo_DoUndo2);
		IMPAPI(Undo_EndBlock);
		IMPAPI(Undo_EndBlock2);
		IMPAPI(Undo_OnStateChange);
		IMPAPI(Undo_OnStateChange_Item);
		IMPAPI(Undo_OnStateChange2);
		IMPAPI(Undo_OnStateChangeEx);
		IMPAPI(Undo_OnStateChangeEx2);
		IMPAPI(UpdateArrange);
		IMPAPI(UpdateItemInProject);
		IMPAPI(UpdateTimeline);
		IMPAPI(ValidatePtr);

		g_hInst = hInstance;
		g_hwndParent = GetMainHwnd();
		g_rec = rec;

		if (errcnt)
		{
			char txt[512]="";
			// hacky "fix" for ReaMote that loads the extension
			GetWindowText(rec->hwnd_main, txt, sizeof(txt));
			if (_strnicmp("ReaMote", txt, 7))
			{
				_snprintf(txt, sizeof(txt),
					// keep the message on a single line (for the LangPack generator) 
					__LOCALIZE_VERFMT("The version of SWS extension you have installed is incompatible with your version of REAPER.\nYou probably have a REAPER version less than v%s installed.\nPlease install the latest version of REAPER from www.reaper.fm.","sws_mbox"),
					"4.26"); // <- update compatible version here

				//JFB: NULL parent so that the message is at least visible in taskbars (hidden since REAPER v4 and its "splash 2.0")
				MessageBox(NULL, txt, __LOCALIZE("SWS - Version Incompatibility","sws_mbox"), MB_OK);
			}
			ERR_RETURN("SWS version incompatibility\n")
		}

#ifdef _WIN32
		if (IsLocalized())
			setlocale(LC_ALL, "");
#endif

		if (!rec->Register("hookcommand",(void*)hookCommandProc))
			ERR_RETURN("hook command error\n")

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
		if(!FNGExtensionInit(hInstance, rec))
			ERR_RETURN("Fingers init error\n")
		if (!PadreInit())
			ERR_RETURN("Padre init error\n")
		if (!AboutBoxInit())
			ERR_RETURN("About box init error\n")
		if (!AutorenderInit())
			ERR_RETURN("Autorender init error\n")
		if (!IXInit())
			ERR_RETURN("IX init error\n")
		if (!BreederInit())
			ERR_RETURN("Breeder init error\n")
		if (!SNM_Init(rec)) // keep it as the last init (for cyle actions)
			ERR_RETURN("S&M init error\n")

		if (!rec->Register("hookcustommenu", (void*)swsMenuHook))
			ERR_RETURN("Menu hook error\n")
		AddExtensionsMainMenu();

		if (!RegisterExportedFuncs(rec))
			ERR_RETURN("Reascript export failed\n");
		RegisterExportedAPI(rec); // optional: no test on the returned value

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
