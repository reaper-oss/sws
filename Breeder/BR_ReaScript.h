/******************************************************************************
/ BR_ReaScript.h
/
/ Copyright (c) 2014-2015 Dominik Martin Drzic
/ http://forum.cockos.com/member.php?u=27094
/ http://github.com/reaper-oss/sws
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

class BR_Envelope;

/******************************************************************************
* ReaScript export                                                            *
******************************************************************************/
BR_Envelope*    BR_EnvAlloc (TrackEnvelope* envelope, bool takeEnvelopesUseProjectTime);
int             BR_EnvCountPoints (BR_Envelope* envelope);
bool            BR_EnvDeletePoint (BR_Envelope* envelope, int id);
int             BR_EnvFind (BR_Envelope* envelope, double position, double delta);
int             BR_EnvFindNext (BR_Envelope* envelope, double position);
int             BR_EnvFindPrevious (BR_Envelope* envelope, double position);
bool            BR_EnvFree (BR_Envelope* envelope, bool commit);
MediaItem_Take* BR_EnvGetParentTake (BR_Envelope* envelope);
MediaTrack*     BR_EnvGetParentTrack (BR_Envelope* envelope);
bool            BR_EnvGetPoint (BR_Envelope* envelope, int id, double* positionOut, double* valueOut, int* shapeOut, bool* selectedOut, double* bezierOut);
void            BR_EnvGetProperties (BR_Envelope* envelope, bool* activeOut, bool* visibleOut, bool* armedOut, bool* inLaneOut, int* laneHeightOut, int* defaultShapeOut, double* minValueOut, double* maxValueOut, double* centerValueOut, int* typeOut, bool* faderScalingOut, int* AIoptionsOut);
bool            BR_EnvSetPoint (BR_Envelope* envelope, int id, double position, double value, int shape, bool selected, double bezier);
void            BR_EnvSetProperties (BR_Envelope* envelope, bool active, bool visible, bool armed, bool inLane, int laneHeight, int defaultShape, bool faderScaling, int* AIoptions);
void            BR_EnvSortPoints (BR_Envelope* envelope);
double          BR_EnvValueAtPos (BR_Envelope* envelope, double position);
void            BR_GetArrangeView (ReaProject* proj, double* startPositionOut, double* endPositionOut);
double          BR_GetClosestGridDivision (double position);
void            BR_GetCurrentTheme (char* themePathOut, int themePathOut_sz, char* themeNameOut, int themeNameOut_sz);
MediaItem*      BR_GetMediaItemByGUID (ReaProject* proj, const char* guidStringIn);
void            BR_GetMediaItemGUID (MediaItem* item, char* guidStringOut, int guidStringOut_sz);
bool            BR_GetMediaItemImageResource (MediaItem* item, char* imageOut, int imageOut_sz, int* imageFlagsOut);
void            BR_GetMediaItemTakeGUID (MediaItem_Take* take, char* guidStringOut, int guidStringOut_sz);
bool            BR_GetMediaSourceProperties (MediaItem_Take* take, bool* sectionOut, double* startOut, double* lengthOut, double* fadeOut, bool* reverseOut);
MediaTrack*     BR_GetMediaTrackByGUID (ReaProject* proj, const char* guidStringIn);
int             BR_GetMediaTrackFreezeCount (MediaTrack* track);
void            BR_GetMediaTrackGUID (MediaTrack* track, char* guidStringOut, int guidStringOut_sz);
void            BR_GetMediaTrackLayouts (MediaTrack* track, char* mcpLayoutNameOut, int mcpLayoutNameOut_sz, char* tcpLayoutNameOut, int tcpLayoutNameOut_sz);
TrackEnvelope*  BR_GetMediaTrackSendInfo_Envelope (MediaTrack* track, int category, int sendidx, int envelopeType);
MediaTrack*     BR_GetMediaTrackSendInfo_Track (MediaTrack* track, int category, int sendidx, int trackType);
double          BR_GetMidiSourceLenPPQ (MediaItem_Take* take);
bool            BR_GetMidiTakePoolGUID (MediaItem_Take* take, char* guidStringOut, int guidStringOut_sz);
bool            BR_GetMidiTakeTempoInfo (MediaItem_Take* take, bool* ignoreProjTempoOut, double* bpmOut, int* numOut, int* denOut);
void            BR_GetMouseCursorContext (char* windowOut, int windowOut_sz, char* segmentOut, int segmentOut_sz, char* detailsOut, int detailsOut_sz);
TrackEnvelope*  BR_GetMouseCursorContext_Envelope (bool* takeEnvelopeOut);
MediaItem*      BR_GetMouseCursorContext_Item ();
void*           BR_GetMouseCursorContext_MIDI (bool* inlineEditorOut, int* noteRowOut, int* ccLaneOut, int* ccLaneValOut, int* ccLaneIdOut);
double          BR_GetMouseCursorContext_Position ();
int             BR_GetMouseCursorContext_StretchMarker ();
MediaItem_Take* BR_GetMouseCursorContext_Take ();
MediaTrack*     BR_GetMouseCursorContext_Track ();
double          BR_GetNextGridDivision (double position);
double          BR_GetPrevGridDivision (double position);
double          BR_GetSetTrackSendInfo (MediaTrack* track, int category, int sendidx, const char* parmname, bool setNewValue, double newValue);
int             BR_GetTakeFXCount (MediaItem_Take* take);
bool            BR_IsTakeMidi (MediaItem_Take* take, bool* inProjectMidiOut);
bool			BR_IsMidiOpenInInlineEditor(MediaItem_Take* take);
MediaItem*      BR_ItemAtMouseCursor (double* positionOut);
bool            BR_MIDI_CCLaneRemove (void* midiEditor, int laneId);
bool            BR_MIDI_CCLaneReplace (void* midiEditor, int laneId, int newCC);
double          BR_PositionAtMouseCursor (bool checkRuler);
void            BR_SetArrangeView (ReaProject* proj, double startPosition, double endPosition);
bool            BR_SetItemEdges (MediaItem* item, double startTime, double endTime);
void            BR_SetMediaItemImageResource (MediaItem* item, const char* imageIn, int imageFlags);
bool            BR_SetMediaSourceProperties (MediaItem_Take* take, bool section, double start, double length, double fade, bool reverse);
bool            BR_SetMediaTrackLayouts (MediaTrack* track, const char* mcpLayoutNameIn, const char* tcpLayoutNameIn);
bool            BR_SetMidiTakeTempoInfo (MediaItem_Take* take, bool ignoreProjTempo, double bpm, int num, int den);
bool            BR_SetTakeSourceFromFile (MediaItem_Take* take, const char* filenameIn, bool inProjectData);
bool            BR_SetTakeSourceFromFile2 (MediaItem_Take* take, const char* filenameIn, bool inProjectData, bool keepSourceProperties);
MediaItem_Take* BR_TakeAtMouseCursor (double* positionOut);
MediaTrack*     BR_TrackAtMouseCursor (int* contextOut, double* positionOut);
bool            BR_TrackFX_GetFXModuleName (MediaTrack* track, int fx, char* nameOut, int nameOutSz);
int             BR_Win32_CB_FindString (void* comboBoxHwnd, int startId, const char* string);
int             BR_Win32_CB_FindStringExact (void* comboBoxHwnd, int startId, const char* string);
void            BR_Win32_ClientToScreen (void* hwnd, int xIn, int yIn, int* xOut, int* yOut);
void*           BR_Win32_FindWindowEx (const char* hwndParent, const char* hwndChildAfter, const char* className, const char* windowName, bool searchClass, bool searchName);
int             BR_Win32_GET_X_LPARAM (int lParam);
int             BR_Win32_GET_Y_LPARAM (int lParam);
int             BR_Win32_GetConstant (const char* constantName);
bool            BR_Win32_GetCursorPos (int* xOut, int* yOut);
void*           BR_Win32_GetFocus ();
void*           BR_Win32_GetForegroundWindow ();
void*           BR_Win32_GetMainHwnd();
void*           BR_Win32_GetMixerHwnd (bool* isDockedOut);
void            BR_Win32_GetMonitorRectFromRect (bool workingAreaOnly, int leftIn, int topIn, int rightIn, int bottomIn, int* leftOut, int* topOut, int* rightOut, int* bottomOut);
void*           BR_Win32_GetParent (void* hwnd);
int             BR_Win32_GetPrivateProfileString (const char* sectionName, const char* keyName, const char* defaultString, const char* filePath, char* stringOut, int stringOut_sz);
void*           BR_Win32_GetWindow (void* hwnd, int cmd);
int             BR_Win32_GetWindowLong (void* hwnd, int index);
bool            BR_Win32_GetWindowRect(void* hwnd, int* leftOut, int* topOut, int* rightOut, int* bottomOut);
int             BR_Win32_GetWindowText (void* hwnd, char* textOut, int textOut_sz);
int             BR_Win32_HIBYTE (int value);
int             BR_Win32_HIWORD (int value);
void            BR_Win32_HwndToString (void* hwnd, char* stringOut, int stringOut_sz);
bool            BR_Win32_IsWindow (void* hwnd);
bool            BR_Win32_IsWindowVisible (void* hwnd);
int             BR_Win32_LOBYTE (int value);
int             BR_Win32_LOWORD (int value);
int             BR_Win32_MAKELONG (int low, int high);
int             BR_Win32_MAKELPARAM (int low, int high);
int             BR_Win32_MAKELRESULT (int low, int high);
int             BR_Win32_MAKEWORD (int low, int high);
int             BR_Win32_MAKEWPARAM (int low, int high);
void*           BR_Win32_MIDIEditor_GetActive ();
void            BR_Win32_ScreenToClient (void* hwnd, int xIn, int yIn, int* xOut, int* yOut);
int             BR_Win32_SendMessage (void* hwnd, int msg, int lParam, int wParam);
void*           BR_Win32_SetFocus (void* hwnd);
int             BR_Win32_SetForegroundWindow (void* hwnd);
int             BR_Win32_SetWindowLong (void* hwnd, int index, int newLong);
bool            BR_Win32_SetWindowPos (void* hwnd, const char* hwndInsertAfter, int x, int y, int width, int height, int flags);
int             BR_Win32_ShellExecute (const char* operation, const char* file, const char* parameters, const char* directoy, int showFlags);
bool            BR_Win32_ShowWindow (void* hwnd, int cmdShow);
void*           BR_Win32_StringToHwnd (const char* string);
void*           BR_Win32_WindowFromPoint (int x, int y);
bool            BR_Win32_WritePrivateProfileString (const char* sectionName, const char* keyName, const char* value, const char* filePath);


