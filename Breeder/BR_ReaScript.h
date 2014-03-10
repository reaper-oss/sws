/******************************************************************************
/ BR_ReaScript.h
/
/ Copyright (c) 2014 Dominik Martin Drzic
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
* ReaScript export                                                            *
******************************************************************************/
bool            BR_GetMediaSourceProperties (MediaItem_Take* take, bool* section, double* start, double* length, double* fade, bool* reverse);
void            BR_GetMouseCursorContext (char* window, char* segment, char* details, int char_sz);
TrackEnvelope*  BR_GetMouseCursorContext_Envelope (bool* takeEnvelope);
MediaItem*      BR_GetMouseCursorContext_Item ();
double          BR_GetMouseCursorContext_Position ();
MediaItem_Take* BR_GetMouseCursorContext_Take ();
MediaTrack*     BR_GetMouseCursorContext_Track ();
MediaItem*      BR_ItemAtMouseCursor (double* position);
double          BR_PositionAtMouseCursor (bool checkRuler);
bool            BR_SetMediaSourceProperties (MediaItem_Take* take, bool section, double start, double length, double fade, bool reverse);
bool            BR_SetTakeSourceFromFile (MediaItem_Take* take, const char* filename, bool inProjectData);
MediaItem_Take* BR_TakeAtMouseCursor (double* position);
MediaTrack*     BR_TrackAtMouseCursor (int* context, double* position);

/******************************************************************************
* Big description!                                                            *
******************************************************************************/
#define BR_MOUSE_REASCRIPT_DESC "[BR] \
Get mouse cursor context. Each parameter returns information in a form of string as specified in the table below.\n\n               \
To get more info on stuff that was found under mouse cursor see BR_GetMouseCursorContext_Envelope,                                  \
BR_GetMouseCursorContext_Item, BR_GetMouseCursorContext_Position, BR_GetMouseCursorContext_Take, BR_GetMouseCursorContext_Track \n  \
Note that these functions will return every element found under mouse cursor. For example, if details is \"env_segment\",           \
BR_GetMouseCursorContext_Item will still return valid item if it was under that envelope segment.\n\n                               \
<table border=\"2\">                                                                                                                \
<tr><th style=\"width:100px\">Window</th> <th style=\"width:100px\">Segment</th> <th style=\"width:300px\">Details</th>      </tr>  \
<tr><td rowspan=\"1\" align = \"center\"> unknown   </th> <td> \"\"     </td> <td> \"\"                                </td> </tr>  \
<tr><td rowspan=\"4\" align = \"center\"> ruler     </th> <td> regions  </td> <td> \"\"                                </td> </tr>  \
<tr>                                                      <td> markers  </td> <td> \"\"                                </td> </tr>  \
<tr>                                                      <td> tempo    </td> <td> \"\"                                </td> </tr>  \
<tr>                                                      <td> timeline </td> <td> \"\"                                </td> </tr>  \
<tr><td rowspan=\"1\" align = \"center\"> transport </th> <td> \"\"     </td> <td> \"\"                                </td> </tr>  \
<tr><td rowspan=\"3\" align = \"center\"> tcp       </th> <td> track    </td> <td> \"\"                                </td> </tr>  \
<tr>                                                      <td> envelope </td> <td> \"\"                                </td> </tr>  \
<tr>                                                      <td> empty    </td> <td> \"\"                                </td> </tr>  \
<tr><td rowspan=\"2\" align = \"center\"> mcp       </th> <td> track    </td> <td> \"\"                                </td> </tr>  \
<tr>                                                      <td> empty    </td> <td> \"\"                                </td> </tr>  \
<tr><td rowspan=\"3\" align = \"center\"> arrange   </th> <td> track    </td> <td> item, env_point, env_segment, empty </td> </tr>  \
<tr>                                                      <td> envelope </td> <td> env_point, env_segment, empty       </td> </tr>  \
<tr>                                                      <td> empty    </td> <td> \"\"                                </td> </tr>  \
</table>"