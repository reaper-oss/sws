/******************************************************************************
/ js_swell_ReaScript.cpp
/
/ Copyright (c) 2018 The Human Race
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


#include "stdafx.h"
#include <string>
#include <stdlib.h>
#include <errno.h>
#include "js_swell_ReaScript.h"



namespace Julian
{
	// While windows are being enumerated, this struct stores the information
	//		such as the title text that must be matched, as well as the list of all matching window HWNDs.
	struct EnumWindowsStruct
	{
		const char* target; // Title text that must be matched
		bool		exact;  // Match exactly, or substring?
		char*		temp;
		unsigned int	tempLen;
		HWND		hwnd;  // HWND to find
		char*		hwndString; // List of all matching HWNDs
		unsigned int	hwndLen;
	};
	const int TEMP_LEN = 65;	// For temporary storage of pointer strings.
	const int API_LEN = 1024;	// Maximum length of strings passed between Lua and API.
	const int EXT_LEN = 16000;	// What is the maximum length of ExtState strings? 

	const int ERR_ALREADY_INTERCEPTED = 0;
	const int ERR_NOT_WINDOW = -1;
	const int ERR_PARSING = -2;
	const int ERR_ORIGPROC = -3;


	// This struct is used to store the data of intercepted messages.
	//		In addition to the standard wParam and lParam, a unique timestamp is added.
	struct sMsgData
	{
		bool passthrough;
		double time;
		WPARAM wParam;
		LPARAM lParam;
	};

	// This struct and map store the data of each HWND that is being intercepted.
	// (Each window can only be intercepted by one script at a time.)
	// For each window, three bitfields summarize the up/down states of most of the keyboard. 
	// UPDATE: Apparently, REAPER's windows don't receive keystrokes via the message queue, and the keyboard can therefore not be intercepted.  
	struct sWindowData
	{
		WNDPROC origProc;
		std::map<UINT, sMsgData> messages;
		//int keysBitfieldBACKtoHOME	= 0; // Virtual key codes VK_BACK (backspace) to VK_HOME
		//int keysBitfieldAtoSLEEP	= 0;
		//int keysBitfieldLEFTto9		= 0;
	};
	const bool BLOCK = false;

	////////////////////////////////////////////////////////////////////////////
	// THE MAIN MAP FOR ALL INTERCEPTS
	// Each window that is being intercepted, will be mapped to its data struct.
	std::map <HWND, sWindowData> mapWindowToData;



	// This map contains all the WM_ messages in swell-types.h. These can be assumed to be valid cross-platform.
	std::map<std::string, UINT> mapWM_toMsg
	{
		pair<std::string, UINT>("WM_CREATE", 0x0001),
		pair<std::string, UINT>("WM_DESTROY", 0x0002),
		pair<std::string, UINT>("WM_MOVE", 0x0003),
		pair<std::string, UINT>("WM_SIZE", 0x0005),
		pair<std::string, UINT>("WM_ACTIVATE", 0x0006),
		//pair<std::string, UINT>("WM_SETREDRAW", 0x000B), // Not implemented cross-platform
		//pair<std::string, UINT>("WM_SETTEXT", 0x000C),
		pair<std::string, UINT>("WM_PAINT", 0x000F),
		pair<std::string, UINT>("WM_CLOSE", 0x0010),
		pair<std::string, UINT>("WM_ERASEBKGND", 0x0014),
		pair<std::string, UINT>("WM_SHOWWINDOW", 0x0018),
		pair<std::string, UINT>("WM_ACTIVATEAPP", 0x001C),
		pair<std::string, UINT>("WM_SETCURSOR", 0x0020),
		pair<std::string, UINT>("WM_MOUSEACTIVATE", 0x0021),
		pair<std::string, UINT>("WM_GETMINMAXINFO", 0x0024),
		pair<std::string, UINT>("WM_DRAWITEM", 0x002B),
		pair<std::string, UINT>("WM_SETFONT", 0x0030),
		pair<std::string, UINT>("WM_GETFONT", 0x0031),
		//pair<std::string, UINT>("WM_GETOBJECT", 0x003D),
		pair<std::string, UINT>("WM_COPYDATA", 0x004A),
		pair<std::string, UINT>("WM_NOTIFY", 0x004E),
		pair<std::string, UINT>("WM_CONTEXTMENU", 0x007B),
		pair<std::string, UINT>("WM_STYLECHANGED", 0x007D),
		pair<std::string, UINT>("WM_DISPLAYCHANGE", 0x007E),
		pair<std::string, UINT>("WM_NCDESTROY", 0x0082),
		pair<std::string, UINT>("WM_NCCALCSIZE", 0x0083),
		pair<std::string, UINT>("WM_NCHITTEST", 0x0084),
		pair<std::string, UINT>("WM_NCPAINT", 0x0085),
		pair<std::string, UINT>("WM_NCMOUSEMOVE", 0x00A0),
		pair<std::string, UINT>("WM_NCLBUTTONDOWN", 0x00A1),
		pair<std::string, UINT>("WM_NCLBUTTONUP", 0x00A2),
		pair<std::string, UINT>("WM_NCLBUTTONDBLCLK", 0x00A3),
		pair<std::string, UINT>("WM_NCRBUTTONDOWN", 0x00A4),
		pair<std::string, UINT>("WM_NCRBUTTONUP", 0x00A5),
		pair<std::string, UINT>("WM_NCRBUTTONDBLCLK", 0x00A6),
		pair<std::string, UINT>("WM_NCMBUTTONDOWN", 0x00A7),
		pair<std::string, UINT>("WM_NCMBUTTONUP", 0x00A8),
		pair<std::string, UINT>("WM_NCMBUTTONDBLCLK", 0x00A9),
		pair<std::string, UINT>("WM_KEYFIRST", 0x0100),
		pair<std::string, UINT>("WM_KEYDOWN", 0x0100),
		pair<std::string, UINT>("WM_KEYUP", 0x0101),
		pair<std::string, UINT>("WM_CHAR", 0x0102),
		pair<std::string, UINT>("WM_DEADCHAR", 0x0103),
		pair<std::string, UINT>("WM_SYSKEYDOWN", 0x0104),
		pair<std::string, UINT>("WM_SYSKEYUP", 0x0105),
		pair<std::string, UINT>("WM_SYSCHAR", 0x0106),
		pair<std::string, UINT>("WM_SYSDEADCHAR", 0x0107),
		pair<std::string, UINT>("WM_KEYLAST", 0x0108),
		pair<std::string, UINT>("WM_INITDIALOG", 0x0110),
		pair<std::string, UINT>("WM_COMMAND", 0x0111),
		pair<std::string, UINT>("WM_SYSCOMMAND", 0x0112),
		pair<std::string, UINT>("SC_CLOSE", 0xF060),
		pair<std::string, UINT>("WM_TIMER", 0x0113),
		pair<std::string, UINT>("WM_HSCROLL", 0x0114),
		pair<std::string, UINT>("WM_VSCROLL", 0x0115),
		pair<std::string, UINT>("WM_INITMENUPOPUP", 0x0117),
		pair<std::string, UINT>("WM_GESTURE", 0x0119),
		pair<std::string, UINT>("WM_MOUSEFIRST", 0x0200),
		pair<std::string, UINT>("WM_MOUSEMOVE", 0x0200),
		pair<std::string, UINT>("WM_LBUTTONDOWN", 0x0201),
		pair<std::string, UINT>("WM_LBUTTONUP", 0x0202),
		pair<std::string, UINT>("WM_LBUTTONDBLCLK", 0x0203),
		pair<std::string, UINT>("WM_RBUTTONDOWN", 0x0204),
		pair<std::string, UINT>("WM_RBUTTONUP", 0x0205),
		pair<std::string, UINT>("WM_RBUTTONDBLCLK", 0x0206),
		pair<std::string, UINT>("WM_MBUTTONDOWN", 0x0207),
		pair<std::string, UINT>("WM_MBUTTONUP", 0x0208),
		pair<std::string, UINT>("WM_MBUTTONDBLCLK", 0x0209),
		pair<std::string, UINT>("WM_MOUSEWHEEL", 0x020A),
		pair<std::string, UINT>("WM_MOUSEHWHEEL", 0x020E),
		pair<std::string, UINT>("WM_MOUSELAST", 0x020A),
		pair<std::string, UINT>("WM_CAPTURECHANGED", 0x0215),
		pair<std::string, UINT>("WM_DROPFILES", 0x0233),
		pair<std::string, UINT>("WM_USER", 0x0400)
	};

	std::map<UINT, std::string> mapMsgToWM_
	{
		pair<UINT, std::string>(0x0001, "WM_CREATE"),
		pair<UINT, std::string>(0x0002, "WM_DESTROY"),
		pair<UINT, std::string>(0x0003, "WM_MOVE"),
		pair<UINT, std::string>(0x0005, "WM_SIZE"),
		pair<UINT, std::string>(0x0006, "WM_ACTIVATE"),
		pair<UINT, std::string>(0x000F, "WM_PAINT"),
		pair<UINT, std::string>(0x0010, "WM_CLOSE"),
		pair<UINT, std::string>(0x0014, "WM_ERASEBKGND"),
		pair<UINT, std::string>(0x0018, "WM_SHOWWINDOW"),
		pair<UINT, std::string>(0x001C, "WM_ACTIVATEAPP"),
		pair<UINT, std::string>(0x0020, "WM_SETCURSOR"),
		pair<UINT, std::string>(0x0021, "WM_MOUSEACTIVATE"),
		pair<UINT, std::string>(0x0024, "WM_GETMINMAXINFO"),
		pair<UINT, std::string>(0x002B, "WM_DRAWITEM"),
		pair<UINT, std::string>(0x0030, "WM_SETFONT"),
		pair<UINT, std::string>(0x0031, "WM_GETFONT"),
		pair<UINT, std::string>(0x004A, "WM_COPYDATA"),
		pair<UINT, std::string>(0x004E, "WM_NOTIFY"),
		pair<UINT, std::string>(0x007B, "WM_CONTEXTMENU"),
		pair<UINT, std::string>(0x007D, "WM_STYLECHANGED"),
		pair<UINT, std::string>(0x007E, "WM_DISPLAYCHANGE"),
		pair<UINT, std::string>(0x0082, "WM_NCDESTROY"),
		pair<UINT, std::string>(0x0083, "WM_NCCALCSIZE"),
		pair<UINT, std::string>(0x0084, "WM_NCHITTEST"),
		pair<UINT, std::string>(0x0085, "WM_NCPAINT"),
		pair<UINT, std::string>(0x00A0, "WM_NCMOUSEMOVE"),
		pair<UINT, std::string>(0x00A1, "WM_NCLBUTTONDOWN"),
		pair<UINT, std::string>(0x00A2, "WM_NCLBUTTONUP"),
		pair<UINT, std::string>(0x00A3, "WM_NCLBUTTONDBLCLK"),
		pair<UINT, std::string>(0x00A4, "WM_NCRBUTTONDOWN"),
		pair<UINT, std::string>(0x00A5, "WM_NCRBUTTONUP"),
		pair<UINT, std::string>(0x00A6, "WM_NCRBUTTONDBLCLK"),
		pair<UINT, std::string>(0x00A7, "WM_NCMBUTTONDOWN"),
		pair<UINT, std::string>(0x00A8, "WM_NCMBUTTONUP"),
		pair<UINT, std::string>(0x00A9, "WM_NCMBUTTONDBLCLK"),
		pair<UINT, std::string>(0x0100, "WM_KEYFIRST"),
		pair<UINT, std::string>(0x0100, "WM_KEYDOWN"),
		pair<UINT, std::string>(0x0101, "WM_KEYUP"),
		pair<UINT, std::string>(0x0102, "WM_CHAR"),
		pair<UINT, std::string>(0x0103, "WM_DEADCHAR"),
		pair<UINT, std::string>(0x0104, "WM_SYSKEYDOWN"),
		pair<UINT, std::string>(0x0105, "WM_SYSKEYUP"),
		pair<UINT, std::string>(0x0106, "WM_SYSCHAR"),
		pair<UINT, std::string>(0x0107, "WM_SYSDEADCHAR"),
		pair<UINT, std::string>(0x0108, "WM_KEYLAST"),
		pair<UINT, std::string>(0x0110, "WM_INITDIALOG"),
		pair<UINT, std::string>(0x0111, "WM_COMMAND"),
		pair<UINT, std::string>(0x0112, "WM_SYSCOMMAND"),
		pair<UINT, std::string>(0xF060, "SC_CLOSE"),
		pair<UINT, std::string>(0x0113, "WM_TIMER"),
		pair<UINT, std::string>(0x0114, "WM_HSCROLL"),
		pair<UINT, std::string>(0x0115, "WM_VSCROLL"),
		pair<UINT, std::string>(0x0117, "WM_INITMENUPOPUP"),
		pair<UINT, std::string>(0x0119, "WM_GESTURE"),
		pair<UINT, std::string>(0x0200, "WM_MOUSEFIRST"),
		pair<UINT, std::string>(0x0200, "WM_MOUSEMOVE"),
		pair<UINT, std::string>(0x0201, "WM_LBUTTONDOWN"),
		pair<UINT, std::string>(0x0202, "WM_LBUTTONUP"),
		pair<UINT, std::string>(0x0203, "WM_LBUTTONDBLCLK"),
		pair<UINT, std::string>(0x0204, "WM_RBUTTONDOWN"),
		pair<UINT, std::string>(0x0205, "WM_RBUTTONUP"),
		pair<UINT, std::string>(0x0206, "WM_RBUTTONDBLCLK"),
		pair<UINT, std::string>(0x0207, "WM_MBUTTONDOWN"),
		pair<UINT, std::string>(0x0208, "WM_MBUTTONUP"),
		pair<UINT, std::string>(0x0209, "WM_MBUTTONDBLCLK"),
		pair<UINT, std::string>(0x020A, "WM_MOUSEWHEEL"),
		pair<UINT, std::string>(0x020E, "WM_MOUSEHWHEEL"),
		pair<UINT, std::string>(0x020A, "WM_MOUSELAST"),
		pair<UINT, std::string>(0x0215, "WM_CAPTURECHANGED"),
		pair<UINT, std::string>(0x0233, "WM_DROPFILES"),
		pair<UINT, std::string>(0x0400, "WM_USER")
	};
}



bool Window_GetRect(void* windowHWND, int* leftOut, int* topOut, int* rightOut, int* bottomOut)
{
	HWND hwnd = (HWND)windowHWND;
	RECT r{ 0, 0, 0, 0 };
	bool isOK = !!GetWindowRect(hwnd, &r);
	*leftOut   = (int)r.left;
	*rightOut  = (int)r.right;
	*topOut	   = (int)r.top;
	*bottomOut = (int)r.bottom;
	return (isOK);
}

void Window_ScreenToClient(void* windowHWND, int x, int y, int* xOut, int* yOut)
{
	// Unlike Win32, Cockos WDL doesn't return a bool to confirm success.
	POINT p{ x, y };
	HWND hwnd = (HWND)windowHWND;
	ScreenToClient(hwnd, &p);
	*xOut = (int)p.x;
	*yOut = (int)p.y;
}

void Window_ClientToScreen(void* windowHWND, int x, int y, int* xOut, int* yOut)
{
	// Unlike Win32, Cockos WDL doesn't return a bool to confirm success.
	POINT p{ x, y };
	HWND hwnd = (HWND)windowHWND;
	ClientToScreen(hwnd, &p);
	*xOut = (int)p.x;
	*yOut = (int)p.y;
}


bool Window_GetClientRect(void* windowHWND, int* leftOut, int* topOut, int* rightOut, int* bottomOut)
{
	// Unlike Win32, Cockos WDL doesn't return a bool to confirm success.
	// However, if hwnd is not a true hwnd, SWELL will return a {0,0,0,0} rect.
	HWND hwnd = (HWND)windowHWND;
	RECT r{ 0, 0, 0, 0 };
#ifdef _WIN32
	bool isOK = !!GetClientRect(hwnd, &r);
#else
	GetClientRect(hwnd, &r);
	bool isOK = (r.bottom != 0 || r.right != 0);
#endif
	if (isOK)
	{
		POINT p{ 0, 0 };
		ClientToScreen(hwnd, &p);
		*leftOut = (int)p.x;
		*rightOut = (int)p.x + (int)r.right;
		*topOut = (int)p.y;
		*bottomOut = (int)p.y + (int)r.bottom;
	}
	return (isOK);
}


/*
bool Window_GetClientRect(void* windowHWND, int* widthOut, int* heightOut)
{
	// Unlike Win32, Cockos WDL doesn't return a bool to confirm success.
	// However, if hwnd is not a true hwnd, SWELL will return a {0,0,0,0} rect.
	RECT r;
#ifdef _WIN32
	bool isOK = !!GetClientRect((HWND)windowHWND, &r);
#else
	GetClientRect((HWND)windowHWND, &r);
	bool isOK = (r.bottom != 0 || r.right != 0);
#endif
	*widthOut = r.right;
	*heightOut = r.bottom;
	return (isOK);
}
*/

