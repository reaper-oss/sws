/******************************************************************************
/ SnM_Util.h
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

//#pragma once

#ifndef _SNM_UTIL_H_
#define _SNM_UTIL_H_

#include "../MarkerList/MarkerListClass.h"


const char* GetFileRelativePath(const char* _fn);
const char* GetFileExtension(const char* _fn);
void GetFilenameNoExt(const char* _fullFn, char* _fn, int _fnSz);
const char* GetFilenameWithExt(const char* _fullFn);
void Filenamize(char* _fnInOut, int _fnInOutSz);
bool IsValidFilenameErrMsg(const char* _fn, bool _errMsg);
bool FileExistsErrMsg(const char* _fn, bool _errMsg=true);
bool SNM_DeleteFile(const char* _filename, bool _recycleBin);
bool SNM_CopyFile(const char* _destFn, const char* _srcFn);
bool BrowseResourcePath(const char* _title, const char* _dir, const char* _fileFilters, char* _fn, int _fnSize, bool _wantFullPath = false);
void GetShortResourcePath(const char* _resSubDir, const char* _fullFn, char* _shortFn, int _shortFnSize);
void GetFullResourcePath(const char* _resSubDir, const char* _shortFn, char* _fullFn, int _fullFnSize);
bool LoadChunk(const char* _fn, WDL_FastString* _chunkOut, bool _trim = true, int _approxMaxlen = 0);
bool SaveChunk(const char* _fn, WDL_FastString* _chunk, bool _indent);
WDL_HeapBuf* LoadBin(const char* _fn);
bool SaveBin(const char* _fn, const WDL_HeapBuf* _hb);
#ifdef _SNM_MISC
bool TranscodeFileToFile64(const char* _outFn, const char* _inFn);
#endif
WDL_HeapBuf* TranscodeStr64ToHeapBuf(const char* _str64);
bool GenerateFilename(const char* _dir, const char* _name, const char* _ext, char* _updatedFn, int _updatedSz);
void ScanFiles(WDL_PtrList<WDL_String>* _files, const char* _initDir, const char* _ext, bool _subdirs);
void StringToExtensionConfig(WDL_FastString* _str, ProjectStateContext* _ctx);
void ExtensionConfigToString(WDL_FastString* _str, ProjectStateContext* _ctx);
void SaveIniSection(const char* _iniSectionName, WDL_FastString* _iniSection, const char* _iniFn);
void UpdatePrivateProfileSection(const char* _oldAppName, const char* _newAppName, const char* _iniFn, const char* _newIniFn = NULL);
void UpdatePrivateProfileString(const char* _appName, const char* _oldKey, const char* _newKey, const char* _iniFn, const char* _newIniFn = NULL);
void SNM_UpgradeIniFiles();
bool SNM_SetProjectMarker(ReaProject* _proj, int _num, bool _isrgn, double _pos, double _rgnend, const char* _name, int _color = 0);
bool SNM_GetProjectMarkerName(ReaProject* _proj, int _num, bool _isrgn, WDL_FastString* _name);
int FindMarkerRegion(double _pos, int _flags, int* _idOut = NULL);
int MakeMarkerRegionId(int _num, bool _isRgn);
int GetMarkerRegionIdFromIndex(int _idx);
int GetMarkerRegionIndexFromId(int _id);
int GetMarkerRegionNumFromId(int _id);
bool IsRegion(int _id);
int EnumMarkerRegionDesc(int _idx, char* _descOut, int _outSz, int _flags, bool _wantsName, bool _wantsTime = true);
void FillMarkerRegionMenu(HMENU _menu, int _msgStart, int _flags, UINT _uiState = 0);
int SNM_SnapToMeasure(double _pos);
void TranslatePos(double _pos, int* _h, int* _m = NULL, int* _s = NULL, int* _ms = NULL);
double SeekPlay(double _pos, bool _seek = true, bool _moveView = false);
int _snprintfSafe(char* _buf, size_t _n, const char* _fmt, ...);
int _snprintfStrict(char* _buf, size_t _n, const char* _fmt, ...);
bool GetStringWithRN(const char* _bufSrc, char* _buf, int _bufSize);
const char* FindFirstRN(const char* _str);
char* ShortenStringToFirstRN(char* _str);
void Replace02d(char* _str, char _replaceCh);
bool IsMacro(const char* _actionName);
bool GetSectionNameAsURL(bool _alr, const char* _section, char* _sectionURL, int _sectionURLSize);
#ifdef _SNM_MISC
WDL_UINT64 FNV64(WDL_UINT64 h, const unsigned char* data, int sz);
bool FNV64(const char* _strIn, char* _strOut);
#endif


class MarkerRegion : public MarkerItem {
public:
	MarkerRegion(bool _bReg, double _dPos, double _dRegEnd, const char* _cName, int _num, int _color)
		: MarkerItem(_bReg, _dPos, _dRegEnd, _cName, _num, _color) { m_id=MakeMarkerRegionId(_num, _bReg); }
	int GetId() { return m_id; }
protected:
	int m_id;
};


#endif
