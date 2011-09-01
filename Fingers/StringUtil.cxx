#include "stdafx.h"
#include "StringUtil.hxx"

StringVector::StringVector(const std::string& inStr)
{
	mString = inStr;
	std::string::size_type i = inStr.find_first_not_of(' ');
	while(true) {
		std::string::size_type posSpace = inStr.find_first_of(' ', i);
		std::string::size_type posChar = inStr.find_first_not_of(' ',i );
		
		if(posChar == std::string::npos)
			return;

		if(posSpace == std::string::npos) {
			SubStringIndex index;
			index.offset = posChar;
			index.length = inStr.length() - posChar;
			mIndexes.push_back(index);
			return;
		}
		i = posSpace + 1;
		if (posSpace < posChar) {
			continue;
		}
		SubStringIndex index;
		index.offset = posChar;
		index.length = posSpace - posChar;
		mString[posSpace] = 0;
		mIndexes.push_back(index);
	};
}

bool StringVector::empty() const
{
	return mIndexes.empty();
}

const char* StringVector::atPtr(int index) const
{
	return &mString.c_str()[mIndexes.at(index).offset];
}

int StringVector::size() const
{
	return (int)mIndexes.size();
}

std::string StringVector::at(int index) const
{
	const SubStringIndex &subStringIndex = mIndexes.at(index);
	return mString.substr(subStringIndex.offset, subStringIndex.length);
}