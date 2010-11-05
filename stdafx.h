/******************************************************************************
/ stdafx.h
/
/ Copyright (c) 2010 Tim Payne (SWS)
/ http://www.standingwaterstudios.com/reaper
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

#pragma once

#define STRICT
#ifdef _WIN32
#include <windows.h>
#include <windowsx.h>
#include <process.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <crtdbg.h>
#include <commctrl.h>
#else
#include "../WDL/swell/swell.h"
#endif
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <time.h>
#include <float.h>
#include <sys/stat.h>

// stl
#include <sstream>
#include <fstream>
#include <iostream>
#include <cctype>
#include <vector>
#include <algorithm>
#include <bitset>
#include <iomanip>
#include <string>
#include <list>
#include <map>
#include <set>

#pragma warning(disable : 4996) // POSIX deprecation warnings
#pragma warning(disable : 4267) // size_t to int warnings
#include "../WDL/wdltypes.h"
// Temporary fix for out-of-date WDL
#include "../WDL/ptrlist.h"
#include "../WDL/wdlstring.h"
#include "../WDL/heapbuf.h"
#include "../WDL/db2val.h"
#include "../WDL/wingui/wndsize.h"
#include "../WDL/lice/lice.h"
#include "../WDL/dirscan.h"
#include "../WDL/wingui/virtwnd.h"
#include "../WDL/wingui/virtwnd-controls.h"
#include "../WDL/assocarray.h"


#include "../WDL/win32_utf8.h"
#include "../WDL/lineparse.h"
#pragma warning(default : 4996)
#pragma warning(default : 4267)

#include "reaper/icontheme.h"
#include "reaper/reaper_plugin.h"
#include "reaper/sws_rpf_wrapper.h"
#include "Utility/SectionLock.h"
#include "sws_util.h"
#include "sws_wnd.h"
#include "resource.h"
#include "Xenakios/XenakiosExts.h"
#include "ObjectState/ObjectState.h"
#include "ObjectState/TrackFX.h"
#include "ObjectState/TrackSends.h"

// Padre
#include "Padre/padreUtils.h"
#include "Padre/padreMidi.h"
#include "Padre/padreMidiItemProcBase.h"
