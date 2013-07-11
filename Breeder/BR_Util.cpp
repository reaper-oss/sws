/******************************************************************************
/ BR_Util.cpp
/
/ Copyright (c) 2013 Dominik Martin Drzic
/ http://forum.cockos.com/member.php?u=27094
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
#include "stdafx.h"
#include "BR_Util.h"

bool IsThisFraction (char* str, double &convertedFraction)
{
	replace(str, str+strlen(str), ',', '.' );
	char* buf = strstr(str,"/");

	if (!buf)
	{	
		convertedFraction = atof(str);
		_snprintf(str, strlen(str)+1, "%g", convertedFraction);
		return false;
	}
	else
	{
		int num = atoi(str);
		int den = atoi(buf+1);
		_snprintf(str, strlen(str)+1, "%d/%d", num, den);
		if (den != 0)
			convertedFraction = (double)num/(double)den;			
		else
			convertedFraction = 0;
		return true;
	}
};

int GetFirstDigit (int val)
{
	val = abs(val);
	while (val >= 10)
		val /= 10;
	return val;
};

int ClearBit (int val, int pos)
{
	return val & ~(1 << pos);
};

int SetBit (int val, int pos)
{
	return val | 1 << pos;
};

double AltAtof (char* str)
{
	replace(str, str+strlen(str), ',', '.' );
	return atof(str);
};

double EndOfProject (bool markers, bool regions)
{
	double projEnd = 0;
	int tracks = GetNumTracks();
	for (int i = 0; i < tracks; i++)
	{
		MediaTrack* track = GetTrack(0, i);
		MediaItem* item = GetTrackMediaItem(track, GetTrackNumMediaItems(track) - 1);
		double itemEnd = GetMediaItemInfo_Value(item, "D_POSITION") + GetMediaItemInfo_Value(item, "D_LENGTH");
		if (itemEnd > projEnd)
			projEnd = itemEnd;
	}

	if (markers || regions)
	{
		bool region; double start, end; int i = 0;
		while (i = EnumProjectMarkers(i, &region, &start, &end, NULL, NULL))
		{
			if (regions)
				if (region && end > projEnd)
					projEnd = end;
			if (markers)
				if (!region && start > projEnd)
					projEnd = start;
		}
	}
	return projEnd;
};

void CenterWindowInReaper (HWND hwnd, HWND zOrder, bool startUp)
{
	RECT r; GetWindowRect(hwnd, &r);
	int width = r.right-r.left;
	int height = r.bottom-r.top; 

	// On startup GetWindowRect could provide the wrong data so we read the .ini instead
	if (startUp)
	{
		// Not maximized
		if (GetPrivateProfileInt("Reaper", "wnd_state", 1, get_ini_file()) == 0)
		{
			int wnd_x = GetPrivateProfileInt("Reaper", "wnd_x", 1, get_ini_file());
			int wnd_y = GetPrivateProfileInt("Reaper", "wnd_y", 1, get_ini_file());
			int wnd_w = GetPrivateProfileInt("Reaper", "wnd_w", 1, get_ini_file());
			int wnd_h = GetPrivateProfileInt("Reaper", "wnd_h", 1, get_ini_file());
			r.left = wnd_x + (wnd_w - r.right  + r.left)/2;
			r.top = wnd_y + (wnd_h - r.bottom + r.top)/2;
		}
		// Maximized (won't work with multiple displays if reaper is not maximized in primary display...heh)
		else
		{
			int wnd_w = GetSystemMetrics(SM_CXSCREEN);
			int wnd_h = GetSystemMetrics(SM_CYSCREEN);
			r.left = (wnd_w - r.right  + r.left)/2;
			r.top = (wnd_h - r.bottom + r.top)/2;
		}
	}
	else
	{
		RECT r1; GetWindowRect(g_hwndParent, &r1);
		r.left = r1.left + (r1.right  - r1.left - r.right  + r.left)/2;
		r.top = r1.top +  (r1.bottom - r1.top  - r.bottom + r.top)/2;
	}

	#ifdef _WIN32
	if (r.left < 0 || r.top < 0 || r.left+width > GetSystemMetrics(SM_CXVIRTUALSCREEN) || r.top+height > GetSystemMetrics(SM_CYVIRTUALSCREEN))
	{
		r.left = 0;
		r.top = 0;
	}
	#else
		EnsureNotCompletelyOffscreen(&r); // not really working with multiple monitors (treats as offscreen if not on the default monitor)
	#endif
	SetWindowPos(hwnd, zOrder, r.left, r.top, 0, 0, SWP_NOSIZE);
};

void GetSelItemsInTrack (MediaTrack* track, vector<MediaItem*> &items)
{
	for (int i = 0; i < CountTrackMediaItems(track) ; ++i)
		if (*(bool*)GetSetMediaItemInfo(GetTrackMediaItem(track, i), "B_UISEL", NULL))
			items.push_back(GetTrackMediaItem(track, i));
};

void ReplaceAll (string &str, string oldStr, string newStr)
{
	if (oldStr.empty())
		return;
	
	size_t pos = 0, oldLen = oldStr.length(), newLen = newStr.length();
	while ((pos = str.find(oldStr, pos)) != std::string::npos) 
	{
		str.replace(pos, oldLen, newStr);
		pos += newLen;
	}
};

void CommandTimer (COMMAND_T* ct)
{
	// Start timer
	#ifdef WIN32
		LARGE_INTEGER ticks, start, end;
		QueryPerformanceFrequency(&ticks);
		QueryPerformanceCounter(&start);
	#else
		timeval start, end;
		gettimeofday(&start, NULL);
	#endif

	// Run command
	ct->doCommand(ct);

	// Stop timer
	#ifdef WIN32
		QueryPerformanceCounter(&end);
		int msTime = (int)((double)(end.QuadPart - start.QuadPart) * 1000 / (double)ticks.QuadPart + 0.5);
	#else
		gettimeofday(&end, NULL);
		int msTime = (int)((double)(end.tv_sec - start.tv_sec) * 1000 + (double)(end.tv_usec - start.tv_usec) / 1000 + 0.5);
	#endif

	// Print result to console
	int cmd = ct->accel.accel.cmd;
	const char* name = kbd_getTextFromCmd(cmd, NULL);	
	
	WDL_FastString string;
	string.AppendFormatted(256, "%d ms to execute: %s\n", msTime, name);
	ShowConsoleMsg(string.Get());
};