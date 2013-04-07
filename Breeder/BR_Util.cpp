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

bool IsThisFraction (char* tmp, int tmpLen, double &convertedFraction)
{
	replace(tmp, tmp+strlen(tmp), ',', '.' );
	char* buf = strstr(tmp,"/");

	if(!buf)
	{	
		convertedFraction = atof(tmp);
		_snprintf(tmp, tmpLen, "%g", convertedFraction);
		return false;
	}
	else
	{
		double num = atof(tmp);
		double den = atof(buf+1);
		_snprintf(tmp, tmpLen, "%g/%g", num, den);
		if (den != 0)
			convertedFraction = num/den;			
		else
			convertedFraction = 0;
		return true;
	}
};

void CenterWindowInReaper (HWND hwnd, HWND zOrder, bool startUp)
{
	int x = 0, y = 0;
	RECT r; GetWindowRect(hwnd, &r);

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
			x = wnd_x + (wnd_w - r.right  + r.left)/2;
			y = wnd_y + (wnd_h - r.bottom + r.top)/2;
		}
		// Maximized (won't work with multiple displays if reaper is not maximized in primary display...heh)
		else
		{
			int wnd_w = GetSystemMetrics(SM_CXSCREEN);
			int wnd_h = GetSystemMetrics(SM_CYSCREEN);
			x = (wnd_w - r.right  + r.left)/2;
			y = (wnd_h - r.bottom + r.top)/2;
		}
	}
	else
	{
		RECT r1; GetWindowRect(g_hwndParent, &r1);
		x = r1.left + (r1.right  - r1.left - r.right  + r.left)/2;
		y = r1.top +  (r1.bottom - r1.top  - r.bottom + r.top)/2;
	}

	// Ensure coordinates are not offscreen (probably should add cocoa version for ifdef part)
	if (x < 0 || y < 0)
	{
		x = 0;
		y = 0;
	}
#ifdef _WIN32
	if (x > GetSystemMetrics(SM_CXVIRTUALSCREEN) || y > GetSystemMetrics(SM_CYVIRTUALSCREEN))
	{
		x = 0;
		y = 0;
	}
#endif
	SetWindowPos(hwnd, zOrder, x, y, 0, 0, SWP_NOSIZE);
};

int GetFirstDigit (int number)
{
	number = abs(number);
	while (number >= 10)
		number /= 10;
	return number;
};

double AltAtof (char* tmp)
{
	replace(tmp, tmp+strlen(tmp), ',', '.' );
	return atof(tmp);
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

void GetSelItemsInTrack (MediaTrack* track, vector<MediaItem*> &items)
{
	for (int i = 0; i < CountTrackMediaItems(track) ; ++i)
		if (*(bool*)GetSetMediaItemInfo(GetTrackMediaItem(track, i), "B_UISEL", NULL))
			items.push_back(GetTrackMediaItem(track, i));
}