/******************************************************************************
/ Menus.h
/
/ Copyright (c) 2011 Tim Payne (SWS)
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

void AddToMenuOrdered(HMENU hMenu, const char* text, int id, int iInsertAfter = -1, bool bPos = false, UINT uiSate = MFS_UNCHECKED);
void AddToMenu(HMENU hMenu, const char* text, int id, int iInsertAfter = -1, bool bPos = false, UINT uiSate = MFS_UNCHECKED);
void AddSubMenu(HMENU hMenu, HMENU subMenu, const char* text, int iInsertAfter = -1, UINT uiSate = MFS_UNCHECKED);
int FindSortedPos(HMENU hMenu, const char* text);
HMENU FindMenuItem(HMENU hMenu, int iCmd, int* iPos);
void SWSSetMenuText(HMENU hMenu, int iCmd, const char* cText);
int SWSGetMenuPosFromID(HMENU hMenu, UINT id);

void SWSCreateExtensionsMenu(HMENU hMenu);
