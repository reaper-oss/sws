/******************************************************************************
/ SnM_FXChain.cpp
/
/ Copyright (c) 2009 and later Jeffos
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

#include "SnM.h"
#include "SnM_Chunk.h"
#include "SnM_Dlg.h"
#include "SnM_FX.h"
#include "SnM_FXChain.h"
#include "SnM_Item.h"
#include "SnM_Resources.h"
#include "SnM_Track.h"
#include "SnM_Util.h"

#include <WDL/localize/localize.h>

WDL_FastString g_fXChainClipboard;

///////////////////////////////////////////////////////////////////////////////
// Take FX chains
///////////////////////////////////////////////////////////////////////////////

void MakeChunkTakeFX(WDL_FastString* _outTakeFX, const WDL_FastString* _inRfxChain)
{
	if (_outTakeFX && _inRfxChain)
	{
		_outTakeFX->Append("<TAKEFX\nWNDRECT 0 0 0 0\nSHOW 0\nLASTSEL 1\nDOCKED 0\n");
		_outTakeFX->Append(_inRfxChain);
		_outTakeFX->Append(">\n");
	}
}

int CopyTakeFXChain(WDL_FastString* _fxChain, int _startSelItem)
{
	WDL_PtrList<MediaItem> items;
	SNM_GetSelectedItems(NULL, &items);
	for (int i = _startSelItem; _fxChain && i < items.GetSize(); i++)
	{
		if (MediaItem* item = items.Get(i))
		{
			SNM_FXChainTakePatcher p(item);
			if (WDL_FastString* fxChain = p.GetFXChain()) 
			{
				WDL_PtrList<const char> removedKeywords;
				removedKeywords.Add("WNDRECT");
				removedKeywords.Add("SHOW");
				removedKeywords.Add("FLOATPOS");
				removedKeywords.Add("LASTSEL");
				removedKeywords.Add("DOCKED");
				RemoveChunkLines(fxChain, &removedKeywords, true);

				// NF: fix #938
				char TAKEFX_NCH_state[8] = "0";
				if (p.Parse(SNM_GET_CHUNK_CHAR, 1, "ITEM", "TAKEFX_NCH", _startSelItem, 1, TAKEFX_NCH_state, NULL, NULL)) {
					if (atoi(TAKEFX_NCH_state) > 0) {
						fxChain->Append(">\nTAKEFX_NCH ");
						fxChain->Append(TAKEFX_NCH_state);
						fxChain->Append("\n");
					}
				}

				// check fx chain consistency 
				// note: testing the length is not enough since RemoveChunkLines() does not delete but blanks lines
				if (FindKeyword(fxChain->Get())) {
					_fxChain->Set(fxChain);
					return i;
				}
			}
		}
	}
	return -1;
}

void PasteTakeFXChain(const char* _title, WDL_FastString* _chain, bool _activeOnly)
{
	bool updated = false;
	if (_chain && _chain->GetLength())
	{
		for (int i = 1; i <= GetNumTracks(); i++) // skip master
		{
			MediaTrack* tr = CSurf_TrackFromID(i, false);
			for (int j = 0; tr && j < GetTrackNumMediaItems(tr); j++)
			{
				MediaItem* item = GetTrackMediaItem(tr,j);
				if (item && *(bool*)GetSetMediaItemInfo(item,"B_UISEL",NULL))
				{
					SNM_TakeParserPatcher p(item, CountTakes(item));
					int tkIdx = (_activeOnly ? *(int*)GetSetMediaItemInfo(item, "I_CURTAKE", NULL) : 0);
					bool done = false;
					while (!done && tkIdx >= 0)
					{
						WDL_FastString takeChunk;
						int tkPos, tklen;
						if (p.GetTakeChunk(tkIdx, &takeChunk, &tkPos, &tklen)) 
						{
							SNM_ChunkParserPatcher ptk(&takeChunk, false);

							// fx chain exists for this take?
							// note: eolTakeFx is '\n' position + 1
							int eolTkFx = ptk.Parse(SNM_GET_SUBCHUNK_OR_LINE_EOL, 1, "TAKEFX", "<TAKEFX", 0);

							// paste fx chain (just before the end of the current TAKEFX)
							if (eolTkFx > 0) {
								ptk.GetChunk()->Insert(_chain->Get(), eolTkFx-2); //-2: before ">\n"
							}
							// set/create fx chain (after SOURCE)
							else 
							{
								int eolSrc = ptk.Parse(SNM_GET_SUBCHUNK_OR_LINE_EOL, 1, "SOURCE", "<SOURCE", 0);
								if (eolSrc > 0) {
									WDL_FastString newTakeFx;
									MakeChunkTakeFX(&newTakeFx, _chain);
									ptk.GetChunk()->Insert(newTakeFx.Get(), eolSrc);
								}
							}
							updated = p.ReplaceTake(tkPos, tklen, ptk.GetChunk());
						}
						else done = true;

						if (_activeOnly) done = true;
						else tkIdx++;
					}
				}
			}
		}
	}
	if (updated)
		Undo_OnStateChangeEx2(NULL, _title, UNDO_STATE_ALL, -1);
}

// _chain: NULL clears the FX chain
void SetTakeFXChain(const char* _title, WDL_FastString* _chain, bool _activeOnly)
{
	bool updated = false;
	for (int i = 1; i <= GetNumTracks(); i++) // skip master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		for (int j = 0; tr && j < GetTrackNumMediaItems(tr); j++)
		{
			MediaItem* item = GetTrackMediaItem(tr,j);
			if (item && *(bool*)GetSetMediaItemInfo(item,"B_UISEL",NULL))
			{
				SNM_FXChainTakePatcher p(item);
				updated |= p.SetFXChain(_chain, _activeOnly);
			}
		}
	}
	if (updated)
		Undo_OnStateChangeEx2(NULL, _title, UNDO_STATE_ALL, -1);
}


///////////////////////////////////////////////////////////////////////////////
// Track FX chains
///////////////////////////////////////////////////////////////////////////////

// best effort for http://github.com/reaper-oss/sws/issues/363
// update the track's nb of channels if that info exists (as a comment) in the provided chunk
// return true if update done
bool SetTrackChannelsForFXChain(MediaTrack* _tr, WDL_FastString* _chain)
{
	if (_tr && _chain && !strncmp(_chain->Get(), "#NCHAN ", 7))
	{
		int nbChInChain = atoi((const char*)(_chain->Get()+7));
		if (nbChInChain && !(nbChInChain % 2)) // valid and even?
		{
			int nbChInTr = *(int*)GetSetMediaTrackInfo(_tr, "I_NCHAN", NULL);
			if (nbChInChain > nbChInTr)
			{
				GetSetMediaTrackInfo(_tr, "I_NCHAN", &nbChInChain);
				return true;
			}
		}
	}
	return false;
}

void PasteTrackFXChain(const char* _title, WDL_FastString* _chain, bool _inputFX)
{
	bool updated = false;
	if (_chain && _chain->GetLength())
	{
		for (int i=0; i <= GetNumTracks(); i++) // incl. master
		{
			MediaTrack* tr = CSurf_TrackFromID(i, false);
			if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
			{
				// (try to) set track channels
				updated |= SetTrackChannelsForFXChain(tr, _chain);

				// the meat
				bool patch = false;
				SNM_FXChainTrackPatcher p(tr);
				WDL_FastString currentFXChain;
				int pos = p.GetSubChunk(_inputFX ? "FXCHAIN_REC" : "FXCHAIN", 2, 0, &currentFXChain, "<ITEM");

				// paste (well.. insert at the end of the current FX chain)
				if (pos >= 0) 
				{
					patch = true;
					p.GetChunk()->Insert(_chain->Get(), pos + currentFXChain.GetLength() - 2); // -2: before ">\n"
					p.IncUpdates();
				}
				// create fx chain
				else 
					patch = p.SetFXChain(_chain, _inputFX);

				updated |= patch;
			}
		}
	}
	if (updated)
		Undo_OnStateChangeEx2(NULL, _title, UNDO_STATE_ALL, -1);
}

// _chain: NULL to clear
void SetTrackFXChain(const char* _title, WDL_FastString* _chain, bool _inputFX)
{
	bool updated = false;
	for (int i=0; i <= GetNumTracks(); i++) // incl. master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			// (try to) set track channels
			updated |= SetTrackChannelsForFXChain(tr, _chain);

			// the meat
			SNM_FXChainTrackPatcher p(tr);
			bool patch = p.SetFXChain(_chain, _inputFX);
			updated |= patch;
		}
	}
	if (updated)
		Undo_OnStateChangeEx2(NULL, _title, UNDO_STATE_ALL, -1);
}

// returns the first copied track idx (1-based, 0 = master)
int CopyTrackFXChain(WDL_FastString* _fxChain, bool _inputFX, int _startTr)
{
	for (int i=_startTr; i >= 0 && i <= GetNumTracks(); i++) // incl. master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			SNM_ChunkParserPatcher p(tr);
			WDL_FastString fxChain;
			if (p.GetSubChunk(_inputFX ? "FXCHAIN_REC" : "FXCHAIN", 2, 0, &fxChain, "<ITEM") >= 0)
			{
				// remove all fx param envelopes
				{
					SNM_ChunkParserPatcher p2(&fxChain);
					p2.RemoveSubChunk("PARMENV", 2, -1);
				}

				// brutal removal via RemoveChunkLines() is ok here
				WDL_PtrList<const char> removedKeywords;
				removedKeywords.Add(_inputFX ? "<FXCHAIN_REC" : "<FXCHAIN");
				removedKeywords.Add("WNDRECT");
				removedKeywords.Add("SHOW");
				removedKeywords.Add("FLOATPOS");
				removedKeywords.Add("LASTSEL");
				removedKeywords.Add("DOCKED");
				RemoveChunkLines(&fxChain, &removedKeywords, true);
				fxChain.DeleteSub(fxChain.GetLength()-2, 2); // remove last ">\n" 

				// check fx chain consistency 
				// note: testing the length is not enough since RemoveChunkLines() does not delete but blanks lines
				if (FindKeyword(fxChain.Get())) {
					_fxChain->Set(&fxChain);
					return i;
				}
			}
		}
	}
	return -1;
}


///////////////////////////////////////////////////////////////////////////////
// Cut, copy, paste, clear, etc..
///////////////////////////////////////////////////////////////////////////////

// *** Track FX chains ***

void ClearTrackFXChain(COMMAND_T* _ct) {
	SetTrackFXChain(SWS_CMD_SHORTNAME(_ct), NULL, false);
}

void CopyTrackFXChain(COMMAND_T* _ct) {
	CopyTrackFXChain(&g_fXChainClipboard, false);
}

void CutTrackFXChain(COMMAND_T* _ct) {
	CopyTrackFXChain(&g_fXChainClipboard, false);
	SetTrackFXChain(SWS_CMD_SHORTNAME(_ct), NULL, false);
}

void PasteTrackFXChain(COMMAND_T* _ct) {
	PasteTrackFXChain(SWS_CMD_SHORTNAME(_ct), &g_fXChainClipboard, false);
}

// paste (replace)
void SetTrackFXChain(COMMAND_T* _ct) {
	SetTrackFXChain(SWS_CMD_SHORTNAME(_ct), &g_fXChainClipboard, false);
}


// *** Track Input FX chains ***

void ClearTrackInputFXChain(COMMAND_T* _ct) {
	SetTrackFXChain(SWS_CMD_SHORTNAME(_ct), NULL, true);
}

void CopyTrackInputFXChain(COMMAND_T* _ct) {
	CopyTrackFXChain(&g_fXChainClipboard, true);
}

void CutTrackInputFXChain(COMMAND_T* _ct) {
	CopyTrackFXChain(&g_fXChainClipboard, true);
	SetTrackFXChain(SWS_CMD_SHORTNAME(_ct), NULL, true);
}

void PasteTrackInputFXChain(COMMAND_T* _ct) {
	PasteTrackFXChain(SWS_CMD_SHORTNAME(_ct), &g_fXChainClipboard, true);
}

// paste (replace)
void SetTrackInputFXChain(COMMAND_T* _ct) {
	SetTrackFXChain(SWS_CMD_SHORTNAME(_ct), &g_fXChainClipboard, true);
}


// *** Take FX Chains ***

void CopyTakeFXChain(COMMAND_T* _ct) {
	CopyTakeFXChain(&g_fXChainClipboard);
}

void CutTakeFXChain(COMMAND_T* _ct) {
	CopyTakeFXChain(_ct);
	SetTakeFXChain(SWS_CMD_SHORTNAME(_ct), NULL, true);
}

void PasteTakeFXChain(COMMAND_T* _ct) {
	PasteTakeFXChain(SWS_CMD_SHORTNAME(_ct), &g_fXChainClipboard, true);
}

void SetTakeFXChain(COMMAND_T* _ct) {
	SetTakeFXChain(SWS_CMD_SHORTNAME(_ct), &g_fXChainClipboard, true);
}

void PasteAllTakesFXChain(COMMAND_T* _ct) {
	PasteTakeFXChain(SWS_CMD_SHORTNAME(_ct), &g_fXChainClipboard, false);
}

void SetAllTakesFXChain(COMMAND_T* _ct) {
	SetTakeFXChain(SWS_CMD_SHORTNAME(_ct), &g_fXChainClipboard, false);
}

void ClearActiveTakeFXChain(COMMAND_T* _ct) {
	SetTakeFXChain(SWS_CMD_SHORTNAME(_ct), NULL, true);
}

void ClearAllTakesFXChain(COMMAND_T* _ct) {
	SetTakeFXChain(SWS_CMD_SHORTNAME(_ct), NULL, false);
}


// *** Common to take & track FX chains ***

void SmartCopyFXChain(COMMAND_T* _ct) {
	if (GetCursorContext() == 1 && CountSelectedMediaItems(NULL)) CopyTakeFXChain(&g_fXChainClipboard);
	else CopyTrackFXChain(&g_fXChainClipboard, false);
}

void SmartPasteFXChain(COMMAND_T* _ct) {
	if (GetCursorContext() == 1 && CountSelectedMediaItems(NULL)) PasteTakeFXChain(SWS_CMD_SHORTNAME(_ct), &g_fXChainClipboard, true);
	else PasteTrackFXChain(SWS_CMD_SHORTNAME(_ct), &g_fXChainClipboard, false);
}

void SmartPasteReplaceFXChain(COMMAND_T* _ct) {
	if (GetCursorContext() == 1 && CountSelectedMediaItems(NULL)) SetTakeFXChain(SWS_CMD_SHORTNAME(_ct), &g_fXChainClipboard, true);
	else SetTrackFXChain(SWS_CMD_SHORTNAME(_ct), &g_fXChainClipboard, false);
}

void SmartCutFXChain(COMMAND_T* _ct) {
	if (GetCursorContext() == 1 && CountSelectedMediaItems(NULL)) {
		CopyTakeFXChain(_ct);
		SetTakeFXChain(SWS_CMD_SHORTNAME(_ct), NULL, true);
	}
	else {
		CopyTrackFXChain(&g_fXChainClipboard, false);
		SetTrackFXChain(SWS_CMD_SHORTNAME(_ct), NULL, false);
	}
}


///////////////////////////////////////////////////////////////////////////////
// Other
///////////////////////////////////////////////////////////////////////////////

void ReassignLearntMIDICh(COMMAND_T* _ct)
{
	bool updated = false;
	int ch = (int)_ct->user;
	int prm = ch; // -1: all fx, -2: sel fx, -3: all fx to input channel

	// Prompt for channel if needed
	switch(prm) {
		case -1:
		case -2:
			ch = PromptForInteger(SWS_CMD_SHORTNAME(_ct), __LOCALIZE("MIDI channel","sws_mbox"), 1, 16); // loops on err
			if (ch == -1) return; // user has cancelled
			break;
	}

	for (int i=0; i <= GetNumTracks(); i++) // incl. master
	{
		MediaTrack* tr = CSurf_TrackFromID(i, false);
		if (tr && *(int*)GetSetMediaTrackInfo(tr, "I_SELECTED", NULL))
		{
			SNM_LearnMIDIChPatcher p(tr);
			switch(prm)
			{
				case -2: {
					int fx = GetSelectedTrackFX(tr);
					if (fx > 0)
						updated |= p.SetChannel(ch, fx);
					break;
				}
				case -3: {
					int in = *(int*)GetSetMediaTrackInfo(tr, "I_RECINPUT", NULL);
					if ((in & 0x1000) == 0x1000) {
						in &= 0x1F;
						if (in > 0)
							updated |= p.SetChannel(in-1, -1);
					}
					break;
				}
				default:
					updated |= p.SetChannel(ch, -1);
					break;
			}
		}
	}
	if (updated)
		Undo_OnStateChangeEx2(NULL, SWS_CMD_SHORTNAME(_ct), UNDO_STATE_ALL, -1);
}
