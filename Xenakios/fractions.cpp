/******************************************************************************
/ Fractions.cpp
/
/ Copyright (c) 2009 Tim Payne (SWS), original code by Xenakios
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

typedef struct t_Notevalue_struct
{
	const char *NotevalueStr;
	double NotevalueBeats;
} t_Notevalue_struct;

t_Notevalue_struct g_NoteValues[]=
{

	{"2",8},
	{"1",4},
	{"1/2",2},
	{"1/4T",2.0/3},
	{"1/4",1},
	{"1/8",1.0/2},
	{"1/16",1.0/4},
	{"1/32",1.0/8},
	{"1/64",1.0/16},
	{"1/128",1.0/32},
	{NULL,0}

};

double GetBeatValueFromTable(int indx)
{
	double TheResult=0.0;
	TheResult=g_NoteValues[indx].NotevalueBeats;
	return TheResult;
}

static double flexi_atof(const char *p)
{
  if (!p || !*p) return 0.0;
  char buf[512];
  lstrcpyn(buf,p,sizeof(buf));
  char *n=buf;
  while (*n)
  {
    if (*n == ',') *n='.';
    n++;
  }
  return atof(buf);
}

double parseFrac(const char *buf)
{
  double v=flexi_atof(buf);
  buf=strstr(buf,"/");
  if (buf)
  {
    double d=flexi_atof(buf+1);
    if (fabs(d) > 0.1) 
      v/=d;
  }
  return v;
}

void InitFracBox(HWND hwnd, const char *buf)
{
	bool bFound = false;
	for (int x = 0; g_NoteValues[x].NotevalueStr; x ++)
	{
		int r = (int)SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)g_NoteValues[x].NotevalueStr);
		if (!bFound && !strcmp(buf,g_NoteValues[x].NotevalueStr))
		{
			bFound = true;
			SendMessage(hwnd, CB_SETCURSEL, r, 0);
		}
	}

	if (!bFound)
		SetWindowText(hwnd, buf);
}
