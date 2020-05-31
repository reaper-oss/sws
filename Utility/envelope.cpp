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

// wraps GetEnvelopeStateChunk() to make it more save
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
			throw bad_get_env_chunk_big(__LOCALIZE("std::bad_alloc thrown.", "sws_mbox"));
		}
	}

	return {};
}
