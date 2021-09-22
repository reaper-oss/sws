/******************************************************************************
/ stdafx.h
/
/ Copyright (c) 2011 and later Tim Payne (SWS), Jeffos
/
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

// Yeah, I know "AFX" is for MFC, and we don't use MFC in this project.
// Force of habit. :)

#pragma once

#define STRICT
#ifdef _WIN32
#  include <winsock2.h> // must be inculded before windows.h, see OscPkt/udp.h
#  include <windows.h>
#  include <windowsx.h>
#  include <process.h>
#  include <shlwapi.h>
#  include <shlobj.h>
#  include <crtdbg.h>
#  include <commctrl.h>
#else
#  include <WDL/swell/swell.h>
#  include <sys/time.h>
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
#include <cstdlib>
#include <cmath>
#include <vector>
#include <algorithm>
#include <bitset>
#include <iomanip>
#include <string>
#include <list>
#include <map>
#include <set>
#include <numeric>
#include <ctime>
#include <limits>

#include <reaper_plugin.h>
#include "reaper/sws_rpf_wrapper.h"
#include "reaper/icontheme.h"

#include "WDL/wdlcstring.h"
#undef snprintf
#undef vsnprintf

#include "WDL/wdltypes.h"
#include "WDL/ptrlist.h"
#include "WDL/wdlstring.h"
#include "WDL/heapbuf.h"
#include "WDL/db2val.h"
#include "WDL/wingui/wndsize.h"
#include "WDL/lice/lice.h"
#include "WDL/dirscan.h"
#include "WDL/wingui/virtwnd.h"
#include "WDL/wingui/virtwnd-controls.h"
#include "WDL/assocarray.h"
#include "WDL/win32_utf8.h"
#include "WDL/lineparse.h"
#include "WDL/MersenneTwister.h"
#include "WDL/fileread.h"
#include "WDL/filewrite.h" // #647

// Headers that are used "enough" to be worth of being precompiled,
// at the expense of needing recompile of the headers on change
#include "Utility/configvar.h"
#include "Utility/SectionLock.h"
#include "sws_util.h"
#include "sws_wnd.h"
#include "Menus.h"
#include "resource.h"
#include "Xenakios/XenakiosExts.h"
#include "ObjectState/ObjectState.h"
#include "ObjectState/TrackFX.h"
#include "ObjectState/TrackSends.h"
#include "SnM/SnM_ChunkParserPatcher.h"
#include "Padre/padreUtils.h"
#include "Padre/padreMidi.h"
#include "Padre/padreMidiItemProcBase.h"
#include "Breeder/BR_Timer.h"
#include "Utility/envelope.hpp"
#include "Utility/hidpi.h"
#include "Utility/win32-utf8.h"
