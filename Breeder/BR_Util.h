/******************************************************************************
/ BR_Util.h
/
/ Copyright (c) 2013-2014 Dominik Martin Drzic
/ http://forum.cockos.com/member.php?u=27094
/ https://code.google.com/p/sws-extension
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
const int SCROLLBAR_W     = 17;
const int NEGATIVE_INF    = -150;
const double VOLUME_DELTA = 0.0000000000001;
const double PAN_DELTA    = 0.001;

/******************************************************************************
* Macros                                                                      *
******************************************************************************/
#define SKIP(counter, count)   {(counter)+=(count); continue;}
#define BINARY(x)              BinaryToDecimal(#x)

/******************************************************************************
* Miscellaneous                                                               *
******************************************************************************/
vector<int> GetDigits (int val); // in 567 [0] = 5, [1] = 6 etc...
set<int> GetAllMenuIds (HMENU hMenu);
int GetUnusedMenuId (HMENU hMenu);
int RoundToInt (double val);
int TruncToInt (double val);
int GetBit (int val, int pos);
int SetBit (int val, int pos, bool set);
int SetBit (int val, int pos);
int ClearBit (int val, int pos);
int ToggleBit (int val, int pos);
int GetFirstDigit (int val);
int GetLastDigit (int val);
int BinaryToDecimal (const char* binaryString);
bool IsFraction (char* str, double& convertedFraction);
double AltAtof (char* str);
double Trunc (double val);
double Round (double val);
double RoundToN (double val, double n);
double TranslateRange (double value, double oldMin, double oldMax, double newMin, double newMax);
void ReplaceAll (string& str, string oldStr, string newStr);
void AppendLine (WDL_FastString& str, const char* line);
template <typename T> bool WritePtr (T* ptr, T val)  {if (ptr){*ptr = val; return true;} return false;}
template <typename T> bool ReadPtr  (T* ptr, T& val) {if (ptr){val = *ptr; return true;} return false;}
template <typename T> bool AreOverlapped   (T x1, T x2, T y1, T y2) {if (x1 > x2) swap(x1, x2); if (y1 > y2) swap(y1, y2); return x1 <= y2 && y1 <= x2;}
template <typename T> bool AreOverlappedEx (T x1, T x2, T y1, T y2) {if (x1 > x2) swap(x1, x2); if (y1 > y2) swap(y1, y2); return x1 <  y2 && y1 <  x2;}
template <typename T> bool CheckBounds   (T val, T min, T max)    {if (min > max) swap(min, max); if (val < min)  return false; if (val > max)  return false; return true;}
template <typename T> bool CheckBoundsEx (T val, T min, T max)    {if (min > max) swap(min, max); if (val <= min) return false; if (val >= max) return false; return true;}
template <typename T> T    SetToBounds   (T val, T min, T max)    {if (min > max) swap(min, max); if (val < min)  return min;   if (val > max)  return max;   return val;}
template <typename T> T    IsEqual (T a, T b, T epsilon) {epsilon = abs(epsilon); return CheckBounds(a, b - epsilon, b + epsilon);}

/******************************************************************************
* General                                                                     *
******************************************************************************/
vector<double> GetProjectMarkers (bool timeSel, double delta = 0);
WDL_FastString GetReaperMenuIniPath ();
WDL_FastString FormatTime (double position, int mode = -1);                                      // same as format_timestr_pos but handles "measures.beats + time" properly
int GetEffectiveTakeId (MediaItem_Take* take, MediaItem* item, int id, int* effectiveTakeCount); // empty takes could be hidden, so displayed id and real id can differ (pass either take or item and take id)
int GetEffectiveCompactLevel (MediaTrack* track);                                                // displayed compact level is determined by the highest compact level of any successive parent
int FindClosestProjMarkerIndex (double position);
double EndOfProject (bool markers, bool regions);
double GetProjectSettingsTempo (int* num, int* den);
double GetGridDivSafe (); // makes sure grid div is never over MAX_GRID_DIV
void InitTempoMap ();
void ScrollToTrackIfNotInArrange (MediaTrack* track);
void StartPlayback (double position);
void GetSetLastAdjustedSend (bool set, MediaTrack** track, int* sendId, int* type); // for type see BR_EnvType (works only for volume and pan, not mute)
void GetSetFocus (bool set, HWND* hwnd, int* context);
void RefreshToolbarAlt (int cmd); // works with MIDI toolbars unlike RefreshToolbarAlt()
bool IsPlaying ();
bool IsPaused ();
bool IsRecording ();
bool TcpVis (MediaTrack* track);
template <typename T> void GetConfig (const char* key, T& val) {val = *static_cast<T*>(GetConfigVar(key));}
template <typename T> void SetConfig (const char* key, T  val) {*static_cast<T*>(GetConfigVar(key)) = val;}

/******************************************************************************
* Items                                                                       *
******************************************************************************/
vector<MediaItem*> GetSelItems (MediaTrack* track);
double GetSourceLengthPPQ (MediaItem_Take* take);
double ProjectTimeToItemTime (MediaItem* item, double projTime);
double ItemTimeToProjectTime (MediaItem* item, double itemTime);
int GetTakeId (MediaItem_Take* take, MediaItem* item = NULL);
int GetTakeType (MediaItem_Take* take); // -1 = unknown, 0 = audio, 1 = MIDI, 2 = video, 3 = click, 4 = timecode generator, 5 = RPR project
bool SetIgnoreTempo (MediaItem* item, bool ignoreTempo, double bpm, int num, int den);
bool DoesItemHaveMidiEvents (MediaItem* item);
bool TrimItem (MediaItem* item, double start, double end);
bool GetMediaSourceProperties (MediaItem_Take* take, bool* section, double* start, double* length, double* fade, bool* reverse);
bool SetMediaSourceProperties (MediaItem_Take* take, bool section, double start, double length, double fade, bool reverse);
bool SetTakeSourceFromFile (MediaItem_Take* take, const char* filename, bool inProjectData, bool keepSourceProperties);
WDL_FastString GetSourceChunk (PCM_source* source);