void* Window_FromPoint(int x, int y)
{
	POINT p{ x, y };
	return WindowFromPoint(p);
}

void* Window_GetParent(void* windowHWND)
{
	return GetParent((HWND)windowHWND);
}

bool  Window_IsChild(void* parentHWND, void* childHWND)
{
	return !!IsChild((HWND)parentHWND, (HWND)childHWND);
}

void* Window_GetRelated(void* windowHWND, int relation)
{
	return GetWindow((HWND)windowHWND, relation);
}



void  Window_SetFocus(void* windowHWND)
{
	// SWELL returns different types than Win32, so this function won't return anything.
	SetFocus((HWND)windowHWND);
}

void  Window_SetForeground(void* windowHWND)
{
	// SWELL returns different types than Win32, so this function won't return anything.
	SetForegroundWindow((HWND)windowHWND);
}

void* Window_GetFocus()
{
	return GetFocus();
}

void* Window_GetForeground()
{
	return GetForegroundWindow();
}



void  Window_Enable(void* windowHWND, bool enable)
{
	EnableWindow((HWND)windowHWND, (BOOL)enable); // (enable ? (int)1 : (int)0));
}

void  Window_Destroy(void* windowHWND)
{
	DestroyWindow((HWND)windowHWND);
}

void  Window_Show(void* windowHWND, int state)
{
	ShowWindow((HWND)windowHWND, state);
}

