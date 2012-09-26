/******************************************************************************
/ SnM_VWnd.cpp
/
/ Copyright (c) 2012 Jeffos
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
#include "SnM.h"


///////////////////////////////////////////////////////////////////////////////
// SNM_ToolbarButton
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


///////////////////////////////////////////////////////////////////////////////
// SNM_ImageVWnd
///////////////////////////////////////////////////////////////////////////////

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


///////////////////////////////////////////////////////////////////////////////
// SNM_AddDelButton
///////////////////////////////////////////////////////////////////////////////

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
// VWnd helpers
///////////////////////////////////////////////////////////////////////////////

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
	if (it && it->toolbar_blank)
	{
		skin.image = it->toolbar_blank;
		skin.olimage = it->toolbar_overlay; // might be NULL!
		WDL_VirtualIconButton_PreprocessSkinConfig(&skin);

		//JFB!!! most stupid hack since WDL 65568bc (overlay = main image size)
		for (int i=0; i<4; i++)
			skin.image_ltrb_ol[i] = 0;

		_btn->SetIcon(&skin);
		_btn->SetForceBorder(false);
		_btn->SetForceText(true); // do not force colors (done in SNM_ToolbarButton::OnPaintOver())
		_btn->SetTextLabel(_text, 0, SNM_GetToolbarFont());
	}
	else 
	{
		_btn->SetIcon(NULL); // important: would crash when switching theme..
		_btn->SetTextLabel(_text, 0, SNM_GetThemeFont());
		_btn->SetForceBorder(true);
	}
}

//JFB TODO? WDL_VWnd? hyperlink?
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

// SWS: My OSX has a problem with max() sometimes.  I don't know why.
#ifndef _WIN32
#define max(x,y) ((x)>(y)?(x):(y))
#endif

// auto position a WDL_VWnd instance
// note: by default all components are hidden, see WM_PAINT in sws_wnd.cpp
// _x: in/out param that gets modified (for the next component to be displayed)
// _h: height of the destination panel
// returns false if hidden
// TODO? WDL_VWnd inheritance rather than checking for inherited types
//       e.g. adding some kind of getPreferedWidthHeight(int* _width, int* _height)
bool SNM_AutoVWndPosition(WDL_VWnd* _comp, WDL_VWnd* _tiedComp, const RECT* _r, int* _x, int _y, int _h, int _xRoom)
{
	if (_comp && _h && abs(_r->bottom-_r->top) >= _h)
	{
		int width=0, height=0; //JFB!!! height used to be _h!
		if (!strcmp(_comp->GetType(), "vwnd_statictext"))
		{
			WDL_VirtualStaticText* txt = (WDL_VirtualStaticText*)_comp;
			RECT tr = {0,0,0,0};
			txt->GetFont()->DrawText(NULL, txt->GetText(), -1, &tr, DT_CALCRECT);
			width = tr.right;
			height = tr.bottom;
		}
		else if (!strcmp(_comp->GetType(), "vwnd_combobox"))
		{
			WDL_VirtualComboBox* cb = (WDL_VirtualComboBox*)_comp;
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
		else if (!strcmp(_comp->GetType(), "vwnd_iconbutton") || !strcmp(_comp->GetType(), "SNM_ToolbarButton"))
		{
			WDL_VirtualIconButton* btn = (WDL_VirtualIconButton*)_comp;
			WDL_VirtualIconButton_SkinConfig* skin = btn->GetIcon();
			if (skin && skin->image)
			{
				width = skin->image->getWidth() / 3;
				height = skin->image->getHeight();
				if (!strcmp(_comp->GetType(), "SNM_ToolbarButton")) {
					width = int(2.6*width); // larger toolbar buttons!
					height = int(0.75*height + 0.5) + 1; // +1 for text vertical alignment
				}
				// .. can be changed below! (i.e. adapt to larger text)
			}
			if (btn->GetFont())
			{
				RECT tr = {0,0,0,0};
				btn->GetFont()->DrawText(NULL, btn->GetTextLabel(), -1, &tr, DT_CALCRECT);
				if (tr.bottom > height)
					height = int(tr.bottom + tr.bottom/2 + 0.5); // unlikely to happen..
				if ((tr.right+int(height/2 + 0.5)) > width)
				width = int(tr.right + height/2 + 0.5); // +height/2 for some air
				if (btn->GetCheckState() != -1) {
					width += tr.bottom; // for the tick zone
					height -= 2;
				}
			}
		}
		else  if (!strcmp(_comp->GetType(), "SNM_MiniAddDelButtons")) {
			width=9;
			height=9*2+1;
		}
		else if (!strcmp(_comp->GetType(), "SNM_MiniKnob")) {
			width=height=25;
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
		RECT tr = {*_x, _y, *_x + width, _y + height};
		_comp->SetPosition(&tr);
		*_x = tr.right + _xRoom;
		_comp->SetVisible(true);
		return true;
	}

	if (_tiedComp && _tiedComp->IsVisible())
		_tiedComp->SetVisible(false);

	return false;
}
