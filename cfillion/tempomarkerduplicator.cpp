#include "stdafx.h"
#include "tempomarkerduplicator.hpp"

#include <cassert>
#include <WDL/localize/localize.h>

static TrackEnvelope *getTempoMap(ReaProject *project)
{
  MediaTrack *masterTrack { GetMasterTrack(project) };
  return GetTrackEnvelopeByName(masterTrack, __LOCALIZE("Tempo map", "env"));
}

TempoMarkerDuplicator::TempoMarkerDuplicator(ReaProject *sourceProject)
  : m_chunk { envelope::GetEnvelopeStateChunkBig(getTempoMap(sourceProject)) },
    m_lastLineOffset { 0 }
{
  m_points.reserve(CountTempoTimeSigMarkers(sourceProject));

  Point point {};
  const char *end { m_chunk.c_str() + m_chunk.size() };

  while(point.offset < m_chunk.size()) {
    m_lastLineOffset = point.offset;
    const char *line { m_chunk.c_str() + point.offset };

    while(line + point.size < end && line[point.size] != '\n')
      ++point.size;
    ++point.size; // include the line/null terminator

    if(1 == sscanf(line, "PT %lf%n", &point.position, &point.prefixSize))
      m_points.push_back(point);

    point.offset += point.size;
    point.size = 0;
  }

  // just in case REAPER doesn't always give points in position order
  std::sort(m_points.begin(), m_points.end());
}

void TempoMarkerDuplicator::copyRange(
  const double startPos, const double endPos, const double distance)
{
  auto it { std::lower_bound(m_points.begin(), m_points.end(), startPos) };
  const auto end { std::upper_bound(it, m_points.end(), endPos) };

  // find the last marker before the source region
  // and copy it at the start of the target region (left edge point)
  if(it != m_points.begin() && it->position > startPos)
    copy(*(it - 1), startPos + distance);

  while(it != end) {
    copy(*it, it->position + distance);
    ++it;
  }

  // copy the last marker before the target region (right edge point)
  // (only if we copied any other points above)
  if(!m_newPoints.empty()) {
    const auto close { std::lower_bound(m_points.begin(), m_points.end(), startPos + distance) };
    if(close != m_points.begin())
      copy(*(close - 1), endPos + distance);
  }
}

void TempoMarkerDuplicator::copy(const Point &point, const double newPos)
{
  // shortcut: not updating chunk offsets in m_newPoints
  // should be fine as long as points are inserted in forward order
  assert(m_newPoints.empty() || m_newPoints.back().position <= newPos);

  // upper bound to overwrite existing point at newPos
  // (later points have priority when more than one at the same position)
  const auto target { std::upper_bound(m_points.begin(), m_points.end(), newPos) };

  Point newPoint;
  newPoint.position = newPos;

  char prefix[255];
  newPoint.prefixSize = snprintf(prefix, sizeof(prefix),
    "PT %.12f", newPoint.position); // %.12f: same precision as REAPER v6.68

  std::string line { m_chunk.substr(point.offset, point.size) };
  line.replace(0, point.prefixSize, prefix);

  newPoint.size = line.size();
  if(target == m_points.end())
    newPoint.offset = m_lastLineOffset;
  else
    newPoint.offset = target->offset;

  // update the offsets into m_chunk of future and later points
  m_lastLineOffset += newPoint.size;
  for(auto it { target }; it != m_points.end(); ++it)
    it->offset += newPoint.size;

  m_chunk.insert(newPoint.offset, line);
  // inserting into m_points right now would invalidate iterators
  // m_points.insert(target, newPoint);
  // also, future calls to copyRange need to only see the original points
  // so that it may insert the correct edge point on the right side
  m_newPoints.push_back(newPoint);
}

void TempoMarkerDuplicator::commit(ReaProject *targetProject)
{
  if(!m_newPoints.empty()) {
    m_points.reserve(m_points.size() + m_newPoints.size());
    std::copy(m_newPoints.begin(), m_newPoints.end(), std::back_inserter(m_points));
    std::sort(m_points.begin(), m_points.end());
    m_newPoints.clear();
  }

  // overriding item timebase to time so they don't move or stretch
  std::vector<double> timebases;
  timebases.reserve(CountMediaItems(targetProject));
  for(int i = 0; i < CountMediaItems(targetProject); ++i) {
    MediaItem *item { GetMediaItem(targetProject, i) };
    timebases.push_back(GetMediaItemInfo_Value(item, "C_BEATATTACHMODE"));
    SetMediaItemInfo_Value(item, "C_BEATATTACHMODE", 0); // 0 = time
  }

  SetEnvelopeStateChunk(getTempoMap(targetProject), m_chunk.c_str(), false);

  // workaround for applying metronome pattern immediately
  // https://github.com/reaper-oss/sws/pull/1562#issuecomment-1281842721
  //
  // also necessary for the item timebase restoration below to apply
  double p1, p3, p4; int p2, p5, p6; bool p7;
  if(GetTempoTimeSigMarker(targetProject, 0, &p1, &p2, &p3, &p4, &p5, &p6, &p7))
    SetTempoTimeSigMarker(targetProject, 0, p1, p2, p3, p4, p5, p6, p7);

  for(size_t i = 0; i < timebases.size(); ++i) {
    MediaItem *item { GetMediaItem(targetProject, i) };
    SetMediaItemInfo_Value(item, "C_BEATATTACHMODE", timebases[i]);
  }
}
