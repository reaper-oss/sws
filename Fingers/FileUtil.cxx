#include "stdafx.h"

#include "FileUtil.hxx"
#include "windows.h"
#include "commdlg.h"
#include <sstream>
#include "reaper_plugin_functions.h"

bool GetSaveFile(char *title, const char *extensions, const char *defaultExtension, std::string &fileName, const char *defaultFileName)
{
	OPENFILENAMEA ofn;
    char szFileName[MAX_PATH] = "";
	if (defaultFileName)
		strcpy(szFileName, defaultFileName);

    ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetMainHwnd();
	ofn.lpstrTitle = title;
    ofn.lpstrFilter = extensions;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
	ofn.lpstrDefExt = defaultExtension;
    ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY;

	if(GetSaveFileNameA(&ofn)) {
		fileName = ofn.lpstrFile;
		return true;
	}
	return false;
}

bool GetOpenFile(char *title, const char *extensions, const char *defaultExtension, std::string &fileName)
{
	OPENFILENAMEA ofn;
    char szFileName[MAX_PATH] = "";

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn); 
    ofn.hwndOwner = GetMainHwnd();
	ofn.lpstrTitle = title;
    ofn.lpstrFilter = extensions;
    ofn.lpstrFile = szFileName;
	ofn.lpstrDefExt = defaultExtension;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY;

	if(GetOpenFileNameA(&ofn)) {
		fileName = ofn.lpstrFile;
		return true;
	}
	return false;
}

static std::string GetBeforeToken(std::string &str, std::string &token)
{
	std::string::size_type i = str.find_first_of(token);
	if(i != std::string::npos)
		return str.substr(0, i);
}

static void EraseBeforeToken(std::string &str, std::string &token, bool inclusive)
{
	int offset = 0;
	if(inclusive)
		offset++;
	std::string::size_type i = str.find_first_of(token);
	if(i != std::string::npos)
		str = str.substr(i);
}

static int CountCharInString(std::string &str, char c)
{
	int len = str.length();
	int count = 0;
	int i = 0;
	while(i < len) {
		if(str[i] == c)
			count++;
		i++;
	}
	return count;
}

static void RemoveCommonFromLeft(std::string &str1, std::string &str2)
{
	int len = max(str1.length(), str2.length());
	int i = 0;
	while(str1[i] == str2[i] && i < len) i++;
	str1 = str1.substr(i);	
	str2 = str2.substr(i);
}

std::string GetRelativePath(std::string &parentPath, std::string &childPath)
{
	if(parentPath[0] != childPath[0])
		return childPath;
	std::string parent = parentPath;
	std::string child = childPath;
	std::ostringstream oss;
	RemoveCommonFromLeft(parent, child);
	int count = CountCharInString(parent, '\\');
	if(count == 0)
		oss << ".\\";
	else {
		for(int i = 0; i < count; i++)
			oss << "..\\";
	}
	oss << child;
	return oss.str();
}

std::string getConfigFilePath()
{
	char cBuf[256];
	strncpy(cBuf, get_ini_file(), 256);
#ifdef WIN32
	char* pC = strrchr(cBuf, '\\');
#else
	char* pC = strrchr(cBuf, '/');
#endif
	if (!pC)
		return 0;
	strcpy(pC+1, "fng_settings.xml");
	return cBuf;
}