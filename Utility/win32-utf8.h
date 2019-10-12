#pragma once

// extracted from ReaPack
namespace win32 {
#ifdef _WIN32
  std::wstring widen(const char *, UINT codepage = CP_UTF8, int size = -1);
  inline std::wstring widen(const std::string &str) { return widen(str.c_str()); }

  std::string narrow(const wchar_t *);
  inline std::string narrow(const std::wstring &str) { return narrow(str.c_str()); }
#else
  inline std::string widen(const char *s, UINT = 0) { return s; }
  inline std::string widen(const std::string &s) { return s; }

  inline std::string narrow(const char *s) { return s; }
  inline std::string narrow(const std::string &s) { return s; }
#endif
}

#ifdef _WIN32
#include <windows.h>

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

#endif // _WIN32
