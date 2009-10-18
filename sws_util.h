/******************************************************************************
/ sws_util.h
/
/ Copyright (c) 2009 Tim Payne (SWS)
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
#define LAST_COMMAND ((char*)-1)
#define SWS_STARTSUBMENU ((char*)-2)
#define SWS_ENDSUBMENU ((char*)-3)
#define MINTRACKHEIGHT 24
#define DEFACCEL { 0, 0, 0 }

typedef struct COMMAND_T
{
	gaccel_register_t accel;
	char* id;
	void (*doCommand)(COMMAND_T*);
	char* menuText;
	int user;
} COMMAND_T;

template<class PTRTYPE> class SWSProjConfig
{
private:
	WDL_PtrList<void> m_projects;
	WDL_PtrList<PTRTYPE> m_data;

public:
	SWSProjConfig() {}
	PTRTYPE* Get()
	{
		ReaProject* pProj = Enum_Projects(-1, NULL, 0);
		int i = m_projects.Find(pProj);
		if (i >= 0)
			return m_data.Get(i);
		m_projects.Add(pProj);
		return m_data.Add(new PTRTYPE);
	}
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
#define WM_KILLFOCUS 0x0BADF00D
#define BS_MULTILINE 0
#define _strndup strndup
#define _snprintf snprintf
#define _stricmp stricmp
#define _strnicmp strnicmp
//sws_util.mm
void GetDateString(int time, char* buf, int bufsize);
void GetTimeString(int time, char* buf, int bufsize);
int GetCustomColors();
#endif
// Utility functions
BOOL IsCommCtrlVersion6();
void AddToMenu(HMENU hMenu, const char* text, int id, int iInsertAfter = -1, bool bPos = false);
void AddSubMenu(HMENU hMenu, HMENU subMenu, const char* text, int iInsertAfter = -1);
HMENU FindMenuItem(HMENU hMenu, int iCmd, int* iPos);
void SWSCheckMenuItem(HMENU hMenu, int iCmd, bool bChecked);
void SWSSetMenuText(HMENU hMenu, int iCmd, const char* cText);
void SaveWindowPos(HWND hwnd, const char* cKey);
void RestoreWindowPos(HWND hwnd, const char* cKey, bool bRestoreSize = true);
int NumSelTracks();
void SaveSelected();
void RestoreSelected();
void ClearSelected();
int SWSRegisterCommand(COMMAND_T* pCommand);   // One command
int SWSRegisterCommands(COMMAND_T* pCommands); // Multiple commands in a table, terminated with LAST_COMMAND
int SWSGetCommandID(void (*cmdFunc)(COMMAND_T*), int user = 0, char** pMenuText = NULL);
HMENU SWSCreateMenu(COMMAND_T pCommands[], HMENU hMenu = NULL, int* iIndex = NULL);
int GetFolderDepth(MediaTrack* tr, int* iType, MediaTrack** nextTr);
int GetTrackVis(MediaTrack* tr); // &1 == mcp, &2 == tcp
void SetTrackVis(MediaTrack* tr, int vis); // &1 == mcp, &2 == tcp
int AboutBoxInit(); // Not worth its own .h
void* GetConfigVar(const char* cVar);