bool Window_IsVisible(void* windowHWND)
{
	return !!IsWindowVisible((HWND)windowHWND);
}



void* Window_SetCapture(void* windowHWND)
{
	return SetCapture((HWND)windowHWND);
}

void* Window_GetCapture()
{
	return GetCapture();
}

void  Window_ReleaseCapture()
{
	ReleaseCapture();
}



BOOL CALLBACK Window_Find_Callback_Child(HWND hwnd, LPARAM structPtr)
{
	using namespace Julian;
	EnumWindowsStruct* s = reinterpret_cast<EnumWindowsStruct*>(structPtr);
	int len = GetWindowText(hwnd, s->temp, s->tempLen);
	s->temp[s->tempLen - 1] = '\0'; // Make sure that loooong titles are properly terminated.
	for (int i = 0; (s->temp[i] != '\0') && (i < len); i++) s->temp[i] = (char)tolower(s->temp[i]); // FindWindow is case-insensitive, so this implementation is too
	if (     (s->exact  && (strcmp(s->temp, s->target) == 0)    )
		|| (!(s->exact) && (strstr(s->temp, s->target) != NULL)))
	{
		s->hwnd = hwnd;
		return FALSE;
	}
	else
		return TRUE;
}

BOOL CALLBACK Window_Find_Callback_Top(HWND hwnd, LPARAM structPtr)
{
	using namespace Julian;
	EnumWindowsStruct* s = reinterpret_cast<EnumWindowsStruct*>(structPtr);
	int len = GetWindowText(hwnd, s->temp, s->tempLen);
	s->temp[s->tempLen-1] = '\0'; // Make sure that loooong titles are properly terminated.
	for (int i = 0; (s->temp[i] != '\0') && (i < len); i++) s->temp[i] = (char)tolower(s->temp[i]); // FindWindow is case-insensitive, so this implementation is too
	if (     (s->exact  && (strcmp(s->temp, s->target) == 0)    )
		|| (!(s->exact) && (strstr(s->temp, s->target) != NULL)))
	{
		s->hwnd = hwnd;
		return FALSE;
	}
	else
	{
		EnumChildWindows(hwnd, Window_Find_Callback_Child, structPtr);
		if (s->hwnd != NULL) return FALSE;
		else return TRUE;
	}
}

void* Window_Find(const char* title, bool exact)
{
	using namespace Julian;
	// Cockos SWELL doesn't provide FindWindow, and FindWindowEx doesn't provide the NULL, NULL top-level mode,
	//		so must code own implementation...
	// This implemetation adds two features:
	//		* Searches child windows as well, so that script GUIs can be found even if docked.
	//		* Optionally matches substrings.

	// FindWindow is case-insensitive, so this implementation is too. 
	// Must first convert title to lowercase:
	char titleLower[API_LEN];
	int i = 0;
	for (; (title[i] != '\0') && (i < API_LEN - 1); i++) titleLower[i] = (char)tolower(title[i]); // Convert to lowercase
	titleLower[i] = '\0';

	// To communicate with callback functions, use an EnumWindowsStruct:
	char temp[API_LEN] = ""; // Will temprarily store titles as well as pointer string, so must be longer than TEMP_LEN.
	EnumWindowsStruct e{ titleLower, exact, temp, sizeof(temp), NULL, "", 0 };
	EnumWindows(Window_Find_Callback_Top, reinterpret_cast<LPARAM>(&e));
	return e.hwnd;
}



