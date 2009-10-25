/******************************************************************************
/ ProjectMgr.cpp
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


#include "stdafx.h"
#include "ProjectMgr.h"

// Globals
static SWSProjConfig<WDL_PtrList<WDL_String> > g_relatedProjects;

void SaveProjectList(COMMAND_T*)
{
	int i = 0;
	bool bValid = false;
	char filename[256] = { 0, };
	while (EnumProjects(i++, filename, 256))
		if (filename[0])
			bValid = true;
	if (!bValid)
	{
		MessageBox(g_hwndParent, "No saved projects are open.  Please save your project(s) first.", "SWS Project List Save", MB_OK);
		return;
	}

	char cPath[256];
	GetProjectPath(cPath, 256);
//	ofn.lpstrDefExt = "RPL";
	if (BrowseForSaveFile("Select project list", cPath, NULL, "Reaper Project List (*.RPL)\0*.RPL\0All Files\0*.*\0", filename, 256))
	{
		FILE* f = fopen(filename, "w");
		if (f)
		{
			i = 0;
			while (EnumProjects(i++, filename, 256))
			{
				if (filename[0])
				{
					strncat(filename, "\r\n", 256);
					fwrite(filename, strlen(filename), 1, f);
				}
			}
			fclose(f);
		}
		else
			MessageBox(g_hwndParent, "Unable to write to file.", "SWS Project List Save", MB_OK);
	}
}

void OpenProjectList(COMMAND_T*)
{
	char cPath[256];
	GetProjectPath(cPath, 256);
	char* filename = BrowseForFiles("Select project list", cPath, NULL, false, "Reaper Project List (*.RPL)\0*.RPL\0All Files\0*.*\0");
	if (filename)
	{
		FILE* f = fopen(filename, "r");
		if (f)
		{
			// Save "prompt on new project" variable
			int iNewProjOpts;
			int sztmp;
			int* pNewProjOpts = (int*)get_config_var("newprojdo", &sztmp);
			iNewProjOpts = *pNewProjOpts;
			*pNewProjOpts = 0;
			int i = 0;

			if (MessageBox(g_hwndParent, "Close active tabs first?", "SWS Project List Open", MB_YESNO) == IDYES)
				Main_OnCommand(40886, 0);
			else
				i = 1;

			while(fgets(filename, 256, f))
			{
				char* pC;
				while((pC = strchr(filename, '\r'))) *pC = 0; // Strip newlines no matter the format
				while((pC = strchr(filename, '\n'))) *pC = 0;
				if (filename[0])
				{
					if (i++)
						Main_OnCommand(40859, 0); // 40859: New project tab
					Main_openProject(filename);
				}
			}
			fclose(f);

			*pNewProjOpts = iNewProjOpts;
		}
		else
			MessageBox(g_hwndParent, "Unable to open file.", "SWS Project List Open", MB_OK);
		
		free(filename);
	}
}

void AddRelatedProject(COMMAND_T* = NULL)
{
	// TODO fix
	if (g_relatedProjects.Get()->GetSize() >= 5)
	{
		MessageBox(g_hwndParent, "Too many related projects.", "SWS Add Related Project", MB_OK);
		return;
	}

	char cPath[256];
	GetProjectPath(cPath, 256);

	char* filename = BrowseForFiles("Select related project", cPath, NULL, false, "Reaper Project (*.RPP)\0*.RPP");
	if (filename)
	{
		g_relatedProjects.Get()->Add(new WDL_String(filename));
		free(filename);
	}
}

void OpenRelatedProject(COMMAND_T* pCmd)
{
	if ((int)pCmd->user == g_relatedProjects.Get()->GetSize())
		// Give the user the chance to add a related project if they selected the first open spot
		if (MessageBox(g_hwndParent, "No related project found.  Add one now?", "SWS Open Related Project", MB_YESNO) == IDYES)
			AddRelatedProject();

	if ((int)pCmd->user >= g_relatedProjects.Get()->GetSize())
		return;

	WDL_String* pStr = g_relatedProjects.Get()->Get((int)pCmd->user);
	ReaProject* pProj;
	// See if it's already opened
	char cOpenedProj[256];
	int i = 0;
	while ((pProj = Enum_Projects(i++, cOpenedProj, 256)))
	{
		if (_stricmp(cOpenedProj, pStr->Get()) == 0)
		{
			SelectProjectInstance(pProj);
			return;
		}
	}

	// Nope, open in new tab
	// Save "prompt on new project" variable
	int iNewProjOpts;
	int sztmp;
	int* pNewProjOpts = (int*)get_config_var("newprojdo", &sztmp);
	iNewProjOpts = *pNewProjOpts;
	*pNewProjOpts = 0;
	pProj = Enum_Projects(-1, NULL, 0);
	Main_OnCommand(40859, 0); // 40859: New project tab
	Main_openProject(pStr->Get());
	EnumProjects(-1, cOpenedProj, 256);
	if (_stricmp(pStr->Get(), cOpenedProj))
	{
		Main_OnCommand(40860, 0); // 40860 = Close current project tab
		SelectProjectInstance(pProj);
		g_relatedProjects.Get()->Delete((int)pCmd->user, true);
	}
	*pNewProjOpts = iNewProjOpts;
}

void LastProjectTab(COMMAND_T*)
{
	int iNumProjects = 0;
	while (EnumProjects(++iNumProjects, NULL, 0));
	SelectProjectInstance(Enum_Projects(iNumProjects-1, NULL, 0));
}

void FirstProjectTab(COMMAND_T*)
{
	SelectProjectInstance(Enum_Projects(0, NULL, 0));
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

static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "SWS: Save list of open projects" },	"SWS_PROJLISTSAVE",		SaveProjectList,		"Save list of open projects...", },
	{ { DEFACCEL, "SWS: Open projects from list" },		"SWS_PROJLISTSOPEN",	OpenProjectList,		"Open projects from list...", },
	{ { DEFACCEL, "SWS: Add related project" },			"SWS_ADDRELATEDPROJ",	AddRelatedProject,		"Add related project...", },
	{ { DEFACCEL, NULL }, NULL, NULL, SWS_SEPARATOR, },
	{ { DEFACCEL, "SWS: Open related project 1" },		"SWS_OPENRELATED1",		OpenRelatedProject,		"(related projects list)", 0 },
	{ { DEFACCEL, "SWS: Open related project 2" },		"SWS_OPENRELATED2",		OpenRelatedProject,		NULL, 1 },
	{ { DEFACCEL, "SWS: Open related project 3" },		"SWS_OPENRELATED3",		OpenRelatedProject,		NULL, 2 },
	{ { DEFACCEL, "SWS: Open related project 4" },		"SWS_OPENRELATED4",		OpenRelatedProject,		NULL, 3 },
	{ { DEFACCEL, "SWS: Open related project 5" },		"SWS_OPENRELATED5",		OpenRelatedProject,		NULL, 4 },

	{ { DEFACCEL, "SWS: Switch to last project tab" },	"SWS_LASTPROJTAB",		LastProjectTab,			NULL, },
	{ { DEFACCEL, "SWS: Switch to first project tab" },	"SWS_FIRSTPROJTAB",		FirstProjectTab,		NULL, },
	{ {}, LAST_COMMAND, }, // Denote end of table
};

static int g_iORPCmdIndex = 0;
static void menuhook(int menuid, HMENU hMenu, int flag)
{
	if (menuid == MAINMENU_FILE && flag == 0)
		AddSubMenu(hMenu, SWSCreateMenu(g_commandTable), "SWS Project management", 40897);
	else if (flag == 1)
	{
		int iPos;
		hMenu = FindMenuItem(hMenu, g_commandTable[g_iORPCmdIndex].accel.accel.cmd, &iPos);
		if (hMenu)
		{
			if (!g_relatedProjects.Get()->GetSize())
			{
				MENUITEMINFO mi={sizeof(MENUITEMINFO),};
				mi.fMask = MIIM_TYPE | MIIM_STATE;
				mi.fType = MFT_STRING;
				mi.fState = MFS_GRAYED;
				mi.dwTypeData = g_commandTable[g_iORPCmdIndex].menuText;
				SetMenuItemInfo(hMenu, iPos, true, &mi);
			}
			else
			{
				for (int i = g_relatedProjects.Get()->GetSize()-1; i >= 0; i--)
					AddToMenu(hMenu, g_relatedProjects.Get()->Get(i)->Get(), SWSGetCommandID(OpenRelatedProject, i), g_commandTable[g_iORPCmdIndex].accel.accel.cmd);
				DeleteMenu(hMenu, iPos, MF_BYPOSITION);
			}
		}
	}
}

int ProjectMgrInit()
{
	SWSRegisterCommands(g_commandTable);

	// Save the index of OpenRelatedProject() for later
	g_iORPCmdIndex = -1;
	while (g_commandTable[++g_iORPCmdIndex].doCommand != OpenRelatedProject);

	if (!plugin_register("projectconfig",&g_projectconfig))
		return 0;

	if (!plugin_register("hookmenu", (void*)menuhook))
		return 0;

	return 1;
}