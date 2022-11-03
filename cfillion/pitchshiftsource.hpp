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

#include <WDL/mutex.h>

class PitchShiftSource : public PCM_source {
public:
  PitchShiftSource(PCM_source *src);
  ~PitchShiftSource();

  PCM_source *Duplicate() override { return nullptr; }
  bool   IsAvailable() override { return m_src->IsAvailable(); }
  const  char *GetType() override { return "SWS_PITCHSHIFT"; }
  bool   SetFileName(const char *) override { return false; }
  int    GetNumChannels() override { return m_src->GetNumChannels(); }
  double GetSampleRate() override { return m_src->GetSampleRate(); }
  double GetLength() override;
  int    PropertiesWindow(HWND) override { return 0; }
  void   GetSamples(PCM_source_transfer_t *block) override;
  void   GetPeakInfo(PCM_source_peaktransfer_t *) override {}
  void   SaveState(ProjectStateContext *) override {}
  int    LoadState(const char *, ProjectStateContext *) override { return -1; }
  void   Peaks_Clear(bool) override {}
  int    PeaksBuild_Begin() override { return 0; }
  int    PeaksBuild_Run() override { return 0; }
  void   PeaksBuild_Finish() override {}

  // only safe to call from the main thread
  bool isPastEnd(double position);
  double getVolume() { return m_volume; }
  void   setVolume(double v);
  double getPan() { return m_pan; }
  void   setPan(double p);
  double getPlayRate() { return m_rate; }
  void   setPlayRate(double);
  double getPitch() { return m_pitch; }
  void   setPitch(double);
  bool   getPreservePitch() { return m_preservePitch; }
  void   setPreservePitch(bool);
  int    getMode() { return m_mode; }
  void   setMode(int);
  double getFadeInLen() { return m_fadeInLen; }
  void   setFadeInLen(double);
  double getFadeOutLen() { return m_fadeOutLen; }
  void   setFadeOutLen(double);
  double getFadeOutEnd() { return m_fadeOutEnd; }
  void   setFadeOutEnd(double);

private:
  // must lock m_mutex before using these
  double currentLength();
  void getShiftedSamples(PCM_source_transfer_t *);
  void applyGain(PCM_source_transfer_t *, double sampleTime, double fadeOutStart);
  void updateTempoShift();

  double m_pitch, m_rate, m_volume, m_pan, m_fadeInLen, m_fadeOutLen;
  bool m_preservePitch;
  int m_mode;

  WDL_SharedMutex m_mutex;
  PCM_source *m_src;
  IReaperPitchShift *m_ps;
  double m_readTime, m_writeTime; // for pitch-shifting
  double m_playTime; // position-independent
  double m_fadeOutEnd;
};
