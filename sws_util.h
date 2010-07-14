/******************************************************************************
/ sws_util.h
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

#pragma once
#define IMPAPI(x) if (!((*((void **)&(x)) = (void *)rec->GetFunc(#x)))) errcnt++;
#define IMPVAR(x,nm) if (!((*(void **)&(x)) = get_config_var(nm,&sztmp)) || sztmp != sizeof(*x)) errcnt++;

// Use this macro to include the ReaProject* cast.  Try to get cockos to fix in the gen header
#define Enum_Projects(idx, name, namelen) ((ReaProject*)EnumProjects(idx, name, namelen))

#define SWS_INI "SWS"
#define SWS_SEPARATOR "SEPARATOR"
#define LAST_COMMAND ((char*)(INT_PTR)-1)
#define SWS_STARTSUBMENU ((char*)(INT_PTR)-2)
#define SWS_ENDSUBMENU ((char*)(INT_PTR)-3)
#define MINTRACKHEIGHT 24
#define DEFACCEL { 0, 0, 0 }
#define UTF8_BULLET "\xE2\x80\xA2"
#define UTF8_CIRCLE "\xE2\x97\xA6"
#define UTF8_BOX "\xE2\x96\xA1"
#define UTF8_BBOX "\xE2\x96\xA0"

#ifdef _WIN32
#define PATH_SLASH_CHAR '\\'
#else
#define PATH_SLASH_CHAR '/'
#endif

typedef struct COMMAND_T
{
	gaccel_register_t accel;
	char* id;
	void (*doCommand)(COMMAND_T*);
	const char* menuText;
	INT_PTR user;
	bool (*getEnabled)(COMMAND_T*);
} COMMAND_T;

template<class PTRTYPE> class SWSProjConfig
{
private:
	WDL_PtrList<void> m_projects;
	WDL_PtrList<PTRTYPE> m_data;

public:
	SWSProjConfig() {}
	~SWSProjConfig() { m_data.Empty(true); }
	PTRTYPE* Get()
	{
		ReaProject* pProj = Enum_Projects(-1, NULL, 0);
		int i = m_projects.Find(pProj);
		if (i >= 0)
			return m_data.Get(i);
		m_projects.Add(pProj);
		return m_data.Add(new PTRTYPE);
	}
	PTRTYPE* Get(int iProj) { return m_data.Get(iProj); }
	int GetNumProj() { return m_data.GetSize(); }
	void Cleanup()
	{
		if (m_projects.GetSize())
		{
			for (int i = m_projects.GetSize() - 1; i >= 0; i--)
			{
				int j = 0;
				ReaProject* pProj;
				while ((pProj = Enum_Projects(j, NULL, 0)))
					if (m_projects.Get(j++) == pProj)
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
extern int g_i0;
extern int g_i1;
extern int g_i2;
extern bool g_bTrue;
extern bool g_bFalse;

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
#endif

// Command/action handling, sws_extension.cpp
#define SWSRegisterCommand(c) SWSRegisterCommand2(c, __FILE__)
#define SWSRegisterCommands(c) SWSRegisterCommands2(c, __FILE__)
int SWSRegisterCommand2(COMMAND_T* pCommand, const char* cFile);   // One command
int SWSRegisterCommands2(COMMAND_T* pCommands, const char* cFile); // Multiple commands in a table, terminated with LAST_COMMAND
void ActionsList(COMMAND_T*);
COMMAND_T* SWSUnregisterCommand(int id);
int SWSGetCommandID(void (*cmdFunc)(COMMAND_T*), INT_PTR user = 0, const char** pMenuText = NULL);
HMENU SWSCreateMenu(COMMAND_T pCommands[], HMENU hMenu = NULL, int* iIndex = NULL);

// Utility functions, sws_util.cpp
BOOL IsCommCtrlVersion6();
void AddToMenu(HMENU hMenu, const char* text, int id, int iInsertAfter = -1, bool bPos = false);
void AddSubMenu(HMENU hMenu, HMENU subMenu, const char* text, int iInsertAfter = -1);
HMENU FindMenuItem(HMENU hMenu, int iCmd, int* iPos);
void SWSSetMenuText(HMENU hMenu, int iCmd, const char* cText);
void SaveWindowPos(HWND hwnd, const char* cKey);
void RestoreWindowPos(HWND hwnd, const char* cKey, bool bRestoreSize = true);
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
char* GetHashString(const char* in, char* out);
MediaTrack* GuidToTrack(const GUID* guid);
bool GuidsEqual(const GUID* g1, const GUID* g2);
bool TrackMatchesGuid(MediaTrack* tr, const GUID* g);
const char* stristr(const char* str1, const char* str2);