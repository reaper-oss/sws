/******************************************************************************
/ SnM_VWnd.h
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

#ifndef _SNM_VWND_H_
#define _SNM_VWND_H_

#define SNM_DEF_VWND_X_STEP			12


// slight override in order to ignore separators
//JFB!! would be great if WDL_VirtualComboBox's GetCurSel() and SetCurSel() were "virtual"
// => TODO when done: cleanup GetCurSel2->GetCurSel, etc..
class SNM_VirtualComboBox : public WDL_VirtualComboBox
{
public:
	SNM_VirtualComboBox() : WDL_VirtualComboBox() {}
	virtual ~SNM_VirtualComboBox() {}
	int GetCurSel2()
	{
		int sel = WDL_VirtualComboBox::GetCurSel();
		for (int i=sel; i>=0; i--)
			if (!strcmp("<SEP>", m_items.Get(i)))
				sel--;
		return sel;
	}
	void SetCurSel2(int _sel)
	{
		for (int i=0; i<m_items.GetSize(); i++) {
			if (!strcmp("<SEP>", m_items.Get(i))) _sel++;
			else if (i==_sel) break;
		}
		WDL_VirtualComboBox::SetCurSel(_sel);
	}
};

class SNM_DynSizedText : public WDL_VWnd {
public:
	SNM_DynSizedText() : WDL_VWnd()
	{
		m_fontName.Set("Arial"); // very default font, ok on osx/win..
		m_alpha=255;
		m_col=0;
		m_maxLineIdx=m_lastFontH=-1; 
		m_wantBorder=false;
		m_wantTitleLane=true;
		m_align=m_titleAlign=DT_CENTER;
		m_font.SetFromHFont(NULL,LICE_FONT_FLAG_OWNS_HFONT);
	}
	virtual const char *GetType() { return "SNM_DynSizedText"; }
	virtual void OnPaint(LICE_IBitmap* _drawbm, int _origin_x, int _origin_y, RECT* _cliprect, int rscale);
	virtual void SetText(const char* _txt, int _col = 0, unsigned char _alpha = 255);
	virtual void SetTitle(const char* _txt);
	virtual void SetBorder(bool _b) { m_wantBorder = _b; }
	virtual void SetAlign(UINT _mainAlign, UINT _titleAlign = DT_CENTER) { m_align=_mainAlign; m_titleAlign=_titleAlign; }
	virtual const char* GetFontName() { return m_fontName.Get(); }
	virtual void SetFontName(const char* _fontName) { m_fontName.Set(_fontName); }
	virtual bool HasTitleLane();
	virtual bool WantTitleLane() { return m_wantTitleLane; }
	virtual void SetWantTitleLane(bool _want) { m_wantTitleLane=_want; }

protected:
	virtual int GetTitleLaneHeight();
	void DrawLines(LICE_IBitmap* _drawbm, RECT* _r, int _fontHeight);

	LICE_CachedFont m_font;
	WDL_FastString m_title, m_fontName, m_lastText;
	WDL_PtrList_DeleteOnDestroy<WDL_FastString> m_lines;
	int m_maxLineIdx, m_lastFontH, m_col;
	bool m_wantBorder, m_wantTitleLane;
	unsigned char m_alpha;
	UINT m_align, m_titleAlign;
};

class SNM_FiveMonitors : public WDL_VWnd {
public:
	SNM_FiveMonitors() : WDL_VWnd(), m_nbRows(2) {}
	virtual const char *GetType() { return "SNM_FiveMonitors"; }
	virtual void AddMonitors(SNM_DynSizedText* _m0, SNM_DynSizedText* _m1, SNM_DynSizedText* _m2, SNM_DynSizedText* _m3, SNM_DynSizedText* _m4);
	virtual bool HasValidChildren();
	virtual void OnPaint(LICE_IBitmap* _drawbm, int _origin_x, int _origin_y, RECT* _cliprect, int rscale);
	virtual void SetText(int _monNum, const char* _txt, int _col = 0, unsigned char _alpha = 255);
	virtual void SetTitles(const char* _title1="1", const char* _title2="2", const char* _title3="3", const char* _title4="4");
	virtual void SetPosition(const RECT* _r);
	virtual void SetFontName(const char* _fontName);
	virtual void SetRows(int _nbRows);
	virtual int GetRows() { return m_nbRows; }
protected:
	int m_nbRows;
};

class SNM_ImageVWnd : public WDL_VWnd {
public:
	SNM_ImageVWnd(LICE_IBitmap* _img = NULL) : WDL_VWnd(), m_img(_img) {}
	virtual const char *GetType() { return "SNM_ImageVWnd"; }
	virtual const char* GetFilename() { return m_fn.Get(); }
	virtual int GetWidth();
	virtual int GetHeight();
	virtual void SetImage(const char* _fn);
	virtual void OnPaint(LICE_IBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect, int rscale);
protected:
	LICE_IBitmap* m_img;
	WDL_FastString m_fn;
};

//JFB TODO: hyperlink
class SNM_Logo : public SNM_ImageVWnd {
public:
	SNM_Logo();
	virtual const char *GetType() { return "SNM_Logo"; }
	virtual bool GetToolTipString(int xpos, int ypos, char* bufOut, int bufOutSz) { lstrcpyn(bufOut, "Strong & Mighty", bufOutSz); return true; }
};

class SNM_TinyButton : public WDL_VWnd {
public:
	SNM_TinyButton() : WDL_VWnd(), m_en(true) {}
	virtual const char *GetType() { return "SNM_TinyButton"; }
	virtual void OnPaint(LICE_IBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect, int rscale) {}
	virtual void SetEnabled(bool _en) { m_en=_en; }
	virtual int OnMouseDown(int xpos, int ypos) { return m_en?1:0;	}
	virtual void OnMouseUp(int xpos, int ypos) { if (m_en) SendCommand(WM_COMMAND,GetID(),0,this); }
protected:
	bool m_en;
};

class SNM_TinyPlusButton : public SNM_TinyButton {
public:
	SNM_TinyPlusButton() : SNM_TinyButton() {}
	virtual void OnPaint(LICE_IBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect, int rscale);
};

class SNM_TinyMinusButton : public SNM_TinyButton {
public:
	SNM_TinyMinusButton() : SNM_TinyButton() {}
	virtual void OnPaint(LICE_IBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect, int rscale);
};

class SNM_TinyLeftButton : public SNM_TinyButton {
public:
	SNM_TinyLeftButton() : SNM_TinyButton() {}
	virtual void OnPaint(LICE_IBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect, int rscale);
};

class SNM_TinyRightButton : public SNM_TinyButton {
public:
	SNM_TinyRightButton() : SNM_TinyButton() {}
	virtual void OnPaint(LICE_IBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect, int rscale);
};

class SNM_TinyTickBox : public SNM_TinyButton {
public:
	SNM_TinyTickBox() : SNM_TinyButton(), m_checkstate(0) {}
	virtual void OnPaint(LICE_IBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect, int rscale);
	void SetCheckState(char _state); // to mimic WDL_VirtualIconButton (eases replacement), 0=unchecked, 1=checked, -1 is unsupported
	char GetCheckState() { return m_checkstate; }
protected:
	char m_checkstate;
};

class SNM_TwoTinyButtons : public WDL_VWnd {
public:
	SNM_TwoTinyButtons() : WDL_VWnd() {}
	virtual const char *GetType() { return "SNM_TwoTinyButtons"; }
	virtual void SetPosition(const RECT* _r);
};

class SNM_ToolbarButton : public WDL_VirtualIconButton {
public:
	SNM_ToolbarButton() : WDL_VirtualIconButton() {}
	virtual const char *GetType() { return "SNM_ToolbarButton"; }
	virtual char GetPressed() { return m_pressed; }
	virtual void SetGrayed(bool _grayed);
};

class SNM_Knob : public WDL_VirtualSlider {
public:
	SNM_Knob() : WDL_VirtualSlider(), m_factor(1.0) {
		SetKnobBias(1, 3); // 1: force knob
		SetScrollMessage(WM_VSCROLL);
	}
	virtual const char *GetType() { return "SNM_Knob"; }
	virtual void SetRangeFactor(int _minr, int _maxr, int _center, double _factor) { m_factor=_factor; m_minr=int(_factor*_minr+0.5); m_maxr=int(_factor*_maxr+0.5); m_center=int(_factor*_center+0.5); }
	virtual int GetSliderPosition() { return int(WDL_VirtualSlider::GetSliderPosition()/m_factor + 0.5); }
	virtual void SetSliderPosition(int _pos) { WDL_VirtualSlider::SetSliderPosition(int(_pos*m_factor + 0.5)); }
protected:
	double m_factor; // for knob usability (especially with short ranges)
};

class SNM_KnobCaption : public WDL_VWnd {
public:
	SNM_KnobCaption() : WDL_VWnd(), m_value(-666) {}
	virtual void SetPosition(const RECT* _r);
	void DrawText(LICE_IBitmap* _drawbm, RECT* _rect, UINT _dtFlags);
	virtual void OnPaint(LICE_IBitmap *drawbm, int origin_x, int origin_y, RECT *cliprect, int rscale);
	virtual const char *GetType() { return "SNM_KnobCaption"; }
	virtual void SetTitle(const char* _txt) { m_title.Set(_txt); }
	virtual void SetSuffix(const char* _txt) { m_suffix.Set(_txt); }
	virtual void SetZeroText(const char* _txt) { m_zeroTxt.Set(_txt); }
	virtual void SetValue(int _value);
	virtual LICE_CachedFont* GetFont();
protected:
	int m_value;
	WDL_FastString m_title, m_suffix, m_zeroTxt;
};

// helpers
void SNM_SkinButton(WDL_VirtualIconButton* _btn, WDL_VirtualIconButton_SkinConfig* _skin, const char* _text);
void SNM_SkinToolbarButton(SNM_ToolbarButton* _btn, const char* _text);
void SNM_SkinKnob(SNM_Knob* _knob);
bool SNM_AddLogo(LICE_IBitmap* _bm, const RECT* _r, int _x = -1, int _h = -1);
bool SNM_AutoVWndPosition(UINT _align, WDL_VWnd* _comp, WDL_VWnd* _tiedComp, const RECT* _r, int* _x, int _y, int _h, int _xRoom = SNM_DEF_VWND_X_STEP);

#endif
