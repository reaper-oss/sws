/******************************************************************************
/ cfillion.hpp
/
/ Copyright (c) 2017 Christian Fillion
/ https://cfillion.ca
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

typedef HWND__ FxChain;

void CF_SetClipboard(const char *);
void CF_GetClipboard(char *buf, int bufSize);
const char *CF_GetClipboardBig(WDL_FastString *);

bool CF_ShellExecute(const char *file, const char *args = NULL);
bool CF_LocateInExplorer(const char *file);

void CF_GetSWSVersion(char *buf, int bufSize);
int CF_GetCustomColor(int index);
void CF_SetCustomColor(int index, int color);

// deprecated, exposed natively in REAPER v6.71
int CF_EnumerateActions(int section, int idx, char *nameBuf, int nameBufSize);
const char *CF_GetCommandText(int section, int command);

HWND CF_GetFocusedFXChain();
HWND CF_GetTrackFXChain(MediaTrack *);
HWND CF_GetTrackFXChainEx(ReaProject *, MediaTrack *, bool inputChain);
HWND CF_GetTakeFXChain(MediaItem_Take *);
int CF_EnumSelectedFX(HWND chain, int index = -1);
bool CF_SelectTrackFX(MediaTrack *, int index);

int CF_GetMediaSourceBitDepth(PCM_source *);
double CF_GetMediaSourceBitRate(PCM_source *);
bool CF_GetMediaSourceOnline(PCM_source *);
void CF_SetMediaSourceOnline(PCM_source *, bool set);
bool CF_GetMediaSourceMetadata(PCM_source *, const char *name, char *buf, int bufSize);
bool CF_GetMediaSourceRPP(PCM_source *source, char *buf, const int bufSize);
int CF_EnumMediaSourceCues(PCM_source *source, const int index, double *time,
  double *endTime, bool *isRegion, char *name, const int nameSize, bool *isChapter);
bool CF_ExportMediaSource(PCM_source *source, const char *file);
bool CF_PCM_Source_SetSectionInfo(PCM_source *section, PCM_source *source,
  double offset, double length, bool reverse);

BOOL CF_GetScrollInfo(HWND, int bar, LPSCROLLINFO);

class CF_Preview;
CF_Preview *CF_CreatePreview(PCM_source *);
bool CF_Preview_GetValue(CF_Preview *, const char *name, double *valueOut);
bool CF_Preview_GetPeak(CF_Preview *, int channel, double *peakvolOut);
bool CF_Preview_SetValue(CF_Preview *, const char *name, double newValue);
bool CF_Preview_SetOutputTrack(CF_Preview *, ReaProject *, MediaTrack *);
bool CF_Preview_Play(CF_Preview *);
bool CF_Preview_Stop(CF_Preview *);
void CF_Preview_StopAll();
