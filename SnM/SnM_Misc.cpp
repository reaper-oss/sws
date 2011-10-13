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
#include "SNM_Chunk.h"


///////////////////////////////////////////////////////////////////////////////
// File util
// JFB!!! TODO: WDL_UTF8, v4's exist_fn, ..
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

//JFB!!! crappy code for OSX compatibility..
bool SNM_CopyFile(const char* _destFn, const char* _srcFn)
{
	if (_destFn && _srcFn)
	{
		WDL_String chunk;
		if (LoadChunk(_srcFn, &chunk, false) && chunk.GetLength())
			return SaveChunk(_destFn, &chunk, false);
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
// - work with non existing files
// - nop for non resource paths (c:\temp\test.RfxChain -> c:\temp\test.RfxChain)
// - nop for full resource paths 
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

bool LoadChunk(const char* _fn, WDL_String* _chunk, bool _trim, int _maxlen)
{
	if (_chunk)
	{
		_chunk->Set("");
		if (_fn && *_fn)
		{
			if (FILE* f = fopenUTF8(_fn, "r"))
			{
				char str[SNM_MAX_CHUNK_LINE_LENGTH];
				while(fgets(str, SNM_MAX_CHUNK_LINE_LENGTH, f))
				{
					if (_maxlen && (int)(_chunk->GetLength()+strlen(str)) > _maxlen)
						break;

					if (_trim) // left trim + remove empty lines
					{
						char* p = str;
						while(*p && (*p == ' ' || *p == '\t')) p++;
						if (*p != '\n') // !*p managed in Append()
							_chunk->Append(p, SNM_MAX_CHUNK_LINE_LENGTH);
					}
					else
						_chunk->Append(str, SNM_MAX_CHUNK_LINE_LENGTH);
				}
				fclose(f);
				return true;
			}
		}
	}
	return false;
}

bool SaveChunk(const char* _fn, WDL_String* _chunk, bool _indent)
{
	if (_fn && *_fn && _chunk)
	{
		SNM_ChunkIndenter p(_chunk, false); // nothing done yet
		if (_indent) p.Indent();
		if (FILE* f = fopenUTF8(_fn, "w"))
		{
			fputs(p.GetUpdates() ? p.GetChunk()->Get() : _chunk->Get(), f); // avoid parser commit: faster
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
		// see http://code.google.com/p/sws-extension/issues/detail?id=358
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
/*JFB note: doing that leads to something close to the issue 292 (ini file cache odd pb)
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
		char* p = strchr(_out->Get(), '%');
		while(p)
		{
			int pos = p - _out->Get();
			_out->Insert("%", ++pos); // ++pos! but Insert() clamps to length..
			p = (pos+1 < _out->GetLength()) ? strchr((char*)(_out->Get()+pos+1), '%') : NULL;
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
		if (char* p = strchr(_str, '%')) {
			p[0] = _replaceCh;
			if (char* p2 = strchr((char*)(p+1), ' '))
				memmove((char*)(p+1), p2, strlen(p2)+1);
			else
				p[1] = '\0'; //assumes there's another char just after '%'
		}
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

bool WaitForTrackMute(DWORD* _muteTime)
{
	if (_muteTime && *_muteTime) {
		unsigned int fade = int(*(int*)GetConfigVar("mutefadems10") / 10 + 0.5);
		while ((GetTickCount() - *_muteTime) < fade)
		{
#ifdef _WIN32
			// keep the UI updating
			MSG msg;
			while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
				DispatchMessage(&msg);
#endif
			Sleep(1);
		}
		*_muteTime = 0;
		return true;
	}
	return false;
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
//	if ((GetTickCount() - waitTime) > _minReTrigger)
	{
		waitTime = GetTickCount();
		while((GetTickCount() - waitTime) < _timeOut) // for safety
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

void OpenStuff(COMMAND_T* _ct) {}

void TestWDLString(COMMAND_T*) {}

#endif

int TestReport(bool _ok, int _tstId, char* _report)
{
	static DWORD t = GetTickCount();
	int t2=0;
	if (_tstId)
	{
		t2 = GetTickCount() - t;
		char reportline[64] = "";
		_snprintf(reportline, 64, "Test %02d\t%d ms\t%s\n", _tstId, t2, _ok ? "ok" : "FAILED!");
		strcat(_report, reportline);
	}
	t = GetTickCount();
	return t2;
}

void TestWDLString(COMMAND_T*)
{
	char report[4096] = ""; 
	int tst=0, t=0;
	DWORD t2 = 0;


	WDL_String massiveChunk,massiveChunk2,massiveChunk3; // 2 & 3 will get altered
	t2 = GetTickCount();
	LoadChunk("C:\\MASSIVE_CHUNK.RPP", &massiveChunk); // this will be looooong with slow WDL_Stirng..
	{
		char reportline[64] = "";
		_snprintf(reportline, 64, "pre-Test chunk loaded in %1.3f s (%d bytes)\n", (float)((GetTickCount()-t2)/1000), massiveChunk.GetLength());
		strcat(report, reportline);
	}

	massiveChunk3.Set(&massiveChunk);

	TestReport(false, 0, NULL); //reinit

	WDL_String s;

	//////////////////////////////////////////////////////////////////////

	// Set
	strcat(report, "\nSet:\n");

	/*for (int i=0; i < 50; i++)*/ massiveChunk2.Set(massiveChunk.Get());
	t += TestReport(massiveChunk.GetLength() == massiveChunk2.GetLength(), ++tst, report);

	for (int i=0; i < 1000000; i++) s.Set("1234567890");
	t += TestReport(s.GetLength() == 10 && !strcmp(s.Get(), "1234567890"), ++tst, report);

	for (int i=0; i < 1000000; i++) s.Set("1234567890", 5);
	t += TestReport(s.GetLength() == 5 && !strcmp(s.Get(), "12345"), ++tst, report);
	for (int i=0; i < 1000000; i++) s.Set("");
	t += TestReport(s.GetLength() == 0 && s.Get()[0] == 0, ++tst, report);
	for (int i=0; i < 1000000; i++) s.Set("1234567890", 255);
	t += TestReport(s.GetLength() == 10 && !strcmp(s.Get(), "1234567890"), ++tst, report);
	{ // unalloc tst
		WDL_String s2;
		t += TestReport(!s2.GetLength() && s2.Get()[0] == 0, ++tst, report);
	}


	// Append
	strcat(report, "\nAppend:\n");

	s.Set("123");
	s.Append("4567890");
	t += TestReport(s.GetLength() == 10 && !strcmp(s.Get(), "1234567890"), ++tst, report);
	s.Append("1234567890", 3);
	t += TestReport(s.GetLength() == 13 && !strcmp(s.Get(), "1234567890123"), ++tst, report);
	s.Append("", 3);
	t += TestReport(s.GetLength() == 13 && !strcmp(s.Get(), "1234567890123"), ++tst, report);
	s.Append("4567890", 255);
	t += TestReport(s.GetLength() == 20 && !strcmp(s.Get(), "12345678901234567890"), ++tst, report);
	{ // unalloc tst
		WDL_String s2;
		s2.Append("1234567890", 5);
		t += TestReport(s2.GetLength() == 5 && !strcmp(s2.Get(), "12345"), ++tst, report);
	}
	// speed tests
	s.Set("");
	t2 = GetTickCount();
	for (int i=0; (GetTickCount()-t2) < 10000 && i < 1000000; i++)
		s.Append("1234567890");
	t += TestReport((GetTickCount()-t2) < 10000 && s.GetLength() == 10000000, ++tst, report);

	s.Set("");
	char* pEOL = massiveChunk.Get()-1;
	t2 = GetTickCount();
	while((GetTickCount()-t2) < 10000) {
		char* pLine = pEOL+1;
		pEOL = strchr(pLine, '\n');
		if (!pEOL) break;
		else if (*pLine) s.Append(pLine, (int)(pEOL-pLine+1));
	}
	t += TestReport((GetTickCount()-t2) < 10000 && s.GetLength() == massiveChunk.GetLength(), ++tst, report);


	// DeleteSub
	strcat(report, "\nDeleteSub:\n");

	s.Set("12345678901234567890");
	s.DeleteSub(255, 2);
	t += TestReport(s.GetLength() == 20 && !strcmp(s.Get(), "12345678901234567890"), ++tst, report);
	s.DeleteSub(10, 255);
	t += TestReport(s.GetLength() == 10 && !strcmp(s.Get(), "1234567890"), ++tst, report);
	s.DeleteSub(5, 2);
	t += TestReport(s.GetLength() == 8 && !strcmp(s.Get(), "12345890"), ++tst, report);
	s.DeleteSub(0, s.GetLength()*2);
	t += TestReport(!s.GetLength() && s.Get()[0] == 0, ++tst, report);
	{ // unalloc tst
		WDL_String s2;
		s2.DeleteSub(0, 5);
		t += TestReport(!s2.GetLength() && s2.Get()[0] == 0, ++tst, report);
	}
	// speed test
	t2 = GetTickCount();
	while((GetTickCount()-t2) < 10000 && massiveChunk2.GetLength())
		massiveChunk2.DeleteSub(0, 20000); // 20000 chars so that we don't get fooled with slow memmove..
	t += TestReport((GetTickCount()-t2) < 10000 && !massiveChunk2.GetLength(), ++tst, report);


	// Insert
	strcat(report, "\nInsert:\n");

	s.Set("1234567890");
	s.Insert("4567890", 255, 3);
	t += TestReport(s.GetLength() == 13 && !strcmp(s.Get(), "1234567890456"), ++tst, report);
	s.Insert("1234567890", 10, 3);
	t += TestReport(s.GetLength() == 16 && !strcmp(s.Get(), "1234567890123456"), ++tst, report);
	s.Insert("7890", 255);
	t += TestReport(s.GetLength() == 20 && !strcmp(s.Get(), "12345678901234567890"), ++tst, report);
	s.Insert("", 2);
	t += TestReport(s.GetLength() == 20 && !strcmp(s.Get(), "12345678901234567890"), ++tst, report);
	s.Insert("1234567890123", 0, 10);
	t += TestReport(s.GetLength() == 30 && !strcmp(s.Get(), "123456789012345678901234567890"), ++tst, report);
	{ // unalloc tst
		WDL_String s2;
		s2.Insert("1234567890", 5, 5);
		t += TestReport(s2.GetLength() == 5 && !strcmp(s2.Get(), "12345"), ++tst, report);
	}
	// speed tests
	s.Set("");
	t2 = GetTickCount();
	for (int i=0; (GetTickCount()-t2) < 10000 && i < 1000000; i++)
		s.Insert("1234567890", !s.GetLength() ? 0 : s.GetLength()-9);
	t += TestReport((GetTickCount()-t2) < 10000 && s.GetLength() == 10000000, ++tst, report);

	s.Set("");
	pEOL = massiveChunk.Get()-1;
	t2 = GetTickCount();
	while((GetTickCount()-t2) < 10000) {
		char* pLine = pEOL+1;
		pEOL = strchr(pLine, '\n');
		if (!pEOL) break;
		else if (*pLine) s.Insert(pLine, max(0, s.GetLength()-50), (int)(pEOL-pLine+1)); //makes no sense, just for test..
	}
	t += TestReport((GetTickCount()-t2) < 10000 && s.GetLength() == massiveChunk.GetLength(), ++tst, report);


	// SetLen
	strcat(report, "\nSetLen:\n");

	s.Set("1234567890");
	for (int i=0; i < 1000000; i++) s.SetLen(20);
	t += TestReport(s.GetLength() == 10 && !strcmp(s.Get(), "1234567890"), ++tst, report);
	for (int i=0; i < 1000000; i++) s.SetLen(5);
	t += TestReport(s.GetLength() == 5 && !strcmp(s.Get(), "12345"), ++tst, report);
	for (int i=0; i < 1000000; i++) s.SetLen(0);
	t += TestReport(!s.GetLength() && s.Get()[0] == 0, ++tst, report);
	{ // unalloc tst
		WDL_String s2;
		s2.SetLen(5);
		t += TestReport(s2.GetLength() == 5, ++tst, report); // no test on content: garbage
	}


	// SetFormatted
	strcat(report, "\nSetFormatted:\n");

	for (int i=0; i < 1000000; i++) s.SetFormatted(10, "%s%d", "12345", 67890);
	t += TestReport(s.GetLength() == 10 && !strcmp(s.Get(), "1234567890"), ++tst, report);
	for (int i=0; i < 1000000; i++) s.SetFormatted(5, "%s", "1234567890");
	t += TestReport(!s.GetLength() && s.Get()[0] == 0, ++tst, report); // hum.. would be better to strip down? (but taht would be a functionnal change!)
	for (int i=0; i < 1000000; i++) s.SetFormatted(20, "%s", "1234567890");
	t += TestReport(s.GetLength() == 10 && !strcmp(s.Get(), "1234567890"), ++tst, report);
	{ // unalloc tst
		WDL_String s2;
		s2.SetFormatted(10, "%s", "1234567890");
		t += TestReport(s2.GetLength() == 10 && !strcmp(s2.Get(), "1234567890"), ++tst, report);
	}
	// no speed test: would suck hard (we'd need our own vsnprintf code..)


	// AppendFormatted
	strcat(report, "\nAppendFormatted:\n");

	s.Set("1234567890");
	s.AppendFormatted(10, "%s%d", "12345", 67890);
	t += TestReport(s.GetLength() == 20 && !strcmp(s.Get(), "12345678901234567890"), ++tst, report);
	s.AppendFormatted(5, "%s", "1234567890");
	t += TestReport(s.GetLength() == 20 && !strcmp(s.Get(), "12345678901234567890"), ++tst, report); // hum.. would be better to strip down? (but taht would be a functionnal change!)
	{ // unalloc tst
		WDL_String s2;
		s2.AppendFormatted(10, "%s", "1234567890");
		t += TestReport(s2.GetLength() == 10 && !strcmp(s2.Get(), "1234567890"), ++tst, report);
	}
	// speed test
	s.Set("");
	pEOL = massiveChunk3.Get()-1;
	t2 = GetTickCount();
	while((GetTickCount()-t2) < 10000) {
		char* pLine = pEOL+1;
		pEOL = strchr(pLine, '\n');
		if (!pEOL) break;
		else if (*pLine) {
			*pEOL = 0;
			s.AppendFormatted((int)(pEOL-pLine+1), "%s\n", pLine);
		}
	}
	t += TestReport((GetTickCount()-t2) < 10000 && s.GetLength() == massiveChunk.GetLength(), ++tst, report);


	// Ellipsize
	strcat(report, "\nEllipsize:\n");

	s.Set("1234567890");
	s.Ellipsize(255, 6);
	t += TestReport(s.GetLength() == 5 && !strcmp(s.Get(), "12..."), ++tst, report);
	s.Set("1234567890");
	s.Ellipsize(255, 5);
	t += TestReport(s.GetLength() == 4 && !strcmp(s.Get(), "1..."), ++tst, report);
	s.Set("1234567890");
	s.Ellipsize(5, 255);
	t += TestReport(s.GetLength() == 10 && !strcmp(s.Get(), "1234567890"), ++tst, report);
	s.Set("123");
	s.Ellipsize(5, 255);
	t += TestReport(s.GetLength() == 3 && !strcmp(s.Get(), "123"), ++tst, report);
	s.Set("123");
	s.Ellipsize(5, 2);
	t += TestReport(s.GetLength() == 3 && !strcmp(s.Get(), "123"), ++tst, report);
	s.Set("1");
	s.Ellipsize(1, 2);
	t += TestReport(s.GetLength() == 1 && !strcmp(s.Get(), "1"), ++tst, report);
	{ // unalloc tst
		WDL_String s2;
		s2.Ellipsize(255, 6);
		t += TestReport(!s2.GetLength() && s2.Get()[0] == 0, ++tst, report);
	}
	
	//////////////////////////////////////////////////////////////////////

	WDL_String tstStr("1234567890");
	WDL_String tstStr4567890("4567890");
	WDL_String tstStr7890("7890");
	WDL_String tstStr1234567890123("1234567890123");
	WDL_String tstEmptyStr;

	// Set2
	strcat(report, "\nSet2:\n");
	
	WDL_String massiveChunk4;
	/*for (int i=0; i < 50; i++)*/ massiveChunk4.Set(&massiveChunk);
	t += TestReport(massiveChunk.GetLength() == massiveChunk4.GetLength(), ++tst, report);

	for (int i=0; i < 1000000; i++) s.Set(&tstStr);
	t += TestReport(s.GetLength() == 10 && !strcmp(s.Get(), "1234567890"), ++tst, report);

	for (int i=0; i < 1000000; i++) s.Set(&tstStr, 5);
	t += TestReport(s.GetLength() == 5 && !strcmp(s.Get(), "12345"), ++tst, report);
	for (int i=0; i < 1000000; i++) s.Set(&tstEmptyStr);
	t += TestReport(s.GetLength() == 0 && s.Get()[0] == 0, ++tst, report);
	for (int i=0; i < 1000000; i++) s.Set(&tstStr, 255);
	t += TestReport(s.GetLength() == 10 && !strcmp(s.Get(), "1234567890"), ++tst, report);
	{ // unalloc tst
		WDL_String s1;
		WDL_String s2(s1);
		t += TestReport(!s2.GetLength() && s2.Get()[0] == 0, ++tst, report);
	}


	// Append2
	strcat(report, "\nAppend2:\n");

	s.Set("123");
	s.Append(&tstStr4567890);
	t += TestReport(s.GetLength() == 10 && !strcmp(s.Get(), "1234567890"), ++tst, report);
	s.Append(&tstStr, 3);
	t += TestReport(s.GetLength() == 13 && !strcmp(s.Get(), "1234567890123"), ++tst, report);
	s.Append(&tstEmptyStr, 3);
	t += TestReport(s.GetLength() == 13 && !strcmp(s.Get(), "1234567890123"), ++tst, report);
	s.Append(&tstStr4567890, 255);
	t += TestReport(s.GetLength() == 20 && !strcmp(s.Get(), "12345678901234567890"), ++tst, report);
	{ // unalloc tst
		WDL_String s2;
		s2.Append(&tstStr, 5);
		t += TestReport(s2.GetLength() == 5 && !strcmp(s2.Get(), "12345"), ++tst, report);
	}
	// speed test
	s.Set("");
	t2 = GetTickCount();
	for (int i=0; (GetTickCount()-t2) < 10000 && i < 1000000; i++)
		s.Append(&tstStr);
	t += TestReport((GetTickCount()-t2) < 10000 && s.GetLength() == 10000000, ++tst, report);


	// Insert2
	strcat(report, "\nInsert2:\n");

	s.Set("1234567890");
	s.Insert(&tstStr4567890, 255, 3);
	t += TestReport(s.GetLength() == 13 && !strcmp(s.Get(), "1234567890456"), ++tst, report);
	s.Insert(&tstStr, 10, 3);
	t += TestReport(s.GetLength() == 16 && !strcmp(s.Get(), "1234567890123456"), ++tst, report);
	s.Insert(&tstStr7890, 255);
	t += TestReport(s.GetLength() == 20 && !strcmp(s.Get(), "12345678901234567890"), ++tst, report);
	s.Insert(&tstEmptyStr, 2);
	t += TestReport(s.GetLength() == 20 && !strcmp(s.Get(), "12345678901234567890"), ++tst, report);
	s.Insert(&tstStr1234567890123, 0, 10);
	t += TestReport(s.GetLength() == 30 && !strcmp(s.Get(), "123456789012345678901234567890"), ++tst, report);
	{ // unalloc tst
		WDL_String s2;
		s2.Insert(&tstStr, 5, 5);
		t += TestReport(s2.GetLength() == 5 && !strcmp(s2.Get(), "12345"), ++tst, report);
	}
	// speed tests
	s.Set("");
	t2 = GetTickCount();
	for (int i=0; (GetTickCount()-t2) < 10000 && i < 1000000; i++)
		s.Insert(&tstStr, !s.GetLength() ? 0 : s.GetLength()-9);
	t += TestReport((GetTickCount()-t2) < 10000 && s.GetLength() == 10000000, ++tst, report);



	//////////////////////////////////////////////////////////////////////

	char totalTime[64];
	_snprintf(totalTime, 64, "TOTAL TIME: %1.3f sec\n", (float)t/1000);

	char report2[4096] = ""; 
	strcat(report2, totalTime);
	strcat(report2, report);
	ShowConsoleMsg(report2);
}

