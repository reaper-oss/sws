#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#ifndef _WIN32
#  define stricmp strcasecmp
#endif

#include <WDL/wdltypes.h>
#include <WDL/assocarray.h>
#include <WDL/ptrlist.h>
#include <WDL/wdlstring.h>

#ifdef _WIN32
#  define FNV64_IV ((WDL_UINT64)(0xCBF29CE484222325i64))
#else
#  define FNV64_IV ((WDL_UINT64)(0xCBF29CE484222325LL))
#endif

WDL_UINT64 FNV64(WDL_UINT64 h, const unsigned char* data, int sz)
{
  int i;
  for (i=0; i < sz; ++i)
  {
#ifdef _WIN32
    h *= (WDL_UINT64)0x00000100000001B3i64;
#else
    h *= (WDL_UINT64)0x00000100000001B3LL;
#endif
    h ^= data[i];
  }
  return h;
}

static bool isblank(char c)
{
  return c==' ' || c== '\t' || c=='\r' || c=='\n';
}

WDL_StringKeyedArray<char *> section_descs;

WDL_StringKeyedArray< WDL_PtrList<char> * > translations;
WDL_StringKeyedArray< WDL_StringKeyedArray<bool> * > translations_indexed;

void gotString(const char *str, size_t len, const char *secname)
{
  WDL_PtrList<char> *sec=translations.Get(secname);
  if (!sec)
  {
    sec=new WDL_PtrList<char>;
    translations.Insert(secname,sec);
  }
  WDL_StringKeyedArray<bool> *sec2 = translations_indexed.Get(secname);
  if (!sec2)
  {
    sec2 = new WDL_StringKeyedArray<bool>(true);
    translations_indexed.Insert(secname,sec2);
  }
  if (len > strlen(str)) 
  {
    fprintf(stderr,"gotString got len>strlen(str)\n");
    exit(1);
  }
  char buf[8192];
  if (len > sizeof(buf)-8)
  {
    fprintf(stderr,"argh, got string longer than 8k, adjust code accordingly or check input.\n");
    exit(1);
  }

  memcpy(buf,str,len);
  buf[len]=0;
  if (sec2->Get(buf)) return; //already in list

  sec2->Insert(buf,true);
  sec->Add(strdup(buf));
}
size_t length_of_quoted_string(char *p, bool convertRCquotesToSlash)
{
  size_t l=0;
  while (p[l])
  {
    if (convertRCquotesToSlash && p[l] == '\"' && p[l+1] == '\"')  p[l]='\\';

    if (p[l] == '\"') return l;
    if (p[l] == '\\') 
    {
      l++;
    }
    l++;
  }
  fprintf(stderr,"ERROR: mismatched quotes, check input!\n");
  exit(1);
  return -1;
}

static int uint64cmpfunc(WDL_UINT64 *a, WDL_UINT64 *b)
{
  if (*a < *b) return -1;
  if (*a > *b) return 1;
  return 0;
}
static bool isLocalizeCall(const char *p)
{
  if (strncmp(p,"__LOCALIZE",10)) return false;
  p+=10;
  if (*p != '(') 
  {    
    if (*p != '_') return false;
    while (*p == '_' || (*p >= 'A'  && *p <= 'Z')||(*p>='0' && *p <='9')) p++;
  }
  if (*p == '(') return true;
  return false;
}
WDL_UINT64 outputLine(const char *strv, int casemode)
{
  WDL_UINT64 h = FNV64_IV;
  const char *p=strv;
  while (*p)
  {
    char c = *p++;
    if (c == '\\')
    {
      if (*p == '\\'||*p == '"' || *p == '\'') h=FNV64(h,(unsigned char *)p,1);
      else if (*p == 'n') h=FNV64(h,(unsigned char *)"\n",1);
      else if (*p == 'r') h=FNV64(h,(unsigned char *)"\r",1);
      else if (*p == 't') h=FNV64(h,(unsigned char *)"\t",1);
      else if (*p == '0') h=FNV64(h,(unsigned char *)"",1);
      else 
      {
        fprintf(stderr,"ERROR: unknown escape seq in '%s' at '%s'\n",strv,p);
        exit(1);
      }
      p++;
    }
    else h=FNV64(h,(unsigned char *)&c,1);
  }
  h=FNV64(h,(unsigned char *)"",1);

  printf("%08X%08X=",(int)(h>>32),(int)(h&0xffffffff));
  while (*strv)
  {
    char c = *strv++;
    if (casemode == 2)
    {
      switch (tolower(c))
      {
        case 'o': c='0'; break;
        case 'i': c='1'; break;
        case 'e': c='3'; break;
        case 'a': c='4'; break;
        case 's': c='5'; break;
      }
    }
    else if (casemode==-1) c=tolower(c);
    else if (casemode==1) c=toupper(c);
    printf("%c",c);
  }
  printf("\n");
  return h;
}


