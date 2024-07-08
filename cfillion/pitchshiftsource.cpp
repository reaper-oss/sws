#include "stdafx.h"
#include "pitchshiftsource.hpp"
#include "../Breeder/BR_Util.h" // GetSourceType

#include <WDL/denormal.h>

PitchShiftSource *PitchShiftSource::create(PCM_source *src)
{
  if(GetSourceType(src) == SourceType::MIDI) // "MIDI" or "MIDIPOOL"
    return new PitchShiftSource_MIDI(src);
  else
    return new PitchShiftSource_Audio(src);
}

PitchShiftSource::PitchShiftSource(PCM_source *src)
  : m_pitch { 0.0 }, m_rate { 1.0 }, m_volume { 1.0 }, m_pan { 0.0 },
    m_fadeInLen { 0.0 }, m_fadeOutLen { 0.0 }, m_flags { PreservePitch },
    m_mode { -1 }, m_src { src->Duplicate() },
    m_playTime { 0.0 }, m_writeTime { 0.0 }, m_fadeOutEnd { 0.0 }
{
}

PitchShiftSource::~PitchShiftSource()
{
  PCM_Source_Destroy(m_src);
}

PitchShiftSource_Audio::PitchShiftSource_Audio(PCM_source *src)
  : PitchShiftSource { src }, m_readTime { 0.0 },
    m_ps { ReaperGetPitchShiftAPI(REAPER_PITCHSHIFT_API_VER) }
{
  updateTempoShift();
  m_peaks.resize(GetNumChannels());
}

PitchShiftSource_Audio::~PitchShiftSource_Audio()
{
  delete m_ps; // unsafe on Windows (different C++ runtime = different heap)?
}

PitchShiftSource_MIDI::PitchShiftSource_MIDI(PCM_source *src)
  : PitchShiftSource { src }
{
  updateTempoShift();
  m_peaks.resize(16);
}

int PitchShiftSource::GetNumChannels()
{
  // Force stereo for applying pan when outputting to a stereo hardware output.
  //
  // REAPER already calls GetSamples with block->nch=2 when outputting through
  // a track even when the source reports having a single channel.
  //
  // This does not interfere with the preview's mono flag (I_OUTCHAN | 1024).
  const int chans { m_src->GetNumChannels() };
  return chans == 1 ? 2 : chans;
}

double PitchShiftSource::GetLength()
{
  // Returning a truncated length (m_fadeOutLen) here would cause
  // a 1 buffer glitch when it kicks in.
  WDL_MutexLockShared lock { &m_mutex };
  return sourceLength();
}

double PitchShiftSource_Audio::sourceLength() const
{
  return m_src->GetLength() / m_rate;
}

double PitchShiftSource_MIDI::sourceLength() const
{
  return m_src->GetLength();
}

bool PitchShiftSource::isPastEnd(const double position)
{
  WDL_MutexLockShared lock { &m_mutex };
  const double length { m_fadeOutEnd ? m_fadeOutEnd : sourceLength() };
  return position >= length || m_flags & StopServiced;
}

bool PitchShiftSource::readPeak(const size_t chan, double *out)
{
  if(chan >= m_peaks.size())
    return false;

  WDL_MutexLockExclusive lock { &m_mutex };
  Peak &peak { m_peaks[chan] };
  peak.read = true;
  *out = peak.max;
  return true;
}

void PitchShiftSource::GetSamples(PCM_source_transfer_t *tx)
{
  Block block { tx };
  block.sampleTime = 1.0 / tx->samplerate;

  {
    WDL_MutexLockShared lock { &m_mutex };
    const double effectiveLength = m_fadeOutEnd ? m_fadeOutEnd : sourceLength();
    block.fadeOutStart = effectiveLength - m_fadeOutLen;
    block.isSeek = m_writeTime != tx->time_s;
    if(block.isSeek && tx->time_s == 0.0 && !(m_flags & (ManualSeek | Looping)))
      m_flags |= WrappedAround;
    writeSamples(block);
  }

  WDL_MutexLockExclusive lock { &m_mutex };

  for(Peak &peak : m_peaks) {
    if(peak.read)
      peak = {};
  }
  if(!(m_flags & WrappedAround))
    writePeaks(tx);

  if(block.isSeek)
    m_writeTime = tx->time_s;
  m_writeTime += tx->length * block.sampleTime;

  // update flags (even though they're MIDI ones) once we have a write lock
  if(m_flags & StopRequest)
    m_flags |= StopServiced;
  m_flags &= ~(AllNotesOff | StopRequest);
  if(block.isSeek) // wait until a seek is received to avoid a race condition
    m_flags &= ~ManualSeek;
}