BOOL CALLBACK Window_ListFind_Callback_Child(HWND hwnd, LPARAM structPtr)
{
	using namespace Julian;
	EnumWindowsStruct* s = reinterpret_cast<EnumWindowsStruct*>(structPtr);
	int len = GetWindowText(hwnd, s->temp, s->tempLen);
	s->temp[s->tempLen - 1] = '\0'; // Make sure that loooong titles are properly terminated.
	// FindWindow is case-insensitive, so this implementation is too. Convert to lowercase.
	for (int i = 0; (s->temp[i] != '\0') && (i < len); i++) s->temp[i] = (char)tolower(s->temp[i]); 
	// If exact, match entire title, otherwise substring
	if (	 (s->exact  && (strcmp(s->temp, s->target) == 0))
		|| (!(s->exact) && (strstr(s->temp, s->target) != NULL)))
	{
		// Convert pointer to string (leaving two spaces for 0x, and add comma separator)
		sprintf_s(s->temp, s->tempLen - 1, "0x%llX,", (unsigned long long int)hwnd);
		// Concatenate to hwndString
		if (strlen(s->hwndString) + strlen(s->temp) < s->hwndLen - 1)
		{
			strcat(s->hwndString, s->temp);
		}
	}
	return TRUE;
}

BOOL CALLBACK Window_ListFind_Callback_Top(HWND hwnd, LPARAM structPtr)
{
	using namespace Julian;
	EnumWindowsStruct* s = reinterpret_cast<EnumWindowsStruct*>(structPtr);
	int len = GetWindowText(hwnd, s->temp, s->tempLen);
	s->temp[s->tempLen - 1] = '\0'; // Make sure that loooong titles are properly terminated.
	// FindWindow is case-insensitive, so this implementation is too. Convert to lowercase.
	for (int i = 0; (s->temp[i] != '\0') && (i < len); i++) s->temp[i] = (char)tolower(s->temp[i]); 
	// If exact, match entire title, otherwise substring
	if (	 (s->exact  && (strcmp(s->temp, s->target) == 0))
		|| (!(s->exact) && (strstr(s->temp, s->target) != NULL)))
	{
		// Convert pointer to string (leaving two spaces for 0x, and add comma separator)
		sprintf_s(s->temp, s->tempLen - 1, "0x%llX,", (unsigned long long int)hwnd);
		// Concatenate to hwndString
		if (strlen(s->hwndString) + strlen(s->temp) < s->hwndLen - 1)
		{
			strcat(s->hwndString, s->temp);
		}
	}
	// Now search all child windows before returning
	EnumChildWindows(hwnd, Window_ListFind_Callback_Child, structPtr);
	return TRUE;
}

void Window_ListFind(const char* title, bool exact, const char* section, const char* key)
{
	using namespace Julian;
	// Cockos SWELL doesn't provide FindWindow, and FindWindowEx doesn't provide the NULL, NULL top-level mode,
	//		so must code own implementation...
	// This implemetation adds three features:
	//		* Searches child windows as well, so that script GUIs can be found even if docked.
	//		* Optionally matches substrings.
	//		* Finds ALL windows that match title.

	// FindWindow is case-insensitive, so this implementation is too. 
	// Must first convert title to lowercase:
	char titleLower[API_LEN];
	for (int i = 0; i < API_LEN; i++)
		if (title[i] == '\0') { titleLower[i] = '\0'; break; }
		else				  { titleLower[i] = (char)tolower(title[i]); }
	titleLower[API_LEN-1] = '\0'; // Make sure that loooong titles are properly terminated.
	// To communicate with callback functions, use an EnumWindowsStruct:
	char temp[API_LEN] = ""; // Will temporarily store pointer strings, as well as titles.
	char hwndString[EXT_LEN] = ""; // Concatenate all pointer strings into this long string.
	EnumWindowsStruct e{ titleLower, exact, temp, sizeof(temp), NULL, hwndString, sizeof(hwndString) }; // All the info that will be passed to the Enum callback functions.
	
	EnumWindows(Window_ListFind_Callback_Top, reinterpret_cast<LPARAM>(&e));
	
	SetExtState(section, key, (const char*)hwndString, false);
}



BOOL CALLBACK Window_FindChild_Callback(HWND hwnd, LPARAM structPtr)
{
	using namespace Julian;
	EnumWindowsStruct* s = reinterpret_cast<EnumWindowsStruct*>(structPtr);
	int len = GetWindowText(hwnd, s->temp, s->tempLen);
	for (int i = 0; (s->temp[i] != '\0') && (i < len); i++) s->temp[i] = (char)tolower(s->temp[i]); // Convert to lowercase
	if (     (s->exact  && (strcmp(s->temp, s->target) == 0))
		|| (!(s->exact) && (strstr(s->temp, s->target) != NULL)))
	{
		s->hwnd = hwnd;
		return FALSE;
	}
	else
		return TRUE;
}

void* Window_FindChild(void* parentHWND, const char* title, bool exact)
{
	using namespace Julian;
	// Cockos SWELL doesn't provide fully-functional FindWindowEx, so rather code own implementation.
	// This implemetation adds two features:
	//		* Searches child windows as well, so that script GUIs can be found even if docked.
	//		* Optionally matches substrings.

	// FindWindow is case-insensitive, so this implementation is too. 
	// Must first convert title to lowercase:
	char titleLower[API_LEN]; 
	for (int i = 0; i < API_LEN; i++)
		if (title[i] == '\0') { titleLower[i] = '\0'; break; }
		else				  { titleLower[i] = (char)tolower(title[i]); }

	// To communicate with callback functions, use an EnumWindowsStruct:
	char temp[TEMP_LEN];
	EnumWindowsStruct e{ titleLower, exact, temp, sizeof(temp), NULL, "", 0 };
	EnumChildWindows((HWND)parentHWND, Window_FindChild_Callback, reinterpret_cast<LPARAM>(&e));
	return e.hwnd;
}



BOOL CALLBACK Window_ListAllChild_Callback(HWND hwnd, LPARAM strPtr)
{
	using namespace Julian;
	char* hwndString = reinterpret_cast<char*>(strPtr);
	char temp[TEMP_LEN] = "";
	sprintf_s(temp, TEMP_LEN - 1, "0x%llX,", (unsigned long long int)hwnd); // Print with leading 0x so that Lua tonumber will automatically notice that it is hexadecimal.
	if (strlen(hwndString) + strlen(temp) < EXT_LEN - 1)
	{
		strcat(hwndString, temp);
		return TRUE;
	}
	else
		return FALSE;
}

void Window_ListAllChild(void* parentHWND, const char* section, const char* key) //char* buf, int buf_sz)
{
	using namespace Julian;
	HWND hwnd = (HWND)parentHWND;
	char hwndString[EXT_LEN] = "";
	EnumChildWindows(hwnd, Window_ListAllChild_Callback, reinterpret_cast<LPARAM>(hwndString));
	SetExtState(section, key, (const char*)hwndString, false);
}



BOOL CALLBACK Window_ListAllTop_Callback(HWND hwnd, LPARAM strPtr)
{
	using namespace Julian;
	char* hwndString = reinterpret_cast<char*>(strPtr);
	char temp[TEMP_LEN] = "";
	sprintf_s(temp, TEMP_LEN - 1, "0x%llX,", (unsigned long long int)hwnd); // Print with leading 0x so that Lua tonumber will automatically notice that it is hexadecimal.
	if (strlen(hwndString) + strlen(temp) < EXT_LEN - 1)
	{
		strcat(hwndString, temp);
		return TRUE;
	}
	else
		return FALSE;
}

