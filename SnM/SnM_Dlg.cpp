/******************************************************************************
/ SnM_Dlg.cpp
/
/ Copyright (c) 2009-2012 Jeffos
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
#include "../reaper/localize.h"
#ifndef _WIN32
#include "../Prompt.h"
#endif


///////////////////////////////////////////////////////////////////////////////
// WDL_VWnd custom controls
///////////////////////////////////////////////////////////////////////////////

void SNM_ToolbarButton::OnPaintOver(LICE_IBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect)
{
	WDL_VirtualIconButton::OnPaintOver(drawbm, origin_x, origin_y, cliprect);

	// paint text over (obeys the theme's "toolbar text" on/off colors)
	if (m_iconCfg && m_iconCfg->olimage && m_forcetext)
	{
		bool isdown = !!(m_pressed&1);
		ColorTheme* ct = SNM_GetColorTheme();
		LICE_IFont *font = SNM_GetToolbarFont();
		bool isVert=false;
		if (font && m_textfontv && m_position.right-m_position.left < m_position.bottom - m_position.top)
		{
			isVert=true;
			font = m_textfontv;
		}

		// draw text
		if (font&&m_textlbl.Get()[0])
		{
			int fgc = m_forcetext_color ? m_forcetext_color :
				(!ct ? LICE_RGBA_FROMNATIVE(GSC(COLOR_BTNTEXT),255) :
					(isdown ? LICE_RGBA_FROMNATIVE(ct->toolbar_button_text_on,255) : LICE_RGBA_FROMNATIVE(ct->toolbar_button_text,255)));

			//font->SetCombineMode(LICE_BLIT_MODE_COPY, alpha); // this affects the glyphs that get cached
			font->SetBkMode(TRANSPARENT);
			font->SetTextColor(fgc);

			RECT r2=m_position;
			r2.left += origin_x+m_margin_l;
			r2.right += origin_x-m_margin_r;
			r2.top += origin_y+m_margin_t;
			r2.bottom += origin_y-m_margin_b;

			int f = DT_SINGLELINE|DT_NOPREFIX;
			if (isVert) f |= DT_CENTER | (m_textalign<0?DT_TOP:m_textalign>0?DT_BOTTOM:DT_VCENTER);
			else f |= DT_VCENTER|(m_textalign<0?DT_LEFT:m_textalign>0?DT_RIGHT:DT_CENTER);
			font->DrawText(drawbm,m_textlbl.Get(),-1,&r2,f);
		}
	}

}

int SNM_ImageVWnd::GetWidth() {
	if (m_img) return m_img->getWidth();
	return 0;
}

int SNM_ImageVWnd::GetHeight() {
	if (m_img) return m_img->getHeight();
	return 0;
}

void SNM_ImageVWnd::OnPaint(LICE_IBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect) {
	if (m_img)
	{
		LICE_ScaledBlit(drawbm,m_img,
			m_position.left+origin_x,m_position.top+origin_y,
			m_position.right-m_position.left,m_position.bottom-m_position.top, 
			0.0f,0.0f,(float)GetWidth(),(float)GetHeight(),
			1.0f,LICE_BLIT_MODE_COPY|LICE_BLIT_USE_ALPHA);
	}
}

void SNM_AddDelButton::OnPaint(LICE_IBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect)
{
	RECT r = m_position;
	r.left+=origin_x;
	r.right+=origin_x;
	r.top += origin_y;
	r.bottom += origin_y;

	ColorTheme* ct = SNM_GetColorTheme();
	int col = ct ? LICE_RGBA_FROMNATIVE(ct->main_text,255) : LICE_RGBA(255,255,255,255);
	float alpha = m_en ? 0.8f : 0.4f;

	// border
	LICE_Line(drawbm,r.left,r.bottom-1,r.left,r.top,col,alpha,0,false);
	LICE_Line(drawbm,r.left,r.top,r.right-1,r.top,col,alpha,0,false);
	LICE_Line(drawbm,r.right-1,r.top,r.right-1,r.bottom-1,col,alpha,0,false);
	LICE_Line(drawbm,r.left,r.bottom-1,r.right-1,r.bottom-1,col,alpha,0,false);

	// + or -
	int delta = m_add?2:3;
	LICE_Line(drawbm,r.left+delta,int(r.top+((r.bottom-r.top)/2)+0.5),r.right-(delta+1),int(r.top+((r.bottom-r.top)/2)+0.5),col,alpha,0,false);
	if (m_add)
		LICE_Line(drawbm,int(r.left+((r.right-r.left)/2)+0.5), r.top+delta,int(r.left+((r.right-r.left)/2)+0.5),r.bottom-(delta+1),col,alpha,0,false);
}


///////////////////////////////////////////////////////////////////////////////
// Themed UIs
///////////////////////////////////////////////////////////////////////////////

ColorTheme* SNM_GetColorTheme(bool _checkForSize) {
	int sz; ColorTheme* ct = (ColorTheme*)GetColorThemeStruct(&sz);
	if (ct && (!_checkForSize || _checkForSize && sz >= sizeof(ColorTheme))) return ct;
	return NULL;
}

IconTheme* SNM_GetIconTheme(bool _checkForSize) {
	int sz; IconTheme* it = (IconTheme*)GetIconThemeStruct(&sz);
	if (it && (!_checkForSize || _checkForSize && sz >= sizeof(IconTheme))) return it;
	return NULL;
}

LICE_CachedFont* SNM_GetThemeFont()
{
	static LICE_CachedFont themeFont;
	if (!themeFont.GetHFont())
	{
		LOGFONT lf = {
			SNM_FONT_HEIGHT, 0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,SNM_FONT_NAME
		};
		themeFont.SetFromHFont(CreateFontIndirect(&lf),LICE_FONT_FLAG_OWNS_HFONT | (g_SNMClearType?LICE_FONT_FLAG_FORCE_NATIVE:0));
	}
	themeFont.SetBkMode(TRANSPARENT);
	ColorTheme* ct = SNM_GetColorTheme();
	themeFont.SetTextColor(ct ? LICE_RGBA_FROMNATIVE(ct->main_text,255) : LICE_RGBA(255,255,255,255));
	return &themeFont;
}

LICE_CachedFont* SNM_GetToolbarFont()
{
	static LICE_CachedFont themeFont;
	if (!themeFont.GetHFont())
	{
		LOGFONT lf = {
			SNM_FONT_HEIGHT,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,SNM_FONT_NAME
		};
		themeFont.SetFromHFont(CreateFontIndirect(&lf), LICE_FONT_FLAG_OWNS_HFONT | (g_SNMClearType?LICE_FONT_FLAG_FORCE_NATIVE:0));
	}
	themeFont.SetBkMode(TRANSPARENT);
	ColorTheme* ct = SNM_GetColorTheme();
	themeFont.SetTextColor(ct ? LICE_RGBA_FROMNATIVE(ct->toolbar_button_text,255) : LICE_RGBA(255,255,255,255));
	return &themeFont;
}

HBRUSH g_hb = NULL;
int g_lastThemeBrushColor = -1;

HBRUSH SNM_GetThemeBrush(int _col)
{
	if (g_hb) {
		DeleteObject(g_hb);
		g_hb = NULL;
	}
	g_lastThemeBrushColor = (_col==-666 ? GSC_mainwnd(COLOR_WINDOW) : _col);
	g_hb = (HBRUSH)CreateSolidBrush(g_lastThemeBrushColor);
	return g_hb;
}

void SNM_GetThemeListColors(int* _bg, int* _txt)
{
	int bgcol=-1, txtcol=-1;
	ColorTheme* ct = SNM_GetColorTheme(true); // true: list view colors are recent (v4.11)
	if (ct) {
		bgcol = ct->genlist_bg;
		txtcol = ct->genlist_fg;
		// note: grid & selection colors not managed
	}
	if (bgcol == txtcol) { // safety
		bgcol = GSC_mainwnd(COLOR_WINDOW);
		txtcol = GSC_mainwnd(COLOR_BTNTEXT);
	}
	if (_bg) *_bg = bgcol;
	if (_txt) *_txt = txtcol;
}

void SNM_GetThemeEditColors(int* _bg, int* _txt)
{
	int bgcol=-1, txtcol=-1;
	ColorTheme* ct = SNM_GetColorTheme();
	if (ct) {
		bgcol =  ct->main_editbk;
		txtcol = GSC_mainwnd(COLOR_BTNTEXT);
	}
	if (bgcol == txtcol) { // safety
		bgcol = GSC_mainwnd(COLOR_WINDOW);
		txtcol = GSC_mainwnd(COLOR_BTNTEXT);
	}
	if (_bg) *_bg = bgcol;
	if (_txt) *_txt = txtcol;
}

void SNM_ThemeListView(SWS_ListView* _lv)
{
	if (_lv && _lv->GetHWND())
	{
		int bgcol, txtcol;
		SNM_GetThemeListColors(&bgcol, &txtcol);
		ListView_SetBkColor(_lv->GetHWND(), bgcol);
		ListView_SetTextColor(_lv->GetHWND(), txtcol);
		ListView_SetTextBkColor(_lv->GetHWND(), bgcol);
	}
}

LICE_IBitmap* SNM_GetThemeLogo()
{
	static LICE_IBitmap* snmLogo;
	if (!snmLogo)
	{
/*JFB commented: load from resources, KO for OSX (looks for REAPER's resources..)
#ifdef _WIN32
		snmLogo = LICE_LoadPNGFromResource(g_hInst,IDB_SNM,NULL);
#else
		// SWS doesn't work, sorry. :( 
		//snmLogo =  LICE_LoadPNGFromNamedResource("SnM.png",NULL);
		snmLogo = NULL;
#endif
*/
		// logo is now loaded from memory (OSX support)
		if (WDL_HeapBuf* hb = TranscodeStr64ToHeapBuf(SNM_LOGO_PNG_FILE)) {
			snmLogo = LICE_LoadPNGFromMemory(hb->Get(), hb->GetSize());
			delete hb;
		}
	}
	return snmLogo;
}

