#pragma once

#include <windows.h>

const char *GetResourcePathUTF8();
#define GetResourcePath() GetResourcePathUTF8()

DWORD GetPrivateProfileSectionUTF8(LPCTSTR appName, LPTSTR retStr, DWORD nSize, LPCTSTR fileName);
#ifdef GetPrivateProfileSection
#  undef GetPrivateProfileSection
#endif
#define GetPrivateProfileSection GetPrivateProfileSectionUTF8

BOOL WritePrivateProfileSectionUTF8(LPCTSTR appName, LPCTSTR str, LPCTSTR fileName);
#ifdef WritePrivateProfileSection
#  undef WritePrivateProfileSection
#endif
#define WritePrivateProfileSection WritePrivateProfileSectionUTF8

HWND FindWindowExUTF8(HWND parent, HWND after, const char *className, const char *title);
#ifdef FindWindowEx
#  undef FindWindowEx
#endif
#define FindWindowEx FindWindowExUTF8
