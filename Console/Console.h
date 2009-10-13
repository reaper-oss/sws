/******************************************************************************
/ Console.h
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
#pragma once

// Command types:
typedef enum {
	SOLO_ENABLE,
	SOLO_DISABLE,
	SOLO_TOGGLE,
	SOLO_EXCLUSIVE,
	MUTE_ENABLE,
	MUTE_DISABLE,
	MUTE_TOGGLE,
	MUTE_EXCLUSIVE,
	ARM_ENABLE,
	ARM_DISABLE,
	ARM_TOGGLE,
	ARM_EXCLUSIVE,
	PHASE_ENABLE,
	PHASE_DISABLE,
	PHASE_TOGGLE,
	PHASE_EXCLUSIVE,
	SELECT_ENABLE,
	SELECT_DISABLE,
	SELECT_TOGGLE,
	SELECT_EXCLUSIVE,
	FX_ENABLE,
	FX_DISABLE,
	FX_TOGGLE,
	FX_EXCLUSIVE,
	VOLUME_SET,
	VOLUME_TRIM,
	PAN_SET,
	PAN_TRIM,
	NAME_SET,
	NAME_PREFIX,
	NAME_SUFFIX,
	CHANNELS_SET,
	INPUT_SET,
	COLOR_SET,
	HELP_CMD,
	UNKNOWN_COMMAND,
	NUM_COMMANDS,
} CONSOLE_COMMAND;

typedef struct {
	CONSOLE_COMMAND	command;
	char			cPrefix;
	char			cKey;
	int				iNumArgs;  // + 64 if non-numeric args, -1 for no track args either
	char*			cHelpPrefix;
	char*			cHelpSuffix;
} console_COMMAND_T;

#define NUMERIC_ARGS(a) (g_commands[(a)].iNumArgs > 0 && !(g_commands[(a)].iNumArgs & 64))

CONSOLE_COMMAND Tokenize(char* strCommand, char** trackid, char** args);
void ParseTrackId(char* strId);
void ProcessCommand(CONSOLE_COMMAND command, char* args);
char* StatusString(CONSOLE_COMMAND command, char* args);
int ConsoleInit();


