/******************************************************************************
/ BR_Util.h
/
/ Copyright (c) 2013-2014 Dominik Martin Drzic
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

/******************************************************************************
* Constants                                                                   *
******************************************************************************/
const int VERT_SCROLL_W = 17;

/******************************************************************************
* Macros                                                                      *
******************************************************************************/
#define SKIP(counter, count)    {(counter)+=(count); continue;}

/******************************************************************************
* Miscellaneous                                                               *
******************************************************************************/
bool IsThisFraction (char* str, double& convertedFraction);
double AltAtof (char* str);
void ReplaceAll (string& str, string oldStr, string newStr);
int GetBit (int val, int pos);
int SetBit (int val, int pos);
int ToggleBit (int val, int pos);
int ClearBit (int val, int pos);
int GetFirstDigit (int val);
int GetLastDigit (int val);
vector<int> GetDigits(int val);
template <typename T> void WritePtr (T* ptr, T val) {if (ptr) *ptr = val;};
template <typename T> void ReadPtr  (T* ptr, T& val) {if (ptr) val = *ptr;};
template <typename T> T CheckBounds (T val, T min, T max) {if (val < min) return min; if (val > max) return max; return val;};

/******************************************************************************
* General                                                                     *
******************************************************************************/
vector<MediaItem*> GetSelItems (MediaTrack* track);
vector<double> GetProjectMarkers (bool timeSel);
double EndOfProject (bool markers, bool regions);
double GetProjectSettingsTempo (int* num, int* den);
bool TcpVis (MediaTrack* track);
template <typename T> void GetConfig (const char* key, T& val) { val = *static_cast<T*>(GetConfigVar(key)); };
template <typename T> void SetConfig (const char* key, T  val) { *static_cast<T*>(GetConfigVar(key)) = val; };

/******************************************************************************
* Height                                                                      *
******************************************************************************/
int GetTrackHeight (MediaTrack* track, int* offsetY);
int GetItemHeight (MediaItem* item, int* offsetY);
int GetTakeHeight (MediaItem_Take* take, int* offsetY);
int GetTakeHeight (MediaItem* item, int id, int* offsetY);
int GetTakeEnvHeight (MediaItem_Take* take, int* offsetY);
int GetTakeEnvHeight (MediaItem* item, int id, int* offsetY);
int GetTrackEnvHeight (TrackEnvelope* envelope, int* offsetY, MediaTrack* parent = NULL);

/******************************************************************************
* Arrange                                                                     *
******************************************************************************/
void MoveArrange (double amountTime);
void CenterArrange (double position);
void MoveArrangeToTarget (double target, double reference);
bool IsOffScreen (double position);
bool PositionAtMouseCursor (double* position);
MediaItem* ItemAtMouseCursor (double* position);

/******************************************************************************
* Window                                                                      *
******************************************************************************/
HWND GetTcpWnd ();
HWND GetTcpTrackWnd (MediaTrack* track);
HWND GetArrangeWnd ();
void CenterDialog (HWND hwnd, HWND target, HWND zOrder);

/******************************************************************************
* Theming                                                                     *
******************************************************************************/
void ThemeListViewOnInit (HWND list);
bool ThemeListViewInProc (HWND hwnd, int uMsg, LPARAM lParam, HWND list, bool grid);

/******************************************************************************
* ReaScript                                                                   *
******************************************************************************/
bool BR_SetTakeSourceFromFile (MediaItem_Take* take, const char* filename, bool inProjectData);