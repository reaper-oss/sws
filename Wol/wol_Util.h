/******************************************************************************
/ wol_Util.h
/
/ Copyright (c) 2014 and later wol
/ http://forum.cockos.com/member.php?u=70153
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

#include "../Breeder/BR_EnvelopeUtil.h"



///////////////////////////////////////////////////////////////////////////////////////////////////
/// Track
///////////////////////////////////////////////////////////////////////////////////////////////////
bool SaveSelectedTracks();
bool RestoreSelectedTracks();

/* refreshes UI too */
void SetTrackHeight(MediaTrack* track, int height, bool useChunk = false);

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Arrange/TCP
///////////////////////////////////////////////////////////////////////////////////////////////////
enum VerticalZoomCenter
{
	CLIP_IN_ARRANGE = 0,		// Do not change order, add at the end
	MID_ARRANGE,
	MOUSE_CURSOR,
	UPPER_HALF,
	LOWER_HALF
};

int GetTcpEnvMinHeight();
int GetTcpTrackMinHeight();
int GetCurrentTcpMaxHeight();
void SetArrangeScroll(int offsetY, int height, VerticalZoomCenter center = CLIP_IN_ARRANGE);
void SetArrangeScrollTo(MediaTrack* track, VerticalZoomCenter center = CLIP_IN_ARRANGE);
inline void SetArrangeScrollTo(MediaItem* item, VerticalZoomCenter center = CLIP_IN_ARRANGE);
inline void SetArrangeScrollTo(MediaItem_Take* take, VerticalZoomCenter center = CLIP_IN_ARRANGE);
void SetArrangeScrollTo(TrackEnvelope* envelope, bool check = true, VerticalZoomCenter center = CLIP_IN_ARRANGE);
void SetArrangeScrollTo(BR_Envelope* envelope, VerticalZoomCenter center = CLIP_IN_ARRANGE);

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Envelope
///////////////////////////////////////////////////////////////////////////////////////////////////
int CountVisibleTrackEnvelopesInTrackLane(MediaTrack* track);

/*  Return   -1 for envelope not in track lane or not visible, *laneCount and *envCount not modified
			 0 for overlap disabled, *laneCount = *envCount = number of lanes (same as visible envelopes)
			 1 for overlap enabled, envelope height < overlap limit -> single lane (one envelope visible), *laneCount = *envCount = 1
			 2 for overlap enabled, envelope height < overlap limit -> single lane (overlapping), *laneCount = 1, *envCount = number of visible envelopes
			 3 for overlap enabled, envelope height > overlap limit -> single lane (one envelope visible), *laneCount = *envCount = 1
			 4 for overlap enabled, envelope height > overlap limit -> multiple lanes, *laneCount = *envCount = number of lanes (same as visible envelopes) */
int GetEnvelopeOverlapState(TrackEnvelope* envelope, int* laneCount = NULL, int* envCount = NULL);

void PutSelectedEnvelopeInLane(COMMAND_T* ct);
bool IsEnvelopeInMediaLane(TrackEnvelope* env = NULL);

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Midi
///////////////////////////////////////////////////////////////////////////////////////////////////
int AdjustRelative(int _adjmode, int _reladj);
