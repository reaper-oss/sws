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



namespace julian
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

	// This struct is used to store the data of intercepted messages.
	//		In addition to the standard wParam and lParam, a unique timestamp is added.
	struct sMsgData
	{
		double time;
		WPARAM wParam;
		LPARAM lParam;
	};

	// This struct and map store the data of each HWND that is being intercepted.
	// (Each window can only be intercepted by one script at a time.)
	// For each window, three bitfields summarize the up/down states of most of the keyboard.
	struct sWindowData
	{
		WNDPROC origProc;
		std::map<UINT, bool>		messagesToIntercept; // passthrough = true, block = false
		std::map<UINT, sMsgData>	latestMessage;
		int keysBitfieldBACKtoHOME	= 0; // Virtual key codes VK_BACK (backspace) to VK_HOME
		int keysBitfieldAtoSLEEP	= 0;
		int keysBitfieldLEFTto9		= 0;
	};
	const bool BLOCK = false;

	// Each window that is being intercepted, will be mapped to its data struct.
	std::map <HWND, sWindowData> mapWindowToData;
	
}



bool  Window_GetRect(void* windowHWND, int* leftOut, int* topOut, int* rightOut, int* bottomOut)
{
	HWND hwnd = (HWND)windowHWND;
	RECT r{ 0, 0, 0, 0 };
	bool isOK = !!GetWindowRect(hwnd, &r);
	if (isOK)
	{
		*leftOut   = (int)r.left;
		*rightOut  = (int)r.right;
		*topOut	   = (int)r.top;
		*bottomOut = (int)r.bottom;
	}
	return (isOK);
}

void  Window_ScreenToClient(void* windowHWND, int x, int y, int* xOut, int* yOut)
{
	// Unlike Win32, Cockos WDL doesn't return a bool to confirm success.
	POINT p{ x, y };
	HWND hwnd = (HWND)windowHWND;
	ScreenToClient(hwnd, &p);
	*xOut = (int)p.x;
	*yOut = (int)p.y;
}

void  Window_ClientToScreen(void* windowHWND, int x, int y, int* xOut, int* yOut)
{
	// Unlike Win32, Cockos WDL doesn't return a bool to confirm success.
	POINT p{ x, y };
	HWND hwnd = (HWND)windowHWND;
	ClientToScreen(hwnd, &p);
	*xOut = (int)p.x;
	*yOut = (int)p.y;
}

void  Window_GetClientRect(void* windowHWND, int* leftOut, int* topOut, int* rightOut, int* bottomOut)
{
	// Unlike Win32, Cockos WDL doesn't return a bool to confirm success.
	HWND hwnd = (HWND)windowHWND;
	RECT r{ 0, 0, 0, 0 };
	POINT p{ 0, 0 }; 
	ClientToScreen(hwnd, &p);
	GetClientRect(hwnd, &r);
	*leftOut   = (int)p.x;
	*rightOut  = (int)p.x + (int)r.right;
	*topOut    = (int)p.y;
	*bottomOut = (int)p.y + (int)r.bottom;
}

bool Window_GetScrollInfo(void* windowHWND, const char* scrollbar, int* positionOut, int* pageOut, int* minOut, int* maxOut, int* trackPosOut)
{
	HWND hwnd = (HWND)windowHWND;
	SCROLLINFO si = { sizeof(SCROLLINFO), };
	si.fMask = SIF_ALL;
	int nBar = ((strchr(scrollbar, 'v') || strchr(scrollbar, 'V')) ? SB_VERT : SB_HORZ); // Match strings such as "SB_VERT", "VERT" or "v".
	bool isOK = !!CoolSB_GetScrollInfo(hwnd, nBar, &si);
	*pageOut = si.nPage;
	*positionOut = si.nPos;
	*minOut = si.nMin;
	*maxOut = si.nMax;
	*trackPosOut = si.nTrackPos;
	return isOK;
}

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
	using namespace julian;
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
	using namespace julian;
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
	using namespace julian;
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
	using namespace julian;
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
	using namespace julian;
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
	using namespace julian;
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
	using namespace julian;
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
	using namespace julian;
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
	using namespace julian;
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
	using namespace julian;
	HWND hwnd = (HWND)parentHWND;
	char hwndString[EXT_LEN] = "";
	EnumChildWindows(hwnd, Window_ListAllChild_Callback, reinterpret_cast<LPARAM>(hwndString));
	SetExtState(section, key, (const char*)hwndString, false);
}



