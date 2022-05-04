/******************************************************************************
/ ProjectMgr.cpp
/
/ Copyright (c) 2010 Tim Payne (SWS)
/
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

#include "../SnM/SnM_Dlg.h"
#include "ProjectMgr.h"
#include "ProjectList.h"

#include <WDL/localize/localize.h>

// Globals
static SWSProjConfig<WDL_PtrList_DOD<WDL_String> > g_relatedProjects;

#define DELWINDOW_POS_KEY "DelRelatedProjectWindowPosition"

// ****************************************************************************
// **** Project list  *********************************************************
// ****************************************************************************
void SaveProjectList(COMMAND_T*)
{
	std::vector<std::string> projectFiles;

	int i = 0;
	char filename[MAX_PATH]{};
	while (EnumProjects(i++, filename, sizeof(filename)))
	{
		if (filename[0])
			projectFiles.push_back(filename);
	}

	if (projectFiles.empty())
	{
		MessageBox(g_hwndParent, __LOCALIZE("No saved projects are open. Please save your project(s) first.","sws_mbox"), __LOCALIZE("SWS Project List Save","sws_mbox"), MB_OK);
		return;
	}

	char cPath[MAX_PATH];
	GetProjectPath(cPath, sizeof(cPath));
	if (!BrowseForSaveFile(__LOCALIZE("Select project list","sws_mbox"), cPath, nullptr, "Reaper Project List (*.RPL)\0*.RPL\0All Files\0*.*\0", filename, sizeof(filename)))
		return;

	std::ofstream file(win32::widen(filename).c_str());

	if (!file)
	{
		MessageBox(g_hwndParent, __LOCALIZE("Unable to write to file.","sws_mbox"), __LOCALIZE("SWS Project List Save","sws_mbox"), MB_OK);
		return;
	}

	for (const std::string &projectfn : projectFiles)
		file << projectfn << "\n";

	file.close();
}

void OpenProjectsFromList(COMMAND_T*)
{
	char directory[MAX_PATH]{};
	GetProjectPath(directory, sizeof(directory));

	char *filename = BrowseForFiles(__LOCALIZE("Select project list","sws_mbox"), directory, nullptr, false, "Reaper Project List (*.RPL)\0*.RPL\0All Files\0*.*\0");
	if (!filename)
		return;

	std::ifstream file(win32::widen(filename).c_str());
	free(filename);

	if (!file)
	{
		MessageBox(g_hwndParent, __LOCALIZE("Unable to open file.","sws_mbox"), __LOCALIZE("SWS Project List Open","sws_mbox"), MB_OK);
		return;
	}

	// Save "prompt on new project" variable
	ConfigVarOverride<int> newprojdo("newprojdo", 0);

	int projectCount = -1;
	while (EnumProjects(++projectCount, nullptr, 0));

	char projectfn[10];
	EnumProjects(-1, projectfn, sizeof(projectfn));

	bool newTab = false;

	if (projectCount > 1 || projectfn[0] || GetNumTracks() > 0)
	{
		if (MessageBox(g_hwndParent, __LOCALIZE("Close active tabs first?","sws_mbox"), __LOCALIZE("SWS Project List Open","sws_mbox"), MB_YESNO) == IDYES)
			Main_OnCommand(40886, 0); // File: Close all projects
		else
			newTab = true;
	}

	std::string line;
	while (std::getline(file, line))
	{
		if (line.empty())
			continue;

		if (newTab)
			Main_OnCommand(41929, 0); // New project tab (ignore default template)
		else
			newTab = true;

		Main_openProject(line.c_str());
	}

	file.close();
}

// ****************************************************************************
// **** Related projects ******************************************************
// ****************************************************************************

void AddRelatedProject(COMMAND_T* = NULL)
{
	char cPath[256];
	GetProjectPath(cPath, 256);

	char* filename = BrowseForFiles(__LOCALIZE("Select related project(s)","sws_mbox"), cPath, NULL, false, "Reaper Project (*.RPP)\0*.RPP");
	char* pBuf = filename;
	if (pBuf)
	{
		while(filename[0])
		{
			g_relatedProjects.Get()->Add(new WDL_String(filename));
			filename += strlen(filename) +1;
		}

		free(pBuf);
		Undo_OnStateChangeEx(__LOCALIZE("Add related project(s)","sws_mbox"), UNDO_STATE_MISCCFG, -1);
	}
}

void OpenRelatedProject(COMMAND_T* pCmd)
{
	if ((int)pCmd->user == g_relatedProjects.Get()->GetSize())
		// Give the user the chance to add a related project if they selected the first open spot
		if (MessageBox(g_hwndParent, __LOCALIZE("No related project found. Add one now?","sws_mbox"), __LOCALIZE("SWS Open Related Project","sws_mbox"), MB_YESNO) == IDYES)
			AddRelatedProject();

	if ((int)pCmd->user >= g_relatedProjects.Get()->GetSize())
		return;

	WDL_String* pStr = g_relatedProjects.Get()->Get((int)pCmd->user);
	ReaProject* pProj;
	// See if it's already opened
	char cOpenedProj[256];
	int i = 0;
	while ((pProj = EnumProjects(i++, cOpenedProj, 256)))
	{
		if (_stricmp(cOpenedProj, pStr->Get()) == 0)
		{
			SelectProjectInstance(pProj);
			return;
		}
	}

	// Nope, open in new tab
	// Save "prompt on new project" variable
	ConfigVarOverride<int> newprojdo("newprojdo", 0);
	pProj = EnumProjects(-1, NULL, 0);
	Main_OnCommand(41929, 0); // New project tab (ignore default template)
	Main_openProject(pStr->Get());
	EnumProjects(-1, cOpenedProj, 256);
	if (_stricmp(pStr->Get(), cOpenedProj))
	{
		Main_OnCommand(40860, 0); // 40860 = Close current project tab
		SelectProjectInstance(pProj);
		g_relatedProjects.Get()->Delete((int)pCmd->user, true);
	}
}

void OpenLastProject(COMMAND_T*)
{
	char key[32];
	char cLastProj[MAX_PATH];
	int recentNum = GetPrivateProfileInt("REAPER", "numrecent", 0, get_ini_file());
	if (recentNum > 0)
	{
		sprintf(key, "recent%02d", recentNum);
		GetPrivateProfileString("Recent", key, "", cLastProj, MAX_PATH, get_ini_file());
		if (cLastProj[0])
			Main_openProject(cLastProj);
	}
}

static int GetLoadCommandID(int iSlot, bool bCreateNew)
{
	static int iLastRegistered = 0;
	if (iSlot > iLastRegistered && bCreateNew)
	{
		char cID[BUFFER_SIZE];
		char cDesc[BUFFER_SIZE];
		snprintf(cID, BUFFER_SIZE, "SWS_OPENRELATED%d", iSlot+1);
		snprintf(cDesc, BUFFER_SIZE, __LOCALIZE_VERFMT("SWS: Open related project %d","sws_actions"), iSlot+1);
		iLastRegistered = iSlot;
		return SWSRegisterCommandExt(OpenRelatedProject, cID, cDesc, iSlot, false);
	}

	return SWSGetCommandID(OpenRelatedProject, iSlot);
}

static INT_PTR WINAPI doDeleteDialog(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwndDlg, uMsg, wParam, lParam))
		return r;

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			HWND list = GetDlgItem(hwndDlg, IDC_COMBO);
			WDL_UTF8_HookComboBox(list);
			for (int i = 0; i < g_relatedProjects.Get()->GetSize(); i++)
				SendMessage(list, CB_ADDSTRING, 0, (LPARAM)g_relatedProjects.Get()->Get(i)->Get());
            SendMessage(list, CB_SETCURSEL, 0, 0);
			SetWindowText(hwndDlg, __LOCALIZE("SWS Delete Related Project","sws_mbox"));
			RestoreWindowPos(hwndDlg, DELWINDOW_POS_KEY);
			return 0;
		}
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
				{
					HWND list = GetDlgItem(hwndDlg, IDC_COMBO);
					int iList = (int)SendMessage(list, CB_GETCURSEL, 0, 0);
					if (iList >= 0 && iList < g_relatedProjects.Get()->GetSize())
						g_relatedProjects.Get()->Delete(iList, true);
					Undo_OnStateChangeEx(__LOCALIZE("Delete related project","sws_undo"), UNDO_STATE_MISCCFG, -1);
				}
				// Fall through to cancel to save/end
				case IDCANCEL:
					SaveWindowPos(hwndDlg, DELWINDOW_POS_KEY);
					EndDialog(hwndDlg, 0);
					break;
			}
			break;
	}
	return 0;
}

void DelRelatedProject(COMMAND_T*)
{
	if (!g_relatedProjects.Get()->GetSize())
		MessageBox(g_hwndParent, __LOCALIZE("No related projects to delete","sws_mbox"), __LOCALIZE("SWS Delete Related Project","sws_mbox"), MB_OK);
	else
		DialogBox(g_hInst, MAKEINTRESOURCE(IDD_LOAD) ,g_hwndParent, doDeleteDialog);
}

void LastProjectTab(COMMAND_T*)
{
	int iNumProjects = 0;
	while (EnumProjects(++iNumProjects, NULL, 0));
	SelectProjectInstance(EnumProjects(iNumProjects-1, NULL, 0));
}

void OpenProjectTab(COMMAND_T* ctx)
{
	ReaProject* proj = EnumProjects((int)ctx->user, NULL, 0);
	if (proj)
		SelectProjectInstance(proj);
}

void UpdateOpenProjectTabActions()
{
	// Add more actions for project tabs if > 10
	static int iActions = 10;
	int iProjs = iActions-1;
	while (EnumProjects(++iProjs, NULL, 0));
	if (iProjs > iActions)
		for (; iActions < iProjs; iActions++)
		{
			char cID[BUFFER_SIZE];
			char cDesc[BUFFER_SIZE];
			snprintf(cID, BUFFER_SIZE, "SWS_PROJTAB%d", iActions+1);
			snprintf(cDesc, BUFFER_SIZE, __LOCALIZE_VERFMT("SWS: Switch to project tab %d","sws_actions"), iActions+1);
			SWSRegisterCommandExt(OpenProjectTab, cID, cDesc, iActions, false);
		}
}

static bool ProcessExtensionLine(const char *line, ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	LineParser lp(false);
	if (lp.parse(line) || lp.getnumtokens() < 1)
		return false;
	if (strcmp(lp.gettoken_str(0), "RELATEDPROJECT") != 0)
		return false; // only look for RELATEDPROJECT lines

	if (lp.getnumtokens() == 2)
	{
		g_relatedProjects.Get()->Add(new WDL_String(lp.gettoken_str(1)));
	}
	return true;
}

static void SaveExtensionConfig(ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	for (int i = 0; i < g_relatedProjects.Get()->GetSize(); i++)
		ctx->AddLine("RELATEDPROJECT \"%s\"", g_relatedProjects.Get()->Get(i)->Get());
}

static void BeginLoadProjectState(bool isUndo, struct project_config_extension_t *reg)
{
	g_relatedProjects.Get()->Empty(true);
	g_relatedProjects.Cleanup();
}

static project_config_extension_t g_projectconfig = { ProcessExtensionLine, SaveExtensionConfig, BeginLoadProjectState, NULL };

//!WANT_LOCALIZE_SWS_CMD_TABLE_BEGIN:sws_actions
COMMAND_T g_projMgrCmdTable[] = 
{
	{ { DEFACCEL, "SWS: Save list of open projects" },	"SWS_PROJLISTSAVE",		SaveProjectList,		"Save list of open projects...", },
	{ { DEFACCEL, "SWS: Open projects from list" },		"SWS_PROJLISTSOPEN",	OpenProjectsFromList,	"Open projects from list...", },
	{ { DEFACCEL, "SWS: Add related project(s)" },		"SWS_ADDRELATEDPROJ",	AddRelatedProject,		"Add related project(s)...", },
	{ { DEFACCEL, "SWS: Delete related project" },		"SWS_DELRELATEDPROJ",	DelRelatedProject,		"Delete related project...", },
	{ { DEFACCEL, "SWS: Open project list" },			"SWS_PROJLIST_OPEN",	OpenProjectList,		"SWS Project List", 0, ProjectListEnabled },
	{ { DEFACCEL, "SWS: Open related project 1" },		"SWS_OPENRELATED1",		OpenRelatedProject,		"(related projects list)", 0 },
	{ { DEFACCEL, "SWS: Open last project" },			"SWS_OPENLASTPROJ",		OpenLastProject,		NULL, },

	{ { DEFACCEL, "SWS: Switch to last project tab" },	"SWS_LASTPROJTAB",		LastProjectTab,			NULL, },
	{ { DEFACCEL, "SWS: Switch to project tab 1" },		"SWS_FIRSTPROJTAB",		OpenProjectTab,			NULL, 0 },
	{ { DEFACCEL, "SWS: Switch to project tab 2" },		"SWS_PROJTAB2",			OpenProjectTab,			NULL, 1 },
	{ { DEFACCEL, "SWS: Switch to project tab 3" },		"SWS_PROJTAB3",			OpenProjectTab,			NULL, 2 },
	{ { DEFACCEL, "SWS: Switch to project tab 4" },		"SWS_PROJTAB4",			OpenProjectTab,			NULL, 3 },
	{ { DEFACCEL, "SWS: Switch to project tab 5" },		"SWS_PROJTAB5",			OpenProjectTab,			NULL, 4 },
	{ { DEFACCEL, "SWS: Switch to project tab 6" },		"SWS_PROJTAB6",			OpenProjectTab,			NULL, 5 },
	{ { DEFACCEL, "SWS: Switch to project tab 7" },		"SWS_PROJTAB7",			OpenProjectTab,			NULL, 6 },
	{ { DEFACCEL, "SWS: Switch to project tab 8" },		"SWS_PROJTAB8",			OpenProjectTab,			NULL, 7 },
	{ { DEFACCEL, "SWS: Switch to project tab 9" },		"SWS_PROJTAB9",			OpenProjectTab,			NULL, 8 },
	{ { DEFACCEL, "SWS: Switch to project tab 10" },	"SWS_PROJTAB10",		OpenProjectTab,			NULL, 9 },

	{ {}, LAST_COMMAND, }, // Denote end of table
};
//!WANT_LOCALIZE_SWS_CMD_TABLE_END

static int g_iORPCmdIndex = 0;
static void menuhook(const char* menustr, HMENU hMenu, int flag)
{
	if (flag == 1)
	{
		// Delete all related project entries and regenerate
		int iFirstPos;

		hMenu = FindMenuItem(hMenu, g_projMgrCmdTable[g_iORPCmdIndex].cmdId, &iFirstPos);

		if (hMenu)
		{
			int iSlot = 0;
			int iPos;
			while (true)
			{
				int iCmd = GetLoadCommandID(iSlot, false);
				if (iCmd)
				{
					if (FindMenuItem(hMenu, iCmd, &iPos))
						DeleteMenu(hMenu, iPos, MF_BYPOSITION);
					else
						break;
				}
				else
					break;
				iSlot++;
			}

			if (!g_relatedProjects.Get()->GetSize())
			{
				MENUITEMINFO mi={sizeof(MENUITEMINFO),};
				mi.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID;
				mi.fType = MFT_STRING;
				mi.fState = MFS_GRAYED;
				mi.dwTypeData = (char *)__localizeFunc(g_projMgrCmdTable[g_iORPCmdIndex].menuText,"sws_menu",0);
				mi.wID = g_projMgrCmdTable[g_iORPCmdIndex].cmdId;
				InsertMenuItem(hMenu, iFirstPos, true, &mi);
			}
			else
			{
				for (int i = 0; i < g_relatedProjects.Get()->GetSize(); i++)
					AddToMenu(hMenu, g_relatedProjects.Get()->Get(i)->Get(), GetLoadCommandID(i, true), iFirstPos++, true, MFS_UNCHECKED);
			}
		}
	}
}

int ProjectMgrInit()
{
	SWSRegisterCommands(g_projMgrCmdTable);

	// Save the index of OpenRelatedProject() for later
	g_iORPCmdIndex = -1;
	while (g_projMgrCmdTable[++g_iORPCmdIndex].doCommand != OpenRelatedProject);

	if (!plugin_register("projectconfig",&g_projectconfig))
		return 0;

	if (!plugin_register("hookcustommenu", (void*)menuhook))
		return 0;

	return 1;
}

void ProjectMgrExit()
{
	plugin_register("-projectconfig",&g_projectconfig);
	plugin_register("-hookcustommenu", (void*)menuhook);
}
