/******************************************************************************
/ tempomarkerduplicator.hpp
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

class TempoMarkerDuplicator {
public:
  TempoMarkerDuplicator(ReaProject *sourceProject = nullptr);
  void copyRange(double startPos, double endPos, double distance);
  void commit(ReaProject *targetProject = nullptr);

private:
  struct Point {
    double position;
    size_t offset, size;
    int prefixSize;

    operator double() const { return position; }
  };

  std::string m_chunk;
  size_t m_lastLineOffset;
  std::vector<Point> m_points, m_newPoints;

  void copy(const Point &, double newPos);
};
