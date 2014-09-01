/******************************************************************************
/ wol_Util.h
/
/ Copyright (c) 2014 wol
/ http://forum.cockos.com/member.php?u=70153
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



void wol_UtilInit();

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Track
///////////////////////////////////////////////////////////////////////////////////////////////////
bool SaveSelectedTracks();
bool RestoreSelectedTracks();

/* refreshes UI too */
void SetTrackHeight(MediaTrack* track, int height);

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Tcp
///////////////////////////////////////////////////////////////////////////////////////////////////
int GetTcpEnvMinHeight();
int GetTcpTrackMinHeight();
int GetCurrentTcpMaxHeight();
void ScrollToTrackEnvelopeIfNotInArrange(TrackEnvelope* envelope);
void ScrollToTrackIfNotInArrange(TrackEnvelope* envelope);

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Envelope
///////////////////////////////////////////////////////////////////////////////////////////////////
struct EnvelopeHeight
{
	TrackEnvelope* env;
	int h;
};
int CountVisibleTrackEnvelopesInTrackLane(MediaTrack* track);

/*  Return   -1 for envelope not in track lane or not visible, *laneCount and *envCount not modified
			 0 for overlap disabled, *laneCount = *envCount = number of lanes (same as visible envelopes)
			 1 for overlap enabled, envelope height < overlap limit -> single lane (one envelope visible), *laneCount = *envCount = 1
			 2 for overlap enabled, envelope height < overlap limit -> single lane (overlapping), *laneCount = 1, *envCount = number of visible envelopes
			 3 for overlap enabled, envelope height > overlap limit -> single lane (one envelope visible), *laneCount = *envCount = 1
			 4 for overlap enabled, envelope height > overlap limit -> multiple lanes, *laneCount = *envCount = number of lanes (same as visible envelopes) */
int GetEnvelopeOverlapState(TrackEnvelope* envelope, int* laneCount = NULL, int* envCount = NULL);

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Midi
///////////////////////////////////////////////////////////////////////////////////////////////////
int AdjustRelative(int _adjmode, int _reladj);

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Ini
///////////////////////////////////////////////////////////////////////////////////////////////////
const char* GetWolIni();
void SaveIniSettings(const char* section, const char* key, bool val, const char* path = GetWolIni());
void SaveIniSettings(const char* section, const char* key, int val, const char* path = GetWolIni());
void SaveIniSettings(const char* section, const char* key, const char* val, const char* path = GetWolIni());
bool GetIniSettings(const char* section, const char* key, bool defVal, const char* path = GetWolIni());
int GetIniSettings(const char* section, const char* key, int defVal, const char* path = GetWolIni());
int GetIniSettings(const char* section, const char* key, const char* defVal, char* outBuf, int maxOutBufSize, const char* path = GetWolIni());
void DeleteIniSection(const char* section, const char* path = GetWolIni());
void DeleteIniKey(const char* section, const char* key, const char* path = GetWolIni());
#ifdef _WIN32
void FlushIni(const char* path = GetWolIni());
#endif