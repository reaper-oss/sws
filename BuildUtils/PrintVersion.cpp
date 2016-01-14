/******************************************************************************
/ PrintVersion.cpp
/
/ A little win32 console application to print the version string from a
/ specifically formatted header file, used for SWS extension release.
/
/ Copyright (c) 2010 Tim Payne (SWS)
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
#include <windows.h>

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "Usage: PrintVersion version.h [-d]ate [format_string]\n\n");
		fprintf(stderr, "Prints the version # from a version.h file.\n");
		return 1;
	}

	FILE* pF;
	fopen_s(&pF, argv[1], "r");
	if (!pF)
	{
		fprintf(stderr, "PrintVersion: file %s not found.\n", argv[1]);
		return 2;
	}

	// Read the entire file into a buffer
	char cBuf[4096];
	fseek(pF, 0L, SEEK_END);
	int iOrigSize = ftell(pF);
	if (iOrigSize < 7 || (int)sizeof(cBuf) <= iOrigSize)
	{
		fprintf(stderr, "PrintVersion: invalid file %s.\n", argv[1]);
		return 3;
	}
	fseek(pF, 0L, SEEK_SET);
	fread(cBuf, sizeof(char), iOrigSize, pF);
	fclose(pF);
	cBuf[iOrigSize] = 0;

	char* pVersion = strstr(cBuf, "#define");
	int iVersion[4];
	int iVersionIdx = 0;
	while (iVersionIdx < 4)
	{
		// Go to the next digit
		while(pVersion && *pVersion && !isdigit(*pVersion)) pVersion++;
		if (*pVersion == 0) {
			fprintf(stderr, "PrintVersion: #define with version not found.\n");
			return 3;
		}
		iVersion[iVersionIdx++] = atoi(pVersion);
		
		// Go to the next non-digit
		while(isdigit(*pVersion) && *pVersion) pVersion++;
	}

	// Custom format string?
	int i=2; for(i; i<argc; i++) if(_stricmp(argv[i], "-d")) break; // not the optional date?
	if (i<argc)
		printf(argv[i], iVersion[0], iVersion[1], iVersion[2], iVersion[3]);
	else
		printf("v%d.%d.%d #%d", iVersion[0], iVersion[1], iVersion[2], iVersion[3]);

	// Optional date (in English whatever are regional settings and supported languages)
	i=2; for (i; i<argc; i++) if (!_stricmp(argv[i], "-d")) break;
	if (i<argc)
	{
		char pDate[128] = "";
		GetDateFormat(LOCALE_USER_DEFAULT, 0, NULL, "M", pDate, sizeof(pDate));
		if (int m = atoi(pDate))
		{
			const char cMonths[][16] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
			GetDateFormat(LOCALE_USER_DEFAULT, 0, NULL, "d, yyyy", pDate, sizeof(pDate));
			printf(" (%s %s)", cMonths[m-1], pDate);
		}
	}

	printf("\n");
	return 0;
}