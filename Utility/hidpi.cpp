/******************************************************************************
/ hidpi.cpp
/
/ Copyright (c) 2020 Christian Fillion
/ https://cfillion.ca
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

#ifdef _WIN32
#include <ShellScalingApi.h> // GetDpiForMonitor
#include "win32-import.h"
#endif // _WIN32

unsigned int hidpi::GetDpiForPoint(const POINT &point)
{
#ifdef _WIN32
  // not linking against shcore.dll for compatibility with Windows older than 8.1
  static win32::import<decltype(GetDpiForMonitor)> _GetDpiForMonitor
    {"SHCore.dll", "GetDpiForMonitor"};

  // using MONITOR_DEFAULTTONEAREST to match GetMonitorRectFromPoint from BR_Util
  HMONITOR monitor = MonitorFromPoint(point, MONITOR_DEFAULTTONEAREST);

  if (_GetDpiForMonitor && monitor) {
    unsigned int dpiX, dpiY;
    if (S_OK == _GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY))
      return (dpiX * 256) / 96; // same scaling as WDL_WndSizer::calc_dpi
    else
      return 256;
  }
#endif

  return 0;
}

unsigned int hidpi::GetDpiForWindow(HWND window)
{
  // returns 0 if Per-Monitor v2 DPI awareness is disabled for the thread
  return WDL_WndSizer::calc_dpi(window);
}

bool hidpi::IsDifferentDpi(const unsigned int a, const unsigned int b)
{
  return a && b && a != b;
}