void Window_ListAllTop(const char* section, const char* key) //char* buf, int buf_sz)
{
	using namespace Julian;
	char hwndString[EXT_LEN] = "";
	EnumWindows(Window_ListAllTop_Callback, reinterpret_cast<LPARAM>(hwndString));
	SetExtState(section, key, (const char*)hwndString, false);
}



BOOL CALLBACK MIDIEditor_ListAll_Callback_Child(HWND hwnd, LPARAM lParam)
{
	using namespace Julian;
	if (MIDIEditor_GetMode(hwnd) != -1) // Is MIDI editor?
	{
		char* hwndString = reinterpret_cast<char*>(lParam);
		char temp[TEMP_LEN] = "";
		sprintf_s(temp, TEMP_LEN - 1, "0x%llX,", (unsigned long long int)hwnd); // Print with leading 0x so that Lua tonumber will automatically notice that it is hexadecimal.
		if (strstr(hwndString, temp) == NULL) // Match with bounding 0x and comma
		{
			if ((strlen(hwndString) + strlen(temp)) < API_LEN - 1)
			{
				strcat(hwndString, temp);
			}
			else
				return FALSE;
		}
	}
	return TRUE;
}

BOOL CALLBACK MIDIEditor_ListAll_Callback_Top(HWND hwnd, LPARAM lParam)
{
	using namespace Julian;
	if (MIDIEditor_GetMode(hwnd) != -1) // Is MIDI editor?
	{
		char* hwndString = reinterpret_cast<char*>(lParam);
		char temp[TEMP_LEN] = "";
		sprintf_s(temp, TEMP_LEN - 1, "0x%llX,", (unsigned long long int)hwnd);
		if (strstr(hwndString, temp) == NULL) // Match with bounding 0x and comma
		{
			if ((strlen(hwndString) + strlen(temp)) < API_LEN - 1)
			{
				strcat(hwndString, temp);
			}
			else
				return FALSE;
		}
	}
	else // Check if any child windows are MIDI editor. For example if docked in docker or main window.
	{
		EnumChildWindows(hwnd, MIDIEditor_ListAll_Callback_Child, lParam);
	}
	return TRUE;
}

void MIDIEditor_ListAll(char* buf, int buf_sz)
{
	using namespace Julian;
	char hwndString[API_LEN] = "";
	// To find docked editors, must also enumerate child windows.
	EnumWindows(MIDIEditor_ListAll_Callback_Top, reinterpret_cast<LPARAM>(hwndString));
	strcpy_s(buf, buf_sz, hwndString);
}



void Window_Move(void* windowHWND, int left, int top)
{
	// WARNING: SetWindowPos is not completely implemented in SWELL (only NOSIZE and NOMOVE are available).
	// This API therefore divides SetWindowPos's capabilities into four functions:
	//		Move, Resize, SetPosition and SetZOrder, only the first three of which are functional in non-Windows systems.
	// This function moves windows without resizing or requiring any info about window size.
	HWND hwnd = (HWND)windowHWND;
	SetWindowPos(hwnd, NULL, left, top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOSIZE);
}

void Window_Resize(void* windowHWND, int width, int height)
{
	// WARNING: SetWindowPos is not completely implemented in SWELL (only NOSIZE and NOMOVE are available).
	// This API therefore divides SetWindowPos's capabilities into four functions:
	//		Move, Resize, SetPosition and SetZOrder, only the first three of which are functional in non-Windows systems.
	// This function resizes windows without moving or requiring any info about window position.
	HWND hwnd = (HWND)windowHWND;
	SetWindowPos(hwnd, NULL, 0, 0, width, height, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE);
}

void Window_SetPosition(void* windowHWND, int left, int top, int width, int height)
{
	// WARNING: SetWindowPos is not completely implemented in SWELL:
	//   * Only NOSIZE and NOMOVE flags are available.
	//   * The documentation is outdated, and Z order *is* implemented. However, only INSERT_AFTER, HWND_BOTTOM and HWND_TOP are available.
	//     (I think that HWND_NOTOPMOST and HWND_TOPMOST both default to HWND_TOP.)
	HWND hwnd = (HWND)windowHWND;
	SetWindowPos(hwnd, NULL, left, top, width, height, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER );
}

bool Window_SetZOrder(void* windowHWND, const char* ZOrder, void* insertAfterHWND, int flags)
{
	// WARNING: SetWindowPos is not completely implemented in SWELL:
	//   * Only NOSIZE and NOMOVE flags are available.
	//   * The documentation is outdated, and Z order *is* implemented. However, only INSERT_AFTER, HWND_BOTTOM and HWND_TOP are available.
	//     I think that HWND_NOTOPMOST and HWND_TOPMOST both default to HWND_TOP, so this function will do the same.
#ifdef _WIN32
	unsigned int uFlags = ((unsigned int)flags) | SWP_NOMOVE | SWP_NOSIZE;
	HWND insertAfter;
	// Search for single chars that can distinguish the different ZOrder strings.
	if      (strchr(ZOrder, 'I') || strchr(ZOrder, 'i')) insertAfter = (HWND)insertAfterHWND; // insertAfter
	else if (strchr(ZOrder, 'B') || strchr(ZOrder, 'b')) insertAfter = (HWND) 1; // Bottom
	//else if (strchr(ZOrder, 'N') || strchr(ZOrder, 'n')) insertAfter = (HWND)-2; // NoTopmost // Whoops, bug! "N" is also in "HWND"!
	//else if (strchr(ZOrder, 'M') || strchr(ZOrder, 'm')) insertAfter = (HWND)-1; // Topmost
	else												 insertAfter = (HWND) 0; // Top

	return !!SetWindowPos((HWND)windowHWND, insertAfter, 0, 0, 0, 0, uFlags);
#else
	return false;
#endif
}



bool Window_SetTitle(void* windowHWND, const char* title)
{
	return !!SetWindowText((HWND)windowHWND, title);
}

void Window_GetTitle(void* windowHWND, char* buf, int buf_sz)
{
	GetWindowText((HWND)windowHWND, buf, buf_sz);
}



void* Window_HandleFromAddress(int addressLow32Bits, int addressHigh32Bits)
{
	unsigned long long longHigh = (unsigned int)addressHigh32Bits;
	unsigned long long longLow  = (unsigned int)addressLow32Bits;
	unsigned long long longAddress = longLow | (longHigh << 32);
	//unsigned long long longAddress = ((unsigned long long)addressLow32Bits) | (((unsigned long long)addressHigh32Bits) << 32);
	return (HWND)longAddress;
}

bool  Window_IsWindow(void* windowHWND)
{
	return !!IsWindow((HWND)windowHWND);
}




bool WindowMessage_ListIntercepts(void* windowHWND, char* buf, int buf_sz)
{
	using namespace Julian;
	buf[0] = 0;
	if (mapWindowToData.count((HWND)windowHWND))
	{
		auto& messages = mapWindowToData[(HWND)windowHWND].messages;
		for (const auto& it : messages)
		{
			if (strlen(buf) < buf_sz - 32)
			{
				if (mapMsgToWM_.count(it.first))
					sprintf(strchr(buf, 0), "%s:", mapMsgToWM_[it.first]);
				else
					sprintf(strchr(buf, 0), "0x%04X:", it.first);
				if ((it.second).passthrough)
					strcat(buf, "passthrough,");
				else
					strcat(buf, "block,");
			}
			else
				return false;
		}
		char* lastComma{ strrchr(buf, ',') }; // Remove final comma
		if (lastComma)
			*lastComma = 0;
		return true;
	}
}

