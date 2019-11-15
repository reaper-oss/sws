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
  static const unsigned int CLIPBOARD_FORMAT = CF_UNICODETEXT;
#else
// on SWELL/generic CF_TEXT may be implemented as a function call which
// may not be available until after loading is complete
#  define CLIPBOARD_FORMAT (CF_TEXT)
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
  SetClipboardData(CLIPBOARD_FORMAT, mem);
  CloseClipboard();
}

class ClipboardReader {
public:
  ClipboardReader()
  {
    OpenClipboard(GetMainHwnd());
    m_mem = GetClipboardData(CLIPBOARD_FORMAT);
    m_data = reinterpret_cast<decltype(m_data)>(GlobalLock(m_mem));

#ifdef _WIN32
    m_size = WideCharToMultiByte(CP_UTF8, 0, m_data, -1, nullptr, 0, nullptr, nullptr) - 1;
#else
    m_size = strlen(m_data);
#endif
  }

  // does not null-terminate the output buffer unless its bigger than the
  // clipboard data
  void read(char *buf, int bufSize)
  {
    if(bufSize > m_size) {
      bufSize = m_size;
      buf[m_size] = 0;
    }

#ifdef _WIN32
    WideCharToMultiByte(CP_UTF8, 0, m_data, -1, buf, bufSize, nullptr, nullptr);
#else
    std::copy(m_data, m_data + bufSize, buf);
#endif
  }

  ~ClipboardReader()
  {
    GlobalUnlock(m_mem);
    CloseClipboard();
  }

  int size() const { return m_size; }
  operator bool() const { return m_data != nullptr; }

private:
  HANDLE m_mem;
#ifdef _WIN32
  const wchar_t *m_data;
#else
  const char *m_data;
#endif
  int m_size;
};

void CF_GetClipboard(char *buf, int bufSize)
{
  if(ClipboardReader clipboard{}) {
    realloc_cmd_ptr(&buf, &bufSize, clipboard.size());
    clipboard.read(buf, bufSize);
  }
}

const char *CF_GetClipboardBig(WDL_FastString *output)
{
  ClipboardReader clipboard;

  if(!clipboard || g_script_strs.Find(output) == -1)
    return nullptr;

  output->SetLen(clipboard.size());
  clipboard.read(const_cast<char *>(output->Get()), output->GetLength());

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
  return ShellExecute(nullptr, "open", file, args, nullptr, SW_SHOW);
#endif
}

bool CF_LocateInExplorer(const char *file)
{
  // Quotes inside the filename must not be escaped for the SWELL implementation
  WDL_FastString arg;
  arg.SetFormatted(strlen(file) + 10, R"(/select,"%s")", file);

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

HWND CF_GetTrackFXChain(MediaTrack *track)
{
  std::ostringstream chainTitle;
  chainTitle << __LOCALIZE("FX: ", "fx");

  if(track == GetMasterTrack(nullptr))
    chainTitle << __LOCALIZE("Master Track", "fx");
  else {
    const int trackNumber = static_cast<int>(
      GetMediaTrackInfo_Value(track, "IP_TRACKNUMBER"));
    const char *trackName = static_cast<char *>(
      GetSetMediaTrackInfo(track, "P_NAME", nullptr));

    chainTitle << __LOCALIZE("Track", "fx") << ' ' << trackNumber;

    if(strlen(trackName) > 0)
      chainTitle << " \"" << trackName << '"';
  }

  return FindWindowEx(nullptr, nullptr, nullptr, chainTitle.str().c_str());
}

HWND CF_GetTakeFXChain(MediaItem_Take *take)
{
  // Shameful hack follows. The FX chain window does have some user data
  // attached to it (pointer to an internal FxChain object?) but it does not
  // seem to hint back to the take in any obvious way.

  GUID *guid = static_cast<GUID *>(GetSetMediaItemTakeInfo(take, "GUID", nullptr));

  char guidStr[64];
  guidToString(guid, guidStr);

  string originalName = GetTakeName(take);
  GetSetMediaItemTakeInfo_String(take, "P_NAME", guidStr, true);

  char chainTitle[128];
  snprintf(chainTitle, sizeof(chainTitle), R"(%s%s "%s")",
    __LOCALIZE("FX: ", "fx"), __LOCALIZE("Item", "fx"), guidStr);

  HWND window = FindWindowEx(nullptr, nullptr, nullptr, chainTitle);

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
  case 1: {
    MediaTrack *track = GetTrack(nullptr, trackIndex - 1);
    return CF_GetTrackFXChain(track);
  }
  case 2: {
    MediaTrack *track = GetTrack(nullptr, trackIndex - 1);
    MediaItem *item = GetTrackMediaItem(track, itemIndex);
    MediaItem_Take *take = GetMediaItemTake(item, HIWORD(fxIndex));
    return CF_GetTakeFXChain(take);
  }
  default:
    return nullptr;
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
  char *rpp = nullptr;
  source->Extended(PCM_SOURCE_EXT_GETASSOCIATED_RPP, &rpp, nullptr, nullptr);

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
  const int add = source->Extended(PCM_SOURCE_EXT_ENUMCUES_EX, (void *)(intptr_t)index, &cue, nullptr);

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
  return source->Extended(PCM_SOURCE_EXT_EXPORTTOFILE, (void *)file, nullptr, nullptr) > 0;
}
