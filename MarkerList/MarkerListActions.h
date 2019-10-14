/******************************************************************************
/ MarkerListActions.h
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


#pragma once

void ListToClipboard(COMMAND_T* = NULL);
void ListToClipboardTimeSel(COMMAND_T* = NULL);
void ClipboardToList(COMMAND_T* = NULL);
void ExportToClipboard(COMMAND_T* = NULL);
void ExportToFile(COMMAND_T* = NULL);
void DeleteAllMarkers();
void DeleteAllMarkers(COMMAND_T*);
void DeleteAllRegions();
void DeleteAllRegions(COMMAND_T*);
void RenumberIds(COMMAND_T* = NULL);
void RenumberRegions(COMMAND_T*);
void SelNextRegion(COMMAND_T*);
void SelPrevRegion(COMMAND_T*);
void GotoEndInclMarkers(COMMAND_T*);
void SelNextMarkerOrRegion(COMMAND_T*);
void SelPrevMarkerOrRegion(COMMAND_T*);
void MarkersToRegions(COMMAND_T*);
void RegionsToMarkers(COMMAND_T*);
