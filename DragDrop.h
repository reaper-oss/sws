/******************************************************************************
/ DragDrop.h
/
/ Copyright (c) 2009 Tim Payne (SWS)
/ https://code.google.com/p/sws-extension
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

class SWS_IDataObject : public IDataObject
{
public:
	HRESULT STDMETHODCALLTYPE QueryInterface (REFIID iid, void ** ppvObject);
	ULONG	STDMETHODCALLTYPE AddRef (void);
	ULONG	STDMETHODCALLTYPE Release (void);
	HRESULT STDMETHODCALLTYPE GetData(FORMATETC *pformatetcIn, STGMEDIUM *pMedium);
	HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium) { return DATA_E_FORMATETC; }
	HRESULT STDMETHODCALLTYPE QueryGetData(FORMATETC *pformatetc);
	HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(FORMATETC *pFormatEctIn, FORMATETC *pFormatEtcOut);
	HRESULT STDMETHODCALLTYPE SetData(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease) { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc);
	HRESULT STDMETHODCALLTYPE DAdvise(FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection) { return OLE_E_ADVISENOTSUPPORTED; }
	HRESULT STDMETHODCALLTYPE DUnadvise(DWORD dwConnection) { return OLE_E_ADVISENOTSUPPORTED; }
	HRESULT STDMETHODCALLTYPE EnumDAdvise(IEnumSTATDATA **ppenumAdvise) { return OLE_E_ADVISENOTSUPPORTED; }

	SWS_IDataObject(FORMATETC *fmtetc, STGMEDIUM *stgmed);
    ~SWS_IDataObject();

private:
	int MatchFormatEtc(FORMATETC *pFormatEtc);
	LONG m_iRef;
	FORMATETC* m_pFormatEtc;
	STGMEDIUM* m_pStgMedium;
};

class SWS_IDropSource : public IDropSource
{
public:
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
	HRESULT STDMETHODCALLTYPE QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState);
	HRESULT STDMETHODCALLTYPE GiveFeedback(DWORD dwEffect) { return DRAGDROP_S_USEDEFAULTCURSORS; }
	SWS_IDropSource():m_iRef(1) {}
	~SWS_IDropSource() {}

private:
	LONG m_iRef;

};

class SWS_EnumFormatEtc : public IEnumFORMATETC
{
public:
	HRESULT STDMETHODCALLTYPE QueryInterface (REFIID iid, void ** ppvObject);
	ULONG	STDMETHODCALLTYPE AddRef (void);
	ULONG	STDMETHODCALLTYPE Release (void);
	HRESULT STDMETHODCALLTYPE Next(ULONG celt, FORMATETC * rgelt, ULONG * pceltFetched);
	HRESULT STDMETHODCALLTYPE Skip(ULONG celt); 
	HRESULT STDMETHODCALLTYPE Reset(void);
	HRESULT STDMETHODCALLTYPE Clone(IEnumFORMATETC ** ppEnumFormatEtc);
	SWS_EnumFormatEtc(FORMATETC *pFormatEtc);
	~SWS_EnumFormatEtc();

private:
	LONG m_iRef;
	FORMATETC* m_pFormatEtc;
	bool m_bEnum;
};
