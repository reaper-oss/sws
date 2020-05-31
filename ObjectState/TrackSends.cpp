/******************************************************************************
/ TrackSends.cpp
/
/ Copyright (c) 2010 Tim Payne (SWS)
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

#include "../SnM/SnM_Dlg.h"
#include "TrackSends.h"

#include <WDL/localize/localize.h>

// Functions for getting/setting track sends

// Resolve missing receive tracks
static const char* g_cErrorStr;
static TrackSend* g_ts;
static MediaTrack* g_send;
static MediaTrack* g_recv;
static int g_iResolveRet;
#define RESOLVE_WND_POS "ResolveReceiveWndPoc"

INT_PTR WINAPI doResolve(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (INT_PTR r = SNM_HookThemeColorsMessage(hwndDlg, uMsg, wParam, lParam))
		return r;

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			SetDlgItemText(hwndDlg, IDC_TEXT, g_cErrorStr);
			CheckDlgButton(hwndDlg, IDC_APPLY, BST_CHECKED);
			HWND hTracks = GetDlgItem(hwndDlg, IDC_TRACK);
			WDL_UTF8_HookComboBox(hTracks);
			SendMessage(hTracks, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("(create new track)","sws_DLG_114"));
				
			for (int i = 1; i <= GetNumTracks(); i++)
			{
				MediaTrack* tr = CSurf_TrackFromID(i, false);
				if (tr != g_send)
				{
					char cName[80];
					snprintf(cName, 80, "%d: %s", i, (char*)GetSetMediaTrackInfo(tr, "P_NAME", NULL));
					SendMessage(hTracks, CB_ADDSTRING, 0, (LPARAM)cName);
				}
			}
			SendMessage(hTracks, CB_SETCURSEL, 0, 0);
			RestoreWindowPos(hwndDlg, RESOLVE_WND_POS, false);
			g_iResolveRet = 0;
			return 0;
		}
		case WM_COMMAND:
			wParam = LOWORD(wParam);

			if (wParam == IDC_DELETE)
				g_iResolveRet = 1;
			else if (wParam == IDOK)
			{
				char cTrack[10];
				GetDlgItemText(hwndDlg, IDC_TRACK, cTrack, 10);
				int id = atol(cTrack);
				if (!id) // assume they chose "create new track"
				{
					id = GetNumTracks()+1;
					InsertTrackAtIndex(id, false);
					TrackList_AdjustWindows(false);
				}
				g_recv = CSurf_TrackFromID(id, false);
				g_iResolveRet = 2;
			}
			
			if (wParam == IDC_DELETE || wParam == IDOK || wParam == IDCANCEL)
			{
				SaveWindowPos(hwndDlg, RESOLVE_WND_POS);
				EndDialog(hwndDlg,0);
			}
			return 0;
	}
	return 0;
}

// Return TRUE on delete send
bool ResolveMissingRecv(MediaTrack* tr, int iSend, TrackSend* ts, WDL_PtrList<TrackSendFix>* pFix)
{
	WDL_FastString str;
	char* cName = (char*)GetSetMediaTrackInfo(tr, "P_NAME", NULL);
	if (!cName || !cName[0])
		cName = (char*)__LOCALIZE("(unnamed)","sws_DLG_114");
	str.SetFormatted(200, __LOCALIZE_VERFMT("Send %d on track %d \"%s\" is missing its receive track!","sws_DLG_114"), iSend+1, CSurf_TrackToID(tr, false), cName);

	g_cErrorStr = str.Get();
	g_ts = ts;
	g_send = tr;
	g_recv = NULL;

	DialogBox(g_hInst, MAKEINTRESOURCE(IDD_RECVMISSING), g_hwndParent, doResolve);

	if (g_iResolveRet == 1)
		return true;
	else if (g_iResolveRet == 2)
	{
		GUID* newGuid = (GUID*)GetSetMediaTrackInfo(g_recv, "GUID", NULL);
		if (pFix)
			pFix->Add(new TrackSendFix(ts->GetGuid(), newGuid));
		ts->SetGuid(newGuid);
	}

	return false;
}

TrackSend::TrackSend(GUID* guid, const char* str, const char* auxvol, const char* auxpan, const char* auxmute)
{
	m_destGuid = *guid;
	// Strip out the "AUXRECV X" header
	const char* p = strchr(str + 8, ' ') + 1;
	m_str.Set(p);

	m_auxvolstr.Set(auxvol);
	m_auxpanstr.Set(auxpan);
	m_auxmutestr.Set(auxmute);
}

// For backward compat only, or perhaps for creating new sends?
TrackSend::TrackSend(GUID* guid, int iMode, double dVol, double dPan, int iMute, int iMono, int iPhase, int iSrc, int iDest, int iMidi, int iAuto)
{
	m_destGuid = *guid;
	m_str.SetFormatted(200, "%d %.14f %.14f %d %d %d %d %d -1.0 %d %d",
		iMode, dVol, dPan, iMute, iMono, iPhase, iSrc, iDest, iMidi, iAuto);
}

// Set the AUXRECV string using the second parameter in the string as the destination track GUID
// NF: additionally set the send env's for building snapshot from XML/RPP chunk etc.
TrackSend::TrackSend(const char* str, const char* auxvol, const char* auxpan, const char* auxmute)
{
	LineParser lp(false);
	lp.parse(str);
	stringToGuid(lp.gettoken_str(1), &m_destGuid);
	// Strip out the "AUXRECV X" header
	const char* p = strchr(str + 8, ' ') + 1;
	m_str.Set(p);

	m_auxvolstr.Set(auxvol);
	m_auxpanstr.Set(auxpan);
	m_auxmutestr.Set(auxmute);
}

TrackSend::TrackSend(TrackSend& ts)
{
	m_destGuid = ts.m_destGuid;
	m_str.Set(ts.m_str.Get());
}

WDL_FastString* TrackSend::AuxRecvString(MediaTrack* srcTr, WDL_FastString* str)
{
	int iSrc = CSurf_TrackToID(srcTr, false) - 1;
	str->SetFormatted(20, "AUXRECV %d ", iSrc);
	str->Append(m_str.Get());
	return str;
}

WDL_FastString TrackSend::GetAuxvolstr()
{
	return m_auxvolstr;
}

WDL_FastString TrackSend::GetAuxpanstr()
{
	return m_auxpanstr;
}

WDL_FastString TrackSend::GetAuxmutestr()
{
	return m_auxmutestr;
}

void TrackSend::GetChunk(WDL_FastString* chunk)
{
	char guidStr[64];
	guidToString(&m_destGuid, guidStr);
	chunk->AppendFormatted(chunk->GetLength() + 150, "AUXSEND %s %s\n", guidStr, m_str.Get());

	if (m_auxvolstr.GetLength())
		chunk->Append(m_auxvolstr.Get());
	if (m_auxpanstr.GetLength())
		chunk->Append(m_auxpanstr.Get());
	if (m_auxmutestr.GetLength())
		chunk->Append(m_auxmutestr.Get());
}

TrackSends::TrackSends(TrackSends& ts)
{
	for (int i = 0; i < ts.m_hwSends.GetSize(); i++)
		m_hwSends.Add(new WDL_FastString(ts.m_hwSends.Get(i)->Get()));
	for (int i = 0; i < ts.m_sends.GetSize(); i++)
		m_sends.Add(new TrackSend(*ts.m_sends.Get(i)));
}

TrackSends::~TrackSends()
{
	m_hwSends.Empty(true);
	m_sends.Empty(true);
}

enum class EnvTypes {
	AUXVOL = 0,
	AUXPAN,
	AUXMUTE
};

static void DisplayStoreSendEnvError(MediaTrack* tr, EnvTypes envType, const envelope::bad_get_env_chunk_big& ex)
{
	std::ostringstream ss;
	ss << "Track " << static_cast<int>(GetMediaTrackInfo_Value(tr, "IP_TRACKNUMBER")) << ":\n";

	switch (envType)
	{
	case EnvTypes::AUXVOL:
		ss << __LOCALIZE("Error storing send volume envelope!\n", "sws_mbox");
		break;
	case EnvTypes::AUXPAN:
		ss << __LOCALIZE("Error storing send pan envelope!\n", "sws_mbox");
		break;
	case EnvTypes::AUXMUTE:
		ss << __LOCALIZE("Error storing send mute envelope!\n", "sws_mbox");
		break;
	}
	ss << ex.what(); // localized in envelope.cpp

	MessageBox(g_hwndParent, ss.str().c_str(), __LOCALIZE("SWS Snapshots - Error", "sws_mbox"), MB_OK);
}

void TrackSends::Build(MediaTrack* tr)
{
	// Get the HW sends from the track object string
	const char* trackStr = SWS_GetSetObjectState(tr, NULL);
	char line[4096];
	int pos = 0;
	while (GetChunkLine(trackStr, line, 4096, &pos, false))
	{
		if (strncmp(line, "HWOUT", 5) == 0)
			m_hwSends.Add(new WDL_FastString(line));
	}
	SWS_FreeHeapPtr(trackStr);

	// Get the track sends from the cooresponding track object string
	MediaTrack* pDest;
	int idx = 0;
	char searchStr[21];
	sprintf(searchStr, "AUXRECV %d ", CSurf_TrackToID(tr, false) - 1);
	while ((pDest = (MediaTrack*)GetSetTrackSendInfo(tr, 0, idx++, "P_DESTTRACK", NULL)))
	{
		GUID guid = *(GUID*)GetSetMediaTrackInfo(pDest, "GUID", NULL);
		// Have we already parsed this particular dest track string?
		bool bParsed = false;
		for (int i = 0; i < m_sends.GetSize(); i++)
			if (GuidsEqual(m_sends.Get(i)->GetGuid(), &guid))
			{
				bParsed = true;
				break;
			}
		if (bParsed)
			continue;

		// We haven't parsed yet!
		trackStr = SWS_GetSetObjectState(pDest, NULL);
		pos = 0;

		// count receive occurence for getting correct receive env. chunk
		char searchStrAUXRECV[20] = "AUXRECV";
		int receiveIdx = -1;

		while (GetChunkLine(trackStr, line, 4096, &pos, false))
		{ 
			if (strncmp(line, searchStrAUXRECV, strlen(searchStrAUXRECV)) == 0) // AUXRECV line found...
				receiveIdx += 1;

			if (strncmp(line, searchStr, strlen(searchStr)) == 0) {
	
				// get the AUXVOL, AUXPAN, AUXMUTE subchunks
				// note: envelope points in automations items aren't included in envelope chunks
				// https://forum.cockos.com/showthread.php?t=205279
				std::string AUXVOLstr, AUXPANstr, AUXMUTEstr;

				TrackEnvelope* auxrcvVolEnv = (TrackEnvelope*)GetSetTrackSendInfo(pDest, -1, receiveIdx, "P_ENV", (void*)"<VOLENV");
				TrackEnvelope* auxrcvPanEnv = (TrackEnvelope*)GetSetTrackSendInfo(pDest, -1, receiveIdx, "P_ENV", (void*)"<PANENV");
				TrackEnvelope* auxrcvMuteEnv = (TrackEnvelope*)GetSetTrackSendInfo(pDest, -1, receiveIdx, "P_ENV", (void*)"<MUTEENV");

				if (auxrcvVolEnv) 
				{
					try {
						AUXVOLstr = envelope::GetEnvelopeStateChunkBig(auxrcvVolEnv);
					} 
					catch (const envelope::bad_get_env_chunk_big &ex) {
						
						DisplayStoreSendEnvError(tr, EnvTypes::AUXVOL, ex);
					}
				}

				if (auxrcvPanEnv) 
				{
					try {
						AUXPANstr = envelope::GetEnvelopeStateChunkBig(auxrcvPanEnv);
					}
					catch (const envelope::bad_get_env_chunk_big& ex) {
						DisplayStoreSendEnvError(tr, EnvTypes::AUXPAN, ex);
					}
					
				}

				if (auxrcvMuteEnv) 
				{
					try {
						AUXMUTEstr = envelope::GetEnvelopeStateChunkBig(auxrcvMuteEnv);
					}
					catch (const envelope::bad_get_env_chunk_big& ex) {
						DisplayStoreSendEnvError(tr, EnvTypes::AUXMUTE, ex);
					}
					
				}
				m_sends.Add(new TrackSend(&guid, line, AUXVOLstr.c_str(), AUXPANstr.c_str(), AUXMUTEstr.c_str()));
			}
		}
		SWS_FreeHeapPtr(trackStr);
	}
}

void TrackSends::UpdateReaper(MediaTrack* tr, WDL_PtrList<TrackSendFix>* pFix)
{
	// First replace all the hw sends with the stored
	const char* trackStr = SWS_GetSetObjectState(tr, NULL);
	WDL_FastString newTrackStr;
	char line[4096];
	int pos = 0;
	bool bChanged = false;
	while (GetChunkLine(trackStr, line, 4096, &pos, true))
	{
		if (strncmp(line, "HWOUT", 5) != 0)
			newTrackStr.Append(line);
		else
			bChanged = true;
	}
	for (int i = 0; i < m_hwSends.GetSize(); i++)
	{
		bChanged = true;
		AppendChunkLine(&newTrackStr, m_hwSends.Get(i)->Get());
	}

	SWS_FreeHeapPtr(trackStr);
	if (bChanged)
		SWS_GetSetObjectState(tr, &newTrackStr);

	// Check for destination track validity
	for (int i = 0; i < m_sends.GetSize(); i++)
		if (!GuidToTrack(m_sends.Get(i)->GetGuid()))
		{
			bool bFixed = false;
			if (pFix)
			{
				for (int j = 0; j < pFix->GetSize(); j++)
					if (GuidsEqual(&pFix->Get(j)->m_oldGuid, m_sends.Get(i)->GetGuid()))
					{
						m_sends.Get(i)->SetGuid(&pFix->Get(j)->m_newGuid);
						bFixed = true;
						break;
					}
			}
			if (!bFixed)
			{
				GUID newGuid = GUID_NULL;
				int iRet = ResolveMissingRecv(tr, i, m_sends.Get(i), pFix);
				if (iRet == 1) // Success!
					m_sends.Get(i)->SetGuid(&newGuid);
				else if (iRet == 2) // Delete
				{
					m_sends.Delete(i, true);
					i--;
				}
			}
		}

	// Now, delete any existing sends and add as necessary
	// Loop through each track
	char searchStr[20];
	sprintf(searchStr, "AUXRECV %d", CSurf_TrackToID(tr, false) - 1);
	WDL_FastString sendStr;
	GUID* trGuid = (GUID*)GetSetMediaTrackInfo(tr, "GUID", NULL);
	for (int i = 1; i <= GetNumTracks(); i++)
	{
		MediaTrack* pDest = CSurf_TrackFromID(i, false);
		newTrackStr.Set("");
		trackStr = NULL;
		MediaTrack* pSrc;
		int idx = 0;
		while ((pSrc = (MediaTrack*)GetSetTrackSendInfo(pDest, -1, idx++, "P_SRCTRACK", NULL)))
			if (pSrc == tr)
			{
				// also delete <AUXVOLENV etc. subchunks, because they are now also stored in TrackSend object
				// otherwise they'd appear twice
				TrackEnvelope* auxrcvVolEnv = (TrackEnvelope*)GetSetTrackSendInfo(pDest, -1, idx - 1, "P_ENV", (void*)"<VOLENV");
				TrackEnvelope* auxrcvPanEnv = (TrackEnvelope*)GetSetTrackSendInfo(pDest, -1, idx - 1, "P_ENV", (void*)"<PANENV");
				TrackEnvelope* auxrcvMuteEnv = (TrackEnvelope*)GetSetTrackSendInfo(pDest, -1, idx - 1, "P_ENV", (void*)"<MUTEENV");

				if (auxrcvVolEnv)
					SetEnvelopeStateChunk(auxrcvVolEnv, "<AUXVOLENV\n>", false);
				if (auxrcvPanEnv)
					SetEnvelopeStateChunk(auxrcvPanEnv, "<AUXPANENV\n>", false);
				if (auxrcvMuteEnv)
					SetEnvelopeStateChunk(auxrcvMuteEnv, "<AUXMUTEENV\n>", false);

				trackStr = SWS_GetSetObjectState(pDest, NULL);
				pos = 0;
				// Remove existing recvs from the src track
				while (GetChunkLine(trackStr, line, 4096, &pos, true))
				{
					if (strncmp(line, searchStr, strlen(searchStr)) != 0)
						newTrackStr.Append(line);
				}
				break;
			}

		// append stored sends to the track chunk
		GUID guid = *(GUID*)GetSetMediaTrackInfo(pDest, "GUID", NULL);
		for (int i = 0; i < m_sends.GetSize(); i++)
		{
			if (GuidsEqual(&guid, m_sends.Get(i)->GetGuid()) && !GuidsEqual(&guid, trGuid))
			{
				if (!trackStr)
				{
					trackStr = SWS_GetSetObjectState(pDest, NULL);
					newTrackStr.Set(trackStr);
				}
				AppendChunkLine(&newTrackStr, m_sends.Get(i)->AuxRecvString(tr, &sendStr)->Get()); 

				// also append the stored env's to the track chunk
				AppendChunkLine(&newTrackStr, m_sends.Get(i)->GetAuxvolstr().Get());
				AppendChunkLine(&newTrackStr, m_sends.Get(i)->GetAuxpanstr().Get());
				AppendChunkLine(&newTrackStr, m_sends.Get(i)->GetAuxmutestr().Get());
			}
		}

		if (trackStr)
		{
			SWS_GetSetObjectState(pDest, &newTrackStr);
			SWS_FreeHeapPtr(trackStr);
		}
	}
}

void TrackSends::GetChunk(WDL_FastString* chunk)
{
	for (int i = 0; i < m_hwSends.GetSize(); i++)
	{
		chunk->Append(m_hwSends.Get(i)->Get());
		chunk->Append("\n");
	}
	for (int i = 0; i < m_sends.GetSize(); i++)
		m_sends.Get(i)->GetChunk(chunk);
}
