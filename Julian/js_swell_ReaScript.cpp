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
#include "js_swell_ReaScript.h"


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
	/*
	bool isOK = !!ScreenToClient(hwnd, &p);
	if (isOK)
	{
		*xOut = (int)p.x;
		*yOut = (int)p.y;
	}
	return isOK;
	*/
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


namespace julian
{
	struct EnumWindowsStruct
	{
		const char* target;
		bool		exact;
		char*		temp;
		int			tempLen;
		HWND		hwnd;
		char*		hwndString;
		int			hwndLen;
	};
	const int TEMP_LEN = 65;	// For temporary storage of pointer strings.
	const int API_LEN  = 1024;  // Maximum length of strings passed between Lua and API.
	const int EXT_LEN  = 16000; // What is the maximum length of ExtState strings? 
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
		sprintf_s(s->temp + 2, s->tempLen - 3, "%p,", hwnd);
		// Replace leading 0's with 0x
		int i = 2; while (s->temp[i] == '0') i++;	// Find first non-'0' character
		s->temp[i - 2] = '0';
		s->temp[i - 1] = 'x';
		// Concatenate to hwndString
		if (strlen(s->hwndString) + strlen(s->temp + i - 2) < s->hwndLen)
		{
			strcat(s->hwndString, s->temp + i - 2);
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
		sprintf_s(s->temp + 2, s->tempLen - 3, "%p,", hwnd);
		// Replace leading 0's with 0x
		int i = 2; while (s->temp[i] == '0') i++;	// Find first non-'0' character
		s->temp[i - 2] = '0';
		s->temp[i - 1] = 'x';
		// Concatenate to hwndString
		if (strlen(s->hwndString) + strlen(s->temp + i - 2) < s->hwndLen)
		{
			strcat(s->hwndString, s->temp + i - 2);
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
	sprintf_s(temp + 2, TEMP_LEN - 3, "%p,", hwnd); // Leave two spaces for 0x, add comma separator
	// Remove leading 0's
	int i = 2; while (temp[i] == '0') i++;	// Find first non-'0' character
	temp[i - 2] = '0';
	temp[i - 1] = 'x';
	if (strlen(hwndString) + strlen(temp + i - 2) < EXT_LEN)
	{
		strcat(hwndString, temp + i-2);
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
	sprintf_s(temp + 2, TEMP_LEN - 3, "%p,", hwnd); // Leave two spaces for 0x, add comma separator
	// Remove leading 0's
	int i = 2; while (temp[i] == '0') i++;	// Find first non-'0' character
	temp[i - 2] = '0';
	temp[i - 1] = 'x';
	if (strlen(hwndString) + strlen(temp + i-2) < EXT_LEN)
	{
		strcat(hwndString, temp + i-2);
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



BOOL CALLBACK MIDIEditor_ListAll_Callback_Child(HWND hwnd, LPARAM structPtr)
{
	using namespace julian;
	if (MIDIEditor_GetMode(hwnd) != -1) // Is MIDI editor?
	{
		char* hwndString = reinterpret_cast<char*>(structPtr);
		char temp[TEMP_LEN] = "";
		sprintf_s(temp + 2, TEMP_LEN - 3, "%p,", hwnd); // Leave two spaces for 0x, add comma separator
		// Remove leading 0's
		int i = 2; while (temp[i] == '0') i++;	// Find first non-'0' character
		temp[i - 2] = '0';
		temp[i - 1] = 'x';
		if (strstr(hwndString, temp + i - 2) == NULL) // Match with bounding 0x and comma
		{
			if ((strlen(hwndString) + strlen(temp + i - 2)) < API_LEN)
			{
				strcat(hwndString, temp + i - 2);
			}
			else
				return FALSE;
		}
	}
	return TRUE;
}

BOOL CALLBACK MIDIEditor_ListAll_Callback_Top(HWND hwnd, LPARAM structPtr)
{
	using namespace julian;
	if (MIDIEditor_GetMode(hwnd) != -1) // Is MIDI editor?
	{
		char* hwndString = reinterpret_cast<char*>(structPtr);
		char temp[TEMP_LEN] = "";
		sprintf_s(temp + 2, TEMP_LEN - 3, "%p,", hwnd); // Leave two spaces for 0x, add comma separator
		// Remove leading 0's
		int i = 2; while (temp[i] == '0') i++;	// Find first non-'0' character
		temp[i - 2] = '0';
		temp[i - 1] = 'x';
		if (strstr(hwndString, temp + i - 2) == NULL) // Match with bounding 0x and comma
		{
			if ((strlen(hwndString) + strlen(temp+i-2)) < API_LEN)
			{
				strcat(hwndString, temp + i-2);
			}
			else
				return FALSE;
		}
	}
	else // Check if any child windows are MIDI editor. For example if docked in docker or main window.
	{
		EnumChildWindows(hwnd, MIDIEditor_ListAll_Callback_Child, structPtr);
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
	HWND hwnd = (HWND)windowHWND;
	SetWindowPos(hwnd, NULL, left, top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOSIZE);
}

void Window_Resize(void* windowHWND, int width, int height)
{
	HWND hwnd = (HWND)windowHWND;
	SetWindowPos(hwnd, NULL, 0, 0, width, height, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE);
}



bool Window_SetTitle(void* windowHWND, const char* title)
{
	return !!SetWindowText((HWND)windowHWND, title);
}

void Window_GetTitle(void* windowHWND, char* buf, int buf_sz)
{
	GetWindowText((HWND)windowHWND, buf, buf_sz);
}



void* Window_HandleFromAddress(int address)
{
	return (HWND)address;
}

bool  Window_IsWindow(void* windowHWND)
{
	return !!IsWindow((HWND)windowHWND);
}



void* GDI_GetDC(void* windowHWND)
{
	return GetDC((HWND)windowHWND);
}

void  GDI_ReleaseDC(void* windowHWND, void* deviceHDC)
{
	ReleaseDC((HWND)windowHWND, (HDC)deviceHDC);
}

void  GDI_Rectangle(void* deviceHDC, int left, int top, int right, int bottom)
{
	Rectangle((HDC)deviceHDC, left, top, right, bottom);
}



int  Mouse_GetState(int key)
{
	// If key == 0, return all states in same format as gfx.mouse_cap.
	// Otherwise, use key to return state of single key or button.
	
	// Least significant bit will be removed from return values, 
	//		since only interested in buttons that are currently down.
	if (key)
		return (GetAsyncKeyState(key)>>1);
	else
	{
		int state = 0;
		if (GetAsyncKeyState(MK_LBUTTON)>>1)	state |= 1;
		if (GetAsyncKeyState(MK_RBUTTON)>>1)	state |= 2;
		if (GetAsyncKeyState(MK_MBUTTON)>>1)	state |= 64;
		if (GetAsyncKeyState(VK_CONTROL)>>1)	state |= 4;
		if (GetAsyncKeyState(VK_SHIFT)>>1)		state |= 8;
		if (GetAsyncKeyState(VK_MENU)>>1)		state |= 16;
		if (GetAsyncKeyState(VK_LWIN)>>1)		state |= 32;
		return state;
	}
}

bool Mouse_SetPosition(int x, int y)
{
	return !!SetCursorPos(x, y);
}
