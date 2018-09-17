#include "stdafx.h"
#include "dopp.h"
#include "../Breeder/BR_Util.h"

// just exposing some useful functions to REAPER. 

void DO_GetArrangeVPos(int* ViewPosOut)
{
	HWND arrhwnd = GetArrangeWnd();
	SCROLLINFO si = { sizeof(SCROLLINFO), };
	si.fMask = SIF_ALL;
	CoolSB_GetScrollInfo(arrhwnd, SB_VERT, &si);
	*ViewPosOut = si.nPos;
}

// can be useful in linux
void* DO_GetArrangeHwnd()
{
	return GetArrangeWnd();
}

void* DO_GetTCPTrackHwnd(MediaTrack* track)
{
	HWND hwnd = GetWindow(GetTcpWnd(), GW_CHILD);
	do
	{
		if ((MediaTrack*)GetWindowLongPtr(hwnd, GWLP_USERDATA) == track)
			return hwnd;
	} while ((hwnd = GetWindow(hwnd, GW_HWNDNEXT)));
	return NULL;
}