/******************************************************************************
* Fades                                                                       *
******************************************************************************/
double GetNormalizedFadeValue (double position, double fadeStart, double fadeEnd, int fadeShape, double fadeCurve, bool isFadeOut);
double GetEffectiveFadeLength (MediaItem* item, bool isFadeOut, int* fadeShape = NULL, double* fadeCurve = NULL);

/******************************************************************************
* Stretch markers                                                             *
******************************************************************************/
int FindPreviousStretchMarker (MediaItem_Take* take, double proj);
int FindNextStretchMarker (MediaItem_Take* take, double position);
int FindClosestStretchMarker (MediaItem_Take* take, double position);
int FindStretchMarker (MediaItem_Take* take, double position, double surroundingRange = 0);

/******************************************************************************
* Grid                                                                        *
******************************************************************************/
double GetNextGridDiv (double position); // unlike other functions, this one doesn't care about grid visibility
double GetClosestGridLine (double position);
double GetClosestMeasureGridLine (double position);
double GetClosestLeftSideGridLine (double position);
double GetClosestRightSideGridLine (double position);

/******************************************************************************
* Locking                                                                     *
******************************************************************************/
enum BR_LockElements
{
	TIME_SEL          = 1,
	ITEM_FULL         = 2,
	ITEM_HORZ_MOVE    = 64,
	ITEM_VERT_MOVE    = 128,
	ITEM_EDGES        = 256,
	FADES_VOL_HANDLE  = 512,
	STRETCH_MARKERS   = 4096,
	TAKE_ENV          = 2048,
	TRACK_ENV         = 4,
	REGIONS           = 16,
	MARKERS           = 8,
	TEMPO_MARKERS     = 32,
	LOOP_POINTS       = 1024
};
bool IsLockingActive ();
bool IsLocked (int lockElements);
bool IsItemLocked (MediaItem* item);

/******************************************************************************
* Height                                                                      *
******************************************************************************/
int GetTrackHeightFromVZoomIndex (MediaTrack* track, int vZoom);     // also takes track compacting into account
int GetEnvHeightFromTrackHeight (int trackHeight);                   // what is envelope height in case it's height override is 0 ?
int GetTrackHeight (MediaTrack* track, int* offsetY, int* topGap = NULL, int* bottomGap = NULL);
int GetItemHeight (MediaItem* item, int* offsetY);
int GetTakeHeight (MediaItem_Take* take, int* offsetY);
int GetTakeHeight (MediaItem* item, int id, int* offsetY);
int GetTakeEnvHeight (MediaItem_Take* take, int* offsetY);
int GetTakeEnvHeight (MediaItem* item, int id, int* offsetY);
int GetTrackEnvHeight (TrackEnvelope* envelope, int* offsetY, bool drawableRangeOnly, MediaTrack* parent = NULL);

/******************************************************************************
* Arrange                                                                     *
******************************************************************************/
void MoveArrange (double amountTime);
void CenterArrange (double position);
void SetArrangeStart (double start);
void MoveArrangeToTarget (double target, double reference);
bool IsOffScreen (double position);

/******************************************************************************
* Window                                                                      *
******************************************************************************/
HWND FindReaperWndByTitle (const char* name);
HWND GetArrangeWnd ();
HWND GetRulerWndAlt ();
HWND GetTransportWnd ();
HWND GetMixerWnd ();
HWND GetMixerMasterWnd ();
HWND GetMediaExplorerWnd ();
HWND GetMcpWnd ();
HWND GetTcpWnd ();
HWND GetTcpTrackWnd (MediaTrack* track);
HWND GetNotesView (void* midiEditor);
HWND GetPianoView (void* midiEditor);
MediaTrack* HwndToTrack (HWND hwnd, int* hwndContext);  // context: 0->unknown, 1->TCP, 2->MCP (works even if hwnd is not a track but something else in mcp/tcp)
TrackEnvelope* HwndToEnvelope (HWND hwnd);
void CenterDialog (HWND hwnd, HWND target, HWND zOrder);
void GetMonitorRectFromPoint (const POINT& p, RECT* r);
void BoundToRect (RECT& boundingRect, RECT* r);

/******************************************************************************
* Theming                                                                     *
******************************************************************************/
void DrawTooltip (LICE_IBitmap* bm, const char* text);
void SetWndIcon (HWND hwnd); // win32 only
void ThemeListViewOnInit (HWND list);
bool ThemeListViewInProc (HWND hwnd, int uMsg, LPARAM lParam, HWND list, bool grid);

/******************************************************************************
* Height helpers (exposed here for usage outside of BR_Util.h)                *
******************************************************************************/
void GetTrackGap (int trackHeight, int* top, int* bottom);
bool IsLastTakeTooTall (int itemHeight, int averageTakeHeight, int effectiveTakeCount, int* lastTakeHeight);
int GetMinTakeHeight (MediaTrack* track, int takeCount, int trackHeight, int itemHeight);
int GetItemHeight (MediaItem* item, int* offsetY, int trackHeight, int trackOffsetY);
int GetTakeHeight (MediaItem_Take* take, MediaItem* item, int id, int* offsetY, bool averagedLast);                                   // pass either take or
int GetTakeHeight (MediaItem_Take* take, MediaItem* item, int id, int* offsetY, bool averagedLast, int trackHeight, int trackOffset); // item and take id