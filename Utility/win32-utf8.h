#pragma once

#include <windows.h>

DWORD GetPrivateProfileSectionUTF8(LPCTSTR appStr, LPTSTR retStr, DWORD nSize, LPCTSTR fnStr);
BOOL WritePrivateProfileSectionUTF8(LPCTSTR appStr, LPCTSTR str, LPCTSTR fnStr);

#ifdef GetPrivateProfileSection
#undef GetPrivateProfileSection
#endif
#define GetPrivateProfileSection GetPrivateProfileSectionUTF8

#ifdef WritePrivateProfileSection
#undef WritePrivateProfileSection
#endif
#define WritePrivateProfileSection WritePrivateProfileSectionUTF8
