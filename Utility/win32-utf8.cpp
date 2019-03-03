/******************************************************************************
/ win32-utf8.cpp
/
/ Copyright (c) 2017 Christian Fillion
/ https://cfillion.ca
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
#include "../SnM/SnM.h"

#ifndef _WIN32
#  error This file should not be built on non-Windows systems.
#endif

static_assert(GetPrivateProfileString == GetPrivateProfileStringUTF8,
 "The current version of WDL does not override GetPrivateProfileString"
 " to GetPrivateProfileStringUTF8. Update WDL.");

using namespace std;

static string g_resourcePath;

static wstring widen(const char *input, const UINT codepage = CP_UTF8, const int size = -1)
{
  const int wsize = MultiByteToWideChar(codepage, 0, input, size, nullptr, 0) - 1;

  wstring output(wsize, 0);
  MultiByteToWideChar(codepage, 0, input, size, &output[0], wsize);

  return output;
}

static string narrow(const wchar_t *input)
{
  const int size = WideCharToMultiByte(CP_UTF8, 0,
    input, -1, nullptr, 0, nullptr, nullptr) - 1;

  string output(size, 0);
  WideCharToMultiByte(CP_UTF8, 0, input, -1, &output[0], size, nullptr, nullptr);

  return output;
}

static int strlistlen(const char *list)
{
  int size = 1;
  const char *p = list;

  while(*p) {
    const int segmentLen = strlen(p) + 1;
    p += segmentLen;
    size += segmentLen;
  }

  return size;
}

#undef GetResourcePath
const char *GetResourcePathUTF8()
{
  if(g_resourcePath.empty()) {
    const char *rcPath = GetResourcePath();

    // Convert from the current system codepage to UTF-8 for backward
    // compatibility with older versions of REAPER (#934).
    //
    // 5.70 onward: GetResourcePath is always UTF-8
    // 5.60 - 5.62: GetResourcePath is ANSI if possible, UTF-8 otherwise
    // up to 5.52:  GetResourcePath is always ANSI

    if(atof(GetAppVersion()) < 5.70 && !WDL_HasUTF8(rcPath))
      g_resourcePath = narrow(widen(rcPath, CP_ACP).c_str());
    else
      g_resourcePath = rcPath;
  }

  return g_resourcePath.c_str();
}

DWORD GetPrivateProfileSectionUTF8(LPCTSTR appName, LPTSTR ret, DWORD size, LPCTSTR fileName)
{
  // Using the ANSI version of the API when the ini filename doesn't contain
  // Unicode to allow reading and writing full UTF-8 contents. This matches the
  // behavior of the other *UTF8 wrappers in WDL's win32-utf8 (and thus REAPER).
  //
  // Characters outside of the ANSI code page are replaced with '?' when using
  // the wide version to the API unless the ini file already contains UTF-16.
  // Writing ini files as UTF-16 would likely make them unportable with macOS
  // and Linux.
  //
  // See <https://forum.cockos.com/showthread.php?p=1927329>.

  if(!WDL_HasUTF8(fileName))
    return GetPrivateProfileSectionA(appName, ret, size, fileName);

  wchar_t wideRet[SNM_MAX_INI_SECTION];
  const int wideSize = GetPrivateProfileSectionW(widen(appName).c_str(), wideRet,
    SNM_MAX_INI_SECTION, widen(fileName).c_str());

  return WideCharToMultiByte(CP_UTF8, 0, wideRet, wideSize, ret, size, nullptr, nullptr);
}

BOOL WritePrivateProfileSectionUTF8(LPCTSTR appName, LPCTSTR string, LPCTSTR fileName)
{
  if(!WDL_HasUTF8(fileName))
    return WritePrivateProfileSectionA(appName, string, fileName);

  const wstring &wideString = widen(string, CP_UTF8, strlistlen(string));

  return WritePrivateProfileSectionW(widen(appName).c_str(),
    wideString.c_str(), widen(fileName).c_str());
}

HWND FindWindowExUTF8(HWND parent, HWND after, const char *className, const char *title)
{
  return FindWindowExW(parent, after,
    className ? widen(className).c_str() : nullptr, title ? widen(title).c_str() : nullptr);
}
