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

const char* GetFileRelativePath(const char* _fn)
{
	if (const char* p = strrchr(_fn, PATH_SLASH_CHAR))
		return p+1;
	return _fn;
}

const char* GetFileExtension(const char* _fn)
{
	if (const char* p = strrchr(_fn, '.'))
		return p+1;
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

void Filenamize(char* _fnInOut)
{
	if (_fnInOut)
	{
		int i=0, j=0;
		while (_fnInOut[i])
		{
			if (_fnInOut[i] != ':' && _fnInOut[i] != '/' && _fnInOut[i] != '\\' &&
				_fnInOut[i] != '*' && _fnInOut[i] != '?' && _fnInOut[i] != '\"' &&
				_fnInOut[i] != '>' && _fnInOut[i] != '<' && _fnInOut[i] != '\'' && _fnInOut[i] != '|')
				_fnInOut[j++] = _fnInOut[i];
			i++;
		}
		_fnInOut[j] = '\0';
	}
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

// browse + return short resource filename (if possible and if _wantFullPath == false)
// returns false if cancelled
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

// get a short filename from a full resource path
// ex: C:\Documents and Settings\<user>\Application Data\REAPER\FXChains\EQ\JS\test.RfxChain -> EQ\JS\test.RfxChain
// notes: 
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

// get a full resource path from a short filename
// ex: EQ\JS\test.RfxChain -> C:\Documents and Settings\<user>\Application Data\REAPER\FXChains\EQ\JS\test.RfxChain
// notes: 
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

// use Filenamize() first!
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


///////////////////////////////////////////////////////////////////////////////
// ini file helpers
///////////////////////////////////////////////////////////////////////////////

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
	if (g_SNMIniFileVersion < 1) // i.e. sws version < v2.1.0 #18
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
	if (g_SNMIniFileVersion < 2) // i.e. sws version < v2.1.0 #21
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
	if (g_SNMIniFileVersion < 3) // i.e. sws version < v2.1.0 #22
	{
		WritePrivateProfileString("RESOURCE_VIEW", "DblClick_To", NULL, g_SNMIniFn.Get()); // remove key
		UpdatePrivateProfileString("RESOURCE_VIEW", "FilterByPath", "Filter", g_SNMIniFn.Get());
	}
	if (g_SNMIniFileVersion < 4) // i.e. sws version < v2.1.0 #28
		UpdatePrivateProfileString("RESOURCE_VIEW", "AutoSaveTrTemplateWithItems", "AutoSaveTrTemplateFlags", g_SNMIniFn.Get());
}


///////////////////////////////////////////////////////////////////////////////
// string localization stuff
// remarks:
// - *if* a langpack file is defined, we use a cache so that localized string
//   getters can return const char* (getters are somehow "no-op" otherwise)
// - strings that can change dynamically must not be localized (typically cycle
//   action names which can change although their cache ids remain the same..
//   fortunately, localizating a cycle action name does not make sense: it is
//   defined by the user)
///////////////////////////////////////////////////////////////////////////////

WDL_StringKeyedArray<WDL_FastString*> g_langpackFns(false, deletefaststrptr);
#ifdef _SNM_I8N_CACHE2
WDL_StringKeyedArray<WDL_StringKeyedArray<WDL_StringKeyedArray<WDL_FastString*>* >* > g_i8nCache;
#else
void freei8nCacheValue(WDL_FastString* p) {delete p;}
WDL_StringKeyedArray<WDL_FastString*> g_i8nCache(false, freei8nCacheValue);
#endif

void ClearLangPackCache(const char* _langpack)
{
	if (WDL_FastString* fnStr = g_langpackFns.Get(_langpack, NULL))
	{
#ifdef _WIN32
		// force ini file cache refresh: see http://support.microsoft.com/kb/68827
		// and http://code.google.com/p/sws-extension/issues/detail?id=397
		WritePrivateProfileString(NULL, NULL, NULL, fnStr->Get());
#endif
		g_langpackFns.Delete(_langpack);
	}
}

bool IsLangPackUsed(const char* _langpack) {
	return (GetCurLangPackFn(_langpack)->GetLength() > 0);
}

// same langpack for SWS & REPAER?
bool IsLangPackShared() {
	return (_stricmp(GetCurLangPackFn("SWS")->Get(), GetCurLangPackFn("REAPER")->Get()) == 0);
}

// lazy init: "" means initialized but no langpack defined
// note: this function never returns NULL
WDL_FastString* GetCurLangPackFn(const char* _langpack)
{
	WDL_FastString* fnStr = g_langpackFns.Get(_langpack, NULL);
	if (!fnStr)
	{
		fnStr = new WDL_FastString("");
		g_langpackFns.Insert(_langpack, fnStr);

		char fn[BUFFER_SIZE] = ""; 
		GetPrivateProfileString(_langpack, "langpack", "", fn, BUFFER_SIZE, get_ini_file());
		fnStr->Set(fn);

		if (*fn && !FileExists(fn))
		{
			fnStr->SetFormatted(BUFFER_SIZE, "%s%cLangPack%c%s", GetResourcePath(), PATH_SLASH_CHAR, PATH_SLASH_CHAR, fn);
			if (fnStr->GetLength() && !FileExists(fnStr->Get()))
				fnStr->Set("");
		}
	}
	return fnStr;
}

void GetLocalizedProfileString(const char* _langpack, const char* _section, const char* _key, const char* _defaultStr, char* _bufOut, int _bufOutSz)
{
	// langpack exists? 
	WDL_FastString* fnStr = GetCurLangPackFn(_langpack);
	if (fnStr->GetLength())
	{
		GetPrivateProfileString(_section, _key, "", _bufOut, _bufOutSz, fnStr->Get()); 
		// not found => 2nd try in the common section of the langpack file
		if (!*_bufOut)
		{
			if (!_stricmp(_langpack, "SWS"))
				GetPrivateProfileString(SNM_I8N_SWS_COMMON_SEC, _key, _defaultStr, _bufOut, _bufOutSz, fnStr->Get());
			    //JFB TODO: 3rd try ?
			else
				GetPrivateProfileString("common", _key, _defaultStr, _bufOut, _bufOutSz, fnStr->Get());
		}
	}
}

//JFB!!! debug point when exit!!
// this function never returns NULL
// note: nothing is instanciated if no langpack file is defined (lazy init otherwise)
const char* GetFromI8NCache(const char* _langpack, const char* _section, const char* _key, const char* _defaultStr, bool _swsAction)
{
	// langpack exists? 
	WDL_FastString* fnStr = GetCurLangPackFn(_langpack);
	if (fnStr->GetLength())
	{
#ifdef _SNM_I8N_CACHE2
		WDL_StringKeyedArray<WDL_StringKeyedArray<WDL_FastString*>* >* c1 = g_i8nCache.Get(_langpack, NULL);
		if (!c1) {
			c1 = new WDL_StringKeyedArray<WDL_StringKeyedArray<WDL_FastString*>* >;
			g_i8nCache.Insert(_langpack, c1);
		}
		WDL_StringKeyedArray<WDL_FastString*>* c2 = c1->Get(_section, NULL);
		if (!c2) {
			c2 = new WDL_StringKeyedArray<WDL_FastString*>;
			c1->Insert(_section, c2);
		}
		WDL_FastString* str = c2->Get(_key, NULL);
#else
		// make the key (most significant ids first)
		WDL_FastString cacheKeyStr(_key);
		cacheKeyStr.Append(_section);
		cacheKeyStr.Append(_langpack);
		WDL_FastString* str = g_i8nCache.Get(cacheKeyStr.Get(), NULL);
#endif
		if (!str)
		{
			char buf[SNM_I8N_DEF_STR_LEN] = "";
			GetLocalizedProfileString(_langpack, _section, _key, _defaultStr, buf, SNM_I8N_DEF_STR_LEN); 
			str = new WDL_FastString(buf);

			// special case: SWS action tags (SWS:, SWS/S&M:, etc..) are removed from langpack files 
			// because the extension code expects such tags (the user must not customize them!)
			// => bring them back
			if (_swsAction && !IsSwsAction(str->Get()))
					if (int len = IsSwsAction(_defaultStr))
						str->Insert(_defaultStr, 0, len);

#ifdef _SNM_I8N_CACHE2
			c2->Insert(_key, str);
#else
			g_i8nCache.Insert(cacheKeyStr.Get(), str);
#endif
		}
		return str->Get();
	}
	return _defaultStr;
}

const char* GetLocalizedString(const char* _langpack, const char* _section, const char* _key, const char* _defaultStr) {
	return GetFromI8NCache(_langpack, _section, _key, _defaultStr, false);
}

const char* GetLocalizedActionName(const char* _custId, const char* _defaultStr, const char* _section) {
	return GetFromI8NCache("SWS", _section, _custId, _defaultStr, true);
}

// just for code factorization..
bool IsDupLangPackKeyErrMsg(const char* _key, WDL_StringKeyedArray<WDL_FastString*>* _strings)
{
	if (_strings->Get(_key, NULL))
	{
		WDL_FastString msg("LangPack file generation/upgrade aborted!\nDuplicated string: \"");
		msg.Append(_key);
		msg.Append("\"");
		MessageBox(GetMainHwnd(), msg.Get(), "S&M - Error", MB_OK);
		return true;
	}
	return false;
}

// just for code factorization..
// note: multiples Append() are faster than AppendFormatted()
void AppendToLangPackStr(const char* _section, WDL_StringKeyedArray<WDL_FastString*>* _strings, WDL_FastString* _strOut, bool _isNew)
{
#ifndef _SNM_I8N_VIA_SAVEINISECTION
	if (_section)
	{
		_strOut->Append("[");
		_strOut->Append(_section);
		_strOut->Append("]\n");
	}
#else
	_strOut->SetFormatted(128, "; NOTE: as you translate a string, remove the %s from the beginning of the line.\n", SNM_I8N_NEW_STR_TAG);
#endif
	for (int i=0; i < _strings->GetSize(); i++)
	{
		const char* name = "";
		if (WDL_FastString* id = _strings->Enumerate(i, &name, NULL))
			if (name)
			{
				if (_isNew) _strOut->Append(SNM_I8N_NEW_STR_TAG);
				_strOut->Append(id->Get());
				_strOut->Append("=");
				_strOut->Append(name);
				_strOut->Append("\n");
			}
	}
#ifndef _SNM_I8N_VIA_SAVEINISECTION
	_strOut->Append("\n");
#endif
}

// returns 1 if upgrade done, 0 if already present , -1 on error
int UpgradeLangPackAction(const char* _section, const char* _customId, const char* _defActionName, WDL_StringKeyedArray<WDL_FastString*>* _strings)
{
	char buf[SNM_I8N_DEF_STR_LEN] = "";
	GetLocalizedProfileString("SWS", _section, _customId, "", buf, SNM_I8N_DEF_STR_LEN);
	const char* name = (*buf ? buf+IsSwsAction(buf) : (const char*)(_defActionName+IsSwsAction(_defActionName)));

	if (IsDupLangPackKeyErrMsg(name, _strings))
		return -1; // err
#ifndef _SNM_I8N_VIA_SAVEINISECTION
	if (*buf)
	{
		_strings->Insert(name, new WDL_FastString(_customId));
		return 0;
	}
	else
	{
		WDL_FastString* idStr = new WDL_FastString(SNM_I8N_NEW_STR_TAG);
		idStr->Append(_customId);
		_strings->Insert(name, idStr); // sort behind the scene..
		return 1; 
	}
#else
	if (*buf)
	{
		_strings->Insert(name, new WDL_FastString(_customId));
		return 0;
	}
	else
	{
		WDL_FastString* idStr = new WDL_FastString(SNM_I8N_NEW_STR_TAG);
		idStr->Append(_customId);
		GetLocalizedProfileString("SWS", _section, idStr->Get(), "", buf, SNM_I8N_DEF_STR_LEN);

		// already tagged as new?
		if (*buf) {
			_strings->Insert((const char*)(buf+IsSwsAction(buf)), idStr);
			return 0;
		}
		// new!
		else {
			_strings->Insert(name, idStr); // sort behind the scene..
			return 1; 
		}
	}
#endif
}

bool ConfirmNewLangPackMsg()
{
	WDL_FastString* fnStr = GetCurLangPackFn("SWS");
	if (fnStr->GetLength())
	{
		WDL_FastString msg("A LangPack file is already defined: ");
		msg.Append(fnStr->Get());
		msg.Append("\nAre you sure you want to assign a new LangPack (the current file will not be deleted) ?");
		if (IDYES != MessageBox(GetMainHwnd(), msg.Get(), "S&M - Error", MB_YESNO))
			return false;
	}
	return true;
}

void ResetLangPack(COMMAND_T* _ct)
{
	// langpack exists? 
	WDL_FastString* fnStr = GetCurLangPackFn("SWS");
	if (fnStr->GetLength())
	{
		WritePrivateProfileString("SWS", "langpack", NULL, get_ini_file());

		WDL_FastString msg("Reseted SWS LangPack file ");
		msg.Append(fnStr->Get());
		msg.Append("\nThis file is preserved. Restart REAPER to complete the operation!");
		MessageBox(GetMainHwnd(), msg.Get(), "S&M - Information", MB_OK);

		ClearLangPackCache("SWS"); // done after: fnStr is used above..
	}
	else
		MessageBox(GetMainHwnd(), "No SWS LangPack file is defined!", "S&M - Error", MB_OK);
}

void LoadAssignLangPack(COMMAND_T* _ct)
{
	if (!ConfirmNewLangPackMsg())
		return;

	WDL_FastString resPath;
	resPath.SetFormatted(BUFFER_SIZE, "%s%cLangPack%c", GetResourcePath(), PATH_SLASH_CHAR, PATH_SLASH_CHAR);
	if (char* fn = BrowseForFiles("S&M - Load SWS LangPack file", resPath.Get(), NULL, false, SNM_TXT_EXT_LIST))
	{
		WritePrivateProfileString("SWS", "langpack", strstr(fn, resPath.Get()) ? GetFilenameWithExt(fn) : fn, get_ini_file());
		ClearLangPackCache("SWS");
		free(fn);
		MessageBox(GetMainHwnd(), "Restart REAPER to complete the operation!", "S&M - Information", MB_OK);
	}
}

void GenerateLangPack(COMMAND_T* _ct)
{
	if (!ConfirmNewLangPackMsg())
		return;

	WDL_FastString resPath;
	resPath.SetFormatted(BUFFER_SIZE, "%s%cLangPack%c", GetResourcePath(), PATH_SLASH_CHAR, PATH_SLASH_CHAR);

	WDL_FastString defaultFn;
	defaultFn.Append(resPath.Get());
	defaultFn.Append("sws_english.txt");

	char fn[BUFFER_SIZE] = "";
	if (BrowseForSaveFile("S&M - Generate default SWS LangPack file", defaultFn.Get(), defaultFn.Get(), SNM_TXT_EXT_LIST, fn, BUFFER_SIZE))
	{
		WDL_FastString langpack;
#ifndef _SNM_I8N_VIA_SAVEINISECTION
		langpack.Append(SNM_I8N_DEF_LANGPACK_NAME);
		langpack.AppendFormatted(128, "; NOTE: as you translate a string, remove %s from the beginning of the line.\n\n", SNM_I8N_NEW_STR_TAG);
#endif
		WDL_StringKeyedArray<WDL_FastString*> orderedStrings(true, deletefaststrptr); // just to sort things
		int strCount = 0;

		// 1) add all sws actions of the main action section (except cycle actions)
		for (int i=g_iFirstCommand; i <= g_iLastCommand; i++)
			if (COMMAND_T* ct = SWSGetCommandByID(i))
				if (!strstr(ct->id, "_CYCLACTION")) {						
					// remove action tags (SWS:, SWS/S&M:, etc..)
					char* name = (char*)(ct->accel.desc+IsSwsAction(ct->accel.desc));
					if (IsDupLangPackKeyErrMsg(name, &orderedStrings)) return;
					orderedStrings.Insert(name, new WDL_FastString(ct->id));
				}
		AppendToLangPackStr(SNM_I8N_SWS_ACTION_SEC, &orderedStrings, &langpack, true);
#ifdef _SNM_I8N_VIA_SAVEINISECTION
		SaveIniSection(SNM_I8N_SWS_ACTION_SEC, &langpack, fn);
#endif
		strCount += orderedStrings.GetSize();
		orderedStrings.DeleteAll();

		// 2) add all actions of the "s&m extension" action section
		int i=0;
		while(g_SNMSection_cmdTable[i].id != LAST_COMMAND)
		{
			MIDI_COMMAND_T* ct = &g_SNMSection_cmdTable[i];
			char* name = (char*)(ct->accel.desc+IsSwsAction(ct->accel.desc));
			if (IsDupLangPackKeyErrMsg(name, &orderedStrings)) return;
			orderedStrings.Insert(name, new WDL_FastString(ct->id));
			i++;
		}
		AppendToLangPackStr(SNM_I8N_SNM_ACTION_SEC, &orderedStrings, &langpack, true);
#ifdef _SNM_I8N_VIA_SAVEINISECTION
		SaveIniSection(SNM_I8N_SNM_ACTION_SEC, &langpack, fn);
#endif
		strCount += orderedStrings.GetSize();
		orderedStrings.DeleteAll();

		// 3) write the LangPack file
#ifndef _SNM_I8N_VIA_SAVEINISECTION
		if (FILE* f = fopenUTF8(fn, "w"))
#endif
		{
#ifndef _SNM_I8N_VIA_SAVEINISECTION
			fputs(langpack.Get(), f);
			fclose(f);
#endif
			// 4) update REAPER.ini + reset localized strings cache + final info message
			WritePrivateProfileString("SWS", "langpack", strstr(fn, resPath.Get()) ? GetFilenameWithExt(fn) : fn, get_ini_file());
			ClearLangPackCache("SWS");

#ifdef _SNM_I8N_VIA_SAVEINISECTION
			// add langpack name if needed
			if (!IsLangPackShared() && LoadChunk(fn, &langpack, true) && strncmp(langpack.Get(), "#NAME:", 6)) 
			{
				langpack.Insert(SNM_I8N_DEF_LANGPACK_NAME, 0);
				if (FILE* f = fopenUTF8(fn, "w")) {
					fputs(langpack.Get(), f);
					fclose(f);
				}
			}
#endif
			WDL_FastString msg;
			msg.SetFormatted(BUFFER_SIZE, "Wrote %s (%d strings)\n", fn, strCount);
			msg.Append("Restart REAPER after editing this file!");
			MessageBox(GetMainHwnd(), msg.Get(), "S&M - Information", MB_OK);
		}
#ifndef _SNM_I8N_VIA_SAVEINISECTION
		else
			MessageBox(GetMainHwnd(), "Unable to write to file!", "S&M - Error", MB_OK);
#endif
	}
}

void UpgradeLangPack(COMMAND_T* _ct)
{
	WDL_FastString* fnStr = GetCurLangPackFn("SWS"); // cache may have changed..
	if (!fnStr->GetLength()) {
		GenerateLangPack(NULL); // null is ok: no undo point, etc..
		return;
	}

	WDL_FastString langpack;
#ifndef _SNM_I8N_VIA_SAVEINISECTION
	// get the current langpack name 
	langpack.Append(SNM_I8N_DEF_LANGPACK_NAME);
	WDL_FastString nameStr;
	if (LoadChunk(fnStr->Get(), &nameStr, true, 512) && !strncmp(nameStr.Get(), "#NAME:", 6))
		if (const char* p = strchr(nameStr.Get(), '\n'))
			langpack.Set(nameStr.Get(), (int)(p-nameStr.Get())+1);
	langpack.AppendFormatted(128, "; NOTE: as you translate a string, remove %s from the beginning of the line.\n\n", SNM_I8N_NEW_STR_TAG);
#endif
	int upToDate=0, newStrCount=0;
	WDL_StringKeyedArray<WDL_FastString*> orderedStrings(true, deletefaststrptr); // just to sort things

	// 1) upgrade all sws actions of the main action section (except cycle actions)
	for (int i=g_iFirstCommand; i <= g_iLastCommand; i++)
	{
		if (COMMAND_T* ct = SWSGetCommandByID(i))
			if (!strstr(ct->id, "_CYCLACTION"))
			{
				int r = UpgradeLangPackAction(SNM_I8N_SWS_ACTION_SEC, ct->id, ct->accel.desc, &orderedStrings);
				if (r < 0) return; // err
				newStrCount += r;
			}
	}
	AppendToLangPackStr(SNM_I8N_SWS_ACTION_SEC, &orderedStrings, &langpack, false);
#ifdef _SNM_I8N_VIA_SAVEINISECTION
	SaveIniSection(SNM_I8N_SWS_ACTION_SEC, &langpack, fnStr->Get());
#endif
	orderedStrings.DeleteAll();

	// 2) add all actions of the s&m action section
	upToDate=0;
	int i=0;
	while(g_SNMSection_cmdTable[i].id != LAST_COMMAND)
	{
		MIDI_COMMAND_T* ct = &g_SNMSection_cmdTable[i];
		int r = UpgradeLangPackAction(SNM_I8N_SNM_ACTION_SEC, ct->id, ct->accel.desc, &orderedStrings);
		if (r < 0) return; // err
		newStrCount += r;
		i++;
	}
	AppendToLangPackStr(SNM_I8N_SNM_ACTION_SEC, &orderedStrings, &langpack, false);
#ifdef _SNM_I8N_VIA_SAVEINISECTION
	SaveIniSection(SNM_I8N_SNM_ACTION_SEC, &langpack, fnStr->Get());
#endif
	orderedStrings.DeleteAll();

	// 3) write the LangPack file (if something upgraded) + msg
	// note: do not clear the langpack cache: simple upgrade
	WDL_FastString msg;
	if (newStrCount)
	{
#ifndef _SNM_I8N_VIA_SAVEINISECTION
		if (FILE* f = fopenUTF8(fnStr->Get(), "w"))
#endif
		{
#ifndef _SNM_I8N_VIA_SAVEINISECTION
			fputs(langpack.Get(), f);
			fclose(f);
			msg.SetFormatted(BUFFER_SIZE, "Upgraded %s\n", fnStr->Get());
#else
			msg.SetFormatted(BUFFER_SIZE, "Upgraded %s (%d new strings)\n", fnStr->Get(), newStrCount);
#endif
			msg.Append("Restart REAPER after editing this file!");
			MessageBox(GetMainHwnd(), msg.Get(), "S&M - Information", MB_OK);
		}
#ifndef _SNM_I8N_VIA_SAVEINISECTION
		else
			MessageBox(GetMainHwnd(), "Unable to write to file!", "S&M - Error", MB_OK);
#endif
	}
	else {
		msg.SetFormatted(BUFFER_SIZE, "%s is up to date!", fnStr->Get());
		MessageBox(GetMainHwnd(), msg.Get(), "S&M - Information", MB_OK);
	}
}


///////////////////////////////////////////////////////////////////////////////
// marker/region helpers
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


///////////////////////////////////////////////////////////////////////////////
// string helpers
///////////////////////////////////////////////////////////////////////////////

void makeUnformatedConfigString(const char* _in, WDL_FastString* _out)
{
	if (_in && _out)
	{
		_out->Set(_in);
		const char* p = strchr(_out->Get(), '%');
		while(p)
		{
			int pos = (int)(p - _out->Get());
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

int IsSwsAction(const char* _actionName)
{
	if (const char* p = strstr(_actionName, ": ")) // no strchr() here: make sure p[2] is not out of bounds
		if (const char* tag = stristr(_actionName, "sws")) // make sure it is a sws tag
			if (tag < p) // make really sure
				return ((int)(p+2-_actionName));
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// theme slots (Resources view)
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
// image slots (Resources view)
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
// other misc helpers
///////////////////////////////////////////////////////////////////////////////

// FNV-1 hash
/*JFB example
	WDL_FastString keyStr;
	WDL_UINT64 h = FNV1Hash64(_defaultStr);
	keyStr.AppendFormatted(32, "%X", h>>32);
	keyStr.AppendFormatted(32, "%X", h);
	cacheKeyStr.Append(keyStr.Get());
*/
WDL_UINT64 FNV1Hash64(const char* _data)
{
	int len = strlen(_data)+1;
	const unsigned char* p = (const unsigned char*)_data;
	WDL_UINT64 h = 0xcbf29ce484222325ull;
	for (; len; --len) {
		h *= 0x100000001b3ull;
		h ^= *p++;
	}
	return h;
}

// _expectedSection: no check if NULL
//JFB no localization for section names yet (confirmed by Cockos: http://forum.cockos.com/showpost.php?p=894702&postcount=117)
bool LearnAction(char* _idstrOut, int _idStrSz, const char* _expectedSection)
{
	char section[SNM_MAX_SECTION_NAME_LEN] = "";
	int actionId, selItem = GetSelectedAction(section, SNM_MAX_SECTION_NAME_LEN, &actionId, _idstrOut, _idStrSz);
	if (_expectedSection && strcmp(section, _expectedSection)) //JFB!!! localization!
		selItem = -1;
	switch (selItem)
	{
		case -2:
			MessageBox(GetMainHwnd(), "The column 'Custom ID' is not displayed in the Actions window!\n(to display it, right click on its table header > show action IDs)", "S&M - Cycle Action editor - Error", MB_OK);
			return false;
		case -1:
		{
			char bufMsg[256] = "Actions window not opened or no selected action!";
			if (_expectedSection)
				_snprintf(bufMsg, 256, "Actions window not opened or section '%s' not selected or no selected action!", _expectedSection);
			MessageBox(GetMainHwnd(), bufMsg, "S&M - Cycle Action editor - Error", MB_OK);
			return false;
		}
	}
	return true;
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
// misc	actions
///////////////////////////////////////////////////////////////////////////////

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
	POINT p;
	GetCursorPos(&p);
	mouse_event(MOUSEEVENTF_LEFTDOWN, p.x, p.y, 0, 0);
	mouse_event(MOUSEEVENTF_LEFTUP, p.x, p.y, 0, 0);
	WinWaitForEvent(WM_LBUTTONUP);
}

// dump actions or the wiki ALR summary for the current section displayed in the action dlg 
// API LIMITATION: the action dlg is used because only the main section could be dumped othewise..
// See http://forum.cockos.com/showthread.php?t=61929 and http://wiki.cockos.com/wiki/index.php/Action_List_Reference
// _type: 1 & 2 for ALR wiki (1=native actions, 2=SWS)
// _type: 3 & 4 for basic dump (3=native actions, 4=SWS)
bool DumpActionList(int _type, const char* _title, const char* _lineFormat, const char* _heading, const char* _ending)
{
	if (IsLangPackUsed("SWS")) {
		MessageBox(GetMainHwnd(), "Dump failed: a SWS LangPack file is being used,\nplease restore factory settings first!", "S&M - Error", MB_OK);
		return false;
	}

	char currentSection[SNM_MAX_SECTION_NAME_LEN] = "";
	HWND hList = GetActionListBox(currentSection, SNM_MAX_SECTION_NAME_LEN);
	if (hList && currentSection)
	{
		char sectionURL[SNM_MAX_SECTION_NAME_LEN] = ""; 
		if (!GetSectionNameAsURL(_type == 1 || _type == 2, currentSection, sectionURL, SNM_MAX_SECTION_NAME_LEN))
		{
			MessageBox(GetMainHwnd(), "Dump failed: unknown section!", _title, MB_OK);
			return false;
		}

		char name[SNM_MAX_SECTION_NAME_LEN*2] = "", fn[BUFFER_SIZE] = "";
		_snprintf(name, SNM_MAX_SECTION_NAME_LEN*2, "%s%s.txt", sectionURL, !(_type % 2) ? "_SWS" : "");
		if (!BrowseForSaveFile(_title, GetResourcePath(), name, SNM_TXT_EXT_LIST, fn, BUFFER_SIZE))
			return false;

		//flush
		FILE* f = fopenUTF8(fn, "w"); 
		if (f)
		{
			fputs("\n", f);
			fclose(f);

			f = fopenUTF8(fn, "a"); 
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

				int isSws = IsSwsAction(cmdName);
				if (strncmp(cmdName, "Custom:", 7) &&
					((_type%2 && !isSws) || (!(_type%2) && isSws && !strstr(customId, "_CYCLACTION"))))
				{
					if (!*customId) // for native actions..
						_snprintf(customId, SNM_MAX_ACTION_CUSTID_LEN, "%d", cmdId);
					fprintf(f, _lineFormat, sectionURL, customId, cmdName, customId);
				}
			}
			if (_ending)
				fprintf(f, _ending); 

			fclose(f);

			char msg[BUFFER_SIZE] = "";
			_snprintf(msg, BUFFER_SIZE, "Wrote %s", fn); 
			MessageBox(GetMainHwnd(), msg, _title, MB_OK);
			return true;
		}
		else
			MessageBox(GetMainHwnd(), "Dump failed: unable to write to file!", _title, MB_OK);
	}
	else
		MessageBox(GetMainHwnd(), "Dump failed: action window not opened!", _title, MB_OK);
	return false;
}

void DumpWikiActionList(COMMAND_T* _ct)
{
	DumpActionList(
		(int)_ct->user, 
		"S&M - Save ALR Wiki summary", 
		"|-\n| [[%s_%s|%s]] || %s\n",
		"{| class=\"wikitable\"\n|-\n! Action name !! Cmd ID\n",
		"|}\n");
}

void DumpActionList(COMMAND_T* _ct) {
	DumpActionList((int)_ct->user, "S&M - Dump action list", "%s\t%s\t%s\n", "Section\tId\tAction\n", NULL);
}
