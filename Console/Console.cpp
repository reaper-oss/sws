/******************************************************************************
/ Console.cpp
/
/ Copyright (c) 2011 and later Tim Payne (SWS), Jeffos
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
/ This presents a console to the user
/ Setup action "SWS: Open console", default 'c' key
/ (mapped by REAPERs built in keyboard/command mapping dialog)
/
******************************************************************************/


#include "stdafx.h"

#include "../url.h"
#include "../Freeze/Freeze.h"
#include "../SnM/SnM.h"
#include "../SnM/SnM_CSurf.h"
#include "../SnM/SnM_Util.h"
#include "Console.h"

#include <WDL/localize/localize.h>

ReaConsoleWnd* g_pConsoleWnd = NULL;
static WDL_TypedBuf<int> g_selTracks;
static char g_cLastKey = 0;
static DWORD g_dwLastKeyMsg = 0;
#define CONSOLE_WINDOWPOS_KEY "ReaConsoleWindowPos"
bool g_bCloseOnReturnPref = false;

void ParseTrackId(char* strId, bool bReset = true);
void ProcessCommand(CONSOLE_COMMAND command, const char* args);
const char* StatusString(CONSOLE_COMMAND command, const char* args);

typedef struct CUSTOM_COMMAND
{
	gaccel_register_t accel;
	const char* id;
	void (*doCommand)(void);
	const char* menuText;
	bool isSubMenu;
} CUSTOM_COMMAND;

WDL_PtrList<CUSTOM_COMMAND> g_customCommands;

//!WANT_LOCALIZE_STRINGS_BEGIN:sws_DLG_100
static console_COMMAND_T g_commands[NUM_COMMANDS] = 
{
	{ SOLO_ENABLE,    '+', 'o',  0, "Enable solo on " ,      0 },
	{ SOLO_DISABLE,   '-', 'o',  0, "Disable solo on ",      0 },
	{ SOLO_TOGGLE,      0, 'o',  0, "Toggle solo on ",       0 },
	{ SOLO_EXCLUSIVE,   0, 'O',  0, "Solo only ",            0 },
	{ MUTE_ENABLE,    '+', 'm',  0, "Enable mute on ",       0 },
	{ MUTE_DISABLE,   '-', 'm',  0, "Disable mute on ",      0 },
	{ MUTE_TOGGLE,      0, 'm',  0, "Toggle mute on ",       0 },
	{ MUTE_EXCLUSIVE,   0, 'M',  0, "Mute only ",            0 },
	{ ARM_ENABLE,     '+', 'a',  0, "Enable arm on ",        0 },
	{ ARM_DISABLE,    '-', 'a',  0, "Disable arm on ",       0 },
	{ ARM_TOGGLE,       0, 'a',  0, "Toggle arm on ",        0 },
	{ ARM_EXCLUSIVE,    0, 'A',  0, "Arm only ",             0 },
	{ PHASE_ENABLE,   '+', 'h',  0, "Invert phase on ",      0 },
	{ PHASE_DISABLE,  '-', 'h',  0, "Normal phase on ",      0 },
	{ PHASE_TOGGLE,     0, 'h',  0, "Flip phase on ",        0 },
	{ PHASE_EXCLUSIVE,  0, 'H',  0, "Invert phase on only ", 0 },
	{ SELECT_ENABLE,  '+', 's',  0, "Select ",               0 },
	{ SELECT_DISABLE, '-', 's',  0, "Unselect ",             0 },
	{ SELECT_TOGGLE,    0, 's',  0, "Toggle select on ",     0 },
	{ SELECT_EXCLUSIVE, 0, 'S',  0, "Select only ",          0 },
	{ FX_ENABLE,      '+', 'f',  0, "Enable FX on ",         0 },
	{ FX_DISABLE,     '-', 'f',  0, "Disable FX on ",        0 },
	{ FX_TOGGLE,        0, 'f',  0, "Toggle FX enable on ",  0 },
	{ FX_EXCLUSIVE,     0, 'F',  0, "Enable FX on only ",    0 },
	{ FX_ADD,           0, 'x',  9, "Add FX on ",            " %s" },
	{ VOLUME_SET,       0, 'V',  1, "Set volume on ",        " %s dB" },
	{ VOLUME_TRIM,      0, 'v',  1, "Trim volume on ",       " %s dB" },
	{ PAN_SET,          0, 'P',  1, "Set pan on ",           " %s%" },
	{ PAN_TRIM,         0, 'p',  1, "Trim pan on ",          " %s%" },
	{ NAME_SET,         0, 'n',  9, "Name ",                 " to %s" },
	{ NAME_PREFIX,      0, 'b',  9, "Name ",                 " with prefix %s" },
	{ NAME_SUFFIX,      0, 'z',  9, "Name ",                 " with suffix %s" },
	{ CHANNELS_SET,     0, 'l',  1, "Set # channels on ",    " to %s" },
	{ INPUT_SET,        0, 'i',  1, "Set input on ",         " to %s" },
	{ COLOR_SET,        0, 'c',  9, "Change color on ",      " to %s" },
	{ MARKER_ADD,       0, '!', 25, "Insert action marker ", "!%s" },
	{ OSC_CMD,          0, '/', 25, "Send local OSC message", 0 },
	{ WRITE_STATE,      0, 'W', 25, "Write non-persistent extended state", 0 },
	{ HELP_CMD,         0, '?', -1, SWS_URL_HELP_DIR"/reaconsole.php", 0 },
	{ UNKNOWN_COMMAND,  0,   0, -1, "Enter a command...",    0 },
};
//!WANT_LOCALIZE_STRINGS_END

