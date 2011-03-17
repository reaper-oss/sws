#ifndef __FILEUTIL_H
#define __FILEUTIL_H
#include <string>

std::string GetRelativePath(std::string &parentPath, std::string &childPath);
std::string getConfigFilePath();

#endif