/******************************************************************************
/ SnM_CSurf.h
/
/ Copyright (c) 2013 Jeffos
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

//#pragma once

#ifndef _SNM_CSURF_H_
#define _SNM_CSURF_H_


// SWSTimeSlice:IReaperControlSurface callbacks
void SNM_CSurfRun();
void SNM_CSurfSetTrackTitle();
void SNM_CSurfSetTrackListChange();
void SNM_CSurfSetPlayState(bool _play, bool _pause, bool _rec);
int SNM_CSurfExtended(int _call, void* _parm1, void* _parm2, void* _parm3);


// osc csurf feedback
class SNM_OscCSurf {
public:
	SNM_OscCSurf(const char* _name, int _flags, int _portIn, const char* _ipOut, int _portOut, int _maxOut, int _waitOut, const char* _layout)
		: m_name(_name), m_flags(_flags), m_portIn(_portIn), 
		m_ipOut(_ipOut), m_portOut(_portOut), m_maxOut(_maxOut), m_waitOut(_waitOut), m_layout(_layout) {}
	SNM_OscCSurf(SNM_OscCSurf* _osc)
		: m_name(&_osc->m_name), m_flags(_osc->m_flags), m_portIn(_osc->m_portIn), 
		m_ipOut(&_osc->m_ipOut), m_portOut(_osc->m_portOut), m_maxOut(_osc->m_maxOut), m_waitOut(_osc->m_waitOut), m_layout(&_osc->m_layout) {}
	~SNM_OscCSurf() {}
	bool SendStr(const char* _msg, const char* _oscArg, int _msgArg = -1);
	bool SendStrBundle(WDL_PtrList<WDL_FastString> * _strs);
	bool Equals(SNM_OscCSurf* _osc);

	WDL_FastString m_name;
	int m_flags;
	int m_portIn;
	WDL_FastString m_ipOut;
	int m_portOut, m_maxOut, m_waitOut;
	WDL_FastString m_layout;
};

SNM_OscCSurf* LoadOscCSurfs(WDL_PtrList<SNM_OscCSurf>* _out, const char* _name = NULL);
void AddOscCSurfMenu(HMENU _menu, SNM_OscCSurf* _activeOsc, int _startMsg, int _endMsg);


// fake/local osc csurf (local input)
bool SNM_SendLocalOscMessage(const char* _oscMsg);


// IReaperControlSurface "proxy"
#ifdef _SNM_CSURF_PROXY

#define SNM_CSURFMAP(x) SWS_SectionLock lock(&m_csurfsMutex); for (int i=0; i<m_csurfs.GetSize(); i++) m_csurfs.Get(i)->x;

class SNM_CSurfProxy : public IReaperControlSurface {
public:
	SNM_CSurfProxy() {}
	~SNM_CSurfProxy() { RemoveAll(); }
	const char *GetTypeString() { return ""; }
	const char *GetDescString() { return ""; }
	const char *GetConfigString() { return ""; }
	void Run() { SNM_CSURFMAP(Run()) }
	void CloseNoReset() { SNM_CSURFMAP(CloseNoReset()) }
	void SetTrackListChange() { SNM_CSURFMAP(SetTrackListChange()) }
	void SetSurfaceVolume(MediaTrack *tr, double volume) { SNM_CSURFMAP(SetSurfaceVolume(tr, volume)) }
	void SetSurfacePan(MediaTrack *tr, double pan) { SNM_CSURFMAP(SetSurfacePan(tr, pan)) }
	void SetSurfaceMute(MediaTrack *tr, bool mute) { SNM_CSURFMAP(SetSurfaceMute(tr, mute)) }
	void SetSurfaceSelected(MediaTrack *tr, bool sel) { SNM_CSURFMAP(SetSurfaceSelected(tr, sel)) }
	void SetSurfaceSolo(MediaTrack *tr, bool solo) { SNM_CSURFMAP(SetSurfaceSolo(tr, solo)) }
	void SetSurfaceRecArm(MediaTrack *tr, bool recarm) { SNM_CSURFMAP(SetSurfaceRecArm(tr, recarm)) }
	void SetPlayState(bool play, bool pause, bool rec) { SNM_CSURFMAP(SetPlayState(play, pause, rec)) }
	void SetRepeatState(bool rep) { SNM_CSURFMAP(SetRepeatState(rep)) }
	void SetTrackTitle(MediaTrack *tr, const char *title) { SNM_CSURFMAP(SetTrackTitle(tr, title)) }
	bool GetTouchState(MediaTrack *tr, int isPan) { SNM_CSURFMAP(GetTouchState(tr, isPan)) return false; }
	void SetAutoMode(int mode) { SNM_CSURFMAP(SetAutoMode(mode)) }
	void ResetCachedVolPanStates() { SNM_CSURFMAP(ResetCachedVolPanStates()) }
	void OnTrackSelection(MediaTrack *tr) { SNM_CSURFMAP(OnTrackSelection(tr)) }
	bool IsKeyDown(int key) { SNM_CSURFMAP(IsKeyDown(key)) return false; }
	int Extended(int call, void *parm1, void *parm2, void *parm3) { 
		SWS_SectionLock lock(&m_csurfsMutex);
		int ret=0;
		for (int i=0; i<m_csurfs.GetSize(); i++)
			ret = m_csurfs.Get(i)->Extended(call, parm1, parm2, parm3) ? 1 : ret;
		return ret;
	}
	void Add(IReaperControlSurface* csurf) {
		SWS_SectionLock lock(&m_csurfsMutex);
		if (m_csurfs.Find(csurf)<0)
			m_csurfs.Add(csurf);
	}
	void Remove(IReaperControlSurface* csurf) {
		SWS_SectionLock lock(&m_csurfsMutex);
		m_csurfs.Delete(m_csurfs.Find(csurf), false);
	}
	void RemoveAll() {
		SWS_SectionLock lock(&m_csurfsMutex);
		for (int i=m_csurfs.GetSize(); i>=0; i--)
			if (IReaperControlSurface* csurf = m_csurfs.Get(i)) {
				csurf->Extended(SNM_CSURF_EXT_UNREGISTER, NULL, NULL, NULL);
				m_csurfs.Delete(i, false);
			}
	}
private:
	WDL_PtrList<IReaperControlSurface> m_csurfs;
	SWS_Mutex m_csurfsMutex;
};

extern SNM_CSurfProxy* g_SNM_CSurfProxy;

bool SNM_RegisterCSurf(IReaperControlSurface* _csurf);
bool SNM_UnregisterCSurf(IReaperControlSurface* _csurf);

#endif


#endif
