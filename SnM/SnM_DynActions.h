/******************************************************************************
 / SnM_DynActions.h
 /
 / Copyright (c) 2016 and later Jeffos
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

//#pragma once

#ifndef _SNM_DYNACTIONS_H_
#define _SNM_DYNACTIONS_H_

class DynActionListView : public SWS_ListView
{
public:
	DynActionListView(HWND hwndList, HWND hwndEdit);
  
protected:
	void SetItemText(SWS_ListItem* item, int iCol, const char* str);
	void GetItemText(SWS_ListItem* item, int iCol, char* str, int iStrMax);
	void GetItemList(SWS_ListItemList* pList);
};

void ShowDynActionWnd(COMMAND_T*);

#endif