void SNM_SkinButton(WDL_VirtualIconButton* _btn, WDL_VirtualIconButton_SkinConfig* _skin, const char* _text)
{
	if (_skin && _skin->image) {
		_btn->SetIcon(_skin);
		_btn->SetForceBorder(false);
	}
	else {
		_btn->SetIcon(NULL);
		_btn->SetTextLabel(_text, 0, SNM_GetThemeFont());
		_btn->SetForceBorder(true);
	}
}

void SNM_SkinToolbarButton(SNM_ToolbarButton* _btn, const char* _text)
{
	static WDL_VirtualIconButton_SkinConfig skin;
	IconTheme* it = SNM_GetIconTheme(true); // true: blank & overlay images are recent (v4)
	if (it && it->toolbar_blank
/*JFB no! (lazy init behind the scene?)
		&& it->toolbar_overlay
*/
		)
	{
		skin.image = it->toolbar_blank;
		skin.olimage = it->toolbar_overlay;
		WDL_VirtualIconButton_PreprocessSkinConfig(&skin);

		//JFB!!! most stupid hack since WDL 65568bc (overlay == main image size)
		for (int i=0; i<4 ; i++)
			skin.image_ltrb_ol[i]=0;

		_btn->SetIcon(&skin);
		_btn->SetForceBorder(false);
		_btn->SetForceText(true); // do not force colors (done in SNM_ToolbarButton::OnPaintOver())
		_btn->SetTextLabel(_text, 0, SNM_GetToolbarFont());
	}
	else 
	{
		_btn->SetIcon(NULL); // important: could crash when switching theme..
		_btn->SetTextLabel(_text, 0, SNM_GetThemeFont());
		_btn->SetForceBorder(true);
	}
}

