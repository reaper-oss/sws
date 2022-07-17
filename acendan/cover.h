/******************************************************************************
/ cover.h
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
#pragma once

#include <taglib/fileref.h>		// Required for forward declaration of CCover::GetEmbedded

/******************************************************************************/
/*
* The following classes have been adapted from Rainmeter's implementation of
* TagLib for parsing media cover art. Rainmeter is open source, covered under
* a GNU GPL v2 license. See respective file attributions below...
*
* Copyright (C) 2011 Rainmeter Project Developers
*
* This Source Code Form is subject to the terms of the GNU General Public
* License; either version 2 of the License, or (at your option) any later
* version. If a copy of the GPL was not distributed with this file, You can
* obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>.
*
* Rainmeter/Library/NowPlaying/Cover.h
* https://github.com/rainmeter/rainmeter/blob/master/Library/NowPlaying/Cover.h
*/
class CCover
{
public:
	static bool GetCached(std::string& path);
	static bool GetLocal(std::string filename, const std::string& folder, std::string& target);
	static bool GetEmbedded(const TagLib::FileRef& fr, const std::string& target);
	static std::string GetFileFolder(const std::string& file);
};

/******************************************************************************/
/*
* Rainmeter/Library/NowPlaying/Player.h
* https://github.com/rainmeter/rainmeter/blob/master/Library/NowPlaying/Player.h
*/
class Player
{
public:
	Player();
	~Player();

	bool FindCover(const std::string& fp);
	std::string GetCoverPath();

protected:
	std::string m_TempCoverPath;	// Path to temp image file
	std::string m_CoverPath;		// Path to cover art image
	std::string m_FilePath;			// Path to media file
};
