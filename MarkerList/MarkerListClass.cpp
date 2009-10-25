/******************************************************************************
/ MarkerListClass.cpp
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
#include "../Utility/SectionLock.h"
#include "MarkerListClass.h"
#include "MarkerListActions.h"

MarkerItem::MarkerItem(bool bReg, double dPos, double dRegEnd, const char* cName, int id)
{
	m_bReg = bReg;
	m_dPos = dPos;
	m_dRegEnd = bReg ? dRegEnd : -1.0;
	m_id = id;
	SetName(cName);
}

MarkerItem::MarkerItem(LineParser* lp)
{
	m_id      = atol(lp->gettoken_str(0));
	m_dPos    = atof(lp->gettoken_str(1));
	SetName(lp->gettoken_str(2));
	m_bReg    = atol(lp->gettoken_str(3)) ? true : false;
	m_dRegEnd = atof(lp->gettoken_str(4));
}

MarkerItem::~MarkerItem()
{
	delete [] m_cName;
}

char* MarkerItem::ItemString(char* str, int iSize)
{
	_snprintf(str, iSize, "%d %.14f \"%s\" %d %.14f", m_id, m_dPos, GetName(), m_bReg ? 1 : 0, m_dRegEnd);
	return str;
}
void MarkerItem::SetName(const char* newname)
{
	if (newname && strlen(newname))
	{
		m_cName = new char[strlen(newname)+1];
		strcpy(m_cName, newname);
	}
	else
		m_cName = NULL;
}

bool MarkerItem::Compare(bool bReg, double dPos, double dRegEnd, const char* cName, int id)
{
	return (bReg == m_bReg && dPos == m_dPos && strcmp(cName, GetName()) == 0 && id == m_id && (!bReg || dRegEnd == m_dRegEnd));
}

bool MarkerItem::Compare(MarkerItem* mi)
{
	return Compare(mi->m_bReg, mi->m_dPos, mi->m_dRegEnd, mi->GetName(), mi->m_id);
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
	m_items = NULL;
	m_hLock = NULL;

	if (bGetCurList)
		BuildFromReaper();
}

MarkerList::~MarkerList()
{
	SectionLock lock(m_hLock);
	m_items.Empty(true);
	delete[] m_name;
}

// return true if something has changed!
bool MarkerList::BuildFromReaper()
{
	SectionLock lock(m_hLock);

	// Instead of emptying and starting over, try to just add or delete for simple cases (added/deleted marker)
	// markers moved in time are reallocated
	// m_items maintains the same ordering as EnumProjectMarkers
	int id, x = 0, i = 0;
	bool bR, bChanged = false;
	double dPos, dRend;
	char *cName;

	while ((x=EnumProjectMarkers(x, &bR, &dPos, &dRend, &cName, &id)))
	{
		if (i >= m_items.GetSize() || !m_items.Get(i)->Compare(bR, dPos, dRend, cName ? cName : "", id))
		{ // not found, try ahead one more
			if (i+1 >= m_items.GetSize() || !m_items.Get(i+1)->Compare(bR, dPos, dRend, cName, id))
			{	// not found one ahead, assume new and insert at current position
				m_items.Insert(i, new MarkerItem(bR, dPos, dRend, cName, id));
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

	SectionLock lock(m_hLock);

	for (int i = 0; i < m_items.GetSize(); i++)
	{
		MarkerItem* mi = m_items.Get(i);
		AddProjectMarker(NULL, mi->m_bReg, mi->m_dPos, mi->m_dRegEnd, mi->GetName(), mi->m_id);
	}
}

void MarkerList::ListToClipboard()
{
	SectionLock lock(m_hLock);
	if (OpenClipboard(g_hwndParent))
	{
		int iSize = ApproxSize()*2;
		char* str = new char[iSize];
		str[0] = 0;
		for (int i = 0; i < m_items.GetSize(); i++)
		{
			m_items.Get(i)->ItemString(str+strlen(str), iSize);
			strcpy(str+strlen(str), "\r\n");
			iSize -= (int)strlen(str);
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
	SectionLock lock(m_hLock);
	//if (IsClipboardFormatAvailable(CF_TEXT) && OpenClipboard(g_hwndParent))
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
			if (!lp.parse(line) && lp.getnumtokens() == 5)
				m_items.Add(new MarkerItem(&lp));
			line = strtok(NULL, "\r\n");
		}
		delete [] data;

		if (m_items.GetSize())
			UpdateReaper();
	}
}

void MarkerList::ExportToClipboard(const char* format)
{
	SectionLock lock(m_hLock);
	char* str = new char[ApproxSize()*2];

	// Get end of project
	double dSavedCur = GetCursorPosition();
	CSurf_GoEnd();
	double dEnd = GetCursorPosition();
	SetEditCurPos(dSavedCur, true, true);

	char* s = str;
	int count = 1;

	for (int i = 0; i < m_items.GetSize(); i++)
	{
		if (format[0] == 'a' ||
			(format[0] == 'r' && m_items.Get(i)->m_bReg) ||
			(format[0] == 'm' && !m_items.Get(i)->m_bReg))
		{
			// Cheat a bit and use the region end variable for markers as "location of next marker or eop"
			if (!m_items.Get(i)->m_bReg)
			{
				if (i < m_items.GetSize() - 1)
					m_items.Get(i)->m_dRegEnd = m_items.Get(i+1)->m_dPos;
				else
					m_items.Get(i)->m_dRegEnd = dEnd;
			}

			for (unsigned int j = 1; j < strlen(format); j++)
			{
				switch(format[j])
				{
				case 'n':
					s += sprintf(s, "%d", count++);
					break;
				case 'i':
					s += sprintf(s, "%d", m_items.Get(i)->m_id);
					break;
				case 'l':
				{
					double len = m_items.Get(i)->m_dRegEnd-m_items.Get(i)->m_dPos;
					if (len < 0.0)
						len = 0.0;
					format_timestr_pos(len, s, (int)(4096-(s-str)), 5);
					s += strlen(s)-3;
					s[0] = 0;
					break;
				}
				case 'd':
					s += sprintf(s, "%s", m_items.Get(i)->GetName());
					break;
				case 't':
					format_timestr_pos(m_items.Get(i)->m_dPos, s, (int)(4096-(s-str)), 5);
					s += strlen(s)-3;
					s[0] = 0;
					break;
				case 's':
					format_timestr_pos(m_items.Get(i)->m_dPos, s, (int)(4096-(s-str)), 4);
					s += strlen(s);
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

	if (!strlen(str) || !OpenClipboard(g_hwndParent))
	{
		delete [] str;
		return;
	}

    EmptyClipboard();

	// Not sure what the HGLOBAL deal is but it's straight from the help on SetClipboardData
	HGLOBAL hglbCopy; 
    hglbCopy = GlobalAlloc(GMEM_MOVEABLE, strlen(str)+1); 
    memcpy(GlobalLock(hglbCopy), str, strlen(str)+1);
    GlobalUnlock(hglbCopy); 
    SetClipboardData(CF_TEXT, hglbCopy); 
	CloseClipboard();
	delete [] str;
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
		if (m_items.Get(i)->m_dPos > dEnd ||
			(!m_items.Get(i)->m_bReg && m_items.Get(i)->m_dPos < dStart) ||
			(m_items.Get(i)->m_bReg  && m_items.Get(i)->m_dRegEnd < dStart))
		{ // delete the item
			m_items.Delete(i, true);
			i--;
		}
		else
		{
			if (m_items.Get(i)->m_bReg && m_items.Get(i)->m_dPos < dStart && m_items.Get(i)->m_dRegEnd >= dStart)
				m_items.Get(i)->m_dPos = dStart;
			
			if (bOffset)
			{
				m_items.Get(i)->m_dPos -= dStart;
				if (m_items.Get(i)->m_bReg)
					m_items.Get(i)->m_dRegEnd -= dStart;
			}
		}
	}
}
