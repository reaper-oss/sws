/******************************************************************************
/ MakeWhatsNew.cpp
/
/ A little win32 console application to parse a What's New file (.txt) and
/ create a formatted HTML file.
/ This file is also used in the main SWS project so the code must be portable
/
/ Copyright (c) 2010-2012 Tim Payne (SWS), Jeffos
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
#include <WDL/ptrlist.h>
#include <WDL/wdlcstring.h>
#include <WDL/wdlstring.h>

class IntStack
{
public:
	WDL_PtrList<void> m_stack;
	INT_PTR Top() { if (m_stack.GetSize()) return (INT_PTR)m_stack.Get(m_stack.GetSize()-1); else return 0; }
	void Push(INT_PTR i) { m_stack.Add((void*)i); }
	INT_PTR Pop() { INT_PTR i = (INT_PTR)m_stack.Get(m_stack.GetSize()-1); m_stack.Delete(m_stack.GetSize()-1); return i; }
	int Size() { return m_stack.GetSize(); }
};

// Write the file back to the output with formatting
// Formatting is:
// Line starting with ! : Make into a header line
// Line starting with + : Make into bullet
// URLs are converted into <a href>.  If you suffix a link with |, the following text will be used as the desc text
// Text "issue %d" is converted into a link into the tracker
int GenHtmlWhatsNew(const char* fnIn, const char* fnOut, bool bFullHTML, const char* _url)
{
	FILE* pIn;
	FILE* pOut;
#ifdef _WIN32
	fopen_s(&pIn, fnIn, "r");
#else
	pIn = fopen(fnIn, "r");
#endif
	if (!pIn)
	{
		fprintf(stderr, "Input file %s not found.\n", fnIn);
		return 2;
	}
	if (*fnOut)
	{
#ifdef _WIN32
		fopen_s(&pOut, fnOut, "wb");
#else
		pOut = fopen(fnOut, "w");
#endif
		if (!pOut)
		{
			fprintf(stderr, "Output file %s not found.\n", fnOut);
			return 3;
		}
	}
	else
		pOut = stdout;

	// Read the entire file into a buffer
	fseek(pIn, 0L, SEEK_END);
	int iSize = ftell(pIn); // Doesn't subtract out the text conversion, but allocate that size anyway
	char* cBuf = new char[iSize+2];
	fseek(pIn, 0L, SEEK_SET);
	iSize = (int)fread(cBuf, sizeof(char), iSize, pIn);
	fclose(pIn);
	// Ensure newline termination
	if (cBuf[iSize-1] != '\n')
		cBuf[iSize++] = '\n';
	cBuf[iSize] = 0;

	int iPos = 0;

// #ifndef _WIN32 //JFB commented: needed on Win too
	// get rid of '\r'
	int iPos2 = 0;
	char* cBuf2 = new char[iSize+2];
	while (cBuf[iPos] && iPos < iSize) {
		if (cBuf[iPos] != '\r') {
			cBuf2[iPos2] = cBuf[iPos];
			iPos2++;
		}
		iPos++;
	}
	iSize = iPos2;
	cBuf2[iSize] = 0;
	delete [] cBuf;
	cBuf = cBuf2;
	iPos = 0;
//#endif

	const INT_PTR NO_SECTION	= 0;
	const INT_PTR HEADER		= 1;
	const INT_PTR URL			= 2;
	const INT_PTR URL_STRING	= 3;
	const INT_PTR BULLET		= 4;
	const INT_PTR PARAGRAPH		= 6;
	IntStack curSection;
	WDL_String url;

	// Insert header if desired
	if (bFullHTML)
	{
		fputs(R"(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8"/>
  <title>SWS/S&amp;M Extension - What's new?</title>
  <link rel="stylesheet" href="https://fonts.googleapis.com/css?family=Yanone+Kaffeesatz:700"/>
  <style>
  body {
    background-color: #1c120f;
    color: #c6c3c3;
    font: 12px/18px "HelveticaNeue", "Helvetica Neue", Helvetica, Arial, sans-serif;
  }
  h1, h3, a {
    color: #feb56f;
    text-decoration: none;
    text-shadow: 1px 1px 2px black;
  }
  h1, h3 { font-family: "Yanone Kaffeesatz", sans-serif; }
  h1 { font-size: 3em; }
  h3 { font-size: 2em; display: block }
  a:hover, a:focus { color: #ffcfa1; }
  hr { border: 2px solid #2b2626; }
  ul { margin: 0; }
  </style>
</head>
<body>
<h1>)", pOut);
		if (_url)
		{
			fputs("<a href=\"", pOut);
			fputs(_url, pOut);
			fputs("\">", pOut);
		}
		fputs("SWS/S&amp;M Extension", pOut);
		if (_url)
		{
			fputs("</a>", pOut);
		}
		fputs(" - What's new?</h1>\n", pOut);
	}
	else
		fputs("<h1>What's new?</h1><br />\n", pOut);


	while (cBuf[iPos] && iPos < iSize)
	{
		if (cBuf[iPos] != '\n' && curSection.Top() != URL)
		{
			// Start of line must be a paragraph, a heading, or a list
			if (curSection.Top() == NO_SECTION)
			{
				if (cBuf[iPos] == '!')
				{
					if (iPos)
						fputs("<hr />", pOut);
					fputs("<h3>", pOut);
					curSection.Push(HEADER);
					iPos++;
				}
				else if (cBuf[iPos] == '+'
					&& (iPos == 0 || cBuf[iPos-1] == '\n')) // '+' at start of file/line?
				{
					fputs("<ul>\n", pOut);
					curSection.Push(BULLET);
				}
				else
					curSection.Push(PARAGRAPH);
			}
			
			if (curSection.Top() == BULLET && cBuf[iPos] == '+'
				&& (iPos == 0 || cBuf[iPos-1] == '\n'))  // '+' at start of file/line?
			{
				fputs("    <li>", pOut);
				iPos++;
			}

			if (!strncmp(&cBuf[iPos], "http://", 7) || !strncmp(&cBuf[iPos], "https://", 8))
			{
				fputs("<a href=\"", pOut);
				curSection.Push(URL);
				url.SetLen(0);
			}

			if (strnicmp(&cBuf[iPos], "issue ", 6) == 0)
			{
				int iIssue = atol(&cBuf[iPos+6]);
				if (iIssue != 0)
				{
					fprintf(pOut, "<a href=\"https://github.com/reaper-oss/sws/issues/%d\">%cssue %d</a>", iIssue, cBuf[iPos], iIssue);
					iPos += 6;
					while (isalnum(cBuf[iPos++]));
					iPos--;
				}
			}
		}
		else if (curSection.Top() == URL)
		{	// URL
			if (cBuf[iPos] == '|')
			{
				fputs("\">", pOut);
				curSection.Pop();
				curSection.Push(URL_STRING);
				iPos++;
			}
			else if (cBuf[iPos] == ' ' || cBuf[iPos] == '\n')
			{
				fprintf(pOut, "\">%s</a>", url.Get());
				curSection.Pop();
			}
		}
		
		if (cBuf[iPos] == '|' && curSection.Top() == URL_STRING)
		{
			fputs("</a>", pOut);
			curSection.Pop();
			iPos++;
		}
		
		// Close out tags at end of lines
		if (cBuf[iPos] == '\n')
		{
			bool bContinue = false;
			while(!bContinue && curSection.Size())
			{
				switch(curSection.Top())
				{
				case HEADER:
					fputs("</h3>\n", pOut);
					if (!bFullHTML)
						fputs("<br />\n", pOut);
					curSection.Pop();
					break;
				case BULLET:
					if (cBuf[iPos+1] != ' ')
					{
						fputs("</li>\n", pOut);
						if (cBuf[iPos+1] != '+')
						{
							fputs("</ul>\n", pOut);
							if (cBuf[iPos+1] == '\n')
								fputs("<br />", pOut);
							fputs("\n", pOut);
							curSection.Pop();
						}
					}
					else
						fputs("<br />\n", pOut);
					bContinue = true;
					break;
				case URL:
				case URL_STRING:
					fputs("</a>", pOut);
					curSection.Pop();
					break;
				case PARAGRAPH:
					fputs("<br />\n", pOut);
					if (cBuf[iPos+1] == '\n' || cBuf[iPos+1] == '+' || cBuf[iPos+1] == 0)
					{
						if (cBuf[iPos+1] != '+')
							fputs("<br />\n", pOut);
						curSection.Pop();
						if (cBuf[iPos+1] != '+')
							iPos++;
					}
					else
					{
						bContinue = true;
					}
					break;
				}
			}
		}
		// Special cases for <strong></strong>
		else if (strnicmp(&cBuf[iPos], "<strong>", 8) == 0)
		{
			fputs("<strong>", pOut);
			iPos += 7;
		}
		else if (strnicmp(&cBuf[iPos], "</strong>", 9) == 0)
		{
			fputs("</strong>", pOut);
			iPos += 8;
		}
		else // "Default" case, just write out the character
		{
			if (cBuf[iPos] == '\"')
				fputs("&quot;", pOut);
			else if (cBuf[iPos] == '&')
				fputs("&amp;", pOut);
			else if (cBuf[iPos] == '>')
				fputs("&gt;", pOut);
			else if (cBuf[iPos] == '<')
				fputs("&lt;", pOut);
			else if (cBuf[iPos] == ' ' && cBuf[iPos+1] == ' ')
				fputs("&nbsp;", pOut);
			else
				fwrite(&cBuf[iPos], 1, 1, pOut);
			
			// If outputting URL, save the text in case we haven't specified replacement URL text
			if (curSection.Top() == URL)
				url.AppendFormatted(500, "%c", cBuf[iPos]);
		}

		iPos++;
	}

	if (bFullHTML)
		fputs("</body></html>\n", pOut);

	if (pOut != stdout)
		fclose(pOut);
	delete [] cBuf;

	return 0;
}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "Usage: MakeWhatsNew [-h] whatsnew.txt [new.html]\n\n");
		fprintf(stderr, "Creates an HTML text file from a whatsnew.txt.  Use -h to write full html header/footer tags.\n");
		return 1;
	}

	bool bFullHTML = false;
	int iNextArg = 1;
	if (argc > 2 && strcmp(argv[1], "-h") == 0)
	{
		bFullHTML = true;
		iNextArg++;
	}

	char fnIn[2048]="", fnOut[2048]="";
	lstrcpyn_safe(fnIn, argv[iNextArg], sizeof(fnIn));
	iNextArg++;

	if (argc == iNextArg + 1)
		lstrcpyn_safe(fnOut, argv[iNextArg], sizeof(fnOut));

	return GenHtmlWhatsNew(fnIn, fnOut, bFullHTML, NULL);
}
