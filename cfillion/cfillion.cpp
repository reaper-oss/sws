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
#include "cfillion.hpp"

#ifdef _WIN32
static const unsigned int FORMAT = CF_UNICODETEXT;
#else
static const unsigned int FORMAT = CF_TEXT;
#endif

extern WDL_PtrList_DOD<WDL_FastString> g_script_strs;

void CF_SetClipboard(const char *buf)
{
#ifdef _WIN32
  const int length = MultiByteToWideChar(CP_UTF8, 0, buf, -1, nullptr, 0);
  const size_t size = length * sizeof(wchar_t);
#else
  const size_t size = strlen(buf) + 1;
#endif

  HANDLE mem = GlobalAlloc(GMEM_MOVEABLE, size);
#ifdef _WIN32
  MultiByteToWideChar(CP_UTF8, 0, buf, -1, (wchar_t *)GlobalLock(mem), length);
#else
  memcpy(GlobalLock(mem), buf, size);
#endif
  GlobalUnlock(mem);

  OpenClipboard(GetMainHwnd());
  EmptyClipboard();
  SetClipboardData(FORMAT, mem);
  CloseClipboard();
}

void CF_GetClipboard(char *buf, int bufSize)
{
  OpenClipboard(GetMainHwnd());
  HANDLE mem = GetClipboardData(FORMAT);

  if(void *data = GlobalLock(mem)) {
#ifdef _WIN32
    WideCharToMultiByte(CP_UTF8, 0, (const wchar_t *)data, -1,
      buf, bufSize - 1, nullptr, nullptr);

    // Insert a null terminator if the buffer is too small to hold the entire
    // clipboard data. WideCharToMultiByte inserts it at the end of the string
    // by itself when the buffer is big enough.
    buf[bufSize - 1] = 0;
#else
    snprintf(buf, bufSize, "%s", (const char *)data);
#endif
    GlobalUnlock(mem);
  }

  CloseClipboard();
}

const char *CF_GetClipboardBig(WDL_FastString *output)
{
  if(g_script_strs.Find(output) == -1)
    return NULL;

  OpenClipboard(GetMainHwnd());
  HANDLE mem = GetClipboardData(FORMAT);

  if(void *data = GlobalLock(mem)) {
#ifdef _WIN32
    const int size = WideCharToMultiByte(CP_UTF8, 0,
      (const wchar_t *)data, -1, nullptr, 0, nullptr, nullptr) - 1;

    output->SetLen(size);

    WideCharToMultiByte(CP_UTF8, 0, (const wchar_t *)data, -1,
      const_cast<char *>(output->Get()), size, nullptr, nullptr);
#else
    output->Set((const char *)data);
#endif
    GlobalUnlock(mem);
  }

  CloseClipboard();

  return output->Get();
}
