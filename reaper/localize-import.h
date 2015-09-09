/************************************************
** Copyright (C) 2006-2012, Cockos Incorporated
*/

// used by plug-ins to access imported localization
// usage: 
// #define LOCALIZE_IMPORT_PREFIX "midi_" (should be the directory name with _)
// #include "../localize-import.h"
// ...
// IMPORT_LOCALIZE_VST(hostcb) 
// ...or
// IMPORT_LOCALIZE_RPLUG(rec)
//


#ifdef _REAPER_LOCALIZE_H_
#error you must include localize-import.h before localize.h, sorry
#endif

static const char *(*importedLocalizeFunc)(const char *str, const char *subctx, int flags);
static void (*importedLocalizeMenu)(const char *rescat, HMENU hMenu, LPCSTR lpMenuName);
static DLGPROC (*importedLocalizePrepareDialog)(const char *rescat, HINSTANCE hInstance, const char *lpTemplate, DLGPROC dlgProc, LPARAM lPAram, void **ptrs, int nptrs);


#define IMPORT_LOCALIZE_VST(hostcb) \
    *(VstIntPtr *)&importedLocalizeFunc = hostcb(NULL,0xdeadbeef,0xdeadf00d,0,(void*)"__localizeFunc",0.0); \
    *(VstIntPtr *)&importedLocalizeMenu = hostcb(NULL,0xdeadbeef,0xdeadf00d,0,(void*)"__localizeMenu",0.0); \
    *(VstIntPtr *)&importedLocalizePrepareDialog = hostcb(NULL,0xdeadbeef,0xdeadf00d,0,(void*)"__localizePrepareDialog",0.0);

#define IMPORT_LOCALIZE_RPLUG(rec) \
  *(void **)&importedLocalizeFunc = rec->GetFunc("__localizeFunc"); \
  *(void **)&importedLocalizeMenu = rec->GetFunc("__localizeMenu"); \
  *(void **)&importedLocalizePrepareDialog = rec->GetFunc("__localizePRepareDialog");

const char *__localizeFunc(const char *str, const char *subctx, int flags)
{
  if (str && importedLocalizeFunc) return importedLocalizeFunc(str,subctx,flags);
  return str;
}

HMENU __localizeLoadMenu(HINSTANCE hInstance, const char *lpMenuName)
{
  HMENU menu = LoadMenu(hInstance,lpMenuName);
  if (menu && importedLocalizeMenu) importedLocalizeMenu(LOCALIZE_IMPORT_PREFIX,menu,lpMenuName);
  return menu;
}

HWND __localizeDialog(HINSTANCE hInstance, const char *lpTemplate, HWND hwndParent, DLGPROC dlgProc, LPARAM lParam, int mode)
{
  void *p[4];
  if (importedLocalizePrepareDialog)
  {
    DLGPROC newDlg  = importedLocalizePrepareDialog(LOCALIZE_IMPORT_PREFIX,hInstance,lpTemplate,dlgProc,lParam,p,sizeof(p)/sizeof(p[0]));
    if (newDlg)
    {
      dlgProc = newDlg;
      lParam = (LPARAM)(INT_PTR)p;
    }
  }
  switch (mode)
  {
    case 0: return CreateDialogParam(hInstance,lpTemplate,hwndParent,dlgProc,lParam);
    case 1: return (HWND) (INT_PTR)DialogBoxParam(hInstance,lpTemplate,hwndParent,dlgProc,lParam);
  }
  return 0;
}

static void localizeKbdSection(KbdSectionInfo *sec, const char *nm)
{
  if (importedLocalizeFunc)
  {
    int x;
    if (sec->action_list) for (x=0;x<sec->action_list_cnt; x++)
    {
      KbdCmd *p = &sec->action_list[x];
      if (p->text) p->text = __localizeFunc(p->text,nm,2/*LOCALIZE_FLAG_NOCACHE*/);
    }
  }
}
