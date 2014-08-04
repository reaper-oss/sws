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
bool IsFraction (char* str, double& convertedFraction);
double AltAtof (char* str);
double RoundToN (double val, double n);
double TranslateRange (double value, double oldMin, double oldMax, double newMin, double newMax);
void ReplaceAll (string& str, string oldStr, string newStr);
void AppendLine (WDL_FastString& str, const char* line);
int Round (double val);
int GetBit (int val, int pos);
int SetBit (int val, int pos, bool set);
int SetBit (int val, int pos);
int ClearBit (int val, int pos);
int ToggleBit (int val, int pos);
int GetFirstDigit (int val);
int GetLastDigit (int val);
int BinaryToDecimal (const char* binaryString);
vector<int> GetDigits (int val); // in 567 [0] = 5, [1] = 6 etc...
WDL_FastString GetSourceChunk (PCM_source* source);
template <typename T> bool WritePtr (T* ptr, T val)  {if (ptr){*ptr = val; return true;} return false;}
template <typename T> bool ReadPtr  (T* ptr, T& val) {if (ptr){val = *ptr; return true;} return false;}
template <typename T> bool CheckBounds   (T val, T min, T max) {if (val < min)  return false; if (val > max)  return false; return true;}
template <typename T> bool CheckBoundsEx (T val, T min, T max) {if (val <= min) return false; if (val >= max) return false; return true;}
template <typename T> T    SetToBounds   (T val, T min, T max) {if (val < min)  return min;   if (val > max)  return max;   return val;}
template <typename T> T    IsEqual (T a, T b, T epsilon) {epsilon = abs(epsilon); return CheckBounds(a, b - epsilon, b + epsilon);}

/******************************************************************************
* General                                                                     *
******************************************************************************/
vector<MediaItem*> GetSelItems (MediaTrack* track);
vector<double> GetProjectMarkers (bool timeSel, double delta = 0);
WDL_FastString FormatTime (double position, int mode = -1);  // same as format_timestr_pos but handles "measures.beats + time" properly
int GetTakeId (MediaItem_Take* take, MediaItem* item = NULL);
double EndOfProject (bool markers, bool regions);
double GetProjectSettingsTempo (int* num, int* den);
double GetSourceLengthPPQ (MediaItem_Take* take);
double GetGridDivSafe (); // makes sure grid div is never over MAX_GRID_DIV
void InitTempoMap ();
void ScrollToTrackIfNotInArrange (MediaTrack* track);
void StartPlayback (double position);
void GetSetLastAdjustedSend (bool set, MediaTrack** track, int* sendId, int* type); // for type see BR_EnvType (works only for volume and pan, not mute)
bool SetIgnoreTempo (MediaItem* item, bool ignoreTempo, double bpm, int num, int den);
bool DoesItemHaveMidiEvents (MediaItem* item);
bool TrimItem (MediaItem* item, double start, double end);
bool IsPlaying ();
bool IsPaused ();
bool IsRecording ();
bool TcpVis (MediaTrack* track);
bool GetMediaSourceProperties (MediaItem_Take* take, bool* section, double* start, double* length, double* fade, bool* reverse);
bool SetMediaSourceProperties (MediaItem_Take* take, bool section, double start, double length, double fade, bool reverse);
bool SetTakeSourceFromFile (MediaItem_Take* take, const char* filename, bool inProjectData, bool keepSourceProperties);
template <typename T> void GetConfig (const char* key, T& val) {val = *static_cast<T*>(GetConfigVar(key));}
template <typename T> void SetConfig (const char* key, T  val) {*static_cast<T*>(GetConfigVar(key)) = val;}

/******************************************************************************
* Grid                                                                        *
******************************************************************************/
double GetNextGridDiv (double position);           // unlike other functions, this one doesn't care about grid visibility
double GetClosestGridLine (double position);
double GetClosestMeasureGridLine (double position);
double GetClosestLeftSideGridLine (double position);
double GetClosestRightSideGridLine (double position);

/******************************************************************************
* Height                                                                      *
******************************************************************************/
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
MediaTrack* HwndToTrack (HWND hwnd, int* hwndContext);  // context: 0->unknown, 1->tcp, 2->mcp (works even if hwnd is not a track but something else in mcp/tcp)
TrackEnvelope* HwndToEnvelope (HWND hwnd);
void CenterDialog (HWND hwnd, HWND target, HWND zOrder);

/******************************************************************************
* Mouse cursor                                                                *
******************************************************************************/
struct BR_MouseContextInfo
{
											  // In case the thing is invalid:
	MediaTrack* track;                        // NULL
	MediaItem* item;                          // NULL
	MediaItem_Take* take;                     // NULL
	TrackEnvelope* envelope;                  // NULL
	void* midiEditor;                         // NULL
	bool takeEnvelope, midiInlineEditor;      // false
	double position;                          // -1
	int noteRow, ccLaneVal, ccLaneId;         // -1
	int ccLane;                               // -2
	BR_MouseContextInfo();
};

void GetMouseCursorContext (const char** window, const char** segment, const char** details, BR_MouseContextInfo* info);
double PositionAtMouseCursor (bool checkRuler, bool checkCursorVisibility = true, int* yOffset = NULL, bool* overRuler = NULL);
MediaItem* ItemAtMouseCursor (double* position);
MediaItem_Take* TakeAtMouseCursor (double* position);
MediaTrack* TrackAtMouseCursor (int* context, double* position); // context: 0->TCP, 1->MCP, 2->Arrange

/******************************************************************************
* Theming                                                                     *
******************************************************************************/
void DrawTooltip (LICE_IBitmap* bm, const char* text);
void ThemeListViewOnInit (HWND list);
bool ThemeListViewInProc (HWND hwnd, int uMsg, LPARAM lParam, HWND list, bool grid);