//JFB TODO? WDL_VWnd? hyperlink ?
bool SNM_AddLogo(LICE_IBitmap* _bm, const RECT* _r, int _x, int _h)
{
	if (_bm)
	{
		LICE_IBitmap* logo = SNM_GetThemeLogo();
		if (logo && (_x + logo->getWidth() < _r->right - 5))
		{
			int y = _r->top + int(_h/2 - logo->getHeight()/2 + 0.5);
			LICE_Blit(_bm,logo,_r->right-logo->getWidth()-8,y,NULL,0.125f,LICE_BLIT_MODE_ADD|LICE_BLIT_USE_ALPHA);
			return true;
		}
	}
	return false;
}

//JFB not used yet
bool SNM_AddLogo2(SNM_Logo* _logo, const RECT* _r, int _x, int _h)
{
	if (_x+_logo->GetWidth() < _r->right-8)
	{
		int x = _r->right - _logo->GetWidth() - 8;
		int y = _r->top + int(_h/2 - _logo->GetHeight()/2 + 0.5);
		RECT tr = {x, y, x + _logo->GetWidth(), y + _logo->GetHeight()};
		_logo->SetPosition(&tr);
		_logo->SetVisible(true);
		return true;
	}
	return false;
}

// auto position a WDL_VWnd instance
// note: by default all components are hidden, see WM_PAINT in sws_wnd.cpp
// _x: I/O param that gets modified (for the next WDL_VWnd to be placed in the panel)
// _h: height of the panel
// returns false if hidden
// JFB TODO? REMARK: 
//    ideally, we'd need to mod WDL_VWnd here rather than checking inherited types (!)
//    e.g. adding a kind of getPreferedWidthHeight(int* _width, int* _height)
bool SNM_AutoVWndPosition(WDL_VWnd* _c, WDL_VWnd* _tiedComp, const RECT* _r, int* _x, int _y, int _h, int _xRoomNextComp)
{
	if (_c && _h && abs(_r->bottom-_r->top) >= _h)
	{
		int width=0, height=_h;

		// see top remark..
		if (!strcmp(_c->GetType(), "vwnd_combobox"))
		{
			WDL_VirtualComboBox* cb = (WDL_VirtualComboBox*)_c;
			for (int i=0; i < cb->GetCount(); i++) {
				RECT tr = {0,0,0,0};
				cb->GetFont()->DrawText(NULL, cb->GetItem(i), -1, &tr, DT_CALCRECT);
				width = max(width, tr.right);
				height = tr.bottom; 
			}
/*JFB better? InvalidateRect/RequestRedraw issue anyway..
			RECT tr = {0,0,0,0};
			cb->GetFont()->DrawText(NULL, cb->GetItem(cb->GetCurSel()), -1, &tr, DT_CALCRECT);
			width = tr.right;
			height = tr.bottom; 
*/
			height = height + int(height/2 + 0.5);
			width += 2*height; // 2*height for the arrow zone (square)
		}
		else if (!strcmp(_c->GetType(), "vwnd_statictext"))
		{
			WDL_VirtualStaticText* txt = (WDL_VirtualStaticText*)_c;
			RECT tr = {0,0,0,0};
			txt->GetFont()->DrawText(NULL, txt->GetText(), -1, &tr, DT_CALCRECT);
			width = tr.right;
		}
		else if (!strcmp(_c->GetType(), "vwnd_iconbutton") || !strcmp(_c->GetType(), "SNM_ToolbarButton"))
		{
			WDL_VirtualIconButton* btn = (WDL_VirtualIconButton*)_c;
			WDL_VirtualIconButton_SkinConfig* skin = btn->GetIcon();
			if (skin && skin->image)
			{
				width = skin->image->getWidth() / 3;
				height = skin->image->getHeight();
				if (!strcmp(_c->GetType(), "SNM_ToolbarButton")) {
					width = int(2.6*width); // larger toolbar buttons!
					height = int(0.75*height + 0.5) + 1; // +1 for text vertical alignment
				}
			}
			else if (btn->GetFont())
			{
				RECT tr = {0,0,0,0};
				btn->GetFont()->DrawText(NULL, btn->GetTextLabel(), -1, &tr, DT_CALCRECT);
				height = tr.bottom + int(tr.bottom/2 + 0.5);
				width = int(tr.right + height/2 + 0.5); // +height/2 for some air
				if (btn->GetCheckState() != -1) {
					width += tr.bottom; // for the tick zone
					height -= 2;
				}
/*JFB -= 2 above.. glitch but looks better/aligned
				// workaround for paint glitch with odd (i.e. not even) heights
				if (height%2 == 1) { height--; _y++; }
*/
			}
		}
		else  if (!strcmp(_c->GetType(), "SNM_MiniAddDelButtons")) {
			width=9;
			height=9*2+1;
		}
		else if (!strcmp(_c->GetType(), "SNM_MiniKnob")) {
			width=21;
			height=21;
		}

		if (*_x+width > _r->right-10) // enough horizontal room?
		{
			if (*_x+20 > (_r->right-10)) // ensures a minimum width
			{
				if (_tiedComp && _tiedComp->IsVisible())
					_tiedComp->SetVisible(false);
				return false;
			}
			width = _r->right - 10 - *_x; // force width
		}

		_y += int(_h/2 - height/2 + 0.5);
		RECT tr = {*_x, _y, *_x + width, _y+height};
		_c->SetPosition(&tr);
		*_x = tr.right + _xRoomNextComp;
		_c->SetVisible(true);
		return true;
	}

	if (_tiedComp && _tiedComp->IsVisible())
		_tiedComp->SetVisible(false);

	return false;
}

