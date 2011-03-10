/******************************************************************************
/ SnM_Misc.cpp
/
/ Copyright (c) 2009-2011 Tim Payne (SWS), Jeffos
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
#include "SNM_ChunkParserPatcher.h"

#ifdef _WIN32
#pragma comment (lib, "winmm.lib")
#endif


///////////////////////////////////////////////////////////////////////////////
// File util
///////////////////////////////////////////////////////////////////////////////

bool SNM_DeleteFile(const char* _filename) {
	if (_filename && *_filename) {
#ifdef _WIN32
	return (SendFileToRecycleBin(_filename) ? false : true); // avoid warning warning C4800
#else
	return (DeleteFile(_filename) ? true : false); // avoid warning warning C4800
#endif
	}
	return false;
}

// Browse + return short resource filename (if possible and if _wantFullPath == false)
// Returns false if cancelled
bool BrowseResourcePath(const char* _title, const char* _resSubDir, const char* _fileFilters, char* _filename, int _maxFilename, bool _wantFullPath)
{
	bool ok = false;
	char defaultPath[BUFFER_SIZE] = "";
	sprintf(defaultPath, "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, _resSubDir);
	char* filename = BrowseForFiles(_title, defaultPath, NULL, false, _fileFilters);
	if (filename) 
	{
		if(!_wantFullPath)
			GetShortResourcePath(_resSubDir, filename, _filename, _maxFilename);
		else
			strncpy(_filename, filename, _maxFilename);
		free(filename);
		ok = true;
	}
	return ok;
}

// Get a short filename from a full resource path
// ex: C:\Documents and Settings\<user>\Application Data\REAPER\FXChains\EQ\JS\test.RfxChain -> EQ\JS\test.RfxChain
// Notes: 
// - *must* work with non existing files (i.e. just some string processing here)
// - *must* be nop for non resource paths (c:\temp\test.RfxChain -> c:\temp\test.RfxChain)
// - *must* be nop for short resource paths 
void GetShortResourcePath(const char* _resSubDir, const char* _fullFn, char* _shortFn, int _maxFn)
{
	if (_resSubDir && *_resSubDir && _fullFn && *_fullFn)
	{
		char defaultPath[BUFFER_SIZE] = "";
		_snprintf(defaultPath, BUFFER_SIZE, "%s%c%s%c", GetResourcePath(), PATH_SLASH_CHAR, _resSubDir, PATH_SLASH_CHAR);
		if(stristr(_fullFn, defaultPath) == _fullFn) 
			strncpy(_shortFn, (char*)(_fullFn + strlen(defaultPath)), _maxFn);
		else
			strncpy(_shortFn, _fullFn, _maxFn);
	}
	else if (_shortFn)
		*_shortFn = '\0';
}

// Get a full resource path from a short filename
// ex: EQ\JS\test.RfxChain -> C:\Documents and Settings\<user>\Application Data\REAPER\FXChains\EQ\JS\test.RfxChain
// Notes: 
// - *must* work with non existing files
// - *must* be nop for non resource paths (c:\temp\test.RfxChain -> c:\temp\test.RfxChain)
// - *must* be nop for full resource paths 
void GetFullResourcePath(const char* _resSubDir, const char* _shortFn, char* _fullFn, int _maxFn)
{
	if (_shortFn && _fullFn) 
	{
		if (*_shortFn == '\0') {
			*_fullFn = '\0';
			return;
		}
		if (!stristr(_shortFn, GetResourcePath())) {

			char resFn[BUFFER_SIZE], resDir[BUFFER_SIZE];
			_snprintf(resFn, BUFFER_SIZE, "%s%c%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, _resSubDir, PATH_SLASH_CHAR, _shortFn);
			strcpy(resDir, resFn);
			char* p = strrchr(resDir, PATH_SLASH_CHAR);
			if (p) *p = '\0';
			if (FileExists(resDir)) {
				strncpy(_fullFn, resFn, _maxFn);
				return;
			}
		}
		strncpy(_fullFn, _shortFn, _maxFn);
	}
	else if (_fullFn)
		*_fullFn = '\0';
}

bool LoadChunk(const char* _fn, WDL_String* _chunk)
{
	if (_fn && *_fn && _chunk)
	{
		FILE* f = fopenUTF8(_fn, "r");
		if (f)
		{
			_chunk->Set("");
			char str[4096];
			while(fgets(str, 4096, f))
				_chunk->Append(str);
			fclose(f);
			return true;
		}
	}
	return false;
}

bool SaveChunk(const char* _fn, WDL_String* _chunk)
{
	if (_fn && *_fn && _chunk)
	{
		FILE* f = fopenUTF8(_fn, "w"); 
		if (f)
		{
			fputs(_chunk->Get(), f);
			fclose(f);
			return true;
		}
	}
	return false;
}

void GenerateFilename(const char* _dir, const char* _name, const char* _ext, char* _updatedFn, int _updatedSz)
{
	if (_dir && _name && _ext && _updatedFn && *_dir)
	{
		char fn[BUFFER_SIZE];
		bool slash = _dir[strlen(_dir)-1] == PATH_SLASH_CHAR;
		if (slash) _snprintf(fn, BUFFER_SIZE, "%s%s.%s", _dir, _name, _ext);
		else _snprintf(fn, BUFFER_SIZE, "%s%c%s.%s", _dir, PATH_SLASH_CHAR, _name, _ext);

		int i=0;
		while(FileExists(fn))
			if (slash) _snprintf(fn, BUFFER_SIZE, "%s%s_%03d.%s", _dir, _name, ++i, _ext);
			else _snprintf(fn, BUFFER_SIZE, "%s%c%s_%03d.%s", _dir, PATH_SLASH_CHAR, _name, ++i, _ext);
		strncpy(_updatedFn, fn, _updatedSz);
	}
}

void StringToExtensionConfig(char* _str, ProjectStateContext* _ctx)
{
	if (_str && _ctx)
	{
		char* pEOL = _str-1;
		char curLine[4096] = "";
		for(;;) 
		{
			char* pLine = pEOL+1;
			pEOL = strchr(pLine, '\n');
			if (!pEOL)
				break;
			int curLineLength = (int)(pEOL-pLine);
			memcpy(curLine, pLine, curLineLength);
			curLine[curLineLength] = '\0'; // min(SNM_MAX_CHUNK_LINE_LENGTH-1, curLineLength) ?
			_ctx->AddLine(curLine);
		}
	}
}

void ExtensionConfigToString(WDL_String* _str, ProjectStateContext* _ctx)
{
	if (_str && _ctx)
	{
		LineParser lp(false);
		char linebuf[4096];
		while(true)
		{
			if (!_ctx->GetLine(linebuf,sizeof(linebuf)) && !lp.parse(linebuf))
			{
				_str->Append(linebuf);
				_str->Append("\n");
				if (lp.gettoken_str(0)[0] == '>')
					break;
			}
			else
				break;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// Util
///////////////////////////////////////////////////////////////////////////////

int SNM_MinMax(int _val, int _min, int _max) {
	return min(_max, max(_min, _val));
}

bool GetALRStartOfURL(const char* _section, char* _sectionURL, int _sectionURLMaxSize)
{
	if (!_stricmp(_section, "Main") || !strcmp(_section, "Main (alt recording)"))
		strncpy(_sectionURL, "ALR_Main", _sectionURLMaxSize);
	else if (!_stricmp(_section, "Media explorer"))
		strncpy(_sectionURL, "ALR_MediaExplorer", _sectionURLMaxSize);
	else if (!_stricmp(_section, "MIDI Editor"))
		strncpy(_sectionURL, "ALR_MIDIEditor", _sectionURLMaxSize);
	else if (!_stricmp(_section, "MIDI Event List Editor"))
		strncpy(_sectionURL, "ALR_MIDIEvtList", _sectionURLMaxSize);
	else if (!_stricmp(_section, "MIDI Inline Editor"))
		strncpy(_sectionURL, "ALR_MIDIInline", _sectionURLMaxSize);
	else
		return false;
	return true;
}


///////////////////////////////////////////////////////////////////////////////
// Messages, prompt, etc..
///////////////////////////////////////////////////////////////////////////////

// GUI for lazy guys
void SNM_ShowConsoleMsg(const char* _msg, const char* _title, bool _clear)
{
	if (_clear) 
		ShowConsoleMsg(""); //clear
	ShowConsoleMsg(_msg);

	// a little hack..
	if (_title) 
	{
		HWND w = SearchWindow("ReaScript console output");
		if (w)
			SetWindowText(w, _title);
		else
			w = SearchWindow(_title);

		if (w) 
			SetForegroundWindow(w);
	}
}

// http://forum.cockos.com/showthread.php?t=48793
void SNM_ShowConsoleDbg(bool _clear, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int bufsize = 2048;
    char* buffer = (char*)malloc(bufsize);
#ifdef _WIN32
    while(buffer && _vsnprintf(buffer, bufsize, format, args) < 0) {
#else
	while(buffer && vsnprintf(buffer, bufsize, format, args) < 0) {
#endif
        bufsize *= 2;
        buffer = (char*)realloc(buffer, bufsize);
    }
    if (buffer)
		SNM_ShowConsoleMsg(buffer, "Debug", _clear);
    va_end(args);
    free(buffer);
}

//returns -1 on cancel, MIDI channel otherwise (0-based)
int PromptForMIDIChannel(const char* _title)
{
	int ch = -1;
	while (ch == -1)
	{
		char reply[8]= ""; // no default
		if (GetUserInputs(_title, 1, "MIDI Channel (1-16):", reply, 8))
		{
			ch = atoi(reply); //0 on error
			if (ch > 0 && ch <= 16) {
				return (ch-1);
			}
			else 
			{
				ch = -1;
				MessageBox(GetMainHwnd(), "Invalid MIDI channel!\nPlease enter a value in [1; 16].", "S&M - Error", /*MB_ICONERROR | */MB_OK);
			}
		}
		else return -1; // user has cancelled
	}
	return -1;
}

