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

void CF_Preview::stopWatch()
{
  for(int i { g_previews.GetSize() - 1 }; i >= 0; --i) {
    CF_Preview *preview { g_previews.Get(i) };
    if(preview->isDangling() && preview->stop(false))
      delete preview;
    else
      preview->asyncTick();
  }
}

bool CF_Preview::isValid(CF_Preview *preview)
{
  if(g_previews.Find(preview) < 0)
    return false;

  switch(preview->m_state) {
  case FadeOut:
  case AsyncStop:
    return false;
  default:
    return true;
  }
}

void CF_Preview::stopAll()
{
  for(int i { g_previews.GetSize() - 1 }; i >= 0; --i)
    g_previews.Get(i)->stop();
}

CF_Preview::PreviewInst::PreviewInst(PitchShiftSource *src)
  : preview_register_t { {}, src, 0, 0.0, false, 1.0 }, project { nullptr }
{
}

CF_Preview::CF_Preview(PCM_source *source)
  : m_state { Idle }, m_measureAlign { 0.0 },
    m_src { PitchShiftSource::create(source) },
    m_reg { m_src }, m_async { nullptr }
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

  stop(false, false);

#ifdef _WIN32
  DeleteCriticalSection(&m_reg.cs);
#else
  pthread_mutex_destroy(&m_reg.mutex);
#endif

  delete m_src;
}

bool CF_Preview::isDangling()
{
  switch(m_state) {
  case Idle:
    return true;
  case AsyncStop:
  case AsyncRestart:
    return false;
  default:
    break;
  }

  // checking state to not miss destroying previews after fade-outs
  if((!m_reg.loop || m_state != Playing) && m_src->isPastEnd(getPosition()))
    return true;
  else if(!m_reg.project)
    return false;

  return !ValidatePtr(m_reg.project, "ReaProject*") ||
        !ValidatePtr2(m_reg.project, m_reg.preview_track, "MediaTrack*");
}

bool CF_Preview::play(const bool obeyMeasureAlign)
{
  if(m_state == AsyncStop)
    m_state = AsyncRestart;
  if(m_state != Idle)
    return true;

  constexpr int flags { Buffered | Varispeed };
  const double measureAlign { obeyMeasureAlign ? m_measureAlign : 0.0 };

  if(m_reg.project ? !!PlayTrackPreview2Ex(m_reg.project, &m_reg, flags, measureAlign)
                   : !!PlayPreviewEx(&m_reg, flags, measureAlign)) {
    m_state = Playing;
    return true;
  }

  return false;
}

bool CF_Preview::stop(const bool allowFadeOut, const bool allowAsync)
{
  if(m_state == Idle)
    return true;
  if(allowFadeOut && m_src->startFadeOut()) {
    m_state = FadeOut;
    return false;
  }
  else if(allowAsync && !m_src->requestStop()) {
    if(m_state != AsyncRestart) {
      m_state = AsyncStop;

      // make REAPER call the source's GetSamples to service the stop request
      LockPreviewMutex lock { m_reg };
      if(m_src->isPastEnd(m_reg.curpos))
        m_reg.curpos = 0;
    }
    return false;
  }

  if(m_reg.project)
    StopTrackPreview2(m_reg.project, &m_reg);
  else
    StopPreview(&m_reg);

  m_state = Idle;
  return true;
}

void CF_Preview::asyncTick()
{
  const bool restart { m_state == AsyncRestart };
  if(m_state == AsyncStop || restart)
    stop(false); // sets state to Idle when ready to restart
  if(m_state == Idle && restart) {
    m_reg.project       = m_async.project;
    m_reg.m_out_chan    = m_async.m_out_chan;
    m_reg.preview_track = m_async.preview_track;
    play(false);
  }
}

CF_Preview::PreviewInst &CF_Preview::writableInst()
{
  switch(m_state) {
  case AsyncStop:
  case AsyncRestart:
    return m_async;
  default:
    return m_reg;
  }
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
  m_src->seekOrLoop(m_reg.loop);
}

void CF_Preview::setOutput(const int channel)
{
  if(channel < 0)
    return;

  const bool restart { m_state > Idle && m_reg.project };
  if(restart)
    stop(false);

  {
    PreviewInst &inst { writableInst() };
    LockPreviewMutex lock { inst };
    inst.project = nullptr;
    inst.m_out_chan = channel;
    inst.preview_track = nullptr;
  }

  if(restart)
    play(false);
}

void CF_Preview::setOutput(MediaTrack *track)
{
  if(track == m_reg.preview_track && m_state != AsyncRestart)
    return;

  const bool restart { m_state > Idle };
  if(restart)
    stop(false);

  PreviewInst &inst { writableInst() };
  inst.project = static_cast<ReaProject *>
    (GetSetMediaTrackInfo(track, "P_PROJECT", nullptr));
  inst.m_out_chan = -1;
  inst.preview_track = track;

  if(restart)
    play(false);
}

void CF_Preview::setLoop(const bool loop)
{
  LockPreviewMutex lock { m_reg };
  m_reg.loop = loop;
  m_src->seekOrLoop(m_reg.loop);
}
