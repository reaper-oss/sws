/******************************************************************************
/ BR_Util.h
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
#pragma once

#define SKIP(x,y) {x+=y; continue;}

bool IsThisFraction (char* str, double &convertedFraction);
int GetFirstDigit (int val);
int ClearBit (int val, int pos);
int SetBit (int val, int pos);
double AltAtof (char* str);
double EndOfProject (bool markers, bool regions);
void CenterDialog (HWND hwnd, HWND target, HWND zOrder);
void GetSelItemsInTrack (MediaTrack* track, vector<MediaItem*> &items);
void ReplaceAll (string& str, string oldStr, string newStr);
void CommandTimer (COMMAND_T* ct);
bool BR_SetTakeSourceFromFile(MediaItem_Take* take, char* filename, bool inProjectData);