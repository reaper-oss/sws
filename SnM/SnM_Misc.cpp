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
#include "SNM_FXChainView.h"
#include "../../WDL/projectcontext.h"


///////////////////////////////////////////////////////////////////////////////
// File util
// JFB!!! TODO: WDL_UTF8, exist_fn, ..
///////////////////////////////////////////////////////////////////////////////

const char* GetFileExtension(const char* _fn)
{
	if (const char* ext = strrchr(_fn, '.'))
		return ext+1;
	return "";
}

void GetFilenameNoExt(const char* _fullFn, char* _fn, int _fnSz)
{
	const char* p = strrchr(_fullFn, PATH_SLASH_CHAR);
	if (p) p++;
	else p = _fullFn;
	lstrcpyn(_fn, p, _fnSz);
	_fn = strrchr(_fn, '.');
	if (_fn) *_fn = '\0';
}

const char* GetFilenameWithExt(const char* _fullFn)
{
	const char* p = strrchr(_fullFn, PATH_SLASH_CHAR);
	if (p) p++;
	else p = _fullFn;
	return p;
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
			MessageBox(GetMainHwnd(), buf, "S&M - Error", MB_OK);
		}
	}
	return exists;
}

bool SNM_DeleteFile(const char* _filename, bool _recycleBin)
{
	bool ok = false;
	if (_filename && *_filename) 
	{
#ifdef _WIN32
		if (_recycleBin)
			return (SendFileToRecycleBin(_filename) ? false : true); // avoid warning warning C4800
#endif
		ok = (DeleteFile(_filename) ? true : false); // avoid warning warning C4800
	}
	return ok;
}

bool SNM_CopyFile(const char* _destFn, const char* _srcFn)
{
	bool ok = false;
	if (_destFn && _srcFn) {
		if (WDL_HeapBuf* hb = LoadBin(_srcFn)) {
			ok = SaveBin(_destFn, hb);
			DELETE_NULL(hb);
		}
	}
	return ok;
}

