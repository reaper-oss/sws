/******************************************************************************
/ DragDrop.cpp
/
/ Copyright (c) 2009 Tim Payne (SWS)
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

class SWS_IDataObject : public IDataObject
{
public:
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppvObject) 
	{
		// check to see what interface has been requested
		if(iid == IID_IDataObject || iid == IID_IUnknown)
		{
			AddRef();
			*ppvObject = this;
			return S_OK;
		}
		else
		{
			*ppvObject = 0;
			return E_NOINTERFACE;
		}
	}
	ULONG STDMETHODCALLTYPE AddRef() { return ++m_iRef; }
	ULONG STDMETHODCALLTYPE Release()
	{
		--m_iRef;
		if (!m_iRef)
		{
			delete this;
			return 0;
		}
		return m_iRef;
	}
	HRESULT STDMETHODCALLTYPE GetData(FORMATETC *pformatetcIn, STGMEDIUM *pMedium)
	{
		if (MatchFormatEtc(pformatetcIn))
			return DV_E_FORMATETC;
		pMedium->tymed = m_pFormatEtc->tymed;
		pMedium->pUnkForRelease = 0;
		if (m_pFormatEtc->tymed != TYMED_HGLOBAL)
			return DV_E_FORMATETC;
		DWORD len    = GlobalSize(m_pStgMedium->hGlobal);
		PVOID source = GlobalLock(m_pStgMedium->hGlobal);
		PVOID dest   = GlobalAlloc(GMEM_FIXED, len);
		memcpy(dest, source, len);
		GlobalUnlock(m_pStgMedium->hGlobal);
		pMedium->hGlobal = dest;
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium) { return DATA_E_FORMATETC; }
	HRESULT STDMETHODCALLTYPE QueryGetData(FORMATETC *pformatetc)
	{
		if (MatchFormatEtc(pformatetc))
			return DV_E_FORMATETC;
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(FORMATETC *pFormatEctIn, FORMATETC *pFormatEtcOut)
	{
		pFormatEtcOut->ptd = NULL;
		return E_NOTIMPL;
	}
	HRESULT STDMETHODCALLTYPE SetData(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease) { return E_NOTIMPL; }
	HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc) { return 0; }
	HRESULT STDMETHODCALLTYPE DAdvise(FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection) { return OLE_E_ADVISENOTSUPPORTED; }
	HRESULT STDMETHODCALLTYPE DUnadvise(DWORD dwConnection) { return OLE_E_ADVISENOTSUPPORTED; }
	HRESULT STDMETHODCALLTYPE EnumDAdvise(IEnumSTATDATA **ppenumAdvise) { return OLE_E_ADVISENOTSUPPORTED; }
	SWS_IDataObject(FORMATETC *fmtetc, STGMEDIUM *stgmed)
	{
		m_iRef = 1;
		m_pFormatEtc  = new FORMATETC;
		m_pStgMedium  = new STGMEDIUM;
		m_pFormatEtc = fmtetc;
		m_pStgMedium = stgmed;
	}
    ~SWS_IDataObject()
	{
		delete m_pFormatEtc;
		delete m_pStgMedium;
	}

private:
	int MatchFormatEtc(FORMATETC *pFormatEtc)
	{
		if((pFormatEtc->tymed    &  m_pFormatEtc->tymed)   &&
			pFormatEtc->cfFormat == m_pFormatEtc->cfFormat && 
			pFormatEtc->dwAspect == m_pFormatEtc->dwAspect)
		{
			return 0;
		}
		return -1;
	}

	ULONG m_iRef;
	FORMATETC* m_pFormatEtc;
	STGMEDIUM* m_pStgMedium;
};

class SWS_IDropSource : public IDropSource
{
public:
	HRESULT STDMETHODCALLTYPE QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState) { return 0; }
	HRESULT STDMETHODCALLTYPE GiveFeedback(DWORD dwEffect) { return 0; }
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) { return 0; }
	ULONG STDMETHODCALLTYPE AddRef() { return 0; }
	ULONG STDMETHODCALLTYPE Release() { return 0; }
};

void SWS_MediaPoolFileView::OnBeginDrag()
{
	SWS_IDataObject dataObj(...);
	SWS_IDropSource dropSrc;
	DWORD effect;
	LVITEM li;
	li.mask = LVIF_STATE | LVIF_PARAM;
	li.stateMask = LVIS_SELECTED;
	li.iSubItem = 0;
	
	// Get the amount of memory needed for the file list
	int iMemNeeded = 0;
	for (int i = 0; i < ListView_GetItemCount(m_hwndList); i++)
	{
		li.iItem = i;
		ListView_GetItem(m_hwndList, &li);
		if (li.state & LVIS_SELECTED)
			iMemNeeded += strlen(((SWS_MediaPoolFile*)li.lParam)->m_cFilename) + 1;
	}
	if (!iMemNeeded)
		return;

	iMemNeeded += sizeof(DROPFILES) + 1;

	HGLOBAL hgDrop = GlobalAlloc (GHND | GMEM_SHARE, iMemNeeded);
	DROPFILES* pDrop = (DROPFILES*)GlobalLock(hgDrop); // 'spose should do some error checking...
	pDrop->pFiles = sizeof(DROPFILES);
	pDrop->fWide = false;
	char* pBuf = (char*)(pDrop + pDrop->pFiles);

	// Add the files to the DROPFILES struct, double-NULL terminated
	for (int i = 0; i < ListView_GetItemCount(m_hwndList); i++)
	{
		li.iItem = i;
		ListView_GetItem(m_hwndList, &li);
		if (li.state & LVIS_SELECTED)
		{
			strcpy(pBuf, ((SWS_MediaPoolFile*)li.lParam)->m_cFilename);
			pBuf += strlen(pBuf) + 1;
		}
	}
	*pBuf = 0;
	GlobalUnlock(hgDrop);
	FORMATETC etc = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	dataObj.SetData(&etc, (STGMEDIUM*)hgDrop, false);

	int ret = DoDragDrop(&dataObj, &dropSrc, 0, &effect);
}
