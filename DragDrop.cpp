/******************************************************************************
/ DragDrop.cpp
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

#include "stdafx.h"
#include "DragDrop.h"

HRESULT STDMETHODCALLTYPE SWS_IDataObject::QueryInterface(REFIID iid, void **ppvObject) 
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

ULONG STDMETHODCALLTYPE SWS_IDataObject::AddRef()
{
	return InterlockedIncrement(&m_iRef);
}

ULONG STDMETHODCALLTYPE SWS_IDataObject::Release()
{
	InterlockedDecrement(&m_iRef);
	if (!m_iRef)
	{
		delete this;
		return 0;
	}
	return m_iRef;
}

HRESULT STDMETHODCALLTYPE SWS_IDataObject::GetData(FORMATETC *pformatetcIn, STGMEDIUM *pMedium)
{
	if (MatchFormatEtc(pformatetcIn))
		return DV_E_FORMATETC;
	pMedium->tymed = m_pFormatEtc->tymed;
	pMedium->pUnkForRelease = 0;
	if (m_pFormatEtc->tymed != TYMED_HGLOBAL)
		return DV_E_FORMATETC;
	DWORD len    = (DWORD)GlobalSize(m_pStgMedium->hGlobal);
	PVOID source = GlobalLock(m_pStgMedium->hGlobal);
	PVOID dest   = GlobalAlloc(GMEM_FIXED, len);
	memcpy(dest, source, len);
	GlobalUnlock(m_pStgMedium->hGlobal);
	pMedium->hGlobal = dest;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE SWS_IDataObject::QueryGetData(FORMATETC *pformatetc)
{
	if (MatchFormatEtc(pformatetc))
		return DV_E_FORMATETC;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE SWS_IDataObject::GetCanonicalFormatEtc(FORMATETC *pFormatEctIn, FORMATETC *pFormatEtcOut)
{
	pFormatEtcOut->ptd = NULL;
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE SWS_IDataObject::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppEnumFormatEtc)
{
	*ppEnumFormatEtc = new SWS_EnumFormatEtc(m_pFormatEtc);
	return S_OK;
}

SWS_IDataObject::SWS_IDataObject(FORMATETC *fmtetc, STGMEDIUM *stgmed)
{
	m_iRef = 1;
	m_pFormatEtc  = new FORMATETC;
	m_pStgMedium  = new STGMEDIUM;
	*m_pFormatEtc = *fmtetc;
	*m_pStgMedium = *stgmed;
}

SWS_IDataObject::~SWS_IDataObject()
{
	delete m_pFormatEtc;
	delete m_pStgMedium;
}

int SWS_IDataObject::MatchFormatEtc(FORMATETC *pFormatEtc)
{
	if((pFormatEtc->tymed    &  m_pFormatEtc->tymed)   &&
		pFormatEtc->cfFormat == m_pFormatEtc->cfFormat && 
		pFormatEtc->dwAspect == m_pFormatEtc->dwAspect)
	{
		return 0;
	}
	return -1;
}

HRESULT STDMETHODCALLTYPE SWS_IDropSource::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
	// if the <Escape> key has been pressed since the last call, cancel the drop
	if(fEscapePressed == TRUE)
		return DRAGDROP_S_CANCEL;	
	// if the <LeftMouse> button has been released, then do the drop!
	if((grfKeyState & MK_LBUTTON) == 0)
		return DRAGDROP_S_DROP;
	// continue with the drag-drop
	return S_OK;
}

HRESULT STDMETHODCALLTYPE SWS_IDropSource::QueryInterface(REFIID riid, void **ppvObject)
{
    // check to see what interface has been requested
    if(riid == IID_IDropSource || riid == IID_IUnknown)
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


ULONG STDMETHODCALLTYPE SWS_IDropSource::AddRef()
{
	return InterlockedIncrement(&m_iRef);
}

ULONG STDMETHODCALLTYPE SWS_IDropSource::Release()
{
	InterlockedDecrement(&m_iRef);
	if (!m_iRef)
	{
		delete this;
		return 0;
	}
	return m_iRef;
}

SWS_EnumFormatEtc::SWS_EnumFormatEtc(FORMATETC *pFormatEtc)
{
	m_iRef = 1;
	m_bEnum = false;
	m_pFormatEtc = new FORMATETC;
	*m_pFormatEtc = *pFormatEtc;
	if (pFormatEtc->ptd)
	{
		m_pFormatEtc->ptd = (DVTARGETDEVICE*)CoTaskMemAlloc(sizeof(DVTARGETDEVICE));
		*(m_pFormatEtc->ptd) = *(pFormatEtc->ptd);
	}
}

SWS_EnumFormatEtc::~SWS_EnumFormatEtc()
{
	if(m_pFormatEtc)
	{
		if(m_pFormatEtc->ptd)
			CoTaskMemFree(m_pFormatEtc->ptd);
		delete m_pFormatEtc;
	}
}

ULONG STDMETHODCALLTYPE SWS_EnumFormatEtc::AddRef(void)
{
	return InterlockedIncrement(&m_iRef);
}

ULONG STDMETHODCALLTYPE SWS_EnumFormatEtc::Release(void)
{
	InterlockedDecrement(&m_iRef);
	if (!m_iRef)
	{
		delete this;
		return 0;
	}
	return m_iRef;
}

HRESULT STDMETHODCALLTYPE SWS_EnumFormatEtc::QueryInterface(REFIID iid, void **ppvObject)
{
    // check to see what interface has been requested
    if(iid == IID_IEnumFORMATETC || iid == IID_IUnknown)
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

HRESULT STDMETHODCALLTYPE SWS_EnumFormatEtc::Next(ULONG celt, FORMATETC *pFormatEtc, ULONG * pceltFetched)
{
	ULONG copied = 0;

	// validate arguments
	if(celt == 0 || pFormatEtc == 0)
		return E_INVALIDARG;

	if (!m_bEnum)
	{
		*pFormatEtc = *m_pFormatEtc;
		if (pFormatEtc->ptd)
		{
			pFormatEtc->ptd = (DVTARGETDEVICE*)CoTaskMemAlloc(sizeof(DVTARGETDEVICE));
			*(pFormatEtc->ptd) = *(m_pFormatEtc->ptd);
		}
		m_bEnum = true;
		++copied;
	}

	if(pceltFetched != 0) 
		*pceltFetched = copied;

	// did we copy all that was requested?
	return (copied == celt) ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE SWS_EnumFormatEtc::Skip(ULONG celt)
{
	if (m_bEnum && celt)
		return S_FALSE;
	else if (celt)
		m_bEnum = true;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE SWS_EnumFormatEtc::Reset(void)
{
	m_bEnum = false;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE SWS_EnumFormatEtc::Clone(IEnumFORMATETC ** ppEnumFormatEtc)
{
	SWS_EnumFormatEtc* pClone = new SWS_EnumFormatEtc(m_pFormatEtc);
	pClone->m_bEnum = m_bEnum;
	return S_OK;
}
