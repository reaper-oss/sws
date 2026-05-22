/******************************************************************************
/ envelope.hpp
/
/ Copyright (c) 2019
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

#include <stdexcept>

namespace envelope
{

std::string GetEnvelopeStateChunkBig(TrackEnvelope *, bool isUndo = false);

class bad_get_env_chunk_big : public std::runtime_error {
public:
	using runtime_error::runtime_error;
};

class FlatEnvPoints {
public:
	struct Point { int ai, id; double pos; bool sel; };

	FlatEnvPoints(TrackEnvelope *env);

	bool empty() const { return m_points.empty(); }
	size_t size() const { return m_points.size(); }
	const Point &operator[](const size_t i) const { return m_points[i]; }

	const Point *findLast(bool (*)(const Point &)) const;
	const Point *findFirst(bool (*)(const Point &)) const;

	std::vector<Point>::const_iterator begin() const { return m_points.begin(); }
	std::vector<Point>::const_iterator end() const { return m_points.end(); }

private:
	std::vector<Point> m_points;
};

}
