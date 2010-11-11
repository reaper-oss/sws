/******************************************************************************
/ SnM_FindView.cpp
/
/ Copyright (c) 2009-2010 Tim Payne (SWS), Jeffos 
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
#include "SNM_ChunkParserPatcher.h"
#include "../Freeze/ItemSelState.h"


#define SAVEWINDOW_POS_KEY		"S&M - Find Save Window Position"
#define MAX_SEARCH_STR_LEN		128

enum {
  BUTTONID_FIND=1000,
  BUTTONID_PREV,
  BUTTONID_NEXT,
  COMBOID_TYPE
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

bool TakeNameMatch(MediaItem_Take* _tk, const char* _searchStr) {
	char* takeName = (char*)GetSetMediaItemTakeInfo(_tk, "P_NAME", NULL);
	return (takeName && stristr(takeName, _searchStr));
}

bool TakeFilenameMatch(MediaItem_Take* _tk, const char* _searchStr) {
	bool match = false;
	PCM_source* src =(PCM_source*)GetSetMediaItemTakeInfo(_tk,"P_SOURCE",NULL);
	if (src) {
		const char* takeFilename = src->GetFileName();
		match = (takeFilename && stristr(takeFilename, _searchStr));
	}
	return match;
}

bool ItemNotesMatch(MediaItem_Take* _tk, const char* _searchStr) {
	bool match = false;
	MediaItem* item = GetMediaItemTake_Item(_tk);
	if (item)
	{
		SNM_ChunkParserPatcher p(item);
		WDL_String notes;
		if (p.GetSubChunk("NOTES", 2, 0, &notes))
			match = (stristr(notes.Get(), _searchStr) != NULL); //JFB!! bof..
	}
	return match;
}

bool TrackNameMatch(MediaTrack* _tr, const char* _searchStr) {
	char* name = (char*)GetSetMediaTrackInfo(_tr, "P_NAME", NULL);
	return (name && stristr(name, _searchStr));
}

bool TrackNotesMatch(MediaTrack* _tr, const char* _searchStr) {
	bool match = false;
	for (int i=0; i < g_pTracksNotes.Get()->GetSize(); i++)
	{
		if (g_pTracksNotes.Get()->Get(i)->m_tr == _tr)
		{
			match = (stristr(g_pTracksNotes.Get()->Get(i)->m_notes.Get(), _searchStr) != NULL);
			break;
		}
	}
	return match;
}



///////////////////////////////////////////////////////////////////////////////
// SNM_FindWnd
///////////////////////////////////////////////////////////////////////////////

SNM_FindWnd::SNM_FindWnd()
:SWS_DockWnd(IDD_SNM_FIND, "Find", 30008, SWSGetCommandID(OpenFindView))
{
	// GUI inits
	if (m_bShowAfterInit)
		Show(false, false);

	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
	if (GetDlgItem(m_hwnd, IDC_EDIT))
		SetWindowLongPtr(GetDlgItem(m_hwnd, IDC_EDIT), GWLP_USERDATA, 0xdeadf00b);
}

int SNM_FindWnd::GetType(){
	return m_type;
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
			update = FindMediaItem(_mode, true, ItemNotesMatch);
		break;

		case TYPE_TRACK_NAME:
			update = FindTrack(_mode, TrackNameMatch);
		break;
		case TYPE_TRACK_NOTES:
			update = FindTrack(_mode, TrackNotesMatch);
		break;

		case TYPE_MARKER_REGION:
			update = FindMarkerRegion(_mode);

		default:
		break;
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
		MediaTrack* tr = CSurf_TrackFromID(i,false); 
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

bool SNM_FindWnd::FindMediaItem(int _dir, bool _allTakes, bool (*job)(MediaItem_Take*,const char*))
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

		if (startItem)
		{
			MediaTrack* startTr = GetMediaItem_Track(startItem);
			int startTrIdx = CSurf_TrackToID(startTr, false);

			// find startItem idx
			int startItemIdx=-1;
			MediaItem* item = NULL;
			while (item != startItem) 
				item = GetTrackMediaItem(startTr,++startItemIdx);

			bool firstItem = true, breakSelection = false;
			for (int i = startTrIdx; 
				!breakSelection && i <= CountTracks(NULL) && i>=1; 
				i += (!_dir ? 1 : _dir))
			{
				MediaTrack* tr = CSurf_TrackFromID(i,false); 
				int nbItems = GetTrackNumMediaItems(tr);
				for (int j = (firstItem ? startItemIdx : (_dir >= 0 ? 0 : (nbItems-1))); 
					 tr && !breakSelection && j < nbItems && j >= 0; 
					 j += (!_dir ? 1 : _dir))
				{
					item = GetTrackMediaItem(tr,j);
					firstItem = false;
					for (int k=0; item && k < GetMediaItemNumTakes(item); k++)
					{
						MediaItem_Take* tk = GetMediaItemTake(item, k);
						if (tk && (_allTakes || (!_allTakes && tk == GetActiveTake(item))))
						{
							if (job(tk, g_searchStr))
							{
								if (!update)
									Undo_BeginBlock();

								update = found = true;
								GetSetMediaItemInfo(item, "B_UISEL", &sel);
								if (_dir) 
								{
									breakSelection = true;
									break;
								}
							}
						}
					}
				}
			}
		}

		if (!found)
			DisplayNotFoundMsg(g_searchStr);
		else
		{
			//JFB TODO: would be cool to scroll to selected items, but no native actions..
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
			for (int i = startTrIdx; 
				i <= CountTracks(NULL) && i>=0; 
				i += (!_dir ? 1 : _dir))
			{
				MediaTrack* tr = CSurf_TrackFromID(i,false); 
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

		if (!found)
			DisplayNotFoundMsg(g_searchStr);
		else
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
		while ((x=EnumProjectMarkers(x, &bR, &dPos, &dRend, &cName, &id)))
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

		if (found) {
			SetEditCurPos(dMinMaxPos, true, false);
			update = true;
		}
		else
			DisplayNotFoundMsg(g_searchStr);
	}

	if (update)
		Undo_OnStateChangeEx("Find: change cursor position", UNDO_STATE_ALL, -1);

	return update;
}

void SNM_FindWnd::DisplayNotFoundMsg(const char* _searchStr)
{
	g_notFound = true;
	m_parentVwnd.RequestRedraw(NULL);
}

void SNM_FindWnd::OnInitDlg()
{
	m_resize.init_item(IDC_EDIT, 0.0, 0.0, 1.0, 0.0);
	SetWindowLongPtr(GetDlgItem(m_hwnd, IDC_EDIT), GWLP_USERDATA, 0xdeadf00b);

	// Load prefs 
	m_type = GetPrivateProfileInt("FIND_VIEW", "TYPE", 0, g_SNMiniFilename.Get());

	// WDL GUI init
    m_parentVwnd.SetRealParent(m_hwnd);

	m_btnFind.SetID(BUTTONID_FIND);
	m_btnFind.SetRealParent(m_hwnd);
	m_btnFind.SetImmediate(true);
	m_parentVwnd.AddChild(&m_btnFind);

	m_btnPrev.SetID(BUTTONID_PREV);
	m_btnPrev.SetRealParent(m_hwnd);
	m_btnPrev.SetImmediate(true);
	m_parentVwnd.AddChild(&m_btnPrev);

	m_btnNext.SetID(BUTTONID_NEXT);
	m_btnNext.SetRealParent(m_hwnd);
	m_btnNext.SetImmediate(true);
	m_parentVwnd.AddChild(&m_btnNext);

	m_cbType.SetID(COMBOID_TYPE);
	m_cbType.SetRealParent(m_hwnd);
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

	g_notFound = false;
	*g_searchStr = 0;

	m_parentVwnd.RequestRedraw(NULL);
}

void SNM_FindWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	// WDL GUI
	switch(wParam)
	{
		case (IDC_EDIT | (EN_CHANGE << 16)):
		{
			GetDlgItemText(GetHWND(), IDC_EDIT, g_searchStr, MAX_SEARCH_STR_LEN);
			g_notFound = false;
			m_parentVwnd.RequestRedraw(NULL); // buttons grayed for some types
			break;
		}
		default:
		{
			if (HIWORD(wParam)==CBN_SELCHANGE && LOWORD(wParam)==COMBOID_TYPE) 
			{
				m_type = m_cbType.GetCurSel();
				g_notFound = false;
				m_parentVwnd.RequestRedraw(NULL); // buttons grayed for some types
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
	WritePrivateProfileString("FIND_VIEW", "TYPE", cType, g_SNMiniFilename.Get());

	m_cbType.Empty();
	m_parentVwnd.RemoveAllChildren(false);

	g_notFound = false;
	*g_searchStr = 0;
}

static void DrawControls(WDL_VWnd_Painter *_painter, RECT _r, WDL_VWnd* _parentVwnd)
{
	if (!g_pFindWnd)
		return;

	int xo=0, yo=0, sz;
    LICE_IBitmap *bm = _painter->GetBuffer(&xo,&yo);
	if (bm)
	{
		ColorTheme* ct = (ColorTheme*)GetColorThemeStruct(&sz);

		static LICE_IBitmap *logo=  NULL;
		if (!logo)
		{
#ifdef _WIN32
			logo = LICE_LoadPNGFromResource(g_hInst,IDB_SNM,NULL);
#else
			// SWS doesn't work, sorry. :( logo =  LICE_LoadPNGFromNamedResource("SnM.png",NULL);
			logo = NULL;
#endif
		}

		static LICE_CachedFont tmpfont;
		if (!tmpfont.GetHFont())
		{
			LOGFONT lf = {
				14,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
				CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,SWSDLG_TYPEFACE
			};
			if (ct) 
				lf = ct->mediaitem_font;
			tmpfont.SetFromHFont(CreateFontIndirect(&lf),LICE_FONT_FLAG_OWNS_HFONT);                 
		}
		tmpfont.SetBkMode(TRANSPARENT);
		if (ct)	tmpfont.SetTextColor(LICE_RGBA_FROMNATIVE(ct->main_text,255));
		else tmpfont.SetTextColor(LICE_RGBA(255,255,255,255));

		int x0=_r.left+10; int y0=_r.top+5;

		// Dropdown
		WDL_VirtualComboBox* cbVwnd = (WDL_VirtualComboBox*)_parentVwnd->GetChildByID(COMBOID_TYPE);
		if (cbVwnd)
		{
			RECT tr2={x0,y0+3,x0+190,y0+25-2};
			x0 = tr2.right+5;
			cbVwnd->SetPosition(&tr2);
			cbVwnd->SetFont(&tmpfont);
		}

		if (logo && (_r.right - _r.left) > (x0+logo->getWidth()))
			LICE_Blit(bm,logo,_r.right-logo->getWidth()-8,y0+3,NULL,0.125f,LICE_BLIT_MODE_ADD|LICE_BLIT_USE_ALPHA);

		x0 = _r.left+8; y0 = _r.top+5+60;
		int w=25, h=25; //default width/height
		IconTheme* it = (IconTheme*)GetIconThemeStruct(&sz);// returns the whole icon theme (icontheme.h) and the size

		// Buttons
		WDL_VirtualIconButton* btnVwnd = (WDL_VirtualIconButton*)_parentVwnd->GetChildByID(BUTTONID_FIND);
		if (btnVwnd)
		{
			WDL_VirtualIconButton_SkinConfig* img=NULL;
			img = it ? &(it->transport_play[0]) : NULL;
			if (img) {
				btnVwnd->SetIcon(img);
				w = img->image->getWidth() / 3;
				h = img->image->getHeight();
			}
			RECT tr2={x0,y0,x0+w,y0+h};
			btnVwnd->SetPosition(&tr2);
			x0 += 5+w;
			btnVwnd->SetGrayed(!g_searchStr || !(*g_searchStr) || g_pFindWnd->GetType() == TYPE_MARKER_REGION);
		}
		btnVwnd = (WDL_VirtualIconButton*)_parentVwnd->GetChildByID(BUTTONID_PREV);
		if (btnVwnd)
		{
			WDL_VirtualIconButton_SkinConfig* img=NULL;
			img = it ? &(it->transport_rew) : NULL;
			if (img) {
				btnVwnd->SetIcon(img);
				w = img->image->getWidth() / 3;
				h = img->image->getHeight();
			}
			RECT tr2={x0,y0,x0+w,y0+h};
			btnVwnd->SetPosition(&tr2);
			x0 += 5+w;
			btnVwnd->SetGrayed(!g_searchStr || !(*g_searchStr));
		}
		btnVwnd = (WDL_VirtualIconButton*)_parentVwnd->GetChildByID(BUTTONID_NEXT);
		if (btnVwnd)
		{
			WDL_VirtualIconButton_SkinConfig* img=NULL;
			img = it ? &(it->transport_fwd) : NULL;
			if (img) {
				btnVwnd->SetIcon(img);
				w = img->image->getWidth() / 3;
				h = img->image->getHeight();
			}
			RECT tr2={x0,y0,x0+w,y0+h};
			btnVwnd->SetPosition(&tr2);
			x0 += 5+w;
			btnVwnd->SetGrayed(!g_searchStr || !(*g_searchStr));
		}

		if (g_notFound)
		{
			x0+=5;
			RECT tr={x0,y0,x0+60,y0+h};
			tmpfont.DrawText(bm, "Not found!", -1, &tr, DT_LEFT | DT_VCENTER);
			x0 = tr.right+5;
		}

	    _painter->PaintVirtWnd(_parentVwnd);
	}
}

int SNM_FindWnd::OnUnhandledMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_PAINT:
		{
			RECT r; int sz;
			GetClientRect(m_hwnd,&r);		
	        m_parentVwnd.SetPosition(&r);

			ColorTheme* ct = (ColorTheme*)GetColorThemeStruct(&sz);
			if (ct)	m_vwnd_painter.PaintBegin(m_hwnd, ct->tracklistbg_color);
			else m_vwnd_painter.PaintBegin(m_hwnd, LICE_RGBA(0,0,0,255));
			DrawControls(&m_vwnd_painter, r, &m_parentVwnd);
			m_vwnd_painter.PaintEnd();
		}
		break;

		case WM_LBUTTONDOWN:
		{
			SetFocus(g_pFindWnd->GetHWND());
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);
			WDL_VWnd *w = m_parentVwnd.VirtWndFromPoint(x,y);
			if (w) 
			{
				g_notFound = false;
				m_parentVwnd.RequestRedraw(NULL);

				w->OnMouseDown(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
				if (w == &m_btnFind)
					Find(0);
				else if (w == &m_btnPrev) 
					Find(-1);
				else if (w == &m_btnNext) 
					Find(1);
			}
		}
		break;

		case WM_LBUTTONUP:
		{
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);
			WDL_VWnd *w = m_parentVwnd.VirtWndFromPoint(x,y);
			if (w) w->OnMouseUp(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
		}
		break;

		case WM_MOUSEMOVE:
		{
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);
			WDL_VWnd *w = m_parentVwnd.VirtWndFromPoint(x,y);
			if (w) w->OnMouseMove(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
		}
		break;

		default:
			break;
	}
	return 0;
}

/*
static void menuhook(const char* menustr, HMENU hMenu, int flag)
{
	if (!strcmp(menustr, "Main view") && !flag)
	{
		int cmd = NamedCommandLookup("_S&M_SHOWFIND");
		if (cmd > 0)
		{
			int afterCmd = NamedCommandLookup("_SWSCONSOLE");
			AddToMenu(hMenu, "S&&M Find", cmd, afterCmd > 0 ? afterCmd : 40075);
		}
	}
}
*/
int FindViewInit()
{
	g_pFindWnd = new SNM_FindWnd();
	if (!g_pFindWnd/* || !plugin_register("hookcustommenu", (void*)menuhook)*/)
		return 0;
	return 1;
}

void FindViewExit() {
	delete g_pFindWnd;
	g_pFindWnd = NULL;
}

void OpenFindView(COMMAND_T*) {
	if (g_pFindWnd)
		g_pFindWnd->Show(true, true);
}

bool IsFindViewDisplayed(COMMAND_T*){
	return (g_pFindWnd && g_pFindWnd->IsValidWindow());
}
