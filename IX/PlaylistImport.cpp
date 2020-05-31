/******************************************************************************
/ PlaylistImport.cpp
/
/ Copyright (c) 2012 Philip S. Considine (IX)
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

#include "IX.h"

#include <WDL/localize/localize.h>

struct SPlaylistEntry
{
	double length;
	string title;
	string path;
	bool exists;
};

// Fill outbuf with filenames retrieved from playlist
void ParseM3U(string listpath, vector<SPlaylistEntry> &filelist)
{
	char buf[1024] = {0};
	ifstream file(listpath.c_str(), ios_base::in);

	if(file.is_open())
	{
		// m3u playlist file should start with the string "#EXTM3U"
		file.getline(buf, sizeof(buf));
		string str = buf;
		if(str.find("#EXTM3U") != 0)
		{
			return;
		}

		string pathroot = listpath.substr(0, listpath.find_last_of("\\") + 1);

		// Each entry in the playlist consists of two lines.
		// The first contains the length in seconds and the title, the second should be the file path relative to the playlist location.
		//	#EXTINF:331,Bernard Pretty Purdie - Hap'nin'
		//	Music\Bernard Pretty Purdie\Lialeh\07 - Hap'nin'.mp3
		// Streaming media should have length -1
		while(!file.eof())
		{
			SPlaylistEntry e{};

			file.getline(buf, sizeof(buf));
			string str = buf;

			if(str.find("#EXTINF:") == 0)
			{
				str = str.substr(str.find_first_of(":") + 1); // Remove "#EXTINF:"

				e.length = atof(str.substr(0, str.find_first_of(",")).c_str());

				if(e.length < 0) continue; // Streaming media should have length -1

				e.title = str.substr(str.find_first_of(",") + 1);

				file.getline(buf, sizeof(buf));
				e.path = buf;

				// Make path absolute if not already
				if(e.path.find(':') == string::npos)
				{
					e.path.insert(0, pathroot);
				}

				filelist.push_back(e);
			}
		}
	}

	file.close();
}

// Fill filelist with info retrieved from the playlist
void ParsePLS(string listpath, vector<SPlaylistEntry> &filelist)
{
	char buf[1024] = {0};
	ifstream file(listpath.c_str(), ios_base::in);

	if(file.is_open())
	{
		// pls playlist file should start with the string "[playlist]"
		file.getline(buf, sizeof(buf));
		string str = buf;
		if(str.find("[playlist]") == string::npos)
		{
			return;
		}

		string pathroot = listpath.substr(0, listpath.find_last_of("\\") + 1);

		// Find number of entries
		int count = 0;
		while(!file.eof())
		{
			file.getline(buf, sizeof(buf));
			str = buf;
			if(str.find("NumberOfEntries") != string::npos)
			{
				count = atoi(str.substr(str.find_first_of("=") + 1).c_str());
				filelist.resize(count);
				break;
			}
		}

		if(count == 0) return;

		// Each entry in the playlist consists of three lines:
		//	File1=E:\Music\James Brown\20 all time greatest hits!\13 - Get On The Good Foot.mp3
		//	Title1=James Brown - Get On The Good Foot
		//	Length1=215
		// Streaming media should have length -1
		file.seekg(0);
		while(!file.eof())
		{
			file.getline(buf, sizeof(buf));
			string str = buf;

			if(str.empty()) continue;

			str = str.erase(str.find_last_of('\r')); // Winamp pls files have extra carriage returns which screw up file_exists()

			if(str.find("File") == 0)
			{
				str = str.substr(4); // Strip "File"
				int index = atoi(str.substr(0, str.find_first_of("=")).c_str()) - 1;

				if(index > -1 && index < count)
				{
					SPlaylistEntry &e = filelist.at(index);
					e.path = str.substr(str.find_first_of("=") + 1);

					// Make path absolute if not already
					if(e.path.find(':') == string::npos)
					{
						e.path.insert(0, pathroot);
					}
				}
			}
			else if(str.find("Title") == 0)
			{
				str = str.substr(5); // Strip "Title"
				int index = atoi(str.substr(0, str.find_first_of("=")).c_str()) - 1;

				if(index > -1 && index < count)
				{
					SPlaylistEntry &e = filelist.at(index);
					e.title = str.substr(str.find_first_of("=") + 1);
				}
			}
			else if(str.find("Length") == 0)
			{
				str = str.substr(6); // Strip "Length"
				int index = atoi(str.substr(0, str.find_first_of("=")).c_str()) - 1;

				if(index > -1 && index < count)
				{
					SPlaylistEntry &e = filelist.at(index);
					e.length = atoi(str.substr(str.find_first_of("=") + 1).c_str());
				}
			}
		}
	}

	// Remove streaming media
	filelist.erase(remove_if(filelist.begin(), filelist.end(),
		[](const SPlaylistEntry &e) { return e.length < 0; }), filelist.end());

	file.close();
}

// Import local files from m3u/pls playlists onto a new track
void PlaylistImport(COMMAND_T* ct)
{
	char cPath[256];
	GetProjectPath(cPath, sizeof(cPath));

	string listpath;
	if(char *path = BrowseForFiles(__LOCALIZE("Import playlist","sws_mbox"), cPath, NULL, false, "Playlist files (*.m3u,*.pls)\0*.m3u;*.pls\0All Files (*.*)\0*.*\0"))
	{
		listpath = path;
		free(path);
	}
	else
		return;

	const string &ext = ParseFileExtension(listpath);

	vector<SPlaylistEntry> filelist;

	// Decide what kind of playlist we have
	if(ext == "m3u")
		ParseM3U(listpath, filelist);
	else if(ext == "pls")
		ParsePLS(listpath, filelist);

	if(filelist.empty())
	{
		ShowMessageBox(__LOCALIZE("Failed to import playlist. No files found.","sws_mbox"), __LOCALIZE("Import playlist","sws_mbox"), 0);
		return;
	}

	// Validate files
	size_t badfiles = 0;
	for(SPlaylistEntry &e : filelist)
	{
		e.exists = file_exists(e.path.c_str());

		if(!e.exists)
			++badfiles;
	}

	// If files can't be found, ask user what to do.
	if(badfiles > 0)
	{
		const size_t limit = min<size_t>(badfiles, 9); // avoid enormous messagebox

		stringstream ss;
		ss << __LOCALIZE("The following files cannot be found. Create items for them anyway?\n","sws_mbox");

		size_t n = 0;
		for(const SPlaylistEntry &e : filelist) {
			if(!e.exists)
			{
				ss << "\n " << e.path;

				if(++n >= limit)
					break;
			}
		}

		if(badfiles > limit)
			ss << "\n +" << badfiles - limit << __LOCALIZE(" more files","sws_mbox");

		switch(ShowMessageBox(ss.str().c_str(), __LOCALIZE("Import playlist", "sws_mbox"), MB_YESNOCANCEL)) {
		case IDCANCEL:
			return;
		case IDNO:
			filelist.erase(remove_if(filelist.begin(), filelist.end(),
				[](const SPlaylistEntry &e) { return !e.exists; }), filelist.end());
			break;
		}
	}

	Undo_BeginBlock2(NULL);

	// Add new track
	int idx = GetNumTracks();
	int panMode = 5;		// Stereo pan
	double panLaw = 1.0;	// 0dB
	InsertTrackAtIndex(idx, false);
	MediaTrack *pTrack = GetTrack(NULL, idx);
	GetSetMediaTrackInfo(pTrack, "P_NAME", (void*) listpath.c_str());
	GetSetMediaTrackInfo(pTrack, "I_PANMODE", (void*) &panMode);
	GetSetMediaTrackInfo(pTrack, "D_PANLAW", (void*) &panLaw);
	SetOnlyTrackSelected(pTrack);

	// Add new items to track
	double pos = 0.0;
	for(const SPlaylistEntry &e : filelist)
	{
		PCM_source *pSrc = PCM_Source_CreateFromFile(e.path.c_str());
		if(!pSrc)
			continue;

		MediaItem *pItem = AddMediaItemToTrack(pTrack);
		if(!pItem)
			continue;

		MediaItem_Take *pTake = AddTakeToMediaItem(pItem);
		if(!pTake)
			continue;

		GetSetMediaItemTakeInfo(pTake, "P_SOURCE", pSrc);
		GetSetMediaItemTakeInfo(pTake, "P_NAME", (void*) e.title.c_str());
		SetMediaItemPosition(pItem, pos, false);
		SetMediaItemLength(pItem, e.length, false);
		pos += e.length;
	}

	Undo_EndBlock2(NULL, SWS_CMD_SHORTNAME(ct), UNDO_STATE_ITEMS|UNDO_STATE_TRACKCFG);

	TrackList_AdjustWindows(false);
	UpdateTimeline();

	Main_OnCommand(40047, 0); // Build missing peaks
}

//!WANT_LOCALIZE_1ST_STRING_BEGIN:sws_actions
static COMMAND_T g_commandTable[] =
{
	{ { DEFACCEL, "SWS/IX: Import m3u/pls playlist" },	"IX_PLAYLIST_IMPORT",	PlaylistImport,	NULL, 0},

	{ {}, LAST_COMMAND, }, // Denote end of table
};
//!WANT_LOCALIZE_1ST_STRING_END

int PlaylistImportInit()
{
	SWSRegisterCommands(g_commandTable);

	return 1;
}
