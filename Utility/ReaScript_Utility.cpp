/******************************************************************************
/ ReaScript_Utilty.cpp
/
/ Copyright (c) 2022 ReaTeam
/
/ Permission is hereby granted, free of charge, to any person obtaining a copy
/ of this software and associated documentation files (the "Software"), to deal
/ in the Software without restriction, including without limitation the rights to
/ use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
/ of the Software, and to permit persons to whom the Software is furnished to
/ do so, subject to the following conditions:
/
/ The above copyright notice and this permission notice shall be included in all
/ copies or substantial portions of the Software.
/
/ THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
/ EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
/ OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
/ NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
/ HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
/ WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/ FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
/ OTHER DEALINGS IN THE SOFTWARE.
/
******************************************************************************/

#include "stdafx.h"
#include <cstring>

#include "ReaScript_Utility.hpp"

// from https://github.com/cfillion/reaimgui/blob/b31d7fa6cf317167d363dc7882a193cc03f90762/api/input.cpp#L27-L39
void CopyToBuffer(const char* value, char* buf, const size_t bufSize)
{
    int newSize{};
    const size_t valuestrlen = strlen(value);
    if (valuestrlen >= bufSize && realloc_cmd_ptr(&buf, &newSize, valuestrlen)) {
        // the buffer is no longer null-terminated after using realloc_cmd_ptr!
        std::memcpy(buf, value, newSize);
    }
    else {
        const size_t limit{ std::min(bufSize - 1, valuestrlen) };
        std::memcpy(buf, value, limit);
        buf[limit] = '\0';
    }
}