// Split into various categories, independent of actually having a correct and/or finished command string
// Basically just a fancy tokenizer
CONSOLE_COMMAND ParseConsoleCommand(char* strCommand, char** trackid, char** args)
{
	*trackid = *args = (char*)"";
	char* p;
	CONSOLE_COMMAND command = UNKNOWN_COMMAND;
	int index = 0;
	int i;
	if (!strCommand || !strCommand[0])
		return UNKNOWN_COMMAND;

	if ((strCommand[0] == '-' || strCommand[0] == '+') && strCommand[1])
		index++;

	// Try to find out the command
	for (i = 0; i < NUM_COMMANDS; i++)
		if (g_commands[i].cKey == strCommand[index] && 
			(!g_commands[i].cPrefix || g_commands[i].cPrefix == strCommand[0]))
		{
			command = g_commands[i].command;
			break;
		}
	if (i == NUM_COMMANDS)
		return UNKNOWN_COMMAND;

	*trackid = &strCommand[index+1];

	if ((g_commands[command].iNumArgs & ARGS_MASK) <= 0)
		return command;

	if (!(g_commands[command].iNumArgs & NOTRACK_ARG))
	{
		// If there's a semicolon, treat that as the space between the tracks id and the arguments
		// Looks at just the first semicolon
		p = strchr(*trackid, ';');
		if (p)
		{
			p[0] = 0;
			*args = p+1;
		}

		p = strchr(*trackid, ' ');
		if (p)
		{
			if (p == *trackid)
				*trackid = (char*)"";
			p[0] = 0;
			// Go to first non-space char
			while (*(++p) == ' ');
			*args = p;
		}
		else
		{
			// No spaces, assume it's the argument that's being typed
			*args = *trackid;
			*trackid = (char*)"";
		}
	}
	else
	{
		*args = *trackid;
		*trackid = (char*)"";
		return command;
	}

	// Make sure a numeric argument is valid
	if (NUMERIC_ARGS(command) &&
		(!isdigit(**args) && !((**args == '-' || **args == '+' || **args == '.') && isdigit((*args)[1]))))
	{
		if (!**trackid)
			*trackid = *args;
		*args = (char*)"";
	}

	return command;
}

