/******************************************************************************
/ BR_ReaScript.h
/
/ Copyright (c) 2014-2015 Dominik Martin Drzic
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
void            BR_EnvGetProperties (BR_Envelope* envelope, bool* activeOut, bool* visibleOut, bool* armedOut, bool* inLaneOut, int* laneHeightOut, int* defaultShapeOut, double* minValueOut, double* maxValueOut, double* centerValueOut, int* typeOut, bool* faderScalingOut);
bool            BR_EnvSetPoint (BR_Envelope* envelope, int id, double position, double value, int shape, bool selected, double bezier);
void            BR_EnvSetProperties (BR_Envelope* envelope, bool active, bool visible, bool armed, bool inLane, int laneHeight, int defaultShape, bool faderScaling);
void            BR_EnvSortPoints (BR_Envelope* envelope);
double          BR_EnvValueAtPos (BR_Envelope* envelope, double position);
double          BR_GetMidiSourceLenPPQ (MediaItem_Take* take);
MediaItem*      BR_GetMediaItemByGUID (ReaProject* proj, const char* guidStringIn);
void            BR_GetMediaItemGUID (MediaItem* item, char* guidStringOut, int guidStringOut_sz);
void            BR_GetMediaItemTakeGUID (MediaItem_Take* take, char* guidStringOut, int guidStringOut_sz);
bool            BR_GetMediaSourceProperties (MediaItem_Take* take, bool* sectionOut, double* startOut, double* lengthOut, double* fadeOut, bool* reverseOut);
MediaTrack*     BR_GetMediaTrackByGUID (ReaProject* proj, const char* guidStringIn);
void            BR_GetMediaTrackGUID (MediaTrack* track, char* bufOut, int buf_sz);
void            BR_GetMouseCursorContext (char* windowOut, int windowOut_sz, char* segmentOut, int segmentOut_sz, char* detailsOut, int detailsOut_sz);
TrackEnvelope*  BR_GetMouseCursorContext_Envelope (bool* takeEnvelopeOut);
MediaItem*      BR_GetMouseCursorContext_Item ();
void*           BR_GetMouseCursorContext_MIDI (bool* inlineEditorOut, int* noteRowOut, int* ccLaneOut, int* ccLaneValOut, int* ccLaneIdOut);
double          BR_GetMouseCursorContext_Position ();
int             BR_GetMouseCursorContext_StretchMarker ();
MediaItem_Take* BR_GetMouseCursorContext_Take ();
MediaTrack*     BR_GetMouseCursorContext_Track ();
MediaItem*      BR_ItemAtMouseCursor (double* positionOut);
bool            BR_IsTakeMidi (MediaItem_Take* take, bool* inProjectMidiOut);
bool            BR_MIDI_CCLaneRemove (HWND midiEditor, int laneId);
bool            BR_MIDI_CCLaneReplace (HWND midiEditor, int laneId, int newCC);
double          BR_PositionAtMouseCursor (bool checkRuler);
bool            BR_SetMediaSourceProperties (MediaItem_Take* take, bool section, double start, double length, double fade, bool reverse);
bool            BR_SetTakeSourceFromFile (MediaItem_Take* take, const char* filenameIn, bool inProjectData);
bool            BR_SetTakeSourceFromFile2 (MediaItem_Take* take, const char* filenameIn, bool inProjectData, bool keepSourceProperties);
MediaItem_Take* BR_TakeAtMouseCursor (double* positionOut);
MediaTrack*     BR_TrackAtMouseCursor (int* contextOut, double* positionOut);

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
