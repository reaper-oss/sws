/******************************************************************************
/ Fractions.cpp
/
/ Copyright (c) 2009 Tim Payne (SWS), original code by Xenakios
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

typedef struct t_Notevalue_struct
{
	char *NotevalueStr;
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
	{NULL,NULL}

};

double GetBeatValueFromTable(int indx)
{
	double TheResult=0.0;
	TheResult=g_NoteValues[indx].NotevalueBeats;
	return TheResult;
}

static void showFrac(double &val, char *buf)
{
#define FLOATNEAR(x,y) (fabs(x-y)<0.00000001)
  if (FLOATNEAR(val,floor(val))) { val=floor(val); if (buf) sprintf(buf,"%d",(int)val); }
  else if (FLOATNEAR(val,ceil(val))) { val=ceil(val); if (buf) sprintf(buf,"%d",(int)val); }
  else 
  {
    int n,d;
    for (n = 1; n <= 16; n ++)
    {
      if (val < n/4096 - 0.000001) continue;
      
      int mv=n<4?4096:128;

      for (d = 2; d <= mv; d ++)
      {
        double v= (double)n/(double)d;
        if (FLOATNEAR(val,v))
        {
          if (buf) sprintf(buf,"%d/%d",n,d);
          val=v;
          return;
        }
        if (val > v) break; // our fractions only get smaller, so early out this
      }
    }
    if (buf) sprintf(buf,"%.3f",val);
  }
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



char *fraction_strs[]=
{
  "1/128",
  "1/64",
  "1/32",
  "1/16",
  "1/12",
  "1/8",
  "1/4",
  "1/2",
  "1",
  "2",
  "4",
  NULL
};

char *notevalue_strs[]=
{
  "1/128",
  "1/64",
  "1/32",
  "1/16",
  "1/12",
  "1/8",
  "1/4",
  "1/8",
  "4th",
  "2nd",
  "1",
  NULL
};

void InitFracBox(HWND hwnd, char *buf)
{

  int x;
  int a=0;
  //for (x = 0; fraction_strs[x]; x ++)
  for (x = 0; g_NoteValues[x].NotevalueStr; x ++)
  {
    //int r=SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)fraction_strs[x]);
	int r=(int)SendMessage(hwnd,CB_ADDSTRING,0,(LPARAM)g_NoteValues[x].NotevalueStr);
    //if (!a && !strcmp(buf,fraction_strs[x]))
	if (!a && !strcmp(buf,g_NoteValues[x].NotevalueStr))
    {
      a=1;
      SendMessage(hwnd,CB_SETCURSEL,r,0);
    }
  }

  if (!a)
    SetWindowText(hwnd,buf);
}