void SNM_UIInit() {}

void SNM_UIExit() {
	if (g_hb) DeleteObject(g_hb);
	if (LICE_IBitmap* logo = SNM_GetThemeLogo()) DELETE_NULL(logo);
}


///////////////////////////////////////////////////////////////////////////////
// Messages, prompt, etc..
///////////////////////////////////////////////////////////////////////////////

// GUI for lazy guys
void SNM_ShowMsg(const char* _msg, const char* _title, HWND _hParent, bool _clear)
{
#ifdef _WIN32
	if (_clear) ShowConsoleMsg("");
	ShowConsoleMsg(_msg);
	if (_title) // a little hack..
	{
		HWND h = GetReaWindowByTitle(__localizeFunc("ReaScript console output", "DLG_437", 0));
		if (h)
			SetWindowText(h, _title);
		else // already opened?
			h = GetReaWindowByTitle(_title);
		if (h)
			SetForegroundWindow(h);
	}
#else
	//JFB nice but modal..
	HWND h = _hParent;
	if (!h) h = GetMainHwnd();
	char msg[4096] = "";
	GetStringWithRN(_msg, msg, 4096); // truncates if needed
	DisplayInfoBox(h, _title, msg);
#endif
}

// _min and _max: 1-based (i.e. as displayed)
//returns -1 on cancel, 0-based number otherwise
int PromptForInteger(const char* _title, const char* _what, int _min, int _max)
{
	WDL_String str;
	int nb = -1;
	while (nb == -1)
	{
		str.SetFormatted(128, "%s (%d-%d):", _what, _min, _max);
		char reply[8]= ""; // no default
		if (GetUserInputs(_title, 1, str.Get(), reply, 8))
		{
			nb = atoi(reply); // 0 on error
			if (nb >= _min && nb <= _max)
				return (nb-1);
			else {
				nb = -1;
				str.SetFormatted(128, "Invalid %s!\nPlease enter a value in [%d; %d].", _what, _min, _max);
				MessageBox(GetMainHwnd(), str.Get(), __LOCALIZE("S&M - Error","sws_mbox"), MB_OK);
			}
		}
		else return -1; // user has cancelled
	}
	return -1;
}