WDL_StringKeyedArray<int> g_resdefs;
const char *getResourceDefinesFromHeader(const char *fn)
{
  g_resdefs.DeleteAll();

  FILE *fp=fopen(fn,"rb");
  if (!fp) return "error opening header";
  for (;;)
  {
    char buf[8192];
    buf[0]=0;
    fgets(buf,sizeof(buf),fp);
    if (!buf[0]) break;
    char *p = buf;
    while (*p) p++;
    while (p>buf && (p[-1] == '\r'|| p[-1] == '\n' || p[-1] == ' ')) p--;
    *p=0;
    
    if (!strncmp(buf,"#define",7))
    {
      p=buf;
      while (*p && *p != ' '&& *p != '\t') p++;
      while (*p == ' ' || *p == '\t') p++;
      char *n1 = p;
      while (*p && *p != ' '&& *p != '\t') p++;
      if (*p) *p++=0;
      while (*p == ' ' || *p == '\t') p++;
      int a = atoi(p);
      if (a && *n1)
      {
        g_resdefs.Insert(n1,a);
      }
    }
  }

  fclose(fp);
  return NULL;
}

void processRCfile(FILE *fp, const char *dirprefix)
{
  char sname[512];
  sname[0]=0;
  int depth=0;
  for (;;)
  {
    char buf[8192];
    buf[0]=0;
    fgets(buf,sizeof(buf),fp);
    if (!buf[0]) break;
    char *p = buf;
    while (*p) p++;
    while (p>buf && (p[-1] == '\r'|| p[-1] == '\n' || p[-1] == ' ')) p--;
    *p=0;

    p=buf;
    if (sname[0]) while (*p == ' ' || *p == '\t') p++;
    char *first_tok = p;
    if (!strncmp(first_tok,"CLASS",5) && isblank(first_tok[5]) && first_tok[6]=='\"') continue;

    while (*p && *p != '\t' && *p != ' ') p++;
    if (*p) *p++=0;
    while (*p == '\t' || *p == ' ') p++;
    char *second_tok = p;

    if ((!strncmp(second_tok,"DIALOG",6) && isblank(second_tok[6])) ||
        (!strncmp(second_tok,"DIALOGEX",8) && isblank(second_tok[8]))||
        (!strncmp(second_tok,"MENU",4) && isblank(second_tok[4])))
    {
      if (sname[0])
      {
        fprintf(stderr,"got %s inside a block\n",second_tok);
        exit(1);
      }
      int sec = g_resdefs.Get(first_tok);
      if (!sec) 
      {
        fprintf(stderr, "unknown dialog %s\n",first_tok);
        exit(1);
      }
      sprintf(sname,"%s%s%s_%d",dirprefix?dirprefix:"",dirprefix?"_":"",second_tok[0] == 'M' ? "MENU" : "DLG",sec);
      section_descs.Insert(sname,strdup(first_tok));
    }
    else if (sname[0] && *second_tok && *first_tok)
    {
      if (*second_tok == '"')
      {
        size_t l = length_of_quoted_string(second_tok+1,true);
        if (l>0)
        {
          gotString(second_tok+1,l,sname);
          
          // OSX menu support: store a 2nd string w/o \tshortcuts, strip '&' too
          // note: relies on length_of_quoted_string() pre-conversion above
          if (depth && strstr(sname, "MENU_"))
          {
            size_t j=0;
            char* m=second_tok+1;
            for(;;)
            {
              if (!*m || (*m == '\\' && *(m+1)=='t') || (*m=='\"' && *(m-1)!='\\')) { buf[j]=0; break; }
              if (*m != '&') buf[j++] = *m;
              m++;
            }
            if (j!=l) gotString(buf,j,sname);
          }
        }
      }
    }
    else if (!strcmp(first_tok,"BEGIN")) 
    {
      depth++;
    }
    else if (!strcmp(first_tok,"END")) 
    {
      depth--;
      if (depth<0)
      {
        fprintf(stderr,"extra END\n");
        exit(1);
      }
      if (!depth) sname[0]=0;
    }

  }
  if (depth!=0)
  {
    fprintf(stderr,"missing some ENDs at end of rc file\n");
    exit(1);
  }
}

