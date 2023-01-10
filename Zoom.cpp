/******************************************************************************
/ Zoom.cpp
/
/ Copyright (c) 2009 and later Tim Payne (SWS)
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

#include "./Breeder/BR_EnvelopeUtil.h"
#include "./Breeder/BR_Util.h"
#include "./nofish/nofish.h" // NF_IsObeyTrackHeightLockEnabled
#include "./ObjectState/TrackEnvelope.h"
#include "./SnM/SnM_Dlg.h"

#include <WDL/localize/localize.h>

#define VZOOM_RANGE 40

void ZoomTool(COMMAND_T* = NULL);
void SaveZoomSlice(bool bSWS);
#define WM_CANCEL_ZOOM	4000
#define WM_START_ZOOM	4001

void SetReaperWndSize(COMMAND_T* = NULL)
{
	int x = GetPrivateProfileInt("REAPER", "setwndsize_x", 640, get_ini_file());
	int y = GetPrivateProfileInt("REAPER", "setwndsize_y", 480, get_ini_file());
	HWND hwnd = GetMainHwnd();
	SetWindowPos(hwnd, NULL, 0, 0, x, y, SWP_NOMOVE | SWP_NOZORDER);
}

// hwnd: optional, dRoom and dOffset: normalized values (%)
void SetHorizPos(HWND hwnd, double dPos, double dRoom = 0.0, double dOffset = 0.0)
{
	double start, end;
	GetSet_ArrangeView2(NULL, false, 0, 0, &start, &end); // full arrange view's start/end time -- v5.12pre4+ only

	double start_timeOut = dPos - (end-start) * dOffset;
	double end_timeOut = dPos + (end-start) * (1.0-dOffset);
	start_timeOut -= (end_timeOut-start_timeOut) * dRoom;
	end_timeOut += (end_timeOut-start_timeOut) * dRoom;
	GetSet_ArrangeView2(NULL, true, 0, 0, &start_timeOut, &end_timeOut); // includes UI refresh
}

void SetVertPos(HWND hwnd, int iTrack, bool bPixels, int iExtra = 0) // 1 based track index!
{
	SCROLLINFO si = { sizeof(SCROLLINFO), };
	si.fMask = SIF_ALL;
	CoolSB_GetScrollInfo(hwnd, SB_VERT, &si);

	int oldmax = si.nMax;
	si.nMax = 0;
	if (bPixels)
		si.nPos = iTrack;
	else
		si.nPos = iExtra;

	MediaTrack* masterTrack = GetMasterTrack(NULL);
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* track = CSurf_TrackFromID(i, false);
		int iHeight = *(int*)GetSetMediaTrackInfo(track, "I_WNDH", NULL);
		if (track == masterTrack && TcpVis(track)) iHeight += GetMasterTcpGap();
		if (!bPixels && i < iTrack)
			si.nPos += iHeight;
		si.nMax += iHeight;
	}
	si.nMax += 65;

	if (oldmax == si.nPage && (UINT)si.nMax > si.nPage)
		si.nPage++;
	else if ((UINT)si.nMax < si.nPage)
		si.nMax = si.nPage;

	CoolSB_SetScrollInfo(hwnd, SB_VERT, &si, true);
	SendMessage(hwnd, WM_VSCROLL, si.nPos << 16 | SB_THUMBPOSITION, 0);
}

static float get_reaper_vzoom()
{
	if (ConfigVar<float> vzoom3 { "vzoom3" }) // REAPER v6.73+devXXXX
		return *vzoom3;
	return static_cast<float>(*ConfigVar<int>("vzoom2"));
}

static void set_reaper_vzoom(const float vz)
{
	// vzoom2 is ignored in REAPER v6.73+devXXXX onwards [p=2632786]
	*ConfigVar<int>("vzoom2") = static_cast<int>(floor(vz+0.5));
	ConfigVar<float>("vzoom3").try_set(vz);
}

void VertZoomRange(int iFirst, int iNum, bool* bZoomed, bool bMinimizeOthers, bool includeEnvelopes)
{
	HWND hTrackView = GetTrackWnd();
	MediaTrack* masterTrack = GetMasterTrack(NULL);
	if (!hTrackView || iNum == 0)
		return;

	// Zoom in vertically
	RECT rect;
	GetClientRect(hTrackView, &rect);
	int iTotalHeight = rect.bottom;
	int lastTrackId = iFirst + iNum - (includeEnvelopes ? 1 : 2);

	const int minTrackHeight = SNM_GetIconTheme()->tcp_small_height;
	const bool obeyHeightLock = NF_IsObeyTrackHeightLockEnabled();

	if (bMinimizeOthers)
	{
		set_reaper_vzoom(0.f);
		int iMinimizedTracks = 0;
		// setting I_HEIGHTOVERRIDE to 0 on locked track effectively disables track lock
		// see https://forum.cockos.com/showthread.php?p=2202082
		for (int i = 0; i <= GetNumTracks(); i++)
		{
			MediaTrack* tr = CSurf_TrackFromID(i, false);
			const bool locked = GetMediaTrackInfo_Value(tr, "B_HEIGHTLOCK");
			if (!obeyHeightLock || !locked)
			{
				SetMediaTrackInfo_Value(tr, "I_HEIGHTOVERRIDE", locked ? minTrackHeight : 0);
				iMinimizedTracks += 1;
			}		
		}

		Main_OnCommand(40112, 0); // Zoom out vert to minimize envelope lanes too (since vZoom is now 0) (calls refresh)
		if (iMinimizedTracks <= 1) // ignore master track since there can be no items on it
			return;

		// Get the size of shown but not zoomed tracks
		int iNotZoomedSize = 0;
		int iZoomed = 0;

		for (int i = 0; i < iNum; i++)
		{
			MediaTrack* tr = CSurf_TrackFromID(i+iFirst, false);
			const bool locked = GetMediaTrackInfo_Value(tr, "B_HEIGHTLOCK");

			if (bZoomed[i] && (!obeyHeightLock || !locked))
			{
				iZoomed++;
				if (tr == masterTrack && TcpVis(tr) && iNum > 1) iNotZoomedSize += GetMasterTcpGap();
			}
			else
			{
				if (TcpVis(tr))
				{
					int trackHeight = 0;
					if (obeyHeightLock && locked)
						trackHeight = static_cast<int>(GetMediaTrackInfo_Value(tr, "I_HEIGHTOVERRIDE"));
					else
						trackHeight = GetTrackHeightFromVZoomIndex(tr, 0);

					trackHeight += CountTrackEnvelopePanels(tr) * GetEnvHeightFromTrackHeight(trackHeight);
					iNotZoomedSize += trackHeight;
				}
			}
		}
		// Pixels we have to work with will all the sel tracks and their envelopes
		iTotalHeight -= iNotZoomedSize;
		int iEachHeight = iTotalHeight / iZoomed;
		int iLanesHeight = 0;
		while (true)
		{
			iLanesHeight = 0;
			// Calc envelope height
			for (int i = 0; i < iNum; i++)
			{
				if (bZoomed[i] && i + iFirst <= lastTrackId) // don't check envelope lanes height for the last track if includeEnvelopes == true
					iLanesHeight += CountTrackEnvelopePanels(CSurf_TrackFromID(i + iFirst, false)) * GetEnvHeightFromTrackHeight(iEachHeight);
			}
			if (iEachHeight * iZoomed + iLanesHeight <= iTotalHeight)
				break;

			// Guess new iEachHeight - may be wrong!
			int iNewEachHeight = (int)(iTotalHeight / ((double)iLanesHeight / iEachHeight + iZoomed));
			if (iNewEachHeight == iEachHeight)
				iEachHeight--;
			else
				iEachHeight = iNewEachHeight;
			if (iEachHeight < minTrackHeight)
				break;
		}

		// Check track is over minimum and make sure last track fills the TCP exactly to the bottom
		int leftOverHeight = 0;
		if (iEachHeight < minTrackHeight)
		{
			iEachHeight = minTrackHeight;
		}
		else
		{
			leftOverHeight = iTotalHeight - (iEachHeight * iZoomed) - iLanesHeight;
			if (iEachHeight + leftOverHeight >= (int) (iEachHeight * 1.5)) // if leftover height is to big, just leave it cropped (this is the same mechanism REAPER uses for take heights within the item)
				leftOverHeight = 0;

		}

		for (int i = 0; i < iNum; i++)
		{
			if (bZoomed[i])
			{
				if (i + 1 == iNum)
					iEachHeight +=leftOverHeight;
				MediaTrack* tr = CSurf_TrackFromID(i + iFirst, false);
				if (!obeyHeightLock || !GetMediaTrackInfo_Value(tr, "B_HEIGHTLOCK"))
					GetSetMediaTrackInfo(tr, "I_HEIGHTOVERRIDE", &iEachHeight);
			}
		}
		TrackList_AdjustWindows(false);
		UpdateTimeline();
	}
	else
	{
		vector<vector<int> > envelopeHeights;
		for (int i = 0; i < iNum; i++)
		{
			if (i+iFirst <= lastTrackId) // don't check envelope lanes height for the last track if includeEnvelopes == true
			{
				vector<int> tmp;
				envelopeHeights.push_back(tmp);
				MediaTrack* tr = CSurf_TrackFromID(i+iFirst, false);
				if (TcpVis(tr))
				{
					for (int j = 0; j < CountTrackEnvelopes(tr); j++)
					{
						BR_Envelope envelope(GetTrackEnvelope(tr, j));
						if (envelope.IsInLane())
							envelopeHeights.back().push_back(envelope.GetLaneHeight());
					}
				}
			}
		}

		int iZoom = VZOOM_RANGE+1;
		int iHeight;
		int trackHeight = 0;
		do
		{
			iZoom--;
			iHeight = 0;
			for (int i = 0; i < iNum; i++)
			{
				MediaTrack* tr = CSurf_TrackFromID(i+iFirst, false);
				if (TcpVis(tr))
				{
					const bool locked = GetMediaTrackInfo_Value(tr, "B_HEIGHTLOCK");
					if (obeyHeightLock && locked)
						trackHeight = static_cast<int>(GetMediaTrackInfo_Value(tr, "I_HEIGHTOVERRIDE"));
					else
						trackHeight = GetTrackHeightFromVZoomIndex(tr, iZoom);

					int envHeight = 0;
					if (i < (int)envelopeHeights.size())
					{
						for (size_t j = 0; j < envelopeHeights[i].size(); j++)
						{
							int h = envelopeHeights[i][j];
							envHeight += (h != 0) ? h : GetEnvHeightFromTrackHeight(trackHeight);
						}
					}
					iHeight += trackHeight + envHeight;
					if (tr == masterTrack && TcpVis(tr) && iNum > 1) iHeight += GetMasterTcpGap();
				}
			}
		} while (iHeight > iTotalHeight && iZoom > 0);

		// Reset custom track sizes
		for (int i = 0; i <= GetNumTracks(); i++)
		{
			MediaTrack* tr = CSurf_TrackFromID(i, false);
			const bool locked = GetMediaTrackInfo_Value(tr, "B_HEIGHTLOCK");
			if (!obeyHeightLock || !locked)
				SetMediaTrackInfo_Value(tr, "I_HEIGHTOVERRIDE", locked ? trackHeight : 0);
		}
		set_reaper_vzoom(static_cast<float>(iZoom));
		TrackList_AdjustWindows(false);
		UpdateTimeline();
	}

	SetVertPos(hTrackView, iFirst, false);
}

// iOthers == 0 nothing, 1 minimize, 2 hide from TCP
void VertZoomSelTracks(int iOthers, bool includeEnvelopes)
{
	// Find first and last selected tracks
	int iFirstSel = -1;
	int iLastSel = -1;
	WDL_TypedBuf<bool> hbSelected;
	hbSelected.Resize(GetNumTracks()+1);
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (GetTrackVis(tr) & 2 && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			if (iFirstSel == -1)
				iFirstSel = i;
			iLastSel = i;
			hbSelected.Get()[i] = true;
		}
		else
			hbSelected.Get()[i] = false;
	}
	if (iFirstSel == -1)
		return;

	// Hide tracks from TCP only after making sure there's actually something selected
	if (iOthers == 2)
		for (int i = 0; i <= GetNumTracks(); i++)
			if (!hbSelected.Get()[i])
			{
				MediaTrack* tr = CSurf_TrackFromID(i, false);
				SetTrackVis(tr, GetTrackVis(tr) & 1);
			}

	VertZoomRange(iFirstSel, 1+iLastSel-iFirstSel, hbSelected.Get()+iFirstSel, iOthers == 1, includeEnvelopes);
	SaveZoomSlice(true);
}

// iOthers == 0 nothing, 1 minimize, 2 hide from TCP
void VertZoomSelItems(int iOthers, bool includeEnvelopes)
{
	// Find the tracks of the first and last selected item
	int y1 = -1, y2 = -1;
	WDL_TypedBuf<bool> hbVisOnTrack;
	hbVisOnTrack.Resize(GetNumTracks());
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		hbVisOnTrack.Get()[i-1] = false;
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (GetTrackVis(tr) & 2)
		{
			for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
			{
				MediaItem* mi = GetTrackMediaItem(tr, j);
				if (*(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL))
				{
					if (y1 == -1)
						y1 = i;
					y2 = i;
					hbVisOnTrack.Get()[i-1] = true;
					break;
				}
			}
		}
		if (iOthers == 2 && !hbVisOnTrack.Get()[i-1])
			SetTrackVis(tr, GetTrackVis(tr) & 1);
	}

	if (y1 == -1)
		return;

	// Hide tracks from TCP only after making sure there's actually something selected
	if (iOthers == 2)
	{
		for (int i = 1; i <= GetNumTracks(); i++)
		{
			if (!hbVisOnTrack.Get()[i-1])
			{
				MediaTrack* tr = CSurf_TrackFromID(i, false);
				SetTrackVis(tr, GetTrackVis(tr) & 1);
			}
		}
		if (TcpVis(GetMasterTrack(NULL)))
			SetTrackVis(GetMasterTrack(NULL), GetTrackVis(GetMasterTrack(NULL)) & 1);
	}
	VertZoomRange(y1, 1+y2-y1, hbVisOnTrack.Get()+y1-1, iOthers == 1, includeEnvelopes);
	SaveZoomSlice(true);
}

void HorizZoomSelItems(bool bTimeSel = false)
{
	HWND hTrackView = GetTrackWnd();
	if (!hTrackView)
		return;

	RECT r;
	GetClientRect(hTrackView, &r);

	double x1, x2;
	GetSet_LoopTimeRange(false, false, &x1, &x2, false);
	if (!bTimeSel || x1 == x2)
	{
		// Find the coordinates of the first and last selected item
		x1 = DBL_MAX; x2 = -DBL_MAX;
		for (int i = 1; i <= GetNumTracks(); i++)
		{
			MediaTrack* tr = CSurf_TrackFromID(i, false);
			if (GetTrackVis(tr) & 2)
				for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
				{
					MediaItem* mi = GetTrackMediaItem(tr, j);
					if (*(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL))
					{
						double d = *(double*)GetSetMediaItemInfo(mi, "D_POSITION", NULL);
						if (d < x1)
							x1 = d;
						d += *(double*)GetSetMediaItemInfo(mi, "D_LENGTH", NULL);
						if (d > x2)
							x2 = d;
					}
				}
		}

		if (x1 == DBL_MAX)
			return;
	}

	adjustZoom(0.94 * r.right / (x2 - x1), 1, false, -1);
	SetHorizPos(hTrackView, x1, 0.03);
	SaveZoomSlice(true);
}

// ct->user is the screen position in %, negative for play cursor (vs edit cursor)
void ScrollToCursor(COMMAND_T* ct)
{
	SetHorizPos(NULL, ct->user<0 && (GetPlayState()&1) ? GetPlayPosition() : GetCursorPosition(), 0.0, 0.01 * abs((int)ct->user));
}

void HorizScroll(const int amount)
{
	HWND hwnd = GetTrackWnd();
	SCROLLINFO si = { sizeof(SCROLLINFO), };
	si.fMask = SIF_ALL;
	CoolSB_GetScrollInfo(hwnd, SB_HORZ, &si);
	si.nPos += (int)((double)amount * si.nPage / 100.0);
	if (si.nPos < 0) si.nPos = 0;
	else if (si.nPos > si.nMax) si.nPos = si.nMax;
	CoolSB_SetScrollInfo(hwnd, SB_HORZ, &si, true);
	SendMessage(hwnd, WM_HSCROLL, SB_THUMBPOSITION, 0);
}

void HorizScroll(COMMAND_T * ctx)
{
	HorizScroll(static_cast<int>(ctx->user));
}

void ZoomToSelItems(COMMAND_T* ct)			{ VertZoomSelItems(0, ct ? (int)ct->user == 0 : false); HorizZoomSelItems(); }
void ZoomToSelItemsMin(COMMAND_T* ct)		{ VertZoomSelItems(1, (int)ct->user == 0); HorizZoomSelItems(); }
void VZoomToSelItems(COMMAND_T* ct)			{ VertZoomSelItems(0, (int)ct->user == 0); }
void VZoomToSelItemsMin(COMMAND_T* ct)		{ VertZoomSelItems(1, (int)ct->user == 0); }
void HZoomToSelItems(COMMAND_T* = NULL)		{ HorizZoomSelItems(); }
void ZoomToSIT(COMMAND_T* ct)				{ VertZoomSelItems(0, (int)ct->user == 0); HorizZoomSelItems(true); }
void ZoomToSITMin(COMMAND_T* ct)			{ VertZoomSelItems(1, (int)ct->user == 0); HorizZoomSelItems(true); }
void FitSelTracks(COMMAND_T* ct)			{ VertZoomSelTracks(0, (int)ct->user == 0); }
void FitSelTracksMin(COMMAND_T* ct)			{ VertZoomSelTracks(1, (int)ct->user == 0); }

class ArrangeState
{
private:
	double dHZoom;
	float fVZoom;
	int iXPos;
	int iYPos;
	WDL_TypedBuf<int>  hbTrackHeights;
	WDL_TypedBuf<int>  hbTrackVis;
	WDL_TypedBuf<int>  hbEnvHeights;
	WDL_TypedBuf<bool> hbEnvVis;
	bool m_bHoriz;
	bool m_bVert;

public:
	ArrangeState():hbTrackHeights(256),hbTrackVis(256),hbEnvHeights(256),hbEnvVis(256)  { Clear(); }
	void Clear()
	{
		dHZoom = 0.0; iXPos = 0; iYPos = 0; m_bHoriz = false; m_bVert = false;
		hbTrackHeights.Resize(0, false);
		hbTrackVis.Resize(0, false);
		hbEnvHeights.Resize(0, false);
		hbEnvVis.Resize(0, false);
	}
	void Save(bool bSaveHoriz, bool bSaveVert)
	{
		HWND hTrackView = GetTrackWnd();
		if (!hTrackView)
			return;

		m_bHoriz = bSaveHoriz;
		m_bVert  = bSaveVert;
		SCROLLINFO si = { sizeof(SCROLLINFO), };
		si.fMask = SIF_ALL;

		// Horiz
		if (m_bHoriz)
		{
			dHZoom = GetHZoomLevel();
			CoolSB_GetScrollInfo(hTrackView, SB_HORZ, &si);
			iXPos = si.nPos;
		}

		// Vert
		if (m_bVert)
		{
			fVZoom = get_reaper_vzoom();
			hbTrackHeights.Resize(GetNumTracks()+1, false);
			hbTrackVis.Resize(GetNumTracks()+1, false);
			hbEnvHeights.Resize(0, false);
			hbEnvVis.Resize(0, false);
			for (int i = 0; i <= GetNumTracks(); i++)
			{
				MediaTrack* tr = CSurf_TrackFromID(i, false);
				hbTrackHeights.Get()[i] = *(int*)GetSetMediaTrackInfo(tr, "I_HEIGHTOVERRIDE", NULL);
				hbTrackVis.Get()[i] = GetTrackVis(tr);
				int iNumEnvelopes = CountTrackEnvelopes(tr);
				if (iNumEnvelopes)
				{
					int iEnv = hbEnvHeights.GetSize();
					hbEnvHeights.Resize(iEnv + iNumEnvelopes);
					hbEnvVis.Resize(iEnv + iNumEnvelopes);
					for (int j = 0; j < iNumEnvelopes; j++)
					{
						BR_Envelope envelope(GetTrackEnvelope(tr, j));
						hbEnvHeights.Get()[iEnv] = envelope.GetLaneHeight();
						hbEnvVis.Get()[iEnv] = envelope.IsVisible();
						iEnv++;
					}
				}
			}
			CoolSB_GetScrollInfo(hTrackView, SB_VERT, &si);
			iYPos = si.nPos;
		}
	}

	void Restore()
	{
		HWND hTrackView = GetTrackWnd();
		if (!hTrackView || !(m_bHoriz || m_bVert))
			return;

		PreventUIRefresh(1);

		// Horiz zoom
		if (m_bHoriz)
			adjustZoom(dHZoom, 1, false, -1);

		// Vert zoom
		if (m_bVert)
		{
			set_reaper_vzoom(fVZoom);
			int iSaved = hbTrackHeights.GetSize();
			int iEnvPtr = 0;
			for (int i = 0; i <= GetNumTracks(); i++)
			{
				MediaTrack* tr = CSurf_TrackFromID(i, false);
				if (i < iSaved)
				{
					SetTrackVis(tr, hbTrackVis.Get()[i]);
					GetSetMediaTrackInfo(tr, "I_HEIGHTOVERRIDE", hbTrackHeights.Get()+i);
				}
				else
					GetSetMediaTrackInfo(tr, "I_HEIGHTOVERRIDE", &g_i0);

				for (int j = 0; j < CountTrackEnvelopes(tr); j++)
				{
					BR_Envelope envelope(GetTrackEnvelope(tr, j));
					if (iEnvPtr < hbEnvHeights.GetSize())
					{
						envelope.SetVisible(hbEnvVis.Get()[iEnvPtr]);
						envelope.SetLaneHeight(hbEnvHeights.Get()[iEnvPtr]);
						iEnvPtr++;
					}
					else
						envelope.SetLaneHeight(0);
					envelope.Commit();
				}
			}

			TrackList_AdjustWindows(false);
			UpdateTimeline();

			SetVertPos(hTrackView, iYPos, true);
		}

		if (m_bHoriz)
			SetHorizPos(hTrackView, (double)iXPos / dHZoom);

		SaveZoomSlice(true);
		PreventUIRefresh(-1);
	}
};

static SWSProjConfig<ArrangeState> g_stdAS[5];
static SWSProjConfig<ArrangeState> g_togAS;
static bool g_bASToggled = false;

// iType == 0 tracks, 1 items or time sel, 2 just items, 3 horiz items or time sel, 4 horiz items, 5 horiz time sel
// iOthers == 0 nothing, 1 minimize, 2 hide from TCP
void TogZoom(int iType, int iOthers, bool includeEnvelopes)
{
	if (g_bASToggled)
	{
		g_togAS.Get()->Restore();
		g_bASToggled = false;
		return;
	}

	double d1, d2;
	GetSet_LoopTimeRange(false, false, &d1, &d2, false);
	const int itemCount = CountSelectedMediaItems(NULL);

	if (((iType == 1 || iType == 3) && itemCount == 0 && d1 == d2) ||
		((iType == 2 || iType == 4) && itemCount == 0)             ||
		(iType == 5                 && d1 == d2)
	   )
	{
		return;
	}


	g_bASToggled = true;
	g_togAS.Get()->Save(true, iType < 3);

	if (iType == 0)
	{
		Main_OnCommand(40031, 0);
		VertZoomSelTracks(iOthers, includeEnvelopes);
	}
	else
	{
		if (d1 != d2 && (iType == 1 || iType == 3 || iType == 5))
			Main_OnCommand(40031, 0); // View: Zoom time selection
		else
			HorizZoomSelItems();
		if (iType < 3)
			VertZoomSelItems(iOthers, includeEnvelopes);
	}
}

void TogZoomTT(COMMAND_T* ct)				{ TogZoom(0, 0, (int)ct->user == 0); }
void TogZoomTTMin(COMMAND_T* ct)			{ TogZoom(0, 1, (int)ct->user == 0); }
void TogZoomTTHide(COMMAND_T* ct)			{ TogZoom(0, 2, (int)ct->user == 0); }
void TogZoomItems(COMMAND_T* ct)			{ TogZoom(1, 0, (int)ct->user == 0); }
void TogZoomItemsMin(COMMAND_T* ct)			{ TogZoom(1, 1, (int)ct->user == 0); }
void TogZoomItemsHide(COMMAND_T* ct)		{ TogZoom(1, 2, (int)ct->user == 0); }
void TogZoomItemsOnly(COMMAND_T* ct)		{ TogZoom(2, 0, (int)ct->user == 0); }
void TogZoomItemsOnlyMin(COMMAND_T* ct)		{ TogZoom(2, 1, (int)ct->user == 0); }
void TogZoomItemsOnlyHide(COMMAND_T* ct)	{ TogZoom(2, 2, (int)ct->user == 0); }
void TogZoomHoriz(COMMAND_T* ct)			{ TogZoom((int)ct->user, 0, true); }

void SaveCurrentArrangeViewSlot(COMMAND_T* ct)	{ g_stdAS[(int)ct->user].Get()->Save(true, true); }
void RestoreArrangeViewSlot(COMMAND_T* ct)      { g_stdAS[(int)ct->user].Get()->Restore(); }

// Returns the track at a point on the track view window
// Point is in client coords
MediaTrack* TrackAtPoint(HWND hTrackView, int iY, int* iOffset, int* iYMin, int* iYMax)
{
	SCROLLINFO si = { sizeof(SCROLLINFO), };
	si.fMask = SIF_ALL;
	CoolSB_GetScrollInfo(hTrackView, SB_VERT, &si);

	int iVPos = -si.nPos; // Account for current scroll pos
	int iTrack = 0;
	int iTrackH = 0;

	// Find the current track #
	while (iTrack <= GetNumTracks())
	{
		MediaTrack* track = CSurf_TrackFromID(iTrack, false);
		iTrackH = *(int*)GetSetMediaTrackInfo(track, "I_WNDH", NULL);
		if (iVPos + iTrackH > iY)
			break;
		if (iTrack == 0 && TcpVis(track) && iTrackH != 0)
			iTrackH += GetMasterTcpGap();
		iVPos += iTrackH;
		iTrack++;
	}

	// Set extents if outside of std region
	if (iYMin)
		*iYMin = iVPos;
	if (iYMax)
		*iYMax = iVPos;

	if (iTrack <= GetNumTracks())
	{
		if (iYMin)
			*iYMin = iVPos;
		if (iYMax)
			*iYMax = iVPos + iTrackH;
		if (iOffset)
			*iOffset = iY - iVPos;
		return CSurf_TrackFromID(iTrack, false);
	}
	return NULL;
}

// Returns the (first) item at the point p in the trackview.
// p is in client coords; rExtents can extend well past the ClientRect
MediaItem* ItemAtPoint(HWND hTrackView, POINT p, RECT* rExtents, MediaTrack** pTr)
{
	if (rExtents) rExtents->left = rExtents->right = 0; // Init rExtents

	// First get the track
	int iY1, iY2;
	MediaTrack* tr = TrackAtPoint(hTrackView, p.y, NULL, &iY1, &iY2);
	if (pTr)
		*pTr = tr;
	if (rExtents)
	{
		rExtents->top = iY1;
		rExtents->bottom = iY2;
	}
	if (!tr)
		return NULL;

	// Convert x coord to time
	SCROLLINFO si = { sizeof(SCROLLINFO), };
	si.fMask = SIF_ALL;
	CoolSB_GetScrollInfo(hTrackView, SB_HORZ, &si); // Get the current scroll pos
	RECT r;
	GetClientRect(hTrackView, &r);	// Get the window width
	double dPos = (p.x + si.nPos) / GetHZoomLevel();

	// Then, maybe find an item
	for (int i = 0; i < GetTrackNumMediaItems(tr); i++)
	{
		MediaItem* mi = GetTrackMediaItem(tr, i);
		double dStart = *(double*)GetSetMediaItemInfo(mi, "D_POSITION", NULL);
		double dEnd   = *(double*)GetSetMediaItemInfo(mi, "D_LENGTH", NULL) + dStart;
		if (dPos >= dStart && dPos <= dEnd)
		{
			if (rExtents)
			{
				rExtents->left  = (int)(GetHZoomLevel() * dStart + 0.5) - si.nPos;
				rExtents->right = (int)(GetHZoomLevel() * dEnd + 0.5) - si.nPos;
			}
			return mi;
		}
	}
	return NULL;
}

// Class for saving/restoring the zoom state.  This is a lighter-weight version
// of the arrange state class above.
// To be perfect you need to GetSetEnvelopeState (for envelope info)
// This class ignores envelope stuffs and other things at the risk of the restore being "off" if tracks
// are added, deleted, or if envs are added, deleted, sizes changed, etc.  It does support track height however.
// This should be fixed! TRP
class ZoomState
{
private:
	// Zoom
	WDL_TypedBuf<int> m_iTrackHeights;
	double m_dHZoom;
	float m_fVZoom;
	bool m_bProjExtents;

	// Pos
	MediaTrack* m_trVPos;
	int m_iVPos, m_iHPos;

public:
	ZoomState():m_iTrackHeights(256),m_dHZoom(0.0),m_fVZoom(0.f),m_iVPos(0),m_iHPos(0),m_trVPos(NULL),m_bProjExtents(false) {}
	void SaveZoom()
	{
		// Save the track heights
		m_iTrackHeights.Resize(0, false);
		int* pHeights = m_iTrackHeights.Resize(GetNumTracks()+1); // +1 for master
		for (int i = 0; i <= GetNumTracks(); i++)
			pHeights[i] = *(int*)GetSetMediaTrackInfo(CSurf_TrackFromID(i, false), "I_HEIGHTOVERRIDE", NULL);

		m_dHZoom = GetHZoomLevel();
		m_fVZoom = get_reaper_vzoom();
		m_bProjExtents = false;
	}
	void ZoomToProject()
	{
		VertZoomRange(0, GetNumTracks()+1, NULL, false, true);
		Main_OnCommand(40295, 0);
		SaveZoom();
		SavePos();
		m_bProjExtents = true;
	}
	bool IsZoomedToProject() { return m_bProjExtents; }
	void SavePos()
	{
		HWND hTrackView = GetTrackWnd();
		if (!hTrackView)
			return;

		m_trVPos = TrackAtPoint(hTrackView, 0, &m_iVPos, NULL, NULL);

		SCROLLINFO si = { sizeof(SCROLLINFO), };
		si.fMask = SIF_ALL;
		CoolSB_GetScrollInfo(hTrackView, SB_HORZ, &si);
		m_iHPos = si.nPos;
	}

	void Restore()
	{
		if (m_bProjExtents)
			ZoomToProject();

		HWND hTrackView = GetTrackWnd();
		if (!hTrackView || (m_dHZoom == 0.0 && m_fVZoom == 0.f))
			return;

		adjustZoom(m_dHZoom, 1, false, -1);
		set_reaper_vzoom(m_fVZoom);

		// Restore track heights, ignoring the fact that tracks could have been added/removed
		for (int i = 0; i < m_iTrackHeights.GetSize() && i <= GetNumTracks(); i++)
			GetSetMediaTrackInfo(CSurf_TrackFromID(i, false), "I_HEIGHTOVERRIDE", &(m_iTrackHeights.Get()[i]));

		TrackList_AdjustWindows(false);
		UpdateTimeline();

		// Restore positions
		int iTrack = CSurf_TrackToID(m_trVPos, false);
		if (iTrack >= 0)
			SetVertPos(hTrackView, iTrack, false, m_iVPos);
		SetHorizPos(hTrackView, (double)m_iHPos / m_dHZoom);
		SaveZoomSlice(true);
	}

	bool IsZoomEqual(ZoomState* zs)
	{	// Ignores the horiz/vert positions!
		if (zs->m_dHZoom != m_dHZoom || zs->m_fVZoom != m_fVZoom)
			return false;
		if (zs->m_iTrackHeights.GetSize() != m_iTrackHeights.GetSize())
			return false;
		for (int i = 0; i < m_iTrackHeights.GetSize(); i++)
			if (zs->m_iTrackHeights.Get()[i] != m_iTrackHeights.Get()[i])
				return false;

		return true;
	}
	// Debug
	//void Print(int i)
	//{
	//	dprintf("ZoomState %d: H: %.2f @ %d, V: %g @ %d, Proj: %s\n", i, m_dHZoom, m_iHPos, m_fVZoom, m_iVPos, m_bProjExtents ? "yes" : "no");
	//}
};

static SWSProjConfig<WDL_PtrList_DOD<ZoomState> > g_zoomStack;
static SWSProjConfig<int> g_zoomLevel;

// Undo zoom prefs
static bool g_bUndoSWSOnly = false;
static bool g_bLastUndoProj = false;

// Save the current zoom state, called from the slice
void SaveZoomSlice(bool bSWS)
{
	// Do the initialization of g_zoomLevel, as project changes can result in new PtrLists
	if (g_zoomStack.Get()->GetSize() == 0)
		*g_zoomLevel.Get() = 0;

	static ZoomState* zs = NULL;
	if (!zs)
		zs = new ZoomState;

	zs->SaveZoom();

	if (g_zoomStack.Get()->GetSize() == 0 || !g_zoomStack.Get()->Get(*g_zoomLevel.Get())->IsZoomEqual(zs))
	{	// The zoom level has been changed by the user!
		if (bSWS || !g_bUndoSWSOnly || g_zoomStack.Get()->GetSize() == 0)
		{
			while (*g_zoomLevel.Get())
			{
				g_zoomStack.Get()->Delete(0, true);
				*g_zoomLevel.Get() -= 1;
			}

			// Trim the bottom of the stack if it's huge.
			if (g_zoomStack.Get()->GetSize() == 50)
				g_zoomStack.Get()->Delete(49, true);

			g_zoomStack.Get()->Insert(0, zs);
		}
		else
		{
			delete g_zoomStack.Get()->Get(*g_zoomLevel.Get());
			g_zoomStack.Get()->Set(*g_zoomLevel.Get(), zs);
		}

		zs = NULL;
	}

	// Save the position for every call
	if (g_zoomStack.Get()->GetSize())
		g_zoomStack.Get()->Get(*g_zoomLevel.Get())->SavePos();
}

void UndoZoom(COMMAND_T* = NULL)
{
	if (*g_zoomLevel.Get() + 1 < g_zoomStack.Get()->GetSize())
	{
		*g_zoomLevel.Get() += 1;
		g_zoomStack.Get()->Get(*g_zoomLevel.Get())->Restore();
	}
	else if (g_bLastUndoProj)
	{
		if (!g_zoomStack.Get()->GetSize() || !g_zoomStack.Get()->Get(*g_zoomLevel.Get())->IsZoomedToProject())
		{
			*g_zoomLevel.Get() += 1;
			g_zoomStack.Get()->Add(new ZoomState);
		}
		g_zoomStack.Get()->Get(g_zoomStack.Get()->GetSize()-1)->ZoomToProject();
	}
}

void RedoZoom(COMMAND_T*)
{
	if (*g_zoomLevel.Get() > 0)
	{
		*g_zoomLevel.Get() -= 1;
		g_zoomStack.Get()->Get(*g_zoomLevel.Get())->Restore();
	}
}

void CreateZoomRect(HWND h, RECT* newR, POINT* p1, POINT* p2)
{
	RECT r;
	if (p1->x < p2->x)	{ r.left = p1->x; r.right = p2->x; }
	else				{ r.left = p2->x; r.right = p1->x; }
	if (p1->y < p2->y)	{ r.top = p1->y; r.bottom = p2->y; }
	else				{ r.top = p2->y; r.bottom = p1->y; }
	RECT clientR;
	GetClientRect(h, &clientR);
	IntersectRect(newR, &r, &clientR);
}

static bool g_bZooming = false;
static WNDPROC g_ReaperTrackWndProc = NULL;
static WNDPROC g_ReaperRulerWndProc = NULL;
static HCURSOR g_hZoomInCur = NULL;
static HCURSOR g_hZoomOutCur = NULL;
static HCURSOR g_hZoomUndoCur = NULL;
static HCURSOR g_hZoomDragCur = NULL;

// Zoom tool Prefs
// (defaults are actually set in ZoomInit)
static bool g_bMidMouseButton = false;
static int  g_iMidMouseModifier = 0;
static bool g_bItemZoom = false;
static bool g_bUndoZoom = false;
static bool g_bUnzoomMode = false;
static bool g_bDragUpUndo = false;
static bool g_bSetCursor = false;
static bool g_bSeekPlay = false;
static bool g_bSetTimesel = false;
static bool g_bDragZoomUpper = false;
static bool g_bDragZoomLower = false;
static double g_dDragZoomScale = 0.1;

// Zoom into a given rectangle on Reaper's track view
// If dX1 != dX2, use those as times in s to horiz zoom to
void ZoomToRect(HWND hTrackView, RECT* rZoom, double dX1, double dX2)
{
	RECT r;
	GetClientRect(hTrackView, &r); // Need for width of view wnd

	if (dX1 != dX2)
	{	// Use these doubles as the x points (more accurate for item zooms!)
		//  Add 3% on each side for some "breathing" room, just like in Reaper zooms
		adjustZoom(0.94 * (r.right - r.left) / (dX2 - dX1), 1, false, -1);
		SetHorizPos(hTrackView, dX1, 0.03);
	}
	else if (rZoom->left != rZoom->right)
	{	// Convert the rect left/right points to pixels, where 0 is the start of the project.
		// These are same units as the scroll bar.
		int iX1, iX2;
		SCROLLINFO si = { sizeof(SCROLLINFO), };
		si.fMask = SIF_ALL;
		CoolSB_GetScrollInfo(hTrackView, SB_HORZ, &si); // Get the current scroll pos
		iX1 = rZoom->left + si.nPos;
		iX2 = rZoom->right + si.nPos;
		dX1 = (double)iX1 / GetHZoomLevel();
		dX2 = (double)iX2 / GetHZoomLevel();
		adjustZoom(GetHZoomLevel() * r.right / (iX2 - iX1), 1, false, -1);
		SetHorizPos(hTrackView, dX1);
	}

	if (g_bSetCursor)
		SetEditCurPos(dX1, false, g_bSeekPlay);
	if (g_bSetTimesel)
		GetSet_LoopTimeRange(true, false, &dX1, &dX2, true);

	// Now, find the track(s) at the rect top/bottom and zoom in
	if (rZoom->top != rZoom->bottom)
	{
		MediaTrack* tr = TrackAtPoint(hTrackView, rZoom->top, NULL, NULL, NULL);
		if (tr)
		{	// If no track returned, user zoomed over the "dead" area at the bottom
			int iFirst = CSurf_TrackToID(tr, false);
			int iNum;
			tr = TrackAtPoint(hTrackView, rZoom->bottom-1, NULL, NULL, NULL);
			if (tr)
				iNum = CSurf_TrackToID(tr, false) - iFirst + 1;
			else
				iNum = GetNumTracks() - iFirst + 1;
			VertZoomRange(iFirst, iNum, NULL, false, true);
		}
	}
	SaveZoomSlice(true);
}

void SetZoomCursor(HWND hwnd, POINT pStart)
{
	HCURSOR newCur = NULL;
	POINT p;
	GetCursorPos(&p);
	ScreenToClient(hwnd, &p);

	if (g_bUnzoomMode && pStart.y - p.y > 2)
		newCur = g_hZoomOutCur;
	else if ((g_bDragUpUndo && pStart.y - p.y > 2) || (g_bUndoZoom && abs(p.x - pStart.x) <= 2 && abs(p.y - pStart.y) <= 2 && !ItemAtPoint(hwnd, p, NULL, NULL)))
		newCur = g_hZoomUndoCur;
	else
		newCur = g_hZoomInCur;

	if (GetCursor() != newCur && newCur)
		SetCursor(newCur);
}

LRESULT CALLBACK ZoomWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
#ifdef _WIN32
	static RECT rDraw;
#endif
	static RECT rZoom;
	static MediaItem* itemZoom;
	static bool bUndoZoom;
	static POINT pStart;

	if (uMsg == WM_START_ZOOM || (g_bMidMouseButton && uMsg == WM_MBUTTONDOWN && SWS_GetModifiers() == g_iMidMouseModifier))
	{
		POINT p;
		RECT r;
		GetCursorPos(&p);
		GetWindowRect(hwnd, &r);
		if (PtInRect(&r, p))
		{
			ScreenToClient(hwnd, &p);
			SetZoomCursor(hwnd, p);
		}
		g_bZooming = true;
		RefreshToolbar(SWSGetCommandID(ZoomTool));
		itemZoom = NULL;
		bUndoZoom = false;
		pStart.x = pStart.y = 0;
		rZoom.left = rZoom.top = rZoom.right = rZoom.bottom = 0;
#ifdef _WIN32
		rDraw.left = rDraw.top = rDraw.right = rDraw.bottom = 0;
#endif
	}
	if (g_bZooming)
	{
#ifdef _WIN32
		static LICE_SysBitmap* bmStd = NULL;
#else
		static void* pFocusRect = NULL;
#endif
		static bool bMBDown = false;

		if (uMsg == WM_LBUTTONDOWN || (g_bMidMouseButton && uMsg == WM_MBUTTONDOWN && SWS_GetModifiers() == g_iMidMouseModifier))
		{
			GetCursorPos(&pStart);
			ScreenToClient(hwnd, &pStart);
			SetCapture(hwnd);
			bMBDown = true;
		}
		else if (uMsg == WM_SETCURSOR)
		{
			SetZoomCursor(hwnd, pStart);
			return 1;
		}
#ifdef _WIN32
		else if (uMsg == WM_PAINT)
		{
			// If there's a paint request then let std wnd handle, then get the data
			g_ReaperTrackWndProc(hwnd, uMsg, wParam, lParam);
			delete bmStd;
			PAINTSTRUCT ps;
			HDC dc = BeginPaint(hwnd, &ps);
			RECT r;
			GetClientRect(hwnd, &r);
			bmStd = new LICE_SysBitmap(r.right - r.left, r.bottom - r.top);
			BitBlt(bmStd->getDC(), 0, 0, bmStd->getWidth(), bmStd->getHeight(), dc, 0, 0, SRCCOPY);
			EndPaint(hwnd, &ps);
			return 0;
		}
#endif
		if (uMsg == WM_MOUSEMOVE || uMsg == WM_LBUTTONDOWN || (g_bMidMouseButton && uMsg == WM_MBUTTONDOWN && SWS_GetModifiers() == g_iMidMouseModifier) || uMsg == WM_START_ZOOM)
		{
			// There's a lot of rects here:
			// rBox - the drawn rectangle (win only)
			// rZoom - the area you're zooming into, also the shaded area
			// rDraw - a union of rBox and rZoom to get the full draw area.
			//         The union takes care of outside of track area case.
			//         Also U the old draw area to erase old stuff properly
			// rLastDraw - previous mouse move draw area
			// finally r, rDraw U rLastDraw, to erase the old and add some new

			if (pStart.x != 0 && pStart.y != 0)
				SetZoomCursor(hwnd, pStart);

			static RECT rBox;
			POINT p;
			GetCursorPos(&p);
			ScreenToClient(hwnd, &p);
			if (!bMBDown)
				pStart = p;
			CreateZoomRect(hwnd, &rBox, &pStart, &p);

			rZoom = rBox; // Zoom area starts as the drawn box
			itemZoom = NULL;
			bUndoZoom = false;

			// Check for really small move, and do item zoom mode if so
			if (rBox.right - rBox.left <= 2 && rBox.bottom - rBox.top <= 2)
			{
				if (g_bItemZoom)
				{	// Change zoom area to match item
					itemZoom = ItemAtPoint(hwnd, p, &rZoom, NULL);
					if (itemZoom)
					{
						rZoom.left++;
						rZoom.right++;
					}
					else
						rZoom.top = rZoom.bottom;
				}
			}
			else if (g_bDragUpUndo && pStart.y - p.y > 2)
			{
				bUndoZoom = true;
				rZoom.top = rZoom.bottom;
				rZoom.left = rZoom.right;
				rBox.top = rBox.bottom;
			}
			else
			{
				int y;
				TrackAtPoint(hwnd, rZoom.top, NULL, &y, NULL);
				rZoom.top = y;
				TrackAtPoint(hwnd, rZoom.bottom, NULL, NULL, &y);
				rZoom.bottom = y;
				rZoom.right++;
			}

#ifdef _WIN32
			HDC dc = GetDC(hwnd);
			if (!dc)
				return 0;

			// Get the entire screen
			if (!bmStd)
			{
				RECT r;
				GetClientRect(hwnd, &r);
				bmStd = new LICE_SysBitmap(r.right - r.left, r.bottom - r.top);
				BitBlt(bmStd->getDC(), 0, 0, bmStd->getWidth(), bmStd->getHeight(), dc, 0, 0, SRCCOPY);
			}

			// Are we in "unzoom" mode?
			if (g_bUnzoomMode && pStart.y - p.y > 2)
			{
				// Adjust "rBox" to be around the mini-view
				RECT rMini;
				rMini.left = pStart.x - bmStd->getWidth() / 20;
				rMini.right = rMini.left + bmStd->getWidth() / 10;
				rMini.top = pStart.y - bmStd->getHeight() / 20;
				rMini.bottom = rMini.top + bmStd->getHeight() / 10;
				UnionRect(&rBox, &rBox, &rMini);
				rBox.bottom += rMini.top - rBox.top;
				rBox.right += rMini.left - rBox.left;

				GetClientRect(hwnd, &rDraw);

				rZoom.left = (rBox.left - rMini.left) * 10;
				rZoom.right = bmStd->getWidth() + (rBox.right - rMini.right) * 10;
				rZoom.top = (rBox.top - rMini.top) * 10;
				rZoom.bottom = bmStd->getHeight() + (rBox.bottom - rMini.bottom) * 10;

				LICE_SysBitmap bm(bmStd->getWidth(), bmStd->getHeight());
				LICE_Copy(&bm, bmStd);
				LICE_FillRect(&bm, 0, 0, bm.getWidth(), bm.getHeight(), LICE_RGBA(0, 0, 0, 255), 0.7f, LICE_BLIT_MODE_COPY);
				LICE_ScaledBlit(&bm, bmStd, rMini.left, rMini.top, rMini.right - rMini.left, rMini.bottom - rMini.top, 0.0, 0.0, (float)bmStd->getWidth(), (float)bmStd->getHeight(), 1.0, LICE_BLIT_MODE_COPY);
				LICE_DrawRect(&bm, rBox.left, rBox.top, rBox.right - rBox.left - 1, rBox.bottom - rBox.top - 1, LICE_RGBA(255, 255, 255, 150), 1.0, LICE_BLIT_MODE_COPY);
				BitBlt(dc, 0, 0, bm.getWidth(), bm.getHeight(), bm.getDC(), 0, 0, SRCCOPY);

				// Now that we're done drawing, adjust rZoom to 0 height if necessary
				// This is to avoid unintentional vert zoom per mbn http://github.com/reaper-oss/sws/issues/178#c14
				if (rMini.top == rBox.top && rMini.bottom == rBox.bottom)
					rZoom.top = rZoom.bottom;
			}
			else
			{
				RECT rLastDraw = rDraw;
				// Trim rZoom left/right as it might be large from item zoom (or others)
				if (rZoom.left < 0) rZoom.left = 0;
				if (rZoom.right > bmStd->getWidth()) rZoom.right = bmStd->getWidth();

				UnionRect(&rDraw, &rBox, &rZoom);
				UnionRect(&rDraw, &rDraw, &rLastDraw);

				LICE_SysBitmap bm(rDraw.right - rDraw.left + 1, rDraw.bottom - rDraw.top + 1);
				LICE_Blit(&bm, bmStd, 0, 0, rDraw.left, rDraw.top, bm.getWidth(), bm.getHeight(), 1.0, LICE_BLIT_MODE_COPY);

				// Don't draw small boxes
				if (rZoom.right - rZoom.left > 1 && rZoom.bottom - rZoom.top > 1)
					LICE_FillRect(&bm, rZoom.left - rDraw.left, rZoom.top - rDraw.top, rZoom.right - rZoom.left, rZoom.bottom - rZoom.top, LICE_RGBA(128, 128, 110, 150), 1.0, LICE_BLIT_MODE_HSVADJ);
				if (rBox.right - rBox.left > 1 && rBox.bottom - rBox.top > 1)
					LICE_DrawRect(&bm, rBox.left - rDraw.left, rBox.top - rDraw.top, rBox.right - rBox.left - 1, rBox.bottom - rBox.top - 1, LICE_RGBA(255, 255, 255, 150), 1.0, LICE_BLIT_MODE_COPY);
				BitBlt(dc, rDraw.left, rDraw.top, bm.getWidth(), bm.getHeight(), bm.getDC(), 0, 0, SRCCOPY);
			}

			ReleaseDC(hwnd, dc);
#else
			RECT r, rDraw = rZoom;
			GetClientRect(hwnd, &r);
			if (rDraw.left   < r.left)   rDraw.left   = r.left;  // You'd think IntersectRect would work here, but it doesnt for some reason.
			if (rDraw.right  > r.right)  rDraw.right  = r.right;
			if (rDraw.top    < r.top)    rDraw.top    = r.top;
			if (rDraw.bottom > r.bottom) rDraw.bottom = r.bottom;
			SWELL_DrawFocusRect(hwnd, &rDraw, &pFocusRect);
#endif
			return 0;
		}
		// Done with the zoom
		else if (uMsg == WM_LBUTTONUP || uMsg == WM_RBUTTONUP || uMsg == WM_MBUTTONUP || uMsg == WM_CANCEL_ZOOM)
		{
			if (GetCapture() == hwnd)
				ReleaseCapture();

#ifdef _WIN32
			delete bmStd;
			bmStd = NULL;
#else
			SWELL_DrawFocusRect(hwnd, NULL, &pFocusRect);
#endif
			g_bZooming = false;
			SendMessage(hwnd, WM_SETCURSOR, (WPARAM)hwnd, 0);
			RefreshToolbar(SWSGetCommandID(ZoomTool));

			if (bMBDown && (uMsg == WM_LBUTTONUP || (g_bMidMouseButton && uMsg == WM_MBUTTONUP)))
			{
				if (itemZoom)
				{
					double dX1 = *(double*)GetSetMediaItemInfo(itemZoom, "D_POSITION", NULL);
					double dX2 = *(double*)GetSetMediaItemInfo(itemZoom, "D_LENGTH", NULL) + dX1;
					ZoomToRect(hwnd, &rZoom, dX1, dX2);
				}
				else if (rZoom.right - rZoom.left > 1 || rZoom.bottom - rZoom.top > 1)
					ZoomToRect(hwnd, &rZoom, 0.0, 0.0);
				else if (g_bUndoZoom || bUndoZoom)
					UndoZoom();
				else
					UpdateTimeline();
			}
			else
				UpdateTimeline();

			bMBDown = false;
			return 0;
		}
	}

	return g_ReaperTrackWndProc(hwnd, uMsg, wParam, lParam);
}

enum { UPPER = 1, LOWER = 2 }; // Part of time display where drag started
#define UPPER_DEADBAND 3   // pixels
#define LOWER_DEADBAND 5

LRESULT CALLBACK DragZoomWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (g_bDragZoomUpper || g_bDragZoomLower)
	{
		static POINT ptPrev;
		static int dragArea;
		static bool bDragStart = false;
		static bool bDragging = false;
		static double dOrigPos;

		switch (uMsg)
		{
			case WM_LBUTTONDOWN:
			{
				ptPrev.x = GET_X_LPARAM(lParam);
				ptPrev.y = GET_Y_LPARAM(lParam);
				RECT r;
				GetWindowRect(hwnd, &r);
				if (ptPrev.y * 2 <= abs(r.bottom - r.top) + 1)
					dragArea = UPPER;
				else
					dragArea = LOWER;
				bDragStart = (dragArea == UPPER && g_bDragZoomUpper) || (dragArea == LOWER && g_bDragZoomLower);
				break;
			}

			case WM_LBUTTONUP:
				bDragStart = false;
				bDragging = false;
				break;

			case WM_MOUSEMOVE:
				if (bDragStart)
				{
					POINT ptClient;
					ptClient.x = GET_X_LPARAM(lParam);
					ptClient.y = GET_Y_LPARAM(lParam);

					int xDelta = ptClient.x - ptPrev.x;
					int yDelta = ptClient.y - ptPrev.y;
					ptPrev.x = ptClient.x;

					// Don't start drag zooming until we've crossed a threshold
					// Then, then take over control of mouse movements (don't call the main wndproc)
					if (!bDragging && ((dragArea == UPPER && abs(yDelta) < UPPER_DEADBAND) || (dragArea == LOWER && abs(yDelta) < LOWER_DEADBAND)))
						break;

					if (!bDragging)
					{
						bDragging = true;
						ptPrev.y = ptClient.y;
						yDelta = yDelta > 0 ? 1 : -1; // Smooth engagement
						SCROLLINFO si = { sizeof(SCROLLINFO), };
						si.fMask = SIF_ALL;
						CoolSB_GetScrollInfo(GetTrackWnd(), SB_HORZ, &si);
						dOrigPos = (si.nPos + ptClient.x) / GetHZoomLevel();
					}

					if (!xDelta && !yDelta)
						return 0;

// Setcursor just doesn't work well on OSX :( (the cursor jumps),
// ...and SetCursorPos works, but it's SLOW
#ifdef _WIN32
					SetCursor(g_hZoomDragCur);
#endif

					if (yDelta)
					{
#ifdef _WIN32

						// Keep the cursor from moving up and down regardless of actual mouse movement
						POINT ptScreen = ptClient;
						ptScreen.y = ptPrev.y;
						ClientToScreen(hwnd, &ptScreen);
						SetCursorPos(ptScreen.x, ptScreen.y);
#else
						ptPrev.y = ptClient.y;
#endif
						adjustZoom((double)yDelta * g_dDragZoomScale, 0, false, 3);
					}

					static double dPrevPos = 0.0;
					double dNewPos = dOrigPos - (ptClient.x / GetHZoomLevel());
					if (dNewPos != dPrevPos)
					{
						SetHorizPos(NULL, dNewPos);
						dPrevPos = dNewPos;
					}

					return 0;
				}
				break;
		}
	}
	return g_ReaperRulerWndProc(hwnd, uMsg, wParam, lParam);
}

void EnableDragZoom(COMMAND_T* _ct)
{
	if ((int)_ct->user)
		g_bDragZoomUpper = !g_bDragZoomUpper;
	else
		g_bDragZoomLower = !g_bDragZoomLower;
}

int IsDragZoomEnabled(COMMAND_T* _ct) {
	return (int)_ct->user ? g_bDragZoomUpper : g_bDragZoomLower;
}

void ZoomTool(COMMAND_T*)
{
	HWND hTrackView = GetTrackWnd();
	if (!hTrackView)
		return;

	// Handle start/cancel from the wnd proc to keep things in one place
	if (!g_bZooming)
		SendMessage(hTrackView, WM_START_ZOOM, 0, 0);
	else
		SendMessage(hTrackView, WM_CANCEL_ZOOM, 0, 0);
}

int IsZoomMode(COMMAND_T*)
{
	return g_bZooming;
}

#define ZOOMPREFS_WNDPOS_KEY "ZoomPrefs WndPos"

static INT_PTR WINAPI ZoomPrefsProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwndDlg, uMsg, wParam, lParam))
		return r;

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			CheckDlgButton(hwndDlg, IDC_MMZOOM, g_bMidMouseButton);
			CheckDlgButton(hwndDlg, IDC_ITEMZOOM, g_bItemZoom);
			CheckDlgButton(hwndDlg, IDC_UNDOZOOM, g_bUndoZoom);
#ifdef _WIN32
			if (g_bUnzoomMode)
				CheckDlgButton(hwndDlg, IDC_DRAGUP_UNZOOM, BST_CHECKED);
			else
#else
			ShowWindow(GetDlgItem(hwndDlg, IDC_DRAGUP_UNZOOM), SW_HIDE);
#endif
			if (g_bDragUpUndo)
				CheckDlgButton(hwndDlg, IDC_DRAGUP_UNDO, BST_CHECKED);
			else
				CheckDlgButton(hwndDlg, IDC_DRAGUP_ZOOM, BST_CHECKED);
			CheckDlgButton(hwndDlg, IDC_UNDOSWSONLY, g_bUndoSWSOnly);
			CheckDlgButton(hwndDlg, IDC_LASTUNDOPROJ, g_bLastUndoProj);
			for (int i = 0; i < NUM_MODIFIERS; i++)
				SendMessage(GetDlgItem(hwndDlg, IDC_MMMODIFIER), CB_ADDSTRING, 0, (LPARAM)g_modifiers[i].cDesc);
			for (int i = 0; i < NUM_MODIFIERS; i++)
				if (g_iMidMouseModifier == g_modifiers[i].iModifier)
				{
					SendMessage(GetDlgItem(hwndDlg, IDC_MMMODIFIER), CB_SETCURSEL, (WPARAM)i, 0);
					break;
				}
			EnableWindow(GetDlgItem(hwndDlg, IDC_MMMODIFIER), g_bMidMouseButton);
			CheckDlgButton(hwndDlg, IDC_MOVECUR, g_bSetCursor);
			CheckDlgButton(hwndDlg, IDC_SEEKPLAY, g_bSeekPlay);
			EnableWindow(GetDlgItem(hwndDlg, IDC_SEEKPLAY), g_bSetCursor);
			CheckDlgButton(hwndDlg, IDC_SETTIMESEL, g_bSetTimesel);
			CheckDlgButton(hwndDlg, IDC_DRAGUPPER, g_bDragZoomUpper);
			CheckDlgButton(hwndDlg, IDC_DRAGLOWER, g_bDragZoomLower);
			char str[314];
			sprintf(str, "%.2f", g_dDragZoomScale);
			SetWindowText(GetDlgItem(hwndDlg, IDC_DRAGSCALE), str);
			EnableWindow(GetDlgItem(hwndDlg, IDC_DRAGSCALE), g_bDragZoomUpper || g_bDragZoomLower);

			RestoreWindowPos(hwndDlg, ZOOMPREFS_WNDPOS_KEY, false);
			return 0;
		}
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDC_MMZOOM:
					EnableWindow(GetDlgItem(hwndDlg, IDC_MMMODIFIER), IsDlgButtonChecked(hwndDlg, IDC_MMZOOM) == BST_CHECKED);
					break;
				case IDC_DRAGUPPER:
				case IDC_DRAGLOWER:
					g_bDragZoomUpper  = IsDlgButtonChecked(hwndDlg, IDC_DRAGUPPER) == BST_CHECKED;
					g_bDragZoomLower  = IsDlgButtonChecked(hwndDlg, IDC_DRAGLOWER) == BST_CHECKED;
					EnableWindow(GetDlgItem(hwndDlg, IDC_DRAGSCALE), g_bDragZoomUpper || g_bDragZoomLower);
					break;
				case IDC_MOVECUR:
					g_bSetCursor	  = IsDlgButtonChecked(hwndDlg, IDC_MOVECUR) == BST_CHECKED;
					EnableWindow(GetDlgItem(hwndDlg, IDC_SEEKPLAY), g_bSetCursor);
					break;
				case IDOK:
				{
					g_bMidMouseButton = IsDlgButtonChecked(hwndDlg, IDC_MMZOOM) == BST_CHECKED;
					g_bItemZoom       = IsDlgButtonChecked(hwndDlg, IDC_ITEMZOOM) == BST_CHECKED;
					g_bUndoZoom       = IsDlgButtonChecked(hwndDlg, IDC_UNDOZOOM) == BST_CHECKED;
#ifdef _WIN32
					g_bUnzoomMode     = IsDlgButtonChecked(hwndDlg, IDC_DRAGUP_UNZOOM) == BST_CHECKED;
#endif
					g_bDragUpUndo     = IsDlgButtonChecked(hwndDlg, IDC_DRAGUP_UNDO) == BST_CHECKED;
					g_bUndoSWSOnly    = IsDlgButtonChecked(hwndDlg, IDC_UNDOSWSONLY) == BST_CHECKED;
					g_bLastUndoProj   = IsDlgButtonChecked(hwndDlg, IDC_LASTUNDOPROJ) == BST_CHECKED;
					g_iMidMouseModifier = g_modifiers[SendMessage(GetDlgItem(hwndDlg, IDC_MMMODIFIER), CB_GETCURSEL, 0, 0)].iModifier;
					g_bSetCursor	  = IsDlgButtonChecked(hwndDlg, IDC_MOVECUR) == BST_CHECKED;
					g_bSeekPlay       = IsDlgButtonChecked(hwndDlg, IDC_SEEKPLAY) == BST_CHECKED;
					g_bSetTimesel	  = IsDlgButtonChecked(hwndDlg, IDC_SETTIMESEL) == BST_CHECKED;
					g_bDragZoomUpper  = IsDlgButtonChecked(hwndDlg, IDC_DRAGUPPER) == BST_CHECKED;
					g_bDragZoomLower  = IsDlgButtonChecked(hwndDlg, IDC_DRAGLOWER) == BST_CHECKED;
					char str[32];
					GetWindowText(GetDlgItem(hwndDlg, IDC_DRAGSCALE), str, 32);
					g_dDragZoomScale  = atof(str);
				}
				// Fall through to cancel to save/end
				case IDCANCEL:
					SaveWindowPos(hwndDlg, ZOOMPREFS_WNDPOS_KEY);
					EndDialog(hwndDlg, 0);
					break;
			}
			break;
	}
	return 0;
}


void ZoomPrefs(COMMAND_T*)
{
	DialogBox(g_hInst,MAKEINTRESOURCE(IDD_ZOOMPREFS), g_hwndParent, ZoomPrefsProc);
}

static bool ProcessExtensionLine(const char *line, ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
	return false;
}
static void SaveExtensionConfig(ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg)
{
}

static void BeginLoadProjectState(bool isUndo, struct project_config_extension_t *reg)
{
	for (int i = 0; i < 5; ++i)
		g_stdAS[i].Get()->Clear();
	for (int i = 0; i < 5; ++i)
		g_stdAS[i].Cleanup();

	g_togAS.Get()->Clear();
	g_togAS.Cleanup();
	g_bASToggled = false;
	g_zoomStack.Cleanup();
	g_zoomLevel.Cleanup();
}

int IsTogZoomed(COMMAND_T*)
{
	return g_bASToggled;
}

static project_config_extension_t g_projectconfig = { ProcessExtensionLine, SaveExtensionConfig, BeginLoadProjectState, NULL };

//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] =
{
	{ { DEFACCEL, "SWS: Set reaper window size to reaper.ini setwndsize" }, "SWS_SETWINDOWSIZE", SetReaperWndSize,    NULL, },
	{ { DEFACCEL, "SWS: Horizontal scroll to put edit cursor at 10%" },     "SWS_HSCROLL10",     ScrollToCursor,      NULL, 10, },
	{ { DEFACCEL, "SWS: Horizontal scroll to put edit cursor at 50%" },     "SWS_HSCROLL50",     ScrollToCursor,      NULL, 50, },
	{ { DEFACCEL, "SWS: Horizontal scroll to put play cursor at 10%" },     "SWS_HSCROLLPLAY10", ScrollToCursor,      NULL, -10, },
	{ { DEFACCEL, "SWS: Horizontal scroll to put play cursor at 50%" },     "SWS_HSCROLLPLAY50", ScrollToCursor,      NULL, -50, },

	{ { DEFACCEL, "SWS: Vertical zoom to selected tracks" },                                                       "SWS_VZOOMFIT",             FitSelTracks,        NULL, 0},
	{ { DEFACCEL, "SWS: Vertical zoom to selected tracks, minimize others" },                                      "SWS_VZOOMFITMIN",          FitSelTracksMin,     NULL, 0},
	{ { DEFACCEL, "SWS: Vertical zoom to selected items" },                                                        "SWS_VZOOMIITEMS",          VZoomToSelItems,     NULL, 0},
	{ { DEFACCEL, "SWS: Vertical zoom to selected items, minimize others" },                                       "SWS_VZOOMITEMSMIN",        VZoomToSelItemsMin,  NULL, 0},
	{ { DEFACCEL, "SWS: Vertical zoom to selected tracks (ignore last track's envelope lanes)" },                  "SWS_VZOOMFIT_NO_ENV",      FitSelTracks,        NULL, 1},
	{ { DEFACCEL, "SWS: Vertical zoom to selected tracks, minimize others (ignore last track's envelope lanes)" }, "SWS_VZOOMFITMIN_NO_ENV",   FitSelTracksMin,     NULL, 1},
	{ { DEFACCEL, "SWS: Vertical zoom to selected items (ignore last track's envelope lanes)" },                   "SWS_VZOOMIITEMS_NO_ENV",   VZoomToSelItems,     NULL, 1},
	{ { DEFACCEL, "SWS: Vertical zoom to selected items, minimize others (ignore last track's envelope lanes)" },  "SWS_VZOOMITEMSMIN_NO_ENV", VZoomToSelItemsMin,  NULL, 1},

	{ { DEFACCEL, "SWS: Horizontal zoom to selected items" }, "SWS_HZOOMITEMS",          HZoomToSelItems,     NULL, },

	{ { DEFACCEL, "SWS: Zoom to selected items" },                                                                         "SWS_ITEMZOOM",           ZoomToSelItems,      NULL, 0},
	{ { DEFACCEL, "SWS: Zoom to selected items, minimize others" },                                                        "SWS_ITEMZOOMMIN",        ZoomToSelItemsMin,   NULL, 0},
	{ { DEFACCEL, "SWS: Zoom to selected items or time selection" },                                                       "SWS_ZOOMSIT",            ZoomToSIT,           NULL, 0},
	{ { DEFACCEL, "SWS: Zoom to selected items or time selection, minimize others" },                                      "SWS_ZOOMSITMIN",         ZoomToSITMin,        NULL, 0},
	{ { DEFACCEL, "SWS: Zoom to selected items (ignore last track's envelope lanes)" },                                    "SWS_ITEMZOOM_NO_ENV",    ZoomToSelItems,      NULL, 1},
	{ { DEFACCEL, "SWS: Zoom to selected items, minimize others (ignore last track's envelope lanes)" },                   "SWS_ITEMZOOMMIN_NO_ENV", ZoomToSelItemsMin,   NULL, 1},
	{ { DEFACCEL, "SWS: Zoom to selected items or time selection (ignore last track's envelope lanes)" },                  "SWS_ZOOMSIT_NO_ENV",     ZoomToSIT,           NULL, 1},
	{ { DEFACCEL, "SWS: Zoom to selected items or time selection, minimize others (ignore last track's envelope lanes)" }, "SWS_ZOOMSITMIN_NO_ENV",  ZoomToSITMin,        NULL, 1},

	{ { DEFACCEL, "SWS: Toggle zoom to selected tracks and time selection" },                                                           "SWS_TOGZOOMTT",               TogZoomTT,           NULL, 0, IsTogZoomed },
	{ { DEFACCEL, "SWS: Toggle zoom to selected tracks and time selection, minimize others" },                                          "SWS_TOGZOOMTTMIN",            TogZoomTTMin,        NULL, 0, IsTogZoomed },
	{ { DEFACCEL, "SWS: Toggle zoom to selected tracks and time selection, hide others" },                                              "SWS_TOGZOOMTTHIDE",           TogZoomTTHide,       NULL, 0, IsTogZoomed },
	{ { DEFACCEL, "SWS: Toggle zoom to selected items or time selection" },                                                             "SWS_TOGZOOMI",                TogZoomItems,        NULL, 0, IsTogZoomed },
	{ { DEFACCEL, "SWS: Toggle zoom to selected items or time selection, minimize other tracks" },                                      "SWS_TOGZOOMIMIN",             TogZoomItemsMin,     NULL, 0, IsTogZoomed },
	{ { DEFACCEL, "SWS: Toggle zoom to selected items or time selection, hide other tracks" },                                          "SWS_TOGZOOMIHIDE",            TogZoomItemsHide,    NULL, 0, IsTogZoomed },
	{ { DEFACCEL, "SWS: Toggle zoom to selected items" },                                                                               "SWS_TOGZOOMIONLY",            TogZoomItemsOnly,    NULL, 0, IsTogZoomed },
	{ { DEFACCEL, "SWS: Toggle zoom to selected items, minimize other tracks" },                                                        "SWS_TOGZOOMIONLYMIN",         TogZoomItemsOnlyMin, NULL, 0, IsTogZoomed },
	{ { DEFACCEL, "SWS: Toggle zoom to selected items, hide other tracks" },                                                            "SWS_TOGZOOMIONLYHIDE",        TogZoomItemsOnlyHide,NULL, 0, IsTogZoomed },
	{ { DEFACCEL, "SWS: Toggle zoom to selected tracks and time selection (ignore last track's envelope lanes)" },                      "SWS_TOGZOOMTT_NO_ENV",        TogZoomTT,           NULL, 1, IsTogZoomed },
	{ { DEFACCEL, "SWS: Toggle zoom to selected tracks and time selection, minimize others (ignore last track's envelope lanes)" },     "SWS_TOGZOOMTTMIN_NO_ENV",     TogZoomTTMin,        NULL, 1, IsTogZoomed },
	{ { DEFACCEL, "SWS: Toggle zoom to selected tracks and time selection, hide others (ignore last track's envelope lanes)" },         "SWS_TOGZOOMTTHIDE_NO_ENV",    TogZoomTTHide,       NULL, 1, IsTogZoomed },
	{ { DEFACCEL, "SWS: Toggle zoom to selected items or time selection (ignore last track's envelope lanes)" },                        "SWS_TOGZOOMI_NO_ENV",         TogZoomItems,        NULL, 1, IsTogZoomed },
	{ { DEFACCEL, "SWS: Toggle zoom to selected items or time selection, minimize other tracks (ignore last track's envelope lanes)" }, "SWS_TOGZOOMIMIN_NO_ENV",      TogZoomItemsMin,     NULL, 1, IsTogZoomed },
	{ { DEFACCEL, "SWS: Toggle zoom to selected items or time selection, hide other tracks (ignore last track's envelope lanes)" },     "SWS_TOGZOOMIHIDE_NO_ENV",     TogZoomItemsHide,    NULL, 1, IsTogZoomed },
	{ { DEFACCEL, "SWS: Toggle zoom to selected items (ignore last track's envelope lanes)" },                                          "SWS_TOGZOOMIONLY_NO_ENV",     TogZoomItemsOnly,    NULL, 1, IsTogZoomed },
	{ { DEFACCEL, "SWS: Toggle zoom to selected items, minimize other tracks (ignore last track's envelope lanes)" },                   "SWS_TOGZOOMIONLYMIN_NO_ENV",  TogZoomItemsOnlyMin, NULL, 1, IsTogZoomed },
	{ { DEFACCEL, "SWS: Toggle zoom to selected items, hide other tracks (ignore last track's envelope lanes)" },                       "SWS_TOGZOOMIONLYHIDE_NO_ENV", TogZoomItemsOnlyHide,NULL, 1, IsTogZoomed },

	{ { DEFACCEL, "SWS: Toggle horizontal zoom to selected items or time selection" }, "SWS_TOGZOOMHORIZ",        TogZoomHoriz,        NULL, 3, IsTogZoomed },
	{ { DEFACCEL, "SWS: Toggle horizontal zoom to selected items" },                   "SWS_TOGZOOMHORIZ_ITEMS",  TogZoomHoriz,        NULL, 4, IsTogZoomed },
	{ { DEFACCEL, "SWS: Toggle horizontal zoom to time selection" },                   "SWS_TOGZOOMHORIZ_TSEL",   TogZoomHoriz,        NULL, 5, IsTogZoomed },

	{ { DEFACCEL, "SWS: Scroll left 10%" },  "SWS_SCROLL_L10",  HorizScroll, NULL, -10 },
	{ { DEFACCEL, "SWS: Scroll right 10%" }, "SWS_SCROLL_R10",  HorizScroll, NULL, 10 },
	{ { DEFACCEL, "SWS: Scroll left 1%" },   "SWS_SCROLL_L1",   HorizScroll, NULL, -1 },
	{ { DEFACCEL, "SWS: Scroll right 1%" },  "SWS_SCROLL_R1",   HorizScroll, NULL, 1 },

	{ { DEFACCEL, "SWS: Save current arrange view, slot 1" }, "SWS_SAVEVIEW",      SaveCurrentArrangeViewSlot, NULL, 0 },
	{ { DEFACCEL, "SWS: Save current arrange view, slot 2" }, "WOL_SAVEVIEWS2",    SaveCurrentArrangeViewSlot, NULL, 1 },
	{ { DEFACCEL, "SWS: Save current arrange view, slot 3" }, "WOL_SAVEVIEWS3",    SaveCurrentArrangeViewSlot, NULL, 2 },
	{ { DEFACCEL, "SWS: Save current arrange view, slot 4" }, "WOL_SAVEVIEWS4",    SaveCurrentArrangeViewSlot, NULL, 3 },
	{ { DEFACCEL, "SWS: Save current arrange view, slot 5" }, "WOL_SAVEVIEWS5",    SaveCurrentArrangeViewSlot, NULL, 4 },
	{ { DEFACCEL, "SWS: Restore arrange view, slot 1" },      "SWS_RESTOREVIEW",   RestoreArrangeViewSlot, NULL, 0 },
	{ { DEFACCEL, "SWS: Restore arrange view, slot 2" },      "WOL_RESTOREVIEWS2", RestoreArrangeViewSlot, NULL, 1 },
	{ { DEFACCEL, "SWS: Restore arrange view, slot 3" },      "WOL_RESTOREVIEWS3", RestoreArrangeViewSlot, NULL, 2 },
	{ { DEFACCEL, "SWS: Restore arrange view, slot 4" },      "WOL_RESTOREVIEWS4", RestoreArrangeViewSlot, NULL, 3 },
	{ { DEFACCEL, "SWS: Restore arrange view, slot 5" },      "WOL_RESTOREVIEWS5", RestoreArrangeViewSlot, NULL, 4 },

	{ { DEFACCEL, "SWS: Undo zoom" },           "SWS_UNDOZOOM",  UndoZoom,  NULL, },
	{ { DEFACCEL, "SWS: Redo zoom" },           "SWS_REDOZOOM",  RedoZoom,  NULL, },
	{ { DEFACCEL, "SWS: Zoom tool (marquee)" }, "SWS_ZOOM",      ZoomTool,  NULL, 0, IsZoomMode },
	{ { DEFACCEL, "SWS: Zoom preferences" },    "SWS_ZOOMPREFS", ZoomPrefs, "SWS Zoom preferences...", },

	{ { DEFACCEL, "SWS: Toggle drag zoom enable (ruler bottom half)" }, "SWS_EN_DRAGZOOM_BOT", EnableDragZoom, NULL, 0, IsDragZoomEnabled},
	{ { DEFACCEL, "SWS: Toggle drag zoom enable (ruler top half)" },    "SWS_EN_DRAGZOOM_TOP", EnableDragZoom, NULL, 1, IsDragZoomEnabled},

	{ { DEFACCEL, NULL }, NULL, NULL, SWS_SEPARATOR, }, // for main Extensions menu
	{ {}, LAST_COMMAND, }, // Denote end of table
};
//!WANT_LOCALIZE_1ST_STRING_END

void ZoomSlice()
{
	static bool bRecurseCheck = false;
	if (!bRecurseCheck)
	{
		bRecurseCheck = true;
		SaveZoomSlice(false);
		bRecurseCheck = false;
	}
	else
		dprintf("Zoom slice recurse!\n");
}

static int translateAccel(MSG *msg, accelerator_register_t *ctx)
{
	// Catch the ESC key when zooming to cancel
	if (g_bZooming && msg->message == WM_KEYDOWN && msg->wParam == VK_ESCAPE)
	{
		SendMessage(GetTrackWnd(), WM_CANCEL_ZOOM, 0, 0);
		return 1;
	}
	return 0;
}

static accelerator_register_t g_ar = { translateAccel, TRUE, NULL };


#define ZOOMPREFS_KEY "ZoomPrefs"
#define DRAGZOOMSCALE_KEY "DragZoomScale"
int ZoomInit(bool hookREAPERWndProcs)
{
	if (hookREAPERWndProcs)
	{
		HWND hTrackView = GetTrackWnd();
		if (hTrackView) g_ReaperTrackWndProc = (WNDPROC)SetWindowLongPtr(hTrackView, GWLP_WNDPROC, (LONG_PTR)ZoomWndProc);
		HWND hRuler = GetRulerWnd();
		if (hRuler) g_ReaperRulerWndProc = (WNDPROC)SetWindowLongPtr(hRuler, GWLP_WNDPROC, (LONG_PTR)DragZoomWndProc);
		return 1;
	}

	if (!plugin_register("accelerator",&g_ar))
		return 0;
	SWSRegisterCommands(g_commandTable);
	if (!plugin_register("projectconfig",&g_projectconfig))
		return 0;

	g_hZoomInCur   = GetSwsMouseCursor(CURSOR_ZOOM_IN);
	g_hZoomOutCur  = GetSwsMouseCursor(CURSOR_ZOOM_OUT);
	g_hZoomUndoCur = GetSwsMouseCursor(CURSOR_ZOOM_UNDO);
	g_hZoomDragCur = GetSwsMouseCursor(CURSOR_ZOOM_DRAG);

	// Restore prefs
	int iPrefs = GetPrivateProfileInt(SWS_INI, ZOOMPREFS_KEY, 0, get_ini_file());
	g_bMidMouseButton = !!(iPrefs & 1);
	g_bItemZoom = !!(iPrefs & 2);
	g_bUndoZoom = !!(iPrefs & 4);
	g_bUnzoomMode = !!(iPrefs & 8);
	g_bUndoSWSOnly = !!(iPrefs & 16);
	g_bLastUndoProj = !!(iPrefs & 32);
	g_bDragUpUndo = !!(iPrefs & 64);
	g_iMidMouseModifier = (iPrefs >> 8) & 0x7;
	g_bSetCursor = !!(iPrefs & 2048);
	g_bSetTimesel = !!(iPrefs & 4096);
	g_bSeekPlay = !!(iPrefs & 8192);
	g_bDragZoomUpper = !!(iPrefs & 16384);
	g_bDragZoomLower = !!(iPrefs & 32768);
	char str[32];
	GetPrivateProfileString(SWS_INI, DRAGZOOMSCALE_KEY, "0.1", str, 32, get_ini_file());
	g_dDragZoomScale = atof(str);
	return 1;
}

void ZoomExit()
{
	plugin_register("-accelerator",&g_ar);
	plugin_register("-projectconfig",&g_projectconfig);

	// Write the zoom prefs
	char str[314];
	sprintf(str, "%d", (g_bMidMouseButton ? 1 : 0) + (g_bItemZoom ? 2 : 0) + (g_bUndoZoom ? 4 : 0) +
		(g_bUnzoomMode ? 8 : 0) + (g_bUndoSWSOnly ? 16 : 0) + (g_bLastUndoProj ? 32 : 0) +
		(g_bDragUpUndo ? 64 : 0) + (g_iMidMouseModifier << 8) + (g_bSetCursor ? 2048 : 0) +
		(g_bSetTimesel ? 4096 : 0) + (g_bSeekPlay ? 8192 : 0) + (g_bDragZoomUpper ? 16384 : 0) +
		(g_bDragZoomLower ? 32768 : 0));
	WritePrivateProfileString(SWS_INI, ZOOMPREFS_KEY, str, get_ini_file());
	sprintf(str, "%.2f", g_dDragZoomScale);
	WritePrivateProfileString(SWS_INI, DRAGZOOMSCALE_KEY, str, get_ini_file());
}
