/******************************************************************************
/ cover.cpp
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
#include "cover.h"

#include <taglib/ape/apefile.h>
#include <taglib/ape/apetag.h>
#include <taglib/asf/asffile.h>
#include <taglib/mpeg/id3v1/id3v1genres.h>
#include <taglib/mpeg/id3v2/id3v2tag.h>
#include <taglib/mpeg/id3v2/frames/attachedpictureframe.h>
#include <taglib/mpeg/id3v2/frames/commentsframe.h>
#include <taglib/mpeg/id3v2/frames/textidentificationframe.h>
#include <taglib/flac/flacfile.h>
#include <taglib/mpc/mpcfile.h>
#include <taglib/mpeg/mpegfile.h>
#include <taglib/mp4/mp4file.h>
#include <taglib/tag.h>
#include <taglib/toolkit/taglib.h>
#include <taglib/toolkit/tstring.h>
#include <taglib/ogg/vorbis/vorbisfile.h>
#include <taglib/riff/wav/wavfile.h>

#include <sys/stat.h>		// For FileExists()

/******************************************************************************/
namespace
{
	// Check whether file exists on filesystem. Could be swapped with std::filesystem::exists() if needed/preferred.
	bool FileExists(const std::string& filepath)
	{
		struct stat buffer;
		return (stat(filepath.c_str(), &buffer) == 0);
	}

	// Get temporary file for cover art image
	std::string GetTempFilePath(const std::string& extension = ".jpg")
	{
		char buffer[L_tmpnam];
		tmpnam(buffer);
		std::string tempFile = buffer;
		tempFile = tempFile.substr(0, tempFile.find_last_of(".")) + extension;
		return tempFile;
	}

	// Write ByteVector image data to file
	bool WriteCoverToFile(const TagLib::ByteVector& data, const std::string& target)
	{
		FILE* f = fopen(target.c_str(), "wb");
		if (f)
		{
			const bool written = fwrite(data.data(), 1, data.size(), f) == data.size();
			fclose(f);
			return written;
		}

		return false;
	}

	// APE: https://www.monkeysaudio.com/developers.html
	bool ExtractCoverAPE(TagLib::APE::Tag* tag, const std::string& target)
	{
		const TagLib::APE::ItemListMap& listMap = tag->itemListMap();
		if (listMap.contains("COVER ART (FRONT)"))
		{
			const TagLib::ByteVector nullStringTerminator(1, 0);
			TagLib::ByteVector item = listMap["COVER ART (FRONT)"].value();
			const int pos = item.find(nullStringTerminator);	// Skip the filename.
			if (pos != -1)
			{
				const TagLib::ByteVector& pic = item.mid(pos + 1);
				return WriteCoverToFile(pic, target);
			}
		}

		return false;
	}

	// ID3: https://id3.org/
	bool ExtractCoverID3(TagLib::ID3v2::Tag* tag, const std::string& target)
	{
		const TagLib::ID3v2::FrameList& frameList = tag->frameList("APIC");
		if (!frameList.isEmpty())
		{
			// Just grab the first image.
			const auto* frame = (TagLib::ID3v2::AttachedPictureFrame*)frameList.front();
			return WriteCoverToFile(frame->picture(), target);
		}

		return false;
	}

	// ASF: https://docs.microsoft.com/en-us/windows/win32/wmformat/overview-of-the-asf-format
	bool ExtractCoverASF(TagLib::ASF::File* file, const std::string& target)
	{
		const TagLib::ASF::AttributeListMap& attrListMap = file->tag()->attributeListMap();
		if (attrListMap.contains("WM/Picture"))
		{
			const TagLib::ASF::AttributeList& attrList = attrListMap["WM/Picture"];
			if (!attrList.isEmpty())
			{
				// Let's grab the first cover. TODO: Check/loop for correct type.
				const TagLib::ASF::Picture& wmpic = attrList[0].toPicture();
				if (wmpic.isValid())
				{
					return WriteCoverToFile(wmpic.picture(), target);
				}
			}
		}

		return false;
	}

	// FLAC: https://xiph.org/flac/
	bool ExtractCoverFLAC(TagLib::FLAC::File* file, const std::string& target)
	{
		const TagLib::List<TagLib::FLAC::Picture*>& picList = file->pictureList();
		if (!picList.isEmpty())
		{
			// Just grab the first image.
			const TagLib::FLAC::Picture* pic = picList[0];
			return WriteCoverToFile(pic->data(), target);
		}

		return false;
	}

