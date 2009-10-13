/******************************************************************************
/ Color.cpp
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
#include "../Freeze/Freeze.h"
#include "Color.h"

#define COLORDLG_WINDOWPOS_KEY "ColorDlgPos"
#define GRADIENT_COLOR_KEY "ColorGradients"

static COLORREF g_custColors[16];
static COLORREF g_crGradStart = 0;
static COLORREF g_crGradEnd = 0;

void UpdateCustomColors()
{
	GetPrivateProfileStruct("REAPER", "custcolors", g_custColors, sizeof(g_custColors), get_ini_file());
}

bool AllBlack()
{
	UpdateCustomColors();
	for (int i = 0; i < 16; i++)
		if (g_custColors[i])
			return false;
	return true;
}

INT_PTR WINAPI doColorDlg(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
			RestoreWindowPos(hwndDlg, COLORDLG_WINDOWPOS_KEY, false);
			return 0;
		case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT pDI = (LPDRAWITEMSTRUCT)lParam;
			HBRUSH hb = NULL;
			switch (pDI->CtlID)
			{
				case IDC_COLOR1:
					hb = CreateSolidBrush(g_crGradStart);
					break;
				case IDC_COLOR2:
					hb = CreateSolidBrush(g_crGradEnd);
					break;
			}
			FillRect(pDI->hDC, &pDI->rcItem, hb);
			DeleteObject(hb);
			return 1;
		}
		case WM_COMMAND:
		{
			wParam = LOWORD(wParam);
			bool bChanged = false;
			switch(wParam)
			{
				case IDC_COLOR1:
				case IDC_COLOR2:
				case IDC_SETCUST:
				{
					CHOOSECOLOR cc;
					memset(&cc, 0, sizeof(CHOOSECOLOR));
					cc.lStructSize = sizeof(CHOOSECOLOR);
					cc.hwndOwner = hwndDlg;
					cc.lpCustColors = g_custColors;
					cc.Flags = CC_FULLOPEN | CC_RGBINIT;

					if (wParam == IDC_COLOR1)
					{
						cc.rgbResult = g_crGradStart;
						if (ChooseColor(&cc))
						{
							g_crGradStart = cc.rgbResult;
							bChanged = true;
						}
					}
					else if (wParam == IDC_COLOR2)
					{
						cc.rgbResult = g_crGradEnd;
						if (ChooseColor(&cc))
						{
							g_crGradEnd = cc.rgbResult;
							bChanged = true;
						}
					}
					else if (wParam == IDC_SETCUST)
					{
						if (ChooseColor(&cc))
							bChanged = true;
					}
				}
				break;
				case IDC_SAVECOL:
				case IDC_LOADCOL:
				case IDC_LOADFROMTHEME:
				{
					char filename[256] = { 0 };
					char cPath[256] = { 0 };
					GetPrivateProfileString("REAPER", "lastthemefn", "", cPath, 256, get_ini_file());
					char* pC = strrchr(cPath, '\\');
					if (!pC)
						pC = strrchr(cPath, '/');
					if (pC)
						*pC = 0;

					OPENFILENAME ofn;
					memset(&ofn, 0, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = hwndDlg;
					ofn.lpstrFilter = "SWS Color Files (*.SWSColor)\0*.SWSColor\0Color Theme Files (*.ReaperTheme)\0*.ReaperTheme\0All Files\0*.*\0";
					ofn.lpstrFile = filename;
					ofn.lpstrInitialDir = cPath;
					ofn.nMaxFile = 256;
#if _MSC_VER > 1310
					ofn.Flags = OFN_DONTADDTORECENT | OFN_EXPLORER;
#else
					ofn.Flags = OFN_EXPLORER;
#endif
					ofn.lpstrDefExt = "SWSColor";

					if (wParam == IDC_SAVECOL)
					{
						ofn.Flags |= OFN_OVERWRITEPROMPT;
						if (GetSaveFileName(&ofn))
						{
							char key[32];
							char val[32];
							for (int i = 0; i < 16; i++)
							{
								sprintf(key, "custcolor%d", i+1);
								sprintf(val, "%d", g_custColors[i]);
								WritePrivateProfileString("SWS Color", key, val, ofn.lpstrFile);
							}
							sprintf(val, "%d", g_crGradStart);
							WritePrivateProfileString("SWS Color", "gradientStart", val, ofn.lpstrFile);
							sprintf(val, "%d", g_crGradEnd);
							WritePrivateProfileString("SWS Color", "gradientEnd", val, ofn.lpstrFile);
						}
					}
					else if (wParam == IDC_LOADCOL || wParam == IDC_LOADFROMTHEME)
					{
						ofn.Flags |= OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
						if (wParam == IDC_LOADFROMTHEME || GetOpenFileName(&ofn))
						{
							char key[32];
							for (int i = 0; i < 16; i++)
							{
								sprintf(key, "custcolor%d", i+1);
								g_custColors[i] = GetPrivateProfileInt("SWS Color", key, g_custColors[i], ofn.lpstrFile);
							}
							g_crGradStart = GetPrivateProfileInt("SWS Color", "gradientStart", g_crGradStart, ofn.lpstrFile);
							g_crGradEnd   = GetPrivateProfileInt("SWS Color", "gradientEnd", g_crGradEnd, ofn.lpstrFile);
							bChanged = true;
						}
					}
				}
				break;
				case IDOK:
					// Do something ?
					// fall through!
				case IDCANCEL:
					SaveWindowPos(hwndDlg, COLORDLG_WINDOWPOS_KEY);
					EndDialog(hwndDlg,0);
					break;
			}

			if (bChanged)
			{
				char str[256];
				sprintf(str, "%d %d", g_crGradStart, g_crGradEnd);
				WritePrivateProfileString(SWS_INI, GRADIENT_COLOR_KEY, str, get_ini_file());
				WritePrivateProfileStruct("REAPER", "custcolors", g_custColors, sizeof(g_custColors), get_ini_file());
				RedrawWindow(hwndDlg, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
			}

			return 0;
		}
		case WM_DESTROY:
			// We're done
			return 0;
	}
	return 0;
}

void ShowColorDialog(COMMAND_T* = NULL)
{
	DialogBox(g_hInst,MAKEINTRESOURCE(IDD_COLOR),g_hwndParent,doColorDlg);
}

void ColorChildren(COMMAND_T* = NULL)
{
	int iParentDepth;
	COLORREF crParentColor;
	bool bSelected = false;
	MediaTrack* gfd = NULL;
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		int iType;
		int iFolder = GetFolderDepth(tr, &iType, &gfd);

		if (bSelected)
			GetSetMediaTrackInfo(tr, "I_CUSTOMCOLOR", &crParentColor);

		if (iType == 1)
		{
			if (!bSelected && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			{
				iParentDepth = iFolder;
				bSelected = true;
			}
			crParentColor = *(COLORREF*)GetSetMediaTrackInfo(tr, "I_CUSTOMCOLOR", NULL);
		}

		if (iType + iFolder <= iParentDepth)
			bSelected = false;
	}
	Undo_OnStateChangeEx("Set track(s) children to same color", UNDO_STATE_TRACKCFG, -1);
}


void WhiteItem(COMMAND_T* = NULL)
{
	int iWhite = 0x1000000 | RGB(255, 255, 255);
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* mi = GetTrackMediaItem(tr, j);
			if (*(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL))
				GetSetMediaItemInfo(mi, "I_CUSTOMCOLOR", &iWhite);
		}
	}
	Undo_OnStateChange("Set item(s) color white");
	UpdateTimeline();
}

void BlackItem(COMMAND_T* = NULL)
{
	int iBlack = 0x1000000 | RGB(0, 0, 0);
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* mi = GetTrackMediaItem(tr, j);
			if (*(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL))
				GetSetMediaItemInfo(mi, "I_CUSTOMCOLOR", &iBlack);
		}
	}
	Undo_OnStateChange("Set item(s) color black");
	UpdateTimeline();
}

void WhiteTrack(COMMAND_T* = NULL)
{
	int iWhite = 0x1000000 | RGB(255, 255, 255);
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			GetSetMediaTrackInfo(tr, "I_CUSTOMCOLOR", &iWhite);
	}
	Undo_OnStateChangeEx("Set track(s) color white", UNDO_STATE_TRACKCFG, -1);
}

void BlackTrack(COMMAND_T* = NULL)
{
	int iBlack = 0x1000000 | RGB(0, 0, 0);
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			GetSetMediaTrackInfo(tr, "I_CUSTOMCOLOR", &iBlack);
	}
	Undo_OnStateChangeEx("Set track(s) color black", UNDO_STATE_TRACKCFG, -1);
}

void ColorTrackPrev(COMMAND_T* = NULL)
{
	MediaTrack* prevTr = CSurf_TrackFromID(0, false);
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			GetSetMediaTrackInfo(tr, "I_CUSTOMCOLOR", GetSetMediaTrackInfo(prevTr, "I_CUSTOMCOLOR", NULL));
		prevTr = tr;
	}
	Undo_OnStateChangeEx("Set track(s) to previous track's color", UNDO_STATE_TRACKCFG, -1);
}

void ColorTrackNext(COMMAND_T* = NULL)
{
	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			GetSetMediaTrackInfo(tr, "I_CUSTOMCOLOR", GetSetMediaTrackInfo(CSurf_TrackFromID(i+1, false), "I_CUSTOMCOLOR", NULL));
	}
	Undo_OnStateChangeEx("Set track(s) to previous track's color", UNDO_STATE_TRACKCFG, -1);
}



COLORREF GetNextCustColor(COLORREF cr)
{
	int i;
	UpdateCustomColors();
	for (i = 0; i < 16; i++)
		if (cr == g_custColors[i])
			break;
	// Don't repeat!
	COLORREF newCr;
	for (int k = 0; k < 16; k++)
	{
		newCr = g_custColors[++i & 15];
		if (cr != newCr && newCr != 0)
			break;
	}
	return newCr;
}

void ColorTrackNextCust(COMMAND_T* = NULL)
{
	if (AllBlack())
		return;

	for (int j = 1; j <= GetNumTracks(); j++)
	{
		MediaTrack* tr = CSurf_TrackFromID(j, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			COLORREF cr = *(int*)GetSetMediaTrackInfo(tr, "I_CUSTOMCOLOR", NULL) & 0xFFFFFF;
			cr = GetNextCustColor(cr) | 0x1000000;
			GetSetMediaTrackInfo(tr, "I_CUSTOMCOLOR", &cr);
		}
	}
	Undo_OnStateChangeEx("Set track(s) to next custom color", UNDO_STATE_TRACKCFG, -1);
}

void ColorItemNextCust(COMMAND_T* = NULL)
{
	if (AllBlack())
		return;

	for (int j = 1; j <= GetNumTracks(); j++)
	{
		MediaTrack* tr = CSurf_TrackFromID(j, false);
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* mi = GetTrackMediaItem(tr, j);
			if (*(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL))
			{
				COLORREF cr = *(int*)GetSetMediaItemInfo(mi, "I_CUSTOMCOLOR", NULL) & 0xFFFFFF;
				cr = GetNextCustColor(cr) | 0x1000000;
				GetSetMediaItemInfo(mi, "I_CUSTOMCOLOR", &cr);
			}
		}
	}
	Undo_OnStateChange("Set item(s) to next custom color");
	UpdateTimeline();
}

void TrackRandomCol(COMMAND_T* = NULL)
{
	COLORREF cr;
	// All black check
	if (AllBlack())
		return;
	while (!(cr = g_custColors[(int)((double)rand() * 16.0 / (RAND_MAX+1))]));
	cr |= 0x1000000;
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			GetSetMediaTrackInfo(tr, "I_CUSTOMCOLOR", &cr);
	}
	Undo_OnStateChangeEx("Set track(s) to one random custom color", UNDO_STATE_TRACKCFG, -1);
}

void TrackRandomCols(COMMAND_T* = NULL)
{
	COLORREF cr;
	// All black check
	if (AllBlack())
		return;
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			while (!(cr = g_custColors[(int)((double)rand() * 16.0 / (RAND_MAX+1))]));
			cr |= 0x1000000;
			GetSetMediaTrackInfo(tr, "I_CUSTOMCOLOR", &cr);
		}
	}
	Undo_OnStateChangeEx("Set track(s) to random custom color(s)", UNDO_STATE_TRACKCFG, -1);
}

void ItemRandomCol(COMMAND_T* = NULL)
{
	COLORREF cr;
	// All black check
	if (AllBlack())
		return;
	while (!(cr = g_custColors[(int)((double)rand() * 16.0 / (RAND_MAX+1))]));
	cr |= 0x1000000;
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* mi = GetTrackMediaItem(tr, j);
			if (*(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL))
				GetSetMediaItemInfo(mi, "I_CUSTOMCOLOR", &cr);
		}
	}
	Undo_OnStateChange("Set item(s) to one random custom color");
	UpdateTimeline();
}

void ItemRandomCols(COMMAND_T* = NULL)
{
	COLORREF cr;
	// All black check
	if (AllBlack())
		return;
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* mi = GetTrackMediaItem(tr, j);
			if (*(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL))
			{
				while (!(cr = g_custColors[(int)((double)rand() * 16.0 / (RAND_MAX+1))]));
				cr |= 0x1000000;
				GetSetMediaItemInfo(mi, "I_CUSTOMCOLOR", &cr);
			}
		}
	}
	Undo_OnStateChange("Set item(s) to random custom color(s)");
	UpdateTimeline();
}

void TrackCustomColor(int iCustColor)
{
	COLORREF cr;
	UpdateCustomColors();
	cr = g_custColors[iCustColor] | 0x1000000;
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			GetSetMediaTrackInfo(tr, "I_CUSTOMCOLOR", &cr);
	}
	char cUndoText[100];
	sprintf(cUndoText, "Set track(s) to custom color %d", iCustColor+1);
	Undo_OnStateChangeEx(cUndoText, UNDO_STATE_TRACKCFG, -1);
}

void ItemCustomColor(int iCustColor)
{
	COLORREF cr;
	UpdateCustomColors();
	cr = g_custColors[iCustColor] | 0x1000000;
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* mi = GetTrackMediaItem(tr, j);
			if (*(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL))
				GetSetMediaItemInfo(mi, "I_CUSTOMCOLOR", &cr);
		}
	}
	char cUndoText[100];
	sprintf(cUndoText, "Set item(s) to custom color %d", iCustColor+1);
	Undo_OnStateChange(cUndoText);
	UpdateTimeline();
}

void TrackCustCol(COMMAND_T* ct) { TrackCustomColor((int)ct->user); }
void ItemCustCol(COMMAND_T* ct)  { ItemCustomColor((int)ct->user); }

COLORREF CalcGradient(COLORREF crStart, COLORREF crEnd, double dPos)
{
	int r, g, b;

	// Get the color differences
	r = (GetRValue(crEnd) - GetRValue(crStart));
	g = (GetGValue(crEnd) - GetGValue(crStart));
	b = (GetBValue(crEnd) - GetBValue(crStart));
	r = (int)((double)r * dPos + GetRValue(crStart));
	g = (int)((double)g * dPos + GetGValue(crStart));
	b = (int)((double)b * dPos + GetBValue(crStart));

	return RGB(r, g, b);
}

// Gradient stuffs
void TrackGradient(COMMAND_T* = NULL)
{
	int iNumSel = NumSelTracks();
	int iCurPos = 0;
	if (iNumSel < 2)
		return;
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			COLORREF cr = CalcGradient(g_crGradStart, g_crGradEnd, (double)iCurPos++ / (iNumSel-1)) | 0x1000000;
			GetSetMediaTrackInfo(tr, "I_CUSTOMCOLOR", &cr);
		}
	}
	Undo_OnStateChangeEx("Set tracks to color gradient", UNDO_STATE_TRACKCFG, -1);
}

void TrackOrderedCol(COMMAND_T* = NULL)
{
	int iCurPos = 0;
	UpdateCustomColors();
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			COLORREF cr = g_custColors[iCurPos++ % 16] | 0x1000000;
			GetSetMediaTrackInfo(tr, "I_CUSTOMCOLOR", &cr);
		}
	}
	Undo_OnStateChangeEx("Set track(s) to ordered custom color color(s)", UNDO_STATE_TRACKCFG, -1);
}

void ItemTrackGrad(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		int iCurPos = 0;
		int iNumSel = 0;
		// Count the number of selected items on this track
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
			if (*(bool*)GetSetMediaItemInfo(GetTrackMediaItem(tr, j), "B_UISEL", NULL))
				iNumSel++;
		// Now set colors
		if (iNumSel == 1)
		{
			for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
			{
				MediaItem* mi = GetTrackMediaItem(tr, j);
				if (*(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL))
				{
					COLORREF cr = g_crGradStart | 0x1000000;
					GetSetMediaItemInfo(mi, "I_CUSTOMCOLOR", &cr);
				}
			}
		}
		else if (iNumSel > 1)
		{
			for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
			{
				MediaItem* mi = GetTrackMediaItem(tr, j);
				if (*(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL))
				{
					COLORREF cr = CalcGradient(g_crGradStart, g_crGradEnd, (double)iCurPos++ / (iNumSel-1)) | 0x1000000;
					GetSetMediaItemInfo(mi, "I_CUSTOMCOLOR", &cr);
				}
			}
		}
	}
	Undo_OnStateChange("Set selected item(s) to color gradient per track");
	UpdateTimeline();
}

void ItemGradient(COMMAND_T* = NULL)
{
	int iCurPos = 0;
	int iNumSel = 0;
	// First, must count the number of selected items 
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
			if (*(bool*)GetSetMediaItemInfo(GetTrackMediaItem(tr, j), "B_UISEL", NULL))
				iNumSel++;
	}
	// Now, set the colors
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* mi = GetTrackMediaItem(tr, j);
			if (*(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL))
			{
				COLORREF cr = CalcGradient(g_crGradStart, g_crGradEnd, (double)iCurPos++ / (iNumSel-1)) | 0x1000000;
				GetSetMediaItemInfo(mi, "I_CUSTOMCOLOR", &cr);
			}
		}
	}
	Undo_OnStateChange("Set selected item(s) to color gradient");
	UpdateTimeline();
}

void ItemOrdColTrack(COMMAND_T* = NULL)
{
	UpdateCustomColors();
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		int iCurPos = 0;
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* mi = GetTrackMediaItem(tr, j);
			if (*(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL))
			{
				COLORREF cr = g_custColors[iCurPos++ % 16] | 0x1000000;
				GetSetMediaItemInfo(mi, "I_CUSTOMCOLOR", &cr);
			}
		}
	}
	Undo_OnStateChange("Set selected item(s) to ordered custom colors");
	UpdateTimeline();
}

void ItemOrderedCol(COMMAND_T* = NULL)
{
	int iCurPos = 0;
	UpdateCustomColors();
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* mi = GetTrackMediaItem(tr, j);
			if (*(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL))
			{
				COLORREF cr = g_custColors[iCurPos++ % 16] | 0x1000000;
				GetSetMediaItemInfo(mi, "I_CUSTOMCOLOR", &cr);
			}
		}
	}
	Undo_OnStateChange("Set selected item(s) to ordered custom colors");
	UpdateTimeline();
}

void ItemToTrackCol(COMMAND_T* = NULL)
{
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		COLORREF cr = *(COLORREF*)GetSetMediaTrackInfo(tr, "I_CUSTOMCOLOR", NULL);

		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* mi = GetTrackMediaItem(tr, j);
			if (*(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL))
				GetSetMediaItemInfo(mi, "I_CUSTOMCOLOR", &cr);
		}
	}
	Undo_OnStateChange("Set selected item(s) to respective track color");
	UpdateTimeline();
}

static COMMAND_T g_commandTable[] = 
{
	{ { DEFACCEL, "SWS: Open color management window" },                          "SWSCOLORWND",			ShowColorDialog,	"Color management...", },

	{ { DEFACCEL, "SWS: Set selected track(s) to color white" },	              "SWS_WHITETRACK",			WhiteTrack,			NULL, },
	{ { DEFACCEL, "SWS: Set selected track(s) to color black" },	              "SWS_BLACKTRACK",			BlackTrack,			NULL, },
	{ { DEFACCEL, "SWS: Set selected track(s) to previous track's color" },       "SWS_COLTRACKPREV",		ColorTrackPrev,		NULL, },
	{ { DEFACCEL, "SWS: Set selected track(s) to next track's color" },           "SWS_COLTRACKNEXT",		ColorTrackNext,		NULL, },
	{ { DEFACCEL, "SWS: Set selected track(s) to next custom color" },            "SWS_COLTRACKNEXTCUST",	ColorTrackNextCust,	NULL, },

	// Start of menu!!
	{ { DEFACCEL, "SWS: Set selected track(s) to one random custom color" },      "SWS_TRACKRANDCOL",		TrackRandomCol,		"Set to one random custom color", },
	{ { DEFACCEL, "SWS: Set selected track(s) to random custom color(s)" },       "SWS_TRACKRANDCOLS",		TrackRandomCols,	"Set to random custom color(s)", },
	{ { DEFACCEL, "SWS: Set selected tracks to color gradient" },                 "SWS_TRACKGRAD",			TrackGradient,		"Set to color gradient", },
	{ { DEFACCEL, "SWS: Set selected track(s) to ordered custom colors" },        "SWS_TRACKORDCOL",		TrackOrderedCol,	"Set to ordered custom colors", },
	{ { DEFACCEL, "SWS: Set selected track(s) children to same color" },          "SWS_COLCHILDREN",		ColorChildren,		"Set children to same color", },
	{ { DEFACCEL, "SWS: Set selected track(s) to custom color 1" },               "SWS_TRACKCUSTCOL1",		TrackCustCol,		"Set to custom color 1", 0 },
	{ { DEFACCEL, "SWS: Set selected track(s) to custom color 2" },               "SWS_TRACKCUSTCOL2",		TrackCustCol,		"Set to custom color 2", 1 },
	{ { DEFACCEL, "SWS: Set selected track(s) to custom color 3" },               "SWS_TRACKCUSTCOL3",		TrackCustCol,		"Set to custom color 3", 2 },
	{ { DEFACCEL, "SWS: Set selected track(s) to custom color 4" },               "SWS_TRACKCUSTCOL4",		TrackCustCol,		"Set to custom color 4", 3 },
	{ { DEFACCEL, "SWS: Set selected track(s) to custom color 5" },               "SWS_TRACKCUSTCOL5",		TrackCustCol,		"Set to custom color 5", 4 },
	{ { DEFACCEL, "SWS: Set selected track(s) to custom color 6" },               "SWS_TRACKCUSTCOL6",		TrackCustCol,		"Set to custom color 6", 5 },
	{ { DEFACCEL, "SWS: Set selected track(s) to custom color 7" },               "SWS_TRACKCUSTCOL7",		TrackCustCol,		"Set to custom color 7", 6 },
	{ { DEFACCEL, "SWS: Set selected track(s) to custom color 8" },               "SWS_TRACKCUSTCOL8",		TrackCustCol,		"Set to custom color 8", 7 },
	{ { DEFACCEL, "SWS: Set selected track(s) to custom color 9" },               "SWS_TRACKCUSTCOL9",		TrackCustCol,		"Set to custom color 9", 8 },
	{ { DEFACCEL, "SWS: Set selected track(s) to custom color 10" },              "SWS_TRACKCUSTCOL10",		TrackCustCol,		"Set to custom color 10", 9 },
	{ { DEFACCEL, "SWS: Set selected track(s) to custom color 11" },              "SWS_TRACKCUSTCOL11",		TrackCustCol,		"Set to custom color 11", 10 },
	{ { DEFACCEL, "SWS: Set selected track(s) to custom color 12" },              "SWS_TRACKCUSTCOL12",		TrackCustCol,		"Set to custom color 12", 11 },
	{ { DEFACCEL, "SWS: Set selected track(s) to custom color 13" },              "SWS_TRACKCUSTCOL13",		TrackCustCol,		"Set to custom color 13", 12 },
	{ { DEFACCEL, "SWS: Set selected track(s) to custom color 14" },              "SWS_TRACKCUSTCOL14",		TrackCustCol,		"Set to custom color 14", 13 },
	{ { DEFACCEL, "SWS: Set selected track(s) to custom color 15" },              "SWS_TRACKCUSTCOL15",		TrackCustCol,		"Set to custom color 15", 14 },
	{ { DEFACCEL, "SWS: Set selected track(s) to custom color 16" },              "SWS_TRACKCUSTCOL16",		TrackCustCol,		"Set to custom color 16", 15 },
	//  End of menu!!

	{ { DEFACCEL, "SWS: Set selected item(s) to color white" },	                  "SWS_WHITEITEM",			WhiteItem,			NULL, },
	{ { DEFACCEL, "SWS: Set selected item(s) to color black" },	                  "SWS_BLACKITEM",			BlackItem,			NULL, },
	{ { DEFACCEL, "SWS: Set selected item(s) to next custom color" },             "SWS_COLITEMNEXTCUST",	ColorItemNextCust,	NULL, },
	
	// Start of menu!!
	{ { DEFACCEL, "SWS: Set selected item(s) to one random custom color" },       "SWS_ITEMRANDCOL",		ItemRandomCol,		"Set to one random custom color", },
	{ { DEFACCEL, "SWS: Set selected item(s) to random custom color(s)" },        "SWS_ITEMRANDCOLS",		ItemRandomCols,		"Set to random custom color(s)", },
	{ { DEFACCEL, "SWS: Set selected item(s) to color gradient per track" },      "SWS_ITEMTRACKGRAD",		ItemTrackGrad,		"Set to color gradient per track", },
	{ { DEFACCEL, "SWS: Set selected item(s) to color gradient" },                "SWS_ITEMGRAD",			ItemGradient,		"Set to color gradient", },
	{ { DEFACCEL, "SWS: Set selected item(s) to ordered custom colors per track" },"SWS_ITEMORDCOLTRACK",	ItemOrdColTrack,	"Set to ordered custom colors per track", },
	{ { DEFACCEL, "SWS: Set selected item(s) to ordered custom colors" },         "SWS_ITEMORDCOL",			ItemOrderedCol,		"Set to ordered custom colors", },
	{ { DEFACCEL, "SWS: Set selected item(s) to respective track color" },        "SWS_ITEMTRKCOL",			ItemToTrackCol,		"Set to track color", },
	{ { DEFACCEL, "SWS: Set selected item(s) to custom color 1" },                "SWS_ITEMCUSTCOL1",		ItemCustCol,		"Set to custom color 1", 0 },
	{ { DEFACCEL, "SWS: Set selected item(s) to custom color 2" },                "SWS_ITEMCUSTCOL2",		ItemCustCol,		"Set to custom color 2", 1 },
	{ { DEFACCEL, "SWS: Set selected item(s) to custom color 3" },                "SWS_ITEMCUSTCOL3",		ItemCustCol,		"Set to custom color 3", 2 },
	{ { DEFACCEL, "SWS: Set selected item(s) to custom color 4" },                "SWS_ITEMCUSTCOL4",		ItemCustCol,		"Set to custom color 4", 3 },
	{ { DEFACCEL, "SWS: Set selected item(s) to custom color 5" },                "SWS_ITEMCUSTCOL5",		ItemCustCol,		"Set to custom color 5", 4 },
	{ { DEFACCEL, "SWS: Set selected item(s) to custom color 6" },                "SWS_ITEMCUSTCOL6",		ItemCustCol,		"Set to custom color 6", 5 },
	{ { DEFACCEL, "SWS: Set selected item(s) to custom color 7" },                "SWS_ITEMCUSTCOL7",		ItemCustCol,		"Set to custom color 7", 6 },
	{ { DEFACCEL, "SWS: Set selected item(s) to custom color 8" },                "SWS_ITEMCUSTCOL8",		ItemCustCol,		"Set to custom color 8", 7 },
	{ { DEFACCEL, "SWS: Set selected item(s) to custom color 9" },                "SWS_ITEMCUSTCOL9",		ItemCustCol,		"Set to custom color 9", 8 },
	{ { DEFACCEL, "SWS: Set selected item(s) to custom color 10" },               "SWS_ITEMCUSTCOL10",		ItemCustCol,		"Set to custom color 10", 9 },
	{ { DEFACCEL, "SWS: Set selected item(s) to custom color 11" },               "SWS_ITEMCUSTCOL11",		ItemCustCol,		"Set to custom color 11", 10 },
	{ { DEFACCEL, "SWS: Set selected item(s) to custom color 12" },               "SWS_ITEMCUSTCOL12",		ItemCustCol,		"Set to custom color 12", 11 },
	{ { DEFACCEL, "SWS: Set selected item(s) to custom color 13" },               "SWS_ITEMCUSTCOL13",		ItemCustCol,		"Set to custom color 13", 12 },
	{ { DEFACCEL, "SWS: Set selected item(s) to custom color 14" },               "SWS_ITEMCUSTCOL14",		ItemCustCol,		"Set to custom color 14", 13 },
	{ { DEFACCEL, "SWS: Set selected item(s) to custom color 15" },               "SWS_ITEMCUSTCOL15",		ItemCustCol,		"Set to custom color 15", 14 },
	{ { DEFACCEL, "SWS: Set selected item(s) to custom color 16" },               "SWS_ITEMCUSTCOL16",		ItemCustCol,		"Set to custom color 16", 15 },
	// End of menu!!

	{ {}, LAST_COMMAND, NULL }, // Denote end of table
};

static void menuhook(int menuid, HMENU hMenu, int flag)
{
	if ((menuid == CTXMENU_TCP || menuid == CTXMENU_ITEM) && flag == 0)
	{	// Initialize the menu
		void* pFirstCommand;
		void* pLastCommand;
		if (menuid == CTXMENU_TCP)
		{
			pFirstCommand = TrackRandomCol;
			pLastCommand  = TrackCustCol;
		}
		else
		{
			pFirstCommand = ItemRandomCol;
			pLastCommand  = ItemCustCol;
		}

		HMENU hSubMenu = CreatePopupMenu();
		int i = 0;
		while (g_commandTable[i].doCommand != pFirstCommand)
			i++;
		do
		{
			AddToMenu(hSubMenu, g_commandTable[i].menuText, g_commandTable[i].accel.accel.cmd);
			i++;
		}
		while (!(g_commandTable[i-1].doCommand == pLastCommand && g_commandTable[i-1].user == 15));

		// Finish with color dialog
		AddToMenu(hSubMenu, g_commandTable[0].menuText, g_commandTable[0].accel.accel.cmd);

		if (menuid == CTXMENU_TCP)
			AddSubMenu(hMenu, hSubMenu, "SWS track color", 40359);
		else if (menuid == CTXMENU_ITEM)
			AddSubMenu(hMenu, hSubMenu, "SWS item color", 40707);
	}
#ifdef _WIN32
	else if (flag == 1)
	{	// Update the color swatches
		// Color commands can be anywhere on the menu, so find 'em no matter where
		bool bInitialized = false;

		HDC hdcScreen;
		HDC hDC;

		int iCommand1 = SWSGetCommandID(TrackCustCol, 0);
		int iCommand2 = SWSGetCommandID(ItemCustCol, 0);

		for (int i = 0; i < 32; i++)
		{
			int iPos;
			HMENU h;
			if (i < 16)
				h = FindMenuItem(hMenu, iCommand1 + i, &iPos);
			else
				h = FindMenuItem(hMenu, iCommand2 + i-16, &iPos);
			if (h)
			{
				if (!bInitialized)
				{	// Reduce thrashing
					UpdateCustomColors();
					hdcScreen = CreateDC("DISPLAY", NULL, NULL, NULL); 
					hDC = CreateCompatibleDC(hdcScreen);
					bInitialized = true;
				}

				HBITMAP hBMP;
				int s = GetSystemMetrics(SM_CYMENUCHECK);
				RECT rColor = { 0, 0, s, s };
				RECT rSpace = { s, 0, s+3, s };
				MENUITEMINFO mi={sizeof(MENUITEMINFO),};
				mi.fMask = MIIM_BITMAP;

				GetMenuItemInfo(h, iPos, true, &mi);
				if (mi.hbmpItem)
					hBMP = mi.hbmpItem;
				else
					hBMP = CreateCompatibleBitmap(hdcScreen, rSpace.right, s);
				SelectObject(hDC, hBMP);
				HBRUSH hb = CreateSolidBrush(g_custColors[i%16]);
				FillRect(hDC, &rColor, hb);
				DeleteObject(hb);
				FillRect(hDC, &rSpace, (HBRUSH)(COLOR_MENU+1));
				if (!mi.hbmpItem)
				{
					mi.hbmpItem = hBMP;
					SetMenuItemInfo(h, iPos, true, &mi);
				}
			}
		}
	    if (bInitialized)
		{
			DeleteDC(hDC);
			DeleteDC(hdcScreen);
		}
	}
#endif
}

int ColorInit()
{
	SWSRegisterCommands(g_commandTable);
	srand((UINT)time(NULL));

	if (!plugin_register("hookmenu", menuhook))
		return 0;

	// Load color gradients from the INI file
	// Restore state
	char str[256];
	GetPrivateProfileString(SWS_INI, GRADIENT_COLOR_KEY, "0 16777215", str, 256, get_ini_file());
	LineParser lp(false);
	if (!lp.parse(str))
	{
		g_crGradStart = lp.gettoken_int(0);
		g_crGradEnd = lp.gettoken_int(1);
	}

	return 1;
}
