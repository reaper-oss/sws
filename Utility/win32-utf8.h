#pragma once

#include <windows.h>

const char *GetResourcePathUTF8();
#define GetResourcePath() GetResourcePathUTF8()

DWORD GetPrivateProfileSectionUTF8(LPCTSTR appStr, LPTSTR retStr, DWORD nSize, LPCTSTR fnStr);

#ifdef GetPrivateProfileSection
#  undef GetPrivateProfileSection
#endif
#define GetPrivateProfileSection GetPrivateProfileSectionUTF8

BOOL WritePrivateProfileSectionUTF8(LPCTSTR appStr, LPCTSTR str, LPCTSTR fnStr);

#ifdef WritePrivateProfileSection
#  undef WritePrivateProfileSection
#endif
#define WritePrivateProfileSection WritePrivateProfileSectionUTF8