void processCPPfile(FILE *fp)
{
  char clocsec[512];
  clocsec[0]=0;
  bool want1st=false,wantSwsCmds=false;
  for (;;)
  {
    char buf[8192];
    buf[0]=0;
    fgets(buf,sizeof(buf),fp);
    if (!buf[0]) break;
    char *p = buf;
    while (*p) p++;
    while (p>buf && (p[-1] == '\r'|| p[-1] == '\n' || p[-1] == ' ')) p--;
    *p=0;
    char *commentp =strstr(buf,"//");
    p=buf;
    while (*p && (!commentp || p < commentp)) // ignore __LOCALIZE after //
    {
      if ((p==buf || (!isalnum(static_cast<unsigned char>(p[-1])) && p[-1] != '_')) && isLocalizeCall(p))
      {
        while (*p != '(') p++;
        p++;
        while (isblank(*p)) p++;
        if (*p++ != '"')
        {
          fprintf(stderr,"Error: missing \" on '%s'\n",buf);
          exit(1);
        }
        size_t l = length_of_quoted_string(p,false);
        char *sp = p;
        p+=l+1;
        while (isblank(*p)) p++;
        if (*p++ != ',')
        {
          fprintf(stderr,"Error: missing , on '%s'\n",buf);
          exit(1);
        }
        while (isblank(*p)) p++;
        if (*p++ != '"')
        {
          fprintf(stderr,"Error: missing second \" on '%s'\n",buf);
          exit(1);
        }
        size_t l2 = length_of_quoted_string(p,false);
        char sec[512];
        memcpy(sec,p,l2);
        sec[l2]=0;
        p+=l2;
        gotString(sp,l,sec);
      }
      p++;
    }

    if (clocsec[0])
    {
      int cnt=0;
      p=buf;
      while (*p)
      {
        if (*p == '"')
        {
          size_t l = length_of_quoted_string(p+1,false);
          if (l >= 7 && !strncmp(p+1,"MM_CTX_",7))
          {
            // ignore MM_CTX_* since these are internal strings
          }
          else if (strstr(p+1,"[Internal]"))
          {
            //JFB ingore other internal strings
          }
          else if (want1st && cnt)
          {
            //JFB WANT_LOCALIZE_1ST_STRING*: ignore
          }
          else if (wantSwsCmds && cnt==1)
          {
            //JFB WANT_LOCALIZE_SWS_CMD_TABLE*: ignore 2nd strings (custom action ids)
          }
          else
          {
            //JFB WANT_LOCALIZE_SWS_CMD_TABLE*: force context for 3rd strings (menu items)
            gotString(p+1, l, wantSwsCmds && cnt==2 ? "sws_menu" : clocsec);
          }
          p += l+2;
          cnt++;
        }
        else
        {
          if (*p == '\\') p++;
          p++;
        }
      }
    }

    char *p2;
    if (commentp && (p2=strstr(commentp,"!WANT_LOCALIZE_STRINGS_BEGIN:"))) strcpy(clocsec,strstr(p2,":")+1);
    else if (commentp && (strstr(commentp,"!WANT_LOCALIZE_STRINGS_END"))) clocsec[0]=0;

    if (commentp && (p2=strstr(commentp,"!WANT_LOCALIZE_1ST_STRING_BEGIN:"))) {
      strcpy(clocsec,strstr(p2,":")+1);
      want1st=true;
    }
    else if (commentp && strstr(commentp,"!WANT_LOCALIZE_1ST_STRING_END")) {
      clocsec[0]=0;
      want1st=false;
    }

    if (commentp && (p2=strstr(commentp,"!WANT_LOCALIZE_SWS_CMD_TABLE_BEGIN:"))) {
      strcpy(clocsec,strstr(p2,":")+1);
      wantSwsCmds=true;
    }
    else if (commentp && strstr(commentp,"!WANT_LOCALIZE_SWS_CMD_TABLE_END")) {
      clocsec[0]=0;
      wantSwsCmds=false;
    }
  }
}



