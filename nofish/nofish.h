/******************************************************************************
/ nofish.h
/
/ Copyright (c) 2016 nofish
/ http://forum.cockos.com/member.php?u=6870
/ https://github.com/nofishonfriday/sws
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

int nofish_Init();

// #514
void UpdateMIDIGridToolbar();

// #587 utility functions
class NFTrackItemUtilities
{
public:
	// NFTrackItemUtilities();
	// ~NFTrackItemUtilities();

	void NFSaveSelectedTracks();
	void NFRestoreSelectedTracks();

	void NFSaveSelectedItems();
	void NFRestoreSelectedItems();

	void NFUnselectAllTracks();
	void NFSelectTracksOfSelectedItems();

	int NFCountSelectedItems_OnTrack(MediaTrack* track);
	MediaItem* NFGetSelectedItems_OnTrack(int track_sel_id, int idx);

	// const vector<int>& NFGetIntVector() const;
	vector <int> count_sel_items_on_track; // should probably be private for good coding practice
	
	int GetMaxValfromIntVector(vector <int>);

private:
	// vector <int> count_sel_items_on_track;

	// save/restore tracks/items
	vector <MediaItem*> selItems;
	vector <MediaTrack*> selTracks;
};

/*
NFTrackItemUtilities::NFTrackItemUtilities()
{
}

NFTrackItemUtilities::~NFTrackItemUtilities()
{
}
*/



