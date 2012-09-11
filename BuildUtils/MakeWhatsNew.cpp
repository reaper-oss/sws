/******************************************************************************
/ MakeWhatsNew.cpp
/
/ A little win32 console application to parse a What's New file (.txt) and
/ create a formatted HTML file.
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
#include "../../WDL/ptrlist.h"
#include "../../WDL/wdlstring.h"

class IntStack
{
public:
	WDL_PtrList<void> m_stack;
	int Top() { if (m_stack.GetSize()) return (int)m_stack.Get(m_stack.GetSize()-1); else return 0; }
	void Push(int i) { m_stack.Add((void*)i); }
	int Pop() { int i = (int)m_stack.Get(m_stack.GetSize()-1); m_stack.Delete(m_stack.GetSize()-1); return i; }
	int Size() { return m_stack.GetSize(); }
};

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

	FILE* pIn;
	FILE* pOut;
	fopen_s(&pIn, argv[iNextArg], "r");
	if (!pIn)
	{
		fprintf(stderr, "Input file %s not found.\n", argv[iNextArg]);
		return 2;
	}
	iNextArg++;
	if (argc == iNextArg + 1)
	{
		fopen_s(&pOut, argv[iNextArg], "w");
		if (!pOut)
		{
			fprintf(stderr, "Output file %s not found.\n", argv[iNextArg]);
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
	iSize = fread(cBuf, sizeof(char), iSize, pIn);
	fclose(pIn);
	// Ensure newline termination
	if (cBuf[iSize-1] != '\n')
		cBuf[iSize++] = '\n';
	cBuf[iSize] = 0;

	int iPos = 0;
	const int NO_SECTION	= 0;
	const int HEADER		= 1;
	const int URL			= 2;
	const int URL_STRING	= 3;
	const int BULLET		= 4;
	const int PARAGRAPH		= 6;
	IntStack curSection;
	WDL_String url;

	// Write the file back to the output with formatting
	// Formatting is:
	// Line starting with ! : Make into a header line
	// Line starting with + : Make into bullet
	// URLs are converted into <a href>.  If you suffix a link with |, the following text will be used as the desc text
	// Text "issue %d" is converted into a link into the tracker
	// 

	// Insert header if desired
	if (bFullHTML)
	{
		fputs("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n", pOut);
		fputs("<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">\n", pOut);
		fputs("<head><title>SWS/S&M Extension What's New</title></head>\n", pOut);
		fputs("<body>\n", pOut);
	}

	while (cBuf[iPos] && iPos < iSize)
	{
		if (cBuf[iPos] != '\n' && curSection.Top() != URL)
		{
			// Start of line must be a paragraph, a heading, or a list
			if (curSection.Top() == NO_SECTION)
			{
				if (cBuf[iPos] == '!')
				{
					fputs("<h3>", pOut);
					curSection.Push(HEADER);
					iPos++;
				}
				else if (cBuf[iPos] == '+')
				{
					fputs("<ul>\n", pOut);
					curSection.Push(BULLET);
				}
				else
				{
					fputs("<p>", pOut);
					curSection.Push(PARAGRAPH);
				}
			}
			
			if (curSection.Top() == BULLET && cBuf[iPos] == '+')
			{
				fputs("    <li>", pOut);
				iPos++;
			}
			else if (strncmp(&cBuf[iPos], "http://", 7) == 0)
			{
				fputs("<a href=\"", pOut);
				curSection.Push(URL);
				url.SetLen(0);
			}
			else if (_strnicmp(&cBuf[iPos], "issue ", 6) == 0)
			{
				int iIssue = atol(&cBuf[iPos+6]);
				if (iIssue != 0)
				{
					fprintf(pOut, "<a href=\"http://code.google.com/p/sws-extension/issues/detail?id=%d\">%cssue %d</a>", iIssue, cBuf[iPos], iIssue);
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
					curSection.Pop();
					break;
				case BULLET:
					if (cBuf[iPos+1] != '\n' && cBuf[iPos+1] != '+')
						fputs("<br />\n", pOut);
					else
					{
						fputs("</li>\n", pOut);
						if (cBuf[iPos+1] != '+')
						{
							fputs("</ul>\n", pOut);
							curSection.Pop();
						}
					}
					bContinue = true;
					break;
				case URL:
				case URL_STRING:
					fputs("</a>", pOut);
					curSection.Pop();
					break;
				case PARAGRAPH:
					if (cBuf[iPos+1] == '\n' || cBuf[iPos+1] == '+' || cBuf[iPos+1] == 0)
					{
						fputs("</p>\n", pOut);
						curSection.Pop();
						if (cBuf[iPos+1] != '+')
							iPos++;
					}
					else
					{
						fputs("<br />\n", pOut);
						bContinue = true;
					}
				}
			}
		}
		else // "Default" case, just write out the character
		{
			if (cBuf[iPos] == '\"') // Replace quotes with &quot;
				fputs("&quot;", pOut);
			else if (cBuf[iPos] == '&') // Replace & with &amp;
				fputs("&amp;", pOut);
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