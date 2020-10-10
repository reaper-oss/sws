/******************************************************************************
/ SnM_CSurf.cpp
/
/ Copyright (c) 2013 and later Jeffos
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
#include "SnM_CSurf.h"
#include "SnM_LiveConfigs.h"
#include "SnM_Misc.h"
#include "SnM_Notes.h"
#include "SnM_RegionPlaylist.h"
#include "SnM_Resources.h"
#include "SnM_Track.h"
#include "SnM_Util.h"
#include "../OscPkt/oscpkt.h"
#include "../OscPkt/udp.h"

#include <WDL/localize/localize.h>

///////////////////////////////////////////////////////////////////////////////
// SWSTimeSlice:IReaperControlSurface callbacks
///////////////////////////////////////////////////////////////////////////////

// called from the main thread via IReaperControlSurface::Run()
// processing order is important here
void SNM_CSurfRun()
{
	static bool sRecurseCheck = false;
	if (sRecurseCheck)
		return;

	sRecurseCheck = true;

	PlaylistRun();
	ScheduledJob::Run();
	StopTrackPreviewsRun();
	UpdateMarkerRegionRun();
	AutoRefreshToolbarRun();

	sRecurseCheck = false;
}

void SNM_CSurfSetTrackTitle() {
	NotesSetTrackTitle();
	LiveConfigsSetTrackTitle();
}

void SNM_CSurfSetTrackListChange()
{
	NotesSetTrackListChange();
	LiveConfigsTrackListChange();
	RegionPlaylistSetTrackListChange();
	ResourcesTrackListChange();
}

bool g_lastPlayState=false, g_lastPauseState=false, g_lastRecState=false;

void SNM_CSurfSetPlayState(bool _play, bool _pause, bool _rec)
{
	if (g_lastPlayState != _play)
	{
		if (g_lastPlayState && !_play)
			PlaylistStopped(_pause);
		g_lastPlayState = _play;
	}

	if (g_lastPauseState != _pause)
	{
		if (g_lastPlayState && !_pause)
			PlaylistUnpaused();
		g_lastPauseState = _pause;
	}

	if (g_lastRecState != _rec)
		g_lastRecState = _rec;
}

int SNM_CSurfExtended(int _call, void* _parm1, void* _parm2, void* _parm3)
{
#ifdef _SNM_DEBUG
//	char dbg[256] = "";
//	snprintf(dbg, sizeof(dbg), "SNM_CSurfExtended() - Call: %d, prm1: %p, prm2: %p prm3: %p\n", _call, _parm1, _parm2, _parm3);
//	OutputDebugString(dbg);
#endif
	return 0; // i.e. unsupported
}


///////////////////////////////////////////////////////////////////////////////
// OSC feedtack
///////////////////////////////////////////////////////////////////////////////

bool SNM_OscCSurf::SendStr(const char* _msg, const char* _oscArg, int _msgArg)
{
	if (_msg && *_msg && _oscArg)
	{
		oscpkt::UdpSocket sock;
		sock.connectTo(m_ipOut.Get(), m_portOut);
		if (sock.isOk())
		{
			WDL_FastString msg(_msg);
			if (_msgArg>=0)
				msg.SetFormatted(SNM_MAX_OSC_MSG_LEN, _msg, _msgArg);

			oscpkt::Message oscMsg(msg.Get());
			oscMsg.pushStr(_oscArg);
			oscpkt::PacketWriter pw;
			pw.startBundle();
			pw.addMessage(oscMsg);
			pw.endBundle();
			if ((int)pw.packetSize() < m_maxOut) // C4018
				return sock.sendPacket(pw.packetData(), pw.packetSize());
		}
	}
	return false;
}

bool SNM_OscCSurf::SendStrBundle(WDL_PtrList<WDL_FastString> * _strs)
{
	if (_strs && _strs->GetSize())
	{
		oscpkt::UdpSocket sock;
		sock.connectTo(m_ipOut.Get(), m_portOut);
		if (sock.isOk())
		{
			// build osc messages
			WDL_PtrList_DeleteOnDestroy<oscpkt::Message> msgs; // for "auto-deletion"
			for (int i=0; i<_strs->GetSize(); i+=2)
			{
				if (WDL_FastString* msg = _strs->Get(i))
				{
					oscpkt::Message* oscMsg = msgs.Add(new oscpkt::Message(msg->Get()));
					if (WDL_FastString* oscArg = _strs->Get(i+1))
						oscMsg->pushStr(oscArg->Get());
					else
						return false;
				}
			}

			// bundle & send osc messages
			if (msgs.GetSize())
			{
				oscpkt::PacketWriter pw;
				pw.startBundle();
				for (int i=0; i<msgs.GetSize(); i++)
					pw.addMessage(*msgs.Get(i));
				pw.endBundle();
				if ((int)pw.packetSize() < m_maxOut) // C4018
					return sock.sendPacket(pw.packetData(), pw.packetSize());
			}
		}
	}
	return false;
}

bool SNM_OscCSurf::Equals(SNM_OscCSurf* _osc)
{
	return _osc &&
		m_flags==_osc->m_flags &&
		m_portIn==_osc->m_portIn && 
		m_portOut==_osc->m_portOut && 
		m_maxOut==_osc->m_maxOut && 
		m_waitOut==_osc->m_waitOut && 
		!strcmp(m_name.Get(), _osc->m_name.Get()) &&
		!strcmp(m_ipOut.Get(), _osc->m_ipOut.Get()) &&
		!strcmp(m_layout.Get(), _osc->m_layout.Get());
}

// _out: instanciate all SNM_OscCSurf's from ini file
// _name: if != NULL, return the first osc csurf matching _name
// ^^ _out & _name are exclusive: "get all csurfs" or "get csurf by name"
// it is up to the caller to unalloc the returned value and/or _out
SNM_OscCSurf* LoadOscCSurfs(WDL_PtrList<SNM_OscCSurf>* _out, const char* _name)
{
// reaper.ini format:
// csurf_cnt=1
// csurf_0=OSC "name" 3 8000 "192.168.1.76" 9109 1024 10 "osc layout"

	if (_out)
		_out->Empty(true);

	char buf[16]="", bufline[SNM_MAX_CHUNK_LINE_LENGTH] = "";
	GetPrivateProfileString("REAPER", "csurf_cnt", "0", buf, sizeof(buf), get_ini_file()); 
	int cnt = atoi(buf);
	for (int i=0; i<cnt; i++)
	{
		if (snprintfStrict(buf, sizeof(buf), "csurf_%d", i) > 0)
		{
			LineParser lp(false);
			GetPrivateProfileString("REAPER", buf, "", bufline, sizeof(bufline), get_ini_file()); 
			if (!lp.parse(bufline) && lp.getnumtokens()>=9 && !_stricmp(lp.gettoken_str(0), "OSC"))
			{
				// not optimal.. but more readable
				SNM_OscCSurf* osc = new SNM_OscCSurf(
					lp.gettoken_str(1),
					lp.gettoken_int(2),
					lp.gettoken_int(3),
					lp.gettoken_str(4),
					lp.gettoken_int(5),
					lp.gettoken_int(6),
					lp.gettoken_int(7),
					lp.gettoken_str(8));

				if (_name && *_name) {
					if (!_stricmp(lp.gettoken_str(1), _name))
						return osc;
				}
				
				if (_out) _out->Add(osc);
				else delete osc;
			}
		}
	}
	return NULL;
}

// just to factorize osc cursf menu creations
void AddOscCSurfMenu(HMENU _menu, SNM_OscCSurf* _activeOsc, int _startMsg, int _endMsg)
{
	WDL_PtrList_DeleteOnDestroy<SNM_OscCSurf> oscCSurfs;
	LoadOscCSurfs(&oscCSurfs);
	int cnt = oscCSurfs.GetSize();

	HMENU hOscMenu = CreatePopupMenu();
	AddSubMenu(_menu, hOscMenu, __LOCALIZE("OSC feedback","sws_DLG_155"));
	if (cnt)
	{
		AddToMenu(hOscMenu, __LOCALIZE("None","sws_DLG_155"), _startMsg, -1, false, _activeOsc==NULL ? MFS_CHECKED : MFS_UNCHECKED);
		for (int i=0; i<cnt; i++)
			if (SNM_OscCSurf* osc = oscCSurfs.Get(i))
				if ((_startMsg+i+1) <= _endMsg)
					AddToMenu(hOscMenu, osc->m_name.Get(), _startMsg+i+1, -1, false, osc->Equals(_activeOsc) ? MFS_CHECKED : MFS_UNCHECKED);
	}
	else
		AddToMenu(hOscMenu, __LOCALIZE("(No OSC device found)","sws_DLG_155"), -1, -1, false, MF_GRAYED);
}


///////////////////////////////////////////////////////////////////////////////
// Fake/local OSC CSurf
// To send OSC messages to REAPER as if they were sent by a device
///////////////////////////////////////////////////////////////////////////////

#ifdef _SNM_MISC
void SNM_LocalOscCallback(void* _obj, const char* _msg, int _msglen) 
{
	if (_msg && _msglen)
	{
		oscpkt::PacketReader pr;
		pr.init(_msg, _msglen);
		oscpkt::Message* msg;
		while (pr.isOk() && (msg = pr.popMessage()) != 0)
			cout << "SNM_LocalOscCallback: " << msg << "\n";
	}
}
#endif

// _oscMsg is a human readable osc message, ex:
// - /track/1/fx/1/preset MyPreset
// - /track/1/fx/1/preset "My Preset"
// - /track/1/fx/1,2/fxparam/1,1/value 0.25 0.5
// notes: 
// 1) OscLocalMessageToHost() is not used here because 
//    there is no way to manage osc messages with string args
// 2) REAPER BUG? Using a global var for the handler + calling 
//    DestroyLocalOscHandler() in SNM_Exit() crashes REAPER (v4.32)
// 3) callbacks do not work yet (native issue, see below)
bool SNM_SendLocalOscMessage(const char* _oscMsg)
{
	if (!_oscMsg)
		return false;

	bool sent = false;
	static void* sOscHandler; //JFB static ATM, see above note
	if (!sOscHandler)
	{
#ifdef _SNM_MISC
		// REAPER BUG (v4.5rc13): does not work, see
		// http://forum.cockos.com/showthread.php?t=116377
		sOscHandler = CreateLocalOscHandler(NULL, SNM_LocalOscCallback);
#else
		sOscHandler = CreateLocalOscHandler(NULL, NULL);
#endif
	}
	if (sOscHandler)
	{
		LineParser lp(false);
		if (!lp.parse(_oscMsg) && lp.getnumtokens()>0)
		{
			oscpkt::Message msg(lp.gettoken_str(0));
			if (lp.getnumtokens()>1)
			{
				double d;
				int success;
				for (int i=1; i<lp.getnumtokens(); i++) // i=1!
				{
					d = lp.gettoken_float(i, &success);
					if (success)
						msg.pushFloat((float)d);
					else
					{
						WDL_FastString arg(lp.gettoken_str(i));
						// strip quotes, if needed
						const char* p1 = arg.Get();
						const char* p2 = p1 + arg.GetLength() - 1;
						if ((*p1=='"' || *p1=='\'' || *p1=='`') && 
							(*p2=='"' || *p2=='\'' || *p2=='`'))
						{
							arg.Set(p1+1, arg.GetLength()-2);
						}
						msg.pushStr(arg.Get());
					}
				}
			}
			oscpkt::PacketWriter pw;
			pw.addMessage(msg);
			SendLocalOscMessage(sOscHandler, pw.packetData(), pw.packetSize());
			sent = true;
		}
//		DestroyLocalOscHandler(sOscHandler); // see above note
	}
	return sent;
}
