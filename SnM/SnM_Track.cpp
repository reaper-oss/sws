/******************************************************************************
/ SnM_Track.cpp
/
/ Copyright (c) 2009-2010 Tim Payne (SWS), JF Bédague
/ http://www.standingwaterstudios.com/reaper
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
#include "SnM_Actions.h"
#include "SNM_Chunk.h"
#include "SnM_FXChainView.h"


///////////////////////////////////////////////////////////////////////////////
// Track grouping
///////////////////////////////////////////////////////////////////////////////

#ifdef _SNM_TRACK_GROUP_EX
// Track grouping example code (just did what I needed..)
// Deprecated (since native solo defeat added in REAPER v3.5) but working: see related ifdef'd code 
int addSoloToGroup(MediaTrack * _tr, int _group, bool _master, SNM_ChunkParserPatcher* _cpp)
{
	int updates = 0;
	if (_tr && _cpp && _group > 0 && _group <= 32)
	{
		WDL_String grpLine;
		double grpMask = pow(2.0,(_group-1)*1.0);

		// no track grouping yet ?
		if (!_cpp->Parse(SNM_GET_SUBCHUNK_OR_LINE, 1, "TRACK", "GROUP_FLAGS", -1 , 0, 0, &grpLine))
		{
			int patchPos = _cpp->Parse(SNM_GET_CHUNK_CHAR, 1, "TRACK", "TRACKHEIGHT", -1, 0, 0);
			if (patchPos > 0)
			{
				patchPos--; // see SNM_ChunkParserPatcher..
				WDL_String s;
				s.Append("GROUP_FLAGS 0 0 0 0 0 0");
				s.AppendFormatted(128, _master ? " %d 0 " : " 0 %d ", (int)grpMask);
				s.Append("0 0 0 0 0 0 0 0 0\n");
				_cpp->GetChunk()->Insert(s.Get(), patchPos);				

				// as we're directly working on the cached chunk..
				updates = _cpp->SetUpdates(1);
			}
		}
		// track grouping already exist => patch only what's needed
		else
		{
			// complete the line if needed
			while (grpLine.GetLength() < 64)
			{
				if (grpLine.Get()[grpLine.GetLength()-1] != ' ')
					grpLine.Append(" ");
				grpLine.Append("0 0");
			}

			LineParser lp(false);
			lp.parse(grpLine.Get());
			WDL_String newFlags;
			for (int i=0; i < lp.getnumtokens(); i++)
			{
				if ((i==7 && _master) || (i==8 && !_master))
					newFlags.AppendFormatted(128, "%d", ((int)grpMask) | lp.gettoken_int(i));
				else
					newFlags.Append(lp.gettoken_str(i));
				if (i != (lp.getnumtokens()-1))
					newFlags.Append(" ");
			}
			updates = _cpp->ReplaceLine("TRACK","GROUP_FLAGS", 1, 0, newFlags.Get());
		}
	}
	return updates;
}
#endif


///////////////////////////////////////////////////////////////////////////////
// Track folders
///////////////////////////////////////////////////////////////////////////////

class TrackFolder
{
public:
	TrackFolder(MediaTrack* _tr, int _state) {
		m_tr = _tr;
		m_state = _state;
	}
	MediaTrack* m_tr;
	int m_state;
};

WDL_PtrList_DeleteOnDestroy<TrackFolder> g_trackFolderStates;
WDL_PtrList_DeleteOnDestroy<TrackFolder> g_trackFolderCompactStates;

void saveTracksFolderStates(COMMAND_T* _ct)
{
	const char* strState = !_ct->user ? "I_FOLDERDEPTH" : "I_FOLDERCOMPACT";
	WDL_PtrList_DeleteOnDestroy<TrackFolder>* saveList = !_ct->user ? &g_trackFolderStates : &g_trackFolderCompactStates;
	saveList->Empty(true);
	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i+1,false); // doesn't include master
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			saveList->Add(new TrackFolder(tr, *(int*)GetSetMediaTrackInfo(tr, strState, NULL)));
	}
}

void restoreTracksFolderStates(COMMAND_T* _ct)
{
	bool updated = false;
	const char* strState = !_ct->user ? "I_FOLDERDEPTH" : "I_FOLDERCOMPACT";
	WDL_PtrList_DeleteOnDestroy<TrackFolder>* saveList = !_ct->user ? &g_trackFolderStates : &g_trackFolderCompactStates;
	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i+1,false); // doesn't include master
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			for(int j=0; j < saveList->GetSize(); j++)
			{
				TrackFolder* savedTF = saveList->Get(j);
				int current = *(int*)GetSetMediaTrackInfo(tr, strState, NULL);
				if (savedTF->m_tr == tr && 
					(!_ct->user && savedTF->m_state != current) ||
					(_ct->user && *(int*)GetSetMediaTrackInfo(tr, "I_FOLDERDEPTH", NULL) == 1 && savedTF->m_state != current))
				{
					GetSetMediaTrackInfo(tr, strState, &(savedTF->m_state));
					updated = true;
					break;
				}
			}
		}
	}
	if (updated)
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}

void setTracksFolderState(COMMAND_T* _ct)
{
	bool updated = false;
	for (int i = 0; i < GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i+1,false); // doesn't include master
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL) &&
			_ct->user != *(int*)GetSetMediaTrackInfo(tr, "I_FOLDERDEPTH", NULL))
		{
			GetSetMediaTrackInfo(tr, "I_FOLDERDEPTH", &(_ct->user));
			updated = true;
		}
	}
	if (updated)
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}


///////////////////////////////////////////////////////////////////////////////
// Env. arming
///////////////////////////////////////////////////////////////////////////////

void toggleArmTrackEnv(COMMAND_T* _ct)
{
	bool updated = false;
	for (int i = 0; i <= GetNumTracks(); i++)
	{
		MediaTrack* tr = CSurf_TrackFromID(i,false); 
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			SNM_ArmEnvParserPatcher p(tr);
			switch(_ct->user)
			{
				// ALL envs
				case 0:
					p.SetNewValue(-1);//toggle
					updated = (p.ParsePatch(-1) > 0);
					break;
				case 1:
					p.SetNewValue(1);//arm
					updated = (p.ParsePatch(-1) > 0); 
					break;
				case 2:
					p.SetNewValue(0); //disarn
					updated = (p.ParsePatch(-1) > 0);
					break;
				// track vol/pan/mute envs
				case 3:
					updated = (p.ParsePatch(SNM_TOGGLE_CHUNK_INT, 2, "VOLENV2", "ARM", 2, 0, 1) > 0);
					break;
				case 4:
					updated = (p.ParsePatch(SNM_TOGGLE_CHUNK_INT, 2, "PANENV2", "ARM", 2, 0, 1) > 0);
					break;
				case 5:
					updated = (p.ParsePatch(SNM_TOGGLE_CHUNK_INT, 2, "MUTEENV", "ARM", 2, 0, 1) > 0);
					break;
				// receive envs
				case 6:
					p.SetNewValue(-1); //toggle
					updated = (p.ParsePatch(-2) > 0);
					break;
				case 7:
					p.SetNewValue(-1); //toggle
					updated = (p.ParsePatch(-3) > 0);
					break;
				case 8:
					p.SetNewValue(-1); //toggle
					updated = (p.ParsePatch(-4) > 0);
					break;
				//plugin envs
				case 9:
					p.SetNewValue(-1); //toggle
					updated = (p.ParsePatch(-5) > 0);
					break;

				default:
					break;
			}
		}
	}
	if (updated)
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}


///////////////////////////////////////////////////////////////////////////////
// Track selection (all with ReaProject*)
///////////////////////////////////////////////////////////////////////////////

int CountSelectedTracksWithMaster(ReaProject* _proj)
{
	int selCnt = CountSelectedTracks(_proj);
	MediaTrack* mtr = GetMasterTrack(_proj);
	if (mtr && *(int*)GetSetMediaTrackInfo(mtr, "I_SELECTED", NULL))
		selCnt++;
	return selCnt;
}

// Takes the master track into account
// => to be used with CountSelectedTracksWithMaster() and not the API's CountSelectedTracks()
// If selected, the master will be returnd with the _idx = 0
MediaTrack* GetSelectedTrackWithMaster(ReaProject* _proj, int _idx)
{
	MediaTrack* mtr = GetMasterTrack(_proj);
	if (mtr && *(int*)GetSetMediaTrackInfo(mtr, "I_SELECTED", NULL)) 
	{
		if (!_idx)
			return mtr;
		else
			return GetSelectedTrack(_proj, _idx-1);
	}
	else 
		return GetSelectedTrack(_proj, _idx);
	return NULL;
}

MediaTrack* GetFirstSelectedTrackWithMaster(ReaProject* _proj) {
	return GetSelectedTrackWithMaster(_proj, 0);
}


///////////////////////////////////////////////////////////////////////////////
// Track template slots
///////////////////////////////////////////////////////////////////////////////

/*JFB4!!! TODO:
- patch: preserve items
- undo
- mouse pointer => wait
*/
void loadSetOrAddTrackTemplate(const char* _title, bool _add, int _slot, bool _errMsg)
{
	bool updated = false;
	if (CountSelectedTracksWithMaster(NULL))
	{
		// Prompt for slot if needed
		if (_slot == -1) _slot = g_trTemplateFiles.PromptForSlot(_title); //loops on err
		if (_slot == -1) return; // user has cancelled

		// main job
		if (g_trTemplateFiles.LoadOrBrowseSlot(_slot, _errMsg) && g_trTemplateFiles.Get(_slot)->m_fullPath.GetLength())
		{
			WDL_String chunk;
			// add as new track
			if (_add)
			{
				Main_openProject(g_trTemplateFiles.Get(_slot)->m_fullPath.Get());
				updated = true;
			}
			// patch selected tracks
			else if (LoadChunk(g_trTemplateFiles.Get(_slot)->m_fullPath.Get(), &chunk) && chunk.GetLength())
			{
				for (int i = 0; i <= GetNumTracks(); i++)
				{
					MediaTrack* tr = CSurf_TrackFromID(i,false); 
					if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
					{
						SNM_ChunkParserPatcher p(tr);
						p.SetChunk(&chunk, 1);
						updated |= true;
					}
				}
			}
		}
	}
}

void loadSetTrackTemplate(COMMAND_T* _ct) {
	int slot = (int)_ct->user;
	loadSetOrAddTrackTemplate(SNM_CMD_SHORTNAME(_ct), false, slot, slot < 0 || !g_trTemplateFiles.Get(slot)->IsDefault());
}

void loadAddTrackTemplate(COMMAND_T* _ct) {
	int slot = (int)_ct->user;
	loadSetOrAddTrackTemplate(SNM_CMD_SHORTNAME(_ct), true, slot, slot < 0 || !g_trTemplateFiles.Get(slot)->IsDefault());
}

void clearTrackTemplateSlotPrompt(COMMAND_T* _ct) {
	int slot = g_trTemplateFiles.PromptForSlot(SNM_CMD_SHORTNAME(_ct)); //loops on err
	if (slot == -1) return; // user has cancelled
	else g_trTemplateFiles.ClearSlot(slot);
}