// ParseId fills in array of ints (g_selTracks.Get()) according to id string
void ParseTrackId(char* strId, bool bReset)
{
	int track;
	const char* cName;
	char* p;
	bool bChildren = false;
	bool bInvert = false;

	if (!strId || !GetNumTracks())
		return;

	if (bReset)
	{
		g_selTracks.Resize(GetNumTracks(), false);
		memset(g_selTracks.Get(), 0, g_selTracks.GetSize() * sizeof(int));
	}

	// Comma seperated list or whitespace? strip and recall
	if (strchr(strId, ','))
	{
		char temp[128];
		char* token;
		strncpy(temp, strId, 128);
		temp[127]=0; // Just in case
		token = strtok(temp, ",");
		if (!token)
			ParseTrackId((char*)"", false);
		while (token)
		{
			ParseTrackId(token, false);
			token = strtok(NULL, ",");
		}
		return;
	}

	// Strip out / and signify a child
	while ((p = strchr(strId, '/')) != NULL)
	{
		for (; *p; p++)
			*p = *(p+1);
		bChildren = true;
	}

	// Ignore the beginning spaces
	while (strId[0] == ' ')
		strId++;

	// If the first char is a ! invert selection
	if (strId[0] == '!')
	{
		strId++;
		bInvert = true;
	}

	// If the string is "all" or exactly "*", select all tracks.
	if (_stricmp(strId, __LOCALIZE("all","sws_DLG_100")) == 0 || strcmp(strId, "*") == 0)
		for (track = 0; track < GetNumTracks(); track++)
			g_selTracks.Get()[track] = 1;

	// If the string is empty, use the tracks' selected flags
	else if (strId[0] == 0)
		for (track = 0; track < GetNumTracks(); track++)
		{
			// If tracks were selected before (because of a comma separated list) don't change it here.
			if (g_selTracks.Get()[track])
				break;
			int iSel = *((int*)GetSetMediaTrackInfo(CSurf_TrackFromID(track+1, false), "I_SELECTED", NULL));
			g_selTracks.Get()[track] = iSel;
		}

	// If a range is specified, select those numbers
	else if ((p = strchr(strId, '-')) != NULL)
	{
		// Make sure the string is valid:
		int nondigchars = 0;
		for (int i = 0; i < (int)strlen(strId); i++)
			if (!isdigit(strId[i]))
				nondigchars++;
		if (nondigchars != 1)
			return;

		int start = atol(strId);
		int end = atol(p+1);
		if (start < 1)
			start = 1;
		if (end > GetNumTracks())
			end = GetNumTracks();
		for (track = start-1; track < end; track++)
			g_selTracks.Get()[track] = 1;
	}
	
	// If a wildcard is in the string, use loose matches
	else if ((p = strchr(strId, '*')) != NULL)
	{
		char* strMatch = NULL;
		for (track = 0; track < GetNumTracks(); track++)
		{
			cName = (const char*)GetSetMediaTrackInfo(CSurf_TrackFromID(track+1, false), "P_NAME", NULL);
			if (cName && cName[0])
			{
				if (p == strId && _strnicmp(strId+1, cName+1+strlen(cName)-strlen(strId), strlen(strId)-1) == 0)
					g_selTracks.Get()[track] = 1;
				else if (p-strId == strlen(strId) - 1 && _strnicmp(strId, cName, strlen(strId)-1) == 0)
					g_selTracks.Get()[track] = 1;
				// This "should" be the double wildcard case, but check anyway
				else if (strId[0] == '*' && strlen(strId) > 2 && strId[strlen(strId)-1] == '*')
				{
					if (!strMatch)
					{
						strMatch = new char[strlen(strId)-1];
						lstrcpyn(strMatch, strId+1, (int)strlen(strId)-1);
					}
					if (stristr(cName, strMatch))
						g_selTracks.Get()[track] = 1;
				}
			}
		}
		delete [] strMatch;
	}
	// Check for exact numeric
	else if ((track = atol(strId)) > 0 && track <= GetNumTracks())
		g_selTracks.Get()[track-1] = 1;

	// Check for exact name matches, with "auto compelete"
	//   e.g. if there's no exact match, but only one track that starts with the string, select that one
	else
	{
		int iCloseMatch = 0;
		int iExactMatch = 0;
		int iMatchedTrack;
		for (track = 0; track < GetNumTracks(); track++)
		{
			cName = (const char*)GetSetMediaTrackInfo(CSurf_TrackFromID(track+1, false), "P_NAME", NULL);
			// Exact name match
			if (cName[0] && _stricmp(strId, cName) == 0)
			{
				iExactMatch++;
				g_selTracks.Get()[track] = 1;
			}
			// Exact number match
			else if (atol(strId) == track+1)
			{
				iExactMatch++;
				g_selTracks.Get()[track] = 1;
			}
			// Check for close match
			else if (cName[0] && _strnicmp(strId, cName, strlen(strId)) == 0)
			{
				iCloseMatch++;
				iMatchedTrack = track;
			}
		}

		if (!iExactMatch && iCloseMatch == 1)
			g_selTracks.Get()[iMatchedTrack] = 1;
	}

	if (bChildren)
	{
		int iParentDepth;
		bool bSelected = false;
		MediaTrack* gfd = NULL;
		for (int i = 0; i < GetNumTracks(); i++)
		{
			MediaTrack* tr = CSurf_TrackFromID(i+1, false);
			int iType;
			int iFolder = GetFolderDepth(tr, &iType, &gfd);

			if (bSelected)
				g_selTracks.Get()[i] = 1;

			if (iType == 1 && !bSelected && g_selTracks.Get()[i])
			{
				iParentDepth = iFolder;
				bSelected = true;
			}

			if (iType + iFolder <= iParentDepth)
				bSelected = false;
		}
	}

	if (bInvert)
	{
		for (int i = 0; i < GetNumTracks(); i++)
			g_selTracks.Get()[i] = g_selTracks.Get()[i] ? 0 : 1;
	}
}

