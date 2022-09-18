/******************************************************************************
/ MarkerListActions.cpp
/
/ Copyright (c) 2012 Tim Payne (SWS)
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

#include "MarkerListClass.h"
#include "MarkerList.h"
#include "MarkerListActions.h"
#include "../SnM/SnM_Project.h"

#include <WDL/localize/localize.h>

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
	g_pMarkerList->Update();
}

void ExportToClipboard(COMMAND_T*)
{
	char format[256];
	GetPrivateProfileString(SWS_INI, EXPORT_FORMAT_KEY, EXPORT_FORMAT_DEFAULT, format, 256, get_ini_file());

	if (!g_curList)
		g_curList = new MarkerList("CurrentList", true);
	else
		g_curList->BuildFromReaper();

	g_curList->ExportToClipboard(format);
}

void ExportToFile(COMMAND_T*)
{
	char format[256];
	GetPrivateProfileString(SWS_INI, EXPORT_FORMAT_KEY, EXPORT_FORMAT_DEFAULT, format, 256, get_ini_file());

	if (!g_curList)
		g_curList = new MarkerList("CurrentList", true);
	else
		g_curList->BuildFromReaper();

	g_curList->ExportToFile(format);
}

void DeleteAllMarkers()
{
	bool bReg;
	int iID;
	int x = 0, lastx = 0;
	while ((x = EnumProjectMarkers(x, &bReg, NULL, NULL, NULL, &iID)))
	{
		if (!bReg)
		{
			DeleteProjectMarker(NULL, iID, false);
			x = lastx;
		}
		lastx = x;
	}
}

// Command version with undo point and wnd update
void DeleteAllMarkers(COMMAND_T* ct)
{
	DeleteAllMarkers();
	Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_MISCCFG, -1);
	g_pMarkerList->Update();
}

void DeleteAllRegions()
{
	bool bReg;
	int iID;
	int x = 0, lastx = 0;
	while ((x = EnumProjectMarkers(x, &bReg, NULL, NULL, NULL, &iID)))
	{
		if (bReg)
		{
			DeleteProjectMarker(NULL, iID, true);
			x = lastx;
		}
		lastx = x;
	}
}

// Command version with undo point and wnd update
void DeleteAllRegions(COMMAND_T* ct)
{
	DeleteAllRegions();
	Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_MISCCFG, -1);
	g_pMarkerList->Update();
}

void RenumberIds(COMMAND_T* ct)
{
	MarkerList ml(NULL, true);
	DeleteAllMarkers();
	int iID = 1;
	for (int i = 0; i < ml.m_items.GetSize(); i++)
	{
		MarkerItem* mi = ml.m_items.Get(i);
		if (!mi->IsRegion())
		{
			mi->SetNum(iID++);
			mi->AddToProject();
		}
	}
	g_pMarkerList->Update();
	UpdateTimeline();
	Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_MISCCFG, -1);
}

void RenumberRegions(COMMAND_T* ct)
{
	MarkerList ml(NULL, true);
	DeleteAllRegions();
	int iID = 1;
	for (int i = 0; i < ml.m_items.GetSize(); i++)
	{
		MarkerItem* mi = ml.m_items.Get(i);
		if (mi->IsRegion())
		{
			mi->SetNum(iID++);
			mi->AddToProject();
		}
	}
	g_pMarkerList->Update();
	UpdateTimeline();
	Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_MISCCFG, -1);
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
			GetSet_LoopTimeRange(true, false, &dRegStart, &dRegEnd, false);
			return;
		}
	}
	// Nothing found, loop again and set to first region
	while ((x = EnumProjectMarkers(x, &bReg, &dRegStart, &dRegEnd, NULL, NULL)))
	{
		if (bReg)
		{
			GetSet_LoopTimeRange(true, false, &dRegStart, &dRegEnd, false);
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
		GetSet_LoopTimeRange(true, false, &dRegStart, &dRegEnd, false);
	else if (bRegions)
		GetSet_LoopTimeRange(true, false, &d1, &d2, false);
}

// Goto the end of the project, *including* perhaps marker ends
void GotoEndInclMarkers(COMMAND_T*)
{
	Main_OnCommand(40043, 0);
	int x = 0;
	double dRegStart, dRegEnd;
	bool bReg;
	double dMarkerEnd = -DBL_MAX;
	while ((x = EnumProjectMarkers(x, &bReg, &dRegStart, &dRegEnd, NULL, NULL)))
	{
		if (bReg && dRegEnd > dMarkerEnd)
			dMarkerEnd = dRegEnd;
		else if (!bReg && dRegStart > dMarkerEnd)
			dMarkerEnd = dRegStart;
	}
	if (dMarkerEnd > GetCursorPosition())
		SetEditCurPos(dMarkerEnd, true, true);
}

void SelNextMarkerOrRegion(COMMAND_T*)
{
	double dCurPos = GetCursorPosition();
	double dCurStart, dCurEnd, dRegStart, dRegEnd;
	GetSet_LoopTimeRange(false, false, &dCurStart, &dCurEnd, false);
	int x = 0;
	bool bReg;
	while ((x = EnumProjectMarkers(x, &bReg, &dRegStart, &dRegEnd, NULL, NULL)))
	{
		bool bSelMatches = dCurStart == dRegStart && dCurEnd == dRegEnd;
		if (dRegStart > dCurPos || (bReg && dRegStart >= dCurPos && !bSelMatches))
		{
			GetSet_LoopTimeRange(true, false, &dRegStart, bReg ? &dRegEnd : &dRegStart, false);
			SetEditCurPos(dRegStart, true, true);
			return;
		}
	}
	// Currently no wraparound, if needed add "go to first" here.
}

void SelPrevMarkerOrRegion(COMMAND_T*)
{
	// Save the current marker list so we can traverse the list bacwards
	MarkerList ml(NULL, true);

	double dCurPos = GetCursorPosition();
	double dCurStart, dCurEnd;
	GetSet_LoopTimeRange(false, false, &dCurStart, &dCurEnd, false);
	bool bCurSel = dCurStart != dCurEnd;
	for (int i = ml.m_items.GetSize()-1; i >= 0; i--)
	{
		MarkerItem* mi = ml.m_items.Get(i);
		if (mi->GetPos() < dCurPos || (!mi->IsRegion() && mi->GetPos() <= dCurPos && bCurSel))
		{
			double dNewStart = mi->GetPos(), dNewEnd = mi->GetRegEnd();
			GetSet_LoopTimeRange(true, false, &dNewStart, mi->IsRegion() ? &dNewEnd : &dNewStart, false);
			SetEditCurPos(mi->GetPos(), true, true);
			return;
		}
	}
}

void MarkersToRegions(COMMAND_T* ct)
{
	MarkerList ml(NULL, true);

	WDL_PtrList<MarkerItem> &markers = ml.m_items;

	if(markers.GetSize() == 0) return; // Bail if there are no markers/regions

	double projEnd = SNM_GetProjectLength();

	Undo_BeginBlock2(NULL);

	// Insert dummy marker at project start if necessary
	if(markers.Get(0)->GetPos() > 0)
	{
		markers.Insert(0, new MarkerItem(false, 0, 0, "", 0, 0));
	}

	// Convert markers to regions
	for(int i = 0, c = markers.GetSize(); i < c; i++)
	{
		MarkerItem *pm = markers.Get(i);
		if(!pm->IsRegion())
		{
			// Find next marker
			MarkerItem *pNext = NULL;
			int n = i + 1;
			do
			{
				pNext = markers.Get(n++);
			}
			while(pNext && pNext->IsRegion());

			double pos = pm->GetPos();
			double end = pNext ? pNext->GetPos() : projEnd;

			if(pos != end)
			{
				pm->SetReg(true);
				pm->SetRegEnd(end);
			}
		}
	}

	ml.UpdateReaper();

	Undo_EndBlock2(NULL, __LOCALIZE("Convert markers to regions","sws_undo"), UNDO_STATE_MISCCFG);
}


void RegionsToMarkers(COMMAND_T*)
{
	MarkerList ml(NULL, true);

	WDL_PtrList<MarkerItem> &markers = ml.m_items;

	if(markers.GetSize() == 0) return; // Bail if there are no markers/regions

	Undo_BeginBlock2(NULL);

	for(int i = 0, c = markers.GetSize(); i < c; i++)
	{
		MarkerItem *pm = markers.Get(i);

		if(pm->IsRegion())
		{
			pm->SetReg(false);
		}
	}

	ml.UpdateReaper();

	Undo_EndBlock2(NULL, __LOCALIZE("Convert regions to markers","sws_undo"), UNDO_STATE_MISCCFG);
}