bool WindowMessage_Post(void* windowHWND, const char* message, int wParamLow, int wParamHigh, int lParamLow, int lParamHigh)
{
	using namespace Julian;

	std::string msgString = message;
	UINT uMsg = 0;
	if (mapWM_toMsg.count(msgString))
		uMsg = mapWM_toMsg[msgString];
	else
	{
		errno = 0;
		char* endPtr;
		uMsg = strtoul(message, &endPtr, 16);
		if (endPtr == message || errno != 0) // 0x0000 is a valid message type, so cannot assume 0 is error.
			return false;
	}

	WPARAM wParam = MAKEWPARAM(wParamLow, wParamHigh);
	LPARAM lParam = MAKELPARAM(lParamLow, lParamHigh);
	HWND hwnd = (HWND)windowHWND;

	// Is this window currently being intercepted?
	if (mapWindowToData.count(hwnd)) {
		sWindowData& w = mapWindowToData[hwnd];
		if (w.messages.count(uMsg)) {
			w.origProc(hwnd, uMsg, wParam, lParam); // WindowProcs usually return 0 if message was handled.  But not always, 
			return true;
		}
	}
	return !!PostMessage(hwnd, uMsg, wParam, lParam);
}

int WindowMessage_Send(void* windowHWND, const char* message, int wParamLow, int wParamHigh, int lParamLow, int lParamHigh)
{
	using namespace Julian;

	std::string msgString = message;
	UINT uMsg = 0;
	if (mapWM_toMsg.count(msgString))
		uMsg = mapWM_toMsg[msgString];
	else
	{
		errno = 0;
		char* endPtr;
		uMsg = strtoul(message, &endPtr, 16);
		if (endPtr == message || errno != 0) // 0x0000 is a valid message type, so cannot assume 0 is error.
			return FALSE;
	}

	WPARAM wParam = MAKEWPARAM(wParamLow, wParamHigh);
	LPARAM lParam = MAKELPARAM(lParamLow, lParamHigh);

	return (int)SendMessage((HWND)windowHWND, uMsg, wParam, lParam);
}

bool WindowMessage_Peek(void* windowHWND, const char* message, double* timeOut, int* wParamLowOut, int* wParamHighOut, int* lParamLowOut, int* lParamHighOut)
{
	// lParamLow, lParamHigh, and wParamHigh are signed, whereas wParamLow is unsigned.
	using namespace Julian;

	std::string msgString = message;
	UINT uMsg = 0;
	if (mapWM_toMsg.count(msgString))
		uMsg = mapWM_toMsg[msgString];
	else
	{
		errno = 0;
		char* endPtr;
		uMsg = strtoul(message, &endPtr, 16);
		if (endPtr == message || errno != 0) // 0x0000 is a valid message type, so cannot assume 0 is error.
			return false;
	}

	if (mapWindowToData.count((HWND)windowHWND))
	{
		sWindowData& w = mapWindowToData[(HWND)windowHWND];

		if (w.messages.count(uMsg))
		{
			sMsgData& m = w.messages[uMsg];

			*timeOut = m.time;
			*lParamLowOut = GET_X_LPARAM(m.lParam);
			*lParamHighOut = GET_Y_LPARAM(m.lParam);
			*wParamLowOut = GET_KEYSTATE_WPARAM(m.wParam);
			*wParamHighOut = GET_WHEEL_DELTA_WPARAM(m.wParam);

			return true;
		}
	}
	return false;
}

LRESULT CALLBACK WindowMessage_Intercept_Callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	using namespace Julian;

	// If not in map, we don't know how to call original process.
	if (mapWindowToData.count(hwnd) == 0)
		return 1;

	sWindowData& windowData = mapWindowToData[hwnd]; // Get reference/alias because want to write into existing struct.

	// Always get KEYDOWN and KEYUP, to construct keyboard bitfields
	/*
	if (uMsg == WM_KEYDOWN)
	{
		if ((wParam >= 0x41) && (wParam <= VK_SLEEP)) // A to SLEEP in virtual key codes
			windowData.keysBitfieldAtoSLEEP |= (1 << (wParam - 0x41));
		else if ((wParam >= VK_LEFT) && (wParam <= 0x39)) // LEFT to 9 in virtual key codes
			windowData.keysBitfieldLEFTto9 |= (1 << (wParam - VK_LEFT));
		else if ((wParam >= VK_BACK) && (wParam <= VK_HOME)) // BACKSPACE to HOME in virtual key codes
			windowData.keysBitfieldLEFTto9 |= (1 << (wParam - VK_BACK));
	}

	else if (uMsg == WM_KEYUP)
	{
		if ((wParam >= 0x41) && (wParam <= VK_SLEEP)) // A to SLEEP in virtual key codes
			windowData.keysBitfieldAtoSLEEP &= !(1 << (wParam - 0x41));
		else if ((wParam >= VK_LEFT) && (wParam <= 0x39)) // LEFT to 9 in virtual key codes
			windowData.keysBitfieldLEFTto9 &= !(1 << (wParam - VK_LEFT));
		else if ((wParam >= VK_BACK) && (wParam <= VK_HOME)) // BACKSPACE to HOME in virtual key codes
			windowData.keysBitfieldBACKtoHOME &= !(1 << (wParam - VK_BACK));
	}
	*/

	// Event that should be intercepted? 
	if (windowData.messages.count(uMsg)) // ".contains" has only been implemented in more recent C++ versions
	{
		windowData.messages[uMsg].time = time_precise();
		windowData.messages[uMsg].wParam = wParam;
		windowData.messages[uMsg].lParam = lParam;

		// If event will not be passed through, can quit here.
		if (windowData.messages[uMsg].passthrough == false)
		{
			// Most WM_ messages return 0 if processed, with only a few exceptions:
			switch (uMsg)
			{
			case WM_SETCURSOR:
			case WM_DRAWITEM:
			case WM_COPYDATA:
				return 1;
			case WM_MOUSEACTIVATE:
				return 3;
			default:
				return 0;
			}
		}
	}

	// Any other event that isn't intercepted.
	return windowData.origProc(hwnd, uMsg, wParam, lParam);
}

