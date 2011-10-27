/******************************************************************************
/ SnM_Track.cpp
/
/ Copyright (c) 2009-2011 Tim Payne (SWS), Jeffos
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

WDL_FastString g_trackGrpClipboard;

void copyCutTrackGrouping(COMMAND_T* _ct)
{
	int updates = 0;
	bool copyDone = false;
	g_trackGrpClipboard.Set(""); // reset clipboard
	for (int i=1; i <= GetNumTracks(); i++) // skip master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			SNM_ChunkParserPatcher p(tr);
			if (!copyDone)
				copyDone = (p.Parse(SNM_GET_SUBCHUNK_OR_LINE, 1, "TRACK", "GROUP_FLAGS", -1 , 0, 0, &g_trackGrpClipboard, NULL, "MAINSEND") > 0);

			// cut (for all selected tracks)
			if ((int)_ct->user)
				updates += p.RemoveLines("GROUP_FLAGS", true); // brutal removing ok: "GROUP_FLAGS" is not part of freeze data
			// single copy: the 1st found track grouping
			else if (copyDone)
				break;
		}
	}
	if (updates)
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}

void pasteTrackGrouping(COMMAND_T* _ct)
{
	int updates = 0;
	for (int i = 1; i <= GetNumTracks(); i++) // skip master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			SNM_ChunkParserPatcher p(tr);
			updates += p.RemoveLines("GROUP_FLAGS", true); // brutal removing ok: "GROUP_FLAGS" is not part of freeze data
			int patchPos = p.Parse(SNM_GET_CHUNK_CHAR, 1, "TRACK", "TRACKHEIGHT", -1, 0, 0, NULL, NULL, "MAINSEND"); //JFB strstr instead?
			if (patchPos > 0)
			{
				p.GetChunk()->Insert(g_trackGrpClipboard.Get(), --patchPos);				
				p.SetUpdates(1);  // as we're directly working on the cached chunk..
				updates++;
			}
		}
	}
	if (updates)
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}

void removeTrackGrouping(COMMAND_T* _ct)
{
	int updates = 0;
	for (int i=1; i <= GetNumTracks(); i++) // skip master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL)) {
			SNM_ChunkParserPatcher p(tr);
			updates += p.RemoveLines("GROUP_FLAGS", true); // brutal removing ok: "GROUP_FLAGS" is not part of freeze data
		}
	}
	if (updates)
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}

bool SetTrackGroup(int _group)
{
	int updates = 0;
	if (_group >= 0 && _group < SNM_MAX_TRACK_GROUPS)
	{
		double grpMask = pow(2.0, _group*1.0);
		char grpDefault[SNM_MAX_CHUNK_LINE_LENGTH] = "";
		GetPrivateProfileString("REAPER", "tgrpdef", "", grpDefault, SNM_MAX_CHUNK_LINE_LENGTH, get_ini_file());
		WDL_FastString defFlags(grpDefault);
		while (defFlags.GetLength() < 64) { // complete the line (64 is more than needed: future-proof)
			if (defFlags.Get()[defFlags.GetLength()-1] != ' ') defFlags.Append(" ");
			defFlags.Append("0");
		}

		WDL_FastString newFlags("GROUP_FLAGS ");
		LineParser lp(false);
		if (!lp.parse(defFlags.Get())) {
			for (int i=0; i < lp.getnumtokens(); i++) {
				int n = lp.gettoken_int(i);
				newFlags.AppendFormatted(128, "%d", !n ? 0 : (int)grpMask);
				newFlags.Append(i == (lp.getnumtokens()-1) ? "\n" : " ");
			}
		}

		for (int i = 1; i <= GetNumTracks(); i++) // skip master
		{
			MediaTrack* tr = CSurf_TrackFromID(i, false);
			if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			{
				SNM_ChunkParserPatcher p(tr);
				updates += p.RemoveLine("TRACK", "GROUP_FLAGS", 1, 0, "TRACKHEIGHT");
				int pos = p.Parse(SNM_GET_CHUNK_CHAR, 1, "TRACK", "TRACKHEIGHT", -1, 0, 0, NULL, NULL, "INQ");
				if (pos > 0) {
					pos--; // see SNM_ChunkParserPatcher..
					p.GetChunk()->Insert(newFlags.Get(), pos);				
					p.SetUpdates(++updates); // as we're directly working on the cached chunk..
				}
			}
		}
	}
	return (updates > 0);
}

int FindFirstUnusedGroup()
{
	bool grp[SNM_MAX_TRACK_GROUPS];
	for (int i=0; i < SNM_MAX_TRACK_GROUPS; i++) grp[i] = false;

	for (int i = 1; i <= GetNumTracks(); i++) // skip master
	{
		//JFB TODO? exclude selected tracks?
		if (MediaTrack* tr = CSurf_TrackFromID(i, false))
		{
			SNM_ChunkParserPatcher p(tr);
			WDL_FastString grpLine;
			if (p.Parse(SNM_GET_SUBCHUNK_OR_LINE, 1, "TRACK", "GROUP_FLAGS", -1 , 0, 0, &grpLine, NULL, "TRACKHEIGHT"))
			{
				LineParser lp(false);
				if (!lp.parse(grpLine.Get())) {
					for (int j=1; j < lp.getnumtokens(); j++) { // skip 1st token GROUP_FLAGS
						int val = lp.gettoken_int(j);
						for (int k=0; k < SNM_MAX_TRACK_GROUPS; k++) {
							int grpMask = int(pow(2.0, k*1.0));
							grp[k] |= ((val & grpMask) == grpMask);
						}
					}
				}
			}
		}
	}

	for (int i=0; i < SNM_MAX_TRACK_GROUPS; i++) 
		if (!grp[i]) return i;

	return -1;
}

void SetTrackGroup(COMMAND_T* _ct) {
	if (SetTrackGroup((int)_ct->user))
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}

void SetTrackToFirstUnusedGroup(COMMAND_T* _ct) {
	if (SetTrackGroup(FindFirstUnusedGroup()))
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}


///////////////////////////////////////////////////////////////////////////////
// Track folders
///////////////////////////////////////////////////////////////////////////////

WDL_PtrList_DeleteOnDestroy<SNM_TrackInt> g_trackFolderStates;
WDL_PtrList_DeleteOnDestroy<SNM_TrackInt> g_trackFolderCompactStates;

void saveTracksFolderStates(COMMAND_T* _ct)
{
	const char* strState = !_ct->user ? "I_FOLDERDEPTH" : "I_FOLDERCOMPACT";
	WDL_PtrList_DeleteOnDestroy<SNM_TrackInt>* saveList = !_ct->user ? &g_trackFolderStates : &g_trackFolderCompactStates;
	saveList->Empty(true);
	for (int i = 1; i <= GetNumTracks(); i++) // skip master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			saveList->Add(new SNM_TrackInt(tr, *(int*)GetSetMediaTrackInfo(tr, strState, NULL)));
	}
}

void restoreTracksFolderStates(COMMAND_T* _ct)
{
	bool updated = false;
	const char* strState = !_ct->user ? "I_FOLDERDEPTH" : "I_FOLDERCOMPACT";
	WDL_PtrList_DeleteOnDestroy<SNM_TrackInt>* saveList = !_ct->user ? &g_trackFolderStates : &g_trackFolderCompactStates;
	for (int i = 1; i <= GetNumTracks(); i++) // skip master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			for(int j=0; j < saveList->GetSize(); j++)
			{
				SNM_TrackInt* savedTF = saveList->Get(j);
				int current = *(int*)GetSetMediaTrackInfo(tr, strState, NULL);
				if (savedTF->m_tr == tr && 
					(!_ct->user && savedTF->m_int != current) ||
					(_ct->user && *(int*)GetSetMediaTrackInfo(tr, "I_FOLDERDEPTH", NULL) == 1 && savedTF->m_int != current))
				{
					GetSetMediaTrackInfo(tr, strState, &(savedTF->m_int));
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
	for (int i = 1; i <= GetNumTracks(); i++) // skip master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
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

// callers must unallocate the returned value, if any
WDL_PtrList<MediaTrack>* getChildTracks(MediaTrack* _tr)
{
	int depth = (_tr ? *(int*)GetSetMediaTrackInfo(_tr, "I_FOLDERDEPTH", NULL) : 0);
	if (depth == 1)
	{
		WDL_PtrList<MediaTrack>* childTracks = new WDL_PtrList<MediaTrack>;
		int trIdx = CSurf_TrackToID(_tr, false) + 1;
		while (depth > 0 && trIdx <= GetNumTracks())
		{
			MediaTrack* tr = CSurf_TrackFromID(trIdx, false);
			depth += *(int*)GetSetMediaTrackInfo(tr, "I_FOLDERDEPTH", NULL);
			childTracks->Add(tr);
			trIdx++;
		}

		if (childTracks->GetSize())
			return childTracks;
		else
			delete childTracks;
	}
	return NULL;
}


///////////////////////////////////////////////////////////////////////////////
// Track envelopes
///////////////////////////////////////////////////////////////////////////////

const char g_trackEnvelopes[][SNM_MAX_ENV_SUBCHUNK_NAME] = {
	"PARMENV",
	"VOLENV2",
	"PANENV2",
	"WIDTHENV2",
	"VOLENV",
	"PANENV",
	"WIDTHENV",
	"MUTEENV",
	"AUXVOLENV",
	"AUXPANENV",
	"AUXMUTEENV",
/*JFB!!! master playrate env. not managed (safer: I don't really understand the model..)
	"MASTERPLAYSPEEDENV",
*/
	"MASTERWIDTHENV",
	"MASTERWIDTHENV2", // == (SNM_MAX_ENV_SUBCHUNK_NAME + 1) atm !
	"MASTERVOLENV",
	"MASTERPANENV",
	"MASTERVOLENV2",
	"MASTERPANENV2"
};

