/******************************************************************************
/ acendan.cpp
/
/ Copyright (c) 2022 acendan
/ https://forum.cockos.com/member.php?u=142333
/ https://github.com/acendan/reascripts
/ https://aaroncendan.me
/
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
#include "acendan.h"
#include "cover.h"

namespace acendan
{
	Player player;
	
	// Load the image data from a file into a temp .jpg file. Temp file is destroyed on REAPER shutdown.
	bool GetMediaFileCoverImage(const char* path, char* temppathOut, int temppathOut_sz)
	{
		if (!path || !*path || !temppathOut || temppathOut_sz <= 0) return false;
		*temppathOut = 0;

		if (acendan::player.FindCover(path))
		{
			const auto& coverPath = acendan::player.GetCoverPath();
			if (!coverPath.empty())
			{
				snprintf(temppathOut, temppathOut_sz, "%s", coverPath.c_str());
			}
		}

		return !!*temppathOut;
	}
}

/******************************************************************************/
// Register commands in Actions List - must have !WANT_LOCAL... comments!
// { { DEFACCEL, "ACendan: Hello World" }, "AC_HELLO_WORLD", acendan::HelloWorld, NULL },

//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] =
{

	{ {}, LAST_COMMAND, },
};
//!WANT_LOCALIZE_1ST_STRING_END

int acendan_Init()
{
	SWSRegisterCommands(g_commandTable);
	return 1;
}
