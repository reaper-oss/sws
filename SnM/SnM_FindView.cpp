/******************************************************************************
/ SnM_FindView.cpp
/
/ Copyright (c) 2010-2011 Tim Payne (SWS), Jeffos
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
#include "SnM_Actions.h"
#include "SNM_FindView.h"
#include "../Zoom.h"

#define MAX_SEARCH_STR_LEN		128
//#define ICON_BUTTONS // use icon buttons? => text buttons if undefined (transport buttons can look bad with some themes..)

enum {
  BUTTONID_FIND=1000,
  BUTTONID_PREV,
  BUTTONID_NEXT,
  BUTTONID_ZOOM_SCROLL_EN,
  COMBOID_TYPE,
  TXTID_RESULT
};

enum {
	TYPE_ITEM_NAME=0,
	TYPE_ITEM_NAME_ALL_TAKES,
	TYPE_ITEM_FILENAME,
	TYPE_ITEM_FILENAME_ALL_TAKES,
	TYPE_ITEM_NOTES,
	TYPE_TRACK_NAME,
	TYPE_TRACK_NOTES,
	TYPE_MARKER_REGION
};

// Globals
static SNM_FindWnd* g_pFindWnd = NULL;
char g_searchStr[MAX_SEARCH_STR_LEN] = "";
bool g_notFound=false;

bool TakeNameMatch(MediaItem_Take* _tk, const char* _searchStr)
{
	char* takeName = _tk ? (char*)GetSetMediaItemTakeInfo(_tk, "P_NAME", NULL) : NULL;
	return (takeName && stristr(takeName, _searchStr));
}

bool TakeFilenameMatch(MediaItem_Take* _tk, const char* _searchStr)
{
	bool match = false;
	PCM_source* src = _tk ? (PCM_source*)GetSetMediaItemTakeInfo(_tk, "P_SOURCE", NULL) : NULL;
	if (src) 
	{
		const char* takeFilename = src->GetFileName();
		match = (takeFilename && stristr(takeFilename, _searchStr));
	}
	return match;
}

bool ItemNotesMatch(MediaItem* _item, const char* _searchStr)
{
	bool match = false;
	if (_item)
	{
		SNM_ChunkParserPatcher p(_item);
		WDL_FastString notes;
		if (p.GetSubChunk("NOTES", 2, 0, &notes, "VOLPAN") >= 0) // rmk: we use VOLPAN as it also exists for empty items
			//JFB TODO? we compare a formated string with a normal one here, oh well..
			match = (stristr(notes.Get(), _searchStr) != NULL);
	}
	return match;
}

bool TrackNameMatch(MediaTrack* _tr, const char* _searchStr) {
	char* name = _tr ? (char*)GetSetMediaTrackInfo(_tr, "P_NAME", NULL) : NULL;
	return (name && stristr(name, _searchStr));
}

bool TrackNotesMatch(MediaTrack* _tr, const char* _searchStr) 
{
	bool match = false;
	if (_tr)
	{
		for (int i=0; i < g_pTracksNotes.Get()->GetSize(); i++)
		{
			if (g_pTracksNotes.Get()->Get(i)->m_tr == _tr)
			{
				match = (stristr(g_pTracksNotes.Get()->Get(i)->m_notes.Get(), _searchStr) != NULL);
				break;
			}
		}
	}
	return match;
}


///////////////////////////////////////////////////////////////////////////////
// SNM_FindWnd
///////////////////////////////////////////////////////////////////////////////

SNM_FindWnd::SNM_FindWnd()
:SWS_DockWnd(IDD_SNM_FIND, "Find", "SnMFind", 30008, SWSGetCommandID(OpenFindView))
{
	m_type = 0;
	m_zoomSrollItems = false;

	// Must call SWS_DockWnd::Init() to restore parameters and open the window if necessary
	Init();
}

bool SNM_FindWnd::Find(int _mode)
{
	bool update = false;
	switch(m_type)
	{
		case TYPE_ITEM_NAME:
			update = FindMediaItem(_mode, false, TakeNameMatch);
		break;
		case TYPE_ITEM_NAME_ALL_TAKES:
			update = FindMediaItem(_mode, true, TakeNameMatch);
		break;
		case TYPE_ITEM_FILENAME:
			update = FindMediaItem(_mode, false, TakeFilenameMatch);
		break;
		case TYPE_ITEM_FILENAME_ALL_TAKES:
			update = FindMediaItem(_mode, true, TakeFilenameMatch);
		break;
		case TYPE_ITEM_NOTES:
			update = FindMediaItem(_mode, false, NULL, ItemNotesMatch);
		break;
		case TYPE_TRACK_NAME:
			update = FindTrack(_mode, TrackNameMatch);
		break;
		case TYPE_TRACK_NOTES:
			update = FindTrack(_mode, TrackNotesMatch);
		break;
		case TYPE_MARKER_REGION:
			update = FindMarkerRegion(_mode);
	}
	return update;
}

MediaItem* SNM_FindWnd::FindPrevNextItem(int _dir, MediaItem* _item)
{
	if (!_dir)
		return NULL;

	MediaItem* previous = NULL;
	int startTrIdx = (_dir == -1 ? CountTracks(NULL) : 1);
	if (_item)
		startTrIdx = CSurf_TrackToID(GetMediaItem_Track(_item), false);

	bool found = (_item == NULL);
	for (int i = startTrIdx; !previous && i <= CountTracks(NULL) && i >= 1; i+=_dir)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false); 
		int nbItems = GetTrackNumMediaItems(tr);
		for (int j = (_dir > 0 ? 0 : (nbItems-1)); j < nbItems && j >= 0; j+=_dir)
		{
			MediaItem* item = GetTrackMediaItem(tr,j);
			if (found && item) {
				previous = item;
				break;
			}
			if (_item && item == _item)
				found = true;
		}
	}
	return previous;
}

// param _allTakes only makes sense if jobTake() is used
bool SNM_FindWnd::FindMediaItem(int _dir, bool _allTakes, bool (*jobTake)(MediaItem_Take*,const char*), bool (*jobItem)(MediaItem*,const char*))
{
	bool update = false, found = false, sel = true;
	if (g_searchStr && *g_searchStr)
	{
		MediaItem* startItem = NULL;
		bool clearCurrentSelection = false;
		if (_dir)
		{
			if (CountSelectedMediaItems(NULL))
			{
				startItem = FindPrevNextItem(_dir, GetSelectedMediaItem(NULL, _dir > 0 ? 0 : CountSelectedMediaItems(NULL) - 1));
				clearCurrentSelection = (startItem != NULL); 
			}
			else
				startItem = FindPrevNextItem(_dir, NULL);
		}
		else
		{
			startItem = FindPrevNextItem(1, NULL);
			clearCurrentSelection = (startItem != NULL); 
		}

		if (clearCurrentSelection)
		{
			Undo_BeginBlock();
			Main_OnCommand(40289,0); // unselect all items
			update = true;
		}

		MediaItem* item = NULL;
		if (startItem)
		{
			MediaTrack* startTr = GetMediaItem_Track(startItem);
			int startTrIdx = CSurf_TrackToID(startTr, false);

			// find startItem idx
			int startItemIdx=-1;
			while (item != startItem) 
				item = GetTrackMediaItem(startTr,++startItemIdx);

			bool firstItem = true, breakSelection = false;
			for (int i = startTrIdx; !breakSelection && i <= CountTracks(NULL) && i>=1; i += (!_dir ? 1 : _dir))
			{
				MediaTrack* tr = CSurf_TrackFromID(i, false); 
				int nbItems = GetTrackNumMediaItems(tr);
				for (int j = (firstItem ? startItemIdx : (_dir >= 0 ? 0 : (nbItems-1))); 
					 tr && !breakSelection && j < nbItems && j >= 0; 
					 j += (!_dir ? 1 : _dir))
				{
					item = GetTrackMediaItem(tr,j);
					firstItem = false;

					// search at item level 
					if (jobItem)
					{
						if (jobItem(item, g_searchStr))
						{
							if (!update) Undo_BeginBlock();
							update = found = true;
							GetSetMediaItemInfo(item, "B_UISEL", &sel);
							if (_dir) breakSelection = true;
						}
					}
					// search at take level 
					else if (jobTake)
					{
						int nbTakes = GetMediaItemNumTakes(item);
						for (int k=0; item && k < nbTakes; k++)
						{
							MediaItem_Take* tk = GetMediaItemTake(item, k);
							if (tk && (_allTakes || (!_allTakes && tk == GetActiveTake(item))))
							{
								if (jobTake(tk, g_searchStr))
								{
									if (!update) Undo_BeginBlock();
									update = found = true;
									GetSetMediaItemInfo(item, "B_UISEL", &sel);
									if (_dir) {
										breakSelection = true;
										break;
									}
								}
							}
						}
					}
				}
			}
		}

		UpdateNotFoundMsg(found);
		if (found && m_zoomSrollItems)
		{
			if (!_dir)
				 ZoomToSelItems();
			else if (item)
				scrollToSelItem(item);
		}
	}

	if (update)
	{
		UpdateTimeline();
		Undo_EndBlock("Find: change media item selection", UNDO_STATE_ALL);
	}
	return update;
}

bool SNM_FindWnd::FindTrack(int _dir, bool (*job)(MediaTrack*,const char*))
{
	bool update = false, found = false;
	if (g_searchStr && *g_searchStr)
	{
		int startTrIdx = -1;
		bool clearCurrentSelection = false;
		if (_dir)
		{
			int selTracksCount = CountSelectedTracksWithMaster(NULL);
			if (selTracksCount)
			{
				MediaTrack* startTr = GetSelectedTrackWithMaster(NULL, _dir > 0 ? 0 : selTracksCount - 1);
				int id = CSurf_TrackToID(startTr, false);
				if ((_dir > 0 && id < CountTracks(NULL)) || (_dir < 0 && id >0))
				{
					startTrIdx = id + _dir;
					clearCurrentSelection = true;
				}
			}
			else
				startTrIdx = (_dir > 0 ? 0 : CountTracks(NULL));
		}
		else
		{
			startTrIdx = 0;
			clearCurrentSelection = true;
		}

		if (clearCurrentSelection)
		{
			Undo_BeginBlock();
			Main_OnCommand(40297,0); // unselect all tracks
			update = true;
		}

		if (startTrIdx >= 0)
		{
			for (int i = startTrIdx; i <= CountTracks(NULL) && i>=0; i += (!_dir ? 1 : _dir))
			{
				MediaTrack* tr = CSurf_TrackFromID(i, false); 
				if (tr && job(tr, g_searchStr))
				{
					if (!update)
						Undo_BeginBlock();

					update = found = true;
					GetSetMediaTrackInfo(tr, "I_SELECTED", &g_i1);
					if (_dir) 
						break;
				}
			}
		}

		UpdateNotFoundMsg(found);	
		if (found)
			Main_OnCommand(40913,0); // scroll to selected tracks
	}

	if (update)
		Undo_EndBlock("Find: change track selection", UNDO_STATE_ALL);

	return update;
}

bool SNM_FindWnd::FindMarkerRegion(int _dir)
{
	if (!_dir)
		return false;

	bool update = false, found = false;
	if (g_searchStr && *g_searchStr)
	{
		double startPos = GetCursorPosition();

		int id, x = 0;
		bool bR;
		double dPos, dRend, dMinMaxPos = _dir < 0 ? -DBL_MAX : DBL_MAX;
		char *cName;
		while ((x=EnumProjectMarkers2(NULL, x, &bR, &dPos, &dRend, &cName, &id)))
		{
			if (_dir == 1 && dPos > startPos) {
				if (stristr(cName, g_searchStr)) {
					found = true;
					dMinMaxPos = min(dPos, dMinMaxPos);
				}
			}
			else if (_dir == -1 && dPos < startPos) {
				if (stristr(cName, g_searchStr)) {
					found = true;
					dMinMaxPos = max(dPos, dMinMaxPos);
				}
			}
		}

		UpdateNotFoundMsg(found);	
		if (found) {
			SetEditCurPos(dMinMaxPos, true, false);
			update = true;
		}
	}

	if (update)
		Undo_OnStateChangeEx("Find: change cursor position", UNDO_STATE_ALL, -1);

	return update;
}

void SNM_FindWnd::UpdateNotFoundMsg(bool _found)
{
	g_notFound = !_found;
	m_parentVwnd.RequestRedraw(NULL);
}

void SNM_FindWnd::OnInitDlg()
{
	m_resize.init_item(IDC_EDIT, 0.0, 0.0, 1.0, 0.0);
	SetWindowLongPtr(GetDlgItem(m_hwnd, IDC_EDIT), GWLP_USERDATA, 0xdeadf00b);

	// Load prefs 
	m_type = GetPrivateProfileInt("FIND_VIEW", "Type", 0, g_SNMiniFilename.Get());
	m_zoomSrollItems = (GetPrivateProfileInt("FIND_VIEW", "ZoomScrollToFoundItems", 0, g_SNMiniFilename.Get()) == 1);

	// WDL GUI init
	m_vwnd_painter.SetGSC(WDL_STYLE_GetSysColor);
    m_parentVwnd.SetRealParent(m_hwnd);

	m_btnEnableZommScroll.SetID(BUTTONID_ZOOM_SCROLL_EN);
	m_btnEnableZommScroll.SetCheckState(m_zoomSrollItems);
	m_parentVwnd.AddChild(&m_btnEnableZommScroll);

	m_btnFind.SetID(BUTTONID_FIND);
	m_parentVwnd.AddChild(&m_btnFind);

	m_btnPrev.SetID(BUTTONID_PREV);
	m_parentVwnd.AddChild(&m_btnPrev);

	m_btnNext.SetID(BUTTONID_NEXT);
	m_parentVwnd.AddChild(&m_btnNext);

	m_cbType.SetID(COMBOID_TYPE);
	m_cbType.AddItem("Find in item names");
	m_cbType.AddItem("Find in item names (all takes)");
	m_cbType.AddItem("Find in media filenames");
	m_cbType.AddItem("Find in media filenames (all takes)");
	m_cbType.AddItem("Find in item notes");
	m_cbType.AddItem("Find in track names");
	m_cbType.AddItem("Find in track notes");
	m_cbType.AddItem("Find in marker/region names");
	m_cbType.SetCurSel(m_type);
	m_parentVwnd.AddChild(&m_cbType);

	m_txtResult.SetID(TXTID_RESULT);
	m_txtResult.SetColors(LICE_RGBA(170,0,0,255));
	m_parentVwnd.AddChild(&m_txtResult);

	g_notFound = false;
//	*g_searchStr = 0;
	SetDlgItemText(m_hwnd, IDC_EDIT, g_searchStr);

	m_parentVwnd.RequestRedraw(NULL);
}

void SNM_FindWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	// WDL GUI
	switch(wParam)
	{
		case (IDC_EDIT | (EN_CHANGE << 16)):
		{
			GetDlgItemText(m_hwnd, IDC_EDIT, g_searchStr, MAX_SEARCH_STR_LEN);
			UpdateNotFoundMsg(true);
			break;
		}
		default:
		{
			WORD hiwParam = HIWORD(wParam);
			if (!hiwParam || hiwParam == 600) // 600 for large tick box
			{
				switch(LOWORD(wParam))
				{
					case BUTTONID_ZOOM_SCROLL_EN: m_zoomSrollItems = !m_zoomSrollItems; break;
					case BUTTONID_FIND: Find(0); break;
					case BUTTONID_PREV: Find(-1); break;
					case BUTTONID_NEXT: Find(1); break;
				}
			}
			else if (HIWORD(wParam)==CBN_SELCHANGE && LOWORD(wParam)==COMBOID_TYPE) 
			{
				m_type = m_cbType.GetCurSel();
				UpdateNotFoundMsg(true); // + redraw
				SetFocus(GetDlgItem(m_hwnd, IDC_EDIT));
			}
			else 
				Main_OnCommand((int)wParam, (int)lParam);
			break;
		}
	}
}

void SNM_FindWnd::OnDestroy() 
{
	// save prefs
	char cType[2];
	sprintf(cType, "%d", m_type);
	WritePrivateProfileString("FIND_VIEW", "Type", cType, g_SNMiniFilename.Get());
	WritePrivateProfileString("FIND_VIEW", "ZoomScrollToFoundItems", m_zoomSrollItems ? "1" : "0", g_SNMiniFilename.Get());

	m_cbType.Empty();
	m_parentVwnd.RemoveAllChildren(false);
	m_parentVwnd.SetRealParent(NULL);

	g_notFound = false;
//	*g_searchStr = 0;
}

int SNM_FindWnd::OnKey(MSG* _msg, int _iKeyState) 
{
	if (_msg->message == WM_KEYDOWN && _msg->wParam == VK_F3)
	{
		// F3: find next
		if (!_iKeyState) {
				Find(1); 
				return 1;
		}
		// Shift-F3: find previous
		else if (_iKeyState == LVKF_SHIFT) {
				Find(-1);
				return 1;
		}
	}
	return 0;
}

void SNM_FindWnd::DrawControls(LICE_IBitmap* _bm, RECT* _r)
{
	if (!_bm)
		return;

	IconTheme* it = NULL;
#ifdef ICON_BUTTONS
	it = (IconTheme*)GetIconThemeStruct(NULL);
#endif
	LICE_CachedFont* font = SNM_GetThemeFont();
	int x0=_r->left+10, h=35;

	// 1st row of controls
	bool drawLogo = false;
	m_cbType.SetFont(font);
	if (SetVWndAutoPosition(&m_cbType, NULL, _r, &x0, _r->top, h))
	{
		switch (m_type)
		{
			case TYPE_ITEM_NAME:
			case TYPE_ITEM_NAME_ALL_TAKES:
			case TYPE_ITEM_FILENAME:
			case TYPE_ITEM_FILENAME_ALL_TAKES:
			case TYPE_ITEM_NOTES:
				m_btnEnableZommScroll.SetVisible(true);
				m_btnEnableZommScroll.SetCheckState(m_zoomSrollItems);
				m_btnEnableZommScroll.SetTextLabel("Zoom/Scroll", -1, font);
				drawLogo = SetVWndAutoPosition(&m_btnEnableZommScroll, NULL, _r, &x0, _r->top, h);
				break;
			default:
				m_btnEnableZommScroll.SetVisible(false);
				drawLogo = true;
				break;
		}
	}

	if (drawLogo)
		AddSnMLogo(_bm, _r, x0, h);

	// 2nd row of controls
	x0 = _r->left+8; 
	h = 45;
	int y0 = _r->top+60;

	// Buttons
	WDL_VirtualIconButton_SkinConfig* img = it ? &(it->transport_play[0]) : NULL;
	if (img)
		m_btnFind.SetIcon(img);
	else {
		m_btnFind.SetTextLabel("Find all", 0, font);
		m_btnFind.SetForceBorder(true);
	}
	m_btnFind.SetGrayed(!g_searchStr || !(*g_searchStr) || m_type == TYPE_MARKER_REGION);
#ifdef ICON_BUTTONS
	if (SetVWndAutoPosition(&m_btnFind, NULL, _r, &x0, y0, h, 0))
#else
	if (SetVWndAutoPosition(&m_btnFind, NULL, _r, &x0, y0, h, 5))
#endif
	{
		img = it ? &(it->transport_rew) : NULL;
		if (img)
			m_btnPrev.SetIcon(img);
		else {
			m_btnPrev.SetTextLabel("<", 0, font);
			m_btnPrev.SetForceBorder(true);
		}
		m_btnPrev.SetGrayed(!g_searchStr || !(*g_searchStr));
#ifdef ICON_BUTTONS
		if (SetVWndAutoPosition(&m_btnPrev, NULL, _r, &x0, y0, h, 0))
#else
		if (SetVWndAutoPosition(&m_btnPrev, NULL, _r, &x0, y0, h, 5))
#endif
		{
			img = it ? &(it->transport_fwd) : NULL;
			if (img)
				m_btnNext.SetIcon(img);
			else {
				m_btnNext.SetTextLabel(">", 0, font);
				m_btnNext.SetForceBorder(true);
			}
			m_btnNext.SetGrayed(!g_searchStr || !(*g_searchStr));
			SetVWndAutoPosition(&m_btnNext, NULL, _r, &x0, y0, h);
		}
	}

	m_txtResult.SetFont(font);
	m_txtResult.SetText(g_notFound ? "Not found !" : "");
	SetVWndAutoPosition(&m_txtResult, NULL, _r, &x0, y0, h);
}

INT_PTR SNM_FindWnd::OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_PAINT:
		{
			int xo, yo;
			RECT r;
			GetClientRect(m_hwnd, &r);		
			m_parentVwnd.SetPosition(&r);
			m_vwnd_painter.PaintBegin(m_hwnd, WDL_STYLE_GetSysColor(COLOR_WINDOW));
			DrawControls(m_vwnd_painter.GetBuffer(&xo, &yo), &r);
			m_vwnd_painter.PaintVirtWnd(&m_parentVwnd);
			m_vwnd_painter.PaintEnd();
		}
		break;
		case WM_LBUTTONDOWN:
/* commented: selects find text otherwise
			SetFocus(m_hwnd); 
*/
			if (m_parentVwnd.OnMouseDown(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)) > 0)
				SetCapture(m_hwnd);
			break;
		case WM_LBUTTONUP:
			if (GetCapture() == m_hwnd)	{
				m_parentVwnd.OnMouseUp(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
				ReleaseCapture();
			}
			break;
		case WM_MOUSEMOVE:
			m_parentVwnd.OnMouseMove(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
			break;
#ifdef _SNM_THEMABLE
		case WM_CTLCOLOREDIT:
			if ((HWND)lParam == GetDlgItem(m_hwnd, IDC_EDIT)) {
				int bg, txt; SNM_GetThemeEditColors(&bg, &txt);
				SetBkColor((HDC)wParam, bg);
				SetTextColor((HDC)wParam, txt);
				return (INT_PTR)SNM_GetThemeBrush(bg);
			}
			break;
#endif
	}
	return 0;
}

int FindViewInit()
{
	g_pFindWnd = new SNM_FindWnd();
	if (!g_pFindWnd)
		return 0;
	return 1;
}

void FindViewExit() {
	delete g_pFindWnd;
	g_pFindWnd = NULL;
}

void OpenFindView(COMMAND_T*) {
	if (g_pFindWnd) {
		g_pFindWnd->Show(true, true);
		SetFocus(GetDlgItem(g_pFindWnd->GetHWND(), IDC_EDIT));
	}
}

bool IsFindViewDisplayed(COMMAND_T*){
	return (g_pFindWnd && g_pFindWnd->IsValidWindow());
}

void FindNextPrev(COMMAND_T* _ct) {
	if (g_pFindWnd)
		g_pFindWnd->Find((int)_ct->user); 
}