int trackEnvelopesCount() {
	static int cnt = (int)(sizeof(g_trackEnvelopes) / SNM_MAX_ENV_SUBCHUNK_NAME);
	return cnt;
}

const char* trackEnvelope(int _i) {
	return (_i < 0 || _i >= trackEnvelopesCount() ? "" : g_trackEnvelopes[_i]);
}

bool trackEnvelopesLookup(const char* _str) {
	int sz = trackEnvelopesCount();
	for (int i=0; i < sz; i++)
		if (!strcmp(_str, g_trackEnvelopes[i]))
			return true;
	return false;
}

// note: master playrate env. not managed , see SNM_ArmEnvParserPatcher
void toggleArmTrackEnv(COMMAND_T* _ct)
{
	bool updated = false;
	for (int i = 0; i <= GetNumTracks(); i++) // include master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false); 
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
					p.SetNewValue(0); //disarm
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
			}
		}
	}
	if (updated)
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}


///////////////////////////////////////////////////////////////////////////////
// Toolbar track env. write mode toggle
///////////////////////////////////////////////////////////////////////////////

WDL_PtrList_DeleteOnDestroy<SNM_TrackInt> g_toolbarAutoModeToggles;

void toggleWriteEnvExists(COMMAND_T* _ct)
{
	bool updated = false;
	// Restore write modes
	if (g_toolbarAutoModeToggles.GetSize())
	{
		for (int i = 0; i < g_toolbarAutoModeToggles.GetSize(); i++)
		{
			SNM_TrackInt* tri = g_toolbarAutoModeToggles.Get(i);
			*(int*)GetSetMediaTrackInfo(tri->m_tr, "I_AUTOMODE", &(tri->m_int));
			updated = true;
		}
		g_toolbarAutoModeToggles.Empty(true);
	}
	// Set read mode and store previous write modes
	else
	{
		for (int i = 0; i <= GetNumTracks(); i++) // include master
		{
			MediaTrack* tr = CSurf_TrackFromID(i, false); 
			int autoMode = tr ? *(int*)GetSetMediaTrackInfo(tr, "I_AUTOMODE", NULL) : -1;
			if (autoMode >= 2 /* touch */ && autoMode <= 4 /* latch */)
			{
				*(int*)GetSetMediaTrackInfo(tr, "I_AUTOMODE", &g_i1); // set read mode
				g_toolbarAutoModeToggles.Add(new SNM_TrackInt(tr, autoMode));
				updated = true;
			}
		}
	}

	if (updated)
	{
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);

		// in case auto refresh toolbar bar option is off..
		RefreshToolbar(NamedCommandLookup("_S&M_TOOLBAR_WRITE_ENV"));
	}
}