// Browse + return short resource filename (if possible and if _wantFullPath == false)
// Returns false if cancelled
bool BrowseResourcePath(const char* _title, const char* _resSubDir, const char* _fileFilters, char* _fn, int _fnSize, bool _wantFullPath)
{
	bool ok = false;
	char defaultPath[BUFFER_SIZE] = "";
	_snprintf(defaultPath, BUFFER_SIZE, "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, _resSubDir);

	if (char* filename = BrowseForFiles(_title, defaultPath, NULL, false, _fileFilters)) 
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

			char resFn[BUFFER_SIZE] = "", resDir[BUFFER_SIZE];
			_snprintf(resFn, BUFFER_SIZE, "%s%c%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, _resSubDir, PATH_SLASH_CHAR, _shortFn);
			lstrcpyn(resDir, resFn, BUFFER_SIZE);
			if (char* p = strrchr(resDir, PATH_SLASH_CHAR)) *p = '\0';
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

bool LoadChunk(const char* _fn, WDL_FastString* _chunk, bool _trim, int _maxlen)
{
	if (!_chunk)
		return false;

	_chunk->Set("");
	if (_fn && *_fn)
	{
		if (FILE* f = fopenUTF8(_fn, "r"))
		{
			char str[SNM_MAX_CHUNK_LINE_LENGTH];
			while(fgets(str, SNM_MAX_CHUNK_LINE_LENGTH, f) && *str)
			{
				if (_maxlen && (int)(_chunk->GetLength()+strlen(str)) > _maxlen)
					break;

				if (_trim) // left trim + remove empty lines
				{
					char* p = str;
					while(*p && (*p == ' ' || *p == '\t')) p++;
					if (*p != '\n') // the !*p case is managed in Append()
						_chunk->Append(p);
				}
				else
					_chunk->Append(str);
			}
			fclose(f);
			return true;
		}
	}
	return false;
}

bool SaveChunk(const char* _fn, WDL_FastString* _chunk, bool _indent)
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

// returns NULL if failed, otherwise it's up to the caller to free the returned buffer
WDL_HeapBuf* LoadBin(const char* _fn)
{
  WDL_HeapBuf* hb = NULL;
  if (FILE* f = fopenUTF8(_fn , "rb"))
  {
	long lSize;
	fseek(f, 0, SEEK_END);
	lSize = ftell(f);
	rewind(f);

	hb = new WDL_HeapBuf();
	void* p = hb->Resize(lSize,false);
	if (p && hb->GetSize() == lSize) {
		size_t result = fread(p,1,lSize,f);
		if (result != lSize)
			DELETE_NULL(hb);
	}
	else
		DELETE_NULL(hb);
	fclose(f);
  }
  return hb;
}

bool SaveBin(const char* _fn, const WDL_HeapBuf* _hb)
{
	bool ok = false;
	if (_hb && _hb->GetSize())
		if (FILE* f = fopenUTF8(_fn , "wb")) {
			ok = (fwrite(_hb->Get(), 1, _hb->GetSize(), f) == _hb->GetSize());
			fclose(f);
		}
	return ok;
}

bool TranscodeFileToFile64(const char* _outFn, const char* _inFn)
{
	bool ok = false;
	WDL_HeapBuf* hb = LoadBin(_inFn); 
	if (hb && hb->GetSize())
	{
		ProjectStateContext* ctx = ProjectCreateFileWrite(_outFn);
		cfg_encode_binary(ctx, hb->Get(), hb->GetSize());
		delete ctx;
		ok = FileExistsErrMsg(_outFn, false);
	}
	delete hb;
	return ok;
}

// returns NULL if failed, otherwise it's up to the caller to free the returned buffer
WDL_HeapBuf* TranscodeStr64ToHeapBuf(const char* _str64)
{
	WDL_HeapBuf* hb = NULL;
	long len = strlen(_str64);
	WDL_HeapBuf hbTmp;
	void* p = hbTmp.Resize(len, false);
	if (p && hbTmp.GetSize() == len)
	{
		memcpy(p, _str64, len);
		ProjectStateContext* ctx = ProjectCreateMemCtx(&hbTmp);
		hb = new WDL_HeapBuf();
		cfg_decode_binary(ctx, hb);
		delete ctx;
	}
	return hb;
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

void StringToExtensionConfig(WDL_FastString* _str, ProjectStateContext* _ctx)
{
	if (_str && _ctx)
	{
		// see http://code.google.com/p/sws-extension/issues/detail?id=358
		WDL_FastString unformatedStr;
		makeUnformatedConfigString(_str->Get(), &unformatedStr);

		const char* pEOL = unformatedStr.Get()-1;
		char curLine[SNM_MAX_CHUNK_LINE_LENGTH] = "";
		for(;;) 
		{
			const char* pLine = pEOL+1;
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

void ExtensionConfigToString(WDL_FastString* _str, ProjectStateContext* _ctx)
{
	if (_str && _ctx)
	{
		LineParser lp(false);
		char linebuf[SNM_MAX_CHUNK_LINE_LENGTH] = "";
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

// write a full ini file section in one go
// "the data in the buffer pointed to by the lpString parameter consists 
// of one or more null-terminated strings, followed by a final null character"
void SaveIniSection(const char* _iniSectionName, WDL_FastString* _iniSection, const char* _iniFn)
{
	if (_iniSectionName && _iniSection && _iniFn)
	{
		if (char* buf = (char*)calloc(_iniSection->GetLength()+1, sizeof(char)))
		{
			memcpy(buf, _iniSection->Get(), _iniSection->GetLength());
			for (int j=0; j < _iniSection->GetLength(); j++)
				if (buf[j] == '\n') 
					buf[j] = '\0';
			WritePrivateProfileStruct(_iniSectionName, NULL, NULL, 0, _iniFn); // flush section
			WritePrivateProfileSection(_iniSectionName, buf, _iniFn);
			free(buf);
		}
	}
}

void UpdatePrivateProfileSection(const char* _oldAppName, const char* _newAppName, const char* _iniFn, const char* _newIniFn)
{
	char buf[SNM_MAX_INI_SECTION]="";
	int sectionSz = GetPrivateProfileSection(_oldAppName, buf, SNM_MAX_INI_SECTION, _iniFn);
	WritePrivateProfileStruct(_oldAppName, NULL, NULL, 0, _iniFn); // flush section
	if (sectionSz)
		WritePrivateProfileSection(_newAppName, buf, _newIniFn ? _newIniFn : _iniFn);
}

void UpdatePrivateProfileString(const char* _appName, const char* _oldKey, const char* _newKey, const char* _iniFn, const char* _newIniFn)
{
	char buf[BUFFER_SIZE]="";
	GetPrivateProfileString(_appName, _oldKey, "", buf, BUFFER_SIZE, _iniFn);
	WritePrivateProfileString(_appName, _oldKey, NULL, _iniFn); // remove key
	if (*buf)
		WritePrivateProfileString(_appName, _newKey, buf, _newIniFn ? _newIniFn : _iniFn);
}

void SNM_UpgradeIniFiles()
{
	if (g_iniFileVersion < 1) // i.e. sws version < v2.1.0 #18
	{
		// upgrade deprecated section names 
		UpdatePrivateProfileSection("FXCHAIN", "FXChains", g_SNMIniFn.Get());
		UpdatePrivateProfileSection("FXCHAIN_VIEW", "RESOURCE_VIEW", g_SNMIniFn.Get());
		// upgrade deprecated key names (automatically generated now..)
		UpdatePrivateProfileString("RESOURCE_VIEW", "DblClick_Type", "DblClickFXChains", g_SNMIniFn.Get());
		UpdatePrivateProfileString("RESOURCE_VIEW", "DblClick_Type_Tr_Template", "DblClickTrackTemplates", g_SNMIniFn.Get());
		UpdatePrivateProfileString("RESOURCE_VIEW", "DblClick_Type_Prj_Template", "DblClickProjectTemplates", g_SNMIniFn.Get());
		UpdatePrivateProfileString("RESOURCE_VIEW", "AutoSaveDirFXChain", "AutoSaveDirFXChains", g_SNMIniFn.Get());
		UpdatePrivateProfileString("RESOURCE_VIEW", "AutoFillDirFXChain", "AutoFillDirFXChains", g_SNMIniFn.Get());
		UpdatePrivateProfileString("RESOURCE_VIEW", "AutoSaveDirTrTemplate", "AutoSaveDirTrackTemplates", g_SNMIniFn.Get());
		UpdatePrivateProfileString("RESOURCE_VIEW", "AutoFillDirTrTemplate", "AutoFillDirTrackTemplates", g_SNMIniFn.Get());
		UpdatePrivateProfileString("RESOURCE_VIEW", "AutoSaveDirPrjTemplate", "AutoSaveDirProjectTemplates", g_SNMIniFn.Get());
		UpdatePrivateProfileString("RESOURCE_VIEW", "AutoFillDirPrjTemplate", "AutoFillDirProjectTemplates", g_SNMIniFn.Get());
	}
	if (g_iniFileVersion < 2) // i.e. sws version < v2.1.0 #21
	{
		// move cycle actions to a new dedicated file (+ make backup if it already exists)
		WDL_FastString fn;
		fn.SetFormatted(BUFFER_SIZE, SNM_CYCLACTION_BAK_FILE, GetResourcePath());
		if (FileExists(g_SNMCyclactionIniFn.Get()))
			MoveFile(g_SNMCyclactionIniFn.Get(), fn.Get());
		UpdatePrivateProfileSection("MAIN_CYCLACTIONS", "Main_Cyclactions", g_SNMIniFn.Get(), g_SNMCyclactionIniFn.Get());
		UpdatePrivateProfileSection("ME_LIST_CYCLACTIONS", "ME_List_Cyclactions", g_SNMIniFn.Get(), g_SNMCyclactionIniFn.Get());
		UpdatePrivateProfileSection("ME_PIANO_CYCLACTIONS", "ME_Piano_Cyclactions", g_SNMIniFn.Get(), g_SNMCyclactionIniFn.Get());
	}
	if (g_iniFileVersion < 3) // i.e. sws version < v2.1.0 #22
	{
		WritePrivateProfileString("RESOURCE_VIEW", "DblClick_To", NULL, g_SNMIniFn.Get()); // remove key
		UpdatePrivateProfileString("RESOURCE_VIEW", "FilterByPath", "Filter", g_SNMIniFn.Get());
	}
}

// fills a list of filenames for the desired extension
// if _ext == NULL or '\0', look for media files
// note: it is up to the caller to free _files (e.g. WDL_PtrList_DeleteOnDestroy)
void ScanFiles(WDL_PtrList<WDL_String>* _files, const char* _initDir, const char* _ext, bool _subdirs)
{
	WDL_DirScan ds;
	if (_files && _initDir && !ds.First(_initDir))
	{
		bool lookForMedias = (!_ext || !*_ext);
		do 
		{
			if (!strcmp(ds.GetCurrentFN(), ".") || !strcmp(ds.GetCurrentFN(), "..")) 
				continue;

			WDL_String* foundFn = new WDL_String();
			ds.GetCurrentFullFN(foundFn);

			if (ds.GetCurrentIsDirectory())
			{
				if (_subdirs) ScanFiles(_files, foundFn->Get(), _ext, true);
				delete foundFn;
			}
			else
			{
				const char* ext = GetFileExtension(foundFn->Get());
				if ((lookForMedias && IsMediaExtension(ext, false)) || (!lookForMedias && !_stricmp(ext, _ext)))
					_files->Add(foundFn);
				else
					delete foundFn;
			}
		}
		while(!ds.Next());
	}
}

///////////////////////////////////////////////////////////////////////////////
// Other helpers
///////////////////////////////////////////////////////////////////////////////

// returns the 1st marker or region index found at _pos
// note: relies on markers & regions indexed by positions
int FindMarkerRegion(double _pos, int* _idOut)
{
	if (_idOut)
		*_idOut = -1;

	int x=0, idx=-1, num=-1; double dPos, dEnd; bool isRgn;
	while (x = EnumProjectMarkers2(NULL, x, &isRgn, &dPos, &dEnd, NULL, &num))
	{
		if (_pos >= dPos && (!isRgn || (isRgn && _pos <= dEnd)))
		{
			//JFB TODO? exclude ended regions?
			if (EnumProjectMarkers2(NULL, x, NULL, &dPos, NULL, NULL, NULL))
			{
				if (_pos < dPos) {
					idx = x-1;
					break;
				}
			}
			else {
				idx = x-1;
				break;
			}
		}
	}
	if (idx >= 0 && _idOut)
		*_idOut = MakeMarkerRegionId(num, isRgn);
	return idx;
}

// API LIMITATION: no way to identify a marker (or a region)
// => we build an id in "best effort" mode (like region|num)
// => we would need something like GUIDs for regions & markers..
int MakeMarkerRegionId(int _markrgnindexnumber, bool _isRgn)
{
	// note: MSB is ignored so that the encoded number is always positive
	if (_markrgnindexnumber >= 0 && _markrgnindexnumber <= 0x3FFFFFFF) {
		_markrgnindexnumber |= ((_isRgn?1:0) << 30);
		return _markrgnindexnumber;
	}
	return -1;
}

int GetMarkerRegionIdFromIndex(int _idx)
{
	if (_idx >= 0)
	{
		int num; bool isRgn;
		if (EnumProjectMarkers2(NULL, _idx, &isRgn, NULL, NULL, NULL, &num))
			return MakeMarkerRegionId(num, isRgn);
	}
	return -1;
}

// see MakeMarkerRegionId()'s comments
int GetMarkerRegionIndexFromId(int _id)
{
	if (_id > 0)
	{
		int num = _id & 0x3FFFFFFF;
		bool isRgn = IsRegion(_id);
		int x=0, num2; bool isRgn2;
		while (x = EnumProjectMarkers2(NULL, x, &isRgn2, NULL, NULL, NULL, &num2))
			if (num == num2 && isRgn == isRgn2)
				return x-1;
	}
	return -1;
}

bool IsRegion(int _id) {
	return (_id > 0 && (_id&0x40000000) != 0);
}

void TranslatePos(double _pos, int* _h, int* _m, int* _s, int* _ms)
{
	if (_h) *_h = int(_pos/3600);
	if (_m) *_m = int((_pos - 3600*int(_pos/3600)) / 60);
	if (_s) *_s = int(_pos - 3600*int(_pos/3600) - 60*int((_pos-3600*int(_pos/3600))/60));
	if (_ms) *_ms = int(1000*(_pos-int(_pos)) + 0.5); // rounding
}

void makeUnformatedConfigString(const char* _in, WDL_FastString* _out)
{
	if (_in && _out)
	{
		_out->Set(_in);
		const char* p = strchr(_out->Get(), '%');
		while(p)
		{
			int pos = p - _out->Get();
			_out->Insert("%", ++pos); // ++pos! but Insert() clamps to length..
			p = (pos+1 < _out->GetLength()) ? strchr((_out->Get()+pos+1), '%') : NULL;
		}
	}
}

bool GetStringWithRN(const char* _bufIn, char* _bufOut, int _bufOutSz)
{
	if (!_bufOut || !_bufIn)
		return false;

	int i=0, j=0;
	while (_bufIn[i] && j < _bufOutSz)
	{
		if (_bufIn[i] == '\n') {
			_bufOut[j++] = '\r';
			_bufOut[j++] = '\n';
		}
		else if (_bufIn[i] != '\r')
			_bufOut[j++] = _bufIn[i];
		i++;
	}

	if (j < _bufOutSz)
		_bufOut[j] = '\0';
	else
		_bufOut[_bufOutSz-1] = '\0'; // truncates, best effort..

	return (j < _bufOutSz);
}

void ShortenStringToFirstRN(char* _str) {
	if (_str) {
		char* p = strchr(_str, '\r');
		if (p) *p = '\0';
		p = strchr(_str, '\n');
		if (p) *p = '\0';
	}
}

// replace "%blabla " with "_replaceCh " in _str
void ReplaceStringFormat(char* _str, char _replaceCh) {
	if (_str && *_str)
		if (char* p = strchr(_str, '%')) {
			if (p[1]) {
				p[0] = _replaceCh;
				if (char* p2 = strchr((char*)(p+1), ' ')) memmove((char*)(p+1), p2, strlen(p2)+1);
				else p[1] = '\0';
			}
			else p[0] = '\0';
		}
}

bool GetSectionNameAsURL(bool _alr, const char* _section, char* _sectionURL, int _sectionURLSize)
{
	if (!_section || !_sectionURL)
		return false;

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
		else
			return false;
	}
	else
		lstrcpyn(_sectionURL, _section, _sectionURLSize);
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
// Theme slots (Resources view)
///////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
void LoadThemeSlot(int _slotType, const char* _title, int _slot)
{
	if (WDL_FastString* fnStr = g_slots.Get(_slotType)->GetOrPromptOrBrowseSlot(_title, _slot))
	{
		char cmd[BUFFER_SIZE]=""; _snprintf(cmd, BUFFER_SIZE, "%s\\reaper.exe", GetExePath());
		WDL_FastString fnStr2; fnStr2.SetFormatted(BUFFER_SIZE, " \"%s\"", fnStr->Get());
		_spawnl(_P_NOWAIT, cmd, fnStr2.Get(), NULL);
		delete fnStr;
	}
}

void LoadThemeSlot(COMMAND_T* _ct) {
	LoadThemeSlot(g_tiedSlotActions[SNM_SLOT_THM], SNM_CMD_SHORTNAME(_ct), (int)_ct->user);
}
#endif


///////////////////////////////////////////////////////////////////////////////
// Image slots (Resources view)
///////////////////////////////////////////////////////////////////////////////

void ShowImageSlot(int _slotType, const char* _title, int _slot) {
	if (WDL_FastString* fnStr = g_slots.Get(_slotType)->GetOrPromptOrBrowseSlot(_title, _slot)) {
		OpenImageView(fnStr->Get());
		delete fnStr;
	}
}

void ShowImageSlot(COMMAND_T* _ct) {
	ShowImageSlot(g_tiedSlotActions[SNM_SLOT_IMG], SNM_CMD_SHORTNAME(_ct), (int)_ct->user);
}

void SetSelTrackIconSlot(int _slotType, const char* _title, int _slot) {
	if (WDL_FastString* fnStr = g_slots.Get(_slotType)->GetOrPromptOrBrowseSlot(_title, _slot)) {
		SetSelTrackIcon(fnStr->Get());
		delete fnStr;
	}
}

void SetSelTrackIconSlot(COMMAND_T* _ct) {
	SetSelTrackIconSlot(g_tiedSlotActions[SNM_SLOT_IMG], SNM_CMD_SHORTNAME(_ct), (int)_ct->user);
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
		if (!GetSectionNameAsURL(_type == 1 || _type == 2, currentSection, sectionURL, SNM_MAX_SECTION_NAME_LEN))
		{
			MessageBox(GetMainHwnd(), "Error: unknown section!", _title, MB_OK);
			return false;
		}

		char name[SNM_MAX_SECTION_NAME_LEN*2]; char filename[BUFFER_SIZE];
		_snprintf(name, SNM_MAX_SECTION_NAME_LEN*2, "%s%s.txt", sectionURL, !(_type % 2) ? "_SWS" : "");
		if (!BrowseForSaveFile(_title, GetResourcePath(), name, "Text files (*.txt)\0*.txt\0All files (*.*)\0*.*\0", filename, BUFFER_SIZE))
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
			MessageBox(GetMainHwnd(), msg, _title, MB_OK);
			return true;

		}
		else
			MessageBox(GetMainHwnd(), "Error: unable to write to file!", _title, MB_OK);
	}
	else
		MessageBox(GetMainHwnd(), "Error: action window not opened!", _title, MB_OK);
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
		"S&M - Save ALR Wiki summary", 
		"|-\n| [[%s_%s|%s]] || %s\n",
		"{| class=\"wikitable\"\n|-\n! Action name !! Cmd ID\n",
		"|}\n");
}

void DumpActionList(COMMAND_T* _ct) {
	dumpActionList((int)_ct->user, "S&M - Dump action list", "%s\t%s\t%s\n", "Section\tId\tAction\n", NULL);
}


#ifdef _SNM_MISC
///////////////////////////////////////////////////////////////////////////////
// tests, other..
///////////////////////////////////////////////////////////////////////////////

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
	if (BrowseForSaveFile("S&M - Save ALR Wiki summary", cPath, NULL, "Text file (*.TXT)\0*.TXT\0All Files\0*.*\0", filename, BUFFER_SIZE))
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
			MessageBox(GetMainHwnd(), "Unable to write to file.", "Save ALR Wiki summary", MB_OK);
	}
}

void OpenStuff(COMMAND_T* _ct) {}
void TestWDLString(COMMAND_T*) {}

#endif