bool FileExistsErrMsg(const char* _fn, bool _errMsg)
{
	bool exists = false;
	if (_fn && *_fn)
	{
		exists = FileExists(_fn);
		if (!exists && _errMsg)
		{
			char buf[BUFFER_SIZE];
			_snprintf(buf, BUFFER_SIZE, "File not found:\n%s", _fn);
			MessageBox(g_hwndParent, buf, "S&M - Error", MB_OK);
		}
	}
	return exists;
}


///////////////////////////////////////////////////////////////////////////////
// Misc	actions
///////////////////////////////////////////////////////////////////////////////

// http://forum.cockos.com/showthread.php?t=60657
void LetREAPERBreathe(COMMAND_T* _ct)
{
#ifdef _WIN32
	static bool hasinit;
	if (!hasinit) { hasinit=true; InitCommonControls();  }
#endif
	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_SNM_WAIT), GetForegroundWindow(), WaitDlgProc);
}

void WinWaitForEvent(DWORD _event, DWORD _timeOut, DWORD _minReTrigger)
{
#ifdef _WIN32
	static DWORD waitTime = 0;
//	if ((timeGetTime() - waitTime) > _minReTrigger)
	{
		waitTime = timeGetTime();
		while((timeGetTime() - waitTime) < _timeOut) // for safety
		{
			MSG msg;
			if(PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
			{
				// new message to be processed
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				if(msg.message == _event)
					break;
			}
		}
	}
#endif
}

// http://forum.cockos.com/showthread.php?p=612065
void SimulateMouseClick(COMMAND_T* _ct)
{
	POINT p; // not sure setting the pos is really needed..
	GetCursorPos(&p);
	mouse_event(MOUSEEVENTF_LEFTDOWN, p.x, p.y, 0, 0);
	mouse_event(MOUSEEVENTF_LEFTUP, p.x, p.y, 0, 0);
	WinWaitForEvent(WM_LBUTTONUP);
}

// Create the Wiki ALR summary for the current section displayed in the "Action" dlg 
// This is the hack version, see clean but limited dumpWikiActions() below
// http://forum.cockos.com/showthread.php?t=61929

#define TITLE_SAVE_ALR_WIKI "Save ALR Wiki summary"

void DumpWikiActions2(COMMAND_T* _ct)
{
	char currentSection[64] = "";
	HWND hList = GetActionListBox(currentSection, 64);
	if (hList && currentSection)
	{
		char sectionURL[64]= ""; 
		if (!GetALRStartOfURL(currentSection, sectionURL, 64))
		{
			MessageBox(g_hwndParent, "Error: unknown section!", TITLE_SAVE_ALR_WIKI, MB_OK);
			return;
		}

		char fn[128]; char filename[BUFFER_SIZE];
		sprintf(fn, "%s%s.txt", sectionURL, _ct->user == 2 ? "_SWS" : _ct->user == 3 ? "_FNG" : "");
		if (!BrowseForSaveFile(TITLE_SAVE_ALR_WIKI, GetResourcePath(), fn, "Text files (*.txt)\0*.txt\0All files (*.*)\0*.*\0", filename, BUFFER_SIZE))
			return;

		//flush
		FILE* f = fopenUTF8(filename, "w"); 
		if (f)
		{
			fputs("\n", f);
			fclose(f);

			f = fopenUTF8(filename, "a"); 
			if (!f) return; //just in case..

			fprintf(f, "{| class=\"wikitable\"\n"); 
			fprintf(f, "|-\n"); 
			fprintf(f, "! Action name !! Cmd ID\n"); 

			LVITEM li;
			li.mask = LVIF_STATE | LVIF_PARAM;
			li.iSubItem = 0;
			for (int i = 0; i < ListView_GetItemCount(hList); i++)
			{
				li.iItem = i;
				ListView_GetItem(hList, &li);
				int cmdId = (int)li.lParam;

				char customId[64] = "";
				char cmdName[256] = "";
				ListView_GetItemText(hList,i,1,cmdName,256);
				ListView_GetItemText(hList,i,3,customId,64);

				if (!strstr(cmdName,"Custom:") &&
					//native only
					((_ct->user == 1 && !strstr(cmdName,"SWS:") && !strstr(cmdName,"SWS/") && !strstr(cmdName,"FNG:")) ||
					// SWS only
                    (_ct->user == 2 && (strstr(cmdName,"SWS:") || strstr(cmdName,"SWS/"))) ||
					// FNG only
					(_ct->user == 3 && strstr(cmdName,"FNG:") && !strstr(cmdName,"SWS"))))
				{
					if (!*customId) sprintf(customId, "%d", cmdId);
					fprintf(f, "|-\n| [[%s_%s|%s]] || %s\n", sectionURL, customId, cmdName, customId);
				}
			}
			fprintf(f, "|}\n");
			fclose(f);

			char msg[BUFFER_SIZE] = "";
			sprintf(msg, "Wrote %s", filename); 
			MessageBox(g_hwndParent, msg, TITLE_SAVE_ALR_WIKI, MB_OK);

		}
		else
			MessageBox(g_hwndParent, "Error: unable to write to file!", TITLE_SAVE_ALR_WIKI, MB_OK);
	}
	else
		MessageBox(g_hwndParent, "Error: action window not opened!", TITLE_SAVE_ALR_WIKI, MB_OK);
}


///////////////////////////////////////////////////////////////////////////////
// tests, other..
///////////////////////////////////////////////////////////////////////////////

#ifdef _SNM_MISC
void ShowTakeEnvPadreTest(COMMAND_T* _ct)
{
	bool updated = false;
	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i+1,false); // doesn't include master
		for (int j = 0; tr && j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* item = GetTrackMediaItem(tr,j);
			if (item && *(bool*)GetSetMediaItemInfo(item,"B_UISEL",NULL))
			{
				switch(_ct->user)
				{
					case 1: updated |= ShowTakeEnvPan(GetActiveTake(item)); break;
					case 2: updated |= ShowTakeEnvMute(GetActiveTake(item)); break;
					case 0: default: updated |= ShowTakeEnvVol(GetActiveTake(item)); break;
				}
			}
		}
	}
	if (updated)
	{
		UpdateTimeline();
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
	}
}