/******************************************************************************
* Big description!                                                            *
******************************************************************************/
#define BR_MOUSE_REASCRIPT_DESC "[BR] \
Get mouse cursor context. Each parameter returns information in a form of string as specified in the table below.\n\n\
To get more info on stuff that was found under mouse cursor see BR_GetMouseCursorContext_Envelope, BR_GetMouseCursorContext_Item, \
BR_GetMouseCursorContext_MIDI, BR_GetMouseCursorContext_Position, BR_GetMouseCursorContext_Take, BR_GetMouseCursorContext_Track \n\n\
<table border=\"2\">\
<tr><th style=\"width:100px\">Window</th> <th style=\"width:100px\">Segment</th> <th style=\"width:300px\">Details</th>                                           </tr>\
<tr><th rowspan=\"1\" align = \"center\"> unknown     </th>    <td> \"\"        </td>   <td> \"\"                                                           </td> </tr>\
<tr><th rowspan=\"4\" align = \"center\"> ruler       </th>    <td> region_lane </td>   <td> \"\"                                                           </td> </tr>\
<tr>                                                           <td> marker_lane </td>   <td> \"\"                                                           </td> </tr>\
<tr>                                                           <td> tempo_lane  </td>   <td> \"\"                                                           </td> </tr>\
<tr>                                                           <td> timeline    </td>   <td> \"\"                                                           </td> </tr>\
<tr><th rowspan=\"1\" align = \"center\"> transport   </th>    <td> \"\"        </td>   <td> \"\"                                                           </td> </tr>\
<tr><th rowspan=\"3\" align = \"center\"> tcp         </th>    <td> track       </td>   <td> \"\"                                                           </td> </tr>\
<tr>                                                           <td> envelope    </td>   <td> \"\"                                                           </td> </tr>\
<tr>                                                           <td> empty       </td>   <td> \"\"                                                           </td> </tr>\
<tr><th rowspan=\"2\" align = \"center\"> mcp         </th>    <td> track       </td>   <td> \"\"                                                           </td> </tr>\
<tr>                                                           <td> empty       </td>   <td> \"\"                                                           </td> </tr>\
<tr><th rowspan=\"3\" align = \"center\"> arrange     </th>    <td> track       </td>   <td> empty,<br>item, item_stretch_marker,<br>env_point, env_segment </td> </tr>\
<tr>                                                           <td> envelope    </td>   <td> empty, env_point, env_segment                                  </td> </tr>\
<tr>                                                           <td> empty       </td>   <td> \"\"                                                           </td> </tr>\
<tr><th rowspan=\"5\" align = \"center\"> midi_editor </th>    <td> unknown     </td>   <td> \"\"                                                           </td> </tr>\
<tr>                                                           <td> ruler       </td>   <td> \"\"                                                           </td> </tr>\
<tr>                                                           <td> piano       </td>   <td> \"\"                                                           </td> </tr>\
<tr>                                                           <td> notes       </td>   <td> \"\"                                                           </td> </tr>\
<tr>                                                           <td> cc_lane     </td>   <td> cc_selector, cc_lane                                           </td> </tr>\
</table>"
