/******************************************************************************
/ sws_util.cpp
/
/ Copyright (c) 2010 Tim Payne (SWS)
/ https://code.google.com/p/sws-extension
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
#include "reaper/localize.h"

// Globals
double g_d0 = 0.0;
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

void SetWindowPosAtMouse(HWND hwnd)
{
	RECT r;
	GetWindowRect(hwnd, &r);
	POINT p;
	GetCursorPos(&p);
	r.right = p.x + abs(r.right-r.left);
	r.bottom = p.y + abs(r.bottom-r.top);
	r.left = p.x;
	r.top = p.y;
	EnsureNotCompletelyOffscreen(&r);
	SetWindowPos(hwnd, NULL, r.left, r.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}

#ifndef _WIN32
int GetMenuString(HMENU hMenu, UINT uIDItem, char* lpString, int nMaxCount, UINT uFlag)
{
	if (hMenu && lpString)
	{
		MENUITEMINFO xInfo;
		xInfo.cbSize = sizeof(MENUITEMINFO);
/* On Win OS
		xInfo.fMask = MIIM_STRING;
*/
		xInfo.fMask = MIIM_TYPE; // ok on OSX/SWELL
		xInfo.fType = MFT_STRING;
		xInfo.fState = 0;
		xInfo.wID = 0;
		xInfo.hSubMenu = NULL;
		xInfo.hbmpChecked = NULL;
		xInfo.hbmpUnchecked = NULL;
		xInfo.dwItemData = 0;
		xInfo.dwTypeData = lpString;
		xInfo.cch = nMaxCount;
		if (GetMenuItemInfo(hMenu, uIDItem, (uFlag&MF_BYPOSITION) == MF_BYPOSITION, &xInfo))
			return strlen(lpString);
	}
	return 0;
}
#endif

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
	int iTrack = CSurf_TrackToID(tr, false);
	if (iTrack == 0)
		return *(int*)GetConfigVar("showmaintrack") ? 3 : 1; // For now, always return master vis in MCP
	else if (iTrack < 0)
		return 0;

	int iVis = *(bool*)GetSetMediaTrackInfo(tr, "B_SHOWINMIXER", NULL) ? 1 : 0;
	iVis    |= *(bool*)GetSetMediaTrackInfo(tr, "B_SHOWINTCP", NULL) ? 2 : 0;
	return iVis;
}

void SetTrackVis(MediaTrack* tr, int vis) // &1 == mcp, &2 == tcp
{
	int iTrack = CSurf_TrackToID(tr, false);
	if (iTrack == 0)
	{	// TODO - obey master in mcp
		if ((vis & 2) != (*(int*)GetConfigVar("showmaintrack") ? 2 : 0))
			Main_OnCommand(40075, 0);
	}
	else if (iTrack > 0)
	{
		if (vis != GetTrackVis(tr))
		{
			GetSetMediaTrackInfo(tr, "B_SHOWINTCP",   vis & 2 ? &g_bTrue : &g_bFalse);
			GetSetMediaTrackInfo(tr, "B_SHOWINMIXER", vis & 1 ? &g_bTrue : &g_bFalse);
		}
	}
}

void* GetConfigVar(const char* cVar)
{
	int sztmp;
	void* p = NULL;
	if (int iOffset = projectconfig_var_getoffs(cVar, &sztmp))
	{
		p = projectconfig_var_addr(EnumProjects(-1, NULL, 0), iOffset);
	}
	else if (p = get_config_var(cVar, &sztmp))
	{
	}
	else
	{
		p = get_midi_config_var(cVar, &sztmp);
	}
	return p;
}

HWND GetTrackWnd()
{
#ifdef _WIN32
	static const char* sTrackview = __LOCALIZE("trackview", "DLG_102");
	return FindWindowEx(g_hwndParent,0,"REAPERTrackListWindow", sTrackview);
#else
	return GetWindow(g_hwndParent, GW_CHILD);
#endif
}

