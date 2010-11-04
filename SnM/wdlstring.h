/*
    WDL - wdlstring.h
    Copyright (C) 2005 and later, Cockos Incorporated
  
    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.

*/

/*

  This file provides a simple class for variable-length string manipulation.
  It provides only the simplest features, and does not do anything confusing like
  operator overloading. It uses a WDL_HeapBuf for internal storage.

  You can do Set, Get, Append, Insert, and SetLen.. that's about it

  JFB mod --->
     Avoid many strlen() calls thanks to a new member attribute m_length (faster).
     One drawback is that if the string is shorten manually (without using SetLen(n, true)),
     things can go wrong..
  JFB mod <---
  
*/

#ifndef _WDL_STRING_H_
#define _WDL_STRING_H_

#include "heapbuf.h"
#include <stdio.h>
#include <stdarg.h>

#ifndef WDL_STRING_IMPL_ONLY
class WDL_String
{
public:
//JFB mod --->
//  WDL_String(int hbgran) : m_hb(hbgran WDL_HEAPBUF_TRACEPARM("WDL_String(4)"))
  WDL_String(int hbgran) : m_hb(hbgran WDL_HEAPBUF_TRACEPARM("WDL_String(4)")), m_length(0)
//JFB mod <---
  {
  }
//JFB mod --->
//  WDL_String(const char *initial=NULL, int initial_len=0) : m_hb(128 WDL_HEAPBUF_TRACEPARM("WDL_String"))
  WDL_String(const char *initial=NULL, int initial_len=0) : m_hb(128 WDL_HEAPBUF_TRACEPARM("WDL_String")), m_length(0)
//JFB mod <---
  {
    if (initial) Set(initial,initial_len);
  }
//JFB mod --->
//  WDL_String(WDL_String &s) : m_hb(128 WDL_HEAPBUF_TRACEPARM("WDL_String(2)"))
  WDL_String(WDL_String &s) : m_hb(128 WDL_HEAPBUF_TRACEPARM("WDL_String(2)")), m_length(0)
//JFB mod <---
  {
    Set(s.Get());
  }
//JFB mod --->
//  WDL_String(WDL_String *s) : m_hb(128 WDL_HEAPBUF_TRACEPARM("WDL_String(3)"))
  WDL_String(WDL_String *s) : m_hb(128 WDL_HEAPBUF_TRACEPARM("WDL_String(3)")), m_length(0)
//JFB mod <---
  {
    if (s && s != this) Set(s->Get());
  }

  ~WDL_String()
  {
  }
#define WDL_STRING_PREFIX
#else
#define WDL_STRING_PREFIX WDL_String::
#endif

  void WDL_STRING_PREFIX Set(const char *str, int maxlen
#ifdef WDL_STRING_INTF_ONLY
      =0); 
#else
#ifdef WDL_STRING_IMPL_ONLY
    )
#else
    =0)
#endif
  {
    int s=strlen(str);
    if (maxlen && s > maxlen) s=maxlen;   
    if (!s && !m_hb.GetSize()) return; // do nothing if setting to empty and not allocated

    char *newbuf=(char*)m_hb.Resize(s+1,false);
    if (newbuf) 
    {
      memcpy(newbuf,str,s);
      newbuf[s]=0;

      //JFB added --->
      m_length = s;
      //JFB <---
    }
  }
#endif

  void WDL_STRING_PREFIX Append(const char *str, int maxlen
#ifdef WDL_STRING_INTF_ONLY
      =0); 
#else
#ifdef WDL_STRING_IMPL_ONLY
    )
#else
    =0)
#endif
  {
    int s=strlen(str);
    if (maxlen && s > maxlen) s=maxlen;
    if (!s) return; // do nothing if setting to empty and not allocated

    //JFB mod --->
//    int olds=strlen(Get());
    int olds=m_length;
    //JFB <---

    char *newbuf=(char*)m_hb.Resize(olds + s + 1,false);
    if (newbuf)
    {
      memcpy(newbuf + olds, str, s);
      newbuf[olds+s]=0;

      //JFB added --->
      m_length += s;
      //JFB <---
    }
  }
#endif

  void WDL_STRING_PREFIX DeleteSub(int position, int len)
#ifdef WDL_STRING_INTF_ONLY
    ;
#else
    {
	  char *p=Get();
	  if (!p || !*p) return;

      //JFB mod --->
//	  int l=strlen(p);
      int l=m_length;
      //JFB <---

	  if (position < 0 || position >= l) return;
	  if (position+len > l) len=l-position;
	  memmove(p+position,p+position+len,l-position-len+1); // +1 for null

      //JFB added --->
	  m_length -= len;
      //JFB <---
  }
