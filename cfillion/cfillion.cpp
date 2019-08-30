/******************************************************************************
/ cfillion.cpp
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

#include "stdafx.h"
#include "cfillion.hpp"

#include "reaper/localize.h"
#include "version.h"

#ifdef _WIN32
static const unsigned int FORMAT = CF_UNICODETEXT;
#else
  // on SWELL/generic CF_TEXT may be implemented as a function call which
  // may not be available until after loading is complete
#define FORMAT (CF_TEXT)
#endif

extern WDL_PtrList_DOD<WDL_FastString> g_script_strs;

void CF_SetClipboard(const char *buf)
{
#ifdef _WIN32
  const int length = MultiByteToWideChar(CP_UTF8, 0, buf, -1, nullptr, 0);
  const size_t size = length * sizeof(wchar_t);
#else
  const size_t size = strlen(buf) + 1;
#endif

  HANDLE mem = GlobalAlloc(GMEM_MOVEABLE, size);
#ifdef _WIN32
  MultiByteToWideChar(CP_UTF8, 0, buf, -1, (wchar_t *)GlobalLock(mem), length);
#else
  memcpy(GlobalLock(mem), buf, size);
#endif
  GlobalUnlock(mem);

  OpenClipboard(GetMainHwnd());
  EmptyClipboard();
  SetClipboardData(FORMAT, mem);
  CloseClipboard();
}

void CF_GetClipboard(char *buf, int bufSize)
{
  OpenClipboard(GetMainHwnd());
  HANDLE mem = GetClipboardData(FORMAT);

  if(void *data = GlobalLock(mem)) {
#ifdef _WIN32
    WideCharToMultiByte(CP_UTF8, 0, (const wchar_t *)data, -1,
      buf, bufSize - 1, nullptr, nullptr);

    // Insert a null terminator if the buffer is too small to hold the entire
    // clipboard data. WideCharToMultiByte inserts it at the end of the string
    // by itself when the buffer is big enough.
    buf[bufSize - 1] = 0;
#else
    snprintf(buf, bufSize, "%s", (const char *)data);
#endif
    GlobalUnlock(mem);
  }

  CloseClipboard();
}

const char *CF_GetClipboardBig(WDL_FastString *output)
{
  if(g_script_strs.Find(output) == -1)
    return NULL;

  OpenClipboard(GetMainHwnd());
  HANDLE mem = GetClipboardData(FORMAT);

  if(void *data = GlobalLock(mem)) {
#ifdef _WIN32
    const int size = WideCharToMultiByte(CP_UTF8, 0,
      (const wchar_t *)data, -1, nullptr, 0, nullptr, nullptr) - 1;

    output->SetLen(size);

    WideCharToMultiByte(CP_UTF8, 0, (const wchar_t *)data, -1,
      const_cast<char *>(output->Get()), size, nullptr, nullptr);
#else
    output->Set((const char *)data);
#endif
    GlobalUnlock(mem);
  }

  CloseClipboard();

  return output->Get();
}

bool CF_ShellExecute(const char *file, const char *args)
{
  // Windows's implementation of ShellExecute returns a fake HINSTANCE (greater
  // than 32 on success) while SWELL's implementation returns a BOOL.

#ifdef _WIN32
  static_assert(&ShellExecute == &ShellExecuteUTF8,
    "ShellExecute is not aliased to ShellExecuteUTF8");
  HINSTANCE ret = ShellExecute(nullptr, "open", file, args, nullptr, SW_SHOW);
  return ret > (HINSTANCE)32;
#else
  return ShellExecute(NULL, "open", file, args, NULL, SW_SHOW);
#endif
}

bool CF_LocateInExplorer(const char *file)
{
  // Quotes inside the filename must not be escaped for the SWELL implementation
  WDL_FastString arg;
  arg.SetFormatted(strlen(file) + 10, "/select,\"%s\"", file);

  return CF_ShellExecute("explorer.exe", arg.Get());
}

void CF_GetSWSVersion(char *buf, const int bufSize)
{
  snprintf(buf, bufSize, "%d.%d.%d.%d", SWS_VERSION);
}

int CF_EnumerateActions(const int section, const int idx, char *nameBuf, const int nameBufSize)
{
  const char *name = "";
  const int cmdId = kbd_enumerateActions(SectionFromUniqueID(section), idx, &name);
  snprintf(nameBuf, nameBufSize, "%s", name);
  return cmdId;
}

const char *CF_GetCommandText(const int section, const int command)
{
  return kbd_getTextFromCmd(command, SectionFromUniqueID(section));
}

static HWND CF_GetTrackFXChain(const int trackIndex)
{
  char chainTitle[128];

  if(trackIndex < 1)
    snprintf(chainTitle, sizeof(chainTitle), "%s%s",
      __LOCALIZE("FX: ", "fx"), __LOCALIZE("Master Track", "fx"));
  else
    snprintf(chainTitle, sizeof(chainTitle), "%s%s %d",
      __LOCALIZE("FX: ", "fx"), __LOCALIZE("Track", "fx"), trackIndex);

  return FindWindowEx(NULL, NULL, NULL, chainTitle);
}

HWND CF_GetTrackFXChain(MediaTrack *track)
{
  int trackNumber = 0;

  if(track != GetMasterTrack(NULL))
    trackNumber = static_cast<int>(GetMediaTrackInfo_Value(track, "IP_TRACKNUMBER"));

  return CF_GetTrackFXChain(trackNumber);
}

HWND CF_GetTakeFXChain(MediaItem_Take *take)
{
  // Shameful hack follows. The FX chain window does have some user data
  // attached to it (pointer to an internal FxChain object?) but it does not
  // seem to hint back to the take in any obvious way.

  GUID *guid = static_cast<GUID *>(GetSetMediaItemTakeInfo(take, "GUID", NULL));

  char guidStr[64];
  guidToString(guid, guidStr);

  string originalName = GetTakeName(take);
  GetSetMediaItemTakeInfo_String(take, "P_NAME", guidStr, true);

  char chainTitle[128];
  snprintf(chainTitle, sizeof(chainTitle), "%s%s \"%s\"",
    __LOCALIZE("FX: ", "fx"), __LOCALIZE("Item", "fx"), guidStr);

  HWND window = FindWindowEx(NULL, NULL, NULL, chainTitle);

  GetSetMediaItemTakeInfo_String(take, "P_NAME",
    const_cast<char *>(originalName.c_str()), true);

  return window;
}

HWND CF_GetFocusedFXChain()
{
  // Original idea by amagalma
  // https://forum.cockos.com/showthread.php?t=207220

  int trackIndex, itemIndex, fxIndex;
  switch(GetFocusedFX(&trackIndex, &itemIndex, &fxIndex)) {
  case 1:
    return CF_GetTrackFXChain(trackIndex);
  case 2: {
    MediaTrack *track = GetTrack(NULL, trackIndex - 1);
    MediaItem *item = GetTrackMediaItem(track, itemIndex);
    MediaItem_Take *take = GetMediaItemTake(item, HIWORD(fxIndex));
    return CF_GetTakeFXChain(take);
  }
  default:
    return NULL;
  }
}

int CF_EnumSelectedFX(HWND fxChain, const int index)
{
  const HWND list = GetDlgItem(fxChain, 1076);
  return ListView_GetNextItem(list, index, LVNI_SELECTED);
}

int CF_GetMediaSourceBitDepth(PCM_source *source)
{
  // parameter validation is already done on the REAPER side, so we're sure source is a valid PCM_source
  return source->GetBitsPerSample();
}

bool CF_GetMediaSourceOnline(PCM_source *source)
{
  return source->IsAvailable();
}

void CF_SetMediaSourceOnline(PCM_source *source, const bool online)
{
  source->SetAvailable(online);
}

bool CF_GetMediaSourceMetadata(PCM_source *source, const char *name, char *buf, const int bufSize)
{
  return source->Extended(PCM_SOURCE_EXT_GETMETADATA, (void *)name, (void *)buf, (void *)(intptr_t)bufSize) > 0;
}

bool CF_GetMediaSourceRPP(PCM_source *source, char *buf, const int bufSize)
{
  char *rpp = NULL;
  source->Extended(PCM_SOURCE_EXT_GETASSOCIATED_RPP, &rpp, NULL, NULL);

  if(rpp && buf) {
    snprintf(buf, bufSize, "%s", rpp);
    return true;
  }
  else
    return false;
}

int CF_EnumMediaSourceCues(PCM_source *source, const int index, double *time, double *endTime, bool *isRegion, char *name, const int nameSize)
{
  REAPER_cue cue{};
  const int add = source->Extended(PCM_SOURCE_EXT_ENUMCUES_EX, (void *)(intptr_t)index, &cue, NULL);

  if(time)
    *time = cue.m_time;
  if(endTime)
    *endTime = cue.m_endtime;
  if(isRegion)
    *isRegion = cue.m_isregion;

  if(name && cue.m_name)
    snprintf(name, nameSize, "%s", cue.m_name);

  return add ? index + add : 0;
}

bool CF_ExportMediaSource(PCM_source *source, const char *file)
{
  return source->Extended(PCM_SOURCE_EXT_EXPORTTOFILE, (void *)file, NULL, NULL) > 0;
}