HWND GetRulerWnd()
{
#ifdef _WIN32
	static const char* sTimeline = __LOCALIZE("timeline", "DLG_102");
	return FindWindowEx(g_hwndParent, 0, "REAPERTimeDisplay", sTimeline);
#else
	return GetWindow(GetTrackWnd(), GW_HWNDNEXT);
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

// overrides the native GetTrackGUID(): returns a special GUID for the master track
// (useful for persistence as no GUID is stored in RPP files for the master..)
// note: TrackToGuid(NULL) will return also NULL, contrary to GetTrackGUID(NULL)
const GUID* TrackToGuid(MediaTrack* tr)
{
	if (tr)
	{
		if (tr == GetMasterTrack(NULL))
			return &GUID_NULL;
		else
			return GetTrackGUID(tr);
	}
	return NULL;
}

MediaTrack* GuidToTrack(const GUID* guid)
{
	if (guid)
		for (int i=0; i<=GetNumTracks(); i++)
			if (MediaTrack* tr = CSurf_TrackFromID(i, false))
				if (TrackMatchesGuid(tr, guid))
					return tr;
	return NULL;
}

bool GuidsEqual(const GUID* g1, const GUID* g2)
{
	return g1 && g2 && !memcmp(g1, g2, sizeof(GUID));
}

bool TrackMatchesGuid(MediaTrack* tr, const GUID* g)
{
	if (tr && g)
	{
		if (GuidsEqual(GetTrackGUID(tr), g))
			return true;
		// 2nd try for the master
		if (tr == GetMasterTrack(NULL))
			return GuidsEqual(g, &GUID_NULL);
	}
	return false;
}

const char* stristr(const char* str1, const char* str2)
{
#if defined(_WIN32) && !defined(WDL_NO_SUPPORT_UTF8)
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

#ifdef _WIN32
void dprintf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int len = _vscprintf(format, args) + 1;
    char* buffer = new char[len];
	vsprintf_s(buffer, len, format, args); // C4996
    OutputDebugString(buffer);
	delete[] buffer;
}
#endif

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

int SWS_GetModifiers()
{
	int iKeys = GetAsyncKeyState(VK_CONTROL) & 0x8000 ? SWS_CTRL  : 0;
	iKeys    |= GetAsyncKeyState(VK_MENU)    & 0x8000 ? SWS_ALT   : 0;
	iKeys    |= GetAsyncKeyState(VK_SHIFT)   & 0x8000 ? SWS_SHIFT : 0;
	return iKeys;
}

MODIFIER g_modifiers[NUM_MODIFIERS] =
{
	{ 0, "None" },
	{ 2, "Ctrl" },
	{ 1, "Alt" },
	{ 4, "Shift" },
	{ 3, "Ctrl + Alt" },
	{ 6, "Ctrl + Shift" },
	{ 5, "Alt + Shift" },
	{ 7, "Ctrl + Alt + Shift" }
};

bool SWS_IsWindow(HWND hwnd)
{
#ifdef _WIN32
	return IsWindow(hwnd) ? true : false;
#else
	// ISSUE 498: OSX IsWindow is broken for docked windows!
	//   Justin recommends not using IsWindow at all, but there are a lot of cases
	//   where we rely on it for error checking.  Instead of removing all calls and
	//   doing internal window state handling I replaced all IsWindow calls with
	//   this function that checks for docked windows on OSX.  It's a bit of a
	//   hack, but less risky IMO than rewriting tons of window handling code.
	//	
	//   Maybe could replace with return hwnd != NULL;
	return (bool)IsWindow(hwnd) ? true : (DockIsChildOfDock(hwnd, NULL) != -1);
#endif
}


// Localization

WDL_FastString* g_LangPack = NULL;;

// Never returns NULL ("" means no langpack defined)
WDL_FastString* GetLangPack()
{
	// lazy init, works because swtching LangPacks requires REAPER restart
	if (!g_LangPack)
	{
		g_LangPack = new WDL_FastString;
#ifdef _SWS_LOCALIZATION
		char fn[BUFFER_SIZE] = ""; 
		GetPrivateProfileString("REAPER", "langpack", "", fn, sizeof(fn), get_ini_file());
		if (*fn && strcmp(fn, "<>"))
		{
			g_LangPack->Set(fn);
			if (!FileExists(fn)) // retry with fn as "short name"
			{
				g_LangPack->SetFormatted(BUFFER_SIZE, "%s%cLangPack%c%s", GetResourcePath(), PATH_SLASH_CHAR, PATH_SLASH_CHAR, fn);
				if (!FileExists(g_LangPack->Get())) // does not exist either..
					g_LangPack->Set("");
			}
		}
#endif
	}
	return g_LangPack;
}

bool IsLocalized() {
#ifdef _SWS_LOCALIZATION
	return (GetLangPack()->GetLength() > 0);
#else
	return false;
#endif
}

// check that action tags ("SWS: ", "SWS/FNG: ", SWS/S&M: ", etc..) are preserved
const char* GetLocalizedActionName(const char* _defaultStr, int _flags, const char* _section)
{
#ifdef _SWS_LOCALIZATION
	const char* p = __localizeFunc(_defaultStr, _section, _flags);
	if (IsSwsAction(p))
		return p;
#endif
	return _defaultStr;
}

bool IsLocalizableAction(const char* _customId) {
	return (!strstr(_customId, "_CYCLACTION") && !strstr(_customId, "_SWSCONSOLE_CUST"));
}

// wrappers to ignore localized envelope names
TrackEnvelope* SWS_GetTakeEnvelopeByName(MediaItem_Take* take, const char* envname) {
	return GetTakeEnvelopeByName(take, __localizeFunc(envname, "item", 0));
}
TrackEnvelope* SWS_GetTrackEnvelopeByName(MediaTrack* track, const char* envname) {
	return GetTrackEnvelopeByName(track,  __localizeFunc(envname, "envname", 0));
}