bool writeEnvExists(COMMAND_T* _ct)
{
	for (int i = 0; i <= GetNumTracks(); i++) // include master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false); 
		int autoMode = tr ? *(int*)GetSetMediaTrackInfo(tr, "I_AUTOMODE", NULL) : -1;
		if (autoMode >= 2 /* touch */ && autoMode <= 4 /* latch */)
			return true;
	}
	return false;
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
		if (!_idx) return mtr;
		else return GetSelectedTrack(_proj, _idx-1);
	}
	else 
		return GetSelectedTrack(_proj, _idx);
	return NULL;
}

MediaTrack* GetFirstSelectedTrackWithMaster(ReaProject* _proj) {
	return GetSelectedTrackWithMaster(_proj, 0);
}


///////////////////////////////////////////////////////////////////////////////
// Track templates & track template slots (of the Resources view)
///////////////////////////////////////////////////////////////////////////////

bool makeSingleTrackTemplateChunk(WDL_FastString* _inRawChunk, WDL_FastString* _out, bool _delItems, bool _delEnvs)
{
	if (_inRawChunk && _out)
	{
		// truncate to 1st track found in the template
		SNM_ChunkParserPatcher p(_inRawChunk);
		if (p.GetSubChunk("TRACK", 1, 0, _out) >= 0)
		{
			// remove receives from the template (as we patch a single track)
			// note: occurs with multiple tracks in a template file (w/ rcv 
			//   between those tracks), we remove them because -if applied- 
			//   track ids of the template won't match the project ones
			SNM_EnvRemover p2(_out);
			p2.RemoveLine("TRACK", "AUXRECV", 1, -1, "MIDIOUT"); // check depth & parent (skip frozen AUXRECV)

			// remove items from template, if any
//JFB!!! a faire partout
			if (_delItems)
				p2.RemoveSubChunk("ITEM", 2, -1);

			// remove all envs in one go
			if (_delEnvs)
				p2.RemoveEnvelopes();
			return true;
		}
	}
	return false;
}