BOOL CALLBACK Window_ListAllTop_Callback(HWND hwnd, LPARAM strPtr)
{
	using namespace julian;
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
	using namespace julian;
	char hwndString[EXT_LEN] = "";
	EnumWindows(Window_ListAllTop_Callback, reinterpret_cast<LPARAM>(hwndString));
	SetExtState(section, key, (const char*)hwndString, false);
}



BOOL CALLBACK MIDIEditor_ListAll_Callback_Child(HWND hwnd, LPARAM lParam)
{
	using namespace julian;
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
	using namespace julian;
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
	using namespace julian;
	char hwndString[API_LEN] = "";
	// To find docked editors, must also enumerate child windows.
	EnumWindows(MIDIEditor_ListAll_Callback_Top, reinterpret_cast<LPARAM>(hwndString));
	strcpy_s(buf, buf_sz, hwndString);
}



void Window_Move(void* windowHWND, int left, int top)
{
	// WARNING: SetWindowPos is not completely implemented in SWELL.
	// This API therefore divides SetWindowPos's capabilities into three functions:
	//		Move, Resize and SetZOrder, only the first two of which are functional in non-Windows systems.
	HWND hwnd = (HWND)windowHWND;
	SetWindowPos(hwnd, NULL, left, top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOSIZE);
}

void Window_Resize(void* windowHWND, int width, int height)
{
	// WARNING: SetWindowPos is not completely implemented in SWELL.
	// This API therefore divides SetWindowPos's capabilities into three functions:
	//		Move, Resize and SetZOrder, only the first two of which are functional in non-Windows systems.
	HWND hwnd = (HWND)windowHWND;
	SetWindowPos(hwnd, NULL, 0, 0, width, height, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE);
}