///////////////////////////////////////////////////////////////////////////////
// Cue buss dialog box
///////////////////////////////////////////////////////////////////////////////

int GetComboSendIdxType(int _reaType) 
{
	switch(_reaType) {
		case 0: return 1;
		case 3: return 2; 
		case 1: return 3; 
		default: return 1;
	}
	return 1; // in case _reaType comes from mars
}

const char* GetSendTypeStr(int _type) 
{
	switch(_type) {
		case 1: return __localizeFunc("Post-Fader (Post-Pan)","common",0);
		case 2: return __localizeFunc("Pre-Fader (Post-FX)","common",0);
		case 3: return __localizeFunc("Pre-FX","common",0);
		default: return "";
	}
}

void fillHWoutDropDown(HWND _hwnd, int _idc)
{
	char buffer[BUFFER_SIZE];
	lstrcpyn(buffer, __LOCALIZE("None","sws_DLG_149"), BUFFER_SIZE);
	SendDlgItemMessage(_hwnd,_idc,CB_ADDSTRING,0,(LPARAM)buffer);
	SendDlgItemMessage(_hwnd,_idc,CB_SETITEMDATA,0,0);
	
	// get mono outputs
	WDL_PtrList<WDL_FastString> monos;
	int monoIdx=0;
	while (GetOutputChannelName(monoIdx))
	{
		monos.Add(new WDL_FastString(GetOutputChannelName(monoIdx)));
		monoIdx++;
	}

	// add stereo outputs
	WDL_PtrList<WDL_FastString> stereos;
	if (monoIdx)
	{
		for(int i=0; i < (monoIdx-1); i++)
		{
			WDL_FastString* hw = new WDL_FastString();
			hw->SetFormatted(256, "%s / %s", monos.Get(i)->Get(), monos.Get(i+1)->Get());
			stereos.Add(hw);
		}
	}

	// fill dropdown
	for(int i=0; i < stereos.GetSize(); i++) {
		SendDlgItemMessage(_hwnd,_idc,CB_ADDSTRING,0,(LPARAM)stereos.Get(i)->Get());
		SendDlgItemMessage(_hwnd,_idc,CB_SETITEMDATA,i,i+1); // +1 for <none>
	}
	for(int i=0; i < monos.GetSize(); i++) {
		SendDlgItemMessage(_hwnd,_idc,CB_ADDSTRING,0,(LPARAM)monos.Get(i)->Get());
		SendDlgItemMessage(_hwnd,_idc,CB_SETITEMDATA,i,i+1); // +1 for <none>
	}

//	SendDlgItemMessage(_hwnd,_idc,CB_SETCURSEL,x0,0);
	monos.Empty(true);
	stereos.Empty(true);
}

