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

#ifdef _WIN32
#pragma comment (lib, "winmm.lib")
#endif


///////////////////////////////////////////////////////////////////////////////
// File util
// JFB!!! TODO: WDL_UTF8, exist_fn, ..
///////////////////////////////////////////////////////////////////////////////

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

bool SNM_CopyFile(const char* _destFn, const char* _srcFn)
{
	if (_destFn && _srcFn)
	{
		WDL_String chunk;
		if (LoadChunk(_srcFn, &chunk) && chunk.GetLength())
			return SaveChunk(_destFn, &chunk);
	}
	return false;
}


// Browse + return short resource filename (if possible and if _wantFullPath == false)
// Returns false if cancelled
bool BrowseResourcePath(const char* _title, const char* _resSubDir, const char* _fileFilters, char* _fn, int _fnSize, bool _wantFullPath)
{
	bool ok = false;
	char defaultPath[BUFFER_SIZE] = "";
	sprintf(defaultPath, "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, _resSubDir);
	char* filename = BrowseForFiles(_title, defaultPath, NULL, false, _fileFilters);
	if (filename) 
	{
		if(!_wantFullPath)
			GetShortResourcePath(_resSubDir, filename, _fn, _fnSize);
		else
			lstrcpyn(_fn, filename, _fnSize);
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
void GetShortResourcePath(const char* _resSubDir, const char* _fullFn, char* _shortFn, int _fnSize)
{
	if (_resSubDir && *_resSubDir && _fullFn && *_fullFn)
	{
		char defaultPath[BUFFER_SIZE] = "";
		_snprintf(defaultPath, BUFFER_SIZE, "%s%c%s%c", GetResourcePath(), PATH_SLASH_CHAR, _resSubDir, PATH_SLASH_CHAR);
		if(stristr(_fullFn, defaultPath) == _fullFn) 
			lstrcpyn(_shortFn, (char*)(_fullFn + strlen(defaultPath)), _fnSize);
		else
			lstrcpyn(_shortFn, _fullFn, _fnSize);
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
void GetFullResourcePath(const char* _resSubDir, const char* _shortFn, char* _fullFn, int _fnSize)
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
				lstrcpyn(_fullFn, resFn, _fnSize);
				return;
			}
		}
		lstrcpyn(_fullFn, _shortFn, _fnSize);
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
		lstrcpyn(_updatedFn, fn, _updatedSz);
	}
}

void StringToExtensionConfig(WDL_String* _str, ProjectStateContext* _ctx)
{
	if (_str && _ctx)
	{
		WDL_String unformatedStr;
		makeUnformatedConfigString(_str->Get(), &unformatedStr);

		char* pEOL = unformatedStr.Get()-1;
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

// Write a full INI file's section in one go
void SaveIniSection(const char* _iniSectionName, WDL_String* _iniSection, const char* _iniFn)
{
	if (_iniSectionName && _iniSection && _iniFn)
	{
/*JFB!!! doing that leads to something close to the issue 292 (ini file cache odd pb)
		if (_iniSection->GetLength())
			_iniSection->Append(" \n");
*/
		// "The data in the buffer pointed to by the lpString parameter consists 
		// of one or more null-terminated strings, followed by a final null character"
		char* buf = (char*)calloc(_iniSection->GetLength()+1, sizeof(char));
		lstrcpyn(buf, _iniSection->Get(), _iniSection->GetLength());
		for (int j=0; j < _iniSection->GetLength(); j++)
			if (buf[j] == '\n') 
				buf[j] = '\0';
		WritePrivateProfileStruct(_iniSectionName, NULL, NULL, 0, _iniFn); //flush section
		WritePrivateProfileSection(_iniSectionName, buf, _iniFn);
		free(buf);
	}
}


///////////////////////////////////////////////////////////////////////////////
// Util
///////////////////////////////////////////////////////////////////////////////

// REAPER bug: NamedCommandLookup() can return an id for an unregistered _cmdId 
int SNM_NamedCommandLookup(const char* _cmdId)
{
	int id = NamedCommandLookup(_cmdId);
	if (id)
	{
		const char* buf = kbd_getTextFromCmd((DWORD)id, NULL);
		if (buf && *buf)
			return id;
	}
	return 0;
}

int FindMarker(double _pos)
{
	int x=0, idx=-1; double dMarkerPos;
	// relies on markers indexed by positions
	while (x = EnumProjectMarkers(x, NULL, &dMarkerPos, NULL, NULL, NULL))
	{
		if (_pos >= dMarkerPos)
		{
			double dMarkerPos2;
			if (EnumProjectMarkers(x, NULL, &dMarkerPos2, NULL, NULL, NULL))
			{
				if (_pos < dMarkerPos2)
				{
					idx = x-1;
					break;
				}
			}
			else
				idx = x-1;
		}
	}
	return idx;
}

void makeUnformatedConfigString(const char* _in, WDL_String* _out)
{
	if (_in && _out)
	{
		_out->Set(_in);
		char* p = strstr(_out->Get(), "%");
		while(p)
		{
			int pos = p - _out->Get();
			_out->Insert("%", ++pos); // ++ops! but Insert() clamps to length..
			p = (pos+1 < _out->GetLength()) ? strstr((char*)(_out->Get()+pos+1), "%") : NULL;
		}
	}
}

bool GetStringWithRN(const char* _bufSrc, char* _buf, int _bufSize)
{
	if (!_buf || !_bufSrc)
		return false;

	memset(_buf, 0, sizeof(_buf));

	int i=0, j=0;
	while (_bufSrc[i] && i < _bufSize && j < _bufSize)
	{
		if (_bufSrc[i] == '\n') {
			_buf[j++] = '\r';
			_buf[j++] = '\n';
		}
		else if (_bufSrc[i] != '\r')
			_buf[j++] = _bufSrc[i];
		i++;
	}
	_buf[_bufSize-1] = 0; //just in case..
	return true;
}

void ShortenStringToFirstRN(char* _str) {
	if (_str) {
		char* p = strchr(_str, '\r');
		if (p) *p = '\0';
		p = strchr(_str, '\n');
		if (p) *p = '\0';
	}
}

// replace "%blabla" with '_replaceCh' in _str
void ReplaceStringFormat(char* _str, char _replaceCh) {
	if (_str && *_str)
		if (char* p = strstr(_str, "%")) {
			p[0] = _replaceCh;
			if (char* p2 = strstr((char*)(p+1), " "))
				memmove((char*)(p+1), p2, strlen(p2)+1);
			else
				p[1] = '\0'; //assumes there's another char just after '%'
		}
}

int SNM_MinMax(int _val, int _min, int _max) {
	return min(_max, max(_min, _val));
}

bool GetSectionName(bool _alr, const char* _section, char* _sectionURL, int _sectionURLSize)
{
	if (_alr)
	{
		if (!_stricmp(_section, "Main") || !strcmp(_section, "Main (alt recording)"))
			lstrcpyn(_sectionURL, "ALR_Main", _sectionURLSize);
		else if (!_stricmp(_section, "Media explorer"))
			lstrcpyn(_sectionURL, "ALR_MediaExplorer", _sectionURLSize);
		else if (!_stricmp(_section, "MIDI Editor"))
			lstrcpyn(_sectionURL, "ALR_MIDIEditor", _sectionURLSize);
		else if (!_stricmp(_section, "MIDI Event List Editor"))
			lstrcpyn(_sectionURL, "ALR_MIDIEvtList", _sectionURLSize);
		else if (!_stricmp(_section, "MIDI Inline Editor"))
			lstrcpyn(_sectionURL, "ALR_MIDIInline", _sectionURLSize);
		else return false;
	}
	else
	{
		if (_section && _sectionURL)
			lstrcpyn(_sectionURL, _section, _sectionURLSize);
		else return false;
	}
	return true;
}


///////////////////////////////////////////////////////////////////////////////
// Misc	actions
///////////////////////////////////////////////////////////////////////////////

#ifdef _SNM_MISC
// http://forum.cockos.com/showthread.php?t=60657
void LetREAPERBreathe(COMMAND_T* _ct)
{
#ifdef _WIN32
	static bool hasinit;
	if (!hasinit) { hasinit=true; InitCommonControls();  }
#endif
	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_SNM_WAIT), GetForegroundWindow(), WaitDlgProc);
}
#endif

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

// _type: 1 & 2 for ALR wiki (1=native actions, 2=SWS)
// _type: 3 & 4 for basic dump (3=native actions, 4=SWS)
bool dumpActionList(int _type, const char* _title, const char* _lineFormat, const char* _heading, const char* _ending)
{
	char currentSection[SNM_MAX_SECTION_NAME_LEN] = "";
	HWND hList = GetActionListBox(currentSection, SNM_MAX_SECTION_NAME_LEN);
	if (hList && currentSection)
	{
		char sectionURL[SNM_MAX_SECTION_NAME_LEN] = ""; 
		if (!GetSectionName(_type == 1 || _type == 2, currentSection, sectionURL, SNM_MAX_SECTION_NAME_LEN))
		{
			MessageBox(g_hwndParent, "Error: unknown section!", _title, MB_OK);
			return false;
		}

		char fn[SNM_MAX_SECTION_NAME_LEN*2]; char filename[BUFFER_SIZE];
		sprintf(fn, "%s%s.txt", sectionURL, !(_type % 2) ? "_SWS" : "");
		if (!BrowseForSaveFile(_title, GetResourcePath(), fn, "Text files (*.txt)\0*.txt\0All files (*.*)\0*.*\0", filename, BUFFER_SIZE))
			return false;

		//flush
		FILE* f = fopenUTF8(filename, "w"); 
		if (f)
		{
			fputs("\n", f);
			fclose(f);

			f = fopenUTF8(filename, "a"); 
			if (!f) return false; //just in case..

			if (_heading)
				fprintf(f, _heading); 

			LVITEM li;
			li.mask = LVIF_STATE | LVIF_PARAM;
			li.iSubItem = 0;
			for (int i = 0; i < ListView_GetItemCount(hList); i++)
			{
				li.iItem = i;
				ListView_GetItem(hList, &li);
				int cmdId = (int)li.lParam;

				char customId[SNM_MAX_ACTION_CUSTID_LEN] = "";
				char cmdName[SNM_MAX_ACTION_NAME_LEN] = "";
				ListView_GetItemText(hList, i, 1, cmdName, SNM_MAX_ACTION_NAME_LEN);
				ListView_GetItemText(hList, i, g_bv4 ? 4 : 3, customId, SNM_MAX_ACTION_CUSTID_LEN);

				if (!strstr(cmdName,"Custom:") &&
					((_type % 2 && !strstr(cmdName,"SWS:") && !strstr(cmdName,"SWS/")) ||
                     (!(_type % 2) && (strstr(cmdName,"SWS:") || strstr(cmdName,"SWS/")))))
				{
					if (!*customId) 
						_snprintf(customId, SNM_MAX_ACTION_CUSTID_LEN, "%d", cmdId);
					fprintf(f, _lineFormat, sectionURL, customId, cmdName, customId);
				}
			}
			if (_ending)
				fprintf(f, _ending); 

			fclose(f);

			char msg[BUFFER_SIZE] = "";
			_snprintf(msg, BUFFER_SIZE, "Wrote %s", filename); 
			MessageBox(g_hwndParent, msg, _title, MB_OK);
			return true;

		}
		else
			MessageBox(g_hwndParent, "Error: unable to write to file!", _title, MB_OK);
	}
	else
		MessageBox(g_hwndParent, "Error: action window not opened!", _title, MB_OK);
	return false;
}

// Create the Wiki ALR summary for the current section displayed in the "Action" dlg 
// This is the hack version, see clean but limited dumpWikiActionList() below
// http://forum.cockos.com/showthread.php?t=61929
// http://wiki.cockos.com/wiki/index.php/Action_List_Reference
void DumpWikiActionList2(COMMAND_T* _ct)
{
	dumpActionList(
		(int)_ct->user, 
		"Save ALR Wiki summary", 
		"|-\n| [[%s_%s|%s]] || %s\n",
		"{| class=\"wikitable\"\n|-\n! Action name !! Cmd ID\n",
		"|}\n");
}

void DumpActionList(COMMAND_T* _ct) {
	dumpActionList((int)_ct->user, "Dump action list", "%s\t%s\t%s\n", "Section\tId\tAction\n", NULL);
}


///////////////////////////////////////////////////////////////////////////////
// tests, other..
///////////////////////////////////////////////////////////////////////////////

#ifdef _SNM_MISC
void ShowTakeEnvPadreTest(COMMAND_T* _ct)
{
	bool updated = false;
	for (int i = 1; i <= GetNumTracks(); i++) // skip master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
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
void dumpWikiActionList(COMMAND_T* _ct)
{
	char filename[BUFFER_SIZE], cPath[BUFFER_SIZE];
	lstrcpyn(cPath, GetExePath(), BUFFER_SIZE);
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


