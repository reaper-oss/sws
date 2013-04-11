/******************************************************************************
/ sws_util.h
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

#pragma once

#define IMPAPI(x)				if (!((*((void **)&(x)) = (void *)rec->GetFunc(#x)))) errcnt++;
#define IMPVAR(x,nm)			if (!((*(void **)&(x)) = get_config_var(nm,&sztmp)) || sztmp != sizeof(*x)) errcnt++;

// Use this macro to include the ReaProject* cast.  Try to get cockos to fix in the gen header
#define Enum_Projects(idx, name, namelen) ((ReaProject*)EnumProjects(idx, name, namelen))

#define BUFFER_SIZE				2048
#define SWS_THEMING				true
#define SWS_INI					"SWS"
#define SWS_SEPARATOR			"SEPARATOR"
#define LAST_COMMAND			((char*)(INT_PTR)-1)
#define SWS_STARTSUBMENU		((char*)(INT_PTR)-2)
#define SWS_ENDSUBMENU			((char*)(INT_PTR)-3)
#define MINTRACKHEIGHT			24
#define DEFACCEL				{ 0, 0, 0 }

#define UTF8_BULLET				"\xE2\x80\xA2"
#define UTF8_CIRCLE				"\xE2\x97\xA6"
#define UTF8_BOX				"\xE2\x96\xA1"
#define UTF8_BBOX				"\xE2\x96\xA0"
#define UTF8_INFINITY			"\xE2\x88\x9E"

// +IsSwsAction() to skip "SWS: ", "SWS/S&M: ", "SWS/FNG: ", etc...
#define SWS_CMD_SHORTNAME(_ct)	(GetLocalizedActionName(_ct->accel.desc) + IsSwsAction(_ct->accel.desc))
#define __ARRAY_SIZE(x)			(sizeof(x) / sizeof(x[0]))
#define BOUNDED(x,lo,hi)		((x) < (lo) ? (lo) : (x) > (hi) ? (hi) : (x))
#define FREE_NULL(p)			{free(p);p=0;}
#define DELETE_NULL(p)			{delete(p); p=0;}
#define STR_HELPER(x)			#x
#define STR(x)					STR_HELPER(x)

// For checking to see if items are adjacent
// Found one case of items after split having diff edges 5e-11 apart, 1e-9 (still much greater than one sample)
#define SWS_ADJACENT_ITEM_THRESHOLD 1.0e-9

#ifdef _WIN32
#define PATH_SLASH_CHAR '\\'
#else
#define PATH_SLASH_CHAR '/'
#endif

// Aliases to keys
#define SWS_ALT		LVKF_ALT		// 1
#define SWS_CTRL	LVKF_CONTROL	// 2
#define SWS_SHIFT	LVKF_SHIFT		// 4

typedef struct MODIFIER
{
	int iModifier;
	const char* cDesc;
} MODIFIER;

#define NUM_MODIFIERS 8
extern MODIFIER g_modifiers[]; // sws_util.h

typedef struct COMMAND_T
{
	gaccel_register_t accel;
	const char* id;
	void (*doCommand)(COMMAND_T*);
	const char* menuText;
	INT_PTR user;
	int (*getEnabled)(COMMAND_T*);
	bool fakeToggle;
} COMMAND_T;

typedef void (*SWS_COMMANDFUNC)(COMMAND_T*);
#define SWS_NOOP ((SWS_COMMANDFUNC)-1)

template<class PTRTYPE> class SWSProjConfig
{
protected:
	WDL_PtrList<void> m_projects;
	WDL_PtrList<PTRTYPE> m_data;

public:
	SWSProjConfig() {}
	virtual ~SWSProjConfig() { Empty(); }
	PTRTYPE* Get()
	{
		ReaProject* pProj = (ReaProject*)GetCurrentProjectInLoadSave();
		if (!pProj) // If not in a project load/save context, above returns NULL
			pProj = Enum_Projects(-1, NULL, 0);
		int i = m_projects.Find(pProj);
		if (i >= 0)
			return m_data.Get(i);
		m_projects.Add(pProj);
		return m_data.Add(new PTRTYPE);
	}
	PTRTYPE* Get(int iProj) { return m_data.Get(iProj); }
	int GetNumProj() { return m_data.GetSize(); }
	void Empty()
	{
		m_projects.Empty(false);
		m_data.Empty(true); 
	}
	void Cleanup()
	{
		if (m_projects.GetSize())
		{
			for (int i = m_projects.GetSize() - 1; i >= 0; i--)
			{
				int j = 0;
				ReaProject* pProj;
				while ((pProj = Enum_Projects(j++, NULL, 0)))
					if (m_projects.Get(i) == pProj)
						break;
				if (!pProj)
				{
					m_projects.Delete(i, false);
					m_data.Delete(i, true);
				}
			}
		}
	}
};

extern REAPER_PLUGIN_HINSTANCE g_hInst;
extern HWND g_hwndParent;
extern reaper_plugin_info_t* g_rec;
extern double g_d0;
extern int g_i0;
extern int g_i1;
extern int g_i2;
extern bool g_bTrue;
extern bool g_bFalse;
extern int g_iFirstCommand;
extern int g_iLastCommand;

// Stuff to do in swell someday
#ifndef _WIN32
#define BS_MULTILINE 0
#define _strdup strdup
#define _strndup strndup
#define _snprintf snprintf
#define _stricmp stricmp
#define _strnicmp strnicmp
#define LVKF_ALT 1
#define LVKF_CONTROL 2
#define LVKF_SHIFT 4
extern const GUID GUID_NULL;
//sws_util.mm
void SWS_GetDateString(int time, char* buf, int bufsize);
void SWS_GetTimeString(int time, char* buf, int bufsize);
void SetColumnArrows(HWND h, int iSortCol);
int GetCustomColors(COLORREF custColors[]);
void SetCustomColors(COLORREF custColors[]);
void ShowColorChooser(COLORREF initialCol);
bool GetChosenColor(COLORREF* pColor);
void HideColorChooser();
void EnableColumnResize(HWND h);
HCURSOR SWS_LoadCursor(int id);
#define MOUSEEVENTF_LEFTDOWN    0x0002 /* left button down */
#define MOUSEEVENTF_LEFTUP      0x0004 /* left button up */
#define MOUSEEVENTF_RIGHTDOWN   0x0008 /* right button down */
#define MOUSEEVENTF_RIGHTUP     0x0010 /* right button up */
void mouse_event(DWORD dwFlags, DWORD dx, DWORD dy, DWORD dwData, ULONG_PTR dwExtraInfo);
int ListView_GetSelectedCountCast(HWND h);
int ListView_GetItemCountCast(HWND h);
bool ListView_GetItemCast(HWND h, LVITEM *item);
void ListView_GetItemTextCast(HWND hwnd, int item, int subitem, char *text, int textmax);
void ListView_RedrawItemsCast(HWND h, int startitem, int enditem);
bool ListView_SetItemStateCast(HWND h, int ipos, int state, int statemask);
BOOL IsWindowEnabled(HWND hwnd);
int GetMenuString(HMENU hMenu, UINT uIDItem, char* lpString, int nMaxCount, UINT uFlag);
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

