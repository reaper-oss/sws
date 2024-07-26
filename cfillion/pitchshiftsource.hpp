/******************************************************************************
/ pitchsiftsource.hpp
/
/ Copyright (c) 2022 Christian Fillion
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

#include <vector>
#include <WDL/mutex.h>

class PitchShiftSource : public PCM_source {
public:
  static PitchShiftSource *create(PCM_source *src);

  PitchShiftSource(PCM_source *src);
  ~PitchShiftSource();

  PCM_source *Duplicate() override { return nullptr; }
  bool   IsAvailable() override { return m_src->IsAvailable(); }
  bool   SetFileName(const char *) override { return false; }
  int    GetNumChannels() override;
  double GetSampleRate() override { return m_src->GetSampleRate(); }
  double GetLength() override;
  int    PropertiesWindow(HWND) override { return 0; }
  // marked final: does important bookkeeping that subclasses shouldn't bypassed
  void   GetSamples(PCM_source_transfer_t *block) override final;
  void   GetPeakInfo(PCM_source_peaktransfer_t *) override {}
  void   SaveState(ProjectStateContext *) override {}
  int    LoadState(const char *, ProjectStateContext *) override { return -1; }
  void   Peaks_Clear(bool) override {}
  int    PeaksBuild_Begin() override { return 0; }
  int    PeaksBuild_Run() override { return 0; }
  void   PeaksBuild_Finish() override {}

  // only safe to call from the main thread
  bool   isPastEnd(double position);
  double getVolume() { return m_volume; }
  void   setVolume(double v);
  double getPan() { return m_pan; }
  void   setPan(double p);
  double getPlayRate() { return m_rate; }
  void   setPlayRate(double);
  double getPitch() { return m_pitch; }
  void   setPitch(double);
  bool   getPreservePitch() { return m_flags & PreservePitch; }
  void   setPreservePitch(bool);
  int    getMode() { return m_mode; }
  void   setMode(int);
  double getFadeInLen() { return m_fadeInLen; }
  void   setFadeInLen(double);
  double getFadeOutLen() { return m_fadeOutLen; }
  void   setFadeOutLen(double);
  bool   startFadeOut();
  bool   readPeak(size_t, double *);
  virtual bool requestStop() { return true; }
  void seekOrLoop(bool looping);

protected:
  struct Block {
    PCM_source_transfer_t *tx;
    double sampleTime, fadeOutStart;
    bool isSeek;
  };
  struct Peak {
    bool read;
    double max;
  };
  enum Flags {
    PreservePitch = 1<<0,
    AllNotesOff   = 1<<1,
    StopRequest   = 1<<2,
    StopServiced  = 1<<3,
    WrappedAround = 1<<4,
    ManualSeek    = 1<<5,
    Looping       = 1<<6,
  };

  // must lock m_mutex before using these
  virtual double sourceLength() const = 0;
  virtual void writeSamples(const Block &) = 0;
  virtual void writePeaks(const PCM_source_transfer_t *) = 0;
  virtual void updateTempoShift() = 0;
  double computeGain(const Block &, double time, int samplesUntilNextCall);

  double m_pitch, m_rate, m_volume, m_pan, m_fadeInLen, m_fadeOutLen;
  int m_flags, m_mode;

  WDL_SharedMutex m_mutex;
  PCM_source *m_src;
  double m_playTime;  // for fade-ins, position-independent
  double m_writeTime; // for seek detection and fade-outs
  double m_fadeOutEnd;
  std::vector<Peak> m_peaks;
};

class PitchShiftSource_Audio final : public PitchShiftSource {
public:
  PitchShiftSource_Audio(PCM_source *);
  ~PitchShiftSource_Audio() override;

  const char *GetType() override { return "SWS_PITCHSHIFT_AUDIO"; }

protected:
  double sourceLength() const override;
  void writeSamples(const Block &) override;
  void writePeaks(const PCM_source_transfer_t *) override;
  void updateTempoShift() override;

private:
  void getShiftedSamples(const Block &);

  double m_readTime; // for pitch-shifting
  IReaperPitchShift *m_ps;
};

class PitchShiftSource_MIDI final : public PitchShiftSource {
public:
  PitchShiftSource_MIDI(PCM_source *);

  const char *GetType() override { return "SWS_PITCHSHIFT_MIDI"; }

  bool requestStop() override;

protected:
  double sourceLength() const override;
  void writeSamples(const Block &) override;
  void writePeaks(const PCM_source_transfer_t *) override;
  void updateTempoShift() override;

private:
  void addCCAllChans(MIDI_eventlist *events, unsigned char cc, unsigned char val);
};
