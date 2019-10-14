/******************************************************************************
/ padreRmeTotalmix.h
/
/ Copyright (c) 2009-2010 Tim Payne (SWS), Jeffos (S&M), P. Bourdon
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

#pragma once

enum RmeTotalmixCmd {	eTOTALMIX_LOADUSER1, eTOTALMIX_LOADUSER2, eTOTALMIX_LOADUSER3, eTOTALMIX_LOADUSER4,
						eTOTALMIX_LOADUSER5, eTOTALMIX_LOADUSER6, eTOTALMIX_LOADUSER7, eTOTALMIX_LOADUSER8,
						eTOTALMIX_LOADFACT1, eTOTALMIX_LOADFACT2, eTOTALMIX_LOADFACT3, eTOTALMIX_LOADFACT4,
						eTOTALMIX_LOADFACT5, eTOTALMIX_LOADFACT6, eTOTALMIX_LOADFACT7, eTOTALMIX_LOADFACT8,
						eTOTALMIX_MASTERMUTE
					};

void RmeTotalmix(RmeTotalmixCmd cmd);


