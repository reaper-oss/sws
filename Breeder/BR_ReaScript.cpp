/*****************************************************************************
/ BR_ReaScript.cpp
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
#include "stdafx.h"
#include "BR_ReaScript.h"
#include "BR_Util.h"

/******************************************************************************
* Globals                                                                     *
******************************************************************************/
static BR_MouseContextInfo g_mouseInfo;

/******************************************************************************
* ReaScript export                                                            *
******************************************************************************/
bool BR_GetMediaSourceProperties (MediaItem_Take* take, bool* section, double* start, double* length, double* fade, bool* reverse)
{
	return GetMediaSourceProperties(take, section, start, length, fade, reverse);
}

void BR_GetMouseCursorContext (char* window, char* segment, char* details, int char_sz)
{
	const char* _window;
	const char* _segment;
	const char* _details;
	GetMouseCursorContext(&_window, &_segment, &_details, &g_mouseInfo);

	strncpy(window,_window, char_sz-1);
	strncpy(segment,_segment, char_sz-1);
	strncpy(details,_details, char_sz-1);
}

TrackEnvelope* BR_GetMouseCursorContext_Envelope (bool* takeEnvelope)
{
	WritePtr(takeEnvelope, g_mouseInfo.takeEnvelope);
	return g_mouseInfo.envelope;
}

MediaItem* BR_GetMouseCursorContext_Item ()
{
	return g_mouseInfo.item;
}

double BR_GetMouseCursorContext_Position ()
{
	return g_mouseInfo.position;
}

MediaItem_Take* BR_GetMouseCursorContext_Take ()
{
	return g_mouseInfo.take;
}

MediaTrack* BR_GetMouseCursorContext_Track ()
{
	return g_mouseInfo.track;
}

MediaItem* BR_ItemAtMouseCursor (double* position)
{
	return ItemAtMouseCursor(position);
}

double BR_PositionAtMouseCursor (bool checkRuler)
{
	return PositionAtMouseCursor(checkRuler);
}

bool BR_SetMediaSourceProperties (MediaItem_Take* take, bool section, double start, double length, double fade, bool reverse)
{
	return SetMediaSourceProperties(take, section, start, length, fade, reverse);
}

bool BR_SetTakeSourceFromFile (MediaItem_Take* take, const char* filename, bool inProjectData)
{
	return SetTakeSourceFromFile(take, filename, inProjectData, false);
}

bool BR_SetTakeSourceFromFile2 (MediaItem_Take* take, const char* filename, bool inProjectData, bool keepSourceProperties)
{
	return SetTakeSourceFromFile(take, filename, inProjectData, keepSourceProperties);
}

MediaItem_Take* BR_TakeAtMouseCursor (double* position)
{
	return TakeAtMouseCursor(position);
}

MediaTrack* BR_TrackAtMouseCursor (int* context, double* position)
{
	return TrackAtMouseCursor(context, position);
}