double PitchShiftSource::computeGain(const Block &block,
  const double time, const int samplesUntilNextCall)
{
  const double
    fadeIn { m_playTime < m_fadeInLen ? m_playTime / m_fadeInLen : 1.0 },
    timeInFadeOut { m_fadeOutLen ? time - block.fadeOutStart : 0.0 },
    fadeOut { timeInFadeOut > 0 ? 1 - (timeInFadeOut / m_fadeOutLen) : 1.0 };
  m_playTime += samplesUntilNextCall * block.sampleTime;
  return m_volume * std::max(0.0, fadeIn * fadeOut);
}

void PitchShiftSource_Audio::writeSamples(const Block &block)
{
  if(m_rate == 1.0 && m_pitch == 0.0) {
    m_src->GetSamples(block.tx);
    m_readTime = 0;
  }
  else
    getShiftedSamples(block);

  // no pan law
  const double pan[] {
    m_pan > 0 ? 1.0 - m_pan : 1.0, // left
    m_pan < 0 ? m_pan + 1.0 : 1.0, // right
  };

  ReaSample *sample { block.tx->samples },
            *lastSample { sample + (block.tx->samples_out * block.tx->nch) };
  for(double time { block.tx->time_s }; sample < lastSample; sample += block.tx->nch) {
    const double gain { computeGain(block, time, 1) };
    for(int i {}; i < block.tx->nch; ++i)
      sample[i] *= gain * pan[i & 1];
    time += block.sampleTime;
  }
}

void PitchShiftSource_Audio::getShiftedSamples(const Block &block)
{
  m_ps->set_srate(block.tx->samplerate);
  m_ps->set_nch(block.tx->nch);

  const double bufSizeMul { m_rate > 1.0 ? m_rate : 1.0 };
  PCM_source_transfer_t sourceBlock {};
  sourceBlock.samplerate = block.tx->samplerate;
  sourceBlock.nch = block.tx->nch;
  sourceBlock.length = static_cast<int>(block.tx->length * bufSizeMul);

  if(block.isSeek || !m_readTime) {
    m_readTime  = block.tx->time_s * m_rate;
    m_ps->Reset(); // to give immediate feedback with very slow play rates
  }

  bool isEOF = false;
  do {
    // feed the pitch shifter until it has enough data to fill the output block
    sourceBlock.samples = m_ps->GetBuffer(sourceBlock.length);
    sourceBlock.time_s = m_readTime;
    m_src->GetSamples(&sourceBlock);
    m_ps->BufferDone(sourceBlock.samples_out);
    m_readTime += sourceBlock.samples_out * block.sampleTime;

    isEOF = sourceBlock.samples_out < sourceBlock.length;
    if(isEOF)
      m_ps->FlushSamples();

    const int remaining { block.tx->length - block.tx->samples_out };
    ReaSample *sample { block.tx->samples + (block.tx->samples_out * block.tx->nch) };
    block.tx->samples_out += m_ps->GetSamples(remaining, sample);
  } while(block.tx->samples_out < block.tx->length && !isEOF);
}

static unsigned char clamp7b(const int v)
{
  return std::max(std::min(127, v), 0);
}

void PitchShiftSource_MIDI::writeSamples(const Block &block)
{
  if(!block.tx->midi_events) // null when outputting to a hardware output
    return;

  if(block.isSeek || m_flags & (AllNotesOff | StopRequest))
    addCCAllChans(block.tx->midi_events, MIDI_event_t::CC_ALL_NOTES_OFF, 0);

  if(m_flags & (StopRequest | StopServiced))
    return;

  m_src->GetSamples(block.tx);

  const double gain { computeGain(block, block.tx->time_s, block.tx->length) };
  for(int i = 0; MIDI_event_t *event { block.tx->midi_events->EnumItems(&i) };) {
    if(event->is_note())
      event->midi_message[1] = clamp7b(event->midi_message[1] + static_cast<int>(m_pitch));
    if(event->is_note_on())
      event->midi_message[2] = clamp7b(static_cast<int>(event->midi_message[2] * gain));
  }
}

void PitchShiftSource_MIDI::addCCAllChans(MIDI_eventlist *events,
  const unsigned char cc, const unsigned char val)
{
  for(unsigned char chan {}; chan < 16; ++chan) {
    MIDI_event_t event { 0, 3,
      { static_cast<unsigned char>(0xb0 | chan), cc, val } };
    events->AddItem(&event);
  }
}