int WindowMessage_Intercept(void* windowHWND, const char* messages)
{
	using namespace Julian;
	HWND hwnd = (HWND)windowHWND;

	// According to swell-functions.h, IsWindow is slow in swell. However, Window_Intercept will probably not be called many times per script. 
	if (!IsWindow(hwnd))
		return ERR_NOT_WINDOW;

	// strtok *replaces* characters in the string, so better copy messages to new char array.
	char msg[API_LEN];
	if (strcpy_s(msg, API_LEN, messages) != 0)
		return ERR_PARSING;

	// messages string will be parsed into uMsg message types and passthrough modifiers 
	UINT uMsg;
	bool passthrough;

	// For use while tokenizing messages string
	char *token;
	std::string msgString;
	const char* delim = ":;,= \n\t";
	char* endPtr; 

	// Parsed info will be stored in these temporary maps
	std::map<UINT, sMsgData> newMessages;

	// Parse!
	token = strtok(msg, delim);
	while (token)
	{
		// Get message number
		msgString = token;
		if (mapWM_toMsg.count(msgString))
			uMsg = mapWM_toMsg[msgString];
		else
		{
			errno = 0;
			uMsg = strtoul(token, &endPtr, 16);
			if (endPtr == token || errno != 0) // 0x0000 is a valid message type, so cannot assume 0 is error.
				return ERR_PARSING;
		}

		// Now get passthrough
		token = strtok(NULL, delim);
		if (token == NULL)
			return ERR_PARSING; // Each message type must be followed by a modifier
		else if (token[0] == 'p' || token[0] == 'P')
			passthrough = true;
		else if (token[0] == 'b' || token[0] == 'B')
			passthrough = false;
		else // Not block or passthrough
			return ERR_PARSING;

		// Save in temporary maps
		newMessages.emplace(uMsg, sMsgData{ passthrough, 0, 0, 0 }); // time = 0 indicates that this message type is being intercepted OK, but that no message has yet been received.

		token = strtok(NULL, delim);
	}

	// Parsing went OK?  Any messages to intercept?
	if (newMessages.size() == 0)
		return ERR_PARSING;

	// Is this window already being intercepted?
	if (mapWindowToData.count(hwnd) == 0) // Not yet intercepted
	{
		// Try to get the original process.
		WNDPROC origProc = nullptr;
#ifdef _WIN32
		origProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)WindowMessage_Intercept_Callback);
#else
		origProc = (WNDPROC)SetWindowLong(hwnd, GWL_WNDPROC, (LONG_PTR)WindowMessage_Intercept_Callback);
#endif
		if (!origProc)
			return ERR_ORIGPROC;

		// Got everything OK.  Finally, store struct.
		Julian::mapWindowToData.emplace(hwnd, sWindowData{ origProc, newMessages }); // Insert into static map of namespace
		return 1;
	}

	// Already intercepted.  So add to existing maps.
	else
	{
		// Check that no overlaps: only one script may intercept each message type
		// Want to update existing map, so use aliases/references
		auto& existingMsg = Julian::mapWindowToData[hwnd].messages; // Messages that are already being intercepted for this window
		for (const auto& it : newMessages)
		{
			if (existingMsg.count(it.first)) // Oops, already intercepting this message type
			{
				return ERR_ALREADY_INTERCEPTED;
			}
		}
		// No overlaps, so add new intercepts to existing messages to intercept
		existingMsg.insert(newMessages.begin(), newMessages.end());
		return 1;
	}
}

int WindowMessage_Release(void* windowHWND, const char* messages)
{
	using namespace Julian;
	HWND hwnd = (HWND)windowHWND;

	if (mapWindowToData.count(hwnd) == 0)
		return ERR_NOT_WINDOW;

	// strtok *replaces* characters in the string, so better copy messages to new char array.
	char msg[API_LEN];
	if (strcpy_s(msg, sizeof(msg), messages) != 0)
		return ERR_PARSING;

	// messages string will be parsed into uMsg message types and passthrough modifiers 
	UINT uMsg;
	char *token;
	std::string msgString;
	const char* delim = ":;,= \n\t";
	std::set<UINT> messagesToErase;

	// Parse!
	token = strtok(msg, delim);
	while (token)
	{
		// Get message number
		msgString = token;
		if (mapWM_toMsg.count(msgString))
			uMsg = mapWM_toMsg[msgString];
		else
			uMsg = strtoul(token, NULL, 16);
		if (!uMsg || (errno == ERANGE))
			return ERR_PARSING;

		// Store this parsed uMsg number
		messagesToErase.insert(uMsg);

		token = strtok(NULL, delim);
	}

	// Erase all message types that have been parsed
	auto& existingMessages = Julian::mapWindowToData[hwnd].messages; // Messages that are already being intercepted for this window
	for (const UINT it : messagesToErase)
		existingMessages.erase(it);

	// If no messages need to be intercepted any more, release this window
	if (existingMessages.size() == 0)
		WindowMessage_ReleaseWindow(hwnd);

	return TRUE;
}

void WindowMessage_ReleaseWindow(void* windowHWND)
{
	using namespace Julian;

	if (mapWindowToData.count((HWND)windowHWND))
	{
		WNDPROC origProc = mapWindowToData[(HWND)windowHWND].origProc;
#ifdef _WIN32
		SetWindowLongPtr((HWND)windowHWND, GWLP_WNDPROC, (LONG_PTR)origProc);
#else
		SetWindowLong((HWND)windowHWND, GWL_WNDPROC, (LONG_PTR)origProc);
#endif
		mapWindowToData.erase((HWND)windowHWND);
	}
}

void WindowMessage_ReleaseAll()
{
	using namespace Julian;
	for (auto it = mapWindowToData.begin(); it != mapWindowToData.end(); ++it)
	{
		HWND hwnd = it->first;
		WNDPROC origProc = it->second.origProc;
#ifdef _WIN32
		SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)origProc);
#else
		SetWindowLong(hwnd, GWL_WNDPROC, (LONG_PTR)origProc);
#endif
	}
	mapWindowToData.clear();
}


int Mouse_GetState(int flags)
{
	int state = 0;
	if ((flags & 1)  && (GetAsyncKeyState(VK_LBUTTON) >> 1))	state |= 1;
	if ((flags & 2)  && (GetAsyncKeyState(VK_RBUTTON) >> 1))	state |= 2;
	if ((flags & 64) && (GetAsyncKeyState(VK_MBUTTON) >> 1))	state |= 64;
	if ((flags & 4)  && (GetAsyncKeyState(VK_CONTROL) >> 1))	state |= 4;
	if ((flags & 8)  && (GetAsyncKeyState(VK_SHIFT) >> 1))		state |= 8;
	if ((flags & 16) && (GetAsyncKeyState(VK_MENU) >> 1))		state |= 16;
	if ((flags & 32) && (GetAsyncKeyState(VK_LWIN) >> 1))		state |= 32;
	return state;
}

bool Mouse_SetPosition(int x, int y)
{
	return !!SetCursorPos(x, y);
}

void* Mouse_LoadCursor(int cursorNumber)
{
	// GetModuleHandle isn't implemented in SWELL, but fortunately also not necessary.
	// In SWELL, LoadCursor ignores hInst and will automatically loads either standard cursor or "registered cursor".
#ifdef _WIN32
	HINSTANCE hInst; 
	if (cursorNumber > 32000) // In Win32, hInst = NULL loads standard Window cursors, with IDs > 32000.
		hInst = NULL;
	else
		hInst = GetModuleHandle(NULL); // REAPER exe file.
	return LoadCursor(hInst, MAKEINTRESOURCE(cursorNumber));
#else
	return SWELL_LoadCursor(MAKEINTRESOURCE(cursorNumber));
#endif
}

void* Mouse_LoadCursorFromFile(const char* pathAndFileName)
{
#ifdef _WIN32
	return LoadCursorFromFile(pathAndFileName);
#else
	return SWELL_LoadCursorFromFile(pathAndFileName);
#endif
}