#endif

  void WDL_STRING_PREFIX Insert(const char *str, int position, int maxlen
#ifdef WDL_STRING_INTF_ONLY
      =0); 
#else
#ifdef WDL_STRING_IMPL_ONLY
    )
#else
    =0)
#endif
  {
      //JFB mod --->
//	  int sl=strlen(Get());
      int sl=m_length, olds=m_length;
      //JFB <---

	  if (position > sl) position=sl;

	  int ilen=strlen(str);
	  if (maxlen > 0 && maxlen < ilen) ilen=maxlen;

	  Append(str);  //JFB indirectly updates m_length but see below..
	  char *cur=Get();

      memmove(cur+position+ilen,cur+position,sl-position);
	  memcpy(cur+position,str,ilen);
	  cur[sl+ilen]=0;

      //JFB added --->
	  m_length = olds+ilen;
      //JFB <---
  }
#endif

  void WDL_STRING_PREFIX SetLen(int length, bool resizeDown
#ifdef WDL_STRING_INTF_ONLY
      =false); 
#else
#ifdef WDL_STRING_IMPL_ONLY
    )
#else
    =false)
#endif
  {                       // can use to resize down, too, or resize up for a sprintf() etc
    char *b=(char*)m_hb.Resize(length+1,resizeDown);
    //JFB mod --->
//    if (b) b[length]=0;
    if (b)
	{
		b[length]=0;
		if (resizeDown && (length < m_length)) m_length = length;
		else m_length = strlen(b); //JFB lazy here
	}
    //JFB <---
  }
#endif

  void WDL_STRING_PREFIX SetFormatted(int maxlen, const char* fmt, ...) 
#ifdef WDL_STRING_INTF_ONLY
    ; 
#else
  {
    char* b= (char*) m_hb.Resize(maxlen+1,false);
  	va_list arglist;
		va_start(arglist, fmt);
    #ifdef _WIN32
		int written = _vsnprintf(b, maxlen, fmt, arglist);
    #else
		int written = vsnprintf(b, maxlen, fmt, arglist);
    #endif
    if (written < 0) written = 0;
		va_end(arglist);
    b[written] = '\0';

    //JFB added --->
    m_length = written;
    //JFB <---
  }
#endif

  void WDL_STRING_PREFIX AppendFormatted(int maxlen, const char* fmt, ...) 
#ifdef WDL_STRING_INTF_ONLY
    ; 
#else
  {
    //JFB mod --->
//    int offs=strlen(Get());
    int offs=m_length;
    //JFB <---

    char* b= (char*) m_hb.Resize(offs+maxlen+1,false)+offs;
  	va_list arglist;
		va_start(arglist, fmt);
    #ifdef _WIN32
		int written = _vsnprintf(b, maxlen, fmt, arglist);
    #else
		int written = vsnprintf(b, maxlen, fmt, arglist);
    #endif
    if (written < 0) written = 0;
		va_end(arglist);
    b[written] = '\0';

    //JFB added --->
    m_length += written;
    //JFB <---
  }
#endif

  void WDL_STRING_PREFIX Ellipsize(int minlen, int maxlen)
#ifdef WDL_STRING_INTF_ONLY
    ;
#else
  {
    char* b = Get();

    //JFB mod --->
//    if ((int) strlen(b) > maxlen) {
    if (m_length > maxlen) {
    //JFB <---
      int i;
      for (i = maxlen-4; i >= minlen; --i) {
        if (b[i] == ' ') {
          strcpy(b+i, "...");
          break;
        }
      }
      if (i < minlen) strcpy(b+maxlen-4, "...");    
    }

    //JFB added --->
    m_length = strlen(b); // ok, lazy here..  
    //JFB <---
  }
#endif

#ifndef WDL_STRING_IMPL_ONLY
  char *Get()
  {
    char *p=NULL;
    if (m_hb.GetSize()) p=(char *)m_hb.Get();
    if (p) return p;
    static char c; c=0; 
    return &c; // don't return "", in case it gets written to.
  }

  int GetLength()
  {
    //JFB mod --->
//    return (int)strlen(Get());
    return m_length;
    //JFB <---
  }

  //JFB mod --->
//  private:
  protected:
  //JFB <---
    WDL_HeapBuf m_hb;
    //JFB added --->
    int m_length;
    //JFB <---
};
#endif

#endif

