/******************************************************************************
/ SnM_Util.cpp
/
/ Copyright (c) 2012 Jeffos
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
#include "SnM.h"
#include "../reaper/localize.h"
#include "../../WDL/projectcontext.h"


///////////////////////////////////////////////////////////////////////////////
// File util
// JFB TODO: WDL_UTF8, exist_fn, ..
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

void Filenamize(char* _fnInOut, int _fnInOutSz)
{
	if (_fnInOut)
	{
		int i=0, j=0;
		while (_fnInOut[i] && i < _fnInOutSz)
		{
			if (_fnInOut[i] != ':' && _fnInOut[i] != '/' && _fnInOut[i] != '\\' &&
				_fnInOut[i] != '*' && _fnInOut[i] != '?' && _fnInOut[i] != '\"' &&
				_fnInOut[i] != '>' && _fnInOut[i] != '<' && _fnInOut[i] != '\'' && _fnInOut[i] != '|')
				_fnInOut[j++] = _fnInOut[i];
			i++;
		}
		if (j>=_fnInOutSz)
			j = _fnInOutSz-1;
		_fnInOut[j] = '\0';
	}
}

bool IsValidFilenameErrMsg(const char* _fn, bool _errMsg)
{
	bool ko = (!_fn || !*_fn
		|| strchr(_fn, ':') || strchr(_fn, '/') || strchr(_fn, '\\')
		|| strchr(_fn, '*') || strchr(_fn, '?') || strchr(_fn, '\"')
		|| strchr(_fn, '>') || strchr(_fn, '<') || strchr(_fn, '\'')
		|| strchr(_fn, '|'));

	if (ko && _errMsg)
	{
		char buf[BUFFER_SIZE];
		lstrcpyn(buf, __LOCALIZE("Empty filename!","sws_mbox"), BUFFER_SIZE);
		if (_fn && *_fn) _snprintfSafe(buf, sizeof(buf), __LOCALIZE_VERFMT("Invalid filename: %s\nFilenames cannot contain any of the following characters: / \\ * ? \" < > ' | :","sws_mbox"), _fn);
		MessageBox(GetMainHwnd(), buf, __LOCALIZE("S&M - Error","sws_mbox"), MB_OK);
	}
	return !ko;
}

// the API function file_exists() is a bit different, it returns false for folder paths, etc..
bool FileExistsErrMsg(const char* _fn, bool _errMsg)
{
	bool exists = FileExists(_fn);
	if (!exists && _errMsg)
	{
		char buf[BUFFER_SIZE];
		lstrcpyn(buf, __LOCALIZE("Empty filename!","sws_mbox"), BUFFER_SIZE);
		if (_fn && *_fn) _snprintfSafe(buf, sizeof(buf), __LOCALIZE_VERFMT("Invalid filename: %s\nFilenames cannot contain any of the following characters: / \\ * ? \" < > ' | :","sws_mbox"), _fn);
		MessageBox(GetMainHwnd(), buf, __LOCALIZE("S&M - Error","sws_mbox"), MB_OK);
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
			return (SendFileToRecycleBin(_filename) ? false : true); // avoid warning C4800
#endif
		ok = (DeleteFile(_filename) ? true : false); // avoid warning C4800
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
	char defaultPath[BUFFER_SIZE];
	if (_snprintfStrict(defaultPath, sizeof(defaultPath), "%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, _resSubDir) < 0)
		*defaultPath = '\0';
	if (char* fn = BrowseForFiles(_title, defaultPath, NULL, false, _fileFilters)) 
	{
		if(!_wantFullPath)
			GetShortResourcePath(_resSubDir, fn, _fn, _fnSize);
		else
			lstrcpyn(_fn, fn, _fnSize);
		free(fn);
		ok = true;
	}
	return ok;
}

// get a short filename from a full resource path
// ex: C:\Documents and Settings\<user>\Application Data\REAPER\FXChains\EQ\JS\test.RfxChain -> EQ\JS\test.RfxChain
// notes: 
// - *must* work with non existing files (just some string processing here)
// - *must* be no-op for non resource paths (c:\temp\test.RfxChain -> c:\temp\test.RfxChain)
// - *must* be no-op for short resource paths 
void GetShortResourcePath(const char* _resSubDir, const char* _fullFn, char* _shortFn, int _fnSize)
{
	if (_resSubDir && *_resSubDir && _fullFn && *_fullFn)
	{
		char defaultPath[BUFFER_SIZE] = "";
		if (_snprintfStrict(defaultPath, sizeof(defaultPath), "%s%c%s%c", GetResourcePath(), PATH_SLASH_CHAR, _resSubDir, PATH_SLASH_CHAR) < 0)
			*defaultPath = '\0';
		if (strstr(_fullFn, defaultPath) == _fullFn) // no stristr: osx + utf-8
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
// - no-op for non resource paths (c:\temp\test.RfxChain -> c:\temp\test.RfxChain)
// - no-op for full resource paths 
void GetFullResourcePath(const char* _resSubDir, const char* _shortFn, char* _fullFn, int _fnSize)
{
	if (_shortFn && _fullFn) 
	{
		if (*_shortFn == '\0') {
			*_fullFn = '\0';
			return;
		}
		if (!strstr(_shortFn, GetResourcePath())) // no stristr: osx + utf-8
		{
			char resFn[BUFFER_SIZE] = "", resDir[BUFFER_SIZE];
			if (_snprintfStrict(resFn, sizeof(resFn), "%s%c%s%c%s", GetResourcePath(), PATH_SLASH_CHAR, _resSubDir, PATH_SLASH_CHAR, _shortFn) > 0)
			{
				lstrcpyn(resDir, resFn, BUFFER_SIZE);
				if (char* p = strrchr(resDir, PATH_SLASH_CHAR)) *p = '\0';
				if (FileExists(resDir)) {
					lstrcpyn(_fullFn, resFn, _fnSize);
					return;
				}
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
/*JFB commented: horribly slow!
		if (ProjectStateContext* ctx = ProjectCreateFileRead(_fn))
		{
			char buf[SNM_MAX_CHUNK_LINE_LENGTH];
			while(!ctx->GetLine(buf, SNM_MAX_CHUNK_LINE_LENGTH))
			{
				if (_maxlen && (int)(_chunk->GetLength()+strlen(buf)) > _maxlen)
					break;
				_chunk->Append(buf);
			}
			delete ctx;
			return true;
		}
*/
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
					if (*p != '\n' && *p != '\r') // the !*p case is managed in Append()
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
		SNM_ChunkIndenter p(_chunk, false); // no auto-commit, see trick below
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
bool GenerateFilename(const char* _dir, const char* _name, const char* _ext, char* _updatedFn, int _updatedSz)
{
	if (_dir && _name && _ext && _updatedFn && *_dir)
	{
		char fn[BUFFER_SIZE];
		bool slash = _dir[strlen(_dir)-1] == PATH_SLASH_CHAR;
		if (slash) {
			if (_snprintfStrict(fn, sizeof(fn), "%s%s.%s", _dir, _name, _ext) <= 0)
				return false;
		} else {
			if (_snprintfStrict(fn, sizeof(fn), "%s%c%s.%s", _dir, PATH_SLASH_CHAR, _name, _ext) <= 0)
				return false;
		}

		int i=0;
		while(FileExists(fn))
		{
			if (slash) {
				if (_snprintfStrict(fn, sizeof(fn), "%s%s_%03d.%s", _dir, _name, ++i, _ext) <= 0)
					return false;
			} else {
				if (_snprintfStrict(fn, sizeof(fn), "%s%c%s_%03d.%s", _dir, PATH_SLASH_CHAR, _name, ++i, _ext) <= 0)
					return false;
			}
		}
		lstrcpyn(_updatedFn, fn, _updatedSz);
		return true;
	}
	return false;
}

