/******************************************************************************
/ SnM_Misc.cpp
/
/ Copyright (c) 2009-2010 Tim Payne (SWS), JF Bédague 
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
// Misc	actions
///////////////////////////////////////////////////////////////////////////////

// see http://forum.cockos.com/showthread.php?t=60657
void letREAPERBreathe(COMMAND_T* _ct)
{
#ifdef _WIN32
  static bool hasinit;
  if (!hasinit) { hasinit=true; InitCommonControls();  }
#endif
	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_SNM_WAIT), GetForegroundWindow(), WaitDlgProc);
}

void winWaitForEvent(DWORD _event, DWORD _timeOut, DWORD _minReTrigger)
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
void simulateMouseClick(COMMAND_T* _ct)
{
	POINT p; // not sure setting the pos is really needed..
	GetCursorPos(&p);
	mouse_event(MOUSEEVENTF_LEFTDOWN, p.x, p.y, 0, 0);
	mouse_event(MOUSEEVENTF_LEFTUP, p.x, p.y, 0, 0);
	winWaitForEvent(WM_LBUTTONUP);
}

// Create the Wiki ALR summary for the current section displayed in the "Action" dlg 
// This is the hack version, see clean but limited dumpWikiActions() below
// http://forum.cockos.com/showthread.php?t=61929

#define TITLE_SAVE_ALR_WIKI "Save ALR Wiki summary"

void dumpWikiActions2(COMMAND_T* _ct)
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
// S&M Util
///////////////////////////////////////////////////////////////////////////////

// GUI for lazy guys
void SNM_ShowConsoleMsg(const char* _msg, const char* _title, bool _clear)
{
	if (_clear) ShowConsoleMsg(""); //clear
	ShowConsoleMsg(_msg);

	// a little hack..
	if (_title) {
		HWND w = SearchWindow("ReaScript console output");
		if (w) SetWindowText(w, _title);
	}
}

bool SNM_DeleteFile(const char* _filename)
{
#ifdef _WIN32
	return (SendFileToRecycleBin(_filename) ? false : true); // avoid warning warning C4800
#else
	return (DeleteFile(_filename) ? true : false); // avoid warning warning C4800
#endif
}

HWND SearchWindow(const char* _title)
{
	HWND searchedWnd = NULL;
#ifdef _WIN32
	searchedWnd = FindWindow(NULL, _title);
#else
/*JFB TODO OSX: tried what follows that in the hope it'll work on OSX but it's KO (http://code.google.com/p/sws-extension/issues/detail?id=175#c83)
	if (GetMainHwnd())
	{
		HWND w = GetWindow(GetMainHwnd(), GW_HWNDFIRST);
		while (w)
		{ 
			if (IsWindowVisible(w) && GetWindow(w, GW_OWNER) == GetMainHwnd())
			{
				char name[BUFFER_SIZE] = "";
				int iLenName = GetWindowText(w, name, BUFFER_SIZE);
				if (!strcmp(name, _title)) {
					searchedWnd = w;
					break;
				}
			}
			w = GetWindow(w, GW_HWNDNEXT);
		}
	}
*/
#endif
	return searchedWnd;
}

HWND GetActionListBox(char* _currentSection, int _sectionMaxSize)
{
	HWND actionsWnd = SearchWindow("Actions");
	if (actionsWnd && _currentSection)
	{
		HWND cbSection = GetDlgItem(actionsWnd, 0x525);
		if (cbSection)
			GetWindowText(cbSection, _currentSection, _sectionMaxSize);
	}
	return (actionsWnd ? GetDlgItem(actionsWnd, 0x52B) : NULL);
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

// Browse a given directory in the default resource path and return shorten filename (if possible)
// If a file is chosen in this directory, just copy the filename relative to this path into _filename, full path otherwise (or if _fullPath is true)
// Returns false if cancelled
bool BrowseResourcePath(const char* _title, const char* _resSubDir, const char* _fileFilters, char* _filename, int _maxFilename, bool _fullPath)
{
	bool ok = false;

	//JFB3 more checks ?

	char defaultPath[BUFFER_SIZE] = "";
	sprintf(defaultPath, "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, _resSubDir);
	char* filename = BrowseForFiles(_title, defaultPath, NULL, false, _fileFilters);
	if (filename) 
	{
		if(!_fullPath && stristr(filename, defaultPath)) //JFB ignore case on OSX ?
			strncpy(_filename, (char*)(filename+strlen(defaultPath)+1), _maxFilename);
		else
			strncpy(_filename, filename, _maxFilename);
		free(filename);
		ok = true;
	}
	return ok;
}

void GetShortResourcePath(const char* _resSubDir, const char* _longFilename, char* _filename, int _maxFilename)
{
	if (_resSubDir && *_resSubDir && _longFilename && *_longFilename)
	{
		char defaultPath[BUFFER_SIZE] = "";
		sprintf(defaultPath, "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, _resSubDir);
		if(stristr(_longFilename, defaultPath)) //JFB ignore case on OSX ?
			strncpy(_filename, (char*)(_longFilename+strlen(defaultPath)+1), _maxFilename);
		else
			strncpy(_filename, _longFilename, _maxFilename);
	}
	else
		*_filename = '\0';
}

void GetFullResourcePath(const char* _resSubDir, const char* _shortFilename, char* _filename, int _maxFilename)
{
	if (_shortFilename && _filename) 
	{
		if (*_shortFilename == '\0')
			*_filename = '\0';
		else if (FileExists(_shortFilename))
			strncpy(_filename, _shortFilename, _maxFilename);
		else
			_snprintf(_filename, _maxFilename, "%s%c%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, _resSubDir, PATH_SLASH_CHAR, _shortFilename);
	}

	if (_filename && *_filename && !FileExists(_filename)) //just in case
		*_filename = '\0';
}

bool LoadChunk(const char* _filename, WDL_String* _chunk)
{
	if (_filename && *_filename && _chunk)
	{
		FILE* f = fopenUTF8(_filename, "r");
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
// tests..
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


///////////////////////////////////////////////////////////////////////////////
// Other
///////////////////////////////////////////////////////////////////////////////

// Create the Wiki ALR summary 
// no hack but limited to the main section and native actions
void dumpWikiActions(COMMAND_T* _ct)
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

void openStuff(COMMAND_T* _ct)
{
}

#endif