// apply a track template (primitive, no undo point)
bool applyTrackTemplate(MediaTrack* _tr, WDL_FastString* _tmpltChunk, bool _rawChunk, SNM_ChunkParserPatcher* _p, bool _itemsFromTmplt, bool _envsFromTmplt)
{
	bool updated = false;
	if (_tr && _tmpltChunk)
	{
		WDL_FastString newChunk;
		SNM_ChunkParserPatcher* p = (_p ? _p : new SNM_ChunkParserPatcher(_tr));

		// safety: force item removal for master track
		if (_itemsFromTmplt && GetMasterTrack(NULL) == _tr) {
			_itemsFromTmplt = false;
			_rawChunk = true;
		}

		if (_rawChunk)
			makeSingleTrackTemplateChunk(_tmpltChunk, &newChunk, !_itemsFromTmplt, !_envsFromTmplt);
		else
			newChunk.Set(_tmpltChunk);

		// add track's items, if any
		if (!_itemsFromTmplt && GetMasterTrack(NULL) != _tr)
		{
			int posItems = p->GetSubChunk("ITEM", 2, 0); //JFB!!! get all in one work !?
			if (posItems >= 0)
				newChunk.Insert((char*)(p->GetChunk()->Get()+posItems), newChunk.GetLength()-2, p->GetChunk()->GetLength()-posItems- 2); // -2: ">\n"
		}

		p->SetChunk(&newChunk, 1);
		if (!_p) delete p; // + auto-commit
		updated = true;
	}
	return updated;
}

