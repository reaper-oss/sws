/******************************************************************************
/ cfillion.cpp
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
#include "SnM\SnM.h"

#ifdef _WIN32
#  define widen_cstr(cstr) widen(cstr).c_str()
#else
#  error This file should not be built on non-Windows systems.
#endif

static_assert(GetPrivateProfileString == GetPrivateProfileStringUTF8,
  "The current version of WDL does not override GetPrivateProfileString to GetPrivateProfileStringUTF8. Update WDL.");

using namespace std;

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

DWORD GetPrivateProfileSectionUTF8(LPCTSTR appStr, LPTSTR retStr, DWORD nSize, LPCTSTR fnStr)
{
  wchar_t ret[SNM_MAX_INI_SECTION];
  const int size = GetPrivateProfileSectionW(widen_cstr(appStr), ret,
    SNM_MAX_INI_SECTION, widen_cstr(fnStr));

  return WideCharToMultiByte(CP_UTF8, 0, ret, size, retStr, nSize, nullptr, nullptr);
}

BOOL WritePrivateProfileSectionUTF8(LPCTSTR appStr, LPCTSTR str, LPCTSTR fnStr)
{
  int size = 1;
  const char *p = str;
  while(*p) {
    const int segmentLen = strlen(p) + 1;
    p += segmentLen;
    size += segmentLen;
  }

  return WritePrivateProfileSectionW(widen_cstr(appStr), widen(str, CP_UTF8, size).c_str(), widen_cstr(fnStr));
}
