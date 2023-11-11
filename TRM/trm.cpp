/******************************************************************************
/ trm.cpp
/
/ Copyright (c) 2023 Ték Róbert Máté
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
#include "trm.h"

#include <chrono>
#include <string>
#include <vector>

int TRM_Init()
{
    return 1;
}

namespace {

class AutoTimer {
    using clock = std::chrono::steady_clock;

    clock::time_point begin;
    std::string       name;

public:
    AutoTimer(const char* name) :
        begin(clock::now()),
        name(name)
    {}

    AutoTimer(AutoTimer&&) noexcept = default;
    AutoTimer& operator=(AutoTimer&&) noexcept = default;

    ~AutoTimer()
    {
        using namespace std::chrono;
        const auto end      = clock::now();
        const auto duration = duration_cast<milliseconds>(end-begin);

        ShowConsoleMsg(name.c_str());

        char buf[64];
        sprintf(buf, " took %d ms\n", static_cast<int>(duration.count()));
        ShowConsoleMsg(buf);
    }
};

std::vector<AutoTimer> g_runningTimers;

} // namespace {

void TRM_DEV_TimerPush(const char* mesurePointName)
{
    g_runningTimers.emplace_back(mesurePointName);
}

void TRM_DEV_TimerPop()
{
    if (g_runningTimers.empty())
        return;

    g_runningTimers.erase(--g_runningTimers.end());
}
