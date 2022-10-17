/******************************************************************************
/ Base64.cpp
/
/ Copyright (c) 2009 Tim Payne (SWS)
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

#include "stdafx.h"
#include <stdlib.h>
#include <string.h>
#include "Base64.h"

// This following Base64 code adapted from http://base64.sourceforge.net/b64.c
// Copyright (c) 2001 Bob Trower, Trantor Standard Systems Inc.
// Visit above link for full license info or to get original source.
// Modified so that the '=' char returns zero for compat with other base64 systems.
static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char cd64[]="|$$$}rstuvwxyz{$$$>$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Base64::Base64()
{
	m_pEncodedBuf = NULL;
	m_pDecodedBuf = NULL;
}

Base64::~Base64()
{
	delete [] m_pEncodedBuf;
	delete [] m_pDecodedBuf;
}

//////////////////////////////////////////////////////////////////////
// Public Member Functions
//////////////////////////////////////////////////////////////////////
char* Base64::Encode(const char* pInput, int iInputLen, const bool pad)
{
	int iLen = iInputLen;
	int iEncodedLen;
	
	//calculate encoded buffer size
	if (pad)
		iEncodedLen = static_cast<int>(4 * ceil(iLen / 3.f));
	else
		iEncodedLen = static_cast<int>(ceil(4 * iLen / 3.f));

	// allocate:
	if (m_pEncodedBuf != NULL)
		free (m_pEncodedBuf);
	m_pEncodedBuf = new char[iEncodedLen + 1];
	char* pOutput = m_pEncodedBuf;

	//let's step through the buffer (in groups of three bytes) and encode it...
	while (iLen >= 3)
	{
		*(pOutput++) = cb64[(unsigned char)pInput[0] >> 2];
		*(pOutput++) = cb64[(((unsigned char)pInput[0] & 0x03) << 4) | (((unsigned char)pInput[1] & 0xF0) >> 4)];
		*(pOutput++) = cb64[(((unsigned char)pInput[1] & 0x0F) << 2) | (((unsigned char)pInput[2] & 0xC0) >> 6)];
		*(pOutput++) = cb64[(unsigned char)pInput[2] & 0x3F];
		iLen -= 3;
		pInput += 3;
	}

	//do we have some chars left?
	if (iLen != 0)
	{
		*(pOutput++) = cb64[(unsigned char)pInput[0] >> 2];

		if (iLen == 1)
		{
			*(pOutput++) = cb64[((unsigned char)pInput[0] & 0x03) << 4];
		}
		else // iLen == 2
		{
			*(pOutput++) = cb64[(((unsigned char)pInput[0] & 0x03) << 4) | (((unsigned char)pInput[1] & 0xF0) >> 4)];
			*(pOutput++) = cb64[(((unsigned char)pInput[1] & 0x0F) << 2)];
		}
	}

	while (pad && pOutput < m_pEncodedBuf + iEncodedLen)
		*(pOutput++) = '=';

	// Null terminate
	*pOutput = 0;
	
	return m_pEncodedBuf;
}

// Decode a base64 string to a binary buffer
// Encoded string must be null terminated
char* Base64::Decode(const char* pEncodedBuf, int *iOutLen)
{
	int iDecodedLen;
	int iLen, iBlock, i;
	if (iOutLen)
		*iOutLen = 0;

	// allocate buffer to hold the decoded string:
	const int iEncodedLen = strlen(pEncodedBuf);
	iDecodedLen = static_cast<int>(3 * (iEncodedLen / 4.f));

	// remove padding from decoded length
	for(int i = iEncodedLen - 1; i >= 0 && pEncodedBuf[i] == '='; --i, --iDecodedLen);

	if (m_pDecodedBuf != NULL)
		free (m_pDecodedBuf);
	m_pDecodedBuf = new char[iDecodedLen];

	// allocate a local scratch buffer for decoding - work with BYTE's to avoid fatal sign extensions by compiler:
	char* pInput = new char[iEncodedLen+1];
	strcpy(pInput, pEncodedBuf);

	// Loop for each byte of input:
	iLen = iBlock = i = 0;
	while (pInput[iBlock+i])
	{
		if ((unsigned char)pInput[iBlock+i] < 0x2B || (unsigned char)pInput[iBlock+i] > 0x7A)
		{
			delete [] pInput;
			return NULL;
		}
		if (pInput[iBlock+i] == '=')
			break;
		pInput[iBlock+i] = cd64[(unsigned char)pInput[iBlock+i] - 0x2B];
		if (pInput[iBlock+i] == '$')
		{
			delete [] pInput;
			return NULL;
		}
		pInput[iBlock+i] -= 0x3E;

		switch(i++)
		{
			// case 0: no data to copy yet!
		case 1:
			m_pDecodedBuf[iLen++] = ((unsigned char)pInput[iBlock+0] << 2 | (unsigned char)pInput[iBlock+1] >> 4);
			break;
		case 2:
			m_pDecodedBuf[iLen++] = ((unsigned char)pInput[iBlock+1] << 4 | (unsigned char)pInput[iBlock+2] >> 2);
			break;
		case 3:
			m_pDecodedBuf[iLen++] = ((((unsigned char)pInput[iBlock+2] << 6) & 0xC0) | (unsigned char)pInput[iBlock+3]);
			i = 0;
			iBlock += 4;
			break;
		}
	}

	// clean and check:
	delete [] pInput;
	if (iLen != iDecodedLen)
		return NULL;

	// done:
	if (iOutLen)
		*iOutLen = iLen;
	return m_pDecodedBuf;
}