HWND g_cueBussHwnd = NULL;
int g_cueBussConfId = 0; // not saved in prefs yet
bool g_cueBussDisableSave = false;

void FillCueBussDlg(HWND _hwnd=NULL)
{
	HWND hwnd = _hwnd?_hwnd:g_cueBussHwnd;
	if (!hwnd)
		return;

	g_cueBussDisableSave=true;
	char busName[BUFFER_SIZE]="", trTemplatePath[BUFFER_SIZE]="";
	int reaType, userType, soloDefeat, hwOuts[8];
	bool trTemplate, showRouting, sendToMaster;
	readCueBusIniFile(g_cueBussConfId, busName, &reaType, &trTemplate, trTemplatePath, &showRouting, &soloDefeat, &sendToMaster, hwOuts);
	userType = GetComboSendIdxType(reaType);
	SetDlgItemText(hwnd,IDC_SNM_CUEBUS_NAME,busName);

	for(int i=0; i<3; i++) {
		if (_hwnd) // do it once (WM_INITDIALOG)
			SendDlgItemMessage(hwnd,IDC_SNM_CUEBUS_TYPE,CB_ADDSTRING,0,(LPARAM)GetSendTypeStr(i+1));
		if (userType==(i+1)) SendDlgItemMessage(hwnd,IDC_SNM_CUEBUS_TYPE,CB_SETCURSEL,i,0);
	}

	SetDlgItemText(hwnd,IDC_SNM_CUEBUS_TEMPLATE,trTemplatePath);
	CheckDlgButton(hwnd, IDC_CHECK1, sendToMaster);
	CheckDlgButton(hwnd, IDC_CHECK2, showRouting);
	CheckDlgButton(hwnd, IDC_CHECK3, trTemplate);
	CheckDlgButton(hwnd, IDC_CHECK4, (soloDefeat == 1));

	for(int i=0; i < SNM_MAX_HW_OUTS; i++) {
		if (_hwnd) // do it once (WM_INITDIALOG)
			fillHWoutDropDown(hwnd,IDC_SNM_CUEBUS_HWOUT1+i);
		SendDlgItemMessage(hwnd,IDC_SNM_CUEBUS_HWOUT1+i,CB_SETCURSEL,hwOuts[i],0);
	}

//	SetFocus(GetDlgItem(hwnd, IDC_SNM_CUEBUS_NAME));
	PostMessage(hwnd, WM_COMMAND, IDC_CHECK3, 0); // enable//disable state

	g_cueBussDisableSave = false;
}

void SaveCueBussSettings()
{
	if (!g_cueBussHwnd || g_cueBussDisableSave)
		return;

	char cueBusName[BUFFER_SIZE]="";
	GetDlgItemText(g_cueBussHwnd,IDC_SNM_CUEBUS_NAME,cueBusName,BUFFER_SIZE);

	int userType=2, reaType;
	int combo = (int)SendDlgItemMessage(g_cueBussHwnd,IDC_SNM_CUEBUS_TYPE,CB_GETCURSEL,0,0);
	if(combo != CB_ERR)
		userType = combo+1;
	switch(userType) {
		case 1: reaType=0; break;
		case 2: reaType=3; break;
		case 3: reaType=1; break;
		default: reaType=3; break;
	}

	int sendToMaster = IsDlgButtonChecked(g_cueBussHwnd, IDC_CHECK1);
	int showRouting = IsDlgButtonChecked(g_cueBussHwnd, IDC_CHECK2);
	int trTemplate = IsDlgButtonChecked(g_cueBussHwnd, IDC_CHECK3);
	int soloDefeat = IsDlgButtonChecked(g_cueBussHwnd, IDC_CHECK4);

	char trTemplatePath[BUFFER_SIZE]="";
	GetDlgItemText(g_cueBussHwnd,IDC_SNM_CUEBUS_TEMPLATE,trTemplatePath,BUFFER_SIZE);

	int hwOuts[SNM_MAX_HW_OUTS];
	for (int i=0; i<SNM_MAX_HW_OUTS; i++) {
		hwOuts[i] = (int)SendDlgItemMessage(g_cueBussHwnd,IDC_SNM_CUEBUS_HWOUT1+i,CB_GETCURSEL,0,0);
		if(hwOuts[i] == CB_ERR)	hwOuts[i] = '\0';
	}

	saveCueBusIniFile(g_cueBussConfId, cueBusName, reaType, (trTemplate == 1), trTemplatePath, (showRouting == 1), soloDefeat, (sendToMaster == 1), hwOuts);
}

