#include "stdafx.h"

#include <vector>
#include <string>

#include "StringUtil.hxx"

std::vector<std::string>* stringTokenize(const std::string &inStr)
{
	std::vector<std::string> *tokens = new std::vector<std::string>;
	std::string::size_type i = 0;
	while(1) {
		std::string::size_type posSpace = inStr.find_first_of(' ', i);
		std::string::size_type posChar = inStr.find_first_not_of(' ',i );
		if(posSpace < posChar) {
			i = posChar;
			continue;
		}
		if(posChar == std::string::npos)
			return tokens;

		if(posSpace == std::string::npos) {
			tokens->push_back(inStr.substr(posChar));
			return tokens;
		}
		tokens->push_back(inStr.substr(posChar, posSpace - posChar));
		i = posSpace + 1;
	}
}