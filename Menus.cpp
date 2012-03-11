/******************************************************************************
/ Menus.cpp
/
/ Copyright (c) 2011 Tim Payne (SWS)
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

// Utility functions for manipulating menus, as well as the main menu creation function
// for the SWS extension.

#include "stdafx.h"
#include "./reaper/localize.h"


// *************************** UTILITY FUNCTIONS ***************************

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

int SWSGetMenuPosFromID(HMENU hMenu, UINT id)
{	// Replacement for deprecated windows func GetMenuPosFromID
	MENUITEMINFO mi={sizeof(MENUITEMINFO),};
	mi.fMask = MIIM_ID;
	for (int i = 0; i < GetMenuItemCount(hMenu); i++)
	{
		GetMenuItemInfo(hMenu, i, true, &mi);
		if (mi.wID == id)
			return i;
	}
	return -1;
}

// *************************** MENU CREATION ***************************

// important both __LOCALIZE() parameters MUST remain literal strings 
// (used by the build_sample_langpack tool)
void SWSCreateExtensionsMenu(HMENU hMenu)
{
	if (GetMenuItemCount(hMenu))
		AddToMenu(hMenu, SWS_SEPARATOR, 0);

	// Create the common "Extensions" menu 
	AddToMenu(hMenu, __LOCALIZE("About SWS Extensions", "sws_ext_menu"), NamedCommandLookup("_SWS_ABOUT"));
	AddToMenu(hMenu, __LOCALIZE("Auto Color/Icon", "sws_ext_menu"), NamedCommandLookup("_SWSAUTOCOLOR_OPEN"));

	HMENU hAutoRenderSubMenu = CreatePopupMenu();
	AddSubMenu(hMenu, hAutoRenderSubMenu, __LOCALIZE("Autorender", "sws_ext_menu"));
	AddToMenu(hAutoRenderSubMenu, __LOCALIZE("Batch render regions", "sws_ext_menu"), NamedCommandLookup("_AUTORENDER"));
	AddToMenu(hAutoRenderSubMenu, __LOCALIZE("Edit project metadata", "sws_ext_menu"), NamedCommandLookup("_AUTORENDER_METADATA"));
	AddToMenu(hAutoRenderSubMenu, __LOCALIZE("Open render path", "sws_ext_menu"), NamedCommandLookup("_AUTORENDER_OPEN_RENDER_PATH"));
	AddToMenu(hAutoRenderSubMenu, __LOCALIZE("Show help", "sws_ext_menu"), NamedCommandLookup("_AUTORENDER_HELP"));
	AddToMenu(hAutoRenderSubMenu, __LOCALIZE("Global preferences", "sws_ext_menu"), NamedCommandLookup("_AUTORENDER_PREFERENCES"));

	AddToMenu(hMenu, __LOCALIZE("Command parameters", "sws_ext_menu"), NamedCommandLookup("_XENAKIOS_SHOW_COMMANDPARAMS"));
	AddToMenu(hMenu, __LOCALIZE("Cue Buss generator", "sws_ext_menu"), NamedCommandLookup("_S&M_SENDS4"));
	AddToMenu(hMenu, __LOCALIZE("Cycle Action editor...", "sws_ext_menu"), NamedCommandLookup("_S&M_CYCLEDITOR"));
	AddToMenu(hMenu, __LOCALIZE("Envelope processor...", "sws_ext_menu"), NamedCommandLookup("_PADRE_ENVPROC"));
	AddToMenu(hMenu, __LOCALIZE("Fill gaps...", "sws_ext_menu"), NamedCommandLookup("_SWS_AWFILLGAPSADV"));
	AddToMenu(hMenu, __LOCALIZE("Find", "sws_ext_menu"), NamedCommandLookup("_S&M_SHOWFIND"));
	AddToMenu(hMenu, __LOCALIZE("Groove tool...", "sws_ext_menu"), NamedCommandLookup("_FNG_GROOVE_TOOL"));
	AddToMenu(hMenu, __LOCALIZE("Label processor", "sws_ext_menu"), NamedCommandLookup("_IX_LABEL_PROC"));
	AddToMenu(hMenu, __LOCALIZE("LFO generator...", "sws_ext_menu"), NamedCommandLookup("_PADRE_ENVLFO"));
	AddToMenu(hMenu, __LOCALIZE("Live Configs", "sws_ext_menu"), NamedCommandLookup("_S&M_SHOWMIDILIVE"));
	AddToMenu(hMenu, __LOCALIZE("MarkerList", "sws_ext_menu"), NamedCommandLookup("_SWSMARKERLIST1"));

	HMENU hMarkerSubMenu = CreatePopupMenu();
	AddSubMenu(hMenu, hMarkerSubMenu, __LOCALIZE("Marker utilites", "sws_ext_menu"));
	AddToMenu(hMarkerSubMenu, __LOCALIZE("Load marker set...", "sws_ext_menu"), NamedCommandLookup("_SWSMARKERLIST2"));
	AddToMenu(hMarkerSubMenu, __LOCALIZE("Save marker set...", "sws_ext_menu"), NamedCommandLookup("_SWSMARKERLIST3"));
	AddToMenu(hMarkerSubMenu, __LOCALIZE("Delete marker set...", "sws_ext_menu"), NamedCommandLookup("_SWSMARKERLIST4"));
	AddToMenu(hMarkerSubMenu, SWS_SEPARATOR, 0);
	AddToMenu(hMarkerSubMenu, __LOCALIZE("Copy marker set to clipboard", "sws_ext_menu"), NamedCommandLookup("_SWSMARKERLIST5"));
	AddToMenu(hMarkerSubMenu, __LOCALIZE("Paste marker set from clipboard", "sws_ext_menu"), NamedCommandLookup("_SWSMARKERLIST6"));
	AddToMenu(hMarkerSubMenu, SWS_SEPARATOR, 0);
	AddToMenu(hMarkerSubMenu, __LOCALIZE("Reorder marker IDs", "sws_ext_menu"), NamedCommandLookup("_SWSMARKERLIST7"));
	AddToMenu(hMarkerSubMenu, __LOCALIZE("Reorder region IDs", "sws_ext_menu"), NamedCommandLookup("_SWSMARKERLIST8"));
	AddToMenu(hMarkerSubMenu, SWS_SEPARATOR, 0);
	AddToMenu(hMarkerSubMenu, __LOCALIZE("Select next region", "sws_ext_menu"), NamedCommandLookup("_SWS_SELNEXTREG"));
	AddToMenu(hMarkerSubMenu, __LOCALIZE("Select prev region", "sws_ext_menu"), NamedCommandLookup("_SWS_SELPREVREG"));
	AddToMenu(hMarkerSubMenu, __LOCALIZE("Delete all markers", "sws_ext_menu"), NamedCommandLookup("_SWSMARKERLIST9"));
	AddToMenu(hMarkerSubMenu, __LOCALIZE("Delete all regions", "sws_ext_menu"), NamedCommandLookup("_SWSMARKERLIST10"));
	AddToMenu(hMenu, __LOCALIZE("Notes/Subtitles/Help", "sws_ext_menu"), NamedCommandLookup("_S&M_SHOW_NOTES_VIEW"));
	AddToMenu(hMenu, __LOCALIZE("Project List", "sws_ext_menu"), NamedCommandLookup("_SWS_PROJLIST_OPEN"));

	HMENU hPrjMgmtSubMenu = CreatePopupMenu();
	AddSubMenu(hMenu, hPrjMgmtSubMenu, __LOCALIZE("Project Management", "sws_ext_menu"));
	AddToMenu(hPrjMgmtSubMenu, __LOCALIZE("Open projects from list...", "sws_ext_menu"), NamedCommandLookup("_SWS_PROJLISTSOPEN"));
	AddToMenu(hPrjMgmtSubMenu, __LOCALIZE("Save list of open projects...", "sws_ext_menu"), NamedCommandLookup("_SWS_PROJLISTSAVE"));
	AddToMenu(hPrjMgmtSubMenu, __LOCALIZE("Add related project(s)...", "sws_ext_menu"), NamedCommandLookup("_SWS_ADDRELATEDPROJ"));
	AddToMenu(hPrjMgmtSubMenu, __LOCALIZE("Delete related project...", "sws_ext_menu"), NamedCommandLookup("_SWS_DELRELATEDPROJ"));
	AddToMenu(hPrjMgmtSubMenu, SWS_SEPARATOR, 0);
	AddToMenu(hPrjMgmtSubMenu, __LOCALIZE("(related projects list)", "sws_ext_menu"), NamedCommandLookup("_SWS_OPENRELATED1"));

	AddToMenu(hMenu, __LOCALIZE("ReaConsole...", "sws_ext_menu"), NamedCommandLookup("_SWSCONSOLE"));
	AddToMenu(hMenu, __LOCALIZE("Resources", "sws_ext_menu"), NamedCommandLookup("_S&M_SHOW_RESOURCES_VIEW"));
	AddToMenu(hMenu, __LOCALIZE("Snapshots", "sws_ext_menu"), NamedCommandLookup("_SWSSNAPSHOT_OPEN"));
	AddToMenu(hMenu, __LOCALIZE("Tracklist", "sws_ext_menu"), NamedCommandLookup("_SWSTL_OPEN"));
	AddToMenu(hMenu, __LOCALIZE("Zoom preferences", "sws_ext_menu"), NamedCommandLookup("_SWS_ZOOMPREFS"));

	AddToMenu(hMenu, SWS_SEPARATOR, 0);

#ifdef _SWS_LOCALIZATION
	HMENU hLangPackSubMenu = CreatePopupMenu();
	AddSubMenu(hMenu, hLangPackSubMenu, __LOCALIZE("SWS Language file", "sws_ext_menu"));
	AddToMenu(hLangPackSubMenu, __LOCALIZE("Load LangPack file...", "sws_ext_menu"), NamedCommandLookup("_S&M_LOAD_LANGPACK"));
	AddToMenu(hLangPackSubMenu, __LOCALIZE("Reset to factory settings (English)", "sws_ext_menu"), NamedCommandLookup("_S&M_RESET_LANGPACK"));
#ifdef _SWS_DEBUG
	AddToMenu(hLangPackSubMenu, __LOCALIZE("[Internal] Generate actions LangPack file...", "sws_ext_menu"), NamedCommandLookup("_S&M_GEN_LANGPACK"));
#endif
#endif

	HMENU hOptionsSubMenu = CreatePopupMenu();
	AddSubMenu(hMenu, hOptionsSubMenu, __LOCALIZE("SWS Options", "sws_ext_menu"));
	AddToMenu(hOptionsSubMenu, __LOCALIZE("Enable auto coloring", "sws_ext_menu"), NamedCommandLookup("_SWSAUTOCOLOR_ENABLE"));
	AddToMenu(hOptionsSubMenu, __LOCALIZE("Enable auto icon", "sws_ext_menu"), NamedCommandLookup("_S&MAUTOICON_ENABLE"));
	AddToMenu(hOptionsSubMenu, __LOCALIZE("Enable marker actions", "sws_ext_menu"), NamedCommandLookup("_SWSMA_TOGGLE"));
	AddToMenu(hOptionsSubMenu, __LOCALIZE("Enable record input check", "sws_ext_menu"), NamedCommandLookup("_SWS_TOGRECINCHECK"));
	AddToMenu(hOptionsSubMenu, __LOCALIZE("Enable red ruler while recording", "sws_ext_menu"), NamedCommandLookup("_SWS_RECREDRULER"));
	AddToMenu(hOptionsSubMenu, __LOCALIZE("Enable toolbars auto refresh", "sws_ext_menu"), NamedCommandLookup("_S&M_TOOLBAR_REFRESH_ENABLE"));
}