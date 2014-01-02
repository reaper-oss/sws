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
#include "../SnM/SnM_Dlg.h"

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

void CenterDialog (HWND hwnd, HWND target, HWND zOrder)
{
	RECT r; GetWindowRect(hwnd, &r);
	RECT r1; GetWindowRect(target, &r1);
	int hwndW = r.right - r.left;
	int hwndH = r.bottom - r.top;
	int targW = r1.right - r1.left;	
	int targH = r1.bottom - r1.top;

	r.left = r1.left + (targW-hwndW)/2;
	r.top = r1.top + (targH-hwndH)/2;
	r.right = r.left + hwndW;
	r.bottom = r.top + hwndH;

	EnsureNotCompletelyOffscreen(&r);
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

void BR_ThemeListViewOnInit (HWND list)
{
	SWS_ListView listView(list, NULL, 0, NULL, NULL, false, NULL, true);
	SNM_ThemeListView(&listView);
};

bool BR_ThemeListViewInProc (HWND hwnd, int uMsg, LPARAM lParam, HWND list, bool grid)
{
	if (SWS_THEMING)
	{
		int colors[LISTVIEW_COLORHOOK_STATESIZE];
		int columns = (grid) ? (Header_GetItemCount(ListView_GetHeader(list))) : (0);
		if (ListView_HookThemeColorsMessage(hwnd, uMsg, lParam, colors, GetWindowLong(list,GWL_ID), 0, columns))
			return true;
	}
	return false;
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

bool BR_SetTakeSourceFromFile(MediaItem_Take* take, char* filename, bool inProjectData)
{
	if (take && file_exists(filename))
	{
		if (PCM_source* oldSource = (PCM_source*)GetSetMediaItemTakeInfo(take,"P_SOURCE",NULL))
		{
			PCM_source* newSource = PCM_Source_CreateFromFileEx(filename, !inProjectData);
			GetSetMediaItemTakeInfo(take,"P_SOURCE", newSource);
			delete oldSource;
			return true;
		}
	}
	return false;
};