void PitchShiftSource_Audio::writePeaks(const PCM_source_transfer_t *block)
{
  const size_t peakChans { std::min<size_t>(m_peaks.size(), block->nch) };
  for(ReaSample *sample { block->samples },
                *lastSample { sample + (block->samples_out * block->nch) };
      sample < lastSample; sample += block->nch) {
    for(size_t c = 0; c < peakChans; ++c)
      GetDoubleMaxAbsValue(&m_peaks[c].max, &sample[c]);
  }
}

void PitchShiftSource_MIDI::writePeaks(const PCM_source_transfer_t *block)
{
  if(!block->midi_events) // null when outputting to a hardware output
    return;

  for(int i = 0; MIDI_event_t *event { block->midi_events->EnumItems(&i) };) {
    if(event->is_note_on()) {
      const double value { event->midi_message[2] / 127.0 };
      GetDoubleMaxAbsValue(&m_peaks[event->midi_message[0] & 0xF].max, &value);
    }
  }
}

void PitchShiftSource_Audio::updateTempoShift()
{
  double shift { pow(2.0, m_pitch / 12.0) };
  if(!(m_flags & PreservePitch))
    shift *= m_rate;

  m_ps->SetQualityParameter(m_mode);
  m_ps->set_tempo(m_rate);
  m_ps->set_shift(shift);

  // to have getShiftedSamples reset m_readTime and m_ps next time it's used
  if(m_rate == 1.0 && m_pitch == 0.0)
    m_writeTime = 0.0;
}

void PitchShiftSource_MIDI::updateTempoShift()
{
  m_flags |= AllNotesOff;

  double tempo { 120 * m_rate };
  m_src->Extended(PCM_SOURCE_EXT_SETPREVIEWTEMPO, &tempo, nullptr, nullptr);
}

void PitchShiftSource::setVolume(const double volume)
{
  // OK to read unlocked: only called/modified from the main thread
  if(volume == m_volume || volume < 0)
    return; // don't exclusively lock the mutex for no reason

  WDL_MutexLockExclusive lock { &m_mutex };
  m_volume = volume;
}

void PitchShiftSource::setPan(const double pan)
{
  if(pan == m_pan || pan < -1 || pan > 1)
    return;

  WDL_MutexLockExclusive lock { &m_mutex };
  m_pan = pan;
}

void PitchShiftSource::setPlayRate(const double playRate)
{
  // rubberband crashes at rates < 0.005 and preserving pitch
  if(playRate < 0.01 || playRate > 100 || playRate == m_rate)
    return;

  WDL_MutexLockExclusive lock { &m_mutex };
  m_rate = playRate;
  updateTempoShift();
}

void PitchShiftSource::setPitch(const double pitch)
{
  if(pitch == m_pitch)
    return;

  WDL_MutexLockExclusive lock { &m_mutex };
  m_pitch = pitch;
  updateTempoShift();
}

void PitchShiftSource::setPreservePitch(const bool preservePitch)
{
  if(preservePitch == !!(m_flags & PreservePitch))
    return;

  WDL_MutexLockExclusive lock { &m_mutex };
  m_flags &= ~PreservePitch;
  if(preservePitch)
    m_flags |= PreservePitch;
  updateTempoShift();
}

void PitchShiftSource::setMode(const int mode)
{
  if(mode == m_mode)
    return;

  WDL_MutexLockExclusive lock { &m_mutex };
  m_mode = mode;
  updateTempoShift();
}

void PitchShiftSource::setFadeInLen(const double len)
{
  if(len == m_fadeInLen)
    return;

  WDL_MutexLockExclusive lock { &m_mutex };
  m_fadeInLen = len;
}

void PitchShiftSource::setFadeOutLen(const double len)
{
  if(len == m_fadeOutLen)
    return;

  WDL_MutexLockExclusive lock { &m_mutex };
  m_fadeOutLen = len;
}

bool PitchShiftSource::startFadeOut()
{
  if(!m_fadeOutLen)
    return false;

  WDL_MutexLockExclusive lock { &m_mutex };
  m_fadeOutEnd = m_writeTime + m_fadeOutLen;
  return true;
}

bool PitchShiftSource_MIDI::requestStop()
{
  WDL_MutexLockExclusive lock { &m_mutex };
  if(m_flags & StopServiced) {
    m_flags &= ~StopServiced;
    return true;
  }
  m_flags |= StopRequest;
  return false;
}

void PitchShiftSource::seekOrLoop(const bool looping)
{
  WDL_MutexLockExclusive lock { &m_mutex };
  m_flags &= ~(Looping | WrappedAround);
  m_flags |= ManualSeek;
  if(looping)
    m_flags |= Looping;
}