	// MPEG: https://www.mpeg.org/
	bool ExtractCoverMP4(TagLib::MP4::File* file, const std::string& target)
	{
		TagLib::MP4::Tag* tag = file->tag();
		const TagLib::MP4::ItemListMap& itemListMap = tag->itemListMap();
		if (itemListMap.contains("covr"))
		{
			const TagLib::MP4::CoverArtList& coverArtList = itemListMap["covr"].toCoverArtList();
			if (!coverArtList.isEmpty())
			{
				const TagLib::MP4::CoverArt* pic = &(coverArtList.front());
				return WriteCoverToFile(pic->data(), target);
			}
		}

		return false;
	}

}  // namespace


/******************************************************************************/
// Checks if cover art is in cache.
bool CCover::GetCached(std::string& path)
{
	path += ".art";
	return FileExists(path);
}

// Attempts to find local cover art in various formats.
bool CCover::GetLocal(std::string filename, const std::string& folder, std::string& target)
{
	std::string testPath = folder + filename + ".";
	std::string::size_type origLen = testPath.length();

	const int extCount = 4;
	std::string extName[extCount] = { "jpg", "jpeg", "png", "bmp" };

	for (int i = 0; i < extCount; ++i)
	{
		testPath += extName[i];
		if (FileExists(testPath))
		{
			target = testPath;
			return true;
		}
		else
		{
			// Get rid of the added extension
			testPath.resize(origLen);
		}
	}

	return false;
}

// Attempts to extract cover art from audio files.
bool CCover::GetEmbedded(const TagLib::FileRef& fr, const std::string& target)
{
	bool found = false;

	if (TagLib::MPEG::File* file = dynamic_cast<TagLib::MPEG::File*>(fr.file()))
	{
		if (file->ID3v2Tag())
		{
			found = ExtractCoverID3(file->ID3v2Tag(), target);
		}
		if (!found && file->APETag())
		{
			found = ExtractCoverAPE(file->APETag(), target);
		}
	}
	else if (TagLib::FLAC::File* file = dynamic_cast<TagLib::FLAC::File*>(fr.file()))
	{
		found = ExtractCoverFLAC(file, target);

		if (!found && file->ID3v2Tag())
		{
			found = ExtractCoverID3(file->ID3v2Tag(), target);
		}
	}
	else if (TagLib::RIFF::WAV::File* file = dynamic_cast<TagLib::RIFF::WAV::File*>(fr.file()))
	{
		if (auto* tag = file->ID3v2Tag())
		{
			found = ExtractCoverID3(tag, target);
		}
	}
	else if (TagLib::MP4::File* file = dynamic_cast<TagLib::MP4::File*>(fr.file()))
	{
		found = ExtractCoverMP4(file, target);
	}
	else if (TagLib::ASF::File* file = dynamic_cast<TagLib::ASF::File*>(fr.file()))
	{
		found = ExtractCoverASF(file, target);
	}
	else if (TagLib::APE::File* file = dynamic_cast<TagLib::APE::File*>(fr.file()))
	{
		if (file->APETag())
		{
			found = ExtractCoverAPE(file->APETag(), target);
		}
	}
	else if (TagLib::MPC::File* file = dynamic_cast<TagLib::MPC::File*>(fr.file()))
	{
		if (file->APETag())
		{
			found = ExtractCoverAPE(file->APETag(), target);
		}
	}

	return found;
}

// Returns path without filename
std::string CCover::GetFileFolder(const std::string& file)
{
	std::string::size_type pos = file.find_last_of(L'\\');
	if (pos != std::string::npos)
	{
		return file.substr(0, ++pos);
	}

	return file;
}

/******************************************************************************/
Player::Player()
{
	m_TempCoverPath = GetTempFilePath();
}

Player::~Player()
{
	// Delete temp image file
	std::remove(m_TempCoverPath.c_str());
}

bool Player::FindCover(const std::string& fp)
{
	m_FilePath = fp;
	m_CoverPath.clear();

	TagLib::FileRef fr(m_FilePath.c_str(), false);
	if (!fr.isNull() && CCover::GetEmbedded(fr, m_TempCoverPath))
	{
		m_CoverPath = m_TempCoverPath;
	}
	else
	{
		std::string trackFolder = CCover::GetFileFolder(m_FilePath);

		if (!CCover::GetLocal("cover", trackFolder, m_CoverPath) &&
			!CCover::GetLocal("folder", trackFolder, m_CoverPath))
		{
			// Nothing found
			m_CoverPath.clear();
		}
	}
	return !m_CoverPath.empty();
}

std::string Player::GetCoverPath()
{
	return m_CoverPath;
}
