/******************************************************************************
/ Color.cpp
/
/ Copyright (c) 2011 Tim Payne (SWS)
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

#include "../Freeze/Freeze.h"
#include "../SnM/SnM_Dlg.h"
#include "Color.h"

#include <WDL/localize/localize.h>

#define COLORDLG_WINDOWPOS_KEY "ColorDlgPos"
#define GRADIENT_COLOR_KEY "ColorGradients"
#define RECREDRULER_KEY "RecRedRuler"

// Globals
COLORREF g_custColors[16];
COLORREF g_crGradStart = 0;
COLORREF g_crGradEnd = 0;
static bool g_bRecRedRuler = false;

void UpdateCustomColors()
{
#ifndef __APPLE__
	GetPrivateProfileStruct("REAPER", "custcolors", g_custColors, sizeof(g_custColors), get_ini_file());
#else
	GetCustomColors(g_custColors);
#endif
}

bool AllBlack()
{
	UpdateCustomColors();
	for (int i = 0; i < 16; i++)
		if (g_custColors[i])
			return false;
	return true;
}

void PersistColors()
{
	char str[256];
	sprintf(str, "%d %d", g_crGradStart, g_crGradEnd);
	WritePrivateProfileString(SWS_INI, GRADIENT_COLOR_KEY, str, get_ini_file());
#ifndef __APPLE__
	WritePrivateProfileStruct("REAPER", "custcolors", g_custColors, sizeof(g_custColors), get_ini_file());
#else
	SetCustomColors(g_custColors);
#endif
}

INT_PTR WINAPI doColorDlg(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
#ifdef __APPLE__
	static int iSettingColor = -1;
#endif
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwndDlg, uMsg, wParam, lParam))
		return r;

	switch (uMsg)
	{
		case WM_INITDIALOG:
			UpdateCustomColors();
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
#ifdef __APPLE__
		case WM_TIMER:
		{
			COLORREF cr;
			if (iSettingColor != -1 && GetChosenColor(&cr))
			{
				switch (iSettingColor)
				{
				case 0:
					g_crGradStart = cr;
					break;
				case 1:
					g_crGradEnd = cr;
					break;
				case 2:
					UpdateCustomColors();
					break;
				}
				PersistColors();
				InvalidateRect(hwndDlg, NULL, 0);
				KillTimer(hwndDlg, 1);
				iSettingColor = -1;
			}
			break;
		}
#endif
		case WM_COMMAND:
		{
			wParam = LOWORD(wParam);
			switch(wParam)
			{
				case IDC_COLOR1:
				case IDC_COLOR2:
				case IDC_SETCUST:
				{
#ifdef _WIN32
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
							PersistColors();
							RedrawWindow(hwndDlg, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
						}
					}
					else if (wParam == IDC_COLOR2)
					{
						cc.rgbResult = g_crGradEnd;
						if (ChooseColor(&cc))
						{
							g_crGradEnd = cc.rgbResult;
							PersistColors();
							RedrawWindow(hwndDlg, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
						}
					}
					else if (wParam == IDC_SETCUST && ChooseColor(&cc))
						PersistColors();
#elif !defined(__APPLE__)
					COLORREF cc = 0;
					if (wParam == IDC_COLOR1) cc = g_crGradStart;
					if (wParam == IDC_COLOR2) cc = g_crGradEnd;

					if (SWELL_ChooseColor(hwndDlg,&cc,16,g_custColors))
					{
						if (wParam == IDC_COLOR1) g_crGradStart = cc;
						if (wParam == IDC_COLOR2) g_crGradEnd = cc;
						PersistColors();
						InvalidateRect(hwndDlg,NULL,FALSE);
					}
#else
					switch(wParam)
					{
						case IDC_COLOR1:  iSettingColor = 0; ShowColorChooser(g_crGradStart); break;
						case IDC_COLOR2:  iSettingColor = 1; ShowColorChooser(g_crGradEnd);   break;
						case IDC_SETCUST: iSettingColor = 2; ShowColorChooser(g_custColors[0]); break;
					}
					SetTimer(hwndDlg, 1, 50, NULL);
#endif
					break;
				}
				case IDC_SAVECOL:
				case IDC_LOADCOL:
				case IDC_LOADFROMTHEME:
				{
					char cPath[512] = { 0 };
					const char* cExt = "SWS Color Files (*.SWSColor)\0*.SWSColor\0Color Theme Files (*.ReaperTheme)\0*.ReaperTheme\0All Files\0*.*\0";
					GetPrivateProfileString("REAPER", "lastthemefn", "", cPath, 512, get_ini_file());
					char* pC = strrchr(cPath, PATH_SLASH_CHAR);
					if (pC)
						*pC = 0;

					if (wParam == IDC_SAVECOL)
					{
						char cFilename[512];
						UpdateCustomColors();
						if (BrowseForSaveFile(__LOCALIZE("Save color theme","sws_color"), cPath, NULL, cExt, cFilename, 512))
						{
							char key[32];
							char val[32];
							for (int i = 0; i < 16; i++)
							{
								sprintf(key, "custcolor%d", i+1);
								sprintf(val, "%d", g_custColors[i]);
								WritePrivateProfileString("SWS Color", key, val, cFilename);
							}
							sprintf(val, "%d", g_crGradStart);
							WritePrivateProfileString("SWS Color", "gradientStart", val, cFilename);
							sprintf(val, "%d", g_crGradEnd);
							WritePrivateProfileString("SWS Color", "gradientEnd", val, cFilename);
						}
					}
					else if (wParam == IDC_LOADCOL || wParam == IDC_LOADFROMTHEME)
					{
#ifdef __APPLE__
						if (MessageBox(hwndDlg, __LOCALIZE("WARNING: Loading colors from file will overwrite your global personalized color choices.\nIf these are important to you, press press cancel to abort the loading of new colors!","sws_color"), __LOCALIZE("OSX Color Load WARNING","sws_color"), MB_OKCANCEL) == IDCANCEL)
							break;
#endif
						if (wParam == IDC_LOADCOL)
						{
							char* cFile = BrowseForFiles(__LOCALIZE("Choose color theme file","sws_color"), cPath, NULL, false, cExt);
							if (cFile)
							{
								lstrcpyn(cPath, cFile, 512);
								free(cFile);
							}
							else
								cPath[0] = 0;
						}
						else
							GetPrivateProfileString("REAPER", "lastthemefn", "", cPath, 512, get_ini_file());

						if (cPath[0])
						{
							char key[32];
							bool bFound = false;
							for (int i = 0; i < 16; i++)
							{
								sprintf(key, "custcolor%d", i+1);
								int iColor = GetPrivateProfileInt("SWS Color", key, -1, cPath);
								if (iColor != -1)
								{
									g_custColors[i] = iColor;
									bFound = true;
								}

							}
							if (!bFound)
							{
								char cMsg[512];
								snprintf(cMsg, 512, __LOCALIZE_VERFMT("No SWS custom colors found in %s.","sws_color"), cPath);
								MessageBox(hwndDlg, cMsg, __LOCALIZE("SWS Color Load","sws_color"), MB_OK);
							}

							g_crGradStart = GetPrivateProfileInt("SWS Color", "gradientStart", g_crGradStart, cPath);
							g_crGradEnd   = GetPrivateProfileInt("SWS Color", "gradientEnd", g_crGradEnd, cPath);
							PersistColors();
#ifdef _WIN32
							RedrawWindow(hwndDlg, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
#else
							InvalidateRect(hwndDlg, NULL, 0);
#endif
						}
					}
				}
				break;
				case IDOK:
					// Do something ?
					// fall through!
				case IDCANCEL:
#ifdef __APPLE__
					if (iSettingColor != -1)
					{
						HideColorChooser();
						KillTimer(hwndDlg, 1);
					}
#endif
					SaveWindowPos(hwndDlg, COLORDLG_WINDOWPOS_KEY);
					EndDialog(hwndDlg,0);
					break;
			}
			return 0;
		}
	}
	return 0;
}

void ShowColorDialog(COMMAND_T*)
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
	Undo_OnStateChangeEx(__LOCALIZE("Set track(s) children to same color","sws_undo"), UNDO_STATE_TRACKCFG, -1);
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
	Undo_OnStateChange(__LOCALIZE("Set item(s) color white","sws_undo"));
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
	Undo_OnStateChange(__LOCALIZE("Set item(s) color black","sws_undo"));
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
	Undo_OnStateChangeEx(__LOCALIZE("Set track(s) color white","sws_undo"), UNDO_STATE_TRACKCFG, -1);
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
	Undo_OnStateChangeEx(__LOCALIZE("Set track(s) color black","sws_undo"), UNDO_STATE_TRACKCFG, -1);
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
	Undo_OnStateChangeEx(__LOCALIZE("Set track(s) to previous track's color","sws_undo"), UNDO_STATE_TRACKCFG, -1);
}

void ColorTrackNext(COMMAND_T* = NULL)
{
	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			GetSetMediaTrackInfo(tr, "I_CUSTOMCOLOR", GetSetMediaTrackInfo(CSurf_TrackFromID(i+1, false), "I_CUSTOMCOLOR", NULL));
	}
	Undo_OnStateChangeEx(__LOCALIZE("Set track(s) to previous track's color","sws_undo"), UNDO_STATE_TRACKCFG, -1);
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
	Undo_OnStateChangeEx(__LOCALIZE("Set track(s) to next custom color","sws_undo"), UNDO_STATE_TRACKCFG, -1);
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
	Undo_OnStateChange(__LOCALIZE("Set item(s) to next custom color","sws_undo"));
	UpdateTimeline();
}

void TrackRandomCol(COMMAND_T* = NULL)
{
	COLORREF cr;
	// All black check
	if (AllBlack())
		return;
	while (!(cr = g_custColors[rand() % 16]));
	cr |= 0x1000000;
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			GetSetMediaTrackInfo(tr, "I_CUSTOMCOLOR", &cr);
	}
	Undo_OnStateChangeEx(__LOCALIZE("Set track(s) to one random custom color","sws_undo"), UNDO_STATE_TRACKCFG, -1);
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
			while (!(cr = g_custColors[rand() % 16]));
			cr |= 0x1000000;
			GetSetMediaTrackInfo(tr, "I_CUSTOMCOLOR", &cr);
		}
	}
	Undo_OnStateChangeEx(__LOCALIZE("Set track(s) to random custom color(s)","sws_undo"), UNDO_STATE_TRACKCFG, -1);
}

void ItemRandomCol(COMMAND_T* = NULL)
{
	COLORREF cr;
	// All black check
	if (AllBlack())
		return;
	while (!(cr = g_custColors[rand() % 16]));
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
	Undo_OnStateChange(__LOCALIZE("Set item(s) to one random custom color","sws_undo"));
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
				while (!(cr = g_custColors[rand() % 16]));
				cr |= 0x1000000;
				GetSetMediaItemInfo(mi, "I_CUSTOMCOLOR", &cr);
			}
		}
	}
	Undo_OnStateChange(__LOCALIZE("Set item(s) to random custom color(s)","sws_undo"));
	UpdateTimeline();
}

void TakeRandomCols(COMMAND_T* = NULL)
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
				for (int h = 0; h < CountTakes(mi); h++)
				{
					while (!(cr = g_custColors[rand() % 16]));
					cr |= 0x1000000;
					GetSetMediaItemTakeInfo(GetTake(mi, h), "I_CUSTOMCOLOR", &cr);
				}
			}
		}
	}
	Undo_OnStateChange(__LOCALIZE("Set takes in selected item(s) to random custom color(s)","sws_undo"));
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
	snprintf(cUndoText, sizeof(cUndoText), __LOCALIZE_VERFMT("Set track(s) to custom color %d","sws_undo"), iCustColor+1);
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
	snprintf(cUndoText, sizeof(cUndoText), __LOCALIZE_VERFMT("Set item(s) to custom color %d","sws_undo"), iCustColor+1);
	Undo_OnStateChange(cUndoText);
	UpdateTimeline();
}

void TakeCustomColor(int iCustColor)
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
			{
				MediaItem_Take* take = GetActiveTake(mi);
				if (take)
					GetSetMediaItemTakeInfo(take, "I_CUSTOMCOLOR", &cr);
			}
		}
	}
	char cUndoText[100];
	snprintf(cUndoText, sizeof(cUndoText), __LOCALIZE_VERFMT("Set take(s) to custom color %d","sws_undo"), iCustColor+1);
	Undo_OnStateChange(cUndoText);
	UpdateTimeline();
}

void TrackCustCol(COMMAND_T* ct) { TrackCustomColor((int)ct->user); }
void ItemCustCol(COMMAND_T* ct)  { ItemCustomColor((int)ct->user); }
void TakeCustCol(COMMAND_T* ct)  { TakeCustomColor((int)ct->user); }

void RandomColorAll(COMMAND_T*)
{
	// Get the first selected track
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			Undo_BeginBlock();
			Main_OnCommand(40360, 0); // Set track(s) to one random color
			COLORREF cr = *(COLORREF*)GetSetMediaTrackInfo(tr, "I_CUSTOMCOLOR", NULL);
			for (int i = 1; i <= GetNumTracks(); i++)
			{
				tr = CSurf_TrackFromID(i, false);
				for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
				{
					MediaItem* mi = GetTrackMediaItem(tr, j);
					if (*(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL))
						GetSetMediaItemInfo(mi, "I_CUSTOMCOLOR", &cr);
				}
			}
			UpdateTimeline();
			Undo_EndBlock(__LOCALIZE("Set selected track(s)/item(s) to one random color","sws_undo"), UNDO_STATE_TRACKCFG | UNDO_STATE_ITEMS);
			return;
		}
	}
	// No tracks selected so just run the item action
	Main_OnCommand(40706, 0); // Set item(s) to one random color
}

void CustomColorAll(COMMAND_T*)
{
	// Get the first selected track
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			Undo_BeginBlock();
			Main_OnCommand(40357, 0); // Set track(s) to one custom color
			// Assume the user clicked 'OK' in the color picker!
			COLORREF cr = *(COLORREF*)GetSetMediaTrackInfo(tr, "I_CUSTOMCOLOR", NULL);
			for (int i = 1; i <= GetNumTracks(); i++)
			{
				tr = CSurf_TrackFromID(i, false);
				for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
				{
					MediaItem* mi = GetTrackMediaItem(tr, j);
					if (*(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL))
						GetSetMediaItemInfo(mi, "I_CUSTOMCOLOR", &cr);
				}
			}
			UpdateTimeline();
			Undo_EndBlock(__LOCALIZE("Set selected track(s)/item(s) to custom color","sws_undo"), UNDO_STATE_TRACKCFG | UNDO_STATE_ITEMS);
			return;
		}
	}
	// No tracks selected so just run the item action
	Main_OnCommand(40704, 0); // Set item(s) to one custom color
}

int RecRedRulerEnabled(COMMAND_T*)
{
	return g_bRecRedRuler;
}

void ColorTimer()
{
	static int iRulerLaneCol[3];
	static bool bRecording = false;

	if (!bRecording && g_bRecRedRuler && GetPlayState() & 4)
	{
		int iSize;
		ColorTheme* colors = (ColorTheme*)GetColorThemeStruct(&iSize);
		for (int i = 0; i < 3; i++)
		{
			iRulerLaneCol[i] = colors->ruler_lane_bgcolor[i];
			colors->ruler_lane_bgcolor[i] = RGB(0xFF, 0, 0);
		}
		UpdateTimeline();
		bRecording = true;
	}
	else if (bRecording && (!g_bRecRedRuler || !(GetPlayState() & 4)))
	{
		int iSize;
		ColorTheme* colors = (ColorTheme*)GetColorThemeStruct(&iSize);
		for (int i = 0; i < 3; i++)
			colors->ruler_lane_bgcolor[i] = iRulerLaneCol[i];
		UpdateTimeline();
		bRecording = false;
	}
}

void RecRedRuler(COMMAND_T*)
{
	g_bRecRedRuler = !g_bRecRedRuler;
	if (g_bRecRedRuler) plugin_register("timer", (void*)ColorTimer);
	else                plugin_register("-timer",(void*)ColorTimer);
	WritePrivateProfileString(SWS_INI, RECREDRULER_KEY, g_bRecRedRuler ? "1" : "0", get_ini_file());
}

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
	Undo_OnStateChangeEx(__LOCALIZE("Set tracks to color gradient","sws_undo"), UNDO_STATE_TRACKCFG, -1);
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
	Undo_OnStateChangeEx(__LOCALIZE("Set track(s) to ordered custom color color(s)","sws_undo"), UNDO_STATE_TRACKCFG, -1);
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
	Undo_OnStateChange(__LOCALIZE("Set selected item(s) to color gradient per track","sws_undo"));
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
	Undo_OnStateChange(__LOCALIZE("Set selected item(s) to color gradient","sws_undo"));
	UpdateTimeline();
}

void TakeGradient(COMMAND_T* = NULL)
{
	int tCurPos = 0;
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* mi = GetTrackMediaItem(tr, j);
			if (*(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL))
			{
				for (int h = 0; h < CountTakes(mi); h++)
				{
					COLORREF cr = CalcGradient(g_crGradStart, g_crGradEnd, (double)tCurPos++ / (CountTakes(mi)-1)) | 0x1000000;
					GetSetMediaItemTakeInfo(GetTake(mi, h), "I_CUSTOMCOLOR", &cr);
				}
				tCurPos = 0;
			}
		}
	}
	Undo_OnStateChange(__LOCALIZE("Set takes in selected item(s) to color gradient","sws_undo"));
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
	Undo_OnStateChange(__LOCALIZE("Set selected item(s) to ordered custom colors","sws_undo"));
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
	Undo_OnStateChange(__LOCALIZE("Set selected item(s) to ordered custom colors","sws_undo"));
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
	Undo_OnStateChange(__LOCALIZE("Set selected item(s) to respective track color","sws_undo"));
	UpdateTimeline();
}

void TakeOrderedCol(COMMAND_T* = NULL)
{
	UpdateCustomColors();
	int tCurPos = 0;
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* mi = GetTrackMediaItem(tr, j);
			if (*(bool*)GetSetMediaItemInfo(mi, "B_UISEL", NULL))
			{
				for (int h = 0; h < CountTakes(mi); h++)
				{
					COLORREF cr = g_custColors[tCurPos++ % 16] | 0x1000000;
					GetSetMediaItemTakeInfo(GetTake(mi, h), "I_CUSTOMCOLOR", &cr);
				}
				tCurPos = 0;
			}
		}
	}
	Undo_OnStateChange(__LOCALIZE("Set takes in selected item(s) to ordered custom colors","sws_undo"));
	UpdateTimeline();
}

//!WANT_LOCALIZE_SWS_CMD_TABLE_BEGIN:sws_actions
static COMMAND_T g_commandTable[] =
{
	{ { DEFACCEL, "SWS: Open color management window" },                          "SWSCOLORWND",			ShowColorDialog,	"Show color management", },
	{ { DEFACCEL, "SWS: Toggle ruler red while recording" },                      "SWS_RECREDRULER",		RecRedRuler,		"Enable red ruler while recording (SWS)", 0, RecRedRulerEnabled },
	// Position of the above two commands in this table is important for menu generation!

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

	{ { DEFACCEL, "SWS: Set selected track(s)/item(s) to one random color" },     "SWS_RANDOMCOLALL",		RandomColorAll,		NULL, },
	{ { DEFACCEL, "SWS: Set selected track(s)/item(s) to custom color..." },      "SWS_CUSTOMCOLALL",		CustomColorAll,		NULL, },

	{ { DEFACCEL, "SWS: Set takes in selected item(s) to random custom color(s)"},"SWS_TAKESRANDCOLS",		TakeRandomCols,		NULL, },
	{ { DEFACCEL, "SWS: Set takes in selected item(s) to color gradient"},		  "SWS_TAKEGRAD",			TakeGradient,		NULL, },
	{ { DEFACCEL, "SWS: Set takes in selected item(s) to ordered custom colors"}, "SWS_TAKEORDCOL",			TakeOrderedCol,		NULL, },
	{ { DEFACCEL, "SWS: Set selected take(s) to custom color 1" },                "SWS_TAKECUSTCOL1",		TakeCustCol,		NULL, 0 },
	{ { DEFACCEL, "SWS: Set selected take(s) to custom color 2" },                "SWS_TAKECUSTCOL2",		TakeCustCol,		NULL, 1 },
	{ { DEFACCEL, "SWS: Set selected take(s) to custom color 3" },                "SWS_TAKECUSTCOL3",		TakeCustCol,		NULL, 2 },
	{ { DEFACCEL, "SWS: Set selected take(s) to custom color 4" },                "SWS_TAKECUSTCOL4",		TakeCustCol,		NULL, 3 },
	{ { DEFACCEL, "SWS: Set selected take(s) to custom color 5" },                "SWS_TAKECUSTCOL5",		TakeCustCol,		NULL, 4 },
	{ { DEFACCEL, "SWS: Set selected take(s) to custom color 6" },                "SWS_tAKECUSTCOL6",		TakeCustCol,		NULL, 5 },
	{ { DEFACCEL, "SWS: Set selected take(s) to custom color 7" },                "SWS_TAKECUSTCOL7",		TakeCustCol,		NULL, 6 },
	{ { DEFACCEL, "SWS: Set selected take(s) to custom color 8" },                "SWS_TAKECUSTCOL8",		TakeCustCol,		NULL, 7 },
	{ { DEFACCEL, "SWS: Set selected take(s) to custom color 9" },                "SWS_TAKECUSTCOL9",		TakeCustCol,		NULL, 8 },
	{ { DEFACCEL, "SWS: Set selected take(s) to custom color 10" },               "SWS_TAKECUSTCOL10",		TakeCustCol,		NULL, 9 },
	{ { DEFACCEL, "SWS: Set selected take(s) to custom color 11" },               "SWS_TAKECUSTCOL11",		TakeCustCol,		NULL, 10 },
	{ { DEFACCEL, "SWS: Set selected take(s) to custom color 12" },               "SWS_TAKECUSTCOL12",		TakeCustCol,		NULL, 11 },
	{ { DEFACCEL, "SWS: Set selected take(s) to custom color 13" },               "SWS_TAKECUSTCOL13",		TakeCustCol,		NULL, 12 },
	{ { DEFACCEL, "SWS: Set selected take(s) to custom color 14" },               "SWS_TAKECUSTCOL14",		TakeCustCol,		NULL, 13 },
	{ { DEFACCEL, "SWS: Set selected take(s) to custom color 15" },               "SWS_TAKECUSTCOL15",		TakeCustCol,		NULL, 14 },
	{ { DEFACCEL, "SWS: Set selected take(s) to custom color 16" },               "SWS_TAKECUSTCOL16",		TakeCustCol,		NULL, 15 },

	{ {}, LAST_COMMAND, NULL }, // Denote end of table
};
//!WANT_LOCALIZE_SWS_CMD_TABLE_END

//JFB menu items are not localized here (ideally it should be done through __LOCALIZE() and not with the table command above).
static void menuhook(const char* menustr, HMENU hMenu, int flag)
{
	int menuid = -1;
	if (strcmp(menustr, "Track control panel context") == 0)
		menuid = 0;
	else if (strcmp(menustr, "Media item context") == 0)
		menuid = 1;

	if (menuid >= 0 && flag == 0)
	{	// Initialize the menu
		void (*pFirstCommand)(COMMAND_T*);
		void (*pLastCommand)(COMMAND_T*);
		if (menuid == 0)
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
			AddToMenuOrdered(hSubMenu, __localizeFunc(g_commandTable[i].menuText,"sws_menu",0), g_commandTable[i].cmdId);
			i++;
		}
		while (!(g_commandTable[i-1].doCommand == pLastCommand && g_commandTable[i-1].user == 15));

		// Finish with color dialog
		AddToMenuOrdered(hSubMenu, __localizeFunc(g_commandTable[0].menuText,"sws_menu",0), g_commandTable[0].cmdId);

		if (menuid == 0)
			AddSubMenu(hMenu, hSubMenu, __LOCALIZE("SWS track color","sws_menu"), 40359);
		else
			AddSubMenu(hMenu, hSubMenu, __LOCALIZE("SWS item color","sws_menu"), 40707);
	}
	else if (flag == 1)
	{	// Update the color swatches
		// Color commands can be anywhere on the menu, so find 'em no matter where
#ifdef _WIN32
		static WDL_PtrList<void> pBitmaps;
		HDC hdcScreen = NULL;
		HDC hDC = NULL;

		if (pBitmaps.GetSize() == 0)
		{
			hdcScreen = CreateDC("DISPLAY", NULL, NULL, NULL);
			int s = GetSystemMetrics(SM_CYMENUCHECK);
			UpdateCustomColors();
			for (int i = 0; i < 16; i++)
				pBitmaps.Add(CreateCompatibleBitmap(hdcScreen, s+3, s));
		}

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
				if (!hDC)
				{	// Reduce thrashing
					UpdateCustomColors();
					if (!hdcScreen)
						hdcScreen = CreateDC("DISPLAY", NULL, NULL, NULL);
					hDC = CreateCompatibleDC(hdcScreen);
				}

				int s = GetSystemMetrics(SM_CYMENUCHECK);
				RECT rColor = { 0, 0, s, s };
				RECT rSpace = { s, 0, s+3, s };

				SelectObject(hDC, pBitmaps.Get(i%16));
				HBRUSH hb = CreateSolidBrush(g_custColors[i%16]);
				FillRect(hDC, &rColor, hb);
				DeleteObject(hb);
				FillRect(hDC, &rSpace, (HBRUSH)(COLOR_MENU+1));

				MENUITEMINFO mi={sizeof(MENUITEMINFO),};
				mi.fMask = MIIM_BITMAP;
				mi.hbmpItem = (HBITMAP)pBitmaps.Get(i%16);
				SetMenuItemInfo(h, iPos, true, &mi);
			}
		}
		if (hDC)
			DeleteDC(hDC);
		if (hdcScreen)
			DeleteDC(hdcScreen);
#elif defined(__APPLE__)
		UpdateCustomColors();

		int iCommand1 = SWSGetCommandID(TrackCustCol, 0);
		int iCommand2 = SWSGetCommandID(ItemCustCol, 0);

		for (int i = 0; i < 32; i++)
		{
			int iPos;
			HMENU h;
			if (i < 16)
				h = FindMenuItem(hMenu, iCommand1 + i, &iPos);
			else
				h = FindMenuItem(hMenu, iCommand2 + i - 16, &iPos);

			if (h)
				SetMenuItemSwatch(h, iPos, 12, g_custColors[i % 16]);
		}
#else
		int iCommand1 = SWSGetCommandID(TrackCustCol, 0);
		int iCommand2 = SWSGetCommandID(ItemCustCol, 0);
		static WDL_PtrList<void> pBitmaps;

		const int h = (SWELL_GetScaling256() * 16)/256;
		const int w = h + 4;

		if (pBitmaps.GetSize() == 0)
		{
			unsigned char *bits = (unsigned char *)calloc(w*h,sizeof(int));

			UpdateCustomColors();
			if (bits)
			{
				for (int i = 0; i < 16; i++)
					pBitmaps.Add(CreateBitmap(w,h,1,32,bits));
				free(bits);
			}
		}

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
				HBITMAP bm = (HBITMAP) pBitmaps.Get(i%16);
				if (bm)
				{
					BITMAP bmi;
					GetObject(bm,sizeof(bmi),&bmi);
					if (bmi.bmBits && bmi.bmPlanes == 1 && bmi.bmBitsPixel == 32)
					{
						const int n = (bmi.bmWidthBytes * bmi.bmHeight)/4;
						const int cc = g_custColors[i%16] | 0xFF000000;
						int *wr = (int *)bmi.bmBits;
						for (int j=0;j<n; j ++)
							wr[j] = cc;
					}

					MENUITEMINFO mi={sizeof(MENUITEMINFO),};
					mi.fMask = MIIM_BITMAP;
					mi.hbmpItem = bm;
					SetMenuItemInfo(h, iPos, true, &mi);
				}
			}
		}

#endif
	}
}

int ColorInit()
{
	SWSRegisterCommands(g_commandTable);
	srand((UINT)time(NULL));

	if (!plugin_register("hookcustommenu", (void*)menuhook))
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
	g_bRecRedRuler = GetPrivateProfileInt(SWS_INI, RECREDRULER_KEY, g_bRecRedRuler, get_ini_file()) ? true : false;
	if (g_bRecRedRuler) plugin_register("timer", (void*)ColorTimer);

	return 1;
}

void ColorExit()
{
	plugin_register("-hookcustommenu", (void*)menuhook);
	plugin_register("-timer", (void*)ColorTimer);
}
