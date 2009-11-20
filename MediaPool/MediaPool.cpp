/******************************************************************************
/ MediaPool.cpp
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
#include "MediaPool.h"

#define DELETE_MSG		0x10004
	
// Globals
static COMMAND_T* g_pCommandTable;
static SWS_MediaPoolWnd* g_pMediaPoolWnd;

void InsertFile(COMMAND_T* t)
{
	InsertMedia((char*)t->user, 0);
}

SWS_MediaPoolFile::SWS_MediaPoolFile(const char* cFilename, int id, bool bAction): m_id(id), m_bAction(bAction)
{
	m_cFilename = new char[strlen(cFilename)+1];
	strcpy(m_cFilename, cFilename);
}

SWS_MediaPoolFile::~SWS_MediaPoolFile()
{
	delete [] m_cFilename;
}

char* SWS_MediaPoolFile::GetString(char* str, int iStrMax)
{
	_snprintf(str, iStrMax, "  \"%s\" %d %d\n", m_cFilename, m_id, m_bAction ? 1 : 0);
	return str;
}

SWS_MediaPoolGroup::SWS_MediaPoolGroup(const char* cName, bool bGlobal):m_cGroupname(NULL), m_bGlobal(bGlobal)
{
	SetName(cName);
}

void SWS_MediaPoolGroup::AddFile(const char* cFile, int id, bool bAction)
{
	// Don't add dupes
	for (int i = 0; i < m_files.GetSize(); i++)
		if (_stricmp(cFile, m_files.Get(i)->m_cFilename) == 0)
			return;

	SWS_MediaPoolFile* pFile = m_files.Add(new SWS_MediaPoolFile(cFile, id, bAction));
	if (m_bGlobal && bAction)
		RegisterGlobalCommand(pFile->m_cFilename);
	else if (bAction)
		RegisterLocalCommand(pFile->m_cFilename, pFile->m_id);
}

void SWS_MediaPoolGroup::SetName(const char* cName)
{
	if (m_cGroupname)
		delete m_cGroupname;

	if (cName && cName[0])
	{
		m_cGroupname = new char[strlen(cName)+1];
		strcpy(m_cGroupname, cName);
	}
	else
		m_cGroupname = NULL;
}

void SWS_MediaPoolGroup::RegisterGlobalCommand(const char* cFilename)
{
	COMMAND_T* cmd = new COMMAND_T;
	memset(&cmd->accel.accel, 0, sizeof(cmd->accel.accel));
	const char* desc = "SWS: Insert from media pool %s";
	cmd->accel.desc = new char[strlen(desc) + strlen(cFilename)];
	sprintf((char*)cmd->accel.desc, desc, cFilename);
	const char* id = "SWSMP_";
	cmd->id = new char[strlen(id) + 41];
	strcpy(cmd->id, id);
	GetHashString(cFilename, cmd->id + strlen(id));
	cmd->doCommand = InsertFile;
	cmd->menuText = NULL;
	cmd->user = (int)cFilename;
	SWSRegisterCommand(cmd);			
}

void SWS_MediaPoolGroup::RegisterLocalCommand(const char* cFilename, int id)
{
	COMMAND_T* cmd = new COMMAND_T;
	memset(&cmd->accel.accel, 0, sizeof(cmd->accel.accel));
	const char* desc = "SWS: Insert from media pool %s";
	cmd->accel.desc = new char[strlen(desc) + strlen(cFilename)];
	sprintf((char*)cmd->accel.desc, desc, cFilename);
	const char* idStr = "SWSMP_INSERT%d";
	cmd->id = new char[strlen(idStr) + 5];
	sprintf(cmd->id, idStr, id);
	cmd->doCommand = InsertFile;
	cmd->menuText = NULL;
	cmd->user = (int)cFilename;
	SWSRegisterCommand(cmd);			
}

void SWS_MediaPoolGroup::UnregisterGlobalCommand(const char* cFile)
{
	// TODO
}

void SWS_MediaPoolGroup::UnregisterLocalCommand(const char* cFile, int id)
{
	// TODO
}

SWS_MediaPoolGroup::~SWS_MediaPoolGroup()
{
	m_files.Empty(true);
	delete [] m_cGroupname;
}

static SWS_LVColumn g_gvCol = { 250, 1, "" };

SWS_MediaPoolGroupView::SWS_MediaPoolGroupView(HWND hwndList, HWND hwndEdit, SWS_MediaPoolWnd* pWnd)
:SWS_ListView(hwndList, hwndEdit, 1, &g_gvCol, "MediaPool Group State", false), m_pWnd(pWnd)
{
}

void SWS_MediaPoolGroupView::SetItemText(LPARAM item, int iCol, const char* str)
{
	((SWS_MediaPoolGroup*)item)->SetName(str);
}

void SWS_MediaPoolGroupView::GetItemText(LPARAM item, int iCol, char* str, int iStrMax)
{
	lstrcpyn(str, ((SWS_MediaPoolGroup*)item)->m_cGroupname, iStrMax);
}

bool SWS_MediaPoolGroupView::OnItemSelChange(LPARAM item, bool bSel)
{
	SWS_MediaPoolGroup* group = (SWS_MediaPoolGroup*)item;
	if (m_pWnd->m_curGroup != group)
	{
		m_pWnd->m_curGroup = group;
		m_pWnd->Update();
	}
	return false;
}

int SWS_MediaPoolGroupView::GetItemCount()
{
	return m_pWnd->m_globalGroups.GetSize() + m_pWnd->m_projGroups.Get()->GetSize();
}

LPARAM SWS_MediaPoolGroupView::GetItemPointer(int iItem)
{
	int iSize = m_pWnd->m_globalGroups.GetSize();
	if (iItem < iSize)
		return (LPARAM)m_pWnd->m_globalGroups.Get(iItem);
	else
		return (LPARAM)m_pWnd->m_projGroups.Get()->Get(iItem - iSize);
}

bool SWS_MediaPoolGroupView::GetItemState(LPARAM item)
{
	return (SWS_MediaPoolGroup*)item == m_pWnd->m_curGroup;
}

static SWS_LVColumn g_fvCols[] = { { 300, 0, "Path" }, { 200, 0, "Filename" }, { 35, 0, "Act" } };

SWS_MediaPoolFileView::SWS_MediaPoolFileView(HWND hwndList, HWND hwndEdit, SWS_MediaPoolWnd* pWnd)
:SWS_ListView(hwndList, hwndEdit, 3, g_fvCols, "MediaPool FileList State", false), m_pWnd(pWnd)
{
}

void SWS_MediaPoolFileView::GetItemText(LPARAM item, int iCol, char* str, int iStrMax)
{
	SWS_MediaPoolFile* pFile = (SWS_MediaPoolFile*)item;
	switch (iCol)
	{
		case 0: // Path
		{
			lstrcpyn(str, pFile->m_cFilename, iStrMax);
			char* pSlash = strrchr(str, PATH_SLASH_CHAR);
			if (pSlash)
				*pSlash = 0;
			break;
		}
		case 1: // Filename
		{
			const char* pSlash = strrchr(pFile->m_cFilename, PATH_SLASH_CHAR);
			if (pSlash)
				lstrcpyn(str, pSlash+1, iStrMax);
			else
				lstrcpyn(str, pFile->m_cFilename, iStrMax);
			break;
		}
		case 2:
			*str = 0;
			break;
	}
}

bool SWS_MediaPoolFileView::OnItemSelChange(LPARAM item, bool bSel)
{
	// TODO update the info text
	return false;
}

void SWS_MediaPoolFileView::OnItemDblClk(LPARAM item, int iCol)
{
	// Insert the file
	InsertMedia(((SWS_MediaPoolFile*)item)->m_cFilename, 0);
}

int SWS_MediaPoolFileView::GetItemCount()
{
	if (m_pWnd->m_curGroup)
		return m_pWnd->m_curGroup->m_files.GetSize();
	return 0;
}

LPARAM SWS_MediaPoolFileView::GetItemPointer(int iItem)
{
	if (m_pWnd->m_curGroup)
		return (LPARAM)m_pWnd->m_curGroup->m_files.Get(iItem);
	return 0;
}


void SWS_MediaPoolFileView::OnBeginDrag()
{
	// TODO, see DragDrop.cpp/.h
}

SWS_MediaPoolWnd::SWS_MediaPoolWnd()
:SWS_DockWnd(IDD_MEDIAPOOL, "Media Pool", 30004), m_curGroup(NULL)
{
	// Load files from the "database"
	char cBuf[512];
	strncpy(cBuf, get_ini_file(), 256);
	char* pC = strrchr(cBuf, PATH_SLASH_CHAR);
	if (pC)
	{
		strcpy(pC+1, "sws_mediapool.txt");
		FILE* f = fopen(cBuf, "r");
		if (f)
		{
			int iGroup = -1;
			while (fgets(cBuf, 512, f))
			{
				LineParser lp(false);
				if (!lp.parse(cBuf))
				{
					if (strcmp(lp.gettoken_str(0), "<GROUP") == 0)
					{
						m_globalGroups.Add(new SWS_MediaPoolGroup(lp.gettoken_str(1), true));
						iGroup++;
					}
					else if (lp.gettoken_str(0)[0] != '>')
					{
						m_globalGroups.Get(iGroup)->AddFile(lp.gettoken_str(0), lp.gettoken_int(1), lp.gettoken_int(2) ? true : false);
					}
				}
			}
			fclose(f);
		}
	}

	if (m_bShowAfterInit)
		Show(false, false);
}

void SWS_MediaPoolWnd::Update()
{
	static bool bRecurseCheck = false;
	if (!IsValidWindow() || bRecurseCheck || !m_pLists.GetSize())
		return;
	bRecurseCheck = true;
	m_pLists.Get(0)->Update();
	m_pLists.Get(1)->Update();
	bRecurseCheck = false;
}

void SWS_MediaPoolWnd::OnInitDlg()
{
	m_resize.init_item(IDC_GROUPS, 0.0, 0.0, 0.0, 1.0);
	m_resize.init_item(IDC_FILES, 0.0, 0.0, 1.0, 1.0);
	m_resize.init_item(IDC_ADD, 0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_ADDGROUP, 0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_PREVIEW, 0.0, 1.0, 0.0, 1.0);
	m_resize.init_item(IDC_INFO, 0.0, 1.0, 0.0, 1.0);
#ifdef _WIN32
	DragAcceptFiles(m_hwnd, true);
#endif

	m_pLists.Add(new SWS_MediaPoolGroupView(GetDlgItem(m_hwnd, IDC_GROUPS), GetDlgItem(m_hwnd, IDC_EDIT), this));
	m_pLists.Add(new SWS_MediaPoolFileView(GetDlgItem(m_hwnd, IDC_FILES), GetDlgItem(m_hwnd, IDC_EDIT), this));

	Update();
}

void SWS_MediaPoolWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
		case IDC_ADD | (BN_CLICKED << 16):
			if (!m_curGroup)
			{
				MessageBox(m_hwnd, "Please select or add a group first.", "Media Pool Error", MB_OK);
			}
			else
			{
				char projPath[MAX_PATH];
				GetProjectPath(projPath, MAX_PATH);
				char* pFiles = BrowseForFiles("Select file(s)", projPath, NULL, true, plugin_getFilterList());
				if (pFiles)
				{
					char* pBuf = pFiles;
					while(pFiles[0])
					{
						char* pExt = strrchr(pFiles, '.');
						if (pExt && IsMediaExtension(pExt, false))
						{
							m_curGroup->AddFile(pFiles, m_curGroup->m_files.GetSize(), false);
							pFiles += strlen(pFiles) + 1;
						}
					}
					free(pBuf);
				}
				Update();
			}
			break;
		case IDC_ADDGROUP | (BN_CLICKED << 16):
		{
			char cGroupName[128] = "";
			if (GetUserInputs("Add Group", 1, "Group name", cGroupName, 128) && cGroupName[0])
			{
				// TODO global vs. proj groups
				m_curGroup = m_globalGroups.Add(new SWS_MediaPoolGroup(cGroupName, true));
				Update();
			}
			break;
		}
		case IDC_PREVIEW | (EN_CHANGE << 16):
			// TODO preview media
			MessageBox(m_hwnd, "Not yet implemented.", "Media Pool Error", MB_OK);
			break;
		case DELETE_MSG:
			// Delete selected
			Update();
			break;
	}
}

void SWS_MediaPoolWnd::OnDroppedFiles(HDROP h)
{
	// Check to see if we dropped on a group
	POINT pt;
	DragQueryPoint(h, &pt);
	SWS_MediaPoolGroup* pGroup = (SWS_MediaPoolGroup*)m_pLists.Get(0)->GetHitItem(pt.x, pt.y, NULL);
	if (pGroup)
		m_curGroup = pGroup;
	
	if (m_curGroup)
	{
		char cFile[512];
		int iFiles = DragQueryFile(h, 0xFFFFFFFF, NULL, 0);
		for (int i = 0; i < iFiles; i++)
		{
			DragQueryFile(h, i, cFile, 512);
			char* pExt = strrchr(cFile, '.');
			if (pExt && IsMediaExtension(pExt, false))
				m_curGroup->AddFile(cFile, m_curGroup->m_files.GetSize(), false);
		}
	}
	else
		MessageBox(m_hwnd, "Please select or add a group first.", "Media Pool Error", MB_OK);

	DragFinish(h);
}

HMENU SWS_MediaPoolWnd::OnContextMenu(int x, int y)
{
	// TODO ?
	return NULL;
	//HMENU contextMenu = CreatePopupMenu();
	//return contextMenu;
}

void OpenMediaPool(COMMAND_T*)
{
	g_pMediaPoolWnd->Show(true, true);
}

static bool ProcessExtensionLine(const char *line, ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	LineParser lp(false);
	if (lp.parse(line) || lp.getnumtokens() < 1)
		return false;
	if (strcmp(lp.gettoken_str(0), "<SWSMEDIA") != 0)
		return false; // only look for <SWSMEDIA lines

	char linebuf[4096];
	WDL_PtrList<SWS_MediaPoolGroup>* pGroups = g_pMediaPoolWnd->m_projGroups.Get();
	while(true)
	{
		if (!ctx->GetLine(linebuf,sizeof(linebuf)) && !lp.parse(linebuf))
		{
			if (strcmp(lp.gettoken_str(0), "<GROUP") == 0)
				pGroups->Add(new SWS_MediaPoolGroup(lp.gettoken_str(1), false));
			else if (lp.gettoken_str(0)[0] != '>')
				break;
			else
				pGroups->Get(pGroups->GetSize()-1)->AddFile(lp.gettoken_str(0), lp.gettoken_int(1), lp.gettoken_int(2) ? true : false);
		}
		else
			break;
	}
	g_pMediaPoolWnd->Update();
	return true;
}

static void SaveExtensionConfig(ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	WDL_PtrList<SWS_MediaPoolGroup>* pGroups = g_pMediaPoolWnd->m_projGroups.Get();
	if (!pGroups->GetSize())
		return;
	ctx->AddLine("<SWSMEDIA");
	char cLine[512];
	for (int i = 0; i < pGroups->GetSize(); i++)
	{
		ctx->AddLine("<GROUP \"%s\"", pGroups->Get(i)->m_cGroupname); 
		for (int j = 0; j < pGroups->Get(i)->m_files.GetSize(); j++)
			ctx->AddLine(pGroups->Get(i)->m_files.Get(j)->GetString(cLine, 512));
		ctx->AddLine(">");
	}
	ctx->AddLine(">");
}

static void BeginLoadProjectState(bool isUndo, struct project_config_extension_t *reg)
{
	g_pMediaPoolWnd->m_projGroups.Get()->Empty(true);
	g_pMediaPoolWnd->m_projGroups.Cleanup();
	g_pMediaPoolWnd->Update();
}

static project_config_extension_t g_projectconfig = { ProcessExtensionLine, SaveExtensionConfig, BeginLoadProjectState, NULL };

static COMMAND_T g_commandTable[] =
{
	{ { DEFACCEL, "SWS: Show media pool" }, "SWSMP_OPEN", OpenMediaPool, "SWS Media pool", },
	{ {}, LAST_COMMAND, }, // Denote end of table
};

// Seems damned "hacky" to me, but it works, so be it.
static int translateAccel(MSG *msg, accelerator_register_t *ctx)
{
	if (g_pMediaPoolWnd->IsActive())
	{
		if (msg->message == WM_KEYDOWN)
		{
			bool bCtrl  = GetAsyncKeyState(VK_CONTROL) & 0x8000 ? true : false;
			bool bAlt   = GetAsyncKeyState(VK_MENU)    & 0x8000 ? true : false;
			bool bShift = GetAsyncKeyState(VK_SHIFT)   & 0x8000 ? true : false;
			HWND hwnd = g_pMediaPoolWnd->GetHWND();

#ifdef _WIN32
			if (msg->wParam == VK_TAB && !bCtrl && !bAlt && !bShift)
			{
				SendMessage(hwnd, WM_NEXTDLGCTL, 0, 0);
				return 1;
			}
			else if (msg->wParam == VK_TAB && !bCtrl && !bAlt && bShift)
			{
				SendMessage(hwnd, WM_NEXTDLGCTL, 1, 0);
				return 1;
			}
			else
#endif				
			if (msg->wParam == VK_DELETE && !bCtrl && !bAlt && !bShift)
			{
				SendMessage(hwnd, WM_COMMAND, DELETE_MSG, 0);
				return 1;
			}
		}
		return -666;
	}
	return 0;
}

static accelerator_register_t g_ar = { translateAccel, TRUE, NULL };

static void menuhook(const char* menustr, HMENU hMenu, int flag)
{
	if (strcmp(menustr, "Main view") == 0 && flag == 0)
		AddToMenu(hMenu, g_commandTable[0].menuText, g_commandTable[0].accel.accel.cmd, 40075);
	else if (flag == 1)
		SWSCheckMenuItem(hMenu, g_commandTable[0].accel.accel.cmd, g_pMediaPoolWnd->IsValidWindow());
}

int MediaPoolInit()
{
	if (!plugin_register("accelerator",&g_ar))
		return 0;
	if (!plugin_register("projectconfig",&g_projectconfig))
		return 0;

	SWSRegisterCommands(g_commandTable);
	g_pCommandTable = g_commandTable;

	if (!plugin_register("hookcustommenu", (void*)menuhook))
		return 0;

	g_pMediaPoolWnd = new SWS_MediaPoolWnd;

	return 1;
}

void MediaPoolExit()
{
	// Write the media "database"
	// Load files from the "database"
	char cBuf[512];
	strncpy(cBuf, get_ini_file(), 256);
	char* pC = strrchr(cBuf, PATH_SLASH_CHAR);
	if (pC)
	{
		strcpy(pC+1, "sws_mediapool.txt");
		FILE* f = fopen(cBuf, "w");
		if (f)
		{
			char cLine[512];
			WDL_PtrList<SWS_MediaPoolGroup>* pGroups = &g_pMediaPoolWnd->m_globalGroups;
			for (int i = 0; i < pGroups->GetSize(); i++)
			{
				sprintf(cLine, "<GROUP \"%s\"\n", pGroups->Get(i)->m_cGroupname);
				fputs(cLine, f);
				for (int j = 0; j < pGroups->Get(i)->m_files.GetSize(); j++)
					fputs(pGroups->Get(i)->m_files.Get(j)->GetString(cLine, 512), f);
				fputs(">\n", f);
			}
			fclose(f);
		}
	}

	delete g_pMediaPoolWnd;
}