// fills a list of filenames for the desired extension
// if _ext == NULL or '\0', look for media files
// note: it is up to the caller to free _files (WDL_PtrList_DeleteOnDestroy)
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
		const char* pEOL = _str->Get()-1;
		char curLine[SNM_MAX_CHUNK_LINE_LENGTH] = "";
		for(;;) 
		{
			const char* pLine = pEOL+1;
			pEOL = strchr(pLine, '\n');
			if (!pEOL)
				break;
			int curLineLen = (int)(pEOL-pLine);
			if (curLineLen >= SNM_MAX_CHUNK_LINE_LENGTH)
				curLineLen = SNM_MAX_CHUNK_LINE_LENGTH-1; // trim long lines
			memcpy(curLine, pLine, curLineLen);
			curLine[curLineLen] = '\0';
			_ctx->AddLine("%s", curLine); // "%s" needed, see http://code.google.com/p/sws-extension/issues/detail?id=358
		}
	}
}

void ExtensionConfigToString(WDL_FastString* _str, ProjectStateContext* _ctx)
{
	if (_str && _ctx)
	{
		LineParser lp(false);
		char linebuf[SNM_MAX_CHUNK_LINE_LENGTH] = "";
		for(;;) 
		{
			if (!_ctx->GetLine(linebuf, sizeof(linebuf)) && !lp.parse(linebuf))
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
// Ini file helpers
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
	g_SNMIniFileVersion = GetPrivateProfileInt("General", "IniFileUpgrade", 0, g_SNMIniFn.Get());

	if (g_SNMIniFileVersion < 1) // < v2.1.0 #18
	{
		// upgrade deprecated section names 
		UpdatePrivateProfileSection("FXCHAIN", "FXChains", g_SNMIniFn.Get());
		UpdatePrivateProfileSection("FXCHAIN_VIEW", "RESOURCE_VIEW", g_SNMIniFn.Get());
		// upgrade deprecated key names (automatically generated now)
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
	if (g_SNMIniFileVersion < 2) // < v2.1.0 #21
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
	if (g_SNMIniFileVersion < 3) // < v2.1.0 #22
	{
		WritePrivateProfileString("RESOURCE_VIEW", "DblClick_To", NULL, g_SNMIniFn.Get()); // remove key
		UpdatePrivateProfileString("RESOURCE_VIEW", "FilterByPath", "Filter", g_SNMIniFn.Get());
	}
	if (g_SNMIniFileVersion < 4) // < v2.2.0
	{
		UpdatePrivateProfileString("RESOURCE_VIEW", "AutoSaveTrTemplateWithItems", "AutoSaveTrTemplateFlags", g_SNMIniFn.Get());
#ifdef _WIN32
		UpdatePrivateProfileSection("data\\track_icons", "Track_icons", g_SNMIniFn.Get());
		UpdatePrivateProfileString("RESOURCE_VIEW", "AutoFillDirdata\\track_icons", "AutoFillDirTrack_icons", g_SNMIniFn.Get());
		UpdatePrivateProfileString("RESOURCE_VIEW", "TiedActionsdata\\track_icons", "TiedActionsTrack_icons", g_SNMIniFn.Get());
		UpdatePrivateProfileString("RESOURCE_VIEW", "DblClickdata\\track_icons", "DblClickTrack_icons", g_SNMIniFn.Get());
#else
		UpdatePrivateProfileSection("data/track_icons", "Track_icons", g_SNMIniFn.Get());
		UpdatePrivateProfileString("RESOURCE_VIEW", "AutoFillDirdata/track_icons", "AutoFillDirTrack_icons", g_SNMIniFn.Get());
		UpdatePrivateProfileString("RESOURCE_VIEW", "TiedActionsdata/track_icons", "TiedActionsTrack_icons", g_SNMIniFn.Get());
		UpdatePrivateProfileString("RESOURCE_VIEW", "DblClickdata/track_icons", "DblClickTrack_icons", g_SNMIniFn.Get());
#endif
	}
	if (g_SNMIniFileVersion < 5) // < v2.2.0 #3
		UpdatePrivateProfileSection("LAST_CUEBUS", "CueBuss1", g_SNMIniFn.Get());
	if (g_SNMIniFileVersion < 6) // < vv2.2.0 #6
		WritePrivateProfileStruct("RegionPlaylist", NULL, NULL, 0, g_SNMIniFn.Get()); // flush section

	g_SNMIniFileVersion = SNM_INI_FILE_VERSION;
}


///////////////////////////////////////////////////////////////////////////////
// Marker/region helpers
///////////////////////////////////////////////////////////////////////////////

// returns the 1st marker or region index found at _pos
// note: relies on markers & regions indexed by positions
// _flags: &SNM_MARKER_MASK=marker, &SNM_REGION_MASK=region
int FindMarkerRegion(double _pos, int _flags, int* _idOut)
{
	if (_idOut)
		*_idOut = -1;

	int idx=-1, x=0, lastx=0, num; double dPos, dEnd; bool isRgn;
	while (x = EnumProjectMarkers3(NULL, x, &isRgn, &dPos, &dEnd, NULL, &num, NULL))
	{
		if ((!isRgn && _flags&SNM_MARKER_MASK) || (isRgn && _flags&SNM_REGION_MASK))
		{
			if (_pos >= dPos && (!isRgn || (isRgn && _pos <= dEnd)))
			{
				bool isRgn2;
				if (EnumProjectMarkers3(NULL, x, &isRgn2, &dPos, NULL, NULL, NULL, NULL) &&
					((!isRgn2 && _flags&SNM_MARKER_MASK) || (isRgn2 && _flags&SNM_REGION_MASK)))
				{
					if (_pos < dPos) {
						idx = lastx;
						break;
					}
				}
				else {
					idx = lastx;
					break;
				}
			}
		}
		lastx=x;
	}
	if (idx >= 0 && _idOut)
		*_idOut = MakeMarkerRegionId(num, isRgn);
	return idx;
}

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

int GetMarkerRegionIndexFromId(int _id)
{
	if (_id > 0)
	{
		int x=0, lastx=0, num=(_id&0x3FFFFFFF), num2; 
		bool isRgn = IsRegion(_id), isRgn2;
		while (x = EnumProjectMarkers3(NULL, x, &isRgn2, NULL, NULL, NULL, &num2, NULL)) {
			if (num == num2 && isRgn == isRgn2)
				return lastx;
			lastx=x;
		}
	}
	return -1;
}

int GetMarkerRegionNumFromId(int _id) {
	return _id>0 ? (_id&0x3FFFFFFF) : -1;
}

bool IsRegion(int _id) {
	return (_id > 0 && (_id&0x40000000) != 0);
}

// returns next region/marker index or 0 when finished enumerating
// ideally the caller should also check !*_descOut
// _flags: &1=marker, &2=region
// note: the string formating inspired by the native popup "jump to marker"
int EnumMarkerRegionDesc(int _idx, char* _descOut, int _outSz, int _flags, bool _wantsName)
{
	int nextIdx = 0;
	if (_descOut && _idx >= 0)
	{
		*_descOut = '\0';
		double pos, end; int num; bool isRgn; char* name;
		if (nextIdx = EnumProjectMarkers2(NULL, _idx, &isRgn, &pos, &end, &name, &num))
		{
			if (!isRgn && _flags&SNM_MARKER_MASK)
			{
				char timeStr[32] = "";
				format_timestr_pos(pos, timeStr, 32, -1);
				if (_wantsName && *name)
					_snprintfSafe(_descOut, _outSz, "%d: %s [%s]", num, name, timeStr);
				else
					_snprintfSafe(_descOut, _outSz, "%d: [%s]", num, timeStr);
			}
			if (isRgn && _flags&SNM_REGION_MASK)
			{
				char timeStr1[32]="", timeStr2[32]="";
				format_timestr_pos(pos, timeStr1, 32, -1);
				format_timestr_pos(end, timeStr2, 32, -1);
				if (_wantsName && *name)
					_snprintfSafe(_descOut, _outSz, "%d: %s [%s -> %s]", num, name, timeStr1, timeStr2);
				else
					_snprintfSafe(_descOut, _outSz, "%d: [%s -> %s]", num, timeStr1, timeStr2);
			}
		}
	}
	return nextIdx;
}

// _flags: &1=marker, &2=region
void FillMarkerRegionMenu(HMENU _menu, int _msgStart, int _flags, UINT _uiState)
{
	int x=0, lastx=0;
	char desc[SNM_MAX_MARKER_NAME_LEN]="";
	while (x = EnumMarkerRegionDesc(x, desc, SNM_MAX_MARKER_NAME_LEN, _flags, true)) {
		if (*desc)
			AddToMenu(_menu, desc, _msgStart+lastx, -1, false, _uiState);
		lastx=x;
	}
	if (!GetMenuItemCount(_menu))
		AddToMenu(_menu, __LOCALIZE("(No region!)","sws_menu"), 0, -1, false, MF_GRAYED);
}


///////////////////////////////////////////////////////////////////////////////
// Time helpers
///////////////////////////////////////////////////////////////////////////////

// note: the API's SnapToGrid() requires snap options to be enabled..
int SNM_SnapToMeasure(double _pos)
{
	int meas;
	if (TimeMap2_timeToBeats(NULL, _pos, &meas, NULL, NULL, NULL) > SNM_FUDGE_FACTOR) {
		double measPosA = TimeMap2_beatsToTime(NULL, 0.0, &meas); meas++;
		double measPosB = TimeMap2_beatsToTime(NULL, 0.0, &meas);
		return ((_pos-measPosA) > (measPosB-_pos) ? meas : meas-1);
	}
	return meas;
}

void TranslatePos(double _pos, int* _h, int* _m, int* _s, int* _ms)
{
	if (_h) *_h = int(_pos/3600);
	if (_m) *_m = int((_pos - 3600*int(_pos/3600)) / 60);
	if (_s) *_s = int(_pos - 3600*int(_pos/3600) - 60*int((_pos-3600*int(_pos/3600))/60));
	if (_ms) *_ms = int(1000*(_pos-int(_pos)) + 0.5); // rounding
}


///////////////////////////////////////////////////////////////////////////////
// String helpers
///////////////////////////////////////////////////////////////////////////////

// a _snprintf that ensures the string is always null terminated (but truncated if needed)
int _snprintfSafe(char* _buf, size_t _n, const char* _fmt, ...)
{
  va_list va;
  va_start(va, _fmt);
  *_buf='\0';
#if defined(_WIN32) && defined(_MSC_VER)
  int l = _vsnprintf(_buf, _n, _fmt, va);
  if (l < 0 || l >= (int)_n) {
    _buf[_n-1]=0;
    l = strlen(_buf);
  }
#else
  // vsnprintf() on non-win32, always null terminates
  int l = vsnprintf(_buf, _n, _fmt, va);
  if (l>=(int)_n)
    l=_n-1;
#endif
  va_end(va);
  return l;
}

// a _snprintf that returns >=0 when the string is null terminated and not truncated
// => callers must check the returned value
int _snprintfStrict(char* _buf, size_t _n, const char* _fmt, ...)
{
  va_list va;
  va_start(va, _fmt);
  *_buf='\0';
#if defined(_WIN32) && defined(_MSC_VER)
  int l = _vsnprintf(_buf, _n, _fmt, va);
  if (l < 0 || l >= (int)_n) {
    _buf[_n-1]=0;
    l = -1;
  }
#else
  // vsnprintf() on non-win32, always null terminates
  int l = vsnprintf(_buf, _n, _fmt, va);
  if (l>=(int)_n)
    l = -1;
#endif
  va_end(va);
  return l;
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


///////////////////////////////////////////////////////////////////////////////
// action helpers
///////////////////////////////////////////////////////////////////////////////

// corner-case note: "Custom: " can be removed after export/tweak-by-hand/re-import,
// this is not managed and leads to native problems anyway..
bool IsMacro(const char* _cmdName) {
	const char* custom = __localizeFunc("Custom","actions",0);
	int len = strlen(custom);
	return (_cmdName && (int)strlen(_cmdName)>len && !strncmp(_cmdName, custom, len) && _cmdName[len] == ':');
}

bool LearnAction(char* _idstrOut, int _idStrSz, const char* _expectedLocalizedSection)
{
	char section[SNM_MAX_SECTION_NAME_LEN] = "";
	int actionId, selItem = GetSelectedAction(section, SNM_MAX_SECTION_NAME_LEN, &actionId, _idstrOut, _idStrSz);
	if (strcmp(section, _expectedLocalizedSection))
		selItem = -1;
	switch (selItem)
	{
		case -2:
			MessageBox(GetMainHwnd(), __LOCALIZE("The column 'Custom ID' is not displayed in the Actions window!\n(to display it, right click on its table header > show action IDs)","sws_mbox"), __LOCALIZE("S&M - Error","sws_mbox"), MB_OK);
			return false;
		case -1: {
			char msg[256];
			_snprintfSafe(msg, sizeof(msg), __LOCALIZE_VERFMT("Actions window not opened or section '%s' not selected or no selected action!","sws_mbox"), _expectedLocalizedSection);
			MessageBox(GetMainHwnd(), msg, __LOCALIZE("S&M - Error","sws_mbox"), MB_OK);
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
		if (!_stricmp(_section, __localizeFunc("Main","accel_sec",0)) || !strcmp(_section, __localizeFunc("Main (alt recording)","accel_sec",0)))
			lstrcpyn(_sectionURL, "ALR_Main", _sectionURLSize);
		else if (!_stricmp(_section, __localizeFunc("Media explorer","accel_sec",0)))
			lstrcpyn(_sectionURL, "ALR_MediaExplorer", _sectionURLSize);
		else if (!_stricmp(_section, __localizeFunc("MIDI Editor","accel_sec",0)))
			lstrcpyn(_sectionURL, "ALR_MIDIEditor", _sectionURLSize);
		else if (!_stricmp(_section, __localizeFunc("MIDI Event List Editor","accel_sec",0)))
			lstrcpyn(_sectionURL, "ALR_MIDIEvtList", _sectionURLSize);
		else if (!_stricmp(_section, __localizeFunc("MIDI Inline Editor","accel_sec",0)))
			lstrcpyn(_sectionURL, "ALR_MIDIInline", _sectionURLSize);
		else
			return false;
	}
	else
		lstrcpyn(_sectionURL, _section, _sectionURLSize);
	return true;
}


///////////////////////////////////////////////////////////////////////////////
// other util funcs
///////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#define FNV64_IV ((WDL_UINT64)(0xCBF29CE484222325i64))
#else
#define FNV64_IV ((WDL_UINT64)(0xCBF29CE484222325LL))
#endif

WDL_UINT64 FNV64(WDL_UINT64 h, const unsigned char* data, int sz)
{
  int i;
  for (i=0; i < sz; ++i)
  {
#ifdef _WIN32
    h *= (WDL_UINT64)0x00000100000001B3i64;
#else
    h *= (WDL_UINT64)0x00000100000001B3LL;
#endif
    h ^= data[i];
  }
  return h;
}

// _strOut[65] by definition..
bool FNV64(const char* _strIn, char* _strOut)
{
	WDL_UINT64 h = FNV64_IV;
	const char *p=_strIn;
	while (*p)
	{
		char c = *p++;
		if (c == '\\')
		{
			if (*p == '\\'||*p == '"' || *p == '\'') h=FNV64(h,(unsigned char *)p,1);
			else if (*p == 'n') h=FNV64(h,(unsigned char *)"\n",1);
			else if (*p == 'r') h=FNV64(h,(unsigned char *)"\r",1);
			else if (*p == 't') h=FNV64(h,(unsigned char *)"\t",1);
			else if (*p == '0') h=FNV64(h,(unsigned char *)"",1);
			else return false;
			p++;
		}
		else
			h=FNV64(h,(unsigned char *)&c,1);
	}
	h=FNV64(h,(unsigned char *)"",1);
	return (_snprintfStrict(_strOut, 65, "%08X%08X",(int)(h>>32),(int)(h&0xffffffff)) > 0);
}

