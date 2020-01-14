/******************************************************************************
/ SnM_Dlg.h
/
/ Copyright (c) 2012 and later Jeffos
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

#ifndef _SNM_DLG_H_
#define _SNM_DLG_H_

extern bool g_SNM_ClearType;

void SNM_UIInit();
void SNM_UIExit();
void SNM_UIRefresh(COMMAND_T*);


///////////////////////////////////////////////////////////////////////////////
// S&M window manager: ensure lazy init of dockable windows, it also overrides
// the default SWS screenset mgmt (slight design conflict: for lazy init,
// screensets should be managed outside SWS_DockWnd).
// Note: everything is in the .h because of the <class T> inheritance hassle..
///////////////////////////////////////////////////////////////////////////////

// for single instance wnds
template<class T> class SNM_WindowManager
{
public:
	SNM_WindowManager(const char* _id) : m_id(_id), m_wnd(NULL) {}
	~SNM_WindowManager() { Delete(); }
	T* Get() { return reinterpret_cast<T *>(m_wnd); }

	T* Init()
	{ 
#ifdef _SNM_SCREENSET_DEBUG
		char dbg[256]="";
		_snprintfSafe(dbg, sizeof(dbg), "SNM_WindowManager::Init() - Register: %s\n", m_id.Get());
		OutputDebugString(dbg);
#endif
		screenset_registerNew((char*)m_id.Get(), SNM_ScreensetCallback, this);
		return GetWithLazyInit();
	}

	T* Create() { return Create(nullptr); }

	// creates the wnd *if needed*
	template<class... Args>
	T* Create(bool* _isNew, Args&&... args)
	{
		if (_isNew)
			*_isNew = !m_wnd;

		if (!m_wnd)
		{
#ifdef _SNM_SCREENSET_DEBUG
			char dbg[256]="";
			_snprintfSafe(dbg, sizeof(dbg), "SNM_WindowManager::Create() >>>>>>>>>>>>>>>>> %s\n", m_id.Get());
			OutputDebugString(dbg);
#endif

			// allocate memory before construction so that m_wnd is valid when
			// constructing the window
			m_wnd = new char[sizeof(T)];
			new(m_wnd) T(args...);
		}

		return Get();
	}

	void Delete()
	{
		if (!m_wnd)
			return;

		Get()->~T(); // also unregisters screenset callbacks..
		delete[] m_wnd;
		m_wnd = nullptr;
		// screenset_registerNew((char*)m_id.Get(), SNM_ScreensetCallback, this);
	}

	HWND GetMsgHWND() {
		return m_wnd ? Get()->GetHWND() : GetMainHwnd();
	}

protected:
	WDL_FastString m_id;

private:
	// the trick
	static LRESULT SNM_ScreensetCallback(int _action, const char* _id, void* _param, void* _actionParm, int _actionParmSize)
	{
#ifdef _SNM_SCREENSET_DEBUG
		char dbg[256]="";
		_snprintfSafe(dbg, sizeof(dbg), 
			"SNM_ScreensetCallback() - _id: %s, _action: %d, _param: %p, _actionParm: %p, _actionParmSize: %d\n", 
			_id, _action, _param, _actionParm, _actionParmSize);
		OutputDebugString(dbg);
#endif
		if (SNM_WindowManager* wc = (SNM_WindowManager*)_param)
		{
			SWS_DockWnd* wnd = (SWS_DockWnd*)wc->Get();
			switch(_action)
			{
				case SCREENSET_ACTION_GETHWND:
					return wnd ? (LRESULT)wnd->GetHWND() : 0;
				case SCREENSET_ACTION_IS_DOCKED:
					return wnd ? (LRESULT)wnd->IsDocked() : 0;
				case SCREENSET_ACTION_SWITCH_DOCK:
					if (wnd && SWS_IsWindow(wnd->GetHWND()))
						wnd->ToggleDocking();
					return 0;
				// from the SDK: "load state from actionParm (of actionParmSize). if both are NULL, hide"
				case SCREENSET_ACTION_LOAD_STATE:
					if (_actionParm && _actionParmSize)
						wnd = (SWS_DockWnd*)wc->GetWithLazyInit((const char*)_actionParm);
					if (wnd)
						wnd->LoadState((char*)_actionParm, _actionParmSize);
					return 0;
				// if the wnd does not exist yet,
				// create it & save its state (== current reaper.ini's state)
				case SCREENSET_ACTION_SAVE_STATE:
					wnd = (SWS_DockWnd*)wc->Create();
					return wnd ? wnd->SaveState((char*)_actionParm, _actionParmSize) : 0;
			}
		}
		return 0;
	}

	// _actionParm==NULL => read state from ini file
	T* GetWithLazyInit(const char* _actionParm = NULL)
	{
		if (!m_wnd && (SWS_GetDockWndState(m_id.Get(), _actionParm) & 1)) 
			Create();
		return Get();
	}

	char *m_wnd;
};


// this class should not be used directly, see SNM_MultiWindowManager
// remark: the syntax "SNM_WindowManager<T>::bla" is required to compil things on OSX..
template<class T> class SNM_DynWindowManager : public SNM_WindowManager<T>
{
public:
	// _wndId is a formatted string (%d)
	SNM_DynWindowManager(const char* _wndId, int _idx) : SNM_WindowManager<T>(""), m_idx(_idx) {
		SNM_WindowManager<T>::m_id.SetFormatted(64, _wndId, _idx+1);
	}

	~SNM_DynWindowManager() {
		SNM_WindowManager<T>::Delete();
	}

	// creates the wnd *if needed*
	T* Create(bool* _isNew = NULL)
	{
		return SNM_WindowManager<T>::Create(_isNew, m_idx);
	}

private:
	int m_idx;
};


// for multiple instance wnds
template<class T> class SNM_MultiWindowManager
{
public:
	// _id is as formatted string (%d style)
	SNM_MultiWindowManager(const char* _id) : m_id(_id) {}
	~SNM_MultiWindowManager() { DeleteAll(); }
	T* Get(int _idx) {
		if (SNM_DynWindowManager<T>* mgr = m_mgrs.Get(_idx, NULL)) return mgr->Get();
		return NULL;
	}
	T* Init(int _idx)
	{
		SNM_DynWindowManager<T>* mgr = m_mgrs.Get(_idx, NULL);
		if (!mgr) {
			mgr = new SNM_DynWindowManager<T>(m_id.Get(), _idx);
			if (mgr) m_mgrs.Insert(_idx, mgr);
		}
		return mgr ? mgr->Init() : NULL;
	}
	// creates the wnd *if needed*
	T* Create(int _idx, bool* _isNew = NULL)
	{
		SNM_DynWindowManager<T>* mgr = m_mgrs.Get(_idx, NULL);
		if (!mgr) {
			mgr = new SNM_DynWindowManager<T>(m_id.Get(), _idx);
			if (mgr) m_mgrs.Insert(_idx, mgr);
		}
		return mgr ? mgr->Create(_isNew) : NULL;
	}
	void DeleteAll()
	{
		for (int i=m_mgrs.GetSize()-1; i>=0; i--) 
		{
			if (SNM_DynWindowManager<T>* mgr = m_mgrs.Enumerate(i, NULL, NULL))
				mgr->Delete();
			m_mgrs.DeleteByIndex(i);
		}
	}
	HWND GetMsgHWND(int _idx) {
		if (SNM_DynWindowManager<T>* mgr = m_mgrs.Get(_idx, NULL))
			return mgr->GetMsgHWND();
		return GetMainHwnd();
	}

private:
	WDL_IntKeyedArray<SNM_DynWindowManager<T>*> m_mgrs; // no valdispose(), intentional
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
void SNM_ThemeListView(HWND _h);
void SNM_ThemeListView(SWS_ListView* _lv);
LICE_IBitmap* SNM_GetThemeLogo();
WDL_DLGRET SNM_HookThemeColorsMessage(HWND _hwnd, UINT _uMsg, WPARAM _wParam, LPARAM _lParam, bool _wantColorEdit = true);

void SNM_ShowMsg(const char* _msg, const char* _title = "", HWND _hParent = NULL); 
void SNM_CloseMsg(HWND _hParent = NULL);
int PromptForInteger(const char* _title, const char* _what, int _min, int _max, bool _showMinMax = true);

#endif