WDL_DLGRET CueBussDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	const char cWndPosKey[] = "CueBus Window Pos"; 
	switch(Message)
	{
        case WM_INITDIALOG: {
			RestoreWindowPos(hwnd, cWndPosKey, false);
			char buf[4] = "";
			for(int i=0; i < SNM_MAX_CUE_BUSS_CONFS; i++) {
				_snprintf(buf,4,"%d",i+1);
				SendDlgItemMessage(hwnd,IDC_COMBO,CB_ADDSTRING,0,(LPARAM)buf);
			}
			SendDlgItemMessage(hwnd,IDC_COMBO,CB_SETCURSEL,0,0);
			FillCueBussDlg(hwnd);
			return 0;
		}
		break;

		case WM_CLOSE :
			g_cueBussHwnd = NULL; // for proper toggle state report, see openCueBussWnd()
			break;

		case WM_COMMAND :
            switch(LOWORD(wParam))
            {
				case IDC_COMBO:
					// config id update
					if(HIWORD(wParam) == CBN_SELCHANGE) {
						int id = (int)SendDlgItemMessage(hwnd,IDC_COMBO,CB_GETCURSEL,0,0);
						if (id != CB_ERR) {
							g_cueBussConfId = id;
							FillCueBussDlg();
						}
					}
					break;
                case IDOK:
					cueTrack(__LOCALIZE("Create cue buss from track selection","sws_undo"), g_cueBussConfId);
					return 0;
				case IDCANCEL:
					g_cueBussHwnd = NULL; // for proper toggle state report, see openCueBussWnd()
					ShowWindow(hwnd, SW_HIDE);
					return 0;
				case IDC_FILES: {
					char curPath[BUFFER_SIZE]="";
					GetDlgItemText(hwnd,IDC_SNM_CUEBUS_TEMPLATE,curPath,BUFFER_SIZE);
					if (!*curPath || !FileExists(curPath))
						_snprintf(curPath, BUFFER_SIZE, "%s%c%TrackTemplates", GetResourcePath(), PATH_SLASH_CHAR);
					if (char* fn = BrowseForFiles(__LOCALIZE("S&M - Load track template","sws_DLG_149"), curPath, NULL, false, "REAPER Track Template (*.RTrackTemplate)\0*.RTrackTemplate\0")) {
						SetDlgItemText(hwnd,IDC_SNM_CUEBUS_TEMPLATE,fn);
						free(fn);
						SaveCueBussSettings();
					}
					break;
				}
				case IDC_CHECK3: {
					bool templateEnable = (IsDlgButtonChecked(hwnd, IDC_CHECK3) == 1);
					EnableWindow(GetDlgItem(hwnd, IDC_SNM_CUEBUS_TEMPLATE), templateEnable);
					EnableWindow(GetDlgItem(hwnd, IDC_FILES), templateEnable);
					EnableWindow(GetDlgItem(hwnd, IDC_SNM_CUEBUS_NAME), !templateEnable);
					for(int k=0; k < SNM_MAX_HW_OUTS ; k++)
						EnableWindow(GetDlgItem(hwnd, IDC_SNM_CUEBUS_HWOUT1+k), !templateEnable);
					EnableWindow(GetDlgItem(hwnd, IDC_CHECK1), !templateEnable);
					EnableWindow(GetDlgItem(hwnd, IDC_CHECK4), !templateEnable);
//					SetFocus(GetDlgItem(hwnd, templateEnable ? IDC_SNM_CUEBUS_TEMPLATE : IDC_SNM_CUEBUS_NAME));
					SaveCueBussSettings();
					break;
				}
				case IDC_SNM_CUEBUS_SOLOGRP:
				case IDC_CHECK1:
				case IDC_CHECK2:
				case IDC_CHECK4:
					SaveCueBussSettings();
					break;
				case IDC_SNM_CUEBUS_TYPE:
				case IDC_SNM_CUEBUS_HWOUT1:
				case IDC_SNM_CUEBUS_HWOUT2:
				case IDC_SNM_CUEBUS_HWOUT3:
				case IDC_SNM_CUEBUS_HWOUT4:
				case IDC_SNM_CUEBUS_HWOUT5:
				case IDC_SNM_CUEBUS_HWOUT6:
				case IDC_SNM_CUEBUS_HWOUT7:
				case IDC_SNM_CUEBUS_HWOUT8:
					if(HIWORD(wParam) == CBN_SELCHANGE)
						SaveCueBussSettings();
					break;
				case IDC_SNM_CUEBUS_TEMPLATE:
				case IDC_SNM_CUEBUS_NAME:
					if (HIWORD(wParam)==EN_CHANGE)
						SaveCueBussSettings();
					break;
			}
			break;

		case WM_DESTROY:
			SaveWindowPos(hwnd, cWndPosKey);
			break; 
	}
	return 0;
}

