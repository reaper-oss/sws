#include "stdafx.h"
#include "preview.hpp"

WDL_PtrList_DOD<CF_Preview> g_previews;

enum Flags {
  Buffered  = 1,
  Varispeed = 2,
};

class LockPreviewMutex {
public:
  LockPreviewMutex(preview_register_t &reg)
    : m_reg { reg }
  {
#ifdef _WIN32
    EnterCriticalSection(&m_reg.cs);
#else
    pthread_mutex_lock(&m_reg.mutex);
#endif
  }

  ~LockPreviewMutex()
  {
#ifdef _WIN32
    LeaveCriticalSection(&m_reg.cs);
#else
    pthread_mutex_unlock(&m_reg.mutex);
#endif
  }

private:
  preview_register_t &m_reg;
};

static void stopWatch()
{
  for(int i { g_previews.GetSize() - 1 }; i >= 0; --i) {
    CF_Preview *preview { g_previews.Get(i) };
    if(preview->isDangling())
      delete preview;
  }
}

bool CF_Preview::isValid(CF_Preview *ptr)
{
  const int index { g_previews.Find(ptr) };
  return index >= 0 && g_previews.Get(index)->m_state != FadeOut;
}

void CF_Preview::stopAll()
{
  for(int i { g_previews.GetSize() - 1 }; i >= 0; --i)
    g_previews.Get(i)->stop();
}

CF_Preview::CF_Preview(PCM_source *source)
  : m_state { Idle }, m_measureAlign { 0.0 },
    m_project { nullptr }, m_src { source },
    m_reg { {}, &m_src, 0, 0.0, false, 1.0 }
{
#ifdef _WIN32
  InitializeCriticalSection(&m_reg.cs);
#else
  pthread_mutex_init(&m_reg.mutex, nullptr);
#endif

  g_previews.Add(this);

  if(g_previews.GetSize() == 1)
    plugin_register("timer", reinterpret_cast<void *>(&stopWatch));
}

CF_Preview::~CF_Preview()
{
  g_previews.Delete(g_previews.Find(this), false);

  if(g_previews.GetSize() == 0)
    plugin_register("-timer", reinterpret_cast<void *>(&stopWatch));

  stop(false);

#ifdef _WIN32
  DeleteCriticalSection(&m_reg.cs);
#else
  pthread_mutex_destroy(&m_reg.mutex);
#endif
}

bool CF_Preview::isDangling()
{
  return
    m_state == Idle || m_src.isPastEnd(getPosition()) ||
    (m_project && (!ValidatePtr(m_project, "ReaProject*") ||
                   !ValidatePtr2(m_project, m_reg.preview_track, "MediaTrack*")))
  ;
}

bool CF_Preview::play(const bool obeyMeasureAlign)
{
  if(m_state != Idle)
    return true;

  constexpr int flags { Buffered | Varispeed };
  const double measureAlign { obeyMeasureAlign ? m_measureAlign : 0.0 };

  if(m_project ? !!PlayTrackPreview2Ex(m_project, &m_reg, flags, measureAlign)
               : !!PlayPreviewEx(&m_reg, flags, measureAlign)) {
    m_state = Playing;
    return true;
  }

  return false;
}

void CF_Preview::stop(const bool startFadeOut)
{
  if(startFadeOut) {
    m_src.setFadeOutEnd(getPosition() + getFadeOutLen());
    m_state = FadeOut;
    return;
  }

  if(m_project)
    StopTrackPreview2(m_project, &m_reg);
  else
    StopPreview(&m_reg);

  m_state = Idle;
}

double CF_Preview::getPosition()
{
  LockPreviewMutex lock { m_reg };
  return m_reg.curpos;
}

void CF_Preview::setPosition(const double newpos)
{
  LockPreviewMutex lock { m_reg };
  m_reg.curpos = newpos;
}

bool CF_Preview::getPeak(const int channel, double *out)
{
  if(channel < 0 || channel >= 2)
    return false;

  LockPreviewMutex lock { m_reg };
  *out = m_reg.peakvol[channel];
  m_reg.peakvol[channel] = 0;
  return true;
}

void CF_Preview::setOutput(const int channel)
{
  if(channel < 0)
    return;

  const bool restart { m_state > Idle && m_project };
  if(restart) {
    StopTrackPreview2(m_project, &m_reg);
    m_state = Idle;
  }

  {
    LockPreviewMutex lock { m_reg };
    m_project = nullptr;
    m_reg.m_out_chan = channel;
    m_reg.preview_track = nullptr;
  }

  if(restart)
    play(false);
}

void CF_Preview::setOutput(MediaTrack *track)
{
  if(track == m_reg.preview_track)
    return;

  const bool restart { m_state > Idle };
  if(restart)
    stop(false);

  m_project = static_cast<ReaProject *>
    (GetSetMediaTrackInfo(track, "P_PROJECT", nullptr));
  m_reg.m_out_chan = -1;
  m_reg.preview_track = track;

  if(restart)
    play(false);
}

void CF_Preview::setLoop(const bool loop)
{
  LockPreviewMutex lock { m_reg };
  m_reg.loop = loop;
}