void Mouse_SetCursor(void* cursorHandle)
{
	SetCursor((HCURSOR)cursorHandle);
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void* GDI_GetDC(void* windowHWND)
{
	return GetDC((HWND)windowHWND);
}

void* GDI_GetWindowDC(void* windowHWND)
{
	return GetWindowDC((HWND)windowHWND);
}

void GDI_ReleaseDC(void* windowHWND, void* deviceHDC)
{
	ReleaseDC((HWND)windowHWND, (HDC)deviceHDC);
}

void GDI_SetPen(void* deviceHDC, int iStyle, int width, int color)
{
	HPEN pen = CreatePen(iStyle, width, color);
	SelectObject((HDC)deviceHDC, pen);
}

void GDI_SetFont(void* deviceHDC, int height, int weight, int angle, bool italic, bool underline, bool strikeOut, const char* fontName)
{
	HFONT font = CreateFont(height, 0, angle, 0, weight, (BOOL)italic, (BOOL)underline, (BOOL)strikeOut, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH|FF_DONTCARE, fontName);
	SelectObject((HDC)deviceHDC, font);
}

void GDI_RoundRect(void* deviceHDC, int left, int top, int right, int bottom, int xrnd, int yrnd)
{
	RoundRect((HDC)deviceHDC, left, top, right, bottom, xrnd, yrnd);
}

void GDI_FillRect(void* deviceHDC, int left, int top, int right, int bottom, int color)
{
	RECT r{ left, top, right, bottom };
	FillRect((HDC)deviceHDC, &r, CreateSolidBrush(color));
}

void GDI_SetBkMode(void* deviceHDC, int mode)
{
	SetBkMode((HDC)deviceHDC, mode);
}

void GDI_SetBkColor(void* deviceHDC, int color)
{
	SetBkColor((HDC)deviceHDC, color);
}

void GDI_SetTextColor(void* deviceHDC, int color)
{
	SetTextColor((HDC)deviceHDC, color);
}

int GDI_GetTextColor(void* deviceHDC)
{
	return GetTextColor((HDC)deviceHDC);
}

int GDI_DrawText(void* deviceHDC, const char *text, int len, int left, int top, int right, int bottom, int align)
{
	RECT r{ left, top, right, bottom };
	return DrawText((HDC)deviceHDC, text, len, &r, align);
}

void GDI_SetPixel(void* deviceHDC, int x, int y, int color)
{
	SetPixel((HDC)deviceHDC, x, y, color);
}

void GDI_Rectangle(void* deviceHDC, int left, int top, int right, int bottom)
{
	Rectangle((HDC)deviceHDC, left, top, right, bottom);
}

void GDI_MoveTo(void* deviceHDC, int x, int y)
{
	MoveToEx((HDC)deviceHDC, x, y, NULL);
}

void GDI_LineTo(void* deviceHDC, int x, int y)
{
	LineTo((HDC)deviceHDC, x, y);
}

void GDI_Ellipse(void* deviceHDC, int left, int top, int right, int bottom)
{
	Ellipse((HDC)deviceHDC, left, top, right, bottom);
}

void GDI_DrawFocusRect(void* windowHWND, int left, int top, int right, int bottom)
{
	RECT r{ left, top, right, bottom };
#ifdef _WIN32
	HDC dc = GetDC((HWND)windowHWND);
	DrawFocusRect(dc, &r);
#else
	SWELL_DrawFocusRect((HWND)windowHWND, &r, NULL)
#endif
}


///////////////////////////////////////////////////////////////////////////////////////////////////

bool Window_GetScrollInfo(void* windowHWND, const char* scrollbar, int* positionOut, int* pageSizeOut, int* minOut, int* maxOut, int* trackPosOut)
{
	SCROLLINFO si = { sizeof(SCROLLINFO), SIF_ALL, };
	int nBar = ((strchr(scrollbar, 'v') || strchr(scrollbar, 'V')) ? SB_VERT : SB_HORZ); // Match strings such as "SB_VERT", "VERT" or "v".
	bool isOK = !!CoolSB_GetScrollInfo((HWND)windowHWND, nBar, &si);
	*pageSizeOut = si.nPage;
	*positionOut = si.nPos;
	*minOut = si.nMin;
	*maxOut = si.nMax;
	*trackPosOut = si.nTrackPos;
	return isOK;
}

bool Window_SetScrollInfo(void* windowHWND, const char* scrollbar, int position, int min, int max)
{
	SCROLLINFO si = { sizeof(SCROLLINFO), SIF_PAGE, min, max, 0, position, 0 }; // Application cannot set trackpos, and SetScrollInfo ignores this value

	if (strchr(scrollbar, 'H') || strchr(scrollbar, 'h')) // Match strings such as "SB_HORZ", "HORZ" or "h".
	{
		CoolSB_GetScrollInfo((HWND)windowHWND, SB_HORZ, &si); // Get page size (aka clientrect size, which cannot be changed)
		si.fMask = SIF_RANGE;
		bool isOK = !!CoolSB_SetScrollInfo((HWND)windowHWND, SB_HORZ, &si, TRUE);
		SendMessage((HWND)windowHWND, WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, position), NULL);
		return isOK;
	}
	else
	{
		CoolSB_GetScrollInfo((HWND)windowHWND, SB_VERT, &si); // Get page size (aka clientrect size, which cannot be changed)
		si.fMask = SIF_RANGE;
		bool isOK = !!SetScrollInfo((HWND)windowHWND, SB_VERT, &si, TRUE);
		SendMessage((HWND)windowHWND, WM_VSCROLL, MAKEWPARAM(SB_THUMBPOSITION, position), NULL);
		return isOK;
	}
}

int Scroll_GetPos(void* windowHWND, const char* scrollbar)
{
	int nBar = ((strchr(scrollbar, 'v') || strchr(scrollbar, 'V')) ? SB_VERT : SB_HORZ); // Match strings such as "SB_VERT", "VERT" or "v".
	return CoolSB_GetScrollPos((HWND)windowHWND, nBar);
}

bool Scroll_SetPos(void* windowHWND, const char* scrollbar, int position)
{
	HWND hwnd = (HWND)windowHWND;
	int nBar = ((strchr(scrollbar, 'v') || strchr(scrollbar, 'V')) ? SB_VERT : SB_HORZ); // Match strings such as "SB_VERT", "VERT" or "v".
	bool isOK = !!CoolSB_SetScrollPos((HWND)windowHWND, nBar, position, TRUE);
	InvalidateRect(hwnd, NULL, TRUE);
	UpdateWindow(hwnd);
	return isOK;
}

bool Window_Scroll(void* windowHWND, int XAmount, int YAmount)
{
	RECT r; GetClientRect((HWND)windowHWND, &r);
	int XAbs = (XAmount < 0) ? -XAmount : XAmount;
	int YAbs = (YAmount < 0) ? -YAmount : YAmount;
	r.left = r.left - XAbs;
	r.right = r.right + XAbs;
	r.top = r.top - YAbs;
	r.bottom = r.bottom + YAbs;
	return !!ScrollWindow((HWND)windowHWND, XAmount, YAmount, NULL, &r);
}

bool Window_InvalidateRect(void* windowHWND, int left, int top, int right, int bottom)
{
	// SWELL ignores bErase, however I'm not sure whether it defaults to TRUE or FALSE.
	const RECT r{ left, top, right, bottom };
	return !!InvalidateRect((HWND)windowHWND, &r, 0);
}

void Window_Update(void* windowHWND)
{
	UpdateWindow((HWND)windowHWND);
}