void openCueBussWnd(COMMAND_T* _ct)
{
	static HWND hwnd = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_SNM_CUEBUSS), GetMainHwnd(), CueBussDlgProc);
	if (g_cueBussHwnd) {
		g_cueBussHwnd = NULL;
		ShowWindow(hwnd, SW_HIDE);
	}
	else {
		g_cueBussHwnd = hwnd;
		ShowWindow(hwnd, SW_SHOW);
		SetFocus(hwnd);
	}
}

bool isCueBussWndDisplayed(COMMAND_T* _ct) {
	return (g_cueBussHwnd && IsWindow(g_cueBussHwnd) && IsWindowVisible(g_cueBussHwnd) ? true : false);
}


#ifdef _SNM_MISC // wip
///////////////////////////////////////////////////////////////////////////////
// Merge LangPack dialog box
///////////////////////////////////////////////////////////////////////////////

HWND g_langpackMrgHwnd = NULL;

WDL_DLGRET LangpackMergeDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	const char cWndPosKey[] = "LangPack Merger Window Pos";
	switch(Message)
	{
        case WM_INITDIALOG :
			RestoreWindowPos(hwnd, cWndPosKey, false);
			SetFocus(GetDlgItem(hwnd, IDC_SNM_CUEBUS_NAME));
			PostMessage(hwnd, WM_COMMAND, IDC_CHECK3, 0); // enable//disable state
			return 0;

		case WM_CLOSE :
			g_langpackMrgHwnd = NULL; // for proper toggle state report, see openCueBussWnd()
			break;

		case WM_COMMAND :
            switch(LOWORD(wParam))
            {
                case IDOK:
					return 0;

				case IDCANCEL:
					g_langpackMrgHwnd = NULL; // for proper toggle state report, see openCueBussWnd()
					ShowWindow(hwnd, SW_HIDE);
					return 0;

				case IDC_BUTTON1:
				{
					char curPath[BUFFER_SIZE]="";
					GetDlgItemText(hwnd,IDC_EDIT1,curPath,BUFFER_SIZE);
					if (!*curPath || !FileExists(curPath))
						_snprintf(curPath, BUFFER_SIZE, "%s%c%LangPack", GetResourcePath(), PATH_SLASH_CHAR);					
					if (char* fn = BrowseForFiles("S&M - Load LangPack file", curPath, NULL, false, SNM_TXT_EXT_LIST)) {
						SetDlgItemText(hwnd,IDC_SNM_CUEBUS_TEMPLATE,fn);
						free(fn);
					}
					break;
				}
			}
			break;

		case WM_DESTROY:
			SaveWindowPos(hwnd, cWndPosKey);
			break; 
	}
	return 0;
}

void openLangpackMrgWnd(COMMAND_T* _ct)
{
	static HWND hwnd = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_SNM_MERGE_LANGPACK), GetMainHwnd(), LangpackMergeDlgProc);
	if (g_langpackMrgHwnd) {
		g_langpackMrgHwnd = NULL;
		ShowWindow(hwnd, SW_HIDE);
	}
	else {
		g_langpackMrgHwnd = hwnd;
		ShowWindow(hwnd, SW_SHOW);
		SetFocus(hwnd);
	}
}

bool isLangpackMrgWndDisplayed(COMMAND_T* _ct) {
	return (g_langpackMrgHwnd && IsWindow(g_langpackMrgHwnd) && IsWindowVisible(g_langpackMrgHwnd) ? true : false);
}

#endif