void applyOrImportTrackSlot(const char* _title, bool _import, int _slot, bool _itemsFromTmplt, bool _envsFromTmplt, bool _errMsg)
{
	bool updated = false;

	// Prompt for slot if needed
	if (_slot == -1) _slot = g_trTemplateFiles.PromptForSlot(_title); //loops on err
	if (_slot == -1) return; // user has cancelled

	char fn[BUFFER_SIZE]="";
	if (g_trTemplateFiles.GetOrBrowseSlot(_slot, fn, BUFFER_SIZE, _errMsg)) 
	{
		WDL_FastString tmp;

		// add as new track
		if (_import)
		{
			Main_openProject(fn);
/* commented: Main_openProject() includes undo point 
			updated = true;
*/
		}
		// patch selected tracks with 1st track found in template
		else if (CountSelectedTracksWithMaster(NULL) && LoadChunk(fn, &tmp) && tmp.GetLength())
		{
			WDL_FastString tmpltChunk;
			makeSingleTrackTemplateChunk(&tmp, &tmpltChunk, !_itemsFromTmplt, !_envsFromTmplt);
			for (int i = 0; i <= GetNumTracks(); i++) // include master
			{
				MediaTrack* tr = CSurf_TrackFromID(i, false);
				if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
					updated |= applyTrackTemplate(tr, &tmpltChunk, false, NULL, _itemsFromTmplt); //manages master track specific case..
			}
		}
	}
	if (updated && _title)
		Undo_OnStateChangeEx(_title, UNDO_STATE_ALL, -1);
}

void loadSetTrackTemplate(COMMAND_T* _ct) {
	int slot = (int)_ct->user;
	if (slot < 0 || slot < g_trTemplateFiles.GetSize())
		applyOrImportTrackSlot(SNM_CMD_SHORTNAME(_ct), false, slot, false, false, slot < 0 || !g_trTemplateFiles.Get(slot)->IsDefault());
}

void loadImportTrackTemplate(COMMAND_T* _ct) {
	int slot = (int)_ct->user;
	if (slot < 0 || slot < g_trTemplateFiles.GetSize())
		applyOrImportTrackSlot(SNM_CMD_SHORTNAME(_ct), true, slot, false, false, slot < 0 || !g_trTemplateFiles.Get(slot)->IsDefault());
}

