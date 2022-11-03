/******************************************************************************
/ preview.hpp
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

#include "pitchshiftsource.hpp"

class CF_Preview {
public:
  static bool isValid(CF_Preview *);
  static void stopAll();

  CF_Preview(PCM_source *);
  ~CF_Preview();

  double getPosition();
  void   setPosition(double);
  bool   getPeak(int channel, double *out);
  int    getOutputChannel() { return m_reg.m_out_chan; }
  void   setOutput(int channel);
  void   setOutput(MediaTrack *);
  bool   getLoop() { return m_reg.loop; }
  void   setLoop(bool);
  double getMeasureAlign() { return m_measureAlign; }
  void   setMeasureAlign(double ma) { m_measureAlign = ma; }

  // proxy to/from PitchShiftSource
  double getLength() { return m_src.GetLength(); }
  // m_reg.volume does not apply when previewing through a track!
  double getVolume() { return m_src.getVolume(); }
  void   setVolume(double v) { m_src.setVolume(v); }
  double getPan() { return m_src.getPan(); }
  void   setPan(double pan) { m_src.setPan(pan); }
  double getPlayRate() { return m_src.getPlayRate(); }
  void   setPlayRate(double playRate) { m_src.setPlayRate(playRate); }
  double getPitch() { return m_src.getPitch(); }
  void   setPitch(double pitch) { m_src.setPitch(pitch); }
  bool   getPreservePitch() { return m_src.getPreservePitch(); }
  void   setPreservePitch(bool preserve) { m_src.setPreservePitch(preserve); }
  int    getPitchMode() { return m_src.getMode(); }
  void   setPitchMode(int mode) { m_src.setMode(mode); }
  double getFadeInLen() { return m_src.getFadeInLen(); }
  void   setFadeInLen(double len) { m_src.setFadeInLen(len); }
  double getFadeOutLen() { return m_src.getFadeOutLen(); }
  void   setFadeOutLen(double len) { m_src.setFadeOutLen(len); }

  bool isDangling();
  bool play(bool useMeasureAlign = true);
  void stop(bool startFadeOut = true);

private:
  enum State { Idle, Playing, FadeOut };
  State m_state;
  double m_measureAlign;
  ReaProject *m_project;
  PitchShiftSource m_src;
  preview_register_t m_reg;
};
