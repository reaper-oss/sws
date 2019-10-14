/******************************************************************************
/ SnM_CSurf.h
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


#endif