// Create the Wiki ALR summary 
// no hack but limited to the main section and native actions
void DumpWikiActions(COMMAND_T* _ct)
{
	char filename[BUFFER_SIZE], cPath[BUFFER_SIZE];
	strncpy(cPath, GetExePath(), BUFFER_SIZE);
	if (BrowseForSaveFile("Save ALR Wiki summary", cPath, NULL, "Text file (*.TXT)\0*.TXT\0All Files\0*.*\0", filename, BUFFER_SIZE))
	{
		//flush
		FILE* f = fopenUTF8(filename, "w"); 
		fputs("\n", f);
		fclose(f);

		f = fopenUTF8(filename, "a"); 
		if (f) 
		{ 
			fprintf(f, "{| class=\"wikitable\"\n"); 
			fprintf(f, "|-\n"); 
			fprintf(f, "! Action name !! Cmd ID\n"); 

			int i = 0;
			const char *cmdName;
			int cmdId;
			while( 0 != (cmdId = kbd_enumerateActions(NULL, i++, &cmdName))) 
			{
				if (!strstr(cmdName,"SWS:") && 
					!strstr(cmdName,"SWS/") && 
					!strstr(cmdName,"FNG:") && 
					!strstr(cmdName,"Custom:"))
				{
					fprintf(f, "|-\n"); 
					fprintf(f, "| [[ALR_%d|%s]] || %d\n", cmdId, cmdName, cmdId);
				}
			}			
			fprintf(f, "|}\n");
			fclose(f);
		}
		else
			MessageBox(g_hwndParent, "Unable to write to file.", "Save ALR Wiki summary", MB_OK);
	}
}
#endif


