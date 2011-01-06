#ifndef __FILEUTIL_H
#define __FILEUTIL_H
#include <string>

bool GetSaveFile(char *title, const char *extensions, const char *defaultExtension, std::string &fileName, const char *defaultFileName = 0);
bool GetOpenFile(char *title, const char *extensions, const char *defaultExtension, std::string &fileName);
std::string GetRelativePath(std::string &parentPath, std::string &childPath);

std::string getConfigFilePath();

#endif