// Command/action handling, sws_extension.cpp
int SWSRegisterCmd(COMMAND_T* pCommand, const char* cFile, int cmdId = 0, bool localize = true); // One command
COMMAND_T* SWSUnregisterCmd(int id);

int SWSRegisterCmds(COMMAND_T* pCommands, const char* cFile, bool localize); // Multiple commands in a table, terminated with LAST_COMMAND
#define SWSRegisterCommands(c) SWSRegisterCmds(c, __FILE__, true)

int SWSCreateRegisterDynamicCmd(int cmdId, void (*doCommand)(COMMAND_T*), int (*getEnabled)(COMMAND_T*), const char* cID, const char* cDesc, INT_PTR user, const char* cFile, bool localize);
#define SWSRegisterCommandExt(a, b, c, d, e) SWSCreateRegisterDynamicCmd(0, a, NULL, b, c, d, __FILE__, e)
void SWSFreeUnregisterDynamicCmd(int id);

void ActionsList(COMMAND_T*);
int SWSGetCommandID(void (*cmdFunc)(COMMAND_T*), INT_PTR user = 0, const char** pMenuText = NULL);
COMMAND_T* SWSGetCommandByID(int cmdId);
int IsSwsAction(const char* _actionName);

HMENU SWSCreateMenuFromCommandTable(COMMAND_T pCommands[], HMENU hMenu = NULL, int* iIndex = NULL);;

// Utility functions, sws_util.cpp
BOOL IsCommCtrlVersion6();
void SaveWindowPos(HWND hwnd, const char* cKey);
void RestoreWindowPos(HWND hwnd, const char* cKey, bool bRestoreSize = true);
void SetWindowPosAtMouse(HWND hwnd);

MediaTrack* GetFirstSelectedTrack();
int NumSelTracks();
void SaveSelected();
void RestoreSelected();
void ClearSelected();
int GetFolderDepth(MediaTrack* tr, int* iType, MediaTrack** nextTr);
int GetTrackVis(MediaTrack* tr); // &1 == mcp, &2 == tcp
void SetTrackVis(MediaTrack* tr, int vis); // &1 == mcp, &2 == tcp
int AboutBoxInit(); // Not worth its own .h
void* GetConfigVar(const char* cVar);
HWND GetTrackWnd();
HWND GetRulerWnd();
char* GetHashString(const char* in, char* out);
MediaTrack* GuidToTrack(const GUID* guid);
bool GuidsEqual(const GUID* g1, const GUID* g2);
bool TrackMatchesGuid(MediaTrack* tr, const GUID* g);
const char* stristr(const char* str1, const char* str2);

#ifdef _WIN32
void dprintf(const char* format, ...);
#else
#define dprintf printf
#ifndef OutputDebugString
#define OutputDebugString printf
#endif
#endif

void SWS_GetSelectedTracks(WDL_TypedBuf<MediaTrack*>* buf, bool bMaster = false);
void SWS_GetSelectedMediaItems(WDL_TypedBuf<MediaItem*>* buf);
void SWS_GetSelectedMediaItemsOnTrack(WDL_TypedBuf<MediaItem*>* buf, MediaTrack* tr);
int SWS_GetModifiers();
void WinSpawnNotepad(const char* pFilename);
void makeEscapedConfigString(const char *in, WDL_FastString *out); //JFB: temp (WDL's ProjectContext does not use WDL_FastString yet)
bool SWS_IsWindow(HWND hwnd);

// Localization, sws_util.cpp
#define _SWS_LOCALIZATION
WDL_FastString* GetLangPack();
bool IsLocalized();
const char* GetLocalizedActionName(const char* _defaultStr, int _flags = 0, const char* _section = "sws_actions");
bool IsLocalizableAction(const char* _customId);
TrackEnvelope* SWS_GetTakeEnvelopeByName(MediaItem_Take* take, const char* envname);
TrackEnvelope* SWS_GetTrackEnvelopeByName(MediaTrack* track, const char* envname);

// Generate html whatsnew, MakeWhatsNew.cpp
int GenHtmlWhatsNew(const char* fnIn, const char* fnOut, bool bFullHTML);

// Functions export to reascript and c++ plugins, Reascript.cpp
bool RegisterExportedFuncs(reaper_plugin_info_t* _rec);
bool RegisterExportedAPI(reaper_plugin_info_t* _rec);