bool Window_SetZOrder(void* windowHWND, const char* ZOrder, void* insertAfterHWND, int flags)
{
	// WARNING: SetWindowPos is not completely implemented in SWELL.
	// This API therefore divides SetWindowPos's capabilities into three functions:
	//		Move, Resize and SetZOrder, only the first two of which are functional in non-Windows systems.
	// SetZOrder can do everything that SetWindowPos does, except move and resize windows.
#ifdef _WIN32
	unsigned int uFlags = ((unsigned int)flags) | SWP_NOMOVE | SWP_NOSIZE;
	HWND insertAfter;
	// Search for single chars that can distinguish the different ZOrder strings.
	if      (strchr(ZOrder, 'I') || strchr(ZOrder, 'i')) insertAfter = (HWND)insertAfterHWND; // insertAfter
	else if (strchr(ZOrder, 'B') || strchr(ZOrder, 'b')) insertAfter = (HWND) 1; // Bottom
	else if (strchr(ZOrder, 'N') || strchr(ZOrder, 'n')) insertAfter = (HWND)-2; // NoTopmost
	else if (strchr(ZOrder, 'M') || strchr(ZOrder, 'm')) insertAfter = (HWND)-1; // Topmost
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



bool Window_PostMessage(void* windowHWND, int message, int wParamLow, int wParamHigh, int lParamLow, int lParamHigh)
{
	WPARAM wParam = MAKEWPARAM(wParamLow, wParamHigh);
	LPARAM lParam = MAKELPARAM(lParamLow, lParamHigh);

	// Is this window currently being intercepted?
	using namespace julian;
	if (mapWindowToData.count((HWND)windowHWND)) {
		sWindowData w = mapWindowToData[(HWND)windowHWND];
		if (w.messagesToIntercept.count((UINT)message)) {
			return !w.origProc((HWND)windowHWND, (UINT)message, wParam, lParam); // WindowProcs usually return 0 if message was handled.  But not always, 
		}
	}
	return !!PostMessage((HWND)windowHWND, (UINT)message, wParam, lParam);
}

bool Window_PollMessage(void* windowHWND, int message, double* timeOut, int* wParamLowOut, int* wParamHighOut, int* lParamLowOut, int* lParamHighOut)
{
	// lParamLow, lParamHigh, and wParamHigh are signed, whereas wParamLow is unsigned.
	using namespace julian;

	if (mapWindowToData.count((HWND)windowHWND))
	{
		sWindowData w = mapWindowToData[(HWND)windowHWND];

		if (w.latestMessage.count((UINT)message))
		{
			sMsgData m = w.latestMessage[(UINT)message];

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

LRESULT CALLBACK Window_Intercept_Callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	using namespace julian;

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
	if (windowData.messagesToIntercept.count(uMsg)) // ".contains" has only been implemented in more recent C++ versions
	{
		windowData.latestMessage.erase(uMsg);
		windowData.latestMessage.emplace(uMsg, sMsgData{ time_precise(), wParam, lParam });

		// If event will not be passed through, can quit here.
		if (windowData.messagesToIntercept[uMsg] == BLOCK)
			return 0;
	}

	// Any other event that isn't intercepted.
	return windowData.origProc(hwnd, uMsg, wParam, lParam);
}

int Window_Intercept(void* windowHWND, const char* messages)
{
	using namespace julian;
	HWND hwnd = (HWND)windowHWND;

	// Each window can only be intercepted by one script. Therefore check if alreay in map.
	if (mapWindowToData.count(hwnd) > 0)
		return -1;

	// According to swell-functions.h, IsWindow is slow in swell. However, Window_Intercept will probably not be called many times per script. 
	if (!IsWindow(hwnd))
		return -2;

	char msg[API_LEN];
	if (strcpy_s(msg, sizeof(msg), messages) != 0)
		return -3;

	// messages string will be parsed into uMsg message types and passthrough modifiers 
	const char delim[] = ":;, ";
	UINT uMsg;
	bool passthrough;
	char *token, *next_token, *str_end; // For use in strtok function

	// Parsed info will be stored in these maps
	std::map<UINT, bool>	messagesToIntercept; // passthrough = true, block = false
	std::map<UINT, sMsgData>latestMessage;

	// Parse!
	token = strtok_s(msg, delim, &next_token);
	while (token)
	{
		// Get message number
		uMsg = strtoul(token, &str_end, 16);
		if (!uMsg || (errno == ERANGE)) return -3;

		// Now get passthrough
		token = strtok_s(NULL, delim, &next_token);
		if (token == NULL) return -3; // Each message type must be followed by a modifier
		if (token[0] == 'p' || token[0] == 'P')
			passthrough = true;
		else if (token[0] == 'b' || token[0] == 'B')
			passthrough = false;
		else
			return -3;

		messagesToIntercept.emplace(uMsg, passthrough);
		latestMessage.emplace(uMsg, sMsgData{ 0, 0, 0 }); // time = 0 means that this message type is being intercepted OK, but no message has been intercepted yet.

		token = strtok_s(NULL, delim, &next_token);
	}

	// Parsing went OK?  Any messages to intercept?
	if (messagesToIntercept.size() == 0)
		return -3;

	// Try to get the original process.
	WNDPROC origProc = nullptr;
#ifdef _WIN32
	origProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)Window_Intercept_Callback);
#else
	origProc = (WNDPROC)SetWindowLong(hwnd, GWL_WNDPROC, (LONG_PTR)Window_Intercept_Callback);
#endif
	if (!origProc)
		return -4;

	// Got everything OK.  Finally, store struct.
	sWindowData s; // { origProc, messagesToIntercept, latestMessage };
	s.origProc = origProc;
	s.messagesToIntercept = messagesToIntercept;
	s.latestMessage = latestMessage;
	mapWindowToData.insert(pair<HWND, sWindowData>(hwnd, s));

	return 1;
}

bool Window_ReleaseIntercept(void* windowHWND)
{
	using namespace julian;

	if (mapWindowToData.count((HWND)windowHWND))
	{
		sWindowData windowData = mapWindowToData[(HWND)windowHWND];
		WNDPROC origProc = windowData.origProc;
#ifdef _WIN32
		SetWindowLongPtr((HWND)windowHWND, GWLP_WNDPROC, (LONG_PTR)origProc);
#else
		SetWindowLong((HWND)windowHWND, GWL_WNDPROC, (LONG_PTR)origProc);
#endif
		mapWindowToData.erase((HWND)windowHWND);
		return true;
	}
	else
		return false;
}


void Window_ReleaseAllIntercepts()
{
	using namespace julian;
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
	if ((flags & 1)  && (GetAsyncKeyState(MK_LBUTTON) >> 1))	state |= 1;
	if ((flags & 2)  && (GetAsyncKeyState(MK_RBUTTON) >> 1))	state |= 2;
	if ((flags & 64) && (GetAsyncKeyState(MK_MBUTTON) >> 1))	state |= 64;
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

void* Mouse_SetCapture(void* windowHWND)
{
	return SetCapture((HWND)windowHWND);
}

void* Mouse_GetCapture()
{
	return GetCapture();
}

void Mouse_ReleaseCapture()
{
	ReleaseCapture();
}


