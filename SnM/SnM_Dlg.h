/******************************************************************************
/ SnM_Dlg.h
/
/ Copyright (c) 2012-2013 Jeffos
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

#ifndef _SNM_DLG_H_
#define _SNM_DLG_H_

extern bool g_SNM_ClearType;

void SNM_UIInit();
void SNM_UIExit();
void SNM_UIRefresh(COMMAND_T*);


///////////////////////////////////////////////////////////////////////////////
// S&M window "manager": ensure safe lazy init of dockable windows
// Also overrides the default SWS screenset mgmt due to a slight design conflict
// (for lazy init, screensets should be managed outside SWS_DockWnd).
// Also avoids to touch the main SWS screenset code
// The trick is:
// => temp screenset registration - w/o instanciation, contrary to SWS wnds
// => the temp callback instanciates the wnd if needed
// => temp callback is replaced w/ the main SWS callback, see SWS_DockWnd
//    constructor and SWS_DockWnd::screensetCallback()
// Note: codeed in .h just because of template <class T> hassle..
///////////////////////////////////////////////////////////////////////////////

// code in .h just because of the (template) hassle..
template<class T> class SNM_WindowManager
{
public:
	SNM_WindowManager(const char* _id) : m_id(_id), m_wnd(NULL) {}
	virtual ~SNM_WindowManager() { Delete(false); }
	virtual T* Get() { return m_wnd; }
	virtual T* CreateFromIni() { return LazyInit(); }
	virtual T* CreateIfNeeded(bool* _isNew = NULL)
	{
		if (_isNew)
			*_isNew = false;
		if (!m_wnd)
		{
			m_wnd = Create();
			if (_isNew)
				*_isNew = (m_wnd!=NULL);
		}
		return m_wnd;
	}
	virtual void Delete(bool _register = false)
	{
		if (m_wnd)
		{
			DELETE_NULL(m_wnd); // incl. screenset unregister
			if (_register) {
				screenset_unregister((char*)m_id.Get()); // just in case..
				screenset_registerNew((char*)m_id.Get(), ScreensetCallback, this);
			}
		}
	}
	virtual HWND GetMsgHWND() {
		return m_wnd ? m_wnd->GetHWND() : GetMainHwnd();
	}

protected:
	// wnd instantiation => temp screenset callback is replaced w/ the main SWS callback
	virtual T* Create() { return new T; }

	WDL_FastString m_id;
	T* m_wnd;

private:
	// the trick
	static LRESULT ScreensetCallback(int _action, char* _id, void* _param, void* _actionParm, int _actionParmSize)
	{
		if (_actionParm && _actionParmSize>0)
			if (SNM_WindowManager* wc = (SNM_WindowManager*)_param)
				if (_action==SCREENSET_ACTION_LOAD_STATE && *(const char*)_actionParm && !strcmp(_id, wc->m_id.Get()))
					// wnd lazy init that unregister this callback & re-register SWS_DockWnd::screensetCallback()
					if (T* wnd = wc->LazyInit((const char*)_actionParm))
						return wnd->screensetCallback(_action, _id, wnd, _actionParm, _actionParmSize);
		return 0;
	}

	// window lazy init
	// _stateBuf=="__SnMWndIniState__" at init time only (read state from ini file), filled by ScreensetCallback otherwise
	T* LazyInit(const char* _stateBuf = "__SnMWndIniState__")
	{
		if (!m_wnd && _stateBuf && *_stateBuf)
		{
			bool init = !strcmp(_stateBuf, "__SnMWndIniState__");
			if (SWS_IsDockWndOpen(m_id.Get(), init ? NULL : _stateBuf)) {
				m_wnd = Create();
			}
			// do not create the wnd yet but register to its screenset updates
			if (!m_wnd && init) {
				screenset_unregister((char*)m_id.Get()); // just in case..
				screenset_registerNew((char*)m_id.Get(), ScreensetCallback, this);
			}
		}
		return m_wnd;
	}
};


// this class should not be used directly, see SNM_MultiWindowManager
// remark: some "SNM_WindowManager<T>::" are required for things to compile on OSX (useless on Win)
template<class T> class SNM_WindowInstManager : public SNM_WindowManager<T>
{
public:
	// _id is a formatted string (%d)
	SNM_WindowInstManager(const char* _wndId, int _idx) : SNM_WindowManager<T>(""), m_idx(_idx) {
		SNM_WindowManager<T>::m_id.SetFormatted(64, _wndId, _idx+1);
	}
	virtual ~SNM_WindowInstManager() {
		SNM_WindowManager<T>::Delete(false);
	}
protected:
	virtual T* Create() { return new T(m_idx); }
private:
	int m_idx;
};



// helper to manage multiple windows instances, a bit like a single S&M dockable window
template<class T> class SNM_MultiWindowManager
{
public:
	// _id is as formatted string (%d style)
	SNM_MultiWindowManager(const char* _id) : m_id(_id) {}
	virtual ~SNM_MultiWindowManager() { DeleteAll(false); }
	virtual T* Get(int _idx) {
		if (SNM_WindowInstManager<T>* mgr = m_mgrs.Get(_idx, NULL)) return mgr->Get();
		return NULL;
	}
	virtual T* CreateFromIni(int _idx)
	{
		SNM_WindowInstManager<T>* mgr = m_mgrs.Get(_idx, NULL);
		if (!mgr) {
			mgr = new SNM_WindowInstManager<T>(m_id.Get(), _idx);
			if (mgr) m_mgrs.Insert(_idx, mgr);
		}
		return mgr ? mgr->CreateFromIni() : NULL;
	}
	virtual T* CreateIfNeeded(int _idx, bool* _isNew = NULL)
	{
		SNM_WindowInstManager<T>* mgr = m_mgrs.Get(_idx, NULL);
		if (!mgr) {
			mgr = new SNM_WindowInstManager<T>(m_id.Get(), _idx);
			if (mgr) m_mgrs.Insert(_idx, mgr);
		}
		return mgr ? mgr->CreateIfNeeded(_isNew) : NULL;
	}
	virtual void DeleteAll(bool _register = false)
	{
		for (int i=m_mgrs.GetSize()-1; i>=0; i--) 
		{
			if (SNM_WindowInstManager<T>* mgr = m_mgrs.Enumerate(i, NULL, NULL))
				mgr->Delete(_register);
			m_mgrs.DeleteByIndex(i);
		}
	}
	virtual HWND GetMsgHWND(int _idx) {
		if (SNM_WindowInstManager<T>* mgr = m_mgrs.Get(_idx, NULL))
			return mgr->GetMsgHWND();
		return GetMainHwnd();
	}

protected:
	WDL_IntKeyedArray<SNM_WindowInstManager<T>* > m_mgrs; // no valdispose() provided, intentional
	WDL_FastString m_id;
};


///////////////////////////////////////////////////////////////////////////////

ColorTheme* SNM_GetColorTheme(bool _checkForSize = false);
IconTheme* SNM_GetIconTheme(bool _checkForSize = false);
LICE_CachedFont* SNM_GetFont(int _type = 0);
LICE_CachedFont* SNM_GetThemeFont();
LICE_CachedFont* SNM_GetToolbarFont();
void SNM_GetThemeListColors(int* _bg, int* _txt, int* _grid = NULL);
void SNM_GetThemeEditColors(int* _bg, int* _txt);
void SNM_ThemeListView(SWS_ListView* _lv);
LICE_IBitmap* SNM_GetThemeLogo();
WDL_DLGRET SNM_HookThemeColorsMessage(HWND _hwnd, UINT _uMsg, WPARAM _wParam, LPARAM _lParam, bool _wantColorEdit = true);

void SNM_ShowMsg(const char* _msg, const char* _title = "", HWND _hParent = NULL); 
int PromptForInteger(const char* _title, const char* _what, int _min, int _max);

#endif