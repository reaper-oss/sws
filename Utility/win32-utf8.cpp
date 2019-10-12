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
#include "SnM/SnM.h"

#ifndef _WIN32
#  error This file should not be built on non-Windows systems.
#endif

static_assert(GetPrivateProfileString == GetPrivateProfileStringUTF8,
 "The current version of WDL does not override GetPrivateProfileString"
 " to GetPrivateProfileStringUTF8. Update WDL.");

#define widen_cstr(cstr) win32::widen(cstr).c_str()

std::wstring win32::widen(const char *input, const UINT codepage, const int size)
{
  const int wsize = MultiByteToWideChar(codepage, 0, input, size, nullptr, 0) - 1;

  std::wstring output(wsize, 0);
  MultiByteToWideChar(codepage, 0, input, size, &output[0], wsize);

  return output;
}

std::string win32::narrow(const wchar_t *input)
{
  const int size = WideCharToMultiByte(CP_UTF8, 0,
    input, -1, nullptr, 0, nullptr, nullptr) - 1;

  std::string output(size, 0);
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
  const int wideSize = GetPrivateProfileSectionW(widen_cstr(appName), wideRet,
    SNM_MAX_INI_SECTION, widen_cstr(fileName));

  return WideCharToMultiByte(CP_UTF8, 0, wideRet, wideSize, ret, size, nullptr, nullptr);
}

BOOL WritePrivateProfileSectionUTF8(LPCTSTR appName, LPCTSTR string, LPCTSTR fileName)
{
  if(!WDL_HasUTF8(fileName))
    return WritePrivateProfileSectionA(appName, string, fileName);

  const std::wstring &wideString = win32::widen(string, CP_UTF8, strlistlen(string));

  return WritePrivateProfileSectionW(
    widen_cstr(appName), wideString.c_str(), widen_cstr(fileName));
}

HWND FindWindowExUTF8(HWND parent, HWND after, const char *className, const char *title)
{
  return FindWindowExW(parent, after,
    className ? widen_cstr(className) : nullptr, title ? widen_cstr(title) : nullptr);
}