void replaceOrPasteItemsFromTrackSlot(const char* _title, bool _paste, int _slot, bool _errMsg)
{
	bool updated = false;

	// prompt for slot if needed
	if (_slot == -1) _slot = g_trTemplateFiles.PromptForSlot(_title); //loops on err
	if (_slot == -1) return; // user has cancelled

	char fn[BUFFER_SIZE]="";
	if (g_trTemplateFiles.GetOrBrowseSlot(_slot, fn, BUFFER_SIZE, _errMsg)) 
	{
		WDL_FastString tmpltChunk;
		if (CountSelectedTracks(NULL) && LoadChunk(fn, &tmpltChunk) && tmpltChunk.GetLength())
		{
			WDL_FastString tmpltItemsChunk;
			{
				SNM_ChunkParserPatcher p(&tmpltChunk);
				//JFB!!! all items in one go???
				int posItemsInTmplt = p.GetSubChunk("ITEM", 2, 0); // no breakKeyword possible here: chunk ends with items
				if (posItemsInTmplt >= 0)
					tmpltItemsChunk.Set((const char*)(p.GetChunk()->Get()+posItemsInTmplt), p.GetChunk()->GetLength()-posItemsInTmplt-2);  // -2: ">\n"
			}

			for (int i = 1; i <= GetNumTracks(); i++) // skip master
			{
				MediaTrack* tr = CSurf_TrackFromID(i, false);
				if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
				{
					SNM_ChunkParserPatcher p(tr);

					if (!_paste) // delete items?
					{
						int posItemsInTr = p.GetSubChunk("ITEM", 2, 0); // no breakKeyword possible here: chunk ends with items
						if (posItemsInTr >= 0) {
							p.GetChunk()->DeleteSub(posItemsInTr, p.GetChunk()->GetLength()-posItemsInTr-2);  // -2: ">\n"
							updated |= (p.SetUpdates(1) > 0); // as we directly work on the chunk
						}
					}

					if (tmpltItemsChunk.GetLength())
					{
						p.GetChunk()->Insert(tmpltItemsChunk.Get(), p.GetChunk()->GetLength()-2); // -2: before ">\n"
						updated |= (p.SetUpdates(p.GetUpdates() + 1) > 0); // as we directly work on the chunk
					}
				}
			}
		}
	}
	if (updated && _title)
		Undo_OnStateChangeEx(_title, UNDO_STATE_ALL, -1);
}

void appendTrackChunk(MediaTrack* _tr, WDL_FastString* _chunk, bool _delItems, bool _delEnvs)
{
	if (_tr && _chunk)
	{
		//JFB!!! double get => timing: bof!
		SNM_ChunkParserPatcher p(_tr);
		SNM_EnvRemover p2(p.GetChunk(), false);
		if (_delEnvs) p2.RemoveEnvelopes();
		if (_delItems) p2.RemoveSubChunk("ITEM", 2, -1);
		_chunk->Append(p2.GetChunk());
	}
}

void appendSelTrackTemplates(bool _delItems, bool _delEnvs, WDL_FastString* _chunk)
{
	if (!_chunk)
		return;

	WDL_PtrList<MediaTrack> tracks;

	// append selected track chunks (+ folders) -------------------------------
	for (int i = 0; i <= GetNumTracks(); i++) // include master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			appendTrackChunk(tr, _chunk, _delItems, _delEnvs);
			tracks.Add(tr);

			// folder: save child templates
			WDL_PtrList<MediaTrack>* childTracks = getChildTracks(tr);
			if (childTracks)
			{
				for (int j=0; j < childTracks->GetSize(); j++) {
					appendTrackChunk(childTracks->Get(j), _chunk, _delItems, _delEnvs);
					tracks.Add(childTracks->Get(j));
				}
				i += childTracks->GetSize(); // skip children
				delete childTracks;
			}
		}
	}

	// update receives ids ----------------------------------------------------
	// note: no breakKeywords used here, multiple tracks in the template chunk!
	SNM_ChunkParserPatcher p(_chunk);
	WDL_FastString line;
	int occurence = 0;
	int pos = p.Parse(SNM_GET_SUBCHUNK_OR_LINE, 1, "TRACK", "AUXRECV", -1, occurence, 1, &line); 
	while (pos > 0)
	{
		pos--; // see SNM_ChunkParserPatcher

		bool replaced = false;
		line.SetLen(line.GetLength()-1); // remove trailing '\n'
		LineParser lp(false);
		if (!lp.parse(line.Get()) && lp.getnumtokens() > 1)
		{
			int success, curId = lp.gettoken_int(1, &success);
			if (success)
			{
				MediaTrack* tr = CSurf_TrackFromID(curId+1, false);
				int newId = tracks.Find(tr);
				if (newId >= 0)
				{
					const char* p3rdTokenToEol = strchr(line.Get(), ' ');
					if (p3rdTokenToEol) p3rdTokenToEol = strchr((char*)(p3rdTokenToEol+1), ' ');
					if (p3rdTokenToEol)
					{
						WDL_FastString newRcv;
						newRcv.SetFormatted(SNM_MAX_CHUNK_LINE_LENGTH, "AUXRECV %d%s\n", newId, p3rdTokenToEol);
						replaced = p.ReplaceLine(pos, newRcv.Get());
						if (replaced)
							occurence++;
					}
				}
			}
		}

		if (!replaced)
			replaced = p.ReplaceLine(pos, "");
		if (!replaced) // skip, just in case..
			occurence++;

		line.Set("");
		pos = p.Parse(SNM_GET_SUBCHUNK_OR_LINE, 1, "TRACK", "AUXRECV", -1, occurence, 1, &line);
	}
}

