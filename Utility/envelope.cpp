/******************************************************************************
/ envelope.cpp
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

#include "stdafx.h"

#include "envelope.hpp"

#include <WDL/localize/localize.h>

// wraps GetEnvelopeStateChunk() to make it more safe
// until maybe it gets enhanced one day
// https://forum.cockos.com/showthread.php?p=2142245#post2142245
// throws envelope::bad_get_env_chunk_big on failure
std::string envelope::GetEnvelopeStateChunkBig(TrackEnvelope *envelope, bool isUndo /*= false*/)
{
	std::string buffer(1024, '\0');

	while (true) {
		// can't use std::string::front (C++11) as we're targeting OSX 10.5 (as of September 2019)
		// though &operator[0] should be ok:
		// https://github.com/reaper-oss/sws/pull/1202#issuecomment-532476783
		// if (!GetEnvelopeStateChunk(envelope, &buffer.front(), buffer.size() + 1, isUndo))
		if (!GetEnvelopeStateChunk(envelope, &buffer[0], buffer.size() + 1, isUndo))
			break;

		const size_t endpos = buffer.find('\0');

		if (endpos != std::string::npos) {
			buffer.resize(endpos);
			return buffer;
		}

		if (buffer.size() > 100 << 20) { // 100 MiB
			throw bad_get_env_chunk_big(__LOCALIZE("The envelope chunk size exceeded the 100 MiB limit.", "sws_mbox"));
		}

		try {
			buffer.resize(buffer.size() * 2);
		}
		catch (const std::bad_alloc &) {
			throw bad_get_env_chunk_big(__LOCALIZE("Could not allocate envelope chunk memory.", "sws_mbox"));
		}
	}

	return {};
}

envelope::FlatEnvPoints::FlatEnvPoints(TrackEnvelope *env)
{
	const std::string &chunk = GetEnvelopeStateChunkBig(env);
	const auto actStart = chunk.find("\nACT ");
	int aiOptions = -1;
	if (actStart != std::string::npos) {
		LineParser lp(false);
		lp.parse(&chunk[actStart]);
		aiOptions = lp.gettoken_int(2);
	}
	if (aiOptions < 0) aiOptions = *ConfigVar<int>("pooledenvattach");

	const bool envelopeBypassed = aiOptions & 4;
	const int aiCount = CountAutomationItems(env);

	if (!envelopeBypassed) {
		int ai = 0;
		double nextAiStart = DBL_MAX, nextAiEnd = DBL_MAX;
		if (aiCount > 0) {
			nextAiStart = GetSetAutomationItemInfo(env, ai, "D_POSITION", 0, false);
			nextAiEnd = GetSetAutomationItemInfo(env, ai, "D_LENGTH", 0, false) + nextAiStart;
		}

		const int points = CountEnvelopePoints(env);
		m_points.reserve(points);
		for (int pi = 0; pi < points; ++pi) {
			Point pt {-1, pi};
			GetEnvelopePoint(env, pi, &pt.pos, nullptr, nullptr, nullptr, &pt.sel);

			while (pt.pos >= nextAiEnd) {
				if (++ai < aiCount) {
					nextAiStart = GetSetAutomationItemInfo(env, ai, "D_POSITION", 0, false);
					nextAiEnd = GetSetAutomationItemInfo(env, ai, "D_LENGTH", 0, false) + nextAiStart;
				}
				else {
					nextAiStart = nextAiEnd = DBL_MAX;
					break;
				}
			}

			if (pt.pos < nextAiStart)
				m_points.push_back(pt);
		}
	}

	for (int ai = 0; ai < aiCount; ++ai) {
		const int points = CountEnvelopePointsEx(env, ai);
		m_points.reserve(m_points.size() + points);
		for (int pi = 0; pi < points; ++pi) {
			Point pt {ai, pi};
			GetEnvelopePointEx(env, ai, pi, &pt.pos, nullptr, nullptr, nullptr, &pt.sel);
			m_points.push_back(pt);
		}
	}

	if (aiCount > 0) {
		std::sort(m_points.begin(), m_points.end(), [](const Point &a, const Point &b) {
			return a.pos < b.pos;
		});
	}
}

auto envelope::FlatEnvPoints::findFirst(bool (*pred)(const Point &)) const -> const Point *
{
	auto it = std::find_if(m_points.begin(), m_points.end(), pred);
	return it == m_points.end() ? nullptr : &*it;
}

auto envelope::FlatEnvPoints::findLast(bool (*pred)(const Point &)) const -> const Point *
{
	auto it = std::find_if(m_points.rbegin(), m_points.rend(), pred);
	return it == m_points.rend() ? nullptr : &*it;
}
