/******************************************************************************
/ sws_util.cpp
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
#include "../WDL/sha.h"

// Globals
int  g_i0 = 0;
int  g_i1 = 1;
int  g_i2 = 2;
bool g_bTrue  = true;
bool g_bFalse = false;
#ifndef _WIN32
const GUID GUID_NULL = { 0, 0, 0, "\0\0\0\0\0\0\0" };
#endif

BOOL IsCommCtrlVersion6()
{
#ifndef _WIN32
	return true;
#else
    static BOOL isCommCtrlVersion6 = -1;
    if (isCommCtrlVersion6 != -1)
        return isCommCtrlVersion6;
    
    //The default value
    isCommCtrlVersion6 = FALSE;
    
    HINSTANCE commCtrlDll = LoadLibrary("comctl32.dll");
    if (commCtrlDll)
    {
        DLLGETVERSIONPROC pDllGetVersion;
        pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(commCtrlDll, "DllGetVersion");
        
        if (pDllGetVersion)
        {
            DLLVERSIONINFO dvi = {0};
            dvi.cbSize = sizeof(DLLVERSIONINFO);
            (*pDllGetVersion)(&dvi);
            
            isCommCtrlVersion6 = (dvi.dwMajorVersion == 6);
        }
        
        FreeLibrary(commCtrlDll);
    }
    
    return isCommCtrlVersion6;
#endif
}

void AddToMenu(HMENU hMenu, const char* text, int id, int iInsertAfter, bool bPos, UINT uiSate)
{
	if (!text)
		return;

	int iPos = GetMenuItemCount(hMenu);
	if (bPos)
		iPos = iInsertAfter;
	else
	{
		if (iInsertAfter < 0)
			iPos += iInsertAfter + 1;
		else
		{
			HMENU h = FindMenuItem(hMenu, iInsertAfter, &iPos);
			if (h)
			{
				hMenu = h;
				iPos++;
			}
		}
	}
	
	MENUITEMINFO mi={sizeof(MENUITEMINFO),};
	if (strcmp(text, SWS_SEPARATOR) == 0)
	{
		mi.fType = MFT_SEPARATOR;
		mi.fMask = MIIM_TYPE;
		InsertMenuItem(hMenu, iPos, true, &mi);
	}
	else
	{
		mi.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
		mi.fState = uiSate;
		mi.fType = MFT_STRING;
		mi.dwTypeData = (char*)text;
		mi.wID = id;
		InsertMenuItem(hMenu, iPos, true, &mi);
	}
}

void AddSubMenu(HMENU hMenu, HMENU subMenu, const char* text, int iInsertAfter, UINT uiSate)
{
	int iPos = GetMenuItemCount(hMenu);
	if (iInsertAfter < 0)
	{
		iPos += iInsertAfter + 1;
		if (iPos < 0)
			iPos = 0;
	}
	else
	{
		HMENU h = FindMenuItem(hMenu, iInsertAfter, &iPos);
		if (h)
		{
			hMenu = h;
			iPos++;
		}
	}

	MENUITEMINFO mi={sizeof(MENUITEMINFO),};
	mi.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_STATE;
	mi.fState = uiSate;
	mi.fType = MFT_STRING;
	mi.hSubMenu = subMenu;
	mi.dwTypeData = (LPSTR)text;
	InsertMenuItem(hMenu, iPos, true, &mi);
}

HMENU FindMenuItem(HMENU hMenu, int iCmd, int* iPos)
{
	MENUITEMINFO mi={sizeof(MENUITEMINFO),};
	mi.fMask = MIIM_ID | MIIM_SUBMENU;
	for (int i = 0; i < GetMenuItemCount(hMenu); i++)
	{
		GetMenuItemInfo(hMenu, i, true, &mi);
		if (mi.hSubMenu)
		{
			HMENU hSubMenu = FindMenuItem(mi.hSubMenu, iCmd, iPos);
			if (hSubMenu)
				return hSubMenu;
		}
		if (mi.wID == iCmd)
		{
			*iPos = i;
			return hMenu;
		}
	}
	return NULL;
}

void SWSSetMenuText(HMENU hMenu, int iCmd, const char* cText)
{
	int iPos;
	hMenu = FindMenuItem(hMenu, iCmd, &iPos);
	if (hMenu)
	{
		MENUITEMINFO mi={sizeof(MENUITEMINFO),};
		mi.fMask = MIIM_TYPE;
		mi.fType = MFT_STRING;
		mi.dwTypeData = (char*)cText;
		SetMenuItemInfo(hMenu, iPos, true, &mi);
	}
}

void SaveWindowPos(HWND hwnd, const char* cKey)
{
	// Remember the dialog position
	char str[256];
	RECT r;
	GetWindowRect(hwnd, &r);
	sprintf(str, "%d %d %d %d", r.left, r.top, r.right - r.left, r.bottom - r.top);
	WritePrivateProfileString(SWS_INI, cKey, str, get_ini_file());
}

void RestoreWindowPos(HWND hwnd, const char* cKey, bool bRestoreSize)
{
	char str[256];
	GetPrivateProfileString(SWS_INI, cKey, "unknown", str, 256,get_ini_file());
	LineParser lp(false);
	if (!lp.parse(str) && lp.getnumtokens() == 4)
	{
		RECT r;
		r.left   = lp.gettoken_int(0);
		r.top    = lp.gettoken_int(1);
		r.right  = lp.gettoken_int(0) + lp.gettoken_int(2);
		r.bottom = lp.gettoken_int(1) + lp.gettoken_int(3);
		EnsureNotCompletelyOffscreen(&r);
		if (bRestoreSize)
			SetWindowPos(hwnd, NULL, r.left, r.top, r.right-r.left, r.bottom-r.top, SWP_NOZORDER);
		else
			SetWindowPos(hwnd, NULL, r.left, r.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
	}
}

MediaTrack* GetFirstSelectedTrack()
{
	for (int j = 0; j <= GetNumTracks(); j++)
	{
		MediaTrack* tr = CSurf_TrackFromID(j, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			return tr;
	}
	return NULL;
}

// NumSelTracks takes the master track into account (contrary to CountSelectedTracks())
int NumSelTracks()
{
	int iCount = 0;
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			iCount++;
	}
	return iCount;
}

static MediaTrack** g_pSelTracks = NULL;
static int g_iNumSel = 0;
void SaveSelected()
{
	if (g_pSelTracks)
		delete [] g_pSelTracks;
	g_iNumSel = NumSelTracks();
	if (g_iNumSel)
	{
		g_pSelTracks = new MediaTrack*[g_iNumSel];
		int i = 0;
		for (int j = 0; j <= GetNumTracks(); j++)
			if (*(int*)GetSetMediaTrackInfo(CSurf_TrackFromID(j, false), "I_SELECTED", NULL))
				g_pSelTracks[i++] = CSurf_TrackFromID(j, false);
	}
}

void RestoreSelected()
{
	int iSel = 1;
	for (int i = 0; i < g_iNumSel; i++)
		if (CSurf_TrackToID(g_pSelTracks[i], false) >= 0)
			GetSetMediaTrackInfo(g_pSelTracks[i], "I_SELECTED", &iSel);
}

void ClearSelected()
{
	int iSel = 0;
	for (int i = 0; i <= GetNumTracks(); i++)
		GetSetMediaTrackInfo(CSurf_TrackFromID(i, false), "I_SELECTED", &iSel);
}

int GetFolderDepth(MediaTrack* tr, int* iType, MediaTrack** nextTr) // iType: 0=normal, 1=parent, < 0=last in folder
{
	//static MediaTrack* nextTr = NULL; Let calling fcn keep track so state is reset appropriately
	static int iLastFolder = 0;
	int iFolder;

	if (tr == *nextTr)
	{
		*nextTr = CSurf_TrackFromID(CSurf_TrackToID(tr, false) + 1, false);

		if (CSurf_TrackToID(tr, false) == 0) // Special case for master
		{
			if (iType)
				*iType = 1;
			return -1; // Master is at '-1' depth
		}

		iFolder = *(int*)GetSetMediaTrackInfo(tr, "I_FOLDERDEPTH", NULL);
			
		if (iType)
			*iType = iFolder;

		if (iFolder == 0)
			return iLastFolder;
		else if (iFolder == 1)
		{
			iLastFolder++;
			return iLastFolder - 1; // Count parent as at the "previous" level
		}
		else if (iFolder < 0)
		{
			int iCur = iLastFolder;
			iLastFolder += iFolder;
			return iCur;
		}
	}
	else
	{
		// Must iterate until reaching the track passed in
		iLastFolder = 0;
		*nextTr = CSurf_TrackFromID(0, false);
		while (tr != *nextTr)
			GetFolderDepth(*nextTr, NULL, nextTr);

		return GetFolderDepth(tr, iType, nextTr);
	}
	return -1;
}

int GetTrackVis(MediaTrack* tr) // &1 == mcp, &2 == tcp
{
	if (CSurf_TrackToID(tr, false) <= 0)
		return 0;

	int iVis = *(bool*)GetSetMediaTrackInfo(tr, "B_SHOWINMIXER", NULL) ? 1 : 0;
	iVis    |= *(bool*)GetSetMediaTrackInfo(tr, "B_SHOWINTCP", NULL) ? 2 : 0;
	return iVis;
}

void SetTrackVis(MediaTrack* tr, int vis) // &1 == mcp, &2 == tcp
{
	if (CSurf_TrackToID(tr, false) <= 0 || vis == GetTrackVis(tr))
		return;

	GetSetMediaTrackInfo(tr, "B_SHOWINTCP", vis & 2 ? &g_bTrue : &g_bFalse);
	GetSetMediaTrackInfo(tr, "B_SHOWINMIXER", vis & 1 ? &g_bTrue : &g_bFalse);
}

void* GetConfigVar(const char* cVar)
{
	int sztmp;
	int iOffset = projectconfig_var_getoffs(cVar, &sztmp);
	if (iOffset)
		return projectconfig_var_addr(Enum_Projects(-1, NULL, 0), iOffset);
	else
		return get_config_var(cVar, &sztmp);
}

HWND GetTrackWnd()
{
#ifdef _WIN32
	return FindWindowEx(g_hwndParent,0,"REAPERTrackListWindow","trackview");
#else
	return GetWindow(g_hwndParent, GW_CHILD); // Not guaranteed to work?
#endif
}

// Output string must be 41 bytes minimum.  out is returned as a convenience.
char* GetHashString(const char* in, char* out)
{
	WDL_SHA1 sha;
	sha.add(in, (int)strlen(in));
	char hash[20];
	sha.result(hash);
	for (int i = 0; i < 20; i++)
		sprintf(out + i*2, "%02X", (unsigned char)(hash[i] & 0xFF));
	out[40] = 0;
	return out;
}

MediaTrack* GuidToTrack(const GUID* guid)
{
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (TrackMatchesGuid(tr, guid))
			return tr;
	}
	return NULL;
}

bool GuidsEqual(const GUID* g1, const GUID* g2)
{
	return memcmp(g1, g2, sizeof(GUID)) == 0;
}

bool TrackMatchesGuid(MediaTrack* tr, const GUID* g)
{
	if (CSurf_TrackToID(tr, false) == 0)
		return GuidsEqual(g, &GUID_NULL);
	GUID* gTr = (GUID*)GetSetMediaTrackInfo(tr, "GUID", NULL);
	return gTr && GuidsEqual(gTr, g);
}

const char* stristr(const char* str1, const char* str2)
{
#ifdef _WIN32
	// Don't mess with UTF8, TODO fix!
	if (WDL_HasUTF8(str1) || WDL_HasUTF8(str2))
		return strstr(str1, str2);
#endif

	const char* p1 = str1; const char* p2 = str2;

	while(*p1 && *p2)
	{
		if (tolower(*p1) == tolower(*p2))
		{
			p1++;
			p2++;
		}
		else
		{
			p1++;
			p2 = str2;
		}
	}
	if (!*p2)
		return p1 - strlen(str2);
	return NULL;
}

void SWS_GetSelectedTracks(WDL_TypedBuf<MediaTrack*>* buf, bool bMaster)
{
	buf->Resize(0);
	for (int i = (bMaster ? 0 : 1); i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (*(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			int pos = buf->GetSize();
			buf->Resize(pos + 1);
			buf->Get()[pos] = tr;
		}
	}
}

void SWS_GetSelectedMediaItems(WDL_TypedBuf<MediaItem*>* buf)
{
	buf->Resize(0);
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* item = GetTrackMediaItem(tr, j);
			if (*(bool*)GetSetMediaItemInfo(item, "B_UISEL", NULL))
			{
				int pos = buf->GetSize();
				buf->Resize(pos + 1);
				buf->Get()[pos] = item;
			}
		}
	}
}

void SWS_GetSelectedMediaItemsOnTrack(WDL_TypedBuf<MediaItem*>* buf, MediaTrack* tr)
{
	buf->Resize(0);
	if (CSurf_TrackToID(tr, false) <= 0)
		return;
	for (int j = 0; j < GetTrackNumMediaItems(tr); j++)
	{
		MediaItem* item = GetTrackMediaItem(tr, j);
		if (*(bool*)GetSetMediaItemInfo(item, "B_UISEL", NULL))
		{
			int pos = buf->GetSize();
			buf->Resize(pos + 1);
			buf->Get()[pos] = item;
		}
	}
}