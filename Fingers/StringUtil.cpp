#include "stdafx.h"
#include "StringUtil.h"

StringVector::StringVector(const std::string& inStr)
{
    mString = inStr;
    std::string::size_type posChar = inStr.find_first_not_of(' ');
    while(true) {

    if(posChar == std::string::npos)
        return;

    std::string::size_type posSpace = inStr.find_first_of(' ', posChar);

    SubStringIndex index;
    index.offset = posChar;
    // end of string
    if(posSpace == std::string::npos) {
        index.length = inStr.length() - posChar;
        mIndexes.push_back(index);
        return;
    } else {
        index.length = posSpace - posChar;
        mString[posSpace] = 0;
        mIndexes.push_back(index);
    }
    posChar = inStr.find_first_not_of(' ', posSpace + 1);
    };
}

bool StringVector::empty() const
{
    return mIndexes.empty();
}

const char* StringVector::at(int index) const
{
    return &mString.c_str()[mIndexes.at(index).offset];
}

unsigned int StringVector::size() const
{
    return (int)mIndexes.size();
}
