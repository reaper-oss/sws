/******************************************************************************
/ MarkerListClass.cpp
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

#include "../Utility/SectionLock.h"
#include "MarkerListClass.h"
#include "MarkerListActions.h"
#include "../SnM/SnM_Project.h"

#include <WDL/localize/localize.h>
#include <WDL/projectcontext.h>

MarkerItem::MarkerItem(bool bReg, double dPos, double dRegEnd, const char* cName, int num, int color)
{
	m_bReg = bReg;
	m_dPos = dPos;
	m_dRegEnd = bReg ? dRegEnd : -1.0;
	m_num = num;
	SetName(cName);
	m_iColor = color;
}

MarkerItem::MarkerItem(LineParser* lp)
{
	m_num     = lp->gettoken_int(0);
	m_dPos    = lp->gettoken_float(1);
	SetName(lp->gettoken_str(2));
	m_bReg    = lp->gettoken_int(3) ? true : false;
	m_dRegEnd = lp->gettoken_float(4);
	m_iColor  = lp->gettoken_int(5);
}

char* MarkerItem::ItemString(char* str, int iSize)
{
	WDL_FastString name;
	makeEscapedConfigString(GetName(), &name);
	snprintf(str, iSize, "%d %.14f %s %d %.14f %d", m_num, m_dPos, name.Get(), m_bReg ? 1 : 0, m_dRegEnd, m_iColor);
	return str;
}

bool MarkerItem::Compare(bool bReg, double dPos, double dRegEnd, const char* cName, int num, int color)
{
	return (bReg == m_bReg && dPos == m_dPos && num == m_num && (!bReg || dRegEnd == m_dRegEnd) && m_iColor == color && strcmp(cName, GetName()) == 0);
}

bool MarkerItem::Compare(MarkerItem* mi)
{
	return Compare(mi->IsRegion(), mi->GetPos(), mi->GetRegEnd(), mi->GetName(), mi->m_num, mi->m_iColor);
}

void MarkerItem::AddToProject()
{
	AddProjectMarker2(NULL, m_bReg, m_dPos, m_dRegEnd, GetName(), m_num, m_iColor ? m_iColor | 0x1000000 : 0);
}

void MarkerItem::UpdateProject()
{
	SetProjectMarker4(NULL, m_num, m_bReg, m_dPos, m_dRegEnd, GetName(), m_iColor ? m_iColor | 0x1000000 : 0, !*GetName() ? 1 : 0);
}

MarkerList::MarkerList(const char* name, bool bGetCurList)
{
	if (name && strlen(name))
	{
		m_name = new char[strlen(name)+1];
		strcpy(m_name, name);
	}
	else
		m_name = NULL;

	m_items.Empty(true);

	if (bGetCurList)
		BuildFromReaper();
}

MarkerList::~MarkerList()
{
	SWS_SectionLock lock(&m_mutex);
	m_items.Empty(true);
	delete[] m_name;
}

// return true if something has changed!
bool MarkerList::BuildFromReaper()
{
	SWS_SectionLock lock(&m_mutex);

	// Instead of emptying and starting over, try to just add or delete for simple cases (added/deleted marker)
	// markers moved in time are reallocated
	// m_items maintains the same ordering as EnumProjectMarkers
	int id, x = 0, i = 0, iColor = 0;
	bool bR, bChanged = false;
	double dPos, dRend;
	const char *cName;

	while ((x=EnumMarkers(x, &bR, &dPos, &dRend, &cName, &id, &iColor)))
	{
		if (i >= m_items.GetSize() || !m_items.Get(i)->Compare(bR, dPos, dRend, cName ? cName : "", id, iColor))
		{ // not found, try ahead one more
			if (i+1 >= m_items.GetSize() || !m_items.Get(i+1)->Compare(bR, dPos, dRend, cName, id, iColor))
			{	// not found one ahead, assume new and insert at current position
				m_items.Insert(i, new MarkerItem(bR, dPos, dRend, cName, id, iColor));
				bChanged = true;
			}
			else
			{	// found one ahead, the current one must have been deleted
				m_items.Delete(i, true);
				bChanged = true;
			}
		}
		i++;
	}
	while(i < m_items.GetSize())
	{
		bChanged = true;
		m_items.Delete(i, true);
	}

	return bChanged;
}

void MarkerList::UpdateReaper()
{	// Function to take content of list and update Reaper environment
	// First delete all markers/regions, then regen from list
	bool bReg;
	int iID;
	while (EnumProjectMarkers(0, &bReg, NULL, NULL, NULL, &iID))
		DeleteProjectMarker(NULL, iID, bReg);

	SWS_SectionLock lock(&m_mutex);

	for (int i = 0; i < m_items.GetSize(); i++)
		m_items.Get(i)->AddToProject();

	UpdateTimeline();
}

void MarkerList::ListToClipboard()
{
	SWS_SectionLock lock(&m_mutex);
	if (OpenClipboard(g_hwndParent))
	{
		int iSize = ApproxSize()*2;
		char* str = new char[iSize];
		char* pStr = str;
		str[0] = 0;
		for (int i = 0; i < m_items.GetSize(); i++)
		{
			m_items.Get(i)->ItemString(pStr, iSize-(int)(pStr-str));
			pStr += strlen(pStr);
			lstrcpyn(pStr, "\r\n", iSize-(int)(pStr-str));
			pStr += 2;
		}
	    EmptyClipboard();
		
		// Not sure what the HGLOBAL deal is but it's straight from the help on SetClipboardData
		HGLOBAL hglbCopy; 
        hglbCopy = GlobalAlloc(GMEM_MOVEABLE, strlen(str)+1); 
		if (hglbCopy)
		{
			memcpy(GlobalLock(hglbCopy), str, strlen(str)+1);	
			GlobalUnlock(hglbCopy);
			SetClipboardData(CF_TEXT, hglbCopy); 
		}
		CloseClipboard();
		delete [] str;
	}
}

void MarkerList::ClipboardToList()
{
	SWS_SectionLock lock(&m_mutex);
	if (OpenClipboard(g_hwndParent))
	{
		m_items.Empty(true);
		LineParser lp(false);
		HGLOBAL clipBoard = GetClipboardData(CF_TEXT);
		char* data = NULL;
		if (clipBoard)
		{
			char* clipData = (char*)GlobalLock(clipBoard);
			if (clipData)
			{
				data = new char[strlen(clipData)+10];
				strcpy(data, clipData);
				GlobalUnlock(clipBoard);
			}
		}
		CloseClipboard();
		if (!data)
			return;

		char* line = strtok(data, "\r\n");
		while(line)
		{
			if (!lp.parse(line) && lp.getnumtokens() >= 5)
				m_items.Add(new MarkerItem(&lp));
			line = strtok(NULL, "\r\n");
		}
		delete [] data;

		if (m_items.GetSize())
			UpdateReaper();
	}
}

char* MarkerList::GetFormattedList(const char* format) // Must delete [] returned string
{
	SWS_SectionLock lock(&m_mutex);
	int iLen = ApproxSize()*2;
	char* str = new char[iLen];
	str[0] = 0;

	double dEnd = SNM_GetProjectLength();
	char* s = str;
	int count = 1;

	for (int i = 0; i < m_items.GetSize(); i++)
	{
		if (format[0] == 'a' ||
			(format[0] == 'r' && m_items.Get(i)->IsRegion()) ||
			(format[0] == 'm' && !m_items.Get(i)->IsRegion()))
		{
			// Cheat a bit and use the region end variable for markers as "location of next marker or eop"
			if (!m_items.Get(i)->IsRegion())
			{
				if (i < m_items.GetSize() - 1)
					m_items.Get(i)->SetRegEnd(m_items.Get(i+1)->GetPos());
				else
					m_items.Get(i)->SetRegEnd(dEnd);
			}

			for (unsigned int j = 1; j < strlen(format); j++)
			{
				switch(format[j])
				{
				case 'n':
					s += sprintf(s, "%d", count++);
					break;
				case 'i':
					s += sprintf(s, "%d", m_items.Get(i)->GetNum());
					break;
				case 'l':
				{
					double len = m_items.Get(i)->GetRegEnd() - m_items.Get(i)->GetPos();
					if (len < 0.0)
						len = 0.0;
					format_timestr_pos(len, s, (int)(iLen-(s-str)), 5);
					s += strlen(s)-3;
					s[0] = 0;
					break;
				}
				case 'd':
					s += sprintf(s, "%s", m_items.Get(i)->GetName());
					break;
				case 't':
					format_timestr_pos(m_items.Get(i)->GetPos(), s, (int)(iLen-(s-str)), 5);
					s += strlen(s)-3;
					s[0] = 0;
					break;
				case 'T':
					format_timestr_pos(m_items.Get(i)->GetPos(), s, (int)(iLen-(s-str)), 5);
					s += strlen(s);
					// Change the final : to a .
					s[-3] = '.';
					break;
				case 's':
					format_timestr_pos(m_items.Get(i)->GetPos(), s, (int)(iLen-(s-str)), 4);
					s += strlen(s);
					break;
				case 'p':
					format_timestr_pos(m_items.Get(i)->GetPos(), s, (int)(iLen-(s-str)), -1);
					s += strlen(s);
					break;
				case '\\':
					j++;
					s[0] = format[j];
					s++;
					break;
				default:
					s[0] = format[j];
					s[1] = 0;
					s++;
				}
			}
			strcpy(s, "\r\n");
			s += 2;
		}
	}
	return str;
}


void MarkerList::ExportToClipboard(const char* format)
{

	char* str = GetFormattedList(format);
	
	if (!str || !strlen(str) || !OpenClipboard(g_hwndParent))
	{
		delete [] str;
		return;
	}

	EmptyClipboard();
	HGLOBAL hglbCopy;
#ifdef _WIN32
	#if !defined(WDL_NO_SUPPORT_UTF8)
	if (WDL_HasUTF8(str))
	{
		DWORD size;
		WCHAR* wc = WDL_UTF8ToWC(str, false, 0, &size);
		hglbCopy = GlobalAlloc(GMEM_MOVEABLE, size*sizeof(WCHAR)); 
		memcpy(GlobalLock(hglbCopy), wc, size*sizeof(WCHAR));
		free(wc);
		GlobalUnlock(hglbCopy);
		SetClipboardData(CF_UNICODETEXT, hglbCopy);
	}
	else
	#endif
#endif
	{
		hglbCopy = GlobalAlloc(GMEM_MOVEABLE, strlen(str)+1); 
		memcpy(GlobalLock(hglbCopy), str, strlen(str)+1);
		GlobalUnlock(hglbCopy);
		SetClipboardData(CF_TEXT, hglbCopy);
	}
	CloseClipboard();
	delete [] str;
}

void MarkerList::ExportToFile(const char* format)
{
	// Note - UTF8 untested
	char cFilename[512];
	if (BrowseForSaveFile(__LOCALIZE("Choose text file to save markers to","sws_DLG_102"), NULL, NULL, "TXT files\0*.txt\0", cFilename, 512))
	{
		char* str = GetFormattedList(format);
		FILE* f = fopen(cFilename, "w");
		if (f)
		{
			fputs(str, f);
			fclose(f);
		}
		delete [] str;
	}
}

int MarkerList::ApproxSize()
{
	int size = 64; // Start off with some padding
	for (int i = 0; i < m_items.GetSize(); i++)
	{
		size += 64;
		size += (int)strlen(m_items.Get(i)->GetName());
	}
	return size;
}

void MarkerList::CropToTimeSel(bool bOffset)
{
	double dStart, dEnd;
	GetSet_LoopTimeRange(false, false, &dStart, &dEnd, false);
	// If no time sel just return
	if (dStart == dEnd)
		return;

	// Don't crop the end of regions
	for (int i = 0; i < m_items.GetSize(); i++)
	{
		MarkerItem* item = m_items.Get(i);
		if (item->GetPos() > dEnd ||
			(!item->IsRegion() && item->GetPos() < dStart) ||
			(item->IsRegion()  && item->GetRegEnd() < dStart))
		{ // delete the item
			m_items.Delete(i, true);
			i--;
		}
		else
		{
			if (item->IsRegion() && item->GetPos() < dStart && item->GetRegEnd() >= dStart)
				item->SetPos(dStart);
			
			if (bOffset)
			{
				item->SetPos(item->GetPos() - dStart);
				if (item->IsRegion())
					item->SetRegEnd(item->GetRegEnd() - dStart);
			}
		}
	}
}

// Helper func
int EnumMarkers(int idx, bool* isrgn, double* pos, double* rgnend, const char** name, int* markrgnindexnumber, int* color)
{
	return EnumProjectMarkers3(NULL, idx, isrgn, pos, rgnend, name, markrgnindexnumber, color);
}
