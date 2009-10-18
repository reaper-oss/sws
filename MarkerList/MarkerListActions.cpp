/******************************************************************************
/ MarkerListActions.cpp
/
/ Copyright (c) 2009 Tim Payne (SWS)
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
#include "MarkerListClass.h"
#include "MarkerList.h"
#include "MarkerListActions.h"

void ListToClipboard(COMMAND_T*)
{
	if (!g_curList)
		g_curList = new MarkerList("CurrentList", true);
	else
		g_curList->BuildFromReaper();
	g_curList->ListToClipboard();
}

void ListToClipboardTimeSel(COMMAND_T*)
{
	MarkerList ml("Cropped", true);
	ml.CropToTimeSel(true);
	ml.ListToClipboard();
}

void ClipboardToList(COMMAND_T*)
{
	MarkerList list("Clipboard", false);
	list.ClipboardToList();
	pMarkerList->Update();
}

void ExportToClipboard(COMMAND_T*)
{
	char format[256];
	GetPrivateProfileString(SWS_INI, TRACKLIST_FORMAT_KEY, TRACKLIST_FORMAT_DEFAULT, format, 256, get_ini_file());

	if (!g_curList)
		g_curList = new MarkerList("CurrentList", true);
	else
		g_curList->BuildFromReaper();

	g_curList->ExportToClipboard(format);
}

void DeleteAllMarkers(COMMAND_T*)
{
	bool bReg;
	int iID;
	Undo_BeginBlock();
	while (EnumProjectMarkers(0, &bReg, NULL, NULL, NULL, &iID))
		if (!bReg)
			DeleteProjectMarker(NULL, iID, false);
	Undo_EndBlock("Delete all markers", UNDO_STATE_MISCCFG);
	pMarkerList->Update();
}

void DeleteAllRegions(COMMAND_T*)
{
	bool bReg;
	int iID;
	Undo_BeginBlock();
	while (EnumProjectMarkers(0, &bReg, NULL, NULL, NULL, &iID))
		if (bReg)
			DeleteProjectMarker(NULL, iID, true);
	Undo_EndBlock("Delete all regions", UNDO_STATE_MISCCFG);
	pMarkerList->Update();
}

void RenumberIds(COMMAND_T*)
{
	MarkerList ml(NULL, true);
	DeleteAllMarkers();
	for (int i = 0; i < ml.m_items.GetSize(); i++)
	{
		MarkerItem* mi = ml.m_items.Get(i);
		if (!mi->m_bReg)
			AddProjectMarker(NULL, false, mi->m_dPos, mi->m_dRegEnd, mi->GetName(), i+1);
	}
	pMarkerList->Update();
}

void RenumberRegions(COMMAND_T*)
{
	MarkerList ml(NULL, true);
	DeleteAllMarkers();
	for (int i = 0; i < ml.m_items.GetSize(); i++)
	{
		MarkerItem* mi = ml.m_items.Get(i);
		if (mi->m_bReg)
			AddProjectMarker(NULL, true, mi->m_dPos, mi->m_dRegEnd, mi->GetName(), i+1);
	}
	pMarkerList->Update();
}

void SelNextRegion(COMMAND_T*)
{
	double dCurPos, d2;
	GetSet_LoopTimeRange(false, true, &dCurPos, &d2, false);
	if (dCurPos == d2)
		dCurPos = GetCursorPosition();

	int x = 0;
	bool bReg;
	double dRegStart;
	double dRegEnd;
	while ((x = EnumProjectMarkers(x, &bReg, &dRegStart, &dRegEnd, NULL, NULL)))
	{
		if (bReg && dRegStart > dCurPos)
		{
			GetSet_LoopTimeRange(true, true, &dRegStart, &dRegEnd, false);
			return;
		}
	}
	// Nothing found, loop again and set to first region
	while ((x = EnumProjectMarkers(x, &bReg, &dRegStart, &dRegEnd, NULL, NULL)))
	{
		if (bReg)
		{
			GetSet_LoopTimeRange(true, true, &dRegStart, &dRegEnd, false);
			return;
		}
	}
}

void SelPrevRegion(COMMAND_T*)
{
	double dCurPos, d2;
	GetSet_LoopTimeRange(false, true, &dCurPos, &d2, false);
	if (dCurPos == d2)
		dCurPos = GetCursorPosition();

	int x = 0;
	bool bReg;
	double d1, dRegStart, dRegEnd;
	bool bFound = false;
	bool bRegions = false;
	while ((x = EnumProjectMarkers(x, &bReg, &d1, &d2, NULL, NULL)))
	{
		if (bReg)
		{
			bRegions = true;
			if (dCurPos > d1)
			{
				dRegStart = d1;
				dRegEnd = d2;
				bFound = true;
			}
		}
	}
	if (bFound)
		GetSet_LoopTimeRange(true, true, &dRegStart, &dRegEnd, false);
	else if (bRegions)
		GetSet_LoopTimeRange(true, true, &d1, &d2, false);
}