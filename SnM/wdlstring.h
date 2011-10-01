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

  Optionnal WDL_STRING_FAST ifdef: good for large strings & repetitive processing.
  When defined, this will avoid many calls to strlen() but the string's length
  must not be changed 'manually' (use SetLen(7) rather than Get()[8]=0, etc..)
*/

#ifndef _WDL_STRING_H_
#define _WDL_STRING_H_

#include "heapbuf.h"
#include <stdio.h>
#include <stdarg.h>


#ifndef WDL_STRING_INTF_ONLY
static int strminlen(const char* str, int n)
{
  if (n > 0) {
    const char* p = str;
    while (p-str < n && *p) ++p;
    return (int)(p-str);
  }
  return (int)strlen(str);
}
#endif


#ifndef WDL_STRING_IMPL_ONLY
class WDL_String
{
public:
  WDL_String(int hbgran) : m_hb(hbgran WDL_HEAPBUF_TRACEPARM("WDL_String(4)"))
#ifdef WDL_STRING_FAST
  , m_length(0)
#endif
  {
  }
  WDL_String(const char *initial=NULL, int initial_len=0) : m_hb(128 WDL_HEAPBUF_TRACEPARM("WDL_String"))
#ifdef WDL_STRING_FAST
  , m_length(0)
#endif
  {
    if (initial) Set(initial,initial_len);
  }
  WDL_String(WDL_String &s) : m_hb(128 WDL_HEAPBUF_TRACEPARM("WDL_String(2)"))
#ifdef WDL_STRING_FAST
  , m_length(0)
#endif
  {
    Set(s.Get());
  }
  WDL_String(WDL_String *s) : m_hb(128 WDL_HEAPBUF_TRACEPARM("WDL_String(3)"))
#ifdef WDL_STRING_FAST
  , m_length(0)
#endif
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
    // this would be ideal
    //int s = (maxlen ? strnlen(str, maxlen) : strlen(str));
    int s = strminlen(str, maxlen);
    if (!s && !m_hb.GetSize())
    {
#ifdef WDL_STRING_FAST
      m_length = 0;
#endif
      return; // do nothing if setting to empty and not allocated
    }

    if (char* newbuf=(char*)m_hb.Resize(s+1,false)) 
    {
      memcpy(newbuf,str,s);
      newbuf[s]=0;
#ifdef WDL_STRING_FAST
      m_length = s;
#endif
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
    int s = strminlen(str, maxlen);
    if (!s) return;

    int olds=GetLength();
    if (char* newbuf=(char*)m_hb.Resize(olds+s+1,false))
    {
      memcpy(newbuf + olds, str, s);
      newbuf[olds+s]=0;
#ifdef WDL_STRING_FAST
      m_length += s;
#endif
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

	  int l=GetLength();
	  if (position < 0 || position >= l) return;
	  if (position+len > l) len=l-position;
	  memmove(p+position,p+position+len,l-position-len+1); // +1 for null
#ifdef WDL_STRING_FAST
	  m_length -= len;
#endif
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
    int sl=GetLength();
    if (position > sl) position=sl;

    int ilen=strminlen(str,maxlen);
    if (!ilen) return;

    if (char* newbuf=(char*)m_hb.Resize(sl+ilen+1,false))
    {
      memmove(newbuf+position+ilen,newbuf+position,sl-position);
      memcpy(newbuf+position,str,ilen);
      newbuf[sl+ilen]=0;
#ifdef WDL_STRING_FAST
      m_length += ilen;
#endif
    }
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
    if (char* b=(char*)m_hb.Resize(length+1,resizeDown))
    {
      b[length]=0;
#ifdef WDL_STRING_FAST
      m_length = (length < m_length ? length : m_length); // string len != allocation size
#endif
    }
  }
#endif

  void WDL_STRING_PREFIX SetFormatted(int maxlen, const char* fmt, ...) 
#ifdef WDL_STRING_INTF_ONLY
    ; 
#else
  {
    if (char* b=(char*)m_hb.Resize(maxlen+1,false))
    {
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
#ifdef WDL_STRING_FAST
      m_length = written;
#endif
    }
  }
#endif

  void WDL_STRING_PREFIX AppendFormatted(int maxlen, const char* fmt, ...) 
#ifdef WDL_STRING_INTF_ONLY
    ; 
#else
  {
    int offs=GetLength();
    if (char* b=(char*)m_hb.Resize(offs+maxlen+1,false)+offs)
    {
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
#ifdef WDL_STRING_FAST
      m_length += written;
#endif
    }
  }
#endif

  void WDL_STRING_PREFIX Ellipsize(int minlen, int maxlen)
#ifdef WDL_STRING_INTF_ONLY
    ;
#else
  {
    if (GetLength() > maxlen)
    {
      char* b = Get();
      int i;
      for (i = maxlen-4; i >= minlen; --i) {
        if (b[i] == ' ') {
          strcpy(b+i, "...");
#ifdef WDL_STRING_FAST
          m_length = i+3;
#endif
          break;
        }
      }
      if (i < minlen && maxlen > 4) {
        strcpy(b+maxlen-4, "...");
#ifdef WDL_STRING_FAST
        m_length = maxlen-1;
#endif
      }
    }
  }
#endif

#ifndef WDL_STRING_IMPL_ONLY
  char *Get()
  {
    char *p=NULL;
    if (m_hb.GetSize()) p=(char *)m_hb.Get();
    if (p) return p;

#ifdef WDL_STRING_FAST
    m_length=0;
#endif
    static char c; c=0; 
    return &c; // don't return "", in case it gets written to.
  }

  int GetLength() {
#ifdef WDL_STRING_FAST
  return (m_hb.GetSize()?m_length:0);
#else
  return (int)strlen(Get());
#endif
}

  private:
    WDL_HeapBuf m_hb;
#ifdef WDL_STRING_FAST
    int m_length;
#endif
};
#endif

#endif

