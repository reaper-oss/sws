/******************************************************************************
/ IncVersion.cpp
/
/ A little win32 console application to increment VC-style RC version #s
/ in a header file.  This is used in the Release build of the SWS extension.
/
/ Copyright (c) 2009 Tim Payne (SWS)
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "../../wdl/lineparse.h"

bool GetSWSVersion(int* _v)
{
	FILE* pF;
	fopen_s(&pF, "sws\\version.h", "r+b");
	if (!pF)
		return false;

	// Get the file size
	/* Get the number of bytes */
	fseek(pF, 0L, SEEK_END);
	int iOrigSize = ftell(pF);
	char* cBuf = new char[iOrigSize+10];
	fseek(pF, 0L, SEEK_SET);
	fread(cBuf, sizeof(char), iOrigSize, pF);
	fseek(pF, 0L, SEEK_SET);
	cBuf[iOrigSize] = 0;

	bool bOnDefineLine = false;
	int iNumCommas = 0;
	while(*cBuf)
	{
		if (*cBuf == '\n')
		{
			bOnDefineLine = false;
			iNumCommas = 0;
		}
		else if (strncmp(cBuf, "#define", 7) == 0)
			bOnDefineLine = true;

		if (bOnDefineLine && isdigit(*cBuf)) {
			_v[iNumCommas] = atoi(cBuf);
			if (iNumCommas == 3)
				return true;
		}
		else if (bOnDefineLine && *cBuf == ',')
			iNumCommas++;
		cBuf++;
	}

	fclose(pF);
	return false;
}