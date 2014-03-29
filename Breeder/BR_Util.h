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
const int VERT_SCROLL_W   = 17;
const int NEGATIVE_INF    = -150;
const double VOLUME_DELTA = 0.0000000000001;
const double PAN_DELTA    = 0.001;

/******************************************************************************
* Macros                                                                      *
******************************************************************************/
#define SKIP(counter, count)    {(counter)+=(count); continue;}

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
int SetBit (int val, int pos);
int ToggleBit (int val, int pos);
int ClearBit (int val, int pos);
int GetFirstDigit (int val);
int GetLastDigit (int val);
vector<int> GetDigits(int val);
WDL_FastString GetSourceChunk(PCM_source* source);
template <typename T> void WritePtr (T* ptr, T val)  {if (ptr)*ptr = val;}
template <typename T> void ReadPtr  (T* ptr, T& val) {if (ptr) val = *ptr;}
template <typename T> bool CheckBounds (T val, T min, T max) {if (val < min) return false; if (val > max) return false; return true;}
template <typename T> T    SetToBounds (T val, T min, T max) {if (val < min) return min; if (val > max) return max; return val;}

/******************************************************************************
* General                                                                     *
******************************************************************************/
vector<MediaItem*> GetSelItems (MediaTrack* track);
vector<double> GetProjectMarkers (bool timeSel, double delta = 0);
int GetTakeChannelCount (MediaItem_Take* take);
double GetClosestGrid (double position);
double GetClosestMeasureGrid (double position);
double GetClosestLeftSideGrid (double position);
double GetClosestRightSideGrid (double position);
double EndOfProject (bool markers, bool regions);
double GetProjectSettingsTempo (int* num, int* den);
void InitTempoMap ();
void ScrollToTrackIfNotInArrange (MediaTrack* track);
bool TcpVis (MediaTrack* track);
bool IsMidi (MediaItem_Take* take);
bool GetMediaSourceProperties (MediaItem_Take* take, bool* section, double* start, double* length, double* fade, bool* reverse);
bool SetMediaSourceProperties (MediaItem_Take* take, bool section, double start, double length, double fade, bool reverse);
bool SetTakeSourceFromFile (MediaItem_Take* take, const char* filename, bool inProjectData, bool keepSourceProperties);
template <typename T> void GetConfig (const char* key, T& val) {val = *static_cast<T*>(GetConfigVar(key));}
template <typename T> void SetConfig (const char* key, T  val) {*static_cast<T*>(GetConfigVar(key)) = val;}

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
void SetArrangeStart (double start);
void MoveArrangeToTarget (double target, double reference);
bool IsOffScreen (double position);

/******************************************************************************
* Mouse cursor                                                                *
******************************************************************************/
struct BR_MouseContextInfo
{
	MediaTrack* track; MediaItem* item; MediaItem_Take* take; TrackEnvelope* envelope; bool takeEnvelope; double position;
	BR_MouseContextInfo () {track = NULL; item = NULL; take = NULL; envelope = NULL; takeEnvelope = false; position = -1;}
};
void GetMouseCursorContext (const char** window, const char** segment, const char** details, BR_MouseContextInfo* info);
double PositionAtMouseCursor (bool checkRuler);
MediaItem* ItemAtMouseCursor (double* position);
MediaItem_Take* TakeAtMouseCursor (double* position);
MediaTrack* TrackAtMouseCursor (int* context, double* position); // context: 0->TCP, 1->MCP, 2->Arrange

/******************************************************************************
* Window                                                                      *
******************************************************************************/
HWND FindInFloatingDockers (const char* name);
HWND FindInReaperDockers (const char* name);
HWND FindInReaper (const char* name);
HWND FindFloating (const char* name);
HWND GetMixerWnd ();
HWND GetMixerMasterWnd ();
HWND GetArrangeWnd ();
HWND GetTransportWnd ();
HWND GetRulerWndAlt ();
HWND GetMcpWnd ();
HWND GetTcpWnd ();
HWND GetTcpTrackWnd (MediaTrack* track);
MediaTrack* HwndToTrack (HWND hwnd, int* hwndContext);  // context: 0->unknown, 1->tcp, 2->mcp (works even if hwnd is not a track but something else in mcp/tcp)
TrackEnvelope* HwndToEnvelope (HWND hwnd);
void CenterDialog (HWND hwnd, HWND target, HWND zOrder);

/******************************************************************************
* Theming                                                                     *
******************************************************************************/
void ThemeListViewOnInit (HWND list);
bool ThemeListViewInProc (HWND hwnd, int uMsg, LPARAM lParam, HWND list, bool grid);

/******************************************************************************
* MIDI (saved/inserted by using time position, not ppq)                       *
******************************************************************************/
struct BR_NoteEvent
{
	BR_NoteEvent (MediaItem_Take* take, int id);
	void InsertEvent (MediaItem_Take* take);
	bool selected, muted;
	double timePos, timeEnd;
	int chan, pitch, vel;
};

struct BR_CCEvent
{
	BR_CCEvent (MediaItem_Take* take, int id);
	void InsertEvent (MediaItem_Take* take);
	bool selected, muted;
	double timePos;
	int chanmsg, chan, msg2, msg3;
};

struct BR_SysEvent
{
	BR_SysEvent (MediaItem_Take* take, int id);
	void InsertEvent (MediaItem_Take* take);
	bool selected, muted;
	double timePos;
	int type;
	WDL_FastString msg;
};

struct BR_MidiTake
{
	BR_MidiTake (MediaItem_Take* take, int noteCount = 0, int ccCount = 0, int sysCount = 0);
	vector<BR_NoteEvent> noteEvents;
	vector<BR_CCEvent> ccEvents;
	vector<BR_SysEvent> sysEvents;
	MediaItem_Take* take;
};