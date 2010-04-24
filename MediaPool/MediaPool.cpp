/******************************************************************************
/ MediaPool.cpp
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
#include "MediaPool.h"
#ifdef _WIN32
#include "DragDrop.h"
#endif
#include "../../WDL/projectcontext.h"

#define DELETE_MSG		0x10004
	
// Globals
static SWS_MediaPoolWnd* g_pMediaPoolWnd;

void InsertFile(const char* cFile)
{
	if (GetPlayState())
	{
		double dSavedPos = GetCursorPosition();
		SetEditCurPos(GetPlayPosition(), false, false);
		InsertMedia((char*)cFile, 0);
		SetEditCurPos(dSavedPos, false, false);
	}
	else
		InsertMedia((char*)cFile, 0);
}

void InsertFile(COMMAND_T* t)
{
	InsertFile((const char*)t->user);
}

SWS_MediaPoolFile::SWS_MediaPoolFile(const char* cFilename, int id): m_id(id), m_bAction(false), m_bActive(false)
{
	m_cFilename = new char[strlen(cFilename)+1];
	strcpy(m_cFilename, cFilename);
}

SWS_MediaPoolFile::~SWS_MediaPoolFile()
{
	if (m_bAction)
		UnregisterCommand();
	delete [] m_cFilename;
}

char* SWS_MediaPoolFile::GetString(char* str, int iStrMax)
{
	_snprintf(str, iStrMax, "\"%s\" %d %d", m_cFilename, m_id, m_bAction ? 1 : 0);
	return str;
}

void SWS_MediaPoolFile::SetAction(bool bAction, const char* cGroup, bool bActive)
{
	if ((bAction && bActive) != (m_bAction && m_bActive))
	{
		if (bAction && bActive)
			RegisterCommand(cGroup);
		else
			UnregisterCommand();
		m_bAction = bAction;
		m_bActive = bActive;
	}
}

void SWS_MediaPoolFile::RegisterCommand(const char* cGroup)
{
	COMMAND_T* cmd = new COMMAND_T;
	memset(&cmd->accel, 0, sizeof(cmd->accel));


	if (cGroup && cGroup[0])
	{
		const char* desc = "SWS: Insert file #%d from media pool group %s (%s)";
		cmd->accel.desc = new char[strlen(desc) + 5 + strlen(cGroup) + strlen(m_cFilename)];
		sprintf((char*)cmd->accel.desc, desc, m_id + 1, cGroup, m_cFilename);

		const char* idStr = "SWSMP_INSERT%s%d";
		cmd->id = new char[strlen(idStr) + strlen(cGroup) +  5];
		sprintf(cmd->id, idStr, cGroup, m_id);
	}
	else
	{
		const char* desc = "SWS: Insert %s from media pool";
		cmd->accel.desc = new char[strlen(desc) + strlen(m_cFilename)];
		sprintf((char*)cmd->accel.desc, desc, m_cFilename);

		const char* id = "SWSMP_";
		cmd->id = new char[strlen(id) + 41];
		strcpy(cmd->id, id);
		GetHashString(m_cFilename, cmd->id + strlen(id));
	}

	cmd->doCommand = InsertFile;
	cmd->menuText = NULL;
	cmd->user = (int)m_cFilename;
	cmd->getEnabled = NULL;
	SWSRegisterCommand(cmd);
}

void SWS_MediaPoolFile::UnregisterCommand()
{
	int id = SWSGetCommandID(InsertFile, (int)m_cFilename);
	if (id)
	{
		COMMAND_T* cmd = SWSUnregisterCommand(id);
		if (cmd)
		{
			delete [] cmd->accel.desc;
			delete [] cmd->id;
			delete cmd;
		}
	}
}

SWS_MediaPoolGroup::SWS_MediaPoolGroup(const char* cName, bool bGlobal):m_cGroupname(NULL), m_bGlobal(bGlobal)
{
	SetName(cName);
}

void SWS_MediaPoolGroup::AddFile(const char* cFile, int id, bool bAction)
{
	// Don't add dupes
	for (int i = 0; i < m_files.GetSize(); i++)
		if (_stricmp(cFile, m_files.Get(i)->GetFilename()) == 0)
			return;

	SWS_MediaPoolFile* pFile = m_files.Add(new SWS_MediaPoolFile(cFile, id));
	pFile->SetAction(bAction, m_bGlobal ? NULL : m_cGroupname, m_bGlobal);
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

void SWS_MediaPoolGroup::SetActive(bool bActive)
{
	for (int i = 0; i < m_files.GetSize(); i++)
		m_files.Get(i)->SetAction(m_files.Get(i)->GetAction(), m_bGlobal ? NULL : m_cGroupname, bActive);
}

void SWS_MediaPoolGroup::PopulateFromReaper()
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* mi = GetTrackMediaItem(tr, j);
			if (mi)
			{
				for (int k = 0; k < GetMediaItemNumTakes(mi); k++)
				{
					MediaItem_Take* mit = GetMediaItemTake(mi, k);
					if (mit)
					{
						PCM_source* src = (PCM_source*)GetSetMediaItemTakeInfo(mit, "P_SOURCE", NULL);
						bool bMIDI = false;
						if (src)
						{
							const char* cFilename = src->GetFileName();
							if (strcmp(src->GetType(), "SECTION") != 0)
							{
								if (strcmp(src->GetType(), "MIDI") == 0 || !cFilename || strcmp(cFilename, "") == 0)
									bMIDI = true;
							}
							else
							{
								PCM_source *otherSrc = src->GetSource();
								if (otherSrc)
									cFilename = otherSrc->GetFileName();
								else
									bMIDI = true;
							}
							if (!bMIDI && cFilename && cFilename[0])
							{
								int L;
								for (L = 0; L < m_files.GetSize(); L++)
									if (strcmp(cFilename, m_files.Get(L)->GetFilename()) == 0)
										break;
								if (L == m_files.GetSize())
									m_files.Add(new SWS_MediaPoolFile(cFilename, 0));
								else
									m_files.Get(L)->SetID(m_files.Get(L)->GetID() + 1);
							}
						}
					}
				}
			}
		}
	}
}

SWS_MediaPoolGroup::~SWS_MediaPoolGroup()
{
	m_files.Empty(true);
	delete [] m_cGroupname;
}

static SWS_LVColumn g_gvCols[] = { { 75, 1, "Group" }, { 25, 0, "#" }, { 44, 2, "Global" } };

SWS_MediaPoolGroupView::SWS_MediaPoolGroupView(HWND hwndList, HWND hwndEdit, SWS_MediaPoolWnd* pWnd)
:SWS_ListView(hwndList, hwndEdit, 3, g_gvCols, "MediaPool Group State", false), m_pWnd(pWnd)
{
}

void SWS_MediaPoolGroupView::DoDelete()
{
	for (int i = 0; i < ListView_GetItemCount(m_hwndList); i++)
		if (IsSelected(i))
		{
			SWS_MediaPoolGroup* pGroup = (SWS_MediaPoolGroup*)GetListItem(i);
			if (pGroup)
			{
				if (pGroup->m_bGlobal)
				{
					int index = m_pWnd->m_globalGroups.Find(pGroup);
					if (index >= 0)
						m_pWnd->m_globalGroups.Delete(index, true);
				}
				else
				{
					int index = m_pWnd->m_projGroups.Get()->Find(pGroup);
					if (index >= 0)
						m_pWnd->m_projGroups.Get()->Delete(index, true);
				}
				m_pWnd->m_curGroup = &m_pWnd->m_projGroup;
			}
		}
}

void SWS_MediaPoolGroupView::SetItemText(LPARAM item, int iCol, const char* str)
{
	if (item && iCol == 0)
		((SWS_MediaPoolGroup*)item)->SetName(str);
}

void SWS_MediaPoolGroupView::GetItemText(LPARAM item, int iCol, char* str, int iStrMax)
{
	SWS_MediaPoolGroup* pGroup = (SWS_MediaPoolGroup*)item;
	switch (iCol)
	{
	case 0: // Name
		lstrcpyn(str, pGroup->m_cGroupname, iStrMax);
		break;
	case 1: // # of files
		_snprintf(str, iStrMax, "%d", pGroup->m_files.GetSize());
		break;
	case 2: // Global flag
		if (pGroup != &m_pWnd->m_projGroup)
			lstrcpyn(str, pGroup->m_bGlobal ? UTF8_BULLET : UTF8_CIRCLE, iStrMax);
		else
			str[0] = 0;
		break;
	}
}

void SWS_MediaPoolGroupView::OnItemBtnClk(LPARAM item, int iCol, int iKeyState)
{
	SWS_MediaPoolGroup* pGroup = (SWS_MediaPoolGroup*)item;
	if (pGroup && pGroup != &m_pWnd->m_projGroup && iCol == 2)
	{
		// Switch to/from global
		if (pGroup->m_bGlobal)
		{
			int index = m_pWnd->m_globalGroups.Find(pGroup);
			m_pWnd->m_globalGroups.Delete(index);
			m_pWnd->m_projGroups.Get()->Add(pGroup);
		}
		else
		{
			int index = m_pWnd->m_projGroups.Get()->Find(pGroup);
			m_pWnd->m_projGroups.Get()->Delete(index);
			m_pWnd->m_globalGroups.Add(pGroup);
		}
		pGroup->m_bGlobal = !pGroup->m_bGlobal;
		for (int i = 0; i < pGroup->m_files.GetSize(); i++)
		{
			pGroup->m_files.Get(i)->SetAction(pGroup->m_files.Get(i)->GetAction(), pGroup->m_bGlobal ? NULL : pGroup->m_cGroupname, false);
			pGroup->m_files.Get(i)->SetAction(pGroup->m_files.Get(i)->GetAction(), pGroup->m_bGlobal ? NULL : pGroup->m_cGroupname, true);
		}

		// TODO how to handle UNDO for global states??
		//Undo_OnStateChangeEx("Media pool global switch", UNDO_STATE_MISCCFG, -1);

		Update();
	}
}

void SWS_MediaPoolGroupView::OnItemSelChanged(LPARAM item, int iState)
{
	SWS_MediaPoolGroup* group = (SWS_MediaPoolGroup*)item;
	if (iState & LVIS_FOCUSED && m_pWnd->m_curGroup != group)
	{
		m_pWnd->m_curGroup = group;
		m_pWnd->Update();
	}
	else if (!(iState * LVIS_FOCUSED) && m_pWnd->m_curGroup == group)
	{
		m_pWnd->m_curGroup = NULL;
		m_pWnd->Update();
	}
}

void SWS_MediaPoolGroupView::GetItemList(WDL_TypedBuf<LPARAM>* pBuf)
{
	//     curproj + globals                          + project groups
	pBuf->Resize(1 + m_pWnd->m_globalGroups.GetSize() + m_pWnd->m_projGroups.Get()->GetSize());
	pBuf->Get()[0] = (LPARAM)&m_pWnd->m_projGroup;
	int iBuf = 1;

	for (int i = 0; i < m_pWnd->m_globalGroups.GetSize(); i++)
		pBuf->Get()[iBuf++] = (LPARAM)m_pWnd->m_globalGroups.Get(i);
	for (int i = 0; i < m_pWnd->m_projGroups.Get()->GetSize(); i++)
		pBuf->Get()[iBuf++] = (LPARAM)m_pWnd->m_projGroups.Get()->Get(i);
}

int SWS_MediaPoolGroupView::GetItemState(LPARAM item)
{
	return (SWS_MediaPoolGroup*)item == m_pWnd->m_curGroup ? 1 : 0;
}

static SWS_LVColumn g_fvCols[] = { { 25, 0, "#" }, { 300, 0, "Path" }, { 200, 0, "Filename" }, { 45, 2, "Action" } };

SWS_MediaPoolFileView::SWS_MediaPoolFileView(HWND hwndList, HWND hwndEdit, SWS_MediaPoolWnd* pWnd)
:SWS_ListView(hwndList, hwndEdit, 4, g_fvCols, "MediaPool FileList State", false), m_pWnd(pWnd)
{
}

void SWS_MediaPoolFileView::DoDelete()
{
	if (m_pWnd->m_curGroup)
	{
		for (int i = 0; i < ListView_GetItemCount(m_hwndList); i++)
			if (IsSelected(i))
			{
				SWS_MediaPoolFile* pFile = (SWS_MediaPoolFile*)GetListItem(i);
				int index = m_pWnd->m_curGroup->m_files.Find(pFile);
				if (index >= 0)
					m_pWnd->m_curGroup->m_files.Delete(index, true);
			}
	}
}


void SWS_MediaPoolFileView::GetItemText(LPARAM item, int iCol, char* str, int iStrMax)
{
	SWS_MediaPoolFile* pFile = (SWS_MediaPoolFile*)item;
	switch (iCol)
	{
		case 0: // #
			_snprintf(str, iStrMax, "%d", pFile->GetID() + 1);
			break;
		case 1: // Path
		{
			lstrcpyn(str, pFile->GetFilename(), iStrMax);
			char* pSlash = strrchr(str, PATH_SLASH_CHAR);
			if (pSlash)
				*pSlash = 0;
			break;
		}
		case 2: // Filename
		{
			const char* pSlash = strrchr(pFile->GetFilename(), PATH_SLASH_CHAR);
			if (pSlash)
				lstrcpyn(str, pSlash+1, iStrMax);
			else
				lstrcpyn(str, pFile->GetFilename(), iStrMax);
			break;
		}
		case 3:
			if (m_pWnd->m_curGroup != &m_pWnd->m_projGroup)
				lstrcpyn(str, pFile->GetAction() ? UTF8_BULLET : UTF8_CIRCLE, iStrMax);
			else
				str[0] = 0;
			break;
	}
}

void SWS_MediaPoolFileView::OnItemBtnClk(LPARAM item, int iCol, int iKeyState)
{
	if (iCol == 3 && m_pWnd->m_curGroup != &m_pWnd->m_projGroup)
	{
		SWS_MediaPoolFile* pFile = (SWS_MediaPoolFile*)item;
		bool bNewState = !pFile->GetAction();
		for (int i = 0; i < ListView_GetItemCount(m_hwndList); i++)
			if (IsSelected(i))
			{
				SWS_MediaPoolFile* pFile = (SWS_MediaPoolFile*)GetListItem(i);
				pFile->SetAction(bNewState, m_pWnd->m_curGroup->m_bGlobal ? NULL : m_pWnd->m_curGroup->m_cGroupname, true);
			}
		Update();
	}
}

void SWS_MediaPoolFileView::OnItemSelChanged(LPARAM item, int iState)
{
	// TODO update the info text
}

void SWS_MediaPoolFileView::OnItemDblClk(LPARAM item, int iCol)
{
	// Insert the file
	InsertFile((char*)((SWS_MediaPoolFile*)item)->GetFilename());
}

void SWS_MediaPoolFileView::GetItemList(WDL_TypedBuf<LPARAM>* pBuf)
{
	int iSize = 0;
	if (m_pWnd->m_curGroup)
		iSize = m_pWnd->m_curGroup->m_files.GetSize();
	pBuf->Resize(iSize);
	for (int i = 0; i < iSize; i++)
		pBuf->Get()[i] = (LPARAM)m_pWnd->m_curGroup->m_files.Get(i);
}

void SWS_MediaPoolFileView::OnBeginDrag()
{
#ifdef _WIN32
	LVITEM li;
	li.mask = LVIF_STATE | LVIF_PARAM;
	li.stateMask = LVIS_SELECTED;
	li.iSubItem = 0;
	
	// Get the amount of memory needed for the file list
	int iMemNeeded = 0;
	for (int i = 0; i < ListView_GetItemCount(m_hwndList); i++)
	{
		li.iItem = i;
		ListView_GetItem(m_hwndList, &li);
		if (li.state & LVIS_SELECTED)
			iMemNeeded += (int)strlen(((SWS_MediaPoolFile*)li.lParam)->GetFilename()) + 1;
	}
	if (!iMemNeeded)
		return;

	iMemNeeded += sizeof(DROPFILES) + 1;

	HGLOBAL hgDrop = GlobalAlloc (GHND | GMEM_SHARE, iMemNeeded);
	DROPFILES* pDrop = (DROPFILES*)GlobalLock(hgDrop); // 'spose should do some error checking...
	pDrop->pFiles = sizeof(DROPFILES);
	pDrop->fWide = false;
	char* pBuf = (char*)pDrop + pDrop->pFiles;

	// Add the files to the DROPFILES struct, double-NULL terminated
	for (int i = 0; i < ListView_GetItemCount(m_hwndList); i++)
	{
		li.iItem = i;
		ListView_GetItem(m_hwndList, &li);
		if (li.state & LVIS_SELECTED)
		{
			strcpy(pBuf, ((SWS_MediaPoolFile*)li.lParam)->GetFilename());
			pBuf += strlen(pBuf) + 1;
		}
	}
	*pBuf = 0;
	GlobalUnlock(hgDrop);
	FORMATETC etc = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	STGMEDIUM stgmed = { TYMED_HGLOBAL, { 0 }, 0 };
	stgmed.hGlobal = hgDrop;

	SWS_IDataObject* dataObj = new SWS_IDataObject(&etc, &stgmed);
	SWS_IDropSource* dropSrc = new SWS_IDropSource;
	DWORD effect;

	DoDragDrop(dataObj, dropSrc, DROPEFFECT_COPY, &effect);
#endif
}

SWS_MediaPoolWnd::SWS_MediaPoolWnd()
:SWS_DockWnd(IDD_MEDIAPOOL, "Media Pool", 30004), m_curGroup(NULL), m_projGroup("Project", false)
{
	// Load files from the "database"
	char cBuf[512];
	strncpy(cBuf, get_ini_file(), 256);
	char* pC = strrchr(cBuf, PATH_SLASH_CHAR);
	if (pC)
	{
		strcpy(pC+1, "sws_mediapool.txt");
		ProjectStateContext* mediaPool = ProjectCreateFileRead(cBuf);
		if (mediaPool)
		{
			int iGroup = -1;
			while(!mediaPool->GetLine(cBuf, 512))
			{
				LineParser lp(false);
				if (!lp.parse(cBuf) && lp.getnumtokens())
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
			delete mediaPool;
		}
	}

	m_curGroup = &m_projGroup;

	if (m_bShowAfterInit)
		Show(false, false);
}

void SWS_MediaPoolWnd::Update()
{
	static bool bRecurseCheck = false;
	if (!IsValidWindow() || bRecurseCheck || !m_pLists.GetSize())
		return;
	bRecurseCheck = true;
	// Disable the file view if there's no group selected
	EnableWindow(m_pLists.Get(1)->GetHWND(), m_curGroup ? true : false);

	// Update the active current project
	static ReaProject* curProject = NULL;
	if (curProject != Enum_Projects(-1, NULL, 0))
	{
		curProject = Enum_Projects(-1, NULL, 0);
		for (int i = 0; i < m_projGroups.GetNumProj(); i++)
			for (int j = 0; j < m_projGroups.Get(i)->GetSize(); j++)
			{
				m_projGroups.Get(i)->Get(j)->SetActive(false);
				if (m_curGroup == m_projGroups.Get(i)->Get(j))
					m_curGroup = &m_projGroup;
			}
		for (int i = 0; i < m_projGroups.Get()->GetSize(); i++)
			m_projGroups.Get()->Get(i)->SetActive(true);
	}

	if (m_curGroup == &m_projGroup)
	{
		m_projGroup.m_files.Empty(true);
		// TODO does this take way too long?
		// Add all project files
		m_projGroup.PopulateFromReaper();
	}

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
			if (!m_curGroup || m_curGroup == &m_projGroup)
			{
				MessageBox(m_hwnd, "Please select or add a group first (besides \"Project\").", "Media Pool Error", MB_OK);
			}
			else
			{
				char projPath[MAX_PATH];
				GetProjectPath(projPath, MAX_PATH);
				char* pFiles = BrowseForFiles("Select file(s)", projPath, NULL, true, plugin_getFilterList());
				if (pFiles)
				{
					char* pBuf = pFiles;
					bool bUndo = false;
					while(pFiles[0])
					{
						char* pExt = strrchr(pFiles, '.');
						if (pExt && IsMediaExtension(pExt+1, false))
						{
							m_curGroup->AddFile(pFiles, m_curGroup->m_files.GetSize(), false);
							bUndo = !m_curGroup->m_bGlobal;
						}
						pFiles += strlen(pFiles) + 1;
					}
					free(pBuf);
					if (bUndo)
						Undo_OnStateChangeEx("Add files to project media pool", UNDO_STATE_MISCCFG, -1);
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
		case IDC_PREVIEW | (BN_CLICKED << 16):
			// TODO preview media
			MessageBox(m_hwnd, "Not yet implemented.", "Media Pool Error", MB_OK);
			break;
		case DELETE_MSG:
			if (m_curGroup != &m_projGroup)
			{
				if (m_pLists.Get(0)->IsActive(false))
				{
					SWS_MediaPoolGroupView* pGV = (SWS_MediaPoolGroupView*)m_pLists.Get(0);
					pGV->DoDelete();
				}
				else if (m_pLists.Get(1)->IsActive(false))
				{
					SWS_MediaPoolFileView* pFV = (SWS_MediaPoolFileView*)m_pLists.Get(1);
					pFV->DoDelete();
				}
				Update();
			}
			break;
	}
}

void SWS_MediaPoolWnd::OnDroppedFiles(HDROP h)
{
	// Check to see if we dropped on a group
	POINT pt;
	DragQueryPoint(h, &pt);

	RECT r; // ClientToScreen doesn't work right, wtf?
	GetWindowRect(m_hwnd, &r);
	pt.x += r.left;
	pt.y += r.top;
	SWS_MediaPoolGroup* pGroup = (SWS_MediaPoolGroup*)m_pLists.Get(0)->GetHitItem(pt.x, pt.y, NULL);
	if (pGroup)
		m_curGroup = pGroup;
	
	if (!m_curGroup || m_curGroup == &m_projGroup)
	{
		MessageBox(m_hwnd, "Please select or add a group first (besides \"Project\").", "Media Pool Error", MB_OK);
	}
	else
	{
		char cFile[512];
		int iFiles = DragQueryFile(h, 0xFFFFFFFF, NULL, 0);
		bool bUndo = false;
		for (int i = 0; i < iFiles; i++)
		{
			DragQueryFile(h, i, cFile, 512);
			char* pExt = strrchr(cFile, '.');
			if (pExt && IsMediaExtension(pExt+1, false))
			{
				m_curGroup->AddFile(cFile, m_curGroup->m_files.GetSize(), false);
				bUndo = !m_curGroup->m_bGlobal;
			}
		}
		if (bUndo)
			Undo_OnStateChangeEx("Add files to project media pool", UNDO_STATE_MISCCFG, -1);
		Update();
	}

	DragFinish(h);
}

HMENU SWS_MediaPoolWnd::OnContextMenu(int x, int y)
{
	// TODO ?
	return NULL;
	//HMENU contextMenu = CreatePopupMenu();
	//return contextMenu;
}

int SWS_MediaPoolWnd::OnKey(MSG* msg, int iKeyState)
{
	if (msg->message == WM_KEYDOWN && msg->wParam == VK_DELETE && !iKeyState)
	{
		OnCommand(DELETE_MSG, 0);
		return 1;
	}
	return 0;
}

void OpenMediaPool(COMMAND_T*)
{
	g_pMediaPoolWnd->Show(true, true);
}

void MediaPoolUpdate()
{
	g_pMediaPoolWnd->Update();
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
			else if (lp.gettoken_str(0)[0] == '>')
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

static bool MediaPoolEnabled(COMMAND_T*)
{
	return g_pMediaPoolWnd->IsValidWindow();
}

static COMMAND_T g_commandTable[] =
{
	{ { DEFACCEL, "SWS: Show media pool" }, "SWSMP_OPEN", OpenMediaPool, "SWS Media pool", 0, MediaPoolEnabled },
	{ {}, LAST_COMMAND, }, // Denote end of table
};

static void menuhook(const char* menustr, HMENU hMenu, int flag)
{
	if (strcmp(menustr, "Main view") == 0 && flag == 0)
		AddToMenu(hMenu, g_commandTable[0].menuText, g_commandTable[0].accel.accel.cmd, 40075);
}

int MediaPoolInit()
{
	if (!plugin_register("projectconfig",&g_projectconfig))
		return 0;

	SWSRegisterCommands(g_commandTable);

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
		ProjectStateContext* mediaPool = ProjectCreateFileWrite(cBuf);
		if (mediaPool)
		{
			char cLine[512];
			WDL_PtrList<SWS_MediaPoolGroup>* pGroups = &g_pMediaPoolWnd->m_globalGroups;
			for (int i = 0; i < pGroups->GetSize(); i++)
			{
				mediaPool->AddLine("<GROUP \"%s\"", pGroups->Get(i)->m_cGroupname);
				for (int j = 0; j < pGroups->Get(i)->m_files.GetSize(); j++)
					mediaPool->AddLine(pGroups->Get(i)->m_files.Get(j)->GetString(cLine, 512));
				mediaPool->AddLine(">");
			}
			delete mediaPool;
		}
	}

	delete g_pMediaPoolWnd;
}
