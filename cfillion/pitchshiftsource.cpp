#include "stdafx.h"
#include "pitchshiftsource.hpp"

#include <WDL/denormal.h>

PitchShiftSource::PitchShiftSource(PCM_source *src)
  : m_pitch { 0.0 }, m_rate { 1.0 }, m_volume { 1.0 }, m_pan { 0.0 },
    m_fadeInLen { 0.0 }, m_fadeOutLen { 0.0 }, m_preservePitch { true },
    m_mode { -1 }, m_src { src->Duplicate() },
    m_ps { ReaperGetPitchShiftAPI(REAPER_PITCHSHIFT_API_VER) },
    m_readTime { 0.0 }, m_writeTime { 0.0 }, m_playTime { 0.0 }, m_fadeOutEnd { 0.0 }
{
  updateTempoShift();
  m_ps->SetQualityParameter(m_mode);

  m_peaks.resize(GetNumChannels());
}

PitchShiftSource::~PitchShiftSource()
{
  PCM_Source_Destroy(m_src);

  delete m_ps; // unsafe on Windows (different C++ runtime = different heap)?
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
  WDL_MutexLockShared lock { &m_mutex };
  return currentLength();
}

double PitchShiftSource::currentLength()
{
  return m_src->GetLength() / m_rate;
}

bool PitchShiftSource::isPastEnd(const double position)
{
  WDL_MutexLockShared lock { &m_mutex };
  return position >= currentLength() || (m_fadeOutEnd && position >= m_fadeOutEnd);
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

void PitchShiftSource::GetSamples(PCM_source_transfer_t *block)
{
  m_mutex.LockShared();

  if(m_rate == 1.0 && m_pitch == 0.0)
    m_src->GetSamples(block);
  else
    getShiftedSamples(block);

  const double sampleTime   { 1.0 / block->samplerate },
               fadeOutEnd   { m_fadeOutEnd ? m_fadeOutEnd : currentLength() },
               fadeOutStart { fadeOutEnd - m_fadeOutLen },
               blockEndTime { block->time_s + (block->length * sampleTime) };

  if(m_volume != 1.0 || m_pan != 0.0 ||
     m_playTime < m_fadeInLen || blockEndTime >= fadeOutStart)
    applyGain(block, sampleTime, fadeOutStart);
  else
    m_playTime += block->length * sampleTime;

  m_mutex.UnlockShared();

  WDL_MutexLockExclusive lock { &m_mutex };
  updatePeaks(block);
}

void PitchShiftSource::applyGain(PCM_source_transfer_t *block,
  const double sampleTime, const double fadeOutStart)
{
  // no pan law
  const double pan[] {
    m_pan > 0 ? 1.0 - m_pan : 1.0, // left
    m_pan < 0 ? m_pan + 1.0 : 1.0, // right
  };

  ReaSample *sample { block->samples },
            *lastSample { sample + (block->samples_out * block->nch) };

  for(double time { block->time_s }; sample < lastSample; sample += block->nch) {
    const double fadeIn { m_playTime < m_fadeInLen ? m_playTime / m_fadeInLen : 1.0 },
                 timeInFadeOut { m_fadeOutLen ? time - fadeOutStart : 0.0 },
                 fadeOut { timeInFadeOut > 0 ? 1 - (timeInFadeOut / m_fadeOutLen) : 1.0 },
                 gain { m_volume * std::max(0.0, fadeIn * fadeOut) };
    for(int i {}; i < block->nch; ++i)
      sample[i] *= gain * pan[i & 1];
    time += sampleTime;
    m_playTime += sampleTime;
  }
}

void PitchShiftSource::getShiftedSamples(PCM_source_transfer_t *block)
{
  m_ps->set_srate(block->samplerate);
  m_ps->set_nch(block->nch);

  const double bufSizeMul { m_rate > 1.0 ? m_rate : 1.0 };
  PCM_source_transfer_t sourceBlock {};
  sourceBlock.samplerate = block->samplerate;
  sourceBlock.nch = block->nch;
  sourceBlock.length = static_cast<int>(block->length * bufSizeMul);

  const double sampleTime { 1.0 / block->samplerate };
  if(m_writeTime != block->time_s) {
    // reset m_readTime when seeking
    m_readTime  = block->time_s * m_rate;
    m_writeTime = block->time_s;
    m_ps->Reset(); // to give immediate feedback with very slow play rates
  }
  m_writeTime += block->length * sampleTime;

  do {
    const int remaining { block->length - block->samples_out };
    ReaSample *sample { block->samples + (block->samples_out * block->nch) };
    block->samples_out += m_ps->GetSamples(remaining, sample);
    if(block->samples_out >= block->length)
      break; // output block is full

    // feed the pitch shifter until it has enough data to fill the output block
    sourceBlock.samples = m_ps->GetBuffer(sourceBlock.length);
    sourceBlock.time_s = m_readTime;
    m_src->GetSamples(&sourceBlock);
    m_ps->BufferDone(sourceBlock.samples_out);
    m_readTime += sourceBlock.samples_out * sampleTime;
  } while(sourceBlock.samples_out == sourceBlock.length); // until EOF
}

void PitchShiftSource::updatePeaks(const PCM_source_transfer_t *block)
{
  for(Peak &peak : m_peaks) {
    if(peak.read)
      peak = {};
  }

  const size_t peakChans { std::min<size_t>(m_peaks.size(), block->nch) };
  for(ReaSample *sample { block->samples },
                *lastSample { sample + (block->samples_out * block->nch) };
      sample < lastSample; sample += block->nch) {
    for(size_t c = 0; c < peakChans; ++c)
      GetDoubleMaxAbsValue(&m_peaks[c].max, &sample[c]);
  }
}

void PitchShiftSource::updateTempoShift()
{
  double shift { pow(2.0, m_pitch / 12.0) };
  if(!m_preservePitch)
    shift *= m_rate;

  m_ps->set_tempo(m_rate);
  m_ps->set_shift(shift);

  // to have getShiftedSamples reset m_readTime and m_ps next time it's used
  if(m_rate == 1.0 && m_pitch == 0.0)
    m_writeTime = 0.0;
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
  if(preservePitch == m_preservePitch)
    return;

  WDL_MutexLockExclusive lock { &m_mutex };
  m_preservePitch = preservePitch;
  updateTempoShift();
}

void PitchShiftSource::setMode(const int mode)
{
  if(mode == m_mode)
    return;

  WDL_MutexLockExclusive lock { &m_mutex };
  m_ps->SetQualityParameter(mode);
  m_mode = mode;
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

void PitchShiftSource::setFadeOutEnd(const double time)
{
  if(time == m_fadeOutEnd)
    return;

  WDL_MutexLockExclusive lock { &m_mutex };
  m_fadeOutEnd = time;
}