bool autoSaveTrackSlots(bool _delItems, bool _delEnvs, const char* _dirPath, char* _fn, int _fnSize)
{
	WDL_FastString fullChunk;
	appendSelTrackTemplates(_delItems, _delEnvs, &fullChunk);

	char* trName = NULL;
	for (int i = 0; i <= GetNumTracks(); i++) // include master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL)) {
			trName = (char*)GetSetMediaTrackInfo(tr, "P_NAME", NULL);
			break;
		}
	}
	GenerateFilename(_dirPath, !trName ? "Master" : (*trName == '\0' ? "Untitled" : trName), g_trTemplateFiles.GetFileExt(), _fn, _fnSize);
	return (SaveChunk(_fn, &fullChunk, true) && g_trTemplateFiles.AddSlot(_fn));
}


///////////////////////////////////////////////////////////////////////////////
// Track input actions
///////////////////////////////////////////////////////////////////////////////

void setMIDIInputChannel(COMMAND_T* _ct)
{
	bool updated = false;
	int ch = (int)_ct->user; // 0: all channels
	for (int i = 1; i <= GetNumTracks(); i++) // skip master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			int in = *(int*)GetSetMediaTrackInfo(tr, "I_RECINPUT", NULL);
			if (((in & 0x1000) == 0x1000) && ((in & 0x1F) != ch))
			{
				in &= 0x1FE0; 
				in |= ch;
				GetSetMediaTrackInfo(tr, "I_RECINPUT", &in);
				updated = true;
			}
		}
	}
	if (updated)
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}

void remapMIDIInputChannel(COMMAND_T* _ct)
{
	bool updated = false;
	int ch = (int)_ct->user; // 0: source channel

	char pLine[SNM_MAX_CHUNK_LINE_LENGTH] = "";
	if (ch)	
		_snprintf(pLine, SNM_MAX_CHUNK_LINE_LENGTH, "MIDI_INPUT_CHANMAP %d\n", ch-1);	

	for (int i = 1; i <= GetNumTracks(); i++) // skip master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			int in = *(int*)GetSetMediaTrackInfo(tr, "I_RECINPUT", NULL);
			if ((in & 0x1000) == 0x1000) // midi in?
			{
				SNM_ChunkParserPatcher p(tr);
				char currentCh[3] = "";
				int chunkPos = p.Parse(SNM_GET_CHUNK_CHAR, 1, "TRACK", "MIDI_INPUT_CHANMAP", 2, 0, 1, currentCh, NULL, "TRACKID");
				if (chunkPos > 0) {
					if (!ch || atoi(currentCh) != (ch-1))
						updated |= p.ReplaceLine(--chunkPos, pLine); // pLine may be "", i.e. remove line
				}
				else 
					updated |= p.InsertAfterBefore(0, pLine, "TRACK", "TRACKHEIGHT", 1, 0, "TRACKID");
			}
		}
	}
	if (updated)
		Undo_OnStateChangeEx(SNM_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}
