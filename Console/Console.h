/******************************************************************************
/ Console.h
/
/ Copyright (c) 2009 and later Tim Payne (SWS), Jeffos
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
	FX_ADD,
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
	MARKER_ADD,
	OSC_CMD,
	WRITE_STATE,
	HELP_CMD,
	UNKNOWN_COMMAND,
	NUM_COMMANDS,
} CONSOLE_COMMAND;

typedef struct {
	CONSOLE_COMMAND	command;
	char			cPrefix;
	char			cKey;
	int				iNumArgs;  // + 8 if non-numeric args, + 16 for no track args either
	const char*		cHelpPrefix;
	const char*		cHelpSuffix;
} console_COMMAND_T;

#define ARGS_MASK    7
#define STRING_ARG   8
#define NOTRACK_ARG  16
#define NUMERIC_ARGS(a) (g_commands[(a)].iNumArgs > 0 && !(g_commands[(a)].iNumArgs & STRING_ARG))

int ConsoleInit();
void ConsoleExit();
CONSOLE_COMMAND ParseConsoleCommand(char *strCommand, char **trackid, char **args);
void RunConsoleCommand(const char* cmd);
bool LoadConsoleCmds(WDL_PtrList<WDL_FastString>* _outCmds);

class ReaConsoleWnd : public SWS_DockWnd
{
public:
	ReaConsoleWnd();
	void OnCommand(WPARAM wParam, LPARAM lParam);
	void GetMinSize(int* w, int* h) { *w=100; *h=38; }
	void ShowConsole();
	void Update();
protected:
	void OnInitDlg();
	HMENU OnContextMenu(int x, int y, bool* wantDefaultItems);
	int OnKey(MSG* msg, int iKeyState);
	void OnResize();
private:
	char m_strCmd[256];
	char* m_pTrackId;
	char* m_pArgs;
	CONSOLE_COMMAND m_cmd;
};