int main(int argc, char **argv)
{
  int x;
  int casemode=0;
  for (x=1;x<argc;x++)
  {
    if (argv[x][0] == '-')
    {
      if (!strcmp(argv[x],"--lower")) casemode = -1;
      else if (!strcmp(argv[x],"--leet")) casemode = 2;
      else if (!strcmp(argv[x],"--upper")) casemode = 1;
      else if (!strcmp(argv[x],"--template")) casemode = 3;
      else
      {
        printf("Usage: build_sample_langpack [--leet|--lower|--upper|--template] file.rc file.cpp ...\n");
        exit(1);
      }
      continue;
    }
    FILE *fp = fopen(argv[x],"rb");
    if (!fp)
    {
      fprintf(stderr,"Error opening %s\n",argv[x]);
      return 1;
    }
    size_t alen = strlen(argv[x]);
    if (alen>3 && !stricmp(argv[x]+alen-3,".rc"))
    {
      WDL_String s(argv[x]);
      WDL_String dpre;
      char *p=s.Get();
      while (*p) p++;
      while (p>=s.Get() && *p != '\\' && *p != '/') p--;
      *++p=0;
      if (p>s.Get())
      {
        p-=2;
        while (p>=s.Get() && *p != '\\' && *p != '/') p--;
        dpre.Set(++p); // get dir name portion
        if (dpre.GetLength()) dpre.Get()[dpre.GetLength()-1]=0;
      }

      s.Append("resource.h");
      const char *err=getResourceDefinesFromHeader(s.Get());     
      if (err)
      {
        fprintf(stderr,"Error reading %s: %s\n",s.Get(),err);
        exit(1);
      }
      processRCfile(fp,dpre.Get()[0]?dpre.Get():NULL);
    }
    else 
    {
      processCPPfile(fp);
    }
      
    fclose(fp);
  }

  if (casemode==3) 
  {
    // no #NAME: SWS LangPack files have to be merged with the main REAPER one
    printf("; SWS/S&M Template LangPack\n"
      "; NOTE: As you translate a string, remove the ; from the beginning of\n"
      "; the line. If the line begins with ;^, then it is an optional string,\n"
      "; and you should only modify that line if the definition in [common]\n"
      "; is not accurate for that context.\n"
      "; You can enlarge windows using 5CA1E00000000000=scale, for example:\n"
      ";     [sws_DLG_109] ; IDD_ABOUT\n"
      ";     5CA1E00000000000=1.2\n"
      "; This makes the about box 1.2x wider than default.\n"
      "; Do not change action tags like SWS:, SWS/S&M:, etc (such strings\n"
      "; would be ignored).\n"
      "; Once translated, the SWS LangPack has to be merged with the main REAPER one:\n"
      "; see http://forum.cockos.com/showpost.php?p=941893&postcount=810.\n\n");
  }
  else
  {
    printf("#NAME:%s\n",
         casemode==-1 ? "SWS/S&M English (lower case, demo)" : 
         casemode==1 ? "SWS/S&M English (upper case, demo)" : 
         casemode==2 ? "SWS/S&M English (leet-speak, demo)" : 
         "SWS/S&M English (sample language pack)");
  }

  WDL_StringKeyedArray<bool> common_found;
  {
    int pos[4096]={0,};
    fprintf(stderr,"%d sections\n",translations_indexed.GetSize());
    if (translations_indexed.GetSize() > 4096)
    {
      fprintf(stderr,"too many translation sections, check input or adjust code here\n");
      exit(1);
    }
    printf("[common]\n");
    WDL_FastString matchlist;
    WDL_AssocArray<WDL_UINT64, bool> ids(uint64cmpfunc); 
    int minpos = 0;
    for (;;)
    {
      int matchcnt=0;
      matchlist.Set("");
      const char *str=NULL;
      for(x=minpos;x<translations_indexed.GetSize();x++)
      {
        const char *secname;
        WDL_StringKeyedArray<bool> *l = translations_indexed.Enumerate(x,&secname);
        int sz=l->GetSize();
        if (!str)
        {
          if (x>minpos) 
          {
            memset(pos,0,sizeof(pos)); // start over
            minpos=x;
          }
          while (!str && pos[x]<sz)
          {
            l->Enumerate(pos[x]++,&str); 
            if (common_found.Get(str)) str=NULL; // skip if we've already analyzed this string
          }
          if (str) matchlist.Set(secname); 
        }
        else 
        {
          while (pos[x] < sz)
          {
            const char *tv=NULL;
            l->Enumerate(pos[x],&tv);
            int c = strcmp(tv,str);
            if (c>0) break; 
            pos[x]++;
            if (!c)
            {
              matchlist.Append(", ");
              matchlist.Append(secname);
              matchcnt++;
              break;
            }
          }
        }
      }
      if (matchcnt>0)
      {
        common_found.Insert(str,true);
        //printf("; used by: %s\n",matchlist.Get());
        if (casemode==3) printf(";");
        WDL_UINT64 a = outputLine(str,casemode);
        if (ids.Get(a))
        {
          fprintf(stderr,"duplicate hash for strings in common section, hope this is OK.. 64 bit hash fail!\n");
          exit(1);
        }
//        printf("\n");
        ids.Insert(a,true);
      }
      if (minpos == x-1) break;
    }
    printf("\n");
  }

  for(x=0;x<translations.GetSize();x++)
  {
    const char *nm=NULL;
    WDL_PtrList<char> *p = translations.Enumerate(x,&nm);
    if (x) printf("\n");
    char *secinfo = section_descs.Get(nm);
    printf("[%s]%s%s\n",nm,secinfo?" ; ":"", secinfo?secinfo:"");
    int a;
    int y;
    WDL_AssocArray<WDL_UINT64, bool> ids(uint64cmpfunc); 
    for (a=0;a<2;a++)
    {
      for (y=0;y<p->GetSize();y++)
      {
        char *strv=p->Get(y);
        if (!!common_found.Get(strv) != a) continue;

        if (a) printf(";^");
        else if (casemode==3) printf(";");

        WDL_UINT64 a = outputLine(strv,casemode);
        if (ids.Get(a))
        {
          fprintf(stderr,"duplicate hash for strings in section, hope this is OK.. 64 bit hash fail!\n");
          exit(1);
        }
        ids.Insert(a,true);
      }
    }
  }
  return 0;
}