// Here's where we actually do the command from the user
void ProcessCommand(CONSOLE_COMMAND command, const char* args)
{
	int i;
	bool b;
	double dVal = 0.0;
	double d;
	int count = 0;

	if (command >= NUM_COMMANDS || (g_commands[command].iNumArgs > 0 && (!args || !args[0])))
		return;

	if (NUMERIC_ARGS(command))
		dVal = atof(args);

	if (g_commands[command].iNumArgs & NOTRACK_ARG)
	{
		switch(command)
		{
		case MARKER_ADD:
			{
				char markerStr[64];
				snprintf(markerStr, sizeof(markerStr), "!%s", args);
				AddProjectMarker(NULL, false, GetCursorPosition(), 0.0, markerStr, -1);
				UpdateTimeline();
				break;
			}
		case OSC_CMD:
			{
				char oscStr[256];
				snprintf(oscStr, sizeof(oscStr), "/%s", args);
				SNM_SendLocalOscMessage(oscStr);
				break;
			}
		case WRITE_STATE:
			{
				const char *section = strtok((char *)args, " ");
				const char *key = strtok(NULL, " ");
				if (section && key) {
					const char *value = strtok(NULL, "");
					if (value)
						SetExtState(section, key, value, false);
					else
						DeleteExtState(section, key, false);
				}
				break;
			}
		default:
			break;
		}
		return;
	}

	for (int track = 0; track < GetNumTracks(); track++)
	{
		MediaTrack* pMt = CSurf_TrackFromID(track+1, false);
		// Do the class of commands that works only on the selected track
		if (g_selTracks.Get()[track])
		{
			switch(command)
			{
			case SOLO_ENABLE:
				GetSetMediaTrackInfo(pMt, "I_SOLO", &g_i2);
				break;
			case SOLO_DISABLE:
				GetSetMediaTrackInfo(pMt, "I_SOLO", &g_i0);
				break;
			case SOLO_TOGGLE:
				i = *((int*)GetSetMediaTrackInfo(pMt, "I_SOLO", NULL)) ? 0 : 2;
				GetSetMediaTrackInfo(pMt, "I_SOLO", &i);
				break;
			case MUTE_ENABLE:
				GetSetMediaTrackInfo(pMt, "B_MUTE", &g_bTrue);
				break;
			case MUTE_DISABLE:
				GetSetMediaTrackInfo(pMt, "B_MUTE", &g_bFalse);
				break;
			case MUTE_TOGGLE:
				b = !(*((bool*)GetSetMediaTrackInfo(pMt, "B_MUTE", NULL)));
				GetSetMediaTrackInfo(pMt, "B_MUTE", &b);
				break;
			case ARM_ENABLE:
				GetSetMediaTrackInfo(pMt, "I_RECARM", &g_i1);
				break;
			case ARM_DISABLE:
				GetSetMediaTrackInfo(pMt, "I_RECARM", &g_i0);
				break;
			case ARM_TOGGLE:
				i = *((int*)GetSetMediaTrackInfo(pMt, "I_RECARM", NULL)) ? 0 : 1;
				GetSetMediaTrackInfo(pMt, "I_RECARM", &i);
				break;
			case PHASE_ENABLE:
				GetSetMediaTrackInfo(pMt, "B_PHASE", &g_bTrue);
				break;
			case PHASE_DISABLE:
				GetSetMediaTrackInfo(pMt, "B_PHASE", &g_bFalse);
				break;
			case PHASE_TOGGLE:
				b = !(*((bool*)GetSetMediaTrackInfo(pMt, "B_PHASE", NULL)));
				GetSetMediaTrackInfo(pMt, "B_PHASE", &b);
				break;
			case SELECT_ENABLE:
				GetSetMediaTrackInfo(pMt, "I_SELECTED", &g_i1);
				break;
			case SELECT_DISABLE:
				GetSetMediaTrackInfo(pMt, "I_SELECTED", &g_i0);
				break;
			case SELECT_TOGGLE:
				i = *((int*)GetSetMediaTrackInfo(pMt, "I_SELECTED", NULL)) ? 0 : 1;
				GetSetMediaTrackInfo(pMt, "I_SELECTED", &i);
				break;
			case FX_ENABLE:
				GetSetMediaTrackInfo(pMt, "I_FXEN", &g_i1);
				break;
			case FX_DISABLE:
				GetSetMediaTrackInfo(pMt, "I_FXEN", &g_i0);
				break;
			case FX_TOGGLE:
				i = *((int*)GetSetMediaTrackInfo(pMt, "I_FXEN", NULL)) ? 0 : 1;
				GetSetMediaTrackInfo(pMt, "I_FXEN", &i);
				break;
			case FX_ADD:
				TrackFX_GetByName(pMt, args, true);
				break;
			case VOLUME_SET:
				d = DB2VAL(dVal);
				GetSetMediaTrackInfo(pMt, "D_VOL", &d);
				break;
			case VOLUME_TRIM:
				d = *((double*)GetSetMediaTrackInfo(pMt, "D_VOL", NULL));
				d = DB2VAL(VAL2DB(d) + dVal);
				GetSetMediaTrackInfo(pMt, "D_VOL", &d);
				break;
			case PAN_SET:
				d = dVal / 100.0;
				GetSetMediaTrackInfo(pMt, "D_PAN", &d);
				break;
			case PAN_TRIM:
				d = *((double*)GetSetMediaTrackInfo(pMt, "D_PAN", NULL));
				d += dVal/100.0;
				if (d > 1.0)
					d = 1.0;
				else if (d < -1.0)
					d = -1.0;
				GetSetMediaTrackInfo(pMt, "D_PAN", &d);
				break;
			case NAME_SET:
				GetSetMediaTrackInfo(pMt, "P_NAME", (char*)args);
				break;
			case NAME_PREFIX:
			{
				char cName[256];
				snprintf(cName, 256, "%s %s", args, (char*)GetSetMediaTrackInfo(pMt, "P_NAME", NULL));
				GetSetMediaTrackInfo(pMt, "P_NAME", cName);
				break;
			}
			case NAME_SUFFIX:
			{
				char cName[256];
				snprintf(cName, 256, "%s %s", (char*)GetSetMediaTrackInfo(pMt, "P_NAME", NULL), args);
				GetSetMediaTrackInfo(pMt, "P_NAME", cName);
				break;
			}
			case CHANNELS_SET:
				i = (int)dVal;
				if (i >= 2 && i <= 64 && i % 2 == 0)
					GetSetMediaTrackInfo(pMt, "I_NCHAN", &i);
				break;
			case INPUT_SET:
				i = (int)dVal-1;
				if (strchr(args, '-'))
					i += count++;
				if (strchr(args, 's'))
					i |= 1024;
				if (strchr(args, 'r'))
					i |= 512;
				else if (strchr(args, 'm'))
					i += 4096 | ((63 << 5) + 1);
				GetSetMediaTrackInfo(pMt, "I_RECINPUT", &i);
				break;
			case COLOR_SET:
				i = -1;
				if (_stricmp(args, "red") == 0)
					i = RGB(255, 0, 0);
				else if (_stricmp(args, __LOCALIZE("blue","sws_DLG_100")) == 0)
					i = RGB(0, 0, 255);
				else if (_stricmp(args, __LOCALIZE("green","sws_DLG_100")) == 0)
					i = RGB(0, 255, 0);
				else if (_stricmp(args, __LOCALIZE("grey","sws_DLG_100")) == 0 || _stricmp(args, __LOCALIZE("gray","sws_DLG_100")) == 0)
					i = RGB(128, 128, 128);
				else if (_stricmp(args, __LOCALIZE("black","sws_DLG_100")) == 0)
					i = RGB(0, 0, 0);
				else if (_stricmp(args, __LOCALIZE("white","sws_DLG_100")) == 0)
					i = RGB(255, 255, 255);
				else if (_stricmp(args, __LOCALIZE("yellow","sws_DLG_100")) == 0)
					i = RGB(255, 255, 0);
				else if (_stricmp(args, __LOCALIZE("cyan","sws_DLG_100")) == 0)
					i = RGB(0, 255, 255);
				else if (_stricmp(args, __LOCALIZE("purple","sws_DLG_100")) == 0 || _stricmp(args, "violet") == 0)
					i = RGB(255, 0, 255);
				else if (_stricmp(args, __LOCALIZE("orange","sws_DLG_100")) == 0)
					i = RGB(255, 128, 0);
				else if (_stricmp(args, __LOCALIZE("magenta","sws_DLG_100")) == 0)
					i = RGB(255, 0, 128);
				else if (strstr(args, "0x") == args)
				{
					const long color = strtol(args, NULL, 16);
					i = RGB((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
				}
				else
				{
					int index = (int)atol(args) - 1;
					if (index >= 0 && index < 16)
					{
						char colstr[3] = { 0, 0, 0 };
						char custcolors[130];
						GetPrivateProfileString("REAPER", "custcolors", "", custcolors, 129, get_ini_file());
						i = 0;
						for (int j = 0; j < 3; j++)
						{
							strncpy(colstr, custcolors + index * 8 + j * 2, 2);
							i |= strtol(colstr, NULL, 16) << j * 8;
						}
					}
					else if (index == -1)
						i = 0;
				}

				if (i != -1)
				{
					if (i)
						i |= 0x1000000;
					GetSetMediaTrackInfo(pMt, "I_CUSTOMCOLOR", &i);
				}
				break;
			default:
				break;
			}
		}

		// Now do the class of commands that works on every track no matter what
		switch(command)
		{
		case SOLO_EXCLUSIVE:
			if (g_selTracks.Get()[track])
				GetSetMediaTrackInfo(pMt, "I_SOLO", &g_i2);
			else
				GetSetMediaTrackInfo(pMt, "I_SOLO", &g_i0);
			break;
		case MUTE_EXCLUSIVE:
			GetSetMediaTrackInfo(pMt, "B_MUTE", &g_selTracks.Get()[track]);
			break;
		case ARM_EXCLUSIVE:
			GetSetMediaTrackInfo(pMt, "I_RECARM", &g_selTracks.Get()[track]);
			break;
		case PHASE_EXCLUSIVE:
			GetSetMediaTrackInfo(pMt, "B_PHASE", &g_selTracks.Get()[track]);
			break;
		case SELECT_EXCLUSIVE:
			GetSetMediaTrackInfo(pMt, "I_SELECTED", &g_selTracks.Get()[track]);
			break;
		case FX_EXCLUSIVE:
			GetSetMediaTrackInfo(pMt, "I_FXEN", &g_selTracks.Get()[track]);
			break;
		default:
			break;
		}
	}
}

// Provide a human readable string of what's up:
const char* StatusString(CONSOLE_COMMAND command, const char* args)
{
	static char status[512];

	if (command >= NUM_COMMANDS)
		return __LOCALIZE("Internal error, contact SWS","sws_DLG_100");

	int n = snprintf(status, sizeof(status), "%s", __localizeFunc(g_commands[command].cHelpPrefix, "sws_DLG_100", LOCALIZE_FLAG_NOCACHE));

	// add space if localized (trailing ' ' cannot be retrieved from LangPack files)
	if (IsLocalized() && n>0 && n<(sizeof(status)-1) && status[n-1] != ' ')
	{
		strcat(status, " ");
		n++;
	}

	if (g_commands[command].iNumArgs < 0)
		return status;

	if (!(g_commands[command].iNumArgs & NOTRACK_ARG))
	{
		int previous_n = n;
		bool all = true;
		for (int i = 0; i < GetNumTracks(); i++)
			if (!g_selTracks.Get()[i])
			{
				all = false;
				break;
			}
		if (all)
		{
			n += snprintf(status + n, sizeof(status) - n, "all");
		}
		else
		{
			for (int i = 0; i < GetNumTracks(); i++)
				if (g_selTracks.Get()[i])
				{
					const char* cName = (const char*)GetSetMediaTrackInfo(CSurf_TrackFromID(i+1, false), "P_NAME", NULL);
					if (strlen(cName) + n + 20 >= 512)
						// Really grunge string overflow check.  Can't see the status string past two lines anyway.
						return status;
					if (cName && cName[0])
						n += snprintf(status + n, sizeof(status) - n, "[%d] %s, ", i+1, cName);
					else
						n += snprintf(status + n, sizeof(status) - n, "[%d], ", i+1);
				}

			if (n == previous_n)
				n += snprintf(status + n, sizeof(status) - n, "%s", __LOCALIZE("nothing","sws_DLG_100"));
			else
			{
				status[n-2] = 0; // take off last ", "
				n -= 2;
			}
		}
	}

	if (args && args[0] && g_commands[command].iNumArgs > 0 && g_commands[command].cHelpSuffix)
		n += snprintf(status + n, sizeof(status) - n, __localizeFunc(g_commands[command].cHelpSuffix, "sws_DLG_100", LOCALIZE_FLAG_VERIFY_FMTS), args);

	return status;
}

void ConsoleCommand(COMMAND_T* ct)
{
	g_cLastKey = (int)ct->user;
	if (g_pConsoleWnd)
		g_pConsoleWnd->ShowConsole();
}

void BringKeyCommand(COMMAND_T* = NULL)
{
	if (GetTickCount() - g_dwLastKeyMsg > 10)
		g_cLastKey = '\0';
	if (g_pConsoleWnd)
		g_pConsoleWnd->ShowConsole();
}

// primitive (no undo point)
void RunConsoleCommand(const char* cmd)
{
	char strCommand[128] = "";
	lstrcpyn(strCommand, cmd, sizeof(strCommand));
	char* pTrackId = strCommand;
	char* pArgs = strCommand;
	CONSOLE_COMMAND command = ParseConsoleCommand(strCommand, &pTrackId, &pArgs);
	ParseTrackId(pTrackId);
	ProcessCommand(command, pArgs);
}

void RunConsoleCommand(COMMAND_T* ct)
{
	const char* strCommand = (const char*)ct->user;
	RunConsoleCommand(strCommand);

	char cUndo[256];
	snprintf(cUndo, sizeof(cUndo), __LOCALIZE("ReaConsole custom command %s","sws_undo"), strCommand);
	Undo_OnStateChangeEx(cUndo, UNDO_STATE_ALL, -1); // UNDO_STATE_TRACKCFG is not enough (marker, osc, ..)
}

int IsConsoleDisplayed(COMMAND_T*) {
	return (g_pConsoleWnd && g_pConsoleWnd->IsWndVisible());
}

void EditCustomCommands(COMMAND_T*)
{
#ifdef _WIN32
	// Open the custom commands file in notepad
	char cArg[256], cNotepad[1024];
	strncpy(cArg, get_ini_file(), sizeof(cArg));
	char* pC = strrchr(cArg, PATH_SLASH_CHAR);
	if (!pC)
		return;
	strcpy(pC+1, "reaconsole_customcommands.txt");

	char* cWindir = getenv("windir");
	if (!cWindir)
		return;
	snprintf(cNotepad, sizeof(cNotepad), "%s\\notepad.exe", cWindir);
	_spawnl(_P_NOWAIT, cNotepad, cNotepad, cArg, NULL);
#endif
}


///////////////////////////////////////////////////////////////////////////////

// This is really just a hack to find out what key the user hit 
/*JFB oh yeah.. haha!
  I keep this code because 1) it works fine 2) replacing it with the new ReaConsoleWnd::OnKey()
  would look better but it would break the feature "SWS: Open console and copy keystroke"
*/
static int translateAccel(MSG *msg, accelerator_register_t *ctx)
{
	static bool bShift = false;

	if (msg->message == WM_KEYDOWN && msg->wParam == VK_SHIFT )
		bShift = true;
	else if (msg->message == WM_KEYUP && msg->wParam == VK_SHIFT)
		bShift = false;
	else if (msg->message == WM_KEYDOWN && msg->wParam >= '!' && msg->wParam <= 'Z')
	{
		g_dwLastKeyMsg = GetTickCount();
		g_cLastKey = (char)msg->wParam;
		if (!bShift && msg->wParam >= 'A')
			g_cLastKey += 0x20;
		else if (bShift && msg->wParam == '1')
			g_cLastKey = '!';
	}
	return 0;
}

static accelerator_register_t g_ar = { translateAccel, TRUE, NULL };


///////////////////////////////////////////////////////////////////////////////

//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] = 
{
	{ { { FVIRTKEY, 'C', 0 }, "SWS: Open console" },								"SWSCONSOLE",       ConsoleCommand,  "SWS ReaConsole", 0, IsConsoleDisplayed },
	{ { DEFACCEL,   "SWS: Open console and copy keystroke" },				"SWSCONSOLE2",      BringKeyCommand, NULL, },
	{ { DEFACCEL,   "SWS: Open console with 'S' to select track(s)" },		"SWSCONSOLEEXSEL",  ConsoleCommand,  NULL,   'S' },
	{ { DEFACCEL,   "SWS: Open console with 'n' to name track(s)" },		"SWSCONSOLENAME",   ConsoleCommand,  NULL,   'n' },
	{ { DEFACCEL,   "SWS: Open console with 'o' to solo track(s)" },		"SWSCONSOLESOLO",   ConsoleCommand,  NULL,   'o' },
	{ { DEFACCEL,   "SWS: Open console with 'a' to arm track(s)" },			"SWSCONSOLEARM",    ConsoleCommand,  NULL,   'a' },
	{ { DEFACCEL,   "SWS: Open console with 'm' to mute track(s)" },		"SWSCONSOLEMUTE",   ConsoleCommand,  NULL,   'm' },
	{ { DEFACCEL,   "SWS: Open console with 'f' to toggle FX enable" },		"SWSCONSOLEFX",     ConsoleCommand,  NULL,   'f' },
	{ { DEFACCEL,   "SWS: Open console with 'i' to set track(s) input" },	"SWSCONSOLEINPUT",  ConsoleCommand,  NULL,   'i' },
	{ { DEFACCEL,   "SWS: Open console with 'b' to prefix track(s)" },		"SWSCONSOLEPREFIX", ConsoleCommand,  NULL,   'b' },
	{ { DEFACCEL,   "SWS: Open console with 'z' to suffix track(s)" },		"SWSCONSOLESUFFIX", ConsoleCommand,  NULL,   'z' },
	{ { DEFACCEL,   "SWS: Open console with 'c' to color track(s)" },		"SWSCONSOLECOLOR",  ConsoleCommand,  NULL,   'c' },
	{ { DEFACCEL,   "SWS: Open console with 'h' to flip phase on track(s)" },"SWSCONSOLEPHASE", ConsoleCommand,  NULL,   'h' },
	{ { DEFACCEL,   "SWS: Open console with 'V' to set track(s) volume" },	"SWSCONSOLEVOL",    ConsoleCommand,  NULL,   'V' },
	{ { DEFACCEL,   "SWS: Open console with 'P' to set track(s) pan" },		"SWSCONSOLEPAN",    ConsoleCommand,  NULL,   'P' },
	{ { DEFACCEL,   "SWS: Open console with 'v' to trim volume on track(s)" },"SWSCONSOLEVOLT", ConsoleCommand,  NULL,   'v' },
	{ { DEFACCEL,   "SWS: Open console with 'p' to trim pan on track(s)" },	"SWSCONSOLEPANT",   ConsoleCommand,  NULL,   'p' },
	{ { DEFACCEL,   "SWS: Open console with 'l' to set track(s) # channels" },"SWSCONSOLECHAN", ConsoleCommand,  NULL,   'l' },
	{ { DEFACCEL,   "SWS: Open console with '!' to add action marker" },	"SWSCONSOLEMARKER", ConsoleCommand,  NULL,   '!' },
	{ { DEFACCEL,   "SWS/S&M: Open console with 'x' to add track FX" },		"S&M_CONSOLE_ADDFX",  ConsoleCommand,  NULL, 'x' },
	{ { DEFACCEL,   "SWS/S&M: Open console with '/' to send a local OSC message" }, "S&M_CONSOLE_OSC",  ConsoleCommand,  NULL, '/' },
#ifdef _WIN32
	{ { DEFACCEL,   "SWS: [Deprecated, use the Cycle Action editor instead] Edit console custom commands (restart needed after save)" }, "SWSCONSOLEEDITCUST",  EditCustomCommands,  NULL, },
#endif

	{ {}, LAST_COMMAND, }, // Denote end of table
};
//!WANT_LOCALIZE_1ST_STRING_END


///////////////////////////////////////////////////////////////////////////////

int ConsoleInit()
{
	if (!plugin_register("accelerator",&g_ar))
		return 0;

	g_bCloseOnReturnPref = (GetPrivateProfileInt("SWS", "CloseConsoleOnReturnKey", 0, get_ini_file()) == 1);

	SWSRegisterCommands(g_commandTable);

	// Add custom commands
	WDL_PtrList_DeleteOnDestroy<WDL_FastString> custCmds;
	if (LoadConsoleCmds(&custCmds))
	{
		char id[SNM_MAX_ACTION_CUSTID_LEN];
		char desc[SNM_MAX_ACTION_NAME_LEN];
		for (int i=0; i<custCmds.GetSize(); i++)
			if (custCmds.Get(i) && custCmds.Get(i)->GetLength())
				if (snprintfStrict(id, sizeof(id), "SWSCONSOLE_CUST%d", i+1) > 0) {
					snprintf(desc, sizeof(desc), __LOCALIZE_VERFMT("SWS: Run console command: %s","sws_actions"), custCmds.Get(i)->Get());
					SWSRegisterCommandExt(RunConsoleCommand, id, desc, (INT_PTR)_strdup(custCmds.Get(i)->Get()), false);
				}
	}
	g_pConsoleWnd = new ReaConsoleWnd();
	if (!g_pConsoleWnd)
		return 0;

	return 1;
}

void ConsoleExit()
{
	plugin_register("-accelerator",&g_ar);
	WritePrivateProfileString("SWS","CloseConsoleOnReturnKey",g_bCloseOnReturnPref?"1":"0",get_ini_file());
	DELETE_NULL(g_pConsoleWnd);
}

// _outCmds: it is up to the caller to unalloc items
bool LoadConsoleCmds(WDL_PtrList<WDL_FastString>* _outCmds)
{
	char buf[SNM_MAX_PATH] = "";
	if (_outCmds && snprintfStrict(buf, sizeof(buf), SNM_CONSOLE_FILE, GetResourcePath()) > 0)
	{
		char* c;
		_outCmds->Empty(true);
		if (FILE* f = fopenUTF8(buf, "r"))
		{
			while(fgets(buf, sizeof(buf), f))
			{
				if ((c = (char*)FindFirstRN(buf)) != NULL) *c = '\0';
				if (*buf && *buf!='[' && *buf!='/')
					_outCmds->Add(new WDL_FastString(buf));
			}
			fclose(f);
			return true;
		}
	}
	return false;
}


///////////////////////////////////////////////////////////////////////////////
// ReaConsoleWnd
///////////////////////////////////////////////////////////////////////////////

ReaConsoleWnd::ReaConsoleWnd()
	: SWS_DockWnd(IDD_CONSOLE, "ReaConsole", "ReaConsole")
{
	*m_strCmd = '\0';
	m_pTrackId = m_strCmd;
	m_pArgs = m_strCmd;
	m_cmd = UNKNOWN_COMMAND;

	// Must call SWS_DockWnd::Init() to restore parameters and open the window if necessary
	Init();
}

void ReaConsoleWnd::OnInitDlg()
{
	m_resize.init_item(IDC_COMMAND, 0.0, 0.0, 1.0, 0.0);
	m_resize.init_item(IDC_STATUS, 0.0, 0.0, 1.0, 0.0);
	SetWindowLongPtr(GetDlgItem(m_hwnd, IDC_COMMAND), GWLP_USERDATA, 0xdeadf00b);
	Update();
}

HMENU ReaConsoleWnd::OnContextMenu(int x, int y, bool* wantDefaultItems)
{
	HMENU hMenu = CreatePopupMenu();
#ifdef _WIN32
	AddToMenu(hMenu, __LOCALIZE("Close on ENTER key (CTRL+ENTER otherwise)","sws_DLG_100"), IDC_OPTIONS, -1, false, g_bCloseOnReturnPref?MFS_CHECKED:MFS_UNCHECKED);
#else
	AddToMenu(hMenu, __LOCALIZE("Close on ENTER key (CMD+ENTER otherwise)","sws_DLG_100"), IDC_OPTIONS, -1, false, g_bCloseOnReturnPref?MFS_CHECKED:MFS_UNCHECKED);
#endif
	return hMenu;
}

int ReaConsoleWnd::OnKey(MSG* msg, int iKeyState) 
{
	HWND h = GetDlgItem(m_hwnd, IDC_COMMAND);
/*JFB not needed: IDC_COMMAND is the single control of this window..
#ifdef _WIN32
	if (_msg->hwnd == h)
#else
	if (GetFocus() == h)
#endif
*/
	{
		if ((msg->message == WM_KEYDOWN || msg->message == WM_CHAR) &&
			msg->wParam == VK_RETURN)
		{
			SendMessage(m_hwnd, WM_COMMAND, MAKEWPARAM(IDC_APPLY, 0), 0);
			return 1; // eat
		}
		// ctrl+A => select all
		else if ((msg->message == WM_KEYDOWN || msg->message == WM_CHAR) &&
				 msg->wParam == 'A' && iKeyState == LVKF_CONTROL)
		{
			SetFocus(h);
			SendMessage(h, EM_SETSEL, 0, -1);
			return 1; // eat
		}
	}
	return 0; // pass-thru
}

void ReaConsoleWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch(LOWORD(wParam))
	{
		case IDC_OPTIONS:
			g_bCloseOnReturnPref = !g_bCloseOnReturnPref;
			break;
		case IDC_COMMAND:
			if (HIWORD(wParam)==EN_CHANGE)
				Update();
			break;
		case IDC_APPLY: // replaced IDOK: it would always close the window, see SWS_DockWnd::WndProc()
		{
			ProcessCommand(m_cmd, m_pArgs);

			char cUndo[256];
			snprintf(cUndo, sizeof(cUndo), __LOCALIZE("ReaConsole command %s","sws_undo"), m_strCmd);
			Undo_OnStateChangeEx(cUndo, UNDO_STATE_ALL, -1); // UNDO_STATE_TRACKCFG is not enough (marker, osc, ..)

			// close the window? if ctrl-enter was pressed by default, or enter if g_bCloseOnReturnPref is true (issue 588)
			if ((g_bCloseOnReturnPref && (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0x8000) ||
				(!g_bCloseOnReturnPref && (GetAsyncKeyState(VK_CONTROL) & 0x8000)))
			{
				m_bUserClosed = true;
				DestroyWindow(m_hwnd);
			}
			else
			{
				// Select all the text so the next typed clears the box
				HWND h = GetDlgItem(m_hwnd, IDC_COMMAND);
				SetFocus(h);
				SendMessage(h, EM_SETSEL, 0, -1);
			}

			break;
		}
		default:
			Main_OnCommand((int)wParam, (int)lParam);
			break;
	}
}

void ReaConsoleWnd::OnResize()
{
#ifdef _WIN32
	InvalidateRect(GetDlgItem(m_hwnd, IDC_STATUS), NULL, 0);
#endif
}

void ReaConsoleWnd::ShowConsole()
{
	Show(false, true);

	m_strCmd[0] = g_cLastKey;
	m_strCmd[1] = '\0';
	SetDlgItemText(m_hwnd, IDC_COMMAND, m_strCmd);

	// Move the cursor to the end 
	HWND h = GetDlgItem(m_hwnd, IDC_COMMAND);  
	SetFocus(h);
	SendMessage(h, EM_SETSEL, 1, 1);

	m_cmd = ParseConsoleCommand(m_strCmd, &m_pTrackId, &m_pArgs);
	ParseTrackId(m_pTrackId);
	SetDlgItemText(m_hwnd, IDC_STATUS, StatusString(m_cmd, m_pArgs));
}

void ReaConsoleWnd::Update()
{
	GetDlgItemText(m_hwnd, IDC_COMMAND, m_strCmd, 100);
	m_cmd = ParseConsoleCommand(m_strCmd, &m_pTrackId, &m_pArgs);
	ParseTrackId(m_pTrackId);
	SetDlgItemText(m_hwnd, IDC_STATUS, StatusString(m_cmd, m_pArgs));
}
